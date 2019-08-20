/*
 * clock-view-settings: Shared object instance holding settings for plugin
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

#ifndef __XFDASHBOARD_CLOCK_VIEW_SETTINGS__
#define __XFDASHBOARD_CLOCK_VIEW_SETTINGS__

#include <libxfdashboard/libxfdashboard.h>
#include <glib-object.h>
#include <clutter/clutter.h>

G_BEGIN_DECLS

#define XFDASHBOARD_TYPE_CLOCK_VIEW_SETTINGS				(xfdashboard_clock_view_settings_get_type())
#define XFDASHBOARD_CLOCK_VIEW_SETTINGS(obj)				(G_TYPE_CHECK_INSTANCE_CAST((obj), XFDASHBOARD_TYPE_CLOCK_VIEW_SETTINGS, XfdashboardClockViewSettings))
#define XFDASHBOARD_IS_CLOCK_VIEW_SETTINGS(obj)				(G_TYPE_CHECK_INSTANCE_TYPE((obj), XFDASHBOARD_TYPE_CLOCK_VIEW_SETTINGS))
#define XFDASHBOARD_CLOCK_VIEW_SETTINGS_CLASS(klass)		(G_TYPE_CHECK_CLASS_CAST((klass), XFDASHBOARD_TYPE_CLOCK_VIEW_SETTINGS, XfdashboardClockViewSettingsClass))
#define XFDASHBOARD_IS_CLOCK_VIEW_SETTINGS_CLASS(klass)		(G_TYPE_CHECK_CLASS_TYPE((klass), XFDASHBOARD_TYPE_CLOCK_VIEW_SETTINGS))
#define XFDASHBOARD_CLOCK_VIEW_SETTINGS_GET_CLASS(obj)		(G_TYPE_INSTANCE_GET_CLASS((obj), XFDASHBOARD_TYPE_CLOCK_VIEW_SETTINGS, XfdashboardClockViewSettingsClass))

typedef struct _XfdashboardClockViewSettings				XfdashboardClockViewSettings; 
typedef struct _XfdashboardClockViewSettingsPrivate			XfdashboardClockViewSettingsPrivate;
typedef struct _XfdashboardClockViewSettingsClass			XfdashboardClockViewSettingsClass;

struct _XfdashboardClockViewSettings
{
	/* Parent instance */
	GObject									parent_instance;

	/* Private structure */
	XfdashboardClockViewSettingsPrivate		*priv;
};

struct _XfdashboardClockViewSettingsClass
{
	/*< private >*/
	/* Parent class */
	GObjectClass							parent_class;
};

/* Public API */
GType xfdashboard_clock_view_settings_get_type(void) G_GNUC_CONST;

XfdashboardClockViewSettings* xfdashboard_clock_view_settings_new(void);

const ClutterColor* xfdashboard_clock_view_settings_get_hour_color(XfdashboardClockViewSettings *self);
void xfdashboard_clock_view_settings_set_hour_color(XfdashboardClockViewSettings *self, const ClutterColor *inColor);

const ClutterColor* xfdashboard_clock_view_settings_get_minute_color(XfdashboardClockViewSettings *self);
void xfdashboard_clock_view_settings_set_minute_color(XfdashboardClockViewSettings *self, const ClutterColor *inColor);

const ClutterColor* xfdashboard_clock_view_settings_get_second_color(XfdashboardClockViewSettings *self);
void xfdashboard_clock_view_settings_set_second_color(XfdashboardClockViewSettings *self, const ClutterColor *inColor);

const ClutterColor* xfdashboard_clock_view_settings_get_background_color(XfdashboardClockViewSettings *self);
void xfdashboard_clock_view_settings_set_background_color(XfdashboardClockViewSettings *self, const ClutterColor *inColor);

XFDASHBOARD_DECLARE_PLUGIN_TYPE(xfdashboard_clock_view_settings);

G_END_DECLS

#endif
