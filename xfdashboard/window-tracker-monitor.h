/*
 * window-tracker-monitor: A monitor object tracked by window tracker.
 *                         It provides information about position and
 *                         size of monitor within screen and also a flag
 *                         if this monitor is the primary one.
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

#ifndef __XFDASHBOARD_WINDOW_TRACKER_MONITOR__
#define __XFDASHBOARD_WINDOW_TRACKER_MONITOR__

#include <glib-object.h>

G_BEGIN_DECLS

#define XFDASHBOARD_TYPE_WINDOW_TRACKER_MONITOR					(xfdashboard_window_tracker_monitor_get_type())
#define XFDASHBOARD_WINDOW_TRACKER_MONITOR(obj)					(G_TYPE_CHECK_INSTANCE_CAST((obj), XFDASHBOARD_TYPE_WINDOW_TRACKER_MONITOR, XfdashboardWindowTrackerMonitor))
#define XFDASHBOARD_IS_WINDOW_TRACKER_MONITOR(obj)				(G_TYPE_CHECK_INSTANCE_TYPE((obj), XFDASHBOARD_TYPE_WINDOW_TRACKER_MONITOR))
#define XFDASHBOARD_WINDOW_TRACKER_MONITOR_CLASS(klass)			(G_TYPE_CHECK_CLASS_CAST((klass), XFDASHBOARD_TYPE_WINDOW_TRACKER_MONITOR, XfdashboardWindowTrackerMonitorClass))
#define XFDASHBOARD_IS_WINDOW_TRACKER_MONITOR_CLASS(klass)		(G_TYPE_CHECK_CLASS_TYPE((klass), XFDASHBOARD_TYPE_WINDOW_TRACKER_MONITOR))
#define XFDASHBOARD_WINDOW_TRACKER_MONITOR_GET_CLASS(obj)		(G_TYPE_INSTANCE_GET_CLASS((obj), XFDASHBOARD_TYPE_WINDOW_TRACKER_MONITOR, XfdashboardWindowTrackerMonitorClass))

typedef struct _XfdashboardWindowTrackerMonitor					XfdashboardWindowTrackerMonitor;
typedef struct _XfdashboardWindowTrackerMonitorClass			XfdashboardWindowTrackerMonitorClass;
typedef struct _XfdashboardWindowTrackerMonitorPrivate			XfdashboardWindowTrackerMonitorPrivate;

struct _XfdashboardWindowTrackerMonitor
{
	/* Parent instance */
	GObject										parent_instance;

	/* Private structure */
	XfdashboardWindowTrackerMonitorPrivate		*priv;
};

struct _XfdashboardWindowTrackerMonitorClass
{
	/*< private >*/
	/* Parent class */
	GObjectClass								parent_class;

	/*< public >*/
	/* Virtual functions */
	void (*primary_changed)(XfdashboardWindowTrackerMonitor *self);
	void (*geometry_changed)(XfdashboardWindowTrackerMonitor *self);
};

/* Public API */
GType xfdashboard_window_tracker_monitor_get_type(void) G_GNUC_CONST;

gint xfdashboard_window_tracker_monitor_get_number(XfdashboardWindowTrackerMonitor *self);

gboolean xfdashboard_window_tracker_monitor_is_primary(XfdashboardWindowTrackerMonitor *self);

gint xfdashboard_window_tracker_monitor_get_x(XfdashboardWindowTrackerMonitor *self);
gint xfdashboard_window_tracker_monitor_get_y(XfdashboardWindowTrackerMonitor *self);
gint xfdashboard_window_tracker_monitor_get_width(XfdashboardWindowTrackerMonitor *self);
gint xfdashboard_window_tracker_monitor_get_height(XfdashboardWindowTrackerMonitor *self);
void xfdashboard_window_tracker_monitor_get_geometry(XfdashboardWindowTrackerMonitor *self,
														gint *outX,
														gint *outY,
														gint *outWidth,
														gint *outHeight);

G_END_DECLS

#endif	/* __XFDASHBOARD_WINDOW_TRACKER_MONITOR__ */
