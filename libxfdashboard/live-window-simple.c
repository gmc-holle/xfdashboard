/*
 * live-window-simple: An actor showing the content of a window which will
 *                     be updated if changed and visible on active workspace.
 * 
 * Copyright 2012-2017 Stephan Haller <nomad@froevel.de>
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

#include <libxfdashboard/live-window-simple.h>

#include <glib/gi18n-lib.h>
// TODO: #include <clutter/clutter.h>
// TODO: #include <clutter/x11/clutter-x11.h>
// TODO: #include <gtk/gtk.h>
// TODO: #include <math.h>

// TODO: #include <libxfdashboard/button.h>
// TODO: #include <libxfdashboard/stage.h>
#include <libxfdashboard/click-action.h>
#include <libxfdashboard/window-content.h>
// TODO: #include <libxfdashboard/image-content.h>
#include <libxfdashboard/stylable.h>
#include <libxfdashboard/compat.h>


/* Define this class in GObject system */
G_DEFINE_TYPE(XfdashboardLiveWindowSimple,
				xfdashboard_live_window_simple,
				XFDASHBOARD_TYPE_BACKGROUND)

/* Private structure - access only by public API if needed */
#define XFDASHBOARD_LIVE_WINDOW_SIMPLE_GET_PRIVATE(obj)                        \
	(G_TYPE_INSTANCE_GET_PRIVATE((obj), XFDASHBOARD_TYPE_LIVE_WINDOW_SIMPLE, XfdashboardLiveWindowSimplePrivate))

struct _XfdashboardLiveWindowSimplePrivate
{
	/* Properties related */
	XfdashboardWindowTrackerWindow		*window;

	/* Instance related */
	XfdashboardWindowTracker			*windowTracker;

	gboolean							isVisible;

	ClutterActor						*actorWindow;
};

/* Properties */
enum
{
	PROP_0,

	PROP_WINDOW,

	PROP_LAST
};

static GParamSpec* XfdashboardLiveWindowSimpleProperties[PROP_LAST]={ 0, };

/* Signals */
enum
{
	SIGNAL_CLICKED,

	SIGNAL_GEOMETRY_CHANGED,
	SIGNAL_VISIBILITY_CHANGED,
	SIGNAL_WORKSPACE_CHANGED,

	SIGNAL_LAST
};

static guint XfdashboardLiveWindowSimpleSignals[SIGNAL_LAST]={ 0, };

/* IMPLEMENTATION: Private variables and methods */

/* Check if window should be shown */
static gboolean _xfdashboard_live_window_simple_is_visible_window(XfdashboardLiveWindowSimple *self, XfdashboardWindowTrackerWindow *inWindow)
{
	g_return_val_if_fail(XFDASHBOARD_IS_WINDOW_TRACKER_WINDOW(inWindow), FALSE);

	/* Determine if windows should be shown depending on its state */
	if(xfdashboard_window_tracker_window_is_skip_pager(inWindow) ||
		xfdashboard_window_tracker_window_is_skip_tasklist(inWindow))
	{
		return(FALSE);
	}

	/* If we get here the window should be shown */
	return(TRUE);
}

/* This actor was clicked */
static void _xfdashboard_live_window_simple_on_clicked(XfdashboardLiveWindowSimple *self,
														ClutterActor *inActor,
														gpointer inUserData)
{
	XfdashboardClickAction				*action;

	g_return_if_fail(XFDASHBOARD_IS_LIVE_WINDOW_SIMPLE(self));
	g_return_if_fail(CLUTTER_IS_ACTOR(inActor));
	g_return_if_fail(XFDASHBOARD_IS_CLICK_ACTION(inUserData));

	action=XFDASHBOARD_CLICK_ACTION(inUserData);

	/* Only emit any of these signals if click was perform with left button */
	if(xfdashboard_click_action_get_button(action)!=XFDASHBOARD_CLICK_ACTION_LEFT_BUTTON) return;

	/* Emit "clicked" signal */
	g_signal_emit(self, XfdashboardLiveWindowSimpleSignals[SIGNAL_CLICKED], 0);
}

