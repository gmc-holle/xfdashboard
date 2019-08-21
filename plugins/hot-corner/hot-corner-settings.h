/*
 * hot-corner-settings: Shared object instance holding settings for plugin
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

#ifndef __XFDASHBOARD_HOT_CORNER_SETTINGS__
#define __XFDASHBOARD_HOT_CORNER_SETTINGS__

#include <libxfdashboard/libxfdashboard.h>

G_BEGIN_DECLS

/* Public definitions */
typedef enum /*< prefix=XFDASHBOARD_HOT_CORNER_SETTINGS_ACTIVATION_CORNER >*/
{
	XFDASHBOARD_HOT_CORNER_SETTINGS_ACTIVATION_CORNER_TOP_LEFT=0,
	XFDASHBOARD_HOT_CORNER_SETTINGS_ACTIVATION_CORNER_TOP_RIGHT,
	XFDASHBOARD_HOT_CORNER_SETTINGS_ACTIVATION_CORNER_BOTTOM_LEFT,
	XFDASHBOARD_HOT_CORNER_SETTINGS_ACTIVATION_CORNER_BOTTOM_RIGHT,
} XfdashboardHotCornerSettingsActivationCorner;

GType xfdashboard_hot_corner_settings_activation_corner_get_type(void) G_GNUC_CONST;
#define XFDASHBOARD_TYPE_HOT_CORNER_SETTINGS_ACTIVATION_CORNER	(xfdashboard_hot_corner_settings_activation_corner_get_type())


/* Object declaration */
#define XFDASHBOARD_TYPE_HOT_CORNER_SETTINGS			(xfdashboard_hot_corner_settings_get_type())
#define XFDASHBOARD_HOT_CORNER_SETTINGS(obj)			(G_TYPE_CHECK_INSTANCE_CAST((obj), XFDASHBOARD_TYPE_HOT_CORNER_SETTINGS, XfdashboardHotCornerSettings))
#define XFDASHBOARD_IS_HOT_CORNER_SETTINGS(obj)			(G_TYPE_CHECK_INSTANCE_TYPE((obj), XFDASHBOARD_TYPE_HOT_CORNER_SETTINGS))
#define XFDASHBOARD_HOT_CORNER_SETTINGS_CLASS(klass)	(G_TYPE_CHECK_CLASS_CAST((klass), XFDASHBOARD_TYPE_HOT_CORNER_SETTINGS, XfdashboardHotCornerSettingsClass))
#define XFDASHBOARD_IS_HOT_CORNER_SETTINGS_CLASS(klass)	(G_TYPE_CHECK_CLASS_TYPE((klass), XFDASHBOARD_TYPE_HOT_CORNER_SETTINGS))
#define XFDASHBOARD_HOT_CORNER_SETTINGS_GET_CLASS(obj)	(G_TYPE_INSTANCE_GET_CLASS((obj), XFDASHBOARD_TYPE_HOT_CORNER_SETTINGS, XfdashboardHotCornerSettingsClass))

typedef struct _XfdashboardHotCornerSettings			XfdashboardHotCornerSettings; 
typedef struct _XfdashboardHotCornerSettingsPrivate		XfdashboardHotCornerSettingsPrivate;
typedef struct _XfdashboardHotCornerSettingsClass		XfdashboardHotCornerSettingsClass;

struct _XfdashboardHotCornerSettings
{
	/* Parent instance */
	GObject								parent_instance;

	/* Private structure */
	XfdashboardHotCornerSettingsPrivate	*priv;
};

struct _XfdashboardHotCornerSettingsClass
{
	/*< private >*/
	/* Parent class */
	GObjectClass						parent_class;
};

/* Public API */
GType xfdashboard_hot_corner_settings_get_type(void) G_GNUC_CONST;

XFDASHBOARD_DECLARE_PLUGIN_TYPE(xfdashboard_hot_corner_settings);

XfdashboardHotCornerSettings* xfdashboard_hot_corner_settings_new(void);

XfdashboardHotCornerSettingsActivationCorner xfdashboard_hot_corner_settings_get_activation_corner(XfdashboardHotCornerSettings *self);
void xfdashboard_hot_corner_settings_set_activation_corner(XfdashboardHotCornerSettings *self, const XfdashboardHotCornerSettingsActivationCorner inCorner);

gint xfdashboard_hot_corner_settings_get_activation_radius(XfdashboardHotCornerSettings *self);
void xfdashboard_hot_corner_settings_set_activation_radius(XfdashboardHotCornerSettings *self, gint inRadius);

gint64 xfdashboard_hot_corner_settings_get_activation_duration(XfdashboardHotCornerSettings *self);
void xfdashboard_hot_corner_settings_set_activation_duration(XfdashboardHotCornerSettings *self, gint64 inDuration);

G_END_DECLS

#endif
