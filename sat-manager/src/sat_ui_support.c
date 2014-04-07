/*
 * tel-plugin-dbus_tapi
 *
 * Copyright (c) 2013 Samsung Electronics Co. Ltd. All rights reserved.
 * Copyright (c) 2013 Intel Corporation. All rights reserved.
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
#include <unistd.h>
#include <sys/types.h>
#include <sys/xattr.h>
#include <errno.h>
#include <aul.h>
#include <appsvc.h>
#include <bundle.h>

#include "sat_ui_support.h"

#define SAT_UI_APP "org.tizen.sat-ui"
#define DBUS_LAUNCH_SAT_APP(cmd_type, encoded_data) \
	do { \
		bundle *bundle_data = 0; \
		gint rv; \
		\
		bundle_data = bundle_create(); \
		bundle_add(bundle_data, "KEY_EXEC_TYPE", "1"); \
		bundle_add(bundle_data, "cmd_type", cmd_type); \
		bundle_add(bundle_data, "data", encoded_data); \
		\
		rv = aul_launch_app(SAT_UI_APP, bundle_data); \
		dbg("aul launch app rv: (%d)", rv); \
		bundle_free(bundle_data); \
	} while (0)

static TcoreStorage *strg_vconf = NULL;
#if 0
static gboolean _sat_ui_support_processing_setup_menu_ind(GVariant *data)
{
	gchar *encoded_data = NULL, *cmd_type = NULL;
	TelSatSetupMenuInfo setup_menu;
	gchar *title;
	gint command_id, item_cnt;
	gboolean is_present, is_helpinfo, is_updated;
	GVariant *items = NULL, *icon_id, *icon_list;

	memset(&setup_menu, 0, sizeof(TelSatSetupMenuInfo));

	g_variant_get(data, "(ibs@vibb@v@v)", &command_id, &is_present, &title, &items, &item_cnt,
				&is_helpinfo, &is_updated, &icon_id, &icon_list);

	setup_menu.command_id = command_id;
	setup_menu.is_main_menu_present = (is_present ? 1 : 0);
	memcpy(setup_menu.sat_main_title, title, TEL_SAT_DEF_TITLE_LEN_MAX+1);
	setup_menu.sat_main_menu_item_count = item_cnt;
	if(items && item_cnt > 0){
		int index = 0;
		GVariant *unbox;
		GVariantIter *iter;

		gchar *item_str;
		gint item_id;
		unbox = g_variant_get_variant(items);
		dbg("items(%p) items type_format(%s)", items, g_variant_get_type_string(unbox));

		g_variant_get(unbox, "a(si)", &iter);
		while(g_variant_iter_loop(iter,"(si)",&item_str, &item_id)){
			setup_menu.sat_main_menu_item[index].item_id = item_id;
			memcpy(setup_menu.sat_main_menu_item[index].item_string, item_str, TEL_SAT_DEF_ITEM_STR_LEN_MAX + 6);
			index++;
		}
		g_variant_iter_free(iter);
	}
	setup_menu.is_sat_main_menu_help_info = (is_helpinfo ? 1 : 0);
	setup_menu.is_updated_sat_main_menu = (is_updated ? 1 : 0);

	cmd_type = g_strdup_printf("%d", TEL_SAT_PROATV_CMD_SETUP_MENU);
	encoded_data = g_base64_encode((const guchar*)&setup_menu, sizeof(TelSatSetupMenuInfo));

	dbg("launching SAT APP for setup menu ind");
	DBUS_LAUNCH_SAT_APP(cmd_type, encoded_data);

	g_free(encoded_data);
	g_free(cmd_type);

	return TRUE;
}
#endif
static gboolean _sat_ui_support_processing_display_text_ind(GVariant *data)
{
	gchar *encoded_data = NULL, *cmd_type = NULL;
	TelSatDisplayTextInd display_text;
	gchar* text;
	gint command_id, text_len, duration;
	gboolean high_priority, user_rsp_required, immediately_rsp;
	GVariant *icon_id = NULL;

	memset(&display_text, 0, sizeof(TelSatDisplayTextInd));

	g_variant_get(data, "(isiibbb@v)", &command_id, &text, &text_len, &duration,
		&high_priority, &user_rsp_required, &immediately_rsp, &icon_id);

	display_text.command_id = command_id;
	memcpy(display_text.text.string, text, TEL_SAT_TEXT_STRING_LEN_MAX+1);
	display_text.text.string_len = text_len;
	display_text.duration = duration;
	display_text.is_priority_high = (high_priority ? 1 : 0);
	display_text.is_user_resp_required = (user_rsp_required ? 1 : 0);
	display_text.immediately_resp = (immediately_rsp ? 1 : 0);
	dbg("duration(%d) user_rsp(%d) immediately_rsp(%d)", duration, user_rsp_required, immediately_rsp);

	cmd_type = g_strdup_printf("%d", TEL_SAT_PROATV_CMD_DISPLAY_TEXT);
	encoded_data = g_base64_encode((const guchar*)&display_text, sizeof(TelSatDisplayTextInd));

	dbg("launching SAT APP for display text ind");
	DBUS_LAUNCH_SAT_APP(cmd_type, encoded_data);

	g_free(encoded_data);
	g_free(cmd_type);

	return TRUE;
}

static gboolean _sat_ui_support_processing_select_item_ind(GVariant *data)
{
	gchar *encoded_data = NULL, *cmd_type = NULL;
	TelSatSelectItemInd select_item;
	gboolean help_info ;
	gchar *selected_text;
	gint command_id, default_item_id, menu_cnt, text_len =0;
	GVariant *menu_items, *icon_id, *icon_list;

	memset(&select_item, 0, sizeof(TelSatSelectItemInd));

	g_variant_get(data, "(ibsiii@v@v@v)", &command_id, &help_info, &selected_text,
		&text_len, &default_item_id, &menu_cnt, &menu_items, &icon_id, &icon_list);

	select_item.command_id = command_id;
	select_item.is_help_info_available = (help_info ? 1 : 0);
	memcpy(select_item.text.string, selected_text, TEL_SAT_DEF_TITLE_LEN_MAX+1);
	select_item.text.string_len = text_len;
	select_item.default_item_index = default_item_id;
	select_item.menu_item_count = menu_cnt;
	if(menu_items && menu_cnt > 0){
		int index = 0;
		GVariant *unbox;
		GVariantIter *iter;
		gchar *item_str;
		gint item_id, item_len;
		unbox = g_variant_get_variant(menu_items);
		dbg("items(%p) items type_format(%s)", menu_items, g_variant_get_type_string(unbox));

		g_variant_get(unbox, "a(iis)", &iter);
		while(g_variant_iter_loop(iter,"(iis)",&item_id, &item_len, &item_str)){
			select_item.menu_item[index].item_id = item_id;
			select_item.menu_item[index].text_len = item_len;
			memcpy(select_item.menu_item[index].text, item_str, TEL_SAT_ITEM_TEXT_LEN_MAX + 1);
			index++;
		}
		g_variant_iter_free(iter);
	}

	cmd_type = g_strdup_printf("%d", TEL_SAT_PROATV_CMD_SELECT_ITEM);
	encoded_data = g_base64_encode((const guchar*)&select_item, sizeof(TelSatSelectItemInd));

	dbg("launching SAT APP for select item ind");
	DBUS_LAUNCH_SAT_APP(cmd_type, encoded_data);

	g_free(encoded_data);
	g_free(cmd_type);

	return TRUE;
}

static gboolean _sat_ui_support_processing_get_inkey_ind(GVariant *data)
{
	gchar *encoded_data = NULL, *cmd_type = NULL;
	TelSatGetInkeyInd get_inkey;
	gint command_id, key_type, input_character_mode;
	gint text_len, duration;
	gboolean is_numeric, is_help_info;
	gchar *text;
	GVariant *icon_id;

	memset(&get_inkey, 0, sizeof(TelSatGetInkeyInd));

	g_variant_get(data, "(iiibbsii@v)", &command_id, &key_type, &input_character_mode,
		&is_numeric,&is_help_info, &text, &text_len, &duration, &icon_id);

	get_inkey.command_id = command_id;
	get_inkey.key_type = key_type;
	get_inkey.input_char_mode = input_character_mode;
	get_inkey.is_numeric = (is_numeric ? 1 : 0);
	get_inkey.is_help_info_available = (is_help_info ? 1 : 0);
	memcpy(get_inkey.text.string, text, TEL_SAT_TEXT_STRING_LEN_MAX+1);
	get_inkey.text.string_len = text_len;
	get_inkey.duration = duration;

	cmd_type = g_strdup_printf("%d", TEL_SAT_PROATV_CMD_GET_INKEY);
	encoded_data = g_base64_encode((const guchar*)&get_inkey, sizeof(TelSatGetInkeyInd));

	dbg("launching SAT APP for get inkey ind");
	DBUS_LAUNCH_SAT_APP(cmd_type, encoded_data);

	g_free(encoded_data);
	g_free(cmd_type);

	return TRUE;
}

static gboolean _sat_ui_support_processing_get_input_ind(GVariant *data)
{
	gchar *encoded_data = NULL, *cmd_type = NULL;
	TelSatGetInputInd get_input;
	gint command_id, input_character_mode;
	gint text_len, def_text_len, rsp_len_min, rsp_len_max;
	gboolean is_numeric, is_help_info, is_echo_input;
	gchar *text, *def_text;
	GVariant *icon_id;

	memset(&get_input, 0, sizeof(TelSatGetInputInd));

	g_variant_get(data, "(iibbbsiiisi@v)", &command_id, &input_character_mode, &is_numeric, &is_help_info, &is_echo_input,
		&text, &text_len, &rsp_len_max, &rsp_len_min, &def_text, &def_text_len, &icon_id);

	get_input.command_id = command_id;
	get_input.input_char_mode = input_character_mode;
	get_input.is_numeric = (is_numeric ? 1 : 0);
	get_input.is_help_info_available = (is_help_info ? 1 : 0);
	get_input.is_echo_input = (is_echo_input ? 1 : 0);
	memcpy(get_input.text.string, text, TEL_SAT_TEXT_STRING_LEN_MAX+1);
	get_input.text.string_len = text_len;
	get_input.resp_len.max = rsp_len_max;
	get_input.resp_len.min = rsp_len_min;
	memcpy(get_input.default_text.string, def_text, TEL_SAT_TEXT_STRING_LEN_MAX+1);
	get_input.default_text.string_len = def_text_len;

	cmd_type = g_strdup_printf("%d", TEL_SAT_PROATV_CMD_GET_INPUT);
	encoded_data = g_base64_encode((const guchar*)&get_input, sizeof(TelSatGetInputInd));

	dbg("launching SAT APP for get input ind");
	DBUS_LAUNCH_SAT_APP(cmd_type, encoded_data);

	g_free(encoded_data);
	g_free(cmd_type);

	return TRUE;
}

static gboolean _sat_ui_support_processing_refresh_ind(GVariant *data)
{
	gchar *encoded_data = NULL, *cmd_type = NULL;
	TelSatRefreshIndUiInfo refresh_info;
	gint command_id = 0;
	gint refresh_type =0;
	GVariant *file_list = NULL;

	memset(&refresh_info, 0, sizeof(TelSatRefreshIndUiInfo));

	dbg("refresh type_format(%s)", g_variant_get_type_string(data));
	g_variant_get(data, "(ii@v)", &command_id, &refresh_type, &file_list);

	refresh_info.command_id = command_id;
	refresh_info.duration = 10000;
	refresh_info.refresh_type = refresh_type;

	cmd_type = g_strdup_printf("%d", TEL_SAT_PROATV_CMD_REFRESH);
	encoded_data = g_base64_encode((const guchar*)&refresh_info, sizeof(TelSatRefreshIndUiInfo));

	dbg("launching SAT APP for get input ind");
	DBUS_LAUNCH_SAT_APP(cmd_type, encoded_data);

	g_free(encoded_data);
	g_free(cmd_type);

	return TRUE;
}

static gboolean _sat_ui_support_processing_play_tone_ind(GVariant *data)
{
	gchar *encoded_data = NULL, *cmd_type = NULL;
	TelSatPlayToneInd play_tone_info;
	gint command_id, tone_type, duration;
	gint text_len;
	gchar* text;
	GVariant *icon_id;

	memset(&play_tone_info, 0, sizeof(TelSatPlayToneInd));

	g_variant_get(data, "(isi@vii)", &command_id, &text, &text_len, &icon_id, &tone_type, &duration);

	play_tone_info.command_id = command_id;
	play_tone_info.duration = duration;
	play_tone_info.text.string_len = text_len;
	memcpy(play_tone_info.text.string, text, TEL_SAT_TEXT_STRING_LEN_MAX+1);
	play_tone_info.tone = tone_type;

	cmd_type = g_strdup_printf("%d", TEL_SAT_PROATV_CMD_PLAY_TONE);
	encoded_data = g_base64_encode((const guchar*)&play_tone_info, sizeof(TelSatPlayToneInd));

	dbg("launching SAT APP for play tone ind");
	DBUS_LAUNCH_SAT_APP(cmd_type, encoded_data);

	g_free(encoded_data);
	g_free(cmd_type);

	return TRUE;
}

static gboolean _sat_ui_support_processing_idle_mode_text_ind(GVariant *data)
{
	gchar *encoded_data = NULL, *cmd_type = NULL;
	TelSatSetupIdleModeTextInd idle_mode_text_info;
	gint command_id, text_len;
	gchar* text;
	GVariant *icon_id;

	memset(&idle_mode_text_info, 0, sizeof(TelSatSetupIdleModeTextInd));

	g_variant_get(data, "(isi@v)", &command_id, &text, &text_len, &icon_id);

	idle_mode_text_info.command_id = command_id;
	idle_mode_text_info.text.string_len = text_len;
	memcpy(idle_mode_text_info.text.string, text, TEL_SAT_TEXT_STRING_LEN_MAX+1);

	cmd_type = g_strdup_printf("%d", TEL_SAT_PROATV_CMD_SETUP_IDLE_MODE_TEXT);
	encoded_data = g_base64_encode((const guchar*)&idle_mode_text_info, sizeof(TelSatSetupIdleModeTextInd));

	dbg("launching SAT APP for setup idle mode text ind");
	DBUS_LAUNCH_SAT_APP(cmd_type, encoded_data);

	g_free(encoded_data);
	g_free(cmd_type);

	return TRUE;
}

static gboolean _sat_ui_support_processing_ui_info_ind(TelSatProactiveCmdType cmd, GVariant *data)
{
	gchar *encoded_data = NULL, *cmd_type = NULL;
	TelSatSendUiInfo ui_info;
	gint command_id, text_len;
	gboolean user_confirm;
	gchar *text;

	memset(&ui_info, 0, sizeof(TelSatSendUiInfo));

	g_variant_get(data, "(isib)", &command_id, &text, &text_len, &user_confirm);
	dbg("command_id(%d) data(%s) len(%d) user_confirm(%d)", command_id, text, text_len, user_confirm);

	ui_info.command_id = command_id;
	memcpy(ui_info.text.string, text, TEL_SAT_TEXT_STRING_LEN_MAX+1);
	ui_info.text.string_len = text_len;
	ui_info.user_confirm = (user_confirm ? 1 : 0);

	cmd_type = g_strdup_printf("%d", cmd);
	encoded_data = g_base64_encode((const guchar*)&ui_info, sizeof(TelSatSendUiInfo));

	dbg("launching SAT APP for ui info ind");
	DBUS_LAUNCH_SAT_APP(cmd_type, encoded_data);

	g_free(encoded_data);
	g_free(cmd_type);

	return TRUE;
}

static gboolean _sat_ui_support_create_desktop_file(const gchar *title)
{
	int rv = 0;
	int b_check = 0;
	FILE *f_out;

	if(!title){
		dbg("title does not exist");
		return FALSE;
	}

	b_check = access("/opt/share/applications/org.tizen.sat-ui.desktop", F_OK);
	if( b_check == 0 && !(g_strcmp0(title,"temp")) ){
		dbg("desktop file aleady exist");
		return FALSE;
	}

	f_out = fopen("/opt/share/applications/org.tizen.sat-ui.desktop", "w");
	if(!f_out){
		dbg("fail to create sat-ui desktop file");
		return FALSE;
	}

	fprintf(f_out, "Package=org.tizen.sat-ui\n");
	fprintf(f_out, "Name=%s\n",title);
	fprintf(f_out, "Type=Application\n");
	fprintf(f_out, "Version=0.2.2\n");

	if( g_strcmp0(title,"temp") == 0 ){
		fprintf(f_out, "NoDisplay=true\n");
	}

	fprintf(f_out, "Exec=/usr/apps/org.tizen.sat-ui/bin/sat-ui KEY_EXEC_TYPE 0\n");
	fprintf(f_out, "Icon=org.tizen.sat-ui.png\n");
	fprintf(f_out, "X-Tizen-TaskManage=True\n");
	fprintf(f_out, "X-Tizen-Multiple=False\n");
	fprintf(f_out, "X-Tizen-Removable=False\n");
	fprintf(f_out, "Comment=SIM Application UI\n");
	fclose(f_out);
	//fflush(f_out);

	//rv = system("/bin/cp /tmp/org.tizen.sat-ui.desktop /opt/share/applications/");
	//dbg("the result to create desktop file (%d)", rv);
	//rv = system("/bin/rm /tmp/org.tizen.sat-ui.desktop");

	rv = setxattr("/opt/share/applications/org.tizen.sat-ui.desktop", "security.SMACK64","_",1,0);
	dbg("the result to apply the smack value(%d)", rv);

	b_check = access("/opt/share/applications/org.tizen.sat-ui.desktop", F_OK);
	dbg("access result (%d)", b_check);

	if(rv == -1 || rv == 127) {
		dbg("rv (%d)", rv);
		return FALSE;
	}

	//rv = system("/bin/ls -al /opt/share/applications > /opt/var/log/desktop_file_list.txt");

	return TRUE;
}

static void _sat_ui_support_storage_key_callback(TcoreStorageKey key, void *value, void *user_data)
{
	GVariant *tmp = NULL;
	gboolean type_check = FALSE;

	dbg("storage key(%d) callback", key);
	tmp = (GVariant *)value;
	if(!tmp || !strg_vconf){
		err("value is null");
		return;
	}

	if(key == STORAGE_KEY_IDLE_SCREEN_LAUNCHED) {
		int b_launched = 0;
		const gchar *title = (const gchar*)user_data;
		gboolean ret;

		type_check = g_variant_is_of_type(tmp, G_VARIANT_TYPE_INT32);
		if(!type_check){
			dbg("wrong variant data type");
			goto EXIT;
		}

		b_launched = g_variant_get_int32(tmp);
		if(b_launched < 0) {
			dbg("tcore_storage_get_int(VCONFKEY_IDLE_SCREEN_LAUNCHED) failed");
			goto EXIT;
		}

		if(b_launched) {
			dbg("idle screen is ready, create desktop file.");
			ret = _sat_ui_support_create_desktop_file(title);
		}
	}
	else {
		dbg("unspported key.");
	}

EXIT:
	g_variant_unref(tmp);
	return;
}

static gboolean _sat_ui_support_register_key_callback(TcoreStorageKey key, void * user_data)
{
	gboolean ret = FALSE;

	if (strg_vconf == NULL) {
		err("VCONF TcoreStorage is NULL!!!");
		return ret;
	}

	dbg("Set key callback - KEY: [%d]", key);
	ret = tcore_storage_set_key_callback(strg_vconf, key, _sat_ui_support_storage_key_callback, user_data);

	return ret;
}

gboolean sat_ui_support_terminate_sat_ui()
{
	gint rv;
	bundle *bundle_data = 0;
	gchar *cmd_type = NULL;

	cmd_type = g_strdup_printf("%d", TEL_SAT_PROATV_CMD_TYPE_END_PROACTIVE_SESSION);

	bundle_data = bundle_create();
	bundle_add(bundle_data, "KEY_EXEC_TYPE", "1");
	bundle_add(bundle_data, "cmd_type", cmd_type);

	rv = aul_launch_app(SAT_UI_APP, bundle_data);
	dbg("session end aul (%d)", rv);

	bundle_free(bundle_data);
	g_free(cmd_type);

	return TRUE;
}

gboolean sat_ui_support_launch_sat_ui(TelSatProactiveCmdType cmd_type, GVariant *data)
{
	gboolean result = FALSE;
	sat_ui_support_create_desktop_file("temp");

	switch(cmd_type){
		case TEL_SAT_PROATV_CMD_NONE:
		case TEL_SAT_PROATV_CMD_SEND_DTMF:
			result = _sat_ui_support_processing_ui_info_ind(cmd_type, data);
		break;
		/* No need to launch SAT UI in case of setup menu command */
		/*case TEL_SAT_PROATV_CMD_SETUP_MENU:
			result = _sat_ui_support_processing_setup_menu_ind(data);
		*/
		break;
		case TEL_SAT_PROATV_CMD_DISPLAY_TEXT:
			result = _sat_ui_support_processing_display_text_ind(data);
		break;
		case TEL_SAT_PROATV_CMD_SELECT_ITEM:
			result = _sat_ui_support_processing_select_item_ind(data);
		break;
		case TEL_SAT_PROATV_CMD_GET_INKEY:
			result = _sat_ui_support_processing_get_inkey_ind(data);
		break;
		case TEL_SAT_PROATV_CMD_GET_INPUT:
			result = _sat_ui_support_processing_get_input_ind(data);
		break;
		case TEL_SAT_PROATV_CMD_REFRESH:
			result = _sat_ui_support_processing_refresh_ind(data);
		break;
		case TEL_SAT_PROATV_CMD_PLAY_TONE:
			result = _sat_ui_support_processing_play_tone_ind(data);
		break;
		case TEL_SAT_PROATV_CMD_SETUP_IDLE_MODE_TEXT:
			result = _sat_ui_support_processing_idle_mode_text_ind(data);
		break;
		case TEL_SAT_PROATV_CMD_SETUP_EVENT_LIST:
		break;
		default:
			dbg("does not need to launch sat-ui");
		break;
	}

	return result;
}

