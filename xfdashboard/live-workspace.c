/*
 * live-workspace: An actor showing the content of a workspace which will
 *                 be updated if changed.
 * 
 * Copyright 2012-2015 Stephan Haller <nomad@froevel.de>
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

#include "live-workspace.h"

#include <glib/gi18n-lib.h>
#include <clutter/clutter.h>
#include <clutter/x11/clutter-x11.h>
#include <math.h>

#include "button.h"
#include "live-window.h"
#include "window-tracker.h"
#include "click-action.h"
#include "window-content.h"

/* Define this class in GObject system */
G_DEFINE_TYPE(XfdashboardLiveWorkspace,
				xfdashboard_live_workspace,
				XFDASHBOARD_TYPE_BACKGROUND)

/* Private structure - access only by public API if needed */
#define XFDASHBOARD_LIVE_WORKSPACE_GET_PRIVATE(obj) \
	(G_TYPE_INSTANCE_GET_PRIVATE((obj), XFDASHBOARD_TYPE_LIVE_WORKSPACE, XfdashboardLiveWorkspacePrivate))

struct _XfdashboardLiveWorkspacePrivate
{
	/* Properties related */
	XfdashboardWindowTrackerWorkspace	*workspace;

	/* Instance related */
	XfdashboardWindowTracker			*windowTracker;
};

/* Properties */
enum
{
	PROP_0,

	PROP_WORKSPACE,

	PROP_LAST
};

static GParamSpec* XfdashboardLiveWorkspaceProperties[PROP_LAST]={ 0, };

/* Signals */
enum
{
	SIGNAL_CLICKED,

	SIGNAL_LAST
};

static guint XfdashboardLiveWorkspaceSignals[SIGNAL_LAST]={ 0, };

/* IMPLEMENTATION: Private variables and methods */

/* Check if window should be shown */
static gboolean _xfdashboard_live_workspace_is_visible_window(XfdashboardLiveWorkspace *self,
																XfdashboardWindowTrackerWindow *inWindow)
{
	XfdashboardLiveWorkspacePrivate		*priv;

	g_return_val_if_fail(XFDASHBOARD_IS_LIVE_WORKSPACE(self), FALSE);
	g_return_val_if_fail(XFDASHBOARD_IS_WINDOW_TRACKER_WINDOW(inWindow), FALSE);

	priv=self->priv;

	/* Determine if windows should be shown depending on its state */
	if(xfdashboard_window_tracker_window_is_skip_pager(inWindow) ||
		xfdashboard_window_tracker_window_is_skip_tasklist(inWindow) ||
		(priv->workspace && !xfdashboard_window_tracker_window_is_visible_on_workspace(inWindow, priv->workspace)) ||
		xfdashboard_window_tracker_window_is_stage(inWindow))
	{
		return(FALSE);
	}

	/* If we get here the window should be shown */
	return(TRUE);
}

/* Find live window actor by window */
static ClutterActor* _xfdashboard_live_workspace_find_by_window(XfdashboardLiveWorkspace *self,
																XfdashboardWindowTrackerWindow *inWindow)
{
	ClutterActor						*child;
	ClutterActorIter					iter;
	ClutterContent						*content;
	XfdashboardWindowTrackerWindow		*window;

	g_return_val_if_fail(XFDASHBOARD_IS_LIVE_WORKSPACE(self), NULL);
	g_return_val_if_fail(XFDASHBOARD_IS_WINDOW_TRACKER_WINDOW(inWindow), NULL);

	/* Iterate through list of current actors and find the one for requested window */
	clutter_actor_iter_init(&iter, CLUTTER_ACTOR(self));
	while(clutter_actor_iter_next(&iter, &child))
	{
		content=clutter_actor_get_content(child);
		if(!content || !XFDASHBOARD_IS_WINDOW_CONTENT(content)) continue;

		window=xfdashboard_window_content_get_window(XFDASHBOARD_WINDOW_CONTENT(content));
		if(window==inWindow) return(child);
	}

	/* If we get here we did not find the window and we return NULL */
	return(NULL);
}

