/*
 * click-action: Bad workaround for click action which prevent drag actions
 *               to work properly since clutter version 1.12 at least.
 *               This object/file is a complete copy of the original
 *               clutter-click-action.{c,h} files of clutter 1.12 except
 *               for one line, the renamed function names and the applied
 *               coding style. The clutter-click-action.{c,h} files of
 *               later clutter versions do not differ much from this one
 *               so this object should work also for this versions.
 *
 *               See bug: https://bugzilla.gnome.org/show_bug.cgi?id=714993
 * 
 * Copyright 2012-2015 Stephan Haller <nomad@froevel.de>
 *         original by Emmanuele Bassi <ebassi@linux.intel.com>
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

#include "click-action.h"

#include <glib/gi18n-lib.h>

#include "marshal.h"
#include "actor.h"
#include "stylable.h"

/* Define this class in GObject system */
G_DEFINE_TYPE(XfdashboardClickAction,
				xfdashboard_click_action,
				CLUTTER_TYPE_ACTION);

/* Private structure - access only by public API if needed */
#define XFDASHBOARD_CLICK_ACTION_GET_PRIVATE(obj) \
	(G_TYPE_INSTANCE_GET_PRIVATE((obj), XFDASHBOARD_TYPE_CLICK_ACTION, XfdashboardClickActionPrivate))

struct _XfdashboardClickActionPrivate
{
	/* Properties related */
	guint					isHeld : 1;
	guint					isPressed : 1;

	gint					longPressThreshold;
	gint					longPressDuration;

	/* Instance related */
	ClutterActor			*stage;

	guint					eventID;
	guint					captureID;
	guint					longPressID;

	gint					dragThreshold;

	guint					pressButton;
	gint					pressDeviceID;
	ClutterEventSequence	*pressSequence;
	ClutterModifierType		modifierState;
	gfloat					pressX;
	gfloat					pressY;
};

/* Properties */
enum
{
	PROP_0,

	PROP_HELD,
	PROP_PRESSED,
	PROP_LONG_PRESS_THRESHOLD,
	PROP_LONG_PRESS_DURATION,

	PROP_LAST
};

GParamSpec* XfdashboardClickActionProperties[PROP_LAST]={ 0, };

/* Signals */
enum
{
	SIGNAL_CLICKED,
	SIGNAL_LONG_PRESS,

	SIGNAL_LAST
};

guint XfdashboardClickActionSignals[SIGNAL_LAST]={ 0, };

/* IMPLEMENTATION: Private variables and methods */

/* Set press state */
static void _xfdashboard_click_action_set_pressed(XfdashboardClickAction *self, gboolean isPressed)
{
	XfdashboardClickActionPrivate	*priv;
	ClutterActor					*actor;

	g_return_if_fail(XFDASHBOARD_IS_CLICK_ACTION(self));

	priv=self->priv;

	/* Set value if changed */
	isPressed=!!isPressed;

	if(priv->isPressed!=isPressed)
	{
		/* Set value */
		priv->isPressed=isPressed;

		/* Style state */
		actor=clutter_actor_meta_get_actor(CLUTTER_ACTOR_META(self));
		if(XFDASHBOARD_IS_ACTOR(actor))
		{
			if(priv->isPressed) xfdashboard_stylable_add_pseudo_class(XFDASHBOARD_STYLABLE(actor), "pressed");
				else xfdashboard_stylable_remove_pseudo_class(XFDASHBOARD_STYLABLE(actor), "pressed");
		}

		/* Notify about property change */
		g_object_notify_by_pspec(G_OBJECT(self), XfdashboardClickActionProperties[PROP_PRESSED]);
	}
}

