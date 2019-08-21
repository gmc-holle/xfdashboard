/*
 * plugins-manager: Single-instance managing plugins
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

/**
 * SECTION:plugins-manager
 * @short_description: The plugin manager class
 * @include: xfdashboard/plugins-manager.h
 *
 * #XfdashboardPluginsManager is a single instance object. It is managing all
 * plugins by loading and enabling or disabling them.
 *
 * The plugin manager will look up each plugin at the following paths and order:
 *
 * - Path specified in evironment variable XFDASHBOARD_PLUGINS_PATH
 * - $XDG_DATA_HOME/xfdashboard/plugins
 * - (install prefix)/lib/xfdashboard/plugins
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <libxfdashboard/plugins-manager.h>

#include <glib/gi18n-lib.h>

#include <libxfdashboard/plugin.h>
#include <libxfdashboard/application.h>
#include <libxfdashboard/compat.h>
#include <libxfdashboard/debug.h>


/* Define this class in GObject system */
struct _XfdashboardPluginsManagerPrivate
{
	/* Instance related */
	gboolean				isInited;
	GList					*searchPaths;
	GList					*plugins;

	XfconfChannel			*xfconfChannel;

	XfdashboardApplication	*application;
	guint					applicationInitializedSignalID;
};

G_DEFINE_TYPE_WITH_PRIVATE(XfdashboardPluginsManager,
							xfdashboard_plugins_manager,
							G_TYPE_OBJECT)

/* IMPLEMENTATION: Private variables and methods */
#define ENABLED_PLUGINS_XFCONF_PROP			"/enabled-plugins"

/* Single instance of plugin manager */
static XfdashboardPluginsManager*			_xfdashboard_plugins_manager=NULL;

/* Add path to search path but avoid duplicates */
static gboolean _xfdashboard_plugins_manager_add_search_path(XfdashboardPluginsManager *self,
																const gchar *inPath)
{
	XfdashboardPluginsManagerPrivate	*priv;
	gchar								*normalizedPath;
	GList								*iter;
	gchar								*iterPath;

	g_return_val_if_fail(XFDASHBOARD_IS_PLUGINS_MANAGER(self), FALSE);
	g_return_val_if_fail(inPath && *inPath, FALSE);

	priv=self->priv;

	/* Normalize requested path to add to list of search paths that means
	 * that it should end with a directory seperator.
	 */
	if(!g_str_has_suffix(inPath, G_DIR_SEPARATOR_S))
	{
		normalizedPath=g_strjoin(NULL, inPath, G_DIR_SEPARATOR_S, NULL);
	}
		else normalizedPath=g_strdup(inPath);

	/* Check if path is already in list of search paths */
	for(iter=priv->searchPaths; iter; iter=g_list_next(iter))
	{
		/* Get search path at iterator */
		iterPath=(gchar*)iter->data;

		/* If the path at iterator matches the requested one that it is
		 * already in list of search path, so return here with fail result.
		 */
		if(g_strcmp0(iterPath, normalizedPath)==0)
		{
			XFDASHBOARD_DEBUG(self, PLUGINS,
								"Path '%s' was already added to search paths of plugin manager",
								normalizedPath);

			/* Release allocated resources */
			if(normalizedPath) g_free(normalizedPath);

			/* Return fail result */
			return(FALSE);
		}
	}

	/* If we get here the requested path is not in list of search path and
	 * we can add it now.
	 */
	priv->searchPaths=g_list_append(priv->searchPaths, g_strdup(normalizedPath));
	XFDASHBOARD_DEBUG(self, PLUGINS,
						"Added path '%s' to search paths of plugin manager",
						normalizedPath);

	/* Release allocated resources */
	if(normalizedPath) g_free(normalizedPath);

	/* Return success result */
	return(TRUE);
}

