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

#include "dtapi_gps.h"
#include "dtapi_util.h"

#include <plugin.h>
#include <tel_gps.h>

#define AC_GPS		"telephony_framework::api_gps"

static void
on_response_dtapi_gps_confirm_measure_pos(gint result, const void *response, void *user_data)
{
	DbusRespCbData *rsp_cb_data = user_data;
	tcore_check_return_assert(NULL != rsp_cb_data);

	dbg("result: [%d]", result);

	telephony_gps_complete_confirm_measure_pos(rsp_cb_data->interface_object,
		rsp_cb_data->invocation, result);
	tcore_free(rsp_cb_data);
}

static void
on_response_dtapi_gps_set_frequency_aiding(gint result, const void *response, void *user_data)
{
	DbusRespCbData *rsp_cb_data = user_data;
	tcore_check_return_assert(NULL != rsp_cb_data);

	dbg("result: [%d]", result);

	telephony_gps_complete_set_frequency_aiding(rsp_cb_data->interface_object,
		rsp_cb_data->invocation, result);
	tcore_free(rsp_cb_data);
}

static gboolean
dtapi_gps_confirm_measure_pos(TelephonyGps *gps, GDBusMethodInvocation *invocation,
	gchar *data, gpointer user_data)
{
	TelGpsDataInfo gps_data = {0, NULL};
	TcorePlugin *plugin = (TcorePlugin *)user_data;
	DbusRespCbData *rsp_cb_data;
	TelReturn result;

	if (dtapi_check_access_control(invocation, AC_GPS, "w") == FALSE)
		return TRUE;

	gps_data.data = g_base64_decode(data, &gps_data.data_len);

	rsp_cb_data = dtapi_create_resp_cb_data(gps, invocation, NULL, 0);

	result = tcore_plugin_dispatch_request(plugin, TRUE, TCORE_COMMAND_GPS_CONFIRM_MEASURE_POS,
				&gps_data, sizeof(TelGpsDataInfo), on_response_dtapi_gps_confirm_measure_pos, rsp_cb_data);

	if (result != TEL_RETURN_SUCCESS) {
		dtapi_return_error(invocation, G_DBUS_ERROR_FAILED, "Dispatch failed");
		tcore_free(rsp_cb_data);
	}
	tcore_free(gps_data.data);

	return TRUE;
}

static gboolean
dtapi_gps_set_frequency_aiding(TelephonyGps *gps, GDBusMethodInvocation *invocation,
	gboolean state, gpointer user_data)
{
	DbusRespCbData *rsp_cb_data;
	TcorePlugin *plugin = (TcorePlugin *)user_data;
	TelReturn result;

	if (dtapi_check_access_control(invocation, AC_GPS, "w") == FALSE)
		return TRUE;

	rsp_cb_data = dtapi_create_resp_cb_data(gps, invocation, NULL, 0);

	result = tcore_plugin_dispatch_request(plugin, TRUE, TCORE_COMMAND_GPS_SET_FREQUENCY_AIDING,
				&state, sizeof(gboolean), on_response_dtapi_gps_set_frequency_aiding, rsp_cb_data);

	if (result != TEL_RETURN_SUCCESS) {
		dtapi_return_error(invocation, G_DBUS_ERROR_FAILED, "Dispatch failed");
		tcore_free(rsp_cb_data);
	}

	return TRUE;
}

gboolean dtapi_setup_gps_interface(TelephonyObjectSkeleton *object, TcorePlugin *plugin)
{
	TelephonyGps *gps = telephony_gps_skeleton_new();
	tcore_check_return_value_assert(NULL != gps, FALSE);

	telephony_object_skeleton_set_gps(object, gps);
	g_object_unref(gps);

	dbg("gps = %p", gps);

	g_signal_connect(gps, "handle-confirm-measure-pos",
		G_CALLBACK(dtapi_gps_confirm_measure_pos), plugin);

	g_signal_connect(gps, "handle-set-frequency-aiding",
		G_CALLBACK(dtapi_gps_set_frequency_aiding), plugin);

	return TRUE;
}

gboolean dtapi_handle_gps_notification(TelephonyObjectSkeleton *object, TcorePlugin *plugin,
	TcoreNotification command, guint data_len, const void *data)
{
	TelephonyGps *gps;

	tcore_check_return_value_assert(NULL != object, FALSE);
	gps = telephony_object_peek_gps(TELEPHONY_OBJECT(object));

	switch (command) {
	case TCORE_NOTIFICATION_GPS_ASSIST_DATA: {
		gchar *encoded_data = NULL;
		dbg("[NOTI] TCORE_NOTIFICATION_GPS_ASSIST_DATA");
		tcore_check_return_value_assert(NULL != data, FALSE);
		encoded_data = g_base64_encode((const guchar *)data, data_len);
		telephony_gps_emit_assist_data(gps, encoded_data);
		tcore_free(encoded_data);
	} break;

	case TCORE_NOTIFICATION_GPS_MEASURE_POSITION: {
		gchar *encoded_data = NULL;
		dbg("[NOTI] TCORE_NOTIFICATION_GPS_MEASURE_POSITION");
		tcore_check_return_value_assert(NULL != data, FALSE);
		encoded_data = g_base64_encode((const guchar *)data, data_len);
		telephony_gps_emit_measure_position(gps, encoded_data);
		tcore_free(encoded_data);
	} break;

	case TCORE_NOTIFICATION_GPS_RESET_ASSIST_DATA: {
		dbg("[NOTI] TCORE_NOTIFICATION_GPS_RESET_ASSIST_DATA");
		telephony_gps_emit_reset_assist_data(gps);
	} break;

	default: {
		err("Unhandled command: [0x%x]", command);
	}
	}

	return TRUE;
}
