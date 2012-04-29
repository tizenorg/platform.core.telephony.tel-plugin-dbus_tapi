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
#include <TelSs.h>

#include "tel_cs_conn.h"
#include "common.h"
#include "ts_utility.h"
#include "ts_common.h"
#include "ts_svr_req.h"
#include "ts_noti.h"
#include "modules.h"

static enum telephony_ss_class _convert_call_type_to_ss_class( TelSsCallType_t call_type )
{
	switch ( call_type ) {
	case TAPI_CALL_TYPE_VOICE_EV :				/**< Voice Call */
		return SS_CLASS_VOICE;

	case TAPI_CALL_TYPE_DATA_EV :				/**< Data Call */
		return SS_CLASS_ALL_DATA_TELE;

	case TAPI_CALL_TYPE_FAX_EV :					/**< FAX   Call */
		return SS_CLASS_FAX;

	case TAPI_CALL_TYPE_SHORT_MESS_EV :		/**< Short Message */
		return SS_CLASS_SMS;

	case TAPI_CALL_TYPE_ALL_ASYNC_EV:	/**< All Async services */
		return SS_CLASS_ALL_ASYNC;

	case TAPI_CALL_TYPE_ALL_SYNC_EV:	/**< All sync services */
		return SS_CLASS_ALL_SYNC;

	case TAPI_CALL_TYPE_DATA_CIRCUIT_SYNC_EV:	/**< Data Circuit Sync */
		break;

	case TAPI_CALL_TYPE_ALL_TELE_BEARER:	/**< all tele and bearer services */
		return SS_CLASS_ALL_TELE_BEARER;

	case TAPI_CALL_TYPE_ALL_TELE:		/**< All tele services */
		return SS_CLASS_ALL_TELE;

	default:
		break;
	}

	return SS_CLASS_NONE;
}

static enum telephony_ss_class _convert_cf_type_to_ss_class( TelSsForwardType_t cf_type )
{
	switch ( cf_type ) {
	case TAPI_CS_FORWARD_TYPE_VOICE_EV:			/**< Voice call forward */
		return SS_CLASS_VOICE;

	case TAPI_CS_FORWARD_TYPE_ALL_ASYNC_EV:          /**< All Async services */
		return SS_CLASS_ALL_ASYNC;

	case TAPI_CS_FORWARD_TYPE_ALL_SYNC_EV:			/**< All sync services */
		return SS_CLASS_ALL_SYNC;

	case TAPI_CS_FORWARD_TYPE_DATA_EV:				/**< Data call forward */
		return SS_CLASS_ALL_DATA_TELE;

	case TAPI_CS_FORWARD_TYPE_FAX_EV:				/**< Fax call forward */
		return SS_CLASS_FAX;

    case TAPI_CS_FORWARD_TYPE_ALL_TELE_BEARER:	/**< All tele and bearer services */
		return SS_CLASS_ALL_TELE_BEARER;

    case TAPI_CS_FORWARD_TYPE_ALL_TELE:		/**< All tele services */
		return SS_CLASS_ALL_TELE;

    case TAPI_CS_FORWARD_TYPE_AUX_VOICE:		/**< AUX Voice */
		return SS_CLASS_AUX_VOICE ;

	default:
		break;
	}

	return SS_CLASS_NONE;
}

static enum telephony_ss_barring_mode _convert_ss_barring_type(TelSsCallBarType_t type)
{
	enum telephony_ss_barring_mode mode = 0;

	dbg("Before conversion - Barring flavor: [%x] ", type);

