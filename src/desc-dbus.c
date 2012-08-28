/*
 * tel-plugin-dbus-tapi
 *
 * Copyright (c) 2012 Samsung Electronics Co., Ltd. All rights reserved.
 *
 * Contact: Ja-young Gu <jygu@samsung.com>
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

#include <stdio.h>
#include <string.h>
#include <pthread.h>
#include <unistd.h>
#include <stdlib.h>
#include <time.h>
#include <glib.h>
#include <glib-object.h>
#include <gio/gio.h>

#include <tcore.h>
#include <server.h>
#include <plugin.h>
#include <hal.h>
#include <communicator.h>
#include <storage.h>
#include <queue.h>
#include <user_request.h>
#include <co_network.h>
#include <co_sim.h>
#include <co_ps.h>

#include "generated-code.h"
#include "common.h"

static void add_modem(struct custom_data *ctx, TcorePlugin *p)
{
	TelephonyObjectSkeleton *object;
	char *plugin_name = NULL;
	char *path = NULL;
	GSList *co_list = NULL;

	plugin_name = tcore_plugin_ref_plugin_name(p);
	if (!plugin_name)
		return;

	path = g_strdup_printf("%s/%s", MY_DBUS_PATH, plugin_name);
	dbg("path = [%s]", path);

	object = g_hash_table_lookup(ctx->objects, path);
	if (object) {
		dbg("dbus interface object already created. (object = %p)", object);
		goto OUT;
	}

	object = telephony_object_skeleton_new(path);
	dbg("new dbus object created. (object = %p)", object);
	g_hash_table_insert(ctx->objects, g_strdup(path), object);

	/* Add interfaces */
	dbus_plugin_setup_modem_interface(object, ctx);

	co_list = tcore_plugin_get_core_objects_bytype(p, CORE_OBJECT_TYPE_CALL);
	if (co_list) {
		g_slist_free(co_list);
		dbus_plugin_setup_call_interface(object, ctx);
	}


	co_list = tcore_plugin_get_core_objects_bytype(p, CORE_OBJECT_TYPE_NETWORK);
	if (co_list) {
		g_slist_free(co_list);
		dbus_plugin_setup_network_interface(object, ctx);
	}

	co_list = tcore_plugin_get_core_objects_bytype(p, CORE_OBJECT_TYPE_SS);
	if (co_list) {
		g_slist_free(co_list);
		dbus_plugin_setup_ss_interface(object, ctx);
	}

	co_list = tcore_plugin_get_core_objects_bytype(p, CORE_OBJECT_TYPE_SMS);
	if (co_list) {
		g_slist_free(co_list);
		dbus_plugin_setup_sms_interface(object, ctx);
	}

	co_list = tcore_plugin_get_core_objects_bytype(p, CORE_OBJECT_TYPE_SAT);
	if (co_list) {
		g_slist_free(co_list);
		dbus_plugin_setup_sat_interface(object, ctx);
	}

	co_list = tcore_plugin_get_core_objects_bytype(p, CORE_OBJECT_TYPE_PHONEBOOK);
	if (co_list) {
		g_slist_free(co_list);
		dbus_plugin_setup_phonebook_interface(object, ctx);
	}

	co_list = tcore_plugin_get_core_objects_bytype(p, CORE_OBJECT_TYPE_SAP);
	if (co_list) {
		g_slist_free(co_list);
		dbus_plugin_setup_sap_interface(object, ctx);
	}

	co_list = tcore_plugin_get_core_objects_bytype(p, CORE_OBJECT_TYPE_SIM);
	if (co_list) {
		g_slist_free(co_list);
		dbus_plugin_setup_sim_interface(object, ctx);
	}

	g_dbus_object_manager_server_export (ctx->manager, G_DBUS_OBJECT_SKELETON (object));

OUT:
	if (path)
		g_free(path);
}

static void refresh_object(struct custom_data *ctx)
{
	GSList *plugins;
	GSList *cur;
	GSList *co_list;
	TcorePlugin *p;

	if (!ctx->manager) {
		dbg("not ready..");
		return;
	}

	plugins = tcore_server_ref_plugins(ctx->server);
	if (!plugins)
		return;

	cur = plugins;
	for (cur = plugins; cur; cur = cur->next) {
		p = cur->data;
		if (!p)
			continue;

		co_list = tcore_plugin_get_core_objects_bytype(p, CORE_OBJECT_TYPE_MODEM);
		if (!co_list)
			continue;

		if (!tcore_object_get_hal(co_list->data)) {
			g_slist_free(co_list);
			continue;
		}

		g_slist_free(co_list);

		add_modem(ctx, p);
	}
}

