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
#include <unistd.h>
#include <stdlib.h>
#include <glib.h>

#include <tcore.h>
#include <plugin.h>
#include <server.h>
#include <storage.h>
#include <user_request.h>
#include <co_network.h>
#include <communicator.h>

#include <TapiCommon.h>
#include <TelSim.h>
#include <TelNetwork.h>

#include "tel_cs_conn.h"
#include "common.h"
#include "ts_utility.h"
#include "ts_common.h"
#include "ts_svr_req.h"
#include "ts_noti.h"
#include "modules.h"

#if 0
typedef enum
{
	TAPI_NETWORK_SYSTEM_NO_SRV,				/**< No Service available */
	TAPI_NETWORK_SYSTEM_GSM,				/**< Available service is GSM  */
	TAPI_NETWORK_SYSTEM_GPRS,				/**< Available service is GPRS */
	TAPI_NETWORK_SYSTEM_EGPRS,				/**< Available service is EGPRS  */
	TAPI_NETWORK_SYSTEM_PCS1900,			/**< Available service is PCS1900 band */
	TAPI_NETWORK_SYSTEM_UMTS,				/**< Available service is UMTS  */
	TAPI_NETWORK_SYSTEM_GSM_AND_UMTS,		/**< Both GSM and UMTS systems available */
	TAPI_NETWORK_SYSTEM_HSDPA,				/**< Available service is hsdpa */
	TAPI_NETWORK_SYSTEM_IS95A,				/**< Available service is IS95A */
	TAPI_NETWORK_SYSTEM_IS95B,				/**< Available service is IS95B */
	TAPI_NETWORK_SYSTEM_CDMA_1X,			/**< Available service is CDMA 1X */
	TAPI_NETWORK_SYSTEM_EVDO_REV_0,	/**< Available service is EV-DO rev0 */
	TAPI_NETWORK_SYSTEM_1X_EVDO_REV_0_HYBRID, /**< Available service is  1X and EV-DO rev0 */
	TAPI_NETWORK_SYSTEM_EVDO_REV_A,	/**< Available service is  EV-DO revA */
	TAPI_NETWORK_SYSTEM_1X_EVDO_REV_A_HYBRID, /**< Available service is 1X and EV-DO revA */
	TAPI_NETWORK_SYSTEM_EVDV,		/**< Available service is EV-DV */
} TelNetworkSystemType_t;

enum telephony_network_access_technology {
	NETWORK_ACT_UNKNOWN = 0x0,
	NETWORK_ACT_GSM = 0x1,
	NETWORK_ACT_GPRS,
	NETWORK_ACT_EGPRS,
	NETWORK_ACT_UMTS = 0x4,
	NETWORK_ACT_UTRAN = 0x4,
	NETWORK_ACT_GSM_UTRAN,
	NETWORK_ACT_IS95A = 0x11,
	NETWORK_ACT_IS95B,
	NETWORK_ACT_CDMA_1X,
	NETWORK_ACT_EVDO_REV0,
	NETWORK_ACT_CDMA_1X_EVDO_REV0,
	NETWORK_ACT_EVDO_REVA,
	NETWORK_ACT_CDMA_1X_EVDO_REVA,
	NETWORK_ACT_EVDV,
	NETWORK_ACT_LTE = 0x21,
	NETWORK_ACT_NOT_SPECIFIED = 0xFF
};
#endif

