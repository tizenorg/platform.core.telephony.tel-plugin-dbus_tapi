#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <glib-object.h>
#include <gio/gio.h>

#include <aul.h>
#include <bundle.h>

#include "TelSat.h"
#include "sat_ui_support.h"

#define g_variant_iter_free0( iter ) \
	if ( iter ) \
		g_variant_iter_free( iter );\
	else \
		dbg("iter : 0");

static gboolean _sat_ui_support_processing_setup_menu_ind(GVariant *data)
{
	gint rv;
	bundle *bundle_data = 0;
	gchar *encoded_data = NULL, *cmd_type = NULL;
	TelSatSetupMenuInfo_t setup_menu;

	gchar *title;
	gint command_id, item_cnt;
	gboolean b_present, b_helpinfo, b_updated;
	GVariant *items = NULL, *icon_id, *icon_list;

	memset(&setup_menu, 0, sizeof(TelSatSetupMenuInfo_t));

	g_variant_get(data, "(ibs@vibb@v@v)", &command_id, &b_present, &title, &items, &item_cnt,
				&b_helpinfo, &b_updated, &icon_id, &icon_list);

	setup_menu.commandId = command_id;
	setup_menu.bIsMainMenuPresent = (b_present ? 1 : 0);
	memcpy(setup_menu.satMainTitle, title, TAPI_SAT_DEF_TITLE_LEN_MAX+1);
	setup_menu.satMainMenuNum = item_cnt;
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
			setup_menu.satMainMenuItem[index].itemId = item_id;
			memcpy(setup_menu.satMainMenuItem[index].itemString, item_str, TAPI_SAT_DEF_ITEM_STR_LEN_MAX + 6);
			index++;
		}
		g_variant_iter_free0(iter);
	}
	setup_menu.bIsSatMainMenuHelpInfo = (b_helpinfo ? 1 : 0);
	setup_menu.bIsUpdatedSatMainMenu = (b_updated ? 1 : 0);

	cmd_type = g_strdup_printf("%d", SAT_PROATV_CMD_SETUP_MENU);
	encoded_data = g_base64_encode((const guchar*)&setup_menu, sizeof(TelSatSetupMenuInfo_t));

	bundle_data = bundle_create();
	bundle_add(bundle_data, "KEY_EXEC_TYPE", "1");
	bundle_add(bundle_data, "cmd_type", cmd_type);
	bundle_add(bundle_data, "data", encoded_data);

	rv = aul_launch_app("com.samsung.sat-ui", bundle_data);
	dbg("setup menu ind (%d)", rv);

	bundle_free(bundle_data);
	g_free(encoded_data);
	g_free(cmd_type);

	return TRUE;
}

static gboolean _sat_ui_support_processing_display_text_ind(GVariant *data)
{
	gint rv;
	bundle *bundle_data = 0;
	gchar *encoded_data = NULL, *cmd_type = NULL;
	TelSatDisplayTextInd_t display_text;

	gchar* text;
	gint command_id, text_len, duration;
	gboolean high_priority, user_rsp_required, immediately_rsp;
	GVariant *icon_id = NULL;

	memset(&display_text, 0, sizeof(TelSatDisplayTextInd_t));

	g_variant_get(data, "(isiibbb@v)", &command_id, &text, &text_len, &duration,
		&high_priority, &user_rsp_required, &immediately_rsp, &icon_id);

	display_text.commandId = command_id;
	memcpy(display_text.text.string, text, TAPI_SAT_DEF_TEXT_STRING_LEN_MAX+1);
	display_text.text.stringLen = text_len;
	display_text.duration = duration;
	display_text.bIsPriorityHigh = (high_priority ? 1 : 0);
	display_text.bIsUserRespRequired = (user_rsp_required ? 1 : 0);
	dbg("duration(%d) user_rsp(%d)", duration, user_rsp_required);

	cmd_type = g_strdup_printf("%d", SAT_PROATV_CMD_DISPLAY_TEXT);
	encoded_data = g_base64_encode((const guchar*)&display_text, sizeof(TelSatDisplayTextInd_t));

	bundle_data = bundle_create();
	bundle_add(bundle_data, "KEY_EXEC_TYPE", "1");
	bundle_add(bundle_data, "cmd_type", cmd_type);
	bundle_add(bundle_data, "data", encoded_data);

	rv = aul_launch_app("com.samsung.sat-ui", bundle_data);
	dbg("display text ind (%d)", rv);

	bundle_free(bundle_data);
	g_free(encoded_data);
	g_free(cmd_type);

	return TRUE;
}

