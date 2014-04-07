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

#include "dtapi_sim.h"
#include "dtapi_util.h"

#include <plugin.h>

#define AC_SIM		"telephony_framework::api_sim"

static void on_response_dtapi_sim_get_imsi(gint result,
	const void *data, void *cb_data)
{
	DbusRespCbData *rsp_cb_data = cb_data;
	const TelSimImsiInfo *imsi = data;

	tcore_check_return_assert(NULL != rsp_cb_data);

	dbg("result[%d]", result);

	if (result != TEL_SIM_RESULT_SUCCESS) {
		telephony_sim_complete_get_imsi(rsp_cb_data->interface_object,
			rsp_cb_data->invocation, result, NULL, NULL, NULL);
	} else {
		tcore_check_return_assert(NULL != imsi);
		dbg("mcc[%s] mnc[%s] msin[%s]", imsi->mcc, imsi->mnc, imsi->msin);

		/* Update Properties */
		telephony_sim_set_mcc(rsp_cb_data->interface_object, imsi->mcc);
		telephony_sim_set_mnc(rsp_cb_data->interface_object, imsi->mnc);
		telephony_sim_set_msin(rsp_cb_data->interface_object, imsi->msin);

		telephony_sim_complete_get_imsi(rsp_cb_data->interface_object,
			rsp_cb_data->invocation, result,
			imsi->mcc, imsi->mnc, imsi->msin);
	}
	tcore_free(rsp_cb_data);
}

static gboolean dtapi_sim_get_imsi(TelephonySim *sim,
	GDBusMethodInvocation *invocation, gpointer user_data)
{
	TelReturn result;
	TcorePlugin *plugin = user_data;
	DbusRespCbData *rsp_cb_data;

	if (dtapi_check_access_control(invocation, AC_SIM, "r") == FALSE)
		return TRUE;

	rsp_cb_data = dtapi_create_resp_cb_data(sim, invocation, NULL, 0);

	result = tcore_plugin_dispatch_request(plugin, TRUE, TCORE_COMMAND_SIM_GET_IMSI, NULL, 0,
			on_response_dtapi_sim_get_imsi, rsp_cb_data);
	if (result != TEL_RETURN_SUCCESS){
		dtapi_return_error(invocation, G_DBUS_ERROR_FAILED, "Dispatch Failed");
		tcore_free(rsp_cb_data);
	}

	return TRUE;
}

static void on_response_dtapi_sim_get_ecc(gint result,
	const void *data, void *cb_data)
{
	DbusRespCbData *rsp_cb_data = cb_data;
	const TelSimEccList *sim_ecc_list = data;
	GVariant *ecc_list;
	GVariantBuilder variant_builder;
	guint count = 0;

	tcore_check_return_assert(NULL != rsp_cb_data);

	dbg("result: [%d] ", result);

	g_variant_builder_init(&variant_builder, G_VARIANT_TYPE("aa{sv}"));
	if (result == TEL_SIM_RESULT_SUCCESS) {
		tcore_check_return_assert(NULL != sim_ecc_list);

		dbg("ECC List count: [%d]", sim_ecc_list->count);
		for (count = 0; count < sim_ecc_list->count; count++) {
			g_variant_builder_open(&variant_builder, G_VARIANT_TYPE("a{sv}"));

			g_variant_builder_add(&variant_builder, "{sv}",
				"number", g_variant_new_string(sim_ecc_list->list[count].number));
			g_variant_builder_add(&variant_builder, "{sv}",
				"name", g_variant_new_string(sim_ecc_list->list[count].name));
			g_variant_builder_add(&variant_builder, "{sv}",
				"category", g_variant_new_int32(sim_ecc_list->list[count].category));

			g_variant_builder_close(&variant_builder);

			dbg("[%d] : Number: [%s] Name: [%s] Category: [%d]", count,
				sim_ecc_list->list[count].number,
				sim_ecc_list->list[count].name,
				sim_ecc_list->list[count].category);
		}
	}
	ecc_list = g_variant_builder_end(&variant_builder);

	telephony_sim_complete_get_ecc(rsp_cb_data->interface_object,
		rsp_cb_data->invocation, result, count, ecc_list);

	g_variant_unref(ecc_list);

	tcore_free(rsp_cb_data);
}

static gboolean dtapi_sim_get_ecc(TelephonySim *sim,
	GDBusMethodInvocation *invocation, gpointer user_data)
{
	TelReturn result;
	TcorePlugin *plugin = user_data;
	DbusRespCbData *rsp_cb_data;
	/* TBD : ECC list can be cached in sim coreobject and can be used instead of
	  * sending request to modem plugin
	  */

	if (dtapi_check_access_control(invocation, AC_SIM, "r") == FALSE)
		return TRUE;

	rsp_cb_data = dtapi_create_resp_cb_data(sim, invocation, NULL, 0);

	result = tcore_plugin_dispatch_request(plugin, TRUE,
		TCORE_COMMAND_SIM_GET_ECC, NULL, 0,
		on_response_dtapi_sim_get_ecc, rsp_cb_data);
	if (result != TEL_RETURN_SUCCESS){
		dtapi_return_error(invocation, G_DBUS_ERROR_FAILED, "Dispatch Failed");
		tcore_free(rsp_cb_data);
	}

	return TRUE;
}

static void on_response_dtapi_sim_get_iccid(gint result,
	const void *data, void *cb_data)
{
	DbusRespCbData *rsp_cb_data = cb_data;
	const gchar *iccid = NULL;

	tcore_check_return_assert(NULL != rsp_cb_data);

	dbg("result[%d] ", result);

	if (result == TEL_SIM_RESULT_SUCCESS) {
		iccid = data;
		dbg("iccid[%s]", iccid);
	}
	telephony_sim_complete_get_iccid(rsp_cb_data->interface_object,
		rsp_cb_data->invocation, result, iccid);
	tcore_free(rsp_cb_data);
}

static gboolean dtapi_sim_get_iccid(TelephonySim *sim,
	GDBusMethodInvocation *invocation, gpointer user_data)
{
	TelReturn result;
	TcorePlugin *plugin = user_data;
	DbusRespCbData *rsp_cb_data;
	/* TBD : ICCID can be cached in sim coreobject and can be used instead of
	  * sending request to modem plugin
	  */

	if (dtapi_check_access_control(invocation, AC_SIM, "r") == FALSE)
		return TRUE;

	rsp_cb_data = dtapi_create_resp_cb_data(sim, invocation, NULL, 0);
	result = tcore_plugin_dispatch_request(plugin, TRUE,
		TCORE_COMMAND_SIM_GET_ICCID, NULL, 0,
		on_response_dtapi_sim_get_iccid, rsp_cb_data);
	if (result != TEL_RETURN_SUCCESS){
		dtapi_return_error(invocation, G_DBUS_ERROR_FAILED, "Dispatch Failed");
		tcore_free(rsp_cb_data);
	}

	return TRUE;
}

static void
on_response_dtapi_sim_get_language(gint result,
	const void *data, void *cb_data)
{
	DbusRespCbData *rsp_cb_data = cb_data;
	TelSimLanguagePreferenceCode language = TEL_SIM_LP_LANG_UNSPECIFIED;

	tcore_check_return_assert(NULL != rsp_cb_data);

	dbg("result[%d] ", result);

	if (result == TEL_SIM_RESULT_SUCCESS) {
		tcore_check_return_assert(NULL != data);
		language = *(TelSimLanguagePreferenceCode *)data;
		dbg("language[%d]", language);
	}
	telephony_sim_complete_get_language(rsp_cb_data->interface_object,
							rsp_cb_data->invocation, result, language);
	tcore_free(rsp_cb_data);
}

