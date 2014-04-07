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

#include "dtapi_call.h"
#include "dtapi_util.h"

#include <stdio.h>
#include <aul.h>
#include <appsvc.h>
#include <bundle.h>
#include <tcore.h>
#include <plugin.h>
#include <co_call.h>

#define AC_CALL	"telephony_framework::api_call"

static void _launch_voice_call(const TelCallIncomingInfo *incoming_call_info)
{
	char id[2] = {0, };
	char cli[2] = {0, };
	char forward[2] = {0, };
	char active_line[2] = {0, };
	char cna[2] = {0, };
	char number[TEL_CALL_CALLING_NUMBER_LEN_MAX + 1] = {0, };
	char name[TEL_CALL_CALLING_NAME_LEN_MAX + 1] = {0, };

	bundle *kb  = 0;

	snprintf(id, 2, "%d", incoming_call_info->call_id);
	dbg("id : [%s]", id );
	snprintf(cli, 2, "%d", incoming_call_info->cli_validity);
	dbg("cli : [%s]", cli );
	snprintf(number, TEL_CALL_CALLING_NUMBER_LEN_MAX + 1, "%s", incoming_call_info->number);
	dbg("number : [%s]", number );
	snprintf(forward, 2, "%d", incoming_call_info->forward);
	dbg("forward : [%s]", forward );
	snprintf(active_line, 2, "%d", incoming_call_info->active_line);
	dbg("active_line : [%s]", active_line );

	if (incoming_call_info->cni_validity == TEL_CALL_CNI_VALIDITY_VALID)
		snprintf(cna, 2, "%d", 0);
	else
		snprintf(cna, 2, "%d", 1);

	dbg("cna : [%s]", cna);
	snprintf(name, TEL_CALL_CALLING_NAME_LEN_MAX + 1, "%s", incoming_call_info->name);
	dbg("name : [%s]", name);

	kb = bundle_create();
	if(!kb) {
		err("bundle creation failed");
		return;
	}
	/* AppSvc */
	appsvc_set_operation(kb, APPSVC_OPERATION_CALL);
	appsvc_set_uri(kb,"tel:MT");

	appsvc_add_data(kb, "launch-type", "MT");
	appsvc_add_data(kb, "handle", id);
	appsvc_add_data(kb, "number", number);
	appsvc_add_data(kb, "name_mode", cna);
	appsvc_add_data(kb, "name", name);
	appsvc_add_data(kb, "clicause", cli);
	appsvc_add_data(kb, "fwded", forward);
	appsvc_add_data(kb, "activeline", active_line);

	appsvc_run_service(kb, 0, NULL, NULL);
	bundle_free(kb);
}

static void
_launch_video_call(const TelCallIncomingInfo *incoming_call_info)
{
	char id[2] = {0, };
	char cli[2] = {0, };
	char forward[2] = {0, };
	char number[TEL_CALL_CALLING_NUMBER_LEN_MAX + 1] = {0, };
	int ret = 0;
	bundle *kb  = 0;

	snprintf( id, 2, "%d", incoming_call_info->call_id);
	dbg("id : [%s]", id );
	snprintf( number, TEL_CALL_CALLING_NUMBER_LEN_MAX + 1, "%s", incoming_call_info->number);
	dbg("number : [%s]", number );
	snprintf( cli, 2, "%d", incoming_call_info->cli_validity);
	dbg("cli : [%s]", cli );
	snprintf( forward, 2, "%d", incoming_call_info->forward);
	dbg("forward : [%s]", forward );

	kb = bundle_create();
	if(!kb) {
		err("bundle creation failed");
		return;
	}
	bundle_add(kb, "KEY_CALL_TYPE", "mt");
	bundle_add(kb, "KEY_CALL_HANDLE", id);
	bundle_add(kb, "KEY_CALLING_PARTY_NUMBER", number);
	bundle_add(kb, "KEY_CLI_CAUSE", cli);
	bundle_add(kb, "KEY_FORWARDED", forward);

	ret = aul_launch_app("com.samsung.vtmain", kb);
	bundle_free(kb);

	dbg("VT AUL return: [%d]",ret);
}

/* To be moved inside libtcore */
static gboolean
_check_call_type(TelCallType type)
{
	switch (type) {
	case TEL_CALL_TYPE_VOICE:
	case TEL_CALL_TYPE_VIDEO:
	case TEL_CALL_TYPE_E911:
		return TRUE;
	}

	return FALSE;
}

/* To be moved inside libtcore */
static gboolean
_check_call_ecc(TelCallEmergencyCategory ecc)
{
	switch (ecc) {
	case TEL_CALL_ECC_DEFAULT:
	case TEL_CALL_ECC_POLICE:
	case TEL_CALL_ECC_AMBULANCE:
	case TEL_CALL_ECC_FIREBRIGADE:
	case TEL_CALL_ECC_MARINEGUARD:
	case TEL_CALL_ECC_MOUNTAINRESCUE:
	case TEL_CALL_ECC_MANUAL_ECALL:
	case TEL_CALL_ECC_AUTO_ECALL:
		return TRUE;
	}

	return FALSE;
}

/* To be moved inside libtcore */
static gboolean
_check_answer_type(TelCallAnswerType type)
{
	switch (type) {
	case TEL_CALL_ANSWER_ACCEPT:
	case TEL_CALL_ANSWER_REJECT:
	case TEL_CALL_ANSWER_REPLACE:
	case TEL_CALL_ANSWER_HOLD_AND_ACCEPT:
		return TRUE;
	}

	return FALSE;
}

/* To be moved inside libtcore */
static gboolean
_check_end_type(TelCallAnswerType type)
{
	switch (type) {
	case TEL_CALL_END:
	case TEL_CALL_END_ALL:
	case TEL_CALL_END_ACTIVE_ALL:
	case TEL_CALL_END_HOLD_ALL:
		return TRUE;
	}

	return FALSE;
}

