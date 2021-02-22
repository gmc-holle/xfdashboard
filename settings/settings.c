/*
 * settings: Settings of application
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

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "settings.h"

#include <common/xfconf-settings.h>
#include <glib/gi18n-lib.h>
#include <libxfce4ui/libxfce4ui.h>
#include <math.h>

#include "general.h"
#include "plugins.h"
#include "themes.h"


/* Define this class in GObject system */
struct _XfdashboardSettingsAppPrivate
{
	/* Instance related */
	XfdashboardSettings				*settings;

	GtkBuilder						*builder;
	GObject							*dialog;

	XfdashboardSettingsGeneral		*general;
	XfdashboardSettingsThemes		*themes;
	XfdashboardSettingsPlugins		*plugins;

	GtkWidget						*widgetHelpButton;
	GtkWidget						*widgetCloseButton;
};

G_DEFINE_TYPE_WITH_PRIVATE(XfdashboardSettingsApp,
							xfdashboard_settings_app,
							G_TYPE_OBJECT)

/* IMPLEMENTATION: Private variables and methods */
#define XFDASHBOARD_XFCONF_CHANNEL					"xfdashboard"

#define PREFERENCES_UI_FILE							"preferences.ui"


/* Help button was clicked */
static void _xfdashboard_settings_app_on_help_clicked(XfdashboardSettingsApp *self,
														GtkWidget *inWidget)
{
	XfdashboardSettingsAppPrivate			*priv;
	GtkWindow								*window;

	g_return_if_fail(XFDASHBOARD_IS_SETTINGS_APP(self));

	priv=self->priv;

	/* Show online manual for xfdashboard but ask user if needed */
	window=NULL;
	if(GTK_IS_WINDOW(priv->dialog)) window=GTK_WINDOW(priv->dialog);

	xfce_dialog_show_help_with_version(window,
										"xfdashboard",
										"start",
										NULL,
										NULL);
}

/* Close button was clicked */
static void _xfdashboard_settings_app_on_close_clicked(XfdashboardSettingsApp *self,
														GtkWidget *inWidget)
{
	g_return_if_fail(XFDASHBOARD_IS_SETTINGS_APP(self));

	/* Quit main loop */
	gtk_main_quit();
}

/* Set up search paths */
static GList* _xfdashboard_settings_app_path_file_list_add(GList *inSearchPaths, const gchar *inPath, gboolean isFile)
{
	gchar								*normalizedPath;
	GList								*iter;
	gchar								*iterPath;

	g_return_val_if_fail(inPath && *inPath, FALSE);

	/* Normalize requested path to add to list of search paths that means
	 * that it should end with a directory seperator.
	 */
	if(!isFile && !g_str_has_suffix(inPath, G_DIR_SEPARATOR_S))
	{
		normalizedPath=g_strjoin(NULL, inPath, G_DIR_SEPARATOR_S, NULL);
	}
		else normalizedPath=g_strdup(inPath);

	/* Check if path is already in list of search paths */
	for(iter=inSearchPaths; iter; iter=g_list_next(iter))
	{
		/* Get search path at iterator */
		iterPath=(gchar*)iter->data;

		/* If the path at iterator matches the requested one that it is
		 * already in list of search path, so return here with fail result.
		 */
		if(g_strcmp0(iterPath, normalizedPath)==0)
		{
			/* Release allocated resources */
			if(normalizedPath) g_free(normalizedPath);

			/* Return fail result by returning unmodified path list */
			return(inSearchPaths);
		}
	}

	/* If we get here the requested path is not in list of search path and
	 * we can add it now.
	 */
	iter=g_list_append(inSearchPaths, g_strdup(normalizedPath));
	inSearchPaths=iter;

	/* Release allocated resources */
	if(normalizedPath) g_free(normalizedPath);

	/* Return success result by returning modified path list */
	return(inSearchPaths);
}

/* Converts GList of paths of type string to NULL-terminated list, a strv.
 * It also frees the GList.
 */
