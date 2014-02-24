/*
 * background: An actor providing background rendering. Usually other
 *             actors are derived from this one.
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

#include "background.h"
#include "enums.h"

#include <glib/gi18n-lib.h>
#include <gdk/gdk.h>
#include <math.h>

/* Define this class in GObject system */
G_DEFINE_TYPE(XfdashboardBackground,
				xfdashboard_background,
				XFDASHBOARD_TYPE_ACTOR)

/* Private structure - access only by public API if needed */
#define XFDASHBOARD_BACKGROUND_GET_PRIVATE(obj) \
	(G_TYPE_INSTANCE_GET_PRIVATE((obj), XFDASHBOARD_TYPE_BACKGROUND, XfdashboardBackgroundPrivate))

struct _XfdashboardBackgroundPrivate
{
	/* Properties related */
	XfdashboardBackgroundType	type;

	ClutterColor				*fillColor;

	ClutterColor				*outlineColor;
	gfloat						outlineWidth;

	XfdashboardCorners			corners;
	gfloat						cornersRadius;

	/* Instance related */
	ClutterContent				*canvas;
	ClutterImage				*image;
};

/* Properties */
enum
{
	PROP_0,

	PROP_TYPE,

	PROP_FILL_COLOR,
	PROP_OUTLINE_COLOR,
	PROP_OUTLINE_WIDTH,
	PROP_CORNERS,
	PROP_CORNERS_RADIUS,

	PROP_IMAGE,

	PROP_LAST
};

static GParamSpec* XfdashboardBackgroundProperties[PROP_LAST]={ 0, };

/* IMPLEMENTATION: Private variables and methods */

/* Rectangle canvas should be redrawn */
static gboolean _xfdashboard_background_on_draw_canvas(XfdashboardBackground *self,
														cairo_t *inContext,
														int inWidth,
														int inHeight,
														gpointer inUserData)
{
	XfdashboardBackgroundPrivate	*priv;

	g_return_val_if_fail(XFDASHBOARD_IS_BACKGROUND(self), TRUE);
	g_return_val_if_fail(CLUTTER_IS_CANVAS(inUserData), TRUE);

	priv=self->priv;

	/* Clear current contents of the canvas */
	cairo_save(inContext);
	cairo_set_operator(inContext, CAIRO_OPERATOR_CLEAR);
	cairo_paint(inContext);
	cairo_restore(inContext);

	cairo_set_operator(inContext, CAIRO_OPERATOR_OVER);

	/* Do nothing if type is none (we should not get here but just in case we do) */
	if(priv->type==XFDASHBOARD_BACKGROUND_TYPE_NONE) return(CLUTTER_EVENT_PROPAGATE);

	/* Determine if we should draw rounded corners */

	/* Draw rectangle with or without rounded corners */
	if((priv->type & XFDASHBOARD_BACKGROUND_TYPE_ROUNDED_CORNERS) && 
		(priv->corners & XFDASHBOARD_CORNERS_ALL) &&
		priv->cornersRadius>0.0f)
	{
		/* Determine radius for rounded corners */
		gdouble						radius;

		radius=MIN(priv->cornersRadius, inWidth/2.0f);
		radius=MIN(radius, inHeight/2.0f);

		/* Top-left */
		if(priv->corners & XFDASHBOARD_CORNERS_TOP_LEFT)
		{
			cairo_move_to(inContext, 0, radius);
			cairo_arc(inContext, radius, radius, radius, G_PI, G_PI*1.5);
		}
			else cairo_move_to(inContext, 0, 0);

		/* Top-right */
		if(priv->corners & XFDASHBOARD_CORNERS_TOP_RIGHT)
		{
			cairo_line_to(inContext, inWidth-radius, 0);
			cairo_arc(inContext, inWidth-radius, radius, radius, G_PI*1.5, 0);
		}
			else cairo_line_to(inContext, inWidth, 0);

		/* Bottom-right */
		if(priv->corners & XFDASHBOARD_CORNERS_BOTTOM_RIGHT)
		{
			cairo_line_to(inContext, inWidth, inHeight-radius);
			cairo_arc(inContext, inWidth-radius, inHeight-radius, radius, 0, G_PI/2.0);
		}
			else cairo_line_to(inContext, inWidth, inHeight);

		/* Bottom-left */
		if(priv->corners & XFDASHBOARD_CORNERS_BOTTOM_LEFT)
		{
			cairo_line_to(inContext, radius, inHeight);
			cairo_arc(inContext, radius, inHeight-radius, radius, G_PI/2.0, G_PI);
		}
			else cairo_line_to(inContext, 0, inHeight);

		/* Close to top-left */
		if(priv->corners & XFDASHBOARD_CORNERS_TOP_LEFT) cairo_line_to(inContext, 0, radius);
			else cairo_line_to(inContext, 0, 0);
	}
		else
		{
			cairo_rectangle(inContext, 0, 0, inWidth, inHeight);
		}

	/* Fill if type requests it */
	if(priv->type & XFDASHBOARD_BACKGROUND_TYPE_FILL)
	{
		/* Set color for filling */
		if(priv->fillColor) clutter_cairo_set_source_color(inContext, priv->fillColor);

		/* Fill */
		cairo_fill_preserve(inContext);
	}

	/* Draw outline if type requests it */
	if(priv->type & XFDASHBOARD_BACKGROUND_TYPE_OUTLINE)
	{
		/* Set up line properties for outline */
		if(priv->outlineColor) clutter_cairo_set_source_color(inContext, priv->outlineColor);
		cairo_set_line_width(inContext, priv->outlineWidth);

		/* Draw outline */
		cairo_stroke_preserve(inContext);
	}

	/* Done drawing */
	cairo_close_path(inContext);
	return(CLUTTER_EVENT_PROPAGATE);
}

