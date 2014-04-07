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

#include "dtapi_ss.h"
#include "dtapi_util.h"

#include <appsvc.h>
#include <bundle.h>
#include <plugin.h>
#include <tel_ss.h>

#define AC_SS		"telephony_framework::api_ss"
#define MAX_SS_USSD_LEN 208

typedef struct{
	int status;
	guint length;
	char data[MAX_SS_USSD_LEN + 1];
} CissDataType;

static void _launch_ciss(const TelSsUssdNoti *ussd)
{
	gchar *encoded_data;
	CissDataType ciss_data;

	tcore_check_return_assert(NULL != ussd);
	bundle *kb = bundle_create();
	if (!kb) {
		err("bundle_create() failed");
		return;
	}

	memset(&ciss_data, 0x00, sizeof(CissDataType));
	ciss_data.status = ussd->status;

	if(NULL == ussd->str) {
		err("Empty USSD string");
		bundle_free(kb);
		return;
	}
	ciss_data.length = strlen((const char*)ussd->str);
	if(ciss_data.length > MAX_SS_USSD_LEN) {
		err("USSD data length is more, Unable to launch ciss");
		bundle_free(kb);
		return;
	}
	g_strlcpy(ciss_data.data, (gchar *)ussd->str, MAX_SS_USSD_LEN + 1);

	dbg("launch ciss application by appsvc");
	appsvc_set_operation(kb, "http://tizen.org/appcontrol/operation/ciss");
	appsvc_set_pkgname(kb, "com.samsung.ciss");

	encoded_data = g_base64_encode((guchar *)&ciss_data, sizeof(CissDataType));
	appsvc_add_data(kb, "CISS_LAUNCHING_MODE", "RESP");
	appsvc_add_data(kb, "KEY_EVENT_TYPE", "100");
	appsvc_add_data(kb, "KEY_ENCODED_DATA", encoded_data);

	dbg("ciss appsvc run");
	appsvc_run_service(kb, 0, NULL, NULL);

	bundle_free(kb);
	g_free(encoded_data);
}

static void on_response_dtapi_ss_set_barring(gint result,
	const void *data, void *cb_data)
{
	DbusRespCbData *resp_cb_data = cb_data;

	dbg("Barring set response result: [%d]", result);
	tcore_check_return_assert(NULL != resp_cb_data);

	telephony_ss_complete_set_barring(resp_cb_data->interface_object,
		resp_cb_data->invocation, result);

	tcore_free(resp_cb_data);
}

static gboolean dtapi_ss_set_barring(TelephonySs *ss,
	GDBusMethodInvocation *invocation, gint class,
	gboolean enable, gint type, const gchar *pwd,
	gpointer user_data)
{
	TelSsBarringInfo req;
	TelReturn result;
	TcorePlugin *plugin = user_data;
	DbusRespCbData *cb_data = NULL;

	if (dtapi_check_access_control(invocation, AC_SS, "x") == FALSE)
		return TRUE;

	req.class = class;
	req.enable = enable;
	req.type = type;
	g_strlcpy(req.pwd, pwd, TEL_SS_BARRING_PASSWORD_LEN_MAX + 1 );
	dbg("barring password: [%s] class: [%d], barring type: [%d]", req.pwd, req.class, req.type);

	cb_data = dtapi_create_resp_cb_data(ss, invocation, NULL, 0);

	result = tcore_plugin_dispatch_request(plugin, TRUE,
		TCORE_COMMAND_SS_SET_BARRING,
		&req, sizeof(TelSsBarringInfo),
		on_response_dtapi_ss_set_barring, cb_data);
	if (result != TEL_RETURN_SUCCESS) {
		dtapi_return_error(invocation, G_DBUS_ERROR_FAILED, "Dispatch failed");
		tcore_free(cb_data);
	}

	return TRUE;
}

