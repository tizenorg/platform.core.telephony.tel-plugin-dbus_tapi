#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <glib.h>

#include <tcore.h>
#include <plugin.h>
#include <communicator.h>
#include <server.h>
#include <user_request.h>

#include "generated-code.h"
#include "common.h"


static void _free_hook(UserRequest *ur)
{
	const struct tcore_user_info *ui;

	ui = tcore_user_request_ref_user_info(ur);
	if (!ui)
		return;

	if (ui->user_data)
		free(ui->user_data);
}

char *dbus_plugin_get_plugin_name_by_object_path(const char *object_path)
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
	char *plugin_name;
	struct tcore_user_info ui = { 0, };
	struct dbus_request_info *dbus_info;

	plugin_name = GET_PLUGIN_NAME(invocation);
	dbg("plugin_name = [%s]", plugin_name);

	ur = tcore_user_request_new(ctx->comm, plugin_name);

	dbus_info = calloc(sizeof(struct dbus_request_info), 1);
	dbus_info->interface_object = object;
	dbus_info->invocation = invocation;

	ui.user_data = dbus_info;

	tcore_user_request_set_user_info(ur, &ui);
	tcore_user_request_set_free_hook(ur, _free_hook);

	return ur;
}