/* IMPLEMENTATION: ClutterActor */

/* Paint actor */
static void _xfdashboard_background_paint_node(ClutterActor *self,
												ClutterPaintNode *inRootNode)
{
	XfdashboardBackgroundPrivate	*priv=XFDASHBOARD_BACKGROUND(self)->priv;
	ClutterContentIface				*iface;

	/* First draw canvas for background */
	if(priv->type!=XFDASHBOARD_BACKGROUND_TYPE_NONE)
	{
		iface=CLUTTER_CONTENT_GET_IFACE(priv->canvas);
		if(iface->paint_content) iface->paint_content(priv->canvas, self, inRootNode);
	}

	/* If available draw image for background */
	if(priv->image)
	{
		iface=CLUTTER_CONTENT_GET_IFACE(priv->image);
		if(iface->paint_content) iface->paint_content(CLUTTER_CONTENT(priv->image), self, inRootNode);
	}

	/* Now chain up to draw the actor */
	if(CLUTTER_ACTOR_CLASS(xfdashboard_background_parent_class)->paint_node)
	{
		CLUTTER_ACTOR_CLASS(xfdashboard_background_parent_class)->paint_node(self, inRootNode);
	}
}

/* Get preferred width/height */
static void _xfdashboard_background_get_preferred_height(ClutterActor *self,
															gfloat inForWidth,
															gfloat *outMinHeight,
															gfloat *outNaturalHeight)
{
	XfdashboardBackgroundPrivate	*priv=XFDASHBOARD_BACKGROUND(self)->priv;
	gfloat							minHeight, naturalHeight;
	
	minHeight=naturalHeight=0.0f;

	/* Determine size if any type of rectangle should be drawn */
	if(priv->type & XFDASHBOARD_BACKGROUND_TYPE_ROUNDED_CORNERS)
	{
		naturalHeight=priv->cornersRadius*2.0f;
	}

	/* Store sizes computed */
	if(outMinHeight) *outMinHeight=minHeight;
	if(outNaturalHeight) *outNaturalHeight=naturalHeight;
}

static void _xfdashboard_background_get_preferred_width(ClutterActor *self,
														gfloat inForHeight,
														gfloat *outMinWidth,
														gfloat *outNaturalWidth)
{
	XfdashboardBackgroundPrivate	*priv=XFDASHBOARD_BACKGROUND(self)->priv;
	gfloat							minWidth, naturalWidth;

	minWidth=naturalWidth=0.0f;

	/* Determine size if any type of rectangle should be drawn */
	if(priv->type & XFDASHBOARD_BACKGROUND_TYPE_ROUNDED_CORNERS)
	{
		naturalWidth=priv->cornersRadius*2.0f;
	}

	/* Store sizes computed */
	if(outMinWidth) *outMinWidth=minWidth;
	if(outNaturalWidth) *outNaturalWidth=naturalWidth;
}

