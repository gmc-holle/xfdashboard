/*
 * gradient-color: A special purpose boxed color type for solid colors
 *                 or gradients
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

#ifndef __LIBXFDASHBOARD_GRADIENT_COLOR__
#define __LIBXFDASHBOARD_GRADIENT_COLOR__

#if !defined(__LIBXFDASHBOARD_H_INSIDE__) && !defined(LIBXFDASHBOARD_COMPILATION)
#error "Only <libxfdashboard/libxfdashboard.h> can be included directly."
#endif

#include <clutter/clutter.h>


G_BEGIN_DECLS

/* Public definitions */
/**
 * XfdashboardGradientType:
 * @XFDASHBOARD_GRADIENT_TYPE_SOLID: The color is a single-color solid color
 * @XFDASHBOARD_GRADIENT_TYPE_PATH_GRADIENT: The color is a linear gradient
 *
 * Determines the type of color.
 */
typedef enum /*< prefix=XFDASHBOARD_GRADIENT_TYPE >*/
{
	XFDASHBOARD_GRADIENT_TYPE_NONE=0,
	XFDASHBOARD_GRADIENT_TYPE_SOLID,
	XFDASHBOARD_GRADIENT_TYPE_LINEAR_GRADIENT,
	XFDASHBOARD_GRADIENT_TYPE_PATH_GRADIENT
} XfdashboardGradientType;

/**
 * XFDASHBOARD_VALUE_HOLDS_GRADIENT_COLOR:
 * @x: a #GValue
 *
 * Evaluates to %TRUE if @x holds a #XfdashboardGradientColor<!-- -->.
 */
#define XFDASHBOARD_VALUE_HOLDS_GRADIENT_COLOR(x)			(G_VALUE_HOLDS((x), XFDASHBOARD_TYPE_GRADIENT_COLOR))


/* Object declaration (XfdashboardGradientColor) */
#define XFDASHBOARD_TYPE_GRADIENT_COLOR						(xfdashboard_gradient_color_get_type())

typedef struct _XfdashboardGradientColor					XfdashboardGradientColor;


/* Public API (XfdashboardGradientColor) */
GType xfdashboard_gradient_color_get_type(void) G_GNUC_CONST;

/* Creation functions */
XfdashboardGradientColor* xfdashboard_gradient_color_new_solid(const ClutterColor *inColor);
XfdashboardGradientColor* xfdashboard_gradient_color_new_linear_gradient(const ClutterColor *inStartColor, const ClutterColor *inEndColor);
XfdashboardGradientColor* xfdashboard_gradient_color_new_path_gradient(const ClutterColor *inStartColor, const ClutterColor *inEndColor);

XfdashboardGradientColor* xfdashboard_gradient_color_copy(const XfdashboardGradientColor *self);
void xfdashboard_gradient_color_free(XfdashboardGradientColor *self);

/* Utility functions */
gint xfdashboard_gradient_color_compare(const XfdashboardGradientColor *inLeft, const XfdashboardGradientColor *inRight);
gboolean xfdashboard_gradient_color_equal(const XfdashboardGradientColor *inLeft, const XfdashboardGradientColor *inRight);

XfdashboardGradientColor* xfdashboard_gradient_color_from_string(const gchar *inString);
gchar* xfdashboard_gradient_color_to_string(const XfdashboardGradientColor *self);

/* Type functions */
XfdashboardGradientType xfdashboard_gradient_color_get_gradient_type(const XfdashboardGradientColor *self);

/* Solid color functions */
const ClutterColor* xfdashboard_gradient_color_get_solid_color(const XfdashboardGradientColor *self);
void xfdashboard_gradient_color_set_solid_color(XfdashboardGradientColor *self, const ClutterColor *inColor);

/* Linear gradient functions */
gdouble xfdashboard_gradient_color_get_angle(const XfdashboardGradientColor *self);
void xfdashboard_gradient_color_set_angle(XfdashboardGradientColor *self, gdouble inRadians);

gboolean xfdashboard_gradient_color_get_repeat(const XfdashboardGradientColor *self);
gdouble xfdashboard_gradient_color_get_length(const XfdashboardGradientColor *self);
void xfdashboard_gradient_color_set_repeat(XfdashboardGradientColor *self, gboolean inRepeat, gdouble inLength);

/* Common gradient functions */
guint xfdashboard_gradient_color_get_number_stops(const XfdashboardGradientColor *self);

void xfdashboard_gradient_color_get_stop(const XfdashboardGradientColor *self,
											guint inIndex,
											gdouble *outOffset,
											ClutterColor *outColor);
gboolean xfdashboard_gradient_color_add_stop(XfdashboardGradientColor *self,
												gdouble inOffset,
												const ClutterColor *inColor);

void xfdashboard_gradient_color_interpolate(const XfdashboardGradientColor *self,
											gdouble inProgress,
											ClutterColor *outColor);


/* Object declaration (XfdashboardParamSpecGradientColor) */

#define XFDASHBOARD_TYPE_PARAM_SPEC_GRADIENT_COLOR			(xfdashboard_param_spec_gradient_color_get_type())
#define XFDASHBOARD_PARAM_SPEC_GRADIENT_COLOR(pspec)		(G_TYPE_CHECK_INSTANCE_CAST((pspec), XFDASHBOARD_TYPE_PARAM_SPEC_GRADIENT_COLOR, XfdashboardParamSpecGradientColor))
#define XFDASHBOARD_IS_PARAM_SPEC_GRADIENT_COLOR(pspec)		(G_TYPE_CHECK_INSTANCE_TYPE((pspec), XFDASHBOARD_TYPE_PARAM_SPEC_GRADIENT_COLOR))

typedef struct _XfdashboardParamSpecGradientColor			XfdashboardParamSpecGradientColor;


/* Public API (XfdashboardParamSpecGradientColor) */
GType xfdashboard_param_spec_gradient_color_get_type(void);

GParamSpec* xfdashboard_param_spec_gradient_color(const gchar *inName,
													const gchar *inNick,
													const gchar *inBlurb,
													const XfdashboardGradientColor *inDefaultValue,
													GParamFlags inFlags);


/* IMPLEMENTATION: Public API (GValue) */

void xfdashboard_value_set_gradient_color(GValue *ioValue, const XfdashboardGradientColor *inColor);
const XfdashboardGradientColor* xfdashboard_value_get_gradient_color(const GValue *inValue);

G_END_DECLS

#endif	/* __LIBXFDASHBOARD_GRADIENT_COLOR__ */