static TelNetworkSystemType_t _act_table[] = {
		[NETWORK_ACT_GSM] =  TAPI_NETWORK_SYSTEM_GSM,
		[NETWORK_ACT_GPRS] = TAPI_NETWORK_SYSTEM_GPRS,
		[NETWORK_ACT_EGPRS] = TAPI_NETWORK_SYSTEM_EGPRS,
		[NETWORK_ACT_UMTS] = TAPI_NETWORK_SYSTEM_UMTS,
		[NETWORK_ACT_GSM_UTRAN] = TAPI_NETWORK_SYSTEM_GSM_AND_UMTS,
		[NETWORK_ACT_IS95A] = TAPI_NETWORK_SYSTEM_IS95A,
		[NETWORK_ACT_IS95B] = TAPI_NETWORK_SYSTEM_IS95B,
		[NETWORK_ACT_CDMA_1X] = TAPI_NETWORK_SYSTEM_CDMA_1X,
		[NETWORK_ACT_EVDO_REV0] = TAPI_NETWORK_SYSTEM_EVDO_REV_0,
		[NETWORK_ACT_CDMA_1X_EVDO_REV0] = TAPI_NETWORK_SYSTEM_1X_EVDO_REV_0_HYBRID,
		[NETWORK_ACT_EVDO_REVA] = TAPI_NETWORK_SYSTEM_EVDO_REV_A,
		[NETWORK_ACT_CDMA_1X_EVDO_REVA] = TAPI_NETWORK_SYSTEM_1X_EVDO_REV_A_HYBRID,
		[NETWORK_ACT_EVDV] = TAPI_NETWORK_SYSTEM_EVDV,
		[NETWORK_ACT_NOT_SPECIFIED] = TAPI_NETWORK_SYSTEM_NO_SRV,
};

static char *_get_network_name_by_plmn(CoreObject *o, const char *plmn)
{
	struct tcore_network_operator_info *noi = NULL;
	char mcc[4] = { 0, };
	char mnc[4] = { 0, };

	if (!plmn)
		return NULL;

	snprintf(mcc, 4, "%s", plmn);
	snprintf(mnc, 4, "%s", plmn+3);

	if (mnc[2] == '#')
		mnc[2] = '\0';

	noi = tcore_network_operator_info_find(o, mcc, mnc);
	if (noi) {
		dbg("%s-%s: country=[%s], oper=[%s]", mcc, mnc, noi->country, noi->name);
		return noi->name;
	}
	else {
		dbg("%s-%s: no network operator name", mcc, mnc);
	}

	return NULL;
}

