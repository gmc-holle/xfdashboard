/*
 * action-button: A button representing an action to execute when clicked
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

#include "action-button.h"

#include <glib/gi18n-lib.h>

#include "focus-manager.h"

/* Define this class in GObject system */
G_DEFINE_TYPE(XfdashboardActionButton,
				xfdashboard_action_button,
				XFDASHBOARD_TYPE_BUTTON)

/* Private structure - access only by public API if needed */
#define XFDASHBOARD_ACTION_BUTTON_GET_PRIVATE(obj) \
	(G_TYPE_INSTANCE_GET_PRIVATE((obj), XFDASHBOARD_TYPE_ACTION_BUTTON, XfdashboardActionButtonPrivate))

struct _XfdashboardActionButtonPrivate
{
	/* Properties related */
	gchar								*target;
	gchar								*action;

	/* Instance related */
	XfdashboardFocusManager				*focusManager;
};

/* Properties */
enum
{
	PROP_0,

	PROP_TARGET,
	PROP_ACTION,

	PROP_LAST
};

static GParamSpec* XfdashboardActionButtonProperties[PROP_LAST]={ 0, };


/* IMPLEMENTATION: Private variables and methods */

/* This button was clicked */
static void _xfdashboard_action_button_clicked(XfdashboardButton *inButton)
{
	XfdashboardActionButton				*self;
	XfdashboardActionButtonPrivate		*priv;
	GSList								*targets;
	GSList								*iter;

	g_return_if_fail(XFDASHBOARD_IS_ACTION_BUTTON(inButton));

	self=XFDASHBOARD_ACTION_BUTTON(inButton);
	priv=self->priv;
	targets=NULL;

	/* Get target object to perform action at */
	targets=xfdashboard_focus_manager_get_targets(priv->focusManager, priv->target);
	g_debug("Target list for '%s' has %d entries",
				priv->target,
				g_slist_length(targets));

	/* Emit action at each actor in target list */
	for(iter=targets; iter; iter=g_slist_next(iter))
	{
		GObject							*targetObject;
		guint							signalID;
		GSignalQuery					signalData={ 0, };
		XfdashboardFocusable			*currentFocus;
		const ClutterEvent				*event;
		gboolean						eventStatus;

		/* Get target to emit action signal at */
		targetObject=G_OBJECT(iter->data);

		/* Check if target provides action requested as signal */
		signalID=g_signal_lookup(priv->action, G_OBJECT_TYPE(targetObject));
		if(!signalID)
		{
			g_warning(_("Object type %s does not provide action '%s'"),
						G_OBJECT_TYPE_NAME(targetObject),
						priv->action);
			continue;
		}

		/* Query signal for detailed data */
		g_signal_query(signalID, &signalData);

		/* Check if signal is an action signal */
		if(!(signalData.signal_flags & G_SIGNAL_ACTION))
		{
			g_warning(_("Action '%s' at object type %s is not an action signal."),
						priv->action,
						G_OBJECT_TYPE_NAME(targetObject));
			continue;
		}

		/* In debug mode also check if signal has right signature
		 * to be able to handle this action properly.
		 */
		if(signalID)
		{
			GType						returnValueType=G_TYPE_BOOLEAN;
			GType						parameterTypes[]={ XFDASHBOARD_TYPE_FOCUSABLE, G_TYPE_STRING, CLUTTER_TYPE_EVENT };
			guint						parameterCount;
			guint						i;

			/* Check if signal wants the right type of return value */
			if(signalData.return_type!=returnValueType)
			{
				g_critical(_("Action '%s' at object type %s wants return value of type %s but expected is %s."),
							priv->action,
							G_OBJECT_TYPE_NAME(targetObject),
							g_type_name(signalData.return_type),
							g_type_name(returnValueType));
			}

			/* Check if signals wants the right number and types of parameters */
			parameterCount=sizeof(parameterTypes)/sizeof(GType);
			if(signalData.n_params!=parameterCount)
			{
				g_critical(_("Action '%s' at object type %s wants %u parameters but expected are %u."),
							priv->action,
							G_OBJECT_TYPE_NAME(targetObject),
							signalData.n_params,
							parameterCount);
			}

			for(i=0; i<(parameterCount<signalData.n_params ? parameterCount : signalData.n_params); i++)
			{
				if(signalData.param_types[i]!=parameterTypes[i])
				{
					g_critical(_("Action '%s' at object type %s wants type %s at parameter %u but type %s is expected."),
								priv->action,
								G_OBJECT_TYPE_NAME(targetObject),
								g_type_name(signalData.param_types[i]),
								i+1,
								g_type_name(parameterTypes[i]));
				}
			}
		}

		/* Emit action signal at target */
		g_debug("Emitting action signal '%s' at actor %s",
					priv->action,
					G_OBJECT_TYPE_NAME(targetObject));

		currentFocus=xfdashboard_focus_manager_get_focus(priv->focusManager);
		event=clutter_get_current_event();
		eventStatus=CLUTTER_EVENT_PROPAGATE;
		g_signal_emit_by_name(targetObject, priv->action, currentFocus, priv->action, event, &eventStatus);

		g_debug("Action signal '%s' was %s by actor %s",
					priv->action,
					eventStatus==CLUTTER_EVENT_STOP ? "handled" : "not handled",
					G_OBJECT_TYPE_NAME(targetObject));
	}

	/* Release allocated resources */
	if(targets) g_slist_free_full(targets, g_object_unref);
}

/* IMPLEMENTATION: GObject */

