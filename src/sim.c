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
#include <co_sim.h>
#include <communicator.h>
#include <co_phonebook.h>

#include <TapiCommon.h>
#include <TelSim.h>

#include "tel_cs_conn.h"
#include "common.h"
#include "ts_utility.h"
#include "ts_common.h"
#include "ts_svr_req.h"
#include "ts_noti.h"
#include "modules.h"


static gboolean dbus_sim_data_request(struct custom_data *ctx, CoreObject *co, enum tel_sim_status sim_status )
{
	UserRequest *ur = NULL;
	TcorePlugin *plugin = NULL;

	switch(sim_status){
		case SIM_STATUS_INITIALIZING :
		case SIM_STATUS_PIN_REQUIRED :
		case SIM_STATUS_PUK_REQUIRED :
		case SIM_STATUS_CARD_BLOCKED :
		case SIM_STATUS_NCK_REQUIRED :
		case SIM_STATUS_NSCK_REQUIRED :
		case SIM_STATUS_SPCK_REQUIRED :
		case SIM_STATUS_CCK_REQUIRED :
		case SIM_STATUS_LOCK_REQUIRED :
			if(ctx->b_recv_first_status == FALSE){
				dbg("received sim status at first time");
				plugin = tcore_server_find_plugin(ctx->server, TCORE_PLUGIN_DEFAULT);
				dbg("req - TREQ_SIM_GET_LANGUAGE ");
				ur = tcore_user_request_new(ctx->comm, tcore_plugin_get_description(plugin)->name);
				tcore_user_request_set_command(ur, TREQ_SIM_GET_LANGUAGE);
				tcore_communicator_dispatch_request(ctx->comm, ur);

				dbg("req - TREQ_SIM_GET_ICCID ");
				ur = tcore_user_request_new(ctx->comm, tcore_plugin_get_description(plugin)->name);
				tcore_user_request_set_command(ur, TREQ_SIM_GET_ICCID);
				tcore_communicator_dispatch_request(ctx->comm, ur);

				dbg("req - TREQ_SIM_GET_MSISDN ");
				ur = tcore_user_request_new(ctx->comm, tcore_plugin_get_description(plugin)->name);
				tcore_user_request_set_command(ur, TREQ_SIM_GET_MSISDN);
				tcore_communicator_dispatch_request(ctx->comm, ur);

				dbg("req - TREQ_SIM_GET_ECC ");
				ur = tcore_user_request_new(ctx->comm, tcore_plugin_get_description(plugin)->name);
				tcore_user_request_set_command(ur, TREQ_SIM_GET_ECC);
				tcore_communicator_dispatch_request(ctx->comm, ur);
				ctx->b_recv_first_status = TRUE;
			}
			break;

		default :
			break;
	}
	return TRUE;
}

static gboolean dbus_sim_security_request(struct custom_data *ctx, CoreObject *co)
{
	UserRequest *ur = NULL;
	TcorePlugin *plugin = NULL;
	struct treq_sim_get_facility_status req_facility;

	plugin = tcore_server_find_plugin(ctx->server, TCORE_PLUGIN_DEFAULT);

	dbg("req - SIM_FACILITY_PS ");
	ur = tcore_user_request_new(ctx->comm, tcore_plugin_get_description(plugin)->name);
	req_facility.type = SIM_FACILITY_PS;
	tcore_user_request_set_data(ur, sizeof(struct treq_sim_get_facility_status), &req_facility);
	tcore_user_request_set_command(ur, TREQ_SIM_GET_FACILITY_STATUS);
	tcore_communicator_dispatch_request(ctx->comm, ur);

	dbg("req - SIM_FACILITY_SC ");
	ur = tcore_user_request_new(ctx->comm, tcore_plugin_get_description(plugin)->name);
	req_facility.type = SIM_FACILITY_SC;
	tcore_user_request_set_data(ur, sizeof(struct treq_sim_get_facility_status), &req_facility);
	tcore_user_request_set_command(ur, TREQ_SIM_GET_FACILITY_STATUS);
	tcore_communicator_dispatch_request(ctx->comm, ur);

	dbg("req - SIM_FACILITY_FD ");
	ur = tcore_user_request_new(ctx->comm, tcore_plugin_get_description(plugin)->name);
	req_facility.type = SIM_FACILITY_FD;
	tcore_user_request_set_data(ur, sizeof(struct treq_sim_get_facility_status), &req_facility);
	tcore_user_request_set_command(ur, TREQ_SIM_GET_FACILITY_STATUS);
	tcore_communicator_dispatch_request(ctx->comm, ur);

	return TRUE;
}

static gboolean dbus_sim_security_update(struct custom_data *ctx, enum tel_sim_facility_type type, enum tel_sim_pin_operation_result result, gboolean b_enable)
{
	dbg("current ctx addr[0x%x], facility[%d], op result[%d], b_enable[%d]", ctx, type, result, b_enable);
	switch(type) {
		case SIM_FACILITY_PS :
			if(result == SIM_PUK_REQUIRED)
				ctx->sim_lock = TAPI_SIM_PIN_STATUS_BLOCKED;
			else if(result == SIM_CARD_ERROR)
				ctx->sim_lock = TAPI_SIM_PIN_STATUS_PUK_BLOCKED;
			else if(result == SIM_PIN_OPERATION_SUCCESS)
				ctx->sim_lock = b_enable;
			break;
		case SIM_FACILITY_SC :
			if(result == SIM_PUK_REQUIRED)
				ctx->pin_lock = TAPI_SIM_PIN_STATUS_BLOCKED;
			else if(result == SIM_CARD_ERROR)
				ctx->pin_lock = TAPI_SIM_PIN_STATUS_PUK_BLOCKED;
			else if(result == SIM_PIN_OPERATION_SUCCESS)
				ctx->pin_lock = b_enable;
			break;
		case SIM_FACILITY_FD :
			if( result == SIM_PIN_OPERATION_SUCCESS )
				ctx->fdn_lock = b_enable;
			break;
		default :
			break;
	}
	return TRUE;
}

