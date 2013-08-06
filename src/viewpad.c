/*
 * viewpad: A viewpad managing views
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

#include "viewpad.h"
#include "view-manager.h"
#include "scrollbar.h"
#include "utils.h"
#include "enums.h"

/* Define this class in GObject system */
G_DEFINE_TYPE(XfdashboardViewpad,
				xfdashboard_viewpad,
				CLUTTER_TYPE_ACTOR)

/* Private structure - access only by public API if needed */
#define XFDASHBOARD_VIEWPAD_GET_PRIVATE(obj) \
	(G_TYPE_INSTANCE_GET_PRIVATE((obj), XFDASHBOARD_TYPE_VIEWPAD, XfdashboardViewpadPrivate))

struct _XfdashboardViewpadPrivate
{
	/* Properties related */
	gfloat					spacing;
	XfdashboardView			*activeView;
	XfdashboardPolicy		hScrollbarPolicy;
	gboolean				hScrollbarVisible;
	XfdashboardPolicy		vScrollbarPolicy;
	gboolean				vScrollbarVisible;

	/* Instance related */
	XfdashboardViewManager	*viewManager;

	ClutterLayoutManager	*layout;
	ClutterActor			*container;
	ClutterActor			*hScrollbar;
	ClutterActor			*vScrollbar;
};

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

GParamSpec* XfdashboardViewpadProperties[PROP_LAST]={ 0, };

/* Signals */
enum
{
	SIGNAL_VIEW_ACTIVATING,
	SIGNAL_VIEW_ACTIVATED,
	SIGNAL_VIEW_DEACTIVATING,
	SIGNAL_VIEW_DEACTIVATED,

	SIGNAL_LAST
};

guint XfdashboardViewpadSignals[SIGNAL_LAST]={ 0, };

/* IMPLEMENTATION: Private variables and methods */
#define DEFAULT_SPACING				4.0f
#define DEFAULT_SCROLLBAR_POLICY	XFDASHBOARD_POLICY_AUTOMATIC

/* Update view depending on scrollbar values */
void _xfdashboard_viewpad_update_view_viewport(XfdashboardViewpad *self)
{
	g_return_if_fail(XFDASHBOARD_IS_VIEWPAD(self));

	XfdashboardViewpadPrivate	*priv=self->priv;
	ClutterMatrix				transform;
	gfloat						x, y, w, h;

	/* Check for active view */
	if(priv->activeView==NULL)
	{
		g_warning(_("Cannot update viewport of view because no one is active"));
		return;
	}

	/* Get offset from scrollbars */
	x=xfdashboard_scrollbar_get_value(XFDASHBOARD_SCROLLBAR(priv->hScrollbar));
	y=xfdashboard_scrollbar_get_value(XFDASHBOARD_SCROLLBAR(priv->vScrollbar));

	/* Get view size from clipping */
	clutter_actor_get_clip(CLUTTER_ACTOR(priv->activeView), NULL, NULL, &w, &h);

	/* Set transformation (offset) */
	cogl_matrix_init_identity(&transform);
	cogl_matrix_translate(&transform, -x, -y, 0.0f);
	clutter_actor_set_transform(priv->activeView, &transform);

	/* Set new clipping */
	clutter_actor_set_clip(CLUTTER_ACTOR(priv->activeView), x, y, w, h);
}

/* The value of a scrollbar has changed */
void _xfdashboard_viewpad_on_scrollbar_value_changed(XfdashboardViewpad *self,
														gfloat inValue,
														gpointer inUserData)
{
	g_return_if_fail(XFDASHBOARD_IS_VIEWPAD(self));

	_xfdashboard_viewpad_update_view_viewport(self);
}

/* Allocation of a view changed */
void _xfdashboard_viewpad_update_scrollbars(XfdashboardViewpad *self)
{
	g_return_if_fail(XFDASHBOARD_IS_VIEWPAD(self));

	XfdashboardViewpadPrivate	*priv=self->priv;
	gfloat						w, h;

	/* Set range of scroll bar to width and height of active view */
	if(priv->activeView) clutter_actor_get_preferred_size(priv->activeView, NULL, NULL, &w, &h);
		else w=h=1.0f;

	xfdashboard_scrollbar_set_range(XFDASHBOARD_SCROLLBAR(priv->hScrollbar), w);
	xfdashboard_scrollbar_set_range(XFDASHBOARD_SCROLLBAR(priv->vScrollbar), h);
}

