/*
 * viewpad.c: A viewpad managing views
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

#include "viewpad.h"

#include <math.h>

/* Define this class in GObject system */
G_DEFINE_TYPE(XfdashboardViewpad,
				xfdashboard_viewpad,
				CLUTTER_TYPE_ACTOR)

/* Private structure - access only by public API if needed */
#define XFDASHBOARD_VIEWPAD_GET_PRIVATE(obj) \
	(G_TYPE_INSTANCE_GET_PRIVATE((obj), XFDASHBOARD_TYPE_VIEWPAD, XfdashboardViewpadPrivate))

struct _XfdashboardViewpadPrivate
{
	/* List of views and the active one */
	GList					*views;
	XfdashboardView			*activeView;

	/* Scroll bars */
	gfloat					thickness;

	gboolean				horizontalScrollbarVisible;
	ClutterActor			*horizontalScrollbar;
	GtkPolicyType			horizontalScrollbarPolicy;

	gboolean				verticalScrollbarVisible;
	ClutterActor			*verticalScrollbar;
	GtkPolicyType			verticalScrollbarPolicy;
};

/* Properties */
enum
{
	PROP_0,

	PROP_THICKNESS,

	PROP_ACTIVE_VIEW,

	PROP_VERTICAL_SCROLLBAR,
	PROP_VERTICAL_SCROLLBAR_POLICY,
	PROP_HORIZONTAL_SCROLLBAR,
	PROP_HORIZONTAL_SCROLLBAR_POLICY,

	PROP_LAST
};

static GParamSpec* XfdashboardViewpadProperties[PROP_LAST]={ 0, };

/* Signals */
enum
{
	VIEW_ADDED,
	VIEW_REMOVED,
	VIEW_ACTIVATED,
	VIEW_DEACTIVATED,

	SIGNAL_LAST
};

static guint XfdashboardViewpadSignals[SIGNAL_LAST]={ 0, };

/* IMPLEMENTATION: Private variables and methods */
#define DEFAULT_THICKNESS			8.0f
#define DEFAULT_SCROLLBAR_POLICY	GTK_POLICY_AUTOMATIC

/* Resetting scroll bars was requested */
static void _xfdashboard_viewpad_reset_scrollbars(ClutterActor *inActor, gpointer inUserData)
{
	g_return_if_fail(XFDASHBOARD_IS_VIEW(inActor));
	g_return_if_fail(XFDASHBOARD_IS_VIEWPAD(inUserData));

	/* Reset value of scroll bars to zero */
	XfdashboardViewpadPrivate	*priv=XFDASHBOARD_VIEWPAD(inUserData)->priv;

	xfdashboard_scrollbar_set_value(XFDASHBOARD_SCROLLBAR(priv->horizontalScrollbar), 0.0f);
	xfdashboard_scrollbar_set_value(XFDASHBOARD_SCROLLBAR(priv->verticalScrollbar), 0.0f);
}

/* Allocation of a view changed */
static void _xfdashboard_viewpad_allocation_changed(ClutterActor *inActor,
													ClutterActorBox *inBox,
													ClutterAllocationFlags inFlags,
													gpointer inUserData)
{
	g_return_if_fail(XFDASHBOARD_IS_VIEW(inActor));
	g_return_if_fail(XFDASHBOARD_IS_VIEWPAD(inUserData));

	XfdashboardViewpadPrivate	*priv=XFDASHBOARD_VIEWPAD(inUserData)->priv;
	gfloat						w, h;

	/* Set range of scroll bar to width and height of view
	 * which allocation just changed
	 */
	clutter_actor_get_preferred_size(inActor, NULL, NULL, &w, &h);

	xfdashboard_scrollbar_set_range(XFDASHBOARD_SCROLLBAR(priv->horizontalScrollbar), w);
	xfdashboard_scrollbar_set_range(XFDASHBOARD_SCROLLBAR(priv->verticalScrollbar), h);
}

