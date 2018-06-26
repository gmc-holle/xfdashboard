/*
 * window-tracker: Tracks windows, workspaces, monitors and
 *                 listens for changes. It also bundles libwnck into one
 *                 class.
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

#include <libxfdashboard/window-tracker.h>

#include <glib/gi18n-lib.h>

#include <libxfdashboard/window-tracker-backend.h>
#include <libxfdashboard/marshal.h>
#include <libxfdashboard/compat.h>
#include <libxfdashboard/debug.h>


/* Define this class in GObject system */
G_DEFINE_INTERFACE(XfdashboardWindowTracker,
					xfdashboard_window_tracker,
					G_TYPE_OBJECT)


/* Signals */

enum
{
	SIGNAL_WINDOW_STACKING_CHANGED,

	SIGNAL_ACTIVE_WINDOW_CHANGED,
	SIGNAL_WINDOW_OPENED,
	SIGNAL_WINDOW_CLOSED,
	SIGNAL_WINDOW_GEOMETRY_CHANGED,
	SIGNAL_WINDOW_ACTIONS_CHANGED,
	SIGNAL_WINDOW_STATE_CHANGED,
	SIGNAL_WINDOW_ICON_CHANGED,
	SIGNAL_WINDOW_NAME_CHANGED,
	SIGNAL_WINDOW_WORKSPACE_CHANGED,
	SIGNAL_WINDOW_MONITOR_CHANGED,

	SIGNAL_ACTIVE_WORKSPACE_CHANGED,
	SIGNAL_WORKSPACE_ADDED,
	SIGNAL_WORKSPACE_REMOVED,
	SIGNAL_WORKSPACE_NAME_CHANGED,

	SIGNAL_PRIMARY_MONITOR_CHANGED,
	SIGNAL_MONITOR_ADDED,
	SIGNAL_MONITOR_REMOVED,
	SIGNAL_MONITOR_GEOMETRY_CHANGED,
	SIGNAL_SCREEN_SIZE_CHANGED,

	SIGNAL_WINDOW_MANAGER_CHANGED,

	SIGNAL_LAST
};

static guint XfdashboardWindowTrackerSignals[SIGNAL_LAST]={ 0, };

/* IMPLEMENTATION: Private variables and methods */
#define XFDASHBOARD_WINDOWS_TRACKER_WARN_NOT_IMPLEMENTED(self, vfunc)  \
	g_warning(_("Object of type %s does not implement required virtual function XfdashboardWindowTracker::%s"),\
				G_OBJECT_TYPE_NAME(self), \
				vfunc);

/* Default signal handler for signal "window_closed" */
static void _xfdashboard_window_tracker_real_window_closed(XfdashboardWindowTracker *self,
															XfdashboardWindowTrackerWindow *inWindow)
{
	g_return_if_fail(XFDASHBOARD_IS_WINDOW_TRACKER(self));
	g_return_if_fail(XFDASHBOARD_IS_WINDOW_TRACKER_WINDOW(inWindow));

	/* By default (if not overidden) emit "closed" signal at window */
	g_signal_emit_by_name(inWindow, "closed");
}


/* IMPLEMENTATION: GObject */

/* Interface initialization
 * Set up default functions
 */
