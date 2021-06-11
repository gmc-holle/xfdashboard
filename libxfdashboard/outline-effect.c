/*
 * outline-effect: Draws an outline on top of actor
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
	XfdashboardGradientColor	*color;
	gfloat						width;
	XfdashboardBorders			borders;
	XfdashboardCorners			corners;
	gfloat						cornersRadius;

	/* Instance related */
	CoglPipeline				*pipeline;
	CoglTexture					*texture;
	gint						drawLineWidth;
	gfloat						drawRadius;
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

/* Draw a single outline with current cairo context (line width, pattern etc.) */
static void _xfdashboard_outline_effect_draw_outline_intern(XfdashboardOutlineEffect *self,
															cairo_t *inContext,
															gint inWidth,
															gint inHeight,
															gfloat inOffset,
															gboolean inIsGradient)
{
	XfdashboardOutlineEffectPrivate		*priv;
	gfloat								penOffset;

	g_return_if_fail(XFDASHBOARD_IS_OUTLINE_EFFECT(self));
	g_return_if_fail(inWidth>0);
	g_return_if_fail(inHeight>0);
	g_return_if_fail(inOffset>=0.0f && inOffset<=self->priv->drawLineWidth);

	priv=self->priv;

	/* Calculate offset for pen width */
	if(inIsGradient) penOffset=0.0f;
		else penOffset=(priv->drawLineWidth / 2.0f);

	/* Set path for rounded borders and corners */
	if(priv->drawRadius>0.0f)
	{
		/* Top-left corner */
		if((priv->corners & XFDASHBOARD_CORNERS_TOP_LEFT) &&
			(priv->borders & XFDASHBOARD_BORDERS_LEFT) &&
			(priv->borders & XFDASHBOARD_BORDERS_TOP))
		{
			cairo_arc(inContext,
						priv->drawRadius+penOffset, priv->drawRadius+penOffset,
						priv->drawRadius-inOffset,
						M_PI, M_PI*1.5f);
			cairo_stroke(inContext);
		}

		/* Top border */
		if(priv->borders & XFDASHBOARD_BORDERS_TOP)
		{
			gfloat		offset1=0.0f;
			gfloat		offset2=0.0f;

			if(priv->corners & XFDASHBOARD_CORNERS_TOP_LEFT) offset1=priv->drawRadius;
			if(priv->corners & XFDASHBOARD_CORNERS_TOP_RIGHT) offset2=priv->drawRadius;

			cairo_move_to(inContext, offset1+penOffset, inOffset+penOffset);
			cairo_line_to(inContext, inWidth-offset2-penOffset, inOffset+penOffset);
			cairo_stroke(inContext);
		}

		/* Top-right corner */
		if((priv->corners & XFDASHBOARD_CORNERS_TOP_RIGHT) &&
			(priv->borders & XFDASHBOARD_BORDERS_TOP) &&
			(priv->borders & XFDASHBOARD_BORDERS_RIGHT))
		{
			cairo_arc(inContext,
						inWidth-priv->drawRadius-penOffset, priv->drawRadius+penOffset,
						priv->drawRadius-inOffset,
						M_PI*1.5f, M_PI*2.0f);
			cairo_stroke(inContext);
			cairo_stroke(inContext);
		}

		/* Right border */
		if(priv->borders & XFDASHBOARD_BORDERS_RIGHT)
		{
			gfloat		offset1=0.0f;
			gfloat		offset2=0.0f;

			if(priv->corners & XFDASHBOARD_CORNERS_TOP_RIGHT) offset1=priv->drawRadius;
			if(priv->corners & XFDASHBOARD_CORNERS_BOTTOM_RIGHT) offset2=priv->drawRadius;

			cairo_move_to(inContext, inWidth-inOffset-penOffset, offset1+penOffset);
			cairo_line_to(inContext, inWidth-inOffset-penOffset, inHeight-offset2-penOffset);
			cairo_stroke(inContext);
		}

		/* Bottom-right corner */
		if((priv->corners & XFDASHBOARD_CORNERS_BOTTOM_RIGHT) &&
			(priv->borders & XFDASHBOARD_BORDERS_RIGHT) &&
			(priv->borders & XFDASHBOARD_BORDERS_BOTTOM))
		{
			cairo_arc(inContext,
						inWidth-priv->drawRadius-penOffset, inHeight-priv->drawRadius-penOffset,
						priv->drawRadius-inOffset,
						0.0f, M_PI*0.5f);
			cairo_stroke(inContext);
		}

		/* Bottom border */
		if(priv->borders & XFDASHBOARD_BORDERS_BOTTOM)
		{
			gfloat		offset1=0.0f;
			gfloat		offset2=0.0f;

			if(priv->corners & XFDASHBOARD_CORNERS_BOTTOM_LEFT) offset1=priv->drawRadius;
			if(priv->corners & XFDASHBOARD_CORNERS_BOTTOM_RIGHT) offset2=priv->drawRadius;

			cairo_move_to(inContext, offset1+penOffset, inHeight-inOffset-penOffset);
			cairo_line_to(inContext, inWidth-offset2-penOffset, inHeight-inOffset-penOffset);
			cairo_stroke(inContext);
		}

		/* Bottom-left corner */
		if((priv->corners & XFDASHBOARD_CORNERS_BOTTOM_LEFT) &&
			(priv->borders & XFDASHBOARD_BORDERS_BOTTOM) &&
			(priv->borders & XFDASHBOARD_BORDERS_LEFT))
		{
			cairo_arc(inContext,
						priv->drawRadius+penOffset, inHeight-priv->drawRadius-penOffset,
						priv->drawRadius-inOffset,
						M_PI*0.5f, M_PI);
			cairo_stroke(inContext);
		}

		/* Left border */
		if(priv->borders & XFDASHBOARD_BORDERS_LEFT)
		{
			gfloat		offset1=0.0f;
			gfloat		offset2=0.0f;

			if(priv->corners & XFDASHBOARD_CORNERS_TOP_LEFT) offset1=priv->drawRadius;
			if(priv->corners & XFDASHBOARD_CORNERS_BOTTOM_LEFT) offset2=priv->drawRadius;

			cairo_move_to(inContext, inOffset+penOffset, offset1+penOffset);
			cairo_line_to(inContext, inOffset+penOffset, inHeight-offset2-penOffset);
			cairo_stroke(inContext);
		}
	}
		/* Set up path for flat lines without rounded corners */
		else
		{
			/* Top border */
			if(priv->borders & XFDASHBOARD_BORDERS_TOP)
			{
				cairo_move_to(inContext, inOffset+penOffset, inOffset+penOffset);
				cairo_line_to(inContext, inWidth-inOffset-penOffset, inOffset+penOffset);
				cairo_stroke(inContext);
			}

			/* Right border */
			if(priv->borders & XFDASHBOARD_BORDERS_RIGHT)
			{
				cairo_move_to(inContext, inWidth-inOffset-penOffset, inOffset+penOffset);
				cairo_line_to(inContext, inWidth-inOffset-penOffset, inHeight-inOffset-penOffset);
				cairo_stroke(inContext);
			}

			/* Bottom border */
			if(priv->borders & XFDASHBOARD_BORDERS_BOTTOM)
			{
				cairo_move_to(inContext, inOffset+penOffset, inHeight-inOffset-penOffset);
				cairo_line_to(inContext, inWidth-inOffset-penOffset, inHeight-inOffset-penOffset);
				cairo_stroke(inContext);
			}

			/* Left border */
			if(priv->borders & XFDASHBOARD_BORDERS_LEFT)
			{
				cairo_move_to(inContext, inOffset+penOffset, inOffset+penOffset);
				cairo_line_to(inContext, inOffset+penOffset, inHeight-inOffset-penOffset);
				cairo_stroke(inContext);
			}
		}
}