static TReturn send_response(Communicator *comm, UserRequest *ur, enum tcore_response_command command, unsigned int data_len, const void *data)
{
	struct custom_data *ctx = NULL;
	const struct tcore_user_info *ui;

	dbg("Response Command = [0x%x], data_len = %d", command, data_len);

	ctx = tcore_communicator_ref_user_data(comm);
	if (!ctx) {
		dbg("user_data is NULL");
		return FALSE;
	}

	ui = tcore_user_request_ref_user_info(ur);

	switch (command & (TCORE_RESPONSE | 0x0FF00000)) {
		case TRESP_CALL:
			dbus_plugin_call_response(ctx, ur, ui->user_data, command, data_len, data);
			break;

		case TRESP_SS:
			dbus_plugin_ss_response(ctx, ur, ui->user_data, command, data_len, data);
			break;

		case TRESP_PS:
			break;

		case TRESP_SIM:
			dbus_plugin_sim_response(ctx, ur, ui->user_data, command, data_len, data);
			break;

		case TRESP_SAP:
			dbus_plugin_sap_response(ctx, ur, ui->user_data, command, data_len, data);
			break;

		case TRESP_PHONEBOOK:
			dbus_plugin_phonebook_response(ctx, ur, ui->user_data, command, data_len, data);
			break;

		case TRESP_MODEM:
			dbus_plugin_modem_response(ctx, ur, ui->user_data, command, data_len, data);
			break;

		case TRESP_SMS:
			dbus_plugin_sms_response(ctx, ur, ui->user_data, command, data_len, data);
			break;

		case TRESP_SAT:
			dbus_plugin_sat_response(ctx, ur, ui->user_data, command, data_len, data);
			break;
		case TRESP_CUSTOM:
			break;

		case TRESP_NETWORK:
			dbus_plugin_network_response(ctx, ur, ui->user_data, command, data_len, data);
			break;

		default:
			warn("unknown command (0x%x)", command);
			break;
	}

	return FALSE;
}

static TReturn send_notification(Communicator *comm, CoreObject *source, enum tcore_notification_command command, unsigned int data_len, const void *data)
{
	struct custom_data *ctx = NULL;
	char *plugin_name;
	char *path;
	TelephonyObjectSkeleton *object;

	dbg("notification !!! (command = 0x%x, data_len = %d)", command, data_len);

	ctx = tcore_communicator_ref_user_data(comm);
	if (!ctx) {
		dbg("user_data is NULL");
		return FALSE;
	}

	plugin_name = tcore_plugin_ref_plugin_name(tcore_object_ref_plugin(source));
	if (plugin_name) {
		path = g_strdup_printf("%s/%s", MY_DBUS_PATH, plugin_name);
	}
	else {
		path = g_strdup_printf("%s", MY_DBUS_PATH);
	}
	dbg("path = [%s]", path);

	object = g_hash_table_lookup(ctx->objects, path);
	dbg("dbus inteface object = %p", object);

	switch (command & (TCORE_NOTIFICATION | 0x0FF00000)) {
		case TNOTI_CALL:
			dbus_plugin_call_notification(ctx, plugin_name, object, command, data_len, data);
			break;

		case TNOTI_SS:
			dbus_plugin_ss_notification(ctx, plugin_name, object, command, data_len, data);
			break;

		case TNOTI_PS:
			break;

		case TNOTI_SIM:
			dbus_plugin_sim_notification(ctx, plugin_name, object, command, data_len, data);
			break;

		case TNOTI_SAP:
			dbus_plugin_sap_notification(ctx, plugin_name, object, command, data_len, data);
			break;

		case TNOTI_PHONEBOOK:
			dbus_plugin_phonebook_notification(ctx, plugin_name, object, command, data_len, data);
			break;

		case TNOTI_MODEM:
			dbus_plugin_modem_notification(ctx, plugin_name, object, command, data_len, data);
			break;

		case TNOTI_SMS:
			dbus_plugin_sms_notification(ctx, plugin_name, object, command, data_len, data);
			break;

		case TNOTI_SAT:
			dbus_plugin_sat_notification(ctx, plugin_name, object, command, data_len, data);
			break;
		case TNOTI_CUSTOM:
			break;

		case TNOTI_NETWORK:
			dbus_plugin_network_notification(ctx, plugin_name, object, command, data_len, data);
			break;

		case TNOTI_SERVER:
			if (command == TNOTI_SERVER_RUN) {
				refresh_object(ctx);
			}
			break;

		default:
			warn("unknown command (0x%x)", command);
			break;
	}

	return FALSE;
}


