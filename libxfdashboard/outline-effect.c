/*
 * outline-effect: Draws an outline on top of actor
 * 
 * Copyright 2012-2020 Stephan Haller <nomad@froevel.de>
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

#define COGL_ENABLE_EXPERIMENTAL_API
#define CLUTTER_ENABLE_EXPERIMENTAL_API

#include <libxfdashboard/outline-effect.h>

#include <glib/gi18n-lib.h>
#include <math.h>

#include <libxfdashboard/enums.h>
#include <libxfdashboard/compat.h>


/* Define this class in GObject system */
struct _XfdashboardOutlineEffectPrivate
{
	/* Properties related */
	ClutterColor				*color;
	gfloat						width;
	XfdashboardBorders			borders;
	XfdashboardCorners			corners;
	gfloat						cornersRadius;

	/* Instance related */
	CoglPipeline				*pipeline;
	CoglTexture					*texture;
};

G_DEFINE_TYPE_WITH_PRIVATE(XfdashboardOutlineEffect,
							xfdashboard_outline_effect,
							CLUTTER_TYPE_EFFECT)

/* Properties */
enum
{
	PROP_0,

	PROP_COLOR,
	PROP_WIDTH,
	PROP_BORDERS,
	PROP_CORNERS,
	PROP_CORNERS_RADIUS,

	PROP_LAST
};

static GParamSpec* XfdashboardOutlineEffectProperties[PROP_LAST]={ 0, };

/* IMPLEMENTATION: Private variables and methods */

static CoglPipeline		*_xfdashboard_outline_effect_base_pipeline=NULL;

/* Invalidate cached texture */
static void _xfdashboard_outline_effect_invalidate(XfdashboardOutlineEffect *self)
{
	XfdashboardOutlineEffectPrivate		*priv;

	g_return_if_fail(XFDASHBOARD_IS_OUTLINE_EFFECT(self));

	priv=self->priv;

	/* Release texture */
	if(priv->texture)
	{
		cogl_object_unref(priv->texture);
		priv->texture=NULL;
	}
}

