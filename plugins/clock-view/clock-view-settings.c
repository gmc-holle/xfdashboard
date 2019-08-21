/*
 * clock-view-settings: Shared object instance holding settings for plugin
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

#include "clock-view-settings.h"

#include <libxfdashboard/libxfdashboard.h>
#include <glib/gi18n-lib.h>
#include <gtk/gtk.h>
#include <math.h>


/* Define this class in GObject system */
struct _XfdashboardClockViewSettingsPrivate
{
	/* Properties related */
	ClutterColor			*hourColor;
	ClutterColor			*minuteColor;
	ClutterColor			*secondColor;
	ClutterColor			*backgroundColor;

	/* Instance related */
	XfconfChannel			*xfconfChannel;
	guint					xfconfHourColorBindingID;
	guint					xfconfMinuteColorBindingID;
	guint					xfconfSecondColorBindingID;
	guint					xfconfBackgroundColorBindingID;
};

G_DEFINE_DYNAMIC_TYPE_EXTENDED(XfdashboardClockViewSettings,
								xfdashboard_clock_view_settings,
								G_TYPE_OBJECT,
								0,
								G_ADD_PRIVATE_DYNAMIC(XfdashboardClockViewSettings))

/* Define this class in this plugin */
XFDASHBOARD_DEFINE_PLUGIN_TYPE(xfdashboard_clock_view_settings);

/* Properties */
enum
{
	PROP_0,

	PROP_HOUR_COLOR,
	PROP_MINUTE_COLOR,
	PROP_SECOND_COLOR,
	PROP_BACKGROUOND_COLOR,

	PROP_LAST
};

static GParamSpec* XfdashboardClockViewSettingsProperties[PROP_LAST]={ 0, };


/* IMPLEMENTATION: Private variables and methods */

#define XFDASHBOARD_XFCONF_CHANNEL		"xfdashboard"

#define COLOR_HOUR_XFCONF_PROP			"/plugins/"PLUGIN_ID"/hour-color"
#define COLOR_MINUTE_XFCONF_PROP		"/plugins/"PLUGIN_ID"/minute-color"
#define COLOR_SECOND_XFCONF_PROP		"/plugins/"PLUGIN_ID"/second-color"
#define COLOR_BACKGROUND_XFCONF_PROP	"/plugins/"PLUGIN_ID"/background-color"


/* IMPLEMENTATION: GObject */

/* Dispose this object */
static void _xfdashboard_clock_view_settings_dispose(GObject *inObject)
{
	XfdashboardClockViewSettings			*self=XFDASHBOARD_CLOCK_VIEW_SETTINGS(inObject);
	XfdashboardClockViewSettingsPrivate		*priv=self->priv;

	/* Release allocated resources */
	if(priv->xfconfHourColorBindingID)
	{
		xfconf_g_property_unbind(priv->xfconfHourColorBindingID);
		priv->xfconfHourColorBindingID=0;
	}

	if(priv->xfconfMinuteColorBindingID)
	{
		xfconf_g_property_unbind(priv->xfconfMinuteColorBindingID);
		priv->xfconfMinuteColorBindingID=0;
	}

	if(priv->xfconfSecondColorBindingID)
	{
		xfconf_g_property_unbind(priv->xfconfSecondColorBindingID);
		priv->xfconfSecondColorBindingID=0;
	}

	if(priv->xfconfBackgroundColorBindingID)
	{
		xfconf_g_property_unbind(priv->xfconfBackgroundColorBindingID);
		priv->xfconfBackgroundColorBindingID=0;
	}

	if(priv->xfconfChannel)
	{
		priv->xfconfChannel=NULL;
	}

	if(priv->hourColor)
	{
		clutter_color_free(priv->hourColor);
		priv->hourColor=NULL;
	}

	if(priv->minuteColor)
	{
		clutter_color_free(priv->minuteColor);
		priv->minuteColor=NULL;
	}

	if(priv->secondColor)
	{
		clutter_color_free(priv->secondColor);
		priv->secondColor=NULL;
	}

	/* Call parent's class dispose method */
	G_OBJECT_CLASS(xfdashboard_clock_view_settings_parent_class)->dispose(inObject);
}

/* Set/get properties */
static void _xfdashboard_clock_view_settings_set_property(GObject *inObject,
															guint inPropID,
															const GValue *inValue,
															GParamSpec *inSpec)
{
	XfdashboardClockViewSettings			*self=XFDASHBOARD_CLOCK_VIEW_SETTINGS(inObject);

	switch(inPropID)
	{
		case PROP_HOUR_COLOR:
			xfdashboard_clock_view_settings_set_hour_color(self, clutter_value_get_color(inValue));
			break;

		case PROP_MINUTE_COLOR:
			xfdashboard_clock_view_settings_set_minute_color(self, clutter_value_get_color(inValue));
			break;

		case PROP_SECOND_COLOR:
			xfdashboard_clock_view_settings_set_second_color(self, clutter_value_get_color(inValue));
			break;

		case PROP_BACKGROUOND_COLOR:
			xfdashboard_clock_view_settings_set_background_color(self, clutter_value_get_color(inValue));
			break;

		default:
			G_OBJECT_WARN_INVALID_PROPERTY_ID(inObject, inPropID, inSpec);
			break;
	}
}

