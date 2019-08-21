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

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <glib/gi18n-lib.h>
#include <stdlib.h>

#include <libxfdashboard/application-tracker.h>

#include <libxfdashboard/application-database.h>
#include <libxfdashboard/window-tracker.h>
#include <libxfdashboard/marshal.h>
#include <libxfdashboard/compat.h>
#include <libxfdashboard/debug.h>


/* Define this class in GObject system */
struct _XfdashboardApplicationTrackerPrivate
{
	/* Instance related */
	GList							*runningApps;

	XfdashboardApplicationDatabase	*appDatabase;
	XfdashboardWindowTracker		*windowTracker;
};

G_DEFINE_TYPE_WITH_PRIVATE(XfdashboardApplicationTracker,
				xfdashboard_application_tracker,
				G_TYPE_OBJECT)

/* Signals */
enum
{
	SIGNAL_STATE_CHANGED,

	SIGNAL_LAST
};

static guint XfdashboardApplicationTrackerSignals[SIGNAL_LAST]={ 0, };

/* IMPLEMENTATION: Private variables and methods */

/* Single instance of application database */
static XfdashboardApplicationTracker*		_xfdashboard_application_tracker=NULL;

typedef struct _XfdashboardApplicationTrackerItem	XfdashboardApplicationTrackerItem;
struct _XfdashboardApplicationTrackerItem
{
	GPid				pid;

	GAppInfo			*appInfo;
	gchar				*desktopID;

	GList				*windows;
};

/* Remove a window from application tracker item if it exists */
static gboolean _xfdashboard_application_tracker_item_remove_window(XfdashboardApplicationTrackerItem *inItem,
																	XfdashboardWindowTrackerWindow *inWindow)
{
	GList									*iter;
	XfdashboardWindowTrackerWindow			*window;

	g_return_val_if_fail(inItem, FALSE);
	g_return_val_if_fail(XFDASHBOARD_IS_WINDOW_TRACKER_WINDOW(inWindow), FALSE);

	/* Iterate through list of windows for application tracker item and if window
	 * was found remove it from list of windows and return immediately.
	 */
	for(iter=inItem->windows; iter; iter=g_list_next(iter))
	{
		/* Get window */
		window=(XfdashboardWindowTrackerWindow*)iter->data;
		if(!window) continue;

		/* Check if window we are iterating right now is the one to lookup */
		if(window==inWindow)
		{
			inItem->windows=g_list_delete_link(inItem->windows, iter);
			return(TRUE);
		}
	}

	/* If we get here the window to remove was not found, so return FALSE here */
	return(FALSE);
}

/* Add a window to application tracker item but avoid duplicates */
static gboolean _xfdashboard_application_tracker_item_add_window(XfdashboardApplicationTrackerItem *inItem,
																XfdashboardWindowTrackerWindow *inWindow)
{
	GList									*iter;
	XfdashboardWindowTrackerWindow			*window;

	g_return_val_if_fail(inItem, FALSE);
	g_return_val_if_fail(XFDASHBOARD_IS_WINDOW_TRACKER_WINDOW(inWindow), FALSE);

	/* Iterate through list of windows for application tracker item and return
	 * immediately if requested window to add is found in list.
	 */
	for(iter=inItem->windows; iter; iter=g_list_next(iter))
	{
		/* Get window */
		window=(XfdashboardWindowTrackerWindow*)iter->data;
		if(!window) continue;

		/* Check if window we are iterating right now is the one to lookup */
		if(window==inWindow) return(FALSE);
	}

	/* If we get here the window was not found in list of windows of application
	 * tracker item, so add it to beginning of this list as it should be the
	 * last active windows for this application.
	 */
	inItem->windows=g_list_prepend(inItem->windows, inWindow);

	/* Return TRUE as window was added */
	return(TRUE);
}

/* Free application tracker item */
static void _xfdashboard_application_tracker_item_free(XfdashboardApplicationTrackerItem *inItem)
{
	g_return_if_fail(inItem);

	/* Release allocated resources of application tracker item */
	if(inItem->appInfo) g_object_unref(inItem->appInfo);
	if(inItem->desktopID) g_free(inItem->desktopID);
	if(inItem->windows) g_list_free(inItem->windows);

	/* Release allocated resources of application tracker item itself */
	g_free(inItem);
}

/* Create new application tracker item */
static XfdashboardApplicationTrackerItem* _xfdashboard_application_tracker_item_new(GAppInfo *inAppInfo,
																					XfdashboardWindowTrackerWindow *inWindow)
{
	XfdashboardApplicationTrackerItem		*item;

	g_return_val_if_fail(G_IS_APP_INFO(inAppInfo), NULL);
	g_return_val_if_fail(XFDASHBOARD_IS_WINDOW_TRACKER_WINDOW(inWindow), NULL);

	/* Create new application item and set up */
	item=g_new0(XfdashboardApplicationTrackerItem, 1);
	item->pid=xfdashboard_window_tracker_window_get_pid(inWindow);
	item->appInfo=g_object_ref(inAppInfo);
	item->desktopID=g_strdup(g_app_info_get_id(inAppInfo));
	item->windows=g_list_prepend(item->windows, inWindow);

	/* Return newly created application tracker item */
	return(item);
}

