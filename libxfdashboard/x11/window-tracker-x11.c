/*
 * window-tracker: Tracks windows, workspaces, monitors and
 *                 listens for changes. It also bundles libwnck into one
 *                 class.
 *                 By wrapping libwnck objects we can use a virtual
 *                 stable API while the API in libwnck changes within versions.
 *                 We only need to use #ifdefs in window tracker object
 *                 and nowhere else in the code.
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

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <libxfdashboard/x11/window-tracker-x11.h>

#define WNCK_I_KNOW_THIS_IS_UNSTABLE
#include <libwnck/libwnck.h>

#include <glib/gi18n-lib.h>
#include <clutter/clutter.h>
#include <clutter/x11/clutter-x11.h>
#include <gdk/gdkx.h>
#ifdef HAVE_XINERAMA
#include <X11/extensions/Xinerama.h>
#endif

#include <libxfdashboard/window-tracker.h>
#include <libxfdashboard/x11/window-tracker-monitor-x11.h>
#include <libxfdashboard/x11/window-tracker-window-x11.h>
#include <libxfdashboard/x11/window-tracker-workspace-x11.h>
#include <libxfdashboard/marshal.h>
#include <libxfdashboard/application.h>
#include <libxfdashboard/compat.h>
#include <libxfdashboard/debug.h>


/* Define this class in GObject system */
static void _xfdashboard_window_tracker_x11_window_tracker_iface_init(XfdashboardWindowTrackerInterface *iface);

G_DEFINE_TYPE_WITH_CODE(XfdashboardWindowTrackerX11,
						xfdashboard_window_tracker_x11,
						G_TYPE_OBJECT,
						G_IMPLEMENT_INTERFACE(XFDASHBOARD_TYPE_WINDOW_TRACKER, _xfdashboard_window_tracker_x11_window_tracker_iface_init))

/* Private structure - access only by public API if needed */
#define XFDASHBOARD_WINDOW_TRACKER_X11_GET_PRIVATE(obj)                        \
	(G_TYPE_INSTANCE_GET_PRIVATE((obj), XFDASHBOARD_TYPE_WINDOW_TRACKER_X11, XfdashboardWindowTrackerX11Private))

struct _XfdashboardWindowTrackerX11Private
{
	/* Properties related */
	XfdashboardWindowTrackerWindowX11		*activeWindow;
	XfdashboardWindowTrackerWorkspaceX11	*activeWorkspace;
	XfdashboardWindowTrackerMonitorX11		*primaryMonitor;

	/* Instance related */
	GList									*windows;
	GList									*windowsStacked;
	GList									*workspaces;
	GList									*monitors;

	XfdashboardApplication					*application;
	gboolean								isAppSuspended;
	guint									suspendSignalID;

	WnckScreen								*screen;

	gboolean								supportsMultipleMonitors;
	GdkScreen								*gdkScreen;
};

/* Properties */
enum
{
	PROP_0,

	/* Overriden properties of interface: XfdashboardWindowTracker */
	PROP_ACTIVE_WINDOW,
	PROP_ACTIVE_WORKSPACE,
	PROP_PRIMARY_MONITOR,

	PROP_LAST
};

static GParamSpec* XfdashboardWindowTrackerX11Properties[PROP_LAST]={ 0, };


/* IMPLEMENTATION: Private variables and methods */

/* Free workspace object */
static void _xfdashboard_window_tracker_x11_free_workspace(XfdashboardWindowTrackerX11 *self,
															XfdashboardWindowTrackerWorkspaceX11 *inWorkspace)
{
	XfdashboardWindowTrackerX11Private		*priv;
	GList									*iter;

	g_return_if_fail(XFDASHBOARD_IS_WINDOW_TRACKER_X11(self));
	g_return_if_fail(XFDASHBOARD_IS_WINDOW_TRACKER_WORKSPACE_X11(inWorkspace));

	priv=self->priv;

#if DEBUG
	/* There must be only one reference on that workspace object */
	XFDASHBOARD_DEBUG(self, WINDOWS,
				"Freeing workspace %s@%p named '%s' with ref-count=%d",
				G_OBJECT_TYPE_NAME(inWorkspace), inWorkspace,
				xfdashboard_window_tracker_workspace_get_name(XFDASHBOARD_WINDOW_TRACKER_WORKSPACE(inWorkspace)),
				G_OBJECT(inWorkspace)->ref_count);
	g_assert(G_OBJECT(inWorkspace)->ref_count==1);
#endif

	/* Find entry in workspace list and remove it if found */
	iter=g_list_find(priv->workspaces, inWorkspace);
	if(iter)
	{
		priv->workspaces=g_list_delete_link(priv->workspaces, iter);
	}

	/* Free workspace object */
	g_object_unref(inWorkspace);
}

/* Get workspace object for requested wnck workspace */
static XfdashboardWindowTrackerWorkspaceX11* _xfdashboard_window_tracker_x11_get_workspace_for_wnck(XfdashboardWindowTrackerX11 *self,
																									WnckWorkspace *inWorkspace)
{
	XfdashboardWindowTrackerX11Private		*priv;
	GList									*iter;
	XfdashboardWindowTrackerWorkspaceX11	*workspace;

	g_return_val_if_fail(XFDASHBOARD_IS_WINDOW_TRACKER_X11(self), NULL);
	g_return_val_if_fail(WNCK_IS_WORKSPACE(inWorkspace), NULL);

	priv=self->priv;

	/* Iterate through list of workspace object and check if an object for the
	 * request wnck workspace exist. If one is found then return this existing
	 * workspace object.
	 */
	for(iter=priv->workspaces; iter; iter=g_list_next(iter))
	{
		/* Get currently iterated workspace object */
		workspace=XFDASHBOARD_WINDOW_TRACKER_WORKSPACE_X11(iter->data);
		if(!workspace) continue;

		/* Check if this workspace object wraps the requested wnck workspace */
		if(xfdashboard_window_tracker_workspace_x11_get_workspace(workspace)==inWorkspace)
		{
			/* Return existing workspace object */
			return(workspace);
		}
	}

	/* If we get here, return NULL as we have not found a matching workspace
	 * object for the requested wnck workspace.
	 */
	return(NULL);
}

/* Create workspace object which must not exist yet */
static XfdashboardWindowTrackerWorkspaceX11* _xfdashboard_window_tracker_x11_create_workspace_for_wnck(XfdashboardWindowTrackerX11 *self,
																										WnckWorkspace *inWorkspace)
{
	XfdashboardWindowTrackerX11Private		*priv;
	XfdashboardWindowTrackerWorkspaceX11	*workspace;

	g_return_val_if_fail(XFDASHBOARD_IS_WINDOW_TRACKER_X11(self), NULL);
	g_return_val_if_fail(WNCK_IS_WORKSPACE(inWorkspace), NULL);

	priv=self->priv;

	/* Iterate through list of workspace object and check if an object for the
	 * request wnck workspace exist. If one is found then the existing workspace object.
	 */
	workspace=_xfdashboard_window_tracker_x11_get_workspace_for_wnck(self, inWorkspace);
	if(workspace)
	{
		/* Return existing workspace object */
		XFDASHBOARD_DEBUG(self, WINDOWS,
					"A workspace object %s@%p for wnck workspace %s@%p named '%s' exists already",
					G_OBJECT_TYPE_NAME(workspace), workspace,
					G_OBJECT_TYPE_NAME(inWorkspace), inWorkspace, wnck_workspace_get_name(inWorkspace));

		return(workspace);
	}

	/* Create workspace object */
	workspace=XFDASHBOARD_WINDOW_TRACKER_WORKSPACE_X11(g_object_new(XFDASHBOARD_TYPE_WINDOW_TRACKER_WORKSPACE_X11,
														"workspace", inWorkspace,
														NULL));
	if(!workspace)
	{
		g_critical(_("Could not create workspace object of type %s for workspace '%s'"),
					g_type_name(XFDASHBOARD_TYPE_WINDOW_TRACKER_WORKSPACE_X11),
					wnck_workspace_get_name(inWorkspace));
		return(NULL);
	}

	/* Add new workspace object to list of workspace objects */
	priv->workspaces=g_list_prepend(priv->workspaces, workspace);

	/* Return new workspace object */
	XFDASHBOARD_DEBUG(self, WINDOWS,
				"Created workspace object %s@%p for wnck workspace %s@%p named '%s'",
				G_OBJECT_TYPE_NAME(workspace), workspace,
				G_OBJECT_TYPE_NAME(inWorkspace), inWorkspace, wnck_workspace_get_name(inWorkspace));
	return(workspace);
}

/* Free window object */
static void _xfdashboard_window_tracker_x11_free_window(XfdashboardWindowTrackerX11 *self,
														XfdashboardWindowTrackerWindowX11 *inWindow)
{
	XfdashboardWindowTrackerX11Private		*priv;
	GList									*iter;

	g_return_if_fail(XFDASHBOARD_IS_WINDOW_TRACKER_X11(self));
	g_return_if_fail(XFDASHBOARD_IS_WINDOW_TRACKER_WINDOW_X11(inWindow));

	priv=self->priv;

#if DEBUG
	/* There must be only one reference on that window object */
	XFDASHBOARD_DEBUG(self, WINDOWS,
				"Freeing window %s@%p named '%s' with ref-count=%d",
				G_OBJECT_TYPE_NAME(inWindow), inWindow,
				xfdashboard_window_tracker_window_get_name(XFDASHBOARD_WINDOW_TRACKER_WINDOW(inWindow)),
				G_OBJECT(inWindow)->ref_count);
	g_assert(G_OBJECT(inWindow)->ref_count==1);
#endif

	/* Find entry in window lists and remove it if found */
	iter=g_list_find(priv->windows, inWindow);
	if(iter)
	{
		priv->windows=g_list_delete_link(priv->windows, iter);
	}

	iter=g_list_find(priv->windowsStacked, inWindow);
	if(iter)
	{
		priv->windowsStacked=g_list_delete_link(priv->windowsStacked, iter);
	}

	/* Free window object */
	g_object_unref(inWindow);
}

