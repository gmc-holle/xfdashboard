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
 
/**
 * SECTION:settings
 * @short_description: The settings class
 * @include: xfdashboard/settings.h
 *
 * This class #XfdashboardSettings contains all settings for this
 * library.
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <libxfdashboard/settings.h>

#include <glib/gi18n-lib.h>

#include <libxfdashboard/applications-search-provider.h>
#include <libxfdashboard/utils.h>
#include <libxfdashboard/marshal.h>
#include <libxfdashboard/enums.h>
#include <libxfdashboard/compat.h>
#include <libxfdashboard/debug.h>


/* Define this class in GObject system */
struct _XfdashboardSettingsPrivate
{
	/* Core settings */
	gchar											*theme;
	gboolean										allowSubwindows;
	gchar											**enabledPlugins;
	gchar											**favourites;
	gboolean										alwaysLaunchNewInstance;
	gboolean										enableWorkaroundUnmappedWindow;
	gchar											*windowContentCreationPriority;
	gboolean										enableAnimations;
	guint											notificationTimeout;
	gboolean										resetSearchOnResume;
	gchar											*switchToViewOnResume;
	gboolean										reselectThemeFocusOnResume;

	/* Application search provider settings */
	XfdashboardApplicationsSearchProviderSortMode	applicationsSearchProviderSortMode;

	/* Applications view provider settings */
	gboolean										applicationsViewShowAllApps;

	/* Search view settings */
	guint											searchViewDelaySearchTimeout;

	/* Windows view settings */
	gboolean										windowsViewScrollEventChangesWorkspace;

	/* Static runtime settings */
	gchar											**bindingFiles;
	gchar											**themeSearchPaths;
	gchar											**pluginSearchPaths;
	gchar											*configPath;
	gchar											*dataPath;

	/* Instance related */
	GList											*plugins;
};

G_DEFINE_TYPE_WITH_PRIVATE(XfdashboardSettings,
							xfdashboard_settings,
							G_TYPE_INITIALLY_UNOWNED);

/* Properties */
enum
{
	PROP_0,

	/* Core settings */
	PROP_THEME,
	PROP_ALLOW_SUBWINDOWS,
	PROP_ENABLED_PLUGINS,
	PROP_FAVOURITES,
	PROP_ALWAYS_LAUNCH_NEW_INSTANCE,
	PROP_ENABLE_WORKAROUND_UNMAPPED_WINDOW,
	PROP_WINDOW_CONTENT_CREATION_PRIORITY,
	PROP_ENABLE_ANIMATIONS,
	PROP_NOTIFICATION_TIMEOUT,
	PROP_RESET_SEARCH_ON_RESUME,
	PROP_SWITCH_TO_VIEW_ON_RESUME,
	PROP_RESELECT_THEME_FOCUS_ON_RESUME,

	/* Application search provider settings */
	PROP_APPLICATIONS_SEARCH_PROVIDER_SORT_MODE,

	/* Applications view provider settings */
	PROP_APPLICATIONS_VIEW_SHOW_ALL_APPS,

	/* Search view settings */
	PROP_SEARCH_VIEW_DELAY_SEARCH_TIMEOUT,

	/* Windows view settings */
	PROP_WINDOWS_VIEW_SCROLL_EVENT_CHANGES_WORKSPACE,

	/* Static runtime settings */
	PROP_BINDING_FILES,
	PROP_THEME_SEARCH_PATHS,
	PROP_PLUGIN_SEARCH_PATHS,
	PROP_CONFIG_PATH,
	PROP_DATA_PATH,

	PROP_LAST
};

static GParamSpec* XfdashboardSettingsProperties[PROP_LAST]={ 0, };

/* Signals */
enum
{
	SIGNAL_CHANGED,

	SIGNAL_LAST
};

static guint XfdashboardSettingsSignals[SIGNAL_LAST]={ 0, };


/* IMPLEMENTATION: Private variables and methods */
#define DEFAULT_THEME											"xfdashboard"
#define DEFAULT_ALLOW_SUBWINDOWS								TRUE
#define DEFAULT_ALWAYS_LAUNCH_NEW_INSTANCE						TRUE
#define DEFAULT_ENABLE_WORKAROUND_UNMAPPED_WINDOW				FALSE
#define DEFAULT_WINDOW_CONTENT_CREATION_PRIORITY				"immediate"
#define DEFAULT_ENABLE_ANIMATIONS								TRUE
#define DEFAULT_NOTIFICATION_TIMEOUT							3000
#define DEFAULT_RESET_SEARCH_ON_RESUME							TRUE
#define DEFAULT_SWITCH_TO_VIEW_ON_RESUME						NULL
#define DEFAULT_RESELECT_THEME_FOCUS_ON_RESUME					FALSE
#define DEFAULT_APPLICATIONS_SEARCH_PROVIDER_SORT_MODE			XFDASHBOARD_APPLICATIONS_SEARCH_PROVIDER_SORT_MODE_NONE
#define DEFAULT_APPLICATIONS_VIEW_SHOW_ALL_APPS					FALSE
#define DEFAULT_SEARCH_VIEW_DELAY_SEARCH_TIMEOUT				0
#define DEFAULT_WINDOWS_VIEW_SCROLL_EVENT_CHANGES_WORKSPACE		FALSE

typedef struct _XfdashboardSettingsPluginEntry					XfdashboardSettingsPluginEntry;
struct _XfdashboardSettingsPluginEntry
{
	XfdashboardPlugin				*plugin;
	XfdashboardPluginSettings		*pluginSettings;
	guint							pluginSettingsChangedSignalID;
};

/* Free plugin settings entry */
static void _xfdashboard_settings_plugin_entry_free(XfdashboardSettingsPluginEntry *inData)
{
	g_return_if_fail(inData);

	XFDASHBOARD_DEBUG(NULL, PLUGINS,
						"Unregistered plugin '%s' for plugin settings at settings",
						inData->plugin ? xfdashboard_plugin_get_id(inData->plugin) : "<unknown>");

	/* Release allocated resources */
	if(inData->pluginSettings)
	{
		if(inData->pluginSettingsChangedSignalID)
		{
			g_signal_handler_disconnect(inData->pluginSettings, inData->pluginSettingsChangedSignalID);
		}
		g_object_unref(inData->pluginSettings);
	}

	/* Release entry */
	g_free(inData);
}

/* A plugin settings object has changed its settings */
static void _xfdashboard_settings_on_plugin_settings_changed(XfdashboardSettings *self,
																GParamSpec *inParamSpec,
																gpointer inUserData)
{
	XfdashboardSettingsPrivate			*priv;
	XfdashboardPluginSettings			*pluginSettings;
	GList								*iter;
	XfdashboardSettingsPluginEntry		*entry;

	g_return_if_fail(XFDASHBOARD_IS_SETTINGS(self));
	g_return_if_fail(G_IS_PARAM_SPEC(inParamSpec));
	g_return_if_fail(XFDASHBOARD_IS_PLUGIN_SETTINGS(inUserData));

	priv=self->priv;
	pluginSettings=XFDASHBOARD_PLUGIN_SETTINGS(inUserData);

	/* Lookup plugin for plugin setting, retrieve its plugin ID and emit signal */
	for(iter=priv->plugins; iter; iter=g_list_next(iter))
	{
		/* Get iterated registered plugin */
		entry=(XfdashboardSettingsPluginEntry*)iter->data;
		if(!entry) continue;

		/* Check if iterated plugin has the requested ID and return it
		 * if it matches.
		 */
		if(entry->pluginSettings==pluginSettings)
		{
			/* Emit "changed" signal with plugin ID as a plugin settings was changed */
			g_signal_emit(self,
							XfdashboardSettingsSignals[SIGNAL_CHANGED], g_param_spec_get_name_quark(inParamSpec),
							xfdashboard_plugin_get_id(entry->plugin),
							inParamSpec);
			return;
		}
	}
}

/* Set the list of file locations where to load and merge key and mouse bindings
 * from. The binding files are loaded in given order and the binding from the
 * latter binding files will override the existing binding of the former ones.
 * The list of file locations must be a %NULL-terminated list of strings where
 * each string contains the absolute file path to a binding file. Non-existing
 * files will be silently skipped and ignored. Set to NULL if not interested in
 * any binding.
 */
static void _xfdashboard_settings_set_binding_files(XfdashboardSettings *self, const gchar **inFileLocations)
{
	XfdashboardSettingsPrivate		*priv;

	g_return_if_fail(XFDASHBOARD_IS_SETTINGS(self));
	g_return_if_fail(inFileLocations==NULL || *inFileLocations);

	priv=self->priv;

	/* Set value if changed */
	if(!xfdashboard_strv_equal((const gchar**)priv->bindingFiles, inFileLocations))
	{
		/* Set value */
		if(priv->bindingFiles)
		{
			g_strfreev(priv->bindingFiles);
			priv->bindingFiles=NULL;
		}

		if(inFileLocations)
		{
			priv->bindingFiles=g_strdupv((gchar**)inFileLocations);
		}

		/* Notify about property change */
		g_object_notify_by_pspec(G_OBJECT(self), XfdashboardSettingsProperties[PROP_BINDING_FILES]);
	}
}

/* Set the list of paths where to lookup themes at settings. The themes are
 * searched in the %NULL-terminated list of paths in given order and the first
 * path containing the theme will be used. Even if it is possible to set it to
 * NULL, it is not adviced as no theme can be loaded in this case.
 */
