/*
 * window-tracker: Tracks windows, workspaces, monitors and
 *                 listens for changes
 * 
 * Copyright 2012-2013 Stephan Haller <nomad@froevel.de>
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

#ifndef __XFDASHBOARD_WINDOW_TRACKER__
#define __XFDASHBOARD_WINDOW_TRACKER__

#define WNCK_I_KNOW_THIS_IS_UNSTABLE
#include <libwnck/libwnck.h>

G_BEGIN_DECLS

#define XFDASHBOARD_TYPE_WINDOW_TRACKER				(xfdashboard_window_tracker_get_type())
#define XFDASHBOARD_WINDOW_TRACKER(obj)				(G_TYPE_CHECK_INSTANCE_CAST((obj), XFDASHBOARD_TYPE_WINDOW_TRACKER, XfdashboardWindowTracker))
#define XFDASHBOARD_IS_WINDOW_TRACKER(obj)			(G_TYPE_CHECK_INSTANCE_TYPE((obj), XFDASHBOARD_TYPE_WINDOW_TRACKER))
#define XFDASHBOARD_WINDOW_TRACKER_CLASS(klass)		(G_TYPE_CHECK_CLASS_CAST((klass), XFDASHBOARD_TYPE_WINDOW_TRACKER, XfdashboardWindowTrackerClass))
#define XFDASHBOARD_IS_WINDOW_TRACKER_CLASS(klass)	(G_TYPE_CHECK_CLASS_TYPE((klass), XFDASHBOARD_TYPE_WINDOW_TRACKER))
#define XFDASHBOARD_WINDOW_TRACKER_GET_CLASS(obj)	G_TYPE_INSTANCE_GET_CLASS((obj), XFDASHBOARD_TYPE_WINDOW_TRACKER, XfdashboardWindowTrackerClass))

typedef struct _XfdashboardWindowTracker			XfdashboardWindowTracker;
typedef struct _XfdashboardWindowTrackerClass		XfdashboardWindowTrackerClass;
typedef struct _XfdashboardWindowTrackerPrivate		XfdashboardWindowTrackerPrivate;

struct _XfdashboardWindowTracker
{
	/* Parent instance */
	GObject								parent_instance;

	/* Private structure */
	XfdashboardWindowTrackerPrivate		*priv;
};

struct _XfdashboardWindowTrackerClass
{
	/*< private >*/
	/* Parent class */
	GObjectClass						parent_class;

	/*< public >*/
	/* Virtual functions */
	void (*active_window_changed)(XfdashboardWindowTracker *self, WnckWindow *inOldWindow, WnckWindow *inNewWindow);
	void (*window_opened)(XfdashboardWindowTracker *self, WnckWindow *inWindow);
	void (*window_closed)(XfdashboardWindowTracker *self, WnckWindow *inWindow);
	void (*window_geometry_changed)(XfdashboardWindowTracker *self, WnckWindow *inWindow);
	void (*window_actions_changed)(XfdashboardWindowTracker *self, WnckWindow *inWindow);
	void (*window_state_changed)(XfdashboardWindowTracker *self, WnckWindow *inWindow);
	void (*window_icon_changed)(XfdashboardWindowTracker *self, WnckWindow *inWindow);
	void (*window_name_changed)(XfdashboardWindowTracker *self, WnckWindow *inWindow);
	void (*window_workspace_changed)(XfdashboardWindowTracker *self, WnckWindow *inWindow, WnckWorkspace *inWorkspace);

	void (*active_workspace_changed)(XfdashboardWindowTracker *self, WnckWorkspace *inOldWorkspace, WnckWorkspace *inNewWorkspace);
	void (*workspace_added)(XfdashboardWindowTracker *self, WnckWorkspace *inWorkspace);
	void (*workspace_removed)(XfdashboardWindowTracker *self, WnckWorkspace *inWorkspace);
	void (*workspace_name_changed)(XfdashboardWindowTracker *self, WnckWorkspace *inWorkspace);
};

/* Public API */
GType xfdashboard_window_tracker_get_type(void) G_GNUC_CONST;

XfdashboardWindowTracker* xfdashboard_window_tracker_get_default(void);

WnckWindow* xfdashboard_window_tracker_get_active_window(XfdashboardWindowTracker *self);
WnckWorkspace* xfdashboard_window_tracker_get_active_workspace(XfdashboardWindowTracker *self);

G_END_DECLS

#endif	/* __XFDASHBOARD_WINDOW_TRACKER__ */
