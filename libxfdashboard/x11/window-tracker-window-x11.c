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

/**
 * SECTION:window-tracker-window-x11
 * @short_description: A window used by X11 window tracker
 * @include: xfdashboard/x11/window-tracker-window-x11.h
 *
 * This is the X11 backend of #XfdashboardWindowTrackerWindow
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <libxfdashboard/x11/window-tracker-window-x11.h>

#define WNCK_I_KNOW_THIS_IS_UNSTABLE
#include <libwnck/libwnck.h>

#include <glib/gi18n-lib.h>
#include <clutter/x11/clutter-x11.h>
#include <gtk/gtkx.h>
#include <gdk/gdkx.h>
#ifdef HAVE_XINERAMA
#include <X11/extensions/Xinerama.h>
#endif

#include <libxfdashboard/x11/window-content-x11.h>
#include <libxfdashboard/x11/window-tracker-workspace-x11.h>
#include <libxfdashboard/x11/window-tracker-x11.h>
#include <libxfdashboard/window-tracker.h>
#include <libxfdashboard/core.h>
#include <libxfdashboard/desktop-app-info.h>
#include <libxfdashboard/marshal.h>
#include <libxfdashboard/compat.h>
#include <libxfdashboard/debug.h>


/* Define this class in GObject system */
static void _xfdashboard_window_tracker_window_x11_window_tracker_window_iface_init(XfdashboardWindowTrackerWindowInterface *iface);

struct _XfdashboardWindowTrackerWindowX11Private
{
	/* Properties related */
	WnckWindow								*window;
	XfdashboardWindowTrackerWindowState		state;
	XfdashboardWindowTrackerWindowAction	actions;

	/* Instance related */
	WnckWorkspace							*workspace;

	gint									lastGeometryX;
	gint									lastGeometryY;
	gint									lastGeometryWidth;
	gint									lastGeometryHeight;

	ClutterContent							*content;
};

G_DEFINE_TYPE_WITH_CODE(XfdashboardWindowTrackerWindowX11,
						xfdashboard_window_tracker_window_x11,
						G_TYPE_OBJECT,
						G_ADD_PRIVATE(XfdashboardWindowTrackerWindowX11)
						G_IMPLEMENT_INTERFACE(XFDASHBOARD_TYPE_WINDOW_TRACKER_WINDOW, _xfdashboard_window_tracker_window_x11_window_tracker_window_iface_init))

/* Properties */
enum
{
	PROP_0,

	PROP_WINDOW,

	/* Overriden properties of interface: XfdashboardWindowTrackerWindow */
	PROP_STATE,
	PROP_ACTIONS,

	PROP_LAST
};

static GParamSpec* XfdashboardWindowTrackerWindowX11Properties[PROP_LAST]={ 0, };


/* IMPLEMENTATION: Private variables and methods */
#define XFDASHBOARD_WINDOW_TRACKER_WINDOW_X11_WARN_NO_WINDOW(self)             \
	g_critical("No wnck window wrapped at %s in called function %s",           \
				G_OBJECT_TYPE_NAME(self),                                      \
				__func__);

#define XFDASHBOARD_WINDOW_TRACKER_WINDOW_X11_WARN_WRONG_WINDOW(self)          \
	g_critical("Got signal from wrong wnck window wrapped at %s in called function %s",\
				G_OBJECT_TYPE_NAME(self),                                      \
				__func__);

/* Try to resolve name to AppInfo */
static GAppInfo*  _xfdashboard_window_tracker_window_x11_window_tracker_window_resolve_name_to_appinfo(XfdashboardWindowTrackerWindowX11 *self,
																										const gchar *inName)
{
	XfdashboardWindowTrackerWindowX11Private	*priv;
	GAppInfo									*appInfo;
	gchar										*iterName;
	gchar										*iterNameLowerCase;
	XfdashboardApplicationDatabase				*appDatabase;
	GList										*apps;

	g_return_val_if_fail(XFDASHBOARD_IS_WINDOW_TRACKER_WINDOW_X11(self), NULL);
	g_return_val_if_fail(inName && *inName, NULL);
	g_return_val_if_fail(self->priv->window, NULL);

	priv=self->priv;
	appInfo=NULL;

	/* Get list of applications */
	appDatabase=xfdashboard_core_get_application_database(NULL);
	apps=xfdashboard_application_database_get_all_applications(appDatabase);

	/* Build desktop ID from name */
	if(!g_str_has_suffix(inName, ".desktop")) iterName=g_strconcat(inName, ".desktop", NULL);
		else iterName=g_strdup(inName);

	iterNameLowerCase=g_utf8_strdown(iterName, -1);

	/* Lookup application from unmodified name */
	appInfo=xfdashboard_application_database_lookup_desktop_id(appDatabase, iterName);

	/* Lookup application from to-lower-case converted name if previous
	 * lookup with unmodified name failed.
	 */
	if(!appInfo)
	{
		/* Lookup application from lower-case name */
		appInfo=xfdashboard_application_database_lookup_desktop_id(appDatabase, iterNameLowerCase);
	}

	/* If no application was found for the name it may be an application
	 * located in a subdirectory. Then the desktop ID is prefixed with
	 * the subdirectory's name followed by a dash. So iterate through all
	 * applications and lookup with glob pattern '*-' followed by name
	 * and suffix '.desktop'.
	 */
	if(!appInfo)
	{
		GList									*iterApps;
		GList									*foundSubdirApps;
		gchar									*globName;
		GPatternSpec							*globPattern;
		GAppInfo								*globAppInfo;

		/* Build glob pattern */
		globAppInfo=NULL;
		globName=g_strconcat("*-", iterNameLowerCase, NULL);
		globPattern=g_pattern_spec_new(globName);

		/* Iterate through application and collect applications matching
		 * glob pattern.
		 */
		foundSubdirApps=NULL;
		for(iterApps=apps; iterApps; iterApps=g_list_next(iterApps))
		{
			if(!G_IS_APP_INFO(iterApps->data)) continue;
			globAppInfo=G_APP_INFO(iterApps->data);

#if GLIB_CHECK_VERSION(2,70,0)
			if(g_pattern_spec_match_string(globPattern, g_app_info_get_id(globAppInfo)))
#else
			if(g_pattern_match_string(globPattern, g_app_info_get_id(globAppInfo)))
#endif
			{
				foundSubdirApps=g_list_prepend(foundSubdirApps, globAppInfo);
				XFDASHBOARD_DEBUG(self, APPLICATIONS,
									"Found possible application '%s' for window '%s' using pattern '%s'",
									g_app_info_get_id(globAppInfo),
									wnck_window_get_name(priv->window),
									globName);
			}
		}

		/* If exactly one application was collected because it matched
		 * the glob pattern then we found the application.
		 */
		if(g_list_length(foundSubdirApps)==1)
		{
			appInfo=G_APP_INFO(g_object_ref(G_OBJECT(foundSubdirApps->data)));

			XFDASHBOARD_DEBUG(self, APPLICATIONS,
								"Found exactly one application named '%s' for window '%s' using pattern '%s'",
								g_app_info_get_id(appInfo),
								wnck_window_get_name(priv->window),
								globName);
		}

		/* Release allocated resources */
		if(foundSubdirApps) g_list_free(foundSubdirApps);
		if(globPattern) g_pattern_spec_free(globPattern);
		if(globName) g_free(globName);
	}

	/* Release allocated resources */
	if(iterName) g_free(iterName);
	if(iterNameLowerCase) g_free(iterNameLowerCase);
	if(apps) g_list_free_full(apps, g_object_unref);
	if(appDatabase) g_object_unref(appDatabase);

	/* Return found AppInfo */
	return(appInfo);
}

/* Try to resolve start-up WM class to AppInfo */
static GAppInfo* _xfdashboard_window_tracker_window_x11_window_tracker_window_resolve_startupwm_to_appinfo(XfdashboardWindowTrackerWindowX11 *self)
{
	XfdashboardWindowTrackerWindowX11Private	*priv;
	GAppInfo									*appInfo;
	XfdashboardApplicationDatabase				*appDatabase;
	GList										*apps;
	GList										*iter;

	g_return_val_if_fail(XFDASHBOARD_IS_WINDOW_TRACKER_WINDOW_X11(self), NULL);
	g_return_val_if_fail(self->priv->window, NULL);

	priv=self->priv;
	appInfo=NULL;

	/* Get list of applications */
	appDatabase=xfdashboard_core_get_application_database(NULL);
	apps=xfdashboard_application_database_get_all_applications(appDatabase);

	/* Iterate through application and lookup start-up WM class */
	for(iter=apps; iter && !appInfo; iter=g_list_next(iter))
	{
		XfdashboardDesktopAppInfo				*iterAppInfo;
		gchar									*appInfoStartupWM;
		const gchar								*value;

		/* Skip AppInfo which are not of type XfdashboardDesktopAppInfo */
		if(!XFDASHBOARD_IS_DESKTOP_APP_INFO(iter->data)) continue;

		iterAppInfo=XFDASHBOARD_DESKTOP_APP_INFO(iter->data);

		/* If AppInfo has no entry for start-up WM class then skip it */
		if(!xfdashboard_desktop_app_info_has_key(iterAppInfo, G_KEY_FILE_DESKTOP_KEY_STARTUP_WM_CLASS)) continue;

		/* Check if start-up WM class of AppInfo matches */
		appInfoStartupWM=xfdashboard_desktop_app_info_get_string(iterAppInfo, G_KEY_FILE_DESKTOP_KEY_STARTUP_WM_CLASS);

		value=wnck_window_get_class_group_name(priv->window);
		if(!appInfo &&
			value &&
			g_strcmp0(appInfoStartupWM, value)==0)
		{
			appInfo=G_APP_INFO(g_object_ref(iterAppInfo));
		}

		value=wnck_window_get_class_instance_name(priv->window);
		if(!appInfo &&
			value &&
			g_strcmp0(appInfoStartupWM, value)==0)
		{
			appInfo=G_APP_INFO(g_object_ref(iterAppInfo));
		}

		/* Release allocated resources */
		if(appInfoStartupWM) g_free(appInfoStartupWM);
	}

	/* Release allocated resources */
	if(apps) g_list_free_full(apps, g_object_unref);
	if(appDatabase) g_object_unref(appDatabase);

	/* Return found AppInfo */
	return(appInfo);
}

