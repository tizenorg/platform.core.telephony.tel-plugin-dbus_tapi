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

#include "dtapi_manager.h"
#include "dtapi_util.h"

#include <server.h>

#define AC_MANAGER	"telephony_framework::api_manager"

static void
dtapi_manager_get_modems(TelephonyManager *mgr, GDBusMethodInvocation *invocation,
	Server *server)
{
	GSList *cp_name_list, *temp_list;
	gchar **list;
	unsigned int count;
	const char *name = NULL;
	dbg("Entry");

	cp_name_list = tcore_server_get_modem_plugin_list(server);
	if (cp_name_list == NULL) {
		err("Modem List is NULL");
		telephony_manager_complete_get_modems(mgr, invocation, NULL);
		return;
	}

	count = g_slist_length(cp_name_list);
	list = g_try_malloc0(sizeof(gchar *) * (count+1));

	count = 0;
	temp_list = cp_name_list;
	for ( ; cp_name_list ; cp_name_list = cp_name_list->next) {
		name = cp_name_list->data;
		list[count] = g_strdup(name);
		dbg("list[%d]: %s", count, list[count]);
		count++;
	}

	telephony_manager_complete_get_modems(mgr, invocation, (const gchar **)list);

	/* Free memory */
	for (;count > 0; count--)
		g_free(list[count]);

	g_free(list);

	/* Freeing the received list of CP names */
	g_slist_free_full(temp_list, g_free);
}

TelephonyManager *dtapi_manager_new(Server *server)
{
	TelephonyManager *mgr = telephony_manager_skeleton_new();
	tcore_check_return_value_assert(NULL != mgr, NULL);

	g_signal_connect(mgr, "handle-get-modems",
		G_CALLBACK(dtapi_manager_get_modems), server);

	return mgr;
}