/* Position and/or size of window has changed */
static void _xfdashboard_live_window_simple_on_geometry_changed(XfdashboardLiveWindowSimple *self,
																XfdashboardWindowTrackerWindow *inWindow,
																gpointer inUserData)
{
	XfdashboardLiveWindowSimplePrivate	*priv;

	g_return_if_fail(XFDASHBOARD_IS_LIVE_WINDOW_SIMPLE(self));
	g_return_if_fail(XFDASHBOARD_IS_WINDOW_TRACKER_WINDOW(inWindow));

	priv=self->priv;

	/* Check if signal is for this window */
	if(inWindow!=priv->window) return;

	/* Actor's allocation may change because of new geometry so relayout */
	clutter_actor_queue_relayout(CLUTTER_ACTOR(self));

	/* Emit "geometry-changed" signal */
	g_signal_emit(self, XfdashboardLiveWindowSimpleSignals[SIGNAL_GEOMETRY_CHANGED], 0);
}

/* Window's state has changed */
static void _xfdashboard_live_window_simple_on_state_changed(XfdashboardLiveWindowSimple *self,
																XfdashboardWindowTrackerWindow *inWindow,
																gpointer inUserData)
{
	XfdashboardLiveWindowSimplePrivate	*priv;
	gboolean							isVisible;

	g_return_if_fail(XFDASHBOARD_IS_LIVE_WINDOW_SIMPLE(self));
	g_return_if_fail(XFDASHBOARD_IS_WINDOW_TRACKER_WINDOW(inWindow));

	priv=self->priv;

	/* Check if signal is for this window */
	if(inWindow!=priv->window) return;

	/* Check if window's visibility has changed */
	isVisible=_xfdashboard_live_window_simple_is_visible_window(self, inWindow);
	if(priv->isVisible!=isVisible)
	{
		priv->isVisible=isVisible;
		g_signal_emit(self, XfdashboardLiveWindowSimpleSignals[SIGNAL_VISIBILITY_CHANGED], 0);
	}

	/* Add or remove class depending on 'pinned' window state */
	if(xfdashboard_window_tracker_window_is_pinned(inWindow))
	{
		xfdashboard_stylable_add_class(XFDASHBOARD_STYLABLE(self), "window-state-pinned");
	}
		else
		{
			xfdashboard_stylable_remove_class(XFDASHBOARD_STYLABLE(self), "window-state-pinned");
		}

	/* Add or remove class depending on 'minimized' window state */
	if(xfdashboard_window_tracker_window_is_minimized(inWindow))
	{
		xfdashboard_stylable_add_class(XFDASHBOARD_STYLABLE(self), "window-state-minimized");
	}
		else
		{
			xfdashboard_stylable_remove_class(XFDASHBOARD_STYLABLE(self), "window-state-minimized");
		}

	/* Add or remove class depending on 'maximized' window state */
	if(xfdashboard_window_tracker_window_is_maximized(inWindow))
	{
		xfdashboard_stylable_add_class(XFDASHBOARD_STYLABLE(self), "window-state-maximized");
	}
		else
		{
			xfdashboard_stylable_remove_class(XFDASHBOARD_STYLABLE(self), "window-state-maximized");
		}

	/* Add or remove class depending on 'urgent' window state */
	if(xfdashboard_window_tracker_window_is_urgent(inWindow))
	{
		xfdashboard_stylable_add_class(XFDASHBOARD_STYLABLE(self), "window-state-urgent");
	}
		else
		{
			xfdashboard_stylable_remove_class(XFDASHBOARD_STYLABLE(self), "window-state-urgent");
		}
}

/* Window's workspace has changed */
static void _xfdashboard_live_window_simple_on_workspace_changed(XfdashboardLiveWindowSimple *self,
																	XfdashboardWindowTrackerWindow *inWindow,
																	gpointer inUserData)
{
	XfdashboardLiveWindowSimplePrivate	*priv;

	g_return_if_fail(XFDASHBOARD_IS_LIVE_WINDOW_SIMPLE(self));
	g_return_if_fail(XFDASHBOARD_IS_WINDOW_TRACKER_WINDOW(inWindow));

	priv=self->priv;

	/* Check if signal is for this window */
	if(inWindow!=priv->window) return;

	/* Emit "workspace-changed" signal */
	g_signal_emit(self, XfdashboardLiveWindowSimpleSignals[SIGNAL_WORKSPACE_CHANGED], 0);
}

/* IMPLEMENTATION: ClutterActor */