static gchar** _xfdashboard_settings_app_path_file_list_list_to_strv(GList *inPaths)
{
	guint			pathCount;
	gchar			**strv;
	GList			*iter;
	gchar			*path;
	guint			i;

	/* Get size of list */
	pathCount=g_list_length(inPaths);

	/* Skip empty search path list */
	if(pathCount==0) return(NULL);

	/* Initialize empty NULL-terminated list of needed size */
	strv=g_new0(gchar*, pathCount+1);

	/* Iterate through list and move iterated path to NULL-terminated list */
	i=0;
	for(iter=inPaths; iter; iter=g_list_next(iter))
	{
		/* Get iterated path from list of search paths. Skip empty paths. */
		path=iter->data;
		if(!path) continue;

		/* Move iterated path to NULL-terminated list */
		strv[i]=path;
		i++;
	}

	/* Free search path list */
	g_list_free(inPaths);

	/* Return new NULL-terminated list */
	return(strv);
}

/* Create and set up settings object instance */
static gboolean _xfdashboard_settings_app_create_settings(XfdashboardSettingsApp *self)
{
	XfdashboardSettingsAppPrivate			*priv;
	GList									*pathFileList;
	const gchar								*environmentVariable;
	gchar									**themesSearchPaths;
	gchar									**pluginsSearchPaths;
	gchar									**bindingFilePaths;
	gchar									*entry;
	const gchar								*homeDirectory;

	g_return_val_if_fail(XFDASHBOARD_IS_SETTINGS_APP(self), FALSE);

	priv=self->priv;
	themesSearchPaths=NULL;
	pluginsSearchPaths=NULL;
	bindingFilePaths=NULL;

	/* If settings is already set up return immediately */
	if(priv->settings) return(TRUE);

	/* Set up search path for themes */
	pathFileList=NULL;

	environmentVariable=g_getenv("XFDASHBOARD_THEME_PATH");
	if(environmentVariable)
	{
		gchar						**paths;
		gchar						**pathIter;

		pathIter=paths=g_strsplit(environmentVariable, ":", -1);
		while(*pathIter)
		{
			pathFileList=_xfdashboard_settings_app_path_file_list_add(pathFileList, *pathIter, FALSE);
			pathIter++;
		}
		g_strfreev(paths);
	}

	entry=g_build_filename(g_get_user_data_dir(), "themes", NULL);
	pathFileList=_xfdashboard_settings_app_path_file_list_add(pathFileList, entry, FALSE);
	g_free(entry);

	homeDirectory=g_get_home_dir();
	if(homeDirectory)
	{
		entry=g_build_filename(homeDirectory, ".themes", NULL);
		pathFileList=_xfdashboard_settings_app_path_file_list_add(pathFileList, entry, FALSE);
		g_free(entry);
	}

	entry=g_build_filename(PACKAGE_DATADIR, "themes", NULL);
	pathFileList=_xfdashboard_settings_app_path_file_list_add(pathFileList, entry, FALSE);
	g_free(entry);

	themesSearchPaths=_xfdashboard_settings_app_path_file_list_list_to_strv(pathFileList);

	/* Set up search path for plugins */
	pathFileList=NULL;

	environmentVariable=g_getenv("XFDASHBOARD_PLUGINS_PATH");
	if(environmentVariable)
	{
		gchar						**paths;
		gchar						**pathIter;

		pathIter=paths=g_strsplit(environmentVariable, ":", -1);
		while(*pathIter)
		{
			pathFileList=_xfdashboard_settings_app_path_file_list_add(pathFileList, *pathIter, FALSE);
			pathIter++;
		}
		g_strfreev(paths);
	}

	entry=g_build_filename(g_get_user_data_dir(), "xfdashboard", "plugins", NULL);
	pathFileList=_xfdashboard_settings_app_path_file_list_add(pathFileList, entry, FALSE);
	g_free(entry);

	entry=g_build_filename(PACKAGE_LIBDIR, "xfdashboard", "plugins", NULL);
	pathFileList=_xfdashboard_settings_app_path_file_list_add(pathFileList, entry, FALSE);
	g_free(entry);

	pluginsSearchPaths=_xfdashboard_settings_app_path_file_list_list_to_strv(pathFileList);

	/* Set up file path for bindings */
	pathFileList=NULL;

	entry=g_build_filename(PACKAGE_DATADIR, "xfdashboard", "bindings.xml", NULL);
	pathFileList=_xfdashboard_settings_app_path_file_list_add(pathFileList, entry, TRUE);
	g_free(entry);

	entry=g_build_filename(g_get_user_config_dir(), "xfdashboard", "bindings.xml", NULL);
	pathFileList=_xfdashboard_settings_app_path_file_list_add(pathFileList, entry, TRUE);
	g_free(entry);

	environmentVariable=g_getenv("XFDASHBOARD_BINDINGS_POOL_FILE");
	if(environmentVariable)
	{
		gchar						**paths;
		gchar						**pathIter;

		pathIter=paths=g_strsplit(environmentVariable, ":", -1);
		while(*pathIter)
		{
			pathFileList=_xfdashboard_settings_app_path_file_list_add(pathFileList, *pathIter, TRUE);
			pathIter++;
		}
		g_strfreev(paths);
	}

	bindingFilePaths=_xfdashboard_settings_app_path_file_list_list_to_strv(pathFileList);

	/* Create settings instance for Xfconf settings storage */
	priv->settings=g_object_new(XFDASHBOARD_TYPE_XFCONF_SETTINGS,
							"binding-files", bindingFilePaths,
							"theme-search-paths", themesSearchPaths,
							"plugin-search-paths", pluginsSearchPaths,
							NULL);
	if(!priv->settings)
	{
		g_critical("Cannot create xfconf settings backend");

		/* Release allocated resources */
		if(themesSearchPaths) g_strfreev(themesSearchPaths);
		if(pluginsSearchPaths) g_strfreev(pluginsSearchPaths);
		if(bindingFilePaths) g_strfreev(bindingFilePaths);

		/* Return failed state */
		return(FALSE);
	}

	/* Setting object instance is unowned on creation, so take a reference to own it */
	g_object_ref(priv->settings);

	/* Release allocated resources */
	if(themesSearchPaths) g_strfreev(themesSearchPaths);
	if(pluginsSearchPaths) g_strfreev(pluginsSearchPaths);
	if(bindingFilePaths) g_strfreev(bindingFilePaths);

	/* Return success result */
	return(TRUE);
}

