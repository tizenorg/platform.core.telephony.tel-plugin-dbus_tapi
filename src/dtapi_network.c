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

#include "dtapi_network.h"
#include "dtapi_util.h"

#include <plugin.h>

#define AC_NETWORK	"telephony_framework::api_network"

static void on_response_dtapi_network_get_identity_info(gint result,
	const void *data, void *cb_data)
{
	DbusRespCbData *rsp_cb_data = cb_data;
	const TelNetworkIdentityInfo *identity_info = data;

	tcore_check_return_assert(rsp_cb_data != NULL);

	dbg("result[%d]", result);

	if ((result != TEL_NETWORK_RESULT_SUCCESS)
			|| (identity_info == NULL)) {
		telephony_network_complete_get_identity_info(rsp_cb_data->interface_object,
			rsp_cb_data->invocation, result, NULL, NULL, NULL);
	} else {
		dbg("plmn[%s] short_name[%s] long_name[%s]",
				identity_info->plmn,
				identity_info->short_name,
				identity_info->long_name);
		telephony_network_complete_get_identity_info(rsp_cb_data->interface_object,
			rsp_cb_data->invocation, result, identity_info->plmn,
			identity_info->short_name, identity_info->long_name);
	}

	tcore_free(rsp_cb_data);
}

static gboolean dtapi_network_get_identity_info(TelephonyNetwork *network,
	GDBusMethodInvocation *invocation, gpointer user_data)
{
	DbusRespCbData *rsp_cb_data = NULL;
	TelReturn result;

	if (dtapi_check_access_control(invocation, AC_NETWORK, "r") == FALSE)
		return TRUE;

	rsp_cb_data = dtapi_create_resp_cb_data(network, invocation, NULL, 0);
	result = tcore_plugin_dispatch_request(user_data, TRUE,
		TCORE_COMMAND_NETWORK_GET_IDENTITY_INFO,
		NULL, 0,
		on_response_dtapi_network_get_identity_info, rsp_cb_data);
	if (result != TEL_RETURN_SUCCESS) {
		dtapi_return_error(invocation, G_DBUS_ERROR_FAILED, "Dispatch failed");
		tcore_free(rsp_cb_data);
	}

	return TRUE;
}

static void on_response_dtapi_network_search(gint result,
	const void *data, void *cb_data)
{
	DbusRespCbData *rsp_cb_data = cb_data;
	const TelNetworkPlmnList *plmn_list = data;

	GVariant *network_list = NULL;
	GVariantBuilder variant_builder;

	guint count = 0;

	tcore_check_return_assert(rsp_cb_data != NULL);

	dbg("result[%d]", result);

	g_variant_builder_init(&variant_builder, G_VARIANT_TYPE("aa{sv}"));
	if ((result == TEL_SIM_RESULT_SUCCESS)
			&& (plmn_list != NULL)) {
		GVariantBuilder variant_builder2;
		GVariant *network_identity = NULL;

		dbg("Network Search PLMN List count: [%d]", plmn_list->count);
		for (count = 0; count < plmn_list->count; count++) {
			g_variant_builder_open(&variant_builder, G_VARIANT_TYPE("a{sv}"));

			g_variant_builder_add(&variant_builder, "{sv}",
				"plmn_status", g_variant_new_int32(plmn_list->network_list[count].plmn_status));
			g_variant_builder_add(&variant_builder, "{sv}",
				"act", g_variant_new_int32(plmn_list->network_list[count].act));

			g_variant_builder_init(&variant_builder2, G_VARIANT_TYPE("a{sv}"));
			g_variant_builder_add(&variant_builder2, "{sv}",
				"plmn",
				g_variant_new_string(plmn_list->network_list[count].network_identity.plmn));
			g_variant_builder_add(&variant_builder2, "{sv}",
				"short_name",
				g_variant_new_string(plmn_list->network_list[count].network_identity.short_name));
			g_variant_builder_add(&variant_builder2, "{sv}",
				"long_name",
				g_variant_new_string(plmn_list->network_list[count].network_identity.long_name));
			network_identity = g_variant_builder_end(&variant_builder2);

			g_variant_builder_add(&variant_builder, "{sv}",
				"network_identity", network_identity);

			g_variant_builder_close(&variant_builder);

			dbg("[%d] : PLMN Status: [%d] AcT: [%d]", count,
				plmn_list->network_list[count].plmn_status,
				plmn_list->network_list[count].act);
		}
	}
	network_list = g_variant_builder_end(&variant_builder);

	telephony_network_complete_search(rsp_cb_data->interface_object,
		rsp_cb_data->invocation, result, count, network_list);
	g_variant_unref(network_list);

	tcore_free(rsp_cb_data);
}

