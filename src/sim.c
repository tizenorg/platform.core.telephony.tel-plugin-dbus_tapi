/*
 * tel-plugin-socket-communicator
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
#include <errno.h>
#include <glib.h>
#include <glib-object.h>
#include <gio/gio.h>

#include <tcore.h>
#include <server.h>
#include <plugin.h>
#include <hal.h>
#include <communicator.h>
#include <core_object.h>
#include <queue.h>
#include <user_request.h>
#include <util.h>
#include <co_sim.h>

#include "generated-code.h"
#include "common.h"


static gboolean dbus_sim_data_request(struct custom_data *ctx, enum tel_sim_status sim_status )
{
	UserRequest *ur = NULL;
	TcorePlugin *plugin = NULL;

	switch(sim_status){
		case SIM_STATUS_INITIALIZING :
		case SIM_STATUS_PIN_REQUIRED :
		case SIM_STATUS_PUK_REQUIRED :
		case SIM_STATUS_CARD_BLOCKED :
		case SIM_STATUS_NCK_REQUIRED :
		case SIM_STATUS_NSCK_REQUIRED :
		case SIM_STATUS_SPCK_REQUIRED :
		case SIM_STATUS_CCK_REQUIRED :
		case SIM_STATUS_LOCK_REQUIRED :
			if(ctx->sim_recv_first_status == FALSE){
				dbg("received sim status at first time");
				plugin = tcore_server_find_plugin(ctx->server, TCORE_PLUGIN_DEFAULT);

				dbg("req - TREQ_SIM_GET_ECC ");
				ur = tcore_user_request_new(ctx->comm, tcore_plugin_get_description(plugin)->name);
				tcore_user_request_set_command(ur, TREQ_SIM_GET_ECC);
				tcore_communicator_dispatch_request(ctx->comm, ur);
				ctx->sim_recv_first_status = TRUE;
			}
			break;

		default :
			break;
	}
	return TRUE;
}

static gboolean on_sim_get_init_status(TelephonySim *sim, GDBusMethodInvocation *invocation,
		gpointer user_data)
{
	struct custom_data *ctx = user_data;
	gint tmp_cardstatus = 0xff;
	gboolean b_changed = FALSE;
	GSList *co_list = NULL;
	CoreObject *co_sim = NULL;
	TcorePlugin *plugin = NULL;

	dbg("Func Entrance");

	plugin = tcore_server_find_plugin(ctx->server, TCORE_PLUGIN_DEFAULT);
	co_list = tcore_plugin_get_core_objects_bytype(plugin, CORE_OBJECT_TYPE_SIM);
	if (!co_list) {
		dbg("error- co_list is NULL");
	}
	co_sim = (CoreObject *)co_list->data;
	g_slist_free(co_list);

	if (!co_sim) {
		dbg("error- co_sim is NULL");
	}

	tmp_cardstatus = tcore_sim_get_status(co_sim);
	b_changed = tcore_sim_get_identification(co_sim);
	dbg("sim init info - cardstatus[%d],changed[%d]", tmp_cardstatus, b_changed);

	telephony_sim_complete_get_init_status(sim, invocation, tmp_cardstatus, b_changed);

	return TRUE;
}

static gboolean on_sim_get_card_type(TelephonySim *sim, GDBusMethodInvocation *invocation,
		gpointer user_data)
{
	struct custom_data *ctx = user_data;
	enum tel_sim_type type = SIM_TYPE_UNKNOWN;
	GSList *co_list = NULL;
	CoreObject *co_sim = NULL;
	TcorePlugin *plugin = NULL;

	dbg("Func Entrance");

	plugin = tcore_server_find_plugin(ctx->server, TCORE_PLUGIN_DEFAULT);
	co_list = tcore_plugin_get_core_objects_bytype(plugin, CORE_OBJECT_TYPE_SIM);
	if (!co_list) {
		dbg("error- co_list is NULL");
	}
	co_sim = (CoreObject *)co_list->data;
	g_slist_free(co_list);

	if (!co_sim) {
		dbg("error- co_sim is NULL");
	}

	type = tcore_sim_get_type(co_sim);

	telephony_sim_complete_get_card_type(sim, invocation, type);

	return TRUE;
}

static gboolean on_sim_get_imsi(TelephonySim *sim, GDBusMethodInvocation *invocation,
		gpointer user_data)
{
	struct custom_data *ctx = user_data;
	struct tel_sim_imsi *n_imsi;
	GSList *co_list = NULL;
	CoreObject *co_sim = NULL;
	TcorePlugin *plugin = NULL;

	dbg("Func Entrance");
	plugin = tcore_server_find_plugin(ctx->server, TCORE_PLUGIN_DEFAULT);
	co_list = tcore_plugin_get_core_objects_bytype(plugin, CORE_OBJECT_TYPE_SIM);
	if (!co_list) {
		dbg("error- co_list is NULL");
	}
	co_sim = (CoreObject *)co_list->data;
	g_slist_free(co_list);

	if (!co_sim) {
		dbg("error- co_sim is NULL");
	}

	n_imsi = tcore_sim_get_imsi(co_sim);
	dbg("n_imsi->plmn[%s]", n_imsi->plmn);
	dbg("n_imsi->msin[%s]", n_imsi->msin);
	telephony_sim_complete_get_imsi(sim, invocation, n_imsi->plmn, n_imsi->msin);
	return TRUE;
}

static gboolean on_sim_get_ecc(TelephonySim *sim, GDBusMethodInvocation *invocation,
		gpointer user_data)
{
	struct custom_data *ctx = user_data;
	GVariant *gv = NULL;
	GVariantBuilder b;
	int i;

	dbg("Func Entrance");

	g_variant_builder_init(&b, G_VARIANT_TYPE("aa{sv}"));

	for (i = 0; i < ctx->cached_sim_ecc.ecc_count; i++) {
		g_variant_builder_open(&b, G_VARIANT_TYPE("a{sv}"));
		g_variant_builder_add(&b, "{sv}", "name", g_variant_new_string(ctx->cached_sim_ecc.ecc[i].ecc_string));
		g_variant_builder_add(&b, "{sv}", "number", g_variant_new_string(ctx->cached_sim_ecc.ecc[i].ecc_num));
		g_variant_builder_add(&b, "{sv}", "category", g_variant_new_int32(ctx->cached_sim_ecc.ecc[i].ecc_category));
		g_variant_builder_close(&b);
	}
	gv = g_variant_builder_end(&b);

	if (!gv)
		dbg("error - ecc gv is NULL");

	telephony_sim_complete_get_ecc(sim, invocation, gv);

	return TRUE;
}

static gboolean on_sim_get_iccid(TelephonySim *sim, GDBusMethodInvocation *invocation,
		gpointer user_data)
{
	struct custom_data *ctx = user_data;
	UserRequest *ur = NULL;

	dbg("Func Entrance");
	ur = MAKE_UR(ctx, sim, invocation);

	tcore_user_request_set_command(ur, TREQ_SIM_GET_ICCID);
	tcore_communicator_dispatch_request(ctx->comm, ur);

	return TRUE;
}

static gboolean on_sim_get_language(TelephonySim *sim, GDBusMethodInvocation *invocation,
		gpointer user_data)
{
	struct custom_data *ctx = user_data;
	UserRequest *ur = NULL;

	ur = MAKE_UR(ctx, sim, invocation);

	tcore_user_request_set_command(ur, TREQ_SIM_GET_LANGUAGE);
	tcore_communicator_dispatch_request(ctx->comm, ur);

	return TRUE;
}

static gboolean on_sim_set_language(TelephonySim *sim, GDBusMethodInvocation *invocation,
		gint arg_language, gpointer user_data)
{
	struct custom_data *ctx = user_data;
	UserRequest *ur = NULL;

	struct treq_sim_set_language set_language;
	memset(&set_language, 0, sizeof(struct treq_sim_set_language));
	set_language.language = arg_language;

	dbg("set_language.language[%d]", set_language.language);
	ur = MAKE_UR(ctx, sim, invocation);

	tcore_user_request_set_data(ur, sizeof(struct treq_sim_set_language), &set_language);
	tcore_user_request_set_command(ur, TREQ_SIM_SET_LANGUAGE);
	tcore_communicator_dispatch_request(ctx->comm, ur);

	return TRUE;
}

static gboolean on_sim_get_callforwarding(TelephonySim *sim, GDBusMethodInvocation *invocation,
		gpointer user_data)
{
	struct custom_data *ctx = user_data;
	UserRequest *ur = NULL;

	ur = MAKE_UR(ctx, sim, invocation);

	tcore_user_request_set_command(ur, TREQ_SIM_GET_CALLFORWARDING);
	tcore_communicator_dispatch_request(ctx->comm, ur);

	return TRUE;
}

static gboolean on_sim_get_message_waiting(TelephonySim *sim, GDBusMethodInvocation *invocation,
		gpointer user_data)
{
	struct custom_data *ctx = user_data;
	UserRequest *ur = NULL;

	ur = MAKE_UR(ctx, sim, invocation);

	tcore_user_request_set_command(ur, TREQ_SIM_GET_MESSAGEWAITING);
	tcore_communicator_dispatch_request(ctx->comm, ur);

	return TRUE;
}

static gboolean on_sim_get_mailbox(TelephonySim *sim, GDBusMethodInvocation *invocation,
		gpointer user_data)
{
	struct custom_data *ctx = user_data;
	UserRequest *ur = NULL;

	ur = MAKE_UR(ctx, sim, invocation);

	tcore_user_request_set_command(ur, TREQ_SIM_GET_MAILBOX);
	tcore_communicator_dispatch_request(ctx->comm, ur);

	return TRUE;
}


static gboolean on_sim_get_cphsinfo(TelephonySim *sim, GDBusMethodInvocation *invocation,
		gpointer user_data)
{
	struct custom_data *ctx = user_data;
	UserRequest *ur = NULL;

	ur = MAKE_UR(ctx, sim, invocation);

	tcore_user_request_set_command(ur, TREQ_SIM_GET_CPHS_INFO);
	tcore_communicator_dispatch_request(ctx->comm, ur);

	return TRUE;
}

static gboolean on_sim_get_msisdn(TelephonySim *sim, GDBusMethodInvocation *invocation,
		gpointer user_data)
{
	struct custom_data *ctx = user_data;
	UserRequest *ur = NULL;

	ur = MAKE_UR(ctx, sim, invocation);

	tcore_user_request_set_command(ur, TREQ_SIM_GET_MSISDN);
	tcore_communicator_dispatch_request(ctx->comm, ur);

	return TRUE;
}

static gboolean on_sim_get_oplmnwact(TelephonySim *sim, GDBusMethodInvocation *invocation,
		gpointer user_data)
{
	struct custom_data *ctx = user_data;
	UserRequest *ur = NULL;

	ur = MAKE_UR(ctx, sim, invocation);

	tcore_user_request_set_command(ur, TREQ_SIM_GET_OPLMNWACT);
	tcore_communicator_dispatch_request(ctx->comm, ur);


	return TRUE;
}

static gboolean on_sim_get_spn(TelephonySim *sim, GDBusMethodInvocation *invocation,
		gpointer user_data)
{
	struct custom_data *ctx = user_data;
	UserRequest *ur = NULL;

	ur = MAKE_UR(ctx, sim, invocation);

	tcore_user_request_set_command(ur, TREQ_SIM_GET_SPN);
	tcore_communicator_dispatch_request(ctx->comm, ur);

	return TRUE;
}

static gboolean on_sim_get_cphs_netname(TelephonySim *sim, GDBusMethodInvocation *invocation,
		gpointer user_data)
{
	struct custom_data *ctx = user_data;
	UserRequest *ur = NULL;

	ur = MAKE_UR(ctx, sim, invocation);

	tcore_user_request_set_command(ur, TREQ_SIM_GET_CPHS_NETNAME);
	tcore_communicator_dispatch_request(ctx->comm, ur);

	return TRUE;
}

static gboolean on_sim_authentication(TelephonySim *sim, GDBusMethodInvocation *invocation,
		gint arg_type,
	    GVariant *arg_rand,
	    GVariant *arg_autn,
	    gpointer user_data)
{
	struct custom_data *ctx = user_data;
	UserRequest *ur = NULL;
	GVariantIter *iter = NULL;
	GVariant *rand_gv = NULL;
	GVariant *autn_gv = NULL;
	guchar rt_i;
	int i =0;

	struct treq_sim_req_authentication req_auth;
	memset(&req_auth, 0, sizeof(struct treq_sim_req_authentication));

	req_auth.auth_type = arg_type;

	rand_gv = g_variant_get_variant(arg_rand);
	g_variant_get(rand_gv, "ay", &iter);
	while ( g_variant_iter_loop (iter, "y", &rt_i)) {
		req_auth.rand_data[i] = rt_i;
		i++;
	}
	req_auth.rand_length = (unsigned int)i;

	i = 0;
	autn_gv = g_variant_get_variant(arg_autn);
	g_variant_get(autn_gv, "ay", &iter);
	while ( g_variant_iter_loop (iter, "y", &rt_i)) {
		req_auth.autn_data[i] = rt_i;
		i++;
	}
	req_auth.autn_length = (unsigned int)i;

	ur = MAKE_UR(ctx, sim, invocation);
	tcore_user_request_set_data(ur, sizeof(struct treq_sim_req_authentication), &req_auth);
	tcore_user_request_set_command(ur, TREQ_SIM_REQ_AUTHENTICATION);
	tcore_communicator_dispatch_request(ctx->comm, ur);

	return TRUE;
}

static gboolean on_sim_verify_sec(TelephonySim *sim, GDBusMethodInvocation *invocation,
	    gint arg_type,
	    const gchar *arg_password,
		gpointer user_data)
{
	struct custom_data *ctx = user_data;
	UserRequest *ur = NULL;

	struct treq_sim_verify_pins verify_pins;
	memset(&verify_pins, 0, sizeof(struct treq_sim_verify_pins));

	verify_pins.pin_type = arg_type;
	verify_pins.pin_length = strlen(arg_password);
	memcpy(verify_pins.pin, arg_password, verify_pins.pin_length);

	ur = MAKE_UR(ctx, sim, invocation);
	tcore_user_request_set_data(ur, sizeof(struct treq_sim_verify_pins), &verify_pins);
	tcore_user_request_set_command(ur, TREQ_SIM_VERIFY_PINS);
	tcore_communicator_dispatch_request(ctx->comm, ur);

	return TRUE;
}

static gboolean on_sim_verify_puk(TelephonySim *sim, GDBusMethodInvocation *invocation,
	    gint arg_type,
	    const gchar *arg_puk,
	    const gchar *arg_new_pin,
		gpointer user_data)
{
	struct custom_data *ctx = user_data;
	UserRequest *ur = NULL;

	struct treq_sim_verify_puks verify_puks;
	memset(&verify_puks, 0, sizeof(struct treq_sim_verify_puks));

	verify_puks.puk_type = arg_type;
	verify_puks.puk_length = strlen(arg_puk);
	memcpy(verify_puks.puk, arg_puk, verify_puks.puk_length);
	verify_puks.pin_length = strlen(arg_new_pin);
	memcpy(verify_puks.pin, arg_new_pin, verify_puks.pin_length);

	ur = MAKE_UR(ctx, sim, invocation);
	tcore_user_request_set_data(ur, sizeof(struct treq_sim_verify_puks), &verify_puks);
	tcore_user_request_set_command(ur, TREQ_SIM_VERIFY_PUKS);
	tcore_communicator_dispatch_request(ctx->comm, ur);

	return TRUE;
}

static gboolean on_sim_change_pin(TelephonySim *sim, GDBusMethodInvocation *invocation,
	    gint arg_type,
	    const gchar *arg_old_password,
	    const gchar *arg_new_password,
		gpointer user_data)
{
	struct custom_data *ctx = user_data;
	UserRequest *ur = NULL;

	struct treq_sim_change_pins change_pins;
	memset(&change_pins, 0, sizeof(struct treq_sim_change_pins));

	change_pins.type = arg_type;
	change_pins.old_pin_length = strlen(arg_old_password);
	memcpy(change_pins.old_pin, arg_old_password, change_pins.old_pin_length);
	change_pins.new_pin_length = strlen(arg_new_password);
	memcpy(change_pins.new_pin, arg_new_password, change_pins.new_pin_length);

	ur = MAKE_UR(ctx, sim, invocation);
	tcore_user_request_set_data(ur, sizeof(struct treq_sim_change_pins), &change_pins);
	tcore_user_request_set_command(ur, TREQ_SIM_CHANGE_PINS);
	tcore_communicator_dispatch_request(ctx->comm, ur);

	return TRUE;
}

static gboolean on_sim_disable_facility(TelephonySim *sim, GDBusMethodInvocation *invocation,
	    gint arg_type,
	    const gchar *arg_password,
		gpointer user_data)
{
	struct custom_data *ctx = user_data;
	UserRequest *ur = NULL;

	struct treq_sim_disable_facility dis_facility;
	memset(&dis_facility, 0, sizeof(struct treq_sim_disable_facility));

	dbg("arg_type[%d]", arg_type);
	switch (arg_type) {
		case 1:
			dis_facility.type = SIM_FACILITY_PS;
			break;
		case 3:
			dis_facility.type = SIM_FACILITY_SC;
			break;
		case 4:
			dis_facility.type = SIM_FACILITY_FD;
			break;
		case 5:
			dis_facility.type = SIM_FACILITY_PN;
			break;
		case 6:
			dis_facility.type = SIM_FACILITY_PU;
			break;
		case 7:
			dis_facility.type = SIM_FACILITY_PP;
			break;
		case 8:
			dis_facility.type = SIM_FACILITY_PC;
			break;
		default:
			dbg("error - not handled type[%d]", arg_type);
			break;
	}
	dis_facility.password_length = strlen(arg_password);
	memcpy(dis_facility.password, arg_password, dis_facility.password_length);

	ur = MAKE_UR(ctx, sim, invocation);
	tcore_user_request_set_data(ur, sizeof(struct treq_sim_disable_facility), &dis_facility);
	tcore_user_request_set_command(ur, TREQ_SIM_DISABLE_FACILITY);
	tcore_communicator_dispatch_request(ctx->comm, ur);

	return TRUE;
}

static gboolean on_sim_enable_facility(TelephonySim *sim, GDBusMethodInvocation *invocation,
	    gint arg_type,
	    const gchar *arg_password,
		gpointer user_data)
{
	struct custom_data *ctx = user_data;
	UserRequest *ur = NULL;

	struct treq_sim_enable_facility en_facility;
	memset(&en_facility, 0, sizeof(struct treq_sim_enable_facility));

	dbg("arg_type[%d]", arg_type);
	switch (arg_type) {
		case 1:
			en_facility.type = SIM_FACILITY_PS;
			break;
		case 3:
			en_facility.type = SIM_FACILITY_SC;
			break;
		case 4:
			en_facility.type = SIM_FACILITY_FD;
			break;
		case 5:
			en_facility.type = SIM_FACILITY_PN;
			break;
		case 6:
			en_facility.type = SIM_FACILITY_PU;
			break;
		case 7:
			en_facility.type = SIM_FACILITY_PP;
			break;
		case 8:
			en_facility.type = SIM_FACILITY_PC;
			break;
		default:
			dbg("error - not handled type[%d]", arg_type);
			break;
	}
	en_facility.password_length = strlen(arg_password);
	memcpy(en_facility.password, arg_password, en_facility.password_length);

	ur = MAKE_UR(ctx, sim, invocation);
	tcore_user_request_set_data(ur, sizeof(struct treq_sim_enable_facility), &en_facility);
	tcore_user_request_set_command(ur, TREQ_SIM_ENABLE_FACILITY);
	tcore_communicator_dispatch_request(ctx->comm, ur);

	return TRUE;
}

static gboolean on_sim_get_facility(TelephonySim *sim, GDBusMethodInvocation *invocation,
	    gint arg_type,
		gpointer user_data)
{
	struct custom_data *ctx = user_data;
	UserRequest *ur = NULL;

	struct treq_sim_get_facility_status facility;
	memset(&facility, 0, sizeof(struct treq_sim_get_facility_status));

	dbg("arg_type[%d]", arg_type);
	switch (arg_type) {
		case 1:
			facility.type = SIM_FACILITY_PS;
			break;
		case 3:
			facility.type = SIM_FACILITY_SC;
			break;
		case 4:
			facility.type = SIM_FACILITY_FD;
			break;
		case 5:
			facility.type = SIM_FACILITY_PN;
			break;
		case 6:
			facility.type = SIM_FACILITY_PU;
			break;
		case 7:
			facility.type = SIM_FACILITY_PP;
			break;
		case 8:
			facility.type = SIM_FACILITY_PC;
			break;
		default:
			dbg("error - not handled type[%d]", arg_type);
			break;
	}

	ur = MAKE_UR(ctx, sim, invocation);
	tcore_user_request_set_data(ur, sizeof(struct treq_sim_get_facility_status), &facility);
	tcore_user_request_set_command(ur, TREQ_SIM_GET_FACILITY_STATUS);
	tcore_communicator_dispatch_request(ctx->comm, ur);

	return TRUE;
}

static gboolean on_sim_get_lock_info(TelephonySim *sim, GDBusMethodInvocation *invocation,
	    gint arg_type,
		gpointer user_data)
{
	struct custom_data *ctx = user_data;
	UserRequest *ur = NULL;

	struct treq_sim_get_lock_info lock_info;
	memset(&lock_info, 0, sizeof(struct treq_sim_get_lock_info));

	dbg("arg_type[%d]", arg_type);
	switch (arg_type) {
		case 1:
			lock_info.type = SIM_FACILITY_PS;
			break;
		case 3:
			lock_info.type = SIM_FACILITY_SC;
			break;
		case 4:
			lock_info.type = SIM_FACILITY_FD;
			break;
		case 5:
			lock_info.type = SIM_FACILITY_PN;
			break;
		case 6:
			lock_info.type = SIM_FACILITY_PU;
			break;
		case 7:
			lock_info.type = SIM_FACILITY_PP;
			break;
		case 8:
			lock_info.type = SIM_FACILITY_PC;
			break;
		default:
			dbg("error - not handled type[%d]", arg_type);
			break;
	}

	ur = MAKE_UR(ctx, sim, invocation);
	tcore_user_request_set_data(ur, sizeof(struct treq_sim_get_lock_info), &lock_info);
	tcore_user_request_set_command(ur, TREQ_SIM_GET_LOCK_INFO);
	tcore_communicator_dispatch_request(ctx->comm, ur);

	return TRUE;
}

static gboolean on_sim_transfer_apdu(TelephonySim *sim, GDBusMethodInvocation *invocation,
		GVariant *arg_apdu,
		gpointer user_data)
{
	struct custom_data *ctx = user_data;
	UserRequest *ur = NULL;
	struct treq_sim_transmit_apdu send_apdu;

	GVariantIter *iter = NULL;
	GVariant *inner_gv = NULL;
	guchar rt_i;
	int i =0;

	dbg("Func Entrance");
	memset(&send_apdu, 0, sizeof(struct treq_sim_transmit_apdu));

	inner_gv = g_variant_get_variant(arg_apdu);

	g_variant_get(inner_gv, "ay", &iter);
	while ( g_variant_iter_loop (iter, "y", &rt_i)) {
		send_apdu.apdu[i] = rt_i;
		i++;
	}
	send_apdu.apdu_length = (unsigned int)i;
	g_variant_iter_free(iter);
	g_variant_unref(inner_gv);
	g_variant_unref(arg_apdu);

	for(i=0; i < (int)send_apdu.apdu_length; i++)
		dbg("apdu[%d][0x%02x]",i, send_apdu.apdu[i]);

	ur = MAKE_UR(ctx, sim, invocation);
	tcore_user_request_set_data(ur, sizeof(struct treq_sim_transmit_apdu), &send_apdu);
	tcore_user_request_set_command(ur, TREQ_SIM_TRANSMIT_APDU);
	tcore_communicator_dispatch_request(ctx->comm, ur);

	return TRUE;
}

static gboolean on_sim_get_atr(TelephonySim *sim, GDBusMethodInvocation *invocation,
		gpointer user_data)
{
	struct custom_data *ctx = user_data;
	UserRequest *ur = NULL;

	ur = MAKE_UR(ctx, sim, invocation);

	tcore_user_request_set_command(ur, TREQ_SIM_GET_ATR);
	tcore_communicator_dispatch_request(ctx->comm, ur);

	return TRUE;
}

gboolean dbus_plugin_setup_sim_interface(TelephonyObjectSkeleton *object, struct custom_data *ctx)
{
	TelephonySim *sim;

	sim = telephony_sim_skeleton_new();
	telephony_object_skeleton_set_sim(object, sim);
	g_object_unref(sim);

	dbg("sim = %p", sim);

	g_signal_connect (sim,
			"handle-get-init-status",
			G_CALLBACK (on_sim_get_init_status),
			ctx);

	g_signal_connect (sim,
			"handle-get-card-type",
			G_CALLBACK (on_sim_get_card_type),
			ctx);

	g_signal_connect (sim,
			"handle-get-imsi",
			G_CALLBACK (on_sim_get_imsi),
			ctx);

	g_signal_connect (sim,
			"handle-get-ecc",
			G_CALLBACK (on_sim_get_ecc),
			ctx);

	g_signal_connect (sim,
			"handle-get-iccid",
			G_CALLBACK (on_sim_get_iccid),
			ctx);

	g_signal_connect (sim,
			"handle-get-language",
			G_CALLBACK (on_sim_get_language),
			ctx);

	g_signal_connect (sim,
			"handle-set-language",
			G_CALLBACK (on_sim_set_language),
			ctx);

	g_signal_connect (sim,
			"handle-get-callforwarding",
			G_CALLBACK (on_sim_get_callforwarding),
			ctx);

	g_signal_connect (sim,
			"handle-get-message-waiting",
			G_CALLBACK (on_sim_get_message_waiting),
			ctx);

	g_signal_connect (sim,
			"handle-get-mailbox",
			G_CALLBACK (on_sim_get_mailbox),
			ctx);

	g_signal_connect (sim,
			"handle-get-cphsinfo",
			G_CALLBACK (on_sim_get_cphsinfo),
			ctx);

	g_signal_connect (sim,
			"handle-get-msisdn",
			G_CALLBACK (on_sim_get_msisdn),
			ctx);

	g_signal_connect (sim,
			"handle-get-oplmnwact",
			G_CALLBACK (on_sim_get_oplmnwact),
			ctx);

	g_signal_connect (sim,
			"handle-get-spn",
			G_CALLBACK (on_sim_get_spn),
			ctx);

	g_signal_connect (sim,
			"handle-get-cphs-net-name",
			G_CALLBACK (on_sim_get_cphs_netname),
			ctx);

	g_signal_connect (sim,
			"handle-authentication",
			G_CALLBACK (on_sim_authentication),
			ctx);

	g_signal_connect (sim,
			"handle-verify-sec",
			G_CALLBACK (on_sim_verify_sec),
			ctx);

	g_signal_connect (sim,
			"handle-verify-puk",
			G_CALLBACK (on_sim_verify_puk),
			ctx);

	g_signal_connect (sim,
			"handle-change-pin",
			G_CALLBACK (on_sim_change_pin),
			ctx);

	g_signal_connect (sim,
			"handle-disable-facility",
			G_CALLBACK (on_sim_disable_facility),
			ctx);

	g_signal_connect (sim,
			"handle-enable-facility",
			G_CALLBACK (on_sim_enable_facility),
			ctx);

	g_signal_connect (sim,
			"handle-get-facility",
			G_CALLBACK (on_sim_get_facility),
			ctx);

	g_signal_connect (sim,
			"handle-get-lock-info",
			G_CALLBACK (on_sim_get_lock_info),
			ctx);

	g_signal_connect (sim,
			"handle-transfer-apdu",
			G_CALLBACK (on_sim_transfer_apdu),
			ctx);

	g_signal_connect (sim,
			"handle-get-atr",
			G_CALLBACK (on_sim_get_atr),
			ctx);

	return TRUE;
}

gboolean dbus_plugin_sim_response(struct custom_data *ctx, UserRequest *ur,
		struct dbus_request_info *dbus_info, enum tcore_response_command command,
		unsigned int data_len, const void *data)
{
	const struct tresp_sim_verify_pins *resp_verify_pins = data;
	const struct tresp_sim_verify_puks *resp_verify_puks = data;
	const struct tresp_sim_change_pins *resp_change_pins = data;
	const struct tresp_sim_get_facility_status *resp_get_facility = data;
	const struct tresp_sim_disable_facility *resp_dis_facility = data;
	const struct tresp_sim_enable_facility *resp_en_facility = data;
	const struct tresp_sim_transmit_apdu *resp_apdu = data;
	const struct tresp_sim_get_atr *resp_get_atr = data;
	const struct tresp_sim_read *resp_read = data;
	const struct tresp_sim_req_authentication *resp_auth = data;
	const struct tresp_sim_set_language *resp_set_language = data;
	const struct tresp_sim_get_lock_info *resp_lock = data;
	gint f_type =0;
	int i =0;
	dbg("Command = [0x%x], data_len = %d", command, data_len);

	switch (command) {
		case TRESP_SIM_GET_ECC: {
			dbg("resp comm - TRESP_SIM_GET_ECC");
			/*			GVariant *gv = NULL;
			GVariantBuilder b;
			g_variant_builder_init(&b, G_VARIANT_TYPE("aa{sv}"));

			for (i = 0; i < resp_read->data.ecc.ecc_count; i++) {
				g_variant_builder_open(&b, G_VARIANT_TYPE("a{sv}"));
				g_variant_builder_add(&b, "{sv}", "name", g_variant_new_string(resp_read->data.ecc.ecc[i].ecc_string));
				g_variant_builder_add(&b, "{sv}", "number", g_variant_new_string(resp_read->data.ecc.ecc[i].ecc_num));
				g_variant_builder_add(&b, "{sv}", "category", g_variant_new_int32(resp_read->data.ecc.ecc[i].ecc_category));
				g_variant_builder_close(&b);
			}
			gv = g_variant_builder_end(&b);
			ctx->cached_sim_ecc = gv;*/
			memcpy((void*)&ctx->cached_sim_ecc, (const void*)&resp_read->data.ecc, sizeof(struct tel_sim_ecc_list));
		}
			break;

		case TRESP_SIM_GET_ICCID:
			dbg("resp comm - TRESP_SIM_GET_ICCID");
			dbg("dbus_info->interface_object[%p], dbus_info->invocation[%p],dbus_info->interface_object, dbus_info->invocation");
			dbg("result[%d], iccid[%s]", resp_read->result, resp_read->data.iccid.iccid);
			telephony_sim_complete_get_iccid(dbus_info->interface_object, dbus_info->invocation,
					resp_read->result,
					resp_read->data.iccid.iccid);
			break;

		case TRESP_SIM_GET_LANGUAGE:
			dbg("resp comm - TRESP_SIM_GET_LANGUAGE");
			telephony_sim_complete_get_language(dbus_info->interface_object, dbus_info->invocation,
					resp_read->result,
					resp_read->data.language.language[0]);
			break;

		case TRESP_SIM_SET_LANGUAGE:
			dbg("resp comm - TRESP_SIM_SET_LANGUAGE");
			telephony_sim_complete_set_language(dbus_info->interface_object, dbus_info->invocation,
					resp_set_language->result);
			break;

		case TRESP_SIM_GET_CALLFORWARDING:
			dbg("resp comm - TRESP_SIM_GET_CALLFORWARDING");
			telephony_sim_complete_get_callforwarding (dbus_info->interface_object, dbus_info->invocation,
					resp_read->result,
					resp_read->data.cf.voice1,
					resp_read->data.cf.voice2);
			break;

		case TRESP_SIM_GET_MESSAGEWAITING:
			dbg("resp comm - TRESP_SIM_GET_MESSAGEWAITING");
			if (resp_read->data.mw.b_cphs) {
				telephony_sim_complete_get_message_waiting(dbus_info->interface_object,	dbus_info->invocation,
						resp_read->result,
						resp_read->data.mw.mw_data_u.cphs_mw.b_voice1,
						resp_read->data.mw.mw_data_u.cphs_mw.b_voice2,
						resp_read->data.mw.mw_data_u.cphs_mw.b_fax,
						resp_read->data.mw.mw_data_u.cphs_mw.b_data);
			} else {
				telephony_sim_complete_get_message_waiting(dbus_info->interface_object,	dbus_info->invocation,
						resp_read->result,
						resp_read->data.mw.mw_data_u.mw.voice_count,
						0,
						resp_read->data.mw.mw_data_u.mw.fax_count,
						resp_read->data.mw.mw_data_u.mw.video_count);
			}
			break;

		case TRESP_SIM_GET_MAILBOX: {
			GVariant *gv = NULL;
			GVariantBuilder b;
			dbg("resp comm - TRESP_SIM_GET_MAILBOX");

			g_variant_builder_init(&b, G_VARIANT_TYPE("aa{sv}"));

			if(resp_read->data.mailbox.voice1.DiallingnumLength){
				g_variant_builder_open(&b, G_VARIANT_TYPE("a{sv}"));
				g_variant_builder_add(&b, "{sv}", "type", g_variant_new_string("voice1"));
				g_variant_builder_add(&b, "{sv}", "name", g_variant_new_string(resp_read->data.mailbox.voice1.AlphaId));
				g_variant_builder_add(&b, "{sv}", "number", g_variant_new_string(resp_read->data.mailbox.voice1.DiallingNum));
				g_variant_builder_add(&b, "{sv}", "ton", g_variant_new_int32(resp_read->data.mailbox.voice1.TypeOfNumber));
				g_variant_builder_close(&b);
			}

			if(resp_read->data.mailbox.voice2.DiallingnumLength){
				g_variant_builder_open(&b, G_VARIANT_TYPE("a{sv}"));
				g_variant_builder_add(&b, "{sv}", "type", g_variant_new_string("voice2"));
				g_variant_builder_add(&b, "{sv}", "name", g_variant_new_string(resp_read->data.mailbox.voice2.AlphaId));
				g_variant_builder_add(&b, "{sv}", "number", g_variant_new_string(resp_read->data.mailbox.voice2.DiallingNum));
				g_variant_builder_add(&b, "{sv}", "ton", g_variant_new_int32(resp_read->data.mailbox.voice2.TypeOfNumber));
				g_variant_builder_close(&b);
			}

			if(resp_read->data.mailbox.fax.DiallingnumLength){
				g_variant_builder_open(&b, G_VARIANT_TYPE("a{sv}"));
				g_variant_builder_add(&b, "{sv}", "type", g_variant_new_string("fax"));
				g_variant_builder_add(&b, "{sv}", "name", g_variant_new_string(resp_read->data.mailbox.fax.AlphaId));
				g_variant_builder_add(&b, "{sv}", "number", g_variant_new_string(resp_read->data.mailbox.fax.DiallingNum));
				g_variant_builder_add(&b, "{sv}", "ton", g_variant_new_int32(resp_read->data.mailbox.fax.TypeOfNumber));
				g_variant_builder_close(&b);
			}

			if(resp_read->data.mailbox.video.DiallingnumLength){
				g_variant_builder_open(&b, G_VARIANT_TYPE("a{sv}"));
				g_variant_builder_add(&b, "{sv}", "type", g_variant_new_string("video"));
				g_variant_builder_add(&b, "{sv}", "name", g_variant_new_string(resp_read->data.mailbox.video.AlphaId));
				g_variant_builder_add(&b, "{sv}", "number", g_variant_new_string(resp_read->data.mailbox.video.DiallingNum));
				g_variant_builder_add(&b, "{sv}", "ton", g_variant_new_int32(resp_read->data.mailbox.video.TypeOfNumber));
				g_variant_builder_close(&b);
			}
			gv = g_variant_builder_end(&b);
			telephony_sim_complete_get_mailbox (dbus_info->interface_object, dbus_info->invocation,
					resp_read->result,
					gv);
			g_variant_unref(gv);
		}
			break;

		case TRESP_SIM_GET_CPHS_INFO:
			dbg("resp comm - TRESP_SIM_GET_CPHS_INFO");
			telephony_sim_complete_get_cphsinfo (dbus_info->interface_object, dbus_info->invocation,
					resp_read->result,
					resp_read->data.cphs.CphsPhase,
					resp_read->data.cphs.CphsServiceTable.bOperatorNameShortForm,
					resp_read->data.cphs.CphsServiceTable.bMailBoxNumbers,
					resp_read->data.cphs.CphsServiceTable.bServiceStringTable,
					resp_read->data.cphs.CphsServiceTable.bCustomerServiceProfile,
					resp_read->data.cphs.CphsServiceTable.bInformationNumbers);
			break;

		case TRESP_SIM_GET_SPN:
			dbg("resp comm - TRESP_SIM_GET_SPN");
			telephony_sim_complete_get_spn (dbus_info->interface_object, dbus_info->invocation,
					resp_read->result,
					resp_read->data.spn.display_condition, (const gchar *)resp_read->data.spn.spn);
			break;

		case TRESP_SIM_GET_CPHS_NETNAME:
			dbg("resp comm - TRESP_SIM_GET_CPHS_NETNAME");
			telephony_sim_complete_get_cphs_net_name (dbus_info->interface_object, dbus_info->invocation,
					resp_read->result,
					(const gchar *)resp_read->data.cphs_net.full_name, (const gchar *)resp_read->data.cphs_net.short_name);
			break;

		case TRESP_SIM_GET_MSISDN:{
			GVariant *gv = NULL;
			GVariantBuilder b;
			dbg("resp comm - TRESP_SIM_GET_MSISDN");
			g_variant_builder_init(&b, G_VARIANT_TYPE("aa{sv}"));

			for(i=0;i < resp_read->data.msisdn_list.count; i++){
				g_variant_builder_open(&b,G_VARIANT_TYPE("a{sv}"));
				g_variant_builder_add(&b, "{sv}", "name", g_variant_new_string((const gchar *)resp_read->data.msisdn_list.msisdn[i].name));
				g_variant_builder_add(&b, "{sv}", "number", g_variant_new_string((const gchar *)resp_read->data.msisdn_list.msisdn[i].num));
				g_variant_builder_close(&b);
			}
			gv = g_variant_builder_end(&b);

			telephony_sim_complete_get_msisdn (dbus_info->interface_object, dbus_info->invocation,
					resp_read->result,
					gv);
			g_variant_unref(gv);
		}
			break;

		case TRESP_SIM_GET_OPLMNWACT:{
			GVariant *gv = NULL;
			GVariantBuilder b;
			dbg("resp comm - TRESP_SIM_GET_OPLMNWACT");
			g_variant_builder_init(&b, G_VARIANT_TYPE("aa{sv}"));

			for(i=0;i < resp_read->data.opwa.opwa_count; i++){
				g_variant_builder_open(&b,G_VARIANT_TYPE("a{sv}"));
				g_variant_builder_add(&b, "{sv}", "plmn", g_variant_new_string((const gchar *)resp_read->data.opwa.opwa[i].plmn));
				g_variant_builder_add(&b, "{sv}", "b_umts", g_variant_new_boolean(resp_read->data.opwa.opwa[i].b_umts));
				g_variant_builder_add(&b, "{sv}", "b_gsm", g_variant_new_boolean(resp_read->data.opwa.opwa[i].b_gsm));
				g_variant_builder_close(&b);
			}
			gv = g_variant_builder_end(&b);

			telephony_sim_complete_get_oplmnwact (dbus_info->interface_object, dbus_info->invocation,
					resp_read->result,
					gv);
			g_variant_unref(gv);
		}
			break;

		case TRESP_SIM_REQ_AUTHENTICATION: {
			GVariantBuilder *builder = NULL;
			GVariant *ak = NULL;
			GVariant *cp = NULL;
			GVariant *it = NULL;
			GVariant *resp = NULL;
			GVariant *ak_gv = NULL;
			GVariant *cp_gv = NULL;
			GVariant *it_gv = NULL;
			GVariant *resp_gv = NULL;
			int i =0;

			dbg("resp comm - TRESP_SIM_REQ_AUTHENTICATION");

			builder = g_variant_builder_new (G_VARIANT_TYPE ("ay"));
			for(i = 0; i < (int)resp_auth->authentication_key_length; i++) {
				dbg("resp_auth->authentication_key[%d][0x%02x]", i,resp_auth->authentication_key[i]);
				g_variant_builder_add (builder, "y", resp_auth->authentication_key[i]);
			}
			ak = g_variant_builder_end(builder);
			ak_gv = g_variant_new("v", ak);

			builder = g_variant_builder_new (G_VARIANT_TYPE ("ay"));
			for(i = 0; i < (int)resp_auth->cipher_length; i++) {
				dbg("resp_auth->cipher_data[%d][0x%02x]", i,resp_auth->cipher_data[i]);
				g_variant_builder_add (builder, "y", resp_auth->cipher_data[i]);
			}
			cp = g_variant_builder_end(builder);
			cp_gv = g_variant_new("v", cp);

			builder = g_variant_builder_new (G_VARIANT_TYPE ("ay"));
			for(i = 0; i < (int)resp_auth->integrity_length; i++) {
				dbg("resp_auth->integrity_data[%d][0x%02x]", i,resp_auth->integrity_data[i]);
				g_variant_builder_add (builder, "y", resp_auth->integrity_data[i]);
			}
			it = g_variant_builder_end(builder);
			it_gv = g_variant_new("v", it);

			builder = g_variant_builder_new (G_VARIANT_TYPE ("ay"));
			for(i = 0; i < (int)resp_auth->resp_length; i++) {
				dbg("resp_auth->resp_data[%d][0x%02x]", i,resp_auth->resp_data[i]);
				g_variant_builder_add (builder, "y", resp_auth->resp_data[i]);
			}
			resp = g_variant_builder_end(builder);
			resp_gv = g_variant_new("v", resp);

			telephony_sim_complete_authentication (dbus_info->interface_object, dbus_info->invocation,
					resp_auth->result,
					resp_auth->auth_type,
					resp_auth->auth_result,
					ak_gv,
					cp_gv,
					it_gv,
					resp_gv);
		}
			break;

		case TRESP_SIM_VERIFY_PINS:
			dbg("resp comm - TRESP_SIM_VERIFY_PINS");
			telephony_sim_complete_verify_sec(dbus_info->interface_object, dbus_info->invocation,
					resp_verify_pins->result,
					resp_verify_pins->pin_type,
					resp_verify_pins->retry_count);
			break;

		case TRESP_SIM_VERIFY_PUKS:
			dbg("resp comm - TRESP_SIM_VERIFY_PUKS");
			telephony_sim_complete_verify_puk (dbus_info->interface_object, dbus_info->invocation,
					resp_verify_puks->result,
					resp_verify_puks->pin_type,
					resp_verify_puks->retry_count);
			break;

		case TRESP_SIM_CHANGE_PINS:
			dbg("resp comm - TRESP_SIM_CHANGE_PINS");
			telephony_sim_complete_change_pin(dbus_info->interface_object, dbus_info->invocation,
					resp_change_pins->result,
					resp_change_pins->pin_type,
					resp_change_pins->retry_count);
			break;

		case TRESP_SIM_DISABLE_FACILITY:
			dbg("resp comm - TRESP_SIM_DISABLE_FACILITY");
			dbg("resp_dis_facility->type[%d]", resp_dis_facility->type);
			switch (resp_dis_facility->type) {
				case SIM_FACILITY_PS:
					f_type = 1;
					break;
				case SIM_FACILITY_SC:
					f_type = 3;
					break;
				case SIM_FACILITY_FD:
					f_type = 4;
					break;
				case SIM_FACILITY_PN:
					f_type = 5;
					break;
				case SIM_FACILITY_PU:
					f_type = 6;
					break;
				case SIM_FACILITY_PP:
					f_type = 7;
					break;
				case SIM_FACILITY_PC:
					f_type = 8;
					break;
				default:
					dbg("error - not handled type[%d]", resp_dis_facility->type);
					break;
			}
			telephony_sim_complete_disable_facility(dbus_info->interface_object, dbus_info->invocation,
					resp_dis_facility->result,
					f_type,
					resp_dis_facility->retry_count);
			break;

		case TRESP_SIM_ENABLE_FACILITY:
			dbg("resp comm - TRESP_SIM_ENABLE_FACILITY");
			dbg("resp_en_facility->type[%d]", resp_en_facility->type);
			switch (resp_en_facility->type) {
				case SIM_FACILITY_PS:
					f_type = 1;
					break;
				case SIM_FACILITY_SC:
					f_type = 3;
					break;
				case SIM_FACILITY_FD:
					f_type = 4;
					break;
				case SIM_FACILITY_PN:
					f_type = 5;
					break;
				case SIM_FACILITY_PU:
					f_type = 6;
					break;
				case SIM_FACILITY_PP:
					f_type = 7;
					break;
				case SIM_FACILITY_PC:
					f_type = 8;
					break;
				default:
					dbg("error - not handled type[%d]", resp_en_facility->type);
					break;
			}
			telephony_sim_complete_enable_facility(dbus_info->interface_object, dbus_info->invocation,
					resp_en_facility->result,
					f_type,
					resp_en_facility->retry_count);
			break;

		case TRESP_SIM_GET_FACILITY_STATUS:
			dbg("resp comm - TRESP_SIM_GET_FACILITY_STATUS");
			dbg("resp_get_facility->type[%d]", resp_get_facility->type);
			switch (resp_get_facility->type) {
				case SIM_FACILITY_PS:
					f_type = 1;
					break;
				case SIM_FACILITY_SC:
					f_type = 3;
					break;
				case SIM_FACILITY_FD:
					f_type = 4;
					break;
				case SIM_FACILITY_PN:
					f_type = 5;
					break;
				case SIM_FACILITY_PU:
					f_type = 6;
					break;
				case SIM_FACILITY_PP:
					f_type = 7;
					break;
				case SIM_FACILITY_PC:
					f_type = 8;
					break;
				default:
					dbg("error - not handled type[%d]", resp_get_facility->type);
					break;
			}
			telephony_sim_complete_get_facility(dbus_info->interface_object, dbus_info->invocation,
					resp_get_facility->result,
					f_type,
					resp_get_facility->b_enable);
			break;

		case TRESP_SIM_GET_LOCK_INFO:
			dbg("resp comm - TRESP_SIM_GET_LOCK_INFO");
			dbg("resp_lock->type[%d]", resp_lock->type);
			switch (resp_lock->type) {
				case SIM_FACILITY_PS:
					f_type = 1;
					break;
				case SIM_FACILITY_SC:
					f_type = 3;
					break;
				case SIM_FACILITY_FD:
					f_type = 4;
					break;
				case SIM_FACILITY_PN:
					f_type = 5;
					break;
				case SIM_FACILITY_PU:
					f_type = 6;
					break;
				case SIM_FACILITY_PP:
					f_type = 7;
					break;
				case SIM_FACILITY_PC:
					f_type = 8;
					break;
				default:
					dbg("error - not handled type[%d]", resp_lock->type);
					break;
			}
			telephony_sim_complete_get_lock_info(dbus_info->interface_object, dbus_info->invocation,
					resp_lock->result,
					f_type,
					resp_lock->lock_status,
					resp_lock->retry_count);
			break;

		case TRESP_SIM_TRANSMIT_APDU: {
			GVariantBuilder *builder = NULL;
			GVariant * apdu_gv = NULL;
			GVariant *inner_gv = NULL;
			int i =0;

			dbg("resp comm - TRESP_SIM_TRANSMIT_APDU");
			builder = g_variant_builder_new (G_VARIANT_TYPE ("ay"));
			for(i = 0; i < (int)resp_apdu->apdu_resp_length; i++) {
				dbg("resp_apdu->apdu_resp[%d][0x%02x]", i,resp_apdu->apdu_resp[i]);
				g_variant_builder_add (builder, "y", resp_apdu->apdu_resp[i]);
			}
			inner_gv = g_variant_builder_end(builder);
/*			g_variant_builder_unref (builder);*/
			apdu_gv = g_variant_new("v", inner_gv);

			telephony_sim_complete_transfer_apdu(dbus_info->interface_object, dbus_info->invocation,
					resp_apdu->result,
					apdu_gv);
		}
			break;

		case TRESP_SIM_GET_ATR:{
			GVariantBuilder *builder = NULL;
			GVariant * atr_gv = NULL;
			GVariant *inner_gv = NULL;
			int i =0;

			dbg("resp comm - TRESP_SIM_GET_ATR");
			builder = g_variant_builder_new (G_VARIANT_TYPE ("ay"));
			for(i = 0; i < (int)resp_get_atr->atr_length; i++) {
				dbg("resp_apdu->apdu_resp[%d][0x%02x]", i,resp_get_atr->atr[i]);
				g_variant_builder_add (builder, "y", resp_get_atr->atr[i]);
			}
			inner_gv = g_variant_builder_end(builder);
/*			g_variant_builder_unref (builder);*/
			atr_gv = g_variant_new("v", inner_gv);

			telephony_sim_complete_get_atr(dbus_info->interface_object, dbus_info->invocation,
					resp_get_atr->result,
					atr_gv);
		}
			break;

		default:
			dbg("not handled TRESP type[%d]", command);
			break;
	}
	return TRUE;
}

gboolean dbus_plugin_sim_notification(struct custom_data *ctx, const char *plugin_name,
		TelephonyObjectSkeleton *object, enum tcore_notification_command command,
		unsigned int data_len, const void *data)
{
	TelephonySim *sim;
	const struct tnoti_sim_status *n_sim_status = data;

	if (!object) {
		dbg("object is NULL");
		return FALSE;
	}

	sim = telephony_object_peek_sim(TELEPHONY_OBJECT(object));
	dbg("sim = %p", sim);

	dbg("notification !!! (command = 0x%x, data_len = %d)", command, data_len);

	switch (command) {
		case TNOTI_SIM_STATUS:
			dbg("notified sim_status[%d]", n_sim_status->sim_status);
			dbus_sim_data_request(ctx, n_sim_status->sim_status);
			telephony_sim_emit_status (sim, n_sim_status->sim_status);
			break;

		default:
		dbg("not handled command[%d]", command);
		break;
	}

	return TRUE;
}
