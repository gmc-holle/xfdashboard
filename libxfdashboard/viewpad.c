/*
 * viewpad: A viewpad managing views
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

#include <libxfdashboard/viewpad.h>

#include <glib/gi18n-lib.h>
#include <gtk/gtk.h>
#include <math.h>

#include <libxfdashboard/view-manager.h>
#include <libxfdashboard/scrollbar.h>
#include <libxfdashboard/enums.h>
#include <libxfdashboard/utils.h>
#include <libxfdashboard/focusable.h>
#include <libxfdashboard/focus-manager.h>
#include <libxfdashboard/compat.h>
#include <libxfdashboard/debug.h>


/* Define this class in GObject system */
static void _xfdashboard_viewpad_focusable_iface_init(XfdashboardFocusableInterface *iface);

struct _XfdashboardViewpadPrivate
{
	/* Properties related */
	gfloat							spacing;
	XfdashboardView					*activeView;
	XfdashboardVisibilityPolicy		hScrollbarPolicy;
	gboolean						hScrollbarVisible;
	XfdashboardVisibilityPolicy		vScrollbarPolicy;
	gboolean						vScrollbarVisible;

	/* Instance related */
	XfdashboardViewManager			*viewManager;

	ClutterLayoutManager			*layout;
	ClutterActor					*container;
	ClutterActor					*hScrollbar;
	ClutterActor					*vScrollbar;

	guint							scrollbarUpdateID;

	gboolean						doRegisterFocusableViews;

	ClutterActorBox					*lastAllocation;
};

G_DEFINE_TYPE_WITH_CODE(XfdashboardViewpad,
						xfdashboard_viewpad,
						XFDASHBOARD_TYPE_BACKGROUND,
						G_ADD_PRIVATE(XfdashboardViewpad)
						G_IMPLEMENT_INTERFACE(XFDASHBOARD_TYPE_FOCUSABLE, _xfdashboard_viewpad_focusable_iface_init))

/* Properties */
enum
{
	PROP_0,

	PROP_SPACING,
	PROP_ACTIVE_VIEW,
	PROP_HSCROLLBAR_POLICY,
	PROP_HSCROLLBAR_VISIBLE,
	PROP_VSCROLLBAR_POLICY,
	PROP_VSCROLLBAR_VISIBLE,

	PROP_LAST
};

static GParamSpec* XfdashboardViewpadProperties[PROP_LAST]={ 0, };

/* Signals */
enum
{
	SIGNAL_VIEW_ADDED,
	SIGNAL_VIEW_REMOVED,

	SIGNAL_VIEW_ACTIVATING,
	SIGNAL_VIEW_ACTIVATED,
	SIGNAL_VIEW_DEACTIVATING,
	SIGNAL_VIEW_DEACTIVATED,

	SIGNAL_LAST
};

static guint XfdashboardViewpadSignals[SIGNAL_LAST]={ 0, };

/* Forward declaration */
static void _xfdashboard_viewpad_allocate(ClutterActor *self, const ClutterActorBox *inBox, ClutterAllocationFlags inFlags);

/* IMPLEMENTATION: Private variables and methods */

/* Update view depending on scrollbar values */
static void _xfdashboard_viewpad_update_view_viewport(XfdashboardViewpad *self)
{
	XfdashboardViewpadPrivate	*priv;
	ClutterMatrix				transform;
	gfloat						x, y, w, h;

	g_return_if_fail(XFDASHBOARD_IS_VIEWPAD(self));

	priv=self->priv;

	/* Check for active view */
	if(priv->activeView==NULL)
	{
		g_warning(_("Cannot update viewport of view because no one is active"));
		return;
	}

	/* Get offset from scrollbars and view size from clipping */
	if(clutter_actor_has_clip(CLUTTER_ACTOR(priv->activeView)))
	{
		clutter_actor_get_clip(CLUTTER_ACTOR(priv->activeView), &x, &y, &w, &h);
	}
		else
		{
 			x=y=0.0f;
			clutter_actor_get_size(CLUTTER_ACTOR(priv->activeView), &w, &h);
		}

	/* To avoid blur convert float to ints (virtually) */
	x=ceil(x);
	y=ceil(y);
	w=ceil(w);
	h=ceil(h);

	/* Set transformation (offset) */
	cogl_matrix_init_identity(&transform);
	cogl_matrix_translate(&transform, -x, -y, 0.0f);
	clutter_actor_set_transform(CLUTTER_ACTOR(priv->activeView), &transform);

	/* Set new clipping */
	clutter_actor_set_clip(CLUTTER_ACTOR(priv->activeView), x, y, w, h);
}

/* The value of a scrollbar has changed */
static void _xfdashboard_viewpad_on_scrollbar_value_changed(XfdashboardViewpad *self,
															gfloat inValue,
															gpointer inUserData)
{
	XfdashboardViewpadPrivate	*priv;
	ClutterActor				*scrollbar;
	gfloat						x, y, w, h;

	g_return_if_fail(XFDASHBOARD_IS_VIEWPAD(self));
	g_return_if_fail(XFDASHBOARD_IS_SCROLLBAR(inUserData));

	priv=self->priv;
	scrollbar=CLUTTER_ACTOR(inUserData);

	/* Update clipping */
	if(clutter_actor_has_clip(CLUTTER_ACTOR(priv->activeView)))
	{
		clutter_actor_get_clip(CLUTTER_ACTOR(priv->activeView), &x, &y, &w, &h);
		if(scrollbar==priv->hScrollbar) x=inValue;
			else if(scrollbar==priv->vScrollbar) y=inValue;
	}
		else
		{
			x=y=0.0f;
			clutter_actor_get_size(CLUTTER_ACTOR(priv->activeView), &w, &h);
		}
	clutter_actor_set_clip(CLUTTER_ACTOR(priv->activeView), x, y, w, h);

	/* Update viewport */
	_xfdashboard_viewpad_update_view_viewport(self);
}

/* Allocation of a view changed */
static void _xfdashboard_viewpad_update_scrollbars(XfdashboardViewpad *self)
{
	XfdashboardViewpadPrivate	*priv;
	gfloat						w, h;

	g_return_if_fail(XFDASHBOARD_IS_VIEWPAD(self));

	priv=self->priv;

	/* Set range of scroll bar to width and height of active view
	 * But we need to check for nan-values here - I do not get rid of it :(
	 */
	if(priv->activeView) clutter_actor_get_size(CLUTTER_ACTOR(priv->activeView), &w, &h);
		else w=h=1.0f;

	xfdashboard_scrollbar_set_range(XFDASHBOARD_SCROLLBAR(priv->vScrollbar), isnan(h)==0 ? h : 0.0f);

	/* If any scroll bar policy is automatic then reallocate the
	 * same allocation again in an unkindly way to force a recalculation
	 * if scroll bars needed to shown (or hidden what is unlikely)
	 */
	if(clutter_actor_is_visible(CLUTTER_ACTOR(self)) &&
		(priv->hScrollbarPolicy==XFDASHBOARD_VISIBILITY_POLICY_AUTOMATIC ||
			priv->vScrollbarPolicy==XFDASHBOARD_VISIBILITY_POLICY_AUTOMATIC))
	{
		ClutterActorBox			box;

		clutter_actor_get_allocation_box(CLUTTER_ACTOR(self), &box);
		_xfdashboard_viewpad_allocate(CLUTTER_ACTOR(self), &box, CLUTTER_DELEGATE_LAYOUT);
	}
}