/* Create actor for window but respect window stacking when adding */
static ClutterActor* _xfdashboard_live_workspace_create_and_add_window_actor(XfdashboardLiveWorkspace *self,
																				XfdashboardWindowTrackerWindow *inWindow)
{
	XfdashboardLiveWorkspacePrivate	*priv;
	ClutterActor					*actor;
	ClutterContent					*content;
	GList							*windows;
	ClutterActor					*lastWindowActor;
	XfdashboardWindowTrackerWindow	*window;

	g_return_val_if_fail(XFDASHBOARD_IS_LIVE_WORKSPACE(self), NULL);
	g_return_val_if_fail(XFDASHBOARD_IS_WINDOW_TRACKER_WINDOW(inWindow), NULL);

	priv=self->priv;

	/* We cannot assume that each window newly opened or moved to this workspace
	 * will be on top of all other windows. We need to respect window stacking.
	 * Therefore we iterate through list of windows in stacking order and find
	 * the last window we have an actor for before we the window requested.
	 */
	lastWindowActor=NULL;
	windows=xfdashboard_window_tracker_get_windows_stacked(priv->windowTracker);
	for( ; windows; windows=g_list_next(windows))
	{
		/* Get window from list */
		window=XFDASHBOARD_WINDOW_TRACKER_WINDOW(windows->data);
		if(!window) continue;

		/* We do not need to check if window would be visible on this workspace
		 * as it should not have been created if it is not visible.
		 */
		lastWindowActor=_xfdashboard_live_workspace_find_by_window(self, window);
		if(lastWindowActor) break;
	}

	/* Check if we have to "move" an existing window actor or if we have to create
	 * a new actor for window
	 */
	actor=_xfdashboard_live_workspace_find_by_window(self, inWindow);
	if(actor)
	{
		/* Move existing window actor to new stacking position */
		g_object_ref(actor);
		clutter_actor_remove_child(CLUTTER_ACTOR(self), actor);
		clutter_actor_insert_child_above(CLUTTER_ACTOR(self), actor, lastWindowActor);
		g_object_unref(actor);
	}
		else
		{
			/* Create actor */
			actor=clutter_actor_new();
			content=xfdashboard_window_content_new_for_window(inWindow);
			clutter_actor_set_content(actor, content);
			g_object_unref(content);

			/* Add new actor at right stacking position */
			clutter_actor_insert_child_above(CLUTTER_ACTOR(self), actor, lastWindowActor);
		}

	return(actor);
}

/* This actor was clicked */
static void _xfdashboard_live_workspace_on_clicked(XfdashboardLiveWorkspace *self, ClutterActor *inActor, gpointer inUserData)
{
	g_return_if_fail(XFDASHBOARD_IS_LIVE_WORKSPACE(self));

	/* Emit "clicked" signal */
	g_signal_emit(self, XfdashboardLiveWorkspaceSignals[SIGNAL_CLICKED], 0);
}

/* A window was closed */
static void _xfdashboard_live_workspace_on_window_closed(XfdashboardLiveWorkspace *self,
															XfdashboardWindowTrackerWindow *inWindow,
															gpointer inUserData)
{
	ClutterActor						*windowActor;

	g_return_if_fail(XFDASHBOARD_IS_LIVE_WORKSPACE(self));
	g_return_if_fail(XFDASHBOARD_IS_WINDOW_TRACKER_WINDOW(inWindow));

	/* Find and destroy actor */
	windowActor=_xfdashboard_live_workspace_find_by_window(self, inWindow);
	if(windowActor) clutter_actor_destroy(windowActor);
}

/* A window was opened */
static void _xfdashboard_live_workspace_on_window_opened(XfdashboardLiveWorkspace *self,
															XfdashboardWindowTrackerWindow *inWindow,
															gpointer inUserData)
{
	g_return_if_fail(XFDASHBOARD_IS_LIVE_WORKSPACE(self));
	g_return_if_fail(XFDASHBOARD_IS_WINDOW_TRACKER_WINDOW(inWindow));

	/* Check if window is visible on this workspace */
	if(!_xfdashboard_live_workspace_is_visible_window(self, inWindow)) return;

	/* Create actor for window */
	_xfdashboard_live_workspace_create_and_add_window_actor(self, inWindow);
}

/* A window's position and/or size has changed */
static void _xfdashboard_live_workspace_on_window_geometry_changed(XfdashboardLiveWorkspace *self,
																	XfdashboardWindowTrackerWindow *inWindow,
																	gpointer inUserData)
{
	g_return_if_fail(XFDASHBOARD_IS_LIVE_WORKSPACE(self));
	g_return_if_fail(XFDASHBOARD_IS_WINDOW_TRACKER_WINDOW(inWindow));

	/* Actor's allocation may change because of new geometry so relayout */
	clutter_actor_queue_relayout(CLUTTER_ACTOR(self));
}