/* Set held state */
static void _xfdashboard_click_action_set_held(XfdashboardClickAction *self, gboolean isHeld)
{
	XfdashboardClickActionPrivate	*priv;

	g_return_if_fail(XFDASHBOARD_IS_CLICK_ACTION(self));

	priv=self->priv;

	/* Set value if changed */
	isHeld=!!isHeld;

	if(priv->isHeld!=isHeld)
	{
		/* Set value */
		priv->isHeld=isHeld;

		/* Notify about property change */
		g_object_notify_by_pspec(G_OBJECT(self), XfdashboardClickActionProperties[PROP_HELD]);
	}
}

/* Emit "long-press" signal */
static gboolean _xfdashboard_click_action_emit_long_press(gpointer inUserData)
{
	XfdashboardClickAction			*self;
	XfdashboardClickActionPrivate	*priv;
	ClutterActor					*actor;
	gboolean						result;

	g_return_val_if_fail(XFDASHBOARD_IS_CLICK_ACTION(inUserData), FALSE);

	self=XFDASHBOARD_CLICK_ACTION(inUserData);
	priv=self->priv;

	/* Reset variables */
	priv->longPressID=0;

	/* Emit signal */
	actor=clutter_actor_meta_get_actor(CLUTTER_ACTOR_META(inUserData));
	g_signal_emit(self, XfdashboardClickActionSignals[SIGNAL_LONG_PRESS], 0, actor, CLUTTER_LONG_PRESS_ACTIVATE, &result);

	/* Disconnect signal handlers */
	if(priv->captureID!=0)
	{
		g_signal_handler_disconnect(priv->stage, priv->captureID);
		priv->captureID=0;
	}

	/* Reset state of this action */
	_xfdashboard_click_action_set_pressed(self, FALSE);
	_xfdashboard_click_action_set_held(self, FALSE);

	/* Event handled */
	return(FALSE);
}

/* Query if long-press events should be handled and signals emitted */
static void _xfdashboard_click_action_query_long_press(XfdashboardClickAction *self)
{
	XfdashboardClickActionPrivate	*priv;
	ClutterActor					*actor;
	gboolean						result;
	gint							timeout;

	g_return_if_fail(XFDASHBOARD_IS_CLICK_ACTION(self));

	priv=self->priv;
	result=FALSE;

	/* If no duration was set get default one from settings */
	if(priv->longPressDuration<0)
	{
		ClutterSettings				*settings=clutter_settings_get_default();

		g_object_get(settings, "long-press-duration", &timeout, NULL);
	}
		else timeout=priv->longPressDuration;

	/* Emit signal to determine if long-press should be supported */
	actor=clutter_actor_meta_get_actor(CLUTTER_ACTOR_META(self));
	g_signal_emit(self, XfdashboardClickActionSignals[SIGNAL_LONG_PRESS], 0, actor, CLUTTER_LONG_PRESS_QUERY, &result);

	if(result)
	{
		priv->longPressID=clutter_threads_add_timeout(timeout,
														_xfdashboard_click_action_emit_long_press,
														self);
	}
}

/* Cancel long-press handling */
static void _xfdashboard_click_action_cancel_long_press(XfdashboardClickAction *self)
{
	XfdashboardClickActionPrivate	*priv;

	g_return_if_fail(XFDASHBOARD_IS_CLICK_ACTION(self));

	priv=self->priv;

	/* Remove signals/sources and emit cancel signal */
	if(priv->longPressID!=0)
	{
		ClutterActor				*actor;
		gboolean					result;

		/* Remove source */
		g_source_remove(priv->longPressID);
		priv->longPressID=0;

		/* Emit signal */
		actor=clutter_actor_meta_get_actor(CLUTTER_ACTOR_META(self));
		g_signal_emit(self, XfdashboardClickActionSignals[SIGNAL_LONG_PRESS], 0, actor, CLUTTER_LONG_PRESS_CANCEL, &result);
	}
}