static void on_response_dtapi_ss_get_barring_status(gint result,
	const void *data, void *cb_data)
{
	DbusRespCbData *resp_cb_data = cb_data;
	GVariant *records = NULL;
	GVariantBuilder builder;
	guint count = 0;

	dbg("Barring get status response result: [%d]", result);

	tcore_check_return_assert(NULL != resp_cb_data);

	g_variant_builder_init(&builder, G_VARIANT_TYPE("aa{sv}"));
	if (result == TEL_SIM_RESULT_SUCCESS) {
		const TelSsBarringResp *resp = data;
		tcore_check_return_assert(NULL != resp);

		dbg("Barring status List count: [%d]", resp->record_num);
		for (count = 0; count < resp->record_num; count++) {
			g_variant_builder_open(&builder, G_VARIANT_TYPE("a{sv}"));

			g_variant_builder_add(&builder, "{sv}",
				"class", g_variant_new_int32(resp->records[count].class));
			g_variant_builder_add(&builder, "{sv}",
				"enable", g_variant_new_boolean(resp->records[count].enable));
			g_variant_builder_add(&builder, "{sv}",
				"type", g_variant_new_int32(resp->records[count].type));

			g_variant_builder_close(&builder);

			dbg("[%d] : Class: [%d] Enable: [%s] Type: [%d]", count,
				resp->records[count].class,
				(resp->records[count].enable ? "YES" : "NO"),
				resp->records[count].type);
		}
	}
	records = g_variant_builder_end(&builder);

	telephony_ss_complete_get_barring_status(resp_cb_data->interface_object,
		resp_cb_data->invocation, result, count, records);
	g_variant_unref(records);

	tcore_free(resp_cb_data);
}

static gboolean dtapi_ss_get_barring_status(TelephonySs *ss,
	GDBusMethodInvocation *invocation,
	gint class, gint type, gpointer user_data)
{
	TelSsBarringGetInfo req;
	TelReturn result;
	TcorePlugin *plugin = user_data;
	DbusRespCbData *cb_data = NULL;

	if (dtapi_check_access_control(invocation, AC_SS, "r") == FALSE)
		return TRUE;

	req.class = class;
	req.type = type;

	dbg("class: %d, type: %d", req.class, req.type);
	cb_data = dtapi_create_resp_cb_data(ss, invocation, NULL, 0);

	result = tcore_plugin_dispatch_request(plugin, TRUE,
		TCORE_COMMAND_SS_GET_BARRING_STATUS,
		&req, sizeof(TelSsBarringGetInfo),
		on_response_dtapi_ss_get_barring_status, cb_data);
	if (result != TEL_RETURN_SUCCESS) {
		dtapi_return_error(invocation, G_DBUS_ERROR_FAILED, "Dispatch failed");
		tcore_free(cb_data);
	}

	return TRUE;
}

static void on_response_dtapi_ss_change_barring_password(
	gint result, const void *data, void *cb_data)
{
	DbusRespCbData *resp_cb_data = cb_data;

	dbg("Barring change password response result: [%d]", result);

	tcore_check_return_assert(NULL != resp_cb_data);

	telephony_ss_complete_change_barring_password(resp_cb_data->interface_object,
		resp_cb_data->invocation, result);

	tcore_free(resp_cb_data);
}

static gboolean dtapi_ss_change_barring_password(TelephonySs *ss,
	GDBusMethodInvocation *invocation,
	const gchar *old_pwd, const gchar *new_pwd,
	gpointer user_data)
{
	TelSsBarringPwdInfo req;
	TelReturn result;
	TcorePlugin *plugin = user_data;
	DbusRespCbData *cb_data = NULL;

	if (dtapi_check_access_control(invocation, AC_SS, "x") == FALSE)
		return TRUE;

	g_strlcpy(req.old_pwd, old_pwd, TEL_SS_BARRING_PASSWORD_LEN_MAX + 1);
	g_strlcpy(req.new_pwd, new_pwd, TEL_SS_BARRING_PASSWORD_LEN_MAX + 1);

	cb_data = dtapi_create_resp_cb_data(ss, invocation, NULL, 0);

	result = tcore_plugin_dispatch_request(plugin, TRUE,
		TCORE_COMMAND_SS_CHANGE_BARRING_PASSWORD,
		&req, sizeof(TelSsBarringPwdInfo),
		on_response_dtapi_ss_change_barring_password, cb_data);
	if (result != TEL_RETURN_SUCCESS) {
		dtapi_return_error(invocation, G_DBUS_ERROR_FAILED, "Dispatch failed");
		tcore_free(cb_data);
	}

	return TRUE;
}

