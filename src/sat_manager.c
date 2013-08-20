/*
 * tel-plugin-dbus_tapi
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
#include <stdlib.h>
#include <errno.h>
#include <sys/time.h>
#include <glib-object.h>

#include <tcore.h>
#include <server.h>
#include <plugin.h>
#include <storage.h>
#include <hal.h>
#include <communicator.h>
#include <core_object.h>
#include <queue.h>
#include <user_request.h>
#include <util.h>
#include <co_sat.h>
#include <type/call.h>
#include <type/sim.h>

#include "generated-code.h"
#include "common.h"
#include "sat_manager.h"
#include "sat_ui_support/sat_ui_support.h"

#define SAT_DEF_CMD_Q_MAX 10
#define SAT_TIME_OUT 30000

static struct sat_manager_queue_data *sat_queue[SAT_DEF_CMD_Q_MAX] = {NULL, };

static unsigned char _convert_decimal_to_bcd(int dec)
{
	int tmp1, tmp0;
	unsigned char tmp3;

	tmp1 = dec/10;
	tmp0 = dec - tmp1*10;
	tmp3 = tmp0 << 4;
	tmp3 += tmp1;

	dbg("input decimal(%d), bcd(%d%d), endian(%x)", dec, tmp1, tmp0, tmp3);
	return tmp3;
}

static const gchar* _convert_sim_lang_to_string(enum tel_sim_language_type lang)
{
	dbg("convert lang(%d)", lang);
	switch(lang){
		case SIM_LANG_GERMAN:
			return "de_DE.UTF-8";
		case SIM_LANG_ENGLISH:
			return "en_GB.UTF-8";
		case SIM_LANG_ITALIAN:
			return "it_IT.UTF-8";
		case SIM_LANG_FRENCH:
			return "fr_FR.UTF-8";
		case SIM_LANG_SPANISH:
			return "es_ES.UTF-8";
		case SIM_LANG_DUTCH:
			return "nl_NL.UTF-8";
		case SIM_LANG_SWEDISH:
			return "sv_SE.UTF-8";
		case SIM_LANG_DANISH:
			return "da_DK.UTF-8";
		case SIM_LANG_PORTUGUESE:
			return "pt_PT.UTF-8";
		case SIM_LANG_FINNISH:
			return "fi_FI.UTF-8";
		case SIM_LANG_NORWEGIAN:
			return "nb_NO.UTF-8";
		case SIM_LANG_GREEK:
			return "el_GR.UTF-8";
		case SIM_LANG_TURKISH:
			return "tr_TR.UTF-8";
		case SIM_LANG_HUNGARIAN:
			return "hu_HU.UTF-8";
		case SIM_LANG_POLISH:
			return "pl_PL.UTF-8";
		case SIM_LANG_KOREAN:
			return "ko_KR.UTF-8";
		case SIM_LANG_CHINESE:
			return "zh_CH.UTF-8";
		case SIM_LANG_RUSSIAN:
			return "ru_RU.UTF-8";
		case SIM_LANG_JAPANESE:
			return "ja_JP.UTF-8";
		default:
			return NULL;
	}//end of switch

	return NULL;
}

static enum tel_sim_language_type _convert_string_to_sim_lang(const gchar* lang_str)
{
	dbg("convert lang(%s)", lang_str);

	if (g_str_equal(lang_str, "de_DE.UTF-8") == TRUE) {
		return SIM_LANG_GERMAN;
	}
	else if (g_str_equal(lang_str, "en_GB.UTF-8") == TRUE) {
		return SIM_LANG_ENGLISH;
	}
	else if (g_str_equal(lang_str, "it_IT.UTF-8") == TRUE) {
		return SIM_LANG_ITALIAN;
	}
	else if (g_str_equal(lang_str, "fr_FR.UTF-8") == TRUE) {
		return SIM_LANG_FRENCH;
	}
	else if (g_str_equal(lang_str, "es_ES.UTF-8") == TRUE) {
		return SIM_LANG_SPANISH;
	}
	else if (g_str_equal(lang_str, "nl_NL.UTF-8") == TRUE) {
		return SIM_LANG_DUTCH;
	}
	else if (g_str_equal(lang_str, "sv_SE.UTF-8") == TRUE) {
		return SIM_LANG_SWEDISH;
	}
	else if (g_str_equal(lang_str, "da_DK.UTF-8") == TRUE) {
		return SIM_LANG_DANISH;
	}
	else if (g_str_equal(lang_str, "pt_PT.UTF-8") == TRUE) {
		return SIM_LANG_PORTUGUESE;
	}
	else if (g_str_equal(lang_str, "fi_FI.UTF-8") == TRUE) {
		return SIM_LANG_FINNISH;
	}
	else if (g_str_equal(lang_str, "nb_NO.UTF-8") == TRUE) {
		return SIM_LANG_NORWEGIAN;
	}
	else if (g_str_equal(lang_str, "el_GR.UTF-8") == TRUE) {
		return SIM_LANG_GREEK;
	}
	else if (g_str_equal(lang_str, "tr_TR.UTF-8") == TRUE) {
		return SIM_LANG_TURKISH;
	}
	else if (g_str_equal(lang_str, "hu_HU.UTF-8") == TRUE) {
		return SIM_LANG_HUNGARIAN;
	}
	else if (g_str_equal(lang_str, "pl_PL.UTF-8") == TRUE) {
		return SIM_LANG_POLISH;
	}
	else if (g_str_equal(lang_str, "ko_KR.UTF-8") == TRUE) {
		return SIM_LANG_KOREAN;
	}
	else if (g_str_equal(lang_str, "zh_CH.UTF-8") == TRUE) {
		return SIM_LANG_CHINESE;
	}
	else if (g_str_equal(lang_str, "ru_RU.UTF-8") == TRUE) {
		return SIM_LANG_RUSSIAN;
	}
	else if (g_str_equal(lang_str, "ja_JP.UTF-8") == TRUE) {
		return SIM_LANG_JAPANESE;
	}

	dbg("there is no matched language");
	return SIM_LANG_UNSPECIFIED;
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

	return 0;
}

static int _get_queue_empty_index(void)
{
	int cnt = 0;
	int i;
	int index = -1;

	for(i =0; i<SAT_DEF_CMD_Q_MAX ; i++) {
		if(sat_queue[i]) {
			dbg("index[%d] is being used",i);
			cnt++;
		}
	}
	for(i =0; i<SAT_DEF_CMD_Q_MAX ; i++) {
		if(!sat_queue[i]) {
			dbg("found empty slot [%p] at index [%d]", sat_queue[i] ,i);
			index = i;
			break;
		}
	}
	dbg("[SAT]SAT Command Queue current length [%d], MAX [%d]. \n", cnt, SAT_DEF_CMD_Q_MAX);
	return index;
}

static gboolean _push_data(struct custom_data *ctx, struct sat_manager_queue_data *cmd_obj)
{
	struct sat_manager_queue_data *item;
	int index = cmd_obj->cmd_id;

	if (sat_queue[index]) {
		dbg("[SAT] sat_queue[%d] is not null [%p].\n", sat_queue[index]);
		return FALSE;
	}

	item = g_new0(struct sat_manager_queue_data, 1);

	if(!item) {
		dbg("[SAT] FAILED TO ALLOC QUEUE ITEM!\n");
		return FALSE;
	}

	memcpy((void*)item, (void*)cmd_obj, sizeof(struct sat_manager_queue_data));
	sat_queue[index] = item;
	dbg("push data to queue at index[%d], [%p].\n",index, item);
	return TRUE;
}

static gboolean _pop_nth_data(struct custom_data *ctx, struct sat_manager_queue_data *cmd_obj, int command_id)
{
	struct sat_manager_queue_data *item;
	int index = command_id;

	if(!sat_queue[index]) {
		dbg("[SAT] sat_queue[%d] is null !!\n", index);
		return FALSE;
	}

	item = sat_queue[index];

	memcpy((void*)cmd_obj, (void*)item, sizeof(struct sat_manager_queue_data));
	dbg("pop data from queue at index[%d],[%p].\n",index, item);
	sat_queue[index] = NULL;
	g_free(item);
	return TRUE;
}

static gboolean _peek_nth_data(struct custom_data *ctx, struct sat_manager_queue_data *cmd_obj, int command_id)
{
	struct sat_manager_queue_data *item = NULL;

	int index = command_id;

	if(!sat_queue[index]) {
		dbg("[SAT] sat_queue[%d] is null !!\n", index);
		return FALSE;
	}

	item = sat_queue[index];
	memcpy((void*)cmd_obj, (void*)item, sizeof(struct sat_manager_queue_data));
	dbg("peek data from queue at index[%d],[%p].\n",index, item);
	return TRUE;
}

void sat_manager_init_queue(struct custom_data *ctx)
{
	int i;
	dbg("Enter");
	for(i=0;i<SAT_DEF_CMD_Q_MAX;i++) {
		sat_queue[i] = NULL;
	}
}

static gboolean sat_manager_enqueue_cmd(struct custom_data *ctx, struct sat_manager_queue_data *cmd_obj)
{
	int id;

	id = _get_queue_empty_index();
	if(id < 0) {
		dbg("Fail to get empty index.\n");
		return FALSE;
	}
	cmd_obj->cmd_id = id;
	return _push_data(ctx, cmd_obj);
}

static gboolean sat_manager_dequeue_cmd_by_id(struct custom_data *ctx, struct sat_manager_queue_data *cmd_obj, int cmd_id)
{
	return _pop_nth_data(ctx, cmd_obj, cmd_id);
}

static gboolean sat_manager_queue_peek_data_by_id(struct custom_data *ctx, struct sat_manager_queue_data *cmd_obj, int command_id)
{
	return _peek_nth_data(ctx, cmd_obj, command_id);
}

static gboolean sat_manager_check_availiable_event_list(struct tel_sat_setup_event_list_tlv *event_list_tlv)
{
	gboolean rv = TRUE;
	int index = 0;

	for(index = 0; index < event_list_tlv->event_list.event_list_cnt; index++){
		if(event_list_tlv->event_list.evt_list[index] == EVENT_USER_ACTIVITY){
			dbg("do user activity");
		}
		else if(event_list_tlv->event_list.evt_list[index] == EVENT_IDLE_SCREEN_AVAILABLE){
			dbg("do idle screen");
		}
		else if(event_list_tlv->event_list.evt_list[index] == EVENT_LANGUAGE_SELECTION){
			dbg("do language selection");
		}
		else if(event_list_tlv->event_list.evt_list[index] == EVENT_BROWSER_TERMINATION){
			dbg("do browser termination");
		}
		else if(event_list_tlv->event_list.evt_list[index] == EVENT_DATA_AVAILABLE){
			dbg("do data available (bip)");
		}
		else if(event_list_tlv->event_list.evt_list[index] == EVENT_CHANNEL_STATUS){
			dbg("do channel status (bip)");
		}
		else{
			dbg("unmanaged event (%d)", event_list_tlv->event_list.evt_list[index]);
			rv = FALSE;
		}
	}

	return rv;
}

static TReturn sat_manager_send_terminal_response(Communicator *comm, TcorePlugin *target_plg, struct treq_sat_terminal_rsp_data *tr)
{
	TReturn rv = TCORE_RETURN_SUCCESS;
	UserRequest *ur = NULL;

	ur = tcore_user_request_new(comm, tcore_server_get_cp_name_by_plugin(target_plg));
	if (!ur) {
		dbg("ur is NULL");
		return TCORE_RETURN_FAILURE;
	}

	tcore_user_request_set_command(ur, TREQ_SAT_REQ_TERMINALRESPONSE);
	tcore_user_request_set_data(ur, sizeof(struct treq_sat_terminal_rsp_data), (void *)tr);
	rv = tcore_communicator_dispatch_request(comm, ur);
	if (rv != TCORE_RETURN_SUCCESS) {
		dbg("fail to send terminal response",rv);
		tcore_user_request_unref(ur);
		rv = TCORE_RETURN_FAILURE;
	}

	return rv;
}

gboolean sat_manager_remove_cmd_by_id(struct custom_data *ctx, int cmd_id)
{
	struct sat_manager_queue_data *item;
	int index = cmd_id;

	if(!sat_queue[index]) {
		dbg("[SAT] sat_queue[%d] is already null !!\n", index);
		return FALSE;
	}
	item = sat_queue[index];

	dbg("remove data from queue at index[%d],[%p].\n",index, item);
	sat_queue[index] = NULL;
	g_free(item);
	return TRUE;
}

GVariant* sat_manager_caching_setup_menu_info(struct custom_data *ctx, const char *plugin_name, struct tel_sat_setup_menu_tlv* setup_menu_tlv)
{
	TcorePlugin *plg = NULL;
	GVariant *setup_menu_info = NULL;
	struct sat_manager_queue_data q_data;

	gushort title_len = 0;
	gint command_id = 0, menu_cnt = 0;
	gboolean menu_present = FALSE, help_info = FALSE, updated = FALSE;
	gchar main_title[SAT_ALPHA_ID_LEN_MAX];
	GVariantBuilder *v_builder = NULL;
	GVariant *menu_items = NULL;

	dbg("interpreting setup menu notification");
	memset(&main_title, 0 , SAT_ALPHA_ID_LEN_MAX);

	plg = tcore_server_find_plugin(ctx->server, plugin_name);
	if (!plg){
		dbg("there is no valid plugin at this point");
		return NULL;
	}

	if ((setup_menu_tlv->icon_id.is_exist)
			&& (setup_menu_tlv->icon_id.icon_qualifer == ICON_QUALI_NOT_SELF_EXPLANATORY)
			&& (!setup_menu_tlv->alpha_id.is_exist || setup_menu_tlv->alpha_id.alpha_data_len == 0)){

		struct treq_sat_terminal_rsp_data *tr = NULL;
		dbg("exceptional case to fix gcf case 2.4 command not understood");

		tr = g_new0(struct treq_sat_terminal_rsp_data, 1);
		tr->cmd_number = setup_menu_tlv->command_detail.cmd_num;
		tr->cmd_type = setup_menu_tlv->command_detail.cmd_type;

		memcpy((void*)&tr->terminal_rsp_data.setup_menu.command_detail, &setup_menu_tlv->command_detail, sizeof(struct tel_sat_cmd_detail_info));
		tr->terminal_rsp_data.setup_menu.device_id.src = setup_menu_tlv->device_id.dest;
		tr->terminal_rsp_data.setup_menu.device_id.dest = setup_menu_tlv->device_id.src;
		tr->terminal_rsp_data.setup_menu.result_type = RESULT_COMMAND_DATA_NOT_UNDERSTOOD_BY_ME;

		sat_manager_send_terminal_response(ctx->comm, plg, tr);
		g_free(tr);
		return NULL;
	}

	//check menu update
	if(ctx->cached_sat_main_menu){
		dbg("main menu info is updated");
		updated = TRUE;
	}

	//menu helpinfo
	help_info = setup_menu_tlv->command_detail.cmd_qualifier.setup_menu.help_info;

	//menu presence
	menu_present = setup_menu_tlv->command_detail.cmd_qualifier.setup_menu.select_preference;

	//menu item count
	menu_cnt = setup_menu_tlv->menu_item_cnt;

	//check the validation of content
	if(!menu_cnt || (setup_menu_tlv->menu_item_cnt == 1 && setup_menu_tlv->menu_item[0].text_len == 0)){
		//support GCF case 27.22.4.8.1 - 1.1 setup menu

		struct treq_sat_terminal_rsp_data *tr = NULL;
		dbg("no menu item");

		tr = (struct treq_sat_terminal_rsp_data *)calloc(1, sizeof(struct treq_sat_terminal_rsp_data));
		tr->cmd_number = setup_menu_tlv->command_detail.cmd_num;
		tr->cmd_type = setup_menu_tlv->command_detail.cmd_type;

		memcpy((void*)&tr->terminal_rsp_data.setup_menu.command_detail, &setup_menu_tlv->command_detail, sizeof(struct tel_sat_cmd_detail_info));
		tr->terminal_rsp_data.setup_menu.device_id.src = setup_menu_tlv->device_id.dest;
		tr->terminal_rsp_data.setup_menu.device_id.dest = setup_menu_tlv->device_id.src;
		tr->terminal_rsp_data.setup_menu.result_type = RESULT_SUCCESS;

		sat_manager_send_terminal_response(ctx->comm, plg, tr);
		g_free(tr);

		return NULL;
	}

	//get title
	if(setup_menu_tlv->alpha_id.alpha_data_len)
		tcore_util_convert_string_to_utf8((unsigned char*)&main_title,(unsigned short *)&title_len,
			setup_menu_tlv->alpha_id.dcs.a_format,
			(unsigned char*)&setup_menu_tlv->alpha_id.alpha_data,
			(unsigned short)setup_menu_tlv->alpha_id.alpha_data_len);
	dbg("sat main menu title(%s)",main_title);

	v_builder = g_variant_builder_new(G_VARIANT_TYPE ("a(si)"));

	//get menu items
	if(!setup_menu_tlv->next_act_ind_list.cnt){
		int index = 0;

		dbg("setup_menu_tlv->next_act_ind_list.cnt == 0");

		for(index = 0; index < menu_cnt; index++){
			gushort item_len;
			gchar item_str[SAT_ITEM_TEXT_LEN_MAX + 1];

			if(!setup_menu_tlv->alpha_id.alpha_data_len)
				setup_menu_tlv->alpha_id.dcs.a_format = ALPHABET_FORMAT_8BIT_DATA;

			memset(&item_str, 0 , SAT_ITEM_TEXT_LEN_MAX + 1);
			tcore_util_convert_string_to_utf8((unsigned char*)&item_str, (unsigned short *)&item_len,
				setup_menu_tlv->alpha_id.dcs.a_format,
				(unsigned char*)&setup_menu_tlv->menu_item[index].text,
				(unsigned short)setup_menu_tlv->menu_item[index].text_len);

			dbg( "index(%d) item_id(%d) item_string(%s)", index, setup_menu_tlv->menu_item[index].item_id, item_str);

			//g_variant_builder_add(v_builder, "(sy)", &item_str, setup_menu_tlv->menu_item[index].item_id);
			g_variant_builder_add(v_builder, "(si)", item_str, (gint32)(setup_menu_tlv->menu_item[index].item_id));
		}
	}
	else{
		int index = 0;

		dbg("setup_menu_tlv->next_act_ind_list.cnt == 0");

		for(index = 0; index < menu_cnt; index++){
			gushort item_len;
			gchar item_str[SAT_ITEM_TEXT_LEN_MAX + 1];

			if(setup_menu_tlv->alpha_id.alpha_data_len == 0)
				setup_menu_tlv->alpha_id.dcs.a_format = ALPHABET_FORMAT_8BIT_DATA;

			memset(&item_str, '\0' , SAT_ITEM_TEXT_LEN_MAX + 1);
			tcore_util_convert_string_to_utf8((unsigned char*)&item_str, (unsigned short *)&item_len,
				setup_menu_tlv->alpha_id.dcs.a_format,
				(unsigned char*)&setup_menu_tlv->menu_item[index].text,
				(unsigned short)setup_menu_tlv->menu_item[index].text_len);

			if( setup_menu_tlv->next_act_ind_list.indicator_list[index] == SAT_PROATV_CMD_SEND_SMS) {
				g_strlcat(item_str," [Send SMS]", 11);
			}
			else if (setup_menu_tlv->next_act_ind_list.indicator_list[index]== SAT_PROATV_CMD_SETUP_CALL) {
				g_strlcat(item_str," [Set Up Call]", 14);
			}
			else if (setup_menu_tlv->next_act_ind_list.indicator_list[index]== SAT_PROATV_CMD_LAUNCH_BROWSER){
				g_strlcat(item_str," [Launch Browser]", 17);
			}
			else if (setup_menu_tlv->next_act_ind_list.indicator_list[index]== SAT_PROATV_CMD_PROVIDE_LOCAL_INFO)	{
				g_strlcat(item_str," [Provide Terminal Information]", 31);
			}

			dbg( "index(%d) item_id(%d) item_string(%s)", index, setup_menu_tlv->menu_item[index].item_id, item_str);

			//g_variant_builder_add(v_builder, "(sy)", g_strdup(item_str), setup_menu_tlv->menu_item[index].item_id);
			g_variant_builder_add(v_builder, "(si)", item_str, (gint32)(setup_menu_tlv->menu_item[index].item_id));
		}

	}
	menu_items = g_variant_builder_end(v_builder);

	//enqueue data and generate cmd_id
	memset(&q_data, 0x00, sizeof(struct sat_manager_queue_data));
	q_data.cmd_type = SAT_PROATV_CMD_SETUP_MENU;
	memcpy((void*)&(q_data.cmd_data.setupMenuInd), setup_menu_tlv, sizeof(struct tel_sat_setup_menu_tlv));
	sat_manager_enqueue_cmd(ctx, &q_data);
	command_id = q_data.cmd_id;

	setup_menu_info = g_variant_new("(ibsvibb)", command_id, menu_present, main_title, menu_items,
			menu_cnt, help_info, updated);

	return setup_menu_info;
}

GVariant* sat_manager_display_text_noti(struct custom_data *ctx, const char *plugin_name, struct tel_sat_display_text_tlv* display_text_tlv)
{
	TcorePlugin *plg = NULL;
	GVariant *display_text = NULL;
	struct sat_manager_queue_data q_data;

	gushort text_len = 0;
	gint command_id = 0, duration= 0, tmp_duration = 0;
	gboolean immediately_rsp = FALSE, high_priority = FALSE, user_rsp_required = FALSE;
	gchar text[SAT_TEXT_STRING_LEN_MAX];
	GVariant *icon_id = NULL;

	dbg("interpreting display text notification");
	memset(&text, 0 , SAT_TEXT_STRING_LEN_MAX);

	plg = tcore_server_find_plugin(ctx->server, plugin_name);
	if (!plg){
		dbg("there is no valid plugin at this point");
		return NULL;
	}

	if ( (display_text_tlv->icon_id.is_exist && display_text_tlv->icon_id.icon_qualifer == ICON_QUALI_NOT_SELF_EXPLANATORY)
			|| !display_text_tlv->text.string_length ){

		struct treq_sat_terminal_rsp_data *tr = NULL;
		dbg("[SAT]  exceptional case to fix gcf case 2.4 command not understood");

		tr = (struct treq_sat_terminal_rsp_data *)calloc(1, sizeof(struct treq_sat_terminal_rsp_data));
		tr->cmd_number = display_text_tlv->command_detail.cmd_num;
		tr->cmd_type = display_text_tlv->command_detail.cmd_type;

		memcpy((void*)&tr->terminal_rsp_data.display_text.command_detail, &display_text_tlv->command_detail, sizeof(struct tel_sat_cmd_detail_info));
		tr->terminal_rsp_data.display_text.device_id.src = DEVICE_ID_ME;
		tr->terminal_rsp_data.display_text.device_id.dest = display_text_tlv->device_id.src;
		tr->terminal_rsp_data.display_text.result_type = RESULT_COMMAND_DATA_NOT_UNDERSTOOD_BY_ME;

		sat_manager_send_terminal_response(ctx->comm, plg, tr);
		g_free(tr);

		return NULL;
	}

	//user resp required & time_duration
	if(display_text_tlv->command_detail.cmd_qualifier.display_text.text_clear_type == TEXT_WAIT_FOR_USER_TO_CLEAR_MSG){
		user_rsp_required = TRUE;
		duration = SAT_TIME_OUT;
	}
	else{
		duration = 15000;
	}

	//immediate response requested
	if (display_text_tlv->immediate_response_requested)
		immediately_rsp = TRUE;

	//high priority
	if (display_text_tlv->command_detail.cmd_qualifier.display_text.text_priority == TEXT_PRIORITY_HIGH)
		high_priority = TRUE;
	dbg("user rsp required(%d), immediately rsp(%d) priority(%d)",user_rsp_required, immediately_rsp, high_priority);

	//get text
	tcore_util_convert_string_to_utf8((unsigned char*) &text, (unsigned short *) &text_len,
			display_text_tlv->text.dcs.a_format,
			(unsigned char*) &display_text_tlv->text.string,
			(unsigned short) display_text_tlv->text.string_length);
	dbg("sat display text(%s)",text);

	//duration
	if(!display_text_tlv->duration.time_interval){
		tmp_duration = _get_time_in_ms(&display_text_tlv->duration);
	}

	if(tmp_duration > 0)
		duration = tmp_duration;

/*	 ETSI TS 102 223 6.4.1 DISPLAY TEXT
	 If help information is requested by the user, this command may be used to display help information on the screen. The
	 help information should be sent as high priority text and with the option that it should be cleared after a short delay.*/

/*	if (ctx->help_requested == TRUE) {
		ad->bIsPriorityHigh = TRUE;
		ad->duration = 7000;
		ctx->help_requested = FALSE;
	}*/

	//enqueue data and generate cmd_id
	memset(&q_data, 0x00, sizeof(struct sat_manager_queue_data));
	q_data.cmd_type = SAT_PROATV_CMD_DISPLAY_TEXT;
	memcpy((void*)&(q_data.cmd_data.displayTextInd), display_text_tlv, sizeof(struct tel_sat_display_text_tlv));
	sat_manager_enqueue_cmd(ctx, &q_data);
	command_id = q_data.cmd_id;

	icon_id = g_variant_new("()");

	display_text = g_variant_new("(isiibbbv)", command_id, text, text_len, duration,
			high_priority, user_rsp_required, immediately_rsp, icon_id);

	return display_text;
}

GVariant* sat_manager_select_item_noti(struct custom_data *ctx, const char *plugin_name, struct tel_sat_select_item_tlv* select_item_tlv)
{
	TcorePlugin *plg = NULL;
	GVariant *select_item = NULL;
	struct sat_manager_queue_data q_data;

	int index = 0;
	gushort text_len = 0;
	gint command_id = 0, default_item_id = 0, menu_cnt = 0;
	gboolean help_info = FALSE;
	gchar text[SAT_TEXT_STRING_LEN_MAX];
	GVariantBuilder *v_builder = NULL;
	GVariant *menu_items = NULL;
	GVariant *icon_id = NULL;
	GVariant *icon_list = NULL;

	dbg("interpreting select item notification");
	memset(&text, 0 , SAT_TEXT_STRING_LEN_MAX);

	plg = tcore_server_find_plugin(ctx->server, plugin_name);
	if (!plg){
		dbg("there is no valid plugin at this point");
		return NULL;
	}

	if ((select_item_tlv->icon_id.is_exist)
			&& (select_item_tlv->icon_id.icon_qualifer == ICON_QUALI_NOT_SELF_EXPLANATORY)
			&& ( !select_item_tlv->alpha_id.is_exist || select_item_tlv->alpha_id.alpha_data_len == 0) ) {

		struct treq_sat_terminal_rsp_data *tr = NULL;
		dbg("[SAT]  exceptional case to fix gcf case 2.4 command not understood");

		tr = (struct treq_sat_terminal_rsp_data *) calloc(1, sizeof(struct treq_sat_terminal_rsp_data));

		tr->cmd_number = select_item_tlv->command_detail.cmd_num;
		tr->cmd_type = select_item_tlv->command_detail.cmd_type;

		memcpy((void*) &tr->terminal_rsp_data.select_item.command_detail, &select_item_tlv->command_detail, sizeof(struct tel_sat_cmd_detail_info));
		tr->terminal_rsp_data.select_item.device_id.src = select_item_tlv->device_id.dest;
		tr->terminal_rsp_data.select_item.device_id.dest = select_item_tlv->device_id.src;
		tr->terminal_rsp_data.select_item.result_type = RESULT_COMMAND_DATA_NOT_UNDERSTOOD_BY_ME;

		sat_manager_send_terminal_response(ctx->comm, plg, tr);
		g_free(tr);

		return NULL;
	}

	// help info
	help_info = select_item_tlv->command_detail.cmd_qualifier.select_item.help_info;

	if(!select_item_tlv->alpha_id.is_exist){
		dbg("set the item dcs value to ALPHABET_FORMAT_8BIT_DATA");
		select_item_tlv->alpha_id.dcs.a_format = ALPHABET_FORMAT_8BIT_DATA;
	}

	// select item text
	if(select_item_tlv->alpha_id.is_exist && select_item_tlv->alpha_id.alpha_data_len > 0)
		tcore_util_convert_string_to_utf8((unsigned char*)&text,(unsigned short *)&text_len,
				select_item_tlv->alpha_id.dcs.a_format,
				(unsigned char*)&select_item_tlv->alpha_id.alpha_data,
				(unsigned short)select_item_tlv->alpha_id.alpha_data_len);
	dbg("select item text(%s)",text);

	//default item id
	default_item_id = select_item_tlv->item_identifier.item_identifier;
	dbg( "default item id(%d)", default_item_id);

	//item count
	menu_cnt = select_item_tlv->menu_item_cnt;
	dbg( "menu item count(%d)", menu_cnt);

	//items
	v_builder = g_variant_builder_new(G_VARIANT_TYPE ("a(iis)"));
	for(index= 0; index< menu_cnt; index++){
		gushort item_len;
		gchar item_str[SAT_ITEM_TEXT_LEN_MAX + 1];

		memset(&item_str, 0 , SAT_ITEM_TEXT_LEN_MAX + 1);

		tcore_util_convert_string_to_utf8((unsigned char*) &item_str, (unsigned short *) &item_len,
				select_item_tlv->alpha_id.dcs.a_format,
				(unsigned char*) &select_item_tlv->menu_item[index].text,
				(unsigned short) select_item_tlv->menu_item[index].text_len);

		if (select_item_tlv->item_next_act_ind_list.cnt != 0) {
			if( select_item_tlv->item_next_act_ind_list.indicator_list[index] == SAT_PROATV_CMD_SEND_SMS) {
				g_strlcat(item_str," [Send SMS]", 11);
			}
			else if (select_item_tlv->item_next_act_ind_list.indicator_list[index]== SAT_PROATV_CMD_SETUP_CALL) {
				g_strlcat(item_str," [Set Up Call]", 14);
			}
			else if (select_item_tlv->item_next_act_ind_list.indicator_list[index]== SAT_PROATV_CMD_LAUNCH_BROWSER){
				g_strlcat(item_str," [Launch Browser]", 17);
			}
			else if (select_item_tlv->item_next_act_ind_list.indicator_list[index]== SAT_PROATV_CMD_PROVIDE_LOCAL_INFO)	{
				g_strlcat(item_str," [Provide Terminal Information]", 31);
			}
		}

		item_len = strlen(item_str);
		dbg( "index(%d) item_id(%d) item_len(%d) item_string(%s)", index, select_item_tlv->menu_item[index].item_id, item_len, item_str);
		g_variant_builder_add(v_builder, "(iis)", (gint32)(select_item_tlv->menu_item[index].item_id), item_len, item_str);
	}
	menu_items = g_variant_builder_end(v_builder);

	// generate command id
	memset(&q_data, 0x00, sizeof(struct sat_manager_queue_data));
	q_data.cmd_type = SAT_PROATV_CMD_SELECT_ITEM;
	memcpy((void*)&(q_data.cmd_data.selectItemInd), select_item_tlv, sizeof(struct tel_sat_select_item_tlv));
	sat_manager_enqueue_cmd(ctx, &q_data);
	command_id = q_data.cmd_id;

	icon_id = g_variant_new_variant(menu_items);
	icon_list = g_variant_new_variant(menu_items);

	select_item = g_variant_new("(ibsiiivvv)", command_id, help_info, text, text_len,
			default_item_id, menu_cnt, menu_items, icon_id, icon_list);

	return select_item;
}

