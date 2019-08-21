/*
 * view: Abstract class for views, optional with scrollbars
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

#include <libxfdashboard/view.h>

#include <glib/gi18n-lib.h>
#include <gtk/gtk.h>

#include <libxfdashboard/marshal.h>
#include <libxfdashboard/image-content.h>
#include <libxfdashboard/utils.h>
#include <libxfdashboard/focus-manager.h>
#include <libxfdashboard/viewpad.h>
#include <libxfdashboard/enums.h>
#include <libxfdashboard/compat.h>


/* Define this class in GObject system */
struct _XfdashboardViewPrivate
{
	/* Properties related */
	gchar					*viewID;

	gchar					*viewName;

	gchar					*viewIcon;
	ClutterContent			*viewIconImage;

	XfdashboardViewFitMode	fitMode;

	gboolean				isEnabled;

	/* Layout manager */
	guint					signalChangedID;
};

G_DEFINE_ABSTRACT_TYPE_WITH_PRIVATE(XfdashboardView,
									xfdashboard_view,
									XFDASHBOARD_TYPE_ACTOR)

/* Properties */
enum
{
	PROP_0,

	PROP_VIEW_ID,
	PROP_VIEW_NAME,
	PROP_VIEW_ICON,

	PROP_VIEW_FIT_MODE,

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
	SIGNAL_CHILD_NEEDS_SCROLL,
	SIGNAL_CHILD_ENSURE_VISIBLE,

	ACTION_VIEW_ACTIVATE,

	SIGNAL_LAST
};

static guint XfdashboardViewSignals[SIGNAL_LAST]={ 0, };

/* IMPLEMENTATION: Private variables and methods */

/* Find viewpad which contains this view */
static XfdashboardViewpad* _xfdashboard_view_find_viewpad(XfdashboardView *self)
{
	ClutterActor				*viewpad;

	g_return_val_if_fail(XFDASHBOARD_IS_VIEW(self), NULL);

	/* Iterate through parent actors and for each viewpad found
	 * check if it contains this view.
	 */
	viewpad=clutter_actor_get_parent(CLUTTER_ACTOR(self));
	while(viewpad)
	{
		/* Check if this parent actor is a viewpad and
		 * if it contains this view.
		 */
		if(XFDASHBOARD_IS_VIEWPAD(viewpad) &&
			xfdashboard_viewpad_has_view(XFDASHBOARD_VIEWPAD(viewpad), self))
		{
			/* Viewpad found so return it */
			return(XFDASHBOARD_VIEWPAD(viewpad));
		}

		/* Continue with next parent actor */
		viewpad=clutter_actor_get_parent(viewpad);
	}

	/* If we get here the viewpad could not be found so return NULL */
	return(NULL);
}

/* Action signal to close currently selected window was emitted */
static gboolean _xfdashboard_view_activate(XfdashboardView *self,
											XfdashboardFocusable *inSource,
											const gchar *inAction,
											ClutterEvent *inEvent)
{
	XfdashboardViewPrivate		*priv;
	XfdashboardViewpad			*viewpad;
	XfdashboardFocusManager		*focusManager;

	g_return_val_if_fail(XFDASHBOARD_IS_VIEW(self), CLUTTER_EVENT_PROPAGATE);

	priv=self->priv;

	/* Only enabled views can be activated */
	if(!priv->isEnabled) return(CLUTTER_EVENT_STOP);

	/* Find viewpad which contains this view */
	viewpad=_xfdashboard_view_find_viewpad(self);
	if(!viewpad) return(CLUTTER_EVENT_STOP);

	/* Activate view at viewpad if this view is not the active one */
	if(xfdashboard_viewpad_get_active_view(viewpad)!=self)
	{
		xfdashboard_viewpad_set_active_view(viewpad, self);
	}

	/* Set focus to view if it has not the focus */
	focusManager=xfdashboard_focus_manager_get_default();
	if(XFDASHBOARD_IS_FOCUSABLE(self) &&
		!xfdashboard_focus_manager_has_focus(focusManager, XFDASHBOARD_FOCUSABLE(self)))
	{
		xfdashboard_focus_manager_set_focus(focusManager, XFDASHBOARD_FOCUSABLE(self));
	}
	g_object_unref(focusManager);

	/* Action handled */
	return(CLUTTER_EVENT_STOP);
}

