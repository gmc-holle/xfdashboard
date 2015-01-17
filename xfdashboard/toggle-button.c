/*
 * toggle-button: A button which can toggle its state between on and off
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

#include "toggle-button.h"

#include <glib/gi18n-lib.h>

#include "stylable.h"

/* Define this class in GObject system */
G_DEFINE_TYPE(XfdashboardToggleButton,
				xfdashboard_toggle_button,
				XFDASHBOARD_TYPE_BUTTON)

/* Private structure - access only by public API if needed */
#define XFDASHBOARD_TOGGLE_BUTTON_GET_PRIVATE(obj) \
	(G_TYPE_INSTANCE_GET_PRIVATE((obj), XFDASHBOARD_TYPE_TOGGLE_BUTTON, XfdashboardToggleButtonPrivate))

struct _XfdashboardToggleButtonPrivate
{
	/* Properties related */
	gboolean		toggleState;
	gboolean		autoToggleOnClick;
};

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

	/* Set up private structure */
	g_type_class_add_private(klass, sizeof(XfdashboardToggleButtonPrivate));

	/* Define properties */
	XfdashboardToggleButtonProperties[PROP_TOGGLE_STATE]=
		g_param_spec_boolean("toggle-state",
								_("Toggle state"),
								_("State of toggle"),
								FALSE,
								G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS);

	XfdashboardToggleButtonProperties[PROP_AUTO_TOGGLE]=
		g_param_spec_boolean("auto-toggle",
								_("Auto toggle"),
								_("If set the toggle state will be toggled on each click"),
								TRUE,
								G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS);

	g_object_class_install_properties(gobjectClass, PROP_LAST, XfdashboardToggleButtonProperties);

	/* Define signals */
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

	priv=self->priv=XFDASHBOARD_TOGGLE_BUTTON_GET_PRIVATE(self);

	/* This actor reacts on events */
	clutter_actor_set_reactive(CLUTTER_ACTOR(self), TRUE);

	/* Set up default values */
	priv->toggleState=FALSE;
	priv->autoToggleOnClick=TRUE;
}

/* IMPLEMENTATION: Public API */

/* Create new actor */
ClutterActor* xfdashboard_toggle_button_new(void)
{
	return(g_object_new(XFDASHBOARD_TYPE_TOGGLE_BUTTON,
						"text", N_(""),
						"button-style", XFDASHBOARD_STYLE_TEXT,
						NULL));
}

ClutterActor* xfdashboard_toggle_button_new_with_text(const gchar *inText)
{
	return(g_object_new(XFDASHBOARD_TYPE_TOGGLE_BUTTON,
						"text", inText,
						"button-style", XFDASHBOARD_STYLE_TEXT,
						NULL));
}

ClutterActor* xfdashboard_toggle_button_new_with_icon(const gchar *inIconName)
{
	return(g_object_new(XFDASHBOARD_TYPE_TOGGLE_BUTTON,
						"icon-name", inIconName,
						"button-style", XFDASHBOARD_STYLE_ICON,
						NULL));
}

ClutterActor* xfdashboard_toggle_button_new_full(const gchar *inIconName, const gchar *inText)
{
	return(g_object_new(XFDASHBOARD_TYPE_TOGGLE_BUTTON,
						"text", inText,
						"icon-name", inIconName,
						"button-style", XFDASHBOARD_STYLE_BOTH,
						NULL));
}

/* Get/set toggle state */
gboolean xfdashboard_toggle_button_get_toggle_state(XfdashboardToggleButton *self)
{
	g_return_val_if_fail(XFDASHBOARD_IS_TOGGLE_BUTTON(self), 0);

	return(self->priv->toggleState);
}

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

/* Get/set auto-toggle (on click) */
gboolean xfdashboard_toggle_button_get_auto_toggle(XfdashboardToggleButton *self)
{
	g_return_val_if_fail(XFDASHBOARD_IS_TOGGLE_BUTTON(self), 0);

	return(self->priv->autoToggleOnClick);
}

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