static gboolean dtapi_sim_get_language(TelephonySim *sim,
	GDBusMethodInvocation *invocation, gpointer user_data)
{
	TelReturn result;
	TcorePlugin *plugin = user_data;
	DbusRespCbData *rsp_cb_data;

	if (dtapi_check_access_control(invocation, AC_SIM, "r") == FALSE)
		return TRUE;

	rsp_cb_data = dtapi_create_resp_cb_data(sim, invocation, NULL, 0);

	result = tcore_plugin_dispatch_request(plugin, TRUE, TCORE_COMMAND_SIM_GET_LANGUAGE, NULL, 0,
		on_response_dtapi_sim_get_language, rsp_cb_data);
	if (result != TEL_RETURN_SUCCESS){
		dtapi_return_error(invocation, G_DBUS_ERROR_FAILED, "Dispatch Failed");
		tcore_free(rsp_cb_data);
	}

	return TRUE;
}

static void
on_response_dtapi_sim_set_language(gint result,
	const void *data, void *cb_data)
{
	DbusRespCbData *rsp_cb_data = cb_data;

	tcore_check_return_assert(NULL != rsp_cb_data);

	dbg("result[%d]", result);

	telephony_sim_complete_set_language(rsp_cb_data->interface_object,
							rsp_cb_data->invocation, result);
	tcore_free(rsp_cb_data);
}

static gboolean dtapi_sim_set_language(TelephonySim *sim,
	GDBusMethodInvocation *invocation,
	gint language, gpointer user_data)
{
	TelReturn result;
	TcorePlugin *plugin = user_data;
	DbusRespCbData *rsp_cb_data;
	TelSimLanguagePreferenceCode language_preference;

	if (dtapi_check_access_control(invocation, AC_SIM, "w") == FALSE)
		return TRUE;

	rsp_cb_data = dtapi_create_resp_cb_data(sim, invocation, NULL, 0);

	language_preference = language;
	result = tcore_plugin_dispatch_request(plugin, TRUE,
		TCORE_COMMAND_SIM_SET_LANGUAGE,
		&language_preference, sizeof(TelSimLanguagePreferenceCode),
		on_response_dtapi_sim_set_language, rsp_cb_data);
	if (result != TEL_RETURN_SUCCESS){
		dtapi_return_error(invocation, G_DBUS_ERROR_FAILED, "Dispatch Failed");
		tcore_free(rsp_cb_data);
	}

	return TRUE;
}

static void on_response_dtapi_sim_get_call_forwarding_info(gint result,
	const void *data, void *cb_data)
{
	DbusRespCbData *rsp_cb_data = cb_data;
	const TelSimCfisList *cf_list = data;
	guint count = 0;
	GVariant *list;
	GVariantBuilder variant_builder;

	tcore_check_return_assert(NULL != rsp_cb_data);

	dbg("result[%d] ", result);

	g_variant_builder_init(&variant_builder, G_VARIANT_TYPE("aa{sv}"));
	if (result == TEL_SIM_RESULT_SUCCESS) {
		gpointer indication;
		GVariant *indication_var;
		tcore_check_return_assert(cf_list != NULL);

		dbg("Call forwarding List count: [%d]", cf_list->profile_count);
		for (count = 0; count < cf_list->profile_count; count++) {
			g_variant_builder_open(&variant_builder, G_VARIANT_TYPE("a{sv}"));

			g_variant_builder_add(&variant_builder, "{sv}",
				"profile_id", g_variant_new_byte(cf_list->cf[count].profile_id));

			indication = (gpointer)cf_list->cf[count].indication;
			indication_var = g_variant_new_from_data(G_VARIANT_TYPE("ai"),
				indication, sizeof(cf_list->cf[count].indication),
				TRUE, NULL, NULL);
			g_variant_builder_add(&variant_builder, "{sv}",
				"indication", indication_var);

			g_variant_builder_close(&variant_builder);

			dbg("[%d] : Profile ID: [%s]", count,
				cf_list->cf[count].profile_id);
		}
	}
	list = g_variant_builder_end(&variant_builder);

	telephony_sim_complete_get_call_forwarding_info(rsp_cb_data->interface_object,
		rsp_cb_data->invocation, result, count, list);
	g_variant_unref(list);

	tcore_free(rsp_cb_data);
}

static gboolean dtapi_sim_get_call_forwarding_info(TelephonySim *sim,
	GDBusMethodInvocation *invocation, gpointer user_data)
{
	TelReturn result;
	TcorePlugin *plugin = user_data;
	DbusRespCbData *rsp_cb_data;

	if (dtapi_check_access_control(invocation, AC_SIM, "r") == FALSE)
		return TRUE;

	rsp_cb_data = dtapi_create_resp_cb_data(sim, invocation, NULL, 0);

	result = tcore_plugin_dispatch_request(plugin, TRUE, TCORE_COMMAND_SIM_GET_CALL_FORWARDING_INFO, NULL, 0,
			on_response_dtapi_sim_get_call_forwarding_info, rsp_cb_data);
	if (result != TEL_RETURN_SUCCESS){
		dtapi_return_error(invocation, G_DBUS_ERROR_FAILED, "Dispatch Failed");
		tcore_free(rsp_cb_data);
	}

	return TRUE;
}

static void on_response_dtapi_sim_get_message_waiting_info(gint result,
	const void *data, void *cb_data)
{
	DbusRespCbData *rsp_cb_data = cb_data;
	const TelSimMwisList *mw_list = data;

	GVariantBuilder variant_builder;
	GVariant *list = NULL;

	tcore_check_return_assert(NULL != rsp_cb_data);

	dbg("result[%d]", result);

	g_variant_builder_init(&variant_builder, G_VARIANT_TYPE("aa{sv}"));
	if (result == TEL_SIM_RESULT_SUCCESS) {
		GVariantBuilder variant_builder2;
		GVariant *list2 = NULL;
		guint count, count2;

		tcore_check_return_assert(mw_list != NULL);

		dbg("Message waiting count: [%d]", mw_list->profile_count);

		for (count = 0; count < mw_list->profile_count; count++) {
			g_variant_builder_open(&variant_builder, G_VARIANT_TYPE("a{sv}"));

			g_variant_builder_add(&variant_builder, "{sv}",
				"profile_id", g_variant_new_byte(mw_list->mw[count].profile_id));

			g_variant_builder_add(&variant_builder, "{sv}",
				"count_valid", g_variant_new_boolean(mw_list->mw[count].count_valid));

			dbg("[%d] Profile ID: [%d] Count valid: [%s]", count,
				mw_list->mw[count].profile_id,
				mw_list->mw[count].count_valid ? "YES" : "NO");

			g_variant_builder_init(&variant_builder2, G_VARIANT_TYPE("aa{sv}"));
			for (count2 = 0 ; count2 < TEL_SIM_MAILBOX_TYPE_MAX ; count2++) {
				g_variant_builder_open(&variant_builder2, G_VARIANT_TYPE("a{sv}"));

				g_variant_builder_add(&variant_builder2, "{sv}", "indication",
					g_variant_new_boolean(mw_list->mw[count].msg_waiting[count2].indication));

				g_variant_builder_add(&variant_builder2, "{sv}", "count",
					g_variant_new_byte(mw_list->mw[count].msg_waiting[count2].count));

				g_variant_builder_close(&variant_builder2);
			}
			list2 = g_variant_builder_end(&variant_builder2);

			g_variant_builder_add(&variant_builder, "{sv}",
				"msg_waiting", list2);

			g_variant_builder_close(&variant_builder);
		}
	}
	list = g_variant_builder_end(&variant_builder);

	telephony_sim_complete_get_message_waiting_info(rsp_cb_data->interface_object,
		rsp_cb_data->invocation, result, mw_list->profile_count, list);

	tcore_free(rsp_cb_data);
}

