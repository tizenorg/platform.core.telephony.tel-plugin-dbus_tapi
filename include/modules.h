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

#ifndef __MODULES_H__
#define __MODULES_H__

__BEGIN_DECLS

void dbus_request_call(struct custom_data *ctx, TcorePlugin *plugin, int tapi_service_function,
		GArray* in_param1, GArray* in_param2, GArray* in_param3, GArray* in_param4, GArray** out_param1,
		GArray** out_param2, GArray** out_param3, GArray** out_param4, GError** error);
void dbus_request_cdmadata(struct custom_data *ctx, TcorePlugin *plugin, int tapi_service_function,
		GArray* in_param1, GArray* in_param2, GArray* in_param3, GArray* in_param4, GArray** out_param1,
		GArray** out_param2, GArray** out_param3, GArray** out_param4, GError** error);
void dbus_request_cfg(struct custom_data *ctx, TcorePlugin *plugin, int tapi_service_function,
		GArray* in_param1, GArray* in_param2, GArray* in_param3, GArray* in_param4, GArray** out_param1,
		GArray** out_param2, GArray** out_param3, GArray** out_param4, GError** error);
void dbus_request_gprs(struct custom_data *ctx, TcorePlugin *plugin, int tapi_service_function, GArray* in_param1,
		GArray* in_param2, GArray* in_param3, GArray* in_param4, GArray** out_param1, GArray** out_param2,
		GArray** out_param3, GArray** out_param4, GError** error);
void dbus_request_gps(struct custom_data *ctx, TcorePlugin *plugin, int tapi_service_function,
		GArray* in_param1, GArray* in_param2, GArray* in_param3, GArray* in_param4, GArray** out_param1,
		GArray** out_param2, GArray** out_param3, GArray** out_param4, GError** error);
void dbus_request_misc(struct custom_data *ctx, TcorePlugin *plugin, int tapi_service_function,
		GArray* in_param1, GArray* in_param2, GArray* in_param3, GArray* in_param4, GArray** out_param1,
		GArray** out_param2, GArray** out_param3, GArray** out_param4, GError** error);
void dbus_request_network(struct custom_data *ctx, TcorePlugin *plugin, int tapi_service_function,
		GArray* in_param1, GArray* in_param2, GArray* in_param3, GArray* in_param4, GArray** out_param1,
		GArray** out_param2, GArray** out_param3, GArray** out_param4, GError** error);
void dbus_request_omadm(struct custom_data *ctx, TcorePlugin *plugin, int tapi_service_function,
		GArray* in_param1, GArray* in_param2, GArray* in_param3, GArray* in_param4, GArray** out_param1,
		GArray** out_param2, GArray** out_param3, GArray** out_param4, GError** error);
void dbus_request_power(struct custom_data *ctx, TcorePlugin *plugin, int tapi_service_function,
		GArray* in_param1, GArray* in_param2, GArray* in_param3, GArray* in_param4, GArray** out_param1,
		GArray** out_param2, GArray** out_param3, GArray** out_param4, GError** error);
void dbus_request_productivity(struct custom_data *ctx, TcorePlugin *plugin, int tapi_service_function,
		GArray* in_param1, GArray* in_param2, GArray* in_param3, GArray* in_param4, GArray** out_param1,
		GArray** out_param2, GArray** out_param3, GArray** out_param4, GError** error);
void dbus_request_sat(struct custom_data *ctx, TcorePlugin *plugin, int tapi_service_function,
		GArray* in_param1, GArray* in_param2, GArray* in_param3, GArray* in_param4, GArray** out_param1,
		GArray** out_param2, GArray** out_param3, GArray** out_param4, GError** error);
void dbus_request_sim(struct custom_data *ctx, TcorePlugin *plugin, int tapi_service_function, GArray* in_param1,
		GArray* in_param2, GArray* in_param3, GArray* in_param4, GArray** out_param1, GArray** out_param2,
		GArray** out_param3, GArray** out_param4, GError** error);
void dbus_request_sms(struct custom_data *ctx, TcorePlugin *plugin, int tapi_service_function, GArray* in_param1,
		GArray* in_param2, GArray* in_param3, GArray* in_param4, GArray** out_param1, GArray** out_param2,
		GArray** out_param3, GArray** out_param4, GError** error);
void dbus_request_sound(struct custom_data *ctx, TcorePlugin *plugin, int tapi_service_function, GArray* in_param1,
		GArray* in_param2, GArray* in_param3, GArray* in_param4, GArray** out_param1, GArray** out_param2,
		GArray** out_param3, GArray** out_param4, GError** error);
void dbus_request_ss(struct custom_data *ctx, TcorePlugin *plugin, int tapi_service_function, GArray* in_param1,
		GArray* in_param2, GArray* in_param3, GArray* in_param4, GArray** out_param1, GArray** out_param2,
		GArray** out_param3, GArray** out_param4, GError** error);
void dbus_request_util(struct custom_data *ctx, TcorePlugin *plugin, int tapi_service_function, GArray* in_param1,
		GArray* in_param2, GArray* in_param3, GArray* in_param4, GArray** out_param1, GArray** out_param2,
		GArray** out_param3, GArray** out_param4, GError** error);