/* Window stacking has changed */
static void _xfdashboard_live_workspace_on_window_stacking_changed(XfdashboardLiveWorkspace *self,
																	gpointer inUserData)
{
	XfdashboardLiveWorkspacePrivate		*priv;
	GList								*windows;
	XfdashboardWindowTrackerWindow		*window;
	ClutterActor						*actor;

	g_return_if_fail(XFDASHBOARD_IS_LIVE_WORKSPACE(self));

	priv=self->priv;

	/* Get stacked order of windows */
	windows=xfdashboard_window_tracker_get_windows_stacked(priv->windowTracker);

	/* Iterate through list of stacked window from beginning to end
	 * and reinsert each window found to bottom of this actor
	 */
	for( ; windows; windows=g_list_next(windows))
	{
		/* Get window and find corresponding actor */
		window=XFDASHBOARD_WINDOW_TRACKER_WINDOW(windows->data);
		if(!window) continue;

		actor=_xfdashboard_live_workspace_find_by_window(self, window);
		if(!actor) continue;

		/* If we get here the window actor was found so move to bottom */
		g_object_ref(actor);
		clutter_actor_remove_child(CLUTTER_ACTOR(self), actor);
		clutter_actor_insert_child_above(CLUTTER_ACTOR(self), actor, NULL);
		g_object_unref(actor);
	}
}

/* A window's state has changed */
static void _xfdashboard_live_workspace_on_window_state_changed(XfdashboardLiveWorkspace *self,
																XfdashboardWindowTrackerWindow *inWindow,
																gpointer inUserData)
{
	/* We need to see it from the point of view of a workspace.
	 * If a window is visible on the workspace but we have no actor
	 * for this window then create it. If a window is not visible anymore
	 * on this workspace then destroy the corresponding actor.
	 * That is why initially we set any window to invisible because if
	 * changed window is not visible on this workspace it will do nothing.
	 */

	ClutterActor		*windowActor;
	gboolean			newVisible;
	gboolean			currentVisible;

	g_return_if_fail(XFDASHBOARD_IS_LIVE_WORKSPACE(self));
	g_return_if_fail(XFDASHBOARD_IS_WINDOW_TRACKER_WINDOW(inWindow));

	currentVisible=FALSE;

	/* Find window and get current visibility state */
	windowActor=_xfdashboard_live_workspace_find_by_window(self, inWindow);
	if(windowActor)
	{
		currentVisible=!!CLUTTER_ACTOR_IS_VISIBLE(windowActor);
	}

	/* Check if window's visibility has changed */
	newVisible=_xfdashboard_live_workspace_is_visible_window(self, inWindow);
	if(newVisible!=currentVisible)
	{
		if(newVisible) _xfdashboard_live_workspace_create_and_add_window_actor(self, inWindow);
			else if(windowActor) clutter_actor_destroy(windowActor);
	}
}

/* A window's workspace has changed */
static void _xfdashboard_live_workspace_on_window_workspace_changed(XfdashboardLiveWorkspace *self,
																	XfdashboardWindowTrackerWindow *inWindow,
																	XfdashboardWindowTrackerWorkspace *inWorkspace,
																	gpointer inUserData)
{
	XfdashboardLiveWorkspacePrivate		*priv;
	ClutterActor						*windowActor;

	g_return_if_fail(XFDASHBOARD_IS_LIVE_WORKSPACE(self));
	g_return_if_fail(XFDASHBOARD_IS_WINDOW_TRACKER_WINDOW(inWindow));

	priv=self->priv;

	/* Check if window was removed from workspace or added */
	if(inWorkspace!=priv->workspace)
	{
		/* Find actor for window */
		windowActor=_xfdashboard_live_workspace_find_by_window(self, inWindow);

		/* Destroy window actor */
		if(windowActor) clutter_actor_destroy(windowActor);
	}
		else
		{
			/* Add window actor */
			_xfdashboard_live_workspace_create_and_add_window_actor(self, inWindow);
		}
}

/* IMPLEMENTATION: ClutterActor */

/* Get preferred width/height */
static void _xfdashboard_live_workspace_get_preferred_height(ClutterActor *self,
															gfloat inForWidth,
															gfloat *outMinHeight,
															gfloat *outNaturalHeight)
{
	XfdashboardLiveWorkspacePrivate		*priv=XFDASHBOARD_LIVE_WORKSPACE(self)->priv;
	gfloat								minHeight, naturalHeight;
	gfloat								childWidth, childHeight;

	minHeight=naturalHeight=0.0f;

	/* Determine size of workspace if available (should usually be the largest actor) */
	if(priv->workspace)
	{
		childWidth=(gfloat)xfdashboard_window_tracker_workspace_get_width(priv->workspace);
		childHeight=(gfloat)xfdashboard_window_tracker_workspace_get_height(priv->workspace);

		if(inForWidth<0.0f) naturalHeight=childHeight;
			else naturalHeight=(childHeight/childWidth)*inForWidth;
	}

	/* Store sizes computed */
	if(outMinHeight) *outMinHeight=minHeight;
	if(outNaturalHeight) *outNaturalHeight=naturalHeight;
}