/* Draw outline */
static void _xfdashboard_outline_effect_draw_outline(XfdashboardOutlineEffect *self,
														cairo_t *inContext,
														gint inWidth,
														gint inHeight)
{
	XfdashboardOutlineEffectPrivate		*priv;
	gfloat								lineWidth;
	cairo_pattern_t						*pattern;

	g_return_if_fail(XFDASHBOARD_IS_OUTLINE_EFFECT(self));
	g_return_if_fail(inWidth>0);
	g_return_if_fail(inHeight>0);

	priv=self->priv;

	/* Clear current contents of the canvas */
	cairo_save(inContext);
	cairo_set_operator(inContext, CAIRO_OPERATOR_CLEAR);
	cairo_paint(inContext);
	cairo_restore(inContext);

	cairo_set_operator(inContext, CAIRO_OPERATOR_OVER);

	/* Set path for rounded borders and corners */
	if(priv->cornersRadius>0.0f)
	{
		/* Top-left corner */
		if(priv->corners & XFDASHBOARD_CORNERS_TOP_LEFT &&
			priv->borders & XFDASHBOARD_BORDERS_LEFT &&
			priv->borders & XFDASHBOARD_BORDERS_TOP)
		{
			cairo_move_to(inContext, 0, priv->cornersRadius);
			cairo_arc(inContext, priv->cornersRadius, priv->cornersRadius, priv->cornersRadius, M_PI, M_PI*1.5f);
		}

		/* Top border */
		if(priv->borders & XFDASHBOARD_BORDERS_TOP)
		{
			gfloat		offset1=0.0f;
			gfloat		offset2=0.0f;

			if(priv->corners & XFDASHBOARD_CORNERS_TOP_LEFT) offset1=priv->cornersRadius;
			if(priv->corners & XFDASHBOARD_CORNERS_TOP_RIGHT) offset2=priv->cornersRadius;

			cairo_move_to(inContext, offset1, 0);
			cairo_line_to(inContext, inWidth-offset2, 0);
		}

		/* Top-right corner */
		if(priv->corners & XFDASHBOARD_CORNERS_TOP_RIGHT &&
			priv->borders & XFDASHBOARD_BORDERS_TOP &&
			priv->borders & XFDASHBOARD_BORDERS_RIGHT)
		{
			cairo_move_to(inContext, inWidth-priv->cornersRadius, 0);
			cairo_arc(inContext, inWidth-priv->cornersRadius, priv->cornersRadius, priv->cornersRadius, M_PI*1.5f, M_PI*2.0f);
		}

		/* Right border */
		if(priv->borders & XFDASHBOARD_BORDERS_RIGHT)
		{
			gfloat		offset1=0.0f;
			gfloat		offset2=0.0f;

			if(priv->corners & XFDASHBOARD_CORNERS_TOP_RIGHT) offset1=priv->cornersRadius;
			if(priv->corners & XFDASHBOARD_CORNERS_BOTTOM_RIGHT) offset2=priv->cornersRadius;

			cairo_move_to(inContext, inWidth, offset1);
			cairo_line_to(inContext, inWidth, inHeight-offset2);
		}

		/* Bottom-right corner */
		if(priv->corners & XFDASHBOARD_CORNERS_BOTTOM_RIGHT &&
			priv->borders & XFDASHBOARD_BORDERS_RIGHT &&
			priv->borders & XFDASHBOARD_BORDERS_BOTTOM)
		{
			cairo_move_to(inContext, inWidth, inHeight-priv->cornersRadius);
			cairo_arc(inContext, inWidth-priv->cornersRadius, inHeight-priv->cornersRadius, priv->cornersRadius, 0.0f, M_PI*0.5f);
		}

		/* Bottom border */
		if(priv->borders & XFDASHBOARD_BORDERS_BOTTOM)
		{
			gfloat		offset1=0.0f;
			gfloat		offset2=0.0f;

			if(priv->corners & XFDASHBOARD_CORNERS_BOTTOM_LEFT) offset1=priv->cornersRadius;
			if(priv->corners & XFDASHBOARD_CORNERS_BOTTOM_RIGHT) offset2=priv->cornersRadius;

			cairo_move_to(inContext, offset1, inHeight);
			cairo_line_to(inContext, inWidth-offset2, inHeight);
		}

		/* Bottom-left corner */
		if(priv->corners & XFDASHBOARD_CORNERS_BOTTOM_LEFT &&
			priv->borders & XFDASHBOARD_BORDERS_BOTTOM &&
			priv->borders & XFDASHBOARD_BORDERS_LEFT)
		{
			cairo_move_to(inContext, priv->cornersRadius, inHeight);
			cairo_arc(inContext, priv->cornersRadius, inHeight-priv->cornersRadius, priv->cornersRadius, M_PI*0.5f, M_PI);
		}

		/* Left border */
		if(priv->borders & XFDASHBOARD_BORDERS_LEFT)
		{
			gfloat		offset1=0.0f;
			gfloat		offset2=0.0f;

			if(priv->corners & XFDASHBOARD_CORNERS_TOP_LEFT) offset1=priv->cornersRadius;
			if(priv->corners & XFDASHBOARD_CORNERS_BOTTOM_LEFT) offset2=priv->cornersRadius;

			cairo_move_to(inContext, 0, offset1);
			cairo_line_to(inContext, 0, inHeight-offset2);
		}
	}
		/* Set up path for flat lines without rounded corners */
		else
		{
			/* Top border */
			if(priv->borders & XFDASHBOARD_BORDERS_TOP)
			{
				cairo_move_to(inContext, 0, 0);
				cairo_line_to(inContext, inWidth, 0);
			}

			/* Right border */
			if(priv->borders & XFDASHBOARD_BORDERS_RIGHT)
			{
				cairo_move_to(inContext, inWidth, 0);
				cairo_line_to(inContext, inWidth, inHeight);
			}

			/* Bottom border */
			if(priv->borders & XFDASHBOARD_BORDERS_BOTTOM)
			{
				cairo_move_to(inContext, 0, inHeight);
				cairo_line_to(inContext, inWidth, inHeight);
			}

			/* Left border */
			if(priv->borders & XFDASHBOARD_BORDERS_LEFT)
			{
				cairo_move_to(inContext, 0, 0);
				cairo_line_to(inContext, 0, inHeight);
			}
		}

	/* Round line width for better looking and check if we can draw
	 * outline with configured line width. That means it needs to be
	 * greater or equal to 1.0
	 */
	lineWidth=floor(priv->width+0.5);
	cairo_set_line_width(inContext, lineWidth);

	/* Set line color(s) */
	pattern=cairo_pattern_create_rgba(priv->color->red / 255.0f,
										priv->color->green / 255.0f,
										priv->color->blue / 255.0f,
										priv->color->alpha / 255.0f);
	// TODO: Gradient!
	cairo_set_source(inContext, pattern);

	/* Draw outline by path created with pattern set up */
	cairo_stroke(inContext);

	/* Release allocated resources */
	cairo_pattern_destroy(pattern);
}