static gboolean _sat_ui_support_processing_select_item_ind(GVariant *data)
{
	gint rv;
	bundle *bundle_data = 0;
	gchar *encoded_data = NULL, *cmd_type = NULL;
	TelSatSelectItemInd_t select_item;

	gboolean help_info ;
	gchar *selected_text;
	gint command_id, default_item_id, menu_cnt, text_len =0;
	GVariant *menu_items, *icon_id, *icon_list;

	memset(&select_item, 0, sizeof(TelSatSelectItemInd_t));

	g_variant_get(data, "(ibsiii@v@v@v)", &command_id, &help_info, &selected_text,
		&text_len, &default_item_id, &menu_cnt, &menu_items, &icon_id, &icon_list);

	select_item.commandId = command_id;
	select_item.bIsHelpInfoAvailable = (help_info ? 1 : 0);
	memcpy(select_item.text.string, selected_text, TAPI_SAT_DEF_TITLE_LEN_MAX+1);
	select_item.text.stringLen = text_len;
	select_item.defaultItemIndex = default_item_id;
	select_item.menuItemCount = menu_cnt;
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
			select_item.menuItem[index].itemId = item_id;
			select_item.menuItem[index].textLen = item_len;
			memcpy(select_item.menuItem[index].text, item_str, TAPI_SAT_ITEM_TEXT_LEN_MAX + 1);
			index++;
		}
		g_variant_iter_free0(iter);
	}

	cmd_type = g_strdup_printf("%d", SAT_PROATV_CMD_SELECT_ITEM);
	encoded_data = g_base64_encode((const guchar*)&select_item, sizeof(TelSatSelectItemInd_t));

	bundle_data = bundle_create();
	bundle_add(bundle_data, "KEY_EXEC_TYPE", "1");
	bundle_add(bundle_data, "cmd_type", cmd_type);
	bundle_add(bundle_data, "data", encoded_data);

	rv = aul_launch_app("com.samsung.sat-ui", bundle_data);
	dbg("select item aul (%d)", rv);

	bundle_free(bundle_data);
	g_free(encoded_data);
	g_free(cmd_type);

	return TRUE;
}

static gboolean _sat_ui_support_processing_get_inkey_ind(GVariant *data)
{
	gint rv;
	bundle *bundle_data = 0;
	gchar *encoded_data = NULL, *cmd_type = NULL;
	TelSatGetInkeyInd_t get_inkey;

	gint command_id, key_type, input_character_mode;
	gint text_len, duration;
	gboolean b_numeric, b_help_info;
	gchar *text;
	GVariant *icon_id;

	memset(&get_inkey, 0, sizeof(TelSatGetInkeyInd_t));

	g_variant_get(data, "(iiibbsii@v)", &command_id, &key_type, &input_character_mode,
		&b_numeric,&b_help_info, &text, &text_len, &duration, &icon_id);

	get_inkey.commandId = command_id;
	get_inkey.keyType = key_type;
	get_inkey.inputCharMode = input_character_mode;
	get_inkey.bIsNumeric = (b_numeric ? 1 : 0);
	get_inkey.bIsHelpInfoAvailable = (b_help_info ? 1 : 0);
	memcpy(get_inkey.text.string, text, TAPI_SAT_DEF_TEXT_STRING_LEN_MAX+1);
	get_inkey.text.stringLen = text_len;
	get_inkey.duration = duration;

	cmd_type = g_strdup_printf("%d", SAT_PROATV_CMD_GET_INKEY);
	encoded_data = g_base64_encode((const guchar*)&get_inkey, sizeof(TelSatGetInkeyInd_t));

	bundle_data = bundle_create();
	bundle_add(bundle_data, "KEY_EXEC_TYPE", "1");
	bundle_add(bundle_data, "cmd_type", cmd_type);
	bundle_add(bundle_data, "data", encoded_data);

	rv = aul_launch_app("com.samsung.sat-ui", bundle_data);
	dbg("get inkey aul (%d)", rv);

	bundle_free(bundle_data);
	g_free(encoded_data);
	g_free(cmd_type);

	return TRUE;
}

