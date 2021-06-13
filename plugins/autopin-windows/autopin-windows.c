/*
 * autopin-windows: Pins or unpins windows depending on the monitor
 *                  it is located
 * 
 * Copyright 2012-2021 Stephan Haller <nomad@froevel.de>
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

#include "autopin-windows.h"

#include <libxfdashboard/libxfdashboard.h>
#include <glib/gi18n-lib.h>
#include <gtk/gtk.h>
#include <math.h>


/* Define this class in GObject system */
struct _XfdashboardAutopinWindowsPrivate
{
	/* Instance related */
	XfdashboardWindowTracker				*windowTracker;
	guint									windowOpenedSignaledID;
	guint									windowMonitorChangedSignalID;

	gboolean								unpinOnDispose;
	GSList									*pinnedWindows;
};

G_DEFINE_DYNAMIC_TYPE_EXTENDED(XfdashboardAutopinWindows,
								xfdashboard_autopin_windows,
								G_TYPE_OBJECT,
								0,
								G_ADD_PRIVATE_DYNAMIC(XfdashboardAutopinWindows))

/* Define this class in this plugin */
XFDASHBOARD_DEFINE_PLUGIN_TYPE(xfdashboard_autopin_windows);


/* IMPLEMENTATION: Private variables and methods */

/* Update pin state of window depending on monitor it is located on */
static void _xfdashboard_autopin_windows_update_window_pin_state(XfdashboardAutopinWindows *self, XfdashboardWindowTrackerWindow *inWindow)
{
	XfdashboardAutopinWindowsPrivate		*priv;
	XfdashboardWindowTrackerMonitor			*currentMonitor;
	gboolean								isPrimary;
	XfdashboardWindowTrackerWindowState		windowState;

	g_return_if_fail(XFDASHBOARD_IS_AUTOPIN_WINDOWS(self));
	g_return_if_fail(XFDASHBOARD_IS_WINDOW_TRACKER_WINDOW(inWindow));

	priv=self->priv;

	/* Get monitor the window is located on */
	currentMonitor=xfdashboard_window_tracker_window_get_monitor(inWindow);
	if(!currentMonitor)
	{
		XFDASHBOARD_DEBUG(self, PLUGIN,
							"Skipping window '%s' because we could not get monitor",
							xfdashboard_window_tracker_window_get_name(inWindow));
		return;
	}

	/* Get primary state of new monitor and window state */
	isPrimary=xfdashboard_window_tracker_monitor_is_primary(currentMonitor);
	windowState=xfdashboard_window_tracker_window_get_state(inWindow);
	XFDASHBOARD_DEBUG(self, PLUGIN,
						"Window '%s' is on %s monitor with state %u (%s)",
						xfdashboard_window_tracker_window_get_name(inWindow),
						isPrimary ? "primary" : "non-primary",
						windowState,
						(windowState & XFDASHBOARD_WINDOW_TRACKER_WINDOW_STATE_PINNED) ? "pinned" : "unpinned");

	/* Check if window is a "normal" window which could be pinned by user or
	 * this plugin. This depends on its state and role (meaning it is not a
	 * stage window).
	 */
	if(windowState &
		(XFDASHBOARD_WINDOW_TRACKER_WINDOW_STATE_SKIP_PAGER | XFDASHBOARD_WINDOW_TRACKER_WINDOW_STATE_SKIP_TASKLIST))
	{
		XFDASHBOARD_DEBUG(self, PLUGIN,
							"Skipping window '%s' because it is skipped from pager and/or tasklist",
							xfdashboard_window_tracker_window_get_name(inWindow));
		return;
	}

	if(xfdashboard_window_tracker_window_is_stage(inWindow))
	{
		XFDASHBOARD_DEBUG(self, PLUGIN,
							"Skipping window '%s' because it is the stage window",
							xfdashboard_window_tracker_window_get_name(inWindow));
		return;
	}

	/* Pin window if moved to non-primary monitor and is not pinned yet or unpin
	 * window if moved to primary monitor and is not unpinned yet. Otherwise
	 * keep window untouched.
	 */
	if(isPrimary &&
		(windowState & XFDASHBOARD_WINDOW_TRACKER_WINDOW_STATE_PINNED))
	{
		XFDASHBOARD_DEBUG(self, PLUGIN,
							"Unpinning window '%s' as it is located on primary monitor",
							xfdashboard_window_tracker_window_get_name(inWindow));

		windowState=windowState & ~XFDASHBOARD_WINDOW_TRACKER_WINDOW_STATE_PINNED;
		xfdashboard_window_tracker_window_set_state(inWindow, windowState);

		/* Remember that we pinned this window, so we can unpin it on dispose of
		 * this object instance.
		 */
		priv->pinnedWindows=g_slist_prepend(priv->pinnedWindows, inWindow);
	}
		else if(!isPrimary &&
				!(windowState & XFDASHBOARD_WINDOW_TRACKER_WINDOW_STATE_PINNED))
		{
			XFDASHBOARD_DEBUG(self, PLUGIN,
								"Pinning window '%s' as it is located on non-primary monitor",
								xfdashboard_window_tracker_window_get_name(inWindow));

			windowState=windowState | XFDASHBOARD_WINDOW_TRACKER_WINDOW_STATE_PINNED;
			xfdashboard_window_tracker_window_set_state(inWindow, windowState);

			/* Forget this window as it is unpinned if we remebered it */
			priv->pinnedWindows=g_slist_remove(priv->pinnedWindows, inWindow);
		}
}