static void _xfdashboard_clock_view_settings_get_property(GObject *inObject,
															guint inPropID,
															GValue *outValue,
															GParamSpec *inSpec)
{
	XfdashboardClockViewSettings			*self=XFDASHBOARD_CLOCK_VIEW_SETTINGS(inObject);
	XfdashboardClockViewSettingsPrivate		*priv=self->priv;

	switch(inPropID)
	{
		case PROP_HOUR_COLOR:
			clutter_value_set_color(outValue, priv->hourColor);
			break;

		case PROP_MINUTE_COLOR:
			clutter_value_set_color(outValue, priv->minuteColor);
			break;

		case PROP_SECOND_COLOR:
			clutter_value_set_color(outValue, priv->secondColor);
			break;

		case PROP_BACKGROUOND_COLOR:
			clutter_value_set_color(outValue, priv->backgroundColor);
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
void xfdashboard_clock_view_settings_class_init(XfdashboardClockViewSettingsClass *klass)
{
	GObjectClass			*gobjectClass=G_OBJECT_CLASS(klass);

	/* Override functions */
	gobjectClass->dispose=_xfdashboard_clock_view_settings_dispose;
	gobjectClass->set_property=_xfdashboard_clock_view_settings_set_property;
	gobjectClass->get_property=_xfdashboard_clock_view_settings_get_property;

	/* Define properties */
	XfdashboardClockViewSettingsProperties[PROP_HOUR_COLOR]=
		clutter_param_spec_color("hour-color",
									_("Hour color"),
									_("Color to draw the hour hand with"),
									CLUTTER_COLOR_LightChameleon,
									G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS);

	XfdashboardClockViewSettingsProperties[PROP_MINUTE_COLOR]=
		clutter_param_spec_color("minute-color",
									_("Minute color"),
									_("Color to draw the minute hand with"),
									CLUTTER_COLOR_LightChameleon,
									G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS);

	XfdashboardClockViewSettingsProperties[PROP_SECOND_COLOR]=
		clutter_param_spec_color("second-color",
									_("Second color"),
									_("Color to draw the second hand with"),
									CLUTTER_COLOR_White,
									G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS);

	XfdashboardClockViewSettingsProperties[PROP_BACKGROUOND_COLOR]=
		clutter_param_spec_color("background-color",
									_("Background color"),
									_("Color to draw the circle with that holds the second hand"),
									CLUTTER_COLOR_Blue,
									G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS);

	g_object_class_install_properties(gobjectClass, PROP_LAST, XfdashboardClockViewSettingsProperties);
}

/* Class finalization */
void xfdashboard_clock_view_settings_class_finalize(XfdashboardClockViewSettingsClass *klass)
{
}

/* Object initialization
 * Create private structure and set up default values
 */
void xfdashboard_clock_view_settings_init(XfdashboardClockViewSettings *self)
{
	XfdashboardClockViewSettingsPrivate		*priv;

	self->priv=priv=xfdashboard_clock_view_settings_get_instance_private(self);

	/* Set up default values */
	priv->hourColor=clutter_color_copy(CLUTTER_COLOR_LightChameleon);
	priv->minuteColor=clutter_color_copy(CLUTTER_COLOR_LightChameleon);
	priv->secondColor=clutter_color_copy(CLUTTER_COLOR_White);
	priv->backgroundColor=clutter_color_copy(CLUTTER_COLOR_Blue);
	priv->xfconfChannel=xfconf_channel_get(XFDASHBOARD_XFCONF_CHANNEL);

	/* Bind to xfconf to react on changes */
	priv->xfconfHourColorBindingID=
		xfconf_g_property_bind(priv->xfconfChannel,
								COLOR_HOUR_XFCONF_PROP,
								G_TYPE_STRING,
								self,
								"hour-color");

	priv->xfconfMinuteColorBindingID=
		xfconf_g_property_bind(priv->xfconfChannel,
								COLOR_MINUTE_XFCONF_PROP,
								G_TYPE_STRING,
								self,
								"minute-color");

	priv->xfconfSecondColorBindingID=
		xfconf_g_property_bind(priv->xfconfChannel,
								COLOR_SECOND_XFCONF_PROP,
								G_TYPE_STRING,
								self,
								"second-color");

	priv->xfconfBackgroundColorBindingID=
		xfconf_g_property_bind(priv->xfconfChannel,
								COLOR_BACKGROUND_XFCONF_PROP,
								G_TYPE_STRING,
								self,
								"background-color");
}


/* IMPLEMENTATION: Public API */

/* Create new instance */
XfdashboardClockViewSettings* xfdashboard_clock_view_settings_new(void)
{
	return(XFDASHBOARD_CLOCK_VIEW_SETTINGS(g_object_new(XFDASHBOARD_TYPE_CLOCK_VIEW_SETTINGS, NULL)));	
}

/* Get/set color to draw hour hand with */
const ClutterColor* xfdashboard_clock_view_settings_get_hour_color(XfdashboardClockViewSettings *self)
{
	g_return_val_if_fail(XFDASHBOARD_IS_CLOCK_VIEW_SETTINGS(self), NULL);

	return(self->priv->hourColor);
}

void xfdashboard_clock_view_settings_set_hour_color(XfdashboardClockViewSettings *self, const ClutterColor *inColor)
{
	XfdashboardClockViewSettingsPrivate		*priv;

	g_return_if_fail(XFDASHBOARD_IS_CLOCK_VIEW_SETTINGS(self));
	g_return_if_fail(inColor);

	priv=self->priv;

	/* Set value if changed */
	if(priv->hourColor==NULL ||
		!clutter_color_equal(inColor, priv->hourColor))
	{
		/* Set value */
		if(priv->hourColor) clutter_color_free(priv->hourColor);
		priv->hourColor=clutter_color_copy(inColor);

		/* Notify about property change */
		g_object_notify_by_pspec(G_OBJECT(self), XfdashboardClockViewSettingsProperties[PROP_HOUR_COLOR]);
	}
}

/* Get/set color to draw minute hand with */
const ClutterColor* xfdashboard_clock_view_settings_get_minute_color(XfdashboardClockViewSettings *self)
{
	g_return_val_if_fail(XFDASHBOARD_IS_CLOCK_VIEW_SETTINGS(self), NULL);

	return(self->priv->minuteColor);
}

void xfdashboard_clock_view_settings_set_minute_color(XfdashboardClockViewSettings *self, const ClutterColor *inColor)
{
	XfdashboardClockViewSettingsPrivate		*priv;

	g_return_if_fail(XFDASHBOARD_IS_CLOCK_VIEW_SETTINGS(self));
	g_return_if_fail(inColor);

	priv=self->priv;

	/* Set value if changed */
	if(priv->minuteColor==NULL ||
		!clutter_color_equal(inColor, priv->minuteColor))
	{
		/* Set value */
		if(priv->minuteColor) clutter_color_free(priv->minuteColor);
		priv->minuteColor=clutter_color_copy(inColor);

		/* Notify about property change */
		g_object_notify_by_pspec(G_OBJECT(self), XfdashboardClockViewSettingsProperties[PROP_MINUTE_COLOR]);
	}
}

/* Get/set color to draw second hand with */
const ClutterColor* xfdashboard_clock_view_settings_get_second_color(XfdashboardClockViewSettings *self)
{
	g_return_val_if_fail(XFDASHBOARD_IS_CLOCK_VIEW_SETTINGS(self), NULL);

	return(self->priv->secondColor);
}

void xfdashboard_clock_view_settings_set_second_color(XfdashboardClockViewSettings *self, const ClutterColor *inColor)
{
	XfdashboardClockViewSettingsPrivate		*priv;

	g_return_if_fail(XFDASHBOARD_IS_CLOCK_VIEW_SETTINGS(self));
	g_return_if_fail(inColor);

	priv=self->priv;

	/* Set value if changed */
	if(priv->secondColor==NULL ||
		!clutter_color_equal(inColor, priv->secondColor))
	{
		/* Set value */
		if(priv->secondColor) clutter_color_free(priv->secondColor);
		priv->secondColor=clutter_color_copy(inColor);

		/* Notify about property change */
		g_object_notify_by_pspec(G_OBJECT(self), XfdashboardClockViewSettingsProperties[PROP_SECOND_COLOR]);
	}
}

/* Get/set color to draw background with that holds second hand */
const ClutterColor* xfdashboard_clock_view_settings_get_background_color(XfdashboardClockViewSettings *self)
{
	g_return_val_if_fail(XFDASHBOARD_IS_CLOCK_VIEW_SETTINGS(self), NULL);

	return(self->priv->backgroundColor);
}

void xfdashboard_clock_view_settings_set_background_color(XfdashboardClockViewSettings *self, const ClutterColor *inColor)
{
	XfdashboardClockViewSettingsPrivate		*priv;

	g_return_if_fail(XFDASHBOARD_IS_CLOCK_VIEW_SETTINGS(self));
	g_return_if_fail(inColor);

	priv=self->priv;

	/* Set value if changed */
	if(priv->backgroundColor==NULL ||
		!clutter_color_equal(inColor, priv->backgroundColor))
	{
		/* Set value */
		if(priv->backgroundColor) clutter_color_free(priv->backgroundColor);
		priv->backgroundColor=clutter_color_copy(inColor);

		/* Notify about property change */
		g_object_notify_by_pspec(G_OBJECT(self), XfdashboardClockViewSettingsProperties[PROP_BACKGROUOND_COLOR]);
	}
}
