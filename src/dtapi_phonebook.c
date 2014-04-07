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

#include "dtapi_phonebook.h"
#include "dtapi_util.h"

#include <co_phonebook.h>
#include <plugin.h>

#define AC_PHONEBOOK	"telephony_framework::api_phonebook"

static gboolean __check_phonebook_status(TcorePlugin *plugin)
{
	CoreObject *co_pb;
	gboolean init_status = FALSE;

	co_pb = tcore_plugin_ref_core_object(plugin, CORE_OBJECT_TYPE_PHONEBOOK);
	tcore_check_return_value(co_pb != NULL, FALSE);

	if (tcore_phonebook_get_status(co_pb, &init_status) == FALSE) {
		err("Get Status Failed");
		return FALSE;
	}
	tcore_check_return_value(init_status == TRUE, FALSE);

	return TRUE;
}

static gboolean dtapi_phonebook_get_init_info(TelephonyPhonebook *phonebook,
	GDBusMethodInvocation *invocation, gpointer user_data)
{
	TcorePlugin *plugin = user_data;
	gboolean pb_status = FALSE;
	CoreObject *co_pb = NULL;
	TelPbList *list = NULL;

	dbg("Entry");

	if (dtapi_check_access_control(invocation, AC_PHONEBOOK, "r") == FALSE)
		return TRUE;

	co_pb = tcore_plugin_ref_core_object(plugin, CORE_OBJECT_TYPE_PHONEBOOK);
	tcore_check_return_value(co_pb != NULL, TRUE);

	if (tcore_phonebook_get_support_list(co_pb, &list) == FALSE) {
		dtapi_return_error(invocation, G_DBUS_ERROR_FAILED, "Get Support List Failed");
		return TRUE;
	}
	tcore_check_return_value(list != NULL, TRUE);

	dbg("FDN: [%s], ADN: [%s], SDN: [%s], USIM: [%s]",
		list->fdn ? "TRUE" : "FALSE", list->adn ? "TRUE" : "FALSE",
		list->sdn ? "TRUE" : "FALSE", list->usim ? "TRUE" : "FALSE");

	if (tcore_phonebook_get_status(co_pb, &pb_status) == FALSE) {
		dtapi_return_error(invocation, G_DBUS_ERROR_FAILED, "Get Status Failed");
		tcore_free(list);
		return TRUE;
	}

	telephony_phonebook_complete_get_init_info(phonebook, invocation,
		TEL_RETURN_SUCCESS, pb_status,
		list->fdn, list->adn, list->sdn, list->usim);
	tcore_free(list);

	return TRUE;
}