static gboolean dtapi_sim_get_message_waiting_info(TelephonySim *sim,
	GDBusMethodInvocation *invocation, gpointer user_data)
{
	TelReturn result;
	TcorePlugin *plugin = user_data;
	DbusRespCbData *rsp_cb_data;

	if (dtapi_check_access_control(invocation, AC_SIM, "r") == FALSE)
		return TRUE;

	rsp_cb_data = dtapi_create_resp_cb_data(sim, invocation, NULL, 0);

	result = tcore_plugin_dispatch_request(plugin, TRUE, TCORE_COMMAND_SIM_GET_MESSAGE_WAITING_INFO, NULL, 0,
		on_response_dtapi_sim_get_message_waiting_info, rsp_cb_data);
	if (result != TEL_RETURN_SUCCESS){
		dtapi_return_error(invocation, G_DBUS_ERROR_FAILED, "Dispatch Failed");
		tcore_free(rsp_cb_data);
	}

	return TRUE;
}

static void on_response_dtapi_sim_set_message_waiting_info(gint result,
	const void *data, void *cb_data)
{
	DbusRespCbData *rsp_cb_data = cb_data;

	tcore_check_return_assert(NULL != rsp_cb_data);

	dbg("result[%d]", result);

	telephony_sim_complete_set_message_waiting_info(rsp_cb_data->interface_object,
		rsp_cb_data->invocation, result);
	tcore_free(rsp_cb_data);
}

static gboolean dtapi_sim_set_message_waiting_info(TelephonySim *sim,
	GDBusMethodInvocation *invocation,
	guchar profile_id, gboolean count_valid,
	GVariant *mw, gpointer user_data)
{
	TelReturn result;
	TcorePlugin *plugin = user_data;
	TelSimMwis req_data = {0,};
	DbusRespCbData *rsp_cb_data;
	GVariantIter *iter = NULL;
	unsigned int i = 0;

	if (dtapi_check_access_control(invocation, AC_SIM, "w") == FALSE)
		return TRUE;

	req_data.profile_id = profile_id;
	req_data.count_valid = count_valid;

	rsp_cb_data = dtapi_create_resp_cb_data(sim, invocation, NULL, 0);

	g_variant_get(mw, "a(by)", &iter);
	while (g_variant_iter_next(iter, "(by)",
			&(req_data.msg_waiting[i].indication),
			&(req_data.msg_waiting[i].count))) {
		i++;
	}

	g_variant_iter_free(iter);

	result = tcore_plugin_dispatch_request(plugin,
		TRUE, TCORE_COMMAND_SIM_SET_MESSAGE_WAITING_INFO,
		&req_data, sizeof(TelSimMwis),
		on_response_dtapi_sim_set_message_waiting_info, rsp_cb_data);
	if (result != TEL_RETURN_SUCCESS){
		dtapi_return_error(invocation,
			G_DBUS_ERROR_FAILED, "Dispatch Failed");
		tcore_free(rsp_cb_data);
	}

	return TRUE;
}

static void on_response_dtapi_sim_get_mailbox_info(gint result,
	const void *data, void *cb_data)
{
	DbusRespCbData *rsp_cb_data = cb_data;
	const TelSimMailboxList *mb_list = data;
	guint count = 0;
	GVariant *list;
	GVariantBuilder variant_builder;

	tcore_check_return_assert(NULL != rsp_cb_data);

	dbg("result[%d]", result);

	g_variant_builder_init(&variant_builder, G_VARIANT_TYPE("aa{sv}"));
	if (result == TEL_SIM_RESULT_SUCCESS) {
		GVariant *alpha_id, *number;

		tcore_check_return_assert(mb_list != NULL);
		dbg("Mailbox list count: [%d]", mb_list->count);

		for (count = 0; count < count; count++) {
			g_variant_builder_open(&variant_builder, G_VARIANT_TYPE("a{sv}"));

			g_variant_builder_add(&variant_builder, "{sv}",
				"profile_id", g_variant_new_byte(mb_list->list[count].profile_id));

			g_variant_builder_add(&variant_builder, "{sv}",
				"mb_type", g_variant_new_int32(mb_list->list[count].mb_type));

			g_variant_builder_add(&variant_builder, "{sv}",
				"alpha_id_len", g_variant_new_uint32(mb_list->list[count].alpha_id_len));

			alpha_id = g_variant_new_from_data(G_VARIANT_TYPE("ay"),
				mb_list->list[count].alpha_id, TEL_SIM_ALPHA_ID_LEN_MAX,
				TRUE, NULL, NULL);
			g_variant_builder_add(&variant_builder, "{sv}",
				"alpha_id", alpha_id);

			number = g_variant_new_from_data(G_VARIANT_TYPE("ay"),
				mb_list->list[count].number, TEL_SIM_MBDN_NUM_LEN_MAX,
				TRUE, NULL, NULL);
			g_variant_builder_add(&variant_builder, "{sv}",
				"number", number);

			g_variant_builder_close(&variant_builder);
		}
	}
	list = g_variant_builder_end(&variant_builder);

	telephony_sim_complete_get_mailbox_info(rsp_cb_data->interface_object,
		rsp_cb_data->invocation, result, mb_list->alpha_id_max_len, mb_list->count, list);
	g_variant_unref(list);

	tcore_free(rsp_cb_data);
}

static gboolean dtapi_sim_get_mailbox_info(TelephonySim *sim,
	GDBusMethodInvocation *invocation, gpointer user_data)
{
	TelReturn result;
	TcorePlugin *plugin = user_data;
	DbusRespCbData *rsp_cb_data;

	if (dtapi_check_access_control(invocation, AC_SIM, "r") == FALSE)
		return TRUE;

	rsp_cb_data = dtapi_create_resp_cb_data(sim, invocation, NULL, 0);

	result = tcore_plugin_dispatch_request(plugin, TRUE, TCORE_COMMAND_SIM_GET_MAILBOX_INFO, NULL, 0,
			on_response_dtapi_sim_get_mailbox_info, rsp_cb_data);
	if (result != TEL_RETURN_SUCCESS) {
		dtapi_return_error(invocation, G_DBUS_ERROR_FAILED, "Dispatch Failed");
		tcore_free(rsp_cb_data);
	}

	return TRUE;
}

static void on_response_dtapi_sim_set_mailbox_info(gint result,
	const void *data, void *cb_data)
{
	DbusRespCbData *rsp_cb_data = cb_data;

	tcore_check_return_assert(NULL != rsp_cb_data);

	dbg("result[%d]", result);

	telephony_sim_complete_set_mailbox_info(rsp_cb_data->interface_object,
		rsp_cb_data->invocation, result);
	tcore_free(rsp_cb_data);
}

static gboolean dtapi_sim_set_mailbox_info(TelephonySim *sim,
	GDBusMethodInvocation *invocation,
	guchar profile_id, gint mb_type,
	gint alpha_id_len, const gchar *alpha_id,
	const gchar *num, gpointer user_data)
{
	TelReturn result;
	TcorePlugin *plugin = user_data;
	TelSimMailBoxNumber req_data= {0,};
	DbusRespCbData *rsp_cb_data;

	if (dtapi_check_access_control(invocation, AC_SIM, "w") == FALSE)
		return TRUE;

	rsp_cb_data = dtapi_create_resp_cb_data(sim, invocation, NULL, 0);

	req_data.profile_id = profile_id;
	req_data.mb_type = mb_type;
	req_data.alpha_id_len = alpha_id_len;
	memcpy(req_data.alpha_id, alpha_id, alpha_id_len);
	memcpy(req_data.number, num, strlen(num));

	result = tcore_plugin_dispatch_request(plugin, TRUE,
		TCORE_COMMAND_SIM_SET_MAILBOX_INFO,
		&req_data, sizeof(TelSimMailBoxNumber),
		on_response_dtapi_sim_set_mailbox_info, rsp_cb_data);
	if (result != TEL_RETURN_SUCCESS) {
		dtapi_return_error(invocation, G_DBUS_ERROR_FAILED, "Dispatch Failed");
		tcore_free(rsp_cb_data);
	}

	return TRUE;
}

