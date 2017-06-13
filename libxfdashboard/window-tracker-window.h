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

#ifndef __LIBXFDASHBOARD_WINDOW_TRACKER_WINDOW__
#define __LIBXFDASHBOARD_WINDOW_TRACKER_WINDOW__

#if !defined(__LIBXFDASHBOARD_H_INSIDE__) && !defined(LIBXFDASHBOARD_COMPILATION)
#error "Only <libxfdashboard/libxfdashboard.h> can be included directly."
#endif

#include <clutter/clutter.h>
#include <glib-object.h>
#include <gdk/gdk.h>

#include <libxfdashboard/window-tracker-workspace.h>
#include <libxfdashboard/window-tracker-monitor.h>

G_BEGIN_DECLS

/* Public definitions */
/**
 * XfdashboardWindowTrackerWindowState:
 * @XFDASHBOARD_WINDOW_TRACKER_WINDOW_STATE_HIDDEN: The window is not visible on its
 *                                                  #XfdashboardWindowTrackerWorkspace,
 *                                                  e.g. when minimized.
 * @XFDASHBOARD_WINDOW_TRACKER_WINDOW_STATE_MINIMIZED: The window is minimized.
 * @XFDASHBOARD_WINDOW_TRACKER_WINDOW_STATE_MAXIMIZED: The window is maximized.
 * @XFDASHBOARD_WINDOW_TRACKER_WINDOW_STATE_FULLSCREEN: The window is fullscreen.
 * @XFDASHBOARD_WINDOW_TRACKER_WINDOW_STATE_SKIP_PAGER: The window should not be included on pagers.
 * @XFDASHBOARD_WINDOW_TRACKER_WINDOW_STATE_SKIP_TASKLIST: The window should not be included on tasklists.
 * @XFDASHBOARD_WINDOW_TRACKER_WINDOW_STATE_PINNED: The window is on all workspaces.
 * @XFDASHBOARD_WINDOW_TRACKER_WINDOW_STATE_URGENT: The window requires a response from the user.
 *
 * Type used as a bitmask to describe the state of a #XfdashboardWindowTrackerWindow.
 */
typedef enum /*< flags,prefix=XFDASHBOARD_WINDOW_TRACKER_WINDOW_STATE >*/
{
	XFDASHBOARD_WINDOW_TRACKER_WINDOW_STATE_HIDDEN=1 << 0,
	XFDASHBOARD_WINDOW_TRACKER_WINDOW_STATE_MINIMIZED=1 << 1,
	XFDASHBOARD_WINDOW_TRACKER_WINDOW_STATE_MAXIMIZED=1 << 2,
	XFDASHBOARD_WINDOW_TRACKER_WINDOW_STATE_FULLSCREEN=1 << 3,
	XFDASHBOARD_WINDOW_TRACKER_WINDOW_STATE_SKIP_PAGER=1 << 4,
	XFDASHBOARD_WINDOW_TRACKER_WINDOW_STATE_SKIP_TASKLIST=1 << 5,
	XFDASHBOARD_WINDOW_TRACKER_WINDOW_STATE_PINNED=1 << 6,
	XFDASHBOARD_WINDOW_TRACKER_WINDOW_STATE_URGENT=1 << 7,
} XfdashboardWindowTrackerWindowState;

/**
 * XfdashboardWindowTrackerWindowAction:
 * @XFDASHBOARD_WINDOW_TRACKER_WINDOW_ACTION_CLOSE: The window may be closed.
 *
 * Type used as a bitmask to describe the actions that can be done for a #XfdashboardWindowTrackerWindow.
 */
typedef enum /*< flags,prefix=XFDASHBOARD_WINDOW_TRACKER_WINDOW_ACTION >*/
{
	XFDASHBOARD_WINDOW_TRACKER_WINDOW_ACTION_CLOSE=1 << 0,
} XfdashboardWindowTrackerWindowAction;


/* Object declaration */
#define XFDASHBOARD_TYPE_WINDOW_TRACKER_WINDOW				(xfdashboard_window_tracker_window_get_type())
#define XFDASHBOARD_WINDOW_TRACKER_WINDOW(obj)				(G_TYPE_CHECK_INSTANCE_CAST((obj), XFDASHBOARD_TYPE_WINDOW_TRACKER_WINDOW, XfdashboardWindowTrackerWindow))
#define XFDASHBOARD_IS_WINDOW_TRACKER_WINDOW(obj)			(G_TYPE_CHECK_INSTANCE_TYPE((obj), XFDASHBOARD_TYPE_WINDOW_TRACKER_WINDOW))
#define XFDASHBOARD_WINDOW_TRACKER_WINDOW_GET_IFACE(obj)	(G_TYPE_INSTANCE_GET_INTERFACE((obj), XFDASHBOARD_TYPE_WINDOW_TRACKER_WINDOW, XfdashboardWindowTrackerWindowInterface))