static void _xfdashboard_settings_set_theme_search_paths(XfdashboardSettings *self, const gchar **inSearchPaths)
{
	XfdashboardSettingsPrivate		*priv;

	g_return_if_fail(XFDASHBOARD_IS_SETTINGS(self));
	g_return_if_fail(inSearchPaths==NULL || *inSearchPaths);

	priv=self->priv;

	/* Set value if changed */
	if(!xfdashboard_strv_equal((const gchar**)priv->themeSearchPaths, inSearchPaths))
	{
		/* Set value */
		if(priv->themeSearchPaths)
		{
			g_strfreev(priv->themeSearchPaths);
			priv->themeSearchPaths=NULL;
		}

		if(inSearchPaths)
		{
			priv->themeSearchPaths=g_strdupv((gchar**)inSearchPaths);
		}

		/* Notify about property change */
		g_object_notify_by_pspec(G_OBJECT(self), XfdashboardSettingsProperties[PROP_THEME_SEARCH_PATHS]);
	}
}

/* Set the list of paths where to lookup plugins at settings. The plugins are
 * searched in the %NULL-terminated list of paths in given order and the first
 * path containing the plugin will be used. Even if it is possible to set it to
 * NULL, it is not adviced as no plugin can be loaded in this case.
 */
static void _xfdashboard_settings_set_plugin_search_paths(XfdashboardSettings *self, const gchar **inSearchPaths)
{
	XfdashboardSettingsPrivate		*priv;

	g_return_if_fail(XFDASHBOARD_IS_SETTINGS(self));
	g_return_if_fail(inSearchPaths==NULL || *inSearchPaths);

	priv=self->priv;

	/* Set value if changed */
	if(!xfdashboard_strv_equal((const gchar**)priv->pluginSearchPaths, inSearchPaths))
	{
		/* Set value */
		if(priv->pluginSearchPaths)
		{
			g_strfreev(priv->pluginSearchPaths);
			priv->pluginSearchPaths=NULL;
		}

		if(inSearchPaths)
		{
			priv->pluginSearchPaths=g_strdupv((gchar**)inSearchPaths);
		}

		/* Notify about property change */
		g_object_notify_by_pspec(G_OBJECT(self), XfdashboardSettingsProperties[PROP_PLUGIN_SEARCH_PATHS]);
	}
}

/* Set the base path of configuration files for application or NULL to disable
 * application specific configuration files.
 */
static void _xfdashboard_settings_set_config_path(XfdashboardSettings *self, const gchar *inPath)
{
	XfdashboardSettingsPrivate		*priv;

	g_return_if_fail(XFDASHBOARD_IS_SETTINGS(self));
	g_return_if_fail(!inPath || *inPath);

	priv=self->priv;

	/* Set value if changed */
	if(g_strcmp0(priv->configPath, inPath)!=0)
	{
		/* Set value */
		if(priv->configPath)
		{
			g_free(priv->configPath);
			priv->configPath=NULL;
		}

		if(inPath)
		{
			priv->configPath=g_strdup(inPath);
		}

		/* Notify about property change */
		g_object_notify_by_pspec(G_OBJECT(self), XfdashboardSettingsProperties[PROP_CONFIG_PATH]);
	}
}

/* Set the base path of data files for application or NULL to disable application
 * specific data files.
 */
static void _xfdashboard_settings_set_data_path(XfdashboardSettings *self, const gchar *inPath)
{
	XfdashboardSettingsPrivate		*priv;

	g_return_if_fail(XFDASHBOARD_IS_SETTINGS(self));
	g_return_if_fail(!inPath || *inPath);

	priv=self->priv;

	/* Set value if changed */
	if(g_strcmp0(priv->dataPath, inPath)!=0)
	{
		/* Set value */
		if(priv->dataPath)
		{
			g_free(priv->dataPath);
			priv->dataPath=NULL;
		}

		if(inPath)
		{
			priv->dataPath=g_strdup(inPath);
		}

		/* Notify about property change */
		g_object_notify_by_pspec(G_OBJECT(self), XfdashboardSettingsProperties[PROP_CONFIG_PATH]);
	}
}


/* IMPLEMENTATION: GObject */

/* Default nofity signal handler */
static void _xfdashboard_settings_notify(GObject *inObject, GParamSpec *inParamSpec)
{
	XfdashboardSettings	*self=XFDASHBOARD_SETTINGS(inObject);

	/* Only emit "changed" signal if changed property can be read and is not
	 * construct-only (as this one cannot be changed later at runtime).
	 */
	if((inParamSpec->flags & G_PARAM_READABLE) &&
		G_LIKELY(!(inParamSpec->flags & G_PARAM_CONSTRUCT_ONLY)))
	{
		GParamSpec		*redirectTarget;

		/* If the parameter specification is redirected, notify on the target */
		redirectTarget=g_param_spec_get_redirect_target(inParamSpec);
		if(redirectTarget) inParamSpec=redirectTarget;

		/* Emit "changed" signal but set no plugin ID as a core settings was changed */
		g_signal_emit(self, XfdashboardSettingsSignals[SIGNAL_CHANGED], g_param_spec_get_name_quark(inParamSpec), NULL, inParamSpec);
	}
}
 
/* Dispose this object */
static void _xfdashboard_settings_dispose(GObject *inObject)
{
	XfdashboardSettings			*self=XFDASHBOARD_SETTINGS(inObject);
	XfdashboardSettingsPrivate	*priv=self->priv;

	/* Release allocated resources */
	if(priv->theme)
	{
		g_free(priv->theme);
		priv->theme=NULL;
	}

	if(priv->enabledPlugins)
	{
		g_strfreev(priv->enabledPlugins);
		priv->enabledPlugins=NULL;
	}

	if(priv->favourites)
	{
		g_strfreev(priv->favourites);
		priv->favourites=NULL;
	}

	if(priv->windowContentCreationPriority)
	{
		g_free(priv->windowContentCreationPriority);
		priv->windowContentCreationPriority=NULL;
	}

	if(priv->switchToViewOnResume)
	{
		g_free(priv->switchToViewOnResume);
		priv->switchToViewOnResume=NULL;
	}

	if(priv->plugins)
	{
		g_list_free_full(priv->plugins, (GDestroyNotify)_xfdashboard_settings_plugin_entry_free);
		priv->plugins=NULL;
	}

	/* Call parent's class dispose method */
	G_OBJECT_CLASS(xfdashboard_settings_parent_class)->dispose(inObject);
}

/* Set/get properties */
static void _xfdashboard_settings_set_property(GObject *inObject,
												guint inPropID,
												const GValue *inValue,
												GParamSpec *inSpec)
{
	XfdashboardSettings	*self=XFDASHBOARD_SETTINGS(inObject);

	switch(inPropID)
	{
		/* Core settings */
		case PROP_THEME:
			xfdashboard_settings_set_theme(self, g_value_get_string(inValue));
			break;

		case PROP_ALLOW_SUBWINDOWS:
			xfdashboard_settings_set_allow_subwindows(self, g_value_get_boolean(inValue));
			break;

		case PROP_ENABLED_PLUGINS:
			xfdashboard_settings_set_enabled_plugins(self, (const gchar**)g_value_get_boxed(inValue));
			break;

		case PROP_FAVOURITES:
			xfdashboard_settings_set_favourites(self, (const gchar**)g_value_get_boxed(inValue));
			break;

		case PROP_ALWAYS_LAUNCH_NEW_INSTANCE:
			xfdashboard_settings_set_always_launch_new_instance(self, g_value_get_boolean(inValue));
			break;

		case PROP_ENABLE_WORKAROUND_UNMAPPED_WINDOW:
			xfdashboard_settings_set_enable_workaround_unmapped_window(self, g_value_get_boolean(inValue));
			break;

		case PROP_WINDOW_CONTENT_CREATION_PRIORITY:
			xfdashboard_settings_set_window_content_creation_priority(self, g_value_get_string(inValue));
			break;

		case PROP_ENABLE_ANIMATIONS:
			xfdashboard_settings_set_enable_animations(self, g_value_get_boolean(inValue));
			break;

		case PROP_NOTIFICATION_TIMEOUT:
			xfdashboard_settings_set_notification_timeout(self, g_value_get_uint(inValue));
			break;

		case PROP_RESET_SEARCH_ON_RESUME:
			xfdashboard_settings_set_reset_search_on_resume(self, g_value_get_boolean(inValue));
			break;

		case PROP_SWITCH_TO_VIEW_ON_RESUME:
			xfdashboard_settings_set_switch_to_view_on_resume(self, g_value_get_string(inValue));
			break;

		case PROP_RESELECT_THEME_FOCUS_ON_RESUME:
			xfdashboard_settings_set_reselect_theme_focus_on_resume(self, g_value_get_boolean(inValue));
			break;

		/* Applications search provider settings */
		case PROP_APPLICATIONS_SEARCH_PROVIDER_SORT_MODE:
			xfdashboard_settings_set_applications_search_sort_mode(self, g_value_get_flags(inValue));
			break;

		/* Applications view provider settings */
		case PROP_APPLICATIONS_VIEW_SHOW_ALL_APPS:
			xfdashboard_settings_set_show_all_applications(self, g_value_get_boolean(inValue));
			break;

		/* Search view settings */
		case PROP_SEARCH_VIEW_DELAY_SEARCH_TIMEOUT:
			xfdashboard_settings_set_delay_search_timeout(self, g_value_get_uint(inValue));
			break;

		/* Windows view settings */
		case PROP_WINDOWS_VIEW_SCROLL_EVENT_CHANGES_WORKSPACE:
			xfdashboard_settings_set_scroll_event_changes_workspace(self, g_value_get_boolean(inValue));
			break;

		/* Static runtime settings */
		case PROP_BINDING_FILES:
			_xfdashboard_settings_set_binding_files(self, (const gchar**)g_value_get_boxed(inValue));
			break;

		case PROP_THEME_SEARCH_PATHS:
			_xfdashboard_settings_set_theme_search_paths(self, (const gchar**)g_value_get_boxed(inValue));
			break;

		case PROP_PLUGIN_SEARCH_PATHS:
			_xfdashboard_settings_set_plugin_search_paths(self, (const gchar**)g_value_get_boxed(inValue));
			break;

		case PROP_CONFIG_PATH:
			_xfdashboard_settings_set_config_path(self, g_value_get_string(inValue));
			break;

		case PROP_DATA_PATH:
			_xfdashboard_settings_set_data_path(self, g_value_get_string(inValue));
			break;

		/* Unknown settings ;) */
		default:
			G_OBJECT_WARN_INVALID_PROPERTY_ID(inObject, inPropID, inSpec);
			break;
	}
}