/* Get window object for requested wnck window */
static XfdashboardWindowTrackerWindowX11* _xfdashboard_window_tracker_x11_get_window_for_wnck(XfdashboardWindowTrackerX11 *self,
																								WnckWindow *inWindow)
{
	XfdashboardWindowTrackerX11Private		*priv;
	GList									*iter;
	XfdashboardWindowTrackerWindowX11		*window;

	g_return_val_if_fail(XFDASHBOARD_IS_WINDOW_TRACKER_X11(self), NULL);
	g_return_val_if_fail(WNCK_IS_WINDOW(inWindow), NULL);

	priv=self->priv;

	/* Iterate through list of window object and check if an object for the
	 * request wnck window exist. If one is found then return this existing
	 * window object.
	 */
	for(iter=priv->windows; iter; iter=g_list_next(iter))
	{
		/* Get currently iterated window object */
		if(!XFDASHBOARD_IS_WINDOW_TRACKER_WINDOW_X11(iter->data)) continue;

		window=XFDASHBOARD_WINDOW_TRACKER_WINDOW_X11(iter->data);
		if(!window) continue;

		/* Check if this window object wraps the requested wnck window */
		if(xfdashboard_window_tracker_window_x11_get_window(window)==inWindow)
		{
			/* Return existing window object */
			return(window);
		}
	}

	/* If we get here, return NULL as we have not found a matching window
	 * object for the requested wnck window.
	 */
	return(NULL);
}

/* Build correctly ordered list of windows in stacked order. The list will not
 * take a reference at the window object and must not be unreffed if list is
 * freed.
 */
static void _xfdashboard_window_tracker_x11_build_stacked_windows_list(XfdashboardWindowTrackerX11 *self)
{
	XfdashboardWindowTrackerX11Private		*priv;
	GList									*wnckWindowsStacked;
	GList									*newWindowsStacked;
	GList									*iter;
	WnckWindow								*wnckWindow;
	XfdashboardWindowTrackerWindowX11		*window;

	g_return_if_fail(XFDASHBOARD_IS_WINDOW_TRACKER(self));

	priv=self->priv;

	/* Get list of stacked windows from wnck */
	wnckWindowsStacked=wnck_screen_get_windows_stacked(priv->screen);

	/* Build new list of stacked windows containing window objects */
	newWindowsStacked=NULL;
	for(iter=wnckWindowsStacked; iter; iter=g_list_next(iter))
	{
		/* Get wnck window iterated */
		wnckWindow=WNCK_WINDOW(iter->data);
		if(!wnckWindow) continue;

		/* Lookup window object from wnck window iterated */
		window=_xfdashboard_window_tracker_x11_get_window_for_wnck(self, wnckWindow);
		if(window)
		{
			newWindowsStacked=g_list_prepend(newWindowsStacked, window);
		}
	}
	newWindowsStacked=g_list_reverse(newWindowsStacked);

	/* Release old stacked windows list */
	g_list_free(priv->windowsStacked);
	priv->windowsStacked=newWindowsStacked;
}

/* Create window object which must not exist yet */
static XfdashboardWindowTrackerWindowX11* _xfdashboard_window_tracker_x11_create_window_for_wnck(XfdashboardWindowTrackerX11 *self,
																									WnckWindow *inWindow)
{
	XfdashboardWindowTrackerX11Private		*priv;
	XfdashboardWindowTrackerWindowX11		*window;

	g_return_val_if_fail(XFDASHBOARD_IS_WINDOW_TRACKER_X11(self), NULL);
	g_return_val_if_fail(WNCK_IS_WINDOW(inWindow), NULL);

	priv=self->priv;

	/* Iterate through list of window object and check if an object for the
	 * request wnck window exist. If one is found then the existing window object.
	 */
	window=_xfdashboard_window_tracker_x11_get_window_for_wnck(self, inWindow);
	if(window)
	{
		/* Return existing window object */
		XFDASHBOARD_DEBUG(self, WINDOWS,
					"A window object %s@%p for wnck window %s@%p named '%s' exists already",
					G_OBJECT_TYPE_NAME(window), window,
					G_OBJECT_TYPE_NAME(inWindow), inWindow, wnck_window_get_name(inWindow));

		return(window);
	}

	/* Create window object */
	window=XFDASHBOARD_WINDOW_TRACKER_WINDOW_X11(g_object_new(XFDASHBOARD_TYPE_WINDOW_TRACKER_WINDOW_X11,
																"window", inWindow,
																NULL));
	if(!window)
	{
		g_critical(_("Could not create window object of type %s for window '%s'"),
					g_type_name(XFDASHBOARD_TYPE_WINDOW_TRACKER_WINDOW_X11),
					wnck_window_get_name(inWindow));
		return(NULL);
	}

	/* Add new window object to list of window objects */
	priv->windows=g_list_prepend(priv->windows, window);

	/* Assume window stacking changed to get correctly ordered list of windows */
	_xfdashboard_window_tracker_x11_build_stacked_windows_list(self);

	/* Return new window object */
	XFDASHBOARD_DEBUG(self, WINDOWS,
				"Created window object %s@%p for wnck window %s@%p named '%s'",
				G_OBJECT_TYPE_NAME(window), window,
				G_OBJECT_TYPE_NAME(inWindow), inWindow, wnck_window_get_name(inWindow));
	return(window);
}

/* Position and/or size of window has changed */
static void _xfdashboard_window_tracker_x11_on_window_geometry_changed(XfdashboardWindowTrackerX11 *self,
																		gpointer inUserData)
{
	XfdashboardWindowTrackerWindowX11		*window;

	g_return_if_fail(XFDASHBOARD_IS_WINDOW_TRACKER(self));
	g_return_if_fail(XFDASHBOARD_IS_WINDOW_TRACKER_WINDOW_X11(inUserData));

	window=XFDASHBOARD_WINDOW_TRACKER_WINDOW_X11(inUserData);

	/* Emit signal */
	XFDASHBOARD_DEBUG(self, WINDOWS,
						"Window '%s' changed position and/or size",
						xfdashboard_window_tracker_window_get_name(XFDASHBOARD_WINDOW_TRACKER_WINDOW(window)));
	g_signal_emit_by_name(self, "window-geometry-changed", window);
}

/* Action items of window has changed */
static void _xfdashboard_window_tracker_x11_on_window_actions_changed(XfdashboardWindowTrackerX11 *self,
																		XfdashboardWindowTrackerWindowAction inChangedMask,
																		XfdashboardWindowTrackerWindowAction inNewValue,
																		gpointer inUserData)
{
	XfdashboardWindowTrackerWindowX11	*window;

	g_return_if_fail(XFDASHBOARD_IS_WINDOW_TRACKER(self));
	g_return_if_fail(XFDASHBOARD_IS_WINDOW_TRACKER_WINDOW_X11(inUserData));

	window=XFDASHBOARD_WINDOW_TRACKER_WINDOW_X11(inUserData);

	/* Emit signal */
	XFDASHBOARD_DEBUG(self, WINDOWS,
						"Window '%s' changed actions to %u with mask %u",
						xfdashboard_window_tracker_window_get_name(XFDASHBOARD_WINDOW_TRACKER_WINDOW(window)),
						inNewValue, inChangedMask);
	g_signal_emit_by_name(self, "window-actions-changed", window);
}

/* State of window has changed */
static void _xfdashboard_window_tracker_x11_on_window_state_changed(XfdashboardWindowTrackerX11 *self,
																	XfdashboardWindowTrackerWindowState inChangedMask,
																	XfdashboardWindowTrackerWindowState inNewValue,
																	gpointer inUserData)
{
	XfdashboardWindowTrackerWindowX11	*window;

	g_return_if_fail(XFDASHBOARD_IS_WINDOW_TRACKER(self));
	g_return_if_fail(XFDASHBOARD_IS_WINDOW_TRACKER_WINDOW_X11(inUserData));

	window=XFDASHBOARD_WINDOW_TRACKER_WINDOW_X11(inUserData);

	/* Emit signal */
	XFDASHBOARD_DEBUG(self, WINDOWS,
						"Window '%s' changed state to %u with mask %u",
						xfdashboard_window_tracker_window_get_name(XFDASHBOARD_WINDOW_TRACKER_WINDOW(window)),
						inNewValue, inChangedMask);
	g_signal_emit_by_name(self, "window-state-changed", window);
}

/* Icon of window has changed */
static void _xfdashboard_window_tracker_x11_on_window_icon_changed(XfdashboardWindowTrackerX11 *self,
																	gpointer inUserData)
{
	XfdashboardWindowTrackerWindowX11	*window;

	g_return_if_fail(XFDASHBOARD_IS_WINDOW_TRACKER(self));
	g_return_if_fail(XFDASHBOARD_IS_WINDOW_TRACKER_WINDOW_X11(inUserData));

	window=XFDASHBOARD_WINDOW_TRACKER_WINDOW_X11(inUserData);

	/* Emit signal */
	XFDASHBOARD_DEBUG(self, WINDOWS,
						"Window '%s' changed its icon",
						xfdashboard_window_tracker_window_get_name(XFDASHBOARD_WINDOW_TRACKER_WINDOW(window)));
	g_signal_emit_by_name(self, "window-icon-changed", window);
}

