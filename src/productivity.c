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
#include <util.h>
#include <communicator.h>

#include <TapiCommon.h>
#include <ITapiProductivity.h>

#include "tel_cs_conn.h"
#include "common.h"
#include "ts_utility.h"
#include "ts_common.h"
#include "ts_svr_req.h"
#include "ts_noti.h"
#include "modules.h"

#define SVCMODE_CMD_ENTER			1
#define SVCMODE_CMD_END				2
#define SVCMODE_CMD_SEND_KEYCODE	3
#define SVCMODE_CMD_REQUEST_DUMP	4
#define SVCMODE_CMD_DISPLAY_SCREEN	5
#define SVCMODE_CMD_SCREEN_CONFIG   6
#define SVCMODE_CMD_CHANGE_SVCMODE  7
#define SVCMODE_CMD_DEVICE_TEST     8

#define FACTORY_CMD_SET_OMISSION_AVOIDANCE 0x10
#define FACTORY_CMD_GET_OMISSION_AVOIDANCE 0x11

static int svcmode_convert_table[] = {
		[TAPI_SVC_MODE_TEST_MANUAL] = 0x01,
		[TAPI_SVC_MODE_NAM_EDIT] = 0x03,
		[TAPI_SVC_MODE_MONITOR] = 0x04,
		[TAPI_SVC_MODE_TEST_AUTO] = 0x02,
		[TAPI_SVC_MODE_NAM_BASIC_EDIT] = 0x03,
		[TAPI_SVC_MODE_PHONE_TEST] = 0x05,
		[TAPI_SVC_MODE_OPERATOR_SPECIFIC_TEST] = 0x06,
};

static int svcmode_convert_table_r[] = {
		[0x01] = TAPI_SVC_MODE_TEST_MANUAL,
		[0x02] = TAPI_SVC_MODE_TEST_AUTO,
		[0x03] = TAPI_SVC_MODE_NAM_EDIT,
		[0x04] = TAPI_SVC_MODE_MONITOR,
		[0x05] = TAPI_SVC_MODE_PHONE_TEST,
		[0x06] = TAPI_SVC_MODE_OPERATOR_SPECIFIC_TEST
};

static int svcmode_convert_debug_dump_table[] = {
		[TAPI_SVC_DBG_CP_DUMP_TARGET_ALL] = 0x00,
		[TAPI_SVC_DBG_CP_DUMP_TARGET_MSG] = 0x01,
		[TAPI_SVC_DBG_CP_DUMP_TARGET_LOG] = 0x02,
		[TAPI_SVC_DBG_CP_DUMP_TARGET_LOG2] = 0x03,
		[TAPI_SVC_DBG_CP_DUMP_TARGET_RAM_DUMP] = 0x04,
};

