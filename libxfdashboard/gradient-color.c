/*
 * gradient-color: A special purpose boxed color type for solid colors
 *                 or gradients
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

/**
 * SECTION:gradient-color
 * @short_description: Solid colors and gradients
 * @include: xfdashboard/gradient-color.h
 *
 * XfdashboardGradientColor is a boxed type that represents customized
 * colors, i.e. either a single-colored solid one or gradients.
 *
 * It is mostly a result of parsing a color expression at CSS. The type
 * of the color can be obtained with xfdashboard_gradient_color_get_color_type().
 * To obtain the color for a solid one use xfdashboard_gradient_color_get_solid_color(). 
 * To obtain the gradient represented by a #XfdashboardGradientColor, it has
 * to be interpolated by the requested progress with
 * xfdashboard_gradient_color_interpolate(). 
 *
 * It is not necessary normally to deal directly with #XfdashboardGradientColor,
 * since they are mostly used behind the scenes at theme CSS styles and
 * by actors supporting gradients like #XfdashboardBackground.
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <libxfdashboard/gradient-color.h>

#include <glib/gi18n-lib.h>
#include <math.h>

/* Forward declarions */
static void _xfdashboard_gradient_color_transform_from_string(const GValue *inSourceValue, GValue *ioDestValue);
static void _xfdashboard_gradient_color_transform_to_string(const GValue *inSourceValue, GValue *ioDestValue);


/* Define this boxed type in GObject system */
struct _XfdashboardGradientColor
{
	XfdashboardGradientType			type;
	ClutterColor					*solid;
	GArray							*stops;
	gdouble							gradientAngle;
	gboolean						gradientRepeat;
	gdouble							gradientLength;
};

G_DEFINE_BOXED_TYPE_WITH_CODE(XfdashboardGradientColor,
								xfdashboard_gradient_color,
								xfdashboard_gradient_color_copy,
								xfdashboard_gradient_color_free,
								{
									g_value_register_transform_func(G_TYPE_STRING, g_define_type_id, _xfdashboard_gradient_color_transform_from_string);
									g_value_register_transform_func(g_define_type_id, G_TYPE_STRING, _xfdashboard_gradient_color_transform_to_string);
								} );


/* IMPLEMENTATION: Private variables and methods */

typedef struct _XfdashboardGradientColorStop 		XfdashboardGradientColorStop;
struct _XfdashboardGradientColorStop
{
	gdouble							offset;
	ClutterColor					color;
};

/* Parse a string into a XfdashboardGradientColor value */
static gboolean _xfdashboard_gradient_color_transform_from_string_parse(gchar **inTokens, XfdashboardGradientColor **outColor)
{
	XfdashboardGradientType			type;

	g_return_val_if_fail(inTokens, FALSE);
	g_return_val_if_fail(*inTokens, FALSE);
	g_return_val_if_fail(outColor, FALSE);
	g_return_val_if_fail(*outColor==NULL, FALSE);

#define NEXT_TOKEN(x)	while(*(x) && !(*(*(x)))) { (x)++; }
#define NEED_TOKEN(x)	NEXT_TOKEN(x); if(!*(x)) return(FALSE);

	/* We first expect the type of this color whereby "solid" is optional.
	 * We expect more tokens to parse.
	 */
	NEED_TOKEN(inTokens);

	type=XFDASHBOARD_GRADIENT_TYPE_NONE;
	if(g_strcmp0(*inTokens, "solid")==0)
	{
		type=XFDASHBOARD_GRADIENT_TYPE_SOLID;
		inTokens++;
	}
		else if(g_strcmp0(*inTokens, "path")==0)
		{
			type=XFDASHBOARD_GRADIENT_TYPE_PATH_GRADIENT;
			inTokens++;
		}

	/* If type of color is none or solid then read in color. The type can be
	 * none as keyword "solid" is optional.
	 */
	if(type==XFDASHBOARD_GRADIENT_TYPE_NONE ||
		type==XFDASHBOARD_GRADIENT_TYPE_SOLID)
	{
		ClutterColor				solidColor={ 0, };

		/* Expect start color */
		NEED_TOKEN(inTokens);
		clutter_color_from_string(&solidColor, *inTokens);

		/* Set up color */
		*outColor=xfdashboard_gradient_color_new_solid(&solidColor);

		/* Return success result */
		return(TRUE);
	}

	/* If type of color is path gradient, then read in start, end colors as well as
	 * offsets and colors to create color stops.
	 */
	if(type==XFDASHBOARD_GRADIENT_TYPE_PATH_GRADIENT)
	{
		ClutterColor				startColor={ 0, };
		ClutterColor				endColor={ 0, };
		gdouble						offset;
		ClutterColor				stopColor={ 0, };

		/* Expect start color */
		NEED_TOKEN(inTokens);
		clutter_color_from_string(&startColor, *inTokens);
		inTokens++;

		NEED_TOKEN(inTokens);
		clutter_color_from_string(&endColor, *inTokens);
		inTokens++;

		/* Set up initial path gradient color */
		*outColor=xfdashboard_gradient_color_new_path_gradient(&startColor, &endColor);

		/* If more tokens exists, read in offset and color for color stop */
		NEXT_TOKEN(inTokens);
		while(*inTokens)
		{
			NEED_TOKEN(inTokens);
			offset=g_ascii_strtod(*inTokens, NULL);
			inTokens++;

			NEED_TOKEN(inTokens);
			clutter_color_from_string(&stopColor, *inTokens);
			inTokens++;

			xfdashboard_gradient_color_add_stop(*outColor, offset, &stopColor);

			/* Move to next token or to end of token list. End of token list
			 * will end this loop and return success result.
			 */
			NEXT_TOKEN(inTokens);
		};

		/* Return success result */
		return(TRUE);
	}

#undef NEED_TOKEN
#undef NEXT_TOKEN

	/* If we get here, something went wrong */
	return(FALSE);
}