/* Find application tracker item by desktop ID */
static XfdashboardApplicationTrackerItem* _xfdashboard_application_tracker_find_item_by_desktop_id(XfdashboardApplicationTracker *self,
																									const gchar *inDesktopID)
{
	XfdashboardApplicationTrackerPrivate		*priv;
	GList										*iter;

	g_return_val_if_fail(XFDASHBOARD_IS_APPLICATION_TRACKER(self), NULL);
	g_return_val_if_fail(inDesktopID && *inDesktopID, NULL);

	priv=self->priv;

	/* Iterate through list of application tracker item and lookup the one
	 * matching the requested desktop ID.
	 */
	for(iter=priv->runningApps; iter; iter=g_list_next(iter))
	{
		XfdashboardApplicationTrackerItem		*item;

		/* Get application tracker item */
		item=(XfdashboardApplicationTrackerItem*)iter->data;
		if(!item) continue;

		/* Check if it matched requested desktop ID */
		if(g_strcmp0(item->desktopID, inDesktopID)==0) return(item);
	}

	/* If we get here no item matched requested desktop ID */
	return(NULL);
}

/* Find application tracker item by desktop application info */
static XfdashboardApplicationTrackerItem* _xfdashboard_application_tracker_find_item_by_app_info(XfdashboardApplicationTracker *self,
																									GAppInfo *inAppInfo)
{
	g_return_val_if_fail(XFDASHBOARD_IS_APPLICATION_TRACKER(self), NULL);
	g_return_val_if_fail(G_IS_APP_INFO(inAppInfo), NULL);

	return(_xfdashboard_application_tracker_find_item_by_desktop_id(self, g_app_info_get_id(inAppInfo)));
}

/* Find application tracker item by window */
static XfdashboardApplicationTrackerItem* _xfdashboard_application_tracker_find_item_by_window(XfdashboardApplicationTracker *self,
																								XfdashboardWindowTrackerWindow *inWindow)
{
	XfdashboardApplicationTrackerPrivate		*priv;
	GList										*appIter;

	g_return_val_if_fail(XFDASHBOARD_IS_APPLICATION_TRACKER(self), NULL);
	g_return_val_if_fail(XFDASHBOARD_IS_WINDOW_TRACKER_WINDOW(inWindow), NULL);

	priv=self->priv;

	/* Iterate through list of window of each application tracker item and
	 * lookup item owning the requested window.
	 */
	for(appIter=priv->runningApps; appIter; appIter=g_list_next(appIter))
	{
		XfdashboardApplicationTrackerItem		*item;
		GList									*windowIter;
		XfdashboardWindowTrackerWindow			*window;

		/* Get application tracker item */
		item=(XfdashboardApplicationTrackerItem*)appIter->data;
		if(!item) continue;

		/* Iterate through list of known window for item */
		for(windowIter=item->windows; windowIter; windowIter=g_list_next(windowIter))
		{
			/* Get window */
			window=(XfdashboardWindowTrackerWindow*)windowIter->data;
			if(!window) continue;

			/* Check if window we are iterating right now is the one to lookup */
			if(window==inWindow) return(item);
		}
	}

	/* If we get here no item owns requested window */
	return(NULL);
}

#if defined(__linux__)
/* Get process' environment set from requested PID when running at Linux
 * by reading in file in proc filesystem.
 */