void dbus_request_network(struct custom_data *ctx, TcorePlugin *plugin, int tapi_service_function, GArray* in_param1,
		GArray* in_param2, GArray* in_param3, GArray* in_param4, GArray** out_param1, GArray** out_param2,
		GArray** out_param3, GArray** out_param4, GError** error)
{
	int api_err = TAPI_API_SUCCESS;
	tapi_dbus_connection_name conn_name;
	unsigned int legacy_plmn;
	TelNetworkServiceDomain_t info_service_domain;
	TelNetworkPrefferedPlmnInfo_t *info_preferred_plmn_info;
	TelNetworkPrefferedPlmnOp_t info_preferred_plmn_operation;
	TelNetworkBandPreferred_t info_band_mode;
	TelNetworkBand_t info_band;
	TelNetworkMode_t *info_order;
	TelNetworkPowerOnAttach_t info_power_on_attach;
	int request_id = 0;

	struct treq_network_set_plmn_selection_mode data_plmn_select;
	struct treq_network_set_service_domain data_service_domain;
	struct treq_network_set_band data_band;
	struct treq_network_set_preferred_plmn data_preferred_plmn;
	struct treq_network_set_order data_order;
	struct treq_network_set_power_on_attach data_power_on_attach;
	const struct tresp_network_search *data_network_search = NULL;
	TReturn ret;
	GSList *co_list = NULL;
	CoreObject *co_network = NULL;
	UserRequest *ur = NULL;
	struct tcore_user_info ui = { 0, };
	int i;

	conn_name = g_array_index(in_param4, tapi_dbus_connection_name, 0);

	co_list = tcore_plugin_get_core_objects_bytype(plugin, CORE_OBJECT_TYPE_NETWORK);
	if (!co_list) {
		api_err = TAPI_API_NOT_SUPPORTED;
		goto OUT;
	}

	co_network = (CoreObject *)co_list->data;
	g_slist_free(co_list);

	if (!co_network) {
		api_err = TAPI_API_NOT_SUPPORTED;
		goto OUT;
	}

	ur = tcore_user_request_new(ctx->comm, tcore_plugin_get_description(plugin)->name);
	if (!ur) {
		api_err = TAPI_API_SERVER_FAILURE;
		goto OUT;
	}

	ui.appname = conn_name.name;
	tcore_user_request_set_user_info(ur, &ui);

	switch (tapi_service_function) {
		case TAPI_CS_NETWORK_SEARCH: /* 0x402 */
			tcore_user_request_set_data(ur, 0, NULL);
			tcore_user_request_set_command(ur, TREQ_NETWORK_SEARCH);

			ret = tcore_communicator_dispatch_request(ctx->comm, ur);
			if (ret != TCORE_RETURN_SUCCESS) {
				api_err = TAPI_API_OPERATION_FAILED;
			}

			dbg("ret = 0x%x", ret);
			break;

		case TAPI_CS_NETWORK_SELECT_AUTOMATIC: /* 0x410 */
			data_plmn_select.mode = NETWORK_SELECT_MODE_GSM_AUTOMATIC;

			tcore_user_request_set_data(ur, sizeof(struct treq_network_set_plmn_selection_mode), &data_plmn_select);
			tcore_user_request_set_command(ur, TREQ_NETWORK_SET_PLMN_SELECTION_MODE);

			ret = tcore_communicator_dispatch_request(ctx->comm, ur);
			if (ret != TCORE_RETURN_SUCCESS) {
				api_err = TAPI_API_OPERATION_FAILED;
			}

			dbg("ret = 0x%x", ret);
			break;

		case TAPI_CS_NETWORK_SELECT_MANUAL: /* 0x411 */
			legacy_plmn = g_array_index(in_param1, unsigned int, 0);
			data_plmn_select.mode = NETWORK_SELECT_MODE_GSM_MANUAL;
			snprintf(data_plmn_select.plmn, 6, "%u", legacy_plmn);
			if (strlen(data_plmn_select.plmn) <= 5)
				data_plmn_select.plmn[5] = '#';

			data_network_search = ctx->plmn_list_search_result_cache;
			for (i = 0; i < data_network_search->list_count; i++) {
				if (!g_strcmp0(data_network_search->list[i].plmn, data_plmn_select.plmn)) {
					data_plmn_select.act = data_network_search->list[i].act;
					break;
				}
			}

			tcore_user_request_set_data(ur, sizeof(struct treq_network_set_plmn_selection_mode), &data_plmn_select);
			tcore_user_request_set_command(ur, TREQ_NETWORK_SET_PLMN_SELECTION_MODE);

			ret = tcore_communicator_dispatch_request(ctx->comm, ur);
			if (ret != TCORE_RETURN_SUCCESS) {
				api_err = TAPI_API_OPERATION_FAILED;
			}

			dbg("ret = 0x%x", ret);
			break;

		case TAPI_CS_NETWORK_GETSELECTIONMODE: /* 0x401 */
			tcore_user_request_set_data(ur, 0, NULL);
			tcore_user_request_set_command(ur, TREQ_NETWORK_GET_PLMN_SELECTION_MODE);

			ret = tcore_communicator_dispatch_request(ctx->comm, ur);
			if (ret != TCORE_RETURN_SUCCESS) {
				api_err = TAPI_API_OPERATION_FAILED;
			}

			dbg("ret = 0x%x", ret);
			break;

		case TAPI_CS_NETWORK_SETSERVICEDOMAIN: /* 0x405 */
			info_service_domain = g_array_index(in_param1, TelNetworkServiceDomain_t, 0);
			data_service_domain.domain = info_service_domain;

			tcore_user_request_set_data(ur, sizeof(struct treq_network_set_service_domain), &data_service_domain);
			tcore_user_request_set_command(ur, TREQ_NETWORK_SET_SERVICE_DOMAIN);

			ret = tcore_communicator_dispatch_request(ctx->comm, ur);
			if (ret != TCORE_RETURN_SUCCESS) {
				api_err = TAPI_API_OPERATION_FAILED;
			}

			dbg("ret = 0x%x", ret);
			break;

		case TAPI_CS_NETWORK_GETSERVICEDOMAIN: /* 0x406 */
			tcore_user_request_set_data(ur, 0, NULL);
			tcore_user_request_set_command(ur, TREQ_NETWORK_GET_SERVICE_DOMAIN);

			ret = tcore_communicator_dispatch_request(ctx->comm, ur);
			if (ret != TCORE_RETURN_SUCCESS) {
				api_err = TAPI_API_OPERATION_FAILED;
			}

			dbg("ret = 0x%x", ret);
			break;

		case TAPI_CS_NETWORK_SETNETWORKBAND: /* 0x403 */
			info_band_mode = g_array_index(in_param1,TelNetworkBandPreferred_t, 0);
			info_band = g_array_index(in_param2,TelNetworkBand_t, 0);

			data_band.mode = info_band_mode;
			data_band.band = info_band;

			tcore_user_request_set_data(ur, sizeof(struct treq_network_set_band), &data_band);
			tcore_user_request_set_command(ur, TREQ_NETWORK_SET_BAND);

			ret = tcore_communicator_dispatch_request(ctx->comm, ur);
			if (ret != TCORE_RETURN_SUCCESS) {
				api_err = TAPI_API_OPERATION_FAILED;
			}

			dbg("ret = 0x%x", ret);
			break;

		case TAPI_CS_NETWORK_GETNETWORKBAND: /* 0x404 */
			tcore_user_request_set_data(ur, 0, NULL);
			tcore_user_request_set_command(ur, TREQ_NETWORK_GET_BAND);

			ret = tcore_communicator_dispatch_request(ctx->comm, ur);
			if (ret != TCORE_RETURN_SUCCESS) {
				api_err = TAPI_API_OPERATION_FAILED;
			}

			dbg("ret = 0x%x", ret);
			break;

		case TAPI_CS_NETWORK_SETPREFPLMN: /* 0x409 */
			info_preferred_plmn_operation = g_array_index(in_param1,TelNetworkPrefferedPlmnOp_t, 0);
			info_preferred_plmn_info = &g_array_index(in_param2,TelNetworkPrefferedPlmnInfo_t, 0);

			data_preferred_plmn.operation = info_preferred_plmn_operation;
			memcpy(data_preferred_plmn.plmn, info_preferred_plmn_info->Plmn, 6);

			if (strlen((char *)info_preferred_plmn_info->Plmn) == 4) {
				data_preferred_plmn.plmn[4] = data_preferred_plmn.plmn[3];
				data_preferred_plmn.plmn[3] = '0';
			}

			if (strlen((char *)info_preferred_plmn_info->Plmn) <= 5) {
				data_preferred_plmn.plmn[5] = '#';
			}

			data_preferred_plmn.ef_index = info_preferred_plmn_info->Index;

			switch (info_preferred_plmn_info->SystemType) {
				case TAPI_NETWORK_SYSTEM_GSM:
					data_preferred_plmn.act = NETWORK_ACT_GSM;
					break;

				case TAPI_NETWORK_SYSTEM_UMTS:
					data_preferred_plmn.act = NETWORK_ACT_UMTS;
					break;

				case TAPI_NETWORK_SYSTEM_GPRS:
					data_preferred_plmn.act = NETWORK_ACT_GPRS;
					break;

				case TAPI_NETWORK_SYSTEM_EGPRS:
					data_preferred_plmn.act = NETWORK_ACT_EGPRS;
					break;

				case TAPI_NETWORK_SYSTEM_GSM_AND_UMTS:
					data_preferred_plmn.act = NETWORK_ACT_UMTS;
					break;

				default:
					break;
			}

			tcore_user_request_set_data(ur, sizeof(struct treq_network_set_preferred_plmn), &data_preferred_plmn);
			tcore_user_request_set_command(ur, TREQ_NETWORK_SET_PREFERRED_PLMN);

			ret = tcore_communicator_dispatch_request(ctx->comm, ur);
			if (ret != TCORE_RETURN_SUCCESS) {
				api_err = TAPI_API_OPERATION_FAILED;
			}

			dbg("ret = 0x%x", ret);
			break;

		case TAPI_CS_NETWORK_GETPREFPLMN: /* 0x40A */
			tcore_user_request_set_data(ur, 0, NULL);
			tcore_user_request_set_command(ur, TREQ_NETWORK_GET_PREFERRED_PLMN);

			ret = tcore_communicator_dispatch_request(ctx->comm, ur);
			if (ret != TCORE_RETURN_SUCCESS) {
				api_err = TAPI_API_OPERATION_FAILED;
			}

			dbg("ret = 0x%x", ret);
			break;

		case TAPI_CS_NETWORK_SETNETWORKORDER: /* 0x40000412 */
			info_order = &g_array_index(in_param1,TelNetworkMode_t, 0);
			data_order.order = *info_order;

			tcore_user_request_set_data(ur, sizeof(struct treq_network_set_order), &data_order);
			tcore_user_request_set_command(ur, TREQ_NETWORK_SET_ORDER);

			ret = tcore_communicator_dispatch_request(ctx->comm, ur);
			if (ret != TCORE_RETURN_SUCCESS) {
				api_err = TAPI_API_OPERATION_FAILED;
			}

			dbg("ret = 0x%x", ret);
			break;

		case TAPI_CS_NETWORK_GETNETWORKORDER: /* 0x40000413 */
			tcore_user_request_set_data(ur, 0, NULL);
			tcore_user_request_set_command(ur, TREQ_NETWORK_GET_ORDER);

			ret = tcore_communicator_dispatch_request(ctx->comm, ur);
			if (ret != TCORE_RETURN_SUCCESS) {
				api_err = TAPI_API_OPERATION_FAILED;
			}

			dbg("ret = 0x%x", ret);
			break;

		case TAPI_CS_NETWORK_SETPOWERONATTACH: /* 0x40000414 */
			info_power_on_attach = g_array_index(in_param1,TelNetworkPowerOnAttach_t, 0);
			data_power_on_attach.enable = info_power_on_attach;

			tcore_user_request_set_data(ur, sizeof(struct treq_network_set_power_on_attach), &data_power_on_attach);
			tcore_user_request_set_command(ur, TREQ_NETWORK_SET_POWER_ON_ATTACH);

			ret = tcore_communicator_dispatch_request(ctx->comm, ur);
			if (ret != TCORE_RETURN_SUCCESS) {
				api_err = TAPI_API_OPERATION_FAILED;
			}

			dbg("ret = 0x%x", ret);
			break;

		case TAPI_CS_NETWORK_GETPOWERONATTACH: /* 0x40000415 */
			tcore_user_request_set_data(ur, 0, NULL);
			tcore_user_request_set_command(ur, TREQ_NETWORK_GET_POWER_ON_ATTACH);

			ret = tcore_communicator_dispatch_request(ctx->comm, ur);
			if (ret != TCORE_RETURN_SUCCESS) {
				api_err = TAPI_API_OPERATION_FAILED;
			}

			dbg("ret = 0x%x", ret);
			break;

		case TAPI_CS_NETWORK_CANCELMANUALSEARCH: /* 0x4000416 */
			tcore_user_request_set_data(ur, 0, NULL);
			tcore_user_request_set_command(ur, TREQ_NETWORK_SET_CANCEL_MANUAL_SEARCH);

			ret = tcore_communicator_dispatch_request(ctx->comm, ur);
			if (ret != TCORE_RETURN_SUCCESS) {
				api_err = TAPI_API_OPERATION_FAILED;
			}

			dbg("ret = 0x%x", ret);
			break;

		/* Unused */
		case TAPI_CS_NETWORK_SETNETWORKMODE:
		case TAPI_CS_NETWORK_GETNETWORKMODE:
		case TAPI_CS_NETWORK_SETROAMINGMODE:
		case TAPI_CS_NETWORK_GETROAMINGMODE:
		case TAPI_CS_NETWORK_SETCDMAHYBRIDMODE:
		case TAPI_CS_NETWORK_GETCDMAHYBRIDMODE:
		case TAPI_CS_NETWORK_CANCELMANUALSELECTION:
		default:
			api_err = TAPI_API_NOT_SUPPORTED;
			break;
	}

OUT:
	if (api_err != TAPI_API_SUCCESS) {
		tcore_user_request_free(ur);
	}
	g_array_append_vals(*out_param1, &api_err, sizeof(int));
	g_array_append_vals(*out_param2, &request_id, sizeof(int));
}