static void _xfdashboard_settings_get_property(GObject *inObject,
												guint inPropID,
												GValue *outValue,
												GParamSpec *inSpec)
{
	XfdashboardSettings	*self=XFDASHBOARD_SETTINGS(inObject);

	switch(inPropID)
	{
		/* Core settings */
		case PROP_THEME:
			g_value_set_string(outValue, self->priv->theme);
			break;

		case PROP_ALLOW_SUBWINDOWS:
			g_value_set_boolean(outValue, self->priv->allowSubwindows);
			break;

		case PROP_ENABLED_PLUGINS:
			g_value_set_boxed(outValue, self->priv->enabledPlugins);
			break;

		case PROP_FAVOURITES:
			g_value_set_boxed(outValue, self->priv->favourites);
			break;

		case PROP_ALWAYS_LAUNCH_NEW_INSTANCE:
			g_value_set_boolean(outValue, self->priv->alwaysLaunchNewInstance);
			break;

		case PROP_ENABLE_WORKAROUND_UNMAPPED_WINDOW:
			g_value_set_boolean(outValue, self->priv->enableWorkaroundUnmappedWindow);
			break;

		case PROP_WINDOW_CONTENT_CREATION_PRIORITY:
			g_value_set_string(outValue, self->priv->windowContentCreationPriority);
			break;

		case PROP_ENABLE_ANIMATIONS:
			g_value_set_boolean(outValue, self->priv->enableAnimations);
			break;

		case PROP_NOTIFICATION_TIMEOUT:
			g_value_set_uint(outValue, self->priv->notificationTimeout);
			break;

		case PROP_RESET_SEARCH_ON_RESUME:
			g_value_set_boolean(outValue, self->priv->resetSearchOnResume);
			break;

		case PROP_SWITCH_TO_VIEW_ON_RESUME:
			g_value_set_string(outValue, self->priv->switchToViewOnResume);
			break;

		case PROP_RESELECT_THEME_FOCUS_ON_RESUME:
			g_value_set_boolean(outValue, self->priv->reselectThemeFocusOnResume);
			break;

		/* Application search provider settings */
		case PROP_APPLICATIONS_SEARCH_PROVIDER_SORT_MODE:
			g_value_set_flags(outValue, self->priv->applicationsSearchProviderSortMode);
			break;

		/* Applications view provider settings */
		case PROP_APPLICATIONS_VIEW_SHOW_ALL_APPS:
			g_value_set_boolean(outValue, self->priv->applicationsViewShowAllApps);
			break;

		/* Search view settings */
		case PROP_SEARCH_VIEW_DELAY_SEARCH_TIMEOUT:
			g_value_set_uint(outValue, self->priv->searchViewDelaySearchTimeout);
			break;

		/* Windows view settings */
		case PROP_WINDOWS_VIEW_SCROLL_EVENT_CHANGES_WORKSPACE:
			g_value_set_boolean(outValue, self->priv->windowsViewScrollEventChangesWorkspace);
			break;

		/* Static runtime settings */
		case PROP_BINDING_FILES:
			g_value_set_boxed(outValue, self->priv->bindingFiles);
			break;

		case PROP_THEME_SEARCH_PATHS:
			g_value_set_boxed(outValue, self->priv->themeSearchPaths);
			break;

		case PROP_PLUGIN_SEARCH_PATHS:
			g_value_set_boxed(outValue, self->priv->pluginSearchPaths);
			break;

		case PROP_CONFIG_PATH:
			g_value_set_string(outValue, self->priv->configPath);
			break;

		case PROP_DATA_PATH:
			g_value_set_string(outValue, self->priv->dataPath);
			break;

		/* Unknown settings ;) */
		default:
			G_OBJECT_WARN_INVALID_PROPERTY_ID(inObject, inPropID, inSpec);
			break;
	}
}

/* Class initialization
 * Override functions in parent classes and define properties
 * and signals
 */