static void on_response_dtapi_phonebook_get_info(gint result,
		const void *response, void *user_data)
{
	DbusRespCbData *rsp_cb_data = user_data;
	const TelPbInfo *pb_info = response;
	GVariant *var_info = NULL;
	GVariantBuilder builder;
	tcore_check_return_assert(rsp_cb_data != NULL);

	dbg("Result: [%d]", result);

	g_variant_builder_init(&builder, G_VARIANT_TYPE("a{sv}"));
	if (TEL_PB_RESULT_SUCCESS != result || pb_info == NULL) {
		err("[DBUS]Get Info Response Failed");
		var_info = g_variant_builder_end(&builder);
		telephony_phonebook_complete_get_info(rsp_cb_data->interface_object,
			rsp_cb_data->invocation, result, -1, var_info);
		g_variant_unref(var_info);

		tcore_free(rsp_cb_data);
		return;
	}

	if (TEL_PB_USIM == pb_info->pb_type) {
		TelPbUsimInfo *usim = (TelPbUsimInfo *)(&pb_info->info_u.usim);

		g_variant_builder_add(&builder, "{sv}",
			"max_count", g_variant_new_uint32(usim->max_count));
		g_variant_builder_add(&builder, "{sv}",
			"used_count", g_variant_new_uint32(usim->used_count));
		g_variant_builder_add(&builder, "{sv}",
			"max_num_len", g_variant_new_uint32(usim->max_num_len));
		g_variant_builder_add(&builder, "{sv}",
			"max_text_len", g_variant_new_uint32(usim->max_text_len));
		g_variant_builder_add(&builder, "{sv}",
			"max_anr_count", g_variant_new_byte(usim->max_anr_count));
		g_variant_builder_add(&builder, "{sv}",
			"max_anr_len", g_variant_new_uint32(usim->max_anr_len));
		g_variant_builder_add(&builder, "{sv}",
			"max_email_count", g_variant_new_byte(usim->max_email_count));
		g_variant_builder_add(&builder, "{sv}",
			"max_email_len", g_variant_new_uint32(usim->max_email_len));
		g_variant_builder_add(&builder, "{sv}",
			"max_sne_len", g_variant_new_uint32(usim->max_sne_len));
		g_variant_builder_add(&builder, "{sv}",
			"max_gas_count", g_variant_new_uint32(usim->max_gas_count));
		g_variant_builder_add(&builder, "{sv}",
			"max_gas_len", g_variant_new_uint32(usim->max_gas_len));
		g_variant_builder_add(&builder, "{sv}",
			"max_aas_count", g_variant_new_uint32(usim->max_aas_count));
		g_variant_builder_add(&builder, "{sv}",
			"max_aas_len", g_variant_new_uint32(usim->max_aas_len));

		dbg("pb_type: [%d] \n\tmax_count: [%d] used_count: [%d] " \
			"max_num_len: [%d] max_text_len: [%d] \n\tmax_anr_count: [%d] " \
			"max_anr_len: [%d] max_email_count: [%d] max_email_len: [%d] " \
			"max_sne_len: [%d] \n\tmax_gas_count: [%d] max_gas_len: [%d] " \
			"max_aas_count: [%d] max_aas_len: [%d]",
			pb_info->pb_type,
			usim->max_count, usim->used_count,
			usim->max_num_len, usim->max_text_len,
			usim->max_anr_count, usim->max_anr_len,
			usim->max_email_count, usim->max_email_len,
			usim->max_sne_len,
			usim->max_gas_count, usim->max_gas_len,
			usim->max_aas_count, usim->max_aas_len);
	}
	else {
		TelPbSimInfo *sim = (TelPbSimInfo *)(&pb_info->info_u.sim);

		g_variant_builder_add(&builder, "{sv}",
			"max_count", g_variant_new_uint32(sim->max_count));
		g_variant_builder_add(&builder, "{sv}",
			"used_count", g_variant_new_uint32(sim->used_count));
		g_variant_builder_add(&builder, "{sv}",
			"max_num_len", g_variant_new_uint32(sim->max_num_len));
		g_variant_builder_add(&builder, "{sv}",
			"max_text_len", g_variant_new_uint32(sim->max_text_len));

		dbg("pb_type: [%d] max_count: [%d] used_count: [%d] " \
			"max_num_len: [%d] max_text_len: [%d]",
			pb_info->pb_type,
			sim->max_count, sim->used_count,
			sim->max_num_len, sim->max_text_len);
	}
	var_info = g_variant_builder_end(&builder);

	telephony_phonebook_complete_get_info(rsp_cb_data->interface_object, rsp_cb_data->invocation,
			result, pb_info->pb_type, var_info);

	tcore_free(rsp_cb_data);
	g_variant_unref(var_info);
}

static gboolean dtapi_phonebook_get_info(TelephonyPhonebook *phonebook,
	GDBusMethodInvocation *invocation,
	gint req_type, gpointer user_data)
{
	DbusRespCbData *rsp_cb_data = NULL;
	TcorePlugin *plugin = user_data;
	TelReturn ret;

	dbg("Entry");

	if (dtapi_check_access_control(invocation, AC_PHONEBOOK, "r") == FALSE)
		return TRUE;

	if (__check_phonebook_status(plugin) == FALSE) {
		dtapi_return_error(invocation, G_DBUS_ERROR_FAILED, "Invalid Phonebook Status");
		return TRUE;
	}

	rsp_cb_data = dtapi_create_resp_cb_data(phonebook, invocation, NULL, 0);

	ret = tcore_plugin_dispatch_request(plugin, TRUE,
		TCORE_COMMAND_PHONEBOOK_GET_INFO,
		&req_type, sizeof(gint),
		on_response_dtapi_phonebook_get_info, rsp_cb_data);
	if (ret != TEL_RETURN_SUCCESS) {
		dtapi_return_error(invocation, G_DBUS_ERROR_FAILED, "Dispatch Failed");
		tcore_free(rsp_cb_data);
	}

	return TRUE;
}

