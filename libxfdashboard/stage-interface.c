/*
 * stage-interface: A top-level actor for a monitor at stage
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

#include <libxfdashboard/stage-interface.h>

#include <glib/gi18n-lib.h>

#include <libxfdashboard/actor.h>
#include <libxfdashboard/enums.h>
#include <libxfdashboard/stage.h>
#include <libxfdashboard/stylable.h>
#include <libxfdashboard/compat.h>
#include <libxfdashboard/debug.h>


/* Define this class in GObject system */
struct _XfdashboardStageInterfacePrivate
{
	/* Properties related */
	XfdashboardWindowTrackerMonitor			*monitor;

	XfdashboardStageBackgroundImageType		backgroundType;
	ClutterColor							*backgroundColor;

	/* Instance related */
	GBinding								*bindingBackgroundImageType;
	GBinding								*bindingBackgroundColor;

	guint									geometryChangedID;
	guint									primaryChangedID;
};

G_DEFINE_TYPE_WITH_PRIVATE(XfdashboardStageInterface,
							xfdashboard_stage_interface,
							XFDASHBOARD_TYPE_ACTOR)

/* Properties */
enum
{
	PROP_0,

	PROP_MONITOR,

	PROP_BACKGROUND_IMAGE_TYPE,
	PROP_BACKGROUND_COLOR,

	PROP_LAST
};

static GParamSpec* XfdashboardStageInterfaceProperties[PROP_LAST]={ 0, };


/* IMPLEMENTATION: Private variables and methods */

/* Monitor size changed */
static void _xfdashboard_stage_interface_on_geometry_changed(XfdashboardStageInterface *self, gpointer inUserData)
{
	XfdashboardStageInterfacePrivate	*priv;
	gint								x, y, w, h;

	g_return_if_fail(XFDASHBOARD_IS_STAGE_INTERFACE(self));

	priv=self->priv;

	/* Resize actor to new monitor */
	xfdashboard_window_tracker_monitor_get_geometry(priv->monitor, &x, &y, &w, &h);
	clutter_actor_set_position(CLUTTER_ACTOR(self), x, y);
	clutter_actor_set_size(CLUTTER_ACTOR(self), w, h);

	XFDASHBOARD_DEBUG(self, ACTOR,
						"Stage interface moved to %d,%d and resized to %dx%d because %s monitor %d changed geometry",
						x, y,
						w, h,
						xfdashboard_window_tracker_monitor_is_primary(priv->monitor) ? "primary" : "non-primary",
						xfdashboard_window_tracker_monitor_get_number(priv->monitor));
}

/* Monitor changed primary state */
static void _xfdashboard_stage_interface_on_primary_changed(XfdashboardStageInterface *self, gpointer inUserData)
{
	XfdashboardStageInterfacePrivate	*priv;
	gboolean							isPrimary;

	g_return_if_fail(XFDASHBOARD_IS_STAGE_INTERFACE(self));

	priv=self->priv;

	/* Get new primary state of monitor */
	isPrimary=xfdashboard_window_tracker_monitor_is_primary(priv->monitor);

	/* Depending on primary state set CSS class */
	if(isPrimary) xfdashboard_stylable_add_class(XFDASHBOARD_STYLABLE(self), "primary-monitor");
		else xfdashboard_stylable_remove_class(XFDASHBOARD_STYLABLE(self), "primary-monitor");

	XFDASHBOARD_DEBUG(self, ACTOR,
						"Stage interface changed primary state to %s because of monitor %d",
						xfdashboard_window_tracker_monitor_is_primary(priv->monitor) ? "primary" : "non-primary",
						xfdashboard_window_tracker_monitor_get_number(priv->monitor));
}

/* IMPLEMENTATION: ClutterActor */

/* Actor was (re)parented */
static void xfdashboard_stage_interface_parent_set(ClutterActor *inActor, ClutterActor *inOldParent)
{
	XfdashboardStageInterface			*self;
	XfdashboardStageInterfacePrivate	*priv;
	ClutterActorClass					*parentClass;
	ClutterActor						*newParent;

	g_return_if_fail(XFDASHBOARD_IS_STAGE_INTERFACE(inActor));

	self=XFDASHBOARD_STAGE_INTERFACE(inActor);
	priv=self->priv;

	/* Call parent's virtual function */
	parentClass=CLUTTER_ACTOR_CLASS(xfdashboard_stage_interface_parent_class);
	if(parentClass->parent_set)
	{
		parentClass->parent_set(inActor, inOldParent);
	}

	/* Set up property bindings to new parent actor */
	newParent=clutter_actor_get_parent(inActor);

	if(priv->bindingBackgroundImageType)
	{
		g_object_unref(priv->bindingBackgroundImageType);
		priv->bindingBackgroundImageType=NULL;
	}

	if(priv->bindingBackgroundColor)
	{
		g_object_unref(priv->bindingBackgroundColor);
		priv->bindingBackgroundColor=NULL;
	}

	if(newParent && XFDASHBOARD_IS_STAGE(newParent))
	{
		priv->bindingBackgroundImageType=g_object_bind_property(self, "background-image-type", newParent, "background-image-type", G_BINDING_DEFAULT);
		priv->bindingBackgroundColor=g_object_bind_property(self, "background-color", newParent, "background-color", G_BINDING_DEFAULT);
	}
}