static gboolean dtapi_network_search(TelephonyNetwork *network,
	GDBusMethodInvocation *invocation, gpointer user_data)
{
	DbusRespCbData *rsp_cb_data = NULL;
	TelReturn result;

	if (dtapi_check_access_control(invocation, AC_NETWORK, "x") == FALSE)
			return TRUE;

	rsp_cb_data = dtapi_create_resp_cb_data(network, invocation, NULL, 0);

	result = tcore_plugin_dispatch_request(user_data, TRUE,
		TCORE_COMMAND_NETWORK_SEARCH,
		NULL, 0,
		on_response_dtapi_network_search, rsp_cb_data);
	if (result != TEL_RETURN_SUCCESS) {
		dtapi_return_error(invocation, G_DBUS_ERROR_FAILED, "Dispatch failed");
		tcore_free(rsp_cb_data);
	}

	return TRUE;
}

static void on_response_dtapi_network_cancel_search(gint result,
	const void *data, void *cb_data)
{
	DbusRespCbData *rsp_cb_data = cb_data;
	tcore_check_return_assert(rsp_cb_data != NULL);

	dbg("result[%d]", result);
	telephony_network_complete_cancel_search(rsp_cb_data->interface_object,
		rsp_cb_data->invocation, result);

	tcore_free(rsp_cb_data);
}

static gboolean dtapi_network_cancel_search(TelephonyNetwork *network,
	GDBusMethodInvocation *invocation, gpointer user_data)
{
	DbusRespCbData *rsp_cb_data = NULL;
	TelReturn result;

	if (dtapi_check_access_control(invocation, AC_NETWORK, "x") == FALSE)
			return TRUE;

	rsp_cb_data = dtapi_create_resp_cb_data(network, invocation, NULL, 0);

	result = tcore_plugin_dispatch_request(user_data, TRUE,
		TCORE_COMMAND_NETWORK_CANCEL_SEARCH,
		NULL, 0,
		on_response_dtapi_network_cancel_search, rsp_cb_data);
	if (result != TEL_RETURN_SUCCESS) {
		dtapi_return_error(invocation, G_DBUS_ERROR_FAILED, "Dispatch failed");
		tcore_free(rsp_cb_data);
	}

	return TRUE;
}

static void on_response_dtapi_network_select_automatic(gint result,
	const void *data, void *cb_data)
{
	DbusRespCbData *rsp_cb_data = cb_data;
	tcore_check_return_assert(rsp_cb_data != NULL);

	dbg("result[%d]", result);
	telephony_network_complete_select_automatic(rsp_cb_data->interface_object,
		rsp_cb_data->invocation, result);

	tcore_free(rsp_cb_data);
}

static gboolean dtapi_network_select_automatic(TelephonyNetwork *network,
	GDBusMethodInvocation *invocation, gpointer user_data)
{
	DbusRespCbData *rsp_cb_data = NULL;
	TelReturn result;

	if (dtapi_check_access_control(invocation, AC_NETWORK, "w") ==  FALSE)
		return TRUE;

	rsp_cb_data = dtapi_create_resp_cb_data(network, invocation, NULL, 0);

	result = tcore_plugin_dispatch_request(user_data, TRUE,
		TCORE_COMMAND_NETWORK_SELECT_AUTOMATIC,
		NULL, 0,
		on_response_dtapi_network_select_automatic, rsp_cb_data);
	if (result != TEL_RETURN_SUCCESS) {
		dtapi_return_error(invocation, G_DBUS_ERROR_FAILED, "Dispatch failed");
		tcore_free(rsp_cb_data);
	}

	return TRUE;
}