/* Callback for windows moved to another monitor */
static void _xfdashboard_autopin_windows_on_window_monitor_changed(XfdashboardAutopinWindows *self,
																	XfdashboardWindowTrackerWindow *inWindow,
																	XfdashboardWindowTrackerMonitor *inOldMonitor,
																	XfdashboardWindowTrackerMonitor *inNewMonitor,
																	gpointer inUserData)
{

	g_return_if_fail(XFDASHBOARD_IS_AUTOPIN_WINDOWS(self));
	g_return_if_fail(XFDASHBOARD_IS_WINDOW_TRACKER_WINDOW(inWindow));
	g_return_if_fail(XFDASHBOARD_IS_WINDOW_TRACKER_MONITOR(inNewMonitor));

	/* Check moved window if it needs to be pinned or unpinned */
	XFDASHBOARD_DEBUG(self, PLUGIN,
						"Window '%s' with state %u (%s) moved from monitor %d (%s) to %d (%s) and needs to be %s",
						xfdashboard_window_tracker_window_get_name(inWindow),
						xfdashboard_window_tracker_window_get_state(inWindow),
						(xfdashboard_window_tracker_window_get_state(inWindow) & XFDASHBOARD_WINDOW_TRACKER_WINDOW_STATE_PINNED) ? "pinned" : "unpinned",
						inOldMonitor ? xfdashboard_window_tracker_monitor_get_number(inOldMonitor) : -1,
						(inOldMonitor && xfdashboard_window_tracker_monitor_is_primary(inOldMonitor)) ? "primary" : "non-primary",
						inNewMonitor ? xfdashboard_window_tracker_monitor_get_number(inNewMonitor) : -1,
						(inNewMonitor && xfdashboard_window_tracker_monitor_is_primary(inNewMonitor)) ? "primary" : "non-primary",
						(inNewMonitor && xfdashboard_window_tracker_monitor_is_primary(inNewMonitor)) ? "unpinned" : "pinned");

	_xfdashboard_autopin_windows_update_window_pin_state(self, inWindow);
}

/* Callback for new windows opened */
static void _xfdashboard_autopin_windows_on_window_opened(XfdashboardAutopinWindows *self,
															XfdashboardWindowTrackerWindow *inWindow,
															gpointer inUserData)
{
	g_return_if_fail(XFDASHBOARD_IS_AUTOPIN_WINDOWS(self));
	g_return_if_fail(XFDASHBOARD_IS_WINDOW_TRACKER_WINDOW(inWindow));

	/* Check window if newly opened window needs to be pinned or unpinned */
	XFDASHBOARD_DEBUG(self, PLUGIN,
						"Window '%s' was opened, checking pin state",
						xfdashboard_window_tracker_window_get_name(inWindow));
	_xfdashboard_autopin_windows_update_window_pin_state(self, inWindow);
}

/* Callback for windows closed */
static void _xfdashboard_autopin_windows_on_window_closed(XfdashboardAutopinWindows *self,
															XfdashboardWindowTrackerWindow *inWindow,
															gpointer inUserData)
{
	XfdashboardAutopinWindowsPrivate		*priv;

	g_return_if_fail(XFDASHBOARD_IS_AUTOPIN_WINDOWS(self));
	g_return_if_fail(XFDASHBOARD_IS_WINDOW_TRACKER_WINDOW(inWindow));

	priv=self->priv;

	/* Forget this window as it was closed so we cannot unping it when this
	 * object will be destroyed.
	 */
	XFDASHBOARD_DEBUG(self, PLUGIN,
						"Forget window '%s' which was closed",
						xfdashboard_window_tracker_window_get_name(inWindow));
	priv->pinnedWindows=g_slist_remove(priv->pinnedWindows, inWindow);
}


/* IMPLEMENTATION: GObject */

