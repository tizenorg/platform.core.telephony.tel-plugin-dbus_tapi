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
#include <pthread.h>
#include <unistd.h>
#include <stdlib.h>
#include <time.h>
#include <glib.h>

#include <tcore.h>
#include <server.h>
#include <plugin.h>
#include <hal.h>
#include <communicator.h>
#include <storage.h>
#include <queue.h>
#include <user_request.h>
#include <co_network.h>
#include <co_sim.h>
#include <co_ps.h>

#include <TapiCommon.h>
#include <ITapiPS.h>

#include "tel_cs_conn.h"
#include "common.h"
#include "ts_common.h"
#include "ts_noti.h"
#include "ts_svr_req.h"
#include "modules.h"
#include "sat_mgr.h"

static TReturn send_response(Communicator *comm, UserRequest *ur, enum tcore_response_command command, unsigned int data_len, const void *data)
{
	struct custom_data *ctx = NULL;
	const struct tcore_user_info *ui;
	enum tcore_request_command req_cmd;

	ctx = tcore_communicator_ref_user_data(comm);
	if (!ctx) {
		dbg("user_data is NULL");
		return FALSE;
	}

	ui = tcore_user_request_ref_user_info(ur);
	req_cmd = tcore_user_request_get_command(ur);

	dbg("appname = [%s], Response Command = [0x%x], requested Command = [0x%x], data_len = %d", ui->appname, command, req_cmd, data_len);

	switch(command & (TCORE_RESPONSE | 0x0FF00000))
	{
		case TRESP_CALL:
			return dbus_response_call(ctx, ur, ui->appname, command, data_len, data);
			break;
		case TRESP_SS:
			return dbus_response_ss(ctx, ur, ui->appname, command, data_len, data);
			break;
		case TRESP_PS:
			return dbus_response_gprs(ctx, ur, ui->appname, command, data_len, data);
			break;
		case TRESP_SIM:
		case TRESP_SAP:
		case TRESP_PHONEBOOK:
			return dbus_response_sim(ctx, ur, ui->appname, command, data_len, data);
			break;
		case TRESP_NETWORK:
			return dbus_response_network(ctx, ur, ui->appname, command, data_len, data);
			break;
		case TRESP_MODEM:
			return dbus_response_power(ctx, ur, ui->appname, command, data_len, data);
			break;
		case TRESP_SMS:
			return dbus_response_sms(ctx, ur, ui->appname, command, data_len, data);
			break;
		case TRESP_SAT:
			return dbus_response_sat(ctx, ur, ui->appname, command, data_len, data);
			break;
		case TRESP_CUSTOM:
			return dbus_response_productivity(ctx, ur, ui->appname, command, data_len, data);
			break;
		default:
			dbg("[ERR] Not handled this response(0x%x) in send_response()", command);
			break;
	}

	return FALSE;
}

static TReturn send_notification(Communicator *comm, CoreObject *source, enum tcore_notification_command command, unsigned int data_len, const void *data)
{
	struct custom_data *ctx = NULL;

	dbg("notification !!! (command = 0x%x, data_len = %d)", command, data_len);

	ctx = tcore_communicator_ref_user_data(comm);
	if (!ctx) {
		dbg("user_data is NULL");
		return FALSE;
	}

	switch(command & (TCORE_NOTIFICATION | 0x0FF00000))
	{
		case TNOTI_CALL:
			return dbus_notification_call(ctx, source, command, data_len, data);
			break;
		case TNOTI_PS:
			return dbus_notification_gprs(ctx, source, command, data_len, data);
			break;
		case TNOTI_SIM:
		case TNOTI_SAP:
		case TNOTI_PHONEBOOK:
			return dbus_notification_sim(ctx, source, command, data_len, data);
			break;
		case TNOTI_NETWORK:
			return dbus_notification_network(ctx, source, command, data_len, data);
			break;
		case TNOTI_SERVER:
			return TRUE;
		case TNOTI_MODEM:
			return dbus_notification_power(ctx, source, command, data_len, data);
			break;
		case TNOTI_SMS:
			return dbus_notification_sms(ctx, source, command, data_len, data);
			break;
		case TNOTI_SAT:
			return dbus_notification_sat(ctx, source, command, data_len, data);
			break;
		case TNOTI_CUSTOM:
			return dbus_notification_productivity(ctx, source, command, data_len, data);
			break;
		default:
			dbg("[ERR] Not handled this notification(0x%x) in send_notification()", command);
			break;
	}

	return FALSE;
}

struct tcore_communitor_operations ops = {
	.send_response = send_response,
	.send_notification = send_notification,
};

static gboolean on_load()
{
	dbg("i'm load!");

	return TRUE;
}

static gboolean on_init(TcorePlugin *p)
{
	Communicator *comm;
	struct custom_data *data;

	if (!p)
		return FALSE;

	dbg("i'm init!");

	data = calloc(sizeof(struct custom_data), 1);
	if (!data)
		return FALSE;

	data->plugin = p;

	comm = tcore_communicator_new(p, "legacy_dbus", &ops);
	tcore_communicator_link_user_data(comm, data);

	data->comm = comm;
	data->server = tcore_plugin_ref_server(p);
	data->strg = tcore_server_find_storage(data->server, "vconf");

	sat_mgr_init_cmd_queue(data);
	ts_init_delivery_system(data);
	ts_register_dbus_factory(data);

	tcore_storage_set_int(data->strg, STORAGE_KEY_TELEPHONY_EVENT_SYSTEM_READY, STORAGE_VALUE_ON); /* event_system_ready on */

	return TRUE;
}

static void on_unload(TcorePlugin *p)
{
	struct custom_data *data;
	Communicator *comm;

	if (!p)
		return;

	dbg("i'm unload");

	comm = tcore_server_find_communicator(tcore_plugin_ref_server(p), "legacy_dbus");
	if (!comm)
		return;

	data = tcore_communicator_ref_user_data(comm);
	if (!data)
		return;

	free(data);
}

struct tcore_plugin_define_desc plugin_define_desc =
{
	.name = "DBUS_COMMUNICATOR",
	.priority = TCORE_PLUGIN_PRIORITY_HIGH,
	.version = 1,
	.load = on_load,
	.init = on_init,
	.unload = on_unload
};