/* Try to resolve window's executable to AppInfo */
static GAppInfo* _xfdashboard_window_tracker_window_x11_window_tracker_window_resolve_binary_executable_to_appinfo(XfdashboardWindowTrackerWindowX11 *self)
{
	XfdashboardWindowTrackerWindowX11Private	*priv;
	GAppInfo									*appInfo;
	XfdashboardApplicationDatabase				*appDatabase;
	GList										*apps;
	GList										*iter;
	gchar										*windowExecutable;
#if defined(__linux__)
	int											windowPID;
#endif

	g_return_val_if_fail(XFDASHBOARD_IS_WINDOW_TRACKER_WINDOW_X11(self), NULL);
	g_return_val_if_fail(self->priv->window, NULL);

	priv=self->priv;
	appInfo=NULL;
	windowExecutable=NULL;

	/* Get executable of process runnning this window */
#if defined(__linux__)
	windowPID=wnck_window_get_pid(priv->window);
	if(windowPID)
	{
		gchar									*procExecFile;
		GFile									*file;
		GFileInfo								*fileInfo;
		GError									*error;

		error=NULL;

		/* Get path to executable symlinked in proc filesystem */
		procExecFile=g_strdup_printf("/proc/%d/exe", windowPID);

		/* Read target of symlink of executable in proc filesystem.
		 * That is the window's executable.
		 */
		file=g_file_new_for_path(procExecFile);
		fileInfo=g_file_query_info(file,
									G_FILE_ATTRIBUTE_STANDARD_NAME "," G_FILE_ATTRIBUTE_STANDARD_SYMLINK_TARGET,
									G_FILE_QUERY_INFO_NOFOLLOW_SYMLINKS,
									NULL,
									&error);
		if(!fileInfo)
		{
			/* Show warning */
			g_warning("Could not determine executable of window '%s': %s",
						wnck_window_get_name(priv->window),
						error ? error->message : "unknown  error");
		}
			else
			{
				windowExecutable=g_strdup(g_file_info_get_symlink_target(fileInfo));
			}

		/* Release allocated resources */
		if(error) g_error_free(error);
		if(fileInfo) g_object_unref(fileInfo);
		if(file) g_object_unref(file);
		if(procExecFile) g_free(procExecFile);
	}
#else
	/* Unsupported OS */
#endif

	/* If we could not get window's executable then we do not need to compare it
	 * againt the executables of all applications.
	 */
	if(!windowExecutable) return(NULL);

	/* Get list of applications */
	appDatabase=xfdashboard_core_get_application_database(NULL);
	apps=xfdashboard_application_database_get_all_applications(appDatabase);

	/* Iterate through application and lookup start-up WM class */
	for(iter=apps; iter && !appInfo; iter=g_list_next(iter))
	{
		XfdashboardDesktopAppInfo				*iterAppInfo;
		const gchar								*iterExecutable;
		gboolean								executableMatches;

		/* Skip AppInfo which are not of type XfdashboardDesktopAppInfo */
		if(!XFDASHBOARD_IS_DESKTOP_APP_INFO(iter->data)) continue;

		iterAppInfo=XFDASHBOARD_DESKTOP_APP_INFO(iter->data);

		/* If AppInfo has no entry for binary executable then skip it.
		 * But this should never happen.
		 */
		iterExecutable=g_app_info_get_executable(G_APP_INFO(iterAppInfo));
		if(G_UNLIKELY(!iterExecutable)) continue;

		/* Check if executable of AppInfo matches the executable of process
		 * running this window. If either AppInfo's executable or window's
		 * executable is a relative path then check only their basenames as
		 * we cannot determine if the process, which spawned the window's
		 * executable, had the correct PATH enviroment set up to spawn the
		 * process. So we have to assume it. If both executables are available
		 * as absolute path then compre them entirely.
		 */
		if(!g_path_is_absolute(iterExecutable) ||
			!g_path_is_absolute(windowExecutable))
		{
			gchar								*iterExecutableBasename;
			gchar								*windowExecutableBasename;

			/* Get basename of both executables */
			iterExecutableBasename=g_path_get_basename(iterExecutable);
			windowExecutableBasename=g_path_get_basename(windowExecutable);

			/* Compare basename of both executables */
			executableMatches=(g_strcmp0(iterExecutableBasename, windowExecutableBasename)==0 ? TRUE : FALSE);

			/* Release allocated resources */
			g_free(iterExecutableBasename);
			g_free(windowExecutableBasename);
		}
			else
			{
				/* Both executable have absolute path so compare them entirely */
				executableMatches=(g_strcmp0(iterExecutable, windowExecutable)==0 ? TRUE : FALSE);
			}

		/* If both executables matched then we found the AppInfo */
		if(executableMatches)
		{
			appInfo=G_APP_INFO(g_object_ref(iterAppInfo));
		}
	}

	/* Release allocated resources */
	if(apps) g_list_free_full(apps, g_object_unref);
	if(appDatabase) g_object_unref(appDatabase);
	if(windowExecutable) g_free(windowExecutable);

	/* Return found AppInfo */
	return(appInfo);
}

/* Get state of window */
static void _xfdashboard_window_tracker_window_x11_update_state(XfdashboardWindowTrackerWindowX11 *self)
{
	XfdashboardWindowTrackerWindowX11Private	*priv;
	XfdashboardWindowTrackerWindowState			newState;
	WnckWindowState								wnckState;

	g_return_if_fail(XFDASHBOARD_IS_WINDOW_TRACKER_WINDOW_X11(self));

	priv=self->priv;
	newState=0;

	/* A wnck window must be wrapped by this object */
	if(!priv->window)
	{
		XFDASHBOARD_WINDOW_TRACKER_WINDOW_X11_WARN_NO_WINDOW(self);
	}
		else
		{
			/* Get state of wnck window to determine state */
			wnckState=wnck_window_get_state(priv->window);

			/* Determine window state */
			if(wnckState & WNCK_WINDOW_STATE_HIDDEN) newState|=XFDASHBOARD_WINDOW_TRACKER_WINDOW_STATE_HIDDEN;

			if(wnckState & WNCK_WINDOW_STATE_MINIMIZED)
			{
				newState|=XFDASHBOARD_WINDOW_TRACKER_WINDOW_STATE_MINIMIZED;
			}
				else
				{
					if((wnckState & WNCK_WINDOW_STATE_MAXIMIZED_HORIZONTALLY) &&
						(wnckState & WNCK_WINDOW_STATE_MAXIMIZED_VERTICALLY))
					{
						newState|=XFDASHBOARD_WINDOW_TRACKER_WINDOW_STATE_MAXIMIZED;
					}
				}

			if(wnckState & WNCK_WINDOW_STATE_FULLSCREEN) newState|=XFDASHBOARD_WINDOW_TRACKER_WINDOW_STATE_FULLSCREEN;
			if(wnckState & WNCK_WINDOW_STATE_SKIP_PAGER) newState|=XFDASHBOARD_WINDOW_TRACKER_WINDOW_STATE_SKIP_PAGER;
			if(wnckState & WNCK_WINDOW_STATE_SKIP_TASKLIST) newState|=XFDASHBOARD_WINDOW_TRACKER_WINDOW_STATE_SKIP_TASKLIST;
			if(wnckState & WNCK_WINDOW_STATE_DEMANDS_ATTENTION) newState|=XFDASHBOARD_WINDOW_TRACKER_WINDOW_STATE_URGENT;
			if(wnckState & WNCK_WINDOW_STATE_URGENT) newState|=XFDASHBOARD_WINDOW_TRACKER_WINDOW_STATE_URGENT;

			/* "Pin" is not a wnck window state and do not get confused with the
			 * "sticky" state as it refers only to the window's stickyness on
			 * the viewport. So we have to ask wnck if it is pinned.
			 */
			if(wnck_window_is_pinned(priv->window)) newState|=XFDASHBOARD_WINDOW_TRACKER_WINDOW_STATE_PINNED;
		}

	/* Set value if changed */
	if(priv->state!=newState)
	{
		/* Set value */
		priv->state=newState;

		/* Notify about property change */
		g_object_notify_by_pspec(G_OBJECT(self), XfdashboardWindowTrackerWindowX11Properties[PROP_STATE]);
	}
}

