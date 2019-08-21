/*
 * plugin: Plugin functions for 'gnome-shell-search-provider'
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

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <libxfdashboard/libxfdashboard.h>
#include <libxfce4util/libxfce4util.h>

#include "gnome-shell-search-provider.h"


/* IMPLEMENTATION: Private variables and methods */
typedef struct _XfdashboardGnomeShellSearchProviderPluginPrivate	XfdashboardGnomeShellSearchProviderPluginPrivate;
struct _XfdashboardGnomeShellSearchProviderPluginPrivate
{
	/* Private structure */
	GList			*providers;
	GFileMonitor	*fileMonitor;
};


/* IMPLEMENTATION: XfdashboardPlugin */

/* Forward declarations */
G_MODULE_EXPORT void plugin_init(XfdashboardPlugin *self);

/* Get provider name (ID) from file */
static gchar* _xfdashboard_gnome_shell_search_provider_plugin_get_provider_name_from_file(GFile *inFile,
																							GError **outError)
{
	const gchar					*filename;
	gchar						*pluginID;
	gchar						*providerName;

	g_return_val_if_fail(G_IS_FILE(inFile), NULL);
	g_return_val_if_fail(outError==NULL || *outError==NULL, NULL);

	/* Get file name of Gnome-Shell search provider */
	filename=g_file_get_basename(inFile);
	if(!g_str_has_suffix(filename, ".ini"))
	{
		/* Set error */
		g_set_error_literal(outError,
							G_IO_ERROR,
							G_IO_ERROR_INVALID_FILENAME,
							_("Gnome-Shell search provider filename has wrong file extension."));

		return(NULL);
	}

	/* Build unique provider name for Gnome-Shell search provider
	 * using this plugin from file name without file extension.
	 */
	providerName=g_strndup(filename, strlen(filename)-4);
	pluginID=g_strdup_printf("%s.%s", PLUGIN_ID, providerName);
	g_free(providerName);

	/* Return provider name (ID) found */
	return(pluginID);
}

/* The directory containing Gnome-Shell search providers has changed */
static void _xfdashboard_gnome_shell_search_provider_plugin_on_file_monitor_changed(GFileMonitor *self,
																					GFile *inFile,
																					GFile *inOtherFile,
																					GFileMonitorEvent inEventType,
																					gpointer inUserData)
{
	XfdashboardGnomeShellSearchProviderPluginPrivate	*priv;
	XfdashboardSearchManager							*searchManager;
	gchar												*filePath;
	gchar												*providerName;
	GError												*error;
	gboolean											success;

	g_return_if_fail(G_IS_FILE_MONITOR(self));
	g_return_if_fail(inUserData);

	priv=(XfdashboardGnomeShellSearchProviderPluginPrivate*)inUserData;
	error=NULL;

	/* Get search manager where search providers were registered at */
	searchManager=xfdashboard_search_manager_get_default();

	/* Get file path */
	filePath=g_file_get_path(inFile);

	/* Check if a search provider was added */
	if(inEventType==G_FILE_MONITOR_EVENT_CREATED &&
		g_file_query_file_type(inFile, G_FILE_QUERY_INFO_NONE, NULL)==G_FILE_TYPE_REGULAR &&
		g_str_has_suffix(filePath, ".ini"))
	{
		providerName=_xfdashboard_gnome_shell_search_provider_plugin_get_provider_name_from_file(inFile, &error);
		if(providerName)
		{
			/* Register search provider */
			success=xfdashboard_search_manager_register(searchManager, providerName, XFDASHBOARD_TYPE_GNOME_SHELL_SEARCH_PROVIDER);
			if(success)
			{
				priv->providers=g_list_prepend(priv->providers, g_strdup(providerName));
				g_debug("Successfully registered Gnome-Shell search provider at file '%s' with ID '%s'", filePath, providerName);
			}
				else
				{
					g_debug("Failed to register Gnome-Shell search provider at file '%s' with ID '%s'", filePath, providerName);
				}
		}
			else
			{
				/* Show warning */
				g_warning(_("Could not register Gnome-Shell search provider at file '%s': %s"),
							filePath,
							(error && error->message) ? error->message : _("Unknown error"));
			}

		/* Release allocated resources */
		if(providerName) g_free(providerName);
	}

	/* Check if an existing search provider was removed */
	if(inEventType==G_FILE_MONITOR_EVENT_DELETED &&
		g_str_has_suffix(filePath, ".ini"))
	{
		providerName=_xfdashboard_gnome_shell_search_provider_plugin_get_provider_name_from_file(inFile, NULL);
		if(providerName &&
			xfdashboard_search_manager_has_registered_id(searchManager, providerName))
		{
			/* Unregister search provider */
			success=xfdashboard_search_manager_unregister(searchManager, providerName);
			if(success)
			{
				GList									*iter;
				gchar									*iterName;

				/* Iterate through list of registered providers by this plugin
				 * and remove any matching the provider name.
				 */
				for(iter=priv->providers; iter; iter=g_list_next(iter))
				{
					iterName=(gchar*)iter->data;
					if(g_strcmp0(iterName, providerName)==0)
					{
						/* Free list data at iterator */
						g_free(iterName);

						/* Remove and free list element iterator */
						priv->providers=g_list_delete_link(priv->providers, iter);
					}
				}

				g_debug("Successfully unregistered Gnome-Shell search provider at file '%s' with ID '%s'", filePath, providerName);
			}
				else
				{
					g_debug("Failed to unregister Gnome-Shell search provider at file '%s' with ID '%s'", filePath, providerName);
				}
		}

		/* Release allocated resources */
		if(providerName) g_free(providerName);
	}

	/* Release allocated resources */
	if(filePath) g_free(filePath);
	if(searchManager) g_object_unref(searchManager);
}

