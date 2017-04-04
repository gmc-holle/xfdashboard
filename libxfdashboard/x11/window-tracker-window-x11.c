/*
 * window-tracker-window: A window tracked by window tracker and also
 *                        a wrapper class around WnckWindow.
 *                        By wrapping libwnck objects we can use a virtual
 *                        stable API while the API in libwnck changes
 *                        within versions. We only need to use #ifdefs in
 *                        window tracker object and nowhere else in the code.
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

/**
 * SECTION:window-tracker-window-x11
 * @short_description: A window used by X11 window tracker
 * @include: xfdashboard/x11/window-tracker-window-x11.h
 *
 * TODO: DESCRIPTION
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <libxfdashboard/x11/window-tracker-window-x11.h>

#define WNCK_I_KNOW_THIS_IS_UNSTABLE
#include <libwnck/libwnck.h>

#include <glib/gi18n-lib.h>
#include <clutter/x11/clutter-x11.h>
#include <gtk/gtkx.h>
#include <gdk/gdkx.h>
#ifdef HAVE_XINERAMA
#include <X11/extensions/Xinerama.h>
#endif

#include <libxfdashboard/x11/window-content-x11.h>
#include <libxfdashboard/x11/window-tracker-workspace-x11.h>
#include <libxfdashboard/x11/window-tracker-x11.h>
#include <libxfdashboard/window-tracker.h>
#include <libxfdashboard/marshal.h>
#include <libxfdashboard/compat.h>
#include <libxfdashboard/debug.h>


/* Define this class in GObject system */
static void _xfdashboard_window_tracker_window_x11_window_tracker_window_iface_init(XfdashboardWindowTrackerWindowInterface *iface);

G_DEFINE_TYPE_WITH_CODE(XfdashboardWindowTrackerWindowX11,
						xfdashboard_window_tracker_window_x11,
						G_TYPE_OBJECT,
						G_IMPLEMENT_INTERFACE(XFDASHBOARD_TYPE_WINDOW_TRACKER_WINDOW, _xfdashboard_window_tracker_window_x11_window_tracker_window_iface_init))

/* Private structure - access only by public API if needed */
#define XFDASHBOARD_WINDOW_TRACKER_WINDOW_X11_GET_PRIVATE(obj)                 \
	(G_TYPE_INSTANCE_GET_PRIVATE((obj), XFDASHBOARD_TYPE_WINDOW_TRACKER_WINDOW_X11, XfdashboardWindowTrackerWindowX11Private))

struct _XfdashboardWindowTrackerWindowX11Private
{
	/* Properties related */
	WnckWindow								*window;
	XfdashboardWindowTrackerWindowState		state;
	XfdashboardWindowTrackerWindowAction	actions;

	/* Instance related */
	WnckWorkspace							*workspace;

	gint									lastGeometryX;
	gint									lastGeometryY;
	gint									lastGeometryWidth;
	gint									lastGeometryHeight;
};


/* Properties */
enum
{
	PROP_0,

	PROP_WINDOW,

	/* Overriden properties of interface: XfdashboardWindowTrackerWindow */
	PROP_STATE,
	PROP_ACTIONS,

	PROP_LAST
};

static GParamSpec* XfdashboardWindowTrackerWindowX11Properties[PROP_LAST]={ 0, };


/* IMPLEMENTATION: Private variables and methods */
#define XFDASHBOARD_WINDOW_TRACKER_WINDOW_X11_WARN_NO_WINDOW(self)             \
	g_critical(_("No wnck window wrapped at %s in called function %s"),        \
				G_OBJECT_TYPE_NAME(self),                                      \
				__func__);

#define XFDASHBOARD_WINDOW_TRACKER_WINDOW_X11_WARN_WRONG_WINDOW(self)          \
	g_critical(_("Got signal from wrong wnck window wrapped at %s in called function %s"),\
				G_OBJECT_TYPE_NAME(self),                                      \
				__func__);

/* Get state of window */
static void _xfdashboard_window_tracker_window_x11_update_state(XfdashboardWindowTrackerWindowX11 *self)
{
	XfdashboardWindowTrackerWindowX11Private	*priv;
	XfdashboardWindowTrackerWindowState			newState;
	WnckWindowState								wnckState;

	g_return_if_fail(XFDASHBOARD_IS_WINDOW_TRACKER_WINDOW_X11(self));

	priv=self->priv;
	newState=0;

	/* A wnck window must be wrapped by this object */
	if(!priv->window)
	{
		XFDASHBOARD_WINDOW_TRACKER_WINDOW_X11_WARN_NO_WINDOW(self);
	}
		else
		{
			/* Get state of wnck window to determine state */
			wnckState=wnck_window_get_state(priv->window);

			/* Determine window state */
			if(wnckState & WNCK_WINDOW_STATE_HIDDEN) newState|=XFDASHBOARD_WINDOW_TRACKER_WINDOW_STATE_HIDDEN;

			if(wnckState & WNCK_WINDOW_STATE_MINIMIZED)
			{
				newState|=XFDASHBOARD_WINDOW_TRACKER_WINDOW_STATE_MINIMIZED;
			}
				else
				{
					if((wnckState & WNCK_WINDOW_STATE_MAXIMIZED_HORIZONTALLY) &&
						(wnckState & WNCK_WINDOW_STATE_MAXIMIZED_VERTICALLY))
					{
						newState|=XFDASHBOARD_WINDOW_TRACKER_WINDOW_STATE_MAXIMIZED;
					}
				}

			if(wnckState & WNCK_WINDOW_STATE_FULLSCREEN) newState|=XFDASHBOARD_WINDOW_TRACKER_WINDOW_STATE_FULLSCREEN;
			if(wnckState & WNCK_WINDOW_STATE_SKIP_PAGER) newState|=XFDASHBOARD_WINDOW_TRACKER_WINDOW_STATE_SKIP_PAGER;
			if(wnckState & WNCK_WINDOW_STATE_SKIP_TASKLIST) newState|=XFDASHBOARD_WINDOW_TRACKER_WINDOW_STATE_SKIP_TASKLIST;
			if(wnckState & WNCK_WINDOW_STATE_DEMANDS_ATTENTION) newState|=XFDASHBOARD_WINDOW_TRACKER_WINDOW_STATE_URGENT;
			if(wnckState & WNCK_WINDOW_STATE_URGENT) newState|=XFDASHBOARD_WINDOW_TRACKER_WINDOW_STATE_URGENT;

			/* "Pin" is not a wnck window state and do not get confused with the
			 * "sticky" state as it refers only to the window's stickyness on
			 * the viewport. So we have to ask wnck if it is pinned.
			 */
			if(wnck_window_is_pinned(priv->window)) newState|=XFDASHBOARD_WINDOW_TRACKER_WINDOW_STATE_PINNED;
		}

	/* Set value if changed */
	if(priv->state!=newState)
	{
		/* Set value */
		priv->state=newState;

		/* Notify about property change */
		g_object_notify_by_pspec(G_OBJECT(self), XfdashboardWindowTrackerWindowX11Properties[PROP_STATE]);
	}
}

/* Get actions of window */
static void _xfdashboard_window_tracker_window_x11_update_actions(XfdashboardWindowTrackerWindowX11 *self)
{
	XfdashboardWindowTrackerWindowX11Private	*priv;
	XfdashboardWindowTrackerWindowAction		newActions;
	WnckWindowActions							wnckActions;

	g_return_if_fail(XFDASHBOARD_IS_WINDOW_TRACKER_WINDOW_X11(self));

	priv=self->priv;
	newActions=0;

	/* A wnck window must be wrapped by this object */
	if(!priv->window)
	{
		XFDASHBOARD_WINDOW_TRACKER_WINDOW_X11_WARN_NO_WINDOW(self);
	}
		else
		{
			/* Get actions of wnck window to determine state */
			wnckActions=wnck_window_get_actions(priv->window);

			/* Determine window actions */
			if(wnckActions & WNCK_WINDOW_ACTION_CLOSE) newActions|=XFDASHBOARD_WINDOW_TRACKER_WINDOW_ACTION_CLOSE;
		}

	/* Set value if changed */
	if(priv->actions!=newActions)
	{
		/* Set value */
		priv->actions=newActions;

		/* Notify about property change */
		g_object_notify_by_pspec(G_OBJECT(self), XfdashboardWindowTrackerWindowX11Properties[PROP_ACTIONS]);
	}
}

