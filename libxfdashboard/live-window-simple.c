/*
 * live-window-simple: An actor showing the content of a window which will
 *                     be updated if changed and visible on active workspace.
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

#include <libxfdashboard/live-window-simple.h>

#include <glib/gi18n-lib.h>

#include <libxfdashboard/click-action.h>
#include <libxfdashboard/window-content.h>
#include <libxfdashboard/image-content.h>
#include <libxfdashboard/stylable.h>
#include <libxfdashboard/enums.h>
#include <libxfdashboard/compat.h>
#include <libxfdashboard/debug.h>


/* Define this class in GObject system */
struct _XfdashboardLiveWindowSimplePrivate
{
	/* Properties related */
	XfdashboardWindowTrackerWindow			*window;
	XfdashboardLiveWindowSimpleDisplayType	displayType;

	/* Instance related */
	gboolean								isVisible;
	ClutterActor							*actorWindow;
	gboolean								destroyOnClose;
};

G_DEFINE_TYPE_WITH_PRIVATE(XfdashboardLiveWindowSimple,
							xfdashboard_live_window_simple,
							XFDASHBOARD_TYPE_BACKGROUND)

/* Properties */
enum
{
	PROP_0,

	PROP_WINDOW,
	PROP_DISPLAY_TYPE,
	PROP_DESTROY_ON_CLOSE,

	PROP_LAST
};

static GParamSpec* XfdashboardLiveWindowSimpleProperties[PROP_LAST]={ 0, };

/* Signals */
enum
{
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
	XfdashboardWindowTrackerWindowState		state;

	g_return_val_if_fail(XFDASHBOARD_IS_WINDOW_TRACKER_WINDOW(inWindow), FALSE);

	/* Determine if windows should be shown depending on its state */
	state=xfdashboard_window_tracker_window_get_state(inWindow);
	if((state & XFDASHBOARD_WINDOW_TRACKER_WINDOW_STATE_SKIP_PAGER) ||
		(state & XFDASHBOARD_WINDOW_TRACKER_WINDOW_STATE_SKIP_TASKLIST))
	{
		return(FALSE);
	}

	/* If we get here the window should be shown */
	return(TRUE);
}

/* Position and/or size of window has changed */
static void _xfdashboard_live_window_simple_on_geometry_changed(XfdashboardLiveWindowSimple *self,
																gpointer inUserData)
{
	XfdashboardLiveWindowSimplePrivate	*priv;
	XfdashboardWindowTrackerWindow		*window;

	g_return_if_fail(XFDASHBOARD_IS_LIVE_WINDOW_SIMPLE(self));
	g_return_if_fail(XFDASHBOARD_IS_WINDOW_TRACKER_WINDOW(inUserData));

	priv=self->priv;
	window=XFDASHBOARD_WINDOW_TRACKER_WINDOW(inUserData);

	/* Check if signal is for this window */
	if(window!=priv->window) return;

	/* Actor's allocation may change because of new geometry so relayout */
	clutter_actor_queue_relayout(CLUTTER_ACTOR(self));

	/* Emit "geometry-changed" signal */
	g_signal_emit(self, XfdashboardLiveWindowSimpleSignals[SIGNAL_GEOMETRY_CHANGED], 0);
}