/* Allocate position and size of actor and its children */
static void _xfdashboard_background_allocate(ClutterActor *self,
												const ClutterActorBox *inBox,
												ClutterAllocationFlags inFlags)
{
	XfdashboardBackgroundPrivate	*priv=XFDASHBOARD_BACKGROUND(self)->priv;

	/* Chain up to store the allocation of the actor */
	CLUTTER_ACTOR_CLASS(xfdashboard_background_parent_class)->allocate(self, inBox, inFlags);

	/* Set size of canvas */
	clutter_canvas_set_size(CLUTTER_CANVAS(priv->canvas),
								clutter_actor_box_get_width(inBox),
								clutter_actor_box_get_height(inBox));
}

/* IMPLEMENTATION: GObject */

/* Dispose this object */
static void _xfdashboard_background_dispose(GObject *inObject)
{
	/* Release allocated variables */
	XfdashboardBackgroundPrivate	*priv=XFDASHBOARD_BACKGROUND(inObject)->priv;

	if(priv->canvas)
	{
		g_object_unref(priv->canvas);
		priv->canvas=NULL;
	}

	if(priv->image)
	{
		g_object_unref(priv->image);
		priv->image=NULL;
	}

	if(priv->fillColor)
	{
		clutter_color_free(priv->fillColor);
		priv->fillColor=NULL;
	}

	if(priv->outlineColor)
	{
		clutter_color_free(priv->outlineColor);
		priv->outlineColor=NULL;
	}

	/* Call parent's class dispose method */
	G_OBJECT_CLASS(xfdashboard_background_parent_class)->dispose(inObject);
}

/* Set/get properties */
static void _xfdashboard_background_set_property(GObject *inObject,
													guint inPropID,
													const GValue *inValue,
													GParamSpec *inSpec)
{
	XfdashboardBackground			*self=XFDASHBOARD_BACKGROUND(inObject);

	switch(inPropID)
	{
		case PROP_TYPE:
			xfdashboard_background_set_background_type(self, g_value_get_flags(inValue));
			break;

		case PROP_FILL_COLOR:
			xfdashboard_background_set_fill_color(self, clutter_value_get_color(inValue));
			break;

		case PROP_OUTLINE_COLOR:
			xfdashboard_background_set_outline_color(self, clutter_value_get_color(inValue));
			break;

		case PROP_OUTLINE_WIDTH:
			xfdashboard_background_set_outline_width(self, g_value_get_float(inValue));
			break;

		case PROP_CORNERS:
			xfdashboard_background_set_corners(self, g_value_get_flags(inValue));
			break;

		case PROP_CORNERS_RADIUS:
			xfdashboard_background_set_corner_radius(self, g_value_get_float(inValue));
			break;

		case PROP_IMAGE:
			xfdashboard_background_set_image(self, CLUTTER_IMAGE(g_value_get_object(inValue)));
			break;

		default:
			G_OBJECT_WARN_INVALID_PROPERTY_ID(inObject, inPropID, inSpec);
			break;
	}
}