/* Set new active view and deactive current one */
static void _xfdashboard_viewpad_activate_view(XfdashboardViewpad *self, XfdashboardView *inView)
{
	XfdashboardViewpadPrivate	*priv;
	gfloat						x, y;
	XfdashboardFocusManager		*focusManager;
	gboolean					hasFocus;

	g_return_if_fail(XFDASHBOARD_IS_VIEWPAD(self));
	g_return_if_fail(inView==NULL || XFDASHBOARD_IS_VIEW(inView));

	priv=self->priv;
	hasFocus=FALSE;

	/* Only set value if it changes */
	if(inView==priv->activeView) return;

	/* Check if view is a child of this actor */
	if(inView && clutter_actor_contains(CLUTTER_ACTOR(self), CLUTTER_ACTOR(inView))==FALSE)
	{
		g_warning(_("View %s is not a child of %s and cannot be activated"),
					G_OBJECT_TYPE_NAME(inView), G_OBJECT_TYPE_NAME(self));
		return;
	}

	/* Only allow enabled views to be activated */
	if(inView && !xfdashboard_view_get_enabled(inView))
	{
		g_warning(_("Cannot activate disabled view %s at %s"),
					G_OBJECT_TYPE_NAME(inView), G_OBJECT_TYPE_NAME(self));
		return;
	}

	/* Determine if this viewpad has the focus because we have to move focus in this case */
	focusManager=xfdashboard_focus_manager_get_default();

	/* Deactivate current view */
	if(priv->activeView)
	{
		/* Unset focus at current active view if this view has the focus */
		hasFocus=xfdashboard_focus_manager_has_focus(focusManager, XFDASHBOARD_FOCUSABLE(priv->activeView));
		if(hasFocus)
		{
			xfdashboard_focusable_unset_focus(XFDASHBOARD_FOCUSABLE(priv->activeView));
			XFDASHBOARD_DEBUG(self, ACTOR,
								"Unset focus from view '%s' because it is the active view at viewpad",
								xfdashboard_view_get_name(priv->activeView));
		}

		/* Hide current view and emit signal before and after deactivation */
		g_signal_emit(self, XfdashboardViewpadSignals[SIGNAL_VIEW_DEACTIVATING], 0, priv->activeView);
		g_signal_emit_by_name(priv->activeView, "deactivating");

		clutter_actor_hide(CLUTTER_ACTOR(priv->activeView));
		XFDASHBOARD_DEBUG(self, ACTOR,
							"Deactivated view %s",
							G_OBJECT_TYPE_NAME(priv->activeView));

		g_signal_emit_by_name(priv->activeView, "deactivated");
		g_signal_emit(self, XfdashboardViewpadSignals[SIGNAL_VIEW_DEACTIVATED], 0, priv->activeView);

		g_object_unref(priv->activeView);
		priv->activeView=NULL;
	}

	/* Activate new view (if available) by showing new view, setting up
	 * scrollbars and emitting signal before and after activation.
	 * Prevent signal handling for scrollbars' "value-changed" as it will
	 * mess up with clipping and viewport. We only need to set value of
	 * scrollbars but we do not need to handle the changed value.
	 */
	if(inView)
	{
		priv->activeView=g_object_ref(inView);

		g_signal_emit(self, XfdashboardViewpadSignals[SIGNAL_VIEW_ACTIVATING], 0, priv->activeView);
		g_signal_emit_by_name(priv->activeView, "activating");

		g_signal_handlers_block_by_func(priv->hScrollbar, _xfdashboard_viewpad_on_scrollbar_value_changed, self);
		g_signal_handlers_block_by_func(priv->vScrollbar, _xfdashboard_viewpad_on_scrollbar_value_changed, self);

		x=y=0.0f;
		clutter_actor_get_clip(CLUTTER_ACTOR(priv->activeView), &x, &y, NULL, NULL);
		_xfdashboard_viewpad_update_scrollbars(self);
		xfdashboard_scrollbar_set_value(XFDASHBOARD_SCROLLBAR(priv->hScrollbar), x);
		xfdashboard_scrollbar_set_value(XFDASHBOARD_SCROLLBAR(priv->vScrollbar), y);
		_xfdashboard_viewpad_update_view_viewport(self);
		clutter_actor_show(CLUTTER_ACTOR(priv->activeView));
		XFDASHBOARD_DEBUG(self, ACTOR,
							"Activated view %s",
							G_OBJECT_TYPE_NAME(priv->activeView));

		g_signal_handlers_unblock_by_func(priv->hScrollbar, _xfdashboard_viewpad_on_scrollbar_value_changed, self);
		g_signal_handlers_unblock_by_func(priv->vScrollbar, _xfdashboard_viewpad_on_scrollbar_value_changed, self);

		g_signal_emit_by_name(priv->activeView, "activated");
		g_signal_emit(self, XfdashboardViewpadSignals[SIGNAL_VIEW_ACTIVATED], 0, priv->activeView);

		/* Set focus to new active view if this viewpad has the focus */
		if(hasFocus)
		{
			xfdashboard_focus_manager_set_focus(focusManager, XFDASHBOARD_FOCUSABLE(priv->activeView));
			XFDASHBOARD_DEBUG(self, ACTOR,
								"The previous active view at viewpad had focus so set focus to new active view '%s'",
								xfdashboard_view_get_name(priv->activeView));
		}
	}

	/* If no view is active at this time move focus to next focusable actor
	 * if this viewpad has the focus.
	 */
	if(hasFocus && !priv->activeView)
	{
		XfdashboardFocusable	*newFocusable;

		newFocusable=xfdashboard_focus_manager_get_next_focusable(focusManager, XFDASHBOARD_FOCUSABLE(self));
		if(newFocusable)
		{
			xfdashboard_focus_manager_set_focus(focusManager, newFocusable);
			XFDASHBOARD_DEBUG(self, ACTOR,
								"Viewpad has focus but no view is active so move focus to next focusable actor of type '%s'",
								G_OBJECT_TYPE_NAME(newFocusable));
		}
	}

	/* Release allocated resources */
	if(focusManager) g_object_unref(focusManager);

	/* Notify about property change */
	g_object_notify_by_pspec(G_OBJECT(self), XfdashboardViewpadProperties[PROP_ACTIVE_VIEW]);
}

/* A view was disabled */
static void _xfdashboard_viewpad_on_view_disabled(XfdashboardViewpad *self, XfdashboardView *inView)
{
	XfdashboardViewpadPrivate	*priv;
	ClutterActorIter			iter;
	ClutterActor				*child;
	XfdashboardView				*firstActivatableView;

	g_return_if_fail(XFDASHBOARD_IS_VIEWPAD(self));
	g_return_if_fail(XFDASHBOARD_IS_VIEW(inView));

	priv=self->priv;
	firstActivatableView=NULL;

	/* If the currently disabled view is the active one, activate a next available view */
	if(inView==priv->activeView)
	{
		/* Iterate through create views and lookup view of given type */
		clutter_actor_iter_init(&iter, CLUTTER_ACTOR(self));
		while(clutter_actor_iter_next(&iter, &child))
		{
			/* Check if child is a view otherwise continue iterating */
			if(XFDASHBOARD_IS_VIEW(child)!=TRUE) continue;

			/* If child is not the view being disabled check if it could
			 * become the next activatable view
			 * the first activatable view after we destroyed all views found.
			 */
			if(XFDASHBOARD_VIEW(child)!=inView &&
				xfdashboard_view_get_enabled(XFDASHBOARD_VIEW(child)))
			{
				firstActivatableView=XFDASHBOARD_VIEW(child);
			}
		}

		/* Now activate the first activatable view we found during iteration.
		 * It can also be no view (NULL pointer).
		 */
		XFDASHBOARD_DEBUG(self, ACTOR,
							"Disabled view %s was the active view in %s - will activate %s",
							G_OBJECT_TYPE_NAME(inView),
							G_OBJECT_TYPE_NAME(self),
							firstActivatableView ? G_OBJECT_TYPE_NAME(firstActivatableView) : "no other view");
		_xfdashboard_viewpad_activate_view(self, firstActivatableView);
	}
}

/* A view was enabled */
static void _xfdashboard_viewpad_on_view_enabled(XfdashboardViewpad *self, XfdashboardView *inView)
{
	XfdashboardViewpadPrivate	*priv;

	g_return_if_fail(XFDASHBOARD_IS_VIEWPAD(self));
	g_return_if_fail(XFDASHBOARD_IS_VIEW(inView));

	priv=self->priv;

	/* If no view is active this new enabled view will be activated  */
	if(!priv->activeView) _xfdashboard_viewpad_activate_view(self, inView);
}

