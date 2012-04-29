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
#include <core_object.h>
#include <communicator.h>

#include <TapiCommon.h>
#include <TelSim.h>
#include <ITapiPS.h>

#include "tel_cs_conn.h"
#include "common.h"
#include "ts_utility.h"
#include "ts_common.h"
#include "ts_svr_req.h"
#include "ts_noti.h"
#include "modules.h"

void dbus_request_gprs(struct custom_data *ctx, TcorePlugin *plugin, int tapi_service_function, GArray* in_param1,
		GArray* in_param2, GArray* in_param3, GArray* in_param4, GArray** out_param1, GArray** out_param2,
		GArray** out_param3, GArray** out_param4, GError** error)
{
	/* legacy telephony */
	int api_err = TAPI_API_SUCCESS;
	tapi_ps_net_start_req_t info_activate;
	tapi_ps_btdun_pincontrol info_pincontrol;
	tapi_ps_net_stop_req_t info_deactivate;
	tapi_dbus_connection_name conn_name;

	/* new telephony */
	struct treq_ps_pdp_activate data_activate;
	struct treq_ps_pdp_deactivate data_deactivate;
	struct treq_ps_set_dun_pin_control data_pincontrol;
	TReturn ret;
	GSList *co_pslist = NULL;
	CoreObject *co_ps = NULL;
	UserRequest *ur = NULL;
	struct tcore_user_info ui = { 0, };

	conn_name = g_array_index(in_param4, tapi_dbus_connection_name, 0);

	co_pslist = tcore_plugin_get_core_objects_bytype(plugin, CORE_OBJECT_TYPE_PS);
	if (!co_pslist) {
		api_err = TAPI_API_NOT_SUPPORTED;
		goto OUT;
	}

	co_ps = (CoreObject *)co_pslist->data;
	g_slist_free(co_pslist);

	if (!co_ps) {
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
		case TAPI_CS_GPRS_STARTNETWORK: /* 0x700 */
			info_activate = g_array_index(in_param1, tapi_ps_net_start_req_t, 0);

			memset(&data_activate, 0, sizeof(struct treq_ps_pdp_activate));
			data_activate.context_id = info_activate.cont_id;
			data_activate.secondary_context_id = 0;
			strncpy(data_activate.apn, info_activate.pdp_info.apn, 102);
			strncpy(data_activate.pdp_address, (char *)info_activate.pdp_info.pdp_address, 20);
			data_activate.pdp_type = info_activate.pdp_info.pdp_type;
			strncpy(data_activate.username, (char *)info_activate.pdp_info.username, 32);
			strncpy(data_activate.password, (char *)info_activate.pdp_info.password, 32);
			strncpy(data_activate.dns1, (char *)info_activate.pdp_info.dns1, 16);
			strncpy(data_activate.dns2, (char *)info_activate.pdp_info.dns2, 16);
			data_activate.auth_type = info_activate.pdp_info.auth_type;

			tcore_user_request_set_data(ur, sizeof(struct treq_ps_pdp_activate), &data_activate);
			tcore_user_request_set_command(ur, TREQ_PS_SET_PDP_ACTIVATE);

			ret = tcore_communicator_dispatch_request(ctx->comm, ur);
			if (ret != TCORE_RETURN_SUCCESS) {
				api_err = TAPI_API_OPERATION_FAILED;
			}

			dbg("ret = 0x%x", ret);
			break;

		case TAPI_CS_GPRS_PDP_DEACTIVATION_SET: /* 0x701 */
			memset(&info_deactivate, 0, sizeof(tapi_ps_net_stop_req_t));
			info_deactivate = g_array_index(in_param1, tapi_ps_net_stop_req_t, 0);

			memset(&data_deactivate, 0, sizeof(struct treq_ps_pdp_deactivate));
			data_deactivate.context_id = info_deactivate.cont_id;
			data_deactivate.secondary_context_id = 0;

			tcore_user_request_set_data(ur, sizeof(struct treq_ps_pdp_deactivate), &data_deactivate);
			tcore_user_request_set_command(ur, TREQ_PS_SET_PDP_DEACTIVATE);

			ret = tcore_communicator_dispatch_request(ctx->comm, ur);
			if (ret != TCORE_RETURN_SUCCESS) {
				api_err = TAPI_API_OPERATION_FAILED;
			}

			dbg("ret = 0x%x", ret);
			break;

		case TAPI_CS_GPRS_BTDUN_PINCTRL_GET: /* 0x706 */
			memset(&info_pincontrol, 0, sizeof(tapi_ps_btdun_pincontrol));
			info_pincontrol = g_array_index(in_param1, tapi_ps_btdun_pincontrol, 0);

			data_pincontrol.signal = info_pincontrol.signal;
			data_pincontrol.status = info_pincontrol.status;

			tcore_user_request_set_data(ur, sizeof(struct treq_ps_set_dun_pin_control), &data_pincontrol);
			tcore_user_request_set_command(ur, TREQ_PS_SET_DUN_PIN_CONTROL);

			ret = tcore_communicator_dispatch_request(ctx->comm, ur);
			if (ret != TCORE_RETURN_SUCCESS) {
				api_err = TAPI_API_OPERATION_FAILED;
			}

			dbg("ret = 0x%x", ret);
			break;

		case TAPI_CS_GPRS_DATA_DORMANT: /* 0x702 */
		case TAPI_CS_GPRS_PORT_LIST_SET: /* 0x703 */
		case TAPI_CS_GPRS_PORT_LIST_GET: /* 0x704 */
		default:
			api_err = TAPI_API_NOT_SUPPORTED;
			break;
	}

OUT:
	if (api_err != TAPI_API_SUCCESS) {
		tcore_user_request_free(ur);
	}
	g_array_append_vals(*out_param1, &api_err, sizeof(int));
}