static void on_response_dtapi_ss_set_forwarding(gint result,
	const void *data, void *cb_data)
{
	DbusRespCbData *resp_cb_data = cb_data;

	dbg("Forwarding set response result: [%d]", result);
	tcore_check_return_assert(NULL != resp_cb_data);

	telephony_ss_complete_set_forwarding(resp_cb_data->interface_object,
		resp_cb_data->invocation, result);

	tcore_free(resp_cb_data);
}

static gboolean dtapi_ss_set_forwarding(TelephonySs *ss,
	GDBusMethodInvocation *invocation,
	gint class, gint mode, gint condition,
	const gchar *number, gint wait_time,
	gpointer user_data)
{
	TelSsForwardInfo req;
	TelReturn result;
	TcorePlugin *plugin = user_data;
	DbusRespCbData *cb_data = NULL;

	if (dtapi_check_access_control(invocation, AC_SS, "x") == FALSE)
		return TRUE;

	req.class = class;
	req.mode = mode;
	req.condition = condition;
	req.wait_time = wait_time;
	g_strlcpy(req.number, number, TEL_SS_NUMBER_LEN_MAX + 1);

	dbg("class: %d, mode: %d, condition: %d,  wait_time: %d, number: %s",
			req.class, req.mode, req.condition, req.wait_time, req.number);

	cb_data = dtapi_create_resp_cb_data(ss, invocation, NULL, 0);

	result = tcore_plugin_dispatch_request(plugin, TRUE,
		TCORE_COMMAND_SS_SET_FORWARDING,
		&req, sizeof(TelSsForwardInfo),
		on_response_dtapi_ss_set_forwarding, cb_data);
	if (result != TEL_RETURN_SUCCESS ) {
		dtapi_return_error(invocation, G_DBUS_ERROR_FAILED, "Dispatch failed");
		tcore_free(cb_data);
	}

	return TRUE;
}

static void on_response_dtapi_ss_get_forwarding_status(gint result,
	const void *data, void *cb_data)
{
	DbusRespCbData *resp_cb_data = cb_data;
	GVariant *records = NULL;
	GVariantBuilder builder;
	guint count = 0;

	dbg("Forwarding get status response result: [%d]", result);

	tcore_check_return_assert(NULL != resp_cb_data);

	g_variant_builder_init(&builder, G_VARIANT_TYPE("aa{sv}"));
	if (result == TEL_SIM_RESULT_SUCCESS) {
		const TelSsForwardingResp *resp = data;
		tcore_check_return_assert(NULL != resp);

		dbg("Forwarding status List count: [%d]", resp->record_num);
		for (count = 0; count < resp->record_num; count++) {
			g_variant_builder_open(&builder, G_VARIANT_TYPE("a{sv}"));

			g_variant_builder_add(&builder, "{sv}",
				"class", g_variant_new_int32(resp->records[count].class));
			g_variant_builder_add(&builder, "{sv}",
				"enable", g_variant_new_boolean(resp->records[count].enable));
			g_variant_builder_add(&builder, "{sv}",
				"condition", g_variant_new_int32(resp->records[count].condition));
			g_variant_builder_add(&builder, "{sv}",
				"number", g_variant_new_string(resp->records[count].number));
			g_variant_builder_add(&builder, "{sv}",
				"wait_time", g_variant_new_int32(resp->records[count].wait_time));

			g_variant_builder_close(&builder);

			dbg("[%d] : Class: [%d] Enable: [%s] Condition: [%d] "\
				"Number: [%s] Wait time: [%d]", count,
				resp->records[count].class,
				(resp->records[count].enable ? "YES" : "NO"),
				resp->records[count].condition,
				resp->records[count].number,
				resp->records[count].wait_time);
		}
	}
	records = g_variant_builder_end(&builder);

	telephony_ss_complete_get_forwarding_status(resp_cb_data->interface_object,
			resp_cb_data->invocation, result, count, records);

	g_variant_unref(records);
	tcore_free(resp_cb_data);
}

