/*
 * plugins: Plugin settings of application
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
#include "config.h"
#endif

#include "plugins.h"

#include <libxfdashboard/plugin.h>
#include <glib/gi18n-lib.h>
#include <xfconf/xfconf.h>


/* Define this class in GObject system */
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
	GtkWidget		*widgetPluginConfigureButton;
	GtkWidget		*widgetPluginPreferencesDialog;
	GtkWidget		*widgetPluginPreferencesWidgetBox;
	GtkWidget		*widgetPluginPreferencesDialogTitle;
};

G_DEFINE_TYPE_WITH_PRIVATE(XfdashboardSettingsPlugins,
							xfdashboard_settings_plugins,
							G_TYPE_OBJECT)

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

#define XFDASHBOARD_TREE_VIEW_COLUMN_ID				"column-id"

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

/* Close button in plugin's preferences dialog was clicked */
static void _xfdashboard_settings_plugins_on_perferences_dialog_close_clicked(XfdashboardSettingsPlugins *self,
																				GtkWidget *inWidget)
{
	GtkWidget		*dialog;

	g_return_if_fail(XFDASHBOARD_IS_SETTINGS_PLUGINS(self));
	g_return_if_fail(GTK_IS_WIDGET(inWidget));

	/* Get dialog this widget belongs to */
	dialog=inWidget;
	while(!GTK_IS_DIALOG(dialog)) dialog=gtk_widget_get_parent(dialog);

	/* Send response if dialog was found */
	if(dialog)
	{
		gtk_dialog_response(GTK_DIALOG(dialog), GTK_RESPONSE_CLOSE);
	}
}

/* Preferences icon in tree view was clicked */
static gboolean _xfdashboard_settings_plugins_call_preferences(XfdashboardSettingsPlugins *self,
																GdkEvent *inEvent,
																GtkTreeView *inTreeView,
																GtkTreePath *inPath)
{
	XfdashboardSettingsPluginsPrivate		*priv;
	GtkTreeModel							*model;
	GtkTreeIter								iter;
	gboolean								isConfigurable;
	XfdashboardPlugin						*plugin;
	GtkWidget								*pluginPreferencesWidget;

	g_return_val_if_fail(XFDASHBOARD_IS_SETTINGS_PLUGINS(self), FALSE);
	g_return_val_if_fail(GTK_IS_TREE_VIEW(inTreeView), FALSE);
	g_return_val_if_fail(inPath, FALSE);

	priv=self->priv;
	isConfigurable=FALSE;
	plugin=NULL;
	pluginPreferencesWidget=NULL;

	/* We only handle released single left-clicks in this event handler
	 * if an event was given.
	 */
	if(inEvent && inEvent->button.button!=1)
	{
		g_debug("Will not handle event: Got button %d in button event but expected 1.",
					inEvent->button.button);
		return(FALSE);
	}

	if(inEvent && inEvent->type!=GDK_BUTTON_RELEASE)
	{
		g_debug("Will not handle event: Got button event %d in button event but expected %d.",
					inEvent->type,
					GDK_BUTTON_RELEASE);
		return(FALSE);
	}

	/* If a preferences window is visible do nothing because only one
	 * window of this kind can be visible and set up at a time.
	 */
	if(gtk_widget_is_visible(priv->widgetPluginPreferencesDialog))
	{
		g_debug("Will not handle event: Only one preferences window can be visible at a time.");
		return(FALSE);
	}

	/* Get plugins' model */
	model=gtk_tree_view_get_model(GTK_TREE_VIEW(inTreeView));

	/* Get iterator for path */
	if(G_UNLIKELY(!gtk_tree_model_get_iter(model, &iter, inPath)))
	{
		g_debug("Could not get iterator for selection path");
		return(FALSE);
	}

	/* Check if it plugin is marked to be configurable */
	gtk_tree_model_get(model, &iter,
						XFDASHBOARD_SETTINGS_PLUGINS_COLUMN_IS_CONFIGURABLE, &isConfigurable,
						XFDASHBOARD_SETTINGS_PLUGINS_COLUMN_PLUGIN, &plugin,
						-1);

	if(!isConfigurable)
	{
		g_debug("Plugin '%s' is not configurable", xfdashboard_plugin_get_id(plugin));

		/* Release allocated resources */
		if(plugin) g_object_unref(plugin);

		return(FALSE);
	}

	/* Emit action "configure" at plugin */
	g_debug("Emitting signal 'configure' at plugin '%s' of type %s",
				xfdashboard_plugin_get_id(plugin),
				G_OBJECT_TYPE_NAME(plugin));

	g_signal_emit_by_name(plugin, "configure", &pluginPreferencesWidget);

	/* If plugin returned a GTK+ widget then create window and add the returned widget to it */
	if(pluginPreferencesWidget)
	{
		gint								response;
		gchar								*name;
		gchar								*title;
		GtkWidget							*toplevelWindow;

		/* Add returned widget from plugin to dialog */
		gtk_container_add(GTK_CONTAINER(priv->widgetPluginPreferencesWidgetBox), pluginPreferencesWidget);

		/* Set title of dialog */
		g_object_get(plugin, "name", &name, NULL);
		title=g_strdup_printf(_("Configure plugin: %s"), name);
		gtk_label_set_text(GTK_LABEL(priv->widgetPluginPreferencesDialogTitle), title);
		if(title) g_free(title);
		if(name) g_free(name);

		/* Set parent window */
		toplevelWindow=gtk_widget_get_toplevel(priv->widgetPlugins);
		if(gtk_widget_is_toplevel(toplevelWindow) &&
			GTK_IS_WINDOW(toplevelWindow))
		{
			gtk_window_set_transient_for(GTK_WINDOW(priv->widgetPluginPreferencesDialog), GTK_WINDOW(toplevelWindow));
		}

		/* Show dialog in modal mode but do not care about dialog's response code
		 * as "close" is the only action.
		 */
		response=gtk_dialog_run(GTK_DIALOG(priv->widgetPluginPreferencesDialog));
		g_debug("Plugins preferences dialog response for plugin '%s' is %d",
				xfdashboard_plugin_get_id(plugin),
				response);

		/* First hide dialog */
		gtk_widget_hide(priv->widgetPluginPreferencesDialog);

		/* Unset parent window */
		gtk_window_set_transient_for(GTK_WINDOW(priv->widgetPluginPreferencesDialog), NULL);

		/* Now destroy returned widget from plugin to get it removed from dialog */
		gtk_widget_destroy(pluginPreferencesWidget);
	}

	/* Release allocated resources */
	if(plugin) g_object_unref(plugin);

	/* If we get here the event was handled */
    return(TRUE);
}

