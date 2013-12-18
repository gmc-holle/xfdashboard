/*
 * drag-action: Drag action for actors
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

#include "drag-action.h"
#include "drop-action.h"
#include "marshal.h"

#include <glib/gi18n-lib.h>

/* Define this class in GObject system */
G_DEFINE_TYPE(XfdashboardDragAction,
				xfdashboard_drag_action,
				CLUTTER_TYPE_DRAG_ACTION)

/* Private structure - access only by public API if needed */
#define XFDASHBOARD_DRAG_ACTION_GET_PRIVATE(obj) \
	(G_TYPE_INSTANCE_GET_PRIVATE((obj), XFDASHBOARD_TYPE_DRAG_ACTION, XfdashboardDragActionPrivate))

struct _XfdashboardDragActionPrivate
{
	/* Properties related */
	ClutterActor			*source;
	ClutterActor			*actor;

	/* Instance related */
	GSList					*targets;
	XfdashboardDropAction	*lastDropTarget;
	gfloat					lastDeltaX, lastDeltaY;
};

/* Properties */
enum
{
	PROP_0,

	PROP_SOURCE,

	PROP_LAST
};

GParamSpec* XfdashboardDragActionProperties[PROP_LAST]={ 0, };

/* Signals */
enum
{
	SIGNAL_DRAG_CANCEL,

	SIGNAL_LAST
};

guint XfdashboardDragActionSignals[SIGNAL_LAST]={ 0, };

/* IMPLEMENTATION: Private variables and methods */

/* Sort drop action targets */
static gint _xfdashboard_drag_action_sort_targets_callback(gconstpointer inLeft, gconstpointer inRight)
{
	ClutterActor		*actor1, *actor2;
	gfloat				depth1, depth2;
	gfloat				x1, y1, w1, h1;
	gfloat				x2, y2, w2, h2;
	ClutterActorBox		*box1, *box2;
	gint				numberPoint1, numberPoint2;

	g_return_val_if_fail(XFDASHBOARD_IS_DROP_ACTION(inLeft) && XFDASHBOARD_IS_DROP_ACTION(inRight), 0);

	actor1=clutter_actor_meta_get_actor(CLUTTER_ACTOR_META(inLeft));
	actor2=clutter_actor_meta_get_actor(CLUTTER_ACTOR_META(inRight));

	/* Return -1 if actor in inLeft should be inserted before actor in inRight
	 * and return 1 if otherwise. If both actors can be handled equal then
	 * return 0. But how to decide?
	 * The actor with higher z-depth should be inserted before. If both actors
	 * have equal z-depth then the actor with the most edge points within the
	 * other actor (overlap) should be inserted before. Edge points are:
	 * [left,top], [right,top], [left,bottom] and [right, bottom].
	*/
	depth1=clutter_actor_get_z_position(actor1);
	depth2=clutter_actor_get_z_position(actor2);
	if(depth1>depth2) return(-1);
		else if(depth1<depth2) return(1);

	clutter_actor_get_transformed_position(actor1, &x1, &y1);
	clutter_actor_get_transformed_size(actor1, &w1, &h1);
	box1=clutter_actor_box_new(x1, y1, x1+w1, y1+h1);

	clutter_actor_get_transformed_position(actor2, &x2, &y2);
	clutter_actor_get_transformed_size(actor2, &w2, &h2);
	box2=clutter_actor_box_new(x2, y2, x2+w2, y2+h2);

	numberPoint1 =(clutter_actor_box_contains(box1, x2, y2) ? 1 : 0);
	numberPoint1+=(clutter_actor_box_contains(box1, x2+w2, y2) ? 1 : 0);
	numberPoint1+=(clutter_actor_box_contains(box1, x2, y2+h2) ? 1 : 0);
	numberPoint1+=(clutter_actor_box_contains(box1, x2+w2, y2+h2) ? 1 : 0);

	numberPoint2 =(clutter_actor_box_contains(box2, x1, y1) ? 1 : 0);
	numberPoint2+=(clutter_actor_box_contains(box2, x1+w1, y1) ? 1 : 0);
	numberPoint2+=(clutter_actor_box_contains(box2, x1, y1+h1) ? 1 : 0);
	numberPoint2+=(clutter_actor_box_contains(box2, x1+w1, y1+h1) ? 1 : 0);

	clutter_actor_box_free(box1);
	clutter_actor_box_free(box2);

	/* Return result */
	if(numberPoint1>numberPoint2) return(1);
		else if(numberPoint2>numberPoint1) return(-1);
	return(0);
}

