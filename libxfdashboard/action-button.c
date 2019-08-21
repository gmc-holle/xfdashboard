/*
 * action-button: A button representing an action to execute when clicked
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

/**
 * SECTION:action-button
 * @short_description: A button to perform a key binding action
 * @include: xfdashboard/action-button.h
 *
 * This actor is a #XfdashboardButton and behaves exactly like a key binding which
 * performs a specified action on a specific actor when the associated key
 * combination is pressed. But instead of a key combination a button is displayed
 * and the action performed when this button is clicked.
 *
 * A #XfdashboardActionButton is usually created in the layout definition
 * of a theme but it can also be created with xfdashboard_action_button_new()
 * followed by a call to xfdashboard_action_button_set_target() and
 * xfdashboard_action_button_set_action() to configure it.
 *
 * For example a #XfdashboardActionButton can be created which will quit the
 * application when clicked:
 *
 * |[<!-- language="C" -->
 *   ClutterActor       *actionButton;
 *
 *   actionButton=xfdashboard_action_button_new();
 *   xfdashboard_action_button_set_target(XFDASHBOARD_ACTION_BUTTON(actionButton), "XfdashboardApplication");
 *   xfdashboard_action_button_set_action(XFDASHBOARD_ACTION_BUTTON(actionButton), "exit");
 * ]|
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <libxfdashboard/action-button.h>

#include <glib/gi18n-lib.h>

#include <libxfdashboard/focusable.h>
#include <libxfdashboard/focus-manager.h>
#include <libxfdashboard/compat.h>
#include <libxfdashboard/debug.h>


/* Define this class in GObject system */
static void _xfdashboard_action_button_focusable_iface_init(XfdashboardFocusableInterface *iface);

struct _XfdashboardActionButtonPrivate
{
	/* Properties related */
	gchar								*target;
	gchar								*action;

	/* Instance related */
	XfdashboardFocusManager				*focusManager;
};

G_DEFINE_TYPE_WITH_CODE(XfdashboardActionButton,
						xfdashboard_action_button,
						XFDASHBOARD_TYPE_BUTTON,
						G_ADD_PRIVATE(XfdashboardActionButton)
						G_IMPLEMENT_INTERFACE(XFDASHBOARD_TYPE_FOCUSABLE, _xfdashboard_action_button_focusable_iface_init))

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
	XFDASHBOARD_DEBUG(self, ACTOR, "Target list for '%s' has %d entries",
						priv->target,
						g_slist_length(targets));

	/* Emit action at each actor in target list */
	for(iter=targets; iter; iter=g_slist_next(iter))
	{
		GObject							*targetObject;
		guint							signalID;
		GSignalQuery					signalData={ 0, };
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
		XFDASHBOARD_DEBUG(self, ACTOR, "Emitting action signal '%s' at actor %s",
							priv->action,
							G_OBJECT_TYPE_NAME(targetObject));

		event=clutter_get_current_event();
		eventStatus=CLUTTER_EVENT_PROPAGATE;
		g_signal_emit_by_name(targetObject,
								priv->action,
								XFDASHBOARD_FOCUSABLE(self),
								priv->action,
								event,
								&eventStatus);

		XFDASHBOARD_DEBUG(self, ACTOR, "Action signal '%s' was %s by actor %s",
							priv->action,
							eventStatus==CLUTTER_EVENT_STOP ? "handled" : "not handled",
							G_OBJECT_TYPE_NAME(targetObject));
	}

	/* Release allocated resources */
	if(targets) g_slist_free_full(targets, g_object_unref);
}

/* IMPLEMENTATION: Interface XfdashboardFocusable */

/* Determine if actor can get the focus */
static gboolean _xfdashboard_action_button_focusable_can_focus(XfdashboardFocusable *inFocusable)
{
	XfdashboardFocusableInterface		*selfIface;
	XfdashboardFocusableInterface		*parentIface;

	g_return_val_if_fail(XFDASHBOARD_IS_FOCUSABLE(inFocusable), FALSE);
	g_return_val_if_fail(XFDASHBOARD_IS_ACTION_BUTTON(inFocusable), FALSE);

	/* Call parent class interface function */
	selfIface=XFDASHBOARD_FOCUSABLE_GET_IFACE(inFocusable);
	parentIface=g_type_interface_peek_parent(selfIface);

	if(parentIface && parentIface->can_focus)
	{
		if(!parentIface->can_focus(inFocusable)) return(FALSE);
	}

	/* If we get here this actor can be focused */
	return(TRUE);
}

/* Determine if this actor supports selection */
static gboolean _xfdashboard_action_button_focusable_supports_selection(XfdashboardFocusable *inFocusable)
{
	g_return_val_if_fail(XFDASHBOARD_IS_FOCUSABLE(inFocusable), FALSE);
	g_return_val_if_fail(XFDASHBOARD_IS_ACTION_BUTTON(inFocusable), FALSE);

	/* This actor supports selection */
	return(TRUE);
}

/* Get current selection */
static ClutterActor* _xfdashboard_action_button_focusable_get_selection(XfdashboardFocusable *inFocusable)
{
	XfdashboardActionButton		*self;

	g_return_val_if_fail(XFDASHBOARD_IS_FOCUSABLE(inFocusable), NULL);
	g_return_val_if_fail(XFDASHBOARD_IS_ACTION_BUTTON(inFocusable), NULL);

	self=XFDASHBOARD_ACTION_BUTTON(inFocusable);

	/* Return the actor itself as current selection */
	return(CLUTTER_ACTOR(self));
}