/* A button was pressed in tree view */
static gboolean _xfdashboard_settings_plugins_on_treeview_button_pressed(XfdashboardSettingsPlugins *self,
																			GdkEvent *inEvent,
																			gpointer inUserData)
{
	GtkTreeView								*treeView;
	GtkTreePath								*path;
	GtkTreeViewColumn						*column;
	GtkTreeModel							*model;
	GtkTreeIter								iter;
	guint									columnID;
	gboolean								isValid;
	gboolean								eventHandled;

	g_return_val_if_fail(XFDASHBOARD_IS_SETTINGS_PLUGINS(self), FALSE);
	g_return_val_if_fail(inEvent, FALSE);
	g_return_val_if_fail(GTK_IS_TREE_VIEW(inUserData), FALSE);

	treeView=GTK_TREE_VIEW(inUserData);
	path=NULL;
	column=NULL;
	isValid=FALSE;
	eventHandled=FALSE;

	/* Get tree path where event happened */
	if(!gtk_tree_view_get_path_at_pos(treeView, inEvent->button.x, inEvent->button.y, &path, &column, NULL, NULL) ||
		!path ||
		!column)
	{
		g_debug("Could not get path and column in tree view for position %.2f, %.2f",
				inEvent->button.x,
				inEvent->button.y);

		/* Release allocated resources */
		if(path) gtk_tree_path_free(path);

		/* Event was not handled */
		return(FALSE);
	}

	/* Get plugins' model */
	model=gtk_tree_view_get_model(treeView);

	/* Get iterator for path */
	if(G_UNLIKELY(!gtk_tree_model_get_iter(model, &iter, path)))
	{
		g_debug("Could not get iterator for selection path");

		/* Release allocated resources */
		if(path) gtk_tree_path_free(path);

		/* Event was not handled */
		return(FALSE);
	}

	/* Get plugin at path and check if it is valid. If it is invalid we do not need to process
	 * this event any futher.
	 */
	gtk_tree_model_get(model, &iter,
						XFDASHBOARD_SETTINGS_PLUGINS_COLUMN_IS_VALID, &isValid,
						-1);

	if(!isValid)
	{
		g_debug("Will not process event for invalid plugin.");

		/* Release allocated resources */
		if(path) gtk_tree_path_free(path);

		/* Event was not handled */
		return(FALSE);
	}

	/* Check if column where event happened can be handled */
	columnID=GPOINTER_TO_INT(g_object_get_data(G_OBJECT(column), XFDASHBOARD_TREE_VIEW_COLUMN_ID));
	switch(columnID)
	{
		case XFDASHBOARD_SETTINGS_PLUGINS_COLUMN_IS_CONFIGURABLE:
			eventHandled=_xfdashboard_settings_plugins_call_preferences(self,
																		inEvent,
																		treeView,
																		path);
			break;

		default:
			g_debug("This event is not handled for column ID %d", columnID);
			break;
	}

	/* Release allocated resources */
	if(path) gtk_tree_path_free(path);

	/* Return result if event was handled */
    return(eventHandled);
}