/* Allocation of a view changed */
static gboolean _xfdashboard_viewpad_on_allocation_changed_repaint_callback(gpointer inUserData)
{
	XfdashboardViewpad			*self;
	XfdashboardViewpadPrivate	*priv;

	g_return_val_if_fail(XFDASHBOARD_IS_VIEWPAD(inUserData), G_SOURCE_REMOVE);

	self=XFDASHBOARD_VIEWPAD(inUserData);
	priv=self->priv;

	/* Update scrollbars */
	_xfdashboard_viewpad_update_scrollbars(self);

	/* Ensure view is visible */
	_xfdashboard_viewpad_on_scrollbar_value_changed(self,
													xfdashboard_scrollbar_get_value(XFDASHBOARD_SCROLLBAR(priv->hScrollbar)),
													priv->hScrollbar);
	_xfdashboard_viewpad_on_scrollbar_value_changed(self,
													xfdashboard_scrollbar_get_value(XFDASHBOARD_SCROLLBAR(priv->vScrollbar)),
													priv->vScrollbar);

	/* Do not call this callback again */
	priv->scrollbarUpdateID=0;
	return(G_SOURCE_REMOVE);
}

static void _xfdashboard_viewpad_on_allocation_changed(ClutterActor *inActor,
														ClutterActorBox *inBox,
														ClutterAllocationFlags inFlags,
														gpointer inUserData)
{
	XfdashboardViewpad			*self;
	XfdashboardViewpadPrivate	*priv;
	XfdashboardView				*view G_GNUC_UNUSED;

	g_return_if_fail(XFDASHBOARD_IS_VIEWPAD(inActor));
	g_return_if_fail(XFDASHBOARD_IS_VIEW(inUserData));

	self=XFDASHBOARD_VIEWPAD(inActor);
	priv=self->priv;
	view=XFDASHBOARD_VIEW(inUserData);

	/* Defer updating scrollbars but only if view whose allocation
	 * has changed is the active one
	 */
	if(priv->scrollbarUpdateID==0)
	{
		priv->scrollbarUpdateID=
			clutter_threads_add_repaint_func_full(CLUTTER_REPAINT_FLAGS_QUEUE_REDRAW_ON_ADD | CLUTTER_REPAINT_FLAGS_POST_PAINT,
													_xfdashboard_viewpad_on_allocation_changed_repaint_callback,
													self,
													NULL);
	}
}

/* Scroll to requested position in view */
static void _xfdashboard_viewpad_on_view_scroll_to(XfdashboardViewpad *self,
													gfloat inX,
													gfloat inY,
													gpointer inUserData)
{
	XfdashboardViewpadPrivate	*priv;
	XfdashboardView				*view;
	gfloat						x, y, w, h;

	g_return_if_fail(XFDASHBOARD_IS_VIEWPAD(self));
	g_return_if_fail(XFDASHBOARD_IS_VIEW(inUserData));

	priv=self->priv;
	view=XFDASHBOARD_VIEW(inUserData);

	/* If to-scroll view is the active view in viewpad
	 * just set scrollbar value to the new ones
	 */
	if(view==priv->activeView)
	{
		if(inX>=0.0f) xfdashboard_scrollbar_set_value(XFDASHBOARD_SCROLLBAR(priv->hScrollbar), inX);
		if(inY>=0.0f) xfdashboard_scrollbar_set_value(XFDASHBOARD_SCROLLBAR(priv->vScrollbar), inY);
	}
		/* If to-scroll view is not the active one update its clipping */
		else
		{
			if(clutter_actor_has_clip(CLUTTER_ACTOR(view)))
			{
				clutter_actor_get_clip(CLUTTER_ACTOR(view), &x, &y, &w, &h);
				if(inX>=0.0f) x=inX;
				if(inY>=0.0f) y=inY;
			}
				else
				{
					x=y=0.0f;
					clutter_actor_get_size(CLUTTER_ACTOR(view), &w, &h);
				}
			clutter_actor_set_clip(CLUTTER_ACTOR(view), x, y, w, h);
		}
}

/* Determine if scrolling is needed to get requested actor visible in viewpad and
 * return the distance in x and y direction if scrolling is needed.
 */
static gboolean _xfdashboard_viewpad_view_needs_scrolling_for_child(XfdashboardViewpad *self,
																	XfdashboardView *inView,
																	ClutterActor *inViewChild,
																	gfloat *outScrollX,
																	gfloat *outScrollY)
{
	XfdashboardViewpadPrivate	*priv;
	ClutterVertex				origin;
	ClutterVertex				transformedUpperLeft;
	ClutterVertex				transformedLowerRight;
	gfloat						x, y, w, h;
	gboolean					viewFitsIntoViewpad;
	gboolean					needScrolling;
	gfloat						scrollX, scrollY;
	gfloat						viewpadWidth, viewpadHeight;

	g_return_val_if_fail(XFDASHBOARD_IS_VIEWPAD(self), FALSE);
	g_return_val_if_fail(XFDASHBOARD_IS_VIEW(inView), FALSE);
	g_return_val_if_fail(CLUTTER_IS_ACTOR(inViewChild), FALSE);

	priv=self->priv;
	viewFitsIntoViewpad=FALSE;
	needScrolling=FALSE;
	scrollX=scrollY=0.0f;

	/* Check if view would fit into this viewpad completely */
	if(priv->lastAllocation)
	{
		viewpadWidth=clutter_actor_box_get_width(priv->lastAllocation);
		viewpadHeight=clutter_actor_box_get_height(priv->lastAllocation);

		clutter_actor_get_size(CLUTTER_ACTOR(inView), &w, &h);
		if(w<=viewpadWidth && h<=viewpadHeight) viewFitsIntoViewpad=TRUE;
	}
		else
		{
			clutter_actor_get_size(CLUTTER_ACTOR(self), &viewpadWidth, &viewpadHeight);
		}

	/* Get position and size of view but respect scrolled position */
	if(inView==priv->activeView)
	{
		x=xfdashboard_scrollbar_get_value(XFDASHBOARD_SCROLLBAR(priv->hScrollbar));
		y=xfdashboard_scrollbar_get_value(XFDASHBOARD_SCROLLBAR(priv->vScrollbar));
		w=viewpadWidth;
		h=viewpadHeight;
	}
		else
		{
			if(clutter_actor_has_clip(CLUTTER_ACTOR(inView)))
			{
				clutter_actor_get_clip(CLUTTER_ACTOR(inView), &x, &y, &w, &h);
			}
				else
				{
					x=y=0.0f;
					clutter_actor_get_size(CLUTTER_ACTOR(inView), &w, &h);
				}
		}

	/* Check that upper left point of actor is visible otherwise set flag for scrolling */
	if(!viewFitsIntoViewpad)
	{
		origin.x=origin.y=origin.z=0.0f;
		clutter_actor_apply_relative_transform_to_point(inViewChild, CLUTTER_ACTOR(inView), &origin, &transformedUpperLeft);
	}
		else
		{
			origin.x=origin.y=origin.z=0.0f;
			clutter_actor_apply_relative_transform_to_point(CLUTTER_ACTOR(inView), CLUTTER_ACTOR(self), &origin, &transformedUpperLeft);
		}

	if(transformedUpperLeft.x<x ||
		transformedUpperLeft.x>(x+w) ||
		transformedUpperLeft.y<y ||
		transformedUpperLeft.y>(y+h))
	{
		needScrolling=TRUE;
	}

	/* Check that lower right point of actor is visible otherwise set flag for scrolling */
	if(!viewFitsIntoViewpad)
	{
		origin.z=0.0f;
		clutter_actor_get_size(inViewChild, &origin.x, &origin.y);
		clutter_actor_apply_relative_transform_to_point(inViewChild, CLUTTER_ACTOR(inView), &origin, &transformedLowerRight);
	}
		else
		{
			origin.x=clutter_actor_box_get_width(priv->lastAllocation);
			origin.y=clutter_actor_box_get_height(priv->lastAllocation);
			origin.z=0.0f;
			clutter_actor_apply_relative_transform_to_point(CLUTTER_ACTOR(inView), CLUTTER_ACTOR(self), &origin, &transformedLowerRight);
		}

	if(transformedLowerRight.x<x ||
		transformedLowerRight.x>(x+w) ||
		transformedLowerRight.y<y ||
		transformedLowerRight.y>(y+h))
	{
		needScrolling=TRUE;
	}

	/* Check if we need to scroll */
	if(needScrolling)
	{
		gfloat					distanceUpperLeft;
		gfloat					distanceLowerRight;

		/* Find shortest way to scroll and then scroll */
		distanceUpperLeft=sqrtf(powf(transformedUpperLeft.x-x, 2.0f)+powf(transformedUpperLeft.y-y, 2.0f));
		distanceLowerRight=sqrtf(powf(transformedLowerRight.x-(x+w), 2.0f)+powf(transformedLowerRight.y-(y+h), 2.0f));

		if(distanceUpperLeft<=distanceLowerRight)
		{
			scrollX=transformedUpperLeft.x;
			scrollY=transformedUpperLeft.y;
		}
			else
			{
				scrollX=transformedUpperLeft.x;
				scrollY=transformedLowerRight.y-h;
			}
	}

	/* Store values computed */
	if(outScrollX) *outScrollX=scrollX;
	if(outScrollY) *outScrollY=scrollY;

	/* Return TRUE if scrolling is needed otherwise FALSE */
	return(needScrolling);
}