typedef struct _XfdashboardWindowTrackerWindow				XfdashboardWindowTrackerWindow;
typedef struct _XfdashboardWindowTrackerWindowInterface		XfdashboardWindowTrackerWindowInterface;

struct _XfdashboardWindowTrackerWindowInterface
{
	/*< private >*/
	/* Parent interface */
	GTypeInterface						parent_interface;

	/*< public >*/
	/* Virtual functions */
	gboolean (*is_equal)(XfdashboardWindowTrackerWindow *inLeft, XfdashboardWindowTrackerWindow *inRight);

	gboolean (*is_visible)(XfdashboardWindowTrackerWindow *self);
	void (*show)(XfdashboardWindowTrackerWindow *self);
	void (*hide)(XfdashboardWindowTrackerWindow *self);

	XfdashboardWindowTrackerWindow* (*get_parent)(XfdashboardWindowTrackerWindow *self);

	XfdashboardWindowTrackerWindowState (*get_state)(XfdashboardWindowTrackerWindow *self);
	XfdashboardWindowTrackerWindowAction (*get_actions)(XfdashboardWindowTrackerWindow *self);

	const gchar* (*get_name)(XfdashboardWindowTrackerWindow *self);

	GdkPixbuf* (*get_icon)(XfdashboardWindowTrackerWindow *self);
	const gchar* (*get_icon_name)(XfdashboardWindowTrackerWindow *self);

	XfdashboardWindowTrackerWorkspace* (*get_workspace)(XfdashboardWindowTrackerWindow *self);
	gboolean (*is_on_workspace)(XfdashboardWindowTrackerWindow *self, XfdashboardWindowTrackerWorkspace *inWorkspace);

	XfdashboardWindowTrackerMonitor* (*get_monitor)(XfdashboardWindowTrackerWindow *self);
	gboolean (*is_on_monitor)(XfdashboardWindowTrackerWindow *self, XfdashboardWindowTrackerMonitor *inMonitor);

	void (*get_geometry)(XfdashboardWindowTrackerWindow *self, gint *outX, gint *outY, gint *outWidth, gint *outHeight);
	void (*set_geometry)(XfdashboardWindowTrackerWindow *self, gint inX, gint inY, gint inWidth, gint inHeight);
	void (*move)(XfdashboardWindowTrackerWindow *self, gint inX, gint inY);
	void (*resize)(XfdashboardWindowTrackerWindow *self, gint inWidth, gint inHeight);
	void (*move_to_workspace)(XfdashboardWindowTrackerWindow *self, XfdashboardWindowTrackerWorkspace *inWorkspace);
	void (*activate)(XfdashboardWindowTrackerWindow *self);
	void (*close)(XfdashboardWindowTrackerWindow *self);

	gint (*get_pid)(XfdashboardWindowTrackerWindow *self);
	gchar** (*get_instance_names)(XfdashboardWindowTrackerWindow *self);

	ClutterContent* (*get_content)(XfdashboardWindowTrackerWindow *self);

	ClutterStage* (*get_stage)(XfdashboardWindowTrackerWindow *self);

	/* Signals */
	void (*name_changed)(XfdashboardWindowTrackerWindow *self);
	void (*state_changed)(XfdashboardWindowTrackerWindow *self,
							XfdashboardWindowTrackerWindowState inChangedStates,
							XfdashboardWindowTrackerWindowState inNewState);
	void (*actions_changed)(XfdashboardWindowTrackerWindow *self,
							XfdashboardWindowTrackerWindowAction inChangedActions,
							XfdashboardWindowTrackerWindowAction inNewActions);
	void (*icon_changed)(XfdashboardWindowTrackerWindow *self);
	void (*workspace_changed)(XfdashboardWindowTrackerWindow *self,
								XfdashboardWindowTrackerWorkspace *inOldWorkspace);
	void (*monitor_changed)(XfdashboardWindowTrackerWindow *self,
							XfdashboardWindowTrackerMonitor *inOldMonitor);
	void (*geometry_changed)(XfdashboardWindowTrackerWindow *self);
};

