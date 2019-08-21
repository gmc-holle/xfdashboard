/*
 * outline-effect: Draws an outline on top of actor
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

#ifndef __LIBXFDASHBOARD_OUTLINE_EFFECT__
#define __LIBXFDASHBOARD_OUTLINE_EFFECT__

#if !defined(__LIBXFDASHBOARD_H_INSIDE__) && !defined(LIBXFDASHBOARD_COMPILATION)
#error "Only <libxfdashboard/libxfdashboard.h> can be included directly."
#endif

#include <clutter/clutter.h>

#include <libxfdashboard/types.h>

G_BEGIN_DECLS

#define XFDASHBOARD_TYPE_OUTLINE_EFFECT				(xfdashboard_outline_effect_get_type())
#define XFDASHBOARD_OUTLINE_EFFECT(obj)				(G_TYPE_CHECK_INSTANCE_CAST((obj), XFDASHBOARD_TYPE_OUTLINE_EFFECT, XfdashboardOutlineEffect))
#define XFDASHBOARD_IS_OUTLINE_EFFECT(obj)			(G_TYPE_CHECK_INSTANCE_TYPE((obj), XFDASHBOARD_TYPE_OUTLINE_EFFECT))
#define XFDASHBOARD_OUTLINE_EFFECT_CLASS(klass)		(G_TYPE_CHECK_CLASS_CAST((klass), XFDASHBOARD_TYPE_OUTLINE_EFFECT, XfdashboardOutlineEffectClass))
#define XFDASHBOARD_IS_OUTLINE_EFFECT_CLASS(klass)	(G_TYPE_CHECK_CLASS_TYPE((klass), XFDASHBOARD_TYPE_OUTLINE_EFFECT))
#define XFDASHBOARD_OUTLINE_EFFECT_GET_CLASS(obj)	(G_TYPE_INSTANCE_GET_CLASS((obj), XFDASHBOARD_TYPE_OUTLINE_EFFECT, XfdashboardOutlineEffectClass))

typedef struct _XfdashboardOutlineEffect			XfdashboardOutlineEffect;
typedef struct _XfdashboardOutlineEffectClass		XfdashboardOutlineEffectClass;
typedef struct _XfdashboardOutlineEffectPrivate		XfdashboardOutlineEffectPrivate;

struct _XfdashboardOutlineEffect
{
	/*< private >*/
	/* Parent instance */
	ClutterEffect						parent_instance;

	/* Private structure */
	XfdashboardOutlineEffectPrivate		*priv;
};

struct _XfdashboardOutlineEffectClass
{
	/*< private >*/
	/* Parent class */
	ClutterEffectClass					parent_class;
};

/* Public API */
GType xfdashboard_outline_effect_get_type(void) G_GNUC_CONST;

ClutterEffect* xfdashboard_outline_effect_new(void);

const ClutterColor* xfdashboard_outline_effect_get_color(XfdashboardOutlineEffect *self);
void xfdashboard_outline_effect_set_color(XfdashboardOutlineEffect *self, const ClutterColor *inColor);

gfloat xfdashboard_outline_effect_get_width(XfdashboardOutlineEffect *self);
void xfdashboard_outline_effect_set_width(XfdashboardOutlineEffect *self, const gfloat inWidth);

XfdashboardBorders xfdashboard_outline_effect_get_borders(XfdashboardOutlineEffect *self);
void xfdashboard_outline_effect_set_borders(XfdashboardOutlineEffect *self, XfdashboardBorders inBorders);

XfdashboardCorners xfdashboard_outline_effect_get_corners(XfdashboardOutlineEffect *self);
void xfdashboard_outline_effect_set_corners(XfdashboardOutlineEffect *self, XfdashboardCorners inCorners);

gfloat xfdashboard_outline_effect_get_corner_radius(XfdashboardOutlineEffect *self);
void xfdashboard_outline_effect_set_corner_radius(XfdashboardOutlineEffect *self, const gfloat inRadius);

G_END_DECLS

#endif	/* __LIBXFDASHBOARD_OUTLINE_EFFECT__ */