/* More complex drawing outline function for path gradient colors */
static void _xfdashboard_outline_effect_draw_outline_path_gradient(XfdashboardOutlineEffect *self,
																	cairo_t *inContext,
																	gint inWidth,
																	gint inHeight)
{
	XfdashboardOutlineEffectPrivate		*priv;
	ClutterColor						progressColor;
	gfloat								offset;
	gdouble								progress;

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

	/* Set line width */
	cairo_set_line_width(inContext, 1.0f);

	/* Draw rounded or flat rectangle in 0.5 pixel steps in line width and
	 * in color matching the progress. 0.5 pixel is chosen to mostly ensure
	 * that the lines drawn will slightly overlap to prevent "holes".
	 */
	offset=0.0f;
	while(offset<priv->drawLineWidth)
	{
		/* Calculate progress */
		progress=(offset/priv->drawLineWidth);

		/* Interpolate color according to progress */
		xfdashboard_gradient_color_interpolate(priv->color, progress, &progressColor);

		/* Set line color */
		clutter_cairo_set_source_color(inContext, &progressColor);

		/* Draw outline */
		_xfdashboard_outline_effect_draw_outline_intern(self, inContext, inWidth, inHeight, offset, TRUE);

		/* Adjust offset for next outline to draw */
		offset+=0.5f;
	}

	/* Draw last outline in final color at final position to ensure its visible */
	xfdashboard_gradient_color_interpolate(priv->color, 1.0, &progressColor);
	clutter_cairo_set_source_color(inContext, &progressColor);
	_xfdashboard_outline_effect_draw_outline_intern(self, inContext, inWidth, inHeight, priv->drawLineWidth, TRUE);
}

