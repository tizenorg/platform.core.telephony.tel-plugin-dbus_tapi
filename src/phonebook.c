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
#include <glib-object.h>

#include <tcore.h>
#include <server.h>
#include <plugin.h>
#include <hal.h>
#include <communicator.h>
#include <core_object.h>
#include <queue.h>
#include <user_request.h>
#include <util.h>
#include <co_phonebook.h>
#include <co_sim.h>

#include "generated-code.h"
#include "common.h"

static gboolean on_phonebook_get_init_status(TelephonyPhonebook *phonebook, GDBusMethodInvocation *invocation, gpointer user_data)
{
	struct custom_data *ctx = user_data;
	gboolean pb_status = FALSE;
	struct tel_phonebook_support_list *list = NULL;
	GSList *co_list = NULL;
	CoreObject *co_pb = NULL;
	TcorePlugin *plugin = NULL;

	dbg("Func Entrance");

	plugin = tcore_server_find_plugin(ctx->server, TCORE_PLUGIN_DEFAULT);
	co_list = tcore_plugin_get_core_objects_bytype(plugin, CORE_OBJECT_TYPE_PHONEBOOK);
	if (!co_list) {
		dbg("error- co_list is NULL");
	}
	co_pb = (CoreObject *)co_list->data;
	g_slist_free(co_list);

	if (!co_pb) {
		dbg("error- co_pb is NULL");
	}

	pb_status = tcore_phonebook_get_status(co_pb);
	list = tcore_phonebook_get_support_list(co_pb);

	telephony_phonebook_complete_get_init_status(phonebook, invocation,
			pb_status,
			list->b_fdn,
			list->b_adn,
			list->b_sdn,
			list->b_usim,
			list->b_aas,
			list->b_gas);

	return TRUE;
}

static gboolean on_phonebook_get_count(TelephonyPhonebook *phonebook, GDBusMethodInvocation *invocation, gint arg_req_type, gpointer user_data)
{
	struct custom_data *ctx = user_data;
	UserRequest *ur = NULL;

	struct treq_phonebook_get_count pb_count;

	ur = MAKE_UR(ctx, phonebook, invocation);
	memset(&pb_count, 0, sizeof(struct treq_phonebook_get_count));

	pb_count.phonebook_type = arg_req_type;

	tcore_user_request_set_data(ur, sizeof(struct treq_phonebook_get_count), &pb_count);
	tcore_user_request_set_command(ur, TREQ_PHONEBOOK_GETCOUNT);
	tcore_communicator_dispatch_request(ctx->comm, ur);

	return TRUE;
}

static gboolean on_phonebook_get_info(TelephonyPhonebook *phonebook, GDBusMethodInvocation *invocation, gint arg_req_type, gpointer user_data)
{
	struct custom_data *ctx = user_data;
	UserRequest *ur = NULL;

	struct treq_phonebook_get_info pb_info;

	ur = MAKE_UR(ctx, phonebook, invocation);
	memset(&pb_info, 0, sizeof(struct treq_phonebook_get_info));

	pb_info.phonebook_type = arg_req_type;

	tcore_user_request_set_data(ur, sizeof(struct treq_phonebook_get_info), &pb_info);
	tcore_user_request_set_command(ur, TREQ_PHONEBOOK_GETMETAINFO);
	tcore_communicator_dispatch_request(ctx->comm, ur);

	return TRUE;
}

static gboolean on_phonebook_get_usim_info(TelephonyPhonebook *phonebook, GDBusMethodInvocation *invocation, gpointer user_data)
{
	struct custom_data *ctx = user_data;
	UserRequest *ur = NULL;
	ur = MAKE_UR(ctx, phonebook, invocation);
	tcore_user_request_set_data(ur, 0, NULL);
	tcore_user_request_set_command(ur, TREQ_PHONEBOOK_GETUSIMINFO);
	tcore_communicator_dispatch_request(ctx->comm, ur);
	return TRUE;
}