static void _xfdashboard_drag_action_sort_targets(XfdashboardDragAction *self)
{
	XfdashboardDragActionPrivate	*priv;

	g_return_if_fail(XFDASHBOARD_IS_DRAG_ACTION(self));

	priv=self->priv;

	/* Sort targets */
	priv->targets=g_slist_sort(priv->targets, _xfdashboard_drag_action_sort_targets_callback);
}

/* Transform stage coordinates to drop action's target actor coordinates */
static void _xfdashboard_drag_action_transform_stage_point(XfdashboardDropAction *inDropTarget,
															gfloat inStageX,
															gfloat inStageY,
															gfloat *outActorX,
															gfloat *outActorY)
{
	ClutterActor	*actor;
	gfloat			x, y;

	g_return_if_fail(XFDASHBOARD_IS_DROP_ACTION(inDropTarget));

	/* Get target actor of drop action and transform coordinates */
	actor=clutter_actor_meta_get_actor(CLUTTER_ACTOR_META(inDropTarget));
	clutter_actor_transform_stage_point(actor, inStageX, inStageY, &x, &y);

	/* Set return values */
	if(outActorX) *outActorX=x;
	if(outActorY) *outActorY=y;
}

/* Find drop target at position */
static XfdashboardDropAction* _xfdashboard_drag_action_find_drop_traget_at_coord(XfdashboardDragAction *self,
																					gfloat inStageX,
																					gfloat inStageY)
{
	XfdashboardDragActionPrivate	*priv;
	GSList							*list;

	g_return_val_if_fail(XFDASHBOARD_IS_DRAG_ACTION(self), NULL);

	priv=self->priv;

	/* Iterate through list and return first drop target in list
	 * where coordinates fit in
	 */
	for(list=priv->targets; list; list=list->next)
	{
		ClutterActorMeta			*actorMeta=CLUTTER_ACTOR_META(list->data);
		ClutterActor				*actor=clutter_actor_meta_get_actor(actorMeta);
		gfloat						x, y, w, h;

		/* Get position and size of actor in stage coordinates */
		clutter_actor_get_transformed_position(actor, &x, &y);
		clutter_actor_get_transformed_size(actor, &w, &h);

		/* If given stage coordinates fit in actor we found it */
		if(inStageX>=x && inStageX<(x+w) &&
			inStageY>=y && inStageY<(y+h))
		{
			return(XFDASHBOARD_DROP_ACTION(actorMeta));
		}
	}

	/* If we get here no drop target was found */
	return(NULL);
}


/* Allocation of a drop target changed */
static void _xfdashboard_drag_on_drop_target_allocation_changed(XfdashboardDragAction *self,
																ClutterActorBox *inBox,
																ClutterAllocationFlags inFlags,
																gpointer inUserData)
{
	g_return_if_fail(XFDASHBOARD_IS_DRAG_ACTION(self));

	/* Resort list as overlapping of actors might have changed */
	_xfdashboard_drag_action_sort_targets(self);
}

