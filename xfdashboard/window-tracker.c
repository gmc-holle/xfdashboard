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

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "window-tracker.h"

#define WNCK_I_KNOW_THIS_IS_UNSTABLE
#include <libwnck/libwnck.h>

#include <glib/gi18n-lib.h>
#include <clutter/clutter.h>
#include <clutter/x11/clutter-x11.h>
#include <gdk/gdkx.h>
#ifdef HAVE_XINERAMA
#include <X11/extensions/Xinerama.h>
#endif

#include "marshal.h"
#include "application.h"

/* Define this class in GObject system */
G_DEFINE_TYPE(XfdashboardWindowTracker,
				xfdashboard_window_tracker,
				G_TYPE_OBJECT)

/* Private structure - access only by public API if needed */
#define XFDASHBOARD_WINDOW_TRACKER_GET_PRIVATE(obj) \
	(G_TYPE_INSTANCE_GET_PRIVATE((obj), XFDASHBOARD_TYPE_WINDOW_TRACKER, XfdashboardWindowTrackerPrivate))

struct _XfdashboardWindowTrackerPrivate
{
	/* Properties related */
	WnckWindow							*activeWindow;
	WnckWorkspace						*activeWorkspace;
	XfdashboardWindowTrackerMonitor		*primaryMonitor;

	/* Instance related */
	XfdashboardApplication				*application;
	gboolean							isAppSuspended;
	guint								suspendSignalID;

	WnckScreen							*screen;

	gboolean							supportsMultipleMonitors;
	GdkScreen							*gdkScreen;
	GList								*monitors;
};

/* Properties */
enum
{
	PROP_0,

	PROP_ACTIVE_WINDOW,
	PROP_ACTIVE_WORKSPACE,
	PROP_PRIMARY_MONITOR,

	PROP_LAST
};

static GParamSpec* XfdashboardWindowTrackerProperties[PROP_LAST]={ 0, };

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
#define LAST_X_DATA_KEY			"last-x"
#define LAST_Y_DATA_KEY			"last-y"
#define LAST_WIDTH_DATA_KEY		"last-width"
#define LAST_HEIGHT_DATA_KEY	"last-height"

static XfdashboardWindowTracker *_xfdashboard_window_tracker_singleton=NULL;


/* Position and/or size of window has changed */
static void _xfdashboard_window_tracker_on_window_geometry_changed(XfdashboardWindowTracker *self, gpointer inUserData)
{
	XfdashboardWindowTrackerPrivate			*priv;
	WnckWindow								*window;
	gint									x, y, width, height;
	gint									lastX, lastY, lastWidth, lastHeight;
	XfdashboardWindowTrackerMonitor			*currentMonitor;
	XfdashboardWindowTrackerMonitor			*lastMonitor;
	GList									*iter;
	gint									screenWidth, screenHeight;
	gint									windowMiddleX, windowMiddleY;

	g_return_if_fail(XFDASHBOARD_IS_WINDOW_TRACKER(self));
	g_return_if_fail(WNCK_IS_WINDOW(inUserData));

	priv=self->priv;
	window=WNCK_WINDOW(inUserData);


	/* Get last and current position and size of window to determine
	 * if window has moved or resized and do not emit this signal if
	 * neither happened, although it is very unlike that the window
	 * has not moved or resized if this signal was called.
	 */
	lastX=GPOINTER_TO_INT(g_object_get_data(G_OBJECT(window), LAST_X_DATA_KEY));
	lastY=GPOINTER_TO_INT(g_object_get_data(G_OBJECT(window), LAST_Y_DATA_KEY));
	lastWidth=GPOINTER_TO_INT(g_object_get_data(G_OBJECT(window), LAST_WIDTH_DATA_KEY));
	lastHeight=GPOINTER_TO_INT(g_object_get_data(G_OBJECT(window), LAST_HEIGHT_DATA_KEY));

	xfdashboard_window_tracker_window_get_position_size(window, &x, &y, &width, &height);

	if(G_UNLIKELY(lastX==x && lastY==y && lastWidth==width && lastHeight==height))
	{
		g_debug("Window '%s' has not moved or resized", wnck_window_get_name(window));
		return;
	}

	/* Emit signal */
	g_debug("Window '%s' changed position and/or size", wnck_window_get_name(window));
	g_signal_emit(self, XfdashboardWindowTrackerSignals[SIGNAL_WINDOW_GEOMETRY_CHANGED], 0, window);

	/* Get monitor at old position of window and the monitor at current position.
	 * If they differ emit signal for window changed monitor.
	 */
	screenWidth=xfdashboard_window_tracker_get_screen_width(self);
	screenHeight=xfdashboard_window_tracker_get_screen_height(self);

	windowMiddleX=lastX+(lastWidth/2);
	if(windowMiddleX>screenWidth) windowMiddleX=screenWidth-1;

	windowMiddleY=lastY+(lastHeight/2);
	if(windowMiddleY>screenHeight) windowMiddleY=screenHeight-1;

	lastMonitor=NULL;
	for(iter=priv->monitors; iter && !lastMonitor; iter=g_list_next(iter))
	{
		gint								monitorX, monitorY, monitorWidth, monitorHeight;
		XfdashboardWindowTrackerMonitor		*monitor;

		/* Get monitor */
		monitor=XFDASHBOARD_WINDOW_TRACKER_MONITOR(iter->data);

		/* Get monitor geometry */
		xfdashboard_window_tracker_monitor_get_geometry(monitor, &monitorX, &monitorY, &monitorWidth, &monitorHeight);

		/* Check if mid-point of window (adjusted to screen size) is within monitor */
		if(windowMiddleX>=monitorX && windowMiddleX<(monitorX+monitorWidth) &&
			windowMiddleY>=monitorY && windowMiddleY<(monitorY+monitorHeight))
		{
			lastMonitor=monitor;
		}
	}

	currentMonitor=xfdashboard_window_tracker_window_get_monitor(window);
	if(currentMonitor!=lastMonitor)
	{
		/* Emit signal */
		g_debug("Window '%s' moved from monitor %d to %d",
					wnck_window_get_name(window),
					xfdashboard_window_tracker_monitor_get_number(lastMonitor),
					xfdashboard_window_tracker_monitor_get_number(currentMonitor));
		g_signal_emit(self, XfdashboardWindowTrackerSignals[SIGNAL_WINDOW_MONITOR_CHANGED], 0, window, lastMonitor, currentMonitor);
	}

	/* Remember new position and size as last known ones */
	g_object_set_data(G_OBJECT(window), LAST_X_DATA_KEY, GINT_TO_POINTER(x));
	g_object_set_data(G_OBJECT(window), LAST_Y_DATA_KEY, GINT_TO_POINTER(y));
	g_object_set_data(G_OBJECT(window), LAST_WIDTH_DATA_KEY, GINT_TO_POINTER(width));
	g_object_set_data(G_OBJECT(window), LAST_HEIGHT_DATA_KEY, GINT_TO_POINTER(height));
}

/* Action items of window has changed */
static void _xfdashboard_window_tracker_on_window_actions_changed(XfdashboardWindowTracker *self,
																	WnckWindowActions inChangedMask,
																	WnckWindowActions inNewValue,
																	gpointer inUserData)
{
	WnckWindow			*window;

	g_return_if_fail(XFDASHBOARD_IS_WINDOW_TRACKER(self));
	g_return_if_fail(WNCK_IS_WINDOW(inUserData));

	window=WNCK_WINDOW(inUserData);

	/* Emit signal */
	g_debug("Window '%s' changed actions to %u with mask %u", wnck_window_get_name(window), inNewValue, inChangedMask);
	g_signal_emit(self, XfdashboardWindowTrackerSignals[SIGNAL_WINDOW_ACTIONS_CHANGED], 0, window);
}