/* Name of window has changed */
static void _xfdashboard_window_tracker_x11_on_window_name_changed(XfdashboardWindowTrackerX11 *self,
																	gpointer inUserData)
{
	XfdashboardWindowTrackerWindowX11	*window;

	g_return_if_fail(XFDASHBOARD_IS_WINDOW_TRACKER(self));
	g_return_if_fail(XFDASHBOARD_IS_WINDOW_TRACKER_WINDOW_X11(inUserData));

	window=XFDASHBOARD_WINDOW_TRACKER_WINDOW_X11(inUserData);

	/* Emit signal */
	XFDASHBOARD_DEBUG(self, WINDOWS,
						"Window changed its name to '%s'",
						xfdashboard_window_tracker_window_get_name(XFDASHBOARD_WINDOW_TRACKER_WINDOW(window)));
	g_signal_emit_by_name(self, "window-name-changed", window);
}

/* A window has moved to another monitor */
static void _xfdashboard_window_tracker_x11_on_window_monitor_changed(XfdashboardWindowTrackerX11 *self,
																		XfdashboardWindowTrackerMonitor *inOldMonitor,
																		gpointer inUserData)
{
	XfdashboardWindowTrackerWindowX11		*window;
	XfdashboardWindowTrackerMonitor			*newMonitor;

	g_return_if_fail(XFDASHBOARD_IS_WINDOW_TRACKER(self));
	g_return_if_fail(!inOldMonitor || XFDASHBOARD_IS_WINDOW_TRACKER_MONITOR_X11(inOldMonitor));
	g_return_if_fail(XFDASHBOARD_IS_WINDOW_TRACKER_WINDOW_X11(inUserData));

	window=XFDASHBOARD_WINDOW_TRACKER_WINDOW_X11(inUserData);

	/* Get monitor window resides on. */
	newMonitor=xfdashboard_window_tracker_window_get_monitor(XFDASHBOARD_WINDOW_TRACKER_WINDOW(window));

	/* Emit signal */
	XFDASHBOARD_DEBUG(self, WINDOWS,
						"Window '%s' moved from monitor %d (%s) to %d (%s)",
						xfdashboard_window_tracker_window_get_name(XFDASHBOARD_WINDOW_TRACKER_WINDOW(window)),
						inOldMonitor ? xfdashboard_window_tracker_monitor_get_number(inOldMonitor) : -1,
						(inOldMonitor && xfdashboard_window_tracker_monitor_is_primary(inOldMonitor)) ? "primary" : "non-primary",
						newMonitor ? xfdashboard_window_tracker_monitor_get_number(newMonitor) : -1,
						(newMonitor && xfdashboard_window_tracker_monitor_is_primary(newMonitor)) ? "primary" : "non-primary");
	g_signal_emit_by_name(self, "window-monitor-changed", window, inOldMonitor, newMonitor);
}

/* A window has moved to another workspace */
static void _xfdashboard_window_tracker_x11_on_window_workspace_changed(XfdashboardWindowTrackerX11 *self,
																		XfdashboardWindowTrackerWorkspace *inOldWorkspace,
																		gpointer inUserData)
{
	XfdashboardWindowTrackerWindowX11		*window;
	XfdashboardWindowTrackerWorkspace		*newWorkspace;

	g_return_if_fail(XFDASHBOARD_IS_WINDOW_TRACKER(self));
	g_return_if_fail(!inOldWorkspace || XFDASHBOARD_IS_WINDOW_TRACKER_WORKSPACE_X11(inOldWorkspace));
	g_return_if_fail(XFDASHBOARD_IS_WINDOW_TRACKER_WINDOW_X11(inUserData));

	window=XFDASHBOARD_WINDOW_TRACKER_WINDOW_X11(inUserData);

	/* Get workspace window resides on. */
	newWorkspace=xfdashboard_window_tracker_window_get_workspace(XFDASHBOARD_WINDOW_TRACKER_WINDOW(window));

	/* Emit signal */
	XFDASHBOARD_DEBUG(self, WINDOWS,
						"Window '%s' moved to workspace %d (%s)",
						xfdashboard_window_tracker_window_get_name(XFDASHBOARD_WINDOW_TRACKER_WINDOW(window)),
						newWorkspace ? xfdashboard_window_tracker_workspace_get_number(newWorkspace) : -1,
						newWorkspace ? xfdashboard_window_tracker_workspace_get_name(newWorkspace) : "<nil>");
	g_signal_emit_by_name(self, "window-workspace-changed", window, newWorkspace);
}

/* A window was activated */
static void _xfdashboard_window_tracker_x11_on_active_window_changed(XfdashboardWindowTrackerX11 *self,
																		WnckWindow *inPreviousWindow,
																		gpointer inUserData)
{
	XfdashboardWindowTrackerX11Private		*priv;
	WnckScreen								*screen;
	XfdashboardWindowTrackerWindowX11		*oldActiveWindow;
	XfdashboardWindowTrackerWindowX11		*newActiveWindow;
	WnckWindow								*activeWindow;

	g_return_if_fail(XFDASHBOARD_IS_WINDOW_TRACKER(self));
	g_return_if_fail(inPreviousWindow==NULL || WNCK_IS_WINDOW(inPreviousWindow));
	g_return_if_fail(WNCK_IS_SCREEN(inUserData));

	priv=self->priv;
	screen=WNCK_SCREEN(inUserData);

	/* Get and remember new active window */
	oldActiveWindow=priv->activeWindow;

	newActiveWindow=NULL;
	activeWindow=wnck_screen_get_active_window(screen);
	if(activeWindow)
	{
		newActiveWindow=_xfdashboard_window_tracker_x11_get_window_for_wnck(self, activeWindow);
		if(!newActiveWindow)
		{
			XFDASHBOARD_DEBUG(self, WINDOWS,
						"No window object of type %s found for new active wnck window %s@%p named '%s'",
						g_type_name(XFDASHBOARD_TYPE_WINDOW_TRACKER_WINDOW_X11),
						G_OBJECT_TYPE_NAME(activeWindow), activeWindow, wnck_window_get_name(activeWindow));

			return;
		}
	}

	priv->activeWindow=newActiveWindow;

	/* Emit signal */
	XFDASHBOARD_DEBUG(self, WINDOWS,
						"Active window changed from '%s' to '%s'",
						oldActiveWindow ? xfdashboard_window_tracker_window_get_name(XFDASHBOARD_WINDOW_TRACKER_WINDOW(oldActiveWindow)) : "<nil>",
						newActiveWindow ? xfdashboard_window_tracker_window_get_name(XFDASHBOARD_WINDOW_TRACKER_WINDOW(newActiveWindow)) : "<nil>");
	g_signal_emit_by_name(self, "active-window-changed", oldActiveWindow, priv->activeWindow);
}

/* A window was closed */
static void _xfdashboard_window_tracker_x11_on_window_closed(XfdashboardWindowTrackerX11 *self,
																WnckWindow *inWindow,
																gpointer inUserData)
{
	XfdashboardWindowTrackerX11Private		*priv;
	XfdashboardWindowTrackerWindowX11		*window;

	g_return_if_fail(XFDASHBOARD_IS_WINDOW_TRACKER(self));
	g_return_if_fail(WNCK_IS_WINDOW(inWindow));
	g_return_if_fail(WNCK_IS_SCREEN(inUserData));

	priv=self->priv;

	/* Should not happen but if closed window is the last active known one, then
	 * reset to NULL
	 */
	if(xfdashboard_window_tracker_window_x11_get_window(priv->activeWindow)==inWindow)
	{
		priv->activeWindow=NULL;
	}

	/* Get window object for closed wnck window */
	window=_xfdashboard_window_tracker_x11_get_window_for_wnck(self, inWindow);
	if(!window)
	{
		XFDASHBOARD_DEBUG(self, WINDOWS,
					"No window object of type %s found for wnck window %s@%p named '%s'",
					g_type_name(XFDASHBOARD_TYPE_WINDOW_TRACKER_WINDOW_X11),
					G_OBJECT_TYPE_NAME(inWindow), inWindow, wnck_window_get_name(inWindow));

		return;
	}

	/* Remove all signal handlers for closed window */
	g_signal_handlers_disconnect_by_data(window, self);

	/* Emit signals */
	XFDASHBOARD_DEBUG(self, WINDOWS,
						"Window '%s' closed",
						wnck_window_get_name(inWindow));
	g_signal_emit_by_name(self, "window-closed", window);

	/* Remove window from window list */
	_xfdashboard_window_tracker_x11_free_window(self, window);
}

/* A new window was opened */
static void _xfdashboard_window_tracker_x11_on_window_opened(XfdashboardWindowTrackerX11 *self,
																WnckWindow *inWindow,
																gpointer inUserData)
{
	XfdashboardWindowTrackerX11Private		*priv;
	XfdashboardWindowTrackerWindowX11		*window;

	g_return_if_fail(XFDASHBOARD_IS_WINDOW_TRACKER(self));
	g_return_if_fail(WNCK_IS_WINDOW(inWindow));
	g_return_if_fail(WNCK_IS_SCREEN(inUserData));

	priv=self->priv;

	/* Create window object */
	window=_xfdashboard_window_tracker_x11_create_window_for_wnck(self, inWindow);
	if(!window) return;

	/* Connect signals on newly opened window */
	g_signal_connect_swapped(window, "actions-changed", G_CALLBACK(_xfdashboard_window_tracker_x11_on_window_actions_changed), self);
	g_signal_connect_swapped(window, "state-changed", G_CALLBACK(_xfdashboard_window_tracker_x11_on_window_state_changed), self);
	g_signal_connect_swapped(window, "icon-changed", G_CALLBACK(_xfdashboard_window_tracker_x11_on_window_icon_changed), self);
	g_signal_connect_swapped(window, "name-changed", G_CALLBACK(_xfdashboard_window_tracker_x11_on_window_name_changed), self);
	g_signal_connect_swapped(window, "monitor-changed", G_CALLBACK(_xfdashboard_window_tracker_x11_on_window_monitor_changed), self);
	g_signal_connect_swapped(window, "workspace-changed", G_CALLBACK(_xfdashboard_window_tracker_x11_on_window_workspace_changed), self);
	g_signal_connect_swapped(window, "geometry-changed", G_CALLBACK(_xfdashboard_window_tracker_x11_on_window_geometry_changed), self);

	/* Block signal handler for 'geometry-changed' at window if application is suspended */
	if(priv->isAppSuspended)
	{
		g_signal_handlers_block_by_func(window, _xfdashboard_window_tracker_x11_on_window_geometry_changed, self);
	}

	/* Emit signal */
	XFDASHBOARD_DEBUG(self, WINDOWS,
				"Window '%s' created",
				wnck_window_get_name(inWindow));
	g_signal_emit_by_name(self, "window-opened", window);
}

