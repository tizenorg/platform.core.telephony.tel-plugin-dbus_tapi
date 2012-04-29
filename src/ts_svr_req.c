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
#include <assert.h>
#include <glib.h>
#include <glib-object.h>
#include <unistd.h>
#include <stdlib.h>

#include <tcore.h>
#include <plugin.h>
#include <server.h>
#include <user_request.h>
#include <co_sim.h>

#include <TapiCommon.h>
#include <TelSim.h>

#include "tel_cs_conn.h"
#include "common.h"
#include "ts_utility.h"
#include "ts_common.h"
#include "ts_svr_req.h"
#include "modules.h"

gboolean telephony_service_requests_cb(
		AppFactory* factory,
		int tapi_service,
		int tapi_service_function,
		GArray* in_param1,
		GArray* in_param2,
		GArray* in_param3,
		GArray* in_param4,
		GArray* in_param5,
		GArray** out_param1,
		GArray** out_param2,
		GArray** out_param3,
		GArray** out_param4,
		GError** error);

gboolean telephony_service_requests_sec_cb(AppFactory* factory,
		int tapi_service,
		int tapi_service_function,
		GArray* in_param1,
		GArray* in_param2,
		GArray* in_param3,
		GArray* in_param4,
		GArray* in_param5,
		GArray** out_param1,
		GArray** out_param2,
		GArray** out_param3,
		GArray** out_param4,
		GError** error);

#include "tapi-lib-stub.h"


gboolean telephony_service_requests_cb(AppFactory* factory, int tapi_service, int tapi_service_function,
		GArray* in_param1, GArray* in_param2, GArray* in_param3, GArray* in_param4, GArray* in_param5,
		GArray** out_param1, GArray** out_param2, GArray** out_param3, GArray** out_param4, GError** error)
{
	TapiResult_t ret_val = TAPI_API_SUCCESS;
	TcorePlugin *plugin;
	int requestId = 0;
	int no_data = 0;
	tapi_dbus_connection_name app_name;

	if (in_param4 != NULL)
		app_name = g_array_index(in_param4, tapi_dbus_connection_name, 0);

	dbg("app:[%s], tapi_service_function:[0x%x]", app_name.name, tapi_service_function);
	*out_param1 = g_array_new(FALSE, FALSE, sizeof(gchar));
	*out_param2 = g_array_new(FALSE, FALSE, sizeof(gchar));
	*out_param3 = g_array_new(FALSE, FALSE, sizeof(gchar));
	*out_param4 = g_array_new(FALSE, FALSE, sizeof(gchar));

	plugin = tcore_server_find_plugin(factory->data->server, TCORE_PLUGIN_DEFAULT);

	switch (tapi_service) {
		case TAPI_CS_SERVICE_SIM:
			dbg("TAPI_CS_SERVICE_SIM");
			dbus_request_sim(factory->data, plugin, tapi_service_function, in_param1, in_param2, in_param3, in_param4, out_param1, out_param2,
					out_param3, out_param4, error);
			break;

		case TAPI_CS_SERVICE_GPRS:
			dbg("TAPI_CS_SERVICE_GPRS");
			dbus_request_gprs(factory->data, plugin, tapi_service_function, in_param1, in_param2, in_param3, in_param4, out_param1,
					out_param2, out_param3, out_param4, error);
			g_array_append_vals(*out_param2, &requestId, sizeof(int));
			break;

		case TAPI_CS_SERVICE_NETWORK:
			dbg("TAPI_CS_SERVICE_NETWORK");
			dbus_request_network(factory->data, plugin, tapi_service_function, in_param1, in_param2, in_param3, in_param4, out_param1,
					out_param2, out_param3, out_param4, error);
			break;

		case TAPI_CS_SERVICE_CALL:
			dbg("TAPI_CS_SERVICE_CALL");
			dbus_request_call(factory->data, plugin, tapi_service_function, in_param1, in_param2, in_param3, in_param4, out_param1,
					out_param2, out_param3, out_param4, error);
			break;

		case TAPI_CS_SERVICE_SS:
			dbg("TAPI_CS_SERVICE_SS");
			dbus_request_ss(factory->data, plugin, tapi_service_function, in_param1, in_param2, in_param3, in_param4, out_param1,
					out_param2, out_param3, out_param4, error);
			break;

		case TAPI_CS_SERVICE_SIMATK:
			dbg("TAPI_CS_SERVICE_SIMATK");
			dbus_request_sat(factory->data, plugin, tapi_service_function, in_param1, in_param2, in_param3, in_param4, out_param1,
					out_param2, out_param3, out_param4, error);
			break;

		case TAPI_CS_SERVICE_NETTEXT:
			dbg("TAPI_CS_SERVICE_NETTEXT");
			dbus_request_sms(factory->data, plugin, tapi_service_function, in_param1, in_param2, in_param3, in_param4, out_param1,
					out_param2, out_param3, out_param4, error);
			break;

		case TAPI_CS_SERVICE_SOUND:
			dbg("TAPI_CS_SERVICE_SOUND");
			dbus_request_sound(factory->data, plugin, tapi_service_function, in_param1, in_param2, in_param3, in_param4, out_param1,
					out_param2, out_param3, out_param4, error);
			break;

		case TAPI_CS_SERVICE_UTIL:
			dbg("TAPI_CS_SERVICE_UTIL");
			dbus_request_util(factory->data, plugin, tapi_service_function, in_param1, in_param2, in_param3, in_param4, out_param1,
					out_param2, out_param3, out_param4, error);
			break;

		default:
			ret_val = TAPI_API_NOT_SUPPORTED;
			g_array_append_vals(*out_param1, &ret_val, sizeof(TapiResult_t));
			g_array_append_vals(*out_param2, &requestId, sizeof(int));
			g_array_append_vals(*out_param3, &no_data, sizeof(int));
			dbg("telephony_service_requests_cb:DEFAULT");
			break;
	}

	return TRUE;
}

