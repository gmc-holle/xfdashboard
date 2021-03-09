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
#ifdef HAVE_BACKEND_GDK
#include <libxfdashboard/gdk/window-tracker-backend-gdk.h>
#endif
#include <libxfdashboard/core.h>
#include <libxfdashboard/marshal.h>
#include <libxfdashboard/compat.h>
#include <libxfdashboard/debug.h>


/* Define this class in GObject system */
G_DEFINE_INTERFACE(XfdashboardWindowTrackerBackend,
					xfdashboard_window_tracker_backend,
					G_TYPE_OBJECT)


/* IMPLEMENTATION: Private variables and methods */
#define XFDASHBOARD_WINDOWS_TRACKER_BACKEND_WARN_NOT_IMPLEMENTED(self, vfunc)  \
	g_warning("Object of type %s does not implement required virtual function XfdashboardWindowTrackerBackend::%s",\
				G_OBJECT_TYPE_NAME(self), \
				vfunc);

struct _XfdashboardWindowTrackerBackendMap
{
	const gchar							*backendID;
	const gchar							*clutterBackendID;
	XfdashboardWindowTrackerBackend*	(*createBackend)(void);
};
typedef struct _XfdashboardWindowTrackerBackendMap	XfdashboardWindowTrackerBackendMap;

static XfdashboardWindowTrackerBackendMap	_xfdashboard_window_tracker_backend_map[]=
											{
#ifdef CLUTTER_WINDOWING_X11
												{ "x11", CLUTTER_WINDOWING_X11, xfdashboard_window_tracker_backend_x11_new },
#endif
#ifdef HAVE_BACKEND_GDK
#ifdef   CLUTTER_WINDOWING_GDK
												{ "gdk", CLUTTER_WINDOWING_GDK, xfdashboard_window_tracker_backend_gdk_new },
#endif   /* CLUTTER_WINDOWING_GDK */
#endif /* HAVE_BACKEND_GDK */
												{ NULL, NULL, NULL }
											};


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
 * xfdashboard_window_tracker_backend_set_backend:
 * @inBackend: the backend to use
 *
 * Sets the backend that xfdashboard should try to use. It will also restrict
 * the backend Clutter should try to use. By default xfdashboard will select
 * the backend automatically based on the backend Clutter uses.
 *
 * For example:
 *
 * |[<!-- language="C" -->
 *   xfdashboard_window_tracker_backend_set_backend("x11");
 * ]|
 *
 * Will make xfdashboard and Clutter use the X11 backend.
 *
 * Possible backends are: x11 and gdk.
 *
 * This function must be called before the first API call to xfdashboard or any
 * library xfdashboard depends on like Clutter, GTK+ etc. This function can also
 * be called only once.
 */
void xfdashboard_window_tracker_backend_set_backend(const gchar *inBackend)
{
#if CLUTTER_CHECK_VERSION(1, 16, 0)
	XfdashboardWindowTrackerBackendMap	*iter;
	static gboolean						wasSet=FALSE;

	g_return_if_fail(inBackend && *inBackend);

	/* Warn if this function was called more than once */
	if(wasSet)
	{
		g_critical("Cannot set backend to '%s' because it the backend was already set",
					inBackend);
		return;
	}

	/* Set flag that this function was called regardless of the result of this
	 * function call.
	 */
	wasSet=TRUE;

	/* Backend can only be set if application was not already created */
	if(xfdashboard_core_has_default())
	{
		g_critical("Cannot set backend to '%s' because application is already initialized",
					inBackend);
		return;
	}

	/* Iterate through list of available backends and lookup the requested
	 * backend. If this entry is found, restrict Clutter backend as listed in
	 * found entry and return.
	 */
	for(iter=_xfdashboard_window_tracker_backend_map; iter->backendID; iter++)
	{
		/* If this entry does not match requested backend, try next one */
		if(g_strcmp0(iter->backendID, inBackend)!=0) continue;

		/* The entry matches so restrict allowed backends in Clutter to the one
		 * listed at this entry.
		 */
		clutter_set_windowing_backend(iter->clutterBackendID);

		return;
	}

	/* If we get here the requested backend is unknown */
	g_warning("Unknown backend '%s' - using default backend", inBackend);
#endif
}

/**
 * xfdashboard_window_tracker_backend_create:
 *
 * Creates an instance of #XfdashboardWindowTrackerBackend matching the clutter
 * windowing backend used. If no matching window tracker backend can be found
 * or created, %NULL will returned.
 *
 * Return value: (transfer full): The instance of #XfdashboardWindowTrackerBackend
 *   or %NULL if no instance could be created
 */
XfdashboardWindowTrackerBackend* xfdashboard_window_tracker_backend_create(void)
{
	XfdashboardWindowTrackerBackend		*backend;
	XfdashboardWindowTrackerBackendMap	*iter;

	/* Iterate through list of available backends and check if any entry
	 * matches the backend Clutter is using. If we can find a matching entry
	 * then create our backend which interacts with Clutter's backend.
	 */
	backend=NULL;
	for(iter=_xfdashboard_window_tracker_backend_map; !backend && iter->backendID; iter++)
	{
		/* If this entry does not match any backend at Clutter, try next one */
		if(!clutter_check_windowing_backend(iter->clutterBackendID)) continue;

		/* The entry matches so try to create our backend */
		XFDASHBOARD_DEBUG(NULL, WINDOWS,
							"Found window tracker backend ID '%s' for clutter backend '%s'",
							iter->backendID,
							iter->clutterBackendID);

		backend=(iter->createBackend)();
		if(!backend)
		{
			XFDASHBOARD_DEBUG(NULL, WINDOWS,
								"Could not create window tracker backend of ID '%s' for clutter backend '%s'",
								iter->backendID,
								iter->clutterBackendID);
		}
			else
			{
				XFDASHBOARD_DEBUG(backend, WINDOWS,
									"Create window tracker backend of type %s with ID '%s' for clutter backend '%s'",
									G_OBJECT_TYPE_NAME(backend),
									iter->backendID,
									iter->clutterBackendID);
			}
	}

	/* Check if we could create any backend */
	if(!backend)
	{
		g_critical("Cannot find any usable window tracker backend");
		return(NULL);
	}

	/* Return created backend */
	return(backend);
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
 * xfdashboard_window_tracker_backend_get_window_for_stage:
 * @self: A #XfdashboardWindowTrackerBackend
 * @inStage: A #ClutterStage
 *
 * Retrieves the window created for the requested stage @inStage from window
 * tracker backend @self.
 *
 * Return value: (transfer none): The #XfdashboardWindowTrackerWindow representing
 *   the window of requested stage or %NULL if not available. The returned object
 *   is owned by Xfdashboard and it should not be referenced or unreferenced.
 */
XfdashboardWindowTrackerWindow* xfdashboard_window_tracker_backend_get_window_for_stage(XfdashboardWindowTrackerBackend *self,
																						ClutterStage *inStage)
{
	XfdashboardWindowTrackerBackendInterface		*iface;

	g_return_val_if_fail(XFDASHBOARD_IS_WINDOW_TRACKER_BACKEND(self), NULL);

	iface=XFDASHBOARD_WINDOW_TRACKER_BACKEND_GET_IFACE(self);

	/* Call virtual function */
	if(iface->get_window_for_stage)
	{
		return(iface->get_window_for_stage(self, inStage));
	}

	/* If we get here the virtual function was not overridden */
	XFDASHBOARD_WINDOWS_TRACKER_BACKEND_WARN_NOT_IMPLEMENTED(self, "get_window_for_stage");
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
