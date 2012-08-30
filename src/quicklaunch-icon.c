/*
 * quicklaunch-icon.c: An actor used in quicklaunch showing
 *                     icon of an application
 * 
 * Copyright 2012 Stephan Haller <nomad@froevel.de>
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

#include "quicklaunch-icon.h"

#include <gdk/gdk.h>
#include <gtk/gtk.h>

/* Define this class in GObject system */
G_DEFINE_TYPE(XfdashboardQuicklaunchIcon,
				xfdashboard_quicklaunch_icon,
				CLUTTER_TYPE_TEXTURE)

/* Private structure - access only by public API if needed */
#define XFDASHBOARD_QUICKLAUNCH_ICON_GET_PRIVATE(obj) \
	(G_TYPE_INSTANCE_GET_PRIVATE((obj), XFDASHBOARD_TYPE_QUICKLAUNCH_ICON, XfdashboardQuicklaunchIconPrivate))

struct _XfdashboardQuicklaunchIconPrivate
{
	/* Application information this actor represents */
	GDesktopAppInfo		*appInfo;

	/* Actor actions */
	ClutterAction		*clickAction;
};

/* Properties */
enum
{
	PROP_0,

	PROP_DESKTOP_FILE,
	PROP_DESKTOP_APPLICATION_INFO,
	
	PROP_LAST
};

static GParamSpec* XfdashboardQuicklaunchIconProperties[PROP_LAST]={ 0, };

/* Signals */
enum
{
	CLICKED,

	SIGNAL_LAST
};

static guint XfdashboardQuicklaunchIconSignals[SIGNAL_LAST]={ 0, };

/* Private constants */

/* IMPLEMENTATION: Private variables and methods */
#define DEFAULT_ICON_SIZE		64

/* Set desktop file and setup actors for display application icon */
void _xfdashboard_quicklaunch_icon_set_desktop_file(XfdashboardQuicklaunchIcon *self, const gchar *inDesktopFile)
{
	XfdashboardQuicklaunchIconPrivate	*priv;
	GIcon								*icon=NULL;
	GdkPixbuf							*iconPixbuf=NULL;
	GtkIconInfo							*iconInfo=NULL;
	GError								*error=NULL;

	g_return_if_fail(XFDASHBOARD_IS_QUICKLAUNCH_ICON(self));

	priv=XFDASHBOARD_QUICKLAUNCH_ICON(self)->priv;
	
	/* Dispose current application information */
	if(priv->appInfo)
	{
		g_object_unref(priv->appInfo);
		priv->appInfo=NULL;
	}

	/* Get new application information of basename of desktop file given */
	if(inDesktopFile)
	{
		priv->appInfo=g_desktop_app_info_new(inDesktopFile);
		if(!priv->appInfo)
		{
			g_warning("Could not get application info '%s' for quicklaunch icon",
						inDesktopFile);
		}
	}

	/* Set up icon of application */
	if(priv->appInfo)
	{
		icon=g_app_info_get_icon(G_APP_INFO(priv->appInfo));
		if(!icon) g_warning("Could not get icon for desktop file '%s'", inDesktopFile);
	}

	if(icon)
	{
		iconInfo=gtk_icon_theme_lookup_by_gicon(gtk_icon_theme_get_default(),
												icon,
												DEFAULT_ICON_SIZE,
												0);
		if(!iconInfo) g_warning("Could not lookup icon for '%s'", inDesktopFile);
	}

	if(iconInfo)
	{
		error=NULL;
		iconPixbuf=gtk_icon_info_load_icon(iconInfo, &error);
		if(!iconPixbuf)
		{
			g_warning("Could not load icon for quicklaunch icon actor: %s",
						(error && error->message) ?  error->message : "unknown error");
			if(error!=NULL) g_error_free(error);
		}
	}
	
	if(iconPixbuf)
	{
		error=NULL;
		if(!clutter_texture_set_from_rgb_data(CLUTTER_TEXTURE(self),
													gdk_pixbuf_get_pixels(iconPixbuf),
													gdk_pixbuf_get_has_alpha(iconPixbuf),
													gdk_pixbuf_get_width(iconPixbuf),
													gdk_pixbuf_get_height(iconPixbuf),
													gdk_pixbuf_get_rowstride(iconPixbuf),
													gdk_pixbuf_get_has_alpha(iconPixbuf) ? 4 : 3,
													CLUTTER_TEXTURE_NONE,
													&error))
		{
			g_warning("Could not create quicklaunch icon actor: %s",
						(error && error->message) ?  error->message : "unknown error");
		}

		if(error!=NULL) g_error_free(error);
		g_object_unref(iconPixbuf);
	}
	
	if(iconInfo) gtk_icon_info_free(iconInfo);

	/* Queue a redraw as the actors are now available */
	clutter_actor_queue_redraw(CLUTTER_ACTOR(self));
}

/* proxy ClickAction signals */
static void xfdashboard_quicklaunch_icon_clicked(ClutterClickAction *inAction,
													ClutterActor *inActor,
													gpointer inUserData)
{
	g_signal_emit(inActor, XfdashboardQuicklaunchIconSignals[CLICKED], 0);
}

/* IMPLEMENTATION: GObject */

/* Dispose this object */
static void xfdashboard_quicklaunch_icon_dispose(GObject *inObject)
{
	XfdashboardQuicklaunchIconPrivate	*priv=XFDASHBOARD_QUICKLAUNCH_ICON(inObject)->priv;

	/* Dispose current application information */
	if(priv->appInfo)
	{
		g_object_unref(priv->appInfo);
		priv->appInfo=NULL;
	}

	/* Call parent's class dispose method */
	G_OBJECT_CLASS(xfdashboard_quicklaunch_icon_parent_class)->dispose(inObject);
}

