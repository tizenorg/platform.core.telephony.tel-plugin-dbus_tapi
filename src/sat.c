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

#include <security-server.h>

#include <tcore.h>
#include <plugin.h>
#include <server.h>
#include <storage.h>
#include <user_request.h>
#include <communicator.h>
#include <core_object.h>
#include <co_sat.h>

#include <TapiCommon.h>
#include <TelSat.h>

#include "tel_cs_conn.h"
#include "common.h"
#include "ts_utility.h"
#include "ts_common.h"
#include "ts_svr_req.h"
#include "ts_noti.h"
#include "modules.h"

#include "sat_mgr.h"

void dbus_request_sat(struct custom_data *ctx, TcorePlugin *plugin, int tapi_service_function, GArray* in_param1,
		GArray* in_param2, GArray* in_param3, GArray* in_param4, GArray** out_param1, GArray** out_param2,
		GArray** out_param3, GArray** out_param4, GError** error)
{
	TReturn ret;
	tapi_dbus_connection_name conn_name;
	UserRequest *ur = NULL;
	struct tcore_user_info ui = { 0, };
	TelSatSetupMenuInfo_t satMainMenu;

	int api_err = TAPI_API_SUCCESS;
/*	int security_result  = 0;*/
	int request_id = 0;

	conn_name = g_array_index(in_param4, tapi_dbus_connection_name, 0);
	ur = tcore_user_request_new(ctx->comm, tcore_plugin_get_description(plugin)->name);
	if (!ur) {
		api_err = TAPI_API_SERVER_FAILURE;
		goto OUT;
	}

/*	security_result = security_server_check_privilege(cookie, security_server_get_gid(SECURITY_GID_TEL_SAT));
	if (security_result < 0) {
		TS_DEBUG(LEVEL_DEBUG, "security failed. (gid_name='%s' retval=%d)", SECURITY_GID_TEL_SAT, security_result);
		api_err = TAPI_API_NOT_SUPPORTED;
		goto OUT;
	}*/

	ui.appname = conn_name.name;
	tcore_user_request_set_user_info(ur, &ui);

	switch (tapi_service_function) {
		case TAPI_CS_SIMATK_SEND_MENU_SELECTION_ENVELOPE: {
			TelSatMenuSelectionReqInfo_t* selected_data = NULL;
			struct treq_sat_envelop_cmd_data envelop_data;
			dbg("TAPI_CS_SIMATK_SEND_MENU_SELECTION_ENVELOPE");
			selected_data = &g_array_index(in_param1, TelSatMenuSelectionReqInfo_t, 0);
			if (!selected_data) {
				api_err = TAPI_API_OPERATION_FAILED;
				break;
			}
			memset(&envelop_data, 0, sizeof(struct treq_sat_envelop_cmd_data));
			envelop_data.sub_cmd = ENVELOP_MENU_SELECTION;
			envelop_data.envelop_data.menu_select.device_identitie.src = DEVICE_ID_KEYPAD;
			envelop_data.envelop_data.menu_select.device_identitie.dest = DEVICE_ID_SIM;
			envelop_data.envelop_data.menu_select.item_identifier.item_identifier =	selected_data->itemIdentifier;
			envelop_data.envelop_data.menu_select.help_request = ((selected_data->bIsHelpRequested == 1) ? TRUE : FALSE);

			tcore_user_request_set_data(ur, sizeof(struct treq_sat_envelop_cmd_data), (void *) &envelop_data);
			tcore_user_request_set_command(ur, TREQ_SAT_REQ_ENVELOPE);

			ret = tcore_communicator_dispatch_request(ctx->comm, ur);
			if (ret != TCORE_RETURN_SUCCESS)
				api_err = TAPI_API_OPERATION_FAILED;

			g_array_append_vals(*out_param2, &request_id, sizeof(int));
			dbg("ret = 0x%x", ret);
		}
			break;

		case TAPI_CS_SIMATK_SEND_EVENT_DOWNLOAD:
			tcore_user_request_set_data(ur, 0, NULL);
			tcore_user_request_set_command(ur, TREQ_SAT_REQ_ENVELOPE);
			ret = tcore_communicator_dispatch_request(ctx->comm, ur);
			if (ret != TCORE_RETURN_SUCCESS)
				api_err = TAPI_API_OPERATION_FAILED;

			g_array_append_vals(*out_param2, &request_id, sizeof(int));
			dbg("ret = 0x%x", ret);
			break;

		case TAPI_CS_UI_USER_CONFIRM: {
			TelSatUiUserConfirmInfo_t cnf;
			TelSatTextInfo_t* additional_data = NULL;

			memset(&cnf, 0x00, sizeof(TelSatUiUserConfirmInfo_t));
			cnf.commandId = g_array_index(in_param1,int, 0);
			cnf.commandType = g_array_index(in_param1,int, 1);
			cnf.keyType = g_array_index(in_param1, int, 2);
			cnf.dataLen = g_array_index(in_param1, int, 3);

			dbg("command_id = 0x%x", cnf.commandId);
			dbg("command_type = 0x%x", cnf.commandType);
			dbg("key_type = 0x%x", cnf.keyType);
			dbg("data_length = %d", cnf.dataLen);

			if (cnf.dataLen > 0) {
				additional_data = &g_array_index(in_param2, TelSatTextInfo_t, 0);
				dbg("tel_send_sat_ui_user_confirm :[%d]", additional_data->string[0]);
				api_err = sat_mgr_handle_user_confirm(ctx, plugin, conn_name.name, &cnf,	additional_data->string, additional_data->stringLen);
				if (api_err != TAPI_API_SUCCESS)
					dbg("return_value = %d", api_err);
			}
			else if (cnf.dataLen == 0) {
				api_err = sat_mgr_handle_user_confirm(ctx, plugin, conn_name.name, &cnf, NULL, 0);
				if (api_err != TAPI_API_SUCCESS)
					dbg("return_value = %d", api_err);
			}
			else if (cnf.dataLen < 0) {
				api_err = TAPI_API_SERVER_FAILURE;
			}
			if (api_err != TCORE_RETURN_SUCCESS) {
				api_err = TAPI_API_OPERATION_FAILED;
			}
			dbg("api_err = 0x%x", api_err);
		}
			break;

		case TAPI_CS_SIMATK_SEND_APP_EXEC_RESULT: {
			TelSatAppsRetInfo_t* app_req_info = NULL;
			dbg("TAPI_CS_SIMATK_SEND_APP_EXEC_RESULT");
			app_req_info = &g_array_index(in_param1, TelSatAppsRetInfo_t, 0);
			ret = sat_mgr_handle_app_exec_result(ctx, plugin, conn_name.name, app_req_info);
			if (ret != TCORE_RETURN_SUCCESS)
				api_err = TAPI_API_OPERATION_FAILED;

			dbg("ret = 0x%x", ret);
		}
			break;

		case TAPI_CS_SIMATK_GET_MAIN_MENU_INFO:
			dbg("TAPI_CS_SIMATK_GET_MAIN_MENU_INFO");
			if (ctx->pSatMainMenu != NULL && ctx->pSatMainMenu->satMainMenuNum > 0) {
				dbg("[SAT] pSatMainMenu->satMainMenuNum: [%d]", ctx->pSatMainMenu->satMainMenuNum);
				dbg("[SAT] pSatMainMenu->satiMainTitle [%s]", ctx->pSatMainMenu->satMainTitle);
				ctx->pSatMainMenu->bIsMainMenuPresent = TRUE;
				g_array_append_vals(*out_param2, ctx->pSatMainMenu, sizeof(TelSatSetupMenuInfo_t));
			}
			else {
				memset(&satMainMenu, 0x00, sizeof(TelSatSetupMenuInfo_t));
				satMainMenu.bIsMainMenuPresent = FALSE;
				g_array_append_vals(*out_param2, &satMainMenu, sizeof(TelSatSetupMenuInfo_t));
			}
			break;

		case TAPI_CS_UI_DISPLAY_STATUS:
		default:
			api_err = TAPI_API_NOT_SUPPORTED;
			break;
	}

OUT:
	if (api_err != TAPI_API_SUCCESS)
		tcore_user_request_free(ur);

	g_array_append_vals(*out_param1, &api_err, sizeof(int));
	g_array_append_vals(*out_param2, &request_id, sizeof(int));
}