static GHashTable* _xfdashboard_application_tracker_get_environment_from_pid(gint inPID)
{
	GHashTable		*environments;
	gchar			*procEnvFile;
	gchar			*envContent;
	gsize			envLength;
	GError			*error;
	gchar			*iter;

	g_return_val_if_fail(inPID>0, NULL);

	error=NULL;

	/* Create environment hash-table which will be returned on success */
	environments=g_hash_table_new_full(g_str_hash, g_str_equal, g_free, g_free);
	if(!environments)
	{
		g_warning(_("Could not create environment lookup table for PID %d"), inPID);
		return(NULL);
	}

	/* Open environment variables of process.
	 * This is the initial set of environment variables set when process was spawned.
	 * But that is ok because the environment variables we lookup are set
	 * at launch time and do not change.
	 */
	envContent=NULL;
	envLength=0;

	procEnvFile=g_strdup_printf("/proc/%d/environ", inPID);
	if(!g_file_get_contents(procEnvFile, &envContent, &envLength, &error))
	{
		XFDASHBOARD_DEBUG(_xfdashboard_application_tracker, APPLICATIONS,
							"Could not read environment varibles for PID %d at %s: %s",
							inPID,
							procEnvFile,
							error ? error->message : _("Unknown error"));

		/* Release allocated resources */
		if(error) g_error_free(error);
		if(procEnvFile) g_free(procEnvFile);
		if(envContent) g_free(envContent);
		if(environments) g_hash_table_destroy(environments);

		/* Return NULL result */
		return(NULL);
	}

	XFDASHBOARD_DEBUG(_xfdashboard_application_tracker, APPLICATIONS,
						"environment set for PID %d at %s is %lu bytes long",
						inPID,
						procEnvFile,
						envLength);

	/* Iterate through environment variables and insert copy of environment
	 * variable's name as key and the copy of its value as value into hash-table.
	 */
	iter=envContent;
	while(envLength>0)
	{
		const gchar		*name;
		const gchar		*value;

		/* Skip NULL-termination */
		if(!*iter)
		{
			envLength--;
			iter++;
			continue;
		}

		/* Split string up to next NULL termination into environment
		 * name and value.
		 * Current position of iterator is begin of environment variable's
		 * name. Next step is to iterate further until the first '=' is found
		 * which is the seperator for name and value of environment. This
		 * character is replace with NULL to mark end of name so it can be
		 * simply copied with g_strdup() later. After this is done iterate
		 * through string up to NULL termination or end of file but the
		 * first character iterated marks begin of value.
		 */
		name=iter;

		while(*iter && *iter!='=' && envLength>0)
		{
			iter++;
			envLength--;
		}
		if(*iter!='=')
		{
			g_warning(_("Malformed environment '%s' in environment set for PID %d at %s"),
						name,
						inPID,
						procEnvFile);

			/* Release allocated resources */
			if(procEnvFile) g_free(procEnvFile);
			if(envContent) g_free(envContent);
			if(environments) g_hash_table_destroy(environments);

			/* Return NULL result */
			return(NULL);
		}
		*iter=0;

		value=NULL;
		do
		{
			iter++;
			envLength--;
			if(!value) value=iter;
		}
		while(*iter && envLength>0);

		/* Insert environment name and value into hash-table but fail on
		 * duplicate key as a environment variable's name should not be
		 * found twice.
		 */
		if(g_hash_table_lookup(environments, name))
		{
			g_warning(_("Unexpected duplicate name '%s' in environment set for PID %d at %s: %s = %s"),
						name,
						inPID,
						procEnvFile,
						name,
						value);

			/* Release allocated resources */
			if(procEnvFile) g_free(procEnvFile);
			if(envContent) g_free(envContent);
			if(environments) g_hash_table_destroy(environments);

			/* Return NULL result */
			return(NULL);
		}

		g_hash_table_insert(environments, g_strdup(name), g_strdup(value));
	}

	/* Release allocated resources */
	if(procEnvFile) g_free(procEnvFile);
	if(envContent) g_free(envContent);

	/* Return hash-table with environment variables found */
	return(environments);
}
#else
/* Fallback funtion to get process' environment set from requested PID
 * when running at an unsupported system. It just simply returns NULL.
 */
static GHashTable* _xfdashboard_application_tracker_get_environment_from_pid(gint inPID)
{
	static gboolean		wasWarningPrinted=FALSE;

	if(!wasWarningPrinted)
	{
		g_warning("Determination of application by checking environment variables is not supported at this system.");
		wasWarningPrinted=TRUE;
	}

	/* Return NULL to not check environment by callee */
	return(NULL);
}
#endif

/* Get desktop ID from process' environment which owns window.
 * Callee is responsible to free result with g_object_unref().
 */