	switch ( type ) {
		case TAPI_CALL_BARRING_ALL:
			mode = SS_BARR_MODE_AB;
			break;

		case TAPI_CALL_BARRING_OUTGOING:
			mode = SS_BARR_MODE_AOB;
			break;

		case TAPI_CALL_BARRING_ALL_OUTGOING_CALLS:
			mode = SS_BARR_MODE_BAOC;
			break;

		case TAPI_CALL_BARRING_ALL_OUTGOING_INTERN_CALL:
			mode = SS_BARR_MODE_BOIC;
			break;

		case TAPI_CALL_BARRING_ALL_OUTGOING_INTERN_CALL_EXCEPT:
			mode = SS_BARR_MODE_BOIC_NOT_HC;
			break;

		case TAPI_CALL_BARRING_INCOMING:
			mode = SS_BARR_MODE_AIB;
			break;

		case TAPI_CALL_BARRING_ALL_INCOMING_CALLS:
			mode = SS_BARR_MODE_BAIC;
			break;

		case TAPI_CALL_BARRING_ALL_INCOMING_CALLS_ROAMING:
			mode = SS_BARR_MODE_BIC_ROAM;
			break;

		case TAPI_CALL_BARRING_ALL_INCOMING_CALLS_INSIM:
			mode = SS_BARR_MODE_BIC_NOT_SIM;
			break;

		default:
			dbg("  DEFAULT ERR Requested flavor not supported ");
			break;
	}
	dbg("Helper Func Return Converted Barring Type- [%x]", mode);
	return mode;
}
void dbus_request_ss(struct custom_data *ctx, TcorePlugin *plugin, int tapi_service_function, GArray* in_param1,
		GArray* in_param2, GArray* in_param3, GArray* in_param4, GArray** out_param1, GArray** out_param2,
		GArray** out_param3, GArray** out_param4, GError** error)
{
	int					api_err = TAPI_API_SUCCESS;
	int					req_id = 0xff;

	CoreObject*			o = NULL;
	UserRequest*		ur = NULL;
	struct tcore_user_info	ui = { 0, };

	tapi_dbus_connection_name conn_name;
	GSList*				co_list = 0;

	TReturn				ret = 0;


	conn_name = g_array_index(in_param4, tapi_dbus_connection_name, 0);

	co_list = tcore_plugin_get_core_objects_bytype(plugin, CORE_OBJECT_TYPE_SS);
	if (!co_list) {
		api_err = TAPI_API_NOT_SUPPORTED;
		goto OUT;
	}

	o = (CoreObject *)co_list->data;
	g_slist_free(co_list);

	if (!o) {
		api_err = TAPI_API_NOT_SUPPORTED;
		goto OUT;
	}

	ur = tcore_user_request_new( ctx->comm, tcore_plugin_get_description(plugin)->name);
	if (!ur) {
		api_err = TAPI_API_SERVER_FAILURE;
		goto OUT;
	}

	ui.appname = conn_name.name;
	tcore_user_request_set_user_info(ur, &ui);

	switch (tapi_service_function) {

		case TAPI_CS_SS_BARRSETREQ: {
			struct treq_ss_barring data;
			TelSsCallBarringInfo_t *barringInfo = 0;
			barringInfo = &g_array_index( in_param1, TelSsCallBarringInfo_t, 0 );

			dbg("barring type : [0x%x]", barringInfo->Type);
			dbg("teleservice type : [0x%x]", barringInfo->CallType);
			dbg("barring mode : [0x%x]", barringInfo->Mode);

			data.class = _convert_call_type_to_ss_class( barringInfo->CallType );
			data.mode = _convert_ss_barring_type( barringInfo->Type );

			memcpy( data.password, barringInfo->szPassword, MAX_SS_BARRING_PASSWORD_LEN );

			tcore_user_request_set_data(ur, sizeof( struct treq_ss_barring ), &data);

			if ( barringInfo->Mode == TAPI_SS_CALL_BAR_ACTIVATE ) {

				dbg("ss barring activate");
				tcore_user_request_set_command( ur, TREQ_SS_BARRING_ACTIVATE );

			} else if ( barringInfo->Mode == TAPI_SS_CALL_BAR_DEACTIVATE ) {

				dbg("ss barring deactivate");
				tcore_user_request_set_command( ur, TREQ_SS_BARRING_DEACTIVATE );

			} else {

				dbg("[ error ] : wrong barring mode");
				break;
			}

			ret = tcore_communicator_dispatch_request(ctx->comm, ur);
			if (ret != TCORE_RETURN_SUCCESS) {
				api_err = TAPI_API_OPERATION_FAILED;
			}


		} break;

		case TAPI_CS_SS_BARRCHANGEPWDREQ: {

			char *temp = 0;
			struct treq_ss_barring_change_password data;

			dbg("SS service Type : TAPI_CS_SS_BARRCHANGEPWDREQ");

			/* Extract the call barring old password information from in_param1 */
			temp = &g_array_index(in_param1, char, 0);
			memcpy(data.password_old, temp, TAPI_SS_GSM_BARR_PW_LEN_MAX);

			/* Extract the call barring new password information from in_param2 */
			temp = &g_array_index(in_param2, char, 0);
			memcpy(data.password_new, temp, TAPI_SS_GSM_BARR_PW_LEN_MAX);

			/* Extract the call barring new password information from in_param2 */
			temp = &g_array_index(in_param3, char, 0);
			memcpy(data.password_confirm, temp, TAPI_SS_GSM_BARR_PW_LEN_MAX);

			dbg("Old Password :[%s]", data.password_old);
			dbg("New Password :[%s]", data.password_new);
			dbg("New Password Again :[%s]", data.password_confirm);

			tcore_user_request_set_data(ur, sizeof( struct treq_ss_barring_change_password ), &data);
			tcore_user_request_set_command( ur, TREQ_SS_BARRING_CHANGE_PASSWORD );

			ret = tcore_communicator_dispatch_request(ctx->comm, ur);
			if (ret != TCORE_RETURN_SUCCESS) {
				api_err = TAPI_API_OPERATION_FAILED;
			}

		} break;

		case TAPI_CS_SS_BARRQUERYSTATUSREQ: {

			struct treq_ss_barring data;
			TelSsCallType_t call_type = 0;
			TelSsCallBarType_t bar_type = 0;

			dbg("SS service Type : TAPI_CS_SS_BARRQUERYSTATUSREQ");

			/* Extract the call barring flavor  information from in_param1 */
			bar_type = g_array_index(in_param1, TelSsCallBarType_t, 0);
			data.mode = _convert_ss_barring_type( bar_type );

			/* Extract the tele service information from in_param2 */
			call_type = g_array_index(in_param2, TelSsCallType_t, 0);
			data.class = _convert_call_type_to_ss_class( call_type );

			dbg("Barring Type :[%x]", data.mode);
			dbg("Teleservice Type :[%x]", data.class);

			tcore_user_request_set_data(ur, sizeof( struct treq_ss_barring ), &data);
			tcore_user_request_set_command( ur, TREQ_SS_BARRING_GET_STATUS );

			ret = tcore_communicator_dispatch_request(ctx->comm, ur);
			if (ret != TCORE_RETURN_SUCCESS) {
				api_err = TAPI_API_OPERATION_FAILED;
			}

		} break;

		case TAPI_CS_SS_FWDSETREQ: {

			struct treq_ss_forwarding data;
			TelSsForwardInfo_t * forwardInfo = NULL;

			dbg("SS service Type :TAPI_CS_SS_FWDSETREQ");

			/* Extract the call barring request information from in_param1 */
			forwardInfo = &g_array_index(in_param1, TelSsForwardInfo_t, 0);

			dbg("Forwarding mode :[%x]", forwardInfo->Mode);
			dbg("Forwarding condition :[%x]", forwardInfo->Condition);
			dbg("Forwarding Teleservice :[%x]", forwardInfo->Type);
			dbg("Forwarding Number :[%s]", forwardInfo->szPhoneNumber);

			data.class = _convert_cf_type_to_ss_class(forwardInfo->Type);
			data.mode = (forwardInfo->Condition + 1);
			data.time = forwardInfo->NoReplyConditionTimer;
			memcpy(data.number, forwardInfo->szPhoneNumber, MAX_SS_FORWARDING_NUMBER_LEN);

			tcore_user_request_set_data(ur, sizeof( struct treq_ss_forwarding ), &data);

			switch ( forwardInfo->Mode ) {
				case 0: // disable
					tcore_user_request_set_command( ur, TREQ_SS_FORWARDING_DEACTIVATE );
					break;
				case 1: // enable
					tcore_user_request_set_command( ur, TREQ_SS_FORWARDING_ACTIVATE );		
					break;
				case 2: // register
					tcore_user_request_set_command( ur, TREQ_SS_FORWARDING_REGISTER );
					break;
				case 3: // erasure
					tcore_user_request_set_command( ur, TREQ_SS_FORWARDING_DEREGISTER );
					break;
				default:
					dbg("[ error ] unknown forwarding mode");
					goto OUT;
					break;
			}

			ret = tcore_communicator_dispatch_request(ctx->comm, ur);
			if (ret != TCORE_RETURN_SUCCESS) {
				api_err = TAPI_API_OPERATION_FAILED;
			}

		} break;

		case TAPI_CS_SS_FWDQUERYSTATUSREQ: {

			struct treq_ss_forwarding data;

			dbg("SS service Type : TAPI_CS_SS_FWDQUERYSTATUSREQ");

			/* Extract the teleservice request information from in_param1 */
			data.class = _convert_cf_type_to_ss_class( g_array_index(in_param1, TelSsForwardType_t, 0) );
			/* Extract the forwarding flavor request information from in_param2 */
			data.mode = g_array_index(in_param2, TelSsForwardWhen_t, 0);
			data.mode++; // to convert tcore mode enum value

			dbg("Forwarding Type :[%x]", data.mode);
			dbg("Teleservice Type :[%x]", data.class);

			tcore_user_request_set_data(ur, sizeof( struct treq_ss_forwarding ), &data);
			tcore_user_request_set_command( ur, TREQ_SS_FORWARDING_GET_STATUS );

			ret = tcore_communicator_dispatch_request(ctx->comm, ur);
			if (ret != TCORE_RETURN_SUCCESS) {
				api_err = TAPI_API_OPERATION_FAILED;
			}

		} break;

		case TAPI_CS_SS_WAITSETREQ: {

			struct treq_ss_waiting data;
			TelSsWaitingInfo_t *CwInfo = NULL;

			dbg(" SS service Type : TAPI_CS_SS_WAITSETREQ");

			CwInfo = &g_array_index(in_param1, TelSsWaitingInfo_t, 0);

			dbg("Teleservice Type :[%x]", CwInfo->CallType);

			data.class = _convert_call_type_to_ss_class(CwInfo->CallType);

			tcore_user_request_set_data(ur, sizeof( struct treq_ss_waiting ), &data);

			if ( CwInfo->Mode == 0 ) {
				tcore_user_request_set_command( ur, TREQ_SS_WAITING_ACTIVATE );
			} else if ( CwInfo->Mode == 1 )
				tcore_user_request_set_command( ur, TREQ_SS_WAITING_DEACTIVATE );
			else {
				dbg("[ error ] unknown waiting mode : [ %d ]", CwInfo->Mode );
				goto OUT;
			}

			ret = tcore_communicator_dispatch_request(ctx->comm, ur);
			if (ret != TCORE_RETURN_SUCCESS) {
				api_err = TAPI_API_OPERATION_FAILED;
			}

		} break;

		case TAPI_CS_SS_WAITQUERYSTATUSREQ: {

			struct treq_ss_waiting data;

			dbg("SS service Type : TAPI_CS_SS_WAITQUERYSTATUSREQ");

			/* Extract the teleservice request information from in_param1 */
			data.class = _convert_call_type_to_ss_class( g_array_index(in_param1, TelSsCallType_t, 0) );

			dbg("Teleservice Type :[%x]", data.class);

			tcore_user_request_set_data(ur, sizeof( struct treq_ss_waiting ), &data);
			tcore_user_request_set_command( ur, TREQ_SS_WAITING_GET_STATUS );

			ret = tcore_communicator_dispatch_request(ctx->comm, ur);
			if (ret != TCORE_RETURN_SUCCESS) {
				api_err = TAPI_API_OPERATION_FAILED;
			}


		} break;

		case TAPI_CS_SS_CLIQUERYSTATUSREQ: {
			struct treq_ss_cli data;

			dbg("SS service Type : TAPI_CS_SS_CLIQUERYSTATUSREQ");

			/* Extract the Cli service type request information from in_param1 */
			data.type = g_array_index(in_param1, TelSsCliType_t, 0);

			dbg("Cli service Type :[%x]", data.type);

			tcore_user_request_set_data(ur, sizeof( struct treq_ss_cli ), &data);
			tcore_user_request_set_command( ur, TREQ_SS_CLI_GET_STATUS );

			ret = tcore_communicator_dispatch_request(ctx->comm, ur);
			if (ret != TCORE_RETURN_SUCCESS) {
				api_err = TAPI_API_OPERATION_FAILED;
			}

		} break;

		case TAPI_CS_SS_USSDSENDSTRINGREQ: {
			TelSsUssdMsgInfo_t *ussd_msg = NULL;
			struct treq_ss_ussd data;

			dbg("SS service Type : TAPI_CS_SS_USSDSENDSTRINGREQ");

			ussd_msg = &g_array_index(in_param1, TelSsUssdMsgInfo_t, 0);

			data.type = SS_USSD_TYPE_USER_INITIATED;

			memset(data.str, '\0', MAX_SS_USSD_LEN);
			memcpy(data.str, ussd_msg->szUssdString, MAX_SS_USSD_LEN);

			tcore_user_request_set_data(ur, sizeof( struct treq_ss_ussd ), &data);
			tcore_user_request_set_command( ur, TREQ_SS_SEND_USSD );

			ret = tcore_communicator_dispatch_request(ctx->comm, ur);
			if (ret != TCORE_RETURN_SUCCESS) {
				api_err = TAPI_API_OPERATION_FAILED;
			}

		} break;

		case TAPI_CS_SS_USSDRSP: {
			TelSsUssdMsgInfo_t *ussd_rsp = { 0, };
			struct treq_ss_ussd data;

			ussd_rsp = &g_array_index(in_param1, TelSsUssdMsgInfo_t, 0);

			dbg("Response USSD string: USSD String:[%s]", ussd_rsp->szUssdString);

			data.type = SS_USSD_TYPE_USER_RES;

			memset(data.str, '\0', MAX_SS_USSD_LEN);
			memcpy(data.str, ussd_rsp->szUssdString, MAX_SS_USSD_LEN);

			tcore_user_request_set_data(ur, sizeof( struct treq_ss_ussd ), &data);
			tcore_user_request_set_command( ur, TREQ_SS_SEND_USSD );

			ret = tcore_communicator_dispatch_request(ctx->comm, ur);
			if (ret != TCORE_RETURN_SUCCESS) {
				api_err = TAPI_API_OPERATION_FAILED;
			}

		} break;

		case TAPI_CS_SS_USSDCANCEL: {

			struct treq_ss_ussd data;

			data.type = SS_USSD_TYPE_USER_RELEASE;

			tcore_user_request_set_data(ur, sizeof( struct treq_ss_ussd ), &data);
			tcore_user_request_set_command( ur, TREQ_SS_SEND_USSD );

			ret = tcore_communicator_dispatch_request(ctx->comm, ur);
			if (ret != TCORE_RETURN_SUCCESS) {
				api_err = TAPI_API_OPERATION_FAILED;
			}

		} break;

		case TAPI_CS_SS_GETCALLCOSTREQ: {
			TelSsAocType_t aoc_type;

			dbg(" SS service Type : TAPI_CS_SS_GETCALLCOSTREQ");

			/* Extract the AOC type request information from in_param1 */
			aoc_type = g_array_index(in_param1, TelSsAocType_t, 0);

		} break;

		case TAPI_CS_SS_SETCALLCOSTREQ: {
			TelCallAocInfo_t aoc_set_info = { 0, };

			dbg("SS service Type : TAPI_CS_SS_SETCALLCOSTREQ");

			/* Extract the Aoc type request information from in_param1 */
			aoc_set_info = g_array_index(in_param1, TelCallAocInfo_t, 0);

		} break;

		default:
			api_err = TAPI_API_NOT_SUPPORTED;
			break;
	}

OUT:
	g_array_append_vals(*out_param1, &api_err, sizeof(int));
	g_array_append_vals(*out_param2, &req_id, sizeof(int));
}