static gboolean
on_manager_getmodems (TelephonyManager *mgr,
		GDBusMethodInvocation  *invocation,
		gpointer                user_data)
{
	struct custom_data *ctx = user_data;
	GSList *plugins;
	GSList *cur;
	GSList *co_list;
	int max_count = 0;
	int count;
	TcorePlugin *p;
	gchar **list;

	plugins = tcore_server_ref_plugins(ctx->server);
	if (!plugins) {
		telephony_manager_complete_get_modems(mgr, invocation, NULL);
		return TRUE;
	}

	max_count = g_slist_length(plugins);
	list = calloc(sizeof(gchar *) * max_count, 1);
	count = 0;

	cur = plugins;
	for (cur = plugins; cur; cur = cur->next) {
		p = cur->data;
		if (!p)
			continue;

		co_list = tcore_plugin_get_core_objects_bytype(p, CORE_OBJECT_TYPE_MODEM);
		if (!co_list)
			continue;

		if (!tcore_object_get_hal(co_list->data)) {
			g_slist_free(co_list);
			continue;
		}
		g_slist_free(co_list);

		list[count] = g_strdup(tcore_plugin_get_description(p)->name);
		count++;
	}

	telephony_manager_complete_get_modems(mgr, invocation, (const gchar **)list);

	for (;count >= 0; count--) {
		if (list[count]) {
			g_free(list[count]);
		}
	}
	free(list);

	return TRUE;
}

static void on_bus_acquired(GDBusConnection *conn, const gchar *name, gpointer user_data)
{
	gboolean rv = FALSE;
	gpointer handle = NULL;
	static Storage *strg;
	struct custom_data *ctx = user_data;
	TelephonyManager *mgr;

	ctx->manager = g_dbus_object_manager_server_new (MY_DBUS_PATH);

	refresh_object(ctx);

	/* Add interface to default object path */
	mgr = telephony_manager_skeleton_new();
	g_signal_connect (mgr,
			"handle-get-modems",
			G_CALLBACK (on_manager_getmodems),
			ctx); /* user_data */

	g_dbus_interface_skeleton_export(G_DBUS_INTERFACE_SKELETON(mgr), conn, MY_DBUS_PATH, NULL);

	g_dbus_object_manager_server_set_connection (ctx->manager, conn);

	//set telephony ready registry
	strg = tcore_server_find_storage(ctx->server, "vconf");
	handle = tcore_storage_create_handle(strg, "vconf");

	rv = tcore_storage_set_bool(strg, STORAGE_KEY_TELEPHONY_READY, TRUE);
	if(!rv){
		dbg("fail to set the telephony status to registry");
	}

	dbg("done to acquire the dbus");
}

struct tcore_communitor_operations ops = {
	.send_response = send_response,
	.send_notification = send_notification,
};

static gboolean on_load()
{
	dbg("i'm load!");

	return TRUE;
}

static gboolean on_init(TcorePlugin *p)
{
	Communicator *comm;
	struct custom_data *data;
	guint id;

	if (!p)
		return FALSE;

	dbg("i'm init!");

	data = calloc(sizeof(struct custom_data), 1);
	if (!data) {
		return FALSE;
	}

	data->plugin = p;

	comm = tcore_communicator_new(p, "dbus", &ops);
	tcore_communicator_link_user_data(comm, data);

	data->comm = comm;
	data->server = tcore_plugin_ref_server(p);

	data->objects = g_hash_table_new(g_str_hash, g_str_equal);
	data->cached_sat_main_menu = NULL;

	dbg("data = %p", data);

	id = g_bus_own_name (G_BUS_TYPE_SYSTEM,
			MY_DBUS_SERVICE,
			G_BUS_NAME_OWNER_FLAGS_REPLACE,
			on_bus_acquired,
			NULL, NULL,
			data,
			NULL);

	return TRUE;
}

static void on_unload(TcorePlugin *p)
{
	struct custom_data *data;
	Communicator *comm;

	if (!p)
		return;

	dbg("i'm unload");

	comm = tcore_server_find_communicator(tcore_plugin_ref_server(p), "dbus");
	if (!comm)
		return;

	data = tcore_communicator_ref_user_data(comm);
	if (!data)
		return;

	g_hash_table_destroy(data->objects);

	free(data);
}

struct tcore_plugin_define_desc plugin_define_desc =
{
	.name = "NEW_DBUS_COMMUNICATOR",
	.priority = TCORE_PLUGIN_PRIORITY_HIGH,
	.version = 1,
	.load = on_load,
	.init = on_init,
	.unload = on_unload
};
