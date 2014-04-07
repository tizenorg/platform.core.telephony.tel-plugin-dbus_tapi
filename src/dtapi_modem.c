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

#include "dtapi_modem.h"
#include "dtapi_util.h"

#include <plugin.h>
#include <tel_modem.h>

#define AC_MODEM	"telephony_framework::api_modem"

static void on_response_dtapi_modem_set_power_status(gint result,
	const void *response, void *user_data)
{
	DbusRespCbData *rsp_cb_data = user_data;

	tcore_check_return_assert(NULL != rsp_cb_data);

	dbg("result: [%d]", result);

	telephony_modem_complete_set_power_status(rsp_cb_data->interface_object,
		rsp_cb_data->invocation, result);

	tcore_free(rsp_cb_data);
}

static void on_response_dtapi_modem_set_flight_mode(gint result,
	const void *response, void *user_data)
{
	DbusRespCbData *rsp_cb_data = user_data;

	tcore_check_return_assert(NULL != rsp_cb_data);

	dbg("result: [%d]", result);

	telephony_modem_complete_set_flight_mode(rsp_cb_data->interface_object,
		rsp_cb_data->invocation, result);

	tcore_free(rsp_cb_data);
}

static void on_response_dtapi_modem_get_flight_mode(gint result,
	const void *response, void *user_data)
{
	DbusRespCbData *rsp_cb_data = user_data;
	TelModemFlightModeStatus flight_mode_status = TEL_MODEM_FLIGHT_MODE_OFF;
	gboolean enable;

	tcore_check_return_assert(NULL != rsp_cb_data && NULL != response);

	enable = *(gboolean *)response;
	dbg("result: [%d] Flight mode: [%s]", result, (enable ? "ON" : "OFF"));

	telephony_modem_complete_get_flight_mode(rsp_cb_data->interface_object,
		rsp_cb_data->invocation, result, enable);

	if (enable == TRUE)
		flight_mode_status = TEL_MODEM_FLIGHT_MODE_ON;

	/* Update property */
	telephony_modem_set_flight_mode_status(rsp_cb_data->interface_object, flight_mode_status);

	tcore_free(rsp_cb_data);
}

static void on_response_dtapi_modem_get_version(gint result,
	const void *response, void *user_data)
{
	DbusRespCbData *rsp_cb_data = user_data;
	const TelModemVersion *version = response;

	tcore_check_return_assert(NULL != rsp_cb_data && NULL != version);

	dbg("result: [%d] SW: [%s] HW: [%s] CAL: [%s] PC: [%s]", result,
		version->software_version, version->hardware_version,
		version->calibration_date, version->product_code);

	/* Send response */
	telephony_modem_complete_get_version(rsp_cb_data->interface_object,
		rsp_cb_data->invocation, result,
		(const gchar *)version->software_version,
		(const gchar *)version->hardware_version,
		(const gchar *)version->calibration_date,
		(const gchar *)version->product_code);

	/* Update property */
	{
		GVariantBuilder var_builder;
		GVariant *var_version = NULL;

		g_variant_builder_init(&var_builder, G_VARIANT_TYPE("a{sv}"));

		g_variant_builder_add(&var_builder, "{sv}",
			"software_version", g_variant_new_string(version->software_version));
		g_variant_builder_add(&var_builder, "{sv}",
			"hardware_version", g_variant_new_string(version->hardware_version));
		g_variant_builder_add(&var_builder, "{sv}",
			"calibration_date", g_variant_new_string(version->hardware_version));
		g_variant_builder_add(&var_builder, "{sv}",
			"product_code", g_variant_new_string(version->product_code));

		var_version = g_variant_builder_end(&var_builder);
		tcore_check_return_assert(NULL != var_version);

		/* Set property */
		telephony_modem_set_version(rsp_cb_data->interface_object, var_version);
	}

	tcore_free(rsp_cb_data);
}

static void on_response_dtapi_modem_get_imei(gint result,
	const void *response, void *user_data)
{
	DbusRespCbData *rsp_cb_data = user_data;
	const gchar *imei = response;

	tcore_check_return_assert(NULL != rsp_cb_data);

	dbg("result: [%d] IMEI: [%s]", result, (imei ? imei : "\0"));

	telephony_modem_complete_get_imei(rsp_cb_data->interface_object,
		rsp_cb_data->invocation, result, imei);

	/* Update property */
	telephony_modem_set_imei(rsp_cb_data->interface_object, imei);

	tcore_free(rsp_cb_data);
}

