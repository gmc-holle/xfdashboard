/*
 * settings: Settings of application
 * 
 * Copyright 2012-2020 Stephan Haller <nomad@froevel.de>
 * 
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston,
 * MA 02110-1301, USA.
 * 
 * 
 */

#ifndef __XFDASHBOARD_SETTINGS_APP__
#define __XFDASHBOARD_SETTINGS_APP__

#include <gtk/gtkx.h>
#include <libxfdashboard/settings.h>

G_BEGIN_DECLS

#define XFDASHBOARD_TYPE_SETTINGS_APP				(xfdashboard_settings_app_get_type())
#define XFDASHBOARD_SETTINGS_APP(obj)				(G_TYPE_CHECK_INSTANCE_CAST((obj), XFDASHBOARD_TYPE_SETTINGS_APP, XfdashboardSettingsApp))
#define XFDASHBOARD_IS_SETTINGS_APP(obj)			(G_TYPE_CHECK_INSTANCE_TYPE((obj), XFDASHBOARD_TYPE_SETTINGS_APP))
#define XFDASHBOARD_SETTINGS_APP_CLASS(klass)		(G_TYPE_CHECK_CLASS_CAST((klass), XFDASHBOARD_TYPE_SETTINGS_APP, XfdashboardSettingsAppClass))
#define XFDASHBOARD_IS_SETTINGS_APP_CLASS(klass)	(G_TYPE_CHECK_CLASS_TYPE((klass), XFDASHBOARD_TYPE_SETTINGS_APP))
#define XFDASHBOARD_SETTINGS_APP_GET_CLASS(obj)		(G_TYPE_INSTANCE_GET_CLASS((obj), XFDASHBOARD_TYPE_SETTINGS_APP, XfdashboardSettingsAppClass))

typedef struct _XfdashboardSettingsApp				XfdashboardSettingsApp;
typedef struct _XfdashboardSettingsAppClass			XfdashboardSettingsAppClass;
typedef struct _XfdashboardSettingsAppPrivate		XfdashboardSettingsAppPrivate;

struct _XfdashboardSettingsApp
{
	/* Parent instance */
	GObject							parent_instance;

	/* Private structure */
	XfdashboardSettingsAppPrivate	*priv;
};

struct _XfdashboardSettingsAppClass
{
	/*< private >*/
	/* Parent class */
	GObjectClass					parent_class;

	/*< public >*/
	/* Virtual functions */
};

/* Public API */
GType xfdashboard_settings_app_get_type(void) G_GNUC_CONST;

XfdashboardSettingsApp* xfdashboard_settings_app_new(void);

GtkWidget* xfdashboard_settings_app_create_dialog(XfdashboardSettingsApp *self);
GtkWidget* xfdashboard_settings_app_create_plug(XfdashboardSettingsApp *self, Window inSocketID);

GtkBuilder* xfdashboard_settings_app_get_builder(XfdashboardSettingsApp *self);
XfdashboardSettings* xfdashboard_settings_app_get_settings(XfdashboardSettingsApp *self);

G_END_DECLS

#endif	/* __XFDASHBOARD_SETTINGS_APP__ */