TReturn dbus_response_gprs(struct custom_data *ctx, UserRequest *ur, const char *appname,
		enum tcore_response_command command, unsigned int data_len, const void *data)
{
	const struct tresp_ps_set_pdp_activate *act_info = data;
	const struct tresp_ps_set_pdp_deactivate *deact_info = data;
	tapi_ps_pdp_info_t resp;

	switch (command) {
		case TRESP_PS_SET_PDP_ACTIVATE:
			resp.cont_id = act_info->context_id;
			resp.err = act_info->result;

			ts_delivery_event(ctx->EvtDeliveryHandle, TAPI_EVENT_CLASS_DATA, TAPI_EVENT_PS_PDP_ACT_RSP, appname, 0,
					TAPI_API_SUCCESS, sizeof(tapi_ps_pdp_info_t), (void *) &resp);
			break;

		case TRESP_PS_SET_PDP_DEACTIVATE:
			/* disconnect by user */
			resp.cont_id = deact_info->context_id;
			resp.err = deact_info->result;

			ts_delivery_event(ctx->EvtDeliveryHandle, TAPI_EVENT_CLASS_DATA, TAPI_EVENT_PS_PDP_DEACT_RSP, appname, 0,
					TAPI_API_SUCCESS, sizeof(tapi_ps_pdp_info_t), (void *) &resp);
			break;

		default:
			break;
	}

	return TRUE;
}

TReturn dbus_notification_gprs(struct custom_data *ctx, CoreObject *source, enum tcore_notification_command command,
		unsigned int data_len, const void *data)
{
	const struct tnoti_ps_call_status *call_status_info = data;
	const struct tnoti_ps_pdp_ipconfiguration *ip_info = data;
	const struct tnoti_ps_dun_pin_control *pin_info = data;

	tapi_ps_net_start_rsp_t ip_noti;
	tapi_ps_btdun_pincontrol pin_noti;

	switch (command) {
		case TNOTI_PS_CALL_STATUS:
			if (call_status_info->state == 1) {
			}
			else {
				/* disconnect */
				tapi_ps_pdp_info_t resp;

				resp.cont_id = call_status_info->context_id;
				resp.err = call_status_info->result;

				dbg("pdp disconnected by network");

				ts_delivery_event(ctx->EvtDeliveryHandle, TAPI_EVENT_CLASS_DATA, TAPI_EVENT_PS_PDP_DEACT_IND, NULL, 0,
						TAPI_API_SUCCESS, sizeof(tapi_ps_pdp_info_t), (void *) &resp);
			}
			break;

		case TNOTI_PS_PDP_IPCONFIGURATION:
			ip_noti.cont_id = ip_info->context_id;
			ip_noti.err = 0;
			ip_noti.pdp_info.field_flag = ip_info->field_flag;

			if (ip_info->field_flag & 0x0001) {
				memcpy(&ip_noti.pdp_info.ip_address, ip_info->ip_address, 4);
			}

			if (ip_info->field_flag & 0x0002) {
				memcpy(&ip_noti.pdp_info.primary_dns, ip_info->primary_dns, 4);
			}

			if (ip_info->field_flag & 0x0004) {
				memcpy(&ip_noti.pdp_info.secondary_dns, ip_info->secondary_dns, 4);
			}

			if (ip_info->field_flag & 0x0008) {
				memcpy(&ip_noti.pdp_info.gateway, ip_info->gateway, 4);
			}

			if (ip_info->field_flag & 0x0010) {
				memcpy(&ip_noti.pdp_info.subnet_mask, ip_info->subnet_mask, 4);
			}

			snprintf(ip_noti.devname, TAPI_PDP_NAME_LEN_MAX, "pdp%d", ip_noti.cont_id - 1);

			ts_delivery_event(ctx->EvtDeliveryHandle, TAPI_EVENT_CLASS_DATA, TAPI_EVENT_PS_PDP_ACT_IPCONFIG_INFO, NULL, 0,
					TAPI_API_SUCCESS, sizeof(tapi_ps_pdp_info_t), (void *) &ip_noti);
			break;

		case TNOTI_PS_DUN_PIN_CONTROL:
			pin_noti.signal = pin_info->signal;
			pin_noti.status = pin_info->status;
			ts_delivery_event(ctx->EvtDeliveryHandle, TAPI_EVENT_CLASS_DATA, TAPI_EVENT_PS_BTDUN_PINCONTROL_NOTI, NULL, 0,
					TAPI_API_SUCCESS, sizeof(tapi_ps_btdun_pincontrol), &pin_noti);
			break;

		case TNOTI_PS_EXTERNAL_CALL:
			ts_delivery_event(ctx->EvtDeliveryHandle, TAPI_EVENT_CLASS_DATA, TAPI_EVENT_PS_EXTERNAL_CALL_IND, NULL, 0,
					TAPI_API_SUCCESS, 0, NULL);
			break;

		default:
			break;
	}

	return TRUE;
}
