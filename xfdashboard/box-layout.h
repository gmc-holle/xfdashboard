/*
 * box-layout: A ClutterBoxLayout derived layout manager disregarding
 *             text direction and enforcing left-to-right layout in
 *             horizontal orientation
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

#ifndef __XFDASHBOARD_BOX_LAYOUT__
#define __XFDASHBOARD_BOX_LAYOUT__

#include <clutter/clutter.h>

G_BEGIN_DECLS

#define XFDASHBOARD_TYPE_BOX_LAYOUT				(xfdashboard_box_layout_get_type())
#define XFDASHBOARD_BOX_LAYOUT(obj)				(G_TYPE_CHECK_INSTANCE_CAST((obj), XFDASHBOARD_TYPE_BOX_LAYOUT, XfdashboardBoxLayout))
#define XFDASHBOARD_IS_BOX_LAYOUT(obj)			(G_TYPE_CHECK_INSTANCE_TYPE((obj), XFDASHBOARD_TYPE_BOX_LAYOUT))
#define XFDASHBOARD_BOX_LAYOUT_CLASS(klass)		(G_TYPE_CHECK_CLASS_CAST((klass), XFDASHBOARD_TYPE_BOX_LAYOUT, XfdashboardBoxLayoutClass))
#define XFDASHBOARD_IS_BOX_LAYOUT_CLASS(klass)	(G_TYPE_CHECK_CLASS_TYPE((klass), XFDASHBOARD_TYPE_BOX_LAYOUT))
#define XFDASHBOARD_BOX_LAYOUT_GET_CLASS(obj)	(G_TYPE_INSTANCE_GET_CLASS((obj), XFDASHBOARD_TYPE_BOX_LAYOUT, XfdashboardBoxLayoutClass))

typedef struct _XfdashboardBoxLayout			XfdashboardBoxLayout;
typedef struct _XfdashboardBoxLayoutClass		XfdashboardBoxLayoutClass;

struct _XfdashboardBoxLayout
{
	/* Parent instance */
	ClutterBoxLayout 				parent_instance;
};

struct _XfdashboardBoxLayoutClass
{
	/*< private >*/
	/* Parent class */
	ClutterBoxLayoutClass			parent_class;
};

/* Public API */
GType xfdashboard_box_layout_get_type(void) G_GNUC_CONST;

ClutterLayoutManager* xfdashboard_box_layout_new(void);

G_END_DECLS

#endif	/* __XFDASHBOARD_BOX_LAYOUT__ */