/* Set new active view and deactive current one */
void _xfdashboard_viewpad_activate_view(XfdashboardViewpad *self, XfdashboardView *inView)
{
	g_return_if_fail(XFDASHBOARD_IS_VIEWPAD(self));
	g_return_if_fail(inView==NULL || XFDASHBOARD_IS_VIEW(inView));

	XfdashboardViewpadPrivate	*priv=self->priv;
	gfloat						w, h;

	/* Check if view is a child of this actor */
	if(inView && clutter_actor_contains(CLUTTER_ACTOR(self), CLUTTER_ACTOR(inView))==FALSE)
	{
		g_warning(_("View %s is not a child of %s and cannot be activated"),
					G_OBJECT_TYPE_NAME(inView), G_OBJECT_TYPE_NAME(self));
		return;
	}

	/* Only set value if it changes */
	if(inView==priv->activeView) return;

	/* Deactivate current view */
	if(priv->activeView)
	{
		/* Hide current view and emit signal before and after deactivation */
		g_signal_emit(self, XfdashboardViewpadSignals[SIGNAL_VIEW_DEACTIVATING], 0, priv->activeView);
		g_signal_emit_by_name(priv->activeView, "deactivating");

		clutter_actor_hide(CLUTTER_ACTOR(priv->activeView));
		g_debug("Deactivated view %s", G_OBJECT_TYPE_NAME(priv->activeView));

		g_signal_emit_by_name(priv->activeView, "deactivated");
		g_signal_emit(self, XfdashboardViewpadSignals[SIGNAL_VIEW_DEACTIVATED], 0, priv->activeView);

		g_object_unref(priv->activeView);
		priv->activeView=NULL;
	}

	/* Activate new view (if available) by showing new view, setting up
	 * scrollbars and emitting signal before and after activation
	 */
	if(inView)
	{
		priv->activeView=g_object_ref(inView);

		g_signal_emit(self, XfdashboardViewpadSignals[SIGNAL_VIEW_ACTIVATING], 0, priv->activeView);
		g_signal_emit_by_name(priv->activeView, "activating");

		_xfdashboard_viewpad_update_scrollbars(self);
		xfdashboard_scrollbar_set_value(XFDASHBOARD_SCROLLBAR(priv->hScrollbar), 0);
		xfdashboard_scrollbar_set_value(XFDASHBOARD_SCROLLBAR(priv->vScrollbar), 0);
		clutter_actor_show(CLUTTER_ACTOR(priv->activeView));
		g_debug("Activated view %s", G_OBJECT_TYPE_NAME(priv->activeView));

		g_signal_emit_by_name(priv->activeView, "activated");
		g_signal_emit(self, XfdashboardViewpadSignals[SIGNAL_VIEW_ACTIVATED], 0, priv->activeView);
	}

	/* Notify about property change */
	g_object_notify_by_pspec(G_OBJECT(self), XfdashboardViewpadProperties[PROP_ACTIVE_VIEW]);
}

/* Allocation of a view changed */
void _xfdashboard_viewpad_on_allocation_changed(ClutterActor *inActor,
												ClutterActorBox *inBox,
												ClutterAllocationFlags inFlags,
												gpointer inUserData)
{
	g_return_if_fail(XFDASHBOARD_IS_VIEWPAD(inActor));
	g_return_if_fail(XFDASHBOARD_IS_VIEW(inUserData));

	XfdashboardViewpad			*self=XFDASHBOARD_VIEWPAD(inActor);
	XfdashboardViewpadPrivate	*priv=self->priv;
	XfdashboardView				*view=XFDASHBOARD_VIEW(inUserData);

	/* Update scrollbars only if view whose allocation has changed
	 * is the active one
	 */
	if(view==priv->activeView) _xfdashboard_viewpad_update_scrollbars(self);
}