/* Size of screen has changed so resize stage window */
static void _xfdashboard_window_tracker_window_x11_on_stage_screen_size_changed(XfdashboardWindowTracker *inWindowTracker,
																				gint inWidth,
																				gint inHeight,
																				gpointer inUserData)
{
#ifdef HAVE_XINERAMA
	XfdashboardWindowTrackerWindowX11	*realStageWindow;
	WnckWindow							*stageWindow;
	GdkDisplay							*display;
	GdkScreen							*screen;
	XineramaScreenInfo					*monitors;
	int									monitorsCount;
	gint								top, bottom, left, right;
	gint								topIndex, bottomIndex, leftIndex, rightIndex;
	gint								i;
	Atom								atomFullscreenMonitors;
	XEvent								xEvent;

	g_return_if_fail(XFDASHBOARD_IS_WINDOW_TRACKER(inWindowTracker));
	g_return_if_fail(XFDASHBOARD_IS_WINDOW_TRACKER_WINDOW_X11(inUserData));

	realStageWindow=XFDASHBOARD_WINDOW_TRACKER_WINDOW_X11(inUserData);

	XFDASHBOARD_DEBUG(inWindowTracker, WINDOWS, "Set fullscreen across all monitors using Xinerama");

	/* Get wnck window for stage window object as it is needed a lot from this
	 * point on.
	 */
	stageWindow=xfdashboard_window_tracker_window_x11_get_window(realStageWindow);

	/* If window manager does not support fullscreen across all monitors
	 * return here.
	 */
	if(!wnck_screen_net_wm_supports(wnck_window_get_screen(stageWindow), "_NET_WM_FULLSCREEN_MONITORS"))
	{
		g_warning(_("Keep window fullscreen on primary monitor because window manager does not support _NET_WM_FULLSCREEN_MONITORS."));
		return;
	}

	/* Get display */
	display=gdk_display_get_default();

	/* Get screen */
	screen=gdk_screen_get_default();

	/* Check if Xinerama is active on display. If not try to move and resize
	 * stage window to primary monitor.
	 */
	if(!XineramaIsActive(GDK_DISPLAY_XDISPLAY(display)))
	{
		gint				primaryMonitor;
		GdkRectangle		geometry;

		/* Get position and size of primary monitor and try to move and resize
		 * stage window to its position and size. Even if it fails it should
		 * resize the stage to the size of current monitor this window is
		 * fullscreened to. Tested with xfwm4.
		 */
		primaryMonitor=gdk_screen_get_primary_monitor(screen);
		gdk_screen_get_monitor_geometry(screen, primaryMonitor, &geometry);
		wnck_window_set_geometry(stageWindow,
									WNCK_WINDOW_GRAVITY_STATIC,
									WNCK_WINDOW_CHANGE_X | WNCK_WINDOW_CHANGE_Y | WNCK_WINDOW_CHANGE_WIDTH | WNCK_WINDOW_CHANGE_HEIGHT,
									geometry.x, geometry.y, geometry.width, geometry.height);
		return;
	}

	/* Get monitors from Xinerama */
	monitors=XineramaQueryScreens(GDK_DISPLAY_XDISPLAY(display), &monitorsCount);
	if(monitorsCount<=0 || !monitors)
	{
		if(monitors) XFree(monitors);
		return;
	}

	/* Get monitor indices for each corner of screen */
	top=gdk_screen_get_height(screen);
	left=gdk_screen_get_width(screen);
	bottom=0;
	right=0;
	topIndex=bottomIndex=leftIndex=rightIndex=0;
	for(i=0; i<monitorsCount; i++)
	{
		XFDASHBOARD_DEBUG(inWindowTracker, WINDOWS,
							"Checking edges at monitor %d with upper-left at %d,%d and lower-right at %d,%d [size: %dx%d]",
							i,
							monitors[i].x_org,
							monitors[i].y_org,
							monitors[i].x_org+monitors[i].width, monitors[i].y_org+monitors[i].height,
							monitors[i].width, monitors[i].height);

		if(left>monitors[i].x_org)
		{
			left=monitors[i].x_org;
			leftIndex=i;
		}

		if(right<(monitors[i].x_org+monitors[i].width))
		{
			right=(monitors[i].x_org+monitors[i].width);
			rightIndex=i;
		}

		if(top>monitors[i].y_org)
		{
			top=monitors[i].y_org;
			topIndex=i;
		}

		if(bottom<(monitors[i].y_org+monitors[i].height))
		{
			bottom=(monitors[i].y_org+monitors[i].height);
			bottomIndex=i;
		}
	}
	XFDASHBOARD_DEBUG(inWindowTracker, WINDOWS,
						"Found edge monitors: left=%d (monitor %d), right=%d (monitor %d), top=%d (monitor %d), bottom=%d (monitor %d)",
						left, leftIndex,
						right, rightIndex,
						top, topIndex,
						bottom, bottomIndex);

	/* Get X atom for fullscreen-across-all-monitors */
	atomFullscreenMonitors=XInternAtom(GDK_DISPLAY_XDISPLAY(display),
										"_NET_WM_FULLSCREEN_MONITORS",
										False);

	/* Send event to X to set window to fullscreen over all monitors */
	memset(&xEvent, 0, sizeof(xEvent));
	xEvent.type=ClientMessage;
	xEvent.xclient.window=wnck_window_get_xid(stageWindow);
	xEvent.xclient.display=GDK_DISPLAY_XDISPLAY(display);
	xEvent.xclient.message_type=atomFullscreenMonitors;
	xEvent.xclient.format=32;
	xEvent.xclient.data.l[0]=topIndex;
	xEvent.xclient.data.l[1]=bottomIndex;
	xEvent.xclient.data.l[2]=leftIndex;
	xEvent.xclient.data.l[3]=rightIndex;
	xEvent.xclient.data.l[4]=0;
	XSendEvent(GDK_DISPLAY_XDISPLAY(display),
				DefaultRootWindow(GDK_DISPLAY_XDISPLAY(display)),
				False,
				SubstructureRedirectMask | SubstructureNotifyMask,
				&xEvent);

	/* Release allocated resources */
	if(monitors) XFree(monitors);
#else
	XfdashboardWindowTrackerWindowX11	*realStageWindow;
	WnckWindow							*stageWindow;
	GdkScreen							*screen;
	gint								primaryMonitor;
	GdkRectangle						geometry;

	g_return_if_fail(XFDASHBOARD_IS_WINDOW_TRACKER(inWindowTracker));
	g_return_if_fail(XFDASHBOARD_IS_WINDOW_TRACKER_WINDOW_X11(inUserData));

	realStageWindow=XFDASHBOARD_WINDOW_TRACKER_WINDOW_X11(inUserData);

	XFDASHBOARD_DEBUG(inWindowTracker, WINDOWS, "No support for multiple monitor: Setting fullscreen on primary monitor");

	/* Get wnck window for stage window object as it is needed a lot from this
	 * point on.
	 */
	stageWindow=xfdashboard_window_tracker_window_x11_get_window(realStageWindow);

	/* Get screen */
	screen=gdk_screen_get_default();

	/* Get position and size of primary monitor and try to move and resize
	 * stage window to its position and size. Even if it fails it should
	 * resize the stage to the size of current monitor this window is
	 * fullscreened to. Tested with xfwm4.
	 */
	primaryMonitor=gdk_screen_get_primary_monitor(screen);
	gdk_screen_get_monitor_geometry(screen, primaryMonitor, &geometry);
	wnck_window_set_geometry(stageWindow,
								WNCK_WINDOW_GRAVITY_STATIC,
								WNCK_WINDOW_CHANGE_X | WNCK_WINDOW_CHANGE_Y | WNCK_WINDOW_CHANGE_WIDTH | WNCK_WINDOW_CHANGE_HEIGHT,
								geometry.x, geometry.y, geometry.width, geometry.height);

	XFDASHBOARD_DEBUG(inWindowTracker, WINDOWS,
						"Moving stage window to %d,%d and resize to %dx%d",
						geometry.x, geometry.y,
						geometry.width, geometry.height);
#endif
}

/* State of stage window changed */
static void _xfdashboard_window_tracker_window_x11_on_stage_state_changed(WnckWindow *inWindow,
																			WnckWindowState inChangedMask,
																			WnckWindowState inNewValue,
																			gpointer inUserData)
{
	g_return_if_fail(WNCK_IS_WINDOW(inWindow));

	/* Set 'skip-tasklist' if changed */
	if((inChangedMask & WNCK_WINDOW_STATE_SKIP_TASKLIST) &&
		!(inNewValue & WNCK_WINDOW_STATE_SKIP_TASKLIST))
	{
		wnck_window_set_skip_tasklist(WNCK_WINDOW(inWindow), TRUE);
		XFDASHBOARD_DEBUG(inWindow, WINDOWS,
							"State 'skip-tasklist' for stage window %p needs reset",
							inWindow);
	}

	/* Set 'skip-pager' if changed */
	if((inChangedMask & WNCK_WINDOW_STATE_SKIP_PAGER) &&
		!(inNewValue & WNCK_WINDOW_STATE_SKIP_PAGER))
	{
		wnck_window_set_skip_pager(WNCK_WINDOW(inWindow), TRUE);
		XFDASHBOARD_DEBUG(inWindow, WINDOWS,
							"State 'skip-pager' for stage window %p needs reset",
							inWindow);
	}

	/* Set 'make-above' if changed */
	if((inChangedMask & WNCK_WINDOW_STATE_ABOVE) &&
		!(inNewValue & WNCK_WINDOW_STATE_ABOVE))
	{
		wnck_window_make_above(WNCK_WINDOW(inWindow));
		XFDASHBOARD_DEBUG(inWindow, WINDOWS,
							"State 'make-above' for stage window %p needs reset",
							inWindow);
	}
}

