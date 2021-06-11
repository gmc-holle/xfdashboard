/*
 * settings: A generic class containing the settings
 * 
 * Copyright 2012-2021 Stephan Haller <nomad@froevel.de>
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

#ifndef __LIBXFDASHBOARD_SETTINGS__
#define __LIBXFDASHBOARD_SETTINGS__

#if !defined(__LIBXFDASHBOARD_H_INSIDE__) && !defined(LIBXFDASHBOARD_COMPILATION)
#error "Only <libxfdashboard/libxfdashboard.h> can be included directly."
#endif

#include <glib-object.h>

#include <libxfdashboard/applications-search-provider.h>
#include <libxfdashboard/plugin.h>

G_BEGIN_DECLS

#define XFDASHBOARD_TYPE_SETTINGS				(xfdashboard_settings_get_type())
#define XFDASHBOARD_SETTINGS(obj)				(G_TYPE_CHECK_INSTANCE_CAST((obj), XFDASHBOARD_TYPE_SETTINGS, XfdashboardSettings))
#define XFDASHBOARD_IS_SETTINGS(obj)			(G_TYPE_CHECK_INSTANCE_TYPE((obj), XFDASHBOARD_TYPE_SETTINGS))
#define XFDASHBOARD_SETTINGS_CLASS(klass)		(G_TYPE_CHECK_CLASS_CAST((klass), XFDASHBOARD_TYPE_SETTINGS, XfdashboardSettingsClass))
#define XFDASHBOARD_IS_SETTINGS_CLASS(klass)	(G_TYPE_CHECK_CLASS_TYPE((klass), XFDASHBOARD_TYPE_SETTINGS))
#define XFDASHBOARD_SETTINGS_GET_CLASS(obj)		(G_TYPE_INSTANCE_GET_CLASS((obj), XFDASHBOARD_TYPE_SETTINGS, XfdashboardSettingsClass))

typedef struct _XfdashboardSettings				XfdashboardSettings;
typedef struct _XfdashboardSettingsClass		XfdashboardSettingsClass;
typedef struct _XfdashboardSettingsPrivate		XfdashboardSettingsPrivate;

/**
 * XfdashboardSettings:
 *
 * The #XfdashboardSettings structure contains only private data and
 * should be accessed using the provided API
 */
struct _XfdashboardSettings
{
	/*< private >*/
	/* Parent instance */
	GInitiallyUnowned					parent_instance;

	/* Private structure */
	XfdashboardSettingsPrivate			*priv;
};

/**
 * XfdashboardSettingsClass:
 * @settings_changed: The class closure for changed signal
 *
 * The #XfdashboardSettingsClass class structure
 */
struct _XfdashboardSettingsClass
{
	/*< private >*/
	/* Parent class */
	GInitiallyUnownedClass				parent_class;

	/*< public >*/
	/* Virtual functions */
	void (*changed)(XfdashboardSettings *self, const gchar *inPluginID, GParamSpec *inParamSpec);

	void (*plugin_added)(XfdashboardSettings *self, XfdashboardPlugin *inPlugin);
	void (*plugin_removed)(XfdashboardSettings *self, XfdashboardPlugin *inPlugin);
};


/* Public API */
GType xfdashboard_settings_get_type(void) G_GNUC_CONST;

void xfdashboard_settings_add_plugin(XfdashboardSettings *self, XfdashboardPlugin *inPlugin);
void xfdashboard_settings_remove_plugin(XfdashboardSettings *self, XfdashboardPlugin *inPlugin);
XfdashboardPlugin* xfdashboard_settings_lookup_plugin_by_id(XfdashboardSettings *self, const gchar *inPluginID);

const gchar* xfdashboard_settings_get_theme(XfdashboardSettings *self);
void xfdashboard_settings_set_theme(XfdashboardSettings *self, const gchar *inTheme);

gboolean xfdashboard_settings_get_allow_subwindows(XfdashboardSettings *self);
void xfdashboard_settings_set_allow_subwindows(XfdashboardSettings *self, gboolean inAllowSubwindows);