static void on_response_dtapi_network_select_manual(gint result,
	const void *data, void *cb_data)
{
	DbusRespCbData *rsp_cb_data = cb_data;
	tcore_check_return_assert(rsp_cb_data != NULL);

	dbg("result[%d]", result);
	telephony_network_complete_select_manual(rsp_cb_data->interface_object,
			rsp_cb_data->invocation, result);
	tcore_free(rsp_cb_data);
}

static gboolean dtapi_network_select_manual(TelephonyNetwork *network,
	GDBusMethodInvocation *invocation,
	const gchar *plmn, gint act, gpointer user_data)
{
	DbusRespCbData *rsp_cb_data = NULL;
	TelReturn result ;
	TelNetworkSelectManualInfo req_data;

	if (dtapi_check_access_control(invocation, AC_NETWORK, "w") == FALSE)
			return TRUE;

	rsp_cb_data = dtapi_create_resp_cb_data(network, invocation, NULL, 0);

	req_data.plmn = (char *)plmn;
	req_data.act = act;

	result = tcore_plugin_dispatch_request(user_data, TRUE,
		TCORE_COMMAND_NETWORK_SELECT_MANUAL,
		&req_data, sizeof(TelNetworkSelectManualInfo),
		on_response_dtapi_network_select_manual, rsp_cb_data);
	if (result != TEL_RETURN_SUCCESS) {
		dtapi_return_error(invocation, G_DBUS_ERROR_FAILED, "Dispatch failed");
		tcore_free(rsp_cb_data);
	}

	return TRUE;
}

static void on_response_dtapi_network_get_selection_mode(gint result,
	const void *data, void *cb_data)
{
	DbusRespCbData *rsp_cb_data = cb_data;
	TelNetworkSelectionMode selection_mode = 0;

	tcore_check_return_assert(rsp_cb_data != NULL);

	dbg("result[%d]", result);
	if ((result == TEL_NETWORK_RESULT_SUCCESS)
			&& (NULL != data)) {
		selection_mode = *(TelNetworkSelectionMode *)data;
		dbg("selection_mode[%d]", selection_mode);
	}

	telephony_network_complete_get_selection_mode(rsp_cb_data->interface_object,
		rsp_cb_data->invocation, result, selection_mode);

	tcore_free(rsp_cb_data);
}

static gboolean dtapi_network_get_selection_mode(TelephonyNetwork *network,
	GDBusMethodInvocation *invocation, gpointer user_data)
{
	DbusRespCbData *rsp_cb_data = NULL;
	TelReturn result ;

	if (dtapi_check_access_control(invocation, AC_NETWORK, "r") ==  FALSE)
		return TRUE;

	rsp_cb_data = dtapi_create_resp_cb_data(network, invocation, NULL, 0);

	result = tcore_plugin_dispatch_request(user_data, TRUE,
		TCORE_COMMAND_NETWORK_GET_SELECTION_MODE,
		NULL, 0,
		on_response_dtapi_network_get_selection_mode, rsp_cb_data);
	if (result != TEL_RETURN_SUCCESS) {
		dtapi_return_error(invocation, G_DBUS_ERROR_FAILED, "Dispatch failed");
		tcore_free(rsp_cb_data);
	}

	return TRUE;
}

static void on_response_dtapi_network_set_preferred_plmn(gint result,
	const void *data, void *cb_data)
{
	DbusRespCbData *rsp_cb_data = cb_data;
	tcore_check_return_assert(rsp_cb_data != NULL);

	dbg("result[%d]", result);
	telephony_network_complete_set_preferred_plmn(rsp_cb_data->interface_object,
		rsp_cb_data->invocation, result);

	tcore_free(rsp_cb_data);
}