/* Window's state has changed */
static void _xfdashboard_live_window_simple_on_state_changed(XfdashboardLiveWindowSimple *self,
																XfdashboardWindowTrackerWindowState inOldState,
																gpointer inUserData)
{
	XfdashboardLiveWindowSimplePrivate		*priv;
	XfdashboardWindowTrackerWindow			*window;
	gboolean								isVisible;
	XfdashboardWindowTrackerWindowState		state;

	g_return_if_fail(XFDASHBOARD_IS_LIVE_WINDOW_SIMPLE(self));
	g_return_if_fail(XFDASHBOARD_IS_WINDOW_TRACKER_WINDOW(inUserData));

	priv=self->priv;
	window=XFDASHBOARD_WINDOW_TRACKER_WINDOW(inUserData);

	/* Check if signal is for this window */
	if(window!=priv->window) return;

	/* Check if window's visibility has changed */
	isVisible=_xfdashboard_live_window_simple_is_visible_window(self, window);
	if(priv->isVisible!=isVisible)
	{
		priv->isVisible=isVisible;
		g_signal_emit(self, XfdashboardLiveWindowSimpleSignals[SIGNAL_VISIBILITY_CHANGED], 0);
	}

	/* Add or remove class depending on 'pinned' window state */
	state=xfdashboard_window_tracker_window_get_state(window);
	if(state & XFDASHBOARD_WINDOW_TRACKER_WINDOW_STATE_PINNED)
	{
		xfdashboard_stylable_add_class(XFDASHBOARD_STYLABLE(self), "window-state-pinned");
	}
		else
		{
			xfdashboard_stylable_remove_class(XFDASHBOARD_STYLABLE(self), "window-state-pinned");
		}

	/* Add or remove class depending on 'minimized' window state */
	if(state & XFDASHBOARD_WINDOW_TRACKER_WINDOW_STATE_MINIMIZED)
	{
		xfdashboard_stylable_add_class(XFDASHBOARD_STYLABLE(self), "window-state-minimized");
	}
		else
		{
			xfdashboard_stylable_remove_class(XFDASHBOARD_STYLABLE(self), "window-state-minimized");
		}

	/* Add or remove class depending on 'maximized' window state */
	if(state & XFDASHBOARD_WINDOW_TRACKER_WINDOW_STATE_MAXIMIZED)
	{
		xfdashboard_stylable_add_class(XFDASHBOARD_STYLABLE(self), "window-state-maximized");
	}
		else
		{
			xfdashboard_stylable_remove_class(XFDASHBOARD_STYLABLE(self), "window-state-maximized");
		}

	/* Add or remove class depending on 'urgent' window state */
	if(state & XFDASHBOARD_WINDOW_TRACKER_WINDOW_STATE_URGENT)
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
																	XfdashboardWindowTrackerWorkspace *inWorkspace,
																	gpointer inUserData)
{
	XfdashboardLiveWindowSimplePrivate	*priv;
	XfdashboardWindowTrackerWindow		*window;

	g_return_if_fail(XFDASHBOARD_IS_LIVE_WINDOW_SIMPLE(self));
	g_return_if_fail(!inWorkspace || XFDASHBOARD_IS_WINDOW_TRACKER_WORKSPACE(inWorkspace));
	g_return_if_fail(XFDASHBOARD_IS_WINDOW_TRACKER_WINDOW(inUserData));

	priv=self->priv;
	window=XFDASHBOARD_WINDOW_TRACKER_WINDOW(inUserData);

	/* Check if signal is for this window */
	if(window!=priv->window) return;

	/* Emit "workspace-changed" signal */
	g_signal_emit(self, XfdashboardLiveWindowSimpleSignals[SIGNAL_WORKSPACE_CHANGED], 0);
}

/* Window's was closed */
static void _xfdashboard_live_window_simple_on_closed(XfdashboardLiveWindowSimple *self,
														gpointer inUserData)
{
	XfdashboardLiveWindowSimplePrivate	*priv;
	XfdashboardWindowTrackerWindow		*window;

	g_return_if_fail(XFDASHBOARD_IS_LIVE_WINDOW_SIMPLE(self));
	g_return_if_fail(XFDASHBOARD_IS_WINDOW_TRACKER_WINDOW(inUserData));

	priv=self->priv;
	window=XFDASHBOARD_WINDOW_TRACKER_WINDOW(inUserData);

	/* Check if signal is for this window */
	if(window!=priv->window) return;

	/* Check if actor should be destroy when window was closed */
	if(priv->destroyOnClose)
	{
		XFDASHBOARD_DEBUG(self, WINDOWS,
							"Window '%s' was closed and auto-destruction of actor was requested",
							xfdashboard_window_tracker_window_get_name(priv->window));
		clutter_actor_destroy(CLUTTER_ACTOR(self));
	}
}

