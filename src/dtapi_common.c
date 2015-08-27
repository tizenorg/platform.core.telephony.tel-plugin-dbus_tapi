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
#include <stdlib.h>
#include <glib.h>

#include <tcore.h>
#include <plugin.h>
#include <communicator.h>
#include <server.h>
#include <user_request.h>

#include <libxml/xmlmemory.h>
#include <libxml/parser.h>
#include <libxml/tree.h>

#include <cynara-creds-gdbus.h>
#include <cynara-session.h>

#include "generated-code.h"
#include "dtapi_common.h"

#define PERM_WRITE	"w"
#define PERM_EXECUTE	"x"
#define TELEPHONY_PRIVILEGE		"http://tizen.org/privilege/telephony"
#define TELEPHONY_ADMIN_PRIVILEGE	"http://tizen.org/privilege/telephony.admin"

static void _free_hook(UserRequest *ur)
{
	struct dbus_request_info *user_info;

	user_info = (struct dbus_request_info *)tcore_user_request_ref_user_info(ur);
	if (user_info)
		free(user_info);
}

char *dbus_plugin_get_cp_name_by_object_path(const char *object_path)
{
	if (!object_path)
		return NULL;

	if (!g_str_has_prefix(object_path, MY_DBUS_PATH))
		return NULL;

	return (char *)object_path + strlen(MY_DBUS_PATH) + 1;
}

UserRequest *dbus_plugin_macro_user_request_new(struct custom_data *ctx,
	void *object, GDBusMethodInvocation *invocation)
{
	UserRequest *ur = NULL;
	char *cp_name;
	struct dbus_request_info *dbus_info;

	cp_name = GET_CP_NAME(invocation);

	dbus_info = calloc(1, sizeof(struct dbus_request_info));
	if (!dbus_info)
		return NULL;

	ur = tcore_user_request_new(ctx->comm, cp_name);

	dbus_info->interface_object = object;
	dbus_info->invocation = invocation;

	tcore_user_request_set_user_info(ur, dbus_info);
	tcore_user_request_set_free_hook(ur, _free_hook);

	return ur;
}

gboolean check_access_control(cynara *p_cynara, GDBusMethodInvocation *invoc, const char *label, const char *perm)
{
	GDBusConnection *conn;
	const char *sender_unique_name;
	pid_t pid;
	int ret;
	int result = FALSE;
	/* For cynara */
	gchar *client_smack = NULL;
	char *client_session = NULL;
	gchar *uid_string = NULL;
	const char *privilege = NULL;

	if (!p_cynara) {
		warn("access control denied (fail to get cynara handle)");
		goto OUT;
	}

	conn = g_dbus_method_invocation_get_connection(invoc);
	if (!conn) {
		warn("access control denied (no connection info)");
		goto OUT;
	}

	sender_unique_name = g_dbus_method_invocation_get_sender(invoc);

	/* Get PID */
	ret = cynara_creds_gdbus_get_pid(conn, sender_unique_name, &pid);
	if (ret != CYNARA_API_SUCCESS) {
		warn("access control denied (fail to get pid). ret = %d", ret);
		goto OUT;
	}

	/* Get UID */
	ret = cynara_creds_gdbus_get_user(conn, sender_unique_name, USER_METHOD_DEFAULT, &uid_string);
	if (ret != CYNARA_API_SUCCESS) {
		warn("access control denied (fail to get uid for cynara). ret = %d", ret);
		goto OUT;
	}

	/* Get Smack label */
	ret = cynara_creds_gdbus_get_client(conn, sender_unique_name, CLIENT_METHOD_DEFAULT, &client_smack);
	if (ret != CYNARA_API_SUCCESS) {
		warn("access control denied (fail to get smack for cynara). ret = %d", ret);
		goto OUT;
	}

	dbg("sender: %s pid = %u uid = %s smack = %s", sender_unique_name, pid, uid_string, client_smack);

	client_session = cynara_session_from_pid(pid);
	if (!client_session) {
		warn("access control denied (fail to get cynara client session)");
		goto OUT;
	}

	if (g_strrstr(perm, PERM_WRITE) == NULL && g_strrstr(perm, PERM_EXECUTE) == NULL)
		privilege = TELEPHONY_PRIVILEGE;
	else
		privilege = TELEPHONY_ADMIN_PRIVILEGE;

	ret = cynara_check(p_cynara, client_smack, client_session, uid_string, privilege);
	if (ret != CYNARA_API_ACCESS_ALLOWED)
		warn("pid(%u) access (%s - %s) denied(%d)", pid, label, perm, ret);
	else
		result = TRUE;
OUT:
	if (result == FALSE) {
		g_dbus_method_invocation_return_error(invoc,
				G_DBUS_ERROR,
				G_DBUS_ERROR_ACCESS_DENIED,
				"No access rights");
	}
	free(client_session);
	g_free(client_smack);
	g_free(uid_string);

	return result;
}