/* The active window changed. Reselect stage window as active one if it is visible */
static void _xfdashboard_window_tracker_window_x11_on_stage_active_window_changed(WnckScreen *inScreen,
																					WnckWindow *inPreviousWindow,
																					gpointer inUserData)
{
	XfdashboardWindowTrackerWindowX11			*self;
	XfdashboardWindowTrackerWindowX11Private	*priv;
	WnckWindow									*activeWindow;
	gboolean									reselect;

	g_return_if_fail(WNCK_IS_SCREEN(inScreen));
	g_return_if_fail(inPreviousWindow==NULL || WNCK_IS_WINDOW(inPreviousWindow));
	g_return_if_fail(XFDASHBOARD_IS_WINDOW_TRACKER_WINDOW_X11(inUserData));

	self=XFDASHBOARD_WINDOW_TRACKER_WINDOW_X11(inUserData);
	priv=self->priv;
	reselect=FALSE;

	/* Reactive stage window if not hidden */
	activeWindow=wnck_screen_get_active_window(inScreen);

	if(inPreviousWindow && inPreviousWindow==priv->window) reselect=TRUE;
	if(!activeWindow || activeWindow!=priv->window) reselect=TRUE;
	if(!(priv->state & (XFDASHBOARD_WINDOW_TRACKER_WINDOW_STATE_MINIMIZED | XFDASHBOARD_WINDOW_TRACKER_WINDOW_STATE_HIDDEN))) reselect=TRUE;

	if(reselect)
	{
		wnck_window_activate_transient(priv->window, xfdashboard_window_tracker_x11_get_time());
		XFDASHBOARD_DEBUG(self, WINDOWS,
							"Active window changed from %p (%s) to %p (%s) but stage window %p is visible and should be active one",
							inPreviousWindow, inPreviousWindow ? wnck_window_get_name(inPreviousWindow) : "<nil>",
							activeWindow, activeWindow ? wnck_window_get_name(activeWindow) : "<nil>",
							priv->window);
	}
}

/* Proxy signal for mapped wnck window which changed name */
static void _xfdashboard_window_tracker_window_x11_on_wnck_name_changed(XfdashboardWindowTrackerWindowX11 *self,
																		gpointer inUserData)
{
	XfdashboardWindowTrackerWindowX11Private	*priv;
	WnckWindow									*window;

	g_return_if_fail(XFDASHBOARD_IS_WINDOW_TRACKER_WINDOW_X11(self));
	g_return_if_fail(WNCK_IS_WINDOW(inUserData));

	priv=self->priv;
	window=WNCK_WINDOW(inUserData);

	/* Check that window emitting this signal is the mapped window of this object */
	if(priv->window!=window)
	{
		XFDASHBOARD_WINDOW_TRACKER_WINDOW_X11_WARN_WRONG_WINDOW(self);
		return;
	}

	/* Proxy signal */
	g_signal_emit_by_name(self, "name-changed");
}

/* Proxy signal for mapped wnck window which changed states */
static void _xfdashboard_window_tracker_window_x11_on_wnck_state_changed(XfdashboardWindowTrackerWindowX11 *self,
																			WnckWindowState inChangedStates,
																			WnckWindowState inNewState,
																			gpointer inUserData)
{
	XfdashboardWindowTrackerWindowX11Private	*priv;
	WnckWindow									*window;

	g_return_if_fail(XFDASHBOARD_IS_WINDOW_TRACKER_WINDOW_X11(self));
	g_return_if_fail(WNCK_IS_WINDOW(inUserData));

	priv=self->priv;
	window=WNCK_WINDOW(inUserData);

	/* Check that window emitting this signal is the mapped window of this object */
	if(priv->window!=window)
	{
		XFDASHBOARD_WINDOW_TRACKER_WINDOW_X11_WARN_WRONG_WINDOW(self);
		return;
	}

	/* Update state before emitting signal */
	_xfdashboard_window_tracker_window_x11_update_state(self);

	/* Proxy signal */
	g_signal_emit_by_name(self, "state-changed", inChangedStates, inNewState);
}

/* Proxy signal for mapped wnck window which changed actions */
static void _xfdashboard_window_tracker_window_x11_on_wnck_actions_changed(XfdashboardWindowTrackerWindowX11 *self,
																			WnckWindowActions inChangedActions,
																			WnckWindowActions inNewActions,
																			gpointer inUserData)
{
	XfdashboardWindowTrackerWindowX11Private	*priv;
	WnckWindow									*window;

	g_return_if_fail(XFDASHBOARD_IS_WINDOW_TRACKER_WINDOW_X11(self));
	g_return_if_fail(WNCK_IS_WINDOW(inUserData));

	priv=self->priv;
	window=WNCK_WINDOW(inUserData);

	/* Check that window emitting this signal is the mapped window of this object */
	if(priv->window!=window)
	{
		XFDASHBOARD_WINDOW_TRACKER_WINDOW_X11_WARN_WRONG_WINDOW(self);
		return;
	}

	/* Update actions before emitting signal */
	_xfdashboard_window_tracker_window_x11_update_actions(self);

	/* Proxy signal */
	g_signal_emit_by_name(self, "actions-changed", inChangedActions, inNewActions);
}

/* Proxy signal for mapped wnck window which changed icon */
static void _xfdashboard_window_tracker_window_x11_on_wnck_icon_changed(XfdashboardWindowTrackerWindowX11 *self,
																		gpointer inUserData)
{
	XfdashboardWindowTrackerWindowX11Private	*priv;
	WnckWindow									*window;

	g_return_if_fail(XFDASHBOARD_IS_WINDOW_TRACKER_WINDOW_X11(self));
	g_return_if_fail(WNCK_IS_WINDOW(inUserData));

	priv=self->priv;
	window=WNCK_WINDOW(inUserData);

	/* Check that window emitting this signal is the mapped window of this object */
	if(priv->window!=window)
	{
		XFDASHBOARD_WINDOW_TRACKER_WINDOW_X11_WARN_WRONG_WINDOW(self);
		return;
	}

	/* Proxy signal */
	g_signal_emit_by_name(self, "icon-changed");
}

/* Proxy signal for mapped wnck window which changed workspace */
static void _xfdashboard_window_tracker_window_x11_on_wnck_workspace_changed(XfdashboardWindowTrackerWindowX11 *self,
																				gpointer inUserData)
{
	XfdashboardWindowTrackerWindowX11Private	*priv;
	WnckWindow									*window;
	XfdashboardWindowTrackerWorkspace			*oldWorkspace;

	g_return_if_fail(XFDASHBOARD_IS_WINDOW_TRACKER_WINDOW_X11(self));
	g_return_if_fail(WNCK_IS_WINDOW(inUserData));

	priv=self->priv;
	window=WNCK_WINDOW(inUserData);

	/* Check that window emitting this signal is the mapped window of this object */
	if(priv->window!=window)
	{
		XFDASHBOARD_WINDOW_TRACKER_WINDOW_X11_WARN_WRONG_WINDOW(self);
		return;
	}

	/* Get mapped workspace object for last known workspace of this window */
	oldWorkspace=NULL;
	if(priv->workspace)
	{
		XfdashboardWindowTracker				*windowTracker;

		windowTracker=xfdashboard_window_tracker_get_default();
		oldWorkspace=xfdashboard_window_tracker_x11_get_workspace_for_wnck(XFDASHBOARD_WINDOW_TRACKER_X11(windowTracker), priv->workspace);
		g_object_unref(windowTracker);
	}

	/* Proxy signal */
	g_signal_emit_by_name(self, "workspace-changed", oldWorkspace);

	/* Remember new workspace as last known workspace */
	priv->workspace=wnck_window_get_workspace(window);
}

/* Proxy signal for mapped wnck window which changed geometry */
static void _xfdashboard_window_tracker_window_x11_on_wnck_geometry_changed(XfdashboardWindowTrackerWindowX11 *self,
																			gpointer inUserData)
{
	XfdashboardWindowTrackerWindowX11Private	*priv;
	WnckWindow									*window;
	gint										x, y, width, height;

	g_return_if_fail(XFDASHBOARD_IS_WINDOW_TRACKER_WINDOW_X11(self));
	g_return_if_fail(WNCK_IS_WINDOW(inUserData));

	priv=self->priv;
	window=WNCK_WINDOW(inUserData);

	/* Check that window emitting this signal is the mapped window of this object */
	if(priv->window!=window)
	{
		XFDASHBOARD_WINDOW_TRACKER_WINDOW_X11_WARN_WRONG_WINDOW(self);
		return;
	}

	/* Get current position and size of window and check against last known
	 * position and size of window to determine if window has moved or resized.
	 */
	wnck_window_get_geometry(priv->window, &x, &y, &width, &height);
	if(priv->lastGeometryX!=x ||
		priv->lastGeometryY!=y ||
		priv->lastGeometryWidth!=width ||
		priv->lastGeometryHeight!=height)
	{
		XfdashboardWindowTracker				*windowTracker;
		gint									screenWidth, screenHeight;
		XfdashboardWindowTrackerMonitor			*oldMonitor;
		XfdashboardWindowTrackerMonitor			*currentMonitor;
		gint									windowMiddleX, windowMiddleY;

		/* Get window tracker */
		windowTracker=xfdashboard_window_tracker_get_default();

		/* Get monitor at old position of window and the monitor at current.
		 * If they differ emit signal for window changed monitor.
		 */
		xfdashboard_window_tracker_get_screen_size(windowTracker, &screenWidth, &screenHeight);

		windowMiddleX=priv->lastGeometryX+(priv->lastGeometryWidth/2);
		if(windowMiddleX>screenWidth) windowMiddleX=screenWidth-1;

		windowMiddleY=priv->lastGeometryY+(priv->lastGeometryHeight/2);
		if(windowMiddleY>screenHeight) windowMiddleY=screenHeight-1;

		oldMonitor=xfdashboard_window_tracker_get_monitor_by_position(windowTracker, windowMiddleX, windowMiddleY);

		currentMonitor=xfdashboard_window_tracker_window_get_monitor(XFDASHBOARD_WINDOW_TRACKER_WINDOW(self));

		if(currentMonitor!=oldMonitor)
		{
			/* Emit signal */
			XFDASHBOARD_DEBUG(self, WINDOWS,
								"Window '%s' moved from monitor %d (%s) to %d (%s)",
								wnck_window_get_name(priv->window),
								oldMonitor ? xfdashboard_window_tracker_monitor_get_number(oldMonitor) : -1,
								(oldMonitor && xfdashboard_window_tracker_monitor_is_primary(oldMonitor)) ? "primary" : "non-primary",
								currentMonitor ? xfdashboard_window_tracker_monitor_get_number(currentMonitor) : -1,
								(currentMonitor && xfdashboard_window_tracker_monitor_is_primary(currentMonitor)) ? "primary" : "non-primary");
			g_signal_emit_by_name(self, "monitor-changed", oldMonitor);
		}

		/* Remember current position and size as last known ones */
		priv->lastGeometryX=x;
		priv->lastGeometryY=y;
		priv->lastGeometryWidth=width;
		priv->lastGeometryHeight=height;

		/* Release allocated resources */
		g_object_unref(windowTracker);
	}

	/* Proxy signal */
	g_signal_emit_by_name(self, "geometry-changed");
}

