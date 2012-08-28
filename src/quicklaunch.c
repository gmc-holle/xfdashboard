/*
 * quicklaunch.c: Quicklaunch box
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

#include "quicklaunch.h"
#include "quicklaunch-icon.h"

/* Define this class in GObject system */
static void clutter_container_iface_init(ClutterContainerIface *iface);

G_DEFINE_TYPE_WITH_CODE(XfdashboardQuicklaunch,
						xfdashboard_quicklaunch,
						CLUTTER_TYPE_ACTOR,
						G_IMPLEMENT_INTERFACE(CLUTTER_TYPE_CONTAINER, clutter_container_iface_init))

/* Private structure - access only by public API if needed */
#define XFDASHBOARD_QUICKLAUNCH_GET_PRIVATE(obj) \
	(G_TYPE_INSTANCE_GET_PRIVATE((obj), XFDASHBOARD_TYPE_QUICKLAUNCH, XfdashboardQuicklaunchPrivate))

struct _XfdashboardQuicklaunchPrivate
{
	/* Children (list of XfdashboardQuicklaunchIcon) of this box */
	GList					*children;

	/* Layout manager */
	ClutterLayoutManager	*layoutManager;
	guint					signalChangedID;
};

/* Properties */
enum
{
	PROP_0,

	PROP_LAYOUT_MANAGER,
	
	PROP_LAST
};

static GParamSpec* XfdashboardQuicklaunchProperties[PROP_LAST]={ 0, };

/* IMPLEMENTATION: Private variables and methods */

static guint	quicklaunch_max_size=64;
static gchar	*quicklaunch_apps[]=	{
											"firefox.desktop",
											"evolution.desktop",
											"Terminal.desktop",
											"Thunar.desktop"
										};

/* Draw background into Cairo texture */
static gboolean _xfdashboard_quicklaunch_draw_background(ClutterCairoTexture *inActor,
															cairo_t *inCairo)
{
	guint		width, height;

	/* Clear the contents of actor to avoid painting
	 * over the previous contents
	 */
	clutter_cairo_texture_clear(inActor);

	/* Scale to the size of actor */
	clutter_cairo_texture_get_surface_size(inActor, &width, &height);

	cairo_set_line_cap(inCairo, CAIRO_LINE_CAP_ROUND);
	cairo_set_line_width(inCairo, 1);

	/* Draw surrounding rectangle and fill it */
	clutter_cairo_set_source_color(inCairo, CLUTTER_COLOR_White);
	cairo_move_to(inCairo, 0, 0);
	cairo_rel_line_to(inCairo, width, 0);
	cairo_rel_line_to(inCairo, 0, height);
	cairo_rel_line_to(inCairo, -width, 0);
	cairo_close_path(inCairo);
	cairo_stroke(inCairo);

	return(TRUE);
}

/* Sort children by their depth */
static gint _xfdashboard_quicklaunch_sort_by_depth(gconstpointer inA, gconstpointer inB)
{
	gfloat	depthA=clutter_actor_get_depth((ClutterActor*)inA);
	gfloat	depthB=clutter_actor_get_depth((ClutterActor*)inB);

	if(depthA<depthB) return(-1);
		else if(depthA>depthB) return(1);

	return(0);
}

/* Replace layout manager */
void _xfdashboard_quicklaunch_set_layout_manager(XfdashboardQuicklaunch *self, ClutterLayoutManager *inManager)
{
	/* Get private structure */
	XfdashboardQuicklaunchPrivate	*priv=self->priv;

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

	g_object_notify_by_pspec(G_OBJECT(self), XfdashboardQuicklaunchProperties[PROP_LAYOUT_MANAGER]);
}


/* IMPLEMENTATION: ClutterContainer */

