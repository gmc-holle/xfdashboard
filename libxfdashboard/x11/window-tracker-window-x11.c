/*
 * window-tracker-window: A window tracked by window tracker and also
 *                        a wrapper class around WnckWindow.
 *                        By wrapping libwnck objects we can use a virtual
 *                        stable API while the API in libwnck changes
 *                        within versions. We only need to use #ifdefs in
 *                        window tracker object and nowhere else in the code.
 * 
 * Copyright 2012-2020 Stephan Haller <nomad@froevel.de>
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
 * This is the X11 backend of #XfdashboardWindowTrackerWindow
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
#include <libxfdashboard/core.h>
#include <libxfdashboard/marshal.h>
#include <libxfdashboard/compat.h>
#include <libxfdashboard/debug.h>


/* Define this class in GObject system */
static void _xfdashboard_window_tracker_window_x11_window_tracker_window_iface_init(XfdashboardWindowTrackerWindowInterface *iface);

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

	ClutterContent							*content;
};

G_DEFINE_TYPE_WITH_CODE(XfdashboardWindowTrackerWindowX11,
						xfdashboard_window_tracker_window_x11,
						G_TYPE_OBJECT,
						G_ADD_PRIVATE(XfdashboardWindowTrackerWindowX11)
						G_IMPLEMENT_INTERFACE(XFDASHBOARD_TYPE_WINDOW_TRACKER_WINDOW, _xfdashboard_window_tracker_window_x11_window_tracker_window_iface_init))

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
	g_critical("No wnck window wrapped at %s in called function %s",           \
				G_OBJECT_TYPE_NAME(self),                                      \
				__func__);

#define XFDASHBOARD_WINDOW_TRACKER_WINDOW_X11_WARN_WRONG_WINDOW(self)          \
	g_critical("Got signal from wrong wnck window wrapped at %s in called function %s",\
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
	XfdashboardWindowTrackerWindowState			oldStates;

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

	/* Remember current states as old ones for signal emission before updating them */
	oldStates=priv->state;

	/* Update state before emitting signal */
	_xfdashboard_window_tracker_window_x11_update_state(self);

	/* Proxy signal */
	g_signal_emit_by_name(self, "state-changed", oldStates);
}

/* Proxy signal for mapped wnck window which changed actions */
static void _xfdashboard_window_tracker_window_x11_on_wnck_actions_changed(XfdashboardWindowTrackerWindowX11 *self,
																			WnckWindowActions inChangedActions,
																			WnckWindowActions inNewActions,
																			gpointer inUserData)
{
	XfdashboardWindowTrackerWindowX11Private	*priv;
	WnckWindow									*window;
	XfdashboardWindowTrackerWindowAction		oldActions;

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

	/* Remember current actions as old ones for signal emission before updating them */
	oldActions=priv->actions;

	/* Update actions before emitting signal */
	_xfdashboard_window_tracker_window_x11_update_actions(self);

	/* Proxy signal */
	g_signal_emit_by_name(self, "actions-changed", oldActions);
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

		windowTracker=xfdashboard_core_get_window_tracker(NULL);
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
		windowTracker=xfdashboard_core_get_window_tracker(NULL);

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
		/* If we have created a content for this window then remove weak reference
		 * and reset content variable to NULL. First call to get window content
		 * will recreate it. Already used contents will not be affected.
		 */
		if(priv->content)
		{
			XFDASHBOARD_DEBUG(self, WINDOWS,
								"Removing cached content with ref-count %d from %s@%p for wnck-window %p because wnck-window will change to %p",
								G_OBJECT(priv->content)->ref_count,
								G_OBJECT_TYPE_NAME(self), self,
								priv->window,
								inWindow);
			g_object_remove_weak_pointer(G_OBJECT(priv->content), (gpointer*)&priv->content);
			priv->content=NULL;
		}

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
	windowTracker=xfdashboard_core_get_window_tracker(NULL);
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
	windowTracker=xfdashboard_core_get_window_tracker(NULL);
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
		g_critical("Either no wnck workspace is wrapped at %s or workspace is not available anymore when called at function %s",
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
		g_critical("Either no wnck workspace is wrapped at %s or workspace is not available anymore when called at function %s",
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

	g_return_val_if_fail(XFDASHBOARD_IS_WINDOW_TRACKER_WINDOW_X11(inWindow), NULL);

	self=XFDASHBOARD_WINDOW_TRACKER_WINDOW_X11(inWindow);
	priv=self->priv;

	/* A wnck window must be wrapped by this object */
	if(!priv->window)
	{
		XFDASHBOARD_WINDOW_TRACKER_WINDOW_X11_WARN_NO_WINDOW(self);
		return(NULL);
	}

	/* Create content for window only if no content is already available. If it
	 * is available just return it with taking an extra reference on it.
	 */
	if(!priv->content)
	{
		priv->content=xfdashboard_window_content_x11_new_for_window(self);
		g_object_add_weak_pointer(G_OBJECT(priv->content), (gpointer*)&priv->content);
		XFDASHBOARD_DEBUG(self, WINDOWS,
							"Created content %s@%p for window %s@%p (wnck-window=%p)",
							priv->content ? G_OBJECT_TYPE_NAME(priv->content) : "<unknown>", priv->content,
							G_OBJECT_TYPE_NAME(self), self,
							priv->window);
	}
		else
		{
			g_object_ref(priv->content);
			XFDASHBOARD_DEBUG(self, WINDOWS,
								"Using cached content %s@%p (ref-count=%d) for window %s@%p (wnck-window=%p)",
								priv->content ? G_OBJECT_TYPE_NAME(priv->content) : "<unknown>", priv->content,
								priv->content ? G_OBJECT(priv->content)->ref_count : 0,
								G_OBJECT_TYPE_NAME(self), self,
								priv->window);
		}

	/* Return content */
	return(priv->content);
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
}


/* IMPLEMENTATION: GObject */

/* Dispose this object */
static void _xfdashboard_window_tracker_window_x11_dispose(GObject *inObject)
{
	XfdashboardWindowTrackerWindowX11			*self=XFDASHBOARD_WINDOW_TRACKER_WINDOW_X11(inObject);
	XfdashboardWindowTrackerWindowX11Private	*priv=self->priv;

	/* Dispose allocated resources */
	if(priv->content)
	{
		XFDASHBOARD_DEBUG(self, WINDOWS,
							"Removing cached content with ref-count %d from %s@%p for wnck-window %p",
							G_OBJECT(priv->content)->ref_count,
							G_OBJECT_TYPE_NAME(self), self,
							priv->window);
		g_object_remove_weak_pointer(G_OBJECT(priv->content), (gpointer*)&priv->content);
		priv->content=NULL;
	}

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

	/* Define properties */
	XfdashboardWindowTrackerWindowX11Properties[PROP_WINDOW]=
		g_param_spec_object("window",
							"Window",
							"The mapped wnck window",
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

	priv=self->priv=xfdashboard_window_tracker_window_x11_get_instance_private(self);

	/* Set default values */
	priv->window=NULL;
	priv->content=NULL;
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
	return(priv->window);
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
