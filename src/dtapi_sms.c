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

#include "dtapi_sms.h"
#include "dtapi_util.h"

#include <plugin.h>
#include <tel_sms.h>

#define AC_SMS		"telephony_framework::api_sms"

static void on_response_dtapi_sms_send(gint result,
	const void *data, void *cb_data)
{
	DbusRespCbData *rsp_cb_data = cb_data;
	dbg("SEND_MSG_RESPONSE RECEIVED - result:[%d]", result);

	tcore_check_return_assert(rsp_cb_data != NULL);

	telephony_sms_complete_send(rsp_cb_data->interface_object,
		rsp_cb_data->invocation, result);

	tcore_free(rsp_cb_data);
}

static gboolean dtapi_sms_send(TelephonySms *sms,
	GDBusMethodInvocation *invocation,
	gboolean more_msgs, GVariant *sca, const gchar *tpdu,
	gpointer user_data)
{
	DbusRespCbData *rsp_cb_data = NULL;
	TcorePlugin *plugin =  user_data;

	TelSmsSendInfo send_msg;
	gsize decoded_tpdu_length;
	guchar *decoded_tpdu = NULL;

	GVariantIter *iter = NULL;
	GVariant *key_value;
	const gchar *key;

	TelReturn result;

	if (dtapi_check_access_control(invocation, AC_SMS, "x") == FALSE)
		return TRUE;

	if (NULL == tpdu || NULL == sca) {
		dtapi_return_error(invocation, G_DBUS_ERROR_FAILED, "Invalid Parameter");
		return TRUE;
	}

	decoded_tpdu = g_base64_decode(tpdu, &decoded_tpdu_length);
	if (decoded_tpdu_length > (TEL_SMS_SMDATA_SIZE_MAX + 1)) {
		dtapi_return_error(invocation, G_DBUS_ERROR_FAILED, "Tpdu Decoding Failed");
		tcore_free(decoded_tpdu);
		return TRUE;
	}

	memset (&send_msg, 0x0, sizeof(TelSmsSendInfo));
	rsp_cb_data = dtapi_create_resp_cb_data(sms, invocation, NULL, 0);
	send_msg.more_msgs = more_msgs;

	g_variant_get(sca, "a{sv}", &iter);
	while (g_variant_iter_loop(iter, "{sv}", &key, &key_value)) {
		if (g_strcmp0(key, "ton") == 0) {
			send_msg.send_data.sca.ton = g_variant_get_byte(key_value);
		}
		else if (g_strcmp0(key, "npi") == 0) {
			send_msg.send_data.sca.npi = g_variant_get_byte(key_value);
		}
		else if (g_strcmp0(key, "number") == 0) {
			g_strlcpy(send_msg.send_data.sca.number,
				g_variant_get_string(key_value, NULL),
				TEL_SMS_SCA_LEN_MAX + 1);
		}
	}
	g_variant_iter_free(iter);

	memcpy(&(send_msg.send_data.tpdu[0]), decoded_tpdu, decoded_tpdu_length);
	send_msg.send_data.tpdu_length = decoded_tpdu_length;
	tcore_free(decoded_tpdu);

	result = tcore_plugin_dispatch_request(plugin, TRUE,
		TCORE_COMMAND_SMS_SEND_SMS,
		&send_msg, sizeof(TelSmsSendInfo),
		on_response_dtapi_sms_send, rsp_cb_data);
	if (result != TEL_RETURN_SUCCESS) {
		dtapi_return_error(invocation, G_DBUS_ERROR_FAILED, "Dispatch Failed");
		tcore_free(rsp_cb_data);
	}

	return TRUE;
}

static void on_response_dtapi_sms_read_in_sim(gint result,
	const void *data, void *cb_data)
{
	DbusRespCbData *rsp_cb_data = cb_data;

	gint status = 0;
	gchar *tpdu = NULL;

	GVariantBuilder sca_builder;
	GVariant *sca = NULL;

	dbg("READ_MSG_RESPONSE RECEIVED - result:[%d]", result);
	tcore_check_return_assert(rsp_cb_data != NULL);

	g_variant_builder_init(&sca_builder, G_VARIANT_TYPE("a{sv}"));
	if (TEL_SMS_RESULT_SUCCESS == result) {
		const TelSmsSimDataInfo *read_sms = data;

		tcore_check_return_assert(read_sms != NULL);
		tcore_check_return_assert(read_sms->data.tpdu_length <= TEL_SMS_SMDATA_SIZE_MAX);
		tcore_check_return_assert(read_sms->data.tpdu_length != 0);

		g_variant_builder_add(&sca_builder, "{sv}",
			"ton", g_variant_new_byte(read_sms->data.sca.ton));
		g_variant_builder_add(&sca_builder, "{sv}",
			"npi", g_variant_new_byte(read_sms->data.sca.npi));
		g_variant_builder_add(&sca_builder, "{sv}",
			"number", g_variant_new_string(read_sms->data.sca.number));

		tpdu = g_base64_encode(&(read_sms->data.tpdu[0]),
			read_sms->data.tpdu_length);
		status  = read_sms->status;
	} else {
		tpdu = g_strdup(" ");
	}
	sca = g_variant_builder_end(&sca_builder);

	telephony_sms_complete_read_in_sim(rsp_cb_data->interface_object,
		rsp_cb_data->invocation, result, status, sca, tpdu);

	tcore_free(rsp_cb_data);
	tcore_free(tpdu);
}