/* Get actions of window */
static void _xfdashboard_window_tracker_window_x11_update_actions(XfdashboardWindowTrackerWindowX11 *self)
{
	XfdashboardWindowTrackerWindowX11Private	*priv;
	XfdashboardWindowTrackerWindowAction		newActions;
	WnckWindowActions							wnckActions;

	g_return_if_fail(XFDASHBOARD_IS_WINDOW_TRACKER_WINDOW_X11(self));

	priv=self->priv;
	newActions=0;

	/* A wnck window must be wrapped by this object */
	if(!priv->window)
	{
		XFDASHBOARD_WINDOW_TRACKER_WINDOW_X11_WARN_NO_WINDOW(self);
	}
		else
		{
			/* Get actions of wnck window to determine state */
			wnckActions=wnck_window_get_actions(priv->window);

			/* Determine window actions */
			if(wnckActions & WNCK_WINDOW_ACTION_CLOSE) newActions|=XFDASHBOARD_WINDOW_TRACKER_WINDOW_ACTION_CLOSE;
		}

	/* Set value if changed */
	if(priv->actions!=newActions)
	{
		/* Set value */
		priv->actions=newActions;

		/* Notify about property change */
		g_object_notify_by_pspec(G_OBJECT(self), XfdashboardWindowTrackerWindowX11Properties[PROP_ACTIONS]);
	}
}

/* Proxy signal for mapped wnck window which changed name */
static void _xfdashboard_window_tracker_window_x11_on_wnck_name_changed(XfdashboardWindowTrackerWindowX11 *self,
																		gpointer inUserData)
{
	XfdashboardWindowTrackerWindowX11Private	*priv;
	WnckWindow									*window;

	g_return_if_fail(XFDASHBOARD_IS_WINDOW_TRACKER_WINDOW_X11(self));
	g_return_if_fail(WNCK_IS_WINDOW(inUserData));

	priv=self->priv;
	window=WNCK_WINDOW(inUserData);

	/* Check that window emitting this signal is the mapped window of this object */
	if(priv->window!=window)
	{
		XFDASHBOARD_WINDOW_TRACKER_WINDOW_X11_WARN_WRONG_WINDOW(self);
		return;
	}

	/* Proxy signal */
	g_signal_emit_by_name(self, "name-changed");
}

/* Proxy signal for mapped wnck window which changed states */
static void _xfdashboard_window_tracker_window_x11_on_wnck_state_changed(XfdashboardWindowTrackerWindowX11 *self,
																			WnckWindowState inChangedStates,
																			WnckWindowState inNewState,
																			gpointer inUserData)
{
	XfdashboardWindowTrackerWindowX11Private	*priv;
	WnckWindow									*window;
	XfdashboardWindowTrackerWindowState			oldStates;

	g_return_if_fail(XFDASHBOARD_IS_WINDOW_TRACKER_WINDOW_X11(self));
	g_return_if_fail(WNCK_IS_WINDOW(inUserData));

	priv=self->priv;
	window=WNCK_WINDOW(inUserData);

	/* Check that window emitting this signal is the mapped window of this object */
	if(priv->window!=window)
	{
		XFDASHBOARD_WINDOW_TRACKER_WINDOW_X11_WARN_WRONG_WINDOW(self);
		return;
	}

	/* Remember current states as old ones for signal emission before updating them */
	oldStates=priv->state;

	/* Update state before emitting signal */
	_xfdashboard_window_tracker_window_x11_update_state(self);

	/* Proxy signal */
	g_signal_emit_by_name(self, "state-changed", oldStates);
}

/* Proxy signal for mapped wnck window which changed actions */
static void _xfdashboard_window_tracker_window_x11_on_wnck_actions_changed(XfdashboardWindowTrackerWindowX11 *self,
																			WnckWindowActions inChangedActions,
																			WnckWindowActions inNewActions,
																			gpointer inUserData)
{
	XfdashboardWindowTrackerWindowX11Private	*priv;
	WnckWindow									*window;
	XfdashboardWindowTrackerWindowAction		oldActions;

	g_return_if_fail(XFDASHBOARD_IS_WINDOW_TRACKER_WINDOW_X11(self));
	g_return_if_fail(WNCK_IS_WINDOW(inUserData));

	priv=self->priv;
	window=WNCK_WINDOW(inUserData);

	/* Check that window emitting this signal is the mapped window of this object */
	if(priv->window!=window)
	{
		XFDASHBOARD_WINDOW_TRACKER_WINDOW_X11_WARN_WRONG_WINDOW(self);
		return;
	}

	/* Remember current actions as old ones for signal emission before updating them */
	oldActions=priv->actions;

	/* Update actions before emitting signal */
	_xfdashboard_window_tracker_window_x11_update_actions(self);

	/* Proxy signal */
	g_signal_emit_by_name(self, "actions-changed", oldActions);
}

/* Proxy signal for mapped wnck window which changed icon */
static void _xfdashboard_window_tracker_window_x11_on_wnck_icon_changed(XfdashboardWindowTrackerWindowX11 *self,
																		gpointer inUserData)
{
	XfdashboardWindowTrackerWindowX11Private	*priv;
	WnckWindow									*window;

	g_return_if_fail(XFDASHBOARD_IS_WINDOW_TRACKER_WINDOW_X11(self));
	g_return_if_fail(WNCK_IS_WINDOW(inUserData));

	priv=self->priv;
	window=WNCK_WINDOW(inUserData);

	/* Check that window emitting this signal is the mapped window of this object */
	if(priv->window!=window)
	{
		XFDASHBOARD_WINDOW_TRACKER_WINDOW_X11_WARN_WRONG_WINDOW(self);
		return;
	}

	/* Proxy signal */
	g_signal_emit_by_name(self, "icon-changed");
}

/* Proxy signal for mapped wnck window which changed workspace */
static void _xfdashboard_window_tracker_window_x11_on_wnck_workspace_changed(XfdashboardWindowTrackerWindowX11 *self,
																				gpointer inUserData)
{
	XfdashboardWindowTrackerWindowX11Private	*priv;
	WnckWindow									*window;
	XfdashboardWindowTrackerWorkspace			*oldWorkspace;

	g_return_if_fail(XFDASHBOARD_IS_WINDOW_TRACKER_WINDOW_X11(self));
	g_return_if_fail(WNCK_IS_WINDOW(inUserData));

	priv=self->priv;
	window=WNCK_WINDOW(inUserData);

	/* Check that window emitting this signal is the mapped window of this object */
	if(priv->window!=window)
	{
		XFDASHBOARD_WINDOW_TRACKER_WINDOW_X11_WARN_WRONG_WINDOW(self);
		return;
	}

	/* Get mapped workspace object for last known workspace of this window */
	oldWorkspace=NULL;
	if(priv->workspace)
	{
		XfdashboardWindowTracker				*windowTracker;

		windowTracker=xfdashboard_core_get_window_tracker(NULL);
		oldWorkspace=xfdashboard_window_tracker_x11_get_workspace_for_wnck(XFDASHBOARD_WINDOW_TRACKER_X11(windowTracker), priv->workspace);
		g_object_unref(windowTracker);
	}

	/* Proxy signal */
	g_signal_emit_by_name(self, "workspace-changed", oldWorkspace);

	/* Remember new workspace as last known workspace */
	priv->workspace=wnck_window_get_workspace(window);
}