static void __add_call_status_to_builder(CallObject *co, GVariantBuilder *variant_builder)
{
	guint call_id;
	TelCallType call_type;
	TelCallState call_state;
	gboolean mo_call;
	gboolean mpty;
	TelCallCliValidity cli_validity;
	gchar num[TEL_CALL_CALLING_NUMBER_LEN_MAX + 1] = {0, };
	TelCallCniValidity cni_validity;
	gchar name[TEL_CALL_CALLING_NAME_LEN_MAX + 1] = {0, };
	gboolean forward;
	TelCallActiveLine active_line;

	g_variant_builder_open(variant_builder, G_VARIANT_TYPE("a{sv}"));
	if (co != NULL) {
		  /*
		   * Ignore return values of tcore_call_object_get/set_xxx()
		   * as in this case return value is always TRUE.
		   */
		tcore_call_object_get_id(co, &call_id);
		g_variant_builder_add(variant_builder, "{sv}",
			"call_id", g_variant_new_uint32(call_id));

		tcore_call_object_get_call_type(co, &call_type);
		g_variant_builder_add(variant_builder, "{sv}",
			"call_type", g_variant_new_int32(call_type));

		tcore_call_object_get_direction(co, &mo_call);
		g_variant_builder_add(variant_builder, "{sv}",
			"mo_call", g_variant_new_boolean(mo_call));

		tcore_call_object_get_state(co, &call_state);
		g_variant_builder_add(variant_builder, "{sv}",
			"call_state", g_variant_new_int32(call_state));

		tcore_call_object_get_multiparty_state(co, &mpty);
		g_variant_builder_add(variant_builder, "{sv}",
			"mpty", g_variant_new_boolean(mpty));

		tcore_call_object_get_active_line(co, &active_line);
		g_variant_builder_add(variant_builder, "{sv}",
			"active_line", g_variant_new_int32(active_line));

		tcore_call_object_get_cli_validity(co, &cli_validity);
		g_variant_builder_add(variant_builder, "{sv}",
			"cli_validity", g_variant_new_int32(cli_validity));

		tcore_call_object_get_number(co, num);
		g_variant_builder_add(variant_builder, "{sv}",
			"num", g_variant_new_string(num));
		/* the following data will be valid only in case of MT call */
		if (mo_call == FALSE) {
		tcore_call_object_get_cni_validity(co, &cni_validity);
			g_variant_builder_add(variant_builder, "{sv}",
				"cni_validity", g_variant_new_int32(cni_validity));

			tcore_call_object_get_name(co, name);
			g_variant_builder_add(variant_builder, "{sv}",
				"name", g_variant_new_string(name));

			tcore_call_object_get_mt_forward(co, &forward);
			g_variant_builder_add(variant_builder, "{sv}",
				"forward", g_variant_new_boolean(forward));
		}
	}
	g_variant_builder_close(variant_builder);
}

static void
on_response_dtapi_call_dial(gint result,
		const void *data,
		void *cb_data)
{
	DbusRespCbData *rsp_cb_data = cb_data;

	tcore_check_return_assert(rsp_cb_data != NULL);
	dbg("Call dial response result: [%d]", result);
	telephony_call_complete_dial(rsp_cb_data->interface_object, rsp_cb_data->invocation, result);
	tcore_free(rsp_cb_data);
}

static gboolean
dtapi_call_dial(TelephonyCall *call,
		GDBusMethodInvocation *invocation,
		gint call_type,
		gint ecc,
		gchar* call_number,
		gpointer user_data)
{
	DbusRespCbData *rsp_cb_data = NULL;
	TelCallDial req;
	TcorePlugin *plugin = user_data;
	TelReturn result;

	if (dtapi_check_access_control(invocation, AC_CALL, "x") == FALSE)
		return TRUE;

	 if ((_check_call_type(call_type) == FALSE)
	 		|| ((call_type == TEL_CALL_TYPE_E911)
	 		&& (_check_call_ecc(ecc) == FALSE))
	 		|| ((call_type != TEL_CALL_TYPE_E911)
	 		&& (call_number == NULL || strlen(call_number) == 0
	 		|| strlen(call_number) > TEL_CALL_CALLING_NUMBER_LEN_MAX))) {

		dtapi_return_error(invocation, G_DBUS_ERROR_FAILED, "Invalid parameter");
		return TRUE;
	}

	req.call_type = call_type;
	req.ecc = ecc;
	memcpy( req.number, call_number, strlen(call_number));
	req.number[strlen(call_number)] = '\0';

	rsp_cb_data = dtapi_create_resp_cb_data(call, invocation, NULL, 0);

	result = tcore_plugin_dispatch_request( plugin, TRUE, TCORE_COMMAND_CALL_DIAL,
					&req, sizeof(TelCallDial), on_response_dtapi_call_dial, rsp_cb_data);
	if (result != TEL_RETURN_SUCCESS) {
		dtapi_return_error(invocation, G_DBUS_ERROR_FAILED, "Dispatch failed");
		tcore_free(rsp_cb_data);
	}

	return TRUE;
}

static void
on_response_dtapi_call_answer(gint result,
		const void *data,
		void *cb_data)
{
	DbusRespCbData *rsp_cb_data = cb_data;

	tcore_check_return_assert(rsp_cb_data != NULL);
	dbg("Call answer response result: [%d]", result);
	telephony_call_complete_answer(rsp_cb_data->interface_object, rsp_cb_data->invocation, result);
	tcore_free(rsp_cb_data);
}

static gboolean
dtapi_call_answer(TelephonyCall *call,
		GDBusMethodInvocation *invocation,
		gint answer_type,
		gpointer user_data)
{
	DbusRespCbData *rsp_cb_data = NULL;
	TcorePlugin *plugin = user_data;
	TelReturn result;

	if (dtapi_check_access_control(invocation, AC_CALL, "x") == FALSE)
		return TRUE;

	if (!_check_answer_type(answer_type)) {
		dtapi_return_error(invocation, G_DBUS_ERROR_FAILED, "Invalid answer type");
		return TRUE;
	}

	rsp_cb_data = dtapi_create_resp_cb_data(call, invocation, NULL, 0);

	result = tcore_plugin_dispatch_request( plugin, TRUE, TCORE_COMMAND_CALL_ANSWER,
					&answer_type, sizeof(gint), on_response_dtapi_call_answer, rsp_cb_data);
	if (result != TEL_RETURN_SUCCESS) {
		dtapi_return_error(invocation, G_DBUS_ERROR_FAILED, "Dispatch failed");
		tcore_free(rsp_cb_data);
	}

	return TRUE;
}

static void
on_response_dtapi_call_end(gint result,
		const void *data,
		void *cb_data)
{
	DbusRespCbData *rsp_cb_data = cb_data;

	tcore_check_return_assert(rsp_cb_data != NULL);
	dbg("Call end response result: [%d]", result);
	telephony_call_complete_end(rsp_cb_data->interface_object, rsp_cb_data->invocation, result);
	tcore_free(rsp_cb_data);
}

static gboolean
dtapi_call_end(TelephonyCall *call,
		GDBusMethodInvocation *invocation,
		guint call_id,
		gint end_type,
		gpointer user_data)
{
	DbusRespCbData *rsp_cb_data = NULL;
	TelCallEnd req;
	TcorePlugin *plugin = user_data;
	TelReturn result;

	if (dtapi_check_access_control(invocation, AC_CALL, "x") == FALSE)
		return TRUE;

	if (!_check_end_type(end_type)) {
		dtapi_return_error(invocation, G_DBUS_ERROR_FAILED, "Invalid parameter");
		return TRUE;
	}

	req.call_id= call_id;
	req.end_type= end_type;
	rsp_cb_data = dtapi_create_resp_cb_data(call, invocation, NULL, 0);

	result = tcore_plugin_dispatch_request( plugin, TRUE, TCORE_COMMAND_CALL_END,
					&req, sizeof(TelCallEnd), on_response_dtapi_call_end, rsp_cb_data);
	if (result != TEL_RETURN_SUCCESS) {
		dtapi_return_error(invocation, G_DBUS_ERROR_FAILED, "Dispatch failed");
		tcore_free(rsp_cb_data);
	}

	return TRUE;
}

