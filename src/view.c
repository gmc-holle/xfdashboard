/*
 * view: Abstract class for views, optional with scrollbars
 * 
 * Copyright 2012-2013 Stephan Haller <nomad@froevel.de>
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

#include <glib/gi18n-lib.h>
#include <gtk/gtk.h>

#include "view.h"
#include "utils.h"

/* Define this class in GObject system */
G_DEFINE_ABSTRACT_TYPE(XfdashboardView,
						xfdashboard_view,
						CLUTTER_TYPE_ACTOR)

/* Private structure - access only by public API if needed */
#define XFDASHBOARD_VIEW_GET_PRIVATE(obj) \
	(G_TYPE_INSTANCE_GET_PRIVATE((obj), XFDASHBOARD_TYPE_VIEW, XfdashboardViewPrivate))

struct _XfdashboardViewPrivate
{
	/* Properties related */
	gchar					*viewName;

	gchar					*viewIcon;
	ClutterImage			*viewIconImage;

	/* Layout manager */
	guint					signalChangedID;
};

/* Properties */
enum
{
	PROP_0,

	PROP_VIEW_NAME,
	PROP_VIEW_ICON,

	PROP_LAST
};

static GParamSpec* XfdashboardViewProperties[PROP_LAST]={ 0, };

/* Signals */
enum
{
	SIGNAL_ACTIVATING,
	SIGNAL_ACTIVATED,
	SIGNAL_DEACTIVATING,
	SIGNAL_DEACTIVATED,

	SIGNAL_NAME_CHANGED,
	SIGNAL_ICON_CHANGED,

	SIGNAL_LAST
};

static guint XfdashboardViewSignals[SIGNAL_LAST]={ 0, };

/* IMPLEMENTATION: Private variables and methods */
#define DEFAULT_ICON_SIZE	64		// TODO: Replace by settings/theming object

/* IMPLEMENTATION: GObject */

/* Called last in object instance creation chain */
void _xfdashboard_view_constructed(GObject *inObject)
{
	g_return_if_fail(XFDASHBOARD_IS_VIEW(inObject));

	XfdashboardView			*self=XFDASHBOARD_VIEW(inObject);
	XfdashboardViewClass	*klass=XFDASHBOARD_VIEW_GET_CLASS(self);

	/* Call parent's class constructed method */
	G_OBJECT_CLASS(xfdashboard_view_parent_class)->constructed(inObject);

	/* Call our virtual function "created" */
	if(klass->created) (klass->created)(self);
		else
		{
			g_warning(_("%s has not implemented virtual function 'created'"), G_OBJECT_TYPE_NAME(inObject));

			/* Set up default values for properties */
			g_object_set(G_OBJECT(self),
							"view-name", _("unnamed"),
							"view-icon", GTK_STOCK_MISSING_IMAGE,
							NULL);
		}
}

/* Dispose this object */
void _xfdashboard_view_dispose(GObject *inObject)
{
	XfdashboardView			*self=XFDASHBOARD_VIEW(inObject);
	XfdashboardViewPrivate	*priv=self->priv;

	/* Release allocated resources */
	if(priv->viewName)
	{
		g_free(priv->viewName);
		priv->viewName=NULL;
	}

	if(priv->viewIcon)
	{
		g_free(priv->viewIcon);
		priv->viewIcon=NULL;
	}

	if(priv->viewIconImage)
	{
		g_object_unref(priv->viewIconImage);
		priv->viewIconImage=NULL;
	}

	/* Call parent's class dispose method */
	G_OBJECT_CLASS(xfdashboard_view_parent_class)->dispose(inObject);
}

/* Set/get properties */
void _xfdashboard_view_set_property(GObject *inObject,
												guint inPropID,
												const GValue *inValue,
												GParamSpec *inSpec)
{
	XfdashboardView		*self=XFDASHBOARD_VIEW(inObject);
	
	switch(inPropID)
	{
		case PROP_VIEW_NAME:
			xfdashboard_view_set_name(self, g_value_get_string(inValue));
			break;
			
		case PROP_VIEW_ICON:
			xfdashboard_view_set_icon(self, g_value_get_string(inValue));
			break;

		default:
			G_OBJECT_WARN_INVALID_PROPERTY_ID(inObject, inPropID, inSpec);
			break;
	}
}

