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
#include <co_sms.h>
#include <communicator.h>

#include <TapiCommon.h>
#include <TelNetText.h>

#include "tel_cs_conn.h"
#include "common.h"
#include "ts_utility.h"
#include "ts_common.h"
#include "ts_svr_req.h"
#include "ts_noti.h"
#include "modules.h"

void dbus_request_sms(struct custom_data *ctx, TcorePlugin *plugin, int tapi_service_function, GArray* in_param1,
		GArray* in_param2, GArray* in_param3, GArray* in_param4, GArray** out_param1, GArray** out_param2,
		GArray** out_param3, GArray** out_param4, GError** error)
{
	/* legacy telephony */
	int api_err = TAPI_API_SUCCESS;
	tapi_dbus_connection_name conn_name;

	/* new telephony */
	struct treq_sms_send_umts_msg sendUmtsMsg;
	struct treq_sms_send_cdma_msg cdmaMsg;
	struct treq_sms_read_msg readMsg;
	struct treq_sms_save_msg saveMsg;
	struct treq_sms_delete_msg deleteMsg;
	struct treq_sms_get_msg_count getMsgCnt;
	struct treq_sms_get_sca getSca;
	struct treq_sms_set_sca setSca;
	struct treq_sms_get_cb_config getCbConfig;
	struct treq_sms_set_cb_config setCbConfig;
	struct treq_sms_set_mem_status memStatus;
	struct treq_sms_get_pref_brearer getPrefBrearer;
	struct treq_sms_set_pref_brearer setPrefBrearer;
	struct treq_sms_set_delivery_report deliveryReport;
	struct treq_sms_set_msg_status msgStatus;
	struct treq_sms_get_params getParams;
	struct treq_sms_set_params setParams;
	struct treq_sms_get_paramcnt getParamCnt;
	GSList *co_smslist = NULL;
	CoreObject *co_sms = NULL;
	UserRequest *ur = NULL;
	struct tcore_user_info ui = { 0, 0, 0, NULL };
	TReturn ret = TCORE_RETURN_SUCCESS;
	int request_id = 0;

	conn_name = g_array_index(in_param4, tapi_dbus_connection_name, 0);

	co_smslist = tcore_plugin_get_core_objects_bytype(plugin, CORE_OBJECT_TYPE_SMS);
	if (!co_smslist) {
		api_err = TAPI_API_NOT_SUPPORTED;
		dbg("[SMS_ERR] = 0x%x", api_err);
		goto OUT;
	}

	co_sms = (CoreObject *)co_smslist->data;
	g_slist_free(co_smslist);

	if (!co_sms) {
		api_err = TAPI_API_NOT_SUPPORTED;
		dbg("[SMS_ERR] = 0x%x", api_err);
		goto OUT;
	}

	ur = tcore_user_request_new(ctx->comm, tcore_plugin_get_description(plugin)->name);
	ui.appname = conn_name.name;
	tcore_user_request_set_user_info(ur, &ui);

	switch (tapi_service_function) {
		case TAPI_CS_NETTEXT_SEND:
			sendUmtsMsg.msgDataPackage = g_array_index(in_param1, struct telephony_sms_DataPackageInfo, 0);
			sendUmtsMsg.more = g_array_index(in_param2, TS_BOOL, 0);

			tcore_user_request_set_data(ur, sizeof(struct treq_sms_send_umts_msg), &sendUmtsMsg);
			tcore_user_request_set_command(ur, TREQ_SMS_SEND_UMTS_MSG);

			ret = tcore_communicator_dispatch_request(ctx->comm, ur);
			if (ret != TCORE_RETURN_SUCCESS) {
				api_err = TAPI_API_OPERATION_FAILED;
			}

			break;

		case TAPI_CS_NETTEXT_SEND_EX:
			memcpy(&cdmaMsg, &g_array_index(in_param1, struct telephony_sms_CdmaMsgInfo, 0), sizeof(struct telephony_sms_CdmaMsgInfo));
			cdmaMsg.more= g_array_index(in_param2,unsigned int, 0);

			tcore_user_request_set_data(ur, sizeof(struct treq_sms_send_cdma_msg), &cdmaMsg);
			tcore_user_request_set_command(ur, TREQ_SMS_SEND_CDMA_MSG);

			ret = tcore_communicator_dispatch_request(ctx->comm, ur);
			if (ret != TCORE_RETURN_SUCCESS) {
				api_err = TAPI_API_OPERATION_FAILED;
			}

			break;

		case TAPI_CS_NETTEXT_READ:
			readMsg.index = g_array_index(in_param1, int, 0);

			tcore_user_request_set_data(ur, sizeof(struct treq_sms_read_msg), &readMsg);
			tcore_user_request_set_command(ur, TREQ_SMS_READ_MSG);

			ret = tcore_communicator_dispatch_request(ctx->comm, ur);
			if (ret != TCORE_RETURN_SUCCESS) {
				api_err = TAPI_API_OPERATION_FAILED;
			}

			break;

		case TAPI_CS_NETTEXT_WRITE: {
				struct telephony_sms_Data tmp;
				tmp = g_array_index(in_param1, struct telephony_sms_Data, 0);
				saveMsg.simIndex = tmp.simIndex;
				saveMsg.msgStatus = tmp.msgStatus;
				memcpy(&saveMsg.msgDataPackage, &tmp.smsData, sizeof(struct telephony_sms_DataPackageInfo));

				tcore_user_request_set_data(ur, sizeof(struct treq_sms_save_msg), &saveMsg);
				tcore_user_request_set_command(ur, TREQ_SMS_SAVE_MSG);

				ret = tcore_communicator_dispatch_request(ctx->comm, ur);
				if (ret != TCORE_RETURN_SUCCESS) {
					api_err = TAPI_API_OPERATION_FAILED;
				}

			}
			break;

		case TAPI_CS_NETTEXT_DELETE:
			deleteMsg.index = g_array_index(in_param1, int, 0);

			tcore_user_request_set_data(ur, sizeof(struct treq_sms_delete_msg), &deleteMsg);
			tcore_user_request_set_command(ur, TREQ_SMS_DELETE_MSG);

			ret = tcore_communicator_dispatch_request(ctx->comm, ur);
			if (ret != TCORE_RETURN_SUCCESS) {
				api_err = TAPI_API_OPERATION_FAILED;
			}

			break;

		case TAPI_CS_NETTEXT_GETCOUNT:
			tcore_user_request_set_data(ur, sizeof(struct treq_sms_get_msg_count), &getMsgCnt);
			tcore_user_request_set_command(ur, TREQ_SMS_GET_COUNT);

			ret = tcore_communicator_dispatch_request(ctx->comm, ur);
			if (ret != TCORE_RETURN_SUCCESS) {
				api_err = TAPI_API_OPERATION_FAILED;
			}

			break;

		case TAPI_CS_NETTEXT_GETSCA:
			getSca.index = g_array_index(in_param1, int, 0);

			tcore_user_request_set_data(ur, sizeof(struct treq_sms_get_sca), &getSca);
			tcore_user_request_set_command(ur, TREQ_SMS_GET_SCA);

			ret = tcore_communicator_dispatch_request(ctx->comm, ur);
			if (ret != TCORE_RETURN_SUCCESS) {
				api_err = TAPI_API_OPERATION_FAILED;
			}

			break;

		case TAPI_CS_NETTEXT_SETSCA:
			setSca.scaInfo = g_array_index(in_param1, struct telephony_sms_AddressInfo, 0);
			setSca.index = g_array_index(in_param2, int, 0);

			if ((setSca.scaInfo.dialNumLen <= 0) || (setSca.scaInfo.dialNumLen > (TAPI_NETTEXT_MAX_SMS_SERVICE_CENTER_ADDR + 1)))
			{
				api_err = TAPI_API_INVALID_INPUT;
			}
			else if(setSca.index != 0)
			{
				dbg("Index except 0 is supported");
				api_err = TAPI_API_NOT_SUPPORTED;
			}
			else
			{
				tcore_user_request_set_data(ur, sizeof(struct treq_sms_set_sca), &setSca);
				tcore_user_request_set_command(ur, TREQ_SMS_SET_SCA);

				ret = tcore_communicator_dispatch_request(ctx->comm, ur);
				if (ret != TCORE_RETURN_SUCCESS) {
					api_err = TAPI_API_OPERATION_FAILED;
				}
			}

			break;

		case TAPI_CS_NETTEXT_GETCBCONFIG:
			tcore_user_request_set_data(ur, sizeof(struct treq_sms_get_cb_config), &getCbConfig);
			tcore_user_request_set_command(ur, TREQ_SMS_GET_CB_CONFIG);

			ret = tcore_communicator_dispatch_request(ctx->comm, ur);
			if (ret != TCORE_RETURN_SUCCESS) {
				api_err = TAPI_API_OPERATION_FAILED;
			}

			break;

		case TAPI_CS_NETTEXT_SETCBCONFIG:
			setCbConfig = g_array_index(in_param1, struct treq_sms_set_cb_config, 0);

			tcore_user_request_set_data(ur, sizeof(struct treq_sms_set_cb_config), &setCbConfig);
			tcore_user_request_set_command(ur, TREQ_SMS_SET_CB_CONFIG);

			ret = tcore_communicator_dispatch_request(ctx->comm, ur);
			if (ret != TCORE_RETURN_SUCCESS) {
				api_err = TAPI_API_OPERATION_FAILED;
			}

			break;

		case TAPI_CS_NETTEXT_SETMEMSTATUS:
			memStatus.memory_status = g_array_index(in_param1, int, 0);

			tcore_user_request_set_data(ur, sizeof(struct treq_sms_set_mem_status), &memStatus);
			tcore_user_request_set_command(ur, TREQ_SMS_SET_MEM_STATUS);

			ret = tcore_communicator_dispatch_request(ctx->comm, ur);
			if (ret != TCORE_RETURN_SUCCESS) {
				api_err = TAPI_API_OPERATION_FAILED;
			}

			break;

		case TAPI_CS_NETTEXT_GETPREFBEARER:
			tcore_user_request_set_data(ur, sizeof(struct treq_sms_get_pref_brearer), &getPrefBrearer);
			tcore_user_request_set_command(ur, TREQ_SMS_GET_PREF_BEARER);

			ret = tcore_communicator_dispatch_request(ctx->comm, ur);
			if (ret != TCORE_RETURN_SUCCESS) {
				api_err = TAPI_API_OPERATION_FAILED;
			}

			break;

		case TAPI_CS_NETTEXT_SETPREFBEARER:
			setPrefBrearer.svc = g_array_index(in_param1,int, 0);

			tcore_user_request_set_data(ur, sizeof(struct treq_sms_set_pref_brearer), &setPrefBrearer);
			tcore_user_request_set_command(ur, TREQ_SMS_SET_PREF_BEARER);

			ret = tcore_communicator_dispatch_request(ctx->comm, ur);
			if (ret != TCORE_RETURN_SUCCESS) {
				api_err = TAPI_API_OPERATION_FAILED;
			}

			break;

		case TAPI_CS_NETTEXT_SETDELIVERREPORT:
			deliveryReport.dataInfo = g_array_index(in_param1, struct telephony_sms_DataPackageInfo, 0);
			deliveryReport.rspType = g_array_index(in_param2, int, 0);

			tcore_user_request_set_data(ur, sizeof(struct treq_sms_set_delivery_report), &deliveryReport);
			tcore_user_request_set_command(ur, TREQ_SMS_SET_DELIVERY_REPORT);

			ret = tcore_communicator_dispatch_request(ctx->comm, ur);
			if (ret != TCORE_RETURN_SUCCESS) {
				api_err = TAPI_API_OPERATION_FAILED;
			}

			break;

		case TAPI_CS_NETTEXT_SETMSGSTATUS:
			msgStatus.index = g_array_index(in_param1,int, 0);
			msgStatus.msgStatus = g_array_index(in_param2, enum telephony_sms_MsgStatus, 0);

			tcore_user_request_set_data(ur, sizeof(struct treq_sms_set_msg_status), &msgStatus);
			tcore_user_request_set_command(ur, TREQ_SMS_SET_MSG_STATUS);

			ret = tcore_communicator_dispatch_request(ctx->comm, ur);
			if (ret != TCORE_RETURN_SUCCESS) {
				api_err = TAPI_API_OPERATION_FAILED;
			}

			break;

		case TAPI_CS_NETTEXT_GETPARAMS:
			getParams.index = g_array_index(in_param1,int, 0);

			tcore_user_request_set_data(ur, sizeof(struct treq_sms_get_params), &getParams);
			tcore_user_request_set_command(ur, TREQ_SMS_GET_PARAMS);

			ret = tcore_communicator_dispatch_request(ctx->comm, ur);
			if (ret != TCORE_RETURN_SUCCESS) {
				api_err = TAPI_API_OPERATION_FAILED;
			}

			break;

		case TAPI_CS_NETTEXT_SETPARAMS:
			setParams = g_array_index(in_param1, struct treq_sms_set_params, 0);

			tcore_user_request_set_data(ur, sizeof(struct treq_sms_set_params), &setParams);
			tcore_user_request_set_command(ur, TREQ_SMS_SET_PARAMS);

			ret = tcore_communicator_dispatch_request(ctx->comm, ur);
			if (ret != TCORE_RETURN_SUCCESS) {
				api_err = TAPI_API_OPERATION_FAILED;
			}

			break;

		case TAPI_CS_NETTEXT_GETPARAMCNT:
			tcore_user_request_set_data(ur, sizeof(struct treq_sms_get_paramcnt), &getParamCnt);
			tcore_user_request_set_command(ur, TREQ_SMS_GET_PARAMCNT);

			ret = tcore_communicator_dispatch_request(ctx->comm, ur);
			if (ret != TCORE_RETURN_SUCCESS) {
				api_err = TAPI_API_OPERATION_FAILED;
			}

			break;

		case TAPI_CS_NETTEXT_DEVICEREADY:
			dbg("[tcore_SMS] internal operation, please check TAPI_CS_NETTEXT_DEVICEREADY with MMFW !!!");
			api_err = TAPI_API_OPERATION_FAILED;
			break;

		case TAPI_CS_NETTEXT_DEVICESTATUS:
			{
				int device_status = 0x01;
				dbg("[tcore_SMS] internal operation, please check TAPI_CS_NETTEXT_DEVICESTATUS with MMFW !!!");
				g_array_append_vals(*out_param3, &device_status, sizeof(int));
			}
			break;

		default:
			api_err = TAPI_API_NOT_SUPPORTED;
			break;
	}

	dbg("[tcore_SMS] tapi_service_function[0x%x], ret = 0x%x", tapi_service_function, ret);

OUT:
	if (api_err != TAPI_API_SUCCESS) {
		tcore_user_request_free(ur);
	}
	g_array_append_vals(*out_param1, &api_err, sizeof(int));
	g_array_append_vals(*out_param2, &request_id, sizeof(int));	
}