static void
on_response_dtapi_call_send_dtmf(gint result,
		const void *data,
		void *cb_data)
{
	DbusRespCbData *rsp_cb_data = cb_data;

	tcore_check_return_assert(rsp_cb_data != NULL);
	dbg("Call send dtmf response result: [%d]", result);
	telephony_call_complete_send_dtmf(rsp_cb_data->interface_object, rsp_cb_data->invocation, result);
	tcore_free(rsp_cb_data);
}

static gboolean
dtapi_call_send_dtmf(TelephonyCall *call,
		GDBusMethodInvocation *invocation,
		gchar *dtmf_str,
		gpointer user_data)
{
	DbusRespCbData *rsp_cb_data = NULL;
	TcorePlugin *plugin = user_data;
	TelReturn result;

	if (dtapi_check_access_control(invocation, AC_CALL, "x") == FALSE)
		return TRUE;

	if ((dtmf_str == NULL) || (strlen(dtmf_str) == 0)) {
		dtapi_return_error(invocation, G_DBUS_ERROR_FAILED, "Invalid parameter");
		return TRUE;
	}

	rsp_cb_data = dtapi_create_resp_cb_data(call, invocation, NULL, 0);

	result = tcore_plugin_dispatch_request( plugin, TRUE, TCORE_COMMAND_CALL_SEND_DTMF,
				dtmf_str, strlen(dtmf_str) + 1, on_response_dtapi_call_send_dtmf, rsp_cb_data);
	if (result != TEL_RETURN_SUCCESS) {
		dtapi_return_error(invocation, G_DBUS_ERROR_FAILED, "Dispatch failed");
		tcore_free(rsp_cb_data);
	}

	return TRUE;
}

static void
on_response_dtapi_call_hold(gint result,
		const void *data,
		void *cb_data)
{
	DbusRespCbData *rsp_cb_data = cb_data;

	tcore_check_return_assert(rsp_cb_data != NULL);
	dbg("Call hold response result: [%d]", result);
	telephony_call_complete_hold(rsp_cb_data->interface_object, rsp_cb_data->invocation, result);
	tcore_free(rsp_cb_data);
}

static gboolean
dtapi_call_hold(TelephonyCall *call,
		GDBusMethodInvocation *invocation,
		gpointer user_data)
{
	DbusRespCbData *rsp_cb_data = NULL;
	TcorePlugin *plugin = user_data;
	TelReturn result;

	if (dtapi_check_access_control(invocation, AC_CALL, "x") == FALSE)
		return TRUE;

	rsp_cb_data = dtapi_create_resp_cb_data(call, invocation, NULL, 0);

	result = tcore_plugin_dispatch_request(plugin, TRUE, TCORE_COMMAND_CALL_HOLD, NULL,
					0, on_response_dtapi_call_hold, rsp_cb_data);
	if (result != TEL_RETURN_SUCCESS) {
		dtapi_return_error(invocation, G_DBUS_ERROR_FAILED, "Dispatch failed");
		tcore_free(rsp_cb_data);
	}

	return TRUE;
}

static void
on_response_dtapi_call_active(gint result,
		const void *data,
		void *cb_data)
{
	DbusRespCbData *rsp_cb_data = cb_data;

	tcore_check_return_assert(rsp_cb_data != NULL);
	dbg("Call active response result: [%d]", result);
	telephony_call_complete_active(rsp_cb_data->interface_object, rsp_cb_data->invocation, result);
	tcore_free(rsp_cb_data);
}

static gboolean
dtapi_call_active(TelephonyCall *call,
		GDBusMethodInvocation *invocation,
		gpointer user_data)
{
	DbusRespCbData *rsp_cb_data = NULL;
	TcorePlugin *plugin = user_data;
	TelReturn result;

	if (dtapi_check_access_control(invocation, AC_CALL, "x") == FALSE)
		return TRUE;

	rsp_cb_data = dtapi_create_resp_cb_data(call, invocation, NULL, 0);

	result = tcore_plugin_dispatch_request( plugin, TRUE, TCORE_COMMAND_CALL_ACTIVE, NULL, 0,
					on_response_dtapi_call_active, rsp_cb_data);
	if (result != TEL_RETURN_SUCCESS) {
		dtapi_return_error(invocation, G_DBUS_ERROR_FAILED, "Dispatch failed");
		tcore_free(rsp_cb_data);
	}

	return TRUE;
}

static void
on_response_dtapi_call_swap(gint result,
		const void *data,
		void *cb_data)
{
	DbusRespCbData *rsp_cb_data = cb_data;

	tcore_check_return_assert(rsp_cb_data != NULL);
	dbg("Call swap response result: [%d]", result);
	telephony_call_complete_swap(rsp_cb_data->interface_object, rsp_cb_data->invocation, result);
	tcore_free(rsp_cb_data);
}

static gboolean
dtapi_call_swap(TelephonyCall *call,
		GDBusMethodInvocation *invocation,
		gpointer user_data)
{
	DbusRespCbData *rsp_cb_data = NULL;
	TcorePlugin *plugin = user_data;
	TelReturn result;

	if (dtapi_check_access_control(invocation, AC_CALL, "x") == FALSE)
		return TRUE;

	rsp_cb_data = dtapi_create_resp_cb_data(call, invocation, NULL, 0);

	result = tcore_plugin_dispatch_request( plugin, TRUE, TCORE_COMMAND_CALL_SWAP, NULL,
					0, on_response_dtapi_call_swap, rsp_cb_data);
	if (result != TEL_RETURN_SUCCESS) {
		dtapi_return_error(invocation, G_DBUS_ERROR_FAILED, "Dispatch failed");
		tcore_free(rsp_cb_data);
	}

	return TRUE;
}

static void
on_response_dtapi_call_join(gint result,
		const void *data,
		void *cb_data)
{
	DbusRespCbData *rsp_cb_data = cb_data;

	tcore_check_return_assert(rsp_cb_data != NULL);
	dbg("Call join response result: [%d]", result);
	telephony_call_complete_join(rsp_cb_data->interface_object, rsp_cb_data->invocation, result);
	tcore_free(rsp_cb_data);
}

static gboolean
dtapi_call_join(TelephonyCall *call,
		GDBusMethodInvocation *invocation,
		gpointer user_data)
{
	DbusRespCbData *rsp_cb_data = NULL;
	TcorePlugin *plugin = user_data;
	TelReturn result;

	if (dtapi_check_access_control(invocation, AC_CALL, "x") == FALSE)
		return TRUE;

	rsp_cb_data = dtapi_create_resp_cb_data(call, invocation, NULL, 0);

	result = tcore_plugin_dispatch_request( plugin, TRUE, TCORE_COMMAND_CALL_JOIN,
					NULL, 0, on_response_dtapi_call_join, rsp_cb_data);
	if (result != TEL_RETURN_SUCCESS) {
		dtapi_return_error(invocation, G_DBUS_ERROR_FAILED, "Dispatch failed");
		tcore_free(rsp_cb_data);
	}

	return TRUE;
}

