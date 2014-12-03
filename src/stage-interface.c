/*
 * stage-interface: A top-level actor for a monitor at stage
 * 
 * Copyright 2012-2014 Stephan Haller <nomad@froevel.de>
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

#include "stage-interface.h"

#include <glib/gi18n-lib.h>

#include "actor.h"
#include "enums.h"
#include "stage.h"

/* Define this class in GObject system */
G_DEFINE_TYPE(XfdashboardStageInterface,
				xfdashboard_stage_interface,
				XFDASHBOARD_TYPE_ACTOR)

/* Private structure - access only by public API if needed */
#define XFDASHBOARD_STAGE_INTERFACE_GET_PRIVATE(obj) \
	(G_TYPE_INSTANCE_GET_PRIVATE((obj), XFDASHBOARD_TYPE_STAGE_INTERFACE, XfdashboardStageInterfacePrivate))

struct _XfdashboardStageInterfacePrivate
{
	/* Properties related */
	XfdashboardStageBackgroundImageType		backgroundType;
	ClutterColor							*backgroundColor;

	/* Instance related */
	ClutterConstraint						*positionConstraint;
	ClutterConstraint						*sizeConstraint;

	GBinding								*bindingBackgroundImageType;
	GBinding								*bindingBackgroundColor;
};

/* Properties */
enum
{
	PROP_0,

	PROP_BACKGROUND_IMAGE_TYPE,
	PROP_BACKGROUND_COLOR,

	PROP_LAST
};

static GParamSpec* XfdashboardStageInterfaceProperties[PROP_LAST]={ 0, };

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

	/* Set up binding constraints to reference new parent actor */
	newParent=clutter_actor_get_parent(inActor);

	if(priv->positionConstraint)
	{
		clutter_bind_constraint_set_source(CLUTTER_BIND_CONSTRAINT(priv->positionConstraint), newParent);
	}
		else
		{
			priv->positionConstraint=clutter_bind_constraint_new(newParent, CLUTTER_BIND_POSITION, 0.0f);
			clutter_actor_add_constraint(inActor, priv->positionConstraint);
		}

	if(priv->sizeConstraint)
	{
		clutter_bind_constraint_set_source(CLUTTER_BIND_CONSTRAINT(priv->sizeConstraint), newParent);
	}
		else
		{
			priv->sizeConstraint=clutter_bind_constraint_new(newParent, CLUTTER_BIND_SIZE, 0.0f);
			clutter_actor_add_constraint(inActor, priv->sizeConstraint);
		}

	/* Set up property bindings */
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
};

/* IMPLEMENTATION: GObject */

/* Dispose this object */
static void _xfdashboard_stage_interface_dispose(GObject *inObject)
{
	XfdashboardStageInterface			*self=XFDASHBOARD_STAGE_INTERFACE(inObject);
	XfdashboardStageInterfacePrivate	*priv=self->priv;

	/* Release allocated resources */
	if(priv->positionConstraint)
	{
		priv->positionConstraint=NULL;
	}

	if(priv->sizeConstraint)
	{
		priv->sizeConstraint=NULL;
	}

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

	gobjectClass->dispose=_xfdashboard_stage_interface_dispose;
	gobjectClass->set_property=_xfdashboard_stage_interface_set_property;
	gobjectClass->get_property=_xfdashboard_stage_interface_get_property;

	/* Set up private structure */
	g_type_class_add_private(klass, sizeof(XfdashboardStageInterfacePrivate));

	/* Define properties */
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

	priv=self->priv=XFDASHBOARD_STAGE_INTERFACE_GET_PRIVATE(self);

	/* Set default values */
	priv->backgroundType=XFDASHBOARD_STAGE_BACKGROUND_IMAGE_TYPE_NONE;
	priv->backgroundColor=NULL;
	priv->positionConstraint=NULL;
	priv->sizeConstraint=NULL;
	priv->bindingBackgroundImageType=NULL;
	priv->bindingBackgroundColor=NULL;
}

/* IMPLEMENTATION: Public API */

/* Create new instance */
ClutterActor* xfdashboard_stage_interface_new(void)
{
	return(CLUTTER_ACTOR(g_object_new(XFDASHBOARD_TYPE_STAGE_INTERFACE, NULL)));
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
