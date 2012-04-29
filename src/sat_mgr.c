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
#include <communicator.h>
#include <core_object.h>
#include <co_sat.h>
#include <type/sat.h>

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

#define SAT_DEF_CMD_Q_MAX 10
#define SAT_TIME_OUT 60000

static int _get_queue_size(struct custom_data *ctx)
{
	int temp;
	temp = (int)g_queue_get_length(&ctx->sat_q);
	dbg("[SAT]SAT Command Queue current Size [%d], MAX SIZE [%d]\n", temp,	SAT_DEF_CMD_Q_MAX);
	return temp;
}

static gboolean _enqueue_cmd(struct custom_data *ctx, struct sat_mgr_cmd_q *cmd_obj)
{
	struct sat_mgr_cmd_q* item = NULL;
	if (_get_queue_size(ctx) == (SAT_DEF_CMD_Q_MAX - 1)) {
		dbg("[SAT] FAILED TO ENQUEUE - QUEUE FULL!\n");
		return FALSE;
	}

	item = g_new(struct sat_mgr_cmd_q, 1);

	if (item == NULL) {
		dbg("[SAT] FAILED TO ALLOC QUEUE ITEM!\n");
		return FALSE;
	}

	memcpy((void*)item, cmd_obj, sizeof(struct sat_mgr_cmd_q));
	g_queue_push_tail(&ctx->sat_q, item);
	return TRUE;
}

static gboolean _dequeue_cmd_front(struct custom_data *ctx, struct sat_mgr_cmd_q *cmd_obj)
{
	struct sat_mgr_cmd_q *item = NULL;
	if (g_queue_is_empty(&ctx->sat_q))
		return FALSE;
	item = g_queue_pop_head(&ctx->sat_q);
	memcpy((void*)cmd_obj, item, sizeof(struct sat_mgr_cmd_q));
	g_free(item);
	return TRUE;
}

static gboolean _dequeue_cmd_front_by_id(struct custom_data *ctx, struct sat_mgr_cmd_q *cmd_obj)
{
	if (g_queue_is_empty(&ctx->sat_q))
		return FALSE;
	memcpy((void*)cmd_obj, g_queue_peek_head(&ctx->sat_q), sizeof(struct sat_mgr_cmd_q));
	return TRUE;
}

void sat_mgr_init_cmd_queue(struct custom_data *ctx)
{
	g_queue_init(&ctx->sat_q);
}

static gboolean sat_mgr_enqueue_cmd(struct custom_data *ctx, struct sat_mgr_cmd_q *cmd_obj)
{
	cmd_obj->cmd_id = g_queue_get_length(&ctx->sat_q);
	return _enqueue_cmd(ctx, cmd_obj);
}

static gboolean sat_mgr_dequeue_cmd(struct custom_data *ctx, struct sat_mgr_cmd_q *cmd_obj, unsigned short cmd_id)
{
	struct sat_mgr_cmd_q cmd_q_entry;
	int i;

	for (i = 0; i < _get_queue_size(ctx); i++) {
		if(FALSE == _dequeue_cmd_front_by_id(ctx, &cmd_q_entry))
			return FALSE;

		dbg("[SAT] command ID from Q [ %d], in param command id [%d]", cmd_q_entry.cmd_id, cmd_id);
		if (cmd_q_entry.cmd_id == cmd_id) {
			if(FALSE == _dequeue_cmd_front(ctx, cmd_obj))
				return FALSE;

			return TRUE;
		}
		else {
			if(FALSE == _dequeue_cmd_front(ctx, &cmd_q_entry))
				return FALSE;

			if(FALSE == _enqueue_cmd(ctx, &cmd_q_entry))
				return FALSE;
		}
	}
	return FALSE;
}

static gboolean sat_mgr_dequeue_cmd_front_by_id(struct custom_data *ctx, struct sat_mgr_cmd_q *cmd_obj)
{
	return _dequeue_cmd_front_by_id(ctx, cmd_obj);
}

static int _get_string_length(unsigned char *in, unsigned short in_max)
{
	int i = 0;
	if (in == NULL)
		return 0;
	/* 0xFF is the end of string */
	while (in[i] != 0xFF && i < in_max) {
		i++;
	}
	/* last space character must be deleted */
	while (in[i - 1] == 0x20 && i > 0) {
		i--;
	}
	return i;
}

static unsigned int _get_time_in_ms(struct tel_sat_duration *dr)
{
	switch (dr->time_unit) {
		case TIME_UNIT_MINUTES:
			return (unsigned int)dr->time_interval * 60000;
			break;

		case TIME_UNIT_SECONDS:
			return (unsigned int)dr->time_interval * 1000;
			break;

		case TIME_UNIT_TENTHS_OF_SECONDS:
			return (unsigned int)dr->time_interval * 100;
			break;

		case TIME_UNIT_RESERVED:
		default:
			return 0;	// set default
			break;
	}
}

static enum tel_sat_me_problem_type _get_tcore_me_problem_type(TelSatMeProblemType_t mp)
{
	switch (mp) {
		case TAPI_SAT_ME_PROBLEM_NO_SPECIFIC_CAUSE:
			return ME_PROBLEM_NO_SPECIFIC_CAUSE;
			break;
		case TAPI_SAT_ME_PROBLEM_SCREEN_BUSY:
			return ME_PROBLEM_SCREEN_BUSY;
			break;
		case TAPI_SAT_ME_PROBLEM_ME_BUSY_ON_CALL:
			return ME_PROBLEM_ME_BUSY_ON_CALL;
			break;
		case TAPI_SAT_ME_PROBLEM_ME_BUSY_ON_SS:
			return ME_PROBLEM_ME_BUSY_ON_SS;
			break;
		case TAPI_SAT_ME_PROBLEM_NO_SERVICE:
			return ME_PROBLEM_NO_SERVICE;
			break;
		case TAPI_SAT_ME_PROBLEM_ACCESS_CONTROL_CLASS_BAR:
			return ME_PROBLEM_ACCESS_CONTROL_CLASS_BAR;
			break;
		case TAPI_SAT_ME_PROBLEM_RADIO_RES_NOT_GRANTED:
			return ME_PROBLEM_RADIO_RES_NOT_GRANTED;
			break;
		case TAPI_SAT_ME_PROBLEM_NOT_IN_SPEECH_CALL:
			return ME_PROBLEM_NOT_IN_SPEECH_CALL;
			break;
		case TAPI_SAT_ME_PROBLEM_ME_BUSY_ON_USSD:
			return ME_PROBLEM_ME_BUSY_ON_USSD;
			break;
		case TAPI_SAT_ME_PROBLEM_ME_BUSY_ON_SEND_DTMF_CMD:
			return ME_PROBLEM_ME_BUSY_ON_SEND_DTMF_CMD;
			break;
		case TAPI_SAT_ME_PROBLEM_NO_USIM_ACTIVE:
			return ME_PROBLEM_NO_USIM_ACTIVE;
			break;
		default :
			dbg("not handled ME problem value[0x%x]", mp);
			return ME_PROBLEM_INVALID;
	}
}

static TapiResult_t sat_mgr_send_terminal_response(struct custom_data *ctx, TcorePlugin *p, char* conn_name, struct treq_sat_terminal_rsp_data *tr)
{
	struct tcore_user_info ui = { 0, };
	UserRequest *ur = NULL;
	TcorePlugin *np = NULL;
	TReturn rt = TCORE_RETURN_SUCCESS;

	if(!np)
		np = tcore_server_find_plugin(ctx->server, TCORE_PLUGIN_DEFAULT);
	else
		np = p;

	dbg("plugin address[0x%x], modem name[%s]", np, tcore_plugin_get_description(np)->name);
	ur = tcore_user_request_new(ctx->comm, tcore_plugin_get_description(np)->name);
	if (!ur) {
		rt = TAPI_API_SERVER_FAILURE;
		return rt;
	}

	if(conn_name)
		ui.appname = conn_name;

	tcore_user_request_set_user_info(ur, &ui);
	tcore_user_request_set_command(ur, TREQ_SAT_REQ_TERMINALRESPONSE);
	tcore_user_request_set_data(ur, sizeof(struct treq_sat_terminal_rsp_data), (void *)tr);
	rt = tcore_communicator_dispatch_request(ctx->comm, ur);
	if (rt != TCORE_RETURN_SUCCESS) {
		dbg("error rt[0x%x]",rt);
		rt = TAPI_API_OPERATION_FAILED;
	}
	else {
		rt = TAPI_API_SUCCESS;
	}
	return rt;
}