gboolean sat_ui_support_launch_call_application(TelSatProactiveCmdType cmd_type, GVariant *data)
{
	//gint rv;
	char buffer[300];
	bundle *bundle_data = 0;

	dbg("launch call application by aul");
	bundle_data = bundle_create();

	appsvc_set_operation(bundle_data, APPSVC_OPERATION_CALL);
	appsvc_set_uri(bundle_data,"tel:MT");

	switch(cmd_type){
		case TEL_SAT_PROATV_CMD_SETUP_CALL:{
			gint command_id, call_type, confirm_text_len, text_len, duration;
			gchar *confirm_text, *text, *call_number;
			GVariant *icon_id;

			dbg("setup call type_format(%s)", g_variant_get_type_string(data));
			g_variant_get(data, "(isisi@visi)", &command_id, &confirm_text, &confirm_text_len, &text, &text_len, &icon_id, &call_type, &call_number, &duration);

			appsvc_add_data(bundle_data, "launch-type","SATSETUPCALL");

			snprintf(buffer, 300, "%d",command_id);
			appsvc_add_data(bundle_data, "cmd_id",buffer);
			dbg("cmd_id(%s)",buffer);

			snprintf(buffer, 300, "%d",call_type);
			appsvc_add_data(bundle_data, "cmd_qual", buffer);
			dbg("cmd_qual(%s)",buffer);

			snprintf(buffer, 300, "%s", text);
			appsvc_add_data(bundle_data, "disp_text", buffer);
			dbg("disp_text(%s)",buffer);

			snprintf(buffer, 300, "%s", call_number);
			appsvc_add_data(bundle_data, "call_num", buffer);
			dbg("call_num(%s)",buffer);

			snprintf(buffer, 300, "%d", duration);
			appsvc_add_data(bundle_data, "dur", buffer);
			dbg("dur(%s)",buffer);
		} break;

		default:
			bundle_free(bundle_data);
			return FALSE;
		break;
	}

	appsvc_run_service(bundle_data, 0, NULL, NULL);
	dbg("call app is called");
	//rv = aul_launch_app("com.samsung.call",bundle_data);
	//dbg("call app aul (%d)", rv);
	bundle_free(bundle_data);

	return TRUE;
}