static gboolean dtapi_network_set_preferred_plmn(TelephonyNetwork *network,
	GDBusMethodInvocation *invocation,
	guint index, const gchar *plmn, gint act, gpointer user_data)
{
	DbusRespCbData *rsp_cb_data = NULL;
	TelReturn result ;
	TelNetworkPreferredPlmnInfo req_data;

	if (dtapi_check_access_control(invocation, AC_NETWORK, "w") == FALSE)
		return TRUE;

	rsp_cb_data = dtapi_create_resp_cb_data(network, invocation, NULL, 0);
	req_data.index = index;
	req_data.plmn = (char *)plmn;
	req_data.act = act;
	result = tcore_plugin_dispatch_request(user_data, TRUE,
		TCORE_COMMAND_NETWORK_SET_PREFERRED_PLMN,
		&req_data, sizeof(TelNetworkPreferredPlmnInfo),
		on_response_dtapi_network_set_preferred_plmn, rsp_cb_data);
	if (result != TEL_RETURN_SUCCESS) {
		dtapi_return_error(invocation, G_DBUS_ERROR_FAILED, "Dispatch failed");
		tcore_free(rsp_cb_data);
	}

	return TRUE;
}

static void on_response_dtapi_network_get_preferred_plmn(gint result,
	const void *data, void *cb_data)
{
	DbusRespCbData *rsp_cb_data = cb_data;
	const TelNetworkPreferredPlmnList *plmn_list = data;

	GVariant *list = NULL;
	GVariantBuilder variant_builder;

	guint count = 0;

	tcore_check_return_assert(rsp_cb_data != NULL);

	dbg("result[%d]", result);

	g_variant_builder_init(&variant_builder, G_VARIANT_TYPE("aa{sv}"));
	if ((result == TEL_SIM_RESULT_SUCCESS)
			&& (NULL != plmn_list)) {
		dbg("Preferred PLMN List count: [%d]", plmn_list->count);
		for (count = 0; count < plmn_list->count; count++) {
			g_variant_builder_open(&variant_builder, G_VARIANT_TYPE("a{sv}"));

			g_variant_builder_add(&variant_builder, "{sv}",
				"index", g_variant_new_uint32(plmn_list->list[count].index));
			g_variant_builder_add(&variant_builder, "{sv}",
				"plmn", g_variant_new_string(plmn_list->list[count].plmn));
			g_variant_builder_add(&variant_builder, "{sv}",
				"act", g_variant_new_int32(plmn_list->list[count].act));

			g_variant_builder_close(&variant_builder);

			dbg("[%d] : Index: [%d] PLMN: [%s] AcT: [%d]", count,
				plmn_list->list[count].index,
				plmn_list->list[count].plmn,
				plmn_list->list[count].act);
		}
	}
	list = g_variant_builder_end(&variant_builder);

	telephony_network_complete_get_preferred_plmn(rsp_cb_data->interface_object,
		rsp_cb_data->invocation, result, count, list);
	g_variant_unref(list);

	tcore_free(rsp_cb_data);
}

static gboolean dtapi_network_get_preferred_plmn(TelephonyNetwork *network,
	GDBusMethodInvocation *invocation, gpointer user_data)
{
	DbusRespCbData *rsp_cb_data = NULL;
	TelReturn result ;

	if (dtapi_check_access_control(invocation, AC_NETWORK, "r") == FALSE)
		return TRUE;

	rsp_cb_data = dtapi_create_resp_cb_data(network, invocation, NULL, 0);

	result = tcore_plugin_dispatch_request(user_data, TRUE,
		TCORE_COMMAND_NETWORK_GET_PREFERRED_PLMN,
		NULL, 0,
		on_response_dtapi_network_get_preferred_plmn, rsp_cb_data);
	if (result != TEL_RETURN_SUCCESS) {
		dtapi_return_error(invocation, G_DBUS_ERROR_FAILED, "Dispatch failed");
		tcore_free(rsp_cb_data);
	}

	return TRUE;
}

static void on_response_dtapi_network_set_mode(gint result,
	const void *data, void *cb_data)
{
	DbusRespCbData *rsp_cb_data = cb_data;
	tcore_check_return_assert(rsp_cb_data != NULL);

	dbg("result[%d]", result);
	telephony_network_complete_set_mode(rsp_cb_data->interface_object,
		rsp_cb_data->invocation, result);

	tcore_free(rsp_cb_data);
}