/* Proxy signal for mapped wnck window which changed geometry */
static void _xfdashboard_window_tracker_window_x11_on_wnck_geometry_changed(XfdashboardWindowTrackerWindowX11 *self,
																			gpointer inUserData)
{
	XfdashboardWindowTrackerWindowX11Private	*priv;
	WnckWindow									*window;
	gint										x, y, width, height;

	g_return_if_fail(XFDASHBOARD_IS_WINDOW_TRACKER_WINDOW_X11(self));
	g_return_if_fail(WNCK_IS_WINDOW(inUserData));

	priv=self->priv;
	window=WNCK_WINDOW(inUserData);

	/* Check that window emitting this signal is the mapped window of this object */
	if(priv->window!=window)
	{
		XFDASHBOARD_WINDOW_TRACKER_WINDOW_X11_WARN_WRONG_WINDOW(self);
		return;
	}

	/* Get current position and size of window and check against last known
	 * position and size of window to determine if window has moved or resized.
	 */
	wnck_window_get_geometry(priv->window, &x, &y, &width, &height);
	if(priv->lastGeometryX!=x ||
		priv->lastGeometryY!=y ||
		priv->lastGeometryWidth!=width ||
		priv->lastGeometryHeight!=height)
	{
		XfdashboardWindowTracker				*windowTracker;
		gint									screenWidth, screenHeight;
		XfdashboardWindowTrackerMonitor			*oldMonitor;
		XfdashboardWindowTrackerMonitor			*currentMonitor;
		gint									windowMiddleX, windowMiddleY;

		/* Get window tracker */
		windowTracker=xfdashboard_core_get_window_tracker(NULL);

		/* Get monitor at old position of window and the monitor at current.
		 * If they differ emit signal for window changed monitor.
		 */
		xfdashboard_window_tracker_get_screen_size(windowTracker, &screenWidth, &screenHeight);

		windowMiddleX=priv->lastGeometryX+(priv->lastGeometryWidth/2);
		if(windowMiddleX>screenWidth) windowMiddleX=screenWidth-1;

		windowMiddleY=priv->lastGeometryY+(priv->lastGeometryHeight/2);
		if(windowMiddleY>screenHeight) windowMiddleY=screenHeight-1;

		oldMonitor=xfdashboard_window_tracker_get_monitor_by_position(windowTracker, windowMiddleX, windowMiddleY);

		currentMonitor=xfdashboard_window_tracker_window_get_monitor(XFDASHBOARD_WINDOW_TRACKER_WINDOW(self));

		if(currentMonitor!=oldMonitor)
		{
			/* Emit signal */
			XFDASHBOARD_DEBUG(self, WINDOWS,
								"Window '%s' moved from monitor %d (%s) to %d (%s)",
								wnck_window_get_name(priv->window),
								oldMonitor ? xfdashboard_window_tracker_monitor_get_number(oldMonitor) : -1,
								(oldMonitor && xfdashboard_window_tracker_monitor_is_primary(oldMonitor)) ? "primary" : "non-primary",
								currentMonitor ? xfdashboard_window_tracker_monitor_get_number(currentMonitor) : -1,
								(currentMonitor && xfdashboard_window_tracker_monitor_is_primary(currentMonitor)) ? "primary" : "non-primary");
			g_signal_emit_by_name(self, "monitor-changed", oldMonitor);
		}

		/* Remember current position and size as last known ones */
		priv->lastGeometryX=x;
		priv->lastGeometryY=y;
		priv->lastGeometryWidth=width;
		priv->lastGeometryHeight=height;

		/* Release allocated resources */
		g_object_unref(windowTracker);
	}

	/* Proxy signal */
	g_signal_emit_by_name(self, "geometry-changed");
}

/* Set wnck window to map in this window object */
static void _xfdashboard_window_tracker_window_x11_set_window(XfdashboardWindowTrackerWindowX11 *self,
																WnckWindow *inWindow)
{
	XfdashboardWindowTrackerWindowX11Private	*priv;

	g_return_if_fail(XFDASHBOARD_IS_WINDOW_TRACKER_WINDOW_X11(self));
	g_return_if_fail(!inWindow || WNCK_IS_WINDOW(inWindow));

	priv=self->priv;

	/* Set value if changed */
	if(priv->window!=inWindow)
	{
		/* If we have created a content for this window then remove weak reference
		 * and reset content variable to NULL. First call to get window content
		 * will recreate it. Already used contents will not be affected.
		 */
		if(priv->content)
		{
			XFDASHBOARD_DEBUG(self, WINDOWS,
								"Removing cached content with ref-count %d from %s@%p for wnck-window %p because wnck-window will change to %p",
								G_OBJECT(priv->content)->ref_count,
								G_OBJECT_TYPE_NAME(self), self,
								priv->window,
								inWindow);
			g_object_remove_weak_pointer(G_OBJECT(priv->content), (gpointer*)&priv->content);
			priv->content=NULL;
		}

		/* Disconnect signals to old window (if available) and reset states */
		if(priv->window)
		{
			/* Remove weak reference at old window */
			g_object_remove_weak_pointer(G_OBJECT(priv->window), (gpointer*)&priv->window);

			/* Disconnect signal handlers */
			g_signal_handlers_disconnect_by_data(priv->window, self);
			priv->window=NULL;
		}
		priv->state=0;
		priv->actions=0;
		priv->workspace=NULL;

		/* Set new value */
		priv->window=inWindow;

		/* Initialize states and connect signals if window is set */
		if(priv->window)
		{
			/* Add weak reference at new window */
			g_object_add_weak_pointer(G_OBJECT(priv->window), (gpointer*)&priv->window);

			/* Initialize states */
			_xfdashboard_window_tracker_window_x11_update_state(self);
			_xfdashboard_window_tracker_window_x11_update_actions(self);
			priv->workspace=wnck_window_get_workspace(priv->window);
			wnck_window_get_geometry(priv->window,
										&priv->lastGeometryX,
										&priv->lastGeometryY,
										&priv->lastGeometryWidth,
										&priv->lastGeometryHeight);

			/* Connect signals */
			g_signal_connect_swapped(priv->window,
										"name-changed",
										G_CALLBACK(_xfdashboard_window_tracker_window_x11_on_wnck_name_changed),
										self);
			g_signal_connect_swapped(priv->window,
										"state-changed",
										G_CALLBACK(_xfdashboard_window_tracker_window_x11_on_wnck_state_changed),
										self);
			g_signal_connect_swapped(priv->window,
										"actions-changed",
										G_CALLBACK(_xfdashboard_window_tracker_window_x11_on_wnck_actions_changed),
										self);
			g_signal_connect_swapped(priv->window,
										"icon-changed",
										G_CALLBACK(_xfdashboard_window_tracker_window_x11_on_wnck_icon_changed),
										self);
			g_signal_connect_swapped(priv->window,
										"workspace-changed",
										G_CALLBACK(_xfdashboard_window_tracker_window_x11_on_wnck_workspace_changed),
										self);
			g_signal_connect_swapped(priv->window,
										"geometry-changed",
										G_CALLBACK(_xfdashboard_window_tracker_window_x11_on_wnck_geometry_changed),
										self);
		}

		/* Notify about property change */
		g_object_notify_by_pspec(G_OBJECT(self), XfdashboardWindowTrackerWindowX11Properties[PROP_WINDOW]);
	}
}


/* IMPLEMENTATION: Interface XfdashboardWindowTrackerWindow */

/* Determine if window is visible */
static gboolean _xfdashboard_window_tracker_window_x11_window_tracker_window_is_visible(XfdashboardWindowTrackerWindow *inWindow)
{
	XfdashboardWindowTrackerWindowX11			*self;
	XfdashboardWindowTrackerWindowX11Private	*priv;

	g_return_val_if_fail(XFDASHBOARD_IS_WINDOW_TRACKER_WINDOW_X11(inWindow), FALSE);

	self=XFDASHBOARD_WINDOW_TRACKER_WINDOW_X11(inWindow);
	priv=self->priv;

	/* Windows are invisible if hidden but not minimized */
	if((priv->state & XFDASHBOARD_WINDOW_TRACKER_WINDOW_STATE_HIDDEN) &&
		!(priv->state & XFDASHBOARD_WINDOW_TRACKER_WINDOW_STATE_MINIMIZED))
	{
		return(FALSE);
	}

	/* If we get here the window is visible */
	return(TRUE);
}

/* Show window */
static void _xfdashboard_window_tracker_window_x11_window_tracker_window_show(XfdashboardWindowTrackerWindow *inWindow)
{
	XfdashboardWindowTrackerWindowX11			*self;
	XfdashboardWindowTrackerWindowX11Private	*priv;

	g_return_if_fail(XFDASHBOARD_IS_WINDOW_TRACKER_WINDOW_X11(inWindow));

	self=XFDASHBOARD_WINDOW_TRACKER_WINDOW_X11(inWindow);
	priv=self->priv;

	/* A wnck window must be wrapped by this object */
	if(!priv->window)
	{
		XFDASHBOARD_WINDOW_TRACKER_WINDOW_X11_WARN_NO_WINDOW(self);
		return;
	}

	/* Show (unminize) window */
	wnck_window_unminimize(priv->window, xfdashboard_window_tracker_x11_get_time());
}

/* Show window */
static void _xfdashboard_window_tracker_window_x11_window_tracker_window_hide(XfdashboardWindowTrackerWindow *inWindow)
{
	XfdashboardWindowTrackerWindowX11			*self;
	XfdashboardWindowTrackerWindowX11Private	*priv;

	g_return_if_fail(XFDASHBOARD_IS_WINDOW_TRACKER_WINDOW_X11(inWindow));

	self=XFDASHBOARD_WINDOW_TRACKER_WINDOW_X11(inWindow);
	priv=self->priv;

	/* A wnck window must be wrapped by this object */
	if(!priv->window)
	{
		XFDASHBOARD_WINDOW_TRACKER_WINDOW_X11_WARN_NO_WINDOW(self);
		return;
	}

	/* Hide (minimize) window */
	wnck_window_minimize(priv->window);
}

/* Get parent window if this window is a child window */
static XfdashboardWindowTrackerWindow* _xfdashboard_window_tracker_window_x11_window_tracker_window_get_parent(XfdashboardWindowTrackerWindow *inWindow)
{
	XfdashboardWindowTrackerWindowX11			*self;
	XfdashboardWindowTrackerWindowX11Private	*priv;
	WnckWindow									*parentWindow;
	XfdashboardWindowTracker					*windowTracker;
	XfdashboardWindowTrackerWindow				*foundWindow;

	g_return_val_if_fail(XFDASHBOARD_IS_WINDOW_TRACKER_WINDOW_X11(inWindow), NULL);

	self=XFDASHBOARD_WINDOW_TRACKER_WINDOW_X11(inWindow);
	priv=self->priv;

	/* A wnck window must be wrapped by this object */
	if(!priv->window)
	{
		XFDASHBOARD_WINDOW_TRACKER_WINDOW_X11_WARN_NO_WINDOW(self);
		return(NULL);
	}

	/* Get parent window */
	parentWindow=wnck_window_get_transient(priv->window);
	if(!parentWindow) return(NULL);

	/* Get window tracker and lookup the mapped and matching XfdashboardWindowTrackerWindow
	 * for wnck window.
	 */
	windowTracker=xfdashboard_core_get_window_tracker(NULL);
	foundWindow=xfdashboard_window_tracker_x11_get_window_for_wnck(XFDASHBOARD_WINDOW_TRACKER_X11(windowTracker), parentWindow);
	g_object_unref(windowTracker);

	/* Return found window object */
	return(foundWindow);
}