static void xfdashboard_settings_class_init(XfdashboardSettingsClass *klass)
{
	GObjectClass		*gobjectClass=G_OBJECT_CLASS(klass);

	/* Override functions */
	gobjectClass->dispose=_xfdashboard_settings_dispose;
	gobjectClass->set_property=_xfdashboard_settings_set_property;
	gobjectClass->get_property=_xfdashboard_settings_get_property;
	gobjectClass->notify=_xfdashboard_settings_notify;

	/* Define properties */
	/**
	 * XfdashboardSettings:theme:
	 *
	 * The name of theme to use in application.
	 */
	XfdashboardSettingsProperties[PROP_THEME]=
		g_param_spec_string("theme",
								"Theme",
								"Name of theme",
								DEFAULT_THEME,
								G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS);

	/**
	 * XfdashboardSettings:allow-subwindows:
	 *
	 * A flag indicating if live windows should also show sub-windows like dialogs etc.
	 * within the main window. If set to %TRUE they will be shown and not shown if
	 * set to %FALSE.
	 */
	XfdashboardSettingsProperties[PROP_ALLOW_SUBWINDOWS]=
		g_param_spec_boolean("allow-subwindows",
								"Allow sub-windows",
								"Whether to show sub-windows if requested by theme",
								DEFAULT_ALLOW_SUBWINDOWS,
								G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS);

	/**
	 * XfdashboardSettings:enabled-plugins:
	 *
	 * A %NULL-terminated list of strings where each one contains the name of the
	 * plugin to load and enable.
	 */
	XfdashboardSettingsProperties[PROP_ENABLED_PLUGINS]=
		g_param_spec_boxed("enabled-plugins",
								"Enabled plugins",
								"An array of strings containing the names of plugins to load and enable",
								G_TYPE_STRV,
								G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS);

	/**
	 * XfdashboardSettings:favourites:
	 *
	 * A %NULL-terminated list of strings where each one contains either the desktop ID
	 * or an absolute file path to the desktop file of a favourite.
	 */
	XfdashboardSettingsProperties[PROP_FAVOURITES]=
		g_param_spec_boxed("favourites",
							"Favourites",
							"An array of strings pointing to desktop files shown as icons",
							G_TYPE_STRV,
							G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS);

	/**
	 * XfdashboardSettings:always-launch-new-instance:
	 *
	 * A flag indicating if a new instance should an application choosen, e.g. at quicklaunch,
	 * should be launched. If set to %TRUE, always a new instance will be launched. If set
	 * to %FALSE an already running instance will be brought to front if existing, otherwise
	 * it launches an instance of chosen application.
	 */
	XfdashboardSettingsProperties[PROP_ALWAYS_LAUNCH_NEW_INSTANCE]=
		g_param_spec_boolean("always-launch-new-instance",
								"Always launch new instance",
								"Whether to always start a new instance of application or to bring an existing one to front",
								DEFAULT_ALWAYS_LAUNCH_NEW_INSTANCE,
								G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS);

	/**
	 * XfdashboardSettings:enable-unmapped-window-workaround:
	 *
	 * A flag indicating if a workaround for unmapped windows should be enabled. If set to %TRUE
	 * this workaround is enabled and if set to %FALSE it is disabled.
	 *
	 * This option is usually only useful at desktop running on an X11 server. The workaround for
	 * window being unmapped is to map them again to make a still image from the window content and
	 * then to unmap the window again. As long as the window is unmapped the still image is shown
	 * as window content.
	 */
	XfdashboardSettingsProperties[PROP_ENABLE_WORKAROUND_UNMAPPED_WINDOW]=
		g_param_spec_boolean("enable-unmapped-window-workaround",
								"Enable unmapped window workaround",
								"Whether to enable a visual workaround for unmapped windows",
								DEFAULT_ENABLE_WORKAROUND_UNMAPPED_WINDOW,
								G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS);

	/**
	 * XfdashboardSettings:window-content-creation-priority:
	 *
	 * A string defining the priority how fast the initial window content image should be created.
	 * Possible values are: immediate, high, normal and low.
	 */
	XfdashboardSettingsProperties[PROP_WINDOW_CONTENT_CREATION_PRIORITY]=
		g_param_spec_string("window-content-creation-priority",
								"Window content creation priority",
								"The priority how fast the initial window content image should be created",
								DEFAULT_WINDOW_CONTENT_CREATION_PRIORITY,
								G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS);

	/**
	 * XfdashboardSettings:enable-animations:
	 *
	 * A flag indicating if animations as specified at theme should be applied and run. If set to
	 * %TRUE then animations are run and if set to %FALSE no animation is played at all.
	 */
	XfdashboardSettingsProperties[PROP_ENABLE_ANIMATIONS]=
		g_param_spec_boolean("enable-animations",
								"Enable animations",
								"Whether to enable visual animations",
								DEFAULT_ENABLE_ANIMATIONS,
								G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS);

	/**
	 * XfdashboardSettings:delay-search-timeout:
	 *
	 * The number of milliseconds to wait at least before a hiding a notification.
	 * The duration how long a notification is shown will be calculated but might
	 * be less than this value so this value defines how long a notification must
	 * be displayed at least.
	 */
	XfdashboardSettingsProperties[PROP_NOTIFICATION_TIMEOUT]=
		g_param_spec_uint("min-notification-timeout",
							"Minimum notification timeout",
							"The number of milliseconds to wait at least before hiding a notification",
							0,
							G_MAXUINT,
							DEFAULT_NOTIFICATION_TIMEOUT,
							G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS);

	/**
	 * XfdashboardSettings:reset-search-on-resume:
	 *
	 * A flag indicating if any started and running search should be stopped and resetted
	 * when application resumes. If set to %TRUE the search box will be emptied resulting
	 * in the active running search to be stopped. If set to %FALSE nothing happens when
	 * application resumes.
	 */
	XfdashboardSettingsProperties[PROP_RESET_SEARCH_ON_RESUME]=
		g_param_spec_boolean("reset-search-on-resume",
								"Reset search on resume",
								"Whether to reset and end search when application resumes",
								DEFAULT_RESET_SEARCH_ON_RESUME,
								G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS);

	/**
	 * XfdashboardSettings:switch-to-view-on-resume:
	 *
	 * A string containing the view's ID to switch to when application resume.
	 * If set to %NULL or if string contains an invalid or unknown view ID
	 * then the view will not be changed when application resumes.
	 */
	XfdashboardSettingsProperties[PROP_SWITCH_TO_VIEW_ON_RESUME]=
		g_param_spec_string("switch-to-view-on-resume",
								"Switch to view on resume",
								"The view ID to switch to when applications resumes",
								DEFAULT_SWITCH_TO_VIEW_ON_RESUME,
								G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS);
	/**
	 * XfdashboardSettings:reselect-theme-focus-on-resume:
	 *
	 * A flag indicating if to focus the actor as it was defined at theme when the application resumes.
	 * If set to %TRUE then the actor which has set the focus at theme will get the focus. If set to
	 * %FALSE the previous focus will be tried to be restored.
	 */
	XfdashboardSettingsProperties[PROP_RESELECT_THEME_FOCUS_ON_RESUME]=
		g_param_spec_boolean("reselect-theme-focus-on-resume",
								"Reselect theme focus on resume",
								"Whether to focus the actor as define as the focused at theme when applications resumes",
								DEFAULT_RESELECT_THEME_FOCUS_ON_RESUME,
								G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS);

	/**
	 * XfdashboardSettings:applications-search-sort-mode:
	 *
	 * The sort mode how applications should be sorted in search result returned
	 * by applications search provider.
	 */
	XfdashboardSettingsProperties[PROP_APPLICATIONS_SEARCH_PROVIDER_SORT_MODE]=
		g_param_spec_flags("applications-search-sort-mode",
							"Application search provider: Sort mode",
							"The sort mode of applications in applications search provider",
							XFDASHBOARD_TYPE_APPLICATIONS_SEARCH_PROVIDER_SORT_MODE,
							DEFAULT_APPLICATIONS_SEARCH_PROVIDER_SORT_MODE,
							G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS);

	/**
	 * XfdashboardSettings:show-all-applications:
	 *
	 * A flag if set to %TRUE the applications in applications will all be shown at once without
	 * any (sub-)menu. If set to %FALSE then category menus containing the corresponding
	 * applications will be shown instead.
	 */
	XfdashboardSettingsProperties[PROP_APPLICATIONS_VIEW_SHOW_ALL_APPS]=
		g_param_spec_boolean("show-all-applications",
								"Applications view: Show all applications",
								"Whether to show all applications in application view at once or to show menus to categorize applications",
								DEFAULT_APPLICATIONS_VIEW_SHOW_ALL_APPS,
								G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS);

	/**
	 * XfdashboardSettings:delay-search-timeout:
	 *
	 * The number of milliseconds to wait before the initial search is performed.
	 */
	XfdashboardSettingsProperties[PROP_SEARCH_VIEW_DELAY_SEARCH_TIMEOUT]=
		g_param_spec_uint("delay-search-timeout",
							"Search view: Delay search timeout",
							"The number of milliseconds to wait before the initial search is performed",
							0,
							G_MAXUINT,
							DEFAULT_SEARCH_VIEW_DELAY_SEARCH_TIMEOUT,
							G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS);

	/**
	 * XfdashboardSettings:scroll-event-changes-workspace:
	 *
	 * A flag indicating if the mouse wheel will generate events to scroll through the available
	 * workspaces. If set to %TRUE this option will be enabled and if set to %FALSE then it will
	 * be disabled.
	 */
	XfdashboardSettingsProperties[PROP_WINDOWS_VIEW_SCROLL_EVENT_CHANGES_WORKSPACE]=
		g_param_spec_boolean("scroll-event-changes-workspace",
								"Windows view: Scroll event changes workspace",
								"Whether the mouse wheel will scroll through the workspaces",
								DEFAULT_WINDOWS_VIEW_SCROLL_EVENT_CHANGES_WORKSPACE,
								G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS);

	/**
	 * XfdashboardSettings:binding-files:
	 *
	 * A %NULL-terminated list of strings where each one contains the file location of a binding
	 * file. The binding files are loaded in the given order and merged wherby the latter bindings
	 * override the former ones. If a file in this list does not exist, it will be skipped silently.
	 * Set to %NULL or leave the default value untouched if not interested in any binding.
	 */
	XfdashboardSettingsProperties[PROP_BINDING_FILES]=
		g_param_spec_boxed("binding-files",
								"Binding files",
								"An array of file locations to load and merge key and mouse bindings from",
								G_TYPE_STRV,
								G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY | G_PARAM_STATIC_STRINGS);

	/**
	 * XfdashboardSettings:theme-search-paths:
	 *
	 * A %NULL-terminated list of strings where each one contains the path the lookup a requested
	 * theme at. The theme is looked up in the order given at this search path list and the first
	 * path containing it will be used. Even if it is possible to set it to %NULL or to keep the
	 * default empty value, it is not adviced not to set this property as no theme can be loaded
	 * in this case.
	 */
	XfdashboardSettingsProperties[PROP_THEME_SEARCH_PATHS]=
		g_param_spec_boxed("theme-search-paths",
								"Theme search paths",
								"An array of paths to look up themes at",
								G_TYPE_STRV,
								G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY | G_PARAM_STATIC_STRINGS);

	/**
	 * XfdashboardSettings:plugin-search-paths:
	 *
	 * A %NULL-terminated list of strings where each one contains the path the lookup a requested
	 * plug-in at. The plug-in is looked up in the order given at this search path list and the first
	 * path containing it will be used. Even if it is possible to set it to %NULL or to keep the
	 * default empty value, it is not adviced not to set this property as as no plugin can be loaded
	 * in this case.
	 */
	XfdashboardSettingsProperties[PROP_PLUGIN_SEARCH_PATHS]=
		g_param_spec_boxed("plugin-search-paths",
								"Plugin search paths",
								"An array of paths to look up plugins at",
								G_TYPE_STRV,
								G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY | G_PARAM_STATIC_STRINGS);

	/**
	 * XfdashboardSettings:config-path:
	 *
	 * The base path of configuration files of application or NULL to disable application specific
	 * configuration files.
	 */
	XfdashboardSettingsProperties[PROP_CONFIG_PATH]=
		g_param_spec_string("config-path",
								"Configuration path",
								"Base path to configuration files of application",
								NULL,
								G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY | G_PARAM_STATIC_STRINGS);

	/**
	 * XfdashboardSettings:data-path:
	 *
	 * The base path of data files of application or NULL to disable application specific data files.
	 */
	XfdashboardSettingsProperties[PROP_DATA_PATH]=
		g_param_spec_string("data-path",
								"Data path",
								"Base path to data files of application",
								NULL,
								G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY | G_PARAM_STATIC_STRINGS);

	g_object_class_install_properties(gobjectClass, PROP_LAST, XfdashboardSettingsProperties);

	/* Define signals */
	/**
	 * XfdashboardSettings::changed:
	 * @self: The settings object whose value or any of its plugin settings value has changed
	 * @inPlugID: %NULL if a core setting has changed or the plugin ID whose value has changed
	 * @inParamSpec: the #GParamSpec of the property which changed
	 *
	 * The ::changed signal is emitted when one of its properties or a property of
	 * a plugin settings has its value set and the GObject::notify signal is emitted
	 * as well.
	 */
	XfdashboardSettingsSignals[SIGNAL_CHANGED]=
		g_signal_new("changed",
						G_TYPE_FROM_CLASS(klass),
						G_SIGNAL_RUN_FIRST | G_SIGNAL_NO_RECURSE | G_SIGNAL_DETAILED | G_SIGNAL_NO_HOOKS | G_SIGNAL_ACTION,
						G_STRUCT_OFFSET(XfdashboardSettingsClass, changed),
						NULL,
						NULL,
						_xfdashboard_marshal_VOID__STRING_PARAM,
						G_TYPE_NONE,
						2,
						G_TYPE_STRING,
						G_TYPE_PARAM);
}

/* Object initialization
 * Create private structure and set up default values
 */