gboolean sat_ui_support_launch_browser_application(TelSatProactiveCmdType cmd_type, GVariant *data)
{
//	gint rv;
	char buffer[300];
	bundle *bundle_data = 0;

	dbg("launch browser application by aul");
	bundle_data = bundle_create();

	appsvc_set_operation(bundle_data, APPSVC_OPERATION_VIEW);

	switch(cmd_type){
		case TEL_SAT_PROATV_CMD_LAUNCH_BROWSER:{
			gint command_id, launch_type, browser_id;
			gint url_len, text_len, gateway_proxy_len;
			gchar *url, *text, *gateway_proxy;
			GVariant *icon_id = NULL;

			dbg("launch_browser type_format(%s)", g_variant_get_type_string(data));
			g_variant_get(data, "(iiisisisi@v)", &command_id, &launch_type, &browser_id, &url, &url_len, &gateway_proxy, &gateway_proxy_len, &text, &text_len, &icon_id);

			appsvc_set_uri(bundle_data, url);

			snprintf(buffer, 300, "%d",command_id);
			appsvc_add_data(bundle_data, "cmd_id",buffer);
			dbg("cmd_id(%s)",buffer);

			snprintf(buffer, 300, "%d",launch_type);
			appsvc_add_data(bundle_data, "launch_type", buffer);
			dbg("launch_type(%s)",buffer);

			snprintf(buffer, 300, "%s", gateway_proxy);
			appsvc_add_data(bundle_data, "proxy", buffer);
			dbg("proxy(%s)",buffer);

		} break;

		default:
			bundle_free(bundle_data);
			return FALSE;
		break;
	}

	appsvc_run_service(bundle_data, 0, NULL, NULL);
	dbg("browser app is called");
	//rv = aul_launch_app("com.samsung.call",bundle_data);
	//dbg("call app aul (%d)", rv);
	bundle_free(bundle_data);

	return TRUE;
}