/* Add an actor to container */
static void xfdashboard_quicklaunch_add(ClutterContainer *self,
									ClutterActor *inActor)
{
	XfdashboardQuicklaunchPrivate	*priv=XFDASHBOARD_QUICKLAUNCH(self)->priv;
	GList							*list, *prev=NULL;
	gfloat							actorDepth;

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
static void xfdashboard_quicklaunch_remove(ClutterContainer *self,
									ClutterActor *inActor)
{
	XfdashboardQuicklaunchPrivate	*priv=XFDASHBOARD_QUICKLAUNCH(self)->priv;

	g_object_ref(inActor);

	priv->children=g_list_remove(priv->children, inActor);
	clutter_actor_unparent(inActor);

	clutter_actor_queue_relayout(CLUTTER_ACTOR(self));

	g_signal_emit_by_name(self, "actor-removed", inActor);

	g_object_unref(inActor);
}

/* For each child in list of children call callback */
static void xfdashboard_quicklaunch_foreach(ClutterContainer *self,
										ClutterCallback inCallback,
										gpointer inUserData)
{
	XfdashboardQuicklaunchPrivate	*priv=XFDASHBOARD_QUICKLAUNCH(self)->priv;

	g_list_foreach(priv->children, (GFunc)inCallback, inUserData);
}

/* Raise a child above another one */
static void xfdashboard_quicklaunch_raise(ClutterContainer *self,
									ClutterActor *inActor,
									ClutterActor *inSibling)
{
	XfdashboardQuicklaunchPrivate	*priv=XFDASHBOARD_QUICKLAUNCH(self)->priv;

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
static void xfdashboard_quicklaunch_lower(ClutterContainer *self,
									ClutterActor *inActor,
									ClutterActor *inSibling)
{
	XfdashboardQuicklaunchPrivate	*priv=XFDASHBOARD_QUICKLAUNCH(self)->priv;

	priv->children=g_list_remove(priv->children, inActor);

	if(inSibling==NULL) priv->children=g_list_prepend(priv->children, inActor);
	else
	{
		gint				idx=g_list_index(priv->children, inSibling);

		priv->children=g_list_insert(priv->children, inActor, idx);
	}

	clutter_actor_queue_relayout(CLUTTER_ACTOR(self));
}

static void xfdashboard_quicklaunch_sort_depth_order(ClutterContainer *self)
{
	XfdashboardQuicklaunchPrivate	*priv=XFDASHBOARD_QUICKLAUNCH(self)->priv;

	priv->children=g_list_sort(priv->children, _xfdashboard_quicklaunch_sort_by_depth);

	clutter_actor_queue_relayout(CLUTTER_ACTOR(self));
}

/* Override interface virtual methods */
static void clutter_container_iface_init(ClutterContainerIface *inInterface)
{
	inInterface->add=xfdashboard_quicklaunch_add;
	inInterface->remove=xfdashboard_quicklaunch_remove;
	inInterface->foreach=xfdashboard_quicklaunch_foreach;
	inInterface->raise=xfdashboard_quicklaunch_raise;
	inInterface->lower=xfdashboard_quicklaunch_lower;
	inInterface->sort_depth_order=xfdashboard_quicklaunch_sort_depth_order;
}

/* IMPLEMENTATION: ClutterActor */

/* Paint actor */
static void xfdashboard_quicklaunch_paint(ClutterActor *inActor)
{
	XfdashboardQuicklaunchPrivate	*priv=XFDASHBOARD_QUICKLAUNCH(inActor)->priv;

	g_list_foreach(priv->children, (GFunc)clutter_actor_paint, NULL);
}

/* Retrieve paint volume of this actor and its children */
static gboolean xfdashboard_quicklaunch_get_paint_volume(ClutterActor *inActor,
													ClutterPaintVolume *inVolume)
{
	XfdashboardQuicklaunchPrivate	*priv=XFDASHBOARD_QUICKLAUNCH(inActor)->priv;
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
static void xfdashboard_quicklaunch_pick(ClutterActor *inActor, const ClutterColor *inPick)
{
	XfdashboardQuicklaunchPrivate	*priv=XFDASHBOARD_QUICKLAUNCH(inActor)->priv;

	CLUTTER_ACTOR_CLASS(xfdashboard_quicklaunch_parent_class)->pick(inActor, inPick);

	g_list_foreach(priv->children, (GFunc)clutter_actor_paint, NULL);
}

/* Get preferred width/height */
static void xfdashboard_quicklaunch_get_preferred_width(ClutterActor *inActor,
														gfloat inForHeight,
														gfloat *outMinWidth,
														gfloat *outNaturalWidth)
{
	XfdashboardQuicklaunchPrivate	*priv=XFDASHBOARD_QUICKLAUNCH(inActor)->priv;

	clutter_layout_manager_get_preferred_width(priv->layoutManager,
												CLUTTER_CONTAINER(inActor),
												inForHeight,
												outMinWidth,
												outNaturalWidth);
}

static void xfdashboard_quicklaunch_get_preferred_height(ClutterActor *inActor,
															gfloat inForWidth,
															gfloat *outMinHeight,
															gfloat *outNaturalWidth)
{
	XfdashboardQuicklaunchPrivate	*priv=XFDASHBOARD_QUICKLAUNCH(inActor)->priv;

	clutter_layout_manager_get_preferred_height(priv->layoutManager,
												CLUTTER_CONTAINER(inActor),
												inForWidth,
												outMinHeight,
												outNaturalWidth);
}

/* Allocate position and size of actor and its children*/
static void xfdashboard_quicklaunch_allocate(ClutterActor *inActor,
												const ClutterActorBox *inAllocation,
												ClutterAllocationFlags inFlags)
{
	XfdashboardQuicklaunchPrivate	*priv=XFDASHBOARD_QUICKLAUNCH(inActor)->priv;
	ClutterActorClass		*klass;
	ClutterActorBox			box;
	gfloat					w, h;

	klass=CLUTTER_ACTOR_CLASS(xfdashboard_quicklaunch_parent_class);
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
static void xfdashboard_quicklaunch_destroy(ClutterActor *inActor)
{
	XfdashboardQuicklaunchPrivate		*priv=XFDASHBOARD_QUICKLAUNCH(inActor)->priv;

	/* Destroy all our children */
	g_list_foreach(priv->children, (GFunc)clutter_actor_destroy, NULL);

	/* Call parent's class destroy method */
	if(CLUTTER_ACTOR_CLASS(xfdashboard_quicklaunch_parent_class)->destroy)
	{
		CLUTTER_ACTOR_CLASS(xfdashboard_quicklaunch_parent_class)->destroy(inActor);
	}
}

/* IMPLEMENTATION: GObject */

/* Dispose this object */
static void xfdashboard_quicklaunch_dispose(GObject *inObject)
{
  XfdashboardQuicklaunch		*self=XFDASHBOARD_QUICKLAUNCH(inObject);

  _xfdashboard_quicklaunch_set_layout_manager(self, NULL);

  G_OBJECT_CLASS(xfdashboard_quicklaunch_parent_class)->dispose(inObject);
}

/* Set/get properties */
static void xfdashboard_quicklaunch_set_property(GObject *inObject,
													guint inPropID,
													const GValue *inValue,
													GParamSpec *inSpec)
{
	XfdashboardQuicklaunch		*self=XFDASHBOARD_QUICKLAUNCH(inObject);
	
	switch(inPropID)
	{
		case PROP_LAYOUT_MANAGER:
			_xfdashboard_quicklaunch_set_layout_manager(self, g_value_get_object(inValue));
			break;

		default:
			G_OBJECT_WARN_INVALID_PROPERTY_ID(inObject, inPropID, inSpec);
			break;
	}
}

static void xfdashboard_quicklaunch_get_property(GObject *inObject,
													guint inPropID,
													GValue *outValue,
													GParamSpec *inSpec)
{
	XfdashboardQuicklaunch		*self=XFDASHBOARD_QUICKLAUNCH(inObject);

	switch(inPropID)
	{
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
static void xfdashboard_quicklaunch_class_init(XfdashboardQuicklaunchClass *klass)
{
	GObjectClass			*gobjectClass=G_OBJECT_CLASS(klass);
	ClutterActorClass		*actorClass=CLUTTER_ACTOR_CLASS(klass);

	/* Override functions */
	actorClass->paint=xfdashboard_quicklaunch_paint;
	actorClass->get_paint_volume=xfdashboard_quicklaunch_get_paint_volume;
	actorClass->pick=xfdashboard_quicklaunch_pick;
	actorClass->get_preferred_width=xfdashboard_quicklaunch_get_preferred_width;
	actorClass->get_preferred_height=xfdashboard_quicklaunch_get_preferred_height;
	actorClass->allocate=xfdashboard_quicklaunch_allocate;
	actorClass->destroy=xfdashboard_quicklaunch_destroy;

	gobjectClass->set_property=xfdashboard_quicklaunch_set_property;
	gobjectClass->get_property=xfdashboard_quicklaunch_get_property;
	gobjectClass->dispose=xfdashboard_quicklaunch_dispose;

	/* Set up private structure */
	g_type_class_add_private(klass, sizeof(XfdashboardQuicklaunchPrivate));

	/* Define properties */
	XfdashboardQuicklaunchProperties[PROP_LAYOUT_MANAGER]=
		g_param_spec_object ("layout-manager",
								"Layout Manager",
								"The layout manager used by the box",
								CLUTTER_TYPE_LAYOUT_MANAGER,
								G_PARAM_READWRITE | G_PARAM_CONSTRUCT);

	g_object_class_install_properties(gobjectClass, PROP_LAST, XfdashboardQuicklaunchProperties);
}

/* Object initialization
 * Create private structure and set up default values
 */
static void xfdashboard_quicklaunch_init(XfdashboardQuicklaunch *self)
{
	XfdashboardQuicklaunchPrivate	*priv;
	ClutterActor					*actor;
	gint							i;
	
	priv=self->priv=XFDASHBOARD_QUICKLAUNCH_GET_PRIVATE(self);

	/* Set up background actor and bind its size to this box */
	actor=clutter_cairo_texture_new(quicklaunch_max_size, 1);
	clutter_actor_add_constraint(actor, clutter_bind_constraint_new(CLUTTER_ACTOR(self), CLUTTER_BIND_POSITION, 0));
	clutter_actor_add_constraint(actor, clutter_bind_constraint_new(CLUTTER_ACTOR(self), CLUTTER_BIND_SIZE, 0));
	clutter_cairo_texture_set_auto_resize(CLUTTER_CAIRO_TEXTURE(actor), TRUE);
	g_signal_connect(actor, "draw", G_CALLBACK(_xfdashboard_quicklaunch_draw_background), NULL);
	clutter_container_add_actor(CLUTTER_CONTAINER(self), actor);

	/* TODO: Remove the following actor(s) for application icons
	 *       in quicklaunch box as soon as xfconf is implemented
	 */
	 for(i=0; i<(sizeof(quicklaunch_apps)/sizeof(quicklaunch_apps[0])); i++)
	 {
		 actor=xfdashboard_quicklaunch_icon_new_full(quicklaunch_apps[i]);
		 clutter_container_add_actor(CLUTTER_CONTAINER(self), actor);
	 }
}

/* Implementation: Public API */

/* Create new actor */
ClutterActor* xfdashboard_quicklaunch_new(void)
{
	ClutterLayoutManager	*layout;
	
	/* Create layout manager used in this view */
	layout=clutter_box_layout_new();
	clutter_box_layout_set_vertical(CLUTTER_BOX_LAYOUT(layout), TRUE);
	
	return(g_object_new(XFDASHBOARD_TYPE_QUICKLAUNCH,
						"layout-manager", layout,
						NULL));
}

/* Remove all children from view */
void xfdashboard_quicklaunch_remove_all(XfdashboardQuicklaunch *self)
{
	g_return_if_fail(XFDASHBOARD_IS_QUICKLAUNCH(self));

	/* Destroy all our children */
	g_list_foreach(self->priv->children, (GFunc)clutter_actor_destroy, NULL);
	g_list_free(self->priv->children);
	self->priv->children=NULL;
}

/* Get/Set layout manager */
void xfdashboard_quicklaunch_set_layout_manager(XfdashboardQuicklaunch *self, ClutterLayoutManager *inManager)
{
	g_return_if_fail(XFDASHBOARD_IS_QUICKLAUNCH(self));
	g_return_if_fail(CLUTTER_IS_LAYOUT_MANAGER(inManager));

	_xfdashboard_quicklaunch_set_layout_manager(self, inManager);
}

ClutterLayoutManager* xfdashboard_quicklaunch_get_layout_manager(XfdashboardQuicklaunch *self)
{
  g_return_val_if_fail(XFDASHBOARD_IS_QUICKLAUNCH(self), NULL);

  return(self->priv->layoutManager);
}
