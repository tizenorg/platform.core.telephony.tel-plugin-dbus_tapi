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

#include "generated-code.h"

#include <glib.h>
#include <tcore.h>

gboolean dtapi_setup_ss_interface(TelephonyObjectSkeleton *object, TcorePlugin *plugin);
gboolean dtapi_handle_ss_notification(TelephonyObjectSkeleton *object, TcorePlugin *plugin,
	TcoreNotification command, guint data_len, const void *data);