static void on_response_dtapi_phonebook_read_record(gint result,
	const void *response, void *user_data)
{
	DbusRespCbData *rsp_cb_data = user_data;
	const TelPbReadRecord *read_record = response;

	GVariantBuilder read_builder;
	GVariant *var_read = NULL;

	dbg("Result: [%d]", result);

	tcore_check_return_assert(NULL != rsp_cb_data);

	g_variant_builder_init(&read_builder, G_VARIANT_TYPE("a{sv}"));
	if (TEL_PB_RESULT_SUCCESS == result
			&& read_record != NULL) {
		if (read_record->pb_type == TEL_PB_USIM) {
			GVariant *var_anr = NULL, *var_email = NULL;
			GVariantBuilder anr_builder, email_builder;

			TelPbUsimRecord *usim = (TelPbUsimRecord *)(&read_record->rec_u.usim);
			guint count = 0;

			g_variant_builder_add(&read_builder, "{sv}",
				"name", g_variant_new_string(usim->name));
			g_variant_builder_add(&read_builder, "{sv}",
				"number", g_variant_new_string(usim->number));
			g_variant_builder_add(&read_builder, "{sv}",
				"sne", g_variant_new_string(usim->sne));
			g_variant_builder_add(&read_builder, "{sv}",
				"grp_name", g_variant_new_string(usim->grp_name));

			dbg("Phonebook type: [%d] Name: [%s] Number: [%s] "\
				"SNE: [%s] Group Name: [%s]",
				read_record->pb_type, usim->name,
				usim->number, usim->sne, usim->grp_name);

			/* ANR */
			g_variant_builder_add(&read_builder, "{sv}",
				"anr_count",  g_variant_new_byte(usim->anr_count));
			dbg("ANR Count: [%d]", usim->anr_count);

			g_variant_builder_init(&anr_builder, G_VARIANT_TYPE("aa{sv}"));
			for (count = 0; count < usim->anr_count; count++) {
				g_variant_builder_open(&anr_builder, G_VARIANT_TYPE("a{sv}"));

				g_variant_builder_add(&anr_builder, "{sv}",
					"number",  g_variant_new_string(usim->anr[count].number));
				g_variant_builder_add(&anr_builder, "{sv}",
					"description", g_variant_new_boolean(usim->anr[count].description));
				g_variant_builder_add(&anr_builder, "{sv}",
					"aas", g_variant_new_string(usim->anr[count].aas));

				g_variant_builder_close(&anr_builder);

				dbg("ANR[%d] - Number: [%s] Description: [%s] AAS: [%s]", count,
					usim->anr[count].number,
					usim->anr[count].description ? "YES" : "NO",
					usim->anr[count].aas);
			}
			var_anr = g_variant_builder_end(&anr_builder);
			g_variant_builder_add(&read_builder, "{sv}",
				"anr", var_anr);

			/* e-mail */
			g_variant_builder_add(&read_builder, "{sv}",
				"email_count", g_variant_new_byte(usim->email_count));
			dbg("e-mail Count: [%d]", usim->email_count);
			g_variant_builder_init(&email_builder, G_VARIANT_TYPE("a{sv}"));
			if (usim->email_count && usim->email_count <= TEL_PB_EMAIL_MAX_COUNT) {
				for (count = 0; count < usim->email_count; count++) {
					char *tmp = g_strdup_printf("%d", count);

					dbg("e-mail[%s] - [%s]", tmp, usim->email[count]);
					g_variant_builder_add(&email_builder, "{sv}",
						tmp,
						g_variant_new_from_data(G_VARIANT_TYPE("ay"),
							usim->email[count], strlen(usim->email[count]),
							TRUE, NULL, NULL));
					g_free(tmp);
				}
			}
			var_email = g_variant_builder_end(&email_builder);
			g_variant_builder_add(&read_builder, "{sv}",
				"email", var_email);

			g_variant_builder_add(&read_builder, "{sv}",
				"hidden", g_variant_new_boolean(usim->hidden));
			dbg("Hidden: [%s]", usim->hidden ? "YES" : "NO");
		}
		else {
			TelPbSimRecord *sim = (TelPbSimRecord *)&(read_record->rec_u.sim);

			g_variant_builder_add(&read_builder, "{sv}",
				"name", g_variant_new_string(sim->name));
			g_variant_builder_add(&read_builder, "{sv}",
				"number", g_variant_new_string(sim->number));

			dbg("Phonebook type: [%d] Name: [%s] Number: [%s]",
				read_record->pb_type, sim->name, sim->number);
		}
	}
	var_read = g_variant_builder_end(&read_builder);

	telephony_phonebook_complete_read_record(rsp_cb_data->interface_object,
		rsp_cb_data->invocation, result,
		read_record->index, read_record->next_index,
		read_record->pb_type, var_read);

	tcore_free(rsp_cb_data);
}