/* An event was captured */
static gboolean _xfdashboard_click_action_on_captured_event(XfdashboardClickAction *self,
															ClutterEvent *inEvent,
															gpointer inUserData)
{
	XfdashboardClickActionPrivate	*priv;
	ClutterActor					*stage G_GNUC_UNUSED;
	ClutterActor					*actor;
	ClutterModifierType				modifierState;
	gboolean						hasButton;

	g_return_val_if_fail(XFDASHBOARD_IS_CLICK_ACTION(self), CLUTTER_EVENT_PROPAGATE);
	g_return_val_if_fail(CLUTTER_IS_ACTOR(inUserData), CLUTTER_EVENT_PROPAGATE);

	priv=self->priv;
	stage=CLUTTER_ACTOR(inUserData);
	hasButton=TRUE;

	/* Handle captured event */
	actor=clutter_actor_meta_get_actor(CLUTTER_ACTOR_META(self));
	switch(clutter_event_type(inEvent))
	{
		case CLUTTER_TOUCH_END:
			hasButton=FALSE;

		case CLUTTER_BUTTON_RELEASE:
			if(!priv->isHeld) return(CLUTTER_EVENT_STOP);

			if((hasButton && clutter_event_get_button(inEvent)!=priv->pressButton) ||
				(hasButton && clutter_event_get_click_count(inEvent)!=1) ||
				clutter_event_get_device_id(inEvent)!=priv->pressDeviceID ||
				clutter_event_get_event_sequence(inEvent)!=priv->pressSequence)
			{
				return(CLUTTER_EVENT_PROPAGATE);
			}

			_xfdashboard_click_action_set_held(self, FALSE);
			_xfdashboard_click_action_cancel_long_press(self);

			/* Disconnect the capture */
			if(priv->captureID!=0)
			{
				g_signal_handler_disconnect(priv->stage, priv->captureID);
				priv->captureID = 0;
			}

			if(priv->longPressID!=0)
			{
				g_source_remove(priv->longPressID);
				priv->longPressID=0;
			}

			if(!clutter_actor_contains(actor, clutter_event_get_source(inEvent)))
			{
				return(CLUTTER_EVENT_PROPAGATE);
			}

			/* Exclude any button-mask so that we can compare
			 * the press and release states properly
			 */
			modifierState=clutter_event_get_state(inEvent) &
							~(CLUTTER_BUTTON1_MASK |
								CLUTTER_BUTTON2_MASK |
								CLUTTER_BUTTON3_MASK |
								CLUTTER_BUTTON4_MASK |
								CLUTTER_BUTTON5_MASK);

			/* If press and release states don't match we simply ignore
			 * modifier keys. i.e. modifier keys are expected to be pressed
			 * throughout the whole click
			 */
			if(modifierState!=priv->modifierState) priv->modifierState=0;

			_xfdashboard_click_action_set_pressed(self, FALSE);
			g_signal_emit(self, XfdashboardClickActionSignals[SIGNAL_CLICKED], 0, actor);
			break;

		case CLUTTER_MOTION:
		case CLUTTER_TOUCH_UPDATE:
			{
				gfloat				motionX, motionY;
				gfloat				deltaX, deltaY;

				if(!priv->isHeld) return(CLUTTER_EVENT_PROPAGATE);

				clutter_event_get_coords (inEvent, &motionX, &motionY);

				deltaX=ABS(motionX-priv->pressX);
				deltaY=ABS(motionY-priv->pressY);

				if(deltaX>priv->dragThreshold || deltaY>priv->dragThreshold)
				{
					_xfdashboard_click_action_cancel_long_press(self);
				}
			}
			break;

		default:
			break;
	}

	/* This is line changed in returning CLUTTER_EVENT_PROPAGATE
	 * instead of CLUTTER_EVENT_STOP
	 */
	return(CLUTTER_EVENT_PROPAGATE);
}

