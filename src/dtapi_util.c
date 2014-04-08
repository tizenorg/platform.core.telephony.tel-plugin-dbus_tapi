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

#include "dtapi_util.h"

#include <security-server.h>

GVariant *dtapi_create_empty_variant(const gchar *format_string)
{
	GVariantBuilder builder;

	g_variant_builder_init(&builder, G_VARIANT_TYPE(format_string));
	return g_variant_builder_end(&builder);
}

inline DbusRespCbData *dtapi_create_resp_cb_data(void *interface_object,
	GDBusMethodInvocation *invocation, void *user_data, unsigned int ud_len)
{
	DbusRespCbData *rsp_cb_data = tcore_malloc0(sizeof(DbusRespCbData) + ud_len);
	rsp_cb_data->interface_object = interface_object;
	rsp_cb_data->invocation = invocation;
	if ((user_data != NULL) && (ud_len > 0))
		memcpy(rsp_cb_data->user_data, user_data, ud_len);
	return rsp_cb_data;
}

inline const char *dtapi_get_cp_name_by_object_path(const char *object_path)
{
	if (!object_path)
		return NULL;

	if (!g_str_has_prefix(object_path, TELEPHONY_OBJECT_PATH)) {
		return NULL;
	}

	return object_path + strlen(TELEPHONY_OBJECT_PATH) + 1;
}

gboolean dtapi_check_access_control(GDBusMethodInvocation *invoc,
	const char *label, const char *perm)
{
	GDBusConnection *conn;
	GVariant *result_pid;
	GVariant *param;
	GError *error = NULL;
	const char *sender;
	unsigned int pid;
	int ret;
	int result = FALSE;

	conn = g_dbus_method_invocation_get_connection(invoc);
	if (conn == NULL) {
		err("access control denied(no connection info)");
		goto OUT;
	}

	sender = g_dbus_method_invocation_get_sender(invoc);
	dbg("sender: %s", sender);

	param = g_variant_new("(s)", sender);
	if (param == NULL) {
		err("access control denied(sender info fail)");
		goto OUT;
	}

	result_pid = g_dbus_connection_call_sync (conn, "org.freedesktop.DBus",
				"/org/freedesktop/DBus",
				"org.freedesktop.DBus",
				"GetConnectionUnixProcessID",
				param, NULL,
				G_DBUS_CALL_FLAGS_NONE, -1, NULL, &error);

	if (error) {
		err("access control denied (dbus error: %d(%s))",
					error->code, error->message);
		g_error_free (error);
		goto OUT;
	}

	if (result_pid == NULL) {
		err("access control denied(fail to get pid)");
		goto OUT;
	}

	g_variant_get(result_pid, "(u)", &pid);
	g_variant_unref(result_pid);

	dbg ("pid = %u", pid);

	// ret = security_server_check_privilege_by_pid(pid, label, perm);
	ret = 0;
	if (ret != SECURITY_SERVER_API_SUCCESS) {
#ifdef ENABLE_CHECK_PRIVILEGE
		err("access control(%s - %s) denied(%d)", label, perm, ret);
#else
		dbg("access control(%s - %s) denied(%d)", label, perm, ret);
		result = TRUE;
#endif
	}
	else
		result = TRUE;

OUT:

	if (!result)
		dtapi_return_error(invoc, G_DBUS_ERROR_ACCESS_DENIED, "Access denied");

	return result;
}
