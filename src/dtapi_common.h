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

#ifndef __COMMON_H__
#define __COMMON_H__

#include <glib.h>
#include <gio/gio.h>

#include "type/sim.h"

#include "generated-code.h"

#define MY_DBUS_PATH		"/org/tizen/telephony"
#define MY_DBUS_SERVICE	"org.tizen.telephony"

enum dbus_tapi_sim_slot_id {
	SIM_SLOT_PRIMARY,
	SIM_SLOT_SECONDARY,
	SIM_SLOT_TERTIARY
};

struct cached_data{
	char *cp_name;
	gpointer cached_sat_main_menu;
};

struct custom_data {
	TcorePlugin *plugin;
	Communicator *comm;
	Server *server;

	GHashTable *objects;
	GDBusObjectManagerServer *manager;
	TelephonyManager *mgr;
	enum tel_sim_status sim1_status;
	enum tel_sim_status sim2_status;
	int valid_sim_count;
	GSList *cached_data;

	gboolean name_acquired;
	guint owner_id;
};

struct dbus_request_info {
	void *interface_object;
	GDBusMethodInvocation *invocation;
};

#define DEFAULT_MSG_REQ_FAILED "Request failed"

#define GET_CP_NAME(invocation) dbus_plugin_get_cp_name_by_object_path(g_dbus_method_invocation_get_object_path(invocation))
#define MAKE_UR(ctx, object, invocation) dbus_plugin_macro_user_request_new(ctx, object, invocation)
#define FAIL_RESPONSE(ivc, msg) g_dbus_method_invocation_return_error (ivc, \
	G_DBUS_ERROR, G_DBUS_ERROR_FAILED, msg);

gboolean dtapi_init(TcorePlugin *p);
void dtapi_deinit(TcorePlugin *p);

char *dbus_plugin_get_cp_name_by_object_path(const char *object_path);
UserRequest *dbus_plugin_macro_user_request_new(struct custom_data *ctx, void *object, GDBusMethodInvocation *invocation);

gboolean dbus_plugin_util_load_xml(const char *docname, char *groupname, void **i_doc, void **i_root_node);
void dbus_plugin_util_unload_xml(void **i_doc, void **i_root_node);

void dtapi_dispatch_request(struct custom_data *ctx,
	void *object, GDBusMethodInvocation *invocation,
	enum tcore_request_command req_command,
	void *req_data, unsigned int req_data_len);
TReturn dtapi_dispatch_request_ex(struct custom_data *ctx,
	void *object, GDBusMethodInvocation *invocation,
	enum tcore_request_command req_command,
	void *req_data, unsigned int req_data_len);

gboolean dbus_plugin_setup_network_interface(TelephonyObjectSkeleton *object, struct custom_data *ctx);
gboolean dbus_plugin_network_response(struct custom_data *ctx, UserRequest *ur, struct dbus_request_info *dbus_info, enum tcore_response_command command, unsigned int data_len, const void *data);
gboolean dbus_plugin_network_notification(struct custom_data *ctx, CoreObject *source, TelephonyObjectSkeleton *object, enum tcore_notification_command command, unsigned int data_len, const void *data);

gboolean dbus_plugin_setup_sap_interface(TelephonyObjectSkeleton *object, struct custom_data *ctx);
gboolean dbus_plugin_sap_response(struct custom_data *ctx, UserRequest *ur, struct dbus_request_info *dbus_info, enum tcore_response_command command, unsigned int data_len, const void *data);
gboolean dbus_plugin_sap_notification(struct custom_data *ctx, CoreObject *source, TelephonyObjectSkeleton *object, enum tcore_notification_command command, unsigned int data_len, const void *data);

gboolean dbus_plugin_setup_phonebook_interface(TelephonyObjectSkeleton *object, struct custom_data *ctx);
gboolean dbus_plugin_phonebook_response(struct custom_data *ctx, UserRequest *ur, struct dbus_request_info *dbus_info, enum tcore_response_command command, unsigned int data_len, const void *data);
gboolean dbus_plugin_phonebook_notification(struct custom_data *ctx, CoreObject *source, TelephonyObjectSkeleton *object, enum tcore_notification_command command, unsigned int data_len, const void *data);