/* Create texture with outline for actor */
static CoglTexture* _xfdashboard_outline_effect_create_texture(XfdashboardOutlineEffect *self,
																gfloat inWidth,
																gfloat inHeight)
{
	CoglContext							*coglContext;
	CoglBitmap							*bitmapBuffer;
	CoglBuffer							*buffer;
	CoglTexture							*texture;

	g_return_val_if_fail(XFDASHBOARD_IS_OUTLINE_EFFECT(self), NULL);
	g_return_val_if_fail(inWidth>=1.0f, NULL);
	g_return_val_if_fail(inHeight>=1.0f, NULL);

	texture=NULL;

	/* Set up bitmap buffer to draw outline into */
	coglContext=clutter_backend_get_cogl_context(clutter_get_default_backend());
	bitmapBuffer=cogl_bitmap_new_with_size(coglContext,
											inWidth,
											inHeight,
											CLUTTER_CAIRO_FORMAT_ARGB32);
	if(!bitmapBuffer) return(NULL);

	/* If buffer could be created and retrieved begin to draw */
	buffer=COGL_BUFFER(cogl_bitmap_get_buffer(bitmapBuffer));
	if(buffer)
	{
		gpointer						bufferData;
		gboolean						mappedBuffer;
		cairo_t							*cairoContext;
		cairo_surface_t					*cairoSurface;

		/* Tell cogl that this buffer may change from time to time */
		cogl_buffer_set_update_hint(buffer, COGL_BUFFER_UPDATE_HINT_DYNAMIC);

		/* Create surface but try to map data of buffer for direct access which is faster */
		bufferData=cogl_buffer_map(buffer, COGL_BUFFER_ACCESS_READ_WRITE, COGL_BUFFER_MAP_HINT_DISCARD);
		if(bufferData)
		{
			cairoSurface=cairo_image_surface_create_for_data(bufferData,
																CAIRO_FORMAT_ARGB32,
																inWidth,
																inHeight,
																cogl_bitmap_get_rowstride(bitmapBuffer));
			mappedBuffer=TRUE;
		}
			/* Buffer could not be mapped, so create a surface whose data we have
			 * to copy once the outline is drawn.
			 */
			else
			{
				cairoSurface=cairo_image_surface_create(CAIRO_FORMAT_ARGB32,
														inWidth,
														inHeight);

				mappedBuffer=FALSE;
			}

		/* Create drawing context and draw outline */
		cairoContext=cairo_create(cairoSurface);
		_xfdashboard_outline_effect_draw_outline(self, cairoContext, inWidth, inHeight);
		cairo_destroy(cairoContext);

		/* If buffer could be mapped before, unmap it now */
		if(mappedBuffer)
		{
			cogl_buffer_unmap(buffer);
		}
			/* Otherwise copy surface data to buffer now */
			else
			{
				cogl_buffer_set_data(buffer,
										0,
										cairo_image_surface_get_data(cairoSurface),
										cairo_image_surface_get_stride(cairoSurface)*inHeight);
			}

		/* Create texture from buffer */
		texture=cogl_texture_2d_new_from_bitmap(bitmapBuffer);

		/* Destroy surface */
		cairo_surface_destroy(cairoSurface);
	}

	/* Release allocated resources */
	if(bitmapBuffer) cogl_object_unref(bitmapBuffer);

	/* Return created texture */
	return(texture);
}

