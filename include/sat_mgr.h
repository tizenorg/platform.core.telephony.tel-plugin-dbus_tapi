#ifndef SAT_MGR_H_
#define SAT_MGR_H_

#include <tcore.h>
#include "common.h"
#include "type/sat.h"

typedef union {
	struct tel_sat_display_text_tlv displayTextInd; /**<	Parsed proactive command info from TLV to Telephony data type - display text	*/
	struct tel_sat_get_inkey_tlv getInkeyInd; /**<	Parsed proactive command info from TLV to Telephony data type - getInkey	*/
	struct tel_sat_get_input_tlv getInputInd; /**<	Parsed proactive command info from TLV to Telephony data type - getInput	*/
	struct tel_sat_setup_menu_tlv setupMenuInd; /**<	Parsed proactive command info from TLV to Telephony data type - setup menu	*/
	struct tel_sat_select_item_tlv selectItemInd; /**<	Parsed proactive command info from TLV to Telephony data type - select item	*/
} sat_mgr_proactive_cmd_data;

/**
 * This structure defines the Command Queue Info.
 */
struct sat_mgr_cmd_q {
	TelSatCommandType_t cmd_type; /**<Type of Command*/
	int cmd_id; /**<Command Id*/
	sat_mgr_proactive_cmd_data cmd_data; /**<Proactive Cmd Ind Info*/
};


/*================================================================================================*/

void sat_mgr_init_cmd_queue(struct custom_data *ctx);
void sat_mgr_setup_menu_noti(struct custom_data *ctx, struct tel_sat_setup_menu_tlv* setup_menu_tlv);
void sat_mgr_display_text_noti(struct custom_data *ctx, struct tel_sat_display_text_tlv* display_text_tlv);
void sat_mgr_select_item_noti(struct custom_data *ctx, struct tel_sat_select_item_tlv* select_item_tlv);
void sat_mgr_get_inkey_noti(struct custom_data *ctx, struct tel_sat_get_inkey_tlv* get_inkey_tlv);
void sat_mgr_get_input_noti(struct custom_data *ctx, struct tel_sat_get_input_tlv* get_input_tlv);
TapiResult_t sat_mgr_handle_app_exec_result(struct custom_data *ctx, TcorePlugin *p, char *conn_name, TelSatAppsRetInfo_t* app_result);
TapiResult_t sat_mgr_handle_user_confirm(struct custom_data *ctx, TcorePlugin *p, char *conn_name,	TelSatUiUserConfirmInfo_t *cnf, unsigned char* additional_data, unsigned short ad_data_length);
#endif /* SAT_MGR_H_ */