/* Window stacking has changed */
static void _xfdashboard_window_tracker_x11_on_window_stacking_changed(XfdashboardWindowTrackerX11 *self,
																		gpointer inUserData)
{
	g_return_if_fail(XFDASHBOARD_IS_WINDOW_TRACKER(self));

	/* Before emitting the signal, build a correctly ordered list of windows */
	_xfdashboard_window_tracker_x11_build_stacked_windows_list(self);

	/* Emit signal */
	XFDASHBOARD_DEBUG(self, WINDOWS, "Window stacking has changed");
	g_signal_emit_by_name(self, "window-stacking-changed");
}

/* A workspace changed its name */
static void _xfdashboard_window_tracker_x11_on_workspace_name_changed(XfdashboardWindowTrackerX11 *self,
																		gpointer inUserData)
{
	XfdashboardWindowTrackerWorkspaceX11	*workspace;

	g_return_if_fail(XFDASHBOARD_IS_WINDOW_TRACKER(self));
	g_return_if_fail(XFDASHBOARD_IS_WINDOW_TRACKER_WORKSPACE_X11(inUserData));

	workspace=XFDASHBOARD_WINDOW_TRACKER_WORKSPACE_X11(inUserData);

	/* Emit signal */
	XFDASHBOARD_DEBUG(self, WINDOWS,
						"Workspace #%d changed name to '%s'",
						xfdashboard_window_tracker_workspace_get_number(XFDASHBOARD_WINDOW_TRACKER_WORKSPACE(workspace)),
						xfdashboard_window_tracker_workspace_get_name(XFDASHBOARD_WINDOW_TRACKER_WORKSPACE(workspace)));
	g_signal_emit_by_name(self, "workspace-name-changed", workspace);

}

/* A workspace was activated */
static void _xfdashboard_window_tracker_x11_on_active_workspace_changed(XfdashboardWindowTrackerX11 *self,
																		WnckWorkspace *inPreviousWorkspace,
																		gpointer inUserData)
{
	XfdashboardWindowTrackerX11Private		*priv;
	WnckScreen								*screen;
	XfdashboardWindowTrackerWorkspaceX11	*oldActiveWorkspace;
	XfdashboardWindowTrackerWorkspaceX11	*newActiveWorkspace;
	WnckWorkspace							*activeWorkspace;

	g_return_if_fail(XFDASHBOARD_IS_WINDOW_TRACKER(self));
	g_return_if_fail(inPreviousWorkspace==NULL || WNCK_IS_WORKSPACE(inPreviousWorkspace));
	g_return_if_fail(WNCK_IS_SCREEN(inUserData));

	priv=self->priv;
	screen=WNCK_SCREEN(inUserData);

	/* Get and remember new active workspace */
	oldActiveWorkspace=priv->activeWorkspace;

	newActiveWorkspace=NULL;
	activeWorkspace=wnck_screen_get_active_workspace(screen);
	if(activeWorkspace)
	{
		newActiveWorkspace=_xfdashboard_window_tracker_x11_get_workspace_for_wnck(self, activeWorkspace);
		if(!newActiveWorkspace)
		{
			XFDASHBOARD_DEBUG(self, WINDOWS,
						"No workspace object of type %s found for new active wnck workspace %s@%p named '%s'",
						g_type_name(XFDASHBOARD_TYPE_WINDOW_TRACKER_WORKSPACE_X11),
						G_OBJECT_TYPE_NAME(activeWorkspace), activeWorkspace, wnck_workspace_get_name(activeWorkspace));

			return;
		}
	}

	priv->activeWorkspace=newActiveWorkspace;

	/* Emit signal */
	XFDASHBOARD_DEBUG(self, WINDOWS,
						"Active workspace changed from #%d (%s) to #%d (%s)",
						oldActiveWorkspace ? wnck_workspace_get_number(inPreviousWorkspace) : -1,
						oldActiveWorkspace ? wnck_workspace_get_name(inPreviousWorkspace) : "<nil>",
						priv->activeWorkspace ? wnck_workspace_get_number(activeWorkspace) : -1,
						priv->activeWorkspace ? wnck_workspace_get_name(activeWorkspace) : "<nil>");
	g_signal_emit_by_name(self, "active-workspace-changed", oldActiveWorkspace, priv->activeWorkspace);
}

/* A workspace was destroyed */
static void _xfdashboard_window_tracker_x11_on_workspace_destroyed(XfdashboardWindowTrackerX11 *self,
																	WnckWorkspace *inWorkspace,
																	gpointer inUserData)
{
	XfdashboardWindowTrackerX11Private		*priv;
	XfdashboardWindowTrackerWorkspaceX11	*workspace;

	g_return_if_fail(XFDASHBOARD_IS_WINDOW_TRACKER(self));
	g_return_if_fail(WNCK_IS_WORKSPACE(inWorkspace));
	g_return_if_fail(WNCK_IS_SCREEN(inUserData));

	priv=self->priv;

	/* Should not happen but if destroyed workspace is the last active known one,
	 * then reset to NULL
	 */
	if(xfdashboard_window_tracker_workspace_x11_get_workspace(priv->activeWorkspace)==inWorkspace)
	{
		priv->activeWorkspace=NULL;
	}

	/* Get workspace object for wnck workspace */
	workspace=_xfdashboard_window_tracker_x11_get_workspace_for_wnck(self, inWorkspace);
	if(!workspace)
	{
		XFDASHBOARD_DEBUG(self, WINDOWS,
					"No workspace object of type %s found for wnck workspace %s@%p named '%s'",
					g_type_name(XFDASHBOARD_TYPE_WINDOW_TRACKER_WINDOW_X11),
					G_OBJECT_TYPE_NAME(inWorkspace), inWorkspace, wnck_workspace_get_name(inWorkspace));

		return;
	}

	/* Remove all signal handlers for closed window */
	g_signal_handlers_disconnect_by_data(workspace, self);

	/* Emit signal */
	XFDASHBOARD_DEBUG(self, WINDOWS,
						"Workspace #%d (%s) destroyed",
						wnck_workspace_get_number(inWorkspace),
						wnck_workspace_get_name(inWorkspace));
	g_signal_emit_by_name(self, "workspace-removed", workspace);

	/* Remove workspace from workspace list */
	_xfdashboard_window_tracker_x11_free_workspace(self, workspace);
}

/* A new workspace was created */
static void _xfdashboard_window_tracker_x11_on_workspace_created(XfdashboardWindowTrackerX11 *self,
																	WnckWorkspace *inWorkspace,
																	gpointer inUserData)
{
	XfdashboardWindowTrackerWorkspaceX11		*workspace;

	g_return_if_fail(XFDASHBOARD_IS_WINDOW_TRACKER(self));
	g_return_if_fail(WNCK_IS_WORKSPACE(inWorkspace));
	g_return_if_fail(WNCK_IS_SCREEN(inUserData));

	/* Create workspace object */
	workspace=_xfdashboard_window_tracker_x11_create_workspace_for_wnck(self, inWorkspace);
	if(!workspace) return;

	/* Connect signals on newly created workspace */
	g_signal_connect_swapped(workspace, "name-changed", G_CALLBACK(_xfdashboard_window_tracker_x11_on_workspace_name_changed), self);

	/* Emit signal */
	XFDASHBOARD_DEBUG(self, WINDOWS,
						"New workspace #%d (%s) created",
						wnck_workspace_get_number(inWorkspace),
						wnck_workspace_get_name(inWorkspace));
	g_signal_emit_by_name(self, "workspace-added", workspace);
}

/* Primary monitor has changed */
static void _xfdashboard_window_tracker_x11_on_primary_monitor_changed(XfdashboardWindowTrackerX11 *self,
																		gpointer inUserData)
{
	XfdashboardWindowTrackerX11Private		*priv;
	XfdashboardWindowTrackerMonitorX11		*monitor;

	g_return_if_fail(XFDASHBOARD_IS_WINDOW_TRACKER(self));
	g_return_if_fail(XFDASHBOARD_IS_WINDOW_TRACKER_MONITOR_X11(inUserData));

	priv=self->priv;
	monitor=XFDASHBOARD_WINDOW_TRACKER_MONITOR_X11(inUserData);

	/* If monitor emitting this signal is the (new) primary one
	 * then update primary monitor value of this instance.
	 */
	if(xfdashboard_window_tracker_monitor_is_primary(XFDASHBOARD_WINDOW_TRACKER_MONITOR(monitor)) &&
		priv->primaryMonitor!=monitor)
	{
		XfdashboardWindowTrackerMonitorX11	*oldMonitor;

		/* Remember old monitor for signal emission */
		oldMonitor=priv->primaryMonitor;

		/* Set value */
		priv->primaryMonitor=monitor;

		/* Emit signal */
		g_signal_emit_by_name(self, "primary-monitor-changed", oldMonitor, priv->primaryMonitor);

		/* Notify about property change */
		g_object_notify_by_pspec(G_OBJECT(self), XfdashboardWindowTrackerX11Properties[PROP_PRIMARY_MONITOR]);

		XFDASHBOARD_DEBUG(self, WINDOWS,
							"Primary monitor changed from %d to %d",
							oldMonitor ? xfdashboard_window_tracker_monitor_get_number(XFDASHBOARD_WINDOW_TRACKER_MONITOR(oldMonitor)) : -1,
							xfdashboard_window_tracker_monitor_get_number(XFDASHBOARD_WINDOW_TRACKER_MONITOR(monitor)));
	}
}

