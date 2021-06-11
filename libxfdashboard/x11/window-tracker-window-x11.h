/*
 * window-tracker-window: A window tracked by window tracker and also
 *                        a wrapper class around WnckWindow.
 *                        By wrapping libwnck objects we can use a virtual
 *                        stable API while the API in libwnck changes
 *                        within versions. We only need to use #ifdefs in
 *                        window tracker object and nowhere else in the code.
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

#ifndef __LIBXFDASHBOARD_WINDOW_TRACKER_WINDOW_X11__
#define __LIBXFDASHBOARD_WINDOW_TRACKER_WINDOW_X11__

#if !defined(__LIBXFDASHBOARD_H_INSIDE__) && !defined(LIBXFDASHBOARD_COMPILATION)
#error "Only <libxfdashboard/libxfdashboard.h> can be included directly."
#endif

#include <glib-object.h>
#include <clutter/x11/clutter-x11.h>

#define WNCK_I_KNOW_THIS_IS_UNSTABLE
#include <libwnck/libwnck.h>

G_BEGIN_DECLS

#define XFDASHBOARD_TYPE_WINDOW_TRACKER_WINDOW_X11				(xfdashboard_window_tracker_window_x11_get_type())
#define XFDASHBOARD_WINDOW_TRACKER_WINDOW_X11(obj)				(G_TYPE_CHECK_INSTANCE_CAST((obj), XFDASHBOARD_TYPE_WINDOW_TRACKER_WINDOW_X11, XfdashboardWindowTrackerWindowX11))
#define XFDASHBOARD_IS_WINDOW_TRACKER_WINDOW_X11(obj)			(G_TYPE_CHECK_INSTANCE_TYPE((obj), XFDASHBOARD_TYPE_WINDOW_TRACKER_WINDOW_X11))
#define XFDASHBOARD_WINDOW_TRACKER_WINDOW_X11_CLASS(klass)		(G_TYPE_CHECK_CLASS_CAST((klass), XFDASHBOARD_TYPE_WINDOW_TRACKER_WINDOW_X11, XfdashboardWindowTrackerWindowX11Class))
#define XFDASHBOARD_IS_WINDOW_TRACKER_WINDOW_X11_CLASS(klass)	(G_TYPE_CHECK_CLASS_TYPE((klass), XFDASHBOARD_TYPE_WINDOW_TRACKER_WINDOW_X11))
#define XFDASHBOARD_WINDOW_TRACKER_WINDOW_X11_GET_CLASS(obj)	(G_TYPE_INSTANCE_GET_CLASS((obj), XFDASHBOARD_TYPE_WINDOW_TRACKER_WINDOW_X11, XfdashboardWindowTrackerWindowX11Class))

typedef struct _XfdashboardWindowTrackerWindowX11				XfdashboardWindowTrackerWindowX11;
typedef struct _XfdashboardWindowTrackerWindowX11Class			XfdashboardWindowTrackerWindowX11Class;
typedef struct _XfdashboardWindowTrackerWindowX11Private		XfdashboardWindowTrackerWindowX11Private;

struct _XfdashboardWindowTrackerWindowX11
{
	/*< private >*/
	/* Parent instance */
	GObject										parent_instance;

	/* Private structure */
	XfdashboardWindowTrackerWindowX11Private	*priv;
};

struct _XfdashboardWindowTrackerWindowX11Class
{
	/*< private >*/
	/* Parent class */
	GObjectClass								parent_class;

	/*< public >*/
	/* Virtual functions */
};

/* Public API */
GType xfdashboard_window_tracker_window_x11_get_type(void) G_GNUC_CONST;

WnckWindow* xfdashboard_window_tracker_window_x11_get_window(XfdashboardWindowTrackerWindowX11 *self);
gulong xfdashboard_window_tracker_window_x11_get_xid(XfdashboardWindowTrackerWindowX11 *self);

G_END_DECLS

#endif	/* __LIBXFDASHBOARD_WINDOW_TRACKER_WINDOW_X11__ */