const gchar** xfdashboard_settings_get_enabled_plugins(XfdashboardSettings *self);
void xfdashboard_settings_set_enabled_plugins(XfdashboardSettings *self, const gchar **inEnabledPlugins);

const gchar** xfdashboard_settings_get_favourites(XfdashboardSettings *self);
void xfdashboard_settings_set_favourites(XfdashboardSettings *self, const gchar **inFavourites);

gboolean xfdashboard_settings_get_always_launch_new_instance(XfdashboardSettings *self);
void xfdashboard_settings_set_always_launch_new_instance(XfdashboardSettings *self, gboolean inAlwaysLaunchNewInstance);

gboolean xfdashboard_settings_get_enable_workaround_unmapped_window(XfdashboardSettings *self);
void xfdashboard_settings_set_enable_workaround_unmapped_window(XfdashboardSettings *self, gboolean inEnableWorkaroundUnmappedWindow);

const gchar* xfdashboard_settings_get_window_content_creation_priority(XfdashboardSettings *self);
void xfdashboard_settings_set_window_content_creation_priority(XfdashboardSettings *self, const gchar *inWindowContentCreationPriority);

gboolean xfdashboard_settings_get_enable_animations(XfdashboardSettings *self);
void xfdashboard_settings_set_enable_animations(XfdashboardSettings *self, gboolean inEnableAnimations);

guint xfdashboard_settings_get_notification_timeout(XfdashboardSettings *self);
void xfdashboard_settings_set_notification_timeout(XfdashboardSettings *self, guint inNotificationTimeout);

gboolean xfdashboard_settings_get_reset_search_on_resume(XfdashboardSettings *self);
void xfdashboard_settings_set_reset_search_on_resume(XfdashboardSettings *self, gboolean inResetSearchOnResume);

const gchar* xfdashboard_settings_get_switch_to_view_on_resume(XfdashboardSettings *self);
void xfdashboard_settings_set_switch_to_view_on_resume(XfdashboardSettings *self, const gchar *inSwitchToViewOnResume);

gboolean xfdashboard_settings_get_reselect_theme_focus_on_resume(XfdashboardSettings *self);
void xfdashboard_settings_set_reselect_theme_focus_on_resume(XfdashboardSettings *self, gboolean inReselectThemeFocusOnResume);

XfdashboardApplicationsSearchProviderSortMode xfdashboard_settings_get_applications_search_sort_mode(XfdashboardSettings *self);
void xfdashboard_settings_set_applications_search_sort_mode(XfdashboardSettings *self, XfdashboardApplicationsSearchProviderSortMode inApplicationsSearchProviderSortMode);

gboolean xfdashboard_settings_get_show_all_applications(XfdashboardSettings *self);
void xfdashboard_settings_set_show_all_applications(XfdashboardSettings *self, gboolean inApplicationsViewShowAllApps);

guint xfdashboard_settings_get_delay_search_timeout(XfdashboardSettings *self);
void xfdashboard_settings_set_delay_search_timeout(XfdashboardSettings *self, guint inSearchViewDelaySearchTimeout);

gboolean xfdashboard_settings_get_scroll_event_changes_workspace(XfdashboardSettings *self);
void xfdashboard_settings_set_scroll_event_changes_workspace(XfdashboardSettings *self, gboolean inWindowsViewScrollEventChangesWorkspace);

const gchar** xfdashboard_settings_get_binding_files(XfdashboardSettings *self);

const gchar** xfdashboard_settings_get_theme_search_paths(XfdashboardSettings *self);

const gchar** xfdashboard_settings_get_plugin_search_paths(XfdashboardSettings *self);

const gchar* xfdashboard_settings_get_config_path(XfdashboardSettings *self);

const gchar* xfdashboard_settings_get_data_path(XfdashboardSettings *self);

G_END_DECLS

#endif	/* __LIBXFDASHBOARD_SETTINGS__ */