/* A monitor has changed its position and/or size */
static void _xfdashboard_window_tracker_x11_on_monitor_geometry_changed(XfdashboardWindowTrackerX11 *self,
																		gpointer inUserData)
{
	XfdashboardWindowTrackerMonitorX11			*monitor;

	g_return_if_fail(XFDASHBOARD_IS_WINDOW_TRACKER(self));
	g_return_if_fail(XFDASHBOARD_IS_WINDOW_TRACKER_MONITOR_X11(inUserData));

	monitor=XFDASHBOARD_WINDOW_TRACKER_MONITOR_X11(inUserData);

	/* A monitor changed its position and/or size so re-emit the signal */
	g_signal_emit_by_name(self, "monitor-geometry-changed", monitor);
}

/* Create a monitor object */
static XfdashboardWindowTrackerMonitorX11* _xfdashboard_window_tracker_x11_monitor_new(XfdashboardWindowTrackerX11 *self,
																						guint inMonitorIndex)
{
	XfdashboardWindowTrackerX11Private		*priv;
	XfdashboardWindowTrackerMonitorX11		*monitor;

	g_return_val_if_fail(XFDASHBOARD_IS_WINDOW_TRACKER(self), NULL);
	g_return_val_if_fail(inMonitorIndex>=g_list_length(self->priv->monitors), NULL);

	priv=self->priv;

	/* Create monitor object */
	monitor=XFDASHBOARD_WINDOW_TRACKER_MONITOR_X11(g_object_new(XFDASHBOARD_TYPE_WINDOW_TRACKER_MONITOR_X11,
															"monitor-index", inMonitorIndex,
															NULL));
	priv->monitors=g_list_append(priv->monitors, monitor);

	/* Connect signals */
	g_signal_connect_swapped(monitor,
								"primary-changed",
								G_CALLBACK(_xfdashboard_window_tracker_x11_on_primary_monitor_changed),
								self);
	g_signal_connect_swapped(monitor,
								"geometry-changed",
								G_CALLBACK(_xfdashboard_window_tracker_x11_on_monitor_geometry_changed),
								self);

	/* Emit signal */
	g_signal_emit_by_name(self, "monitor-added", monitor);
	XFDASHBOARD_DEBUG(self, WINDOWS,
						"Monitor %d added",
						inMonitorIndex);

	/* If we newly added monitor is the primary one then emit signal. We could not
	 * have done it yet because the signals were connect to new monitor object
	 * after its creation.
	 */
	if(xfdashboard_window_tracker_monitor_is_primary(XFDASHBOARD_WINDOW_TRACKER_MONITOR(monitor)))
	{
		_xfdashboard_window_tracker_x11_on_primary_monitor_changed(self, monitor);
	}

	/* Return newly created monitor */
	return(monitor);
}

/* Free a monitor object */
static void _xfdashboard_window_tracker_x11_monitor_free(XfdashboardWindowTrackerX11 *self,
															XfdashboardWindowTrackerMonitorX11 *inMonitor)
{
	XfdashboardWindowTrackerX11Private		*priv;
	GList									*iter;

	g_return_if_fail(XFDASHBOARD_IS_WINDOW_TRACKER_X11(self));
	g_return_if_fail(XFDASHBOARD_IS_WINDOW_TRACKER_MONITOR_X11(inMonitor));

	priv=self->priv;

	/* Find monitor to free */
	iter=g_list_find(priv->monitors,  inMonitor);
	if(!iter)
	{
		g_critical(_("Cannot release unknown monitor %d"),
					xfdashboard_window_tracker_monitor_get_number(XFDASHBOARD_WINDOW_TRACKER_MONITOR(inMonitor)));
		return;
	}

	/* Disconnect signals */
	g_signal_handlers_disconnect_by_data(inMonitor, self);

	/* Emit signal */
	g_signal_emit_by_name(self, "monitor-removed", inMonitor);
	XFDASHBOARD_DEBUG(self, WINDOWS,
						"Monitor %d removed",
						xfdashboard_window_tracker_monitor_get_number(XFDASHBOARD_WINDOW_TRACKER_MONITOR(inMonitor)));

	/* Remove monitor object from list */
	priv->monitors=g_list_delete_link(priv->monitors, iter);

	/* Unref monitor object. Usually this is the last reference released
	 * and the object will be destroyed.
	 */
	g_object_unref(inMonitor);
}

#ifdef HAVE_XINERAMA
/* Number of monitors, primary monitor or size of any monitor changed */
static void _xfdashboard_window_tracker_x11_on_monitors_changed(XfdashboardWindowTrackerX11 *self,
																gpointer inUserData)
{
	XfdashboardWindowTrackerX11Private			*priv;
	GdkScreen								*screen;
	gint									currentMonitorCount;
	gint									newMonitorCount;
	gint									i;
	XfdashboardWindowTrackerMonitorX11		*monitor;
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
		XFDASHBOARD_DEBUG(self, WINDOWS,
							"Number of monitors changed from %d to %d",
							currentMonitorCount,
							newMonitorCount);
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
			_xfdashboard_window_tracker_x11_monitor_new(self, i);
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

			monitor=XFDASHBOARD_WINDOW_TRACKER_MONITOR_X11(iter->data);

			/* Free monitor object */
			_xfdashboard_window_tracker_x11_monitor_free(self, monitor);
		}
	}
}
#endif

/* Total size of screen changed */
static void _xfdashboard_window_tracker_x11_on_screen_size_changed(XfdashboardWindowTrackerX11 *self,
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
	XFDASHBOARD_DEBUG(self, WINDOWS,
						"Screen size changed to %dx%d",
						w,
						h);
	g_signal_emit_by_name(self, "screen-size-changed", w, h);
}

/* Suspension state of application changed */
static void _xfdashboard_window_tracker_x11_on_application_suspended_changed(XfdashboardWindowTrackerX11 *self,
																				GParamSpec *inSpec,
																				gpointer inUserData)
{
	XfdashboardWindowTrackerX11Private		*priv;
	XfdashboardApplication					*app;
	GList									*iter;
	XfdashboardWindowTrackerWindow			*window;

	g_return_if_fail(XFDASHBOARD_IS_WINDOW_TRACKER(self));
	g_return_if_fail(XFDASHBOARD_IS_APPLICATION(inUserData));

	priv=self->priv;
	app=XFDASHBOARD_APPLICATION(inUserData);

	/* Get application suspend state */
	priv->isAppSuspended=xfdashboard_application_is_suspended(app);

	/* Iterate through all windows and connect handler to signal 'geometry-changed'
	 * if application was resumed or disconnect signal handler if it was suspended.
	 */
	for(iter=xfdashboard_window_tracker_get_windows(XFDASHBOARD_WINDOW_TRACKER(self)); iter; iter=g_list_next(iter))
	{
		/* Get window */
		window=XFDASHBOARD_WINDOW_TRACKER_WINDOW(iter->data);
		if(!window) continue;

		/* If application was suspended disconnect signal handlers ... */
		if(priv->isAppSuspended)
		{
			g_signal_handlers_block_by_func(window, _xfdashboard_window_tracker_x11_on_window_geometry_changed, self);
		}
			/* ... otherwise if application was resumed reconnect signals handlers
			 * and emit 'geometry-changed' signal to reflect latest changes of
			 * position and size of window.
			 */
			else
			{
				/* Reconnect signal handler */
				g_signal_handlers_unblock_by_func(window, _xfdashboard_window_tracker_x11_on_window_geometry_changed, self);

				/* Call signal handler to reflect latest changes */
				_xfdashboard_window_tracker_x11_on_window_geometry_changed(self, window);
			}
	}
}


/* IMPLEMENTATION: Interface XfdashboardWindowTracker */

/* Get list of all windows (if wanted in stack order) */
static GList* _xfdashboard_window_tracker_x11_window_tracker_get_windows(XfdashboardWindowTracker *inWindowTracker)
{
	XfdashboardWindowTrackerX11				*self;
	XfdashboardWindowTrackerX11Private		*priv;

	g_return_val_if_fail(XFDASHBOARD_IS_WINDOW_TRACKER_X11(inWindowTracker), NULL);

	self=XFDASHBOARD_WINDOW_TRACKER_X11(inWindowTracker);
	priv=self->priv;

	/* Return list of window objects created */
	return(priv->windows);
}

/* Get list of all windows in stack order */
static GList* _xfdashboard_window_tracker_x11_window_tracker_get_windows_stacked(XfdashboardWindowTracker *inWindowTracker)
{
	XfdashboardWindowTrackerX11				*self;
	XfdashboardWindowTrackerX11Private		*priv;

	g_return_val_if_fail(XFDASHBOARD_IS_WINDOW_TRACKER_X11(inWindowTracker), NULL);

	self=XFDASHBOARD_WINDOW_TRACKER_X11(inWindowTracker);
	priv=self->priv;

	/* Return list of window in stack order */
	return(priv->windowsStacked);
}

/* Get active window */
static XfdashboardWindowTrackerWindow* _xfdashboard_window_tracker_x11_window_tracker_get_active_window(XfdashboardWindowTracker *inWindowTracker)
{
	XfdashboardWindowTrackerX11				*self;
	XfdashboardWindowTrackerX11Private		*priv;

	g_return_val_if_fail(XFDASHBOARD_IS_WINDOW_TRACKER_X11(inWindowTracker), NULL);

	self=XFDASHBOARD_WINDOW_TRACKER_X11(inWindowTracker);
	priv=self->priv;

	return(XFDASHBOARD_WINDOW_TRACKER_WINDOW(priv->activeWindow));
}