static void xfdashboard_settings_init(XfdashboardSettings *self)
{
	XfdashboardSettingsPrivate	*priv;

	priv=self->priv=xfdashboard_settings_get_instance_private(self);

	/* Set default core settings */
	priv->theme=g_strdup(DEFAULT_THEME);
	priv->allowSubwindows=DEFAULT_ALLOW_SUBWINDOWS;
	priv->enabledPlugins=NULL;
	priv->favourites=NULL;
	priv->alwaysLaunchNewInstance=DEFAULT_ALWAYS_LAUNCH_NEW_INSTANCE;
	priv->enableWorkaroundUnmappedWindow=DEFAULT_ENABLE_WORKAROUND_UNMAPPED_WINDOW;
	priv->windowContentCreationPriority=DEFAULT_WINDOW_CONTENT_CREATION_PRIORITY;
	priv->enableAnimations=DEFAULT_ENABLE_ANIMATIONS;
	priv->notificationTimeout=DEFAULT_NOTIFICATION_TIMEOUT;
	priv->resetSearchOnResume=DEFAULT_RESET_SEARCH_ON_RESUME;
	priv->switchToViewOnResume=g_strdup(DEFAULT_SWITCH_TO_VIEW_ON_RESUME);
	priv->reselectThemeFocusOnResume=DEFAULT_RESELECT_THEME_FOCUS_ON_RESUME;
	priv->bindingFiles=NULL;
	priv->themeSearchPaths=NULL;
	priv->pluginSearchPaths=NULL;
	priv->configPath=NULL;
	priv->dataPath=NULL;

	/* Set default applications search provider settings */
	priv->applicationsSearchProviderSortMode=DEFAULT_APPLICATIONS_SEARCH_PROVIDER_SORT_MODE;

	/* Set default applications view settings */
	priv->applicationsViewShowAllApps=DEFAULT_APPLICATIONS_VIEW_SHOW_ALL_APPS;

	/* Set default search view settings */
	priv->searchViewDelaySearchTimeout=DEFAULT_SEARCH_VIEW_DELAY_SEARCH_TIMEOUT;

	/* Set default windows view settings */
	priv->windowsViewScrollEventChangesWorkspace=DEFAULT_WINDOWS_VIEW_SCROLL_EVENT_CHANGES_WORKSPACE;

	/* Set default values */
	priv->plugins=NULL;
}


/* IMPLEMENTATION: Public API */

/**
 * xfdashboard_settings_add_plugin:
 * @self: A #XfdashboardSettings
 * @inPlugin: The plugin and its plugin settings to add to this settings
 *
 * Adds the plugin at @inPlugin with its plugin settings at settings at @self.
 */
void xfdashboard_settings_add_plugin(XfdashboardSettings *self, XfdashboardPlugin *inPlugin)
{
	XfdashboardSettingsPrivate		*priv;
	XfdashboardSettingsClass		*klass;
	GList							*iter;
	XfdashboardPluginSettings		*settings;
	XfdashboardSettingsPluginEntry	*entry;

	g_return_if_fail(XFDASHBOARD_IS_SETTINGS(self));
	g_return_if_fail(XFDASHBOARD_IS_PLUGIN(inPlugin));

	priv=self->priv;
	klass=XFDASHBOARD_SETTINGS_GET_CLASS(self);

	/* Check if plugin is already added to list */
	for(iter=priv->plugins; iter; iter=g_list_next(iter))
	{
		/* Get iterated plugin */
		entry=(XfdashboardSettingsPluginEntry*)iter->data;
		if(!entry) continue;

		/* Check if iterated plugin is the same as the one to add and return
		 * immediately if they match.
		 */
		if(entry->plugin==inPlugin ||
			g_strcmp0(xfdashboard_plugin_get_id(entry->plugin), xfdashboard_plugin_get_id(inPlugin))==0)
		{
			g_warning("Plugin settings for plugin '%s' is already registered",
						xfdashboard_plugin_get_id(inPlugin));
			return;
		}
	}

	/* Get settings of plugin loaded */
	settings=xfdashboard_plugin_get_settings(inPlugin);

	/* If we get here the plugin is not added yet, so set up plugin entry,
	 * connect signals and add to list. Do not take a reference on plugin as
	 * it will prevent the plugin to get unloaded.
	 */
	entry=g_new0(XfdashboardSettingsPluginEntry, 1);
	entry->plugin=inPlugin;
	if(settings)
	{
		entry->pluginSettings=g_object_ref(settings);
		entry->pluginSettingsChangedSignalID=
			g_signal_connect_swapped(settings,
										"changed",
										G_CALLBACK(_xfdashboard_settings_on_plugin_settings_changed),
										self);
	}

	/* Add new plugin and its plugin settings to list */ 
	priv->plugins=g_list_prepend(priv->plugins, entry);

	/* Call virtual funtion for plugin added */
	if(klass->plugin_added)
	{
		(klass->plugin_added)(self, inPlugin);
	}

	XFDASHBOARD_DEBUG(self, PLUGINS,
						"Added plugin '%s' with plugin settings %s to settings",
						xfdashboard_plugin_get_id(inPlugin),
						settings ? G_OBJECT_TYPE_NAME(settings) : "<null>");
}

/**
 * xfdashboard_settings_remove_plugin:
 * @self: A #XfdashboardSettings
 * @inPlugin: The plugin with its plugin settings to remove from this settings
 *
 * Removes the plugin at @inPlugin with its plugin settings from settings at @self.
 */
void xfdashboard_settings_remove_plugin(XfdashboardSettings *self, XfdashboardPlugin *inPlugin)
{
	XfdashboardSettingsPrivate		*priv;
	XfdashboardSettingsClass		*klass;
	GList							*iter;
	XfdashboardSettingsPluginEntry	*entry;

	g_return_if_fail(XFDASHBOARD_IS_SETTINGS(self));
	g_return_if_fail(XFDASHBOARD_IS_PLUGIN(inPlugin));

	priv=self->priv;
	klass=XFDASHBOARD_SETTINGS_GET_CLASS(self);

	/* Lookup entries matching requested plugin to remove */
	iter=priv->plugins;
	while(iter)
	{
		GList						*currentIter;

		/* Get next entry to iterate */
		currentIter=iter;
		iter=g_list_next(iter);

		/* Get iterated registered plugin */
		entry=(XfdashboardSettingsPluginEntry*)currentIter->data;
		if(!entry) continue;

		/* If this entry matches the requested plugin, remove it */
		if(entry->plugin==inPlugin ||
			g_strcmp0(xfdashboard_plugin_get_id(entry->plugin), xfdashboard_plugin_get_id(inPlugin))==0)
		{
			/* Remove entry from list */
			priv->plugins=g_list_remove_link(priv->plugins, currentIter);

			/* Call virtual funtion for plugin removed */
			if(klass->plugin_removed)
			{
				(klass->plugin_removed)(self, entry->plugin);
			}

			/* Release entry itself */
			XFDASHBOARD_DEBUG(self, PLUGINS,
								"Removed plugin '%s' with plugin settings %s from settings",
								xfdashboard_plugin_get_id(entry->plugin),
								entry->pluginSettings ? G_OBJECT_TYPE_NAME(entry->pluginSettings) : "<null>");
			_xfdashboard_settings_plugin_entry_free(entry);
			g_list_free(currentIter);
		}
	}
}

/**
 * xfdashboard_settings_lookup_plugin_by_id:
 * @self: A #XfdashboardSettings
 * @inPluginID: The plugin's ID to lookup
 *
 * Iterates through the list of plugins added at settings at @self and returns
 * the plugin of type #XfdashboardPlugin matching the requested plugin ID at
 * @inPluginID.
 *
 * Return value: The plugin of type #XfdashboardPlugin found or %NULL if plugin
 *   was not found.
 */
XfdashboardPlugin* xfdashboard_settings_lookup_plugin_by_id(XfdashboardSettings *self, const gchar *inPluginID)
{
	XfdashboardSettingsPrivate		*priv;
	GList							*iter;
	XfdashboardSettingsPluginEntry	*entry;

	g_return_val_if_fail(XFDASHBOARD_IS_SETTINGS(self), NULL);
	g_return_val_if_fail(inPluginID && *inPluginID, NULL);

	priv=self->priv;

	/* Iterate through list of plugins and lookup requested ID */
	for(iter=priv->plugins; iter; iter=g_list_next(iter))
	{
		/* Get iterated registered plugin */
		entry=(XfdashboardSettingsPluginEntry*)iter->data;
		if(!entry) continue;

		/* Check if iterated plugin has the requested ID and return it
		 * if it matches.
		 */
		if(g_strcmp0(xfdashboard_plugin_get_id(entry->plugin), inPluginID)==0)
		{
			return(entry->plugin);
		}
	}

	/* If we get here the plugin ID was not found, so return NULL */
	return(NULL);
}

/**
 * xfdashboard_settings_get_theme:
 * @self: A #XfdashboardSettings
 *
 * Retrieves the name of theme in settings at @self.
 *
 * Return value: The name of theme
 */
const gchar* xfdashboard_settings_get_theme(XfdashboardSettings *self)
{
	g_return_val_if_fail(XFDASHBOARD_IS_SETTINGS(self), NULL);

	return(self->priv->theme);
}

/**
 * xfdashboard_settings_set_theme:
 * @self: A #XfdashboardSettings
 * @inTheme: The name of new theme
 *
 * Sets the name of theme in settings at @self.
 */
void xfdashboard_settings_set_theme(XfdashboardSettings *self, const gchar *inTheme)
{
	XfdashboardSettingsPrivate		*priv;

	g_return_if_fail(XFDASHBOARD_IS_SETTINGS(self));
	g_return_if_fail(inTheme && *inTheme);

	priv=self->priv;

	/* Set value if changed */
	if(g_strcmp0(priv->theme, inTheme)!=0)
	{
		/* Set value */
		if(priv->theme)
		{
			g_free(priv->theme);
			priv->theme=NULL;
		}

		if(inTheme)
		{
			priv->theme=g_strdup(inTheme);
		}

		/* Notify about property change */
		g_object_notify_by_pspec(G_OBJECT(self), XfdashboardSettingsProperties[PROP_THEME]);
	}
}

/**
 * xfdashboard_settings_get_allow_subwindows:
 * @self: A #XfdashboardSettings
 *
 * Retrieve if live windows (actors derived from type #XfdashboardLiveWindow) should
 * show their corresponding sub-windows from settings at @self.
 *
 * Return value: %TRUE if sub-windows should be shown, otherwise %FALSE
 */
gboolean xfdashboard_settings_get_allow_subwindows(XfdashboardSettings *self)
{
	g_return_val_if_fail(XFDASHBOARD_IS_SETTINGS(self), FALSE);

	return(self->priv->allowSubwindows);
}