static void
on_response_dtapi_call_split(gint result,
		const void *data,
		void *cb_data)
{
	DbusRespCbData *rsp_cb_data = cb_data;

	tcore_check_return_assert(rsp_cb_data != NULL);
	dbg("Call split response result: [%d]", result);
	telephony_call_complete_split(rsp_cb_data->interface_object, rsp_cb_data->invocation, result);
	tcore_free(rsp_cb_data);
}

static gboolean
dtapi_call_split(TelephonyCall *call,
		GDBusMethodInvocation *invocation,
		guint call_id,
		gpointer user_data)
{
	DbusRespCbData *rsp_cb_data = NULL;
	TcorePlugin *plugin = user_data;
	TelReturn result;

	if (dtapi_check_access_control(invocation, AC_CALL, "x") == FALSE)
		return TRUE;

	rsp_cb_data = dtapi_create_resp_cb_data(call, invocation, NULL, 0);

	result = tcore_plugin_dispatch_request(plugin, TRUE, TCORE_COMMAND_CALL_SPLIT, &call_id,
					sizeof(guint), on_response_dtapi_call_split, rsp_cb_data);
	if (result != TEL_RETURN_SUCCESS) {
		dtapi_return_error(invocation, G_DBUS_ERROR_FAILED, "Dispatch failed");
		tcore_free(rsp_cb_data);
	}

	return TRUE;
}

static void
on_response_dtapi_call_transfer(gint result,
		const void *data,
		void *cb_data)
{
	DbusRespCbData *rsp_cb_data = cb_data;

	tcore_check_return_assert(rsp_cb_data != NULL);
	dbg("Call transfer response result: [%d]", result);
	telephony_call_complete_transfer(rsp_cb_data->interface_object, rsp_cb_data->invocation, result);
	tcore_free(rsp_cb_data);
}

static gboolean
dtapi_call_transfer(TelephonyCall *call,
		GDBusMethodInvocation *invocation,
		gpointer user_data)
{
	DbusRespCbData *rsp_cb_data = NULL;
	TcorePlugin *plugin = user_data;
	TelReturn result;

	if (dtapi_check_access_control(invocation, AC_CALL, "x") == FALSE)
		return TRUE;

	rsp_cb_data = dtapi_create_resp_cb_data(call, invocation, NULL, 0);

	result = tcore_plugin_dispatch_request( plugin, TRUE, TCORE_COMMAND_CALL_TRANSFER,
				NULL, 0, on_response_dtapi_call_transfer, rsp_cb_data);
	if (result != TEL_RETURN_SUCCESS) {
		dtapi_return_error(invocation, G_DBUS_ERROR_FAILED, "Dispatch failed");
		tcore_free(rsp_cb_data);
	}

	return TRUE;
}

static void
on_response_dtapi_call_deflect(gint result,
		const void *data,
		void *cb_data)
{
	DbusRespCbData *rsp_cb_data = cb_data;

	tcore_check_return_assert(rsp_cb_data != NULL);
	dbg("Call deflect response result: [%d]", result);
	telephony_call_complete_deflect(rsp_cb_data->interface_object, rsp_cb_data->invocation, result);
	tcore_free(rsp_cb_data);
}

static gboolean
dtapi_call_deflect(TelephonyCall *call,
		GDBusMethodInvocation *invocation,
		gchar *deflect_to,
		gpointer user_data)
{
	DbusRespCbData *rsp_cb_data = NULL;
	TcorePlugin *plugin = user_data;
	TelReturn result;

	if (dtapi_check_access_control(invocation, AC_CALL, "x") == FALSE)
		return TRUE;

	if ((deflect_to == NULL) || ((strlen(deflect_to) == 0)
		|| (strlen(deflect_to) > TEL_CALL_CALLING_NUMBER_LEN_MAX))) {
		dtapi_return_error(invocation, G_DBUS_ERROR_FAILED, "Invalid parameter");
		return TRUE;
	}

	rsp_cb_data = dtapi_create_resp_cb_data(call, invocation, NULL, 0);
	result = tcore_plugin_dispatch_request( plugin, TRUE, TCORE_COMMAND_CALL_DEFLECT,
				deflect_to, strlen(deflect_to) + 1, on_response_dtapi_call_deflect, rsp_cb_data);
	if (result != TEL_RETURN_SUCCESS) {
		dtapi_return_error(invocation, G_DBUS_ERROR_FAILED, "Dispatch failed");
		tcore_free(rsp_cb_data);
	}

	return TRUE;
}

static void
on_response_dtapi_call_set_active_line(gint result,
		const void *data,
		void *cb_data)
{
	DbusRespCbData *rsp_cb_data = cb_data;

	tcore_check_return_assert(rsp_cb_data != NULL);
	dbg("Call set active line response result: [%d]", result);
	telephony_call_complete_set_active_line(rsp_cb_data->interface_object, rsp_cb_data->invocation, result);
	tcore_free(rsp_cb_data);
}

static gboolean
dtapi_call_set_active_line(TelephonyCall *call,
		GDBusMethodInvocation *invocation,
		gint active_line,
		gpointer user_data)
{
	DbusRespCbData *rsp_cb_data = NULL;
	TcorePlugin *plugin = user_data;
	TelReturn result;

	if (dtapi_check_access_control(invocation, AC_CALL, "x") == FALSE)
		return TRUE;

	rsp_cb_data = dtapi_create_resp_cb_data(call, invocation, NULL, 0);

	result = tcore_plugin_dispatch_request( plugin, TRUE, TCORE_COMMAND_CALL_SET_ACTIVE_LINE,
				&active_line, sizeof(gint), on_response_dtapi_call_set_active_line, rsp_cb_data);
	if (result != TEL_RETURN_SUCCESS) {
		dtapi_return_error(invocation, G_DBUS_ERROR_FAILED, "Dispatch failed");
		tcore_free(rsp_cb_data);
	}

	return TRUE;
}

static void
on_response_dtapi_call_get_active_line(gint result,
		const void *data,
		void *cb_data)
{
	DbusRespCbData *rsp_cb_data = cb_data;

	tcore_check_return_assert(rsp_cb_data != NULL);
	dbg("Call get active line response result : [%d]", result);
	telephony_call_complete_get_active_line(rsp_cb_data->interface_object, rsp_cb_data->invocation,
		result, GPOINTER_TO_INT(data));
	tcore_free(rsp_cb_data);
}

