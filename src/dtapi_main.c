/*
 * tel-plugin-dbus_tapi
 *
 * Copyright (c) 2013 Samsung Electronics Co. Ltd. All rights reserved.
 * Copyright (c) 2013 Intel Corporation. All rights reserved.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "dtapi_main.h"
#include "dtapi_call.h"
#include "dtapi_gps.h"
#include "dtapi_modem.h"
#include "dtapi_network.h"
#include "dtapi_phonebook.h"
#include "dtapi_sap.h"
#include "dtapi_sat.h"
#include "dtapi_sim.h"
#include "dtapi_sms.h"
#include "dtapi_ss.h"
#include "dtapi_util.h"
#include "dtapi_manager.h"

#include <tel_generated_code.h>

#include <glib.h>
#include <tcore.h>
#include <server.h>
#include <communicator.h>
#include <core_object.h>
#include <plugin.h>
#include <storage.h>

#include "sat_manager.h"

#define DBUS_COMMUNICATOR_NAME "dbus"

typedef struct {
	TcorePlugin *plugin;
	Communicator *comm;
	Server *server;
	guint bus_id;

	GHashTable *objects;
	GDBusObjectManagerServer *manager;
} CustomData;

static void __set_telephony_ready(Server *server)
{
	TcoreStorage *strg;
	gboolean ret;

	strg = tcore_server_find_storage(server, "vconf");
	ret = tcore_storage_set_bool(strg, STORAGE_KEY_READY, TRUE);
	if (ret == TRUE) {
		dbg("Set Telephony Ready: [%s]", ret ? "SUCCESS" : "FAIL");
	}
}

static gboolean __is_server_notification(gint cmd)
{
	return ((cmd & (TCORE_NOTIFICATION | 0x0FF00000))
			== TCORE_SERVER_NOTIFICATION);
}

static void __add_modem(TcorePlugin *plugin, CustomData *ctx)
{
	TelephonyObjectSkeleton *object;
	CoreObject *co;
	guint co_type;
	char *path = NULL;
	GSList *co_list, *co_elem;
	const char *cp_name;

	dbg("Entry");

	/* Get CP Name */
	cp_name = tcore_server_get_cp_name_by_plugin(plugin);
	if (cp_name == NULL) {
		err("CP Name is NULL");
		return;
	}

	co_list = tcore_plugin_ref_core_objects(plugin);
	if (co_list == NULL) {
		err("No Core Objects");
		return;
	}

	path = g_strdup_printf("%s/%s", TELEPHONY_OBJECT_PATH, cp_name);
	dbg("PATH: [%s]", path);

	object = g_hash_table_lookup(ctx->objects, path);
	if (object == NULL) {
		/* Create a new D-Bus object at path /org/tizen/telephony/<cp_name> */
		object = telephony_object_skeleton_new(path);
		if (object == NULL) {
			err("Creation of new DBUS Interface object failed");
			goto OUT;
		}
		dbg("New DBUS Interface object created!!! (object = %p)", object);
		g_hash_table_insert(ctx->objects, g_strdup(path), object);
	} else {		/* DBUS object already exists */
		dbg("DBUS Interface object already created!!! (object = %p)", object);
		goto OUT;
	}

	/* Export interfaces rooted at /org/tizen/telephony/<cp_name> */
	for (co_elem = co_list; co_elem; co_elem = co_elem->next) {
		co = co_elem->data;
		co_type = tcore_object_get_type(co);
		switch (co_type) {
		case CORE_OBJECT_TYPE_MODEM:
			dtapi_setup_modem_interface(object, plugin);
		break;

		case CORE_OBJECT_TYPE_CALL:
			dtapi_setup_call_interface(object, plugin);
		break;

		case CORE_OBJECT_TYPE_NETWORK:
			dtapi_setup_network_interface(object, plugin);
		break;

		case CORE_OBJECT_TYPE_SS:
			dtapi_setup_ss_interface(object, plugin);
		break;

		case CORE_OBJECT_TYPE_SMS:
			dtapi_setup_sms_interface(object, plugin);
		break;

		case CORE_OBJECT_TYPE_SIM:
			dtapi_setup_sim_interface(object, plugin);
		break;

		case CORE_OBJECT_TYPE_PHONEBOOK:
			dtapi_setup_phonebook_interface(object, plugin);
		break;

		case CORE_OBJECT_TYPE_SAP:
			dtapi_setup_sap_interface(object, plugin);
		break;

		case CORE_OBJECT_TYPE_GPS:
			dtapi_setup_gps_interface(object, plugin);
		break;

		case CORE_OBJECT_TYPE_SAT:
			dtapi_setup_sat_interface(object, plugin, ctx->plugin);
		break;

		default:
			msg("Skipping: [%u]", co_type);
		continue;
		}
	}
	/* Export the object */
	g_dbus_object_manager_server_export(ctx->manager, G_DBUS_OBJECT_SKELETON(object));

	if (g_dbus_object_manager_server_get_connection (ctx->manager) != NULL)
		__set_telephony_ready(ctx->server);

	g_object_unref(object);

