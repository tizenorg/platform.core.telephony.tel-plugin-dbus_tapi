#include <stdio.h>
#include <string.h>
#include <pthread.h>
#include <unistd.h>
#include <stdlib.h>
#include <time.h>
#include <glib.h>
#include <glib-object.h>
#include <gio/gio.h>

#include <tcore.h>
#include <server.h>
#include <plugin.h>
#include <hal.h>
#include <communicator.h>
#include <storage.h>
#include <queue.h>
#include <user_request.h>
#include <co_ss.h>
#include <co_sim.h>
#include <co_ps.h>

#include "generated-code.h"
#include "common.h"



static gboolean
on_ss_activate_barring (TelephonySs *ss,
		GDBusMethodInvocation *invocation,
		gint ss_class,
		gint barring_mode,
		const gchar *barring_password,
		gpointer user_data)
{
	char buf[5];
	struct treq_ss_barring req;
	struct custom_data *ctx = user_data;
	UserRequest *ur = NULL;

	memset(&req, 0, sizeof(struct treq_ss_barring));

	req.class = ss_class;
	req.mode = barring_mode;

	memcpy(req.password, barring_password, MAX_SS_BARRING_PASSWORD_LEN);
	memcpy(buf, barring_password, MAX_SS_BARRING_PASSWORD_LEN);

	buf[4] = 0;
	dbg("req.password = [%s]", buf);

	dbg("class = %d, mode = %d", req.class, req.mode);

	ur = MAKE_UR(ctx, ss, invocation);

	tcore_user_request_set_data(ur, sizeof(struct treq_ss_barring), &req);
	tcore_user_request_set_command(ur, TREQ_SS_BARRING_ACTIVATE);
	tcore_communicator_dispatch_request(ctx->comm, ur);

	return TRUE;
}

static gboolean
on_ss_deactivate_barring (TelephonySs *ss,
		GDBusMethodInvocation *invocation,
		gint ss_class,
		gint barring_mode,
		const gchar *barring_password,
		gpointer user_data)
{
	char buf[5];
	struct treq_ss_barring req;
	struct custom_data *ctx = user_data;
	UserRequest *ur = NULL;

	memset(&req, 0, sizeof(struct treq_ss_barring));

	req.class = ss_class;
	req.mode = barring_mode;

	memcpy(req.password, barring_password, MAX_SS_BARRING_PASSWORD_LEN);
	memcpy(buf, barring_password, MAX_SS_BARRING_PASSWORD_LEN);

	buf[4] = 0;
	dbg("req.password = [%s]", buf);
	dbg("class = %d, mode = %d", req.class, req.mode);

	ur = MAKE_UR(ctx, ss, invocation);

	tcore_user_request_set_data(ur, sizeof(struct treq_ss_barring), &req);
	tcore_user_request_set_command(ur, TREQ_SS_BARRING_DEACTIVATE);
	tcore_communicator_dispatch_request(ctx->comm, ur);

	return TRUE;
}

static gboolean
on_ss_change_barring_password (TelephonySs *ss,
		GDBusMethodInvocation *invocation,
		const gchar *barring_password,
		const gchar *barring_password_new,
		const gchar *barring_password_confirm,
		gpointer user_data)
{
	char buf[5];
	struct treq_ss_barring_change_password req;
	struct custom_data *ctx = user_data;
	UserRequest *ur = NULL;

	memset(&req, 0, sizeof(struct treq_ss_barring_change_password));

	memcpy(req.password_old, barring_password, MAX_SS_BARRING_PASSWORD_LEN);
	memcpy(req.password_new, barring_password_new, MAX_SS_BARRING_PASSWORD_LEN);
	memcpy(req.password_confirm, barring_password_confirm, MAX_SS_BARRING_PASSWORD_LEN);

	memcpy(buf, barring_password, MAX_SS_BARRING_PASSWORD_LEN);
	buf[4] = 0;
	dbg("req.password_old = [%s]", buf);

	memcpy(buf, barring_password_new, MAX_SS_BARRING_PASSWORD_LEN);
	dbg("req.password_new = [%s]", buf);


	memcpy(buf, barring_password_confirm, MAX_SS_BARRING_PASSWORD_LEN);
	dbg("req.password_confirm = [%s]", buf);

	ur = MAKE_UR(ctx, ss, invocation);

	tcore_user_request_set_data(ur, sizeof(struct treq_ss_barring_change_password), &req);
	tcore_user_request_set_command(ur, TREQ_SS_BARRING_CHANGE_PASSWORD);
	tcore_communicator_dispatch_request(ctx->comm, ur);

	return TRUE;
}