/* This view was enabled */
static void _xfdashboard_view_enabled(XfdashboardView *self)
{
	XfdashboardViewpad			*viewpad;

	g_return_if_fail(XFDASHBOARD_IS_VIEW(self));

	/* If view is not a child of a viewpad show view actor directly */
	viewpad=_xfdashboard_view_find_viewpad(self);
	if(!viewpad)
	{
		clutter_actor_show(CLUTTER_ACTOR(self));
	}
}

/* This view was disabled */
static void _xfdashboard_view_disabled(XfdashboardView *self)
{
	XfdashboardViewpad			*viewpad;

	g_return_if_fail(XFDASHBOARD_IS_VIEW(self));

	/* If view is not a child of a viewpad hide view actor directly */
	viewpad=_xfdashboard_view_find_viewpad(self);
	if(!viewpad)
	{
		clutter_actor_hide(CLUTTER_ACTOR(self));
	}
}

/* Set view ID */
static void _xfdashboard_view_set_id(XfdashboardView *self, const gchar *inID)
{
	XfdashboardViewPrivate	*priv;

	g_return_if_fail(XFDASHBOARD_IS_VIEW(self));
	g_return_if_fail(inID && *inID);

	priv=self->priv;

	/* Set value if changed */
	if(g_strcmp0(priv->viewID, inID)!=0)
	{
		if(priv->viewID) g_free(priv->viewID);
		priv->viewID=g_strdup(inID);

		/* Notify about property change */
		g_object_notify_by_pspec(G_OBJECT(self), XfdashboardViewProperties[PROP_VIEW_ID]);
	}
}

/* IMPLEMENTATION: GObject */