TReturn dbus_response_sms(struct custom_data *ctx, UserRequest *ur, const char *appname,
		enum tcore_response_command command, unsigned int data_len, const void *data)
{
	int RequestId = 0;

	dbg("[tcore_SMS] command = 0x%x", command);

	switch (command) {
		case TRESP_SMS_SEND_UMTS_MSG: {
				const struct tresp_sms_send_umts_msg *resp = data;

				dbg("resp->result = 0x%x", resp->result);

				if(resp->result == SMS_SUCCESS) {
					ts_delivery_event(ctx->EvtDeliveryHandle, TAPI_EVENT_CLASS_NETTEXT, TAPI_EVENT_NETTEXT_SENTSTATUS_CNF,
							appname, RequestId, resp->result, sizeof(struct telephony_sms_DataPackageInfo), (void *) &resp->dataInfo);
				}
				else
				{
					ts_delivery_event(ctx->EvtDeliveryHandle, TAPI_EVENT_CLASS_NETTEXT, TAPI_EVENT_NETTEXT_SENTSTATUS_CNF,
							appname, RequestId, resp->result, sizeof(struct telephony_sms_DataPackageInfo), NULL);
				}
			}
			break;

		case TRESP_SMS_SEND_CDMA_MSG: {
				const struct tresp_sms_send_cdma_msg *resp = data;

				dbg("resp->result = 0x%x", resp->result);

				if(resp->result == SMS_SUCCESS) {
					ts_delivery_event(ctx->EvtDeliveryHandle, TAPI_EVENT_CLASS_NETTEXT, TAPI_EVENT_NETTEXT_SENTSTATUS_EX_CNF,
							appname, RequestId, resp->result, sizeof(TelSmsIs637CauseCode_t), (void *) &(resp->causeCode));
				}
				else
				{
					ts_delivery_event(ctx->EvtDeliveryHandle, TAPI_EVENT_CLASS_NETTEXT, TAPI_EVENT_NETTEXT_SENTSTATUS_EX_CNF,
							appname, RequestId, resp->result, sizeof(TelSmsIs637CauseCode_t), NULL);
				}
			}
			break;

		case TRESP_SMS_READ_MSG: {
				const struct tresp_sms_read_msg *resp = data;

				dbg("resp->result = 0x%x", resp->result);

				ts_delivery_event(ctx->EvtDeliveryHandle, TAPI_EVENT_CLASS_NETTEXT, TAPI_EVENT_NETTEXT_READ_SMS_CNF,
						appname, RequestId, resp->result, sizeof(struct telephony_sms_Data), (void *) &resp->dataInfo);
			}
			break;

		case TRESP_SMS_SAVE_MSG: {
				const struct tresp_sms_save_msg *resp = data;

				dbg("resp->result = 0x%x", resp->result);

				ts_delivery_event(ctx->EvtDeliveryHandle, TAPI_EVENT_CLASS_NETTEXT, TAPI_EVENT_NETTEXT_SAVE_STATUS_CNF,
						appname, RequestId, resp->result, sizeof(int), (void *) &resp->index);
			}
			break;

		case TRESP_SMS_DELETE_MSG: {
				const struct tresp_sms_delete_msg *resp = data;

				dbg("resp->result = 0x%x", resp->result);

				ts_delivery_event(ctx->EvtDeliveryHandle, TAPI_EVENT_CLASS_NETTEXT, TAPI_EVENT_NETTEXT_DELETE_STATUS_CNF,
						appname, RequestId, resp->result, sizeof(int), (void *) &resp->index);
			}
			break;

		case TRESP_SMS_GET_STORED_MSG_COUNT: {
				const struct tresp_sms_get_storedMsgCnt *resp = data;
				TelSmsStoredMsgCountInfo_t	storedMsgCnt;

				dbg("resp->result = 0x%x", resp->result);

				if(resp->result == SMS_SUCCESS) {
					memcpy(&storedMsgCnt, &(resp->storedMsgCnt), sizeof(TelSmsStoredMsgCountInfo_t));

					ts_delivery_event(ctx->EvtDeliveryHandle, TAPI_EVENT_CLASS_NETTEXT, TAPI_EVENT_NETTEXT_GET_COUNT_CNF,
						appname, RequestId, resp->result, sizeof(TelSmsStoredMsgCountInfo_t), (void *) &storedMsgCnt);
				}
				else
				{
					ts_delivery_event(ctx->EvtDeliveryHandle, TAPI_EVENT_CLASS_NETTEXT, TAPI_EVENT_NETTEXT_GET_COUNT_CNF,
						appname, RequestId, resp->result, sizeof(TelSmsStoredMsgCountInfo_t), NULL);
				}
			}
			break;

		case TRESP_SMS_GET_SCA: {
				const struct tresp_sms_get_sca *resp = data;
				TelSmsAddressInfo_t scaInfo;

				dbg("resp->result = 0x%x", resp->result);

				if(resp->result == SMS_SUCCESS) {
					memcpy(&scaInfo, &(resp->scaAddress), sizeof(TelSmsAddressInfo_t));
					scaInfo.DialNumLen = resp->scaAddress.dialNumLen;
					scaInfo.Ton = resp->scaAddress.typeOfNum;
					scaInfo.Npi= resp->scaAddress.numPlanId;
					memcpy(&scaInfo.szDiallingNum[0], &resp->scaAddress.diallingNum[0], TAPI_SIM_SMSP_ADDRESS_LEN + 1);

					ts_delivery_event(ctx->EvtDeliveryHandle, TAPI_EVENT_CLASS_NETTEXT, TAPI_EVENT_NETTEXT_GET_SCA_CNF,
						appname, RequestId, resp->result, sizeof(TelSmsAddressInfo_t), (void *) &scaInfo);
				}
				else
				{
					ts_delivery_event(ctx->EvtDeliveryHandle, TAPI_EVENT_CLASS_NETTEXT, TAPI_EVENT_NETTEXT_GET_SCA_CNF,
						appname, RequestId, resp->result, sizeof(TelSmsAddressInfo_t), NULL);
				}
			}
			break;

		case TRESP_SMS_SET_SCA: {
				const struct tresp_sms_set_sca *resp = data;
				TelSmsSetResponse RequestType = TAPI_NETTEXT_SETSCADDR_RSP;

				dbg("resp->result = 0x%x", resp->result);

				ts_delivery_event(ctx->EvtDeliveryHandle, TAPI_EVENT_CLASS_NETTEXT, TAPI_EVENT_NETTEXT_SET_REQUEST_CNF,
						appname, RequestId, resp->result, sizeof(TelSmsSetResponse), (void *) &RequestType);
			}
			break;

		case TRESP_SMS_GET_CB_CONFIG: {
				const struct tresp_sms_get_cb_config *resp = data;
				TelSmsCbConfig_t cbConfig;

				dbg("resp->result = 0x%x", resp->result);

				memcpy(&cbConfig, &resp->cbConfig, sizeof(TelSmsCbConfig_t));

				if(resp->result == SMS_SUCCESS) {
					ts_delivery_event(ctx->EvtDeliveryHandle, TAPI_EVENT_CLASS_NETTEXT, TAPI_EVENT_NETTEXT_GET_CB_CONFIG_CNF,
						appname, RequestId, resp->result, sizeof(TelSmsCbConfig_t), (void *) &cbConfig);
				}
				else
				{
					ts_delivery_event(ctx->EvtDeliveryHandle, TAPI_EVENT_CLASS_NETTEXT, TAPI_EVENT_NETTEXT_GET_CB_CONFIG_CNF,
						appname, RequestId, resp->result, sizeof(TelSmsCbConfig_t), NULL);
				}
			}
			break;

		case TRESP_SMS_SET_CB_CONFIG: {
				const struct tresp_sms_set_cb_config *resp = data;
				TelSmsSetResponse RequestType = TAPI_NETTEXT_CBSETCONFIG_RSP;

				dbg("resp->result = 0x%x", resp->result);

				ts_delivery_event(ctx->EvtDeliveryHandle, TAPI_EVENT_CLASS_NETTEXT, TAPI_EVENT_NETTEXT_SET_REQUEST_CNF,
						appname, RequestId, resp->result, sizeof(TelSmsSetResponse), (void *) &RequestType);
			}
			break;

		case TRESP_SMS_SET_MEM_STATUS: {
				const struct tresp_sms_set_mem_status *resp = data;
				TelSmsSetResponse RequestType = TAPI_NETTEXT_SETMEMORYSTATUS_RSP;

				dbg("resp->result = 0x%x", resp->result);

				ts_delivery_event(ctx->EvtDeliveryHandle, TAPI_EVENT_CLASS_NETTEXT, TAPI_EVENT_NETTEXT_SET_REQUEST_CNF,
						appname, RequestId, resp->result, sizeof(TelSmsCause_t), (void *) &RequestType);
			}
			break;
#if 0
		case TRESP_SMS_GET_PREF_BEARER:
			ts_delivery_event(ctx->EvtDeliveryHandle, TAPI_EVENT_CLASS_NETTEXT, TAPI_EVENT_NETTEXT_GET_SMSBEARER_CNF,
					appname, RequestId, status, sizeof(TelSmsBearerType_t), (void *) bearer);
			break;

		case TRESP_SMS_SET_PREF_BEARER:
			ts_delivery_event(ctx->EvtDeliveryHandle, TAPI_EVENT_CLASS_NETTEXT, TAPI_EVENT_NETTEXT_SET_REQUEST_CNF,
					appname, RequestId, Status, sizeof(TelSmsCause_t), (void *) &RequestType);
			break;
#endif
		case TRESP_SMS_SET_DELIVERY_REPORT: {
				const struct tresp_sms_set_delivery_report *resp = data;

				dbg("resp->result = 0x%x", resp->result);

				ts_delivery_event(ctx->EvtDeliveryHandle, TAPI_EVENT_CLASS_NETTEXT, TAPI_EVENT_NETTEXT_DELIVERY_REPORT_CNF,
						appname, RequestId, resp->result, 0, NULL);
			}
			break;

		case TRESP_SMS_SET_MSG_STATUS: {
				const struct tresp_sms_set_mem_status *resp = data;
				TelSmsSetResponse RequestType = TAPI_NETTEXT_SETMESSAGESTATUS_RSP;

				dbg("resp->result = 0x%x", resp->result);

				ts_delivery_event(ctx->EvtDeliveryHandle, TAPI_EVENT_CLASS_NETTEXT, TAPI_EVENT_NETTEXT_SET_REQUEST_CNF,
						appname, RequestId, resp->result, sizeof(TelSmsCause_t), (void *) &RequestType);
			}
			break;

		case TRESP_SMS_GET_PARAMS: {
				const struct tresp_sms_get_params *resp = data;
				TelSmsParams_t paramInfo;

				dbg("resp->result = 0x%x", resp->result);

				memcpy(&paramInfo, &resp->paramsInfo, sizeof(TelSmsParams_t));

				if(resp->result == SMS_SUCCESS) {
					ts_delivery_event(ctx->EvtDeliveryHandle, TAPI_EVENT_CLASS_NETTEXT, TAPI_EVENT_NETTEXT_GET_PARAM_CNF,
						appname, RequestId, resp->result, sizeof(TelSmsParams_t), (void *) &paramInfo);
				}
				else
				{
					ts_delivery_event(ctx->EvtDeliveryHandle, TAPI_EVENT_CLASS_NETTEXT, TAPI_EVENT_NETTEXT_GET_PARAM_CNF,
						appname, RequestId, resp->result, sizeof(TelSmsParams_t), NULL);
				}
			}
			break;

		case TRESP_SMS_SET_PARAMS:{
				const struct tresp_sms_set_params *resp = data;
				TelSmsSetResponse RequestType = TAPI_NETTEXT_SETPARAMETERS_RSP;

				dbg("resp->result = 0x%x", resp->result);

				ts_delivery_event(ctx->EvtDeliveryHandle, TAPI_EVENT_CLASS_NETTEXT, TAPI_EVENT_NETTEXT_SET_REQUEST_CNF,
						appname, RequestId, resp->result, sizeof(TelSmsCause_t), (void *) &RequestType);
			}
			break;

		case TRESP_SMS_GET_PARAMCNT: {
				const struct tresp_sms_get_paramcnt *resp = data;

				dbg("resp->result = 0x%x", resp->result);

				if(resp->result == SMS_SUCCESS) {
					ts_delivery_event(ctx->EvtDeliveryHandle, TAPI_EVENT_CLASS_NETTEXT, TAPI_EVENT_NETTEXT_PARAM_COUNT_IND,
						appname, RequestId, resp->result, sizeof(int), (void *) &resp->recordCount);
				}
				else
				{
					ts_delivery_event(ctx->EvtDeliveryHandle, TAPI_EVENT_CLASS_NETTEXT, TAPI_EVENT_NETTEXT_PARAM_COUNT_IND,
						appname, RequestId, resp->result, sizeof(int), NULL);
				}
			}
			break;

		default:
			break;
	}