static gboolean dtapi_ss_get_forwarding_status(TelephonySs *ss,
	GDBusMethodInvocation *invocation,
	gint class, gint condition, gpointer user_data)
{
	TelSsForwardGetInfo req;
	TelReturn result;
	TcorePlugin *plugin = user_data;
	DbusRespCbData *cb_data = NULL;

	if (dtapi_check_access_control(invocation, AC_SS, "r") == FALSE)
		return TRUE;

	req.class = class;
	req.condition = condition;
	dbg("class: %d, condition: %d", req.class, req.condition);

	cb_data = dtapi_create_resp_cb_data(ss, invocation, NULL, 0);

	result = tcore_plugin_dispatch_request(plugin, TRUE,
		TCORE_COMMAND_SS_GET_FORWARDING_STATUS,
		&req, sizeof(TelSsForwardGetInfo),
		on_response_dtapi_ss_get_forwarding_status, cb_data);
	if (result != TEL_RETURN_SUCCESS) {
		dtapi_return_error(invocation, G_DBUS_ERROR_FAILED, "Dispatch failed");
		tcore_free(cb_data);
	}

	return TRUE;
}

static void on_response_dtapi_ss_set_waiting(gint result,
	const void *data, void *cb_data)
{
	DbusRespCbData *resp_cb_data = cb_data;

	dbg("Waiting set response result: [%d]", result);
	tcore_check_return_assert(NULL != resp_cb_data);

	telephony_ss_complete_set_waiting(resp_cb_data->interface_object,
		resp_cb_data->invocation, result);

	tcore_free(resp_cb_data);
}

static gboolean dtapi_ss_set_waiting(TelephonySs *ss,
	GDBusMethodInvocation *invocation,
	gint class, gboolean enable, gpointer user_data)
{
	TelSsWaitingInfo req;
	TelReturn result;
	TcorePlugin *plugin = user_data;
	DbusRespCbData *cb_data = NULL;

	if (dtapi_check_access_control(invocation, AC_SS, "x") == FALSE)
		return TRUE;

	req.class = class;
	req.enable = enable;
	dbg("class: %d, enable: %d", req.class, req.enable);

	cb_data = dtapi_create_resp_cb_data(ss, invocation, NULL, 0);
	result = tcore_plugin_dispatch_request(plugin, TRUE,
		TCORE_COMMAND_SS_SET_WAITING,
		&req, sizeof(TelSsWaitingInfo),
		on_response_dtapi_ss_set_waiting, cb_data);
	if (result != TEL_RETURN_SUCCESS ) {
		dtapi_return_error(invocation, G_DBUS_ERROR_FAILED, "Dispatch failed");
		tcore_free(cb_data);
	}

	return TRUE;
}

static void on_response_dtapi_ss_get_waiting_status(gint result,
	const void *data, void *cb_data)
{
	DbusRespCbData *resp_cb_data = cb_data;
	GVariant *records = NULL;
	GVariantBuilder builder;
	guint count = 0;

	dbg("Waiting get status response result: [%d]", result);

	tcore_check_return_assert(NULL != resp_cb_data);

	g_variant_builder_init(&builder, G_VARIANT_TYPE("aa{sv}"));
	if (result == TEL_SS_RESULT_SUCCESS) {
		const TelSsWaitingResp *resp = data;
		tcore_check_return_assert(NULL != resp);

		dbg("Waiting status List count: [%d]", resp->record_num);
		for (count = 0; count < resp->record_num; count++) {
			g_variant_builder_open(&builder, G_VARIANT_TYPE("a{sv}"));

			g_variant_builder_add(&builder, "{sv}",
				"class", g_variant_new_int32(resp->records[count].class));
			g_variant_builder_add(&builder, "{sv}",
				"enable", g_variant_new_boolean(resp->records[count].enable));

			g_variant_builder_close(&builder);

			dbg("[%d] : Class: [%d] Enable: [%s]", count,
				resp->records[count].class,
				(resp->records[count].enable ? "YES" : "NO"));
		}
	}
	records = g_variant_builder_end(&builder);

	telephony_ss_complete_get_waiting_status(resp_cb_data->interface_object,
		resp_cb_data->invocation, result, count, records);
	g_variant_unref(records);

	tcore_free(resp_cb_data);
}

