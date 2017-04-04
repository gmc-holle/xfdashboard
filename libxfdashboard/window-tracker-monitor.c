/*
 * window-tracker-monitor: A monitor object tracked by window tracker.
 *                         It provides information about position and
 *                         size of monitor within screen and also a flag
 *                         if this monitor is the primary one.
 * 
 * Copyright 2012-2016 Stephan Haller <nomad@froevel.de>
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

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <libxfdashboard/window-tracker-monitor.h>

#include <glib/gi18n-lib.h>

#include <libxfdashboard/compat.h>


/* Define this class in GObject system */
G_DEFINE_INTERFACE(XfdashboardWindowTrackerMonitor,
					xfdashboard_window_tracker_monitor,
					G_TYPE_OBJECT)


/* Signals */
enum
{
	SIGNAL_PRIMARY_CHANGED,
	SIGNAL_GEOMETRY_CHANGED,

	SIGNAL_LAST
};

static guint XfdashboardWindowTrackerMonitorSignals[SIGNAL_LAST]={ 0, };


/* IMPLEMENTATION: Private variables and methods */
#define XFDASHBOARD_WINDOWS_TRACKER_MONITOR_WARN_NOT_IMPLEMENTED(self, vfunc)\
	g_warning(_("Object of type %s does not implement required virtual function XfdashboardWindowTrackerMonitor::%s"),\
				G_OBJECT_TYPE_NAME(self), \
				vfunc);

/* Default implementation of virtual function "is_equal" */
static gboolean _xfdashboard_window_tracker_monitor_real_is_equal(XfdashboardWindowTrackerMonitor *inLeft,
																	XfdashboardWindowTrackerMonitor *inRight)
{
	gint			leftIndex, rightIndex;

	g_return_val_if_fail(XFDASHBOARD_IS_WINDOW_TRACKER_MONITOR(inLeft), FALSE);
	g_return_val_if_fail(XFDASHBOARD_IS_WINDOW_TRACKER_MONITOR(inRight), FALSE);

	/* Check if both are the same workspace or refer to same one */
	leftIndex=xfdashboard_window_tracker_monitor_get_number(inLeft);
	rightIndex=xfdashboard_window_tracker_monitor_get_number(inRight);
	if(inLeft==inRight || leftIndex==rightIndex) return(TRUE);

	/* If we get here then they cannot be considered equal */
	return(FALSE);
}


/* IMPLEMENTATION: GObject */

/* Interface initialization
 * Set up default functions
 */
static void xfdashboard_window_tracker_monitor_default_init(XfdashboardWindowTrackerMonitorInterface *iface)
{
	static gboolean		initialized=FALSE;
	GParamSpec			*property;

	/* The following virtual functions should be overriden if default
	 * implementation does not fit.
	 */
	iface->is_equal=_xfdashboard_window_tracker_monitor_real_is_equal;

	/* Define properties, signals and actions */
	if(!initialized)
	{
		/* Define properties */
		property=g_param_spec_int("monitor-index",
									_("Monitor index"),
									_("The index of this monitor"),
									0, G_MAXINT,
									0,
									G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS | G_PARAM_CONSTRUCT_ONLY);
		g_object_interface_install_property(iface, property);

		property=g_param_spec_boolean("is-primary",
										_("Is primary"),
										_("Whether this monitor is the primary one"),
										FALSE,
										G_PARAM_READABLE | G_PARAM_STATIC_STRINGS);
		g_object_interface_install_property(iface, property);

		/* Define signals */
		XfdashboardWindowTrackerMonitorSignals[SIGNAL_PRIMARY_CHANGED]=
			g_signal_new("primary-changed",
							G_TYPE_FROM_INTERFACE(iface),
							G_SIGNAL_RUN_LAST,
							G_STRUCT_OFFSET(XfdashboardWindowTrackerMonitorInterface, primary_changed),
							NULL,
							NULL,
							g_cclosure_marshal_VOID__VOID,
							G_TYPE_NONE,
							0);

		XfdashboardWindowTrackerMonitorSignals[SIGNAL_GEOMETRY_CHANGED]=
			g_signal_new("geometry-changed",
							G_TYPE_FROM_INTERFACE(iface),
							G_SIGNAL_RUN_LAST,
							G_STRUCT_OFFSET(XfdashboardWindowTrackerMonitorInterface, geometry_changed),
							NULL,
							NULL,
							g_cclosure_marshal_VOID__VOID,
							G_TYPE_NONE,
							0);

		/* Set flag that base initialization was done for this interface */
		initialized=TRUE;
	}
}


