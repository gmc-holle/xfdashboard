/*
 * workspace-selector: Workspace selector box
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

#include <libxfdashboard/workspace-selector.h>

#include <glib/gi18n-lib.h>
#include <math.h>

#include <libxfdashboard/enums.h>
#include <libxfdashboard/window-tracker.h>
#include <libxfdashboard/live-workspace.h>
#include <libxfdashboard/application.h>
#include <libxfdashboard/drop-action.h>
#include <libxfdashboard/windows-view.h>
#include <libxfdashboard/live-window.h>
#include <libxfdashboard/application-button.h>
#include <libxfdashboard/utils.h>
#include <libxfdashboard/stylable.h>
#include <libxfdashboard/focusable.h>
#include <libxfdashboard/stage-interface.h>
#include <libxfdashboard/compat.h>
#include <libxfdashboard/debug.h>


/* Define this class in GObject system */
static void _xfdashboard_workspace_selector_focusable_iface_init(XfdashboardFocusableInterface *iface);

struct _XfdashboardWorkspaceSelectorPrivate
{
	/* Properties related */
	gfloat								spacing;
	ClutterOrientation					orientation;
	gfloat								maxSize;
	gfloat								maxFraction;
	gboolean							usingFraction;
	gboolean							showCurrentMonitorOnly;

	/* Instance related */
	XfdashboardWindowTracker			*windowTracker;
	XfdashboardWindowTrackerWorkspace	*activeWorkspace;
};

G_DEFINE_TYPE_WITH_CODE(XfdashboardWorkspaceSelector,
						xfdashboard_workspace_selector,
						XFDASHBOARD_TYPE_BACKGROUND,
						G_ADD_PRIVATE(XfdashboardWorkspaceSelector)
						G_IMPLEMENT_INTERFACE(XFDASHBOARD_TYPE_FOCUSABLE, _xfdashboard_workspace_selector_focusable_iface_init))

/* Properties */
enum
{
	PROP_0,

	PROP_SPACING,

	PROP_ORIENTATION,

	PROP_MAX_SIZE,
	PROP_MAX_FRACTION,
	PROP_USING_FRACTION,

	PROP_SHOW_CURRENT_MONITOR_ONLY,

	PROP_LAST
};

static GParamSpec* XfdashboardWorkspaceSelectorProperties[PROP_LAST]={ 0, };

/* IMPLEMENTATION: Private variables and methods */
#define DEFAULT_MAX_SIZE			256.0f
#define DEFAULT_MAX_FRACTION		0.25f
#define DEFAULT_USING_FRACTION		TRUE
#define DEFAULT_ORIENTATION			CLUTTER_ORIENTATION_VERTICAL

/* Get maximum (horizontal or vertical) size either by static size or fraction */
static gfloat _xfdashboard_workspace_selector_get_max_size_internal(XfdashboardWorkspaceSelector *self)
{
	XfdashboardWorkspaceSelectorPrivate		*priv;
	XfdashboardStageInterface				*stageInterface;
	gfloat									w, h;
	gfloat									size, fraction;

	g_return_val_if_fail(XFDASHBOARD_IS_WORKSPACE_SELECTOR(self), 0.0f);

	priv=self->priv;

	/* Get size of monitor of stage interface where this actor is shown at
	 * to determine maximum size by fraction or to update maximum size or
	 * fraction and send notifications.
	 */
	stageInterface=xfdashboard_get_stage_of_actor(CLUTTER_ACTOR(self));
	if(!stageInterface) return(0.0f);

	clutter_actor_get_size(CLUTTER_ACTOR(stageInterface), &w, &h);

	/* If fraction should be used to determine maximum size get width or height
	 * of stage depending on orientation and calculate size by fraction
	 */
	if(priv->usingFraction)
	{
		/* Calculate size by fraction */
		if(priv->orientation==CLUTTER_ORIENTATION_HORIZONTAL) size=h*priv->maxFraction;
			else size=w*priv->maxFraction;

		/* Update maximum size if it has changed */
		if(priv->maxSize!=size)
		{
			priv->maxSize=size;

			/* Notify about property change */
			g_object_notify_by_pspec(G_OBJECT(self), XfdashboardWorkspaceSelectorProperties[PROP_MAX_SIZE]);
		}

		return(size);
	}

	/* Calculate fraction from size */
	if(priv->orientation==CLUTTER_ORIENTATION_HORIZONTAL) fraction=priv->maxSize/h;
		else fraction=priv->maxSize/w;

	/* Update maximum fraction if it has changed */
	if(priv->maxFraction!=fraction)
	{
		priv->maxFraction=fraction;

		/* Notify about property change */
		g_object_notify_by_pspec(G_OBJECT(self), XfdashboardWorkspaceSelectorProperties[PROP_MAX_FRACTION]);
	}

	/* Otherwise return static maximum size configured */
	return(priv->maxSize);
}

/* Find live workspace actor for native workspace */
static XfdashboardLiveWorkspace* _xfdashboard_workspace_selector_find_actor_for_workspace(XfdashboardWorkspaceSelector *self,
																							XfdashboardWindowTrackerWorkspace *inWorkspace)
{
	ClutterActorIter				iter;
	ClutterActor					*child;
	XfdashboardLiveWorkspace		*liveWorkspace;

	g_return_val_if_fail(XFDASHBOARD_IS_WORKSPACE_SELECTOR(self), NULL);
	g_return_val_if_fail(XFDASHBOARD_IS_WINDOW_TRACKER_WORKSPACE(inWorkspace), NULL);

	/* Iterate through workspace actor and lookup the one handling requesting workspace */
	clutter_actor_iter_init(&iter, CLUTTER_ACTOR(self));
	while(clutter_actor_iter_next(&iter, &child))
	{
		if(XFDASHBOARD_IS_LIVE_WORKSPACE(child))
		{
			liveWorkspace=XFDASHBOARD_LIVE_WORKSPACE(child);
			if(xfdashboard_live_workspace_get_workspace(liveWorkspace)==inWorkspace)
			{
				return(liveWorkspace);
			}
		}
	}

	return(NULL);
}

/* Get height of a child of this actor */
static void _xfdashboard_workspace_selector_get_preferred_height_for_child(XfdashboardWorkspaceSelector *self,
																			ClutterActor *inChild,
																			gfloat inForWidth,
																			gfloat *outMinHeight,
																			gfloat *outNaturalHeight)
{
	XfdashboardWorkspaceSelectorPrivate		*priv;
	gfloat									minHeight, naturalHeight;
	gfloat									maxSize;

	g_return_if_fail(XFDASHBOARD_IS_WORKSPACE_SELECTOR(self));

	priv=self->priv;

	/* Set up default values */
	minHeight=naturalHeight=0.0f;

	/* Determine height for horizontal orientation ... */
	if(priv->orientation==CLUTTER_ORIENTATION_HORIZONTAL)
	{
		/* Get height of child */
		clutter_actor_get_preferred_height(inChild, inForWidth, &minHeight, &naturalHeight);

		/* Adjust child's height to maximum height */
		maxSize=_xfdashboard_workspace_selector_get_max_size_internal(self)-(2*priv->spacing);
		if(maxSize>=0.0)
		{
			/* Adjust minimum width if it exceed limit */
			if(minHeight>maxSize) minHeight=maxSize;

			/* Adjust natural width if it exceed limit */
			if(naturalHeight>maxSize) naturalHeight=maxSize;
		}
	}
		/* ... otherwise determine height for vertical orientation */
		else
		{
			/* Adjust requested width to maximum width */
			maxSize=_xfdashboard_workspace_selector_get_max_size_internal(self)-(2*priv->spacing);
			if(maxSize>=0.0f && inForWidth>maxSize) inForWidth=maxSize;

			/* Get height of child */
			clutter_actor_get_preferred_height(inChild, inForWidth, &minHeight, &naturalHeight);
		}

	/* Store sizes computed */
	if(outMinHeight) *outMinHeight=minHeight;
	if(outNaturalHeight) *outNaturalHeight=naturalHeight;
}