/* Get preferred width/height */
static void _xfdashboard_stage_interface_get_preferred_height(ClutterActor *inActor,
																gfloat inForWidth,
																gfloat *outMinHeight,
																gfloat *outNaturalHeight)
{
	XfdashboardStageInterface			*self=XFDASHBOARD_STAGE_INTERFACE(inActor);
	XfdashboardStageInterfacePrivate	*priv=self->priv;
	gfloat								minHeight, naturalHeight;
	gint								h;
	ClutterActor						 *stage;

	/* Set up default values */
	minHeight=naturalHeight=0.0f;

	/* Get monitor size if available otherwise get stage size */
	if(priv->monitor)
	{
		xfdashboard_window_tracker_monitor_get_geometry(priv->monitor, NULL, NULL, NULL, &h);
		minHeight=naturalHeight=h;
	}
		else
		{
			stage=clutter_actor_get_stage(inActor);
			minHeight=naturalHeight=clutter_actor_get_height(stage);
		}

	/* Store sizes computed */
	if(outMinHeight) *outMinHeight=minHeight;
	if(outNaturalHeight) *outNaturalHeight=naturalHeight;
}

static void _xfdashboard_stage_interface_get_preferred_width(ClutterActor *inActor,
																gfloat inForHeight,
																gfloat *outMinWidth,
																gfloat *outNaturalWidth)
{
	XfdashboardStageInterface			*self=XFDASHBOARD_STAGE_INTERFACE(inActor);
	XfdashboardStageInterfacePrivate	*priv=self->priv;
	gfloat								minWidth, naturalWidth;
	gint								w;
	ClutterActor						 *stage;

	/* Set up default values */
	minWidth=naturalWidth=0.0f;

	/* Get monitor size if available otherwise get stage size */
	if(priv->monitor)
	{
		xfdashboard_window_tracker_monitor_get_geometry(priv->monitor, NULL, NULL, &w, NULL);
		minWidth=naturalWidth=w;
	}
		else
		{
			stage=clutter_actor_get_stage(inActor);
			minWidth=naturalWidth=clutter_actor_get_width(stage);
		}

	/* Store sizes computed */
	if(outMinWidth) *outMinWidth=minWidth;
	if(outNaturalWidth) *outNaturalWidth=naturalWidth;
}

/* IMPLEMENTATION: GObject */

/* Dispose this object */
static void _xfdashboard_stage_interface_dispose(GObject *inObject)
{
	XfdashboardStageInterface			*self=XFDASHBOARD_STAGE_INTERFACE(inObject);
	XfdashboardStageInterfacePrivate	*priv=self->priv;

	/* Release allocated resources */
	if(priv->bindingBackgroundImageType)
	{
		g_object_unref(priv->bindingBackgroundImageType);
		priv->bindingBackgroundImageType=NULL;
	}

	if(priv->bindingBackgroundColor)
	{
		g_object_unref(priv->bindingBackgroundColor);
		priv->bindingBackgroundColor=NULL;
	}

	if(priv->monitor)
	{
		if(priv->geometryChangedID)
		{
			g_signal_handler_disconnect(priv->monitor, priv->geometryChangedID);
			priv->geometryChangedID=0;
		}

		if(priv->primaryChangedID)
		{
			g_signal_handler_disconnect(priv->monitor, priv->primaryChangedID);
			priv->primaryChangedID=0;
		}

		g_object_unref(priv->monitor);
		priv->monitor=NULL;
	}

	if(priv->backgroundColor)
	{
		clutter_color_free(priv->backgroundColor);
		priv->backgroundColor=NULL;
	}

	/* Call parent's class dispose method */
	G_OBJECT_CLASS(xfdashboard_stage_interface_parent_class)->dispose(inObject);
}