static gboolean dtapi_modem_set_power_status(TelephonyModem *modem,
	GDBusMethodInvocation *invocation,
	TelModemPowerStatus status, gpointer user_data)
{
	DbusRespCbData *rsp_cb_data;
	TcorePlugin *plugin = (TcorePlugin *)user_data;
	TelReturn ret;

	dbg("Entry");

	if (dtapi_check_access_control(invocation, AC_MODEM, "w") == FALSE)
		return TRUE;

	rsp_cb_data = dtapi_create_resp_cb_data(modem, invocation, NULL, 0);

	ret = tcore_plugin_dispatch_request(plugin, TRUE,
		TCORE_COMMAND_MODEM_SET_POWER_STATUS,
		&status, sizeof(TelModemPowerStatus),
		on_response_dtapi_modem_set_power_status, rsp_cb_data);
	if (ret != TEL_RETURN_SUCCESS) {
		dtapi_return_error(invocation, G_DBUS_ERROR_FAILED, "Dispatch failed");
		tcore_free(rsp_cb_data);
	}

	return TRUE;
}

static gboolean dtapi_modem_set_flight_mode(TelephonyModem *modem,
	GDBusMethodInvocation *invocation,
	gboolean enable, gpointer user_data)
{
	DbusRespCbData *rsp_cb_data;
	TcorePlugin *plugin = (TcorePlugin *)user_data;
	TelReturn ret;

	dbg("Entry");

	if (dtapi_check_access_control(invocation, AC_MODEM, "w") == FALSE)
		return TRUE;

	rsp_cb_data = dtapi_create_resp_cb_data(modem, invocation, NULL, 0);

	ret = tcore_plugin_dispatch_request(plugin, TRUE,
		TCORE_COMMAND_MODEM_SET_FLIGHTMODE,
		&enable, sizeof(gboolean),
		on_response_dtapi_modem_set_flight_mode, rsp_cb_data);
	if (ret != TEL_RETURN_SUCCESS) {
		dtapi_return_error(invocation, G_DBUS_ERROR_FAILED, "Dispatch failed");
		tcore_free(rsp_cb_data);
	}

	return TRUE;
}

static gboolean dtapi_modem_get_flight_mode(TelephonyModem *modem,
	GDBusMethodInvocation *invocation, gpointer user_data)
{
	DbusRespCbData *rsp_cb_data;
	TcorePlugin *plugin = (TcorePlugin *)user_data;
	TelReturn ret;

	dbg("Entry");

	if (dtapi_check_access_control(invocation, AC_MODEM, "r") == FALSE)
		return TRUE;

	rsp_cb_data = dtapi_create_resp_cb_data(modem, invocation, NULL, 0);

	ret = tcore_plugin_dispatch_request(plugin, TRUE,
		TCORE_COMMAND_MODEM_GET_FLIGHTMODE,
		NULL, 0,
		on_response_dtapi_modem_get_flight_mode, rsp_cb_data);
	if (ret != TEL_RETURN_SUCCESS) {
		dtapi_return_error(invocation, G_DBUS_ERROR_FAILED, "Dispatch failed");
		tcore_free(rsp_cb_data);
	}

	return TRUE;
}

static gboolean dtapi_modem_get_version(TelephonyModem *modem,
	GDBusMethodInvocation *invocation, gpointer user_data)
{
	DbusRespCbData *rsp_cb_data;
	TcorePlugin *plugin = (TcorePlugin *)user_data;
	TelReturn ret;

	dbg("Entry");

	if (dtapi_check_access_control(invocation, AC_MODEM, "r") == FALSE)
		return TRUE;

	rsp_cb_data = dtapi_create_resp_cb_data(modem, invocation, NULL, 0);

	ret = tcore_plugin_dispatch_request(plugin, TRUE,
		TCORE_COMMAND_MODEM_GET_VERSION,
		NULL, 0,
		on_response_dtapi_modem_get_version, rsp_cb_data);
	if (ret != TEL_RETURN_SUCCESS) {
		dtapi_return_error(invocation, G_DBUS_ERROR_FAILED, "Dispatch failed");
		tcore_free(rsp_cb_data);
	}

	return TRUE;
}