/* Get number of workspaces */
static gint _xfdashboard_window_tracker_x11_window_tracker_get_workspaces_count(XfdashboardWindowTracker *inWindowTracker)
{
	XfdashboardWindowTrackerX11				*self;
	XfdashboardWindowTrackerX11Private		*priv;

	g_return_val_if_fail(XFDASHBOARD_IS_WINDOW_TRACKER_X11(inWindowTracker), 0);

	self=XFDASHBOARD_WINDOW_TRACKER_X11(inWindowTracker);
	priv=self->priv;

	/* Return number of workspaces */
	return(wnck_screen_get_workspace_count(priv->screen));
}

/* Get list of workspaces */
static GList* _xfdashboard_window_tracker_x11_window_tracker_get_workspaces(XfdashboardWindowTracker *inWindowTracker)
{
	XfdashboardWindowTrackerX11				*self;
	XfdashboardWindowTrackerX11Private		*priv;

	g_return_val_if_fail(XFDASHBOARD_IS_WINDOW_TRACKER_X11(inWindowTracker), NULL);

	self=XFDASHBOARD_WINDOW_TRACKER_X11(inWindowTracker);
	priv=self->priv;

	/* Return list of workspaces */
	return(priv->workspaces);
}

/* Get active workspace */
static XfdashboardWindowTrackerWorkspace* _xfdashboard_window_tracker_x11_window_tracker_get_active_workspace(XfdashboardWindowTracker *inWindowTracker)
{
	XfdashboardWindowTrackerX11				*self;
	XfdashboardWindowTrackerX11Private		*priv;

	g_return_val_if_fail(XFDASHBOARD_IS_WINDOW_TRACKER_X11(inWindowTracker), NULL);

	self=XFDASHBOARD_WINDOW_TRACKER_X11(inWindowTracker);
	priv=self->priv;

	return(XFDASHBOARD_WINDOW_TRACKER_WORKSPACE(priv->activeWorkspace));
}

/* Get workspace by number */
static XfdashboardWindowTrackerWorkspace* _xfdashboard_window_tracker_x11_window_tracker_get_workspace_by_number(XfdashboardWindowTracker *inWindowTracker,
																															gint inNumber)
{
	XfdashboardWindowTrackerX11				*self;
	XfdashboardWindowTrackerX11Private		*priv;
	WnckWorkspace							*wnckWorkspace;
	XfdashboardWindowTrackerWorkspaceX11	*workspace;

	g_return_val_if_fail(XFDASHBOARD_IS_WINDOW_TRACKER_X11(inWindowTracker), NULL);

	self=XFDASHBOARD_WINDOW_TRACKER_X11(inWindowTracker);
	priv=self->priv;

	g_return_val_if_fail(inNumber>=0 && inNumber<wnck_screen_get_workspace_count(priv->screen), NULL);

	/* Get wnck workspace by number */
	wnckWorkspace=wnck_screen_get_workspace(priv->screen, inNumber);

	/* Get workspace object for wnck workspace */
	workspace=_xfdashboard_window_tracker_x11_get_workspace_for_wnck(self, wnckWorkspace);
	if(!workspace)
	{
		XFDASHBOARD_DEBUG(self, WINDOWS,
					"No workspace object of type %s found for wnck workspace %s@%p named '%s'",
					g_type_name(XFDASHBOARD_TYPE_WINDOW_TRACKER_WINDOW_X11),
					G_OBJECT_TYPE_NAME(wnckWorkspace), wnckWorkspace, wnck_workspace_get_name(wnckWorkspace));

		return(NULL);
	}

	/* Return workspace object */
	return(XFDASHBOARD_WINDOW_TRACKER_WORKSPACE(workspace));
}

/* Determine if multiple monitors are supported */
static gboolean _xfdashboard_window_tracker_x11_window_tracker_supports_multiple_monitors(XfdashboardWindowTracker *inWindowTracker)
{
	XfdashboardWindowTrackerX11				*self;
	XfdashboardWindowTrackerX11Private		*priv;

	g_return_val_if_fail(XFDASHBOARD_IS_WINDOW_TRACKER_X11(inWindowTracker), FALSE);

	self=XFDASHBOARD_WINDOW_TRACKER_X11(inWindowTracker);
	priv=self->priv;

	return(priv->supportsMultipleMonitors);
}

/* Get number of monitors */
static gint _xfdashboard_window_tracker_x11_window_tracker_get_monitors_count(XfdashboardWindowTracker *inWindowTracker)
{
	XfdashboardWindowTrackerX11				*self;
	XfdashboardWindowTrackerX11Private		*priv;

	g_return_val_if_fail(XFDASHBOARD_IS_WINDOW_TRACKER_X11(inWindowTracker), 0);

	self=XFDASHBOARD_WINDOW_TRACKER_X11(inWindowTracker);
	priv=self->priv;

	/* Return number of monitors */
	return(g_list_length(priv->monitors));
}

/* Get list of monitors */
static GList* _xfdashboard_window_tracker_x11_window_tracker_get_monitors(XfdashboardWindowTracker *inWindowTracker)
{
	XfdashboardWindowTrackerX11				*self;
	XfdashboardWindowTrackerX11Private		*priv;

	g_return_val_if_fail(XFDASHBOARD_IS_WINDOW_TRACKER_X11(inWindowTracker), NULL);

	self=XFDASHBOARD_WINDOW_TRACKER_X11(inWindowTracker);
	priv=self->priv;

	/* Return list of workspaces */
	return(priv->monitors);
}

/* Get primary monitor */
static XfdashboardWindowTrackerMonitor* _xfdashboard_window_tracker_x11_window_tracker_get_primary_monitor(XfdashboardWindowTracker *inWindowTracker)
{
	XfdashboardWindowTrackerX11				*self;
	XfdashboardWindowTrackerX11Private		*priv;

	g_return_val_if_fail(XFDASHBOARD_IS_WINDOW_TRACKER_X11(inWindowTracker), NULL);

	self=XFDASHBOARD_WINDOW_TRACKER_X11(inWindowTracker);
	priv=self->priv;

	return(XFDASHBOARD_WINDOW_TRACKER_MONITOR(priv->primaryMonitor));
}

/* Get monitor by number */
static XfdashboardWindowTrackerMonitor* _xfdashboard_window_tracker_x11_window_tracker_get_monitor_by_number(XfdashboardWindowTracker *inWindowTracker,
																														gint inNumber)
{
	XfdashboardWindowTrackerX11				*self;
	XfdashboardWindowTrackerX11Private		*priv;

	g_return_val_if_fail(XFDASHBOARD_IS_WINDOW_TRACKER_X11(inWindowTracker), NULL);

	self=XFDASHBOARD_WINDOW_TRACKER_X11(inWindowTracker);
	priv=self->priv;

	g_return_val_if_fail(inNumber>=0, NULL);
	g_return_val_if_fail(((guint)inNumber)<g_list_length(priv->monitors), NULL);

	/* Return monitor at index */
	return(XFDASHBOARD_WINDOW_TRACKER_MONITOR(g_list_nth_data(priv->monitors, inNumber)));
}

/* Get monitor at requested position */
static XfdashboardWindowTrackerMonitor* _xfdashboard_window_tracker_x11_window_tracker_get_monitor_by_position(XfdashboardWindowTracker *inWindowTracker,
																														gint inX,
																														gint inY)
{
	XfdashboardWindowTrackerX11				*self;
	XfdashboardWindowTrackerX11Private		*priv;
	GList									*iter;
	XfdashboardWindowTrackerMonitorX11		*monitor;

	g_return_val_if_fail(XFDASHBOARD_IS_WINDOW_TRACKER_X11(inWindowTracker), NULL);

	self=XFDASHBOARD_WINDOW_TRACKER_X11(inWindowTracker);
	priv=self->priv;

	/* Iterate through monitors and return the one containing the requested position */
	for(iter=priv->monitors; iter; iter=g_list_next(iter))
	{
		/* Get monitor at iterator */
		monitor=XFDASHBOARD_WINDOW_TRACKER_MONITOR_X11(iter->data);
		if(!monitor) continue;

		/* Check if this monitor contains the requested position. If it does
		 * then return it.
		 */
		if(xfdashboard_window_tracker_monitor_contains(XFDASHBOARD_WINDOW_TRACKER_MONITOR(monitor), inX, inY))
		{
			return(XFDASHBOARD_WINDOW_TRACKER_MONITOR(monitor));
		}
	}

	/* If we get here none of the monitors contains the requested position,
	 * so return NULL here.
	 */
	return(NULL);
}

/* Get size of screen */
static void _xfdashboard_window_tracker_x11_window_tracker_get_screen_size(XfdashboardWindowTracker *inWindowTracker,
																					gint *outWidth,
																					gint *outHeight)
{
	XfdashboardWindowTrackerX11				*self;
	XfdashboardWindowTrackerX11Private		*priv;
	gint									width, height;

	g_return_if_fail(XFDASHBOARD_IS_WINDOW_TRACKER_X11(inWindowTracker));

	self=XFDASHBOARD_WINDOW_TRACKER_X11(inWindowTracker);
	priv=self->priv;

	/* Get width and height of screen */
	width=gdk_screen_get_width(priv->gdkScreen);
	height=gdk_screen_get_height(priv->gdkScreen);

	/* Store result */
	if(outWidth) *outWidth=width;
	if(outHeight) *outHeight=height;
}