static void _xfdashboard_live_workspace_get_preferred_width(ClutterActor *self,
															gfloat inForHeight,
															gfloat *outMinWidth,
															gfloat *outNaturalWidth)
{
	XfdashboardLiveWorkspacePrivate		*priv=XFDASHBOARD_LIVE_WORKSPACE(self)->priv;
	gfloat								minWidth, naturalWidth;
	gfloat								childWidth, childHeight;

	minWidth=naturalWidth=0.0f;

	/* Determine size of workspace if available (should usually be the largest actor) */
	if(priv->workspace)
	{
		childWidth=(gfloat)xfdashboard_window_tracker_workspace_get_width(priv->workspace);
		childHeight=(gfloat)xfdashboard_window_tracker_workspace_get_height(priv->workspace);

		if(inForHeight<0.0f) naturalWidth=childWidth;
			else naturalWidth=(childWidth/childHeight)*inForHeight;
	}

	/* Store sizes computed */
	if(outMinWidth) *outMinWidth=minWidth;
	if(outNaturalWidth) *outNaturalWidth=naturalWidth;
}

/* Allocate position and size of actor and its children */
static void _xfdashboard_live_workspace_allocate(ClutterActor *self,
												const ClutterActorBox *inBox,
												ClutterAllocationFlags inFlags)
{
	XfdashboardLiveWorkspacePrivate		*priv=XFDASHBOARD_LIVE_WORKSPACE(self)->priv;
	gfloat								availableWidth, availableHeight;
	gfloat								workspaceWidth, workspaceHeight;
	ClutterContent						*content;
	XfdashboardWindowTrackerWindow		*window;
	gint								x, y, w, h;
	ClutterActor						*child;
	ClutterActorIter					iter;
	ClutterActorBox						childAllocation={ 0, };

	/* Chain up to store the allocation of the actor */
	CLUTTER_ACTOR_CLASS(xfdashboard_live_workspace_parent_class)->allocate(self, inBox, inFlags);

	/* If we handle no workspace to not set allocation of children */
	if(!priv->workspace) return;

	/* Get size of workspace and this allocation as it is needed
	 * to calculate translated position and size
	 */
	clutter_actor_box_get_size(inBox, &availableWidth, &availableHeight);

	workspaceWidth=(gfloat)xfdashboard_window_tracker_workspace_get_width(priv->workspace);
	workspaceHeight=(gfloat)xfdashboard_window_tracker_workspace_get_height(priv->workspace);

	/* Iterate through window actors, calculate translated allocation of
	 * position and size to available size of this actor
	 */
	clutter_actor_iter_init(&iter, self);
	while(clutter_actor_iter_next(&iter, &child))
	{
		/* Get window actor */
		if(!CLUTTER_IS_ACTOR(child)) continue;

		/* Get associated window */
		content=clutter_actor_get_content(child);
		if(!content || !XFDASHBOARD_IS_WINDOW_CONTENT(content)) continue;

		window=xfdashboard_window_content_get_window(XFDASHBOARD_WINDOW_CONTENT(content));
		if(!window) continue;

		/* Get real size of child */
		xfdashboard_window_tracker_window_get_position_size(window, &x, &y, &w, &h);

		/* Calculate translated position and size of child */
		childAllocation.x1=ceil((x/workspaceWidth)*availableWidth);
		childAllocation.y1=ceil((y/workspaceHeight)*availableHeight);
		childAllocation.x2=childAllocation.x1+ceil((w/workspaceWidth)*availableWidth);
		childAllocation.y2=childAllocation.y1+ceil((h/workspaceHeight)*availableHeight);

		/* Set allocation of child */
		clutter_actor_allocate(child, &childAllocation, inFlags);
	}
}

/* IMPLEMENTATION: GObject */

