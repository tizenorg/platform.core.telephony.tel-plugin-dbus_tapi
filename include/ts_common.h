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

#ifndef __TS_COMMON_H__
#define __TS_COMMON_H__

#include <sys/ioctl.h>

#include <TelDefines.h>
#include <TapiCommon.h>

#include <glib-2.0/glib.h>
#include <dbus/dbus-protocol.h>
#include <dbus/dbus-glib.h>

#define TS_REQUEST_ID_RANGE_MAX 245		//available request id : 0 ~ 244
#define TS_INVALID_REQUEST_ID 0xff
#define TS_NOTI_REQUEST_ID 0xff

enum ts_internal_request_id_e {
	TS_REQID_SIM_PRE_DATA = 245,
	TS_REQID_SIM_POST_DATA = 246,
	TS_REQID_SIM_CPHS_DATA = 247,
	TS_REQID_SIM_LOCK_DATA = 248,
	TS_REQID_SIM_PB_DATA = 249,
	TS_REQID_SIM_UPDATE_DATA = 250,
	TS_REQID_NOTIFICATION = 251,
	TS_REQID_INVALID = 0xff
};


#define TAPI_MAX_NITZ_PLMN_PAIRS		2


/**
 * Security GID name
 */
#define SECURITY_GID_TEL_CALL		"tel_call"
#define SECURITY_GID_TEL_CALL_INFO	"tel_call_info"
#define SECURITY_GID_TEL_MSG		"tel_msg"
#define SECURITY_GID_TEL_MSG_INFO	"tel_msg_info"
#define SECURITY_GID_TEL_NET		"tel_net"
#define SECURITY_GID_TEL_NET_INFO	"tel_net_info"
#define SECURITY_GID_TEL_GPRS		"tel_gprs"
#define SECURITY_GID_TEL_GPRS_INFO	"tel_gprs_info"
#define SECURITY_GID_TEL_SIM		"tel_sim"
#define SECURITY_GID_TEL_SIM_INFO	"tel_sim_info"
#define SECURITY_GID_TEL_SAP		"tel_sap"
#define SECURITY_GID_TEL_SAT		"tel_sat"
#define SECURITY_GID_TEL_SS			"tel_ss"
#define SECURITY_GID_TEL_SS_INFO	"tel_ss_info"


/*This is part of platform provided code skeleton for client server model*/
#define APP_FACTORY_TYPE (app_factory_get_type ())

/*This is part of platform provided code skeleton for client server model*/
typedef struct _AppFactory
{
	GObject object;
	struct custom_data *data;
} AppFactory;

/*This is part of platform provided code skeleton for client server model*/
typedef struct _AppFactoryClass
{
	GObjectClass object_class;
} AppFactoryClass;


#endif