static void on_response_dtapi_sim_get_msisdn(gint result,
	const void *data, void *cb_data)
{
	DbusRespCbData *rsp_cb_data = cb_data;
	const TelSimMsisdnList *msisdn_list = data;
	guint count = 0;
	GVariant *list;
	GVariantBuilder variant_builder;

	tcore_check_return_assert(NULL != rsp_cb_data);

	dbg("result[%d]", result);

	g_variant_builder_init(&variant_builder, G_VARIANT_TYPE("aa{sv}"));
	if (result == TEL_SIM_RESULT_SUCCESS) {
		tcore_check_return_assert(msisdn_list != NULL);

		dbg("MSISDN List count: [%d]", msisdn_list->count);
		for (count = 0; count < msisdn_list->count; count++) {
			g_variant_builder_open(&variant_builder, G_VARIANT_TYPE("a{sv}"));

			g_variant_builder_add(&variant_builder, "{sv}",
				"alpha_id", g_variant_new_string(msisdn_list->list[count].alpha_id));
			g_variant_builder_add(&variant_builder, "{sv}",
				"num", g_variant_new_string(msisdn_list->list[count].num));

			g_variant_builder_close(&variant_builder);

			dbg("[%d] : Aplha ID: [%s] Number: [%s]", count,
				msisdn_list->list[count].alpha_id,
				msisdn_list->list[count].num);
		}
	}
	list = g_variant_builder_end(&variant_builder);

	telephony_sim_complete_get_msisdn(rsp_cb_data->interface_object,
		rsp_cb_data->invocation, result, count, list);
	g_variant_unref(list);

	tcore_free(rsp_cb_data);
}

static gboolean dtapi_sim_get_msisdn(TelephonySim *sim,
	GDBusMethodInvocation *invocation, gpointer user_data)
{
	TelReturn result;
	TcorePlugin *plugin = user_data;
	DbusRespCbData *rsp_cb_data;

	if (dtapi_check_access_control(invocation, AC_SIM, "r") == FALSE)
		return TRUE;

	rsp_cb_data = dtapi_create_resp_cb_data(sim, invocation, NULL, 0);

	result = tcore_plugin_dispatch_request(plugin, TRUE,
		TCORE_COMMAND_SIM_GET_MSISDN,
		NULL, 0,
		on_response_dtapi_sim_get_msisdn, rsp_cb_data);
	if (result != TEL_RETURN_SUCCESS) {
		dtapi_return_error(invocation, G_DBUS_ERROR_FAILED, "Dispatch Failed");
		tcore_free(rsp_cb_data);
	}

	return TRUE;
}

static void on_response_dtapi_sim_get_spn(gint result,
	const void *data, void *cb_data)
{
	DbusRespCbData *rsp_cb_data = cb_data;
	const TelSimSpn *spn = data;

	tcore_check_return_assert(NULL != rsp_cb_data);

	dbg("result[%d] ", result);

	if (result == TEL_SIM_RESULT_SUCCESS) {
		tcore_check_return_assert(NULL != spn);
		dbg("display_condition[%d] spn[%s]", spn->display_condition, spn->spn);
		telephony_sim_complete_get_spn(rsp_cb_data->interface_object,
			rsp_cb_data->invocation, result,
			spn->display_condition, spn->spn);
	} else {
		telephony_sim_complete_get_spn(rsp_cb_data->interface_object,
			rsp_cb_data->invocation, result,
			TEL_SIM_DISP_INVALID, NULL);
	}
	tcore_free(rsp_cb_data);
}

static gboolean dtapi_sim_get_spn(TelephonySim *sim,
	GDBusMethodInvocation *invocation, gpointer user_data)
{
	TelReturn result;
	TcorePlugin *plugin = user_data;
	DbusRespCbData *rsp_cb_data;

	if (dtapi_check_access_control(invocation, AC_SIM, "r") == FALSE)
		return TRUE;

	rsp_cb_data = dtapi_create_resp_cb_data(sim, invocation, NULL, 0);

	result = tcore_plugin_dispatch_request(plugin, TRUE,
		TCORE_COMMAND_SIM_GET_SPN,
		NULL, 0,
		on_response_dtapi_sim_get_spn, rsp_cb_data);
	if (result != TEL_RETURN_SUCCESS) {
		dtapi_return_error(invocation, G_DBUS_ERROR_FAILED, "Dispatch Failed");
		tcore_free(rsp_cb_data);
	}

	return TRUE;
}

static void on_response_dtapi_sim_get_cphs_net_name(gint result,
	const void *data, void *cb_data)
{
	DbusRespCbData *rsp_cb_data = cb_data;
	const TelSimCphsNetName *cphs_net_name = data;

	tcore_check_return_assert(NULL != rsp_cb_data);

	dbg("result[%d] ", result);

	if (result == TEL_SIM_RESULT_SUCCESS) {
		tcore_check_return_assert(NULL != cphs_net_name);
		dbg("full_name[%s] short_name[%s]",
			cphs_net_name->full_name, cphs_net_name->short_name);
		telephony_sim_complete_get_cphs_net_name(rsp_cb_data->interface_object,
			rsp_cb_data->invocation, result,
			cphs_net_name->full_name, cphs_net_name->short_name);
	} else {
		telephony_sim_complete_get_cphs_net_name(rsp_cb_data->interface_object,
			rsp_cb_data->invocation, result, "", "");
	}
	tcore_free(rsp_cb_data);
}

static gboolean dtapi_sim_get_cphs_net_name(TelephonySim *sim,
	GDBusMethodInvocation *invocation, gpointer user_data)
{
	TelReturn result;
	TcorePlugin *plugin = user_data;
	DbusRespCbData *rsp_cb_data;

	if (dtapi_check_access_control(invocation, AC_SIM, "r") == FALSE)
		return TRUE;

	rsp_cb_data = dtapi_create_resp_cb_data(sim, invocation, NULL, 0);

	result = tcore_plugin_dispatch_request(plugin, TRUE,
		TCORE_COMMAND_SIM_GET_CPHS_NET_NAME,
		NULL, 0,
		on_response_dtapi_sim_get_cphs_net_name, rsp_cb_data);
	if (result != TEL_RETURN_SUCCESS) {
		dtapi_return_error(invocation, G_DBUS_ERROR_FAILED, "Dispatch Failed");
		tcore_free(rsp_cb_data);
	}

	return TRUE;
}

static void on_response_dtapi_sim_get_sp_display_info(gint result,
	const void *data, void *cb_data)
{
	DbusRespCbData *rsp_cb_data = cb_data;
	const TelSimSpPlmnList *sp_list = data;
	guint count = 0;
	GVariant *list;
	GVariantBuilder variant_builder;

	tcore_check_return_assert(NULL != rsp_cb_data);

	dbg("result[%d]", result);

	g_variant_builder_init(&variant_builder, G_VARIANT_TYPE("aa{sv}"));
	if (result == TEL_SIM_RESULT_SUCCESS) {
		tcore_check_return_assert(sp_list != NULL);

		dbg("SPN List count: [%d]", sp_list->count);
		for (count = 0; count < sp_list->count; count++) {
			g_variant_builder_open(&variant_builder, G_VARIANT_TYPE("a{sv}"));

			g_variant_builder_add(&variant_builder, "{sv}",
				"index", g_variant_new_uint32(sp_list->list[count].index));
			g_variant_builder_add(&variant_builder, "{sv}",
				"plmn", g_variant_new_string(sp_list->list[count].plmn));

			g_variant_builder_close(&variant_builder);

			dbg("[%d] : Index: [%d] PLMN: [%s]", count,
				sp_list->list[count].index,
				sp_list->list[count].plmn);
		}
	}
	list = g_variant_builder_end(&variant_builder);

	telephony_sim_complete_get_sp_display_info(rsp_cb_data->interface_object,
		rsp_cb_data->invocation, result, count, list);
	g_variant_unref(list);

	tcore_free(rsp_cb_data);
}


