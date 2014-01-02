/*
 * workspace-selector: Workspace selector box
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

#include "workspace-selector.h"

#include <glib/gi18n-lib.h>
#include <math.h>

#include "enums.h"
#include "window-tracker.h"
#include "live-workspace.h"
#include "application.h"
#include "drop-action.h"
#include "windows-view.h"
#include "live-window.h"
#include "application-button.h"
#include "utils.h"

/* Define this class in GObject system */
G_DEFINE_TYPE(XfdashboardWorkspaceSelector,
				xfdashboard_workspace_selector,
				XFDASHBOARD_TYPE_BACKGROUND)
                                                
/* Private structure - access only by public API if needed */
#define XFDASHBOARD_WORKSPACE_SELECTOR_GET_PRIVATE(obj) \
	(G_TYPE_INSTANCE_GET_PRIVATE((obj), XFDASHBOARD_TYPE_WORKSPACE_SELECTOR, XfdashboardWorkspaceSelectorPrivate))

struct _XfdashboardWorkspaceSelectorPrivate
{
	/* Properties related */
	gfloat								scaleMin;
	gfloat								scaleMax;
	gfloat								scaleStep;

	gfloat								spacing;

	ClutterOrientation					orientation;

	/* Instance related */
	XfdashboardWindowTracker			*windowTracker;
	XfdashboardWindowTrackerWorkspace	*activeWorkspace;

	gfloat								scaleCurrent;
};

/* Properties */
enum
{
	PROP_0,

	PROP_SPACING,
	PROP_ORIENTATION,

	PROP_LAST
};

static GParamSpec* XfdashboardWorkspaceSelectorProperties[PROP_LAST]={ 0, };

/* IMPLEMENTATION: Private variables and methods */
#define DEFAULT_SCALE_MIN			0.1
#define DEFAULT_SCALE_MAX			1.0
#define DEFAULT_SCALE_STEP			0.1

#define DEFAULT_ORIENTATION			CLUTTER_ORIENTATION_VERTICAL	// TODO: Replace by settings/theming object

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

/* Get scale factor to fit all children into given width */
static gfloat _xfdashboard_workspace_selector_get_scale_for_width(XfdashboardWorkspaceSelector *self,
																	gfloat inForWidth,
																	gboolean inDoMinimumSize)
{
	XfdashboardWorkspaceSelectorPrivate		*priv;
	ClutterActor							*child;
	ClutterActorIter						iter;
	gint									numberChildren;
	gfloat									totalWidth, scalableWidth;
	gfloat									childWidth;
	gfloat									childMinWidth, childNaturalWidth;
	gfloat									scale;
	gboolean								recheckWidth;

	g_return_val_if_fail(XFDASHBOARD_IS_WORKSPACE_SELECTOR(self), 0.0f);
	g_return_val_if_fail(inForWidth>=0.0f, 0.0f);

	priv=self->priv;

	/* Count visible children and determine their total width */
	numberChildren=0;
	totalWidth=0.0f;
	clutter_actor_iter_init(&iter, CLUTTER_ACTOR(self));
	while(clutter_actor_iter_next(&iter, &child))
	{
		/* Only check visible children */
		if(!CLUTTER_ACTOR_IS_VISIBLE(child)) continue;

		/* Get width of child */
		clutter_actor_get_preferred_width(child, -1, &childMinWidth, &childNaturalWidth);
		if(inDoMinimumSize==TRUE) childWidth=childMinWidth;
			else childWidth=childNaturalWidth;

		/* Determine total size so far */
		totalWidth+=ceil(childWidth);

		/* Count visible children */
		numberChildren++;
	}
	if(numberChildren==0) return(priv->scaleMax);

	/* Determine scalable width. That is the width without spacing
	 * between children and the spacing used as padding.
	 */
	scalableWidth=inForWidth-((numberChildren+1)*priv->spacing);

	/* Get scale factor */
	scale=priv->scaleMax;
	if(totalWidth>0.0f)
	{
		scale=floor((scalableWidth/totalWidth)/priv->scaleStep)*priv->scaleStep;
		scale=MIN(scale, priv->scaleMax);
		scale=MAX(scale, priv->scaleMin);
	}

	/* Check if all visible children would really fit into width
	 * otherwise we need to decrease scale factor one step down
	 */
	if(scale>priv->scaleMin)
	{
		do
		{
			recheckWidth=FALSE;
			totalWidth=priv->spacing;

			/* Iterate through visible children and sum their scaled
			 * widths. The total width will be initialized with unscaled
			 * spacing and all visible children's scaled width will also
			 * be added with unscaled spacing to have the padding added.
			 */
			clutter_actor_iter_init(&iter, CLUTTER_ACTOR(self));
			while(clutter_actor_iter_next(&iter, &child))
			{
				/* Only check visible children */
				if(!CLUTTER_ACTOR_IS_VISIBLE(child)) continue;

				/* Get scaled size of child and add to total width */
				clutter_actor_get_preferred_width(child, -1, &childMinWidth, &childNaturalWidth);
				if(inDoMinimumSize==TRUE) childWidth=childMinWidth;
					else childWidth=childNaturalWidth;

				childWidth*=scale;
				totalWidth+=ceil(childWidth)+priv->spacing;
			}

			/* If total width is greater than given width
			 * decrease scale factor by one step and recheck
			 */
			if(totalWidth>inForWidth && scale>priv->scaleMin)
			{
				scale-=priv->scaleStep;
				recheckWidth=TRUE;
			}
		}
		while(recheckWidth==TRUE);
	}

	/* Return found scale factor */
	return(scale);
}