/* Transform string value to XfdashboardGradientColor value */
static void _xfdashboard_gradient_color_transform_from_string(const GValue *inSourceValue, GValue *ioDestValue)
{
	const gchar							*text;
	XfdashboardGradientColor			*color;

	g_return_if_fail(G_VALUE_HOLDS_STRING(inSourceValue));
	g_return_if_fail(XFDASHBOARD_VALUE_HOLDS_GRADIENT_COLOR(ioDestValue));

	color=NULL;

	/* If value contains a string, then parse it and create gradient color
	 * from it.
	 */
	text=g_value_get_string(inSourceValue);
	if(text) color=xfdashboard_gradient_color_from_string(text);

	/* Set tranformed value */
	g_value_set_boxed(ioDestValue, color);

	/* Release allocated resources */
	if(color) xfdashboard_gradient_color_free(color);
}

/* Transform XfdashboardGradientColor value to string value */
static void _xfdashboard_gradient_color_transform_to_string(const GValue *inSourceValue, GValue *ioDestValue)
{
	XfdashboardGradientColor		*color;
	gchar							*text;

	g_return_if_fail(XFDASHBOARD_VALUE_HOLDS_GRADIENT_COLOR(inSourceValue));
	g_return_if_fail(G_VALUE_HOLDS_STRING(ioDestValue));

	/* Get color from boxed type */
	color=g_value_get_boxed(inSourceValue);

	/* Get text representation for color */
	text=xfdashboard_gradient_color_to_string(color);

	/* Set text representation at destination */
	g_value_set_string(ioDestValue, text);

	/* Release allocated resources */
	g_free(text);
}


/* IMPLEMENTATION: Public API */

/**
 * xfdashboard_gradient_color_new_solid:
 * @inColor: A #ClutterColor
 *
 * Creates a new single-colored solid color with the provided color at @inColor.
 *
 * Returns: A newly created #XfdashboardGradientColor
 */
XfdashboardGradientColor* xfdashboard_gradient_color_new_solid(const ClutterColor *inColor)
{
	XfdashboardGradientColor		*self;

	g_return_val_if_fail(inColor, NULL);

	/* Create boxed type and set up for use of single-colored solids */ 
	self=g_new0(XfdashboardGradientColor, 1);

	self->type=XFDASHBOARD_GRADIENT_TYPE_SOLID;
	self->solid=clutter_color_copy(inColor);

	/* Return newly created boxed type */
	return(self);
}

/**
 * xfdashboard_gradient_color_new_linear_gradient:
 * @inStartColor: A #ClutterColor to begin linear gradient with
 * @inEndColor: A #ClutterColor to end linear gradient with
 *
 * Creates a new linear gradient with the provided color @inStartColor at start
 * offset (0.0) and color @inEndColor at end offset (1.0). The new gradient
 * is non-repeative and covers the whole area.
 *
 * The path gradient can be more specific by adding color stops with
 * xfdashboard_gradient_color_add_stop().
 *
 * Returns: A newly created #XfdashboardGradientColor
 */
XfdashboardGradientColor* xfdashboard_gradient_color_new_linear_gradient(const ClutterColor *inStartColor, const ClutterColor *inEndColor)
{
	XfdashboardGradientColor		*self;

	g_return_val_if_fail(inStartColor, NULL);
	g_return_val_if_fail(inEndColor, NULL);

	/* Create boxed type and set up for use of linear gradients */
	self=g_new0(XfdashboardGradientColor, 1);

	self->type=XFDASHBOARD_GRADIENT_TYPE_LINEAR_GRADIENT;
	self->stops=g_array_new(FALSE, FALSE, sizeof(XfdashboardGradientColorStop));

	xfdashboard_gradient_color_add_stop(self, 0.0, inStartColor);
	xfdashboard_gradient_color_add_stop(self, 1.0, inEndColor);

	/* Return newly created boxed type */
	return(self);
}