void sat_mgr_setup_menu_noti(struct custom_data *ctx, struct tel_sat_setup_menu_tlv* setup_menu_tlv)
{
	Server *s = NULL;
	Storage *strg = NULL;
	TcorePlugin *p = NULL;
	int i = 0;
	struct sat_mgr_cmd_q cmd_q;
	struct treq_sat_terminal_rsp_data *tr = NULL;

	p = tcore_server_find_plugin(ctx->server, TCORE_PLUGIN_DEFAULT);
	if (!p){
		dbg("there is no valid plugin at this point");
		return;
	}

	s = tcore_plugin_ref_server(p);
	if(!s){
		dbg("there is no valid server at this point");
		return;
	}

	strg = (Storage*)tcore_server_find_storage(s, "vconf");
	if(!strg){
		dbg("there is no valid storage plugin");
		return;
	}

	/*Initialize Data*/
	dbg("[SAT]  Set up Main Menu");
	if((setup_menu_tlv->icon_id.is_exist) &&( setup_menu_tlv->icon_id.icon_qualifer == ICON_QUALI_NOT_SELF_EXPLANATORY)
			&&((setup_menu_tlv->alpha_id.is_exist == FALSE)||setup_menu_tlv->alpha_id.alpha_data_len == 0))	{
		dbg( "[SAT]  exceptional case to fix gcf case 2.4 command not understood");
		tr = (struct treq_sat_terminal_rsp_data *)calloc(1, sizeof(struct treq_sat_terminal_rsp_data));
		tr->cmd_number = setup_menu_tlv->command_detail.cmd_num;
		tr->cmd_type = setup_menu_tlv->command_detail.cmd_type;
		memcpy((void*)&tr->terminal_rsp_data.setup_menu.command_detail, &setup_menu_tlv->command_detail, sizeof(struct tel_sat_cmd_detail_info));
		tr->terminal_rsp_data.setup_menu.device_id.src = setup_menu_tlv->device_id.dest;
		tr->terminal_rsp_data.setup_menu.device_id.dest = setup_menu_tlv->device_id.src;
		tr->terminal_rsp_data.setup_menu.result_type = RESULT_COMMAND_DATA_NOT_UNDERSTOOD_BY_ME;
		sat_mgr_send_terminal_response(ctx, NULL, NULL, tr);
		if (tr)
			free(tr);
		return;
	}

	if(ctx->pSatMainMenu != NULL) {
		dbg( "[SAT]  updated SETUP MENU");
		ctx->pSatMainMenu->bIsUpdatedSatMainMenu = TRUE;
	}
	else {
		dbg( "[SAT]  first SETUP MENU");
		ctx->pSatMainMenu = (TelSatSetupMenuInfo_t*)calloc(1, sizeof(TelSatSetupMenuInfo_t));
		if(ctx->pSatMainMenu == NULL)
			dbg("[SAT]  Memory Allocation for SAT Main Menu Information Variable Failed");
	}

	ctx->pSatMainMenu->satMainMenuNum = setup_menu_tlv->menu_item_cnt;

	/*One Item is mandatory*/
	if( ctx->pSatMainMenu->satMainMenuNum == 1 && setup_menu_tlv->menu_item[0].text_len == 0 ) {/* remove the sat menu */
		dbg("[SAT]  sat menu removed");
		ctx->pSatMainMenu->satMainMenuNum = 0;
	}
	else {
		switch (setup_menu_tlv->alpha_id.dcs.a_format) {
			case ALPHABET_FROMAT_SMS_DEFAULT :
				ctx->setup_menu_a_format = TAPI_SAT_ALPHABET_FORMAT_SMS_DEFAULT;
				break;

			case ALPHABET_FROMAT_8BIT_DATA :
				ctx->setup_menu_a_format = TAPI_SAT_ALPHABET_FORMAT_8BIT_DATA;
				break;

			case ALPHABET_FROMAT_UCS2 :
				ctx->setup_menu_a_format = TAPI_SAT_ALPHABET_FORMAT_UCS2;
				break;

			default :
				ctx->setup_menu_a_format = TAPI_SAT_ALPHABET_FORMAT_RESERVED;
				break;
		}

		if(ctx->pSatMainMenu->satMainMenuNum !=0) {
			if(setup_menu_tlv->alpha_id.alpha_data_len == 0) {
				/*Temporary Using*/
				dbg("error - NO MENU STRING");
			}
			else {
				/*	TODO DCS handling*/
				memcpy((void*)&ctx->pSatMainMenu->satMainTitle, setup_menu_tlv->alpha_id.alpha_data, setup_menu_tlv->alpha_id.alpha_data_len);
			}
			dbg("[SAT] satMainMenu.satMainTitle[%s]", ctx->pSatMainMenu->satMainTitle);

			if(setup_menu_tlv->next_act_ind_list.cnt == 0) {
				dbg( "[SAT]setup_menu_tlv->next_act_ind_list.cnt == 0");

				for(i = 0; i < ctx->pSatMainMenu->satMainMenuNum; i++) {
					dbg( "[SAT] ctx->pSatMainMenu->satMainMenuItem[i].itemId[%d]", setup_menu_tlv->menu_item[i].item_id);
					memset(ctx->pSatMainMenu->satMainMenuItem[i].itemString, 0x00, TAPI_SAT_DEF_ITEM_STR_LEN_MAX + 6);

					if(setup_menu_tlv->alpha_id.alpha_data_len == 0)
						setup_menu_tlv->alpha_id.dcs.a_format = ALPHABET_FROMAT_8BIT_DATA;
					/*	TODO DCS handling*/
					memcpy((void*)&ctx->pSatMainMenu->satMainMenuItem[i].itemString, setup_menu_tlv->menu_item[i].text, setup_menu_tlv->menu_item[i].text_len);

					ctx->pSatMainMenu->satMainMenuItem[i].itemId = setup_menu_tlv->menu_item[i].item_id;
					dbg("[SAT] item id[%d] satMainMenu.satMainMenuItem[i].itemString [%i] : [%s]",
							ctx->pSatMainMenu->satMainMenuItem[i].itemId , i,ctx->pSatMainMenu->satMainMenuItem[i].itemString);
				}
			}
			else {
				for(i = 0; i < ctx->pSatMainMenu->satMainMenuNum; i++) {
					memset(ctx->pSatMainMenu->satMainMenuItem[i].itemString, 0x00, TAPI_SAT_DEF_ITEM_STR_LEN_MAX + 6);

					if(setup_menu_tlv->alpha_id.alpha_data_len == 0)
						setup_menu_tlv->alpha_id.dcs.a_format = ALPHABET_FROMAT_8BIT_DATA;
					/*	TODO DCS handling*/
					memcpy((void*)&ctx->pSatMainMenu->satMainMenuItem[i].itemString, setup_menu_tlv->menu_item[i].text, setup_menu_tlv->menu_item[i].text_len);

					ctx->pSatMainMenu->satMainMenuItem[i].itemId = setup_menu_tlv->menu_item[i].item_id;
					dbg("[SAT] satMainMenu.satMainMenuItem[i].itemString %i : %s\n", i,ctx->pSatMainMenu->satMainMenuItem[i].itemString);

					if( setup_menu_tlv->next_act_ind_list.indicator_list[i]== TAPI_SAT_CMD_TYPE_SEND_SMS) {
						strncat((char *)ctx->pSatMainMenu->satMainMenuItem[i].itemString," [Send SMS]", 11);
					}
					else if (setup_menu_tlv->next_act_ind_list.indicator_list[i]== TAPI_SAT_CMD_TYPE_SETUP_CALL) {
						strncat((char *)ctx->pSatMainMenu->satMainMenuItem[i].itemString," [Set Up Call]", 14);
					}
					else if (setup_menu_tlv->next_act_ind_list.indicator_list[i]== TAPI_SAT_CMD_TYPE_LAUNCH_BROWSER){
						strncat((char *)ctx->pSatMainMenu->satMainMenuItem[i].itemString," [Launch Browser]", 17);
					}
					else if (setup_menu_tlv->next_act_ind_list.indicator_list[i]== TAPI_SAT_CMD_TYPE_PROVIDE_LOCAL_INFO)	{
						strncat((char *)ctx->pSatMainMenu->satMainMenuItem[i].itemString," [Provide Terminal Information]", 31);
					}
					else {
						dbg("ItemNxtAction Ind=0x%x",setup_menu_tlv->next_act_ind_list.indicator_list[i]);
					}
				}
			}
			ctx->pSatMainMenu->bIsSatMainMenuHelpInfo = setup_menu_tlv->command_detail.cmd_qualifier.setup_menu.help_info;
		}
		else {
			dbg("error - there is no menu item");
			return ;
		}
	}

	/*Enqueue Data and Generate cmd_id*/
	memset(&cmd_q, 0x00, sizeof(struct sat_mgr_cmd_q));
	cmd_q.cmd_type = TAPI_SAT_CMD_TYPE_SETUP_MENU;
	memcpy((void*)&(cmd_q.cmd_data.setupMenuInd), setup_menu_tlv, sizeof(struct tel_sat_setup_menu_tlv));
	sat_mgr_enqueue_cmd(ctx, &cmd_q);
	ctx->pSatMainMenu->commandId = cmd_q.cmd_id;

/*
	setup menu proactive cmd can occur after boot time.
	so in that case, sat-ui should refresh about sat main menu list. for that, we publish event with main menu data.
*/
	ts_delivery_event(ctx->EvtDeliveryHandle,
										TAPI_EVENT_CLASS_SAT,
										TAPI_EVENT_SAT_SETUP_MENU_IND,
										NULL,
										0,
										TAPI_API_SUCCESS,
										sizeof(TelSatSetupMenuInfo_t),
										(void *)ctx->pSatMainMenu);

	tcore_storage_set_int(strg, STORAGE_KEY_TELEPHONY_SAT_STATE, STORAGE_VALUE_ON);
}