void xfdashboard_window_tracker_default_init(XfdashboardWindowTrackerInterface *iface)
{
	static gboolean		initialized=FALSE;
	GParamSpec			*property;

	/* The following virtual functions should be overriden if default
	 * implementation does not fit.
	 */
	iface->window_closed=_xfdashboard_window_tracker_real_window_closed;

	/* Define properties, signals and actions */
	if(!initialized)
	{
		/* Define properties */
		property=g_param_spec_object("active-window",
										_("Active window"),
										_("The current active window"),
										XFDASHBOARD_TYPE_WINDOW_TRACKER_WINDOW,
										G_PARAM_READABLE | G_PARAM_STATIC_STRINGS);
		g_object_interface_install_property(iface, property);

		property=g_param_spec_object("active-workspace",
										_("Active workspace"),
										_("The current active workspace"),
										XFDASHBOARD_TYPE_WINDOW_TRACKER_WORKSPACE,
										G_PARAM_READABLE | G_PARAM_STATIC_STRINGS);
		g_object_interface_install_property(iface, property);

		property=g_param_spec_object("primary-monitor",
										_("Primary monitor"),
										_("The current primary monitor"),
										XFDASHBOARD_TYPE_WINDOW_TRACKER_MONITOR,
										G_PARAM_READABLE | G_PARAM_STATIC_STRINGS);
		g_object_interface_install_property(iface, property);

		/* Define signals */
		/**
		 * XfdashboardWindowTracker::window-tacking-changed:
		 * @self: The window tracker
		 *
		 * The ::window-tacking-changed signal is emitted whenever the stacking
		 * order of the windows at the desktop environment has changed.
		 */
		XfdashboardWindowTrackerSignals[SIGNAL_WINDOW_STACKING_CHANGED]=
			g_signal_new("window-stacking-changed",
							G_TYPE_FROM_INTERFACE(iface),
							G_SIGNAL_RUN_LAST,
							G_STRUCT_OFFSET(XfdashboardWindowTrackerInterface, window_stacking_changed),
							NULL,
							NULL,
							g_cclosure_marshal_VOID__VOID,
							G_TYPE_NONE,
							0);

		/**
		 * XfdashboardWindowTracker::active-window-changed:
		 * @self: The window tracker
		 * @inPreviousActiveWindow: The #XfdashboardWindowTrackerWindow which
		 *    was the active window before this change
		 * @inCurrentActiveWindow: The #XfdashboardWindowTrackerWindow which is
		 *    the active window now
		 *
		 * The ::active-window-changed is emitted when the active window has
		 * changed.
		 */
		XfdashboardWindowTrackerSignals[SIGNAL_ACTIVE_WINDOW_CHANGED]=
			g_signal_new("active-window-changed",
							G_TYPE_FROM_INTERFACE(iface),
							G_SIGNAL_RUN_LAST,
							G_STRUCT_OFFSET(XfdashboardWindowTrackerInterface, active_window_changed),
							NULL,
							NULL,
							_xfdashboard_marshal_VOID__OBJECT_OBJECT,
							G_TYPE_NONE,
							2,
							XFDASHBOARD_TYPE_WINDOW_TRACKER_WINDOW,
							XFDASHBOARD_TYPE_WINDOW_TRACKER_WINDOW);

		/**
		 * XfdashboardWindowTracker::window-opened:
		 * @self: The window tracker
		 * @inWindow: The #XfdashboardWindowTrackerWindow opened
		 *
		 * The ::window-opened signal is emitted whenever a new window was opened
		 * at the desktop environment.
		 */
		XfdashboardWindowTrackerSignals[SIGNAL_WINDOW_OPENED]=
			g_signal_new("window-opened",
							G_TYPE_FROM_INTERFACE(iface),
							G_SIGNAL_RUN_LAST,
							G_STRUCT_OFFSET(XfdashboardWindowTrackerInterface, window_opened),
							NULL,
							NULL,
							g_cclosure_marshal_VOID__OBJECT,
							G_TYPE_NONE,
							1,
							XFDASHBOARD_TYPE_WINDOW_TRACKER_WINDOW);

		/**
		 * XfdashboardWindowTracker::window-closed:
		 * @self: The window tracker
		 * @inWindow: The #XfdashboardWindowTrackerWindow closed
		 *
		 * The ::window-closed signal is emitted when a window was closed and is
		 * not available anymore.
		 */
		XfdashboardWindowTrackerSignals[SIGNAL_WINDOW_CLOSED]=
			g_signal_new("window-closed",
							G_TYPE_FROM_INTERFACE(iface),
							G_SIGNAL_RUN_LAST,
							G_STRUCT_OFFSET(XfdashboardWindowTrackerInterface, window_closed),
							NULL,
							NULL,
							g_cclosure_marshal_VOID__OBJECT,
							G_TYPE_NONE,
							1,
							XFDASHBOARD_TYPE_WINDOW_TRACKER_WINDOW);

		/**
		 * XfdashboardWindowTracker::window-geometry-changed:
		 * @self: The window tracker
		 * @inWindow: The #XfdashboardWindowTrackerWindow which changed its size or position
		 *
		 * The ::window-geometry-changed signal is emitted when the size of a
		 * window or its position at screen of the desktop environment has changed.
		 */
		XfdashboardWindowTrackerSignals[SIGNAL_WINDOW_GEOMETRY_CHANGED]=
			g_signal_new("window-geometry-changed",
							G_TYPE_FROM_INTERFACE(iface),
							G_SIGNAL_RUN_LAST,
							G_STRUCT_OFFSET(XfdashboardWindowTrackerInterface, window_geometry_changed),
							NULL,
							NULL,
							g_cclosure_marshal_VOID__OBJECT,
							G_TYPE_NONE,
							1,
							XFDASHBOARD_TYPE_WINDOW_TRACKER_WINDOW);

		/**
		 * XfdashboardWindowTracker::window-actions-changed:
		 * @self: The window tracker
		 * @inWindow: The #XfdashboardWindowTrackerWindow changed the availability
		 *    of actions
		 *
		 * The ::window-actions-changed signal is emitted whenever the availability
		 * of actions of a window changes.
		 */
		XfdashboardWindowTrackerSignals[SIGNAL_WINDOW_ACTIONS_CHANGED]=
			g_signal_new("window-actions-changed",
							G_TYPE_FROM_INTERFACE(iface),
							G_SIGNAL_RUN_LAST,
							G_STRUCT_OFFSET(XfdashboardWindowTrackerInterface, window_actions_changed),
							NULL,
							NULL,
							g_cclosure_marshal_VOID__OBJECT,
							G_TYPE_NONE,
							1,
							XFDASHBOARD_TYPE_WINDOW_TRACKER_WINDOW);

		/**
		 * XfdashboardWindowTracker::window-state-changed:
		 * @self: The window tracker
		 * @inWindow: The #XfdashboardWindowTrackerWindow changed its state
		 *
		 * The ::window-state-changed signal is emitted whenever a window
		 * changes its state. This can happen when @inWindow is (un)minimized,
		 * (un)maximized, (un)pinned, (un)set fullscreen etc. See
		 * #XfdashboardWindowTrackerWindowState for the complete list of states
		 * that might have changed.
		 */
		XfdashboardWindowTrackerSignals[SIGNAL_WINDOW_STATE_CHANGED]=
			g_signal_new("window-state-changed",
							G_TYPE_FROM_INTERFACE(iface),
							G_SIGNAL_RUN_LAST,
							G_STRUCT_OFFSET(XfdashboardWindowTrackerInterface, window_state_changed),
							NULL,
							NULL,
							g_cclosure_marshal_VOID__OBJECT,
							G_TYPE_NONE,
							1,
							XFDASHBOARD_TYPE_WINDOW_TRACKER_WINDOW);

		/**
		 * XfdashboardWindowTracker::window-icon-changed:
		 * @self: The window tracker
		 * @inWindow: The #XfdashboardWindowTrackerWindow changed its icon
		 *
		 * The ::window-icon-changed signal is emitted whenever a window
		 * changes its icon
		 */
		XfdashboardWindowTrackerSignals[SIGNAL_WINDOW_ICON_CHANGED]=
			g_signal_new("window-icon-changed",
							G_TYPE_FROM_INTERFACE(iface),
							G_SIGNAL_RUN_LAST,
							G_STRUCT_OFFSET(XfdashboardWindowTrackerInterface, window_icon_changed),
							NULL,
							NULL,
							g_cclosure_marshal_VOID__OBJECT,
							G_TYPE_NONE,
							1,
							XFDASHBOARD_TYPE_WINDOW_TRACKER_WINDOW);

		/**
		 * XfdashboardWindowTracker::window-name-changed:
		 * @self: The window tracker
		 * @inWindow: The #XfdashboardWindowTrackerWindow changed its name
		 *
		 * The ::window-name-changed signal is emitted whenever a window
		 * changes its name, known as window title.
		 */
		XfdashboardWindowTrackerSignals[SIGNAL_WINDOW_NAME_CHANGED]=
			g_signal_new("window-name-changed",
							G_TYPE_FROM_INTERFACE(iface),
							G_SIGNAL_RUN_LAST,
							G_STRUCT_OFFSET(XfdashboardWindowTrackerInterface, window_name_changed),
							NULL,
							NULL,
							g_cclosure_marshal_VOID__OBJECT,
							G_TYPE_NONE,
							1,
							XFDASHBOARD_TYPE_WINDOW_TRACKER_WINDOW);

		/**
		 * XfdashboardWindowTracker::window-workspace-changed:
		 * @self: The window tracker
		 * @inWindow: The #XfdashboardWindowTrackerWindow moved to another workspace
		 * @inWorkspace: The #XfdashboardWindowTrackerWorkspace where the window
		 *    @inWindow was moved to
		 *
		 * The ::window-workspace-changed signal is emitted whenever a window
		 * moves to another workspace.
		 */
		XfdashboardWindowTrackerSignals[SIGNAL_WINDOW_WORKSPACE_CHANGED]=
			g_signal_new("window-workspace-changed",
							G_TYPE_FROM_INTERFACE(iface),
							G_SIGNAL_RUN_LAST,
							G_STRUCT_OFFSET(XfdashboardWindowTrackerInterface, window_workspace_changed),
							NULL,
							NULL,
							_xfdashboard_marshal_VOID__OBJECT_OBJECT,
							G_TYPE_NONE,
							2,
							XFDASHBOARD_TYPE_WINDOW_TRACKER_WINDOW,
							XFDASHBOARD_TYPE_WINDOW_TRACKER_WORKSPACE);

		/**
		 * XfdashboardWindowTracker::window-monitor-changed:
		 * @self: The window tracker
		 * @inWindow: The #XfdashboardWindowTrackerWindow moved to another monitor
		 * @inPreviousMonitor: The #XfdashboardWindowTrackerMonitor where the window
		 *    @inWindow was located at before this change
		 * @inCurrentMonitor: The #XfdashboardWindowTrackerMonitor where the window
		 *    @inWindow is located at now
		 *
		 * The ::window-monitor-changed signal is emitted whenever a window
		 * moves to another monitor.
		 */
		XfdashboardWindowTrackerSignals[SIGNAL_WINDOW_MONITOR_CHANGED]=
			g_signal_new("window-monitor-changed",
							G_TYPE_FROM_INTERFACE(iface),
							G_SIGNAL_RUN_LAST,
							G_STRUCT_OFFSET(XfdashboardWindowTrackerInterface, window_monitor_changed),
							NULL,
							NULL,
							_xfdashboard_marshal_VOID__OBJECT_OBJECT_OBJECT,
							G_TYPE_NONE,
							3,
							XFDASHBOARD_TYPE_WINDOW_TRACKER_WINDOW,
							XFDASHBOARD_TYPE_WINDOW_TRACKER_MONITOR,
							XFDASHBOARD_TYPE_WINDOW_TRACKER_MONITOR);

		/**
		 * XfdashboardWindowTracker::active-workspace-changed:
		 * @self: The window tracker
		 * @inPreviousActiveWorkspace: The #XfdashboardWindowTrackerWorkspace which
		 *    was the active workspace before this change
		 * @inCurrentActiveWorkspace: The #XfdashboardWindowTrackerWorkspace which
		 *    is the active workspace now
		 *
		 * The ::active-workspace-changed signal is emitted when the active workspace
		 * has changed.
		 */
		XfdashboardWindowTrackerSignals[SIGNAL_ACTIVE_WORKSPACE_CHANGED]=
			g_signal_new("active-workspace-changed",
							G_TYPE_FROM_INTERFACE(iface),
							G_SIGNAL_RUN_LAST,
							G_STRUCT_OFFSET(XfdashboardWindowTrackerInterface, active_workspace_changed),
							NULL,
							NULL,
							_xfdashboard_marshal_VOID__OBJECT_OBJECT,
							G_TYPE_NONE,
							2,
							XFDASHBOARD_TYPE_WINDOW_TRACKER_WORKSPACE,
							XFDASHBOARD_TYPE_WINDOW_TRACKER_WORKSPACE);

		/**
		 * XfdashboardWindowTracker::workspace-added:
		 * @self: The window tracker
		 * @inWorkspace: The #XfdashboardWindowTrackerWorkspace added
		 *
		 * The ::workspace-added signal is emitted whenever a new workspace was added.
		 */
		XfdashboardWindowTrackerSignals[SIGNAL_WORKSPACE_ADDED]=
			g_signal_new("workspace-added",
							G_TYPE_FROM_INTERFACE(iface),
							G_SIGNAL_RUN_LAST,
							G_STRUCT_OFFSET(XfdashboardWindowTrackerInterface, workspace_added),
							NULL,
							NULL,
							g_cclosure_marshal_VOID__OBJECT,
							G_TYPE_NONE,
							1,
							XFDASHBOARD_TYPE_WINDOW_TRACKER_WORKSPACE);

		/**
		 * XfdashboardWindowTracker::workspace-removed:
		 * @self: The window tracker
		 * @inWorkspace: The #XfdashboardWindowTrackerWorkspace removed
		 *
		 * The ::workspace-removed signal is emitted whenever a workspace was removed.
		 */
		XfdashboardWindowTrackerSignals[SIGNAL_WORKSPACE_REMOVED]=
			g_signal_new("workspace-removed",
							G_TYPE_FROM_INTERFACE(iface),
							G_SIGNAL_RUN_LAST,
							G_STRUCT_OFFSET(XfdashboardWindowTrackerInterface, workspace_removed),
							NULL,
							NULL,
							g_cclosure_marshal_VOID__OBJECT,
							G_TYPE_NONE,
							1,
							XFDASHBOARD_TYPE_WINDOW_TRACKER_WORKSPACE);

		/**
		 * XfdashboardWindowTracker::workspace-name-changed:
		 * @self: The window tracker
		 * @inWorkspace: The #XfdashboardWindowTrackerWorkspace changed its name
		 *
		 * The ::workspace-name-changed signal is emitted whenever a workspace
		 * changes its name.
		 */
		XfdashboardWindowTrackerSignals[SIGNAL_WORKSPACE_NAME_CHANGED]=
			g_signal_new("workspace-name-changed",
							G_TYPE_FROM_INTERFACE(iface),
							G_SIGNAL_RUN_LAST,
							G_STRUCT_OFFSET(XfdashboardWindowTrackerInterface, workspace_name_changed),
							NULL,
							NULL,
							g_cclosure_marshal_VOID__OBJECT,
							G_TYPE_NONE,
							1,
							XFDASHBOARD_TYPE_WINDOW_TRACKER_WORKSPACE);

		/**
		 * XfdashboardWindowTracker::primary-monitor-changed:
		 * @self: The window tracker
		 * @inPreviousPrimaryMonitor: The #XfdashboardWindowTrackerMonitor which
		 *    was the primary monitor before this change
		 * @inCurrentPrimaryMonitor: The #XfdashboardWindowTrackerMonitor which
		 *    is the new primary monitor now
		 *
		 * The ::primary-monitor-changed signal is emitted when another monitor
		 * was configured to be the primary monitor.
		 */
		XfdashboardWindowTrackerSignals[SIGNAL_PRIMARY_MONITOR_CHANGED]=
			g_signal_new("primary-monitor-changed",
							G_TYPE_FROM_INTERFACE(iface),
							G_SIGNAL_RUN_LAST,
							G_STRUCT_OFFSET(XfdashboardWindowTrackerInterface, primary_monitor_changed),
							NULL,
							NULL,
							_xfdashboard_marshal_VOID__OBJECT_OBJECT,
							G_TYPE_NONE,
							2,
							XFDASHBOARD_TYPE_WINDOW_TRACKER_MONITOR,
							XFDASHBOARD_TYPE_WINDOW_TRACKER_MONITOR);

		/**
		 * XfdashboardWindowTracker::monitor-added:
		 * @self: The window tracker
		 * @inMonitor: The #XfdashboardWindowTrackerMonitor added
		 *
		 * The ::monitor-added signal is emitted whenever a new monitor was added.
		 */
		XfdashboardWindowTrackerSignals[SIGNAL_MONITOR_ADDED]=
			g_signal_new("monitor-added",
							G_TYPE_FROM_INTERFACE(iface),
							G_SIGNAL_RUN_LAST,
							G_STRUCT_OFFSET(XfdashboardWindowTrackerInterface, monitor_added),
							NULL,
							NULL,
							g_cclosure_marshal_VOID__OBJECT,
							G_TYPE_NONE,
							1,
							XFDASHBOARD_TYPE_WINDOW_TRACKER_MONITOR);

		/**
		 * XfdashboardWindowTracker::monitor-removed:
		 * @self: The window tracker
		 * @inMonitor: The #XfdashboardWindowTrackerMonitor removed
		 *
		 * The ::monitor-removed signal is emitted whenever a monitor was removed.
		 */
		XfdashboardWindowTrackerSignals[SIGNAL_MONITOR_REMOVED]=
			g_signal_new("monitor-removed",
							G_TYPE_FROM_INTERFACE(iface),
							G_SIGNAL_RUN_LAST,
							G_STRUCT_OFFSET(XfdashboardWindowTrackerInterface, monitor_removed),
							NULL,
							NULL,
							g_cclosure_marshal_VOID__OBJECT,
							G_TYPE_NONE,
							1,
							XFDASHBOARD_TYPE_WINDOW_TRACKER_MONITOR);

		/**
		 * XfdashboardWindowTracker::monitor-geometry-changed:
		 * @self: The window tracker
		 * @inMonitor: The #XfdashboardWindowTrackerMonitor which changed its size or position
		 *
		 * The ::monitor-geometry-changed signal is emitted when the size of a
		 * monitor or its position at screen of the desktop environment has changed.
		 */
		XfdashboardWindowTrackerSignals[SIGNAL_MONITOR_GEOMETRY_CHANGED]=
			g_signal_new("monitor-geometry-changed",
							G_TYPE_FROM_INTERFACE(iface),
							G_SIGNAL_RUN_LAST,
							G_STRUCT_OFFSET(XfdashboardWindowTrackerInterface, monitor_geometry_changed),
							NULL,
							NULL,
							g_cclosure_marshal_VOID__OBJECT,
							G_TYPE_NONE,
							1,
							XFDASHBOARD_TYPE_WINDOW_TRACKER_MONITOR);

		/**
		 * XfdashboardWindowTracker::screen-size-changed:
		 * @self: The window tracker
		 *
		 * The ::screen-size-changed signal is emitted when the screen size of
		 * the desktop environment has been changed, e.g. one monitor changed its
		 * resolution.
		 */
		XfdashboardWindowTrackerSignals[SIGNAL_SCREEN_SIZE_CHANGED]=
			g_signal_new("screen-size-changed",
							G_TYPE_FROM_INTERFACE(iface),
							G_SIGNAL_RUN_LAST,
							G_STRUCT_OFFSET(XfdashboardWindowTrackerInterface, screen_size_changed),
							NULL,
							NULL,
							g_cclosure_marshal_VOID__VOID,
							G_TYPE_NONE,
							0);

		/**
		 * XfdashboardWindowTracker::window-manager-changed:
		 * @self: The window tracker
		 *
		 * The ::window-manager-changed signal is emitted when the window manager
		 * of the desktop environment has been replaced with a new one.
		 */
		XfdashboardWindowTrackerSignals[SIGNAL_WINDOW_MANAGER_CHANGED]=
			g_signal_new("window-manager-changed",
							G_TYPE_FROM_INTERFACE(iface),
							G_SIGNAL_RUN_LAST,
							G_STRUCT_OFFSET(XfdashboardWindowTrackerInterface, window_manager_changed),
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

/**
 * xfdashboard_window_tracker_get_default:
 *
 * Retrieves the singleton instance of #XfdashboardWindowTracker. If not needed
 * anymore the caller must unreference the returned object instance.
 *
 * This function is the logical equivalent of:
 *
 * |[<!-- language="C" -->
 *   XfdashboardWindowTrackerBackend *backend;
 *   XfdashboardWindowTracker        *tracker;
 *
 *   backend=xfdashboard_window_tracker_backend_get_default();
 *   tracker=xfdashboard_window_tracker_backend_get_window_tracker(backend);
 *   g_object_unref(backend);
 * ]|
 *
 * Return value: (transfer full): The instance of #XfdashboardWindowTracker.
 */
XfdashboardWindowTracker* xfdashboard_window_tracker_get_default(void)
{
	XfdashboardWindowTrackerBackend		*backend;
	XfdashboardWindowTracker			*windowTracker;

	/* Get default window tracker backend */
	backend=xfdashboard_window_tracker_backend_get_default();
	if(!backend)
	{
		g_critical(_("Could not get default window tracker backend"));
		return(NULL);
	}

	/* Get window tracker object instance of backend */
	windowTracker=xfdashboard_window_tracker_backend_get_window_tracker(backend);

	/* Release allocated resources */
	if(backend) g_object_unref(backend);

	/* Return window tracker object instance */
	return(windowTracker);
}

/**
 * xfdashboard_window_tracker_get_windows:
 * @self: A #XfdashboardWindowTracker
 *
 * Retrieves the list of #XfdashboardWindowTrackerWindow tracked by @self.
 * The list is ordered: the first element in the list is the first
 * #XfdashboardWindowTrackerWindow, etc..
 *
 * Return value: (element-type XfdashboardWindowTrackerWindow) (transfer none):
 *   The list of #XfdashboardWindowTrackerWindow. The list should not be modified
 *   nor freed, as it is owned by Xfdashboard.
 */
GList* xfdashboard_window_tracker_get_windows(XfdashboardWindowTracker *self)
{
	XfdashboardWindowTrackerInterface		*iface;

	g_return_val_if_fail(XFDASHBOARD_IS_WINDOW_TRACKER(self), NULL);

	iface=XFDASHBOARD_WINDOW_TRACKER_GET_IFACE(self);

	/* Call virtual function */
	if(iface->get_windows)
	{
		return(iface->get_windows(self));
	}

	/* If we get here the virtual function was not overridden */
	XFDASHBOARD_WINDOWS_TRACKER_WARN_NOT_IMPLEMENTED(self, "get_windows");
	return(NULL);
}

/**
 * xfdashboard_window_tracker_get_windows_stacked:
 * @self: A #XfdashboardWindowTracker
 *
 * Retrieves the list of #XfdashboardWindowTrackerWindow tracked by @self in
 * stacked order from bottom to top. The list is ordered: the first element in
 * the list is the most bottom #XfdashboardWindowTrackerWindow, etc..
 *
 * Return value: (element-type XfdashboardWindowTrackerWindow) (transfer none):
 *   The list of #XfdashboardWindowTrackerWindow in stacked order. The list should
 *   not be modified nor freed, as it is owned by Xfdashboard.
 */
GList* xfdashboard_window_tracker_get_windows_stacked(XfdashboardWindowTracker *self)
{
	XfdashboardWindowTrackerInterface		*iface;

	g_return_val_if_fail(XFDASHBOARD_IS_WINDOW_TRACKER(self), NULL);

	iface=XFDASHBOARD_WINDOW_TRACKER_GET_IFACE(self);

	/* Call virtual function */
	if(iface->get_windows_stacked)
	{
		return(iface->get_windows_stacked(self));
	}

	/* If we get here the virtual function was not overridden */
	XFDASHBOARD_WINDOWS_TRACKER_WARN_NOT_IMPLEMENTED(self, "get_windows_stacked");
	return(NULL);
}

/**
 * xfdashboard_window_tracker_get_active_window:
 * @self: A #XfdashboardWindowTracker
 *
 * Retrieves the currently active #XfdashboardWindowTrackerWindow.
 *
 * Return value: (transfer none): The #XfdashboardWindowTrackerWindow currently
 *   active or %NULL if not determinable. The returned object is owned by Xfdashboard
 *   and it should not be referenced or unreferenced.
 */
XfdashboardWindowTrackerWindow* xfdashboard_window_tracker_get_active_window(XfdashboardWindowTracker *self)
{
	XfdashboardWindowTrackerInterface		*iface;

	g_return_val_if_fail(XFDASHBOARD_IS_WINDOW_TRACKER(self), NULL);

	iface=XFDASHBOARD_WINDOW_TRACKER_GET_IFACE(self);

	/* Call virtual function */
	if(iface->get_active_window)
	{
		return(iface->get_active_window(self));
	}

	/* If we get here the virtual function was not overridden */
	XFDASHBOARD_WINDOWS_TRACKER_WARN_NOT_IMPLEMENTED(self, "get_active_window");
	return(NULL);
}

/**
 * xfdashboard_window_tracker_get_workspaces_count:
 * @self: A #XfdashboardWindowTracker
 *
 * Retrieves the number of #XfdashboardWindowTrackerWorkspace tracked by @self.
 *
 * Return value: The number of #XfdashboardWindowTrackerWorkspace
 */
gint xfdashboard_window_tracker_get_workspaces_count(XfdashboardWindowTracker *self)
{
	XfdashboardWindowTrackerInterface		*iface;

	g_return_val_if_fail(XFDASHBOARD_IS_WINDOW_TRACKER(self), 0);

	iface=XFDASHBOARD_WINDOW_TRACKER_GET_IFACE(self);

	/* Call virtual function */
	if(iface->get_workspaces_count)
	{
		return(iface->get_workspaces_count(self));
	}

	/* If we get here the virtual function was not overridden */
	XFDASHBOARD_WINDOWS_TRACKER_WARN_NOT_IMPLEMENTED(self, "get_workspaces_count");
	return(0);
}

/**
 * xfdashboard_window_tracker_get_workspaces:
 * @self: A #XfdashboardWindowTracker
 *
 * Retrieves the list of #XfdashboardWindowTrackerWorkspace tracked by @self.
 * The list is ordered: the first element in the list is the first
 * #XfdashboardWindowTrackerWorkspace, etc..
 *
 * Return value: (element-type XfdashboardWindowTrackerWorkspace) (transfer none):
 *   The list of #XfdashboardWindowTrackerWorkspace. The list should not be modified
 *   nor freed, as it is owned by Xfdashboard.
 */
GList* xfdashboard_window_tracker_get_workspaces(XfdashboardWindowTracker *self)
{
	XfdashboardWindowTrackerInterface		*iface;

	g_return_val_if_fail(XFDASHBOARD_IS_WINDOW_TRACKER(self), NULL);

	iface=XFDASHBOARD_WINDOW_TRACKER_GET_IFACE(self);

	/* Call virtual function */
	if(iface->get_workspaces)
	{
		return(iface->get_workspaces(self));
	}

	/* If we get here the virtual function was not overridden */
	XFDASHBOARD_WINDOWS_TRACKER_WARN_NOT_IMPLEMENTED(self, "get_workspaces");
	return(NULL);
}

/**
 * xfdashboard_window_tracker_get_active_workspace:
 * @self: A #XfdashboardWindowTracker
 *
 * Retrieves the currently active #XfdashboardWindowTrackerWorkspace.
 *
 * Return value: (transfer none): The #XfdashboardWindowTrackerWorkspace currently
 *   active or %NULL if not determinable. The returned object is owned by Xfdashboard
 *   and it should not be referenced or unreferenced.
 */
XfdashboardWindowTrackerWorkspace* xfdashboard_window_tracker_get_active_workspace(XfdashboardWindowTracker *self)
{
	XfdashboardWindowTrackerInterface		*iface;

	g_return_val_if_fail(XFDASHBOARD_IS_WINDOW_TRACKER(self), NULL);

	iface=XFDASHBOARD_WINDOW_TRACKER_GET_IFACE(self);

	/* Call virtual function */
	if(iface->get_active_workspace)
	{
		return(iface->get_active_workspace(self));
	}

	/* If we get here the virtual function was not overridden */
	XFDASHBOARD_WINDOWS_TRACKER_WARN_NOT_IMPLEMENTED(self, "get_active_workspace");
	return(NULL);
}

/**
 * xfdashboard_window_tracker_get_workspace_by_number:
 * @self: A #XfdashboardWindowTracker
 * @inNumber: The workspace index, starting from 0
 *
 * Retrieves the #XfdashboardWindowTrackerWorkspace at index @inNumber.
 *
 * Return value: (transfer none): The #XfdashboardWindowTrackerWorkspace at the
 *   index or %NULL if no such workspace exists. The returned object is owned by
 *   Xfdashboard and it should not be referenced or unreferenced.
 */
XfdashboardWindowTrackerWorkspace* xfdashboard_window_tracker_get_workspace_by_number(XfdashboardWindowTracker *self,
																						gint inNumber)
{
	XfdashboardWindowTrackerInterface		*iface;

	g_return_val_if_fail(XFDASHBOARD_IS_WINDOW_TRACKER(self), NULL);
	g_return_val_if_fail(inNumber>=0, NULL);
	g_return_val_if_fail(inNumber<xfdashboard_window_tracker_get_workspaces_count(self), NULL);

	iface=XFDASHBOARD_WINDOW_TRACKER_GET_IFACE(self);

	/* Call virtual function */
	if(iface->get_workspace_by_number)
	{
		return(iface->get_workspace_by_number(self, inNumber));
	}

	/* If we get here the virtual function was not overridden */
	XFDASHBOARD_WINDOWS_TRACKER_WARN_NOT_IMPLEMENTED(self, "get_workspace_by_number");
	return(NULL);
}

/**
 * xfdashboard_window_tracker_supports_multiple_monitors:
 * @self: A #XfdashboardWindowTracker
 *
 * Determines if window tracker at @self supports multiple monitors.
 *
 * If multiple monitors are supported, %TRUE will be returned and the number
 * of monitors can be determined by calling xfdashboard_window_tracker_get_monitors_count().
 * Also each monitor can be accessed xfdashboard_window_tracker_get_monitor_by_number()
 * and other monitor related functions.
 *
 * If multiple monitors are not supported or the desktop environment cannot provide
 * this kind of information, %FALSE will be returned.
 *
 * Return value: %TRUE if @self supports multiple monitors, otherwise %FALSE.
 */
gboolean xfdashboard_window_tracker_supports_multiple_monitors(XfdashboardWindowTracker *self)
{
	XfdashboardWindowTrackerInterface		*iface;

	g_return_val_if_fail(XFDASHBOARD_IS_WINDOW_TRACKER(self), FALSE);

	iface=XFDASHBOARD_WINDOW_TRACKER_GET_IFACE(self);

	/* Call virtual function */
	if(iface->supports_multiple_monitors)
	{
		return(iface->supports_multiple_monitors(self));
	}

	/* If we get here the virtual function was not overridden */
	XFDASHBOARD_WINDOWS_TRACKER_WARN_NOT_IMPLEMENTED(self, "supports_multiple_monitors");
	return(FALSE);
}

/**
 * xfdashboard_window_tracker_get_monitors_count:
 * @self: A #XfdashboardWindowTracker
 *
 * Retrieves the number of #XfdashboardWindowTrackerMonitor tracked by @self.
 *
 * Return value: The number of #XfdashboardWindowTrackerMonitor
 */
gint xfdashboard_window_tracker_get_monitors_count(XfdashboardWindowTracker *self)
{
	XfdashboardWindowTrackerInterface		*iface;

	g_return_val_if_fail(XFDASHBOARD_IS_WINDOW_TRACKER(self), 0);

	iface=XFDASHBOARD_WINDOW_TRACKER_GET_IFACE(self);

	/* Call virtual function */
	if(iface->get_monitors_count)
	{
		return(iface->get_monitors_count(self));
	}

	/* If we get here the virtual function was not overridden */
	XFDASHBOARD_WINDOWS_TRACKER_WARN_NOT_IMPLEMENTED(self, "get_monitors_count");
	return(0);
}

/**
 * xfdashboard_window_tracker_get_monitors:
 * @self: A #XfdashboardWindowTracker
 *
 * Retrieves the list of #XfdashboardWindowTrackerMonitor tracked by @self.
 * The list is ordered: the first element in the list is the first #XfdashboardWindowTrackerMonitor,
 * etc..
 *
 * Return value: (element-type XfdashboardWindowTrackerMonitor) (transfer none):
 *   The list of #XfdashboardWindowTrackerMonitor. The list should not be modified
 *   nor freed, as it is owned by Xfdashboard.
 */
GList* xfdashboard_window_tracker_get_monitors(XfdashboardWindowTracker *self)
{
	XfdashboardWindowTrackerInterface		*iface;

	g_return_val_if_fail(XFDASHBOARD_IS_WINDOW_TRACKER(self), NULL);

	iface=XFDASHBOARD_WINDOW_TRACKER_GET_IFACE(self);

	/* Call virtual function */
	if(iface->get_monitors)
	{
		return(iface->get_monitors(self));
	}

	/* If we get here the virtual function was not overridden */
	XFDASHBOARD_WINDOWS_TRACKER_WARN_NOT_IMPLEMENTED(self, "get_monitors");
	return(NULL);
}

/**
 * xfdashboard_window_tracker_get_primary_monitor:
 * @self: A #XfdashboardWindowTracker
 *
 * Retrieves the primary #XfdashboardWindowTrackerMonitor the user configured
 * at its desktop environment.
 *
 * Return value: (transfer none): The #XfdashboardWindowTrackerMonitor configured
 *   as primary or %NULL if no primary monitor exists. The returned object is
 *   owned by Xfdashboard and it should not be referenced or unreferenced.
 */
XfdashboardWindowTrackerMonitor* xfdashboard_window_tracker_get_primary_monitor(XfdashboardWindowTracker *self)
{
	XfdashboardWindowTrackerInterface		*iface;

	g_return_val_if_fail(XFDASHBOARD_IS_WINDOW_TRACKER(self), NULL);

	iface=XFDASHBOARD_WINDOW_TRACKER_GET_IFACE(self);

	/* Call virtual function */
	if(iface->get_primary_monitor)
	{
		return(iface->get_primary_monitor(self));
	}

	/* If we get here the virtual function was not overridden */
	XFDASHBOARD_WINDOWS_TRACKER_WARN_NOT_IMPLEMENTED(self, "get_primary_monitor");
	return(NULL);
}

/**
 * xfdashboard_window_tracker_get_monitor_by_number:
 * @self: A #XfdashboardWindowTracker
 * @inNumber: The monitor index, starting from 0
 *
 * Retrieves the #XfdashboardWindowTrackerMonitor at index @inNumber.
 *
 * Return value: (transfer none): The #XfdashboardWindowTrackerMonitor at the
 *   index or %NULL if no such monitor exists. The returned object is owned by
 *   Xfdashboard and it should not be referenced or unreferenced.
 */
XfdashboardWindowTrackerMonitor* xfdashboard_window_tracker_get_monitor_by_number(XfdashboardWindowTracker *self,
																					gint inNumber)
{
	XfdashboardWindowTrackerInterface		*iface;

	g_return_val_if_fail(XFDASHBOARD_IS_WINDOW_TRACKER(self), NULL);
	g_return_val_if_fail(inNumber>=0, NULL);
	g_return_val_if_fail(inNumber<xfdashboard_window_tracker_get_monitors_count(self), NULL);

	iface=XFDASHBOARD_WINDOW_TRACKER_GET_IFACE(self);

	/* Call virtual function */
	if(iface->get_monitor_by_number)
	{
		return(iface->get_monitor_by_number(self, inNumber));
	}

	/* If we get here the virtual function was not overridden */
	XFDASHBOARD_WINDOWS_TRACKER_WARN_NOT_IMPLEMENTED(self, "get_monitor_by_number");
	return(NULL);
}

/**
 * xfdashboard_window_tracker_get_monitor_by_position:
 * @self: A #XfdashboardWindowTracker
 * @inX: The X coordinate of position at screen
 * @inY: The Y coordinate of position at screen
 *
 * Retrieves the monitor containing the position at @inX,@inY at screen.
 *
 * Return value: (transfer none): The #XfdashboardWindowTrackerMonitor for the
 *   requested position or %NULL if no monitor could be found containing the
 *   position. The returned object is owned by Xfdashboard and it should not be
 *   referenced or unreferenced.
 */
XfdashboardWindowTrackerMonitor* xfdashboard_window_tracker_get_monitor_by_position(XfdashboardWindowTracker *self,
																						gint inX,
																						gint inY)
{
	XfdashboardWindowTrackerInterface		*iface;

	g_return_val_if_fail(XFDASHBOARD_IS_WINDOW_TRACKER(self), NULL);

	iface=XFDASHBOARD_WINDOW_TRACKER_GET_IFACE(self);

	/* Call virtual function */
	if(iface->get_monitor_by_position)
	{
		return(iface->get_monitor_by_position(self, inX, inY));
	}

	/* If we get here the virtual function was not overridden */
	XFDASHBOARD_WINDOWS_TRACKER_WARN_NOT_IMPLEMENTED(self, "get_monitor_by_position");
	return(NULL);
}

/**
 * xfdashboard_window_tracker_get_screen_size:
 * @self: A #XfdashboardWindowTracker
 * @outWidth: (out): Return location for width of screen.
 * @outHeight: (out): Return location for height of screen.
 *
 * Retrieves width and height of screen of the desktop environment. The screen
 * contains all connected monitors so it the total size of the desktop environment.
 */
void xfdashboard_window_tracker_get_screen_size(XfdashboardWindowTracker *self, gint *outWidth, gint *outHeight)
{
	XfdashboardWindowTrackerInterface		*iface;
	gint									width, height;

	g_return_if_fail(XFDASHBOARD_IS_WINDOW_TRACKER(self));

	iface=XFDASHBOARD_WINDOW_TRACKER_GET_IFACE(self);

	/* Call virtual function */
	if(iface->get_screen_size)
	{
		/* Get screen size */
		iface->get_screen_size(self, &width, &height);

		/* Store result where possible */
		if(outWidth) *outWidth=width;
		if(outHeight) *outHeight=height;

		return;
	}

	/* If we get here the virtual function was not overridden */
	XFDASHBOARD_WINDOWS_TRACKER_WARN_NOT_IMPLEMENTED(self, "get_screen_width");
}

/**
 * xfdashboard_window_tracker_get_window_manager_name:
 * @self: A #XfdashboardWindowTracker
 *
 * Retrieves the name of window manager managing the desktop environment, i.e.
 * windows, workspaces etc.
 *
 * Return value: A string with name of the window manager
 */
const gchar* xfdashboard_window_tracker_get_window_manager_name(XfdashboardWindowTracker *self)
{
	XfdashboardWindowTrackerInterface		*iface;

	g_return_val_if_fail(XFDASHBOARD_IS_WINDOW_TRACKER(self), NULL);

	iface=XFDASHBOARD_WINDOW_TRACKER_GET_IFACE(self);

	/* Call virtual function */
	if(iface->get_window_manager_name)
	{
		return(iface->get_window_manager_name(self));
	}

	/* If we get here the virtual function was not overridden */
	XFDASHBOARD_WINDOWS_TRACKER_WARN_NOT_IMPLEMENTED(self, "get_window_manager_name");
	return(NULL);
}

/**
 * xfdashboard_window_tracker_get_root_window:
 * @self: A #XfdashboardWindowTracker
 *
 * Retrieves the root window of the desktop environment. The root window is
 * usually the desktop seen at the background of the desktop environment.
 *
 * Return value: (transfer none): The #XfdashboardWindowTrackerWindow representing
 *   the root window or %NULL if not available. The returned object is owned by
 *   Xfdashboard and it should not be referenced or unreferenced.
 */
XfdashboardWindowTrackerWindow* xfdashboard_window_tracker_get_root_window(XfdashboardWindowTracker *self)
{
	XfdashboardWindowTrackerInterface		*iface;

	g_return_val_if_fail(XFDASHBOARD_IS_WINDOW_TRACKER(self), NULL);

	iface=XFDASHBOARD_WINDOW_TRACKER_GET_IFACE(self);

	/* Call virtual function */
	if(iface->get_root_window)
	{
		return(iface->get_root_window(self));
	}

	/* If we get here the virtual function was not overridden */
	XFDASHBOARD_WINDOWS_TRACKER_WARN_NOT_IMPLEMENTED(self, "get_root_window");
	return(NULL);
}

/**
 * xfdashboard_window_tracker_get_stage_window:
 * @self: A #XfdashboardWindowTracker
 * @inStage: A #ClutterStage
 *
 * Retrieves the window created for the requested stage @inStage.
 *
 * This function is the logical equivalent of:
 *
 * |[<!-- language="C" -->
 *   XfdashboardWindowTrackerBackend *backend;
 *   XfdashboardWindowTrackerWindow  *stageWindow;
 *
 *   backend=xfdashboard_window_tracker_backend_get_default();
 *   stageWindow=xfdashboard_window_tracker_backend_get_window_for_stage(backend, inStage);
 *   g_object_unref(backend);
 * ]|
 *
 * Return value: (transfer none): The #XfdashboardWindowTrackerWindow representing
 *   the window of requested stage or %NULL if not available. The returned object
 *   is owned by Xfdashboard and it should not be referenced or unreferenced.
 */
XfdashboardWindowTrackerWindow* xfdashboard_window_tracker_get_stage_window(XfdashboardWindowTracker *self,
																			ClutterStage *inStage)
{
	XfdashboardWindowTrackerBackend		*backend;
	XfdashboardWindowTrackerWindow		*stageWindow;

	/* Get default window tracker backend */
	backend=xfdashboard_window_tracker_backend_get_default();
	if(!backend)
	{
		g_critical(_("Could not get default window tracker backend"));
		return(NULL);
	}

	/* Get window for requested stage from backend */
	stageWindow=xfdashboard_window_tracker_backend_get_window_for_stage(backend, inStage);

	/* Release allocated resources */
	if(backend) g_object_unref(backend);

	/* Return window object instance */
	return(stageWindow);
}
