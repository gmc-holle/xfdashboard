/*
 * plugins: Plugin settings of application
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
#include "config.h"
#endif

#include "plugins.h"

#include <glib/gi18n-lib.h>
#include <xfconf/xfconf.h>

#include <libxfdashboard/plugin.h>

/* Define this class in GObject system */
G_DEFINE_TYPE(XfdashboardSettingsPlugins,
				xfdashboard_settings_plugins,
				G_TYPE_OBJECT)

/* Private structure - access only by public API if needed */
#define XFDASHBOARD_SETTINGS_PLUGINS_GET_PRIVATE(obj) \
	(G_TYPE_INSTANCE_GET_PRIVATE((obj), XFDASHBOARD_TYPE_SETTINGS_PLUGINS, XfdashboardSettingsPluginsPrivate))

struct _XfdashboardSettingsPluginsPrivate
{
	/* Properties related */
	GtkBuilder		*builder;

	/* Instance related */
	XfconfChannel	*xfconfChannel;

	GtkWidget		*widgetPlugins;
	GtkWidget		*widgetPluginNameLabel;
	GtkWidget		*widgetPluginName;
	GtkWidget		*widgetPluginAuthorsLabel;
	GtkWidget		*widgetPluginAuthors;
	GtkWidget		*widgetPluginCopyrightLabel;
	GtkWidget		*widgetPluginCopyright;
	GtkWidget		*widgetPluginLicenseLabel;
	GtkWidget		*widgetPluginLicense;
	GtkWidget		*widgetPluginDescriptionLabel;
	GtkWidget		*widgetPluginDescription;
};

/* Properties */
enum
{
	PROP_0,

	PROP_BUILDER,

	PROP_LAST
};

static GParamSpec* XfdashboardSettingsPluginsProperties[PROP_LAST]={ 0, };


/* IMPLEMENTATION: Private variables and methods */
#define XFDASHBOARD_XFCONF_CHANNEL					"xfdashboard"

#define ENABLED_PLUGINS_XFCONF_PROP					"/enabled-plugins"
#define DEFAULT_ENABLED_PLUGINS						NULL

enum
{
	XFDASHBOARD_SETTINGS_PLUGINS_COLUMN_ID,

	XFDASHBOARD_SETTINGS_PLUGINS_COLUMN_FILE,
	XFDASHBOARD_SETTINGS_PLUGINS_COLUMN_NAME,
	XFDASHBOARD_SETTINGS_PLUGINS_COLUMN_AUTHORS,
	XFDASHBOARD_SETTINGS_PLUGINS_COLUMN_COPYRIGHT,
	XFDASHBOARD_SETTINGS_PLUGINS_COLUMN_LICENSE,
	XFDASHBOARD_SETTINGS_PLUGINS_COLUMN_DESCRIPTION,

	XFDASHBOARD_SETTINGS_PLUGINS_COLUMN_IS_VALID,
	XFDASHBOARD_SETTINGS_PLUGINS_COLUMN_IS_INVALID,
	XFDASHBOARD_SETTINGS_PLUGINS_COLUMN_IS_ENABLED,
	XFDASHBOARD_SETTINGS_PLUGINS_COLUMN_IS_CONFIGURABLE,

	XFDASHBOARD_SETTINGS_PLUGINS_COLUMN_PLUGIN,

	XFDASHBOARD_SETTINGS_PLUGINS_COLUMN_LAST
};