static gboolean dtapi_phonebook_read_record(TelephonyPhonebook *phonebook,
	GDBusMethodInvocation *invocation,
	gint req_type, gint index, gpointer user_data)
{
	DbusRespCbData *rsp_cb_data = NULL;
	TcorePlugin *plugin = user_data;
	TelReturn ret;
	TelPbRecordInfo pb_record;

	dbg("Entry");

	if (dtapi_check_access_control(invocation, AC_PHONEBOOK, "r") == FALSE)
		return TRUE;

	if (__check_phonebook_status(plugin) == FALSE) {
		dtapi_return_error(invocation, G_DBUS_ERROR_FAILED, "Invalid Phonebook Status");
		return TRUE;
	}

	pb_record.pb_type = req_type;
	pb_record.index = index;

	dbg("Phonebook Type: [%d] Index: [%d]", pb_record.pb_type, pb_record.index);

	rsp_cb_data = dtapi_create_resp_cb_data(phonebook, invocation, NULL, 0);

	ret = tcore_plugin_dispatch_request(plugin, TRUE,
		TCORE_COMMAND_PHONEBOOK_READ_RECORD,
		&pb_record, sizeof(TelPbRecordInfo),
		on_response_dtapi_phonebook_read_record, rsp_cb_data);
	if (ret != TEL_RETURN_SUCCESS) {
		dtapi_return_error(invocation, G_DBUS_ERROR_FAILED, "Dispatch Failed");
		tcore_free(rsp_cb_data);
	}

	return TRUE;
}

static void on_response_dtapi_phonebook_update_record(gint result,
	const void *response, void *user_data)
{
	DbusRespCbData *rsp_cb_data = user_data;

	dbg("Result: [%d]", result);

	tcore_check_return_assert(rsp_cb_data != NULL);

	telephony_phonebook_complete_update_record(rsp_cb_data->interface_object,
		rsp_cb_data->invocation, result);

	tcore_free(rsp_cb_data);
}