void sat_mgr_display_text_noti(struct custom_data *ctx,	struct tel_sat_display_text_tlv* display_text_tlv)
{
	TelSatDisplayTextInd_t* ad = NULL;
	struct sat_mgr_cmd_q cmd_q;
	unsigned int timer_duration = 0;
	struct treq_sat_terminal_rsp_data *tr = NULL;

	if ((display_text_tlv->icon_id.is_exist) && (display_text_tlv->icon_id.icon_qualifer == ICON_QUALI_NOT_SELF_EXPLANATORY)
			&& (display_text_tlv->text.string_length == 0)) {
		dbg("[SAT]  exceptional case to fix gcf case 2.4 command not understood");
		tr = (struct treq_sat_terminal_rsp_data *)calloc(1, sizeof(struct treq_sat_terminal_rsp_data));
		tr->cmd_number = display_text_tlv->command_detail.cmd_num;
		tr->cmd_type = display_text_tlv->command_detail.cmd_type;
		memcpy((void*)&tr->terminal_rsp_data.display_text.command_detail, &display_text_tlv->command_detail, sizeof(struct tel_sat_cmd_detail_info));
		tr->terminal_rsp_data.display_text.device_id.src = display_text_tlv->device_id.dest;
		tr->terminal_rsp_data.display_text.device_id.dest = display_text_tlv->device_id.src;
		tr->terminal_rsp_data.display_text.result_type = RESULT_COMMAND_DATA_NOT_UNDERSTOOD_BY_ME;
		sat_mgr_send_terminal_response(ctx, NULL, NULL, tr);
		if (tr)
			free(tr);
		return;
	}

	ad = calloc(1, sizeof(TelSatDisplayTextInd_t));
	if (!ad) {
		dbg( "[SAT] ad malloc failed");
		return;
	}

	dbg( "[SAT] immediate response requested is [%d]", display_text_tlv->immediate_response_requested);

	if (display_text_tlv->immediate_response_requested == FALSE)
		ad->bIsUserRespRequired = TRUE;

	ad->bIsPriorityHigh = FALSE;
	if (display_text_tlv->command_detail.cmd_qualifier.display_text.text_priority == TEXT_PRIORITY_HIGH)
		ad->bIsPriorityHigh = TRUE;

	/*1.Text Data*/
	/*	TODO DCS handling*/
	ad->text.stringLen = display_text_tlv->text.string_length;
	memcpy((void*)&ad->text.string, display_text_tlv->text.string, ad->text.stringLen);
	ad->text.string[ad->text.stringLen] = '\0';

	/*2.Icon*/
	/*	TODO icon handling*/

	/*3.Display Duration*/
	/*default duration value is zero - sustain the text*/
	timer_duration = _get_time_in_ms(&display_text_tlv->duration);
	if (display_text_tlv->command_detail.cmd_qualifier.display_text.text_clear_type	== TEXT_AUTO_CLEAR_MSG_AFTER_A_DELAY) {
		if (timer_duration > 0)
			ad->duration = timer_duration;
		else
			ad->duration = 15000; // 15 sec	- default.
	}
	else {
		if (display_text_tlv->immediate_response_requested == FALSE) {
			if (timer_duration > 0)
				ad->duration = timer_duration;
			else
				ad->duration = SAT_TIME_OUT; // 1 min	- default no resp time.
		}
	}

	/* ETSI TS 102 223 6.4.1 DISPLAY TEXT
	 If help information is requested by the user, this command may be used to display help information on the screen. The
	 help information should be sent as high priority text and with the option that it should be cleared after a short delay.
	 */
	if (ctx->help_requested == TRUE) {
		ad->bIsPriorityHigh = TRUE;
		ad->duration = 7000;
		ctx->help_requested = FALSE;
	}

	/*Enqueue Data and Generate commandId*/
	memset(&cmd_q, 0x00, sizeof(struct sat_mgr_cmd_q));
	cmd_q.cmd_type = TAPI_SAT_CMD_TYPE_DISPLAY_TEXT;
	memcpy((void*)&(cmd_q.cmd_data.displayTextInd), display_text_tlv, sizeof(struct tel_sat_display_text_tlv));
	sat_mgr_enqueue_cmd(ctx, &cmd_q);
	ad->commandId = cmd_q.cmd_id;

	ts_delivery_event(ctx->EvtDeliveryHandle,
										TAPI_EVENT_CLASS_SAT,
										TAPI_EVENT_SAT_DISPLAY_TEXT_IND,
										NULL,
										0,
										TAPI_API_SUCCESS,
										sizeof(TelSatDisplayTextInd_t),
										(void *)ad);
	if (ad)
		free(ad);
}