/* An event was received */
static gboolean _xfdashboard_click_action_on_event(XfdashboardClickAction *self, ClutterEvent *inEvent, gpointer inUserData)
{
	XfdashboardClickActionPrivate	*priv;
	gboolean						hasButton;
	ClutterActor					*actor;

	g_return_val_if_fail(XFDASHBOARD_IS_CLICK_ACTION(self), CLUTTER_EVENT_PROPAGATE);
	g_return_val_if_fail(CLUTTER_IS_ACTOR(inUserData), CLUTTER_EVENT_PROPAGATE);

	priv=self->priv;
	hasButton=TRUE;
	actor=CLUTTER_ACTOR(inUserData);

	/* Check if actor is enabled to handle events */
	if(!clutter_actor_meta_get_enabled(CLUTTER_ACTOR_META(self))) return(CLUTTER_EVENT_PROPAGATE);

	/* Handle event */
	switch(clutter_event_type(inEvent))
	{
		case CLUTTER_TOUCH_BEGIN:
			hasButton=FALSE;

		case CLUTTER_BUTTON_PRESS:
			/* We only handle single clicks if it is pointer device */
			if(hasButton && clutter_event_get_click_count(inEvent)!=1)
			{
				return(CLUTTER_EVENT_PROPAGATE);
			}

			/* Do we already held the press? */
			if(priv->isHeld) return(CLUTTER_EVENT_STOP);

			/* Is the source of event a child of this actor. If not do
			 * not handle this event but any other.
			 */
			if(!clutter_actor_contains(actor, clutter_event_get_source(inEvent)))
			{
				return(CLUTTER_EVENT_PROPAGATE);
			}

			/* Remember event data */
			priv->pressButton=hasButton ? clutter_event_get_button(inEvent) : 0;
			priv->pressDeviceID=clutter_event_get_device_id(inEvent);
			priv->pressSequence=clutter_event_get_event_sequence(inEvent);
			priv->modifierState=clutter_event_get_state(inEvent);
			clutter_event_get_coords(inEvent, &priv->pressX, &priv->pressY);

			if(priv->longPressThreshold<0)
			{
				ClutterSettings		*settings=clutter_settings_get_default();

				g_object_get(settings, "dnd-drag-threshold", &priv->dragThreshold, NULL);
			}
				else priv->dragThreshold=priv->longPressThreshold;

			if(priv->stage==NULL) priv->stage=clutter_actor_get_stage(actor);

			/* Connect signals */
			priv->captureID=g_signal_connect_object(priv->stage,
													"captured-event",
													G_CALLBACK(_xfdashboard_click_action_on_captured_event),
													self,
													G_CONNECT_AFTER | G_CONNECT_SWAPPED);

			/* Set state of this action */
			_xfdashboard_click_action_set_pressed(self, TRUE);
			_xfdashboard_click_action_set_held(self, TRUE);
			_xfdashboard_click_action_query_long_press(self);
			break;

		case CLUTTER_ENTER:
			_xfdashboard_click_action_set_pressed(self, priv->isHeld);
			break;

		case CLUTTER_LEAVE:
			_xfdashboard_click_action_set_pressed(self, priv->isHeld);
			_xfdashboard_click_action_cancel_long_press(self);
			break;

		default:
			break;
	}

	return(CLUTTER_EVENT_PROPAGATE);
}

/* IMPLEMENTATION: ClutterActorMeta */

/* Called when attaching and detaching a ClutterActorMeta instance to a ClutterActor */
static void _xfdashboard_click_action_set_actor(ClutterActorMeta *inActorMeta, ClutterActor *inActor)
{
	XfdashboardClickAction			*self;
	XfdashboardClickActionPrivate	*priv;

	g_return_if_fail(XFDASHBOARD_IS_CLICK_ACTION(inActorMeta));

	self=XFDASHBOARD_CLICK_ACTION(inActorMeta);
	priv=self->priv;

	/* Disconnect signals and remove sources */
	if(priv->eventID!=0)
	{
		ClutterActor				*oldActor=clutter_actor_meta_get_actor(inActorMeta);

		if(oldActor!=NULL) g_signal_handler_disconnect(oldActor, priv->eventID);
		priv->eventID=0;
	}

	if(priv->captureID!=0)
	{
		if(priv->stage!=NULL) g_signal_handler_disconnect(priv->stage, priv->captureID);
		priv->captureID=0;
		priv->stage=NULL;
	}

	if(priv->longPressID!=0)
	{
		g_source_remove(priv->longPressID);
		priv->longPressID=0;
	}

	/* Reset state of this action */
	_xfdashboard_click_action_set_pressed(self, FALSE);
	_xfdashboard_click_action_set_held(self, FALSE);

	/* Connect signals */
	if(inActor!=NULL)
	{
		priv->eventID=g_signal_connect_swapped(inActor,
													"event",
													G_CALLBACK(_xfdashboard_click_action_on_event),
													self);
	}

	/* Call parent's class method */
	if(CLUTTER_ACTOR_META_CLASS(xfdashboard_click_action_parent_class)->set_actor)
	{
		CLUTTER_ACTOR_META_CLASS(xfdashboard_click_action_parent_class)->set_actor(inActorMeta, inActor);
	}
}