/* Get preferred width/height */
static void _xfdashboard_live_window_simple_get_preferred_height(ClutterActor *self,
																	gfloat inForWidth,
																	gfloat *outMinHeight,
																	gfloat *outNaturalHeight)
{
	XfdashboardLiveWindowSimplePrivate	*priv=XFDASHBOARD_LIVE_WINDOW_SIMPLE(self)->priv;
	gfloat								minHeight, naturalHeight;
	gfloat								childNaturalHeight;
	ClutterContent						*content;

	minHeight=naturalHeight=0.0f;

	/* Determine size of window if available and visible (should usually be the largest actor) */
	if(priv->actorWindow &&
		clutter_actor_is_visible(priv->actorWindow))
	{
		content=clutter_actor_get_content(priv->actorWindow);
		if(content &&
			XFDASHBOARD_IS_WINDOW_CONTENT(content) &&
			clutter_content_get_preferred_size(content, NULL, &childNaturalHeight))
		{
			if(childNaturalHeight>minHeight) minHeight=childNaturalHeight;
			if(childNaturalHeight>naturalHeight) naturalHeight=childNaturalHeight;
		}
	}

	/* Store sizes computed */
	if(outMinHeight) *outMinHeight=minHeight;
	if(outNaturalHeight) *outNaturalHeight=naturalHeight;
}

static void _xfdashboard_live_window_simple_get_preferred_width(ClutterActor *self,
																gfloat inForHeight,
																gfloat *outMinWidth,
																gfloat *outNaturalWidth)
{
	XfdashboardLiveWindowSimplePrivate	*priv=XFDASHBOARD_LIVE_WINDOW_SIMPLE(self)->priv;
	gfloat								minWidth, naturalWidth;
	gfloat								childNaturalWidth;
	ClutterContent						*content;

	minWidth=naturalWidth=0.0f;

	/* Determine size of window if available and visible (should usually be the largest actor) */
	if(priv->actorWindow &&
		clutter_actor_is_visible(priv->actorWindow))
	{
		content=clutter_actor_get_content(priv->actorWindow);
		if(content &&
			XFDASHBOARD_IS_WINDOW_CONTENT(content) &&
			clutter_content_get_preferred_size(content, &childNaturalWidth, NULL))
		{
			if(childNaturalWidth>minWidth) minWidth=childNaturalWidth;
			if(childNaturalWidth>naturalWidth) naturalWidth=childNaturalWidth;
		}
	}

	/* Store sizes computed */
	if(outMinWidth) *outMinWidth=minWidth;
	if(outNaturalWidth) *outNaturalWidth=naturalWidth;
}

/* Allocate position and size of actor and its children */
static void _xfdashboard_live_window_simple_allocate(ClutterActor *self,
														const ClutterActorBox *inBox,
														ClutterAllocationFlags inFlags)
{
	XfdashboardLiveWindowSimplePrivate	*priv=XFDASHBOARD_LIVE_WINDOW_SIMPLE(self)->priv;
	ClutterActorBox						*boxActorWindow=NULL;

	/* Chain up to store the allocation of the actor */
	CLUTTER_ACTOR_CLASS(xfdashboard_live_window_simple_parent_class)->allocate(self, inBox, inFlags);

	/* Set allocation on window texture */
	boxActorWindow=clutter_actor_box_copy(inBox);
	clutter_actor_box_set_origin(boxActorWindow, 0.0f, 0.0f);
	clutter_actor_allocate(priv->actorWindow, boxActorWindow, inFlags);

	/* Release allocated resources */
	if(boxActorWindow) clutter_actor_box_free(boxActorWindow);
}

/* IMPLEMENTATION: GObject */

/* Dispose this object */
static void _xfdashboard_live_window_simple_dispose(GObject *inObject)
{
	XfdashboardLiveWindowSimple			*self=XFDASHBOARD_LIVE_WINDOW_SIMPLE(inObject);
	XfdashboardLiveWindowSimplePrivate	*priv=self->priv;

	/* Dispose allocated resources */
	if(priv->window)
	{
		priv->window=NULL;
	}

	if(priv->windowTracker)
	{
		g_signal_handlers_disconnect_by_data(priv->windowTracker, self);
		g_object_unref(priv->windowTracker);
		priv->windowTracker=NULL;
	}

	if(priv->actorWindow)
	{
		clutter_actor_destroy(priv->actorWindow);
		priv->actorWindow=NULL;
	}

	/* Call parent's class dispose method */
	G_OBJECT_CLASS(xfdashboard_live_window_simple_parent_class)->dispose(inObject);
}

