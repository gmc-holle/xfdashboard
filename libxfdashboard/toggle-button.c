/*
 * toggle-button: A button which can toggle its state between on and off
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
 * SECTION:toggle-button
 * @short_description: A button which can toggle its state between on and off
 * @include: xfdashboard/toggle-button.h
 *
 * #XfdashboardToggleButton is a #XfdashboardButton which will remain in "pressed"
 * state when clicked. This is the "on" state. When it is clicked again it will
 * change its state back to normal state. This is the "off" state.
 *
 * A toggle button is created by calling either xfdashboard_toggle_button_new() or
 * any other xfdashboard_toggle_button_new_*(). These functions will create a toggle
 * button with state "off".
 *
 * The state of a #XfdashboardToggleButton can be set specifically using
 * xfdashboard_toggle_button_set_toggle_state() and retrieved using
 * xfdashboard_toggle_button_get_toggle_state().
 *
 * On creation the #XfdashboardToggleButton will be configured to change its state
 * automatically when clicked. This behaviour can be changed using
 * xfdashboard_toggle_button_set_auto_toggle() and retrieved using
 * xfdashboard_toggle_button_get_auto_toggle().
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <libxfdashboard/toggle-button.h>

#include <glib/gi18n-lib.h>

#include <libxfdashboard/stylable.h>
#include <libxfdashboard/compat.h>


/* Define this class in GObject system */
struct _XfdashboardToggleButtonPrivate
{
	/* Properties related */
	gboolean		toggleState;
	gboolean		autoToggleOnClick;
};

G_DEFINE_TYPE_WITH_PRIVATE(XfdashboardToggleButton,
							xfdashboard_toggle_button,
							XFDASHBOARD_TYPE_BUTTON)

/* Properties */
enum
{
	PROP_0,

	PROP_TOGGLE_STATE,
	PROP_AUTO_TOGGLE,

	PROP_LAST
};

static GParamSpec* XfdashboardToggleButtonProperties[PROP_LAST]={ 0, };

/* Signals */
enum
{
	SIGNAL_TOGGLED,

	SIGNAL_LAST
};

static guint XfdashboardToggleButtonSignals[SIGNAL_LAST]={ 0, };

/* IMPLEMENTATION: Private variables and methods */

/* Toggle button was clicked so toggle state */
static void _xfdashboard_toggle_button_clicked(XfdashboardButton *inButton)
{
	XfdashboardToggleButton			*self;
	XfdashboardToggleButtonPrivate	*priv;

	g_return_if_fail(XFDASHBOARD_IS_TOGGLE_BUTTON(inButton));

	self=XFDASHBOARD_TOGGLE_BUTTON(inButton);
	priv=self->priv;

	/* Call parent's class default click signal handler */
	if(XFDASHBOARD_BUTTON_CLASS(xfdashboard_toggle_button_parent_class)->clicked)
	{
		XFDASHBOARD_BUTTON_CLASS(xfdashboard_toggle_button_parent_class)->clicked(inButton);
	}

	/* Set new toggle state if "auto-toggle" (on click) is set */
	if(priv->autoToggleOnClick)
	{
		xfdashboard_toggle_button_set_toggle_state(self, !priv->toggleState);
	}
}

/* IMPLEMENTATION: GObject */

/* Set/get properties */
static void _xfdashboard_toggle_button_set_property(GObject *inObject,
													guint inPropID,
													const GValue *inValue,
													GParamSpec *inSpec)
{
	XfdashboardToggleButton			*self=XFDASHBOARD_TOGGLE_BUTTON(inObject);
	
	switch(inPropID)
	{
		case PROP_TOGGLE_STATE:
			xfdashboard_toggle_button_set_toggle_state(self, g_value_get_boolean(inValue));
			break;

		default:
			G_OBJECT_WARN_INVALID_PROPERTY_ID(inObject, inPropID, inSpec);
			break;
	}
}