/* Dispose this object */
static void _xfdashboard_autopin_windows_dispose(GObject *inObject)
{
	XfdashboardAutopinWindows				*self=XFDASHBOARD_AUTOPIN_WINDOWS(inObject);
	XfdashboardAutopinWindowsPrivate		*priv=self->priv;
	GSList									*windowIter;
	XfdashboardWindowTrackerWindow			*window;
	XfdashboardWindowTrackerWindowState		windowState;

	/* Iterate through all windows and unpin them as this plugin will be disposed */
	if(priv->unpinOnDispose && priv->pinnedWindows)
	{
		for(windowIter=priv->pinnedWindows; windowIter; windowIter=g_slist_next(windowIter))
		{
			/* Get window from iterator */
			window=XFDASHBOARD_WINDOW_TRACKER_WINDOW(windowIter->data);
			if(!window) continue;

			/* Unpinned window */
			windowState=xfdashboard_window_tracker_window_get_state(window);
			xfdashboard_window_tracker_window_set_state(window, windowState & ~XFDASHBOARD_WINDOW_TRACKER_WINDOW_STATE_PINNED);
			XFDASHBOARD_DEBUG(self, PLUGIN,
								"Unpinned window '%s' because it was pinned by us and this plugin is shut down",
								xfdashboard_window_tracker_window_get_name(inWindow));
		}

		/* Free and release list */
		g_slist_free(priv->pinnedWindows);
		priv->pinnedWindows=NULL;
	}

	/* Release allocated resources */
	if(priv->windowTracker)
	{
		/* Disconnect signals from window tracker */
		if(priv->windowMonitorChangedSignalID)
		{
			g_signal_handler_disconnect(priv->windowTracker, priv->windowMonitorChangedSignalID);
			priv->windowMonitorChangedSignalID=0;
		}

		if(priv->windowOpenedSignaledID)
		{
			g_signal_handler_disconnect(priv->windowTracker, priv->windowOpenedSignaledID);
			priv->windowOpenedSignaledID=0;
		}

		/* Release window tracker */
		g_object_unref(priv->windowTracker);
		priv->windowTracker=NULL;
	}

	/* Call parent's class dispose method */
	G_OBJECT_CLASS(xfdashboard_autopin_windows_parent_class)->dispose(inObject);
}

/* Class initialization
 * Override functions in parent classes and define properties
 * and signals
 */
void xfdashboard_autopin_windows_class_init(XfdashboardAutopinWindowsClass *klass)
{
	GObjectClass			*gobjectClass=G_OBJECT_CLASS(klass);

	/* Override functions */
	gobjectClass->dispose=_xfdashboard_autopin_windows_dispose;
}

/* Class finalization */
void xfdashboard_autopin_windows_class_finalize(XfdashboardAutopinWindowsClass *klass)
{
}

/* Object initialization
 * Create private structure and set up default values
 */
void xfdashboard_autopin_windows_init(XfdashboardAutopinWindows *self)
{
	XfdashboardAutopinWindowsPrivate		*priv;
	GList									*windowList;
	XfdashboardWindowTrackerWindow			*window;

	self->priv=priv=xfdashboard_autopin_windows_get_instance_private(self);

	/* Set up default values */
	priv->windowTracker=xfdashboard_core_get_window_tracker(NULL);
	priv->windowMonitorChangedSignalID=0;
	priv->windowOpenedSignaledID=0;
	priv->unpinOnDispose=TRUE;
	priv->pinnedWindows=NULL;

	/* Iterate through all windows and pin or unpin them depending on which
	 * monitor they are located at.
	 */
	XFDASHBOARD_DEBUG(self, PLUGIN,
						"Initializing plugin class %s so iterate through active window list",
						G_OBJECT_TYPE_NAME(self));
	windowList=xfdashboard_window_tracker_get_windows(priv->windowTracker);
	for(; windowList; windowList=g_list_next(windowList))
	{
		/* Get window from iterator */
		window=XFDASHBOARD_WINDOW_TRACKER_WINDOW(windowList->data);
		if(!window) continue;

		/* Check window if it needs to be pinned or unpinned */
		_xfdashboard_autopin_windows_update_window_pin_state(self, window);
	}
	XFDASHBOARD_DEBUG(self, PLUGIN,
						"Initialization of plugin class %s completed",
						G_OBJECT_TYPE_NAME(self));

	/* Connect signal to get notified about actor creations  and filter out
	 * and set up the ones we are interested in.
	 */
	priv->windowMonitorChangedSignalID=
		g_signal_connect_swapped(priv->windowTracker,
									"window-monitor-changed",
									G_CALLBACK(_xfdashboard_autopin_windows_on_window_monitor_changed),
									self);

	priv->windowOpenedSignaledID=
		g_signal_connect_swapped(priv->windowTracker,
									"window-opened",
									G_CALLBACK(_xfdashboard_autopin_windows_on_window_opened),
									self);

	priv->windowOpenedSignaledID=
		g_signal_connect_swapped(priv->windowTracker,
									"window-closed",
									G_CALLBACK(_xfdashboard_autopin_windows_on_window_closed),
									self);
}


/* IMPLEMENTATION: Public API */

/* Create new instance */
XfdashboardAutopinWindows* xfdashboard_autopin_windows_new(void)
{
	GObject		*autopinWindows;

	autopinWindows=g_object_new(XFDASHBOARD_TYPE_AUTOPIN_WINDOWS, NULL);
	if(!autopinWindows) return(NULL);

	return(XFDASHBOARD_AUTOPIN_WINDOWS(autopinWindows));
}
