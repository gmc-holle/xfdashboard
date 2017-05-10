/*
 * window-tracker: Tracks windows, workspaces, monitors and
 *                 listens for changes.
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

#ifndef __LIBXFDASHBOARD_WINDOW_TRACKER__
#define __LIBXFDASHBOARD_WINDOW_TRACKER__

#if !defined(__LIBXFDASHBOARD_H_INSIDE__) && !defined(LIBXFDASHBOARD_COMPILATION)
#error "Only <libxfdashboard/libxfdashboard.h> can be included directly."
#endif

#include <glib-object.h>

#include <libxfdashboard/window-tracker-window.h>
#include <libxfdashboard/window-tracker-workspace.h>
#include <libxfdashboard/window-tracker-monitor.h>

G_BEGIN_DECLS

#define XFDASHBOARD_TYPE_WINDOW_TRACKER				(xfdashboard_window_tracker_get_type())
#define XFDASHBOARD_WINDOW_TRACKER(obj)				(G_TYPE_CHECK_INSTANCE_CAST((obj), XFDASHBOARD_TYPE_WINDOW_TRACKER, XfdashboardWindowTracker))
#define XFDASHBOARD_IS_WINDOW_TRACKER(obj)			(G_TYPE_CHECK_INSTANCE_TYPE((obj), XFDASHBOARD_TYPE_WINDOW_TRACKER))
#define XFDASHBOARD_WINDOW_TRACKER_GET_IFACE(obj)	(G_TYPE_INSTANCE_GET_INTERFACE((obj), XFDASHBOARD_TYPE_WINDOW_TRACKER, XfdashboardWindowTrackerInterface))

typedef struct _XfdashboardWindowTracker			XfdashboardWindowTracker;
typedef struct _XfdashboardWindowTrackerInterface	XfdashboardWindowTrackerInterface;

struct _XfdashboardWindowTrackerInterface
{
	/*< private >*/
	/* Parent interface */
	GTypeInterface						parent_interface;

	/*< public >*/
	/* Virtual functions */
	GList* (*get_windows)(XfdashboardWindowTracker *self);
	GList* (*get_windows_stacked)(XfdashboardWindowTracker *self);
	XfdashboardWindowTrackerWindow* (*get_active_window)(XfdashboardWindowTracker *self);

	gint (*get_workspaces_count)(XfdashboardWindowTracker *self);
	GList* (*get_workspaces)(XfdashboardWindowTracker *self);
	XfdashboardWindowTrackerWorkspace* (*get_active_workspace)(XfdashboardWindowTracker *self);
	XfdashboardWindowTrackerWorkspace* (*get_workspace_by_number)(XfdashboardWindowTracker *self, gint inNumber);

	gboolean (*supports_multiple_monitors)(XfdashboardWindowTracker *self);
	gint (*get_monitors_count)(XfdashboardWindowTracker *self);
	GList* (*get_monitors)(XfdashboardWindowTracker *self);
	XfdashboardWindowTrackerMonitor* (*get_primary_monitor)(XfdashboardWindowTracker *self);
	XfdashboardWindowTrackerMonitor* (*get_monitor_by_number)(XfdashboardWindowTracker *self, gint inNumber);
	XfdashboardWindowTrackerMonitor* (*get_monitor_by_position)(XfdashboardWindowTracker *self, gint inX, gint inY);

	void (*get_screen_size)(XfdashboardWindowTracker *self, gint *outWidth, gint *outHeight);

	const gchar* (*get_window_manager_name)(XfdashboardWindowTracker *self);

	XfdashboardWindowTrackerWindow* (*get_root_window)(XfdashboardWindowTracker *self);
	XfdashboardWindowTrackerWindow* (*get_stage_window)(XfdashboardWindowTracker *self, ClutterStage *inStage);

	/* Signals */
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

	void (*window_manager_changed)(XfdashboardWindowTracker *self);
};

/* Public API */
GType xfdashboard_window_tracker_get_type(void) G_GNUC_CONST;

XfdashboardWindowTracker* xfdashboard_window_tracker_get_default(void);

GList* xfdashboard_window_tracker_get_windows(XfdashboardWindowTracker *self);
GList* xfdashboard_window_tracker_get_windows_stacked(XfdashboardWindowTracker *self);
XfdashboardWindowTrackerWindow* xfdashboard_window_tracker_get_active_window(XfdashboardWindowTracker *self);

gint xfdashboard_window_tracker_get_workspaces_count(XfdashboardWindowTracker *self);
GList* xfdashboard_window_tracker_get_workspaces(XfdashboardWindowTracker *self);
XfdashboardWindowTrackerWorkspace* xfdashboard_window_tracker_get_active_workspace(XfdashboardWindowTracker *self);
XfdashboardWindowTrackerWorkspace* xfdashboard_window_tracker_get_workspace_by_number(XfdashboardWindowTracker *self,
																						gint inNumber);

gboolean xfdashboard_window_tracker_supports_multiple_monitors(XfdashboardWindowTracker *self);
gint xfdashboard_window_tracker_get_monitors_count(XfdashboardWindowTracker *self);
GList* xfdashboard_window_tracker_get_monitors(XfdashboardWindowTracker *self);
XfdashboardWindowTrackerMonitor* xfdashboard_window_tracker_get_primary_monitor(XfdashboardWindowTracker *self);
XfdashboardWindowTrackerMonitor* xfdashboard_window_tracker_get_monitor_by_number(XfdashboardWindowTracker *self,
																					gint inNumber);
XfdashboardWindowTrackerMonitor* xfdashboard_window_tracker_get_monitor_by_position(XfdashboardWindowTracker *self,
																						gint inX,
																						gint inY);

void xfdashboard_window_tracker_get_screen_size(XfdashboardWindowTracker *self,
													gint *outWidth,
													gint *outHeight);

const gchar* xfdashboard_window_tracker_get_window_manager_name(XfdashboardWindowTracker *self);

XfdashboardWindowTrackerWindow* xfdashboard_window_tracker_get_root_window(XfdashboardWindowTracker *self);
XfdashboardWindowTrackerWindow* xfdashboard_window_tracker_get_stage_window(XfdashboardWindowTracker *self,
																			ClutterStage *inStage);

G_END_DECLS

#endif	/* __LIBXFDASHBOARD_WINDOW_TRACKER__ */