static void _xfdashboard_toggle_button_get_property(GObject *inObject,
													guint inPropID,
													GValue *outValue,
													GParamSpec *inSpec)
{
	XfdashboardToggleButton			*self=XFDASHBOARD_TOGGLE_BUTTON(inObject);
	XfdashboardToggleButtonPrivate	*priv=self->priv;

	switch(inPropID)
	{
		case PROP_TOGGLE_STATE:
			g_value_set_boolean(outValue, priv->toggleState);
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
static void xfdashboard_toggle_button_class_init(XfdashboardToggleButtonClass *klass)
{
	XfdashboardButtonClass			*buttonClass=XFDASHBOARD_BUTTON_CLASS(klass);
	GObjectClass					*gobjectClass=G_OBJECT_CLASS(klass);

	/* Override functions */
	gobjectClass->set_property=_xfdashboard_toggle_button_set_property;
	gobjectClass->get_property=_xfdashboard_toggle_button_get_property;

	buttonClass->clicked=_xfdashboard_toggle_button_clicked;

	/* Define properties */
	/**
	 * XfdashboardToggleButton:toggle-state:
	 *
	 * A flag indicating if the state of toggle button. It is set to %TRUE if it
	 * is in "on" state that means it is pressed state and %FALSE if it is in
	 * "off" state that means it is not pressed.
	 */
	XfdashboardToggleButtonProperties[PROP_TOGGLE_STATE]=
		g_param_spec_boolean("toggle-state",
								_("Toggle state"),
								_("State of toggle"),
								FALSE,
								G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS);

	/**
	 * XfdashboardToggleButton:auto-toggle:
	 *
	 * A flag indicating if the state of toggle button should be changed between
	 * "on" and "off" state automatically if it was clicked.
	 */
	XfdashboardToggleButtonProperties[PROP_AUTO_TOGGLE]=
		g_param_spec_boolean("auto-toggle",
								_("Auto toggle"),
								_("If set the toggle state will be toggled on each click"),
								TRUE,
								G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS);

	g_object_class_install_properties(gobjectClass, PROP_LAST, XfdashboardToggleButtonProperties);

	/* Define signals */
	/**
	 * XfdashboardToggleButton::toggled:
	 * @self: The #XfdashboardToggleButton which changed its state
	 *
	 * Should be connected if you wish to perform an action whenever the
	 * #XfdashboardToggleButton's state has changed. 
	 *
	 * The state has to be retrieved using xfdashboard_toggle_button_get_toggle_state().
	 */
	XfdashboardToggleButtonSignals[SIGNAL_TOGGLED]=
		g_signal_new("toggled",
						G_TYPE_FROM_CLASS(klass),
						G_SIGNAL_RUN_LAST,
						G_STRUCT_OFFSET(XfdashboardToggleButtonClass, toggled),
						NULL,
						NULL,
						g_cclosure_marshal_VOID__VOID,
						G_TYPE_NONE,
						0);
}

/* Object initialization
 * Create private structure and set up default values
 */
static void xfdashboard_toggle_button_init(XfdashboardToggleButton *self)
{
	XfdashboardToggleButtonPrivate	*priv;

	priv=self->priv=xfdashboard_toggle_button_get_instance_private(self);

	/* This actor reacts on events */
	clutter_actor_set_reactive(CLUTTER_ACTOR(self), TRUE);

	/* Set up default values */
	priv->toggleState=FALSE;
	priv->autoToggleOnClick=TRUE;
}

/* IMPLEMENTATION: Public API */

/**
 * xfdashboard_toggle_button_new:
 *
 * Creates a new #XfdashboardToggleButton actor
 *
 * Return value: The newly created #XfdashboardToggleButton
 */
ClutterActor* xfdashboard_toggle_button_new(void)
{
	return(g_object_new(XFDASHBOARD_TYPE_TOGGLE_BUTTON,
						"text", N_(""),
						"label-style", XFDASHBOARD_LABEL_STYLE_TEXT,
						NULL));
}

/**
 * xfdashboard_toggle_button_new_with_text:
 * @inText: A string containing the text to be placed in the toggle button
 *
 * Creates a new #XfdashboardToggleButton actor with a text label.
 *
 * Return value: The newly created #XfdashboardToggleButton
 */
ClutterActor* xfdashboard_toggle_button_new_with_text(const gchar *inText)
{
	return(g_object_new(XFDASHBOARD_TYPE_TOGGLE_BUTTON,
						"text", inText,
						"label-style", XFDASHBOARD_LABEL_STYLE_TEXT,
						NULL));
}

/**
 * xfdashboard_toggle_button_new_with_icon_name:
 * @inIconName: A string containing the stock icon name or file name for the icon
 *   to be place in the toogle button
 *
 * Creates a new #XfdashboardToggleButton actor with an icon.
 *
 * Return value: The newly created #XfdashboardToggleButton
 */
ClutterActor* xfdashboard_toggle_button_new_with_icon_name(const gchar *inIconName)
{
	return(g_object_new(XFDASHBOARD_TYPE_TOGGLE_BUTTON,
						"icon-name", inIconName,
						"label-style", XFDASHBOARD_LABEL_STYLE_ICON,
						NULL));
}

/**
 * xfdashboard_toggle_button_new_with_gicon:
 * @inIcon: A #GIcon containing the icon image
 *
 * Creates a new #XfdashboardToggleButton actor with an icon.
 *
 * Return value: The newly created #XfdashboardToggleButton
 */
ClutterActor* xfdashboard_toggle_button_new_with_gicon(GIcon *inIcon)
{
	return(g_object_new(XFDASHBOARD_TYPE_TOGGLE_BUTTON,
						"icon-gicon", inIcon,
						"label-style", XFDASHBOARD_LABEL_STYLE_ICON,
						NULL));
}

/**
 * xfdashboard_toggle_button_new_full_with_icon_name:
 * @inIconName: A string containing the stock icon name or file name for the icon
 *   to be place in the toogle button
 * @inText: A string containing the text to be placed in the toggle button
 *
 * Creates a new #XfdashboardToggleButton actor with a text label and an icon.
 *
 * Return value: The newly created #XfdashboardToggleButton
 */
ClutterActor* xfdashboard_toggle_button_new_full_with_icon_name(const gchar *inIconName,
																const gchar *inText)
{
	return(g_object_new(XFDASHBOARD_TYPE_TOGGLE_BUTTON,
						"text", inText,
						"icon-name", inIconName,
						"label-style", XFDASHBOARD_LABEL_STYLE_BOTH,
						NULL));
}

/**
 * xfdashboard_toggle_button_new_full_with_gicon:
 * @inIcon: A #GIcon containing the icon image
 * @inText: A string containing the text to be placed in the toggle button
 *
 * Creates a new #XfdashboardToggleButton actor with a text label and an icon.
 *
 * Return value: The newly created #XfdashboardToggleButton
 */
ClutterActor* xfdashboard_toggle_button_new_full_with_gicon(GIcon *inIcon,
															const gchar *inText)
{
	return(g_object_new(XFDASHBOARD_TYPE_TOGGLE_BUTTON,
						"text", inText,
						"icon-gicon", inIcon,
						"label-style", XFDASHBOARD_LABEL_STYLE_BOTH,
						NULL));
}

/**
 * xfdashboard_toggle_button_get_toggle_state:
 * @self: A #XfdashboardToggleButton
 *
 * Retrieves the current state of @self.
 *
 * Return value: Returns %TRUE if the toggle button is pressed in ("on" state) and
 *   %FALSE if it is raised ("off" state). 
 */
gboolean xfdashboard_toggle_button_get_toggle_state(XfdashboardToggleButton *self)
{
	g_return_val_if_fail(XFDASHBOARD_IS_TOGGLE_BUTTON(self), 0);

	return(self->priv->toggleState);
}

/**
 * xfdashboard_toggle_button_set_toggle_state:
 * @self: A #XfdashboardToggleButton
 * @inToggleState: The state to set at @self
 *
 * Sets the state of @self. If @inToggleState is set to %TRUE then the toggle button
 * will set to and remain in pressed state ("on" state). If set to %FALSE then the
 * toggle button will raised ("off" state).
 */
void xfdashboard_toggle_button_set_toggle_state(XfdashboardToggleButton *self, gboolean inToggleState)
{
	XfdashboardToggleButtonPrivate	*priv;

	g_return_if_fail(XFDASHBOARD_IS_TOGGLE_BUTTON(self));

	priv=self->priv;

	/* Set value if changed */
	if(priv->toggleState!=inToggleState)
	{
		/* Set value */
		priv->toggleState=inToggleState;

		/* Set style classes */
		if(priv->toggleState) xfdashboard_stylable_add_pseudo_class(XFDASHBOARD_STYLABLE(self), "toggled");
			else xfdashboard_stylable_remove_pseudo_class(XFDASHBOARD_STYLABLE(self), "toggled");

		clutter_actor_queue_redraw(CLUTTER_ACTOR(self));

		/* Notify about property change */
		g_object_notify_by_pspec(G_OBJECT(self), XfdashboardToggleButtonProperties[PROP_TOGGLE_STATE]);

		/* Emit signal for change of toggle state */
		g_signal_emit(self, XfdashboardToggleButtonSignals[SIGNAL_TOGGLED], 0);
	}
}

/**
 * xfdashboard_toggle_button_get_auto_toggle:
 * @self: A #XfdashboardToggleButton
 *
 * Retrieves the automatic toggle mode of @self. If automatic toggle mode is %TRUE
 * then it is active and the toggle button changes its state automatically when
 * clicked.
 *
 * Return value: Returns %TRUE if automatic toggle mode is active, otherwise %FALSE.
 */
gboolean xfdashboard_toggle_button_get_auto_toggle(XfdashboardToggleButton *self)
{
	g_return_val_if_fail(XFDASHBOARD_IS_TOGGLE_BUTTON(self), 0);

	return(self->priv->autoToggleOnClick);
}

/**
 * xfdashboard_toggle_button_set_auto_toggle:
 * @self: A #XfdashboardToggleButton
 * @inAuto: The state to set at @self
 *
 * Sets the automatic toggle mode of @self. If @inAuto is set to %TRUE then the toggle
 * button will change its state automatically between pressed ("on") and raised ("off")
 * state when it is clicked. The "clicked" signal will be emitted before the toggle
 * changes its state. If @inAuto is set to %FALSE a signal handler for "clicked" signal
 * should be connected to handle the toggle state on your own.
 */
void xfdashboard_toggle_button_set_auto_toggle(XfdashboardToggleButton *self, gboolean inAuto)
{
	XfdashboardToggleButtonPrivate	*priv;

	g_return_if_fail(XFDASHBOARD_IS_TOGGLE_BUTTON(self));

	priv=self->priv;

	/* Set value if changed */
	if(priv->autoToggleOnClick!=inAuto)
	{
		/* Set value */
		priv->autoToggleOnClick=inAuto;

		/* Notify about property change */
		g_object_notify_by_pspec(G_OBJECT(self), XfdashboardToggleButtonProperties[PROP_AUTO_TOGGLE]);
	}
}

/**
 * xfdashboard_toggle_button_toggle:
 * @self: A #XfdashboardToggleButton
 *
 * Toggles the state of @self. That means that the toggle button will change its
 * state to pressed ("on" state) if it is currently raised ("off" state) or vice
 * versa.
 */
void xfdashboard_toggle_button_toggle(XfdashboardToggleButton *self)
{
	XfdashboardToggleButtonPrivate	*priv;

	g_return_if_fail(XFDASHBOARD_IS_TOGGLE_BUTTON(self));

	priv=self->priv;

	/* Set opposite state of current one */
	xfdashboard_toggle_button_set_toggle_state(self, !priv->toggleState);
}