/* Create and set up GtkBuilder */
static gboolean _xfdashboard_settings_app_create_builder(XfdashboardSettingsApp *self)
{
	XfdashboardSettingsAppPrivate			*priv;
	gchar									*builderFile;
	GtkBuilder								*builder;
	GError									*error;

	g_return_val_if_fail(XFDASHBOARD_IS_SETTINGS_APP(self), FALSE);

	priv=self->priv;
	builderFile=NULL;
	builder=NULL;
	error=NULL;

	/* If builder is already set up return immediately */
	if(priv->builder) return(TRUE);

	/* Search UI file in given environment variable if set.
	 * This makes development easier to test modifications at UI file.
	 */
	if(!builderFile)
	{
		const gchar		*envPath;

		envPath=g_getenv("XFDASHBOARD_UI_PATH");
		if(envPath)
		{
			builderFile=g_build_filename(envPath, PREFERENCES_UI_FILE, NULL);
			g_debug("Trying UI file: %s", builderFile);
			if(!g_file_test(builderFile, G_FILE_TEST_EXISTS | G_FILE_TEST_IS_REGULAR))
			{
				g_free(builderFile);
				builderFile=NULL;
			}
		}
	}

	/* Find UI file at install path */
	if(!builderFile)
	{
		builderFile=g_build_filename(PACKAGE_DATADIR, "xfdashboard", PREFERENCES_UI_FILE, NULL);
		g_debug("Trying UI file: %s", builderFile);
		if(!g_file_test(builderFile, G_FILE_TEST_EXISTS | G_FILE_TEST_IS_REGULAR))
		{
			g_critical("Could not find UI file '%s'.", builderFile);

			/* Release allocated resources */
			g_free(builderFile);

			/* Return fail result */
			return(FALSE);
		}
	}

	/* Create builder */
	builder=gtk_builder_new();
	if(!gtk_builder_add_from_file(builder, builderFile, &error))
	{
		g_critical("Could not load UI resources from '%s': %s",
					builderFile,
					error ? error->message : "Unknown error");

		/* Release allocated resources */
		g_free(builderFile);
		g_object_unref(builder);
		if(error) g_error_free(error);

		/* Return fail result */
		return(FALSE);
	}

	/* Loading UI resource was successful so take extra reference
	 * from builder object to keep it alive. Also get widget, set up
	 * xfconf bindings and connect signals.
	 * REMEMBER: Set (widget's) default value _before_ setting up
	 * xfconf binding.
	 */
	priv->builder=GTK_BUILDER(g_object_ref(builder));
	g_debug("Loaded UI resources from '%s' successfully.", builderFile);

	/* Setup common widgets */
	priv->widgetHelpButton=GTK_WIDGET(gtk_builder_get_object(priv->builder, "help-button"));
	g_signal_connect_swapped(priv->widgetHelpButton,
								"clicked",
								G_CALLBACK(_xfdashboard_settings_app_on_help_clicked),
								self);

	priv->widgetCloseButton=GTK_WIDGET(gtk_builder_get_object(priv->builder, "close-button"));
	g_signal_connect_swapped(priv->widgetCloseButton,
								"clicked",
								G_CALLBACK(_xfdashboard_settings_app_on_close_clicked),
								self);

	/* Tab: General */
	priv->general=xfdashboard_settings_general_new(self);

	/* Tab: Themes */
	priv->themes=xfdashboard_settings_themes_new(self);

	/* Tab: Plugins */
	priv->plugins=xfdashboard_settings_plugins_new(self);

	/* Release allocated resources */
	g_free(builderFile);
	g_object_unref(builder);

	/* Return success result */
	return(TRUE);
}