/* Set source actor */
static void _xfdashboard_drag_action_set_source(XfdashboardDragAction *self, ClutterActor *inSource)
{
	XfdashboardDragActionPrivate	*priv;

	g_return_if_fail(XFDASHBOARD_IS_DRAG_ACTION(self));
	g_return_if_fail(inSource==NULL || CLUTTER_IS_ACTOR(inSource));

	priv=self->priv;

	/* Release old source actor */
	if(priv->source)
	{
		g_object_unref(priv->source);
		priv->source=NULL;
	}

	/* Set new source actor */
	if(inSource)
	{
		priv->source=CLUTTER_ACTOR(g_object_ref(inSource));
	}
}

/* IMPLEMENTATION: ClutterDragAction */

/* Dragging of actor begins */
static void _xfdashboard_drag_action_drag_begin(ClutterDragAction *inAction,
												ClutterActor *inActor,
												gfloat inStageX,
												gfloat inStageY,
												ClutterModifierType inModifiers)
{
	XfdashboardDragAction				*self;
	XfdashboardDragActionPrivate		*priv;
	ClutterDragActionClass				*dragActionClass;
	GSList								*list;

	g_return_if_fail(XFDASHBOARD_IS_DRAG_ACTION(inAction));

	self=XFDASHBOARD_DRAG_ACTION(inAction);
	priv=self->priv;
	dragActionClass=CLUTTER_DRAG_ACTION_CLASS(xfdashboard_drag_action_parent_class);

	/* Call parent's class method */
	if(dragActionClass->drag_begin) dragActionClass->drag_begin(inAction, inActor, inStageX, inStageY, inModifiers);

	/* Remember dragged actor while dragging */
	priv->actor=inActor;

	/* Get list of drop targets. It is a new list with all current
	 * drop targets already reffed. So the drop targets will be valid
	 * while dragging
	 */
	priv->targets=xfdashboard_drop_action_get_targets();

	/* Emit "begin" signal on all drop targets to determine if they
	 * can handle dragged actor and to prepare them for dragging.
	 * All targets returning TRUE (and therefore telling us they
	 * can handle dragged actor and are prepared for drag'n'drop)
	 * will be sorted.
	 */
	list=priv->targets;
	while(list)
	{
		XfdashboardDropAction			*dropTarget=XFDASHBOARD_DROP_ACTION(list->data);
		gboolean						canHandle=FALSE;

		g_signal_emit_by_name(dropTarget, "begin", self, &canHandle);
		if(!canHandle)
		{
			GSList						*next;

			/* Drop target cannot handle dragged actor so unref it and
			 * remove it from list of drop targets
			 */
			next=g_slist_next(list);
			priv->targets=g_slist_remove_link(priv->targets, list);
			g_object_unref(list->data);
			g_slist_free_1(list);
			list=next;
		}
			else list=g_slist_next(list);
	}
	_xfdashboard_drag_action_sort_targets(self);

	/* We should listen to allocation changes for each actor which
	 * is an active drop target.
	 */
	for(list=priv->targets; list; list=g_slist_next(list))
	{
		XfdashboardDropAction			*dropTarget=XFDASHBOARD_DROP_ACTION(list->data);
		ClutterActorMeta				*actorMeta=CLUTTER_ACTOR_META(dropTarget);
		ClutterActor					*actor=clutter_actor_meta_get_actor(actorMeta);

		g_signal_connect_swapped(actor, "allocation-changed", G_CALLBACK(_xfdashboard_drag_on_drop_target_allocation_changed), self);
	}

	/* Setup for dragging */
	priv->lastDropTarget=NULL;
}