/* State of window has changed */
static void _xfdashboard_window_tracker_on_window_state_changed(XfdashboardWindowTracker *self,
																	WnckWindowState inChangedMask,
																	WnckWindowState inNewValue,
																	gpointer inUserData)
{
	WnckWindow			*window;

	g_return_if_fail(XFDASHBOARD_IS_WINDOW_TRACKER(self));
	g_return_if_fail(WNCK_IS_WINDOW(inUserData));

	window=WNCK_WINDOW(inUserData);

	/* Emit signal */
	g_debug("Window '%s' changed state to %u with mask %u", wnck_window_get_name(window), inNewValue, inChangedMask);
	g_signal_emit(self, XfdashboardWindowTrackerSignals[SIGNAL_WINDOW_STATE_CHANGED], 0, window);
}

/* Icon of window has changed */
static void _xfdashboard_window_tracker_on_window_icon_changed(XfdashboardWindowTracker *self, gpointer inUserData)
{
	WnckWindow			*window;

	g_return_if_fail(XFDASHBOARD_IS_WINDOW_TRACKER(self));
	g_return_if_fail(WNCK_IS_WINDOW(inUserData));

	window=WNCK_WINDOW(inUserData);

	/* Emit signal */
	g_debug("Window '%s' changed its icon", wnck_window_get_name(window));
	g_signal_emit(self, XfdashboardWindowTrackerSignals[SIGNAL_WINDOW_ICON_CHANGED], 0, window);
}

/* Name of window has changed */
static void _xfdashboard_window_tracker_on_window_name_changed(XfdashboardWindowTracker *self, gpointer inUserData)
{
	WnckWindow			*window;

	g_return_if_fail(XFDASHBOARD_IS_WINDOW_TRACKER(self));
	g_return_if_fail(WNCK_IS_WINDOW(inUserData));

	window=WNCK_WINDOW(inUserData);

	/* Emit "signal */
	g_debug("Window changed its name to '%s'", wnck_window_get_name(window));
	g_signal_emit(self, XfdashboardWindowTrackerSignals[SIGNAL_WINDOW_NAME_CHANGED], 0, window);
}

/* A window has moved to another workspace */
static void _xfdashboard_window_tracker_on_window_workspace_changed(XfdashboardWindowTracker *self, gpointer inUserData)
{
	WnckWindow			*window;
	WnckWorkspace		*workspace;

	g_return_if_fail(XFDASHBOARD_IS_WINDOW_TRACKER(self));
	g_return_if_fail(WNCK_IS_WINDOW(inUserData));

	window=WNCK_WINDOW(inUserData);

	/* Get workspace window resides on */
	workspace=wnck_window_get_workspace(window);

	/* Emit signal */
	g_debug("Window '%s' moved to workspace %d (%s)",
				wnck_window_get_name(window),
				workspace ? wnck_workspace_get_number(workspace) : -1,
				workspace ? wnck_workspace_get_name(workspace) : "<nil>");
	g_signal_emit(self, XfdashboardWindowTrackerSignals[SIGNAL_WINDOW_WORKSPACE_CHANGED], 0, window, workspace);
}

/* A window was activated */
static void _xfdashboard_window_tracker_on_active_window_changed(XfdashboardWindowTracker *self,
																	WnckWindow *inPreviousWindow,
																	gpointer inUserData)
{
	XfdashboardWindowTrackerPrivate		*priv;
	WnckScreen							*screen;
	WnckWindow							*oldActiveWindow;

	g_return_if_fail(XFDASHBOARD_IS_WINDOW_TRACKER(self));
	g_return_if_fail(inPreviousWindow==NULL || WNCK_IS_WINDOW(inPreviousWindow));
	g_return_if_fail(WNCK_IS_SCREEN(inUserData));

	priv=self->priv;
	screen=WNCK_SCREEN(inUserData);

	/* Get and remember new active window */
	oldActiveWindow=priv->activeWindow;
	priv->activeWindow=wnck_screen_get_active_window(screen);

	/* Emit signal */
	g_debug("Active window changed from '%s' to '%s'",
				oldActiveWindow ? wnck_window_get_name(oldActiveWindow) : "<nil>",
				priv->activeWindow ? wnck_window_get_name(priv->activeWindow) : "<nil>");
	g_signal_emit(self, XfdashboardWindowTrackerSignals[SIGNAL_ACTIVE_WINDOW_CHANGED], 0, oldActiveWindow, priv->activeWindow);
}

/* A window was closed */
static void _xfdashboard_window_tracker_on_window_closed(XfdashboardWindowTracker *self,
															WnckWindow *inWindow,
															gpointer inUserData)
{
	XfdashboardWindowTrackerPrivate		*priv;

	g_return_if_fail(XFDASHBOARD_IS_WINDOW_TRACKER(self));
	g_return_if_fail(WNCK_IS_WINDOW(inWindow));
	g_return_if_fail(WNCK_IS_SCREEN(inUserData));

	priv=self->priv;

	/* Should not happen but if closed window is the last known one
	 * reset to NULL
	 */
	if(priv->activeWindow==inWindow) priv->activeWindow=NULL;

	/* Remove all signal handlers for closed window */
	g_signal_handlers_disconnect_by_data(inWindow, self);

	/* Emit signals */
	g_debug("Window '%s' closed", wnck_window_get_name(inWindow));
	g_signal_emit(self, XfdashboardWindowTrackerSignals[SIGNAL_WINDOW_CLOSED], 0, inWindow);
}

/* A new window was opened */
static void _xfdashboard_window_tracker_on_window_opened(XfdashboardWindowTracker *self,
															WnckWindow *inWindow,
															gpointer inUserData)
{
	XfdashboardWindowTrackerPrivate		*priv;
	gint								x, y, width, height;

	g_return_if_fail(XFDASHBOARD_IS_WINDOW_TRACKER(self));
	g_return_if_fail(WNCK_IS_WINDOW(inWindow));
	g_return_if_fail(WNCK_IS_SCREEN(inUserData));

	priv=self->priv;

	/* Remember current position and size as last known ones */
	xfdashboard_window_tracker_window_get_position_size(inWindow, &x, &y, &width, &height);
	g_object_set_data(G_OBJECT(inWindow), LAST_X_DATA_KEY, GINT_TO_POINTER(x));
	g_object_set_data(G_OBJECT(inWindow), LAST_Y_DATA_KEY, GINT_TO_POINTER(y));
	g_object_set_data(G_OBJECT(inWindow), LAST_WIDTH_DATA_KEY, GINT_TO_POINTER(width));
	g_object_set_data(G_OBJECT(inWindow), LAST_HEIGHT_DATA_KEY, GINT_TO_POINTER(height));

	/* Connect signals on newly opened window */
	g_signal_connect_swapped(inWindow, "actions-changed", G_CALLBACK(_xfdashboard_window_tracker_on_window_actions_changed), self);
	g_signal_connect_swapped(inWindow, "state-changed", G_CALLBACK(_xfdashboard_window_tracker_on_window_state_changed), self);
	g_signal_connect_swapped(inWindow, "icon-changed", G_CALLBACK(_xfdashboard_window_tracker_on_window_icon_changed), self);
	g_signal_connect_swapped(inWindow, "name-changed", G_CALLBACK(_xfdashboard_window_tracker_on_window_name_changed), self);
	g_signal_connect_swapped(inWindow, "workspace-changed", G_CALLBACK(_xfdashboard_window_tracker_on_window_workspace_changed), self);

	/* Connect to 'geometry-changed' at window only if application is not suspended */
	if(!priv->isAppSuspended)
	{
		g_signal_connect_swapped(inWindow, "geometry-changed", G_CALLBACK(_xfdashboard_window_tracker_on_window_geometry_changed), self);
	}

	/* Emit signal */
	g_debug("Window '%s' created", wnck_window_get_name(inWindow));
	g_signal_emit(self, XfdashboardWindowTrackerSignals[SIGNAL_WINDOW_OPENED], 0, inWindow);
}