gboolean telephony_service_requests_sec_cb(AppFactory* factory, int tapi_service, int tapi_service_function,
		GArray* in_param1, GArray* in_param2, GArray* in_param3, GArray* in_param4, GArray* in_param5,
		GArray** out_param1, GArray** out_param2, GArray** out_param3, GArray** out_param4, GError** error)
{
	TapiResult_t ret_val = TAPI_API_SUCCESS;
	TcorePlugin *plugin;
	int requestId = 0;
	int no_data = 0;

	dbg("tapi_service:[%d] tapi_service_function:[0x%x]", tapi_service, tapi_service_function);
	*out_param1 = g_array_new(FALSE, FALSE, sizeof(gchar));
	*out_param2 = g_array_new(FALSE, FALSE, sizeof(gchar));
	*out_param3 = g_array_new(FALSE, FALSE, sizeof(gchar));
	*out_param4 = g_array_new(FALSE, FALSE, sizeof(gchar));

	plugin = tcore_server_find_plugin(factory->data->server, TCORE_PLUGIN_DEFAULT);

	switch (tapi_service) {
		case TAPI_CS_SERVICE_NETWORK:
			dbg("TAPI_CS_SERVICE_NETWORK(internal)");
			dbus_request_network(factory->data, plugin, tapi_service_function, in_param1, in_param2, in_param3, in_param4, out_param1,
					out_param2, out_param3, out_param4, error);
			break;

		case TAPI_CS_SERVICE_SOUND:
			dbg("TAPI_CS_SERVICE_SOUND(internal)");
			dbus_request_sound(factory->data, plugin, tapi_service_function, in_param1, in_param2, in_param3, in_param4, out_param1,
					out_param2, out_param3, out_param4, error);
			break;

		case TAPI_CS_SERVICE_NETTEXT:
			dbg("TAPI_CS_SERVICE_NETTEXT(internal)");
			dbus_request_sms(factory->data, plugin, tapi_service_function, in_param1, in_param2, in_param3, in_param4, out_param1,
					out_param2, out_param3, out_param4, error);
			break;

		case TAPI_CS_SERVICE_SVCMODE:
		case TAPI_CS_SERVICE_IMEI:
			dbg("TAPI_CS_SERVICE_SVCMODE(internal)");
			dbus_request_productivity(factory->data, plugin, tapi_service_function, in_param1, in_param2, in_param3, in_param4, out_param1,
					out_param2, out_param3, out_param4, error);
			break;

		case TAPI_CS_SERVICE_MISC:
			dbg("TAPI_CS_SERVICE_MISC(internal)");
			dbus_request_misc(factory->data, plugin, tapi_service_function, in_param1, in_param2, in_param3, in_param4, out_param1,
					out_param2, out_param3, out_param4, error);
			break;

		case TAPI_CS_SERVICE_DATA:
			dbg("TAPI_CS_SERVICE_DATA(internal)");
			dbus_request_cdmadata(factory->data, plugin, tapi_service_function, in_param1, in_param2, in_param3, in_param4, out_param1,
					out_param2, out_param3, out_param4, error);
			break;

		case TAPI_CS_SERVICE_GPS:
			dbg("TAPI_CS_SERVICE_GPS(internal)");
			dbus_request_gps(factory->data, plugin, tapi_service_function, in_param1, in_param2, in_param3, in_param4, out_param1,
					out_param2, out_param3, out_param4, error);
			break;

		case TAPI_CS_SERVICE_POWER:
			dbg("TAPI_CS_SERVICE_POWER(internal)");
			dbus_request_power(factory->data, plugin, tapi_service_function, in_param1, in_param2, in_param3, in_param4, out_param1,
					out_param2, out_param3, out_param4, error);
			break;

		case TAPI_CS_SERVICE_CFG:
			dbg("TAPI_CS_SERVICE_CFG(internal)");
			dbus_request_cfg(factory->data, plugin, tapi_service_function, in_param1, in_param2, in_param3, in_param4, out_param1,
					out_param2, out_param3, out_param4, error);
			break;

		case TAPI_CS_SERVICE_OMADM:
			dbg("TAPI_CS_SERVICE_OMADM(internal)");
			dbus_request_omadm(factory->data, plugin, tapi_service_function, in_param1, in_param2, in_param3, in_param4, out_param1,
					out_param2, out_param3, out_param4, error);
			break;

		default:
			ret_val = TAPI_API_NOT_SUPPORTED;
			g_array_append_vals(*out_param1, &ret_val, sizeof(TapiResult_t));
			g_array_append_vals(*out_param2, &requestId, sizeof(int));
			g_array_append_vals(*out_param3, &no_data, sizeof(int));
			dbg("telephony_service_requests_sec_cb:DEFAULT");
			break;
	}

	return TRUE;
}