enum dbus_tapi_sim_slot_id get_sim_slot_id_by_cp_name(const char *cp_name)
{
	if (g_str_has_suffix(cp_name , "0"))
		return SIM_SLOT_PRIMARY;
	else if (g_str_has_suffix(cp_name , "1"))
		return SIM_SLOT_SECONDARY;
	else if (g_str_has_suffix(cp_name , "2"))
		return SIM_SLOT_TERTIARY;
	return SIM_SLOT_PRIMARY;
}

gboolean dbus_plugin_util_load_xml(char *docname, char *groupname, void **i_doc, void **i_root_node)
{
	xmlDocPtr *doc = (xmlDocPtr *)i_doc;
	xmlNodePtr *root_node = (xmlNodePtr *)i_root_node;

	dbg("docname:%s, groupname:%s", docname, groupname);

	*doc = xmlParseFile(docname);
	if (*doc) {
		*root_node = xmlDocGetRootElement(*doc);
		if (*root_node) {
			dbg("*root_node->name:%s", (*root_node)->name);
			if (0 == xmlStrcmp((*root_node)->name, (const xmlChar *) groupname)) {
				*root_node = (*root_node)->xmlChildrenNode;
				return TRUE;
			}
			*root_node = NULL;
		}
	}
	xmlFreeDoc(*doc);
	*doc = NULL;
	err("Cannot parse doc(%s)", docname);
	return FALSE;
}

void dbus_plugin_util_unload_xml(void **i_doc, void **i_root_node)
{
	xmlDocPtr *doc = (xmlDocPtr *)i_doc;
	xmlNodePtr *root_node = (xmlNodePtr *)i_root_node;

	dbg("unloading XML");
	if (doc && *doc) {
		xmlFreeDoc(*doc);
		*doc = NULL;
		if (root_node)
			*root_node = NULL;
	}
}

TReturn dtapi_dispatch_request_ex(struct custom_data *ctx,
	void *object, GDBusMethodInvocation *invocation,
	enum tcore_request_command req_command,
	void *req_data, unsigned int req_data_len)
{
	UserRequest *ur;
	TReturn ret;

	ur = MAKE_UR(ctx, object, invocation);

	if (req_data_len)
		tcore_user_request_set_data(ur, req_data_len, req_data);
	tcore_user_request_set_command(ur, req_command);

	ret = tcore_communicator_dispatch_request(ctx->comm, ur);
	if (ret != TCORE_RETURN_SUCCESS) {
		err("tcore_communicator_dispatch_request() : (0x%x)", ret);

		tcore_user_request_unref(ur);
	}

	return ret;
}

void dtapi_dispatch_request(struct custom_data *ctx,
	void *object, GDBusMethodInvocation *invocation,
	enum tcore_request_command req_command,
	void *req_data, unsigned int req_data_len)
{
	TReturn ret;

	ret = dtapi_dispatch_request_ex(ctx, object, invocation,
		req_command, req_data, req_data_len);
	if (ret != TCORE_RETURN_SUCCESS) {
		err("dtapi_dispatch_request_ex() : (0x%x)", ret);

		FAIL_RESPONSE(invocation, DEFAULT_MSG_REQ_FAILED);
	}
}