static gboolean dtapi_sim_get_sp_display_info(TelephonySim *sim,
	GDBusMethodInvocation *invocation, gpointer user_data)
{
	TelReturn result;
	TcorePlugin *plugin = user_data;
	DbusRespCbData *rsp_cb_data;

	if (dtapi_check_access_control(invocation, AC_SIM, "r") == FALSE)
		return TRUE;

	rsp_cb_data = dtapi_create_resp_cb_data(sim, invocation, NULL, 0);

	result = tcore_plugin_dispatch_request(plugin, TRUE,
		TCORE_COMMAND_SIM_GET_SP_DISPLAY_INFO,
		NULL, 0,
		on_response_dtapi_sim_get_sp_display_info, rsp_cb_data);
	if (result != TEL_RETURN_SUCCESS) {
		dtapi_return_error(invocation, G_DBUS_ERROR_FAILED, "Dispatch Failed");
		tcore_free(rsp_cb_data);
	}

	return TRUE;
}

static void on_response_dtapi_sim_req_authentication(gint result,
	const void *data, void *cb_data)
{
	DbusRespCbData *rsp_cb_data = cb_data;
	const TelSimAuthenticationResponse *req_auth = data;

	tcore_check_return_assert(NULL != rsp_cb_data);

	dbg("result[%d]", result);

	if (result == TEL_SIM_RESULT_SUCCESS) {
		gchar *encoded_resp, *encoded_autn, *encoded_cipher, *encoded_integrity;
		tcore_check_return_assert(NULL != req_auth);

		dbg("auth_type[%d] detailed_result[%d] resp_length[%d] \
		authentication_key_length[%d] cipher_length[%d] integrity_length[%d]",
		req_auth->auth_type, req_auth->detailed_result, req_auth->resp_length,
		req_auth->authentication_key_length, req_auth->cipher_length, req_auth->integrity_length);
		/* Encode Base64 - Resp data */
		encoded_resp = g_base64_encode((const guchar *)req_auth->resp_data,
					req_auth->resp_length);

		/* Encode Base64 - auth data */
		encoded_autn = g_base64_encode((const guchar *)req_auth->authentication_key,
					req_auth->authentication_key_length);

		/* Encode Base64 - cipher data */
		encoded_cipher = g_base64_encode((const guchar *)req_auth->cipher_data,
					req_auth->cipher_length);

		/* Encode Base64 - integrity data */
		encoded_integrity = g_base64_encode((const guchar *)req_auth->integrity_data,
					req_auth->integrity_length);

		telephony_sim_complete_req_authentication(rsp_cb_data->interface_object,
			rsp_cb_data->invocation, result,
			req_auth->auth_type, encoded_resp,
			encoded_autn, encoded_cipher, encoded_integrity);
		/* Free resources */
		tcore_free(encoded_resp);
		tcore_free(encoded_autn);
		tcore_free(encoded_cipher);
		tcore_free(encoded_integrity);
	} else {
		telephony_sim_complete_req_authentication(rsp_cb_data->interface_object,
			rsp_cb_data->invocation, result,
			0, NULL, NULL, NULL, NULL);
	}
	tcore_free(rsp_cb_data);
}

static gboolean dtapi_sim_req_authentication(TelephonySim *sim,
	GDBusMethodInvocation *invocation,
	gint auth_type, const gchar *rand_data,
	const gchar *autn_data, gpointer user_data)
{
	TelReturn result;
	TcorePlugin *plugin = user_data;
	DbusRespCbData *rsp_cb_data;
	guchar *decoded_data;
	TelSimAuthenticationData req_data = {0,};
	gsize decoded_data_len;

	if (dtapi_check_access_control(invocation, AC_SIM, "x") == FALSE)
		return TRUE;

	req_data.auth_type = auth_type;

	/* Decode Base64 - rand data */
	decoded_data = g_base64_decode((const gchar *)rand_data, &decoded_data_len);
	if ((decoded_data_len > TEL_SIM_AUTH_MAX_REQ_DATA_LEN) ||(decoded_data_len == 0)) {
		dtapi_return_error(invocation, G_DBUS_ERROR_FAILED, "Decoding Failed");
		tcore_free(decoded_data);
		return TRUE;
	}

	memcpy(req_data.rand_data, decoded_data, decoded_data_len);
	req_data.rand_length = decoded_data_len;

	/* Free resources */
	tcore_free(decoded_data);

	/* Decode Base64 - auth data */
	decoded_data = g_base64_decode((const gchar *)autn_data, &decoded_data_len);
	if (decoded_data_len > TEL_SIM_AUTH_MAX_REQ_DATA_LEN) {
		dtapi_return_error(invocation, G_DBUS_ERROR_FAILED, "Decoding Failed");
		tcore_free(decoded_data);
		return TRUE;
	}

	if(decoded_data_len)
		memcpy(req_data.autn_data, decoded_data, decoded_data_len);
	req_data.autn_length = decoded_data_len;

	/* Free resources */
	tcore_free(decoded_data);

	rsp_cb_data = dtapi_create_resp_cb_data(sim, invocation, NULL, 0);

	result = tcore_plugin_dispatch_request(plugin, TRUE,
		TCORE_COMMAND_SIM_REQ_AUTHENTICATION,
		&req_data, sizeof(TelSimAuthenticationData),
		on_response_dtapi_sim_req_authentication, rsp_cb_data);
	if (result != TEL_RETURN_SUCCESS) {
		dtapi_return_error(invocation, G_DBUS_ERROR_FAILED, "Dispatch Failed");
		tcore_free(rsp_cb_data);
	}

	return TRUE;
}

static void on_response_dtapi_sim_verify_pins(gint result,
	const void *data, void *cb_data)
{
	DbusRespCbData *rsp_cb_data = cb_data;
	const TelSimSecPinResult *verify_pins = data;

	tcore_check_return_assert(NULL != rsp_cb_data);

	dbg("result[%d]", result);

	if (result == TEL_SIM_RESULT_SUCCESS) {
		tcore_check_return_assert(NULL != verify_pins);
		dbg("pin_type[%d] retry_count[%d]",
			verify_pins->pin_type, verify_pins->retry_count);
		telephony_sim_complete_verify_pins(rsp_cb_data->interface_object,
			rsp_cb_data->invocation, result,
			verify_pins->pin_type, verify_pins->retry_count);
	} else {
		telephony_sim_complete_verify_pins(rsp_cb_data->interface_object,
			rsp_cb_data->invocation, result, 0, 0);
	}
	tcore_free(rsp_cb_data);
}

static gboolean dtapi_sim_verify_pins(TelephonySim *sim,
	GDBusMethodInvocation *invocation,
	gint pin_type, const gchar *pw,
	gpointer user_data)
{
	TelReturn result;
	TcorePlugin *plugin = user_data;
	TelSimSecPinPw req_data = {0,};
	DbusRespCbData *rsp_cb_data;

	if (dtapi_check_access_control(invocation, AC_SIM, "x") == FALSE)
		return TRUE;

	rsp_cb_data = dtapi_create_resp_cb_data(sim, invocation, NULL, 0);

	req_data.pin_type =  pin_type;
	req_data.pw = (char *)pw;

	result = tcore_plugin_dispatch_request(plugin, TRUE,
		TCORE_COMMAND_SIM_VERIFY_PINS,
		&req_data, sizeof(TelSimSecPinPw),
		on_response_dtapi_sim_verify_pins, rsp_cb_data);
	if (result != TEL_RETURN_SUCCESS) {
		dtapi_return_error(invocation, G_DBUS_ERROR_FAILED, "Dispatch Failed");
		tcore_free(rsp_cb_data);
	}

	return TRUE;
}