GType app_factory_get_type (void);

/*This is part of platform provided code skeleton for client server model*/
G_DEFINE_TYPE(AppFactory, app_factory, G_TYPE_OBJECT);

/*This is part of platform provided code skeleton for client server model*/
static void app_factory_class_init (AppFactoryClass* factory_class)
{
	dbus_g_object_type_install_info (G_TYPE_FROM_CLASS (factory_class), &dbus_glib__object_info);
}

/*This is part of platform provided code skeleton for client server model*/
static void app_factory_init (AppFactory* factory)
{
}

/**********************************************************************************************
 *
 *	This function registers the dbus service name.This service name must be used by tapi lib to
 *	to communicate with the server
 *
 *	DBUS SERVICE NAME USED :- "org.projectx.telephony"
 *
 ***********************************************************************************************/

static gboolean register_factory(struct custom_data *data)
{
	DBusGConnection* connection;
	DBusGProxy* proxy;
	GError* error = NULL;
	AppFactory* factory;
	guint32 request_name_ret;

	dbg("IN REGISTER FACTORY\n");

	connection = dbus_g_bus_get(DBUS_BUS_SYSTEM, &error);
	if (connection == NULL) {
		if (error != NULL) {
			dbg("failed to get CS connection with error cause");
			g_error_free(error);
		}
		else {
			dbg("failed to get CS connection with NULL error cause");
		}
		return FALSE;
	}

	proxy = dbus_g_proxy_new_for_name(connection, DBUS_SERVICE_DBUS, DBUS_PATH_DBUS, DBUS_INTERFACE_DBUS);
	if (!proxy) {
		dbg("proxy is NULL");
		return FALSE;
	}

	if (!org_freedesktop_DBus_request_name(proxy, "org.projectx.telephony", 0, &request_name_ret, &error)) {
		if (error != NULL) {
			dbg("failed to DBUS request name with error cause");
			g_error_free(error);
		}
		else {
			dbg("failed to DBUS request name with NULL error cause");
		}

		return FALSE;
	}

	factory = g_object_new(APP_FACTORY_TYPE, NULL);
	if (!factory) {
		dbg("factory is NULL");
		return FALSE;
	}

	factory->data = data;

	dbus_g_connection_register_g_object(connection, "/org/projectx/app", G_OBJECT(factory));
	dbg("IN REGISTER FACTORY : SUCCESS");

	return TRUE;
}

gboolean ts_register_dbus_factory(struct custom_data *data)
{
	register_factory(data);

	return TRUE;
}
