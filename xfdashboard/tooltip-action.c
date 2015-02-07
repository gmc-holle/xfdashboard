/*
 * tooltip-action: An action to display a tooltip after a short timeout
 *                 without movement at the referred actor
 * 
 * Copyright 2012-2015 Stephan Haller <nomad@froevel.de>
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

#include "tooltip-action.h"

#include <glib/gi18n-lib.h>
#include <gtk/gtk.h>

#include "actor.h"
#include "stage.h"

/* Define this class in GObject system */
G_DEFINE_TYPE(XfdashboardTooltipAction,
				xfdashboard_tooltip_action,
				CLUTTER_TYPE_ACTION);

/* Private structure - access only by public API if needed */
#define XFDASHBOARD_TOOLTIP_ACTION_GET_PRIVATE(obj) \
	(G_TYPE_INSTANCE_GET_PRIVATE((obj), XFDASHBOARD_TYPE_TOOLTIP_ACTION, XfdashboardTooltipActionPrivate))

struct _XfdashboardTooltipActionPrivate
{
	/* Properties related */
	gchar			*tooltipText;

	/* Instance related */
	ClutterPoint	lastPosition;

	guint			motionID;
	guint			leaveID;
	guint			timeoutSourceID;
};

/* Properties */
enum
{
	PROP_0,

	PROP_TOOLTIP_TEXT,

	PROP_LAST
};

GParamSpec* XfdashboardTooltipActionProperties[PROP_LAST]={ 0, };

/* Signals */
enum
{
	SIGNAL_ACTIVATING,

	SIGNAL_LAST
};

static guint XfdashboardTooltipActionSignals[SIGNAL_LAST]={ 0, };

/* IMPLEMENTATION: Private variables and methods */
static ClutterActor		*_xfdashboard_tooltip_last_event_actor=NULL;

/* Timeout for tooltip has been reached */
static gboolean _xfdashboard_tooltip_action_on_timeout(gpointer inUserData)
{
	XfdashboardTooltipAction			*self;
	XfdashboardTooltipActionPrivate		*priv;
	ClutterActor						*actor;
	ClutterActor						*stage;

	g_return_val_if_fail(XFDASHBOARD_IS_TOOLTIP_ACTION(inUserData), G_SOURCE_REMOVE);

	self=XFDASHBOARD_TOOLTIP_ACTION(inUserData);
	priv=self->priv;

	/* Regardless how this function ends we will let this source be
	 * removed from main loop. So forget source ID ;)
	 */
	priv->timeoutSourceID=0;

	/* Check if last seen actor is this actor. If not we cannot display
	 * a tooltip and remove this source on return.
	 */
	actor=clutter_actor_meta_get_actor(CLUTTER_ACTOR_META(self));
	if(actor!=_xfdashboard_tooltip_last_event_actor) return(G_SOURCE_REMOVE);

	/* Show tooltip */
	stage=clutter_actor_get_stage(actor);
	if(stage && XFDASHBOARD_IS_STAGE(stage))
	{
		/* Emit 'activating' signal for last chance to update tooltip text */
		g_signal_emit(self, XfdashboardTooltipActionSignals[SIGNAL_ACTIVATING], 0);

		/* Show tooltip */
		g_signal_emit_by_name(stage, "show-tooltip", self, NULL);
	}

	/* Remove source */
	return(G_SOURCE_REMOVE);
}

/* Pointer was moved over actor with tooltip */
static gboolean _xfdashboard_tooltip_action_on_motion_event(XfdashboardTooltipAction *self,
															ClutterEvent *inEvent,
															gpointer inUserData)
{
	XfdashboardTooltipActionPrivate		*priv;
	ClutterActor						*actor;
	guint								tooltipTimeout;

	g_return_val_if_fail(XFDASHBOARD_IS_TOOLTIP_ACTION(self), CLUTTER_EVENT_PROPAGATE);
	g_return_val_if_fail(CLUTTER_IS_ACTOR(inUserData), CLUTTER_EVENT_PROPAGATE);

	priv=self->priv;
	actor=CLUTTER_ACTOR(inUserData);
	tooltipTimeout=0;

	/* Remove any timeout source we have added for this actor */
	if(priv->timeoutSourceID!=0)
	{
		g_source_remove(priv->timeoutSourceID);
		priv->timeoutSourceID=0;
	}

	/* Remember position and actor */
	clutter_event_get_position(inEvent, &priv->lastPosition);
	_xfdashboard_tooltip_last_event_actor=actor;

	/* Set up new timeout source */
	g_object_get(gtk_settings_get_default(),
					"gtk-tooltip-timeout", &tooltipTimeout,
					NULL);

	priv->timeoutSourceID=clutter_threads_add_timeout(tooltipTimeout,
														(GSourceFunc)_xfdashboard_tooltip_action_on_timeout,
														self);

	return(CLUTTER_EVENT_PROPAGATE);
}

