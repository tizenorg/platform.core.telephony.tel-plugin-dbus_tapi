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

#ifndef __TS_UTILITY_H__
#define __TS_UTILITY_H__

#include <sys/types.h>
#include <errno.h>
#include <glib.h>
#include <dlog.h>
#include <string.h>

#include <TelDefines.h>

#define TS_SIGNAL_OBJPATH "/org/projectx/telephony_event"
#define TS_SIGNAL_SENDER "org.projectx.telephony_event"
#define TS_SIGNAL_MEMBER_ARG0 "ARG0"
#define TS_SIGNAL_MEMBER_ARG1 "ARG1"
#define TS_SIGNAL_MEMBER_ARG2 "ARG2"
#define TS_SIGNAL_MEMBER_ARG3 "ARG3"
#define TS_SIGNAL_MEMBER_ARG4 "ARG4"
#define TS_SIGNAL_MEMBER_ARG5 "ARG5"
#define TS_SIGNAL_MEMBER_GENRESP "GENRESP"

#endif
