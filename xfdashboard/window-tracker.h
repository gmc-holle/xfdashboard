/*
 * window-tracker: Tracks windows, workspaces, monitors and
 *                 listens for changes. It also bundles libwnck into one
 *                 class.
 *                 By wrapping libwnck objects we can use a virtual
 *                 stable API while the API in libwnck changes within versions.
 *                 We only need to use #ifdefs in window tracker object
 *                 and nowhere else in the code.
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

#ifndef __XFDASHBOARD_WINDOW_TRACKER__
#define __XFDASHBOARD_WINDOW_TRACKER__

#include <glib-object.h>

#include "window-tracker-window.h"
#include "window-tracker-workspace.h"
#include "window-tracker-monitor.h"

G_BEGIN_DECLS

#define XFDASHBOARD_TYPE_WINDOW_TRACKER				(xfdashboard_window_tracker_get_type())
#define XFDASHBOARD_WINDOW_TRACKER(obj)				(G_TYPE_CHECK_INSTANCE_CAST((obj), XFDASHBOARD_TYPE_WINDOW_TRACKER, XfdashboardWindowTracker))
#define XFDASHBOARD_IS_WINDOW_TRACKER(obj)			(G_TYPE_CHECK_INSTANCE_TYPE((obj), XFDASHBOARD_TYPE_WINDOW_TRACKER))
#define XFDASHBOARD_WINDOW_TRACKER_CLASS(klass)		(G_TYPE_CHECK_CLASS_CAST((klass), XFDASHBOARD_TYPE_WINDOW_TRACKER, XfdashboardWindowTrackerClass))
#define XFDASHBOARD_IS_WINDOW_TRACKER_CLASS(klass)	(G_TYPE_CHECK_CLASS_TYPE((klass), XFDASHBOARD_TYPE_WINDOW_TRACKER))
#define XFDASHBOARD_WINDOW_TRACKER_GET_CLASS(obj)	(G_TYPE_INSTANCE_GET_CLASS((obj), XFDASHBOARD_TYPE_WINDOW_TRACKER, XfdashboardWindowTrackerClass))

typedef struct _XfdashboardWindowTracker			XfdashboardWindowTracker;
typedef struct _XfdashboardWindowTrackerClass		XfdashboardWindowTrackerClass;
typedef struct _XfdashboardWindowTrackerPrivate		XfdashboardWindowTrackerPrivate;

struct _XfdashboardWindowTracker
{
	/* Parent instance */
	GObject								parent_instance;

	/* Private structure */
	XfdashboardWindowTrackerPrivate		*priv;
};

struct _XfdashboardWindowTrackerClass
{
	/*< private >*/
	/* Parent class */
	GObjectClass						parent_class;

	/*< public >*/
	/* Virtual functions */
	void (*window_stacking_changed)(XfdashboardWindowTracker *self);

	void (*active_window_changed)(XfdashboardWindowTracker *self,
									XfdashboardWindowTrackerWindow *inOldWindow,
									XfdashboardWindowTrackerWindow *inNewWindow);
	void (*window_opened)(XfdashboardWindowTracker *self, XfdashboardWindowTrackerWindow *inWindow);
	void (*window_closed)(XfdashboardWindowTracker *self, XfdashboardWindowTrackerWindow *inWindow);
	void (*window_geometry_changed)(XfdashboardWindowTracker *self,XfdashboardWindowTrackerWindow *inWindow);
	void (*window_actions_changed)(XfdashboardWindowTracker *self, XfdashboardWindowTrackerWindow *inWindow);
	void (*window_state_changed)(XfdashboardWindowTracker *self, XfdashboardWindowTrackerWindow *inWindow);
	void (*window_icon_changed)(XfdashboardWindowTracker *self, XfdashboardWindowTrackerWindow *inWindow);
	void (*window_name_changed)(XfdashboardWindowTracker *self, XfdashboardWindowTrackerWindow *inWindow);
	void (*window_workspace_changed)(XfdashboardWindowTracker *self,
										XfdashboardWindowTrackerWindow *inWindow,
										XfdashboardWindowTrackerWorkspace *inWorkspace);
	void (*window_monitor_changed)(XfdashboardWindowTracker *self,
										XfdashboardWindowTrackerWindow *inWindow,
										XfdashboardWindowTrackerMonitor *inOldMonitor,
										XfdashboardWindowTrackerMonitor *inNewMonitor);