gboolean sat_ui_support_launch_ciss_application(TelSatProactiveCmdType cmd_type, GVariant *data)
{
	bundle *bundle_data = 0;
	gchar *encoded_data = NULL, *cmd = NULL;

	dbg("launch ciss application by aul");
	bundle_data = bundle_create();

	appsvc_set_operation(bundle_data, "http://tizen.org/appcontrol/operation/ciss");
	appsvc_set_pkgname(bundle_data, "com.samsung.ciss");

	switch(cmd_type){
		case TEL_SAT_PROATV_CMD_SEND_SS:{
			TelSatSendSsIndSsData ss_info;

			gint command_id, ton, npi;
			gint text_len, ss_str_len;
			gchar* text, *ss_string;
			GVariant *icon_id;

			dbg("launch ciss ui for send ss proactive cmd");

			memset(&ss_info, 0, sizeof(TelSatSendSsIndSsData));

			dbg("send ss type_format(%s)", g_variant_get_type_string(data));
			g_variant_get(data, "(isi@viiis)", &command_id, &text, &text_len, &icon_id, &ton, &npi, &ss_str_len, &ss_string);

			ss_info.command_id = command_id;
			memcpy(ss_info.ss_string, ss_string, TEL_SAT_DEF_SS_LEN_MAX+1);
			ss_info.ss_string_len = ss_str_len;

			cmd = g_strdup_printf("%d", cmd_type);
			encoded_data = g_base64_encode((const guchar*)&ss_info, sizeof(TelSatSendSsIndSsData));

		} break;

		case TEL_SAT_PROATV_CMD_SEND_USSD:{
			TelSatSendUssdIndUssdData ussd_info;

			gint command_id;
			gint text_len, ussd_str_len;
			gchar* text, *ussd_string;

			GVariant *icon_id;

			dbg("launch ciss ui for send ussd proactive cmd");

			memset(&ussd_info, 0, sizeof(TelSatSendUssdIndUssdData));

			dbg("send ussd type_format(%s)", g_variant_get_type_string(data));
			g_variant_get(data, "(isi@vis)", &command_id, &text, &text_len, &icon_id, &ussd_str_len, &ussd_string);

			ussd_info.command_id = command_id;
			memcpy(ussd_info.ussd_string, ussd_string, TEL_SAT_DEF_USSD_LEN_MAX+1);
			ussd_info.ussd_string_len = ussd_str_len;

			cmd = g_strdup_printf("%d", cmd_type);
			encoded_data = g_base64_encode((const guchar*)&ussd_info, sizeof(TelSatSendUssdIndUssdData));

		} break;

		default:
			bundle_free(bundle_data);
			return FALSE;
		break;
	}

	appsvc_add_data(bundle_data, "CISS_LAUNCHING_MODE", "RESP");
	appsvc_add_data(bundle_data, "KEY_EVENT_TYPE", cmd);
	appsvc_add_data(bundle_data, "KEY_ENCODED_DATA", encoded_data);

	appsvc_run_service(bundle_data, 0, NULL, NULL);
	dbg("ciss is called");
	bundle_free(bundle_data);

	return TRUE;
}

