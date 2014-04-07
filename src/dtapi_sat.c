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

#include "dtapi_sat.h"
//#include "dtapi_util.h"
#include "plugin.h"
#include "sat-manager/include/sat_manager.h"
#include "sat-manager/include/sat_ui_support.h"

#define AC_SAT	"telephony_framework::api_sat"

static void _util_sat_set_main_menu(DtapiSatPrivateData *sat_data,
	const char *cp_name, GVariant *main_menu)
{
	GSList *list = NULL;
	SatCachedData *object = NULL;

	for (list = sat_data->cached_data; list; list = list->next) {
		object = (SatCachedData *) list->data;
		if (object == NULL)
			continue;

		if (g_strcmp0(object->cp_name, cp_name) == 0 ) {
			/* need to free the previous main_menu */
			g_variant_unref(object->cached_sat_main_menu);
			object->cached_sat_main_menu = main_menu;
			return;
		}
	}

	/* If 'object' is NOT created, then create the object and add to the list */
	object = tcore_try_malloc0(sizeof(SatCachedData));
	if (NULL == object) {
		err(" Malloc Failed");
		return;
	}
	object->cp_name = g_strdup(cp_name);
	object->cached_sat_main_menu = main_menu;

	sat_data->cached_data =
		g_slist_append(sat_data->cached_data, (gpointer)object);
}

static GVariant *_util_sat_get_main_menu(DtapiSatPrivateData *sat_data,
	const char *cp_name)
{
	GSList *list = NULL;
	SatCachedData *object;

	dbg("Get Main menu");
	/*
	 * List of Objects in 'sat_data',
	 * compare cp_name with modem_name stored in 'sat_data'
	 * if matching return main_menu of that object.
	 */
	for (list = sat_data->cached_data; list; list = list->next) {
		object = (SatCachedData *)list->data;
		if (object == NULL)
			continue;

		if (g_strcmp0(object->cp_name, cp_name) == 0)
			return object->cached_sat_main_menu;
	}

	return NULL;
}

static gboolean dtapi_sat_get_main_menu_info(TelephonySat *sat,
	GDBusMethodInvocation *invocation, gpointer user_data)
{
	DtapiSatPluginsInfo *plugins_info = (DtapiSatPluginsInfo *)user_data;
	TcorePlugin *comm_plugin = plugins_info->comm_plugin;
	GVariant *main_menu = NULL;
	DtapiSatPrivateData *sat_data = NULL;
	gchar *title;
	gint command_id, item_cnt;
	gboolean is_present, is_help_info, is_updated;
	GVariant *items;

	dbg("Entry");

	if (dtapi_check_access_control(invocation, AC_SAT, "r") == FALSE)
		return TRUE;

	sat_data = (DtapiSatPrivateData *)tcore_plugin_ref_user_data(comm_plugin);
	if (!sat_data) {
		dtapi_return_error(invocation, G_DBUS_ERROR_FAILED, "Invalid parameter");
		return TRUE;
	}

	main_menu = _util_sat_get_main_menu(sat_data,
		dtapi_get_cp_name_by_object_path(
			g_dbus_method_invocation_get_object_path(invocation)));
	if (!main_menu) {
		dtapi_return_error(invocation, G_DBUS_ERROR_FAILED, "No main menu");
		return TRUE;
	}

	g_variant_get(main_menu, "(ibs@vibb)",
		&command_id, &is_present, &title, &items, &item_cnt,
		&is_help_info, &is_updated);

	dbg("Command ID: [%d] Menu: [%s] Title: [%s] count: [%d]",
		command_id, is_present ? "PRESENT" : "NOT PRESENT",
		title, item_cnt);

	telephony_sat_complete_get_main_menu_info(sat, invocation,
		TEL_SAT_RESULT_SUCCESS, command_id, is_present, title, items, item_cnt,
		is_help_info, is_updated);

	return TRUE;
}

static gboolean dtapi_sat_send_display_status(TelephonySat *sat,
	GDBusMethodInvocation *invocation, gint arg_command_id,
	gboolean arg_display_status, gpointer user_data)
{
	DtapiSatPluginsInfo *plugins_info = (DtapiSatPluginsInfo *)user_data;
	gboolean result = FALSE;
	gint out_param = 1;

	dbg("Entry");

	if (dtapi_check_access_control(invocation, AC_SAT, "x") == FALSE)
		return TRUE;

	result = sat_manager_handle_ui_display_status(plugins_info,
		arg_command_id, arg_display_status);
	if (!result) {
		err("fail to send display status");
	}

	out_param = (result ? 1 : 0);
	telephony_sat_complete_send_ui_display_status(sat, invocation, out_param);

	return TRUE;
}

static gboolean dtapi_sat_send_user_confirm(TelephonySat *sat,
	GDBusMethodInvocation *invocation, gint arg_command_id,
	gint arg_command_type, gint arg_user_confirm_type,
	GVariant *arg_additional_data, gpointer user_data)
{
	DtapiSatPluginsInfo *plugins_info = (DtapiSatPluginsInfo *)user_data;
	gboolean result = FALSE;
	gint out_param = 1;
	GVariant *confirm_data = NULL;

	dbg("Entry");

	if (dtapi_check_access_control(invocation, AC_SAT, "x") == FALSE)
		return TRUE;

	confirm_data = g_variant_new("(iiv)",
		arg_command_id, arg_user_confirm_type, arg_additional_data);

	result = sat_manager_handle_user_confirm(plugins_info, confirm_data);
	if (!result) {
		err("fail to send user confirm");
	}

	out_param = (result ? 1 : 0);
	telephony_sat_complete_send_user_confirm(sat, invocation, out_param);

	return TRUE;
}

static gboolean dtapi_sat_send_app_exec_result(TelephonySat *sat,
	GDBusMethodInvocation *invocation, gint arg_command_id,
	gint arg_command_type, GVariant *arg_exec_result,
	gpointer user_data)
{
	DtapiSatPluginsInfo *plugins_info = (DtapiSatPluginsInfo *)user_data;
	TcorePlugin *plugin = plugins_info->plugin;
	gboolean result = FALSE;
	gint out_param = 1;

	dbg("Entry");

	if (dtapi_check_access_control(invocation, AC_SAT, "x") == FALSE)
		return TRUE;

	dbg("processing app exec result");
	result = sat_manager_handle_app_exec_result(plugin,
		arg_command_id, arg_command_type, arg_exec_result);
	if (!result) {
		err("fail to send exec result");
	}

	out_param = (result ? 1 : 0);
	telephony_sat_complete_send_app_exec_result(sat, invocation, out_param);

	return TRUE;
}

