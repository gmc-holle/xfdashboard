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

#ifndef __LIBXFDASHBOARD_WINDOW_TRACKER_MONITOR__
#define __LIBXFDASHBOARD_WINDOW_TRACKER_MONITOR__

#if !defined(__LIBXFDASHBOARD_H_INSIDE__) && !defined(LIBXFDASHBOARD_COMPILATION)
#error "Only <libxfdashboard/libxfdashboard.h> can be included directly."
#endif

#include <glib-object.h>

G_BEGIN_DECLS

#define XFDASHBOARD_TYPE_WINDOW_TRACKER_MONITOR					(xfdashboard_window_tracker_monitor_get_type())
#define XFDASHBOARD_WINDOW_TRACKER_MONITOR(obj)					(G_TYPE_CHECK_INSTANCE_CAST((obj), XFDASHBOARD_TYPE_WINDOW_TRACKER_MONITOR, XfdashboardWindowTrackerMonitor))
#define XFDASHBOARD_IS_WINDOW_TRACKER_MONITOR(obj)				(G_TYPE_CHECK_INSTANCE_TYPE((obj), XFDASHBOARD_TYPE_WINDOW_TRACKER_MONITOR))
#define XFDASHBOARD_WINDOW_TRACKER_MONITOR_GET_IFACE(obj)		(G_TYPE_INSTANCE_GET_INTERFACE((obj), XFDASHBOARD_TYPE_WINDOW_TRACKER_MONITOR, XfdashboardWindowTrackerMonitorInterface))

typedef struct _XfdashboardWindowTrackerMonitor					XfdashboardWindowTrackerMonitor;
typedef struct _XfdashboardWindowTrackerMonitorInterface		XfdashboardWindowTrackerMonitorInterface;

struct _XfdashboardWindowTrackerMonitorInterface
{
	/*< private >*/
	/* Parent class */
	GTypeInterface						parent_interface;

	/*< public >*/
	/* Virtual functions */
	gboolean (*is_equal)(XfdashboardWindowTrackerMonitor *inLeft, XfdashboardWindowTrackerMonitor *inRight);

	gboolean (*is_primary)(XfdashboardWindowTrackerMonitor *self);
	gint (*get_number)(XfdashboardWindowTrackerMonitor *self);

	void (*get_geometry)(XfdashboardWindowTrackerMonitor *self, gint *outX, gint *outY, gint *outWidth, gint *outHeight);

	/* Signals */
	void (*primary_changed)(XfdashboardWindowTrackerMonitor *self);
	void (*geometry_changed)(XfdashboardWindowTrackerMonitor *self);
};

/* Public API */
GType xfdashboard_window_tracker_monitor_get_type(void) G_GNUC_CONST;

gboolean xfdashboard_window_tracker_monitor_is_equal(XfdashboardWindowTrackerMonitor *inLeft,
														XfdashboardWindowTrackerMonitor *inRight);

gint xfdashboard_window_tracker_monitor_get_number(XfdashboardWindowTrackerMonitor *self);

gboolean xfdashboard_window_tracker_monitor_is_primary(XfdashboardWindowTrackerMonitor *self);

void xfdashboard_window_tracker_monitor_get_geometry(XfdashboardWindowTrackerMonitor *self,
														gint *outX,
														gint *outY,
														gint *outWidth,
														gint *outHeight);
gboolean xfdashboard_window_tracker_monitor_contains(XfdashboardWindowTrackerMonitor *self,
														gint inX,
														gint inY);

G_END_DECLS

#endif	/* __LIBXFDASHBOARD_WINDOW_TRACKER_MONITOR__ */
