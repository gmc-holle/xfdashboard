/*
 * window-tracker-workspace: A workspace tracked by window tracker.
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

#ifndef __LIBXFDASHBOARD_WINDOW_TRACKER_WORKSPACE__
#define __LIBXFDASHBOARD_WINDOW_TRACKER_WORKSPACE__

#if !defined(__LIBXFDASHBOARD_H_INSIDE__) && !defined(LIBXFDASHBOARD_COMPILATION)
#error "Only <libxfdashboard/libxfdashboard.h> can be included directly."
#endif

#include <glib-object.h>

G_BEGIN_DECLS

#define XFDASHBOARD_TYPE_WINDOW_TRACKER_WORKSPACE				(xfdashboard_window_tracker_workspace_get_type())
#define XFDASHBOARD_WINDOW_TRACKER_WORKSPACE(obj)				(G_TYPE_CHECK_INSTANCE_CAST((obj), XFDASHBOARD_TYPE_WINDOW_TRACKER_WORKSPACE, XfdashboardWindowTrackerWorkspace))
#define XFDASHBOARD_IS_WINDOW_TRACKER_WORKSPACE(obj)			(G_TYPE_CHECK_INSTANCE_TYPE((obj), XFDASHBOARD_TYPE_WINDOW_TRACKER_WORKSPACE))
#define XFDASHBOARD_WINDOW_TRACKER_WORKSPACE_GET_IFACE(obj)		(G_TYPE_INSTANCE_GET_INTERFACE((obj), XFDASHBOARD_TYPE_WINDOW_TRACKER_WORKSPACE, XfdashboardWindowTrackerWorkspaceInterface))

typedef struct _XfdashboardWindowTrackerWorkspace				XfdashboardWindowTrackerWorkspace;
typedef struct _XfdashboardWindowTrackerWorkspaceInterface		XfdashboardWindowTrackerWorkspaceInterface;

struct _XfdashboardWindowTrackerWorkspaceInterface
{
	/*< private >*/
	/* Parent interface */
	GTypeInterface						parent_interface;

	/*< public >*/
	/* Virtual functions */
	gboolean (*is_equal)(XfdashboardWindowTrackerWorkspace *inLeft, XfdashboardWindowTrackerWorkspace *inRight);

	gint (*get_number)(XfdashboardWindowTrackerWorkspace *self);
	const gchar* (*get_name)(XfdashboardWindowTrackerWorkspace *self);

	void (*get_size)(XfdashboardWindowTrackerWorkspace *self, gint *outWidth, gint *outHeight);

	gboolean (*is_active)(XfdashboardWindowTrackerWorkspace *self);
	void (*activate)(XfdashboardWindowTrackerWorkspace *self);

	/* Signals */
	void (*name_changed)(XfdashboardWindowTrackerWorkspace *self);
};

/* Public API */
GType xfdashboard_window_tracker_workspace_get_type(void) G_GNUC_CONST;

gboolean xfdashboard_window_tracker_workspace_is_equal(XfdashboardWindowTrackerWorkspace *inLeft,
														XfdashboardWindowTrackerWorkspace *inRight);

gint xfdashboard_window_tracker_workspace_get_number(XfdashboardWindowTrackerWorkspace *self);
const gchar* xfdashboard_window_tracker_workspace_get_name(XfdashboardWindowTrackerWorkspace *self);

void xfdashboard_window_tracker_workspace_get_size(XfdashboardWindowTrackerWorkspace *self,
													gint *outWidth,
													gint *outHeight);

gboolean xfdashboard_window_tracker_workspace_is_active(XfdashboardWindowTrackerWorkspace *self);
void xfdashboard_window_tracker_workspace_activate(XfdashboardWindowTrackerWorkspace *self);

G_END_DECLS

#endif	/* __LIBXFDASHBOARD_WINDOW_TRACKER_WORKSPACE__ */
