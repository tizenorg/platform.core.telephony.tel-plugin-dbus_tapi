#ifndef SAT_UI_SUPPORT_H_
#define SAT_UI_SUPPORT_H_

#include <glib.h>

#include <tcore.h>
#include <type/sat.h>

gboolean sat_ui_support_terminate_sat_ui(void);
gboolean sat_ui_support_launch_call_application(enum tel_sat_proactive_cmd_type cmd_type, GVariant *data);
gboolean sat_ui_support_launch_browser_application(enum tel_sat_proactive_cmd_type cmd_type, GVariant *data);
gboolean sat_ui_support_launch_setting_application(enum tel_sat_proactive_cmd_type cmd_type, GVariant *data);
gboolean sat_ui_support_launch_sat_ui(enum tel_sat_proactive_cmd_type cmd_type, GVariant *data);
gboolean sat_ui_support_create_desktop_file(const gchar *title);
gboolean sat_ui_support_remove_desktop_file(void);

#endif /* SAT_UI_SUPPORT_H_ */