/* IMPLEMENTATION: Public API */

/* Check if both monitors are the same */
gboolean xfdashboard_window_tracker_monitor_is_equal(XfdashboardWindowTrackerMonitor *inLeft,
														XfdashboardWindowTrackerMonitor *inRight)
{
	XfdashboardWindowTrackerMonitorInterface		*iface;

	g_return_val_if_fail(XFDASHBOARD_IS_WINDOW_TRACKER_MONITOR(inLeft), FALSE);
	g_return_val_if_fail(XFDASHBOARD_IS_WINDOW_TRACKER_MONITOR(inRight), FALSE);

	iface=XFDASHBOARD_WINDOW_TRACKER_MONITOR_GET_IFACE(inLeft);

	/* Call virtual function */
	if(iface->is_equal)
	{
		return(iface->is_equal(inLeft, inRight));
	}

	/* If we get here the virtual function was not overridden */
	XFDASHBOARD_WINDOWS_TRACKER_MONITOR_WARN_NOT_IMPLEMENTED(inLeft, "is_equal");
	return(FALSE);
}

/* Get monitor index */
gint xfdashboard_window_tracker_monitor_get_number(XfdashboardWindowTrackerMonitor *self)
{
	XfdashboardWindowTrackerMonitorInterface		*iface;

	g_return_val_if_fail(XFDASHBOARD_IS_WINDOW_TRACKER_MONITOR(self), 0);

	iface=XFDASHBOARD_WINDOW_TRACKER_MONITOR_GET_IFACE(self);

	/* Call virtual function */
	if(iface->get_number)
	{
		return(iface->get_number(self));
	}

	/* If we get here the virtual function was not overridden */
	XFDASHBOARD_WINDOWS_TRACKER_MONITOR_WARN_NOT_IMPLEMENTED(self, "get_number");
	return(0);
}

/* Determine if monitor is primary one */
gboolean xfdashboard_window_tracker_monitor_is_primary(XfdashboardWindowTrackerMonitor *self)
{
	XfdashboardWindowTrackerMonitorInterface		*iface;

	g_return_val_if_fail(XFDASHBOARD_IS_WINDOW_TRACKER_MONITOR(self), FALSE);

	iface=XFDASHBOARD_WINDOW_TRACKER_MONITOR_GET_IFACE(self);

	/* Call virtual function */
	if(iface->is_primary)
	{
		return(iface->is_primary(self));
	}

	/* If we get here the virtual function was not overridden */
	XFDASHBOARD_WINDOWS_TRACKER_MONITOR_WARN_NOT_IMPLEMENTED(self, "get_number");
	return(FALSE);
}

/* Get geometry of monitor */
void xfdashboard_window_tracker_monitor_get_geometry(XfdashboardWindowTrackerMonitor *self,
														gint *outX,
														gint *outY,
														gint *outWidth,
														gint *outHeight)
{
	XfdashboardWindowTrackerMonitorInterface		*iface;
	gint											x, y, w, h;

	g_return_if_fail(XFDASHBOARD_IS_WINDOW_TRACKER_MONITOR(self));

	iface=XFDASHBOARD_WINDOW_TRACKER_MONITOR_GET_IFACE(self);

	/* Get monitor geometry */
	if(iface->get_geometry)
	{
		/* Get geometry */
		iface->get_geometry(self, &x, &y, &w, &h);

		/* Set result */
		if(outX) *outX=x;
		if(outX) *outY=y;
		if(outWidth) *outWidth=w;
		if(outHeight) *outHeight=h;

		return;
	}

	/* If we get here the virtual function was not overridden */
	XFDASHBOARD_WINDOWS_TRACKER_MONITOR_WARN_NOT_IMPLEMENTED(self, "get_geometry");
}

/* Check if requested position is inside monitor's geometry */
gboolean xfdashboard_window_tracker_monitor_contains(XfdashboardWindowTrackerMonitor *self,
														gint inX,
														gint inY)
{
	gint										x, y, width, height;

	g_return_val_if_fail(XFDASHBOARD_IS_WINDOW_TRACKER_MONITOR(self), FALSE);

	/* Get monitor's geometry */
	xfdashboard_window_tracker_monitor_get_geometry(self, &x, &y, &width, &height);

	/* Check if requested position is inside monitor's geometry */
	if(inX>=x &&
		inX<(x+width) &&
		inY>=y &&
		inY<(y+height))
	{
		return(TRUE);
	}

	/* If we get here the requested position is not inside this monitor's geometry,
	 * so return FALSE.
	 */
	return(FALSE);
}