/**
 * xfdashboard_settings_set_allow_subwindows:
 * @self: A #XfdashboardSettings
 * @inAllowSubwindows: Whether to show sub-windows of a live window
 *
 * Sets if live windows (actors derived from type #XfdashboardLiveWindow) should also show
 * their corresponding sub-windows, like dialogs etc., above the live window it shows in
 * settings at @self. If @inAllowSubwindows set to %TRUE then they will be shown and if set
 * to %FALSE they will not be shown.
 */
void xfdashboard_settings_set_allow_subwindows(XfdashboardSettings *self, gboolean inAllowSubwindows)
{
	XfdashboardSettingsPrivate		*priv;

	g_return_if_fail(XFDASHBOARD_IS_SETTINGS(self));

	priv=self->priv;

	/* Set value if changed */
	if(priv->allowSubwindows!=inAllowSubwindows)
	{
		/* Set value */
		priv->allowSubwindows=inAllowSubwindows;

		/* Notify about property change */
		g_object_notify_by_pspec(G_OBJECT(self), XfdashboardSettingsProperties[PROP_ALLOW_SUBWINDOWS]);
	}
}

/**
 * xfdashboard_settings_get_enabled_plugins:
 * @self: A #XfdashboardSettings
 *
 * Retrieve the list of enabled plugins in settings at @self.
 *
 * Return value: A %NULL-terminated list of strings containing
 *   the names of plugins enabled or %NULL if no plugin was enabled
 *   at all.
 */
const gchar** xfdashboard_settings_get_enabled_plugins(XfdashboardSettings *self)
{
	g_return_val_if_fail(XFDASHBOARD_IS_SETTINGS(self), NULL);

	return((const gchar**)self->priv->enabledPlugins);
}

/**
 * xfdashboard_settings_set_enabled_plugins:
 * @self: A #XfdashboardSettings
 * @inEnabledPlugins: The list of enabled plugins
 *
 * Sets the list of enabled plugins from @inEnabledPlugins in settings at @self.
 * The list of enabled plugins at @inEnabledPlugins must be a %NULL-terminated
 * list of strings where each string contains the name of the plugin to load
 * and enable or set @inEnabledPlugins to %NULL to prevent enabling any plugin
 * at all.
 */
void xfdashboard_settings_set_enabled_plugins(XfdashboardSettings *self, const gchar **inEnabledPlugins)
{
	XfdashboardSettingsPrivate		*priv;

	g_return_if_fail(XFDASHBOARD_IS_SETTINGS(self));
	g_return_if_fail(inEnabledPlugins==NULL || *inEnabledPlugins);

	priv=self->priv;

	/* Set value if changed */
	if(!xfdashboard_strv_equal((const gchar**)priv->enabledPlugins, inEnabledPlugins))
	{
		/* Set value */
		if(priv->enabledPlugins)
		{
			g_strfreev(priv->enabledPlugins);
			priv->enabledPlugins=NULL;
		}

		if(inEnabledPlugins)
		{
			priv->enabledPlugins=g_strdupv((gchar**)inEnabledPlugins);
		}

		/* Notify about property change */
		g_object_notify_by_pspec(G_OBJECT(self), XfdashboardSettingsProperties[PROP_ENABLED_PLUGINS]);
	}
}

/**
 * xfdashboard_settings_get_favourites:
 * @self: A #XfdashboardSettings
 *
 * Retrieve the list of favourite applications in settings at @self.
 *
 * Return value: A %NULL-terminated list of strings containing either the
 *   desktop ID or an absolute file path to the desktop file of the favourite
 *   application or %NULL if no favourite application was set at all.
 */
const gchar** xfdashboard_settings_get_favourites(XfdashboardSettings *self)
{
	g_return_val_if_fail(XFDASHBOARD_IS_SETTINGS(self), NULL);

	return((const gchar**)self->priv->favourites);
}

/**
 * xfdashboard_settings_set_favourites:
 * @self: A #XfdashboardSettings
 * @inFavourites: The list of favourite applications
 *
 * Sets the list of favourite applications from @inFavourites in settings at @self.
 * The list of favourite applications at @inFavourites must be a %NULL-terminated
 * list of strings where each string contains either the desktop ID or an absolute
 * file path to the desktop file of that favourite application or set @inFavourites
 * to %NULL for an empty list.
 */
void xfdashboard_settings_set_favourites(XfdashboardSettings *self, const gchar **inFavourites)
{
	XfdashboardSettingsPrivate		*priv;

	g_return_if_fail(XFDASHBOARD_IS_SETTINGS(self));
	g_return_if_fail(inFavourites==NULL || *inFavourites);

	priv=self->priv;

	/* Set value if changed */
	if(!xfdashboard_strv_equal((const gchar**)priv->favourites, inFavourites))
	{
		/* Set value */
		if(priv->favourites)
		{
			g_strfreev(priv->favourites);
			priv->favourites=NULL;
		}

		if(inFavourites)
		{
			priv->favourites=g_strdupv((gchar**)inFavourites);
		}

		/* Notify about property change */
		g_object_notify_by_pspec(G_OBJECT(self), XfdashboardSettingsProperties[PROP_FAVOURITES]);
	}
}

/**
 * xfdashboard_settings_get_always_launch_new_instance:
 * @self: A #XfdashboardSettings
 *
 * Retrieve if always new instances of chosen applications should be launched
 * from settings at @self.
 *
 * Return value: %TRUE if always new instances should be launched, otherwise %FALSE
 */
gboolean xfdashboard_settings_get_always_launch_new_instance(XfdashboardSettings *self)
{
	g_return_val_if_fail(XFDASHBOARD_IS_SETTINGS(self), FALSE);

	return(self->priv->alwaysLaunchNewInstance);
}

/**
 * xfdashboard_settings_set_always_launch_new_instance:
 * @self: A #XfdashboardSettings
 * @inAlwaysLaunchNewInstance: Whether to always launch new instances of applications
 *
 * Sets if always a new instance of an application should be launched in settings
 * at @self. If @inAlwaysLaunchNewInstance is set to %TRUE, always a new instance
 * of chosen application will be launched. If it is set to %FALSE an already running
 * instance will be brought to front if existing, otherwise it launches an instance.
 */
void xfdashboard_settings_set_always_launch_new_instance(XfdashboardSettings *self, gboolean inAlwaysLaunchNewInstance)
{
	XfdashboardSettingsPrivate		*priv;

	g_return_if_fail(XFDASHBOARD_IS_SETTINGS(self));

	priv=self->priv;

	/* Set value if changed */
	if(priv->alwaysLaunchNewInstance!=inAlwaysLaunchNewInstance)
	{
		/* Set value */
		priv->alwaysLaunchNewInstance=inAlwaysLaunchNewInstance;

		/* Notify about property change */
		g_object_notify_by_pspec(G_OBJECT(self), XfdashboardSettingsProperties[PROP_ALWAYS_LAUNCH_NEW_INSTANCE]);
	}
}

/**
 * xfdashboard_settings_get_enable_workaround_unmapped_window:
 * @self: A #XfdashboardSettings
 *
 * Retrieve if a workaround for unmapped windows should be enabled from settings
 * at @self.
 *
 * Return value: %TRUE if this workaround should enabled, otherwise %FALSE
 */
gboolean xfdashboard_settings_get_enable_workaround_unmapped_window(XfdashboardSettings *self)
{
	g_return_val_if_fail(XFDASHBOARD_IS_SETTINGS(self), FALSE);

	return(self->priv->enableWorkaroundUnmappedWindow);
}

/**
 * xfdashboard_settings_set_enable_workaround_unmapped_window:
 * @self: A #XfdashboardSettings
 * @inEnableWorkaroundUnmappedWindow: Whether to enable a workaround for unmapped windows
 *
 * Sets if a workaround for unmapped windows should be enabled in settings at @self.
 * If @inEnableWorkaroundUnmappedWindow is set to %TRUE, the workaround for unmapped
 * windows is enabled and will be used. If it is set to %FALSE, it is disabled and
 * will not be used.
 */
void xfdashboard_settings_set_enable_workaround_unmapped_window(XfdashboardSettings *self, gboolean inEnableWorkaroundUnmappedWindow)
{
	XfdashboardSettingsPrivate		*priv;

	g_return_if_fail(XFDASHBOARD_IS_SETTINGS(self));

	priv=self->priv;

	/* Set value if changed */
	if(priv->enableWorkaroundUnmappedWindow!=inEnableWorkaroundUnmappedWindow)
	{
		/* Set value */
		priv->enableWorkaroundUnmappedWindow=inEnableWorkaroundUnmappedWindow;

		/* Notify about property change */
		g_object_notify_by_pspec(G_OBJECT(self), XfdashboardSettingsProperties[PROP_ENABLE_WORKAROUND_UNMAPPED_WINDOW]);
	}
}

/**
 * xfdashboard_settings_get_window_content_creation_priority:
 * @self: A #XfdashboardSettings
 *
 * Retrieves the priority of window content creation from settings at @self.
 *
 * Return value: The priority
 */
const gchar* xfdashboard_settings_get_window_content_creation_priority(XfdashboardSettings *self)
{
	g_return_val_if_fail(XFDASHBOARD_IS_SETTINGS(self), NULL);

	return(self->priv->windowContentCreationPriority);
}

/**
 * xfdashboard_settings_set_window_content_creation_priority:
 * @self: A #XfdashboardSettings
 * @inWindowContentCreationPriority: The priority of window content creation
 *
 * Sets the priority how fast the initial window content image should be created
 * in settings at @self.
 */