static gboolean _sat_ui_support_processing_get_input_ind(GVariant *data)
{
	gint rv;
	bundle *bundle_data = 0;
	gchar *encoded_data = NULL, *cmd_type = NULL;
	TelSatGetInputInd_t get_input;

	gint command_id, input_character_mode;
	gint text_len, def_text_len, rsp_len_min, rsp_len_max;
	gboolean b_numeric, b_help_info, b_echo_input;
	gchar *text, *def_text;
	GVariant *icon_id;

	memset(&get_input, 0, sizeof(TelSatGetInputInd_t));

	g_variant_get(data, "(iibbbsiiisi@v)", &command_id, &input_character_mode, &b_numeric, &b_help_info, &b_echo_input,
		&text, &text_len, &rsp_len_max, &rsp_len_min, &def_text, &def_text_len, &icon_id);

	get_input.commandId = command_id;
	get_input.inputCharMode = input_character_mode;
	get_input.bIsNumeric = (b_numeric ? 1 : 0);
	get_input.bIsHelpInfoAvailable = (b_help_info ? 1 : 0);
	get_input.bIsEchoInput = (b_echo_input ? 1 : 0);
	memcpy(get_input.text.string, text, TAPI_SAT_DEF_TEXT_STRING_LEN_MAX+1);
	get_input.text.stringLen = text_len;
	get_input.respLen.max = rsp_len_max;
	get_input.respLen.min = rsp_len_min;
	memcpy(get_input.defaultText.string, def_text, TAPI_SAT_DEF_TEXT_STRING_LEN_MAX+1);
	get_input.defaultText.stringLen = def_text_len;

	cmd_type = g_strdup_printf("%d", SAT_PROATV_CMD_GET_INPUT);
	encoded_data = g_base64_encode((const guchar*)&get_input, sizeof(TelSatGetInputInd_t));

	bundle_data = bundle_create();
	bundle_add(bundle_data, "KEY_EXEC_TYPE", "1");
	bundle_add(bundle_data, "cmd_type", cmd_type);
	bundle_add(bundle_data, "data", encoded_data);

	rv = aul_launch_app("com.samsung.sat-ui", bundle_data);
	dbg("get input aul (%d)", rv);

	bundle_free(bundle_data);
	g_free(encoded_data);
	g_free(cmd_type);

	return TRUE;
}

static gboolean _sat_ui_support_processing_ui_info_ind(GVariant *data)
{
	gint rv;
	gint cmd = 0;
	bundle *bundle_data = 0;
	gchar *encoded_data = NULL, *cmd_type = NULL;
	TelSatSendUiInfo_t ui_info;

	gint command_id, text_len;
	gboolean user_confirm;
	gchar *text;

	memset(&ui_info, 0, sizeof(TelSatSendUiInfo_t));

	g_variant_get(data, "(isib)", &command_id, &text, &text_len, &user_confirm);
	dbg("command_id(%d) data(%s) len(%d) user_confirm(%d)", command_id, text, text_len, user_confirm);

	ui_info.commandId = command_id;
	memcpy(ui_info.text.string, text, TAPI_SAT_DEF_TEXT_STRING_LEN_MAX+1);
	ui_info.text.stringLen = text_len;
	ui_info.user_confirm = (user_confirm ? 1 : 0);

	cmd_type = g_strdup_printf("%d", cmd);
	encoded_data = g_base64_encode((const guchar*)&ui_info, sizeof(TelSatSendUiInfo_t));


	bundle_data = bundle_create();
	bundle_add(bundle_data, "KEY_EXEC_TYPE", "1");
	bundle_add(bundle_data, "cmd_type", cmd_type);
	bundle_add(bundle_data, "data", encoded_data);

	rv = aul_launch_app("com.samsung.sat-ui", bundle_data);
	dbg("ui info aul (%d)", rv);

	bundle_free(bundle_data);
	g_free(encoded_data);
	g_free(cmd_type);

	return TRUE;
}

gboolean sat_ui_support_terminate_sat_ui()
{
	gint rv;
	bundle *bundle_data = 0;
	gchar *cmd_type = NULL;

	cmd_type = g_strdup_printf("%d", SAT_PROATV_CMD_TYPE_END_PROACTIVE_SESSION);

	bundle_data = bundle_create();
	bundle_add(bundle_data, "KEY_EXEC_TYPE", "1");
	bundle_add(bundle_data, "cmd_type", cmd_type);

	rv = aul_launch_app("com.samsung.sat-ui", bundle_data);
	dbg("session end aul (%d)", rv);

	bundle_free(bundle_data);
	g_free(cmd_type);

	return TRUE;
}