/* Get window state */
static XfdashboardWindowTrackerWindowState _xfdashboard_window_tracker_window_x11_window_tracker_window_get_state(XfdashboardWindowTrackerWindow *inWindow)
{
	XfdashboardWindowTrackerWindowX11			*self;
	XfdashboardWindowTrackerWindowX11Private	*priv;

	g_return_val_if_fail(XFDASHBOARD_IS_WINDOW_TRACKER_WINDOW_X11(inWindow), 0);

	self=XFDASHBOARD_WINDOW_TRACKER_WINDOW_X11(inWindow);
	priv=self->priv;

	/* Return state of window */
	return(priv->state);
}

/* Set window state */
static void _xfdashboard_window_tracker_window_x11_window_tracker_window_set_state(XfdashboardWindowTrackerWindow *inWindow, XfdashboardWindowTrackerWindowState inState)
{
	XfdashboardWindowTrackerWindowX11			*self;
	XfdashboardWindowTrackerWindowX11Private	*priv;

	g_return_if_fail(XFDASHBOARD_IS_WINDOW_TRACKER_WINDOW_X11(inWindow));

	self=XFDASHBOARD_WINDOW_TRACKER_WINDOW_X11(inWindow);
	priv=self->priv;

	/* A wnck window must be wrapped by this object */
	if(!priv->window)
	{
		XFDASHBOARD_WINDOW_TRACKER_WINDOW_X11_WARN_NO_WINDOW(self);
		return;
	}

	/* Set state if changed */
	if(priv->state!=inState)
	{
		XfdashboardWindowTrackerWindowState		changedStates;

		/* Get state changed */
		changedStates=priv->state ^ inState;
		XFDASHBOARD_DEBUG(self, WINDOWS,
							"Window '%s' for wnck-window %p changed state from %u to %u (changed-mask=%u)",
							wnck_window_get_name(priv->window),
							priv->window,
							priv->state,
							inState,
							changedStates);

		/* Iterate through changed states and update window */
		if((changedStates & XFDASHBOARD_WINDOW_TRACKER_WINDOW_STATE_HIDDEN) ||
			(changedStates & XFDASHBOARD_WINDOW_TRACKER_WINDOW_STATE_MINIMIZED))
		{
			if((inState & XFDASHBOARD_WINDOW_TRACKER_WINDOW_STATE_HIDDEN) ||
				(inState & XFDASHBOARD_WINDOW_TRACKER_WINDOW_STATE_MINIMIZED))
			{
				wnck_window_minimize(priv->window);
			}
				else wnck_window_unminimize(priv->window, xfdashboard_window_tracker_x11_get_time());
		}

		if(changedStates & XFDASHBOARD_WINDOW_TRACKER_WINDOW_STATE_MAXIMIZED)
		{
			if(inState & XFDASHBOARD_WINDOW_TRACKER_WINDOW_STATE_MAXIMIZED) wnck_window_maximize(priv->window);
				else wnck_window_unmaximize(priv->window);
		}

		if(changedStates & XFDASHBOARD_WINDOW_TRACKER_WINDOW_STATE_FULLSCREEN)
		{
			if(inState & XFDASHBOARD_WINDOW_TRACKER_WINDOW_STATE_FULLSCREEN) wnck_window_set_fullscreen(priv->window, TRUE);
				else wnck_window_set_fullscreen(priv->window, FALSE);
		}

		if(changedStates & XFDASHBOARD_WINDOW_TRACKER_WINDOW_STATE_SKIP_PAGER)
		{
			if(inState & XFDASHBOARD_WINDOW_TRACKER_WINDOW_STATE_SKIP_PAGER) wnck_window_set_skip_pager(priv->window, TRUE);
				else wnck_window_set_skip_pager(priv->window, FALSE);
		}

		if(changedStates & XFDASHBOARD_WINDOW_TRACKER_WINDOW_STATE_SKIP_TASKLIST)
		{
			if(inState & XFDASHBOARD_WINDOW_TRACKER_WINDOW_STATE_SKIP_TASKLIST) wnck_window_set_skip_tasklist(priv->window, TRUE);
				else wnck_window_set_skip_tasklist(priv->window, FALSE);
		}

		if(changedStates & XFDASHBOARD_WINDOW_TRACKER_WINDOW_STATE_PINNED)
		{
			if(inState & XFDASHBOARD_WINDOW_TRACKER_WINDOW_STATE_PINNED) wnck_window_pin(priv->window);
				else wnck_window_unpin(priv->window);
		}

		if(changedStates & XFDASHBOARD_WINDOW_TRACKER_WINDOW_STATE_URGENT)
		{
			Display								*display;
			gulong								windowXID;
			gint								trapError;
			XWMHints							*hints;

			display=xfdashboard_window_tracker_x11_get_display();
			windowXID=wnck_window_get_xid(priv->window);

			/* Get current X window hints */
			clutter_x11_trap_x_errors();
			{
				hints=XGetWMHints(display, windowXID);
			}

			/* Check if everything went well */
			trapError=clutter_x11_untrap_x_errors();
			if(trapError!=0)
			{
				XFDASHBOARD_DEBUG(self, WINDOWS,
									"X error %d occured while getting WM hints window '%s'",
									trapError,
									wnck_window_get_name(priv->window));

				/* Ensure hints is a NULL pointer */
				hints=NULL;
			}

			/* If we could get the window hints just add the urgency flag */
			if(hints)
			{
				/* Set urgency flag */
				hints->flags|=XUrgencyHint;

				/* Set new X window hints on window */
				XSetWMHints(display, windowXID, hints);

				/* Release allocated resources */
				XFree(hints);
			}
		}

		/* We do not set the requested window state here and emit a property
		 * changed signal as we might need to wait until X server updates
		 * the window state. So the new state will not reflect the current
		 * or the requested state. But if the window state is update fully
		 * or just partial the signal handler connected to wnck will update
		 * the state. So it is safe not to do it here.
		 */
	}
}

/* Get window actions */
static XfdashboardWindowTrackerWindowAction _xfdashboard_window_tracker_window_x11_window_tracker_window_get_actions(XfdashboardWindowTrackerWindow *inWindow)
{
	XfdashboardWindowTrackerWindowX11			*self;
	XfdashboardWindowTrackerWindowX11Private	*priv;

	g_return_val_if_fail(XFDASHBOARD_IS_WINDOW_TRACKER_WINDOW_X11(inWindow), 0);

	self=XFDASHBOARD_WINDOW_TRACKER_WINDOW_X11(inWindow);
	priv=self->priv;

	/* Return actions of window */
	return(priv->actions);
}

/* Get name (title) of window */
static const gchar* _xfdashboard_window_tracker_window_x11_window_tracker_window_get_name(XfdashboardWindowTrackerWindow *inWindow)
{
	XfdashboardWindowTrackerWindowX11			*self;
	XfdashboardWindowTrackerWindowX11Private	*priv;

	g_return_val_if_fail(XFDASHBOARD_IS_WINDOW_TRACKER_WINDOW_X11(inWindow), NULL);

	self=XFDASHBOARD_WINDOW_TRACKER_WINDOW_X11(inWindow);
	priv=self->priv;

	/* A wnck window must be wrapped by this object */
	if(!priv->window)
	{
		XFDASHBOARD_WINDOW_TRACKER_WINDOW_X11_WARN_NO_WINDOW(self);
		return(NULL);
	}

	/* Check if window has a name to return and return name or NULL */
	if(!wnck_window_has_name(priv->window)) return(NULL);

	return(wnck_window_get_name(priv->window));
}

/* Get icon of window */
static GdkPixbuf* _xfdashboard_window_tracker_window_x11_window_tracker_window_get_icon(XfdashboardWindowTrackerWindow *inWindow)
{
	XfdashboardWindowTrackerWindowX11			*self;
	XfdashboardWindowTrackerWindowX11Private	*priv;

	g_return_val_if_fail(XFDASHBOARD_IS_WINDOW_TRACKER_WINDOW_X11(inWindow), NULL);

	self=XFDASHBOARD_WINDOW_TRACKER_WINDOW_X11(inWindow);
	priv=self->priv;

	/* A wnck window must be wrapped by this object */
	if(!priv->window)
	{
		XFDASHBOARD_WINDOW_TRACKER_WINDOW_X11_WARN_NO_WINDOW(self);
		return(NULL);
	}

	/* Return icon as pixbuf of window */
	return(wnck_window_get_icon(priv->window));
}