void sat_mgr_select_item_noti(struct custom_data *ctx, struct tel_sat_select_item_tlv* select_item_tlv)
{
	TelSatSelectItemInd_t* ad = NULL;
	struct sat_mgr_cmd_q cmd_q;
	struct treq_sat_terminal_rsp_data *tr = NULL;
	int i = 0, tmp_len = 0;

	if ((select_item_tlv->icon_id.is_exist)	&& (select_item_tlv->icon_id.icon_qualifer == ICON_QUALI_NOT_SELF_EXPLANATORY)
			&& ((select_item_tlv->alpha_id.is_exist == FALSE) || select_item_tlv->alpha_id.alpha_data_len == 0)) {
		dbg("[SAT]  exceptional case to fix gcf case 2.4 command not understood");
		tr = (struct treq_sat_terminal_rsp_data *)calloc(1, sizeof(struct treq_sat_terminal_rsp_data));
		tr->cmd_number = select_item_tlv->command_detail.cmd_num;
		tr->cmd_type = select_item_tlv->command_detail.cmd_type;
		memcpy((void*)&tr->terminal_rsp_data.select_item.command_detail, &select_item_tlv->command_detail, sizeof(struct tel_sat_cmd_detail_info));
		tr->terminal_rsp_data.select_item.device_id.src = select_item_tlv->device_id.dest;
		tr->terminal_rsp_data.select_item.device_id.dest = select_item_tlv->device_id.src;
		tr->terminal_rsp_data.select_item.result_type = RESULT_COMMAND_DATA_NOT_UNDERSTOOD_BY_ME;
		sat_mgr_send_terminal_response(ctx, NULL, NULL, tr);
		if (tr)
			free(tr);
		return;
	}

	ad = calloc(1, sizeof(TelSatSelectItemInd_t));
	if (!ad) {
		dbg( "[SAT]  ad malloc failed");
		return;
	}

	/*Enqueue Data and Generate commandId*/
	memset(&cmd_q, 0x00, sizeof(struct sat_mgr_cmd_q));
	cmd_q.cmd_type = TAPI_SAT_CMD_TYPE_SELECT_ITEM;
	memcpy((void*)&(cmd_q.cmd_data.selectItemInd), select_item_tlv, sizeof(struct tel_sat_select_item_tlv));
	sat_mgr_enqueue_cmd(ctx, &cmd_q);
	ad->commandId = cmd_q.cmd_id;

	dbg( "[SAT]  commandId in Queue[%d]", cmd_q.cmd_id);

	/*Help Info*/
	ad->bIsHelpInfoAvailable = select_item_tlv->command_detail.cmd_qualifier.select_item.help_info;

	if (select_item_tlv->alpha_id.is_exist == TRUE) {
		/*	TODO DCS handling*/
		ad->text.stringLen = select_item_tlv->alpha_id.alpha_data_len;
		memcpy((void*) ad->text.string, select_item_tlv->alpha_id.alpha_data, ad->text.stringLen);
	}

	for (i = 0; i < select_item_tlv->menu_item_cnt; i++) {
		ad->menuItem[i].itemId = select_item_tlv->menu_item[i].item_id;
		if (ctx->setup_menu_a_format == TAPI_SAT_ALPHABET_FORMAT_UCS2) {
			dbg("DCS is UCS2");
			/*	TODO DCS handling*/
		}
		else {
			/*	TODO DCS handling*/
			ad->menuItem[i].textLen = _get_string_length(select_item_tlv->menu_item[i].text, TAPI_SAT_ITEM_TEXT_LEN_MAX);
			memcpy((void*)ad->menuItem[i].text, select_item_tlv->menu_item[i].text,	ad->menuItem[i].textLen);
		}
		dbg( "[SAT] Item[%d],text[%s]", i, ad->menuItem[i].text);
	}

	if (select_item_tlv->item_next_act_ind_list.cnt == 0) {
		for (i = 0; i < select_item_tlv->menu_item_cnt; i++) {
			ad->menuItem[i].itemId = select_item_tlv->menu_item[i].item_id;
			ad->menuItem[i].textLen = _get_string_length(select_item_tlv->menu_item[i].text, TAPI_SAT_ITEM_TEXT_LEN_MAX);
			dbg( "[SAT] Item [%d]:text=[%s]", i, ad->menuItem[i].text);
		}
	}
	else {
		for (i = 0; i < select_item_tlv->menu_item_cnt; i++) {
			ad->menuItem[i].itemId = select_item_tlv->menu_item[i].item_id;
			memset(ad->menuItem[i].text, 0x00, (TAPI_SAT_ITEM_TEXT_LEN_MAX + 1));
			tmp_len = _get_string_length(select_item_tlv->menu_item[i].text, (TAPI_SAT_ITEM_TEXT_LEN_MAX + 1));
			memcpy((void*)ad->menuItem[i].text, select_item_tlv->menu_item[i].text, tmp_len);

			if (select_item_tlv->item_next_act_ind_list.indicator_list[i] == TAPI_SAT_CMD_TYPE_SEND_SMS) {
				strncat((char *) ad->menuItem[i].text, " [Send SMS]", 11);
			}
			else if (select_item_tlv->item_next_act_ind_list.indicator_list[i] == TAPI_SAT_CMD_TYPE_SETUP_CALL) {
				strncat((char *) ad->menuItem[i].text, " [Set Up Call]", 14);
			}
			else if (select_item_tlv->item_next_act_ind_list.indicator_list[i] == TAPI_SAT_CMD_TYPE_LAUNCH_BROWSER) {
				strncat((char *) ad->menuItem[i].text, " [Launch Browser]", 17);
			}
			else if (select_item_tlv->item_next_act_ind_list.indicator_list[i] == TAPI_SAT_CMD_TYPE_PROVIDE_LOCAL_INFO) {
				strncat((char *) ad->menuItem[i].text, " [Provide Terminal Information]", 31);
			}
			else {
				dbg( "ItemNxtAction Ind=[0x%x]",	select_item_tlv->item_next_act_ind_list.indicator_list[i]);
			}
			ad->menuItem[i].textLen = strlen((char *) ad->menuItem[i].text);
		}
	}

	ad->menuItemCount = select_item_tlv->menu_item_cnt;
	dbg( "[SAT] ad->menuItemCount[%d]", ad->menuItemCount);

	/*Default item*/
	ad->defaultItemIndex = select_item_tlv->item_identifier.item_identifier;
	dbg( "[SAT] ad->defaultItemIndex[%d]", ad->defaultItemIndex);

	ts_delivery_event(ctx->EvtDeliveryHandle,
										TAPI_EVENT_CLASS_SAT,
										TAPI_EVENT_SAT_SELECT_ITEM_IND,
										NULL,
										0,
										TAPI_API_SUCCESS,
										sizeof(TelSatSelectItemInd_t),
										(void *)ad);
	if (ad)
		free(ad);
}

void sat_mgr_get_inkey_noti(struct custom_data *ctx, struct tel_sat_get_inkey_tlv* get_inkey_tlv)
{
	TelSatGetInkeyInd_t* ad = NULL;
	struct sat_mgr_cmd_q cmd_q;
	unsigned int timer_duration = 0;
	struct treq_sat_terminal_rsp_data *tr = NULL;

	if ((get_inkey_tlv->icon_id.is_exist)
			&& (get_inkey_tlv->icon_id.icon_qualifer == ICON_QUALI_NOT_SELF_EXPLANATORY)
			&& (get_inkey_tlv->text.string_length == 0)) {
		dbg("[SAT]  exceptional case to fix gcf case 2.4 command not understood");
		tr = (struct treq_sat_terminal_rsp_data *)calloc(1, sizeof(struct treq_sat_terminal_rsp_data));
		tr->cmd_number = get_inkey_tlv->command_detail.cmd_num;
		tr->cmd_type = get_inkey_tlv->command_detail.cmd_type;
		memcpy((void*)&tr->terminal_rsp_data.get_inkey.command_detail, &get_inkey_tlv->command_detail, sizeof(struct tel_sat_cmd_detail_info));
		tr->terminal_rsp_data.get_inkey.device_id.src = get_inkey_tlv->device_id.dest;
		tr->terminal_rsp_data.get_inkey.device_id.dest = get_inkey_tlv->device_id.src;
		tr->terminal_rsp_data.get_inkey.result_type = RESULT_COMMAND_DATA_NOT_UNDERSTOOD_BY_ME;
		sat_mgr_send_terminal_response(ctx, NULL, NULL, tr);
	}

	ad = calloc(1, sizeof(TelSatGetInkeyInd_t));
	if (!ad) {
		dbg( "[SAT]  ad malloc failed");
		return;
	}
	/*	 icon */
	/*	TODO icon handling*/

	/* duration*/
	timer_duration = _get_time_in_ms(&get_inkey_tlv->duration);
	if (timer_duration > 0)
		ad->duration = timer_duration;
	else
		ad->duration = SAT_TIME_OUT; // 15 sec	- default.

	/*key type*/
	if (get_inkey_tlv->command_detail.cmd_qualifier.get_inkey.inkey_type == INKEY_TYPE_CHARACTER_SET_ENABLED)
		ad->keyType = TAPI_SAT_INKEY_TYPE_CHARACTER_SET_ENABLED;
	else
		ad->keyType = TAPI_SAT_INKEY_TYPE_YES_NO_REQUESTED;

	/*input mode -if input char is numeric, no need to check input mode*/
	if (get_inkey_tlv->command_detail.cmd_qualifier.get_inkey.alphabet_type	== INPUT_ALPHABET_TYPE_SMS_DEFAULT)
		ad->inputCharMode = TAPI_SAT_USER_INPUT_ALPHABET_TYPE_SMS_DEFAULT;
	else
		ad->inputCharMode = TAPI_SAT_USER_INPUT_ALPHABET_TYPE_UCS2;

	/*Numeric*/
	ad->bIsNumeric = (!get_inkey_tlv->command_detail.cmd_qualifier.get_inkey.alphabet_set);
	/*help information*/
	ad->bIsHelpInfoAvailable =get_inkey_tlv->command_detail.cmd_qualifier.get_inkey.help_info;
	/*text*/
	/*	TODO DCS handling*/
	ad->text.stringLen = get_inkey_tlv->text.string_length;
	memcpy((void*)ad->text.string, get_inkey_tlv->text.string, ad->text.stringLen);
	ad->text.string[ad->text.stringLen] = '\0';
	dbg("[SAT] ad->text.string[%s]", ad->text.string);

	if (get_inkey_tlv->command_detail.cmd_qualifier.get_inkey.immediate_rsp_required) {
		dbg("get_inkey_tlv->command_detail.cmd_qualifier.get_inkey.immediate_rsp_require is TRUE");
		//Send TR if UI display success
	}

	/*Enqueue Data and Generate commandId*/
	memset(&cmd_q, 0x00, sizeof(struct sat_mgr_cmd_q));
	cmd_q.cmd_type = TAPI_SAT_CMD_TYPE_GET_INKEY;
	memcpy((void*)&(cmd_q.cmd_data.getInkeyInd), get_inkey_tlv, sizeof(struct tel_sat_get_inkey_tlv));
	sat_mgr_enqueue_cmd(ctx, &cmd_q);
	ad->commandId = cmd_q.cmd_id;

	ts_delivery_event(ctx->EvtDeliveryHandle,
										TAPI_EVENT_CLASS_SAT,
										TAPI_EVENT_SAT_GET_INKEY_IND,
										NULL,
										0,
										TAPI_API_SUCCESS,
										sizeof(TelSatGetInkeyInd_t),
										(void *)ad);
	if (ad)
		free(ad);
}