/* Pointer left actor with tooltip */
static gboolean _xfdashboard_tooltip_action_on_leave_event(XfdashboardTooltipAction *self,
															ClutterEvent *inEvent,
															gpointer inUserData)
{
	XfdashboardTooltipActionPrivate		*priv;
	ClutterActor						*actor;

	g_return_val_if_fail(XFDASHBOARD_IS_TOOLTIP_ACTION(self), CLUTTER_EVENT_PROPAGATE);
	g_return_val_if_fail(CLUTTER_IS_ACTOR(inUserData), CLUTTER_EVENT_PROPAGATE);

	priv=self->priv;
	actor=CLUTTER_ACTOR(inUserData);

	/* Remove any timeout source we have added for this actor */
	if(priv->timeoutSourceID!=0)
	{
		g_source_remove(priv->timeoutSourceID);
		priv->timeoutSourceID=0;
	}

	/* Clear last actor we remembered if it is pointing to this actor */
	if(_xfdashboard_tooltip_last_event_actor==actor)
	{
		_xfdashboard_tooltip_last_event_actor=NULL;
	}

	return(CLUTTER_EVENT_PROPAGATE);
}

/* IMPLEMENTATION: ClutterActorMeta */

/* Called when attaching and detaching a ClutterActorMeta instance to a ClutterActor */
static void _xfdashboard_tooltip_action_set_actor(ClutterActorMeta *inActorMeta, ClutterActor *inActor)
{
	XfdashboardTooltipAction			*self;
	XfdashboardTooltipActionPrivate		*priv;
	ClutterActor						*oldActor;

	g_return_if_fail(XFDASHBOARD_IS_TOOLTIP_ACTION(inActorMeta));

	self=XFDASHBOARD_TOOLTIP_ACTION(inActorMeta);
	priv=self->priv;

	/* Get current referenced actor */
	oldActor=clutter_actor_meta_get_actor(inActorMeta);

	/* Disconnect signals and remove sources */
	if(priv->motionID!=0)
	{
		if(oldActor!=NULL) g_signal_handler_disconnect(oldActor, priv->motionID);
		priv->motionID=0;
	}

	if(priv->leaveID!=0)
	{
		if(oldActor!=NULL) g_signal_handler_disconnect(oldActor, priv->leaveID);
		priv->leaveID=0;
	}

	if(priv->timeoutSourceID!=0)
	{
		g_source_remove(priv->timeoutSourceID);
		priv->timeoutSourceID=0;
	}

	/* Connect signals */
	if(inActor!=NULL)
	{
		priv->motionID=g_signal_connect_swapped(inActor,
													"motion-event",
													G_CALLBACK(_xfdashboard_tooltip_action_on_motion_event),
													self);

		priv->leaveID=g_signal_connect_swapped(inActor,
													"leave-event",
													G_CALLBACK(_xfdashboard_tooltip_action_on_leave_event),
													self);
	}

	/* Call parent's class method */
	if(CLUTTER_ACTOR_META_CLASS(xfdashboard_tooltip_action_parent_class)->set_actor)
	{
		CLUTTER_ACTOR_META_CLASS(xfdashboard_tooltip_action_parent_class)->set_actor(inActorMeta, inActor);
	}
}

/* IMPLEMENTATION: GObject */

/* Dispose this object */
static void _xfdashboard_tooltip_action_dispose(GObject *inObject)
{
	XfdashboardTooltipAction			*self=XFDASHBOARD_TOOLTIP_ACTION(inObject);
	XfdashboardTooltipActionPrivate		*priv=self->priv;
	ClutterActor						*actor;

	/* Get actor this action belongs to */
	actor=clutter_actor_meta_get_actor(CLUTTER_ACTOR_META(inObject));

	/* Release allocated resources */
	if(priv->motionID!=0)
	{
		if(actor!=NULL) g_signal_handler_disconnect(actor, priv->motionID);
		priv->motionID=0;
	}

	if(priv->leaveID!=0)
	{
		if(actor!=NULL) g_signal_handler_disconnect(actor, priv->leaveID);
		priv->leaveID=0;
	}

	if(priv->timeoutSourceID!=0)
	{
		g_source_remove(priv->timeoutSourceID);
		priv->timeoutSourceID=0;
	}

	if(priv->tooltipText)
	{
		g_free(priv->tooltipText);
		priv->tooltipText=NULL;
	}

	/* Call parent's class dispose method */
	G_OBJECT_CLASS(xfdashboard_tooltip_action_parent_class)->dispose(inObject);
}