/* Plugin enable function */
static void plugin_enable(XfdashboardPlugin *self, gpointer inUserData)
{
	XfdashboardGnomeShellSearchProviderPluginPrivate	*priv;
	XfdashboardSearchManager							*searchManager;
	GFile												*gnomeShellSearchProvidersPath;
	GFileEnumerator										*enumerator;
	GFileInfo											*info;
	GError												*error;
	gchar												*pluginID;

	g_return_if_fail(inUserData);

	priv=(XfdashboardGnomeShellSearchProviderPluginPrivate*)inUserData;
	error=NULL;

	/* Get plugin's ID */
	g_object_get(G_OBJECT(self), "id", &pluginID, NULL);
	g_debug("Enabling plugin '%s'", pluginID);

	/* Get path where Gnome-Shell search providers are stored at */
	gnomeShellSearchProvidersPath=g_file_new_for_path(GNOME_SHELL_PROVIDERS_PATH);
	g_debug("Scanning directory '%s' for Gnome-Shell search providers",
				GNOME_SHELL_PROVIDERS_PATH);

	/* Get search manager where to register search providers at */
	searchManager=xfdashboard_search_manager_get_default();

	/* Create enumerator for search providers path to iterate through path and
	 * register search providers found.
	 */
	enumerator=g_file_enumerate_children(gnomeShellSearchProvidersPath,
											G_FILE_ATTRIBUTE_STANDARD_TYPE "," G_FILE_ATTRIBUTE_STANDARD_NAME,
											G_FILE_QUERY_INFO_NONE,
											NULL,
											&error);
	if(!enumerator)
	{
		/* Show error message */
		g_warning(_("Could not scan for gnome-shell search provider at '%s': %s"),
					GNOME_SHELL_PROVIDERS_PATH,
					(error && error->message) ? error->message : _("Unknown error"));

		/* Release allocated resources */
		if(error) g_error_free(error);
		if(pluginID) g_free(pluginID);
		if(searchManager) g_object_unref(searchManager);
		if(gnomeShellSearchProvidersPath) g_object_unref(gnomeShellSearchProvidersPath);

		return;
	}


	/* Iterate through files in search providers path and for each
	 * ".ini" file found register a search provider
	 */
	while((info=g_file_enumerator_next_file(enumerator, NULL, &error)))
	{
		/* If current iterated file ends with ".ini" then assume it is a Gnome-Shell
		 * search provider and register it.
		 */
		if(g_file_info_get_file_type(info)==G_FILE_TYPE_REGULAR &&
			g_str_has_suffix(g_file_info_get_name(info), ".ini"))
		{
			const gchar			*infoFilename;
			GFile				*infoFile;
			GError				*infoError;
			gchar				*providerName;
			gboolean			success;

			infoError=NULL;

			/* Get file for current one from enumerator */
			infoFilename=g_file_info_get_name(info);
			infoFile=g_file_get_child(g_file_enumerator_get_container(enumerator), infoFilename);

			/* Get provider name for file and register Gnome-Shell search provider */
			providerName=_xfdashboard_gnome_shell_search_provider_plugin_get_provider_name_from_file(infoFile, &infoError);
			if(providerName)
			{
				/* Register search provider */
				success=xfdashboard_search_manager_register(searchManager, providerName, XFDASHBOARD_TYPE_GNOME_SHELL_SEARCH_PROVIDER);
				if(success)
				{
					priv->providers=g_list_prepend(priv->providers, g_strdup(providerName));
					g_debug("Successfully registered Gnome-Shell search provider at file '%s' with ID '%s'", infoFilename, providerName);
				}
					else
					{
						g_debug("Failed to register Gnome-Shell search provider at file '%s' with ID '%s'", infoFilename, providerName);
					}
			}
				else
				{
					/* Show warning */
					g_warning(_("Could not register Gnome-Shell search provider at file '%s': %s"),
								infoFilename,
								(infoError && infoError->message) ? infoError->message : _("Unknown error"));
				}

			/* Release allocated resources */
			if(infoError) g_error_free(infoError);
			if(infoFile) g_object_unref(infoFile);
			if(providerName) g_free(providerName);
		}

		/* Release allocated resources */
		g_object_unref(info);
	}

	if(error)
	{
		/* Show error message */
		g_warning(_("Could not scan for gnome-shell search provider at '%s': %s"),
					GNOME_SHELL_PROVIDERS_PATH,
					(error && error->message) ? error->message : _("Unknown error"));

		/* Release allocated resources */
		if(error) g_error_free(error);
		if(pluginID) g_free(pluginID);
		if(enumerator) g_object_unref(enumerator);
		if(searchManager) g_object_unref(searchManager);
		if(gnomeShellSearchProvidersPath) g_object_unref(gnomeShellSearchProvidersPath);

		return;
	}

	/* Create monitor to get notified about new, changed and removed search providers */
	priv->fileMonitor=g_file_monitor_directory(gnomeShellSearchProvidersPath, G_FILE_MONITOR_NONE, NULL, &error);
	if(priv->fileMonitor)
	{
		g_debug("Created file monitor to watch for changed Gnome-Shell search providers at %s",
					GNOME_SHELL_PROVIDERS_PATH);

		g_signal_connect(priv->fileMonitor,
							"changed",
							G_CALLBACK(_xfdashboard_gnome_shell_search_provider_plugin_on_file_monitor_changed),
							priv);
	}
		else
		{
			/* Just print a warning but do not fail. We will just not get notified
			 * about changes at path where the providers are stored.
			 */
			g_warning(_("Unable to create file monitor for Gnome-Shell search providers at '%s': %s"),
						GNOME_SHELL_PROVIDERS_PATH,
						error ? error->message : _("Unknown error"));

			/* Release allocated resources */
			if(error)
			{
				g_error_free(error);
				error=NULL;
			}
		}

	/* Release allocated resources */
	g_debug("Enabled plugin '%s' with %d search providers",
				pluginID,
				g_list_length(priv->providers));

	if(pluginID) g_free(pluginID);
	if(enumerator) g_object_unref(enumerator);
	if(searchManager) g_object_unref(searchManager);
	if(gnomeShellSearchProvidersPath) g_object_unref(gnomeShellSearchProvidersPath);
}