/* Get width of a child of this actor */
static void _xfdashboard_workspace_selector_get_preferred_width_for_child(XfdashboardWorkspaceSelector *self,
																			ClutterActor *inChild,
																			gfloat inForHeight,
																			gfloat *outMinWidth,
																			gfloat *outNaturalWidth)
{
	XfdashboardWorkspaceSelectorPrivate		*priv;
	gfloat									minWidth, naturalWidth;
	gfloat									maxSize;

	g_return_if_fail(XFDASHBOARD_IS_WORKSPACE_SELECTOR(self));

	priv=self->priv;

	/* Set up default values */
	minWidth=naturalWidth=0.0f;

	/* Determine width for horizontal orientation ... */
	if(priv->orientation==CLUTTER_ORIENTATION_HORIZONTAL)
	{
		/* Adjust requested height to maximum height */
		maxSize=_xfdashboard_workspace_selector_get_max_size_internal(self)-(2*priv->spacing);
		if(maxSize>=0.0f && inForHeight>maxSize) inForHeight=maxSize;

		/* Get width of child */
		clutter_actor_get_preferred_width(inChild, inForHeight, &minWidth, &naturalWidth);
	}
		/* ... otherwise determine height for vertical orientation */
		else
		{
			/* Get width of child */
			clutter_actor_get_preferred_width(inChild, inForHeight, &minWidth, &naturalWidth);

			/* Adjust child's width to maximum width */
			maxSize=_xfdashboard_workspace_selector_get_max_size_internal(self)-(2*priv->spacing);
			if(maxSize>=0.0)
			{
				/* Adjust minimum width if it exceed limit */
				if(minWidth>maxSize) minWidth=maxSize;

				/* Adjust natural width if it exceed limit */
				if(naturalWidth>maxSize) naturalWidth=maxSize;
			}
		}

	/* Store sizes computed */
	if(outMinWidth) *outMinWidth=minWidth;
	if(outNaturalWidth) *outNaturalWidth=naturalWidth;
}

/* Drag of an actor to this view as drop target begins */
static gboolean _xfdashboard_workspace_selector_on_drop_begin(XfdashboardLiveWorkspace *self,
																XfdashboardDragAction *inDragAction,
																gpointer inUserData)
{
	ClutterActor					*dragSource;
	ClutterActor					*draggedActor;
	gboolean						canHandle;

	g_return_val_if_fail(XFDASHBOARD_IS_LIVE_WORKSPACE(self), FALSE);
	g_return_val_if_fail(XFDASHBOARD_IS_DRAG_ACTION(inDragAction), FALSE);
	g_return_val_if_fail(XFDASHBOARD_IS_DROP_ACTION(inUserData), FALSE);

	canHandle=FALSE;

	/* Get source where dragging started and actor being dragged */
	dragSource=xfdashboard_drag_action_get_source(inDragAction);
	draggedActor=xfdashboard_drag_action_get_actor(inDragAction);

	/* We can handle dragged actor if it is a live window and its source
	 * is windows view.
	 */
	if(XFDASHBOARD_IS_WINDOWS_VIEW(dragSource) &&
		XFDASHBOARD_IS_LIVE_WINDOW(draggedActor))
	{
		canHandle=TRUE;
	}

	/* We can handle dragged actor if it is a live window and its source
	 * is a live workspace
	 */
	if(XFDASHBOARD_IS_LIVE_WORKSPACE(dragSource) &&
		XFDASHBOARD_IS_LIVE_WINDOW_SIMPLE(draggedActor))
	{
		canHandle=TRUE;
	}

	/* We can handle dragged actor if it is an application button */
	if(XFDASHBOARD_IS_APPLICATION_BUTTON(draggedActor))
	{
		canHandle=TRUE;
	}

	/* Return TRUE if we can handle dragged actor in this drop target
	 * otherwise FALSE
	 */
	return(canHandle);
}

/* Dragged actor was dropped on this drop target */
static void _xfdashboard_workspace_selector_on_drop_drop(XfdashboardLiveWorkspace *self,
															XfdashboardDragAction *inDragAction,
															gfloat inX,
															gfloat inY,
															gpointer inUserData)
{
	ClutterActor						*draggedActor;

	g_return_if_fail(XFDASHBOARD_IS_LIVE_WORKSPACE(self));
	g_return_if_fail(XFDASHBOARD_IS_DRAG_ACTION(inDragAction));
	g_return_if_fail(XFDASHBOARD_IS_DROP_ACTION(inUserData));

	/* Get dragged actor */
	draggedActor=xfdashboard_drag_action_get_actor(inDragAction);

	/* Check if dragged actor is a window so move window to workspace */
	if(XFDASHBOARD_IS_LIVE_WINDOW_SIMPLE(draggedActor))
	{
		XfdashboardWindowTrackerWindow	*window;

		/* Get window */
		window=xfdashboard_live_window_simple_get_window(XFDASHBOARD_LIVE_WINDOW_SIMPLE(draggedActor));
		g_return_if_fail(window);

		/* Move window to workspace */
		xfdashboard_window_tracker_window_move_to_workspace(window, xfdashboard_live_workspace_get_workspace(self));
	}

	/* Check if dragged actor is a application button so launch app at workspace */
	if(XFDASHBOARD_IS_APPLICATION_BUTTON(draggedActor))
	{
		XfdashboardApplicationButton	*button;
		GAppLaunchContext				*context;

		/* Get application button */
		button=XFDASHBOARD_APPLICATION_BUTTON(draggedActor);

		/* Launch application at workspace where application button was dropped */
		context=xfdashboard_create_app_context(xfdashboard_live_workspace_get_workspace(self));
		xfdashboard_application_button_execute(button, context);
		g_object_unref(context);
	}
}

/* A live workspace was clicked */
static void _xfdashboard_workspace_selector_on_workspace_clicked(XfdashboardWorkspaceSelector *self,
																	gpointer inUserData)
{
	XfdashboardLiveWorkspace	*liveWorkspace;

	g_return_if_fail(XFDASHBOARD_IS_WORKSPACE_SELECTOR(self));
	g_return_if_fail(XFDASHBOARD_IS_LIVE_WORKSPACE(inUserData));

	liveWorkspace=XFDASHBOARD_LIVE_WORKSPACE(inUserData);

	/* Active workspace */
	xfdashboard_window_tracker_workspace_activate(xfdashboard_live_workspace_get_workspace(liveWorkspace));

	/* Quit application */
	xfdashboard_application_suspend_or_quit(NULL);
}

/* A workspace was destroyed */
static void _xfdashboard_workspace_selector_on_workspace_removed(XfdashboardWorkspaceSelector *self,
																	XfdashboardWindowTrackerWorkspace *inWorkspace,
																	gpointer inUserData)
{
	XfdashboardLiveWorkspace		*liveWorkspace;

	g_return_if_fail(XFDASHBOARD_IS_WORKSPACE_SELECTOR(self));
	g_return_if_fail(XFDASHBOARD_IS_WINDOW_TRACKER_WORKSPACE(inWorkspace));

	/* Iterate through children and find workspace to destroy */
	liveWorkspace=_xfdashboard_workspace_selector_find_actor_for_workspace(self, inWorkspace);
	if(liveWorkspace) clutter_actor_destroy(CLUTTER_ACTOR(liveWorkspace));
}