static gboolean
dtapi_call_get_active_line(TelephonyCall *call,
		GDBusMethodInvocation *invocation,
		gpointer user_data)
{
	DbusRespCbData *rsp_cb_data = NULL;
	TcorePlugin *plugin = user_data;
	TelReturn result;

	if (dtapi_check_access_control(invocation, AC_CALL, "x") == FALSE)
		return TRUE;

	rsp_cb_data = dtapi_create_resp_cb_data(call, invocation, NULL, 0);

	result = tcore_plugin_dispatch_request( plugin, TRUE, TCORE_COMMAND_CALL_GET_ACTIVE_LINE, NULL,
				0, on_response_dtapi_call_get_active_line, rsp_cb_data);
	if (result != TEL_RETURN_SUCCESS) {
		dtapi_return_error(invocation, G_DBUS_ERROR_FAILED, "Dispatch failed");
		tcore_free(rsp_cb_data);
	}

	return TRUE;
}

static gboolean
dtapi_call_get_status(TelephonyCall *call,
		GDBusMethodInvocation *invocation,
		guint call_id,
		gpointer user_data)
{
	TcorePlugin *plugin = user_data;
	CoreObject *o = NULL;
	CallObject *co = NULL;
	TelCallType call_type;
	TelCallState call_state;
	gboolean mo_call;
	gboolean mpty;
	TelCallCliValidity cli_validity;
	gchar num[TEL_CALL_CALLING_NUMBER_LEN_MAX + 1] = {0, };
	TelCallCniValidity cni_validity;
	gchar name[TEL_CALL_CALLING_NAME_LEN_MAX + 1] = {0, };
	gboolean forward;
	TelCallActiveLine active_line;

	if (dtapi_check_access_control(invocation, AC_CALL, "r") == FALSE)
		return TRUE;

	o = tcore_plugin_ref_core_object(plugin, CORE_OBJECT_TYPE_CALL);
	if (!o) {
		dtapi_return_error(invocation, G_DBUS_ERROR_FAILED, "Invalid core object");
		return TRUE;
	}
	co = tcore_call_object_find_by_id(o, call_id);
	if (!co) {
		dtapi_return_error(invocation, G_DBUS_ERROR_FAILED, "No Call Object");
		return TRUE;
	}

	tcore_call_object_get_call_type(co, &call_type);
	tcore_call_object_get_direction(co, &mo_call);
	tcore_call_object_get_state(co, &call_state);
	tcore_call_object_get_multiparty_state(co, &mpty);
	tcore_call_object_get_active_line(co, &active_line);
	tcore_call_object_get_cli_validity(co, &cli_validity);
	tcore_call_object_get_number(co, num);

	/* the following data will be valid only in case of MT call */
	if(mo_call == FALSE){
		tcore_call_object_get_cni_validity(co, &cni_validity);
		tcore_call_object_get_name(co, name);
		tcore_call_object_get_mt_forward(co, &forward);
	}

	telephony_call_complete_get_status(call, invocation,
		TEL_RETURN_SUCCESS, call_id, call_type,
		call_state, mo_call, mpty,
		cli_validity, num, cni_validity, name,
		forward, active_line);

	return TRUE;
}

static gboolean
dtapi_call_get_status_all(TelephonyCall *call,
		GDBusMethodInvocation *invocation,
		gpointer user_data)
{
	TcorePlugin *plugin = user_data;
	GSList *list = NULL;
	CoreObject *co = NULL;
	GVariant *gv = NULL;
	GVariantBuilder b;
	TelCallState  state;
	guint count = 0;

	if (dtapi_check_access_control(invocation, AC_CALL, "r") == FALSE)
		return TRUE;

	/* OSP access this function already, can't modify this immediately */
	if (!plugin) {
		dtapi_return_error(invocation, G_DBUS_ERROR_FAILED, "Invalid plugin");
		return TRUE;
	}

	co = tcore_plugin_ref_core_object(plugin, CORE_OBJECT_TYPE_CALL);
	if (!co) {
		dtapi_return_error(invocation, G_DBUS_ERROR_FAILED, "Invalid core object");
		return TRUE;
	}

	g_variant_builder_init(&b, G_VARIANT_TYPE("aa{sv}"));
	for (state = TEL_CALL_STATE_IDLE; state <= TEL_CALL_STATE_WAITING; state++) {
		list = tcore_call_object_find_by_status(co, state);
		dbg("%d Calls are in [%d] state", g_slist_length(list), state);
		while (list) {
			/* Update builder */
			__add_call_status_to_builder((CallObject *)list->data, &b);

			list = g_slist_next( list );
			count++;
		}
	}
	gv = g_variant_builder_end(&b);
	dbg("Total Calls: [%d]", count);

	telephony_call_complete_get_status_all(call, invocation, TEL_RETURN_SUCCESS, count, gv);
	g_variant_unref(gv);

	return TRUE;
}

static void
on_response_dtapi_call_set_volume_info(gint result,
		const void *data,
		void *cb_data)
{
	DbusRespCbData *rsp_cb_data = cb_data;

	tcore_check_return_assert(rsp_cb_data != NULL);
	dbg("Call set volume info response result: [%d]", result);
	telephony_call_complete_set_volume_info(rsp_cb_data->interface_object, rsp_cb_data->invocation, result);
	tcore_free(rsp_cb_data);
}

static gboolean
dtapi_call_set_volume_info(TelephonyCall *call,
		GDBusMethodInvocation *invocation,
		gint device,
		guint volume,
		gpointer user_data)
{
	DbusRespCbData *rsp_cb_data = NULL;
	TelCallVolumeInfo req;
	TcorePlugin *plugin = user_data;
	TelReturn result;

	if (dtapi_check_access_control(invocation, AC_CALL, "x") == FALSE)
		return TRUE;

	memset(&req, 0x0, sizeof(TelCallVolumeInfo));
	req.device = device;
	req.volume = volume;
	rsp_cb_data = dtapi_create_resp_cb_data(call, invocation, NULL, 0);

	result = tcore_plugin_dispatch_request( plugin, TRUE, TCORE_COMMAND_CALL_SET_VOLUME_INFO,
				&req, sizeof(TelCallVolumeInfo), on_response_dtapi_call_set_volume_info, rsp_cb_data);
	if (result != TEL_RETURN_SUCCESS) {
		dtapi_return_error(invocation, G_DBUS_ERROR_FAILED, "Dispatch failed");
		tcore_free(rsp_cb_data);
	}

	return TRUE;
}

static void
on_response_dtapi_call_get_volume_info(gint result,
		const void *data,
		void *cb_data)
{
	DbusRespCbData *rsp_cb_data = cb_data;

	tcore_check_return_assert(rsp_cb_data != NULL);
	dbg("Call get volume info response result: [%d]", result);
	telephony_call_complete_get_volume_info(rsp_cb_data->interface_object, rsp_cb_data->invocation,
		result, GPOINTER_TO_UINT(data));
	tcore_free(rsp_cb_data);
}