static GAppInfo* _xfdashboard_application_tracker_get_desktop_id_from_environment(XfdashboardApplicationTracker *self,
																					XfdashboardWindowTrackerWindow *inWindow)
{
	XfdashboardApplicationTrackerPrivate	*priv;
	GAppInfo								*foundAppInfo;
	gint									windowPID;
	GHashTable								*environments;
	gchar									*value;
	gint									checkPID;

	g_return_val_if_fail(XFDASHBOARD_IS_APPLICATION_TRACKER(self), NULL);
	g_return_val_if_fail(XFDASHBOARD_IS_WINDOW_TRACKER_WINDOW(inWindow), NULL);

	priv=self->priv;
	foundAppInfo=NULL;

	/* Get process ID running this window */
	windowPID=xfdashboard_window_tracker_window_get_pid(inWindow);
	if(windowPID<=0)
	{
		XFDASHBOARD_DEBUG(self, APPLICATIONS,
							"Could not get PID for window '%s' of a running application to parse environment variables",
							xfdashboard_window_tracker_window_get_name(inWindow));

		/* Return NULL result */
		return(NULL);
	}

	/* Get hash-table with environment variables found for window's PID */
	environments=_xfdashboard_application_tracker_get_environment_from_pid(windowPID);
	if(!environments)
	{
		XFDASHBOARD_DEBUG(self, APPLICATIONS,
							"Could not get environments for PID %d of windows '%s'",
							windowPID,
							xfdashboard_window_tracker_window_get_name(inWindow));

		/* Return NULL result */
		return(NULL);
	}

	/* Check that environment variable GIO_LAUNCHED_DESKTOP_FILE_PID exists.
	 * Also check that the PID in value matches the requested window's PID
	 * as the process may inherit the environments of its parent process
	 * but then this one is not the initial process for this application.
	 */
	if(!g_hash_table_lookup_extended(environments, "GIO_LAUNCHED_DESKTOP_FILE_PID", NULL, (gpointer)&value))
	{
		XFDASHBOARD_DEBUG(self, APPLICATIONS,
							"Missing 'GIO_LAUNCHED_DESKTOP_FILE_PID' in environment variables for PID %d of windows '%s'",
							windowPID,
							xfdashboard_window_tracker_window_get_name(inWindow));

		/* Release allocated resources */
		if(environments) g_hash_table_destroy(environments);

		/* Return NULL result */
		return(NULL);
	}

	checkPID=atoi(value);
	if(checkPID!=windowPID)
	{
		XFDASHBOARD_DEBUG(self, APPLICATIONS,
							"PID %d of environment variables does not match requested window PID %d for '%s'",
							checkPID,
							windowPID,
							xfdashboard_window_tracker_window_get_name(inWindow));

		/* Release allocated resources */
		if(environments) g_hash_table_destroy(environments);

		/* Return NULL result */
		return(NULL);
	}

	/* Check that environment variable GIO_LAUNCHED_DESKTOP_FILE exists and
	 * lookup application from full path as set in environment's value.
	 */
	if(!g_hash_table_lookup_extended(environments, "GIO_LAUNCHED_DESKTOP_FILE", NULL, (gpointer)&value))
	{
		XFDASHBOARD_DEBUG(self, APPLICATIONS,
							"Missing 'GIO_LAUNCHED_DESKTOP_FILE' in environment variables for PID %d of windows '%s'",
							windowPID,
							xfdashboard_window_tracker_window_get_name(inWindow));

		/* Release allocated resources */
		if(environments) g_hash_table_destroy(environments);

		/* Return NULL result */
		return(NULL);
	}

	foundAppInfo=xfdashboard_application_database_lookup_desktop_id(priv->appDatabase, value);
	if(!foundAppInfo)
	{
		/* Lookup application from basename of path */
		value=g_strrstr(value, G_DIR_SEPARATOR_S);
		if(value)
		{
			value++;
			foundAppInfo=xfdashboard_application_database_lookup_desktop_id(priv->appDatabase, value);
		}
	}

	/* Release allocated resources */
	if(environments) g_hash_table_destroy(environments);

	/* Return found application info which may be NULL if not found in
	 * application database.
	 */
	XFDASHBOARD_DEBUG(self, APPLICATIONS,
						"Resolved environment variables of window '%s' to desktop ID '%s'",
						xfdashboard_window_tracker_window_get_name(inWindow),
						foundAppInfo ? g_app_info_get_id(foundAppInfo) : "<nil>");

	return(foundAppInfo);
}

/* Get desktop ID from window names.
 * Callee is responsible to free result with g_object_unref().
 */
