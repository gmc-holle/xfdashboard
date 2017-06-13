/*
 * window-tracker-backend: Window tracker backend providing special functions
 *                         for different windowing and clutter backends.
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

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <libxfdashboard/window-tracker-backend.h>

#include <glib/gi18n-lib.h>

#include <libxfdashboard/x11/window-tracker-backend-x11.h>
#include <libxfdashboard/gdk/window-tracker-backend-gdk.h>
#include <libxfdashboard/marshal.h>
#include <libxfdashboard/compat.h>
#include <libxfdashboard/debug.h>


/* Define this class in GObject system */
G_DEFINE_INTERFACE(XfdashboardWindowTrackerBackend,
					xfdashboard_window_tracker_backend,
					G_TYPE_OBJECT)


/* IMPLEMENTATION: Private variables and methods */
#define XFDASHBOARD_WINDOWS_TRACKER_BACKEND_WARN_NOT_IMPLEMENTED(self, vfunc)  \
	g_warning(_("Object of type %s does not implement required virtual function XfdashboardWindowTrackerBackend::%s"),\
				G_OBJECT_TYPE_NAME(self), \
				vfunc);

static XfdashboardWindowTrackerBackend *_xfdashboard_window_tracker_backend_singleton=NULL;


/* IMPLEMENTATION: GObject */

/* Interface initialization
 * Set up default functions
 */
void xfdashboard_window_tracker_backend_default_init(XfdashboardWindowTrackerBackendInterface *iface)
{
	static gboolean		initialized=FALSE;

	/* Define properties, signals and actions */
	if(!initialized)
	{
		/* Set flag that base initialization was done for this interface */
		initialized=TRUE;
	}
}


/* IMPLEMENTATION: Public API */

/**
 * xfdashboard_window_tracker_backend_get_default:
 *
 * Retrieves the singleton instance of #XfdashboardWindowTrackerBackend. If not
 * needed anymore the caller must unreference the returned object instance.
 *
 * Return value: (transfer full): The instance of #XfdashboardWindowTrackerBackend.
 */
XfdashboardWindowTrackerBackend* xfdashboard_window_tracker_backend_get_default(void)
{
	if(G_UNLIKELY(_xfdashboard_window_tracker_backend_singleton==NULL))
	{
		GType			windowTrackerBackendType=G_TYPE_INVALID;
		const gchar		*windowTrackerBackend;

		/* Check if a specific backend was requested */
		windowTrackerBackend=g_getenv("XFDASHBOARD_BACKEND");

		if(g_strcmp0(windowTrackerBackend, "gdk")==0)
		{
			windowTrackerBackendType=XFDASHBOARD_TYPE_WINDOW_TRACKER_BACKEND_GDK;
		}

		/* If no specific backend was requested use default one */
		if(windowTrackerBackendType==G_TYPE_INVALID)
		{
			windowTrackerBackendType=XFDASHBOARD_TYPE_WINDOW_TRACKER_BACKEND_X11;
			XFDASHBOARD_DEBUG(NULL, WINDOWS,
								"Using default backend %s",
								g_type_name(windowTrackerBackendType));
		}

		/* Create singleton */
		_xfdashboard_window_tracker_backend_singleton=XFDASHBOARD_WINDOW_TRACKER_BACKEND(g_object_new(windowTrackerBackendType, NULL));
		XFDASHBOARD_DEBUG(_xfdashboard_window_tracker_backend_singleton, WINDOWS,
							"Created window tracker of type %s for %s backend",
							_xfdashboard_window_tracker_backend_singleton ? G_OBJECT_TYPE_NAME(_xfdashboard_window_tracker_backend_singleton) : "<<unknown>>",
							windowTrackerBackend ? windowTrackerBackend : "default");
	}
		else g_object_ref(_xfdashboard_window_tracker_backend_singleton);

	return(_xfdashboard_window_tracker_backend_singleton);
}

/**
 * xfdashboard_window_tracker_backend_get_name:
 * @self: A #XfdashboardWindowTrackerBackend
 *
 * Retrieves the name of #XfdashboardWindowTrackerBackend at @self.
 *
 * Return value: String containing the name of the window tracker backend.
 */
const gchar* xfdashboard_window_tracker_backend_get_name(XfdashboardWindowTrackerBackend *self)
{
	XfdashboardWindowTrackerBackendInterface		*iface;

	g_return_val_if_fail(XFDASHBOARD_IS_WINDOW_TRACKER_BACKEND(self), NULL);

	iface=XFDASHBOARD_WINDOW_TRACKER_BACKEND_GET_IFACE(self);

	/* Call virtual function */
	if(iface->get_name)
	{
		return(iface->get_name(self));
	}

	/* If we get here the virtual function was not overridden */
	XFDASHBOARD_WINDOWS_TRACKER_BACKEND_WARN_NOT_IMPLEMENTED(self, "get_name");
	return(NULL);
}

/**
 * xfdashboard_window_tracker_backend_get_window_tracker:
 * @self: A #XfdashboardWindowTrackerBackend
 *
 * Retrieves the #XfdashboardWindowTracker used by backend @self. If not needed
 * anymore the caller must unreference the returned object instance.
 *
 * Return value: (transfer full): The instance of #XfdashboardWindowTracker.
 */