static gboolean
dtapi_call_get_volume_info(TelephonyCall *call,
	GDBusMethodInvocation *invocation,
	gint device,
	gpointer user_data)
{
	DbusRespCbData *rsp_cb_data = NULL;
	TelCallSoundDevice req;
	TcorePlugin *plugin = user_data;
	TelReturn result;

	if (dtapi_check_access_control(invocation, AC_CALL, "x") == FALSE)
		return TRUE;

	req = device;
	rsp_cb_data = dtapi_create_resp_cb_data(call, invocation, NULL, 0);

	result = tcore_plugin_dispatch_request( plugin, TRUE, TCORE_COMMAND_CALL_GET_VOLUME_INFO,
				&req, sizeof(TelCallSoundDevice), on_response_dtapi_call_get_volume_info, rsp_cb_data);
	if (result != TEL_RETURN_SUCCESS) {
		dtapi_return_error(invocation, G_DBUS_ERROR_FAILED, "Dispatch failed");
		tcore_free(rsp_cb_data);
	}

	return TRUE;
}

static void
on_response_dtapi_call_set_sound_path(gint result,
		const void *data,
		void *cb_data)
{
	DbusRespCbData *rsp_cb_data = cb_data;

	tcore_check_return_assert(rsp_cb_data != NULL);
	dbg("Call set sound path response result: [%d]", result);
	telephony_call_complete_set_sound_path(rsp_cb_data->interface_object, rsp_cb_data->invocation, result);
	tcore_free(rsp_cb_data);
}

static gboolean
dtapi_call_set_sound_path(TelephonyCall *call,
		GDBusMethodInvocation *invocation,
		gint path,
		gboolean ex_volume,
		gpointer user_data)
{
	DbusRespCbData *rsp_cb_data = NULL;
	TelCallSoundPathInfo req;
	TcorePlugin *plugin = user_data;
	TelReturn result;

	if (dtapi_check_access_control(invocation, AC_CALL, "x") == FALSE)
		return TRUE;

	memset(&req, 0x0, sizeof(TelCallSoundPathInfo));
	req.path = path;
	req.ex_volume = ex_volume;

	rsp_cb_data = dtapi_create_resp_cb_data(call, invocation, NULL, 0);

	result = tcore_plugin_dispatch_request( plugin, TRUE, TCORE_COMMAND_CALL_SET_SOUND_PATH,
				&req, sizeof(TelCallSoundPathInfo), on_response_dtapi_call_set_sound_path, rsp_cb_data);
	if (result != TEL_RETURN_SUCCESS) {
		dtapi_return_error(invocation, G_DBUS_ERROR_FAILED, "Dispatch failed");
		tcore_free(rsp_cb_data);
	}

	return TRUE;
}

static void
on_response_dtapi_call_set_mute(gint result,
		const void *data,
		void *cb_data)
{
	DbusRespCbData *rsp_cb_data = cb_data;

	tcore_check_return_assert(rsp_cb_data != NULL);
	dbg("Call set mute response result: [%d]", result);
	telephony_call_complete_set_mute(rsp_cb_data->interface_object, rsp_cb_data->invocation, result);
	tcore_free(rsp_cb_data);
}

static gboolean
dtapi_call_set_mute(TelephonyCall *call,
		GDBusMethodInvocation *invocation,
		gboolean mute,
		gpointer user_data)
{
	DbusRespCbData *rsp_cb_data = NULL;
	TcorePlugin *plugin = user_data;
	TelReturn result;

	if (dtapi_check_access_control(invocation, AC_CALL, "x") == FALSE)
		return TRUE;

	rsp_cb_data = dtapi_create_resp_cb_data(call, invocation, NULL, 0);

	result = tcore_plugin_dispatch_request( plugin, TRUE, TCORE_COMMAND_CALL_SET_MUTE,
				&mute, sizeof(gboolean), on_response_dtapi_call_set_mute, rsp_cb_data);
	if (result != TEL_RETURN_SUCCESS) {
		dtapi_return_error(invocation, G_DBUS_ERROR_FAILED, "Dispatch failed");
		tcore_free(rsp_cb_data);
	}

	return TRUE;
}

static void
on_response_dtapi_call_get_mute_status(gint result,
		const void *data,
		void *cb_data)
{
	DbusRespCbData *rsp_cb_data = cb_data;

	tcore_check_return_assert(rsp_cb_data != NULL);
	dbg("Call get mute status response result: [%d]", result);
	telephony_call_complete_get_mute_status(rsp_cb_data->interface_object, rsp_cb_data->invocation, result, GPOINTER_TO_INT(data));
	tcore_free(rsp_cb_data);
}

static gboolean
dtapi_call_get_mute_status(TelephonyCall *call,
	GDBusMethodInvocation *invocation,
	gpointer user_data)
{
	DbusRespCbData *rsp_cb_data = NULL;
	TcorePlugin *plugin = user_data;
	TelReturn result;

	if (dtapi_check_access_control(invocation, AC_CALL, "x") == FALSE)
		return TRUE;

	rsp_cb_data = dtapi_create_resp_cb_data(call, invocation, NULL, 0);

	result = tcore_plugin_dispatch_request( plugin, TRUE, TCORE_COMMAND_CALL_GET_MUTE_STATUS,
				NULL, 0, on_response_dtapi_call_get_mute_status, rsp_cb_data);
	if (result != TEL_RETURN_SUCCESS) {
		dtapi_return_error(invocation, G_DBUS_ERROR_FAILED, "Dispatch failed");
		tcore_free(rsp_cb_data);
	}

	return TRUE;
}

static void
on_response_dtapi_call_set_sound_recording(gint result,
		const void *data,
		void *cb_data)
{
	DbusRespCbData *rsp_cb_data = cb_data;

	tcore_check_return_assert(rsp_cb_data != NULL);
	dbg("Call set sound recording response result: [%d]", result);
	telephony_call_complete_set_sound_recording(rsp_cb_data->interface_object, rsp_cb_data->invocation, result);
	tcore_free(rsp_cb_data);
}

static gboolean
dtapi_call_set_sound_recording(TelephonyCall *call,
		GDBusMethodInvocation *invocation,
		gint recording,
		gpointer user_data)
{
	DbusRespCbData *rsp_cb_data = NULL;
	TcorePlugin *plugin = user_data;
	TelReturn result;

	if (dtapi_check_access_control(invocation, AC_CALL, "x") == FALSE)
		return TRUE;

	rsp_cb_data = dtapi_create_resp_cb_data(call, invocation, NULL, 0);

	result = tcore_plugin_dispatch_request( plugin, TRUE, TCORE_COMMAND_CALL_SET_SOUND_RECORDING,
				&recording, sizeof(gint), on_response_dtapi_call_set_sound_recording, rsp_cb_data);
	if (result != TEL_RETURN_SUCCESS) {
		dtapi_return_error(invocation, G_DBUS_ERROR_FAILED, "Dispatch failed");
		tcore_free(rsp_cb_data);
	}

	return TRUE;
}

static void
on_response_dtapi_call_set_sound_equalization(gint result,
		const void *data,
		void *cb_data)
{
	DbusRespCbData *rsp_cb_data = cb_data;

	tcore_check_return_assert(rsp_cb_data != NULL);
	dbg("Call set sound equalization response result: [%d]", result);
	telephony_call_complete_set_sound_equalization(rsp_cb_data->interface_object, rsp_cb_data->invocation, result);
	tcore_free(rsp_cb_data);
}