/* Determine if scrolling is needed to get requested actor visible in viewpad */
static gboolean _xfdashboard_viewpad_on_view_child_needs_scroll(XfdashboardViewpad *self,
																ClutterActor *inActor,
																gpointer inUserData)
{
	XfdashboardView				*view;

	g_return_val_if_fail(XFDASHBOARD_IS_VIEWPAD(self), FALSE);
	g_return_val_if_fail(CLUTTER_IS_ACTOR(inActor), FALSE);
	g_return_val_if_fail(XFDASHBOARD_IS_VIEW(inUserData), FALSE);

	view=XFDASHBOARD_VIEW(inUserData);

	/* Determine if scrolling is needed and return result */
	return(_xfdashboard_viewpad_view_needs_scrolling_for_child(self, view, inActor, NULL, NULL));
}

/* Ensure that a child of a view is visible by scrolling if needed */
static void _xfdashboard_viewpad_on_view_child_ensure_visible(XfdashboardViewpad *self,
														ClutterActor *inActor,
														gpointer inUserData)
{
	XfdashboardView				*view;
	gfloat						scrollX, scrollY;

	g_return_if_fail(XFDASHBOARD_IS_VIEWPAD(self));
	g_return_if_fail(CLUTTER_IS_ACTOR(inActor));
	g_return_if_fail(XFDASHBOARD_IS_VIEW(inUserData));

	view=XFDASHBOARD_VIEW(inUserData);

	/* Check if scrolling is needed to scroll in direction and amount determined */
	if(_xfdashboard_viewpad_view_needs_scrolling_for_child(self, view, inActor, &scrollX, &scrollY))
	{
		_xfdashboard_viewpad_on_view_scroll_to(self, scrollX, scrollY, view);
	}
}

/* Create view of given type and add to this actor */
static void _xfdashboard_viewpad_add_view(XfdashboardViewpad *self, const gchar *inID)
{
	XfdashboardViewpadPrivate	*priv;
	GObject						*view;

	g_return_if_fail(XFDASHBOARD_IS_VIEWPAD(self));
	g_return_if_fail(inID && *inID);

	priv=self->priv;

	/* Create instance and check if it is a view */
	XFDASHBOARD_DEBUG(self, ACTOR,
						"Creating view %s for viewpad",
						inID);

	view=xfdashboard_view_manager_create_view(priv->viewManager, inID);
	if(view==NULL)
	{
		g_critical(_("Failed to create view %s for viewpad"), inID);
		return;
	}

	if(XFDASHBOARD_IS_VIEW(view)!=TRUE)
	{
		g_critical(_("View %s of type %s is not a %s and cannot be added to %s"),
					inID,
					G_OBJECT_TYPE_NAME(view),
					g_type_name(XFDASHBOARD_TYPE_VIEW),
					G_OBJECT_TYPE_NAME(self));
		return;
	}

	/* Add new view instance to this actor but hidden */
	clutter_actor_hide(CLUTTER_ACTOR(view));
	clutter_actor_add_child(CLUTTER_ACTOR(self), CLUTTER_ACTOR(view));
	g_signal_connect_swapped(view, "allocation-changed", G_CALLBACK(_xfdashboard_viewpad_on_allocation_changed), self);
	g_signal_connect_swapped(view, "scroll-to", G_CALLBACK(_xfdashboard_viewpad_on_view_scroll_to), self);
	g_signal_connect_swapped(view, "child-needs-scroll", G_CALLBACK(_xfdashboard_viewpad_on_view_child_needs_scroll), self);
	g_signal_connect_swapped(view, "child-ensure-visible", G_CALLBACK(_xfdashboard_viewpad_on_view_child_ensure_visible), self);
	g_signal_connect_swapped(view, "disabled", G_CALLBACK(_xfdashboard_viewpad_on_view_disabled), self);
	g_signal_connect_swapped(view, "enabled", G_CALLBACK(_xfdashboard_viewpad_on_view_enabled), self);
	g_signal_emit(self, XfdashboardViewpadSignals[SIGNAL_VIEW_ADDED], 0, view);

	/* Set active view if none active (usually it is the first view created) */
	if(priv->activeView==NULL &&
		xfdashboard_view_get_enabled(XFDASHBOARD_VIEW(view)))
	{
		_xfdashboard_viewpad_activate_view(self, XFDASHBOARD_VIEW(view));
	}

	/* If newly added view is focusable register it to focus manager */
	if(priv->doRegisterFocusableViews &&
		XFDASHBOARD_IS_FOCUSABLE(view))
	{
		XfdashboardFocusManager	*focusManager;

		focusManager=xfdashboard_focus_manager_get_default();
		xfdashboard_focus_manager_register_after(focusManager,
													XFDASHBOARD_FOCUSABLE(view),
													XFDASHBOARD_FOCUSABLE(self));
		g_object_unref(focusManager);
	}
}

/* Called when a new view type was registered */
static void _xfdashboard_viewpad_on_view_registered(XfdashboardViewpad *self,
													const gchar *inID,
													gpointer inUserData)
{
	g_return_if_fail(XFDASHBOARD_IS_VIEWPAD(self));
	g_return_if_fail(inID && *inID);

	_xfdashboard_viewpad_add_view(self, inID);
}

/* Called when a view type was unregistered */
static void _xfdashboard_viewpad_on_view_unregistered(XfdashboardViewpad *self,
														const gchar *inID,
														gpointer inUserData)
{
	XfdashboardViewpadPrivate	*priv;
	ClutterActorIter			iter;
	ClutterActor				*child;
	ClutterActor				*firstActivatableView;

	g_return_if_fail(XFDASHBOARD_IS_VIEWPAD(self));
	g_return_if_fail(inID && *inID);

	priv=self->priv;
	firstActivatableView=NULL;

	/* Iterate through create views and lookup view of given type */
	clutter_actor_iter_init(&iter, CLUTTER_ACTOR(self));
	while(clutter_actor_iter_next(&iter, &child))
	{
		/* Check if child is a view otherwise continue iterating */
		if(XFDASHBOARD_IS_VIEW(child)!=TRUE) continue;

		/* If this child is not the one being unregistered and is not
		 * disabled then it will get the first activatable view after
		 * we destroyed all views found.
		 */
		if(!xfdashboard_view_has_id(XFDASHBOARD_VIEW(child), inID))
		{
			if(xfdashboard_view_get_enabled(XFDASHBOARD_VIEW(child))) firstActivatableView=child;
		}
			else
			{
				if(G_OBJECT(child)==G_OBJECT(priv->activeView)) _xfdashboard_viewpad_activate_view(self, NULL);
				g_signal_emit(self, XfdashboardViewpadSignals[SIGNAL_VIEW_REMOVED], 0, child);
				clutter_actor_destroy(child);
			}
	}

	/* Now activate the first activatable view we found during iteration */
	if(firstActivatableView) _xfdashboard_viewpad_activate_view(self, XFDASHBOARD_VIEW(firstActivatableView));
}

/* Scroll event happened at this actor because it was not handled
 * with any child actor
 */
static gboolean _xfdashboard_viewpad_on_scroll_event(ClutterActor *inActor,
														ClutterEvent *inEvent,
														gpointer inUserData)
{
	XfdashboardViewpad				*self;
	XfdashboardViewpadPrivate		*priv;
	gboolean						result;

	g_return_val_if_fail(XFDASHBOARD_IS_VIEWPAD(inActor), FALSE);
	g_return_val_if_fail(inEvent, FALSE);

	self=XFDASHBOARD_VIEWPAD(inActor);
	priv=self->priv;
	result=CLUTTER_EVENT_PROPAGATE;

	/* If vertical scroll bar is visible emit scroll event there */
	if(priv->vScrollbarVisible)
	{
		result=clutter_actor_event(priv->vScrollbar, inEvent, FALSE);
	}
		/* Otherwise try horizontal scroll bar if visible */
		else if(priv->hScrollbarVisible)
		{
			result=clutter_actor_event(priv->hScrollbar, inEvent, FALSE);
		}

	return(result);
}