gboolean sat_ui_support_launch_setting_application(TelSatProactiveCmdType cmd_type, GVariant *data)
{
	gint rv;
	char buffer[300];
	bundle *bundle_data = 0;

	dbg("launch setting application by aul");

	/*TODO : need to make a sync with app engineer*/

	switch(cmd_type){
		case TEL_SAT_PROATV_CMD_LANGUAGE_NOTIFICATION:{
			gint command_id, call_type, text_len, duration;
			gchar *text, *call_number;
			GVariant *icon_id;

			dbg("setup call type_format(%s)", g_variant_get_type_string(data));
			g_variant_get(data, "(isi@visi)", &command_id, &text, &text_len, &icon_id, &call_type, &call_number, &duration);

			bundle_add(bundle_data, "launch-type","SATSETUPCALL");

			snprintf(buffer, 300, "%d",command_id);
			bundle_add(bundle_data, "cmd_id",buffer);
			dbg("cmd_id(%s)",buffer);

			snprintf(buffer, 300, "%d",call_type);
			bundle_add(bundle_data, "cmd_qual", buffer);
			dbg("cmd_qual(%s)",buffer);

			snprintf(buffer, 300, "%s", text);
			bundle_add(bundle_data, "disp_text", buffer);
			dbg("disp_text(%s)",buffer);

			snprintf(buffer, 300, "%s", call_number);
			bundle_add(bundle_data, "call_num", buffer);
			dbg("call_num(%s)",buffer);

			snprintf(buffer, 300, "%d", duration);
			bundle_add(bundle_data, "dur", buffer);
			dbg("dur(%s)",buffer);
		} break;

		case TEL_SAT_PROATV_CMD_PROVIDE_LOCAL_INFO:
			break;

		default:
			return FALSE;
		break;
	}

	rv = aul_launch_app("com.samsung.call",bundle_data);
	bundle_free(bundle_data);

	return TRUE;
}