static GAppInfo* _xfdashboard_application_tracker_get_desktop_id_from_window_names(XfdashboardApplicationTracker *self,
																					XfdashboardWindowTrackerWindow *inWindow)
{
	XfdashboardApplicationTrackerPrivate	*priv;
	GAppInfo								*foundAppInfo;
	gchar									**names;
	gchar									**iter;
	GList									*apps;

	g_return_val_if_fail(XFDASHBOARD_IS_APPLICATION_TRACKER(self), NULL);
	g_return_val_if_fail(XFDASHBOARD_IS_WINDOW_TRACKER_WINDOW(inWindow), NULL);

	priv=self->priv;
	foundAppInfo=NULL;

	/* Get list of applications */
	apps=xfdashboard_application_database_get_all_applications(priv->appDatabase);

	/* Get window's names */
	names=xfdashboard_window_tracker_window_get_instance_names(inWindow);

	/* Iterate through window's names to try to find matching desktop file */
	iter=names;
	while(iter && *iter)
	{
		GAppInfo							*appInfo;
		gchar								*iterName;
		gchar								*iterNameLowerCase;

		/* Build desktop ID from iterated name */
		if(!g_str_has_suffix(*iter, ".desktop")) iterName=g_strconcat(*iter, ".desktop", NULL);
			else iterName=g_strdup(*iter);

		iterNameLowerCase=g_utf8_strdown(iterName, -1);

		/* Lookup application from unmodified name */
		appInfo=xfdashboard_application_database_lookup_desktop_id(priv->appDatabase, iterName);

		/* Lookup application from to-lower-case converted name if previous
		 * lookup with unmodified name failed.
		 */
		if(!appInfo)
		{
			/* Lookup application from lower-case name */
			appInfo=xfdashboard_application_database_lookup_desktop_id(priv->appDatabase, iterNameLowerCase);
		}

		/* If no application was found for the name it may be an application
		 * located in a subdirectory. Then the desktop ID is prefixed with
		 * the subdirectory's name followed by a dash. So iterate through all
		 * applications and lookup with glob pattern '*-' followed by name
		 * and suffix '.desktop'.
		 */
		if(!appInfo)
		{
			GList							*iterApps;
			GList							*foundSubdirApps;
			gchar							*globName;
			GPatternSpec					*globPattern;
			GAppInfo						*globAppInfo;

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

				if(g_pattern_match_string(globPattern, g_app_info_get_id(globAppInfo)))
				{
					foundSubdirApps=g_list_prepend(foundSubdirApps, globAppInfo);
					XFDASHBOARD_DEBUG(self, APPLICATIONS,
										"Found possible application '%s' for window '%s' using pattern '%s'",
										g_app_info_get_id(globAppInfo),
										xfdashboard_window_tracker_window_get_name(inWindow),
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
									xfdashboard_window_tracker_window_get_name(inWindow),
									globName);
			}

			/* Release allocated resources */
			if(foundSubdirApps) g_list_free(foundSubdirApps);
			if(globPattern) g_pattern_spec_free(globPattern);
			if(globName) g_free(globName);
		}

		/* If we still did not find an application continue with next
		 * name in list.
		 */
		if(!appInfo)
		{
			/* Release allocated resources */
			if(iterName) g_free(iterName);
			if(iterNameLowerCase) g_free(iterNameLowerCase);

			/* Continue with next name in list */
			iter++;
			continue;
		}

		/* Check if found application info matches previous one. If it does not match
		 * the desktop IDs found previously are ambigous, so return NULL result.
		 */
		if(foundAppInfo &&
			!g_app_info_equal(foundAppInfo, appInfo))
		{
			XFDASHBOARD_DEBUG(self, APPLICATIONS,
								"Resolved window names of '%s' are ambiguous - discarding desktop IDs '%s' and '%s'",
								xfdashboard_window_tracker_window_get_name(inWindow),
								g_app_info_get_id(foundAppInfo),
								g_app_info_get_id(appInfo));

			/* Release allocated resources */
			if(iterName) g_free(iterName);
			if(iterNameLowerCase) g_free(iterNameLowerCase);
			if(foundAppInfo) g_object_unref(foundAppInfo);
			if(appInfo) g_object_unref(appInfo);
			if(names) g_strfreev(names);
			if(apps) g_list_free_full(apps, g_object_unref);

			return(NULL);
		}

		/* If it is the first application info found, remember it */
		if(!foundAppInfo) foundAppInfo=g_object_ref(appInfo);

		/* Release allocated resources */
		if(iterName) g_free(iterName);
		if(iterNameLowerCase) g_free(iterNameLowerCase);
		if(appInfo) g_object_unref(appInfo);

		/* Continue with next name in list */
		iter++;
	}

	/* Release allocated resources */
	if(names) g_strfreev(names);
	if(apps) g_list_free_full(apps, g_object_unref);

	/* Return found application info */
	XFDASHBOARD_DEBUG(self, APPLICATIONS,
						"Resolved window names of '%s' to desktop ID '%s'",
						xfdashboard_window_tracker_window_get_name(inWindow),
						foundAppInfo ? g_app_info_get_id(foundAppInfo) : "<nil>");

	return(foundAppInfo);
}

