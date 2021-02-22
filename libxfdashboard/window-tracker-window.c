/*
 * window-tracker-window: A window tracked by window tracker.
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

#include <libxfdashboard/window-tracker-window.h>

#include <glib/gi18n-lib.h>

#include <libxfdashboard/window-tracker-backend.h>
#include <libxfdashboard/enums.h>
#include <libxfdashboard/marshal.h>
#include <libxfdashboard/compat.h>


/* Define this class in GObject system */
G_DEFINE_INTERFACE(XfdashboardWindowTrackerWindow,
					xfdashboard_window_tracker_window,
					G_TYPE_OBJECT)


/* Signals */
enum
{
	SIGNAL_NAME_CHANGED,
	SIGNAL_STATE_CHANGED,
	SIGNAL_ACTIONS_CHANGED,
	SIGNAL_ICON_CHANGED,
	SIGNAL_WORKSPACE_CHANGED,
	SIGNAL_MONITOR_CHANGED,
	SIGNAL_GEOMETRY_CHANGED,
	SIGNAL_CLOSED,

	SIGNAL_LAST
};

static guint XfdashboardWindowTrackerWindowSignals[SIGNAL_LAST]={ 0, };


/* IMPLEMENTATION: Private variables and methods */
#define XFDASHBOARD_WINDOWS_TRACKER_WINDOW_WARN_NOT_IMPLEMENTED(self, vfunc)\
	g_warning("Object of type %s does not implement required virtual function XfdashboardWindowTrackerWindow::%s",\
				G_OBJECT_TYPE_NAME(self), \
				vfunc);


/* Default implementation of virtual function "is_equal" */
static gboolean _xfdashboard_window_tracker_window_real_is_equal(XfdashboardWindowTrackerWindow *inLeft,
																	XfdashboardWindowTrackerWindow *inRight)
{
	g_return_val_if_fail(XFDASHBOARD_IS_WINDOW_TRACKER_WINDOW(inLeft), FALSE);
	g_return_val_if_fail(XFDASHBOARD_IS_WINDOW_TRACKER_WINDOW(inRight), FALSE);

	/* Check if both are the same window */
	if(inLeft==inRight) return(TRUE);

	/* If we get here then they cannot be considered equal */
	return(FALSE);
}

/* Default implementation of virtual function "get_monitor" */
static XfdashboardWindowTrackerMonitor* _xfdashboard_window_tracker_window_real_get_monitor(XfdashboardWindowTrackerWindow *self)
{
	XfdashboardWindowTracker			*windowTracker;
	GList								*monitors;
	XfdashboardWindowTrackerMonitor		*monitor;
	XfdashboardWindowTrackerMonitor		*foundMonitor;

	g_return_val_if_fail(XFDASHBOARD_IS_WINDOW_TRACKER_WINDOW(self), NULL);

	/* Get window tracker to retrieve list of monitors */
	windowTracker=xfdashboard_window_tracker_get_default();

	/* Get list of monitors */
	monitors=xfdashboard_window_tracker_get_monitors(windowTracker);

	/* Iterate through list of monitors and return monitor where window fits in */
	foundMonitor=NULL;
	for(; monitors && !foundMonitor; monitors=g_list_next(monitors))
	{
		monitor=XFDASHBOARD_WINDOW_TRACKER_MONITOR(monitors->data);
		if(xfdashboard_window_tracker_window_is_on_monitor(self, monitor))
		{
			foundMonitor=monitor;
		}
	}

	/* Release allocated resources */
	g_object_unref(windowTracker);

	/* Return found monitor */
	return(foundMonitor);
}

