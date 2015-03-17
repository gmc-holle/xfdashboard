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

#ifndef __XFDASHBOARD_WINDOW_TRACKER_WINDOW__
#define __XFDASHBOARD_WINDOW_TRACKER_WINDOW__

#include <clutter/clutter.h>
#include <glib-object.h>
#include <gdk/gdk.h>

#include "window-tracker-workspace.h"
#include "window-tracker-monitor.h"

G_BEGIN_DECLS

#define XFDASHBOARD_TYPE_WINDOW_TRACKER_WINDOW				(xfdashboard_window_tracker_window_get_type())
#define XFDASHBOARD_WINDOW_TRACKER_WINDOW(obj)				(G_TYPE_CHECK_INSTANCE_CAST((obj), XFDASHBOARD_TYPE_WINDOW_TRACKER_WINDOW, XfdashboardWindowTrackerWindow))
#define XFDASHBOARD_IS_WINDOW_TRACKER_WINDOW(obj)			(G_TYPE_CHECK_INSTANCE_TYPE((obj), XFDASHBOARD_TYPE_WINDOW_TRACKER_WINDOW))
#define XFDASHBOARD_WINDOW_TRACKER_WINDOW_CLASS(klass)		(G_TYPE_CHECK_CLASS_CAST((klass), XFDASHBOARD_TYPE_WINDOW_TRACKER_WINDOW, XfdashboardWindowTrackerWindowClass))
#define XFDASHBOARD_IS_WINDOW_TRACKER_WINDOW_CLASS(klass)	(G_TYPE_CHECK_CLASS_TYPE((klass), XFDASHBOARD_TYPE_WINDOW_TRACKER_WINDOW))
#define XFDASHBOARD_WINDOW_TRACKER_WINDOW_GET_CLASS(obj)	(G_TYPE_INSTANCE_GET_CLASS((obj), XFDASHBOARD_TYPE_WINDOW_TRACKER_WINDOW, XfdashboardWindowTrackerWindowClass))

typedef struct _WnckWindow									XfdashboardWindowTrackerWindow;
typedef struct _WnckWindowClass								XfdashboardWindowTrackerWindowClass;

/* Public API */
GType xfdashboard_window_tracker_window_get_type(void) G_GNUC_CONST;

gboolean xfdashboard_window_tracker_window_is_minized(XfdashboardWindowTrackerWindow *inWindow);
gboolean xfdashboard_window_tracker_window_is_visible(XfdashboardWindowTrackerWindow *inWindow);
gboolean xfdashboard_window_tracker_window_is_visible_on_workspace(XfdashboardWindowTrackerWindow *inWindow,
																	XfdashboardWindowTrackerWorkspace *inWorkspace);
gboolean xfdashboard_window_tracker_window_is_visible_on_monitor(XfdashboardWindowTrackerWindow *inWindow,
																	XfdashboardWindowTrackerMonitor *inMonitor);
void xfdashboard_window_tracker_window_show(XfdashboardWindowTrackerWindow *inWindow);
void xfdashboard_window_tracker_window_hide(XfdashboardWindowTrackerWindow *inWindow);

XfdashboardWindowTrackerWorkspace* xfdashboard_window_tracker_window_get_workspace(XfdashboardWindowTrackerWindow *inWindow);
gboolean xfdashboard_window_tracker_window_is_on_workspace(XfdashboardWindowTrackerWindow *inWindow,
															XfdashboardWindowTrackerWorkspace *inWorkspace);
void xfdashboard_window_tracker_window_move_to_workspace(XfdashboardWindowTrackerWindow *inWindow,
															XfdashboardWindowTrackerWorkspace *inWorkspace);

XfdashboardWindowTrackerMonitor* xfdashboard_window_tracker_window_get_monitor(XfdashboardWindowTrackerWindow *inWindow);
gboolean xfdashboard_window_tracker_window_is_on_monitor(XfdashboardWindowTrackerWindow *inWindow,
															XfdashboardWindowTrackerMonitor *inMonitor);

const gchar* xfdashboard_window_tracker_window_get_title(XfdashboardWindowTrackerWindow *inWindow);

GdkPixbuf* xfdashboard_window_tracker_window_get_icon(XfdashboardWindowTrackerWindow *inWindow);
const gchar* xfdashboard_window_tracker_window_get_icon_name(XfdashboardWindowTrackerWindow *inWindow);

gboolean xfdashboard_window_tracker_window_is_skip_pager(XfdashboardWindowTrackerWindow *inWindow);
gboolean xfdashboard_window_tracker_window_is_skip_tasklist(XfdashboardWindowTrackerWindow *inWindow);
gboolean xfdashboard_window_tracker_window_is_pinned(XfdashboardWindowTrackerWindow *inWindow);

gboolean xfdashboard_window_tracker_window_has_close_action(XfdashboardWindowTrackerWindow *inWindow);

void xfdashboard_window_tracker_window_activate(XfdashboardWindowTrackerWindow *inWindow);

void xfdashboard_window_tracker_window_close(XfdashboardWindowTrackerWindow *inWindow);

void xfdashboard_window_tracker_window_get_position(XfdashboardWindowTrackerWindow *inWindow, gint *outX, gint *outY);
void xfdashboard_window_tracker_window_get_size(XfdashboardWindowTrackerWindow *inWindow, gint *outWidth, gint *outHeight);
void xfdashboard_window_tracker_window_get_position_size(XfdashboardWindowTrackerWindow *inWindow,
															gint *outX, gint *outY,
															gint *outWidth, gint *outHeight);

void xfdashboard_window_tracker_window_move(XfdashboardWindowTrackerWindow *inWindow, gint inX, gint inY);
void xfdashboard_window_tracker_window_resize(XfdashboardWindowTrackerWindow *inWindow, gint inWidth, gint inHeight);
void xfdashboard_window_tracker_window_move_resize(XfdashboardWindowTrackerWindow *inWindow,
													gint inX, gint inY,
													gint inWidth, gint inHeight);

gboolean xfdashboard_window_tracker_window_is_stage(XfdashboardWindowTrackerWindow *inWindow);
ClutterStage* xfdashboard_window_tracker_window_find_stage(XfdashboardWindowTrackerWindow *inWindow);
XfdashboardWindowTrackerWindow* xfdashboard_window_tracker_window_get_stage_window(ClutterStage *inStage);

void xfdashboard_window_tracker_window_make_stage_window(XfdashboardWindowTrackerWindow *inWindow);
void xfdashboard_window_tracker_window_unmake_stage_window(XfdashboardWindowTrackerWindow *inWindow);

gulong xfdashboard_window_tracker_window_get_xid(XfdashboardWindowTrackerWindow *inWindow);

G_END_DECLS

#endif	/* __XFDASHBOARD_WINDOW_TRACKER_WINDOW__ */
