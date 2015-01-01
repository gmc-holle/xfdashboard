/*
 * window-tracker-workspace: A workspace tracked by window tracker and
 *                           also a wrapper class around WnckWorkspace.
 *                           By wrapping libwnck objects we can use a 
 *                           virtual stable API while the API in libwnck
 *                           changes within versions. We only need to
 *                           use #ifdefs in window tracker object and
 *                           nowhere else in the code.
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

#ifndef __XFDASHBOARD_WINDOW_TRACKER_WORKSPACE__
#define __XFDASHBOARD_WINDOW_TRACKER_WORKSPACE__

#include <glib-object.h>

G_BEGIN_DECLS

#define XFDASHBOARD_TYPE_WINDOW_TRACKER_WORKSPACE				(xfdashboard_window_tracker_workspace_get_type())
#define XFDASHBOARD_WINDOW_TRACKER_WORKSPACE(obj)				(G_TYPE_CHECK_INSTANCE_CAST((obj), XFDASHBOARD_TYPE_WINDOW_TRACKER_WORKSPACE, XfdashboardWindowTrackerWorkspace))
#define XFDASHBOARD_IS_WINDOW_TRACKER_WORKSPACE(obj)			(G_TYPE_CHECK_INSTANCE_TYPE((obj), XFDASHBOARD_TYPE_WINDOW_TRACKER_WORKSPACE))
#define XFDASHBOARD_WINDOW_TRACKER_WORKSPACE_CLASS(klass)		(G_TYPE_CHECK_CLASS_CAST((klass), XFDASHBOARD_TYPE_WINDOW_TRACKER_WORKSPACE, XfdashboardWindowTrackerWorkspaceClass))
#define XFDASHBOARD_IS_WINDOW_TRACKER_WORKSPACE_CLASS(klass)	(G_TYPE_CHECK_CLASS_TYPE((klass), XFDASHBOARD_TYPE_WINDOW_TRACKER_WORKSPACE))
#define XFDASHBOARD_WINDOW_TRACKER_WORKSPACE_GET_CLASS(obj)		(G_TYPE_INSTANCE_GET_CLASS((obj), XFDASHBOARD_TYPE_WINDOW_TRACKER_WORKSPACE, XfdashboardWindowTrackerWorkspaceClass))

typedef struct _WnckWorkspace									XfdashboardWindowTrackerWorkspace;
typedef struct _WnckWorkspaceClass								XfdashboardWindowTrackerWorkspaceClass;

/* Public API */
GType xfdashboard_window_tracker_workspace_get_type(void) G_GNUC_CONST;

gint xfdashboard_window_tracker_workspace_get_number(XfdashboardWindowTrackerWorkspace *inWorkspace);
const gchar* xfdashboard_window_tracker_workspace_get_name(XfdashboardWindowTrackerWorkspace *inWorkspace);

gint xfdashboard_window_tracker_workspace_get_width(XfdashboardWindowTrackerWorkspace *inWorkspace);
gint xfdashboard_window_tracker_workspace_get_height(XfdashboardWindowTrackerWorkspace *inWorkspace);
void xfdashboard_window_tracker_workspace_get_size(XfdashboardWindowTrackerWorkspace *inWorkspace,
													gint *outWidth,
													gint *outHeight);

void xfdashboard_window_tracker_workspace_activate(XfdashboardWindowTrackerWorkspace *inWorkspace);

G_END_DECLS

#endif	/* __XFDASHBOARD_WINDOW_TRACKER_WORKSPACE__ */