OUT:
	/* Freeing memory */
	g_free(path);
}

static void __refresh_modems(CustomData *ctx)
{
	GSList *mp_list;

	if (!ctx->manager) {
		err("Lost Manager. Can't continue");
		return;
	}

	/* Expose modem plugin interface */
	mp_list = tcore_server_get_modem_plugin_list(ctx->server);
	g_slist_foreach(mp_list, (GFunc)__add_modem, ctx);
	g_slist_free(mp_list);
}

static TelReturn send_notification(Communicator *comm,
	TcorePlugin *plugin, gint command,
	unsigned int data_len, const void *data)
{
	CustomData *ctx = NULL;
	gboolean ret = FALSE;

	dbg("Notification!!! command: [0x%x] data: [%p] data_len: [%d]",
			command, data, data_len);

	if (command == TCORE_SERVER_NOTIFICATION_ADDED_COMMUNICATOR) {
		/* In case of ADDED_COMMUNICATOR, do NOT progress */
		return TEL_RETURN_SUCCESS;
	}

	ctx = tcore_communicator_ref_user_data(comm);
	tcore_check_return_value_assert((ctx != NULL),
			TEL_RETURN_INVALID_PARAMETER);

	/*
	 * Notifications are classified into -
	 *	Server (System) notifications
	 *	Module notifications
	 */
	if (__is_server_notification(command) == TRUE) {
		dbg("Server (System) Notification");

		switch (command) {
		case TCORE_SERVER_NOTIFICATION_ADDED_MODEM_PLUGIN:
			dbg("Modem Plug-in (%s) is added... "
				"Exporting interfaces for the modem",
				tcore_server_get_cp_name_by_plugin((TcorePlugin*)data));

			__add_modem((TcorePlugin*)data, ctx);

			ret = TRUE;
		break;

		default:
			warn("Unsupported System notification: (0x%x)", command);
		break;
		}
	} else {
		TelephonyObjectSkeleton *object;
		const char *cp_name;
		char *path;

		cp_name = tcore_server_get_cp_name_by_plugin(plugin);
		tcore_check_return_value_assert((cp_name != NULL), TEL_RETURN_FAILURE);

		dbg("CP Name: [%s]", cp_name);
		path = g_strdup_printf("%s/%s", TELEPHONY_OBJECT_PATH, cp_name);

		/* Look-up Hash table for Object */
		object = g_hash_table_lookup(ctx->objects, path);
		dbg("Path: [%s] Interface object: [%p]", path, object);
		g_free(path);

		tcore_check_return_value((object != NULL), TEL_RETURN_FAILURE);

		switch (command & (TCORE_NOTIFICATION | 0x0FF00000)) {
		case TCORE_NOTIFICATION_CALL:
			ret = dtapi_handle_call_notification(object,
					plugin, command, data_len, data);
		break;

		case TCORE_NOTIFICATION_SS:
			ret = dtapi_handle_ss_notification(object,
					plugin, command, data_len, data);
		break;

		case TCORE_NOTIFICATION_PS:
			warn("PS Notification [0x%x]... Not handled!!!", command);
		break;

		case TCORE_NOTIFICATION_SIM:
			ret = dtapi_handle_sim_notification(object,
					plugin, command, data_len, data);
		break;

		case TCORE_NOTIFICATION_SAP:
			ret = dtapi_handle_sap_notification(object,
					plugin, command, data_len, data);
		break;

		case TCORE_NOTIFICATION_PHONEBOOK:
			ret = dtapi_handle_phonebook_notification(object,
					plugin, command, data_len, data);
		break;

		case TCORE_NOTIFICATION_MODEM:
			ret = dtapi_handle_modem_notification(object,
					plugin, command, data_len, data);
		break;

		case TCORE_NOTIFICATION_SMS:
			ret = dtapi_handle_sms_notification(object,
					plugin, command, data_len, data);
		break;

		case TCORE_NOTIFICATION_SAT:
			ret = dtapi_handle_sat_notification(object,
					plugin, ctx->plugin, command, data_len, data);
		break;

		case TCORE_NOTIFICATION_NETWORK:
			ret = dtapi_handle_network_notification(object,
					plugin, command, data_len, data);
		break;

		case TCORE_NOTIFICATION_GPS:
			ret = dtapi_handle_gps_notification(object,
					plugin, command, data_len, data);
		break;

		case TCORE_NOTIFICATION_CUSTOM:
			warn("Custom Notification [0x%x]... Not handled!!!", command);
		break;

		default:
			err("Unknown command [0x%x]", command);
		break;
		}
	}

	if (ret == TRUE)
		return TEL_RETURN_SUCCESS;
	else
		return TEL_RETURN_FAILURE;
}