/* Set wnck window to map in this window object */
static void _xfdashboard_window_tracker_window_x11_set_window(XfdashboardWindowTrackerWindowX11 *self,
																WnckWindow *inWindow)
{
	XfdashboardWindowTrackerWindowX11Private	*priv;

	g_return_if_fail(XFDASHBOARD_IS_WINDOW_TRACKER_WINDOW_X11(self));
	g_return_if_fail(!inWindow || WNCK_IS_WINDOW(inWindow));

	priv=self->priv;

	/* Set value if changed */
	if(priv->window!=inWindow)
	{
		/* Disconnect signals to old window (if available) and reset states */
		if(priv->window)
		{
			/* Remove weak reference at old window */
			g_object_remove_weak_pointer(G_OBJECT(priv->window), (gpointer*)&priv->window);

			/* Disconnect signal handlers */
			g_signal_handlers_disconnect_by_data(priv->window, self);
			priv->window=NULL;
		}
		priv->state=0;
		priv->actions=0;
		priv->workspace=NULL;

		/* Set new value */
		priv->window=inWindow;

		/* Initialize states and connect signals if window is set */
		if(priv->window)
		{
			/* Add weak reference at new window */
			g_object_add_weak_pointer(G_OBJECT(priv->window), (gpointer*)&priv->window);

			/* Initialize states */
			_xfdashboard_window_tracker_window_x11_update_state(self);
			_xfdashboard_window_tracker_window_x11_update_actions(self);
			priv->workspace=wnck_window_get_workspace(priv->window);
			wnck_window_get_geometry(priv->window,
										&priv->lastGeometryX,
										&priv->lastGeometryY,
										&priv->lastGeometryWidth,
										&priv->lastGeometryHeight);

			/* Connect signals */
			g_signal_connect_swapped(priv->window,
										"name-changed",
										G_CALLBACK(_xfdashboard_window_tracker_window_x11_on_wnck_name_changed),
										self);
			g_signal_connect_swapped(priv->window,
										"state-changed",
										G_CALLBACK(_xfdashboard_window_tracker_window_x11_on_wnck_state_changed),
										self);
			g_signal_connect_swapped(priv->window,
										"actions-changed",
										G_CALLBACK(_xfdashboard_window_tracker_window_x11_on_wnck_actions_changed),
										self);
			g_signal_connect_swapped(priv->window,
										"icon-changed",
										G_CALLBACK(_xfdashboard_window_tracker_window_x11_on_wnck_icon_changed),
										self);
			g_signal_connect_swapped(priv->window,
										"workspace-changed",
										G_CALLBACK(_xfdashboard_window_tracker_window_x11_on_wnck_workspace_changed),
										self);
			g_signal_connect_swapped(priv->window,
										"geometry-changed",
										G_CALLBACK(_xfdashboard_window_tracker_window_x11_on_wnck_geometry_changed),
										self);
		}

		/* Notify about property change */
		g_object_notify_by_pspec(G_OBJECT(self), XfdashboardWindowTrackerWindowX11Properties[PROP_WINDOW]);
	}
}


/* IMPLEMENTATION: Interface XfdashboardWindowTrackerWindow */

/* Determine if window is visible */
static gboolean _xfdashboard_window_tracker_window_x11_window_tracker_window_is_visible(XfdashboardWindowTrackerWindow *inWindow)
{
	XfdashboardWindowTrackerWindowX11			*self;
	XfdashboardWindowTrackerWindowX11Private	*priv;

	g_return_val_if_fail(XFDASHBOARD_IS_WINDOW_TRACKER_WINDOW_X11(inWindow), FALSE);

	self=XFDASHBOARD_WINDOW_TRACKER_WINDOW_X11(inWindow);
	priv=self->priv;

	/* Windows are invisible if hidden but not minimized */
	if((priv->state & XFDASHBOARD_WINDOW_TRACKER_WINDOW_STATE_HIDDEN) &&
		!(priv->state & XFDASHBOARD_WINDOW_TRACKER_WINDOW_STATE_MINIMIZED))
	{
		return(FALSE);
	}

	/* If we get here the window is visible */
	return(TRUE);
}

/* Show window */
static void _xfdashboard_window_tracker_window_x11_window_tracker_window_show(XfdashboardWindowTrackerWindow *inWindow)
{
	XfdashboardWindowTrackerWindowX11			*self;
	XfdashboardWindowTrackerWindowX11Private	*priv;

	g_return_if_fail(XFDASHBOARD_IS_WINDOW_TRACKER_WINDOW_X11(inWindow));

	self=XFDASHBOARD_WINDOW_TRACKER_WINDOW_X11(inWindow);
	priv=self->priv;

	/* A wnck window must be wrapped by this object */
	if(!priv->window)
	{
		XFDASHBOARD_WINDOW_TRACKER_WINDOW_X11_WARN_NO_WINDOW(self);
		return;
	}

	/* Show (unminize) window */
	wnck_window_unminimize(priv->window, xfdashboard_window_tracker_x11_get_time());
}

/* Show window */
static void _xfdashboard_window_tracker_window_x11_window_tracker_window_hide(XfdashboardWindowTrackerWindow *inWindow)
{
	XfdashboardWindowTrackerWindowX11			*self;
	XfdashboardWindowTrackerWindowX11Private	*priv;

	g_return_if_fail(XFDASHBOARD_IS_WINDOW_TRACKER_WINDOW_X11(inWindow));

	self=XFDASHBOARD_WINDOW_TRACKER_WINDOW_X11(inWindow);
	priv=self->priv;

	/* A wnck window must be wrapped by this object */
	if(!priv->window)
	{
		XFDASHBOARD_WINDOW_TRACKER_WINDOW_X11_WARN_NO_WINDOW(self);
		return;
	}

	/* Hide (minimize) window */
	wnck_window_minimize(priv->window);
}

/* Get parent window if this window is a child window */
static XfdashboardWindowTrackerWindow* _xfdashboard_window_tracker_window_x11_window_tracker_window_get_parent(XfdashboardWindowTrackerWindow *inWindow)
{
	XfdashboardWindowTrackerWindowX11			*self;
	XfdashboardWindowTrackerWindowX11Private	*priv;
	WnckWindow									*parentWindow;
	XfdashboardWindowTracker					*windowTracker;
	XfdashboardWindowTrackerWindow				*foundWindow;

	g_return_val_if_fail(XFDASHBOARD_IS_WINDOW_TRACKER_WINDOW_X11(inWindow), NULL);

	self=XFDASHBOARD_WINDOW_TRACKER_WINDOW_X11(inWindow);
	priv=self->priv;

	/* A wnck window must be wrapped by this object */
	if(!priv->window)
	{
		XFDASHBOARD_WINDOW_TRACKER_WINDOW_X11_WARN_NO_WINDOW(self);
		return(NULL);
	}

	/* Get parent window */
	parentWindow=wnck_window_get_transient(priv->window);
	if(!parentWindow) return(NULL);

	/* Get window tracker and lookup the mapped and matching XfdashboardWindowTrackerWindow
	 * for wnck window.
	 */
	windowTracker=xfdashboard_window_tracker_get_default();
	foundWindow=xfdashboard_window_tracker_x11_get_window_for_wnck(XFDASHBOARD_WINDOW_TRACKER_X11(windowTracker), parentWindow);
	g_object_unref(windowTracker);

	/* Return found window object */
	return(foundWindow);
}