static gboolean dtapi_sms_read_in_sim(TelephonySms *sms,
	GDBusMethodInvocation *invocation,
	guint index, gpointer user_data)
{
	DbusRespCbData *rsp_cb_data = NULL;
	TcorePlugin *plugin =  user_data;
	TelReturn result;

	if (dtapi_check_access_control(invocation, AC_SMS, "r") == FALSE)
		return TRUE;

	rsp_cb_data = dtapi_create_resp_cb_data(sms, invocation, NULL, 0);

	result = tcore_plugin_dispatch_request(plugin, TRUE,
		TCORE_COMMAND_SMS_READ_IN_SIM,
		&index, sizeof(guint),
		on_response_dtapi_sms_read_in_sim, rsp_cb_data);
	if (result != TEL_RETURN_SUCCESS) {
		dtapi_return_error(invocation, G_DBUS_ERROR_FAILED, "Dispatch Failed");
		tcore_free(rsp_cb_data);
	}

	return TRUE;
}

static void on_response_dtapi_sms_write_in_sim(gint result,
	const void *data, void *cb_data)
{
	DbusRespCbData *rsp_cb_data = cb_data;
	guint index = 0;

	dbg("WRITE_SMS_RESPONSE_RECEIVED - result:[%d]", result);

	tcore_check_return_assert(rsp_cb_data != NULL);

	if (TEL_SMS_RESULT_SUCCESS == result) {
		tcore_check_return_assert(NULL != data);
		index = *(guint *)data;
		dbg("SMS writen to index: [%d]", index);
	}

	telephony_sms_complete_write_in_sim(rsp_cb_data->interface_object,
		rsp_cb_data->invocation, result, index);

	tcore_free(rsp_cb_data);
}

static gboolean dtapi_sms_write_in_sim(TelephonySms *sms,
	GDBusMethodInvocation *invocation,
	gint status, GVariant *sca, const gchar *tpdu,
	gpointer user_data)
{
	DbusRespCbData *rsp_cb_data = NULL;
	TcorePlugin *plugin =  user_data;

	TelSmsSimDataInfo write_data;
	guchar *decoded_tpdu = NULL;
	gsize decoded_tpdu_length = 0;

	GVariantIter *iter = NULL;
	GVariant *key_value;
	const gchar *key;

	TelReturn result;

	if (dtapi_check_access_control(invocation, AC_SMS, "w") == FALSE)
		return TRUE;

	if (NULL == sca || NULL == tpdu) {
		dtapi_return_error(invocation, G_DBUS_ERROR_FAILED, "Invalid Parameter");
		return TRUE;
	}

	g_variant_get(sca, "a{sv}", &iter);
	while (g_variant_iter_loop(iter, "{sv}", &key, &key_value)) {
		if (g_strcmp0(key, "ton") == 0) {
			write_data.data.sca.ton = g_variant_get_byte(key_value);
		}
		else if (g_strcmp0(key, "npi") == 0) {
			write_data.data.sca.npi = g_variant_get_byte(key_value);
		}
		else if (g_strcmp0(key, "number") == 0) {
			g_strlcpy(write_data.data.sca.number,
				g_variant_get_string(key_value, NULL),
				TEL_SMS_SCA_LEN_MAX + 1);
		}
	}
	g_variant_iter_free(iter);

	decoded_tpdu = g_base64_decode(tpdu, &decoded_tpdu_length);
	memcpy(&(write_data.data.tpdu[0]), decoded_tpdu, decoded_tpdu_length);
	write_data.data.tpdu_length = decoded_tpdu_length;
	write_data.status = status;

	/* Free decoded data */
	tcore_free(decoded_tpdu);

	rsp_cb_data = dtapi_create_resp_cb_data(sms, invocation, NULL, 0);

	result = tcore_plugin_dispatch_request(plugin, TRUE,
		TCORE_COMMAND_SMS_WRITE_IN_SIM,
		&write_data, sizeof(TelSmsSimDataInfo),
		on_response_dtapi_sms_write_in_sim, rsp_cb_data);
	if (result != TEL_RETURN_SUCCESS) {
		dtapi_return_error(invocation, G_DBUS_ERROR_FAILED, "Dispatch Failed");
		tcore_free(rsp_cb_data);
	}

	return TRUE;
}

static void on_response_dtapi_sms_delete_in_sim(gint result,
	const void *data, void *cb_data)
{
	DbusRespCbData *rsp_cb_data = cb_data;
	dbg("DELETE_SMS RESPONSE RECEIVED - result: [%d]", result);

	tcore_check_return_assert(rsp_cb_data != NULL);

	telephony_sms_complete_delete_in_sim(rsp_cb_data->interface_object,
		rsp_cb_data->invocation, result);

	tcore_free(rsp_cb_data);
}