/* Value of a scroll bar has changed */
static void _xfdashboard_viewpad_value_changed(ClutterActor *inActor, gfloat inNewValue, gpointer inUserData)
{
	g_return_if_fail(XFDASHBOARD_IS_SCROLLBAR(inActor));
	g_return_if_fail(XFDASHBOARD_IS_VIEWPAD(inUserData));

	clutter_actor_queue_relayout(CLUTTER_ACTOR(inUserData));
}

/* A scroll event occured in scroll bar (e.g. by mouse-wheel) */
static gboolean _xfdashboard_viewpad_scroll_event(ClutterActor *inActor,
													ClutterEvent *inEvent,
													gpointer inUserData)
{
	XfdashboardViewpadPrivate	*priv=XFDASHBOARD_VIEWPAD(inActor)->priv;
	gboolean					eventHandled=FALSE;

	/* Redirect event to scroll bar */
	switch(clutter_event_get_scroll_direction(inEvent))
	{
		case CLUTTER_SCROLL_UP:
		case CLUTTER_SCROLL_DOWN:
			if(priv->verticalScrollbar &&
				CLUTTER_ACTOR_IS_VISIBLE(priv->verticalScrollbar))
			{
				g_signal_emit_by_name(priv->verticalScrollbar,
										"scroll-event",
										inEvent,
										&eventHandled);
			}
			break;

		case CLUTTER_SCROLL_LEFT:
		case CLUTTER_SCROLL_RIGHT:
			if(priv->horizontalScrollbar &&
				CLUTTER_ACTOR_IS_VISIBLE(priv->horizontalScrollbar))
			{
				g_signal_emit_by_name(priv->horizontalScrollbar,
										"scroll-event",
										inEvent,
										&eventHandled);
			}
			break;
	}

	return(eventHandled);
}

/* IMPLEMENTATION: ClutterActor */

/* Paint actor */
static void xfdashboard_viewpad_paint(ClutterActor *inActor)
{
	XfdashboardViewpadPrivate	*priv=XFDASHBOARD_VIEWPAD(inActor)->priv;

	if(priv->activeView && CLUTTER_ACTOR_IS_VISIBLE(priv->activeView))
		clutter_actor_paint(CLUTTER_ACTOR(priv->activeView));

	if(priv->verticalScrollbar &&
		CLUTTER_ACTOR_IS_VISIBLE(priv->verticalScrollbar) &&
		priv->verticalScrollbarVisible)
		clutter_actor_paint(priv->verticalScrollbar);

	if(priv->horizontalScrollbar &&
		CLUTTER_ACTOR_IS_VISIBLE(priv->horizontalScrollbar) &&
		priv->horizontalScrollbarVisible)
		clutter_actor_paint(priv->horizontalScrollbar);
}

/* Pick all the child actors */
static void xfdashboard_viewpad_pick(ClutterActor *inActor, const ClutterColor *inPick)
{
	CLUTTER_ACTOR_CLASS(xfdashboard_viewpad_parent_class)->pick(inActor, inPick);

	xfdashboard_viewpad_paint(inActor);
}

/* Get preferred width/height */
static void xfdashboard_viewpad_get_preferred_width(ClutterActor *inActor,
													gfloat inForHeight,
													gfloat *outMinWidth,
													gfloat *outNaturalWidth)
{
	XfdashboardViewpadPrivate	*priv=XFDASHBOARD_VIEWPAD(inActor)->priv;
	gfloat						minWidth, naturalWidth;

	/* Initially actor needs no space at all */
	minWidth=naturalWidth=0.0f;
	if(priv->activeView)
	{
		/* Our natural width is the natural width of the active view */
		clutter_actor_get_preferred_width(CLUTTER_ACTOR(priv->activeView),
											inForHeight,
											NULL,
											&naturalWidth);

		/* Check if we need to add space for vertical scroll-bar */
		if(inForHeight>=0)
		{
			gfloat				viewNaturalHeight;
			gfloat				scrollbarMinWidth, scrollbarNaturalWidth;

			clutter_actor_get_preferred_height(CLUTTER_ACTOR(priv->activeView),
												-1.0f,
												NULL,
												&viewNaturalHeight);
			if(priv->horizontalScrollbarPolicy==GTK_POLICY_ALWAYS ||
				(priv->horizontalScrollbarPolicy==GTK_POLICY_AUTOMATIC &&
					viewNaturalHeight>inForHeight))
			{
				clutter_actor_get_preferred_width(priv->verticalScrollbar,
													inForHeight,
													&scrollbarMinWidth,
													&scrollbarNaturalWidth);
				minWidth+=scrollbarMinWidth;
				naturalWidth+=scrollbarNaturalWidth;
			}
		}
	}

	/* Store sizes calculated */
	if(outMinWidth) *outMinWidth=minWidth;
	if(outNaturalWidth) *outNaturalWidth=naturalWidth;
}