/* Create view of given type and add to this actor */
void _xfdashboard_viewpad_add_view(XfdashboardViewpad *self, GType inViewType)
{
	g_return_if_fail(XFDASHBOARD_IS_VIEWPAD(self));

	XfdashboardViewpadPrivate	*priv=self->priv;
	GObject						*view;

	/* Create instance and check if it is a view */
	g_debug("Creating view %s for viewpad", g_type_name(inViewType));

	view=g_object_new(inViewType, NULL);
	if(view==NULL)
	{
		g_critical(_("Failed to create view instance of %s"), g_type_name(inViewType));
		return;
	}

	if(XFDASHBOARD_IS_VIEW(view)!=TRUE)
	{
		g_critical(_("Instance %s is not a %s and cannot be added to %s"),
					g_type_name(inViewType), g_type_name(XFDASHBOARD_TYPE_VIEW), G_OBJECT_TYPE_NAME(self));
		return;
	}

	/* Add new view instance to this actor but hidden */
	clutter_actor_hide(CLUTTER_ACTOR(view));
	clutter_actor_add_child(CLUTTER_ACTOR(self), CLUTTER_ACTOR(view));
	g_signal_connect_swapped(CLUTTER_ACTOR(view), "allocation-changed", G_CALLBACK(_xfdashboard_viewpad_on_allocation_changed), self);

	/* Set active view if none active (usually it is the first view created) */
	if(priv->activeView==NULL) _xfdashboard_viewpad_activate_view(self, XFDASHBOARD_VIEW(view));
}

/* Called when a new view type was registered */
void _xfdashboard_viewpad_on_view_registered(XfdashboardViewpad *self,
												GType inViewType,
												gpointer inUserData)
{
	g_return_if_fail(XFDASHBOARD_IS_VIEWPAD(self));

	_xfdashboard_viewpad_add_view(self, inViewType);
}

/* Called when a view type was unregistered */
void _xfdashboard_viewpad_on_view_unregistered(XfdashboardViewpad *self,
												GType inViewType,
												gpointer inUserData)
{
	g_return_if_fail(XFDASHBOARD_IS_VIEWPAD(self));

	XfdashboardViewpadPrivate	*priv=self->priv;
	ClutterActorIter			iter;
	ClutterActor				*child, *firstActivatableView;

	/* Iterate through create views and lookup view of given type */
	clutter_actor_iter_init(&iter, CLUTTER_ACTOR(self));
	while(clutter_actor_iter_next(&iter, &child))
	{
		/* Check if child is a view otherwise continue iterating */
		if(XFDASHBOARD_IS_VIEW(child)!=TRUE) continue;

		/* If child is not of type being unregistered it will get
		 * the first activatable view after we destroyed all views found.
		 */
		if(G_OBJECT_TYPE(child)!=inViewType) firstActivatableView=child;
			else
			{
				if(child==priv->activeView) _xfdashboard_viewpad_activate_view(self, NULL);
				clutter_actor_destroy(child);
			}
	}

	/* Now activate the first activatable view we found during iteration */
	if(firstActivatableView) _xfdashboard_viewpad_activate_view(self, XFDASHBOARD_VIEW(firstActivatableView));
}

/* IMPLEMENTATION: ClutterActor */

/* Show this actor and the current active view */
void _xfdashboard_viewpad_show(ClutterActor *self)
{
	XfdashboardViewpadPrivate	*priv=XFDASHBOARD_VIEWPAD(self)->priv;
	ClutterActorClass			*actorClass=CLUTTER_ACTOR_CLASS(xfdashboard_viewpad_parent_class);

	/* Only show active view again */
	if(priv->activeView) clutter_actor_show(CLUTTER_ACTOR(priv->activeView));

	/* Call parent's class show method */
	if(actorClass->show) actorClass->show(self);
}

/* Get preferred width/height */
void _xfdashboard_viewpad_get_preferred_height(ClutterActor *self,
													gfloat inForWidth,
													gfloat *outMinHeight,
													gfloat *outNaturalHeight)
{
	XfdashboardScrollbarPrivate		*priv=XFDASHBOARD_VIEWPAD(self)->priv;
	gfloat							minHeight, naturalHeight;

	/* Do not set any minimum or natural sizes. The parent actor is responsible
	 * to set this actor's sizes as viewport.
	 */
	minHeight=naturalHeight=0.0f;

	/* Store sizes computed */
	if(outMinHeight) *outMinHeight=minHeight;
	if(outNaturalHeight) *outNaturalHeight=naturalHeight;
}