/* A workspace was created */
static void _xfdashboard_workspace_selector_on_workspace_added(XfdashboardWorkspaceSelector *self,
																XfdashboardWindowTrackerWorkspace *inWorkspace,
																gpointer inUserData)
{
	XfdashboardWorkspaceSelectorPrivate		*priv;
	ClutterActor							*actor;
	gint									index;
	ClutterAction							*action;

	g_return_if_fail(XFDASHBOARD_IS_WORKSPACE_SELECTOR(self));
	g_return_if_fail(XFDASHBOARD_IS_WINDOW_TRACKER_WORKSPACE(inWorkspace));

	priv=self->priv;

	/* Get index of workspace for insertion */
	index=xfdashboard_window_tracker_workspace_get_number(inWorkspace);

	/* Create new live workspace actor and insert at index */
	actor=xfdashboard_live_workspace_new_for_workspace(inWorkspace);
	if(priv->showCurrentMonitorOnly)
	{
		XfdashboardStageInterface			*stageInterface;
		XfdashboardWindowTrackerMonitor		*monitor;

		/* Get parent stage interface */
		stageInterface=xfdashboard_get_stage_of_actor(CLUTTER_ACTOR(self));

		/* Get monitor of stage interface if available */
		monitor=NULL;
		if(stageInterface) monitor=xfdashboard_stage_interface_get_monitor(stageInterface);

		/* Set monitor at newly created live workspace actor */
		xfdashboard_live_workspace_set_monitor(XFDASHBOARD_LIVE_WORKSPACE(actor), monitor);
	}
	g_signal_connect_swapped(actor, "clicked", G_CALLBACK(_xfdashboard_workspace_selector_on_workspace_clicked), self);
	clutter_actor_insert_child_at_index(CLUTTER_ACTOR(self), actor, index);

	action=xfdashboard_drop_action_new();
	clutter_actor_add_action(actor, action);
	g_signal_connect_swapped(action, "begin", G_CALLBACK(_xfdashboard_workspace_selector_on_drop_begin), actor);
	g_signal_connect_swapped(action, "drop", G_CALLBACK(_xfdashboard_workspace_selector_on_drop_drop), actor);
}

/* The active workspace has changed */
static void _xfdashboard_workspace_selector_on_active_workspace_changed(XfdashboardWorkspaceSelector *self,
																		XfdashboardWindowTrackerWorkspace *inPrevWorkspace,
																		gpointer inUserData)
{
	XfdashboardWorkspaceSelectorPrivate		*priv;
	XfdashboardLiveWorkspace				*liveWorkspace;
	XfdashboardWindowTrackerWorkspace		*workspace;

	g_return_if_fail(XFDASHBOARD_IS_WORKSPACE_SELECTOR(self));

	priv=self->priv;

	/* Unmark previous workspace */
	if(inPrevWorkspace)
	{
		liveWorkspace=_xfdashboard_workspace_selector_find_actor_for_workspace(self, inPrevWorkspace);
		if(liveWorkspace) xfdashboard_stylable_remove_pseudo_class(XFDASHBOARD_STYLABLE(liveWorkspace), "active");

		priv->activeWorkspace=NULL;
	}

	/* Mark new active workspace */
	workspace=xfdashboard_window_tracker_get_active_workspace(priv->windowTracker);
	if(workspace)
	{
		priv->activeWorkspace=workspace;

		liveWorkspace=_xfdashboard_workspace_selector_find_actor_for_workspace(self, priv->activeWorkspace);
		if(liveWorkspace) xfdashboard_stylable_add_pseudo_class(XFDASHBOARD_STYLABLE(liveWorkspace), "active");
	}
}

/* A scroll event occured in workspace selector (e.g. by mouse-wheel) */
static gboolean _xfdashboard_workspace_selector_on_scroll_event(ClutterActor *inActor,
																ClutterEvent *inEvent,
																gpointer inUserData)
{
	XfdashboardWorkspaceSelector			*self;
	XfdashboardWorkspaceSelectorPrivate		*priv;
	gint									direction;
	gint									currentWorkspace;
	gint									maxWorkspace;
	XfdashboardWindowTrackerWorkspace		*workspace;

	g_return_val_if_fail(XFDASHBOARD_IS_WORKSPACE_SELECTOR(inActor), CLUTTER_EVENT_PROPAGATE);
	g_return_val_if_fail(inEvent, CLUTTER_EVENT_PROPAGATE);

	self=XFDASHBOARD_WORKSPACE_SELECTOR(inActor);
	priv=self->priv;

	/* Get direction of scroll event */
	switch(clutter_event_get_scroll_direction(inEvent))
	{
		case CLUTTER_SCROLL_UP:
		case CLUTTER_SCROLL_LEFT:
			direction=-1;
			break;

		case CLUTTER_SCROLL_DOWN:
		case CLUTTER_SCROLL_RIGHT:
			direction=1;
			break;

		/* Unhandled directions */
		default:
			XFDASHBOARD_DEBUG(self, ACTOR,
								"Cannot handle scroll direction %d in %s",
								clutter_event_get_scroll_direction(inEvent),
								G_OBJECT_TYPE_NAME(self));
			return(CLUTTER_EVENT_PROPAGATE);
	}

	/* Get next workspace in scroll direction */
	currentWorkspace=xfdashboard_window_tracker_workspace_get_number(priv->activeWorkspace);
	maxWorkspace=xfdashboard_window_tracker_get_workspaces_count(priv->windowTracker);

	currentWorkspace+=direction;
	if(currentWorkspace<0 || currentWorkspace>=maxWorkspace) return(CLUTTER_EVENT_STOP);

	/* Activate new workspace */
	workspace=xfdashboard_window_tracker_get_workspace_by_number(priv->windowTracker, currentWorkspace);
	xfdashboard_window_tracker_workspace_activate(workspace);

	return(CLUTTER_EVENT_STOP);
}

/* IMPLEMENTATION: ClutterActor */

/* Get preferred width/height */
static void _xfdashboard_workspace_selector_get_preferred_height(ClutterActor *inActor,
																	gfloat inForWidth,
																	gfloat *outMinHeight,
																	gfloat *outNaturalHeight)
{
	XfdashboardWorkspaceSelector			*self=XFDASHBOARD_WORKSPACE_SELECTOR(inActor);
	XfdashboardWorkspaceSelectorPrivate		*priv=self->priv;
	gfloat									minHeight, naturalHeight;
	ClutterActor							*child;
	ClutterActorIter						iter;
	gfloat									childMinHeight, childNaturalHeight;
	gint									numberChildren;
	gfloat									requestChildSize;
	gfloat									maxSize;

	/* Set up default values */
	minHeight=naturalHeight=0.0f;

	/* Determine width for horizontal orientation ... */
	if(priv->orientation==CLUTTER_ORIENTATION_HORIZONTAL)
	{
		/* Determine maximum size */
		maxSize=_xfdashboard_workspace_selector_get_max_size_internal(self)-(2*priv->spacing);

		/* Count visible children */
		numberChildren=0;
		clutter_actor_iter_init(&iter, CLUTTER_ACTOR(inActor));
		while(clutter_actor_iter_next(&iter, &child))
		{
			/* Only check visible children */
			if(clutter_actor_is_visible(child)) numberChildren++;
		}

		/* All workspace actors should have the same size because they
		 * reside on the same screen and share its size. That's why
		 * we can devide the requested size (reduced by spacings) by
		 * the number of visible actors to get the preferred size for
		 * the "requested size" of each actor.
		 */
		requestChildSize=-1.0f;
		if(numberChildren>0 && inForWidth>=0.0f)
		{
			requestChildSize=inForWidth;
			requestChildSize-=(numberChildren+1)*priv->spacing;
			requestChildSize/=numberChildren;
		}

		clutter_actor_iter_init(&iter, CLUTTER_ACTOR(inActor));
		while(clutter_actor_iter_next(&iter, &child))
		{
			/* Only handle visible children */
			if(!clutter_actor_is_visible(child)) continue;

			/* Get child's size */
			_xfdashboard_workspace_selector_get_preferred_height_for_child(self,
																			child,
																			requestChildSize,
																			&childMinHeight,
																			&childNaturalHeight);

			/* Adjust size to maximal size allowed */
			childMinHeight=MIN(maxSize, childMinHeight);
			childNaturalHeight=MIN(maxSize, childNaturalHeight);

			/* Determine heights */
			minHeight=MAX(minHeight, childMinHeight);
			naturalHeight=MAX(naturalHeight, childNaturalHeight);
		}

		/* Add spacing between children and spacing as padding */
		if(numberChildren>0)
		{
			minHeight+=2*priv->spacing;
			naturalHeight+=2*priv->spacing;
		}
	}
		/* ... otherwise determine height for vertical orientation */
		else
		{
			/* Reduce requested width by spacing */
			if(inForWidth>=0.0f) inForWidth-=2*priv->spacing;

			/* Calculate children heights */
			numberChildren=0;
			clutter_actor_iter_init(&iter, CLUTTER_ACTOR(inActor));
			while(clutter_actor_iter_next(&iter, &child))
			{
				/* Only handle visible children */
				if(!clutter_actor_is_visible(child)) continue;

				/* Get child's size */
				_xfdashboard_workspace_selector_get_preferred_height_for_child(self,
																				child,
																				inForWidth,
																				&childMinHeight,
																				&childNaturalHeight);

				/* Sum heights */
				minHeight+=childMinHeight;
				naturalHeight+=childNaturalHeight;

				/* Count visible children */
				numberChildren++;
			}

			/* Add spacing between children and spacing as padding */
			if(numberChildren>0)
			{
				minHeight+=(numberChildren+1)*priv->spacing;
				naturalHeight+=(numberChildren+1)*priv->spacing;
			}
		}

	/* Store sizes computed */
	if(outMinHeight) *outMinHeight=minHeight;
	if(outNaturalHeight) *outNaturalHeight=naturalHeight;
}