/**
 * xfdashboard_gradient_color_new_path_gradient:
 * @inStartColor: A #ClutterColor to begin path gradient with
 * @inEndColor: A #ClutterColor to end path gradient with
 *
 * Creates a new path gradient with the provided color @inStartColor at start
 * offset (0.0) and color @inEndColor at end offset (1.0).
 *
 * The path gradient can be more specific by adding color stops with
 * xfdashboard_gradient_color_add_stop().
 * 
 * Returns: A newly created #XfdashboardGradientColor
 */
XfdashboardGradientColor* xfdashboard_gradient_color_new_path_gradient(const ClutterColor *inStartColor, const ClutterColor *inEndColor)
{
	XfdashboardGradientColor		*self;

	g_return_val_if_fail(inStartColor, NULL);
	g_return_val_if_fail(inEndColor, NULL);

	/* Create boxed type and set up for use of path gradients */
	self=g_new0(XfdashboardGradientColor, 1);

	self->type=XFDASHBOARD_GRADIENT_TYPE_PATH_GRADIENT;
	self->stops=g_array_new(FALSE, FALSE, sizeof(XfdashboardGradientColorStop));

	xfdashboard_gradient_color_add_stop(self, 0.0, inStartColor);
	xfdashboard_gradient_color_add_stop(self, 1.0, inEndColor);

	/* Return newly created boxed type */
	return(self);
}

/**
 * xfdashboard_gradient_color_copy:
 * @self: A #XfdashboardGradientColor
 *
 * Copies the color at @self to a new color.
 *
 * Returns: A newly created #XfdashboardGradientColor
 */
XfdashboardGradientColor* xfdashboard_gradient_color_copy(const XfdashboardGradientColor *self)
{
	XfdashboardGradientColor		*newColor;

	g_return_val_if_fail(self, NULL);

	/* Create boxed type and copy source color datas.
	 * We can create a shallow copy of the color stops, if any,
	 * because it does not contain any dynamically allocated
	 * resources or pointers.
	 */ 
	newColor=g_new0(XfdashboardGradientColor, 1);

	newColor->type=self->type;
	if(self->solid) newColor->solid=clutter_color_copy(self->solid);
	if(self->stops) newColor->stops=g_array_copy(self->stops);
	newColor->gradientAngle=self->gradientAngle;
	newColor->gradientRepeat=self->gradientRepeat;
	newColor->gradientLength=self->gradientLength;

	/* Return newly created boxed type */
	return(newColor);
}

/**
 * xfdashboard_gradient_color_free:
 * @self: A #XfdashboardGradientColor
 *
 * Frees the memory allocated by @self.
 */
void xfdashboard_gradient_color_free(XfdashboardGradientColor *self)
{
	g_return_if_fail(self);

	/* IRelease it and its allocated resources */
	if(self->solid) clutter_color_free(self->solid);
	if(self->stops) g_array_free(self->stops, TRUE);
	g_free(self);
}

/**
 * xfdashboard_gradient_color_compare:
 * @inLeft: A #XfdashboardGradientColor
 * @inRight: A #XfdashboardGradientColor
 *
 * Compares the color definitions at @inLeft and @inRight and returns 0
 * if both are equal. If both do not match, a negative value will be returned
 * if @inLeft is the "smaller" one and a positibe value if @inRight is
 * the "smaller" one.
 *
 * Returns: 0 if both colors are equal, a negative value if @inLeft is
 *   "smaller" and a positive value if @inRight is smaller.
 */