static gboolean dtapi_modem_get_imei(TelephonyModem *modem,
	GDBusMethodInvocation *invocation, gpointer user_data)
{
	DbusRespCbData *rsp_cb_data;
	TcorePlugin *plugin = (TcorePlugin *)user_data;
	TelReturn ret;

	dbg("Entry");

	if (dtapi_check_access_control(invocation, AC_MODEM, "r") == FALSE)
		return TRUE;

	rsp_cb_data = dtapi_create_resp_cb_data(modem, invocation, NULL, 0);

	ret = tcore_plugin_dispatch_request(plugin, TRUE,
		TCORE_COMMAND_MODEM_GET_IMEI,
		NULL, 0,
		on_response_dtapi_modem_get_imei, rsp_cb_data);
	if (ret != TEL_RETURN_SUCCESS) {
		dtapi_return_error(invocation, G_DBUS_ERROR_FAILED, "Dispatch failed");
		tcore_free(rsp_cb_data);
	}

	return TRUE;
}

gboolean dtapi_setup_modem_interface(TelephonyObjectSkeleton *object,
	TcorePlugin *plugin)
{
	TelephonyModem *modem = telephony_modem_skeleton_new();
	tcore_check_return_value_assert(NULL != modem, FALSE);

	telephony_object_skeleton_set_modem(object, modem);
	g_object_unref(modem);

	dbg("modem = %p", modem);

	g_signal_connect(modem,
		"handle-set-power-status",
		G_CALLBACK(dtapi_modem_set_power_status), plugin);

	g_signal_connect(modem,
		"handle-set-flight-mode",
		G_CALLBACK(dtapi_modem_set_flight_mode), plugin);

	g_signal_connect(modem,
		"handle-get-flight-mode",
		G_CALLBACK(dtapi_modem_get_flight_mode), plugin);

	g_signal_connect(modem,
		"handle-get-version",
		G_CALLBACK(dtapi_modem_get_version), plugin);

	g_signal_connect(modem,
		"handle-get-imei",
		G_CALLBACK(dtapi_modem_get_imei), plugin);

	/* Initialize D-Bus properties */
	telephony_modem_set_status(modem, TEL_MODEM_POWER_OFF);
	telephony_modem_set_flight_mode_status(modem, TEL_MODEM_FLIGHT_MODE_UNKNOWN);
	telephony_modem_set_imei(modem, NULL);
	telephony_modem_set_version(modem, NULL);

	return TRUE;
}

gboolean dtapi_handle_modem_notification(TelephonyObjectSkeleton *object,
	TcorePlugin *plugin, TcoreNotification command,
	guint data_len, const void *data)
{
	TelephonyModem *modem;

	tcore_check_return_value_assert(NULL != object, FALSE);
	modem = telephony_object_peek_modem(TELEPHONY_OBJECT(object));

	switch (command) {
	case TCORE_NOTIFICATION_MODEM_POWER: {
		guint power_status;

		tcore_check_return_value_assert(NULL != data, FALSE);

		power_status = *(guint *)data;
		dbg("modem: [%p] Power status: [%d]", modem, power_status);

		/* Update property */
		telephony_modem_set_status(modem, power_status);
	}
	break;

	case TCORE_NOTIFICATION_MODEM_FLIGHT_MODE: {
		TelModemFlightModeStatus flight_mode_status = TEL_MODEM_FLIGHT_MODE_OFF;
		gboolean status;

		tcore_check_return_value_assert(NULL != data, FALSE);

		status = *(gboolean *)data;
		dbg("modem: [%p] Flight Mode status: %d",
			modem, (status ? "ON" : "OFF"));

		if (status == TRUE)
			flight_mode_status = TEL_MODEM_FLIGHT_MODE_ON;

		/* Update property */
		telephony_modem_set_flight_mode_status(modem, flight_mode_status);
	}
	break;

	default:
		err("Unhandled command: [0x%x]", command);
	break;
	}

	return TRUE;
}