/* Set up settings application */
static gboolean _xfdashboard_settings_app_setup(XfdashboardSettingsApp *self)
{
	g_return_val_if_fail(XFDASHBOARD_IS_SETTINGS_APP(self), FALSE);

	/* First create settings if not done already */
	if(!_xfdashboard_settings_app_create_settings(self)) return(FALSE);

	/* Next setup build if not done already */
	if(!_xfdashboard_settings_app_create_builder(self)) return(FALSE);

	/* If we get here, setup was successful */
	return(TRUE);
}
 

/* IMPLEMENTATION: GObject */

/* Dispose this object */
static void _xfdashboard_settings_app_dispose(GObject *inObject)
{
	XfdashboardSettingsApp			*self=XFDASHBOARD_SETTINGS_APP(inObject);
	XfdashboardSettingsAppPrivate	*priv=self->priv;

	/* Release allocated resouces */
	priv->dialog=NULL;
	priv->widgetHelpButton=NULL;
	priv->widgetCloseButton=NULL;

	if(priv->themes)
	{
		g_object_unref(priv->themes);
		priv->themes=NULL;
	}

	if(priv->general)
	{
		g_object_unref(priv->general);
		priv->general=NULL;
	}

	if(priv->plugins)
	{
		g_object_unref(priv->plugins);
		priv->plugins=NULL;
	}

	if(priv->builder)
	{
		g_object_unref(priv->builder);
		priv->builder=NULL;
	}

	if(priv->settings)
	{
		g_object_unref(priv->settings);
		priv->settings=NULL;
	}

	/* Call parent's class dispose method */
	G_OBJECT_CLASS(xfdashboard_settings_app_parent_class)->dispose(inObject);
}

/* Class initialization
 * Override functions in parent classes and define properties
 * and signals
 */
static void xfdashboard_settings_app_class_init(XfdashboardSettingsAppClass *klass)
{
	GObjectClass		*gobjectClass=G_OBJECT_CLASS(klass);

	/* Override functions */
	gobjectClass->dispose=_xfdashboard_settings_app_dispose;
}

/* Object initialization
 * Create private structure and set up default values
 */