void sat_mgr_get_input_noti(struct custom_data *ctx, struct tel_sat_get_input_tlv* get_input_tlv)
{
	TelSatGetInputInd_t* ad = NULL;
	struct sat_mgr_cmd_q cmd_q;
	struct treq_sat_terminal_rsp_data *tr = NULL;

	if ((get_input_tlv->icon_id.is_exist)
			&& (get_input_tlv->icon_id.icon_qualifer == ICON_QUALI_NOT_SELF_EXPLANATORY)
			&& (get_input_tlv->text.string_length == 0)) {
		dbg("[SAT]  exceptional case to fix gcf case 2.4 command not understood");
		tr = (struct treq_sat_terminal_rsp_data *)calloc(1, sizeof(struct treq_sat_terminal_rsp_data));
		tr->cmd_number = get_input_tlv->command_detail.cmd_num;
		tr->cmd_type = get_input_tlv->command_detail.cmd_type;
		memcpy((void*)&tr->terminal_rsp_data.get_input.command_detail, &get_input_tlv->command_detail, sizeof(struct tel_sat_cmd_detail_info));
		tr->terminal_rsp_data.get_input.device_id.src = get_input_tlv->device_id.dest;
		tr->terminal_rsp_data.get_input.device_id.dest = get_input_tlv->device_id.src;
		tr->terminal_rsp_data.get_input.result_type = RESULT_COMMAND_DATA_NOT_UNDERSTOOD_BY_ME;
		sat_mgr_send_terminal_response(ctx, NULL, NULL, tr);
	}

	ad = calloc(1, sizeof(TelSatGetInputInd_t));
	if (!ad) {
		dbg( "[SAT]  ad malloc failed");
		return;
	}

	dbg( "[SAT]  is SMS7 packing required [%d]",get_input_tlv->command_detail.cmd_qualifier.get_input.user_input_unpacked_format);

	/* icon*/

	/*input mode*/
	if (get_input_tlv->command_detail.cmd_qualifier.get_input.alphabet_type	== INPUT_ALPHABET_TYPE_SMS_DEFAULT)
		ad->inputCharMode = TAPI_SAT_USER_INPUT_ALPHABET_TYPE_SMS_DEFAULT;
	else
		ad->inputCharMode = TAPI_SAT_USER_INPUT_ALPHABET_TYPE_UCS2;
	/*Is input Numeric?*/
	ad->bIsNumeric = (!get_input_tlv->command_detail.cmd_qualifier.get_input.alphabet_set);
	/*help information*/
	ad->bIsHelpInfoAvailable = get_input_tlv->command_detail.cmd_qualifier.get_input.help_info;
	/*echo input*/
	ad->bIsEchoInput = get_input_tlv->command_detail.cmd_qualifier.get_input.me_echo_user_input;

	/*text*/
	/*	TODO DCS handling*/
	ad->text.stringLen = get_input_tlv->text.string_length;
	memcpy((void*)ad->text.string, get_input_tlv->text.string, ad->text.stringLen);
	ad->text.string[ad->text.stringLen] = '\0';

	/*default text*/
	/*	TODO DCS handling*/
	ad->defaultText.stringLen = get_input_tlv->default_text.string_length;
	memcpy((void*)ad->defaultText.string, get_input_tlv->default_text.string, ad->defaultText.stringLen);
	ad->defaultText.string[ad->defaultText.stringLen] = '\0';

	/*min, max value*/
	ad->respLen.max = get_input_tlv->rsp_len.max;
	ad->respLen.min = get_input_tlv->rsp_len.min;

	/*Enqueue Data and Generate commandId*/
	memset(&cmd_q, 0x00, sizeof(struct sat_mgr_cmd_q));
	cmd_q.cmd_type = TAPI_SAT_CMD_TYPE_GET_INPUT;
	memcpy((void*)&(cmd_q.cmd_data.getInputInd), get_input_tlv, sizeof(struct tel_sat_get_input_tlv));
	sat_mgr_enqueue_cmd(ctx, &cmd_q);
	ad->commandId = cmd_q.cmd_id;

	ts_delivery_event(ctx->EvtDeliveryHandle,
										TAPI_EVENT_CLASS_SAT,
										TAPI_EVENT_SAT_GET_INPUT_IND,
										NULL,
										0,
										TAPI_API_SUCCESS,
										sizeof(TelSatGetInputInd_t),
										(void *)ad);
	if (ad)
		free(ad);
}

static TapiResult_t _sat_mgr_app_exec_result_display_text(struct custom_data *ctx, TcorePlugin *p, char *conn_name, TelSatDiplayTextRetInfo_t* ret_status, int cmd_id)
{
	struct treq_sat_terminal_rsp_data tr;
	struct sat_mgr_cmd_q cmd_q;

	memset(&tr, 0x00, sizeof(struct treq_sat_terminal_rsp_data));
	memset(&cmd_q, 0x00, sizeof(struct sat_mgr_cmd_q));

	if (sat_mgr_dequeue_cmd(ctx, &cmd_q, cmd_id) == FALSE) {
		dbg("[SAT] command dequeue failed. didn't find in command Q!!");
		return TAPI_API_SAT_INVALID_COMMAND_ID;
	}

	tr.cmd_number = cmd_q.cmd_data.displayTextInd.command_detail.cmd_num;
	tr.cmd_type = cmd_q.cmd_data.displayTextInd.command_detail.cmd_type;
	memcpy((void*)&tr.terminal_rsp_data.display_text.command_detail, &cmd_q.cmd_data.displayTextInd.command_detail, sizeof(struct tel_sat_cmd_detail_info));
	tr.terminal_rsp_data.display_text.device_id.src = cmd_q.cmd_data.displayTextInd.device_id.dest;
	tr.terminal_rsp_data.display_text.device_id.dest = cmd_q.cmd_data.displayTextInd.device_id.src;

	switch (ret_status->resp) {
		case TAPI_SAT_R_SUCCESS:
			if (cmd_q.cmd_data.displayTextInd.icon_id.icon_info.ics == IMAGE_CODING_SCHEME_COLOUR)
				tr.terminal_rsp_data.display_text.result_type = RESULT_SUCCESS_BUT_REQUESTED_ICON_NOT_DISPLAYED;
			else
				tr.terminal_rsp_data.display_text.result_type = RESULT_SUCCESS;

			tr.terminal_rsp_data.display_text.me_problem_type = ME_PROBLEM_NO_SPECIFIC_CAUSE;
			break;

		case TAPI_SAT_R_ME_UNABLE_TO_PROCESS_COMMAND:
			tr.terminal_rsp_data.display_text.result_type = RESULT_ME_UNABLE_TO_PROCESS_COMMAND;
			tr.terminal_rsp_data.display_text.me_problem_type = _get_tcore_me_problem_type(ret_status->meProblem);
			break;

		default:
			dbg("[SAT] Wrong result from  app ret_status->resp[%d]", ret_status->resp);
			break;
	}
	return sat_mgr_send_terminal_response(ctx, p, conn_name, &tr);
}