/* Set/get properties */
static void _xfdashboard_tooltip_action_set_property(GObject *inObject,
													guint inPropID,
													const GValue *inValue,
													GParamSpec *inSpec)
{
	XfdashboardTooltipAction			*self=XFDASHBOARD_TOOLTIP_ACTION(inObject);

	switch(inPropID)
	{
		case PROP_TOOLTIP_TEXT:
			xfdashboard_tooltip_action_set_text(self, g_value_get_string(inValue));
			break;

		default:
			G_OBJECT_WARN_INVALID_PROPERTY_ID(inObject, inPropID, inSpec);
			break;
	}
}

static void _xfdashboard_tooltip_action_get_property(GObject *inObject,
													guint inPropID,
													GValue *outValue,
													GParamSpec *inSpec)
{
	XfdashboardTooltipAction			*self=XFDASHBOARD_TOOLTIP_ACTION(inObject);
	XfdashboardTooltipActionPrivate		*priv=self->priv;

	switch(inPropID)
	{
		case PROP_TOOLTIP_TEXT:
			g_value_set_string(outValue, priv->tooltipText);
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
static void xfdashboard_tooltip_action_class_init(XfdashboardTooltipActionClass *klass)
{
	GObjectClass			*gobjectClass=G_OBJECT_CLASS(klass);
	ClutterActorMetaClass	*actorMetaClass=CLUTTER_ACTOR_META_CLASS(klass);

	/* Override functions */
	actorMetaClass->set_actor=_xfdashboard_tooltip_action_set_actor;

	gobjectClass->dispose=_xfdashboard_tooltip_action_dispose;
	gobjectClass->set_property=_xfdashboard_tooltip_action_set_property;
	gobjectClass->get_property=_xfdashboard_tooltip_action_get_property;

	/* Set up private structure */
	g_type_class_add_private(klass, sizeof (XfdashboardTooltipActionPrivate));

	/* Define properties */
	XfdashboardTooltipActionProperties[PROP_TOOLTIP_TEXT]=
		g_param_spec_string("tooltip-text",
								_("Tooltip text"),
								_("The text to display in a tooltip"),
								NULL,
								G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS);

	g_object_class_install_properties(gobjectClass, PROP_LAST, XfdashboardTooltipActionProperties);

	/* Define signals */
	XfdashboardTooltipActionSignals[SIGNAL_ACTIVATING]=
		g_signal_new("activating",
						G_TYPE_FROM_CLASS(klass),
						G_SIGNAL_RUN_LAST | G_SIGNAL_NO_HOOKS,
						G_STRUCT_OFFSET(XfdashboardTooltipActionClass, activating),
						NULL,
						NULL,
						g_cclosure_marshal_VOID__VOID,
						G_TYPE_NONE,
						0);
}

/* Object initialization
 * Create private structure and set up default values
 */
static void xfdashboard_tooltip_action_init(XfdashboardTooltipAction *self)
{
	XfdashboardTooltipActionPrivate		*priv;

	priv=self->priv=XFDASHBOARD_TOOLTIP_ACTION_GET_PRIVATE(self);

	/* Set up default values */
	priv->tooltipText=NULL;
	priv->motionID=0;
	priv->leaveID=0;
	priv->timeoutSourceID=0;
}

/* IMPLEMENTATION: Public API */

/* Create new action */
ClutterAction* xfdashboard_tooltip_action_new(void)
{
	return(CLUTTER_ACTION(g_object_new(XFDASHBOARD_TYPE_TOOLTIP_ACTION, NULL)));
}

/* Get/set text of tooltip */
const gchar* xfdashboard_tooltip_action_get_text(XfdashboardTooltipAction *self)
{
	g_return_val_if_fail(XFDASHBOARD_IS_TOOLTIP_ACTION(self), NULL);

	return(self->priv->tooltipText);
}

void xfdashboard_tooltip_action_set_text(XfdashboardTooltipAction *self, const gchar *inTooltipText)
{
	XfdashboardTooltipActionPrivate		*priv;

	g_return_if_fail(XFDASHBOARD_IS_TOOLTIP_ACTION(self));

	priv=self->priv;

	/* Set value if changed */
	if(g_strcmp0(priv->tooltipText, inTooltipText)!=0)
	{
		/* Set value */
		if(priv->tooltipText)
		{
			g_free(priv->tooltipText);
			priv->tooltipText=NULL;
		}

		if(inTooltipText) priv->tooltipText=g_strdup(inTooltipText);

		/* Notify about property change */
		g_object_notify_by_pspec(G_OBJECT(self), XfdashboardTooltipActionProperties[PROP_TOOLTIP_TEXT]);
	}
}

/* Get position relative to actor where last event happened */
void xfdashboard_tooltip_action_get_position(XfdashboardTooltipAction *self, gfloat *outX, gfloat *outY)
{
	XfdashboardTooltipActionPrivate		*priv;

	g_return_if_fail(XFDASHBOARD_IS_TOOLTIP_ACTION(self));

	priv=self->priv;

	/* Set position */
	if(outX) *outX=priv->lastPosition.x;
	if(outY) *outY=priv->lastPosition.y;
}