static gboolean on_phonebook_read_record(TelephonyPhonebook *phonebook, GDBusMethodInvocation *invocation,
		gint arg_req_type, gint arg_index, gpointer user_data)
{
	struct custom_data *ctx = user_data;
	UserRequest *ur = NULL;

	struct treq_phonebook_read_record pb_read;

	ur = MAKE_UR(ctx, phonebook, invocation);
	memset(&pb_read, 0, sizeof(struct treq_phonebook_read_record));

	pb_read.index = (unsigned short)arg_index;
	pb_read.phonebook_type = arg_req_type;

	tcore_user_request_set_data(ur, sizeof(struct treq_phonebook_read_record), &pb_read);
	tcore_user_request_set_command(ur, TREQ_PHONEBOOK_READRECORD);
	tcore_communicator_dispatch_request(ctx->comm, ur);

	return TRUE;
}

static gboolean on_phonebook_update_record(TelephonyPhonebook *phonebook, GDBusMethodInvocation *invocation,
		gint arg_type, gint arg_index, const gchar *arg_name, gint arg_dcs,
		const gchar *arg_number, gint arg_ton, const gchar *arg_number2, gint arg_number2_ton,
		const gchar *arg_number3,gint arg_number3_ton, const gchar *arg_number4, gint arg_number4_ton,
		const gchar *arg_email1, const gchar *arg_email2, const gchar *arg_email3, const gchar *arg_email4,
		gint arg_group_index, gpointer user_data)
{
	struct custom_data *ctx = user_data;
	UserRequest *ur = NULL;

	struct treq_phonebook_update_record pb_update;

	ur = MAKE_UR(ctx, phonebook, invocation);
	memset(&pb_update, 0, sizeof(struct treq_phonebook_update_record));

	dbg("arg_type[%d]",arg_type);
	dbg("arg_index[%d]",arg_index);
	dbg("arg_name[%s]",arg_name);
	dbg("arg_dcs[%d]",arg_dcs);
	dbg("arg_number[%s]",arg_number);
	dbg("arg_ton[%d]",arg_ton);

	pb_update.index = (unsigned short)arg_index;
	pb_update.phonebook_type = arg_type;

	if(strlen(arg_name)){
		snprintf((char *)pb_update.name, strlen(arg_name)+1, "%s", arg_name);
		pb_update.dcs = arg_dcs;
	}

	if(strlen(arg_number)){
		snprintf((char *)pb_update.number, strlen(arg_number)+1, "%s", arg_number);
		pb_update.ton = arg_ton;
	}

	if(strlen(arg_number2)){
		snprintf((char *)pb_update.anr1, strlen(arg_number2)+1, "%s", arg_number2);
		pb_update.anr1_ton = arg_number2_ton;
	}

	if(strlen(arg_number3)){
		snprintf((char *)pb_update.anr1, strlen(arg_number3)+1, "%s", arg_number3);
		pb_update.anr1_ton = arg_number3_ton;
	}

	if(strlen(arg_number4)){
		snprintf((char *)pb_update.anr1, strlen(arg_number4)+1, "%s", arg_number4);
		pb_update.anr1_ton = arg_number4_ton;
	}

	pb_update.group_index = (unsigned short)arg_group_index;

	tcore_user_request_set_data(ur, sizeof(struct treq_phonebook_update_record), &pb_update);
	tcore_user_request_set_command(ur, TREQ_PHONEBOOK_UPDATERECORD);
	tcore_communicator_dispatch_request(ctx->comm, ur);

	return TRUE;
}

