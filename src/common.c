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
#include <dbus/dbus.h>

#include <tcore.h>
#include <plugin.h>
#include <communicator.h>
#include <server.h>
#include <user_request.h>

#include <libxml/xmlmemory.h>
#include <libxml/parser.h>
#include <libxml/tree.h>

#include <cynara-session.h>

#include "generated-code.h"
#include "common.h"


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

	if (!g_str_has_prefix(object_path, MY_DBUS_PATH)) {
		return NULL;
	}

	return (char *)object_path + strlen(MY_DBUS_PATH) + 1;
}

UserRequest *dbus_plugin_macro_user_request_new(struct custom_data *ctx, void *object, GDBusMethodInvocation *invocation)
{
	UserRequest *ur = NULL;
	char *cp_name;
	struct dbus_request_info *dbus_info;

	cp_name = GET_CP_NAME(invocation);
	dbg("cp_name = [%s]", cp_name);

	ur = tcore_user_request_new(ctx->comm, cp_name);

	dbus_info = calloc(1, sizeof(struct dbus_request_info));
	dbus_info->interface_object = object;
	dbus_info->invocation = invocation;

	tcore_user_request_set_user_info(ur, dbus_info);
	tcore_user_request_set_free_hook(ur, _free_hook);

	return ur;
}

gboolean check_access_control (cynara *p_cynara, GDBusMethodInvocation *invoc, const char *label, const char *perm)
{
	GDBusConnection *conn;
	GVariant *result_pid;
	GVariant *param;
	GError *error = NULL;
	const char *sender;
	unsigned int pid;
	int ret;
	int result = FALSE;
	/* For cynara */
	GVariant *result_uid;
	GVariant *result_smack;
	const gchar *unique_name = NULL;
	gchar *client_smack = NULL;
	char *client_session = NULL;
	unsigned int uid;
	gchar *uid_string = NULL;
	const char *privilege = NULL;
	gchar *address = NULL;

	conn = g_dbus_method_invocation_get_connection (invoc);
	if (!conn) {
		warn ("access control denied (no connection info)");
		goto OUT;
	}

	address = g_dbus_address_get_for_bus_sync(G_BUS_TYPE_SYSTEM, NULL, NULL);
	if (!address) {
		warn ("access control denied (fail to get dbus address");
		goto OUT;
	}

	if (!p_cynara) {
		warn ("access control denied (fail to get cynara handle)");
		goto OUT;
	}

	unique_name = g_dbus_connection_get_unique_name(conn);
	if (!unique_name) {
		warn ("access control denied (fail to get unique name)");
		goto OUT;
	}

	sender = g_dbus_method_invocation_get_sender (invoc);

	param = g_variant_new ("(s)", sender);
	if (!param) {
		warn ("access control denied (sender info fail)");
		goto OUT;
	}

	/* Get PID */
	result_pid = g_dbus_connection_call_sync (conn, "org.freedesktop.DBus",
			"/org/freedesktop/DBus",
			"org.freedesktop.DBus",
			"GetConnectionUnixProcessID",
			param, NULL,
			G_DBUS_CALL_FLAGS_NONE, -1, NULL, &error);
	if (error) {
		warn ("access control denied (dbus error: %d(%s))",
				error->code, error->message);
		g_error_free (error);
		goto OUT;
	}

	if (!result_pid) {
		warn ("access control denied (fail to get pid)");
		goto OUT;
	}

	g_variant_get (result_pid, "(u)", &pid);
	g_variant_unref (result_pid);

	/* Get UID */
	result_uid = g_dbus_connection_call_sync (conn, "org.freedesktop.DBus",
			"/org/freedesktop/DBus",
			"org.freedesktop.DBus",
			"GetConnectionUnixUser",
			g_variant_new("(s)", unique_name), G_VARIANT_TYPE("(u)"),
			G_DBUS_CALL_FLAGS_NONE, -1, NULL, &error);
	if (error) {
		warn ("access control denied (dbus error: %d(%s))",
				error->code, error->message);
		g_error_free (error);
		goto OUT;
	}

	if (!result_uid) {
		warn ("access control denied (fail to get uid for cynara)");
		goto OUT;
	}

	g_variant_get (result_uid, "(u)", &uid);
	g_variant_unref (result_uid);
	uid_string = g_strdup_printf("%u", uid);

	/* Get Smack label */
	result_smack = g_dbus_connection_call_sync (conn, "org.freedesktop.DBus",
			"/org/freedesktop/DBus",
			"org.freedesktop.DBus",
			"GetConnectionSmackContext",
			g_variant_new("(s)", unique_name), G_VARIANT_TYPE("(s)"),
			G_DBUS_CALL_FLAGS_NONE, -1, NULL, &error);
	if (error) {
		warn ("access control denied (dbus error: %d(%s))",
				error->code, error->message);
		g_error_free (error);
		goto OUT;
	}
	if (!result_smack) {
		warn ("access control denied (fail to get smack for cynara)");
		goto OUT;
	}
	g_variant_get (result_smack, "(s)", &client_smack);
	g_variant_unref (result_smack);

	dbg ("sender: %s pid = %u uid = %u smack = %s", sender, pid, uid, client_smack);

	client_session = cynara_session_from_pid(pid);
	if (!client_session) {
		warn ("access control denied (fail to get cynara client session)");
		goto OUT;
	}

	if (g_strrstr(perm, "w") == NULL && g_strrstr(perm, "x") == NULL) {
		privilege = "http://tizen.org/privilege/telephony";
	} else {
		privilege = "http://tizen.org/privilege/telephony.admin";
	}

	ret = cynara_check(p_cynara, client_smack, client_session, uid_string, privilege);
	if (ret != CYNARA_API_ACCESS_ALLOWED) {
		warn ("pid(%u) access (%s - %s) denied(%d)", pid, label, perm, ret);
	}
	else
		result = TRUE;
OUT:
	if (result == FALSE) {
		g_dbus_method_invocation_return_error (invoc,
				G_DBUS_ERROR,
				G_DBUS_ERROR_ACCESS_DENIED,
				"No access rights");
	}
	if (client_session)
		free(client_session);
	g_free(client_smack);
	g_free(uid_string);

	return result;
}

enum dbus_tapi_sim_slot_id get_sim_slot_id_by_cp_name(char *cp_name)
{
	if(g_str_has_suffix(cp_name , "0")){
		return SIM_SLOT_PRIMARY;
	} else if (g_str_has_suffix(cp_name , "1")){
		return SIM_SLOT_SECONDARY;
	} else if(g_str_has_suffix(cp_name , "2")){
		return SIM_SLOT_TERTIARY;
	}
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