/* Set/get properties */
static void _xfdashboard_stage_interface_set_property(GObject *inObject,
														guint inPropID,
														const GValue *inValue,
														GParamSpec *inSpec)
{
	XfdashboardStageInterface			*self=XFDASHBOARD_STAGE_INTERFACE(inObject);

	switch(inPropID)
	{
		case PROP_MONITOR:
			xfdashboard_stage_interface_set_monitor(self, XFDASHBOARD_WINDOW_TRACKER_MONITOR(g_value_get_object(inValue)));
			break;

		case PROP_BACKGROUND_IMAGE_TYPE:
			xfdashboard_stage_interface_set_background_image_type(self, g_value_get_enum(inValue));
			break;

		case PROP_BACKGROUND_COLOR:
			xfdashboard_stage_interface_set_background_color(self, clutter_value_get_color(inValue));
			break;

		default:
			G_OBJECT_WARN_INVALID_PROPERTY_ID(inObject, inPropID, inSpec);
			break;
	}
}

static void _xfdashboard_stage_interface_get_property(GObject *inObject,
														guint inPropID,
														GValue *outValue,
														GParamSpec *inSpec)
{
	XfdashboardStageInterface			*self=XFDASHBOARD_STAGE_INTERFACE(inObject);
	XfdashboardStageInterfacePrivate	*priv=self->priv;

	switch(inPropID)
	{
		case PROP_MONITOR:
			g_value_set_object(outValue, priv->monitor);
			break;

		case PROP_BACKGROUND_IMAGE_TYPE:
			g_value_set_enum(outValue, priv->backgroundType);
			break;

		case PROP_BACKGROUND_COLOR:
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
static void xfdashboard_stage_interface_class_init(XfdashboardStageInterfaceClass *klass)
{
	XfdashboardActorClass			*actorClass=XFDASHBOARD_ACTOR_CLASS(klass);
	ClutterActorClass				*clutterActorClass=CLUTTER_ACTOR_CLASS(klass);
	GObjectClass					*gobjectClass=G_OBJECT_CLASS(klass);

	/* Override functions */
	clutterActorClass->parent_set=xfdashboard_stage_interface_parent_set;
	clutterActorClass->get_preferred_width=_xfdashboard_stage_interface_get_preferred_width;
	clutterActorClass->get_preferred_height=_xfdashboard_stage_interface_get_preferred_height;

	gobjectClass->dispose=_xfdashboard_stage_interface_dispose;
	gobjectClass->set_property=_xfdashboard_stage_interface_set_property;
	gobjectClass->get_property=_xfdashboard_stage_interface_get_property;

	/* Define properties */
	XfdashboardStageInterfaceProperties[PROP_MONITOR]=
		g_param_spec_object("monitor",
							_("Monitor"),
							_("The monitor where this stage interface is connected to"),
							XFDASHBOARD_TYPE_WINDOW_TRACKER_MONITOR,
							G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS);

	XfdashboardStageInterfaceProperties[PROP_BACKGROUND_IMAGE_TYPE]=
		g_param_spec_enum("background-image-type",
							_("Background image type"),
							_("Background image type"),
							XFDASHBOARD_TYPE_STAGE_BACKGROUND_IMAGE_TYPE,
							XFDASHBOARD_STAGE_BACKGROUND_IMAGE_TYPE_NONE,
							G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS);

	XfdashboardStageInterfaceProperties[PROP_BACKGROUND_COLOR]=
		clutter_param_spec_color("background-color",
									_("Background color"),
									_("Color of stage's background"),
									NULL,
									G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS);

	g_object_class_install_properties(gobjectClass, PROP_LAST, XfdashboardStageInterfaceProperties);

	/* Define stylable properties */
	xfdashboard_actor_install_stylable_property(actorClass, XfdashboardStageInterfaceProperties[PROP_BACKGROUND_IMAGE_TYPE]);
	xfdashboard_actor_install_stylable_property(actorClass, XfdashboardStageInterfaceProperties[PROP_BACKGROUND_COLOR]);
}

/* Object initialization
 * Create private structure and set up default values
 */
static void xfdashboard_stage_interface_init(XfdashboardStageInterface *self)
{
	XfdashboardStageInterfacePrivate	*priv;

	priv=self->priv=xfdashboard_stage_interface_get_instance_private(self);

	/* Set default values */
	priv->monitor=NULL;
	priv->geometryChangedID=0;
	priv->primaryChangedID=0;
	priv->backgroundType=XFDASHBOARD_STAGE_BACKGROUND_IMAGE_TYPE_NONE;
	priv->backgroundColor=NULL;
	priv->bindingBackgroundImageType=NULL;
	priv->bindingBackgroundColor=NULL;

	/* Let this actor receive events */
	clutter_actor_set_reactive(CLUTTER_ACTOR(self), TRUE);
}

/* IMPLEMENTATION: Public API */

/* Create new instance */
ClutterActor* xfdashboard_stage_interface_new(void)
{
	return(CLUTTER_ACTOR(g_object_new(XFDASHBOARD_TYPE_STAGE_INTERFACE, NULL)));
}

/* Get/set monitor */
XfdashboardWindowTrackerMonitor* xfdashboard_stage_interface_get_monitor(XfdashboardStageInterface *self)
{
	g_return_val_if_fail(XFDASHBOARD_IS_STAGE_INTERFACE(self), NULL);

	return(self->priv->monitor);
}

void xfdashboard_stage_interface_set_monitor(XfdashboardStageInterface *self, XfdashboardWindowTrackerMonitor *inMonitor)
{
	XfdashboardStageInterfacePrivate	*priv;

	g_return_if_fail(XFDASHBOARD_IS_STAGE_INTERFACE(self));
	g_return_if_fail(XFDASHBOARD_IS_WINDOW_TRACKER_MONITOR(inMonitor));

	priv=self->priv;

	/* Set value if changed */
	if(priv->monitor!=inMonitor)
	{
		/* Set value */
		if(priv->monitor)
		{
			if(priv->geometryChangedID)
			{
				g_signal_handler_disconnect(priv->monitor, priv->geometryChangedID);
				priv->geometryChangedID=0;
			}

			if(priv->primaryChangedID)
			{
				g_signal_handler_disconnect(priv->monitor, priv->primaryChangedID);
				priv->primaryChangedID=0;
			}

			g_object_unref(priv->monitor);
			priv->monitor=NULL;
		}

		priv->monitor=XFDASHBOARD_WINDOW_TRACKER_MONITOR(g_object_ref(inMonitor));

		/* Connect signals */
		priv->geometryChangedID=g_signal_connect_swapped(priv->monitor,
															"geometry-changed",
															G_CALLBACK(_xfdashboard_stage_interface_on_geometry_changed),
															self);
		priv->primaryChangedID=g_signal_connect_swapped(priv->monitor,
															"primary-changed",
															G_CALLBACK(_xfdashboard_stage_interface_on_primary_changed),
															self);

		/* Resize actor to new monitor */
		_xfdashboard_stage_interface_on_geometry_changed(self, priv->monitor);

		/* Update actor from primary state */
		_xfdashboard_stage_interface_on_primary_changed(self, priv->monitor);

		/* Notify about property change */
		g_object_notify_by_pspec(G_OBJECT(self), XfdashboardStageInterfaceProperties[PROP_MONITOR]);
	}
}

/* Get/set background type */
XfdashboardStageBackgroundImageType xfdashboard_stage_interface_get_background_image_type(XfdashboardStageInterface *self)
{
	g_return_val_if_fail(XFDASHBOARD_IS_STAGE_INTERFACE(self), XFDASHBOARD_STAGE_BACKGROUND_IMAGE_TYPE_NONE);

	return(self->priv->backgroundType);
}

void xfdashboard_stage_interface_set_background_image_type(XfdashboardStageInterface *self, XfdashboardStageBackgroundImageType inType)
{
	XfdashboardStageInterfacePrivate	*priv;

	g_return_if_fail(XFDASHBOARD_IS_STAGE_INTERFACE(self));
	g_return_if_fail(inType<=XFDASHBOARD_STAGE_BACKGROUND_IMAGE_TYPE_DESKTOP);

	priv=self->priv;

	/* Set value if changed */
	if(priv->backgroundType!=inType)
	{
		/* Set value */
		priv->backgroundType=inType;

		/* Notify about property change */
		g_object_notify_by_pspec(G_OBJECT(self), XfdashboardStageInterfaceProperties[PROP_BACKGROUND_IMAGE_TYPE]);
	}
}

/* Get/set background color */
ClutterColor* xfdashboard_stage_interface_get_background_color(XfdashboardStageInterface *self)
{
	g_return_val_if_fail(XFDASHBOARD_IS_STAGE_INTERFACE(self), NULL);

	return(self->priv->backgroundColor);
}

void xfdashboard_stage_interface_set_background_color(XfdashboardStageInterface *self, const ClutterColor *inColor)
{
	XfdashboardStageInterfacePrivate	*priv;

	g_return_if_fail(XFDASHBOARD_IS_STAGE_INTERFACE(self));

	priv=self->priv;


	/* Set value if changed */
	if((priv->backgroundColor && !inColor) ||
		(!priv->backgroundColor && inColor) ||
		(inColor && clutter_color_equal(inColor, priv->backgroundColor)==FALSE))
	{
		/* Set value */
		if(priv->backgroundColor)
		{
			clutter_color_free(priv->backgroundColor);
			priv->backgroundColor=NULL;
		}

		if(inColor) priv->backgroundColor=clutter_color_copy(inColor);

		/* Notify about property change */
		g_object_notify_by_pspec(G_OBJECT(self), XfdashboardStageInterfaceProperties[PROP_BACKGROUND_COLOR]);
	}
}