/* Default implementation of virtual function "is_on_monitor" */
static gboolean _xfdashboard_window_tracker_window_real_is_on_monitor(XfdashboardWindowTrackerWindow *self,
																		XfdashboardWindowTrackerMonitor *inMonitor)
{
	XfdashboardWindowTracker	*windowTracker;
	gint						windowX, windowY, windowWidth, windowHeight;
	gint						monitorX, monitorY, monitorWidth, monitorHeight;
	gint						screenWidth, screenHeight;
	gint						windowMiddleX, windowMiddleY;

	g_return_val_if_fail(XFDASHBOARD_IS_WINDOW_TRACKER_WINDOW(self), FALSE);
	g_return_val_if_fail(XFDASHBOARD_IS_WINDOW_TRACKER_MONITOR(inMonitor), FALSE);

	/* Get window geometry */
	xfdashboard_window_tracker_window_get_geometry(self,
													&windowX,
													&windowY,
													&windowWidth,
													&windowHeight);

	/* Get monitor geometry */
	xfdashboard_window_tracker_monitor_get_geometry(inMonitor,
													&monitorX,
													&monitorY,
													&monitorWidth,
													&monitorHeight);

	/* Get screen size */
	windowTracker=xfdashboard_window_tracker_get_default();
	xfdashboard_window_tracker_get_screen_size(windowTracker, &screenWidth, &screenHeight);
	g_object_unref(windowTracker);

	/* Check if mid-point of window (adjusted to screen size) is within monitor */
	windowMiddleX=windowX+(windowWidth/2);
	if(windowMiddleX>screenWidth) windowMiddleX=screenWidth-1;

	windowMiddleY=windowY+(windowHeight/2);
	if(windowMiddleY>screenHeight) windowMiddleY=screenHeight-1;

	if(windowMiddleX>=monitorX && windowMiddleX<(monitorX+monitorWidth) &&
		windowMiddleY>=monitorY && windowMiddleY<(monitorY+monitorHeight))
	{
		return(TRUE);
	}

	/* If we get here mid-point of window is out of range of requested monitor */
	return(FALSE);
}


/* IMPLEMENTATION: GObject */

/* Interface initialization
 * Set up default functions
 */
