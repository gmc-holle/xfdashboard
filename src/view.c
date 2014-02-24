/*
 * view: Abstract class for views, optional with scrollbars
 * 
 * Copyright 2012-2014 Stephan Haller <nomad@froevel.de>
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

#include "view.h"

#include <glib/gi18n-lib.h>
#include <gtk/gtk.h>

#include "marshal.h"
#include "image.h"

/* Define this class in GObject system */
G_DEFINE_ABSTRACT_TYPE(XfdashboardView,
						xfdashboard_view,
						XFDASHBOARD_TYPE_ACTOR)

/* Private structure - access only by public API if needed */
#define XFDASHBOARD_VIEW_GET_PRIVATE(obj) \
	(G_TYPE_INSTANCE_GET_PRIVATE((obj), XFDASHBOARD_TYPE_VIEW, XfdashboardViewPrivate))

struct _XfdashboardViewPrivate
{
	/* Properties related */
	gchar					*viewInternalName;

	gchar					*viewName;

	gchar					*viewIcon;
	ClutterImage			*viewIconImage;

	XfdashboardFitMode		fitMode;

	gboolean				isEnabled;

	/* Layout manager */
	guint					signalChangedID;
};

/* Properties */
enum
{
	PROP_0,

	PROP_VIEW_INTERNAL_NAME,
	PROP_VIEW_NAME,
	PROP_VIEW_ICON,

	PROP_FIT_MODE,

	PROP_ENABLED,

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

	SIGNAL_ENABLING,
	SIGNAL_ENABLED,
	SIGNAL_DISABLING,
	SIGNAL_DISABLED,

	SIGNAL_NAME_CHANGED,
	SIGNAL_ICON_CHANGED,

	SIGNAL_SCROLL_TO,

	SIGNAL_LAST
};

static guint XfdashboardViewSignals[SIGNAL_LAST]={ 0, };

/* IMPLEMENTATION: Private variables and methods */

/* IMPLEMENTATION: GObject */

