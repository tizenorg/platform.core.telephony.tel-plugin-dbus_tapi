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

#include "dtapi_sap.h"
#include "dtapi_util.h"

#include <plugin.h>
#include <tel_sap.h>

#define AC_SAP		"telephony_framework::api_sap"

static void on_response_dtapi_sap_req_connect(gint result, const void *response, void *user_data)
{
	DbusRespCbData *rsp_cb_data = user_data;
	guint max_msg_size = 0;

	tcore_check_return_assert(NULL != rsp_cb_data);
	tcore_check_return_assert(NULL != response);

	if (result == TEL_SAP_RESULT_SUCCESS)
		max_msg_size = *(guint *)response;

	dbg("result = [%d], resp_max_size[%d]", result, max_msg_size);

	telephony_sap_complete_req_connect(rsp_cb_data->interface_object, rsp_cb_data->invocation,
									   result, max_msg_size);
	tcore_free(rsp_cb_data);
}

static gboolean dtapi_sap_req_connect(TelephonySap *sap, GDBusMethodInvocation *invocation,
			guint req_max_size, gpointer user_data)
{
	DbusRespCbData *rsp_cb_data;
	TcorePlugin *plugin = (TcorePlugin *)user_data;
	TelReturn ret;

	dbg("Entry");

	if (dtapi_check_access_control(invocation, AC_SAP, "x") == FALSE)
		return TRUE;

	rsp_cb_data = dtapi_create_resp_cb_data(sap, invocation, NULL, 0);

	ret = tcore_plugin_dispatch_request(plugin, TRUE, TCORE_COMMAND_SAP_REQ_CONNECT, &req_max_size,
				sizeof(req_max_size), on_response_dtapi_sap_req_connect, rsp_cb_data);

	if (ret != TEL_RETURN_SUCCESS) {
		dtapi_return_error(invocation, G_DBUS_ERROR_FAILED, "Dispatch failed");
		tcore_free(rsp_cb_data);
	}

	return TRUE;
}

static void on_response_dtapi_sap_req_disconnect(gint result, const void *response, void *user_data)
{
	DbusRespCbData *rsp_cb_data = user_data;
	tcore_check_return_assert(NULL != rsp_cb_data);

	dbg("result = [%d]", result);

	telephony_sap_complete_req_disconnect(rsp_cb_data->interface_object, rsp_cb_data->invocation, result);
	tcore_free(rsp_cb_data);
}

static gboolean dtapi_sap_req_disconnect(TelephonySap *sap, GDBusMethodInvocation *invocation, gpointer user_data)
{
	DbusRespCbData *rsp_cb_data;
	TcorePlugin *plugin = (TcorePlugin *)user_data;
	TelReturn ret;

	dbg("Entry");

	if (dtapi_check_access_control(invocation, AC_SAP, "x") == FALSE)
		return TRUE;

	rsp_cb_data = dtapi_create_resp_cb_data(sap, invocation, NULL, 0);

	ret = tcore_plugin_dispatch_request(plugin, TRUE, TCORE_COMMAND_SAP_REQ_DISCONNECT, NULL, 0,
				on_response_dtapi_sap_req_disconnect, rsp_cb_data);

	if (ret != TEL_RETURN_SUCCESS) {
		dtapi_return_error(invocation, G_DBUS_ERROR_FAILED, "Dispatch failed");
		tcore_free(rsp_cb_data);
	}

	return TRUE;
}

