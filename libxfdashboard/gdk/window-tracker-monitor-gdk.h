/*
 * window-tracker-monitor: A monitor object tracked by window tracker.
 *                         It provides information about position and
 *                         size of monitor within screen and also a flag
 *                         if this monitor is the primary one.
 * 
 * Copyright 2012-2017 Stephan Haller <nomad@froevel.de>
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

#ifndef __LIBXFDASHBOARD_WINDOW_TRACKER_MONITOR_GDK__
#define __LIBXFDASHBOARD_WINDOW_TRACKER_MONITOR_GDK__

#if !defined(__LIBXFDASHBOARD_H_INSIDE__) && !defined(LIBXFDASHBOARD_COMPILATION)
#error "Only <libxfdashboard/libxfdashboard.h> can be included directly."
#endif

#include <glib-object.h>

G_BEGIN_DECLS

#define XFDASHBOARD_TYPE_WINDOW_TRACKER_MONITOR_GDK					(xfdashboard_window_tracker_monitor_gdk_get_type())
#define XFDASHBOARD_WINDOW_TRACKER_MONITOR_GDK(obj)					(G_TYPE_CHECK_INSTANCE_CAST((obj), XFDASHBOARD_TYPE_WINDOW_TRACKER_MONITOR_GDK, XfdashboardWindowTrackerMonitorGDK))
#define XFDASHBOARD_IS_WINDOW_TRACKER_MONITOR_GDK(obj)				(G_TYPE_CHECK_INSTANCE_TYPE((obj), XFDASHBOARD_TYPE_WINDOW_TRACKER_MONITOR_GDK))
#define XFDASHBOARD_WINDOW_TRACKER_MONITOR_GDK_CLASS(klass)			(G_TYPE_CHECK_CLASS_CAST((klass), XFDASHBOARD_TYPE_WINDOW_TRACKER_MONITOR_GDK, XfdashboardWindowTrackerMonitorGDKClass))
#define XFDASHBOARD_IS_WINDOW_TRACKER_MONITOR_GDK_CLASS(klass)		(G_TYPE_CHECK_CLASS_TYPE((klass), XFDASHBOARD_TYPE_WINDOW_TRACKER_MONITOR_GDK))
#define XFDASHBOARD_WINDOW_TRACKER_MONITOR_GDK_GET_CLASS(obj)		(G_TYPE_INSTANCE_GET_CLASS((obj), XFDASHBOARD_TYPE_WINDOW_TRACKER_MONITOR_GDK, XfdashboardWindowTrackerMonitorGDKClass))

typedef struct _XfdashboardWindowTrackerMonitorGDK					XfdashboardWindowTrackerMonitorGDK;
typedef struct _XfdashboardWindowTrackerMonitorGDKClass				XfdashboardWindowTrackerMonitorGDKClass;
typedef struct _XfdashboardWindowTrackerMonitorGDKPrivate			XfdashboardWindowTrackerMonitorGDKPrivate;

struct _XfdashboardWindowTrackerMonitorGDK
{
	/*< private >*/
	/* Parent instance */
	GObject										parent_instance;

	/* Private structure */
	XfdashboardWindowTrackerMonitorGDKPrivate	*priv;
};

struct _XfdashboardWindowTrackerMonitorGDKClass
{
	/*< private >*/
	/* Parent class */
	GObjectClass								parent_class;

	/*< public >*/
	/* Virtual functions */
	void (*primary_changed)(XfdashboardWindowTrackerMonitorGDK *self);
	void (*geometry_changed)(XfdashboardWindowTrackerMonitorGDK *self);
};

/* Public API */
GType xfdashboard_window_tracker_monitor_gdk_get_type(void) G_GNUC_CONST;

G_END_DECLS

#endif	/* __LIBXFDASHBOARD_WINDOW_TRACKER_MONITOR_GDK__ */