static gboolean dtapi_network_set_mode(TelephonyNetwork *network,
	GDBusMethodInvocation *invocation,
	gint mode, gpointer user_data)
{
	DbusRespCbData *rsp_cb_data = NULL;
	TelReturn result ;
	TelNetworkMode network_mode;

	if (dtapi_check_access_control(invocation, AC_NETWORK, "w") == FALSE)
		return TRUE;

	rsp_cb_data = dtapi_create_resp_cb_data(network, invocation, NULL, 0);

	network_mode = mode;
	result = tcore_plugin_dispatch_request(user_data, TRUE,
		TCORE_COMMAND_NETWORK_SET_MODE,
		&network_mode, sizeof(TelNetworkMode),
		on_response_dtapi_network_set_mode, rsp_cb_data);
	if (result != TEL_RETURN_SUCCESS) {
		dtapi_return_error(invocation, G_DBUS_ERROR_FAILED, "Dispatch failed");
		tcore_free(rsp_cb_data);
	}

	return TRUE;
}

static void on_response_dtapi_network_get_mode(gint result,
	const void *data, void *cb_data)
{
	DbusRespCbData *rsp_cb_data = cb_data;
	TelNetworkMode network_mode = 0;

	tcore_check_return_assert(rsp_cb_data != NULL);

	dbg("result[%d]", result);
	if ((result == TEL_SIM_RESULT_SUCCESS)
			&& (data != NULL)) {
		network_mode = *(TelNetworkMode *)data;
		dbg("network_mode[%d]", network_mode);
	}

	telephony_network_complete_get_mode(rsp_cb_data->interface_object,
		rsp_cb_data->invocation, result, network_mode);

	tcore_free(rsp_cb_data);
}

static gboolean dtapi_network_get_mode(TelephonyNetwork *network,
	GDBusMethodInvocation *invocation, gpointer user_data)
{
	DbusRespCbData *rsp_cb_data = NULL;
	TelReturn result;

	if (dtapi_check_access_control(invocation, AC_NETWORK, "r") == FALSE)
		return TRUE;

	rsp_cb_data = dtapi_create_resp_cb_data(network, invocation, NULL, 0);

	result = tcore_plugin_dispatch_request(user_data, TRUE,
		TCORE_COMMAND_NETWORK_GET_MODE,
		NULL, 0,
		on_response_dtapi_network_get_mode, rsp_cb_data);
	if (result != TEL_RETURN_SUCCESS) {
		dtapi_return_error(invocation, G_DBUS_ERROR_FAILED, "Dispatch failed");
		tcore_free(rsp_cb_data);
	}

	return TRUE;
}

