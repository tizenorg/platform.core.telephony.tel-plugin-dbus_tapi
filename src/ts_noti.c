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

#include <string.h>
#include <stdio.h>
#include <sys/types.h>
#include <pwd.h>

#include <dbus/dbus.h>
#include <glib.h>

#include <tcore.h>

#include <TelDefines.h>
#include <TapiCommon.h>
#include <TapiUtility.h>

#include "common.h"
#include "ts_utility.h"
#include "ts_common.h"
#include "ts_noti.h"

static int _ts_send_msg5_event_dest(DBusConnection *EvtDeliveryHandle, const char *destination, const char auto_start, const char *event_string, int group,
		int type, int requestId, int Status, const gchar *encoded_data)
{
	DBusMessage *msg = NULL;
	DBusMessageIter args;
	dbus_uint32_t serial = 0;

	if (EvtDeliveryHandle == NULL) {
		dbg("Evnet Delievery Handle is not available");
		return -1;
	}

	msg = dbus_message_new_signal(TS_SIGNAL_OBJPATH, event_string, TS_SIGNAL_MEMBER_ARG5);
	if (NULL == msg) {
		dbg("Message is NULL");
		return -1;
	}

	if (destination != NULL) {
		if (strlen(destination) > 0) {
			dbus_message_set_destination(msg, destination);
		}
	}
	else {
		//dbg("This signal is going to be broadcated.");
	}

	if (auto_start == TRUE)
		dbus_message_set_auto_start(msg, auto_start);

	dbus_message_iter_init_append(msg, &args);
	dbus_message_iter_append_basic(&args, DBUS_TYPE_INT32, &group);
	dbus_message_iter_append_basic(&args, DBUS_TYPE_INT32, &type);
	dbus_message_iter_append_basic(&args, DBUS_TYPE_INT32, &requestId);
	dbus_message_iter_append_basic(&args, DBUS_TYPE_INT32, &Status);
	dbus_message_iter_append_basic(&args, DBUS_TYPE_STRING, &encoded_data);

	if (!dbus_connection_send(EvtDeliveryHandle, msg, &serial)) {
		dbg("send failed");
		dbus_message_unref(msg);
		return -1;
	}

	dbus_connection_flush(EvtDeliveryHandle);
	dbus_message_unref(msg);
	return 0;
}

/**
 * This function registers with Notification Manger.
 *
 * @param[in]	None
 * @param[out]	None
 * @return		TS_BOOL.
 *
 */

void ts_init_delivery_system(struct custom_data *data)
{
	DBusConnection *conn = NULL;
	DBusError err;
	int ret;

	dbg("Func Entrance");

	dbus_error_init(&err);
	dbus_threads_init_default();

	conn = dbus_bus_get_private(DBUS_BUS_SYSTEM, &err);

	if (dbus_error_is_set(&err)) {
		dbg("failed to get event handle");
		dbus_error_free(&err);
		return;
	}

	ret = dbus_bus_request_name(conn, TS_SIGNAL_SENDER, DBUS_NAME_FLAG_REPLACE_EXISTING, &err);
	dbg("conn = %p", conn);

	if (DBUS_REQUEST_NAME_REPLY_PRIMARY_OWNER == ret) {
		data->EvtDeliveryHandle = conn;
		dbg("Evnet Delievery System init success");
	}
	else {
		dbg("Evnet Delievery System init failed, %s", err.message);
	}

	dbus_error_free(&err);
}

TS_BOOL ts_delivery_event(DBusConnection *EvtDeliveryHandle, int group, int type, const char *dest_name, int requestId, int Status, int data_length, void *data)
{
	int error_code = 0x00;
	TS_BOOL ret = FALSE;
	gchar *encoded_data = NULL;
	char event_string[100];
	int encoded_len = 0x00;

	TAPI_GET_EVENT_NAME(type, event_string);

	if (strncmp(event_string, "Telephony.Unknown", 17) != 0) {
		/*      converts telephony Event Data in binary format to a newly allocated,
		 *      zero-terminated Base-64 encoded string representing data. So that the data can be
		 *      tranported over any tarnsport channel. Like dbus.
		 */
		if (data_length > 0 && data != NULL) {
			encoded_data = g_base64_encode(data, data_length);
			if (encoded_data == NULL) {
				dbg("g_base64_encode: Failed to Enocde the Data.");
				return FALSE;
			}
		}

		if (encoded_data != NULL)
			encoded_len = strlen(encoded_data);
		else
			encoded_data = "";

		switch (type & 0xFF0000) {
			case TAPI_EVENT_TYPE_NOTIFICATION:
				switch (type) {
					case TAPI_EVENT_SAT_SETUP_MENU_IND:
					case TAPI_EVENT_SAT_DISPLAY_TEXT_IND:
					case TAPI_EVENT_SAT_GET_INKEY_IND:
					case TAPI_EVENT_SAT_GET_INPUT_IND:
					case TAPI_EVENT_SAT_PLAY_TONE_IND:
					case TAPI_EVENT_SAT_SELECT_ITEM_IND:
					case TAPI_EVENT_SAT_UI_SEND_SMS_IND:
					case TAPI_EVENT_SAT_UI_SEND_SS_IND:
					case TAPI_EVENT_SAT_UI_SEND_USSD_IND:
					case TAPI_EVENT_SAT_UI_SEND_DTMF_IND:
					case TAPI_EVENT_SAT_UI_SETUP_CALL_IND:
					case TAPI_EVENT_SAT_UI_PLAY_TONE_IND:
					case TAPI_EVENT_SAT_UI_REFRESH_IND:
					case TAPI_EVENT_SAT_UI_LAUNCH_BROWSER_IND:
					case TAPI_EVENT_SAT_PROVIDE_LOCAL_INFO_IND:
					case TAPI_EVENT_SAT_SETUP_IDLE_MODE_TEXT_IND:
						break;
					default:
						error_code = _ts_send_msg5_event_dest(EvtDeliveryHandle, NULL, FALSE, event_string, group, type, requestId, Status,
								encoded_data);
						break;
				}
				break;

			case TAPI_EVENT_TYPE_CONFIRMATION:
				dbg("Reqeust ID=%d Destnation=[%s] Event=[%s]", requestId, dest_name, event_string);
				error_code = _ts_send_msg5_event_dest(EvtDeliveryHandle, dest_name, FALSE, event_string, group, type, requestId, Status,
						encoded_data);
				break;

			default:
				dbg("Unknown event type = 0x%x", type & 0xFF0000);
				break;
		}

		if (error_code < 0) {
			dbg("Failed to publish [%s] and error code [%d]", event_string, error_code);
		}
		else {
			ret = TRUE;
			dbg("Published [%s]...with request id(%d)", event_string, requestId);
		}

		if (NULL != encoded_data && encoded_len > 0) {
			g_free(encoded_data);
		}
	}
	else {
		dbg("New Method for EVENT fetching failed");
	}

	return ret;
}
/* EOF */

