/*
 * application-tracker: A singleton managing states of applications
 * 
 * Copyright 2012-2015 Stephan Haller <nomad@froevel.de>
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

#include "application-tracker.h"

#include "application-database.h"
#include "window-tracker.h"
#include "marshal.h"

/* Define this class in GObject system */
G_DEFINE_TYPE(XfdashboardApplicationTracker,
				xfdashboard_application_tracker,
				G_TYPE_OBJECT)

/* Private structure - access only by public API if needed */
#define XFDASHBOARD_APPLICATION_TRACKER_GET_PRIVATE(obj) \
	(G_TYPE_INSTANCE_GET_PRIVATE((obj), XFDASHBOARD_TYPE_APPLICATION_TRACKER, XfdashboardApplicationTrackerPrivate))

struct _XfdashboardApplicationTrackerPrivate
{
	/* Instance related */
	GList							*runningApps;

	XfdashboardApplicationDatabase	*appDatabase;
	XfdashboardWindowTracker		*windowTracker;
};

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
	 * tracker item, so add it to this list.
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

/* Get desktop ID from process' environment which owns window.
 * Callee is responsible to free result with g_object_unref().
 */
static GAppInfo* _xfdashboard_application_tracker_get_desktop_id_from_environment(XfdashboardApplicationTracker *self,
																					XfdashboardWindowTrackerWindow *inWindow)
{
	XfdashboardApplicationTrackerPrivate	*priv;
	GAppInfo								*foundAppInfo;
	gint									windowPID;
	gchar									*procEnvFile;
	gchar									*envContent;
	gsize									envLength;
	GError									*error;
	gchar									*iter;
	const gchar								*gioLaunchedPID;
	const gchar								*gioLaunchedDesktopFile;

	g_return_val_if_fail(XFDASHBOARD_IS_APPLICATION_TRACKER(self), NULL);
	g_return_val_if_fail(XFDASHBOARD_IS_WINDOW_TRACKER_WINDOW(inWindow), NULL);

	priv=self->priv;
	foundAppInfo=NULL;
	error=NULL;

	/* Get process ID running this window */
	windowPID=xfdashboard_window_tracker_window_get_pid(inWindow);
	if(windowPID<=0)
	{
		g_message("%s: Could not get PID running window '%s' to parse environment variables",
				__func__,
				xfdashboard_window_tracker_window_get_title(inWindow));

		return(NULL);
	}

	/* Open enviroment variables of process.
	 * This is the initial set of environment variables set when process was spawned.
	 * But that is ok because the environment variables we lookup are set
	 * at launch time and do not change.
	 */
	envContent=NULL;
	envLength=0;

	procEnvFile=g_strdup_printf("/proc/%d/environ", windowPID);
	if(!g_file_get_contents(procEnvFile, &envContent, &envLength, &error))
	{
		g_message("%s: Could read in enviroment varibles for PID %d of window '%s' at %s: %s",
					__func__,
					windowPID,
					xfdashboard_window_tracker_window_get_title(inWindow),
					procEnvFile,
					error ? error->message : _("Unknown error"));

		/* Release allocated resources */
		if(error) g_error_free(error);
		if(procEnvFile) g_free(procEnvFile);
		if(envContent) g_free(envContent);

		/* Return NULL result */
		return(NULL);
	}

	g_message("%s: Enviroment set at %s is %lu bytes long for window '%s'",
				__func__,
				procEnvFile,
				envLength,
				xfdashboard_window_tracker_window_get_title(inWindow));

	/* Iterate through enviroment variables and lookup GIO_LAUNCHED_DESKTOP_FILE
	 * and GIO_LAUNCHED_DESKTOP_FILE_PID.
	 */
	gioLaunchedPID=NULL;
	gioLaunchedDesktopFile=NULL;

	iter=envContent;
	while(envLength>0)
	{
		gsize								len;

		/* Skip NULL-termination */
		if(!*iter)
		{
			envLength--;
			iter++;
			continue;
		}

		/* Check current iterated enviroment variable matches a requested one */
		if(g_str_has_prefix(iter, "GIO_LAUNCHED_DESKTOP_FILE="))
		{
			/* Each enviroment variable should be listed only once
			 * otherwise is an error.
			 */
			if(gioLaunchedDesktopFile)
			{
				g_message("%s: Could parse in enviroment varibles for PID %d of window '%s' at %s because GIO_LAUNCHED_DESKTOP_FILE exists more than once",
							__func__,
							windowPID,
							xfdashboard_window_tracker_window_get_title(inWindow),
							procEnvFile);

				/* Release allocated resources */
				if(foundAppInfo) g_object_unref(foundAppInfo);
				if(procEnvFile) g_free(procEnvFile);
				if(envContent) g_free(envContent);

				/* Return NULL result */
				return(NULL);
			}

			/* Remember value of environment variable */
			gioLaunchedDesktopFile=iter;
		}
			else if(g_str_has_prefix(iter, "GIO_LAUNCHED_DESKTOP_FILE_PID="))
			{
				/* Each enviroment variable should be listed only once
				 * otherwise is an error.
				 */
				if(gioLaunchedPID)
				{
					g_message("%s: Could parse in enviroment varibles for PID %d of window '%s' at %s because GIO_LAUNCHED_DESKTOP_FILE_PID exists more than once",
								__func__,
								windowPID,
								xfdashboard_window_tracker_window_get_title(inWindow),
								procEnvFile);

					/* Release allocated resources */
					if(foundAppInfo) g_object_unref(foundAppInfo);
					if(procEnvFile) g_free(procEnvFile);
					if(envContent) g_free(envContent);

					/* Return NULL result */
					return(NULL);
				}

				/* Remember value of environment variable */
				gioLaunchedPID=iter;
			}

		/* If all requested environment variable has been found stop iterating */
		if(gioLaunchedPID && gioLaunchedDesktopFile) break;

		/* Continue with next environment variable */
		len=strlen(iter);
		iter+=len;
		envLength-=len;
	}

	/* If all requested environment variable has been found then check if
	 * GIO_LAUNCHED_DESKTOP_FILE_PID matches window owner's process ID.
	 */
	if(gioLaunchedPID && gioLaunchedDesktopFile)
	{
		/* Move pointer of environment variables to value */
		while(*gioLaunchedPID && *gioLaunchedPID!='=') gioLaunchedPID++;
		while(*gioLaunchedDesktopFile && *gioLaunchedDesktopFile!='=') gioLaunchedDesktopFile++;

		/* Check if pointers points to value assignment character */
		if(*gioLaunchedPID=='=' &&
			*gioLaunchedDesktopFile=='=')
		{
			gint							checkPID;

			/* Move pointer one byte further where value begins really */
			gioLaunchedPID++;
			gioLaunchedDesktopFile++;

			/* Check if PID of enviroment variable matches window owner's
			 * process ID.
			 */
			checkPID=atoi(gioLaunchedPID);
			if(checkPID==windowPID)
			{
				/* Lookup application from full path */
				foundAppInfo=xfdashboard_application_database_lookup_desktop_id(priv->appDatabase, gioLaunchedDesktopFile);
				if(!foundAppInfo)
				{
					/* Lookup application from basename of path */
					gioLaunchedDesktopFile=g_strrstr(gioLaunchedDesktopFile, G_DIR_SEPARATOR_S);
					if(gioLaunchedDesktopFile)
					{
						gioLaunchedDesktopFile++;
						foundAppInfo=xfdashboard_application_database_lookup_desktop_id(priv->appDatabase, gioLaunchedDesktopFile);
					}
				}
			}
				else
				{
					g_message("%s: PID %d of environment variable does not match window PID %d for '%s'",
								__func__,
								checkPID,
								windowPID,
								xfdashboard_window_tracker_window_get_title(inWindow));
				}
		}
	}

	/* Release allocated resources */
	if(procEnvFile) g_free(procEnvFile);
	if(envContent) g_free(envContent);

	/* Return found application info */
	g_message("%s: Resolved enviroment variables of window '%s' to desktop ID '%s'",
				__func__,
				xfdashboard_window_tracker_window_get_title(inWindow),
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

	g_return_val_if_fail(XFDASHBOARD_IS_APPLICATION_TRACKER(self), NULL);
	g_return_val_if_fail(XFDASHBOARD_IS_WINDOW_TRACKER_WINDOW(inWindow), NULL);

	priv=self->priv;
	foundAppInfo=NULL;

	/* Get window's names */
	names=xfdashboard_window_tracker_window_get_instance_names(inWindow);

	/* Iterate through window's names to try to find matching desktop file */
	iter=names;
	while(iter && *iter)
	{
		GAppInfo							*appInfo;

		/* Lookup application from unmodified name */
		if(g_str_has_suffix(*iter, ".desktop"))
		{
			appInfo=xfdashboard_application_database_lookup_desktop_id(priv->appDatabase, *iter);
		}
			else
			{
				gchar						*desktopID;

				desktopID=g_strconcat(*iter, ".desktop", NULL);
				appInfo=xfdashboard_application_database_lookup_desktop_id(priv->appDatabase, desktopID);
				g_free(desktopID);
			}

		/* Lookup application from to-lower-case converted name if previous
		 * lookup with unmodified name failed.
		 */
		if(!appInfo)
		{
			gchar							*lowerDesktopID;

			/* Convert name to lower case */
			lowerDesktopID=g_utf8_strdown(*iter, -1);

			/* Lookup application from lower-case name. No need to add '.desktop'
			 * as suffix as it was done before.
			 */
			if(g_str_has_suffix(lowerDesktopID, ".desktop"))
			{
				appInfo=xfdashboard_application_database_lookup_desktop_id(priv->appDatabase, lowerDesktopID);
			}
				else
				{
					gchar					*desktopID;

					desktopID=g_strconcat(lowerDesktopID, ".desktop", NULL);
					appInfo=xfdashboard_application_database_lookup_desktop_id(priv->appDatabase, desktopID);
					g_free(desktopID);
				}

			/* Free lower case of name */
			g_free(lowerDesktopID);

			/* If we still did not find an application continue with next
			 * name in list.
			 */
			if(!appInfo)
			{
				/* Continue with next name in list */
				iter++;
				continue;
			}
		}

		/* Check if found application info matches previous one. If it does not match
		 * the desktop IDs found previously are ambigous, so return NULL result.
		 */
		if(foundAppInfo &&
			!g_app_info_equal(foundAppInfo, appInfo))
		{
			g_message("%s: Resolved window names of '%s' are ambiguous - discarding desktop IDs '%s' and '%s'",
						__func__,
						xfdashboard_window_tracker_window_get_title(inWindow),
						g_app_info_get_id(foundAppInfo),
						g_app_info_get_id(appInfo));

			/* Release allocated resources */
			if(foundAppInfo) g_object_unref(foundAppInfo);
			if(appInfo) g_object_unref(appInfo);
			if(names) g_strfreev(names);

			return(NULL);
		}

		/* If it is the first application info found, remember it */
		if(!foundAppInfo) foundAppInfo=g_object_ref(appInfo);

		/* Release allocated resources */
		if(appInfo) g_object_unref(appInfo);

		/* Continue with next name in list */
		iter++;
	}

	/* Release allocated resources */
	if(names) g_strfreev(names);

	/* Return found application info */
	g_message("%s: Resolved window names of '%s' to desktop ID '%s'",
				__func__,
				xfdashboard_window_tracker_window_get_title(inWindow),
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

	g_return_if_fail(XFDASHBOARD_IS_APPLICATION_TRACKER(self));
	g_return_if_fail(XFDASHBOARD_IS_WINDOW_TRACKER_WINDOW(inWindow));

	priv=self->priv;
	appInfo=NULL;

	/* Try to find application for window */
	appInfo=_xfdashboard_application_tracker_get_desktop_id_from_environment(self, inWindow);
	if(!appInfo) appInfo=_xfdashboard_application_tracker_get_desktop_id_from_window_names(self, inWindow);

	/* If we could not resolve window to a desktop application info, then stop here */
	if(!appInfo)
	{
		g_message("%s: Could not resolve window '%s' to any desktop ID",
					__func__,
					xfdashboard_window_tracker_window_get_title(inWindow));

		return;
	}

	g_message("%s: Window '%s' belongs to desktop ID '%s'",
				__func__,
				xfdashboard_window_tracker_window_get_title(inWindow),
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
		g_message("%s: Could not find running application for window '%s'",
					__func__,
					xfdashboard_window_tracker_window_get_title(inWindow));

		return;
	}

	g_message("%s: Closing window '%s' for desktop ID '%s'",
				__func__,
				xfdashboard_window_tracker_window_get_title(inWindow),
				item->desktopID);

	/* Remove window from found application tracker item */
	_xfdashboard_application_tracker_item_remove_window(item, inWindow);

	/* If it was the last window then application is not running anymore
	 * because it has no window.
	 */
	if(!item->windows ||
		g_list_length(item->windows)==0)
	{
		g_message("%s: Closing window '%s' for desktop ID '%s' closed last window so remove application from list of running ones",
					__func__,
					xfdashboard_window_tracker_window_get_title(inWindow),
					item->desktopID);

		/* Emit signal as this application is not running anymore */
		g_signal_emit(self, XfdashboardApplicationTrackerSignals[SIGNAL_STATE_CHANGED], g_quark_from_string(item->desktopID), item->desktopID, FALSE);

		/* Remove application tracker item from list of running applications */
		priv->runningApps=g_list_remove(priv->runningApps, item);

		/* Release allocated resources */
		_xfdashboard_application_tracker_item_free(item);
	}
}

/* IMPLEMENTATION: GObject */

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

	/* Unset singleton */
	if(G_LIKELY(G_OBJECT(_xfdashboard_application_tracker)==inObject))
	{
		_xfdashboard_application_tracker=NULL;
	}

	/* Call parent's class dispose method */
	G_OBJECT_CLASS(xfdashboard_application_tracker_parent_class)->dispose(inObject);
}

/* Class initialization
 * Override functions in parent classes and define properties
 * and signals
 */
static void xfdashboard_application_tracker_class_init(XfdashboardApplicationTrackerClass *klass)
{
	GObjectClass		*gobjectClass=G_OBJECT_CLASS(klass);

	/* Override functions */
	gobjectClass->dispose=_xfdashboard_application_tracker_dispose;

	/* Set up private structure */
	g_type_class_add_private(klass, sizeof(XfdashboardApplicationTrackerPrivate));

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

	priv=self->priv=XFDASHBOARD_APPLICATION_TRACKER_GET_PRIVATE(self);

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
}

/* IMPLEMENTATION: Public API */

/* Get single instance of application */
XfdashboardApplicationTracker* xfdashboard_application_tracker_get_default(void)
{
	if(G_UNLIKELY(_xfdashboard_application_tracker==NULL))
	{
		_xfdashboard_application_tracker=g_object_new(XFDASHBOARD_TYPE_APPLICATION_TRACKER, NULL);
	}
		else g_object_ref(_xfdashboard_application_tracker);

	return(_xfdashboard_application_tracker);
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
