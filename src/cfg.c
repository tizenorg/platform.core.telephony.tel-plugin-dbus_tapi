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
#include <co_sim.h>

#include <TapiCommon.h>

#include "tel_cs_conn.h"
#include "common.h"
#include "ts_utility.h"
#include "ts_common.h"
#include "ts_svr_req.h"
#include "modules.h"

void dbus_request_cfg(struct custom_data *ctx, TcorePlugin *plugin, int tapi_service_function, GArray* in_param1,
		GArray* in_param2, GArray* in_param3, GArray* in_param4, GArray** out_param1, GArray** out_param2,
		GArray** out_param3, GArray** out_param4, GError** error)
{
	int api_err = TAPI_API_SUCCESS;

	switch (tapi_service_function) {
		case TAPI_CS_CFG_DEFAULTCONFGURATION_EXE:
		case TAPI_CS_CFG_GET_A_KEY:
		case TAPI_CS_CFG_VERIFY_A_KEY:
		case TAPI_CS_CFG_GET_MSL_INFO:
		case TAPI_CS_CFG_GET_VOCODER_OPTION:
		case TAPI_CS_CFG_SET_VOCODER_OPTION:
		case TAPI_CS_CFG_GET_HIDDEN_MENU_ACCESS:
		case TAPI_CS_CFG_SET_HIDDEN_MENU_ACCESS:
		case TAPI_CS_CFG_GET_CURRENT_CARRIER:
		case TAPI_CS_CFG_SET_CURRENT_CARRIER:
		case TAPI_CS_CFG_GET_SIO_MODE:
		case TAPI_CS_CFG_SET_SIO_MODE:
		case TAPI_CS_CFG_GET_ACTIVATION_DATE:
		case TAPI_CS_CFG_GET_RECONDITIONED_DATE:
		case TAPI_CS_CFG_SET_TTY_MODE:
		case TAPI_CS_CFG_GET_TTY_MODE:
		default:
			api_err = TAPI_API_NOT_SUPPORTED;
			break;
	}

	g_array_append_vals(*out_param1, &api_err, sizeof(int));
}

TReturn dbus_response_cfg(struct custom_data *ctx, UserRequest *ur, const char *appname,
		enum tcore_response_command command, unsigned int data_len, const void *data)
{
	dbg("command = 0x%x", command);

	switch (command) {
		default:
			break;
	}

	return TRUE;
}

TReturn dbus_notification_cfg(struct custom_data *ctx, CoreObject *source, enum tcore_notification_command command,
		unsigned int data_len, const void *data)
{
	dbg("command = 0x%x", command);

	switch (command) {
		default:
			break;
	}

	return TRUE;
}