void _xfdashboard_viewpad_get_preferred_width(ClutterActor *self,
												gfloat inForHeight,
												gfloat *outMinWidth,
												gfloat *outNaturalWidth)
{
	XfdashboardScrollbarPrivate		*priv=XFDASHBOARD_VIEWPAD(self)->priv;
	gfloat							minWidth, naturalWidth;

	/* Do not set any minimum or natural sizes. The parent actor is responsible
	 * to set this actor's sizes as viewport.
	 */
	minWidth=naturalWidth=0.0f;

	/* Store sizes computed */
	if(outMinWidth) *outMinWidth=minWidth;
	if(outNaturalWidth) *outNaturalWidth=naturalWidth;
}

/* Allocate position and size of actor and its children*/
void _xfdashboard_viewpad_allocate(ClutterActor *self,
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
	if(priv->hScrollbarPolicy==XFDASHBOARD_POLICY_ALWAYS ||
		(priv->hScrollbarPolicy==XFDASHBOARD_POLICY_AUTOMATIC &&
			xfdashboard_scrollbar_get_range(XFDASHBOARD_SCROLLBAR(priv->hScrollbar))>viewWidth))
	{
		hScrollbarVisible=TRUE;
	}

	vScrollbarVisible=FALSE;
	if(priv->vScrollbarPolicy==XFDASHBOARD_POLICY_ALWAYS ||
		(priv->vScrollbarPolicy==XFDASHBOARD_POLICY_AUTOMATIC &&
			xfdashboard_scrollbar_get_range(XFDASHBOARD_SCROLLBAR(priv->vScrollbar))>viewHeight))
	{
		vScrollbarVisible=TRUE;
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

		x=ceilf(xfdashboard_scrollbar_get_value(XFDASHBOARD_SCROLLBAR(priv->hScrollbar)));
		y=ceilf(xfdashboard_scrollbar_get_value(XFDASHBOARD_SCROLLBAR(priv->vScrollbar)));

		clutter_actor_get_preferred_size(priv->activeView, NULL, NULL, &w, &h);

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
}

/* IMPLEMENTATION: GObject */

/* Dispose this object */
void _xfdashboard_viewpad_dispose(GObject *inObject)
{
	XfdashboardViewpad			*self=XFDASHBOARD_VIEWPAD(inObject);
	XfdashboardViewpadPrivate	*priv=self->priv;

	/* Deactivate current view */
	if(priv->activeView) _xfdashboard_viewpad_activate_view(self, NULL);

	/* Disconnect signals handlers */
	if(priv->viewManager)
	{
		g_signal_handlers_disconnect_by_data(priv->viewManager, self);
		g_object_unref(priv->viewManager);
		priv->viewManager=NULL;
	}

	/* Call parent's class dispose method */
	G_OBJECT_CLASS(xfdashboard_viewpad_parent_class)->dispose(inObject);
}

/* Set/get properties */
void _xfdashboard_viewpad_set_property(GObject *inObject,
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
			xfdashboard_viewpad_set_horizontal_scrollbar_policy(self, (XfdashboardPolicy)g_value_get_enum(inValue));
			break;

		case PROP_VSCROLLBAR_POLICY:
			xfdashboard_viewpad_set_vertical_scrollbar_policy(self, (XfdashboardPolicy)g_value_get_enum(inValue));
			break;

		default:
			G_OBJECT_WARN_INVALID_PROPERTY_ID(inObject, inPropID, inSpec);
			break;
	}
}