/* Public API */
GType xfdashboard_window_tracker_window_get_type(void) G_GNUC_CONST;

gboolean xfdashboard_window_tracker_window_is_equal(XfdashboardWindowTrackerWindow *inLeft,
													XfdashboardWindowTrackerWindow *inRight);

gboolean xfdashboard_window_tracker_window_is_visible(XfdashboardWindowTrackerWindow *self);
gboolean xfdashboard_window_tracker_window_is_visible_on_workspace(XfdashboardWindowTrackerWindow *self,
																	XfdashboardWindowTrackerWorkspace *inWorkspace);
gboolean xfdashboard_window_tracker_window_is_visible_on_monitor(XfdashboardWindowTrackerWindow *self,
																	XfdashboardWindowTrackerMonitor *inMonitor);
void xfdashboard_window_tracker_window_show(XfdashboardWindowTrackerWindow *self);
void xfdashboard_window_tracker_window_hide(XfdashboardWindowTrackerWindow *self);

XfdashboardWindowTrackerWindow* xfdashboard_window_tracker_window_get_parent(XfdashboardWindowTrackerWindow *self);

XfdashboardWindowTrackerWindowState xfdashboard_window_tracker_window_get_state(XfdashboardWindowTrackerWindow *self);
XfdashboardWindowTrackerWindowAction xfdashboard_window_tracker_window_get_actions(XfdashboardWindowTrackerWindow *self);

const gchar* xfdashboard_window_tracker_window_get_name(XfdashboardWindowTrackerWindow *self);

GdkPixbuf* xfdashboard_window_tracker_window_get_icon(XfdashboardWindowTrackerWindow *self);
const gchar* xfdashboard_window_tracker_window_get_icon_name(XfdashboardWindowTrackerWindow *self);

XfdashboardWindowTrackerWorkspace* xfdashboard_window_tracker_window_get_workspace(XfdashboardWindowTrackerWindow *self);
gboolean xfdashboard_window_tracker_window_is_on_workspace(XfdashboardWindowTrackerWindow *self,
															XfdashboardWindowTrackerWorkspace *inWorkspace);
void xfdashboard_window_tracker_window_move_to_workspace(XfdashboardWindowTrackerWindow *self,
															XfdashboardWindowTrackerWorkspace *inWorkspace);

XfdashboardWindowTrackerMonitor* xfdashboard_window_tracker_window_get_monitor(XfdashboardWindowTrackerWindow *self);
gboolean xfdashboard_window_tracker_window_is_on_monitor(XfdashboardWindowTrackerWindow *self,
															XfdashboardWindowTrackerMonitor *inMonitor);

void xfdashboard_window_tracker_window_get_geometry(XfdashboardWindowTrackerWindow *self,
															gint *outX,
															gint *outY,
															gint *outWidth,
															gint *outHeight);
void xfdashboard_window_tracker_window_set_geometry(XfdashboardWindowTrackerWindow *self,
															gint inX,
															gint inY,
															gint inWidth,
															gint inHeight);
void xfdashboard_window_tracker_window_move(XfdashboardWindowTrackerWindow *self,
											gint inX,
											gint inY);
void xfdashboard_window_tracker_window_resize(XfdashboardWindowTrackerWindow *self,
												gint inWidth,
												gint inHeight);

void xfdashboard_window_tracker_window_activate(XfdashboardWindowTrackerWindow *self);
void xfdashboard_window_tracker_window_close(XfdashboardWindowTrackerWindow *self);

gboolean xfdashboard_window_tracker_window_is_stage(XfdashboardWindowTrackerWindow *self);
ClutterStage* xfdashboard_window_tracker_window_get_stage(XfdashboardWindowTrackerWindow *self);
void xfdashboard_window_tracker_window_show_stage(XfdashboardWindowTrackerWindow *self);
void xfdashboard_window_tracker_window_hide_stage(XfdashboardWindowTrackerWindow *self);

gint xfdashboard_window_tracker_window_get_pid(XfdashboardWindowTrackerWindow *self);
gchar** xfdashboard_window_tracker_window_get_instance_names(XfdashboardWindowTrackerWindow *self);

ClutterContent* xfdashboard_window_tracker_window_get_content(XfdashboardWindowTrackerWindow *self);

G_END_DECLS

#endif	/* __LIBXFDASHBOARD_WINDOW_TRACKER_WINDOW__ */