static void _xfdashboard_workspace_selector_get_preferred_width(ClutterActor *inActor,
																gfloat inForHeight,
																gfloat *outMinWidth,
																gfloat *outNaturalWidth)
{
	XfdashboardWorkspaceSelector			*self=XFDASHBOARD_WORKSPACE_SELECTOR(inActor);
	XfdashboardWorkspaceSelectorPrivate		*priv=self->priv;
	gfloat									minWidth, naturalWidth;
	ClutterActor							*child;
	ClutterActorIter						iter;
	gfloat									childMinWidth, childNaturalWidth;
	gint									numberChildren;
	gfloat									requestChildSize;
	gfloat									maxSize;

	/* Set up default values */
	minWidth=naturalWidth=0.0f;

	/* Determine width for horizontal orientation ... */
	if(priv->orientation==CLUTTER_ORIENTATION_HORIZONTAL)
	{
		/* Reduce requested height by spacing */
		if(inForHeight>=0.0f) inForHeight-=2*priv->spacing;

		/* Calculate children widths */
		numberChildren=0;
		clutter_actor_iter_init(&iter, CLUTTER_ACTOR(inActor));
		while(clutter_actor_iter_next(&iter, &child))
		{
			/* Only handle visible children */
			if(!clutter_actor_is_visible(child)) continue;

			/* Get child's size */
			_xfdashboard_workspace_selector_get_preferred_width_for_child(self,
																			child,
																			inForHeight,
																			&childMinWidth,
																			&childNaturalWidth);

			/* Sum widths */
			minWidth+=childMinWidth;
			naturalWidth+=childNaturalWidth;

			/* Count visible children */
			numberChildren++;
		}

		/* Add spacing between children and spacing as padding */
		if(numberChildren>0)
		{
			minWidth+=(numberChildren+1)*priv->spacing;
			naturalWidth+=(numberChildren+1)*priv->spacing;
		}
	}
		/* ... otherwise determine height for vertical orientation */
		else
		{
			/* Determine maximum size */
			maxSize=_xfdashboard_workspace_selector_get_max_size_internal(self)-(2*priv->spacing);

			/* Count visible children */
			numberChildren=0;
			clutter_actor_iter_init(&iter, CLUTTER_ACTOR(inActor));
			while(clutter_actor_iter_next(&iter, &child))
			{
				/* Only check visible children */
				if(clutter_actor_is_visible(child)) numberChildren++;
			}

			/* All workspace actors should have the same size because they
			 * reside on the same screen and share its size. That's why
			 * we can devide the requested size (reduced by spacings) by
			 * the number of visible actors to get the preferred size for
			 * the "requested size" of each actor.
			 */
			requestChildSize=-1.0f;
			if(numberChildren>0 && inForHeight>=0.0f)
			{
				requestChildSize=inForHeight;
				requestChildSize-=(numberChildren+1)*priv->spacing;
				requestChildSize/=numberChildren;
			}

			clutter_actor_iter_init(&iter, CLUTTER_ACTOR(inActor));
			while(clutter_actor_iter_next(&iter, &child))
			{
				/* Only handle visible children */
				if(!clutter_actor_is_visible(child)) continue;

				/* Get child's size */
				_xfdashboard_workspace_selector_get_preferred_width_for_child(self,
																				child,
																				requestChildSize,
																				&childMinWidth,
																				&childNaturalWidth);

				/* Adjust size to maximal size allowed */
				childMinWidth=MIN(maxSize, childMinWidth);
				childNaturalWidth=MIN(maxSize, childNaturalWidth);

				/* Determine widths */
				minWidth=MAX(minWidth, childMinWidth);
				naturalWidth=MAX(naturalWidth, childNaturalWidth);
			}

			/* Add spacing between children and spacing as padding */
			if(numberChildren>0)
			{
				minWidth+=2*priv->spacing;
				naturalWidth+=2*priv->spacing;
			}
		}

	/* Store sizes computed */
	if(outMinWidth) *outMinWidth=minWidth;
	if(outNaturalWidth) *outNaturalWidth=naturalWidth;
}

/* Allocate position and size of actor and its children */
static void _xfdashboard_workspace_selector_allocate(ClutterActor *inActor,
														const ClutterActorBox *inBox,
														ClutterAllocationFlags inFlags)
{
	XfdashboardWorkspaceSelector			*self=XFDASHBOARD_WORKSPACE_SELECTOR(inActor);
	XfdashboardWorkspaceSelectorPrivate		*priv=self->priv;
	gfloat									availableWidth, availableHeight;
	gfloat									childWidth, childHeight;
	ClutterActor							*child;
	ClutterActorIter						iter;
	ClutterActorBox							childAllocation={ 0, };

	/* Chain up to store the allocation of the actor */
	CLUTTER_ACTOR_CLASS(xfdashboard_workspace_selector_parent_class)->allocate(inActor, inBox, inFlags);

	/* Get available size */
	clutter_actor_box_get_size(inBox, &availableWidth, &availableHeight);

	/* Calculate new position and size of visible children */
	childAllocation.x1=childAllocation.y1=priv->spacing;
	clutter_actor_iter_init(&iter, CLUTTER_ACTOR(inActor));
	while(clutter_actor_iter_next(&iter, &child))
	{
		/* Is child visible? */
		if(!clutter_actor_is_visible(child)) continue;

		/* Calculate new position and size of child */
		if(priv->orientation==CLUTTER_ORIENTATION_HORIZONTAL)
		{
			childHeight=availableHeight-(2*priv->spacing);
			clutter_actor_get_preferred_width(child, childHeight, NULL, &childWidth);

			childAllocation.y1=ceil(MAX(((availableHeight-childHeight))/2.0f, priv->spacing));
			childAllocation.y2=floor(childAllocation.y1+childHeight);
			childAllocation.x2=floor(childAllocation.x1+childWidth);
		}
			else
			{
				childWidth=availableWidth-(2*priv->spacing);
				clutter_actor_get_preferred_height(child, childWidth, NULL, &childHeight);

				childAllocation.x1=ceil(MAX(((availableWidth-childWidth))/2.0f, priv->spacing));
				childAllocation.x2=floor(childAllocation.x1+childWidth);
				childAllocation.y2=floor(childAllocation.y1+childHeight);
			}

		clutter_actor_allocate(child, &childAllocation, inFlags);

		/* Set up for next child */
		if(priv->orientation==CLUTTER_ORIENTATION_HORIZONTAL) childAllocation.x1=floor(childAllocation.x1+childWidth+priv->spacing);
			else childAllocation.y1=floor(childAllocation.y1+childHeight+priv->spacing);
	}
}