/* Set/get properties */
static void xfdashboard_quicklaunch_icon_set_property(GObject *inObject,
												guint inPropID,
												const GValue *inValue,
												GParamSpec *inSpec)
{
	XfdashboardQuicklaunchIcon			*self=XFDASHBOARD_QUICKLAUNCH_ICON(inObject);
	
	switch(inPropID)
	{
		case PROP_DESKTOP_FILE:
			_xfdashboard_quicklaunch_icon_set_desktop_file(self, g_value_get_string(inValue));
			break;

		default:
			G_OBJECT_WARN_INVALID_PROPERTY_ID(inObject, inPropID, inSpec);
			break;
	}
}

static void xfdashboard_quicklaunch_icon_get_property(GObject *inObject,
												guint inPropID,
												GValue *outValue,
												GParamSpec *inSpec)
{
	XfdashboardQuicklaunchIcon	*self=XFDASHBOARD_QUICKLAUNCH_ICON(inObject);

	switch(inPropID)
	{
		case PROP_DESKTOP_FILE:
			g_value_set_string(outValue, g_desktop_app_info_get_filename(self->priv->appInfo));
			break;

		case PROP_DESKTOP_APPLICATION_INFO:
			g_value_set_object(outValue, self->priv->appInfo);
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
static void xfdashboard_quicklaunch_icon_class_init(XfdashboardQuicklaunchIconClass *klass)
{
	GObjectClass		*gobjectClass=G_OBJECT_CLASS(klass);

	/* Override functions */
	gobjectClass->dispose=xfdashboard_quicklaunch_icon_dispose;
	gobjectClass->set_property=xfdashboard_quicklaunch_icon_set_property;
	gobjectClass->get_property=xfdashboard_quicklaunch_icon_get_property;

	/* Set up private structure */
	g_type_class_add_private(klass, sizeof(XfdashboardQuicklaunchIconPrivate));

	/* Define properties */
	XfdashboardQuicklaunchIconProperties[PROP_DESKTOP_FILE]=
		g_param_spec_string("desktop-file",
							"Basename of desktop file",
							"Basename of desktop file containing all information about this application",
							"",
							G_PARAM_READWRITE | G_PARAM_CONSTRUCT);

	XfdashboardQuicklaunchIconProperties[PROP_DESKTOP_APPLICATION_INFO]=
		g_param_spec_object("desktop-application-info",
							"Desktop application information",
							"GDesktopAppInfo object containing all information about this application ",
							G_TYPE_DESKTOP_APP_INFO,
							G_PARAM_READABLE);

	g_object_class_install_properties(gobjectClass, PROP_LAST, XfdashboardQuicklaunchIconProperties);

	/* Define signals */
	XfdashboardQuicklaunchIconSignals[CLICKED]=
		g_signal_new("clicked",
						G_TYPE_FROM_CLASS(klass),
						G_SIGNAL_RUN_LAST,
						G_STRUCT_OFFSET(XfdashboardQuicklaunchIconClass, clicked),
						NULL,
						NULL,
						g_cclosure_marshal_VOID__VOID,
						G_TYPE_NONE,
						0);

}

/* Object initialization
 * Create private structure and set up default values
 */
static void xfdashboard_quicklaunch_icon_init(XfdashboardQuicklaunchIcon *self)
{
	XfdashboardQuicklaunchIconPrivate	*priv;

	priv=self->priv=XFDASHBOARD_QUICKLAUNCH_ICON_GET_PRIVATE(self);

	/* This actor is react on events */
	clutter_actor_set_reactive(CLUTTER_ACTOR(self), TRUE);

	/* Sub-classed texture should synchronize its size to loaded texture */
	clutter_texture_set_sync_size(CLUTTER_TEXTURE(self), TRUE);
	
	/* Set up default values */
	priv->appInfo=NULL;

	/* Connect signals */
	priv->clickAction=clutter_click_action_new();
	clutter_actor_add_action(CLUTTER_ACTOR(self), priv->clickAction);
	g_signal_connect(priv->clickAction, "clicked", G_CALLBACK(xfdashboard_quicklaunch_icon_clicked), NULL);
}

/* Implementation: Public API */

/* Create new actor */
ClutterActor* xfdashboard_quicklaunch_icon_new()
{
	return(g_object_new(XFDASHBOARD_TYPE_QUICKLAUNCH_ICON,
						NULL));
}

ClutterActor* xfdashboard_quicklaunch_icon_new_full(const gchar *inDesktopFile)
{
	return(g_object_new(XFDASHBOARD_TYPE_QUICKLAUNCH_ICON,
						"desktop-file", inDesktopFile,
						NULL));
}

/* Get/set desktop file */
const gchar* xfdashboard_quicklaunch_icon_get_desktop_file(XfdashboardQuicklaunchIcon *self)
{
	g_return_val_if_fail(XFDASHBOARD_IS_QUICKLAUNCH_ICON(self), NULL);

	return(g_desktop_app_info_get_filename(self->priv->appInfo));
}

void xfdashboard_quicklaunch_icon_set_desktop_file(XfdashboardQuicklaunchIcon *self, const gchar *inDesktopFile)
{
	g_return_if_fail(XFDASHBOARD_IS_QUICKLAUNCH_ICON(self));

	_xfdashboard_quicklaunch_icon_set_desktop_file(self, inDesktopFile);
}

/* Get/set desktop application info */
const GDesktopAppInfo* xfdashboard_quicklaunch_icon_get_desktop_application_info(XfdashboardQuicklaunchIcon *self)
{
	g_return_val_if_fail(XFDASHBOARD_IS_QUICKLAUNCH_ICON(self), NULL);

	return(self->priv->appInfo);
}