static gboolean dtapi_ss_get_waiting_status(TelephonySs *ss,
	GDBusMethodInvocation *invocation,
	gint class, gpointer user_data)
{
	TelSsClass req;
	TelReturn result;
	TcorePlugin *plugin = user_data;
	DbusRespCbData *cb_data = NULL;

	if (dtapi_check_access_control(invocation, AC_SS, "r") == FALSE)
		return TRUE;

	req = class;
	dbg("class: %d", req);

	cb_data = dtapi_create_resp_cb_data(ss, invocation, NULL, 0);

	result = tcore_plugin_dispatch_request(plugin, TRUE,
		TCORE_COMMAND_SS_GET_WAITING_STATUS,
		&req, sizeof(TelSsClass),
		on_response_dtapi_ss_get_waiting_status, cb_data);
	if (result != TEL_RETURN_SUCCESS) {
		dtapi_return_error(invocation, G_DBUS_ERROR_FAILED, "Dispatch failed");
		tcore_free(cb_data);
	}

	return TRUE;
}

static void on_response_dtapi_ss_set_cli(gint result,
	const void *data, void *cb_data)
{
	DbusRespCbData *resp_cb_data = cb_data;

	dbg("CLI set response result: [%d]", result);
	tcore_check_return_assert(NULL != resp_cb_data);

	telephony_ss_complete_set_cli(resp_cb_data->interface_object,
		resp_cb_data->invocation, result);

	tcore_free(resp_cb_data);
}

static gboolean dtapi_ss_set_cli(TelephonySs *ss,
	GDBusMethodInvocation *invocation,
	gint type, gint status, gpointer user_data)
{
	TelSsCliInfo req;
	TelReturn result;
	TcorePlugin *plugin = user_data;
	DbusRespCbData *cb_data = NULL;

	if (dtapi_check_access_control(invocation, AC_SS, "x") == FALSE)
		return TRUE;

	req.type = type;
	switch(req.type){
	case TEL_SS_CLI_CLIR:
		req.status.clir = status;
		break;
	case TEL_SS_CLI_CLIP:
		req.status.clip = status;
		break;
	case TEL_SS_CLI_COLP:
		req.status.colp = status;
		break;
	case TEL_SS_CLI_COLR:
		req.status.colr = status;
		break;
	case TEL_SS_CLI_CDIP:
		req.status.cdip = status;
		break;
	case TEL_SS_CLI_CNAP:
		req.status.cnap = status;
		break;
	}

	dbg("type: %d status: %d", req.type, status);

	cb_data = dtapi_create_resp_cb_data(ss, invocation, NULL, 0);

	result = tcore_plugin_dispatch_request(plugin, TRUE,
		TCORE_COMMAND_SS_SET_CLI,
		&req, sizeof(TelSsCliInfo),
		on_response_dtapi_ss_set_cli, cb_data);
	if (result != TEL_RETURN_SUCCESS) {
		dtapi_return_error(invocation, G_DBUS_ERROR_FAILED, "Dispatch failed");
		tcore_free(cb_data);
	}

	return TRUE;
}