/* IMPLEMENTATION: Interface XfdashboardFocusable */

/* Determine if this actor supports selection */
static gboolean _xfdashboard_workspace_selector_focusable_supports_selection(XfdashboardFocusable *inFocusable)
{
	g_return_val_if_fail(XFDASHBOARD_IS_FOCUSABLE(inFocusable), FALSE);
	g_return_val_if_fail(XFDASHBOARD_IS_WORKSPACE_SELECTOR(inFocusable), FALSE);

	/* This actor supports selection */
	return(TRUE);
}

/* Get current selection */
static ClutterActor* _xfdashboard_workspace_selector_focusable_get_selection(XfdashboardFocusable *inFocusable)
{
	XfdashboardWorkspaceSelector			*self;
	XfdashboardWorkspaceSelectorPrivate		*priv;
	XfdashboardLiveWorkspace				*actor;

	g_return_val_if_fail(XFDASHBOARD_IS_FOCUSABLE(inFocusable), NULL);
	g_return_val_if_fail(XFDASHBOARD_IS_WORKSPACE_SELECTOR(inFocusable), NULL);

	self=XFDASHBOARD_WORKSPACE_SELECTOR(inFocusable);
	priv=self->priv;
	actor=NULL;

	/* Find actor for current active workspace which is also the current selection */
	if(priv->activeWorkspace)
	{
		actor=_xfdashboard_workspace_selector_find_actor_for_workspace(self, priv->activeWorkspace);
	}
	if(!actor) return(NULL);

	/* Return current selection */
	return(CLUTTER_ACTOR(actor));
}

/* Set new selection */
static gboolean _xfdashboard_workspace_selector_focusable_set_selection(XfdashboardFocusable *inFocusable,
																		ClutterActor *inSelection)
{
	XfdashboardWorkspaceSelector			*self;
	XfdashboardLiveWorkspace				*actor;
	XfdashboardWindowTrackerWorkspace		*workspace;

	g_return_val_if_fail(XFDASHBOARD_IS_FOCUSABLE(inFocusable), FALSE);
	g_return_val_if_fail(XFDASHBOARD_IS_WORKSPACE_SELECTOR(inFocusable), FALSE);
	g_return_val_if_fail(XFDASHBOARD_IS_LIVE_WORKSPACE(inSelection), FALSE);

	self=XFDASHBOARD_WORKSPACE_SELECTOR(inFocusable);
	actor=XFDASHBOARD_LIVE_WORKSPACE(inSelection);
	workspace=NULL;

	/* Check that selection is a child of this actor */
	if(!clutter_actor_contains(CLUTTER_ACTOR(self), inSelection))
	{
		ClutterActor						*parent;

		parent=clutter_actor_get_parent(inSelection);
		g_warning(_("%s is a child of %s and cannot be selected at %s"),
					G_OBJECT_TYPE_NAME(inSelection),
					parent ? G_OBJECT_TYPE_NAME(parent) : "<nil>",
					G_OBJECT_TYPE_NAME(self));
	}

	/* Get workspace of new selection and set new selection*/
	workspace=xfdashboard_live_workspace_get_workspace(actor);
	if(workspace)
	{
		/* Setting new selection means also to set selected workspace active */
		xfdashboard_window_tracker_workspace_activate(workspace);

		/* New selection was set successfully */
		return(TRUE);
	}

	/* Setting new selection was unsuccessful if we get here */
	g_warning(_("Could not determine workspace of %s to set selection at %s"),
				G_OBJECT_TYPE_NAME(actor),
				G_OBJECT_TYPE_NAME(self));

	return(FALSE);
}

/* Find requested selection target depending of current selection */
static ClutterActor* _xfdashboard_workspace_selector_focusable_find_selection(XfdashboardFocusable *inFocusable,
																				ClutterActor *inSelection,
																				XfdashboardSelectionTarget inDirection)
{
	XfdashboardWorkspaceSelector			*self;
	XfdashboardWorkspaceSelectorPrivate		*priv;
	XfdashboardLiveWorkspace				*selection;
	ClutterActor							*newSelection;
	gchar									*valueName;

	g_return_val_if_fail(XFDASHBOARD_IS_FOCUSABLE(inFocusable), NULL);
	g_return_val_if_fail(XFDASHBOARD_IS_WORKSPACE_SELECTOR(inFocusable), NULL);
	g_return_val_if_fail(!inSelection || XFDASHBOARD_IS_LIVE_WORKSPACE(inSelection), NULL);

	self=XFDASHBOARD_WORKSPACE_SELECTOR(inFocusable);
	priv=self->priv;
	newSelection=NULL;
	selection=NULL;

	/* Find actor for current active workspace which is also the current selection */
	if(priv->activeWorkspace)
	{
		selection=_xfdashboard_workspace_selector_find_actor_for_workspace(self, priv->activeWorkspace);
	}
	if(!selection) return(NULL);

	/* If there is nothing selected return currently determined actor which is
	 * the current active workspace.
	 */
	if(!inSelection)
	{
		valueName=xfdashboard_get_enum_value_name(XFDASHBOARD_TYPE_SELECTION_TARGET, inDirection);
		XFDASHBOARD_DEBUG(self, ACTOR,
							"No selection at %s, so select first child %s for direction %s",
							G_OBJECT_TYPE_NAME(self),
							selection ? G_OBJECT_TYPE_NAME(selection) : "<nil>",
							valueName);
		g_free(valueName);

		return(CLUTTER_ACTOR(selection));
	}

	/* Check that selection is a child of this actor otherwise return NULL */
	if(!clutter_actor_contains(CLUTTER_ACTOR(self), inSelection))
	{
		ClutterActor						*parent;

		parent=clutter_actor_get_parent(inSelection);
		g_warning(_("Cannot lookup selection target at %s because %s is a child of %s"),
					G_OBJECT_TYPE_NAME(self),
					G_OBJECT_TYPE_NAME(inSelection),
					parent ? G_OBJECT_TYPE_NAME(parent) : "<nil>");

		return(NULL);
	}

	/* Find target selection */
	switch(inDirection)
	{
		case XFDASHBOARD_SELECTION_TARGET_LEFT:
			if(priv->orientation==CLUTTER_ORIENTATION_HORIZONTAL)
			{
				newSelection=clutter_actor_get_previous_sibling(CLUTTER_ACTOR(selection));
			}
			break;

		case XFDASHBOARD_SELECTION_TARGET_UP:
			if(priv->orientation==CLUTTER_ORIENTATION_VERTICAL)
			{
				newSelection=clutter_actor_get_previous_sibling(CLUTTER_ACTOR(selection));
			}
			break;

		case XFDASHBOARD_SELECTION_TARGET_RIGHT:
			if(priv->orientation==CLUTTER_ORIENTATION_HORIZONTAL)
			{
				newSelection=clutter_actor_get_next_sibling(CLUTTER_ACTOR(selection));
			}
			break;

		case XFDASHBOARD_SELECTION_TARGET_DOWN:
			if(priv->orientation==CLUTTER_ORIENTATION_VERTICAL)
			{
				newSelection=clutter_actor_get_next_sibling(CLUTTER_ACTOR(selection));
			}
			break;

		case XFDASHBOARD_SELECTION_TARGET_FIRST:
		case XFDASHBOARD_SELECTION_TARGET_PAGE_UP:
		case XFDASHBOARD_SELECTION_TARGET_PAGE_LEFT:
			if(inDirection==XFDASHBOARD_SELECTION_TARGET_FIRST ||
				(inDirection==XFDASHBOARD_SELECTION_TARGET_PAGE_UP && priv->orientation==CLUTTER_ORIENTATION_VERTICAL) ||
				(inDirection==XFDASHBOARD_SELECTION_TARGET_PAGE_LEFT && priv->orientation==CLUTTER_ORIENTATION_HORIZONTAL))
			{
				newSelection=clutter_actor_get_first_child(CLUTTER_ACTOR(self));
			}
			break;

		case XFDASHBOARD_SELECTION_TARGET_LAST:
		case XFDASHBOARD_SELECTION_TARGET_PAGE_DOWN:
		case XFDASHBOARD_SELECTION_TARGET_PAGE_RIGHT:
			if(inDirection==XFDASHBOARD_SELECTION_TARGET_LAST ||
				(inDirection==XFDASHBOARD_SELECTION_TARGET_PAGE_DOWN && priv->orientation==CLUTTER_ORIENTATION_VERTICAL) ||
				(inDirection==XFDASHBOARD_SELECTION_TARGET_PAGE_RIGHT && priv->orientation==CLUTTER_ORIENTATION_HORIZONTAL))
			{
				newSelection=clutter_actor_get_last_child(CLUTTER_ACTOR(self));
			}
			break;

		case XFDASHBOARD_SELECTION_TARGET_NEXT:
			newSelection=clutter_actor_get_next_sibling(CLUTTER_ACTOR(selection));
			if(!newSelection) newSelection=clutter_actor_get_previous_sibling(CLUTTER_ACTOR(selection));
			break;

		default:
			{
				valueName=xfdashboard_get_enum_value_name(XFDASHBOARD_TYPE_SELECTION_TARGET, inDirection);
				g_critical(_("Focusable object %s does not handle selection direction of type %s."),
							G_OBJECT_TYPE_NAME(self),
							valueName);
				g_free(valueName);
			}
			break;
	}

	/* If new selection could be found override current selection with it */
	if(newSelection && XFDASHBOARD_IS_LIVE_WORKSPACE(newSelection))
	{
		selection=XFDASHBOARD_LIVE_WORKSPACE(newSelection);
	}

	/* Return new selection found */
	XFDASHBOARD_DEBUG(self, ACTOR,
						"Selecting %s at %s for current selection %s in direction %u",
						selection ? G_OBJECT_TYPE_NAME(selection) : "<nil>",
						G_OBJECT_TYPE_NAME(self),
						inSelection ? G_OBJECT_TYPE_NAME(inSelection) : "<nil>",
						inDirection);

	return(CLUTTER_ACTOR(selection));
}