/* Window stacking has changed */
static void _xfdashboard_window_tracker_on_window_stacking_changed(XfdashboardWindowTracker *self,
																	gpointer inUserData)
{
	g_return_if_fail(XFDASHBOARD_IS_WINDOW_TRACKER(self));

	/* Emit signal */
	g_debug("Window stacking has changed");
	g_signal_emit(self, XfdashboardWindowTrackerSignals[SIGNAL_WINDOW_STACKING_CHANGED], 0);
}

/* A workspace changed its name */
static void _xfdashboard_window_tracker_on_workspace_name_changed(XfdashboardWindowTracker *self,
																	gpointer inUserData)
{
	WnckWorkspace				*workspace;

	g_return_if_fail(XFDASHBOARD_IS_WINDOW_TRACKER(self));
	g_return_if_fail(WNCK_IS_WORKSPACE(inUserData));

	workspace=WNCK_WORKSPACE(inUserData);

	/* Emit signal */
	g_debug("Workspace #%d changed name to '%s'", wnck_workspace_get_number(workspace), wnck_workspace_get_name(workspace));
	g_signal_emit(self, XfdashboardWindowTrackerSignals[SIGNAL_WORKSPACE_NAME_CHANGED], 0, workspace);

}

/* A workspace was activated */
static void _xfdashboard_window_tracker_on_active_workspace_changed(XfdashboardWindowTracker *self,
																	WnckWorkspace *inPreviousWorkspace,
																	gpointer inUserData)
{
	XfdashboardWindowTrackerPrivate		*priv;
	WnckScreen							*screen;
	WnckWorkspace						*oldActiveWorkspace;

	g_return_if_fail(XFDASHBOARD_IS_WINDOW_TRACKER(self));
	g_return_if_fail(inPreviousWorkspace==NULL || WNCK_IS_WORKSPACE(inPreviousWorkspace));
	g_return_if_fail(WNCK_IS_SCREEN(inUserData));

	priv=self->priv;
	screen=WNCK_SCREEN(inUserData);

	/* Get and remember new active workspace */
	oldActiveWorkspace=priv->activeWorkspace;
	priv->activeWorkspace=wnck_screen_get_active_workspace(screen);

	/* Emit signal */
	g_debug("Active workspace changed from #%d (%s) to #%d (%s)",
				oldActiveWorkspace ? wnck_workspace_get_number(oldActiveWorkspace) : -1,
				oldActiveWorkspace ? wnck_workspace_get_name(oldActiveWorkspace) : "<nil>",
				priv->activeWorkspace ? wnck_workspace_get_number(priv->activeWorkspace) : -1,
				priv->activeWorkspace ? wnck_workspace_get_name(priv->activeWorkspace) : "<nil>");
	g_signal_emit(self, XfdashboardWindowTrackerSignals[SIGNAL_ACTIVE_WORKSPACE_CHANGED], 0, oldActiveWorkspace, priv->activeWorkspace);
}

/* A workspace was destroyed */
static void _xfdashboard_window_tracker_on_workspace_destroyed(XfdashboardWindowTracker *self,
																WnckWorkspace *inWorkspace,
																gpointer inUserData)
{
	XfdashboardWindowTrackerPrivate		*priv;

	g_return_if_fail(XFDASHBOARD_IS_WINDOW_TRACKER(self));
	g_return_if_fail(WNCK_IS_WORKSPACE(inWorkspace));
	g_return_if_fail(WNCK_IS_SCREEN(inUserData));

	priv=self->priv;

	/* Should not happen but if destroyed workspace is the last known one
	 * reset to NULL
	 */
	if(priv->activeWorkspace==inWorkspace) priv->activeWorkspace=NULL;

	/* Remove all signal handlers for closed window */
	g_signal_handlers_disconnect_by_data(inWorkspace, self);

	/* Emit signal */
	g_debug("Workspace #%d (%s) destroyed", wnck_workspace_get_number(inWorkspace), wnck_workspace_get_name(inWorkspace));
	g_signal_emit(self, XfdashboardWindowTrackerSignals[SIGNAL_WORKSPACE_REMOVED], 0, inWorkspace);
}

/* A new workspace was created */
static void _xfdashboard_window_tracker_on_workspace_created(XfdashboardWindowTracker *self,
																WnckWorkspace *inWorkspace,
																gpointer inUserData)
{
	g_return_if_fail(XFDASHBOARD_IS_WINDOW_TRACKER(self));
	g_return_if_fail(WNCK_IS_WORKSPACE(inWorkspace));
	g_return_if_fail(WNCK_IS_SCREEN(inUserData));

	/* Connect signals on newly created workspace */
	g_signal_connect_swapped(inWorkspace, "name-changed", G_CALLBACK(_xfdashboard_window_tracker_on_workspace_name_changed), self);

	/* Emit signal */
	g_debug("New workspace #%d (%s) created", wnck_workspace_get_number(inWorkspace), wnck_workspace_get_name(inWorkspace));
	g_signal_emit(self, XfdashboardWindowTrackerSignals[SIGNAL_WORKSPACE_ADDED], 0, inWorkspace);
}

/* Primary monitor has changed */
static void _xfdashboard_window_tracker_on_primary_monitor_changed(XfdashboardWindowTracker *self,
																	gpointer inUserData)
{
	XfdashboardWindowTrackerPrivate			*priv;
	XfdashboardWindowTrackerMonitor			*monitor;

	g_return_if_fail(XFDASHBOARD_IS_WINDOW_TRACKER(self));
	g_return_if_fail(XFDASHBOARD_IS_WINDOW_TRACKER_MONITOR(inUserData));

	priv=self->priv;
	monitor=XFDASHBOARD_WINDOW_TRACKER_MONITOR(inUserData);

	/* If monitor emitting this signal is the (new) primary one
	 * then update primary monitor value of this instance.
	 */
	if(xfdashboard_window_tracker_monitor_is_primary(monitor) &&
		priv->primaryMonitor!=monitor)
	{
		XfdashboardWindowTrackerMonitor		*oldMonitor;

		/* Remember old monitor for signal emission */
		oldMonitor=priv->primaryMonitor;

		/* Set value */
		priv->primaryMonitor=monitor;

		/* Emit signal */
		g_signal_emit(self, XfdashboardWindowTrackerSignals[SIGNAL_PRIMARY_MONITOR_CHANGED], 0, oldMonitor, priv->primaryMonitor);

		/* Notify about property change */
		g_object_notify_by_pspec(G_OBJECT(self), XfdashboardWindowTrackerProperties[PROP_PRIMARY_MONITOR]);

		g_debug("Primary monitor changed from %d to %d",
					oldMonitor ? xfdashboard_window_tracker_monitor_get_number(oldMonitor) : -1,
					xfdashboard_window_tracker_monitor_get_number(monitor));
	}
}

/* A monitor has changed its position and/or size */
static void _xfdashboard_window_tracker_on_monitor_geometry_changed(XfdashboardWindowTracker *self,
																	gpointer inUserData)
{
	XfdashboardWindowTrackerMonitor			*monitor;

	g_return_if_fail(XFDASHBOARD_IS_WINDOW_TRACKER(self));
	g_return_if_fail(XFDASHBOARD_IS_WINDOW_TRACKER_MONITOR(inUserData));

	monitor=XFDASHBOARD_WINDOW_TRACKER_MONITOR(inUserData);

	/* A monitor changed its position and/or size so re-emit the signal */
	g_signal_emit(self, XfdashboardWindowTrackerSignals[SIGNAL_MONITOR_GEOMETRY_CHANGED], 0, monitor);
}

