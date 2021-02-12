/*
 * xfconf-settings: A class storing settings in Xfconf permanently
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

#ifndef __XFDASHBOARD_XFCONF_XFCONF_SETTINGS__
#define __XFDASHBOARD_XFCONF_XFCONF_SETTINGS__

#if !defined(__LIBXFDASHBOARD_H_INSIDE__) && !defined(LIBXFDASHBOARD_COMPILATION)
#error "Only <libxfdashboard/libxfdashboard.h> can be included directly."
#endif

#include <glib-object.h>

#include <libxfdashboard/settings.h>

G_BEGIN_DECLS

#define XFDASHBOARD_TYPE_XFCONF_SETTINGS				(xfdashboard_xfconf_settings_get_type())
#define XFDASHBOARD_XFCONF_SETTINGS(obj)				(G_TYPE_CHECK_INSTANCE_CAST((obj), XFDASHBOARD_TYPE_XFCONF_SETTINGS, XfdashboardXfconfSettings))
#define XFDASHBOARD_IS_XFCONF_SETTINGS(obj)				(G_TYPE_CHECK_INSTANCE_TYPE((obj), XFDASHBOARD_TYPE_XFCONF_SETTINGS))
#define XFDASHBOARD_XFCONF_SETTINGS_CLASS(klass)		(G_TYPE_CHECK_CLASS_CAST((klass), XFDASHBOARD_TYPE_XFCONF_SETTINGS, XfdashboardXfconfSettingsClass))
#define XFDASHBOARD_IS_XFCONF_SETTINGS_CLASS(klass)		(G_TYPE_CHECK_CLASS_TYPE((klass), XFDASHBOARD_TYPE_XFCONF_SETTINGS))
#define XFDASHBOARD_XFCONF_SETTINGS_GET_CLASS(obj)		(G_TYPE_INSTANCE_GET_CLASS((obj), XFDASHBOARD_TYPE_XFCONF_SETTINGS, XfdashboardXfconfSettingsClass))

typedef struct _XfdashboardXfconfSettings				XfdashboardXfconfSettings;
typedef struct _XfdashboardXfconfSettingsClass			XfdashboardXfconfSettingsClass;
typedef struct _XfdashboardXfconfSettingsPrivate		XfdashboardXfconfSettingsPrivate;

/**
 * XfdashboardXfconfSettings:
 *
 * The #XfdashboardXfconfSettings structure contains only private data and
 * should be accessed using the provided API
 */
struct _XfdashboardXfconfSettings
{
	/*< private >*/
	/* Parent instance */
	XfdashboardSettings							parent_instance;

	/* Private structure */
	XfdashboardXfconfSettingsPrivate			*priv;
};

/**
 * XfdashboardXfconfSettingsClass:
 *
 * The #XfdashboardXfconfSettingsClass class structure contains only private data
 */
struct _XfdashboardXfconfSettingsClass
{
	/*< private >*/
	/* Parent class */
	XfdashboardSettingsClass					parent_class;

	/*< public >*/
	/* Virtual functions */
};

/* Public API */
GType xfdashboard_xfconf_settings_get_type(void) G_GNUC_CONST;

G_END_DECLS

#endif	/* __LIBXFDASHBOARD_XFCONF_SETTINGS__ */