static void on_name_acquired(GDBusConnection *conn, const gchar *name, gpointer user_data)
{
	dbg("Name Acquired %s", name);
}

static void on_name_lost(GDBusConnection *conn, const gchar *name, gpointer user_data)
{
	if (conn == NULL) {
		err("Connection to the bus can't be made");
		return;
	}

	dbg("Name Lost %s", name);

	/* TODO: unregister the objects */
}

static void on_bus_acquired(GDBusConnection *conn, const gchar *name, gpointer user_data)
{
	CustomData *ctx = user_data;
	GError *error = NULL;
	TelephonyManager *mgr = dtapi_manager_new(ctx->server);

	if (FALSE == g_dbus_interface_skeleton_export(G_DBUS_INTERFACE_SKELETON(mgr),
					conn, TELEPHONY_OBJECT_PATH, &error)) {
		err("Unable to export Manager interface [%d] => [%s]", error->code, error->message);
		g_error_free(error);
		return;
	}

	/* Refresh Object */
	__refresh_modems(ctx);

	/* Export all objects */
	g_dbus_object_manager_server_set_connection(ctx->manager, conn);
	dbg("Aquire DBUS - COMPLETE");
}

TcoreCommunicatorOps ops = {
	.send_notification = send_notification,
};

gboolean dtapi_plugin_init(TcorePlugin *plugin)
{
	Communicator *comm;
	CustomData *data;

	data = tcore_malloc0(sizeof(CustomData));

	data->plugin = plugin;
	data->server = tcore_plugin_ref_server(plugin);

	comm = tcore_communicator_new(plugin, DBUS_COMMUNICATOR_NAME, &ops);
	tcore_communicator_link_user_data(comm, data);
	data->comm = comm;

	data->objects = g_hash_table_new_full(g_str_hash, g_str_equal, g_free, NULL);

	/* Create Object Manager rooted at /org/tizen/telephony */
	data->manager = g_dbus_object_manager_server_new(TELEPHONY_OBJECT_PATH);

	/* Request bus name */
	data->bus_id = g_bus_own_name(G_BUS_TYPE_SYSTEM, TELEPHONY_SERVICE,
					G_BUS_NAME_OWNER_FLAGS_REPLACE,
					on_bus_acquired, on_name_acquired, on_name_lost,
					data, NULL);
	dbg("id=[%d]", data->bus_id);

	sat_manager_init(plugin, data->objects, data->server, data->manager);

	return TRUE;
}

void dtapi_plugin_deinit(TcorePlugin *plugin)
{
	CustomData *data;
	Communicator *comm;
	Server *s = tcore_plugin_ref_server(plugin);

	comm = tcore_server_find_communicator(s, DBUS_COMMUNICATOR_NAME);
	data = tcore_communicator_ref_user_data(comm);
	if (!data) {
		tcore_communicator_free(comm);
		return;
	}
	g_hash_table_destroy(data->objects);
	g_bus_unown_name(data->bus_id);
	g_object_unref(data->manager);
	tcore_free(data);
	sat_manager_deinit(plugin);

	tcore_communicator_free(comm);
}