/* Selection in plugins tree view changed */
static void _xfdashboard_settings_plugins_enabled_plugins_on_plugins_selection_changed(XfdashboardSettingsPlugins *self,
																						GtkTreeSelection *inSelection)
{
	XfdashboardSettingsPluginsPrivate		*priv;
	GtkTreeModel							*model;
	GtkTreeIter								iter;
	gchar									*pluginName;
	gchar									*pluginDescription;
	gchar									*pluginAuthors;
	gchar									*pluginCopyright;
	gchar									*pluginLicense;

	g_return_if_fail(XFDASHBOARD_IS_SETTINGS_PLUGINS(self));
	g_return_if_fail(GTK_IS_TREE_SELECTION(inSelection));

	priv=self->priv;
	pluginName=NULL;
	pluginDescription=NULL;
	pluginAuthors=NULL;
	pluginCopyright=NULL;
	pluginLicense=NULL;

	/* Get selected entry from widget */
	if(gtk_tree_selection_get_selected(inSelection, &model, &iter))
	{
		/* Get data from model */
		gtk_tree_model_get(model,
							&iter,
							XFDASHBOARD_SETTINGS_PLUGINS_COLUMN_NAME, &pluginName,
							XFDASHBOARD_SETTINGS_PLUGINS_COLUMN_DESCRIPTION, &pluginDescription,
							XFDASHBOARD_SETTINGS_PLUGINS_COLUMN_AUTHORS, &pluginAuthors,
							XFDASHBOARD_SETTINGS_PLUGINS_COLUMN_COPYRIGHT, &pluginCopyright,
							XFDASHBOARD_SETTINGS_PLUGINS_COLUMN_LICENSE, &pluginLicense,
							-1);
	}

	/* Set text in labels */
	if(pluginName)
	{
		gtk_label_set_text(GTK_LABEL(priv->widgetPluginName), pluginName);
		gtk_widget_show(priv->widgetPluginName);
		gtk_widget_show(priv->widgetPluginNameLabel);
	}
		else
		{
			gtk_widget_hide(priv->widgetPluginName);
			gtk_widget_hide(priv->widgetPluginNameLabel);
		}

	if(pluginDescription)
	{
		gtk_label_set_markup(GTK_LABEL(priv->widgetPluginDescription), pluginDescription);
		gtk_widget_show(priv->widgetPluginDescription);
		gtk_widget_show(priv->widgetPluginDescriptionLabel);
	}
		else
		{
			gtk_widget_hide(priv->widgetPluginDescription);
			gtk_widget_hide(priv->widgetPluginDescriptionLabel);
		}

	if(pluginAuthors)
	{
		gtk_label_set_text(GTK_LABEL(priv->widgetPluginAuthors), pluginAuthors);
		gtk_widget_show(priv->widgetPluginAuthors);
		gtk_widget_show(priv->widgetPluginAuthorsLabel);
	}
		else
		{
			gtk_widget_hide(priv->widgetPluginAuthors);
			gtk_widget_hide(priv->widgetPluginAuthorsLabel);
		}

	if(pluginCopyright)
	{
		gtk_label_set_text(GTK_LABEL(priv->widgetPluginCopyright), pluginCopyright);
		gtk_widget_show(priv->widgetPluginCopyright);
		gtk_widget_show(priv->widgetPluginCopyrightLabel);
	}
		else
		{
			gtk_widget_hide(priv->widgetPluginCopyright);
			gtk_widget_hide(priv->widgetPluginCopyrightLabel);
		}

	if(pluginLicense)
	{
		gtk_label_set_text(GTK_LABEL(priv->widgetPluginLicense), pluginLicense);
		gtk_widget_show(priv->widgetPluginLicense);
		gtk_widget_show(priv->widgetPluginLicenseLabel);
	}
		else
		{
			gtk_widget_hide(priv->widgetPluginLicense);
			gtk_widget_hide(priv->widgetPluginLicenseLabel);
		}

	/* Release allocated resources */
	if(pluginName) g_free(pluginName);
	if(pluginDescription) g_free(pluginDescription);
	if(pluginAuthors) g_free(pluginAuthors);
	if(pluginCopyright) g_free(pluginCopyright);
	if(pluginLicense) g_free(pluginLicense);
}

