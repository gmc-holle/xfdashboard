/*
 * hot-corner-settings: Shared object instance holding settings for plugin
 * 
 * Copyright 2012-2021 Stephan Haller <nomad@froevel.de>
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

#include "hot-corner-settings.h"

#include <libxfdashboard/libxfdashboard.h>
#include <glib/gi18n-lib.h>
#include <gtk/gtk.h>
#include <math.h>


/* Define this class in GObject system */
struct _XfdashboardHotCornerSettingsPrivate
{
	/* Properties related */
	XfdashboardHotCornerSettingsActivationCorner	activationCorner;
	gint											activationRadius;
	gint64											activationDuration;
	gboolean										primaryMonitorOnly;
};

G_DEFINE_DYNAMIC_TYPE_EXTENDED(XfdashboardHotCornerSettings,
								xfdashboard_hot_corner_settings,
								XFDASHBOARD_TYPE_PLUGIN_SETTINGS,
								0,
								G_ADD_PRIVATE_DYNAMIC(XfdashboardHotCornerSettings))

/* Define this class in this plugin */
XFDASHBOARD_DEFINE_PLUGIN_TYPE(xfdashboard_hot_corner_settings);

/* Properties */
enum
{
	PROP_0,

	PROP_ACTIVATION_CORNER,
	PROP_ACTIVATION_RADIUS,
	PROP_ACTIVATION_DURATION,
	PROP_PRIMARY_MONITOR_ONLY,

	PROP_LAST
};

static GParamSpec* XfdashboardHotCornerSettingsProperties[PROP_LAST]={ 0, };


/* IMPLEMENTATION: Enum XFDASHBOARD_TYPE_HOT_CORNER_SETTINGS_ACTIVATION_CORNER */

GType xfdashboard_hot_corner_settings_activation_corner_get_type(void)
{
	static volatile gsize	g_define_type_id__volatile=0;

	if(g_once_init_enter(&g_define_type_id__volatile))
	{
		static const GEnumValue values[]=
		{
			{ XFDASHBOARD_HOT_CORNER_SETTINGS_ACTIVATION_CORNER_TOP_LEFT, "XFDASHBOARD_HOT_CORNER_SETTINGS_ACTIVATION_CORNER_TOP_LEFT", "top-left" },
			{ XFDASHBOARD_HOT_CORNER_SETTINGS_ACTIVATION_CORNER_TOP_RIGHT, "XFDASHBOARD_HOT_CORNER_SETTINGS_ACTIVATION_CORNER_TOP_RIGHT", "top-right" },
			{ XFDASHBOARD_HOT_CORNER_SETTINGS_ACTIVATION_CORNER_BOTTOM_LEFT, "XFDASHBOARD_HOT_CORNER_SETTINGS_ACTIVATION_CORNER_BOTTOM_LEFT", "bottom-left" },
			{ XFDASHBOARD_HOT_CORNER_SETTINGS_ACTIVATION_CORNER_BOTTOM_RIGHT, "XFDASHBOARD_HOT_CORNER_SETTINGS_ACTIVATION_CORNER_BOTTOM_RIGHT", "bottom-right" },
			{ 0, NULL, NULL }
		};

		GType	g_define_type_id=g_enum_register_static(g_intern_static_string("XfdashboardHotCornerSettingsActivationCorner"), values);
		g_once_init_leave(&g_define_type_id__volatile, g_define_type_id);
	}

	return(g_define_type_id__volatile);
}


/* IMPLEMENTATION: Private variables and methods */
#define POLL_POINTER_POSITION_INTERVAL			100

typedef struct _XfdashboardHotCornerSettingsBox		XfdashboardHotCornerSettingsBox;
struct _XfdashboardHotCornerSettingsBox
{
	gint		x1, y1;
	gint		x2, y2;
};

/* Single instance of plugin settings */
static XfdashboardHotCornerSettings*		_xfdashboard_hot_corner_settings=NULL;


/* IMPLEMENTATION: GObject */

/* Construct this object */
static GObject* _xfdashboard_hot_corner_settings_constructor(GType inType,
																guint inNumberConstructParams,
																GObjectConstructParam *inConstructParams)
{
	GObject									*object;

	if(!_xfdashboard_hot_corner_settings)
	{
		object=G_OBJECT_CLASS(xfdashboard_hot_corner_settings_parent_class)->constructor(inType, inNumberConstructParams, inConstructParams);
		_xfdashboard_hot_corner_settings=XFDASHBOARD_HOT_CORNER_SETTINGS(object);
	}
		else
		{
			object=g_object_ref(G_OBJECT(_xfdashboard_hot_corner_settings));
		}

	return(object);
}

/* Finalize this object */
static void _xfdashboard_hot_corner_settings_finalize(GObject *inObject)
{
	/* Release allocated resources finally, e.g. unset singleton */
	if(G_LIKELY(G_OBJECT(_xfdashboard_hot_corner_settings)==inObject))
	{
		_xfdashboard_hot_corner_settings=NULL;
	}

	/* Call parent's class dispose method */
	G_OBJECT_CLASS(xfdashboard_hot_corner_settings_parent_class)->finalize(inObject);
}

