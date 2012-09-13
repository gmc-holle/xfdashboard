/*
 * view.c: A view optional with scrollbars
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

#include "view.h"
#include "viewpad.h"

/* Define this class in GObject system */
static void clutter_container_iface_init(ClutterContainerIface *iface);

G_DEFINE_ABSTRACT_TYPE_WITH_CODE(XfdashboardView,
									xfdashboard_view,
									CLUTTER_TYPE_ACTOR,
									G_IMPLEMENT_INTERFACE(CLUTTER_TYPE_CONTAINER, clutter_container_iface_init))

/* Private structure - access only by public API if needed */
#define XFDASHBOARD_VIEW_GET_PRIVATE(obj) \
	(G_TYPE_INSTANCE_GET_PRIVATE((obj), XFDASHBOARD_TYPE_VIEW, XfdashboardViewPrivate))

struct _XfdashboardViewPrivate
{
	/* Children (list of ClutterActor) of this view */
	GList					*children;

	/* View name */
	gchar					*viewName;

	/* Layout manager */
	ClutterLayoutManager	*layoutManager;
	guint					signalChangedID;
};

/* Properties */
enum
{
	PROP_0,

	PROP_VIEW_NAME,
	PROP_LAYOUT_MANAGER,

	PROP_LAST
};

static GParamSpec* XfdashboardViewProperties[PROP_LAST]={ 0, };

/* Signals */
enum
{
	ACTIVATED,
	DEACTIVATED,
	
	NAME_CHANGED,

	RESET_SCROLLBARS,

	SIGNAL_LAST
};

static guint XfdashboardViewSignals[SIGNAL_LAST]={ 0, };

/* IMPLEMENTATION: Private variables and methods */

/* Sort children by their depth */
static gint _xfdashboard_view_sort_by_depth(gconstpointer inA, gconstpointer inB)
{
	gfloat	depthA=clutter_actor_get_depth((ClutterActor*)inA);
	gfloat	depthB=clutter_actor_get_depth((ClutterActor*)inB);

	if(depthA<depthB) return(-1);
		else if(depthA>depthB) return(1);

	return(0);
}

/* Replace layout manager */
void _xfdashboard_view_set_layout_manager(XfdashboardView *self, ClutterLayoutManager *inManager)
{
	/* Get private structure */
	XfdashboardViewPrivate	*priv=self->priv;

	/* Set new layout manager (if it differs from current one) */
	if(priv->layoutManager==inManager) return;

	if(priv->layoutManager!=NULL)
	{
		if(priv->signalChangedID!=0) g_signal_handler_disconnect(priv->layoutManager, priv->signalChangedID);
		priv->signalChangedID=0;

		clutter_layout_manager_set_container(priv->layoutManager, NULL);
		g_object_unref(priv->layoutManager);
		priv->layoutManager=NULL;
	}

	if(inManager!=NULL)
	{
		priv->layoutManager=g_object_ref_sink(inManager);
		clutter_layout_manager_set_container(inManager, CLUTTER_CONTAINER(self));

		priv->signalChangedID=g_signal_connect_swapped(priv->layoutManager,
														"layout-changed",
														G_CALLBACK(clutter_actor_queue_relayout),
														self);
	}

	clutter_actor_queue_relayout(CLUTTER_ACTOR(self));

	g_object_notify_by_pspec(G_OBJECT(self), XfdashboardViewProperties[PROP_LAYOUT_MANAGER]);
}


/* IMPLEMENTATION: ClutterContainer */

/* Add an actor to container */
static void xfdashboard_view_add(ClutterContainer *self,
									ClutterActor *inActor)
{
	XfdashboardViewPrivate	*priv=XFDASHBOARD_VIEW(self)->priv;
	GList					*list, *prev=NULL;
	gfloat					actorDepth;

	g_object_ref(inActor);

	actorDepth=clutter_actor_get_depth(inActor);

	/* Find the right place to insert actor so that it will still be
	 * sorted and the actor will be after all of the actors at the same
	 * depth */
	for(list=priv->children;
		list && (clutter_actor_get_depth(list->data)<=actorDepth);
		list=list->next)
	{
		prev=list;
	}
	
	/* Insert actor before the found node */
	list=g_list_prepend(list, inActor);
	
	/* Fixup the links */
	if(prev)
	{
		prev->next=list;
		list->prev=prev;
	}
		else priv->children=list;

	clutter_actor_set_parent(inActor, CLUTTER_ACTOR(self));

	clutter_actor_queue_relayout(inActor);

	g_signal_emit_by_name(self, "actor-added", inActor);

	g_object_unref(inActor);
}