/* Dispose this object */
static void _xfdashboard_live_workspace_dispose(GObject *inObject)
{
	XfdashboardLiveWorkspace			*self=XFDASHBOARD_LIVE_WORKSPACE(inObject);
	XfdashboardLiveWorkspacePrivate		*priv=self->priv;

	/* Dispose allocated resources */
	if(priv->windowTracker)
	{
		g_signal_handlers_disconnect_by_data(priv->windowTracker, self);
		g_object_unref(priv->windowTracker);
		priv->windowTracker=NULL;
	}

	if(priv->workspace)
	{
		g_signal_handlers_disconnect_by_data(priv->workspace, self);
		priv->workspace=NULL;
	}

	/* Call parent's class dispose method */
	G_OBJECT_CLASS(xfdashboard_live_workspace_parent_class)->dispose(inObject);
}

/* Set/get properties */
static void _xfdashboard_live_workspace_set_property(GObject *inObject,
													guint inPropID,
													const GValue *inValue,
													GParamSpec *inSpec)
{
	XfdashboardLiveWorkspace			*self=XFDASHBOARD_LIVE_WORKSPACE(inObject);
	
	switch(inPropID)
	{
		case PROP_WORKSPACE:
			xfdashboard_live_workspace_set_workspace(self, g_value_get_object(inValue));
			break;

		default:
			G_OBJECT_WARN_INVALID_PROPERTY_ID(inObject, inPropID, inSpec);
			break;
	}
}