/* Setting '/enabled-plugins' changed either at widget or at xfconf property */
static void _xfdashboard_settings_plugins_enabled_plugins_changed_by_widget(XfdashboardSettingsPlugins *self,
																			gchar *inPath,
																			gpointer inUserData)
{
	XfdashboardSettingsPluginsPrivate		*priv;
	GtkTreeModel							*model;
	GtkTreeIter								modelIter;
	GtkTreeIter								pluginsIter;
	gchar									*pluginID;
	gboolean								isEnabled;
	GPtrArray								*enabledPlugins;

	g_return_if_fail(XFDASHBOARD_IS_SETTINGS_PLUGINS(self));
	g_return_if_fail(inPath && *inPath);

	priv=self->priv;

	/* Get plugins' model */
	model=gtk_tree_view_get_model(GTK_TREE_VIEW(priv->widgetPlugins));

	/* Get iterator for path */
	if(G_UNLIKELY(!gtk_tree_model_get_iter_from_string(model, &modelIter, inPath)))
	{
		g_message("Could not get iterator for path %s", inPath);
		return;
	}

	if(G_UNLIKELY(!gtk_tree_model_get_iter_first(model, &pluginsIter)))
	{
		g_message("Could not get iterator for iterating enabled plugins");
		return;
	}

	/* Get current state before toggling */
	gtk_tree_model_get(model,
						&modelIter,
						XFDASHBOARD_SETTINGS_PLUGINS_COLUMN_IS_ENABLED, &isEnabled,
						-1);

	/* Set new enabled or disabled state of plugin in tree model */
	gtk_list_store_set(GTK_LIST_STORE(model),
						&modelIter,
						XFDASHBOARD_SETTINGS_PLUGINS_COLUMN_IS_ENABLED, !isEnabled,
						-1);

	/* Get and store new list of enabled plugins in Xfconf */
	enabledPlugins=g_ptr_array_new_with_free_func(g_free);
	do
	{
		/* Check if plugin is enabled and get plugin ID */
		gtk_tree_model_get(model,
							&pluginsIter,
							XFDASHBOARD_SETTINGS_PLUGINS_COLUMN_ID, &pluginID,
							XFDASHBOARD_SETTINGS_PLUGINS_COLUMN_IS_ENABLED, &isEnabled,
							-1);

		/* If plugin is enabled add plugin's ID to array */
		if(isEnabled) g_ptr_array_add(enabledPlugins, g_strdup(pluginID));

		/* Release allocated resources */
		if(pluginID) g_free(pluginID);
	}
	while(gtk_tree_model_iter_next(model, &pluginsIter));

	if(enabledPlugins->len > 0)
	{
		gchar								**enabledPluginsList;
		guint								i;
		gboolean							success;

		/* Create string list from array of enabled plugins to store at Xfconf */
		enabledPluginsList=g_new0(gchar*, (enabledPlugins->len)+1);
		for(i=0; i<enabledPlugins->len; i++)
		{
			enabledPluginsList[i]=g_strdup(g_ptr_array_index(enabledPlugins, i));
		}

		success=xfconf_channel_set_string_list(priv->xfconfChannel,
												ENABLED_PLUGINS_XFCONF_PROP,
												(const gchar * const*)enabledPluginsList);
		if(!success) g_critical(_("Could not set list of enabled plugins!"));

		/* Release allocated resources */
		if(enabledPluginsList) g_strfreev(enabledPluginsList);
	}
		else
		{
			/* Array of enabled plugins is empty so reset property at Xfconf */
			xfconf_channel_reset_property(priv->xfconfChannel, ENABLED_PLUGINS_XFCONF_PROP, FALSE);
		}

	/* Release allocated resources */
	if(enabledPlugins) g_ptr_array_free(enabledPlugins, TRUE);
}

static void _xfdashboard_settings_plugins_enabled_plugins_changed_by_xfconf(XfdashboardSettingsPlugins *self,
																			const gchar *inProperty,
																			const GValue *inValue,
																			XfconfChannel *inChannel)
{
	XfdashboardSettingsPluginsPrivate		*priv;
	gchar									**newValues;
	GtkTreeModel							*model;
	GtkTreeIter								modelIter;

	g_return_if_fail(XFDASHBOARD_IS_SETTINGS_PLUGINS(self));
	g_return_if_fail(XFCONF_IS_CHANNEL(inChannel));

	priv=self->priv;

	/* Get new value to set at widget */
	newValues=xfconf_channel_get_string_list(priv->xfconfChannel, ENABLED_PLUGINS_XFCONF_PROP);

	/* Iterate through plugins' model and lookup each item to match against
	 * one value in new value and enable or disable it
	 */
	model=gtk_tree_view_get_model(GTK_TREE_VIEW(priv->widgetPlugins));
	if(!gtk_tree_model_get_iter_first(model, &modelIter)) return;

	do
	{
		gchar								*pluginID;

		/* Get plugin ID of item currently iterated */
		gtk_tree_model_get(model,
							&modelIter,
							XFDASHBOARD_SETTINGS_PLUGINS_COLUMN_ID, &pluginID,
							-1);

		/* Check if plugin ID of item is in list of enabled plugins */
		if(pluginID && newValues)
		{
			gboolean						isEnabled;
			gchar							**valuesIter;

			/* Check if plugin ID of item is in list of enabled plugins */
			isEnabled=FALSE;
			for(valuesIter=newValues; !isEnabled && *valuesIter; valuesIter++)
			{
				if(g_strcmp0(*valuesIter, pluginID)==0) isEnabled=TRUE;
			}

			/* If we found it set "check" flag in enabled column otherwise
			 * unset this flag.
			 */
			gtk_list_store_set(GTK_LIST_STORE(model), &modelIter,
								XFDASHBOARD_SETTINGS_PLUGINS_COLUMN_IS_ENABLED, isEnabled,
								-1);
		}

		/* Release allocated resources */
		if(pluginID) g_free(pluginID);
	}
	while(gtk_tree_model_iter_next(model, &modelIter));

	/* Release allocated resources */
	if(newValues) g_strfreev(newValues);
}