GVariant* sat_manager_get_inkey_noti(struct custom_data *ctx, const char *plugin_name, struct tel_sat_get_inkey_tlv* get_inkey_tlv)
{
	TcorePlugin *plg = NULL;
	GVariant *get_inkey = NULL;
	struct sat_manager_queue_data q_data;

	gint command_id = 0, key_type = 0, input_character_mode = 0;
	gushort text_len = 0;
	gint duration = 0, tmp_duration = 0;
	gboolean b_numeric = FALSE, b_help_info = FALSE;
	gchar text[SAT_TEXT_STRING_LEN_MAX];
	GVariant *icon_id = NULL;

	dbg("interpreting get inkey notification");
	memset(&text, 0 , SAT_TEXT_STRING_LEN_MAX);

	plg = tcore_server_find_plugin(ctx->server, plugin_name);
	if (!plg){
		dbg("there is no valid plugin at this point");
		return NULL;
	}

	if (get_inkey_tlv->icon_id.is_exist && !get_inkey_tlv->text.string_length
			&& (get_inkey_tlv->icon_id.icon_qualifer == ICON_QUALI_NOT_SELF_EXPLANATORY)){

		struct treq_sat_terminal_rsp_data *tr = NULL;
		dbg("[SAT]  exceptional case to fix gcf case 2.4 command not understood");

		tr = (struct treq_sat_terminal_rsp_data *)calloc(1, sizeof(struct treq_sat_terminal_rsp_data));
		tr->cmd_number = get_inkey_tlv->command_detail.cmd_num;
		tr->cmd_type = get_inkey_tlv->command_detail.cmd_type;

		memcpy((void*)&tr->terminal_rsp_data.get_inkey.command_detail, &get_inkey_tlv->command_detail, sizeof(struct tel_sat_cmd_detail_info));
		tr->terminal_rsp_data.get_inkey.device_id.src = get_inkey_tlv->device_id.dest;
		tr->terminal_rsp_data.get_inkey.device_id.dest = get_inkey_tlv->device_id.src;
		tr->terminal_rsp_data.get_inkey.result_type = RESULT_COMMAND_DATA_NOT_UNDERSTOOD_BY_ME;

		sat_manager_send_terminal_response(ctx->comm, plg, tr);
		g_free(tr);

		return NULL;
	}

	//key type
	key_type = get_inkey_tlv->command_detail.cmd_qualifier.get_inkey.inkey_type;

	//time duration
	duration = SAT_TIME_OUT;
	tmp_duration = _get_time_in_ms(&get_inkey_tlv->duration);
	if (tmp_duration > 0)
		duration = tmp_duration;

	//input mode
	input_character_mode = get_inkey_tlv->command_detail.cmd_qualifier.get_inkey.alphabet_type;

	//numeric
	b_numeric = !get_inkey_tlv->command_detail.cmd_qualifier.get_inkey.alphabet_set;

	//help info
	b_help_info = get_inkey_tlv->command_detail.cmd_qualifier.get_inkey.help_info;

	//text & text len
	tcore_util_convert_string_to_utf8((unsigned char*)&text,(unsigned short *)&text_len,
								get_inkey_tlv->text.dcs.a_format ,
								(unsigned char*)&get_inkey_tlv->text.string,
								(unsigned short)get_inkey_tlv->text.string_length);

	dbg("get inkey text(%s)",text);

	if (get_inkey_tlv->command_detail.cmd_qualifier.get_inkey.immediate_rsp_required) {
		dbg("get_inkey immediate_rsp_require is TRUE");
		//Send TR if UI display success
	}

	//enqueue data and generate cmd_id
	memset(&q_data, 0x00, sizeof(struct sat_manager_queue_data));
	q_data.cmd_type = SAT_PROATV_CMD_GET_INKEY;
	memcpy((void*)&(q_data.cmd_data.getInkeyInd), get_inkey_tlv, sizeof(struct tel_sat_get_inkey_tlv));
	sat_manager_enqueue_cmd(ctx, &q_data);
	command_id = q_data.cmd_id;

	icon_id = g_variant_new("()");

	get_inkey = g_variant_new("(iiibbsiiv)", command_id, key_type, input_character_mode, b_numeric,
			b_help_info, text, text_len, duration, icon_id);

	return get_inkey;
}

GVariant* sat_manager_get_input_noti(struct custom_data *ctx, const char *plugin_name, struct tel_sat_get_input_tlv* get_input_tlv)
{
	TcorePlugin *plg = NULL;
	GVariant *get_input = NULL;
	struct sat_manager_queue_data q_data;

	gint command_id = 0, input_character_mode = 0;
	gushort text_len = 0, def_text_len = 0;
	gint rsp_len_min = 0, rsp_len_max = 0;
	gboolean b_numeric = FALSE, b_help_info = FALSE, b_echo_input = FALSE;
	gchar text[SAT_TEXT_STRING_LEN_MAX], def_text[SAT_TEXT_STRING_LEN_MAX];
	GVariant *icon_id = NULL;

	dbg("interpreting get input notification");
	memset(&text, 0 , SAT_TEXT_STRING_LEN_MAX);

	plg = tcore_server_find_plugin(ctx->server, plugin_name);
	if (!plg){
		dbg("there is no valid plugin at this point");
		return NULL;
	}

	if(get_input_tlv->icon_id.is_exist && get_input_tlv->icon_id.icon_qualifer == ICON_QUALI_NOT_SELF_EXPLANATORY ){

		struct treq_sat_terminal_rsp_data *tr = NULL;
		dbg("[SAT]  exceptional case to fix gcf case 2.4 command not understood");

		tr = (struct treq_sat_terminal_rsp_data *)calloc(1, sizeof(struct treq_sat_terminal_rsp_data));
		tr->cmd_number = get_input_tlv->command_detail.cmd_num;
		tr->cmd_type = get_input_tlv->command_detail.cmd_type;

		memcpy((void*)&tr->terminal_rsp_data.get_input.command_detail, &get_input_tlv->command_detail, sizeof(struct tel_sat_cmd_detail_info));
		tr->terminal_rsp_data.get_input.device_id.src = get_input_tlv->device_id.dest;
		tr->terminal_rsp_data.get_input.device_id.dest = get_input_tlv->device_id.src;
		tr->terminal_rsp_data.get_input.result_type = RESULT_COMMAND_DATA_NOT_UNDERSTOOD_BY_ME;

		sat_manager_send_terminal_response(ctx->comm, plg, tr);

		g_free(tr);
		return NULL;
	}

	dbg( "[SAT]  is SMS7 packing required [%d]",get_input_tlv->command_detail.cmd_qualifier.get_input.user_input_unpacked_format);

	//input mode
	input_character_mode =  get_input_tlv->command_detail.cmd_qualifier.get_input.alphabet_type;

	//numeric
	b_numeric = !get_input_tlv->command_detail.cmd_qualifier.get_input.alphabet_set;

	//help info
	b_help_info = get_input_tlv->command_detail.cmd_qualifier.get_input.help_info;

	//echo input
	b_echo_input = get_input_tlv->command_detail.cmd_qualifier.get_input.me_echo_user_input;
	dbg("numeric (%d), help info(%d), echo input(%d)", b_numeric, b_help_info, b_echo_input);

	//text & text len
	if(get_input_tlv->text.string_length){
		tcore_util_convert_string_to_utf8((unsigned char*)&text,(unsigned short *)&text_len,
				get_input_tlv->text.dcs.a_format ,
				(unsigned char*)&get_input_tlv->text.string,
				(unsigned short)get_input_tlv->text.string_length);
		dbg("get input text(%s)",text);
	}
	else{
		memcpy(text,"", 1);
		text_len = 0;
	}

	//response length min & max
	rsp_len_min = get_input_tlv->rsp_len.min;
	rsp_len_max = get_input_tlv->rsp_len.max;

	//default text & default text len
	if(get_input_tlv->default_text.string_length){
		tcore_util_convert_string_to_utf8((unsigned char*)&def_text,(unsigned short *)&def_text_len,
				get_input_tlv->text.dcs.a_format ,
				(unsigned char*)&get_input_tlv->default_text.string,
				(unsigned short)get_input_tlv->default_text.string_length);
		dbg("get input default text(%s)",text);
	}

	//enqueue data and generate cmd_id
	memset(&q_data, 0x00, sizeof(struct sat_manager_queue_data));
	q_data.cmd_type = SAT_PROATV_CMD_GET_INPUT;
	memcpy((void*)&(q_data.cmd_data.getInputInd), get_input_tlv, sizeof(struct tel_sat_get_input_tlv));
	sat_manager_enqueue_cmd(ctx, &q_data);
	command_id = q_data.cmd_id;

	icon_id = g_variant_new("()");

	get_input = g_variant_new("(iibbbsiiisiv)", command_id, input_character_mode, b_numeric, b_help_info,
			b_echo_input, text, text_len, rsp_len_max, rsp_len_min, def_text, def_text_len, icon_id);

	return get_input;
}

GVariant* sat_manager_play_tone_noti(struct custom_data *ctx, const char *plugin_name, struct tel_sat_play_tone_tlv* play_tone_tlv)
{
	TcorePlugin *plg = NULL;
	GVariant *play_tone = NULL;
	struct sat_manager_queue_data q_data;

	gint command_id = 0, tone_type = 0, duration = 0, tmp_duration = 0;
	gushort text_len = 0;
	gchar text[SAT_TEXT_STRING_LEN_MAX];
	GVariant *icon_id = NULL;

	dbg("interpreting play tone notification");
	memset(&text, 0 , SAT_TEXT_STRING_LEN_MAX);

	plg = tcore_server_find_plugin(ctx->server, plugin_name);
	if (!plg){
		dbg("there is no valid plugin at this point");
		return NULL;
	}

	if( (play_tone_tlv->icon_id.is_exist) && ( play_tone_tlv->icon_id.icon_qualifer == ICON_QUALI_NOT_SELF_EXPLANATORY)
			&& (!play_tone_tlv->alpha_id.is_exist || !play_tone_tlv->alpha_id.alpha_data_len))
	{
		struct treq_sat_terminal_rsp_data *tr = NULL;
		dbg("[SAT]  exceptional case to fix gcf case 2.4 command not understood");

		tr = (struct treq_sat_terminal_rsp_data *)calloc(1, sizeof(struct treq_sat_terminal_rsp_data));
		tr->cmd_number = play_tone_tlv->command_detail.cmd_num;
		tr->cmd_type = play_tone_tlv->command_detail.cmd_type;

		memcpy((void*)&tr->terminal_rsp_data.play_tone.command_detail, &play_tone_tlv->command_detail, sizeof(struct tel_sat_cmd_detail_info));
		tr->terminal_rsp_data.play_tone.device_id.src = DEVICE_ID_ME;
		tr->terminal_rsp_data.play_tone.device_id.dest = play_tone_tlv->device_id.src;
		tr->terminal_rsp_data.play_tone.result_type = RESULT_COMMAND_DATA_NOT_UNDERSTOOD_BY_ME;

		sat_manager_send_terminal_response(ctx->comm, plg, tr);
		g_free(tr);

		return NULL;
	}

	//text and text len
	if( play_tone_tlv->alpha_id.is_exist && play_tone_tlv->alpha_id.alpha_data_len){
		tcore_util_convert_string_to_utf8((unsigned char*)&text,(unsigned short *)&text_len,
				play_tone_tlv->alpha_id.dcs.a_format,
				(unsigned char*)&play_tone_tlv->alpha_id.alpha_data,
				(unsigned short)play_tone_tlv->alpha_id.alpha_data_len);
		dbg("play tone ui display text (%s)",text);
	}

	//tone type
	tone_type = play_tone_tlv->tone.tone_type;

	//time duration
	duration = SAT_TIME_OUT;
	tmp_duration = _get_time_in_ms(&play_tone_tlv->duration);
	if (tmp_duration > 0)
		duration = tmp_duration;

	//enqueue data and generate cmd_id
	memset(&q_data, 0x00, sizeof(struct sat_manager_queue_data));
	q_data.cmd_type = SAT_PROATV_CMD_PLAY_TONE;
	memcpy((void*)&(q_data.cmd_data.play_tone), play_tone_tlv, sizeof(struct tel_sat_play_tone_tlv));
	sat_manager_enqueue_cmd(ctx, &q_data);
	command_id = q_data.cmd_id;

	icon_id = g_variant_new("()");

	play_tone = g_variant_new("(isivii)", command_id, text, text_len, icon_id, tone_type, duration);

	return play_tone;
}

GVariant* sat_manager_send_sms_noti(struct custom_data *ctx, const char *plugin_name, struct tel_sat_send_sms_tlv* send_sms_tlv)
{
	TcorePlugin *plg = NULL;
	GVariant *send_sms = NULL;
	struct sat_manager_queue_data q_data;

	int index = 0;
	gint command_id = 0, ton = 0, npi = 0, tpdu_type = 0;
	gboolean b_packing_required = FALSE;
	gushort text_len = 0;
	gint number_len = 0, tpdu_data_len = 0;
	gchar text[SAT_TEXT_STRING_LEN_MAX], dialling_number[SAT_DIALING_NUMBER_LEN_MAX];
	GVariantBuilder *builder = NULL;
	GVariant *tpdu_data = NULL;
	GVariant *icon_id = NULL;

	dbg("interpreting send sms notification");
	memset(&text, 0 , SAT_TEXT_STRING_LEN_MAX);
	memset(&dialling_number, 0 , SAT_DIALING_NUMBER_LEN_MAX);

	plg = tcore_server_find_plugin(ctx->server, plugin_name);
	if (!plg){
		dbg("there is no valid plugin at this point");
		return NULL;
	}

	if( (send_sms_tlv->icon_id.is_exist) && ( send_sms_tlv->icon_id.icon_qualifer == ICON_QUALI_NOT_SELF_EXPLANATORY)
			&& (!send_sms_tlv->alpha_id.is_exist || !send_sms_tlv->alpha_id.alpha_data_len))
	{
		struct treq_sat_terminal_rsp_data *tr = NULL;
		dbg("[SAT]  exceptional case to fix gcf case 2.4 command not understood");

		tr = (struct treq_sat_terminal_rsp_data *)calloc(1, sizeof(struct treq_sat_terminal_rsp_data));
		tr->cmd_number = send_sms_tlv->command_detail.cmd_num;
		tr->cmd_type = send_sms_tlv->command_detail.cmd_type;

		memcpy((void*)&tr->terminal_rsp_data.send_sms.command_detail, &send_sms_tlv->command_detail, sizeof(struct tel_sat_cmd_detail_info));
		tr->terminal_rsp_data.send_sms.device_id.src = DEVICE_ID_ME;
		tr->terminal_rsp_data.send_sms.device_id.dest = send_sms_tlv->device_id.src;
		tr->terminal_rsp_data.send_sms.result_type = RESULT_COMMAND_DATA_NOT_UNDERSTOOD_BY_ME;

		sat_manager_send_terminal_response(ctx->comm, plg, tr);
		g_free(tr);

		return NULL;
	}

	//text and text len
	if( send_sms_tlv->alpha_id.is_exist && send_sms_tlv->alpha_id.alpha_data_len){
		tcore_util_convert_string_to_utf8((unsigned char*)&text,(unsigned short *)&text_len,
				send_sms_tlv->alpha_id.dcs.a_format,
				(unsigned char*)&send_sms_tlv->alpha_id.alpha_data,
				(unsigned short)send_sms_tlv->alpha_id.alpha_data_len);
		dbg("send sms ui display text (%s)",text);
	}
	else{
		memcpy(text,"",1);
		text_len = 0;
	}

	//packing required
	b_packing_required = send_sms_tlv->command_detail.cmd_qualifier.send_sms.packing_by_me_required;

	//address : ton, npi, dialling number, number len
	ton = send_sms_tlv->address.ton;
	npi = send_sms_tlv->address.npi;
	number_len = send_sms_tlv->address.dialing_number_len;
	memcpy(dialling_number, send_sms_tlv->address.dialing_number, SAT_DIALING_NUMBER_LEN_MAX);

	//tpdu data : type, data, data len
	tpdu_type = send_sms_tlv->sms_tpdu.tpdu_type;
	tpdu_data_len = send_sms_tlv->sms_tpdu.data_len;
	builder = g_variant_builder_new(G_VARIANT_TYPE ("ay"));
	for (index = 0; index < tpdu_data_len; index++) {
		g_variant_builder_add(builder, "y", send_sms_tlv->sms_tpdu.data[index]);
	}
	tpdu_data = g_variant_builder_end(builder);

	//enqueue data and generate cmd_id
	memset(&q_data, 0x00, sizeof(struct sat_manager_queue_data));
	q_data.cmd_type = SAT_PROATV_CMD_SEND_SMS;
	memcpy((void*)&(q_data.cmd_data.sendSMSInd), send_sms_tlv, sizeof(struct tel_sat_send_sms_tlv));
	sat_manager_enqueue_cmd(ctx, &q_data);
	command_id = q_data.cmd_id;

	icon_id = g_variant_new("()");

	send_sms = g_variant_new("(isivbiisiivi)", command_id, text, text_len, icon_id, b_packing_required,
			ton, npi, dialling_number, number_len, tpdu_type, tpdu_data, tpdu_data_len);

	return send_sms;
}

GVariant* sat_manager_send_ss_noti(struct custom_data *ctx, const char *plugin_name, struct tel_sat_send_ss_tlv* send_ss_tlv)
{
	TcorePlugin *plg = NULL;
	GVariant *send_ss = NULL;
	struct sat_manager_queue_data q_data;

	gint command_id = 0, ton = 0, npi = 0;
	gushort text_len = 0;
	guchar ss_str_len = 0;
	gchar text[SAT_TEXT_STRING_LEN_MAX], ss_string[SAT_SS_STRING_LEN_MAX];
	GVariant *icon_id = NULL;

	dbg("interpreting send ss notification");
	memset(&text, 0 , SAT_TEXT_STRING_LEN_MAX);
	memset(&ss_string, 0 , SAT_SS_STRING_LEN_MAX);

	plg = tcore_server_find_plugin(ctx->server, plugin_name);
	if (!plg){
		dbg("there is no valid plugin at this point");
		return NULL;
	}

	if( (send_ss_tlv->icon_id.is_exist) && ( send_ss_tlv->icon_id.icon_qualifer == ICON_QUALI_NOT_SELF_EXPLANATORY)
			&& (!send_ss_tlv->alpha_id.is_exist || !send_ss_tlv->alpha_id.alpha_data_len))
	{
		struct treq_sat_terminal_rsp_data *tr = NULL;
		dbg("[SAT]  exceptional case to fix gcf case 2.4 command not understood");

		tr = (struct treq_sat_terminal_rsp_data *)calloc(1, sizeof(struct treq_sat_terminal_rsp_data));
		tr->cmd_number = send_ss_tlv->command_detail.cmd_num;
		tr->cmd_type = send_ss_tlv->command_detail.cmd_type;

		memcpy((void*)&tr->terminal_rsp_data.send_ss.command_detail, &send_ss_tlv->command_detail, sizeof(struct tel_sat_cmd_detail_info));
		tr->terminal_rsp_data.send_ss.device_id.src = send_ss_tlv->device_id.dest;
		tr->terminal_rsp_data.send_ss.device_id.dest = send_ss_tlv->device_id.src;
		tr->terminal_rsp_data.send_ss.result_type = RESULT_COMMAND_DATA_NOT_UNDERSTOOD_BY_ME;

		sat_manager_send_terminal_response(ctx->comm, plg, tr);
		g_free(tr);

		return NULL;
	}

	//text and text len
	if( send_ss_tlv->alpha_id.is_exist && send_ss_tlv->alpha_id.alpha_data_len){
		tcore_util_convert_string_to_utf8((unsigned char*)&text,(unsigned short *)&text_len,
				send_ss_tlv->alpha_id.dcs.a_format,
				(unsigned char*)&send_ss_tlv->alpha_id.alpha_data,
				(unsigned short)send_ss_tlv->alpha_id.alpha_data_len);
		dbg("send ss ui display text (%s)",text);
	}

	//ss string: ton, npi, ss string len, ss string
	ton = send_ss_tlv->ss_string.ton;
	npi = send_ss_tlv->ss_string.npi;
	ss_str_len = send_ss_tlv->ss_string.string_len;
	memcpy(ss_string, send_ss_tlv->ss_string.ss_string, SAT_SS_STRING_LEN_MAX);

	//enqueue data and generate cmd_id
	memset(&q_data, 0x00, sizeof(struct sat_manager_queue_data));
	q_data.cmd_type = SAT_PROATV_CMD_SEND_SS;
	memcpy((void*)&(q_data.cmd_data.send_ss), send_ss_tlv, sizeof(struct tel_sat_send_ss_tlv));
	sat_manager_enqueue_cmd(ctx, &q_data);
	command_id = q_data.cmd_id;

	icon_id = g_variant_new("()");

	send_ss = g_variant_new("(isiviiis)", command_id, text, text_len, icon_id,
			ton, npi, ss_str_len, ss_string);

	return send_ss;
}

GVariant* sat_manager_send_ussd_noti(struct custom_data *ctx, const char *plugin_name, struct tel_sat_send_ussd_tlv* send_ussd_tlv)
{
	TcorePlugin *plg = NULL;
	GVariant *send_ussd = NULL;
	struct sat_manager_queue_data q_data;

	gint command_id = 0;	
	gushort text_len = 0, ussd_str_len = 0;
	gchar text[SAT_TEXT_STRING_LEN_MAX], ussd_string[SAT_USSD_STRING_LEN_MAX];
	GVariant *icon_id = NULL;

	dbg("interpreting send ussd notification");
	memset(&text, 0 , SAT_TEXT_STRING_LEN_MAX);
	memset(&ussd_string, 0 , SAT_USSD_STRING_LEN_MAX);

	plg = tcore_server_find_plugin(ctx->server, plugin_name);
	if (!plg){
		dbg("there is no valid plugin at this point");
		return NULL;
	}

	if( (send_ussd_tlv->icon_id.is_exist) && ( send_ussd_tlv->icon_id.icon_qualifer == ICON_QUALI_NOT_SELF_EXPLANATORY)
			&& (!send_ussd_tlv->alpha_id.is_exist || !send_ussd_tlv->alpha_id.alpha_data_len))
	{
		struct treq_sat_terminal_rsp_data *tr = NULL;
		dbg("[SAT]  exceptional case to fix gcf case 2.4 command not understood");

		tr = (struct treq_sat_terminal_rsp_data *)calloc(1, sizeof(struct treq_sat_terminal_rsp_data));
		tr->cmd_number = send_ussd_tlv->command_detail.cmd_num;
		tr->cmd_type = send_ussd_tlv->command_detail.cmd_type;

		memcpy((void*)&tr->terminal_rsp_data.send_ussd.command_detail, &send_ussd_tlv->command_detail, sizeof(struct tel_sat_cmd_detail_info));
		tr->terminal_rsp_data.send_ussd.device_id.src = send_ussd_tlv->device_id.dest;
		tr->terminal_rsp_data.send_ussd.device_id.dest = send_ussd_tlv->device_id.src;
		tr->terminal_rsp_data.send_ussd.result_type = RESULT_COMMAND_DATA_NOT_UNDERSTOOD_BY_ME;

		sat_manager_send_terminal_response(ctx->comm, plg, tr);
		g_free(tr);

		return NULL;
	}

	//text and text len
	if( send_ussd_tlv->alpha_id.is_exist && send_ussd_tlv->alpha_id.alpha_data_len){
		tcore_util_convert_string_to_utf8((unsigned char*)&text,(unsigned short *)&text_len,
				send_ussd_tlv->alpha_id.dcs.a_format,
				(unsigned char*)&send_ussd_tlv->alpha_id.alpha_data,
				(unsigned short)send_ussd_tlv->alpha_id.alpha_data_len);
		dbg("send ussd ui display text (%s)",text);
	}

	//ussd string
	tcore_util_convert_string_to_utf8((unsigned char*)&ussd_string,(unsigned short *)&ussd_str_len,
				send_ussd_tlv->ussd_string.dsc.a_format,
				(unsigned char*)&send_ussd_tlv->ussd_string.ussd_string,
				(unsigned short)send_ussd_tlv->ussd_string.string_len);

	//enqueue data and generate cmd_id
	memset(&q_data, 0x00, sizeof(struct sat_manager_queue_data));
	q_data.cmd_type = SAT_PROATV_CMD_SEND_USSD;
	memcpy((void*)&(q_data.cmd_data.send_ussd), send_ussd_tlv, sizeof(struct tel_sat_send_ussd_tlv));
	sat_manager_enqueue_cmd(ctx, &q_data);
	command_id = q_data.cmd_id;

	icon_id = g_variant_new("()");

	send_ussd = g_variant_new("(isivis)", command_id, text, text_len, icon_id, ussd_str_len, ussd_string);

	return send_ussd;
}

GVariant* sat_manager_setup_call_noti(struct custom_data *ctx, const char *plugin_name, struct tel_sat_setup_call_tlv* setup_call_tlv)
{
	TcorePlugin *plg = NULL;
	GVariant *setup_call = NULL;
	struct sat_manager_queue_data q_data;

	gushort text_len = 0, confirm_text_len = 0;
	gint command_id = 0, call_type = 0, duration = 0;
	gchar confirm_text[SAT_TEXT_STRING_LEN_MAX], text[SAT_TEXT_STRING_LEN_MAX], call_number[SAT_DIALING_NUMBER_LEN_MAX];
	GVariant *icon_id = NULL;

	dbg("interpreting setup call notification");
	memset(&confirm_text, 0 , SAT_TEXT_STRING_LEN_MAX);
	memset(&text, 0 , SAT_TEXT_STRING_LEN_MAX);
	memset(&call_number, 0 , SAT_DIALING_NUMBER_LEN_MAX);

	plg = tcore_server_find_plugin(ctx->server, plugin_name);
	if (!plg){
		dbg("there is no valid plugin at this point");
		return NULL;
	}

	if(setup_call_tlv->duration.time_interval > 0)
	{
		struct treq_sat_terminal_rsp_data *tr = NULL;
		dbg("[SAT] redial is not supported.\n");

		tr = g_new0(struct treq_sat_terminal_rsp_data, 1);
		tr->cmd_number = setup_call_tlv->command_detail.cmd_num;
		tr->cmd_type = setup_call_tlv->command_detail.cmd_type;

		memcpy((void*)&tr->terminal_rsp_data.setup_call.command_detail, &setup_call_tlv->command_detail, sizeof(struct tel_sat_cmd_detail_info));
		tr->terminal_rsp_data.setup_call.device_id.src = DEVICE_ID_ME;
		tr->terminal_rsp_data.setup_call.device_id.dest = setup_call_tlv->device_id.src;
		tr->terminal_rsp_data.setup_call.result_type = RESULT_BEYOND_ME_CAPABILITIES;

		sat_manager_send_terminal_response(ctx->comm, plg, tr);
		g_free(tr);
		return NULL;
	}

	//check for subaddress field
	if(setup_call_tlv->subaddress.subaddress_len > 0)
	{
		struct treq_sat_terminal_rsp_data *tr = NULL;
		dbg("[SAT] Sub address is not supported > 0)");

		tr = (struct treq_sat_terminal_rsp_data *)calloc(1, sizeof(struct treq_sat_terminal_rsp_data));
		tr->cmd_number = setup_call_tlv->command_detail.cmd_num;
		tr->cmd_type = setup_call_tlv->command_detail.cmd_type;

		memcpy((void*)&tr->terminal_rsp_data.setup_call.command_detail, &setup_call_tlv->command_detail, sizeof(struct tel_sat_cmd_detail_info));
		tr->terminal_rsp_data.setup_call.device_id.src = DEVICE_ID_ME;
		tr->terminal_rsp_data.setup_call.device_id.dest = setup_call_tlv->device_id.src;
		tr->terminal_rsp_data.setup_call.result_type = RESULT_BEYOND_ME_CAPABILITIES;

		sat_manager_send_terminal_response(ctx->comm, plg, tr);
		g_free(tr);
		return NULL;
	}

	//call type
	call_type = setup_call_tlv->command_detail.cmd_qualifier.setup_call.setup_call;

	//call display data
	if(setup_call_tlv->call_setup_alpha_id.alpha_data_len != 0){
		tcore_util_convert_string_to_utf8((unsigned char*)&text,(unsigned short *)&text_len,
				setup_call_tlv->call_setup_alpha_id.dcs.a_format,
				(unsigned char*)&setup_call_tlv->call_setup_alpha_id.alpha_data,
				(unsigned short)setup_call_tlv->call_setup_alpha_id.alpha_data_len);
	}
	else{
		memcpy(text, setup_call_tlv->address.dialing_number, setup_call_tlv->address.dialing_number_len);
		text_len = setup_call_tlv->address.dialing_number_len;
	}
	dbg("setup call display text (%s)",text);

	if(setup_call_tlv->user_confirm_alpha_id.alpha_data_len != 0){
		tcore_util_convert_string_to_utf8((unsigned char*)&confirm_text,(unsigned short *)&confirm_text_len,
				setup_call_tlv->user_confirm_alpha_id.dcs.a_format,
				(unsigned char*)&setup_call_tlv->user_confirm_alpha_id.alpha_data,
				(unsigned short)setup_call_tlv->user_confirm_alpha_id.alpha_data_len);
	}

	//call number
	if(setup_call_tlv->address.ton == TON_INTERNATIONAL){
		call_number[0] = '+';
		memcpy(&call_number[1],setup_call_tlv->address.dialing_number, setup_call_tlv->address.dialing_number_len);
	}
	else{
		memcpy(call_number,setup_call_tlv->address.dialing_number, setup_call_tlv->address.dialing_number_len);
	}
	dbg("setup call call number(%s)",setup_call_tlv->address.dialing_number);

	//duration
	if(setup_call_tlv->duration.time_interval > 0)
		duration = _get_time_in_ms(&setup_call_tlv->duration);

	//enqueue data and generate cmd_id
	memset(&q_data, 0x00, sizeof(struct sat_manager_queue_data));
	q_data.cmd_type = SAT_PROATV_CMD_SETUP_CALL;
	memcpy((void*)&(q_data.cmd_data.setup_call), setup_call_tlv, sizeof(struct tel_sat_setup_call_tlv));
	sat_manager_enqueue_cmd(ctx, &q_data);
	command_id = q_data.cmd_id;

	icon_id = g_variant_new("()");

	setup_call = g_variant_new("(isisivisi)", command_id, confirm_text, confirm_text_len, text, text_len, icon_id, call_type, call_number, duration);

	return setup_call;
}