/* A window was created */
static void _xfdashboard_application_tracker_on_window_opened(XfdashboardApplicationTracker *self,
																XfdashboardWindowTrackerWindow *inWindow,
																gpointer inUserData)
{
	XfdashboardApplicationTrackerPrivate	*priv;
	GAppInfo								*appInfo;
	XfdashboardApplicationTrackerItem		*item;
	XfdashboardWindowTrackerWindowState		state;

	g_return_if_fail(XFDASHBOARD_IS_APPLICATION_TRACKER(self));
	g_return_if_fail(XFDASHBOARD_IS_WINDOW_TRACKER_WINDOW(inWindow));

	priv=self->priv;
	appInfo=NULL;

	/* Get window state */
	state=xfdashboard_window_tracker_window_get_state(inWindow);

	/* Check if window is "visible" and we should try to find the application
	 * it belongs to. To be "visible" means here that the window should not be
	 * skipped in any tasklist or pager. But hidden or minimized window are
	 * "visible" when looking up running application is meant.
	 */
	if(state & XFDASHBOARD_WINDOW_TRACKER_WINDOW_STATE_SKIP_PAGER)
	{
		XFDASHBOARD_DEBUG(self, APPLICATIONS,
							"Do not resolve window '%s' as it has skip-pager set.",
							xfdashboard_window_tracker_window_get_name(inWindow));

		return;
	}

	if(state & XFDASHBOARD_WINDOW_TRACKER_WINDOW_STATE_SKIP_TASKLIST)
	{
		XFDASHBOARD_DEBUG(self, APPLICATIONS,
							"Do not resolve window '%s' as it has skip-tasklist set.",
							xfdashboard_window_tracker_window_get_name(inWindow));

		return;
	}

	/* Try to find application for window */
	appInfo=_xfdashboard_application_tracker_get_desktop_id_from_environment(self, inWindow);
	if(!appInfo) appInfo=_xfdashboard_application_tracker_get_desktop_id_from_window_names(self, inWindow);

	/* If we could not resolve window to a desktop application info, then stop here */
	if(!appInfo)
	{
		XFDASHBOARD_DEBUG(self, APPLICATIONS,
							"Could not resolve window '%s' to any desktop ID",
							xfdashboard_window_tracker_window_get_name(inWindow));

		return;
	}

	XFDASHBOARD_DEBUG(self, APPLICATIONS,
						"Window '%s' belongs to desktop ID '%s'",
						xfdashboard_window_tracker_window_get_name(inWindow),
						g_app_info_get_id(appInfo));

	/* Create application tracker if no one exists for application and window ... */
	item= _xfdashboard_application_tracker_find_item_by_app_info(self, appInfo);
	if(!item)
	{
		/* Create application tracker item */
		item=_xfdashboard_application_tracker_item_new(appInfo, inWindow);

		/* Add new item to list of running applications */
		priv->runningApps=g_list_prepend(priv->runningApps, item);

		/* Emit signal as this application is known to be running now */
		XFDASHBOARD_DEBUG(self, APPLICATIONS,
							"Emitting signal 'state-changed' to running for desktop ID '%s'",
							item->desktopID);
		g_signal_emit(self, XfdashboardApplicationTrackerSignals[SIGNAL_STATE_CHANGED], g_quark_from_string(item->desktopID), item->desktopID, TRUE);
	}
		/* ... otherwise add window to existing one */
		else
		{
			/* Add window to list of window for known running application */
			_xfdashboard_application_tracker_item_add_window(item, inWindow);
		}

	/* Release allocated resources */
	if(appInfo) g_object_unref(appInfo);
}

/* A window was closed */
static void _xfdashboard_application_tracker_on_window_closed(XfdashboardApplicationTracker *self,
																XfdashboardWindowTrackerWindow *inWindow,
																gpointer inUserData)
{
	XfdashboardApplicationTrackerPrivate	*priv;
	XfdashboardApplicationTrackerItem		*item;

	g_return_if_fail(XFDASHBOARD_IS_APPLICATION_TRACKER(self));
	g_return_if_fail(XFDASHBOARD_IS_WINDOW_TRACKER_WINDOW(inWindow));

	priv=self->priv;

	/* Find application tracker item in list of known running applications
	 * matching the window just closed.
	 */
	item= _xfdashboard_application_tracker_find_item_by_window(self, inWindow);
	if(!item)
	{
		XFDASHBOARD_DEBUG(self, APPLICATIONS,
							"Could not find running application for window '%s'",
							xfdashboard_window_tracker_window_get_name(inWindow));

		return;
	}

	XFDASHBOARD_DEBUG(self, APPLICATIONS,
						"Closing window '%s' for desktop ID '%s'",
						xfdashboard_window_tracker_window_get_name(inWindow),
						item->desktopID);

	/* Remove window from found application tracker item */
	_xfdashboard_application_tracker_item_remove_window(item, inWindow);

	/* If it was the last window then application is not running anymore
	 * because it has no window.
	 */
	if(!item->windows ||
		g_list_length(item->windows)==0)
	{
		gchar								*desktopID;

		XFDASHBOARD_DEBUG(self, APPLICATIONS,
							"Closing window '%s' for desktop ID '%s' closed last window so remove application from list of running ones",
							xfdashboard_window_tracker_window_get_name(inWindow),
							item->desktopID);

		/* Create a copy of desktop ID for signal emission because the
		 * application tracker item will be removed and freed before.
		 */
		desktopID=g_strdup(item->desktopID);

		/* Remove application tracker item from list of running applications
		 * and free it.
		 */
		priv->runningApps=g_list_remove(priv->runningApps, item);
		_xfdashboard_application_tracker_item_free(item);

		/* Emit signal as this application is not running anymore */
		XFDASHBOARD_DEBUG(self, APPLICATIONS,
							"Emitting signal 'state-changed' to stopped for desktop ID '%s'",
							desktopID);
		g_signal_emit(self, XfdashboardApplicationTrackerSignals[SIGNAL_STATE_CHANGED], g_quark_from_string(desktopID), desktopID, FALSE);

		/* Release allocated resources */
		g_free(desktopID);
	}
}