/* Create pattern for simple outline drawing function */
static cairo_pattern_t* _xfdashboard_outline_effect_create_pattern(XfdashboardOutlineEffect *self,
																	cairo_t *inContext,
																	int inWidth,
																	int inHeight)
{
	XfdashboardOutlineEffectPrivate		*priv;
	XfdashboardGradientType				type;
	cairo_pattern_t						*pattern;

	g_return_val_if_fail(XFDASHBOARD_IS_OUTLINE_EFFECT(self), NULL);

	priv=self->priv;
	pattern=NULL;

	/* Get type of gradient to create pattern for */
	type=xfdashboard_gradient_color_get_gradient_type(priv->color);

	/* Create solid pattern if gradient is solid type */
	if(type==XFDASHBOARD_GRADIENT_TYPE_SOLID)
	{
		const ClutterColor				*color;

		color=xfdashboard_gradient_color_get_solid_color(priv->color);
		pattern=cairo_pattern_create_rgba(CLAMP(color->red / 255.0, 0.0, 1.0),
											CLAMP(color->green / 255.0, 0.0, 1.0),
											CLAMP(color->blue / 255.0, 0.0, 1.0),
											CLAMP(color->alpha / 255.0, 0.0, 1.0));
	}

	/* Create linear gradient pattern if gradient is linear */
	if(type==XFDASHBOARD_GRADIENT_TYPE_LINEAR_GRADIENT)
	{
		gdouble						width, height;
		gdouble						midX, midY;
		gdouble						startX, startY;
		gdouble						endX, endY;
		gdouble						angle;
		gdouble						atanRectangle;
		gdouble						tanAngle;
		guint						i, stops;

		/* Adjust angle to find region */
		angle=(2*M_PI)-xfdashboard_gradient_color_get_angle(priv->color);
		while(angle<-M_PI) angle+=(2*M_PI);
		while(angle>M_PI) angle-=(2*M_PI);

		/* I do not know why but exactly at radians of 0.0 and M_PI the
		 * start and end points are mirrored so this is a dirty workaround
		 * to swap angles. Maybe it is because tan(angle) resolves to -0.0
		 * instead of 0.0. Looks like a sign error.
		 */
		if(angle==0.0) angle=M_PI;
			else if(angle==M_PI) angle=0.0;

		/* Get size of region */
		width=inWidth;
		height=inHeight;

		/* Find region and calculates points */
		atanRectangle=atan2(height, width);
		tanAngle=tan(angle);

		midX=(width/2.0);
		midY=(height/2.0);

		if(angle!=0.0 && (angle>-atanRectangle) && (angle<=atanRectangle))
		{
			/* Region 1 */
			startX=midX+(width/2.0);
			startY=midY-((width/2.0)*tanAngle);

			endX=midX-(width/2.0);
			endY=midY+((width/2.0)*tanAngle);
		}
			else if((angle>atanRectangle) && (angle<=(M_PI-atanRectangle)))
			{
				/* Region 2 */
				startX=midX+(height/(2.0*tanAngle));
				startY=midY-(height/2.0);

				endX=midX-(height/(2.0*tanAngle));
				endY=midY+(height/2.0);
			}
			else if(angle==0.0 || (angle>(M_PI-atanRectangle)) || (angle<=-(M_PI-atanRectangle)))
			{
				/* Region 3 */
				startX=midX-(width/2.0);
				startY=midY+((width/2.0)*tanAngle);

				endX=midX+(width/2.0);
				endY=midY-((width/2.0)*tanAngle);
			}
			else
			{
				/* Region 4 */
				startX=midX-(height/(2.0*tanAngle));
				startY=midY+(height/2.0);

				endX=midX+(height/(2.0*tanAngle));
				endY=midY-(height/2.0);
			}

		/* Reduce full length to requested length if gradient should be repeated */
		if(xfdashboard_gradient_color_get_repeat(priv->color))
		{
			gdouble					vectorX, vectorY;
			gdouble					distance;
			gdouble					requestedLength;

			/* Calculate distance between start and end point as this is the current
			 * length of pattern.
			 */
			vectorX=endX-startX;
			vectorY=endY-startY;
			distance=sqrt((vectorX*vectorX) + (vectorY*vectorY));

			/* Reduce distance by percentage or absolute length */
			requestedLength=xfdashboard_gradient_color_get_length(priv->color);
			if(requestedLength<0.0)
			{
				vectorX*=-requestedLength;
				vectorY*=-requestedLength;
			}
				else
				{
					vectorX=(vectorX/distance)*requestedLength;
					vectorY=(vectorY/distance)*requestedLength;
				}

			endX=startX+vectorX;
			endY=startY+vectorY;
		}

		/* Create pattern based on calculated points */
		pattern=cairo_pattern_create_linear(startX, startY, endX, endY);

		stops=xfdashboard_gradient_color_get_number_stops(priv->color);
		for(i=0; i<stops; i++)
		{
			gdouble					offset;
			ClutterColor			color;

			xfdashboard_gradient_color_get_stop(priv->color, i, &offset, &color);
			cairo_pattern_add_color_stop_rgba(pattern,
												offset,
												CLAMP(color.red / 255.0, 0.0, 1.0),
												CLAMP(color.green / 255.0, 0.0, 1.0),
												CLAMP(color.blue / 255.0, 0.0, 1.0),
												CLAMP(color.alpha / 255.0, 0.0, 1.0));
		}

		cairo_pattern_set_extend(pattern, xfdashboard_gradient_color_get_repeat(priv->color) ? CAIRO_EXTEND_REPEAT : CAIRO_EXTEND_PAD);
	}

	/* Create solid pattern if gradient is a path gradient as this should only
	 * happen if using the complex drawing function for outline with path gradients
	 * will not match and that's the line width is too small usually. Because of
	 * this small line width we will draw a single line with the end color of
	 * the path gradient.
	 */
	if(type==XFDASHBOARD_GRADIENT_TYPE_PATH_GRADIENT)
	{
		guint						lastIndex;
		ClutterColor				color;

		/* Get last index */
		lastIndex=xfdashboard_gradient_color_get_number_stops(priv->color);

		/* Set color from last color stop */
		xfdashboard_gradient_color_get_stop(priv->color, lastIndex-1, NULL, &color);
		pattern=cairo_pattern_create_rgba(CLAMP(color.red / 255.0, 0.0, 1.0),
											CLAMP(color.green / 255.0, 0.0, 1.0),
											CLAMP(color.blue / 255.0, 0.0, 1.0),
											CLAMP(color.alpha / 255.0, 0.0, 1.0));
	}

	/* Return created pattern */
	return(pattern);
}