XfdashboardWindowTracker* xfdashboard_window_tracker_backend_get_window_tracker(XfdashboardWindowTrackerBackend *self)
{
	XfdashboardWindowTrackerBackendInterface		*iface;

	g_return_val_if_fail(XFDASHBOARD_IS_WINDOW_TRACKER_BACKEND(self), NULL);

	iface=XFDASHBOARD_WINDOW_TRACKER_BACKEND_GET_IFACE(self);

	/* Call virtual function */
	if(iface->get_window_tracker)
	{
		XfdashboardWindowTracker					*windowTracker;

		windowTracker=iface->get_window_tracker(self);
		if(windowTracker) g_object_ref(windowTracker);

		return(windowTracker);
	}

	/* If we get here the virtual function was not overridden */
	XFDASHBOARD_WINDOWS_TRACKER_BACKEND_WARN_NOT_IMPLEMENTED(self, "get_window_tracker");
	return(NULL);
}

/**
 * xfdashboard_window_tracker_backend_get_stage_from_window:
 * @self: A #XfdashboardWindowTrackerBackend
 * @inWindow: A #XfdashboardWindowTrackerWindow defining the stage window
 *
 * Asks the window tracker backend @self to find the #ClutterStage which uses
 * stage window @inWindow.
 *
 * Return value: (transfer none): The #ClutterStage for stage window @inWindow or
 *   %NULL if @inWindow is not a stage window or stage could not be found.
 */
ClutterStage* xfdashboard_window_tracker_backend_get_stage_from_window(XfdashboardWindowTrackerBackend *self,
																		XfdashboardWindowTrackerWindow *inWindow)
{
	XfdashboardWindowTrackerBackendInterface		*iface;

	g_return_val_if_fail(XFDASHBOARD_IS_WINDOW_TRACKER_BACKEND(self), NULL);
	g_return_val_if_fail(XFDASHBOARD_IS_WINDOW_TRACKER_WINDOW(inWindow), NULL);

	iface=XFDASHBOARD_WINDOW_TRACKER_BACKEND_GET_IFACE(self);

	/* Call virtual function */
	if(iface->get_stage_from_window)
	{
		return(iface->get_stage_from_window(self, inWindow));
	}

	/* If we get here the virtual function was not overridden */
	XFDASHBOARD_WINDOWS_TRACKER_BACKEND_WARN_NOT_IMPLEMENTED(self, "get_stage_from_window");
	return(NULL);
}

/**
 * xfdashboard_window_tracker_backend_show_stage_window:
 * @self: A #XfdashboardWindowTrackerBackend
 * @inWindow: A #XfdashboardWindowTrackerWindow defining the stage window
 *
 * Asks the window tracker backend @self to set up and show the window @inWindow
 * for use as stage window.
 */
void xfdashboard_window_tracker_backend_show_stage_window(XfdashboardWindowTrackerBackend *self,
															XfdashboardWindowTrackerWindow *inWindow)
{
	XfdashboardWindowTrackerBackendInterface		*iface;

	g_return_if_fail(XFDASHBOARD_IS_WINDOW_TRACKER_BACKEND(self));
	g_return_if_fail(XFDASHBOARD_IS_WINDOW_TRACKER_WINDOW(inWindow));

	iface=XFDASHBOARD_WINDOW_TRACKER_BACKEND_GET_IFACE(self);

	/* Call virtual function */
	if(iface->show_stage_window)
	{
		iface->show_stage_window(self, inWindow);
		return;
	}

	/* If we get here the virtual function was not overridden */
	XFDASHBOARD_WINDOWS_TRACKER_BACKEND_WARN_NOT_IMPLEMENTED(self, "show_stage_window");
}

/**
 * xfdashboard_window_tracker_backend_hide_stage_window:
 * @self: A #XfdashboardWindowTrackerBackend
 * @inWindow: A #XfdashboardWindowTrackerWindow defining the stage window
 *
 * Asks the window tracker backend @self to hide the stage window @inWindow.
 */
void xfdashboard_window_tracker_backend_hide_stage_window(XfdashboardWindowTrackerBackend *self,
															XfdashboardWindowTrackerWindow *inWindow)
{
	XfdashboardWindowTrackerBackendInterface		*iface;

	g_return_if_fail(XFDASHBOARD_IS_WINDOW_TRACKER_BACKEND(self));
	g_return_if_fail(XFDASHBOARD_IS_WINDOW_TRACKER_WINDOW(inWindow));

	iface=XFDASHBOARD_WINDOW_TRACKER_BACKEND_GET_IFACE(self);

	/* Call virtual function */
	if(iface->hide_stage_window)
	{
		iface->hide_stage_window(self, inWindow);
		return;
	}

	/* If we get here the virtual function was not overridden */
	XFDASHBOARD_WINDOWS_TRACKER_BACKEND_WARN_NOT_IMPLEMENTED(self, "hide_stage_window");
}