static gboolean dtapi_sms_delete_in_sim(TelephonySms *sms,
	GDBusMethodInvocation *invocation,
	guint index, gpointer user_data)
{
	DbusRespCbData *rsp_cb_data = NULL;
	TcorePlugin *plugin =  user_data;
	TelReturn result;

	if (dtapi_check_access_control(invocation, AC_SMS, "x") == FALSE)
		return TRUE;

	rsp_cb_data = dtapi_create_resp_cb_data(sms, invocation, NULL, 0);

	result = tcore_plugin_dispatch_request(plugin, TRUE,
		TCORE_COMMAND_SMS_DELETE_IN_SIM,
		&index, sizeof(guint),
		on_response_dtapi_sms_delete_in_sim, rsp_cb_data);
	if (result != TEL_RETURN_SUCCESS) {
		dtapi_return_error(invocation, G_DBUS_ERROR_FAILED, "Dispatch Failed");
		tcore_free(rsp_cb_data);
	}

	return TRUE;
}

static void on_response_dtapi_sms_get_count(gint result,
	const void *data, void *cb_data)
{
	DbusRespCbData *rsp_cb_data = cb_data;
	GVariant *index_list = NULL;
	GVariantBuilder index;
	unsigned int iter;
	unsigned int total_count = 0;
	unsigned int used_count = 0;

	dbg("GET_COUNT_RESPONSE RECEIVED - result:[%d]", result);

	tcore_check_return_assert(rsp_cb_data != NULL);

	g_variant_builder_init(&index, G_VARIANT_TYPE("au"));
	if (TEL_SMS_RESULT_SUCCESS == result) {
		const TelSmsStoredMsgCountInfo *count_info = data;
		tcore_check_return_assert(NULL != count_info);

		total_count =  count_info->total_count;
		used_count = count_info->used_count;
		dbg("Total count: [%d] Used count: [%d]",
			total_count, used_count);

		for (iter = 0; iter < total_count; iter++) {
			dbg("count_info->index_list[%d] - %d", iter , count_info->index_list[iter]);
			g_variant_builder_add(&index, "u", count_info->index_list[iter]);
		}
	}
	else {
		err("SMS get count failed!!!");
	}
	index_list = g_variant_builder_end(&index);

	telephony_sms_complete_get_count(rsp_cb_data->interface_object,
		rsp_cb_data->invocation, result,
		total_count, used_count, index_list);

	tcore_free(rsp_cb_data);
}

static gboolean dtapi_sms_get_count(TelephonySms *sms,
	GDBusMethodInvocation *invocation, gpointer user_data)
{
	DbusRespCbData *rsp_cb_data = NULL;
	TcorePlugin *plugin =  user_data;
	TelReturn result;

	if (dtapi_check_access_control(invocation, AC_SMS, "r") == FALSE)
		return TRUE;

	rsp_cb_data = dtapi_create_resp_cb_data(sms, invocation, NULL, 0);

	result = tcore_plugin_dispatch_request(plugin, TRUE,
		TCORE_COMMAND_SMS_GET_COUNT,
		NULL, 0,
		on_response_dtapi_sms_get_count, rsp_cb_data);
	if (result != TEL_RETURN_SUCCESS) {
		dtapi_return_error(invocation, G_DBUS_ERROR_FAILED, "Dispatch Failed");
		tcore_free(rsp_cb_data);
	}

	return TRUE;
}

static void on_response_dtapi_sms_set_cb_config(gint result,
	const void *data, void *cb_data)
{
	DbusRespCbData *rsp_cb_data = cb_data;
	dbg("SET_CB_CONFIG RESPONSE RECEIVED - result:[%d]", result);

	tcore_check_return_assert(rsp_cb_data != NULL);

	telephony_sms_complete_set_cb_config(rsp_cb_data->interface_object,
		rsp_cb_data->invocation, result);

	tcore_free(rsp_cb_data);
}

static gboolean dtapi_sms_set_cb_config(TelephonySms *sms,
	GDBusMethodInvocation *invocation,
	gboolean cb_enabled,
	guint msg_id_range_cnt, GVariant *msg_ids,
	gpointer user_data)
{
	DbusRespCbData *rsp_cb_data = NULL;
	TcorePlugin *plugin =  user_data;

	TelSmsCbConfigInfo cb_conf;

	GVariantIter *iter = NULL, *iter_row = NULL;
	GVariant *key_value;
	const gchar *key;
	guint count = 0;

	TelReturn result;

	if (dtapi_check_access_control(invocation, AC_SMS, "w") == FALSE)
		return TRUE;

	rsp_cb_data = dtapi_create_resp_cb_data(sms, invocation, NULL, 0);

	memset(&cb_conf, 0x0, sizeof(TelSmsCbConfigInfo));
	cb_conf.cb_enabled = cb_enabled;
	cb_conf.msg_id_range_cnt = msg_id_range_cnt;

	g_variant_get(msg_ids, "aa{sv}", &iter);
	while (g_variant_iter_next(iter, "a{sv}", &iter_row)) {
		while (g_variant_iter_loop(iter_row, "{sv}", &key, &key_value)) {
			if (g_strcmp0(key, "from_msg_id") == 0) {
				cb_conf.msg_ids[count].from_msg_id =
					g_variant_get_uint16(key_value);
			}
			else if (g_strcmp0(key, "to_msg_id") == 0) {
				cb_conf.msg_ids[count].to_msg_id =
					g_variant_get_uint16(key_value);
			}
			else if (g_strcmp0(key, "number") == 0) {
				cb_conf.msg_ids[count].selected =
					g_variant_get_boolean(key_value);
			}
		}
		count++;
		g_variant_iter_free(iter_row);
	}
	g_variant_iter_free(iter);

	result = tcore_plugin_dispatch_request(plugin, TRUE,
		TCORE_COMMAND_SMS_SET_CB_CONFIG,
		&cb_conf, sizeof(TelSmsCbConfigInfo),
		on_response_dtapi_sms_set_cb_config, rsp_cb_data);
	if (result != TEL_RETURN_SUCCESS) {
		dtapi_return_error(invocation, G_DBUS_ERROR_FAILED, "Dispatch Failed");
		tcore_free(rsp_cb_data);
	}

	return TRUE;
}