/* Plugin disable function */
static void plugin_disable(XfdashboardPlugin *self, gpointer inUserData)
{
	XfdashboardGnomeShellSearchProviderPluginPrivate	*priv;
	XfdashboardSearchManager							*searchManager;
	GList												*iter;
	const gchar											*providerName;
	gboolean											success;
	gchar												*pluginID;

	g_return_if_fail(inUserData);

	priv=(XfdashboardGnomeShellSearchProviderPluginPrivate*)inUserData;

	/* Get plugin's ID */
	g_object_get(G_OBJECT(self), "id", &pluginID, NULL);
	g_debug("Disabling plugin '%s' with %d search providers",
				pluginID,
				g_list_length(priv->providers));

	/* At first remove file monitor for path where search providers are stored */
	if(priv->fileMonitor)
	{
		g_object_unref(priv->fileMonitor);
		priv->fileMonitor=NULL;

		g_debug("Removed file monitor to watch for changed Gnome-Shell search providers at %s",
					GNOME_SHELL_PROVIDERS_PATH);
	}

	/* Get search manager where search providers were registered at */
	searchManager=xfdashboard_search_manager_get_default();

	/* Unregister registered search providers */
	for(iter=priv->providers; iter; iter=g_list_next(iter))
	{
		providerName=(const gchar*)iter->data;

		if(providerName)
		{
			success=xfdashboard_search_manager_unregister(searchManager, providerName);
			if(success)
			{
				g_debug("Successfully unregistered Gnome-Shell search provider with ID '%s'", providerName);
			}
				else
				{
					g_debug("Failed to unregister Gnome-Shell search provider with ID '%s'", providerName);
				}
		}
	}

	g_object_unref(searchManager);

	/* Release allocated resources */
	g_debug("Disabled plugin '%s'", pluginID);

	if(pluginID) g_free(pluginID);
	if(priv->providers)
	{
		g_list_free_full(priv->providers, g_free);
		priv->providers=NULL;
	}
}

/* Plugin initialization function */
G_MODULE_EXPORT void plugin_init(XfdashboardPlugin *self)
{
	static XfdashboardGnomeShellSearchProviderPluginPrivate		priv={ 0, };

	/* Set up localization */
	xfce_textdomain(GETTEXT_PACKAGE, PACKAGE_LOCALE_DIR, "UTF-8");

	/* Set plugin info */
	xfdashboard_plugin_set_info(self,
								"flags", XFDASHBOARD_PLUGIN_FLAG_EARLY_INITIALIZATION,
								"name", _("Gnome-Shell search provider"),
								"description", _("Uses Gnome-Shell search providers as source for searches"),
								"author", "Stephan Haller <nomad@froevel.de>",
								NULL);

	/* Register GObject types of this plugin */
	XFDASHBOARD_REGISTER_PLUGIN_TYPE(self, xfdashboard_gnome_shell_search_provider);

	/* Connect plugin action handlers */
	g_signal_connect(self, "enable", G_CALLBACK(plugin_enable), &priv);
	g_signal_connect(self, "disable", G_CALLBACK(plugin_disable), &priv);
}