/* Draw effect after actor was drawn */
static void _xfdashboard_outline_effect_paint(ClutterEffect *inEffect, ClutterEffectPaintFlags inFlags)
{
	XfdashboardOutlineEffect			*self;
	XfdashboardOutlineEffectPrivate		*priv;
	ClutterActor						*target;
	gfloat								width, height;
	CoglFramebuffer						*framebuffer;

	g_return_if_fail(XFDASHBOARD_IS_OUTLINE_EFFECT(inEffect));

	self=XFDASHBOARD_OUTLINE_EFFECT(inEffect);
	priv=self->priv;

	/* Chain to the next item in the paint sequence */
	target=clutter_actor_meta_get_actor(CLUTTER_ACTOR_META(self));
	clutter_actor_continue_paint(target);

	/* Do not draw outline if this effect is not enabled */
	if(!clutter_actor_meta_get_enabled(CLUTTER_ACTOR_META(self))) return;

	/* Get and check size of outline to draw */
	clutter_actor_get_size(target, &width, &height);

	/* If dirty flag was passed, invalidate cached texture to enforce
	 * creation of a new texture for outline.
	 */
	if(inFlags & CLUTTER_EFFECT_PAINT_ACTOR_DIRTY)
	{
		_xfdashboard_outline_effect_invalidate(self);
	}

	/* Create texture if no one is available */
	if(!priv->texture)
	{
		/* Create texture */
		priv->texture=_xfdashboard_outline_effect_create_texture(self, width, height);

		/* If we still have no texture, do nothing and return */
		if(!priv->texture) return;
	}

	/* Draw texture to stage in actor's space */
	framebuffer=cogl_get_draw_framebuffer();

	cogl_pipeline_set_layer_texture(priv->pipeline,
										0,
										priv->texture);

	cogl_framebuffer_draw_textured_rectangle(framebuffer,
												priv->pipeline,
												0, 0, width, height,
												0.0f, 0.0f, 1.0f, 1.0f);
}


/* IMPLEMENTATION: GObject */

/* Dispose this object */
static void _xfdashboard_outline_effect_dispose(GObject *inObject)
{
	/* Release allocated variables */
	XfdashboardOutlineEffect			*self=XFDASHBOARD_OUTLINE_EFFECT(inObject);
	XfdashboardOutlineEffectPrivate		*priv=self->priv;

	if(priv->texture)
	{
		cogl_object_unref(priv->texture);
		priv->texture=NULL;
	}

	if(priv->pipeline)
	{
		cogl_object_unref(priv->pipeline);
		priv->pipeline=NULL;
	}

	if(priv->color)
	{
		clutter_color_free(priv->color);
		priv->color=NULL;
	}

	/* Call parent's class dispose method */
	G_OBJECT_CLASS(xfdashboard_outline_effect_parent_class)->dispose(inObject);
}

/* Set/get properties */
static void _xfdashboard_outline_effect_set_property(GObject *inObject,
														guint inPropID,
														const GValue *inValue,
														GParamSpec *inSpec)
{
	XfdashboardOutlineEffect			*self=XFDASHBOARD_OUTLINE_EFFECT(inObject);

	switch(inPropID)
	{
		case PROP_COLOR:
			xfdashboard_outline_effect_set_color(self, clutter_value_get_color(inValue));
			break;

		case PROP_WIDTH:
			xfdashboard_outline_effect_set_width(self, g_value_get_float(inValue));
			break;

		case PROP_BORDERS:
			xfdashboard_outline_effect_set_borders(self, g_value_get_flags(inValue));
			break;

		case PROP_CORNERS:
			xfdashboard_outline_effect_set_corners(self, g_value_get_flags(inValue));
			break;

		case PROP_CORNERS_RADIUS:
			xfdashboard_outline_effect_set_corner_radius(self, g_value_get_float(inValue));
			break;

		default:
			G_OBJECT_WARN_INVALID_PROPERTY_ID(inObject, inPropID, inSpec);
			break;
	}
}