/* Dispose this object */
static void _xfdashboard_view_dispose(GObject *inObject)
{
	XfdashboardView			*self=XFDASHBOARD_VIEW(inObject);
	XfdashboardViewPrivate	*priv=self->priv;

	/* Release allocated resources */
	if(priv->viewID)
	{
		g_free(priv->viewID);
		priv->viewID=NULL;
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
		case PROP_VIEW_ID:
			_xfdashboard_view_set_id(self, g_value_get_string(inValue));
			break;

		case PROP_VIEW_NAME:
			xfdashboard_view_set_name(self, g_value_get_string(inValue));
			break;
			
		case PROP_VIEW_ICON:
			xfdashboard_view_set_icon(self, g_value_get_string(inValue));
			break;

		case PROP_VIEW_FIT_MODE:
			xfdashboard_view_set_view_fit_mode(self, (XfdashboardViewFitMode)g_value_get_enum(inValue));
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
		case PROP_VIEW_ID:
			g_value_set_string(outValue, self->priv->viewID);
			break;

		case PROP_VIEW_NAME:
			g_value_set_string(outValue, self->priv->viewName);
			break;

		case PROP_VIEW_ICON:
			g_value_set_string(outValue, self->priv->viewIcon);
			break;

		case PROP_VIEW_FIT_MODE:
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
	XfdashboardActorClass	*actorClass=XFDASHBOARD_ACTOR_CLASS(klass);
	GObjectClass			*gobjectClass=G_OBJECT_CLASS(klass);

	/* Override functions */
	gobjectClass->set_property=_xfdashboard_view_set_property;
	gobjectClass->get_property=_xfdashboard_view_get_property;
	gobjectClass->dispose=_xfdashboard_view_dispose;

	klass->view_activate=_xfdashboard_view_activate;
	klass->enabled=_xfdashboard_view_enabled;
	klass->disabled=_xfdashboard_view_disabled;

	/* Define properties */
	XfdashboardViewProperties[PROP_VIEW_ID]=
		g_param_spec_string("view-id",
							_("View ID"),
							_("The internal ID used to register this type of view"),
							NULL,
							G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS | G_PARAM_CONSTRUCT_ONLY);

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

	XfdashboardViewProperties[PROP_VIEW_FIT_MODE]=
		g_param_spec_enum("view-fit-mode",
							_("View fit mode"),
							_("Defines if view should be fit into viewpad and in which directions it should fit into it"),
							XFDASHBOARD_TYPE_VIEW_FIT_MODE,
							XFDASHBOARD_VIEW_FIT_MODE_NONE,
							G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS);

	XfdashboardViewProperties[PROP_ENABLED]=
		g_param_spec_boolean("enabled",
								_("Enabled"),
								_("This flag indicates if is view is enabled and activable"),
								TRUE,
								G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS);

	g_object_class_install_properties(gobjectClass, PROP_LAST, XfdashboardViewProperties);

	/* Define stylable properties */
	xfdashboard_actor_install_stylable_property(actorClass, XfdashboardViewProperties[PROP_VIEW_ICON]);

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
						G_STRUCT_OFFSET(XfdashboardViewClass, deactivated),
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

	XfdashboardViewSignals[SIGNAL_CHILD_NEEDS_SCROLL]=
		g_signal_new("child-needs-scroll",
						G_TYPE_FROM_CLASS(klass),
						G_SIGNAL_RUN_LAST | G_SIGNAL_ACTION,
						G_STRUCT_OFFSET(XfdashboardViewClass, child_needs_scroll),
						NULL,
						NULL,
						_xfdashboard_marshal_BOOLEAN__OBJECT,
						G_TYPE_BOOLEAN,
						1,
						CLUTTER_TYPE_ACTOR);

	XfdashboardViewSignals[SIGNAL_CHILD_ENSURE_VISIBLE]=
		g_signal_new("child-ensure-visible",
						G_TYPE_FROM_CLASS(klass),
						G_SIGNAL_RUN_LAST | G_SIGNAL_ACTION,
						G_STRUCT_OFFSET(XfdashboardViewClass, child_ensure_visible),
						NULL,
						NULL,
						g_cclosure_marshal_VOID__OBJECT,
						G_TYPE_NONE,
						1,
						CLUTTER_TYPE_ACTOR);

	/* Define actions */
	XfdashboardViewSignals[ACTION_VIEW_ACTIVATE]=
		g_signal_new("view-activate",
						G_TYPE_FROM_CLASS(klass),
						G_SIGNAL_RUN_LAST | G_SIGNAL_ACTION,
						G_STRUCT_OFFSET(XfdashboardViewClass, view_activate),
						g_signal_accumulator_true_handled,
						NULL,
						_xfdashboard_marshal_BOOLEAN__OBJECT_STRING_BOXED,
						G_TYPE_BOOLEAN,
						3,
						XFDASHBOARD_TYPE_FOCUSABLE,
						G_TYPE_STRING,
						CLUTTER_TYPE_EVENT);
}

/* Object initialization
 * Create private structure and set up default values
 */
static void xfdashboard_view_init(XfdashboardView *self)
{
	XfdashboardViewPrivate	*priv;

	priv=self->priv=xfdashboard_view_get_instance_private(self);

	/* Set up default values */
	priv->viewName=NULL;
	priv->viewIcon=NULL;
	priv->viewIconImage=NULL;
	priv->fitMode=XFDASHBOARD_VIEW_FIT_MODE_NONE;
	priv->isEnabled=TRUE;

	/* Set up actor */
	clutter_actor_set_reactive(CLUTTER_ACTOR(self), TRUE);
	xfdashboard_actor_invalidate(XFDASHBOARD_ACTOR(self));
}

/* IMPLEMENTATION: Public API */

/* Get view ID */
const gchar* xfdashboard_view_get_id(XfdashboardView *self)
{
	g_return_val_if_fail(XFDASHBOARD_IS_VIEW(self), NULL);

	return(self->priv->viewID);
}

/* Check if view has requested ID */
gboolean xfdashboard_view_has_id(XfdashboardView *self, const gchar *inID)
{
	XfdashboardViewPrivate	*priv;

	g_return_val_if_fail(XFDASHBOARD_IS_VIEW(self), FALSE);
	g_return_val_if_fail(inID && *inID, FALSE);

	priv=self->priv;

	/* Check if requested ID matches the ID of this view */
	if(g_strcmp0(priv->viewID, inID)!=0) return(FALSE);

	/* If we get here the requested ID matches view's ID */
	return(TRUE);
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
		priv->viewIconImage=xfdashboard_image_content_new_for_icon_name(priv->viewIcon, 64.0f);

		/* Notify about property change */
		g_object_notify_by_pspec(G_OBJECT(self), XfdashboardViewProperties[PROP_VIEW_ICON]);
		g_signal_emit(self, XfdashboardViewSignals[SIGNAL_ICON_CHANGED], 0, CLUTTER_IMAGE(priv->viewIconImage));
	}
}