static void on_response_dtapi_sap_get_atr(gint result, const void *response, void *user_data)
{
	DbusRespCbData *rsp_cb_data = user_data;
	const TelSapAtr *sap_atr = response;
	gchar *encoded_atr = NULL;

	tcore_check_return_assert(NULL != rsp_cb_data);
	tcore_check_return_assert(NULL != sap_atr);
	tcore_check_return_assert(sap_atr->atr_len <= TEL_SAP_ATR_LEN_MAX);

	dbg("result = [%d]", result);

	if (result == TEL_SAP_RESULT_SUCCESS) {
		encoded_atr = g_base64_encode(sap_atr->atr, sap_atr->atr_len);
		dbg("encoded_atr: [%s] encoded_atr_len: [%d]", encoded_atr, strlen(encoded_atr));
	}

	telephony_sap_complete_get_atr(rsp_cb_data->interface_object, rsp_cb_data->invocation,
								   result, encoded_atr);
	tcore_free(encoded_atr);
	tcore_free(rsp_cb_data);
}

static gboolean dtapi_sap_get_atr(TelephonySap *sap, GDBusMethodInvocation *invocation, gpointer user_data)
{
	DbusRespCbData *rsp_cb_data;
	TcorePlugin *plugin = (TcorePlugin *)user_data;
	TelReturn ret;

	dbg("Entry");

	if (dtapi_check_access_control(invocation, AC_SAP, "r") == FALSE)
		return TRUE;

	rsp_cb_data = dtapi_create_resp_cb_data(sap, invocation, NULL, 0);

	ret = tcore_plugin_dispatch_request(plugin, TRUE, TCORE_COMMAND_SAP_GET_ATR, NULL, 0,
					on_response_dtapi_sap_get_atr, rsp_cb_data);

	if (ret != TEL_RETURN_SUCCESS) {
		dtapi_return_error(invocation, G_DBUS_ERROR_FAILED, "Dispatch failed");
		tcore_free(rsp_cb_data);
	}

	return TRUE;
}

static void on_response_dtapi_sap_req_transfer_apdu(gint result, const void *response, void *user_data)
{
	DbusRespCbData *rsp_cb_data = user_data;
	const TelSapApduResp *resp_apdu = response;
	gchar *encoded_apdu = NULL;

	tcore_check_return_assert(NULL != rsp_cb_data);
	tcore_check_return_assert(NULL != resp_apdu);
	tcore_check_return_assert(resp_apdu->apdu_resp_len  <= TEL_SAP_APDU_RESP_LEN_MAX);

	dbg("result = [%d]", result);

	if (result == TEL_SAP_RESULT_SUCCESS) {
		encoded_apdu = g_base64_encode(resp_apdu->apdu_resp, resp_apdu->apdu_resp_len);
		dbg("encoded_apdu: [%s] encoded_apdu_len: [%d]", encoded_apdu, strlen(encoded_apdu));
	}

	telephony_sap_complete_req_transfer_apdu(rsp_cb_data->interface_object, rsp_cb_data->invocation,
								   result, encoded_apdu);
	tcore_free(encoded_apdu);
	tcore_free(rsp_cb_data);
}

static gboolean dtapi_sap_req_transfer_apdu(TelephonySap *sap, GDBusMethodInvocation *invocation,
								gchar *apdu, gpointer user_data)
{
	DbusRespCbData *rsp_cb_data;
	TcorePlugin *plugin = (TcorePlugin *)user_data;
	TelReturn ret;
	TelSapApdu req_apdu;
	guchar *decoded_apdu = NULL;

	dbg("Entry");

	if (dtapi_check_access_control(invocation, AC_SAP, "x") == FALSE)
		return TRUE;

	memset(&req_apdu, 0, sizeof(TelSapApdu));

	decoded_apdu = g_base64_decode(apdu, &req_apdu.apdu_len);
	if ((decoded_apdu != NULL) && (req_apdu.apdu_len <= TEL_SAP_APDU_LEN_MAX)) {
		memcpy(req_apdu.apdu, decoded_apdu, req_apdu.apdu_len);
	} else {
		dtapi_return_error(invocation, G_DBUS_ERROR_FAILED, "Decoding APDU request failed");
		tcore_free(decoded_apdu);
		return TRUE;
	}

	rsp_cb_data = dtapi_create_resp_cb_data(sap, invocation, NULL, 0);

	ret = tcore_plugin_dispatch_request(plugin, TRUE, TCORE_COMMAND_SAP_REQ_TRANSFER_APDU,
			&req_apdu, sizeof(TelSapApdu), on_response_dtapi_sap_req_transfer_apdu, rsp_cb_data);

	if (ret != TEL_RETURN_SUCCESS) {
		dtapi_return_error(invocation, G_DBUS_ERROR_FAILED, "Dispatch failed");
		tcore_free(rsp_cb_data);
	}
	tcore_free(decoded_apdu);

	return TRUE;
}

