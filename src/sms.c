/*
 * tel-plugin-socket-communicator
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
#include <gio/gio.h>

#include <tcore.h>
#include <server.h>
#include <plugin.h>
#include <hal.h>
#include <communicator.h>
#include <storage.h>
#include <queue.h>
#include <user_request.h>
#include <co_sms.h>

#include "generated-code.h"
#include "common.h"

static gboolean
on_sms_send_msg(TelephonySms *sms, GDBusMethodInvocation *invocation,
	gint format,
	GVariant *sca,
	gint tpdu_length,
	GVariant *tpdu_data,
	gint moreMsg,
	gpointer user_data)
{
	struct treq_sms_send_msg sendMsg;
	struct custom_data *ctx = user_data;
	UserRequest *ur = NULL;
	TReturn	ret = TCORE_RETURN_SUCCESS;
	cynara *p_cynara = (ctx) ? ctx->p_cynara : NULL;

	int i = 0;
	GVariantIter *iter = 0;
	GVariant *inner_gv = 0;

	if (!check_access_control(p_cynara, invocation, AC_SMS, "x"))
		return TRUE;

	memset(&sendMsg, 0 , sizeof(struct treq_sms_send_msg));

	inner_gv = g_variant_get_variant(sca);
	g_variant_get(inner_gv, "ay", &iter);
	while (g_variant_iter_loop(iter, "y", &sendMsg.msgDataPackage.sca[i])) {
		i++;
		if (i >= SMS_SMSP_ADDRESS_LEN)
			break;
	}

	sendMsg.msgDataPackage.msgLength = tpdu_length;

	i = 0;
	inner_gv = g_variant_get_variant(tpdu_data);
	g_variant_get(inner_gv, "ay", &iter);
	while (g_variant_iter_loop(iter, "y", &sendMsg.msgDataPackage.tpduData[i])) {
		i++;
		if (i >= SMS_SMDATA_SIZE_MAX + 1)
			break;
	}
	g_variant_iter_free(iter);
	g_variant_unref(inner_gv);

	sendMsg.msgDataPackage.format = format;
	sendMsg.more = moreMsg;

	ur = MAKE_UR(ctx, sms, invocation);
	tcore_user_request_set_data(ur, sizeof(struct treq_sms_send_msg), &sendMsg);

	if (SMS_NETTYPE_3GPP == format) {
		tcore_user_request_set_command(ur, TREQ_SMS_SEND_UMTS_MSG);
	} else if (SMS_NETTYPE_3GPP2 == format) {
		tcore_user_request_set_command(ur, TREQ_SMS_SEND_CDMA_MSG);
	} else {
		err("Invalid Format Received:[%d]", format);
		FAIL_RESPONSE(invocation, DEFAULT_MSG_REQ_FAILED);
		tcore_user_request_unref(ur);
		return TRUE;
	}

	ret = tcore_communicator_dispatch_request(ctx->comm, ur);
	if (ret != TCORE_RETURN_SUCCESS) {
		FAIL_RESPONSE(invocation, DEFAULT_MSG_REQ_FAILED);
		tcore_user_request_unref(ur);
	}

	return  TRUE;
}

static gboolean
on_sms_read_msg(TelephonySms *sms, GDBusMethodInvocation *invocation,
	gint arg_index,
	gpointer user_data)
{
	struct treq_sms_read_msg readMsg = {0,};
	struct custom_data *ctx = user_data;
	UserRequest *ur = NULL;
	TReturn	ret = TCORE_RETURN_SUCCESS;
	cynara *p_cynara = (ctx) ? ctx->p_cynara : NULL;

	if (!check_access_control(p_cynara, invocation, AC_SMS, "r"))
		return TRUE;

	readMsg.index = arg_index;

	ur = MAKE_UR(ctx, sms, invocation);
	tcore_user_request_set_data(ur, sizeof(struct treq_sms_read_msg), &readMsg);
	tcore_user_request_set_command(ur, TREQ_SMS_READ_MSG);

	ret = tcore_communicator_dispatch_request(ctx->comm, ur);
	if (ret != TCORE_RETURN_SUCCESS) {
		FAIL_RESPONSE(invocation, DEFAULT_MSG_REQ_FAILED);
		tcore_user_request_unref(ur);
	}

	return TRUE;
}

static gboolean
on_sms_save_msg(TelephonySms *sms, GDBusMethodInvocation *invocation,
	gint format,
	gint arg_msg_status,
	GVariant * arg_sca,
	gint arg_tpdu_length,
	GVariant * arg_tpdu_data,
	gpointer user_data)
{
	struct treq_sms_save_msg saveMsg = {0,};
	struct custom_data *ctx = user_data;
	UserRequest *ur = NULL;
	TReturn	ret = TCORE_RETURN_SUCCESS;
	cynara *p_cynara = (ctx) ? ctx->p_cynara : NULL;

	int i = 0;
	GVariantIter *iter = 0;
	GVariant *inner_gv = 0;

	if (!check_access_control(p_cynara, invocation, AC_SMS, "w"))
		return TRUE;

	if (SMS_NETTYPE_3GPP == format) {
		saveMsg.msgDataPackage.format = SMS_NETTYPE_3GPP;
	} else if (SMS_NETTYPE_3GPP2 == format) {
		saveMsg.msgDataPackage.format = SMS_NETTYPE_3GPP2;
	} else {
		err("Invalid Format Received:[%d]", format);
		FAIL_RESPONSE(invocation, DEFAULT_MSG_REQ_FAILED);
		tcore_user_request_unref(ur);
		return TRUE;
	}

	saveMsg.simIndex = 0xffff;
	saveMsg.msgStatus = arg_msg_status;

	inner_gv = g_variant_get_variant(arg_sca);
	g_variant_get(inner_gv, "ay", &iter);
	while (g_variant_iter_loop(iter, "y", &saveMsg.msgDataPackage.sca[i])) {
		i++;
		if (i >= SMS_SMSP_ADDRESS_LEN)
			break;
	}

	i = 0;
	inner_gv = g_variant_get_variant(arg_tpdu_data);
	g_variant_get(inner_gv, "ay", &iter);
	while (g_variant_iter_loop(iter, "y", &saveMsg.msgDataPackage.tpduData[i])) {
		i++;
		if (i >= SMS_SMDATA_SIZE_MAX + 1)
			break;
	}
	g_variant_iter_free(iter);
	g_variant_unref(inner_gv);

	saveMsg.msgDataPackage.msgLength = arg_tpdu_length;

	ur = MAKE_UR(ctx, sms, invocation);
	tcore_user_request_set_data(ur, sizeof(struct treq_sms_save_msg), &saveMsg);
	tcore_user_request_set_command(ur, TREQ_SMS_SAVE_MSG);

	ret = tcore_communicator_dispatch_request(ctx->comm, ur);
	if (ret != TCORE_RETURN_SUCCESS) {
		FAIL_RESPONSE(invocation, DEFAULT_MSG_REQ_FAILED);
		tcore_user_request_unref(ur);
	}

	return TRUE;
}

static gboolean
on_sms_delete_msg(TelephonySms *sms, GDBusMethodInvocation *invocation,
	gint arg_index,
	gpointer user_data)
{
	struct treq_sms_delete_msg deleteMsg = {0,};
	struct custom_data *ctx = user_data;
	UserRequest *ur = NULL;
	TReturn	ret = TCORE_RETURN_SUCCESS;
	cynara *p_cynara = (ctx) ? ctx->p_cynara : NULL;

	if (!check_access_control(p_cynara, invocation, AC_SMS, "x"))
		return TRUE;

	deleteMsg.index = arg_index;

	ur = MAKE_UR(ctx, sms, invocation);
	tcore_user_request_set_data(ur, sizeof(struct treq_sms_delete_msg), &deleteMsg);
	tcore_user_request_set_command(ur, TREQ_SMS_DELETE_MSG);

	ret = tcore_communicator_dispatch_request(ctx->comm, ur);
	if (ret != TCORE_RETURN_SUCCESS) {
		FAIL_RESPONSE(invocation, DEFAULT_MSG_REQ_FAILED);
		tcore_user_request_unref(ur);
	}

	return TRUE;
}

static gboolean
on_sms_get_msg_count(TelephonySms *sms, GDBusMethodInvocation *invocation,
	gpointer user_data)
{
	struct custom_data *ctx = user_data;
	UserRequest *ur = NULL;
	TReturn	ret = TCORE_RETURN_SUCCESS;
	cynara *p_cynara = (ctx) ? ctx->p_cynara : NULL;

	if (!check_access_control(p_cynara, invocation, AC_SMS, "r"))
		return TRUE;

	ur = MAKE_UR(ctx, sms, invocation);
	tcore_user_request_set_data(ur, 0, NULL);
	tcore_user_request_set_command(ur, TREQ_SMS_GET_COUNT);

	ret = tcore_communicator_dispatch_request(ctx->comm, ur);
	if (ret != TCORE_RETURN_SUCCESS) {
		FAIL_RESPONSE(invocation, DEFAULT_MSG_REQ_FAILED);
		tcore_user_request_unref(ur);
	}

	return TRUE;
}

static gboolean
on_sms_get_sca(TelephonySms *sms, GDBusMethodInvocation *invocation,
	gint arg_index,
	gpointer user_data)
{
	struct treq_sms_get_sca getSca = {0,};
	struct custom_data *ctx = user_data;
	UserRequest *ur = NULL;
	TReturn	ret = TCORE_RETURN_SUCCESS;
	cynara *p_cynara = (ctx) ? ctx->p_cynara : NULL;

	if (!check_access_control(p_cynara, invocation, AC_SMS, "r"))
		return TRUE;

	getSca.index = arg_index;

	ur = MAKE_UR(ctx, sms, invocation);
	tcore_user_request_set_data(ur, sizeof(struct treq_sms_get_sca), &getSca);
	tcore_user_request_set_command(ur, TREQ_SMS_GET_SCA);

	ret = tcore_communicator_dispatch_request(ctx->comm, ur);
	if (ret != TCORE_RETURN_SUCCESS) {
		FAIL_RESPONSE(invocation, DEFAULT_MSG_REQ_FAILED);
		tcore_user_request_unref(ur);
	}

	return TRUE;
}

static gboolean
on_sms_set_sca(TelephonySms *sms, GDBusMethodInvocation *invocation,
	gint arg_index,
	gint arg_ton,
	gint arg_npi,
	gint arg_dialNumberLength,
	GVariant *arg_dialNumber,
	gpointer user_data)
{
	struct treq_sms_set_sca setSca;
	struct custom_data *ctx = user_data;
	UserRequest *ur = NULL;
	TReturn	ret = TCORE_RETURN_SUCCESS;
	cynara *p_cynara = (ctx) ? ctx->p_cynara : NULL;

	int i = 0;
	GVariantIter *iter = 0;
	GVariant *inner_gv = 0;

	if (!check_access_control(p_cynara, invocation, AC_SMS, "w"))
		return TRUE;

	memset(&setSca, 0, sizeof(struct treq_sms_set_sca));

	setSca.index = arg_index;
	setSca.scaInfo.dialNumLen = arg_dialNumberLength;
	setSca.scaInfo.typeOfNum = arg_ton;
	setSca.scaInfo.numPlanId = arg_npi;

	if ((setSca.scaInfo.dialNumLen <= 0) || (setSca.scaInfo.dialNumLen > (SMS_MAX_SMS_SERVICE_CENTER_ADDR + 1))) {
		err("[tcore_SMS] TAPI_API_INVALID_INPUT !!!");
		FAIL_RESPONSE(invocation, DEFAULT_MSG_REQ_FAILED);
		return  TRUE;
	} else if (setSca.index != 0) {
		err("[tcore_SMS] Index except 0 is supported");
		FAIL_RESPONSE(invocation, DEFAULT_MSG_REQ_FAILED);
		return  TRUE;
	} else {
		inner_gv = g_variant_get_variant(arg_dialNumber);
		g_variant_get(inner_gv, "ay", &iter);
		while (g_variant_iter_loop(iter, "y", &setSca.scaInfo.diallingNum[i])) {
			i++;
			if (i >= SMS_SMSP_ADDRESS_LEN + 1)
				break;
		}

		ur = MAKE_UR(ctx, sms, invocation);
		tcore_user_request_set_data(ur, sizeof(struct treq_sms_set_sca), &setSca);
		tcore_user_request_set_command(ur, TREQ_SMS_SET_SCA);

		g_variant_iter_free(iter);
		g_variant_unref(inner_gv);

		ret = tcore_communicator_dispatch_request(ctx->comm, ur);
		if (ret != TCORE_RETURN_SUCCESS) {
			FAIL_RESPONSE(invocation, DEFAULT_MSG_REQ_FAILED);
			tcore_user_request_unref(ur);
		}
	}

	return TRUE;
}

static gboolean
on_sms_get_cb_config(TelephonySms *sms, GDBusMethodInvocation *invocation,
	gpointer user_data)
{
	struct custom_data *ctx = user_data;
	UserRequest *ur = NULL;
	TReturn	ret = TCORE_RETURN_SUCCESS;
	cynara *p_cynara = (ctx) ? ctx->p_cynara : NULL;

	if (!check_access_control(p_cynara, invocation, AC_SMS, "r"))
		return TRUE;

	ur = MAKE_UR(ctx, sms, invocation);
	tcore_user_request_set_data(ur, 0, NULL);
	tcore_user_request_set_command(ur, TREQ_SMS_GET_CB_CONFIG);

	ret = tcore_communicator_dispatch_request(ctx->comm, ur);
	if (ret != TCORE_RETURN_SUCCESS) {
		FAIL_RESPONSE(invocation, DEFAULT_MSG_REQ_FAILED);
		tcore_user_request_unref(ur);
	}

	return TRUE;
}

static gboolean
on_sms_set_cb_config(TelephonySms *sms, GDBusMethodInvocation *invocation,
	gint arg_net3gppType,
	gboolean arg_cbEnable,
	gint arg_msgIdMaxCount,
	gint arg_msgIdRangeCount,
	GVariant *arg_mdgId,
	gpointer user_data)
{
	struct treq_sms_set_cb_config setCbConfig = {0,};
	struct custom_data *ctx = user_data;
	UserRequest *ur = NULL;
	TReturn	ret = TCORE_RETURN_SUCCESS;
	cynara *p_cynara = (ctx) ? ctx->p_cynara : NULL;

	GVariant *value = NULL;
	GVariant *inner_gv = 0;
	GVariantIter *iter = NULL;
	GVariantIter *iter_row = NULL;
	const gchar *key = NULL;
	int i = 0;

	if (!check_access_control(p_cynara, invocation, AC_SMS, "w"))
		return TRUE;

	setCbConfig.net3gppType = arg_net3gppType;
	setCbConfig.cbEnabled = arg_cbEnable;
	setCbConfig.msgIdMaxCount = arg_msgIdMaxCount;
	setCbConfig.msgIdRangeCount = arg_msgIdRangeCount;

	inner_gv = g_variant_get_variant(arg_mdgId);
	g_variant_get(inner_gv, "aa{sv}", &iter);

	while (g_variant_iter_next(iter, "a{sv}", &iter_row)) {
		while (g_variant_iter_loop(iter_row, "{sv}", &key, &value)) {
			if (!g_strcmp0(key, "FromMsgId"))
				setCbConfig.msgIDs[i].net3gpp.fromMsgId = g_variant_get_uint16(value);
			if (!g_strcmp0(key, "ToMsgId"))
				setCbConfig.msgIDs[i].net3gpp.toMsgId = g_variant_get_uint16(value);
			if (!g_strcmp0(key, "CBCategory"))
				setCbConfig.msgIDs[i].net3gpp2.cbCategory = g_variant_get_uint16(value);
			if (!g_strcmp0(key, "CBLanguage"))
				setCbConfig.msgIDs[i].net3gpp2.cbLanguage = g_variant_get_uint16(value);
			if (!g_strcmp0(key, "Selected"))
				setCbConfig.msgIDs[i].net3gpp2.selected = g_variant_get_byte(value);
		}
		i++;
		g_variant_iter_free(iter_row);
		if (i >= SMS_GSM_SMS_CBMI_LIST_SIZE_MAX)
			break;
	}
	g_variant_iter_free(iter);

	ur = MAKE_UR(ctx, sms, invocation);
	tcore_user_request_set_data(ur, sizeof(struct treq_sms_set_cb_config), &setCbConfig);
	tcore_user_request_set_command(ur, TREQ_SMS_SET_CB_CONFIG);

	ret = tcore_communicator_dispatch_request(ctx->comm, ur);
	if (ret != TCORE_RETURN_SUCCESS) {
		FAIL_RESPONSE(invocation, DEFAULT_MSG_REQ_FAILED);
		tcore_user_request_unref(ur);
	}

	return TRUE;
}

static gboolean
on_sms_set_mem_status(TelephonySms *sms, GDBusMethodInvocation *invocation,
	gint arg_memoryStatus,
	gpointer user_data)
{
	struct treq_sms_set_mem_status memStatus = {0,};
	struct custom_data *ctx = user_data;
	UserRequest *ur = NULL;
	TReturn	ret = TCORE_RETURN_SUCCESS;
	cynara *p_cynara = (ctx) ? ctx->p_cynara : NULL;

	if (!check_access_control(p_cynara, invocation, AC_SMS, "w"))
		return TRUE;

	memStatus.memory_status = arg_memoryStatus;

	ur = MAKE_UR(ctx, sms, invocation);
	tcore_user_request_set_data(ur, sizeof(struct treq_sms_set_mem_status), &memStatus);
	tcore_user_request_set_command(ur, TREQ_SMS_SET_MEM_STATUS);

	ret = tcore_communicator_dispatch_request(ctx->comm, ur);
	if (ret != TCORE_RETURN_SUCCESS) {
		FAIL_RESPONSE(invocation, DEFAULT_MSG_REQ_FAILED);
		tcore_user_request_unref(ur);
	}

	return TRUE;
}

static gboolean
on_sms_get_pref_bearer(TelephonySms *sms, GDBusMethodInvocation *invocation,
	gpointer user_data)
{
	struct treq_sms_get_pref_bearer getPrefBearer;
	struct custom_data *ctx = user_data;
	UserRequest *ur = NULL;
	TReturn	ret = TCORE_RETURN_SUCCESS;
	cynara *p_cynara = (ctx) ? ctx->p_cynara : NULL;

	if (!check_access_control(p_cynara, invocation, AC_SMS, "r"))
		return TRUE;

	ur = MAKE_UR(ctx, sms, invocation);
	tcore_user_request_set_data(ur, sizeof(struct treq_sms_get_pref_bearer), &getPrefBearer);
	tcore_user_request_set_command(ur, TREQ_SMS_GET_PREF_BEARER);

	ret = tcore_communicator_dispatch_request(ctx->comm, ur);
	if (ret != TCORE_RETURN_SUCCESS) {
		FAIL_RESPONSE(invocation, DEFAULT_MSG_REQ_FAILED);
		tcore_user_request_unref(ur);
	}

	return TRUE;
}

static gboolean
on_sms_set_pref_bearer(TelephonySms *sms, GDBusMethodInvocation *invocation,
	gint arg_bearerType,
	gpointer user_data)
{
	struct treq_sms_set_pref_bearer setPrefBearer = {0,};
	struct custom_data *ctx = user_data;
	UserRequest *ur = NULL;
	TReturn	ret = TCORE_RETURN_SUCCESS;
	cynara *p_cynara = (ctx) ? ctx->p_cynara : NULL;

	if (!check_access_control(p_cynara, invocation, AC_SMS, "w"))
		return TRUE;

	setPrefBearer.svc = arg_bearerType;

	ur = MAKE_UR(ctx, sms, invocation);
	tcore_user_request_set_data(ur, sizeof(struct treq_sms_set_pref_bearer), &setPrefBearer);
	tcore_user_request_set_command(ur, TREQ_SMS_SET_PREF_BEARER);

	ret = tcore_communicator_dispatch_request(ctx->comm, ur);
	if (ret != TCORE_RETURN_SUCCESS) {
		FAIL_RESPONSE(invocation, DEFAULT_MSG_REQ_FAILED);
		tcore_user_request_unref(ur);
	}

	return TRUE;
}

static gboolean
on_sms_set_delivery_report(TelephonySms *sms, GDBusMethodInvocation *invocation,
	gint format,
	const gchar *arg_sca,
	gint arg_tpdu_length,
	const gchar *arg_tpdu_data,
	gint arg_rpCause,
	gpointer user_data)
{
	struct treq_sms_set_delivery_report deliveryReport;
	struct custom_data *ctx = user_data;
	UserRequest *ur = NULL;
	TReturn	ret = TCORE_RETURN_SUCCESS;
	cynara *p_cynara = (ctx) ? ctx->p_cynara : NULL;

	guchar *decoded_sca = NULL;
	guchar *decoded_tpdu = NULL;

	gsize decoded_sca_len = 0;
	gsize decoded_tpdu_len = 0;

	if (!check_access_control(p_cynara, invocation, AC_SMS, "w"))
		return TRUE;

	memset(&deliveryReport, 0, sizeof(struct treq_sms_set_delivery_report));

	if (SMS_NETTYPE_3GPP == format) {
		deliveryReport.dataInfo.format = SMS_NETTYPE_3GPP;
	} else if (SMS_NETTYPE_3GPP2 == format) {
		deliveryReport.dataInfo.format = SMS_NETTYPE_3GPP2;
	} else {
		err("Invalid Format Received:[%d]", format);
		FAIL_RESPONSE(invocation, DEFAULT_MSG_REQ_FAILED);
		tcore_user_request_unref(ur);
		return TRUE;
	}

	decoded_sca = g_base64_decode(arg_sca, &decoded_sca_len);
	if (NULL == decoded_sca) {
		warn("g_base64_decode: Failed to decode sca");
		FAIL_RESPONSE(invocation, DEFAULT_MSG_REQ_FAILED);
		return TRUE;
	}

	if (decoded_sca_len > SMS_SMSP_ADDRESS_LEN) {
		err("Invalid Sca length");
		FAIL_RESPONSE(invocation, DEFAULT_MSG_REQ_FAILED);
		goto EXIT;
	}
	memcpy(deliveryReport.dataInfo.sca, decoded_sca, decoded_sca_len);

	decoded_tpdu = g_base64_decode(arg_tpdu_data, &decoded_tpdu_len);
	if (NULL == decoded_tpdu) {
		warn("g_base64_decode: Failed to decode tpdu");
		FAIL_RESPONSE(invocation, DEFAULT_MSG_REQ_FAILED);
		goto EXIT;
	}

	info("[%s] Decoded SCA len:(%d) TPDU len:(%d)", GET_CP_NAME(invocation), decoded_sca_len, decoded_tpdu_len);

	if (decoded_tpdu_len > SMS_SMDATA_SIZE_MAX+1) {
		err("Invalid tpdu length");
		FAIL_RESPONSE(invocation, DEFAULT_MSG_REQ_FAILED);
		goto EXIT;
	}
	memcpy(deliveryReport.dataInfo.tpduData, decoded_tpdu, decoded_tpdu_len);

	deliveryReport.dataInfo.msgLength = arg_tpdu_length;
	deliveryReport.rspType = arg_rpCause;

	ur = MAKE_UR(ctx, sms, invocation);
	tcore_user_request_set_data(ur, sizeof(struct treq_sms_set_delivery_report), &deliveryReport);
	tcore_user_request_set_command(ur, TREQ_SMS_SET_DELIVERY_REPORT);

	ret = tcore_communicator_dispatch_request(ctx->comm, ur);
	if (ret != TCORE_RETURN_SUCCESS) {
		FAIL_RESPONSE(invocation, DEFAULT_MSG_REQ_FAILED);
		tcore_user_request_unref(ur);
	}

EXIT:

	g_free(decoded_sca);
	g_free(decoded_tpdu);

	return TRUE;
}

static gboolean
on_sms_set_msg_status(TelephonySms *sms, GDBusMethodInvocation *invocation,
	gint arg_index,
	gint arg_msgStatus,
	gpointer user_data)
{
	struct treq_sms_set_msg_status msgStatus = {0,};
	struct custom_data *ctx = user_data;
	UserRequest *ur = NULL;
	TReturn	ret = TCORE_RETURN_SUCCESS;
	cynara *p_cynara = (ctx) ? ctx->p_cynara : NULL;

	if (!check_access_control(p_cynara, invocation, AC_SMS, "w"))
		return TRUE;

	msgStatus.index = arg_index;
	msgStatus.msgStatus = arg_msgStatus;

	ur = MAKE_UR(ctx, sms, invocation);
	tcore_user_request_set_data(ur, sizeof(struct treq_sms_set_msg_status), &msgStatus);
	tcore_user_request_set_command(ur, TREQ_SMS_SET_MSG_STATUS);

	ret = tcore_communicator_dispatch_request(ctx->comm, ur);
	if (ret != TCORE_RETURN_SUCCESS) {
		FAIL_RESPONSE(invocation, DEFAULT_MSG_REQ_FAILED);
		tcore_user_request_unref(ur);
	}

	return TRUE;
}

static gboolean
on_sms_get_sms_params(TelephonySms *sms, GDBusMethodInvocation *invocation,
	gint arg_index,
	gpointer user_data)
{
	struct treq_sms_get_params getParams = {0,};
	struct custom_data *ctx = user_data;
	UserRequest *ur = NULL;
	TReturn	ret = TCORE_RETURN_SUCCESS;
	cynara *p_cynara = (ctx) ? ctx->p_cynara : NULL;

	if (!check_access_control(p_cynara, invocation, AC_SMS, "r"))
		return TRUE;

	getParams.index = arg_index;

	ur = MAKE_UR(ctx, sms, invocation);
	tcore_user_request_set_data(ur, sizeof(struct treq_sms_get_params), &getParams);
	tcore_user_request_set_command(ur, TREQ_SMS_GET_PARAMS);

	ret = tcore_communicator_dispatch_request(ctx->comm, ur);
	if (ret != TCORE_RETURN_SUCCESS) {
		FAIL_RESPONSE(invocation, DEFAULT_MSG_REQ_FAILED);
		tcore_user_request_unref(ur);
	}

	return TRUE;
}

static gboolean
on_sms_set_sms_params(TelephonySms *sms, GDBusMethodInvocation *invocation,
	gint arg_recordIndex,
	gint arg_recordLen,
	gint arg_alphaIdLen,
	GVariant *arg_alphaId,
	gint arg_paramIndicator,
	gint arg_destAddr_DialNumLen,
	gint arg_destAddr_Ton,
	gint arg_destAddr_Npi,
	GVariant *arg_destAddr_DiallingNum,
	gint arg_svcCntrAddr_DialNumLen,
	gint arg_SvcCntrAddr_Ton,
	gint arg_svcCntrAddr_Npi,
	GVariant *arg_svcCntrAddr_DialNum,
	gint arg_protocolId,
	gint arg_dataCodingScheme,
	gint arg_validityPeriod,
	gpointer user_data)
{
	struct treq_sms_set_params setParams;
	struct custom_data *ctx = user_data;
	UserRequest *ur = NULL;
	TReturn	ret = TCORE_RETURN_SUCCESS;
	cynara *p_cynara = (ctx) ? ctx->p_cynara : NULL;

	int i = 0;
	GVariantIter *iter = 0;
	GVariant *inner_gv = 0;

	if (!check_access_control(p_cynara, invocation, AC_SMS, "w"))
		return TRUE;

	memset(&setParams, 0, sizeof(struct treq_sms_set_params));

	setParams.params.recordIndex = arg_recordIndex;
	setParams.params.recordLen = arg_recordLen;
	setParams.params.alphaIdLen = arg_alphaIdLen;

	inner_gv = g_variant_get_variant(arg_alphaId);
	g_variant_get(inner_gv, "ay", &iter);
	while (g_variant_iter_loop(iter, "y", &setParams.params.szAlphaId[i])) {
		i++;
		if (i >= SMS_SMSP_ALPHA_ID_LEN_MAX + 1)
			break;
	}

	setParams.params.paramIndicator = arg_paramIndicator;

	setParams.params.tpDestAddr.dialNumLen = arg_destAddr_DialNumLen;
	setParams.params.tpDestAddr.typeOfNum = arg_destAddr_Ton;
	setParams.params.tpDestAddr.numPlanId = arg_destAddr_Npi;

	i = 0;
	inner_gv = g_variant_get_variant(arg_destAddr_DiallingNum);
	g_variant_get(inner_gv, "ay", &iter);
	while (g_variant_iter_loop(iter, "y", &setParams.params.tpDestAddr.diallingNum[i])) {
		i++;
		if (i >= SMS_SMSP_ADDRESS_LEN + 1)
			break;
	}

	setParams.params.tpSvcCntrAddr.dialNumLen = arg_svcCntrAddr_DialNumLen;
	setParams.params.tpSvcCntrAddr.typeOfNum = arg_SvcCntrAddr_Ton;
	setParams.params.tpSvcCntrAddr.numPlanId = arg_svcCntrAddr_Npi;

	i = 0;
	inner_gv = g_variant_get_variant(arg_svcCntrAddr_DialNum);
	g_variant_get(inner_gv, "ay", &iter);
	while (g_variant_iter_loop(iter, "y", &setParams.params.tpSvcCntrAddr.diallingNum[i])) {
		i++;
		if (i >= SMS_SMSP_ADDRESS_LEN + 1)
			break;
	}

	setParams.params.tpProtocolId = arg_protocolId;
	setParams.params.tpDataCodingScheme = arg_dataCodingScheme;
	setParams.params.tpValidityPeriod = arg_validityPeriod;

	ur = MAKE_UR(ctx, sms, invocation);
	tcore_user_request_set_data(ur, sizeof(struct treq_sms_set_params), &setParams);
	tcore_user_request_set_command(ur, TREQ_SMS_SET_PARAMS);

	g_variant_iter_free(iter);
	g_variant_unref(inner_gv);

	ret = tcore_communicator_dispatch_request(ctx->comm, ur);
	if (ret != TCORE_RETURN_SUCCESS) {
		FAIL_RESPONSE(invocation, DEFAULT_MSG_REQ_FAILED);
		tcore_user_request_unref(ur);
	}

	return TRUE;
}

static gboolean
on_sms_get_sms_param_cnt(TelephonySms *sms, GDBusMethodInvocation *invocation,
	gpointer user_data)
{
	struct custom_data *ctx = user_data;
	UserRequest *ur = NULL;
	TReturn	ret = TCORE_RETURN_SUCCESS;
	cynara *p_cynara = (ctx) ? ctx->p_cynara : NULL;

	if (!check_access_control(p_cynara, invocation, AC_SMS, "r"))
		return TRUE;

	ur = MAKE_UR(ctx, sms, invocation);
	tcore_user_request_set_data(ur, 0, NULL);
	tcore_user_request_set_command(ur, TREQ_SMS_GET_PARAMCNT);

	ret = tcore_communicator_dispatch_request(ctx->comm, ur);
	if (ret != TCORE_RETURN_SUCCESS) {
		FAIL_RESPONSE(invocation, DEFAULT_MSG_REQ_FAILED);
		tcore_user_request_unref(ur);
	}

	return TRUE;
}

static gboolean
on_sms_get_sms_ready_status(TelephonySms *sms, GDBusMethodInvocation *invocation,
	gpointer user_data)
{
	struct custom_data *ctx = user_data;
	GSList *co_list = NULL;
	CoreObject *co_sms = NULL;
	TcorePlugin *plugin = NULL;
	gboolean ready_status = FALSE;
	cynara *p_cynara = (ctx) ? ctx->p_cynara : NULL;

	if (!check_access_control(p_cynara, invocation, AC_SMS, "r"))
		return TRUE;

	plugin = tcore_server_find_plugin(ctx->server, GET_CP_NAME(invocation));
	co_list = tcore_plugin_get_core_objects_bytype(plugin, CORE_OBJECT_TYPE_SMS);
	if (!co_list) {
		err("error- co_list is NULL");
		FAIL_RESPONSE(invocation, DEFAULT_MSG_REQ_FAILED);
		return TRUE;
	}

	co_sms = (CoreObject *)co_list->data;
	g_slist_free(co_list);

	if (!co_sms) {
		err("error- co_sms is NULL");
		FAIL_RESPONSE(invocation, DEFAULT_MSG_REQ_FAILED);
		return TRUE;
	}

	ready_status = tcore_sms_get_ready_status(co_sms);
	dbg("[%s] ready_status = %d", GET_CP_NAME(invocation), ready_status);
	telephony_sms_complete_get_sms_ready_status(sms, invocation, ready_status);

	return TRUE;
}

gboolean dbus_plugin_setup_sms_interface(TelephonyObjectSkeleton *object, struct custom_data *ctx)
{
	TelephonySms *sms;

	sms = telephony_sms_skeleton_new();
	telephony_object_skeleton_set_sms(object, sms);
	g_object_unref(sms);

	g_signal_connect(sms,
		"handle-send-msg",
		G_CALLBACK(on_sms_send_msg), ctx);

	g_signal_connect(sms,
		"handle-read-msg",
		G_CALLBACK(on_sms_read_msg), ctx);

	g_signal_connect(sms,
		"handle-save-msg",
		G_CALLBACK(on_sms_save_msg), ctx);

	g_signal_connect(sms,
		"handle-delete-msg",
		G_CALLBACK(on_sms_delete_msg), ctx);

	g_signal_connect(sms,
		"handle-get-msg-count",
		G_CALLBACK(on_sms_get_msg_count), ctx);

	g_signal_connect(sms,
		"handle-get-sca",
		G_CALLBACK(on_sms_get_sca), ctx);

	g_signal_connect(sms,
		"handle-set-sca",
		G_CALLBACK(on_sms_set_sca), ctx);

	g_signal_connect(sms,
		"handle-get-cb-config",
		G_CALLBACK(on_sms_get_cb_config), ctx);

	g_signal_connect(sms,
		"handle-set-cb-config",
		G_CALLBACK(on_sms_set_cb_config), ctx);

	g_signal_connect(sms,
		"handle-set-mem-status",
		G_CALLBACK(on_sms_set_mem_status), ctx);

	g_signal_connect(sms,
		"handle-get-pref-bearer",
		G_CALLBACK(on_sms_get_pref_bearer), ctx);

	g_signal_connect(sms,
		"handle-set-pref-bearer",
		G_CALLBACK(on_sms_set_pref_bearer), ctx);

	g_signal_connect(sms,
		"handle-set-delivery-report",
		G_CALLBACK(on_sms_set_delivery_report), ctx);

	g_signal_connect(sms,
		"handle-set-msg-status",
		G_CALLBACK(on_sms_set_msg_status), ctx);

	g_signal_connect(sms,
		"handle-get-sms-params",
		G_CALLBACK(on_sms_get_sms_params), ctx);

	g_signal_connect(sms,
		"handle-set-sms-params",
		G_CALLBACK(on_sms_set_sms_params), ctx);

	g_signal_connect(sms,
		"handle-get-sms-param-cnt",
		G_CALLBACK(on_sms_get_sms_param_cnt), ctx);

	g_signal_connect(sms,
		"handle-get-sms-ready-status",
		G_CALLBACK(on_sms_get_sms_ready_status), ctx);

	return TRUE;
}

gboolean dbus_plugin_sms_response(struct custom_data *ctx, UserRequest *ur, struct dbus_request_info *dbus_info, enum tcore_response_command command, unsigned int data_len, const void *data)
{
	char *cpname = dbus_info ? GET_CP_NAME(dbus_info->invocation) : "";

	switch (command) {
	case TRESP_SMS_SEND_UMTS_MSG: {
		const struct tresp_sms_send_msg *resp = data;

		dbg("[%s] SEND_UMTS_MSG (result:[0x%x])", cpname, resp->result);
		telephony_sms_complete_send_msg(dbus_info->interface_object, dbus_info->invocation, resp->result);
		}
		break;

	case TRESP_SMS_SEND_CDMA_MSG: {
		const struct tresp_sms_send_msg *resp = data;

		dbg("[%s] SEND_CDMA_MSG (result:[0x%x])", cpname, resp->result);
		telephony_sms_complete_send_msg(dbus_info->interface_object, dbus_info->invocation, resp->result);
		}
		break;

	case TRESP_SMS_READ_MSG: {
		const struct tresp_sms_read_msg *resp = data;
		GVariant *sca = 0, *packet_sca = 0;
		GVariant *tpdu = 0, *packet_tpdu = 0;
		GVariantBuilder b;
		unsigned int i;

		dbg("[%s] READ_MSG (result:[0x%x])", cpname, resp->result);
		g_variant_builder_init(&b, G_VARIANT_TYPE("ay"));

		for (i = 0; i < SMS_SMSP_ADDRESS_LEN; i++)
			g_variant_builder_add(&b, "y", resp->dataInfo.smsData.sca[i]);
		sca = g_variant_builder_end(&b);

		g_variant_builder_init(&b, G_VARIANT_TYPE("ay"));

		for (i = 0; i < SMS_SMDATA_SIZE_MAX + 1; i++)
			g_variant_builder_add(&b, "y", resp->dataInfo.smsData.tpduData[i]);
		tpdu = g_variant_builder_end(&b);

		packet_sca = g_variant_new("v", sca);
		packet_tpdu = g_variant_new("v", tpdu);

		telephony_sms_complete_read_msg(dbus_info->interface_object, dbus_info->invocation,
			resp->result,
			resp->dataInfo.simIndex,
			resp->dataInfo.msgStatus,
			resp->dataInfo.smsData.format,
			packet_sca,
			resp->dataInfo.smsData.msgLength,
			packet_tpdu);
		}
		break;

	case TRESP_SMS_SAVE_MSG: {
		const struct tresp_sms_save_msg *resp = data;

		dbg("[%s] SAVE_MSG (index:[%d] result:[0x%x])", cpname, resp->index, resp->result);
		telephony_sms_complete_save_msg(dbus_info->interface_object, dbus_info->invocation,
			resp->result, resp->index);
		}
		break;

	case TRESP_SMS_DELETE_MSG: {
		const struct tresp_sms_delete_msg *resp = data;

		dbg("[%s] DELETE_MSG (index:[%d] result:[0x%x])", cpname, resp->index, resp->result);
		telephony_sms_complete_delete_msg(dbus_info->interface_object, dbus_info->invocation,
			resp->result, resp->index);
		}
		break;

	case TRESP_SMS_GET_STORED_MSG_COUNT: {
		const struct tresp_sms_get_storedMsgCnt *resp = data;
		GVariant *list;
		GVariantBuilder b;
		unsigned int i;

		dbg("[%s] GET_STORED_MSG_COUNT (result:[0x%x])", cpname, resp->result);
		g_variant_builder_init(&b, G_VARIANT_TYPE("ai"));

		for (i = 0; i < resp->storedMsgCnt.totalCount; i++)
			g_variant_builder_add(&b, "i", resp->storedMsgCnt.indexList[i]);
		list = g_variant_builder_end(&b);

		telephony_sms_complete_get_msg_count(dbus_info->interface_object, dbus_info->invocation,
			resp->result,
			resp->storedMsgCnt.totalCount,
			resp->storedMsgCnt.usedCount,
			list);
		}
		break;

	case TRESP_SMS_GET_SCA: {
		const struct tresp_sms_get_sca *resp = data;
		GVariant *sca = 0, *packet_sca = 0;
		GVariantBuilder b;
		unsigned int i;

		dbg("[%s] GET_SCA (result:[0x%x])", cpname, resp->result);
		g_variant_builder_init(&b, G_VARIANT_TYPE("ay"));

		for (i = 0; i < SMS_SMSP_ADDRESS_LEN + 1; i++)
			g_variant_builder_add(&b, "y", resp->scaAddress.diallingNum[i]);
		sca = g_variant_builder_end(&b);

		packet_sca = g_variant_new("v", sca);

		telephony_sms_complete_get_sca(dbus_info->interface_object, dbus_info->invocation,
			resp->result,
			resp->scaAddress.typeOfNum,
			resp->scaAddress.numPlanId,
			resp->scaAddress.dialNumLen,
			packet_sca);
		}
		break;

	case TRESP_SMS_SET_SCA: {
		const struct tresp_sms_set_sca *resp = data;

		dbg("[%s] SET_SCA (result:[0x%x])", cpname, resp->result);
		telephony_sms_complete_set_sca(dbus_info->interface_object, dbus_info->invocation,
			resp->result);
		}
		break;

	case TRESP_SMS_GET_CB_CONFIG: {
		const struct tresp_sms_get_cb_config *resp = data;
		GVariant *result = NULL;
		GVariantBuilder b;
		int i;

		dbg("[%s] GET_CB_CONFIG (result:[0x%x])", cpname, resp->result);
		g_variant_builder_init(&b, G_VARIANT_TYPE("aa{sv}"));

		for (i = 0; i < resp->cbConfig.msgIdRangeCount; i++) {
			g_variant_builder_open(&b, G_VARIANT_TYPE("a{sv}"));

			if (resp->cbConfig.net3gppType == SMS_NETTYPE_3GPP) {
				g_variant_builder_add(&b, "{sv}", "FromMsgId", g_variant_new_uint16(resp->cbConfig.msgIDs[i].net3gpp.fromMsgId));
				g_variant_builder_add(&b, "{sv}", "ToMsgId", g_variant_new_uint16(resp->cbConfig.msgIDs[i].net3gpp.toMsgId));
			} else if (resp->cbConfig.net3gppType == SMS_NETTYPE_3GPP2) {
				g_variant_builder_add(&b, "{sv}", "CBCategory", g_variant_new_uint16(resp->cbConfig.msgIDs[i].net3gpp2.cbCategory));
				g_variant_builder_add(&b, "{sv}", "CBLanguage", g_variant_new_uint16(resp->cbConfig.msgIDs[i].net3gpp2.cbLanguage));
			} else {
				dbg("Unknown 3gpp type");
				return FALSE;
			}

			g_variant_builder_add(&b, "{sv}", "Selected", g_variant_new_byte(resp->cbConfig.msgIDs[i].net3gpp.selected));
			g_variant_builder_close(&b);
		}

		result = g_variant_builder_end(&b);

		telephony_sms_complete_get_cb_config(dbus_info->interface_object, dbus_info->invocation,
			resp->result,
			resp->cbConfig.net3gppType,
			resp->cbConfig.cbEnabled,
			resp->cbConfig.msgIdMaxCount,
			resp->cbConfig.msgIdRangeCount,
			result);
		}
		break;

	case TRESP_SMS_SET_CB_CONFIG: {
		const struct tresp_sms_set_cb_config *resp = data;

		dbg("[%s] SET_CB_CONFIG (result:[0x%x])", cpname, resp->result);
		telephony_sms_complete_set_cb_config(dbus_info->interface_object, dbus_info->invocation,
			resp->result);
		}
		break;

	case TRESP_SMS_SET_MEM_STATUS: {
		const struct tresp_sms_set_mem_status *resp = data;

		dbg("[%s] SET_MEM_STATUS (result:[0x%x])", cpname, resp->result);
		telephony_sms_complete_set_mem_status(dbus_info->interface_object, dbus_info->invocation,
			resp->result);
		}
		break;

	case TRESP_SMS_GET_PREF_BEARER: {
		const struct tresp_sms_get_pref_bearer *resp = data;

		dbg("[%s] GET_PREF_BEARER (result:[0x%x] svc:[0x%2x])", cpname, resp->result, resp->svc);
		telephony_sms_complete_get_pref_bearer(dbus_info->interface_object, dbus_info->invocation,
			resp->result, resp->svc);
		}
		break;

	case TRESP_SMS_SET_PREF_BEARER: {
		const struct tresp_sms_set_pref_bearer *resp = data;

		dbg("[%s] SET_PREF_BEARER (result:[0x%x])", cpname, resp->result);
		telephony_sms_complete_set_pref_bearer(dbus_info->interface_object, dbus_info->invocation,
			resp->result);
		}
		break;

	case TRESP_SMS_SET_DELIVERY_REPORT: {
		const struct tresp_sms_set_delivery_report *resp = data;

		dbg("[%s] SET_DELIVERY_REPORT (result:[0x%x])", cpname, resp->result);
		telephony_sms_complete_set_delivery_report(dbus_info->interface_object, dbus_info->invocation,
			resp->result);
		}
		break;

	case TRESP_SMS_SET_MSG_STATUS: {
		const struct tresp_sms_set_mem_status *resp = data;

		dbg("[%s] SET_MSG_STATUS (result:[0x%x])", cpname, resp->result);
		telephony_sms_complete_set_msg_status(dbus_info->interface_object, dbus_info->invocation,
			resp->result);
		}
		break;

	case TRESP_SMS_GET_PARAMS: {
		const struct tresp_sms_get_params *resp = data;
		GVariant *alphaId = 0, *packet_alphaId = 0;
		GVariant *destDialNum = 0, *packet_destDialNum = 0;
		GVariant *scaDialNum = 0, *packet_scaDialNum = 0;
		GVariantBuilder b;
		unsigned int i;

		dbg("[%s] GET_PARAMS (result:[0x%x])", cpname, resp->result);
		g_variant_builder_init(&b, G_VARIANT_TYPE("ay"));
		for (i = 0; i < SMS_SMSP_ALPHA_ID_LEN_MAX + 1; i++)
			g_variant_builder_add(&b, "y", resp->paramsInfo.szAlphaId[i]);
		alphaId = g_variant_builder_end(&b);

		g_variant_builder_init(&b, G_VARIANT_TYPE("ay"));
		for (i = 0; i < SMS_SMSP_ADDRESS_LEN + 1; i++)
			g_variant_builder_add(&b, "y", resp->paramsInfo.tpDestAddr.diallingNum[i]);
		destDialNum = g_variant_builder_end(&b);

		g_variant_builder_init(&b, G_VARIANT_TYPE("ay"));
		for (i = 0; i < SMS_SMSP_ADDRESS_LEN + 1; i++)
			g_variant_builder_add(&b, "y", resp->paramsInfo.tpSvcCntrAddr.diallingNum[i]);
		scaDialNum = g_variant_builder_end(&b);

		packet_alphaId = g_variant_new("v", alphaId);
		packet_destDialNum = g_variant_new("v", destDialNum);
		packet_scaDialNum = g_variant_new("v", scaDialNum);

		telephony_sms_complete_get_sms_params(dbus_info->interface_object, dbus_info->invocation,
			resp->result,
			resp->paramsInfo.recordIndex,
			resp->paramsInfo.recordLen,
			resp->paramsInfo.alphaIdLen,
			packet_alphaId,
			resp->paramsInfo.paramIndicator,
			resp->paramsInfo.tpDestAddr.dialNumLen,
			resp->paramsInfo.tpDestAddr.typeOfNum,
			resp->paramsInfo.tpDestAddr.numPlanId,
			packet_destDialNum,
			resp->paramsInfo.tpSvcCntrAddr.dialNumLen,
			resp->paramsInfo.tpSvcCntrAddr.typeOfNum,
			resp->paramsInfo.tpSvcCntrAddr.numPlanId,
			packet_scaDialNum,
			resp->paramsInfo.tpProtocolId,
			resp->paramsInfo.tpDataCodingScheme,
			resp->paramsInfo.tpValidityPeriod);
		}
		break;

	case TRESP_SMS_SET_PARAMS:{
		const struct tresp_sms_set_params *resp = data;

		dbg("[%s] SET_PARAMS (result:[0x%x])", cpname, resp->result);
		telephony_sms_complete_set_sms_params(dbus_info->interface_object, dbus_info->invocation,
			resp->result);
		}
		break;

	case TRESP_SMS_GET_PARAMCNT: {
		const struct tresp_sms_get_paramcnt *resp = data;

		dbg("[%s] GET_PARAMCNT (result:[0x%x])", cpname, resp->result);
		telephony_sms_complete_get_sms_param_cnt(dbus_info->interface_object, dbus_info->invocation,
			resp->result, resp->recordCount);
		}
		break;

	default:
		break;
	}

	return TRUE;
}

gboolean dbus_plugin_sms_notification(struct custom_data *ctx, CoreObject *source, TelephonyObjectSkeleton *object, enum tcore_notification_command command, unsigned int data_len, const void *data)
{
	TelephonySms *sms;
	const char *cp_name;

	if (!object) {
		warn("object is NULL");
		return FALSE;
	}

	cp_name = tcore_server_get_cp_name_by_plugin(tcore_object_ref_plugin(source));

	sms = telephony_object_peek_sms(TELEPHONY_OBJECT(object));
	if (sms == NULL) {
		err("sms object is NULL!!!");
		return FALSE;
	}

	switch (command) {
	case TNOTI_SMS_INCOM_MSG: {
		const struct tnoti_sms_incoming_msg *noti = data;
		gchar *sca = NULL;
		gchar *tpdu = NULL;

		info("[%s] SMS_INCOM_MSG (len[%d])", cp_name, data_len);

		sca = g_base64_encode((const guchar *)(noti->msgInfo.sca), SMS_SMSP_ADDRESS_LEN);
		tpdu = g_base64_encode((const guchar *)(noti->msgInfo.tpduData), SMS_SMDATA_SIZE_MAX + 1);

		telephony_sms_emit_incomming_msg(sms,
			noti->msgInfo.format,
			sca,
			noti->msgInfo.msgLength,
			tpdu);

		g_free(sca);
		g_free(tpdu);
		}
		break;

	case TNOTI_SMS_CB_INCOM_MSG: {
		const struct tnoti_sms_cellBroadcast_msg *noti = data;
		gchar *tpdu = NULL;

		info("[%s] SMS_CB_INCOM_MSG (len[%d])", cp_name, data_len);

		tpdu = g_base64_encode((const guchar*)(noti->cbMsg.msgData), SMS_CB_SIZE_MAX+1);

		telephony_sms_emit_incomming_cb_msg(sms,
			noti->cbMsg.cbMsgType,
			noti->cbMsg.length,
			tpdu);
		g_free(tpdu);
		}
		break;

	case TNOTI_SMS_ETWS_INCOM_MSG: {
		const struct tnoti_sms_etws_msg *noti = data;
		GVariant *msgData = 0, *packet_msgData = 0;
		GVariantBuilder b;
		unsigned int i;

		info("[%s] ETWS_INCOM_MSG (len[%d])", cp_name, data_len);

		g_variant_builder_init(&b, G_VARIANT_TYPE("ay"));

		for (i = 0; i < SMS_ETWS_SIZE_MAX + 1; i++)
			g_variant_builder_add(&b, "y", noti->etwsMsg.msgData[i]);
		msgData = g_variant_builder_end(&b);
		packet_msgData = g_variant_new("v", msgData);

		telephony_sms_emit_incomming_etws_msg(sms,
			noti->etwsMsg.etwsMsgType,
			noti->etwsMsg.length,
			packet_msgData);
		}
		break;

	case TNOTI_SMS_INCOM_EX_MSG: {
		info("[%s] SMS_INCOM_EX_MSG (len[%d])", cp_name, data_len);
		}
		break;

	case TNOTI_SMS_CB_INCOM_EX_MSG: {
		info("[%s] CB_INCOM_EX_MSG (len[%d])", cp_name, data_len);
		}
		break;

	case TNOTI_SMS_MEMORY_STATUS: {
		const struct tnoti_sms_memory_status *noti = data;
		info("[%s] SMS_MEMORY_STATUS (%d)", cp_name, noti->status);
		telephony_sms_emit_memory_status(sms, noti->status);
		}
		break;

	case TNOTI_SMS_DEVICE_READY: {
		const struct tnoti_sms_ready_status *noti = data;
		info("[%s] SMS_DEVICE_READY (%d)", cp_name, noti->status);
#ifdef ENABLE_KPI_LOGS
		if (noti->status != SMS_READY_STATUS_NONE)
			TIME_CHECK("[%s] SMS Service Ready", cp_name);
#endif
		telephony_sms_emit_sms_ready(sms, noti->status);

		}
		break;

	default:
		dbg("unknown notification");
		return FALSE;
		break;
	}

	return TRUE;
}