static void on_response_dtapi_sms_get_cb_config(gint result,
	const void *data, void *cb_data)
{
	DbusRespCbData *rsp_cb_data = cb_data;

	GVariantBuilder config_builder;

	GVariant *msg_ids = NULL;
	gboolean cb_enabled = FALSE;
	guint msg_id_range_cnt = 0;

	dbg("GET_CB_CONFIG RESPONSE RECEIVED - result:[%d]", result);

	tcore_check_return_assert(rsp_cb_data != NULL);

	g_variant_builder_init(&config_builder, G_VARIANT_TYPE("aa{sv}"));
	if (TEL_SMS_RESULT_SUCCESS == result) {
		const TelSmsCbConfigInfo *config = data;
		guint count = 0;
		tcore_check_return_assert(NULL != config);

		cb_enabled = config->cb_enabled;
		msg_id_range_cnt = config->msg_id_range_cnt;

		for (count = 0; count < config->msg_id_range_cnt; count++) {
			g_variant_builder_open(&config_builder, G_VARIANT_TYPE("a{sv}"));

			g_variant_builder_add(&config_builder, "{sv}",
				"from_msg_id",
				g_variant_new_uint16(config->msg_ids[count].from_msg_id));
			g_variant_builder_add(&config_builder, "{sv}",
				"to_msg_id",
				g_variant_new_uint16(config->msg_ids[count].to_msg_id));
			g_variant_builder_add(&config_builder, "{sv}",
				"selected",
				g_variant_new_boolean(config->msg_ids[count].selected));

			g_variant_builder_close(&config_builder);
		}
	}
	msg_ids = g_variant_builder_end(&config_builder);

	telephony_sms_complete_get_cb_config(rsp_cb_data->interface_object,
		rsp_cb_data->invocation, result,
		cb_enabled, msg_id_range_cnt, msg_ids);

	tcore_free(rsp_cb_data);
}

static gboolean dtapi_sms_get_cb_config(TelephonySms *sms,
	GDBusMethodInvocation *invocation, gpointer user_data)
{
	DbusRespCbData *rsp_cb_data = NULL;
	TcorePlugin *plugin =  user_data;
	TelReturn result;

	if (dtapi_check_access_control(invocation, AC_SMS, "r") == FALSE)
		return TRUE;

	rsp_cb_data = dtapi_create_resp_cb_data(sms, invocation, NULL, 0);

	result = tcore_plugin_dispatch_request(plugin, TRUE,
		TCORE_COMMAND_SMS_GET_CB_CONFIG,
		NULL, 0,
		on_response_dtapi_sms_get_cb_config, rsp_cb_data);
	if (result != TEL_RETURN_SUCCESS) {
		dtapi_return_error(invocation, G_DBUS_ERROR_FAILED, "Dispatch Failed");
		tcore_free(rsp_cb_data);
	}

	return TRUE;
}

static void on_response_dtapi_sms_get_parameters(gint result,
	const void *data, void *cb_data)
{
	DbusRespCbData *rsp_cb_data = cb_data;
	GVariant *params = NULL;
	GVariantBuilder params_builder;
	guint count = 0;

	dbg("GET_SMS_PARAMETERS RESPONSE RECEIVED - result:[%d]", result);
	tcore_check_return_assert(rsp_cb_data != NULL);

	g_variant_builder_init(&params_builder, G_VARIANT_TYPE("aa{sv}"));
	if (TEL_SMS_RESULT_SUCCESS == result) {
		const TelSmsParamsInfoList *param_list = data;
		GVariantBuilder sca_builder;
		GVariant *sca;

		tcore_check_return_assert(param_list != NULL);

		for (count = 0; count < param_list->count; count++) {
			g_variant_builder_open(&params_builder, G_VARIANT_TYPE("a{sv}"));

			g_variant_builder_add(&params_builder, "{sv}",
				"index", g_variant_new_uint32(param_list->params[count].index));

			g_variant_builder_init(&sca_builder, G_VARIANT_TYPE("a{sv}"));
			g_variant_builder_add(&sca_builder, "{sv}",
				"ton", g_variant_new_byte(param_list->params[count].sca.ton));
			g_variant_builder_add(&sca_builder, "{sv}",
				"npi", g_variant_new_byte(param_list->params[count].sca.npi));
			g_variant_builder_add(&sca_builder, "{sv}",
				"number", g_variant_new_string(param_list->params[count].sca.number));
			sca = g_variant_builder_end(&sca_builder);
			g_variant_builder_add(&params_builder, "{sv}",
				"sca", sca);

			g_variant_builder_add(&params_builder, "{sv}",
				"vp", g_variant_new_uint16(param_list->params[count].vp));

			g_variant_builder_close(&params_builder);
		}
	}
	params = g_variant_builder_end(&params_builder);

	telephony_sms_complete_get_parameters(rsp_cb_data->interface_object,
		rsp_cb_data->invocation, result, count, params);

	tcore_free(rsp_cb_data);
}