/* Get window state */
static XfdashboardWindowTrackerWindowState _xfdashboard_window_tracker_window_x11_window_tracker_window_get_state(XfdashboardWindowTrackerWindow *inWindow)
{
	XfdashboardWindowTrackerWindowX11			*self;
	XfdashboardWindowTrackerWindowX11Private	*priv;

	g_return_val_if_fail(XFDASHBOARD_IS_WINDOW_TRACKER_WINDOW_X11(inWindow), 0);

	self=XFDASHBOARD_WINDOW_TRACKER_WINDOW_X11(inWindow);
	priv=self->priv;

	/* Return state of window */
	return(priv->state);
}

/* Get window actions */
static XfdashboardWindowTrackerWindowAction _xfdashboard_window_tracker_window_x11_window_tracker_window_get_actions(XfdashboardWindowTrackerWindow *inWindow)
{
	XfdashboardWindowTrackerWindowX11			*self;
	XfdashboardWindowTrackerWindowX11Private	*priv;

	g_return_val_if_fail(XFDASHBOARD_IS_WINDOW_TRACKER_WINDOW_X11(inWindow), 0);

	self=XFDASHBOARD_WINDOW_TRACKER_WINDOW_X11(inWindow);
	priv=self->priv;

	/* Return actions of window */
	return(priv->actions);
}

/* Get name (title) of window */
static const gchar* _xfdashboard_window_tracker_window_x11_window_tracker_window_get_name(XfdashboardWindowTrackerWindow *inWindow)
{
	XfdashboardWindowTrackerWindowX11			*self;
	XfdashboardWindowTrackerWindowX11Private	*priv;

	g_return_val_if_fail(XFDASHBOARD_IS_WINDOW_TRACKER_WINDOW_X11(inWindow), NULL);

	self=XFDASHBOARD_WINDOW_TRACKER_WINDOW_X11(inWindow);
	priv=self->priv;

	/* A wnck window must be wrapped by this object */
	if(!priv->window)
	{
		XFDASHBOARD_WINDOW_TRACKER_WINDOW_X11_WARN_NO_WINDOW(self);
		return(NULL);
	}

	/* Check if window has a name to return and return name or NULL */
	if(!wnck_window_has_name(priv->window)) return(NULL);

	return(wnck_window_get_name(priv->window));
}

/* Get icon of window */
static GdkPixbuf* _xfdashboard_window_tracker_window_x11_window_tracker_window_get_icon(XfdashboardWindowTrackerWindow *inWindow)
{
	XfdashboardWindowTrackerWindowX11			*self;
	XfdashboardWindowTrackerWindowX11Private	*priv;

	g_return_val_if_fail(XFDASHBOARD_IS_WINDOW_TRACKER_WINDOW_X11(inWindow), NULL);

	self=XFDASHBOARD_WINDOW_TRACKER_WINDOW_X11(inWindow);
	priv=self->priv;

	/* A wnck window must be wrapped by this object */
	if(!priv->window)
	{
		XFDASHBOARD_WINDOW_TRACKER_WINDOW_X11_WARN_NO_WINDOW(self);
		return(NULL);
	}

	/* Return icon as pixbuf of window */
	return(wnck_window_get_icon(priv->window));
}

static const gchar* _xfdashboard_window_tracker_window_x11_window_tracker_window_get_icon_name(XfdashboardWindowTrackerWindow *inWindow)
{
	XfdashboardWindowTrackerWindowX11			*self;
	XfdashboardWindowTrackerWindowX11Private	*priv;

	g_return_val_if_fail(XFDASHBOARD_IS_WINDOW_TRACKER_WINDOW_X11(inWindow), NULL);

	self=XFDASHBOARD_WINDOW_TRACKER_WINDOW_X11(inWindow);
	priv=self->priv;

	/* A wnck window must be wrapped by this object */
	if(!priv->window)
	{
		XFDASHBOARD_WINDOW_TRACKER_WINDOW_X11_WARN_NO_WINDOW(self);
		return(NULL);
	}

	/* Check if window has an icon name to return and return icon name or NULL */
	if(!wnck_window_has_icon_name(priv->window)) return(NULL);

	return(wnck_window_get_icon_name(priv->window));
}

/* Get workspace where window is on */
static XfdashboardWindowTrackerWorkspace* _xfdashboard_window_tracker_window_x11_window_tracker_window_get_workspace(XfdashboardWindowTrackerWindow *inWindow)
{
	XfdashboardWindowTrackerWindowX11			*self;
	XfdashboardWindowTrackerWindowX11Private	*priv;
	WnckWorkspace								*wantedWorkspace;
	XfdashboardWindowTracker					*windowTracker;
	XfdashboardWindowTrackerWorkspace			*foundWorkspace;

	g_return_val_if_fail(XFDASHBOARD_IS_WINDOW_TRACKER_WINDOW_X11(inWindow), NULL);

	self=XFDASHBOARD_WINDOW_TRACKER_WINDOW_X11(inWindow);
	priv=self->priv;

	/* A wnck window must be wrapped by this object */
	if(!priv->window)
	{
		XFDASHBOARD_WINDOW_TRACKER_WINDOW_X11_WARN_NO_WINDOW(self);
		return(NULL);
	}

	/* Get real wnck workspace of window to lookup a mapped and matching
	 * XfdashboardWindowTrackerWorkspace object.
	 * NOTE: Workspace may be NULL. In this case return NULL immediately and
	 *       do not lookup a matching workspace object.
	 */
	wantedWorkspace=wnck_window_get_workspace(priv->window);
	if(!wantedWorkspace) return(NULL);

	/* Get window tracker and lookup the mapped and matching XfdashboardWindowTrackerWorkspace
	 * for wnck workspace.
	 */
	windowTracker=xfdashboard_window_tracker_get_default();
	foundWorkspace=xfdashboard_window_tracker_x11_get_workspace_for_wnck(XFDASHBOARD_WINDOW_TRACKER_X11(windowTracker), wantedWorkspace);
	g_object_unref(windowTracker);

	/* Return found workspace */
	return(foundWorkspace);
}

/* Determine if window is on requested workspace */
static gboolean _xfdashboard_window_tracker_window_x11_window_tracker_window_is_on_workspace(XfdashboardWindowTrackerWindow *inWindow,
																								XfdashboardWindowTrackerWorkspace *inWorkspace)
{
	XfdashboardWindowTrackerWindowX11			*self;
	XfdashboardWindowTrackerWindowX11Private	*priv;
	WnckWorkspace								*workspace;

	g_return_val_if_fail(XFDASHBOARD_IS_WINDOW_TRACKER_WINDOW_X11(inWindow), FALSE);
	g_return_val_if_fail(XFDASHBOARD_IS_WINDOW_TRACKER_WORKSPACE_X11(inWorkspace), FALSE);

	self=XFDASHBOARD_WINDOW_TRACKER_WINDOW_X11(inWindow);
	priv=self->priv;

	/* A wnck window must be wrapped by this object */
	if(!priv->window)
	{
		XFDASHBOARD_WINDOW_TRACKER_WINDOW_X11_WARN_NO_WINDOW(self);
		return(FALSE);
	}

	/* Get wnck workspace to check if window is on this one */
	workspace=xfdashboard_window_tracker_workspace_x11_get_workspace(XFDASHBOARD_WINDOW_TRACKER_WORKSPACE_X11(inWorkspace));
	if(!workspace)
	{
		g_critical(_("Either no wnck workspace is wrapped at %s or workspace is not available anymore when called at function %s"),
					G_OBJECT_TYPE_NAME(inWorkspace),
					__func__);
		return(FALSE);
	}

	/* Check if window is on that workspace */
	return(wnck_window_is_on_workspace(priv->window, workspace));
}

/* Get geometry (position and size) of window */
static void _xfdashboard_window_tracker_window_x11_window_tracker_window_get_geometry(XfdashboardWindowTrackerWindow *inWindow,
																				gint *outX,
																				gint *outY,
																				gint *outWidth,
																				gint *outHeight)
{
	XfdashboardWindowTrackerWindowX11			*self;
	XfdashboardWindowTrackerWindowX11Private	*priv;
	gint										x, y, width, height;

	g_return_if_fail(XFDASHBOARD_IS_WINDOW_TRACKER_WINDOW_X11(inWindow));

	self=XFDASHBOARD_WINDOW_TRACKER_WINDOW_X11(inWindow);
	priv=self->priv;

	/* A wnck window must be wrapped by this object */
	if(!priv->window)
	{
		XFDASHBOARD_WINDOW_TRACKER_WINDOW_X11_WARN_NO_WINDOW(self);
		return;
	}

	/* Get window geometry */
	wnck_window_get_client_window_geometry(priv->window, &x, &y, &width, &height);

	/* Set result */
	if(outX) *outX=x;
	if(outX) *outY=y;
	if(outWidth) *outWidth=width;
	if(outHeight) *outHeight=height;
}