static gboolean
dtapi_call_set_sound_equalization(TelephonyCall *call,
		GDBusMethodInvocation *invocation,
		gint equalization_mode,
		gint direction,
		GVariant *param,
		gpointer user_data)
{
	GVariantIter *iter = NULL;
	guint i  =  0;
	DbusRespCbData *rsp_cb_data = NULL;
	TelCallSoundEqualization req;
	TcorePlugin *plugin = user_data;
	TelReturn result;

	if (dtapi_check_access_control(invocation, AC_CALL, "x") == FALSE)
		return TRUE;

	tcore_check_return_value_assert(NULL != param, TRUE);
	memset(&req, 0x0,sizeof(TelCallSoundEqualization));
	req.mode = equalization_mode;
	req.direction = direction;
	g_variant_get(param,"aq",&iter);
	while(g_variant_iter_loop(iter,"q", &(req.parameter[i]))) {
		i++;
	}

	rsp_cb_data = dtapi_create_resp_cb_data(call, invocation, NULL, 0);

	result = tcore_plugin_dispatch_request( plugin, TRUE, TCORE_COMMAND_CALL_SET_SOUND_EQUALIZATION,
				&req, sizeof(TelCallSoundEqualization), on_response_dtapi_call_set_sound_equalization,
				rsp_cb_data);
	if (result != TEL_RETURN_SUCCESS) {
		dtapi_return_error(invocation, G_DBUS_ERROR_FAILED, "Dispatch failed");
		tcore_free(rsp_cb_data);
	}

	return TRUE;
}

gboolean
dtapi_setup_call_interface(TelephonyObjectSkeleton *object,
		TcorePlugin *plugin)
{
	TelephonyCall *call = telephony_call_skeleton_new();
	tcore_check_return_value_assert(call != NULL, FALSE);

	telephony_object_skeleton_set_call(object, call);
	g_object_unref(call);

	dbg("call = %p", call);

	g_signal_connect(call, "handle-dial",
		G_CALLBACK(dtapi_call_dial), plugin);

	g_signal_connect(call, "handle-answer",
		G_CALLBACK(dtapi_call_answer), plugin);

	g_signal_connect(call, "handle-end",
		G_CALLBACK(dtapi_call_end), plugin);

	g_signal_connect(call, "handle-send-dtmf",
		G_CALLBACK(dtapi_call_send_dtmf), plugin);

	g_signal_connect(call, "handle-active",
		G_CALLBACK(dtapi_call_active), plugin);

	g_signal_connect(call, "handle-hold",
		G_CALLBACK(dtapi_call_hold), plugin);

	g_signal_connect(call, "handle-swap",
		G_CALLBACK(dtapi_call_swap), plugin);

	g_signal_connect(call, "handle-join",
		G_CALLBACK(dtapi_call_join), plugin);

	g_signal_connect(call, "handle-split",
		G_CALLBACK(dtapi_call_split), plugin);

	g_signal_connect(call, "handle-transfer",
		G_CALLBACK(dtapi_call_transfer), plugin);

	g_signal_connect(call, "handle-deflect",
		G_CALLBACK(dtapi_call_deflect), plugin);

	g_signal_connect(call, "handle-set-active-line",
		G_CALLBACK(dtapi_call_set_active_line), plugin);

	g_signal_connect(call, "handle-get-active-line",
		G_CALLBACK(dtapi_call_get_active_line), plugin);

	g_signal_connect(call, "handle-get-status",
		G_CALLBACK(dtapi_call_get_status), plugin);

	g_signal_connect(call, "handle-get-status-all",
		G_CALLBACK(dtapi_call_get_status_all), plugin);

	g_signal_connect(call, "handle-set-sound-path",
		G_CALLBACK(dtapi_call_set_sound_path), plugin);

	g_signal_connect(call, "handle-get-volume-info",
		G_CALLBACK(dtapi_call_get_volume_info), plugin);

	g_signal_connect(call, "handle-set-volume-info",
		G_CALLBACK(dtapi_call_set_volume_info), plugin);

	g_signal_connect(call, "handle-set-mute",
		G_CALLBACK(dtapi_call_set_mute), plugin);

	g_signal_connect(call, "handle-get-mute-status",
		G_CALLBACK(dtapi_call_get_mute_status), plugin);

	g_signal_connect(call, "handle-set-sound-recording",
		G_CALLBACK(dtapi_call_set_sound_recording), plugin);

	g_signal_connect(call, "handle-set-sound-equalization",
		G_CALLBACK(dtapi_call_set_sound_equalization), plugin);

	return TRUE;
}