/* Set new selection */
static gboolean _xfdashboard_action_button_focusable_set_selection(XfdashboardFocusable *inFocusable,
																	ClutterActor *inSelection)
{
	XfdashboardActionButton				*self;

	g_return_val_if_fail(XFDASHBOARD_IS_FOCUSABLE(inFocusable), FALSE);
	g_return_val_if_fail(XFDASHBOARD_IS_ACTION_BUTTON(inFocusable), FALSE);
	g_return_val_if_fail(!inSelection || CLUTTER_IS_ACTOR(inSelection), FALSE);

	self=XFDASHBOARD_ACTION_BUTTON(inFocusable);

	/* Setting new selection always fails if it is not this actor itself */
	if(inSelection!=CLUTTER_ACTOR(self)) return(FALSE);

	/* Otherwise setting selection was successful because nothing has changed */
	return(TRUE);
}

/* Find requested selection target depending of current selection */
static ClutterActor* _xfdashboard_action_button_focusable_find_selection(XfdashboardFocusable *inFocusable,
																			ClutterActor *inSelection,
																			XfdashboardSelectionTarget inDirection)
{
	XfdashboardActionButton				*self;

	g_return_val_if_fail(XFDASHBOARD_IS_FOCUSABLE(inFocusable), NULL);
	g_return_val_if_fail(XFDASHBOARD_IS_ACTION_BUTTON(inFocusable), NULL);
	g_return_val_if_fail(!inSelection || CLUTTER_IS_ACTOR(inSelection), NULL);
	g_return_val_if_fail(inDirection>=0 && inDirection<=XFDASHBOARD_SELECTION_TARGET_NEXT, NULL);

	self=XFDASHBOARD_ACTION_BUTTON(inFocusable);

	/* Regardless of "current" selection and direction requested for new selection
	 * we return this actor as new current selection resulting in no change of
	 * selection. It is and will be the actor itself.
	 */
	return(CLUTTER_ACTOR(self));
}

/* Activate selection */
static gboolean _xfdashboard_action_button_focusable_activate_selection(XfdashboardFocusable *inFocusable,
																		ClutterActor *inSelection)
{
	XfdashboardActionButton				*self;

	g_return_val_if_fail(XFDASHBOARD_IS_FOCUSABLE(inFocusable), FALSE);
	g_return_val_if_fail(XFDASHBOARD_IS_ACTION_BUTTON(inFocusable), FALSE);
	g_return_val_if_fail(CLUTTER_IS_ACTOR(inSelection), FALSE);

	self=XFDASHBOARD_ACTION_BUTTON(inFocusable);

	/* Activate selection */
	_xfdashboard_action_button_clicked(XFDASHBOARD_BUTTON(self));

	return(TRUE);
}

/* Interface initialization
 * Set up default functions
 */
void _xfdashboard_action_button_focusable_iface_init(XfdashboardFocusableInterface *iface)
{
	iface->can_focus=_xfdashboard_action_button_focusable_can_focus;

	iface->supports_selection=_xfdashboard_action_button_focusable_supports_selection;
	iface->get_selection=_xfdashboard_action_button_focusable_get_selection;
	iface->set_selection=_xfdashboard_action_button_focusable_set_selection;
	iface->find_selection=_xfdashboard_action_button_focusable_find_selection;
	iface->activate_selection=_xfdashboard_action_button_focusable_activate_selection;
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

	/* Define properties */
	/**
	 * XfdashboardActionButton:target:
	 *
	 * A string with the class name of target at which the action should be
	 * performed.
	 */
	XfdashboardActionButtonProperties[PROP_TARGET]=
		g_param_spec_string("target",
								_("Target"),
								_("The target actor's class name to lookup and to perform action at"),
								NULL,
								G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS);

	/**
	 * XfdashboardActionButton:action:
	 *
	 * A string with the signal action name to perform at target.
	 */
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

	priv=self->priv=xfdashboard_action_button_get_instance_private(self);

	/* This actor is react on events */
	clutter_actor_set_reactive(CLUTTER_ACTOR(self), TRUE);

	/* Set up default values */
	priv->target=NULL;
	priv->action=FALSE;
	priv->focusManager=xfdashboard_focus_manager_get_default();
}

/* IMPLEMENTATION: Public API */

/**
 * xfdashboard_action_button_new:
 *
 * Creates a new #XfdashboardActionButton actor
 *
 * Return value: The newly created #XfdashboardActionButton
 */
ClutterActor* xfdashboard_action_button_new(void)
{
	return(g_object_new(XFDASHBOARD_TYPE_ACTION_BUTTON, NULL));
}

/**
 * xfdashboard_action_button_get_target:
 * @self: A #XfdashboardActionButton
 *
 * Retrieves the target's class name of @self at which the action should be
 * performed.
 *
 * Return value: A string with target's class name
 */
const gchar* xfdashboard_action_button_get_target(XfdashboardActionButton *self)
{
	g_return_val_if_fail(XFDASHBOARD_IS_ACTION_BUTTON(self), NULL);

	return(self->priv->target);
}

/**
 * xfdashboard_action_button_set_target:
 * @self: A #XfdashboardActionButton
 * @inTarget: The target's class name
 *
 * Sets the target's class name at @self at which the action should be
 * performed by this actor.
 */
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

/**
 * xfdashboard_action_button_get_action:
 * @self: A #XfdashboardActionButton
 *
 * Retrieves the action's signal name of @self which will be performed at target.
 *
 * Return value: A string with action's signal name
 */
const gchar* xfdashboard_action_button_get_action(XfdashboardActionButton *self)
{
	g_return_val_if_fail(XFDASHBOARD_IS_ACTION_BUTTON(self), NULL);

	return(self->priv->action);
}

/**
 * xfdashboard_action_button_set_action:
 * @self: A #XfdashboardActionButton
 * @inAction: The action's signal name
 *
 * Sets the action's signal name at @self which will be performed at target.
 */
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