static const gchar* _xfdashboard_window_tracker_window_x11_window_tracker_window_get_icon_name(XfdashboardWindowTrackerWindow *inWindow)
{
	XfdashboardWindowTrackerWindowX11			*self;
	XfdashboardWindowTrackerWindowX11Private	*priv;

	g_return_val_if_fail(XFDASHBOARD_IS_WINDOW_TRACKER_WINDOW_X11(inWindow), NULL);

	self=XFDASHBOARD_WINDOW_TRACKER_WINDOW_X11(inWindow);
	priv=self->priv;

	/* A wnck window must be wrapped by this object */
	if(!priv->window)
	{
		XFDASHBOARD_WINDOW_TRACKER_WINDOW_X11_WARN_NO_WINDOW(self);
		return(NULL);
	}

	/* Check if window has an icon name to return and return icon name or NULL */
	if(!wnck_window_has_icon_name(priv->window)) return(NULL);

	return(wnck_window_get_icon_name(priv->window));
}

/* Get workspace where window is on */
static XfdashboardWindowTrackerWorkspace* _xfdashboard_window_tracker_window_x11_window_tracker_window_get_workspace(XfdashboardWindowTrackerWindow *inWindow)
{
	XfdashboardWindowTrackerWindowX11			*self;
	XfdashboardWindowTrackerWindowX11Private	*priv;
	WnckWorkspace								*wantedWorkspace;
	XfdashboardWindowTracker					*windowTracker;
	XfdashboardWindowTrackerWorkspace			*foundWorkspace;

	g_return_val_if_fail(XFDASHBOARD_IS_WINDOW_TRACKER_WINDOW_X11(inWindow), NULL);

	self=XFDASHBOARD_WINDOW_TRACKER_WINDOW_X11(inWindow);
	priv=self->priv;

	/* A wnck window must be wrapped by this object */
	if(!priv->window)
	{
		XFDASHBOARD_WINDOW_TRACKER_WINDOW_X11_WARN_NO_WINDOW(self);
		return(NULL);
	}

	/* Get real wnck workspace of window to lookup a mapped and matching
	 * XfdashboardWindowTrackerWorkspace object.
	 * NOTE: Workspace may be NULL. In this case return NULL immediately and
	 *       do not lookup a matching workspace object.
	 */
	wantedWorkspace=wnck_window_get_workspace(priv->window);
	if(!wantedWorkspace) return(NULL);

	/* Get window tracker and lookup the mapped and matching XfdashboardWindowTrackerWorkspace
	 * for wnck workspace.
	 */
	windowTracker=xfdashboard_core_get_window_tracker(NULL);
	foundWorkspace=xfdashboard_window_tracker_x11_get_workspace_for_wnck(XFDASHBOARD_WINDOW_TRACKER_X11(windowTracker), wantedWorkspace);
	g_object_unref(windowTracker);

	/* Return found workspace */
	return(foundWorkspace);
}

/* Determine if window is on requested workspace */
static gboolean _xfdashboard_window_tracker_window_x11_window_tracker_window_is_on_workspace(XfdashboardWindowTrackerWindow *inWindow,
																								XfdashboardWindowTrackerWorkspace *inWorkspace)
{
	XfdashboardWindowTrackerWindowX11			*self;
	XfdashboardWindowTrackerWindowX11Private	*priv;
	WnckWorkspace								*workspace;

	g_return_val_if_fail(XFDASHBOARD_IS_WINDOW_TRACKER_WINDOW_X11(inWindow), FALSE);
	g_return_val_if_fail(XFDASHBOARD_IS_WINDOW_TRACKER_WORKSPACE_X11(inWorkspace), FALSE);

	self=XFDASHBOARD_WINDOW_TRACKER_WINDOW_X11(inWindow);
	priv=self->priv;

	/* A wnck window must be wrapped by this object */
	if(!priv->window)
	{
		XFDASHBOARD_WINDOW_TRACKER_WINDOW_X11_WARN_NO_WINDOW(self);
		return(FALSE);
	}

	/* Get wnck workspace to check if window is on this one */
	workspace=xfdashboard_window_tracker_workspace_x11_get_workspace(XFDASHBOARD_WINDOW_TRACKER_WORKSPACE_X11(inWorkspace));
	if(!workspace)
	{
		g_critical("Either no wnck workspace is wrapped at %s or workspace is not available anymore when called at function %s",
					G_OBJECT_TYPE_NAME(inWorkspace),
					__func__);
		return(FALSE);
	}

	/* Check if window is on that workspace */
	return(wnck_window_is_on_workspace(priv->window, workspace));
}

/* Get geometry (position and size) of window */
static void _xfdashboard_window_tracker_window_x11_window_tracker_window_get_geometry(XfdashboardWindowTrackerWindow *inWindow,
																						gint *outX,
																						gint *outY,
																						gint *outWidth,
																						gint *outHeight)
{
	XfdashboardWindowTrackerWindowX11			*self;
	XfdashboardWindowTrackerWindowX11Private	*priv;
	gint										x, y, width, height;

	g_return_if_fail(XFDASHBOARD_IS_WINDOW_TRACKER_WINDOW_X11(inWindow));

	self=XFDASHBOARD_WINDOW_TRACKER_WINDOW_X11(inWindow);
	priv=self->priv;

	/* A wnck window must be wrapped by this object */
	if(!priv->window)
	{
		XFDASHBOARD_WINDOW_TRACKER_WINDOW_X11_WARN_NO_WINDOW(self);
		return;
	}

	/* Get window geometry */
	wnck_window_get_client_window_geometry(priv->window, &x, &y, &width, &height);

	/* Set result */
	if(outX) *outX=x;
	if(outX) *outY=y;
	if(outWidth) *outWidth=width;
	if(outHeight) *outHeight=height;
}

/* Set geometry (position and size) of window */
static void _xfdashboard_window_tracker_window_x11_window_tracker_window_set_geometry(XfdashboardWindowTrackerWindow *inWindow,
																				gint inX,
																				gint inY,
																				gint inWidth,
																				gint inHeight)
{
	XfdashboardWindowTrackerWindowX11			*self;
	XfdashboardWindowTrackerWindowX11Private	*priv;
	WnckWindowMoveResizeMask					flags;
	gint										contentX, contentY;
	gint										contentWidth, contentHeight;
	gint										borderX, borderY;
	gint										borderWidth, borderHeight;

	g_return_if_fail(XFDASHBOARD_IS_WINDOW_TRACKER_WINDOW_X11(inWindow));

	self=XFDASHBOARD_WINDOW_TRACKER_WINDOW_X11(inWindow);
	priv=self->priv;

	/* A wnck window must be wrapped by this object */
	if(!priv->window)
	{
		XFDASHBOARD_WINDOW_TRACKER_WINDOW_X11_WARN_NO_WINDOW(self);
		return;
	}

	/* Get window border size to respect it when moving window */
	wnck_window_get_client_window_geometry(priv->window, &contentX, &contentY, &contentWidth, &contentHeight);
	wnck_window_get_geometry(priv->window, &borderX, &borderY, &borderWidth, &borderHeight);

	/* Get modification flags */
	flags=0;
	if(inX>=0)
	{
		flags|=WNCK_WINDOW_CHANGE_X;
		inX-=(contentX-borderX);
	}

	if(inY>=0)
	{
		flags|=WNCK_WINDOW_CHANGE_Y;
		inY-=(contentY-borderY);
	}

	if(inWidth>=0)
	{
		flags|=WNCK_WINDOW_CHANGE_WIDTH;
		inWidth+=(borderWidth-contentWidth);
	}

	if(inHeight>=0)
	{
		flags|=WNCK_WINDOW_CHANGE_HEIGHT;
		inHeight+=(borderHeight-contentHeight);
	}

	/* Set geometry */
	wnck_window_set_geometry(priv->window,
								WNCK_WINDOW_GRAVITY_STATIC,
								flags,
								inX, inY, inWidth, inHeight);
}

/* Move window */
static void _xfdashboard_window_tracker_window_x11_window_tracker_window_move(XfdashboardWindowTrackerWindow *inWindow,
																		gint inX,
																		gint inY)
{
	_xfdashboard_window_tracker_window_x11_window_tracker_window_set_geometry(inWindow, inX, inY, -1, -1);
}

/* Resize window */
static void _xfdashboard_window_tracker_window_x11_window_tracker_window_resize(XfdashboardWindowTrackerWindow *inWindow,
																			gint inWidth,
																			gint inHeight)
{
	_xfdashboard_window_tracker_window_x11_window_tracker_window_set_geometry(inWindow, -1, -1, inWidth, inHeight);
}

/* Move a window to another workspace */
static void _xfdashboard_window_tracker_window_x11_window_tracker_window_move_to_workspace(XfdashboardWindowTrackerWindow *inWindow,
																					XfdashboardWindowTrackerWorkspace *inWorkspace)
{
	XfdashboardWindowTrackerWindowX11			*self;
	XfdashboardWindowTrackerWindowX11Private	*priv;
	WnckWorkspace								*workspace;

	g_return_if_fail(XFDASHBOARD_IS_WINDOW_TRACKER_WINDOW_X11(inWindow));
	g_return_if_fail(XFDASHBOARD_IS_WINDOW_TRACKER_WORKSPACE_X11(inWorkspace));

	self=XFDASHBOARD_WINDOW_TRACKER_WINDOW_X11(inWindow);
	priv=self->priv;

	/* A wnck window must be wrapped by this object */
	if(!priv->window)
	{
		XFDASHBOARD_WINDOW_TRACKER_WINDOW_X11_WARN_NO_WINDOW(self);
		return;
	}

	/* Get wnck workspace to move window to */
	workspace=xfdashboard_window_tracker_workspace_x11_get_workspace(XFDASHBOARD_WINDOW_TRACKER_WORKSPACE_X11(inWorkspace));
	if(!workspace)
	{
		g_critical("Either no wnck workspace is wrapped at %s or workspace is not available anymore when called at function %s",
					G_OBJECT_TYPE_NAME(inWorkspace),
					__func__);
		return;
	}

	/* Move window to workspace */
	wnck_window_move_to_workspace(priv->window, workspace);
}