gboolean sat_ui_support_create_desktop_file(const gchar *title)
{
	gboolean ret = FALSE;
	int is_launched = 0;

	dbg("check vconf if starter is ready.");

	if (strg_vconf == NULL) {
		err("VCONF TcoreStorage is NULL!!!");
		return ret;
	}

	is_launched = tcore_storage_get_int(strg_vconf, STORAGE_KEY_IDLE_SCREEN_LAUNCHED);
	if(is_launched < 0) {
		dbg("tcore_storage_get_int(VCONFKEY_IDLE_SCREEN_LAUNCHED) failed");
		return FALSE;
	}

	dbg("Launch: [%s]", (is_launched ? "YES" : "NO"));
	if(is_launched) {
		dbg("Idle screen is Ready!!! Create Desktop file");
		ret = _sat_ui_support_create_desktop_file(title);
		if (ret)
			return TRUE;
	}

	dbg("Idle screen is NOT launched yet!!! Register VCONF notification for Starter ready");
	ret = _sat_ui_support_register_key_callback(STORAGE_KEY_IDLE_SCREEN_LAUNCHED, (void *)title);

	return ret;
}

gboolean sat_ui_support_remove_desktop_file(void)
{
	int rv = 0;
	int is_check = 0;

	is_check = access("/opt/share/applications/org.tizen.sat-ui.desktop", F_OK);
	if(is_check != 0){
		dbg("desktop file does not exist");
		return TRUE;
	}

	rv = system("/bin/rm /opt/share/applications/org.tizen.sat-ui.desktop");
	dbg("the result to remove desktop file (%d)", rv);

	return TRUE;
}