static void on_response_dtapi_sap_req_transport_protocol(gint result, const void *response, void *user_data)
{
	DbusRespCbData *rsp_cb_data = user_data;
	tcore_check_return_assert(NULL != rsp_cb_data);

	dbg("result = [%d]", result);

	telephony_sap_complete_req_transport_protocol(rsp_cb_data->interface_object, rsp_cb_data->invocation, result);
	tcore_free(rsp_cb_data);
}

static gboolean dtapi_sap_req_transport_protocol(TelephonySap *sap, GDBusMethodInvocation *invocation,
							gint req_protocol, gpointer user_data)
{
	DbusRespCbData *rsp_cb_data;
	TcorePlugin *plugin = (TcorePlugin *)user_data;
	TelReturn ret;
	TelSimSapProtocol sap_protocoal;

	dbg("Entry");

	if (dtapi_check_access_control(invocation, AC_SAP, "w") == FALSE)
		return TRUE;

	rsp_cb_data = dtapi_create_resp_cb_data(sap, invocation, NULL, 0);

	sap_protocoal = req_protocol;

	ret = tcore_plugin_dispatch_request(plugin, TRUE, TCORE_COMMAND_SAP_REQ_TRANSPORT_PROTOCOL,
			&sap_protocoal, sizeof(TelSimSapProtocol), on_response_dtapi_sap_req_transport_protocol, rsp_cb_data);

	if (ret != TEL_RETURN_SUCCESS) {
		dtapi_return_error(invocation, G_DBUS_ERROR_FAILED, "Dispatch failed");
		tcore_free(rsp_cb_data);
	}

	return TRUE;
}

static void on_response_dtapi_sap_req_power_operation(gint result, const void *response, void *user_data)
{
	DbusRespCbData *rsp_cb_data = user_data;
	tcore_check_return_assert(NULL != rsp_cb_data);

	dbg("result = [%d]", result);

	telephony_sap_complete_req_power_operation(rsp_cb_data->interface_object, rsp_cb_data->invocation, result);
	tcore_free(rsp_cb_data);
}

static gboolean dtapi_sap_req_power_operation(TelephonySap *sap, GDBusMethodInvocation *invocation,
						gint req_power_mode, gpointer user_data)
{
	DbusRespCbData *rsp_cb_data;
	TcorePlugin *plugin = (TcorePlugin *)user_data;
	TelReturn ret;
	TelSapPowerMode sap_power_mode;

	dbg("Entry");

	if (dtapi_check_access_control(invocation, AC_SAP, "w") == FALSE)
		return TRUE;

	rsp_cb_data = dtapi_create_resp_cb_data(sap, invocation, NULL, 0);

	sap_power_mode = req_power_mode;

	ret = tcore_plugin_dispatch_request(plugin, TRUE, TCORE_COMMAND_SAP_REQ_POWER_OPERATION,
			&sap_power_mode, sizeof(TelSapPowerMode), on_response_dtapi_sap_req_power_operation, rsp_cb_data);

	if (ret != TEL_RETURN_SUCCESS) {
		dtapi_return_error(invocation, G_DBUS_ERROR_FAILED, "Dispatch failed");
		tcore_free(rsp_cb_data);
	}

	return TRUE;
}