/* Activate window with its transient windows */
static void _xfdashboard_window_tracker_window_x11_window_tracker_window_activate(XfdashboardWindowTrackerWindow *inWindow)
{
	XfdashboardWindowTrackerWindowX11			*self;
	XfdashboardWindowTrackerWindowX11Private	*priv;

	g_return_if_fail(XFDASHBOARD_IS_WINDOW_TRACKER_WINDOW_X11(inWindow));

	self=XFDASHBOARD_WINDOW_TRACKER_WINDOW_X11(inWindow);
	priv=self->priv;

	/* A wnck window must be wrapped by this object */
	if(!priv->window)
	{
		XFDASHBOARD_WINDOW_TRACKER_WINDOW_X11_WARN_NO_WINDOW(self);
		return;
	}

	/* Activate window */
	wnck_window_activate_transient(priv->window, xfdashboard_window_tracker_x11_get_time());
}

/* Close window */
static void _xfdashboard_window_tracker_window_x11_window_tracker_window_close(XfdashboardWindowTrackerWindow *inWindow)
{
	XfdashboardWindowTrackerWindowX11			*self;
	XfdashboardWindowTrackerWindowX11Private	*priv;

	g_return_if_fail(XFDASHBOARD_IS_WINDOW_TRACKER_WINDOW_X11(inWindow));

	self=XFDASHBOARD_WINDOW_TRACKER_WINDOW_X11(inWindow);
	priv=self->priv;

	/* A wnck window must be wrapped by this object */
	if(!priv->window)
	{
		XFDASHBOARD_WINDOW_TRACKER_WINDOW_X11_WARN_NO_WINDOW(self);
		return;
	}

	/* Close window */
	wnck_window_close(priv->window, xfdashboard_window_tracker_x11_get_time());
}

/* Get process ID owning the requested window */
static gint _xfdashboard_window_tracker_window_x11_window_tracker_window_get_pid(XfdashboardWindowTrackerWindow *inWindow)
{
	XfdashboardWindowTrackerWindowX11			*self;
	XfdashboardWindowTrackerWindowX11Private	*priv;

	g_return_val_if_fail(XFDASHBOARD_IS_WINDOW_TRACKER_WINDOW_X11(inWindow), -1);

	self=XFDASHBOARD_WINDOW_TRACKER_WINDOW_X11(inWindow);
	priv=self->priv;

	/* A wnck window must be wrapped by this object */
	if(!priv->window)
	{
		XFDASHBOARD_WINDOW_TRACKER_WINDOW_X11_WARN_NO_WINDOW(self);
		return(-1);
	}

	/* Return PID retrieved from wnck window */
	return(wnck_window_get_pid(priv->window));
}

/* Try to determine AppInfo for window */
static GAppInfo* _xfdashboard_window_tracker_window_x11_window_tracker_window_get_appinfo(XfdashboardWindowTrackerWindow *inWindow)
{
	XfdashboardWindowTrackerWindowX11			*self;
	XfdashboardWindowTrackerWindowX11Private	*priv;
	GAppInfo									*appInfo;
	const gchar									*value;

	g_return_val_if_fail(XFDASHBOARD_IS_WINDOW_TRACKER_WINDOW_X11(inWindow), NULL);

	self=XFDASHBOARD_WINDOW_TRACKER_WINDOW_X11(inWindow);
	priv=self->priv;
	appInfo=NULL;

	/* A wnck window must be wrapped by this object */
	if(!priv->window)
	{
		XFDASHBOARD_WINDOW_TRACKER_WINDOW_X11_WARN_NO_WINDOW(self);
		return(NULL);
	}

	/* If window property "_GTK_APPLICATION_ID" is available at X11 window,
	 * then read in the value of this property and try to lookup AppInfo
	 * from this value.
	 */
	if(!appInfo)
	{
		GdkScreen								*screen;
		Display									*display;
		Atom									atomGtkAppID;
		Atom									atomUTF8String;
		Atom									actualType;
		int										actualFormat;
		unsigned long							numberItems;
		unsigned long							bytesRemaining;
		unsigned char	*						data;

		data=NULL;

		/* Get X11 display from default screen */
		screen=gdk_screen_get_default();
		display=GDK_DISPLAY_XDISPLAY(gdk_screen_get_display(screen));

		/* Get X11 atoms needed for query */
		atomGtkAppID=XInternAtom(display, "_GTK_APPLICATION_ID", False);
		atomUTF8String=XInternAtom(display, "UTF8_STRING", False);

		/* Query window property "_GTK_APPLICATION_ID" from X11 window */
		XGetWindowProperty(display,
							wnck_window_get_xid(priv->window),
							atomGtkAppID,
							0, G_MAXLONG,
							False,
							atomUTF8String,
							&actualType,
							&actualFormat,
							&numberItems,
							&bytesRemaining,
							&data);

		/* If we got data from X server then add retrieved value to list */
		if(actualType==atomUTF8String &&
			actualFormat==8 &&
			numberItems>0)
		{
			appInfo=_xfdashboard_window_tracker_window_x11_window_tracker_window_resolve_name_to_appinfo(self, (gchar*)data);
		}

		/* Release allocated resources */
		if(data) XFree(data);
	}

	/* Try to resolve startup WM class of window to AppInfo if available */
	if(!appInfo)
	{
		appInfo=_xfdashboard_window_tracker_window_x11_window_tracker_window_resolve_startupwm_to_appinfo(self);
	}

	/* Try to resolve class name of window to AppInfo */
	if(!appInfo)
	{
		value=wnck_window_get_class_group_name(priv->window);
		if(value) appInfo=_xfdashboard_window_tracker_window_x11_window_tracker_window_resolve_name_to_appinfo(self, value);
	}

	/* Try to resolve instance name of window to AppInfo */
	if(!appInfo)
	{
		value=wnck_window_get_class_instance_name(priv->window);
		if(value) appInfo=_xfdashboard_window_tracker_window_x11_window_tracker_window_resolve_name_to_appinfo(self, value);
	}

	/* Try to resolve binary executable of process running window to AppInfo */
	if(!appInfo)
	{
		appInfo=_xfdashboard_window_tracker_window_x11_window_tracker_window_resolve_binary_executable_to_appinfo(self);
	}

	/* Return resolved AppInfo */
	XFDASHBOARD_DEBUG(self, WINDOWS,
						"Resolved window '%s' to desktop ID '%s'",
						wnck_window_get_name(priv->window),
						appInfo ? g_app_info_get_id(appInfo) : "<none>");
	return(appInfo);
}

/* Get content for this window for use in actors.
 * Caller is responsible to remove reference with g_object_unref().
 */
static ClutterContent* _xfdashboard_window_tracker_window_x11_window_tracker_window_get_content(XfdashboardWindowTrackerWindow *inWindow)
{
	XfdashboardWindowTrackerWindowX11			*self;
	XfdashboardWindowTrackerWindowX11Private	*priv;

	g_return_val_if_fail(XFDASHBOARD_IS_WINDOW_TRACKER_WINDOW_X11(inWindow), NULL);

	self=XFDASHBOARD_WINDOW_TRACKER_WINDOW_X11(inWindow);
	priv=self->priv;

	/* A wnck window must be wrapped by this object */
	if(!priv->window)
	{
		XFDASHBOARD_WINDOW_TRACKER_WINDOW_X11_WARN_NO_WINDOW(self);
		return(NULL);
	}

	/* Create content for window only if no content is already available. If it
	 * is available just return it with taking an extra reference on it.
	 */
	if(!priv->content)
	{
		priv->content=xfdashboard_window_content_x11_new_for_window(self);
		g_object_add_weak_pointer(G_OBJECT(priv->content), (gpointer*)&priv->content);
		XFDASHBOARD_DEBUG(self, WINDOWS,
							"Created content %s@%p for window %s@%p (wnck-window=%p)",
							priv->content ? G_OBJECT_TYPE_NAME(priv->content) : "<unknown>", priv->content,
							G_OBJECT_TYPE_NAME(self), self,
							priv->window);
	}
		else
		{
			g_object_ref(priv->content);
			XFDASHBOARD_DEBUG(self, WINDOWS,
								"Using cached content %s@%p (ref-count=%d) for window %s@%p (wnck-window=%p)",
								priv->content ? G_OBJECT_TYPE_NAME(priv->content) : "<unknown>", priv->content,
								priv->content ? G_OBJECT(priv->content)->ref_count : 0,
								G_OBJECT_TYPE_NAME(self), self,
								priv->window);
		}

	/* Return content */
	return(priv->content);
}