GVariant* sat_manager_setup_event_list_noti(struct custom_data *ctx, const char *plugin_name, struct tel_sat_setup_event_list_tlv *event_list_tlv)
{
	TcorePlugin *plg = NULL;
	GVariant *event_list = NULL;

	int index = 0;
	gboolean rv = FALSE;
	gint event_cnt = 0;
	GVariantBuilder *builder = NULL;
	GVariant *evt_list = NULL;
	struct treq_sat_terminal_rsp_data *tr = NULL;

	dbg("interpreting event list notification");

	plg = tcore_server_find_plugin(ctx->server, plugin_name);
	if (!plg){
		dbg("there is no valid plugin at this point");
		return NULL;
	}

	//event cnt
	event_cnt = event_list_tlv->event_list.event_list_cnt;
	dbg("event cnt(%d)", event_cnt);

	//get event
	builder = g_variant_builder_new(G_VARIANT_TYPE ("ai"));
	for (index = 0; index < event_cnt; index++) {
		g_variant_builder_add(builder, "i", event_list_tlv->event_list.evt_list[index]);
	}
	evt_list = g_variant_builder_end(builder);

	event_list = g_variant_new("(iv)", event_cnt, evt_list);

	//send TR - does not need from application's response
	tr = (struct treq_sat_terminal_rsp_data *)calloc(1, sizeof(struct treq_sat_terminal_rsp_data));
	tr->cmd_number = event_list_tlv->command_detail.cmd_num;
	tr->cmd_type = event_list_tlv->command_detail.cmd_type;
	memcpy((void*)&tr->terminal_rsp_data.setup_event_list.command_detail, &event_list_tlv->command_detail, sizeof(struct tel_sat_cmd_detail_info));
	tr->terminal_rsp_data.setup_event_list.device_id.src = event_list_tlv->device_id.dest;
	tr->terminal_rsp_data.setup_event_list.device_id.dest = event_list_tlv->device_id.src;
	tr->terminal_rsp_data.setup_event_list.result_type = RESULT_SUCCESS;
	tr->terminal_rsp_data.setup_event_list.me_problem_type = ME_PROBLEM_NO_SPECIFIC_CAUSE;

	rv = sat_manager_check_availiable_event_list(event_list_tlv);
	sat_manager_send_terminal_response(ctx->comm, plg, tr);
	g_free(tr);

	return event_list;
}

GVariant* sat_manager_setup_idle_mode_text_noti(struct custom_data *ctx, const char *plugin_name, struct tel_sat_setup_idle_mode_text_tlv *idle_mode_tlv)
{
	TcorePlugin *plg = NULL;
	GVariant *idle_mode = NULL;
	struct sat_manager_queue_data q_data;

	gint command_id = 0;
	gushort text_len = 0;
	gchar text[SAT_TEXT_STRING_LEN_MAX];
	GVariant *icon_id = NULL;

	dbg("interpreting setup idle mode text notification");
	memset(&text, 0 , SAT_TEXT_STRING_LEN_MAX);

	plg = tcore_server_find_plugin(ctx->server, plugin_name);
	if (!plg){
		dbg("there is no valid plugin at this point");
		return NULL;
	}

	if( ((idle_mode_tlv->icon_id.is_exist) && ( idle_mode_tlv->icon_id.icon_qualifer == ICON_QUALI_NOT_SELF_EXPLANATORY))
			&& idle_mode_tlv->text.string_length == 0 )
	{
		struct treq_sat_terminal_rsp_data *tr = NULL;
		dbg("[SAT]  exceptional case to fix gcf case 2.4 command not understood");

		tr = (struct treq_sat_terminal_rsp_data *)calloc(1, sizeof(struct treq_sat_terminal_rsp_data));
		tr->cmd_number = idle_mode_tlv->command_detail.cmd_num;
		tr->cmd_type = idle_mode_tlv->command_detail.cmd_type;

		memcpy((void*)&tr->terminal_rsp_data.setup_idle_mode_text.command_detail, &idle_mode_tlv->command_detail, sizeof(struct tel_sat_cmd_detail_info));
		tr->terminal_rsp_data.setup_idle_mode_text.device_id.src = idle_mode_tlv->device_id.dest;
		tr->terminal_rsp_data.setup_idle_mode_text.device_id.dest = idle_mode_tlv->device_id.src;
		tr->terminal_rsp_data.setup_idle_mode_text.result_type = RESULT_COMMAND_DATA_NOT_UNDERSTOOD_BY_ME;

		sat_manager_send_terminal_response(ctx->comm, plg, tr);
		g_free(tr);
		return NULL;
	}

	tcore_util_convert_string_to_utf8((unsigned char*)&text,(unsigned short *)&text_len,
			idle_mode_tlv->text.dcs.a_format,
			(unsigned char*)&idle_mode_tlv->text.string,
			(unsigned short)idle_mode_tlv->text.string_length);

	dbg("setup idle mode text display text (%s)",text);

	//enqueue data and generate cmd_id
	memset(&q_data, 0x00, sizeof(struct sat_manager_queue_data));
	q_data.cmd_type = SAT_PROATV_CMD_SETUP_IDLE_MODE_TEXT;
	memcpy((void*)&(q_data.cmd_data.idle_mode), idle_mode_tlv, sizeof(struct tel_sat_setup_idle_mode_text_tlv));
	sat_manager_enqueue_cmd(ctx, &q_data);
	command_id = q_data.cmd_id;

	icon_id = g_variant_new("()");

	idle_mode = g_variant_new("(isiv)", command_id, text, text_len, icon_id);

	return idle_mode;
}

GVariant* sat_manager_open_channel_noti(struct custom_data *ctx, const char *plugin_name, struct tel_sat_open_channel_tlv *open_channel_tlv)
{
	TcorePlugin *plg = NULL;
	GVariant *open_channel = NULL;
	struct sat_manager_queue_data q_data;

	gint command_id = 0, bearer_type = 0, protocol_type = 0, dest_addr_type = 0;
	gboolean immediate_link = FALSE, auto_reconnection = FALSE, bg_mode = FALSE;
	gushort text_len = 0;
	gint buffer_size = 0, port_number = 0;
	gchar text[SAT_ALPHA_ID_LEN_MAX], dest_address[SAT_OTHER_ADDR_LEN_MAX];
	GVariant *icon_id = NULL;
	GVariant *bearer_param = NULL;
	GVariant *bearer_detail = NULL;

	dbg("interpreting open channel notification");
	memset(&text, 0 , SAT_ALPHA_ID_LEN_MAX);
	memset(&dest_address, 0 , SAT_OTHER_ADDR_LEN_MAX);

	plg = tcore_server_find_plugin(ctx->server, plugin_name);
	if (!plg){
		dbg("there is no valid plugin at this point");
		return NULL;
	}

	//immediate link
	immediate_link = open_channel_tlv->command_detail.cmd_qualifier.open_channel.immediate_link;

	//auto reconnection
	auto_reconnection = open_channel_tlv->command_detail.cmd_qualifier.open_channel.automatic_reconnection;

	//back ground mode
	bg_mode = open_channel_tlv->command_detail.cmd_qualifier.open_channel.background_mode;

	//open channel text
	if(open_channel_tlv->alpha_id.is_exist && open_channel_tlv->alpha_id.alpha_data_len > 0)
		tcore_util_convert_string_to_utf8((unsigned char*)&text,(unsigned short *)&text_len,
				open_channel_tlv->alpha_id.dcs.a_format,
				(unsigned char*)&open_channel_tlv->alpha_id.alpha_data,
				(unsigned short)open_channel_tlv->alpha_id.alpha_data_len);
	dbg("open channel text(%s)",text);

	//buffer size
	buffer_size = open_channel_tlv->buffer_size.size[0];
	buffer_size = buffer_size << 8;
	buffer_size += open_channel_tlv->buffer_size.size[1];
	//memcpy(&buffer_size, open_channel_tlv->buffer_size.size, sizeof(unsigned char)*2);
	dbg("buffer size(%d)", buffer_size);

	//interface transport level
	protocol_type = open_channel_tlv->interface_transport_level.protocol_type;
	port_number = open_channel_tlv->interface_transport_level.port_number;

	//data destination address
	dest_addr_type = open_channel_tlv->data_destination_address.address_type;
	memcpy(dest_address, open_channel_tlv->data_destination_address.address, open_channel_tlv->data_destination_address.address_len);
	dbg("destination IP address (%s)", dest_address);

	//bearer type
	bearer_type = open_channel_tlv->bearer_desc.bearer_type;

	//bearer param & bearer detail
	switch(bearer_type){
		case BEARER_CSD:{
			//bearer param
			gint data_rate = 0, service_type = 0, conn_element_type = 0;

			//bearer detail
			gint ton = 0, npi = 0, time_duration1 = 0, time_duration2 = 0, other_addr_type = 0;
			gushort login_len = 0, pwd_len = 0;
			gchar dialling_number[SAT_DIALING_NUMBER_LEN_MAX], sub_addr[SAT_SUB_ADDR_LEN_MAX];
			gchar other_address[SAT_OTHER_ADDR_LEN_MAX];
			gchar login[SAT_TEXT_STRING_LEN_MAX], pwd[SAT_TEXT_STRING_LEN_MAX];

			memset(&dialling_number, 0 , SAT_DIALING_NUMBER_LEN_MAX);
			memset(&sub_addr, 0 , SAT_SUB_ADDR_LEN_MAX);
			memset(&other_address, 0 , SAT_OTHER_ADDR_LEN_MAX);
			memset(&login, 0 , SAT_TEXT_STRING_LEN_MAX);
			memset(&pwd, 0 , SAT_TEXT_STRING_LEN_MAX);

			//bearer parameter
			data_rate = open_channel_tlv->bearer_desc.bearer_parameter.cs_bearer_param.data_rate;
			service_type = open_channel_tlv->bearer_desc.bearer_parameter.cs_bearer_param.service_type;
			conn_element_type = open_channel_tlv->bearer_desc.bearer_parameter.cs_bearer_param.connection_element_type;

			bearer_param = g_variant_new("(iii)", data_rate, service_type, conn_element_type);

			//bearer detail
			ton = open_channel_tlv->bearer_detail.cs_bearer.address.ton;
			npi = open_channel_tlv->bearer_detail.cs_bearer.address.npi;
			memcpy(dialling_number, open_channel_tlv->bearer_detail.cs_bearer.address.dialing_number, open_channel_tlv->bearer_detail.cs_bearer.address.dialing_number_len);

			memcpy(sub_addr, open_channel_tlv->bearer_detail.cs_bearer.subaddress.subaddress, open_channel_tlv->bearer_detail.cs_bearer.subaddress.subaddress_len);

			time_duration1 = _get_time_in_ms(&open_channel_tlv->bearer_detail.cs_bearer.duration1);
			time_duration2 = _get_time_in_ms(&open_channel_tlv->bearer_detail.cs_bearer.duration2);

			other_addr_type = open_channel_tlv->bearer_detail.cs_bearer.other_address.address_type;
			memcpy(other_address, open_channel_tlv->bearer_detail.cs_bearer.other_address.address, open_channel_tlv->bearer_detail.cs_bearer.other_address.address_len);

			tcore_util_convert_string_to_utf8((unsigned char*) &login, (unsigned short *) &login_len,
					open_channel_tlv->bearer_detail.cs_bearer.text_user_login.dcs.a_format,
					(unsigned char*) &open_channel_tlv->bearer_detail.cs_bearer.text_user_login.string,
					(unsigned short) open_channel_tlv->bearer_detail.cs_bearer.text_user_login.string_length);

			tcore_util_convert_string_to_utf8((unsigned char*) &pwd, (unsigned short *) &pwd_len,
					open_channel_tlv->bearer_detail.cs_bearer.text_user_pwd.dcs.a_format,
					(unsigned char*) &open_channel_tlv->bearer_detail.cs_bearer.text_user_pwd.string,
					(unsigned short) open_channel_tlv->bearer_detail.cs_bearer.text_user_pwd.string_length);

			bearer_detail= g_variant_new("(iissiiisss)", ton, npi, dialling_number, sub_addr, time_duration1, time_duration2,
					other_addr_type, other_address, login, pwd);
		} break;
		case BEARER_GPRS:{
			//bearer param
			gint precedence_class = 0, delay_class = 0, reliability_class = 0;
			gint peak_class = 0, mean_class = 0, pdp_type = 0;

			//bearer detail
			gint other_addr_type = 0;
			gushort login_len = 0, pwd_len = 0;
			gchar network_access_name[SAT_NET_ACC_NAM_LEN_MAX];
			gchar other_address[SAT_OTHER_ADDR_LEN_MAX];
			gchar login[SAT_TEXT_STRING_LEN_MAX], pwd[SAT_TEXT_STRING_LEN_MAX];

			memset(&network_access_name, 0 , SAT_NET_ACC_NAM_LEN_MAX);
			memset(&other_address, 0 , SAT_OTHER_ADDR_LEN_MAX);
			memset(&login, 0 , SAT_TEXT_STRING_LEN_MAX);
			memset(&pwd, 0 , SAT_TEXT_STRING_LEN_MAX);

			//bearer parameter
			precedence_class = open_channel_tlv->bearer_desc.bearer_parameter.ps_bearer_param.precedence_class;
			delay_class = open_channel_tlv->bearer_desc.bearer_parameter.ps_bearer_param.delay_class;
			reliability_class = open_channel_tlv->bearer_desc.bearer_parameter.ps_bearer_param.reliability_class;
			peak_class = open_channel_tlv->bearer_desc.bearer_parameter.ps_bearer_param.peak_throughput_class;
			mean_class = open_channel_tlv->bearer_desc.bearer_parameter.ps_bearer_param.mean_throughput_class;
			pdp_type = open_channel_tlv->bearer_desc.bearer_parameter.ps_bearer_param.pdp_type;

			bearer_param = g_variant_new("(iiiiii)", precedence_class, delay_class, reliability_class, peak_class, mean_class, pdp_type);

			memcpy(network_access_name, open_channel_tlv->bearer_detail.ps_bearer.network_access_name.network_access_name, open_channel_tlv->bearer_detail.ps_bearer.network_access_name.length);

			other_addr_type = open_channel_tlv->bearer_detail.ps_bearer.other_address.address_type;
			memcpy(other_address, open_channel_tlv->bearer_detail.ps_bearer.other_address.address, open_channel_tlv->bearer_detail.ps_bearer.other_address.address_len);

			tcore_util_convert_string_to_utf8((unsigned char*) &login, (unsigned short *) &login_len,
					open_channel_tlv->bearer_detail.ps_bearer.text_user_login.dcs.a_format,
					(unsigned char*) &open_channel_tlv->bearer_detail.ps_bearer.text_user_login.string,
					(unsigned short) open_channel_tlv->bearer_detail.ps_bearer.text_user_login.string_length);

			tcore_util_convert_string_to_utf8((unsigned char*) &pwd, (unsigned short *) &pwd_len,
					open_channel_tlv->bearer_detail.ps_bearer.text_user_pwd.dcs.a_format,
					(unsigned char*) &open_channel_tlv->bearer_detail.ps_bearer.text_user_pwd.string,
					(unsigned short) open_channel_tlv->bearer_detail.ps_bearer.text_user_pwd.string_length);

			bearer_detail= g_variant_new("(sisss)", network_access_name, other_addr_type, other_address, login, pwd);

		} break;
		case BEARER_DEFAULT_BEARER_FROM_TRANSPORT_LAYER:{
			//bearer param

			//bearer detail
			gint other_addr_type = 0;
			gushort login_len = 0, pwd_len = 0;
			gchar other_address[SAT_OTHER_ADDR_LEN_MAX];
			gchar login[SAT_TEXT_STRING_LEN_MAX], pwd[SAT_TEXT_STRING_LEN_MAX];

			memset(&other_address, 0 , SAT_OTHER_ADDR_LEN_MAX);
			memset(&login, 0 , SAT_TEXT_STRING_LEN_MAX);
			memset(&pwd, 0 , SAT_TEXT_STRING_LEN_MAX);

			//bearer parameter
			bearer_param = g_variant_new("()");

			other_addr_type = open_channel_tlv->bearer_detail.default_bearer.other_address.address_type;
			memcpy(other_address, open_channel_tlv->bearer_detail.default_bearer.other_address.address, open_channel_tlv->bearer_detail.default_bearer.other_address.address_len);

			tcore_util_convert_string_to_utf8((unsigned char*) &login, (unsigned short *) &login_len,
					open_channel_tlv->bearer_detail.default_bearer.text_user_login.dcs.a_format,
					(unsigned char*) &open_channel_tlv->bearer_detail.default_bearer.text_user_login.string,
					(unsigned short) open_channel_tlv->bearer_detail.default_bearer.text_user_login.string_length);

			tcore_util_convert_string_to_utf8((unsigned char*) &pwd, (unsigned short *) &pwd_len,
					open_channel_tlv->bearer_detail.default_bearer.text_user_pwd.dcs.a_format,
					(unsigned char*) &open_channel_tlv->bearer_detail.default_bearer.text_user_pwd.string,
					(unsigned short) open_channel_tlv->bearer_detail.default_bearer.text_user_pwd.string_length);

			bearer_detail= g_variant_new("(isss)", other_addr_type, other_address, login, pwd);

		} break;
		case BEARER_LOCAL_LINK_TECHNOLOGY_INDEPENDENT:{
			//bearer param

			//bearer detail
			gushort pwd_len = 0;
			gint remote_address_type =0, time_duration1 = 0, time_duration2 = 0;
			gchar remote_address[SAT_REMOTE_ENTITY_ADDR_LEN_MAX];
			gchar pwd[SAT_TEXT_STRING_LEN_MAX];

			memset(&remote_address, 0 , SAT_REMOTE_ENTITY_ADDR_LEN_MAX);
			memset(&pwd, 0 , SAT_TEXT_STRING_LEN_MAX);

			//bearer parameter
			bearer_param = g_variant_new("()");

			time_duration1 = _get_time_in_ms(&open_channel_tlv->bearer_detail.local_bearer.duration1);
			time_duration2 = _get_time_in_ms(&open_channel_tlv->bearer_detail.local_bearer.duration2);

			tcore_util_convert_string_to_utf8((unsigned char*) &pwd, (unsigned short *) &pwd_len,
											open_channel_tlv->bearer_detail.default_bearer.text_user_pwd.dcs.a_format,
											(unsigned char*) &open_channel_tlv->bearer_detail.default_bearer.text_user_pwd.string,
											(unsigned short) open_channel_tlv->bearer_detail.default_bearer.text_user_pwd.string_length);

			remote_address_type = open_channel_tlv->bearer_detail.local_bearer.remote_entity_address.coding_type;
			memcpy(remote_address, open_channel_tlv->bearer_detail.local_bearer.remote_entity_address.remote_entity_address, open_channel_tlv->bearer_detail.local_bearer.remote_entity_address.length);

			bearer_detail= g_variant_new("(iisis)", time_duration1, time_duration2, pwd, remote_address_type, remote_address);

		} break;
		default:
			dbg("invalid bearer data");
			return NULL;
	}//end of switch

	//enqueue data and generate cmd_id
	memset(&q_data, 0x00, sizeof(struct sat_manager_queue_data));
	q_data.cmd_type = SAT_PROATV_CMD_OPEN_CHANNEL;
	memcpy((void*)&(q_data.cmd_data.open_channel), open_channel_tlv, sizeof(struct tel_sat_open_channel_tlv));
	sat_manager_enqueue_cmd(ctx, &q_data);
	command_id = q_data.cmd_id;

	icon_id = g_variant_new("()");

	//execute bip
	//sat_ui_support_exec_bip();

	open_channel = g_variant_new("(isivbbbiviiiisv)", command_id, text, text_len, icon_id, immediate_link, auto_reconnection, bg_mode,
			bearer_type, bearer_param, buffer_size, protocol_type, port_number, dest_addr_type, dest_address, bearer_detail);

	return open_channel;
}

GVariant* sat_manager_close_channel_noti(struct custom_data *ctx, const char *plugin_name, struct tel_sat_close_channel_tlv *close_channel_tlv)
{
	TcorePlugin *plg = NULL;
	GVariant *close_channel = NULL;
	struct sat_manager_queue_data q_data;

	gint command_id = 0, channel_id = 0;
	gushort text_len = 0;
	gchar text[SAT_ALPHA_ID_LEN_MAX];
	GVariant *icon_id = NULL;

	dbg("interpreting close channel notification");
	memset(&text, 0 , SAT_ALPHA_ID_LEN_MAX);

	plg = tcore_server_find_plugin(ctx->server, plugin_name);
	if (!plg){
		dbg("there is no valid plugin at this point");
		return NULL;
	}

	//channel id
	channel_id = close_channel_tlv->device_id.dest;

	//close channel text
	if(close_channel_tlv->alpha_id.is_exist && close_channel_tlv->alpha_id.alpha_data_len > 0)
		tcore_util_convert_string_to_utf8((unsigned char*)&text,(unsigned short *)&text_len,
				close_channel_tlv->alpha_id.dcs.a_format,
				(unsigned char*)&close_channel_tlv->alpha_id.alpha_data,
				(unsigned short)close_channel_tlv->alpha_id.alpha_data_len);
	dbg("close channel text(%s)",text);

	//enqueue data and generate cmd_id
	memset(&q_data, 0x00, sizeof(struct sat_manager_queue_data));
	q_data.cmd_type = SAT_PROATV_CMD_CLOSE_CHANNEL;
	memcpy((void*)&(q_data.cmd_data.close_channel), close_channel_tlv, sizeof(struct tel_sat_close_channel_tlv));
	sat_manager_enqueue_cmd(ctx, &q_data);
	command_id = q_data.cmd_id;

	icon_id = g_variant_new("()");

	close_channel = g_variant_new("(isivi)", command_id, text, text_len, icon_id, channel_id);

	return close_channel;
}

GVariant* sat_manager_receive_data_noti(struct custom_data *ctx, const char *plugin_name, struct tel_sat_receive_channel_tlv *receive_data_tlv)
{
	TcorePlugin *plg = NULL;
	GVariant *receive_data = NULL;
	struct sat_manager_queue_data q_data;

	gint command_id = 0, channel_id = 0;
	gushort text_len = 0;
	gint channel_data_len = 0;
	gchar text[SAT_ALPHA_ID_LEN_MAX];
	GVariant *icon_id = NULL;

	dbg("interpreting receive data notification");
	memset(&text, 0 , SAT_ALPHA_ID_LEN_MAX);

	plg = tcore_server_find_plugin(ctx->server, plugin_name);
	if (!plg){
		dbg("there is no valid plugin at this point");
		return NULL;
	}

	//channel id
	channel_id = receive_data_tlv->device_id.dest;

	//receive data text
	if(receive_data_tlv->alpha_id.is_exist && receive_data_tlv->alpha_id.alpha_data_len > 0)
		tcore_util_convert_string_to_utf8((unsigned char*)&text,(unsigned short *)&text_len,
				receive_data_tlv->alpha_id.dcs.a_format,
				(unsigned char*)&receive_data_tlv->alpha_id.alpha_data,
				(unsigned short)receive_data_tlv->alpha_id.alpha_data_len);
	dbg("receive data text(%s)",text);

	channel_data_len = receive_data_tlv->channel_data_len.data_len;

	//enqueue data and generate cmd_id
	memset(&q_data, 0x00, sizeof(struct sat_manager_queue_data));
	q_data.cmd_type = SAT_PROATV_CMD_RECEIVE_DATA;
	memcpy((void*)&(q_data.cmd_data.receive_data), receive_data_tlv, sizeof(struct tel_sat_receive_channel_tlv));
	sat_manager_enqueue_cmd(ctx, &q_data);
	command_id = q_data.cmd_id;

	icon_id = g_variant_new("()");

	receive_data = g_variant_new("(isivii)", command_id, text, text_len, icon_id, channel_id, channel_data_len);

	return receive_data;
}

GVariant* sat_manager_send_data_noti(struct custom_data *ctx, const char *plugin_name, struct tel_sat_send_channel_tlv *send_data_tlv)
{
	TcorePlugin *plg = NULL;
	GVariant *send_data = NULL;
	struct sat_manager_queue_data q_data;

	int index = 0;
	gint command_id = 0, channel_id = 0, data_len = 0;
	gboolean send_data_immediately = FALSE;
	gushort text_len = 0;
	gchar text[SAT_ALPHA_ID_LEN_MAX];
	GVariantBuilder *builder = NULL;
	GVariant *channel_data = NULL;
	GVariant *icon_id = NULL;

	dbg("interpreting send data notification");
	memset(&text, 0 , SAT_ALPHA_ID_LEN_MAX);

	plg = tcore_server_find_plugin(ctx->server, plugin_name);
	if (!plg){
		dbg("there is no valid plugin at this point");
		return NULL;
	}

	//send data immediately
	send_data_immediately = send_data_tlv->command_detail.cmd_qualifier.send_data.send_data_immediately;

	//channel id
	channel_id = send_data_tlv->device_id.dest;

	//send data text
	if(send_data_tlv->alpha_id.is_exist && send_data_tlv->alpha_id.alpha_data_len > 0)
		tcore_util_convert_string_to_utf8((unsigned char*)&text,(unsigned short *)&text_len,
				send_data_tlv->alpha_id.dcs.a_format,
				(unsigned char*)&send_data_tlv->alpha_id.alpha_data,
				(unsigned short)send_data_tlv->alpha_id.alpha_data_len);
	dbg("send data text(%s)",text);

	//channel data, data len
	data_len = send_data_tlv->channel_data.data_string_len;
	builder = g_variant_builder_new(G_VARIANT_TYPE ("ay"));
	for (index = 0; index < data_len; index++) {
		dbg("send data index(%d) data(0x%x)",index, send_data_tlv->channel_data.data_string[index]);
		g_variant_builder_add(builder, "y", send_data_tlv->channel_data.data_string[index]);
	}
	channel_data = g_variant_builder_end(builder);

	//enqueue data and generate cmd_id
	memset(&q_data, 0x00, sizeof(struct sat_manager_queue_data));
	q_data.cmd_type = SAT_PROATV_CMD_SEND_DATA;
	memcpy((void*)&(q_data.cmd_data.send_data), send_data_tlv, sizeof(struct tel_sat_send_channel_tlv));
	sat_manager_enqueue_cmd(ctx, &q_data);
	command_id = q_data.cmd_id;

	icon_id = g_variant_new("()");

	send_data = g_variant_new("(isivibvi)", command_id, text, text_len, icon_id, channel_id, send_data_immediately, channel_data, data_len);

	return send_data;
}

GVariant* sat_manager_get_channel_status_noti(struct custom_data *ctx, const char *plugin_name, struct tel_sat_get_channel_status_tlv *get_channel_status_tlv)
{
	TcorePlugin *plg = NULL;
	GVariant *get_channel_status = NULL;
	struct sat_manager_queue_data q_data;

	gint command_id = 0;

	dbg("interpreting get channel status notification");

	plg = tcore_server_find_plugin(ctx->server, plugin_name);
	if (!plg){
		dbg("there is no valid plugin at this point");
		return NULL;
	}

	//enqueue data and generate cmd_id
	memset(&q_data, 0x00, sizeof(struct sat_manager_queue_data));
	q_data.cmd_type = SAT_PROATV_CMD_GET_CHANNEL_STATUS;
	memcpy((void*)&(q_data.cmd_data.get_channel_status), get_channel_status_tlv, sizeof(struct tel_sat_get_channel_status_tlv));
	sat_manager_enqueue_cmd(ctx, &q_data);
	command_id = q_data.cmd_id;

	get_channel_status = g_variant_new("(i)", command_id);

	return get_channel_status;
}