static TapiResult_t _sat_mgr_app_exec_result_setup_menu(struct custom_data *ctx, TcorePlugin *p, char *conn_name, TelSatSetupMenuRetInfo_t* ret_status, int cmd_id)
{
	struct treq_sat_terminal_rsp_data tr;
	struct sat_mgr_cmd_q cmd_q;

	dbg("[SAT]cmd_id[%d]", cmd_id);

	memset(&tr, 0x00, sizeof(struct treq_sat_terminal_rsp_data));
	memset(&cmd_q, 0x00, sizeof(struct sat_mgr_cmd_q));

	if (sat_mgr_dequeue_cmd(ctx, &cmd_q, cmd_id) == FALSE) {
		dbg("[SAT] command dequeue failed. didn't find in command Q!!");
		return TAPI_API_SAT_INVALID_COMMAND_ID;
	}

	tr.cmd_number = cmd_q.cmd_data.setupMenuInd.command_detail.cmd_num;
	tr.cmd_type = cmd_q.cmd_data.setupMenuInd.command_detail.cmd_type;
	memcpy((void*)&tr.terminal_rsp_data.setup_menu.command_detail, &cmd_q.cmd_data.setupMenuInd.command_detail, sizeof(struct tel_sat_cmd_detail_info));
	tr.terminal_rsp_data.setup_menu.device_id.src = cmd_q.cmd_data.setupMenuInd.device_id.dest;
	tr.terminal_rsp_data.setup_menu.device_id.dest = cmd_q.cmd_data.setupMenuInd.device_id.src;

	switch (ret_status->resp) {
		case TAPI_SAT_R_SUCCESS:
			if (cmd_q.cmd_data.setupMenuInd.icon_id.icon_info.ics == IMAGE_CODING_SCHEME_COLOUR)
				tr.terminal_rsp_data.setup_menu.result_type = RESULT_SUCCESS_BUT_REQUESTED_ICON_NOT_DISPLAYED;
			else
				tr.terminal_rsp_data.setup_menu.result_type = RESULT_SUCCESS;

			tr.terminal_rsp_data.setup_menu.me_problem_type = ME_PROBLEM_NO_SPECIFIC_CAUSE;
			break;

		case TAPI_SAT_R_ME_UNABLE_TO_PROCESS_COMMAND:
			tr.terminal_rsp_data.setup_menu.result_type = RESULT_ME_UNABLE_TO_PROCESS_COMMAND;
			tr.terminal_rsp_data.setup_menu.me_problem_type = ME_PROBLEM_NO_SPECIFIC_CAUSE;
			break;

		default:
			dbg("[SAT] Wrong result from app ret_status->resp=[%d]", ret_status->resp);
			break;
	}
	return sat_mgr_send_terminal_response(ctx, p, conn_name, &tr);
}

TapiResult_t sat_mgr_handle_app_exec_result(struct custom_data *ctx, TcorePlugin *p, char *conn_name, TelSatAppsRetInfo_t* app_result)
{
	TapiResult_t rt = TAPI_API_SUCCESS;
	dbg("[SAT] Enter app_result->Command Type =[0x%x] ID=[0x%x]", app_result->commandType, app_result->commandId);

	switch (app_result->commandType) {
		case TAPI_SAT_CMD_TYPE_SETUP_MENU:
			rt = _sat_mgr_app_exec_result_setup_menu(ctx, p, conn_name, &(app_result->appsRet.setupMenu), app_result->commandId);
			break;

		case TAPI_SAT_CMD_TYPE_DISPLAY_TEXT:
			rt = _sat_mgr_app_exec_result_display_text(ctx, p, conn_name, &(app_result->appsRet.displayText), app_result->commandId);
			break;

		default:
			dbg("[SAT] Invalid app_result->commandType =[%d]", app_result->commandType);
			rt = TAPI_API_SERVER_FAILURE;
			break;
	}
	return rt;
}

static TapiResult_t _user_confirm_menu_select(struct custom_data *ctx, TcorePlugin *p, char *conn_name, TelSatUiUserConfirmInfo_t *cnf, unsigned char*additional_data, unsigned short ad_data_length)
{
	struct sat_mgr_cmd_q cmd_q;
	struct treq_sat_terminal_rsp_data tr;

	memset(&cmd_q, 0x00, sizeof(struct sat_mgr_cmd_q));
	memset(&tr, 0x00, sizeof(struct treq_sat_terminal_rsp_data));

	if (!sat_mgr_dequeue_cmd(ctx, &cmd_q, cnf->commandId)) {
		dbg("[SAT] command dequeue failed. didn't find in command Q!!");
		return TAPI_API_SAT_INVALID_COMMAND_ID;
	}

	tr.cmd_number = cmd_q.cmd_data.selectItemInd.command_detail.cmd_num;
	tr.cmd_type = cmd_q.cmd_data.selectItemInd.command_detail.cmd_type;
	memcpy((void*)&tr.terminal_rsp_data.select_item.command_detail, &cmd_q.cmd_data.selectItemInd.command_detail, sizeof(struct tel_sat_cmd_detail_info));
	tr.terminal_rsp_data.select_item.device_id.src = DEVICE_ID_ME;
	tr.terminal_rsp_data.select_item.device_id.dest = DEVICE_ID_SIM;

	switch (cnf->keyType) {
		case TAPI_SAT_USER_CONFIRM_YES:
			tr.terminal_rsp_data.select_item.item_identifier.item_identifier = additional_data[0];
			dbg("[SAT] additional_data[0]= %d", additional_data[0]);
			tr.terminal_rsp_data.select_item.other_info = FALSE;

			if (cmd_q.cmd_data.selectItemInd.icon_id.icon_info.ics == IMAGE_CODING_SCHEME_COLOUR)
				tr.terminal_rsp_data.select_item.result_type = RESULT_SUCCESS_BUT_REQUESTED_ICON_NOT_DISPLAYED;
			else
				tr.terminal_rsp_data.select_item.result_type = RESULT_SUCCESS;

			tr.terminal_rsp_data.select_item.me_problem_type = ME_PROBLEM_NO_SPECIFIC_CAUSE;
			break;

		case TAPI_SAT_USER_CONFIRM_HELP_INFO:
			tr.terminal_rsp_data.select_item.item_identifier.item_identifier = additional_data[0];
			tr.terminal_rsp_data.select_item.other_info = FALSE;
			tr.terminal_rsp_data.select_item.result_type = RESULT_HELP_INFO_REQUIRED_BY_USER;
			tr.terminal_rsp_data.select_item.me_problem_type = ME_PROBLEM_NO_SPECIFIC_CAUSE;
			ctx->help_requested = TRUE;
			break;

		case TAPI_SAT_USER_CONFIRM_END:
			tr.terminal_rsp_data.select_item.result_type = RESULT_PROACTIVE_SESSION_TERMINATED_BY_USER;
			break;

		case TAPI_SAT_USER_CONFIRM_NO_OR_CANCEL:
			tr.terminal_rsp_data.select_item.result_type = RESULT_BACKWARD_MOVE_BY_USER;
			break;

		case TAPI_SAT_USER_CONFIRM_TIMEOUT:
			tr.terminal_rsp_data.select_item.result_type = RESULT_NO_RESPONSE_FROM_USER;
			break;

		default:
			dbg("not handled value[%d] here", cnf->keyType);
			break;
	}
	dbg("[SAT] generalResult = [x%x]", tr.terminal_rsp_data.select_item.result_type);
	dbg("tr.terminal_rsp_data.select_item.item_identifier.item_identifier=%d",tr.terminal_rsp_data.select_item.item_identifier.item_identifier);
	return sat_mgr_send_terminal_response(ctx, p, conn_name, &tr);
}

static TapiResult_t _user_confirm_display_text(struct custom_data *ctx, TcorePlugin *p, char *conn_name, TelSatUiUserConfirmInfo_t *cnf, unsigned char*additional_data, unsigned short ad_data_length)
{
	struct sat_mgr_cmd_q cmd_q;
	struct treq_sat_terminal_rsp_data tr;

	memset(&cmd_q, 0x00, sizeof(struct sat_mgr_cmd_q));
	memset(&tr, 0x00, sizeof(struct treq_sat_terminal_rsp_data));

	if (sat_mgr_dequeue_cmd(ctx, &cmd_q, cnf->commandId) == FALSE) {
		dbg("[SAT] command dequeue failed. didn't find in command Q!!");
		return TAPI_API_SAT_INVALID_COMMAND_ID;
	}

	if (cmd_q.cmd_data.displayTextInd.immediate_response_requested == TRUE)
		return TAPI_API_SUCCESS;

	tr.cmd_number = cmd_q.cmd_data.displayTextInd.command_detail.cmd_num;
	tr.cmd_type = cmd_q.cmd_data.displayTextInd.command_detail.cmd_type;
	memcpy((void*)&tr.terminal_rsp_data.display_text.command_detail, &cmd_q.cmd_data.displayTextInd.command_detail, sizeof(struct tel_sat_cmd_detail_info));
	tr.terminal_rsp_data.select_item.device_id.src = DEVICE_ID_ME;
	tr.terminal_rsp_data.select_item.device_id.dest = DEVICE_ID_SIM;

	switch (cnf->keyType) {
		case TAPI_SAT_USER_CONFIRM_YES: {
			dbg("[SAT] cmd_q.cmd_data.displayTextInd.immediate_response_requested[%d]", cmd_q.cmd_data.displayTextInd.immediate_response_requested);

			if (cmd_q.cmd_data.displayTextInd.icon_id.icon_info.ics == IMAGE_CODING_SCHEME_COLOUR)
				tr.terminal_rsp_data.display_text.result_type = RESULT_SUCCESS_BUT_REQUESTED_ICON_NOT_DISPLAYED;
			else
				tr.terminal_rsp_data.display_text.result_type = RESULT_SUCCESS;

			tr.terminal_rsp_data.display_text.me_problem_type = ME_PROBLEM_NO_SPECIFIC_CAUSE;
		}break;

		case TAPI_SAT_USER_CONFIRM_NO_OR_CANCEL:
			tr.terminal_rsp_data.display_text.result_type = TAPI_SAT_R_BACKWARD_MOVE_BY_USER;
			break;

		case TAPI_SAT_USER_CONFIRM_TIMEOUT:
			if (cmd_q.cmd_data.displayTextInd.command_detail.cmd_qualifier.display_text.text_clear_type == TEXT_WAIT_FOR_USER_TO_CLEAR_MSG )
				tr.terminal_rsp_data.display_text.result_type = RESULT_NO_RESPONSE_FROM_USER;
			tr.terminal_rsp_data.display_text.result_type = RESULT_SUCCESS;
			break;

		case TAPI_SAT_USER_CONFIRM_END:
			tr.terminal_rsp_data.display_text.result_type = RESULT_PROACTIVE_SESSION_TERMINATED_BY_USER;
			break;

		case TAPI_SAT_USER_CONFIRM_HELP_INFO:
		default:
			dbg("not handled value[%d] here", cnf->keyType);
			break;
	}
	return sat_mgr_send_terminal_response(ctx, p, conn_name, &tr);
}