/* Dragged actor moved */
static void _xfdashboard_drag_action_drag_motion(ClutterDragAction *inAction,
													ClutterActor *inActor,
													gfloat inDeltaX,
													gfloat inDeltaY)
{
	XfdashboardDragAction				*self;
	XfdashboardDragActionPrivate		*priv;
	ClutterDragActionClass				*dragActionClass;
	gfloat								stageX, stageY;
	XfdashboardDropAction				*dropTarget;
	gfloat								dropX, dropY;

	g_return_if_fail(XFDASHBOARD_IS_DRAG_ACTION(inAction));

	self=XFDASHBOARD_DRAG_ACTION(inAction);
	priv=self->priv;
	dragActionClass=CLUTTER_DRAG_ACTION_CLASS(xfdashboard_drag_action_parent_class);

	/* Call parent's class method */
	if(dragActionClass->drag_motion) dragActionClass->drag_motion(inAction, inActor, inDeltaX, inDeltaY);

	/* Remember motion delta coordinates */
	priv->lastDeltaX=inDeltaX;
	priv->lastDeltaY=inDeltaY;

	/* Get event coordinates relative to stage */
	clutter_drag_action_get_motion_coords(inAction, &stageX, &stageY);

	/* Find drop target at stage coordinate */
	dropTarget=_xfdashboard_drag_action_find_drop_traget_at_coord(self, stageX, stageY);

	/* If found drop target is not the same as the last one emit "drag-leave"
	 * signal at last drop target and "drag-enter" in new drop target
	 */
	if(priv->lastDropTarget!=dropTarget)
	{
		/* Emit "drag-leave" signal on last drop target */
		if(priv->lastDropTarget)
		{
			g_signal_emit_by_name(priv->lastDropTarget, "drag-leave", self, NULL);
			priv->lastDropTarget=NULL;
		}

		/* Check if new drop target is active and emit "drag-enter" signal */
		if(dropTarget)
		{
			ClutterActorMeta			*actorMeta=CLUTTER_ACTOR_META(dropTarget);
			ClutterActor				*dropActor=clutter_actor_meta_get_actor(actorMeta);

			if(clutter_actor_meta_get_enabled(actorMeta) &&
				CLUTTER_ACTOR_IS_VISIBLE(dropActor) &&
				CLUTTER_ACTOR_IS_REACTIVE(dropActor))
			{
				g_signal_emit_by_name(dropTarget, "drag-enter", self, NULL);
				priv->lastDropTarget=dropTarget;
			}
		}
	}

	/* Transform event coordinates relative to last drop target which
	 * should be the drop target under pointer device if it is active
	 * and emit "drag-motion" signal
	 */
	if(priv->lastDropTarget)
	{
		dropX=dropY=0.0f;
		_xfdashboard_drag_action_transform_stage_point(priv->lastDropTarget,
														stageX, stageY,
														&dropX, &dropY);
		g_signal_emit_by_name(priv->lastDropTarget, "drag-motion", self, dropX, dropY, NULL);
	}
}