void dbus_request_productivity(struct custom_data *ctx, TcorePlugin *plugin, int tapi_service_function, GArray* in_param1,
		GArray* in_param2, GArray* in_param3, GArray* in_param4, GArray** out_param1, GArray** out_param2,
		GArray** out_param3, GArray** out_param4, GError** error)
{
	int api_err = TAPI_API_SUCCESS;
	tapi_dbus_connection_name conn_name;
	tapi_service_mode_t svc_mode;
	tapi_test_mode_sub_t test_sub;
	char key_code;
	tapi_service_mode_debug_cp_dump_t dump_type;
	tapi_factory_omission_avoidance_info_t *oa_info;
	int value;
	unsigned int i;
	char key[10] = { 0, };

	TReturn ret;
	UserRequest *ur = NULL;
	struct tcore_user_info ui = { 0, };
	gchar *req_data;
	GHashTable *req_hash;
	GHashTable *req_sub_hash;
	int custom_command;

	conn_name = g_array_index(in_param4, tapi_dbus_connection_name, 0);

	ur = tcore_user_request_new(ctx->comm, tcore_plugin_get_description(plugin)->name);
	if (!ur) {
		api_err = TAPI_API_SERVER_FAILURE;
		goto OUT;
	}

	ui.appname = conn_name.name;
	tcore_user_request_set_user_info(ur, &ui);

	switch (tapi_service_function) {
		case TAPI_CS_SVCMODE_ENTERSVCMODE:
			svc_mode = svcmode_convert_table[g_array_index(in_param1, tapi_service_mode_t, 0)];
			test_sub = g_array_index(in_param2, tapi_test_mode_sub_t, 0);
			custom_command = SVCMODE_CMD_ENTER;

			req_hash = tcore_util_marshal_create();
			tcore_util_marshal_add_data(req_hash, "svc_mode", &svc_mode, TCORE_UTIL_MARSHAL_DATA_INT_TYPE);
			tcore_util_marshal_add_data(req_hash, "test_sub", &test_sub, TCORE_UTIL_MARSHAL_DATA_INT_TYPE);
			tcore_util_marshal_add_data(req_hash, "custom_command", &custom_command, TCORE_UTIL_MARSHAL_DATA_INT_TYPE);

			req_data = tcore_util_marshal_serialize(req_hash);
			if (!req_data) {
				api_err = TAPI_API_OPERATION_FAILED;
				break;
			}

			dbg("serialize data = [%s]", req_data);
			tcore_user_request_set_data(ur, strlen(req_data), req_data);
			tcore_user_request_set_command(ur, TREQ_CUSTOM);

			ret = tcore_communicator_dispatch_request(ctx->comm, ur);
			if (ret != TCORE_RETURN_SUCCESS) {
				api_err = TAPI_API_OPERATION_FAILED;
			}

			dbg("ret = 0x%x", ret);
			break;

		case TAPI_CS_SVCMODE_ENDSVCMODE:
			svc_mode = svcmode_convert_table[g_array_index(in_param1, tapi_service_mode_t, 0)];
			custom_command = SVCMODE_CMD_END;

			req_hash = tcore_util_marshal_create();
			tcore_util_marshal_add_data(req_hash, "svc_mode", &svc_mode, TCORE_UTIL_MARSHAL_DATA_INT_TYPE);
			tcore_util_marshal_add_data(req_hash, "custom_command", &custom_command, TCORE_UTIL_MARSHAL_DATA_INT_TYPE);

			req_data = tcore_util_marshal_serialize(req_hash);
			if (!req_data) {
				api_err = TAPI_API_OPERATION_FAILED;
				break;
			}

			dbg("serialize data = [%s]", req_data);
			tcore_user_request_set_data(ur, strlen(req_data), req_data);
			tcore_user_request_set_command(ur, TREQ_CUSTOM);

			ret = tcore_communicator_dispatch_request(ctx->comm, ur);
			if (ret != TCORE_RETURN_SUCCESS) {
				api_err = TAPI_API_OPERATION_FAILED;
			}

			dbg("ret = 0x%x", ret);
			break;

		case TAPI_CS_SVCMODE_PROCESS_KEYCODE:
			key_code = g_array_index(in_param1, char, 0);
			custom_command = SVCMODE_CMD_SEND_KEYCODE;

			req_hash = tcore_util_marshal_create();
			tcore_util_marshal_add_data(req_hash, "key_code", &key_code, TCORE_UTIL_MARSHAL_DATA_INT_TYPE);
			tcore_util_marshal_add_data(req_hash, "custom_command", &custom_command, TCORE_UTIL_MARSHAL_DATA_INT_TYPE);

			req_data = tcore_util_marshal_serialize(req_hash);
			if (!req_data) {
				api_err = TAPI_API_OPERATION_FAILED;
				break;
			}

			dbg("serialize data = [%s]", req_data);
			tcore_user_request_set_data(ur, strlen(req_data), req_data);
			tcore_user_request_set_command(ur, TREQ_CUSTOM);

			ret = tcore_communicator_dispatch_request(ctx->comm, ur);
			if (ret != TCORE_RETURN_SUCCESS) {
				api_err = TAPI_API_OPERATION_FAILED;
			}

			dbg("ret = 0x%x", ret);
			break;

		case TAPI_CS_SVCMODE_DEBUG_DUMP:
			dump_type = svcmode_convert_debug_dump_table[g_array_index(in_param1, tapi_service_mode_debug_cp_dump_t, 0)];
			custom_command = SVCMODE_CMD_REQUEST_DUMP;

			req_hash = tcore_util_marshal_create();
			tcore_util_marshal_add_data(req_hash, "dump_type", &dump_type, TCORE_UTIL_MARSHAL_DATA_INT_TYPE);
			tcore_util_marshal_add_data(req_hash, "custom_command", &custom_command, TCORE_UTIL_MARSHAL_DATA_INT_TYPE);

			req_data = tcore_util_marshal_serialize(req_hash);
			if (!req_data) {
				api_err = TAPI_API_OPERATION_FAILED;
				break;
			}

			dbg("serialize data = [%s]", req_data);
			tcore_user_request_set_data(ur, strlen(req_data), req_data);
			tcore_user_request_set_command(ur, TREQ_CUSTOM);

			ret = tcore_communicator_dispatch_request(ctx->comm, ur);
			if (ret != TCORE_RETURN_SUCCESS) {
				api_err = TAPI_API_OPERATION_FAILED;
			}

			dbg("ret = 0x%x", ret);
			break;

		case TAPI_CS_FACTORY_OMISSION_AVOIDANCE_SET:
			oa_info = &g_array_index(in_param1, tapi_factory_omission_avoidance_info_t, 0);
			custom_command = FACTORY_CMD_SET_OMISSION_AVOIDANCE;

			req_hash = tcore_util_marshal_create();
			value = oa_info->cmd.write_cmd;
			tcore_util_marshal_add_data(req_hash, "cmd", &value, TCORE_UTIL_MARSHAL_DATA_INT_TYPE);
			value = oa_info->data_cnt;
			tcore_util_marshal_add_data(req_hash, "count", &value, TCORE_UTIL_MARSHAL_DATA_INT_TYPE);
			tcore_util_marshal_add_data(req_hash, "custom_command", &custom_command, TCORE_UTIL_MARSHAL_DATA_INT_TYPE);

			for (i = 0; i < oa_info->data_cnt; i++) {
				dbg("i=%d [id=0x%x] [result=0x%x]", i, oa_info->data[i].test_id, oa_info->data[i].test_result);
				snprintf(key, 10, "%d", i);

				req_sub_hash = tcore_util_marshal_create();
				value = oa_info->data[i].test_id;
				tcore_util_marshal_add_data(req_sub_hash, "id", &value, TCORE_UTIL_MARSHAL_DATA_INT_TYPE);
				value = oa_info->data[i].test_result;
				tcore_util_marshal_add_data(req_sub_hash, "result", &value, TCORE_UTIL_MARSHAL_DATA_INT_TYPE);

				tcore_util_marshal_add_data(req_hash, key, req_sub_hash, TCORE_UTIL_MARSHAL_DATA_OBJECT_TYPE);
			}

			req_data = tcore_util_marshal_serialize(req_hash);
			if (!req_data) {
				api_err = TAPI_API_OPERATION_FAILED;
				break;
			}

			dbg("serialize data = [%s]", req_data);
			tcore_user_request_set_data(ur, strlen(req_data), req_data);
			tcore_user_request_set_command(ur, TREQ_CUSTOM);

			ret = tcore_communicator_dispatch_request(ctx->comm, ur);
			if (ret != TCORE_RETURN_SUCCESS) {
				api_err = TAPI_API_OPERATION_FAILED;
			}

			dbg("ret = 0x%x", ret);
			break;

		case TAPI_CS_IMEI_STARTFACTORYPROCESS:
		case TAPI_CS_IMEI_COMPARE_ITEM_IND:
		case TAPI_CS_FACTORY_OMISSION_AVOIDANCE_GET:
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

TReturn dbus_response_productivity(struct custom_data *ctx, UserRequest *ur, const char *appname,
		enum tcore_response_command command, unsigned int data_len, const void *data)
{
	GHashTable *resp_hash;
	int custom_command;
	tapi_service_mode_t svc_mode;
	int status;
	int request_id = 0;
	int ret = TAPI_API_SUCCESS;

	dbg("command = 0x%x", command);

	if (command != TRESP_CUSTOM)
		return TRUE;

	resp_hash = tcore_util_marshal_deserialize_string(data);
	if (!resp_hash) {
		warn("hash corrupted");
		return TCORE_RETURN_FAILURE;
	}

	custom_command = tcore_util_marshal_get_int(resp_hash, "custom_command");
	dbg("custom_command = %d", custom_command);

	switch (custom_command) {
		case SVCMODE_CMD_ENTER:
			svc_mode = svcmode_convert_table_r[g_value_get_int(g_hash_table_lookup(resp_hash, "svc_mode"))];
			tcore_util_marshal_destory(resp_hash);

			dbg("svc_mode = %d", svc_mode);
			return ts_delivery_event(ctx->EvtDeliveryHandle, TAPI_EVENT_CLASS_SVCMODE, TAPI_EVENT_SVCMODE_START_NOTI,
					appname, request_id, ret, sizeof(tapi_service_mode_t), &svc_mode);
			break;

		case SVCMODE_CMD_END:
			svc_mode = svcmode_convert_table_r[g_value_get_int(g_hash_table_lookup(resp_hash, "svc_mode"))];
			tcore_util_marshal_destory(resp_hash);

			dbg("svc_mode = %d", svc_mode);
			return ts_delivery_event(ctx->EvtDeliveryHandle, TAPI_EVENT_CLASS_SVCMODE, TAPI_EVENT_SVCMODE_END_NOTI,
					appname, request_id, ret, sizeof(tapi_service_mode_t), &svc_mode);
			break;

		case SVCMODE_CMD_SEND_KEYCODE:
			dbg("send_keycode response. unused.");
			break;

		case SVCMODE_CMD_REQUEST_DUMP:
			status = svcmode_convert_table_r[g_value_get_int(g_hash_table_lookup(resp_hash, "status"))];
			tcore_util_marshal_destory(resp_hash);

			dbg("status = %d", status);
			return ts_delivery_event(ctx->EvtDeliveryHandle, TAPI_EVENT_CLASS_SVCMODE, TAPI_EVENT_SVCMODE_DEBUG_DUMP_CNF,
					appname, request_id, ret, sizeof(int), &status);
			break;

		case FACTORY_CMD_SET_OMISSION_AVOIDANCE:
			status = svcmode_convert_table_r[g_value_get_int(g_hash_table_lookup(resp_hash, "status"))];
			tcore_util_marshal_destory(resp_hash);

			dbg("status = %d", status);
			return ts_delivery_event(ctx->EvtDeliveryHandle, TAPI_EVENT_CLASS_FACTORY,
					TAPI_EVENT_FACTORY_OMISSION_AVOIDANCE_SET_CNF, appname, request_id, ret, sizeof(int), &status);
			break;
	}

	return TRUE;
}

TReturn dbus_notification_productivity(struct custom_data *ctx, CoreObject *source, enum tcore_notification_command command,
		unsigned int data_len, const void *data)
{
	tapi_service_display_info_t noti_display_screen;
	tapi_device_info_t noti_device_info;
	int svc_mode;
	tapi_screen_config_t noti_screen_config;

	GHashTable *noti_hash;
	GHashTable *noti_sub_hash;
	int custom_command;
	int i;
	int count;
	char key[10] = { 0, };
	int line = 0;
	int reverse = 0;
	char *text = NULL;

	dbg("command = 0x%x", command);

	if (command != TNOTI_CUSTOM)
		return TRUE;

	noti_hash = tcore_util_marshal_deserialize_string(data);
	if (!noti_hash) {
		warn("hash corrupted");
		return TCORE_RETURN_FAILURE;
	}

	custom_command = tcore_util_marshal_get_int(noti_hash, "custom_command");
	dbg("custom_command = %d", custom_command);

	switch (custom_command) {
		case SVCMODE_CMD_DISPLAY_SCREEN:
			memset(&noti_display_screen, 0, sizeof(tapi_service_display_info_t));

			count = tcore_util_marshal_get_int(noti_hash, "count");
			noti_display_screen.num_of_lines = count;

			dbg("count = %d", count);

			for (i = 0; i < count; i++) {
				snprintf(key, 10, "%d", i);
				noti_sub_hash = tcore_util_marshal_get_object(noti_hash, key);
				if (noti_sub_hash) {
					line = tcore_util_marshal_get_int(noti_sub_hash, "line");
					reverse = tcore_util_marshal_get_int(noti_sub_hash, "reverse");
					text = tcore_util_marshal_get_string(noti_sub_hash, "text");

					noti_display_screen.display[i].line = line;
					noti_display_screen.display[i].reverse = reverse;
					snprintf(noti_display_screen.display[i].string, 30, "%s", text);
				}

				dbg("i = %d, [%02d][%d] [%s]", i, line, reverse, text);

				if (text)
					free(text);

				if (i > 100)
					break;
			}

			ts_delivery_event(ctx->EvtDeliveryHandle, TAPI_EVENT_CLASS_SVCMODE, TAPI_EVENT_SVCMODE_DISP_SCREEN_NOTI, NULL, 0,
					TAPI_API_SUCCESS, sizeof(tapi_service_display_info_t), (void *) &noti_display_screen);
			break;

		case SVCMODE_CMD_SCREEN_CONFIG:
			memset(&noti_screen_config, 0, sizeof(tapi_screen_config_t));
			noti_screen_config.line_number = tcore_util_marshal_get_int(noti_hash, "line_number");
			noti_screen_config.keypad = tcore_util_marshal_get_int(noti_hash, "keypad");

			ts_delivery_event(ctx->EvtDeliveryHandle, TAPI_EVENT_CLASS_SVCMODE, TAPI_EVENT_SVCMODE_SCREEN_CFG_NOTI,
					NULL, 0, TAPI_API_SUCCESS, sizeof(tapi_screen_config_t), (void *) &noti_screen_config);
			break;

		case SVCMODE_CMD_CHANGE_SVCMODE:
			svc_mode = tcore_util_marshal_get_int(noti_hash, "svc_mode");

			ts_delivery_event(ctx->EvtDeliveryHandle, TAPI_EVENT_CLASS_SVCMODE, TAPI_EVENT_SVCMODE_CHANGE_NOTI, NULL, 0,
					TAPI_API_SUCCESS, sizeof(int), (void *) &svc_mode);
			break;

		case SVCMODE_CMD_DEVICE_TEST:
			memset(&noti_device_info, 0, sizeof(tapi_device_info_t));
			noti_device_info.device_id = tcore_util_marshal_get_int(noti_hash, "device_id");
			noti_device_info.device_status = tcore_util_marshal_get_int(noti_hash, "device_status");

			ts_delivery_event(ctx->EvtDeliveryHandle, TAPI_EVENT_CLASS_SVCMODE, TAPI_EVENT_SVCMODE_SCREEN_CFG_NOTI,
					NULL, 0, TAPI_API_SUCCESS, sizeof(tapi_device_info_t), (void *) &noti_device_info);
			break;

		case SVCMODE_CMD_SEND_KEYCODE:
			dbg("keycode notification. unused.");
			break;

		default:
			break;
	}

	return TRUE;
}