static void on_response_dtapi_sim_verify_puks(gint result,
	const void *data, void *cb_data)
{
	DbusRespCbData *rsp_cb_data = cb_data;
	const TelSimSecPukResult *verify_puks = data;

	tcore_check_return_assert(NULL != rsp_cb_data);

	dbg("result[%d]", result);

	if (result == TEL_SIM_RESULT_SUCCESS) {
		tcore_check_return_assert(NULL != verify_puks);
		dbg("puk_type[%d] retry_count[%d]",
			verify_puks->puk_type, verify_puks->retry_count);

		telephony_sim_complete_verify_puks(rsp_cb_data->interface_object,
			rsp_cb_data->invocation, result,
			verify_puks->puk_type, verify_puks->retry_count);
	} else {
		telephony_sim_complete_verify_puks(rsp_cb_data->interface_object,
			rsp_cb_data->invocation, result, 0, 0);
	}
	tcore_free(rsp_cb_data);
}

static gboolean dtapi_sim_verify_puks(TelephonySim *sim,
	GDBusMethodInvocation *invocation,
	gint puk_type, const gchar *puk_pw,
	const gchar *new_pin_pw, gpointer user_data)
{
	TelReturn result;
	TcorePlugin *plugin = user_data;
	TelSimSecPukPw req_data = {0,};
	DbusRespCbData *rsp_cb_data;

	if (dtapi_check_access_control(invocation, AC_SIM, "x") == FALSE)
		return TRUE;

	rsp_cb_data = dtapi_create_resp_cb_data(sim, invocation, NULL, 0);

	req_data.puk_type =  puk_type;
	req_data.puk_pw = (char *)puk_pw;
	req_data.new_pin_pw = (char *)new_pin_pw;

	result = tcore_plugin_dispatch_request(plugin, TRUE,
		TCORE_COMMAND_SIM_VERIFY_PUKS,
		&req_data, sizeof(TelSimSecPukPw),
		on_response_dtapi_sim_verify_puks, rsp_cb_data);
	if (result != TEL_RETURN_SUCCESS) {
		dtapi_return_error(invocation, G_DBUS_ERROR_FAILED, "Dispatch Failed");
		tcore_free(rsp_cb_data);
	}

	return TRUE;
}

static void on_response_dtapi_sim_change_pins(gint result,
	const void *data, void *cb_data)
{
	DbusRespCbData *rsp_cb_data = cb_data;
	const TelSimSecPinResult *change_pins = data;

	tcore_check_return_assert(NULL != rsp_cb_data);

	dbg("result[%d]", result);

	if (result == TEL_SIM_RESULT_SUCCESS) {
		tcore_check_return_assert(NULL != change_pins);
		dbg("pin_type[%d] retry_count[%d]",
			change_pins->pin_type, change_pins->retry_count);
		telephony_sim_complete_change_pins(rsp_cb_data->interface_object,
			rsp_cb_data->invocation, result,
			change_pins->pin_type, change_pins->retry_count);
	} else {
		telephony_sim_complete_change_pins(rsp_cb_data->interface_object,
							rsp_cb_data->invocation, result, 0, 0);
	}
	tcore_free(rsp_cb_data);
}

static gboolean dtapi_sim_change_pins(TelephonySim *sim,
	GDBusMethodInvocation *invocation,
	gint pin_type, const gchar *old_pw,
	const gchar *new_pw, gpointer user_data)
{
	TelReturn result;
	TcorePlugin *plugin = user_data;
	DbusRespCbData *rsp_cb_data;
	TelSimSecChangePinPw req_data = {0,};

	if (dtapi_check_access_control(invocation, AC_SIM, "x") == FALSE)
		return TRUE;

	rsp_cb_data = dtapi_create_resp_cb_data(sim, invocation, NULL, 0);

	req_data.pin_type =  pin_type;
	req_data.old_pw = (char *)old_pw;
	req_data.new_pw = (char *)new_pw;

	result = tcore_plugin_dispatch_request(plugin, TRUE,
		TCORE_COMMAND_SIM_CHANGE_PINS,
		&req_data, sizeof(TelSimSecChangePinPw),
		on_response_dtapi_sim_change_pins, rsp_cb_data);
	if (result != TEL_RETURN_SUCCESS) {
		dtapi_return_error(invocation, G_DBUS_ERROR_FAILED, "Dispatch Failed");
		tcore_free(rsp_cb_data);
	}

	return TRUE;
}

static void
on_response_dtapi_sim_disable_facility(gint result,
	const void *data, void *cb_data)
{
	DbusRespCbData *rsp_cb_data = cb_data;
	const TelSimFacilityResult *facility = data;

	tcore_check_return_assert(NULL != rsp_cb_data);

	dbg("result[%d]", result);

	if (result == TEL_SIM_RESULT_SUCCESS) {
		tcore_check_return_assert(NULL != facility);
		dbg("type[%d] retry_count[%d]", facility->type, facility->retry_count);
		telephony_sim_complete_disable_facility(rsp_cb_data->interface_object,
			rsp_cb_data->invocation, result,
			facility->type, facility->retry_count);
	} else {
		telephony_sim_complete_disable_facility(rsp_cb_data->interface_object,
			rsp_cb_data->invocation, result,
			0, 0);
	}
	tcore_free(rsp_cb_data);
}

static gboolean dtapi_sim_disable_facility(TelephonySim *sim,
	GDBusMethodInvocation *invocation,
	gint lock_type, const gchar *pw, gpointer user_data)
{
	TelReturn result;
	TcorePlugin *plugin = user_data;
	TelSimFacilityPw req_data= {0,};
	DbusRespCbData *rsp_cb_data;

	if (dtapi_check_access_control(invocation, AC_SIM, "x") == FALSE)
		return TRUE;

	rsp_cb_data = dtapi_create_resp_cb_data(sim, invocation, NULL, 0);

	req_data.lock_type =  lock_type;
	req_data.pw = (char *)pw;

	result = tcore_plugin_dispatch_request(plugin, TRUE,
		TCORE_COMMAND_SIM_DISABLE_FACILITY,
		&req_data, sizeof(TelSimFacilityPw),
		on_response_dtapi_sim_disable_facility, rsp_cb_data);
	if (result != TEL_RETURN_SUCCESS) {
		dtapi_return_error(invocation, G_DBUS_ERROR_FAILED, "Dispatch Failed");
		tcore_free(rsp_cb_data);
	}

	return TRUE;
}

static void
on_response_dtapi_sim_enable_facility(gint result,
	const void *data, void *cb_data)
{
	DbusRespCbData *rsp_cb_data = cb_data;
	const TelSimFacilityResult *facility = data;

	tcore_check_return_assert(NULL != rsp_cb_data);

	dbg("result[%d]", result);

	if (result == TEL_SIM_RESULT_SUCCESS) {
		tcore_check_return_assert(NULL != facility);
		dbg("type[%d] retry_count[%d]", facility->type, facility->retry_count);

		telephony_sim_complete_enable_facility(rsp_cb_data->interface_object,
			rsp_cb_data->invocation, result,
			facility->type, facility->retry_count);
	} else {
		telephony_sim_complete_enable_facility(rsp_cb_data->interface_object,
			rsp_cb_data->invocation, result,
			0, 0);
	}
	tcore_free(rsp_cb_data);
}

static gboolean dtapi_sim_enable_facility(TelephonySim *sim,
	GDBusMethodInvocation *invocation,
	gint lock_type, const gchar *pw, gpointer user_data)
{
	TelReturn result;
	TcorePlugin *plugin = user_data;
	TelSimFacilityPw req_data = {0,};
	DbusRespCbData *rsp_cb_data;

	if (dtapi_check_access_control(invocation, AC_SIM, "x") == FALSE)
		return TRUE;

	rsp_cb_data = dtapi_create_resp_cb_data(sim, invocation, NULL, 0);

	req_data.lock_type =  lock_type;
	req_data.pw = (char *)pw;

	result = tcore_plugin_dispatch_request(plugin, TRUE,
		TCORE_COMMAND_SIM_ENABLE_FACILITY,
		&req_data, sizeof(TelSimFacilityPw),
		on_response_dtapi_sim_enable_facility, rsp_cb_data);
	if (result != TEL_RETURN_SUCCESS) {
		dtapi_return_error(invocation, G_DBUS_ERROR_FAILED, "Dispatch Failed");
		tcore_free(rsp_cb_data);
	}

	return TRUE;
}