static void xfdashboard_viewpad_get_preferred_height(ClutterActor *inActor,
													gfloat inForWidth,
													gfloat *outMinHeight,
													gfloat *outNaturalWidth)
{
	XfdashboardViewpadPrivate	*priv=XFDASHBOARD_VIEWPAD(inActor)->priv;
	gfloat						minHeight, naturalHeight;

	/* Initially actor needs no space at all */
	minHeight=naturalHeight=0.0f;
	if(priv->activeView)
	{
		/* Our natural height is the natural height of the active view */
		clutter_actor_get_preferred_height(CLUTTER_ACTOR(priv->activeView),
											inForWidth,
											NULL,
											&naturalHeight);

		/* Check if we need to add space for horizontal scroll-bar */
		if(inForWidth>=0)
		{
			gfloat				viewNaturalWidth;
			gfloat				scrollbarMinHeight, scrollbarNaturalHeight;

			clutter_actor_get_preferred_width(CLUTTER_ACTOR(priv->activeView),
												-1.0f,
												NULL,
												&viewNaturalWidth);
			if(priv->verticalScrollbarPolicy==GTK_POLICY_ALWAYS ||
				(priv->verticalScrollbarPolicy==GTK_POLICY_AUTOMATIC &&
					viewNaturalWidth>inForWidth))
			{
				clutter_actor_get_preferred_height(priv->verticalScrollbar,
													inForWidth,
													&scrollbarMinHeight,
													&scrollbarNaturalHeight);
				minHeight+=scrollbarMinHeight;
				naturalHeight+=scrollbarNaturalHeight;
			}
		}
	}

	/* Store sizes calculated */
	if(outMinHeight) *outMinHeight=minHeight;
	if(outNaturalWidth) *outNaturalWidth=naturalHeight;
}