/* Create a monitor object */
static XfdashboardWindowTrackerMonitor* _xfdashboard_window_tracker_monitor_new(XfdashboardWindowTracker *self, guint inMonitorIndex)
{
	XfdashboardWindowTrackerPrivate		*priv;
	XfdashboardWindowTrackerMonitor		*monitor;

	g_return_val_if_fail(XFDASHBOARD_IS_WINDOW_TRACKER(self), NULL);
	g_return_val_if_fail(inMonitorIndex>=g_list_length(self->priv->monitors), NULL);

	priv=self->priv;

	/* Create monitor object */
	monitor=XFDASHBOARD_WINDOW_TRACKER_MONITOR(g_object_new(XFDASHBOARD_TYPE_WINDOW_TRACKER_MONITOR,
															"monitor-index", inMonitorIndex,
															NULL));
	priv->monitors=g_list_append(priv->monitors, monitor);

	/* Connect signals */
	g_signal_connect_swapped(monitor,
								"primary-changed",
								G_CALLBACK(_xfdashboard_window_tracker_on_primary_monitor_changed),
								self);
	g_signal_connect_swapped(monitor,
								"geometry-changed",
								G_CALLBACK(_xfdashboard_window_tracker_on_monitor_geometry_changed),
								self);

	/* Emit signal */
	g_signal_emit(self, XfdashboardWindowTrackerSignals[SIGNAL_MONITOR_ADDED], 0, monitor);
	g_debug("Monitor %d added", inMonitorIndex);

	/* If we newly added monitor is the primary one then emit signal. We could not
	 * have done it yet because the signals were connect to new monitor object
	 * after its creation.
	 */
	if(xfdashboard_window_tracker_monitor_is_primary(monitor))
	{
		_xfdashboard_window_tracker_on_primary_monitor_changed(self, monitor);
	}

	/* Return newly created monitor */
	return(monitor);
}

/* Free a monitor object */
static void _xfdashboard_window_tracker_monitor_free(XfdashboardWindowTracker *self, XfdashboardWindowTrackerMonitor *inMonitor)
{
	XfdashboardWindowTrackerPrivate		*priv;
	GList								*iter;

	g_return_if_fail(XFDASHBOARD_IS_WINDOW_TRACKER(self));
	g_return_if_fail(XFDASHBOARD_IS_WINDOW_TRACKER_MONITOR(inMonitor));

	priv=self->priv;

	/* Find monitor to free */
	iter=g_list_find(priv->monitors,  inMonitor);
	if(!iter)
	{
		g_critical(_("Cannot release unknown monitor %d"),
					xfdashboard_window_tracker_monitor_get_number(inMonitor));
		return;
	}

	/* Disconnect signals */
	g_signal_handlers_disconnect_by_data(inMonitor, self);

	/* Emit signal */
	g_signal_emit(self, XfdashboardWindowTrackerSignals[SIGNAL_MONITOR_REMOVED], 0, inMonitor);
	g_debug("Monitor %d removed", xfdashboard_window_tracker_monitor_get_number(inMonitor));

	/* Remove monitor object from list */
	priv->monitors=g_list_delete_link(priv->monitors, iter);

	/* Unref monitor object. Usually this is the last reference released
	 * and the object will be destroyed.
	 */
	g_object_unref(inMonitor);
}

#ifdef HAVE_XINERAMA
/* Number of monitors, primary monitor or size of any monitor changed */
static void _xfdashboard_window_tracker_on_monitors_changed(XfdashboardWindowTracker *self,
																gpointer inUserData)
{
	XfdashboardWindowTrackerPrivate			*priv;
	GdkScreen								*screen;
	gint									currentMonitorCount;
	gint									newMonitorCount;
	gint									i;
	XfdashboardWindowTrackerMonitor			*monitor;
	GList									*iter;

	g_return_if_fail(XFDASHBOARD_IS_WINDOW_TRACKER(self));
	g_return_if_fail(GDK_IS_SCREEN(inUserData));

	priv=self->priv;
	screen=GDK_SCREEN(inUserData);

	/* Get current monitor states */
	currentMonitorCount=g_list_length(priv->monitors);

	/* Get new monitor state */
	newMonitorCount=gdk_screen_get_n_monitors(screen);
	if(newMonitorCount!=currentMonitorCount)
	{
		g_debug("Number of monitors changed from %d to %d", currentMonitorCount, newMonitorCount);
	}

	/* There is no need to check if size of any monitor has changed because
	 * XfdashboardWindowTrackerMonitor instances should also be connected to
	 * this signal and will raise a signal if their size changed. This instance
	 * is connected to this "monitor-has-changed-signal' and will re-emit them.
	 * The same is with primary monitor.
	 */

	/* If number of monitors has increased create newly added monitors */
	if(newMonitorCount>currentMonitorCount)
	{
		for(i=currentMonitorCount; i<newMonitorCount; i++)
		{
			/* Create monitor object */
			_xfdashboard_window_tracker_monitor_new(self, i);
		}
	}

	/* If number of monitors has decreased remove all monitors beyond
	 * the new number of monitors.
	 */
	if(newMonitorCount<currentMonitorCount)
	{
		for(i=currentMonitorCount; i>newMonitorCount; i--)
		{
			/* Get monitor object */
			iter=g_list_last(priv->monitors);
			if(!iter) continue;

			monitor=XFDASHBOARD_WINDOW_TRACKER_MONITOR(iter->data);

			/* Free monitor object */
			_xfdashboard_window_tracker_monitor_free(self, monitor);
		}
	}
}
#endif

/* Total size of screen changed */
static void _xfdashboard_window_tracker_on_screen_size_changed(XfdashboardWindowTracker *self,
																gpointer inUserData)
{
	GdkScreen		*screen;
	gint			w, h;

	g_return_if_fail(XFDASHBOARD_IS_WINDOW_TRACKER(self));
	g_return_if_fail(GDK_IS_SCREEN(inUserData));

	screen=GDK_SCREEN(inUserData);

	/* Get new total size of screen */
	w=gdk_screen_get_width(screen);
	h=gdk_screen_get_height(screen);

	/* Emit signal to tell that screen size has changed */
	g_debug("Screen size changed to %dx%d", w, h);
	g_signal_emit(self, XfdashboardWindowTrackerSignals[SIGNAL_SCREEN_SIZE_CHANGED], 0, w, h);
}

/* Suspension state of application changed */
static void _xfdashboard_window_tracker_on_application_suspended_changed(XfdashboardWindowTracker *self,
																			GParamSpec *inSpec,
																			gpointer inUserData)
{
	XfdashboardWindowTrackerPrivate		*priv;
	XfdashboardApplication				*app;
	GList								*iter;
	XfdashboardWindowTrackerWindow		*window;

	g_return_if_fail(XFDASHBOARD_IS_WINDOW_TRACKER(self));
	g_return_if_fail(XFDASHBOARD_IS_APPLICATION(inUserData));

	priv=self->priv;
	app=XFDASHBOARD_APPLICATION(inUserData);

	/* Get application suspend state */
	priv->isAppSuspended=xfdashboard_application_is_suspended(app);

	/* Iterate through all windows and connect handler to signal 'geometry-changed'
	 * if application was resumed or disconnect signal handler if it was suspended.
	 */
	for(iter=xfdashboard_window_tracker_get_windows(self); iter; iter=g_list_next(iter))
	{
		/* Get window */
		window=XFDASHBOARD_WINDOW_TRACKER_WINDOW(iter->data);
		if(!window) continue;

		/* If application was suspended disconnect signal handlers ... */
		if(priv->isAppSuspended)
		{
			g_signal_handlers_block_by_func(window, _xfdashboard_window_tracker_on_window_geometry_changed, self);
		}
			/* ... otherwise if application was resumed connect signals handlers
			 * and emit 'geometry-changed' signal to reflect latest changes of
			 * position and size of window.
			 */
			else
			{
				/* Connect signal handler */
				g_signal_connect_swapped(window,
											"geometry-changed",
											G_CALLBACK(_xfdashboard_window_tracker_on_window_geometry_changed),
											self);

				/* Call signal handler to reflect latest changes */
				_xfdashboard_window_tracker_on_window_geometry_changed(self, window);
			}
	}
}