void dbus_request_sim(struct custom_data *ctx, TcorePlugin *plugin, int tapi_service_function, GArray* in_param1,
		GArray* in_param2, GArray* in_param3, GArray* in_param4, GArray** out_param1, GArray** out_param2,
		GArray** out_param3, GArray** out_param4, GError** error)
{
	int api_err = TAPI_API_SUCCESS;
	int request_id = 0;
	tapi_dbus_connection_name conn_name;
	/*
	 * Legacy : SIM data, security part
	 */
	TelSimCardStatus_t tmp_cardstatus = 0xff;
	gboolean b_changed = FALSE;
	TelSimImsiInfo_t imsi = { 0, };
	TelSimIccIdInfo_t iccid_data = { 0, };
	TelSimEccData_t ecc_data;
	int fdn_status = FALSE;
	TelSimLanguageInfo_t lang_data = { 0, };
	TelSimCardType_t card_type = TAPI_SIM_CARD_TYPE_UNKNOWN;
	enum tel_sim_type type = SIM_TYPE_UNKNOWN;
	TelSimSecPw_t pin_data = { 0, };
	TelSimSecPw_t puk_data = { 0, };
	TelSimSecPw_t old_pin = { 0, };
	TelSimSecPw_t new_pin = { 0, };
	TelSimPinStatus_t status = 0;
	TelSimPersPw_t pers_data = { 0, };
	unsigned char* pin2 = NULL;
	unsigned int pin2_len = 0;
	TelSimApdu_t apdu_data = { 0, };
	int ecc_count = 0;
	TelSimAuthenticationData_t AuthenticationData = {0,};
	TelSimCphsLocalInfo_t cphs_local = {0,};

	/*
	 * NEW : SIM data, security part
	 */
	struct tel_sim_imsi *n_imsi;
	struct treq_sim_verify_pins verify_pins = { 0, };
	struct treq_sim_verify_puks verify_puks = { 0, };
	struct treq_sim_change_pins change_pins = { 0, };
	struct treq_sim_get_facility_status facility = { 0, };
	struct treq_sim_disable_facility dis_facility = { 0, };
	struct treq_sim_enable_facility en_facility = { 0, };
	struct treq_sim_transmit_apdu send_apdu = { 0, };
	struct treq_sim_set_language set_language = { 0, };
	struct treq_sim_req_authentication req_auth = { 0, };
	/*
	 * Legacy : SIM Phonebook part
	 */
	TelSimPbFileType_t lsp_pb_type = TAPI_SIM_PB_ADN; //default value
	unsigned short lsp_index = 0;
	int lsp_first_index = 0;
	int lsp_init_completed = 0;
	TelSimPbList_t lsp_supported_pb_list = { 0, };
	/*
	 * New : SIM Phonebook part
	 */
	struct tel_phonebook_support_list *pb_list = NULL;
	struct treq_phonebook_get_count pb_count;
	struct treq_phonebook_get_info pb_info;
	struct treq_phonebook_get_usim_info pb_usim;
	struct treq_phonebook_read_record pb_read;
	struct treq_phonebook_update_record pb_update;
	struct treq_phonebook_delete_record pb_delete;

	/*
	 * Legacy : SIM SAP part
	 */
	TelSimSapConnect_t lsa_conn_req = { 0, };
	TelSimSapApduData_t lsa_apdu_data = { 0, };
	TelSimSapProtocol_t lsa_protocol = TAPI_SIM_SAP_PROTOCOL_T0;
	TelSimSapMsgId_t lsa_msg_id = 0x00;
	/*
	 * New : SIM SAP part
	 */
	struct treq_sap_req_connect req_conn;
	struct treq_sap_req_status req_status;
	struct treq_sap_req_atr req_atr;
	struct treq_sap_transfer_apdu t_apdu;
	struct treq_sap_set_protocol set_protocol;
	struct treq_sap_set_power set_power;

	TReturn ret;
	GSList *co_list = NULL;
	CoreObject *co_sim = NULL;
	CoreObject *co_pb = NULL;
	UserRequest *ur = NULL;
	struct tcore_user_info ui = { 0, 0, 0, NULL };

	conn_name = g_array_index(in_param4, tapi_dbus_connection_name, 0);

	co_list = tcore_plugin_get_core_objects_bytype(plugin, CORE_OBJECT_TYPE_SIM);
	if (!co_list) {
		api_err = TAPI_API_NOT_SUPPORTED;
		goto OUT;
	}
	co_sim = (CoreObject *)co_list->data;
	g_slist_free(co_list);

	if (!co_sim) {
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
		case TAPI_CS_SIM_GETIMSIINFO:
			dbg("TAPI_CS_SIM_GETIMSIINFO");
			n_imsi = tcore_sim_get_imsi(co_sim);
			dbg("n_imsi->plmn[%s]", n_imsi->plmn);
			if (n_imsi != NULL) {
				imsi.bValid = TRUE;
				memcpy(&imsi.szMcc, n_imsi->plmn, 3);
				imsi.szMcc[3] = '\0';
				memcpy(&imsi.szMnc, n_imsi->plmn + 3, 2);
				imsi.szMnc[2] = '\0';
				memcpy(&imsi.szMsin, n_imsi->msin, 10);
				imsi.szMsin[10] = '\0';
				dbg("imsi.valid=%d, mcc=%s, mnc=%s, msin=%s",
						imsi.bValid, imsi.szMcc, imsi.szMnc, imsi.szMsin);
			}
			g_array_append_vals(*out_param2, &imsi, sizeof(TelSimImsiInfo_t));
			break;

		case TAPI_CS_SIM_GETFDNSTATUS:
			dbg("TAPI_CS_SIM_GETFDNSTATUS");
			fdn_status = ctx->fdn_lock;
			g_array_append_vals(*out_param2, &fdn_status, sizeof(int));
			break;

		case TAPI_CS_SIM_GETECCINFO:
			dbg("TAPI_CS_SIM_GETECCINFO");
			ecc_count = ctx->ecc_count;
			memcpy(&ecc_data, &ctx->ecc, sizeof(TelSimEccData_t));
			g_array_append_vals(*out_param2, &ecc_data, sizeof(TelSimEccData_t));
			g_array_append_vals(*out_param3, &ecc_count, sizeof(int));
			break;

		case TAPI_CS_SIM_GETICCIDINFO:
			dbg("TAPI_CS_SIM_GETICCIDINFO");
			memcpy(&iccid_data, &ctx->iccid, sizeof(TelSimIccIdInfo_t));
			g_array_append_vals(*out_param2, &iccid_data, sizeof(TelSimIccIdInfo_t));
			break;

		case TAPI_CS_SIM_SETLANGUAGE: //NOT USED FROM APP NOW
			g_array_append_vals(*out_param2, &request_id, sizeof(int));
			tcore_user_request_set_data(ur, sizeof(struct treq_sim_set_language), &set_language);
			tcore_user_request_set_command(ur, TREQ_SIM_SET_LANGUAGE);
			ret = tcore_communicator_dispatch_request(ctx->comm, ur);
			if (ret != TCORE_RETURN_SUCCESS) {
				api_err = TAPI_API_OPERATION_FAILED;
			}
			dbg("ret = 0x%x", ret);
			break;

		case TAPI_CS_SIM_RSIMACCESS: //NOT USED FROM APP NOW
			dbg("TAPI_CS_SIM_RSIMACCESS");
			api_err = TAPI_API_NOT_SUPPORTED;
			break;
		case TAPI_CS_SIM_GETCFINFO: //NOT USED FROM APP NOW
			dbg("TAPI_CS_SIM_GETCFINFO");
			api_err = TAPI_API_NOT_SUPPORTED;
			break;
		case TAPI_CS_SIM_GETMWINFO: //NOT USED FROM APP NOW
			dbg("TAPI_CS_SIM_GETMWINFO");
			api_err = TAPI_API_NOT_SUPPORTED;
			break;
		case TAPI_CS_SIM_GETCPHSINFO:
			dbg("TAPI_CS_SIM_GETCPHSINFO");
			g_array_append_vals(*out_param2, &cphs_local, sizeof(TelSimCphsLocalInfo_t));
			break;
		case TAPI_CS_SIM_GETMBINFO:
			dbg("TAPI_CS_SIM_GETMBINFO");
			api_err = TAPI_API_NOT_SUPPORTED;
			break;
		case TAPI_CS_SIM_GETLANGUAGEINFO:
			dbg("TAPI_CS_SIM_GETLANGUAGEINFO");
			memcpy(&lang_data, &ctx->language, sizeof(TelSimLanguageInfo_t));
			g_array_append_vals (*out_param2, &lang_data,sizeof(TelSimLanguageInfo_t));
			break;
		case TAPI_CS_SIM_GETCARDTYPE: /* 0x1000225 */
			dbg("TAPI_CS_SIM_GETCARDTYPE");
			type = tcore_sim_get_type(co_sim);
			switch (type) {
				case SIM_TYPE_UNKNOWN:
					card_type = TAPI_SIM_CARD_TYPE_UNKNOWN;
					break;
				case SIM_TYPE_GSM:
					card_type = TAPI_SIM_CARD_TYPE_GSM;
					break;
				case SIM_TYPE_USIM:
					card_type = TAPI_SIM_CARD_TYPE_USIM;
					break;
				case SIM_TYPE_RUIM:
					card_type = TAPI_SIM_CARD_TYPE_UNKNOWN;
					break;
				case SIM_TYPE_ISIM:
					card_type = TAPI_SIM_CARD_TYPE_UNKNOWN;
					break;
				default:
					break;
			}
			dbg("card_type = %d", card_type);
			g_array_append_vals(*out_param2, &card_type, sizeof(TelSimCardType_t));
			break;

		case TAPI_CS_SIM_GETSIMINITINFO:
			dbg("TAPI_CS_SIM_GETSIMINITINFO");
			tmp_cardstatus = tcore_sim_get_status(co_sim);
			b_changed = tcore_sim_get_identification(co_sim);
			dbg("sim init info - cardstatus[%d],changed[%d]", tmp_cardstatus, b_changed);
			g_array_append_vals(*out_param2, &tmp_cardstatus, sizeof(TelSimCardStatus_t));
			g_array_append_vals(*out_param3, &b_changed, sizeof(int));
			break;

		case TAPI_CS_SIM_AUTHENTICATION:
			dbg("TAPI_CS_SIM_AUTHENTICATION");
			g_array_append_vals(*out_param2, &request_id, sizeof(int));
			AuthenticationData = g_array_index(in_param1, TelSimAuthenticationData_t, 0);
			req_auth.auth_type = AuthenticationData.auth_type;
			req_auth.autn_length = AuthenticationData.autn_length;
			if(req_auth.autn_length)
				memcpy(req_auth.autn_data, AuthenticationData.autn_data, req_auth.autn_length);
			req_auth.rand_length = AuthenticationData.rand_length;
			if(req_auth.autn_length)
				memcpy(req_auth.rand_data, AuthenticationData.rand_data, req_auth.rand_length);
			tcore_user_request_set_data(ur, sizeof(struct treq_sim_req_authentication), &req_auth);
			tcore_user_request_set_command(ur, TREQ_SIM_REQ_AUTHENTICATION);
			ret = tcore_communicator_dispatch_request(ctx->comm, ur);
			if (ret != TCORE_RETURN_SUCCESS) {
				api_err = TAPI_API_OPERATION_FAILED;
			}
			dbg("ret = 0x%x", ret);
			break;

		case TAPI_CS_SIM_VERIFYSEC:
			dbg("TAPI_CS_SIM_VERIFYSEC");
			g_array_append_vals(*out_param2, &request_id, sizeof(int));
			pin_data = g_array_index(in_param1, TelSimSecPw_t, 0);
			pin_data.pw =(unsigned char*)&g_array_index(in_param2, unsigned char, 0);
			verify_pins.pin_type = pin_data.type;
			verify_pins.pin_length = pin_data.pw_len;
			memcpy(verify_pins.pin, pin_data.pw, verify_pins.pin_length);
			tcore_user_request_set_data(ur, sizeof(struct treq_sim_verify_pins), &verify_pins);
			tcore_user_request_set_command(ur, TREQ_SIM_VERIFY_PINS);
			ret = tcore_communicator_dispatch_request(ctx->comm, ur);
			if (ret != TCORE_RETURN_SUCCESS) {
				api_err = TAPI_API_OPERATION_FAILED;
			}
			dbg("ret = 0x%x", ret);
			break;

		case TAPI_CS_SIM_VERIFYPUK:
			dbg("TAPI_CS_SIM_VERIFYPUK");
			g_array_append_vals(*out_param2, &request_id, sizeof(int));
			puk_data = g_array_index(in_param1, TelSimSecPw_t, 0);
			pin_data = g_array_index(in_param1, TelSimSecPw_t, 1);
			puk_data.pw = (unsigned char*)&g_array_index(in_param2, unsigned char, 0);
			pin_data.pw = (unsigned char*)&g_array_index(in_param3, unsigned char, 0);
			verify_puks.puk_type = puk_data.type;
			verify_puks.puk_length = puk_data.pw_len;
			memcpy(verify_puks.puk, puk_data.pw, verify_puks.puk_length);
			verify_puks.pin_length = pin_data.pw_len;
			memcpy(verify_puks.pin, pin_data.pw, verify_puks.pin_length);
			tcore_user_request_set_data(ur, sizeof(struct treq_sim_verify_puks), &verify_puks);
			tcore_user_request_set_command(ur, TREQ_SIM_VERIFY_PUKS);
			ret = tcore_communicator_dispatch_request(ctx->comm, ur);
			if (ret != TCORE_RETURN_SUCCESS) {
				api_err = TAPI_API_OPERATION_FAILED;
			}
			dbg("ret = 0x%x", ret);
			break;

		case TAPI_CS_SIM_CHANGEPIN:
			dbg("TAPI_CS_SIM_CHANGEPIN");
			g_array_append_vals(*out_param2, &request_id, sizeof(int));
			old_pin = g_array_index(in_param1, TelSimSecPw_t, 0);
			new_pin = g_array_index(in_param1, TelSimSecPw_t, 1);
			old_pin.pw = (unsigned char*)&g_array_index(in_param2, unsigned char, 0);
			new_pin.pw = (unsigned char*)&g_array_index(in_param3, unsigned char, 0);
			change_pins.type = old_pin.type;
			change_pins.old_pin_length = old_pin.pw_len;
			memcpy(change_pins.old_pin, old_pin.pw, change_pins.old_pin_length);
			change_pins.new_pin_length = new_pin.pw_len;
			memcpy(change_pins.new_pin, new_pin.pw, change_pins.new_pin_length);
			tcore_user_request_set_data(ur, sizeof(struct treq_sim_change_pins), &change_pins);
			tcore_user_request_set_command(ur, TREQ_SIM_CHANGE_PINS);
			ret = tcore_communicator_dispatch_request(ctx->comm, ur);
			if (ret != TCORE_RETURN_SUCCESS) {
				api_err = TAPI_API_OPERATION_FAILED;
			}
			dbg("ret = 0x%x", ret);
			break;

		case TAPI_CS_SIM_DISABLESEC:
			dbg("TAPI_CS_SIM_DISABLESEC");
			g_array_append_vals(*out_param2, &request_id, sizeof(int));
			pin_data= g_array_index(in_param1, TelSimSecPw_t, 0);
			pin_data.pw = (unsigned char*)&g_array_index(in_param2, unsigned char, 0);
			if(pin_data.type == TAPI_SIM_PTYPE_PIN1) {
				dis_facility.type = SIM_FACILITY_SC;
			} else if (pin_data.type == TAPI_SIM_PTYPE_SIM) {
				dis_facility.type = SIM_FACILITY_PS;
			} else {
				api_err = TAPI_API_INVALID_INPUT;
				g_array_append_vals(*out_param2, &request_id, sizeof(int));
				goto OUT;
			}
			dis_facility.password_length = pin_data.pw_len;
			memcpy(dis_facility.password, pin_data.pw, dis_facility.password_length);
			tcore_user_request_set_data(ur, sizeof(struct treq_sim_disable_facility), &dis_facility);
			tcore_user_request_set_command(ur, TREQ_SIM_DISABLE_FACILITY);
			ret = tcore_communicator_dispatch_request(ctx->comm, ur);
			if (ret != TCORE_RETURN_SUCCESS) {
				api_err = TAPI_API_OPERATION_FAILED;
			}
			dbg("ret = 0x%x", ret);
			break;

		case TAPI_CS_SIM_ENABLESEC:
			dbg("TAPI_CS_SIM_ENABLESEC");
			g_array_append_vals(*out_param2, &request_id, sizeof(int));
			pin_data= g_array_index(in_param1, TelSimSecPw_t, 0);
			pin_data.pw = (unsigned char*)&g_array_index(in_param2, unsigned char, 0);
			if(pin_data.type == TAPI_SIM_PTYPE_PIN1) {
				en_facility.type = SIM_FACILITY_SC;
			} else if (pin_data.type == TAPI_SIM_PTYPE_SIM) {
				en_facility.type = SIM_FACILITY_PS;
			} else {
				api_err = TAPI_API_INVALID_INPUT;
				g_array_append_vals(*out_param2, &request_id, sizeof(int));
				goto OUT;
			}
			en_facility.password_length = pin_data.pw_len;
			memcpy(en_facility.password, pin_data.pw, en_facility.password_length);
			tcore_user_request_set_data(ur, sizeof(struct treq_sim_enable_facility), &en_facility);
			tcore_user_request_set_command(ur, TREQ_SIM_ENABLE_FACILITY);
			ret = tcore_communicator_dispatch_request(ctx->comm, ur);
			if (ret != TCORE_RETURN_SUCCESS) {
				api_err = TAPI_API_OPERATION_FAILED;
			}
			dbg("ret = 0x%x", ret);
			break;

		case TAPI_CS_SIM_GETPERSSTATUS:
			dbg("TAPI_CS_SIM_GETPERSSTATUS");
			g_array_append_vals(*out_param2, &request_id, sizeof(int));
			type = g_array_index(in_param1, TelSimPersType_t, 0);
			switch(type) {
				case TAPI_SIM_PERS_NET:
					facility.type = SIM_FACILITY_PN;
					break;
				case TAPI_SIM_PERS_NS:
					facility.type = SIM_FACILITY_PU;
					break;
				case TAPI_SIM_PERS_SP:
					facility.type = SIM_FACILITY_PP;
					break;
				case TAPI_SIM_PERS_CP:
					facility.type = SIM_FACILITY_PC;
					break;
				default:
					api_err = TAPI_API_INVALID_INPUT;
					goto OUT;
					break;
			}
			tcore_user_request_set_data(ur, sizeof(struct treq_sim_get_facility_status), &facility);
			tcore_user_request_set_command(ur, TREQ_SIM_GET_FACILITY_STATUS);
			ret = tcore_communicator_dispatch_request(ctx->comm, ur);
			if (ret != TCORE_RETURN_SUCCESS) {
				api_err = TAPI_API_OPERATION_FAILED;
			}
			dbg("ret = 0x%x", ret);
			break;

		case TAPI_CS_SIM_DISABLEPERS:
			dbg("TAPI_CS_SIM_DISABLEPERS");
			g_array_append_vals(*out_param2, &request_id, sizeof(int));
			pers_data= g_array_index(in_param1, TelSimPersPw_t, 0);
			pers_data.pw = (unsigned char*)&g_array_index(in_param2, unsigned char, 0);
			switch(pers_data.type) {
				case TAPI_SIM_PERS_NET:
					dis_facility.type = SIM_FACILITY_PN;
					break;
				case TAPI_SIM_PERS_NS:
					dis_facility.type = SIM_FACILITY_PU;
					break;
				case TAPI_SIM_PERS_SP:
					dis_facility.type = SIM_FACILITY_PP;
					break;
				case TAPI_SIM_PERS_CP:
					dis_facility.type = SIM_FACILITY_PC;
					break;
				default:
					api_err = TAPI_API_INVALID_INPUT;
					goto OUT;
					break;
			}
			dis_facility.password_length = pers_data.pw_len;
			memcpy(dis_facility.password, pers_data.pw, dis_facility.password_length);
			tcore_user_request_set_data(ur, sizeof(struct treq_sim_disable_facility), &dis_facility);
			tcore_user_request_set_command(ur, TREQ_SIM_DISABLE_FACILITY);
			ret = tcore_communicator_dispatch_request(ctx->comm, ur);
			if (ret != TCORE_RETURN_SUCCESS) {
				api_err = TAPI_API_OPERATION_FAILED;
			}
			dbg("ret = 0x%x", ret);
			break;

		case TAPI_CS_SIM_ENABLEPERS:
			dbg("TAPI_CS_SIM_ENABLEPERS");
			g_array_append_vals(*out_param2, &request_id, sizeof(int));
			pers_data= g_array_index(in_param1, TelSimPersPw_t, 0);
			pers_data.pw = (unsigned char*)&g_array_index(in_param2, unsigned char, 0);
			switch(pers_data.type) {
				case TAPI_SIM_PERS_NET:
					en_facility.type = SIM_FACILITY_PN;
					break;
				case TAPI_SIM_PERS_NS:
					en_facility.type = SIM_FACILITY_PU;
					break;
				case TAPI_SIM_PERS_SP:
					en_facility.type = SIM_FACILITY_PP;
					break;
				case TAPI_SIM_PERS_CP:
					en_facility.type = SIM_FACILITY_PC;
					break;
				default:
					api_err = TAPI_API_INVALID_INPUT;
					goto OUT;
					break;
			}
			en_facility.password_length = pers_data.pw_len;
			memcpy(en_facility.password, pers_data.pw, en_facility.password_length);
			tcore_user_request_set_data(ur, sizeof(struct treq_sim_enable_facility), &en_facility);
			tcore_user_request_set_command(ur, TREQ_SIM_ENABLE_FACILITY);
			ret = tcore_communicator_dispatch_request(ctx->comm, ur);
			if (ret != TCORE_RETURN_SUCCESS) {
				api_err = TAPI_API_OPERATION_FAILED;
			}
			dbg("ret = 0x%x", ret);
			break;

		case TAPI_CS_SIM_DISABLEFDN: //NOT USED FROM APP NOW
			dbg("TAPI_CS_SIM_DISABLEFDN");
			g_array_append_vals(*out_param2, &request_id, sizeof(int));
			pin2 = (unsigned char*)&g_array_index(in_param1,  unsigned char, 0);
			pin2_len = g_array_index(in_param2, unsigned int, 0);
			dis_facility.type = SIM_FACILITY_FD;
			dis_facility.password_length = pin2_len;
			memcpy(dis_facility.password, pin2, dis_facility.password_length);
			tcore_user_request_set_data(ur, sizeof(struct treq_sim_disable_facility), &dis_facility);
			tcore_user_request_set_command(ur, TREQ_SIM_DISABLE_FACILITY);
			ret = tcore_communicator_dispatch_request(ctx->comm, ur);
			if (ret != TCORE_RETURN_SUCCESS) {
				api_err = TAPI_API_OPERATION_FAILED;
			}
			dbg("ret = 0x%x", ret);
			break;

		case TAPI_CS_SIM_ENABLEFDN: //NOT USED FROM APP NOW
			dbg("TAPI_CS_SIM_ENABLEFDN");
			g_array_append_vals(*out_param2, &request_id, sizeof(int));
			pin2 = (unsigned char*)&g_array_index(in_param1,  unsigned char, 0);
			pin2_len = g_array_index(in_param2, unsigned int, 0);
			en_facility.type = SIM_FACILITY_FD;
			en_facility.password_length = pin2_len;
			memcpy(en_facility.password, pin2, en_facility.password_length);
			tcore_user_request_set_data(ur, sizeof(struct treq_sim_enable_facility), &en_facility);
			tcore_user_request_set_command(ur, TREQ_SIM_ENABLE_FACILITY);
			ret = tcore_communicator_dispatch_request(ctx->comm, ur);
			if (ret != TCORE_RETURN_SUCCESS) {
				api_err = TAPI_API_OPERATION_FAILED;
			}
			dbg("ret = 0x%x", ret);
			break;

		case TAPI_CS_SIM_APDU:
			dbg("TAPI_CS_SIM_APDU");
			g_array_append_vals(*out_param2, &request_id, sizeof(int));
			apdu_data = g_array_index(in_param1, TelSimApdu_t, 0);
			apdu_data.apdu =(unsigned char*)&g_array_index(in_param2, unsigned char, 0);
			send_apdu.apdu_length = apdu_data.apdu_len;
			memcpy(send_apdu.apdu, apdu_data.apdu, send_apdu.apdu_length);
			tcore_user_request_set_data(ur, sizeof(struct treq_sim_transmit_apdu), &send_apdu);
			tcore_user_request_set_command(ur, TREQ_SIM_TRANSMIT_APDU);
			ret = tcore_communicator_dispatch_request(ctx->comm, ur);
			if (ret != TCORE_RETURN_SUCCESS) {
				api_err = TAPI_API_OPERATION_FAILED;
			}
			dbg("ret = 0x%x", ret);
			break;

		case TAPI_CS_SIM_ATR:
			dbg("TAPI_CS_SIM_ATR");
			g_array_append_vals(*out_param2, &request_id, sizeof(int));
			tcore_user_request_set_command(ur, TREQ_SIM_GET_ATR);
			ret = tcore_communicator_dispatch_request(ctx->comm, ur);
			if (ret != TCORE_RETURN_SUCCESS) {
				api_err = TAPI_API_OPERATION_FAILED;
			}
			dbg("ret = 0x%x", ret);
			break;

		case TAPI_CS_SIM_GETSECSTATUS :
			dbg("TAPI_CS_SIM_GETSECSTATUS");
			type = g_array_index(in_param1, TelSimPinType_t, 0);
			if((int)type == TAPI_SIM_PTYPE_PIN1)
				status = ctx->pin_lock;
			else if((int)type == TAPI_SIM_PTYPE_PIN2)
				status = TAPI_SIM_PIN_STATUS_ENABLED;
			else if((int)type == TAPI_SIM_PTYPE_SIM)
				status = ctx->sim_lock;
			else
				status = TAPI_SIM_PIN_STATUS_UNKNOWN;

			g_array_append_vals (*out_param2, &status, sizeof(TelSimPinStatus_t));
			break;

		case TAPI_CS_SIM_GETPBINITINFO:
			dbg("TAPI_CS_SIM_GETPBINITINFO");
			co_list = tcore_plugin_get_core_objects_bytype(plugin, CORE_OBJECT_TYPE_PHONEBOOK);
			if (!co_list) {
				api_err = TAPI_API_NOT_SUPPORTED;
				goto OUT;
			}
			co_pb = (CoreObject *)co_list->data;
			g_slist_free(co_list);

			if (!co_pb) {
				api_err = TAPI_API_NOT_SUPPORTED;
				goto OUT;
			}
			lsp_init_completed = tcore_phonebook_get_status(co_pb);
			pb_list = (struct tel_phonebook_support_list*)tcore_phonebook_get_support_list(co_pb);
			memcpy(&lsp_supported_pb_list, pb_list, sizeof(struct tel_phonebook_support_list));
			g_array_append_vals (*out_param2, &lsp_init_completed,sizeof(int));
			g_array_append_vals (*out_param3, &lsp_supported_pb_list,sizeof(TelSimPbList_t));
			g_array_append_vals (*out_param4, &lsp_first_index,sizeof(int));
			break;

		case TAPI_CS_SIM_PB_GETCOUNT:
			dbg("TAPI_CS_SIM_PB_GETCOUNT");
			g_array_append_vals(*out_param2, &request_id, sizeof(int));
			lsp_pb_type =  g_array_index(in_param1, TelSimPbFileType_t, 0);
			switch(lsp_pb_type){
				case TAPI_SIM_PB_FDN :
					pb_count.phonebook_type = PB_TYPE_FDN;
					break;
				case TAPI_SIM_PB_ADN :
					pb_count.phonebook_type = PB_TYPE_ADN;
					break;
				case TAPI_SIM_PB_SDN :
					pb_count.phonebook_type = PB_TYPE_SDN;
					break;
				case TAPI_SIM_PB_3GSIM :
					pb_count.phonebook_type = PB_TYPE_USIM;
					break;
				case TAPI_SIM_PB_AAS :
					pb_count.phonebook_type = PB_TYPE_AAS;
					break;
				case TAPI_SIM_PB_GAS :
					pb_count.phonebook_type = PB_TYPE_GAS;
					break;
				default:
					break;
			}
			tcore_user_request_set_data(ur, sizeof(struct treq_phonebook_get_count), &pb_count);
			tcore_user_request_set_command(ur, TREQ_PHONEBOOK_GETCOUNT);
			ret = tcore_communicator_dispatch_request(ctx->comm, ur);
			if (ret != TCORE_RETURN_SUCCESS) {
				api_err = TAPI_API_OPERATION_FAILED;
			}
			dbg("ret = 0x%x", ret);
			break;

		case TAPI_CS_SIM_PB_GETMETAINFO: //NOT USED FROM APP NOW
			dbg("TAPI_CS_SIM_PB_GETMETAINFO");
			g_array_append_vals(*out_param2, &request_id, sizeof(int));
			lsp_pb_type =  g_array_index(in_param1, TelSimPbFileType_t, 0);
			switch(lsp_pb_type){
				case TAPI_SIM_PB_FDN :
					pb_info.phonebook_type = PB_TYPE_FDN;
					break;
				case TAPI_SIM_PB_ADN :
					pb_info.phonebook_type = PB_TYPE_ADN;
					break;
				case TAPI_SIM_PB_SDN :
					pb_info.phonebook_type = PB_TYPE_SDN;
					break;
				case TAPI_SIM_PB_3GSIM :
					pb_info.phonebook_type = PB_TYPE_USIM;
					break;
				case TAPI_SIM_PB_AAS :
					pb_info.phonebook_type = PB_TYPE_AAS;
					break;
				case TAPI_SIM_PB_GAS :
					pb_info.phonebook_type = PB_TYPE_GAS;
					break;
				default:
					break;
			}
			tcore_user_request_set_data(ur, sizeof(struct treq_phonebook_get_info), &pb_info);
			tcore_user_request_set_command(ur, TREQ_PHONEBOOK_GETMETAINFO);
			ret = tcore_communicator_dispatch_request(ctx->comm, ur);
			if (ret != TCORE_RETURN_SUCCESS) {
				api_err = TAPI_API_OPERATION_FAILED;
			}
			dbg("ret = 0x%x", ret);
			break;

		case TAPI_CS_SIM_PB_READRECORD:
			dbg("TAPI_CS_SIM_PB_READRECORD");
			g_array_append_vals(*out_param2, &request_id, sizeof(int));
			lsp_pb_type =  g_array_index(in_param1, TelSimPbFileType_t, 0);
			switch(lsp_pb_type){
				case TAPI_SIM_PB_FDN :
					pb_read.phonebook_type = PB_TYPE_FDN;
					break;
				case TAPI_SIM_PB_ADN :
					pb_read.phonebook_type = PB_TYPE_ADN;
					break;
				case TAPI_SIM_PB_SDN :
					pb_read.phonebook_type = PB_TYPE_SDN;
					break;
				case TAPI_SIM_PB_3GSIM :
					pb_read.phonebook_type = PB_TYPE_USIM;
					break;
				case TAPI_SIM_PB_AAS :
					pb_read.phonebook_type = PB_TYPE_AAS;
					break;
				case TAPI_SIM_PB_GAS :
					pb_read.phonebook_type = PB_TYPE_GAS;
					break;
				default:
					break;
			}
			lsp_index = g_array_index(in_param2, unsigned short, 0);
			pb_read.index = lsp_index;
			tcore_user_request_set_data(ur, sizeof(struct treq_phonebook_read_record), &pb_read);
			tcore_user_request_set_command(ur, TREQ_PHONEBOOK_READRECORD);
			ret = tcore_communicator_dispatch_request(ctx->comm, ur);
			if (ret != TCORE_RETURN_SUCCESS) {
				api_err = TAPI_API_OPERATION_FAILED;
			}
			dbg("ret = 0x%x", ret);
			break;

		case TAPI_CS_SIM_PB_UPDATERECORD: //NOT USED FROM APP NOW
			dbg("TAPI_CS_SIM_PB_UPDATERECORD");
			g_array_append_vals(*out_param2, &request_id, sizeof(int));
			lsp_pb_type =  g_array_index(in_param1, TelSimPbFileType_t, 0);
			switch(lsp_pb_type){
				case TAPI_SIM_PB_FDN :
					pb_update.phonebook_type = PB_TYPE_FDN;
					break;
				case TAPI_SIM_PB_ADN :
					pb_update.phonebook_type = PB_TYPE_ADN;
					break;
				case TAPI_SIM_PB_SDN :
					pb_update.phonebook_type = PB_TYPE_SDN;
					break;
				case TAPI_SIM_PB_3GSIM :
					pb_update.phonebook_type = PB_TYPE_USIM;
					break;
				case TAPI_SIM_PB_AAS :
					pb_update.phonebook_type = PB_TYPE_AAS;
					break;
				case TAPI_SIM_PB_GAS :
					pb_update.phonebook_type = PB_TYPE_GAS;
					break;
				default:
					break;
			}
			lsp_index = g_array_index(in_param2, unsigned short, 0);
			pb_update.index = lsp_index;

			tcore_user_request_set_data(ur, sizeof(struct treq_phonebook_update_record), &pb_update);
			tcore_user_request_set_command(ur, TREQ_PHONEBOOK_UPDATERECORD);
			ret = tcore_communicator_dispatch_request(ctx->comm, ur);
			if (ret != TCORE_RETURN_SUCCESS) {
				api_err = TAPI_API_OPERATION_FAILED;
			}
			dbg("ret = 0x%x", ret);
			break;

		case TAPI_CS_SIM_PB_DELETERECORD: //NOT USED FROM APP NOW
			dbg("TAPI_CS_SIM_PB_DELETERECORD");
			g_array_append_vals(*out_param2, &request_id, sizeof(int));
			lsp_pb_type =  g_array_index(in_param1, TelSimPbFileType_t, 0);
			switch(lsp_pb_type){
				case TAPI_SIM_PB_FDN :
					pb_delete.phonebook_type = PB_TYPE_FDN;
					break;
				case TAPI_SIM_PB_ADN :
					pb_delete.phonebook_type = PB_TYPE_ADN;
					break;
				case TAPI_SIM_PB_SDN :
					pb_delete.phonebook_type = PB_TYPE_SDN;
					break;
				case TAPI_SIM_PB_3GSIM :
					pb_delete.phonebook_type = PB_TYPE_USIM;
					break;
				case TAPI_SIM_PB_AAS :
					pb_delete.phonebook_type = PB_TYPE_AAS;
					break;
				case TAPI_SIM_PB_GAS :
					pb_delete.phonebook_type = PB_TYPE_GAS;
					break;
				default:
					break;
			}
			lsp_index = g_array_index(in_param2, unsigned short, 0);
			pb_delete.index = lsp_index;
			tcore_user_request_set_data(ur, sizeof(struct treq_phonebook_delete_record), &pb_delete);
			tcore_user_request_set_command(ur, TREQ_PHONEBOOK_DELETERECORD);
			ret = tcore_communicator_dispatch_request(ctx->comm, ur);
			if (ret != TCORE_RETURN_SUCCESS) {
				api_err = TAPI_API_OPERATION_FAILED;
			}
			dbg("ret = 0x%x", ret);
			break;

		case TAPI_CS_SIM_GETPBCAPABILITYINFO: //NOT USED FROM APP NOW
			dbg("TAPI_CS_SIM_GETPBCAPABILITYINFO");
			g_array_append_vals(*out_param2, &request_id, sizeof(int));
			tcore_user_request_set_data(ur, sizeof(struct treq_phonebook_get_usim_info), &pb_usim);
			tcore_user_request_set_command(ur, TREQ_PHONEBOOK_GETUSIMINFO);
			ret = tcore_communicator_dispatch_request(ctx->comm, ur);
			if (ret != TCORE_RETURN_SUCCESS) {
				api_err = TAPI_API_OPERATION_FAILED;
			}
			dbg("ret = 0x%x", ret);
			break;

		case TAPI_CS_SIM_SAPCONNECTREQUEST:
			g_array_append_vals(*out_param2, &request_id, sizeof(int));
			lsa_conn_req = g_array_index(in_param1, TelSimSapConnect_t, 0);
			if (lsa_conn_req.MsgId == TAPI_SIM_SAP_CONNECT_REQ) {
				req_conn.max_msg_size = lsa_conn_req.MaxMsgSize;
				tcore_user_request_set_data(ur, sizeof(struct treq_sap_req_connect), &req_conn);
				tcore_user_request_set_command(ur, TREQ_SAP_REQ_CONNECT);
			}else if(lsa_conn_req.MsgId == TAPI_SIM_SAP_DISCONNECT_REQ){
				tcore_user_request_set_data(ur, sizeof(struct treq_sap_req_disconnect), &req_conn);
				tcore_user_request_set_command(ur, TREQ_SAP_REQ_DISCONNECT);
			}else{
				dbg("error");
			}
			ret = tcore_communicator_dispatch_request(ctx->comm, ur);
			if (ret != TCORE_RETURN_SUCCESS) {
				api_err = TAPI_API_OPERATION_FAILED;
			}
			dbg("ret = 0x%x", ret);
			break;

		case TAPI_CS_SIM_SAPCONNECTSTATUS:
			g_array_append_vals(*out_param2, &request_id, sizeof(int));
			tcore_user_request_set_data(ur, sizeof(struct treq_sap_req_status), &req_status);
			tcore_user_request_set_command(ur, TREQ_SAP_REQ_STATUS);
			ret = tcore_communicator_dispatch_request(ctx->comm, ur);
			if (ret != TCORE_RETURN_SUCCESS) {
				api_err = TAPI_API_OPERATION_FAILED;
			}
			dbg("ret = 0x%x", ret);
			break;

		case TAPI_CS_SIM_SAPATRREQUEST:
			g_array_append_vals(*out_param2, &request_id, sizeof(int));
			tcore_user_request_set_data(ur, sizeof(struct treq_sap_req_atr), &req_atr);
			tcore_user_request_set_command(ur, TREQ_SAP_REQ_ATR);
			ret = tcore_communicator_dispatch_request(ctx->comm, ur);
			if (ret != TCORE_RETURN_SUCCESS) {
				api_err = TAPI_API_OPERATION_FAILED;
			}
			dbg("ret = 0x%x", ret);
			break;

		case TAPI_CS_SIM_SAPTRANSFERAPDU:
			g_array_append_vals(*out_param2, &request_id, sizeof(int));
			lsa_apdu_data = g_array_index(in_param1, TelSimSapApduData_t, 0);
			t_apdu.apdu_length = lsa_apdu_data.ApduLength;
			memcpy(t_apdu.apdu_data, lsa_apdu_data.Apdu, lsa_apdu_data.ApduLength);

			tcore_user_request_set_data(ur, sizeof(struct treq_sap_transfer_apdu), &t_apdu);
			tcore_user_request_set_command(ur, TREQ_SAP_TRANSFER_APDU);
			ret = tcore_communicator_dispatch_request(ctx->comm, ur);
			if (ret != TCORE_RETURN_SUCCESS) {
				api_err = TAPI_API_OPERATION_FAILED;
			}
			dbg("ret = 0x%x", ret);
			break;

		case TAPI_CS_SIM_SAPSETPROTOCOL:
			g_array_append_vals(*out_param2, &request_id, sizeof(int));
			lsa_protocol = g_array_index(in_param1, TelSimSapProtocol_t, 0);
			set_protocol.protocol = lsa_protocol;

			tcore_user_request_set_data(ur, sizeof(struct treq_sap_set_protocol), &set_protocol);
			tcore_user_request_set_command(ur, TREQ_SAP_SET_PROTOCOL);
			ret = tcore_communicator_dispatch_request(ctx->comm, ur);
			if (ret != TCORE_RETURN_SUCCESS) {
				api_err = TAPI_API_OPERATION_FAILED;
			}
			dbg("ret = 0x%x", ret);
			break;

		case TAPI_CS_SIM_SAPSETSIMPOWER:
			g_array_append_vals(*out_param2, &request_id, sizeof(int));
			lsa_msg_id = g_array_index(in_param1, TelSimSapMsgId_t, 0);
			if (lsa_msg_id == TAPI_SIM_SAP_POWER_SIM_OFF_REQ) {
				set_power.mode = SAP_POWER_OFF;
			} else if (lsa_msg_id == TAPI_SIM_SAP_POWER_SIM_ON_REQ) {
				set_power.mode = SAP_POWER_ON;
			} else if (lsa_msg_id == TAPI_SIM_SAP_RESET_SIM_REQ) {
				set_power.mode = SAP_POWER_RESET;
			} else {
				dbg("error");
			}

			tcore_user_request_set_data(ur, sizeof(struct treq_sap_set_power), &req_conn);
			tcore_user_request_set_command(ur, TREQ_SAP_SET_POWER);
			ret = tcore_communicator_dispatch_request(ctx->comm, ur);
			if (ret != TCORE_RETURN_SUCCESS) {
				api_err = TAPI_API_OPERATION_FAILED;
			}
			dbg("ret = 0x%x", ret);
			break;

		case TAPI_CS_SIM_SAPCARDREADERSTATUS:
			g_array_append_vals(*out_param2, &request_id, sizeof(int));
			tcore_user_request_set_data(ur, sizeof(struct treq_sap_req_cardreaderstatus), &req_conn);
			tcore_user_request_set_command(ur, TREQ_SAP_REQ_CARDREADERSTATUS);
			ret = tcore_communicator_dispatch_request(ctx->comm, ur);
			if (ret != TCORE_RETURN_SUCCESS) {
				api_err = TAPI_API_OPERATION_FAILED;
			}
			dbg("ret = 0x%x", ret);
			break;

		default:
			api_err = TAPI_API_NOT_SUPPORTED;
			break;
	}

OUT:
	if (api_err != TAPI_API_SUCCESS) {
		tcore_user_request_free(ur);
	}
	dbg("api_err[%d]",api_err);
	g_array_append_vals(*out_param1, &api_err, sizeof(int));
}