TReturn dbus_response_sat(struct custom_data *ctx, UserRequest *ur, const char *appname,
		enum tcore_response_command command, unsigned int data_len, const void *data)
{
	dbg("command = 0x%x", command);

	switch (command) {
		default:
			break;
	}
	return TCORE_RETURN_SUCCESS;
}

TReturn dbus_notification_sat(struct custom_data *ctx, CoreObject *source,
		enum tcore_notification_command command, unsigned int data_len, const void *data)
{
	struct tnoti_sat_proactive_ind *ind = NULL;
	ind = (struct tnoti_sat_proactive_ind *)data;
	dbg("command = 0x%x", command);

	if (command == TNOTI_SAT_PROACTIVE_CMD) {
		switch (ind->cmd_type) {
			case SAT_PROATV_CMD_SETUP_MENU:
				sat_mgr_setup_menu_noti(ctx, (struct tel_sat_setup_menu_tlv*) &ind->proactive_ind_data.setup_menu);
				break;

			case SAT_PROATV_CMD_DISPLAY_TEXT:
				sat_mgr_display_text_noti(ctx, (struct tel_sat_display_text_tlv*) &ind->proactive_ind_data.display_text);
				break;

			case SAT_PROATV_CMD_SELECT_ITEM:
				sat_mgr_select_item_noti(ctx,	(struct tel_sat_select_item_tlv*) &ind->proactive_ind_data.select_item);
				break;

			case SAT_PROATV_CMD_GET_INKEY:
				sat_mgr_get_inkey_noti(ctx, (struct tel_sat_get_inkey_tlv*) &ind->proactive_ind_data.get_inkey);
				break;

			case SAT_PROATV_CMD_GET_INPUT:
				sat_mgr_get_input_noti(ctx, (struct tel_sat_get_input_tlv*) &ind->proactive_ind_data.get_input);
				break;

			default:
				dbg("not handled ind->cmd_type[0x%x]", ind->cmd_type);
				break;
		}
	}

	if (command == TNOTI_SAT_SESSION_END) {
		TelSatEndProactiveSessionIndInfo_t se;
		memset(&se, 0x00, sizeof(TelSatEndProactiveSessionIndInfo_t));
		se.commandType = TAPI_SAT_CMD_TYPE_END_PROACTIVE_SESSION;

		/*Command Queue Clear*/
		sat_mgr_init_cmd_queue(ctx);

		/*just send dummy data*/
		ts_delivery_event(ctx->EvtDeliveryHandle,
											TAPI_EVENT_CLASS_SAT,
											TAPI_EVENT_SAT_END_PROACTIVE_SESSION_IND,
											NULL,
											0,
											TAPI_API_SUCCESS,
											sizeof(TelSatEndProactiveSessionIndInfo_t),
											(void *)&se);
	}
	return TCORE_RETURN_SUCCESS;
}