/* The active window has changed */
static void _xfdashboard_application_tracker_on_active_window_changed(XfdashboardApplicationTracker *self,
																		XfdashboardWindowTrackerWindow *inOldActiveWindow,
																		XfdashboardWindowTrackerWindow *inNewActiveWindow,
																		gpointer inUserData)
{
	XfdashboardApplicationTrackerItem		*item;

	g_return_if_fail(XFDASHBOARD_IS_APPLICATION_TRACKER(self));
	g_return_if_fail(!inNewActiveWindow || XFDASHBOARD_IS_WINDOW_TRACKER_WINDOW(inNewActiveWindow));

	/* New active window might be NULL (why ever?) so return immediately
	 * in this case.
	 */
	if(!inNewActiveWindow)
	{
		XFDASHBOARD_DEBUG(self, APPLICATIONS, "No new active window to check for running application.");
		return;
	}

	/* Find application tracker item in list of known running applications
	 * matching the new active window.
	 */
	item=_xfdashboard_application_tracker_find_item_by_window(self, inNewActiveWindow);
	if(!item)
	{
		XFDASHBOARD_DEBUG(self, APPLICATIONS,
							"Could not find running application for new active window '%s'",
							xfdashboard_window_tracker_window_get_name(inNewActiveWindow));

		return;
	}

	XFDASHBOARD_DEBUG(self, APPLICATIONS,
						"New active window is '%s' and belongs to desktop ID '%s'",
						xfdashboard_window_tracker_window_get_name(inNewActiveWindow),
						item->desktopID);

	/* Move new active window of found application tracker item to begin of
	 * list of windows by removing window from list and prepending it to list
	 * again. This will sort the list of window in order of last activation.
	 * That means the first window in list was the last active one for this
	 * application.
	 */
	item->windows=g_list_remove(item->windows, inNewActiveWindow);
	item->windows=g_list_prepend(item->windows, inNewActiveWindow);
}


/* IMPLEMENTATION: GObject */

/* Construct this object */
static GObject* _xfdashboard_application_tracker_constructor(GType inType,
																guint inNumberConstructParams,
																GObjectConstructParam *inConstructParams)
{
	GObject									*object;

	if(!_xfdashboard_application_tracker)
	{
		object=G_OBJECT_CLASS(xfdashboard_application_tracker_parent_class)->constructor(inType, inNumberConstructParams, inConstructParams);
		_xfdashboard_application_tracker=XFDASHBOARD_APPLICATION_TRACKER(object);
	}
		else
		{
			object=g_object_ref(G_OBJECT(_xfdashboard_application_tracker));
		}

	return(object);
}

/* Dispose this object */
static void _xfdashboard_application_tracker_dispose(GObject *inObject)
{
	XfdashboardApplicationTracker							*self=XFDASHBOARD_APPLICATION_TRACKER(inObject);
	XfdashboardApplicationTrackerPrivate					*priv=self->priv;

	/* Release allocated resources */
	if(priv->runningApps)
	{
		g_list_free_full(priv->runningApps, (GDestroyNotify)_xfdashboard_application_tracker_item_free);
		priv->runningApps=NULL;
	}

	if(priv->windowTracker)
	{
		g_signal_handlers_disconnect_by_data(priv->windowTracker, self);
		g_object_unref(priv->windowTracker);
		priv->windowTracker=NULL;
	}

	if(priv->appDatabase)
	{
		g_object_unref(priv->appDatabase);
		priv->appDatabase=NULL;
	}

	/* Call parent's class dispose method */
	G_OBJECT_CLASS(xfdashboard_application_tracker_parent_class)->dispose(inObject);
}

/* Finalize this object */
static void _xfdashboard_application_tracker_finalize(GObject *inObject)
{
	/* Release allocated resources finally, e.g. unset singleton */
	if(G_LIKELY(G_OBJECT(_xfdashboard_application_tracker)==inObject))
	{
		_xfdashboard_application_tracker=NULL;
	}

	/* Call parent's class dispose method */
	G_OBJECT_CLASS(xfdashboard_application_tracker_parent_class)->finalize(inObject);
}

/* Class initialization
 * Override functions in parent classes and define properties
 * and signals
 */