static gboolean on_phonebook_delete_record(TelephonyPhonebook *phonebook, GDBusMethodInvocation *invocation,
		gint arg_type, gint arg_index, gpointer user_data)
{
	struct custom_data *ctx = user_data;
	UserRequest *ur = NULL;

	struct treq_phonebook_delete_record pb_delete;

	ur = MAKE_UR(ctx, phonebook, invocation);
	memset(&pb_delete, 0, sizeof(struct treq_phonebook_delete_record));

	pb_delete.index = (unsigned short)arg_index;
	pb_delete.phonebook_type = arg_type;

	tcore_user_request_set_data(ur, sizeof(struct treq_phonebook_delete_record), &pb_delete);
	tcore_user_request_set_command(ur, TREQ_PHONEBOOK_DELETERECORD);
	tcore_communicator_dispatch_request(ctx->comm, ur);

	return TRUE;
}

gboolean dbus_plugin_setup_phonebook_interface(TelephonyObjectSkeleton *object, struct custom_data *ctx)
{
	TelephonyPhonebook *phonebook;

	phonebook = telephony_phonebook_skeleton_new();
	telephony_object_skeleton_set_phonebook(object, phonebook);
	g_object_unref(phonebook);

	dbg("phonebook = %p", phonebook);

	g_signal_connect (phonebook,
			"handle-get-init-status",
			G_CALLBACK (on_phonebook_get_init_status),
			ctx);

	g_signal_connect (phonebook,
			"handle-get-count",
			G_CALLBACK (on_phonebook_get_count),
			ctx);

	g_signal_connect (phonebook,
			"handle-get-info",
			G_CALLBACK (on_phonebook_get_info),
			ctx);

	g_signal_connect (phonebook,
			"handle-get-usim-meta-info",
			G_CALLBACK (on_phonebook_get_usim_info),
			ctx);

	g_signal_connect (phonebook,
			"handle-read-record",
			G_CALLBACK (on_phonebook_read_record),
			ctx);

	g_signal_connect (phonebook,
			"handle-update-record",
			G_CALLBACK (on_phonebook_update_record),
			ctx);

	g_signal_connect (phonebook,
			"handle-delete-record",
			G_CALLBACK (on_phonebook_delete_record),
			ctx);

	return TRUE;
}

