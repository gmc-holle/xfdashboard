/*
 * background: An actor providing background rendering. Usually other
 *             actors are derived from this one.
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

#ifndef __LIBXFDASHBOARD_BACKGROUND__
#define __LIBXFDASHBOARD_BACKGROUND__

#if !defined(__LIBXFDASHBOARD_H_INSIDE__) && !defined(LIBXFDASHBOARD_COMPILATION)
#error "Only <libxfdashboard/libxfdashboard.h> can be included directly."
#endif

#include <clutter/clutter.h>

#include <libxfdashboard/actor.h>
#include <libxfdashboard/types.h>

G_BEGIN_DECLS

/* Public definitions */
/**
 * XfdashboardBackgroundType:
 * @XFDASHBOARD_BACKGROUND_TYPE_NONE: The actor will be displayed unmodified.
 * @XFDASHBOARD_BACKGROUND_TYPE_FILL: The actor background will be filled with a color.
 * @XFDASHBOARD_BACKGROUND_TYPE_OUTLINE: The actor will get an outline.
 * @XFDASHBOARD_BACKGROUND_TYPE_ROUNDED_CORNERS: The edges of actor will be rounded.
 *
 * Determines how the background of an actor will be displayed and if it get an styled outline.
 */
typedef enum /*< flags,prefix=XFDASHBOARD_BACKGROUND_TYPE >*/
{
	XFDASHBOARD_BACKGROUND_TYPE_NONE=0,

	XFDASHBOARD_BACKGROUND_TYPE_FILL=1 << 0,
	XFDASHBOARD_BACKGROUND_TYPE_OUTLINE=1 << 1,
	XFDASHBOARD_BACKGROUND_TYPE_ROUNDED_CORNERS=1 << 2,
} XfdashboardBackgroundType;


/* Object declaration */
#define XFDASHBOARD_TYPE_BACKGROUND					(xfdashboard_background_get_type())
#define XFDASHBOARD_BACKGROUND(obj)					(G_TYPE_CHECK_INSTANCE_CAST((obj), XFDASHBOARD_TYPE_BACKGROUND, XfdashboardBackground))
#define XFDASHBOARD_IS_BACKGROUND(obj)				(G_TYPE_CHECK_INSTANCE_TYPE((obj), XFDASHBOARD_TYPE_BACKGROUND))
#define XFDASHBOARD_BACKGROUND_CLASS(klass)			(G_TYPE_CHECK_CLASS_CAST((klass), XFDASHBOARD_TYPE_BACKGROUND, XfdashboardBackgroundClass))
#define XFDASHBOARD_IS_BACKGROUND_CLASS(klass)		(G_TYPE_CHECK_CLASS_TYPE((klass), XFDASHBOARD_TYPE_BACKGROUND))
#define XFDASHBOARD_BACKGROUND_GET_CLASS(obj)		(G_TYPE_INSTANCE_GET_CLASS((obj), XFDASHBOARD_TYPE_BACKGROUND, XfdashboardBackgroundClass))

typedef struct _XfdashboardBackground				XfdashboardBackground;
typedef struct _XfdashboardBackgroundClass			XfdashboardBackgroundClass;
typedef struct _XfdashboardBackgroundPrivate		XfdashboardBackgroundPrivate;

struct _XfdashboardBackground
{
	/*< private >*/
	/* Parent instance */
	XfdashboardActor				parent_instance;

	/* Private structure */
	XfdashboardBackgroundPrivate	*priv;
};

struct _XfdashboardBackgroundClass
{
	/*< private >*/
	/* Parent class */
	XfdashboardActorClass			parent_class;

	/*< public >*/
	/* Virtual functions */
};

/* Public API */
GType xfdashboard_background_get_type(void) G_GNUC_CONST;

ClutterActor* xfdashboard_background_new(void);

/* General functions */
XfdashboardBackgroundType xfdashboard_background_get_background_type(XfdashboardBackground *self);
void xfdashboard_background_set_background_type(XfdashboardBackground *self, const XfdashboardBackgroundType inType);

void xfdashboard_background_set_corners(XfdashboardBackground *self, XfdashboardCorners inCorners);
void xfdashboard_background_set_corner_radius(XfdashboardBackground *self, const gfloat inRadius);

/* Fill functions */
const ClutterColor* xfdashboard_background_get_fill_color(XfdashboardBackground *self);
void xfdashboard_background_set_fill_color(XfdashboardBackground *self, const ClutterColor *inColor);

XfdashboardCorners xfdashboard_background_get_fill_corners(XfdashboardBackground *self);
void xfdashboard_background_set_fill_corners(XfdashboardBackground *self, XfdashboardCorners inCorners);

gfloat xfdashboard_background_get_fill_corner_radius(XfdashboardBackground *self);
void xfdashboard_background_set_fill_corner_radius(XfdashboardBackground *self, const gfloat inRadius);

/* Outline functions */
const ClutterColor* xfdashboard_background_get_outline_color(XfdashboardBackground *self);
void xfdashboard_background_set_outline_color(XfdashboardBackground *self, const ClutterColor *inColor);

gfloat xfdashboard_background_get_outline_width(XfdashboardBackground *self);
void xfdashboard_background_set_outline_width(XfdashboardBackground *self, const gfloat inWidth);

XfdashboardBorders xfdashboard_background_get_outline_borders(XfdashboardBackground *self);
void xfdashboard_background_set_outline_borders(XfdashboardBackground *self, XfdashboardBorders inBorders);

XfdashboardCorners xfdashboard_background_get_outline_corners(XfdashboardBackground *self);
void xfdashboard_background_set_outline_corners(XfdashboardBackground *self, XfdashboardCorners inCorners);

gfloat xfdashboard_background_get_outline_corner_radius(XfdashboardBackground *self);
void xfdashboard_background_set_outline_corner_radius(XfdashboardBackground *self, const gfloat inRadius);

/* Image functions */
ClutterImage* xfdashboard_background_get_image(XfdashboardBackground *self);
void xfdashboard_background_set_image(XfdashboardBackground *self, ClutterImage *inImage);

G_END_DECLS

#endif	/* __LIBXFDASHBOARD_BACKGROUND__ */