TReturn dbus_response_network(struct custom_data *ctx, UserRequest *ur, const char *appname,
		enum tcore_response_command command, unsigned int data_len, const void *data)
{
	const struct tresp_network_search *data_network_search = data;
	const struct tresp_network_set_plmn_selection_mode *data_set_plmn_selection_mode = data;
	const struct tresp_network_get_plmn_selection_mode *data_get_plmn_selection_mode = data;
	const struct tresp_network_set_service_domain *data_set_service_domain = data;
	const struct tresp_network_get_service_domain *data_get_service_domain = data;
	const struct tresp_network_set_band *data_set_band = data;
	const struct tresp_network_get_band *data_get_band = data;
	const struct tresp_network_set_preferred_plmn *data_set_preferred_plmn = data;
	const struct tresp_network_get_preferred_plmn *data_get_preferred_plmn = data;
	const struct tresp_network_set_order *data_set_order = data;
	const struct tresp_network_get_order *data_get_order = data;
	const struct tresp_network_set_power_on_attach *data_set_power_on_attach = data;
	const struct tresp_network_get_power_on_attach *data_get_power_on_attach = data;
	const struct tresp_network_set_cancel_manual_search *data_set_cancel_manual_search = data;

	TelNetworkPlmnList_t info_plmn_list;
	TelNetworkSelectionMode_t info_selection_mode;
	TelNetworkServiceDomain_t info_service_domain;
	TelNetworkBand_t info_network_band;
	TelNetworkPrefferedPlmnList_t info_preferred_plmn_list;
	TelNetworkMode_t info_network_mode;
	TelNetworkPowerOnAttach_t info_power_on_attach;