static TelSsTeleService_t _convert_ss_class( enum telephony_ss_class class )
{
	TelSsTeleService_t service = TAPI_SS_TS_NONE;

	dbg("Helper Func Entrance ");

	switch ( class ) {
		case SS_CLASS_NONE: /* 0x00 *///NEED TO CHECK
			service = TAPI_SS_TS_NONE;
			break;

			/* TELESERVICE */
		case SS_CLASS_ALL_TELE: /* 0x10 : All Teleservices */
			service = TAPI_SS_TS_ALL_TELESERVICES;
			break;

		case SS_CLASS_VOICE: /* 0x11 : All Voice ( telephony ) */
			service = TAPI_SS_TS_ALL_SPEECH;
			break;

		case SS_CLASS_ALL_DATA_TELE: /* 0x12 : All Data Teleservices */
			service = TAPI_SS_TS_ALL_DATA_TELESERVICES;
			break;

		case SS_CLASS_FAX: /* 0x13 : All Fax Service */
			service = TAPI_SS_TS_FACS;
			break;

		case SS_CLASS_SMS: /* 0x16 : SMS service	 */
			service = TAPI_SS_TS_SMS;
			break;

		case SS_CLASS_ALL_TELE_EXPT_SMS: /* 0x19 : All teleservice except SMS */
			service = TAPI_SS_TS_ALL_TELESERVICES_EXCEPT_SMS;
			break;

			/* BEARER SERVICE */
		case SS_CLASS_ALL_BEARER: /* 0X20 : all bearer services */
			service = TAPI_SS_BS_ALL_BEARER_SERVICES;
			break;

		case SS_CLASS_ALL_ASYNC: /* 0x21 : All Async services */
			service = TAPI_SS_BS_ALL_ASYNCHRONOUS;
			break;

		case SS_CLASS_ALL_SYNC: /* 0x22 : All sync services*/
			service = TAPI_SS_BS_ALL_SYNCHRONOUS;
			break;

		case SS_CLASS_ALL_CS_SYNC: /* 0x24 : All Circuit switched sync */
			service = TAPI_SS_BS_DATA_CIRCUIT_SYNC;
			break;

		case SS_CLASS_ALL_CS_ASYNC: /*  0x25 : All Circuit switched async */
			service = TAPI_SS_BS_DATA_CIRCUIT_ASYNC;
			break;

		case SS_CLASS_ALL_DEDI_PS: /*  0x26 : All Dedicated packet Access */
			service = TAPI_SS_BS_DATA_ALL_PDS;
			break;

		case SS_CLASS_ALL_DEDI_PAD: /*  0x27 : All Dedicated PAD Access */
			service = TAPI_SS_BS_ALL_DATA_PADCA;
			break;

		case SS_CLASS_ALL_DATA_CDA: /*  0x28 : All Data CDA*/
			service = TAPI_SS_BS_ALL_DATA_CDA;
			break;

			/* PLMN SPECIFIC TELESERVICE */
		case SS_CLASS_PLMN_TELE_ALL: /*0x50 : PLMN specific teleservices*/
			service = TAPI_SS_BS_ALL_PLMN_SPEC_BS;
			break;

		case SS_CLASS_PLMN_TELE_1: /*0x51 :PLMN specific teleservice 1*/
			service = TAPI_SS_BS_PLMN_SPEC_TELE_1;
			break;

		case SS_CLASS_PLMN_TELE_2: /*0x52 : PLMN specific teleservice 2*/
			service = TAPI_SS_BS_PLMN_SPEC_TELE_2;
			break;

		case SS_CLASS_PLMN_TELE_3: /*0x53 : PLMN specific teleservice 3*/
			service = TAPI_SS_BS_PLMN_SPEC_TELE_3;
			break;

		case SS_CLASS_PLMN_TELE_4: /*0x54 : PLMN specific teleservice 4*/
			service = TAPI_SS_BS_PLMN_SPEC_TELE_4;
			break;

		case SS_CLASS_PLMN_TELE_5: /*0x55 : PLMN specific teleservice 5*/
			service = TAPI_SS_BS_PLMN_SPEC_TELE_5;
			break;

		case SS_CLASS_PLMN_TELE_6: /*0x56 : PLMN specific teleservice 6*/
			service = TAPI_SS_BS_PLMN_SPEC_TELE_6;
			break;

		case SS_CLASS_PLMN_TELE_7: /*0x57 : PLMN specific teleservice 7*/
			service = TAPI_SS_BS_PLMN_SPEC_TELE_7;
			break;

		case SS_CLASS_PLMN_TELE_8: /*0x58 : PLMN specific teleservice 8*/
			service = TAPI_SS_BS_PLMN_SPEC_TELE_8;
			break;

		case SS_CLASS_PLMN_TELE_9: /*0x59 : PLMN specific teleservice 9*/
			service = TAPI_SS_BS_PLMN_SPEC_TELE_9;
			break;

		case SS_CLASS_PLMN_TELE_A: /*0x60 :PLMN specific teleservice 10*/
			service = TAPI_SS_BS_PLMN_SPEC_TELE_A;
			break;

		case SS_CLASS_PLMN_TELE_B: /*0x61 :PLMN specific teleservice 11*/
			service = TAPI_SS_BS_PLMN_SPEC_TELE_B;
			break;

		case SS_CLASS_PLMN_TELE_C: /*0x62 : PLMN specific teleservice 12*/
			service = TAPI_SS_BS_PLMN_SPEC_TELE_C;
			break;

		case SS_CLASS_PLMN_TELE_D: /*0x63 : PLMN specific teleservice 13*/
			service = TAPI_SS_BS_PLMN_SPEC_TELE_D;
			break;

		case SS_CLASS_PLMN_TELE_E: /*0x64 : PLMN specific teleservice 14*/
			service = TAPI_SS_BS_PLMN_SPEC_TELE_E;
			break;

		case SS_CLASS_PLMN_TELE_F: /*0x65 : PLMN specific teleservice 15*/
			service = TAPI_SS_BS_PLMN_SPEC_TELE_F;
			break;
			/* PLMN SPECIFIC BEARER SERVICE */
		case SS_CLASS_PLMN_BEAR_ALL: /*0x70 : All PLMN specific bearer services*/
			service = TAPI_SS_BS_ALL_BEARER_SERVICES;
			break;

		case SS_CLASS_PLMN_BEAR_1: /*0x71 :PLMN specific bearer service 1*/
			service = TAPI_SS_BS_PLMN_SPEC_TELE_1;
			break;

		case SS_CLASS_PLMN_BEAR_2: /*0x72 : PLMN specific bearer service  2*/
			service = TAPI_SS_BS_PLMN_SPEC_TELE_2;
			break;

		case SS_CLASS_PLMN_BEAR_3: /*0x73 : PLMN specific bearer service  3*/
			service = TAPI_SS_BS_PLMN_SPEC_TELE_3;
			break;

		case SS_CLASS_PLMN_BEAR_4: /*0x74 : PLMN specific bearer service  4*/
			service = TAPI_SS_BS_PLMN_SPEC_TELE_4;
			break;

		case SS_CLASS_PLMN_BEAR_5: /*0x75 : PLMN specific bearer service  5*/
			service = TAPI_SS_BS_PLMN_SPEC_TELE_5;
			break;

		case SS_CLASS_PLMN_BEAR_6: /*0x76 : PLMN specific bearer service  6*/
			service = TAPI_SS_BS_PLMN_SPEC_TELE_6;
			break;

		case SS_CLASS_PLMN_BEAR_7: /*0x77 : PLMN specific bearer service  7*/
			service = TAPI_SS_BS_PLMN_SPEC_TELE_7;
			break;

		case SS_CLASS_PLMN_BEAR_8: /*0x78 : PLMN specific bearer service  8*/
			service = TAPI_SS_BS_PLMN_SPEC_TELE_8;
			break;

		case SS_CLASS_PLMN_BEAR_9: /*0x79 : PLMN specific bearer service  9*/
			service = TAPI_SS_BS_PLMN_SPEC_TELE_9;
			break;

		case SS_CLASS_PLMN_BEAR_A: /*0x80 : PLMN specific bearer service  10*/
			service = TAPI_SS_BS_PLMN_SPEC_TELE_A;
			break;

		case SS_CLASS_PLMN_BEAR_B: /*0x81 : PLMN specific bearer service  11*/
			service = TAPI_SS_BS_PLMN_SPEC_TELE_B;
			break;

		case SS_CLASS_PLMN_BEAR_C: /*0x82 : PLMN specific bearer service  12*/
			service = TAPI_SS_BS_PLMN_SPEC_TELE_C;
			break;

		case SS_CLASS_PLMN_BEAR_D: /*0x83 : PLMN specific bearer service  13*/
			service = TAPI_SS_BS_PLMN_SPEC_TELE_D;
			break;

		case SS_CLASS_PLMN_BEAR_E: /*0x84 : PLMN specific bearer service  14*/
			service = TAPI_SS_BS_PLMN_SPEC_TELE_E;
			break;

		case SS_CLASS_PLMN_BEAR_F: /*0x85 : PLMN specific bearer service  15*/
			service = TAPI_SS_BS_PLMN_SPEC_TELE_F;
			break;

			/* CPHS - AUXILIARY SERVICE */
		case SS_CLASS_AUX_VOICE: /* 0x89 : All Auxiliary Voice ( Auxiliary telephony ) */
			service = TAPI_SS_TS_AUX_VOICE;
			break;

		case SS_CLASS_ALL_TELE_BEARER: /* 0xFF : all tele and bearer services */
			service = TAPI_SS_ALL_TELE_BEARER;
			break;

		default:
			service = TAPI_SS_TS_NONE;
			dbg("  DEFAULT - TAPI_SS_TS_NONE");
			break;
	}

	dbg(" Converted Teleservice - [%x] ", service);
	return service;
}