static void _xfdashboard_live_workspace_get_property(GObject *inObject,
													guint inPropID,
													GValue *outValue,
													GParamSpec *inSpec)
{
	XfdashboardLiveWorkspace	*self=XFDASHBOARD_LIVE_WORKSPACE(inObject);

	switch(inPropID)
	{
		case PROP_WORKSPACE:
			g_value_set_object(outValue, self->priv->workspace);
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
static void xfdashboard_live_workspace_class_init(XfdashboardLiveWorkspaceClass *klass)
{
	ClutterActorClass	*actorClass=CLUTTER_ACTOR_CLASS(klass);
	GObjectClass		*gobjectClass=G_OBJECT_CLASS(klass);

	/* Override functions */
	actorClass->get_preferred_width=_xfdashboard_live_workspace_get_preferred_width;
	actorClass->get_preferred_height=_xfdashboard_live_workspace_get_preferred_height;
	actorClass->allocate=_xfdashboard_live_workspace_allocate;

	gobjectClass->dispose=_xfdashboard_live_workspace_dispose;
	gobjectClass->set_property=_xfdashboard_live_workspace_set_property;
	gobjectClass->get_property=_xfdashboard_live_workspace_get_property;

	/* Set up private structure */
	g_type_class_add_private(klass, sizeof(XfdashboardLiveWorkspacePrivate));

	/* Define properties */
	XfdashboardLiveWorkspaceProperties[PROP_WORKSPACE]=
		g_param_spec_object("workspace",
								_("Workspace"),
								_("The workspace to show"),
								XFDASHBOARD_TYPE_WINDOW_TRACKER_WORKSPACE,
								G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS);

	g_object_class_install_properties(gobjectClass, PROP_LAST, XfdashboardLiveWorkspaceProperties);

	/* Define signals */
	XfdashboardLiveWorkspaceSignals[SIGNAL_CLICKED]=
		g_signal_new("clicked",
						G_TYPE_FROM_CLASS(klass),
						G_SIGNAL_RUN_LAST,
						G_STRUCT_OFFSET(XfdashboardLiveWorkspaceClass, clicked),
						NULL,
						NULL,
						g_cclosure_marshal_VOID__VOID,
						G_TYPE_NONE,
						0);
}

/* Object initialization
 * Create private structure and set up default values
 */
static void xfdashboard_live_workspace_init(XfdashboardLiveWorkspace *self)
{
	XfdashboardLiveWorkspacePrivate		*priv;
	ClutterAction						*action;

	priv=self->priv=XFDASHBOARD_LIVE_WORKSPACE_GET_PRIVATE(self);

	/* Set default values */
	priv->windowTracker=xfdashboard_window_tracker_get_default();
	priv->workspace=NULL;

	/* Set up this actor */
	clutter_actor_set_reactive(CLUTTER_ACTOR(self), TRUE);

	/* Connect signals */
	action=xfdashboard_click_action_new();
	clutter_actor_add_action(CLUTTER_ACTOR(self), action);
	g_signal_connect_swapped(action, "clicked", G_CALLBACK(_xfdashboard_live_workspace_on_clicked), self);

	g_signal_connect_swapped(priv->windowTracker, "window-opened", G_CALLBACK(_xfdashboard_live_workspace_on_window_opened), self);
	g_signal_connect_swapped(priv->windowTracker, "window-closed", G_CALLBACK(_xfdashboard_live_workspace_on_window_closed), self);
	g_signal_connect_swapped(priv->windowTracker, "window-geometry-changed", G_CALLBACK(_xfdashboard_live_workspace_on_window_geometry_changed), self);
	g_signal_connect_swapped(priv->windowTracker, "window-state-changed", G_CALLBACK(_xfdashboard_live_workspace_on_window_state_changed), self);
	g_signal_connect_swapped(priv->windowTracker, "window-workspace-changed", G_CALLBACK(_xfdashboard_live_workspace_on_window_workspace_changed), self);
	g_signal_connect_swapped(priv->windowTracker, "window-stacking-changed", G_CALLBACK(_xfdashboard_live_workspace_on_window_stacking_changed), self);
}

/* IMPLEMENTATION: Public API */

/* Create new instance */
ClutterActor* xfdashboard_live_workspace_new(void)
{
	return(CLUTTER_ACTOR(g_object_new(XFDASHBOARD_TYPE_LIVE_WORKSPACE, NULL)));
}

ClutterActor* xfdashboard_live_workspace_new_for_workspace(XfdashboardWindowTrackerWorkspace *inWorkspace)
{
	g_return_val_if_fail(XFDASHBOARD_IS_WINDOW_TRACKER_WORKSPACE(inWorkspace), NULL);

	return(CLUTTER_ACTOR(g_object_new(XFDASHBOARD_TYPE_LIVE_WORKSPACE,
										"workspace", inWorkspace,
										NULL)));
}

/* Get/set window to show */
XfdashboardWindowTrackerWorkspace* xfdashboard_live_workspace_get_workspace(XfdashboardLiveWorkspace *self)
{
	g_return_val_if_fail(XFDASHBOARD_IS_LIVE_WORKSPACE(self), NULL);

	return(self->priv->workspace);
}

void xfdashboard_live_workspace_set_workspace(XfdashboardLiveWorkspace *self, XfdashboardWindowTrackerWorkspace *inWorkspace)
{
	XfdashboardLiveWorkspacePrivate		*priv;
	ClutterContent						*content;
	XfdashboardWindowTrackerWindow		*window;
	ClutterActor						*child;
	ClutterActorIter					iter;
	GList								*windows;
	ClutterActor						*actor;

	g_return_if_fail(XFDASHBOARD_IS_LIVE_WORKSPACE(self));
	g_return_if_fail(XFDASHBOARD_IS_WINDOW_TRACKER_WORKSPACE(inWorkspace));

	priv=self->priv;

	/* Only set value if it changes */
	if(inWorkspace==priv->workspace) return;

	/* Release old value */
	if(priv->workspace)
	{
		g_signal_handlers_disconnect_by_data(priv->workspace, self);
		priv->workspace=NULL;
	}

	/* Set new value
	 * Window tracker objects should never be refed or unrefed, so just set new value
	 */
	priv->workspace=inWorkspace;

	/* Destroy all window actors */
	clutter_actor_iter_init(&iter, CLUTTER_ACTOR(self));
	while(clutter_actor_iter_next(&iter, &child))
	{
		/* Get window actor */
		if(!CLUTTER_IS_ACTOR(child)) continue;

		/* Check if it is really a window actor by retrieving associated window */
		content=clutter_actor_get_content(child);
		if(!content || !XFDASHBOARD_IS_WINDOW_CONTENT(content)) continue;

		/* Destroy window actor */
		clutter_actor_destroy(child);
	}

	/* Create windows for new workspace in stacked order */
	windows=xfdashboard_window_tracker_get_windows_stacked(priv->windowTracker);
	for( ; windows; windows=g_list_next(windows))
	{
		/* Get window */
		window=XFDASHBOARD_WINDOW_TRACKER_WINDOW(windows->data);
		if(!window) continue;

		/* Create window actor if window is visible */
		if(!_xfdashboard_live_workspace_is_visible_window(self, window)) continue;

		actor=clutter_actor_new();
		content=xfdashboard_window_content_new_for_window(window);
		clutter_actor_set_content(actor, content);
		g_object_unref(content);

		/* Insert new actor at bottom */
		clutter_actor_insert_child_above(CLUTTER_ACTOR(self), actor, NULL);
	}

	/* Notify about property change */
	g_object_notify_by_pspec(G_OBJECT(self), XfdashboardLiveWorkspaceProperties[PROP_WORKSPACE]);
}