gboolean
dtapi_handle_call_notification(TelephonyObjectSkeleton *object,
		TcorePlugin *plugin,
		TcoreNotification command,
		guint data_len,
		const void *data)
{
	TelephonyCall *call;

	tcore_check_return_value_assert(object != NULL, FALSE);
	tcore_check_return_value_assert(plugin != NULL, FALSE);

	call = telephony_object_peek_call(TELEPHONY_OBJECT(object));
	tcore_check_return_value_assert(call != NULL, FALSE);

	dbg("Call: [%p] Command: [0x%x]", call, command);

	switch (command) {
	case TCORE_NOTIFICATION_CALL_STATUS_IDLE: {
		const TelCallStatusIdleNoti *idle = data;
		tcore_check_return_value_assert(idle != NULL, FALSE);

		dbg("[Notification] Call status idle - Call Id:[%d] Idle cause: [%d]",
				idle->call_id, idle->cause);

		telephony_call_emit_voice_call_status_idle(call, idle->call_id, idle->cause);
	}
	break;

	case TCORE_NOTIFICATION_CALL_STATUS_ACTIVE: {
		const guint *call_id = data;
		tcore_check_return_value_assert(call_id != NULL, FALSE);

		dbg("[Notification] Call status active - Call Id:[%d] ", *call_id);

		telephony_call_emit_voice_call_status_active(call, *call_id);
	}
	break;

	case TCORE_NOTIFICATION_CALL_STATUS_HELD: {
		const guint *call_id = data;
		tcore_check_return_value_assert(call_id != NULL, FALSE);

		dbg("[Notification] Call status held - Call Id:[%d] ", *call_id);

		telephony_call_emit_voice_call_status_held(call, *call_id);
	}
	break;

	case TCORE_NOTIFICATION_CALL_STATUS_DIALING: {
		const guint *call_id = data;
		tcore_check_return_value_assert(call_id != NULL, FALSE);

		dbg("[Notification] Call status dialing - Call Id:[%d] ", *call_id);

		telephony_call_emit_voice_call_status_dialing(call, *call_id);
	}
	break;

	case TCORE_NOTIFICATION_CALL_STATUS_ALERT: {
		const guint *call_id = data;
		tcore_check_return_value_assert(call_id != NULL, FALSE);

		dbg("[Notification] Call status alert - Call Id:[%d] ", *call_id);

		telephony_call_emit_voice_call_status_alert(call, *call_id);
	}
	break;

	case TCORE_NOTIFICATION_CALL_STATUS_INCOMING: {
		const TelCallIncomingInfo *incoming_call_info = (TelCallIncomingInfo *)data;
		tcore_check_return_value_assert(incoming_call_info != NULL, FALSE);

		dbg("[Notification] Call Status Incoming - Call Id:[%d] ", incoming_call_info->call_id);

		telephony_call_emit_voice_call_status_incoming(call, incoming_call_info->call_id);

		_launch_voice_call(incoming_call_info);
	}
	break;

	case TCORE_NOTIFICATION_VIDEO_CALL_STATUS_IDLE: {
		const TelCallStatusIdleNoti *idle = data;
		tcore_check_return_value_assert(idle != NULL, FALSE);

		dbg("[Notification] Video call status idle notification - Call Id:[%d] Idle cause: [%d]",
			idle->call_id, idle->cause);

		telephony_call_emit_video_call_status_idle(call, idle->call_id, idle->cause);
	}
	break;

	case TCORE_NOTIFICATION_VIDEO_CALL_STATUS_ACTIVE: {
		const guint *call_id = data;
		tcore_check_return_value_assert(call_id != NULL, FALSE);

		dbg("[Notification] Video call status active notification - Call Id:[%d] ", *call_id);

		telephony_call_emit_video_call_status_active(call, *call_id);
	}
	break;

	case TCORE_NOTIFICATION_VIDEO_CALL_STATUS_DIALING: {
		const guint *call_id = data;
		tcore_check_return_value_assert(call_id != NULL, FALSE);

		dbg("[Notification] Video call status dialing - Call Id:[%d] ", *call_id);

		telephony_call_emit_video_call_status_dialing(call, *call_id);
	}
	break;

	case TCORE_NOTIFICATION_VIDEO_CALL_STATUS_ALERT: {
		const guint *call_id = data;
		tcore_check_return_value_assert(call_id != NULL, FALSE);

		dbg("[Notification] Video call status alert -Call Id:[%d] ", *call_id);

		telephony_call_emit_video_call_status_alert(call, *call_id);
	}
	break;

	case TCORE_NOTIFICATION_VIDEO_CALL_STATUS_INCOMING: {
		const TelCallIncomingInfo *incoming_call_info = (TelCallIncomingInfo *)data;
		tcore_check_return_value_assert(incoming_call_info != NULL, FALSE);

		dbg("[Notification] Video call status incoming - Call Id:[%d] ",
			incoming_call_info->call_id);

		telephony_call_emit_video_call_status_incoming(call, incoming_call_info->call_id);
		_launch_video_call(incoming_call_info);
	}
	break;

	case TCORE_NOTIFICATION_CALL_INFO_MO_WAITING: {
		dbg("[Notification] MO Waiting call info");

		telephony_call_emit_mo_waiting(call);
	}
	break;

	case TCORE_NOTIFICATION_CALL_INFO_MO_FORWARDED: {
		dbg("[Notification] MO Forwarded call info");

		telephony_call_emit_mo_forwarded(call);
	}
	break;

	case TCORE_NOTIFICATION_CALL_INFO_MO_BARRED_INCOMING: {
		dbg("[Notification] MO Barred incoming call info");

		telephony_call_emit_mo_barred_incoming(call);
	}
	break;

	case TCORE_NOTIFICATION_CALL_INFO_MO_BARRED_OUTGOING: {
		dbg("[Notification] MO Barred outgoing call info");

		telephony_call_emit_mo_barred_outgoing(call);
	}
	break;

	case TCORE_NOTIFICATION_CALL_INFO_MO_FORWARD_UNCONDITIONAL: {
		dbg("[Notification] MO Forward unconditional call info");

		telephony_call_emit_mo_forward_unconditional(call);
	}
	break;

	case TCORE_NOTIFICATION_CALL_INFO_MO_FORWARD_CONDITIONAL: {
		dbg("[Notification] MO Forward conditional call info");

		telephony_call_emit_mo_forward_conditional(call);
	}
	break;

	case TCORE_NOTIFICATION_CALL_INFO_MO_DEFLECTED: {
		dbg("[Notification] MO Deflected call");

		telephony_call_emit_mo_deflected(call);
	}
	break;

	case TCORE_NOTIFICATION_CALL_INFO_MT_FORWARDED: {
		dbg("[Notification] MT Forwarded call");

		telephony_call_emit_mt_forwarded(call);
	}
	break;

	case TCORE_NOTIFICATION_CALL_INFO_MT_DEFLECTED: {
		dbg("[Notification] MT Deflected call");

		telephony_call_emit_mt_deflected(call);
	}
	break;

	case TCORE_NOTIFICATION_CALL_INFO_TRANSFERED: {
		dbg("[Notification] Transfered call");

		telephony_call_emit_transfered(call);
	}
	break;

	case TCORE_NOTIFICATION_CALL_INFO_HELD: {
		dbg("[Notification] Call held info");

		telephony_call_emit_call_held(call);
	}
	break;

	case TCORE_NOTIFICATION_CALL_INFO_ACTIVE: {
		dbg("[Notification] Call active info");

		telephony_call_emit_call_active(call);
	}
	break;

	case TCORE_NOTIFICATION_CALL_INFO_JOINED: {
		dbg("[Notification] Call joined info");

		telephony_call_emit_call_joined(call);
	}
	break;

	case TCORE_NOTIFICATION_CALL_SOUND_RINGBACK_TONE: {
		const gint *status = data;
		tcore_check_return_value_assert(status != NULL, FALSE);

		dbg("[Notification] Call sound ringback tone notification - Status:[%d] ", *status);

		telephony_call_emit_call_sound_ringback_tone(call, *status);
	}
	break;

	case TCORE_NOTIFICATION_CALL_SOUND_WBAMR: {
		const gint *status = (gint *)data;
		tcore_check_return_value_assert(status != NULL, FALSE);

		dbg("[Notification] Call sound WBAMR notification - Status:[%d] ", *status);

		telephony_call_emit_call_sound_wbamr(call, *status);
	}
	break;

	case TCORE_NOTIFICATION_CALL_SOUND_EQUALIZATION: {
		const TelCallSoundEqualizationNoti *sound_eq = data;
		tcore_check_return_value_assert(sound_eq != NULL, FALSE);

		dbg("[Notification] Call sound equalization notification - Mode:[%d] Direction:[%d]",
			sound_eq->mode, sound_eq->direction);

		telephony_call_emit_call_sound_equalization(call, sound_eq->mode, sound_eq->direction);
	}
	break;

	case TCORE_NOTIFICATION_CALL_SOUND_CLOCK_STATUS: {
		const gboolean *clock_status = data;
		tcore_check_return_value_assert(clock_status != NULL, FALSE);

		dbg("[Notification] Call sound clock status notification - Clock status:[%d]",
			*clock_status);

		telephony_call_emit_call_sound_clock_status(call, *clock_status);
	}
	break;

	default:
		err("Unsupported Command");
	break;
	}

	return TRUE;
}