static TelSsCallBarType_t _convert_ss_barring_mode( enum telephony_ss_barring_mode mode )
{
	TelSsCallBarType_t flavor = 0;

	dbg("Before conversion  [%x] ", mode);

	switch ( mode ) {
		case SS_BARR_MODE_AB:
			flavor = TAPI_CALL_BARRING_ALL;
			break;

		case SS_BARR_MODE_AOB:
			flavor = TAPI_CALL_BARRING_OUTGOING;
			break;

		case SS_BARR_MODE_BAOC:
			flavor = TAPI_CALL_BARRING_ALL_OUTGOING_CALLS;
			break;

		case SS_BARR_MODE_BOIC:
			flavor = TAPI_CALL_BARRING_ALL_OUTGOING_INTERN_CALL;
			break;

		case SS_BARR_MODE_BOIC_NOT_HC:
			flavor = TAPI_CALL_BARRING_ALL_OUTGOING_INTERN_CALL_EXCEPT;
			break;

		case SS_BARR_MODE_AIB:
			flavor = TAPI_CALL_BARRING_INCOMING;
			break;

		case SS_BARR_MODE_BAIC:
			flavor = TAPI_CALL_BARRING_ALL_INCOMING_CALLS;
			break;

		case SS_BARR_MODE_BIC_ROAM:
			flavor = TAPI_CALL_BARRING_ALL_INCOMING_CALLS_ROAMING;
			break;

		case SS_BARR_MODE_BIC_NOT_SIM:
			flavor = TAPI_CALL_BARRING_ALL_INCOMING_CALLS_INSIM;
			break;

		default:
			dbg("  DEFAULT ERR Requested flavor not supported ");
			break;
	}

	dbg("Helper Func Return Converted Barring flavor- [%x] ", flavor);
	return flavor;
}

