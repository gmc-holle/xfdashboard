/*
 * button: A label actor which can react on click actions
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

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <libxfdashboard/button.h>

#include <glib/gi18n-lib.h>
#include <gdk/gdk.h>
#include <math.h>

#include <libxfdashboard/click-action.h>
#include <libxfdashboard/compat.h>


/* Define this class in GObject system */
struct _XfdashboardButtonPrivate
{
	/* Instance related */
	ClutterAction				*clickAction;
};

G_DEFINE_TYPE_WITH_PRIVATE(XfdashboardButton,
							xfdashboard_button,
							XFDASHBOARD_TYPE_LABEL)

/* Properties */
enum
{
	PROP_0,

	PROP_STYLE,

	PROP_LAST
};

static GParamSpec* XfdashboardButtonProperties[PROP_LAST]={ 0, };

/* Signals */
enum
{
	SIGNAL_CLICKED,

	SIGNAL_LAST
};

static guint XfdashboardButtonSignals[SIGNAL_LAST]={ 0, };

/* IMPLEMENTATION: Private variables and methods */

/* Proxy ClickAction signals */
static void _xfdashboard_button_clicked(XfdashboardClickAction *inAction,
										ClutterActor *self,
										gpointer inUserData)
{
	g_return_if_fail(XFDASHBOARD_IS_CLICK_ACTION(inAction));
	g_return_if_fail(XFDASHBOARD_IS_BUTTON(self));

	/* Only emit any of these signals if click was perform with left button 
	 * or is a short touchscreen touch event.
	 */
	if(xfdashboard_click_action_is_left_button_or_tap(inAction))
	{
		/* Emit 'clicked' signal */
		g_signal_emit(self, XfdashboardButtonSignals[SIGNAL_CLICKED], 0);
	}
}

/* IMPLEMENTATION: GObject */

/* Set/get properties */
static void _xfdashboard_button_set_property(GObject *inObject,
												guint inPropID,
												const GValue *inValue,
												GParamSpec *inSpec)
{
	XfdashboardButton			*self=XFDASHBOARD_BUTTON(inObject);

	switch(inPropID)
	{
		case PROP_STYLE:
			xfdashboard_label_set_style(XFDASHBOARD_LABEL(self), g_value_get_enum(inValue));
			break;

		default:
			G_OBJECT_WARN_INVALID_PROPERTY_ID(inObject, inPropID, inSpec);
			break;
	}
}

static void _xfdashboard_button_get_property(GObject *inObject,
												guint inPropID,
												GValue *outValue,
												GParamSpec *inSpec)
{
	XfdashboardButton			*self=XFDASHBOARD_BUTTON(inObject);

	switch(inPropID)
	{
		case PROP_STYLE:
			g_value_set_enum(outValue, xfdashboard_label_get_style(XFDASHBOARD_LABEL(self)));
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
static void xfdashboard_button_class_init(XfdashboardButtonClass *klass)
{
	XfdashboardActorClass	*actorClass=XFDASHBOARD_ACTOR_CLASS(klass);
	GObjectClass			*gobjectClass=G_OBJECT_CLASS(klass);

	/* Override functions */
	gobjectClass->set_property=_xfdashboard_button_set_property;
	gobjectClass->get_property=_xfdashboard_button_get_property;

	/* Define properties */
	XfdashboardButtonProperties[PROP_STYLE]=
		g_param_spec_override("button-style",
								g_object_class_find_property(gobjectClass, "label-style"));

	g_object_class_install_properties(gobjectClass, PROP_LAST, XfdashboardButtonProperties);

	/* Define stylable properties */
	xfdashboard_actor_install_stylable_property(actorClass, XfdashboardButtonProperties[PROP_STYLE]);

	/* Define signals */
	XfdashboardButtonSignals[SIGNAL_CLICKED]=
		g_signal_new("clicked",
						G_TYPE_FROM_CLASS(klass),
						G_SIGNAL_RUN_LAST,
						G_STRUCT_OFFSET(XfdashboardButtonClass, clicked),
						NULL,
						NULL,
						g_cclosure_marshal_VOID__VOID,
						G_TYPE_NONE,
						0);
}

/* Object initialization
 * Create private structure and set up default values
 */
static void xfdashboard_button_init(XfdashboardButton *self)
{
	XfdashboardButtonPrivate	*priv;

	priv=self->priv=xfdashboard_button_get_instance_private(self);

	/* This actor reacts on events */
	clutter_actor_set_reactive(CLUTTER_ACTOR(self), TRUE);

	/* Connect signals */
	priv->clickAction=xfdashboard_click_action_new();
	clutter_actor_add_action(CLUTTER_ACTOR(self), priv->clickAction);
	g_signal_connect(priv->clickAction, "clicked", G_CALLBACK(_xfdashboard_button_clicked), NULL);
}

/* IMPLEMENTATION: Public API */

/* Create new actor */
ClutterActor* xfdashboard_button_new(void)
{
	return(g_object_new(XFDASHBOARD_TYPE_BUTTON,
						"text", N_(""),
						"label-style", XFDASHBOARD_LABEL_STYLE_TEXT,
						NULL));
}

ClutterActor* xfdashboard_button_new_with_text(const gchar *inText)
{
	return(g_object_new(XFDASHBOARD_TYPE_BUTTON,
						"text", inText,
						"label-style", XFDASHBOARD_LABEL_STYLE_TEXT,
						NULL));
}

ClutterActor* xfdashboard_button_new_with_icon_name(const gchar *inIconName)
{
	return(g_object_new(XFDASHBOARD_TYPE_BUTTON,
						"icon-name", inIconName,
						"label-style", XFDASHBOARD_LABEL_STYLE_ICON,
						NULL));
}

ClutterActor* xfdashboard_button_new_with_gicon(GIcon *inIcon)
{
	return(g_object_new(XFDASHBOARD_TYPE_BUTTON,
						"icon-gicon", inIcon,
						"label-style", XFDASHBOARD_LABEL_STYLE_ICON,
						NULL));
}

ClutterActor* xfdashboard_button_new_full_with_icon_name(const gchar *inIconName, const gchar *inText)
{
	return(g_object_new(XFDASHBOARD_TYPE_BUTTON,
						"text", inText,
						"icon-name", inIconName,
						"label-style", XFDASHBOARD_LABEL_STYLE_BOTH,
						NULL));
}

ClutterActor* xfdashboard_button_new_full_with_gicon(GIcon *inIcon, const gchar *inText)
{
	return(g_object_new(XFDASHBOARD_TYPE_BUTTON,
						"text", inText,
						"icon-gicon", inIcon,
						"label-style", XFDASHBOARD_LABEL_STYLE_BOTH,
						NULL));
}
