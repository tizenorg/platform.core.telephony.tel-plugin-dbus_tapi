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

#ifndef __TS_NOTI_H__
#define __TS_NOTI_H__

#include <glib.h>

#include <TelDefines.h>
#include <TapiCommon.h>

void ts_init_delivery_system(struct custom_data *data);

TS_BOOL ts_delivery_event(DBusConnection *EvtDeliveryHandle, int group, int type, const char *dest_name, int requestId, int Status, int data_length, void *data);

#endif	/* _TS_NOTI_H_ */
