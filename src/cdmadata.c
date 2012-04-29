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

void dbus_request_cdmadata(struct custom_data *ctx, TcorePlugin *plugin, int tapi_service_function, GArray* in_param1,
		GArray* in_param2, GArray* in_param3, GArray* in_param4, GArray** out_param1, GArray** out_param2,
		GArray** out_param3, GArray** out_param4, GError** error)
{
	int api_err = TAPI_API_SUCCESS;

	switch (tapi_service_function) {
		case TAPI_CS_DATA_MOBILE_IP_NAI_GET:
		case TAPI_CS_DATA_MOBILE_IP_NAI_SET:
		case TAPI_CS_DATA_CURRENT_NAI_INDEX_GET:
		case TAPI_CS_DATA_TE2_STATUS_GET:
		case TAPI_CS_DATA_ROAM_GUARD_GET:
		case TAPI_CS_DATA_ROAM_GUARD_SET:
		case TAPI_CS_DATA_MODEM_NAI_GET:
		case TAPI_CS_DATA_MODEM_NAI_SET:
		case TAPI_CS_DATA_EVDO_REV_CONFIG_GET:
		case TAPI_CS_DATA_EVDO_REV_CONFIG_SET:
		case TAPI_CS_DATA_RESTORE_NAI:
		case TAPI_CS_DATA_DDTM_CONFIGURATION_GET:
		case TAPI_CS_DATA_DDTM_CONFIGURATION_SET:
		default:
			api_err = TAPI_API_NOT_SUPPORTED;
			break;
	}

	g_array_append_vals(*out_param1, &api_err, sizeof(int));
}

TReturn dbus_response_cdmadata(struct custom_data *ctx, UserRequest *ur, const char *appname,
		enum tcore_response_command command, unsigned int data_len, const void *data)
{
	dbg("command = 0x%x", command);

	switch (command) {
		default:
			break;
	}

	return TRUE;
}

TReturn dbus_notification_cdmadata(struct custom_data *ctx, CoreObject *source, enum tcore_notification_command command,
		unsigned int data_len, const void *data)
{
	dbg("command = 0x%x", command);

	switch (command) {
		default:
			break;
	}

	return TRUE;
}