/* IMPLEMENTATION: ClutterActor */

/* Show this actor and the current active view */
static void _xfdashboard_viewpad_show(ClutterActor *self)
{
	XfdashboardViewpadPrivate	*priv=XFDASHBOARD_VIEWPAD(self)->priv;
	ClutterActorClass			*actorClass=CLUTTER_ACTOR_CLASS(xfdashboard_viewpad_parent_class);

	/* Only show active view again */
	if(priv->activeView) clutter_actor_show(CLUTTER_ACTOR(priv->activeView));

	/* Call parent's class show method */
	if(actorClass->show) actorClass->show(self);
}

/* Get preferred width/height */
static void _xfdashboard_viewpad_get_preferred_height(ClutterActor *self,
														gfloat inForWidth,
														gfloat *outMinHeight,
														gfloat *outNaturalHeight)
{
	gfloat							minHeight, naturalHeight;

	/* Do not set any minimum or natural sizes. The parent actor is responsible
	 * to set this actor's sizes as viewport.
	 */
	minHeight=naturalHeight=0.0f;

	/* Store sizes computed */
	if(outMinHeight) *outMinHeight=minHeight;
	if(outNaturalHeight) *outNaturalHeight=naturalHeight;
}

static void _xfdashboard_viewpad_get_preferred_width(ClutterActor *self,
														gfloat inForHeight,
														gfloat *outMinWidth,
														gfloat *outNaturalWidth)
{
	gfloat							minWidth, naturalWidth;

	/* Do not set any minimum or natural sizes. The parent actor is responsible
	 * to set this actor's sizes as viewport.
	 */
	minWidth=naturalWidth=0.0f;

	/* Store sizes computed */
	if(outMinWidth) *outMinWidth=minWidth;
	if(outNaturalWidth) *outNaturalWidth=naturalWidth;
}

/* Allocate position and size of actor and its children */
static void _xfdashboard_viewpad_allocate(ClutterActor *self,
											const ClutterActorBox *inBox,
											ClutterAllocationFlags inFlags)
{
	XfdashboardViewpadPrivate	*priv=XFDASHBOARD_VIEWPAD(self)->priv;
	ClutterActorClass			*actorClass=CLUTTER_ACTOR_CLASS(xfdashboard_viewpad_parent_class);
	gfloat						viewWidth, viewHeight;
	gfloat						vScrollbarWidth, vScrollbarHeight;
	gfloat						hScrollbarWidth, hScrollbarHeight;
	gboolean					hScrollbarVisible, vScrollbarVisible;
	ClutterActorBox				*box;
	gfloat						x, y, w, h;

	/* Chain up to store the allocation of the actor */
	if(actorClass->allocate) actorClass->allocate(self, inBox, inFlags);

	/* Initialize largest possible allocation for view and determine
	 * real size of view to show. The real size is used to determine
	 * scroll bar visibility if policy is automatic */
	viewWidth=clutter_actor_box_get_width(inBox);
	viewHeight=clutter_actor_box_get_height(inBox);

	/* Determine visibility of scroll bars */
	hScrollbarVisible=FALSE;
	if(priv->hScrollbarPolicy==XFDASHBOARD_VISIBILITY_POLICY_ALWAYS ||
		(priv->hScrollbarPolicy==XFDASHBOARD_VISIBILITY_POLICY_AUTOMATIC &&
			xfdashboard_scrollbar_get_range(XFDASHBOARD_SCROLLBAR(priv->hScrollbar))>viewWidth))
	{
		hScrollbarVisible=TRUE;
	}
	if(xfdashboard_view_get_view_fit_mode(XFDASHBOARD_VIEW(priv->activeView))==XFDASHBOARD_VIEW_FIT_MODE_HORIZONTAL ||
		xfdashboard_view_get_view_fit_mode(XFDASHBOARD_VIEW(priv->activeView))==XFDASHBOARD_VIEW_FIT_MODE_BOTH)
	{
		hScrollbarVisible=FALSE;
	}

	vScrollbarVisible=FALSE;
	if(priv->vScrollbarPolicy==XFDASHBOARD_VISIBILITY_POLICY_ALWAYS ||
		(priv->vScrollbarPolicy==XFDASHBOARD_VISIBILITY_POLICY_AUTOMATIC &&
			xfdashboard_scrollbar_get_range(XFDASHBOARD_SCROLLBAR(priv->vScrollbar))>viewHeight))
	{
		vScrollbarVisible=TRUE;
	}
	if(xfdashboard_view_get_view_fit_mode(XFDASHBOARD_VIEW(priv->activeView))==XFDASHBOARD_VIEW_FIT_MODE_VERTICAL ||
		xfdashboard_view_get_view_fit_mode(XFDASHBOARD_VIEW(priv->activeView))==XFDASHBOARD_VIEW_FIT_MODE_BOTH)
	{
		vScrollbarVisible=FALSE;
	}

	/* Set allocation for visible scroll bars */
	vScrollbarWidth=0.0f;
	vScrollbarHeight=viewHeight;
	clutter_actor_get_preferred_width(priv->vScrollbar, -1, NULL, &vScrollbarWidth);

	hScrollbarWidth=viewWidth;
	hScrollbarHeight=0.0f;
	clutter_actor_get_preferred_height(priv->hScrollbar, -1, NULL, &hScrollbarHeight);

	if(hScrollbarVisible && vScrollbarVisible)
	{
		vScrollbarHeight-=hScrollbarHeight;
		hScrollbarWidth-=vScrollbarWidth;
	}

	if(vScrollbarVisible==FALSE) box=clutter_actor_box_new(0, 0, 0, 0);
		else box=clutter_actor_box_new(viewWidth-vScrollbarWidth, 0, viewWidth, vScrollbarHeight);
	clutter_actor_allocate(priv->vScrollbar, box, inFlags);
	clutter_actor_box_free(box);

	if(hScrollbarVisible==FALSE) box=clutter_actor_box_new(0, 0, 0, 0);
		else box=clutter_actor_box_new(0, viewHeight-hScrollbarHeight, hScrollbarWidth, viewHeight);
	clutter_actor_allocate(priv->hScrollbar, box, inFlags);
	clutter_actor_box_free(box);

	/* Reduce allocation for view by any visible scroll bar
	 * and set allocation and clipping of view
	 */
	if(priv->activeView)
	{
		/* Set allocation */
		if(vScrollbarVisible) viewWidth-=vScrollbarWidth;
		if(hScrollbarVisible) viewHeight-=hScrollbarHeight;

		x=y=0.0f;
		if(clutter_actor_has_clip(CLUTTER_ACTOR(priv->activeView)))
		{
			clutter_actor_get_clip(CLUTTER_ACTOR(priv->activeView), &x, &y, NULL, NULL);
		}

		switch(xfdashboard_view_get_view_fit_mode(XFDASHBOARD_VIEW(priv->activeView)))
		{
			case XFDASHBOARD_VIEW_FIT_MODE_BOTH:
				w=viewWidth;
				h=viewHeight;
				break;

			case XFDASHBOARD_VIEW_FIT_MODE_HORIZONTAL:
				w=viewWidth;
				clutter_actor_get_preferred_height(CLUTTER_ACTOR(priv->activeView), w, NULL, &h);
				break;

			case XFDASHBOARD_VIEW_FIT_MODE_VERTICAL:
				h=viewHeight;
				clutter_actor_get_preferred_width(CLUTTER_ACTOR(priv->activeView), h, NULL, &w);
				break;

			default:
				clutter_actor_get_preferred_size(CLUTTER_ACTOR(priv->activeView), NULL, NULL, &w, &h);
				break;
		}

		box=clutter_actor_box_new(0, 0, w, h);
		clutter_actor_allocate(CLUTTER_ACTOR(priv->activeView), box, inFlags);
		clutter_actor_box_free(box);

		clutter_actor_set_clip(CLUTTER_ACTOR(priv->activeView), x, y, viewWidth, viewHeight);
	}

	/* Only set value if it changes */
	if(priv->hScrollbarVisible!=hScrollbarVisible)
	{
		/* Set new value */
		priv->hScrollbarVisible=hScrollbarVisible;

		/* Notify about property change */
		g_object_notify_by_pspec(G_OBJECT(self), XfdashboardViewpadProperties[PROP_HSCROLLBAR_VISIBLE]);
	}

	if(priv->vScrollbarVisible!=vScrollbarVisible)
	{
		/* Set new value */
		priv->vScrollbarVisible=vScrollbarVisible;

		/* Notify about property change */
		g_object_notify_by_pspec(G_OBJECT(self), XfdashboardViewpadProperties[PROP_VSCROLLBAR_VISIBLE]);
	}

	/* Remember this allocation as last one set */
	if(priv->lastAllocation)
	{
		clutter_actor_box_free(priv->lastAllocation);
		priv->lastAllocation=NULL;
	}

	priv->lastAllocation=clutter_actor_box_copy(inBox);
}