gint xfdashboard_gradient_color_compare(const XfdashboardGradientColor *inLeft, const XfdashboardGradientColor *inRight)
{
	gint								result;
	gdouble								resultDouble;
	guint								i;
	guint32								leftPixel;
	guint32								rightPixel;
	XfdashboardGradientColorStop		*leftStop;
	XfdashboardGradientColorStop		*rightStop;

	/* Check if both colors are provided */
	if(!inLeft)
	{
		return(!inRight ? 0 : -1);
	}

	if(!inRight) return(1);

	/* Check if both colors are of the same type */
	result=inRight->type - inLeft->type;
	if(result!=0) return(result);

	/* Check if both colors share the same color definition depending on their type */
	switch(inLeft->type)
	{
		case XFDASHBOARD_GRADIENT_TYPE_SOLID:
			/* Check if both colors do have the same solid color */
			leftPixel=clutter_color_to_pixel(inLeft->solid);
			rightPixel=clutter_color_to_pixel(inRight->solid);
			if(leftPixel<rightPixel) return(-1);
				else if(leftPixel>rightPixel) return(1);
			break;

		case XFDASHBOARD_GRADIENT_TYPE_LINEAR_GRADIENT:
			/* Check if both colors have the same number of color stops */
			result=inRight->stops->len - inLeft->stops->len;
			if(result!=0) return(result);

			/* Check if both colors have the same color stops */
			for(i=0; i<inLeft->stops->len; i++)
			{
				/* Get color stop of both colors */
				leftStop=&g_array_index(inLeft->stops, XfdashboardGradientColorStop, i);
				rightStop=&g_array_index(inRight->stops, XfdashboardGradientColorStop, i);

				/* Check if color stop of both colors have same offset */
				if(leftStop->offset < rightStop->offset) return(-1);
					else if(leftStop->offset > rightStop->offset) return(1);

				/* Check if color stop of both colors have same color */
				leftPixel=clutter_color_to_pixel(&leftStop->color);
				rightPixel=clutter_color_to_pixel(&rightStop->color);
				if(leftPixel<rightPixel) return(-1);
					else if(leftPixel>rightPixel) return(1);
			}

			/* Check if both colors have same angle */
			resultDouble=inRight->gradientAngle - inLeft->gradientAngle;
			if(resultDouble!=0) return((gint)(round(resultDouble)));

			/* Check if both colors have same repeat flag */
			if(inLeft->gradientRepeat!=inRight->gradientRepeat) return(-1);

			/* Check if both colors have same repeat length if repeat flag is set */
			if(inLeft->gradientRepeat)
			{
				resultDouble=inRight->gradientLength - inLeft->gradientLength;
				if(resultDouble!=0) return((gint)(round(resultDouble)));
			}
			break;

		case XFDASHBOARD_GRADIENT_TYPE_PATH_GRADIENT:
			/* Check if both colors have the same number of color stops */
			result=inRight->stops->len - inLeft->stops->len;
			if(result!=0) return(result);

			/* Check if both colors have the same color stops */
			for(i=0; i<inLeft->stops->len; i++)
			{
				/* Get color stop of both colors */
				leftStop=&g_array_index(inLeft->stops, XfdashboardGradientColorStop, i);
				rightStop=&g_array_index(inRight->stops, XfdashboardGradientColorStop, i);

				/* Check if color stop of both colors have same offset */
				if(leftStop->offset < rightStop->offset) return(-1);
					else if(leftStop->offset > rightStop->offset) return(1);

				/* Check if color stop of both colors have same color */
				leftPixel=clutter_color_to_pixel(&leftStop->color);
				rightPixel=clutter_color_to_pixel(&rightStop->color);
				if(leftPixel<rightPixel) return(-1);
					else if(leftPixel>rightPixel) return(1);
			}
			break;

		default:
			/* Nothing further to check */
			break;
	}

	/* If we get here, both colors are equal, so return 0 */
	return(0);
}

/**
 * xfdashboard_gradient_color_equal:
 * @inLeft: A #XfdashboardGradientColor
 * @inRight: A #XfdashboardGradientColor
 *
 * Compares the color definitions at @inLeft and @inRight and returns %TRUE
 * if both are equal. If both do not match, %FALSE will be returned.
 *
 * Returns: %TRUE if both colors are equal, otherwise %FALSE.
 */
gboolean xfdashboard_gradient_color_equal(const XfdashboardGradientColor *inLeft, const XfdashboardGradientColor *inRight)
{
	return(xfdashboard_gradient_color_compare(inLeft, inRight)==0 ? TRUE : FALSE);
}

/**
 * xfdashboard_gradient_color_to_string:
 * @inString: A string representation of gradient color
 *
 * Creates a gradient color by parsing the string representation of a gradient
 * color as specified at @inString.
 *
 * Returns: A newly created #XfdashboardGradientColor
 **/
XfdashboardGradientColor* xfdashboard_gradient_color_from_string(const gchar *inString)
{
	XfdashboardGradientColor		*color;
	gchar							**tokens;

	g_return_val_if_fail(inString && *inString, NULL);

	color=NULL;

	/* Split text into token for easier parsing */
	tokens=g_strsplit_set(inString, " \r\t\n", -1);
	if(!tokens) return(NULL);

	/* Parse tokens */
	if(!_xfdashboard_gradient_color_transform_from_string_parse(tokens, &color))
	{
		if(color)
		{
			xfdashboard_gradient_color_free(color);
			color=NULL;
		}

		g_warning("Failed to parse gradient color '%s'", inString);
	}

	/* Release allocated resources */
	if(tokens) g_strfreev(tokens);

	/* Return gradient color on success or NULL if failed */
	return(color);
}

/**
 * xfdashboard_gradient_color_to_string:
 * @self: A #XfdashboardGradientColor
 *
 * Creates a string representation of gradient color at @self.
 *
 * Returns: A string for gradient color or %NULL in case of errors.
 *   The caller is responsible to free returned string with g_free().
 **/