static gboolean
on_ss_get_barring_status (TelephonySs *ss,
		GDBusMethodInvocation *invocation,
		gint ss_class,
		gint barring_mode,
		gpointer user_data)
{
	struct treq_ss_barring req;
	struct custom_data *ctx = user_data;
	UserRequest *ur = NULL;

	memset(&req, 0, sizeof(struct treq_ss_barring));

	req.class = ss_class;
	req.mode = barring_mode;

	dbg("class = %d, mode = %d", req.class, req.mode);

	ur = MAKE_UR(ctx, ss, invocation);

	tcore_user_request_set_data(ur, sizeof(struct treq_ss_barring), &req);
	tcore_user_request_set_command(ur, TREQ_SS_BARRING_GET_STATUS);
	tcore_communicator_dispatch_request(ctx->comm, ur);


	return TRUE;
}

static gboolean
on_ss_register_forwarding (TelephonySs *ss,
		GDBusMethodInvocation *invocation,
		gint ss_class,
		gint forward_mode,
		gint forward_no_reply_time,
		const gchar *forward_number,
		gpointer user_data)
{
	struct treq_ss_forwarding req;
	struct custom_data *ctx = user_data;
	UserRequest *ur = NULL;

	memset(&req, 0, sizeof(struct treq_ss_forwarding));

	req.class = ss_class;
	req.mode = forward_mode;
	req.time = forward_no_reply_time;
	snprintf(req.number, MAX_SS_FORWARDING_NUMBER_LEN, "%s", forward_number);

	dbg("class = %d, mode = %d, time = %d, number = %s",
			req.class, req.mode, req.time, req.number);

	ur = MAKE_UR(ctx, ss, invocation);

	tcore_user_request_set_data(ur, sizeof(struct treq_ss_forwarding), &req);
	tcore_user_request_set_command(ur, TREQ_SS_FORWARDING_REGISTER);
	tcore_communicator_dispatch_request(ctx->comm, ur);

	return TRUE;
}

static gboolean
on_ss_deregister_forwarding (TelephonySs *ss,
		GDBusMethodInvocation *invocation,
		gint ss_class,
		gint forward_mode,
		gint forward_no_reply_time,
		const gchar *forward_number,
		gpointer user_data)
{
	struct treq_ss_forwarding req;
	struct custom_data *ctx = user_data;
	UserRequest *ur = NULL;

	memset(&req, 0, sizeof(struct treq_ss_forwarding));

	req.class = ss_class;
	req.mode = forward_mode;
	req.time = forward_no_reply_time;
	snprintf(req.number, MAX_SS_FORWARDING_NUMBER_LEN, "%s", forward_number);

	dbg("class = %d, mode = %d, time = %d, number = %s",
			req.class, req.mode, req.time, req.number);

	ur = MAKE_UR(ctx, ss, invocation);

	tcore_user_request_set_data(ur, sizeof(struct treq_ss_forwarding), &req);
	tcore_user_request_set_command(ur, TREQ_SS_FORWARDING_DEREGISTER);
	tcore_communicator_dispatch_request(ctx->comm, ur);

	return TRUE;
}

static gboolean
on_ss_activate_forwarding (TelephonySs *ss,
		GDBusMethodInvocation *invocation,
		gint ss_class,
		gint forward_mode,
		gint forward_no_reply_time,
		const gchar *forward_number,
		gpointer user_data)
{
	struct treq_ss_forwarding req;
	struct custom_data *ctx = user_data;
	UserRequest *ur = NULL;

	memset(&req, 0, sizeof(struct treq_ss_forwarding));

	req.class = ss_class;
	req.mode = forward_mode;
	req.time = forward_no_reply_time;
	snprintf(req.number, MAX_SS_FORWARDING_NUMBER_LEN, "%s", forward_number);

	dbg("class = %d, mode = %d, time = %d, number = %s",
			req.class, req.mode, req.time, req.number);

	ur = MAKE_UR(ctx, ss, invocation);

	tcore_user_request_set_data(ur, sizeof(struct treq_ss_forwarding), &req);
	tcore_user_request_set_command(ur, TREQ_SS_FORWARDING_ACTIVATE);
	tcore_communicator_dispatch_request(ctx->comm, ur);

	return TRUE;
}