/* IMPLEMENTATION: Interface XfdashboardFocusable */

/* Determine if actor can get the focus */
static gboolean _xfdashboard_viewpad_focusable_can_focus(XfdashboardFocusable *inFocusable)
{
	XfdashboardViewpad			*self;
	XfdashboardViewpadPrivate	*priv;

	g_return_val_if_fail(XFDASHBOARD_IS_FOCUSABLE(inFocusable), FALSE);
	g_return_val_if_fail(XFDASHBOARD_IS_VIEWPAD(inFocusable), FALSE);

	self=XFDASHBOARD_VIEWPAD(inFocusable);
	priv=self->priv;

	/* If it is the first time this viewpad is checked if it is focusable register
	 * all registered and focusable views at focus manager and set flag that newly
	 * added views should be registered to focus manager also because now the insert
	 * position at focus manager for this viewpad is known and we can keep the focus
	 * order for the views.
	 * We determine if this is the first time this viewpad is check for its focusability
	 * by checking if the "do-register-view" flag is still not set.
	 */
	if(!priv->doRegisterFocusableViews)
	{
		XfdashboardFocusManager		*focusManager;
		ClutterActorIter			iter;
		ClutterActor				*child;

		/* Get focus manager */
		focusManager=xfdashboard_focus_manager_get_default();

		/* Iterate through children of this viewpad and register each focusable view found */
		clutter_actor_iter_init(&iter, CLUTTER_ACTOR(self));
		while(clutter_actor_iter_next(&iter, &child))
		{
			/* Check if child is a view otherwise continue iterating */
			if(XFDASHBOARD_IS_VIEW(child)!=TRUE) continue;

			/* If view is focusable register it to focus manager */
			if(XFDASHBOARD_IS_FOCUSABLE(child))
			{
				xfdashboard_focus_manager_register_after(focusManager,
															XFDASHBOARD_FOCUSABLE(child),
															XFDASHBOARD_FOCUSABLE(self));
			}
		}

		/* Release allocated resources */
		if(focusManager) g_object_unref(focusManager);

		/* Set flag that from now on each newly added view should be added to focus manager */
		priv->doRegisterFocusableViews=TRUE;
	}

	/* This viewpad cannot be focused. It is only registered at focus manager as
	 * a placeholder to register focusable views just after this viewpad to keep
	 * order of focusable actors.
	 */
	return(FALSE);
}

/* Interface initialization
 * Set up default functions
 */
void _xfdashboard_viewpad_focusable_iface_init(XfdashboardFocusableInterface *iface)
{
	iface->can_focus=_xfdashboard_viewpad_focusable_can_focus;
}

/* IMPLEMENTATION: GObject */

/* Dispose this object */
static void _xfdashboard_viewpad_dispose(GObject *inObject)
{
	XfdashboardViewpad			*self=XFDASHBOARD_VIEWPAD(inObject);
	XfdashboardViewpadPrivate	*priv=self->priv;

	/* Prevent further registers of views */
	priv->doRegisterFocusableViews=FALSE;

	/* Deactivate current view */
	if(priv->activeView) _xfdashboard_viewpad_activate_view(self, NULL);

	/* Disconnect signals handlers */
	if(priv->viewManager)
	{
		g_signal_handlers_disconnect_by_data(priv->viewManager, self);
		g_object_unref(priv->viewManager);
		priv->viewManager=NULL;
	}

	/* Release allocated resources */
	if(priv->lastAllocation)
	{
		clutter_actor_box_free(priv->lastAllocation);
		priv->lastAllocation=NULL;
	}

	/* Call parent's class dispose method */
	G_OBJECT_CLASS(xfdashboard_viewpad_parent_class)->dispose(inObject);
}

/* Set/get properties */
static void _xfdashboard_viewpad_set_property(GObject *inObject,
												guint inPropID,
												const GValue *inValue,
												GParamSpec *inSpec)
{
	XfdashboardViewpad		*self=XFDASHBOARD_VIEWPAD(inObject);
	
	switch(inPropID)
	{
		case PROP_SPACING:
			xfdashboard_viewpad_set_spacing(self, g_value_get_float(inValue));
			break;

		case PROP_HSCROLLBAR_POLICY:
			xfdashboard_viewpad_set_horizontal_scrollbar_policy(self, (XfdashboardVisibilityPolicy)g_value_get_enum(inValue));
			break;

		case PROP_VSCROLLBAR_POLICY:
			xfdashboard_viewpad_set_vertical_scrollbar_policy(self, (XfdashboardVisibilityPolicy)g_value_get_enum(inValue));
			break;

		default:
			G_OBJECT_WARN_INVALID_PROPERTY_ID(inObject, inPropID, inSpec);
			break;
	}
}