static void xfdashboard_application_tracker_class_init(XfdashboardApplicationTrackerClass *klass)
{
	GObjectClass		*gobjectClass=G_OBJECT_CLASS(klass);

	/* Override functions */
	gobjectClass->constructor=_xfdashboard_application_tracker_constructor;
	gobjectClass->dispose=_xfdashboard_application_tracker_dispose;
	gobjectClass->finalize=_xfdashboard_application_tracker_finalize;

	/* Define signals */
	XfdashboardApplicationTrackerSignals[SIGNAL_STATE_CHANGED]=
		g_signal_new("state-changed",
						G_TYPE_FROM_CLASS(klass),
						G_SIGNAL_RUN_LAST | G_SIGNAL_NO_HOOKS | G_SIGNAL_DETAILED,
						G_STRUCT_OFFSET(XfdashboardApplicationTrackerClass, state_changed),
						NULL,
						NULL,
						_xfdashboard_marshal_VOID__STRING_BOOLEAN,
						G_TYPE_NONE,
						2,
						G_TYPE_STRING,
						G_TYPE_BOOLEAN);
}

/* Object initialization
 * Create private structure and set up default values
 */
static void xfdashboard_application_tracker_init(XfdashboardApplicationTracker *self)
{
	XfdashboardApplicationTrackerPrivate	*priv;

	priv=self->priv=xfdashboard_application_tracker_get_instance_private(self);

	/* Set default values */
	priv->runningApps=NULL;
	priv->appDatabase=xfdashboard_application_database_get_default();
	priv->windowTracker=xfdashboard_window_tracker_get_default();

	/* Load application database if not done already */
	if(!xfdashboard_application_database_is_loaded(priv->appDatabase))
	{
		g_warning(_("Application database was not initialized. Application tracking might not working."));
	}

	/* Connect signals */
	g_signal_connect_swapped(priv->windowTracker,
								"window-opened",
								G_CALLBACK(_xfdashboard_application_tracker_on_window_opened),
								self);
	g_signal_connect_swapped(priv->windowTracker,
								"window-closed",
								G_CALLBACK(_xfdashboard_application_tracker_on_window_closed),
								self);
	g_signal_connect_swapped(priv->windowTracker,
								"active-window-changed",
								G_CALLBACK(_xfdashboard_application_tracker_on_active_window_changed),
								self);
}

/* IMPLEMENTATION: Public API */

/* Get single instance of application */
XfdashboardApplicationTracker* xfdashboard_application_tracker_get_default(void)
{
	GObject									*singleton;

	singleton=g_object_new(XFDASHBOARD_TYPE_APPLICATION_TRACKER, NULL);
	return(XFDASHBOARD_APPLICATION_TRACKER(singleton));
}

/* Get running state of application */
gboolean xfdashboard_application_tracker_is_running_by_desktop_id(XfdashboardApplicationTracker *self,
																	const gchar *inDesktopID)
{
	g_return_val_if_fail(XFDASHBOARD_IS_APPLICATION_TRACKER(self), FALSE);
	g_return_val_if_fail(inDesktopID && *inDesktopID, FALSE);

	return(_xfdashboard_application_tracker_find_item_by_desktop_id(self, inDesktopID) ? TRUE : FALSE);
}

gboolean xfdashboard_application_tracker_is_running_by_app_info(XfdashboardApplicationTracker *self,
																GAppInfo *inAppInfo)
{
	g_return_val_if_fail(XFDASHBOARD_IS_APPLICATION_TRACKER(self), FALSE);
	g_return_val_if_fail(G_IS_APP_INFO(inAppInfo), FALSE);

	return(_xfdashboard_application_tracker_find_item_by_app_info(self, inAppInfo) ? TRUE : FALSE);
}

/* Get window list (sorted by last activation time) for an application.
 * The returned GList is owned by application and should not be modified or freed.
 */
const GList* xfdashboard_application_tracker_get_window_list_by_desktop_id(XfdashboardApplicationTracker *self,
																			const gchar *inDesktopID)
{
	XfdashboardApplicationTrackerItem	*item;

	g_return_val_if_fail(XFDASHBOARD_IS_APPLICATION_TRACKER(self), NULL);
	g_return_val_if_fail(inDesktopID && *inDesktopID, NULL);

	/* Get application tracker item for requested desktop ID */
	item=_xfdashboard_application_tracker_find_item_by_desktop_id(self, inDesktopID);
	if(!item) return(NULL);

	/* Return list of windows for found application tracker item */
	return(item->windows);
}

const GList*  xfdashboard_application_tracker_get_window_list_by_app_info(XfdashboardApplicationTracker *self,
																			GAppInfo *inAppInfo)
{
	XfdashboardApplicationTrackerItem	*item;

	g_return_val_if_fail(XFDASHBOARD_IS_APPLICATION_TRACKER(self), NULL);
	g_return_val_if_fail(G_IS_APP_INFO(inAppInfo), NULL);

	/* Get application tracker item for requested desktop ID */
	item=_xfdashboard_application_tracker_find_item_by_app_info(self, inAppInfo);
	if(!item) return(NULL);

	/* Return list of windows for found application tracker item */
	return(item->windows);
}