/* Find path to plugin */
static gchar* _xfdashboard_plugins_manager_find_plugin_path(XfdashboardPluginsManager *self,
															const gchar *inPluginName)
{
	XfdashboardPluginsManagerPrivate	*priv;
	gchar								*path;
	GList								*iter;
	gchar								*iterPath;

	g_return_val_if_fail(XFDASHBOARD_IS_PLUGINS_MANAGER(self), NULL);
	g_return_val_if_fail(inPluginName && *inPluginName, NULL);

	priv=self->priv;

	/* Iterate through list of search paths, lookup file name containing plugin name
	 * followed by suffix ".so" and return first path found.
	 */
	for(iter=priv->searchPaths; iter; iter=g_list_next(iter))
	{
		/* Get search path at iterator */
		iterPath=(gchar*)iter->data;

		/* Create full path of search path and plugin name */
		path=g_strdup_printf("%s%s%s.%s", iterPath, G_DIR_SEPARATOR_S, inPluginName, G_MODULE_SUFFIX);
		if(!path) continue;

		/* Check if file exists and return it if we does */
		if(g_file_test(path, G_FILE_TEST_IS_REGULAR))
		{
			XFDASHBOARD_DEBUG(self, PLUGINS,
								"Found path %s for plugin '%s'",
								path,
								inPluginName);
			return(path);
		}

		/* Release allocated resources */
		if(path) g_free(path);
	}

	/* If we get here we did not found any suitable file, so return NULL */
	XFDASHBOARD_DEBUG(self, PLUGINS,
						"Plugin '%s' not found in search paths",
						inPluginName);
	return(NULL);
}

/* Find plugin with requested ID */
static XfdashboardPlugin* _xfdashboard_plugins_manager_find_plugin_by_id(XfdashboardPluginsManager *self,
																			const gchar *inPluginID)
{
	XfdashboardPluginsManagerPrivate	*priv;
	XfdashboardPlugin					*plugin;
	const gchar							*pluginID;
	GList								*iter;

	g_return_val_if_fail(XFDASHBOARD_IS_PLUGINS_MANAGER(self), NULL);
	g_return_val_if_fail(inPluginID && *inPluginID, NULL);

	priv=self->priv;

	/* Iterate through list of loaded plugins and return the plugin
	 * with requested ID.
	 */
	for(iter=priv->plugins; iter; iter=g_list_next(iter))
	{
		/* Get currently iterated plugin */
		plugin=XFDASHBOARD_PLUGIN(iter->data);

		/* Get plugin ID */
		pluginID=xfdashboard_plugin_get_id(plugin);

		/* Check if plugin has requested ID then return it */
		if(g_strcmp0(pluginID, inPluginID)==0) return(plugin);
	}

	/* If we get here the plugin with requested ID was not in the list of
	 * loaded plugins, so return NULL.
	 */
	return(NULL);
}

/* Checks if a plugin with requested ID exists */
static gboolean _xfdashboard_plugins_manager_has_plugin_id(XfdashboardPluginsManager *self,
															const gchar *inPluginID)
{
	XfdashboardPlugin					*plugin;

	g_return_val_if_fail(XFDASHBOARD_IS_PLUGINS_MANAGER(self), FALSE);
	g_return_val_if_fail(inPluginID && *inPluginID, FALSE);

	/* Get plugin by requested ID. If NULL is returned the plugin does not exist */
	plugin=_xfdashboard_plugins_manager_find_plugin_by_id(self, inPluginID);
	if(!plugin) return(FALSE);

	/* If we get here the plugin was found */
	return(TRUE);
}

