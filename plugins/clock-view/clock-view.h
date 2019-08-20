/*
 * clock-view: A view showing a clock
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

#ifndef __XFDASHBOARD_CLOCK_VIEW__
#define __XFDASHBOARD_CLOCK_VIEW__

#include <libxfdashboard/libxfdashboard.h>

G_BEGIN_DECLS

#define XFDASHBOARD_TYPE_CLOCK_VIEW				(xfdashboard_clock_view_get_type())
#define XFDASHBOARD_CLOCK_VIEW(obj)				(G_TYPE_CHECK_INSTANCE_CAST((obj), XFDASHBOARD_TYPE_CLOCK_VIEW, XfdashboardClockView))
#define XFDASHBOARD_IS_CLOCK_VIEW(obj)			(G_TYPE_CHECK_INSTANCE_TYPE((obj), XFDASHBOARD_TYPE_CLOCK_VIEW))
#define XFDASHBOARD_CLOCK_VIEW_CLASS(klass)		(G_TYPE_CHECK_CLASS_CAST((klass), XFDASHBOARD_TYPE_CLOCK_VIEW, XfdashboardClockViewClass))
#define XFDASHBOARD_IS_CLOCK_VIEW_CLASS(klass)	(G_TYPE_CHECK_CLASS_TYPE((klass), XFDASHBOARD_TYPE_CLOCK_VIEW))
#define XFDASHBOARD_CLOCK_VIEW_GET_CLASS(obj)	(G_TYPE_INSTANCE_GET_CLASS((obj), XFDASHBOARD_TYPE_CLOCK_VIEW, XfdashboardClockViewClass))

typedef struct _XfdashboardClockView			XfdashboardClockView; 
typedef struct _XfdashboardClockViewPrivate		XfdashboardClockViewPrivate;
typedef struct _XfdashboardClockViewClass		XfdashboardClockViewClass;

struct _XfdashboardClockView
{
	/* Parent instance */
	XfdashboardView						parent_instance;

	/* Private structure */
	XfdashboardClockViewPrivate			*priv;
};

struct _XfdashboardClockViewClass
{
	/*< private >*/
	/* Parent class */
	XfdashboardViewClass				parent_class;
};

/* Public API */
GType xfdashboard_clock_view_get_type(void) G_GNUC_CONST;

XFDASHBOARD_DECLARE_PLUGIN_TYPE(xfdashboard_clock_view);

G_END_DECLS

#endif