static gboolean dtapi_phonebook_update_record(TelephonyPhonebook *phonebook,
	GDBusMethodInvocation *invocation,
	gint pb_type, guint index, GVariant *update_rec,
	gpointer user_data)
{
	DbusRespCbData *rsp_cb_data = NULL;
	TcorePlugin *plugin = user_data;

	TelPbUpdateRecord pb_update;
	GVariantIter *iter;

	TelReturn ret;

	dbg("Entry");

	if (dtapi_check_access_control(invocation, AC_PHONEBOOK, "w") == FALSE)
		return TRUE;

	if (__check_phonebook_status(plugin) == FALSE) {
		dtapi_return_error(invocation, G_DBUS_ERROR_FAILED, "Invalid Phonebook Status");
		return TRUE;
	}

	memset(&pb_update, 0, sizeof(TelPbUpdateRecord));

	pb_update.index = index;
	pb_update.pb_type = pb_type;

	g_variant_get(update_rec, "a{sv}", &iter);
	if (pb_update.pb_type == TEL_PB_USIM) {
		GVariant *key_value;
		const gchar *key;

		TelPbUsimRecord *usim = (TelPbUsimRecord *)&(pb_update.rec_u.usim);

		while (g_variant_iter_loop(iter, "{sv}", &key, &key_value)) {
			if (g_strcmp0(key, "name") == 0) {
				g_strlcpy(usim->name,
					g_variant_get_string(key_value, NULL),
					TEL_PB_TEXT_MAX_LEN + 1);
			}
			else if (g_strcmp0(key, "number") == 0) {
				g_strlcpy(usim->number,
					g_variant_get_string(key_value, NULL),
					TEL_PB_NUMBER_MAX_LEN + 1);
			}
			else if (g_strcmp0(key, "sne") == 0) {
				g_strlcpy(usim->sne,
					g_variant_get_string(key_value, NULL),
					TEL_PB_TEXT_MAX_LEN + 1);
			}
			else if (g_strcmp0(key, "grp_name") == 0) {
				g_strlcpy(usim->grp_name,
					g_variant_get_string(key_value, NULL),
					TEL_PB_TEXT_MAX_LEN + 1);
			}
			else if (g_strcmp0(key, "anr_count") == 0) {
				usim->anr_count = g_variant_get_byte(key_value);
			}
			else if (g_strcmp0(key, "anr") == 0) {
				GVariantIter *iter2 = NULL, *iter_row2= NULL;
				GVariant *key_value2;
				const gchar *key2;
				guint count = 0;

				g_variant_get(key_value, "aa{sv}", &iter2);
				while (g_variant_iter_next(iter2, "a{sv}", &iter_row2)) {
					while (g_variant_iter_loop(iter_row2, "{sv}", &key2, &key_value2)) {
						if (g_strcmp0(key2, "number") == 0) {
							g_strlcpy(usim->anr[count].number,
								g_variant_get_string(key_value2, NULL),
								TEL_PB_NUMBER_MAX_LEN + 1);
						}
						else if (g_strcmp0(key2, "description") == 0) {
							usim->anr[count].description =
								g_variant_get_boolean(key_value2);
						}
						else if (g_strcmp0(key2, "aas") == 0) {
							g_strlcpy(usim->anr[count].aas,
								g_variant_get_string(key_value2, NULL),
								TEL_PB_TEXT_MAX_LEN + 1);
						}
					}
					g_variant_iter_free(iter_row2);
				}
				g_variant_iter_free(iter2);
			}
			else if (g_strcmp0(key, "email_count") == 0) {
				usim->email_count = g_variant_get_byte(key_value);
			}
			else if (g_strcmp0(key, "email") == 0) {
				GVariantIter *iter2 = NULL;
				GVariant *key_value2;
				const gchar *key2;
				guint count = 0;
				char *tmp;

				g_variant_get(key_value, "a{sv}", &iter2);
				while (g_variant_iter_loop(iter2, "{sv}", &key2, &key_value2)) {
					gconstpointer email;

					for (count = 0; count < usim->email_count; count++) {
						tmp = g_strdup_printf("%d", count);
						if (g_strcmp0(key2, tmp) == 0) {
							email = g_variant_get_data(key_value2);
							g_strlcpy(usim->email[count], email, strlen(email)+1);
							dbg("email[%s] - [%s]", tmp, email);
						}
						g_free(tmp);
					}
				}
				g_variant_iter_free(iter2);
			}
			else if (g_strcmp0(key, "hidden") == 0) {
				usim->hidden = g_variant_get_boolean(key_value);
			}
		}
	}
	else {
		GVariant *key_value;
		const gchar *key;

		TelPbSimRecord *sim = (TelPbSimRecord *)&(pb_update.rec_u.sim);

		while (g_variant_iter_loop(iter, "{sv}", &key, &key_value)) {
			if (g_strcmp0(key, "name") == 0) {
				g_strlcpy(sim->name,
					g_variant_get_string(key_value, NULL),
					TEL_PB_TEXT_MAX_LEN + 1);
			}
			else if (g_strcmp0(key, "number") == 0) {
				g_strlcpy(sim->number,
					g_variant_get_string(key_value, NULL),
					TEL_PB_NUMBER_MAX_LEN + 1);
			}
		}
	}
	g_variant_iter_free(iter);
	g_variant_unref(update_rec);

	rsp_cb_data = dtapi_create_resp_cb_data(phonebook, invocation, NULL, 0);

	ret = tcore_plugin_dispatch_request(plugin, TRUE,
		TCORE_COMMAND_PHONEBOOK_UPDATE_RECORD,
		&pb_update, sizeof(TelPbUpdateRecord),
		on_response_dtapi_phonebook_update_record, rsp_cb_data);
	if (TEL_RETURN_SUCCESS != ret) {
		dtapi_return_error(invocation, G_DBUS_ERROR_FAILED, "Dispatch Failed");
		tcore_free(rsp_cb_data);
	}

	return TRUE;
}

static void on_response_dtapi_phonebook_delete_record(gint result,
	const void *response, void *user_data)
{
	DbusRespCbData *rsp_cb_data = user_data;

	dbg("Result: [%d]", result);

	tcore_check_return_assert(rsp_cb_data != NULL);

	telephony_phonebook_complete_delete_record(rsp_cb_data->interface_object,
		rsp_cb_data->invocation, result);

	tcore_free(rsp_cb_data);
}