/* Interface initialization
 * Set up default functions
 */
static void _xfdashboard_window_tracker_window_x11_window_tracker_window_iface_init(XfdashboardWindowTrackerWindowInterface *iface)
{
	iface->is_visible=_xfdashboard_window_tracker_window_x11_window_tracker_window_is_visible;
	iface->show=_xfdashboard_window_tracker_window_x11_window_tracker_window_show;
	iface->hide=_xfdashboard_window_tracker_window_x11_window_tracker_window_hide;

	iface->get_parent=_xfdashboard_window_tracker_window_x11_window_tracker_window_get_parent;

	iface->get_state=_xfdashboard_window_tracker_window_x11_window_tracker_window_get_state;
	iface->set_state=_xfdashboard_window_tracker_window_x11_window_tracker_window_set_state;

	iface->get_actions=_xfdashboard_window_tracker_window_x11_window_tracker_window_get_actions;

	iface->get_name=_xfdashboard_window_tracker_window_x11_window_tracker_window_get_name;

	iface->get_icon=_xfdashboard_window_tracker_window_x11_window_tracker_window_get_icon;
	iface->get_icon_name=_xfdashboard_window_tracker_window_x11_window_tracker_window_get_icon_name;

	iface->get_workspace=_xfdashboard_window_tracker_window_x11_window_tracker_window_get_workspace;
	iface->is_on_workspace=_xfdashboard_window_tracker_window_x11_window_tracker_window_is_on_workspace;

	iface->get_geometry=_xfdashboard_window_tracker_window_x11_window_tracker_window_get_geometry;
	iface->set_geometry=_xfdashboard_window_tracker_window_x11_window_tracker_window_set_geometry;
	iface->move=_xfdashboard_window_tracker_window_x11_window_tracker_window_move;
	iface->resize=_xfdashboard_window_tracker_window_x11_window_tracker_window_resize;
	iface->move_to_workspace=_xfdashboard_window_tracker_window_x11_window_tracker_window_move_to_workspace;
	iface->activate=_xfdashboard_window_tracker_window_x11_window_tracker_window_activate;
	iface->close=_xfdashboard_window_tracker_window_x11_window_tracker_window_close;

	iface->get_pid=_xfdashboard_window_tracker_window_x11_window_tracker_window_get_pid;
	iface->get_appinfo=_xfdashboard_window_tracker_window_x11_window_tracker_window_get_appinfo;

	iface->get_content=_xfdashboard_window_tracker_window_x11_window_tracker_window_get_content;
}


/* IMPLEMENTATION: GObject */

/* Dispose this object */
static void _xfdashboard_window_tracker_window_x11_dispose(GObject *inObject)
{
	XfdashboardWindowTrackerWindowX11			*self=XFDASHBOARD_WINDOW_TRACKER_WINDOW_X11(inObject);
	XfdashboardWindowTrackerWindowX11Private	*priv=self->priv;

	/* Dispose allocated resources */
	if(priv->content)
	{
		XFDASHBOARD_DEBUG(self, WINDOWS,
							"Removing cached content with ref-count %d from %s@%p for wnck-window %p",
							G_OBJECT(priv->content)->ref_count,
							G_OBJECT_TYPE_NAME(self), self,
							priv->window);
		g_object_remove_weak_pointer(G_OBJECT(priv->content), (gpointer*)&priv->content);
		priv->content=NULL;
	}

	if(priv->window)
	{
		/* Remove weak reference at current window */
		g_object_remove_weak_pointer(G_OBJECT(priv->window), (gpointer*)&priv->window);

		/* Disconnect signal handlers */
		g_signal_handlers_disconnect_by_data(priv->window, self);
		priv->window=NULL;
	}

	/* Call parent's class dispose method */
	G_OBJECT_CLASS(xfdashboard_window_tracker_window_x11_parent_class)->dispose(inObject);
}

/* Set/get properties */
static void _xfdashboard_window_tracker_window_x11_set_property(GObject *inObject,
																guint inPropID,
																const GValue *inValue,
																GParamSpec *inSpec)
{
	XfdashboardWindowTrackerWindowX11		*self=XFDASHBOARD_WINDOW_TRACKER_WINDOW_X11(inObject);

	switch(inPropID)
	{
		case PROP_WINDOW:
			_xfdashboard_window_tracker_window_x11_set_window(self, WNCK_WINDOW(g_value_get_object(inValue)));
			break;

		default:
			G_OBJECT_WARN_INVALID_PROPERTY_ID(inObject, inPropID, inSpec);
			break;
	}
}

static void _xfdashboard_window_tracker_window_x11_get_property(GObject *inObject,
																guint inPropID,
																GValue *outValue,
																GParamSpec *inSpec)
{
	XfdashboardWindowTrackerWindowX11		*self=XFDASHBOARD_WINDOW_TRACKER_WINDOW_X11(inObject);

	switch(inPropID)
	{
		case PROP_WINDOW:
			g_value_set_object(outValue, self->priv->window);
			break;

		default:
			G_OBJECT_WARN_INVALID_PROPERTY_ID(inObject, inPropID, inSpec);
			break;
	}
}

/* Class initialization
 * Override functions in parent classes and define properties
 * and signals
 */
void xfdashboard_window_tracker_window_x11_class_init(XfdashboardWindowTrackerWindowX11Class *klass)
{
	GObjectClass						*gobjectClass=G_OBJECT_CLASS(klass);
	XfdashboardWindowTracker			*windowIface;
	GParamSpec							*paramSpec;

	/* Reference interface type to lookup properties etc. */
	windowIface=g_type_default_interface_ref(XFDASHBOARD_TYPE_WINDOW_TRACKER_WINDOW);

	/* Override functions */
	gobjectClass->dispose=_xfdashboard_window_tracker_window_x11_dispose;
	gobjectClass->set_property=_xfdashboard_window_tracker_window_x11_set_property;
	gobjectClass->get_property=_xfdashboard_window_tracker_window_x11_get_property;

	/* Define properties */
	XfdashboardWindowTrackerWindowX11Properties[PROP_WINDOW]=
		g_param_spec_object("window",
							"Window",
							"The mapped wnck window",
							WNCK_TYPE_WINDOW,
							G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY | G_PARAM_STATIC_STRINGS);

	paramSpec=g_object_interface_find_property(windowIface, "state");
	XfdashboardWindowTrackerWindowX11Properties[PROP_STATE]=
		g_param_spec_override("state", paramSpec);

	paramSpec=g_object_interface_find_property(windowIface, "actions");
	XfdashboardWindowTrackerWindowX11Properties[PROP_ACTIONS]=
		g_param_spec_override("actions", paramSpec);

	g_object_class_install_properties(gobjectClass, PROP_LAST, XfdashboardWindowTrackerWindowX11Properties);

	/* Release allocated resources */
	g_type_default_interface_unref(windowIface);
}

/* Object initialization
 * Create private structure and set up default values
 */
void xfdashboard_window_tracker_window_x11_init(XfdashboardWindowTrackerWindowX11 *self)
{
	XfdashboardWindowTrackerWindowX11Private	*priv;

	priv=self->priv=xfdashboard_window_tracker_window_x11_get_instance_private(self);

	/* Set default values */
	priv->window=NULL;
	priv->content=NULL;
}


/* IMPLEMENTATION: Public API */

/**
 * xfdashboard_window_tracker_window_x11_get_window:
 * @self: A #XfdashboardWindowTrackerWindowX11
 *
 * Returns the wrapped window of libwnck.
 *
 * Return value: (transfer none): the #WnckWindow wrapped by @self. The returned
 *   #WnckWindow is owned by libwnck and must not be referenced or unreferenced.
 */
WnckWindow* xfdashboard_window_tracker_window_x11_get_window(XfdashboardWindowTrackerWindowX11 *self)
{
	XfdashboardWindowTrackerWindowX11Private	*priv;

	g_return_val_if_fail(XFDASHBOARD_IS_WINDOW_TRACKER_WINDOW_X11(self), NULL);

	priv=self->priv;

	/* A wnck window must be wrapped by this object */
	if(!priv->window)
	{
		XFDASHBOARD_WINDOW_TRACKER_WINDOW_X11_WARN_NO_WINDOW(self);
		return(NULL);
	}

	/* Return wrapped libwnck window */
	return(priv->window);
}

/**
 * xfdashboard_window_tracker_window_x11_get_xid:
 * @self: A #XfdashboardWindowTrackerWindowX11
 *
 * Gets the X window ID of the wrapped libwnck's window at @self.
 *
 * Return value: the X window ID of @self.
 **/
gulong xfdashboard_window_tracker_window_x11_get_xid(XfdashboardWindowTrackerWindowX11 *self)
{
	XfdashboardWindowTrackerWindowX11Private	*priv;

	g_return_val_if_fail(XFDASHBOARD_IS_WINDOW_TRACKER_WINDOW_X11(self), None);

	priv=self->priv;

	/* A wnck window must be wrapped by this object */
	if(!priv->window)
	{
		XFDASHBOARD_WINDOW_TRACKER_WINDOW_X11_WARN_NO_WINDOW(self);
		return(None);
	}

	/* Return X window ID */
	return(wnck_window_get_xid(priv->window));
}