gboolean dbus_plugin_phonebook_response(struct custom_data *ctx, UserRequest *ur,
		struct dbus_request_info *dbus_info, enum tcore_response_command command,
		unsigned int data_len, const void *data)
{
	const struct tresp_phonebook_get_count *resp_pbcnt = data;
	const struct tresp_phonebook_get_info *resp_entry = data;
	const struct tresp_phonebook_get_usim_info *resp_capa = data;
	const struct tresp_phonebook_read_record *resp_pbread = data;
	const struct tresp_phonebook_update_record *resp_pbupdate = data;
	const struct tresp_phonebook_delete_record *resp_pbdelete = data;

	dbg("application command = [0x%x], data_len = %d", command, data_len);

	switch (command) {
		case TRESP_PHONEBOOK_GETCOUNT:
			dbg("dbus comm - TRESP_PHONEBOOK_GETCOUNT");
			dbg("used[%d]total[%d]", resp_pbcnt->used_count, resp_pbcnt->total_count);
			telephony_phonebook_complete_get_count(dbus_info->interface_object, dbus_info->invocation,
					resp_pbcnt->result, resp_pbcnt->type, resp_pbcnt->used_count, resp_pbcnt->total_count);
			break;

		case TRESP_PHONEBOOK_GETMETAINFO:
			dbg("dbus comm - TRESP_PHONEBOOK_GETMETAINFO");
			dbg("index min[%d]max[%d], num len max[%d] text len max[%d]", resp_entry->index_min, resp_entry->index_max, resp_entry->number_length_max, resp_entry->text_length_max);
			telephony_phonebook_complete_get_info(dbus_info->interface_object, dbus_info->invocation,
					resp_entry->result, resp_entry->type, resp_entry->index_min, resp_entry->index_max,
					resp_entry->number_length_max, resp_entry->text_length_max);
			break;

		case TRESP_PHONEBOOK_GETUSIMINFO:{
			GVariant *gv = NULL;
			GVariantBuilder b;
			int i;
			dbg("resp comm - TRESP_PHONEBOOK_GETUSIMINFO");
			g_variant_builder_init(&b, G_VARIANT_TYPE("aa{sv}"));

			for(i=0;i < resp_capa->field_count; i++){
				g_variant_builder_open(&b,G_VARIANT_TYPE("a{sv}"));
				g_variant_builder_add(&b, "{sv}", "filed_type", g_variant_new_int32(resp_capa->field_list[i].field));
				g_variant_builder_add(&b, "{sv}", "index_max", g_variant_new_int32(resp_capa->field_list[i].index_max));
				g_variant_builder_add(&b, "{sv}", "text_max", g_variant_new_int32(resp_capa->field_list[i].text_max));
				g_variant_builder_add(&b, "{sv}", "used_count", g_variant_new_int32(resp_capa->field_list[i].used_count));
				g_variant_builder_close(&b);
			}
			gv = g_variant_builder_end(&b);

			telephony_phonebook_complete_get_usim_meta_info (dbus_info->interface_object, dbus_info->invocation,
					resp_capa->result,
					gv);
			g_variant_unref(gv);
		}
			break;

		case TRESP_PHONEBOOK_READRECORD:
			dbg("dbus comm - TRESP_PHONEBOOK_READRECORD");
			dbg("resp_pbread->index[%d]",resp_pbread->index );
			dbg("resp_pbread->next_index[%d]",resp_pbread->next_index );
			dbg("resp_pbread->name[%s]",resp_pbread->name );
			dbg("resp_pbread->dcs[%d]",resp_pbread->dcs );
			dbg("resp_pbread->number[%s]",resp_pbread->number );
			dbg("resp_pbread->ton[%d]",resp_pbread->ton );

			telephony_phonebook_complete_read_record(dbus_info->interface_object, dbus_info->invocation,
					resp_pbread->result, resp_pbread->phonebook_type, resp_pbread->index, resp_pbread->next_index, (const gchar *)resp_pbread->name,
					resp_pbread->dcs, (const gchar *)resp_pbread->number, resp_pbread->ton, (const gchar *)resp_pbread->anr1, resp_pbread->anr1_ton,
					(const gchar *)resp_pbread->anr2, resp_pbread->anr2_ton, (const gchar *)resp_pbread->anr3, resp_pbread->anr3_ton,
					(const gchar *)resp_pbread->email1, (const gchar *)resp_pbread->email2, (const gchar *)resp_pbread->email3, (const gchar *)resp_pbread->email4, resp_pbread->group_index);
			break;

		case TRESP_PHONEBOOK_UPDATERECORD:
			dbg("dbus comm - TRESP_PHONEBOOK_UPDATERECORD");
			telephony_phonebook_complete_update_record(dbus_info->interface_object, dbus_info->invocation,resp_pbupdate->result);
			break;

		case TRESP_PHONEBOOK_DELETERECORD:
			dbg("dbus comm - TRESP_PHONEBOOK_DELETERECORD");
			telephony_phonebook_complete_delete_record(dbus_info->interface_object, dbus_info->invocation, resp_pbdelete->result);
			break;

		default:
			dbg("not handled cmd[0x%x]", command);
			break;
	}

	return TRUE;
}

gboolean dbus_plugin_phonebook_notification(struct custom_data *ctx, const char *plugin_name,
		TelephonyObjectSkeleton *object, enum tcore_notification_command command,
		unsigned int data_len, const void *data)
{
	TelephonyPhonebook *phonebook;
	const struct tnoti_phonebook_status *n_pb_status = data;

	if (!object) {
		dbg("object is NULL");
		return FALSE;
	}

	phonebook = telephony_object_peek_phonebook(TELEPHONY_OBJECT(object));
	dbg("phonebook = %p", phonebook);

	switch (command) {
		case TNOTI_PHONEBOOK_STATUS :
			telephony_phonebook_emit_status(phonebook, n_pb_status->b_init);
			break;

		default:
			dbg("not handled cmd[0x%x]", command);
			break;
	}

	return TRUE;
}