/* The configure button for selected plugin was pressed */
static void _xfdashboard_settings_plugins_on_configure_button_pressed(XfdashboardSettingsPlugins *self,
																		GtkWidget *inButton)
{
	XfdashboardSettingsPluginsPrivate		*priv;
	GtkTreeSelection						*selection;
	GtkTreeModel							*model;
	GtkTreeIter								iter;

	g_return_if_fail(XFDASHBOARD_IS_SETTINGS_PLUGINS(self));
	g_return_if_fail(GTK_IS_BUTTON(inButton));

	priv=self->priv;

	/* Check if an entry is selected and call preferences dialog of selected plugin */
	selection=gtk_tree_view_get_selection(GTK_TREE_VIEW(priv->widgetPlugins));
	if(!selection) return;

	if(gtk_tree_selection_get_selected(selection, &model, &iter))
	{
		GtkTreePath							*path;

		/* Get path to selection */
		path=gtk_tree_model_get_path(model, &iter);

		/* Call preferences dialog for selected plugin */
		_xfdashboard_settings_plugins_call_preferences(self,
														NULL,
														GTK_TREE_VIEW(priv->widgetPlugins),
														path);

		/* Release allocated resources */
		if(path) gtk_tree_path_free(path);
	}
}

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
	gboolean								pluginCanConfigure;

	g_return_if_fail(XFDASHBOARD_IS_SETTINGS_PLUGINS(self));
	g_return_if_fail(GTK_IS_TREE_SELECTION(inSelection));

	priv=self->priv;
	pluginName=NULL;
	pluginDescription=NULL;
	pluginAuthors=NULL;
	pluginCopyright=NULL;
	pluginLicense=NULL;
	pluginCanConfigure=FALSE;

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
							XFDASHBOARD_SETTINGS_PLUGINS_COLUMN_IS_CONFIGURABLE, &pluginCanConfigure,
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

	if(pluginCanConfigure)
	{
		gtk_widget_show(priv->widgetPluginConfigureButton);
	}
		else
		{
			gtk_widget_hide(priv->widgetPluginConfigureButton);
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
		g_warning("Could not get iterator for path %s", inPath);
		return;
	}

	if(G_UNLIKELY(!gtk_tree_model_get_iter_first(model, &pluginsIter)))
	{
		g_warning("Could not get iterator for iterating enabled plugins");
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
																	GtkTreeIter *inRight,
																	gpointer inUserData)
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

			signalID=g_signal_lookup("configure", XFDASHBOARD_TYPE_PLUGIN);
			handlerID=g_signal_handler_find(plugin,
											G_SIGNAL_MATCH_ID,
											signalID,
											0,
											NULL,
											NULL,
											NULL);
			if(handlerID)
			{
				pluginIsConfigurable=TRUE;
				g_debug("Plugin '%s' is configurable", pluginID);
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
	priv->widgetPluginConfigureButton=GTK_WIDGET(gtk_builder_get_object(priv->builder, "plugin-configure-button"));

	priv->widgetPluginPreferencesDialog=GTK_WIDGET(gtk_builder_get_object(priv->builder, "plugin-preferences-dialog"));
	priv->widgetPluginPreferencesWidgetBox=GTK_WIDGET(gtk_builder_get_object(priv->builder, "plugin-preferences-widget-box"));
	priv->widgetPluginPreferencesDialogTitle=GTK_WIDGET(gtk_builder_get_object(priv->builder, "configure-plugin-title-label"));

	/* Set up theme list */
	if(priv->widgetPlugins)
	{
		GtkTreeViewColumn				*column;
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
		column=gtk_tree_view_column_new_with_attributes(N_(""),
														renderer,
														"visible", XFDASHBOARD_SETTINGS_PLUGINS_COLUMN_IS_INVALID,
														"sensitive", XFDASHBOARD_SETTINGS_PLUGINS_COLUMN_IS_VALID,
														NULL);
		g_object_set_data(G_OBJECT(column), XFDASHBOARD_TREE_VIEW_COLUMN_ID, GINT_TO_POINTER(XFDASHBOARD_SETTINGS_PLUGINS_COLUMN_IS_VALID));
		gtk_tree_view_insert_column(GTK_TREE_VIEW(priv->widgetPlugins), column, -1);

		renderer=gtk_cell_renderer_toggle_new();
		g_signal_connect_swapped(renderer,
									"toggled",
									G_CALLBACK(_xfdashboard_settings_plugins_enabled_plugins_changed_by_widget),
									self);
		column=gtk_tree_view_column_new_with_attributes(N_(""),
														renderer,
														"active", XFDASHBOARD_SETTINGS_PLUGINS_COLUMN_IS_ENABLED,
														"sensitive", XFDASHBOARD_SETTINGS_PLUGINS_COLUMN_IS_VALID,
														NULL);
		g_object_set_data(G_OBJECT(column), XFDASHBOARD_TREE_VIEW_COLUMN_ID, GINT_TO_POINTER(XFDASHBOARD_SETTINGS_PLUGINS_COLUMN_IS_ENABLED));
		gtk_tree_view_insert_column(GTK_TREE_VIEW(priv->widgetPlugins), column, -1);

		renderer=gtk_cell_renderer_pixbuf_new();
		icon=g_themed_icon_new_with_default_fallbacks("preferences-desktop-symbolic");
		g_object_set(renderer,
						"gicon", icon,
						"stock-size", GTK_ICON_SIZE_MENU,
						NULL);
		if(icon) g_object_unref(icon);
		column=gtk_tree_view_column_new_with_attributes(N_(""),
														renderer,
														"visible", XFDASHBOARD_SETTINGS_PLUGINS_COLUMN_IS_CONFIGURABLE,
														"sensitive", XFDASHBOARD_SETTINGS_PLUGINS_COLUMN_IS_VALID,
														NULL);
		g_object_set_data(G_OBJECT(column), XFDASHBOARD_TREE_VIEW_COLUMN_ID, GINT_TO_POINTER(XFDASHBOARD_SETTINGS_PLUGINS_COLUMN_IS_CONFIGURABLE));
		gtk_tree_view_insert_column(GTK_TREE_VIEW(priv->widgetPlugins), column, -1);

		renderer=gtk_cell_renderer_text_new();
		column=gtk_tree_view_column_new_with_attributes(_("Name"),
														renderer,
														"text", XFDASHBOARD_SETTINGS_PLUGINS_COLUMN_NAME,
														"sensitive", XFDASHBOARD_SETTINGS_PLUGINS_COLUMN_IS_VALID,
														NULL);
		g_object_set_data(G_OBJECT(column), XFDASHBOARD_TREE_VIEW_COLUMN_ID, GINT_TO_POINTER(XFDASHBOARD_SETTINGS_PLUGINS_COLUMN_NAME));
		gtk_tree_view_insert_column(GTK_TREE_VIEW(priv->widgetPlugins), column, -1);

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

		g_signal_connect_swapped(priv->widgetPlugins,
									"button-press-event",
									G_CALLBACK(_xfdashboard_settings_plugins_on_treeview_button_pressed),
									self);
		g_signal_connect_swapped(priv->widgetPlugins,
									"button-release-event",
									G_CALLBACK(_xfdashboard_settings_plugins_on_treeview_button_pressed),
									self);

		g_signal_connect_swapped(priv->widgetPluginConfigureButton,
									"clicked",
									G_CALLBACK(_xfdashboard_settings_plugins_on_configure_button_pressed),
									self);
	}

	/* Set up dialog for plugin preferences */
	if(priv->widgetPluginPreferencesDialog)
	{
		GtkWidget						*widgetCloseButton;

		/* Connect signals */
		widgetCloseButton=GTK_WIDGET(gtk_builder_get_object(priv->builder, "plugin-preferences-dialog-close-button"));
		g_signal_connect_swapped(widgetCloseButton,
									"clicked",
									G_CALLBACK(_xfdashboard_settings_plugins_on_perferences_dialog_close_clicked),
									self);
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
	priv->widgetPluginConfigureButton=NULL;

	priv->widgetPluginPreferencesDialog=NULL;
	priv->widgetPluginPreferencesWidgetBox=NULL;
	priv->widgetPluginPreferencesDialogTitle=NULL;

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

	priv=self->priv=xfdashboard_settings_plugins_get_instance_private(self);

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
	priv->widgetPluginConfigureButton=NULL;

	priv->widgetPluginPreferencesDialog=NULL;
	priv->widgetPluginPreferencesWidgetBox=NULL;
	priv->widgetPluginPreferencesDialogTitle=NULL;
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
