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

#ifndef __LIBXFDASHBOARD_WINDOW_TRACKER_BACKEND_X11__
#define __LIBXFDASHBOARD_WINDOW_TRACKER_BACKEND_X11__

#if !defined(__LIBXFDASHBOARD_H_INSIDE__) && !defined(LIBXFDASHBOARD_COMPILATION)
#error "Only <libxfdashboard/libxfdashboard.h> can be included directly."
#endif

#include <glib-object.h>

#include <libxfdashboard/window-tracker-backend.h>

G_BEGIN_DECLS

#define XFDASHBOARD_TYPE_WINDOW_TRACKER_BACKEND_X11				(xfdashboard_window_tracker_backend_x11_get_type())
#define XFDASHBOARD_WINDOW_TRACKER_BACKEND_X11(obj)				(G_TYPE_CHECK_INSTANCE_CAST((obj), XFDASHBOARD_TYPE_WINDOW_TRACKER_BACKEND_X11, XfdashboardWindowTrackerBackendX11))
#define XFDASHBOARD_IS_WINDOW_TRACKER_BACKEND_X11(obj)			(G_TYPE_CHECK_INSTANCE_TYPE((obj), XFDASHBOARD_TYPE_WINDOW_TRACKER_BACKEND_X11))
#define XFDASHBOARD_WINDOW_TRACKER_BACKEND_X11_CLASS(klass)		(G_TYPE_CHECK_CLASS_CAST((klass), XFDASHBOARD_TYPE_WINDOW_TRACKER_BACKEND_X11, XfdashboardWindowTrackerBackendX11Class))
#define XFDASHBOARD_IS_WINDOW_TRACKER_BACKEND_X11_CLASS(klass)	(G_TYPE_CHECK_CLASS_TYPE((klass), XFDASHBOARD_TYPE_WINDOW_TRACKER_BACKEND_X11))
#define XFDASHBOARD_WINDOW_TRACKER_BACKEND_X11_GET_CLASS(obj)	(G_TYPE_INSTANCE_GET_CLASS((obj), XFDASHBOARD_TYPE_WINDOW_TRACKER_BACKEND_X11, XfdashboardWindowTrackerBackendX11Class))

typedef struct _XfdashboardWindowTrackerBackendX11				XfdashboardWindowTrackerBackendX11;
typedef struct _XfdashboardWindowTrackerBackendX11Class			XfdashboardWindowTrackerBackendX11Class;
typedef struct _XfdashboardWindowTrackerBackendX11Private		XfdashboardWindowTrackerBackendX11Private;

struct _XfdashboardWindowTrackerBackendX11
{
	/*< private >*/
	/* Parent instance */
	GObject											parent_instance;

	/* Private structure */
	XfdashboardWindowTrackerBackendX11Private		*priv;
};

struct _XfdashboardWindowTrackerBackendX11Class
{
	/*< private >*/
	/* Parent class */
	GObjectClass									parent_class;

	/*< public >*/
	/* Virtual functions */
};

/* Public API */
GType xfdashboard_window_tracker_backend_x11_get_type(void) G_GNUC_CONST;

XfdashboardWindowTrackerBackend* xfdashboard_window_tracker_backend_x11_new(void);

G_END_DECLS

#endif	/* __LIBXFDASHBOARD_WINDOW_TRACKER_BACKEND_X11__ */