gboolean dbus_plugin_setup_sim_interface(TelephonyObjectSkeleton *object, struct custom_data *ctx);
gboolean dbus_plugin_sim_response(struct custom_data *ctx, UserRequest *ur, struct dbus_request_info *dbus_info, enum tcore_response_command command, unsigned int data_len, const void *data);
gboolean dbus_plugin_sim_notification(struct custom_data *ctx, CoreObject *source, TelephonyObjectSkeleton *object, enum tcore_notification_command command, unsigned int data_len, const void *data);

gboolean dbus_plugin_setup_sat_interface(TelephonyObjectSkeleton *object, struct custom_data *ctx);
gboolean dbus_plugin_sat_response(struct custom_data *ctx, UserRequest *ur, struct dbus_request_info *dbus_info, enum tcore_response_command command, unsigned int data_len, const void *data);
gboolean dbus_plugin_sat_notification(struct custom_data *ctx, CoreObject *source, TelephonyObjectSkeleton *object, enum tcore_notification_command command, unsigned int data_len, const void *data);

gboolean dbus_plugin_setup_sms_interface(TelephonyObjectSkeleton *object, struct custom_data *ctx);
gboolean dbus_plugin_sms_response(struct custom_data *ctx, UserRequest *ur, struct dbus_request_info *dbus_info, enum tcore_response_command command, unsigned int data_len, const void *data);
gboolean dbus_plugin_sms_notification(struct custom_data *ctx, CoreObject *source, TelephonyObjectSkeleton *object, enum tcore_notification_command command, unsigned int data_len, const void *data);

gboolean dbus_plugin_setup_call_interface(TelephonyObjectSkeleton *object, struct custom_data *ctx);
gboolean dbus_plugin_call_response(struct custom_data *ctx, UserRequest *ur, struct dbus_request_info *dbus_info, enum tcore_response_command command, unsigned int data_len, const void *data);
gboolean dbus_plugin_call_notification(struct custom_data *ctx, CoreObject *source, TelephonyObjectSkeleton *object, enum tcore_notification_command command, unsigned int data_len, const void *data);

gboolean dbus_plugin_setup_ss_interface(TelephonyObjectSkeleton *object, struct custom_data *ctx);
gboolean dbus_plugin_ss_response(struct custom_data *ctx, UserRequest *ur, struct dbus_request_info *dbus_info, enum tcore_response_command command, unsigned int data_len, const void *data);
gboolean dbus_plugin_ss_notification(struct custom_data *ctx, CoreObject *source, TelephonyObjectSkeleton *object, enum tcore_notification_command command, unsigned int data_len, const void *data);

gboolean dbus_plugin_setup_modem_interface(TelephonyObjectSkeleton *object, struct custom_data *ctx);
gboolean dbus_plugin_modem_response(struct custom_data *ctx, UserRequest *ur, struct dbus_request_info *dbus_info, enum tcore_response_command command, unsigned int data_len, const void *data);
gboolean dbus_plugin_modem_notification(struct custom_data *ctx, CoreObject *source, TelephonyObjectSkeleton *object, enum tcore_notification_command command, unsigned int data_len, const void *data);

gboolean dbus_plugin_setup_oem_interface(TelephonyObjectSkeleton *object, struct custom_data *ctx);
gboolean dbus_plugin_oem_response(struct custom_data *ctx, UserRequest *ur, struct dbus_request_info *dbus_info, enum tcore_response_command command, unsigned int data_len, const void *data);
gboolean dbus_plugin_oem_notification(struct custom_data *ctx, CoreObject *source, TelephonyObjectSkeleton *object, enum tcore_notification_command command, unsigned int data_len, const void *data);

enum dbus_tapi_sim_slot_id get_sim_slot_id_by_cp_name(const char *cp_name);

#endif /* __COMMON_H__ */