static void on_response_dtapi_network_get_ngbr_cell_info(gint result,
	const void *data, void *cb_data)
{
	DbusRespCbData *rsp_cb_data = cb_data;
	const TelNetworkNeighbourCellInfo *ngbr_cell_info = data;

	GVariantBuilder gsm_var_builder;
	GVariant *gsm_list = NULL;
	guint gsm_count = 0;

	GVariantBuilder umts_var_builder;
	GVariant *umts_list = NULL;
	guint umts_count = 0;

	tcore_check_return_assert(rsp_cb_data != NULL);

	dbg("result[%d]", result);

	g_variant_builder_init(&gsm_var_builder, G_VARIANT_TYPE("aa{sv}"));
	g_variant_builder_init(&umts_var_builder, G_VARIANT_TYPE("aa{sv}"));
	if ((result == TEL_SIM_RESULT_SUCCESS)
			&& (ngbr_cell_info != NULL)) {
		dbg("GSM Cell info List count: [%d]", ngbr_cell_info->gsm_list_count);
		for (gsm_count = 0; gsm_count < ngbr_cell_info->gsm_list_count; gsm_count++) {
			g_variant_builder_open(&gsm_var_builder, G_VARIANT_TYPE("a{sv}"));

			g_variant_builder_add(&gsm_var_builder, "{sv}",
				"cell_id",
				g_variant_new_int32(ngbr_cell_info->gsm_list[gsm_count].cell_id));
			g_variant_builder_add(&gsm_var_builder, "{sv}",
				"lac",
				g_variant_new_int32(ngbr_cell_info->gsm_list[gsm_count].lac));
			g_variant_builder_add(&gsm_var_builder, "{sv}",
				"bcch",
				g_variant_new_int32(ngbr_cell_info->gsm_list[gsm_count].bcch));
			g_variant_builder_add(&gsm_var_builder, "{sv}",
				"bsic",
				g_variant_new_int32(ngbr_cell_info->gsm_list[gsm_count].bsic));
			g_variant_builder_add(&gsm_var_builder, "{sv}",
				"rxlev",
				g_variant_new_int32(ngbr_cell_info->gsm_list[gsm_count].rxlev));

			g_variant_builder_close(&gsm_var_builder);
		}

		dbg("UMTS Cell info List count: [%d]", ngbr_cell_info->umts_list_count);
		for (umts_count = 0; umts_count < ngbr_cell_info->umts_list_count; umts_count++) {
			g_variant_builder_open(&gsm_var_builder, G_VARIANT_TYPE("a{sv}"));

			g_variant_builder_add(&gsm_var_builder, "{sv}",
				"cell_id",
				g_variant_new_int32(ngbr_cell_info->umts_list[umts_count].cell_id));
			g_variant_builder_add(&gsm_var_builder, "{sv}",
				"lac",
				g_variant_new_int32(ngbr_cell_info->umts_list[umts_count].lac));
			g_variant_builder_add(&gsm_var_builder, "{sv}",
				"arfcn",
				g_variant_new_int32(ngbr_cell_info->umts_list[umts_count].arfcn));
			g_variant_builder_add(&gsm_var_builder, "{sv}",
				"psc",
				g_variant_new_int32(ngbr_cell_info->umts_list[umts_count].psc));
			g_variant_builder_add(&gsm_var_builder, "{sv}",
				"rscp",
				g_variant_new_int32(ngbr_cell_info->umts_list[umts_count].rscp));

			g_variant_builder_close(&gsm_var_builder);
		}

		dbg("GSM Cell info count: [%d] UMTS Cell info count: [%d]",
			gsm_count, umts_count);
	}
	gsm_list = g_variant_builder_end(&gsm_var_builder);
	umts_list = g_variant_builder_end(&umts_var_builder);

	telephony_network_complete_get_ngbr_cell_info(rsp_cb_data->interface_object,
		rsp_cb_data->invocation, result,
		gsm_count, gsm_list, umts_count, umts_list);
	g_variant_unref(gsm_list);
	g_variant_unref(umts_list);

	tcore_free(rsp_cb_data);
}

static gboolean dtapi_network_get_ngbr_cell_info(TelephonyNetwork *network,
	GDBusMethodInvocation *invocation, gpointer user_data)
{
	DbusRespCbData *rsp_cb_data = NULL;
	TelReturn result ;

	if (dtapi_check_access_control(invocation, AC_NETWORK, "r") == FALSE)
		return TRUE;

	rsp_cb_data = dtapi_create_resp_cb_data(network, invocation, NULL, 0);
	result = tcore_plugin_dispatch_request(user_data, TRUE,
		TCORE_COMMAND_NETWORK_GET_NEIGHBORING_CELL_INFO,
		NULL, 0,
		on_response_dtapi_network_get_ngbr_cell_info, rsp_cb_data);
	if (result != TEL_RETURN_SUCCESS) {
		dtapi_return_error(invocation, G_DBUS_ERROR_FAILED, "Dispatch failed");
		tcore_free(rsp_cb_data);
	}

	return TRUE;
}

