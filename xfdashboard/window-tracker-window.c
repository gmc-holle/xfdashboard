/*
 * window-tracker-window: A window tracked by window tracker and also
 *                        a wrapper class around WnckWindow.
 *                        By wrapping libwnck objects we can use a virtual
 *                        stable API while the API in libwnck changes
 *                        within versions. We only need to use #ifdefs in
 *                        window tracker object and nowhere else in the code.
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

#include "window-tracker-window.h"

#define WNCK_I_KNOW_THIS_IS_UNSTABLE
#include <libwnck/libwnck.h>

#include <glib/gi18n-lib.h>
#include <clutter/x11/clutter-x11.h>
#include <gtk/gtkx.h>
#include <gdk/gdkx.h>
#ifdef HAVE_XINERAMA
#include <X11/extensions/Xinerama.h>
#endif

#include "window-tracker.h"
#include "marshal.h"

/* Usually we would define a class in GObject system here but
 * this class is a wrapper around WnckWindow to create a virtual stable
 * libwnck API regardless of its version.
 */

/* IMPLEMENTATION: Private variables and methods */

/* Size of screen has changed so resize stage window */
static void _xfdashboard_window_tracker_window_on_screen_size_changed(XfdashboardWindowTracker *inWindowTracker,
																		gint inWidth,
																		gint inHeight,
																		gpointer inUserData)
{
#ifdef HAVE_XINERAMA
	WnckWindow				*stageWindow;
	GdkDisplay				*display;
	GdkScreen				*screen;
	XineramaScreenInfo		*monitors;
	int						monitorsCount;
	gint					top, bottom, left, right;
	gint					topIndex, bottomIndex, leftIndex, rightIndex;
	gint					i;
	Atom					atomFullscreenMonitors;
	XEvent					xEvent;

	g_return_if_fail(XFDASHBOARD_IS_WINDOW_TRACKER(inWindowTracker));
	g_return_if_fail(WNCK_IS_WINDOW(inUserData));

	stageWindow=WNCK_WINDOW(inUserData);

	g_debug("Set fullscreen across all monitors using Xinerama");

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
		g_debug("Checking edges at monitor %d with upper-left at %d,%d and lower-right at %d,%d [size: %dx%d]",
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
	g_debug("Found edge monitors: left=%d (monitor %d), right=%d (monitor %d), top=%d (monitor %d), bottom=%d (motniro %d)",
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
	WnckWindow				*stageWindow;
	GdkScreen				*screen;
	gint					primaryMonitor;
	GdkRectangle			geometry;

	g_return_if_fail(XFDASHBOARD_IS_WINDOW_TRACKER(inWindowTracker));
	g_return_if_fail(WNCK_IS_WINDOW(inUserData));

	stageWindow=WNCK_WINDOW(inUserData);

	g_debug("No support for multiple monitor: Setting fullscreen on primary monitor");

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

	g_debug("Moving stage window to %d,%d and resize to %dx%d",
				geometry.x, geometry.y,
				geometry.width, geometry.height);
#endif
}

/* State of stage window changed */
static void _xfdashboard_window_tracker_window_on_stage_state_changed(WnckWindow *inWindow,
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
		g_debug("State 'skip-tasklist' for stage window %p needs reset", inWindow);
	}

	/* Set 'skip-pager' if changed */
	if((inChangedMask & WNCK_WINDOW_STATE_SKIP_PAGER) &&
		!(inNewValue & WNCK_WINDOW_STATE_SKIP_PAGER))
	{
		wnck_window_set_skip_pager(WNCK_WINDOW(inWindow), TRUE);
		g_debug("State 'skip-pager' for stage window %p needs reset", inWindow);
	}

	/* Set 'make-above' if changed */
	if((inChangedMask & WNCK_WINDOW_STATE_ABOVE) &&
		!(inNewValue & WNCK_WINDOW_STATE_ABOVE))
	{
		wnck_window_make_above(WNCK_WINDOW(inWindow));
		g_debug("State 'make-above' for stage window %p needs reset", inWindow);
	}
}

/* The active window changed. Reselect stage window as active one if it is visible */
static void _xfdashboard_window_tracker_window_on_stage_active_window_changed(WnckScreen *self,
																				WnckWindow *inPreviousWindow,
																				gpointer inUserData)
{
	WnckWindow		*stageWindow;
	WnckWindow		*activeWindow;
	gboolean		reselect;

	g_return_if_fail(WNCK_IS_SCREEN(self));
	g_return_if_fail(inPreviousWindow==NULL || WNCK_IS_WINDOW(inPreviousWindow));
	g_return_if_fail(WNCK_IS_WINDOW(inUserData));

	stageWindow=WNCK_WINDOW(inUserData);
	reselect=FALSE;

	/* Reactive stage window if not hidden */
	activeWindow=wnck_screen_get_active_window(self);

	if(inPreviousWindow && inPreviousWindow==stageWindow) reselect=TRUE;
	if(!activeWindow || activeWindow!=stageWindow) reselect=TRUE;
	if(!(wnck_window_get_state(stageWindow) & (WNCK_WINDOW_STATE_MINIMIZED | WNCK_WINDOW_STATE_HIDDEN))) reselect=TRUE;

	if(reselect)
	{
		wnck_window_activate_transient(stageWindow, xfdashboard_window_tracker_get_time());
		g_debug("Active window changed from %p (%s) to %p (%s) but stage window %p is visible and should be active one",
					inPreviousWindow, inPreviousWindow ? wnck_window_get_name(inPreviousWindow) : "<nil>",
					activeWindow, activeWindow ? wnck_window_get_name(activeWindow) : "<nil>",
					stageWindow);
	}
}

/* IMPLEMENTATION: Public API */

/* Return type of WnckWindow as our type */
GType xfdashboard_window_tracker_window_get_type(void)
{
	return(WNCK_TYPE_WINDOW);
}

/* Determine if window is minimized */
gboolean xfdashboard_window_tracker_window_is_minized(XfdashboardWindowTrackerWindow *inWindow)
{
	WnckWindowState		state;
	gboolean			isMinimized;

	g_return_val_if_fail(WNCK_IS_WINDOW(inWindow), FALSE);

	isMinimized=FALSE;

	/* Check if state of window has minimized flag set */
	state=wnck_window_get_state(WNCK_WINDOW(inWindow));
	if(state & WNCK_WINDOW_STATE_MINIMIZED) isMinimized=TRUE;

	/* Return minimized state of window */
	return(isMinimized);
}

/* Determine if window is visible */
gboolean xfdashboard_window_tracker_window_is_visible(XfdashboardWindowTrackerWindow *inWindow)
{
	WnckWindowState		state;

	g_return_val_if_fail(WNCK_IS_WINDOW(inWindow), FALSE);

	state=wnck_window_get_state(WNCK_WINDOW(inWindow));

	/* Windows are invisible if hidden but not minimized */
	if((state & WNCK_WINDOW_STATE_HIDDEN) &&
		!(state & WNCK_WINDOW_STATE_MINIMIZED))
	{
		return(FALSE);
	}

	/* If we get here the window is visible */
	return(TRUE);
}

gboolean xfdashboard_window_tracker_window_is_visible_on_workspace(XfdashboardWindowTrackerWindow *inWindow,
																	XfdashboardWindowTrackerWorkspace *inWorkspace)
{
	g_return_val_if_fail(WNCK_IS_WINDOW(inWindow), FALSE);
	g_return_val_if_fail(WNCK_IS_WORKSPACE(inWorkspace), FALSE);

	/* Check if window is visible generally and if it is on requested workspace */
	return(xfdashboard_window_tracker_window_is_visible(inWindow) &&
			wnck_window_is_on_workspace(WNCK_WINDOW(inWindow), WNCK_WORKSPACE(inWorkspace)));
}

gboolean xfdashboard_window_tracker_window_is_visible_on_monitor(XfdashboardWindowTrackerWindow *inWindow,
																	XfdashboardWindowTrackerMonitor *inMonitor)
{
	g_return_val_if_fail(WNCK_IS_WINDOW(inWindow), FALSE);
	g_return_val_if_fail(XFDASHBOARD_IS_WINDOW_TRACKER_MONITOR(inMonitor), FALSE);

	/* Check if window is visible generally and if it is on requested monitor */
	return(xfdashboard_window_tracker_window_is_visible(inWindow) &&
			xfdashboard_window_tracker_window_is_on_monitor(inWindow, inMonitor));
}

/* Set visibility of window (show/hide) */
void xfdashboard_window_tracker_window_show(XfdashboardWindowTrackerWindow *inWindow)
{
	g_return_if_fail(WNCK_IS_WINDOW(inWindow));

	wnck_window_unminimize(WNCK_WINDOW(inWindow), xfdashboard_window_tracker_get_time());
}

void xfdashboard_window_tracker_window_hide(XfdashboardWindowTrackerWindow *inWindow)
{
	g_return_if_fail(WNCK_IS_WINDOW(inWindow));

	wnck_window_minimize(WNCK_WINDOW(inWindow));
}

/* Get workspace where window is on */
XfdashboardWindowTrackerWorkspace* xfdashboard_window_tracker_window_get_workspace(XfdashboardWindowTrackerWindow *inWindow)
{
	g_return_val_if_fail(WNCK_IS_WINDOW(inWindow), NULL);

	return(wnck_window_get_workspace(WNCK_WINDOW(inWindow)));
}

/* Determine if window is on requested workspace */
gboolean xfdashboard_window_tracker_window_is_on_workspace(XfdashboardWindowTrackerWindow *inWindow,
															XfdashboardWindowTrackerWorkspace *inWorkspace)
{
	g_return_val_if_fail(WNCK_IS_WINDOW(inWindow), FALSE);
	g_return_val_if_fail(WNCK_IS_WORKSPACE(inWorkspace), FALSE);

	return(wnck_window_is_on_workspace(WNCK_WINDOW(inWindow), WNCK_WORKSPACE(inWorkspace)));
}

/* Move a window to another workspace */
void xfdashboard_window_tracker_window_move_to_workspace(XfdashboardWindowTrackerWindow *inWindow,
															XfdashboardWindowTrackerWorkspace *inWorkspace)
{
	g_return_if_fail(WNCK_IS_WINDOW(inWindow));
	g_return_if_fail(WNCK_IS_WORKSPACE(inWorkspace));

	wnck_window_move_to_workspace(WNCK_WINDOW(inWindow), WNCK_WORKSPACE(inWorkspace));
}

/* Get monitor where window is on */
XfdashboardWindowTrackerMonitor* xfdashboard_window_tracker_window_get_monitor(XfdashboardWindowTrackerWindow *inWindow)
{
	XfdashboardWindowTracker			*windowTracker;
	GList								*monitors;
	XfdashboardWindowTrackerMonitor		*monitor;
	XfdashboardWindowTrackerMonitor		*foundMonitor;

	g_return_val_if_fail(WNCK_IS_WINDOW(inWindow), NULL);

	/* Get window tracker to retrieve list of monitors */
	windowTracker=xfdashboard_window_tracker_get_default();

	/* Get list of monitors */
	monitors=xfdashboard_window_tracker_get_monitors(windowTracker);

	/* Iterate through list of monitors and return monitor where window fits in */
	foundMonitor=NULL;
	for(; monitors && !foundMonitor; monitors=g_list_next(monitors))
	{
		monitor=XFDASHBOARD_WINDOW_TRACKER_MONITOR(monitors->data);
		if(xfdashboard_window_tracker_window_is_on_monitor(inWindow, monitor))
		{
			foundMonitor=monitor;
		}
	}

	/* Release allocated resources */
	g_object_unref(windowTracker);

	/* Return found monitor */
	return(foundMonitor);
}

/* Determine if window is on requested monitor */
gboolean xfdashboard_window_tracker_window_is_on_monitor(XfdashboardWindowTrackerWindow *inWindow,
															XfdashboardWindowTrackerMonitor *inMonitor)
{
	XfdashboardWindowTracker	*windowTracker;
	gint						windowX, windowY, windowWidth, windowHeight;
	gint						monitorX, monitorY, monitorWidth, monitorHeight;
	gint						screenWidth, screenHeight;
	gint						windowMiddleX, windowMiddleY;

	g_return_val_if_fail(WNCK_IS_WINDOW(inWindow), FALSE);
	g_return_val_if_fail(XFDASHBOARD_IS_WINDOW_TRACKER_MONITOR(inMonitor), FALSE);

	/* Get window geometry */
	xfdashboard_window_tracker_window_get_position_size(inWindow, &windowX, &windowY, &windowWidth, &windowHeight);

	/* Get monitor geometry */
	xfdashboard_window_tracker_monitor_get_geometry(inMonitor, &monitorX, &monitorY, &monitorWidth, &monitorHeight);

	/* Get screen size */
	windowTracker=xfdashboard_window_tracker_get_default();
	screenWidth=xfdashboard_window_tracker_get_screen_width(windowTracker);
	screenHeight=xfdashboard_window_tracker_get_screen_height(windowTracker);
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

/* Get name (title) of window */
const gchar* xfdashboard_window_tracker_window_get_title(XfdashboardWindowTrackerWindow *inWindow)
{
	g_return_val_if_fail(WNCK_IS_WINDOW(inWindow), NULL);

	if(!wnck_window_has_name(WNCK_WINDOW(inWindow))) return(NULL);

	return(wnck_window_get_name(WNCK_WINDOW(inWindow)));
}

/* Get icon of window */
GdkPixbuf* xfdashboard_window_tracker_window_get_icon(XfdashboardWindowTrackerWindow *inWindow)
{
	g_return_val_if_fail(WNCK_IS_WINDOW(inWindow), NULL);

	return(wnck_window_get_icon(WNCK_WINDOW(inWindow)));
}

const gchar* xfdashboard_window_tracker_window_get_icon_name(XfdashboardWindowTrackerWindow *inWindow)
{
	g_return_val_if_fail(WNCK_IS_WINDOW(inWindow), NULL);

	if(!wnck_window_has_icon_name(WNCK_WINDOW(inWindow))) return(NULL);

	return(wnck_window_get_icon_name(WNCK_WINDOW(inWindow)));
}

/* Get state of window */
gboolean xfdashboard_window_tracker_window_is_skip_pager(XfdashboardWindowTrackerWindow *inWindow)
{
	g_return_val_if_fail(WNCK_IS_WINDOW(inWindow), FALSE);

	return(wnck_window_is_skip_pager(WNCK_WINDOW(inWindow)));
}

gboolean xfdashboard_window_tracker_window_is_skip_tasklist(XfdashboardWindowTrackerWindow *inWindow)
{
	g_return_val_if_fail(WNCK_IS_WINDOW(inWindow), FALSE);

	return(wnck_window_is_skip_tasklist(WNCK_WINDOW(inWindow)));
}

gboolean xfdashboard_window_tracker_window_is_pinned(XfdashboardWindowTrackerWindow *inWindow)
{
	g_return_val_if_fail(WNCK_IS_WINDOW(inWindow), FALSE);

	return(wnck_window_is_pinned(WNCK_WINDOW(inWindow)));
}

/* Get possible actions of window */
gboolean xfdashboard_window_tracker_window_has_close_action(XfdashboardWindowTrackerWindow *inWindow)
{
	g_return_val_if_fail(WNCK_IS_WINDOW(inWindow), FALSE);

	return((wnck_window_get_actions(WNCK_WINDOW(inWindow)) & WNCK_WINDOW_ACTION_CLOSE) ? TRUE : FALSE);
}

/* Activate window with its transient windows */
void xfdashboard_window_tracker_window_activate(XfdashboardWindowTrackerWindow *inWindow)
{
	g_return_if_fail(WNCK_IS_WINDOW(inWindow));

	wnck_window_activate_transient(WNCK_WINDOW(inWindow), xfdashboard_window_tracker_get_time());
}

/* Close window */
void xfdashboard_window_tracker_window_close(XfdashboardWindowTrackerWindow *inWindow)
{
	g_return_if_fail(WNCK_IS_WINDOW(inWindow));

	wnck_window_close(WNCK_WINDOW(inWindow), xfdashboard_window_tracker_get_time());
}

/* Get position and size of window */
void xfdashboard_window_tracker_window_get_position(XfdashboardWindowTrackerWindow *inWindow, gint *outX, gint *outY)
{
	xfdashboard_window_tracker_window_get_position_size(inWindow, outX, outY, NULL, NULL);
}

void xfdashboard_window_tracker_window_get_size(XfdashboardWindowTrackerWindow *inWindow, gint *outWidth, gint *outHeight)
{
	xfdashboard_window_tracker_window_get_position_size(inWindow, NULL, NULL, outWidth, outHeight);
}

void xfdashboard_window_tracker_window_get_position_size(XfdashboardWindowTrackerWindow *inWindow,
															gint *outX, gint *outY,
															gint *outWidth, gint *outHeight)
{
	gint		x, y, w, h;

	g_return_if_fail(WNCK_IS_WINDOW(inWindow));

	/* Get window geometry */
	wnck_window_get_client_window_geometry(WNCK_WINDOW(inWindow), &x, &y, &w, &h);

	/* Set result */
	if(outX) *outX=x;
	if(outX) *outY=y;
	if(outWidth) *outWidth=w;
	if(outHeight) *outHeight=h;
}

/* Move and resize window */
void xfdashboard_window_tracker_window_move(XfdashboardWindowTrackerWindow *inWindow,
												gint inX,
												gint inY)
{
	xfdashboard_window_tracker_window_move_resize(inWindow, inX, inY, -1, -1);
}

void xfdashboard_window_tracker_window_resize(XfdashboardWindowTrackerWindow *inWindow,
												gint inWidth,
												gint inHeight)
{
	xfdashboard_window_tracker_window_move_resize(inWindow, -1, -1, inWidth, inHeight);
}

void xfdashboard_window_tracker_window_move_resize(XfdashboardWindowTrackerWindow *inWindow,
													gint inX,
													gint inY,
													gint inWidth,
													gint inHeight)
{
	WnckWindowMoveResizeMask	flags;

	g_return_if_fail(WNCK_IS_WINDOW(inWindow));

	/* Get modification flags */
	flags=0;
	if(inX>=0) flags|=WNCK_WINDOW_CHANGE_X;
	if(inY>=0) flags|=WNCK_WINDOW_CHANGE_Y;
	if(inWidth>=0) flags|=WNCK_WINDOW_CHANGE_WIDTH;
	if(inHeight>=0) flags|=WNCK_WINDOW_CHANGE_HEIGHT;

	/* Set geometry */
	wnck_window_set_geometry(WNCK_WINDOW(inWindow),
								WNCK_WINDOW_GRAVITY_STATIC,
								flags,
								inX, inY, inWidth, inHeight);
}

/* Determine and find stage by requested window */
gboolean xfdashboard_window_tracker_window_is_stage(XfdashboardWindowTrackerWindow *inWindow)
{
	g_return_val_if_fail(WNCK_IS_WINDOW(inWindow), FALSE);

	return(xfdashboard_window_tracker_window_find_stage(inWindow)!=NULL);
}

ClutterStage* xfdashboard_window_tracker_window_find_stage(XfdashboardWindowTrackerWindow *inWindow)
{
	ClutterStage			*foundStage;
	ClutterStage			*stage;
	Window					stageXWindow;
	GSList					*stages, *entry;

	g_return_val_if_fail(WNCK_IS_WINDOW(inWindow), NULL);

	/* Iterate through stages and check if stage window matches requested one */
	foundStage=NULL;
	stages=clutter_stage_manager_list_stages(clutter_stage_manager_get_default());
	for(entry=stages; !foundStage && entry; entry=g_slist_next(entry))
	{
		stage=CLUTTER_STAGE(entry->data);
		if(stage)
		{
			stageXWindow=clutter_x11_get_stage_window(stage);
			if(stageXWindow==wnck_window_get_xid(WNCK_WINDOW(inWindow))) foundStage=stage;
		}
	}
	g_slist_free(stages);

	return(foundStage);
}

/* Get window of stage */
XfdashboardWindowTrackerWindow* xfdashboard_window_tracker_window_get_stage_window(ClutterStage *inStage)
{
	Window					stageXWindow;
	WnckWindow				*window;

	g_return_val_if_fail(CLUTTER_IS_STAGE(inStage), NULL);

	/* Get stage X window and translate to needed window type */
	stageXWindow=clutter_x11_get_stage_window(inStage);
	window=wnck_window_get(stageXWindow);

	return(window);
}

/* Set up window for use as stage window */
void xfdashboard_window_tracker_window_make_stage_window(XfdashboardWindowTrackerWindow *inWindow)
{
	XfdashboardWindowTracker	*windowTracker;
	WnckScreen					*screen;
	guint						signalID;
	gulong						handlerID;

	g_return_if_fail(WNCK_IS_WINDOW(inWindow));

	/* Window of stage should always be above all other windows,
	 * pinned to all workspaces, not be listed in window pager
	 * and set to fullscreen
	 */
	if(!wnck_window_is_skip_tasklist(WNCK_WINDOW(inWindow))) wnck_window_set_skip_tasklist(WNCK_WINDOW(inWindow), TRUE);
	if(!wnck_window_is_skip_pager(WNCK_WINDOW(inWindow))) wnck_window_set_skip_pager(WNCK_WINDOW(inWindow), TRUE);
	if(!wnck_window_is_above(WNCK_WINDOW(inWindow))) wnck_window_make_above(WNCK_WINDOW(inWindow));
	if(!wnck_window_is_pinned(WNCK_WINDOW(inWindow))) wnck_window_pin(WNCK_WINDOW(inWindow));

	/* Get screen of window */
	screen=wnck_window_get_screen(WNCK_WINDOW(inWindow));

	/* Connect signals if not already connected */
	signalID=g_signal_lookup("state-changed", WNCK_TYPE_WINDOW);
	handlerID=g_signal_handler_find(inWindow,
									G_SIGNAL_MATCH_ID | G_SIGNAL_MATCH_FUNC,
									signalID,
									0,
									NULL,
									G_CALLBACK(_xfdashboard_window_tracker_window_on_stage_state_changed),
									NULL);
	if(!handlerID)
	{
		g_signal_connect(inWindow, "state-changed", G_CALLBACK(_xfdashboard_window_tracker_window_on_stage_state_changed), NULL);
		g_debug("Connecting signal to 'state-changed' at window %p", inWindow);
	}

	signalID=g_signal_lookup("active-window-changed", WNCK_TYPE_SCREEN);
	handlerID=g_signal_handler_find(screen,
									G_SIGNAL_MATCH_ID | G_SIGNAL_MATCH_FUNC,
									signalID,
									0,
									NULL,
									G_CALLBACK(_xfdashboard_window_tracker_window_on_stage_active_window_changed),
									NULL);
	if(!handlerID)
	{
		g_signal_connect(screen, "active-window-changed", G_CALLBACK(_xfdashboard_window_tracker_window_on_stage_active_window_changed), inWindow);
		g_debug("Connecting signal to 'active-window-changed' at screen %p of window %p", screen, inWindow);
	}

	windowTracker=xfdashboard_window_tracker_get_default();
	signalID=g_signal_lookup("screen-size-changed", XFDASHBOARD_TYPE_WINDOW_TRACKER);
	handlerID=g_signal_handler_find(windowTracker,
									G_SIGNAL_MATCH_ID | G_SIGNAL_MATCH_FUNC,
									signalID,
									0,
									NULL,
									G_CALLBACK(_xfdashboard_window_tracker_window_on_screen_size_changed),
									NULL);
	if(!handlerID)
	{
		g_signal_connect(windowTracker, "screen-size-changed", G_CALLBACK(_xfdashboard_window_tracker_window_on_screen_size_changed), inWindow);
		g_debug("Connecting signal to 'screen-size-changed' at window %p", inWindow);
	}
	_xfdashboard_window_tracker_window_on_screen_size_changed(windowTracker,
																xfdashboard_window_tracker_get_screen_width(windowTracker),
																xfdashboard_window_tracker_get_screen_height(windowTracker),
																inWindow);
	g_object_unref(windowTracker);
}

/* Unset up stage window (only remove connected signals) */
void xfdashboard_window_tracker_window_unmake_stage_window(XfdashboardWindowTrackerWindow *inWindow)
{
	XfdashboardWindowTracker	*windowTracker;
	WnckScreen					*screen;
	guint						signalID;
	gulong						handlerID;

	g_return_if_fail(WNCK_IS_WINDOW(inWindow));

	/* Get screen of window */
	screen=wnck_window_get_screen(WNCK_WINDOW(inWindow));

	/* Disconnect signals */
	signalID=g_signal_lookup("state-changed", WNCK_TYPE_WINDOW);
	handlerID=g_signal_handler_find(inWindow,
									G_SIGNAL_MATCH_ID | G_SIGNAL_MATCH_FUNC,
									signalID,
									0,
									NULL,
									G_CALLBACK(_xfdashboard_window_tracker_window_on_stage_state_changed),
									NULL);
	if(handlerID)
	{
		g_signal_handler_disconnect(inWindow, handlerID);
		g_debug("Disconnecting handler %lu for signal 'state-changed' at window %p", handlerID, inWindow);
	}

	signalID=g_signal_lookup("active-window-changed", WNCK_TYPE_SCREEN);
	handlerID=g_signal_handler_find(screen,
									G_SIGNAL_MATCH_ID | G_SIGNAL_MATCH_FUNC,
									signalID,
									0,
									NULL,
									G_CALLBACK(_xfdashboard_window_tracker_window_on_stage_active_window_changed),
									NULL);
	if(handlerID)
	{
		g_signal_handler_disconnect(screen, handlerID);
		g_debug("Disconnecting handler %lu for signal 'active-window-changed' at screen %p of window %p", handlerID, screen, inWindow);
	}

	windowTracker=xfdashboard_window_tracker_get_default();
	signalID=g_signal_lookup("screen-size-changed", XFDASHBOARD_TYPE_WINDOW_TRACKER);
	handlerID=g_signal_handler_find(windowTracker,
									G_SIGNAL_MATCH_ID | G_SIGNAL_MATCH_FUNC,
									signalID,
									0,
									NULL,
									G_CALLBACK(_xfdashboard_window_tracker_window_on_screen_size_changed),
									NULL);
	if(handlerID)
	{
		g_signal_handler_disconnect(windowTracker, handlerID);
		g_debug("Disconnecting handler %lu for signal 'screen-size-changed' at window %p", handlerID, inWindow);
	}
	g_object_unref(windowTracker);
}

/* Get X window ID of window */
gulong xfdashboard_window_tracker_window_get_xid(XfdashboardWindowTrackerWindow *inWindow)
{
	g_return_val_if_fail(WNCK_IS_WINDOW(inWindow), None);

	return(wnck_window_get_xid(WNCK_WINDOW(inWindow)));
}
