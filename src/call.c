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

#include <aul.h>
#include <bundle.h>
#include <tcore.h>
#include <plugin.h>
#include <server.h>
#include <storage.h>
#include <user_request.h>
#include <core_object.h>
#include <communicator.h>

#include <TapiCommon.h>
#include <TelCall.h>
#include <ITapiSound.h>

#include "tel_cs_conn.h"
#include "common.h"
#include "ts_utility.h"
#include "ts_common.h"
#include "ts_svr_req.h"
#include "ts_noti.h"
#include "modules.h"

void dbus_request_call(struct custom_data *ctx, TcorePlugin *plugin, int tapi_service_function, GArray* in_param1,
		GArray* in_param2, GArray* in_param3, GArray* in_param4, GArray** out_param1, GArray** out_param2,
		GArray** out_param3, GArray** out_param4, GError** error)
{
	int					api_err = TAPI_API_SUCCESS;
	int					req_id = 0xff;

	CoreObject*			o = NULL;
	UserRequest*		ur = NULL;
	struct tcore_user_info	ui = { 0, 0, 0, NULL, 0, 0, NULL };

	tapi_dbus_connection_name conn_name;
	GSList*				co_list = 0;

	TReturn				ret = 0;


	conn_name = g_array_index(in_param4, tapi_dbus_connection_name, 0);

	co_list = tcore_plugin_get_core_objects_bytype(plugin, CORE_OBJECT_TYPE_CALL);
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
		case TAPI_CS_CALL_SETUP: {
			int tmp_handle = -1; // this is tmp code for old interface

			TelCallSetupParams_t in;
			struct treq_call_dial data;

			in = g_array_index( in_param1, TelCallSetupParams_t, 0 );

			data.type = in.CallType;
			memcpy(data.number, in.szNumber, TAPI_CALL_DIALDIGIT_LEN_MAX) ;

			tcore_user_request_set_data(ur, sizeof( struct treq_call_dial ), &data);
			tcore_user_request_set_command(ur, TREQ_CALL_DIAL);

			ret = tcore_communicator_dispatch_request(ctx->comm, ur);
			if (ret != TCORE_RETURN_SUCCESS) {
				api_err = TAPI_API_OPERATION_FAILED;
			}

			g_array_append_vals(*out_param3, &tmp_handle, sizeof(int)); // for old interface

			dbg("ret = 0x%x", ret);

		} break;

		case TAPI_CS_CALL_ANSWER: {
			unsigned int handle = 0;
			TelCallAnswerType_t answer = 0;

			struct treq_call_answer data;

			handle = g_array_index( in_param1, unsigned int, 0 );
			answer = g_array_index( in_param2, TelCallAnswerType_t, 0 );

			data.id		= handle;
			data.type	= answer;

			tcore_user_request_set_data(ur, sizeof( struct treq_call_answer ), &data);
			tcore_user_request_set_command(ur, TREQ_CALL_ANSWER);

			ret = tcore_communicator_dispatch_request(ctx->comm, ur);
			if (ret != TCORE_RETURN_SUCCESS) {
				api_err = TAPI_API_OPERATION_FAILED;
			}

			dbg("ret = 0x%x", ret);

		} break;

		case TAPI_CS_CALL_RELEASE: {
			struct treq_call_end data;
			unsigned int id =0;

			id = g_array_index( in_param1, unsigned int, 0 );
			data.id = id;
			data.type = CALL_END_TYPE_DEFAULT;

			tcore_user_request_set_data(ur, sizeof( struct treq_call_end ), &data);
			tcore_user_request_set_command(ur, TREQ_CALL_END);

			ret = tcore_communicator_dispatch_request(ctx->comm, ur);
			if (ret != TCORE_RETURN_SUCCESS) {
				api_err = TAPI_API_OPERATION_FAILED;
			}

			dbg("ret = 0x%x", ret);

		} break;

		case TAPI_CS_CALL_RELEASEALL: {
			struct treq_call_end data;
			data.type = CALL_END_TYPE_ALL;

			tcore_user_request_set_data(ur, sizeof( struct treq_call_end ), &data);
			tcore_user_request_set_command(ur, TREQ_CALL_END);

			ret = tcore_communicator_dispatch_request(ctx->comm, ur);
			if (ret != TCORE_RETURN_SUCCESS) {
				api_err = TAPI_API_OPERATION_FAILED;
			}

			dbg("ret = 0x%x", ret);

		} break;

		case TAPI_CS_CALL_RELEASEALL_ACTIVE: {
			struct treq_call_end data;
			data.type = CALL_END_TYPE_ACTIVE_ALL;

			tcore_user_request_set_data(ur, sizeof( struct treq_call_end ), &data);
			tcore_user_request_set_command(ur, TREQ_CALL_END);

			ret = tcore_communicator_dispatch_request(ctx->comm, ur);
			if (ret != TCORE_RETURN_SUCCESS) {
				api_err = TAPI_API_OPERATION_FAILED;
			}

			dbg("ret = 0x%x", ret);

		} break;

		case TAPI_CS_CALL_RELEASEALL_HELD: {
			struct treq_call_end data;
			data.type = CALL_END_TYPE_HOLD_ALL;

			tcore_user_request_set_data(ur, sizeof( struct treq_call_end ), &data);
			tcore_user_request_set_command(ur, TREQ_CALL_END);

			ret = tcore_communicator_dispatch_request(ctx->comm, ur);
			if (ret != TCORE_RETURN_SUCCESS) {
				api_err = TAPI_API_OPERATION_FAILED;
			}

			dbg("ret = 0x%x", ret);

		} break;

		case TAPI_CS_CALL_DTMF: {
			struct treq_call_dtmf data;

			char *str = 0;
			str = &g_array_index(in_param1, char, 0);
	
			if ( str ) {
				strncpy( data.digits, str, MAX_CALL_DTMF_DIGITS_LEN );
			} else {
				dbg("[ error ] str : (0)");
				goto OUT;
			}

			tcore_user_request_set_data(ur, sizeof( struct treq_call_dtmf ), &data);
			tcore_user_request_set_command(ur, TREQ_CALL_SEND_DTMF);

			ret = tcore_communicator_dispatch_request(ctx->comm, ur);
			if (ret != TCORE_RETURN_SUCCESS) {
				api_err = TAPI_API_OPERATION_FAILED;
			}

			dbg("ret = 0x%x", ret);

		} break;
		case TAPI_CS_CALL_ACTIVATE: {
			unsigned int id = 0;
			id = g_array_index( in_param1, unsigned int, 0 );

			tcore_user_request_set_data(ur, sizeof( unsigned int ), &id);
			tcore_user_request_set_command(ur, TREQ_CALL_ACTIVE);

			ret = tcore_communicator_dispatch_request(ctx->comm, ur);
			if (ret != TCORE_RETURN_SUCCESS) {
				api_err = TAPI_API_OPERATION_FAILED;
			}

			dbg("ret = 0x%x", ret);

		} break;

		case TAPI_CS_CALL_HOLD: {
			unsigned int id = 0;
			id = g_array_index( in_param1, unsigned int, 0 );

			tcore_user_request_set_data(ur, sizeof( unsigned int ), &id);
			tcore_user_request_set_command(ur, TREQ_CALL_HOLD);

			ret = tcore_communicator_dispatch_request(ctx->comm, ur);
			if (ret != TCORE_RETURN_SUCCESS) {
				api_err = TAPI_API_OPERATION_FAILED;
			}

			dbg("ret = 0x%x", ret);

		} break;

		case TAPI_CS_CALL_SWAP: {
			unsigned int id = 0;
			id = g_array_index( in_param1, unsigned int, 0 );

			tcore_user_request_set_data(ur, sizeof( unsigned int ), &id);
			tcore_user_request_set_command(ur, TREQ_CALL_SWAP);

			ret = tcore_communicator_dispatch_request(ctx->comm, ur);
			if (ret != TCORE_RETURN_SUCCESS) {
				api_err = TAPI_API_OPERATION_FAILED;
			}

			dbg("ret = 0x%x", ret);

		} break;

		case TAPI_CS_CALL_SETUPCONFCALL: {
			unsigned int id = 0;
			id = g_array_index( in_param1, unsigned int, 0 );

			tcore_user_request_set_data(ur, sizeof( unsigned int ), &id);
			tcore_user_request_set_command(ur, TREQ_CALL_JOIN);

			ret = tcore_communicator_dispatch_request(ctx->comm, ur);
			if (ret != TCORE_RETURN_SUCCESS) {
				api_err = TAPI_API_OPERATION_FAILED;
			}

			dbg("ret = 0x%x", ret);

		} break;

		case TAPI_CS_CALL_SPLITCONFCALL: {
			int id = 0;
			id = g_array_index(in_param1, unsigned int, 0);

			tcore_user_request_set_data(ur, sizeof( unsigned int ), &id);
			tcore_user_request_set_command(ur, TREQ_CALL_SPLIT);

			ret = tcore_communicator_dispatch_request(ctx->comm, ur);
			if (ret != TCORE_RETURN_SUCCESS) {
				api_err = TAPI_API_OPERATION_FAILED;
			}

			dbg("ret = 0x%x", ret);

		} break;

		case TAPI_CS_CALL_TRANSFERCALL: {
			int id = 0;
			id = g_array_index(in_param1, unsigned int, 0);

			tcore_user_request_set_data(ur, sizeof( unsigned int ), &id);
			tcore_user_request_set_command(ur, TREQ_CALL_TRANSFER);

			ret = tcore_communicator_dispatch_request(ctx->comm, ur);
			if (ret != TCORE_RETURN_SUCCESS) {
				api_err = TAPI_API_OPERATION_FAILED;
			}

			dbg("ret = 0x%x", ret);

		} break;

		case TAPI_CS_CALL_GETACTIVELINE:
		case TAPI_CS_CALL_SETACTIVELINE:{

		} break;

		case TAPI_CS_CALL_DEFLECT:{
			TelCallDeflectDstInfo_t info;
			int id = 0;

			id = g_array_index(in_param1, unsigned int, 0);
			info = g_array_index(in_param2, TelCallDeflectDstInfo_t, 0);

			tcore_user_request_set_data(ur, sizeof(info.number), info.number);
			tcore_user_request_set_command(ur, TREQ_CALL_DEFLECT);

			ret = tcore_communicator_dispatch_request(ctx->comm, ur);
			if (ret != TCORE_RETURN_SUCCESS) {
				api_err = TAPI_API_OPERATION_FAILED;
			}

			dbg("ret = 0x%x", ret);

		} break;

		case TAPI_CS_CALL_ACTIVATECCBS:
		case TAPI_CS_CALL_GETSTATUS:
		case TAPI_CS_CALL_GETDURATION:
		case TAPI_CS_CALL_GETCONFERENCELIST:
		case TAPI_CS_CALL_GETPRIVACYMODE:
		case TAPI_CS_CALL_SETPRIVACYMODE:
		case TAPI_CS_CALL_FLASHINFO:
		case TAPI_CS_CALL_EXITEMERGENCYMODE:
		case TAPI_CS_CALL_GETCALLTIME:
		default:
			api_err = TAPI_API_NOT_SUPPORTED;
			break;
	}