/* Try to load plugin */
static gboolean _xfdashboard_plugins_manager_load_plugin(XfdashboardPluginsManager *self,
															const gchar *inPluginID,
															GError **outError)
{
	XfdashboardPluginsManagerPrivate	*priv;
	gchar								*path;
	XfdashboardPlugin					*plugin;
	GError								*error;

	g_return_val_if_fail(XFDASHBOARD_IS_PLUGINS_MANAGER(self), FALSE);
	g_return_val_if_fail(inPluginID && *inPluginID, FALSE);
	g_return_val_if_fail(outError==NULL || *outError==NULL, FALSE);

	priv=self->priv;
	error=NULL;

	/* Check if plugin with requested ID exists already in list of loaded plugins */
	if(_xfdashboard_plugins_manager_has_plugin_id(self, inPluginID))
	{
		XFDASHBOARD_DEBUG(self, PLUGINS,
							"Plugin ID '%s' already loaded.",
							inPluginID);

		/* The plugin is already loaded so return success result */
		return(TRUE);
	}

	/* Find path to plugin */
	path=_xfdashboard_plugins_manager_find_plugin_path(self, inPluginID);
	if(!path)
	{
		/* Set error */
		g_set_error(outError,
					XFDASHBOARD_PLUGIN_ERROR,
					XFDASHBOARD_PLUGIN_ERROR_ERROR,
					_("Could not find module for plugin ID '%s'"),
					inPluginID);

		/* Return error */
		return(FALSE);
	}

	/* Create and load plugin */
	plugin=xfdashboard_plugin_new(path, &error);
	if(!plugin)
	{
		/* Propagate error */
		g_propagate_error(outError, error);

		/* Return error */
		return(FALSE);
	}

	/* Enable plugin if early initialization is requested by plugin */
	if(xfdashboard_plugin_get_flags(plugin) & XFDASHBOARD_PLUGIN_FLAG_EARLY_INITIALIZATION)
	{
		XFDASHBOARD_DEBUG(self, PLUGINS,
							"Enabling plugin '%s' on load because early initialization was requested",
							inPluginID);
		xfdashboard_plugin_enable(plugin);
	}

	/* Store enabled plugin in list of enabled plugins */
	priv->plugins=g_list_prepend(priv->plugins, plugin);

	/* Plugin loaded so return success result */
	return(TRUE);
}

/* Property for list of enabled plugins in xfconf has changed */
static void _xfdashboard_plugins_manager_on_enabled_plugins_changed(XfdashboardPluginsManager *self,
																	const gchar *inProperty,
																	const GValue *inValue,
																	gpointer inUserData)
{
	XfdashboardPluginsManagerPrivate	*priv;
	gchar								**enabledPlugins;

	g_return_if_fail(XFDASHBOARD_IS_PLUGINS_MANAGER(self));
	g_return_if_fail(XFCONF_IS_CHANNEL(inUserData));

	priv=self->priv;

	/* If plugin managed is not inited do not load or "unload" any plugin */
	if(!priv->isInited) return;

	/* Get new list of enabled plugins */
	enabledPlugins=xfconf_channel_get_string_list(xfdashboard_application_get_xfconf_channel(NULL),
													ENABLED_PLUGINS_XFCONF_PROP);

	/* Iterate through list of loaded plugin and check if it also in new list
	 * of enabled plugins. If it is not then disable this plugin.
	 */
	if(priv->plugins)
	{
		GList								*iter;
		XfdashboardPlugin					*plugin;
		const gchar							*pluginID;

		iter=priv->plugins;
		while(iter)
		{
			GList							*nextIter;
			gchar							**listIter;
			gchar							*listIterPluginID;
			gboolean						found;

			/* Get next item getting iterated before doing anything
			 * because the current item may get removed and the iterator
			 * would get invalid.
			 */
			nextIter=g_list_next(iter);

			/* Get currently iterated plugin */
			plugin=XFDASHBOARD_PLUGIN(iter->data);

			/* Get plugin ID */
			pluginID=xfdashboard_plugin_get_id(plugin);

			/* Check if plugin ID is still in new list of enabled plugins */
			found=FALSE;
			for(listIter=enabledPlugins; !found && *listIter; listIter++)
			{
				/* Get plugin ID of new enabled plugins currently iterated */
				listIterPluginID=*listIter;

				/* Check if plugin ID matches this iterated one. If so,
				 * set flag that plugin was found.
				 */
				if(g_strcmp0(pluginID, listIterPluginID)==0) found=TRUE;
			}

			/* Check that found flag is set. If it is not then disable plugin */
			if(!found)
			{
				XFDASHBOARD_DEBUG(self, PLUGINS,
									"Disable plugin '%s'",
									pluginID);

				/* Disable plugin */
				xfdashboard_plugin_disable(plugin);
			}

			/* Move iterator to next item */
			iter=nextIter;
		}
	}

	/* Iterate through new list of enabled plugin and check if it is already
	 * or still in the list of loaded plugins. If it not in this list then
	 * try to load this plugin. If it is then enable it again when it is in
	 * disable state.
	 */
	if(enabledPlugins)
	{
		gchar								**iter;
		gchar								*pluginID;
		GError								*error;
		XfdashboardPlugin					*plugin;

		error=NULL;

		/* Iterate through new list of enabled plugins and load new plugins */
		for(iter=enabledPlugins; *iter; iter++)
		{
			/* Get plugin ID to check */
			pluginID=*iter;

			/* Check if a plugin with this ID exists already */
			plugin=_xfdashboard_plugins_manager_find_plugin_by_id(self, pluginID);
			if(!plugin)
			{
				/* The plugin does not exist so try to load it */
				if(!_xfdashboard_plugins_manager_load_plugin(self, pluginID, &error))
				{
					/* Show error message */
					g_warning(_("Could not load plugin '%s': %s"),
								pluginID,
								error ? error->message : _("Unknown error"));

					/* Release allocated resources */
					if(error)
					{
						g_error_free(error);
						error=NULL;
					}
				}
					else
					{
						XFDASHBOARD_DEBUG(self, PLUGINS,
											"Loaded plugin '%s'",
											pluginID);
					}
			}
				else
				{
					/* The plugin exists already so check if it is disabled and
					 * re-enable it.
					 */
					if(!xfdashboard_plugin_is_enabled(plugin))
					{
						XFDASHBOARD_DEBUG(self, PLUGINS,
											"Re-enable plugin '%s'",
											pluginID);
						xfdashboard_plugin_enable(plugin);
					}
				}
		}
	}

	/* Release allocated resources */
	if(enabledPlugins) g_strfreev(enabledPlugins);
}