static gboolean
on_ss_deactivate_forwarding (TelephonySs *ss,
		GDBusMethodInvocation *invocation,
		gint ss_class,
		gint forward_mode,
		gint forward_no_reply_time,
		const gchar *forward_number,
		gpointer user_data)
{
	struct treq_ss_forwarding req;
	struct custom_data *ctx = user_data;
	UserRequest *ur = NULL;

	memset(&req, 0, sizeof(struct treq_ss_forwarding));

	req.class = ss_class;
	req.mode = forward_mode;
	req.time = forward_no_reply_time;
	snprintf(req.number, MAX_SS_FORWARDING_NUMBER_LEN, "%s", forward_number);

	dbg("class = %d, mode = %d, time = %d, number = %s",
			req.class, req.mode, req.time, req.number);

	ur = MAKE_UR(ctx, ss, invocation);

	tcore_user_request_set_data(ur, sizeof(struct treq_ss_forwarding), &req);
	tcore_user_request_set_command(ur, TREQ_SS_FORWARDING_DEACTIVATE);
	tcore_communicator_dispatch_request(ctx->comm, ur);

	return TRUE;
}

static gboolean
on_ss_get_forwarding_status (TelephonySs *ss,
		GDBusMethodInvocation *invocation,
		gint ss_class,
		gint forward_mode,
		gpointer user_data)
{
	struct treq_ss_forwarding req;
	struct custom_data *ctx = user_data;
	UserRequest *ur = NULL;

	memset(&req, 0, sizeof(struct treq_ss_forwarding));

	req.class = ss_class;
	req.mode = forward_mode;

	dbg("class = %d, mode = %d, time = %d, number = %s",
			req.class, req.mode, req.time, req.number);

	ur = MAKE_UR(ctx, ss, invocation);

	tcore_user_request_set_data(ur, sizeof(struct treq_ss_forwarding), &req);
	tcore_user_request_set_command(ur, TREQ_SS_FORWARDING_GET_STATUS);
	tcore_communicator_dispatch_request(ctx->comm, ur);

	return TRUE;
}

static gboolean
on_ss_activate_waiting (TelephonySs *ss,
		GDBusMethodInvocation *invocation,
		gint ss_class,
		gpointer user_data)
{
	struct treq_ss_waiting req;
	struct custom_data *ctx = user_data;
	UserRequest *ur = NULL;

	memset(&req, 0, sizeof(struct treq_ss_waiting));

	req.class = ss_class;

	dbg("class = %d", req.class);

	ur = MAKE_UR(ctx, ss, invocation);

	tcore_user_request_set_data(ur, sizeof(struct treq_ss_waiting), &req);
	tcore_user_request_set_command(ur, TREQ_SS_WAITING_ACTIVATE);
	tcore_communicator_dispatch_request(ctx->comm, ur);

	return TRUE;
}

static gboolean
on_ss_deactivate_waiting (TelephonySs *ss,
		GDBusMethodInvocation *invocation,
		gint ss_class,
		gpointer user_data)
{
	struct treq_ss_waiting req;
	struct custom_data *ctx = user_data;
	UserRequest *ur = NULL;

	memset(&req, 0, sizeof(struct treq_ss_waiting));

	req.class = ss_class;

	dbg("class = %d", req.class);

	ur = MAKE_UR(ctx, ss, invocation);

	tcore_user_request_set_data(ur, sizeof(struct treq_ss_waiting), &req);
	tcore_user_request_set_command(ur, TREQ_SS_WAITING_DEACTIVATE);
	tcore_communicator_dispatch_request(ctx->comm, ur);

	return TRUE;
}