void xfdashboard_settings_set_window_content_creation_priority(XfdashboardSettings *self, const gchar *inWindowContentCreationPriority)
{
	XfdashboardSettingsPrivate		*priv;

	g_return_if_fail(XFDASHBOARD_IS_SETTINGS(self));
	g_return_if_fail(inWindowContentCreationPriority && *inWindowContentCreationPriority);

	priv=self->priv;

	/* Set value if changed */
	if(g_strcmp0(priv->windowContentCreationPriority, inWindowContentCreationPriority)!=0)
	{
		/* Set value */
		if(priv->windowContentCreationPriority)
		{
			g_free(priv->windowContentCreationPriority);
			priv->windowContentCreationPriority=NULL;
		}

		if(inWindowContentCreationPriority)
		{
			priv->windowContentCreationPriority=g_strdup(inWindowContentCreationPriority);
		}

		/* Notify about property change */
		g_object_notify_by_pspec(G_OBJECT(self), XfdashboardSettingsProperties[PROP_WINDOW_CONTENT_CREATION_PRIORITY]);
	}
}

/**
 * xfdashboard_settings_get_enable_animations:
 * @self: A #XfdashboardSettings
 *
 * Retrieve if animations are enabled from settings at @self.
 *
 * Return value: %TRUE if animations are enabled, otherwise %FALSE
 */
gboolean xfdashboard_settings_get_enable_animations(XfdashboardSettings *self)
{
	g_return_val_if_fail(XFDASHBOARD_IS_SETTINGS(self), FALSE);

	return(self->priv->enableAnimations);
}

/**
 * xfdashboard_settings_set_enable_animations:
 * @self: A #XfdashboardSettings
 * @inEnableAnimations: Whether to enable a workaround for unmapped windows
 *
 * Sets if animations as specified at theme should be applied and run in settings
 * at @self. If @inEnableAnimations is set to %TRUE, then animations are enabled
 * and run. If it is set to %FALSE no animation is played at all.
 */
void xfdashboard_settings_set_enable_animations(XfdashboardSettings *self, gboolean inEnableAnimations)
{
	XfdashboardSettingsPrivate		*priv;

	g_return_if_fail(XFDASHBOARD_IS_SETTINGS(self));

	priv=self->priv;

	/* Set value if changed */
	if(priv->enableAnimations!=inEnableAnimations)
	{
		/* Set value */
		priv->enableAnimations=inEnableAnimations;

		/* Notify about property change */
		g_object_notify_by_pspec(G_OBJECT(self), XfdashboardSettingsProperties[PROP_ENABLE_ANIMATIONS]);
	}
}

/**
 * xfdashboard_settings_get_notification_timeout:
 * @self: A #XfdashboardSettings
 *
 * Retrieve the minimum duration notifications are displayed from settings
 * at @self.
 *
 * Return value: The duration in milliseconds.
 */
guint xfdashboard_settings_get_notification_timeout(XfdashboardSettings *self)
{
	g_return_val_if_fail(XFDASHBOARD_IS_SETTINGS(self), FALSE);

	return(self->priv->notificationTimeout);
}

/**
 * xfdashboard_settings_set_notification_timeout:
 * @self: A #XfdashboardSettings
 * @inNotificationTimeout: The minimum timeout for notifications
 *
 * Sets the minimum duration notifications are displayed in settings at @self.
 * The duration to set is specified in milliseconds at @inNotificationTimeout.
 */
void xfdashboard_settings_set_notification_timeout(XfdashboardSettings *self, guint inNotificationTimeout)
{
	XfdashboardSettingsPrivate		*priv;

	g_return_if_fail(XFDASHBOARD_IS_SETTINGS(self));

	priv=self->priv;

	/* Set value if changed */
	if(priv->notificationTimeout!=inNotificationTimeout)
	{
		/* Set value */
		priv->notificationTimeout=inNotificationTimeout;

		/* Notify about property change */
		g_object_notify_by_pspec(G_OBJECT(self), XfdashboardSettingsProperties[PROP_NOTIFICATION_TIMEOUT]);
	}
}

/**
 * xfdashboard_settings_get_reset_search_on_resume:
 * @self: A #XfdashboardSettings
 *
 * Retrieve if active searchs should be stopped and resetted when application
 * resumes from settings at @self.
 *
 * Return value: %TRUE if active searches should be stopped and resetted,
 *   otherwise %FALSE
 */
gboolean xfdashboard_settings_get_reset_search_on_resume(XfdashboardSettings *self)
{
	g_return_val_if_fail(XFDASHBOARD_IS_SETTINGS(self), FALSE);

	return(self->priv->resetSearchOnResume);
}

/**
 * xfdashboard_settings_set_reset_search_on_resume:
 * @self: A #XfdashboardSettings
 * @inResetSearchOnResume: Whether to reset active searches when application resumes
 *
 * Sets if any active and running search should be stopped and resetted in settings
 * at @self. If @inResetSearchOnResume is set to %TRUE, the search box will be emptied
 * resulting in any active running search to be stopped and resetted. If it is set
 * to %FALSE nothing happens when application resumes.
 */
void xfdashboard_settings_set_reset_search_on_resume(XfdashboardSettings *self, gboolean inResetSearchOnResume)
{
	XfdashboardSettingsPrivate		*priv;

	g_return_if_fail(XFDASHBOARD_IS_SETTINGS(self));

	priv=self->priv;

	/* Set value if changed */
	if(priv->resetSearchOnResume!=inResetSearchOnResume)
	{
		/* Set value */
		priv->resetSearchOnResume=inResetSearchOnResume;

		/* Notify about property change */
		g_object_notify_by_pspec(G_OBJECT(self), XfdashboardSettingsProperties[PROP_RESET_SEARCH_ON_RESUME]);
	}
}

/**
 * xfdashboard_settings_get_switch_to_view_on_resume:
 * @self: A #XfdashboardSettings
 *
 * Retrieves the ID of view to switch to when application resumes from
 * settings at @self.
 *
 * Return value: The view ID
 */
const gchar* xfdashboard_settings_get_switch_to_view_on_resume(XfdashboardSettings *self)
{
	g_return_val_if_fail(XFDASHBOARD_IS_SETTINGS(self), NULL);

	return(self->priv->switchToViewOnResume);
}

/**
 * xfdashboard_settings_set_switch_to_view_on_resume:
 * @self: A #XfdashboardSettings
 * @inSwitchToViewOnResume: The view ID to switch to when application resumes
 *
 * Sets the view ID at @inSwitchToViewOnResume to switch to when application
 * resumes in settings at @self. If @inSwitchToViewOnResume is set to %NULL
 * or to an invalid or unknown view ID then the view will not be changed when
 * application resumes.
 */
void xfdashboard_settings_set_switch_to_view_on_resume(XfdashboardSettings *self, const gchar *inSwitchToViewOnResume)
{
	XfdashboardSettingsPrivate		*priv;

	g_return_if_fail(XFDASHBOARD_IS_SETTINGS(self));
	g_return_if_fail(inSwitchToViewOnResume==NULL || *inSwitchToViewOnResume);

	priv=self->priv;

	/* Set value if changed */
	if(g_strcmp0(priv->switchToViewOnResume, inSwitchToViewOnResume)!=0)
	{
		/* Set value */
		if(priv->switchToViewOnResume)
		{
			g_free(priv->switchToViewOnResume);
			priv->switchToViewOnResume=NULL;
		}

		if(inSwitchToViewOnResume)
		{
			priv->switchToViewOnResume=g_strdup(inSwitchToViewOnResume);
		}

		/* Notify about property change */
		g_object_notify_by_pspec(G_OBJECT(self), XfdashboardSettingsProperties[PROP_SWITCH_TO_VIEW_ON_RESUME]);
	}
}

/**
 * xfdashboard_settings_get_reselect_theme_focus_on_resume:
 * @self: A #XfdashboardSettings
 *
 * Retrieve if to focus the actor as it was defined at theme when application
 * resumes from settings at @self.
 *
 * Return value: %TRUE if actor should be refocused, otherwise %FALSE
 */
gboolean xfdashboard_settings_get_reselect_theme_focus_on_resume(XfdashboardSettings *self)
{
	g_return_val_if_fail(XFDASHBOARD_IS_SETTINGS(self), FALSE);

	return(self->priv->reselectThemeFocusOnResume);
}

/**
 * xfdashboard_settings_set_reselect_theme_focus_on_resume:
 * @self: A #XfdashboardSettings
 * @inReselectThemeFocusOnResume: Whether to refocus the actor as defined at theme
 *   when application resumes
 *
 * Sets if the actor, as defined at theme, should be focused when applications
 * resumes in settings at @self. If @inReselectThemeFocusOnResume is set to %TRUE,
 * the actor which has set the focus set at theme will get the focus when application
 * resumes. If it is set to %FALSE, the previous focus will be tried to be restored.
 */
void xfdashboard_settings_set_reselect_theme_focus_on_resume(XfdashboardSettings *self, gboolean inReselectThemeFocusOnResume)
{
	XfdashboardSettingsPrivate		*priv;

	g_return_if_fail(XFDASHBOARD_IS_SETTINGS(self));

	priv=self->priv;

	/* Set value if changed */
	if(priv->reselectThemeFocusOnResume!=inReselectThemeFocusOnResume)
	{
		/* Set value */
		priv->reselectThemeFocusOnResume=inReselectThemeFocusOnResume;

		/* Notify about property change */
		g_object_notify_by_pspec(G_OBJECT(self), XfdashboardSettingsProperties[PROP_RESELECT_THEME_FOCUS_ON_RESUME]);
	}
}

/**
 * xfdashboard_settings_get_applications_search_sort_mode:
 * @self: A #XfdashboardSettings
 *
 * Retrieve the sort mode of applications of applications search provider from
 * settings at @self.
 *
 * Return value: The sort mode of type #XfdashboardApplicationsSearchProviderSortMode
 */
XfdashboardApplicationsSearchProviderSortMode xfdashboard_settings_get_applications_search_sort_mode(XfdashboardSettings *self)
{
	g_return_val_if_fail(XFDASHBOARD_IS_SETTINGS(self), FALSE);

	return(self->priv->applicationsSearchProviderSortMode);
}