GVariant* sat_manager_refresh_noti(struct custom_data *ctx, const char *plugin_name, struct tel_sat_refresh_tlv *refresh_tlv)
{
	TcorePlugin *plg = NULL;
	GVariant *refresh = NULL;
	struct sat_manager_queue_data q_data;

	gint command_id = 0;
	gint refresh_type =0;
	GVariantBuilder *builder = NULL;
	GVariant *file_list = NULL;
	int index = 0;

	dbg("interpreting refresh notification");

	plg = tcore_server_find_plugin(ctx->server, plugin_name);
	if (!plg){
		dbg("there is no valid plugin at this point");
		return NULL;
	}

	refresh_type = refresh_tlv->command_detail.cmd_qualifier.refresh.refresh;

	builder = g_variant_builder_new(G_VARIANT_TYPE ("ai"));
	for (index = 0; index < refresh_tlv->file_list.file_count; index++) {
		g_variant_builder_add(builder, "i", refresh_tlv->file_list.file_id[index]);
	}
	file_list = g_variant_builder_end(builder);

	//enqueue data and generate cmd_id
	memset(&q_data, 0x00, sizeof(struct sat_manager_queue_data));
	q_data.cmd_type = SAT_PROATV_CMD_REFRESH;
	memcpy((void*)&(q_data.cmd_data.refresh), refresh_tlv, sizeof(struct tel_sat_refresh_tlv));
	sat_manager_enqueue_cmd(ctx, &q_data);
	command_id = q_data.cmd_id;

	refresh = g_variant_new("(iiv)", command_id, refresh_type, file_list);

	return refresh;
}

void sat_manager_more_time_noti(struct custom_data *ctx, const char *plugin_name, struct tel_sat_more_time_tlv *more_time_tlv)
{
	TcorePlugin *plg = NULL;
	struct treq_sat_terminal_rsp_data *tr = NULL;

	dbg("interpreting more time notification");

	plg = tcore_server_find_plugin(ctx->server, plugin_name);
	if (!plg){
		dbg("there is no valid plugin at this point");
		return;
	}
	//send TR - does not need from application's response
	tr = (struct treq_sat_terminal_rsp_data *)calloc(1, sizeof(struct treq_sat_terminal_rsp_data));
	tr->cmd_number = more_time_tlv->command_detail.cmd_num;
	tr->cmd_type = more_time_tlv->command_detail.cmd_type;
	memcpy((void*)&tr->terminal_rsp_data.more_time.command_detail, &more_time_tlv->command_detail, sizeof(struct tel_sat_cmd_detail_info));
	tr->terminal_rsp_data.more_time.device_id.src = more_time_tlv->device_id.dest;
	tr->terminal_rsp_data.more_time.device_id.dest = more_time_tlv->device_id.src;
	tr->terminal_rsp_data.more_time.result_type = RESULT_SUCCESS;
	tr->terminal_rsp_data.more_time.me_problem_type = ME_PROBLEM_NO_SPECIFIC_CAUSE;

	sat_manager_send_terminal_response(ctx->comm, plg, tr);
	g_free(tr);

	return;
}

GVariant* sat_manager_send_dtmf_noti(struct custom_data *ctx, const char *plugin_name, struct tel_sat_send_dtmf_tlv *send_dtmf_tlv)
{
	TcorePlugin *plg = NULL;
	GVariant *send_dtmf = NULL;
	struct sat_manager_queue_data q_data;

	gint command_id = 0;
	gushort text_len = 0;
	gint dtmf_str_len =0;
	gchar text[SAT_TEXT_STRING_LEN_MAX], dtmf_str[SAT_DTMF_STRING_LEN_MAX];
	GVariant *icon_id = NULL;

	dbg("interpreting send dtmf notification");
	memset(&text, 0 , SAT_TEXT_STRING_LEN_MAX);
	memset(&dtmf_str, 0 , SAT_DTMF_STRING_LEN_MAX);

	plg = tcore_server_find_plugin(ctx->server, plugin_name);
	if (!plg){
		dbg("there is no valid plugin at this point");
		return NULL;
	}

	if( (send_dtmf_tlv->icon_id.is_exist) && ( send_dtmf_tlv->icon_id.icon_qualifer == ICON_QUALI_NOT_SELF_EXPLANATORY)
			&& (!send_dtmf_tlv->alpha_id.is_exist || !send_dtmf_tlv->alpha_id.alpha_data_len))
	{
		struct treq_sat_terminal_rsp_data *tr = NULL;
		dbg("[SAT]  exceptional case to fix gcf case 2.4 command not understood");

		tr = (struct treq_sat_terminal_rsp_data *)calloc(1, sizeof(struct treq_sat_terminal_rsp_data));
		tr->cmd_number = send_dtmf_tlv->command_detail.cmd_num;
		tr->cmd_type = send_dtmf_tlv->command_detail.cmd_type;

		memcpy((void*)&tr->terminal_rsp_data.send_dtmf.command_detail, &send_dtmf_tlv->command_detail, sizeof(struct tel_sat_cmd_detail_info));
		tr->terminal_rsp_data.send_dtmf.device_id.src = send_dtmf_tlv->device_id.dest;
		tr->terminal_rsp_data.send_dtmf.device_id.dest = send_dtmf_tlv->device_id.src;
		tr->terminal_rsp_data.send_dtmf.result_type = RESULT_COMMAND_DATA_NOT_UNDERSTOOD_BY_ME;

		sat_manager_send_terminal_response(ctx->comm, plg, tr);
		g_free(tr);

		return NULL;
	}

	//text and text len
	if( send_dtmf_tlv->alpha_id.is_exist && send_dtmf_tlv->alpha_id.alpha_data_len){
		tcore_util_convert_string_to_utf8((unsigned char*)&text,(unsigned short *)&text_len,
				send_dtmf_tlv->alpha_id.dcs.a_format,
				(unsigned char*)&send_dtmf_tlv->alpha_id.alpha_data,
				(unsigned short)send_dtmf_tlv->alpha_id.alpha_data_len);
		dbg("send dtmf ui display text (%s)",text);
	}

	//dtmf string len, dtmf string
	dtmf_str_len = send_dtmf_tlv->dtmf_string.dtmf_length;
	memcpy(dtmf_str, send_dtmf_tlv->dtmf_string.dtmf_string, SAT_DTMF_STRING_LEN_MAX);

	//enqueue data and generate cmd_id
	memset(&q_data, 0x00, sizeof(struct sat_manager_queue_data));
	q_data.cmd_type = SAT_PROATV_CMD_SEND_DTMF;
	memcpy((void*)&(q_data.cmd_data.send_dtmf), send_dtmf_tlv, sizeof(struct tel_sat_send_dtmf_tlv));
	sat_manager_enqueue_cmd(ctx, &q_data);
	command_id = q_data.cmd_id;

	icon_id = g_variant_new("()");

	send_dtmf = g_variant_new("(isivis)", command_id, text, text_len, icon_id,
			dtmf_str_len, dtmf_str);

	return send_dtmf;
}

GVariant* sat_manager_launch_browser_noti(struct custom_data *ctx, const char *plugin_name, struct tel_sat_launch_browser_tlv *launch_browser_tlv)
{
	TcorePlugin *plg = NULL;
	GVariant *launch_browser = NULL;
	struct sat_manager_queue_data q_data;

	gint command_id = 0;
	gint browser_launch_type = 0, browser_id = 0;
	gint url_len =0;
	gushort text_len = 0, gateway_proxy_len = 0;
	gchar url[SAT_URL_LEN_MAX], text[SAT_TEXT_STRING_LEN_MAX], gateway_proxy[SAT_TEXT_STRING_LEN_MAX];
	GVariant *icon_id = NULL;

	dbg("interpreting launch browser notification");
	memset(&url, 0 , SAT_URL_LEN_MAX);
	memset(&text, 0 , SAT_TEXT_STRING_LEN_MAX);
	memset(&gateway_proxy, 0 , SAT_TEXT_STRING_LEN_MAX);

	plg = tcore_server_find_plugin(ctx->server, plugin_name);
	if (!plg){
		dbg("there is no valid plugin at this point");
		return NULL;
	}

	if( (launch_browser_tlv->user_confirm_icon_id.is_exist) && ( launch_browser_tlv->user_confirm_icon_id.icon_qualifer == ICON_QUALI_NOT_SELF_EXPLANATORY)
			&& (!launch_browser_tlv->user_confirm_alpha_id.is_exist || !launch_browser_tlv->user_confirm_alpha_id.alpha_data_len))
	{
		struct treq_sat_terminal_rsp_data *tr = NULL;
		dbg("[SAT]  exceptional case to fix gcf case 2.4 command not understood");

		tr = (struct treq_sat_terminal_rsp_data *)calloc(1, sizeof(struct treq_sat_terminal_rsp_data));
		tr->cmd_number = launch_browser_tlv->command_detail.cmd_num;
		tr->cmd_type = launch_browser_tlv->command_detail.cmd_type;

		memcpy((void*)&tr->terminal_rsp_data.launch_browser.command_detail, &launch_browser_tlv->command_detail, sizeof(struct tel_sat_cmd_detail_info));
		tr->terminal_rsp_data.launch_browser.device_id.src = launch_browser_tlv->device_id.dest;
		tr->terminal_rsp_data.launch_browser.device_id.dest = launch_browser_tlv->device_id.src;
		tr->terminal_rsp_data.launch_browser.result_type = RESULT_COMMAND_DATA_NOT_UNDERSTOOD_BY_ME;

		sat_manager_send_terminal_response(ctx->comm, plg, tr);
		g_free(tr);

		return NULL;
	}

	//browser launch type
	browser_launch_type = launch_browser_tlv->command_detail.cmd_qualifier.launch_browser.launch_browser;

	//browser id
	browser_id = launch_browser_tlv->browser_id;

	//url and url len
	if( launch_browser_tlv->url.url_length){
		url_len = launch_browser_tlv->url.url_length;
		memcpy(url, launch_browser_tlv->url.url, launch_browser_tlv->url.url_length);
		dbg("launch browser url (%s)",url);
	}

	//gateway_proxy_text
	if(launch_browser_tlv->gateway_proxy_text.is_digit_only) {
		memcpy(gateway_proxy, launch_browser_tlv->gateway_proxy_text.string, launch_browser_tlv->gateway_proxy_text.string_length);
		dbg("launch browser gateway_proxy digit type string (%s)",gateway_proxy);
	} else {
		if( launch_browser_tlv->gateway_proxy_text.string_length){
			tcore_util_convert_string_to_utf8((unsigned char*)&gateway_proxy,(unsigned short *)&gateway_proxy_len,
					launch_browser_tlv->gateway_proxy_text.dcs.a_format,
					(unsigned char*)&launch_browser_tlv->gateway_proxy_text.string,
					(unsigned short)launch_browser_tlv->gateway_proxy_text.string_length);
			dbg("launch browser gateway_proxy_text (%s)",gateway_proxy);
		}
	}

	//user confirm text and user confirm text len
	if( launch_browser_tlv->user_confirm_alpha_id.is_exist && launch_browser_tlv->user_confirm_alpha_id.alpha_data_len){
		tcore_util_convert_string_to_utf8((unsigned char*)&text,(unsigned short *)&text_len,
				launch_browser_tlv->user_confirm_alpha_id.dcs.a_format,
				(unsigned char*)&launch_browser_tlv->user_confirm_alpha_id.alpha_data,
				(unsigned short)launch_browser_tlv->user_confirm_alpha_id.alpha_data_len);
		dbg("launch browser user confirm text (%s)",text);
	}

	//enqueue data and generate cmd_id
	memset(&q_data, 0x00, sizeof(struct sat_manager_queue_data));
	q_data.cmd_type = SAT_PROATV_CMD_LAUNCH_BROWSER;
	memcpy((void*)&(q_data.cmd_data.launch_browser), launch_browser_tlv, sizeof(struct tel_sat_launch_browser_tlv));
	sat_manager_enqueue_cmd(ctx, &q_data);
	command_id = q_data.cmd_id;

	icon_id = g_variant_new("()");

	launch_browser = g_variant_new("(iiisisisiv)",
			command_id, browser_launch_type, browser_id, url, url_len, gateway_proxy, gateway_proxy_len, text, text_len, icon_id);

	return launch_browser;
}

GVariant* sat_manager_provide_local_info_noti(struct custom_data *ctx, const char *plugin_name, struct tel_sat_provide_local_info_tlv *provide_local_info_tlv)
{
	TcorePlugin *plg = NULL;
	GVariant *provide_info = NULL;

	gint info_type =0;
	struct treq_sat_terminal_rsp_data *tr = NULL;

	dbg("interpreting provide local info notification");

	plg = tcore_server_find_plugin(ctx->server, plugin_name);
	if (!plg){
		dbg("there is no valid plugin at this point");
		return NULL;
	}

	provide_info = g_variant_new("(i)", info_type);

	//send TR - does not need from application's response
	tr = (struct treq_sat_terminal_rsp_data *)calloc(1, sizeof(struct treq_sat_terminal_rsp_data));
	tr->cmd_number = provide_local_info_tlv->command_detail.cmd_num;
	tr->cmd_type = provide_local_info_tlv->command_detail.cmd_type;
	memcpy((void*)&tr->terminal_rsp_data.provide_local_info.command_detail, &provide_local_info_tlv->command_detail, sizeof(struct tel_sat_cmd_detail_info));
	tr->terminal_rsp_data.provide_local_info.device_id.src = provide_local_info_tlv->device_id.dest;
	tr->terminal_rsp_data.provide_local_info.device_id.dest = provide_local_info_tlv->device_id.src;
	tr->terminal_rsp_data.provide_local_info.other_info = TRUE;

	info_type = provide_local_info_tlv->command_detail.cmd_qualifier.provide_local_info.provide_local_info;

	switch(info_type){
		case LOCAL_INFO_DATE_TIME_AND_TIMEZONE:{
			int err = 0; int gmt = 0, n_flg = 0;
			struct timezone c_tz;
			struct timeval c_time;

			time_t time_val;
			struct tm time_info;

			time(&time_val);

			tzset();
			err = gettimeofday(&c_time, &c_tz);
			localtime_r(&time_val, &time_info);

			//set the time information
			tr->terminal_rsp_data.provide_local_info.other.date_time_and_timezone.year =
				_convert_decimal_to_bcd(time_info.tm_year+1900-2000);

			tr->terminal_rsp_data.provide_local_info.other.date_time_and_timezone.month =
					_convert_decimal_to_bcd(time_info.tm_mon+1);

			tr->terminal_rsp_data.provide_local_info.other.date_time_and_timezone.day =
					_convert_decimal_to_bcd(time_info.tm_mday);

			tr->terminal_rsp_data.provide_local_info.other.date_time_and_timezone.hour =
					_convert_decimal_to_bcd(time_info.tm_hour);

			tr->terminal_rsp_data.provide_local_info.other.date_time_and_timezone.minute =
					_convert_decimal_to_bcd(time_info.tm_min);

			tr->terminal_rsp_data.provide_local_info.other.date_time_and_timezone.second =
					_convert_decimal_to_bcd(time_info.tm_sec);

			gmt = c_tz.tz_minuteswest/60;
			if(gmt < 0){
				gmt = gmt * -1;
				n_flg = 1;
			}

			if(err != 0){
				tr->terminal_rsp_data.provide_local_info.other.date_time_and_timezone.timeZone = 0xFF;
			}
			else{
				tr->terminal_rsp_data.provide_local_info.other.date_time_and_timezone.timeZone =
					_convert_decimal_to_bcd(gmt);

				if(n_flg == 1){
					tr->terminal_rsp_data.provide_local_info.other.date_time_and_timezone.timeZone += 0x80;
				}

				if(time_info.tm_isdst > 0){
					tr->terminal_rsp_data.provide_local_info.other.date_time_and_timezone.timeZone += 0x40;
				}
			}

			tr->terminal_rsp_data.provide_local_info.result_type = RESULT_SUCCESS;
			tr->terminal_rsp_data.provide_local_info.me_problem_type = ME_PROBLEM_NO_SPECIFIC_CAUSE;
		} break;
		case LOCAL_INFO_LANGUAGE:{
			Server *s = NULL;
			static Storage *strg;
			gchar *lang_str = NULL;
			enum tel_sim_language_type lang_type = SIM_LANG_UNSPECIFIED;

			tr->terminal_rsp_data.provide_local_info.result_type = RESULT_SUCCESS;
			tr->terminal_rsp_data.provide_local_info.me_problem_type = ME_PROBLEM_NO_SPECIFIC_CAUSE;

			s = ctx->server;
			strg = tcore_server_find_storage(s, "vconf");
			lang_str = tcore_storage_get_string(strg, STORAGE_KEY_LANGUAGE_SET);
			if(lang_str)
				lang_type = _convert_string_to_sim_lang(lang_str);

			tr->terminal_rsp_data.provide_local_info.other.language = lang_type;
		} break;
		default :{
			tr->terminal_rsp_data.provide_local_info.other_info = FALSE;
			tr->terminal_rsp_data.provide_local_info.result_type = RESULT_ME_UNABLE_TO_PROCESS_COMMAND;
			tr->terminal_rsp_data.provide_local_info.me_problem_type = ME_PROBLEM_NO_SPECIFIC_CAUSE;
		} break;
	}

	sat_manager_send_terminal_response(ctx->comm, plg, tr);
	g_free(tr);

	return provide_info;
}

GVariant* sat_manager_language_notification_noti(struct custom_data *ctx, const char *plugin_name, struct tel_sat_language_notification_tlv *language_notification_tlv)
{
	TcorePlugin *plg = NULL;
	GVariant *language_noti = NULL;
	struct sat_manager_queue_data q_data;

	gint command_id = 0;
	gint language =0;
	gboolean b_specified = FALSE;

	dbg("interpreting langauge notification");

	plg = tcore_server_find_plugin(ctx->server, plugin_name);
	if (!plg){
		dbg("there is no valid plugin at this point");
		return NULL;
	}

	if (language_notification_tlv->command_detail.cmd_qualifier.language_notification.specific_language == TRUE){
		b_specified = TRUE;
		language = language_notification_tlv->language;
	} else {
		b_specified = FALSE;
		language =SIM_LANG_UNSPECIFIED;
	}

	//enqueue data and generate cmd_id
	memset(&q_data, 0x00, sizeof(struct sat_manager_queue_data));
	q_data.cmd_type = SAT_PROATV_CMD_LANGUAGE_NOTIFICATION;
	memcpy((void*)&(q_data.cmd_data.language_notification), language_notification_tlv, sizeof(struct tel_sat_language_notification_tlv));
	sat_manager_enqueue_cmd(ctx, &q_data);
	command_id = q_data.cmd_id;

	language_noti = g_variant_new("(iib)", command_id, language, b_specified);

	return language_noti;
}

static gboolean _sat_manager_handle_setup_menu_result(struct custom_data *ctx, TcorePlugin *plg, gint command_id, GVariant *exec_result)
{
	TReturn rv = TCORE_RETURN_FAILURE;
	gboolean result = FALSE;

	gint resp;
	struct treq_sat_terminal_rsp_data *tr;
	struct sat_manager_queue_data q_data;

	memset(&q_data, 0, sizeof(struct sat_manager_queue_data));

	if (sat_manager_dequeue_cmd_by_id(ctx, &q_data, command_id) == FALSE) {
		dbg("[SAT] command dequeue failed. didn't find in command Q!!");
		return result;
	}

	if(!exec_result){
		dbg("[SAT] setup menu result data is null");
		return result;
	}

	if (!plg){
		dbg("there is no valid plugin at this point");
		return result;
	}

	dbg("exec_result type_format(%s)", g_variant_get_type_string(exec_result));
	g_variant_get(exec_result, "(i)", &resp);

	tr = (struct treq_sat_terminal_rsp_data *)calloc(1, sizeof(struct treq_sat_terminal_rsp_data));
	tr->cmd_number = q_data.cmd_data.setupMenuInd.command_detail.cmd_num;
	tr->cmd_type = q_data.cmd_data.setupMenuInd.command_detail.cmd_type;

	memcpy((void*)&tr->terminal_rsp_data.setup_menu.command_detail,
		&q_data.cmd_data.setupMenuInd.command_detail, sizeof(struct tel_sat_cmd_detail_info));

	tr->terminal_rsp_data.setup_menu.device_id.src = q_data.cmd_data.setupMenuInd.device_id.dest;
	tr->terminal_rsp_data.setup_menu.device_id.dest = q_data.cmd_data.setupMenuInd.device_id.src;

	dbg("[SAT] Response: [0x%02x]", resp);

	switch (resp) {
		case RESULT_SUCCESS:
			tr->terminal_rsp_data.setup_menu.result_type = RESULT_SUCCESS;
			if (q_data.cmd_data.setupMenuInd.icon_id.icon_info.ics == IMAGE_CODING_SCHEME_COLOUR)
				tr->terminal_rsp_data.setup_menu.result_type = RESULT_SUCCESS_BUT_REQUESTED_ICON_NOT_DISPLAYED;
			tr->terminal_rsp_data.setup_menu.me_problem_type = ME_PROBLEM_NO_SPECIFIC_CAUSE;
			break;

		case RESULT_ME_UNABLE_TO_PROCESS_COMMAND:
			tr->terminal_rsp_data.setup_menu.result_type = RESULT_ME_UNABLE_TO_PROCESS_COMMAND;
			tr->terminal_rsp_data.setup_menu.me_problem_type = ME_PROBLEM_NO_SPECIFIC_CAUSE;
			break;

		default:
			//check the default case
			tr->terminal_rsp_data.setup_menu.result_type = resp;
			tr->terminal_rsp_data.setup_menu.me_problem_type = ME_PROBLEM_NO_SPECIFIC_CAUSE;
			dbg("[SAT] wrong result from app exec resp(%d)", resp);
			break;
	}

	result = TRUE;
	rv = sat_manager_send_terminal_response(ctx->comm, plg, tr);
	if(rv != TCORE_RETURN_SUCCESS){
		dbg("fail to send terminal response");
		result = FALSE;
	}

	g_free(tr);

	return result;
}

static gboolean _sat_manager_handle_display_text_result(struct custom_data *ctx, TcorePlugin *plg, gint command_id, GVariant *exec_result)
{
	TReturn rv = TCORE_RETURN_FAILURE;
	gboolean result = FALSE;

	gint resp, me_problem;
	struct treq_sat_terminal_rsp_data *tr;
	struct sat_manager_queue_data q_data;

	memset(&q_data, 0, sizeof(struct sat_manager_queue_data));

	if (sat_manager_dequeue_cmd_by_id(ctx, &q_data, command_id) == FALSE) {
		dbg("[SAT] command dequeue failed. didn't find in command Q!!");
		return result;
	}

	if(!exec_result){
		dbg("[SAT] display text result data is null");
		return result;
	}

	if (!plg){
		dbg("there is no valid plugin at this point");
		return result;
	}

	dbg("exec_result type_format(%s)", g_variant_get_type_string(exec_result));
	g_variant_get(exec_result, "(ii)",&resp, &me_problem);

	tr = (struct treq_sat_terminal_rsp_data *)calloc(1, sizeof(struct treq_sat_terminal_rsp_data));
	tr->cmd_number = q_data.cmd_data.displayTextInd.command_detail.cmd_num;
	tr->cmd_type = q_data.cmd_data.displayTextInd.command_detail.cmd_type;
	memcpy((void*)&tr->terminal_rsp_data.display_text.command_detail, &q_data.cmd_data.displayTextInd.command_detail, sizeof(struct tel_sat_cmd_detail_info));
	tr->terminal_rsp_data.display_text.device_id.src = DEVICE_ID_ME;
	tr->terminal_rsp_data.display_text.device_id.dest = DEVICE_ID_SIM;

	switch (resp) {
		case RESULT_SUCCESS:
			tr->terminal_rsp_data.display_text.result_type = RESULT_SUCCESS;
			tr->terminal_rsp_data.display_text.me_problem_type = ME_PROBLEM_NO_SPECIFIC_CAUSE;
			break;

		case RESULT_ME_UNABLE_TO_PROCESS_COMMAND:
			tr->terminal_rsp_data.display_text.result_type = RESULT_ME_UNABLE_TO_PROCESS_COMMAND;
			tr->terminal_rsp_data.display_text.me_problem_type = me_problem;
			break;

		default:
			tr->terminal_rsp_data.display_text.result_type = resp;
			tr->terminal_rsp_data.display_text.me_problem_type = me_problem;
			dbg("[SAT] wrong result from app exec resp(%d) me_problem(%d)", resp, me_problem);
			break;
	}

	if (q_data.cmd_data.displayTextInd.icon_id.icon_info.ics == IMAGE_CODING_SCHEME_COLOUR)
		tr->terminal_rsp_data.display_text.result_type = RESULT_SUCCESS_BUT_REQUESTED_ICON_NOT_DISPLAYED;

	result = TRUE;
	rv = sat_manager_send_terminal_response(ctx->comm, plg, tr);
	if(rv != TCORE_RETURN_SUCCESS){
		dbg("fail to send terminal response");
		result = FALSE;
	}

	g_free(tr);

	return result;
}

static gboolean _sat_manager_handle_play_tone_result(struct custom_data *ctx, TcorePlugin *plg, gint command_id, GVariant *exec_result)
{
	TReturn rv = TCORE_RETURN_FAILURE;
	gboolean result = FALSE;

	gint resp, me_problem;
	struct treq_sat_terminal_rsp_data *tr;
	struct sat_manager_queue_data q_data;

	memset(&q_data, 0, sizeof(struct sat_manager_queue_data));

	if (sat_manager_dequeue_cmd_by_id(ctx, &q_data, command_id) == FALSE) {
		dbg("[SAT] command dequeue failed. didn't find in command Q!!");
		return result;
	}

	if(!exec_result){
		dbg("[SAT] display text result data is null");
		return result;
	}

	if (!plg){
		dbg("there is no valid plugin at this point");
		return result;
	}

	dbg("exec_result type_format(%s)", g_variant_get_type_string(exec_result));
	g_variant_get(exec_result, "(ii)",&resp, &me_problem);

	tr = (struct treq_sat_terminal_rsp_data *)calloc(1, sizeof(struct treq_sat_terminal_rsp_data));
	tr->cmd_number = q_data.cmd_data.play_tone.command_detail.cmd_num;
	tr->cmd_type = q_data.cmd_data.play_tone.command_detail.cmd_type;
	memcpy((void*)&tr->terminal_rsp_data.play_tone.command_detail, &q_data.cmd_data.play_tone.command_detail, sizeof(struct tel_sat_cmd_detail_info));
	tr->terminal_rsp_data.play_tone.device_id.src = DEVICE_ID_ME;
	tr->terminal_rsp_data.play_tone.device_id.dest = DEVICE_ID_SIM;

	switch (resp) {
		case RESULT_SUCCESS:
			tr->terminal_rsp_data.play_tone.result_type = RESULT_SUCCESS;
			if (q_data.cmd_data.play_tone.icon_id.icon_info.ics == IMAGE_CODING_SCHEME_COLOUR)
				tr->terminal_rsp_data.play_tone.result_type = RESULT_SUCCESS_BUT_REQUESTED_ICON_NOT_DISPLAYED;

			tr->terminal_rsp_data.play_tone.me_problem_type = ME_PROBLEM_NO_SPECIFIC_CAUSE;
			break;

		case RESULT_ME_UNABLE_TO_PROCESS_COMMAND:
			tr->terminal_rsp_data.play_tone.result_type = RESULT_ME_UNABLE_TO_PROCESS_COMMAND;
			tr->terminal_rsp_data.play_tone.me_problem_type = ME_PROBLEM_NO_SPECIFIC_CAUSE;
			break;

		case RESULT_PROACTIVE_SESSION_TERMINATED_BY_USER:
			tr->terminal_rsp_data.play_tone.result_type = RESULT_PROACTIVE_SESSION_TERMINATED_BY_USER;
			tr->terminal_rsp_data.play_tone.me_problem_type = ME_PROBLEM_NO_SPECIFIC_CAUSE;
			break;

		default:
			tr->terminal_rsp_data.play_tone.result_type = resp;
			tr->terminal_rsp_data.play_tone.me_problem_type = me_problem;
			dbg("[SAT] wrong result from app exec resp(%d) me_problem(%d)", resp, me_problem);
			break;
	}

	if (q_data.cmd_data.displayTextInd.icon_id.icon_info.ics == IMAGE_CODING_SCHEME_COLOUR)
		tr->terminal_rsp_data.display_text.result_type = RESULT_SUCCESS_BUT_REQUESTED_ICON_NOT_DISPLAYED;

	result = TRUE;
	rv = sat_manager_send_terminal_response(ctx->comm, plg, tr);
	if(rv != TCORE_RETURN_SUCCESS){
		dbg("fail to send terminal response");
		result = FALSE;
	}

	g_free(tr);
	return result;
}

