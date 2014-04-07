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

#ifndef SAT_MANAGER_H_
#define SAT_MANAGER_H_

#include <tcore.h>
#include <co_sat.h>

#include "dtapi_sat.h"
#include "dtapi_util.h"

typedef union {
	TelSatDisplayTextTlv displayTextInd; /**<	Parsed proactive command info from TLV to Telephony data type - display text	*/
	TelSatGetInkeyTlv getInkeyInd; /**<	Parsed proactive command info from TLV to Telephony data type - getInkey	*/
	TelSatGetInputTlv getInputInd; /**<	Parsed proactive command info from TLV to Telephony data type - getInput	*/
	TelSatPlayToneTlv play_tone;
	TelSatSetupMenuTlv setupMenuInd; /**<	Parsed proactive command info from TLV to Telephony data type - setup menu	*/
	TelSatSelectItemTlv selectItemInd; /**<	Parsed proactive command info from TLV to Telephony data type - select item	*/
	TelSatSendSmsTlv sendSMSInd;
	TelSatSendSsTlv send_ss;
	TelSatSendUssdTlv send_ussd;
	TelSatSetupCallTlv setup_call;
	TelSatRefreshTlv refresh;
	TelSatProvideLocalInfoTlv provide_local_info;
	TelSatSetupIdleModeTextTlv idle_mode;
	TelSatSendDtmfTlv send_dtmf;
	TelSatLanguageNotificationTlv language_notification;
	TelSatLaunchBrowserTlv launch_browser;
	TelSatOpenChannelTlv open_channel;
	TelSatCloseChannelTlv close_channel;
	TelSatReceiveChannelTlv receive_data;
	TelSatSendChannelTlv send_data;
	TelSatGetChannelStatusTlv get_channel_status;
} TelSatManagerProactiveData;

/**
 * This structure defines the Command Queue Info.
 */
typedef struct {
	TelSatProactiveCmdType cmd_type; /**<Type of Command*/
	int cmd_id; /**<Command Id*/
	TelSatManagerProactiveData cmd_data; /**<Proactive Cmd Ind Info*/
} SatManagerQueueData;

typedef struct {
	char *cp_name;
	gpointer cached_sat_main_menu;
} SatCachedData;

typedef struct {
	Server *server;
	SatCachedData *data;
	GHashTable *objects;
	GDBusObjectManagerServer *manager;
	GSList *cached_data;
} DtapiSatPrivateData;

typedef struct
{
	TcorePlugin *plugin;
	TcorePlugin *comm_plugin;
} DtapiSatPluginsInfo;

/*================================================================================================*/
// sat-manager init / deinit
void sat_manager_init(TcorePlugin *plugin, GHashTable *objects, Server *server, GDBusObjectManagerServer *manager);
void sat_manager_deinit(TcorePlugin *plugin);

// queue handling
void sat_manager_init_queue();
gboolean sat_manager_remove_cmd_by_id(int cmd_id);

//application request handling
gboolean sat_manager_handle_user_confirm(DtapiSatPluginsInfo *plugins_info, GVariant *user_confirm_data);
gboolean sat_manager_handle_app_exec_result(TcorePlugin *plugin, gint command_id, gint command_type, GVariant *exec_result);
gboolean sat_manager_handle_ui_display_status(DtapiSatPluginsInfo *plugins_info, gint command_id, gboolean display_status);
gboolean sat_manager_handle_event_download_envelop(int event_type,  int src_dev, int dest_dev, TelSatEnvelopEventDownloadTlv *evt_download, GVariant *download_data);
gboolean sat_manager_update_language(TcorePlugin *plugin, TcorePlugin *comm_plugin, GVariant *language_noti);

//proactive command processing
GVariant *sat_manager_extracting_setup_menu_info(TcorePlugin *plugin, TcorePlugin *comm_plugin, TelSatSetupMenuTlv* setup_menu_tlv);
GVariant *sat_manager_display_text_noti(TcorePlugin *plugin, TelSatDisplayTextTlv* display_text_tlv, int decode_error);
GVariant *sat_manager_select_item_noti(TcorePlugin *plugin, TelSatSelectItemTlv* select_item_tlv, int decode_error);
GVariant *sat_manager_get_inkey_noti(TcorePlugin *plugin, TelSatGetInkeyTlv* get_inkey_tlv, int decode_error);
GVariant *sat_manager_get_input_noti(TcorePlugin *plugin, TelSatGetInputTlv* get_input_tlv, int decode_error);
GVariant *sat_manager_play_tone_noti(TcorePlugin *plugin, TelSatPlayToneTlv* play_tone_tlv);
GVariant *sat_manager_send_sms_noti(TcorePlugin *plugin, TelSatSendSmsTlv* send_sms_tlv);
GVariant *sat_manager_send_ss_noti(TcorePlugin *plugin, TelSatSendSsTlv* send_ss_tlv);
GVariant *sat_manager_send_ussd_noti(TcorePlugin *plugin, TelSatSendUssdTlv* send_ussd_tlv);
GVariant *sat_manager_setup_call_noti(TcorePlugin *plugin, TelSatSetupCallTlv* setup_call_tlv);
GVariant *sat_manager_setup_event_list_noti(TcorePlugin *plugin, TelSatSetupEventListTlv *event_list_tlv);
GVariant *sat_manager_setup_idle_mode_text_noti(TcorePlugin *plugin, TelSatSetupIdleModeTextTlv *idle_mode_tlv);
GVariant *sat_manager_open_channel_noti(TcorePlugin *plugin, TelSatOpenChannelTlv *open_channel_tlv);
GVariant *sat_manager_close_channel_noti(TcorePlugin *plugin, TelSatCloseChannelTlv *close_channel_tlv);
GVariant *sat_manager_receive_data_noti(TcorePlugin *plugin, TelSatReceiveChannelTlv *receive_data_tlv);
GVariant *sat_manager_send_data_noti(TcorePlugin *plugin, TelSatSendChannelTlv *send_data_tlv);
GVariant *sat_manager_get_channel_status_noti(TcorePlugin *plugin, TelSatGetChannelStatusTlv *get_channel_status_tlv);
GVariant *sat_manager_refresh_noti(TcorePlugin *plugin, TelSatRefreshTlv *refresh_tlv);
void sat_manager_more_time_noti(TcorePlugin *plugin, TelSatMoreTimeTlv *more_time_tlv);
GVariant* sat_manager_send_dtmf_noti(TcorePlugin *plugin, TelSatSendDtmfTlv *send_dtmf_tlv);
GVariant* sat_manager_launch_browser_noti(TcorePlugin *plugin, TelSatLaunchBrowserTlv *launch_browser_tlv);
GVariant* sat_manager_provide_local_info_noti(TcorePlugin *plugin, TcorePlugin *comm_plugin, TelSatProvideLocalInfoTlv *provide_local_info_tlv);
GVariant* sat_manager_language_notification_noti(TcorePlugin *plugin, TelSatLanguageNotificationTlv *language_notification_tlv);
gboolean sat_manager_processing_unsupport_proactive_command(TcorePlugin *plugin, TelSatUnsupportCommandTlv *unsupport_tlv);
gboolean sat_manager_handle_sat_ui_launch_fail(TcorePlugin *plugin, TelSatNotiProactiveData *p_ind);

#endif /* SAT_MANAGER_H_ */