/**
 * xfdashboard_settings_set_applications_search_sort_mode:
 * @self: A #XfdashboardSettings
 * @inApplicationsSearchProviderSortMode: The sort mode of applications at
 *   applications search provider
 *
 * Sets the sort mode of applications from applications search provider in settings
 * at @self. The sort mode at @inApplicationsSearchProviderSortMode can be a
 * combination of the flags of type #XfdashboardApplicationsSearchProviderSortMode.
 */
void xfdashboard_settings_set_applications_search_sort_mode(XfdashboardSettings *self, XfdashboardApplicationsSearchProviderSortMode inApplicationsSearchProviderSortMode)
{
	XfdashboardSettingsPrivate		*priv;

	g_return_if_fail(XFDASHBOARD_IS_SETTINGS(self));

	priv=self->priv;

	/* Set value if changed */
	if(priv->applicationsSearchProviderSortMode!=inApplicationsSearchProviderSortMode)
	{
		/* Set value */
		priv->applicationsSearchProviderSortMode=inApplicationsSearchProviderSortMode;

		/* Notify about property change */
		g_object_notify_by_pspec(G_OBJECT(self), XfdashboardSettingsProperties[PROP_APPLICATIONS_SEARCH_PROVIDER_SORT_MODE]);
	}
}

/**
 * xfdashboard_settings_get_show_all_applications:
 * @self: A #XfdashboardSettings
 *
 * Retrieve if applications in applications view are shown all at once or with menus
 * from settings at @self.
 *
 * Return value: %TRUE if all applications should be shown at once without menus,
 *   otherwise %FALSE to show applications structured in menus
 */
gboolean xfdashboard_settings_get_show_all_applications(XfdashboardSettings *self)
{
	g_return_val_if_fail(XFDASHBOARD_IS_SETTINGS(self), FALSE);

	return(self->priv->windowsViewScrollEventChangesWorkspace);
}

/**
 * xfdashboard_settings_set_show_all_applications:
 * @self: A #XfdashboardSettings
 * @inApplicationsViewShowAllApps: Whether to show all applications at once or
 *   structured in menus
 *
 * Sets if applications in applications view are shown all at once or with menus
 * in settings at @self. If @inApplicationsViewShowAllApps is set to %TRUE, all
 * applications in applications view will be shown at once. When set to %FALSE,
 * the applications will be structured with menus.
 */
void xfdashboard_settings_set_show_all_applications(XfdashboardSettings *self, gboolean inApplicationsViewShowAllApps)
{
	XfdashboardSettingsPrivate		*priv;

	g_return_if_fail(XFDASHBOARD_IS_SETTINGS(self));

	priv=self->priv;

	/* Set value if changed */
	if(priv->applicationsViewShowAllApps!=inApplicationsViewShowAllApps)
	{
		/* Set value */
		priv->applicationsViewShowAllApps=inApplicationsViewShowAllApps;

		/* Notify about property change */
		g_object_notify_by_pspec(G_OBJECT(self), XfdashboardSettingsProperties[PROP_APPLICATIONS_VIEW_SHOW_ALL_APPS]);
	}
}

/**
 * xfdashboard_settings_get_delay_search_timeout:
 * @self: A #XfdashboardSettings
 *
 * Retrieve the duration to wait before the initial search is performed from
 * settings at @self.
 *
 * Return value: The duration in milliseconds.
 */
guint xfdashboard_settings_get_delay_search_timeout(XfdashboardSettings *self)
{
	g_return_val_if_fail(XFDASHBOARD_IS_SETTINGS(self), FALSE);

	return(self->priv->searchViewDelaySearchTimeout);
}

/**
 * xfdashboard_settings_set_delay_search_timeout:
 * @self: A #XfdashboardSettings
 * @inSearchViewDelaySearchTimeout: The minimum timeout for notifications
 *
 * Sets the duration to wait before the initial search is performed in settings
 * at @self. The duration is specified in milliseconds at @inSearchViewDelaySearchTimeout.
 */
void xfdashboard_settings_set_delay_search_timeout(XfdashboardSettings *self, guint inSearchViewDelaySearchTimeout)
{
	XfdashboardSettingsPrivate		*priv;

	g_return_if_fail(XFDASHBOARD_IS_SETTINGS(self));

	priv=self->priv;

	/* Set value if changed */
	if(priv->searchViewDelaySearchTimeout!=inSearchViewDelaySearchTimeout)
	{
		/* Set value */
		priv->searchViewDelaySearchTimeout=inSearchViewDelaySearchTimeout;

		/* Notify about property change */
		g_object_notify_by_pspec(G_OBJECT(self), XfdashboardSettingsProperties[PROP_SEARCH_VIEW_DELAY_SEARCH_TIMEOUT]);
	}
}

/**
 * xfdashboard_settings_get_scroll_event_changes_workspace:
 * @self: A #XfdashboardSettings
 *
 * Retrieve if scolling through workspaces with mouse wheel in windows view is
 * enabled from settings at @self.
 *
 * Return value: %TRUE if scrolling through workspaces with mouse wheel is enabled,
 *   otherwise %FALSE
 */
gboolean xfdashboard_settings_get_scroll_event_changes_workspace(XfdashboardSettings *self)
{
	g_return_val_if_fail(XFDASHBOARD_IS_SETTINGS(self), FALSE);

	return(self->priv->windowsViewScrollEventChangesWorkspace);
}

/**
 * xfdashboard_settings_set_scroll_event_changes_workspace:
 * @self: A #XfdashboardSettings
 * @inWindowsViewScrollEventChangesWorkspace: Whether to enable scrolling though workspace
 *   with mouse wheel at windows view
 *
 * Sets if mouse wheel events will scroll through workspaces at windows view in settings
 * at @self. If @inWindowsViewScrollEventChangesWorkspace is set to %TRUE, it enables to scroll
 * through workspaces at windows view with the mouse whell and setting it to %FALSE will disable
 * it.
 */
void xfdashboard_settings_set_scroll_event_changes_workspace(XfdashboardSettings *self, gboolean inWindowsViewScrollEventChangesWorkspace)
{
	XfdashboardSettingsPrivate		*priv;

	g_return_if_fail(XFDASHBOARD_IS_SETTINGS(self));

	priv=self->priv;

	/* Set value if changed */
	if(priv->windowsViewScrollEventChangesWorkspace!=inWindowsViewScrollEventChangesWorkspace)
	{
		/* Set value */
		priv->windowsViewScrollEventChangesWorkspace=inWindowsViewScrollEventChangesWorkspace;

		/* Notify about property change */
		g_object_notify_by_pspec(G_OBJECT(self), XfdashboardSettingsProperties[PROP_WINDOWS_VIEW_SCROLL_EVENT_CHANGES_WORKSPACE]);
	}
}

/**
 * xfdashboard_settings_get_binding_files:
 * @self: A #XfdashboardSettings
 *
 * Retrieve the list of file location to load and merge key and mouse
 * binding from in settings at @self.
 *
 * Return value: A %NULL-terminated list of strings containing file
 *   locations or %NULL if no file locations were set at all.
 */
const gchar** xfdashboard_settings_get_binding_files(XfdashboardSettings *self)
{
	g_return_val_if_fail(XFDASHBOARD_IS_SETTINGS(self), NULL);

	return((const gchar**)self->priv->bindingFiles);
}

/**
 * xfdashboard_settings_get_theme_search_paths:
 * @self: A #XfdashboardSettings
 *
 * Retrieve the list of search paths where to lookup themes at from
 * settings at @self.
 *
 * Return value: A %NULL-terminated list of strings containing paths
 *   or %NULL if no search path was set at all.
 */
const gchar** xfdashboard_settings_get_theme_search_paths(XfdashboardSettings *self)
{
	g_return_val_if_fail(XFDASHBOARD_IS_SETTINGS(self), NULL);

	return((const gchar**)self->priv->themeSearchPaths);
}

/**
 * xfdashboard_settings_get_plugin_search_paths:
 * @self: A #XfdashboardSettings
 *
 * Retrieve the list of search paths where to lookup plugins at from
 * settings at @self.
 *
 * Return value: A %NULL-terminated list of strings containing paths
 *   or %NULL if no search path was set at all.
 */
const gchar** xfdashboard_settings_get_plugin_search_paths(XfdashboardSettings *self)
{
	g_return_val_if_fail(XFDASHBOARD_IS_SETTINGS(self), NULL);

	return((const gchar**)self->priv->pluginSearchPaths);
}

/**
 * xfdashboard_settings_get_config_path:
 * @self: A #XfdashboardSettings
 *
 * Retrieves the base path of configuration files of application in settings
 * at @self.
 *
 * NOTE: To retrieve the base path of configuration files for a specific plugin,
 * use xfdashboard_plugin_get_config_path() instead.
 *
 * Return value: The base path of configuration files or %NULL if support for
 *   configuration files is disabled.
 */
const gchar* xfdashboard_settings_get_config_path(XfdashboardSettings *self)
{
	g_return_val_if_fail(XFDASHBOARD_IS_SETTINGS(self), NULL);

	return(self->priv->configPath);
}

/**
 * xfdashboard_settings_get_data_path:
 * @self: A #XfdashboardSettings
 *
 * Retrieves the base path of data files of application in settings
 * at @self.
 *
 * NOTE: To retrieve the base path of data files for a specific plugin, use
 * xfdashboard_plugin_get_data_path() instead.
 *
 * Return value: The base path of data files or %NULL if support for
 *   data files is disabled.
 */
const gchar* xfdashboard_settings_get_data_path(XfdashboardSettings *self)
{
	g_return_val_if_fail(XFDASHBOARD_IS_SETTINGS(self), NULL);

	return(self->priv->dataPath);
}
