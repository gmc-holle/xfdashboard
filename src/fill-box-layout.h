/*
 * fill-box-layout.h: A box layout expanding actors in direction of axis
 *                    (filling) and using natural size in other direction
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

#ifndef __XFOVERVIEW_FILL_BOX_LAYOUT__
#define __XFOVERVIEW_FILL_BOX_LAYOUT__

#include <clutter/clutter.h>

G_BEGIN_DECLS

#define XFDASHBOARD_TYPE_FILL_BOX_LAYOUT				(xfdashboard_fill_box_layout_get_type())
#define XFDASHBOARD_FILL_BOX_LAYOUT(obj)				(G_TYPE_CHECK_INSTANCE_CAST((obj), XFDASHBOARD_TYPE_FILL_BOX_LAYOUT, XfdashboardFillBoxLayout))
#define XFDASHBOARD_IS_FILL_BOX_LAYOUT(obj)				(G_TYPE_CHECK_INSTANCE_TYPE((obj), XFDASHBOARD_TYPE_FILL_BOX_LAYOUT))
#define XFDASHBOARD_FILL_BOX_LAYOUT_CLASS(klass)		(G_TYPE_CHECK_CLASS_CAST((klass), XFDASHBOARD_TYPE_FILL_BOX_LAYOUT, XfdashboardFillBoxLayoutClass))
#define XFDASHBOARD_IS_FILL_BOX_LAYOUT_CLASS(klass)		(G_TYPE_CHECK_CLASS_TYPE((klass), XFDASHBOARD_TYPE_FILL_BOX_LAYOUT))
#define XFDASHBOARD_FILL_BOX_LAYOUT_GET_CLASS(obj)		(G_TYPE_INSTANCE_GET_CLASS((obj), XFDASHBOARD_TYPE_FILL_BOX_LAYOUT, XfdashboardFillBoxLayoutClass))

typedef struct _XfdashboardFillBoxLayout				XfdashboardFillBoxLayout;
typedef struct _XfdashboardFillBoxLayoutPrivate			XfdashboardFillBoxLayoutPrivate;
typedef struct _XfdashboardFillBoxLayoutClass			XfdashboardFillBoxLayoutClass;

struct _XfdashboardFillBoxLayout
{
	/* Parent instance */
	ClutterLayoutManager 				parent_instance;

	/* Private structure */
	XfdashboardFillBoxLayoutPrivate		*priv;
};

struct _XfdashboardFillBoxLayoutClass
{
	/* Parent class */
	ClutterLayoutManagerClass			parent_class;
};

/* Public API */
GType xfdashboard_fill_box_layout_get_type(void) G_GNUC_CONST;

ClutterLayoutManager* xfdashboard_fill_box_layout_new();

gboolean xfdashboard_fill_box_layout_get_vertical(XfdashboardFillBoxLayout *self);
void xfdashboard_fill_box_layout_set_vertical(XfdashboardFillBoxLayout *self, gboolean inIsVertical);

gboolean xfdashboard_fill_box_layout_get_homogenous(XfdashboardFillBoxLayout *self);
void xfdashboard_fill_box_layout_set_homogenous(XfdashboardFillBoxLayout *self, gboolean inIsHomogenous);

gfloat xfdashboard_fill_box_layout_get_spacing(XfdashboardFillBoxLayout *self);
void xfdashboard_fill_box_layout_set_spacing(XfdashboardFillBoxLayout *self, gfloat inSpacing);

G_END_DECLS

#endif