static void xfdashboard_settings_app_init(XfdashboardSettingsApp *self)
{
	XfdashboardSettingsAppPrivate	*priv;

	priv=self->priv=xfdashboard_settings_app_get_instance_private(self);

	/* Set default values */
	priv->settings=NULL;
	priv->builder=NULL;
	priv->dialog=NULL;
	priv->general=NULL;
	priv->themes=NULL;
	priv->plugins=NULL;
	priv->widgetHelpButton=NULL;
	priv->widgetCloseButton=NULL;
}

/* IMPLEMENTATION: Public API */

/* Create instance of this class */
XfdashboardSettingsApp* xfdashboard_settings_app_new(void)
{
	return(XFDASHBOARD_SETTINGS_APP(g_object_new(XFDASHBOARD_TYPE_SETTINGS_APP, NULL)));
}

/* Create standalone dialog for this settings instance */
GtkWidget* xfdashboard_settings_app_create_dialog(XfdashboardSettingsApp *self)
{
	XfdashboardSettingsAppPrivate	*priv;

	g_return_val_if_fail(XFDASHBOARD_IS_SETTINGS_APP(self), NULL);

	priv=self->priv;

	/* Get builder if not available */
	if(!_xfdashboard_settings_app_setup(self))
	{
		/* An critical error message should be displayed so just return NULL */
		return(NULL);
	}

	/* Get dialog object */
	g_assert(priv->dialog==NULL);

	priv->dialog=gtk_builder_get_object(priv->builder, "preferences-dialog");
	if(!priv->dialog)
	{
		g_critical("Could not get dialog from UI file.");
		return(NULL);
	}

	/* Return widget */
	return(GTK_WIDGET(priv->dialog));
}

/* Create "pluggable" dialog for this settings instance */
GtkWidget* xfdashboard_settings_app_create_plug(XfdashboardSettingsApp *self, Window inSocketID)
{
	XfdashboardSettingsAppPrivate	*priv;
	GtkWidget						*plug;
	GObject							*dialogChild;
#if GTK_CHECK_VERSION(3, 14 ,0)
	GtkWidget						*dialogParent;
#endif

	g_return_val_if_fail(XFDASHBOARD_IS_SETTINGS_APP(self), NULL);
	g_return_val_if_fail(inSocketID, NULL);

	priv=self->priv;

	/* Get builder if not available */
	if(!_xfdashboard_settings_app_setup(self))
	{
		/* An critical error message should be displayed so just return NULL */
		return(NULL);
	}

	/* Get dialog object */
	dialogChild=gtk_builder_get_object(priv->builder, "preferences-plug-child");
	if(!dialogChild)
	{
		g_critical("Could not get dialog from UI file.");
		return(NULL);
	}

	/* Create plug widget and reparent dialog object to it */
	plug=gtk_plug_new(inSocketID);
#if GTK_CHECK_VERSION(3, 14 ,0)
	g_object_ref(G_OBJECT(dialogChild));

	dialogParent=gtk_widget_get_parent(GTK_WIDGET(dialogChild));
	gtk_container_remove(GTK_CONTAINER(dialogParent), GTK_WIDGET(dialogChild));
	gtk_container_add(GTK_CONTAINER(plug), GTK_WIDGET(dialogChild));

	g_object_unref(G_OBJECT(dialogChild));
#else
	gtk_widget_reparent(GTK_WIDGET(dialogChild), plug);
#endif
	gtk_widget_show(GTK_WIDGET(dialogChild));

	/* Return widget */
	return(GTK_WIDGET(plug));
}

/* Get builder */
GtkBuilder* xfdashboard_settings_app_get_builder(XfdashboardSettingsApp *self)
{
	g_return_val_if_fail(XFDASHBOARD_IS_SETTINGS_APP(self), NULL);

	return(self->priv->builder);
}

/* Get settings object instance */
XfdashboardSettings* xfdashboard_settings_app_get_settings(XfdashboardSettingsApp *self)
{
	g_return_val_if_fail(XFDASHBOARD_IS_SETTINGS_APP(self), NULL);

	return(self->priv->settings);
}