/* IMPLEMENTATION: GObject */

/* Dispose this object */
static void _xfdashboard_window_tracker_dispose_free_monitor(gpointer inData, gpointer inUserData)
{
	XfdashboardWindowTracker				*self;
	XfdashboardWindowTrackerMonitor			*monitor;

	g_return_if_fail(XFDASHBOARD_IS_WINDOW_TRACKER_MONITOR(inData));
	g_return_if_fail(XFDASHBOARD_IS_WINDOW_TRACKER(inUserData));

	self=XFDASHBOARD_WINDOW_TRACKER(inUserData);
	monitor=XFDASHBOARD_WINDOW_TRACKER_MONITOR(inData);

	/* Unreference monitor */
	_xfdashboard_window_tracker_monitor_free(self, monitor);
}

static void _xfdashboard_window_tracker_dispose(GObject *inObject)
{
	XfdashboardWindowTracker				*self=XFDASHBOARD_WINDOW_TRACKER(inObject);
	XfdashboardWindowTrackerPrivate			*priv=self->priv;

	/* Dispose allocated resources */
	if(priv->suspendSignalID)
	{
		g_signal_handler_disconnect(xfdashboard_application_get_default(), priv->suspendSignalID);
		priv->suspendSignalID=0;
	}

	if(priv->primaryMonitor)
	{
		priv->primaryMonitor=NULL;
	}

	if(priv->monitors)
	{
		g_list_foreach(priv->monitors, _xfdashboard_window_tracker_dispose_free_monitor, self);
		g_list_free(priv->monitors);
		priv->monitors=NULL;
	}

	if(priv->gdkScreen)
	{
		g_signal_handlers_disconnect_by_data(priv->gdkScreen, self);
		priv->gdkScreen=NULL;
	}

	if(priv->screen)
	{
		/* TODO:
		 * - Disconnect from all windows
		 * - Disconnect from all workspaces
		 */
		g_signal_handlers_disconnect_by_data(priv->screen, self);
		priv->screen=NULL;
	}

	/* Call parent's class dispose method */
	G_OBJECT_CLASS(xfdashboard_window_tracker_parent_class)->dispose(inObject);
}

/* Set/get properties */
static void _xfdashboard_window_tracker_set_property(GObject *inObject,
														guint inPropID,
														const GValue *inValue,
														GParamSpec *inSpec)
{
	switch(inPropID)
	{
		default:
			G_OBJECT_WARN_INVALID_PROPERTY_ID(inObject, inPropID, inSpec);
			break;
	}
}

static void _xfdashboard_window_tracker_get_property(GObject *inObject,
														guint inPropID,
														GValue *outValue,
														GParamSpec *inSpec)
{
	XfdashboardWindowTracker			*self=XFDASHBOARD_WINDOW_TRACKER(inObject);

	switch(inPropID)
	{
		case PROP_ACTIVE_WINDOW:
			g_value_set_object(outValue, self->priv->activeWindow);
			break;

		case PROP_ACTIVE_WORKSPACE:
			g_value_set_object(outValue, self->priv->activeWorkspace);
			break;

		case PROP_PRIMARY_MONITOR:
			g_value_set_object(outValue, self->priv->primaryMonitor);
			break;

		default:
			G_OBJECT_WARN_INVALID_PROPERTY_ID(inObject, inPropID, inSpec);
			break;
	}
}

/* Class initialization
 * Override functions in parent classes and define properties
 * and signals
 */