/* Dragging of actor ended */
static void _xfdashboard_drag_action_drag_end(ClutterDragAction *inAction,
												ClutterActor *inActor,
												gfloat inStageX,
												gfloat inStageY,
												ClutterModifierType inModifiers)
{
	XfdashboardDragAction				*self;
	XfdashboardDragActionPrivate		*priv;
	ClutterDragActionClass				*dragActionClass;
	GSList								*list;
	XfdashboardDropAction				*dropTarget;
	gfloat								dropX, dropY;
	gboolean							canDrop;

	g_return_if_fail(XFDASHBOARD_IS_DRAG_ACTION(inAction));

	self=XFDASHBOARD_DRAG_ACTION(inAction);
	priv=self->priv;
	dragActionClass=CLUTTER_DRAG_ACTION_CLASS(xfdashboard_drag_action_parent_class);
	canDrop=FALSE;

	/* Remove our listerners for allocation changes */
	for(list=priv->targets; list; list=g_slist_next(list))
	{
		XfdashboardDropAction			*target=XFDASHBOARD_DROP_ACTION(list->data);
		ClutterActorMeta				*actorMeta=CLUTTER_ACTOR_META(target);
		ClutterActor					*actor=clutter_actor_meta_get_actor(actorMeta);

		g_signal_handlers_disconnect_by_func(actor, G_CALLBACK(_xfdashboard_drag_on_drop_target_allocation_changed), self);
	}

	/* Find drop target at stage coordinate */
	dropTarget=_xfdashboard_drag_action_find_drop_traget_at_coord(self, inStageX, inStageY);

	/* If drop target was found check if we are allowed to drop on it. */
	if(dropTarget)
	{
		/* Transform event coordinates relative to drop target */
		_xfdashboard_drag_action_transform_stage_point(dropTarget,
														inStageX, inStageY,
														&dropX, &dropY);

		/* Ask drop target if we are allowed to drop dragged actor on it */
		g_signal_emit_by_name(dropTarget, "can-drop", self, dropX, dropY, &canDrop);
	}

	/* If we cannot drop the draggged actor emit "drag-cancel" on dragged actor */
	if(!canDrop) g_signal_emit_by_name(inAction, "drag-cancel", inActor, inStageX, inStageY, NULL);

	/* Iterate through list of drop targets to emit the "end" signal to the ones
	* on which the dragged actor will not be drop (either they were not the target
	* or it did not allow to drop on it). The real drop target gets the "drop"
	* signal.
	*/
	for(list=priv->targets; list; list=g_slist_next(list))
	{
		XfdashboardDropAction			*target=XFDASHBOARD_DROP_ACTION(list->data);

		if(canDrop && target && target==dropTarget)
		{
			g_signal_emit_by_name(dropTarget, "drop", self, dropX, dropY, NULL);
		}
			else
			{
				g_signal_emit_by_name(target, "end", self, NULL);
			}
	}

	/* Call parent's class method at last */
	if(dragActionClass->drag_end) dragActionClass->drag_end(inAction, inActor, inStageX, inStageY, inModifiers);

	/* Forget dragged actor as dragging has ended now */
	priv->actor=NULL;

	/* Free list of drop targets and unref each drop target */
	if(priv->targets) g_slist_free_full(priv->targets, g_object_unref);
	priv->targets=NULL;

	/* Reset variables */
	priv->lastDropTarget=NULL;
}

/* IMPLEMENTATION: GObject */

/* Dispose this object */
static void _xfdashboard_drag_action_dispose(GObject *inObject)
{
	XfdashboardDragAction				*self=XFDASHBOARD_DRAG_ACTION(inObject);
	XfdashboardDragActionPrivate		*priv=self->priv;

	/* Release allocated resources */
	if(priv->source)
	{
		g_object_unref(priv->source);
		priv->source=NULL;
	}

	if(priv->targets)
	{
		g_slist_free_full(priv->targets, g_object_unref);
		priv->targets=NULL;
	}

	/* Call parent's class dispose method */
	G_OBJECT_CLASS(xfdashboard_drag_action_parent_class)->dispose(inObject);
}


/* Set/get properties */
static void _xfdashboard_drag_action_set_property(GObject *inObject,
													guint inPropID,
													const GValue *inValue,
													GParamSpec *inSpec)
{
	XfdashboardDragAction			*self=XFDASHBOARD_DRAG_ACTION(inObject);

	switch(inPropID)
	{
		case PROP_SOURCE:
			_xfdashboard_drag_action_set_source(self, g_value_get_object(inValue));
			break;

		default:
			G_OBJECT_WARN_INVALID_PROPERTY_ID(inObject, inPropID, inSpec);
			break;
	}
}