gboolean sat_ui_support_exec_bip(GDBusConnection *connection, const gchar *path, TelSatProactiveCmdType cmd_type, GVariant *data)
{
	gboolean rv = FALSE;
	gchar *signal_name = NULL;
	GVariant *out_param = NULL;

	switch(cmd_type){
		case TEL_SAT_PROATV_CMD_OPEN_CHANNEL:{
			gint command_id, bearer_type, protocol_type, dest_addr_type;
			gboolean immediate_link, auto_reconnection, bg_mode;
			gint text_len, buffer_size, port_number;
			gchar *text, *dest_address;
			GVariant *icon_id;
			GVariant *bearer_param;
			GVariant *bearer_detail;

			dbg("open channel type_format(%s)", g_variant_get_type_string(data));
			g_variant_get(data,"(isi@vbbbiviiiisv)", &command_id, &text, &text_len, &icon_id, &immediate_link, &auto_reconnection, &bg_mode,
					&bearer_type, &bearer_param, &buffer_size, &protocol_type, &port_number, &dest_addr_type, &dest_address, &bearer_detail);

			out_param = g_variant_new("(isibbbiviiiisv)", command_id, text, text_len, immediate_link, auto_reconnection, bg_mode,
				bearer_type, bearer_param, buffer_size, protocol_type, port_number, dest_addr_type, dest_address, bearer_detail);
			signal_name = g_strdup("OpenChannel");
		} break;
		case TEL_SAT_PROATV_CMD_CLOSE_CHANNEL:{
			gint command_id, channel_id, text_len;
			gchar *text;
			GVariant *icon_id;

			dbg("close channel type_format(%s)", g_variant_get_type_string(data));
			g_variant_get(data, "(isi@vi)", &command_id, &text, &text_len, &icon_id, &channel_id);

			out_param = g_variant_new("(isii)", command_id, text, text_len, channel_id);
			signal_name = g_strdup("CloseChannel");
		} break;
		case TEL_SAT_PROATV_CMD_RECEIVE_DATA:{
			gint command_id, text_len, channel_id, channel_data_len = 0;
			gchar *text;
			GVariant *icon_id;

			dbg("receive data type_format(%s)", g_variant_get_type_string(data));
			g_variant_get(data, "(isi@vii)", &command_id, &text, &text_len, &icon_id, &channel_id, &channel_data_len);

			out_param = g_variant_new("(isiii)", command_id, text, text_len, channel_id, channel_data_len);
			signal_name = g_strdup("ReceiveData");
		} break;
		case TEL_SAT_PROATV_CMD_SEND_DATA:{
			gint command_id, channel_id, text_len, channel_data_len;
			gboolean send_data_immediately;
			gchar *text;
			GVariant *channel_data;
			GVariant *icon_id;

			dbg("send data type_format(%s)", g_variant_get_type_string(data));
			g_variant_get(data, "(isi@vibvi)", &command_id, &text, &text_len, &icon_id, &channel_id, &send_data_immediately, &channel_data, &channel_data_len);

			out_param = g_variant_new("(isiibvi)", command_id, text, text_len, channel_id, send_data_immediately, channel_data, channel_data_len);
			signal_name = g_strdup("SendData");
		} break;
		case TEL_SAT_PROATV_CMD_GET_CHANNEL_STATUS:{
			gint command_id;

			dbg("get channel status type_format(%s)", g_variant_get_type_string(data));
			g_variant_get(data, "(i)", &command_id);

			out_param = g_variant_new("(i)", command_id);
			signal_name = g_strdup("GetChannelStatus");
		} break;
		case TEL_SAT_PROATV_CMD_SETUP_EVENT_LIST:{
			gint event_cnt;
			GVariant *evt_list;

			dbg("setup event list type_format(%s)", g_variant_get_type_string(data));
			g_variant_get(data, "(iv)", &event_cnt, &evt_list);

			out_param = g_variant_new("(iv)", event_cnt, evt_list);
			signal_name = g_strdup("SetupEventList");
		} break;
		default:
			dbg("no matched command");
			return FALSE;
		break;
	}

	dbg("dbus conn(%p), path(%s)", connection, path);
	rv = g_dbus_connection_emit_signal(connection, "org.tizen.bip-manager", path, "org.tizen.telephony.SAT", signal_name, out_param, NULL);

	dbg("send signal to bip-mananger result (%d)", rv);

	return rv;
}