static gboolean _sat_manager_handle_send_sms_result(struct custom_data *ctx, TcorePlugin *plg, gint command_id, GVariant *exec_result)
{
	TReturn rv = TCORE_RETURN_FAILURE;
	gboolean result = FALSE;

	gint resp;
	struct treq_sat_terminal_rsp_data *tr;
	struct sat_manager_queue_data q_data;

	memset(&q_data, 0, sizeof(struct sat_manager_queue_data));

	if (sat_manager_dequeue_cmd_by_id(ctx, &q_data, command_id) == FALSE) {
		dbg("[SAT] command dequeue failed. didn't find in command Q!!");
		return result;
	}

	if(!exec_result){
		dbg("[SAT] send sms data is null");
		return result;
	}

	if (!plg){
		dbg("there is no valid plugin at this point");
		return result;
	}

	dbg("exec_result type_format(%s)", g_variant_get_type_string(exec_result));
	g_variant_get(exec_result, "(i)",&resp);

	tr = (struct treq_sat_terminal_rsp_data *)calloc(1, sizeof(struct treq_sat_terminal_rsp_data));
	tr->cmd_number = q_data.cmd_data.sendSMSInd.command_detail.cmd_num;
	tr->cmd_type = q_data.cmd_data.sendSMSInd.command_detail.cmd_type;
	memcpy((void*)&tr->terminal_rsp_data.send_sms.command_detail, &q_data.cmd_data.sendSMSInd.command_detail, sizeof(struct tel_sat_cmd_detail_info));
	tr->terminal_rsp_data.send_sms.device_id.src = DEVICE_ID_ME;
	tr->terminal_rsp_data.send_sms.device_id.dest = q_data.cmd_data.sendSMSInd.device_id.src;

	switch (resp) {
		case RESULT_SUCCESS:
			tr->terminal_rsp_data.send_sms.result_type = RESULT_SUCCESS;
			if (q_data.cmd_data.sendSMSInd.icon_id.icon_info.ics == IMAGE_CODING_SCHEME_COLOUR)
				tr->terminal_rsp_data.send_sms.result_type = RESULT_SUCCESS_BUT_REQUESTED_ICON_NOT_DISPLAYED;

			break;

		case RESULT_INTRCTN_WITH_CC_OR_SMS_CTRL_PRMNT_PRBLM:
			tr->terminal_rsp_data.send_sms.result_type = RESULT_INTRCTN_WITH_CC_OR_SMS_CTRL_PRMNT_PRBLM;
			tr->terminal_rsp_data.send_sms.cc_problem_type = CC_PROBLEM_ACTION_NOT_ALLOWED;
			break;

		case RESULT_ME_UNABLE_TO_PROCESS_COMMAND:
			tr->terminal_rsp_data.send_sms.result_type = RESULT_ME_UNABLE_TO_PROCESS_COMMAND;
			tr->terminal_rsp_data.send_sms.me_problem_type = ME_PROBLEM_NO_SPECIFIC_CAUSE;
			break;

		case RESULT_BEYOND_ME_CAPABILITIES:
			tr->terminal_rsp_data.send_sms.result_type = RESULT_BEYOND_ME_CAPABILITIES;
			tr->terminal_rsp_data.send_sms.me_problem_type = ME_PROBLEM_NO_SPECIFIC_CAUSE;
			break;

		case RESULT_COMMAND_TYPE_NOT_UNDERSTOOD_BY_ME:
			tr->terminal_rsp_data.send_sms.result_type = RESULT_COMMAND_TYPE_NOT_UNDERSTOOD_BY_ME;
			tr->terminal_rsp_data.send_sms.me_problem_type = ME_PROBLEM_NO_SPECIFIC_CAUSE;
			break;

		case RESULT_COMMAND_DATA_NOT_UNDERSTOOD_BY_ME:
			tr->terminal_rsp_data.send_sms.result_type = RESULT_COMMAND_DATA_NOT_UNDERSTOOD_BY_ME;
			tr->terminal_rsp_data.send_sms.me_problem_type = ME_PROBLEM_NO_SPECIFIC_CAUSE;
			break;

		case RESULT_SMS_RP_ERROR:
			tr->terminal_rsp_data.send_sms.result_type = RESULT_SMS_RP_ERROR;
			tr->terminal_rsp_data.send_sms.me_problem_type = ME_PROBLEM_NO_SPECIFIC_CAUSE;
			break;

		default:
			tr->terminal_rsp_data.send_sms.result_type = RESULT_ME_UNABLE_TO_PROCESS_COMMAND;
			tr->terminal_rsp_data.send_sms.me_problem_type = ME_PROBLEM_NO_SPECIFIC_CAUSE;
			break;
	}

	result = TRUE;
	rv = sat_manager_send_terminal_response(ctx->comm, plg, tr);
	if(rv != TCORE_RETURN_SUCCESS){
		dbg("fail to send terminal response");
		result = FALSE;
	}

	g_free(tr);

/*	if( q_data.cmd_data.sendSMSInd.alpha_id.alpha_data_len && q_data.cmd_data.sendSMSInd.alpha_id.is_exist)
		sat_ui_support_terminate_sat_ui();*/

	return result;
}

static gboolean _sat_manager_handle_send_ss_result(struct custom_data *ctx, TcorePlugin *plg, gint command_id, GVariant *exec_result)
{
	TReturn rv = TCORE_RETURN_FAILURE;
	gboolean result = FALSE;

	gint resp, me_problem, ss_cause, call_ctrl_problem, ss_str_len;
	gchar *ss_string;
	struct treq_sat_terminal_rsp_data *tr;
	struct sat_manager_queue_data q_data;
	//call ctrl action, result data object, text

	memset(&q_data, 0, sizeof(struct sat_manager_queue_data));

	if (sat_manager_dequeue_cmd_by_id(ctx, &q_data, command_id) == FALSE) {
		dbg("[SAT] command dequeue failed. didn't find in command Q!!");
		return result;
	}

	if(!exec_result){
		dbg("[SAT] send ss data is null");
		return result;
	}

	if (!plg){
		dbg("there is no valid plugin at this point");
		return result;
	}

	dbg("exec_result type_format(%s)", g_variant_get_type_string(exec_result));
	g_variant_get(exec_result, "(iiisii)", &resp, &me_problem, &ss_cause, &ss_string, &ss_str_len, &call_ctrl_problem);

	tr = (struct treq_sat_terminal_rsp_data *)calloc(1, sizeof(struct treq_sat_terminal_rsp_data));
	tr->cmd_number = q_data.cmd_data.send_ss.command_detail.cmd_num;
	tr->cmd_type = q_data.cmd_data.send_ss.command_detail.cmd_type;
	memcpy((void*)&tr->terminal_rsp_data.send_ss.command_detail, &q_data.cmd_data.send_ss.command_detail, sizeof(struct tel_sat_cmd_detail_info));
	tr->terminal_rsp_data.send_ss.device_id.src = DEVICE_ID_ME;
	tr->terminal_rsp_data.send_ss.device_id.dest = q_data.cmd_data.send_ss.device_id.src;

	switch (resp) {
		case RESULT_SUCCESS:
			tr->terminal_rsp_data.send_ss.result_type = RESULT_SUCCESS;
			if (q_data.cmd_data.send_ss.icon_id.icon_info.ics == IMAGE_CODING_SCHEME_COLOUR)
				tr->terminal_rsp_data.send_ss.result_type = RESULT_SUCCESS_BUT_REQUESTED_ICON_NOT_DISPLAYED;

			if(ss_str_len > 0 && ss_string != NULL){
				memcpy(&tr->terminal_rsp_data.send_ss.text.dcs, &q_data.cmd_data.send_ss.alpha_id.dcs, sizeof(struct data_coding_scheme));
				memcpy((void*)tr->terminal_rsp_data.send_ss.text.string, ss_string, ss_str_len);
				tr->terminal_rsp_data.send_ss.text.string_length = ss_str_len;
			}
			break;

		case RESULT_SS_RETURN_ERROR:
			tr->terminal_rsp_data.send_ss.result_type = RESULT_SS_RETURN_ERROR;
			tr->terminal_rsp_data.send_ss.ss_problem = ss_cause;
			break;

		case RESULT_NETWORK_UNABLE_TO_PROCESS_COMMAND:
			tr->terminal_rsp_data.send_ss.result_type = RESULT_NETWORK_UNABLE_TO_PROCESS_COMMAND;
			tr->terminal_rsp_data.send_ss.me_problem_type = ME_PROBLEM_NO_SPECIFIC_CAUSE;
			break;

		default:
			tr->terminal_rsp_data.send_ss.result_type = RESULT_SS_RETURN_ERROR;
			tr->terminal_rsp_data.send_ss.ss_problem = SATK_SS_PROBLEM_NO_SPECIFIC_CAUSE;
			break;
	}

	result = TRUE;
	rv = sat_manager_send_terminal_response(ctx->comm, plg, tr);
	if(rv != TCORE_RETURN_SUCCESS){
		dbg("fail to send terminal response");
		result = FALSE;
	}
	g_free(tr);

	if( q_data.cmd_data.send_ss.alpha_id.alpha_data_len && q_data.cmd_data.send_ss.alpha_id.is_exist )
		sat_ui_support_terminate_sat_ui();

	return result;
}

static gboolean _sat_manager_handle_send_ussd_result(struct custom_data *ctx, TcorePlugin *plg, gint command_id, GVariant *exec_result)
{
	TReturn rv = TCORE_RETURN_FAILURE;
	gboolean result = FALSE;

	gint resp, me_problem, ss_cause, ussd_str_len;
	GVariant *ussd_str = NULL;
	struct treq_sat_terminal_rsp_data *tr;
	struct sat_manager_queue_data q_data;
	//call ctrl action, result data object, text, result2, text2

	memset(&q_data, 0, sizeof(struct sat_manager_queue_data));

	if (sat_manager_dequeue_cmd_by_id(ctx, &q_data, command_id) == FALSE) {
		dbg("[SAT] command dequeue failed. didn't find in command Q!!");
		return result;
	}

	if(!exec_result){
		dbg("[SAT] send ss data is null");
		return result;
	}

	if (!plg){
		dbg("there is no valid plugin at this point");
		return result;
	}

	dbg("exec_result type_format(%s)", g_variant_get_type_string(exec_result));
	g_variant_get(exec_result, "(iii@vi)", &resp, &me_problem, &ss_cause, &ussd_str, &ussd_str_len);

	tr = (struct treq_sat_terminal_rsp_data *)calloc(1, sizeof(struct treq_sat_terminal_rsp_data));
	tr->cmd_number = q_data.cmd_data.send_ussd.command_detail.cmd_num;
	tr->cmd_type = q_data.cmd_data.send_ussd.command_detail.cmd_type;
	memcpy((void*)&tr->terminal_rsp_data.send_ussd.command_detail, &q_data.cmd_data.send_ussd.command_detail, sizeof(struct tel_sat_cmd_detail_info));
	tr->terminal_rsp_data.send_ussd.device_id.src = DEVICE_ID_ME;
	tr->terminal_rsp_data.send_ussd.device_id.dest = q_data.cmd_data.send_ussd.device_id.src;

	switch (resp) {
		case RESULT_SUCCESS:
			tr->terminal_rsp_data.send_ussd.result_type = RESULT_SUCCESS;
			if (q_data.cmd_data.send_ussd.icon_id.icon_info.ics == IMAGE_CODING_SCHEME_COLOUR)
				tr->terminal_rsp_data.send_ussd.result_type = RESULT_SUCCESS_BUT_REQUESTED_ICON_NOT_DISPLAYED;

			if(ussd_str_len > 0 && ussd_str){
				int index = 0;
				guchar data;
				GVariantIter *iter = NULL;
				GVariant *intermediate = NULL;

				intermediate = g_variant_get_variant(ussd_str);
				dbg("ussd string format(%s)", g_variant_get_type_string(intermediate));

				g_variant_get(intermediate, "ay", &iter);
				while( g_variant_iter_loop (iter, "y", &data)){
					dbg("index(%d) data(%c)", index, data);

					if(index == 0){
						tr->terminal_rsp_data.send_ussd.text.dcs.raw_dcs = (data & 0x0F);
					}
					else{
						tr->terminal_rsp_data.send_ussd.text.string[index-1] = data;
					}

					index++;
				}
				g_variant_iter_free(iter);
				g_variant_unref(intermediate);

				tr->terminal_rsp_data.send_ussd.text.string_length = index-1;
			}
			break;

		case RESULT_SS_RETURN_ERROR:
		case RESULT_USSD_RETURN_ERROR:
			tr->terminal_rsp_data.send_ussd.result_type = RESULT_USSD_RETURN_ERROR;
			tr->terminal_rsp_data.send_ussd.ussd_problem = ss_cause;
			break;

		case RESULT_NETWORK_UNABLE_TO_PROCESS_COMMAND:
			tr->terminal_rsp_data.send_ussd.result_type = RESULT_NETWORK_UNABLE_TO_PROCESS_COMMAND;
			tr->terminal_rsp_data.send_ussd.me_problem_type = ME_PROBLEM_NO_SPECIFIC_CAUSE;
			break;

		case RESULT_ME_UNABLE_TO_PROCESS_COMMAND:
			tr->terminal_rsp_data.send_ussd.result_type = RESULT_ME_UNABLE_TO_PROCESS_COMMAND;
			tr->terminal_rsp_data.send_ussd.me_problem_type = ME_PROBLEM_NO_SPECIFIC_CAUSE;
			break;

		default:
			tr->terminal_rsp_data.send_ussd.result_type = RESULT_USSD_RETURN_ERROR;
			tr->terminal_rsp_data.send_ussd.ussd_problem = SATK_USSD_PROBLEM_NO_SPECIFIC_CAUSE;
			break;
	}

	result = TRUE;
	rv = sat_manager_send_terminal_response(ctx->comm, plg, tr);
	if(rv != TCORE_RETURN_SUCCESS){
		dbg("fail to send terminal response");
		result = FALSE;
	}
	g_free(tr);

	if( q_data.cmd_data.send_ussd.alpha_id.alpha_data_len && q_data.cmd_data.send_ussd.alpha_id.is_exist )
		sat_ui_support_terminate_sat_ui();

	return result;
}

static gboolean _sat_manager_handle_setup_call_result(struct custom_data *ctx, TcorePlugin *plg, gint command_id, GVariant *exec_result)
{
	TReturn rv = TCORE_RETURN_FAILURE;
	gboolean result = FALSE;

	gint resp, me_problem, cc_problem, call_cause;
	struct treq_sat_terminal_rsp_data *tr;
	struct sat_manager_queue_data q_data;

	memset(&q_data, 0, sizeof(struct sat_manager_queue_data));

	if (sat_manager_dequeue_cmd_by_id(ctx, &q_data, command_id) == FALSE) {
		dbg("[SAT] command dequeue failed. didn't find in command Q!!");
		return result;
	}

	if(!exec_result){
		dbg("[SAT] setup call data is null");
		return result;
	}

	if (!plg){
		dbg("there is no valid plugin at this point");
		return result;
	}

	dbg("exec_result type_format(%s)", g_variant_get_type_string(exec_result));
	g_variant_get(exec_result, "(iiii)",&resp, &me_problem, &cc_problem, &call_cause);

	tr = (struct treq_sat_terminal_rsp_data *)calloc(1, sizeof(struct treq_sat_terminal_rsp_data));
	tr->cmd_number = q_data.cmd_data.setup_call.command_detail.cmd_num;
	tr->cmd_type = q_data.cmd_data.setup_call.command_detail.cmd_type;
	memcpy((void*)&tr->terminal_rsp_data.setup_call.command_detail, &q_data.cmd_data.setup_call.command_detail, sizeof(struct tel_sat_cmd_detail_info));
	tr->terminal_rsp_data.setup_call.device_id.src = DEVICE_ID_ME;
	tr->terminal_rsp_data.setup_call.device_id.dest = q_data.cmd_data.setup_call.device_id.src;

	switch (resp) {
		case RESULT_SUCCESS:
			tr->terminal_rsp_data.setup_call.result_type = RESULT_SUCCESS;
			if (q_data.cmd_data.setup_call.call_setup_icon_id.icon_info.ics == IMAGE_CODING_SCHEME_COLOUR)
				tr->terminal_rsp_data.setup_call.result_type = RESULT_SUCCESS_BUT_REQUESTED_ICON_NOT_DISPLAYED;
			tr->terminal_rsp_data.setup_call.me_problem_type = ME_PROBLEM_NO_SPECIFIC_CAUSE;
			tr->terminal_rsp_data.setup_call.cc_problem_type = CC_PROBLEM_NO_SPECIFIC_CAUSE;
			break;

		case RESULT_ME_UNABLE_TO_PROCESS_COMMAND:
			tr->terminal_rsp_data.setup_call.result_type = RESULT_ME_UNABLE_TO_PROCESS_COMMAND;
			tr->terminal_rsp_data.setup_call.me_problem_type = me_problem;
			break;

		case RESULT_NETWORK_UNABLE_TO_PROCESS_COMMAND:{
			tr->terminal_rsp_data.setup_call.result_type = RESULT_NETWORK_UNABLE_TO_PROCESS_COMMAND;
			switch(call_cause){
				case CALL_ERROR_BUSY:
					tr->terminal_rsp_data.setup_call.network_problem_type = NETWORK_PROBLEM_USER_BUSY;
				break;
				default :
					tr->terminal_rsp_data.setup_call.network_problem_type = NETWORK_PROBLEM_NO_SPECIFIC_CAUSE;
				break;
			}
		}break;

		case RESULT_USER_CLEAR_DOWN_CALL_BEFORE_CONN:
			tr->terminal_rsp_data.setup_call.result_type = RESULT_USER_CLEAR_DOWN_CALL_BEFORE_CONN;
			tr->terminal_rsp_data.setup_call.me_problem_type = ME_PROBLEM_NO_SPECIFIC_CAUSE;
			tr->terminal_rsp_data.setup_call.cc_problem_type = CC_PROBLEM_NO_SPECIFIC_CAUSE;
			break;

		case RESULT_BEYOND_ME_CAPABILITIES:
			tr->terminal_rsp_data.setup_call.result_type = RESULT_BEYOND_ME_CAPABILITIES;
			tr->terminal_rsp_data.setup_call.me_problem_type = ME_PROBLEM_NO_SPECIFIC_CAUSE;
			tr->terminal_rsp_data.setup_call.cc_problem_type = CC_PROBLEM_NO_SPECIFIC_CAUSE;
			break;

		case RESULT_INTRCTN_WITH_CC_OR_SMS_CTRL_PRMNT_PRBLM:
			tr->terminal_rsp_data.setup_call.result_type = RESULT_INTRCTN_WITH_CC_OR_SMS_CTRL_PRMNT_PRBLM;
			tr->terminal_rsp_data.setup_call.me_problem_type = ME_PROBLEM_NO_SPECIFIC_CAUSE;
			tr->terminal_rsp_data.setup_call.cc_problem_type = cc_problem;
			break;


		default:
			break;
	}

	//TODO Other infomation set - not supported
	tr->terminal_rsp_data.setup_call.other_info = FALSE;

	result = TRUE;
	rv = sat_manager_send_terminal_response(ctx->comm, plg, tr);
	if(rv != TCORE_RETURN_SUCCESS){
		dbg("fail to send terminal response");
		result = FALSE;
	}

	g_free(tr);
	return result;
}

static gboolean _sat_manager_handle_setup_idle_mode_text_result(struct custom_data *ctx, TcorePlugin *plg, gint command_id, GVariant *exec_result)
{
	TReturn rv = TCORE_RETURN_FAILURE;
	gboolean result = FALSE;

	gint resp, me_problem;
	struct treq_sat_terminal_rsp_data *tr;
	struct sat_manager_queue_data q_data;

	memset(&q_data, 0, sizeof(struct sat_manager_queue_data));

	if (sat_manager_dequeue_cmd_by_id(ctx, &q_data, command_id) == FALSE) {
		dbg("[SAT] command dequeue failed. didn't find in command Q!!");
		return result;
	}

	if(!exec_result){
		dbg("[SAT] send ss data is null");
		return result;
	}

	if (!plg){
		dbg("there is no valid plugin at this point");
		return result;
	}

	dbg("exec_result type_format(%s)", g_variant_get_type_string(exec_result));
	g_variant_get(exec_result, "(ii)",&resp, &me_problem);

	tr = g_new0(struct treq_sat_terminal_rsp_data, 1);
	tr->cmd_number = q_data.cmd_data.idle_mode.command_detail.cmd_num;
	tr->cmd_type = q_data.cmd_data.idle_mode.command_detail.cmd_type;
	memcpy((void*)&tr->terminal_rsp_data.setup_idle_mode_text.command_detail, &q_data.cmd_data.idle_mode.command_detail, sizeof(struct tel_sat_cmd_detail_info));
	tr->terminal_rsp_data.setup_idle_mode_text.device_id.src = q_data.cmd_data.idle_mode.device_id.dest;
	tr->terminal_rsp_data.setup_idle_mode_text.device_id.dest = q_data.cmd_data.idle_mode.device_id.src;

	switch (resp) {
		case RESULT_SUCCESS:
			tr->terminal_rsp_data.setup_idle_mode_text.result_type = RESULT_SUCCESS;
			if (q_data.cmd_data.idle_mode.icon_id.icon_info.ics == IMAGE_CODING_SCHEME_COLOUR)
				tr->terminal_rsp_data.setup_idle_mode_text.result_type = RESULT_SUCCESS_BUT_REQUESTED_ICON_NOT_DISPLAYED;

			break;
		default:
			tr->terminal_rsp_data.setup_idle_mode_text.result_type = resp;
			tr->terminal_rsp_data.setup_idle_mode_text.me_problem_type = me_problem;
			break;
	}

	result = TRUE;
	rv = sat_manager_send_terminal_response(ctx->comm, plg, tr);
	if(rv != TCORE_RETURN_SUCCESS){
		dbg("fail to send terminal response");
		result = FALSE;
	}
	g_free(tr);

	sat_ui_support_terminate_sat_ui();

	return result;
}

static gboolean sat_manager_handle_open_channel_result(struct custom_data *ctx, TcorePlugin *plg, gint command_id, GVariant *exec_result)
{
	TReturn rv = TCORE_RETURN_FAILURE;
	gboolean result = FALSE;

	gint resp, me_problem, bip_problem;
	gint bearer_type, channel_id, channel_status, channel_status_info, buffer_size;
	gboolean other_info;
	GVariant *desc_tmp, *bearer_desc;

	struct treq_sat_terminal_rsp_data *tr;
	struct sat_manager_queue_data q_data;

	memset(&q_data, 0, sizeof(struct sat_manager_queue_data));

	if (sat_manager_dequeue_cmd_by_id(ctx, &q_data, command_id) == FALSE) {
		dbg("[SAT] command dequeue failed. didn't find in command Q!!");
		return result;
	}

	if(!exec_result){
		dbg("[SAT] open channel data is null");
		return result;
	}

	if (!plg){
		dbg("there is no valid plugin at this point");
		return result;
	}

	dbg("exec_result type_format(%s)", g_variant_get_type_string(exec_result));
	g_variant_get(exec_result, "(iiiiiiiibv)",&resp, &me_problem, &bip_problem,
			&bearer_type, &channel_id, &channel_status, &channel_status_info, &buffer_size,
			&other_info, &desc_tmp);

	bearer_desc = g_variant_get_variant(desc_tmp);

	tr = (struct treq_sat_terminal_rsp_data *)calloc(1, sizeof(struct treq_sat_terminal_rsp_data));
	tr->cmd_number = q_data.cmd_data.open_channel.command_detail.cmd_num;
	tr->cmd_type = q_data.cmd_data.open_channel.command_detail.cmd_type;
	memcpy((void*)&tr->terminal_rsp_data.open_channel.command_detail, &q_data.cmd_data.open_channel.command_detail, sizeof(struct tel_sat_cmd_detail_info));

	tr->terminal_rsp_data.open_channel.device_id.src = q_data.cmd_data.open_channel.device_id.dest;
	tr->terminal_rsp_data.open_channel.device_id.dest = q_data.cmd_data.open_channel.device_id.src;

	tr->terminal_rsp_data.open_channel.result_type = resp;
	switch (resp) {
		case RESULT_SUCCESS:
		case RESULT_SUCCESS_BUT_REQUESTED_ICON_NOT_DISPLAYED:
		case RESULT_SUCCESS_WITH_MISSING_INFO:
			//channel status
			break;

		case RESULT_ME_UNABLE_TO_PROCESS_COMMAND:
		case RESULT_NETWORK_UNABLE_TO_PROCESS_COMMAND:
			tr->terminal_rsp_data.open_channel.me_problem_type = me_problem;
			break;

		case RESULT_BEARER_INDEPENDENT_PROTOCOL_ERROR:
			tr->terminal_rsp_data.open_channel.bip_problem_type = bip_problem;
			break;

		default:
			break;
	}

	tr->terminal_rsp_data.open_channel.channel_status.channel_id = channel_id;
	tr->terminal_rsp_data.open_channel.channel_status.status = channel_status;
	tr->terminal_rsp_data.open_channel.channel_status.status_info = channel_status_info;

	dbg("check channel id(%d) channel status(%d) channel info(%d)", channel_id, channel_status, channel_status_info);

	//memcpy(tr->terminal_rsp_data.open_channel.buffer_size.size, &buffer_size, sizeof(unsigned char)*2);
	tr->terminal_rsp_data.open_channel.buffer_size.size[0] = buffer_size >> 8;
	tr->terminal_rsp_data.open_channel.buffer_size.size[1] = buffer_size & 0xFF;
	dbg("check buffer size[0](0x%x) size[1](0x%x)", tr->terminal_rsp_data.open_channel.buffer_size.size[0], tr->terminal_rsp_data.open_channel.buffer_size.size[1]);

	tr->terminal_rsp_data.open_channel.bearer_desc.bearer_type = bearer_type;
	switch(bearer_type){
		case BEARER_CSD:{
			gint data_rate, service_type, conn_element_type;

			dbg("bearer_desc cs bearer type_format(%s)", g_variant_get_type_string(bearer_desc));
			g_variant_get(bearer_desc, "(iii)",&data_rate, &service_type, &conn_element_type);
			dbg("check cs bearer data_rade(%d), service_type(%d), conn_element_type(%d)", data_rate, service_type, conn_element_type);

			tr->terminal_rsp_data.open_channel.bearer_desc.bearer_parameter.cs_bearer_param.data_rate = data_rate;
			tr->terminal_rsp_data.open_channel.bearer_desc.bearer_parameter.cs_bearer_param.service_type = service_type;
			tr->terminal_rsp_data.open_channel.bearer_desc.bearer_parameter.cs_bearer_param.connection_element_type = conn_element_type;
		}break;
		case BEARER_GPRS:{
			gint precedence_class, delay_class, reliability_class;
			gint peak_class, mean_class, pdp_type;

			dbg("bearer_desc ps bearer type_format(%s)", g_variant_get_type_string(bearer_desc));
			g_variant_get(bearer_desc, "(iiiiii)",&precedence_class, &delay_class, &reliability_class,
					&peak_class, &mean_class, &pdp_type);
			dbg("check ps bearer precedence class(%d), delay class(%d), reliability class(%d) peak class(%d) mean class(%d) pdp_type(%d)",
					precedence_class, delay_class, reliability_class, peak_class, mean_class, pdp_type);

			tr->terminal_rsp_data.open_channel.bearer_desc.bearer_parameter.ps_bearer_param.precedence_class = precedence_class;
			tr->terminal_rsp_data.open_channel.bearer_desc.bearer_parameter.ps_bearer_param.delay_class = delay_class;
			tr->terminal_rsp_data.open_channel.bearer_desc.bearer_parameter.ps_bearer_param.reliability_class = reliability_class;
			tr->terminal_rsp_data.open_channel.bearer_desc.bearer_parameter.ps_bearer_param.peak_throughput_class = peak_class;
			tr->terminal_rsp_data.open_channel.bearer_desc.bearer_parameter.ps_bearer_param.mean_throughput_class = mean_class;
			tr->terminal_rsp_data.open_channel.bearer_desc.bearer_parameter.ps_bearer_param.pdp_type = pdp_type;
		}break;
		case BEARER_LOCAL_LINK_TECHNOLOGY_INDEPENDENT:{
			gint service_type;
			gchar *service_record = NULL;

			dbg("bearer_desc link local type_format(%s)", g_variant_get_type_string(bearer_desc));
			g_variant_get(bearer_desc, "(is)",&service_type, &service_record);
			dbg("check link local service_type(%d), service_record(%d)", service_type, service_record);

			tr->terminal_rsp_data.open_channel.bearer_desc.bearer_parameter.local_link_bearer_param.service_type = service_type;

			if(service_record)
				memcpy(tr->terminal_rsp_data.open_channel.bearer_desc.bearer_parameter.local_link_bearer_param.service_record, service_record, strlen(service_record));

		}break;
		default:
		break;
	}

	result = TRUE;
	rv = sat_manager_send_terminal_response(ctx->comm, plg, tr);
	if(rv != TCORE_RETURN_SUCCESS){
		dbg("fail to send terminal response");
		result = FALSE;
	}

	g_free(tr);
	return result;
}

static gboolean sat_manager_handle_close_channel_result(struct custom_data *ctx, TcorePlugin *plg, gint command_id, GVariant *exec_result)
{
	TReturn rv = TCORE_RETURN_FAILURE;
	gboolean result = FALSE;

	gint resp, me_problem, bip_problem;

	struct treq_sat_terminal_rsp_data *tr;
	struct sat_manager_queue_data q_data;

	memset(&q_data, 0, sizeof(struct sat_manager_queue_data));

	if (sat_manager_dequeue_cmd_by_id(ctx, &q_data, command_id) == FALSE) {
		dbg("[SAT] command dequeue failed. didn't find in command Q!!");
		return result;
	}

	if(!exec_result){
		dbg("[SAT] close channel data is null");
		return result;
	}

	if (!plg){
		dbg("there is no valid plugin at this point");
		return result;
	}

	dbg("exec_result type_format(%s)", g_variant_get_type_string(exec_result));
	g_variant_get(exec_result, "(iii)",&resp, &me_problem, &bip_problem);

	tr = (struct treq_sat_terminal_rsp_data *)calloc(1, sizeof(struct treq_sat_terminal_rsp_data));
	tr->cmd_number = q_data.cmd_data.close_channel.command_detail.cmd_num;
	tr->cmd_type = q_data.cmd_data.close_channel.command_detail.cmd_type;
	memcpy((void*)&tr->terminal_rsp_data.close_channel.command_detail, &q_data.cmd_data.close_channel.command_detail, sizeof(struct tel_sat_cmd_detail_info));

	tr->terminal_rsp_data.close_channel.device_id.src = DEVICE_ID_ME;
	tr->terminal_rsp_data.close_channel.device_id.dest = q_data.cmd_data.close_channel.device_id.src;

	tr->terminal_rsp_data.close_channel.result_type = resp;
	switch (resp) {
		case RESULT_SUCCESS:
		case RESULT_SUCCESS_BUT_REQUESTED_ICON_NOT_DISPLAYED:
		case RESULT_SUCCESS_WITH_MISSING_INFO:
			//channel status
			break;

		case RESULT_ME_UNABLE_TO_PROCESS_COMMAND:
		case RESULT_NETWORK_UNABLE_TO_PROCESS_COMMAND:
			tr->terminal_rsp_data.close_channel.me_problem_type = me_problem;
			break;

		case RESULT_BEARER_INDEPENDENT_PROTOCOL_ERROR:
			tr->terminal_rsp_data.close_channel.bip_problem_type = bip_problem;
			break;

		default:
			break;
	}

	result = TRUE;
	rv = sat_manager_send_terminal_response(ctx->comm, plg, tr);
	if(rv != TCORE_RETURN_SUCCESS){
		dbg("fail to send terminal response");
		result = FALSE;
	}

	g_free(tr);
	return result;
}