static gboolean dtapi_sms_get_parameters(TelephonySms *sms,
	GDBusMethodInvocation *invocation, gpointer user_data)
{
	DbusRespCbData *rsp_cb_data = NULL;
	TcorePlugin *plugin =  user_data;
	TelReturn result;

	if (dtapi_check_access_control(invocation, AC_SMS, "r") == FALSE)
		return TRUE;

	rsp_cb_data = dtapi_create_resp_cb_data(sms, invocation, NULL, 0);

	result = tcore_plugin_dispatch_request(plugin, TRUE,
		TCORE_COMMAND_SMS_GET_PARAMETERS,
		NULL, 0,
		on_response_dtapi_sms_get_parameters, rsp_cb_data);
	if (result != TEL_RETURN_SUCCESS) {
		dtapi_return_error(invocation, G_DBUS_ERROR_FAILED, "Dispatch Failed");
		tcore_free(rsp_cb_data);
	}

	return TRUE;
}

static void on_response_dtapi_sms_set_parameters(gint result,
	const void *data, void *cb_data)
{
	DbusRespCbData *rsp_cb_data = cb_data;
	dbg("SET_SMS_PARAMETERS RESPONSE RECEIVED - result:[%d]", result);

	tcore_check_return_assert(rsp_cb_data != NULL);

	telephony_sms_complete_set_parameters(rsp_cb_data->interface_object,
		rsp_cb_data->invocation, result);

	tcore_free(rsp_cb_data);
}

static gboolean dtapi_sms_set_parameters(TelephonySms *sms,
	GDBusMethodInvocation *invocation,
	guint index, GVariant *sca, guint16 vp,
	gpointer user_data)
{
	DbusRespCbData *rsp_cb_data = NULL;
	TcorePlugin *plugin = user_data;

	TelSmsParamsInfo set_params = {0, };

	GVariantIter *iter = NULL;
	GVariant *key_value;
	const gchar *key;

	TelReturn result;

	if (dtapi_check_access_control(invocation, AC_SMS, "w") == FALSE)
		return TRUE;

	set_params.index = index;
	set_params.vp = vp;

	g_variant_get(sca, "a{sv}", &iter);
	while (g_variant_iter_loop(iter, "{sv}", &key, &key_value)) {
		if (g_strcmp0(key, "ton") == 0) {
			set_params.sca.ton = g_variant_get_byte(key_value);
		}
		else if (g_strcmp0(key, "npi") == 0) {
			set_params.sca.npi = g_variant_get_byte(key_value);
		}
		else if (g_strcmp0(key, "number") == 0) {
			g_strlcpy(set_params.sca.number,
				g_variant_get_string(key_value, NULL),
				TEL_SMS_SCA_LEN_MAX + 1);
		}
	}
	g_variant_iter_free(iter);

	rsp_cb_data = dtapi_create_resp_cb_data(sms, invocation, NULL, 0);

	result = tcore_plugin_dispatch_request(plugin, TRUE,
		TCORE_COMMAND_SMS_SET_PARAMETERS,
		&set_params, sizeof(TelSmsParamsInfo),
		on_response_dtapi_sms_set_parameters, rsp_cb_data);

	if (result != TEL_RETURN_SUCCESS) {
		dtapi_return_error(invocation, G_DBUS_ERROR_FAILED, "Dispatch Failed");
		tcore_free(rsp_cb_data);
	}

	return TRUE;
}

static void on_response_dtapi_sms_send_deliver_report(gint result,
	const void *data, void *cb_data)
{
	DbusRespCbData *rsp_cb_data = cb_data;
	dbg("SEND_DELIVER_REPORT RESPONSE RECEIVED - result:[%d]", result);

	tcore_check_return_assert(rsp_cb_data != NULL);

	telephony_sms_complete_send_deliver_report(rsp_cb_data->interface_object,
		rsp_cb_data->invocation, result);

	tcore_free(rsp_cb_data);
}