/* Set up actor's content depending on display. If no window is set the current
 * content of this actor is destroyed and a new one is not set up. The actor will
 * be displayed empty.
 */
static void _xfdashboard_live_window_simple_setup_content(XfdashboardLiveWindowSimple *self)
{
	XfdashboardLiveWindowSimplePrivate	*priv;
	ClutterContent						*content;

	g_return_if_fail(XFDASHBOARD_IS_LIVE_WINDOW_SIMPLE(self));

	priv=self->priv;

	/* Destroy old actor's content */
	clutter_actor_set_content(priv->actorWindow, NULL);

	/* If no window is set we cannot set up actor's content but only destroy the
	 * old. So return here if no window is set.
	 */
	if(!priv->window) return;

	/* Setup actor's content depending on display type */
	switch(priv->displayType)
	{
		case XFDASHBOARD_LIVE_WINDOW_SIMPLE_DISPLAY_TYPE_LIVE_PREVIEW:
			content=xfdashboard_window_tracker_window_get_content(priv->window);
			clutter_actor_set_content(priv->actorWindow, content);
			g_object_unref(content);
			break;

		case XFDASHBOARD_LIVE_WINDOW_SIMPLE_DISPLAY_TYPE_ICON:
			content=xfdashboard_image_content_new_for_pixbuf(xfdashboard_window_tracker_window_get_icon(priv->window));
			clutter_actor_set_content(priv->actorWindow, content);
			g_object_unref(content);
			break;

		default:
			g_assert_not_reached();
			break;
	}
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

	minHeight=naturalHeight=0.0f;

	/* Determine size of window if available and visible (should usually be the largest actor) */
	if(priv->actorWindow &&
		clutter_actor_is_visible(priv->actorWindow) &&
		priv->window)
	{
		gint							windowHeight;

		xfdashboard_window_tracker_window_get_geometry(priv->window, NULL, NULL, NULL, &windowHeight);
		if(windowHeight<=0.0)
		{
			ClutterContent				*content;
			gfloat						childNaturalHeight;

			/* If we get here getting window size failed so fallback to old
			 * behaviour by getting size of window content associated to actor.
			 */
			content=clutter_actor_get_content(priv->actorWindow);
			if(content &&
				XFDASHBOARD_IS_WINDOW_CONTENT(content))
			{
				if(clutter_content_get_preferred_size(content, NULL, &childNaturalHeight)) windowHeight=childNaturalHeight;
				XFDASHBOARD_DEBUG(self, WINDOWS,
									"Using fallback method to determine preferred height for window '%s'",
									xfdashboard_window_tracker_window_get_name(priv->window));
			}
		}

		if(windowHeight>minHeight) minHeight=windowHeight;
		if(windowHeight>naturalHeight) naturalHeight=windowHeight;
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

	minWidth=naturalWidth=0.0f;

	/* Determine size of window if available and visible (should usually be the largest actor) */
	if(priv->actorWindow &&
		clutter_actor_is_visible(priv->actorWindow) &&
		priv->window)
	{
		gint							windowWidth;

		xfdashboard_window_tracker_window_get_geometry(priv->window, NULL, NULL, &windowWidth, NULL);
		if(windowWidth<=0.0)
		{
			ClutterContent				*content;
			gfloat						childNaturalWidth;

			/* If we get here getting window size failed so fallback to old
			 * behaviour by getting size of window content associated to actor.
			 */
			content=clutter_actor_get_content(priv->actorWindow);
			if(content &&
				XFDASHBOARD_IS_WINDOW_CONTENT(content))
			{
				if(clutter_content_get_preferred_size(content, &childNaturalWidth, NULL)) windowWidth=childNaturalWidth;
				XFDASHBOARD_DEBUG(self, WINDOWS,
									"Using fallback method to determine preferred width for window '%s'",
									xfdashboard_window_tracker_window_get_name(priv->window));
			}
		}

		if(windowWidth>minWidth) minWidth=windowWidth;
		if(windowWidth>naturalWidth) naturalWidth=windowWidth;
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
		g_signal_handlers_disconnect_by_data(priv->window, self);
		priv->window=NULL;
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

		case PROP_DISPLAY_TYPE:
			xfdashboard_live_window_simple_set_display_type(self, g_value_get_enum(inValue));
			break;

		case PROP_DESTROY_ON_CLOSE:
			xfdashboard_live_window_simple_set_destroy_on_close(self, g_value_get_boolean(inValue));
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

		case PROP_DISPLAY_TYPE:
			g_value_set_enum(outValue, self->priv->displayType);
			break;

		case PROP_DESTROY_ON_CLOSE:
			g_value_set_boolean(outValue, self->priv->destroyOnClose);
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
	XfdashboardActorClass	*actorClass=XFDASHBOARD_ACTOR_CLASS(klass);
	ClutterActorClass		*clutterActorClass=CLUTTER_ACTOR_CLASS(klass);
	GObjectClass			*gobjectClass=G_OBJECT_CLASS(klass);

	/* Override functions */
	clutterActorClass->get_preferred_width=_xfdashboard_live_window_simple_get_preferred_width;
	clutterActorClass->get_preferred_height=_xfdashboard_live_window_simple_get_preferred_height;
	clutterActorClass->allocate=_xfdashboard_live_window_simple_allocate;

	gobjectClass->dispose=_xfdashboard_live_window_simple_dispose;
	gobjectClass->set_property=_xfdashboard_live_window_simple_set_property;
	gobjectClass->get_property=_xfdashboard_live_window_simple_get_property;

	/* Define properties */
	XfdashboardLiveWindowSimpleProperties[PROP_WINDOW]=
		g_param_spec_object("window",
								_("Window"),
								_("The window to show"),
								XFDASHBOARD_TYPE_WINDOW_TRACKER_WINDOW,
								G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS);

	XfdashboardLiveWindowSimpleProperties[PROP_DISPLAY_TYPE]=
		g_param_spec_enum("display-type",
							_("Display type"),
							_("How to display the window"),
							XFDASHBOARD_TYPE_LIVE_WINDOW_SIMPLE_DISPLAY_TYPE,
							XFDASHBOARD_LIVE_WINDOW_SIMPLE_DISPLAY_TYPE_LIVE_PREVIEW,
							G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS);

	XfdashboardLiveWindowSimpleProperties[PROP_DESTROY_ON_CLOSE]=
		g_param_spec_boolean("destroy-on-close",
								_("Destroy on close"),
								_("If this actor should be destroy when window was closed"),
								TRUE,
								G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS);

	g_object_class_install_properties(gobjectClass, PROP_LAST, XfdashboardLiveWindowSimpleProperties);

	/* Define stylable properties */
	xfdashboard_actor_install_stylable_property(actorClass, XfdashboardLiveWindowSimpleProperties[PROP_DISPLAY_TYPE]);

	/* Define signals */
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

	priv=self->priv=xfdashboard_live_window_simple_get_instance_private(self);

	/* This actor reacts on events */
	clutter_actor_set_reactive(CLUTTER_ACTOR(self), TRUE);

	/* Set default values */
	priv->window=NULL;
	priv->displayType=XFDASHBOARD_LIVE_WINDOW_SIMPLE_DISPLAY_TYPE_LIVE_PREVIEW;
	priv->destroyOnClose=TRUE;
	

	/* Set up child actors (order is important) */
	priv->actorWindow=clutter_actor_new();
	clutter_actor_show(priv->actorWindow);
	clutter_actor_add_child(CLUTTER_ACTOR(self), priv->actorWindow);
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

	g_return_if_fail(XFDASHBOARD_IS_LIVE_WINDOW_SIMPLE(self));
	g_return_if_fail(!inWindow || XFDASHBOARD_IS_WINDOW_TRACKER_WINDOW(inWindow));

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
	if(priv->window)
	{
		/* Get visibility state of window */
		priv->isVisible=_xfdashboard_live_window_simple_is_visible_window(self, priv->window);

		/* Setup window actor content */
		_xfdashboard_live_window_simple_setup_content(self);

		/* Set up this actor and child actor by calling each signal handler now */
		_xfdashboard_live_window_simple_on_geometry_changed(self, priv->window);
		_xfdashboard_live_window_simple_on_state_changed(self, 0, priv->window);
		_xfdashboard_live_window_simple_on_workspace_changed(self, NULL, priv->window);

		/* Connect signal handlers */
		g_signal_connect_swapped(priv->window, "geometry-changed", G_CALLBACK(_xfdashboard_live_window_simple_on_geometry_changed), self);
		g_signal_connect_swapped(priv->window, "state-changed", G_CALLBACK(_xfdashboard_live_window_simple_on_state_changed), self);
		g_signal_connect_swapped(priv->window, "workspace-changed", G_CALLBACK(_xfdashboard_live_window_simple_on_workspace_changed), self);
		g_signal_connect_swapped(priv->window, "closed", G_CALLBACK(_xfdashboard_live_window_simple_on_closed), self);
	}
		else
		{
			/* Clean window actor */
			clutter_actor_set_content(priv->actorWindow, NULL);

			/* Set window to invisible as NULL window is no window */
			priv->isVisible=FALSE;
		}

	/* Notify about property change */
	g_object_notify_by_pspec(G_OBJECT(self), XfdashboardLiveWindowSimpleProperties[PROP_WINDOW]);
}

/* Get/set display type of window */
XfdashboardLiveWindowSimpleDisplayType xfdashboard_live_window_simple_get_display_type(XfdashboardLiveWindowSimple *self)
{
	g_return_val_if_fail(XFDASHBOARD_IS_LIVE_WINDOW_SIMPLE(self), XFDASHBOARD_LIVE_WINDOW_SIMPLE_DISPLAY_TYPE_LIVE_PREVIEW);

	return(self->priv->displayType);
}

void xfdashboard_live_window_simple_set_display_type(XfdashboardLiveWindowSimple *self, XfdashboardLiveWindowSimpleDisplayType inType)
{
	XfdashboardLiveWindowSimplePrivate	*priv;

	g_return_if_fail(XFDASHBOARD_IS_LIVE_WINDOW_SIMPLE(self));
	g_return_if_fail(inType>=XFDASHBOARD_LIVE_WINDOW_SIMPLE_DISPLAY_TYPE_LIVE_PREVIEW && inType<=XFDASHBOARD_LIVE_WINDOW_SIMPLE_DISPLAY_TYPE_ICON);

	priv=self->priv;

	/* Only set value if it changes */
	if(priv->displayType!=inType)
	{
		/* Set value */
		priv->displayType=inType;

		/* Setup window actor content */
		_xfdashboard_live_window_simple_setup_content(self);

		/* Notify about property change */
		g_object_notify_by_pspec(G_OBJECT(self), XfdashboardLiveWindowSimpleProperties[PROP_DISPLAY_TYPE]);
	}
}

/* Get/set flag for destruction on window close */
gboolean xfdashboard_live_window_simple_get_destroy_on_close(XfdashboardLiveWindowSimple *self)
{
	g_return_val_if_fail(XFDASHBOARD_IS_LIVE_WINDOW_SIMPLE(self), FALSE);

	return(self->priv->destroyOnClose);
}

void xfdashboard_live_window_simple_set_destroy_on_close(XfdashboardLiveWindowSimple *self, gboolean inDestroyOnClose)
{
	XfdashboardLiveWindowSimplePrivate	*priv;

	g_return_if_fail(XFDASHBOARD_IS_LIVE_WINDOW_SIMPLE(self));

	priv=self->priv;

	/* Only set value if it changes */
	if(priv->destroyOnClose!=inDestroyOnClose)
	{
		/* Set value */
		priv->destroyOnClose=inDestroyOnClose;

		/* Notify about property change */
		g_object_notify_by_pspec(G_OBJECT(self), XfdashboardLiveWindowSimpleProperties[PROP_DESTROY_ON_CLOSE]);
	}
}