/* Allocate position and size of actor and its children*/
static void xfdashboard_viewpad_allocate(ClutterActor *inActor,
										const ClutterActorBox *inAllocation,
										ClutterAllocationFlags inFlags)
{
	XfdashboardViewpadPrivate	*priv=XFDASHBOARD_VIEWPAD(inActor)->priv;
	ClutterActorClass			*klass;
	ClutterActorBox				*box;
	gfloat						viewWidth, viewHeight;
	gfloat						vScrollWidth, vScrollHeight;
	gfloat						hScrollWidth, hScrollHeight;

	/* Chain up to store the allocation of the actor */
	klass=CLUTTER_ACTOR_CLASS(xfdashboard_viewpad_parent_class);
	klass->allocate(inActor, inAllocation, inFlags);

	/* Initialize largest possible allocation for view and determine
	 * real size of view to show. The real size is used to determine
	 * scroll bar visibility if policy is automatic */
	viewWidth=clutter_actor_box_get_width(inAllocation);
	viewHeight=clutter_actor_box_get_height(inAllocation);

	/* Set allocation for visible scroll bars */
	priv->horizontalScrollbarVisible=FALSE;
	if(priv->horizontalScrollbarPolicy==GTK_POLICY_ALWAYS ||
		(priv->horizontalScrollbarPolicy==GTK_POLICY_AUTOMATIC &&
			xfdashboard_scrollbar_get_range(XFDASHBOARD_SCROLLBAR(priv->horizontalScrollbar))>viewWidth))
	{
		priv->horizontalScrollbarVisible=TRUE;
	}

	priv->verticalScrollbarVisible=FALSE;
	if(priv->verticalScrollbarPolicy==GTK_POLICY_ALWAYS ||
		(priv->verticalScrollbarPolicy==GTK_POLICY_AUTOMATIC &&
			xfdashboard_scrollbar_get_range(XFDASHBOARD_SCROLLBAR(priv->verticalScrollbar))>viewHeight))
	{
		priv->verticalScrollbarVisible=TRUE;
	}

	vScrollWidth=0.0f;
	vScrollHeight=viewHeight;
	clutter_actor_get_preferred_width(priv->verticalScrollbar, -1, NULL, &vScrollWidth);

	hScrollWidth=viewWidth;
	hScrollHeight=0.0f;
	clutter_actor_get_preferred_height(priv->horizontalScrollbar, -1, NULL, &hScrollHeight);

	if(priv->horizontalScrollbarVisible && priv->verticalScrollbarVisible)
	{
		vScrollHeight-=hScrollHeight;
		hScrollWidth-=vScrollWidth;
	}

	if(priv->verticalScrollbarVisible)
	{
		box=clutter_actor_box_new(viewWidth-vScrollWidth, 0, viewWidth, vScrollHeight);
		clutter_actor_allocate(priv->verticalScrollbar, box, inFlags);
		clutter_actor_box_free(box);
	}

	if(priv->horizontalScrollbarVisible)
	{
		box=clutter_actor_box_new(0, viewHeight-hScrollHeight, hScrollWidth, viewHeight);
		clutter_actor_allocate(priv->horizontalScrollbar, box, inFlags);
		clutter_actor_box_free(box);
	}

	/* Reduce allocation for view by any visible scroll bar
	 * and set allocation and clipping of view
	 */
	if(priv->activeView)
	{
		gfloat					x, y;
		
		/* Set allocation */
		if(priv->verticalScrollbarVisible) viewWidth-=vScrollWidth;
		if(priv->horizontalScrollbarVisible) viewHeight-=hScrollHeight;

		x=ceilf(xfdashboard_scrollbar_get_value(XFDASHBOARD_SCROLLBAR(priv->horizontalScrollbar)));
		y=ceilf(xfdashboard_scrollbar_get_value(XFDASHBOARD_SCROLLBAR(priv->verticalScrollbar)));

		box=clutter_actor_box_new(-x, -y, viewWidth-x, viewHeight-y);
		clutter_actor_allocate(CLUTTER_ACTOR(priv->activeView), box, inFlags);
		clutter_actor_box_free(box);

		clutter_actor_set_clip(CLUTTER_ACTOR(priv->activeView), x, y, viewWidth, viewHeight);
	}
}

/* IMPLEMENTATION: GObject */

/* Dispose this object */
static void xfdashboard_viewpad_dispose(GObject *inObject)
{
	XfdashboardViewpad			*self=XFDASHBOARD_VIEWPAD(inObject);
	XfdashboardViewpadPrivate	*priv=self->priv;
	
	/* Release allocated resources */
	xfdashboard_viewpad_set_active_view(self, NULL);
	g_list_foreach(priv->views, (GFunc)clutter_actor_unparent, NULL);
	g_list_free(priv->views);
	priv->views=NULL;

	if(priv->verticalScrollbar) clutter_actor_destroy(priv->verticalScrollbar);
	priv->verticalScrollbar=NULL;

	if(priv->horizontalScrollbar) clutter_actor_destroy(priv->horizontalScrollbar);
	priv->horizontalScrollbar=NULL;

	/* Call parent's class dispose method */
	G_OBJECT_CLASS(xfdashboard_viewpad_parent_class)->dispose(inObject);
}