gchar* xfdashboard_gradient_color_to_string(const XfdashboardGradientColor *self)
{
	GString										*text;

	g_return_val_if_fail(self, NULL);

	/* Build text representation of color */
	text=g_string_new(NULL);

	switch(self->type)
	{
		/* Build solid color representation, i.e. just the color */
		case XFDASHBOARD_GRADIENT_TYPE_SOLID:
			{
				gchar							*temp;

				temp=clutter_color_to_string(self->solid);
				g_string_append(text, temp);
				g_free(temp);
			}
			break;

		/* Build path gradient representation */
		case XFDASHBOARD_GRADIENT_TYPE_PATH_GRADIENT:
			{
				guint							i;
				guint							lastIndex;
				XfdashboardGradientColorStop	*stop;
				gchar							*temp;

				/* Add type */
				g_string_append(text, "path ");

				/* Add start color */
				stop=&g_array_index(self->stops, XfdashboardGradientColorStop, 0);
				temp=clutter_color_to_string(&stop->color);
				g_string_append(text, temp);
				g_string_append_c(text, ' ');
				g_free(temp);

				/* Add end color */
				lastIndex=self->stops->len - 1;

				stop=&g_array_index(self->stops, XfdashboardGradientColorStop, lastIndex);
				temp=clutter_color_to_string(&stop->color);
				g_string_append(text, temp);
				g_string_append_c(text, ' ');
				g_free(temp);

				/* Add color stop with offset and color */
				for(i=1; i<=(lastIndex-1); i++)
				{
					stop=&g_array_index(self->stops, XfdashboardGradientColorStop, i);

					temp=g_strdup_printf("%f ", stop->offset);
					g_string_append(text, temp);
					g_free(temp);

					temp=clutter_color_to_string(&stop->color);
					g_string_append(text, temp);
					g_string_append_c(text, ' ');
					g_free(temp);
				}
			}
			break;

		/* Do nothing at all other types */
		default:
			break;
	}

	/* Return text representation */
	return(g_string_free(text, FALSE));
}

/**
 * xfdashboard_gradient_color_get_gradient_type:
 * @self: A #XfdashboardGradientColor
 *
 * Determine the gradient type of @self which is one of #XfdashboardGradientType.
 *
 * Returns: The gradient type of @self or %XFDASHBOARD_GRADIENT_TYPE_NONE if @self
 *   is %NULL
 */
XfdashboardGradientType xfdashboard_gradient_color_get_gradient_type(const XfdashboardGradientColor *self)
{
	g_return_val_if_fail(self, XFDASHBOARD_GRADIENT_TYPE_NONE);

	return(self->type);
}

/**
 * xfdashboard_gradient_color_get_solid_color:
 * @self: A #XfdashboardGradientColor
 *
 * Determine the color of @self if it is a solid color type.
 *
 * Returns: The color of @self or %NULL if @self is %NULL or it is not
 *   of gradient type %XFDASHBOARD_GRADIENT_TYPE_SOLID
 */
const ClutterColor* xfdashboard_gradient_color_get_solid_color(const XfdashboardGradientColor *self)
{
	g_return_val_if_fail(self, NULL);
	g_return_val_if_fail(self->type==XFDASHBOARD_GRADIENT_TYPE_SOLID, NULL);

	return(self->solid);
}

/**
 * xfdashboard_gradient_color_set_solid_color:
 * @self: A #XfdashboardGradientColor
 * @inColor: The #ClutterColor to set as solid color
 *
 * Sets the color of @self to @inColor if it is a solid color type.
 */
void xfdashboard_gradient_color_set_solid_color(XfdashboardGradientColor *self, const ClutterColor *inColor)
{
	g_return_if_fail(self);
	g_return_if_fail(self->type==XFDASHBOARD_GRADIENT_TYPE_SOLID);
	g_return_if_fail(inColor);

	/* Set new solid color */
	if(self->solid) clutter_color_free(self->solid);
	self->solid=clutter_color_copy(inColor);
}

/**
 * xfdashboard_gradient_color_get_angle:
 * @self: A #XfdashboardGradientColor
 *
 * Determine the angle of @self if it is a linear gradient type.
 *
 * Returns: The angle of gradient between 0.0 and 2*PI. An angle of 0.0 is
 *   also returned if the gradient type is not linear.
 */
gdouble xfdashboard_gradient_color_get_angle(const XfdashboardGradientColor *self)
{
	g_return_val_if_fail(self, 0.0);
	g_return_val_if_fail(self->type==XFDASHBOARD_GRADIENT_TYPE_LINEAR_GRADIENT, 0.0);

	return(self->gradientAngle);
}

/**
 * xfdashboard_gradient_color_set_angle:
 * @self: A #XfdashboardGradientColor
 * @inAngle: The angle of gradient in radians
 *
 * Sets the angle of @self to @inAngle if it is a linear gradient type. The
 * angle must be provided as radians between 0.0 and 2*PI. A angle of 0.0 is
 * directed to right and increasing angle rotate clockwise, e.g. PI/2 is directed
 * to bottom.
 */
void xfdashboard_gradient_color_set_angle(XfdashboardGradientColor *self, gdouble inAngle)
{
	g_return_if_fail(self);
	g_return_if_fail(self->type==XFDASHBOARD_GRADIENT_TYPE_LINEAR_GRADIENT);
	g_return_if_fail(inAngle>=0.0 && inAngle<=(2*M_PI));

	/* Set angle */
	self->gradientAngle=inAngle;
}