static TapiResult_t _user_confirm_get_inkey(struct custom_data *ctx, TcorePlugin *p, char *conn_name, TelSatUiUserConfirmInfo_t *cnf, unsigned char*additional_data, unsigned short ad_data_length)
{
	TapiResult_t rt = TAPI_API_SUCCESS;
	struct sat_mgr_cmd_q cmd_q;
	struct treq_sat_terminal_rsp_data tr;

	memset(&cmd_q, 0x00, sizeof(struct sat_mgr_cmd_q));
	memset(&tr, 0x00, sizeof(struct treq_sat_terminal_rsp_data));

	if (sat_mgr_dequeue_cmd(ctx, &cmd_q, cnf->commandId) == FALSE) {
		dbg("[SAT] command dequeue failed. didn't find in command Q!!");
		return TAPI_API_SAT_INVALID_COMMAND_ID;
	}

	tr.cmd_number = cmd_q.cmd_data.getInkeyInd.command_detail.cmd_num;
	tr.cmd_type = cmd_q.cmd_data.getInkeyInd.command_detail.cmd_type;
	memcpy((void*)&tr.terminal_rsp_data.get_inkey.command_detail, &cmd_q.cmd_data.getInkeyInd.command_detail, sizeof(struct tel_sat_cmd_detail_info));
	tr.terminal_rsp_data.select_item.device_id.src = DEVICE_ID_ME;
	tr.terminal_rsp_data.select_item.device_id.dest = DEVICE_ID_SIM;

	switch (cnf->keyType) {
		case TAPI_SAT_USER_CONFIRM_YES:
			if (cmd_q.cmd_data.getInkeyInd.icon_id.icon_info.ics == IMAGE_CODING_SCHEME_COLOUR)
				tr.terminal_rsp_data.get_inkey.result_type = RESULT_SUCCESS_BUT_REQUESTED_ICON_NOT_DISPLAYED;
			else
				tr.terminal_rsp_data.get_inkey.result_type = RESULT_SUCCESS;

			if (cmd_q.cmd_data.getInkeyInd.command_detail.cmd_qualifier.get_inkey.inkey_type == INKEY_TYPE_YES_NO_REQUESTED) {
				tr.terminal_rsp_data.get_inkey.text.dcs.m_class = MSG_CLASS_RESERVED;
				tr.terminal_rsp_data.get_inkey.text.dcs.a_format = ALPHABET_FROMAT_8BIT_DATA;
				tr.terminal_rsp_data.get_inkey.text.string_length = 1;
				tr.terminal_rsp_data.get_inkey.text.string[0] = 0x01;
			}
			else if (ad_data_length > 0) {
				tr.terminal_rsp_data.get_inkey.text.string_length = ad_data_length;
				/*Set the input mode*/
				if (cmd_q.cmd_data.getInkeyInd.command_detail.cmd_qualifier.get_inkey.alphabet_set == FALSE) {
					tr.terminal_rsp_data.get_inkey.text.is_digit_only = TRUE;
					tr.terminal_rsp_data.get_inkey.text.dcs.a_format = ALPHABET_FROMAT_8BIT_DATA;
					tr.terminal_rsp_data.get_inkey.text.dcs.m_class = MSG_CLASS_RESERVED;
					/*	TODO DCS handling*/
					memcpy((void*)tr.terminal_rsp_data.get_inkey.text.string, additional_data, ad_data_length);
				}
				else {
					tr.terminal_rsp_data.get_inkey.text.dcs.m_class = MSG_CLASS_RESERVED;

					if(cmd_q.cmd_data.getInkeyInd.command_detail.cmd_qualifier.get_inkey.alphabet_type == INPUT_ALPHABET_TYPE_SMS_DEFAULT )
						tr.terminal_rsp_data.get_inkey.text.dcs.a_format = ALPHABET_FROMAT_SMS_DEFAULT;
					else if(cmd_q.cmd_data.getInkeyInd.command_detail.cmd_qualifier.get_inkey.alphabet_type == INPUT_ALPHABET_TYPE_UCS2 )
						tr.terminal_rsp_data.get_inkey.text.dcs.a_format = ALPHABET_FROMAT_UCS2;
					else
						tr.terminal_rsp_data.get_inkey.text.dcs.a_format = ALPHABET_FROMAT_RESERVED;

					if (tr.terminal_rsp_data.get_inkey.text.dcs.a_format == ALPHABET_FROMAT_UCS2) {
						dbg("UCS2 DATA");
						/*	TODO DCS handling*/
						tr.terminal_rsp_data.get_inkey.text.string_length = ad_data_length;
						memcpy((void*)tr.terminal_rsp_data.get_inkey.text.string, additional_data, ad_data_length);
					}
					else if (tr.terminal_rsp_data.get_inkey.text.dcs.a_format	== ALPHABET_FROMAT_SMS_DEFAULT) {
						dbg("GSM7 packed DATA");
						/*	TODO DCS handling*/
						memcpy((void*)tr.terminal_rsp_data.get_inkey.text.string, additional_data, ad_data_length);
					}
					else {
						dbg("[SAT] invalid DCS[%d]",tr.terminal_rsp_data.get_inkey.text.dcs.a_format);
					}
				}
			}
			break;

		case TAPI_SAT_USER_CONFIRM_HELP_INFO:
			tr.terminal_rsp_data.get_inkey.result_type = RESULT_HELP_INFO_REQUIRED_BY_USER;
			ctx->help_requested = TRUE;
			break;

		case TAPI_SAT_USER_CONFIRM_NO_OR_CANCEL:
			if (cmd_q.cmd_data.getInkeyInd.command_detail.cmd_qualifier.get_inkey.inkey_type	== INKEY_TYPE_YES_NO_REQUESTED) {
				tr.terminal_rsp_data.get_inkey.result_type = RESULT_SUCCESS;
				tr.terminal_rsp_data.get_inkey.text.dcs.m_class = MSG_CLASS_RESERVED;
				tr.terminal_rsp_data.get_inkey.text.dcs.a_format = ALPHABET_FROMAT_8BIT_DATA;
				tr.terminal_rsp_data.get_inkey.text.string_length = 1;
				tr.terminal_rsp_data.get_inkey.text.string[0] = 0x00;
			}
			else {
				tr.terminal_rsp_data.get_inkey.result_type = RESULT_BACKWARD_MOVE_BY_USER;
			}
			break;

		case TAPI_SAT_USER_CONFIRM_TIMEOUT:
			tr.terminal_rsp_data.get_inkey.result_type = RESULT_NO_RESPONSE_FROM_USER;
			if (cmd_q.cmd_data.getInkeyInd.duration.time_interval > 0) {
				tr.terminal_rsp_data.get_inkey.duration.time_interval = cmd_q.cmd_data.getInkeyInd.duration.time_interval;
				tr.terminal_rsp_data.get_inkey.duration.time_unit = cmd_q.cmd_data.getInkeyInd.duration.time_unit;
			}
			break;

		case TAPI_SAT_USER_CONFIRM_END:
			tr.terminal_rsp_data.get_inkey.result_type = RESULT_PROACTIVE_SESSION_TERMINATED_BY_USER;
			break;

		default:
			dbg("not handled value[%d] here", cnf->keyType);
			break;
	}
	dbg("tr.terminal_rsp_data.get_inkey.text.string=[%s]",tr.terminal_rsp_data.get_inkey.text.string);
	rt = sat_mgr_send_terminal_response(ctx, p, conn_name, &tr);;
	return rt;
}

