/*
 * settings: Settings of application
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

#include "settings.h"

#include <glib/gi18n-lib.h>
#include <xfconf/xfconf.h>
#include <math.h>

#include "general.h"
#include "plugins.h"
#include "themes.h"


/* Define this class in GObject system */
struct _XfdashboardSettingsPrivate
{
	/* Instance related */
	XfconfChannel					*xfconfChannel;

	GtkBuilder						*builder;

	XfdashboardSettingsGeneral		*general;
	XfdashboardSettingsThemes		*themes;
	XfdashboardSettingsPlugins		*plugins;

	GtkWidget						*widgetCloseButton;
};

G_DEFINE_TYPE_WITH_PRIVATE(XfdashboardSettings,
							xfdashboard_settings,
							G_TYPE_OBJECT)

/* IMPLEMENTATION: Private variables and methods */
#define XFDASHBOARD_XFCONF_CHANNEL					"xfdashboard"

#define PREFERENCES_UI_FILE							"preferences.ui"


/* Close button was clicked */
static void _xfdashboard_settings_on_close_clicked(XfdashboardSettings *self,
													GtkWidget *inWidget)
{
	g_return_if_fail(XFDASHBOARD_IS_SETTINGS(self));

	/* Quit main loop */
	gtk_main_quit();
}

/* Create and set up GtkBuilder */
static gboolean _xfdashboard_settings_create_builder(XfdashboardSettings *self)
{
	XfdashboardSettingsPrivate				*priv;
	gchar									*builderFile;
	GtkBuilder								*builder;
	GError									*error;

	g_return_val_if_fail(XFDASHBOARD_IS_SETTINGS(self), FALSE);

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
			g_critical(_("Could not find UI file '%s'."), builderFile);

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
		g_critical(_("Could not load UI resources from '%s': %s"),
					builderFile,
					error ? error->message : _("Unknown error"));

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
	priv->widgetCloseButton=GTK_WIDGET(gtk_builder_get_object(priv->builder, "close-button"));
	g_signal_connect_swapped(priv->widgetCloseButton,
								"clicked",
								G_CALLBACK(_xfdashboard_settings_on_close_clicked),
								self);

	/* Tab: General */
	priv->general=xfdashboard_settings_general_new(builder);

	/* Tab: Themes */
	priv->themes=xfdashboard_settings_themes_new(builder);

	/* Tab: Plugins */
	priv->plugins=xfdashboard_settings_plugins_new(builder);

	/* Release allocated resources */
	g_free(builderFile);
	g_object_unref(builder);

	/* Return success result */
	return(TRUE);
}


/* IMPLEMENTATION: GObject */

/* Dispose this object */
static void _xfdashboard_settings_dispose(GObject *inObject)
{
	XfdashboardSettings			*self=XFDASHBOARD_SETTINGS(inObject);
	XfdashboardSettingsPrivate	*priv=self->priv;

	/* Release allocated resouces */
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

	if(priv->xfconfChannel)
	{
		priv->xfconfChannel=NULL;
	}

	if(priv->builder)
	{
		g_object_unref(priv->builder);
		priv->builder=NULL;
	}

	/* Call parent's class dispose method */
	G_OBJECT_CLASS(xfdashboard_settings_parent_class)->dispose(inObject);
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
}

/* Object initialization
 * Create private structure and set up default values
 */
static void xfdashboard_settings_init(XfdashboardSettings *self)
{
	XfdashboardSettingsPrivate	*priv;

	priv=self->priv=xfdashboard_settings_get_instance_private(self);

	/* Set default values */
	priv->xfconfChannel=xfconf_channel_get(XFDASHBOARD_XFCONF_CHANNEL);
	priv->builder=NULL;
	priv->general=NULL;
	priv->themes=NULL;
	priv->plugins=NULL;
	priv->widgetCloseButton=NULL;
}

/* IMPLEMENTATION: Public API */

/* Create instance of this class */
XfdashboardSettings* xfdashboard_settings_new(void)
{
	return(XFDASHBOARD_SETTINGS(g_object_new(XFDASHBOARD_TYPE_SETTINGS, NULL)));
}

/* Create standalone dialog for this settings instance */
GtkWidget* xfdashboard_settings_create_dialog(XfdashboardSettings *self)
{
	XfdashboardSettingsPrivate	*priv;
	GObject						*dialog;

	g_return_val_if_fail(XFDASHBOARD_IS_SETTINGS(self), NULL);

	priv=self->priv;

	/* Get builder if not available */
	if(!_xfdashboard_settings_create_builder(self))
	{
		/* An critical error message should be displayed so just return NULL */
		return(NULL);
	}

	/* Get dialog object */
	dialog=gtk_builder_get_object(priv->builder, "preferences-dialog");
	if(!dialog)
	{
		g_critical(_("Could not get dialog from UI file."));
		return(NULL);
	}

	/* Return widget */
	return(GTK_WIDGET(dialog));
}

/* Create "pluggable" dialog for this settings instance */
GtkWidget* xfdashboard_settings_create_plug(XfdashboardSettings *self, Window inSocketID)
{
	XfdashboardSettingsPrivate	*priv;
	GtkWidget					*plug;
	GObject						*dialogChild;
#if GTK_CHECK_VERSION(3, 14 ,0)
	GtkWidget					*dialogParent;
#endif

	g_return_val_if_fail(XFDASHBOARD_IS_SETTINGS(self), NULL);
	g_return_val_if_fail(inSocketID, NULL);

	priv=self->priv;

	/* Get builder if not available */
	if(!_xfdashboard_settings_create_builder(self))
	{
		/* An critical error message should be displayed so just return NULL */
		return(NULL);
	}

	/* Get dialog object */
	dialogChild=gtk_builder_get_object(priv->builder, "preferences-plug-child");
	if(!dialogChild)
	{
		g_critical(_("Could not get dialog from UI file."));
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