/* IMPLEMENTATION: GObject */

/* Set/get properties */
static void _xfdashboard_click_action_set_property(GObject *inObject,
													guint inPropID,
													const GValue *inValue,
													GParamSpec *inSpec)
{
	XfdashboardClickAction			*self=XFDASHBOARD_CLICK_ACTION(inObject);
	XfdashboardClickActionPrivate	*priv=self->priv;

	switch(inPropID)
	{
		case PROP_LONG_PRESS_DURATION:
			priv->longPressDuration=g_value_get_int(inValue);
			break;

		case PROP_LONG_PRESS_THRESHOLD:
			priv->longPressThreshold=g_value_get_int(inValue);
			break;

		default:
			G_OBJECT_WARN_INVALID_PROPERTY_ID(inObject, inPropID, inSpec);
			break;
	}
}

static void _xfdashboard_click_action_get_property(GObject *inObject,
													guint inPropID,
													GValue *outValue,
													GParamSpec *inSpec)
{
	XfdashboardClickAction			*self=XFDASHBOARD_CLICK_ACTION(inObject);
	XfdashboardClickActionPrivate	*priv=self->priv;

	switch(inPropID)
	{
		case PROP_HELD:
			g_value_set_boolean(outValue, priv->isHeld);
			break;

		case PROP_PRESSED:
			g_value_set_boolean(outValue, priv->isPressed);
			break;

		case PROP_LONG_PRESS_DURATION:
			g_value_set_int(outValue, priv->longPressDuration);
			break;

		case PROP_LONG_PRESS_THRESHOLD:
			g_value_set_int(outValue, priv->longPressThreshold);
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
static void xfdashboard_click_action_class_init(XfdashboardClickActionClass *klass)
{
	GObjectClass			*gobjectClass=G_OBJECT_CLASS(klass);
	ClutterActorMetaClass	*actorMetaClass=CLUTTER_ACTOR_META_CLASS(klass);

	/* Override functions */
	actorMetaClass->set_actor=_xfdashboard_click_action_set_actor;

	gobjectClass->set_property=_xfdashboard_click_action_set_property;
	gobjectClass->get_property=_xfdashboard_click_action_get_property;

	/* Set up private structure */
	g_type_class_add_private(klass, sizeof (XfdashboardClickActionPrivate));

	/* Define properties */
	XfdashboardClickActionProperties[PROP_PRESSED]=
		g_param_spec_boolean("pressed",
								_("Pressed"),
								_("Whether the clickable should be in pressed state"),
								FALSE,
								G_PARAM_READABLE | G_PARAM_STATIC_STRINGS);

	XfdashboardClickActionProperties[PROP_HELD]=
		g_param_spec_boolean("held",
								_("Held"),
								_("Whether the clickable has a grab"),
								FALSE,
								G_PARAM_READABLE | G_PARAM_STATIC_STRINGS);

	XfdashboardClickActionProperties[PROP_LONG_PRESS_DURATION]=
		g_param_spec_int("long-press-duration",
							_("Long Press Duration"),
							_("The minimum duration of a long press to recognize the gesture"),
							-1,
							G_MAXINT,
							-1,
							G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS);

	XfdashboardClickActionProperties[PROP_LONG_PRESS_THRESHOLD]=
		g_param_spec_int("long-press-threshold",
							_("Long Press Threshold"),
							_("The maximum threshold before a long press is cancelled"),
							-1,
							G_MAXINT,
							-1,
							G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS);

	g_object_class_install_properties(gobjectClass, PROP_LAST, XfdashboardClickActionProperties);

	/* Define signals */
	XfdashboardClickActionSignals[SIGNAL_CLICKED]=
		g_signal_new("clicked",
						G_TYPE_FROM_CLASS(klass),
						G_SIGNAL_RUN_LAST,
						G_STRUCT_OFFSET(XfdashboardClickActionClass, clicked),
						NULL, NULL,
						g_cclosure_marshal_VOID__OBJECT,
						G_TYPE_NONE,
						1,
						CLUTTER_TYPE_ACTOR);

	XfdashboardClickActionSignals[SIGNAL_LONG_PRESS]=
		g_signal_new("long-press",
						G_TYPE_FROM_CLASS(klass),
						G_SIGNAL_RUN_LAST,
						G_STRUCT_OFFSET(XfdashboardClickActionClass, long_press),
						NULL, NULL,
						_xfdashboard_marshal_BOOLEAN__OBJECT_ENUM,
						G_TYPE_BOOLEAN,
						2,
						CLUTTER_TYPE_ACTOR,
						CLUTTER_TYPE_LONG_PRESS_STATE);
}

/* Object initialization
 * Create private structure and set up default values
 */
static void xfdashboard_click_action_init(XfdashboardClickAction *self)
{
	XfdashboardClickActionPrivate	*priv;

	priv=self->priv=XFDASHBOARD_CLICK_ACTION_GET_PRIVATE(self);

	/* Set up default values */
	priv->longPressThreshold=-1;
	priv->longPressDuration=-1;
}

/* IMPLEMENTATION: Public API */

/* Create new action */
ClutterAction* xfdashboard_click_action_new(void)
{
	return(CLUTTER_ACTION(g_object_new(XFDASHBOARD_TYPE_CLICK_ACTION, NULL)));
}

/* Get pressed button */
guint xfdashboard_click_action_get_button(XfdashboardClickAction *self)
{
	g_return_val_if_fail(XFDASHBOARD_IS_CLICK_ACTION(self), 0);

	return(self->priv->pressButton);
}

/* Get modifier state of the click action */
ClutterModifierType xfdashboard_click_action_get_state(XfdashboardClickAction *self)
{
	g_return_val_if_fail(XFDASHBOARD_IS_CLICK_ACTION(self), 0);

	return(self->priv->modifierState);
}

/* Get screen coordinates of the button press */
void xfdashboard_click_action_get_coords(XfdashboardClickAction *self, gfloat *outPressX, gfloat *outPressY)
{
	g_return_if_fail(XFDASHBOARD_IS_CLICK_ACTION(self));

	if(outPressX!=NULL) *outPressX=self->priv->pressX;
	if(outPressY!=NULL) *outPressY=self->priv->pressY;
}

/* Emulate a release of the pointer button */
void xfdashboard_click_action_release(XfdashboardClickAction *self)
{
	XfdashboardClickActionPrivate	*priv;

	g_return_if_fail(XFDASHBOARD_IS_CLICK_ACTION(self));

	priv=self->priv;

	/* Only release pointer button if it is held by this action */
	if(!priv->isHeld) return;

	/* Disconnect signal handlers */
	if(priv->captureID!=0)
	{
		g_signal_handler_disconnect(priv->stage, priv->captureID);
		priv->captureID=0;
	}

	/* Reset state of this action */
	_xfdashboard_click_action_cancel_long_press(self);
	_xfdashboard_click_action_set_held(self, FALSE);
	_xfdashboard_click_action_set_pressed(self, FALSE);
}