/* Dispose this object */
static void _xfdashboard_action_button_dispose(GObject *inObject)
{
	XfdashboardActionButton				*self=XFDASHBOARD_ACTION_BUTTON(inObject);
	XfdashboardActionButtonPrivate		*priv=self->priv;

	/* Release our allocated variables */
	if(priv->focusManager)
	{
		g_object_unref(priv->focusManager);
		priv->focusManager=NULL;
	}

	if(priv->target)
	{
		g_free(priv->target);
		priv->target=NULL;
	}

	if(priv->action)
	{
		g_free(priv->action);
		priv->action=NULL;
	}

	/* Call parent's class dispose method */
	G_OBJECT_CLASS(xfdashboard_action_button_parent_class)->dispose(inObject);
}

/* Set/get properties */
static void _xfdashboard_action_button_set_property(GObject *inObject,
															guint inPropID,
															const GValue *inValue,
															GParamSpec *inSpec)
{
	XfdashboardActionButton			*self=XFDASHBOARD_ACTION_BUTTON(inObject);

	switch(inPropID)
	{
		case PROP_TARGET:
			xfdashboard_action_button_set_target(self, g_value_get_string(inValue));
			break;

		case PROP_ACTION:
			xfdashboard_action_button_set_action(self, g_value_get_string(inValue));
			break;

		default:
			G_OBJECT_WARN_INVALID_PROPERTY_ID(inObject, inPropID, inSpec);
			break;
	}
}

static void _xfdashboard_action_button_get_property(GObject *inObject,
															guint inPropID,
															GValue *outValue,
															GParamSpec *inSpec)
{
	XfdashboardActionButton			*self=XFDASHBOARD_ACTION_BUTTON(inObject);
	XfdashboardActionButtonPrivate		*priv=self->priv;

	switch(inPropID)
	{
		case PROP_TARGET:
			g_value_set_string(outValue, priv->target);
			break;

		case PROP_ACTION:
			g_value_set_string(outValue, priv->action);
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
static void xfdashboard_action_button_class_init(XfdashboardActionButtonClass *klass)
{
	XfdashboardButtonClass	*buttonClass=XFDASHBOARD_BUTTON_CLASS(klass);
	GObjectClass			*gobjectClass=G_OBJECT_CLASS(klass);

	/* Override functions */
	buttonClass->clicked=_xfdashboard_action_button_clicked;

	gobjectClass->dispose=_xfdashboard_action_button_dispose;
	gobjectClass->set_property=_xfdashboard_action_button_set_property;
	gobjectClass->get_property=_xfdashboard_action_button_get_property;

	/* Set up private structure */
	g_type_class_add_private(klass, sizeof(XfdashboardActionButtonPrivate));

	/* Define properties */
	XfdashboardActionButtonProperties[PROP_TARGET]=
		g_param_spec_string("target",
								_("Target"),
								_("The target actor's class name to lookup and to perform action at"),
								NULL,
								G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS);

	XfdashboardActionButtonProperties[PROP_ACTION]=
		g_param_spec_string("action",
								_("Action"),
								_("The action signal to perform at target"),
								NULL,
								G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS);

	g_object_class_install_properties(gobjectClass, PROP_LAST, XfdashboardActionButtonProperties);
}

/* Object initialization
 * Create private structure and set up default values
 */
static void xfdashboard_action_button_init(XfdashboardActionButton *self)
{
	XfdashboardActionButtonPrivate		*priv;

	priv=self->priv=XFDASHBOARD_ACTION_BUTTON_GET_PRIVATE(self);

	/* This actor is react on events */
	clutter_actor_set_reactive(CLUTTER_ACTOR(self), TRUE);

	/* Set up default values */
	priv->target=NULL;
	priv->action=FALSE;
	priv->focusManager=xfdashboard_focus_manager_get_default();
}

/* IMPLEMENTATION: Public API */

/* Create new actor */
ClutterActor* xfdashboard_action_button_new(void)
{
	return(g_object_new(XFDASHBOARD_TYPE_ACTION_BUTTON, NULL));
}

/* Get/set target to perform action at */
const gchar* xfdashboard_action_button_get_target(XfdashboardActionButton *self)
{
	g_return_val_if_fail(XFDASHBOARD_IS_ACTION_BUTTON(self), NULL);

	return(self->priv->target);
}

void xfdashboard_action_button_set_target(XfdashboardActionButton *self, const gchar *inTarget)
{
	XfdashboardActionButtonPrivate		*priv;

	g_return_if_fail(XFDASHBOARD_IS_ACTION_BUTTON(self));
	g_return_if_fail(inTarget);

	priv=self->priv;

	/* Set value if changed */
	if(g_strcmp0(priv->target, inTarget)!=0)
	{
		/* Set value */
		if(priv->target) g_free(priv->target);
		priv->target=g_strdup(inTarget);

		/* Notify about property change */
		g_object_notify_by_pspec(G_OBJECT(self), XfdashboardActionButtonProperties[PROP_TARGET]);
	}
}

/* Get/set action to perform at target */
const gchar* xfdashboard_action_button_get_action(XfdashboardActionButton *self)
{
	g_return_val_if_fail(XFDASHBOARD_IS_ACTION_BUTTON(self), NULL);

	return(self->priv->action);
}

void xfdashboard_action_button_set_action(XfdashboardActionButton *self, const gchar *inAction)
{
	XfdashboardActionButtonPrivate		*priv;

	g_return_if_fail(XFDASHBOARD_IS_ACTION_BUTTON(self));
	g_return_if_fail(inAction);

	priv=self->priv;

	/* Set value if changed */
	if(g_strcmp0(priv->action, inAction)!=0)
	{
		/* Set value */
		if(priv->action) g_free(priv->action);
		priv->action=g_strdup(inAction);

		/* Notify about property change */
		g_object_notify_by_pspec(G_OBJECT(self), XfdashboardActionButtonProperties[PROP_ACTION]);
	}
}