/* Remove an actor from container */
static void xfdashboard_view_remove(ClutterContainer *self,
									ClutterActor *inActor)
{
	XfdashboardViewPrivate	*priv=XFDASHBOARD_VIEW(self)->priv;

	g_object_ref(inActor);

	priv->children=g_list_remove(priv->children, inActor);
	clutter_actor_unparent(inActor);

	clutter_actor_queue_relayout(CLUTTER_ACTOR(self));

	g_signal_emit_by_name(self, "actor-removed", inActor);

	g_object_unref(inActor);
}

/* For each child in list of children call callback */
static void xfdashboard_view_foreach(ClutterContainer *self,
										ClutterCallback inCallback,
										gpointer inUserData)
{
	XfdashboardViewPrivate	*priv=XFDASHBOARD_VIEW(self)->priv;

	g_list_foreach(priv->children, (GFunc)inCallback, inUserData);
}

/* Raise a child above another one */
static void xfdashboard_view_raise(ClutterContainer *self,
									ClutterActor *inActor,
									ClutterActor *inSibling)
{
	XfdashboardViewPrivate	*priv=XFDASHBOARD_VIEW(self)->priv;

	priv->children=g_list_remove(priv->children, inActor);

	if(inSibling==NULL) priv->children=g_list_append(priv->children, inActor);
	else
	{
		gint				idx=g_list_index(priv->children, inSibling);

		priv->children=g_list_insert(priv->children, inActor, idx+1);
	}

	clutter_actor_queue_relayout(CLUTTER_ACTOR(self));
}

/* Lower a child above another one */
static void xfdashboard_view_lower(ClutterContainer *self,
									ClutterActor *inActor,
									ClutterActor *inSibling)
{
	XfdashboardViewPrivate	*priv=XFDASHBOARD_VIEW(self)->priv;

	priv->children=g_list_remove(priv->children, inActor);

	if(inSibling==NULL) priv->children=g_list_prepend(priv->children, inActor);
	else
	{
		gint				idx=g_list_index(priv->children, inSibling);

		priv->children=g_list_insert(priv->children, inActor, idx);
	}

	clutter_actor_queue_relayout(CLUTTER_ACTOR(self));
}

static void xfdashboard_view_sort_depth_order(ClutterContainer *self)
{
	XfdashboardViewPrivate	*priv=XFDASHBOARD_VIEW(self)->priv;

	priv->children=g_list_sort(priv->children, _xfdashboard_view_sort_by_depth);

	clutter_actor_queue_relayout(CLUTTER_ACTOR(self));
}

/* Override interface virtual methods */
static void clutter_container_iface_init(ClutterContainerIface *inInterface)
{
	inInterface->add=xfdashboard_view_add;
	inInterface->remove=xfdashboard_view_remove;
	inInterface->foreach=xfdashboard_view_foreach;
	inInterface->raise=xfdashboard_view_raise;
	inInterface->lower=xfdashboard_view_lower;
	inInterface->sort_depth_order=xfdashboard_view_sort_depth_order;
}

/* IMPLEMENTATION: ClutterActor */

/* Paint actor */
static void xfdashboard_view_paint(ClutterActor *inActor)
{
	XfdashboardViewPrivate	*priv=XFDASHBOARD_VIEW(inActor)->priv;

	g_list_foreach(priv->children, (GFunc)clutter_actor_paint, NULL);
}

/* Retrieve paint volume of this actor and its children */
static gboolean xfdashboard_view_get_paint_volume(ClutterActor *inActor,
													ClutterPaintVolume *inVolume)
{
	XfdashboardViewPrivate			*priv=XFDASHBOARD_VIEW(inActor)->priv;
	GList							*list;

	/* If we don't have any children stop here immediately */
	if(priv->children==NULL) return(FALSE);

	/* Otherwise union the paint volumes of all children */
	for(list=priv->children; list!=NULL; list=list->next)
	{
		ClutterActor				*child=list->data;
		const ClutterPaintVolume	*childVolume;

		/* This gets the paint volume of the child transformed into the
		 * group's coordinate space... */
		childVolume=clutter_actor_get_transformed_paint_volume(child, inActor);
		if(!childVolume) return(FALSE);

		clutter_paint_volume_union(inVolume, childVolume);
	}

	return(TRUE);
}

/* Pick all the child actors */
static void xfdashboard_view_pick(ClutterActor *inActor, const ClutterColor *inPick)
{
	XfdashboardViewPrivate	*priv=XFDASHBOARD_VIEW(inActor)->priv;

	CLUTTER_ACTOR_CLASS(xfdashboard_view_parent_class)->pick(inActor, inPick);

	g_list_foreach(priv->children, (GFunc)clutter_actor_paint, NULL);
}