void _xfdashboard_view_get_property(GObject *inObject,
									guint inPropID,
									GValue *outValue,
									GParamSpec *inSpec)
{
	XfdashboardView		*self=XFDASHBOARD_VIEW(inObject);

	switch(inPropID)
	{
		case PROP_VIEW_NAME:
			g_value_set_string(outValue, self->priv->viewName);
			break;

		case PROP_VIEW_ICON:
			g_value_set_string(outValue, self->priv->viewIcon);
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
void xfdashboard_view_class_init(XfdashboardViewClass *klass)
{
	GObjectClass			*gobjectClass=G_OBJECT_CLASS(klass);

	/* Override functions */
	gobjectClass->constructed=_xfdashboard_view_constructed;
	gobjectClass->set_property=_xfdashboard_view_set_property;
	gobjectClass->get_property=_xfdashboard_view_get_property;
	gobjectClass->dispose=_xfdashboard_view_dispose;

	klass->created=NULL;

	/* Set up private structure */
	g_type_class_add_private(klass, sizeof(XfdashboardViewPrivate));

	/* Define properties */
	XfdashboardViewProperties[PROP_VIEW_NAME]=
		g_param_spec_string("view-name",
							_("View name"),
							_("Name of view used to display"),
							NULL,
							G_PARAM_READWRITE);

	XfdashboardViewProperties[PROP_VIEW_ICON]=
		g_param_spec_string("view-icon",
							_("View icon"),
							_("Icon of view used to display. Icon name can be a themed icon name or file name"),
							NULL,
							G_PARAM_READWRITE);

	g_object_class_install_properties(gobjectClass, PROP_LAST, XfdashboardViewProperties);

	/* Define signals */
	XfdashboardViewSignals[SIGNAL_ACTIVATING]=
		g_signal_new("activating",
						G_TYPE_FROM_CLASS(klass),
						G_SIGNAL_RUN_LAST,
						G_STRUCT_OFFSET(XfdashboardViewClass, activating),
						NULL,
						NULL,
						g_cclosure_marshal_VOID__VOID,
						G_TYPE_NONE,
						0);

	XfdashboardViewSignals[SIGNAL_ACTIVATED]=
		g_signal_new("activated",
						G_TYPE_FROM_CLASS(klass),
						G_SIGNAL_RUN_LAST,
						G_STRUCT_OFFSET(XfdashboardViewClass, activated),
						NULL,
						NULL,
						g_cclosure_marshal_VOID__VOID,
						G_TYPE_NONE,
						0);

	XfdashboardViewSignals[SIGNAL_DEACTIVATING]=
		g_signal_new("deactivating",
						G_TYPE_FROM_CLASS(klass),
						G_SIGNAL_RUN_LAST,
						G_STRUCT_OFFSET(XfdashboardViewClass, deactivating),
						NULL,
						NULL,
						g_cclosure_marshal_VOID__VOID,
						G_TYPE_NONE,
						0);

	XfdashboardViewSignals[SIGNAL_DEACTIVATED]=
		g_signal_new("deactivated",
						G_TYPE_FROM_CLASS(klass),
						G_SIGNAL_RUN_LAST,
						G_STRUCT_OFFSET(XfdashboardViewClass, activated),
						NULL,
						NULL,
						g_cclosure_marshal_VOID__VOID,
						G_TYPE_NONE,
						0);

	XfdashboardViewSignals[SIGNAL_NAME_CHANGED]=
		g_signal_new("name-changed",
						G_TYPE_FROM_CLASS(klass),
						G_SIGNAL_RUN_LAST,
						G_STRUCT_OFFSET(XfdashboardViewClass, name_changed),
						NULL,
						NULL,
						g_cclosure_marshal_VOID__STRING,
						G_TYPE_NONE,
						1,
						G_TYPE_STRING);

	XfdashboardViewSignals[SIGNAL_ICON_CHANGED]=
		g_signal_new("icon-changed",
						G_TYPE_FROM_CLASS(klass),
						G_SIGNAL_RUN_LAST,
						G_STRUCT_OFFSET(XfdashboardViewClass, icon_changed),
						NULL,
						NULL,
						g_cclosure_marshal_VOID__OBJECT,
						G_TYPE_NONE,
						1,
						CLUTTER_TYPE_IMAGE);

}

/* Object initialization
 * Create private structure and set up default values
 */
void xfdashboard_view_init(XfdashboardView *self)
{
	XfdashboardViewPrivate	*priv;

	priv=self->priv=XFDASHBOARD_VIEW_GET_PRIVATE(self);

	/* Set up default values */
	priv->viewName=NULL;
	priv->viewIcon=NULL;
	priv->viewIconImage=NULL;

	/* Set up actor */
	clutter_actor_set_reactive(CLUTTER_ACTOR(self), TRUE);
}

/* Implementation: Public API */

/* Get/set name of view */
const gchar* xfdashboard_view_get_name(XfdashboardView *self)
{
	g_return_val_if_fail(XFDASHBOARD_IS_VIEW(self), NULL);

	return(self->priv->viewName);
}

void xfdashboard_view_set_name(XfdashboardView *self, const gchar *inName)
{
	g_return_if_fail(XFDASHBOARD_IS_VIEW(self));
	g_return_if_fail(inName!=NULL);

	XfdashboardViewPrivate	*priv=self->priv;

	/* Set value if changed */
	if(g_strcmp0(priv->viewName, inName)!=0)
	{
		if(priv->viewName) g_free(priv->viewName);
		priv->viewName=g_strdup(inName);

		/* Notify about property change */
		g_object_notify_by_pspec(G_OBJECT(self), XfdashboardViewProperties[PROP_VIEW_NAME]);
		g_signal_emit(self, XfdashboardViewSignals[SIGNAL_NAME_CHANGED], 0, priv->viewName);
	}
}

/* Get/Set icon of view */
const gchar* xfdashboard_view_get_icon(XfdashboardView *self)
{
  g_return_val_if_fail(XFDASHBOARD_IS_VIEW(self), NULL);

  return(self->priv->viewIcon);
}

void xfdashboard_view_set_icon(XfdashboardView *self, const gchar *inIcon)
{
	g_return_if_fail(XFDASHBOARD_IS_VIEW(self));
	g_return_if_fail(inIcon!=NULL);

	XfdashboardViewPrivate	*priv=self->priv;

	/* Set value if changed */
	if(g_strcmp0(priv->viewIcon, inIcon)!=0)
	{
		/* Set new icon name */
		if(priv->viewIcon) g_free(priv->viewIcon);
		priv->viewIcon=g_strdup(inIcon);

		/* Set new icon */
		if(priv->viewIconImage) g_object_unref(priv->viewIconImage);
		priv->viewIconImage=xfdashboard_get_image_for_icon_name(priv->viewIcon, DEFAULT_ICON_SIZE);

		/* Notify about property change */
		g_object_notify_by_pspec(G_OBJECT(self), XfdashboardViewProperties[PROP_VIEW_ICON]);
		g_signal_emit(self, XfdashboardViewSignals[SIGNAL_ICON_CHANGED], 0, priv->viewIconImage);
	}
}