gboolean sat_ui_support_exec_evtdw(GDBusConnection *connection, const gchar *path, TelSatProactiveCmdType cmd_type, GVariant *data)
{
	gboolean rv = FALSE;
	gchar *signal_name = NULL;
	GVariant *out_param = NULL;

	switch(cmd_type){
		case TEL_SAT_PROATV_CMD_SETUP_EVENT_LIST:{
			gint event_cnt;
			GVariant *evt_list;

			dbg("setup event list type_format(%s)", g_variant_get_type_string(data));
			g_variant_get(data, "(iv)", &event_cnt, &evt_list);

			out_param = g_variant_new("(iv)", event_cnt, evt_list);
			signal_name = g_strdup("SetupEventList");
		} break;
		default:
			dbg("no matched command");
			return FALSE;
		break;
	}

	dbg("dbus conn(%p), path(%s)", connection, path);
	rv = g_dbus_connection_emit_signal(connection, "org.tizen.sat-event-downloader", path, "org.tizen.telephony.SAT", signal_name, out_param, NULL);

	dbg("send signal to sat-event-downloader result (%d)", rv);

	return rv;
}

gboolean sat_ui_support_storage_init(Server *server)
{
	TcoreStorage *strg = NULL;

	if(strg_vconf) {
		dbg("vconf storage already exists!!");
		return TRUE;
	}

	dbg("create vconf ");

	strg = tcore_server_find_storage(server, "vconf");
	if(!strg) {
		dbg("cannot find vconf storage!!");
		return FALSE;
	}

	strg_vconf = strg;
	return TRUE;
}