/* Get preferred width/height */
static void xfdashboard_view_get_preferred_width(ClutterActor *inActor,
													gfloat inForHeight,
													gfloat *outMinWidth,
													gfloat *outNaturalWidth)
{
	XfdashboardViewPrivate	*priv=XFDASHBOARD_VIEW(inActor)->priv;

	clutter_layout_manager_get_preferred_width(priv->layoutManager,
												CLUTTER_CONTAINER(inActor),
												inForHeight,
												outMinWidth,
												outNaturalWidth);
}

static void xfdashboard_view_get_preferred_height(ClutterActor *inActor,
													gfloat inForWidth,
													gfloat *outMinHeight,
													gfloat *outNaturalWidth)
{
	XfdashboardViewPrivate	*priv=XFDASHBOARD_VIEW(inActor)->priv;

	clutter_layout_manager_get_preferred_height(priv->layoutManager,
												CLUTTER_CONTAINER(inActor),
												inForWidth,
												outMinHeight,
												outNaturalWidth);
}

/* Allocate position and size of actor and its children*/
static void xfdashboard_view_allocate(ClutterActor *inActor,
										const ClutterActorBox *inAllocation,
										ClutterAllocationFlags inFlags)
{
	XfdashboardViewPrivate	*priv=XFDASHBOARD_VIEW(inActor)->priv;
	ClutterActorClass		*klass;
	ClutterActorBox			box;
	gfloat					w, h;

	klass=CLUTTER_ACTOR_CLASS(xfdashboard_view_parent_class);
	klass->allocate(inActor, inAllocation, inFlags);

	clutter_actor_box_get_size(inAllocation, &w, &h);

	clutter_actor_box_set_origin(&box, 0.0f, 0.0f);
	clutter_actor_box_set_size(&box, w, h);
	clutter_layout_manager_allocate(priv->layoutManager,
									CLUTTER_CONTAINER(inActor),
									&box,
									inFlags);
}
/* Destroy this actor */
static void xfdashboard_view_destroy(ClutterActor *inActor)
{
	XfdashboardViewPrivate		*priv=XFDASHBOARD_VIEW(inActor)->priv;

	/* Destroy all our children */
	g_list_foreach(priv->children, (GFunc)clutter_actor_destroy, NULL);

	/* Call parent's class destroy method */
	if(CLUTTER_ACTOR_CLASS(xfdashboard_view_parent_class)->destroy)
	{
		CLUTTER_ACTOR_CLASS(xfdashboard_view_parent_class)->destroy(inActor);
	}
}

/* IMPLEMENTATION: GObject */

/* Dispose this object */
static void xfdashboard_view_dispose(GObject *inObject)
{
	XfdashboardView			*self=XFDASHBOARD_VIEW(inObject);
	XfdashboardViewPrivate	*priv=self->priv;

	/* Release allocated resources */
	_xfdashboard_view_set_layout_manager(self, NULL);

	if(priv->viewName) g_free(priv->viewName);
	priv->viewName=NULL;

	/* Call parent's class dispose method */
	G_OBJECT_CLASS(xfdashboard_view_parent_class)->dispose(inObject);
}

/* Set/get properties */
static void xfdashboard_view_set_property(GObject *inObject,
												guint inPropID,
												const GValue *inValue,
												GParamSpec *inSpec)
{
	XfdashboardView		*self=XFDASHBOARD_VIEW(inObject);
	
	switch(inPropID)
	{
		case PROP_VIEW_NAME:
			xfdashboard_view_set_view_name(self, g_value_get_string(inValue));
			break;
			
		case PROP_LAYOUT_MANAGER:
			xfdashboard_view_set_layout_manager(self, CLUTTER_LAYOUT_MANAGER(g_value_get_object(inValue)));
			break;

		default:
			G_OBJECT_WARN_INVALID_PROPERTY_ID(inObject, inPropID, inSpec);
			break;
	}
}