/* Application was fully initialized so enabled all loaded plugin except the
 * ones which are already enabled, e.g. plugins which requested early initialization.
 */
static void _xfdashboard_plugins_manager_on_application_initialized(XfdashboardPluginsManager *self,
																	gpointer inUserData)
{
	XfdashboardPluginsManagerPrivate	*priv;
	GList								*iter;
	XfdashboardPlugin					*plugin;

	g_return_if_fail(XFDASHBOARD_IS_PLUGINS_MANAGER(self));
	g_return_if_fail(XFDASHBOARD_IS_APPLICATION(inUserData));

	priv=self->priv;

	/* Iterate through all loaded plugins and enable all plugins which are
	 * not enabled yet.
	 */
	XFDASHBOARD_DEBUG(self, PLUGINS, "Plugin manager will now enable all remaining plugins because application is fully initialized now");
	for(iter=priv->plugins; iter; iter=g_list_next(iter))
	{
		/* Get plugin */
		plugin=XFDASHBOARD_PLUGIN(iter->data);

		/* If plugin is not enabled do it now */
		if(!xfdashboard_plugin_is_enabled(plugin))
		{
			/* Enable plugin */
			XFDASHBOARD_DEBUG(self, PLUGINS,
								"Enabling plugin '%s'",
								xfdashboard_plugin_get_id(plugin));
			xfdashboard_plugin_enable(plugin);
		}
	}

	/* Disconnect signal handler as this signal is emitted only once */
	if(priv->application)
	{
		if(priv->applicationInitializedSignalID)
		{
			g_signal_handler_disconnect(priv->application, priv->applicationInitializedSignalID);
			priv->applicationInitializedSignalID=0;
		}

		priv->application=NULL;
	}
}

/* IMPLEMENTATION: GObject */