TReturn dbus_response_call(struct custom_data *ctx, UserRequest *ur, const char *appname,
		enum tcore_response_command command, unsigned int data_len, const void *data);
TReturn dbus_response_cdmadata(struct custom_data *ctx, UserRequest *ur, const char *appname,
		enum tcore_response_command command, unsigned int data_len, const void *data);
TReturn dbus_response_cfg(struct custom_data *ctx, UserRequest *ur, const char *appname,
		enum tcore_response_command command, unsigned int data_len, const void *data);
TReturn dbus_response_gprs(struct custom_data *ctx, UserRequest *ur, const char *appname,
		enum tcore_response_command command, unsigned int data_len, const void *data);
TReturn dbus_response_gps(struct custom_data *ctx, UserRequest *ur, const char *appname,
		enum tcore_response_command command, unsigned int data_len, const void *data);
TReturn dbus_response_misc(struct custom_data *ctx, UserRequest *ur, const char *appname,
		enum tcore_response_command command, unsigned int data_len, const void *data);
TReturn dbus_response_network(struct custom_data *ctx, UserRequest *ur, const char *appname,
		enum tcore_response_command command, unsigned int data_len, const void *data);
TReturn dbus_response_omadm(struct custom_data *ctx, UserRequest *ur, const char *appname,
		enum tcore_response_command command, unsigned int data_len, const void *data);
TReturn dbus_response_power(struct custom_data *ctx, UserRequest *ur, const char *appname,
		enum tcore_response_command command, unsigned int data_len, const void *data);
TReturn dbus_response_productivity(struct custom_data *ctx, UserRequest *ur, const char *appname,
		enum tcore_response_command command, unsigned int data_len, const void *data);
TReturn dbus_response_sat(struct custom_data *ctx, UserRequest *ur, const char *appname,
		enum tcore_response_command command, unsigned int data_len, const void *data);
TReturn dbus_response_sim(struct custom_data *ctx, UserRequest *ur, const char *appname,
		enum tcore_response_command command, unsigned int data_len, const void *data);
TReturn dbus_response_sms(struct custom_data *ctx, UserRequest *ur, const char *appname,
		enum tcore_response_command command, unsigned int data_len, const void *data);
TReturn dbus_response_sound(struct custom_data *ctx, UserRequest *ur, const char *appname,
		enum tcore_response_command command, unsigned int data_len, const void *data);
TReturn dbus_response_ss(struct custom_data *ctx, UserRequest *ur, const char *appname,
		enum tcore_response_command command, unsigned int data_len, const void *data);
TReturn dbus_response_util(struct custom_data *ctx, UserRequest *ur, const char *appname,
		enum tcore_response_command command, unsigned int data_len, const void *data);

TReturn dbus_notification_call(struct custom_data *ctx, CoreObject *source, enum tcore_notification_command command,
		unsigned int data_len, const void *data);
TReturn dbus_notification_cdmadata(struct custom_data *ctx, CoreObject *source, enum tcore_notification_command command,
		unsigned int data_len, const void *data);
TReturn dbus_notification_cfg(struct custom_data *ctx, CoreObject *source, enum tcore_notification_command command,
		unsigned int data_len, const void *data);
TReturn dbus_notification_gprs(struct custom_data *ctx, CoreObject *source, enum tcore_notification_command command,
		unsigned int data_len, const void *data);
TReturn dbus_notification_gps(struct custom_data *ctx, CoreObject *source, enum tcore_notification_command command,
		unsigned int data_len, const void *data);
TReturn dbus_notification_misc(struct custom_data *ctx, CoreObject *source, enum tcore_notification_command command,
		unsigned int data_len, const void *data);
TReturn dbus_notification_network(struct custom_data *ctx, CoreObject *source, enum tcore_notification_command command,
		unsigned int data_len, const void *data);
TReturn dbus_notification_omadm(struct custom_data *ctx, CoreObject *source, enum tcore_notification_command command,
		unsigned int data_len, const void *data);
TReturn dbus_notification_power(struct custom_data *ctx, CoreObject *source, enum tcore_notification_command command,
		unsigned int data_len, const void *data);
TReturn dbus_notification_productivity(struct custom_data *ctx, CoreObject *source, enum tcore_notification_command command,
		unsigned int data_len, const void *data);
TReturn dbus_notification_sat(struct custom_data *ctx, CoreObject *source, enum tcore_notification_command command,
		unsigned int data_len, const void *data);
TReturn dbus_notification_sim(struct custom_data *ctx, CoreObject *source, enum tcore_notification_command command,
		unsigned int data_len, const void *data);
TReturn dbus_notification_sms(struct custom_data *ctx, CoreObject *source, enum tcore_notification_command command,
		unsigned int data_len, const void *data);
TReturn dbus_notification_sound(struct custom_data *ctx, CoreObject *source, enum tcore_notification_command command,
		unsigned int data_len, const void *data);
TReturn dbus_notification_ss(struct custom_data *ctx, CoreObject *source, enum tcore_notification_command command,
		unsigned int data_len, const void *data);
TReturn dbus_notification_util(struct custom_data *ctx, CoreObject *source, enum tcore_notification_command command,
		unsigned int data_len, const void *data);

__END_DECLS

#endif