static gboolean dtapi_sms_send_deliver_report(TelephonySms *sms,
	GDBusMethodInvocation *invocation,
	gint report, GVariant *sca, const gchar *tpdu,
	gpointer user_data)
{
	DbusRespCbData *rsp_cb_data = NULL;
	TcorePlugin *plugin =  user_data;

	TelSmsDeliverReportInfo dr_info;
	gsize decoded_tpdu_length;
	guchar *decoded_tpdu = NULL;

	GVariantIter *iter = NULL;
	GVariant *key_value;
	const gchar *key;

	TelReturn result;

	if (dtapi_check_access_control(invocation, AC_SMS, "w") == FALSE)
		return TRUE;

	if (NULL == sca || NULL == tpdu) {
		dtapi_return_error(invocation, G_DBUS_ERROR_FAILED, "Invalid Parameter");
		return TRUE;
	}

	decoded_tpdu = g_base64_decode(tpdu, &decoded_tpdu_length);
	if (decoded_tpdu_length > (TEL_SMS_SMDATA_SIZE_MAX + 1)) {
		dtapi_return_error(invocation, G_DBUS_ERROR_FAILED, "Tpdu Decoding Failed");
		tcore_free(decoded_tpdu);
		return TRUE;
	}

	memset (&dr_info, 0x0, sizeof(TelSmsDeliverReportInfo));
	dr_info.report = report;
	memcpy(&(dr_info.data.tpdu[0]), decoded_tpdu, decoded_tpdu_length);
	dr_info.data.tpdu_length = decoded_tpdu_length;
	tcore_free(decoded_tpdu);

	g_variant_get(sca, "a{sv}", &iter);
	while (g_variant_iter_loop(iter, "{sv}", &key, &key_value)) {
		if (g_strcmp0(key, "ton") == 0) {
			dr_info.data.sca.ton = g_variant_get_byte(key_value);
		}
		else if (g_strcmp0(key, "npi") == 0) {
			dr_info.data.sca.npi = g_variant_get_byte(key_value);
		}
		else if (g_strcmp0(key, "number") == 0) {
			g_strlcpy(dr_info.data.sca.number,
				g_variant_get_string(key_value, NULL),
				TEL_SMS_SCA_LEN_MAX + 1);
		}
	}
	g_variant_iter_free(iter);

	rsp_cb_data = dtapi_create_resp_cb_data(sms, invocation, NULL, 0);

	result = tcore_plugin_dispatch_request(plugin, TRUE,
		TCORE_COMMAND_SMS_SEND_DELIVER_REPORT,
		&dr_info, sizeof(TelSmsDeliverReportInfo),
		on_response_dtapi_sms_send_deliver_report, rsp_cb_data);
	if (result != TEL_RETURN_SUCCESS) {
		dtapi_return_error(invocation, G_DBUS_ERROR_FAILED, "Dispatch Failed");
		tcore_free(rsp_cb_data);
	}

	return TRUE;
}

static void on_response_dtapi_sms_set_sca(gint result,
	const void *data, void *cb_data)
{
	DbusRespCbData *rsp_cb_data = cb_data;
	dbg("SET_SCA RESPONSE RECEIVED - result:[%d]", result);

	tcore_check_return_assert(rsp_cb_data != NULL);

	telephony_sms_complete_set_sca(rsp_cb_data->interface_object,
		rsp_cb_data->invocation, result);

	tcore_free(rsp_cb_data);
}

static gboolean dtapi_sms_set_sca(TelephonySms *sms,
	GDBusMethodInvocation *invocation,
	GVariant *sca, gpointer user_data)
{
	DbusRespCbData *rsp_cb_data = NULL;
	TcorePlugin *plugin =  user_data;

	TelSmsSca req_sca;

	GVariantIter *iter = NULL;
	GVariant *key_value;
	const gchar *key;

	TelReturn result;

	if (dtapi_check_access_control(invocation, AC_SMS, "w") == FALSE)
		return TRUE;

	if (NULL == sca) {
		dtapi_return_error(invocation, G_DBUS_ERROR_FAILED, "Invalid Parameter");
		return TRUE;
	}

	memset(&req_sca, 0x0, sizeof(TelSmsSca));

	g_variant_get(sca, "a{sv}", &iter);
	while (g_variant_iter_loop(iter, "{sv}", &key, &key_value)) {
		if (g_strcmp0(key, "ton") == 0) {
			req_sca.ton = g_variant_get_byte(key_value);
		}
		else if (g_strcmp0(key, "npi") == 0) {
			req_sca.npi = g_variant_get_byte(key_value);
		}
		else if (g_strcmp0(key, "number") == 0) {
			g_strlcpy(req_sca.number,
				g_variant_get_string(key_value, NULL),
				TEL_SMS_SCA_LEN_MAX + 1);
		}
	}
	g_variant_iter_free(iter);

	rsp_cb_data = dtapi_create_resp_cb_data(sms, invocation, NULL, 0);

	result = tcore_plugin_dispatch_request(plugin, TRUE,
		TCORE_COMMAND_SMS_SET_SCA,
		&req_sca, sizeof(TelSmsSca),
		on_response_dtapi_sms_set_sca, rsp_cb_data);
	if (result != TEL_RETURN_SUCCESS) {
		dtapi_return_error(invocation, G_DBUS_ERROR_FAILED, "Dispatch Failed");
		tcore_free(rsp_cb_data);
	}

	return TRUE;
}

static void on_response_dtapi_sms_get_sca(gint result,
	const void *data, void *cb_data)
{
	DbusRespCbData *rsp_cb_data = cb_data;
	GVariant *sca = NULL;
	GVariantBuilder sca_builder;

	dbg("GET SCA RESPONSE RECEIVED- result:[%d]", result);

	tcore_check_return_assert(rsp_cb_data != NULL);

	g_variant_builder_init(&sca_builder, G_VARIANT_TYPE("a{sv}"));
	if (TEL_SMS_RESULT_SUCCESS == result) {
		const TelSmsSca *sca_req = data;
		tcore_check_return_assert(sca_req != NULL);

		g_variant_builder_add(&sca_builder, "{sv}",
			"ton", g_variant_new_byte(sca_req->ton));
		g_variant_builder_add(&sca_builder, "{sv}",
			"npi", g_variant_new_byte(sca_req->npi));
		g_variant_builder_add(&sca_builder, "{sv}",
			"number", g_variant_new_string(sca_req->number));
	}
	sca = g_variant_builder_end(&sca_builder);

	telephony_sms_complete_get_sca(rsp_cb_data->interface_object,
		rsp_cb_data->invocation, result, sca);

	tcore_free(rsp_cb_data);
}

