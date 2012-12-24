/*
 * drop-action.c: Drop action for drop targets
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

#include "drop-action.h"
#include "drop-targets.h"
#include "marshal.h"

/* Define this class in GObject system */
G_DEFINE_TYPE(XfdashboardDropAction,
				xfdashboard_drop_action,
				CLUTTER_TYPE_ACTION)

/* Private structure - access only by public API if needed */
#define XFDASHBOARD_DROP_ACTION_GET_PRIVATE(obj) \
	(G_TYPE_INSTANCE_GET_PRIVATE((obj), XFDASHBOARD_TYPE_DROP_ACTION, XfdashboardDropActionPrivate))

struct _XfdashboardDropActionPrivate
{
	/* Target actor of drop action */
	ClutterActor			*targetActor;
	guint					destroySignalID;
};

/* Signals */
enum
{
	BEGIN,
	CAN_DROP,
	DROP,
	END,

	DRAG_ENTER,
	DRAG_MOTION,
	DRAG_LEAVE,

	SIGNAL_LAST
};

static guint XfdashboardDropActionSignals[SIGNAL_LAST]={ 0, };

/* IMPLEMENTATION: Private variables and methods */

/* Signal accumulator which stops further signal emission on first
 * signal handler returning FALSE
 */
gboolean _xfdashboard_drop_action_signal_accumulator_stop_on_first_false(GSignalInvocationHint *inHint,
																			GValue *outAccumulatorResult,
																			const GValue *inSignalHandlerResult,
																			gpointer inUserData)
{
	gboolean		continueEmission;

	continueEmission=g_value_get_boolean(inSignalHandlerResult);
	g_value_set_boolean(outAccumulatorResult, continueEmission);

	return(continueEmission);
}

/* Target actor will be destroyed */
void _xfdashboard_drop_action_on_target_actor_destroy(XfdashboardDropAction *self, ClutterActor *inTargetActor)
{
	g_return_if_fail(XFDASHBOARD_IS_DROP_ACTION(self));
	g_return_if_fail(CLUTTER_IS_ACTOR(inTargetActor));

	XfdashboardDropActionPrivate	*priv=self->priv;

	/* Check that destroyed actor matches drop action's target actor */
	g_return_if_fail(inTargetActor==priv->targetActor);

	/* Disconnect signals */
	if(priv->destroySignalID) g_signal_handler_disconnect(priv->targetActor, priv->destroySignalID);

	/* Unregister drop target */
	xfdashboard_drop_targets_unregister(self);

	priv->destroySignalID=0;
	priv->targetActor=NULL;
}

/* Default signal handler for "begin" */
gboolean _xfdashboard_drop_action_class_real_begin(XfdashboardDropAction *self,
													XfdashboardDragAction *inDragAction)
{
	g_return_val_if_fail(XFDASHBOARD_IS_DROP_ACTION(self), FALSE);
	g_return_val_if_fail(self->priv->targetActor, FALSE);

	XfdashboardDropActionPrivate	*priv=self->priv;

	/* Return TRUE means we can handle dragged actor on this drop target. This is only
	 * possible if drop target is visible and reactive. Otherwise we have to return FALSE.
	 */
	return(CLUTTER_ACTOR_IS_VISIBLE(priv->targetActor) &&
			CLUTTER_ACTOR_IS_REACTIVE(priv->targetActor));
}

/* Default signal handler for "can-drop" */
gboolean _xfdashboard_drop_action_class_real_can_drop(XfdashboardDropAction *self,
														XfdashboardDragAction *inDragAction,
														gfloat inX,
														gfloat inY)
{
	g_return_val_if_fail(XFDASHBOARD_IS_DROP_ACTION(self), FALSE);
	g_return_val_if_fail(self->priv->targetActor, FALSE);

	XfdashboardDropActionPrivate	*priv=self->priv;

	/* Return TRUE means we can handle dragged actor on this drop target. This is only
	 * possible if drop target is visible and reactive. Otherwise we have to return FALSE.
	 */
	return(CLUTTER_ACTOR_IS_VISIBLE(priv->targetActor) &&
			CLUTTER_ACTOR_IS_REACTIVE(priv->targetActor));
}

/* IMPLEMENTATION: ClutterActorMeta */

/* Called when attaching and detaching a ClutterActorMeta instance to a ClutterActor */
void xfdashboard_drop_action_set_actor(ClutterActorMeta	*inActorMeta,
										ClutterActor *inActor)
{
	g_return_if_fail(XFDASHBOARD_IS_DROP_ACTION(inActorMeta));
	g_return_if_fail(inActor==NULL || CLUTTER_IS_ACTOR(inActor));

	XfdashboardDropAction			*self=XFDASHBOARD_DROP_ACTION(inActorMeta);
	XfdashboardDropActionPrivate	*priv=self->priv;

	/* Unregister current drop target */
	if(priv->targetActor)
	{
		/* Disconnect signals */
		if(priv->destroySignalID) g_signal_handler_disconnect(priv->targetActor, priv->destroySignalID);

		/* Unregister drop target */
		xfdashboard_drop_targets_unregister(self);

		priv->destroySignalID=0;
		priv->targetActor=NULL;
	}

	/* Register new drop target */
	if(inActor)
	{
		priv->targetActor=inActor;

		/* Register drop target */
		xfdashboard_drop_targets_register(self);

		/* Connect signals */
		priv->destroySignalID=g_signal_connect_swapped(priv->targetActor,
														"destroy",
														G_CALLBACK(_xfdashboard_drop_action_on_target_actor_destroy),
														self);
	}

	/* Call parent's class method */
	CLUTTER_ACTOR_META_CLASS(xfdashboard_drop_action_parent_class)->set_actor(inActorMeta, inActor);
}