TReturn dbus_response_ss(struct custom_data *ctx, UserRequest *ur, const char *appname,
		enum tcore_response_command command, unsigned int data_len, const void *data)
{
	void *ss_data = 0;
	int ss_data_len = 0;

	TelSsInfo_t ss;
	int i =0;

	dbg("command = 0x%x", command);

	switch (command) {
		case TRESP_SS_BARRING_ACTIVATE:
		case TRESP_SS_BARRING_DEACTIVATE: {
			struct tresp_ss_barring* resp_data = 0;
			resp_data = (struct tresp_ss_barring*)data;
			
			if ( !resp_data->err ) {

				ss.SsType = TAPI_SS_TYPE_BARRING;
				ss.NumberOfRecords = resp_data->record_num;

				for ( i=0; i< resp_data->record_num; i++ )  {

					if ( resp_data->record[i].status ==  SS_STATUS_ACTIVATE )
						ss.SsRecord.BarringRecord.rec_class[i].Status = TAPI_SS_STATUS_ACTIVE;
					else
						ss.SsRecord.BarringRecord.rec_class[i].Status = TAPI_SS_STATUS_QUIESCENT;

					ss.SsRecord.BarringRecord.rec_class[i].Flavour = _convert_ss_barring_mode( resp_data->record[i].mode );
					ss.SsRecord.BarringRecord.rec_class[i].TeleCommService = _convert_ss_class( resp_data->record[i].class );

				}

				ss_data = (void*)&ss;
				ss_data_len = sizeof( TelSsInfo_t );

			} 
				
			return ts_delivery_event( ctx->EvtDeliveryHandle, 
					TAPI_EVENT_CLASS_SS, 
					TAPI_EVENT_SS_BARRING_CNF,
					appname,
					0xff,
					resp_data->err, 
					ss_data_len,
					ss_data );

		} break;
		case TRESP_SS_BARRING_CHANGE_PASSWORD: {

			struct tresp_ss_general *resp_data = (struct tresp_ss_general *)data ;

			return ts_delivery_event( ctx->EvtDeliveryHandle, 
					TAPI_EVENT_CLASS_SS, 
					TAPI_EVENT_SS_BARRING_CHANGEPASS_CNF,
					appname,
					0xff,
					resp_data->err, 
					0,
					(void*)0 );
		} break;

		case TRESP_SS_BARRING_GET_STATUS: {
			struct tresp_ss_barring* resp_data = 0;
			resp_data = (struct tresp_ss_barring*)data;
			
			if ( !resp_data->err ) {

				ss.SsType = TAPI_SS_TYPE_BARRING;
				ss.NumberOfRecords = resp_data->record_num;

				for ( i=0; i< resp_data->record_num; i++ )  {

					if ( resp_data->record[i].status ==  SS_STATUS_ACTIVATE )
						ss.SsRecord.BarringRecord.rec_class[i].Status = TAPI_SS_STATUS_ACTIVE;
					else
						ss.SsRecord.BarringRecord.rec_class[i].Status = TAPI_SS_STATUS_QUIESCENT;

					ss.SsRecord.BarringRecord.rec_class[i].Flavour = _convert_ss_barring_mode( resp_data->record[i].mode );
					ss.SsRecord.BarringRecord.rec_class[i].TeleCommService = _convert_ss_class( resp_data->record[i].class );

				}

				ss_data = (void*)&ss;
				ss_data_len = sizeof( TelSsInfo_t );

			} 
				
			return ts_delivery_event( ctx->EvtDeliveryHandle, 
					TAPI_EVENT_CLASS_SS, 
					TAPI_EVENT_SS_BARRING_QUERYSTATUS_CNF,
					appname,
					0xff,
					resp_data->err, 
					ss_data_len,
					ss_data );

		} break;

			break;
		case TRESP_SS_FORWARDING_ACTIVATE:
		case TRESP_SS_FORWARDING_DEACTIVATE:
		case TRESP_SS_FORWARDING_REGISTER:
		case TRESP_SS_FORWARDING_DEREGISTER: {

			struct tresp_ss_forwarding* resp_data = 0;
			resp_data = (struct tresp_ss_forwarding*)data;
			
			if ( !resp_data->err ) {

				ss.SsType = TAPI_SS_TYPE_FORWARDING;
				ss.NumberOfRecords = resp_data->record_num;

				for ( i=0; i< resp_data->record_num; i++ )  {

					if ( resp_data->record[i].status ==  SS_STATUS_ACTIVATE )
						ss.SsRecord.ForwardingRecord.rec_class[i].Status = TAPI_SS_STATUS_ACTIVE;
					else
						ss.SsRecord.ForwardingRecord.rec_class[i].Status = TAPI_SS_STATUS_QUIESCENT;

					ss.SsRecord.ForwardingRecord.rec_class[i].ForwardCondition = (TelSsForwardWhen_t)( resp_data->record[i].mode - 1 );
					ss.SsRecord.ForwardingRecord.rec_class[i].TeleCommService = _convert_ss_class( resp_data->record[i].class );
					ss.SsRecord.ForwardingRecord.rec_class[i].NoReplyWaitTime = resp_data->record[i].time;
					ss.SsRecord.ForwardingRecord.rec_class[i].bCallForwardingNumberPresent = resp_data->record[i].number_present;
					memcpy( ss.SsRecord.ForwardingRecord.rec_class[i].szCallForwardingNumber, 
							resp_data->record[i].number, TAPI_CALL_DIALDIGIT_LEN_MAX );

				}

				ss_data = (void*)&ss;
				ss_data_len = sizeof( TelSsInfo_t );

			} 

			return ts_delivery_event( ctx->EvtDeliveryHandle, 
					TAPI_EVENT_CLASS_SS, 
					TAPI_EVENT_SS_FORWARD_CNF,
					appname,
					0xff,
					resp_data->err, 
					ss_data_len,
					ss_data );

		} break;
		case TRESP_SS_FORWARDING_GET_STATUS: {
			struct tresp_ss_forwarding* resp_data = 0;
			resp_data = (struct tresp_ss_forwarding*)data;
			
			if ( !resp_data->err ) {

				ss.SsType = TAPI_SS_TYPE_FORWARDING;
				ss.NumberOfRecords = resp_data->record_num;

				for ( i=0; i< resp_data->record_num; i++ )  {

					if ( resp_data->record[i].status ==  SS_STATUS_ACTIVATE )
						ss.SsRecord.ForwardingRecord.rec_class[i].Status = TAPI_SS_STATUS_ACTIVE;
					else
						ss.SsRecord.ForwardingRecord.rec_class[i].Status = TAPI_SS_STATUS_QUIESCENT;

					ss.SsRecord.ForwardingRecord.rec_class[i].ForwardCondition = (TelSsForwardWhen_t)( resp_data->record[i].mode - 1 );
					ss.SsRecord.ForwardingRecord.rec_class[i].TeleCommService = _convert_ss_class( resp_data->record[i].class );
					ss.SsRecord.ForwardingRecord.rec_class[i].NoReplyWaitTime = resp_data->record[i].time;
					ss.SsRecord.ForwardingRecord.rec_class[i].bCallForwardingNumberPresent = resp_data->record[i].number_present;
					memcpy( ss.SsRecord.ForwardingRecord.rec_class[i].szCallForwardingNumber, 
							resp_data->record[i].number, TAPI_CALL_DIALDIGIT_LEN_MAX );

				}

				ss_data = (void*)&ss;
				ss_data_len = sizeof( TelSsInfo_t );

			}

			return ts_delivery_event( ctx->EvtDeliveryHandle, 
					TAPI_EVENT_CLASS_SS, 
					TAPI_EVENT_SS_FORWARD_QUERYSTATUS_CNF,
					appname,
					0xff,
					resp_data->err, 
					ss_data_len,
					ss_data );

		} break;										 
		case TRESP_SS_WAITING_ACTIVATE:
		case TRESP_SS_WAITING_DEACTIVATE: {

			struct tresp_ss_waiting* resp_data = 0;
			resp_data = (struct tresp_ss_waiting*)data;
			
			if ( !resp_data->err ) {

				ss.SsType = TAPI_SS_TYPE_WAITING;
				ss.NumberOfRecords = resp_data->record_num;

				for ( i=0; i< resp_data->record_num; i++ )  {

					if ( resp_data->record[i].status ==  SS_STATUS_ACTIVATE )
						ss.SsRecord.WaitingRecord.rec_class[i].Status = TAPI_SS_STATUS_ACTIVE;
					else
						ss.SsRecord.WaitingRecord.rec_class[i].Status = TAPI_SS_STATUS_QUIESCENT;

					ss.SsRecord.WaitingRecord.rec_class[i].TeleCommService = _convert_ss_class( resp_data->record[i].class );

				}

				ss_data = (void*)&ss;
				ss_data_len = sizeof( TelSsInfo_t );

			}

			return ts_delivery_event( ctx->EvtDeliveryHandle, 
					TAPI_EVENT_CLASS_SS, 
					TAPI_EVENT_SS_WAITING_CNF,
					appname,
					0xff,
					resp_data->err, 
					ss_data_len,
					ss_data );

		} break;
		case TRESP_SS_WAITING_GET_STATUS: {
			struct tresp_ss_waiting* resp_data = 0;
			resp_data = (struct tresp_ss_waiting*)data;
			
			if ( !resp_data->err ) {

				ss.SsType = TAPI_SS_TYPE_WAITING;
				ss.NumberOfRecords = resp_data->record_num;

				for ( i=0; i< resp_data->record_num; i++ )  {

					if ( resp_data->record[i].status ==  SS_STATUS_ACTIVATE )
						ss.SsRecord.WaitingRecord.rec_class[i].Status = TAPI_SS_STATUS_ACTIVE;
					else
						ss.SsRecord.WaitingRecord.rec_class[i].Status = TAPI_SS_STATUS_QUIESCENT;

					ss.SsRecord.WaitingRecord.rec_class[i].TeleCommService = _convert_ss_class( resp_data->record[i].class );

				}

				ss_data = (void*)&ss;
				ss_data_len = sizeof( TelSsInfo_t );

			}

			return ts_delivery_event( ctx->EvtDeliveryHandle, 
					TAPI_EVENT_CLASS_SS, 
					TAPI_EVENT_SS_WAITING_QUERYSTATUS_CNF,
					appname,
					0xff,
					resp_data->err, 
					ss_data_len,
					ss_data );
		} break;
		case TRESP_SS_CLI_ACTIVATE:
		case TRESP_SS_CLI_DEACTIVATE:
			break;
		case TRESP_SS_CLI_GET_STATUS: {
			struct tresp_ss_cli *resp_data = 0;

			resp_data = (struct tresp_ss_cli*)data;

			if ( !resp_data->err ) {
				ss.SsRecord.CliRecord.IdentificationType = resp_data->type;

				if ( resp_data->status )
					ss.SsRecord.CliRecord.CliStatus = TAPI_CLI_STATUS_ACTIVATED;
				else
					ss.SsRecord.CliRecord.CliStatus = TAPI_CLI_STATUS_PROVISIONED;

				ss_data = (void*)&ss;
				ss_data_len = sizeof( TelSsInfo_t );
			}

			return ts_delivery_event( ctx->EvtDeliveryHandle, 
					TAPI_EVENT_CLASS_SS, 
					TAPI_EVENT_SS_CLI_QUERYSTATUS_CNF,
					appname,
					0xff,
					resp_data->err, 
					ss_data_len,
					ss_data );

		} break;
		case TRESP_SS_SEND_USSD: {
			struct tresp_ss_ussd* resp_data = 0;
			TelSsUssdMsgIndInfo_t UssdRecord;
			int event;

			resp_data = (struct tresp_ss_ussd*)data;

			if ( !resp_data->err ) {
				UssdRecord.IndType = resp_data->status;
				UssdRecord.UssdInfo.UssdStringLength = strlen(resp_data->str);
				memcpy( UssdRecord.UssdInfo.szUssdString, resp_data->str, UssdRecord.UssdInfo.UssdStringLength );

				ss_data = (void*)&UssdRecord;
				ss_data_len = sizeof(TelSsUssdMsgIndInfo_t);
			}

			if ( resp_data->type == SS_USSD_TYPE_USER_INITIATED )
				event = TAPI_EVENT_SS_USSD_CNF;
			if ( resp_data->type == SS_USSD_TYPE_USER_RES )
				event = TAPI_EVENT_SS_USSD_RSP_CNF;
			if ( resp_data->type == SS_USSD_TYPE_USER_RELEASE )
				event = TAPI_EVENT_SS_USSD_CANCEL_CNF;

			return ts_delivery_event( ctx->EvtDeliveryHandle, 
					TAPI_EVENT_CLASS_SS, 
					event,
					appname,
					0xff,
					resp_data->err, 
					ss_data_len,
					ss_data );


		} break;
		case TRESP_SS_SET_AOC:
		case TRESP_SS_GET_AOC:

		default:
			break;
	}

	return TRUE;
}

TReturn dbus_notification_ss(struct custom_data *ctx, CoreObject *source, enum tcore_notification_command command,
		unsigned int data_len, const void *data)
{
	dbg("command = 0x%x", command);

	switch (command) {
		case TNOTI_SS_USSD: {
			struct tnoti_ss_ussd* noti_data = 0;
			TelSsUssdMsgIndInfo_t UssdRecord;

			noti_data = (struct tnoti_ss_ussd*)data;

			UssdRecord.IndType = noti_data->status;
			UssdRecord.UssdInfo.UssdStringLength = strlen(noti_data->str);
			memcpy( UssdRecord.UssdInfo.szUssdString, noti_data->str, UssdRecord.UssdInfo.UssdStringLength );

			return ts_delivery_event( ctx->EvtDeliveryHandle, 
					TAPI_EVENT_CLASS_SS, 
					TAPI_EVENT_SS_USSD_IND,
					0,
					0xff,
					0, 
					sizeof(TelSsUssdMsgIndInfo_t),
					(void*)&UssdRecord );

		} break;
		default:
			break;
	}

	return TRUE;
}