TReturn dbus_response_sim(struct custom_data *ctx, UserRequest *ur, const char *appname,
		enum tcore_response_command command, unsigned int data_len, const void *data)
{
	const struct tresp_sim_verify_pins *resp_verify_pins = data;
	const struct tresp_sim_verify_puks *resp_verify_puks = data;
	const struct tresp_sim_change_pins *resp_change_pins = data;
	const struct tresp_sim_get_facility_status *resp_get_facility = data;
	const struct tresp_sim_disable_facility *resp_dis_facility = data;
	const struct tresp_sim_enable_facility *resp_en_facility = data;
	const struct tresp_sim_transmit_apdu *resp_apdu = data;
	const struct tresp_sim_get_atr *resp_get_atr = data;
	const struct tresp_sim_read *resp_read = data;
	const struct tresp_sim_req_authentication *resp_auth = data;
	const struct tresp_sim_set_language *resp_set_language = data;

	TelSimSecResult_t sec_rt = {0,};
	TelSimPersStatus_t pers_rt = {0,};
	TelSimApduResp_t apdu_resp = {0,};
	TelSimAtrResp_t atr_resp = {0,};
	TelSimAuthenticationResponse_t auth_resp = {0,};

	const struct tresp_phonebook_get_count *resp_pbcnt = data;
	const struct tresp_phonebook_get_info *resp_entry = data;
	const struct tresp_phonebook_get_usim_info *resp_capa = data;
	const struct tresp_phonebook_read_record *resp_pbread = data;
	const struct tresp_phonebook_update_record *resp_pbupdate = data;
	const struct tresp_phonebook_delete_record *resp_pbdelete = data;

