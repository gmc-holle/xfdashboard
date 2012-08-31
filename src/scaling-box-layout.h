/*
 * scaling-box-layout.c.h: A box layout scaling all actor to fit in
 *                         allocation of parent actor
 * 
 * Copyright 2012 Stephan Haller <nomad@froevel.de>
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

#ifndef __XFOVERVIEW_SCALING_BOX_LAYOUT__
#define __XFOVERVIEW_SCALING_BOX_LAYOUT__

#include <clutter/clutter.h>

G_BEGIN_DECLS

#define XFDASHBOARD_TYPE_SCALING_BOX_LAYOUT				(xfdashboard_scaling_box_layout_get_type())
#define XFDASHBOARD_SCALING_BOX_LAYOUT(obj)				(G_TYPE_CHECK_INSTANCE_CAST((obj), XFDASHBOARD_TYPE_SCALING_BOX_LAYOUT, XfdashboardScalingBoxLayout))
#define XFDASHBOARD_IS_SCALING_BOX_LAYOUT(obj)			(G_TYPE_CHECK_INSTANCE_TYPE((obj), XFDASHBOARD_TYPE_SCALING_BOX_LAYOUT))
#define XFDASHBOARD_SCALING_BOX_LAYOUT_CLASS(klass)		(G_TYPE_CHECK_CLASS_CAST((klass), XFDASHBOARD_TYPE_SCALING_BOX_LAYOUT, XfdashboardScalingBoxLayoutClass))
#define XFDASHBOARD_IS_SCALING_BOX_LAYOUT_CLASS(klass)	(G_TYPE_CHECK_CLASS_TYPE((klass), XFDASHBOARD_TYPE_SCALING_BOX_LAYOUT))
#define XFDASHBOARD_SCALING_BOX_LAYOUT_GET_CLASS(obj)	(G_TYPE_INSTANCE_GET_CLASS((obj), XFDASHBOARD_TYPE_SCALING_BOX_LAYOUT, XfdashboardScalingBoxLayoutClass))

typedef struct _XfdashboardScalingBoxLayout				XfdashboardScalingBoxLayout;
typedef struct _XfdashboardScalingBoxLayoutPrivate		XfdashboardScalingBoxLayoutPrivate;
typedef struct _XfdashboardScalingBoxLayoutClass		XfdashboardScalingBoxLayoutClass;

struct _XfdashboardScalingBoxLayout
{
	/* Parent instance */
	ClutterLayoutManager 				parent_instance;

	/* Private structure */
	XfdashboardScalingBoxLayoutPrivate	*priv;
};

struct _XfdashboardScalingBoxLayoutClass
{
	/* Parent class */
	ClutterLayoutManagerClass			parent_class;
};

GType xfdashboard_scaling_box_layout_get_type(void) G_GNUC_CONST;

ClutterLayoutManager* xfdashboard_scaling_box_layout_new();

gfloat xfdashboard_scaling_box_layout_get_scale(XfdashboardScalingBoxLayout *self);

gfloat xfdashboard_scaling_box_layout_get_scale_minimum(XfdashboardScalingBoxLayout *self);
void xfdashboard_scaling_box_layout_set_scale_minimum(XfdashboardScalingBoxLayout *self, gfloat inScale);

gfloat xfdashboard_scaling_box_layout_get_scale_maximum(XfdashboardScalingBoxLayout *self);
void xfdashboard_scaling_box_layout_set_scale_maximum(XfdashboardScalingBoxLayout *self, gfloat inScale);

gfloat xfdashboard_scaling_box_layout_get_scale_step(XfdashboardScalingBoxLayout *self);
void xfdashboard_scaling_box_layout_set_scale_step(XfdashboardScalingBoxLayout *self, gfloat inScale);

gfloat xfdashboard_scaling_box_layout_get_spacing(XfdashboardScalingBoxLayout *self);
void xfdashboard_scaling_box_layout_set_spacing(XfdashboardScalingBoxLayout *self, gfloat inSpacing);

const ClutterActorBox* xfdashboard_scaling_box_layout_get_last_allocation(XfdashboardScalingBoxLayout *self);

G_END_DECLS

#endif