static gboolean
on_ss_get_waiting_status (TelephonySs *ss,
		GDBusMethodInvocation *invocation,
		gint ss_class,
		gpointer user_data)
{
	struct treq_ss_waiting req;
	struct custom_data *ctx = user_data;
	UserRequest *ur = NULL;

	memset(&req, 0, sizeof(struct treq_ss_waiting));

	req.class = ss_class;

	dbg("class = %d", req.class);

	ur = MAKE_UR(ctx, ss, invocation);

	tcore_user_request_set_data(ur, sizeof(struct treq_ss_waiting), &req);
	tcore_user_request_set_command(ur, TREQ_SS_WAITING_GET_STATUS);
	tcore_communicator_dispatch_request(ctx->comm, ur);

	return TRUE;
}

static gboolean
on_ss_get_cli_status (TelephonySs *ss,
		GDBusMethodInvocation *invocation,
		gint cli_type,
		gpointer user_data)
{
	struct treq_ss_cli req;
	struct custom_data *ctx = user_data;
	UserRequest *ur = NULL;

	memset(&req, 0, sizeof(struct treq_ss_cli));

	req.type = cli_type;

	dbg("type = %d", req.type);

	ur = MAKE_UR(ctx, ss, invocation);

	tcore_user_request_set_data(ur, sizeof(struct treq_ss_cli), &req);
	tcore_user_request_set_command(ur, TREQ_SS_CLI_GET_STATUS);
	tcore_communicator_dispatch_request(ctx->comm, ur);

	return TRUE;
}

static gboolean
on_ss_send_ussd (TelephonySs *ss,
		GDBusMethodInvocation *invocation,
		gint ussd_type,
		gint ussd_len,
		const gchar *ussd_string,
		gpointer user_data)
{
	struct treq_ss_ussd req;
	struct custom_data *ctx = user_data;
	UserRequest *ur = NULL;

	memset(&req, 0, sizeof(struct treq_ss_ussd));

	req.type = SS_USSD_TYPE_USER_INITIATED;
	snprintf(req.str, MAX_SS_USSD_LEN, "%s", ussd_string);

	dbg("type = %d, string = %s", req.type, req.str);

	ur = MAKE_UR(ctx, ss, invocation);

	tcore_user_request_set_data(ur, sizeof(struct treq_ss_ussd), &req);
	tcore_user_request_set_command(ur, TREQ_SS_SEND_USSD);
	tcore_communicator_dispatch_request(ctx->comm, ur);

	return TRUE;
}

gboolean dbus_plugin_setup_ss_interface(TelephonyObjectSkeleton *object, struct custom_data *ctx)
{
	TelephonySs *ss;

	ss = telephony_ss_skeleton_new();
	telephony_object_skeleton_set_ss(object, ss);
	g_object_unref(ss);

	g_signal_connect (ss,
			"handle-activate-barring",
			G_CALLBACK (on_ss_activate_barring),
			ctx);

	g_signal_connect (ss,
			"handle-deactivate-barring",
			G_CALLBACK (on_ss_deactivate_barring),
			ctx);

	g_signal_connect (ss,
			"handle-change-barring-password",
			G_CALLBACK (on_ss_change_barring_password),
			ctx);

	g_signal_connect (ss,
			"handle-get-barring-status",
			G_CALLBACK (on_ss_get_barring_status),
			ctx);

	g_signal_connect (ss,
			"handle-register-forwarding",
			G_CALLBACK (on_ss_register_forwarding),
			ctx);

	g_signal_connect (ss,
			"handle-deregister-forwarding",
			G_CALLBACK (on_ss_deregister_forwarding),
			ctx);

	g_signal_connect (ss,
			"handle-activate-forwarding",
			G_CALLBACK (on_ss_activate_forwarding),
			ctx);

	g_signal_connect (ss,
			"handle-deactivate-forwarding",
			G_CALLBACK (on_ss_deactivate_forwarding),
			ctx);

	g_signal_connect (ss,
			"handle-get-forwarding-status",
			G_CALLBACK (on_ss_get_forwarding_status),
			ctx);

	g_signal_connect (ss,
			"handle-activate-waiting",
			G_CALLBACK (on_ss_activate_waiting),
			ctx);

	g_signal_connect (ss,
			"handle-deactivate-waiting",
			G_CALLBACK (on_ss_deactivate_waiting),
			ctx);

	g_signal_connect (ss,
			"handle-get-waiting-status",
			G_CALLBACK (on_ss_get_waiting_status),
			ctx);

	g_signal_connect (ss,
			"handle-get-clistatus",
			G_CALLBACK (on_ss_get_cli_status),
			ctx);

	g_signal_connect (ss,
			"handle-send-ussd",
			G_CALLBACK (on_ss_send_ussd),
			ctx);

	return TRUE;
}