/**
 * xfdashboard_gradient_color_get_repeat:
 * @self: A #XfdashboardGradientColor
 *
 * Determine if gradient pattern of @self is repeated if it is a linear gradient
 * type. The length of one gradient pattern, before it is repeated, can be
 * determine with xfdashboard_gradient_color_get_length().
 *
 * Returns: %TRUE if gradient pattern is repeated and if gradient type is a linear
 *   gradient. Otherwise it returns %FALSE.
 */
gboolean xfdashboard_gradient_color_get_repeat(const XfdashboardGradientColor *self)
{
	g_return_val_if_fail(self, FALSE);
	g_return_val_if_fail(self->type==XFDASHBOARD_GRADIENT_TYPE_LINEAR_GRADIENT, FALSE);

	return(self->gradientRepeat);
}

/**
 * xfdashboard_gradient_color_get_length:
 * @self: A #XfdashboardGradientColor
 *
 * Determine the length of gradient pattern at @self if it is a linear gradient
 * and repeated.
 *
 * If a negative value is returned the length is in percentage and the value is a
 * fraction between 0.0 and -1.0. A positive value is the length in pixel. A
 * length of 0.0 means the whole area this gradient pattern is applied to, i.e.
 * the pattern is not repeated.
 *
 * Returns: the length of gradient pattern
 */
gdouble xfdashboard_gradient_color_get_length(const XfdashboardGradientColor *self)
{
	g_return_val_if_fail(self, 0.0);
	g_return_val_if_fail(self->type==XFDASHBOARD_GRADIENT_TYPE_LINEAR_GRADIENT, 0.0);

	return(self->gradientRepeat ? self->gradientLength : 0.0);
}

/**
 * xfdashboard_gradient_color_set_repeat:
 * @self: A #XfdashboardGradientColor
 * @inRepeat: %TRUE if gradient pattern should be repeated
 * @inLength: The length of gradient pattern as percentage or pixels
 *
 * Sets if gradient pattern of @self is repeated or not repeated as specified
 * at @inRepeat if it is a linear gradient type. The length @inLength can
 * either be specified as fraction (percentage) between 0.0 and -1.0 as a
 * negative value must be used. A positive value defines the length in piexels.
 *
 * If pattern should not be repeated the lenght is set to 0.0 indicating that
 * the gradient pattern should be applied to the whole area.
 */
void xfdashboard_gradient_color_set_repeat(XfdashboardGradientColor *self, gboolean inRepeat, gdouble inLength)
{
	g_return_if_fail(self);
	g_return_if_fail(self->type==XFDASHBOARD_GRADIENT_TYPE_LINEAR_GRADIENT);
	g_return_if_fail(inLength>=-1.0);
	g_return_if_fail(inRepeat==FALSE || inLength!=0.0);

	self->gradientRepeat=inRepeat;
	self->gradientLength=(inRepeat ? inLength : 0.0);
}

/**
 * xfdashboard_gradient_color_get_number_stops:
 * @self: A #XfdashboardGradientColor
 *
 * Determines the number of color stops at @self if it is a gradient.
 *
 * Returns: The number of color stops at @self
 */
guint xfdashboard_gradient_color_get_number_stops(const XfdashboardGradientColor *self)
{
	g_return_val_if_fail(self, 0);
	g_return_val_if_fail(self->type!=XFDASHBOARD_GRADIENT_TYPE_NONE, 0);
	g_return_val_if_fail(self->type!=XFDASHBOARD_GRADIENT_TYPE_SOLID, 0);

	return(self->stops->len);
}

/**
 * xfdashboard_gradient_color_get_stop:
 * @self: A #XfdashboardGradientColor
 * @inIndex: The index of stop color to retrieve
 * @outOffset: A return location for the offset or %NULL
 * @outColor: A return location for a #ClutterColor or %NULL
 *
 * Retrieved information of color stop at index @inIndex of @self.
 * If @outOffset is not %NULL then it will contain the offset of
 * the requested color stop. If @outColor is not %NULL then it will
 * contain the color of the requested color stop. In case of errors
 * the return locations will be untouched.
 */
void xfdashboard_gradient_color_get_stop(const XfdashboardGradientColor *self, guint inIndex, gdouble *outOffset, ClutterColor *outColor)
{
	XfdashboardGradientColorStop		*stop;

	g_return_if_fail(self);
	g_return_if_fail(self->type!=XFDASHBOARD_GRADIENT_TYPE_NONE);
	g_return_if_fail(self->type!=XFDASHBOARD_GRADIENT_TYPE_SOLID);
	g_return_if_fail(inIndex<self->stops->len);

	/* Get stop color at requested index */ 
	stop=&g_array_index(self->stops, XfdashboardGradientColorStop, inIndex);

	/* Set result values */
	if(outOffset) *outOffset=stop->offset;
	if(outColor)
	{
		clutter_color_init(outColor,
							stop->color.red,
							stop->color.green,
							stop->color.blue,
							stop->color.alpha);
	}
}