/* Get root (desktop) window */
static XfdashboardWindowTrackerWindow* _xfdashboard_window_tracker_x11_window_tracker_get_root_window(XfdashboardWindowTracker *inWindowTracker)
{
	XfdashboardWindowTrackerX11				*self;
	XfdashboardWindowTrackerX11Private		*priv;
	gulong									backgroundWindowID;
	GList									*windows;
	XfdashboardWindowTrackerWindowX11		*window;

	g_return_val_if_fail(XFDASHBOARD_IS_WINDOW_TRACKER_X11(inWindowTracker), NULL);

	self=XFDASHBOARD_WINDOW_TRACKER_X11(inWindowTracker);
	priv=self->priv;

	/* Find and return root window (the desktop) by known ID */
	backgroundWindowID=wnck_screen_get_background_pixmap(priv->screen);
	if(backgroundWindowID)
	{
		WnckWindow							*backgroundWindow;

		backgroundWindow=wnck_window_get(backgroundWindowID);
		if(backgroundWindow)
		{
			XFDASHBOARD_DEBUG(self, WINDOWS,
						"Found desktop window %s@%p by known background pixmap ID",
						G_OBJECT_TYPE_NAME(backgroundWindow), backgroundWindow);

			/* Get or create window object for wnck background window */
			window=_xfdashboard_window_tracker_x11_create_window_for_wnck(self, backgroundWindow);
			XFDASHBOARD_DEBUG(self, WINDOWS,
						"Resolved desktop window %s@%p to window object %s@%p",
						G_OBJECT_TYPE_NAME(backgroundWindow), backgroundWindow,
						G_OBJECT_TYPE_NAME(window), window);

			/* Return window object found or created */
			return(XFDASHBOARD_WINDOW_TRACKER_WINDOW(window));
		}
	}

	/* Either there was no known ID for the root window or the root window
	 * could not be found (happened a lot when running in daemon mode).
	 * So iterate through list of all known windows and lookup window of
	 * type 'desktop'.
	 */
	for(windows=wnck_screen_get_windows(priv->screen); windows; windows=g_list_next(windows))
	{
		WnckWindow					*wnckWindow;
		WnckWindowType				wnckWindowType;

		wnckWindow=(WnckWindow*)windows->data;
		wnckWindowType=wnck_window_get_window_type(wnckWindow);
		if(wnckWindowType==WNCK_WINDOW_DESKTOP)
		{
			XFDASHBOARD_DEBUG(self, WINDOWS,
						"Desktop window %s@%p found while iterating through window list",
						G_OBJECT_TYPE_NAME(wnckWindow), wnckWindow);

			/* Get or create window object for wnck background window */
			window=_xfdashboard_window_tracker_x11_create_window_for_wnck(self, wnckWindow);
			XFDASHBOARD_DEBUG(self, WINDOWS,
						"Resolved desktop window %s@%p to window object %s@%p",
						G_OBJECT_TYPE_NAME(wnckWindow), wnckWindow,
						G_OBJECT_TYPE_NAME(window), window);

			/* Return window object found or created */
			return(XFDASHBOARD_WINDOW_TRACKER_WINDOW(window));
		}
	}

	/* If we get here either desktop window does not exist or is not known
	 * in window list. So return NULL here.
	 */
	XFDASHBOARD_DEBUG(self, WINDOWS, "Desktop window could not be found");
	return(NULL);
}

/* Get window of stage */
static XfdashboardWindowTrackerWindow* _xfdashboard_window_tracker_x11_window_tracker_get_stage_window(XfdashboardWindowTracker *inWindowTracker,
																										ClutterStage *inStage)
{
	XfdashboardWindowTrackerX11				*self;
	Window									stageXWindow;
	WnckWindow								*wnckWindow;
	XfdashboardWindowTrackerWindowX11		*window;

	g_return_val_if_fail(XFDASHBOARD_IS_WINDOW_TRACKER_X11(inWindowTracker), NULL);
	g_return_val_if_fail(CLUTTER_IS_STAGE(inStage), NULL);

	self=XFDASHBOARD_WINDOW_TRACKER_X11(inWindowTracker);

	/* Get stage X window and translate to needed window type */
	stageXWindow=clutter_x11_get_stage_window(inStage);
	wnckWindow=wnck_window_get(stageXWindow);

	/* Get or create window object for wnck background window */
	window=_xfdashboard_window_tracker_x11_create_window_for_wnck(self, wnckWindow);
	XFDASHBOARD_DEBUG(self, WINDOWS,
				"Resolved stage window %s@%p to window object %s@%p",
				G_OBJECT_TYPE_NAME(wnckWindow), wnckWindow,
				G_OBJECT_TYPE_NAME(window), window);

	return(XFDASHBOARD_WINDOW_TRACKER_WINDOW(window));
}

/* Interface initialization
 * Set up default functions
 */
static void _xfdashboard_window_tracker_x11_window_tracker_iface_init(XfdashboardWindowTrackerInterface *iface)
{
	iface->get_windows=_xfdashboard_window_tracker_x11_window_tracker_get_windows;
	iface->get_windows_stacked=_xfdashboard_window_tracker_x11_window_tracker_get_windows_stacked;
	iface->get_active_window=_xfdashboard_window_tracker_x11_window_tracker_get_active_window;

	iface->get_workspaces_count=_xfdashboard_window_tracker_x11_window_tracker_get_workspaces_count;
	iface->get_workspaces=_xfdashboard_window_tracker_x11_window_tracker_get_workspaces;
	iface->get_active_workspace=_xfdashboard_window_tracker_x11_window_tracker_get_active_workspace;
	iface->get_workspace_by_number=_xfdashboard_window_tracker_x11_window_tracker_get_workspace_by_number;

	iface->supports_multiple_monitors=_xfdashboard_window_tracker_x11_window_tracker_supports_multiple_monitors;
	iface->get_monitors_count=_xfdashboard_window_tracker_x11_window_tracker_get_monitors_count;
	iface->get_monitors=_xfdashboard_window_tracker_x11_window_tracker_get_monitors;
	iface->get_primary_monitor=_xfdashboard_window_tracker_x11_window_tracker_get_primary_monitor;
	iface->get_monitor_by_number=_xfdashboard_window_tracker_x11_window_tracker_get_monitor_by_number;
	iface->get_monitor_by_position=_xfdashboard_window_tracker_x11_window_tracker_get_monitor_by_position;

	iface->get_screen_size=_xfdashboard_window_tracker_x11_window_tracker_get_screen_size;

	iface->get_root_window=_xfdashboard_window_tracker_x11_window_tracker_get_root_window;
	iface->get_stage_window=_xfdashboard_window_tracker_x11_window_tracker_get_stage_window;
}


/* IMPLEMENTATION: GObject */

/* Dispose this object */
static void _xfdashboard_window_tracker_x11_dispose_free_window(gpointer inData,
																gpointer inUserData)
{
	XfdashboardWindowTrackerX11				*self;
	XfdashboardWindowTrackerWindowX11		*window;

	g_return_if_fail(XFDASHBOARD_IS_WINDOW_TRACKER_WINDOW_X11(inData));
	g_return_if_fail(XFDASHBOARD_IS_WINDOW_TRACKER_X11(inUserData));

	self=XFDASHBOARD_WINDOW_TRACKER_X11(inUserData);
	window=XFDASHBOARD_WINDOW_TRACKER_WINDOW_X11(inData);

	/* Unreference window */
	_xfdashboard_window_tracker_x11_free_window(self, window);
}

static void _xfdashboard_window_tracker_x11_dispose_free_workspace(gpointer inData,
																	gpointer inUserData)
{
	XfdashboardWindowTrackerX11				*self;
	XfdashboardWindowTrackerWorkspaceX11	*workspace;

	g_return_if_fail(XFDASHBOARD_IS_WINDOW_TRACKER_WORKSPACE_X11(inData));
	g_return_if_fail(XFDASHBOARD_IS_WINDOW_TRACKER_X11(inUserData));

	self=XFDASHBOARD_WINDOW_TRACKER_X11(inUserData);
	workspace=XFDASHBOARD_WINDOW_TRACKER_WORKSPACE_X11(inData);

	/* Unreference workspace */
	_xfdashboard_window_tracker_x11_free_workspace(self, workspace);
}

static void _xfdashboard_window_tracker_x11_dispose_free_monitor(gpointer inData,
																	gpointer inUserData)
{
	XfdashboardWindowTrackerX11				*self;
	XfdashboardWindowTrackerMonitorX11		*monitor;

	g_return_if_fail(XFDASHBOARD_IS_WINDOW_TRACKER_MONITOR_X11(inData));
	g_return_if_fail(XFDASHBOARD_IS_WINDOW_TRACKER_X11(inUserData));

	self=XFDASHBOARD_WINDOW_TRACKER_X11(inUserData);
	monitor=XFDASHBOARD_WINDOW_TRACKER_MONITOR_X11(inData);

	/* Unreference monitor */
	_xfdashboard_window_tracker_x11_monitor_free(self, monitor);
}

static void _xfdashboard_window_tracker_x11_dispose(GObject *inObject)
{
	XfdashboardWindowTrackerX11				*self=XFDASHBOARD_WINDOW_TRACKER_X11(inObject);
	XfdashboardWindowTrackerX11Private		*priv=self->priv;

	/* Dispose allocated resources */
	if(priv->suspendSignalID)
	{
		if(priv->application)
		{
			g_signal_handler_disconnect(priv->application, priv->suspendSignalID);
			priv->application=NULL;
		}

		priv->suspendSignalID=0;
	}

	if(priv->activeWindow)
	{
		priv->activeWindow=NULL;
	}

	if(priv->windows)
	{
		g_list_foreach(priv->windows, _xfdashboard_window_tracker_x11_dispose_free_window, self);
		g_list_free(priv->windows);
		priv->windows=NULL;
	}

	if(priv->windowsStacked)
	{
		g_list_free(priv->windowsStacked);
		priv->windowsStacked=NULL;
	}

	if(priv->activeWorkspace)
	{
		priv->activeWorkspace=NULL;
	}

	if(priv->workspaces)
	{
		g_list_foreach(priv->workspaces, _xfdashboard_window_tracker_x11_dispose_free_workspace, self);
		g_list_free(priv->workspaces);
		priv->workspaces=NULL;
	}

	if(priv->primaryMonitor)
	{
		priv->primaryMonitor=NULL;
	}

	if(priv->monitors)
	{
		g_list_foreach(priv->monitors, _xfdashboard_window_tracker_x11_dispose_free_monitor, self);
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
		g_signal_handlers_disconnect_by_data(priv->screen, self);
		priv->screen=NULL;
	}

	/* Call parent's class dispose method */
	G_OBJECT_CLASS(xfdashboard_window_tracker_x11_parent_class)->dispose(inObject);
}