	TelSimPbCapabilityInfo_t lpb_capa = {0,};
	TelSimPbStorageInfo_t lpb_cnt = {0,};
	TelSimPbEntryInfo_t lpb_entry = {0,};
	//TelSimPbRecordData_t lpb_data = {0,};
	TelSimPbRecord_t lcpb_data = {0,};

	const struct tresp_sap_req_connect *sap_conn = data;
	const struct tresp_sap_req_status *sap_status = data;
	const struct tresp_sap_req_atr *sap_atr = data;
	const struct tresp_sap_transfer_apdu *sap_apdu = data;
	const struct tresp_sap_set_protocol *sap_protocol = data;
	const struct tresp_sap_set_power *sap_power = data;
	const struct tresp_sap_req_cardreaderstatus *sap_reader = data;

	TelSimSapConnect_t lsa_conn = { 0, };
	TelSimSapConnect_t lsa_disconn = { 0, };
	unsigned int lsa_status = 0;
	TelSimSapAtrInfo_t lsa_atr = { 0, };
	TelSimSapApduData_t lsa_apdu = { 0, };
	TelSimSapResultCode_t lsa_result_code = 0;
	TelSimSapPower_t lsa_power = { 0, };
	TelSimCardReaderStatus_t lsa_reader = { 0, };