/**
 * xfdashboard_gradient_color_add_stop:
 * @self: A #XfdashboardGradientColor
 * @inOffset: The offset of new color stop
 * @inColor: A #ClutterColor for new color stop
 *
 * Adds a new color stop to @self with offset provided at @inOffset and the
 * color provided at @inColor. The color stop will be inserted in the array
 * of color stops just between the one having a lower offset and the one
 * having a higher offfset. This is done for lookup performance reason.
 * Therefore do not assume that a previoudly retrieved index will still
 * point to the same color stop after a new color stop was added.
 *
 * NOTE: If multiple color stops at the same offset will be added, the
 *       behaviour is undefined.
 *
 * Returns: %TRUE if new color stop could be added. %FALSE in case of errors. 
 */
gboolean xfdashboard_gradient_color_add_stop(XfdashboardGradientColor *self, gdouble inOffset, const ClutterColor *inColor)
{
	XfdashboardGradientColorStop		stop;
	guint								index;
	XfdashboardGradientColorStop		*iter;

	g_return_val_if_fail(self, FALSE);
	g_return_val_if_fail(self->type!=XFDASHBOARD_GRADIENT_TYPE_NONE, FALSE);
	g_return_val_if_fail(self->type!=XFDASHBOARD_GRADIENT_TYPE_SOLID, FALSE);
	g_return_val_if_fail(inOffset>=0.0 && inOffset<=1.0, FALSE);
	g_return_val_if_fail(inColor, FALSE);

	/* Find index where to add new color stop at */
	for(index=0; index<self->stops->len; index++)
	{
		/* Get currently iterated color stop */
		iter=&g_array_index(self->stops, XfdashboardGradientColorStop, index);
		if(!iter) return(FALSE);

		/* If this color stop has higher offset than the new one,
		 * stop futher iteration.
		 */
		if(iter->offset>inOffset) break;
	}

	/* Set up new color stop */
	stop.offset=inOffset;
	clutter_color_init(&stop.color,
						inColor->red,
						inColor->green,
						inColor->blue,
						inColor->alpha);
	g_array_insert_val(self->stops, index, stop);

	/* Return TRUE for success when adding new color stop */
	return(TRUE);
}

/**
 * xfdashboard_gradient_color_interpolate:
 * @self: A #XfdashboardGradientColor
 * @inProgress: the interpolation progress
 * @outColor: A return location for the interpolated color of type #ClutterColor
 *
 * Interpolates the color of a gradient between the color stops of @self using
 * the progress as provided at @inProgress. The interpolated color is store
 * at the return location provided by @outColor.
 */
void xfdashboard_gradient_color_interpolate(const XfdashboardGradientColor *self, gdouble inProgress, ClutterColor *outColor)
{
	guint							index;
	XfdashboardGradientColorStop		*iter;
	XfdashboardGradientColorStop		*iterPrevious;

	g_return_if_fail(self);
	g_return_if_fail(self->type!=XFDASHBOARD_GRADIENT_TYPE_NONE);
	g_return_if_fail(self->type!=XFDASHBOARD_GRADIENT_TYPE_SOLID);
	g_return_if_fail(self->stops->len>=2);
	g_return_if_fail(inProgress>=0.0 && inProgress<=1.0);
	g_return_if_fail(outColor);

	/* Find index of last color stop whose offset is below the requested progress */
	iterPrevious=&g_array_index(self->stops, XfdashboardGradientColorStop, 0);
	for(index=0; index<self->stops->len; index++)
	{
		/* Get currently iterated color stop */
		iter=&g_array_index(self->stops, XfdashboardGradientColorStop, index);
		if(!iter) return;

		/* If this color stop has exactly the same offset than the requested one,
		 * return its color directly.
		 */
		if(iter->offset==inProgress)
		{
			clutter_color_init(outColor,
								iter->color.red,
								iter->color.green,
								iter->color.blue,
								iter->color.alpha);
			return;
		};

		/* If this color stop has a higher offset than the requested one, than
		 * interpolate between the previous color stop and this one and store
		 * interpolated color.
		 */
		if(iter->offset>inProgress)
		{
			gdouble					realProgress;

			/* Calculate progress between both color stops */
			realProgress=(inProgress- iterPrevious->offset) / (iter->offset - iterPrevious->offset);

			/* Interpolate color by progress between both color stops */
			clutter_color_interpolate(&iterPrevious->color, &iter->color, realProgress, outColor);

			return;
		}

		/* Continue iteration */
		iterPrevious=iter;
	}

	/* We should never get here but check ... */
	g_assert_not_reached();
}


/* IMPLEMENTATION: Private variables and methods (XfdashboardParamSpecGradientColor) */

struct _XfdashboardParamSpecGradientColor
{
	/*< private >*/
	GParamSpec					parent_instance;

	/*< public >*/
	XfdashboardGradientColor		*defaultValue;
};