/* Sorting function for theme list's model */
static gint _xfdashboard_settings_plugins_sort_plugins_list_model(GtkTreeModel *inModel,
																	GtkTreeIter *inLeft,
																	GtkTreeIter *inRight)
{
	gchar	*leftName;
	gchar	*rightName;
	gint	result;

	/* Get plugin names from model */
	leftName=NULL;
	gtk_tree_model_get(inModel, inLeft,
						XFDASHBOARD_SETTINGS_PLUGINS_COLUMN_NAME, &leftName,
						-1);
	if(!leftName) leftName=g_strdup("");

	rightName=NULL;
	gtk_tree_model_get(inModel, inRight,
						XFDASHBOARD_SETTINGS_PLUGINS_COLUMN_NAME, &rightName,
						-1);
	if(!rightName) rightName=g_strdup("");

	result=g_utf8_collate(leftName, rightName);

	/* Release allocated resources */
	if(leftName) g_free(leftName);
	if(rightName) g_free(rightName);

	/* Return sorting result */
	return(result);
}

/* Populate list of available themes */
static void _xfdashboard_settings_plugins_populate_plugins_list(XfdashboardSettingsPlugins *self,
																GtkWidget *inWidget)
{
	GHashTable						*plugins;
	GtkListStore					*model;
	GList							*pluginsSearchPaths;
	GList							*pathIter;
	const gchar						*envPath;
	gchar							*path;
	GtkTreeIter						modelIter;

	g_return_if_fail(XFDASHBOARD_IS_SETTINGS_PLUGINS(self));
	g_return_if_fail(GTK_IS_TREE_VIEW(inWidget));

	pluginsSearchPaths=NULL;

	/* Create hash-table to keep track of duplicates (e.g. overrides by user) */
	plugins=g_hash_table_new_full(g_str_hash, g_str_equal, g_free, NULL);

	/* Get model of widget to fill */
	model=gtk_list_store_new(XFDASHBOARD_SETTINGS_PLUGINS_COLUMN_LAST,
								G_TYPE_STRING,  /* XFDASHBOARD_SETTINGS_PLUGINS_COLUMN_ID */
								G_TYPE_STRING,  /* XFDASHBOARD_SETTINGS_PLUGINS_COLUMN_FILE */
								G_TYPE_STRING,  /* XFDASHBOARD_SETTINGS_PLUGINS_COLUMN_NAME */
								G_TYPE_STRING,  /* XFDASHBOARD_SETTINGS_PLUGINS_COLUMN_AUTHORS */
								G_TYPE_STRING,  /* XFDASHBOARD_SETTINGS_PLUGINS_COLUMN_COPYRIGHT */
								G_TYPE_STRING,  /* XFDASHBOARD_SETTINGS_PLUGINS_COLUMN_LICENSE */
								G_TYPE_STRING,  /* XFDASHBOARD_SETTINGS_PLUGINS_COLUMN_DESCRIPTION */
								G_TYPE_BOOLEAN, /* XFDASHBOARD_SETTINGS_PLUGINS_COLUMN_IS_VALID */
								G_TYPE_BOOLEAN, /* XFDASHBOARD_SETTINGS_PLUGINS_COLUMN_IS_INVALID */
								G_TYPE_BOOLEAN, /* XFDASHBOARD_SETTINGS_PLUGINS_COLUMN_IS_ENABLED */
								G_TYPE_BOOLEAN, /* XFDASHBOARD_SETTINGS_PLUGINS_COLUMN_IS_CONFIGURABLE */
								XFDASHBOARD_TYPE_PLUGIN /* XFDASHBOARD_SETTINGS_PLUGINS_COLUMN_PLUGIN */
							);
	gtk_tree_sortable_set_sort_func(GTK_TREE_SORTABLE(model),
									XFDASHBOARD_SETTINGS_PLUGINS_COLUMN_NAME,
									(GtkTreeIterCompareFunc)_xfdashboard_settings_plugins_sort_plugins_list_model,
									NULL,
									NULL);
	gtk_tree_sortable_set_sort_column_id(GTK_TREE_SORTABLE(model),
											XFDASHBOARD_SETTINGS_PLUGINS_COLUMN_NAME,
											GTK_SORT_ASCENDING);

	/* Get search paths */
	envPath=g_getenv("XFDASHBOARD_PLUGINS_PATH");
	if(envPath) pluginsSearchPaths=g_list_append(pluginsSearchPaths, g_strdup(envPath));

	path=g_build_filename(g_get_user_data_dir(), "xfdashboard", "plugins", NULL);
	if(path) pluginsSearchPaths=g_list_append(pluginsSearchPaths, path);

	path=g_build_filename(PACKAGE_LIBDIR, "xfdashboard", "plugins", NULL);
	if(path) pluginsSearchPaths=g_list_append(pluginsSearchPaths, path);

	/* Iterate through all plugin at all plugin paths, load them and
	 * check if they are valid and can be configured.
	 */
	for(pathIter=pluginsSearchPaths; pathIter; pathIter=g_list_next(pathIter))
	{
		const gchar					*pluginCurrentPath;
		const gchar					*pluginCurrentFilename;
		GDir						*directory;

		/* Get plugin path to iterate through */
		pluginCurrentPath=(const gchar*)pathIter->data;

		/* Open handle to directory to iterate through
		 * but skip NULL paths or directory objects
		 */
		directory=g_dir_open(pluginCurrentPath, 0, NULL);
		if(G_UNLIKELY(directory==NULL)) continue;

		/* Iterate through directory and find available themes */
		while((pluginCurrentFilename=g_dir_read_name(directory)))
		{
			gchar					*fullPath;
			XfdashboardPlugin		*plugin;
			gchar					*pluginID;
			gchar					*pluginName;
			gchar					*pluginDescription;
			gchar					*pluginAuthors;
			gchar					*pluginCopyright;
			gchar					*pluginLicense;
			gboolean				pluginIsConfigurable;
			guint					signalID;
			gulong					handlerID;
			GError					*error;

			error=NULL;

			/* Check if file is a possible plugin by checking file extension */
			if(!g_str_has_suffix(pluginCurrentFilename, G_MODULE_SUFFIX)) continue;

			/* Get full path */
			fullPath=g_build_filename(pluginCurrentPath, pluginCurrentFilename, NULL);
			if(G_UNLIKELY(!fullPath)) continue;

			/* Load plugin */
			plugin=xfdashboard_plugin_new(fullPath, &error);
			if(!plugin)
			{
				gchar				*message;

				/* Show error message */
				g_warning(_("Could not load plugin '%s' from '%s': %s"),
							pluginName,
							fullPath,
							error ? error->message : _("Unknown error"));

				/* Create error message to store in list */
				message=g_strdup_printf(_("<b>Plugin could not be loaded.</b>\n\n%s"),
										error ? error->message : _("Unknown error"));

				/* Add to widget's list */
				gtk_list_store_append(GTK_LIST_STORE(model), &modelIter);
				gtk_list_store_set(GTK_LIST_STORE(model), &modelIter,
									XFDASHBOARD_SETTINGS_PLUGINS_COLUMN_ID, NULL,
									XFDASHBOARD_SETTINGS_PLUGINS_COLUMN_NAME, pluginCurrentFilename,
									XFDASHBOARD_SETTINGS_PLUGINS_COLUMN_DESCRIPTION, message,
									XFDASHBOARD_SETTINGS_PLUGINS_COLUMN_AUTHORS, NULL,
									XFDASHBOARD_SETTINGS_PLUGINS_COLUMN_COPYRIGHT, NULL,
									XFDASHBOARD_SETTINGS_PLUGINS_COLUMN_LICENSE, NULL,
									XFDASHBOARD_SETTINGS_PLUGINS_COLUMN_FILE, fullPath,
									XFDASHBOARD_SETTINGS_PLUGINS_COLUMN_IS_VALID, FALSE,
									XFDASHBOARD_SETTINGS_PLUGINS_COLUMN_IS_INVALID, TRUE,
									XFDASHBOARD_SETTINGS_PLUGINS_COLUMN_IS_ENABLED, FALSE,
									XFDASHBOARD_SETTINGS_PLUGINS_COLUMN_IS_CONFIGURABLE, FALSE,
									XFDASHBOARD_SETTINGS_PLUGINS_COLUMN_PLUGIN, NULL,
									-1);

				/* Release allocated resources */
				if(message) g_free(message);
				if(fullPath) g_free(fullPath);
				if(error)
				{
					g_error_free(error);
					error=NULL;
				}

				/* Continue with next enabled plugin in list */
				continue;
			}

			/* Get plugin ID to check for duplicates */
			g_object_get(plugin,
							"id", &pluginID,
							NULL);
			if(g_hash_table_lookup(plugins, pluginID))
			{
				g_debug("Invalid plugin '%s': Duplicate plugin at %s",
							pluginID,
							fullPath);

				/* Release allocated resources */
				if(pluginID) g_free(pluginID);
				if(fullPath) g_free(fullPath);

				/* Continue with next entry */
				continue;
			}

			/* Get plugin infos */
			g_object_get(plugin,
							"name", &pluginName,
							"description", &pluginDescription,
							"author", &pluginAuthors,
							"copyright", &pluginCopyright,
							"license", &pluginLicense,
							NULL);

			/* Determine if plugin is configurable */
			pluginIsConfigurable=FALSE;

			signalID=g_signal_lookup("enable", XFDASHBOARD_TYPE_PLUGIN);
			handlerID=g_signal_handler_find(plugin,
											G_SIGNAL_MATCH_ID,
											signalID,
											0,
											NULL,
											NULL,
											NULL);
			if(!handlerID)
			{
				pluginIsConfigurable=TRUE;
				g_message("Plugin '%s' is configurable", pluginID);
			}

			/* Add to widget's list */
			gtk_list_store_append(GTK_LIST_STORE(model), &modelIter);
			gtk_list_store_set(GTK_LIST_STORE(model), &modelIter,
								XFDASHBOARD_SETTINGS_PLUGINS_COLUMN_ID, pluginID,
								XFDASHBOARD_SETTINGS_PLUGINS_COLUMN_NAME, pluginName,
								XFDASHBOARD_SETTINGS_PLUGINS_COLUMN_DESCRIPTION, pluginDescription,
								XFDASHBOARD_SETTINGS_PLUGINS_COLUMN_AUTHORS, pluginAuthors,
								XFDASHBOARD_SETTINGS_PLUGINS_COLUMN_COPYRIGHT, pluginCopyright,
								XFDASHBOARD_SETTINGS_PLUGINS_COLUMN_LICENSE, pluginLicense,
								XFDASHBOARD_SETTINGS_PLUGINS_COLUMN_FILE, fullPath,
								XFDASHBOARD_SETTINGS_PLUGINS_COLUMN_IS_VALID, TRUE,
								XFDASHBOARD_SETTINGS_PLUGINS_COLUMN_IS_INVALID, FALSE,
								XFDASHBOARD_SETTINGS_PLUGINS_COLUMN_IS_ENABLED, FALSE,
								XFDASHBOARD_SETTINGS_PLUGINS_COLUMN_IS_CONFIGURABLE, pluginIsConfigurable,
								XFDASHBOARD_SETTINGS_PLUGINS_COLUMN_PLUGIN, plugin,
								-1);

			/* Remember theme to avoid duplicates (and allow overrides by user */
			g_hash_table_insert(plugins, g_strdup(pluginID), GINT_TO_POINTER(1));
			g_debug("Added plugin '%s' with ID %s from %s", pluginName, pluginID, fullPath);

			/* Release allocated resources */
			if(pluginLicense) g_free(pluginLicense);
			if(pluginCopyright) g_free(pluginCopyright);
			if(pluginAuthors) g_free(pluginAuthors);
			if(pluginDescription) g_free(pluginDescription);
			if(pluginName) g_free(pluginName);
			if(pluginID) g_free(pluginID);
			if(fullPath) g_free(fullPath);
		}

		/* Close handle to directory */
		g_dir_close(directory);
	}

	/* Set new list store at widget */
	gtk_tree_view_set_model(GTK_TREE_VIEW(inWidget), GTK_TREE_MODEL(model));

	/* Release allocated resources */
	if(model) g_object_unref(model);
	if(pluginsSearchPaths) g_list_free_full(pluginsSearchPaths, g_free);
}