/* Set geometry (position and size) of window */
static void _xfdashboard_window_tracker_window_x11_window_tracker_window_set_geometry(XfdashboardWindowTrackerWindow *inWindow,
																				gint inX,
																				gint inY,
																				gint inWidth,
																				gint inHeight)
{
	XfdashboardWindowTrackerWindowX11			*self;
	XfdashboardWindowTrackerWindowX11Private	*priv;
	WnckWindowMoveResizeMask					flags;
	gint										contentX, contentY;
	gint										contentWidth, contentHeight;
	gint										borderX, borderY;
	gint										borderWidth, borderHeight;

	g_return_if_fail(XFDASHBOARD_IS_WINDOW_TRACKER_WINDOW_X11(inWindow));

	self=XFDASHBOARD_WINDOW_TRACKER_WINDOW_X11(inWindow);
	priv=self->priv;

	/* A wnck window must be wrapped by this object */
	if(!priv->window)
	{
		XFDASHBOARD_WINDOW_TRACKER_WINDOW_X11_WARN_NO_WINDOW(self);
		return;
	}

	/* Get window border size to respect it when moving window */
	wnck_window_get_client_window_geometry(priv->window, &contentX, &contentY, &contentWidth, &contentHeight);
	wnck_window_get_geometry(priv->window, &borderX, &borderY, &borderWidth, &borderHeight);

	/* Get modification flags */
	flags=0;
	if(inX>=0)
	{
		flags|=WNCK_WINDOW_CHANGE_X;
		inX-=(contentX-borderX);
	}

	if(inY>=0)
	{
		flags|=WNCK_WINDOW_CHANGE_Y;
		inY-=(contentY-borderY);
	}

	if(inWidth>=0)
	{
		flags|=WNCK_WINDOW_CHANGE_WIDTH;
		inWidth+=(borderWidth-contentWidth);
	}

	if(inHeight>=0)
	{
		flags|=WNCK_WINDOW_CHANGE_HEIGHT;
		inHeight+=(borderHeight-contentHeight);
	}

	/* Set geometry */
	wnck_window_set_geometry(priv->window,
								WNCK_WINDOW_GRAVITY_STATIC,
								flags,
								inX, inY, inWidth, inHeight);
}

/* Move window */
static void _xfdashboard_window_tracker_window_x11_window_tracker_window_move(XfdashboardWindowTrackerWindow *inWindow,
																		gint inX,
																		gint inY)
{
	_xfdashboard_window_tracker_window_x11_window_tracker_window_set_geometry(inWindow, inX, inY, -1, -1);
}

/* Resize window */
static void _xfdashboard_window_tracker_window_x11_window_tracker_window_resize(XfdashboardWindowTrackerWindow *inWindow,
																			gint inWidth,
																			gint inHeight)
{
	_xfdashboard_window_tracker_window_x11_window_tracker_window_set_geometry(inWindow, -1, -1, inWidth, inHeight);
}

/* Move a window to another workspace */
static void _xfdashboard_window_tracker_window_x11_window_tracker_window_move_to_workspace(XfdashboardWindowTrackerWindow *inWindow,
																					XfdashboardWindowTrackerWorkspace *inWorkspace)
{
	XfdashboardWindowTrackerWindowX11			*self;
	XfdashboardWindowTrackerWindowX11Private	*priv;
	WnckWorkspace								*workspace;

	g_return_if_fail(XFDASHBOARD_IS_WINDOW_TRACKER_WINDOW_X11(inWindow));
	g_return_if_fail(XFDASHBOARD_IS_WINDOW_TRACKER_WORKSPACE_X11(inWorkspace));

	self=XFDASHBOARD_WINDOW_TRACKER_WINDOW_X11(inWindow);
	priv=self->priv;

	/* A wnck window must be wrapped by this object */
	if(!priv->window)
	{
		XFDASHBOARD_WINDOW_TRACKER_WINDOW_X11_WARN_NO_WINDOW(self);
		return;
	}

	/* Get wnck workspace to move window to */
	workspace=xfdashboard_window_tracker_workspace_x11_get_workspace(XFDASHBOARD_WINDOW_TRACKER_WORKSPACE_X11(inWorkspace));
	if(!workspace)
	{
		g_critical(_("Either no wnck workspace is wrapped at %s or workspace is not available anymore when called at function %s"),
					G_OBJECT_TYPE_NAME(inWorkspace),
					__func__);
		return;
	}

	/* Move window to workspace */
	wnck_window_move_to_workspace(priv->window, workspace);
}

/* Activate window with its transient windows */
static void _xfdashboard_window_tracker_window_x11_window_tracker_window_activate(XfdashboardWindowTrackerWindow *inWindow)
{
	XfdashboardWindowTrackerWindowX11			*self;
	XfdashboardWindowTrackerWindowX11Private	*priv;

	g_return_if_fail(XFDASHBOARD_IS_WINDOW_TRACKER_WINDOW_X11(inWindow));

	self=XFDASHBOARD_WINDOW_TRACKER_WINDOW_X11(inWindow);
	priv=self->priv;

	/* A wnck window must be wrapped by this object */
	if(!priv->window)
	{
		XFDASHBOARD_WINDOW_TRACKER_WINDOW_X11_WARN_NO_WINDOW(self);
		return;
	}

	/* Activate window */
	wnck_window_activate_transient(priv->window, xfdashboard_window_tracker_x11_get_time());
}

/* Close window */
static void _xfdashboard_window_tracker_window_x11_window_tracker_window_close(XfdashboardWindowTrackerWindow *inWindow)
{
	XfdashboardWindowTrackerWindowX11			*self;
	XfdashboardWindowTrackerWindowX11Private	*priv;

	g_return_if_fail(XFDASHBOARD_IS_WINDOW_TRACKER_WINDOW_X11(inWindow));

	self=XFDASHBOARD_WINDOW_TRACKER_WINDOW_X11(inWindow);
	priv=self->priv;

	/* A wnck window must be wrapped by this object */
	if(!priv->window)
	{
		XFDASHBOARD_WINDOW_TRACKER_WINDOW_X11_WARN_NO_WINDOW(self);
		return;
	}

	/* Close window */
	wnck_window_close(priv->window, xfdashboard_window_tracker_x11_get_time());
}

/* Get process ID owning the requested window */
static gint _xfdashboard_window_tracker_window_x11_window_tracker_window_get_pid(XfdashboardWindowTrackerWindow *inWindow)
{
	XfdashboardWindowTrackerWindowX11			*self;
	XfdashboardWindowTrackerWindowX11Private	*priv;

	g_return_val_if_fail(XFDASHBOARD_IS_WINDOW_TRACKER_WINDOW_X11(inWindow), -1);

	self=XFDASHBOARD_WINDOW_TRACKER_WINDOW_X11(inWindow);
	priv=self->priv;

	/* A wnck window must be wrapped by this object */
	if(!priv->window)
	{
		XFDASHBOARD_WINDOW_TRACKER_WINDOW_X11_WARN_NO_WINDOW(self);
		return(-1);
	}

	/* Return PID retrieved from wnck window */
	return(wnck_window_get_pid(priv->window));
}

/* Get all possible instance name for window, e.g. class name, instance name.
 * Caller is responsible to free result with g_strfreev() if not NULL.
 */
static gchar** _xfdashboard_window_tracker_window_x11_window_tracker_window_get_instance_names(XfdashboardWindowTrackerWindow *inWindow)
{
	XfdashboardWindowTrackerWindowX11			*self;
	XfdashboardWindowTrackerWindowX11Private	*priv;
	GSList										*names;
	GSList										*iter;
	const gchar									*value;
	guint										numberEntries;
	gchar										**result;

	g_return_val_if_fail(XFDASHBOARD_IS_WINDOW_TRACKER_WINDOW_X11(inWindow), NULL);

	self=XFDASHBOARD_WINDOW_TRACKER_WINDOW_X11(inWindow);
	priv=self->priv;
	names=NULL;
	result=NULL;

	/* A wnck window must be wrapped by this object */
	if(!priv->window)
	{
		XFDASHBOARD_WINDOW_TRACKER_WINDOW_X11_WARN_NO_WINDOW(self);
		return(NULL);
	}

	/* Add class name of window to list */
	value=wnck_window_get_class_group_name(priv->window);
	if(value) names=g_slist_prepend(names, g_strdup(value));

	/* Add instance name of window to list */
	value=wnck_window_get_class_instance_name(priv->window);
	if(value) names=g_slist_prepend(names, g_strdup(value));

	/* Add role of window to list */
	value=wnck_window_get_role(priv->window);
	if(value) names=g_slist_prepend(names, g_strdup(value));

	/* If nothing was added to list of name, stop here and return */
	if(!names) return(NULL);

	/* Build result list as a NULL-terminated list of strings */
	numberEntries=g_slist_length(names);

	result=g_new(gchar*, numberEntries+1);
	result[numberEntries]=NULL;
	for(iter=names; iter; iter=g_slist_next(iter))
	{
		numberEntries--;
		result[numberEntries]=iter->data;
	}

	/* Release allocated resources */
	g_slist_free(names);

	/* Return result list */
	return(result);
}

/* Get content for this window for use in actors.
 * Caller is responsible to remove reference with g_object_unref().
 */
static ClutterContent* _xfdashboard_window_tracker_window_x11_window_tracker_window_get_content(XfdashboardWindowTrackerWindow *inWindow)
{
	XfdashboardWindowTrackerWindowX11			*self;
	XfdashboardWindowTrackerWindowX11Private	*priv;
	ClutterContent								*content;

	g_return_val_if_fail(XFDASHBOARD_IS_WINDOW_TRACKER_WINDOW_X11(inWindow), NULL);

	self=XFDASHBOARD_WINDOW_TRACKER_WINDOW_X11(inWindow);
	priv=self->priv;

	/* A wnck window must be wrapped by this object */
	if(!priv->window)
	{
		XFDASHBOARD_WINDOW_TRACKER_WINDOW_X11_WARN_NO_WINDOW(self);
		return(NULL);
	}

	/* Create content for window */
	content=xfdashboard_window_content_x11_new_for_window(self);

	/* Return content */
	return(content);
}