/* Activate selection */
static gboolean _xfdashboard_workspace_selector_focusable_activate_selection(XfdashboardFocusable *inFocusable,
																				ClutterActor *inSelection)
{
	XfdashboardWorkspaceSelector			*self;
	XfdashboardLiveWorkspace				*actor;
	XfdashboardWindowTrackerWorkspace		*workspace;

	g_return_val_if_fail(XFDASHBOARD_IS_FOCUSABLE(inFocusable), FALSE);
	g_return_val_if_fail(XFDASHBOARD_IS_WORKSPACE_SELECTOR(inFocusable), FALSE);
	g_return_val_if_fail(XFDASHBOARD_IS_LIVE_WORKSPACE(inSelection), FALSE);

	self=XFDASHBOARD_WORKSPACE_SELECTOR(inFocusable);
	actor=XFDASHBOARD_LIVE_WORKSPACE(inSelection);
	workspace=NULL;

	/* Check that selection is a child of this actor */
	if(!clutter_actor_contains(CLUTTER_ACTOR(self), inSelection))
	{
		ClutterActor						*parent;

		parent=clutter_actor_get_parent(inSelection);
		g_warning(_("%s is a child of %s and cannot be selected at %s"),
					G_OBJECT_TYPE_NAME(inSelection),
					parent ? G_OBJECT_TYPE_NAME(parent) : "<nil>",
					G_OBJECT_TYPE_NAME(self));
	}

	/* Get workspace of new selection and set new selection*/
	workspace=xfdashboard_live_workspace_get_workspace(actor);
	if(workspace)
	{
		/* Activate workspace */
		xfdashboard_window_tracker_workspace_activate(workspace);

		/* Quit application */
		xfdashboard_application_suspend_or_quit(NULL);

		/* Activation was successful */
		return(TRUE);
	}

	/* Activation was unsuccessful if we get here */
	g_warning(_("Could not determine workspace of %s to set selection at %s"),
				G_OBJECT_TYPE_NAME(actor),
				G_OBJECT_TYPE_NAME(self));
	return(FALSE);
}

/* Interface initialization
 * Set up default functions
 */
void _xfdashboard_workspace_selector_focusable_iface_init(XfdashboardFocusableInterface *iface)
{
	iface->supports_selection=_xfdashboard_workspace_selector_focusable_supports_selection;
	iface->get_selection=_xfdashboard_workspace_selector_focusable_get_selection;
	iface->set_selection=_xfdashboard_workspace_selector_focusable_set_selection;
	iface->find_selection=_xfdashboard_workspace_selector_focusable_find_selection;
	iface->activate_selection=_xfdashboard_workspace_selector_focusable_activate_selection;
}

/* IMPLEMENTATION: GObject */

/* Dispose this object */
static void _xfdashboard_workspace_selector_dispose(GObject *inObject)
{
	XfdashboardWorkspaceSelector			*self=XFDASHBOARD_WORKSPACE_SELECTOR(inObject);
	XfdashboardWorkspaceSelectorPrivate		*priv=self->priv;

	/* Release allocated resources */
	if(priv->activeWorkspace)
	{
		_xfdashboard_workspace_selector_on_active_workspace_changed(self, NULL, priv->activeWorkspace);
		priv->activeWorkspace=NULL;
	}

	if(priv->windowTracker)
	{
		g_signal_handlers_disconnect_by_data(priv->windowTracker, self);
		g_object_unref(priv->windowTracker);
		priv->windowTracker=NULL;
	}

	/* Call parent's class dispose method */
	G_OBJECT_CLASS(xfdashboard_workspace_selector_parent_class)->dispose(inObject);
}

/* Set/get properties */
static void _xfdashboard_workspace_selector_set_property(GObject *inObject,
															guint inPropID,
															const GValue *inValue,
															GParamSpec *inSpec)
{
	XfdashboardWorkspaceSelector			*self=XFDASHBOARD_WORKSPACE_SELECTOR(inObject);

	switch(inPropID)
	{
		case PROP_SPACING:
			xfdashboard_workspace_selector_set_spacing(self, g_value_get_float(inValue));
			break;

		case PROP_ORIENTATION:
			xfdashboard_workspace_selector_set_orientation(self, g_value_get_enum(inValue));
			break;

		case PROP_MAX_SIZE:
			xfdashboard_workspace_selector_set_maximum_size(self, g_value_get_float(inValue));
			break;

		case PROP_MAX_FRACTION:
			xfdashboard_workspace_selector_set_maximum_fraction(self, g_value_get_float(inValue));
			break;

		case PROP_SHOW_CURRENT_MONITOR_ONLY:
			xfdashboard_workspace_selector_set_show_current_monitor_only(self, g_value_get_boolean(inValue));
			break;

		default:
			G_OBJECT_WARN_INVALID_PROPERTY_ID(inObject, inPropID, inSpec);
			break;
	}
}