static gboolean sat_manager_handle_receive_data_result(struct custom_data *ctx, TcorePlugin *plg, gint command_id, GVariant *exec_result)
{
	TReturn rv = TCORE_RETURN_FAILURE;
	gboolean result = FALSE;

	gint resp, me_problem, bip_problem;
	gint data_str_len, data_len;
	gboolean other_info;
	GVariant *received_data;

	struct treq_sat_terminal_rsp_data *tr;
	struct sat_manager_queue_data q_data;

	memset(&q_data, 0, sizeof(struct sat_manager_queue_data));

	if (sat_manager_dequeue_cmd_by_id(ctx, &q_data, command_id) == FALSE) {
		dbg("[SAT] command dequeue failed. didn't find in command Q!!");
		return result;
	}

	if(!exec_result){
		dbg("[SAT] receive data data is null");
		return result;
	}

	if (!plg){
		dbg("there is no valid plugin at this point");
		return result;
	}

	dbg("exec_result type_format(%s)", g_variant_get_type_string(exec_result));
	g_variant_get(exec_result, "(iiiiibv)",&resp, &me_problem, &bip_problem, &data_str_len, &data_len, &other_info, &received_data);

	tr = (struct treq_sat_terminal_rsp_data *)calloc(1, sizeof(struct treq_sat_terminal_rsp_data));
	tr->cmd_number = q_data.cmd_data.receive_data.command_detail.cmd_num;
	tr->cmd_type = q_data.cmd_data.receive_data.command_detail.cmd_type;
	memcpy((void*)&tr->terminal_rsp_data.receive_data.command_detail, &q_data.cmd_data.receive_data.command_detail, sizeof(struct tel_sat_cmd_detail_info));

	tr->terminal_rsp_data.receive_data.device_id.src = DEVICE_ID_ME;
	tr->terminal_rsp_data.receive_data.device_id.dest = q_data.cmd_data.receive_data.device_id.src;

	tr->terminal_rsp_data.receive_data.result_type = resp;
	switch (resp) {
		case RESULT_SUCCESS:
		case RESULT_SUCCESS_BUT_REQUESTED_ICON_NOT_DISPLAYED:
		case RESULT_SUCCESS_WITH_MISSING_INFO:
			//channel status
			break;

		case RESULT_ME_UNABLE_TO_PROCESS_COMMAND:
		case RESULT_NETWORK_UNABLE_TO_PROCESS_COMMAND:
			tr->terminal_rsp_data.receive_data.me_problem_type = me_problem;
			break;

		case RESULT_BEARER_INDEPENDENT_PROTOCOL_ERROR:
			tr->terminal_rsp_data.receive_data.bip_problem_type = bip_problem;
			break;

		default:
			break;
	}

	tr->terminal_rsp_data.receive_data.channel_data_len.data_len = data_len;
	tr->terminal_rsp_data.receive_data.channel_data.data_string_len = data_str_len;

	if(received_data){
		int index = 0;
		guchar data;
		GVariantIter *iter = NULL;

		dbg("additional data exist type_format(%s)", g_variant_get_type_string(received_data));

		g_variant_get(received_data, "ay", &iter);
		while( g_variant_iter_loop (iter, "y", &data)){
			dbg("index(%d) data(%d)", index, data);
			tr->terminal_rsp_data.receive_data.channel_data.data_string[index] = data;
			index++;
		}
		g_variant_iter_free(iter);

		dbg("the last index data(%d), data_total_len(%d)", index, data_str_len);
	}

	result = TRUE;
	rv = sat_manager_send_terminal_response(ctx->comm, plg, tr);
	if(rv != TCORE_RETURN_SUCCESS){
		dbg("fail to send terminal response");
		result = FALSE;
	}

	g_free(tr);
	return result;
}

static gboolean sat_manager_handle_send_data_result(struct custom_data *ctx, TcorePlugin *plg, gint command_id, GVariant *exec_result)
{
	TReturn rv = TCORE_RETURN_FAILURE;
	gboolean result = FALSE;

	gint resp, me_problem, bip_problem;
	gint data_len;

	struct treq_sat_terminal_rsp_data *tr;
	struct sat_manager_queue_data q_data;

	memset(&q_data, 0, sizeof(struct sat_manager_queue_data));

	if (sat_manager_dequeue_cmd_by_id(ctx, &q_data, command_id) == FALSE) {
		dbg("[SAT] command dequeue failed. didn't find in command Q!!");
		return result;
	}

	if(!exec_result){
		dbg("[SAT] send data data is null");
		return result;
	}

	if (!plg){
		dbg("there is no valid plugin at this point");
		return result;
	}

	dbg("exec_result type_format(%s)", g_variant_get_type_string(exec_result));
	g_variant_get(exec_result, "(iiii)",&resp, &me_problem, &bip_problem, &data_len);

	tr = (struct treq_sat_terminal_rsp_data *)calloc(1, sizeof(struct treq_sat_terminal_rsp_data));
	tr->cmd_number = q_data.cmd_data.send_data.command_detail.cmd_num;
	tr->cmd_type = q_data.cmd_data.send_data.command_detail.cmd_type;
	memcpy((void*)&tr->terminal_rsp_data.send_data.command_detail, &q_data.cmd_data.send_data.command_detail, sizeof(struct tel_sat_cmd_detail_info));

	tr->terminal_rsp_data.send_data.device_id.src = DEVICE_ID_ME;
	tr->terminal_rsp_data.send_data.device_id.dest = q_data.cmd_data.send_data.device_id.src;

	tr->terminal_rsp_data.send_data.result_type = resp;
	switch (resp) {
		case RESULT_SUCCESS:
		case RESULT_SUCCESS_BUT_REQUESTED_ICON_NOT_DISPLAYED:
		case RESULT_SUCCESS_WITH_MISSING_INFO:
			//channel status
			break;

		case RESULT_ME_UNABLE_TO_PROCESS_COMMAND:
		case RESULT_NETWORK_UNABLE_TO_PROCESS_COMMAND:
			tr->terminal_rsp_data.send_data.me_problem_type = me_problem;
			break;

		case RESULT_BEARER_INDEPENDENT_PROTOCOL_ERROR:
			tr->terminal_rsp_data.send_data.bip_problem_type = bip_problem;
			break;

		default:
			break;
	}

	tr->terminal_rsp_data.send_data.channel_data_len.data_len = data_len;

	result = TRUE;
	rv = sat_manager_send_terminal_response(ctx->comm, plg, tr);
	if(rv != TCORE_RETURN_SUCCESS){
		dbg("fail to send terminal response");
		result = FALSE;
	}

	g_free(tr);
	return result;
}

static gboolean sat_manager_handle_get_channel_status_result(struct custom_data *ctx, TcorePlugin *plg, gint command_id, GVariant *exec_result)
{
	TReturn rv = TCORE_RETURN_FAILURE;
	gboolean result = FALSE;

	gint resp, me_problem, bip_problem;
	gint channel_id, channel_status, channel_status_info;

	struct treq_sat_terminal_rsp_data *tr;
	struct sat_manager_queue_data q_data;

	memset(&q_data, 0, sizeof(struct sat_manager_queue_data));

	if (sat_manager_dequeue_cmd_by_id(ctx, &q_data, command_id) == FALSE) {
		dbg("[SAT] command dequeue failed. didn't find in command Q!!");
		return result;
	}

	if(!exec_result){
		dbg("[SAT] get channel status data is null");
		return result;
	}

	if (!plg){
		dbg("there is no valid plugin at this point");
		return result;
	}

	dbg("exec_result type_format(%s)", g_variant_get_type_string(exec_result));
	g_variant_get(exec_result, "(iiiiii)",&resp, &me_problem, &bip_problem,
			&channel_id, &channel_status, &channel_status_info);

	tr = (struct treq_sat_terminal_rsp_data *)calloc(1, sizeof(struct treq_sat_terminal_rsp_data));
	tr->cmd_number = q_data.cmd_data.get_channel_status.command_detail.cmd_num;
	tr->cmd_type = q_data.cmd_data.get_channel_status.command_detail.cmd_type;
	memcpy((void*)&tr->terminal_rsp_data.get_channel_status.command_detail, &q_data.cmd_data.get_channel_status.command_detail, sizeof(struct tel_sat_cmd_detail_info));

	tr->terminal_rsp_data.get_channel_status.device_id.src = q_data.cmd_data.get_channel_status.device_id.dest;
	tr->terminal_rsp_data.get_channel_status.device_id.dest = q_data.cmd_data.get_channel_status.device_id.src;

	tr->terminal_rsp_data.get_channel_status.result_type = resp;
	switch (resp) {
		case RESULT_SUCCESS:
		case RESULT_SUCCESS_BUT_REQUESTED_ICON_NOT_DISPLAYED:
		case RESULT_SUCCESS_WITH_MISSING_INFO:
			//channel status
			break;

		case RESULT_ME_UNABLE_TO_PROCESS_COMMAND:
		case RESULT_NETWORK_UNABLE_TO_PROCESS_COMMAND:
			tr->terminal_rsp_data.get_channel_status.me_problem_type = me_problem;
			break;

		case RESULT_BEARER_INDEPENDENT_PROTOCOL_ERROR:
			tr->terminal_rsp_data.get_channel_status.bip_problem_type = bip_problem;
			break;

		default:
			break;
	}

	tr->terminal_rsp_data.get_channel_status.channel_status.channel_id = channel_id;
	tr->terminal_rsp_data.get_channel_status.channel_status.status = channel_status;
	tr->terminal_rsp_data.get_channel_status.channel_status.status_info = channel_status_info;

	result = TRUE;
	rv = sat_manager_send_terminal_response(ctx->comm, plg, tr);
	if(rv != TCORE_RETURN_SUCCESS){
		dbg("fail to send terminal response");
		result = FALSE;
	}

	g_free(tr);
	return result;
}

static gboolean sat_manager_handle_send_dtmf_result(struct custom_data *ctx, TcorePlugin *plg, gint command_id, GVariant *exec_result)
{
	TReturn rv = TCORE_RETURN_FAILURE;
	gboolean result = FALSE;

	gint resp;
	struct treq_sat_terminal_rsp_data *tr;
	struct sat_manager_queue_data q_data;

	tr = (struct treq_sat_terminal_rsp_data *)calloc(1, sizeof(struct treq_sat_terminal_rsp_data));
	memset(&q_data, 0, sizeof(struct sat_manager_queue_data));

	if (sat_manager_dequeue_cmd_by_id(ctx, &q_data, command_id) == FALSE) {
		dbg("[SAT] command dequeue failed. didn't find in command Q!!");
		g_free(tr);
		return result;
	}

	if(!exec_result){
		dbg("[SAT] get channel status data is null");
		g_free(tr);
		return result;
	}

	if (!plg){
		dbg("there is no valid plugin at this point");
		g_free(tr);
		return result;
	}

	dbg("exec_result type_format(%s)", g_variant_get_type_string(exec_result));
	g_variant_get(exec_result, "(i)",&resp);

	tr->cmd_number = q_data.cmd_data.send_dtmf.command_detail.cmd_num;
	tr->cmd_type = q_data.cmd_data.send_dtmf.command_detail.cmd_type;
	memcpy((void*)&tr->terminal_rsp_data.send_dtmf.command_detail, &q_data.cmd_data.send_dtmf.command_detail, sizeof(struct tel_sat_cmd_detail_info));

	tr->terminal_rsp_data.send_dtmf.device_id.src = DEVICE_ID_ME;
	tr->terminal_rsp_data.send_dtmf.device_id.dest = q_data.cmd_data.send_dtmf.device_id.src;

	tr->terminal_rsp_data.send_dtmf.result_type = resp;
	switch (resp) {
		case RESULT_SUCCESS:
			if (q_data.cmd_data.send_dtmf.icon_id.icon_info.ics == IMAGE_CODING_SCHEME_COLOUR)
				tr->terminal_rsp_data.send_dtmf.result_type = RESULT_SUCCESS_BUT_REQUESTED_ICON_NOT_DISPLAYED;
			break;

		case RESULT_ME_UNABLE_TO_PROCESS_COMMAND:
			tr->terminal_rsp_data.send_dtmf.me_problem_type = ME_PROBLEM_NOT_IN_SPEECH_CALL;
			break;

		default:
			tr->terminal_rsp_data.send_dtmf.result_type = RESULT_ME_UNABLE_TO_PROCESS_COMMAND;
			tr->terminal_rsp_data.send_dtmf.me_problem_type = ME_PROBLEM_NO_SPECIFIC_CAUSE;
			break;
	}

	result = TRUE;
	rv = sat_manager_send_terminal_response(ctx->comm, plg, tr);
	if(rv != TCORE_RETURN_SUCCESS){
		dbg("fail to send terminal response");
		result = FALSE;
	}

	g_free(tr);
	return result;
}

static gboolean sat_manager_handle_launch_browser_result(struct custom_data *ctx, TcorePlugin *plg, gint command_id, GVariant *exec_result)
{
	TReturn rv = TCORE_RETURN_FAILURE;
	gboolean result = FALSE;

	gint resp, browser_problem;
	struct treq_sat_terminal_rsp_data *tr;
	struct sat_manager_queue_data q_data;

	memset(&q_data, 0, sizeof(struct sat_manager_queue_data));

	if (sat_manager_dequeue_cmd_by_id(ctx, &q_data, command_id) == FALSE) {
		dbg("[SAT] command dequeue failed. didn't find in command Q!!");
		return result;
	}

	if(!exec_result){
		dbg("[SAT] get channel status data is null");
		return result;
	}

	if (!plg){
		dbg("there is no valid plugin at this point");
		return result;
	}

	dbg("exec_result type_format(%s)", g_variant_get_type_string(exec_result));
	g_variant_get(exec_result, "(ii)",&resp,&browser_problem);

	tr = (struct treq_sat_terminal_rsp_data *)calloc(1, sizeof(struct treq_sat_terminal_rsp_data));
	tr->cmd_number = q_data.cmd_data.launch_browser.command_detail.cmd_num;
	tr->cmd_type = q_data.cmd_data.launch_browser.command_detail.cmd_type;
	memcpy((void*)&tr->terminal_rsp_data.launch_browser.command_detail, &q_data.cmd_data.launch_browser.command_detail, sizeof(struct tel_sat_cmd_detail_info));

	tr->terminal_rsp_data.launch_browser.device_id.src = q_data.cmd_data.launch_browser.device_id.dest;
	tr->terminal_rsp_data.launch_browser.device_id.dest = q_data.cmd_data.launch_browser.device_id.src;

	tr->terminal_rsp_data.launch_browser.result_type = resp;
	switch (resp) {
		case RESULT_SUCCESS:
			if (q_data.cmd_data.launch_browser.user_confirm_icon_id.icon_info.ics == IMAGE_CODING_SCHEME_COLOUR)
				tr->terminal_rsp_data.launch_browser.result_type = RESULT_SUCCESS_BUT_REQUESTED_ICON_NOT_DISPLAYED;

			tr->terminal_rsp_data.launch_browser.me_problem_type = ME_PROBLEM_NO_SPECIFIC_CAUSE;
			tr->terminal_rsp_data.launch_browser.browser_problem_type = BROWSER_PROBLEM_NO_SPECIFIC_CAUSE;
			break;
		case RESULT_ME_UNABLE_TO_PROCESS_COMMAND:
			tr->terminal_rsp_data.launch_browser.me_problem_type = ME_PROBLEM_NO_SPECIFIC_CAUSE;
			tr->terminal_rsp_data.launch_browser.browser_problem_type = BROWSER_PROBLEM_NO_SPECIFIC_CAUSE;
			break;

		case RESULT_LAUNCH_BROWSER_GENERIC_ERROR_CODE:
			tr->terminal_rsp_data.launch_browser.me_problem_type = ME_PROBLEM_NO_SPECIFIC_CAUSE;
			tr->terminal_rsp_data.launch_browser.browser_problem_type = browser_problem;
			break;
		default:
			tr->terminal_rsp_data.launch_browser.result_type = RESULT_ME_UNABLE_TO_PROCESS_COMMAND;
			tr->terminal_rsp_data.launch_browser.me_problem_type = ME_PROBLEM_NO_SPECIFIC_CAUSE;
			tr->terminal_rsp_data.launch_browser.browser_problem_type = BROWSER_PROBLEM_NO_SPECIFIC_CAUSE;
			break;
	}

	result = TRUE;
	rv = sat_manager_send_terminal_response(ctx->comm, plg, tr);
	if(rv != TCORE_RETURN_SUCCESS){
		dbg("fail to send terminal response");
		result = FALSE;
	}

	g_free(tr);
	return result;
}

gboolean sat_manager_handle_app_exec_result(struct custom_data *ctx, TcorePlugin *plg, gint command_id, gint command_type, GVariant *exec_result)
{
	gboolean result = FALSE;
	GVariant *resp = NULL;
	dbg("[SAT] app exec result command id(%d) command type(%d)", command_id, command_type);

	dbg("exec_result type_format(%s)", g_variant_get_type_string(exec_result));
	g_variant_get(exec_result, "v", &resp);

	switch (command_type) {
		case SAT_PROATV_CMD_SETUP_MENU:
			result = _sat_manager_handle_setup_menu_result(ctx, plg, command_id, resp);
			break;

		case SAT_PROATV_CMD_DISPLAY_TEXT:
			result = _sat_manager_handle_display_text_result(ctx, plg, command_id, resp);
			break;

		case SAT_PROATV_CMD_PLAY_TONE:
			result = _sat_manager_handle_play_tone_result(ctx, plg, command_id, resp);
			break;

		case SAT_PROATV_CMD_SEND_SMS:
			result = _sat_manager_handle_send_sms_result(ctx, plg, command_id, resp);
			break;

		case SAT_PROATV_CMD_SEND_SS:
			result = _sat_manager_handle_send_ss_result(ctx, plg, command_id, resp);
			break;

		case SAT_PROATV_CMD_SEND_USSD:
			result = _sat_manager_handle_send_ussd_result(ctx, plg, command_id, resp);
			break;

		case SAT_PROATV_CMD_SETUP_CALL:
			result = _sat_manager_handle_setup_call_result(ctx, plg, command_id, resp);
			break;

		case SAT_PROATV_CMD_SETUP_IDLE_MODE_TEXT:
			result = _sat_manager_handle_setup_idle_mode_text_result(ctx, plg, command_id, resp);
			break;

		case SAT_PROATV_CMD_OPEN_CHANNEL:
			result = sat_manager_handle_open_channel_result(ctx, plg, command_id, resp);
			break;

		case SAT_PROATV_CMD_CLOSE_CHANNEL:
			result = sat_manager_handle_close_channel_result(ctx, plg, command_id, resp);
			break;

		case SAT_PROATV_CMD_RECEIVE_DATA:
			result = sat_manager_handle_receive_data_result(ctx, plg, command_id, resp);
			break;

		case SAT_PROATV_CMD_SEND_DATA:
			result = sat_manager_handle_send_data_result(ctx, plg, command_id, resp);
			break;

		case SAT_PROATV_CMD_GET_CHANNEL_STATUS:
			result = sat_manager_handle_get_channel_status_result(ctx, plg, command_id, resp);
			break;

		case SAT_PROATV_CMD_SEND_DTMF:
			result = sat_manager_handle_send_dtmf_result(ctx, plg, command_id, resp);
			break;

		case SAT_PROATV_CMD_LAUNCH_BROWSER:
			result = sat_manager_handle_launch_browser_result(ctx, plg, command_id, resp);
			break;

		default:
			dbg("[SAT] invalid command type(%d)", command_type);
			break;
	}

	sat_ui_support_terminate_sat_ui();

	return result;
}

static gboolean _sat_manager_handle_menu_select_confirm(struct custom_data *ctx, TcorePlugin *plg, gint command_id, gint confirm_type, GVariant *addtional_data)
{
	TReturn rv = TCORE_RETURN_FAILURE;
	gboolean result = FALSE;

	gint item_id=0;
	struct treq_sat_terminal_rsp_data *tr;
	struct sat_manager_queue_data q_data;

	memset(&q_data, 0, sizeof(struct sat_manager_queue_data));

	if (sat_manager_dequeue_cmd_by_id(ctx, &q_data, command_id) == FALSE) {
		dbg("[SAT] command dequeue failed. didn't find in command Q!!");
		return result;
	}

	if (!plg){
		dbg("there is no valid plugin at this point");
		return result;
	}

	if(addtional_data){
		int index = 0;
		guchar data;
		GVariantIter *iter = NULL;
		GVariant *inner_gv = NULL;

		inner_gv = g_variant_get_variant(addtional_data);
		dbg("additional data exist type_format(%s)", g_variant_get_type_string(inner_gv));

		g_variant_get(inner_gv, "ay", &iter);
		while( g_variant_iter_loop (iter, "y", &data)){
			dbg("index(%d) data(%d)", index, data);
			item_id = data;
			index++;
		}
		g_variant_iter_free(iter);
		g_variant_unref(inner_gv);
	}

	tr = (struct treq_sat_terminal_rsp_data *)calloc(1, sizeof(struct treq_sat_terminal_rsp_data));
	tr->cmd_number = q_data.cmd_data.selectItemInd.command_detail.cmd_num;
	tr->cmd_type = q_data.cmd_data.selectItemInd.command_detail.cmd_type;
	memcpy((void*)&tr->terminal_rsp_data.select_item.command_detail, &q_data.cmd_data.selectItemInd.command_detail, sizeof(struct tel_sat_cmd_detail_info));
	tr->terminal_rsp_data.select_item.device_id.src = DEVICE_ID_ME;
	tr->terminal_rsp_data.select_item.device_id.dest = DEVICE_ID_SIM;

	switch (confirm_type) {
		case USER_CONFIRM_YES:
			tr->terminal_rsp_data.select_item.item_identifier.item_identifier = item_id;
			tr->terminal_rsp_data.select_item.other_info = FALSE;
			tr->terminal_rsp_data.select_item.result_type = RESULT_SUCCESS;
			tr->terminal_rsp_data.select_item.me_problem_type = ME_PROBLEM_NO_SPECIFIC_CAUSE;

			if (q_data.cmd_data.selectItemInd.icon_id.icon_info.ics == IMAGE_CODING_SCHEME_COLOUR)
				tr->terminal_rsp_data.select_item.result_type = RESULT_SUCCESS_BUT_REQUESTED_ICON_NOT_DISPLAYED;

			break;

		case USER_CONFIRM_HELP_INFO:
			tr->terminal_rsp_data.select_item.item_identifier.item_identifier = item_id;
			tr->terminal_rsp_data.select_item.other_info = FALSE;
			tr->terminal_rsp_data.select_item.result_type = RESULT_HELP_INFO_REQUIRED_BY_USER;
			tr->terminal_rsp_data.select_item.me_problem_type = ME_PROBLEM_NO_SPECIFIC_CAUSE;
			//TODO ctx->help_requested = TRUE;
			break;

		case USER_CONFIRM_END:
			tr->terminal_rsp_data.select_item.result_type = RESULT_PROACTIVE_SESSION_TERMINATED_BY_USER;
			break;

		case USER_CONFIRM_NO_OR_CANCEL:
			tr->terminal_rsp_data.select_item.result_type = RESULT_BACKWARD_MOVE_BY_USER;
			break;

		case USER_CONFIRM_TIMEOUT:
			tr->terminal_rsp_data.select_item.result_type = RESULT_NO_RESPONSE_FROM_USER;
			break;

		default:
			dbg("not handled value[%d] here", confirm_type);
			break;
	}

	result = TRUE;
	rv = sat_manager_send_terminal_response(ctx->comm, plg, tr);
	if(rv != TCORE_RETURN_SUCCESS){
		dbg("fail to send terminal response");
		result = FALSE;
	}

	g_free(tr);

	return result;
}

static gboolean _sat_manager_handle_display_text_confirm(struct custom_data *ctx, TcorePlugin *plg, gint command_id, gint confirm_type, GVariant *addtional_data)
{
	TReturn rv = TCORE_RETURN_FAILURE;
	gboolean result = FALSE;

	struct treq_sat_terminal_rsp_data *tr;
	struct sat_manager_queue_data q_data;

	memset(&q_data, 0, sizeof(struct sat_manager_queue_data));

	if (sat_manager_dequeue_cmd_by_id(ctx, &q_data, command_id) == FALSE) {
		dbg("[SAT] command dequeue failed. didn't find in command Q!!");
		return result;
	}

	if (!plg){
		dbg("there is no valid plugin at this point");
		return result;
	}

	tr = (struct treq_sat_terminal_rsp_data *)calloc(1, sizeof(struct treq_sat_terminal_rsp_data));
	tr->cmd_number = q_data.cmd_data.displayTextInd.command_detail.cmd_num;
	tr->cmd_type = q_data.cmd_data.displayTextInd.command_detail.cmd_type;
	memcpy((void*)&tr->terminal_rsp_data.display_text.command_detail, &q_data.cmd_data.displayTextInd.command_detail, sizeof(struct tel_sat_cmd_detail_info));
	tr->terminal_rsp_data.display_text.device_id.src = DEVICE_ID_ME;
	tr->terminal_rsp_data.display_text.device_id.dest = DEVICE_ID_SIM;

	switch (confirm_type){
		case USER_CONFIRM_YES: {
			tr->terminal_rsp_data.display_text.result_type = RESULT_SUCCESS;
			tr->terminal_rsp_data.display_text.me_problem_type = ME_PROBLEM_NO_SPECIFIC_CAUSE;

			if (q_data.cmd_data.displayTextInd.icon_id.icon_info.ics == IMAGE_CODING_SCHEME_COLOUR)
				tr->terminal_rsp_data.display_text.result_type = RESULT_SUCCESS_BUT_REQUESTED_ICON_NOT_DISPLAYED;
		} break;

		case USER_CONFIRM_NO_OR_CANCEL:
			tr->terminal_rsp_data.display_text.result_type = RESULT_BACKWARD_MOVE_BY_USER;
			break;

		case USER_CONFIRM_TIMEOUT:
			tr->terminal_rsp_data.display_text.result_type = RESULT_SUCCESS;

			if (q_data.cmd_data.displayTextInd.command_detail.cmd_qualifier.display_text.text_clear_type == TEXT_WAIT_FOR_USER_TO_CLEAR_MSG )
				tr->terminal_rsp_data.display_text.result_type = RESULT_NO_RESPONSE_FROM_USER;

			break;

		case USER_CONFIRM_END:
			tr->terminal_rsp_data.display_text.result_type = RESULT_PROACTIVE_SESSION_TERMINATED_BY_USER;
			break;

		case USER_CONFIRM_HELP_INFO:
		default:
			dbg("not handled value[%d] here", confirm_type);
			break;
	}

	result = TRUE;
	rv = sat_manager_send_terminal_response(ctx->comm, plg, tr);
	if(rv != TCORE_RETURN_SUCCESS){
		dbg("fail to send terminal response");
		result = FALSE;
	}

	g_free(tr);

	return result;
}