static void xfdashboard_view_get_property(GObject *inObject,
												guint inPropID,
												GValue *outValue,
												GParamSpec *inSpec)
{
	XfdashboardView		*self=XFDASHBOARD_VIEW(inObject);

	switch(inPropID)
	{
		case PROP_VIEW_NAME:
			g_value_set_string(outValue, self->priv->viewName);
			break;

		case PROP_LAYOUT_MANAGER:
			g_value_set_object(outValue, self->priv->layoutManager);
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
	ClutterActorClass		*actorClass=CLUTTER_ACTOR_CLASS(klass);

	/* Override functions */
	actorClass->paint=xfdashboard_view_paint;
	actorClass->get_paint_volume=xfdashboard_view_get_paint_volume;
	actorClass->pick=xfdashboard_view_pick;
	actorClass->get_preferred_width=xfdashboard_view_get_preferred_width;
	actorClass->get_preferred_height=xfdashboard_view_get_preferred_height;
	actorClass->allocate=xfdashboard_view_allocate;
	actorClass->destroy=xfdashboard_view_destroy;

	gobjectClass->set_property=xfdashboard_view_set_property;
	gobjectClass->get_property=xfdashboard_view_get_property;
	gobjectClass->dispose=xfdashboard_view_dispose;

	/* Set up private structure */
	g_type_class_add_private(klass, sizeof(XfdashboardViewPrivate));

	/* Define properties */
	XfdashboardViewProperties[PROP_VIEW_NAME]=
		g_param_spec_string("view-name",
							"View name",
							"View name used in button",
							"",
							G_PARAM_READWRITE | G_PARAM_CONSTRUCT);

	XfdashboardViewProperties[PROP_LAYOUT_MANAGER]=
		g_param_spec_object ("layout-manager",
								"Layout Manager",
								"The layout manager used by the box",
								CLUTTER_TYPE_LAYOUT_MANAGER,
								G_PARAM_READWRITE | G_PARAM_CONSTRUCT);

	g_object_class_install_properties(gobjectClass, PROP_LAST, XfdashboardViewProperties);

	/* Define signals */
	XfdashboardViewSignals[ACTIVATED]=
		g_signal_new("activated",
						G_TYPE_FROM_CLASS(klass),
						G_SIGNAL_RUN_LAST,
						G_STRUCT_OFFSET(XfdashboardViewClass, activated),
						NULL,
						NULL,
						g_cclosure_marshal_VOID__VOID,
						G_TYPE_NONE,
						0);

	XfdashboardViewSignals[DEACTIVATED]=
		g_signal_new("deactivated",
						G_TYPE_FROM_CLASS(klass),
						G_SIGNAL_RUN_LAST,
						G_STRUCT_OFFSET(XfdashboardViewClass, deactivated),
						NULL,
						NULL,
						g_cclosure_marshal_VOID__VOID,
						G_TYPE_NONE,
						0);

	XfdashboardViewSignals[NAME_CHANGED]=
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

	XfdashboardViewSignals[RESET_SCROLLBARS]=
		g_signal_new("reset-scrollbars",
						G_TYPE_FROM_CLASS(klass),
						G_SIGNAL_RUN_LAST,
						G_STRUCT_OFFSET(XfdashboardViewClass, reset_scrollbars),
						NULL,
						NULL,
						g_cclosure_marshal_VOID__VOID,
						G_TYPE_NONE,
						0);
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
	priv->layoutManager=NULL;
}

/* Implementation: Public API */

/* Remove all children from view */
void xfdashboard_view_remove_all(XfdashboardView *self)
{
	g_return_if_fail(XFDASHBOARD_IS_VIEW(self));

	/* Destroy all our children */
	g_list_foreach(self->priv->children, (GFunc)clutter_actor_destroy, NULL);
	g_list_free(self->priv->children);
	self->priv->children=NULL;
}

/* Get/set font to use in label */
const gchar* xfdashboard_view_get_view_name(XfdashboardView *self)
{
	g_return_val_if_fail(XFDASHBOARD_IS_VIEW(self), NULL);

	return(self->priv->viewName);
}

void xfdashboard_view_set_view_name(XfdashboardView *self, const gchar *inName)
{
	g_return_if_fail(XFDASHBOARD_IS_VIEW(self));
	g_return_if_fail(inName!=NULL);

	/* Set font of label */
	XfdashboardViewPrivate	*priv=XFDASHBOARD_VIEW_GET_PRIVATE(self);

	if(g_strcmp0(priv->viewName, inName)!=0)
	{
		if(priv->viewName) g_free(priv->viewName);
		priv->viewName=g_strdup(inName);

		g_signal_emit_by_name(self, "name-changed", priv->viewName);
	}
}

/* Get/Set layout manager */
ClutterLayoutManager* xfdashboard_view_get_layout_manager(XfdashboardView *self)
{
  g_return_val_if_fail(XFDASHBOARD_IS_VIEW(self), NULL);

  return(self->priv->layoutManager);
}

void xfdashboard_view_set_layout_manager(XfdashboardView *self, ClutterLayoutManager *inManager)
{
	g_return_if_fail(XFDASHBOARD_IS_VIEW(self));
	g_return_if_fail(CLUTTER_IS_LAYOUT_MANAGER(inManager));

	_xfdashboard_view_set_layout_manager(self, inManager);
}

/* Reset scroll bars in viewpad this view is connected to */
void xfdashboard_view_reset_scrollbars(XfdashboardView *self)
{
	g_return_if_fail(XFDASHBOARD_IS_VIEW(self));

	/* Check if this view is connected to a viewpad */
	ClutterActor	*parent=clutter_actor_get_parent(CLUTTER_ACTOR(self));
	
	if(!parent) return;
	
	/* Reset scroll bars */
	g_return_if_fail(XFDASHBOARD_IS_VIEWPAD(parent));
	g_signal_emit_by_name(self, "reset-scrollbars", NULL);
}
