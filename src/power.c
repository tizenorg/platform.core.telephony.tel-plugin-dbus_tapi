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
#include <TelPower.h>

#include "tel_cs_conn.h"
#include "common.h"
#include "ts_utility.h"
#include "ts_common.h"
#include "ts_svr_req.h"
#include "ts_noti.h"
#include "modules.h"

void dbus_request_power(struct custom_data *ctx, TcorePlugin *plugin, int tapi_service_function, GArray* in_param1,
		GArray* in_param2, GArray* in_param3, GArray* in_param4, GArray** out_param1, GArray** out_param2,
		GArray** out_param3, GArray** out_param4, GError** error)
{
	int api_err = TAPI_API_SUCCESS;
	tapi_dbus_connection_name conn_name;
	tapi_power_flight_mode_type_t info_mode;

	struct treq_modem_set_flightmode data_mode;
	TReturn ret;
	GSList *co_list = NULL;
	CoreObject *co_modem = NULL;
	UserRequest *ur = NULL;
	struct tcore_user_info ui = { 0, };

	conn_name = g_array_index(in_param4, tapi_dbus_connection_name, 0);

	co_list = tcore_plugin_get_core_objects_bytype(plugin, CORE_OBJECT_TYPE_MODEM);
	if (!co_list) {
		api_err = TAPI_API_NOT_SUPPORTED;
		goto OUT;
	}

	co_modem = (CoreObject *)co_list->data;
	g_slist_free(co_list);

	if (!co_modem) {
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
		case TAPI_CS_POWER_PHONE_POWER_ON_OFF: {
			tapi_power_phone_cmd_t cmd;
			enum tcore_request_command request;

			cmd = g_array_index(in_param1, tapi_power_phone_cmd_t, 0);

			switch ( cmd ) {
				case TAPI_PHONE_POWER_OFF:
					request = TREQ_MODEM_POWER_OFF;
					break;
				case TAPI_PHONE_POWER_ON:
					request = TREQ_MODEM_POWER_ON;
					break;
				case TAPI_PHONE_POWER_RESET:
					request = TREQ_MODEM_POWER_RESET;
					break;
				default:
					api_err = TAPI_API_NOT_SUPPORTED;
					goto OUT;
			}

			tcore_user_request_set_command(ur, request);

			ret = tcore_communicator_dispatch_request(ctx->comm, ur);
			if (ret != TCORE_RETURN_SUCCESS) {
				api_err = TAPI_API_OPERATION_FAILED;
			}
			else {
				api_err = TAPI_API_SUCCESS;
			}

			dbg("ret = 0x%x", ret);

	    } break;

		case TAPI_CS_POWER_FLIGHT_MODE:
			info_mode = g_array_index(in_param1, unsigned int, 0);
			if (info_mode == TAPI_POWER_FLIGHT_MODE_ENTER)
				data_mode.enable = 1;
			else
				data_mode.enable = 0;

			tcore_user_request_set_data(ur, sizeof(struct treq_modem_set_flightmode), &data_mode);
			tcore_user_request_set_command(ur, TREQ_MODEM_SET_FLIGHTMODE);

			ret = tcore_communicator_dispatch_request(ctx->comm, ur);
			if (ret != TCORE_RETURN_SUCCESS) {
				api_err = TAPI_API_OPERATION_FAILED;
			}
			else {
				api_err = TAPI_API_SUCCESS;
			}

			dbg("ret = 0x%x", ret);
			break;

		case TAPI_CS_POWER_REBOOT:
		case TAPI_CS_POWER_RAMDUMP:
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

TReturn dbus_response_power(struct custom_data *ctx, UserRequest *ur, const char *appname,
		enum tcore_response_command command, unsigned int data_len, const void *data)
{
	tapi_power_flight_mode_resp_type_t info_set_flight_mode;
	const struct tresp_modem_set_flightmode *data_set_flight_mode = data;
	int ret = TAPI_API_SUCCESS;
	int request_id = 0;

	dbg("command = 0x%x", command);

	switch (command) {
		case TRESP_MODEM_SET_FLIGHTMODE:
			dbg("reslut = 0x%x", data_set_flight_mode->result);

			if (data_set_flight_mode->result > 0x02) {
				ret = TAPI_API_OPERATION_FAILED;
				info_set_flight_mode = TAPI_POWER_FLIGHT_MODE_RESP_FAIL;
			}

			if (data_set_flight_mode->result == 0x1)
				info_set_flight_mode = TAPI_POWER_FLIGHT_MODE_RESP_ON;
			else if (data_set_flight_mode->result == 0x2)
				info_set_flight_mode = TAPI_POWER_FLIGHT_MODE_RESP_OFF;

			return ts_delivery_event(ctx->EvtDeliveryHandle, TAPI_EVENT_CLASS_POWER, TAPI_EVENT_POWER_FLIGHT_MODE_RESP,
					appname, request_id, ret, sizeof(tapi_power_flight_mode_resp_type_t), &info_set_flight_mode);

		default:
			break;
	}

	return TRUE;
}

TReturn dbus_notification_power(struct custom_data *ctx, CoreObject *source, enum tcore_notification_command command,
		unsigned int data_len, const void *data)
{
	dbg("command = 0x%x", command);

	switch (command) {
		case TNOTI_MODEM_POWER: {
			struct tnoti_modem_power *power = 0;
			int class = 0, event = 0;

			power = (struct tnoti_modem_power *)data;

			if ( power->state == MODEM_STATE_ERROR ) {
				class = TAPI_EVENT_CLASS_DISPLAY;
				event = TAPI_EVENT_DISPLAY_PHONE_FATAL_NOTI;

			} else if ( power->state == MODEM_STATE_OFFLINE ) {
				class = TAPI_EVENT_CLASS_POWER;
				event = TAPI_EVENT_POWER_PHONE_OFF;

			} else if ( power->state == MODEM_STATE_ONLINE ) {
				class = TAPI_EVENT_CLASS_POWER;
				event = TAPI_EVENT_POWER_SERVICE_READY_IND;

			} else {
				dbg("unsupport state : (0x%x)", power->state);
				return FALSE;
			}

			return ts_delivery_event( ctx->EvtDeliveryHandle, 
					class, 
					event,
					0,
					0xff,
					0, 
					0,
					0 );

		} break;
		default:
			break;
	}

	return TRUE;
}