/* Dispose this object */
static void _xfdashboard_view_dispose(GObject *inObject)
{
	XfdashboardView			*self=XFDASHBOARD_VIEW(inObject);
	XfdashboardViewPrivate	*priv=self->priv;

	/* Release allocated resources */
	if(priv->viewInternalName)
	{
		g_free(priv->viewInternalName);
		priv->viewInternalName=NULL;
	}

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
static void _xfdashboard_view_set_property(GObject *inObject,
											guint inPropID,
											const GValue *inValue,
											GParamSpec *inSpec)
{
	XfdashboardView		*self=XFDASHBOARD_VIEW(inObject);
	
	switch(inPropID)
	{
		case PROP_VIEW_INTERNAL_NAME:
			xfdashboard_view_set_internal_name(self, g_value_get_string(inValue));
			break;

		case PROP_VIEW_NAME:
			xfdashboard_view_set_name(self, g_value_get_string(inValue));
			break;
			
		case PROP_VIEW_ICON:
			xfdashboard_view_set_icon(self, g_value_get_string(inValue));
			break;

		case PROP_FIT_MODE:
			xfdashboard_view_set_fit_mode(self, (XfdashboardFitMode)g_value_get_enum(inValue));
			break;

		case PROP_ENABLED:
			xfdashboard_view_set_enabled(self, g_value_get_boolean(inValue));
			break;

		default:
			G_OBJECT_WARN_INVALID_PROPERTY_ID(inObject, inPropID, inSpec);
			break;
	}
}

static void _xfdashboard_view_get_property(GObject *inObject,
											guint inPropID,
											GValue *outValue,
											GParamSpec *inSpec)
{
	XfdashboardView		*self=XFDASHBOARD_VIEW(inObject);

	switch(inPropID)
	{
		case PROP_VIEW_INTERNAL_NAME:
			g_value_set_string(outValue, self->priv->viewInternalName);
			break;

		case PROP_VIEW_NAME:
			g_value_set_string(outValue, self->priv->viewName);
			break;

		case PROP_VIEW_ICON:
			g_value_set_string(outValue, self->priv->viewIcon);
			break;

		case PROP_FIT_MODE:
			g_value_set_enum(outValue, self->priv->fitMode);
			break;

		case PROP_ENABLED:
			g_value_set_boolean(outValue, self->priv->isEnabled);
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
static void xfdashboard_view_class_init(XfdashboardViewClass *klass)
{
	GObjectClass			*gobjectClass=G_OBJECT_CLASS(klass);

	/* Override functions */
	gobjectClass->set_property=_xfdashboard_view_set_property;
	gobjectClass->get_property=_xfdashboard_view_get_property;
	gobjectClass->dispose=_xfdashboard_view_dispose;

	/* Set up private structure */
	g_type_class_add_private(klass, sizeof(XfdashboardViewPrivate));

	/* Define properties */
	XfdashboardViewProperties[PROP_VIEW_INTERNAL_NAME]=
		g_param_spec_string("view-internal-name",
							_("View internal name"),
							_("Internal and untranslated name of view used in application"),
							NULL,
							G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS);

	XfdashboardViewProperties[PROP_VIEW_NAME]=
		g_param_spec_string("view-name",
							_("View name"),
							_("Name of view used to display"),
							NULL,
							G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS);

	XfdashboardViewProperties[PROP_VIEW_ICON]=
		g_param_spec_string("view-icon",
							_("View icon"),
							_("Icon of view used to display. Icon name can be a themed icon name or file name"),
							NULL,
							G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS);

	XfdashboardViewProperties[PROP_FIT_MODE]=
		g_param_spec_string("fit-mode",
							_("Fit mode"),
							_("Defines if view should be fit into viewpad and its orientation"),
							XFDASHBOARD_FIT_MODE_NONE,
							G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS);

	XfdashboardViewProperties[PROP_ENABLED]=
		g_param_spec_boolean("enabled",
								_("Enabled"),
								_("This flag indicates if is view is enabled and activable"),
								TRUE,
								G_PARAM_READABLE | G_PARAM_STATIC_STRINGS);

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

	XfdashboardViewSignals[SIGNAL_ENABLING]=
		g_signal_new("enabling",
						G_TYPE_FROM_CLASS(klass),
						G_SIGNAL_RUN_LAST,
						G_STRUCT_OFFSET(XfdashboardViewClass, enabling),
						NULL,
						NULL,
						g_cclosure_marshal_VOID__VOID,
						G_TYPE_NONE,
						0);

	XfdashboardViewSignals[SIGNAL_ENABLED]=
		g_signal_new("enabled",
						G_TYPE_FROM_CLASS(klass),
						G_SIGNAL_RUN_LAST,
						G_STRUCT_OFFSET(XfdashboardViewClass, enabled),
						NULL,
						NULL,
						g_cclosure_marshal_VOID__VOID,
						G_TYPE_NONE,
						0);

	XfdashboardViewSignals[SIGNAL_DISABLING]=
		g_signal_new("disabling",
						G_TYPE_FROM_CLASS(klass),
						G_SIGNAL_RUN_LAST,
						G_STRUCT_OFFSET(XfdashboardViewClass, disabling),
						NULL,
						NULL,
						g_cclosure_marshal_VOID__VOID,
						G_TYPE_NONE,
						0);

	XfdashboardViewSignals[SIGNAL_DISABLED]=
		g_signal_new("disabled",
						G_TYPE_FROM_CLASS(klass),
						G_SIGNAL_RUN_LAST,
						G_STRUCT_OFFSET(XfdashboardViewClass, disabled),
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

	XfdashboardViewSignals[SIGNAL_SCROLL_TO]=
		g_signal_new("scroll-to",
						G_TYPE_FROM_CLASS(klass),
						G_SIGNAL_RUN_LAST | G_SIGNAL_ACTION,
						G_STRUCT_OFFSET(XfdashboardViewClass, scroll_to),
						NULL,
						NULL,
						_xfdashboard_marshal_VOID__FLOAT_FLOAT,
						G_TYPE_NONE,
						2,
						G_TYPE_FLOAT,
						G_TYPE_FLOAT);
}

/* Object initialization
 * Create private structure and set up default values
 */
static void xfdashboard_view_init(XfdashboardView *self)
{
	XfdashboardViewPrivate	*priv;

	priv=self->priv=XFDASHBOARD_VIEW_GET_PRIVATE(self);

	/* Set up default values */
	priv->viewName=NULL;
	priv->viewIcon=NULL;
	priv->viewIconImage=NULL;
	priv->fitMode=XFDASHBOARD_FIT_MODE_NONE;
	priv->isEnabled=TRUE;

	/* Set up actor */
	clutter_actor_set_reactive(CLUTTER_ACTOR(self), TRUE);
}

/* IMPLEMENTATION: Public API */

/* Get/set internal name of view */
const gchar* xfdashboard_view_get_internal_name(XfdashboardView *self)
{
	g_return_val_if_fail(XFDASHBOARD_IS_VIEW(self), NULL);

	return(self->priv->viewInternalName);
}

void xfdashboard_view_set_internal_name(XfdashboardView *self, const gchar *inInternalName)
{
	XfdashboardViewPrivate	*priv;

	g_return_if_fail(XFDASHBOARD_IS_VIEW(self));
	g_return_if_fail(inInternalName!=NULL);

	priv=self->priv;

	/* Set value if changed */
	if(g_strcmp0(priv->viewInternalName, inInternalName)!=0)
	{
		if(priv->viewInternalName) g_free(priv->viewInternalName);
		priv->viewInternalName=g_strdup(inInternalName);

		/* Notify about property change */
		g_object_notify_by_pspec(G_OBJECT(self), XfdashboardViewProperties[PROP_VIEW_INTERNAL_NAME]);
	}
}

/* Get/set name of view */
const gchar* xfdashboard_view_get_name(XfdashboardView *self)
{
	g_return_val_if_fail(XFDASHBOARD_IS_VIEW(self), NULL);

	return(self->priv->viewName);
}

void xfdashboard_view_set_name(XfdashboardView *self, const gchar *inName)
{
	XfdashboardViewPrivate	*priv;

	g_return_if_fail(XFDASHBOARD_IS_VIEW(self));
	g_return_if_fail(inName!=NULL);

	priv=self->priv;

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

/* Get/set icon of view */
const gchar* xfdashboard_view_get_icon(XfdashboardView *self)
{
  g_return_val_if_fail(XFDASHBOARD_IS_VIEW(self), NULL);

  return(self->priv->viewIcon);
}

void xfdashboard_view_set_icon(XfdashboardView *self, const gchar *inIcon)
{
	XfdashboardViewPrivate	*priv;

	g_return_if_fail(XFDASHBOARD_IS_VIEW(self));
	g_return_if_fail(inIcon!=NULL);

	priv=self->priv;

	/* Set value if changed */
	if(g_strcmp0(priv->viewIcon, inIcon)!=0)
	{
		/* Set new icon name */
		if(priv->viewIcon) g_free(priv->viewIcon);
		priv->viewIcon=g_strdup(inIcon);

		/* Set new icon */
		if(priv->viewIconImage) g_object_unref(priv->viewIconImage);
		priv->viewIconImage=xfdashboard_image_new_for_icon_name(priv->viewIcon, 64.0f);

		/* Notify about property change */
		g_object_notify_by_pspec(G_OBJECT(self), XfdashboardViewProperties[PROP_VIEW_ICON]);
		g_signal_emit(self, XfdashboardViewSignals[SIGNAL_ICON_CHANGED], 0, priv->viewIconImage);
	}
}

/* Get/set fit mode of view */
XfdashboardFitMode xfdashboard_view_get_fit_mode(XfdashboardView *self)
{
  g_return_val_if_fail(XFDASHBOARD_IS_VIEW(self), XFDASHBOARD_FIT_MODE_NONE);

  return(self->priv->fitMode);
}

void xfdashboard_view_set_fit_mode(XfdashboardView *self, XfdashboardFitMode inFitMode)
{
	XfdashboardViewPrivate	*priv;
	XfdashboardViewClass	*klass;

	g_return_if_fail(XFDASHBOARD_IS_VIEW(self));

	priv=self->priv;
	klass=XFDASHBOARD_VIEW_GET_CLASS(self);

	/* Set value if changed */
	if(priv->fitMode!=inFitMode)
	{
		/* Set new fit mode */
		priv->fitMode=inFitMode;

		/* Call virtual function for setting fit mode */
		if(klass->set_fit_mode) klass->set_fit_mode(self, inFitMode);

		/* Notify about property change */
		g_object_notify_by_pspec(G_OBJECT(self), XfdashboardViewProperties[PROP_FIT_MODE]);
	}
}

/* Get/set enabled state of view */
gboolean xfdashboard_view_get_enabled(XfdashboardView *self)
{
  g_return_val_if_fail(XFDASHBOARD_IS_VIEW(self), FALSE);

  return(self->priv->isEnabled);
}

void xfdashboard_view_set_enabled(XfdashboardView *self, gboolean inIsEnabled)
{
	XfdashboardViewPrivate	*priv;
	guint					signalBeforeID, signalAfterID;

	g_return_if_fail(XFDASHBOARD_IS_VIEW(self));

	priv=self->priv;

	/* Set value if changed */
	if(priv->isEnabled!=inIsEnabled)
	{
		/* Get signal ID to emit depending on enabled state */
		signalBeforeID=(inIsEnabled==TRUE ? SIGNAL_ENABLING : SIGNAL_DISABLING);
		signalAfterID=(inIsEnabled==TRUE ? SIGNAL_ENABLED : SIGNAL_DISABLED);

		/* Set new enabled state */
		g_signal_emit(self, XfdashboardViewSignals[signalBeforeID], 0, self);

		priv->isEnabled=inIsEnabled;
		if(priv->isEnabled) xfdashboard_actor_add_style_pseudo_class(XFDASHBOARD_ACTOR(self), "enabled");
			else xfdashboard_actor_remove_style_pseudo_class(XFDASHBOARD_ACTOR(self), "enabled");

		g_signal_emit(self, XfdashboardViewSignals[signalAfterID], 0, self);

		/* Notify about property change */
		g_object_notify_by_pspec(G_OBJECT(self), XfdashboardViewProperties[PROP_ENABLED]);
	}
}

/* Scroll view to requested coordinates */
void xfdashboard_view_scroll_to(XfdashboardView *self, gfloat inX, gfloat inY)
{
	g_return_if_fail(XFDASHBOARD_IS_VIEW(self));

	g_signal_emit(self, XfdashboardViewSignals[SIGNAL_SCROLL_TO], 0, inX, inY);
}