/* Construct this object */
static GObject* _xfdashboard_plugins_manager_constructor(GType inType,
															guint inNumberConstructParams,
															GObjectConstructParam *inConstructParams)
{
	GObject									*object;

	if(!_xfdashboard_plugins_manager)
	{
		object=G_OBJECT_CLASS(xfdashboard_plugins_manager_parent_class)->constructor(inType, inNumberConstructParams, inConstructParams);
		_xfdashboard_plugins_manager=XFDASHBOARD_PLUGINS_MANAGER(object);
	}
		else
		{
			object=g_object_ref(G_OBJECT(_xfdashboard_plugins_manager));
		}

	return(object);
}

/* Dispose this object */
static void _xfdashboard_plugins_manager_dispose_remove_plugin(gpointer inData)
{
	XfdashboardPlugin					*plugin;

	g_return_if_fail(inData && XFDASHBOARD_IS_PLUGIN(inData));

	plugin=XFDASHBOARD_PLUGIN(inData);

	/* Disable plugin */
	xfdashboard_plugin_disable(plugin);

	/* Unload plugin */
	g_type_module_unuse(G_TYPE_MODULE(plugin));
}

static void _xfdashboard_plugins_manager_dispose(GObject *inObject)
{
	XfdashboardPluginsManager			*self=XFDASHBOARD_PLUGINS_MANAGER(inObject);
	XfdashboardPluginsManagerPrivate	*priv=self->priv;

	/* Release allocated resources */
	if(priv->application)
	{
		if(priv->applicationInitializedSignalID)
		{
			g_signal_handler_disconnect(priv->application, priv->applicationInitializedSignalID);
			priv->applicationInitializedSignalID=0;
		}

		priv->application=NULL;
	}

	if(priv->plugins)
	{
		g_list_free_full(priv->plugins, (GDestroyNotify)_xfdashboard_plugins_manager_dispose_remove_plugin);
		priv->plugins=NULL;
	}

	if(priv->searchPaths)
	{
		g_list_free_full(priv->searchPaths, g_free);
		priv->searchPaths=NULL;
	}

	/* Call parent's class dispose method */
	G_OBJECT_CLASS(xfdashboard_plugins_manager_parent_class)->dispose(inObject);
}

/* Finalize this object */
static void _xfdashboard_plugins_manager_finalize(GObject *inObject)
{
	/* Release allocated resources finally, e.g. unset singleton */
	if(G_LIKELY(G_OBJECT(_xfdashboard_plugins_manager)==inObject))
	{
		_xfdashboard_plugins_manager=NULL;
	}

	/* Call parent's class dispose method */
	G_OBJECT_CLASS(xfdashboard_plugins_manager_parent_class)->finalize(inObject);
}

/* Class initialization
 * Override functions in parent classes and define properties
 * and signals
 */
static void xfdashboard_plugins_manager_class_init(XfdashboardPluginsManagerClass *klass)
{
	GObjectClass		*gobjectClass=G_OBJECT_CLASS(klass);

	/* Override functions */
	gobjectClass->constructor=_xfdashboard_plugins_manager_constructor;
	gobjectClass->dispose=_xfdashboard_plugins_manager_dispose;
	gobjectClass->finalize=_xfdashboard_plugins_manager_finalize;
}

/* Object initialization
 * Create private structure and set up default values
 */
static void xfdashboard_plugins_manager_init(XfdashboardPluginsManager *self)
{
	XfdashboardPluginsManagerPrivate		*priv;

	priv=self->priv=xfdashboard_plugins_manager_get_instance_private(self);

	/* Set default values */
	priv->isInited=FALSE;
	priv->searchPaths=NULL;
	priv->plugins=NULL;
	priv->xfconfChannel=xfdashboard_application_get_xfconf_channel(NULL);
	priv->application=xfdashboard_application_get_default();

	/* Connect signal to get notified about changed of enabled-plugins
	 * property in Xfconf.
	 */
	g_signal_connect_swapped(priv->xfconfChannel,
								"property-changed::"ENABLED_PLUGINS_XFCONF_PROP,
								G_CALLBACK(_xfdashboard_plugins_manager_on_enabled_plugins_changed),
								self);

	/* Connect signal to get notified when application is fully initialized
	 * to enable loaded plugins.
	 */
	priv->applicationInitializedSignalID=
		g_signal_connect_swapped(priv->application,
									"initialized",
									G_CALLBACK(_xfdashboard_plugins_manager_on_application_initialized),
									self);
}