static TapiResult_t _user_confirm_get_input(struct custom_data *ctx, TcorePlugin *p, char *conn_name, TelSatUiUserConfirmInfo_t *cnf, unsigned char*additional_data, unsigned short ad_data_length)
{
	TapiResult_t rt = TAPI_API_SUCCESS;
	struct sat_mgr_cmd_q cmd_q;
	struct treq_sat_terminal_rsp_data tr;
	unsigned char* packing_data = NULL;
	unsigned short packing_data_len = 0; //7Bit Packing is needed

	memset(&cmd_q, 0x00, sizeof(struct sat_mgr_cmd_q));
	memset(&tr, 0x00, sizeof(struct treq_sat_terminal_rsp_data));

	if (sat_mgr_dequeue_cmd(ctx, &cmd_q, cnf->commandId) == FALSE) {
		dbg("[SAT] command dequeue failed. didn't find in command Q!!");
		return TAPI_API_SAT_INVALID_COMMAND_ID;
	}

	tr.cmd_number = cmd_q.cmd_data.getInputInd.command_detail.cmd_num;
	tr.cmd_type = cmd_q.cmd_data.getInputInd.command_detail.cmd_type;
	memcpy((void*)&tr.terminal_rsp_data.get_input.command_detail, &cmd_q.cmd_data.getInputInd.command_detail, sizeof(struct tel_sat_cmd_detail_info));
	tr.terminal_rsp_data.select_item.device_id.src = DEVICE_ID_ME;
	tr.terminal_rsp_data.select_item.device_id.dest = DEVICE_ID_SIM;

	switch (cnf->keyType) {
		case TAPI_SAT_USER_CONFIRM_YES:
			if (cmd_q.cmd_data.getInputInd.command_detail.cmd_qualifier.get_input.alphabet_set	== FALSE) {
				dbg("[SAT] alphabet set used");
				tr.terminal_rsp_data.get_input.text.is_digit_only = TRUE;
			}
			tr.terminal_rsp_data.get_input.text.dcs.m_class = MSG_CLASS_RESERVED;
			//SMS-Default-Data
			if (cmd_q.cmd_data.getInputInd.command_detail.cmd_qualifier.get_input.user_input_unpacked_format	== FALSE) {
				dbg("[SAT] packing to SMS7 default");
				packing_data = calloc(1, sizeof(unsigned char));
				if (packing_data == NULL) {
					dbg("[SAT] packing_data malloc failed\n");
					return TAPI_API_OPERATION_FAILED;
				}

				if (ad_data_length != 0) {
					/*	TODO DCS handling*/
					memcpy((void*)&tr.terminal_rsp_data.get_input.text.string, additional_data, ad_data_length);
				}
				tr.terminal_rsp_data.get_input.text.string_length = packing_data_len;
				tr.terminal_rsp_data.get_input.text.dcs.a_format =	TAPI_SAT_ALPHABET_FORMAT_SMS_DEFAULT;
			}
			//SMS 8bit or UCS2 format
			else {
				dbg("[SAT] packing not required");
				if(cmd_q.cmd_data.getInkeyInd.command_detail.cmd_qualifier.get_input.alphabet_type == INPUT_ALPHABET_TYPE_SMS_DEFAULT )
					tr.terminal_rsp_data.get_input.text.dcs.a_format = ALPHABET_FROMAT_SMS_DEFAULT;
				else if(cmd_q.cmd_data.getInkeyInd.command_detail.cmd_qualifier.get_input.alphabet_type == INPUT_ALPHABET_TYPE_UCS2 )
					tr.terminal_rsp_data.get_input.text.dcs.a_format = ALPHABET_FROMAT_UCS2;
				else
					tr.terminal_rsp_data.get_input.text.dcs.a_format = ALPHABET_FROMAT_RESERVED;

				if (tr.terminal_rsp_data.get_input.text.dcs.a_format == ALPHABET_FROMAT_UCS2) {
					/*	TODO DCS handling*/
					tr.terminal_rsp_data.get_input.text.string_length = ad_data_length;
					memcpy((void*)&tr.terminal_rsp_data.get_input.text.string, additional_data, ad_data_length);
				} else {
					/*	TODO DCS handling*/
					tr.terminal_rsp_data.get_input.text.string_length = ad_data_length;
					memcpy((void*)&tr.terminal_rsp_data.get_input.text.string, additional_data, ad_data_length);
				}
			}

			if (cmd_q.cmd_data.getInputInd.icon_id.icon_info.ics == IMAGE_CODING_SCHEME_COLOUR)
				tr.terminal_rsp_data.get_input.result_type = RESULT_SUCCESS_BUT_REQUESTED_ICON_NOT_DISPLAYED;
			else
				tr.terminal_rsp_data.get_input.result_type = RESULT_SUCCESS;
			break;

		case TAPI_SAT_USER_CONFIRM_HELP_INFO:
			tr.terminal_rsp_data.get_input.result_type = RESULT_HELP_INFO_REQUIRED_BY_USER;
			ctx->help_requested = TRUE;
			break;

		case TAPI_SAT_USER_CONFIRM_NO_OR_CANCEL:
			tr.terminal_rsp_data.get_input.result_type = RESULT_BACKWARD_MOVE_BY_USER;
			break;

		case TAPI_SAT_USER_CONFIRM_TIMEOUT:
			tr.terminal_rsp_data.get_input.result_type = RESULT_NO_RESPONSE_FROM_USER;
			break;

		case TAPI_SAT_USER_CONFIRM_END:
			tr.terminal_rsp_data.get_input.result_type = RESULT_PROACTIVE_SESSION_TERMINATED_BY_USER;
			break;
		default:
			break;
	}
	dbg("tr.terminal_rsp_data.get_input.text.string=[%s]",tr.terminal_rsp_data.get_input.text.string);
	rt = sat_mgr_send_terminal_response(ctx, p, conn_name, &tr);;

	if (packing_data)
		free(packing_data);

	return rt;
}

TapiResult_t sat_mgr_handle_user_confirm(struct custom_data *ctx, TcorePlugin *p, char *conn_name,	TelSatUiUserConfirmInfo_t *cnf, unsigned char *additional_data, unsigned short ad_data_length)
{
	TapiResult_t rt = TAPI_API_SUCCESS;
	dbg("[SAT] cnf->commandId=[0x%x], cnf->commandType=[0x%x], cnf->keyType=[%d], ad_data_length=[%d]",
			cnf->commandId, cnf->commandType, cnf->keyType, ad_data_length);

	if (cnf->commandType != TAPI_SAT_CMD_TYPE_SETUP_MENU) {
		struct sat_mgr_cmd_q cmd_q;
		if (sat_mgr_dequeue_cmd_front_by_id(ctx, &cmd_q)) {
			if (cmd_q.cmd_type != cnf->commandType) {
				dbg("[SAT] command type mismatch");
				return TAPI_API_SAT_COMMAND_TYPE_MISMATCH;
			}
		} else {
			dbg("[SAT] no commands in queue");
			return TAPI_API_SAT_INVALID_COMMAND_ID;
		}
	}

	switch (cnf->commandType) {
		case TAPI_SAT_CMD_TYPE_SELECT_ITEM:
			rt = _user_confirm_menu_select(ctx, p, conn_name, cnf,	additional_data, ad_data_length);
			break;
		case TAPI_SAT_CMD_TYPE_DISPLAY_TEXT:
			rt = _user_confirm_display_text(ctx, p, conn_name, cnf, additional_data, ad_data_length);
			break;
		case TAPI_SAT_CMD_TYPE_GET_INKEY:
			rt = _user_confirm_get_inkey(ctx, p, conn_name, cnf, additional_data, ad_data_length);
			break;
		case TAPI_SAT_CMD_TYPE_GET_INPUT:
			rt = _user_confirm_get_input(ctx, p, conn_name, cnf, additional_data, ad_data_length);
			break;
		default:
			dbg("[SAT] sat_mgr_handle_user_confirm :Unknown cnf->commandType:[0x%x]",	cnf->commandType);
			rt = TAPI_API_SERVER_FAILURE;
			break;
	}

	if(rt != TAPI_API_SUCCESS)
		dbg("[SAT] current user confirm procedure make error[%d]", rt);

	return rt;
}