static void _xfdashboard_workspace_selector_get_property(GObject *inObject,
															guint inPropID,
															GValue *outValue,
															GParamSpec *inSpec)
{
	XfdashboardWorkspaceSelector			*self=XFDASHBOARD_WORKSPACE_SELECTOR(inObject);
	XfdashboardWorkspaceSelectorPrivate		*priv=self->priv;

	switch(inPropID)
	{
		case PROP_SPACING:
			g_value_set_float(outValue, priv->spacing);
			break;

		case PROP_ORIENTATION:
			g_value_set_enum(outValue, priv->orientation);
			break;

		case PROP_MAX_SIZE:
			g_value_set_float(outValue, priv->maxSize);
			break;

		case PROP_MAX_FRACTION:
			g_value_set_float(outValue, priv->maxFraction);
			break;

		case PROP_USING_FRACTION:
			g_value_set_boolean(outValue, priv->usingFraction);
			break;

		case PROP_SHOW_CURRENT_MONITOR_ONLY:
			g_value_set_boolean(outValue, priv->showCurrentMonitorOnly);
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
static void xfdashboard_workspace_selector_class_init(XfdashboardWorkspaceSelectorClass *klass)
{
	XfdashboardActorClass	*actorClass=XFDASHBOARD_ACTOR_CLASS(klass);
	ClutterActorClass		*clutterActorClass=CLUTTER_ACTOR_CLASS(klass);
	GObjectClass			*gobjectClass=G_OBJECT_CLASS(klass);

	/* Override functions */
	gobjectClass->dispose=_xfdashboard_workspace_selector_dispose;
	gobjectClass->set_property=_xfdashboard_workspace_selector_set_property;
	gobjectClass->get_property=_xfdashboard_workspace_selector_get_property;

	clutterActorClass->get_preferred_width=_xfdashboard_workspace_selector_get_preferred_width;
	clutterActorClass->get_preferred_height=_xfdashboard_workspace_selector_get_preferred_height;
	clutterActorClass->allocate=_xfdashboard_workspace_selector_allocate;

	/* Define properties */
	XfdashboardWorkspaceSelectorProperties[PROP_SPACING]=
		g_param_spec_float("spacing",
								_("Spacing"),
								_("The spacing between children"),
								0.0, G_MAXFLOAT,
								0.0,
								G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS);

	XfdashboardWorkspaceSelectorProperties[PROP_ORIENTATION]=
		g_param_spec_enum("orientation",
							_("Orientation"),
							_("The orientation to layout children"),
							CLUTTER_TYPE_ORIENTATION,
							DEFAULT_ORIENTATION,
							G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS);

	XfdashboardWorkspaceSelectorProperties[PROP_MAX_SIZE]=
		g_param_spec_float("max-size",
								_("Maximum size"),
								_("The maximum size of this actor for opposite direction of orientation"),
								0.0, G_MAXFLOAT,
								DEFAULT_MAX_SIZE,
								G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS);

	XfdashboardWorkspaceSelectorProperties[PROP_MAX_FRACTION]=
		g_param_spec_float("max-fraction",
								_("Maximum fraction"),
								_("The maximum size of this actor for opposite direction of orientation defined by fraction between 0.0 and 1.0"),
								0.0, G_MAXFLOAT,
								DEFAULT_MAX_FRACTION,
								G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS);

	XfdashboardWorkspaceSelectorProperties[PROP_USING_FRACTION]=
		g_param_spec_boolean("using-fraction",
								_("Using fraction"),
								_("Flag indicating if maximum size is static or defined by fraction between 0.0 and 1.0"),
								DEFAULT_USING_FRACTION,
								G_PARAM_READABLE | G_PARAM_STATIC_STRINGS);

	XfdashboardWorkspaceSelectorProperties[PROP_SHOW_CURRENT_MONITOR_ONLY]=
		g_param_spec_boolean("show-current-monitor-only",
								_("Show current monitor only"),
								_("Show only windows of the monitor where this actor is placed"),
								FALSE,
								G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS);

	g_object_class_install_properties(gobjectClass, PROP_LAST, XfdashboardWorkspaceSelectorProperties);

	/* Define stylable properties */
	xfdashboard_actor_install_stylable_property(actorClass, XfdashboardWorkspaceSelectorProperties[PROP_SPACING]);
	xfdashboard_actor_install_stylable_property(actorClass, XfdashboardWorkspaceSelectorProperties[PROP_ORIENTATION]);
	xfdashboard_actor_install_stylable_property(actorClass, XfdashboardWorkspaceSelectorProperties[PROP_MAX_SIZE]);
	xfdashboard_actor_install_stylable_property(actorClass, XfdashboardWorkspaceSelectorProperties[PROP_MAX_FRACTION]);
}

/* Object initialization
 * Create private structure and set up default values
 */
static void xfdashboard_workspace_selector_init(XfdashboardWorkspaceSelector *self)
{
	XfdashboardWorkspaceSelectorPrivate		*priv;
	ClutterRequestMode						requestMode;
	GList									*workspaces;
	XfdashboardWindowTrackerWorkspace		*workspace;

	priv=self->priv=xfdashboard_workspace_selector_get_instance_private(self);

	/* Set up default values */
	priv->windowTracker=xfdashboard_window_tracker_get_default();
	priv->activeWorkspace=NULL;
	priv->spacing=0.0f;
	priv->orientation=DEFAULT_ORIENTATION;
	priv->maxSize=DEFAULT_MAX_SIZE;
	priv->maxFraction=DEFAULT_MAX_FRACTION;
	priv->usingFraction=DEFAULT_USING_FRACTION;
	priv->showCurrentMonitorOnly=FALSE;

	/* Set up this actor */
	clutter_actor_set_reactive(CLUTTER_ACTOR(self), TRUE);

	requestMode=(priv->orientation==CLUTTER_ORIENTATION_HORIZONTAL ? CLUTTER_REQUEST_HEIGHT_FOR_WIDTH : CLUTTER_REQUEST_WIDTH_FOR_HEIGHT);
	clutter_actor_set_request_mode(CLUTTER_ACTOR(self), requestMode);

	/* Connect signals */
	g_signal_connect(self, "scroll-event", G_CALLBACK(_xfdashboard_workspace_selector_on_scroll_event), NULL);

	g_signal_connect_swapped(priv->windowTracker,
								"workspace-added",
								G_CALLBACK(_xfdashboard_workspace_selector_on_workspace_added),
								self);

	g_signal_connect_swapped(priv->windowTracker,
								"workspace-removed",
								G_CALLBACK(_xfdashboard_workspace_selector_on_workspace_removed),
								self);

	g_signal_connect_swapped(priv->windowTracker,
								"active-workspace-changed",
								G_CALLBACK(_xfdashboard_workspace_selector_on_active_workspace_changed),
								self);

	/* If we there are already workspace known add them to this actor */
	workspaces=xfdashboard_window_tracker_get_workspaces(priv->windowTracker);
	if(workspaces)
	{
		for(; workspaces; workspaces=g_list_next(workspaces))
		{
			workspace=(XfdashboardWindowTrackerWorkspace*)workspaces->data;

			_xfdashboard_workspace_selector_on_workspace_added(self, workspace, NULL);
		}
	}

	/* If active workspace is already available then set it in this actor */
	workspace=xfdashboard_window_tracker_get_active_workspace(priv->windowTracker);
	if(workspace)
	{
		_xfdashboard_workspace_selector_on_active_workspace_changed(self, NULL, NULL);
	}
}

/* IMPLEMENTATION: Public API */

/* Create new actor */
ClutterActor* xfdashboard_workspace_selector_new(void)
{
	return(g_object_new(XFDASHBOARD_TYPE_WORKSPACE_SELECTOR, NULL));
}

ClutterActor* xfdashboard_workspace_selector_new_with_orientation(ClutterOrientation inOrientation)
{
	g_return_val_if_fail(inOrientation==CLUTTER_ORIENTATION_HORIZONTAL || inOrientation==CLUTTER_ORIENTATION_VERTICAL, NULL);

	return(g_object_new(XFDASHBOARD_TYPE_WORKSPACE_SELECTOR,
						"orientation", inOrientation,
						NULL));
}

/* Get/set spacing between children */
gfloat xfdashboard_workspace_selector_get_spacing(XfdashboardWorkspaceSelector *self)
{
	g_return_val_if_fail(XFDASHBOARD_IS_WORKSPACE_SELECTOR(self), 0.0f);

	return(self->priv->spacing);
}

void xfdashboard_workspace_selector_set_spacing(XfdashboardWorkspaceSelector *self, const gfloat inSpacing)
{
	XfdashboardWorkspaceSelectorPrivate		*priv;

	g_return_if_fail(XFDASHBOARD_IS_WORKSPACE_SELECTOR(self));
	g_return_if_fail(inSpacing>=0.0f);

	priv=self->priv;

	/* Set value if changed */
	if(priv->spacing!=inSpacing)
	{
		/* Set value */
		priv->spacing=inSpacing;
		clutter_actor_queue_relayout(CLUTTER_ACTOR(self));

		xfdashboard_background_set_corner_radius(XFDASHBOARD_BACKGROUND(self), priv->spacing);

		/* Notify about property change */
		g_object_notify_by_pspec(G_OBJECT(self), XfdashboardWorkspaceSelectorProperties[PROP_SPACING]);
	}
}

/* Get/set orientation */
ClutterOrientation xfdashboard_workspace_selector_get_orientation(XfdashboardWorkspaceSelector *self)
{
	g_return_val_if_fail(XFDASHBOARD_IS_WORKSPACE_SELECTOR(self), DEFAULT_ORIENTATION);

	return(self->priv->orientation);
}

void xfdashboard_workspace_selector_set_orientation(XfdashboardWorkspaceSelector *self, ClutterOrientation inOrientation)
{
	XfdashboardWorkspaceSelectorPrivate		*priv;
	ClutterRequestMode						requestMode;

	g_return_if_fail(XFDASHBOARD_IS_WORKSPACE_SELECTOR(self));
	g_return_if_fail(inOrientation==CLUTTER_ORIENTATION_HORIZONTAL ||
						inOrientation==CLUTTER_ORIENTATION_VERTICAL);

	priv=self->priv;

	/* Set value if changed */
	if(priv->orientation!=inOrientation)
	{
		/* Set value */
		priv->orientation=inOrientation;

		requestMode=(priv->orientation==CLUTTER_ORIENTATION_HORIZONTAL ? CLUTTER_REQUEST_HEIGHT_FOR_WIDTH : CLUTTER_REQUEST_WIDTH_FOR_HEIGHT);
		clutter_actor_set_request_mode(CLUTTER_ACTOR(self), requestMode);

		clutter_actor_queue_relayout(CLUTTER_ACTOR(self));

		/* Notify about property change */
		g_object_notify_by_pspec(G_OBJECT(self), XfdashboardWorkspaceSelectorProperties[PROP_ORIENTATION]);
	}
}

/* Get/set static maximum size of children */
gfloat xfdashboard_workspace_selector_get_maximum_size(XfdashboardWorkspaceSelector *self)
{
	g_return_val_if_fail(XFDASHBOARD_IS_WORKSPACE_SELECTOR(self), 0.0f);

	return(self->priv->maxSize);
}

void xfdashboard_workspace_selector_set_maximum_size(XfdashboardWorkspaceSelector *self, const gfloat inSize)
{
	XfdashboardWorkspaceSelectorPrivate		*priv;
	gboolean								needRelayout;

	g_return_if_fail(XFDASHBOARD_IS_WORKSPACE_SELECTOR(self));
	g_return_if_fail(inSize>=0.0f);

	priv=self->priv;
	needRelayout=FALSE;

	/* Freeze notification */
	g_object_freeze_notify(G_OBJECT(self));

	/* Set values if changed */
	if(priv->usingFraction)
	{
		/* Set value */
		priv->usingFraction=FALSE;
		needRelayout=TRUE;

		/* Notify about property change */
		g_object_notify_by_pspec(G_OBJECT(self), XfdashboardWorkspaceSelectorProperties[PROP_USING_FRACTION]);
	}

	if(priv->maxSize!=inSize)
	{
		/* Set value */
		priv->maxSize=inSize;
		needRelayout=TRUE;

		/* Notify about property change */
		g_object_notify_by_pspec(G_OBJECT(self), XfdashboardWorkspaceSelectorProperties[PROP_MAX_SIZE]);
	}

	/* Queue a relayout if needed */
	if(needRelayout) clutter_actor_queue_relayout(CLUTTER_ACTOR(self));

	/* Thaw notification */
	g_object_thaw_notify(G_OBJECT(self));
}

/* Get/set maximum size of children by fraction */
gfloat xfdashboard_workspace_selector_get_maximum_fraction(XfdashboardWorkspaceSelector *self)
{
	g_return_val_if_fail(XFDASHBOARD_IS_WORKSPACE_SELECTOR(self), 0.0f);

	return(self->priv->maxFraction);
}

void xfdashboard_workspace_selector_set_maximum_fraction(XfdashboardWorkspaceSelector *self, const gfloat inFraction)
{
	XfdashboardWorkspaceSelectorPrivate		*priv;
	gboolean								needRelayout;

	g_return_if_fail(XFDASHBOARD_IS_WORKSPACE_SELECTOR(self));
	g_return_if_fail(inFraction>0.0f && inFraction<=1.0f);

	priv=self->priv;
	needRelayout=FALSE;

	/* Freeze notification */
	g_object_freeze_notify(G_OBJECT(self));

	/* Set values if changed */
	if(!priv->usingFraction)
	{
		/* Set value */
		priv->usingFraction=TRUE;
		needRelayout=TRUE;

		/* Notify about property change */
		g_object_notify_by_pspec(G_OBJECT(self), XfdashboardWorkspaceSelectorProperties[PROP_USING_FRACTION]);
	}

	if(priv->maxFraction!=inFraction)
	{
		/* Set value */
		priv->maxFraction=inFraction;
		needRelayout=TRUE;

		/* Notify about property change */
		g_object_notify_by_pspec(G_OBJECT(self), XfdashboardWorkspaceSelectorProperties[PROP_MAX_FRACTION]);
	}

	/* Queue a relayout if needed */
	if(needRelayout) clutter_actor_queue_relayout(CLUTTER_ACTOR(self));

	/* Thaw notification */
	g_object_thaw_notify(G_OBJECT(self));
}

/* Get state if maximum size is static or calculated by fraction dynamically */
gboolean xfdashboard_workspace_selector_is_using_fraction(XfdashboardWorkspaceSelector *self)
{
	g_return_val_if_fail(XFDASHBOARD_IS_WORKSPACE_SELECTOR(self), FALSE);

	return(self->priv->usingFraction);
}

/* Get/set orientation */
gboolean xfdashboard_workspace_selector_get_show_current_monitor_only(XfdashboardWorkspaceSelector *self)
{
	g_return_val_if_fail(XFDASHBOARD_IS_WORKSPACE_SELECTOR(self), FALSE);

	return(self->priv->showCurrentMonitorOnly);
}

void xfdashboard_workspace_selector_set_show_current_monitor_only(XfdashboardWorkspaceSelector *self, gboolean inShowCurrentMonitorOnly)
{
	XfdashboardWorkspaceSelectorPrivate		*priv;
	ClutterActorIter						iter;
	ClutterActor							*child;
	XfdashboardStageInterface				*stageInterface;
	XfdashboardWindowTrackerMonitor			*monitor;

	g_return_if_fail(XFDASHBOARD_IS_WORKSPACE_SELECTOR(self));

	priv=self->priv;

	/* Set value if changed */
	if(priv->showCurrentMonitorOnly!=inShowCurrentMonitorOnly)
	{
		/* Set value */
		priv->showCurrentMonitorOnly=inShowCurrentMonitorOnly;

		/* Get parent stage interface */
		stageInterface=xfdashboard_get_stage_of_actor(CLUTTER_ACTOR(self));

		/* Get monitor of stage interface if available and if only windows
		 * of current monitor should be shown.
		 */
		monitor=NULL;
		if(stageInterface && priv->showCurrentMonitorOnly) monitor=xfdashboard_stage_interface_get_monitor(stageInterface);

		/* Iterate through workspace actors and update monitor */
		clutter_actor_iter_init(&iter, CLUTTER_ACTOR(self));
		while(clutter_actor_iter_next(&iter, &child))
		{
			if(XFDASHBOARD_IS_LIVE_WORKSPACE(child))
			{
				xfdashboard_live_workspace_set_monitor(XFDASHBOARD_LIVE_WORKSPACE(child), monitor);
			}
		}

		/* Notify about property change */
		g_object_notify_by_pspec(G_OBJECT(self), XfdashboardWorkspaceSelectorProperties[PROP_SHOW_CURRENT_MONITOR_ONLY]);
	}
}