static void _xfdashboard_viewpad_get_property(GObject *inObject,
												guint inPropID,
												GValue *outValue,
												GParamSpec *inSpec)
{
	XfdashboardViewpad		*self=XFDASHBOARD_VIEWPAD(inObject);

	switch(inPropID)
	{
		case PROP_SPACING:
			g_value_set_float(outValue, self->priv->spacing);
			break;

		case PROP_ACTIVE_VIEW:
			g_value_set_object(outValue, self->priv->activeView);
			break;

		case PROP_HSCROLLBAR_POLICY:
			g_value_set_enum(outValue, self->priv->hScrollbarPolicy);
			break;

		case PROP_HSCROLLBAR_VISIBLE:
			g_value_set_boolean(outValue, self->priv->hScrollbarVisible);
			break;

		case PROP_VSCROLLBAR_POLICY:
			g_value_set_enum(outValue, self->priv->vScrollbarPolicy);
			break;

		case PROP_VSCROLLBAR_VISIBLE:
			g_value_set_boolean(outValue, self->priv->vScrollbarVisible);
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
static void xfdashboard_viewpad_class_init(XfdashboardViewpadClass *klass)
{
	XfdashboardActorClass	*actorClass=XFDASHBOARD_ACTOR_CLASS(klass);
	ClutterActorClass		*clutterActorClass=CLUTTER_ACTOR_CLASS(klass);
	GObjectClass			*gobjectClass=G_OBJECT_CLASS(klass);

	/* Override functions */
	gobjectClass->set_property=_xfdashboard_viewpad_set_property;
	gobjectClass->get_property=_xfdashboard_viewpad_get_property;
	gobjectClass->dispose=_xfdashboard_viewpad_dispose;

	clutterActorClass->show=_xfdashboard_viewpad_show;
	clutterActorClass->get_preferred_width=_xfdashboard_viewpad_get_preferred_width;
	clutterActorClass->get_preferred_height=_xfdashboard_viewpad_get_preferred_height;
	clutterActorClass->allocate=_xfdashboard_viewpad_allocate;

	/* Define properties */
	XfdashboardViewpadProperties[PROP_SPACING]=
		g_param_spec_float("spacing",
							_("Spacing"),
							_("The spacing between views and scrollbars"),
							0.0f, G_MAXFLOAT,
							0.0f,
							G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS);

	XfdashboardViewpadProperties[PROP_ACTIVE_VIEW]=
		g_param_spec_object("active-view",
								_("Active view"),
								_("The current active view in viewpad"),
								XFDASHBOARD_TYPE_VIEW,
								G_PARAM_READABLE | G_PARAM_STATIC_STRINGS);

	XfdashboardViewpadProperties[PROP_HSCROLLBAR_VISIBLE]=
		g_param_spec_boolean("horizontal-scrollbar-visible",
								_("Horizontal scrollbar visibility"),
								_("This flag indicates if horizontal scrollbar is visible"),
								FALSE,
								G_PARAM_READABLE | G_PARAM_STATIC_STRINGS);

	XfdashboardViewpadProperties[PROP_HSCROLLBAR_POLICY]=
		g_param_spec_enum("horizontal-scrollbar-policy",
							_("Horizontal scrollbar policy"),
							_("The policy for horizontal scrollbar controlling when it is displayed"),
							XFDASHBOARD_TYPE_VISIBILITY_POLICY,
							XFDASHBOARD_VISIBILITY_POLICY_AUTOMATIC,
							G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS);

	XfdashboardViewpadProperties[PROP_VSCROLLBAR_VISIBLE]=
		g_param_spec_boolean("vertical-scrollbar-visible",
								_("Vertical scrollbar visibility"),
								_("This flag indicates if vertical scrollbar is visible"),
								FALSE,
								G_PARAM_READABLE | G_PARAM_STATIC_STRINGS);

	XfdashboardViewpadProperties[PROP_VSCROLLBAR_POLICY]=
		g_param_spec_enum("vertical-scrollbar-policy",
							_("Vertical scrollbar policy"),
							_("The policy for vertical scrollbar controlling when it is displayed"),
							XFDASHBOARD_TYPE_VISIBILITY_POLICY,
							XFDASHBOARD_VISIBILITY_POLICY_AUTOMATIC,
							G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS);

	g_object_class_install_properties(gobjectClass, PROP_LAST, XfdashboardViewpadProperties);

	/* Define stylable properties */
	xfdashboard_actor_install_stylable_property(actorClass, XfdashboardViewpadProperties[PROP_SPACING]);
	xfdashboard_actor_install_stylable_property(actorClass, XfdashboardViewpadProperties[PROP_HSCROLLBAR_POLICY]);
	xfdashboard_actor_install_stylable_property(actorClass, XfdashboardViewpadProperties[PROP_VSCROLLBAR_POLICY]);

	/* Define signals */
	XfdashboardViewpadSignals[SIGNAL_VIEW_ADDED]=
		g_signal_new("view-added",
						G_TYPE_FROM_CLASS(klass),
						G_SIGNAL_RUN_LAST,
						G_STRUCT_OFFSET(XfdashboardViewpadClass, view_added),
						NULL,
						NULL,
						g_cclosure_marshal_VOID__OBJECT,
						G_TYPE_NONE,
						1,
						XFDASHBOARD_TYPE_VIEW);

	XfdashboardViewpadSignals[SIGNAL_VIEW_REMOVED]=
		g_signal_new("view-removed",
						G_TYPE_FROM_CLASS(klass),
						G_SIGNAL_RUN_LAST,
						G_STRUCT_OFFSET(XfdashboardViewpadClass, view_removed),
						NULL,
						NULL,
						g_cclosure_marshal_VOID__OBJECT,
						G_TYPE_NONE,
						1,
						XFDASHBOARD_TYPE_VIEW);

	XfdashboardViewpadSignals[SIGNAL_VIEW_ACTIVATING]=
		g_signal_new("view-activating",
						G_TYPE_FROM_CLASS(klass),
						G_SIGNAL_RUN_LAST,
						G_STRUCT_OFFSET(XfdashboardViewpadClass, view_activating),
						NULL,
						NULL,
						g_cclosure_marshal_VOID__OBJECT,
						G_TYPE_NONE,
						1,
						XFDASHBOARD_TYPE_VIEW);

	XfdashboardViewpadSignals[SIGNAL_VIEW_ACTIVATED]=
		g_signal_new("view-activated",
						G_TYPE_FROM_CLASS(klass),
						G_SIGNAL_RUN_LAST,
						G_STRUCT_OFFSET(XfdashboardViewpadClass, view_activated),
						NULL,
						NULL,
						g_cclosure_marshal_VOID__OBJECT,
						G_TYPE_NONE,
						1,
						XFDASHBOARD_TYPE_VIEW);

	XfdashboardViewpadSignals[SIGNAL_VIEW_DEACTIVATING]=
		g_signal_new("view-deactivating",
						G_TYPE_FROM_CLASS(klass),
						G_SIGNAL_RUN_LAST,
						G_STRUCT_OFFSET(XfdashboardViewpadClass, view_deactivating),
						NULL,
						NULL,
						g_cclosure_marshal_VOID__OBJECT,
						G_TYPE_NONE,
						1,
						XFDASHBOARD_TYPE_VIEW);

	XfdashboardViewpadSignals[SIGNAL_VIEW_DEACTIVATED]=
		g_signal_new("view-deactivated",
						G_TYPE_FROM_CLASS(klass),
						G_SIGNAL_RUN_LAST,
						G_STRUCT_OFFSET(XfdashboardViewpadClass, view_deactivated),
						NULL,
						NULL,
						g_cclosure_marshal_VOID__OBJECT,
						G_TYPE_NONE,
						1,
						XFDASHBOARD_TYPE_VIEW);
}

/* Object initialization
 * Create private structure and set up default values
 */
static void xfdashboard_viewpad_init(XfdashboardViewpad *self)
{
	XfdashboardViewpadPrivate	*priv;
	GList						*views, *viewEntry;

	priv=self->priv=xfdashboard_viewpad_get_instance_private(self);

	/* Set up default values */
	priv->viewManager=xfdashboard_view_manager_get_default();
	priv->activeView=NULL;
	priv->spacing=0.0f;
	priv->hScrollbarVisible=FALSE;
	priv->hScrollbarPolicy=XFDASHBOARD_VISIBILITY_POLICY_AUTOMATIC;
	priv->vScrollbarVisible=FALSE;
	priv->vScrollbarPolicy=XFDASHBOARD_VISIBILITY_POLICY_AUTOMATIC;
	priv->scrollbarUpdateID=0;
	priv->doRegisterFocusableViews=FALSE;
	priv->lastAllocation=NULL;

	/* Set up this actor */
	clutter_actor_set_reactive(CLUTTER_ACTOR(self), TRUE);
	g_signal_connect(self, "scroll-event", G_CALLBACK(_xfdashboard_viewpad_on_scroll_event), NULL);

	/* Set up child actors */
	priv->hScrollbar=xfdashboard_scrollbar_new(CLUTTER_ORIENTATION_HORIZONTAL);
	clutter_actor_add_child(CLUTTER_ACTOR(self), priv->hScrollbar);
	g_signal_connect_swapped(priv->hScrollbar, "value-changed", G_CALLBACK(_xfdashboard_viewpad_on_scrollbar_value_changed), self);

	priv->vScrollbar=xfdashboard_scrollbar_new(CLUTTER_ORIENTATION_VERTICAL);
	clutter_actor_add_child(CLUTTER_ACTOR(self), priv->vScrollbar);
	g_signal_connect_swapped(priv->vScrollbar, "value-changed", G_CALLBACK(_xfdashboard_viewpad_on_scrollbar_value_changed), self);

	/* Create instance of each registered view type and add it to this actor
	 * and connect signals
	 */
	views=viewEntry=xfdashboard_view_manager_get_registered(priv->viewManager);
	for(; viewEntry; viewEntry=g_list_next(viewEntry))
	{
		const gchar				*viewID;

		viewID=(const gchar*)viewEntry->data;
		_xfdashboard_viewpad_add_view(self, viewID);
	}
	g_list_free_full(views, g_free);

	g_signal_connect_swapped(priv->viewManager, "registered", G_CALLBACK(_xfdashboard_viewpad_on_view_registered), self);
	g_signal_connect_swapped(priv->viewManager, "unregistered", G_CALLBACK(_xfdashboard_viewpad_on_view_unregistered), self);
}

/* IMPLEMENTATION: Public API */

/* Create new instance */
ClutterActor* xfdashboard_viewpad_new(void)
{
	return(CLUTTER_ACTOR(g_object_new(XFDASHBOARD_TYPE_VIEWPAD, NULL)));
}

/* Get/set spacing */
gfloat xfdashboard_viewpad_get_spacing(XfdashboardViewpad *self)
{
	g_return_val_if_fail(XFDASHBOARD_IS_VIEWPAD(self), 0.0f);

	return(self->priv->spacing);
}

void xfdashboard_viewpad_set_spacing(XfdashboardViewpad *self, gfloat inSpacing)
{
	XfdashboardViewpadPrivate	*priv;

	g_return_if_fail(XFDASHBOARD_IS_VIEWPAD(self));
	g_return_if_fail(inSpacing>=0.0f);

	priv=self->priv;

	/* Only set value if it changes */
	if(inSpacing==priv->spacing) return;

	/* Set new value */
	priv->spacing=inSpacing;
	if(priv->layout)
	{
		clutter_grid_layout_set_column_spacing(CLUTTER_GRID_LAYOUT(priv->layout), priv->spacing);
		clutter_grid_layout_set_row_spacing(CLUTTER_GRID_LAYOUT(priv->layout), priv->spacing);
	}
	clutter_actor_queue_relayout(CLUTTER_ACTOR(self));

	/* Notify about property change */
	g_object_notify_by_pspec(G_OBJECT(self), XfdashboardViewpadProperties[PROP_SPACING]);
}

/* Get list of views */
GList* xfdashboard_viewpad_get_views(XfdashboardViewpad *self)
{
	ClutterActorIter			iter;
	ClutterActor				*child;
	GList						*list;

	g_return_val_if_fail(XFDASHBOARD_IS_VIEWPAD(self), NULL);

	list=NULL;

	/* Iterate through children and create list of views */
	clutter_actor_iter_init(&iter, CLUTTER_ACTOR(self));
	while(clutter_actor_iter_next(&iter, &child))
	{
		/* Check if child is a view and add to list */
		if(XFDASHBOARD_IS_VIEW(child)==TRUE) list=g_list_prepend(list, child);
	}
	list=g_list_reverse(list);

	return(list);
}

/* Determine if viewpad contains requested view */
gboolean xfdashboard_viewpad_has_view(XfdashboardViewpad *self, XfdashboardView *inView)
{
	ClutterActorIter			iter;
	ClutterActor				*child;

	g_return_val_if_fail(XFDASHBOARD_IS_VIEWPAD(self), FALSE);
	g_return_val_if_fail(XFDASHBOARD_IS_VIEW(inView), FALSE);

	/* Iterate through children and check if child is requested view */
	clutter_actor_iter_init(&iter, CLUTTER_ACTOR(self));
	while(clutter_actor_iter_next(&iter, &child))
	{
		if(XFDASHBOARD_IS_VIEW(child) &&
			XFDASHBOARD_VIEW(child)==inView)
		{
			return(TRUE);
		}
	}

	/* If we get here the requested view could not be found
	 * so return FALSE.
	 */
	return(FALSE);
}

/* Find view by type */
XfdashboardView* xfdashboard_viewpad_find_view_by_type(XfdashboardViewpad *self, GType inType)
{
	ClutterActorIter			iter;
	ClutterActor				*child;
	XfdashboardView				*view;

	g_return_val_if_fail(XFDASHBOARD_IS_VIEWPAD(self), NULL);

	view=NULL;

	/* Iterate through children and create list of views */
	clutter_actor_iter_init(&iter, CLUTTER_ACTOR(self));
	while(!view && clutter_actor_iter_next(&iter, &child))
	{
		/* Check if child is a view and of type looking for */
		if(XFDASHBOARD_IS_VIEW(child)==TRUE &&
			G_OBJECT_TYPE(child)==inType)
		{
			view=XFDASHBOARD_VIEW(child);
		}
	}

	/* Return view found which may be NULL if no view of requested type was found */
	return(view);
}

/* Find view by ID */
XfdashboardView* xfdashboard_viewpad_find_view_by_id(XfdashboardViewpad *self, const gchar *inID)
{
	ClutterActorIter			iter;
	ClutterActor				*child;
	XfdashboardView				*view;

	g_return_val_if_fail(XFDASHBOARD_IS_VIEWPAD(self), NULL);
	g_return_val_if_fail(inID && *inID, NULL);

	view=NULL;

	/* Iterate through children and lookup view matching requested name */
	clutter_actor_iter_init(&iter, CLUTTER_ACTOR(self));
	while(!view && clutter_actor_iter_next(&iter, &child))
	{
		/* Check if child is a view and its internal name matches requested name */
		if(XFDASHBOARD_IS_VIEW(child)==TRUE &&
			xfdashboard_view_has_id(XFDASHBOARD_VIEW(child), inID))
		{
			view=XFDASHBOARD_VIEW(child);
		}
	}

	/* Return view found which may be NULL if no view of requested type was found */
	return(view);
}

/* Get/set active view */
XfdashboardView* xfdashboard_viewpad_get_active_view(XfdashboardViewpad *self)
{
	g_return_val_if_fail(XFDASHBOARD_IS_VIEWPAD(self), NULL);

	return(self->priv->activeView);
}

void xfdashboard_viewpad_set_active_view(XfdashboardViewpad *self, XfdashboardView *inView)
{
	XfdashboardViewpadPrivate	*priv;

	g_return_if_fail(XFDASHBOARD_IS_VIEWPAD(self));
	g_return_if_fail(XFDASHBOARD_IS_VIEW(inView));

	priv=self->priv;

	/* Only activate view if it changes */
	if(priv->activeView!=inView) _xfdashboard_viewpad_activate_view(self, inView);
}

/* Get/set scroll bar visibility */
gboolean xfdashboard_viewpad_get_horizontal_scrollbar_visible(XfdashboardViewpad *self)
{
	g_return_val_if_fail(XFDASHBOARD_IS_VIEWPAD(self), FALSE);

	return(self->priv->hScrollbarVisible);
}

gboolean xfdashboard_viewpad_get_vertical_scrollbar_visible(XfdashboardViewpad *self)
{
	g_return_val_if_fail(XFDASHBOARD_IS_VIEWPAD(self), FALSE);

	return(self->priv->vScrollbarVisible);
}

/* Get/set scroll bar policy */
XfdashboardVisibilityPolicy xfdashboard_viewpad_get_horizontal_scrollbar_policy(XfdashboardViewpad *self)
{
	g_return_val_if_fail(XFDASHBOARD_IS_VIEWPAD(self), XFDASHBOARD_VISIBILITY_POLICY_AUTOMATIC);

	return(self->priv->hScrollbarPolicy);
}

void xfdashboard_viewpad_set_horizontal_scrollbar_policy(XfdashboardViewpad *self, XfdashboardVisibilityPolicy inPolicy)
{
	XfdashboardViewpadPrivate	*priv;

	g_return_if_fail(XFDASHBOARD_IS_VIEWPAD(self));

	priv=self->priv;

	/* Only set new value if it differs from current value */
	if(priv->hScrollbarPolicy==inPolicy) return;

	/* Set new value */
	priv->hScrollbarPolicy=inPolicy;
	clutter_actor_queue_relayout(CLUTTER_ACTOR(self));

	/* Notify about property change */
	g_object_notify_by_pspec(G_OBJECT(self), XfdashboardViewpadProperties[PROP_HSCROLLBAR_POLICY]);
}

XfdashboardVisibilityPolicy xfdashboard_viewpad_get_vertical_scrollbar_policy(XfdashboardViewpad *self)
{
	g_return_val_if_fail(XFDASHBOARD_IS_VIEWPAD(self), XFDASHBOARD_VISIBILITY_POLICY_AUTOMATIC);

	return(self->priv->vScrollbarPolicy);
}

void xfdashboard_viewpad_set_vertical_scrollbar_policy(XfdashboardViewpad *self, XfdashboardVisibilityPolicy inPolicy)
{
	XfdashboardViewpadPrivate	*priv;

	g_return_if_fail(XFDASHBOARD_IS_VIEWPAD(self));

	priv=self->priv;

	/* Only set new value if it differs from current value */
	if(priv->vScrollbarPolicy==inPolicy) return;

	/* Set new value */
	priv->vScrollbarPolicy=inPolicy;
	clutter_actor_queue_relayout(CLUTTER_ACTOR(self));

	/* Notify about property change */
	g_object_notify_by_pspec(G_OBJECT(self), XfdashboardViewpadProperties[PROP_VSCROLLBAR_POLICY]);
}
