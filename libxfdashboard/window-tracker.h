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

/* Object declaration */
#define XFDASHBOARD_TYPE_WINDOW_TRACKER				(xfdashboard_window_tracker_get_type())
#define XFDASHBOARD_WINDOW_TRACKER(obj)				(G_TYPE_CHECK_INSTANCE_CAST((obj), XFDASHBOARD_TYPE_WINDOW_TRACKER, XfdashboardWindowTracker))
#define XFDASHBOARD_IS_WINDOW_TRACKER(obj)			(G_TYPE_CHECK_INSTANCE_TYPE((obj), XFDASHBOARD_TYPE_WINDOW_TRACKER))
#define XFDASHBOARD_WINDOW_TRACKER_GET_IFACE(obj)	(G_TYPE_INSTANCE_GET_INTERFACE((obj), XFDASHBOARD_TYPE_WINDOW_TRACKER, XfdashboardWindowTrackerInterface))

typedef struct _XfdashboardWindowTracker			XfdashboardWindowTracker;
typedef struct _XfdashboardWindowTrackerInterface	XfdashboardWindowTrackerInterface;

/**
 * XfdashboardWindowTrackerInterface:
 * @get_windows: Get list of windows tracked
 * @get_windows_stacked: Get list of windows tracked in stacked order
 *    (from bottom to top)
 * @get_active_window: Get the current active window
 * @get_workspaces_count: Get number of workspaces
 * @get_workspaces: Get list of workspaces tracked
 * @get_active_workspace: Get the current active workspace
 * @get_workspace_by_number: Get workspace by its index
 * @supports_multiple_monitors: Whether the window trackers supports and tracks
 *    multiple monitors or not
 * @get_monitors_count: Get number of monitors
 * @get_monitors: Get list of monitors tracked
 * @get_primary_monitor: Get the current primary monitor
 * @get_monitor_by_number: Get monitor by its index
 * @get_monitor_by_position: Get monitor by looking up if it contains the
 *    requested position
 * @get_screen_size: Get total size of screen (size over all connected monitors)
 * @get_window_manager_name: Get name of window manager at desktop environment
 * @get_root_window: Get root window (usually the desktop at background)
 * @window_stacking_changed: Signal emitted when the stacking order of windows
 *    has changed
 * @active_window_changed: Signal emitted when the active window has changed,
 *    e.g. focus moved to another window
 * @window_opened: Signal emitted when a new window was opened
 * @window_closed: Signal emitted when a window was closed
 * @window_geometry_changed: Signal emitted when a window has changed its
 *    position or size or both
 * @window_actions_changed: Signal emitted when the available action of a window
 *    has changed
 * @window_state_changed: Signal emitted when the state of a window has changed
 * @window_icon_changed: Signal emitted when the icon of a window has changed
 * @window_name_changed: Signal emitted when the title of a window has changed
 * @window_workspace_changed: Signal emitted when a window was moved to another
 *    workspace
 * @window_monitor_changed: Signal emitted when a window was moved to another
 *    monitor
 * @active_workspace_changed: Signal emitted when the active workspace has changed
 * @workspace_added: Signal emitted when a new workspace was added
 * @workspace_removed: Signal emitted when a workspace was removed
 * @workspace_name_changed: Signal emitted when the title of a workspace has
 *    changed
 * @primary_monitor_changed: Signal emitted when the primary monitor has changed
 * @monitor_added: Signal emitted when a new monitor was connected
 * @monitor_removed: Signal emitted when a monitor was disconnected or turned off
 * @monitor_geometry_changed: Signal emitted when the resolution of a monitor
 *    has chnaged
 * @screen_size_changed: Signal emitted when the total screen size over all
 *    connected monitors has changed
 * @window_manager_changed: Signal emitted when the window manager was replaced
 */
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

	void (*screen_size_changed)(XfdashboardWindowTracker *self);

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
