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

#ifndef __COMMON_H__
#define __COMMON_H__

#include <glib-2.0/glib.h>
#include <dbus/dbus-protocol.h>
#include <dbus/dbus-glib.h>
#include <dbus/dbus-glib-bindings.h>
#include <glib-object.h>
#include <dbus/dbus.h>
#include <TelSim.h>
#include <TelSat.h>

__BEGIN_DECLS

struct custom_data {
	TcorePlugin *plugin;
	Communicator *comm;
	Server *server;
	Storage *strg;
	DBusConnection *EvtDeliveryHandle;
	gboolean b_recv_first_status;
	TelSimPinStatus_t pin_lock;
	TelSimPinStatus_t sim_lock;
	gboolean fdn_lock;
	TelSimLanguageInfo_t language;
	TelSimIccIdInfo_t iccid;
	TelSimEccData_t ecc;
	int ecc_count;
	void *plmn_list_search_result_cache;
	TelSatSetupMenuInfo_t *pSatMainMenu;
	GQueue sat_q;
	gboolean help_requested;
	TelSatAlphabetFormatType_t setup_menu_a_format;
};

__END_DECLS

#endif