/* Create and set up GtkBuilder */
static void _xfdashboard_settings_plugins_set_builder(XfdashboardSettingsPlugins *self,
														GtkBuilder *inBuilder)
{
	XfdashboardSettingsPluginsPrivate	*priv;

	g_return_if_fail(XFDASHBOARD_IS_SETTINGS_PLUGINS(self));
	g_return_if_fail(GTK_IS_BUILDER(inBuilder));

	priv=self->priv;

	/* Set builder object which must not be set yet */
	g_assert(!priv->builder);

	priv->builder=g_object_ref(inBuilder);

	/* Get widgets from builder */
	priv->widgetPlugins=GTK_WIDGET(gtk_builder_get_object(priv->builder, "plugins"));
	priv->widgetPluginNameLabel=GTK_WIDGET(gtk_builder_get_object(priv->builder, "plugin-name-label"));
	priv->widgetPluginName=GTK_WIDGET(gtk_builder_get_object(priv->builder, "plugin-name"));
	priv->widgetPluginAuthorsLabel=GTK_WIDGET(gtk_builder_get_object(priv->builder, "plugin-authors-label"));
	priv->widgetPluginAuthors=GTK_WIDGET(gtk_builder_get_object(priv->builder, "plugin-authors"));
	priv->widgetPluginCopyrightLabel=GTK_WIDGET(gtk_builder_get_object(priv->builder, "plugin-copyright-label"));
	priv->widgetPluginCopyright=GTK_WIDGET(gtk_builder_get_object(priv->builder, "plugin-copyright"));
	priv->widgetPluginLicenseLabel=GTK_WIDGET(gtk_builder_get_object(priv->builder, "plugin-license-label"));
	priv->widgetPluginLicense=GTK_WIDGET(gtk_builder_get_object(priv->builder, "plugin-license"));
	priv->widgetPluginDescriptionLabel=GTK_WIDGET(gtk_builder_get_object(priv->builder, "plugin-description-label"));
	priv->widgetPluginDescription=GTK_WIDGET(gtk_builder_get_object(priv->builder, "plugin-description"));

	/* Set up theme list */
	if(priv->widgetPlugins)
	{
		GtkTreeSelection				*selection;
		GtkCellRenderer					*renderer;
		GIcon							*icon;

		/* Set up columns and columns' renderes at plugins widget */
		renderer=gtk_cell_renderer_pixbuf_new();
		icon=g_themed_icon_new_with_default_fallbacks("dialog-warning-symbolic");
		g_object_set(renderer,
						"gicon", icon,
						"stock-size", GTK_ICON_SIZE_MENU,
						NULL);
		if(icon) g_object_unref(icon);
		gtk_tree_view_insert_column_with_attributes(GTK_TREE_VIEW(priv->widgetPlugins),
													-1,
													N_(""),
													renderer,
													"visible", XFDASHBOARD_SETTINGS_PLUGINS_COLUMN_IS_INVALID,
													"sensitive", XFDASHBOARD_SETTINGS_PLUGINS_COLUMN_IS_VALID,
													NULL);

		renderer=gtk_cell_renderer_toggle_new();
		g_signal_connect_swapped(renderer,
									"toggled",
									G_CALLBACK(_xfdashboard_settings_plugins_enabled_plugins_changed_by_widget),
									self);
		gtk_tree_view_insert_column_with_attributes(GTK_TREE_VIEW(priv->widgetPlugins),
													-1,
													N_(""),
													renderer,
													"active", XFDASHBOARD_SETTINGS_PLUGINS_COLUMN_IS_ENABLED,
													"sensitive", XFDASHBOARD_SETTINGS_PLUGINS_COLUMN_IS_VALID,
													NULL);

		renderer=gtk_cell_renderer_pixbuf_new();
		icon=g_themed_icon_new_with_default_fallbacks("preferences-desktop-symbolic");
		g_object_set(renderer,
						"gicon", icon,
						"stock-size", GTK_ICON_SIZE_MENU,
						NULL);
		if(icon) g_object_unref(icon);
		gtk_tree_view_insert_column_with_attributes(GTK_TREE_VIEW(priv->widgetPlugins),
													-1,
													N_(""),
													renderer,
													"visible", XFDASHBOARD_SETTINGS_PLUGINS_COLUMN_IS_CONFIGURABLE,
													"sensitive", XFDASHBOARD_SETTINGS_PLUGINS_COLUMN_IS_VALID,
													NULL);

		renderer=gtk_cell_renderer_text_new();
		gtk_tree_view_insert_column_with_attributes(GTK_TREE_VIEW(priv->widgetPlugins),
													-1,
													_("name"),
													renderer,
													"text", XFDASHBOARD_SETTINGS_PLUGINS_COLUMN_NAME,
													"sensitive", XFDASHBOARD_SETTINGS_PLUGINS_COLUMN_IS_VALID,
													NULL);

		/* Ensure only one selection at time is possible */
		selection=gtk_tree_view_get_selection(GTK_TREE_VIEW(priv->widgetPlugins));
		gtk_tree_selection_set_mode(selection, GTK_SELECTION_SINGLE);

		/* Populate list of available themes */
		_xfdashboard_settings_plugins_populate_plugins_list(self, priv->widgetPlugins);

		/* Check enabled plugins */
		_xfdashboard_settings_plugins_enabled_plugins_changed_by_xfconf(self,
																		ENABLED_PLUGINS_XFCONF_PROP,
																		NULL,
																		priv->xfconfChannel);

		/* Connect signals */
		g_signal_connect_swapped(selection,
									"changed",
									G_CALLBACK(_xfdashboard_settings_plugins_enabled_plugins_on_plugins_selection_changed),
									self);

		g_signal_connect_swapped(priv->xfconfChannel,
									"property-changed::"ENABLED_PLUGINS_XFCONF_PROP,
									G_CALLBACK(_xfdashboard_settings_plugins_enabled_plugins_changed_by_xfconf),
									self);

		/* Release allocated resources */
	}
}