/* Set/get properties */
static void _xfdashboard_hot_corner_settings_set_property(GObject *inObject,
													guint inPropID,
													const GValue *inValue,
													GParamSpec *inSpec)
{
	XfdashboardHotCornerSettings			*self=XFDASHBOARD_HOT_CORNER_SETTINGS(inObject);

	switch(inPropID)
	{
		case PROP_ACTIVATION_CORNER:
			xfdashboard_hot_corner_settings_set_activation_corner(self, g_value_get_enum(inValue));
			break;

		case PROP_ACTIVATION_RADIUS:
			xfdashboard_hot_corner_settings_set_activation_radius(self, g_value_get_int(inValue));
			break;

		case PROP_ACTIVATION_DURATION:
			xfdashboard_hot_corner_settings_set_activation_duration(self, g_value_get_uint64(inValue));
			break;

		case PROP_PRIMARY_MONITOR_ONLY:
			xfdashboard_hot_corner_settings_set_primary_monitor_only(self, g_value_get_boolean(inValue));
			break;

		default:
			G_OBJECT_WARN_INVALID_PROPERTY_ID(inObject, inPropID, inSpec);
			break;
	}
}

static void _xfdashboard_hot_corner_settings_get_property(GObject *inObject,
													guint inPropID,
													GValue *outValue,
													GParamSpec *inSpec)
{
	XfdashboardHotCornerSettings			*self=XFDASHBOARD_HOT_CORNER_SETTINGS(inObject);
	XfdashboardHotCornerSettingsPrivate		*priv=self->priv;

	switch(inPropID)
	{
		case PROP_ACTIVATION_CORNER:
			g_value_set_enum(outValue, priv->activationCorner);
			break;

		case PROP_ACTIVATION_RADIUS:
			g_value_set_int(outValue, priv->activationRadius);
			break;

		case PROP_ACTIVATION_DURATION:
			g_value_set_uint64(outValue, priv->activationDuration);
			break;
		case PROP_PRIMARY_MONITOR_ONLY:
			g_value_set_boolean(outValue, priv->primaryMonitorOnly);
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
void xfdashboard_hot_corner_settings_class_init(XfdashboardHotCornerSettingsClass *klass)
{
	GObjectClass			*gobjectClass=G_OBJECT_CLASS(klass);

	/* Override functions */
	gobjectClass->constructor=_xfdashboard_hot_corner_settings_constructor;
	gobjectClass->finalize=_xfdashboard_hot_corner_settings_finalize;
	gobjectClass->set_property=_xfdashboard_hot_corner_settings_set_property;
	gobjectClass->get_property=_xfdashboard_hot_corner_settings_get_property;

	/* Define properties */
	XfdashboardHotCornerSettingsProperties[PROP_ACTIVATION_CORNER]=
		g_param_spec_enum("activation-corner",
							"Activation corner",
							"The hot corner where to trigger the application to suspend or to resume",
							XFDASHBOARD_TYPE_HOT_CORNER_SETTINGS_ACTIVATION_CORNER,
							XFDASHBOARD_HOT_CORNER_SETTINGS_ACTIVATION_CORNER_TOP_LEFT,
							G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS);

	XfdashboardHotCornerSettingsProperties[PROP_ACTIVATION_RADIUS]=
		g_param_spec_int("activation-radius",
							"Activation radius",
							"The radius around hot corner where the pointer must be inside",
							0, G_MAXINT,
							4,
							G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS);

	XfdashboardHotCornerSettingsProperties[PROP_ACTIVATION_DURATION]=
		g_param_spec_uint64("activation-duration",
							"Activation duration",
							"The time in milliseconds the pointer must stay inside the radius at hot corner to trigger",
							0, G_MAXUINT64,
							300,
							G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS);

	XfdashboardHotCornerSettingsProperties[PROP_PRIMARY_MONITOR_ONLY]=
		g_param_spec_boolean("primary-monitor-only",
								"Primary monitor only",
								"A flag indicating if all monitors or only the primary one should be check for hot corner",
								TRUE,
								G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS);

	g_object_class_install_properties(gobjectClass, PROP_LAST, XfdashboardHotCornerSettingsProperties);
}

/* Class finalization */
void xfdashboard_hot_corner_settings_class_finalize(XfdashboardHotCornerSettingsClass *klass)
{
}

/* Object initialization
 * Create private structure and set up default values
 */
void xfdashboard_hot_corner_settings_init(XfdashboardHotCornerSettings *self)
{
	XfdashboardHotCornerSettingsPrivate		*priv;

	self->priv=priv=xfdashboard_hot_corner_settings_get_instance_private(self);

	/* Set up default values */
	priv->activationCorner=XFDASHBOARD_HOT_CORNER_SETTINGS_ACTIVATION_CORNER_TOP_LEFT;
	priv->activationRadius=4;
	priv->activationDuration=300;
	priv->primaryMonitorOnly=TRUE;
}


/* IMPLEMENTATION: Public API */

/* Create new instance */
XfdashboardHotCornerSettings* xfdashboard_hot_corner_settings_new(void)
{
	GObject		*hotCorner;

	hotCorner=g_object_new(XFDASHBOARD_TYPE_HOT_CORNER_SETTINGS, NULL);
	if(!hotCorner) return(NULL);

	return(XFDASHBOARD_HOT_CORNER_SETTINGS(hotCorner));
}

/* Get/set hot corner */
XfdashboardHotCornerSettingsActivationCorner xfdashboard_hot_corner_settings_get_activation_corner(XfdashboardHotCornerSettings *self)
{
	g_return_val_if_fail(XFDASHBOARD_IS_HOT_CORNER_SETTINGS(self), XFDASHBOARD_HOT_CORNER_SETTINGS_ACTIVATION_CORNER_TOP_LEFT);

	return(self->priv->activationCorner);
}

void xfdashboard_hot_corner_settings_set_activation_corner(XfdashboardHotCornerSettings *self, XfdashboardHotCornerSettingsActivationCorner inCorner)
{
	XfdashboardHotCornerSettingsPrivate		*priv;

	g_return_if_fail(XFDASHBOARD_IS_HOT_CORNER_SETTINGS(self));
	g_return_if_fail(inCorner<=XFDASHBOARD_HOT_CORNER_SETTINGS_ACTIVATION_CORNER_BOTTOM_RIGHT);

	priv=self->priv;

	/* Set value if changed */
	if(priv->activationCorner!=inCorner)
	{
		/* Set value */
		priv->activationCorner=inCorner;

		/* Notify about property change */
		g_object_notify_by_pspec(G_OBJECT(self), XfdashboardHotCornerSettingsProperties[PROP_ACTIVATION_CORNER]);
	}
}

/* Get/set radius around hot corner */
gint xfdashboard_hot_corner_settings_get_activation_radius(XfdashboardHotCornerSettings *self)
{
	g_return_val_if_fail(XFDASHBOARD_IS_HOT_CORNER_SETTINGS(self), 0);

	return(self->priv->activationRadius);
}

void xfdashboard_hot_corner_settings_set_activation_radius(XfdashboardHotCornerSettings *self, gint inRadius)
{
	XfdashboardHotCornerSettingsPrivate		*priv;

	g_return_if_fail(XFDASHBOARD_IS_HOT_CORNER_SETTINGS(self));
	g_return_if_fail(inRadius>0);

	priv=self->priv;

	/* Set value if changed */
	if(priv->activationRadius!=inRadius)
	{
		/* Set value */
		priv->activationRadius=inRadius;

		/* Notify about property change */
		g_object_notify_by_pspec(G_OBJECT(self), XfdashboardHotCornerSettingsProperties[PROP_ACTIVATION_RADIUS]);
	}
}

/* Get/set duration when to trigger hot corner */
gint64 xfdashboard_hot_corner_settings_get_activation_duration(XfdashboardHotCornerSettings *self)
{
	g_return_val_if_fail(XFDASHBOARD_IS_HOT_CORNER_SETTINGS(self), 0);

	return(self->priv->activationDuration);
}

void xfdashboard_hot_corner_settings_set_activation_duration(XfdashboardHotCornerSettings *self, gint64 inDuration)
{
	XfdashboardHotCornerSettingsPrivate		*priv;

	g_return_if_fail(XFDASHBOARD_IS_HOT_CORNER_SETTINGS(self));
	g_return_if_fail(inDuration>0);

	priv=self->priv;

	/* Set value if changed */
	if(priv->activationDuration!=inDuration)
	{
		/* Set value */
		priv->activationDuration=inDuration;

		/* Notify about property change */
		g_object_notify_by_pspec(G_OBJECT(self), XfdashboardHotCornerSettingsProperties[PROP_ACTIVATION_DURATION]);
	}
}

/* Get/set flag to check primary monitor only if hot corner was entered */
gboolean xfdashboard_hot_corner_settings_get_primary_monitor_only(XfdashboardHotCornerSettings *self)
{
	g_return_val_if_fail(XFDASHBOARD_IS_HOT_CORNER_SETTINGS(self), 0);

	return(self->priv->primaryMonitorOnly);
}

void xfdashboard_hot_corner_settings_set_primary_monitor_only(XfdashboardHotCornerSettings *self, gboolean inPrimaryOnly)
{
	XfdashboardHotCornerSettingsPrivate		*priv;

	g_return_if_fail(XFDASHBOARD_IS_HOT_CORNER_SETTINGS(self));

	priv=self->priv;

	/* Set value if changed */
	if(priv->primaryMonitorOnly!=inPrimaryOnly)
	{
		/* Set value */
		priv->primaryMonitorOnly=inPrimaryOnly;

		/* Notify about property change */
		g_object_notify_by_pspec(G_OBJECT(self), XfdashboardHotCornerSettingsProperties[PROP_PRIMARY_MONITOR_ONLY]);
	}
}