	return TRUE;
}

TReturn dbus_notification_sms(struct custom_data *ctx, CoreObject *source, enum tcore_notification_command command,
		unsigned int data_len, const void *data)
{
	dbg("command = 0x%x", command);

	switch (command) {
		case TNOTI_SMS_INCOM_MSG: {
                        const struct tnoti_sms_umts_msg *noti = data;

                        ts_delivery_event(ctx->EvtDeliveryHandle, TAPI_EVENT_CLASS_NETTEXT, TAPI_EVENT_NETTEXT_INCOM_IND, NULL, TAPI_REQUEST_NOTI_ID,
                                TAPI_NETTEXT_SUCCESS, sizeof(struct telephony_sms_DataPackageInfo), (void *)&(noti->msgInfo));
			}
			break;
		case TNOTI_SMS_CB_INCOM_MSG: {
			const struct tnoti_sms_cellBroadcast_msg *noti = data;

			ts_delivery_event(ctx->EvtDeliveryHandle, TAPI_EVENT_CLASS_NETTEXT, TAPI_EVENT_NETTEXT_CB_INCOM_IND, NULL,
					TAPI_REQUEST_NOTI_ID, TAPI_NETTEXT_SUCCESS, sizeof(struct telephony_sms_CbMsg), (void *)&(noti->cbMsg));
			}
			break;
		case TNOTI_SMS_INCOM_EX_MSG: {
			 const struct tnoti_sms_cdma_msg *noti = data;

			ts_delivery_event(ctx->EvtDeliveryHandle, TAPI_EVENT_CLASS_NETTEXT, TAPI_EVENT_NETTEXT_INCOM_EX_IND,
					NULL, TAPI_REQUEST_NOTI_ID, TAPI_NETTEXT_SUCCESS, sizeof(struct telephony_sms_CdmaMsgInfo), (void *)&(noti->cdmaMsg));
			}
			break;
		case TNOTI_SMS_CB_INCOM_EX_MSG: {
			 const struct tnoti_sms_cdma_msg *noti = data;

			ts_delivery_event(ctx->EvtDeliveryHandle, TAPI_EVENT_CLASS_NETTEXT, TAPI_EVENT_NETTEXT_CB_INCOM_EX_IND,
					NULL, TAPI_REQUEST_NOTI_ID, TAPI_NETTEXT_SUCCESS, sizeof(struct telephony_sms_CdmaMsgInfo), (void *)&(noti->cdmaMsg));
			}
			break;
		case TNOTI_SMS_MEMORY_STATUS: {
			const struct tnoti_sms_memory_status *noti = data;

			ts_delivery_event(ctx->EvtDeliveryHandle, TAPI_EVENT_CLASS_NETTEXT, TAPI_EVENT_NETTEXT_MEMORY_STATUS_IND,
					NULL, TAPI_REQUEST_NOTI_ID, TAPI_NETTEXT_SUCCESS, sizeof(int), (void *)&(noti->status));
			}
			break;
		case TNOTI_SMS_DEVICE_READY: {
			const struct tnoti_sms_device_ready_status *noti = data;

			ts_delivery_event(ctx->EvtDeliveryHandle, TAPI_EVENT_CLASS_NETTEXT, TAPI_EVENT_NETTEXT_DEVICE_READY_IND,
					NULL, TAPI_REQUEST_NOTI_ID, TAPI_NETTEXT_SUCCESS, sizeof(int), (void *) &(noti->status));
			}
			break;
		default:
			break;
	}

	return TRUE;
}