static void on_response_dtapi_ss_get_cli_status(gint result,
	const void *data, void *cb_data)
{
	DbusRespCbData *resp_cb_data = cb_data;

	const TelSsCliResp *resp = data;
	gint net_status,dev_status;

	dbg("CLI get status response result: [%d]", result);
	tcore_check_return_assert(NULL != resp_cb_data);

	if (result != TEL_RETURN_SUCCESS) {
		telephony_ss_complete_get_cli_status(resp_cb_data->interface_object,
			resp_cb_data->invocation, result, -1, -1, -1);
		tcore_free(resp_cb_data);
		return;
	}

	tcore_check_return_assert(NULL != resp);

	switch(resp->type){
	case TEL_SS_CLI_CLIR:
		net_status = resp->status.clir.net_status;
		dev_status = resp->status.clir.dev_status;
	break;

	case TEL_SS_CLI_CLIP:
		net_status = resp->status.clip.net_status;
		dev_status = resp->status.clip.dev_status;
	break;

	case TEL_SS_CLI_COLP:
		net_status = resp->status.colp.net_status;
		dev_status = resp->status.colp.dev_status;
	break;

	case TEL_SS_CLI_COLR:
		net_status = resp->status.colr.net_status;
		dev_status = resp->status.colr.dev_status;
	break;

	case TEL_SS_CLI_CDIP:
		net_status = resp->status.cdip.net_status;
		dev_status = resp->status.cdip.dev_status;
	break;

	case TEL_SS_CLI_CNAP:
		net_status = resp->status.cnap.net_status;
		dev_status = resp->status.cnap.dev_status;
	break;

	default :
		net_status = -1;
		dev_status = -1;
		result = TEL_SS_RESULT_FAILURE;
	break;
	}

	telephony_ss_complete_get_cli_status(resp_cb_data->interface_object,
		resp_cb_data->invocation, result,
		resp->type, net_status, dev_status);

	tcore_free(resp_cb_data);
}

static gboolean dtapi_ss_get_cli_status(TelephonySs *ss,
	GDBusMethodInvocation *invocation,
	gint type, gpointer user_data)
{
	TelSsCliType req;
	TelReturn result;
	TcorePlugin *plugin = user_data;
	DbusRespCbData *cb_data = NULL;

	if (dtapi_check_access_control(invocation, AC_SS, "r") == FALSE)
		return TRUE;

	req = type;
	dbg("type: [%d]", req);

	cb_data = dtapi_create_resp_cb_data(ss, invocation, NULL, 0);

	result = tcore_plugin_dispatch_request(plugin, TRUE,
		TCORE_COMMAND_SS_GET_CLI_STATUS,
		&req, sizeof(TelSsCliType),
		on_response_dtapi_ss_get_cli_status, cb_data);
	if (result != TEL_RETURN_SUCCESS) {
		dtapi_return_error(invocation, G_DBUS_ERROR_FAILED, "Dispatch failed");
		tcore_free(cb_data);
	}

	return TRUE;
}

static void on_response_dtapi_ss_send_ussd_request(gint result,
	const void *data, void *cb_data)
{
	DbusRespCbData *resp_cb_data = cb_data;

	const TelSsUssdResp *resp = data;

	dbg("Send USSD response result: [%d]", result);
	tcore_check_return_assert(NULL != resp_cb_data);

	if (result != TEL_RETURN_SUCCESS) {
		telephony_ss_complete_send_ussd_request(resp_cb_data->interface_object,
			resp_cb_data->invocation, result, -1, -1, NULL);
		tcore_free(resp_cb_data);
		return;
	}

	tcore_check_return_assert(NULL != resp);

	telephony_ss_complete_send_ussd_request(resp_cb_data->interface_object,
		resp_cb_data->invocation, result,
		resp->type, resp->status, (const gchar *)resp->str);

	tcore_free(resp_cb_data);
}

