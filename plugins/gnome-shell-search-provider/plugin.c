/*
 * plugin: Plugin functions for 'gnome-shell-search-provider'
 * 
 * Copyright 2012-2016 Stephan Haller <nomad@froevel.de>
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

#include "plugin.h"

#include <libxfce4util/libxfce4util.h>

#include "search-manager.h"
#include "gnome-shell-search-provider.h"


/* IMPLEMENTATION: Private variables and methods */
static GList		*xfdashboard_gnome_shell_search_provider_registered_providers=NULL;


/* IMPLEMENTATION: XfdashboardPlugin */

/* Forward declarations */
G_MODULE_EXPORT void plugin_init(XfdashboardPlugin *self);

/* Plugin enable function */
static gboolean plugin_enable(XfdashboardPlugin *self, gpointer inUserData)
{
	XfdashboardSearchManager	*searchManager;
	GFile						*gnomeShellSearchProvidersPath;
	GFileEnumerator				*enumerator;
	GFileInfo					*info;
	GError						*error;
	gchar						*pluginID;

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

		return(XFDASHBOARD_PLUGIN_ACTION_HANDLED);
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
			const gchar			*filename;
			gchar				*gnomeShellSearchProviderName;
			gchar				*providerName;
			gboolean			success;

			filename=g_file_info_get_name(info);
			gnomeShellSearchProviderName=g_strndup(filename, strlen(filename)-4);
			if(gnomeShellSearchProviderName)
			{
				/* Build unique provider name for Gnome-Shell search provider
				 * using this plugin.
				 */
				providerName=g_strdup_printf("%s%s", XFDASHBOARD_GNOME_SHELL_SEARCH_PROVIDER_PREFIX, gnomeShellSearchProviderName);
				g_debug("Register gnome shell search provider '%s' from file %s", providerName, filename);

				/* Register search provider */
				success=xfdashboard_search_manager_register(searchManager, providerName, XFDASHBOARD_TYPE_GNOME_SHELL_SEARCH_PROVIDER);
				if(success)
				{
					xfdashboard_gnome_shell_search_provider_registered_providers=g_list_prepend(xfdashboard_gnome_shell_search_provider_registered_providers, g_strdup(providerName));
					g_debug("Successfully registered Gnome-Shell search provider '%s' with ID '%s'", gnomeShellSearchProviderName, providerName);
				}
					else
					{
						g_debug("Failed to register Gnome-Shell search provider '%s' with ID '%s'", gnomeShellSearchProviderName, providerName);
					}

				g_free(providerName);
				g_free(gnomeShellSearchProviderName);
			}
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

		return(XFDASHBOARD_PLUGIN_ACTION_HANDLED);
	}

	/* Create monitor to get notified about new, changed and removed search providers */
	// TODO:

	/* Release allocated resources */
	g_debug("Enabled plugin '%s' with %d search providers",
				pluginID,
				g_list_length(xfdashboard_gnome_shell_search_provider_registered_providers));

	if(pluginID) g_free(pluginID);
	if(enumerator) g_object_unref(enumerator);
	if(searchManager) g_object_unref(searchManager);
	if(gnomeShellSearchProvidersPath) g_object_unref(gnomeShellSearchProvidersPath);

	/* All done */
	return(XFDASHBOARD_PLUGIN_ACTION_HANDLED);
}

/* Plugin disable function */
static gboolean plugin_disable(XfdashboardPlugin *self, gpointer inUserData)
{
	XfdashboardSearchManager	*searchManager;
	GList						*iter;
	const gchar					*providerName;
	gboolean					success;
	gchar						*pluginID;

	/* Get plugin's ID */
	g_object_get(G_OBJECT(self), "id", &pluginID, NULL);
	g_debug("Disabling plugin '%s' with %d search providers",
				pluginID,
				g_list_length(xfdashboard_gnome_shell_search_provider_registered_providers));

	/* Get search manager where search providers were registered at */
	searchManager=xfdashboard_search_manager_get_default();

	/* Unregister registered search providers */
	for(iter=xfdashboard_gnome_shell_search_provider_registered_providers; iter; iter=g_list_next(iter))
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
	if(xfdashboard_gnome_shell_search_provider_registered_providers)
	{
		g_list_free_full(xfdashboard_gnome_shell_search_provider_registered_providers, g_free);
		xfdashboard_gnome_shell_search_provider_registered_providers=NULL;
	}

	/* All done */
	return(XFDASHBOARD_PLUGIN_ACTION_HANDLED);
}

/* Plugin initialization function */
G_MODULE_EXPORT void plugin_init(XfdashboardPlugin *self)
{
	/* Set up localization */
	xfce_textdomain(GETTEXT_PACKAGE, PACKAGE_LOCALE_DIR, "UTF-8");

	/* Set plugin info */
	xfdashboard_plugin_set_info(self,
								"id", "gnome-shell-search-provider",
								"name", _("Gnome-Shell search provider"),
								"description", _("Uses Gnome-Shell search providers as source for searches"),
								"author", "Stephan Haller <nomad@froevel.de>",
								NULL);

	/* Register GObject types of this plugin */
	XFDASHBOARD_REGISTER_PLUGIN_TYPE(self, xfdashboard_gnome_shell_search_provider);

	/* Connect plugin action handlers */
	g_signal_connect(self, "enable", G_CALLBACK(plugin_enable), NULL);
	g_signal_connect(self, "disable", G_CALLBACK(plugin_disable), NULL);
}