/* Initialize param spec for XfdashboardGradientColor */
static void _xfdashboard_param_spec_gradient_color_init(GParamSpec *inParamSpec)
{
	XfdashboardParamSpecGradientColor	*spec=XFDASHBOARD_PARAM_SPEC_GRADIENT_COLOR(inParamSpec);

	spec->defaultValue=NULL;
}

/* Finalize param spec for XfdashboardGradientColor */
static void _xfdashboard_param_spec_gradient_color_finalize(GParamSpec *inParamSpec)
{
	XfdashboardParamSpecGradientColor	*spec=XFDASHBOARD_PARAM_SPEC_GRADIENT_COLOR(inParamSpec);

	xfdashboard_gradient_color_free(spec->defaultValue);
}

/* Set default value of param spec for XfdashboardGradientColor at GValue */
static void _xfdashboard_param_spec_gradient_color_set_default(GParamSpec *inParamSpec,
																GValue *outValue)
{
	XfdashboardParamSpecGradientColor	*spec=XFDASHBOARD_PARAM_SPEC_GRADIENT_COLOR(inParamSpec);

	xfdashboard_value_set_gradient_color(outValue, spec->defaultValue);
}

/* Compare value of two param specs for XfdashboardGradientColor */
static gint _xfdashboard_param_spec_gradient_color_compare(GParamSpec *inParamSpec,
															const GValue *inLeft,
															const GValue *inRight)
{
	const XfdashboardGradientColor		*colorLeft=g_value_get_boxed(inLeft);
	const XfdashboardGradientColor		*colorRight=g_value_get_boxed(inRight);

	return(xfdashboard_gradient_color_compare(colorLeft, colorRight));
}


/* IMPLEMENTATION: Public API (XfdashboardParamSpecGradientColor) */

/**
 * xfdashboard_param_spec_gradient_color_get_type: (skip)
 *
 * Registers #XfdashboardParamSpecGradientColor to param spec type system.
 */
GType xfdashboard_param_spec_gradient_color_get_type(void)
{
	static GType					type=0;

	if(G_UNLIKELY(type==0))
	{
		const GParamSpecTypeInfo	typeInfo=
			{
				sizeof(XfdashboardParamSpecGradientColor),
				16,
				_xfdashboard_param_spec_gradient_color_init,
				XFDASHBOARD_TYPE_GRADIENT_COLOR,
				_xfdashboard_param_spec_gradient_color_finalize,
				_xfdashboard_param_spec_gradient_color_set_default,
				NULL,
				_xfdashboard_param_spec_gradient_color_compare,
			};

		type=g_param_type_register_static(g_intern_static_string("XfdashboardParamSpecGradientColor"), &typeInfo);
	}

	return(type);
}

/**
 * xfdashboard_param_spec_gradient_color: (skip)
 * @inName: Name of property
 * @inNick: Short name
 * @inBlurb: Description (can be translatable)
 * @inDefaultValue: default value
 * @inFlags: Flags for the param spec
 *
 * Creates a #GParamSpec for properties using #XfdashboardGradientColor.
 *
 * Return value: the newly created #GParamSpec
 */
GParamSpec* xfdashboard_param_spec_gradient_color(const gchar *inName,
													const gchar *inNick,
													const gchar *inBlurb,
													const XfdashboardGradientColor *inDefaultValue,
													GParamFlags inFlags)
{
	XfdashboardParamSpecGradientColor		*spec;

	spec=g_param_spec_internal(XFDASHBOARD_TYPE_PARAM_SPEC_GRADIENT_COLOR,
								inName,
								inNick,
								inBlurb,
								inFlags);
	spec->defaultValue=xfdashboard_gradient_color_copy(inDefaultValue);

	return(G_PARAM_SPEC(spec));
}


/* IMPLEMENTATION: Public API (GValue) */

/**
 * xfdashboard_value_set_gradient_color:
 * @ioValue: a #GValue initialized to #XFDASHBOARD_TYPE_GRADIENT_COLOR
 * @inColor: the color to set
 *
 * Sets @ioValue to @inColor.
 */
void xfdashboard_value_set_gradient_color(GValue *ioValue, const XfdashboardGradientColor *inColor)
{
	g_return_if_fail(XFDASHBOARD_VALUE_HOLDS_GRADIENT_COLOR(ioValue));

	g_value_set_boxed(ioValue, inColor);
}

/**
 * xfdashboard_value_get_gradient_color:
 * @inValue: a #GValue initialized to #XFDASHBOARD_TYPE_GRADIENT_COLOR
 *
 * Gets the #XfdashboardGradientColor contained in @inValue.
 *
 * Return value: (transfer none): The gradient color inside the passed #GValue
 */
const XfdashboardGradientColor* xfdashboard_value_get_gradient_color(const GValue *inValue)
{
	g_return_val_if_fail(XFDASHBOARD_VALUE_HOLDS_GRADIENT_COLOR(inValue), NULL);

	return(g_value_get_boxed(inValue));
}