static void on_response_dtapi_sat_select_menu(gint result,
	const void *data, void *cb_data)
{
	DbusRespCbData *rsp_cb_data = (DbusRespCbData *)cb_data;
	TelSatEnvelopeResp *envelop_resp = (TelSatEnvelopeResp *)data;

	tcore_check_return_assert(NULL != rsp_cb_data);

	dbg("Select Menu Envelope Response: Result:[%d] Env Resp:[%d]", result, *envelop_resp);
	telephony_sat_complete_select_menu(rsp_cb_data->interface_object,
		rsp_cb_data->invocation, result, (gint)*envelop_resp);
	tcore_free(rsp_cb_data);
}

static gboolean dtapi_sat_select_menu(TelephonySat *sat,
	GDBusMethodInvocation *invocation, guchar arg_item_identifier,
	gboolean arg_help_request, gpointer user_data)
{
	DtapiSatPluginsInfo *plugins_info = (DtapiSatPluginsInfo *)user_data;
	TcorePlugin *plugin = plugins_info->plugin;
	DbusRespCbData *rsp_cb_data = NULL;
	TelSatRequestEnvelopCmdData envelop_req;
	TelReturn result;

	dbg("Entry");

	if (dtapi_check_access_control(invocation, AC_SAT, "x") == FALSE)
		return TRUE;

	memset(&envelop_req, 0, sizeof(TelSatRequestEnvelopCmdData));

	envelop_req.sub_cmd = TEL_SAT_ENVELOP_MENU_SELECTION;
	envelop_req.envelop_data.menu_select.device_identitie.src = TEL_SAT_DEVICE_ID_KEYPAD;
	envelop_req.envelop_data.menu_select.device_identitie.dest = TEL_SAT_DEVICE_ID_SIM;
	envelop_req.envelop_data.menu_select.item_identifier = arg_item_identifier;
	envelop_req.envelop_data.menu_select.help_request = arg_help_request;

	rsp_cb_data = dtapi_create_resp_cb_data(sat, invocation, NULL, 0);

	result = tcore_plugin_dispatch_request(plugin, TRUE,
		TCORE_COMMAND_SAT_REQ_ENVELOPE,
		&envelop_req, sizeof(TelSatRequestEnvelopCmdData),
		on_response_dtapi_sat_select_menu, rsp_cb_data);
	if (result != TEL_RETURN_SUCCESS) {
		dtapi_return_error(invocation, G_DBUS_ERROR_FAILED, "Request dispatch failed");
		tcore_free(rsp_cb_data);
	}

	return TRUE;
}

static void on_response_dtapi_sat_download_event(gint result,
	const void *data, void *cb_data)
{
	DbusRespCbData *rsp_cb_data = cb_data;
	TelSatEnvelopeResp *envelop_resp = (TelSatEnvelopeResp *)data;

	tcore_check_return_assert(NULL != rsp_cb_data);

	dbg("Event Download Envelope Response: Result:[%d] Env Resp:[%d]", result, *envelop_resp);
	telephony_sat_complete_download_event(rsp_cb_data->interface_object,
		rsp_cb_data->invocation, result, (gint)*envelop_resp);
	tcore_free(rsp_cb_data);
}

static gboolean dtapi_sat_download_event(TelephonySat *sat,
	GDBusMethodInvocation *invocation, gint arg_event_download_type,
	gint arg_src_device,gint arg_dest_device,
	GVariant *arg_download_data, gpointer user_data)
{
	DtapiSatPluginsInfo *plugins_info = (DtapiSatPluginsInfo *)user_data;
	TcorePlugin *plugin = plugins_info->plugin;
	DbusRespCbData *rsp_cb_data = NULL;
	TelSatRequestEnvelopCmdData envelop_req;
	TelReturn result;

	dbg("Entry");

	if (dtapi_check_access_control(invocation, AC_SAT, "x") == FALSE)
		return TRUE;

	memset(&envelop_req, 0, sizeof(TelSatRequestEnvelopCmdData));

	envelop_req.sub_cmd = TEL_SAT_ENVELOP_EVENT_DOWNLOAD;
	envelop_req.envelop_data.event_download.event = arg_event_download_type;

	sat_manager_handle_event_download_envelop(arg_event_download_type,
		arg_src_device, arg_dest_device, &envelop_req.envelop_data.event_download,
		arg_download_data);

	rsp_cb_data = dtapi_create_resp_cb_data(sat, invocation, NULL, 0);

	result = tcore_plugin_dispatch_request(plugin, TRUE,
		TCORE_COMMAND_SAT_REQ_ENVELOPE,
		&envelop_req, sizeof(TelSatRequestEnvelopCmdData),
		on_response_dtapi_sat_download_event, rsp_cb_data);
	if (result != TEL_RETURN_SUCCESS) {
		dtapi_return_error(invocation, G_DBUS_ERROR_FAILED, "Request dispatch failed");
		tcore_free(rsp_cb_data);
	}

	return TRUE;
}

gboolean dtapi_setup_sat_interface(TelephonyObjectSkeleton *object, TcorePlugin *plugin, TcorePlugin *comm_plugin)
{
	TelephonySat *sat;
	DtapiSatPluginsInfo *plugins_info;

	sat = telephony_sat_skeleton_new();
	telephony_object_skeleton_set_sat(object, sat);
	g_object_unref(sat);

	dbg("sat = %p", sat);

	plugins_info = tcore_malloc0(sizeof(DtapiSatPluginsInfo));
	plugins_info->comm_plugin = comm_plugin;
	plugins_info->plugin = plugin;

	g_signal_connect (sat,
			"handle-get-main-menu-info",
			G_CALLBACK (dtapi_sat_get_main_menu_info),
			plugins_info);

	g_signal_connect (sat,
			"handle-send-ui-display-status",
			G_CALLBACK (dtapi_sat_send_display_status),
			plugins_info);

	g_signal_connect (sat,
			"handle-send-user-confirm",
			G_CALLBACK (dtapi_sat_send_user_confirm),
			plugins_info);

	g_signal_connect (sat,
			"handle-send-app-exec-result",
			G_CALLBACK (dtapi_sat_send_app_exec_result),
			plugins_info);

	g_signal_connect (sat,
			"handle-select-menu",
			G_CALLBACK (dtapi_sat_select_menu),
			plugins_info);

	g_signal_connect (sat,
			"handle-download-event",
			G_CALLBACK (dtapi_sat_download_event),
			plugins_info);

	return TRUE;
}

