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
#include <ITapiSound.h>

#include "tel_cs_conn.h"
#include "common.h"
#include "ts_utility.h"
#include "ts_common.h"
#include "ts_svr_req.h"
#include "ts_noti.h"
#include "modules.h"

void dbus_request_sound(struct custom_data *ctx, TcorePlugin *plugin, int tapi_service_function, GArray* in_param1,
		GArray* in_param2, GArray* in_param3, GArray* in_param4, GArray** out_param1, GArray** out_param2,
		GArray** out_param3, GArray** out_param4, GError** error)
{
	int api_err = TAPI_API_SUCCESS;
	int					req_id = 0xff;

	CoreObject*			o = NULL;
	UserRequest*		ur = NULL;
	struct tcore_user_info	ui = { 0, };

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

	dbg("tapi_service_function : [%d]", tapi_service_function );
			

	switch (tapi_service_function) {
		case TAPI_CS_SOUND_VOLUME_GET: {
			struct treq_call_sound_get_volume_level data;
			tapi_sound_volume_control_t info;

			info = g_array_index(in_param1, tapi_sound_volume_control_t, 0 );

			data.device = ( info.vol_type >> 4 );
			data.device <<= 4;
			data.sound  = ( info.vol_type - data.device );

			tcore_user_request_set_data(ur, sizeof( struct treq_call_sound_get_volume_level ), &data);
			tcore_user_request_set_command(ur, TREQ_CALL_GET_SOUND_VOLUME_LEVEL );

		} break;

		case TAPI_CS_SOUND_VOLUME_SET: {
			struct treq_call_sound_set_volume_level data;
			tapi_sound_volume_control_t info;

			info = g_array_index(in_param1, tapi_sound_volume_control_t, 0 );

			data.device = ( info.vol_type >> 4 );
			data.device <<= 4;
			data.sound  = ( info.vol_type - data.device );
			data.volume = info.volume;

 			tcore_user_request_set_data(ur, sizeof( struct treq_call_sound_set_volume_level ), &data);
			tcore_user_request_set_command(ur, TREQ_CALL_SET_SOUND_VOLUME_LEVEL );

		} break;

		case TAPI_CS_SOUND_MIC_MUTE_GET: {

			tcore_user_request_set_command(ur, TREQ_CALL_GET_MUTE_STATUS );

		} break;

		case TAPI_CS_SOUND_MIC_MUTE_SET: {
			enum tcore_request_command command = 0;
			tapi_sound_mic_mute_t status = 0;

			status = g_array_index( in_param1, tapi_sound_mic_mute_t, 0 );

			if ( status ==  TAPI_SOUND_MIC_MUTE )
				command = TREQ_CALL_MUTE;
			else
				command = TREQ_CALL_UNMUTE;

			tcore_user_request_set_command(ur, command );

		} break;

		case TAPI_CS_SOUND_AUDIO_PATH_CTRL: {
			struct treq_call_sound_set_path	data;

			data.path = g_array_index( in_param1, tapi_sound_audio_path_t, 0 );

 			tcore_user_request_set_data(ur, sizeof( struct treq_call_sound_set_path ), &data);
			tcore_user_request_set_command(ur, TREQ_CALL_SET_SOUND_PATH );

		} break;

		case TAPI_CS_SOUND_AUDIOLOOPBACK_SET: 
		case TAPI_CS_SOUND_AUDIO_CLOCK_CTRL:
		case TAPI_CS_SOUND_VOICE_RECORDING_SET:
		default:
			api_err = TAPI_API_SUCCESS;
			break;
	}

	ret = tcore_communicator_dispatch_request(ctx->comm, ur);
	if (ret != TCORE_RETURN_SUCCESS) {
		api_err = TAPI_API_OPERATION_FAILED;
	}

	dbg("ret = 0x%x", ret);

OUT:
	if (api_err != TAPI_API_SUCCESS) {
		tcore_user_request_free(ur);
	}
	g_array_append_vals(*out_param1, &api_err, sizeof(int));
	g_array_append_vals(*out_param2, &req_id, sizeof(int));
}

TReturn dbus_response_sound(struct custom_data *ctx, UserRequest *ur, const char *appname,
		enum tcore_response_command command, unsigned int data_len, const void *data)
{
	dbg("command = 0x%x", command);

	switch (command) {
		default:
			break;
	}

	return TRUE;
}

TReturn dbus_notification_sound(struct custom_data *ctx, CoreObject *source, enum tcore_notification_command command,
		unsigned int data_len, const void *data)
{
	dbg("command = 0x%x", command);

	switch (command) {
		default:
			break;
	}

	return TRUE;
}
