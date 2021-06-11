/*
 * window-tracker-backend: Window tracker backend providing special functions
 *                         for different windowing and clutter backends.
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

#ifndef __LIBXFDASHBOARD_WINDOW_TRACKER_BACKEND__
#define __LIBXFDASHBOARD_WINDOW_TRACKER_BACKEND__

#if !defined(__LIBXFDASHBOARD_H_INSIDE__) && !defined(LIBXFDASHBOARD_COMPILATION)
#error "Only <libxfdashboard/libxfdashboard.h> can be included directly."
#endif

#include <glib-object.h>

#include <libxfdashboard/window-tracker.h>

G_BEGIN_DECLS

/* Object declaration */
#define XFDASHBOARD_TYPE_WINDOW_TRACKER_BACKEND				(xfdashboard_window_tracker_backend_get_type())
#define XFDASHBOARD_WINDOW_TRACKER_BACKEND(obj)				(G_TYPE_CHECK_INSTANCE_CAST((obj), XFDASHBOARD_TYPE_WINDOW_TRACKER_BACKEND, XfdashboardWindowTrackerBackend))
#define XFDASHBOARD_IS_WINDOW_TRACKER_BACKEND(obj)			(G_TYPE_CHECK_INSTANCE_TYPE((obj), XFDASHBOARD_TYPE_WINDOW_TRACKER_BACKEND))
#define XFDASHBOARD_WINDOW_TRACKER_BACKEND_GET_IFACE(obj)	(G_TYPE_INSTANCE_GET_INTERFACE((obj), XFDASHBOARD_TYPE_WINDOW_TRACKER_BACKEND, XfdashboardWindowTrackerBackendInterface))

typedef struct _XfdashboardWindowTrackerBackend				XfdashboardWindowTrackerBackend;
typedef struct _XfdashboardWindowTrackerBackendInterface	XfdashboardWindowTrackerBackendInterface;

/**
 * XfdashboardWindowTrackerBackendInterface:
 * @get_name: Name of window tracker backend
 * @get_window_tracker: Get window tracker instance used by backend
 * @get_stage_from_window: Get stage using requested window
 * @show_stage_window: Setup and show stage window
 * @hide_stage_window: Hide (and maybe deconfigure) stage window
 */
struct _XfdashboardWindowTrackerBackendInterface
{
	/*< private >*/
	/* Parent interface */
	GTypeInterface						parent_interface;

	/*< public >*/
	/* Virtual functions */
	const gchar* (*get_name)(XfdashboardWindowTrackerBackend *self);

	XfdashboardWindowTracker* (*get_window_tracker)(XfdashboardWindowTrackerBackend *self);

	XfdashboardWindowTrackerWindow* (*get_window_for_stage)(XfdashboardWindowTrackerBackend *self,
															ClutterStage *inStage);
	ClutterStage* (*get_stage_from_window)(XfdashboardWindowTrackerBackend *self,
											XfdashboardWindowTrackerWindow *inWindow);
	void (*show_stage_window)(XfdashboardWindowTrackerBackend *self,
								XfdashboardWindowTrackerWindow *inWindow);
	void (*hide_stage_window)(XfdashboardWindowTrackerBackend *self,
								XfdashboardWindowTrackerWindow *inWindow);
};


/* Public API */
GType xfdashboard_window_tracker_backend_get_type(void) G_GNUC_CONST;

void xfdashboard_window_tracker_backend_set_backend(const gchar *inBackend);

XfdashboardWindowTrackerBackend* xfdashboard_window_tracker_backend_create(void);

const gchar* xfdashboard_window_tracker_backend_get_name(XfdashboardWindowTrackerBackend *self);

XfdashboardWindowTracker* xfdashboard_window_tracker_backend_get_window_tracker(XfdashboardWindowTrackerBackend *self);

XfdashboardWindowTrackerWindow* xfdashboard_window_tracker_backend_get_window_for_stage(XfdashboardWindowTrackerBackend *self,
																						ClutterStage *inStage);
ClutterStage* xfdashboard_window_tracker_backend_get_stage_from_window(XfdashboardWindowTrackerBackend *self,
																		XfdashboardWindowTrackerWindow *inWindow);
void xfdashboard_window_tracker_backend_show_stage_window(XfdashboardWindowTrackerBackend *self,
															XfdashboardWindowTrackerWindow *inWindow);
void xfdashboard_window_tracker_backend_hide_stage_window(XfdashboardWindowTrackerBackend *self,
															XfdashboardWindowTrackerWindow *inWindow);

G_END_DECLS

#endif	/* __LIBXFDASHBOARD_WINDOW_TRACKER_BACKEND__ */
