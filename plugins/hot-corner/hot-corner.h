/*
 * hot-corner: Activates application when pointer is move to a corner
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

#ifndef __XFDASHBOARD_HOT_CORNER__
#define __XFDASHBOARD_HOT_CORNER__

#include <libxfdashboard/libxfdashboard.h>

G_BEGIN_DECLS

/* Public definitions */
typedef enum /*< prefix=XFDASHBOARD_HOT_CORNER_ACTIVATION_CORNER >*/
{
	XFDASHBOARD_HOT_CORNER_ACTIVATION_CORNER_TOP_LEFT=0,
	XFDASHBOARD_HOT_CORNER_ACTIVATION_CORNER_TOP_RIGHT,
	XFDASHBOARD_HOT_CORNER_ACTIVATION_CORNER_BOTTOM_LEFT,
	XFDASHBOARD_HOT_CORNER_ACTIVATION_CORNER_BOTTOM_RIGHT,
} XfdashboardHotCornerActivationCorner;

GType xfdashboard_hot_corner_activation_corner_get_type(void) G_GNUC_CONST;
#define XFDASHBOARD_TYPE_HOT_CORNER_ACTIVATION_CORNER	(xfdashboard_hot_corner_activation_corner_get_type())


/* Object declaration */
#define XFDASHBOARD_TYPE_HOT_CORNER				(xfdashboard_hot_corner_get_type())
#define XFDASHBOARD_HOT_CORNER(obj)				(G_TYPE_CHECK_INSTANCE_CAST((obj), XFDASHBOARD_TYPE_HOT_CORNER, XfdashboardHotCorner))
#define XFDASHBOARD_IS_HOT_CORNER(obj)			(G_TYPE_CHECK_INSTANCE_TYPE((obj), XFDASHBOARD_TYPE_HOT_CORNER))
#define XFDASHBOARD_HOT_CORNER_CLASS(klass)		(G_TYPE_CHECK_CLASS_CAST((klass), XFDASHBOARD_TYPE_HOT_CORNER, XfdashboardHotCornerClass))
#define XFDASHBOARD_IS_HOT_CORNER_CLASS(klass)	(G_TYPE_CHECK_CLASS_TYPE((klass), XFDASHBOARD_TYPE_HOT_CORNER))
#define XFDASHBOARD_HOT_CORNER_GET_CLASS(obj)	(G_TYPE_INSTANCE_GET_CLASS((obj), XFDASHBOARD_TYPE_HOT_CORNER, XfdashboardHotCornerClass))

typedef struct _XfdashboardHotCorner			XfdashboardHotCorner; 
typedef struct _XfdashboardHotCornerPrivate		XfdashboardHotCornerPrivate;
typedef struct _XfdashboardHotCornerClass		XfdashboardHotCornerClass;

struct _XfdashboardHotCorner
{
	/* Parent instance */
	GObject						parent_instance;

	/* Private structure */
	XfdashboardHotCornerPrivate	*priv;
};

struct _XfdashboardHotCornerClass
{
	/*< private >*/
	/* Parent class */
	GObjectClass				parent_class;
};

/* Public API */
GType xfdashboard_hot_corner_get_type(void) G_GNUC_CONST;

XFDASHBOARD_DECLARE_PLUGIN_TYPE(xfdashboard_hot_corner);

XfdashboardHotCorner* xfdashboard_hot_corner_new(void);

G_END_DECLS

#endif