/* IMPLEMENTATION: GObject */

/* Dispose this object */
static void _xfdashboard_settings_plugins_dispose(GObject *inObject)
{
	XfdashboardSettingsPlugins			*self=XFDASHBOARD_SETTINGS_PLUGINS(inObject);
	XfdashboardSettingsPluginsPrivate	*priv=self->priv;

	/* Release allocated resouces */
	priv->widgetPlugins=NULL;
	priv->widgetPluginNameLabel=NULL;
	priv->widgetPluginName=NULL;
	priv->widgetPluginAuthorsLabel=NULL;
	priv->widgetPluginAuthors=NULL;
	priv->widgetPluginCopyrightLabel=NULL;
	priv->widgetPluginCopyright=NULL;
	priv->widgetPluginLicenseLabel=NULL;
	priv->widgetPluginLicense=NULL;
	priv->widgetPluginDescriptionLabel=NULL;
	priv->widgetPluginDescription=NULL;

	if(priv->builder)
	{
		g_object_unref(priv->builder);
		priv->builder=NULL;
	}

	if(priv->xfconfChannel)
	{
		priv->xfconfChannel=NULL;
	}

	/* Call parent's class dispose method */
	G_OBJECT_CLASS(xfdashboard_settings_plugins_parent_class)->dispose(inObject);
}