OUT:
	if (api_err != TAPI_API_SUCCESS) {
		tcore_user_request_free(ur);
	}
	g_array_append_vals(*out_param1, &api_err, sizeof(int));
	g_array_append_vals(*out_param2, &req_id, sizeof(int));
}


static int _get_call_event( enum telephony_call_type type, int event )
{
	if ( type == CALL_TYPE_VIDEO ) {
		dbg("this is video call");

		if ( ( event & 0xFF0000 ) == TAPI_EVENT_TYPE_CONFIRMATION )
			event |= TAPI_EVENT_DATA_CALL_CONFIRMATION; 
		else
			event |= TAPI_EVENT_DATA_CALL_NOTIFICATION; 

	} else {
		dbg("this is voice call");
	}

	return event;
}

TReturn dbus_response_call(struct custom_data *ctx, UserRequest *ur, const char *appname,
		enum tcore_response_command command, unsigned int data_len, const void *data)
{
	dbg("command = 0x%x", command);

	switch (command) {
		case TRESP_CALL_DIAL: {

			struct tresp_call_general* resp_data = 0;
			resp_data = (struct tresp_call_general*)data;

			if ( resp_data->err )
				return ts_delivery_event( ctx->EvtDeliveryHandle,
						TAPI_EVENT_CLASS_CALL_VOICE,
						TAPI_EVENT_CALL_SETUP_CNF,
						appname,
						0xff,
						resp_data->err,
						sizeof( unsigned int ),
						(void*)&resp_data->id );

		} break;

		case TRESP_CALL_ANSWER: {
			struct tresp_call_general* resp_data = 0;
			resp_data = (struct tresp_call_general*)data;

			return ts_delivery_event( ctx->EvtDeliveryHandle,
									  TAPI_EVENT_CLASS_CALL_VOICE,
									  TAPI_EVENT_CALL_ANSWER_CNF,
									  appname,
									  0xff,
									  resp_data->err,
									  sizeof( unsigned int ),
									  (void*)&resp_data->id );

		} break;

		case TRESP_CALL_END: {
			struct tresp_call_general* resp_data = 0;
			resp_data = (struct tresp_call_general*)data;

			return ts_delivery_event( ctx->EvtDeliveryHandle,
									  TAPI_EVENT_CLASS_CALL_VOICE,
									  TAPI_EVENT_CALL_RELEASE_CNF,
									  appname,
									  0xff,
									  resp_data->err,
									  sizeof( unsigned int ),
									  (void*)&resp_data->id );

		} break;
		case TRESP_CALL_END_ALL: {
			struct tresp_call_general* resp_data = 0;
			resp_data = (struct tresp_call_general*)data;

			return ts_delivery_event( ctx->EvtDeliveryHandle,
									  TAPI_EVENT_CLASS_CALL_VOICE,
									  TAPI_EVENT_CALL_RELEASE_ALL_CNF,
									  appname,
									  0xff,
									  resp_data->err,
									  sizeof( unsigned int ),
									  (void*)&resp_data->id );

		} break;
		case TRESP_CALL_END_ALL_ACTIVE: {
			struct tresp_call_general* resp_data = 0;
			resp_data = (struct tresp_call_general*)data;

			return ts_delivery_event( ctx->EvtDeliveryHandle,
									  TAPI_EVENT_CLASS_CALL_VOICE,
									  TAPI_EVENT_CALL_RELEASE_ALL_ACTIVE_CNF,
									  appname,
									  0xff,
									  resp_data->err,
									  sizeof( unsigned int ),
									  (void*)&resp_data->id );

		} break;
		case TRESP_CALL_END_ALL_HELD: {
			struct tresp_call_general* resp_data = 0;
			resp_data = (struct tresp_call_general*)data;

			return ts_delivery_event( ctx->EvtDeliveryHandle,
									  TAPI_EVENT_CLASS_CALL_VOICE,
									  TAPI_EVENT_CALL_RELEASE_ALL_HELD_CNF,
									  appname,
									  0xff,
									  resp_data->err,
									  sizeof( unsigned int ),
									  (void*)&resp_data->id );

		} break;
		case TRESP_CALL_HOLD: {
			struct tresp_call_general* resp_data = 0;
			resp_data = (struct tresp_call_general*)data;

			return ts_delivery_event( ctx->EvtDeliveryHandle,
									  TAPI_EVENT_CLASS_CALL_VOICE,
									  TAPI_EVENT_CALL_HOLD_CNF,
									  appname,
									  0xff,
									  resp_data->err,
									  sizeof( unsigned int ),
									  (void*)&resp_data->id );

		} break;
		case TRESP_CALL_ACTIVE: {
			struct tresp_call_general* resp_data = 0;
			resp_data = (struct tresp_call_general*)data;

			return ts_delivery_event( ctx->EvtDeliveryHandle,
									  TAPI_EVENT_CLASS_CALL_VOICE,
									  TAPI_EVENT_CALL_RETRIEVE_CNF,
									  appname,
									  0xff,
									  resp_data->err,
									  sizeof( unsigned int ),
									  (void*)&resp_data->id );

		} break;
		case TRESP_CALL_SWAP: {
			struct tresp_call_general* resp_data = 0;
			resp_data = (struct tresp_call_general*)data;

			return ts_delivery_event( ctx->EvtDeliveryHandle,
									  TAPI_EVENT_CLASS_CALL_VOICE,
									  TAPI_EVENT_CALL_SWAP_CNF,
									  appname,
									  0xff,
									  resp_data->err,
									  sizeof( unsigned int ),
									  (void*)&resp_data->id );

		} break;
		case TRESP_CALL_JOIN: {
			struct tresp_call_general* resp_data = 0;
			resp_data = (struct tresp_call_general*)data;

			return ts_delivery_event( ctx->EvtDeliveryHandle,
									  TAPI_EVENT_CLASS_CALL_VOICE,
									  TAPI_EVENT_CALL_SETUPCONFERENCE_CNF,
									  appname,
									  0xff,
									  resp_data->err,
									  sizeof( unsigned int ),
									  (void*)&resp_data->id );

		} break;
		case TRESP_CALL_SPLIT: {
			struct tresp_call_general* resp_data = 0;
			resp_data = (struct tresp_call_general*)data;

			return ts_delivery_event( ctx->EvtDeliveryHandle,
									  TAPI_EVENT_CLASS_CALL_VOICE,
									  TAPI_EVENT_CALL_SPLITCONFERENCE_CNF,
									  appname,
									  0xff,
									  resp_data->err,
									  sizeof( unsigned int ),
									  (void*)&resp_data->id );

		} break;
		case TRESP_CALL_DEFLECT: {

		} break;
		case TRESP_CALL_TRANSFER: {
			struct tresp_call_general* resp_data = 0;
			resp_data = (struct tresp_call_general*)data;

			return ts_delivery_event( ctx->EvtDeliveryHandle,
									  TAPI_EVENT_CLASS_CALL_VOICE,
									  TAPI_EVENT_CALL_TRANSFER_CNF,
									  appname,
									  0xff,
									  resp_data->err,
									  sizeof( unsigned int ),
									  (void*)&resp_data->id );

		} break;
		case TRESP_CALL_SEND_DTMF: {
			struct tresp_call_general* resp_data = 0;
			resp_data = (struct tresp_call_general*)data;

			return ts_delivery_event( ctx->EvtDeliveryHandle,
									  TAPI_EVENT_CLASS_CALL_VOICE,
									  TAPI_EVENT_CALL_SEND_DTMF_CNF,
									  appname,
									  0xff,
									  resp_data->err,
									  0,
									  0 );

		} break;
		case TRESP_CALL_SET_SOUND_PATH: {
			struct tresp_call_sound_general* resp_data = 0;
			tapi_sound_error_info_t error_info = { 0 };

			resp_data = (struct tresp_call_sound_general*)data;
			error_info.cause = resp_data->cause;
			error_info.type  = 0x03;

			return ts_delivery_event( ctx->EvtDeliveryHandle,
									  TAPI_EVENT_CLASS_SOUND,
									  TAPI_EVENT_SOUND_SUCCESS_IND,
									  appname,
									  0xff,
									  resp_data->err,
									  sizeof( tapi_sound_error_info_t ),
									  (void*)&error_info );

		} break;

		case TRESP_CALL_SET_SOUND_VOLUME_LEVEL: {
			struct tresp_call_sound_general* resp_data = 0;
			tapi_sound_error_info_t error_info = { 0 };
			unsigned int err = 0;

			resp_data = (struct tresp_call_sound_general*)data;

			if ( resp_data->cause )
				err = TAPI_EVENT_SOUND_ERROR_IND;
			else
				err = TAPI_EVENT_SOUND_SUCCESS_IND;

			error_info.cause = resp_data->cause;
			error_info.type  = 0x01;

			return ts_delivery_event( ctx->EvtDeliveryHandle,
									  TAPI_EVENT_CLASS_SOUND,
									  TAPI_EVENT_SOUND_SUCCESS_IND,
									  appname,
									  0xff,
									  resp_data->err,
									  sizeof( tapi_sound_error_info_t ),
									  (void*)&error_info );

		} break;

		case TRESP_CALL_GET_SOUND_VOLUME_LEVEL: {

		} break;

		case TRESP_CALL_MUTE:
		case TRESP_CALL_UNMUTE: {
			struct tresp_call_sound_general* resp_data = 0;
			tapi_sound_error_info_t error_info = { 0 };
			unsigned int err = 0;

			resp_data = (struct tresp_call_sound_general*)data;

			if ( resp_data->cause )
				err = TAPI_EVENT_SOUND_ERROR_IND;
			else
				err = TAPI_EVENT_SOUND_SUCCESS_IND;

			error_info.cause = resp_data->cause;
			error_info.type  = 0x02;

			return ts_delivery_event( ctx->EvtDeliveryHandle,
									  TAPI_EVENT_CLASS_SOUND,
									  TAPI_EVENT_SOUND_SUCCESS_IND,
									  appname,
									  0xff,
									  resp_data->err,
									  sizeof( tapi_sound_error_info_t ),
									  (void*)&error_info );

		} break;

		case TRESP_CALL_GET_MUTE_STATUS: {

		} break;

		default:
			break;
	}