	int request_id = 0, i =0;

	GSList *co_list;
	CoreObject *co_sim;
	char *modem_name = NULL;
	TcorePlugin *p = NULL;

	Server *s = NULL;
	Storage *strg = NULL;

	char *tmp_msisdn = NULL;

	modem_name = tcore_user_request_get_modem_name(ur);
	if (!modem_name)
		return FALSE;

	p = tcore_server_find_plugin(ctx->server, modem_name);
	free(modem_name);
	if (!p)
		return FALSE;

	co_list = tcore_plugin_get_core_objects_bytype(p, CORE_OBJECT_TYPE_SIM);
	if (!co_list) {
		return FALSE;
	}

	co_sim = (CoreObject *)co_list->data;
	g_slist_free(co_list);

	if (!co_sim) {
		return FALSE;
	}

	s = tcore_plugin_ref_server(p);
	if(!s){
		dbg("there is no valid server at this point");
		return FALSE;
	}

	strg = (Storage*)tcore_server_find_storage(s, "vconf");
	if(!strg){
		dbg("there is no valid storage plugin");
		return FALSE;
	}

	switch (command) {
		case TRESP_SIM_VERIFY_PINS:
			dbg("dbus comm - TRESP_SIM_VERIFY_PINS");
			sec_rt.retry_count = resp_verify_pins->retry_count;
			sec_rt.type = resp_verify_pins->pin_type;
			return ts_delivery_event(ctx->EvtDeliveryHandle, TAPI_EVENT_CLASS_SIM,
					TAPI_EVENT_SIM_VERIFY_SEC_CNF, appname, request_id, resp_verify_pins->result,
					sizeof(TelSimSecResult_t), (void*) &sec_rt);
			break;

		case TRESP_SIM_VERIFY_PUKS:
			dbg("dbus comm - TRESP_SIM_VERIFY_PUKS");
			sec_rt.retry_count = resp_verify_puks->retry_count;
			sec_rt.type = resp_verify_puks->pin_type;
			return ts_delivery_event(ctx->EvtDeliveryHandle, TAPI_EVENT_CLASS_SIM,
					TAPI_EVENT_SIM_VERIFY_PUK_CNF, appname, request_id, resp_verify_puks->result,
					sizeof(TelSimSecResult_t), (void*) &sec_rt);
			break;

		case TRESP_SIM_CHANGE_PINS:
			dbg("dbus comm - TRESP_SIM_CHANGE_PINS");
			sec_rt.retry_count = resp_change_pins->retry_count;
			sec_rt.type = resp_change_pins->pin_type;
			return ts_delivery_event(ctx->EvtDeliveryHandle, TAPI_EVENT_CLASS_SIM,
					TAPI_EVENT_SIM_CHANGE_PINS_CNF, appname, request_id, resp_change_pins->result,
					sizeof(TelSimSecResult_t), (void*) &sec_rt);
			break;

		case TRESP_SIM_GET_FACILITY_STATUS:
			dbg("dbus comm - TRESP_SIM_GET_FACILITY_STATUS");
			switch(resp_get_facility->type){
				case SIM_FACILITY_PN:
					pers_rt.type = TAPI_SIM_PERS_NET;
					break;
				case SIM_FACILITY_PU:
					pers_rt.type = TAPI_SIM_PERS_NS;
					break;
				case SIM_FACILITY_PP:
					pers_rt.type = TAPI_SIM_PERS_SP;
					break;
				case SIM_FACILITY_PC:
					pers_rt.type = TAPI_SIM_PERS_CP;
					break;
				case SIM_FACILITY_PS:
				case SIM_FACILITY_SC:
				case SIM_FACILITY_FD:
					return dbus_sim_security_update(ctx, resp_get_facility->type, resp_get_facility->result, resp_get_facility->b_enable);
					break;
				default:
					break;
			}
			pers_rt.mode = resp_get_facility->b_enable;
			return ts_delivery_event(ctx->EvtDeliveryHandle, TAPI_EVENT_CLASS_SIM,
					TAPI_EVENT_SIM_PERS_STATUS_CNF, appname, request_id, resp_get_facility->result,
					sizeof(TelSimPersStatus_t), (void*) &pers_rt);
			break;

		case TRESP_SIM_DISABLE_FACILITY:
			dbg("dbus comm - TRESP_SIM_DISABLE_FACILITY");
			dbus_sim_security_update(ctx, resp_en_facility->type, resp_en_facility->result, 0);
			sec_rt.retry_count = resp_dis_facility->retry_count;
			switch(resp_dis_facility->type){
				case SIM_FACILITY_PS :
					sec_rt.type = TAPI_SIM_PTYPE_SIM;
				case SIM_FACILITY_SC:
					sec_rt.type = TAPI_SIM_PTYPE_PIN1;
					return ts_delivery_event(ctx->EvtDeliveryHandle, TAPI_EVENT_CLASS_SIM,
							TAPI_EVENT_SIM_DISABLE_SEC_CNF, appname, request_id, resp_dis_facility->result,
							sizeof(TelSimSecResult_t), (void*) &sec_rt);
					break;

				case SIM_FACILITY_FD:
					sec_rt.type = TAPI_SIM_PTYPE_PIN2;
					return ts_delivery_event(ctx->EvtDeliveryHandle, TAPI_EVENT_CLASS_SIM,
							TAPI_EVENT_SIM_DISABLE_FDNMODE_CNF, appname, request_id, resp_dis_facility->result,
							sizeof(TelSimSecResult_t), (void*) &sec_rt);
					break;

				case SIM_FACILITY_PN:
					sec_rt.type = TAPI_SIM_PERS_NET;
				case SIM_FACILITY_PU:
					sec_rt.type = TAPI_SIM_PERS_NS;
				case SIM_FACILITY_PP:
					pers_rt.type = TAPI_SIM_PERS_SP;
				case SIM_FACILITY_PC:
					pers_rt.type = TAPI_SIM_PERS_CP;
					return ts_delivery_event(ctx->EvtDeliveryHandle, TAPI_EVENT_CLASS_SIM,
							TAPI_EVENT_SIM_DISABLE_PERS_CNF, appname, request_id, resp_dis_facility->result, 0, NULL);
					break;
				default:
					break;
			}
			break;

		case TRESP_SIM_ENABLE_FACILITY:
			dbg("dbus comm - TRESP_SIM_ENABLE_FACILITY");
			sec_rt.retry_count = resp_en_facility->retry_count;
			dbus_sim_security_update(ctx, resp_en_facility->type, resp_en_facility->result, 1);
			switch(resp_en_facility->type){
				case SIM_FACILITY_PS :
					sec_rt.type = TAPI_SIM_PTYPE_SIM;
				case SIM_FACILITY_SC:
					sec_rt.type = TAPI_SIM_PTYPE_PIN1;
					dbg("result[%d], sec_rt type[%d],retry cnt[%d]",resp_en_facility->result, sec_rt.type, sec_rt.retry_count);
					return ts_delivery_event(ctx->EvtDeliveryHandle, TAPI_EVENT_CLASS_SIM,
							TAPI_EVENT_SIM_ENABLE_SEC_CNF, appname, request_id, resp_en_facility->result,
							sizeof(TelSimSecResult_t), (void*) &sec_rt);
					break;

				case SIM_FACILITY_FD:
					sec_rt.type = TAPI_SIM_PTYPE_PIN2;
					return ts_delivery_event(ctx->EvtDeliveryHandle, TAPI_EVENT_CLASS_SIM,
							TAPI_EVENT_SIM_ENABLE_FDNMODE_CNF, appname, request_id, resp_en_facility->result,
							sizeof(TelSimSecResult_t), (void*) &sec_rt);
					break;

				case SIM_FACILITY_PN:
					sec_rt.type = TAPI_SIM_PERS_NET;
				case SIM_FACILITY_PU:
					sec_rt.type = TAPI_SIM_PERS_NS;
				case SIM_FACILITY_PP:
					pers_rt.type = TAPI_SIM_PERS_SP;
				case SIM_FACILITY_PC:
					pers_rt.type = TAPI_SIM_PERS_CP;
					return ts_delivery_event(ctx->EvtDeliveryHandle, TAPI_EVENT_CLASS_SIM,
							TAPI_EVENT_SIM_ENABLE_PERS_CNF, appname, request_id, resp_en_facility->result, 0, NULL);
					break;
				default:
					break;
			}
			break;

		case TRESP_SIM_TRANSMIT_APDU:
			dbg("dbus comm - TRESP_SIM_TRANSMIT_APDU");
			apdu_resp.apdu_resp_len = resp_apdu->apdu_resp_length;
			memcpy(apdu_resp.apdu_resp, resp_apdu->apdu_resp, apdu_resp.apdu_resp_len);
			return ts_delivery_event(ctx->EvtDeliveryHandle, TAPI_EVENT_CLASS_SIM,
					TAPI_EVENT_SIM_APDU_CNF, appname, request_id, resp_apdu->result,
					sizeof(TelSimApduResp_t), (void*)&apdu_resp);
			break;

		case TRESP_SIM_GET_ATR:
			dbg("dbus comm - TRESP_SIM_GET_ATR");
			atr_resp.atr_resp_len = resp_get_atr->atr_length;
			memcpy(atr_resp.atr_resp, resp_get_atr->atr, atr_resp.atr_resp_len);
			return ts_delivery_event(ctx->EvtDeliveryHandle, TAPI_EVENT_CLASS_SIM,
					TAPI_EVENT_SIM_ATR_CNF, appname, request_id, resp_get_atr->result,
					sizeof(TelSimAtrResp_t), (void*)&atr_resp);
			break;

		case TRESP_SIM_REQ_AUTHENTICATION:
			dbg("dbus comm - TRESP_SIM_REQ_AUTHENTICATION");
			auth_resp.auth_result = resp_auth->auth_result;
			auth_resp.auth_type = resp_auth->auth_type;
			if(resp_auth->authentication_key_length) {
				auth_resp.authentication_key_length = resp_auth->authentication_key_length;
				memcpy(auth_resp.authentication_key, resp_auth->authentication_key, auth_resp.authentication_key_length);
			}
			if(resp_auth->cipher_length) {
				auth_resp.cipher_length = resp_auth->cipher_length;
				memcpy(auth_resp.cipher_data, resp_auth->cipher_data, auth_resp.cipher_length);
			}
			if(resp_auth->integrity_length) {
				auth_resp.integrity_length = resp_auth->integrity_length;
				memcpy(auth_resp.integrity_data, resp_auth->integrity_data, auth_resp.integrity_length);
			}
			if(resp_auth->resp_length) {
				auth_resp.resp_length = resp_auth->resp_length;
				memcpy(auth_resp.resp_data, resp_auth->resp_data, auth_resp.resp_length);
			}
			return ts_delivery_event(ctx->EvtDeliveryHandle, TAPI_EVENT_CLASS_SIM, TAPI_EVENT_SIM_AUTHENTICATION_CNF,
					appname, request_id, resp_auth->result, sizeof(TelSimAuthenticationResponse_t),
					(void*) &auth_resp);
			break;

		case TRESP_SIM_SET_LANGUAGE:
			dbg("dbus comm - TRESP_SIM_SET_LANGUAGE");
			return ts_delivery_event(ctx->EvtDeliveryHandle, TAPI_EVENT_CLASS_SIM, TAPI_EVENT_SIM_SET_LANGUAGE_CNF,
					appname, request_id, resp_set_language->result, 0,NULL);
			break;

		case TRESP_SIM_GET_ECC:
			dbg("dbus comm - TRESP_SIM_GET_ECC");
			dbg("resp_read->result[%x]",resp_read->result);
			if (tcore_sim_get_type(co_sim) == SIM_TYPE_GSM) {
				ctx->ecc_count = resp_read->data.ecc.ecc_count;
				if (strlen(resp_read->data.ecc.ecc[0].ecc_num))
					snprintf(ctx->ecc.EccInfo.szEcc1, TAPI_SIM_ECC_CODE_LEN * 2 + 1, "%s", resp_read->data.ecc.ecc[0].ecc_num);
				if (strlen(resp_read->data.ecc.ecc[1].ecc_num))
					snprintf(ctx->ecc.EccInfo.szEcc2, TAPI_SIM_ECC_CODE_LEN * 2 + 1, "%s", resp_read->data.ecc.ecc[1].ecc_num);
				if (strlen(resp_read->data.ecc.ecc[2].ecc_num))
					snprintf(ctx->ecc.EccInfo.szEcc3, TAPI_SIM_ECC_CODE_LEN * 2 + 1, "%s", resp_read->data.ecc.ecc[2].ecc_num);
				if (strlen(resp_read->data.ecc.ecc[3].ecc_num))
					snprintf(ctx->ecc.EccInfo.szEcc4, TAPI_SIM_ECC_CODE_LEN * 2 + 1, "%s", resp_read->data.ecc.ecc[3].ecc_num);
				if (strlen(resp_read->data.ecc.ecc[4].ecc_num))
					snprintf(ctx->ecc.EccInfo.szEcc5, TAPI_SIM_ECC_CODE_LEN * 2 + 1, "%s", resp_read->data.ecc.ecc[4].ecc_num);
			} else if (tcore_sim_get_type(co_sim) == SIM_TYPE_USIM) {
				ctx->ecc_count = resp_read->data.ecc.ecc_count;
				for (i = 0; i < resp_read->data.ecc.ecc_count; i++) {
					ctx->ecc.UeccInfo[i].EccLen = strlen(resp_read->data.ecc.ecc[i].ecc_num);
					if (ctx->ecc.UeccInfo[i].EccLen) {
						snprintf(ctx->ecc.UeccInfo[i].szEcc, ctx->ecc.UeccInfo[i].EccLen + 1, "%s",	resp_read->data.ecc.ecc[i].ecc_num);
						ctx->ecc.UeccInfo[i].bUsed = 1;
					}
					if (strlen(resp_read->data.ecc.ecc[i].ecc_string))
						snprintf( ctx->ecc.UeccInfo[i].szEccAlphaIdentifier, strlen(resp_read->data.ecc.ecc[i].ecc_string) + 1, "%s",	resp_read->data.ecc.ecc[i].ecc_string);
					ctx->ecc.UeccInfo[i].EccEmergencyServiceInfo = resp_read->data.ecc.ecc[i].ecc_category;
				}
			}
			break;

		case TRESP_SIM_GET_LANGUAGE:
			dbg("dbus comm - TRESP_SIM_GET_LANGUAGE");
			dbg("resp_read->result[%x]",resp_read->result);
			ctx->language.LpCount = resp_read->data.language.language_count;
			for(i=0; i < (int)ctx->language.LpCount; i++)
				ctx->language.Lp[i] = 	resp_read->data.language.language[i];
			break;

		case TRESP_SIM_GET_ICCID:
			dbg("dbus comm - TRESP_SIM_GET_ICCID");
			dbg("resp_read->result[%x]",resp_read->result);
			ctx->iccid.icc_length = strlen(resp_read->data.iccid.iccid);
			if(ctx->iccid.icc_length > 0)
				memcpy(ctx->iccid.icc_num, resp_read->data.iccid.iccid, ctx->iccid.icc_length);
			break;

		case TRESP_SIM_GET_MSISDN:
			dbg("dbus comm - TRESP_SIM_GET_MSISDN");
			dbg("resp_read->result[%x]",resp_read->result);
			if(resp_read->data.msisdn_list.count > 0) {
				dbg("resp_read->data.msisdn_list.msisdn[0].name[%s]",resp_read->data.msisdn_list.msisdn[0].name);
				dbg("resp_read->data.msisdn_list.msisdn[0].num[%s]",resp_read->data.msisdn_list.msisdn[0].num);
				tmp_msisdn = calloc(strlen((char *)resp_read->data.msisdn_list.msisdn[0].num)+2,1);
				if(resp_read->data.msisdn_list.msisdn[0].ton == SIM_TON_INTERNATIONAL){
					dbg("current msisdn is international number");
					tmp_msisdn[0] = '+';
					snprintf(&tmp_msisdn[1], strlen((char *)resp_read->data.msisdn_list.msisdn[0].num)+1, "%s", resp_read->data.msisdn_list.msisdn[0].num);
				} else {
					snprintf(tmp_msisdn, strlen((char *)resp_read->data.msisdn_list.msisdn[0].num)+1, "%s", resp_read->data.msisdn_list.msisdn[0].num);
				}
				if (tcore_storage_set_string(strg, STORAGE_KEY_TELEPHONY_SUBSCRIBER_NAME, (const char*) &resp_read->data.msisdn_list.msisdn[0].name) == FALSE )
					dbg("[FAIL] UPDATE STORAGE_KEY_TELEPHONY_SUBSCRIBER_NAME");
				if (tcore_storage_set_string(strg, STORAGE_KEY_TELEPHONY_SUBSCRIBER_NUMBER, (const char*) tmp_msisdn) == FALSE )
						dbg("[FAIL] UPDATE STORAGE_KEY_TELEPHONY_SUBSCRIBER_NUMBER");

				g_free(tmp_msisdn);
			}


			break;

		case TRESP_SIM_GET_MAILBOX:
		case TRESP_SIM_GET_CALLFORWARDING:
		case TRESP_SIM_GET_MESSAGEWAITING:
		case TRESP_SIM_GET_CPHS_INFO:
		case TRESP_SIM_GET_OPLMNWACT:
			break;

		case TRESP_PHONEBOOK_GETCOUNT:
			dbg("dbus comm - TRESP_PHONEBOOK_GETCOUNT");
			switch(resp_pbcnt->type){
				case PB_TYPE_FDN:
					lpb_cnt.StorageFileType = TAPI_SIM_PB_FDN;
					break;
				case PB_TYPE_ADN:
					lpb_cnt.StorageFileType = TAPI_SIM_PB_ADN;
					break;
				case PB_TYPE_SDN:
					lpb_cnt.StorageFileType = TAPI_SIM_PB_SDN;
					break;
				case PB_TYPE_USIM:
					lpb_cnt.StorageFileType = TAPI_SIM_PB_3GSIM;
					break;
				case PB_TYPE_AAS:
					lpb_cnt.StorageFileType = TAPI_SIM_PB_AAS;
					break;
				case PB_TYPE_GAS:
					lpb_cnt.StorageFileType = TAPI_SIM_PB_GAS;
					break;
				default:
					dbg("not handled type[%d]",resp_pbcnt->type);
					break;
			}
			lpb_cnt.UsedRecordCount = resp_pbcnt->used_count;
			lpb_cnt.TotalRecordCount = resp_pbcnt->total_count;
			return ts_delivery_event(ctx->EvtDeliveryHandle, TAPI_EVENT_CLASS_SIM,
					TAPI_EVENT_SIM_PB_STORAGE_COUNT_CNF, appname, request_id, resp_pbcnt->result,
					sizeof(TelSimPbStorageInfo_t), (void*) &lpb_cnt);
			break;

		case TRESP_PHONEBOOK_GETMETAINFO:
			dbg("dbus comm - TRESP_PHONEBOOK_GETMETAINFO");
			switch(resp_entry->type){
				case PB_TYPE_FDN:
					lpb_entry.StorageFileType = TAPI_SIM_PB_FDN;
					break;
				case PB_TYPE_ADN:
					lpb_entry.StorageFileType = TAPI_SIM_PB_ADN;
					break;
				case PB_TYPE_SDN:
					lpb_entry.StorageFileType = TAPI_SIM_PB_SDN;
					break;
				case PB_TYPE_USIM:
					lpb_entry.StorageFileType = TAPI_SIM_PB_3GSIM;
					break;
				case PB_TYPE_AAS:
					lpb_entry.StorageFileType = TAPI_SIM_PB_AAS;
					break;
				case PB_TYPE_GAS:
					lpb_entry.StorageFileType = TAPI_SIM_PB_GAS;
					break;
				default:
					dbg("not handled type[%d]",resp_entry->type);
					break;
			}
			lpb_entry.PbIndexMax =  resp_entry->index_max;
			lpb_entry.PbIndexMin = resp_entry->index_min;
			lpb_entry.PbNumLenMax = resp_entry->number_length_max;
			lpb_entry.PbTextLenMax = resp_entry->text_length_max;
			return ts_delivery_event(ctx->EvtDeliveryHandle, TAPI_EVENT_CLASS_SIM,
					TAPI_EVENT_SIM_PB_ENTRY_INFO_CNF, appname, request_id, resp_entry->result,
					sizeof(TelSimPbEntryInfo_t), (void*) &lpb_entry);
			break;

		case TRESP_PHONEBOOK_GETUSIMINFO:
			dbg("dbus comm - TRESP_PHONEBOOK_GETUSIMINFO");
			return ts_delivery_event(ctx->EvtDeliveryHandle, TAPI_EVENT_CLASS_SIM,
					TAPI_EVENT_SIM_PB_CAPABILITY_INFO_CNF, appname, request_id, resp_capa->result,
					sizeof(TelSimPbCapabilityInfo_t), (void*) &lpb_capa);
			break;

		case TRESP_PHONEBOOK_READRECORD:
			dbg("dbus comm - TRESP_PHONEBOOK_READRECORD");
			lcpb_data.index = resp_pbread->index;
			lcpb_data.next_index = resp_pbread->next_index;

			switch(resp_pbread->phonebook_type){
				case PB_TYPE_FDN:
					lcpb_data.phonebook_type = TAPI_SIM_PB_FDN;
					break;
				case PB_TYPE_ADN:
					lcpb_data.phonebook_type = TAPI_SIM_PB_ADN;
					break;
				case PB_TYPE_SDN:
					lcpb_data.phonebook_type = TAPI_SIM_PB_SDN;
					break;
				case PB_TYPE_USIM:
					lcpb_data.phonebook_type = TAPI_SIM_PB_3GSIM;
					break;
				case PB_TYPE_AAS:
					lcpb_data.phonebook_type = TAPI_SIM_PB_AAS;
					break;
				case PB_TYPE_GAS:
					lcpb_data.phonebook_type = TAPI_SIM_PB_GAS;
					break;
				default:
					dbg("not handled type[%d]",resp_pbread->phonebook_type);
					break;
			}
			if(strlen((char*)resp_pbread->name)) {
				snprintf((char *)lcpb_data.name, strlen((char*)resp_pbread->name)+1, "%s", resp_pbread->name);
				switch( resp_pbread->dcs ){
					case PB_TEXT_ASCII :
						lcpb_data.dcs = TAPI_SIM_TEXT_ENC_ASCII;
						break;
					case PB_TEXT_GSM7BIT :
						lcpb_data.dcs = TAPI_SIM_TEXT_ENC_GSM7BIT;
						break;
					case PB_TEXT_UCS2 :
						lcpb_data.dcs = TAPI_SIM_TEXT_ENC_UCS2;
						break;
					case PB_TEXT_HEX :
						lcpb_data.dcs = TAPI_SIM_TEXT_ENC_HEX;
						break;
					default:
						dbg("not handled pb type[%d]", resp_pbread->dcs);
						break;
				}
			}
			if(strlen((char*)resp_pbread->number)) {
				snprintf((char *)lcpb_data.number, strlen((char*)resp_pbread->number)+1, "%s", resp_pbread->number);
				switch( resp_pbread->ton ){
					case PB_TON_UNKNOWN :
						lcpb_data.ton = TAPI_SIM_TON_UNKNOWN;
						break;
					case PB_TON_INTERNATIONAL :
						lcpb_data.ton = TAPI_SIM_TON_INTERNATIONAL;
						break;
					case PB_TON_NATIONAL :
						lcpb_data.ton = TAPI_SIM_TON_NATIONAL;
						break;
					case PB_TON_NETWORK_SPECIFIC :
						lcpb_data.ton = TAPI_SIM_TON_NETWORK_SPECIFIC;
						break;
					case PB_TON_DEDICATED_ACCESS :
						lcpb_data.ton = TAPI_SIM_TON_DEDICATED_ACCESS;
						break;
					case PB_TON_ALPHA_NUMERIC :
						lcpb_data.ton = TAPI_SIM_TON_ALPHA_NUMERIC;
						break;
					case PB_TON_ABBREVIATED_NUMBER :
						lcpb_data.ton = TAPI_SIM_TON_ABBREVIATED_NUMBER;
						break;
					case PB_TON_RESERVED_FOR_EXT :
						lcpb_data.ton = TAPI_SIM_TON_RESERVED_FOR_EXT;
						break;
					default:
						dbg("not handled pb ton[%d]", resp_pbread->ton);
						break;
				}
			}

			if(strlen((char*)resp_pbread->anr1)) {
				snprintf((char *)lcpb_data.anr1, strlen((char*)resp_pbread->anr1)+1, "%s", resp_pbread->anr1);
				switch( resp_pbread->anr1_ton ){
					case PB_TON_UNKNOWN :
						lcpb_data.anr1_ton = TAPI_SIM_TON_UNKNOWN;
						break;
					case PB_TON_INTERNATIONAL :
						lcpb_data.anr1_ton = TAPI_SIM_TON_INTERNATIONAL;
						break;
					case PB_TON_NATIONAL :
						lcpb_data.anr1_ton = TAPI_SIM_TON_NATIONAL;
						break;
					case PB_TON_NETWORK_SPECIFIC :
						lcpb_data.anr1_ton = TAPI_SIM_TON_NETWORK_SPECIFIC;
						break;
					case PB_TON_DEDICATED_ACCESS :
						lcpb_data.anr1_ton = TAPI_SIM_TON_DEDICATED_ACCESS;
						break;
					case PB_TON_ALPHA_NUMERIC :
						lcpb_data.anr1_ton = TAPI_SIM_TON_ALPHA_NUMERIC;
						break;
					case PB_TON_ABBREVIATED_NUMBER :
						lcpb_data.anr1_ton = TAPI_SIM_TON_ABBREVIATED_NUMBER;
						break;
					case PB_TON_RESERVED_FOR_EXT :
						lcpb_data.anr1_ton = TAPI_SIM_TON_RESERVED_FOR_EXT;
						break;
					default:
						dbg("not handled pb anr1_ton[%d]", resp_pbread->anr1_ton);
						break;
				}
			}

			if(strlen((char*)resp_pbread->anr2)) {
				snprintf((char *)lcpb_data.anr2, strlen((char*)resp_pbread->anr2)+1, "%s", resp_pbread->anr2);
				switch( resp_pbread->anr2_ton ){
					case PB_TON_UNKNOWN :
						lcpb_data.anr2_ton = TAPI_SIM_TON_UNKNOWN;
						break;
					case PB_TON_INTERNATIONAL :
						lcpb_data.anr2_ton = TAPI_SIM_TON_INTERNATIONAL;
						break;
					case PB_TON_NATIONAL :
						lcpb_data.anr2_ton = TAPI_SIM_TON_NATIONAL;
						break;
					case PB_TON_NETWORK_SPECIFIC :
						lcpb_data.anr2_ton = TAPI_SIM_TON_NETWORK_SPECIFIC;
						break;
					case PB_TON_DEDICATED_ACCESS :
						lcpb_data.anr2_ton = TAPI_SIM_TON_DEDICATED_ACCESS;
						break;
					case PB_TON_ALPHA_NUMERIC :
						lcpb_data.anr2_ton = TAPI_SIM_TON_ALPHA_NUMERIC;
						break;
					case PB_TON_ABBREVIATED_NUMBER :
						lcpb_data.anr2_ton = TAPI_SIM_TON_ABBREVIATED_NUMBER;
						break;
					case PB_TON_RESERVED_FOR_EXT :
						lcpb_data.anr2_ton = TAPI_SIM_TON_RESERVED_FOR_EXT;
						break;
					default:
						dbg("not handled pb anr2_ton[%d]", resp_pbread->anr2_ton);
						break;
				}
			}

			if(strlen((char*)resp_pbread->anr3)) {
				snprintf((char *)lcpb_data.anr3, strlen((char*)resp_pbread->anr3)+1, "%s", resp_pbread->anr3);
				switch( resp_pbread->anr3_ton ){
					case PB_TON_UNKNOWN :
						lcpb_data.anr3_ton = TAPI_SIM_TON_UNKNOWN;
						break;
					case PB_TON_INTERNATIONAL :
						lcpb_data.anr3_ton = TAPI_SIM_TON_INTERNATIONAL;
						break;
					case PB_TON_NATIONAL :
						lcpb_data.anr3_ton = TAPI_SIM_TON_NATIONAL;
						break;
					case PB_TON_NETWORK_SPECIFIC :
						lcpb_data.anr3_ton = TAPI_SIM_TON_NETWORK_SPECIFIC;
						break;
					case PB_TON_DEDICATED_ACCESS :
						lcpb_data.anr3_ton = TAPI_SIM_TON_DEDICATED_ACCESS;
						break;
					case PB_TON_ALPHA_NUMERIC :
						lcpb_data.anr3_ton = TAPI_SIM_TON_ALPHA_NUMERIC;
						break;
					case PB_TON_ABBREVIATED_NUMBER :
						lcpb_data.anr3_ton = TAPI_SIM_TON_ABBREVIATED_NUMBER;
						break;
					case PB_TON_RESERVED_FOR_EXT :
						lcpb_data.anr3_ton = TAPI_SIM_TON_RESERVED_FOR_EXT;
						break;
					default:
						dbg("not handled pb anr3_ton[%d]", resp_pbread->anr3_ton);
						break;
				}
			}

			if(strlen((char*)resp_pbread->email1))
				snprintf((char *)lcpb_data.email1, strlen((char*)resp_pbread->email1)+1, "%s", resp_pbread->email1);
			if(strlen((char*)resp_pbread->email2))
				snprintf((char *)lcpb_data.email2, strlen((char*)resp_pbread->email2)+1, "%s", resp_pbread->email2);
			if(strlen((char*)resp_pbread->email3))
				snprintf((char *)lcpb_data.email3, strlen((char*)resp_pbread->email3)+1, "%s", resp_pbread->email3);
			if(strlen((char*)resp_pbread->email4))
				snprintf((char *)lcpb_data.email4, strlen((char*)resp_pbread->email4)+1, "%s", resp_pbread->email4);

			lcpb_data.group_index = resp_pbread->group_index;
			lcpb_data.pb_control = resp_pbread->pb_control;

			return ts_delivery_event(ctx->EvtDeliveryHandle, TAPI_EVENT_CLASS_SIM,
					TAPI_EVENT_SIM_PB_ACCESS_READ_CNF, appname, request_id, resp_pbread->result,
					sizeof(TelSimPbRecordData_t), (void*) &lcpb_data);
			break;

		case TRESP_PHONEBOOK_UPDATERECORD:
			dbg("dbus comm - TRESP_PHONEBOOK_UPDATERECORD");
			return ts_delivery_event(ctx->EvtDeliveryHandle, TAPI_EVENT_CLASS_SIM,
					TAPI_EVENT_SIM_PB_UPDATE_CNF, appname, request_id, resp_pbupdate->result,
					0,NULL);
			break;

		case TRESP_PHONEBOOK_DELETERECORD:
			dbg("dbus comm - TRESP_PHONEBOOK_DELETERECORD");
			return ts_delivery_event(ctx->EvtDeliveryHandle, TAPI_EVENT_CLASS_SIM,
					TAPI_EVENT_SIM_PB_DELETE_CNF, appname, request_id, resp_pbdelete->result,
					0,NULL);
			break;

		case TRESP_SAP_REQ_CONNECT:
			lsa_conn.ConnectionStatus = sap_conn->status;
			lsa_conn.MaxMsgSize = sap_conn->max_msg_size;
			lsa_conn.MsgId = TAPI_SIM_SAP_CONNECT_RESP;
			return ts_delivery_event(ctx->EvtDeliveryHandle, TAPI_EVENT_CLASS_SIM,
					TAPI_EVENT_SIM_SAP_CONNECT_CNF, appname, request_id, TAPI_SIM_ACCESS_SUCCESS,
					sizeof(TelSimSapConnect_t), (void*) &lsa_conn);
			break;

		case TRESP_SAP_REQ_DISCONNECT:
			lsa_disconn.MsgId = TAPI_SIM_SAP_DISCONNECT_RESP;
			return ts_delivery_event(ctx->EvtDeliveryHandle, TAPI_EVENT_CLASS_SIM,
					TAPI_EVENT_SIM_SAP_CONNECT_CNF, appname, request_id, TAPI_SIM_ACCESS_SUCCESS,
					sizeof(TelSimSapConnect_t), (void*) &lsa_disconn);
			break;

		case TRESP_SAP_REQ_STATUS:
			lsa_status = sap_status->status;
			return ts_delivery_event(ctx->EvtDeliveryHandle, TAPI_EVENT_CLASS_SIM,
					TAPI_EVENT_SIM_SAP_CONNECT_STATUS_CNF, appname, request_id,
					TAPI_SIM_ACCESS_SUCCESS, sizeof(unsigned int), (void*) &lsa_status);
			break;

		case TRESP_SAP_REQ_ATR:
			lsa_atr.AtrResult = sap_atr->result;
			lsa_atr.AtrLength = sap_atr->atr_length;
			memcpy(lsa_atr.AtrData, sap_atr->atr, lsa_atr.AtrLength);
			return ts_delivery_event(ctx->EvtDeliveryHandle, TAPI_EVENT_CLASS_SIM,
					TAPI_EVENT_SIM_SAP_TRANSFER_ATR_CNF, appname, request_id,
					TAPI_SIM_ACCESS_SUCCESS, sizeof(TelSimSapAtrInfo_t), (void*) &lsa_atr);
			break;

		case TRESP_SAP_TRANSFER_APDU:
			lsa_apdu.ApduLength = sap_apdu->resp_apdu_length;
			memcpy(lsa_apdu.Apdu, sap_apdu->resp_adpdu, lsa_apdu.ApduLength);
			return ts_delivery_event(ctx->EvtDeliveryHandle, TAPI_EVENT_CLASS_SIM,
					TAPI_EVENT_SIM_SAP_TRANSFER_APDU_CNF, appname, request_id,
					TAPI_SIM_ACCESS_SUCCESS, sizeof(TelSimSapApduData_t), (void*) &lsa_apdu);
			break;

		case TRESP_SAP_SET_PROTOCOL:
			lsa_result_code = sap_protocol->result;
			return ts_delivery_event(ctx->EvtDeliveryHandle, TAPI_EVENT_CLASS_SIM,
					TAPI_EVENT_SIM_SAP_SET_PROTOCOL_CNF, appname, request_id,
					TAPI_SIM_ACCESS_SUCCESS, sizeof(TelSimSapResultCode_t),
					(void*) &lsa_result_code);
			break;

		case TRESP_SAP_SET_POWER:
			lsa_power.SimPowerResult = sap_power->result;
			lsa_power.MsgId = 0; //NOT SUPPORTED FROM NOW
			return ts_delivery_event(ctx->EvtDeliveryHandle, TAPI_EVENT_CLASS_SIM,
					TAPI_EVENT_SIM_SAP_SET_SIM_POWER_CNF, appname, request_id,
					TAPI_SIM_ACCESS_SUCCESS, sizeof(TelSimSapPower_t), (void*) &lsa_power);
			break;

		case TRESP_SAP_REQ_CARDREADERSTATUS:
			lsa_reader.CardReaderResult = sap_reader->result;
			lsa_reader.CardReaderStatus = sap_reader->reader_status;
			return ts_delivery_event(ctx->EvtDeliveryHandle, TAPI_EVENT_CLASS_SIM,
					TAPI_EVENT_SIM_SAP_CARD_READER_STATUS_CNF, appname, request_id,
					TAPI_SIM_ACCESS_SUCCESS, sizeof(TelSimCardReaderStatus_t), (void*) &lsa_reader);
			break;

		default:
			break;
	}
	return TRUE;
}