/* Set/get properties */
static void _xfdashboard_settings_plugins_set_property(GObject *inObject,
														guint inPropID,
														const GValue *inValue,
														GParamSpec *inSpec)
{
	XfdashboardSettingsPlugins				*self=XFDASHBOARD_SETTINGS_PLUGINS(inObject);

	switch(inPropID)
	{
		case PROP_BUILDER:
			_xfdashboard_settings_plugins_set_builder(self, GTK_BUILDER(g_value_get_object(inValue)));
			break;

		default:
			G_OBJECT_WARN_INVALID_PROPERTY_ID(inObject, inPropID, inSpec);
			break;
	}
}

static void _xfdashboard_settings_plugins_get_property(GObject *inObject,
														guint inPropID,
														GValue *outValue,
														GParamSpec *inSpec)
{
	XfdashboardSettingsPlugins				*self=XFDASHBOARD_SETTINGS_PLUGINS(inObject);
	XfdashboardSettingsPluginsPrivate		*priv=self->priv;

	switch(inPropID)
	{
		case PROP_BUILDER:
			g_value_set_object(outValue, priv->builder);
			break;

		default:
			G_OBJECT_WARN_INVALID_PROPERTY_ID(inObject, inPropID, inSpec);
			break;
	}
}

/* Class initialization
 * Override functions in parent classes and define properties
 * and signals
 */