	return TRUE;
}

static void _launch_voice_call( struct tnoti_call_status_incoming* incoming )
{
	char id[2] = {0, };
	char cli[2] = {0, };
	char forward[2] = {0, };
	char active_line[2] = {0, };
	char cna[2] = {0, };
	char number[83] = {0, };
	char name[83] = {0, };
	int ret = 0;

	bundle *kb  = 0;

	snprintf( id, 2, "%d", incoming->id );
	dbg("id : [%s]", id );
	snprintf( cli, 2, "%d", incoming->cli.mode );
	dbg("cli : [%s]", id );
	snprintf( number, 83, "%s", incoming->cli.number );
	dbg("number : [%s]", number );
	snprintf( forward, 2, "%d", incoming->forward );
	dbg("forward : [%s]", forward );
	snprintf( active_line, 2, "%d", incoming->active_line );
	dbg("active_line : [%s]", active_line );

	if ( incoming->cna.mode == CALL_CNA_MODE_PRESENT )
		snprintf( cna, 2, "%d", TAPI_CALL_NAME_AVAIL );
	else
		snprintf( cna, 2, "%d", TAPI_CALL_NAME_RESTRICTED );

	dbg("cna : [%s]", cna );
	snprintf( name, 83, "%s", incoming->cna.name );
	dbg("name : [%s]", name );

	kb = bundle_create();
	bundle_add(kb, "launch-type", "MT");
	bundle_add(kb, "handle", id);
	bundle_add(kb, "number", number);
	bundle_add(kb, "name_mode", cna);
	bundle_add(kb, "name", name);
	bundle_add(kb, "clicause", cli);
	bundle_add(kb, "fwded", forward);
	bundle_add(kb, "activeline", active_line);

	ret = aul_launch_app("org.tizen.call", kb);
	bundle_free(kb);

	dbg("aul_launch_app [ voice call ] : %d", ret );
}