static gboolean _sat_manager_handle_get_inkey_confirm(struct custom_data *ctx, TcorePlugin *plg, gint command_id, gint confirm_type, GVariant *addtional_data)
{
	TReturn rv = TCORE_RETURN_FAILURE;
	gboolean result = FALSE;

	gint inkey_data_len = 0;
	gchar inkey_data[SAT_TEXT_STRING_LEN_MAX];
	struct treq_sat_terminal_rsp_data *tr;
	struct sat_manager_queue_data q_data;

	memset(&q_data, 0, sizeof(struct sat_manager_queue_data));
	memset(inkey_data, 0, SAT_TEXT_STRING_LEN_MAX);

	if (sat_manager_dequeue_cmd_by_id(ctx, &q_data, command_id) == FALSE) {
		dbg("[SAT] command dequeue failed. didn't find in command Q!!");
		return result;
	}

	if (!plg){
		dbg("there is no valid plugin at this point");
		return result;
	}

	if(addtional_data){
		int index = 0;
		guchar data;
		GVariantIter *iter = NULL;
		GVariant *inner_gv = NULL;

		inner_gv = g_variant_get_variant(addtional_data);
		dbg("additional data exist type_format(%s)", g_variant_get_type_string(inner_gv));

		g_variant_get(inner_gv, "ay", &iter);
		while( g_variant_iter_loop (iter, "y", &data)){
			dbg("index(%d) data(%d)", index, data);
			inkey_data[index] = data;
			index++;
		}
		g_variant_iter_free(iter);
		g_variant_unref(inner_gv);
		inkey_data_len = index;
	}

	tr = (struct treq_sat_terminal_rsp_data *)calloc(1, sizeof(struct treq_sat_terminal_rsp_data));
	tr->cmd_number = q_data.cmd_data.getInkeyInd.command_detail.cmd_num;
	tr->cmd_type = q_data.cmd_data.getInkeyInd.command_detail.cmd_type;
	memcpy((void*)&tr->terminal_rsp_data.get_inkey.command_detail, &q_data.cmd_data.getInkeyInd.command_detail, sizeof(struct tel_sat_cmd_detail_info));
	tr->terminal_rsp_data.get_inkey.device_id.src = DEVICE_ID_ME;
	tr->terminal_rsp_data.get_inkey.device_id.dest = DEVICE_ID_SIM;

	switch (confirm_type){
		case USER_CONFIRM_YES:
			tr->terminal_rsp_data.get_inkey.result_type = RESULT_SUCCESS;

			if (q_data.cmd_data.getInkeyInd.icon_id.icon_info.ics == IMAGE_CODING_SCHEME_COLOUR)
				tr->terminal_rsp_data.get_inkey.result_type = RESULT_SUCCESS_BUT_REQUESTED_ICON_NOT_DISPLAYED;

			if (q_data.cmd_data.getInkeyInd.command_detail.cmd_qualifier.get_inkey.inkey_type == INKEY_TYPE_YES_NO_REQUESTED) {
				tr->terminal_rsp_data.get_inkey.text.dcs.m_class = MSG_CLASS_RESERVED;
				tr->terminal_rsp_data.get_inkey.text.dcs.a_format = ALPHABET_FORMAT_8BIT_DATA;
				tr->terminal_rsp_data.get_inkey.text.string_length = 1;
				tr->terminal_rsp_data.get_inkey.text.string[0] = 0x01;
			}
			else if(inkey_data_len > 0)
			{
				tr->terminal_rsp_data.get_inkey.text.string_length = inkey_data_len;

				if (!q_data.cmd_data.getInkeyInd.command_detail.cmd_qualifier.get_inkey.alphabet_set){
					tr->terminal_rsp_data.get_inkey.text.is_digit_only = TRUE;
					tr->terminal_rsp_data.get_inkey.text.dcs.a_format = ALPHABET_FORMAT_8BIT_DATA;
					tr->terminal_rsp_data.get_inkey.text.dcs.m_class = MSG_CLASS_RESERVED;

					memcpy((void*)tr->terminal_rsp_data.get_inkey.text.string, inkey_data, inkey_data_len);
				}
				else
				{
					tr->terminal_rsp_data.get_inkey.text.dcs.m_class = MSG_CLASS_RESERVED;

					if(q_data.cmd_data.getInkeyInd.command_detail.cmd_qualifier.get_inkey.alphabet_type == INPUT_ALPHABET_TYPE_SMS_DEFAULT )
					{
						int tmp_len;
						char tmp_str[SAT_TEXT_STRING_LEN_MAX + 1], *packed_data;

						dbg("sat gsm7 encoding");
						tcore_util_convert_utf8_to_gsm((unsigned char*) tmp_str, &tmp_len, (unsigned char*)inkey_data, inkey_data_len);
						packed_data = (char*) tcore_util_pack_gsm7bit((const unsigned char *)tmp_str, tmp_len);

						if(packed_data){
							tr->terminal_rsp_data.get_inkey.text.dcs.a_format = ALPHABET_FORMAT_8BIT_DATA;
							tr->terminal_rsp_data.get_inkey.text.string_length = strlen(packed_data);
							memcpy((void*) tr->terminal_rsp_data.get_inkey.text.string, packed_data, strlen(packed_data));

							g_free(packed_data);
						}
					}
					else if(q_data.cmd_data.getInkeyInd.command_detail.cmd_qualifier.get_inkey.alphabet_type == INPUT_ALPHABET_TYPE_UCS2 )
					{
						dbg("UCS2 DATA");

						tr->terminal_rsp_data.get_inkey.text.dcs.a_format = ALPHABET_FORMAT_UCS2;
						tcore_util_convert_utf8_to_ucs2((unsigned char*)tr->terminal_rsp_data.get_inkey.text.string,
								&tr->terminal_rsp_data.get_inkey.text.string_length, (unsigned char*)inkey_data, inkey_data_len);
					}
					else
					{
						tr->terminal_rsp_data.get_inkey.text.dcs.a_format = ALPHABET_FORMAT_RESERVED;
						dbg("[SAT] invalid DCS[%d]",tr->terminal_rsp_data.get_inkey.text.dcs.a_format);
					}
				}
			}
			break;

		case USER_CONFIRM_HELP_INFO:
			tr->terminal_rsp_data.get_inkey.result_type = RESULT_HELP_INFO_REQUIRED_BY_USER;
			//TODO ctx->help_requested = TRUE;
			break;

		case USER_CONFIRM_NO_OR_CANCEL:
			tr->terminal_rsp_data.get_inkey.result_type = RESULT_BACKWARD_MOVE_BY_USER;

			if (q_data.cmd_data.getInkeyInd.command_detail.cmd_qualifier.get_inkey.inkey_type == INKEY_TYPE_YES_NO_REQUESTED) {
				tr->terminal_rsp_data.get_inkey.result_type = RESULT_SUCCESS;
				tr->terminal_rsp_data.get_inkey.text.dcs.m_class = MSG_CLASS_RESERVED;
				tr->terminal_rsp_data.get_inkey.text.dcs.a_format = ALPHABET_FORMAT_8BIT_DATA;
				tr->terminal_rsp_data.get_inkey.text.string_length = 1;
				tr->terminal_rsp_data.get_inkey.text.string[0] = 0x00;
			}
			break;

		case USER_CONFIRM_TIMEOUT:
			tr->terminal_rsp_data.get_inkey.result_type = RESULT_NO_RESPONSE_FROM_USER;
			break;

		case USER_CONFIRM_END:
			tr->terminal_rsp_data.get_inkey.result_type = RESULT_PROACTIVE_SESSION_TERMINATED_BY_USER;
			break;

		default:
			dbg("not handled value[%d] here", confirm_type);
			break;
	}

	result = TRUE;
	rv = sat_manager_send_terminal_response(ctx->comm, plg, tr);
	if(rv != TCORE_RETURN_SUCCESS){
		dbg("fail to send terminal response");
		result = FALSE;
	}

	g_free(tr);

	return result;
}

static gboolean _sat_manager_handle_get_input_confirm(struct custom_data *ctx, TcorePlugin *plg, gint command_id, gint confirm_type, GVariant *addtional_data)
{
	TReturn rv = TCORE_RETURN_FAILURE;
	gboolean result = FALSE;

	gint input_data_len = 0;
	gchar input_data[SAT_TEXT_STRING_LEN_MAX];
	struct treq_sat_terminal_rsp_data *tr;
	struct sat_manager_queue_data q_data;

	memset(&q_data, 0, sizeof(struct sat_manager_queue_data));
	memset(input_data, 0, SAT_TEXT_STRING_LEN_MAX);

	if (sat_manager_dequeue_cmd_by_id(ctx, &q_data, command_id) == FALSE) {
		dbg("[SAT] command dequeue failed. didn't find in command Q!!");
		return result;
	}

	if (!plg){
		dbg("there is no valid plugin at this point");
		return result;
	}

	if(addtional_data){
		int index = 0;
		guchar data;
		GVariantIter *iter = NULL;
		GVariant *inner_gv = NULL;

		inner_gv = g_variant_get_variant(addtional_data);
		dbg("additional data exist type_format(%s)", g_variant_get_type_string(inner_gv));

		g_variant_get(inner_gv, "ay", &iter);
		while( g_variant_iter_loop (iter, "y", &data)){
			dbg("index(%d) data(%d)", index, data);
			input_data[index] = data;
			index++;
		}
		g_variant_iter_free(iter);
		g_variant_unref(inner_gv);
		input_data_len = index;
	}

	tr = (struct treq_sat_terminal_rsp_data *)calloc(1, sizeof(struct treq_sat_terminal_rsp_data));
	tr->cmd_number = q_data.cmd_data.getInputInd.command_detail.cmd_num;
	tr->cmd_type = q_data.cmd_data.getInputInd.command_detail.cmd_type;
	memcpy((void*)&tr->terminal_rsp_data.get_input.command_detail, &q_data.cmd_data.getInputInd.command_detail, sizeof(struct tel_sat_cmd_detail_info));
	tr->terminal_rsp_data.get_input.device_id.src = DEVICE_ID_ME;
	tr->terminal_rsp_data.get_input.device_id.dest = DEVICE_ID_SIM;

	switch (confirm_type){
		case USER_CONFIRM_YES:
			tr->terminal_rsp_data.get_input.result_type = RESULT_SUCCESS;
			tr->terminal_rsp_data.get_input.text.dcs.m_class = MSG_CLASS_RESERVED;

			if(!q_data.cmd_data.getInputInd.command_detail.cmd_qualifier.get_input.alphabet_set){
				tr->terminal_rsp_data.get_input.text.is_digit_only = TRUE;
			}

			if (q_data.cmd_data.getInputInd.icon_id.icon_info.ics == IMAGE_CODING_SCHEME_COLOUR)
				tr->terminal_rsp_data.get_input.result_type = RESULT_SUCCESS_BUT_REQUESTED_ICON_NOT_DISPLAYED;

			if(!q_data.cmd_data.getInputInd.command_detail.cmd_qualifier.get_input.user_input_unpacked_format){
				dbg("[SAT] packing to SMS7 default");

				tr->terminal_rsp_data.get_input.text.string_length = 0;
				tr->terminal_rsp_data.get_input.text.dcs.a_format = ALPHABET_FORMAT_SMS_DEFAULT;

				if (input_data_len > 0){
					int tmp_len;
					char tmp_str[SAT_TEXT_STRING_LEN_MAX + 1], *packed_data;

					dbg("sat gsm7 encoding");
					tcore_util_convert_utf8_to_gsm((unsigned char*)tmp_str, &tmp_len, (unsigned char*)input_data, input_data_len);
					packed_data = (char*) tcore_util_pack_gsm7bit((const unsigned char *)tmp_str, tmp_len);

					if(packed_data){
						memcpy((void*)tr->terminal_rsp_data.get_input.text.string, packed_data, strlen(packed_data));
						tr->terminal_rsp_data.get_input.text.string_length = strlen(packed_data);

						g_free(packed_data);
					}
				}

			}
			else
			{
				dbg("[SAT] packing not required");

				if(q_data.cmd_data.getInkeyInd.command_detail.cmd_qualifier.get_input.alphabet_type == INPUT_ALPHABET_TYPE_SMS_DEFAULT){

					tr->terminal_rsp_data.get_input.text.dcs.a_format = ALPHABET_FORMAT_8BIT_DATA;
					tcore_util_convert_utf8_to_gsm((unsigned char*)tr->terminal_rsp_data.get_input.text.string,
							&tr->terminal_rsp_data.get_input.text.string_length, (unsigned char*)input_data, input_data_len);

				}
				else if(q_data.cmd_data.getInkeyInd.command_detail.cmd_qualifier.get_input.alphabet_type == INPUT_ALPHABET_TYPE_UCS2 ){

					tr->terminal_rsp_data.get_input.text.dcs.a_format = ALPHABET_FORMAT_UCS2;
					tcore_util_convert_utf8_to_ucs2((unsigned char*)tr->terminal_rsp_data.get_input.text.string,
							&tr->terminal_rsp_data.get_input.text.string_length, (unsigned char*)input_data, input_data_len);

				}
				else{
					tr->terminal_rsp_data.get_input.text.dcs.a_format = ALPHABET_FORMAT_RESERVED;
					dbg("[SAT] invalid DCS[%d]",tr->terminal_rsp_data.get_input.text.dcs.a_format);
				}
			} break;

		case USER_CONFIRM_HELP_INFO:
			tr->terminal_rsp_data.get_input.result_type = RESULT_HELP_INFO_REQUIRED_BY_USER;
			//TODO ctx->help_requested = TRUE;
			break;

		case USER_CONFIRM_NO_OR_CANCEL:
			tr->terminal_rsp_data.get_input.result_type = RESULT_BACKWARD_MOVE_BY_USER;
			break;

		case USER_CONFIRM_TIMEOUT:
			tr->terminal_rsp_data.get_input.result_type = RESULT_NO_RESPONSE_FROM_USER;
			break;

		case USER_CONFIRM_END:
			tr->terminal_rsp_data.get_input.result_type = RESULT_PROACTIVE_SESSION_TERMINATED_BY_USER;
			break;
		default:
			break;
	}

	result = TRUE;
	rv = sat_manager_send_terminal_response(ctx->comm, plg, tr);
	if(rv != TCORE_RETURN_SUCCESS){
		dbg("fail to send terminal response");
		result = FALSE;
	}

	g_free(tr);

	return result;
}

static gboolean _sat_manager_handle_setup_call_confirm(struct custom_data *ctx, TcorePlugin *plg, gint command_id, gint confirm_type, GVariant *addtional_data)
{
	TReturn rv = TCORE_RETURN_FAILURE;
	gboolean result = FALSE;

	gint input_data_len = 0;
	gchar input_data[SAT_TEXT_STRING_LEN_MAX];
	struct treq_sat_terminal_rsp_data *tr;
	struct sat_manager_queue_data q_data;

	memset(&q_data, 0, sizeof(struct sat_manager_queue_data));
	memset(input_data, 0, SAT_TEXT_STRING_LEN_MAX);

	if (sat_manager_dequeue_cmd_by_id(ctx, &q_data, command_id) == FALSE) {
		dbg("[SAT] command dequeue failed. didn't find in command Q!!");
		return result;
	}

	if (!plg){
		dbg("there is no valid plugin at this point");
		return result;
	}

	if(addtional_data){
		int index = 0;
		guchar data;
		GVariantIter *iter = NULL;
		GVariant *inner_gv = NULL;

		inner_gv = g_variant_get_variant(addtional_data);
		dbg("additional data exist type_format(%s)", g_variant_get_type_string(inner_gv));

		g_variant_get(inner_gv, "ay", &iter);
		while( g_variant_iter_loop (iter, "y", &data)){
			dbg("index(%d) data(%d)", index, data);
			input_data[index] = data;
			index++;
		}
		g_variant_iter_free(iter);
		g_variant_unref(inner_gv);
		input_data_len = index;
	}

	tr = g_new0(struct treq_sat_terminal_rsp_data, 1);
	tr->cmd_number = q_data.cmd_data.setup_call.command_detail.cmd_num;
	tr->cmd_type = q_data.cmd_data.setup_call.command_detail.cmd_type;
	memcpy((void*)&tr->terminal_rsp_data.setup_call.command_detail, &q_data.cmd_data.setup_call.command_detail, sizeof(struct tel_sat_cmd_detail_info));
	tr->terminal_rsp_data.setup_call.device_id.src = DEVICE_ID_ME;
	tr->terminal_rsp_data.setup_call.device_id.dest = q_data.cmd_data.setup_call.device_id.src;

	switch(confirm_type){
		case USER_CONFIRM_YES:{
			char *path;
			TelephonySAT *sat;
			TelephonyObjectSkeleton *object;

			gchar *plg_name = NULL;
			GVariant *setup_call = NULL;

			gint command_id, call_type, confirmed_text_len, text_len, duration;
			gchar *confirmed_text, *text, *call_number;
			GVariant *icon_id;

			plg_name = (gchar *)tcore_server_get_cp_name_by_plugin(plg);
			if (plg_name) {
				path = g_strdup_printf("%s/%s", MY_DBUS_PATH, plg_name);
			}
			else {
				path = g_strdup_printf("%s", MY_DBUS_PATH);
			}
			dbg("path = [%s]", path);

			object = g_hash_table_lookup(ctx->objects, path);
			sat = telephony_object_peek_sat(TELEPHONY_OBJECT(object));

			setup_call = sat_manager_setup_call_noti(ctx, plg_name, &q_data.cmd_data.setup_call);

			dbg("setup call type_format(%s)", g_variant_get_type_string(setup_call));
			g_variant_get(setup_call, "(isisi@visi)", &command_id, &confirmed_text, &confirmed_text_len, &text, &text_len, &icon_id, &call_type, &call_number, &duration);

			telephony_sat_emit_setup_call(sat, command_id, confirmed_text, confirmed_text_len, text, text_len, call_type,
					call_number, duration);

			sat_ui_support_launch_call_application(q_data.cmd_data.setup_call.command_detail.cmd_type, setup_call);
			g_free(tr);
			return TRUE;
		}break;

		case USER_CONFIRM_NO_OR_CANCEL:{
			tr->terminal_rsp_data.setup_call.result_type = RESULT_USER_DID_NOT_ACCEPT_CALL_SETUP_REQ;
			tr->terminal_rsp_data.setup_call.me_problem_type = ME_PROBLEM_NO_SPECIFIC_CAUSE;
			tr->terminal_rsp_data.setup_call.cc_problem_type = CC_PROBLEM_NO_SPECIFIC_CAUSE;
		}break;

		case USER_CONFIRM_END:{
			tr->terminal_rsp_data.setup_call.result_type = RESULT_PROACTIVE_SESSION_TERMINATED_BY_USER;
		}break;

		case USER_CONFIRM_HELP_INFO:
		default:
			tr->terminal_rsp_data.setup_call.result_type = RESULT_NO_RESPONSE_FROM_USER;
		break;
	}

	result = TRUE;
	rv = sat_manager_send_terminal_response(ctx->comm, plg, tr);
	if(rv != TCORE_RETURN_SUCCESS){
		dbg("fail to send terminal response");
		result = FALSE;
	}
	g_free(tr);
	return result;
}

static gboolean _sat_manager_handle_send_dtmf_confirm(struct custom_data *ctx, TcorePlugin *plg, gint command_id, gint confirm_type, GVariant *addtional_data)
{
	TReturn rv = TCORE_RETURN_FAILURE;
	gboolean result = FALSE;

	struct treq_sat_terminal_rsp_data *tr;
	struct sat_manager_queue_data q_data;

	memset(&q_data, 0, sizeof(struct sat_manager_queue_data));

	if (sat_manager_dequeue_cmd_by_id(ctx, &q_data, command_id) == FALSE) {
		dbg("[SAT] command dequeue failed. didn't find in command Q!!");
		return result;
	}

	if (!plg){
		dbg("there is no valid plugin at this point");
		return result;
	}

	tr = g_new0(struct treq_sat_terminal_rsp_data, 1);
	tr->cmd_number = q_data.cmd_data.send_dtmf.command_detail.cmd_num;
	tr->cmd_type = q_data.cmd_data.send_dtmf.command_detail.cmd_type;
	memcpy((void*)&tr->terminal_rsp_data.send_dtmf.command_detail, &q_data.cmd_data.send_dtmf.command_detail, sizeof(struct tel_sat_cmd_detail_info));
	tr->terminal_rsp_data.send_dtmf.device_id.src = DEVICE_ID_ME;
	tr->terminal_rsp_data.send_dtmf.device_id.dest = q_data.cmd_data.send_dtmf.device_id.src;

	dbg("confirm_type[%d]", confirm_type);

	switch(confirm_type){
		case USER_CONFIRM_NO_OR_CANCEL:
		case USER_CONFIRM_END:
			tr->terminal_rsp_data.send_dtmf.result_type = RESULT_PROACTIVE_SESSION_TERMINATED_BY_USER;
			tr->terminal_rsp_data.send_dtmf.me_problem_type = ME_PROBLEM_NO_SPECIFIC_CAUSE;
			break;
		default:
			tr->terminal_rsp_data.send_dtmf.result_type = RESULT_NO_RESPONSE_FROM_USER;
			break;
	}

	result = TRUE;
	rv = sat_manager_send_terminal_response(ctx->comm, plg, tr);
	if(rv != TCORE_RETURN_SUCCESS){
		dbg("fail to send terminal response");
		result = FALSE;
	}
	g_free(tr);

	return result;
}

static gboolean _sat_manager_handle_launch_browser_confirm(struct custom_data *ctx, TcorePlugin *plg, gint command_id, gint confirm_type, GVariant *addtional_data)
{
	TReturn rv = TCORE_RETURN_FAILURE;
	gboolean result = FALSE;

	struct treq_sat_terminal_rsp_data *tr;
	struct sat_manager_queue_data q_data;

	memset(&q_data, 0, sizeof(struct sat_manager_queue_data));

	if (sat_manager_dequeue_cmd_by_id(ctx, &q_data, command_id) == FALSE) {
		dbg("[SAT] command dequeue failed. didn't find in command Q!!");
		return result;
	}

	if (!plg){
		dbg("there is no valid plugin at this point");
		return result;
	}

	tr = (struct treq_sat_terminal_rsp_data *)calloc(1, sizeof(struct treq_sat_terminal_rsp_data));
	tr->cmd_number = q_data.cmd_data.launch_browser.command_detail.cmd_num;
	tr->cmd_type = q_data.cmd_data.launch_browser.command_detail.cmd_type;
	memcpy((void*)&tr->terminal_rsp_data.launch_browser.command_detail, &q_data.cmd_data.launch_browser.command_detail, sizeof(struct tel_sat_cmd_detail_info));
	tr->terminal_rsp_data.launch_browser.device_id.src = q_data.cmd_data.launch_browser.device_id.dest;
	tr->terminal_rsp_data.launch_browser.device_id.dest = q_data.cmd_data.launch_browser.device_id.src;

	dbg("confirm_type[%d]", confirm_type);

	switch(confirm_type){
		case USER_CONFIRM_YES:{
			char *path;
			TelephonySAT *sat;
			TelephonyObjectSkeleton *object;

			gchar *plg_name = NULL;
			GVariant *launch_browser = NULL;

			gint command_id = 0;
			gint browser_launch_type = 0, browser_id = 0;
			gint url_len = 0, text_len = 0, gateway_proxy_len =0;
			gchar *url = NULL;
			gchar *text = NULL;
			gchar *gateway_proxy = NULL;
			GVariant *icon_id = NULL;

			plg_name = (gchar *)tcore_server_get_cp_name_by_plugin(plg);
			if (plg_name) {
				path = g_strdup_printf("%s/%s", MY_DBUS_PATH, plg_name);
			} else {
				path = g_strdup_printf("%s", MY_DBUS_PATH);
			}
			dbg("path = [%s]", path);

			object = g_hash_table_lookup(ctx->objects, path);
			sat = telephony_object_peek_sat(TELEPHONY_OBJECT(object));

			launch_browser = sat_manager_launch_browser_noti(ctx, plg_name, &q_data.cmd_data.launch_browser);

			dbg("launch_browser type_format(%s)", g_variant_get_type_string(launch_browser));
			g_variant_get(launch_browser, "(iiisisisi@v)", &command_id, &browser_launch_type, &browser_id, &url, &url_len, &gateway_proxy, &gateway_proxy_len, &text, &text_len, &icon_id);

			telephony_sat_emit_launch_browser(sat, command_id, browser_launch_type, browser_id, url, url_len, gateway_proxy, gateway_proxy_len, text, text_len);

			sat_ui_support_launch_browser_application(q_data.cmd_data.launch_browser.command_detail.cmd_type, launch_browser);

			g_free(path);
			g_free(tr);

			return TRUE;
		}break;

		case USER_CONFIRM_NO_OR_CANCEL:
		case USER_CONFIRM_END:
			tr->terminal_rsp_data.launch_browser.result_type = RESULT_BACKWARD_MOVE_BY_USER;
			tr->terminal_rsp_data.launch_browser.me_problem_type = ME_PROBLEM_NO_SPECIFIC_CAUSE;
			tr->terminal_rsp_data.launch_browser.browser_problem_type = BROWSER_PROBLEM_NO_SPECIFIC_CAUSE;
			break;

		default:
			tr->terminal_rsp_data.launch_browser.result_type = RESULT_NO_RESPONSE_FROM_USER;
			break;
	}

	result = TRUE;
	rv = sat_manager_send_terminal_response(ctx->comm, plg, tr);
	if(rv != TCORE_RETURN_SUCCESS){
		dbg("fail to send terminal response");
		result = FALSE;
	}
	g_free(tr);

	return result;
}

static gboolean _sat_manager_handle_open_channel_confirm(struct custom_data *ctx, TcorePlugin *plg, gint command_id, gint confirm_type, GVariant *addtional_data)
{
	TReturn rv = TCORE_RETURN_FAILURE;
	gboolean result = FALSE;

	struct treq_sat_terminal_rsp_data *tr;
	struct sat_manager_queue_data q_data;

	memset(&q_data, 0, sizeof(struct sat_manager_queue_data));

	if (sat_manager_dequeue_cmd_by_id(ctx, &q_data, command_id) == FALSE) {
		dbg("[SAT] command dequeue failed. didn't find in command Q!!");
		return result;
	}

	if (!plg){
		dbg("there is no valid plugin at this point");
		return result;
	}

	tr = (struct treq_sat_terminal_rsp_data *)calloc(1, sizeof(struct treq_sat_terminal_rsp_data));
	tr->cmd_number = q_data.cmd_data.open_channel.command_detail.cmd_num;
	tr->cmd_type = q_data.cmd_data.open_channel.command_detail.cmd_type;
	memcpy((void*)&tr->terminal_rsp_data.open_channel.command_detail, &q_data.cmd_data.open_channel.command_detail, sizeof(struct tel_sat_cmd_detail_info));
	tr->terminal_rsp_data.open_channel.device_id.src = q_data.cmd_data.send_dtmf.device_id.dest;
	tr->terminal_rsp_data.open_channel.device_id.dest = q_data.cmd_data.send_dtmf.device_id.src;

	dbg("confirm_type[%d]", confirm_type);

	switch(confirm_type){
		case USER_CONFIRM_YES:{
			char *path;
			TelephonyObjectSkeleton *object;

			gchar *plg_name = NULL;

			GVariant *open_channel = NULL;

			gint command_id, bearer_type, protocol_type, dest_addr_type;
			gboolean immediate_link, auto_reconnection, bg_mode;
			gint text_len, buffer_size, port_number;
			gchar *text, *dest_address;
			GVariant *icon_id;
			GVariant *bearer_param;
			GVariant *bearer_detail;

			//emit send_dtmf signal
			plg_name = (gchar *)tcore_server_get_cp_name_by_plugin(plg);
			if (plg_name) {
				path = g_strdup_printf("%s/%s", MY_DBUS_PATH, plg_name);
			} else {
				path = g_strdup_printf("%s", MY_DBUS_PATH);
			}
			dbg("path = [%s]", path);

			object = g_hash_table_lookup(ctx->objects, path);

			open_channel = sat_manager_open_channel_noti(ctx, plg_name, &q_data.cmd_data.open_channel);

			dbg("open channel type_format(%s)", g_variant_get_type_string(open_channel));
			g_variant_get(open_channel,"(isi@vbbbi@viiiis@v)", &command_id, &text, &text_len, &icon_id, &immediate_link, &auto_reconnection, &bg_mode,
					&bearer_type, &bearer_param, &buffer_size, &protocol_type, &port_number, &dest_addr_type, &dest_address, &bearer_detail);

			/*telephony_sat_emit_open_channel(sat, command_id, text, text_len, immediate_link, auto_reconnection, bg_mode,
					bearer_type, bearer_param, buffer_size, protocol_type, port_number, dest_addr_type, dest_address, bearer_detail);*/

			//BIP Manager
			{
				gboolean b_sig = FALSE;
				GDBusConnection *conn = NULL;
				const gchar *g_path = NULL;

				conn = g_dbus_object_manager_server_get_connection(ctx->manager);
				g_path = g_dbus_object_get_object_path(G_DBUS_OBJECT(object));

				b_sig = sat_ui_support_exec_bip(conn, g_path, SAT_PROATV_CMD_OPEN_CHANNEL, open_channel);
			}

			g_free(path);
			g_free(tr);

			return TRUE;
		}break;

		case USER_CONFIRM_NO_OR_CANCEL:
		case USER_CONFIRM_END:
			tr->terminal_rsp_data.open_channel.result_type = RESULT_USER_DID_NOT_ACCEPT_CALL_SETUP_REQ;
			break;
		default:
			tr->terminal_rsp_data.open_channel.result_type = RESULT_NO_RESPONSE_FROM_USER;
			break;
	}

	memcpy((void*)&tr->terminal_rsp_data.open_channel.bearer_desc, &q_data.cmd_data.open_channel.bearer_desc, sizeof(struct tel_sat_bearer_description));
	memcpy((void*)&tr->terminal_rsp_data.open_channel.buffer_size, &q_data.cmd_data.open_channel.buffer_size, sizeof(struct tel_sat_buffer_size));

	result = TRUE;
	rv = sat_manager_send_terminal_response(ctx->comm, plg, tr);
	if(rv != TCORE_RETURN_SUCCESS){
		dbg("fail to send terminal response");
		result = FALSE;
	}
	g_free(tr);

	return result;
}

gboolean sat_manager_handle_user_confirm(struct custom_data *ctx, TcorePlugin *plg, GVariant *user_confirm_data)
{
	gboolean rv = FALSE;
	gboolean result = FALSE;
	struct sat_manager_queue_data q_data;

	gint command_id, command_type, confirm_type;
	GVariant *additional_data = NULL;

	dbg("user_confirm_data type_format(%s)", g_variant_get_type_string(user_confirm_data));
	g_variant_get(user_confirm_data, "(iiv)", &command_id, &confirm_type, &additional_data);

	dbg("[SAT] user confirm data command id(%d), confirm_type(%d)", command_id, confirm_type);

	rv = sat_manager_queue_peek_data_by_id(ctx, &q_data, command_id);
	if(!rv){
		dbg("[SAT] no commands in queue");
		return result;
	}

	command_type = (gint)q_data.cmd_type;
	dbg("[SAT] command type(%d)", command_type);

	switch(command_type){
		case SAT_PROATV_CMD_SELECT_ITEM:
			result = _sat_manager_handle_menu_select_confirm(ctx, plg, command_id, confirm_type, additional_data);
			break;
		case SAT_PROATV_CMD_DISPLAY_TEXT:
			result = _sat_manager_handle_display_text_confirm(ctx, plg, command_id, confirm_type, additional_data);
			break;
		case SAT_PROATV_CMD_GET_INKEY:
			result = _sat_manager_handle_get_inkey_confirm(ctx, plg, command_id, confirm_type, additional_data);
			break;
		case SAT_PROATV_CMD_GET_INPUT:
			result = _sat_manager_handle_get_input_confirm(ctx, plg, command_id, confirm_type, additional_data);
			break;
		case SAT_PROATV_CMD_SETUP_CALL:
			result = _sat_manager_handle_setup_call_confirm(ctx, plg, command_id, confirm_type, additional_data);
			break;
		case SAT_PROATV_CMD_SEND_DTMF:
			result = _sat_manager_handle_send_dtmf_confirm(ctx, plg, command_id, confirm_type, additional_data);
			break;
		case SAT_PROATV_CMD_LAUNCH_BROWSER:
			result = _sat_manager_handle_launch_browser_confirm(ctx, plg, command_id, confirm_type, additional_data);
			break;
		case SAT_PROATV_CMD_OPEN_CHANNEL:
			result = _sat_manager_handle_open_channel_confirm(ctx, plg, command_id, confirm_type, additional_data);
			break;
		default:
			dbg("[SAT] cannot handle user confirm command(0x%x)", command_type);
			break;
	}

	return result;
}