gboolean sat_ui_support_launch_sat_ui(enum tel_sat_proactive_cmd_type cmd_type, GVariant *data)
{
	gboolean result = FALSE;
	sat_ui_support_create_desktop_file("temp");

	switch(cmd_type){
		case SAT_PROATV_CMD_NONE:
			result = _sat_ui_support_processing_ui_info_ind(data);
		break;
		case SAT_PROATV_CMD_SETUP_MENU:
			result = _sat_ui_support_processing_setup_menu_ind(data);
		break;
		case SAT_PROATV_CMD_DISPLAY_TEXT:
			result = _sat_ui_support_processing_display_text_ind(data);
		break;
		case SAT_PROATV_CMD_SELECT_ITEM:
			result = _sat_ui_support_processing_select_item_ind(data);
		break;
		case SAT_PROATV_CMD_GET_INKEY:
			result = _sat_ui_support_processing_get_inkey_ind(data);
		break;
		case SAT_PROATV_CMD_GET_INPUT:
			result = _sat_ui_support_processing_get_input_ind(data);
		break;
		case SAT_PROATV_CMD_SETUP_EVENT_LIST:
		break;
		default:
			dbg("does not need to launch sat-ui");
		break;
	}

	return result;
}

gboolean sat_ui_support_launch_call_application(enum tel_sat_proactive_cmd_type cmd_type, GVariant *data)
{
	gint rv;
	char buffer[300];
	bundle *bundle_data = 0;

	dbg("launch call application by aul");

	switch(cmd_type){
		case SAT_PROATV_CMD_SETUP_CALL:{
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

		default:
			return FALSE;
		break;
	}

	rv = aul_launch_app("com.samsung.call",bundle_data);
	bundle_free(bundle_data);

	return TRUE;
}

gboolean sat_ui_support_launch_browser_application(enum tel_sat_proactive_cmd_type cmd_type, GVariant *data)
{
	gint rv;
	char buffer[300];
	bundle *bundle_data = 0;

	dbg("launch browser application by aul");

	/*TODO : need to make a sync with app engineer*/

	switch(cmd_type){
		case SAT_PROATV_CMD_LAUNCH_BROWSER:{
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

		default:
			return FALSE;
		break;
	}

	rv = aul_launch_app("com.samsung.call",bundle_data);
	bundle_free(bundle_data);

	return TRUE;
}

gboolean sat_ui_support_launch_setting_application(enum tel_sat_proactive_cmd_type cmd_type, GVariant *data)
{
	gint rv;
	char buffer[300];
	bundle *bundle_data = 0;

	dbg("launch setting application by aul");

	/*TODO : need to make a sync with app engineer*/

	switch(cmd_type){
		case SAT_PROATV_CMD_LANGUAGE_NOTIFICATION:{
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

		case SAT_PROATV_CMD_PROVIDE_LOCAL_INFO:
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
	int rv = 0;
	FILE *b_check, *f_out;

	if(!title){
		dbg("title does not exist");
		return FALSE;
	}

	b_check = fopen("/opt/share/applications/com.samsung.sat-ui.desktop", "r");
	if(b_check && !(g_strcmp0(title,"temp")) ){
		dbg("desktop file aleady exist");
		fclose(b_check);
		return TRUE;
	}

	if(b_check)
		fclose(b_check);

	f_out = fopen("/tmp/com.samsung.sat-ui.desktop", "w");
	if(!f_out){
		dbg("fail to create sat-ui desktop file");
		return FALSE;
	}

	fprintf(f_out, "Package=com.samsung.sat-ui\n");
	fprintf(f_out, "Name=%s\n",title);
	fprintf(f_out, "Type=Application\n");
	fprintf(f_out, "Version=0.2.2\n");
	fprintf(f_out, "Exec=/usr/apps/com.samsung.sat-ui/bin/sat-ui KEY_EXEC_TYPE 0\n");
	fprintf(f_out, "Icon=com.samsung.sat-ui.png\n");
	fprintf(f_out, "X-Tizen-TaskManage=True\n");
	fprintf(f_out, "X-Tizen-Multiple=False\n");
	fprintf(f_out, "X-Tizen-Removable=False\n");
	fprintf(f_out, "Comment=SIM Application UI\n");
	fclose(f_out);

	rv = system("/bin/cp /tmp/com.samsung.sat-ui.desktop /opt/share/applications/");
	dbg("the result to create desktop file (%d)", rv);
	rv = system("/bin/rm /tmp/com.samsung.sat-ui.desktop");

	if(rv == -1 || rv == 127)
	 return FALSE;

	return TRUE;
}

gboolean sat_ui_support_remove_desktop_file(void)
{
	int rv = 0;
	FILE *b_check;

	b_check = fopen("/opt/share/applications/com.samsung.sat-ui.desktop", "r");
	if(!b_check){
		dbg("desktop file does not exist");
		return TRUE;
	}

	rv = system("/bin/rm /opt/share/applications/com.samsung.sat-ui.desktop");
	dbg("the result to remove desktop file (%d)", rv);

	return TRUE;
}
