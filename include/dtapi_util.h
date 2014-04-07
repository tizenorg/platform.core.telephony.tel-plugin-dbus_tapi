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

#pragma once

#include <glib.h>
#include <tcore.h>
#include <gio/gio.h>

#define dtapi_return_error(ivc,error,message) \
	do { \
		err("%s", message); \
		g_dbus_method_invocation_return_error(ivc, \
		G_DBUS_ERROR, error, message); \
	} while (0)

typedef struct {
	void *interface_object;
	GDBusMethodInvocation *invocation;
	char user_data[0]; /* Additional user data base pointer */
} DbusRespCbData;

GVariant *dtapi_create_empty_variant(const gchar *format_string); /* We should free the variant after use */
inline DbusRespCbData *dtapi_create_resp_cb_data(void *interface_object,
	GDBusMethodInvocation *invocation, void *user_data, unsigned int ud_len);
inline const char *dtapi_get_cp_name_by_object_path(const char *object_path);
gboolean dtapi_check_access_control(GDBusMethodInvocation *invoc,
	const char *label, const char *perm);