static gboolean dtapi_ss_send_ussd_request(TelephonySs *ss,
	GDBusMethodInvocation *invocation,
	gint type, const gchar *str, gpointer user_data)
{
	TelSsUssdInfo req;
	TelReturn result;
	TcorePlugin *plugin = user_data;
	DbusRespCbData *cb_data = NULL;

	if (dtapi_check_access_control(invocation, AC_SS, "x") == FALSE)
		return TRUE;

	req.type = type;
	req.str = (unsigned char*)tcore_strdup(str);
	dbg("type: %d, string: %s", req.type, req.str);

	cb_data = dtapi_create_resp_cb_data(ss, invocation, NULL, 0);

	result = tcore_plugin_dispatch_request(plugin, TRUE,
		TCORE_COMMAND_SS_SEND_USSD_REQUEST,
		&req, sizeof(TelSsUssdInfo),
		on_response_dtapi_ss_send_ussd_request, cb_data);
	if (result != TEL_RETURN_SUCCESS) {
		dtapi_return_error(invocation, G_DBUS_ERROR_FAILED, "Dispatch failed");
		tcore_free(cb_data);
	}
	tcore_free(req.str);

	return TRUE;
}

gboolean dtapi_setup_ss_interface(TelephonyObjectSkeleton *object,
	TcorePlugin *plugin)
{
	TelephonySs *ss;

	tcore_check_return_value_assert(NULL != object, FALSE);
	tcore_check_return_value_assert(NULL != plugin, FALSE);

	ss = telephony_ss_skeleton_new();
	telephony_object_skeleton_set_ss(object, ss);
	g_object_unref(ss);

	tcore_check_return_value_assert(NULL != ss, FALSE);
	dbg("ss: [%p]", ss);

	g_signal_connect (ss,
		"handle-set-barring",
		G_CALLBACK (dtapi_ss_set_barring),
		plugin);

	g_signal_connect (ss,
		"handle-get-barring-status",
		G_CALLBACK (dtapi_ss_get_barring_status),
		plugin);

	g_signal_connect (ss,
		"handle-change-barring-password",
		G_CALLBACK (dtapi_ss_change_barring_password),
		plugin);


	g_signal_connect (ss,
		"handle-set-forwarding",
		G_CALLBACK (dtapi_ss_set_forwarding),
		plugin);

	g_signal_connect (ss,
		"handle-get-forwarding-status",
		G_CALLBACK (dtapi_ss_get_forwarding_status),
		plugin);

	g_signal_connect (ss,
		"handle-set-waiting",
		G_CALLBACK (dtapi_ss_set_waiting),
		plugin);

	g_signal_connect (ss,
		"handle-get-waiting-status",
		G_CALLBACK (dtapi_ss_get_waiting_status),
		plugin);

	g_signal_connect (ss,
		"handle-set-cli",
		G_CALLBACK (dtapi_ss_set_cli),
		plugin);

	g_signal_connect (ss,
		"handle-get-cli-status",
		G_CALLBACK (dtapi_ss_get_cli_status),
		plugin);

	g_signal_connect (ss,
		"handle-send-ussd-request",
		G_CALLBACK (dtapi_ss_send_ussd_request),
		plugin);

	return TRUE;
}

gboolean dtapi_handle_ss_notification(TelephonyObjectSkeleton *object,
	TcorePlugin *plugin, TcoreNotification command,
	guint data_len, const void *data)
{
	TelephonySs *ss;

	tcore_check_return_value_assert(NULL != object, FALSE);
	tcore_check_return_value_assert(NULL != plugin, FALSE);

	ss = telephony_object_peek_ss(TELEPHONY_OBJECT(object));
	tcore_check_return_value_assert(NULL != ss, FALSE);
	dbg("ss: [%p]", ss);

	switch (command) {
	case TCORE_NOTIFICATION_SS_USSD: {
		TelSsUssdNoti *ussd = (TelSsUssdNoti *)data;
		tcore_check_return_value_assert(NULL != ussd, FALSE);

		telephony_ss_emit_notify_ussd(ss, ussd->status,
				(const gchar*)ussd->str);

		/* Launch CISS application */
		_launch_ciss(ussd);
	}
	break;

	default:
		err("Unsupported notification: [0x%x]", command);
	break;
	}
	return TRUE;
}