void xfdashboard_window_tracker_class_init(XfdashboardWindowTrackerClass *klass)
{
	GObjectClass		*gobjectClass=G_OBJECT_CLASS(klass);

	/* Override functions */
	gobjectClass->dispose=_xfdashboard_window_tracker_dispose;
	gobjectClass->set_property=_xfdashboard_window_tracker_set_property;
	gobjectClass->get_property=_xfdashboard_window_tracker_get_property;

	/* Set up private structure */
	g_type_class_add_private(klass, sizeof(XfdashboardWindowTrackerPrivate));

	/* Define properties */
	XfdashboardWindowTrackerProperties[PROP_ACTIVE_WINDOW]=
		g_param_spec_object("active-window",
							_("Active window"),
							_("The current active window"),
							WNCK_TYPE_WINDOW,
							G_PARAM_READABLE | G_PARAM_STATIC_STRINGS);

	XfdashboardWindowTrackerProperties[PROP_ACTIVE_WORKSPACE]=
		g_param_spec_object("active-workspace",
							_("Active workspace"),
							_("The current active workspace"),
							WNCK_TYPE_WORKSPACE,
							G_PARAM_READABLE | G_PARAM_STATIC_STRINGS);

	XfdashboardWindowTrackerProperties[PROP_PRIMARY_MONITOR]=
		g_param_spec_object("primary-monitor",
							_("Primary monitor"),
							_("The current primary monitor"),
							XFDASHBOARD_TYPE_WINDOW_TRACKER_MONITOR,
							G_PARAM_READABLE | G_PARAM_STATIC_STRINGS);

	g_object_class_install_properties(gobjectClass, PROP_LAST, XfdashboardWindowTrackerProperties);

	/* Define signals */
	XfdashboardWindowTrackerSignals[SIGNAL_WINDOW_STACKING_CHANGED]=
		g_signal_new("window-stacking-changed",
						G_TYPE_FROM_CLASS(klass),
						G_SIGNAL_RUN_LAST,
						G_STRUCT_OFFSET(XfdashboardWindowTrackerClass, window_stacking_changed),
						NULL,
						NULL,
						g_cclosure_marshal_VOID__VOID,
						G_TYPE_NONE,
						0);

	XfdashboardWindowTrackerSignals[SIGNAL_ACTIVE_WINDOW_CHANGED]=
		g_signal_new("active-window-changed",
						G_TYPE_FROM_CLASS(klass),
						G_SIGNAL_RUN_LAST,
						G_STRUCT_OFFSET(XfdashboardWindowTrackerClass, active_window_changed),
						NULL,
						NULL,
						_xfdashboard_marshal_VOID__OBJECT_OBJECT,
						G_TYPE_NONE,
						2,
						WNCK_TYPE_WINDOW,
						WNCK_TYPE_WINDOW);

	XfdashboardWindowTrackerSignals[SIGNAL_WINDOW_OPENED]=
		g_signal_new("window-opened",
						G_TYPE_FROM_CLASS(klass),
						G_SIGNAL_RUN_LAST,
						G_STRUCT_OFFSET(XfdashboardWindowTrackerClass, window_opened),
						NULL,
						NULL,
						g_cclosure_marshal_VOID__OBJECT,
						G_TYPE_NONE,
						1,
						WNCK_TYPE_WINDOW);

	XfdashboardWindowTrackerSignals[SIGNAL_WINDOW_CLOSED]=
		g_signal_new("window-closed",
						G_TYPE_FROM_CLASS(klass),
						G_SIGNAL_RUN_LAST,
						G_STRUCT_OFFSET(XfdashboardWindowTrackerClass, window_closed),
						NULL,
						NULL,
						g_cclosure_marshal_VOID__OBJECT,
						G_TYPE_NONE,
						1,
						WNCK_TYPE_WINDOW);

	XfdashboardWindowTrackerSignals[SIGNAL_WINDOW_GEOMETRY_CHANGED]=
		g_signal_new("window-geometry-changed",
						G_TYPE_FROM_CLASS(klass),
						G_SIGNAL_RUN_LAST,
						G_STRUCT_OFFSET(XfdashboardWindowTrackerClass, window_geometry_changed),
						NULL,
						NULL,
						g_cclosure_marshal_VOID__OBJECT,
						G_TYPE_NONE,
						1,
						WNCK_TYPE_WINDOW);

	XfdashboardWindowTrackerSignals[SIGNAL_WINDOW_ACTIONS_CHANGED]=
		g_signal_new("window-actions-changed",
						G_TYPE_FROM_CLASS(klass),
						G_SIGNAL_RUN_LAST,
						G_STRUCT_OFFSET(XfdashboardWindowTrackerClass, window_actions_changed),
						NULL,
						NULL,
						g_cclosure_marshal_VOID__OBJECT,
						G_TYPE_NONE,
						1,
						WNCK_TYPE_WINDOW);

	XfdashboardWindowTrackerSignals[SIGNAL_WINDOW_STATE_CHANGED]=
		g_signal_new("window-state-changed",
						G_TYPE_FROM_CLASS(klass),
						G_SIGNAL_RUN_LAST,
						G_STRUCT_OFFSET(XfdashboardWindowTrackerClass, window_state_changed),
						NULL,
						NULL,
						g_cclosure_marshal_VOID__OBJECT,
						G_TYPE_NONE,
						1,
						WNCK_TYPE_WINDOW);

	XfdashboardWindowTrackerSignals[SIGNAL_WINDOW_ICON_CHANGED]=
		g_signal_new("window-icon-changed",
						G_TYPE_FROM_CLASS(klass),
						G_SIGNAL_RUN_LAST,
						G_STRUCT_OFFSET(XfdashboardWindowTrackerClass, window_icon_changed),
						NULL,
						NULL,
						g_cclosure_marshal_VOID__OBJECT,
						G_TYPE_NONE,
						1,
						WNCK_TYPE_WINDOW);

	XfdashboardWindowTrackerSignals[SIGNAL_WINDOW_NAME_CHANGED]=
		g_signal_new("window-name-changed",
						G_TYPE_FROM_CLASS(klass),
						G_SIGNAL_RUN_LAST,
						G_STRUCT_OFFSET(XfdashboardWindowTrackerClass, window_name_changed),
						NULL,
						NULL,
						g_cclosure_marshal_VOID__OBJECT,
						G_TYPE_NONE,
						1,
						WNCK_TYPE_WINDOW);

	XfdashboardWindowTrackerSignals[SIGNAL_WINDOW_WORKSPACE_CHANGED]=
		g_signal_new("window-workspace-changed",
						G_TYPE_FROM_CLASS(klass),
						G_SIGNAL_RUN_LAST,
						G_STRUCT_OFFSET(XfdashboardWindowTrackerClass, window_workspace_changed),
						NULL,
						NULL,
						_xfdashboard_marshal_VOID__OBJECT_OBJECT,
						G_TYPE_NONE,
						2,
						WNCK_TYPE_WINDOW,
						WNCK_TYPE_WORKSPACE);

	XfdashboardWindowTrackerSignals[SIGNAL_WINDOW_MONITOR_CHANGED]=
		g_signal_new("window-monitor-changed",
						G_TYPE_FROM_CLASS(klass),
						G_SIGNAL_RUN_LAST,
						G_STRUCT_OFFSET(XfdashboardWindowTrackerClass, window_monitor_changed),
						NULL,
						NULL,
						_xfdashboard_marshal_VOID__OBJECT_OBJECT_OBJECT,
						G_TYPE_NONE,
						3,
						WNCK_TYPE_WINDOW,
						XFDASHBOARD_TYPE_WINDOW_TRACKER_MONITOR,
						XFDASHBOARD_TYPE_WINDOW_TRACKER_MONITOR);

	XfdashboardWindowTrackerSignals[SIGNAL_ACTIVE_WORKSPACE_CHANGED]=
		g_signal_new("active-workspace-changed",
						G_TYPE_FROM_CLASS(klass),
						G_SIGNAL_RUN_LAST,
						G_STRUCT_OFFSET(XfdashboardWindowTrackerClass, active_workspace_changed),
						NULL,
						NULL,
						_xfdashboard_marshal_VOID__OBJECT_OBJECT,
						G_TYPE_NONE,
						2,
						WNCK_TYPE_WORKSPACE,
						WNCK_TYPE_WORKSPACE);

	XfdashboardWindowTrackerSignals[SIGNAL_WORKSPACE_ADDED]=
		g_signal_new("workspace-added",
						G_TYPE_FROM_CLASS(klass),
						G_SIGNAL_RUN_LAST,
						G_STRUCT_OFFSET(XfdashboardWindowTrackerClass, workspace_added),
						NULL,
						NULL,
						g_cclosure_marshal_VOID__OBJECT,
						G_TYPE_NONE,
						1,
						WNCK_TYPE_WORKSPACE);

	XfdashboardWindowTrackerSignals[SIGNAL_WORKSPACE_REMOVED]=
		g_signal_new("workspace-removed",
						G_TYPE_FROM_CLASS(klass),
						G_SIGNAL_RUN_LAST,
						G_STRUCT_OFFSET(XfdashboardWindowTrackerClass, workspace_removed),
						NULL,
						NULL,
						g_cclosure_marshal_VOID__OBJECT,
						G_TYPE_NONE,
						1,
						WNCK_TYPE_WORKSPACE);

	XfdashboardWindowTrackerSignals[SIGNAL_WORKSPACE_NAME_CHANGED]=
		g_signal_new("workspace-name-changed",
						G_TYPE_FROM_CLASS(klass),
						G_SIGNAL_RUN_LAST,
						G_STRUCT_OFFSET(XfdashboardWindowTrackerClass, workspace_name_changed),
						NULL,
						NULL,
						g_cclosure_marshal_VOID__OBJECT,
						G_TYPE_NONE,
						1,
						WNCK_TYPE_WORKSPACE);

	XfdashboardWindowTrackerSignals[SIGNAL_PRIMARY_MONITOR_CHANGED]=
		g_signal_new("primary-monitor-changed",
						G_TYPE_FROM_CLASS(klass),
						G_SIGNAL_RUN_LAST,
						G_STRUCT_OFFSET(XfdashboardWindowTrackerClass, primary_monitor_changed),
						NULL,
						NULL,
						_xfdashboard_marshal_VOID__OBJECT_OBJECT,
						G_TYPE_NONE,
						2,
						XFDASHBOARD_TYPE_WINDOW_TRACKER_MONITOR,
						XFDASHBOARD_TYPE_WINDOW_TRACKER_MONITOR);

	XfdashboardWindowTrackerSignals[SIGNAL_MONITOR_ADDED]=
		g_signal_new("monitor-added",
						G_TYPE_FROM_CLASS(klass),
						G_SIGNAL_RUN_LAST,
						G_STRUCT_OFFSET(XfdashboardWindowTrackerClass, monitor_added),
						NULL,
						NULL,
						g_cclosure_marshal_VOID__OBJECT,
						G_TYPE_NONE,
						1,
						XFDASHBOARD_TYPE_WINDOW_TRACKER_MONITOR);

	XfdashboardWindowTrackerSignals[SIGNAL_MONITOR_REMOVED]=
		g_signal_new("monitor-removed",
						G_TYPE_FROM_CLASS(klass),
						G_SIGNAL_RUN_LAST,
						G_STRUCT_OFFSET(XfdashboardWindowTrackerClass, monitor_removed),
						NULL,
						NULL,
						g_cclosure_marshal_VOID__OBJECT,
						G_TYPE_NONE,
						1,
						XFDASHBOARD_TYPE_WINDOW_TRACKER_MONITOR);

	XfdashboardWindowTrackerSignals[SIGNAL_MONITOR_GEOMETRY_CHANGED]=
		g_signal_new("monitor-geometry-changed",
						G_TYPE_FROM_CLASS(klass),
						G_SIGNAL_RUN_LAST,
						G_STRUCT_OFFSET(XfdashboardWindowTrackerClass, monitor_geometry_changed),
						NULL,
						NULL,
						g_cclosure_marshal_VOID__OBJECT,
						G_TYPE_NONE,
						1,
						XFDASHBOARD_TYPE_WINDOW_TRACKER_MONITOR);

	XfdashboardWindowTrackerSignals[SIGNAL_SCREEN_SIZE_CHANGED]=
		g_signal_new("screen-size-changed",
						G_TYPE_FROM_CLASS(klass),
						G_SIGNAL_RUN_LAST,
						G_STRUCT_OFFSET(XfdashboardWindowTrackerClass, screen_size_changed),
						NULL,
						NULL,
						_xfdashboard_marshal_VOID__INT_INT,
						G_TYPE_NONE,
						2,
						G_TYPE_INT,
						G_TYPE_INT);
}