	void (*active_workspace_changed)(XfdashboardWindowTracker *self,
										XfdashboardWindowTrackerWorkspace *inOldWorkspace,
										XfdashboardWindowTrackerWorkspace *inNewWorkspace);
	void (*workspace_added)(XfdashboardWindowTracker *self, XfdashboardWindowTrackerWorkspace *inWorkspace);
	void (*workspace_removed)(XfdashboardWindowTracker *self, XfdashboardWindowTrackerWorkspace *inWorkspace);
	void (*workspace_name_changed)(XfdashboardWindowTracker *self, XfdashboardWindowTrackerWorkspace *inWorkspace);

	void (*primary_monitor_changed)(XfdashboardWindowTracker *self,
									XfdashboardWindowTrackerMonitor *inOldMonitor,
									XfdashboardWindowTrackerMonitor *inNewMonitor);
	void (*monitor_added)(XfdashboardWindowTracker *self, XfdashboardWindowTrackerMonitor *inMonitor);
	void (*monitor_removed)(XfdashboardWindowTracker *self, XfdashboardWindowTrackerMonitor *inMonitor);
	void (*monitor_geometry_changed)(XfdashboardWindowTracker *self, XfdashboardWindowTrackerMonitor *inMonitor);
	void (*screen_size_changed)(XfdashboardWindowTracker *self, gint inWidth, gint inHeight);
};

/* Public API */
GType xfdashboard_window_tracker_get_type(void) G_GNUC_CONST;

XfdashboardWindowTracker* xfdashboard_window_tracker_get_default(void);

guint32 xfdashboard_window_tracker_get_time(void);

GList* xfdashboard_window_tracker_get_windows(XfdashboardWindowTracker *self);
GList* xfdashboard_window_tracker_get_windows_stacked(XfdashboardWindowTracker *self);
XfdashboardWindowTrackerWindow* xfdashboard_window_tracker_get_active_window(XfdashboardWindowTracker *self);

gint xfdashboard_window_tracker_get_workspaces_count(XfdashboardWindowTracker *self);
GList* xfdashboard_window_tracker_get_workspaces(XfdashboardWindowTracker *self);
XfdashboardWindowTrackerWorkspace* xfdashboard_window_tracker_get_workspace_by_number(XfdashboardWindowTracker *self,
																						gint inNumber);
XfdashboardWindowTrackerWorkspace* xfdashboard_window_tracker_get_active_workspace(XfdashboardWindowTracker *self);

gboolean xfdashboard_window_tracker_supports_multiple_monitors(XfdashboardWindowTracker *self);
gint xfdashboard_window_tracker_get_monitors_count(XfdashboardWindowTracker *self);
GList* xfdashboard_window_tracker_get_monitors(XfdashboardWindowTracker *self);
XfdashboardWindowTrackerMonitor* xfdashboard_window_tracker_get_monitor_by_number(XfdashboardWindowTracker *self,
																					gint inNumber);
XfdashboardWindowTrackerMonitor* xfdashboard_window_tracker_get_primary_monitor(XfdashboardWindowTracker *self);

gint xfdashboard_window_tracker_get_screen_width(XfdashboardWindowTracker *self);
gint xfdashboard_window_tracker_get_screen_height(XfdashboardWindowTracker *self);

XfdashboardWindowTrackerWindow* xfdashboard_window_tracker_get_root_window(XfdashboardWindowTracker *self);

G_END_DECLS

#endif	/* __XFDASHBOARD_WINDOW_TRACKER__ */