TReturn dbus_notification_sim(struct custom_data *ctx, CoreObject *source, enum tcore_notification_command command,
		unsigned int data_len, const void *data)
{
	const struct tnoti_sap_status_changed *n_sap_status = data;
	const struct tnoti_sap_disconnect *n_sap_disconn = data;
	const struct tnoti_sim_status *n_sim_status = data;

	switch (command) {
		case TNOTI_SIM_STATUS:
			dbg("notified sim_status[%d]", n_sim_status->sim_status);
			dbus_sim_data_request(ctx, source,n_sim_status->sim_status);
			if(n_sim_status->sim_status == SIM_STATUS_INIT_COMPLETED)
				dbus_sim_security_request(ctx, source);
			return ts_delivery_event(ctx->EvtDeliveryHandle, TAPI_EVENT_CLASS_SIM, TAPI_EVENT_SIM_STATUS_IND,
					NULL, TAPI_SIM_REQUEST_ID_FOR_NOTI, n_sim_status->sim_status, 0, NULL);
			break;

		case TNOTI_SAP_STATUS:
			return ts_delivery_event(ctx->EvtDeliveryHandle, TAPI_EVENT_CLASS_SIM, TAPI_EVENT_SIM_SAP_CONNECT_STATUS_CNF,
					NULL, TAPI_SIM_REQUEST_ID_FOR_NOTI, TAPI_SIM_ACCESS_SUCCESS, sizeof(enum tel_sap_status_change), (void*)&n_sap_status->status);
			break;

		case TNOTI_SAP_DISCONNECT:
			return ts_delivery_event(ctx->EvtDeliveryHandle, TAPI_EVENT_CLASS_SIM, TAPI_EVENT_SIM_SAP_CONNECT_NOTI,
					NULL, TAPI_SIM_REQUEST_ID_FOR_NOTI, TAPI_SIM_ACCESS_SUCCESS, sizeof(enum tel_sap_disconnect_type), (void*)&n_sap_disconn->type);
			break;

		case TNOTI_PHONEBOOK_STATUS :
			return ts_delivery_event(ctx->EvtDeliveryHandle, TAPI_EVENT_CLASS_SIM, TAPI_EVENT_SIM_FDN_STATUS_IND,
					NULL, TAPI_SIM_REQUEST_ID_FOR_NOTI, TAPI_SIM_PB_SUCCESS, 0, NULL);
			break;

		default :
			break;
	}

	return TRUE;
}