static gboolean dtapi_phonebook_delete_record(TelephonyPhonebook *phonebook,
	GDBusMethodInvocation *invocation,
	gint pb_type, guint index, gpointer user_data)
{
	DbusRespCbData *rsp_cb_data = NULL;
	TcorePlugin *plugin = user_data;
	TelReturn ret;
	TelPbRecordInfo pb_record;

	dbg("Entry");

	if (dtapi_check_access_control(invocation, AC_PHONEBOOK, "w") == FALSE)
		return TRUE;

	if (__check_phonebook_status(plugin) == FALSE) {
		dtapi_return_error(invocation, G_DBUS_ERROR_FAILED, "Invalid Phonebook Status");
		return TRUE;
	}

	pb_record.pb_type = pb_type;
	pb_record.index = index;

	dbg("Phonebook_type: [%d], index: [%d]", pb_record.pb_type, pb_record.index);

	rsp_cb_data = dtapi_create_resp_cb_data(phonebook, invocation, NULL, 0);

	ret = tcore_plugin_dispatch_request(plugin, TRUE,
		TCORE_COMMAND_PHONEBOOK_DELETE_RECORD,
		&pb_record, sizeof(TelPbRecordInfo),
		on_response_dtapi_phonebook_delete_record, rsp_cb_data);
	if (TEL_RETURN_SUCCESS != ret) {
		dtapi_return_error(invocation, G_DBUS_ERROR_FAILED, "Dispatch Failed");
		tcore_free(rsp_cb_data);
	}

	return TRUE;
}

gboolean dtapi_setup_phonebook_interface(TelephonyObjectSkeleton *object,
	TcorePlugin *plugin)
{
	TelephonyPhonebook *phonebook = telephony_phonebook_skeleton_new();
	tcore_check_return_value_assert(phonebook != NULL, FALSE);

	telephony_object_skeleton_set_phonebook(object, phonebook);
	g_object_unref(phonebook);

	dbg("phonebook = %p", phonebook);

	g_signal_connect(phonebook,
		"handle-get-init-info",
		G_CALLBACK(dtapi_phonebook_get_init_info),
		plugin);

	g_signal_connect(phonebook,
		"handle-get-info",
		G_CALLBACK(dtapi_phonebook_get_info),
		plugin);

	g_signal_connect(phonebook,
		"handle-read-record",
		G_CALLBACK(dtapi_phonebook_read_record),
		plugin);

	g_signal_connect(phonebook,
		"handle-update-record",
		G_CALLBACK(dtapi_phonebook_update_record),
		plugin);

	g_signal_connect(phonebook,
		"handle-delete-record",
		G_CALLBACK(dtapi_phonebook_delete_record),
		plugin);

	return TRUE;
}

gboolean dtapi_handle_phonebook_notification(TelephonyObjectSkeleton *object,
	TcorePlugin *plugin, TcoreNotification command,
	guint data_len, const void *data)
{
	TelephonyPhonebook *phonebook;

	tcore_check_return_value_assert(object != NULL, FALSE);
	tcore_check_return_value_assert(plugin != NULL, FALSE);
	tcore_check_return_value_assert(data != NULL, FALSE);

	phonebook = telephony_object_peek_phonebook(TELEPHONY_OBJECT(object));

	tcore_check_return_value_assert(phonebook != NULL, FALSE);

	switch (command) {
	case TCORE_NOTIFICATION_PHONEBOOK_STATUS: {
		const TelPbInitInfo *init_info = data;
		dbg("Phonebook: [%p], init_status: [%d], "
			"FDN: [%d], ADN: [%d], SDN: [%d], USIM: [%d]",
			phonebook, init_info->init_status,
			init_info->pb_list.fdn, init_info->pb_list.adn,
			init_info->pb_list.sdn, init_info->pb_list.usim);

		telephony_phonebook_emit_status(phonebook, init_info->init_status,
			init_info->pb_list.fdn, init_info->pb_list.adn,
			init_info->pb_list.sdn, init_info->pb_list.usim);
	}
	break;

	default:
		err("not handled cmd[0x%x]", command);
	}

	return TRUE;
}