/* Get associated stage of window */
static ClutterStage* _xfdashboard_window_tracker_window_x11_window_tracker_window_get_stage(XfdashboardWindowTrackerWindow *inWindow)
{
	XfdashboardWindowTrackerWindowX11			*self;
	XfdashboardWindowTrackerWindowX11Private	*priv;
	ClutterStage								*foundStage;
	ClutterStage								*stage;
	Window										stageXWindow;
	GSList										*stages, *entry;

	g_return_val_if_fail(XFDASHBOARD_IS_WINDOW_TRACKER_WINDOW_X11(inWindow), NULL);

	self=XFDASHBOARD_WINDOW_TRACKER_WINDOW_X11(inWindow);
	priv=self->priv;

	/* A wnck window must be wrapped by this object */
	if(!priv->window)
	{
		XFDASHBOARD_WINDOW_TRACKER_WINDOW_X11_WARN_NO_WINDOW(self);
		return(NULL);
	}

	/* Iterate through stages and check if stage window matches requested one */
	foundStage=NULL;
	stages=clutter_stage_manager_list_stages(clutter_stage_manager_get_default());
	for(entry=stages; !foundStage && entry; entry=g_slist_next(entry))
	{
		stage=CLUTTER_STAGE(entry->data);
		if(stage)
		{
			stageXWindow=clutter_x11_get_stage_window(stage);
			if(stageXWindow==wnck_window_get_xid(priv->window)) foundStage=stage;
		}
	}
	g_slist_free(stages);

	return(foundStage);
}

/* Set up window for use as stage window */
static void _xfdashboard_window_tracker_window_x11_window_tracker_window_make_stage_window(XfdashboardWindowTrackerWindow *inWindow)
{
	XfdashboardWindowTrackerWindowX11			*self;
	XfdashboardWindowTrackerWindowX11Private	*priv;
	XfdashboardWindowTracker					*windowTracker;
	WnckScreen									*screen;
	guint										signalID;
	gulong										handlerID;
	gint										width, height;

	g_return_if_fail(XFDASHBOARD_IS_WINDOW_TRACKER_WINDOW_X11(inWindow));

	self=XFDASHBOARD_WINDOW_TRACKER_WINDOW_X11(inWindow);
	priv=self->priv;

	/* A wnck window must be wrapped by this object */
	if(!priv->window)
	{
		XFDASHBOARD_WINDOW_TRACKER_WINDOW_X11_WARN_NO_WINDOW(self);
		return;
	}

	/* Window of stage should always be above all other windows, pinned to all
	 * workspaces, not be listed in window pager and set to fullscreen
	 */
	if(!wnck_window_is_skip_tasklist(priv->window)) wnck_window_set_skip_tasklist(priv->window, TRUE);
	if(!wnck_window_is_skip_pager(priv->window)) wnck_window_set_skip_pager(priv->window, TRUE);
	if(!wnck_window_is_above(priv->window)) wnck_window_make_above(priv->window);
	if(!wnck_window_is_pinned(priv->window)) wnck_window_pin(priv->window);

	/* Get screen of window */
	screen=wnck_window_get_screen(priv->window);

	/* Connect signals if not already connected */
	signalID=g_signal_lookup("state-changed", WNCK_TYPE_WINDOW);
	handlerID=g_signal_handler_find(priv->window,
									G_SIGNAL_MATCH_ID | G_SIGNAL_MATCH_FUNC,
									signalID,
									0,
									NULL,
									G_CALLBACK(_xfdashboard_window_tracker_window_x11_on_stage_state_changed),
									NULL);
	if(!handlerID)
	{
		g_signal_connect(priv->window,
							"state-changed",
							G_CALLBACK(_xfdashboard_window_tracker_window_x11_on_stage_state_changed),
							NULL);
		XFDASHBOARD_DEBUG(self, WINDOWS,
							"Connecting signal to 'state-changed' at window %p (wnck-window=%p)",
							self,
							priv->window);
	}

	signalID=g_signal_lookup("active-window-changed", WNCK_TYPE_SCREEN);
	handlerID=g_signal_handler_find(screen,
									G_SIGNAL_MATCH_ID | G_SIGNAL_MATCH_FUNC,
									signalID,
									0,
									NULL,
									G_CALLBACK(_xfdashboard_window_tracker_window_x11_on_stage_active_window_changed),
									NULL);
	if(!handlerID)
	{
		g_signal_connect(screen,
							"active-window-changed",
							G_CALLBACK(_xfdashboard_window_tracker_window_x11_on_stage_active_window_changed),
							self);
		XFDASHBOARD_DEBUG(self, WINDOWS,
							"Connecting signal to 'active-window-changed' at screen %p of window %p (wnck-window=%p)",
							screen,
							self,
							priv->window);
	}

	windowTracker=xfdashboard_window_tracker_get_default();
	signalID=g_signal_lookup("screen-size-changed", XFDASHBOARD_TYPE_WINDOW_TRACKER);
	handlerID=g_signal_handler_find(windowTracker,
									G_SIGNAL_MATCH_ID | G_SIGNAL_MATCH_FUNC,
									signalID,
									0,
									NULL,
									G_CALLBACK(_xfdashboard_window_tracker_window_x11_on_stage_screen_size_changed),
									NULL);
	if(!handlerID)
	{
		g_signal_connect(windowTracker,
							"screen-size-changed",
							G_CALLBACK(_xfdashboard_window_tracker_window_x11_on_stage_screen_size_changed),
							self);
		XFDASHBOARD_DEBUG(self, WINDOWS,
							"Connecting signal to 'screen-size-changed' at window %p (wnck-window=%p)",
							self,
							priv->window);
	}
	xfdashboard_window_tracker_get_screen_size(windowTracker, &width, &height);
	_xfdashboard_window_tracker_window_x11_on_stage_screen_size_changed(windowTracker,
																	width,
																	height,
																	self);
	g_object_unref(windowTracker);
}

/* Unset up stage window (only remove connected signals) */
static void _xfdashboard_window_tracker_window_x11_window_tracker_window_unmake_stage_window(XfdashboardWindowTrackerWindow *inWindow)
{
	XfdashboardWindowTrackerWindowX11			*self;
	XfdashboardWindowTrackerWindowX11Private	*priv;
	XfdashboardWindowTracker					*windowTracker;
	WnckScreen									*screen;
	guint										signalID;
	gulong										handlerID;

	g_return_if_fail(XFDASHBOARD_IS_WINDOW_TRACKER_WINDOW_X11(inWindow));

	self=XFDASHBOARD_WINDOW_TRACKER_WINDOW_X11(inWindow);
	priv=self->priv;

	/* A wnck window must be wrapped by this object */
	if(!priv->window)
	{
		XFDASHBOARD_WINDOW_TRACKER_WINDOW_X11_WARN_NO_WINDOW(self);
		return;
	}

	/* Get screen of window */
	screen=wnck_window_get_screen(WNCK_WINDOW(priv->window));

	/* Disconnect signals */
	signalID=g_signal_lookup("state-changed", WNCK_TYPE_WINDOW);
	handlerID=g_signal_handler_find(priv->window,
									G_SIGNAL_MATCH_ID | G_SIGNAL_MATCH_FUNC,
									signalID,
									0,
									NULL,
									G_CALLBACK(_xfdashboard_window_tracker_window_x11_on_stage_state_changed),
									NULL);
	if(handlerID)
	{
		g_signal_handler_disconnect(priv->window, handlerID);
		XFDASHBOARD_DEBUG(self, WINDOWS,
							"Disconnecting handler %lu for signal 'state-changed' at window %p (wnck-window=%p)",
							handlerID,
							self,
							priv->window);
	}

	signalID=g_signal_lookup("active-window-changed", WNCK_TYPE_SCREEN);
	handlerID=g_signal_handler_find(screen,
									G_SIGNAL_MATCH_ID | G_SIGNAL_MATCH_FUNC,
									signalID,
									0,
									NULL,
									G_CALLBACK(_xfdashboard_window_tracker_window_x11_on_stage_active_window_changed),
									NULL);
	if(handlerID)
	{
		g_signal_handler_disconnect(screen, handlerID);
		XFDASHBOARD_DEBUG(self, WINDOWS,
							"Disconnecting handler %lu for signal 'active-window-changed' at screen %p of window %p (wnck-window=%p)",
							handlerID,
							screen,
							self,
							priv->window);
	}

	windowTracker=xfdashboard_window_tracker_get_default();
	signalID=g_signal_lookup("screen-size-changed", XFDASHBOARD_TYPE_WINDOW_TRACKER);
	handlerID=g_signal_handler_find(windowTracker,
									G_SIGNAL_MATCH_ID | G_SIGNAL_MATCH_FUNC,
									signalID,
									0,
									NULL,
									G_CALLBACK(_xfdashboard_window_tracker_window_x11_on_stage_screen_size_changed),
									NULL);
	if(handlerID)
	{
		g_signal_handler_disconnect(windowTracker, handlerID);
		XFDASHBOARD_DEBUG(self, WINDOWS,
							"Disconnecting handler %lu for signal 'screen-size-changed' at window %p (wnck-window=%p)",
							handlerID,
							self,
							priv->window);
	}
	g_object_unref(windowTracker);
}