/* Object initialization
 * Create private structure and set up default values
 */
void xfdashboard_window_tracker_init(XfdashboardWindowTracker *self)
{
	XfdashboardWindowTrackerPrivate			*priv;
	XfdashboardApplication					*app;

	priv=self->priv=XFDASHBOARD_WINDOW_TRACKER_GET_PRIVATE(self);

	g_debug("Initializing window tracker");

	/* Set default values */
	priv->screen=wnck_screen_get_default();
	priv->gdkScreen=gdk_screen_get_default();
	priv->activeWindow=NULL;
	priv->activeWorkspace=NULL;
	priv->primaryMonitor=NULL;
	priv->monitors=NULL;
	priv->supportsMultipleMonitors=FALSE;

	/* The very first call to libwnck should be setting the client type */
	wnck_set_client_type(WNCK_CLIENT_TYPE_PAGER);

	/* Connect signals to screen */
	g_signal_connect_swapped(priv->screen,
								"window-stacking-changed",
								G_CALLBACK(_xfdashboard_window_tracker_on_window_stacking_changed),
								self);

	g_signal_connect_swapped(priv->screen,
								"window-closed",
								G_CALLBACK(_xfdashboard_window_tracker_on_window_closed),
								self);
	g_signal_connect_swapped(priv->screen,
								"window-opened",
								G_CALLBACK(_xfdashboard_window_tracker_on_window_opened),
								self);
	g_signal_connect_swapped(priv->screen,
								"active-window-changed",
								G_CALLBACK(_xfdashboard_window_tracker_on_active_window_changed),
								self);

	g_signal_connect_swapped(priv->screen,
								"workspace-destroyed",
								G_CALLBACK(_xfdashboard_window_tracker_on_workspace_destroyed),
								self);
	g_signal_connect_swapped(priv->screen,
								"workspace-created",
								G_CALLBACK(_xfdashboard_window_tracker_on_workspace_created),
								self);
	g_signal_connect_swapped(priv->screen,
								"active-workspace-changed",
								G_CALLBACK(_xfdashboard_window_tracker_on_active_workspace_changed),
								self);

	g_signal_connect_swapped(priv->gdkScreen,
								"size-changed",
								G_CALLBACK(_xfdashboard_window_tracker_on_screen_size_changed),
								self);

#ifdef HAVE_XINERAMA
	/* Check if multiple monitors are supported */
	if(XineramaIsActive(GDK_SCREEN_XDISPLAY(priv->gdkScreen)))
	{
		XfdashboardWindowTrackerMonitor		*monitor;
		gint								i;

		/* Set flag that multiple monitors are supported */
		priv->supportsMultipleMonitors=TRUE;

		/* This signal must be called at least after the default signal handler - best at last.
		 * The reason is that all other signal handler should been processed because this handler
		 * could destroy monitor instances even the one of the primary monitor if it was not
		 * handled before. So give the other signal handlers a chance ;)
		 */
		g_signal_connect_data(priv->gdkScreen,
								"monitors-changed",
								G_CALLBACK(_xfdashboard_window_tracker_on_monitors_changed),
								self,
								NULL,
								G_CONNECT_AFTER | G_CONNECT_SWAPPED);

		/* Get monitors */
		for(i=0; i<gdk_screen_get_n_monitors(priv->gdkScreen); i++)
		{
			/* Create monitor object */
			monitor=_xfdashboard_window_tracker_monitor_new(self, i);

			/* Remember primary monitor */
			if(xfdashboard_window_tracker_monitor_is_primary(monitor))
			{
				priv->primaryMonitor=monitor;
			}
		}
	}
#endif

	/* Handle suspension signals from application */
	app=xfdashboard_application_get_default();
	priv->suspendSignalID=g_signal_connect_swapped(app,
													"notify::is-suspended",
													G_CALLBACK(_xfdashboard_window_tracker_on_application_suspended_changed),
													self);
	priv->isAppSuspended=xfdashboard_application_is_suspended(app);
}

/* IMPLEMENTATION: Public API */

/* Create new instance */
XfdashboardWindowTracker* xfdashboard_window_tracker_get_default(void)
{
	if(G_UNLIKELY(_xfdashboard_window_tracker_singleton==NULL))
	{
		_xfdashboard_window_tracker_singleton=
			XFDASHBOARD_WINDOW_TRACKER(g_object_new(XFDASHBOARD_TYPE_WINDOW_TRACKER, NULL));
	}
		else g_object_ref(_xfdashboard_window_tracker_singleton);

	return(_xfdashboard_window_tracker_singleton);

}

/* Get last timestamp for use in libwnck */
guint32 xfdashboard_window_tracker_get_time(void)
{
	const ClutterEvent		*currentClutterEvent;
	guint32					timestamp;
	GdkDisplay				*display;
	GdkWindow				*window;
	GdkEventMask			eventMask;
	GSList					*stages, *entry;
	ClutterStage			*stage;

	/* We don't use clutter_get_current_event_time as it can return
	 * a too old timestamp if there is no current event.
	 */
	currentClutterEvent=clutter_get_current_event();
	if(currentClutterEvent!=NULL) return(clutter_event_get_time(currentClutterEvent));

	/* Next we try timestamp of last GTK+ event */
	timestamp=gtk_get_current_event_time();
	if(timestamp>0) return(timestamp);

	/* Next we try to ask GDK for a timestamp */
	timestamp=gdk_x11_display_get_user_time(gdk_display_get_default());
	if(timestamp>0) return(timestamp);

	/* Next we try to retrieve timestamp of last X11 event in clutter */
	g_debug("No timestamp for windows - trying timestamp of last X11 event in Clutter");
	timestamp=(guint32)clutter_x11_get_current_event_time();
	if(timestamp!=0)
	{
		g_debug("Got timestamp %u of last X11 event in Clutter", timestamp);
		return(timestamp);
	}

	/* Last resort is to get X11 server time via stage windows */
	g_debug("No timestamp for windows - trying last resort via stage windows");

	display=gdk_display_get_default();
	if(!display)
	{
		g_debug("No default display found in GDK to get timestamp for windows");
		return(0);
	}

	/* Iterate through stages, get their GDK window and try to retrieve timestamp */
	timestamp=0;
	stages=clutter_stage_manager_list_stages(clutter_stage_manager_get_default());
	for(entry=stages; timestamp==0 && entry; entry=g_slist_next(entry))
	{
		/* Get stage */
		stage=CLUTTER_STAGE(entry->data);
		if(stage)
		{
			/* Get GDK window of stage */
			window=gdk_x11_window_lookup_for_display(display, clutter_x11_get_stage_window(stage));
			if(!window)
			{
				g_debug("No GDK window found for stage %p to get timestamp for windows", stage);
				continue;
			}

			/* Check if GDK window supports GDK_PROPERTY_CHANGE_MASK event
			 * or application (or worst X server) will hang
			 */
			eventMask=gdk_window_get_events(window);
			if(!(eventMask & GDK_PROPERTY_CHANGE_MASK))
			{
				g_debug("GDK window %p for stage %p does not support GDK_PROPERTY_CHANGE_MASK to get timestamp for windows",
							window, stage);
				continue;
			}

			timestamp=gdk_x11_get_server_time(window);
		}
	}
	g_slist_free(stages);

	/* Return timestamp of last resort */
	g_debug("Last resort timestamp for windows %s (%u)", timestamp ? "found" : "not found", timestamp);
	return(timestamp);
}