static gboolean dtapi_sms_get_sca(TelephonySms *sms,
	GDBusMethodInvocation *invocation, gpointer user_data)
{
	DbusRespCbData *rsp_cb_data = NULL;
	TcorePlugin *plugin =  user_data;
	TelReturn result;

	if (dtapi_check_access_control(invocation, AC_SMS, "r") == FALSE)
		return TRUE;

	rsp_cb_data = dtapi_create_resp_cb_data(sms, invocation, NULL, 0);

	result = tcore_plugin_dispatch_request(plugin, TRUE,
		TCORE_COMMAND_SMS_GET_SCA,
		NULL, 0,
		on_response_dtapi_sms_get_sca, rsp_cb_data);
	if (result != TEL_RETURN_SUCCESS) {
		dtapi_return_error(invocation, G_DBUS_ERROR_FAILED, "Dispatch Failed");
		tcore_free(rsp_cb_data);
	}

	return TRUE;
}

static void on_response_dtapi_sms_set_memory_status(gint result,
	const void *data, void *cb_data)
{
	DbusRespCbData *rsp_cb_data = cb_data;
	dbg("SET_MEMORY STATUS RESPONSE RECEIVED - result:[%d]", result);

	tcore_check_return_assert(rsp_cb_data != NULL);

	telephony_sms_complete_set_memory_status(rsp_cb_data->interface_object,
		rsp_cb_data->invocation, result);

	tcore_free(rsp_cb_data);
}

static gboolean dtapi_sms_set_memory_status(TelephonySms *sms,
	GDBusMethodInvocation *invocation,
	gboolean available, gpointer user_data)
{
	DbusRespCbData *rsp_cb_data = NULL;
	TcorePlugin *plugin =  user_data;
	TelReturn result;

	if (dtapi_check_access_control(invocation, AC_SMS, "w") == FALSE)
		return TRUE;

	rsp_cb_data = dtapi_create_resp_cb_data(sms, invocation, NULL, 0);

	result = tcore_plugin_dispatch_request(plugin, TRUE,
		TCORE_COMMAND_SMS_SET_MEMORY_STATUS,
		&available, sizeof(gboolean),
		on_response_dtapi_sms_set_memory_status, rsp_cb_data);
	if (result != TEL_RETURN_SUCCESS) {
		dtapi_return_error(invocation, G_DBUS_ERROR_FAILED, "Dispatch Failed");
		tcore_free(rsp_cb_data);
	}

	return TRUE;
}

static void on_response_dtapi_sms_set_message_status(gint result,
	const void *data, void *cb_data)
{
	DbusRespCbData *rsp_cb_data = cb_data;
	dbg("SET_MESSAGE STATUS RESPONSE RECEIVED - result:[%d]", result);

	tcore_check_return_assert(rsp_cb_data != NULL);

	telephony_sms_complete_set_message_status(rsp_cb_data->interface_object,
		rsp_cb_data->invocation, result);

	tcore_free(rsp_cb_data);
}

static gboolean dtapi_sms_set_message_status(TelephonySms *sms,
	GDBusMethodInvocation *invocation,
	guint index, gint status, gpointer user_data)
{
	DbusRespCbData *rsp_cb_data = NULL;
	TcorePlugin *plugin =  user_data;
	TelReturn result;
	TelSmsStatusInfo info;

	if (dtapi_check_access_control(invocation, AC_SMS, "w") == FALSE)
		return TRUE;

	memset(&info, 0x0, sizeof(TelSmsStatusInfo));
	info.index = index;
	info.status = status;

	rsp_cb_data = dtapi_create_resp_cb_data(sms, invocation, NULL, 0);

	result = tcore_plugin_dispatch_request(plugin, TRUE,
		TCORE_COMMAND_SMS_SET_MEMORY_STATUS,
		&info, sizeof(TelSmsStatusInfo),
		on_response_dtapi_sms_set_message_status, rsp_cb_data);
	if (result != TEL_RETURN_SUCCESS) {
		dtapi_return_error(invocation, G_DBUS_ERROR_FAILED, "Dispatch Failed");
		tcore_free(rsp_cb_data);
	}

	return TRUE;
}