static void
on_response_dtapi_sim_get_facility(gint result,
	const void *data, void *cb_data)
{
	DbusRespCbData *rsp_cb_data = cb_data;
	const TelSimFacilityInfo *facility = data;

	tcore_check_return_assert(NULL != rsp_cb_data);

	dbg("result[%d]", result);

	if (result == TEL_SIM_RESULT_SUCCESS) {
		tcore_check_return_assert(NULL != facility);
		dbg("type[%d] f_status[%d]",  facility->type, facility->f_status);
		telephony_sim_complete_get_facility(rsp_cb_data->interface_object,
			rsp_cb_data->invocation, result,
			facility->type, facility->f_status);
	} else {
		telephony_sim_complete_get_facility(rsp_cb_data->interface_object,
			rsp_cb_data->invocation, result,
			0, TEL_SIM_FACILITY_STATUS_UNKNOWN);
	}
	tcore_free(rsp_cb_data);
}

static gboolean dtapi_sim_get_facility(TelephonySim *sim,
	GDBusMethodInvocation *invocation,
	gint type, gpointer user_data)
{
	TelReturn result;
	TcorePlugin *plugin = user_data;
	TelSimFacilityInfo req_data = {0,};
	DbusRespCbData *rsp_cb_data;

	if (dtapi_check_access_control(invocation, AC_SIM, "r") == FALSE)
		return TRUE;

	rsp_cb_data = dtapi_create_resp_cb_data(sim, invocation, NULL, 0);

	req_data.type =  type;

	result = tcore_plugin_dispatch_request(plugin, TRUE,
		TCORE_COMMAND_SIM_GET_FACILITY,
		&req_data, sizeof(TelSimFacilityInfo),
		on_response_dtapi_sim_get_facility, rsp_cb_data);
	if (result != TEL_RETURN_SUCCESS) {
		dtapi_return_error(invocation, G_DBUS_ERROR_FAILED, "Dispatch Failed");
		tcore_free(rsp_cb_data);
	}

	return TRUE;
}

static void
on_response_dtapi_sim_get_lock_info(gint result,
	const void *data, void *cb_data)
{
	DbusRespCbData *rsp_cb_data = cb_data;
	const TelSimLockInfo *lock_info = data;

	tcore_check_return_assert(NULL != rsp_cb_data);

	dbg("result[%d]", result);

	if (result == TEL_SIM_RESULT_SUCCESS) {
		tcore_check_return_assert(NULL != lock_info);
		dbg("lock_type[%d] lock_status[%d] retry_count[%d]",
		lock_info->lock_type, lock_info->lock_status, lock_info->retry_count);
		telephony_sim_complete_get_lock_info(rsp_cb_data->interface_object,
			rsp_cb_data->invocation, result,
			lock_info->lock_type, lock_info->lock_status,
			lock_info->retry_count);
	} else {
		telephony_sim_complete_get_lock_info(rsp_cb_data->interface_object,
			rsp_cb_data->invocation, result,
			0, 0, 0);
	}
	tcore_free(rsp_cb_data);
}

static gboolean dtapi_sim_get_lock_info(TelephonySim *sim,
	GDBusMethodInvocation *invocation,
	gint type, gpointer user_data)
{
	TelReturn result;
	TcorePlugin *plugin = user_data;
	TelSimLockInfo req_data = {0,};
	DbusRespCbData *rsp_cb_data;

	if (dtapi_check_access_control(invocation, AC_SIM, "r") == FALSE)
		return TRUE;

	rsp_cb_data = dtapi_create_resp_cb_data(sim, invocation, NULL, 0);

	req_data.lock_type =  type;

	result = tcore_plugin_dispatch_request(plugin, TRUE, TCORE_COMMAND_SIM_GET_LOCK_INFO, &req_data, sizeof(TelSimLockInfo),
			on_response_dtapi_sim_get_lock_info, rsp_cb_data);
	if (result != TEL_RETURN_SUCCESS) {
		dtapi_return_error(invocation, G_DBUS_ERROR_FAILED, "Dispatch Failed");
		tcore_free(rsp_cb_data);
	}

	return TRUE;
}

static void
on_response_dtapi_sim_req_apdu(gint result,
	const void *data, void *cb_data)
{
	DbusRespCbData *rsp_cb_data = cb_data;
	const TelSimApduResp *apdu = data;

	tcore_check_return_assert(NULL != rsp_cb_data);

	dbg("result[%d]", result);

	if (result == TEL_SIM_RESULT_SUCCESS) {
		gchar *encoded_apdu;
		tcore_check_return_assert(NULL != apdu);
		dbg("apdu_resp_len[%d]", apdu->apdu_resp_len);
		/* Encode Base64 - Resp data */
		encoded_apdu = g_base64_encode((const guchar *)apdu->apdu_resp,
					apdu->apdu_resp_len);
		telephony_sim_complete_req_apdu(rsp_cb_data->interface_object,
			rsp_cb_data->invocation, result,
			encoded_apdu);

		/* Free resources */
		tcore_free(encoded_apdu);
	} else {
		telephony_sim_complete_req_apdu(rsp_cb_data->interface_object,
			rsp_cb_data->invocation, result,
			NULL);
	}
	tcore_free(rsp_cb_data);
}

static gboolean dtapi_sim_req_apdu(TelephonySim *sim,
	GDBusMethodInvocation *invocation,
	const gchar *apdu, gpointer user_data)
{
	TelReturn result;
	TcorePlugin *plugin = user_data;
	TelSimApdu req_data = {0,};
	guchar *decoded_data;
	gsize decoded_data_len;
	DbusRespCbData *rsp_cb_data;

	if (dtapi_check_access_control(invocation, AC_SIM, "x") == FALSE)
		return TRUE;

	/* Decode Base64 - apdu */
	decoded_data = g_base64_decode((const gchar *)apdu, &decoded_data_len);
	if ((decoded_data_len > TEL_SIM_APDU_LEN_MAX) || (decoded_data_len == 0)) {
		dtapi_return_error(invocation, G_DBUS_ERROR_FAILED, "Decoding Failed");
		tcore_free(decoded_data);
		return TRUE;
	}

	memcpy(req_data.apdu, decoded_data, decoded_data_len);
	req_data.apdu_len = decoded_data_len;

	/* Free resources */
	tcore_free(decoded_data);

	rsp_cb_data = dtapi_create_resp_cb_data(sim, invocation, NULL, 0);

	result = tcore_plugin_dispatch_request(plugin, TRUE,
		TCORE_COMMAND_SIM_REQ_APDU,
		&req_data, sizeof(TelSimApdu),
		on_response_dtapi_sim_req_apdu, rsp_cb_data);
	if (result != TEL_RETURN_SUCCESS) {
		dtapi_return_error(invocation, G_DBUS_ERROR_FAILED, "Dispatch Failed");
		tcore_free(rsp_cb_data);
	}

	return TRUE;
}

static void
on_response_dtapi_sim_req_atr(gint result,
	const void *data, void *cb_data)
{
	DbusRespCbData *rsp_cb_data = cb_data;
	const TelSimAtr *atr_resp = data;
	gchar *encoded_atr  = NULL;
	tcore_check_return_assert(NULL != rsp_cb_data);

	dbg("result[%d]", result);

	if (result == TEL_SIM_RESULT_SUCCESS) {
		tcore_check_return_assert(NULL != atr_resp);
		dbg("atr_len[%d]", atr_resp->atr_len);
		/* Encode Base64 - Resp data */
		encoded_atr = g_base64_encode((const guchar *)atr_resp->atr,
					atr_resp->atr_len);
	} else {
		err("Sim atr response failed!!");
	}

	telephony_sim_complete_req_atr(rsp_cb_data->interface_object,
		rsp_cb_data->invocation, result, encoded_atr);

	tcore_free(encoded_atr);
	tcore_free(rsp_cb_data);
}