/* Get/set fit mode of view */
XfdashboardViewFitMode xfdashboard_view_get_view_fit_mode(XfdashboardView *self)
{
  g_return_val_if_fail(XFDASHBOARD_IS_VIEW(self), XFDASHBOARD_VIEW_FIT_MODE_NONE);

  return(self->priv->fitMode);
}

void xfdashboard_view_set_view_fit_mode(XfdashboardView *self, XfdashboardViewFitMode inFitMode)
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
		if(klass->set_view_fit_mode) klass->set_view_fit_mode(self, inFitMode);

		/* Notify about property change */
		g_object_notify_by_pspec(G_OBJECT(self), XfdashboardViewProperties[PROP_VIEW_FIT_MODE]);
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

/* Determine if scrolling is needed to get requested actor visible */
gboolean xfdashboard_view_child_needs_scroll(XfdashboardView *self, ClutterActor *inActor)
{
	gboolean			result;

	g_return_val_if_fail(XFDASHBOARD_IS_VIEW(self), FALSE);
	g_return_val_if_fail(CLUTTER_IS_ACTOR(inActor), FALSE);

	result=FALSE;

	/* Only emit signal if given actor is a child of this view */
	if(clutter_actor_contains(CLUTTER_ACTOR(self), inActor))
	{
		g_signal_emit(self, XfdashboardViewSignals[SIGNAL_CHILD_NEEDS_SCROLL], 0, inActor, &result);
	}

	return(result);
}

/* Ensure that a child of this view is visible by scrolling if needed */
void xfdashboard_view_child_ensure_visible(XfdashboardView *self, ClutterActor *inActor)
{
	g_return_if_fail(XFDASHBOARD_IS_VIEW(self));
	g_return_if_fail(CLUTTER_IS_ACTOR(inActor));

	/* Only emit signal if given actor is a child of this view */
	if(clutter_actor_contains(CLUTTER_ACTOR(self), inActor))
	{
		g_signal_emit(self, XfdashboardViewSignals[SIGNAL_CHILD_ENSURE_VISIBLE], 0, inActor);
	}
}

/* Determine if view has the focus */
gboolean xfdashboard_view_has_focus(XfdashboardView *self)
{
	XfdashboardViewPrivate		*priv;
	XfdashboardFocusManager		*focusManager;
	XfdashboardViewpad			*viewpad;

	g_return_val_if_fail(XFDASHBOARD_IS_VIEW(self), FALSE);

	priv=self->priv;

	/* The view can only have the focus if this view is enabled, active and
	 * has the current focus.
	 */
	if(!priv->isEnabled)
	{
		return(FALSE);
	}

	viewpad=_xfdashboard_view_find_viewpad(self);
	if(!viewpad)
	{
		return(FALSE);
	}

	if(xfdashboard_viewpad_get_active_view(XFDASHBOARD_VIEWPAD(viewpad))!=self)
	{
		return(FALSE);
	}

	focusManager=xfdashboard_focus_manager_get_default();
	if(!XFDASHBOARD_IS_FOCUSABLE(self) ||
		!xfdashboard_focus_manager_has_focus(focusManager, XFDASHBOARD_FOCUSABLE(self)))
	{
		g_object_unref(focusManager);
		return(FALSE);
	}

	/* Release allocated resources */
	g_object_unref(focusManager);

	/* All tests passed so this view has the focus */
	return(TRUE);
}
