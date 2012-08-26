/*
 * scalingflowlayout.h: A flow layout scaling all actor to fit in
 *                      allocation of parent actor
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

#ifndef __XFOVERVIEW_SCALINGFLOWLAYOUT__
#define __XFOVERVIEW_SCALINGFLOWLAYOUT__

#include <clutter/clutter.h>

G_BEGIN_DECLS

#define XFDASHBOARD_TYPE_SCALINGFLOW_LAYOUT				(xfdashboard_scalingflow_layout_get_type())
#define XFDASHBOARD_SCALINGFLOW_LAYOUT(obj)				(G_TYPE_CHECK_INSTANCE_CAST((obj), XFDASHBOARD_TYPE_SCALINGFLOW_LAYOUT, XfdashboardScalingFlowLayout))
#define XFDASHBOARD_IS_SCALINGFLOW_LAYOUT(obj)			(G_TYPE_CHECK_INSTANCE_TYPE((obj), XFDASHBOARD_TYPE_SCALINGFLOW_LAYOUT))
#define XFDASHBOARD_SCALINGFLOW_LAYOUT_CLASS(klass)		(G_TYPE_CHECK_CLASS_CAST((klass), XFDASHBOARD_TYPE_SCALINGFLOW_LAYOUT, XfdashboardScalingFlowLayoutClass))
#define XFDASHBOARD_IS_SCALINGFLOW_LAYOUT_CLASS(klass)	(G_TYPE_CHECK_CLASS_TYPE((klass), XFDASHBOARD_TYPE_SCALINGFLOW_LAYOUT))
#define XFDASHBOARD_SCALINGFLOW_LAYOUT_GET_CLASS(obj)	(G_TYPE_INSTANCE_GET_CLASS((obj), XFDASHBOARD_TYPE_SCALINGFLOW_LAYOUT, XfdashboardScalingFlowLayoutClass))

typedef struct _XfdashboardScalingFlowLayout			XfdashboardScalingFlowLayout;
typedef struct _XfdashboardScalingFlowLayoutPrivate		XfdashboardScalingFlowLayoutPrivate;
typedef struct _XfdashboardScalingFlowLayoutClass		XfdashboardScalingFlowLayoutClass;

struct _XfdashboardScalingFlowLayout
{
	/* Parent instance */
	ClutterLayoutManager 				parent_instance;

	/* Private structure */
	XfdashboardScalingFlowLayoutPrivate	*priv;
};

struct _XfdashboardScalingFlowLayoutClass
{
	/* Parent class */
	ClutterLayoutManagerClass			parent_class;
};

GType xfdashboard_scalingflow_layout_get_type(void) G_GNUC_CONST;

ClutterLayoutManager* xfdashboard_scalingflow_layout_new();

gboolean xfdashboard_scalingflow_layout_get_relative_scale(XfdashboardScalingFlowLayout *self);
void xfdashboard_scalingflow_layout_set_relative_scale(XfdashboardScalingFlowLayout *self, gboolean inScaling);

void xfdashboard_scalingflow_layout_set_spacing(XfdashboardScalingFlowLayout *self, gfloat inSpacing);

gfloat xfdashboard_scalingflow_layout_get_row_spacing(XfdashboardScalingFlowLayout *self);
void xfdashboard_scalingflow_layout_set_row_spacing(XfdashboardScalingFlowLayout *self, gfloat inSpacing);

gfloat xfdashboard_scalingflow_layout_get_column_spacing(XfdashboardScalingFlowLayout *self);
void xfdashboard_scalingflow_layout_set_column_spacing(XfdashboardScalingFlowLayout *self, gfloat inSpacing);

G_END_DECLS

#endif
