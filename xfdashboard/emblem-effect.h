/*
 * emblem-effect: Draws an emblem on top of an actor
 * 
 * Copyright 2012-2015 Stephan Haller <nomad@froevel.de>
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

#ifndef __XFDASHBOARD_EMBLEM_EFFECT__
#define __XFDASHBOARD_EMBLEM_EFFECT__

#include <clutter/clutter.h>

#include "types.h"

G_BEGIN_DECLS

#define XFDASHBOARD_TYPE_EMBLEM_EFFECT				(xfdashboard_emblem_effect_get_type())
#define XFDASHBOARD_EMBLEM_EFFECT(obj)				(G_TYPE_CHECK_INSTANCE_CAST((obj), XFDASHBOARD_TYPE_EMBLEM_EFFECT, XfdashboardEmblemEffect))
#define XFDASHBOARD_IS_EMBLEM_EFFECT(obj)			(G_TYPE_CHECK_INSTANCE_TYPE((obj), XFDASHBOARD_TYPE_EMBLEM_EFFECT))
#define XFDASHBOARD_EMBLEM_EFFECT_CLASS(klass)		(G_TYPE_CHECK_CLASS_CAST((klass), XFDASHBOARD_TYPE_EMBLEM_EFFECT, XfdashboardEmblemEffectClass))
#define XFDASHBOARD_IS_EMBLEM_EFFECT_CLASS(klass)	(G_TYPE_CHECK_CLASS_TYPE((klass), XFDASHBOARD_TYPE_EMBLEM_EFFECT))
#define XFDASHBOARD_EMBLEM_EFFECT_GET_CLASS(obj)	(G_TYPE_INSTANCE_GET_CLASS((obj), XFDASHBOARD_TYPE_EMBLEM_EFFECT, XfdashboardEmblemEffectClass))

typedef struct _XfdashboardEmblemEffect			XfdashboardEmblemEffect;
typedef struct _XfdashboardEmblemEffectClass		XfdashboardEmblemEffectClass;
typedef struct _XfdashboardEmblemEffectPrivate		XfdashboardEmblemEffectPrivate;

struct _XfdashboardEmblemEffect
{
	/* Parent instance */
	ClutterEffect						parent_instance;

	/* Private structure */
	XfdashboardEmblemEffectPrivate		*priv;
};

struct _XfdashboardEmblemEffectClass
{
	/*< private >*/
	/* Parent class */
	ClutterEffectClass					parent_class;
};

/* Public API */
GType xfdashboard_emblem_effect_get_type(void) G_GNUC_CONST;

ClutterEffect* xfdashboard_emblem_effect_new(void);

const gchar* xfdashboard_emblem_effect_get_icon_name(XfdashboardEmblemEffect *self);
void xfdashboard_emblem_effect_set_icon_name(XfdashboardEmblemEffect *self, const gchar *inIconName);

gint xfdashboard_emblem_effect_get_icon_size(XfdashboardEmblemEffect *self);
void xfdashboard_emblem_effect_set_icon_size(XfdashboardEmblemEffect *self, const gint inSize);

gfloat xfdashboard_emblem_effect_get_x_align(XfdashboardEmblemEffect *self);
void xfdashboard_emblem_effect_set_x_align(XfdashboardEmblemEffect *self, const gfloat inAlign);

gfloat xfdashboard_emblem_effect_get_y_align(XfdashboardEmblemEffect *self);
void xfdashboard_emblem_effect_set_y_align(XfdashboardEmblemEffect *self, const gfloat inAlign);

ClutterGravity xfdashboard_emblem_effect_get_gravity(XfdashboardEmblemEffect *self);
void xfdashboard_emblem_effect_set_gravity(XfdashboardEmblemEffect *self, const ClutterGravity inGravity);

G_END_DECLS

#endif	/* __XFDASHBOARD_EMBLEM_EFFECT__ */