static void _xfdashboard_outline_effect_get_property(GObject *inObject,
														guint inPropID,
														GValue *outValue,
														GParamSpec *inSpec)
{
	XfdashboardOutlineEffect			*self=XFDASHBOARD_OUTLINE_EFFECT(inObject);
	XfdashboardOutlineEffectPrivate		*priv=self->priv;

	switch(inPropID)
	{
		case PROP_COLOR:
			clutter_value_set_color(outValue, priv->color);
			break;

		case PROP_WIDTH:
			g_value_set_float(outValue, priv->width);
			break;

		case PROP_BORDERS:
			g_value_set_flags(outValue, priv->borders);
			break;

		case PROP_CORNERS:
			g_value_set_flags(outValue, priv->corners);
			break;

		case PROP_CORNERS_RADIUS:
			g_value_set_float(outValue, priv->cornersRadius);
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
static void xfdashboard_outline_effect_class_init(XfdashboardOutlineEffectClass *klass)
{
	ClutterEffectClass				*effectClass=CLUTTER_EFFECT_CLASS(klass);
	GObjectClass					*gobjectClass=G_OBJECT_CLASS(klass);

	/* Override functions */
	gobjectClass->dispose=_xfdashboard_outline_effect_dispose;
	gobjectClass->set_property=_xfdashboard_outline_effect_set_property;
	gobjectClass->get_property=_xfdashboard_outline_effect_get_property;

	effectClass->paint=_xfdashboard_outline_effect_paint;

	/* Define properties */
	XfdashboardOutlineEffectProperties[PROP_COLOR]=
		clutter_param_spec_color("color",
									"Color",
									"Color to draw outline with",
									CLUTTER_COLOR_White,
									G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS);

	XfdashboardOutlineEffectProperties[PROP_WIDTH]=
		g_param_spec_float("width",
							"Width",
							"Width of line used to draw outline",
							0.0f, G_MAXFLOAT,
							1.0f,
							G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS);

	XfdashboardOutlineEffectProperties[PROP_BORDERS]=
		g_param_spec_flags("borders",
							"Borders",
							"Determines which sides of the border to draw",
							XFDASHBOARD_TYPE_BORDERS,
							XFDASHBOARD_BORDERS_ALL,
							G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS);

	XfdashboardOutlineEffectProperties[PROP_CORNERS]=
		g_param_spec_flags("corners",
							"Corners",
							"Determines which corners are rounded",
							XFDASHBOARD_TYPE_CORNERS,
							XFDASHBOARD_CORNERS_ALL,
							G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS);

	XfdashboardOutlineEffectProperties[PROP_CORNERS_RADIUS]=
		g_param_spec_float("corner-radius",
							"Corner radius",
							"Radius of rounded corners",
							0.0f, G_MAXFLOAT,
							0.0f,
							G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS);

	g_object_class_install_properties(gobjectClass, PROP_LAST, XfdashboardOutlineEffectProperties);
}

/* Object initialization
 * Create private structure and set up default values
 */
static void xfdashboard_outline_effect_init(XfdashboardOutlineEffect *self)
{
	XfdashboardOutlineEffectPrivate	*priv;

	priv=self->priv=xfdashboard_outline_effect_get_instance_private(self);

	/* Set up default values */
	priv->color=clutter_color_copy(CLUTTER_COLOR_White);
	priv->width=1.0f;
	priv->borders=XFDASHBOARD_BORDERS_ALL;
	priv->corners=XFDASHBOARD_CORNERS_ALL;
	priv->cornersRadius=0.0f;
	priv->texture=NULL;

	/* Set up pipeline */
	if(G_UNLIKELY(!_xfdashboard_outline_effect_base_pipeline))
    {
		CoglContext					*context;

		/* Get context to create base pipeline */
		context=clutter_backend_get_cogl_context(clutter_get_default_backend());

		/* Create base pipeline */
		_xfdashboard_outline_effect_base_pipeline=cogl_pipeline_new(context);
		cogl_pipeline_set_layer_null_texture(_xfdashboard_outline_effect_base_pipeline,
												0, /* layer number */
												COGL_TEXTURE_TYPE_2D);
	}

	priv->pipeline=cogl_pipeline_copy(_xfdashboard_outline_effect_base_pipeline);
}

/* IMPLEMENTATION: Public API */

/* Create new actor */
ClutterEffect* xfdashboard_outline_effect_new(void)
{
	return(g_object_new(XFDASHBOARD_TYPE_OUTLINE_EFFECT, NULL));
}

/* Get/set color to draw outline with */
const ClutterColor* xfdashboard_outline_effect_get_color(XfdashboardOutlineEffect *self)
{
	g_return_val_if_fail(XFDASHBOARD_IS_OUTLINE_EFFECT(self), NULL);

	return(self->priv->color);
}

void xfdashboard_outline_effect_set_color(XfdashboardOutlineEffect *self, const ClutterColor *inColor)
{
	XfdashboardOutlineEffectPrivate	*priv;

	g_return_if_fail(XFDASHBOARD_IS_OUTLINE_EFFECT(self));
	g_return_if_fail(inColor);

	priv=self->priv;

	/* Set value if changed */
	if(priv->color==NULL || clutter_color_equal(inColor, priv->color)==FALSE)
	{
		/* Set value */
		if(priv->color) clutter_color_free(priv->color);
		priv->color=clutter_color_copy(inColor);

		/* Invalidate effect to get it redrawn */
		_xfdashboard_outline_effect_invalidate(self);
		clutter_effect_queue_repaint(CLUTTER_EFFECT(self));

		/* Notify about property change */
		g_object_notify_by_pspec(G_OBJECT(self), XfdashboardOutlineEffectProperties[PROP_COLOR]);
	}
}

/* Get/set line width for outline */
gfloat xfdashboard_outline_effect_get_width(XfdashboardOutlineEffect *self)
{
	g_return_val_if_fail(XFDASHBOARD_IS_OUTLINE_EFFECT(self), 0.0f);

	return(self->priv->width);
}

void xfdashboard_outline_effect_set_width(XfdashboardOutlineEffect *self, const gfloat inWidth)
{
	XfdashboardOutlineEffectPrivate	*priv;

	g_return_if_fail(XFDASHBOARD_IS_OUTLINE_EFFECT(self));
	g_return_if_fail(inWidth>=0.0f);

	priv=self->priv;

	/* Set value if changed */
	if(priv->width!=inWidth)
	{
		/* Set value */
		priv->width=inWidth;

		/* Invalidate effect to get it redrawn */
		_xfdashboard_outline_effect_invalidate(self);
		clutter_effect_queue_repaint(CLUTTER_EFFECT(self));

		/* Notify about property change */
		g_object_notify_by_pspec(G_OBJECT(self), XfdashboardOutlineEffectProperties[PROP_WIDTH]);
	}
}

/* Get/set sides of border to draw  */
XfdashboardBorders xfdashboard_outline_effect_get_borders(XfdashboardOutlineEffect *self)
{
	g_return_val_if_fail(XFDASHBOARD_IS_OUTLINE_EFFECT(self), XFDASHBOARD_BORDERS_NONE);

	return(self->priv->borders);
}

void xfdashboard_outline_effect_set_borders(XfdashboardOutlineEffect *self, XfdashboardBorders inBorders)
{
	XfdashboardOutlineEffectPrivate	*priv;

	g_return_if_fail(XFDASHBOARD_IS_OUTLINE_EFFECT(self));

	priv=self->priv;

	/* Set value if changed */
	if(priv->borders!=inBorders)
	{
		/* Set value */
		priv->borders=inBorders;

		/* Invalidate effect to get it redrawn */
		_xfdashboard_outline_effect_invalidate(self);
		clutter_effect_queue_repaint(CLUTTER_EFFECT(self));

		/* Notify about property change */
		g_object_notify_by_pspec(G_OBJECT(self), XfdashboardOutlineEffectProperties[PROP_BORDERS]);
	}
}

/* Get/set corners of rectangle to draw rounded */
XfdashboardCorners xfdashboard_outline_effect_get_corners(XfdashboardOutlineEffect *self)
{
	g_return_val_if_fail(XFDASHBOARD_IS_OUTLINE_EFFECT(self), XFDASHBOARD_CORNERS_NONE);

	return(self->priv->corners);
}

void xfdashboard_outline_effect_set_corners(XfdashboardOutlineEffect *self, XfdashboardCorners inCorners)
{
	XfdashboardOutlineEffectPrivate	*priv;

	g_return_if_fail(XFDASHBOARD_IS_OUTLINE_EFFECT(self));

	priv=self->priv;

	/* Set value if changed */
	if(priv->corners!=inCorners)
	{
		/* Set value */
		priv->corners=inCorners;

		/* Invalidate effect to get it redrawn */
		_xfdashboard_outline_effect_invalidate(self);
		clutter_effect_queue_repaint(CLUTTER_EFFECT(self));

		/* Notify about property change */
		g_object_notify_by_pspec(G_OBJECT(self), XfdashboardOutlineEffectProperties[PROP_CORNERS]);
	}
}

/* Get/set radius for rounded corners */
gfloat xfdashboard_outline_effect_get_corner_radius(XfdashboardOutlineEffect *self)
{
	g_return_val_if_fail(XFDASHBOARD_IS_OUTLINE_EFFECT(self), 0.0f);

	return(self->priv->cornersRadius);
}

void xfdashboard_outline_effect_set_corner_radius(XfdashboardOutlineEffect *self, const gfloat inRadius)
{
	XfdashboardOutlineEffectPrivate	*priv;

	g_return_if_fail(XFDASHBOARD_IS_OUTLINE_EFFECT(self));
	g_return_if_fail(inRadius>=0.0f);

	priv=self->priv;

	/* Set value if changed */
	if(priv->cornersRadius!=inRadius)
	{
		/* Set value */
		priv->cornersRadius=inRadius;

		/* Invalidate effect to get it redrawn */
		_xfdashboard_outline_effect_invalidate(self);
		clutter_effect_queue_repaint(CLUTTER_EFFECT(self));

		/* Notify about property change */
		g_object_notify_by_pspec(G_OBJECT(self), XfdashboardOutlineEffectProperties[PROP_CORNERS_RADIUS]);
	}
}