/* Get scale factor to fit all children into given height */
static gfloat _xfdashboard_workspace_selector_get_scale_for_height(XfdashboardWorkspaceSelector *self,
																	gfloat inForHeight,
																	gboolean inDoMinimumSize)
{
	XfdashboardWorkspaceSelectorPrivate		*priv;
	ClutterActor							*child;
	ClutterActorIter						iter;
	gint									numberChildren;
	gfloat									totalHeight, scalableHeight;
	gfloat									childHeight;
	gfloat									childMinHeight, childNaturalHeight;
	gfloat									scale;
	gboolean								recheckHeight;

	g_return_val_if_fail(XFDASHBOARD_IS_WORKSPACE_SELECTOR(self), 0.0f);
	g_return_val_if_fail(inForHeight>=0.0f, 0.0f);

	priv=self->priv;

	/* Count visible children and determine their total height */
	numberChildren=0;
	totalHeight=0.0f;
	clutter_actor_iter_init(&iter, CLUTTER_ACTOR(self));
	while(clutter_actor_iter_next(&iter, &child))
	{
		/* Only check visible children */
		if(!CLUTTER_ACTOR_IS_VISIBLE(child)) continue;

		/* Get height of child */
		clutter_actor_get_preferred_height(child, -1, &childMinHeight, &childNaturalHeight);
		if(inDoMinimumSize==TRUE) childHeight=childMinHeight;
			else childHeight=childNaturalHeight;

		/* Determine total size so far */
		totalHeight+=ceil(childHeight);

		/* Count visible children */
		numberChildren++;
	}
	if(numberChildren==0) return(priv->scaleMax);

	/* Determine scalable height. That is the height without spacing
	 * between children and the spacing used as padding.
	 */
	scalableHeight=inForHeight-((numberChildren+1)*priv->spacing);

	/* Get scale factor */
	scale=priv->scaleMax;
	if(totalHeight>0.0f)
	{
		scale=floor((scalableHeight/totalHeight)/priv->scaleStep)*priv->scaleStep;
		scale=MIN(scale, priv->scaleMax);
		scale=MAX(scale, priv->scaleMin);
	}

	/* Check if all visible children would really fit into height
	 * otherwise we need to decrease scale factor one step down
	 */
	if(scale>priv->scaleMin)
	{
		do
		{
			recheckHeight=FALSE;
			totalHeight=priv->spacing;

			/* Iterate through visible children and sum their scaled
			 * heights. The total height will be initialized with unscaled
			 * spacing and all visible children's scaled height will also
			 * be added with unscaled spacing to have the padding added.
			 */
			clutter_actor_iter_init(&iter, CLUTTER_ACTOR(self));
			while(clutter_actor_iter_next(&iter, &child))
			{
				/* Only check visible children */
				if(!CLUTTER_ACTOR_IS_VISIBLE(child)) continue;

				/* Get scaled size of child and add to total height */
				clutter_actor_get_preferred_height(child, -1, &childMinHeight, &childNaturalHeight);
				if(inDoMinimumSize==TRUE) childHeight=childMinHeight;
					else childHeight=childNaturalHeight;

				childHeight*=scale;
				totalHeight+=ceil(childHeight)+priv->spacing;
			}

			/* If total height is greater than given height
			 * decrease scale factor by one step and recheck
			 */
			if(totalHeight>inForHeight && scale>priv->scaleMin)
			{
				scale-=priv->scaleStep;
				recheckHeight=TRUE;
			}
		}
		while(recheckHeight==TRUE);
	}

	/* Return found scale factor */
	return(scale);
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

	/* Check if we can handle dragged actor from given source */
	if(XFDASHBOARD_IS_WINDOWS_VIEW(dragSource) &&
		XFDASHBOARD_IS_LIVE_WINDOW(draggedActor))
	{
		canHandle=TRUE;
	}

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
	if(XFDASHBOARD_IS_LIVE_WINDOW(draggedActor))
	{
		XfdashboardWindowTrackerWindow	*window;

		/* Get window */
		window=xfdashboard_live_window_get_window(XFDASHBOARD_LIVE_WINDOW(draggedActor));
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
	xfdashboard_application_quit();
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
	ClutterActor		*actor;
	gint				index;
	ClutterAction		*action;

	g_return_if_fail(XFDASHBOARD_IS_WORKSPACE_SELECTOR(self));
	g_return_if_fail(XFDASHBOARD_IS_WINDOW_TRACKER_WORKSPACE(inWorkspace));

	/* Get index of workspace for insertion */
	index=xfdashboard_window_tracker_workspace_get_number(inWorkspace);

	/* Create new live workspace actor and insert at index */
	actor=xfdashboard_live_workspace_new_for_workspace(inWorkspace);
	xfdashboard_background_set_outline_color(XFDASHBOARD_BACKGROUND(actor), CLUTTER_COLOR_White);
	xfdashboard_background_set_outline_width(XFDASHBOARD_BACKGROUND(actor), 4.0f);
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
		if(liveWorkspace)
		{
			xfdashboard_background_set_background_type(XFDASHBOARD_BACKGROUND(liveWorkspace), XFDASHBOARD_BACKGROUND_TYPE_NONE);
		}

		priv->activeWorkspace=NULL;
	}

	/* Mark new active workspace */
	workspace=xfdashboard_window_tracker_get_active_workspace(priv->windowTracker);
	if(workspace)
	{
		priv->activeWorkspace=workspace;

		liveWorkspace=_xfdashboard_workspace_selector_find_actor_for_workspace(self, priv->activeWorkspace);
		if(liveWorkspace)
		{
			xfdashboard_background_set_background_type(XFDASHBOARD_BACKGROUND(liveWorkspace), XFDASHBOARD_BACKGROUND_TYPE_OUTLINE);
		}
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

	g_return_val_if_fail(XFDASHBOARD_IS_WORKSPACE_SELECTOR(inActor), FALSE);
	g_return_val_if_fail(inEvent, FALSE);

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
			g_debug("Cannot handle scroll direction %d in %s", clutter_event_get_scroll_direction(inEvent), G_OBJECT_TYPE_NAME(self));
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
	gfloat									scale;

	/* Set up default values */
	minHeight=naturalHeight=0.0f;

	/* Determine height for horizontal orientation ... */
	if(priv->orientation==CLUTTER_ORIENTATION_HORIZONTAL)
	{
		/* Iterate through visible children and determine heights */
		numberChildren=0;
		clutter_actor_iter_init(&iter, CLUTTER_ACTOR(inActor));
		while(clutter_actor_iter_next(&iter, &child))
		{
			/* Only check visible children */
			if(!CLUTTER_ACTOR_IS_VISIBLE(child)) continue;

			/* Get sizes of child */
			clutter_actor_get_preferred_height(child,
												-1,
												&childMinHeight,
												&childNaturalHeight);

			/* Determine heights */
			minHeight=MAX(minHeight, childMinHeight);
			naturalHeight=MAX(naturalHeight, childNaturalHeight);

			/* Count visible children */
			numberChildren++;
		}

		/* Check if we need to scale width because of the need to fit
		 * all visible children into given limiting width
		 */
		if(inForWidth>=0.0f)
		{
			scale=_xfdashboard_workspace_selector_get_scale_for_width(self, inForWidth, TRUE);
			minHeight*=scale;

			scale=_xfdashboard_workspace_selector_get_scale_for_width(self, inForWidth, FALSE);
			naturalHeight*=scale;
		}

		/* Add spacing as padding */
		if(numberChildren>0)
		{
			minHeight+=2*priv->spacing;
			naturalHeight+=2*priv->spacing;
		}
	}
		/* ... otherwise determine height for vertical orientation */
		else
		{
			/* Iterate through visible children and determine height */
			numberChildren=0;
			clutter_actor_iter_init(&iter, CLUTTER_ACTOR(inActor));
			while(clutter_actor_iter_next(&iter, &child))
			{
				/* Only check visible children */
				if(!CLUTTER_ACTOR_IS_VISIBLE(child)) continue;

				/* Get child's size */
				clutter_actor_get_preferred_height(child,
													inForWidth,
													&childMinHeight,
													&childNaturalHeight);

				/* Determine heights */
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
	gfloat									scale;

	/* Set up default values */
	minWidth=naturalWidth=0.0f;

	/* Determine width for horizontal orientation ... */
	if(priv->orientation==CLUTTER_ORIENTATION_HORIZONTAL)
	{
		/* Iterate through visible children and determine width */
		numberChildren=0;
		clutter_actor_iter_init(&iter, CLUTTER_ACTOR(inActor));
		while(clutter_actor_iter_next(&iter, &child))
		{
			/* Only check visible children */
			if(!CLUTTER_ACTOR_IS_VISIBLE(child)) continue;

			/* Get child's size */
			clutter_actor_get_preferred_width(child,
												inForHeight,
												&childMinWidth,
												&childNaturalWidth);

			/* Determine widths */
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
		/* ... otherwise determine width for vertical orientation */
		else
		{
			/* Iterate through visible children and determine widths */
			numberChildren=0;
			clutter_actor_iter_init(&iter, CLUTTER_ACTOR(inActor));
			while(clutter_actor_iter_next(&iter, &child))
			{
				/* Only check visible children */
				if(!CLUTTER_ACTOR_IS_VISIBLE(child)) continue;

				/* Get sizes of child */
				clutter_actor_get_preferred_width(child,
													-1,
													&childMinWidth,
													&childNaturalWidth);

				/* Determine widths */
				minWidth=MAX(minWidth, childMinWidth);
				naturalWidth=MAX(naturalWidth, childNaturalWidth);

				/* Count visible children */
				numberChildren++;
			}

			/* Check if we need to scale width because of the need to fit
			 * all visible children into given limiting height
			 */
			if(inForHeight>=0.0f)
			{
				scale=_xfdashboard_workspace_selector_get_scale_for_height(self, inForHeight, TRUE);
				minWidth*=scale;

				scale=_xfdashboard_workspace_selector_get_scale_for_height(self, inForHeight, FALSE);
				naturalWidth*=scale;
			}

			/* Add spacing as padding */
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

	/* Find scaling to get all children fit the allocation */
	priv->scaleCurrent=_xfdashboard_workspace_selector_get_scale_for_height(self, availableHeight, FALSE);

	/* Calculate new position and size of visible children */
	childAllocation.x1=childAllocation.y1=priv->spacing;
	clutter_actor_iter_init(&iter, CLUTTER_ACTOR(inActor));
	while(clutter_actor_iter_next(&iter, &child))
	{
		/* Is child visible? */
		if(!CLUTTER_ACTOR_IS_VISIBLE(child)) continue;

		/* Calculate new position and size of child but respect scaling */
		clutter_actor_get_preferred_size(child, NULL, NULL, &childWidth, &childHeight);
		childWidth*=priv->scaleCurrent;
		childHeight*=priv->scaleCurrent;

		if(priv->orientation==CLUTTER_ORIENTATION_HORIZONTAL)
		{
			childAllocation.y1=ceil(MAX(((availableHeight-childHeight))/2.0f, priv->spacing));
			childAllocation.y2=ceil(childAllocation.y1+childHeight);
			childAllocation.x2=ceil(childAllocation.x1+childWidth);
		}
			else
			{
				childAllocation.x1=ceil(MAX(((availableWidth-childWidth))/2.0f, priv->spacing));
				childAllocation.x2=ceil(childAllocation.x1+childWidth);
				childAllocation.y2=ceil(childAllocation.y1+childHeight);
			}

		clutter_actor_allocate(child, &childAllocation, inFlags);

		/* Set up for next child */
		if(priv->orientation==CLUTTER_ORIENTATION_HORIZONTAL) childAllocation.x1=ceil(childAllocation.x1+childWidth+priv->spacing);
			else childAllocation.y1=ceil(childAllocation.y1+childHeight+priv->spacing);
	}
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
	ClutterActorClass	*actorClass=CLUTTER_ACTOR_CLASS(klass);
	GObjectClass		*gobjectClass=G_OBJECT_CLASS(klass);

	/* Override functions */
	gobjectClass->dispose=_xfdashboard_workspace_selector_dispose;
	gobjectClass->set_property=_xfdashboard_workspace_selector_set_property;
	gobjectClass->get_property=_xfdashboard_workspace_selector_get_property;

	actorClass->get_preferred_width=_xfdashboard_workspace_selector_get_preferred_width;
	actorClass->get_preferred_height=_xfdashboard_workspace_selector_get_preferred_height;
	actorClass->allocate=_xfdashboard_workspace_selector_allocate;

	/* Set up private structure */
	g_type_class_add_private(klass, sizeof(XfdashboardWorkspaceSelectorPrivate));

	/* Define properties */
	XfdashboardWorkspaceSelectorProperties[PROP_SPACING]=
		g_param_spec_float("spacing",
								_("Spacing"),
								_("The spacing between children"),
								0.0, G_MAXFLOAT,
								0.0,
								G_PARAM_READWRITE);

	XfdashboardWorkspaceSelectorProperties[PROP_ORIENTATION]=
		g_param_spec_enum("orientation",
							_("Orientation"),
							_("The orientation to layout children"),
							CLUTTER_TYPE_ORIENTATION,
							DEFAULT_ORIENTATION,
							G_PARAM_READWRITE);

	g_object_class_install_properties(gobjectClass, PROP_LAST, XfdashboardWorkspaceSelectorProperties);
}

/* Object initialization
 * Create private structure and set up default values
 */
static void xfdashboard_workspace_selector_init(XfdashboardWorkspaceSelector *self)
{
	XfdashboardWorkspaceSelectorPrivate	*priv;
	ClutterRequestMode				requestMode;

	priv=self->priv=XFDASHBOARD_WORKSPACE_SELECTOR_GET_PRIVATE(self);

	/* Set up default values */
	priv->windowTracker=xfdashboard_window_tracker_get_default();
	priv->activeWorkspace=NULL;
	priv->spacing=0.0f;
	priv->orientation=DEFAULT_ORIENTATION;
	priv->scaleCurrent=DEFAULT_SCALE_MAX;
	priv->scaleMin=DEFAULT_SCALE_MIN;
	priv->scaleMax=DEFAULT_SCALE_MAX;
	priv->scaleStep=DEFAULT_SCALE_STEP;

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
	XfdashboardWorkspaceSelectorPrivate	*priv;

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
	XfdashboardWorkspaceSelectorPrivate	*priv;
	ClutterRequestMode				requestMode;

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