static void xfdashboard_window_tracker_window_default_init(XfdashboardWindowTrackerWindowInterface *iface)
{
	static gboolean		initialized=FALSE;
	GParamSpec			*property;

	/* The following virtual functions should be overriden if default
	 * implementation does not fit.
	 */
	iface->is_equal=_xfdashboard_window_tracker_window_real_is_equal;
	iface->get_monitor=_xfdashboard_window_tracker_window_real_get_monitor;
	iface->is_on_monitor=_xfdashboard_window_tracker_window_real_is_on_monitor;

	/* Define properties, signals and actions */
	if(!initialized)
	{
		/* Define properties */
		property=g_param_spec_flags("state",
									"State",
									"The state of window",
									XFDASHBOARD_TYPE_WINDOW_TRACKER_WINDOW_STATE,
									0,
									G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS);
		g_object_interface_install_property(iface, property);

		property=g_param_spec_flags("actions",
									"Actions",
									"The possible actions at window",
									XFDASHBOARD_TYPE_WINDOW_TRACKER_WINDOW_ACTION,
									0,
									G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS);
		g_object_interface_install_property(iface, property);

		/* Define signals */
		XfdashboardWindowTrackerWindowSignals[SIGNAL_NAME_CHANGED]=
			g_signal_new("name-changed",
							G_TYPE_FROM_INTERFACE(iface),
							G_SIGNAL_RUN_LAST,
							G_STRUCT_OFFSET(XfdashboardWindowTrackerWindowInterface, name_changed),
							NULL,
							NULL,
							g_cclosure_marshal_VOID__VOID,
							G_TYPE_NONE,
							0);

		XfdashboardWindowTrackerWindowSignals[SIGNAL_STATE_CHANGED]=
			g_signal_new("state-changed",
							G_TYPE_FROM_INTERFACE(iface),
							G_SIGNAL_RUN_LAST,
							G_STRUCT_OFFSET(XfdashboardWindowTrackerWindowInterface, state_changed),
							NULL,
							NULL,
							g_cclosure_marshal_VOID__FLAGS,
							G_TYPE_NONE,
							1,
							XFDASHBOARD_TYPE_WINDOW_TRACKER_WINDOW_STATE);

		XfdashboardWindowTrackerWindowSignals[SIGNAL_ACTIONS_CHANGED]=
			g_signal_new("actions-changed",
							G_TYPE_FROM_INTERFACE(iface),
							G_SIGNAL_RUN_LAST,
							G_STRUCT_OFFSET(XfdashboardWindowTrackerWindowInterface, actions_changed),
							NULL,
							NULL,
							g_cclosure_marshal_VOID__FLAGS,
							G_TYPE_NONE,
							1,
							XFDASHBOARD_TYPE_WINDOW_TRACKER_WINDOW_ACTION);

		XfdashboardWindowTrackerWindowSignals[SIGNAL_ICON_CHANGED]=
			g_signal_new("icon-changed",
							G_TYPE_FROM_INTERFACE(iface),
							G_SIGNAL_RUN_LAST,
							G_STRUCT_OFFSET(XfdashboardWindowTrackerWindowInterface, icon_changed),
							NULL,
							NULL,
							g_cclosure_marshal_VOID__VOID,
							G_TYPE_NONE,
							0);

		XfdashboardWindowTrackerWindowSignals[SIGNAL_WORKSPACE_CHANGED]=
			g_signal_new("workspace-changed",
							G_TYPE_FROM_INTERFACE(iface),
							G_SIGNAL_RUN_LAST,
							G_STRUCT_OFFSET(XfdashboardWindowTrackerWindowInterface, workspace_changed),
							NULL,
							NULL,
							g_cclosure_marshal_VOID__OBJECT,
							G_TYPE_NONE,
							1,
							XFDASHBOARD_TYPE_WINDOW_TRACKER_WORKSPACE);

		XfdashboardWindowTrackerWindowSignals[SIGNAL_MONITOR_CHANGED]=
			g_signal_new("monitor-changed",
							G_TYPE_FROM_INTERFACE(iface),
							G_SIGNAL_RUN_LAST,
							G_STRUCT_OFFSET(XfdashboardWindowTrackerWindowInterface, monitor_changed),
							NULL,
							NULL,
							g_cclosure_marshal_VOID__OBJECT,
							G_TYPE_NONE,
							1,
							XFDASHBOARD_TYPE_WINDOW_TRACKER_MONITOR);

		XfdashboardWindowTrackerWindowSignals[SIGNAL_GEOMETRY_CHANGED]=
			g_signal_new("geometry-changed",
							G_TYPE_FROM_INTERFACE(iface),
							G_SIGNAL_RUN_LAST,
							G_STRUCT_OFFSET(XfdashboardWindowTrackerWindowInterface, geometry_changed),
							NULL,
							NULL,
							g_cclosure_marshal_VOID__VOID,
							G_TYPE_NONE,
							0);

		XfdashboardWindowTrackerWindowSignals[SIGNAL_CLOSED]=
			g_signal_new("closed",
							G_TYPE_FROM_INTERFACE(iface),
							G_SIGNAL_RUN_LAST,
							G_STRUCT_OFFSET(XfdashboardWindowTrackerWindowInterface, closed),
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

/* Check if both windows are the same */
gboolean xfdashboard_window_tracker_window_is_equal(XfdashboardWindowTrackerWindow *inLeft,
													XfdashboardWindowTrackerWindow *inRight)
{
	XfdashboardWindowTrackerWindowInterface		*iface;

	g_return_val_if_fail(XFDASHBOARD_IS_WINDOW_TRACKER_WINDOW(inLeft), FALSE);
	g_return_val_if_fail(XFDASHBOARD_IS_WINDOW_TRACKER_WINDOW(inRight), FALSE);

	iface=XFDASHBOARD_WINDOW_TRACKER_WINDOW_GET_IFACE(inLeft);

	/* Call virtual function */
	if(iface->is_equal)
	{
		return(iface->is_equal(inLeft, inRight));
	}

	/* If we get here the virtual function was not overridden */
	XFDASHBOARD_WINDOWS_TRACKER_WINDOW_WARN_NOT_IMPLEMENTED(inLeft, "is_equal");
	return(FALSE);
}

/* Determine if window is visible at all */
gboolean xfdashboard_window_tracker_window_is_visible(XfdashboardWindowTrackerWindow *self)
{
	XfdashboardWindowTrackerWindowInterface		*iface;

	g_return_val_if_fail(XFDASHBOARD_IS_WINDOW_TRACKER_WINDOW(self), FALSE);

	iface=XFDASHBOARD_WINDOW_TRACKER_WINDOW_GET_IFACE(self);

	/* Call virtual function */
	if(iface->is_visible)
	{
		return(iface->is_visible(self));
	}

	/* If we get here the virtual function was not overridden */
	XFDASHBOARD_WINDOWS_TRACKER_WINDOW_WARN_NOT_IMPLEMENTED(self, "is_visible");
	return(FALSE);
}

/* Determine if window is visible and placed on requested workspace */
gboolean xfdashboard_window_tracker_window_is_visible_on_workspace(XfdashboardWindowTrackerWindow *self,
																	XfdashboardWindowTrackerWorkspace *inWorkspace)
{
	g_return_val_if_fail(XFDASHBOARD_IS_WINDOW_TRACKER_WINDOW(self), FALSE);
	g_return_val_if_fail(XFDASHBOARD_IS_WINDOW_TRACKER_WORKSPACE(inWorkspace), FALSE);

	/* Check if window is visible generally and if it is on requested workspace */
	return(xfdashboard_window_tracker_window_is_visible(self) &&
			xfdashboard_window_tracker_window_is_on_workspace(self, inWorkspace));
}

/* Determine if window is visible and placed on requested monitor */
gboolean xfdashboard_window_tracker_window_is_visible_on_monitor(XfdashboardWindowTrackerWindow *self,
																	XfdashboardWindowTrackerMonitor *inMonitor)
{
	g_return_val_if_fail(XFDASHBOARD_IS_WINDOW_TRACKER_WINDOW(self), FALSE);
	g_return_val_if_fail(XFDASHBOARD_IS_WINDOW_TRACKER_MONITOR(inMonitor), FALSE);

	/* Check if window is visible generally and if it is on requested monitor */
	return(xfdashboard_window_tracker_window_is_visible(self) &&
			xfdashboard_window_tracker_window_is_on_monitor(self, inMonitor));
}

/* Show window */
void xfdashboard_window_tracker_window_show(XfdashboardWindowTrackerWindow *self)
{
	XfdashboardWindowTrackerWindowInterface		*iface;

	g_return_if_fail(XFDASHBOARD_IS_WINDOW_TRACKER_WINDOW(self));

	iface=XFDASHBOARD_WINDOW_TRACKER_WINDOW_GET_IFACE(self);

	/* Call virtual function */
	if(iface->show)
	{
		iface->show(self);
		return;
	}

	/* If we get here the virtual function was not overridden */
	XFDASHBOARD_WINDOWS_TRACKER_WINDOW_WARN_NOT_IMPLEMENTED(self, "show");
}

/* Hide window */
void xfdashboard_window_tracker_window_hide(XfdashboardWindowTrackerWindow *self)
{
	XfdashboardWindowTrackerWindowInterface		*iface;

	g_return_if_fail(XFDASHBOARD_IS_WINDOW_TRACKER_WINDOW(self));

	iface=XFDASHBOARD_WINDOW_TRACKER_WINDOW_GET_IFACE(self);

	/* Call virtual function */
	if(iface->hide)
	{
		iface->hide(self);
		return;
	}

	/* If we get here the virtual function was not overridden */
	XFDASHBOARD_WINDOWS_TRACKER_WINDOW_WARN_NOT_IMPLEMENTED(self, "hide");
}

/* Get parent window of this window */
XfdashboardWindowTrackerWindow* xfdashboard_window_tracker_window_get_parent(XfdashboardWindowTrackerWindow *self)
{
	XfdashboardWindowTrackerWindowInterface		*iface;

	g_return_val_if_fail(XFDASHBOARD_IS_WINDOW_TRACKER_WINDOW(self), NULL);

	iface=XFDASHBOARD_WINDOW_TRACKER_WINDOW_GET_IFACE(self);

	/* Call virtual function */
	if(iface->get_parent)
	{
		return(iface->get_parent(self));
	}

	/* If we get here the virtual function was not overridden */
	XFDASHBOARD_WINDOWS_TRACKER_WINDOW_WARN_NOT_IMPLEMENTED(self, "get_parent");
	return(NULL);
}

/* Get state of window */
XfdashboardWindowTrackerWindowState xfdashboard_window_tracker_window_get_state(XfdashboardWindowTrackerWindow *self)
{
	XfdashboardWindowTrackerWindowInterface		*iface;

	g_return_val_if_fail(XFDASHBOARD_IS_WINDOW_TRACKER_WINDOW(self), 0);

	iface=XFDASHBOARD_WINDOW_TRACKER_WINDOW_GET_IFACE(self);

	/* Call virtual function */
	if(iface->get_state)
	{
		return(iface->get_state(self));
	}

	/* If we get here the virtual function was not overridden */
	XFDASHBOARD_WINDOWS_TRACKER_WINDOW_WARN_NOT_IMPLEMENTED(self, "get_state");
	return(0);
}

/* Get possible actions for requested window */
XfdashboardWindowTrackerWindowAction xfdashboard_window_tracker_window_get_actions(XfdashboardWindowTrackerWindow *self)
{
	XfdashboardWindowTrackerWindowInterface		*iface;

	g_return_val_if_fail(XFDASHBOARD_IS_WINDOW_TRACKER_WINDOW(self), 0);

	iface=XFDASHBOARD_WINDOW_TRACKER_WINDOW_GET_IFACE(self);

	/* Call virtual function */
	if(iface->get_actions)
	{
		return(iface->get_actions(self));
	}

	/* If we get here the virtual function was not overridden */
	XFDASHBOARD_WINDOWS_TRACKER_WINDOW_WARN_NOT_IMPLEMENTED(self, "get_actions");
	return(0);
}

/* Get name (title) of window */
const gchar* xfdashboard_window_tracker_window_get_name(XfdashboardWindowTrackerWindow *self)
{
	XfdashboardWindowTrackerWindowInterface		*iface;

	g_return_val_if_fail(XFDASHBOARD_IS_WINDOW_TRACKER_WINDOW(self), NULL);

	iface=XFDASHBOARD_WINDOW_TRACKER_WINDOW_GET_IFACE(self);

	/* Call virtual function */
	if(iface->get_name)
	{
		return(iface->get_name(self));
	}

	/* If we get here the virtual function was not overridden */
	XFDASHBOARD_WINDOWS_TRACKER_WINDOW_WARN_NOT_IMPLEMENTED(self, "get_name");
	return(NULL);
}

/* Get icon of window */
GdkPixbuf* xfdashboard_window_tracker_window_get_icon(XfdashboardWindowTrackerWindow *self)
{
	XfdashboardWindowTrackerWindowInterface		*iface;

	g_return_val_if_fail(XFDASHBOARD_IS_WINDOW_TRACKER_WINDOW(self), NULL);

	iface=XFDASHBOARD_WINDOW_TRACKER_WINDOW_GET_IFACE(self);

	/* Call virtual function */
	if(iface->get_icon)
	{
		return(iface->get_icon(self));
	}

	/* If we get here the virtual function was not overridden */
	XFDASHBOARD_WINDOWS_TRACKER_WINDOW_WARN_NOT_IMPLEMENTED(self, "get_icon");
	return(NULL);
}

const gchar* xfdashboard_window_tracker_window_get_icon_name(XfdashboardWindowTrackerWindow *self)
{
	XfdashboardWindowTrackerWindowInterface		*iface;

	g_return_val_if_fail(XFDASHBOARD_IS_WINDOW_TRACKER_WINDOW(self), NULL);

	iface=XFDASHBOARD_WINDOW_TRACKER_WINDOW_GET_IFACE(self);

	/* Call virtual function */
	if(iface->get_icon_name)
	{
		return(iface->get_icon_name(self));
	}

	/* If we get here the virtual function was not overridden */
	XFDASHBOARD_WINDOWS_TRACKER_WINDOW_WARN_NOT_IMPLEMENTED(self, "get_icon_name");
	return(NULL);
}

/* Get workspace where window is on */
XfdashboardWindowTrackerWorkspace* xfdashboard_window_tracker_window_get_workspace(XfdashboardWindowTrackerWindow *self)
{
	XfdashboardWindowTrackerWindowInterface		*iface;

	g_return_val_if_fail(XFDASHBOARD_IS_WINDOW_TRACKER_WINDOW(self), NULL);

	iface=XFDASHBOARD_WINDOW_TRACKER_WINDOW_GET_IFACE(self);

	/* Call virtual function */
	if(iface->get_workspace)
	{
		return(iface->get_workspace(self));
	}

	/* If we get here the virtual function was not overridden */
	XFDASHBOARD_WINDOWS_TRACKER_WINDOW_WARN_NOT_IMPLEMENTED(self, "get_workspace");
	return(NULL);
}

/* Determine if window is on requested workspace */
gboolean xfdashboard_window_tracker_window_is_on_workspace(XfdashboardWindowTrackerWindow *self,
															XfdashboardWindowTrackerWorkspace *inWorkspace)
{
	XfdashboardWindowTrackerWindowInterface		*iface;

	g_return_val_if_fail(XFDASHBOARD_IS_WINDOW_TRACKER_WINDOW(self), FALSE);

	iface=XFDASHBOARD_WINDOW_TRACKER_WINDOW_GET_IFACE(self);

	/* Call virtual function */
	if(iface->is_on_workspace)
	{
		return(iface->is_on_workspace(self, inWorkspace));
	}

	/* If we get here the virtual function was not overridden */
	XFDASHBOARD_WINDOWS_TRACKER_WINDOW_WARN_NOT_IMPLEMENTED(self, "is_on_workspace");
	return(FALSE);
}

/* Move a window to another workspace */
void xfdashboard_window_tracker_window_move_to_workspace(XfdashboardWindowTrackerWindow *self,
															XfdashboardWindowTrackerWorkspace *inWorkspace)
{
	XfdashboardWindowTrackerWindowInterface		*iface;

	g_return_if_fail(XFDASHBOARD_IS_WINDOW_TRACKER_WINDOW(self));
	g_return_if_fail(XFDASHBOARD_IS_WINDOW_TRACKER_WORKSPACE(inWorkspace));

	iface=XFDASHBOARD_WINDOW_TRACKER_WINDOW_GET_IFACE(self);

	/* Call virtual function */
	if(iface->move_to_workspace)
	{
		iface->move_to_workspace(self, inWorkspace);
		return;
	}

	/* If we get here the virtual function was not overridden */
	XFDASHBOARD_WINDOWS_TRACKER_WINDOW_WARN_NOT_IMPLEMENTED(self, "move_to_workspace");
}

/* Get monitor where window is on */
XfdashboardWindowTrackerMonitor* xfdashboard_window_tracker_window_get_monitor(XfdashboardWindowTrackerWindow *self)
{
	XfdashboardWindowTrackerWindowInterface		*iface;

	g_return_val_if_fail(XFDASHBOARD_IS_WINDOW_TRACKER_WINDOW(self), NULL);

	iface=XFDASHBOARD_WINDOW_TRACKER_WINDOW_GET_IFACE(self);

	/* Call virtual function */
	if(iface->get_monitor)
	{
		return(iface->get_monitor(self));
	}

	/* If we get here the virtual function was not overridden */
	XFDASHBOARD_WINDOWS_TRACKER_WINDOW_WARN_NOT_IMPLEMENTED(self, "get_monitor");
	return(NULL);
}

/* Determine if window is on requested monitor */
gboolean xfdashboard_window_tracker_window_is_on_monitor(XfdashboardWindowTrackerWindow *self,
															XfdashboardWindowTrackerMonitor *inMonitor)
{
	XfdashboardWindowTrackerWindowInterface		*iface;

	g_return_val_if_fail(XFDASHBOARD_IS_WINDOW_TRACKER_WINDOW(self), FALSE);

	iface=XFDASHBOARD_WINDOW_TRACKER_WINDOW_GET_IFACE(self);

	/* Call virtual function */
	if(iface->is_on_monitor)
	{
		return(iface->is_on_monitor(self, inMonitor));
	}

	/* If we get here the virtual function was not overridden */
	XFDASHBOARD_WINDOWS_TRACKER_WINDOW_WARN_NOT_IMPLEMENTED(self, "is_on_monitor");
	return(FALSE);
}

/* Get geometry of window */
void xfdashboard_window_tracker_window_get_geometry(XfdashboardWindowTrackerWindow *self,
													gint *outX,
													gint *outY,
													gint *outWidth,
													gint *outHeight)
{
	XfdashboardWindowTrackerWindowInterface		*iface;
	gint										x, y, width, height;

	g_return_if_fail(XFDASHBOARD_IS_WINDOW_TRACKER_WINDOW(self));

	iface=XFDASHBOARD_WINDOW_TRACKER_WINDOW_GET_IFACE(self);

	/* Get window geometry */
	if(iface->get_geometry)
	{
		/* Get geometry */
		iface->get_geometry(self, &x, &y, &width, &height);

		/* Set result */
		if(outX) *outX=x;
		if(outX) *outY=y;
		if(outWidth) *outWidth=width;
		if(outHeight) *outHeight=height;

		return;
	}

	/* If we get here the virtual function was not overridden */
	XFDASHBOARD_WINDOWS_TRACKER_WINDOW_WARN_NOT_IMPLEMENTED(self, "get_geometry");
}

/* Set geometry of window */
void xfdashboard_window_tracker_window_set_geometry(XfdashboardWindowTrackerWindow *self,
													gint inX,
													gint inY,
													gint inWidth,
													gint inHeight)
{
	XfdashboardWindowTrackerWindowInterface		*iface;

	g_return_if_fail(XFDASHBOARD_IS_WINDOW_TRACKER_WINDOW(self));

	iface=XFDASHBOARD_WINDOW_TRACKER_WINDOW_GET_IFACE(self);

	/* Call virtual function */
	if(iface->set_geometry)
	{
		iface->set_geometry(self, inX, inY, inWidth, inHeight);
		return;
	}

	/* If we get here the virtual function was not overridden */
	XFDASHBOARD_WINDOWS_TRACKER_WINDOW_WARN_NOT_IMPLEMENTED(self, "set_geometry");
}

/* Move window */
void xfdashboard_window_tracker_window_move(XfdashboardWindowTrackerWindow *inWindow,
											gint inX,
											gint inY)
{
	xfdashboard_window_tracker_window_set_geometry(inWindow, inX, inY, -1, -1);
}

/* Resize window */
void xfdashboard_window_tracker_window_resize(XfdashboardWindowTrackerWindow *inWindow,
												gint inWidth,
												gint inHeight)
{
	xfdashboard_window_tracker_window_set_geometry(inWindow, -1, -1, inWidth, inHeight);
}

/* Activate window with its transient windows */
void xfdashboard_window_tracker_window_activate(XfdashboardWindowTrackerWindow *self)
{
	XfdashboardWindowTrackerWindowInterface		*iface;

	g_return_if_fail(XFDASHBOARD_IS_WINDOW_TRACKER_WINDOW(self));

	iface=XFDASHBOARD_WINDOW_TRACKER_WINDOW_GET_IFACE(self);

	/* Call virtual function */
	if(iface->activate)
	{
		iface->activate(self);
		return;
	}

	/* If we get here the virtual function was not overridden */
	XFDASHBOARD_WINDOWS_TRACKER_WINDOW_WARN_NOT_IMPLEMENTED(self, "activate");
}

/* Close window */
void xfdashboard_window_tracker_window_close(XfdashboardWindowTrackerWindow *self)
{
	XfdashboardWindowTrackerWindowInterface		*iface;

	g_return_if_fail(XFDASHBOARD_IS_WINDOW_TRACKER_WINDOW(self));

	iface=XFDASHBOARD_WINDOW_TRACKER_WINDOW_GET_IFACE(self);

	/* Call virtual function */
	if(iface->close)
	{
		iface->close(self);
		return;
	}

	/* If we get here the virtual function was not overridden */
	XFDASHBOARD_WINDOWS_TRACKER_WINDOW_WARN_NOT_IMPLEMENTED(self, "close");
}

/* Determine if window is a stage window */
gboolean xfdashboard_window_tracker_window_is_stage(XfdashboardWindowTrackerWindow *self)
{
	g_return_val_if_fail(XFDASHBOARD_IS_WINDOW_TRACKER_WINDOW(self), FALSE);

	return(xfdashboard_window_tracker_window_get_stage(self)!=NULL);
}

/**
 * xfdashboard_window_tracker_window_get_stage:
 * @self: A #XfdashboardWindowTrackerWindow
 *
 * Get stage for requested stage window @self from default window tracker backend.
 *
 * This function is the logical equivalent of:
 *
 * |[<!-- language="C" -->
 *   XfdashboardWindowTrackerBackend *backend;
 *   ClutterStage                    *stage;
 *
 *   backend=xfdashboard_window_tracker_backend_get_default();
 *   stage=xfdashboard_window_tracker_backend_get_stage_from_window(backend, self);
 *   g_object_unref(backend);
 * ]|
 *
 * Return value: (transfer none): The #ClutterStage for stage window @self or
 *   %NULL if @self is not a stage window or stage could not be found.
 */
ClutterStage* xfdashboard_window_tracker_window_get_stage(XfdashboardWindowTrackerWindow *self)
{
	XfdashboardWindowTrackerBackend		*backend;
	ClutterStage						*stage;

	g_return_val_if_fail(XFDASHBOARD_IS_WINDOW_TRACKER_WINDOW(self), NULL);

	/* Get default window tracker backend */
	backend=xfdashboard_window_tracker_backend_get_default();
	if(!backend)
	{
		g_critical("Could not get default window tracker backend");
		return(NULL);
	}

	/* Redirect function to window tracker backend */
	stage=xfdashboard_window_tracker_backend_get_stage_from_window(backend, self);

	/* Release allocated resources */
	if(backend) g_object_unref(backend);

	/* Return the stage found */
	return(stage);
}

/**
 * xfdashboard_window_tracker_window_show_stage:
 * @self: A #XfdashboardWindowTrackerWindow
 *
 * Asks the default window tracker backend to set up and show the window @self
 * for use as stage window.
 *
 * This function is the logical equivalent of:
 *
 * |[<!-- language="C" -->
 *   XfdashboardWindowTrackerBackend *backend;
 *
 *   backend=xfdashboard_window_tracker_backend_get_default();
 *   xfdashboard_window_tracker_backend_show_stage_window(backend, self);
 *   g_object_unref(backend);
 * ]|
 */
void xfdashboard_window_tracker_window_show_stage(XfdashboardWindowTrackerWindow *self)
{
	XfdashboardWindowTrackerBackend		*backend;

	g_return_if_fail(XFDASHBOARD_IS_WINDOW_TRACKER_WINDOW(self));

	/* Get default window tracker backend */
	backend=xfdashboard_window_tracker_backend_get_default();
	if(!backend)
	{
		g_critical("Could not get default window tracker backend");
		return;
	}

	/* Redirect function to window tracker backend */
	xfdashboard_window_tracker_backend_show_stage_window(backend, self);

	/* Release allocated resources */
	if(backend) g_object_unref(backend);
}

/**
 * xfdashboard_window_tracker_window_hide_stage:
 * @self: A #XfdashboardWindowTrackerWindow
 *
 * Asks the default window tracker backend to hide the stage window @self.
 * This function is the logical equivalent of:
 *
 * |[<!-- language="C" -->
 *   XfdashboardWindowTrackerBackend *backend;
 *
 *   backend=xfdashboard_window_tracker_backend_get_default();
 *   xfdashboard_window_tracker_backend_hide_stage_window(backend, self);
 *   g_object_unref(backend);
 * ]|
 */
void xfdashboard_window_tracker_window_hide_stage(XfdashboardWindowTrackerWindow *self)
{
	XfdashboardWindowTrackerBackend		*backend;

	g_return_if_fail(XFDASHBOARD_IS_WINDOW_TRACKER_WINDOW(self));

	/* Get default window tracker backend */
	backend=xfdashboard_window_tracker_backend_get_default();
	if(!backend)
	{
		g_critical("Could not get default window tracker backend");
		return;
	}

	/* Redirect function to window tracker backend */
	xfdashboard_window_tracker_backend_hide_stage_window(backend, self);

	/* Release allocated resources */
	if(backend) g_object_unref(backend);
}

/* Get process ID owning the requested window */
gint xfdashboard_window_tracker_window_get_pid(XfdashboardWindowTrackerWindow *self)
{
	XfdashboardWindowTrackerWindowInterface		*iface;

	g_return_val_if_fail(XFDASHBOARD_IS_WINDOW_TRACKER_WINDOW(self), -1);

	iface=XFDASHBOARD_WINDOW_TRACKER_WINDOW_GET_IFACE(self);

	/* Call virtual function */
	if(iface->get_pid)
	{
		return(iface->get_pid(self));
	}

	/* If we get here the virtual function was not overridden */
	XFDASHBOARD_WINDOWS_TRACKER_WINDOW_WARN_NOT_IMPLEMENTED(self, "get_pid");
	return(-1);
}

/* Get all possible instance name for window, e.g. class name, instance name.
 * Caller is responsible to free result with g_strfreev() if not NULL.
 */
gchar** xfdashboard_window_tracker_window_get_instance_names(XfdashboardWindowTrackerWindow *self)
{
	XfdashboardWindowTrackerWindowInterface		*iface;

	g_return_val_if_fail(XFDASHBOARD_IS_WINDOW_TRACKER_WINDOW(self), NULL);

	iface=XFDASHBOARD_WINDOW_TRACKER_WINDOW_GET_IFACE(self);

	/* Call virtual function */
	if(iface->get_instance_names)
	{
		return(iface->get_instance_names(self));
	}

	/* If we get here the virtual function was not overridden */
	XFDASHBOARD_WINDOWS_TRACKER_WINDOW_WARN_NOT_IMPLEMENTED(self, "get_instance_names");
	return(NULL);
}

/* Get content for this window for use in actors.
 * Caller is responsible to remove reference with g_object_unref().
 */
ClutterContent* xfdashboard_window_tracker_window_get_content(XfdashboardWindowTrackerWindow *self)
{
	XfdashboardWindowTrackerWindowInterface		*iface;

	g_return_val_if_fail(XFDASHBOARD_IS_WINDOW_TRACKER_WINDOW(self), NULL);

	iface=XFDASHBOARD_WINDOW_TRACKER_WINDOW_GET_IFACE(self);

	/* Call virtual function */
	if(iface->get_content)
	{
		return(iface->get_content(self));
	}

	/* If we get here the virtual function was not overridden */
	XFDASHBOARD_WINDOWS_TRACKER_WINDOW_WARN_NOT_IMPLEMENTED(self, "get_content");
	return(NULL);
}