/* Set/get properties */
static void xfdashboard_viewpad_set_property(GObject *inObject,
												guint inPropID,
												const GValue *inValue,
												GParamSpec *inSpec)
{
	XfdashboardViewpad		*self=XFDASHBOARD_VIEWPAD(inObject);
	
	switch(inPropID)
	{
		case PROP_THICKNESS:
			xfdashboard_viewpad_set_thickness(self, g_value_get_float(inValue));
			break;
			
		case PROP_ACTIVE_VIEW:
			xfdashboard_viewpad_set_active_view(self, XFDASHBOARD_VIEW(g_value_get_object(inValue)));
			break;

		case PROP_VERTICAL_SCROLLBAR_POLICY:
			xfdashboard_viewpad_set_scrollbar_policy(self,
														self->priv->horizontalScrollbarPolicy,
														g_value_get_enum(inValue));
			break;

		case PROP_HORIZONTAL_SCROLLBAR_POLICY:
			xfdashboard_viewpad_set_scrollbar_policy(self,
														g_value_get_enum(inValue),
														self->priv->verticalScrollbarPolicy);
			break;

		default:
			G_OBJECT_WARN_INVALID_PROPERTY_ID(inObject, inPropID, inSpec);
			break;
	}
}

static void xfdashboard_viewpad_get_property(GObject *inObject,
												guint inPropID,
												GValue *outValue,
												GParamSpec *inSpec)
{
	XfdashboardViewpad		*self=XFDASHBOARD_VIEWPAD(inObject);

	switch(inPropID)
	{
		case PROP_THICKNESS:
			g_value_set_float(outValue, self->priv->thickness);
			break;

		case PROP_ACTIVE_VIEW:
			g_value_set_object(outValue, self->priv->activeView);
			break;

		case PROP_VERTICAL_SCROLLBAR:
			g_value_set_object(outValue, self->priv->verticalScrollbar);
			break;

		case PROP_VERTICAL_SCROLLBAR_POLICY:
			g_value_set_enum(outValue, self->priv->verticalScrollbarPolicy);
			break;

		case PROP_HORIZONTAL_SCROLLBAR:
			g_value_set_object(outValue, self->priv->horizontalScrollbar);
			break;

		case PROP_HORIZONTAL_SCROLLBAR_POLICY:
			g_value_set_enum(outValue, self->priv->verticalScrollbarPolicy);
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
	GObjectClass			*gobjectClass=G_OBJECT_CLASS(klass);
	ClutterActorClass		*actorClass=CLUTTER_ACTOR_CLASS(klass);

	/* Override functions */
	actorClass->paint=xfdashboard_viewpad_paint;
	actorClass->pick=xfdashboard_viewpad_pick;
	actorClass->get_preferred_width=xfdashboard_viewpad_get_preferred_width;
	actorClass->get_preferred_height=xfdashboard_viewpad_get_preferred_height;
	actorClass->allocate=xfdashboard_viewpad_allocate;

	gobjectClass->set_property=xfdashboard_viewpad_set_property;
	gobjectClass->get_property=xfdashboard_viewpad_get_property;
	gobjectClass->dispose=xfdashboard_viewpad_dispose;

	/* Set up private structure */
	g_type_class_add_private(klass, sizeof(XfdashboardViewpadPrivate));

	/* Define properties */
	XfdashboardViewpadProperties[PROP_THICKNESS]=
		g_param_spec_float("thickness",
							"Scroll bar thickness",
							"The thickness of vertical and horizontal scroll bars",
							0.0f, G_MAXFLOAT,
							DEFAULT_THICKNESS,
							G_PARAM_READWRITE);

	XfdashboardViewpadProperties[PROP_ACTIVE_VIEW]=
		g_param_spec_object("active-view",
							"Active view",
							"The active view shown currently",
							XFDASHBOARD_TYPE_VIEW,
							G_PARAM_READWRITE);

	XfdashboardViewpadProperties[PROP_VERTICAL_SCROLLBAR]=
		g_param_spec_object("vertical-scrollbar",
							"Vertical scroll bar",
							"The vertical scroll bar",
							XFDASHBOARD_TYPE_SCROLLBAR,
							G_PARAM_READABLE);

	XfdashboardViewpadProperties[PROP_VERTICAL_SCROLLBAR_POLICY]=
		g_param_spec_enum("vertical-scrollbar-policy",
							"Vertical scrollbar policy",
							"When the vertical scrollbar is displayed",
							GTK_TYPE_POLICY_TYPE,
							DEFAULT_SCROLLBAR_POLICY,
							G_PARAM_READWRITE);

	XfdashboardViewpadProperties[PROP_HORIZONTAL_SCROLLBAR]=
		g_param_spec_object("horizontal-scrollbar",
							"Horizontal scroll bar",
							"The horizontal scroll bar",
							XFDASHBOARD_TYPE_SCROLLBAR,
							G_PARAM_READABLE);

	XfdashboardViewpadProperties[PROP_HORIZONTAL_SCROLLBAR_POLICY]=
		g_param_spec_enum("horizontal-scrollbar-policy",
							"Horizontal scrollbar policy",
							"When the horizontal scrollbar is displayed",
							GTK_TYPE_POLICY_TYPE,
							DEFAULT_SCROLLBAR_POLICY,
							G_PARAM_READWRITE);

	g_object_class_install_properties(gobjectClass, PROP_LAST, XfdashboardViewpadProperties);

	/* Define signals */
	XfdashboardViewpadSignals[VIEW_ADDED]=
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

	XfdashboardViewpadSignals[VIEW_REMOVED]=
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

	XfdashboardViewpadSignals[VIEW_ACTIVATED]=
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

	XfdashboardViewpadSignals[VIEW_DEACTIVATED]=
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

	priv=self->priv=XFDASHBOARD_VIEWPAD_GET_PRIVATE(self);

	/* Set up default values */
	priv->views=NULL;
	priv->activeView=NULL;
	priv->thickness=DEFAULT_THICKNESS;
	priv->horizontalScrollbarVisible=FALSE;
	priv->verticalScrollbarVisible=FALSE;
	priv->horizontalScrollbarPolicy=DEFAULT_SCROLLBAR_POLICY;
	priv->verticalScrollbarPolicy=DEFAULT_SCROLLBAR_POLICY;

	/* Create scroll bar actors */
	priv->verticalScrollbar=xfdashboard_scrollbar_new_with_thickness(priv->thickness);
	xfdashboard_scrollbar_set_vertical(XFDASHBOARD_SCROLLBAR(priv->verticalScrollbar), TRUE);
	clutter_actor_set_parent(priv->verticalScrollbar, CLUTTER_ACTOR(self));

	priv->horizontalScrollbar=xfdashboard_scrollbar_new_with_thickness(priv->thickness);
	xfdashboard_scrollbar_set_vertical(XFDASHBOARD_SCROLLBAR(priv->horizontalScrollbar), FALSE);
	clutter_actor_set_parent(priv->horizontalScrollbar, CLUTTER_ACTOR(self));

	/* Connect signals */
	clutter_actor_set_reactive(CLUTTER_ACTOR(self), TRUE);
	g_signal_connect(self, "scroll-event", G_CALLBACK(_xfdashboard_viewpad_scroll_event), NULL);
	g_signal_connect(priv->verticalScrollbar, "value-changed", G_CALLBACK(_xfdashboard_viewpad_value_changed), self);
	g_signal_connect(priv->horizontalScrollbar, "value-changed", G_CALLBACK(_xfdashboard_viewpad_value_changed), self);
}

/* Implementation: Public API */

/* Create new actor */
ClutterActor* xfdashboard_viewpad_new()
{
	return(g_object_new(XFDASHBOARD_TYPE_VIEWPAD, NULL));
}

/* Get list of all views */
GList* xfdashboard_viewpad_get_views(XfdashboardViewpad *self)
{
	g_return_val_if_fail(XFDASHBOARD_IS_VIEWPAD(self), NULL);

	return(self->priv->views);
}

/* Add a view */
void xfdashboard_viewpad_add_view(XfdashboardViewpad *self, XfdashboardView *inView)
{
	g_return_if_fail(XFDASHBOARD_IS_VIEWPAD(self));
	g_return_if_fail(XFDASHBOARD_IS_VIEW(inView));

	XfdashboardViewpadPrivate	*priv=self->priv;
	GList						*view;
	
	/* Only add view if it does not exists in list of views */
	view=g_list_find(priv->views, inView);
	if(G_UNLIKELY(view))
	{
		g_warning("Trying to add an already added view (name: %s)",
					xfdashboard_view_get_view_name(inView));
		return;
	}

	/* Add view to list, set its parent to this viewpad and emit signal */
	priv->views=g_list_append(priv->views, inView);
	clutter_actor_reparent(CLUTTER_ACTOR(inView), CLUTTER_ACTOR(self));
	g_signal_connect(CLUTTER_ACTOR(inView), "allocation-changed", G_CALLBACK(_xfdashboard_viewpad_allocation_changed), self);
	g_signal_connect(CLUTTER_ACTOR(inView), "reset-scrollbars", G_CALLBACK(_xfdashboard_viewpad_reset_scrollbars), self);
	g_signal_emit_by_name(self, "view-added", inView);
}

/* Remove a view */
void xfdashboard_viewpad_remove_view(XfdashboardViewpad *self, XfdashboardView *inView)
{
	g_return_if_fail(XFDASHBOARD_IS_VIEWPAD(self));
	g_return_if_fail(XFDASHBOARD_IS_VIEW(inView));

	XfdashboardViewpadPrivate	*priv=self->priv;
	GList						*view;
	
	/* Lookup view to remove */
	view=g_list_find(priv->views, inView);
	if(G_UNLIKELY(!view))
	{
		g_warning("Trying to remove a non-existent view (name: %s)",
					xfdashboard_view_get_view_name(inView));
		return;
	}

	/* If we are removing active view set NULL-view */
	if(inView==priv->activeView)
	{
		xfdashboard_viewpad_set_active_view(self, NULL);
	}

	/* Remove view */
	priv->views=g_list_remove(priv->views, inView);
	clutter_actor_unparent(CLUTTER_ACTOR(inView));
	g_signal_handlers_disconnect_by_func(CLUTTER_ACTOR(inView), (gpointer)_xfdashboard_viewpad_allocation_changed, self);
	g_signal_handlers_disconnect_by_func(CLUTTER_ACTOR(inView), (gpointer)_xfdashboard_viewpad_reset_scrollbars, self);
	g_signal_emit_by_name(self, "view-removed", inView);

	clutter_actor_queue_redraw(CLUTTER_ACTOR(self));
}

/* Get current active view */
XfdashboardView* xfdashboard_viewpad_get_active_view(XfdashboardViewpad *self)
{
	g_return_val_if_fail(XFDASHBOARD_IS_VIEWPAD(self), NULL);

	return(self->priv->activeView);
}

/* Set active view */
void xfdashboard_viewpad_set_active_view(XfdashboardViewpad *self, XfdashboardView *inView)
{
	g_return_if_fail(XFDASHBOARD_IS_VIEWPAD(self));
	g_return_if_fail(!inView || XFDASHBOARD_IS_VIEW(inView));

	XfdashboardViewpadPrivate	*priv=self->priv;

	/* Only change active view if new one differs from old one */
	if(priv->activeView==inView) return;
	
	/* Check if view to activate exists */
	if(G_UNLIKELY(inView && !g_list_find(self->priv->views, inView)))
	{
		g_warning("Trying to activate a non-existent view (name: %s)",
					xfdashboard_view_get_view_name(inView));
		return;
	}
	
	/* Deactivate current active view by hiding it */
	if(priv->activeView)
	{
		/* Hide actor but remove any clipping before */
		clutter_actor_remove_clip(CLUTTER_ACTOR(priv->activeView));
		clutter_actor_hide(CLUTTER_ACTOR(priv->activeView));
		g_signal_emit_by_name(priv->activeView, "deactivated", NULL);
		g_signal_emit_by_name(self, "view-deactivated", priv->activeView);
		priv->activeView=NULL;
	}

	/* Activate requested view by showing it */
	if(inView)
	{
		/* Set range for scroll bars */
		gfloat					w, h;

		clutter_actor_get_preferred_size(CLUTTER_ACTOR(inView), NULL, NULL, &w, &h);
		xfdashboard_scrollbar_set_range(XFDASHBOARD_SCROLLBAR(priv->horizontalScrollbar), w);
		xfdashboard_scrollbar_set_range(XFDASHBOARD_SCROLLBAR(priv->verticalScrollbar), h);
		xfdashboard_scrollbar_set_value(XFDASHBOARD_SCROLLBAR(priv->horizontalScrollbar), 0);
		xfdashboard_scrollbar_set_value(XFDASHBOARD_SCROLLBAR(priv->verticalScrollbar), 0);

		/* Show view */
		priv->activeView=inView;
		clutter_actor_show(CLUTTER_ACTOR(priv->activeView));
		g_signal_emit_by_name(priv->activeView, "activated", NULL);
		g_signal_emit_by_name(self, "view-activated", priv->activeView);

		clutter_actor_queue_relayout(CLUTTER_ACTOR(self));
	}
}

/* Get scroll bar actors */
XfdashboardScrollbar* xfdashboard_viewpad_get_vertical_scrollbar(XfdashboardViewpad *self)
{
	g_return_val_if_fail(XFDASHBOARD_IS_VIEWPAD(self), NULL);

	return(XFDASHBOARD_SCROLLBAR(self->priv->verticalScrollbar));
}

XfdashboardScrollbar* xfdashboard_viewpad_get_horizontal_scrollbar(XfdashboardViewpad *self)
{
	g_return_val_if_fail(XFDASHBOARD_IS_VIEWPAD(self), NULL);

	return(XFDASHBOARD_SCROLLBAR(self->priv->horizontalScrollbar));
}

/* Get/set scroll bar policy */
void xfdashboard_viewpad_get_scrollbar_policy(XfdashboardViewpad *self,
												GtkPolicyType *outHorizontalPolicy,
												GtkPolicyType *outVerticalPolicy)
{
	g_return_if_fail(XFDASHBOARD_IS_VIEWPAD(self));

	if(outHorizontalPolicy) *outHorizontalPolicy=self->priv->horizontalScrollbarPolicy;
	if(outVerticalPolicy) *outVerticalPolicy=self->priv->verticalScrollbarPolicy;
}

void xfdashboard_viewpad_set_scrollbar_policy(XfdashboardViewpad *self,
												GtkPolicyType inHorizontalPolicy,
												GtkPolicyType inVerticalPolicy)
{
	g_return_if_fail(XFDASHBOARD_IS_VIEWPAD(self));

	/* Only set new value if it differs from current value */
	XfdashboardViewpadPrivate	*priv=self->priv;

	if(inHorizontalPolicy!=priv->horizontalScrollbarPolicy ||
		inVerticalPolicy!=priv->verticalScrollbarPolicy)
	{
		priv->horizontalScrollbarPolicy=inHorizontalPolicy;
		priv->verticalScrollbarPolicy=inVerticalPolicy;

		clutter_actor_queue_relayout(CLUTTER_ACTOR(self));
	}
}

/* Get/set thickness of scroll bars */
gfloat xfdashboard_viewpad_get_thickness(XfdashboardViewpad *self)
{
	g_return_val_if_fail(XFDASHBOARD_IS_VIEWPAD(self), 0.0f);

	return(self->priv->thickness);
}

void xfdashboard_viewpad_set_thickness(XfdashboardViewpad *self, gfloat inThickness)
{
	g_return_if_fail(XFDASHBOARD_IS_VIEWPAD(self));
	g_return_if_fail(inThickness>0.0f);

	/* Only set new value if it differs from current value */
	XfdashboardViewpadPrivate	*priv=self->priv;

	if(inThickness!=priv->thickness)
	{
		priv->thickness=inThickness;
		xfdashboard_scrollbar_set_thickness(XFDASHBOARD_SCROLLBAR(priv->verticalScrollbar), inThickness);
		xfdashboard_scrollbar_set_thickness(XFDASHBOARD_SCROLLBAR(priv->horizontalScrollbar), inThickness);

		clutter_actor_queue_relayout(CLUTTER_ACTOR(self));
	}
}