/* Set/get properties */
static void _xfdashboard_window_tracker_x11_set_property(GObject *inObject,
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

static void _xfdashboard_window_tracker_x11_get_property(GObject *inObject,
															guint inPropID,
															GValue *outValue,
															GParamSpec *inSpec)
{
	XfdashboardWindowTrackerX11			*self=XFDASHBOARD_WINDOW_TRACKER_X11(inObject);

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
void xfdashboard_window_tracker_x11_class_init(XfdashboardWindowTrackerX11Class *klass)
{
	GObjectClass						*gobjectClass=G_OBJECT_CLASS(klass);
	XfdashboardWindowTracker			*trackerIface;
	GParamSpec							*paramSpec;

	/* Reference interface type to lookup properties etc. */
	trackerIface=g_type_default_interface_ref(XFDASHBOARD_TYPE_WINDOW_TRACKER);

	/* Override functions */
	gobjectClass->dispose=_xfdashboard_window_tracker_x11_dispose;
	gobjectClass->set_property=_xfdashboard_window_tracker_x11_set_property;
	gobjectClass->get_property=_xfdashboard_window_tracker_x11_get_property;

	/* Set up private structure */
	g_type_class_add_private(klass, sizeof(XfdashboardWindowTrackerX11Private));

	/* Define properties */
	paramSpec=g_object_interface_find_property(trackerIface, "active-window");
	XfdashboardWindowTrackerX11Properties[PROP_ACTIVE_WINDOW]=
		g_param_spec_override("active-window", paramSpec);

	paramSpec=g_object_interface_find_property(trackerIface, "active-workspace");
	XfdashboardWindowTrackerX11Properties[PROP_ACTIVE_WORKSPACE]=
		g_param_spec_override("active-workspace", paramSpec);

	paramSpec=g_object_interface_find_property(trackerIface, "primary-monitor");
	XfdashboardWindowTrackerX11Properties[PROP_PRIMARY_MONITOR]=
		g_param_spec_override("primary-monitor", paramSpec);

	g_object_class_install_properties(gobjectClass, PROP_LAST, XfdashboardWindowTrackerX11Properties);

	/* Release allocated resources */
	g_type_default_interface_unref(trackerIface);
}

/* Object initialization
 * Create private structure and set up default values
 */
void xfdashboard_window_tracker_x11_init(XfdashboardWindowTrackerX11 *self)
{
	XfdashboardWindowTrackerX11Private		*priv;

	priv=self->priv=XFDASHBOARD_WINDOW_TRACKER_X11_GET_PRIVATE(self);

	XFDASHBOARD_DEBUG(self, WINDOWS, "Initializing window tracker");

	/* Set default values */
	priv->windows=NULL;
	priv->windowsStacked=NULL;
	priv->workspaces=NULL;
	priv->monitors=NULL;
	priv->screen=wnck_screen_get_default();
	priv->gdkScreen=gdk_screen_get_default();
	priv->activeWindow=NULL;
	priv->activeWorkspace=NULL;
	priv->primaryMonitor=NULL;
	priv->supportsMultipleMonitors=FALSE;

	/* The very first call to libwnck should be setting the client type */
	wnck_set_client_type(WNCK_CLIENT_TYPE_PAGER);

	/* Connect signals to screen */
	g_signal_connect_swapped(priv->screen,
								"window-stacking-changed",
								G_CALLBACK(_xfdashboard_window_tracker_x11_on_window_stacking_changed),
								self);

	g_signal_connect_swapped(priv->screen,
								"window-closed",
								G_CALLBACK(_xfdashboard_window_tracker_x11_on_window_closed),
								self);
	g_signal_connect_swapped(priv->screen,
								"window-opened",
								G_CALLBACK(_xfdashboard_window_tracker_x11_on_window_opened),
								self);
	g_signal_connect_swapped(priv->screen,
								"active-window-changed",
								G_CALLBACK(_xfdashboard_window_tracker_x11_on_active_window_changed),
								self);

	g_signal_connect_swapped(priv->screen,
								"workspace-destroyed",
								G_CALLBACK(_xfdashboard_window_tracker_x11_on_workspace_destroyed),
								self);
	g_signal_connect_swapped(priv->screen,
								"workspace-created",
								G_CALLBACK(_xfdashboard_window_tracker_x11_on_workspace_created),
								self);
	g_signal_connect_swapped(priv->screen,
								"active-workspace-changed",
								G_CALLBACK(_xfdashboard_window_tracker_x11_on_active_workspace_changed),
								self);

	g_signal_connect_swapped(priv->gdkScreen,
								"size-changed",
								G_CALLBACK(_xfdashboard_window_tracker_x11_on_screen_size_changed),
								self);

#ifdef HAVE_XINERAMA
	/* Check if multiple monitors are supported */
	if(XineramaIsActive(GDK_SCREEN_XDISPLAY(priv->gdkScreen)))
	{
		XfdashboardWindowTrackerMonitorX11	*monitor;
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
								G_CALLBACK(_xfdashboard_window_tracker_x11_on_monitors_changed),
								self,
								NULL,
								G_CONNECT_AFTER | G_CONNECT_SWAPPED);

		/* Get monitors */
		for(i=0; i<gdk_screen_get_n_monitors(priv->gdkScreen); i++)
		{
			/* Create monitor object */
			monitor=_xfdashboard_window_tracker_x11_monitor_new(self, i);

			/* Remember primary monitor */
			if(xfdashboard_window_tracker_monitor_is_primary(XFDASHBOARD_WINDOW_TRACKER_MONITOR(monitor)))
			{
				priv->primaryMonitor=monitor;
			}
		}
	}
#endif

	/* Handle suspension signals from application */
	priv->application=xfdashboard_application_get_default();
	priv->suspendSignalID=g_signal_connect_swapped(priv->application,
													"notify::is-suspended",
													G_CALLBACK(_xfdashboard_window_tracker_x11_on_application_suspended_changed),
													self);
	priv->isAppSuspended=xfdashboard_application_is_suspended(priv->application);
}


/* IMPLEMENTATION: Public API */

/* Get last timestamp for use in libwnck */
guint32 xfdashboard_window_tracker_x11_get_time(void)
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
	XFDASHBOARD_DEBUG(NULL, WINDOWS, "No timestamp for windows - trying timestamp of last X11 event in Clutter");
	timestamp=(guint32)clutter_x11_get_current_event_time();
	if(timestamp!=0)
	{
		XFDASHBOARD_DEBUG(NULL, WINDOWS,
							"Got timestamp %u of last X11 event in Clutter",
							timestamp);
		return(timestamp);
	}

	/* Last resort is to get X11 server time via stage windows */
	XFDASHBOARD_DEBUG(NULL, WINDOWS, "No timestamp for windows - trying last resort via stage windows");

	display=gdk_display_get_default();
	if(!display)
	{
		XFDASHBOARD_DEBUG(NULL, WINDOWS, "No default display found in GDK to get timestamp for windows");
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
				XFDASHBOARD_DEBUG(NULL, WINDOWS,
									"No GDK window found for stage %p to get timestamp for windows",
									stage);
				continue;
			}

			/* Check if GDK window supports GDK_PROPERTY_CHANGE_MASK event
			 * or application (or worst X server) will hang
			 */
			eventMask=gdk_window_get_events(window);
			if(!(eventMask & GDK_PROPERTY_CHANGE_MASK))
			{
				XFDASHBOARD_DEBUG(NULL, WINDOWS,
									"GDK window %p for stage %p does not support GDK_PROPERTY_CHANGE_MASK to get timestamp for windows",
									window,
									stage);
				continue;
			}

			timestamp=gdk_x11_get_server_time(window);
		}
	}
	g_slist_free(stages);

	/* Return timestamp of last resort */
	XFDASHBOARD_DEBUG(NULL, WINDOWS,
						"Last resort timestamp for windows %s (%u)",
						timestamp ? "found" : "not found",
						timestamp);
	return(timestamp);
}

/* Find and return XfdashboardWindowTrackerWindow object for mapped wnck window */
XfdashboardWindowTrackerWindow* xfdashboard_window_tracker_x11_get_window_for_wnck(XfdashboardWindowTrackerX11 *self,
																					WnckWindow *inWindow)
{
	XfdashboardWindowTrackerWindowX11		*window;

	g_return_val_if_fail(XFDASHBOARD_IS_WINDOW_TRACKER_X11(self), NULL);
	g_return_val_if_fail(WNCK_IS_WINDOW(inWindow), NULL);

	/* Lookup window object for requested wnck window and return it */
	window=_xfdashboard_window_tracker_x11_get_window_for_wnck(self, inWindow);
	return(XFDASHBOARD_WINDOW_TRACKER_WINDOW(window));
}

/* Find and return XfdashboardWindowTrackerWorkspace object for mapped wnck workspace */
XfdashboardWindowTrackerWorkspace* xfdashboard_window_tracker_x11_get_workspace_for_wnck(XfdashboardWindowTrackerX11 *self,
																							WnckWorkspace *inWorkspace)
{
	XfdashboardWindowTrackerWorkspaceX11	*workspace;

	g_return_val_if_fail(XFDASHBOARD_IS_WINDOW_TRACKER_X11(self), NULL);
	g_return_val_if_fail(WNCK_IS_WORKSPACE(inWorkspace), NULL);

	/* Lookup workspace object for requested wnck workspace and return it */
	workspace=_xfdashboard_window_tracker_x11_get_workspace_for_wnck(self, inWorkspace);
	return(XFDASHBOARD_WINDOW_TRACKER_WORKSPACE(workspace));
}