static gboolean dtapi_sim_req_atr(TelephonySim *sim,
	GDBusMethodInvocation *invocation, gpointer user_data)
{
	TelReturn result;
	TcorePlugin *plugin = user_data;
	DbusRespCbData *rsp_cb_data;

	if (dtapi_check_access_control(invocation, AC_SIM, "r") == FALSE)
		return TRUE;

	rsp_cb_data = dtapi_create_resp_cb_data(sim, invocation, NULL, 0);

	result = tcore_plugin_dispatch_request(plugin, TRUE,
		TCORE_COMMAND_SIM_REQ_ATR,
		NULL, 0,
		on_response_dtapi_sim_req_atr, rsp_cb_data);
	if (result != TEL_RETURN_SUCCESS) {
		dtapi_return_error(invocation, G_DBUS_ERROR_FAILED, "Dispatch Failed");
		tcore_free(rsp_cb_data);
	}

	return TRUE;
}

gboolean dtapi_setup_sim_interface(TelephonyObjectSkeleton *object,
	TcorePlugin *plugin)
{
	TelephonySim *sim = telephony_sim_skeleton_new();
	GVariant *var;
	GVariantBuilder var_builder;
	tcore_check_return_value_assert(sim != NULL, FALSE);

	telephony_object_skeleton_set_sim(object, sim);
	g_object_unref(sim);

	dbg("sim = %p", sim);

	g_signal_connect (sim,
			"handle-get-imsi",
			G_CALLBACK (dtapi_sim_get_imsi),
			plugin);

	g_signal_connect (sim,
			"handle-get-ecc",
			G_CALLBACK (dtapi_sim_get_ecc),
			plugin);

	g_signal_connect (sim,
			"handle-get-iccid",
			G_CALLBACK (dtapi_sim_get_iccid),
			plugin);

	g_signal_connect (sim,
			"handle-get-language",
			G_CALLBACK (dtapi_sim_get_language),
			plugin);

	g_signal_connect (sim,
			"handle-set-language",
			G_CALLBACK (dtapi_sim_set_language),
			plugin);

	g_signal_connect (sim,
			"handle-get-call-forwarding-info",
			G_CALLBACK (dtapi_sim_get_call_forwarding_info),
			plugin);

	g_signal_connect (sim,
			"handle-get-message-waiting-info",
			G_CALLBACK (dtapi_sim_get_message_waiting_info),
			plugin);

	g_signal_connect (sim,
			"handle-set-message-waiting-info",
			G_CALLBACK (dtapi_sim_set_message_waiting_info),
			plugin);

	g_signal_connect (sim,
			"handle-get-mailbox-info",
			G_CALLBACK (dtapi_sim_get_mailbox_info),
			plugin);

	g_signal_connect (sim,
			"handle-set-mailbox-info",
			G_CALLBACK (dtapi_sim_set_mailbox_info),
			plugin);

	g_signal_connect (sim,
			"handle-get-msisdn",
			G_CALLBACK (dtapi_sim_get_msisdn),
			plugin);

	g_signal_connect (sim,
			"handle-get-spn",
			G_CALLBACK (dtapi_sim_get_spn),
			plugin);

	g_signal_connect (sim,
			"handle-get-cphs-net-name",
			G_CALLBACK (dtapi_sim_get_cphs_net_name),
			plugin);

	g_signal_connect (sim,
			"handle-get-sp-display-info",
			G_CALLBACK (dtapi_sim_get_sp_display_info),
			plugin);

	g_signal_connect (sim,
			"handle-req-authentication",
			G_CALLBACK (dtapi_sim_req_authentication),
			plugin);

	g_signal_connect (sim,
			"handle-verify-pins",
			G_CALLBACK (dtapi_sim_verify_pins),
			plugin);

	g_signal_connect (sim,
			"handle-verify-puks",
			G_CALLBACK (dtapi_sim_verify_puks),
			plugin);

	g_signal_connect (sim,
			"handle-change-pins",
			G_CALLBACK (dtapi_sim_change_pins),
			plugin);

	g_signal_connect (sim,
			"handle-disable-facility",
			G_CALLBACK (dtapi_sim_disable_facility),
			plugin);

	g_signal_connect (sim,
			"handle-enable-facility",
			G_CALLBACK (dtapi_sim_enable_facility),
			plugin);

	g_signal_connect (sim,
			"handle-get-facility",
			G_CALLBACK (dtapi_sim_get_facility),
			plugin);

	g_signal_connect (sim,
			"handle-get-lock-info",
			G_CALLBACK (dtapi_sim_get_lock_info),
			plugin);

	g_signal_connect (sim,
			"handle-req-apdu",
			G_CALLBACK (dtapi_sim_req_apdu),
			plugin);

	g_signal_connect (sim,
			"handle-req-atr",
			G_CALLBACK (dtapi_sim_req_atr),
			plugin);

	/* Initialize D-Bus properties */
	g_variant_builder_init(&var_builder, G_VARIANT_TYPE("a{sv}"));
	g_variant_builder_add(&var_builder, "{sv}", "status",
		g_variant_new_int32(TEL_SIM_STATUS_UNKNOWN));
	g_variant_builder_add(&var_builder, "{sv}", "change_status",
		g_variant_new_int32(TEL_SIM_CHANGE_STATUS_UNKNOWN));
	var = g_variant_builder_end(&var_builder);
	telephony_sim_set_card_status(sim, var);

	telephony_sim_set_sim_type(sim, TEL_SIM_CARD_TYPE_UNKNOWN);
	telephony_sim_set_mcc(sim, NULL);
	telephony_sim_set_mnc(sim, NULL);
	telephony_sim_set_msin(sim, NULL);

	return TRUE;
}

gboolean dtapi_handle_sim_notification(TelephonyObjectSkeleton *object,
	TcorePlugin *plugin, TcoreNotification command,
	guint data_len, const void *data)
{
	TelephonySim *sim;

	tcore_check_return_value_assert(object != NULL, FALSE);

	sim = telephony_object_peek_sim(TELEPHONY_OBJECT(object));

	dbg("sim = %p - notification !!! (command = 0x%x, data_len = %d)",
		sim, command, data_len);

	switch (command) {
	case TCORE_NOTIFICATION_SIM_TYPE: {
		tcore_check_return_value_assert(data != NULL, FALSE);

		const TelSimCardType sim_type = *(TelSimCardType *)data;
		dbg("SIM Type: [%d]", sim_type);

		/* Update Property */
		telephony_sim_set_sim_type(sim, sim_type);
	}
	break;
	case TCORE_NOTIFICATION_SIM_STATUS: {
		const TelSimCardStatusInfo *sim_info = (TelSimCardStatusInfo *)data;
		GVariant *var;
		GVariantBuilder var_builder;
		tcore_check_return_value_assert(data != NULL, FALSE);

		dbg("SIM Status: [%d], change_status: [%d]",
			sim_info->status, sim_info->change_status);

		g_variant_builder_init(&var_builder, G_VARIANT_TYPE("a{sv}"));
		g_variant_builder_add(&var_builder, "{sv}", "status",
			g_variant_new_int32(sim_info->status));
		g_variant_builder_add(&var_builder, "{sv}", "change_status",
			g_variant_new_int32(sim_info->change_status));
		var = g_variant_builder_end(&var_builder);

		telephony_sim_set_card_status(sim, var);
	}
	break;
	default:
		/* TBD : Check when other dbus properties should be updated */
		err("not handled cmd[0x%x]", command);
	break;
	}

	return TRUE;
}