static gboolean _sat_manager_handle_play_tone_ui_display_status(struct custom_data *ctx, TcorePlugin *plg,struct sat_manager_queue_data *q_data, gboolean display_status)
{
	char *path;
	TelephonySAT *sat;
	TelephonyObjectSkeleton *object;

	gchar *plg_name = NULL;
	GVariant *play_tone = NULL;


	gint command_id, tone_type, duration;
	gint text_len;
	gchar* text;
	GVariant *icon_id;

	if(!display_status){
		struct treq_sat_terminal_rsp_data *tr = NULL;
		dbg("[SAT] fail to show ui display for play tone");

		tr = (struct treq_sat_terminal_rsp_data *)calloc(1, sizeof(struct treq_sat_terminal_rsp_data));

		tr->cmd_number = q_data->cmd_data.play_tone.command_detail.cmd_num;
		tr->cmd_type = q_data->cmd_data.play_tone.command_detail.cmd_type;
		memcpy((void*)&tr->terminal_rsp_data.play_tone.command_detail, &q_data->cmd_data.play_tone.command_detail, sizeof(struct tel_sat_cmd_detail_info));

		tr->terminal_rsp_data.play_tone.device_id.src = DEVICE_ID_ME;
		tr->terminal_rsp_data.play_tone.device_id.dest = q_data->cmd_data.play_tone.device_id.src;
		tr->terminal_rsp_data.play_tone.result_type = RESULT_ME_UNABLE_TO_PROCESS_COMMAND;

		sat_manager_send_terminal_response(ctx->comm, plg, tr);
		g_free(tr);

		return TRUE;
	}

	//emit play tone signal
	plg_name = (gchar *)tcore_server_get_cp_name_by_plugin(plg);
	if (plg_name) {
		path = g_strdup_printf("%s/%s", MY_DBUS_PATH, plg_name);
	}
	else {
		path = g_strdup_printf("%s", MY_DBUS_PATH);
	}
	dbg("path = [%s]", path);

	object = g_hash_table_lookup(ctx->objects, path);
	sat = telephony_object_peek_sat(TELEPHONY_OBJECT(object));

	play_tone = sat_manager_play_tone_noti(ctx, plg_name, &q_data->cmd_data.play_tone);

	dbg("play tone type_format(%s)", g_variant_get_type_string(play_tone));
	g_variant_get(play_tone, "(isi@vii)", &command_id, &text, &text_len, &icon_id, &tone_type, &duration);

	telephony_sat_emit_play_tone(sat, command_id, text, text_len, tone_type, duration);
	g_free(path);

	return TRUE;
}

static gboolean _sat_manager_handle_send_sms_ui_display_status(struct custom_data *ctx, TcorePlugin *plg,struct sat_manager_queue_data *q_data, gboolean display_status)
{
	char *path;
	TelephonySAT *sat;
	TelephonyObjectSkeleton *object;

	gchar *plg_name = NULL;
	GVariant *send_sms = NULL;


	gint command_id, ton, npi, tpdu_type;
	gboolean b_packing_required;
	gint text_len, number_len, tpdu_data_len;
	gchar* text, *dialling_number;
	GVariant *tpdu_data, *icon_id;

	if(!display_status){
		struct treq_sat_terminal_rsp_data *tr = NULL;
		dbg("[SAT] fail to show ui display for send sms");

		tr = g_new0(struct treq_sat_terminal_rsp_data, 1);

		tr->cmd_number = q_data->cmd_data.sendSMSInd.command_detail.cmd_num;
		tr->cmd_type = q_data->cmd_data.sendSMSInd.command_detail.cmd_type;
		memcpy((void*)&tr->terminal_rsp_data.send_sms.command_detail, &q_data->cmd_data.sendSMSInd.command_detail, sizeof(struct tel_sat_cmd_detail_info));

		tr->terminal_rsp_data.send_sms.device_id.src = DEVICE_ID_ME;
		tr->terminal_rsp_data.send_sms.device_id.dest = q_data->cmd_data.sendSMSInd.device_id.src;
		tr->terminal_rsp_data.send_sms.result_type = RESULT_ME_UNABLE_TO_PROCESS_COMMAND;

		sat_manager_send_terminal_response(ctx->comm, plg, tr);
		g_free(tr);
		return TRUE;
	}

	//emit send sms signal
	plg_name = (gchar *)tcore_server_get_cp_name_by_plugin(plg);
	if (plg_name) {
		path = g_strdup_printf("%s/%s", MY_DBUS_PATH, plg_name);
	}
	else {
		path = g_strdup_printf("%s", MY_DBUS_PATH);
	}
	dbg("path = [%s]", path);

	object = g_hash_table_lookup(ctx->objects, path);
	sat = telephony_object_peek_sat(TELEPHONY_OBJECT(object));

	send_sms = sat_manager_send_sms_noti(ctx, plg_name, &q_data->cmd_data.sendSMSInd);

	dbg("send sms type_format(%s)", g_variant_get_type_string(send_sms));
	g_variant_get(send_sms, "(isi@vbiisii@vi)", &command_id, &text, &text_len, &icon_id, &b_packing_required, &ton, &npi,
			&dialling_number, &number_len, &tpdu_type, &tpdu_data, &tpdu_data_len);

	telephony_sat_emit_send_sms(sat, command_id, text, text_len, b_packing_required,
			ton, npi, dialling_number, number_len, tpdu_type, tpdu_data, tpdu_data_len);

	return TRUE;
}

static gboolean _sat_manager_handle_send_ss_ui_display_status(struct custom_data *ctx, TcorePlugin *plg,struct sat_manager_queue_data *q_data, gboolean display_status)
{
	char *path;
	TelephonySAT *sat;
	TelephonyObjectSkeleton *object;

	gchar *plg_name = NULL;
	GVariant *send_ss = NULL;


	gint command_id, ton, npi;
	gint text_len, ss_str_len;
	gchar* text, *ss_string;
	GVariant *icon_id;

	if(!display_status){
		struct treq_sat_terminal_rsp_data *tr = NULL;
		dbg("[SAT] fail to show ui display for send ss");

		tr = (struct treq_sat_terminal_rsp_data *)calloc(1, sizeof(struct treq_sat_terminal_rsp_data));

		tr->cmd_number = q_data->cmd_data.send_ss.command_detail.cmd_num;
		tr->cmd_type = q_data->cmd_data.send_ss.command_detail.cmd_type;
		memcpy((void*)&tr->terminal_rsp_data.send_ss.command_detail, &q_data->cmd_data.send_ss.command_detail, sizeof(struct tel_sat_cmd_detail_info));

		tr->terminal_rsp_data.send_ss.device_id.src = DEVICE_ID_ME;
		tr->terminal_rsp_data.send_ss.device_id.dest = q_data->cmd_data.send_ss.device_id.src;
		tr->terminal_rsp_data.send_ss.result_type = RESULT_ME_UNABLE_TO_PROCESS_COMMAND;

		sat_manager_send_terminal_response(ctx->comm, plg, tr);
		g_free(tr);

		return TRUE;
	}

	//emit send ss signal
	plg_name = (gchar *)tcore_server_get_cp_name_by_plugin(plg);
	if (plg_name) {
		path = g_strdup_printf("%s/%s", MY_DBUS_PATH, plg_name);
	}
	else {
		path = g_strdup_printf("%s", MY_DBUS_PATH);
	}
	dbg("path = [%s]", path);

	object = g_hash_table_lookup(ctx->objects, path);
	sat = telephony_object_peek_sat(TELEPHONY_OBJECT(object));

	send_ss = sat_manager_send_ss_noti(ctx, plg_name, &q_data->cmd_data.send_ss);

	dbg("send ss type_format(%s)", g_variant_get_type_string(send_ss));
	g_variant_get(send_ss, "(isi@viiis)", &command_id, &text, &text_len, &icon_id,
			&ton, &npi, &ss_str_len, &ss_string);

	telephony_sat_emit_send_ss(sat, command_id, text, text_len, ton, npi, ss_string);
	sat_ui_support_launch_ciss_application(SAT_PROATV_CMD_SEND_SS, send_ss);

	return TRUE;
}

static gboolean _sat_manager_handle_send_ussd_ui_display_status(struct custom_data *ctx, TcorePlugin *plg,struct sat_manager_queue_data *q_data, gboolean display_status)
{
	char *path;
	TelephonySAT *sat;
	TelephonyObjectSkeleton *object;

	gchar *plg_name = NULL;
	GVariant *send_ussd = NULL;

	gint command_id;
	gint text_len, ussd_str_len;
	gchar* text, *ussd_string;
	GVariant *icon_id;

	if(!display_status){
		struct treq_sat_terminal_rsp_data *tr = NULL;
		dbg("[SAT] fail to show ui display for send ussd");

		tr = (struct treq_sat_terminal_rsp_data *)calloc(1, sizeof(struct treq_sat_terminal_rsp_data));

		tr->cmd_number = q_data->cmd_data.send_ussd.command_detail.cmd_num;
		tr->cmd_type = q_data->cmd_data.send_ussd.command_detail.cmd_type;
		memcpy((void*)&tr->terminal_rsp_data.send_ussd.command_detail, &q_data->cmd_data.send_ussd.command_detail, sizeof(struct tel_sat_cmd_detail_info));

		tr->terminal_rsp_data.send_ussd.device_id.src = DEVICE_ID_ME;
		tr->terminal_rsp_data.send_ussd.device_id.dest = q_data->cmd_data.send_ussd.device_id.src;
		tr->terminal_rsp_data.send_ussd.result_type = RESULT_ME_UNABLE_TO_PROCESS_COMMAND;

		sat_manager_send_terminal_response(ctx->comm, plg, tr);
		g_free(tr);

		return TRUE;
	}

	/* emit send ussd signal */
	plg_name = (gchar *)tcore_server_get_cp_name_by_plugin(plg);
	if (plg_name) {
		path = g_strdup_printf("%s/%s", MY_DBUS_PATH, plg_name);
	}
	else {
		path = g_strdup_printf("%s", MY_DBUS_PATH);
	}
	dbg("path = [%s]", path);

	object = g_hash_table_lookup(ctx->objects, path);
	sat = telephony_object_peek_sat(TELEPHONY_OBJECT(object));

	send_ussd = sat_manager_send_ussd_noti(ctx, plg_name, &q_data->cmd_data.send_ussd);

	dbg("send ussd type_format(%s)", g_variant_get_type_string(send_ussd));
	g_variant_get(send_ussd, "(isi@vis)", &command_id, &text, &text_len, &icon_id, &ussd_str_len, &ussd_string);

	telephony_sat_emit_setup_ussd(sat, command_id, text, text_len, ussd_string);
	sat_ui_support_launch_ciss_application(SAT_PROATV_CMD_SEND_USSD, send_ussd);

	return TRUE;
}

static gboolean _sat_manager_handle_setup_idle_mode_text_ui_display_status(struct custom_data *ctx, TcorePlugin *plg,struct sat_manager_queue_data *q_data, gboolean display_status)
{
	TReturn rv = TCORE_RETURN_FAILURE;
	gboolean result = FALSE;

	struct treq_sat_terminal_rsp_data *tr;

	if (!plg){
		dbg("there is no valid plugin at this point");
		return result;
	}

	tr = g_new0(struct treq_sat_terminal_rsp_data, 1);
	tr->cmd_number = q_data->cmd_data.idle_mode.command_detail.cmd_num;
	tr->cmd_type = q_data->cmd_data.idle_mode.command_detail.cmd_type;
	memcpy((void*)&tr->terminal_rsp_data.setup_idle_mode_text.command_detail, &q_data->cmd_data.idle_mode.command_detail, sizeof(struct tel_sat_cmd_detail_info));
	tr->terminal_rsp_data.setup_idle_mode_text.device_id.src = q_data->cmd_data.idle_mode.device_id.dest;
	tr->terminal_rsp_data.setup_idle_mode_text.device_id.dest = q_data->cmd_data.idle_mode.device_id.src;

	tr->terminal_rsp_data.setup_idle_mode_text.result_type = RESULT_SUCCESS;
		if (q_data->cmd_data.idle_mode.icon_id.icon_info.ics == IMAGE_CODING_SCHEME_COLOUR)
				tr->terminal_rsp_data.setup_idle_mode_text.result_type = RESULT_SUCCESS_BUT_REQUESTED_ICON_NOT_DISPLAYED;

	//fail to display text
	if(!display_status){
		dbg("[SAT] fail to show ui display for setup_idle_mode_text");
		tr->terminal_rsp_data.setup_idle_mode_text.result_type = RESULT_ME_UNABLE_TO_PROCESS_COMMAND;
	}

	result = TRUE;
	rv = sat_manager_send_terminal_response(ctx->comm, plg, tr);
	if(rv != TCORE_RETURN_SUCCESS){
		dbg("fail to send terminal response");
		result = FALSE;
	}
	g_free(tr);

	sat_ui_support_terminate_sat_ui();

	return result;
}

static gboolean _sat_manager_handle_refresh_ui_display_status(struct custom_data *ctx, TcorePlugin *plg,struct sat_manager_queue_data *q_data, gboolean display_status)
{
	struct treq_sat_terminal_rsp_data tr;

	memset(&tr, 0, sizeof(struct treq_sat_terminal_rsp_data));

	tr.cmd_number = q_data->cmd_data.refresh.command_detail.cmd_num;
	tr.cmd_type = q_data->cmd_data.refresh.command_detail.cmd_type;
	memcpy((void*)&tr.terminal_rsp_data.refresh.command_detail, &q_data->cmd_data.refresh.command_detail, sizeof(struct tel_sat_cmd_detail_info));
	tr.terminal_rsp_data.refresh.device_id.src = q_data->cmd_data.refresh.device_id.dest;
	tr.terminal_rsp_data.refresh.device_id.dest = q_data->cmd_data.refresh.device_id.src;

	if(!display_status){
		dbg("fail to show ui for refresh");
		tr.terminal_rsp_data.refresh.result_type = RESULT_ME_UNABLE_TO_PROCESS_COMMAND;
	}
	else{
		dbg("success to show ui for refresh");
		tr.terminal_rsp_data.more_time.result_type = RESULT_SUCCESS;
		tr.terminal_rsp_data.more_time.me_problem_type = ME_PROBLEM_NO_SPECIFIC_CAUSE;
	}

	dbg("send refresh tr");
	sat_manager_send_terminal_response(ctx->comm, plg, &tr);
	return TRUE;
}

static gboolean _sat_manager_handle_send_dtmf_ui_display_status(struct custom_data *ctx, TcorePlugin *plg,struct sat_manager_queue_data *q_data, gboolean display_status)
{
	char *path;
	TelephonySAT *sat;
	TelephonyObjectSkeleton *object;

	gchar *plg_name = NULL;

	GVariant *send_dtmf = NULL;
	gint command_id = 0;
	gint text_len = 0, dtmf_str_len = 0;
	gchar *text = NULL;
	gchar *dtmf_str = NULL;
	GVariant *icon_id = NULL;

	if(!display_status){
		struct treq_sat_terminal_rsp_data *tr = NULL;
		dbg("[SAT] fail to show ui display for send_dtmf");

		tr = (struct treq_sat_terminal_rsp_data *)calloc(1, sizeof(struct treq_sat_terminal_rsp_data));

		tr->cmd_number = q_data->cmd_data.send_dtmf.command_detail.cmd_num;
		tr->cmd_type = q_data->cmd_data.send_dtmf.command_detail.cmd_type;
		memcpy((void*)&tr->terminal_rsp_data.send_dtmf.command_detail, &q_data->cmd_data.send_dtmf.command_detail, sizeof(struct tel_sat_cmd_detail_info));

		tr->terminal_rsp_data.send_dtmf.device_id.src = DEVICE_ID_ME;
		tr->terminal_rsp_data.send_dtmf.device_id.dest = q_data->cmd_data.send_dtmf.device_id.src;
		tr->terminal_rsp_data.send_dtmf.result_type = RESULT_ME_UNABLE_TO_PROCESS_COMMAND;

		sat_manager_send_terminal_response(ctx->comm, plg, tr);
		g_free(tr);

		return TRUE;
	}

	//emit send_dtmf signal
	plg_name = (gchar *)tcore_server_get_cp_name_by_plugin(plg);
	if (plg_name) {
		path = g_strdup_printf("%s/%s", MY_DBUS_PATH, plg_name);
	} else {
		path = g_strdup_printf("%s", MY_DBUS_PATH);
	}
	dbg("path = [%s]", path);

	object = g_hash_table_lookup(ctx->objects, path);
	sat = telephony_object_peek_sat(TELEPHONY_OBJECT(object));

	send_dtmf = sat_manager_send_dtmf_noti(ctx, plg_name, &q_data->cmd_data.send_dtmf);

	dbg("send_dtmf type_format(%s)", g_variant_get_type_string(send_dtmf));
	g_variant_get(send_dtmf, "(isi@vis)", &command_id, &text, &text_len, &icon_id, &dtmf_str_len, &dtmf_str);

	telephony_sat_emit_send_dtmf(sat, command_id, text, text_len, dtmf_str, dtmf_str_len);

	return TRUE;
}

static gboolean _sat_manager_handle_open_channel_ui_display_status(struct custom_data *ctx, TcorePlugin *plg,struct sat_manager_queue_data *q_data, gboolean display_status)
{
	char *path;
	TelephonyObjectSkeleton *object;

	gchar *plg_name = NULL;

	GVariant *open_channel = NULL;

	gint command_id, bearer_type, protocol_type, dest_addr_type;
	gboolean immediate_link, auto_reconnection, bg_mode;
	gint text_len, buffer_size, port_number;
	gchar *text, *dest_address;
	GVariant *icon_id;
	GVariant *bearer_param;
	GVariant *bearer_detail;

	if(!display_status){
		struct treq_sat_terminal_rsp_data *tr = NULL;
		dbg("[SAT] fail to show ui display for open channel");

		tr = (struct treq_sat_terminal_rsp_data *)calloc(1, sizeof(struct treq_sat_terminal_rsp_data));

		tr->cmd_number = q_data->cmd_data.open_channel.command_detail.cmd_num;
		tr->cmd_type = q_data->cmd_data.open_channel.command_detail.cmd_type;
		memcpy((void*)&tr->terminal_rsp_data.open_channel.command_detail, &q_data->cmd_data.open_channel.command_detail, sizeof(struct tel_sat_cmd_detail_info));

		tr->terminal_rsp_data.open_channel.device_id.src = q_data->cmd_data.send_dtmf.device_id.dest;
		tr->terminal_rsp_data.open_channel.device_id.dest = q_data->cmd_data.send_dtmf.device_id.src;
		tr->terminal_rsp_data.open_channel.result_type = RESULT_ME_UNABLE_TO_PROCESS_COMMAND;

		sat_manager_send_terminal_response(ctx->comm, plg, tr);
		g_free(tr);

		return TRUE;
	}

	//emit send_dtmf signal
	plg_name = (gchar *)tcore_server_get_cp_name_by_plugin(plg);
	if (plg_name) {
		path = g_strdup_printf("%s/%s", MY_DBUS_PATH, plg_name);
	} else {
		path = g_strdup_printf("%s", MY_DBUS_PATH);
	}
	dbg("path = [%s]", path);

	object = g_hash_table_lookup(ctx->objects, path);

	open_channel = sat_manager_open_channel_noti(ctx, plg_name, &q_data->cmd_data.open_channel);

	dbg("open channel type_format(%s)", g_variant_get_type_string(open_channel));
	g_variant_get(open_channel,"(isi@vbbbi@viiiis@v)", &command_id, &text, &text_len, &icon_id, &immediate_link, &auto_reconnection, &bg_mode,
			&bearer_type, &bearer_param, &buffer_size, &protocol_type, &port_number, &dest_addr_type, &dest_address, &bearer_detail);

	/*telephony_sat_emit_open_channel(sat, command_id, text, text_len, immediate_link, auto_reconnection, bg_mode,
			bearer_type, bearer_param, buffer_size, protocol_type, port_number, dest_addr_type, dest_address, bearer_detail);*/

	//BIP Manager
	{
		gboolean b_sig = FALSE;
		GDBusConnection *conn = NULL;
		const gchar *g_path = NULL;

		conn = g_dbus_object_manager_server_get_connection(ctx->manager);
		g_path = g_dbus_object_get_object_path(G_DBUS_OBJECT(object));

		b_sig = sat_ui_support_exec_bip(conn, g_path, SAT_PROATV_CMD_OPEN_CHANNEL, open_channel);
	}

	return TRUE;
}

gboolean sat_manager_handle_ui_display_status(struct custom_data *ctx, TcorePlugin *plg, gint command_id, gboolean display_status)
{
	gboolean result = FALSE;
	struct sat_manager_queue_data q_data;

	dbg("[SAT] ui display status : command id(%d) display status(%d)", command_id, display_status);
	memset(&q_data, 0, sizeof(struct sat_manager_queue_data));

	if (sat_manager_dequeue_cmd_by_id(ctx, &q_data, command_id) == FALSE) {
		dbg("[SAT] command peek data from queue is failed. didn't find in command Q!!");
		return result;
	}

	if (!plg){
		dbg("there is no valid plugin at this point");
		return result;
	}

	switch(q_data.cmd_type){
		case SAT_PROATV_CMD_PLAY_TONE:
			result = _sat_manager_handle_play_tone_ui_display_status(ctx, plg, &q_data, display_status);
			break;
		case SAT_PROATV_CMD_SEND_SMS:
			result = _sat_manager_handle_send_sms_ui_display_status(ctx, plg, &q_data, display_status);
			break;
		case SAT_PROATV_CMD_SEND_SS:
			result = _sat_manager_handle_send_ss_ui_display_status(ctx, plg, &q_data, display_status);
			break;
		case SAT_PROATV_CMD_SEND_USSD:
			result = _sat_manager_handle_send_ussd_ui_display_status(ctx, plg, &q_data, display_status);
			break;
		case SAT_PROATV_CMD_SETUP_IDLE_MODE_TEXT:
			result = _sat_manager_handle_setup_idle_mode_text_ui_display_status(ctx, plg, &q_data, display_status);
			break;
		case SAT_PROATV_CMD_REFRESH:
			result = _sat_manager_handle_refresh_ui_display_status(ctx, plg, &q_data, display_status);
			break;
		case SAT_PROATV_CMD_SEND_DTMF:
			result = _sat_manager_handle_send_dtmf_ui_display_status(ctx, plg, &q_data, display_status);
			break;
		case SAT_PROATV_CMD_OPEN_CHANNEL:
			result = _sat_manager_handle_open_channel_ui_display_status(ctx, plg, &q_data, display_status);
			break;
		default:
			dbg("[SAT] cannot handle ui display status command(0x%x)", q_data.cmd_type);
			break;
	}

	return result;
}

gboolean sat_manager_handle_event_download_envelop(int event_type,  int src_dev, int dest_dev, struct tel_sat_envelop_event_download_tlv *evt_download, GVariant *download_data)
{
	GVariant *data = NULL;

	dbg("download data type_format(%s)", g_variant_get_type_string(download_data));
	g_variant_get(download_data, "v", &data);

	switch(event_type)
	{
		case EVENT_USER_ACTIVITY:
			dbg("data type_format(%s)", g_variant_get_type_string(data));
			evt_download->device_identitie.src = src_dev;
			evt_download->device_identitie.dest = dest_dev;
			break;
		case EVENT_IDLE_SCREEN_AVAILABLE:
			dbg("data type_format(%s)", g_variant_get_type_string(data));
			g_variant_get(data, "(b)", &evt_download->idle_screen);
			evt_download->device_identitie.src = DEVICE_ID_DISPLAY;
			evt_download->device_identitie.dest = dest_dev;
			dbg("idle screen available (%d)", evt_download->idle_screen);
			break;
		case EVENT_LANGUAGE_SELECTION:
			dbg("data type_format(%s)", g_variant_get_type_string(data));
			g_variant_get(data, "(i)", &evt_download->language);
			evt_download->device_identitie.src = src_dev;
			evt_download->device_identitie.dest = dest_dev;
			dbg("selected language (%d)", evt_download->language);
			break;
		case EVENT_BROWSER_TERMINATION:{
			dbg("data type_format(%s)", g_variant_get_type_string(data));
			g_variant_get(data, "(i)", &evt_download->browser_termination);
			evt_download->device_identitie.src = src_dev;
			evt_download->device_identitie.dest = dest_dev;
			dbg("browser termination cause(%d)", evt_download->browser_termination);
		} break;
		case EVENT_DATA_AVAILABLE:{
			gint channel_id, channel_status, channel_info, channel_data_len;

			dbg("data type_format(%s)", g_variant_get_type_string(data));
			g_variant_get(data, "(iiii)", &channel_id, &channel_status, &channel_info, &channel_data_len);
			evt_download->device_identitie.src = src_dev;
			evt_download->device_identitie.dest = dest_dev;
			evt_download->channel_status.channel_id = channel_id;
			evt_download->channel_status.status = channel_status;
			evt_download->channel_status.status_info = channel_info;
			evt_download->channel_data_len.data_len = channel_data_len;
			dbg("data available channel id (%d)", evt_download->channel_status.channel_id);
		} break;
		case EVENT_CHANNEL_STATUS:{
			gint channel_id, channel_status, channel_info;

			dbg("data type_format(%s)", g_variant_get_type_string(data));
			g_variant_get(data, "(iii)", &channel_id, &channel_status, &channel_info);
			evt_download->device_identitie.src = src_dev;
			evt_download->device_identitie.dest = dest_dev;
			evt_download->channel_status.channel_id = channel_id;
			evt_download->channel_status.status = channel_status;
			evt_download->channel_status.status_info = channel_info;

		} break;
		default :
			dbg("not support download event (%d)", event_type);
			break;
	}

	return TRUE;
}

gboolean sat_manager_update_language(struct custom_data *ctx, const char *plugin_name, GVariant *language_noti)
{
	Server *s = NULL;
	TcorePlugin *plg = NULL;
	gpointer handle = NULL;
	static Storage *strg;

	TReturn rv = TCORE_RETURN_FAILURE;
	gboolean result = FALSE;
	const gchar *lang_str = NULL;
	gint command_id, language;
	gboolean b_specified;

	struct treq_sat_terminal_rsp_data *tr;
	struct sat_manager_queue_data q_data;

	s = ctx->server;
	strg = tcore_server_find_storage(s, "vconf");
	handle = tcore_storage_create_handle(strg, "vconf");
	if (!handle){
		err("fail to create vconf handle");
		return result;
	}

	plg = tcore_server_find_plugin(ctx->server, plugin_name);
	if (!plg){
		dbg("there is no valid plugin at this point");
		return result;
	}

	memset(&q_data, 0, sizeof(struct sat_manager_queue_data));

	dbg("language_noti type_format(%s)", g_variant_get_type_string(language_noti));
	g_variant_get(language_noti, "(iib)", &command_id, &language, &b_specified);

	if (sat_manager_dequeue_cmd_by_id(ctx, &q_data, command_id) == FALSE) {
		dbg("[SAT] command dequeue failed. didn't find in command Q!!");
		return result;
	}

	if(q_data.cmd_type != SAT_PROATV_CMD_LANGUAGE_NOTIFICATION){
		dbg("[SAT] Language Noti dequeue failed. didn't find in command Q!!");
		return result;
	}

	tr = g_new0(struct treq_sat_terminal_rsp_data, 1);
	tr->cmd_number = q_data.cmd_data.language_notification.command_detail.cmd_num;
	tr->cmd_type = q_data.cmd_data.language_notification.command_detail.cmd_type;
	memcpy((void*)&tr->terminal_rsp_data.language_notification.command_detail, &q_data.cmd_data.language_notification.command_detail, sizeof(struct tel_sat_cmd_detail_info));
	tr->terminal_rsp_data.language_notification.device_id.src = DEVICE_ID_ME;
	tr->terminal_rsp_data.language_notification.device_id.dest = DEVICE_ID_SIM;
	tr->terminal_rsp_data.language_notification.result_type = RESULT_SUCCESS;

	lang_str = _convert_sim_lang_to_string((enum tel_sim_language_type)language);
	if(!lang_str){
		dbg("language is not exist");
		tr->terminal_rsp_data.language_notification.result_type = RESULT_ME_UNABLE_TO_PROCESS_COMMAND;
	}
	dbg("converted lang (%s)", lang_str);

	result = tcore_storage_set_string(strg,STORAGE_KEY_LANGUAGE_SET, (const char*)lang_str);
	if(!result){
		dbg("fail to update language");
		tr->terminal_rsp_data.language_notification.result_type = RESULT_ME_UNABLE_TO_PROCESS_COMMAND;
	}

	result = TRUE;
	rv = sat_manager_send_terminal_response(ctx->comm, plg, tr);
	if(rv != TCORE_RETURN_SUCCESS){
		dbg("fail to send terminal response");
		result = FALSE;
	}

	g_free(tr);
	return result;
}
