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

#include <libxfdashboard/x11/window-tracker-x11.h>
#include <libxfdashboard/marshal.h>
#include <libxfdashboard/compat.h>


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

	SIGNAL_LAST
};

static guint XfdashboardWindowTrackerSignals[SIGNAL_LAST]={ 0, };

/* IMPLEMENTATION: Private variables and methods */
#define XFDASHBOARD_WINDOWS_TRACKER_WARN_NOT_IMPLEMENTED(self, vfunc)  \
	g_warning(_("Object of type %s does not implement required virtual function XfdashboardWindowTracker::%s"),\
				G_OBJECT_TYPE_NAME(self), \
				vfunc);

static XfdashboardWindowTracker *_xfdashboard_window_tracker_singleton=NULL;


/* IMPLEMENTATION: GObject */

/* Interface initialization
 * Set up default functions
 */
void xfdashboard_window_tracker_default_init(XfdashboardWindowTrackerInterface *iface)
{
	static gboolean		initialized=FALSE;
	GParamSpec			*property;

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

		XfdashboardWindowTrackerSignals[SIGNAL_SCREEN_SIZE_CHANGED]=
			g_signal_new("screen-size-changed",
							G_TYPE_FROM_INTERFACE(iface),
							G_SIGNAL_RUN_LAST,
							G_STRUCT_OFFSET(XfdashboardWindowTrackerInterface, screen_size_changed),
							NULL,
							NULL,
							_xfdashboard_marshal_VOID__INT_INT,
							G_TYPE_NONE,
							2,
							G_TYPE_INT,
							G_TYPE_INT);

		/* Set flag that base initialization was done for this interface */
		initialized=TRUE;
	}
}


/* IMPLEMENTATION: Public API */

/* Create new instance */
XfdashboardWindowTracker* xfdashboard_window_tracker_get_default(void)
{
	if(G_UNLIKELY(_xfdashboard_window_tracker_singleton==NULL))
	{
		_xfdashboard_window_tracker_singleton=
			XFDASHBOARD_WINDOW_TRACKER(g_object_new(XFDASHBOARD_TYPE_WINDOW_TRACKER_X11, NULL));
	}
		else g_object_ref(_xfdashboard_window_tracker_singleton);

	return(_xfdashboard_window_tracker_singleton);

}

/* Get list of all windows (if wanted in stack order) */
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

/* Get active window */
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

/* Get number of workspaces */
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

/* Get list of workspaces */
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

/* Get active workspace */
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

/* Get workspace by number */
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

/* Determine if multiple monitors are supported */
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

/* Get number of monitors */
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

/* Get list of monitors */
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

/* Get primary monitor */
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

/* Get monitor by number */
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

/* Get monitor at position */
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

/* Get width and height of screen */
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

/* Get root (desktop) window */
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

/* Get window of stage*/
XfdashboardWindowTrackerWindow* xfdashboard_window_tracker_get_stage_window(XfdashboardWindowTracker *self,
																			ClutterStage *inStage)
{
	XfdashboardWindowTrackerInterface		*iface;

	g_return_val_if_fail(XFDASHBOARD_IS_WINDOW_TRACKER(self), NULL);
	g_return_val_if_fail(CLUTTER_IS_STAGE(inStage), NULL);

	iface=XFDASHBOARD_WINDOW_TRACKER_GET_IFACE(self);

	/* Call virtual function */
	if(iface->get_stage_window)
	{
		return(iface->get_stage_window(self, inStage));
	}

	/* If we get here the virtual function was not overridden */
	XFDASHBOARD_WINDOWS_TRACKER_WARN_NOT_IMPLEMENTED(self, "get_stage_window");
	return(NULL);
}