gboolean dtapi_setup_network_interface(TelephonyObjectSkeleton *object,
	TcorePlugin *plugin)
{
	TelephonyNetwork *network;

	tcore_check_return_value_assert(NULL != object, FALSE);
	tcore_check_return_value_assert(NULL != plugin, FALSE);

	network = telephony_network_skeleton_new();
	telephony_object_skeleton_set_network(object, network);
	g_object_unref(network);

	dbg("network = %p", network);

	g_signal_connect (network,
			"handle-get-identity-info",
			G_CALLBACK (dtapi_network_get_identity_info),
			plugin);

	g_signal_connect (network,
			"handle-search",
			G_CALLBACK (dtapi_network_search),
			plugin);

	g_signal_connect (network,
			"handle-cancel-search",
			G_CALLBACK (dtapi_network_cancel_search),
			plugin);

	g_signal_connect (network,
			"handle-select-automatic",
			G_CALLBACK (dtapi_network_select_automatic),
			plugin);

	g_signal_connect (network,
			"handle-select-manual",
			G_CALLBACK (dtapi_network_select_manual),
			plugin);

	g_signal_connect (network,
			"handle-get-selection-mode",
			G_CALLBACK (dtapi_network_get_selection_mode),
			plugin);

	g_signal_connect (network,
			"handle-set-preferred-plmn",
			G_CALLBACK (dtapi_network_set_preferred_plmn),
			plugin);

	g_signal_connect (network,
			"handle-get-preferred-plmn",
			G_CALLBACK (dtapi_network_get_preferred_plmn),
			plugin);

	g_signal_connect (network,
			"handle-set-mode",
			G_CALLBACK (dtapi_network_set_mode),
			plugin);

	g_signal_connect (network,
			"handle-get-mode",
			G_CALLBACK (dtapi_network_get_mode),
			plugin);

	g_signal_connect (network,
			"handle-get-ngbr-cell-info",
			G_CALLBACK (dtapi_network_get_ngbr_cell_info),
			plugin);

	/* Initialize D-Bus properties */
	telephony_network_set_rssi(network, 0);
	telephony_network_set_lac(network, -1);
	telephony_network_set_cell_id(network, -1);
	telephony_network_set_rac(network, -1);
	telephony_network_set_act(network, TEL_NETWORK_ACT_UNKNOWN);
	telephony_network_set_cs_status(network, TEL_NETWORK_REG_STATUS_UNKNOWN);
	telephony_network_set_ps_status(network, TEL_NETWORK_REG_STATUS_UNKNOWN);

	return TRUE;
}

gboolean dtapi_handle_network_notification(TelephonyObjectSkeleton *object,
	TcorePlugin *plugin, TcoreNotification command,
	guint data_len, const void *data)
{
	TelephonyNetwork *network;

	tcore_check_return_value_assert(NULL != object, FALSE);
	tcore_check_return_value_assert(NULL != plugin, FALSE);
	tcore_check_return_value_assert(NULL != data, FALSE);

	network = telephony_object_peek_network(TELEPHONY_OBJECT(object));
	tcore_check_return_value_assert(NULL != network, FALSE);

	dbg("Notification!!! Command: [0x%x]", command);

	switch (command) {
	case TCORE_NOTIFICATION_NETWORK_REGISTRATION_STATUS: {
		const TelNetworkRegStatusInfo *registration_status = data;

		/* Update properties */
		telephony_network_set_act(network, registration_status->act);
		telephony_network_set_cs_status(network, registration_status->cs_status);
		telephony_network_set_ps_status(network, registration_status->ps_status);

		telephony_network_emit_registration_status(network,
				registration_status->cs_status,
				registration_status->ps_status,
				registration_status->act);
	}
	break;

	case TCORE_NOTIFICATION_NETWORK_LOCATION_CELLINFO: {
		const TelNetworkCellInfo *cell_info = data;

		/* Update properties */
		telephony_network_set_lac(network, cell_info->lac);
		telephony_network_set_cell_id(network, cell_info->cell_id);
		telephony_network_set_rac(network, cell_info->rac);

		telephony_network_emit_cell_info(network,
				cell_info->lac,
				cell_info->cell_id,
				cell_info->rac);
	}
	break;

	case TCORE_NOTIFICATION_NETWORK_IDENTITY: {
		const TelNetworkIdentityInfo *identity = data;

		telephony_network_emit_identity(network,
				identity->plmn,
				identity->short_name,
				identity->long_name);
	}
	break;

	case TCORE_NOTIFICATION_NETWORK_RSSI: {
		guint rssi = *(guint *)data;

		/* Update properties */
		telephony_network_set_rssi(network, rssi);
		telephony_network_emit_rssi(network, rssi);
	}
	break;

	case TCORE_NOTIFICATION_NETWORK_TIMEINFO: {
		const TelNetworkNitzInfoNoti *time_info = data;

		telephony_network_emit_time_info(network,
				time_info->year,
				time_info->month,
				time_info->day,
				time_info->hour,
				time_info->minute,
				time_info->second,
				time_info->gmtoff,
				time_info->isdst,
				time_info->dstoff,
				time_info->plmn);
	}
	break;

	default:
		err("Unsupported command");
	}

	return TRUE;
}
