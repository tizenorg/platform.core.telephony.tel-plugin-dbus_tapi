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

#ifndef __COMMON_H__
#define __COMMON_H__

#include <glib.h>
#include <glib-object.h>
#include <gio/gio.h>

#include "generated-code.h"

#define AC_MANAGER	"telephony_framework::api_manager"
#define AC_CALL		"telephony_framework::api_call"
#define AC_GPS		"telephony_framework::api_gps"
#define AC_MODEM	"telephony_framework::api_modem"
#define AC_NETWORK	"telephony_framework::api_network"
#define AC_PHONEBOOK	"telephony_framework::api_phonebook"
#define AC_SAP		"telephony_framework::api_sap"
#define AC_SAT		"telephony_framework::api_sat"
#define AC_SIM		"telephony_framework::api_sim"
#define AC_SMS		"telephony_framework::api_sms"
#define AC_SS		"telephony_framework::api_ss"

struct custom_data {
	TcorePlugin *plugin;
	Communicator *comm;
	Server *server;
	guint bus_id;

	GHashTable *objects;
	GDBusObjectManagerServer *manager;
};

typedef struct {
	void *interface_object;
	GDBusMethodInvocation *invocation;
	char user_data[0]; /* Additional user data base pointer */
} DbusRespCbData;

#define GET_CP_NAME(invocation) dbus_plugin_get_cp_name_by_object_path(g_dbus_method_invocation_get_object_path(invocation))
#define MAKE_DBUS_RESP_CB_DATA(data, object, invocation, user_data, user_data_len)  \
	data = g_malloc0(sizeof(DbusRespCbData) + user_data_len); \
	data->interface_object = object; \
	data->invocation = invocation; \
	if ((user_data != NULL) && (user_data_len > 0)) \
		memcpy(data->user_data, user_data, user_data_len);

#define FREE_DBUS_RESP_CB_DATA(data)  g_free(data);

const char *dbus_plugin_get_cp_name_by_object_path(const char *object_path);
gboolean check_access_control(GDBusMethodInvocation *invoc, const char *label, const char *perm);

gboolean dbus_plugin_setup_network_interface(TelephonyObjectSkeleton *object, struct custom_data *ctx);
gboolean dbus_plugin_network_notification(struct custom_data *ctx, CoreObject *source, TelephonyObjectSkeleton *object, enum tcore_notification_command command, unsigned int data_len, const void *data);

gboolean dbus_plugin_setup_sap_interface(TelephonyObjectSkeleton *object, struct custom_data *ctx);
gboolean dbus_plugin_sap_notification(struct custom_data *ctx, CoreObject *source, TelephonyObjectSkeleton *object, enum tcore_notification_command command, unsigned int data_len, const void *data);

gboolean dbus_plugin_setup_phonebook_interface(TelephonyObjectSkeleton *object, struct custom_data *ctx);
gboolean dbus_plugin_phonebook_notification(struct custom_data *ctx, CoreObject *source, TelephonyObjectSkeleton *object, enum tcore_notification_command command, unsigned int data_len, const void *data);

gboolean dbus_plugin_setup_sim_interface(TelephonyObjectSkeleton *object, struct custom_data *ctx);
gboolean dbus_plugin_sim_notification(struct custom_data *ctx, CoreObject *source, TelephonyObjectSkeleton *object, enum tcore_notification_command command, unsigned int data_len, const void *data);

gboolean dbus_plugin_setup_sat_interface(TelephonyObjectSkeleton *object, struct custom_data *ctx);
gboolean dbus_plugin_sat_notification(struct custom_data *ctx, CoreObject *source, TelephonyObjectSkeleton *object, enum tcore_notification_command command, unsigned int data_len, const void *data);

gboolean dbus_plugin_setup_sms_interface(TelephonyObjectSkeleton *object, struct custom_data *ctx);
gboolean dbus_plugin_sms_notification(struct custom_data *ctx, CoreObject *source, TelephonyObjectSkeleton *object, enum tcore_notification_command command, unsigned int data_len, const void *data);

gboolean dbus_plugin_setup_call_interface(TelephonyObjectSkeleton *object, struct custom_data *ctx);
gboolean dbus_plugin_call_notification(struct custom_data *ctx, CoreObject *source, TelephonyObjectSkeleton *object, enum tcore_notification_command command, unsigned int data_len, const void *data);

gboolean dbus_plugin_setup_ss_interface(TelephonyObjectSkeleton *object, struct custom_data *ctx);
gboolean dbus_plugin_ss_notification(struct custom_data *ctx, CoreObject *source, TelephonyObjectSkeleton *object, enum tcore_notification_command command, unsigned int data_len, const void *data);

gboolean dbus_plugin_setup_modem_interface(TelephonyObjectSkeleton *object, struct custom_data *ctx);
gboolean dbus_plugin_modem_notification(struct custom_data *ctx, CoreObject *source, TelephonyObjectSkeleton *object, enum tcore_notification_command command, unsigned int data_len, const void *data);

gboolean dbus_plugin_setup_gps_interface(TelephonyObjectSkeleton *object, struct custom_data *ctx);
gboolean dbus_plugin_gps_notification(struct custom_data *ctx, CoreObject *source, TelephonyObjectSkeleton *object, enum tcore_notification_command command, unsigned int data_len, const void *data);

#endif
