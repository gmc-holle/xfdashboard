/*
 * themes: Theme settings of application
 * 
 * Copyright 2012-2019 Stephan Haller <nomad@froevel.de>
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

#ifndef __XFDASHBOARD_SETTINGS_THEMES__
#define __XFDASHBOARD_SETTINGS_THEMES__

#include <gtk/gtkx.h>

G_BEGIN_DECLS

#define XFDASHBOARD_TYPE_SETTINGS_THEMES				(xfdashboard_settings_themes_get_type())
#define XFDASHBOARD_SETTINGS_THEMES(obj)				(G_TYPE_CHECK_INSTANCE_CAST((obj), XFDASHBOARD_TYPE_SETTINGS_THEMES, XfdashboardSettingsThemes))
#define XFDASHBOARD_IS_SETTINGS_THEMES(obj)				(G_TYPE_CHECK_INSTANCE_TYPE((obj), XFDASHBOARD_TYPE_SETTINGS_THEMES))
#define XFDASHBOARD_SETTINGS_THEMES_CLASS(klass)		(G_TYPE_CHECK_CLASS_CAST((klass), XFDASHBOARD_TYPE_SETTINGS_THEMES, XfdashboardSettingsThemesClass))
#define XFDASHBOARD_IS_SETTINGS_THEMES_CLASS(klass)		(G_TYPE_CHECK_CLASS_TYPE((klass), XFDASHBOARD_TYPE_SETTINGS_THEMES))
#define XFDASHBOARD_SETTINGS_THEMES_GET_CLASS(obj)		(G_TYPE_INSTANCE_GET_CLASS((obj), XFDASHBOARD_TYPE_SETTINGS_THEMES, XfdashboardSettingsThemesClass))

typedef struct _XfdashboardSettingsThemes				XfdashboardSettingsThemes;
typedef struct _XfdashboardSettingsThemesClass			XfdashboardSettingsThemesClass;
typedef struct _XfdashboardSettingsThemesPrivate		XfdashboardSettingsThemesPrivate;

struct _XfdashboardSettingsThemes
{
	/* Parent instance */
	GObject							parent_instance;

	/* Private structure */
	XfdashboardSettingsThemesPrivate	*priv;
};

struct _XfdashboardSettingsThemesClass
{
	/*< private >*/
	/* Parent class */
	GObjectClass					parent_class;

	/*< public >*/
	/* Virtual functions */
};

/* Public API */
GType xfdashboard_settings_themes_get_type(void) G_GNUC_CONST;

XfdashboardSettingsThemes* xfdashboard_settings_themes_new(GtkBuilder *inBuilder);

G_END_DECLS

#endif	/* __XFDASHBOARD_SETTINGS_THEMES__ */