/* Get list of all windows (if wanted in stack order) */
GList* xfdashboard_window_tracker_get_windows(XfdashboardWindowTracker *self)
{
	XfdashboardWindowTrackerPrivate		*priv;

	g_return_val_if_fail(XFDASHBOARD_IS_WINDOW_TRACKER(self), NULL);

	priv=self->priv;

	/* Return list of window (should be in order as they were opened) */
	return(wnck_screen_get_windows(priv->screen));
}

GList* xfdashboard_window_tracker_get_windows_stacked(XfdashboardWindowTracker *self)
{
	XfdashboardWindowTrackerPrivate		*priv;

	g_return_val_if_fail(XFDASHBOARD_IS_WINDOW_TRACKER(self), NULL);

	priv=self->priv;

	/* Return list of window in stack order */
	return(wnck_screen_get_windows_stacked(priv->screen));
}

/* Get active window */
XfdashboardWindowTrackerWindow* xfdashboard_window_tracker_get_active_window(XfdashboardWindowTracker *self)
{
	g_return_val_if_fail(XFDASHBOARD_IS_WINDOW_TRACKER(self), NULL);

	return(self->priv->activeWindow);
}

/* Get number of workspaces */
gint xfdashboard_window_tracker_get_workspaces_count(XfdashboardWindowTracker *self)
{
	g_return_val_if_fail(XFDASHBOARD_IS_WINDOW_TRACKER(self), 0);

	/* Return number of workspaces */
	return(wnck_screen_get_workspace_count(self->priv->screen));
}

/* Get list of workspaces */
GList* xfdashboard_window_tracker_get_workspaces(XfdashboardWindowTracker *self)
{
	g_return_val_if_fail(XFDASHBOARD_IS_WINDOW_TRACKER(self), NULL);

	/* Return list of workspaces */
	return(wnck_screen_get_workspaces(self->priv->screen));
}

/* Get workspace by number */
XfdashboardWindowTrackerWorkspace* xfdashboard_window_tracker_get_workspace_by_number(XfdashboardWindowTracker *self,
																						gint inNumber)
{
	g_return_val_if_fail(XFDASHBOARD_IS_WINDOW_TRACKER(self), NULL);
	g_return_val_if_fail(inNumber>=0 && inNumber<wnck_screen_get_workspace_count(self->priv->screen), NULL);

	/* Return list of workspaces */
	return(wnck_screen_get_workspace(self->priv->screen, inNumber));
}

/* Get active workspace */
XfdashboardWindowTrackerWorkspace* xfdashboard_window_tracker_get_active_workspace(XfdashboardWindowTracker *self)
{
	g_return_val_if_fail(XFDASHBOARD_IS_WINDOW_TRACKER(self), NULL);

	return(self->priv->activeWorkspace);
}

/* Determine if multiple monitors are supported */
gboolean xfdashboard_window_tracker_supports_multiple_monitors(XfdashboardWindowTracker *self)
{
	g_return_val_if_fail(XFDASHBOARD_IS_WINDOW_TRACKER(self), FALSE);

	return(self->priv->supportsMultipleMonitors);
}

/* Get number of monitors */
gint xfdashboard_window_tracker_get_monitors_count(XfdashboardWindowTracker *self)
{
	g_return_val_if_fail(XFDASHBOARD_IS_WINDOW_TRACKER(self), 0);

	/* Return number of monitors */
	return(g_list_length(self->priv->monitors));
}

/* Get list of monitors */
GList* xfdashboard_window_tracker_get_monitors(XfdashboardWindowTracker *self)
{
	g_return_val_if_fail(XFDASHBOARD_IS_WINDOW_TRACKER(self), NULL);

	/* Return list of workspaces */
	return(self->priv->monitors);
}

/* Get monitor by number */
XfdashboardWindowTrackerMonitor* xfdashboard_window_tracker_get_monitor_by_number(XfdashboardWindowTracker *self,
																					gint inNumber)
{
	g_return_val_if_fail(XFDASHBOARD_IS_WINDOW_TRACKER(self), NULL);
	g_return_val_if_fail(inNumber>=0, NULL);
	g_return_val_if_fail(((guint)inNumber)<g_list_length(self->priv->monitors), NULL);

	/* Return monitor at index */
	return(g_list_nth_data(self->priv->monitors, inNumber));
}

/* Get primary monitor */
XfdashboardWindowTrackerMonitor* xfdashboard_window_tracker_get_primary_monitor(XfdashboardWindowTracker *self)
{
	g_return_val_if_fail(XFDASHBOARD_IS_WINDOW_TRACKER(self), NULL);

	return(self->priv->primaryMonitor);
}

/* Get width of screen */
gint xfdashboard_window_tracker_get_screen_width(XfdashboardWindowTracker *self)
{
	g_return_val_if_fail(XFDASHBOARD_IS_WINDOW_TRACKER(self), 0);

	return(gdk_screen_get_width(self->priv->gdkScreen));
}

/* Get width of screen */
gint xfdashboard_window_tracker_get_screen_height(XfdashboardWindowTracker *self)
{
	g_return_val_if_fail(XFDASHBOARD_IS_WINDOW_TRACKER(self), 0);

	return(gdk_screen_get_height(self->priv->gdkScreen));
}

/* Get root (desktop) window */
XfdashboardWindowTrackerWindow* xfdashboard_window_tracker_get_root_window(XfdashboardWindowTracker *self)
{
	XfdashboardWindowTrackerPrivate		*priv;
	gulong								backgroundWindowID;
	GList								*windows;

	g_return_val_if_fail(XFDASHBOARD_IS_WINDOW_TRACKER(self), NULL);

	priv=self->priv;

	/* Find and return root window (the desktop) by known ID */
	backgroundWindowID=wnck_screen_get_background_pixmap(priv->screen);
	if(backgroundWindowID)
	{
		WnckWindow						*backgroundWindow;

		backgroundWindow=wnck_window_get(backgroundWindowID);
		if(backgroundWindow)
		{
			g_debug("Found desktop window by known background pixmap ID");
			return(XFDASHBOARD_WINDOW_TRACKER_WINDOW(backgroundWindow));
		}
	}

	/* Either there was no known ID for the root window or the root window
	 * could not be found (happened a lot when running in daemon mode).
	 * So iterate through list of all known windows and lookup window of
	 * type 'desktop'.
	 */
	for(windows=wnck_screen_get_windows(priv->screen); windows; windows=g_list_next(windows))
	{
		WnckWindow					*window;
		WnckWindowType				windowType;

		window=(WnckWindow*)windows->data;
		windowType=wnck_window_get_window_type(window);
		if(windowType==WNCK_WINDOW_DESKTOP)
		{
			g_debug("Desktop window ID found while iterating through window list");
				return(XFDASHBOARD_WINDOW_TRACKER_WINDOW(window));
		}
	}

	/* If we get here either desktop window does not exist or is not known
	 * in window list. So return NULL here.
	 */
	g_debug("Desktop window could not be found");
	return(NULL);
}