static void _xfdashboard_background_get_property(GObject *inObject,
													guint inPropID,
													GValue *outValue,
													GParamSpec *inSpec)
{
	XfdashboardBackground			*self=XFDASHBOARD_BACKGROUND(inObject);
	XfdashboardBackgroundPrivate	*priv=self->priv;

	switch(inPropID)
	{
		case PROP_TYPE:
			g_value_set_flags(outValue, priv->type);
			break;

		case PROP_FILL_COLOR:
			clutter_value_set_color(outValue, priv->fillColor);
			break;

		case PROP_OUTLINE_COLOR:
			clutter_value_set_color(outValue, priv->outlineColor);
			break;

		case PROP_OUTLINE_WIDTH:
			g_value_set_float(outValue, priv->outlineWidth);
			break;

		case PROP_CORNERS:
			g_value_set_flags(outValue, priv->corners);
			break;

		case PROP_CORNERS_RADIUS:
			g_value_set_float(outValue, priv->cornersRadius);
			break;

		case PROP_IMAGE:
			g_value_set_object(outValue, priv->image);
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
static void xfdashboard_background_class_init(XfdashboardBackgroundClass *klass)
{
	XfdashboardActorClass	*actorClass=XFDASHBOARD_ACTOR_CLASS(klass);
	ClutterActorClass		*clutterActorClass=CLUTTER_ACTOR_CLASS(klass);
	GObjectClass			*gobjectClass=G_OBJECT_CLASS(klass);

	/* Override functions */
	gobjectClass->dispose=_xfdashboard_background_dispose;
	gobjectClass->set_property=_xfdashboard_background_set_property;
	gobjectClass->get_property=_xfdashboard_background_get_property;

	clutterActorClass->paint_node=_xfdashboard_background_paint_node;
	clutterActorClass->get_preferred_width=_xfdashboard_background_get_preferred_width;
	clutterActorClass->get_preferred_height=_xfdashboard_background_get_preferred_height;
	clutterActorClass->allocate=_xfdashboard_background_allocate;

	/* Set up private structure */
	g_type_class_add_private(klass, sizeof(XfdashboardBackgroundPrivate));

	/* Define properties */
	XfdashboardBackgroundProperties[PROP_TYPE]=
		g_param_spec_flags("background-type",
							_("Background type"),
							_("Background type"),
							XFDASHBOARD_TYPE_BACKGROUND_TYPE,
							XFDASHBOARD_BACKGROUND_TYPE_NONE,
							G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS);

	XfdashboardBackgroundProperties[PROP_FILL_COLOR]=
		clutter_param_spec_color("background-fill-color",
									_("Background fill color"),
									_("Color to fill background with"),
									CLUTTER_COLOR_Black,
									G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS);

	XfdashboardBackgroundProperties[PROP_OUTLINE_COLOR]=
		clutter_param_spec_color("outline-color",
									_("Outline color"),
									_("Color to draw outline with"),
									CLUTTER_COLOR_White,
									G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS);

	XfdashboardBackgroundProperties[PROP_OUTLINE_WIDTH]=
		g_param_spec_float("outline-width",
							_("Outline width"),
							_("Width of line used to draw outline"),
							0.0f, G_MAXFLOAT,
							1.0f,
							G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS);

	XfdashboardBackgroundProperties[PROP_CORNERS]=
		g_param_spec_flags("corners",
							_("Corners"),
							_("Determines which corners are rounded"),
							XFDASHBOARD_TYPE_CORNERS,
							XFDASHBOARD_CORNERS_ALL,
							G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS);

	XfdashboardBackgroundProperties[PROP_CORNERS_RADIUS]=
		g_param_spec_float("corner-radius",
							_("Corner radius"),
							_("Radius of rounded corners"),
							0.0f, G_MAXFLOAT,
							0.0f,
							G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS);

	XfdashboardBackgroundProperties[PROP_IMAGE]=
		g_param_spec_object("image",
							_("Image"),
							_("Image to draw as background"),
							CLUTTER_TYPE_IMAGE,
							G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS);

	g_object_class_install_properties(gobjectClass, PROP_LAST, XfdashboardBackgroundProperties);

	/* Define stylable properties */
	xfdashboard_actor_install_stylable_property(actorClass, XfdashboardBackgroundProperties[PROP_TYPE]);
	xfdashboard_actor_install_stylable_property(actorClass, XfdashboardBackgroundProperties[PROP_FILL_COLOR]);
	xfdashboard_actor_install_stylable_property(actorClass, XfdashboardBackgroundProperties[PROP_OUTLINE_COLOR]);
	xfdashboard_actor_install_stylable_property(actorClass, XfdashboardBackgroundProperties[PROP_OUTLINE_WIDTH]);
	xfdashboard_actor_install_stylable_property(actorClass, XfdashboardBackgroundProperties[PROP_CORNERS]);
	xfdashboard_actor_install_stylable_property(actorClass, XfdashboardBackgroundProperties[PROP_CORNERS_RADIUS]);
	xfdashboard_actor_install_stylable_property(actorClass, XfdashboardBackgroundProperties[PROP_IMAGE]);
}

/* Object initialization
 * Create private structure and set up default values
 */
static void xfdashboard_background_init(XfdashboardBackground *self)
{
	XfdashboardBackgroundPrivate	*priv;

	priv=self->priv=XFDASHBOARD_BACKGROUND_GET_PRIVATE(self);

	/* This actor reacts on events */
	clutter_actor_set_reactive(CLUTTER_ACTOR(self), TRUE);

	/* Set up default values */
	priv->type=XFDASHBOARD_BACKGROUND_TYPE_NONE;
	priv->canvas=clutter_canvas_new();
	priv->fillColor=clutter_color_copy(CLUTTER_COLOR_Black);
	priv->outlineColor=clutter_color_copy(CLUTTER_COLOR_White);
	priv->outlineWidth=1.0f;
	priv->corners=XFDASHBOARD_CORNERS_ALL;
	priv->cornersRadius=0.0f;
	priv->image=NULL;

	/* Set up actor */
	clutter_actor_set_content_scaling_filters(CLUTTER_ACTOR(self),
												CLUTTER_SCALING_FILTER_TRILINEAR,
												CLUTTER_SCALING_FILTER_LINEAR);

	/* Connect signals */
	g_signal_connect_swapped(priv->canvas, "draw", G_CALLBACK(_xfdashboard_background_on_draw_canvas), self);
}

/* IMPLEMENTATION: Public API */

/* Create new actor */
ClutterActor* xfdashboard_background_new(void)
{
	return(g_object_new(XFDASHBOARD_TYPE_BACKGROUND,
						"background-type", XFDASHBOARD_BACKGROUND_TYPE_NONE,
						NULL));
}

/* Get/set type of background */
XfdashboardBackgroundType xfdashboard_background_get_background_type(XfdashboardBackground *self)
{
	g_return_val_if_fail(XFDASHBOARD_IS_BACKGROUND(self), XFDASHBOARD_BACKGROUND_TYPE_NONE);

	return(self->priv->type);
}

void xfdashboard_background_set_background_type(XfdashboardBackground *self, const XfdashboardBackgroundType inType)
{
	XfdashboardBackgroundPrivate	*priv;

	g_return_if_fail(XFDASHBOARD_IS_BACKGROUND(self));

	priv=self->priv;

	/* Set value if changed */
	if(priv->type!=inType)
	{
		/* Set value */
		priv->type=inType;
		clutter_content_invalidate(priv->canvas);

		/* Notify about property change */
		g_object_notify_by_pspec(G_OBJECT(self), XfdashboardBackgroundProperties[PROP_TYPE]);
	}
}

/* Get/set color to fill background with */
const ClutterColor* xfdashboard_background_get_fill_color(XfdashboardBackground *self)
{
	g_return_val_if_fail(XFDASHBOARD_IS_BACKGROUND(self), NULL);

	return(self->priv->fillColor);
}

void xfdashboard_background_set_fill_color(XfdashboardBackground *self, const ClutterColor *inColor)
{
	XfdashboardBackgroundPrivate	*priv;

	g_return_if_fail(XFDASHBOARD_IS_BACKGROUND(self));
	g_return_if_fail(inColor);

	priv=self->priv;

	/* Set value if changed */
	if(priv->fillColor==NULL || clutter_color_equal(inColor, priv->fillColor)==FALSE)
	{
		/* Set value */
		if(priv->fillColor) clutter_color_free(priv->fillColor);
		priv->fillColor=clutter_color_copy(inColor);

		/* Invalidate canvas to get it redrawn */
		if(priv->canvas) clutter_content_invalidate(priv->canvas);

		/* Notify about property change */
		g_object_notify_by_pspec(G_OBJECT(self), XfdashboardBackgroundProperties[PROP_FILL_COLOR]);
	}
}

/* Get/set color to draw outline with */
const ClutterColor* xfdashboard_background_get_outline_color(XfdashboardBackground *self)
{
	g_return_val_if_fail(XFDASHBOARD_IS_BACKGROUND(self), NULL);

	return(self->priv->outlineColor);
}

void xfdashboard_background_set_outline_color(XfdashboardBackground *self, const ClutterColor *inColor)
{
	XfdashboardBackgroundPrivate	*priv;

	g_return_if_fail(XFDASHBOARD_IS_BACKGROUND(self));
	g_return_if_fail(inColor);

	priv=self->priv;

	/* Set value if changed */
	if(priv->outlineColor==NULL || clutter_color_equal(inColor, priv->outlineColor)==FALSE)
	{
		/* Set value */
		if(priv->outlineColor) clutter_color_free(priv->outlineColor);
		priv->outlineColor=clutter_color_copy(inColor);

		/* Invalidate canvas to get it redrawn */
		if(priv->canvas) clutter_content_invalidate(priv->canvas);

		/* Notify about property change */
		g_object_notify_by_pspec(G_OBJECT(self), XfdashboardBackgroundProperties[PROP_OUTLINE_COLOR]);
	}
}

/* Get/set line width for outline */
gfloat xfdashboard_background_get_outline_width(XfdashboardBackground *self)
{
	g_return_val_if_fail(XFDASHBOARD_IS_BACKGROUND(self), 0.0f);

	return(self->priv->outlineWidth);
}

void xfdashboard_background_set_outline_width(XfdashboardBackground *self, const gfloat inWidth)
{
	XfdashboardBackgroundPrivate	*priv;

	g_return_if_fail(XFDASHBOARD_IS_BACKGROUND(self));
	g_return_if_fail(inWidth>=0.0f);

	priv=self->priv;

	/* Set value if changed */
	if(priv->outlineWidth!=inWidth)
	{
		/* Set value */
		priv->outlineWidth=inWidth;

		/* Invalidate canvas to get it redrawn */
		if(priv->canvas) clutter_content_invalidate(priv->canvas);

		/* Notify about property change */
		g_object_notify_by_pspec(G_OBJECT(self), XfdashboardBackgroundProperties[PROP_OUTLINE_WIDTH]);
	}
}

/* Get/set corners to draw rounded when drawing a rectangle */
XfdashboardCorners xfdashboard_background_get_corners(XfdashboardBackground *self)
{
	g_return_val_if_fail(XFDASHBOARD_IS_BACKGROUND(self), XFDASHBOARD_CORNERS_NONE);

	return(self->priv->corners);
}

void xfdashboard_background_set_corners(XfdashboardBackground *self, XfdashboardCorners inCorners)
{
	XfdashboardBackgroundPrivate	*priv;

	g_return_if_fail(XFDASHBOARD_IS_BACKGROUND(self));

	priv=self->priv;

	/* Set value if changed */
	if(priv->corners!=inCorners)
	{
		/* Set value */
		priv->corners=inCorners;

		/* Invalidate canvas to get it redrawn */
		if(priv->canvas) clutter_content_invalidate(priv->canvas);

		/* Notify about property change */
		g_object_notify_by_pspec(G_OBJECT(self), XfdashboardBackgroundProperties[PROP_CORNERS]);
	}
}

/* Get/set radius for rounded corners when drawing a rectangle */
gfloat xfdashboard_background_get_corner_radius(XfdashboardBackground *self)
{
	g_return_val_if_fail(XFDASHBOARD_IS_BACKGROUND(self), 0.0f);

	return(self->priv->cornersRadius);
}

void xfdashboard_background_set_corner_radius(XfdashboardBackground *self, const gfloat inRadius)
{
	XfdashboardBackgroundPrivate	*priv;

	g_return_if_fail(XFDASHBOARD_IS_BACKGROUND(self));
	g_return_if_fail(inRadius>=0.0f);

	priv=self->priv;

	/* Set value if changed */
	if(priv->cornersRadius!=inRadius)
	{
		/* Set value */
		priv->cornersRadius=inRadius;

		/* Invalidate canvas to get it redrawn */
		if(priv->canvas) clutter_content_invalidate(priv->canvas);

		/* Notify about property change */
		g_object_notify_by_pspec(G_OBJECT(self), XfdashboardBackgroundProperties[PROP_CORNERS_RADIUS]);
	}
}

/* Get/set image for background */
ClutterImage* xfdashboard_background_get_image(XfdashboardBackground *self)
{
	g_return_val_if_fail(XFDASHBOARD_IS_BACKGROUND(self), NULL);

	return(self->priv->image);
}

void xfdashboard_background_set_image(XfdashboardBackground *self, ClutterImage *inImage)
{
	XfdashboardBackgroundPrivate	*priv;

	g_return_if_fail(XFDASHBOARD_IS_BACKGROUND(self));
	g_return_if_fail(inImage==NULL || CLUTTER_IS_IMAGE(inImage));

	priv=self->priv;

	/* Set value if changed */
	if(priv->image!=inImage)
	{
		/* Set value */
		if(priv->image)
		{
			g_object_unref(priv->image);
			priv->image=NULL;
		}

		if(inImage) priv->image=CLUTTER_IMAGE(g_object_ref(inImage));

		/* Invalidate canvas to get it redrawn */
		if(priv->image) clutter_content_invalidate(CLUTTER_CONTENT(priv->image));

		/* Notify about property change */
		g_object_notify_by_pspec(G_OBJECT(self), XfdashboardBackgroundProperties[PROP_IMAGE]);
	}
}