/* IMPLEMENTATION: Public API */

/**
 * xfdashboard_plugins_manager_get_default:
 *
 * Retrieves the singleton instance of #XfdashboardPluginsManager.
 *
 * Return value: (transfer full): The instance of #XfdashboardPluginsManager.
 *   Use g_object_unref() when done.
 */
XfdashboardPluginsManager* xfdashboard_plugins_manager_get_default(void)
{
	GObject									*singleton;

	singleton=g_object_new(XFDASHBOARD_TYPE_PLUGINS_MANAGER, NULL);
	return(XFDASHBOARD_PLUGINS_MANAGER(singleton));
}

/**
 * xfdashboard_plugins_manager_setup:
 * @self: A #XfdashboardPluginsManager
 *
 * Initializes the plugin manager at @self by loading all enabled plugins. This
 * function can only be called once and is initialized by the application at
 * start-up. So you usually do not have to call this function or it does anything
 * as the plugin manager is already setup.
 *
 * The plugin manager will continue initializing successfully even if a plugin
 * could not be loaded. In this case just a warning is printed.
 *
 * Return value: Returns %TRUE if plugin manager was initialized successfully
 *   or was already initialized. Otherwise %FALSE will be returned.
 */
gboolean xfdashboard_plugins_manager_setup(XfdashboardPluginsManager *self)
{
	XfdashboardPluginsManagerPrivate	*priv;
	gchar								*path;
	const gchar							*envPath;
	gchar								**enabledPlugins;
	gchar								**iter;
	GError								*error;

	g_return_val_if_fail(XFDASHBOARD_IS_PLUGINS_MANAGER(self), FALSE);

	priv=self->priv;
	error=NULL;

	/* If plugin manager is already initialized then return immediately */
	if(priv->isInited) return(TRUE);

	/* Add search paths. Some paths may fail because they already exist
	 * in list of search paths. So this should not fail this function.
	 */
	envPath=g_getenv("XFDASHBOARD_PLUGINS_PATH");
	if(envPath)
	{
		_xfdashboard_plugins_manager_add_search_path(self, envPath);
	}

	path=g_build_filename(g_get_user_data_dir(), "xfdashboard", "plugins", NULL);
	_xfdashboard_plugins_manager_add_search_path(self, path);
	g_free(path);

	path=g_build_filename(PACKAGE_LIBDIR, "xfdashboard", "plugins", NULL);
	_xfdashboard_plugins_manager_add_search_path(self, path);
	g_free(path);

	/* Get list of enabled plugins and try to load them */
	enabledPlugins=xfconf_channel_get_string_list(xfdashboard_application_get_xfconf_channel(NULL),
													ENABLED_PLUGINS_XFCONF_PROP);

	/* Try to load all enabled plugin and collect each error occurred. */
	for(iter=enabledPlugins; iter && *iter; iter++)
	{
		gchar							*pluginID;

		/* Get plugin name */
		pluginID=*iter;
		XFDASHBOARD_DEBUG(self, PLUGINS,
							"Try to load plugin '%s'",
							pluginID);

		/* Try to load plugin */
		if(!_xfdashboard_plugins_manager_load_plugin(self, pluginID, &error))
		{
			/* Show error message */
			g_warning(_("Could not load plugin '%s': %s"),
						pluginID,
						error ? error->message : _("Unknown error"));

			/* Release allocated resources */
			if(error)
			{
				g_error_free(error);
				error=NULL;
			}
		}
			else
			{
				XFDASHBOARD_DEBUG(self, PLUGINS,
									"Loaded plugin '%s'",
									pluginID);
			}
	}

	/* If we get here then initialization was successful so set flag that
	 * this plugin manager is initialized and return with TRUE result.
	 */
	priv->isInited=TRUE;

	/* Release allocated resources */
	if(enabledPlugins) g_strfreev(enabledPlugins);

	return(TRUE);
}