gboolean dtapi_handle_sat_notification(TelephonyObjectSkeleton *object,
	TcorePlugin *plugin, TcorePlugin *comm_plugin, TcoreNotification command,
	guint data_len, const void *data)
{
	TelephonySat *sat;
	const char *cp_name;
	DtapiSatPrivateData *sat_data = NULL;

	if (!object || !plugin) {
		err("Invalid parameters");
		return FALSE;
	}
	cp_name  = tcore_server_get_cp_name_by_plugin(plugin);

	dbg("Notification!!! Command: [0x%x] CP Name: [%s]", command, cp_name);

	sat = telephony_object_peek_sat(TELEPHONY_OBJECT(object));
	dbg("sat: [%p]", sat);

	sat_data = (DtapiSatPrivateData *)tcore_plugin_ref_user_data(comm_plugin);
	if (!sat_data) {
		err("sat_data is null");
		return FALSE;
	}

	/* SAT UI storage init */
	sat_ui_support_storage_init(sat_data->server);

	//session end notification
	switch (command) {
		case TCORE_NOTIFICATION_SAT_SESSION_END: {
			dbg("notified sat session end evt");
			sat_manager_init_queue();

			//sat_ui_support_terminate_sat_ui();
			telephony_sat_emit_end_proactive_session(sat,
				TEL_SAT_PROATV_CMD_TYPE_END_PROACTIVE_SESSION);

			return TRUE;
		}
		break;

		//call control notification
		case TCORE_NOTIFICATION_SAT_CALL_CTRL_RESULT: {
			TelSatNotiCallControlResultInd *cc_result_noti = NULL;
			gint call_ctrl_result = 0, bc_repeat_indicator = 0, ton = 0x0F, npi=0X0F;
			gchar *text = NULL, *call_num = NULL, *ss_string = NULL;
			gchar *sub_addr = NULL, *ccp1 = NULL, *ccp2 = NULL;

			cc_result_noti = (TelSatNotiCallControlResultInd *)data;
			if (NULL == cc_result_noti) {
				err("Indication data is NULL");
				return FALSE;
			}
			dbg("sat call control result notification");

			call_ctrl_result = cc_result_noti->cc_result;
			bc_repeat_indicator = cc_result_noti->bc_repeat_type;

			if (cc_result_noti->address.dialing_number_len > 0) {
				ton = cc_result_noti->address.ton;
				npi = cc_result_noti->address.npi;
				call_num = g_strdup(cc_result_noti->address.dialing_number);
				ss_string = g_strdup("");
			}
			else if (cc_result_noti->ss_string.string_len > 0) {
				ton = cc_result_noti->ss_string.ton;
				npi = cc_result_noti->ss_string.npi;
				call_num = g_strdup("");
				ss_string = g_strdup(cc_result_noti->ss_string.ss_string);
			}

			if (cc_result_noti->alpha_id.alpha_data_len > 0) {
				text = g_strdup(cc_result_noti->alpha_id.alpha_data);
			}
			else{
				text = g_strdup("");
			}

			if (cc_result_noti->sub_address.subaddress_len > 0) {
				sub_addr = g_strdup(cc_result_noti->sub_address.subaddress);
			}
			else{
				sub_addr = g_strdup("");
			}

			if (cc_result_noti->ccp1.data_len > 0) {
				ccp1 = g_strdup(cc_result_noti->ccp1.data);
			}
			else{
				ccp1 = g_strdup("");
			}


			if (cc_result_noti->ccp2.data_len > 0) {
				ccp2 = g_strdup(cc_result_noti->ccp2.data);
			}
			else{
				ccp2 = g_strdup("");
			}

#if 0			/* TODO - unblock */
			telephony_sat_emit_call_control_result(sat, call_ctrl_result, text, ton, npi, call_num,
				ss_string, sub_addr, ccp1, ccp2, bc_repeat_indicator);
#endif
			g_free(text); g_free(call_num); g_free(ss_string);
			g_free(sub_addr); g_free(ccp1); g_free(ccp2);
			return TRUE;
		}
		break;

		case TCORE_NOTIFICATION_SAT_MO_SM_CTRL_RESULT: {
			TelSatNotiMoSmControlResultInd *mo_sm_result_noti = NULL;
			gint call_ctrl_result = 0;
			gint rp_dst_ton = 0x0F, rp_dst_npi = 0X0F, tp_dst_ton = 0x0F, tp_dst_npi = 0X0F;
			gchar *text = NULL, *rp_dst_call_num = NULL, *tp_dst_call_num = NULL;

			mo_sm_result_noti = (TelSatNotiMoSmControlResultInd *)data;
			if (mo_sm_result_noti == NULL) {
				err("Indication data is NULL");
				return FALSE;
			}
			dbg("sat mo sm control result notification");

			call_ctrl_result = mo_sm_result_noti->cc_result;

			if (mo_sm_result_noti->rp_dst_address.dialing_number_len > 0) {
				rp_dst_ton = mo_sm_result_noti->rp_dst_address.ton;
				rp_dst_npi = mo_sm_result_noti->rp_dst_address.npi;
				rp_dst_call_num = g_strdup(mo_sm_result_noti->rp_dst_address.dialing_number);
			} else {
				rp_dst_call_num = g_strdup("");
			}

			if (mo_sm_result_noti->tp_dst_address.dialing_number_len > 0) {
				tp_dst_ton = mo_sm_result_noti->tp_dst_address.ton;
				tp_dst_npi = mo_sm_result_noti->tp_dst_address.npi;
				tp_dst_call_num = g_strdup(mo_sm_result_noti->tp_dst_address.dialing_number);
			} else {
				tp_dst_call_num = g_strdup("");
			}

			if (mo_sm_result_noti->alpha_id.alpha_data_len > 0) {
				text = g_strdup(mo_sm_result_noti->alpha_id.alpha_data);
			}
			else{
				text = g_strdup("");
			}

#if 0			/* TODO - unblock */
			telephony_sat_emit_mo_sm_control_result(sat, call_ctrl_result, text,
				rp_dst_ton, rp_dst_npi, rp_dst_call_num, tp_dst_ton, tp_dst_npi, tp_dst_call_num);
#endif
			g_free(text); g_free(rp_dst_call_num); g_free(tp_dst_call_num);
			return TRUE;
		}
		break;

		//Proactive Command Notification
		case TCORE_NOTIFICATION_SAT_PROACTIVE_CMD: {
			TelSatNotiProactiveData *p_ind;

			if (cp_name == NULL) {
				err("CP name is NULL");
				return FALSE;
			}

			p_ind = (TelSatNotiProactiveData *)data;
			if (p_ind == NULL) {
				err("Indication data is NULL");
				return FALSE;
			}
			dbg("notified sat proactive command(%d)", p_ind->cmd_type);

			switch (p_ind->cmd_type) {
				case TEL_SAT_PROATV_CMD_SETUP_MENU:{
					gboolean rv = FALSE;
					GVariant *menu_info = NULL;
					GVariant *resp = NULL;
					GVariant *exec_result = NULL;

					gchar *title;
					gint command_id, menu_cnt;
					gboolean is_present, is_helpinfo, is_updated;
					GVariant *items;

					menu_info = sat_manager_extracting_setup_menu_info(plugin, comm_plugin, (TelSatSetupMenuTlv*)&p_ind->proactive_ind_data.setup_menu);

					if (!menu_info) {
						err("no main menu data");
						sat_ui_support_remove_desktop_file();
						telephony_sat_emit_end_proactive_session(sat, TEL_SAT_PROATV_CMD_TYPE_END_PROACTIVE_SESSION);
						return TRUE;
					}

					_util_sat_set_main_menu(sat_data, cp_name, menu_info);

					dbg("menu_info type_format(%s)", g_variant_get_type_string(menu_info));
					g_variant_get(menu_info, "(ibs@vibb)", &command_id, &is_present, &title, &items,
							&menu_cnt, &is_helpinfo, &is_updated);

					rv = sat_ui_support_create_desktop_file(title);

					dbg("return value (%d)", rv);
					if (rv)
						resp = g_variant_new("(i)", TEL_SAT_RESULT_SUCCESS);
					else
						resp = g_variant_new("(i)", TEL_SAT_RESULT_ME_UNABLE_TO_PROCESS_COMMAND);

					exec_result = g_variant_new_variant(resp);
					sat_manager_handle_app_exec_result(plugin, command_id, TEL_SAT_PROATV_CMD_SETUP_MENU, exec_result);

					//sat_ui_support_launch_sat_ui(SAT_PROATV_CMD_SETUP_MENU, menu_info);
					if (is_updated)
						telephony_sat_emit_setup_menu(sat, command_id, is_present, title, items, menu_cnt,
							is_helpinfo, is_updated);
				}
				break;

				case TEL_SAT_PROATV_CMD_DISPLAY_TEXT:{
					GVariant *display_text = NULL;

					gint command_id, text_len, duration;
					gboolean high_priority, user_rsp_required, immediately_rsp;
					gchar* text;
					GVariant *icon_id = NULL;
					int ret;

					display_text = sat_manager_display_text_noti(plugin, (TelSatDisplayTextTlv*) &p_ind->proactive_ind_data.display_text, p_ind->decode_err_code);

					if (!display_text) {
						err("no display text data");
						return TRUE;
					}

					dbg("display text type_format(%s)", g_variant_get_type_string(display_text));
					g_variant_get(display_text, "(isiibbb@v)", &command_id, &text, &text_len, &duration,
								&high_priority, &user_rsp_required, &immediately_rsp, &icon_id);

					ret = sat_ui_support_launch_sat_ui(TEL_SAT_PROATV_CMD_DISPLAY_TEXT, display_text);
					if (!ret) {
						int rv;
						err("fail to launch sat-ui, remove the queued data!!\n");
						if (!sat_manager_handle_sat_ui_launch_fail(plugin, p_ind))
							err("Fail to send terminal response\n");
						rv = sat_manager_remove_cmd_by_id(command_id);
						if (!rv)
							err("fail to dequeue data\n");
					}

					telephony_sat_emit_display_text(sat, command_id, text, text_len, duration,
							high_priority, user_rsp_required, immediately_rsp);

				}
				break;

				case TEL_SAT_PROATV_CMD_SELECT_ITEM:{
					GVariant *select_menu = NULL;

					gboolean help_info ;
					gchar *selected_text;
					gint command_id, default_item_id, menu_cnt, text_len =0;
					GVariant *menu_items, *icon_id, *icon_list;
					int ret;

					select_menu = sat_manager_select_item_noti(plugin, (TelSatSelectItemTlv*) &p_ind->proactive_ind_data.select_item, p_ind->decode_err_code);

					if (!select_menu) {
						err("no select menu data");
						return TRUE;
					}

					dbg("select menu type_format(%s)", g_variant_get_type_string(select_menu));
					g_variant_get(select_menu, "(ibsiii@v@v@v)", &command_id, &help_info, &selected_text,
							&text_len, &default_item_id, &menu_cnt, &menu_items, &icon_id, &icon_list);

					ret = sat_ui_support_launch_sat_ui(TEL_SAT_PROATV_CMD_SELECT_ITEM, select_menu);
					if (!ret) {
						int rv;
						err("fail to launch sat-ui, remove the queued data!!\n");
						if (!sat_manager_handle_sat_ui_launch_fail(plugin, p_ind))
							err("Fail to send terminal response\n");
						rv = sat_manager_remove_cmd_by_id(command_id);
						if (!rv)
							err("fail to dequeue data\n");
					}

					telephony_sat_emit_select_item (sat, command_id, help_info, selected_text, text_len,
							default_item_id, menu_cnt, menu_items);
				}
				break;

				case TEL_SAT_PROATV_CMD_GET_INKEY:{
					GVariant *get_inkey = NULL;
					gint command_id, key_type, input_character_mode;
					gint text_len, duration;
					gboolean is_numeric, is_help_info;
					gchar *text;
					GVariant *icon_id;
					int ret;

					get_inkey = sat_manager_get_inkey_noti(plugin, (TelSatGetInkeyTlv*) &p_ind->proactive_ind_data.get_inkey, p_ind->decode_err_code);

					if (!get_inkey) {
						err("no get inkey data");
						return TRUE;
					}

					dbg("get inkey type_format(%s)", g_variant_get_type_string(get_inkey));
					g_variant_get(get_inkey, "(iiibbsii@v)", &command_id, &key_type, &input_character_mode,
							&is_numeric,&is_help_info, &text, &text_len, &duration, &icon_id);

					ret = sat_ui_support_launch_sat_ui(TEL_SAT_PROATV_CMD_GET_INKEY, get_inkey);
					if (!ret) {
						int rv;
						err("fail to launch sat-ui, remove the queued data!!\n");
						if (!sat_manager_handle_sat_ui_launch_fail(plugin, p_ind))
							err("Fail to send terminal response\n");
						rv = sat_manager_remove_cmd_by_id(command_id);
						if (!rv)
							err("fail to dequeue data\n");
					}

					telephony_sat_emit_get_inkey(sat, command_id, key_type, input_character_mode,
							is_numeric, is_help_info, text, text_len, duration);
				}
				break;

				case TEL_SAT_PROATV_CMD_GET_INPUT:{
					GVariant *get_input = NULL;
					gint command_id, input_character_mode;
					gint text_len, def_text_len, rsp_len_min, rsp_len_max;
					gboolean is_numeric, is_help_info, is_echo_input;
					gchar *text, *def_text;
					GVariant *icon_id;
					int ret;

					get_input = sat_manager_get_input_noti(plugin, (TelSatGetInputTlv*) &p_ind->proactive_ind_data.get_input, p_ind->decode_err_code);

					if (!get_input) {
						err("no get input data");
						return TRUE;
					}

					dbg("get input type_format(%s)", g_variant_get_type_string(get_input));
					g_variant_get(get_input, "(iibbbsiiisi@v)", &command_id, &input_character_mode, &is_numeric, &is_help_info, &is_echo_input,
							&text, &text_len, &rsp_len_max, &rsp_len_min, &def_text, &def_text_len, &icon_id);

					ret = sat_ui_support_launch_sat_ui(TEL_SAT_PROATV_CMD_GET_INPUT, get_input);
					if (!ret) {
						int rv;
						err("fail to launch sat-ui, remove the queued data!!\n");
						if (!sat_manager_handle_sat_ui_launch_fail(plugin, p_ind))
							err("Fail to send terminal response\n");
						rv = sat_manager_remove_cmd_by_id(command_id);
						if (!rv)
							err("fail to dequeue data\n");
					}

					telephony_sat_emit_get_input(sat, command_id, input_character_mode, is_numeric, is_help_info,
							is_echo_input, text, text_len, rsp_len_max, rsp_len_min, def_text, def_text_len);
				}
				break;

				case TEL_SAT_PROATV_CMD_PLAY_TONE:{
					GVariant *play_tone = NULL;
					gint command_id, tone_type, duration;
					gint text_len;
					gchar* text;
					GVariant *icon_id;
					int ret;

					play_tone = sat_manager_play_tone_noti(plugin, (TelSatPlayToneTlv*) &p_ind->proactive_ind_data.play_tone);

					if (!play_tone) {
						err("no play tone data");
						return TRUE;
					}

					dbg("play tone type_format(%s)", g_variant_get_type_string(play_tone));
					g_variant_get(play_tone, "(isi@vii)", &command_id, &text, &text_len, &icon_id, &tone_type, &duration);

					ret = sat_ui_support_launch_sat_ui(TEL_SAT_PROATV_CMD_PLAY_TONE, play_tone);
					if (!ret) {
						int rv;
						err("fail to launch sat-ui, remove the queued data!!\n");
						if (!sat_manager_handle_sat_ui_launch_fail(plugin, p_ind))
							err("Fail to send terminal response\n");
						rv = sat_manager_remove_cmd_by_id(command_id);
						if (!rv)
							err("fail to dequeue data\n");
					}

					telephony_sat_emit_play_tone(sat, command_id, text, text_len, tone_type, duration);
				}
				break;

				case TEL_SAT_PROATV_CMD_SEND_SMS:{
					GVariant *send_sms = NULL;

					gint command_id, ton, npi, tpdu_type;
					gboolean is_packing_required;
					gint text_len, number_len, tpdu_data_len;
					gchar* text, *dialling_number;
					GVariant *tpdu_data, *icon_id;

					send_sms = sat_manager_send_sms_noti(plugin, (TelSatSendSmsTlv*) &p_ind->proactive_ind_data.send_sms);

					if (!send_sms) {
						err("no send sms data");
						return TRUE;
					}

					dbg("send sms type_format(%s)", g_variant_get_type_string(send_sms));
					g_variant_get(send_sms, "(isi@vbiisii@vi)", &command_id, &text, &text_len, &icon_id, &is_packing_required, &ton, &npi,
							&dialling_number, &number_len, &tpdu_type, &tpdu_data, &tpdu_data_len);

					dbg("check display text : text(%s) text len(%d)", text, text_len);
					if (text_len > 1 && (g_strcmp0(text,"") != 0) ) {
						GVariant *ui_info = NULL;
						gboolean user_confirm = FALSE;
						int ret;
						dbg("text should be displayed by ui");
						dbg("send sms is pending!!!");

						ui_info = g_variant_new("(isib)", command_id, text, text_len, user_confirm);
						ret = sat_ui_support_launch_sat_ui(TEL_SAT_PROATV_CMD_NONE, ui_info);
						if (!ret) {
							int rv;
							err("fail to launch sat-ui, remove the queued data!!\n");
							if (!sat_manager_handle_sat_ui_launch_fail(plugin, p_ind))
								err("Fail to send terminal response\n");
							rv = sat_manager_remove_cmd_by_id(command_id);
							if (!rv)
								err("fail to dequeue data\n");
						}
						return TRUE;
					}
					telephony_sat_emit_send_sms(sat, command_id, text, text_len, is_packing_required,
							ton, npi, dialling_number, number_len, tpdu_type, tpdu_data, tpdu_data_len);
				}
				break;

				case TEL_SAT_PROATV_CMD_SEND_SS:{
					GVariant *send_ss = NULL;

					gint command_id, ton, npi;
					gint text_len, ss_str_len;
					gchar* text, *ss_string;

					GVariant *icon_id;

					send_ss = sat_manager_send_ss_noti(plugin, (TelSatSendSsTlv*) &p_ind->proactive_ind_data.send_ss);

					if (!send_ss) {
						err("no send ss data");
						return TRUE;
					}

					dbg("send ss type_format(%s)", g_variant_get_type_string(send_ss));
					g_variant_get(send_ss, "(isi@viiis)", &command_id, &text, &text_len, &icon_id,
							&ton, &npi, &ss_str_len, &ss_string);

					dbg("check display text : text(%s) text len(%d)", text, text_len);
					if (text_len > 1 && (g_strcmp0(text,"") != 0) ) {
						GVariant *ui_info = NULL;
						gboolean user_confirm = FALSE;
						int ret;
						dbg("text should be displayed by ui");
						dbg("send ss is pending!!!");

						ui_info = g_variant_new("(isib)", command_id, text, text_len, user_confirm);
						ret = sat_ui_support_launch_sat_ui(TEL_SAT_PROATV_CMD_NONE, ui_info);
						if (!ret) {
							int rv;
							err("fail to launch sat-ui, remove the queued data!!\n");
							if (!sat_manager_handle_sat_ui_launch_fail(plugin, p_ind))
								err("Fail to send terminal response\n");
							rv = sat_manager_remove_cmd_by_id(command_id);
							if (!rv)
								err("fail to dequeue data\n");
						}
						return TRUE;
					}
					telephony_sat_emit_send_ss(sat, command_id, text, text_len, ton, npi, ss_string);
					sat_ui_support_launch_ciss_application(TEL_SAT_PROATV_CMD_SEND_SS, send_ss);
				}
				break;

				case TEL_SAT_PROATV_CMD_SEND_USSD:{
					GVariant *send_ussd = NULL;

					gint command_id;
					gint text_len, ussd_str_len;
					guchar dcs;
					gchar* text, *ussd_string;

					GVariant *icon_id;

					send_ussd = sat_manager_send_ussd_noti(plugin, (TelSatSendUssdTlv*) &p_ind->proactive_ind_data.send_ussd);

					if (!send_ussd) {
						err("no send ussd data");
						return TRUE;
					}

					dbg("send ussd type_format(%s)", g_variant_get_type_string(send_ussd));
					g_variant_get(send_ussd, "(isi@vyis)", &command_id, &text, &text_len, &icon_id, &dcs, &ussd_str_len, &ussd_string);

					dbg("check display text : text(%s) text len(%d)", text, text_len);
					if (text_len > 1 && (g_strcmp0(text,"") != 0) ) {
						GVariant *ui_info = NULL;
						gboolean user_confirm = FALSE;
						int ret;
						dbg("text should be displayed by ui");
						dbg("send ussd is pending!!!");

						ui_info = g_variant_new("(isib)", command_id, text, text_len, user_confirm);
						ret = sat_ui_support_launch_sat_ui(TEL_SAT_PROATV_CMD_NONE, ui_info);
						if (!ret) {
							int rv;
							err("fail to launch sat-ui, remove the queued data!!\n");
							if (!sat_manager_handle_sat_ui_launch_fail(plugin, p_ind))
								err("Fail to send terminal response\n");
							rv = sat_manager_remove_cmd_by_id(command_id);
							if (!rv)
								err("fail to dequeue data\n");
						}
						return TRUE;
					}
					/*TODO: dcs is not passed here. confirm whether is required or not*/
					telephony_sat_emit_setup_ussd(sat, command_id, text, text_len, ussd_string);
					sat_ui_support_launch_ciss_application(TEL_SAT_PROATV_CMD_SEND_USSD, send_ussd);
				}
				break;

				case TEL_SAT_PROATV_CMD_SETUP_CALL:{
					GVariant *setup_call = NULL;

					gint command_id, call_type, confirmed_text_len, text_len, duration;
					gchar *confirmed_text, *text, *call_number;
					GVariant *icon_id;

					setup_call = sat_manager_setup_call_noti(plugin, (TelSatSetupCallTlv*) &p_ind->proactive_ind_data.setup_call);

					if (!setup_call) {
						err("no setup call data");
						return TRUE;
					}

					dbg("setup call type_format(%s)", g_variant_get_type_string(setup_call));
					g_variant_get(setup_call, "(isisi@visi)", &command_id, &confirmed_text, &confirmed_text_len, &text, &text_len, &icon_id, &call_type, &call_number, &duration);

					dbg("check display text : text(%s) text len(%d)", confirmed_text, confirmed_text_len);
					if (confirmed_text_len > 1 && (g_strcmp0(confirmed_text,"") != 0) ) {
						GVariant *ui_info = NULL;
						gboolean user_confirm = TRUE;
						int ret;
						dbg("text should be displayed by ui");
						dbg("setup call is pending!!!");

						ui_info = g_variant_new("(isib)", command_id, confirmed_text, confirmed_text_len, user_confirm);
						ret = sat_ui_support_launch_sat_ui(TEL_SAT_PROATV_CMD_NONE, ui_info);
						if (!ret) {
							int rv;
							err("fail to launch sat-ui, remove the queued data!!\n");
							if (!sat_manager_handle_sat_ui_launch_fail(plugin, p_ind))
								err("Fail to send terminal response\n");
							rv = sat_manager_remove_cmd_by_id(command_id);
							if (!rv)
								err("fail to dequeue data\n");
						}
						return TRUE;
					}

					telephony_sat_emit_setup_call(sat, command_id, confirmed_text, confirmed_text_len,text, text_len, call_type, call_number, duration);
					sat_ui_support_launch_call_application(TEL_SAT_PROATV_CMD_SETUP_CALL, setup_call);
				}
				break;

				case TEL_SAT_PROATV_CMD_SETUP_EVENT_LIST:{
					GVariant *event_list = NULL;

					gint event_cnt;
					GVariant *evt_list;

					event_list = sat_manager_setup_event_list_noti(plugin, (TelSatSetupEventListTlv*) &p_ind->proactive_ind_data.setup_event_list);

					if (!event_list) {
						err("no setup event list data");
						return TRUE;
					}

					dbg("setup event list type_format(%s)", g_variant_get_type_string(event_list));
					g_variant_get(event_list, "(i@v)", &event_cnt, &evt_list);

					telephony_sat_emit_setup_event_list(sat, event_cnt, evt_list);

					//bip proactive command is only handled by BIP Manager
					{
						GDBusConnection *conn = NULL;
						const gchar *g_path = NULL;

						conn = g_dbus_object_manager_server_get_connection(sat_data->manager);
						g_path = g_dbus_object_get_object_path(G_DBUS_OBJECT(object));

						/* TODO: SAT Event Downloader should execute event_list as well. */
						sat_ui_support_exec_evtdw(conn, g_path, TEL_SAT_PROATV_CMD_SETUP_EVENT_LIST, event_list);
						sat_ui_support_exec_bip(conn, g_path, TEL_SAT_PROATV_CMD_SETUP_EVENT_LIST, event_list);
					}
				}
				break;

				case TEL_SAT_PROATV_CMD_SETUP_IDLE_MODE_TEXT:{
					GVariant *setup_idle_mode = NULL;
					int ret;

					gint command_id, text_len;
					gchar* text;
					GVariant *icon_id;

					setup_idle_mode = sat_manager_setup_idle_mode_text_noti(plugin, (TelSatSetupIdleModeTextTlv*) &p_ind->proactive_ind_data.setup_idle_mode_text);

					if (!setup_idle_mode) {
						err("no setup idle mode text data");
						return TRUE;
					}

					dbg("setup idle mode text type_format(%s)", g_variant_get_type_string(setup_idle_mode));
					g_variant_get(setup_idle_mode, "(isi@v)", &command_id, &text, &text_len, &icon_id);

					ret = sat_ui_support_launch_sat_ui(TEL_SAT_PROATV_CMD_SETUP_IDLE_MODE_TEXT, setup_idle_mode);
					if (!ret) {
						int rv;
						err("fail to launch sat-ui, remove the queued data!!\n");
						if (!sat_manager_handle_sat_ui_launch_fail(plugin, p_ind))
							err("Fail to send terminal response\n");
						rv = sat_manager_remove_cmd_by_id(command_id);
						if (!rv)
							err("fail to dequeue data\n");
					}

					telephony_sat_emit_setup_idle_mode_text(sat, command_id, text, text_len);
				}
				break;

				case TEL_SAT_PROATV_CMD_OPEN_CHANNEL:{
					GVariant *open_channel = NULL;

					gint command_id, bearer_type, protocol_type, dest_addr_type;
					gboolean immediate_link, auto_reconnection, bg_mode;
					gint text_len, buffer_size, port_number;
					gchar *text, *dest_address;
					GVariant *icon_id;
					GVariant *bearer_param;
					GVariant *bearer_detail;

					open_channel = sat_manager_open_channel_noti(plugin, (TelSatOpenChannelTlv*) &p_ind->proactive_ind_data.open_channel);

					if (!open_channel) {
						err("no open channel data");
						return TRUE;
					}

					dbg("open channel type_format(%s)", g_variant_get_type_string(open_channel));
					g_variant_get(open_channel,"(isi@vbbbi@viiiis@v)", &command_id, &text, &text_len, &icon_id, &immediate_link, &auto_reconnection, &bg_mode,
							&bearer_type, &bearer_param, &buffer_size, &protocol_type, &port_number, &dest_addr_type, &dest_address, &bearer_detail);

					dbg("check display text : text(%s) text len(%d)", text, text_len);
					if (text_len > 1 && (g_strcmp0(text,"") != 0) ) {
						GVariant *ui_info = NULL;
						gboolean user_confirm = TRUE;
						int ret;
						dbg("text should be displayed by ui");
						dbg("open channel text is displayed!!!");

						ui_info = g_variant_new("(isib)", command_id, text, text_len, user_confirm);
						ret = sat_ui_support_launch_sat_ui(TEL_SAT_PROATV_CMD_NONE, ui_info);
							if (!ret) {
							int rv;
							err("fail to launch sat-ui, remove the queued data!!\n");
							if (!sat_manager_handle_sat_ui_launch_fail(plugin, p_ind))
								err("Fail to send terminal response\n");
							rv = sat_manager_remove_cmd_by_id(command_id);
							if (!rv)
								err("fail to dequeue data\n");
						}
						return TRUE;
					}

					/*telephony_sat_emit_open_channel(sat, command_id, text, text_len, immediate_link, auto_reconnection, bg_mode,
								bearer_type, bearer_param, buffer_size, protocol_type, port_number, dest_addr_type, dest_address, bearer_detail);*/
					//bip proactive command is only handled by BIP Manager
					{
						GDBusConnection *conn = NULL;
						const gchar *g_path = NULL;

						conn = g_dbus_object_manager_server_get_connection(sat_data->manager);
						g_path = g_dbus_object_get_object_path(G_DBUS_OBJECT(object));

						sat_ui_support_exec_bip(conn, g_path, TEL_SAT_PROATV_CMD_OPEN_CHANNEL, open_channel);
					}
				}
				break;

				case TEL_SAT_PROATV_CMD_CLOSE_CHANNEL:{
					GVariant *close_channel = NULL;

					gint command_id, channel_id, text_len;
					gchar *text;
					GVariant *icon_id;

					close_channel = sat_manager_close_channel_noti(plugin, (TelSatCloseChannelTlv*) &p_ind->proactive_ind_data.close_channel);

					if (!close_channel) {
						err("no close channel data");
						return TRUE;
					}

					//TODO check the data for sat-ui

					dbg("close channel type_format(%s)", g_variant_get_type_string(close_channel));
					g_variant_get(close_channel, "(isi@vi)", &command_id, &text, &text_len, &icon_id, &channel_id);

					/*telephony_sat_emit_close_channel(sat, command_id, text, text_len, channel_id);*/

					//bip proactive command is only handled by BIP Manager
					{
						GDBusConnection *conn = NULL;
						const gchar *g_path = NULL;

						conn = g_dbus_object_manager_server_get_connection(sat_data->manager);
						g_path = g_dbus_object_get_object_path(G_DBUS_OBJECT(object));

						sat_ui_support_exec_bip(conn, g_path, TEL_SAT_PROATV_CMD_CLOSE_CHANNEL, close_channel);
					}

				}
				break;

				case TEL_SAT_PROATV_CMD_RECEIVE_DATA:{
					GVariant *receive_data = NULL;

					gint command_id, text_len, channel_id, channel_data_len = 0;
					gchar *text;
					GVariant *icon_id;

					receive_data = sat_manager_receive_data_noti(plugin, (TelSatReceiveChannelTlv*) &p_ind->proactive_ind_data.receive_data);

					if (!receive_data) {
						err("no receive data data");
						return TRUE;
					}

					//TODO check the data for sat-ui

					dbg("receive data type_format(%s)", g_variant_get_type_string(receive_data));
					g_variant_get(receive_data, "(isi@vii)", &command_id, &text, &text_len, &icon_id, &channel_id, &channel_data_len);

					/*telephony_sat_emit_receive_data(sat, command_id, text, text_len, channel_id, channel_data_len);*/

					//bip proactive command is only handled by BIP Manager
					{
						GDBusConnection *conn = NULL;
						const gchar *g_path = NULL;

						conn = g_dbus_object_manager_server_get_connection(sat_data->manager);
						g_path = g_dbus_object_get_object_path(G_DBUS_OBJECT(object));

						sat_ui_support_exec_bip(conn, g_path, TEL_SAT_PROATV_CMD_RECEIVE_DATA, receive_data);
					}

				}
				break;

				case TEL_SAT_PROATV_CMD_SEND_DATA:{
					GVariant *send_data = NULL;

					gint command_id, channel_id, text_len, channel_data_len;
					gboolean send_data_immediately;
					gchar *text;
					GVariant *channel_data;
					GVariant *icon_id;

					send_data = sat_manager_send_data_noti(plugin, (TelSatSendChannelTlv*) &p_ind->proactive_ind_data.send_data);

					if (!send_data) {
						err("no send data data");
						return TRUE;
					}

					//TODO check the data for sat-ui

					dbg("send data type_format(%s)", g_variant_get_type_string(send_data));
					g_variant_get(send_data, "(isi@vib@vi)", &command_id, &text, &text_len, &icon_id, &channel_id, &send_data_immediately, &channel_data, &channel_data_len);

					/*telephony_sat_emit_send_data(sat, command_id, text, text_len, channel_id, send_data_immediately, channel_data, channel_data_len);*/

					//bip proactive command is only handled by BIP Manager
					{
						GDBusConnection *conn = NULL;
						const gchar *g_path = NULL;

						conn = g_dbus_object_manager_server_get_connection(sat_data->manager);
						g_path = g_dbus_object_get_object_path(G_DBUS_OBJECT(object));

						sat_ui_support_exec_bip(conn, g_path, TEL_SAT_PROATV_CMD_SEND_DATA, send_data);
					}
				}
				break;

				case TEL_SAT_PROATV_CMD_GET_CHANNEL_STATUS:{
					GVariant *channel_status = NULL;

					gint command_id;

					channel_status = sat_manager_get_channel_status_noti(plugin, (TelSatGetChannelStatusTlv*) &p_ind->proactive_ind_data.get_channel_status);

					if (!channel_status) {
						err("no get channel status data");
						return TRUE;
					}

					//TODO check the data for sat-ui

					dbg("get channel status type_format(%s)", g_variant_get_type_string(channel_status));
					g_variant_get(channel_status, "(i)", &command_id);

					/*telephony_sat_emit_get_channel_status(sat, command_id);*/

					//bip proactive command is only handled by BIP Manager
					{
						GDBusConnection *conn = NULL;
						const gchar *g_path = NULL;

						conn = g_dbus_object_manager_server_get_connection(sat_data->manager);
						g_path = g_dbus_object_get_object_path(G_DBUS_OBJECT(object));

						sat_ui_support_exec_bip(conn, g_path, TEL_SAT_PROATV_CMD_GET_CHANNEL_STATUS, channel_status);
					}
				}
				break;

				case TEL_SAT_PROATV_CMD_REFRESH:{
					GVariant *refresh = NULL;
					gint command_id = 0;
					gint refresh_type =0;
					GVariant *file_list = NULL;
					int ret;

					refresh = sat_manager_refresh_noti(plugin, (TelSatRefreshTlv*) &p_ind->proactive_ind_data.refresh);

					if (!refresh) {
						err("no refresh data");
						return TRUE;
					}

					dbg("refresh type_format(%s)", g_variant_get_type_string(refresh));
					g_variant_get(refresh, "(ii@v)", &command_id, &refresh_type, &file_list);

					telephony_sat_emit_refresh(sat, command_id, refresh_type, file_list);
					ret = sat_ui_support_launch_sat_ui(TEL_SAT_PROATV_CMD_REFRESH, refresh);
					if (!ret) {
						int rv;
						err("fail to launch sat-ui, remove the queued data!!\n");
						if (!sat_manager_handle_sat_ui_launch_fail(plugin, p_ind))
							err("Fail to send terminal response\n");
						rv = sat_manager_remove_cmd_by_id(command_id);
						if (!rv)
							err("fail to dequeue data\n");
					}
				}
				break;

				case TEL_SAT_PROATV_CMD_MORE_TIME:{
					sat_manager_more_time_noti(plugin, (TelSatMoreTimeTlv*) &p_ind->proactive_ind_data.more_time);
					telephony_sat_emit_more_time(sat);
				}
				break;

				case TEL_SAT_PROATV_CMD_SEND_DTMF:{
					GVariant *send_dtmf = NULL;
					gint command_id = 0;
					gint text_len = 0, dtmf_str_len = 0;
					gchar *text = NULL;
					gchar *dtmf_str = NULL;
					GVariant *icon_id = NULL;

					send_dtmf = sat_manager_send_dtmf_noti(plugin, (TelSatSendDtmfTlv*) &p_ind->proactive_ind_data.send_dtmf);
					if (!send_dtmf) {
						err("no send_dtmf data");
						return TRUE;
					}

					dbg("send_dtmf type_format(%s)", g_variant_get_type_string(send_dtmf));
					g_variant_get(send_dtmf, "(isi@vis)", &command_id, &text, &text_len, &icon_id, &dtmf_str_len, &dtmf_str);

					if (text_len > 1 && (g_strcmp0(text,"") != 0) ) {
						GVariant *ui_info = NULL;
						gboolean user_confirm = FALSE;
						int ret;
						dbg("text should be displayed by ui");
						dbg("send dtmf is displayed!!!");

						ui_info = g_variant_new("(isib)", command_id, text, text_len, user_confirm);
						ret = sat_ui_support_launch_sat_ui(TEL_SAT_PROATV_CMD_SEND_DTMF, ui_info);
						if (!ret) {
							int rv;
							err("fail to launch sat-ui, remove the queued data!!\n");
							if (!sat_manager_handle_sat_ui_launch_fail(plugin, p_ind))
								err("Fail to send terminal response\n");
							rv = sat_manager_remove_cmd_by_id(command_id);
							if (!rv)
								err("fail to dequeue data\n");
						}
						return TRUE;
					}
					telephony_sat_emit_send_dtmf(sat, command_id, text, text_len, dtmf_str, dtmf_str_len);
				}
				break;

				case TEL_SAT_PROATV_CMD_LAUNCH_BROWSER:{
					GVariant *launch_browser = NULL;
					gint command_id = 0;
					gint browser_launch_type = 0, browser_id = 0;
					gint url_len = 0, text_len = 0, gateway_proxy_len =0;
					gchar *url = NULL;
					gchar *text = NULL;
					gchar *gateway_proxy = NULL;
					GVariant *icon_id = NULL;

					launch_browser = sat_manager_launch_browser_noti(plugin, (TelSatLaunchBrowserTlv*) &p_ind->proactive_ind_data.launch_browser);
					if (!launch_browser) {
						err("no launch_browser data");
						return TRUE;
					}

					dbg("launch_browser type_format(%s)", g_variant_get_type_string(launch_browser));
					g_variant_get(launch_browser, "(iiisisisi@v)", &command_id, &browser_launch_type, &browser_id, &url, &url_len, &gateway_proxy, &gateway_proxy_len, &text, &text_len, &icon_id);

					//Popup is mandatory option in browser case
					{
						GVariant *ui_info = NULL;
						gboolean user_confirm = TRUE;
						int ret;
						dbg("text should be displayed by ui");
						dbg("launch browser is displayed!!!");

						ui_info = g_variant_new("(isib)", command_id, text, text_len, user_confirm);
						ret = sat_ui_support_launch_sat_ui(TEL_SAT_PROATV_CMD_LAUNCH_BROWSER, ui_info);
						if (!ret) {
							int rv;
							err("fail to launch sat-ui, remove the queued data!!\n");
							if (!sat_manager_handle_sat_ui_launch_fail(plugin, p_ind))
								err("Fail to send terminal response\n");
							rv = sat_manager_remove_cmd_by_id(command_id);
							if (!rv)
								err("fail to dequeue data\n");
						}
						return TRUE;
					}

					//telephony_sat_emit_launch_browser(sat, command_id, browser_launch_type, browser_id, url, url_len, gateway_proxy, gateway_proxy_len, text, text_len);
				}
				break;

				case TEL_SAT_PROATV_CMD_PROVIDE_LOCAL_INFO:{
					GVariant *provide_info = NULL;
					gint info_type = 0;

					provide_info = sat_manager_provide_local_info_noti(plugin, comm_plugin, (TelSatProvideLocalInfoTlv*) &p_ind->proactive_ind_data.provide_local_info);
					if (!provide_info) {
						err("no provide_info data");
						return TRUE;
					}

					dbg("provide_info type_format(%s)", g_variant_get_type_string(provide_info));
					g_variant_get(provide_info, "(i)", &info_type);

					telephony_sat_emit_provide_local_info(sat, info_type);
				}
				break;

				case TEL_SAT_PROATV_CMD_LANGUAGE_NOTIFICATION:{
					GVariant *language_noti = NULL;
					gint command_id = 0;
					gint language = 0;
					gboolean is_specified = FALSE;

					language_noti = sat_manager_language_notification_noti(plugin, (TelSatLanguageNotificationTlv*) &p_ind->proactive_ind_data.language_notification);
					if (!language_noti) {
						err("no language_noti data");
						return TRUE;
					}

					dbg("language_noti type_format(%s)", g_variant_get_type_string(language_noti));
					g_variant_get(language_noti, "(iib)", &command_id, &language, &is_specified);

					sat_manager_update_language(plugin, comm_plugin, language_noti);

					telephony_sat_emit_language_notification(sat, command_id, language, is_specified);
				}
				break;

				default:{
					gboolean rv = FALSE;
					rv = sat_manager_processing_unsupport_proactive_command(plugin, (TelSatUnsupportCommandTlv *)&p_ind->proactive_ind_data.unsupport_cmd);
					err("not handled ind->cmd_type[0x%x] send error tr result(%d)", p_ind->cmd_type, rv);
				}
				break;
			}
		}
		break;

		default:
			err("Unhandled Notification: [0x%x]", command);
		break;
	}

	return TRUE;
}