gboolean dtapi_setup_sms_interface(TelephonyObjectSkeleton *object,
	TcorePlugin *plugin)
{
	TelephonySms *sms;

	sms = telephony_sms_skeleton_new();
	telephony_object_skeleton_set_sms(object, sms);
	g_object_unref(sms);

	dbg("sms: [%p]", sms);

	g_signal_connect(sms,
		"handle-send",
		G_CALLBACK (dtapi_sms_send), plugin);
	g_signal_connect(sms,
		"handle-read-in-sim",
		G_CALLBACK (dtapi_sms_read_in_sim), plugin);
	g_signal_connect(sms,
		"handle-write-in-sim",
		G_CALLBACK (dtapi_sms_write_in_sim), plugin);
	g_signal_connect(sms,
		"handle-delete-in-sim",
		G_CALLBACK (dtapi_sms_delete_in_sim), plugin);
	g_signal_connect(sms,
		"handle-get-count",
		G_CALLBACK (dtapi_sms_get_count), plugin);
	g_signal_connect(sms,
		"handle-set-cb-config",
		G_CALLBACK (dtapi_sms_set_cb_config), plugin);
	g_signal_connect(sms,
		"handle-get-cb-config",
		G_CALLBACK (dtapi_sms_get_cb_config), plugin);
	g_signal_connect(sms,
		"handle-get-parameters",
		G_CALLBACK (dtapi_sms_get_parameters), plugin);
	g_signal_connect(sms,
		"handle-set-parameters",
		G_CALLBACK (dtapi_sms_set_parameters), plugin);
	g_signal_connect(sms,
		"handle-send-deliver-report",
		G_CALLBACK (dtapi_sms_send_deliver_report), plugin);
	g_signal_connect(sms,
		"handle-set-sca",
		G_CALLBACK (dtapi_sms_set_sca), plugin);
	g_signal_connect(sms,
		"handle-get-sca",
		G_CALLBACK (dtapi_sms_get_sca), plugin);
	g_signal_connect(sms,
		"handle-set-memory-status",
		G_CALLBACK (dtapi_sms_set_memory_status), plugin);
	g_signal_connect(sms,
		"handle-set-message-status",
		G_CALLBACK (dtapi_sms_set_message_status), plugin);

	/* Initialize D-Bus Properties */
	telephony_sms_set_sim_memory_status(sms, FALSE);
	telephony_sms_set_init_status(sms, FALSE);

	return TRUE;
}

gboolean dtapi_handle_sms_notification(TelephonyObjectSkeleton *object,
	TcorePlugin *plugin, TcoreNotification command,
	guint data_len, const void *data)
{
	TelephonySms *sms;

	tcore_check_return_value_assert(NULL != object, FALSE);

	sms = telephony_object_peek_sms(TELEPHONY_OBJECT(object));
	dbg("sms: [%p]", sms);

	switch (command) {
	case TCORE_NOTIFICATION_SMS_INCOM_MSG: {
		const TelSmsDatapackageInfo *incoming_msg = data;
		GVariant *sca = NULL;
		GVariantBuilder sca_builder;
		gchar *tpdu = NULL;

		tcore_check_return_value_assert(NULL != incoming_msg, FALSE);

		dbg("Incoming Message Notification RECEIVED");

		dbg("TON: [%d] NPI: [%d] SCA: [%s] TPDU Length: [%d]",
			incoming_msg->sca.ton, incoming_msg->sca.npi,
			incoming_msg->sca.number, incoming_msg->tpdu_length);
		g_variant_builder_init(&sca_builder, G_VARIANT_TYPE("a{sv}"));
		g_variant_builder_add(&sca_builder, "{sv}",
			"ton", g_variant_new_byte(incoming_msg->sca.ton));
		g_variant_builder_add(&sca_builder, "{sv}",
			"npi", g_variant_new_byte(incoming_msg->sca.npi));
		g_variant_builder_add(&sca_builder, "{sv}",
			"number", g_variant_new_string(incoming_msg->sca.number));
		sca = g_variant_builder_end(&sca_builder);

		tpdu = g_base64_encode(&(incoming_msg->tpdu[0]), incoming_msg->tpdu_length);

		telephony_sms_emit_incoming_msg(sms, sca, tpdu);
		dbg("Emitted incoming Message");

		tcore_free(tpdu);
		dbg("Exit");
	}
	break;

	case TCORE_NOTIFICATION_SMS_CB_INCOM_MSG: {
		gchar *cb_data = NULL;
		const TelSmsCbMsgInfo *cb_noti = data;

		tcore_check_return_value_assert(NULL != cb_noti, FALSE);

		dbg("Incoming CELL BROADCAST Message Notification RECEIVED");

		cb_data = g_base64_encode(cb_noti->cb_data, cb_noti->length);

		telephony_sms_emit_incoming_cb_msg(sms, cb_noti->cb_type, cb_data);
		tcore_free(cb_data);
	}
	break;

	case TCORE_NOTIFICATION_SMS_ETWS_INCOM_MSG: {
		gchar *etws_data = NULL;
		const TelSmsEtwsMsgInfo *etws_noti = data;

		tcore_check_return_value_assert(NULL != etws_noti, FALSE);

		dbg("ETWS Message Notification RECEIVED");

		etws_data = g_base64_encode(etws_noti->etws_data, etws_noti->length);

		telephony_sms_emit_incoming_etws_msg(sms, etws_noti->etws_type, etws_data);
		tcore_free(etws_data);
	}
	break;

	case TCORE_NOTIFICATION_SMS_MEMORY_STATUS: {
		tcore_check_return_value_assert(NULL != data, FALSE);

		const gboolean memory_status = *(gboolean *)data;
		dbg("Memory Status Notification RECEIVED");

		/* Update Property */
		telephony_sms_set_sim_memory_status(sms, memory_status);
	}
	break;

	case TCORE_NOTIFICATION_SMS_DEVICE_READY: {
		tcore_check_return_value_assert(NULL != data, FALSE);

		const gboolean init_status = *(gboolean *)data;
		dbg("Device Ready Notification RECEIVED");

		/* Update Property */
		telephony_sms_set_init_status(sms, init_status);
	}
	break;

	default:
		err("Unsupported Notification [0x%x]", command);
	break;
	}

	return TRUE;
}