/* Interface initialization
 * Set up default functions
 */
static void _xfdashboard_window_tracker_window_x11_window_tracker_window_iface_init(XfdashboardWindowTrackerWindowInterface *iface)
{
	iface->is_visible=_xfdashboard_window_tracker_window_x11_window_tracker_window_is_visible;
	iface->show=_xfdashboard_window_tracker_window_x11_window_tracker_window_show;
	iface->hide=_xfdashboard_window_tracker_window_x11_window_tracker_window_hide;

	iface->get_parent=_xfdashboard_window_tracker_window_x11_window_tracker_window_get_parent;

	iface->get_state=_xfdashboard_window_tracker_window_x11_window_tracker_window_get_state;
	iface->get_actions=_xfdashboard_window_tracker_window_x11_window_tracker_window_get_actions;

	iface->get_name=_xfdashboard_window_tracker_window_x11_window_tracker_window_get_name;

	iface->get_icon=_xfdashboard_window_tracker_window_x11_window_tracker_window_get_icon;
	iface->get_icon_name=_xfdashboard_window_tracker_window_x11_window_tracker_window_get_icon_name;

	iface->get_workspace=_xfdashboard_window_tracker_window_x11_window_tracker_window_get_workspace;
	iface->is_on_workspace=_xfdashboard_window_tracker_window_x11_window_tracker_window_is_on_workspace;

	iface->get_geometry=_xfdashboard_window_tracker_window_x11_window_tracker_window_get_geometry;
	iface->set_geometry=_xfdashboard_window_tracker_window_x11_window_tracker_window_set_geometry;
	iface->move=_xfdashboard_window_tracker_window_x11_window_tracker_window_move;
	iface->resize=_xfdashboard_window_tracker_window_x11_window_tracker_window_resize;
	iface->move_to_workspace=_xfdashboard_window_tracker_window_x11_window_tracker_window_move_to_workspace;
	iface->activate=_xfdashboard_window_tracker_window_x11_window_tracker_window_activate;
	iface->close=_xfdashboard_window_tracker_window_x11_window_tracker_window_close;

	iface->get_pid=_xfdashboard_window_tracker_window_x11_window_tracker_window_get_pid;
	iface->get_instance_names=_xfdashboard_window_tracker_window_x11_window_tracker_window_get_instance_names;

	iface->get_content=_xfdashboard_window_tracker_window_x11_window_tracker_window_get_content;

	iface->get_stage=_xfdashboard_window_tracker_window_x11_window_tracker_window_get_stage;
	iface->make_stage_window=_xfdashboard_window_tracker_window_x11_window_tracker_window_make_stage_window;
	iface->unmake_stage_window=_xfdashboard_window_tracker_window_x11_window_tracker_window_unmake_stage_window;
}


/* IMPLEMENTATION: GObject */

/* Dispose this object */
static void _xfdashboard_window_tracker_window_x11_dispose(GObject *inObject)
{
	XfdashboardWindowTrackerWindowX11			*self=XFDASHBOARD_WINDOW_TRACKER_WINDOW_X11(inObject);
	XfdashboardWindowTrackerWindowX11Private	*priv=self->priv;

	/* Dispose allocated resources */
	if(priv->window)
	{
		/* Remove weak reference at current window */
		g_object_remove_weak_pointer(G_OBJECT(priv->window), (gpointer*)&priv->window);

		/* Disconnect signal handlers */
		g_signal_handlers_disconnect_by_data(priv->window, self);
		priv->window=NULL;
	}

	/* Call parent's class dispose method */
	G_OBJECT_CLASS(xfdashboard_window_tracker_window_x11_parent_class)->dispose(inObject);
}

/* Set/get properties */
static void _xfdashboard_window_tracker_window_x11_set_property(GObject *inObject,
																guint inPropID,
																const GValue *inValue,
																GParamSpec *inSpec)
{
	XfdashboardWindowTrackerWindowX11		*self=XFDASHBOARD_WINDOW_TRACKER_WINDOW_X11(inObject);

	switch(inPropID)
	{
		case PROP_WINDOW:
			_xfdashboard_window_tracker_window_x11_set_window(self, WNCK_WINDOW(g_value_get_object(inValue)));
			break;

		default:
			G_OBJECT_WARN_INVALID_PROPERTY_ID(inObject, inPropID, inSpec);
			break;
	}
}

static void _xfdashboard_window_tracker_window_x11_get_property(GObject *inObject,
																guint inPropID,
																GValue *outValue,
																GParamSpec *inSpec)
{
	XfdashboardWindowTrackerWindowX11		*self=XFDASHBOARD_WINDOW_TRACKER_WINDOW_X11(inObject);

	switch(inPropID)
	{
		case PROP_WINDOW:
			g_value_set_object(outValue, self->priv->window);
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
void xfdashboard_window_tracker_window_x11_class_init(XfdashboardWindowTrackerWindowX11Class *klass)
{
	GObjectClass						*gobjectClass=G_OBJECT_CLASS(klass);
	XfdashboardWindowTracker			*windowIface;
	GParamSpec							*paramSpec;

	/* Reference interface type to lookup properties etc. */
	windowIface=g_type_default_interface_ref(XFDASHBOARD_TYPE_WINDOW_TRACKER_WINDOW);

	/* Override functions */
	gobjectClass->dispose=_xfdashboard_window_tracker_window_x11_dispose;
	gobjectClass->set_property=_xfdashboard_window_tracker_window_x11_set_property;
	gobjectClass->get_property=_xfdashboard_window_tracker_window_x11_get_property;

	/* Set up private structure */
	g_type_class_add_private(klass, sizeof(XfdashboardWindowTrackerWindowX11Private));

	/* Define properties */
	XfdashboardWindowTrackerWindowX11Properties[PROP_WINDOW]=
		g_param_spec_object("window",
							_("Window"),
							_("The mapped wnck window"),
							WNCK_TYPE_WINDOW,
							G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY | G_PARAM_STATIC_STRINGS);

	paramSpec=g_object_interface_find_property(windowIface, "state");
	XfdashboardWindowTrackerWindowX11Properties[PROP_STATE]=
		g_param_spec_override("state", paramSpec);

	paramSpec=g_object_interface_find_property(windowIface, "actions");
	XfdashboardWindowTrackerWindowX11Properties[PROP_ACTIONS]=
		g_param_spec_override("actions", paramSpec);

	g_object_class_install_properties(gobjectClass, PROP_LAST, XfdashboardWindowTrackerWindowX11Properties);

	/* Release allocated resources */
	g_type_default_interface_unref(windowIface);
}

/* Object initialization
 * Create private structure and set up default values
 */
void xfdashboard_window_tracker_window_x11_init(XfdashboardWindowTrackerWindowX11 *self)
{
	XfdashboardWindowTrackerWindowX11Private	*priv;

	priv=self->priv=XFDASHBOARD_WINDOW_TRACKER_WINDOW_X11_GET_PRIVATE(self);

	/* Set default values */
	priv->window=NULL;
}


/* IMPLEMENTATION: Public API */

/**
 * xfdashboard_window_tracker_window_x11_get_window:
 * @self: A #XfdashboardWindowTrackerWindowX11
 *
 * Returns the wrapped window of libwnck.
 *
 * Return value: (transfer none): the #WnckWindow wrapped by @self. The returned
 *   #WnckWindow is owned by libwnck and must not be referenced or unreferenced.
 */
WnckWindow* xfdashboard_window_tracker_window_x11_get_window(XfdashboardWindowTrackerWindowX11 *self)
{
	XfdashboardWindowTrackerWindowX11Private	*priv;

	g_return_val_if_fail(XFDASHBOARD_IS_WINDOW_TRACKER_WINDOW_X11(self), NULL);

	priv=self->priv;

	/* A wnck window must be wrapped by this object */
	if(!priv->window)
	{
		XFDASHBOARD_WINDOW_TRACKER_WINDOW_X11_WARN_NO_WINDOW(self);
		return(NULL);
	}

	/* Return wrapped libwnck window */
	return(self->priv->window);
}

/**
 * xfdashboard_window_tracker_window_x11_get_xid:
 * @self: A #XfdashboardWindowTrackerWindowX11
 *
 * Gets the X window ID of the wrapped libwnck's window at @self.
 *
 * Return value: the X window ID of @self.
 **/
gulong xfdashboard_window_tracker_window_x11_get_xid(XfdashboardWindowTrackerWindowX11 *self)
{
	XfdashboardWindowTrackerWindowX11Private	*priv;

	g_return_val_if_fail(XFDASHBOARD_IS_WINDOW_TRACKER_WINDOW_X11(self), None);

	priv=self->priv;

	/* A wnck window must be wrapped by this object */
	if(!priv->window)
	{
		XFDASHBOARD_WINDOW_TRACKER_WINDOW_X11_WARN_NO_WINDOW(self);
		return(None);
	}

	/* Return X window ID */
	return(wnck_window_get_xid(priv->window));
}