	int i = 0;
	int request_id = 0;
	int ret = TAPI_API_SUCCESS;
	char *buf = NULL;

	GSList *co_list;
	CoreObject *co_network;
	char *modem_name = NULL;
	TcorePlugin *p = NULL;

	modem_name = tcore_user_request_get_modem_name(ur);
	if (!modem_name)
		return FALSE;

	p = tcore_server_find_plugin(ctx->server, modem_name);
	free(modem_name);
	if (!p)
		return FALSE;

	co_list = tcore_plugin_get_core_objects_bytype(p, CORE_OBJECT_TYPE_NETWORK);
	if (!co_list) {
		return FALSE;
	}

	co_network = (CoreObject *)co_list->data;
	g_slist_free(co_list);

	if (!co_network) {
		return FALSE;
	}

	switch (command) {
		case TRESP_NETWORK_SEARCH:
			memset(&info_plmn_list, 0, sizeof(TelNetworkPlmnList_t));
			info_plmn_list.networks_count = data_network_search->list_count;

			dbg("size_of(TelNetworkPlmnList_t) = %d", sizeof(TelNetworkPlmnList_t));
			dbg("list_count = %d", info_plmn_list.networks_count);
			for (i=0; i<info_plmn_list.networks_count; i++) {
				info_plmn_list.network_list[i].plmn_id = atoi(data_network_search->list[i].plmn);
				info_plmn_list.network_list[i].type_of_plmn = data_network_search->list[i].status;
				info_plmn_list.network_list[i].access_technology = _act_table[data_network_search->list[i].act];

				buf = _get_network_name_by_plmn(co_network, data_network_search->list[i].plmn);
				if (buf) {
					memcpy(info_plmn_list.network_list[i].network_name, buf, 40);
					memcpy(info_plmn_list.network_list[i].service_provider_name, buf, 40);
				}
				else {
					snprintf(info_plmn_list.network_list[i].network_name, 40, "%d", info_plmn_list.network_list[i].plmn_id);
					snprintf(info_plmn_list.network_list[i].service_provider_name, 40, "%d", info_plmn_list.network_list[i].plmn_id);
				}

				dbg("[%d] plmn_id = %d", i, info_plmn_list.network_list[i].plmn_id);
				dbg("[%d] type_of_plmn = %d", i, info_plmn_list.network_list[i].type_of_plmn);
				dbg("[%d] access_technology = %d", i, info_plmn_list.network_list[i].access_technology);
				dbg("[%d] network_name = %s", i, info_plmn_list.network_list[i].network_name);
				dbg("[%d] service_provider_name = %s", i, info_plmn_list.network_list[i].service_provider_name);
			}

			/* Save cache */
			if (!ctx->plmn_list_search_result_cache)
				ctx->plmn_list_search_result_cache = calloc(sizeof(struct tresp_network_search), 1);
			memcpy(ctx->plmn_list_search_result_cache, data_network_search, sizeof(struct tresp_network_search));

			return ts_delivery_event(ctx->EvtDeliveryHandle, TAPI_EVENT_CLASS_NETWORK, TAPI_EVENT_NETWORK_SEARCH_CNF,
					appname, request_id, ret, sizeof(TelNetworkPlmnList_t), (void*) &info_plmn_list);
			break;

		case TRESP_NETWORK_SET_PLMN_SELECTION_MODE:
			if (data_set_plmn_selection_mode->result != TCORE_RETURN_SUCCESS)
				ret = TAPI_API_OPERATION_FAILED;

			return ts_delivery_event(ctx->EvtDeliveryHandle, TAPI_EVENT_CLASS_NETWORK, TAPI_EVENT_NETWORK_SELECT_CNF,
					appname, request_id, ret, 0, NULL);
			break;

		case TRESP_NETWORK_GET_PLMN_SELECTION_MODE:
			info_selection_mode = data_get_plmn_selection_mode->mode;

			switch (data_get_plmn_selection_mode->mode) {
				case NETWORK_SELECT_MODE_GLOBAL_AUTOMATIC:
					info_selection_mode = TAPI_NETWORK_SELECTIONMODE_GLOBAL_AUTOMAITIC;
					break;

				case NETWORK_SELECT_MODE_GSM_AUTOMATIC:
					info_selection_mode = TAPI_NETWORK_SELECTIONMODE_AUTOMATIC;
					break;

				case NETWORK_SELECT_MODE_GSM_MANUAL:
					info_selection_mode = TAPI_NETWORK_SELECTIONMODE_MANUAL;
					break;

				case NETWORK_SELECT_MODE_CDMA:
					info_selection_mode = TAPI_NETWORK_SELECTIONMODE_CDMA;
					break;

				default:
					break;
			}

			return ts_delivery_event(ctx->EvtDeliveryHandle, TAPI_EVENT_CLASS_NETWORK,
					TAPI_EVENT_NETWORK_GETSELECTIONMODE_CNF, appname, request_id, ret,
					sizeof(TelNetworkSelectionMode_t), (void*) &info_selection_mode);

			break;

		case TRESP_NETWORK_SET_SERVICE_DOMAIN:
			if (data_set_service_domain->result != TCORE_RETURN_SUCCESS)
				ret = TAPI_API_OPERATION_FAILED;

			return ts_delivery_event(ctx->EvtDeliveryHandle, TAPI_EVENT_CLASS_NETWORK, TAPI_EVENT_NETWORK_SETSERVICEDOMAIN_CNF,
					appname, request_id, ret, 0, NULL);
			break;

		case TRESP_NETWORK_GET_SERVICE_DOMAIN:
			info_service_domain = data_get_service_domain->domain;

			return ts_delivery_event(ctx->EvtDeliveryHandle, TAPI_EVENT_CLASS_NETWORK,
					TAPI_EVENT_NETWORK_GETSERVICEDOMAIN_CNF, appname, request_id, ret, sizeof(TelNetworkServiceDomain_t),
					(void*) &info_service_domain);
			break;

		case TRESP_NETWORK_SET_BAND:
			if (data_set_band->result != TCORE_RETURN_SUCCESS)
				ret = TAPI_API_OPERATION_FAILED;

			return ts_delivery_event(ctx->EvtDeliveryHandle, TAPI_EVENT_CLASS_NETWORK, TAPI_EVENT_NETWORK_SETNWBAND_CNF,
					appname, request_id, ret, 0, NULL);
			break;

		case TRESP_NETWORK_GET_BAND:
			info_network_band = data_get_band->band;

			return ts_delivery_event(ctx->EvtDeliveryHandle, TAPI_EVENT_CLASS_NETWORK, TAPI_EVENT_NETWORK_GETNWBAND_CNF,
					appname, request_id, ret, sizeof(TelNetworkBand_t), (void*) &info_network_band);
			break;

		case TRESP_NETWORK_SET_PREFERRED_PLMN:
			if (data_set_preferred_plmn->result != TCORE_RETURN_SUCCESS)
				ret = TAPI_API_OPERATION_FAILED;

			return ts_delivery_event(ctx->EvtDeliveryHandle, TAPI_EVENT_CLASS_NETWORK,
					TAPI_EVENT_NETWORK_SETPREFFEREDPLMN_CNF, appname, request_id, ret, 0, NULL);
			break;

		case TRESP_NETWORK_GET_PREFERRED_PLMN:
			info_preferred_plmn_list.NumOfPreffPlmns = data_get_preferred_plmn->list_count;

			for (i = 0; i < data_get_preferred_plmn->list_count; i++) {
				info_preferred_plmn_list.PreffPlmnRecord[i].Index = data_get_preferred_plmn->list[i].ef_index;
				memcpy(info_preferred_plmn_list.PreffPlmnRecord[i].Plmn, data_get_preferred_plmn->list[i].plmn, 6);

				buf = _get_network_name_by_plmn(co_network, data_network_search->list[i].plmn);
				if (buf) {
					memcpy(info_preferred_plmn_list.PreffPlmnRecord[i].network_name, buf, 40);
					memcpy(info_preferred_plmn_list.PreffPlmnRecord[i].service_provider_name, buf, 40);
				}
				else {
					memcpy(info_preferred_plmn_list.PreffPlmnRecord[i].network_name,
							data_get_preferred_plmn->list[i].plmn, 6);
					memcpy(info_preferred_plmn_list.PreffPlmnRecord[i].service_provider_name,
							data_get_preferred_plmn->list[i].plmn, 6);
				}

				info_preferred_plmn_list.PreffPlmnRecord[i].SystemType = data_get_preferred_plmn->list[i].act;
			}

			return ts_delivery_event(ctx->EvtDeliveryHandle, TAPI_EVENT_CLASS_NETWORK,
					TAPI_EVENT_NETWORK_GETPREFFEREDPLMN_CNF, appname, request_id, ret, sizeof(TelNetworkPrefferedPlmnList_t),
					(void*) &info_preferred_plmn_list);
			break;

		case TRESP_NETWORK_SET_ORDER:
			if (data_set_order->result != TCORE_RETURN_SUCCESS)
				ret = TAPI_API_OPERATION_FAILED;

			return ts_delivery_event(ctx->EvtDeliveryHandle, TAPI_EVENT_CLASS_NETWORK,
					TAPI_EVENT_NETWORK_SETNWORDER_CNF, appname, request_id, ret, 0, NULL);
			break;

		case TRESP_NETWORK_GET_ORDER:
			info_network_mode = data_get_order->order;

			return ts_delivery_event(ctx->EvtDeliveryHandle, TAPI_EVENT_CLASS_NETWORK,
					TAPI_EVENT_NETWORK_GETNWORDER_CNF, appname, request_id, ret, sizeof(TelNetworkMode_t),
					(void*) &info_network_mode);
			break;

		case TRESP_NETWORK_SET_POWER_ON_ATTACH:
			if (data_set_power_on_attach->result != TCORE_RETURN_SUCCESS)
				ret = TAPI_API_OPERATION_FAILED;

			return ts_delivery_event(ctx->EvtDeliveryHandle, TAPI_EVENT_CLASS_NETWORK,
					TAPI_EVENT_NETWORK_SETPOWERONATTACH_CNF, appname, request_id, ret, 0, NULL);
			break;

		case TRESP_NETWORK_GET_POWER_ON_ATTACH:
			info_power_on_attach = data_get_power_on_attach->enabled;

			return ts_delivery_event(ctx->EvtDeliveryHandle, TAPI_EVENT_CLASS_NETWORK,
					TAPI_EVENT_NETWORK_GETPOWERONATTACH_CNF, appname, request_id, ret, sizeof(TelNetworkPowerOnAttach_t),
					(void*) &info_power_on_attach);
			break;

		case TRESP_NETWORK_SET_CANCEL_MANUAL_SEARCH:
			if (data_set_cancel_manual_search->result != TCORE_RETURN_SUCCESS)
				ret = TAPI_API_OPERATION_FAILED;

			return ts_delivery_event(ctx->EvtDeliveryHandle, TAPI_EVENT_CLASS_NETWORK,
					TAPI_EVENT_NETWORK_CANCELMANUALSEARCH_CNF, appname, request_id, ret, 0, NULL);
			break;

		case TRESP_NETWORK_GET_SERVING_NETWORK:
			/* not support current tapi */
			break;

		default:
			break;
	}
	return TRUE;
}

TReturn dbus_notification_network(struct custom_data *ctx, CoreObject *source, enum tcore_notification_command command,
		unsigned int data_len, const void *data)
{
	dbg("command = 0x%x", command);

	switch (command) {
		default:
			break;
	}

	return TRUE;
}
