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
#ifndef SAT_UI_SUPPORT_H_
#define SAT_UI_SUPPORT_H_

#include <glib.h>
#include <glib-object.h>
#include <gio/gio.h>

#include <tcore.h>
#include <storage.h>
#include <server.h>
#include <type/sat.h>

gboolean sat_ui_support_terminate_sat_ui(void);
gboolean sat_ui_support_launch_call_application(enum tel_sat_proactive_cmd_type cmd_type, GVariant *data);
gboolean sat_ui_support_launch_browser_application(enum tel_sat_proactive_cmd_type cmd_type, GVariant *data);
gboolean sat_ui_support_launch_ciss_application(enum tel_sat_proactive_cmd_type cmd_type, GVariant *data);
gboolean sat_ui_support_launch_setting_application(enum tel_sat_proactive_cmd_type cmd_type, GVariant *data);
gboolean sat_ui_support_launch_sat_ui(enum tel_sat_proactive_cmd_type cmd_type, GVariant *data);
gboolean sat_ui_support_create_desktop_file(const gchar *title);
gboolean sat_ui_support_remove_desktop_file(void);
gboolean sat_ui_support_exec_bip(GDBusConnection *connection, const gchar *path, enum tel_sat_proactive_cmd_type cmd_type, GVariant *data);
gboolean sat_ui_support_exec_evtdw(GDBusConnection *connection, const gchar *path, enum tel_sat_proactive_cmd_type cmd_type, GVariant *data);
gboolean sat_ui_support_storage_init(Server *server);

#endif /* SAT_UI_SUPPORT_H_ */