static void on_response_dtapi_sap_get_card_reader_status(gint result, const void *response, void *user_data)
{
	DbusRespCbData *rsp_cb_data = user_data;
	gint sap_status = 0;
	tcore_check_return_assert(NULL != rsp_cb_data);
	tcore_check_return_assert(NULL != response);

	if(result == TEL_SAP_RESULT_SUCCESS)
		sap_status = *(gint *)response;

	dbg("result = [%d] sap status = [%d]", result,  sap_status);

	telephony_sap_complete_get_card_reader_status(rsp_cb_data->interface_object, rsp_cb_data->invocation,
									result, sap_status);
	tcore_free(rsp_cb_data);
}

static gboolean dtapi_sap_get_card_reader_status(TelephonySap *sap, GDBusMethodInvocation *invocation,
					gpointer user_data)
{
	DbusRespCbData *rsp_cb_data;
	TcorePlugin *plugin = (TcorePlugin *)user_data;
	TelReturn ret;

	dbg("Entry");

	if (dtapi_check_access_control(invocation, AC_SAP, "r") == FALSE)
		return TRUE;

	rsp_cb_data = dtapi_create_resp_cb_data(sap, invocation, NULL, 0);

	ret = tcore_plugin_dispatch_request(plugin, TRUE, TCORE_COMMAND_SAP_GET_CARDREADER_STATUS, NULL, 0,
					on_response_dtapi_sap_get_card_reader_status, rsp_cb_data);

	if (ret != TEL_RETURN_SUCCESS) {
		dtapi_return_error(invocation, G_DBUS_ERROR_FAILED, "Dispatch failed");
		tcore_free(rsp_cb_data);
	}

	return TRUE;
}

gboolean dtapi_setup_sap_interface(TelephonyObjectSkeleton *object, TcorePlugin *plugin)
{
	TelephonySap *sap = telephony_sap_skeleton_new();
	tcore_check_return_value_assert(NULL != sap, FALSE);

	telephony_object_skeleton_set_sap(object, sap);
	g_object_unref(sap);

	dbg("sap = %p", sap);

	g_signal_connect (sap,
			"handle-req-connect",
			G_CALLBACK (dtapi_sap_req_connect),
			plugin);

	g_signal_connect (sap,
			"handle-req-disconnect",
			G_CALLBACK (dtapi_sap_req_disconnect),
			plugin);

	g_signal_connect (sap,
			"handle-get-atr",
			G_CALLBACK (dtapi_sap_get_atr),
			plugin);

	g_signal_connect (sap,
			"handle-req-transfer-apdu",
			G_CALLBACK (dtapi_sap_req_transfer_apdu),
			plugin);

	g_signal_connect (sap,
			"handle-req-transport-protocol",
			G_CALLBACK (dtapi_sap_req_transport_protocol),
			plugin);

	g_signal_connect (sap,
			"handle-req-power-operation",
			G_CALLBACK (dtapi_sap_req_power_operation),
			plugin);

	g_signal_connect (sap,
			"handle-get-card-reader-status",
			G_CALLBACK (dtapi_sap_get_card_reader_status),
			plugin);

	return TRUE;
}

gboolean dtapi_handle_sap_notification(TelephonyObjectSkeleton *object, TcorePlugin *plugin,
	TcoreNotification command, guint data_len, const void *data)
{
	TelephonySap *sap;

	tcore_check_return_value_assert(NULL != object, FALSE);
	tcore_check_return_value_assert(NULL != data, FALSE);
	sap = telephony_object_peek_sap(TELEPHONY_OBJECT(object));

	switch (command) {
	case TCORE_NOTIFICATION_SAP_STATUS:
	{
		int sap_status = *(int *)data;
		dbg("sap(%p) : sap_status = [%d]", sap, sap_status);
		telephony_sap_emit_status(sap, sap_status);
	}
	break;
	default:
		err("not handled command[0x%x]", command);
	break;
	}
	return TRUE;
}