/* IMPLEMENTATION: GObject */

/* Class initialization
 * Override functions in parent classes and define properties
 * and signals
 */
static void xfdashboard_drop_action_class_init(XfdashboardDropActionClass *klass)
{
	ClutterActorMetaClass		*actorMetaClass=CLUTTER_ACTOR_META_CLASS(klass);

	/* Override functions */
	actorMetaClass->set_actor=xfdashboard_drop_action_set_actor;

	klass->begin=_xfdashboard_drop_action_class_real_begin;
	klass->can_drop=_xfdashboard_drop_action_class_real_can_drop;
	
	/* Set up private structure */
	g_type_class_add_private(klass, sizeof(XfdashboardDropActionPrivate));

	/* Define signals */
	XfdashboardDropActionSignals[BEGIN]=
		g_signal_new("begin",
						G_TYPE_FROM_CLASS(klass),
						G_SIGNAL_RUN_LAST,
						G_STRUCT_OFFSET(XfdashboardDropActionClass, begin),
						_xfdashboard_drop_action_signal_accumulator_stop_on_first_false,
						NULL,
						_xfdashboard_marshal_BOOLEAN__OBJECT,
						G_TYPE_BOOLEAN,
						1,
						XFDASHBOARD_TYPE_DRAG_ACTION);

	XfdashboardDropActionSignals[CAN_DROP]=
		g_signal_new("can-drop",
						G_TYPE_FROM_CLASS(klass),
						G_SIGNAL_RUN_LAST,
						G_STRUCT_OFFSET(XfdashboardDropActionClass, can_drop),
						_xfdashboard_drop_action_signal_accumulator_stop_on_first_false,
						NULL,
						_xfdashboard_marshal_BOOLEAN__OBJECT_FLOAT_FLOAT,
						G_TYPE_BOOLEAN,
						3,
						XFDASHBOARD_TYPE_DRAG_ACTION,
						G_TYPE_FLOAT,
						G_TYPE_FLOAT);

	XfdashboardDropActionSignals[DROP]=
		g_signal_new("drop",
						G_TYPE_FROM_CLASS(klass),
						G_SIGNAL_RUN_LAST,
						G_STRUCT_OFFSET(XfdashboardDropActionClass, drop),
						NULL,
						NULL,
						_xfdashboard_marshal_VOID__OBJECT_FLOAT_FLOAT,
						G_TYPE_NONE,
						3,
						XFDASHBOARD_TYPE_DRAG_ACTION,
						G_TYPE_FLOAT,
						G_TYPE_FLOAT);

	XfdashboardDropActionSignals[END]=
		g_signal_new("end",
						G_TYPE_FROM_CLASS(klass),
						G_SIGNAL_RUN_LAST,
						G_STRUCT_OFFSET(XfdashboardDropActionClass, end),
						NULL,
						NULL,
						g_cclosure_marshal_VOID__OBJECT,
						G_TYPE_NONE,
						1,
						XFDASHBOARD_TYPE_DRAG_ACTION);

	XfdashboardDropActionSignals[DRAG_ENTER]=
		g_signal_new("enter",
						G_TYPE_FROM_CLASS(klass),
						G_SIGNAL_RUN_LAST,
						G_STRUCT_OFFSET(XfdashboardDropActionClass, enter),
						NULL,
						NULL,
						g_cclosure_marshal_VOID__OBJECT,
						G_TYPE_NONE,
						1,
						XFDASHBOARD_TYPE_DRAG_ACTION);

	XfdashboardDropActionSignals[DRAG_LEAVE]=
		g_signal_new("leave",
						G_TYPE_FROM_CLASS(klass),
						G_SIGNAL_RUN_LAST,
						G_STRUCT_OFFSET(XfdashboardDropActionClass, leave),
						NULL,
						NULL,
						g_cclosure_marshal_VOID__OBJECT,
						G_TYPE_NONE,
						1,
						XFDASHBOARD_TYPE_DRAG_ACTION);

	XfdashboardDropActionSignals[DRAG_MOTION]=
		g_signal_new("motion",
						G_TYPE_FROM_CLASS(klass),
						G_SIGNAL_RUN_LAST,
						G_STRUCT_OFFSET(XfdashboardDropActionClass, motion),
						NULL,
						NULL,
						_xfdashboard_marshal_VOID__OBJECT_FLOAT_FLOAT,
						G_TYPE_NONE,
						3,
						XFDASHBOARD_TYPE_DRAG_ACTION,
						G_TYPE_FLOAT,
						G_TYPE_FLOAT);
}

/* Object initialization
 * Create private structure and set up default values
 */
static void xfdashboard_drop_action_init(XfdashboardDropAction *self)
{
	XfdashboardDropActionPrivate	*priv;

	/* Set up default values */
	priv=self->priv=XFDASHBOARD_DROP_ACTION_GET_PRIVATE(self);

	priv->destroySignalID=0;
	priv->targetActor=NULL;
}

/* Implementation: Public API */

/* Create new action */
ClutterAction* xfdashboard_drop_action_new(void)
{
	return(g_object_new(XFDASHBOARD_TYPE_DROP_ACTION, NULL));
}