/* Simple draw outline function which can use a cairo pattern */
static void _xfdashboard_outline_effect_draw_outline_simple(XfdashboardOutlineEffect *self,
															cairo_t *inContext,
															gint inWidth,
															gint inHeight)
{
	XfdashboardOutlineEffectPrivate		*priv;
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

	/* Set line width */
	cairo_set_line_width(inContext, priv->drawLineWidth);

	/* Create pattern for line color or gradient */
	pattern=_xfdashboard_outline_effect_create_pattern(self, inContext, inWidth, inHeight);

	/* Set line pattern */
	if(pattern) cairo_set_source(inContext, pattern);

	/* Draw outline */
	_xfdashboard_outline_effect_draw_outline_intern(self, inContext, inWidth, inHeight, 0.0f, FALSE);

	/* Release allocated resources */
	if(pattern) cairo_pattern_destroy(pattern);
}

/* Create texture with outline for actor */
static CoglTexture* _xfdashboard_outline_effect_create_texture(XfdashboardOutlineEffect *self,
																gint inWidth,
																gint inHeight)
{
	XfdashboardOutlineEffectPrivate		*priv;
	CoglContext							*coglContext;
	CoglBitmap							*bitmap;
	CoglBuffer							*buffer;
	CoglTexture							*texture;

	g_return_val_if_fail(XFDASHBOARD_IS_OUTLINE_EFFECT(self), NULL);
	g_return_val_if_fail(inWidth>0, NULL);
	g_return_val_if_fail(inHeight>0, NULL);

	priv=self->priv;
	texture=NULL;

	/* Set up bitmap buffer to draw outline into */
	coglContext=clutter_backend_get_cogl_context(clutter_get_default_backend());
	bitmap=cogl_bitmap_new_with_size(coglContext,
										inWidth,
										inHeight,
										CLUTTER_CAIRO_FORMAT_ARGB32);
	if(!bitmap) return(NULL);

	/* If buffer could be created and retrieved begin to draw */
	buffer=COGL_BUFFER(cogl_bitmap_get_buffer(bitmap));
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
																cogl_bitmap_get_rowstride(bitmap));
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

		/* Create drawing context and draw outline.
		 * If line width after rounding is below 2 pixels use the faster outline
		 * drawing function for solid colors as there will be no visual difference.
		 * Also use the faster outline drawing function if a solid color (inner,
		 * middle and outer color are equal) was set regardless of the line width.
		 * Otherwise use the slower but more accurate drawing function.
		 * Also check that corner radius (if not zero) is at least as large as line
		 * width.
		 */
		cairoContext=cairo_create(cairoSurface);

		priv->drawLineWidth=floor(priv->width+0.5f);
		priv->drawRadius=MAX(priv->cornersRadius, priv->drawLineWidth);

		if(priv->drawLineWidth<2 ||
			xfdashboard_gradient_color_get_gradient_type(priv->color)!=XFDASHBOARD_GRADIENT_TYPE_PATH_GRADIENT)
		{
			_xfdashboard_outline_effect_draw_outline_simple(self, cairoContext, inWidth, inHeight);
		}
			else
			{
				_xfdashboard_outline_effect_draw_outline_path_gradient(self, cairoContext, inWidth, inHeight);
			}

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

		/* Create sliced texture from buffer as it may get very large. It would
		 * be better to check for support of NPOT (non-power-of-two) textures
		 * and the maximum size of a texture supported. If both (NPOT and requested
		 * size is supported) use cogl_texture_2d_new_from_bitmap() instead of
		 * cogl_texture_2d_sliced_new_from_bitmap().
		 */
		texture=cogl_texture_2d_sliced_new_from_bitmap(bitmap, COGL_TEXTURE_MAX_WASTE);

		/* Destroy surface */
		cairo_surface_destroy(cairoSurface);
	}

	/* Release allocated resources */
	if(bitmap) cogl_object_unref(bitmap);

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

	/* Get size of outline to draw */
	clutter_actor_get_size(target, &width, &height);

	/* Do not draw outline if this effect is not enabled or if width or height
	 * is below 1 pixel as the texture will be invalid and causes trouble
	 * (like crashes) because neither width or height (or both) must be 0 pixels.
	 */
	if(!clutter_actor_meta_get_enabled(CLUTTER_ACTOR_META(self)) ||
		width<1.0f ||
		height<1.0f)
	{
		return;
	}

	/* Check if size has changed. If so, destroy texture to create a new one
	 * matching the new size.
	 */
	if(G_LIKELY(priv->texture))
	{
		if(cogl_texture_get_width(priv->texture)!=width ||
			cogl_texture_get_height(priv->texture)!=height)
		{
			_xfdashboard_outline_effect_invalidate(self);
		}
	}

	/* Create texture if no one is available */
	if(!priv->texture)
	{
		/* Create texture */
		priv->texture=_xfdashboard_outline_effect_create_texture(self, width, height);

		/* If we still have no texture, do nothing and return */
		if(!priv->texture) return;

		/* We only need to set texture at pipeline once after the texture
		 * was created.
		 */
		cogl_pipeline_set_layer_texture(priv->pipeline,
											0,
											priv->texture);
	}

	/* Draw texture to stage in actor's space */
	framebuffer=cogl_get_draw_framebuffer();
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
		xfdashboard_gradient_color_free(priv->color);
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
			xfdashboard_outline_effect_set_color(self, g_value_get_boxed(inValue));
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
			g_value_set_boxed(outValue, priv->color);
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
	static XfdashboardGradientColor	*defaultColor=NULL;

	/* Set up default value for param spec */
	if(G_UNLIKELY(!defaultColor))
	{
		defaultColor=xfdashboard_gradient_color_new_solid(CLUTTER_COLOR_White);
	}

	/* Override functions */
	gobjectClass->dispose=_xfdashboard_outline_effect_dispose;
	gobjectClass->set_property=_xfdashboard_outline_effect_set_property;
	gobjectClass->get_property=_xfdashboard_outline_effect_get_property;

	effectClass->paint=_xfdashboard_outline_effect_paint;

	/* Define properties */
	XfdashboardOutlineEffectProperties[PROP_COLOR]=
		xfdashboard_param_spec_gradient_color("color",
											"Color",
											"Color to draw outline with",
											defaultColor,
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
	priv->color=xfdashboard_gradient_color_new_solid(CLUTTER_COLOR_White);
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
const XfdashboardGradientColor* xfdashboard_outline_effect_get_color(XfdashboardOutlineEffect *self)
{
	g_return_val_if_fail(XFDASHBOARD_IS_OUTLINE_EFFECT(self), NULL);

	return(self->priv->color);
}

void xfdashboard_outline_effect_set_color(XfdashboardOutlineEffect *self, const XfdashboardGradientColor *inColor)
{
	XfdashboardOutlineEffectPrivate	*priv;

	g_return_if_fail(XFDASHBOARD_IS_OUTLINE_EFFECT(self));
	g_return_if_fail(inColor);

	priv=self->priv;

	/* Set value if changed */
	if(priv->color==NULL ||
		!xfdashboard_gradient_color_equal(inColor, priv->color))
	{
		/* Set value */
		if(priv->color) xfdashboard_gradient_color_free(priv->color);
		priv->color=xfdashboard_gradient_color_copy(inColor);

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

void xfdashboard_outline_effect_set_borders(XfdashboardOutlineEffect *self, const XfdashboardBorders inBorders)
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

void xfdashboard_outline_effect_set_corners(XfdashboardOutlineEffect *self, const XfdashboardCorners inCorners)
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