/* Set/get properties */
static void _xfdashboard_live_window_simple_set_property(GObject *inObject,
													guint inPropID,
													const GValue *inValue,
													GParamSpec *inSpec)
{
	XfdashboardLiveWindowSimple			*self=XFDASHBOARD_LIVE_WINDOW_SIMPLE(inObject);
	
	switch(inPropID)
	{
		case PROP_WINDOW:
			xfdashboard_live_window_simple_set_window(self, g_value_get_object(inValue));
			break;

		default:
			G_OBJECT_WARN_INVALID_PROPERTY_ID(inObject, inPropID, inSpec);
			break;
	}
}

static void _xfdashboard_live_window_simple_get_property(GObject *inObject,
													guint inPropID,
													GValue *outValue,
													GParamSpec *inSpec)
{
	XfdashboardLiveWindowSimple	*self=XFDASHBOARD_LIVE_WINDOW_SIMPLE(inObject);

	switch(inPropID)
	{
		case PROP_WINDOW:
			g_value_set_object(outValue, self->priv->window);
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
static void xfdashboard_live_window_simple_class_init(XfdashboardLiveWindowSimpleClass *klass)
{
	ClutterActorClass		*clutterActorClass=CLUTTER_ACTOR_CLASS(klass);
	GObjectClass			*gobjectClass=G_OBJECT_CLASS(klass);

	/* Override functions */
	clutterActorClass->get_preferred_width=_xfdashboard_live_window_simple_get_preferred_width;
	clutterActorClass->get_preferred_height=_xfdashboard_live_window_simple_get_preferred_height;
	clutterActorClass->allocate=_xfdashboard_live_window_simple_allocate;

	gobjectClass->dispose=_xfdashboard_live_window_simple_dispose;
	gobjectClass->set_property=_xfdashboard_live_window_simple_set_property;
	gobjectClass->get_property=_xfdashboard_live_window_simple_get_property;

	/* Set up private structure */
	g_type_class_add_private(klass, sizeof(XfdashboardLiveWindowSimplePrivate));

	/* Define properties */
	XfdashboardLiveWindowSimpleProperties[PROP_WINDOW]=
		g_param_spec_object("window",
								_("Window"),
								_("The window to show"),
								XFDASHBOARD_TYPE_WINDOW_TRACKER_WINDOW,
								G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS);

	g_object_class_install_properties(gobjectClass, PROP_LAST, XfdashboardLiveWindowSimpleProperties);

	/* Define signals */
	XfdashboardLiveWindowSimpleSignals[SIGNAL_CLICKED]=
		g_signal_new("clicked",
						G_TYPE_FROM_CLASS(klass),
						G_SIGNAL_RUN_LAST,
						G_STRUCT_OFFSET(XfdashboardLiveWindowSimpleClass, clicked),
						NULL,
						NULL,
						g_cclosure_marshal_VOID__VOID,
						G_TYPE_NONE,
						0);


	XfdashboardLiveWindowSimpleSignals[SIGNAL_GEOMETRY_CHANGED]=
		g_signal_new("geometry-changed",
						G_TYPE_FROM_CLASS(klass),
						G_SIGNAL_RUN_LAST,
						G_STRUCT_OFFSET(XfdashboardLiveWindowSimpleClass, geometry_changed),
						NULL,
						NULL,
						g_cclosure_marshal_VOID__VOID,
						G_TYPE_NONE,
						0);

	XfdashboardLiveWindowSimpleSignals[SIGNAL_VISIBILITY_CHANGED]=
		g_signal_new("visibility-changed",
						G_TYPE_FROM_CLASS(klass),
						G_SIGNAL_RUN_LAST,
						G_STRUCT_OFFSET(XfdashboardLiveWindowSimpleClass, visibility_changed),
						NULL,
						NULL,
						g_cclosure_marshal_VOID__BOOLEAN,
						G_TYPE_NONE,
						1,
						G_TYPE_BOOLEAN);

	XfdashboardLiveWindowSimpleSignals[SIGNAL_WORKSPACE_CHANGED]=
		g_signal_new("workspace-changed",
						G_TYPE_FROM_CLASS(klass),
						G_SIGNAL_RUN_LAST,
						G_STRUCT_OFFSET(XfdashboardLiveWindowSimpleClass, workspace_changed),
						NULL,
						NULL,
						g_cclosure_marshal_VOID__VOID,
						G_TYPE_NONE,
						0);
}

/* Object initialization
 * Create private structure and set up default values
 */
static void xfdashboard_live_window_simple_init(XfdashboardLiveWindowSimple *self)
{
	XfdashboardLiveWindowSimplePrivate	*priv;
	ClutterAction						*action;

	priv=self->priv=XFDASHBOARD_LIVE_WINDOW_SIMPLE_GET_PRIVATE(self);

	/* This actor reacts on events */
	clutter_actor_set_reactive(CLUTTER_ACTOR(self), TRUE);

	/* Set default values */
	priv->windowTracker=xfdashboard_window_tracker_get_default();
	priv->window=NULL;

	/* Set up child actors (order is important) */
	priv->actorWindow=clutter_actor_new();
	clutter_actor_show(priv->actorWindow);
	clutter_actor_add_child(CLUTTER_ACTOR(self), priv->actorWindow);

	/* Connect signals */
	action=xfdashboard_click_action_new();
	clutter_actor_add_action(CLUTTER_ACTOR(self), action);
	g_signal_connect_swapped(action, "clicked", G_CALLBACK(_xfdashboard_live_window_simple_on_clicked), self);

	g_signal_connect_swapped(priv->windowTracker, "window-geometry-changed", G_CALLBACK(_xfdashboard_live_window_simple_on_geometry_changed), self);
	g_signal_connect_swapped(priv->windowTracker, "window-state-changed", G_CALLBACK(_xfdashboard_live_window_simple_on_state_changed), self);
	g_signal_connect_swapped(priv->windowTracker, "window-workspace-changed", G_CALLBACK(_xfdashboard_live_window_simple_on_workspace_changed), self);
}

/* IMPLEMENTATION: Public API */

/* Create new instance */
ClutterActor* xfdashboard_live_window_simple_new(void)
{
	return(CLUTTER_ACTOR(g_object_new(XFDASHBOARD_TYPE_LIVE_WINDOW_SIMPLE, NULL)));
}

ClutterActor* xfdashboard_live_window_simple_new_for_window(XfdashboardWindowTrackerWindow *inWindow)
{
	g_return_val_if_fail(XFDASHBOARD_IS_WINDOW_TRACKER_WINDOW(inWindow), NULL);

	return(CLUTTER_ACTOR(g_object_new(XFDASHBOARD_TYPE_LIVE_WINDOW_SIMPLE,
										"window", inWindow,
										NULL)));
}

/* Get/set window to show */
XfdashboardWindowTrackerWindow* xfdashboard_live_window_simple_get_window(XfdashboardLiveWindowSimple *self)
{
	g_return_val_if_fail(XFDASHBOARD_IS_LIVE_WINDOW_SIMPLE(self), NULL);

	return(self->priv->window);
}

void xfdashboard_live_window_simple_set_window(XfdashboardLiveWindowSimple *self, XfdashboardWindowTrackerWindow *inWindow)
{
	XfdashboardLiveWindowSimplePrivate	*priv;
	ClutterContent					*content;

	g_return_if_fail(XFDASHBOARD_IS_LIVE_WINDOW_SIMPLE(self));
	g_return_if_fail(XFDASHBOARD_IS_WINDOW_TRACKER_WINDOW(inWindow));

	priv=self->priv;

	/* Only set value if it changes */
	if(inWindow==priv->window) return;

	/* Release old value */
	if(priv->window)
	{
		g_signal_handlers_disconnect_by_data(priv->window, self);
		priv->window=NULL;
	}

	/* Set new value
	 * Window tracker objects should never be refed or unrefed, so just set new value
	 */
	priv->window=inWindow;
	priv->isVisible=_xfdashboard_live_window_simple_is_visible_window(self, priv->window);

	/* Setup window actor */
	content=xfdashboard_window_content_new_for_window(priv->window);
	clutter_actor_set_content(priv->actorWindow, content);
	g_object_unref(content);

	/* Set up this actor and child actor by calling each signal handler now */
	_xfdashboard_live_window_simple_on_geometry_changed(self, priv->window, priv->windowTracker);
	_xfdashboard_live_window_simple_on_state_changed(self, priv->window, priv->windowTracker);
	_xfdashboard_live_window_simple_on_workspace_changed(self, priv->window, priv->windowTracker);

	/* Notify about property change */
	g_object_notify_by_pspec(G_OBJECT(self), XfdashboardLiveWindowSimpleProperties[PROP_WINDOW]);
}
