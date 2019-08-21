/*
 * application-tracker: A singleton managing states of applications
 * 
 * Copyright 2012-2019 Stephan Haller <nomad@froevel.de>
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

#ifndef __LIBXFDASHBOARD_APPLICATION_TRACKER__
#define __LIBXFDASHBOARD_APPLICATION_TRACKER__

#if !defined(__LIBXFDASHBOARD_H_INSIDE__) && !defined(LIBXFDASHBOARD_COMPILATION)
#error "Only <libxfdashboard/libxfdashboard.h> can be included directly."
#endif

#include <glib-object.h>
#include <gio/gio.h>

G_BEGIN_DECLS

#define XFDASHBOARD_TYPE_APPLICATION_TRACKER				(xfdashboard_application_tracker_get_type())
#define XFDASHBOARD_APPLICATION_TRACKER(obj)				(G_TYPE_CHECK_INSTANCE_CAST((obj), XFDASHBOARD_TYPE_APPLICATION_TRACKER, XfdashboardApplicationTracker))
#define XFDASHBOARD_IS_APPLICATION_TRACKER(obj)				(G_TYPE_CHECK_INSTANCE_TYPE((obj), XFDASHBOARD_TYPE_APPLICATION_TRACKER))
#define XFDASHBOARD_APPLICATION_TRACKER_CLASS(klass)		(G_TYPE_CHECK_CLASS_CAST((klass), XFDASHBOARD_TYPE_APPLICATION_TRACKER, XfdashboardApplicationTrackerClass))
#define XFDASHBOARD_IS_APPLICATION_TRACKER_CLASS(klass)		(G_TYPE_CHECK_CLASS_TYPE((klass), XFDASHBOARD_TYPE_APPLICATION_TRACKER))
#define XFDASHBOARD_APPLICATION_TRACKER_GET_CLASS(obj)		(G_TYPE_INSTANCE_GET_CLASS((obj), XFDASHBOARD_TYPE_APPLICATION_TRACKER, XfdashboardApplicationTrackerClass))

typedef struct _XfdashboardApplicationTracker				XfdashboardApplicationTracker;
typedef struct _XfdashboardApplicationTrackerClass			XfdashboardApplicationTrackerClass;
typedef struct _XfdashboardApplicationTrackerPrivate		XfdashboardApplicationTrackerPrivate;

struct _XfdashboardApplicationTracker
{
	/*< private >*/
	/* Parent instance */
	GObject									parent_instance;

	/* Private structure */
	XfdashboardApplicationTrackerPrivate	*priv;
};

struct _XfdashboardApplicationTrackerClass
{
	/*< private >*/
	/* Parent class */
	GObjectClass							parent_class;

	/*< public >*/
	/* Virtual functions */
	void (*state_changed)(XfdashboardApplicationTracker *self, const gchar *inDesktopID, gboolean inRunning);
};

/* Public API */
GType xfdashboard_application_tracker_get_type(void) G_GNUC_CONST;

XfdashboardApplicationTracker* xfdashboard_application_tracker_get_default(void);

gboolean xfdashboard_application_tracker_is_running_by_desktop_id(XfdashboardApplicationTracker *self,
																	const gchar *inDesktopID);
gboolean xfdashboard_application_tracker_is_running_by_app_info(XfdashboardApplicationTracker *self,
																GAppInfo *inAppInfo);

const GList* xfdashboard_application_tracker_get_window_list_by_desktop_id(XfdashboardApplicationTracker *self,
																			const gchar *inDesktopID);
const GList*  xfdashboard_application_tracker_get_window_list_by_app_info(XfdashboardApplicationTracker *self,
																			GAppInfo *inAppInfo);

G_END_DECLS

#endif	/* __LIBXFDASHBOARD_APPLICATION_TRACKER__ */