static void _xfdashboard_drag_action_get_property(GObject *inObject,
													guint inPropID,
													GValue *outValue,
													GParamSpec *inSpec)
{
	XfdashboardDragAction			*self=XFDASHBOARD_DRAG_ACTION(inObject);
	XfdashboardDragActionPrivate	*priv=self->priv;

	switch(inPropID)
	{
		case PROP_SOURCE:
			g_value_set_object(outValue, priv->source);
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
void xfdashboard_drag_action_class_init(XfdashboardDragActionClass *klass)
{
	ClutterDragActionClass	*dragActionClass=CLUTTER_DRAG_ACTION_CLASS(klass);
	GObjectClass			*gobjectClass=G_OBJECT_CLASS(klass);

	/* Override functions */
	gobjectClass->set_property=_xfdashboard_drag_action_set_property;
	gobjectClass->get_property=_xfdashboard_drag_action_get_property;
	gobjectClass->dispose=_xfdashboard_drag_action_dispose;

	dragActionClass->drag_begin=_xfdashboard_drag_action_drag_begin;
	dragActionClass->drag_motion=_xfdashboard_drag_action_drag_motion;
	dragActionClass->drag_end=_xfdashboard_drag_action_drag_end;

	/* Set up private structure */
	g_type_class_add_private(klass, sizeof(XfdashboardDragActionPrivate));

	/* Define properties */
	XfdashboardDragActionProperties[PROP_SOURCE]=
		g_param_spec_object("source",
							_("Source"),
							_("The source actor where drag began"),
							CLUTTER_TYPE_ACTOR,
							G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY);

	g_object_class_install_properties(gobjectClass, PROP_LAST, XfdashboardDragActionProperties);

	/* Define signals */
	XfdashboardDragActionSignals[SIGNAL_DRAG_CANCEL]=
		g_signal_new("drag-cancel",
						G_TYPE_FROM_CLASS(klass),
						G_SIGNAL_RUN_LAST,
						G_STRUCT_OFFSET(XfdashboardDragActionClass, drag_cancel),
						NULL,
						NULL,
						_xfdashboard_marshal_VOID__OBJECT_FLOAT_FLOAT,
						G_TYPE_NONE,
						3,
						CLUTTER_TYPE_ACTOR,
						G_TYPE_FLOAT,
						G_TYPE_FLOAT);
}

/* Object initialization
 * Create private structure and set up default values
 */
void xfdashboard_drag_action_init(XfdashboardDragAction *self)
{
	XfdashboardDragActionPrivate	*priv;

	priv=self->priv=XFDASHBOARD_DRAG_ACTION_GET_PRIVATE(self);

	/* Set up default values */
	priv->source=NULL;
	priv->actor=NULL;
	priv->targets=NULL;
	priv->lastDropTarget=NULL;
	priv->lastDeltaX=0.0f;
	priv->lastDeltaY=0.0f;
}

/* IMPLEMENTATION: Public API */

/* Create new action */
ClutterAction* xfdashboard_drag_action_new(void)
{
	return(g_object_new(XFDASHBOARD_TYPE_DRAG_ACTION, NULL));
}

ClutterAction* xfdashboard_drag_action_new_with_source(ClutterActor *inSource)
{
	g_return_val_if_fail(CLUTTER_IS_ACTOR(inSource), NULL);

	return(g_object_new(XFDASHBOARD_TYPE_DRAG_ACTION,
						"source", inSource,
						NULL));
}

/* Get source actor where drag began */
ClutterActor* xfdashboard_drag_action_get_source(XfdashboardDragAction *self)
{
	g_return_val_if_fail(XFDASHBOARD_IS_DRAG_ACTION(self), NULL);

	return(self->priv->source);
}

/* Get dragged actor (not the drag handle used while dragging) */
ClutterActor* xfdashboard_drag_action_get_actor(XfdashboardDragAction *self)
{
	g_return_val_if_fail(XFDASHBOARD_IS_DRAG_ACTION(self), NULL);

	return(self->priv->actor);
}

/* Get last motion delta coordinates */
void xfdashboard_drag_action_get_motion_delta(XfdashboardDragAction *self, gfloat *outDeltaX, gfloat *outDeltaY)
{
	g_return_if_fail(XFDASHBOARD_IS_DRAG_ACTION(self));

	if(outDeltaX) *outDeltaX=self->priv->lastDeltaX;
	if(outDeltaY) *outDeltaY=self->priv->lastDeltaY;
}
