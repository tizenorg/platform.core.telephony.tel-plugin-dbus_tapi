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

#include "dtapi_main.h"
#include "dtapi_util.h"

#include <glib.h>
#include <tcore.h>
#include <plugin.h>

/** @todo Static assert if True */
#ifndef PLUGIN_VERSION
#define PLUGIN_VERSION 1
#endif

static gboolean on_load()
{
	dbg("i'm load");
	return TRUE;
}

static gboolean on_init(TcorePlugin *plugin)
{
	gboolean result = FALSE;
	tcore_check_return_value_assert(NULL != plugin, FALSE);

	result = dtapi_plugin_init(plugin);
	if (result == FALSE) {
		err("Failed intializing the plugin");
	} else {
		dbg("dbus-tapi-plugin INIT SUCCESS");
	}

	return result;
}

static void on_unload(TcorePlugin *plugin)
{
	tcore_check_return_assert(NULL != plugin);

	dtapi_plugin_deinit(plugin);
	dbg("dbus-tapi-plugin UNLOAD COMPLETE");
}

/* Plugin Descriptor */
EXPORT_API struct tcore_plugin_define_desc plugin_define_desc = {
	.name = "DBUS_COMMUNICATOR",
	.priority = TCORE_PLUGIN_PRIORITY_HIGH,
	.version = PLUGIN_VERSION,
	.load = on_load,
	.init = on_init,
	.unload = on_unload
};