gboolean dbus_plugin_ss_response(struct custom_data *ctx, UserRequest *ur, struct dbus_request_info *dbus_info, enum tcore_response_command command, unsigned int data_len, const void *data)
{
	GVariant *result = 0;
	GVariantBuilder b;
	int i = 0;

	switch (command) {
		case TRESP_SS_BARRING_ACTIVATE: {

			const struct tresp_ss_barring *resp = data;

			dbg("receive TRESP_SS_BARRING_ACTIVATE");
			dbg("resp->err = 0x%x", resp->err);

			g_variant_builder_init(&b, G_VARIANT_TYPE("aa{sv}"));

			for (i=0; i<resp->record_num; i++) {
				g_variant_builder_open(&b, G_VARIANT_TYPE("a{sv}"));
				g_variant_builder_add(&b, "{sv}", "ss_class", g_variant_new_int32( resp->record[i].class ));
				g_variant_builder_add(&b, "{sv}", "ss_status", g_variant_new_int32( resp->record[i].status ));
				g_variant_builder_add(&b, "{sv}", "barring_mode", g_variant_new_int32( resp->record[i].mode ));
				g_variant_builder_close(&b);
			}

			result = g_variant_builder_end(&b);

			telephony_ss_complete_activate_barring(dbus_info->interface_object, dbus_info->invocation, result, resp->err);

			g_variant_unref(result);

		} break;

		case TRESP_SS_BARRING_DEACTIVATE: {

			const struct tresp_ss_barring *resp = data;

			dbg("receive TRESP_SS_BARRING_DEACTIVATE");
			dbg("resp->err = 0x%x", resp->err);

			g_variant_builder_init(&b, G_VARIANT_TYPE("aa{sv}"));

			for (i=0; i<resp->record_num; i++) {
				g_variant_builder_open(&b, G_VARIANT_TYPE("a{sv}"));
				g_variant_builder_add(&b, "{sv}", "ss_class", g_variant_new_int32( resp->record[i].class ));
				g_variant_builder_add(&b, "{sv}", "ss_status", g_variant_new_int32( resp->record[i].status ));
				g_variant_builder_add(&b, "{sv}", "barring_mode", g_variant_new_int32( resp->record[i].mode ));
				g_variant_builder_close(&b);
			}

			result = g_variant_builder_end(&b);

			telephony_ss_complete_deactivate_barring(dbus_info->interface_object, dbus_info->invocation, result, resp->err);

			g_variant_unref(result);

		} break;

		case TRESP_SS_BARRING_CHANGE_PASSWORD: {

			const struct tresp_ss_general *resp = data;

			dbg("receive TRESP_SS_BARRING_CHANGE_PASSWORD");
			dbg("resp->err = 0x%x", resp->err);

			telephony_ss_complete_change_barring_password(dbus_info->interface_object, dbus_info->invocation, resp->err);

		} break;

		case TRESP_SS_BARRING_GET_STATUS: {

			const struct tresp_ss_barring *resp = data;

			dbg("receive TRESP_SS_BARRING_GET_STATUS");
			dbg("resp->err = 0x%x", resp->err);

			g_variant_builder_init(&b, G_VARIANT_TYPE("aa{sv}"));

			for (i=0; i<resp->record_num; i++) {
				g_variant_builder_open(&b, G_VARIANT_TYPE("a{sv}"));
				g_variant_builder_add(&b, "{sv}", "ss_class", g_variant_new_int32( resp->record[i].class ));
				g_variant_builder_add(&b, "{sv}", "ss_status", g_variant_new_int32( resp->record[i].status ));
				g_variant_builder_add(&b, "{sv}", "barring_mode", g_variant_new_int32( resp->record[i].mode ));
				g_variant_builder_close(&b);
			}

			result = g_variant_builder_end(&b);

			telephony_ss_complete_get_barring_status(dbus_info->interface_object, dbus_info->invocation, result, resp->err);

			g_variant_unref(result);

		} break;

		case TRESP_SS_FORWARDING_ACTIVATE: {
			const struct tresp_ss_forwarding *resp = data;

			dbg("receive TRESP_SS_FORWARDING_ACTIVATE");
			dbg("resp->err = 0x%x", resp->err);

			g_variant_builder_init(&b, G_VARIANT_TYPE("aa{sv}"));

			for (i=0; i<resp->record_num; i++) {
				g_variant_builder_open(&b, G_VARIANT_TYPE("a{sv}"));
				g_variant_builder_add(&b, "{sv}", "ss_class", g_variant_new_int32( resp->record[i].class ));
				g_variant_builder_add(&b, "{sv}", "ss_status", g_variant_new_int32( resp->record[i].status ));
				g_variant_builder_add(&b, "{sv}", "forwarding_mode", g_variant_new_int32( resp->record[i].mode ));
				g_variant_builder_add(&b, "{sv}", "no_reply_time", g_variant_new_int32( resp->record[i].time ));
				g_variant_builder_add(&b, "{sv}", "number_present", g_variant_new_int32( resp->record[i].number_present ));
				g_variant_builder_add(&b, "{sv}", "forwarding_number", g_variant_new_string( resp->record[i].number ));
				g_variant_builder_close(&b);
			}

			result = g_variant_builder_end(&b);

			telephony_ss_complete_activate_forwarding(dbus_info->interface_object, dbus_info->invocation, result, resp->err);

			g_variant_unref(result);

		} break;

		case TRESP_SS_FORWARDING_DEACTIVATE: {

			const struct tresp_ss_forwarding *resp = data;

			dbg("receive TRESP_SS_FORWARDING_DEACTIVATE");
			dbg("resp->err = 0x%x", resp->err);

			g_variant_builder_init(&b, G_VARIANT_TYPE("aa{sv}"));

			for (i=0; i<resp->record_num; i++) {
				g_variant_builder_open(&b, G_VARIANT_TYPE("a{sv}"));
				g_variant_builder_add(&b, "{sv}", "ss_class", g_variant_new_int32( resp->record[i].class ));
				g_variant_builder_add(&b, "{sv}", "ss_status", g_variant_new_int32( resp->record[i].status ));
				g_variant_builder_add(&b, "{sv}", "forwarding_mode", g_variant_new_int32( resp->record[i].mode ));
				g_variant_builder_add(&b, "{sv}", "no_reply_time", g_variant_new_int32( resp->record[i].time ));
				g_variant_builder_add(&b, "{sv}", "number_present", g_variant_new_int32( resp->record[i].number_present ));
				g_variant_builder_add(&b, "{sv}", "forwarding_number", g_variant_new_string( resp->record[i].number ));
				g_variant_builder_close(&b);
			}

			result = g_variant_builder_end(&b);

			telephony_ss_complete_deactivate_forwarding(dbus_info->interface_object, dbus_info->invocation, result, resp->err);

			g_variant_unref(result);

		} break;

		case TRESP_SS_FORWARDING_REGISTER: {

			const struct tresp_ss_forwarding *resp = data;

			dbg("receive TRESP_SS_FORWARDING_REGISTER");
			dbg("resp->err = 0x%x", resp->err);

			g_variant_builder_init(&b, G_VARIANT_TYPE("aa{sv}"));

			for (i=0; i<resp->record_num; i++) {
				g_variant_builder_open(&b, G_VARIANT_TYPE("a{sv}"));
				g_variant_builder_add(&b, "{sv}", "ss_class", g_variant_new_int32( resp->record[i].class ));
				g_variant_builder_add(&b, "{sv}", "ss_status", g_variant_new_int32( resp->record[i].status ));
				g_variant_builder_add(&b, "{sv}", "forwarding_mode", g_variant_new_int32( resp->record[i].mode ));
				g_variant_builder_add(&b, "{sv}", "no_reply_time", g_variant_new_int32( resp->record[i].time ));
				g_variant_builder_add(&b, "{sv}", "number_present", g_variant_new_int32( resp->record[i].number_present ));
				g_variant_builder_add(&b, "{sv}", "forwarding_number", g_variant_new_string( resp->record[i].number ));
				g_variant_builder_close(&b);
			}

			result = g_variant_builder_end(&b);

			telephony_ss_complete_register_forwarding(dbus_info->interface_object, dbus_info->invocation, result, resp->err);

			g_variant_unref(result);

	    } break;

		case TRESP_SS_FORWARDING_DEREGISTER: {

			const struct tresp_ss_forwarding *resp = data;

			dbg("receive TRESP_SS_FORWARDING_DEREGISTER");
			dbg("resp->err = 0x%x", resp->err);

			g_variant_builder_init(&b, G_VARIANT_TYPE("aa{sv}"));

			for (i=0; i<resp->record_num; i++) {
				g_variant_builder_open(&b, G_VARIANT_TYPE("a{sv}"));
				g_variant_builder_add(&b, "{sv}", "ss_class", g_variant_new_int32( resp->record[i].class ));
				g_variant_builder_add(&b, "{sv}", "ss_status", g_variant_new_int32( resp->record[i].status ));
				g_variant_builder_add(&b, "{sv}", "forwarding_mode", g_variant_new_int32( resp->record[i].mode ));
				g_variant_builder_add(&b, "{sv}", "no_reply_time", g_variant_new_int32( resp->record[i].time ));
				g_variant_builder_add(&b, "{sv}", "number_present", g_variant_new_int32( resp->record[i].number_present ));
				g_variant_builder_add(&b, "{sv}", "forwarding_number", g_variant_new_string( resp->record[i].number ));
				g_variant_builder_close(&b);
			}

			result = g_variant_builder_end(&b);

			telephony_ss_complete_deregister_forwarding(dbus_info->interface_object, dbus_info->invocation, result, resp->err);

			g_variant_unref(result);

		} break;

		case TRESP_SS_FORWARDING_GET_STATUS: {

			const struct tresp_ss_forwarding *resp = data;

			dbg("receive TRESP_SS_FORWARDING_GET_STATUS");
			dbg("resp->err = 0x%x", resp->err);

			g_variant_builder_init(&b, G_VARIANT_TYPE("aa{sv}"));

			for (i=0; i<resp->record_num; i++) {
				g_variant_builder_open(&b, G_VARIANT_TYPE("a{sv}"));
				g_variant_builder_add(&b, "{sv}", "ss_class", g_variant_new_int32( resp->record[i].class ));
				g_variant_builder_add(&b, "{sv}", "ss_status", g_variant_new_int32( resp->record[i].status ));
				g_variant_builder_add(&b, "{sv}", "forwarding_mode", g_variant_new_int32( resp->record[i].mode ));
				g_variant_builder_add(&b, "{sv}", "no_reply_time", g_variant_new_int32( resp->record[i].time ));
				g_variant_builder_add(&b, "{sv}", "number_present", g_variant_new_int32( resp->record[i].number_present ));
				g_variant_builder_add(&b, "{sv}", "forwarding_number", g_variant_new_string( resp->record[i].number ));
				g_variant_builder_close(&b);
			}

			result = g_variant_builder_end(&b);

			telephony_ss_complete_get_forwarding_status(dbus_info->interface_object, dbus_info->invocation, result, resp->err);

			g_variant_unref(result);

		} break;

		case TRESP_SS_WAITING_ACTIVATE: {

			const struct tresp_ss_waiting *resp = data;

			dbg("receive TRESP_SS_WAITING_ACTIVATE");
			dbg("resp->err = 0x%x", resp->err);

			g_variant_builder_init(&b, G_VARIANT_TYPE("aa{sv}"));

			for (i=0; i<resp->record_num; i++) {
				g_variant_builder_open(&b, G_VARIANT_TYPE("a{sv}"));
				g_variant_builder_add(&b, "{sv}", "ss_class", g_variant_new_int32( resp->record[i].class ));
				g_variant_builder_add(&b, "{sv}", "ss_status", g_variant_new_int32( resp->record[i].status ));
				g_variant_builder_close(&b);
			}

			result = g_variant_builder_end(&b);

			telephony_ss_complete_activate_waiting(dbus_info->interface_object, dbus_info->invocation, result, resp->err);

			g_variant_unref(result);

		} break;

		case TRESP_SS_WAITING_DEACTIVATE: {

			const struct tresp_ss_waiting *resp = data;

			dbg("receive TRESP_SS_WAITING_DEACTIVATE");
			dbg("resp->err = 0x%x", resp->err);

			g_variant_builder_init(&b, G_VARIANT_TYPE("aa{sv}"));

			for (i=0; i<resp->record_num; i++) {
				g_variant_builder_open(&b, G_VARIANT_TYPE("a{sv}"));
				g_variant_builder_add(&b, "{sv}", "ss_class", g_variant_new_int32( resp->record[i].class ));
				g_variant_builder_add(&b, "{sv}", "ss_status", g_variant_new_int32( resp->record[i].status ));
				g_variant_builder_close(&b);
			}

			result = g_variant_builder_end(&b);

			telephony_ss_complete_deactivate_waiting(dbus_info->interface_object, dbus_info->invocation, result, resp->err);

			g_variant_unref(result);

		} break;

		case TRESP_SS_WAITING_GET_STATUS: {

			const struct tresp_ss_waiting *resp = data;

			dbg("receive TRESP_SS_WAITING_GET_STATUS");
			dbg("resp->err = 0x%x", resp->err);

			g_variant_builder_init(&b, G_VARIANT_TYPE("aa{sv}"));

			for (i=0; i<resp->record_num; i++) {
				g_variant_builder_open(&b, G_VARIANT_TYPE("a{sv}"));
				g_variant_builder_add(&b, "{sv}", "ss_class", g_variant_new_int32( resp->record[i].class ));
				g_variant_builder_add(&b, "{sv}", "ss_status", g_variant_new_int32( resp->record[i].status ));
				g_variant_builder_close(&b);
			}

			result = g_variant_builder_end(&b);

			telephony_ss_complete_get_waiting_status(dbus_info->interface_object, dbus_info->invocation, result, resp->err);

			g_variant_unref(result);

		} break;

		case TRESP_SS_CLI_GET_STATUS: {

			const struct tresp_ss_cli *resp = data;

			dbg("receive TRESP_SS_CLI_GET_STATUS");
			dbg("resp->err = 0x%x", resp->err);

			telephony_ss_complete_get_clistatus(dbus_info->interface_object, dbus_info->invocation, resp->err, resp->type, resp->status);

		} break;

		case TRESP_SS_SEND_USSD: {

			const struct tresp_ss_ussd *resp = data;

			dbg("receive TRESP_SS_SEND_USSD");
			dbg("resp->err = 0x%x", resp->err);

			if ( resp->err ) {
				dbg("USSD Request is failed");
				telephony_ss_complete_send_ussd(dbus_info->interface_object, dbus_info->invocation, resp->err, resp->type, resp->status, -1, 0);

			} else {
				int ussd_len = strlen(resp->str);
				dbg("USSD Request is Success");
				dbg("USSD : %s (%d)", resp->str, ussd_len);
				telephony_ss_complete_send_ussd(dbus_info->interface_object, dbus_info->invocation, resp->err, resp->type, resp->status, ussd_len, resp->str);

			}

		} break;

		default:
			dbg("not handled command[%d]", command);
		break;
	}

	return TRUE;
}

gboolean dbus_plugin_ss_notification(struct custom_data *ctx, const char *plugin_name, TelephonyObjectSkeleton *object, enum tcore_notification_command command, unsigned int data_len, const void *data)
{
	TelephonySs *ss;
	const struct tnoti_ss_ussd *ussd = data;

	if (!object) {
		dbg("object is NULL");
		return FALSE;
	}

	ss = telephony_object_peek_ss(TELEPHONY_OBJECT(object));
	dbg("ss = %p", ss);

	switch (command) {
		case TNOTI_SS_USSD:
			telephony_ss_emit_notify_ussd(ss,
					ussd->status,
					strlen(ussd->str),
					ussd->str );
			break;

		default:
			dbg("not handled command[%d]", command);
		break;
	}

	return TRUE;
}

