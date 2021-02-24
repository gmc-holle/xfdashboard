/*
 * plugin-settings: A generic class containing the settings of a plugin
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

#ifndef __LIBXFDASHBOARD_PLUGIN_SETTINGS__
#define __LIBXFDASHBOARD_PLUGIN_SETTINGS__

#if !defined(__LIBXFDASHBOARD_H_INSIDE__) && !defined(LIBXFDASHBOARD_COMPILATION)
#error "Only <libxfdashboard/libxfdashboard.h> can be included directly."
#endif

#include <glib-object.h>

G_BEGIN_DECLS

#define XFDASHBOARD_TYPE_PLUGIN_SETTINGS				(xfdashboard_plugin_settings_get_type())
#define XFDASHBOARD_PLUGIN_SETTINGS(obj)				(G_TYPE_CHECK_INSTANCE_CAST((obj), XFDASHBOARD_TYPE_PLUGIN_SETTINGS, XfdashboardPluginSettings))
#define XFDASHBOARD_IS_PLUGIN_SETTINGS(obj)				(G_TYPE_CHECK_INSTANCE_TYPE((obj), XFDASHBOARD_TYPE_PLUGIN_SETTINGS))
#define XFDASHBOARD_PLUGIN_SETTINGS_CLASS(klass)		(G_TYPE_CHECK_CLASS_CAST((klass), XFDASHBOARD_TYPE_PLUGIN_SETTINGS, XfdashboardPluginSettingsClass))
#define XFDASHBOARD_IS_PLUGIN_SETTINGS_CLASS(klass)		(G_TYPE_CHECK_CLASS_TYPE((klass), XFDASHBOARD_TYPE_PLUGIN_SETTINGS))
#define XFDASHBOARD_PLUGIN_SETTINGS_GET_CLASS(obj)		(G_TYPE_INSTANCE_GET_CLASS((obj), XFDASHBOARD_TYPE_PLUGIN_SETTINGS, XfdashboardPluginSettingsClass))

typedef struct _XfdashboardPluginSettings				XfdashboardPluginSettings;
typedef struct _XfdashboardPluginSettingsClass			XfdashboardPluginSettingsClass;

/**
 * XfdashboardPluginSettings:
 *
 * The #XfdashboardPluginSettings structure contains only private data and
 * should be accessed using the provided API
 */
struct _XfdashboardPluginSettings
{
	/*< private >*/
	/* Parent instance */
	GInitiallyUnowned					parent_instance;
};

/**
 * XfdashboardPluginSettingsClass:
 * @settings_changed: The class closure for changed signal
 *
 * The #XfdashboardPluginSettingsClass class structure
 */
struct _XfdashboardPluginSettingsClass
{
	/*< private >*/
	/* Parent class */
	GInitiallyUnownedClass				parent_class;

	/*< public >*/
	/* Virtual functions */
	void (*changed)(XfdashboardPluginSettings *self, GParamSpec *inParamSpec);
};


/* Public API */
GType xfdashboard_plugin_settings_get_type(void) G_GNUC_CONST;

G_END_DECLS

#endif	/* __LIBXFDASHBOARD_PLUGIN_SETTINGS__ */