static void xfdashboard_settings_plugins_class_init(XfdashboardSettingsPluginsClass *klass)
{
	GObjectClass		*gobjectClass=G_OBJECT_CLASS(klass);

	/* Override functions */
	gobjectClass->dispose=_xfdashboard_settings_plugins_dispose;
	gobjectClass->set_property=_xfdashboard_settings_plugins_set_property;
	gobjectClass->get_property=_xfdashboard_settings_plugins_get_property;

	/* Set up private structure */
	g_type_class_add_private(klass, sizeof(XfdashboardSettingsPluginsPrivate));

	/* Define properties */
	XfdashboardSettingsPluginsProperties[PROP_BUILDER]=
		g_param_spec_object("builder",
								_("Builder"),
								_("The initialized GtkBuilder object where to set up themes tab from"),
								GTK_TYPE_BUILDER,
								G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS | G_PARAM_CONSTRUCT_ONLY);

	g_object_class_install_properties(gobjectClass, PROP_LAST, XfdashboardSettingsPluginsProperties);
}

/* Object initialization
 * Create private structure and set up default values
 */
static void xfdashboard_settings_plugins_init(XfdashboardSettingsPlugins *self)
{
	XfdashboardSettingsPluginsPrivate	*priv;

	priv=self->priv=XFDASHBOARD_SETTINGS_PLUGINS_GET_PRIVATE(self);

	/* Set default values */
	priv->builder=NULL;

	priv->xfconfChannel=xfconf_channel_get(XFDASHBOARD_XFCONF_CHANNEL);

	priv->widgetPlugins=NULL;
	priv->widgetPluginNameLabel=NULL;
	priv->widgetPluginName=NULL;
	priv->widgetPluginAuthorsLabel=NULL;
	priv->widgetPluginAuthors=NULL;
	priv->widgetPluginCopyrightLabel=NULL;
	priv->widgetPluginCopyright=NULL;
	priv->widgetPluginLicenseLabel=NULL;
	priv->widgetPluginLicense=NULL;
	priv->widgetPluginDescriptionLabel=NULL;
	priv->widgetPluginDescription=NULL;
}

/* IMPLEMENTATION: Public API */

/* Create instance of this class */
XfdashboardSettingsPlugins* xfdashboard_settings_plugins_new(GtkBuilder *inBuilder)
{
	GObject		*instance;

	g_return_val_if_fail(GTK_IS_BUILDER(inBuilder), NULL);

	/* Create instance */
	instance=g_object_new(XFDASHBOARD_TYPE_SETTINGS_PLUGINS,
							"builder", inBuilder,
							NULL);

	/* Return newly created instance */
	return(XFDASHBOARD_SETTINGS_PLUGINS(instance));
}