void _xfdashboard_viewpad_get_property(GObject *inObject,
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
void xfdashboard_viewpad_class_init(XfdashboardViewpadClass *klass)
{
	ClutterActorClass		*actorClass=CLUTTER_ACTOR_CLASS(klass);
	GObjectClass			*gobjectClass=G_OBJECT_CLASS(klass);

	/* Override functions */
	gobjectClass->set_property=_xfdashboard_viewpad_set_property;
	gobjectClass->get_property=_xfdashboard_viewpad_get_property;
	gobjectClass->dispose=_xfdashboard_viewpad_dispose;

	actorClass->show=_xfdashboard_viewpad_show;
	actorClass->get_preferred_width=_xfdashboard_viewpad_get_preferred_width;
	actorClass->get_preferred_height=_xfdashboard_viewpad_get_preferred_height;
	actorClass->allocate=_xfdashboard_viewpad_allocate;

	/* Set up private structure */
	g_type_class_add_private(klass, sizeof(XfdashboardViewpadPrivate));

	/* Define properties */
	XfdashboardViewpadProperties[PROP_SPACING]=
		g_param_spec_float("spacing",
							_("Spacing"),
							_("The spacing between views and scrollbars"),
							0.0f, G_MAXFLOAT,
							DEFAULT_SPACING,
							G_PARAM_READWRITE);

	XfdashboardViewpadProperties[PROP_ACTIVE_VIEW]=
		g_param_spec_object("active-view",
								_("Active view"),
								_("The current active view in viewpad"),
								XFDASHBOARD_TYPE_VIEW,
								G_PARAM_READABLE);

	XfdashboardViewpadProperties[PROP_HSCROLLBAR_VISIBLE]=
		g_param_spec_boolean("horinzontal-scrollbar-visible",
								_("Horinzontal scrollbar visibility"),
								_("This flag indicates if horizontal scrollbar is visible"),
								FALSE,
								G_PARAM_READABLE);

	XfdashboardViewpadProperties[PROP_HSCROLLBAR_POLICY]=
		g_param_spec_enum("horinzontal-scrollbar-policy",
							_("Horinzontal scrollbar policy"),
							_("The policy for horizontal scrollbar controlling when it is displayed"),
							XFDASHBOARD_TYPE_POLICY,
							DEFAULT_SCROLLBAR_POLICY,
							G_PARAM_READWRITE);

	XfdashboardViewpadProperties[PROP_VSCROLLBAR_VISIBLE]=
		g_param_spec_boolean("vertical-scrollbar-visible",
								_("Vertical scrollbar visibility"),
								_("This flag indicates if vertical scrollbar is visible"),
								FALSE,
								G_PARAM_READABLE);

	XfdashboardViewpadProperties[PROP_VSCROLLBAR_POLICY]=
		g_param_spec_enum("vertical-scrollbar-policy",
							_("Vertical scrollbar policy"),
							_("The policy for vertical scrollbar controlling when it is displayed"),
							XFDASHBOARD_TYPE_POLICY,
							DEFAULT_SCROLLBAR_POLICY,
							G_PARAM_READWRITE);

	g_object_class_install_properties(gobjectClass, PROP_LAST, XfdashboardViewpadProperties);

	/* Define signals */
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
void xfdashboard_viewpad_init(XfdashboardViewpad *self)
{
	XfdashboardViewpadPrivate	*priv;
	GList						*views;
	ClutterLayoutManager		*layout;

	priv=self->priv=XFDASHBOARD_VIEWPAD_GET_PRIVATE(self);

	/* Set up default values */
	priv->viewManager=XFDASHBOARD_VIEW_MANAGER(g_object_ref(xfdashboard_view_manager_get_default()));
	priv->activeView=NULL;
	priv->spacing=DEFAULT_SPACING;
	priv->hScrollbarVisible=FALSE;
	priv->hScrollbarPolicy=DEFAULT_SCROLLBAR_POLICY;
	priv->vScrollbarVisible=FALSE;
	priv->vScrollbarPolicy=DEFAULT_SCROLLBAR_POLICY;

	/* Set up actor */
	priv->hScrollbar=xfdashboard_scrollbar_new(CLUTTER_ORIENTATION_HORIZONTAL);
	clutter_actor_add_child(CLUTTER_ACTOR(self), priv->hScrollbar);
	g_signal_connect_swapped(priv->hScrollbar, "value-changed", G_CALLBACK(_xfdashboard_viewpad_on_scrollbar_value_changed), self);

	priv->vScrollbar=xfdashboard_scrollbar_new(CLUTTER_ORIENTATION_VERTICAL);
	clutter_actor_add_child(CLUTTER_ACTOR(self), priv->vScrollbar);
	g_signal_connect_swapped(priv->vScrollbar, "value-changed", G_CALLBACK(_xfdashboard_viewpad_on_scrollbar_value_changed), self);

	/* Create instance of each registered view type and add it to this actor
	 * and connect signals
	 */
	for(views=xfdashboard_view_manager_get_registered(priv->viewManager); views; views=g_list_next(views))
	{
		GType					viewType;

		viewType=(GType)LISTITEM_TO_GTYPE(views->data);
		_xfdashboard_viewpad_add_view(self, viewType);
	}

	g_signal_connect_swapped(priv->viewManager, "registered", G_CALLBACK(_xfdashboard_viewpad_on_view_registered), self);
	g_signal_connect_swapped(priv->viewManager, "unregistered", G_CALLBACK(_xfdashboard_viewpad_on_view_unregistered), self);
}

/* Implementation: Public API */

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
	g_return_if_fail(XFDASHBOARD_IS_VIEWPAD(self));
	g_return_if_fail(inSpacing>=0.0f);

	XfdashboardViewpadPrivate	*priv=self->priv;

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

/* Get current active view */
XfdashboardView* xfdashboard_viewpad_get_active_view(XfdashboardViewpad *self)
{
	g_return_val_if_fail(XFDASHBOARD_IS_VIEWPAD(self), NULL);

	return(self->priv->activeView);
}

/* Get/set scroll bar visibility */
gboolean xfdashboard_viewpad_get_horizontal_scrollbar_visible(XfdashboardViewpad *self)
{
	g_return_if_fail(XFDASHBOARD_IS_VIEWPAD(self));

	return(self->priv->hScrollbarVisible);
}

gboolean xfdashboard_viewpad_get_vertical_scrollbar_visible(XfdashboardViewpad *self)
{
	g_return_if_fail(XFDASHBOARD_IS_VIEWPAD(self));

	return(self->priv->vScrollbarVisible);
}

/* Get/set scroll bar policy */
XfdashboardPolicy xfdashboard_viewpad_get_horizontal_scrollbar_policy(XfdashboardViewpad *self)
{
	g_return_if_fail(XFDASHBOARD_IS_VIEWPAD(self));

	return(self->priv->hScrollbarPolicy);
}

void xfdashboard_viewpad_set_horizontal_scrollbar_policy(XfdashboardViewpad *self, XfdashboardPolicy inPolicy)
{
	g_return_if_fail(XFDASHBOARD_IS_VIEWPAD(self));

	XfdashboardViewpadPrivate	*priv=self->priv;

	/* Only set new value if it differs from current value */
	if(priv->hScrollbarPolicy==inPolicy) return;

	/* Set new value */
	priv->hScrollbarPolicy=inPolicy;
	clutter_actor_queue_relayout(CLUTTER_ACTOR(self));

	/* Notify about property change */
	g_object_notify_by_pspec(G_OBJECT(self), XfdashboardViewpadProperties[PROP_HSCROLLBAR_POLICY]);
}

XfdashboardPolicy xfdashboard_viewpad_get_vertical_scrollbar_policy(XfdashboardViewpad *self)
{
	g_return_if_fail(XFDASHBOARD_IS_VIEWPAD(self));

	return(self->priv->vScrollbarPolicy);
}

void xfdashboard_viewpad_set_vertical_scrollbar_policy(XfdashboardViewpad *self, XfdashboardPolicy inPolicy)
{
	g_return_if_fail(XFDASHBOARD_IS_VIEWPAD(self));

	XfdashboardViewpadPrivate	*priv=self->priv;

	/* Only set new value if it differs from current value */
	if(priv->vScrollbarPolicy==inPolicy) return;

	/* Set new value */
	priv->vScrollbarPolicy=inPolicy;
	clutter_actor_queue_relayout(CLUTTER_ACTOR(self));

	/* Notify about property change */
	g_object_notify_by_pspec(G_OBJECT(self), XfdashboardViewpadProperties[PROP_VSCROLLBAR_POLICY]);
}