static void _launch_video_call( struct tnoti_call_status_incoming* incoming )
{
	char id[2] = {0, };
	char cli[2] = {0, };
	char forward[2] = {0, };
	char number[83] = {0, };
	int ret = 0;

	bundle *kb  = 0;

	dbg("Func Entrance");

	snprintf( id, 2, "%d", incoming->id );
	dbg("id : [%s]", id );
	snprintf( number, 83, "%s", incoming->cli.number );
	dbg("number : [%s]", number );
	snprintf( cli, 2, "%d", incoming->cli.mode );
	dbg("cli : [%s]", id );
	snprintf( forward, 2, "%d", incoming->forward );
	dbg("forward : [%s]", forward );

	kb = bundle_create();
	bundle_add(kb, "KEY_CALL_TYPE", "mt");
	bundle_add(kb, "KEY_CALL_HANDLE", id);
	bundle_add(kb, "KEY_CALLING_PARTY_NUMBER", number);
	bundle_add(kb, "KEY_CLI_CAUSE", cli);
	bundle_add(kb, "KEY_FORWARDED", forward);

	ret = aul_launch_app("org.tizen.vtmain", kb);
	bundle_free(kb);

	dbg("VT AUL return %d",ret);
}

TReturn dbus_notification_call(struct custom_data *ctx, CoreObject *source, enum tcore_notification_command command,
		unsigned int data_len, const void *data)
{
	dbg("command = 0x%x", command);

	switch (command) {
		case TNOTI_CALL_STATUS_IDLE: {
			struct tnoti_call_status_idle *status;
			TelCallEndInfo_t resp_data;

			status = (struct tnoti_call_status_idle*)data;

			resp_data.CallEndCause	= status->cause; 
			resp_data.pCallHandle	= status->id;
			resp_data.CallStartTime = 0;
			resp_data.CallEndTime	= 0;

			dbg("end noti publish");

			return ts_delivery_event( ctx->EvtDeliveryHandle, 
									  TAPI_EVENT_CLASS_CALL_VOICE, 
									  _get_call_event(status->type, TAPI_EVENT_CALL_END_IND),
									  0,
									  0xff,
									  0,
									  sizeof( TelCallEndInfo_t ),
									  (void*)&resp_data );


		} break;

		case TNOTI_CALL_STATUS_DIALING: {

			struct tnoti_call_status_dialing *status;
			status = (struct tnoti_call_status_dialing*)data;

			dbg("dialing noti publish");

			return ts_delivery_event( ctx->EvtDeliveryHandle, 
									  TAPI_EVENT_CLASS_CALL_VOICE, 
									  _get_call_event(status->type, TAPI_EVENT_CALL_SETUP_CNF),
									  0,
									  0xff,
									  0,
									  sizeof( unsigned int ),
									  (void*)&status->id );


		} break;

		case TNOTI_CALL_STATUS_ALERT: {

			struct tnoti_call_status_alert *status;
			status = (struct tnoti_call_status_alert*)data;

			dbg("alert noti publish");

			return ts_delivery_event( ctx->EvtDeliveryHandle, 
									  TAPI_EVENT_CLASS_CALL_VOICE, 
									  _get_call_event(status->type, TAPI_EVENT_CALL_ALERT_IND),
									  0,
									  0xff,
									  0,
									  sizeof( unsigned int ),
									  (void*)&status->id );


		} break;

		case TNOTI_CALL_STATUS_ACTIVE: {

			struct tnoti_call_status_active *status = 0;
			status = (struct tnoti_call_status_active*)data;

			dbg("active noti publish");

			return ts_delivery_event( ctx->EvtDeliveryHandle, 
									  TAPI_EVENT_CLASS_CALL_VOICE, 
									  _get_call_event(status->type, TAPI_EVENT_CALL_CONNECTED_IND),
									  0,
									  0xff,
									  0,
									  sizeof( unsigned int ),
									  (void*)&status->id );

		} break;

		case TNOTI_CALL_STATUS_HELD: {

			struct tnoti_call_status_held *status;
			status = (struct tnoti_call_status_held*)data;

			dbg("held noti publish");

			return ts_delivery_event( ctx->EvtDeliveryHandle, 
									  TAPI_EVENT_CLASS_CALL_VOICE, 
									  _get_call_event(status->type, TAPI_EVENT_CALL_HOLD_IND ),
									  0,
									  0xff,
									  0,
									  sizeof( unsigned int ),
									  (void*)&status->id );

		} break;

		case TNOTI_CALL_STATUS_INCOMING: {

			struct tnoti_call_status_incoming *incoming = 0;
			incoming = (struct tnoti_call_status_incoming*)data;

			dbg("incoming noti publish");

			if ( incoming->type == CALL_TYPE_VOICE )
				_launch_voice_call( incoming );
			else
				_launch_video_call( incoming );

		} break;

		case TNOTI_CALL_STATUS_WAITING:
			break;

		case TNOTI_CALL_INFO_CALL_CONNECTED_LINE:
		case TNOTI_CALL_INFO_WAITING:
		case TNOTI_CALL_INFO_CUG:
		case TNOTI_CALL_INFO_FORWARDED:
		case TNOTI_CALL_INFO_BARRED_INCOMING:
		case TNOTI_CALL_INFO_BARRED_OUTGOING:
		case TNOTI_CALL_INFO_DEFLECTED:
		case TNOTI_CALL_INFO_CLIR_SUPPRESSION_REJECT:
		case TNOTI_CALL_INFO_FORWARD_UNCONDITIONAL:
		case TNOTI_CALL_INFO_FORWARD_CONDITIONAL:
		case TNOTI_CALL_INFO_CALL_LINE_IDENTITY:
		case TNOTI_CALL_INFO_CALL_NAME_INFORMATION:
		case TNOTI_CALL_INFO_FORWARDED_CALL:
		case TNOTI_CALL_INFO_CUG_CALL:
		case TNOTI_CALL_INFO_DEFLECTED_CALL:
		case TNOTI_CALL_INFO_TRANSFERED_CALL:
			break;
		case TNOTI_CALL_INFO_HELD: {
		} break;
		case TNOTI_CALL_INFO_ACTIVE:
		case TNOTI_CALL_INFO_JOINED:
		case TNOTI_CALL_INFO_RELEASED_ON_HOLD:
		case TNOTI_CALL_INFO_TRANSFER_ALERT:
		case TNOTI_CALL_INFO_TRANSFERED:
		case TNOTI_CALL_INFO_CF_CHECK_MESSAGE:
		default:
			break;
	}

	return TRUE;
}
