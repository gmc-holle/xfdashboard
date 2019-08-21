/*
 * applications-search-provider: Search provider for searching installed
 *                               applications
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

#include <libxfdashboard/applications-search-provider.h>

#include <glib/gi18n-lib.h>
#include <gtk/gtk.h>
#include <errno.h>

#include <libxfdashboard/application-database.h>
#include <libxfdashboard/application-button.h>
#include <libxfdashboard/application.h>
#include <libxfdashboard/drag-action.h>
#include <libxfdashboard/click-action.h>
#include <libxfdashboard/popup-menu.h>
#include <libxfdashboard/popup-menu-item-button.h>
#include <libxfdashboard/popup-menu-item-separator.h>
#include <libxfdashboard/application-tracker.h>
#include <libxfdashboard/utils.h>
#include <libxfdashboard/enums.h>
#include <libxfdashboard/compat.h>
#include <libxfdashboard/debug.h>


/* Define this class in GObject system */
struct _XfdashboardApplicationsSearchProviderPrivate
{
	/* Properties related */
	XfdashboardApplicationsSearchProviderSortMode	nextSortMode;

	/* Instance related */
	XfdashboardApplicationDatabase					*appDB;
	guint											applicationAddedID;
	guint											applicationRemovedID;

	GList											*allApps;

	XfconfChannel									*xfconfChannel;
	guint											xfconfSortModeBindingID;
	XfdashboardApplicationsSearchProviderSortMode	currentSortMode;
};

G_DEFINE_TYPE_WITH_PRIVATE(XfdashboardApplicationsSearchProvider,
							xfdashboard_applications_search_provider,
							XFDASHBOARD_TYPE_SEARCH_PROVIDER)

/* Properties */
enum
{
	PROP_0,

	PROP_SORT_MODE,

	PROP_LAST
};

static GParamSpec* XfdashboardApplicationsSearchProviderProperties[PROP_LAST]={ 0, };

/* IMPLEMENTATION: Private variables and methods */
#define SORT_MODE_XFCONF_PROP													"/components/applications-search-provider/sort-mode"

#define DEFAULT_DELIMITERS														"\t\n\r "

#define XFDASHBOARD_APPLICATIONS_SEARCH_PROVIDER_STATISTICS_FILE				"applications-search-provider-statistics.ini"
#define XFDASHBOARD_APPLICATIONS_SEARCH_PROVIDER_STATISTICS_ENTRIES_GROUP		"Entries"
#define XFDASHBOARD_APPLICATIONS_SEARCH_PROVIDER_STATISTICS_ENTRIES_COUNT		"Count"
#define XFDASHBOARD_APPLICATIONS_SEARCH_PROVIDER_STATISTICS_USED_COUNTER_GROUP	"Used Counters"

typedef struct _XfdashboardApplicationsSearchProviderGlobal			XfdashboardApplicationsSearchProviderGlobal;
struct _XfdashboardApplicationsSearchProviderGlobal
{
	gchar								*filename;

	GHashTable							*stats;

	guint								shutdownSignalID;
	guint								applicationLaunchedSignalID;

	guint								maxUsedCounter;
};

G_LOCK_DEFINE_STATIC(_xfdashboard_applications_search_provider_statistics_lock);
XfdashboardApplicationsSearchProviderGlobal		_xfdashboard_applications_search_provider_statistics={0, };

typedef struct _XfdashboardApplicationsSearchProviderStatistics		XfdashboardApplicationsSearchProviderStatistics;
struct _XfdashboardApplicationsSearchProviderStatistics
{
	gint								refCount;

	guint								usedCounter;
};

/* Create, destroy, ref and unref statistics data */
static XfdashboardApplicationsSearchProviderStatistics* _xfdashboard_applications_search_provider_statistics_new(void)
{
	XfdashboardApplicationsSearchProviderStatistics	*data;

	/* Create statistics data */
	data=g_new0(XfdashboardApplicationsSearchProviderStatistics, 1);
	if(!data) return(NULL);

	/* Set up statistics data */
	data->refCount=1;

	return(data);
}

static void _xfdashboard_applications_search_provider_statistics_free(XfdashboardApplicationsSearchProviderStatistics *inData)
{
	g_return_if_fail(inData);

	/* Release common allocated resources */
	g_free(inData);
}

static XfdashboardApplicationsSearchProviderStatistics* _xfdashboard_applications_search_provider_statistics_ref(XfdashboardApplicationsSearchProviderStatistics *inData)
{
	g_return_val_if_fail(inData, NULL);

	inData->refCount++;
	return(inData);
}

static void _xfdashboard_applications_search_provider_statistics_unref(XfdashboardApplicationsSearchProviderStatistics *inData)
{
	g_return_if_fail(inData);

	inData->refCount--;
	if(inData->refCount==0) _xfdashboard_applications_search_provider_statistics_free(inData);
}

static XfdashboardApplicationsSearchProviderStatistics* _xfdashboard_applications_search_provider_statistics_get(const gchar *inAppID)
{
	XfdashboardApplicationsSearchProviderStatistics		*stats;

	g_return_val_if_fail(_xfdashboard_applications_search_provider_statistics.stats, NULL);
	g_return_val_if_fail(inAppID && *inAppID, NULL);

	/* Lookup statistics data by application ID. If this application could not be found,
	 * then return NULL pointer.
	 */
	if(!g_hash_table_lookup_extended(_xfdashboard_applications_search_provider_statistics.stats, inAppID, NULL, (gpointer*)&stats))
	{
		stats=NULL;
	}

	/* Return statistics data or NULL */
	return(stats);
}

/* An application was launched successfully */
static void _xfdashboard_applications_search_provider_on_application_launched(XfdashboardApplication *inApplication,
																				GAppInfo *inAppInfo,
																				gpointer inUserData)
{
	const gchar											*appID;
	XfdashboardApplicationsSearchProviderStatistics		*stats;

	g_return_if_fail(G_IS_APP_INFO(inAppInfo));

	/* Lock for thread-safety */
	G_LOCK(_xfdashboard_applications_search_provider_statistics_lock);

	/* Get application ID which is used to lookup and store statistics */
	appID=g_app_info_get_id(inAppInfo);

	/* Create new statistics data if application is new, otherwise take an extra
	 * reference on statistics data to keep it alive as it will be removed and
	 * re-added when updating and the removal may decrease the reference counter
	 * to zero which destroys the statistics data.
	 */
	stats=_xfdashboard_applications_search_provider_statistics_get(appID);
	if(!stats) stats=_xfdashboard_applications_search_provider_statistics_new();
		else _xfdashboard_applications_search_provider_statistics_ref(stats);

	/* Increase launch counter and remember it has highest launch counter if it
	 * is now higher than the one we remembered.
	 */
	stats->usedCounter++;
	if(stats->usedCounter>_xfdashboard_applications_search_provider_statistics.maxUsedCounter)
	{
		_xfdashboard_applications_search_provider_statistics.maxUsedCounter=stats->usedCounter;
	}

	/* Store updated statistics */
	g_hash_table_insert(_xfdashboard_applications_search_provider_statistics.stats,
						g_strdup(appID),
						_xfdashboard_applications_search_provider_statistics_ref(stats));

	/* Release extra reference we took to keep this statistics data alive */
	_xfdashboard_applications_search_provider_statistics_unref(stats);

	/* Unlock for thread-safety */
	G_UNLOCK(_xfdashboard_applications_search_provider_statistics_lock);
}

/* Save statistics to file */
static gboolean _xfdashboard_applications_search_provider_save_statistics(GError **outError)
{
	GKeyFile												*keyFile;
	gchar													*keyFileData;
	gsize													keyFileLength;
	GList													*allAppIDs;
	GList													*iter;
	guint													entriesCount;
	gchar													*fileFolder;
	GError													*error;

	g_return_val_if_fail(outError==NULL || *outError==NULL, FALSE);

	error=NULL;

	/* If we have no filename do not store statistics but do not return error */
	if(!_xfdashboard_applications_search_provider_statistics.filename) return(TRUE);

	/* Create parent folders for key file if not available */
	fileFolder=g_path_get_dirname(_xfdashboard_applications_search_provider_statistics.filename);
	if(g_mkdir_with_parents(fileFolder, 0700)<0)
	{
		int											errno_save;

		/* Get error code */
		errno_save=errno;

		/* Set error */
		g_set_error(outError,
						G_IO_ERROR,
						g_io_error_from_errno(errno_save),
						_("Could not create configuration folder for applications search provider at %s: %s"),
						fileFolder,
						g_strerror(errno_save));

		/* Release allocated resources */
		if(fileFolder) g_free(fileFolder);

		return(FALSE);
	}

	/* Create and set up key file to store statistics */
	keyFile=g_key_file_new();

	/* Get list of all applications from statistics hash table, iterate through
	 * all applications and store them in key file.
	 */
	allAppIDs=g_hash_table_get_keys(_xfdashboard_applications_search_provider_statistics.stats);

	g_key_file_set_integer(keyFile,
							XFDASHBOARD_APPLICATIONS_SEARCH_PROVIDER_STATISTICS_ENTRIES_GROUP,
							XFDASHBOARD_APPLICATIONS_SEARCH_PROVIDER_STATISTICS_ENTRIES_COUNT,
							g_list_length(allAppIDs));

	entriesCount=0;
	for(iter=allAppIDs; iter; iter=g_list_next(iter))
	{
		gchar												*name;
		const gchar											*appID;
		XfdashboardApplicationsSearchProviderStatistics		*stats;

		/* Get current iterated application desktop ID */
		appID=(const gchar*)iter->data;

		/* Get statistics data (it must exist because we got a list of all keys) */
		stats=_xfdashboard_applications_search_provider_statistics_get(appID);
		g_assert(stats!=NULL);

		/* Store application desktop ID in "entries" overview group and increase counter */
		entriesCount++;

		name=g_strdup_printf("%d", entriesCount);
		g_key_file_set_string(keyFile,
								XFDASHBOARD_APPLICATIONS_SEARCH_PROVIDER_STATISTICS_ENTRIES_GROUP,
								name,
								appID);
		g_free(name);

		/* Store statistics in key file in their groups but try to avoid to store
		 * default values to keep key file small.
		 */
		if(stats->usedCounter>0)
		{
			g_key_file_set_integer(keyFile,
									XFDASHBOARD_APPLICATIONS_SEARCH_PROVIDER_STATISTICS_USED_COUNTER_GROUP,
									appID,
									stats->usedCounter);
		}
	}

	/* Store key file for statistics in file */
	keyFileData=g_key_file_to_data(keyFile, &keyFileLength, NULL);
	if(!g_file_set_contents(_xfdashboard_applications_search_provider_statistics.filename, keyFileData, keyFileLength, &error))
	{
		/* Propagate error */
		g_propagate_error(outError, error);

		/* Release allocated resources */
		if(fileFolder) g_free(fileFolder);
		if(keyFileData) g_free(keyFileData);
		if(keyFile) g_key_file_free(keyFile);

		return(FALSE);
	}

	/* Release allocated resources */
	if(fileFolder) g_free(fileFolder);
	if(keyFileData) g_free(keyFileData);
	if(keyFile) g_key_file_free(keyFile);

	/* If we get here saving statistics file was successful */
	return(TRUE);
}

/* Load statistics from file */
static gboolean _xfdashboard_applications_search_provider_load_statistics(XfdashboardApplicationsSearchProvider *self,
																			GError **outError)
{
	GKeyFile												*keyFile;
	GList													*allAppIDs;
	GList													*iter;
	guint													entriesCount;
	GError													*error;

	g_return_val_if_fail(XFDASHBOARD_IS_APPLICATIONS_SEARCH_PROVIDER(self), FALSE);
	g_return_val_if_fail(outError==NULL || *outError==NULL, FALSE);

	error=NULL;

	/* If no statistics were set up, we cannot load from file */
	if(!_xfdashboard_applications_search_provider_statistics.stats)
	{
			/* Set error */
			g_set_error(outError,
							G_IO_ERROR,
							G_IO_ERROR_FAILED,
							_("Statistics were not initialized"));

			return(FALSE);
	}

	/* Get path to statistics file to load statistics from */
	if(!_xfdashboard_applications_search_provider_statistics.filename)
	{
		_xfdashboard_applications_search_provider_statistics.filename=
			g_build_filename(g_get_user_data_dir(),
								"xfdashboard",
								XFDASHBOARD_APPLICATIONS_SEARCH_PROVIDER_STATISTICS_FILE,
								NULL);

		if(!_xfdashboard_applications_search_provider_statistics.filename)
		{
			/* Set error */
			g_set_error(outError,
							G_IO_ERROR,
							G_IO_ERROR_NOT_FOUND,
							_("Could not build path to statistics file of applications search provider"));

			return(FALSE);
		}
	}
	XFDASHBOARD_DEBUG(self, APPLICATIONS,
						"Will load statistics of applications search provider from %s",
						_xfdashboard_applications_search_provider_statistics.filename);

	/* If statistics file does not exist then return immediately but with success */
	if(!g_file_test(_xfdashboard_applications_search_provider_statistics.filename, G_FILE_TEST_EXISTS | G_FILE_TEST_IS_REGULAR))
	{
		XFDASHBOARD_DEBUG(self, APPLICATIONS,
							"Statistics file %s does not exists. Will create empty statistics database for applications search provider",
							_xfdashboard_applications_search_provider_statistics.filename);

		return(TRUE);
	}

	/* Load statistics from key file */
	keyFile=g_key_file_new();
	if(!g_key_file_load_from_file(keyFile, _xfdashboard_applications_search_provider_statistics.filename, G_KEY_FILE_NONE, &error))
	{
		/* Propagate error */
		g_propagate_error(outError, error);

		/* Release allocated resources */
		if(keyFile) g_key_file_free(keyFile);

		return(FALSE);
	}

	/* Get number of applications and their application desktop IDs from key file */
	entriesCount=g_key_file_get_integer(keyFile,
										XFDASHBOARD_APPLICATIONS_SEARCH_PROVIDER_STATISTICS_ENTRIES_GROUP,
										XFDASHBOARD_APPLICATIONS_SEARCH_PROVIDER_STATISTICS_ENTRIES_COUNT,
										&error);
	if(error)
	{
		/* Propagate error */
		g_propagate_error(outError, error);

		/* Release allocated resources */
		if(keyFile) g_key_file_free(keyFile);

		return(FALSE);
	}
	XFDASHBOARD_DEBUG(self, APPLICATIONS,
						"Will load statistics for %d applications",
						entriesCount);

	allAppIDs=NULL;
	while(entriesCount>0)
	{
		gchar											*name;
		gchar											*appID;

		/* Get application desktop ID */
		name=g_strdup_printf("%d", entriesCount);
		appID=g_key_file_get_string(keyFile,
									XFDASHBOARD_APPLICATIONS_SEARCH_PROVIDER_STATISTICS_ENTRIES_GROUP,
									name,
									&error);
		g_free(name);

		if(error)
		{
			/* Propagate error */
			g_propagate_error(outError, error);

			/* Release allocated resources */
			if(appID) g_free(appID);
			if(allAppIDs) g_list_free_full(allAppIDs, g_free);
			if(keyFile) g_key_file_free(keyFile);

			return(FALSE);
		}

		/* Store application desktop ID in list to iterate */
		allAppIDs=g_list_prepend(allAppIDs, appID);
		XFDASHBOARD_DEBUG(self, APPLICATIONS,
							"Will load statistics for application '%s'",
							appID);

		/* Continue with next application in entries group */
		entriesCount--;
	}

	/* Iterate through all application desktop IDs and create statistics data
	 * with default values for each one first. Then try to load stored values
	 * from key file.
	 */
	for(iter=allAppIDs; iter; iter=g_list_next(iter))
	{
		const gchar											*appID;
		XfdashboardApplicationsSearchProviderStatistics		*stats;

		/* Get current iterated application desktop ID */
		appID=(const gchar*)iter->data;

		/* Create statistics data for application with default values */
		stats=_xfdashboard_applications_search_provider_statistics_new();
		if(!stats)
		{
			g_critical(_("Could not create statistics data for application '%s' of applications search provider"), appID);
			continue;
		}

		/* Try to load stored values for application from key file */
		if(g_key_file_has_key(keyFile, XFDASHBOARD_APPLICATIONS_SEARCH_PROVIDER_STATISTICS_USED_COUNTER_GROUP, appID, NULL))
		{
			stats->usedCounter=g_key_file_get_integer(keyFile,
														XFDASHBOARD_APPLICATIONS_SEARCH_PROVIDER_STATISTICS_USED_COUNTER_GROUP,
														appID,
														&error);
			if(error)
			{
				g_critical(_("Could not get value from group [%s] for application %s from statistics file of applications search provider: %s"),
							XFDASHBOARD_APPLICATIONS_SEARCH_PROVIDER_STATISTICS_USED_COUNTER_GROUP,
							appID,
							error->message);
				g_clear_error(&error);
			}

			if(stats->usedCounter>_xfdashboard_applications_search_provider_statistics.maxUsedCounter)
			{
				_xfdashboard_applications_search_provider_statistics.maxUsedCounter=stats->usedCounter;
			}
		}

		/* Store statistics data for application in hash-table */
		g_hash_table_insert(_xfdashboard_applications_search_provider_statistics.stats, g_strdup(appID), _xfdashboard_applications_search_provider_statistics_ref(stats));
		XFDASHBOARD_DEBUG(self, APPLICATIONS,
							"Loaded and stored statistics for '%s' for applications search provider",
							appID);

		/* Release allocated resources */
		_xfdashboard_applications_search_provider_statistics_unref(stats);
	}

	/* Release allocated resources */
	if(allAppIDs) g_list_free_full(allAppIDs, g_free);
	if(keyFile) g_key_file_free(keyFile);

	/* If we get here saving statistics file was successful */
	XFDASHBOARD_DEBUG(self, APPLICATIONS,
						"Loaded statistics of applications search provider from %s",
						_xfdashboard_applications_search_provider_statistics.filename);

	return(TRUE);
}

/* Destroy statistics for this search provider */
static void _xfdashboard_applications_search_provider_destroy_statistics(void)
{
	XfdashboardApplication			*application;
	GError							*error;

	error=NULL;

	/* Only existing statistics can be destroyed */
	if(!_xfdashboard_applications_search_provider_statistics.stats) return;

	/* Lock for thread-safety */
	G_LOCK(_xfdashboard_applications_search_provider_statistics_lock);

	/* Get application instance */
	application=xfdashboard_application_get_default();

	/* Disconnect application "shutdown" signal handler */
	if(_xfdashboard_applications_search_provider_statistics.shutdownSignalID)
	{
		g_signal_handler_disconnect(application, _xfdashboard_applications_search_provider_statistics.shutdownSignalID);
		_xfdashboard_applications_search_provider_statistics.shutdownSignalID=0;
	}

	/* Disconnect application "application-launched" signal handler */
	if(_xfdashboard_applications_search_provider_statistics.applicationLaunchedSignalID)
	{
		g_signal_handler_disconnect(application, _xfdashboard_applications_search_provider_statistics.applicationLaunchedSignalID);
		_xfdashboard_applications_search_provider_statistics.applicationLaunchedSignalID=0;
	}

	/* Save statistics to file */
	if(!_xfdashboard_applications_search_provider_save_statistics(&error))
	{
		g_critical(_("Failed to save statistics of applications search provider to %s: %s"),
					_xfdashboard_applications_search_provider_statistics.filename,
					error ? error->message : _("Unknown error"));
		if(error) g_clear_error(&error);
	}

	/* Destroy statistics */
	XFDASHBOARD_DEBUG(NULL, APPLICATIONS, "Destroying statistics of applications search provider");
	g_hash_table_destroy(_xfdashboard_applications_search_provider_statistics.stats);
	_xfdashboard_applications_search_provider_statistics.stats=NULL;

	/* Destroy filename for statistics */
	if(_xfdashboard_applications_search_provider_statistics.filename)
	{
		g_free(_xfdashboard_applications_search_provider_statistics.filename);
		_xfdashboard_applications_search_provider_statistics.filename=NULL;
	}

	/* Reset other variables */
	_xfdashboard_applications_search_provider_statistics.maxUsedCounter=0;

	/* Unlock for thread-safety */
	G_UNLOCK(_xfdashboard_applications_search_provider_statistics_lock);
}

/* Create and load statistics for this search provider if not done already */
static void _xfdashboard_applications_search_provider_create_statistics(XfdashboardApplicationsSearchProvider *self)
{
	XfdashboardApplication			*application;
	GError							*error;

	g_return_if_fail(XFDASHBOARD_IS_APPLICATIONS_SEARCH_PROVIDER(self));

	error=NULL;

	/* Statistics were already set up */
	if(_xfdashboard_applications_search_provider_statistics.stats) return;

	g_assert(!_xfdashboard_applications_search_provider_statistics.shutdownSignalID);
	g_assert(!_xfdashboard_applications_search_provider_statistics.applicationLaunchedSignalID);

	/* Lock for thread-safety */
	G_LOCK(_xfdashboard_applications_search_provider_statistics_lock);

	/* Initialize non-critical variables */
	_xfdashboard_applications_search_provider_statistics.maxUsedCounter=0;

	/* Create hash-table for statistics */
	_xfdashboard_applications_search_provider_statistics.stats=
		g_hash_table_new_full(g_str_hash,
								g_str_equal,
								g_free,
								(GDestroyNotify)_xfdashboard_applications_search_provider_statistics_unref);
	XFDASHBOARD_DEBUG(self, APPLICATIONS, "Created statistics of applications search provider");

	/* Load statistics from file */
	if(!_xfdashboard_applications_search_provider_load_statistics(self, &error))
	{
		g_critical(_("Failed to load statistics of applications search provider from %s: %s"),
					_xfdashboard_applications_search_provider_statistics.filename,
					error ? error->message : _("Unknown error"));
		if(error) g_clear_error(&error);

		/* Destroy hash-table to avoid the half-loaded hash-table being stored again
		 * and overriding existing statistics file (even if it may be broken).
		 * Also release 
		 */
		if(_xfdashboard_applications_search_provider_statistics.stats)
		{
			g_hash_table_destroy(_xfdashboard_applications_search_provider_statistics.stats);
			_xfdashboard_applications_search_provider_statistics.stats=NULL;
		}

		if(_xfdashboard_applications_search_provider_statistics.filename)
		{
			g_free(_xfdashboard_applications_search_provider_statistics.filename);
			_xfdashboard_applications_search_provider_statistics.filename=NULL;
		}

		/* Unlock for thread-safety */
		G_UNLOCK(_xfdashboard_applications_search_provider_statistics_lock);

		/* Return here to prevent further initializations of statistics
		 * which are not used and not deinitialized in 'destroy' function.
		 */
		return;
	}

	/* Get application instance */
	application=xfdashboard_application_get_default();

	/* Connect to "shutdown" signal of application to clean up statistics */
	_xfdashboard_applications_search_provider_statistics.shutdownSignalID=
		g_signal_connect(application,
							"shutdown-final",
							G_CALLBACK(_xfdashboard_applications_search_provider_destroy_statistics),
							NULL);

	/* Connect to "application-launched" signal of application to track app launches */
	_xfdashboard_applications_search_provider_statistics.applicationLaunchedSignalID=
		g_signal_connect(application,
							"application-launched",
							G_CALLBACK(_xfdashboard_applications_search_provider_on_application_launched),
							NULL);

	/* Unlock for thread-safety */
	G_UNLOCK(_xfdashboard_applications_search_provider_statistics_lock);
}

/* An application was added to database */
static void _xfdashboard_applications_search_provider_on_application_added(XfdashboardApplicationsSearchProvider *self,
																			GAppInfo *inAppInfo,
																			gpointer inUserData)
{
	XfdashboardApplicationsSearchProviderPrivate	*priv;

	g_return_if_fail(XFDASHBOARD_IS_APPLICATIONS_SEARCH_PROVIDER(self));

	priv=self->priv;

	/* Release current list of all installed applications */
	if(priv->allApps)
	{
		g_list_free_full(priv->allApps, g_object_unref);
		priv->allApps=NULL;
	}

	/* Get new list of all installed applications */
	priv->allApps=xfdashboard_application_database_get_all_applications(priv->appDB);
}

/* An application was removed to database */
static void _xfdashboard_applications_search_provider_on_application_removed(XfdashboardApplicationsSearchProvider *self,
																				GAppInfo *inAppInfo,
																				gpointer inUserData)
{
	XfdashboardApplicationsSearchProviderPrivate	*priv;

	g_return_if_fail(XFDASHBOARD_IS_APPLICATIONS_SEARCH_PROVIDER(self));

	priv=self->priv;

	/* Release current list of all installed applications */
	if(priv->allApps)
	{
		g_list_free_full(priv->allApps, g_object_unref);
		priv->allApps=NULL;
	}

	/* Get new list of all installed applications */
	priv->allApps=xfdashboard_application_database_get_all_applications(priv->appDB);
}

/* User selected to open a new window or to launch that application at pop-up menu */
static void _xfdashboard_applications_search_provider_on_popup_menu_item_launch(XfdashboardPopupMenuItem *inMenuItem,
																				gpointer inUserData)
{
	GAppInfo							*appInfo;
	XfdashboardApplicationTracker		*appTracker;
	GIcon								*gicon;
	const gchar							*iconName;

	g_return_if_fail(XFDASHBOARD_IS_POPUP_MENU_ITEM(inMenuItem));
	g_return_if_fail(G_IS_APP_INFO(inUserData));

	appInfo=G_APP_INFO(inUserData);
	iconName=NULL;

	/* Get icon of application */
	gicon=g_app_info_get_icon(appInfo);
	if(gicon) iconName=g_icon_to_string(gicon);

	/* Check if we should launch that application or to open a new window */
	appTracker=xfdashboard_application_tracker_get_default();
	if(!xfdashboard_application_tracker_is_running_by_app_info(appTracker, appInfo))
	{
		GAppLaunchContext			*context;
		GError						*error;

		/* Create context to start application at */
		context=xfdashboard_create_app_context(NULL);

		/* Try to launch application */
		error=NULL;
		if(!g_app_info_launch(appInfo, NULL, context, &error))
		{
			/* Show notification about failed application launch */
			xfdashboard_notify(CLUTTER_ACTOR(inMenuItem),
								iconName,
								_("Launching application '%s' failed: %s"),
								g_app_info_get_display_name(appInfo),
								(error && error->message) ? error->message : _("unknown error"));
			g_warning(_("Launching application '%s' failed: %s"),
						g_app_info_get_display_name(appInfo),
						(error && error->message) ? error->message : _("unknown error"));
			if(error) g_error_free(error);
		}
			else
			{
				/* Show notification about successful application launch */
				xfdashboard_notify(CLUTTER_ACTOR(inMenuItem),
									iconName,
									_("Application '%s' launched"),
									g_app_info_get_display_name(appInfo));

				/* Emit signal for successful application launch */
				g_signal_emit_by_name(xfdashboard_application_get_default(), "application-launched", appInfo);

				/* Quit application */
				xfdashboard_application_suspend_or_quit(NULL);
			}

		/* Release allocated resources */
		g_object_unref(context);
	}

	/* Release allocated resources */
	g_object_unref(appTracker);
	g_object_unref(gicon);
}

/* A right-click might have happened on an application icon */
static void _xfdashboard_applications_search_provider_on_popup_menu(XfdashboardApplicationsSearchProvider *self,
																	ClutterActor *inActor,
																	gpointer inUserData)
{
	XfdashboardApplicationButton				*button;
	XfdashboardClickAction						*action;

	g_return_if_fail(XFDASHBOARD_IS_APPLICATIONS_SEARCH_PROVIDER(self));
	g_return_if_fail(XFDASHBOARD_IS_APPLICATION_BUTTON(inActor));
	g_return_if_fail(XFDASHBOARD_IS_CLICK_ACTION(inUserData));

	button=XFDASHBOARD_APPLICATION_BUTTON(inActor);
	action=XFDASHBOARD_CLICK_ACTION(inUserData);

	/* Check if right button was used when the application button was clicked */
	if(xfdashboard_click_action_get_button(action)==XFDASHBOARD_CLICK_ACTION_RIGHT_BUTTON)
	{
		ClutterActor							*popup;
		ClutterActor							*menuItem;
		GAppInfo								*appInfo;
		XfdashboardApplicationTracker			*appTracker;
		gchar									*sourceStyleClass;

		/* Get app info for application button as it is needed most the time */
		appInfo=xfdashboard_application_button_get_app_info(button);
		if(!appInfo)
		{
			g_critical(_("No application information available for clicked application button."));
			return;
		}

		/* Create pop-up menu */
		popup=xfdashboard_popup_menu_new();
		xfdashboard_popup_menu_set_destroy_on_cancel(XFDASHBOARD_POPUP_MENU(popup), TRUE);
		xfdashboard_popup_menu_set_title(XFDASHBOARD_POPUP_MENU(popup), g_app_info_get_display_name(appInfo));
		xfdashboard_popup_menu_set_title_gicon(XFDASHBOARD_POPUP_MENU(popup), g_app_info_get_icon(appInfo));

		/* Add each open window to pop-up of application */
		if(xfdashboard_application_button_add_popup_menu_items_for_windows(button, XFDASHBOARD_POPUP_MENU(popup))>0)
		{
			/* Add a separator to split windows from other actions in pop-up menu */
			menuItem=xfdashboard_popup_menu_item_separator_new();
			clutter_actor_set_x_expand(menuItem, TRUE);
			xfdashboard_popup_menu_add_item(XFDASHBOARD_POPUP_MENU(popup), XFDASHBOARD_POPUP_MENU_ITEM(menuItem));
		}

		/* Add menu item to launch application if it is not running */
		appTracker=xfdashboard_application_tracker_get_default();
		if(!xfdashboard_application_tracker_is_running_by_app_info(appTracker, appInfo))
		{
			menuItem=xfdashboard_popup_menu_item_button_new();
			xfdashboard_label_set_text(XFDASHBOARD_LABEL(menuItem), _("Launch"));
			clutter_actor_set_x_expand(menuItem, TRUE);
			xfdashboard_popup_menu_add_item(XFDASHBOARD_POPUP_MENU(popup), XFDASHBOARD_POPUP_MENU_ITEM(menuItem));

			g_signal_connect(menuItem,
								"activated",
								G_CALLBACK(_xfdashboard_applications_search_provider_on_popup_menu_item_launch),
								appInfo);
		}
		g_object_unref(appTracker);

		/* Add application actions */
		xfdashboard_application_button_add_popup_menu_items_for_actions(button, XFDASHBOARD_POPUP_MENU(popup));

		/* Set style class as pop-up menu has no source set to create style
		 * class automatically because this class is not derived from an actor
		 * class.
		 */
		sourceStyleClass=g_strdup_printf("popup-menu-source-%s", G_OBJECT_TYPE_NAME(self));
		xfdashboard_stylable_add_class(XFDASHBOARD_STYLABLE(popup), sourceStyleClass);
		g_free(sourceStyleClass);

		/* Activate pop-up menu */
		xfdashboard_popup_menu_activate(XFDASHBOARD_POPUP_MENU(popup));
	}
}

/* Drag of an menu item begins */
static void _xfdashboard_applications_search_provider_on_drag_begin(ClutterDragAction *inAction,
																	ClutterActor *inActor,
																	gfloat inStageX,
																	gfloat inStageY,
																	ClutterModifierType inModifiers,
																	gpointer inUserData)
{
	GAppInfo							*appInfo;
	ClutterActor						*dragHandle;
	ClutterStage						*stage;

	g_return_if_fail(CLUTTER_IS_DRAG_ACTION(inAction));
	g_return_if_fail(XFDASHBOARD_IS_APPLICATION_BUTTON(inActor));
	g_return_if_fail(XFDASHBOARD_IS_APPLICATIONS_SEARCH_PROVIDER(inUserData));

	/* Get stage */
	stage=CLUTTER_STAGE(clutter_actor_get_stage(inActor));

	/* Create a application icon for drag handle */
	appInfo=xfdashboard_application_button_get_app_info(XFDASHBOARD_APPLICATION_BUTTON(inActor));

	dragHandle=xfdashboard_application_button_new_from_app_info(appInfo);
	clutter_actor_set_position(dragHandle, inStageX, inStageY);
	clutter_actor_add_child(CLUTTER_ACTOR(stage), dragHandle);

	clutter_drag_action_set_drag_handle(inAction, dragHandle);
}

/* Drag of a result item ends */
static void _xfdashboard_applications_search_provider_on_drag_end(ClutterDragAction *inAction,
																	ClutterActor *inActor,
																	gfloat inStageX,
																	gfloat inStageY,
																	ClutterModifierType inModifiers,
																	gpointer inUserData)
{
	ClutterActor					*dragHandle;

	g_return_if_fail(CLUTTER_IS_DRAG_ACTION(inAction));
	g_return_if_fail(XFDASHBOARD_IS_APPLICATION_BUTTON(inActor));
	g_return_if_fail(XFDASHBOARD_IS_APPLICATIONS_SEARCH_PROVIDER(inUserData));

	/* Destroy clone of application icon used as drag handle */
	dragHandle=clutter_drag_action_get_drag_handle(inAction);
	if(dragHandle)
	{
#if CLUTTER_CHECK_VERSION(1, 14, 0)
		/* Only unset drag handle if not running Clutter in version
		 * 1.12. This prevents a critical warning message in 1.12.
		 * Later versions of Clutter are fixed already.
		 */
		clutter_drag_action_set_drag_handle(inAction, NULL);
#endif
		clutter_actor_destroy(dragHandle);
	}
}

/* Check if given app info matches search terms and return score as fraction
 * between 0.0and 1.0 - so called "relevance". A negative score means that
 * the given app info does not match at all.
 */
static gfloat _xfdashboard_applications_search_provider_score(XfdashboardApplicationsSearchProvider *self,
																gchar **inSearchTerms,
																GAppInfo *inAppInfo)
{
	XfdashboardApplicationsSearchProviderPrivate		*priv;
	gchar												*title;
	gchar												*description;
	GList												*keywords;
	const gchar											*command;
	const gchar											*value;
	gint												matchesFound, matchesExpected;
	gfloat												pointsSearch;
	gfloat												score;

	g_return_val_if_fail(XFDASHBOARD_IS_APPLICATIONS_SEARCH_PROVIDER(self), -1.0f);
	g_return_val_if_fail(G_IS_APP_INFO(inAppInfo), -1.0f);

	priv=self->priv;
	score=-1.0f;
	keywords=NULL;

	/* Empty search term matches no menu item */
	if(!inSearchTerms) return(0.0f);

	matchesExpected=g_strv_length(inSearchTerms);
	if(matchesExpected==0) return(0.0f);

	/* Calculate the highest score points possible which is the highest
	 * launch count among all applications, display name matches all search terms,
	 * description matches all search terms and also the command matches all
	 * search terms.
	 *
	 * But a matching display names weights more than a matching description and
	 * also a matching command or keyword weights more than a matching description.
	 * So the weights are 0.4 for matching display names, 0.25 for matching commands,
	 * 0.25 for matching keywords and 0.1 for matching descriptions.
	 *
	 * While iterating through all search terms we add the weights "points" for
	 * each matching item and when we iterated through all search terms we divide
	 * the total weight "points" by the number of search terms to get the average
	 * which is also the result score when *not* taking the launch count of
	 * application into account.
	 */
	value=g_app_info_get_display_name(inAppInfo);
	if(value) title=g_utf8_strdown(value, -1);
		else title=NULL;

	value=g_app_info_get_description(inAppInfo);
	if(value) description=g_utf8_strdown(value, -1);
		else description=NULL;

	command=g_app_info_get_executable(inAppInfo);

	if(XFDASHBOARD_IS_DESKTOP_APP_INFO(inAppInfo))
	{
		GList											*appKeywords;
		GList											*iter;

		appKeywords=xfdashboard_desktop_app_info_get_keywords(XFDASHBOARD_DESKTOP_APP_INFO(inAppInfo));
		for(iter=appKeywords; iter; iter=g_list_next(iter))
		{
			keywords=g_list_prepend(keywords, g_utf8_strdown(iter->data, -1));
		}
	}

	matchesFound=0;
	pointsSearch=0.0f;
	while(*inSearchTerms)
	{
		gboolean										termMatch;
		gchar											*commandPos;
		gfloat											pointsTerm;

		/* Reset "found" indicator and score of current search term */
		termMatch=FALSE;
		pointsTerm=0.0f;

		/* Check for current search term */
		if(title &&
			g_strstr_len(title, -1, *inSearchTerms))
		{
			pointsTerm+=0.4;
			termMatch=TRUE;
		}

		if(keywords)
		{
			GList						*iter;

			for(iter=keywords; iter; iter=g_list_next(iter))
			{
				if(iter->data &&
					g_strstr_len(iter->data, -1, *inSearchTerms))
				{
					pointsTerm+=0.25;
					termMatch=TRUE;
					break;
				}
			}
		}

		if(command)
		{
			commandPos=g_strstr_len(command, -1, *inSearchTerms);
			if(commandPos &&
				(commandPos==command || *(commandPos-1)==G_DIR_SEPARATOR))
			{
				pointsTerm+=0.25;
				termMatch=TRUE;
			}
		}

		if(description &&
			g_strstr_len(description, -1, *inSearchTerms))
		{
			pointsTerm+=0.1;
			termMatch=TRUE;
		}

		/* Increase match counter if we found a match */
		if(termMatch)
		{
			matchesFound++;
			pointsSearch+=pointsTerm;
		}

		/* Continue with next search term */
		inSearchTerms++;
	}

	/* If we got a match in either title, description or command for each search term
	 * then calculate score and also check if we should take the number of
	 * launches of this application into account.
	 */
	if(matchesFound>=matchesExpected)
	{
		gfloat											currentPoints;
		gfloat											maxPoints;
		XfdashboardApplicationsSearchProviderStatistics	*stats;

		currentPoints=0.0f;
		maxPoints=0.0f;

		/* Set maximum points to the number of expected number of matches
		 * if we should take title, description and command into calculation.
		 */
		if(priv->currentSortMode & XFDASHBOARD_APPLICATIONS_SEARCH_PROVIDER_SORT_MODE_NAMES)
		{
			currentPoints+=pointsSearch;
			maxPoints+=matchesExpected*1.0f;
		}

		/* If used counter should be taken into calculation add the highest number
		 * of any application to the highest points possible and also add the number
		 * of launches of this application to the total points we got so far.
		 */
		if(priv->currentSortMode & XFDASHBOARD_APPLICATIONS_SEARCH_PROVIDER_SORT_MODE_MOST_USED)
		{
			maxPoints+=(_xfdashboard_applications_search_provider_statistics.maxUsedCounter*1.0f);

			stats=_xfdashboard_applications_search_provider_statistics_get(g_app_info_get_id(inAppInfo));
			if(stats) currentPoints+=(stats->usedCounter*1.0f);
		}

		/* Calculate score but if maximum points is still zero we should do a simple
		 * match by setting score to 1.
		 */
		if(maxPoints>0.0f) score=currentPoints/maxPoints;
			else score=1.0f;
	}

	/* Release allocated resources */
	if(keywords) g_list_free_full(keywords, g_free);
	if(description) g_free(description);
	if(title) g_free(title);

	/* Return score of this application for requested search terms */
	return(score);
}

/* Callback to sort each item in result set */
static gint _xfdashboard_applications_search_provider_sort_result_set(GVariant *inLeft,
																		GVariant *inRight,
																		gpointer inUserData)
{
	XfdashboardApplicationsSearchProvider				*self;
	XfdashboardApplicationsSearchProviderPrivate		*priv;
	const gchar											*leftID;
	const gchar											*rightID;
	GAppInfo											*leftAppInfo;
	GAppInfo											*rightAppInfo;
	const gchar											*leftName;
	const gchar											*rightName;
	gchar												*lowerLeftName;
	gchar												*lowerRightName;
	gint												result;

	g_return_val_if_fail(inLeft, 0);
	g_return_val_if_fail(inRight, 0);
	g_return_val_if_fail(XFDASHBOARD_IS_APPLICATIONS_SEARCH_PROVIDER(inUserData), 0);

	self=XFDASHBOARD_APPLICATIONS_SEARCH_PROVIDER(inUserData);
	priv=self->priv;

	/* Get desktop IDs of both items */
	leftID=g_variant_get_string(inLeft, NULL);
	rightID=g_variant_get_string(inRight, NULL);

	/* Get desktop application information of both items */
	leftAppInfo=xfdashboard_application_database_lookup_desktop_id(priv->appDB, leftID);
	if(leftAppInfo) leftName=g_app_info_get_display_name(G_APP_INFO(leftAppInfo));
		else leftName=NULL;

	rightAppInfo=xfdashboard_application_database_lookup_desktop_id(priv->appDB, rightID);
	if(rightAppInfo) rightName=g_app_info_get_display_name(G_APP_INFO(rightAppInfo));
		else rightName=NULL;

	/* Get result of comparing both desktop application information objects */
	if(leftName) lowerLeftName=g_utf8_strdown(leftName, -1);
		else lowerLeftName=NULL;

	if(rightName) lowerRightName=g_utf8_strdown(rightName, -1);
		else lowerRightName=NULL;

	result=g_strcmp0(lowerLeftName, lowerRightName);

	/* Release allocated resources */
	if(rightAppInfo) g_object_unref(rightAppInfo);
	if(leftAppInfo) g_object_unref(leftAppInfo);
	if(lowerLeftName) g_free(lowerLeftName);
	if(lowerRightName) g_free(lowerRightName);

	/* Return result */
	return(result);
}

/* IMPLEMENTATION: XfdashboardSearchProvider */
static void _xfdashboard_applications_search_provider_initialize(XfdashboardSearchProvider *inProvider)
{
	g_return_if_fail(XFDASHBOARD_IS_APPLICATIONS_SEARCH_PROVIDER(inProvider));

	/* Create and load statistics hash-table (will only be done once) */
	_xfdashboard_applications_search_provider_create_statistics(XFDASHBOARD_APPLICATIONS_SEARCH_PROVIDER(inProvider));
}

/* Get display name for this search provider */
static const gchar* _xfdashboard_applications_search_provider_get_name(XfdashboardSearchProvider *inProvider)
{
	return(_("Applications"));
}

/* Get icon-name for this search provider */
static const gchar* _xfdashboard_applications_search_provider_get_icon(XfdashboardSearchProvider *inProvider)
{
	return("go-home");
}

/* Get result set for requested search terms */
static XfdashboardSearchResultSet* _xfdashboard_applications_search_provider_get_result_set(XfdashboardSearchProvider *inProvider,
																							const gchar **inSearchTerms,
																							XfdashboardSearchResultSet *inPreviousResultSet)
{
	XfdashboardApplicationsSearchProvider				*self;
	XfdashboardApplicationsSearchProviderPrivate		*priv;
	XfdashboardSearchResultSet							*resultSet;
	GList												*iter;
	guint												numberTerms;
	gchar												**terms, **termsIter;
	XfdashboardDesktopAppInfo							*appInfo;
	gfloat												score;

	g_return_val_if_fail(XFDASHBOARD_IS_APPLICATIONS_SEARCH_PROVIDER(inProvider), NULL);

	self=XFDASHBOARD_APPLICATIONS_SEARCH_PROVIDER(inProvider);
	priv=self->priv;

	/* Set new match mode */
	priv->currentSortMode=priv->nextSortMode;

	/* To perform case-insensitive searches through model convert all search terms
	 * to lower-case before starting search.
	 * Remember that string list must be NULL terminated.
	 */
	numberTerms=g_strv_length((gchar**)inSearchTerms);
	if(numberTerms==0)
	{
		/* If we get here no search term is given, return no result set */
		return(NULL);
	}

	terms=g_new(gchar*, numberTerms+1);
	if(!terms)
	{
		g_critical(_("Could not allocate memory to copy search criteria for case-insensitive search"));
		return(NULL);
	}

	termsIter=terms;
	while(*inSearchTerms)
	{
		*termsIter=g_utf8_strdown(*inSearchTerms, -1);

		/* Move to next entry where to store lower-case and
		 * initialize with NULL for NULL termination of list.
		 */
		termsIter++;
		*termsIter=NULL;

		/* Move to next search term to convert to lower-case */
		inSearchTerms++;
	}

	/* Create empty result set to store matching result items */
	resultSet=xfdashboard_search_result_set_new();

	/* Perform search */
	for(iter=priv->allApps; iter; iter=g_list_next(iter))
	{
		/* Get app info to check for match */
		appInfo=XFDASHBOARD_DESKTOP_APP_INFO(iter->data);

		/* If desktop app info should be hidden then continue with next one */
		if(!g_app_info_should_show(G_APP_INFO(appInfo)))
		{
			continue;
		}

		/* Check for a match against search terms */
		score=_xfdashboard_applications_search_provider_score(self, terms, G_APP_INFO(appInfo));
		if(score>=0.0f)
		{
			GVariant									*resultItem;

			/* Create result item */
			resultItem=g_variant_new_string(g_app_info_get_id(G_APP_INFO(appInfo)));

			/* Add result item to result set */
			xfdashboard_search_result_set_add_item(resultSet, resultItem);
			xfdashboard_search_result_set_set_item_score(resultSet, resultItem, score);
		}
	}

	/* Sort result set */
	xfdashboard_search_result_set_set_sort_func_full(resultSet,
														_xfdashboard_applications_search_provider_sort_result_set,
														g_object_ref(self),
														g_object_unref);

	/* Release allocated resources */
	if(terms)
	{
		termsIter=terms;
		while(*termsIter)
		{
			g_free(*termsIter);
			termsIter++;
		}
		g_free(terms);
	}

	/* Return result set */
	return(resultSet);
}

/* Create actor for a result item of the result set returned from a search request */
static ClutterActor* _xfdashboard_applications_search_provider_create_result_actor(XfdashboardSearchProvider *inProvider,
																					GVariant *inResultItem)
{
	XfdashboardApplicationsSearchProvider			*self;
	XfdashboardApplicationsSearchProviderPrivate	*priv;
	ClutterActor									*actor;
	ClutterAction									*clickAction;
	ClutterAction									*dragAction;
	GAppInfo										*appInfo;

	g_return_val_if_fail(XFDASHBOARD_IS_APPLICATIONS_SEARCH_PROVIDER(inProvider), NULL);
	g_return_val_if_fail(inResultItem, NULL);

	self=XFDASHBOARD_APPLICATIONS_SEARCH_PROVIDER(inProvider);
	priv=self->priv;

	/* Get app info for result item */
	appInfo=xfdashboard_application_database_lookup_desktop_id(priv->appDB, g_variant_get_string(inResultItem, NULL));
	if(!appInfo) appInfo=xfdashboard_desktop_app_info_new_from_desktop_id(g_variant_get_string(inResultItem, NULL));
	if(!appInfo)
	{
		g_warning(_("Cannot create actor for desktop ID '%s' in result set of %s"),
					g_variant_get_string(inResultItem, NULL),
					G_OBJECT_TYPE_NAME(inProvider));
		return(NULL);
	}

	/* Create actor for result item */
	actor=xfdashboard_application_button_new_from_app_info(appInfo);
	clutter_actor_show(actor);

	clickAction=xfdashboard_click_action_new();
	g_signal_connect_swapped(clickAction, "clicked", G_CALLBACK(_xfdashboard_applications_search_provider_on_popup_menu), self);
	clutter_actor_add_action(actor, clickAction);

	dragAction=xfdashboard_drag_action_new();
	clutter_drag_action_set_drag_threshold(CLUTTER_DRAG_ACTION(dragAction), -1, -1);
	clutter_actor_add_action(actor, dragAction);
	g_signal_connect(dragAction, "drag-begin", G_CALLBACK(_xfdashboard_applications_search_provider_on_drag_begin), self);
	g_signal_connect(dragAction, "drag-end", G_CALLBACK(_xfdashboard_applications_search_provider_on_drag_end), self);

	/* Release allocated resources */
	g_object_unref(appInfo);

	/* Return created actor */
	return(actor);
}

/* Activate result item */
static gboolean _xfdashboard_applications_search_provider_activate_result(XfdashboardSearchProvider* inProvider,
																			GVariant *inResultItem,
																			ClutterActor *inActor,
																			const gchar **inSearchTerms)
{
	XfdashboardApplicationButton		*button;

	g_return_val_if_fail(XFDASHBOARD_IS_APPLICATIONS_SEARCH_PROVIDER(inProvider), FALSE);
	g_return_val_if_fail(XFDASHBOARD_IS_APPLICATION_BUTTON(inActor), FALSE);

	button=XFDASHBOARD_APPLICATION_BUTTON(inActor);

	/* Launch application */
	return(xfdashboard_application_button_execute(button, NULL));
}

/* IMPLEMENTATION: GObject */

/* Dispose this object */
static void _xfdashboard_applications_search_provider_dispose(GObject *inObject)
{
	XfdashboardApplicationsSearchProvider			*self=XFDASHBOARD_APPLICATIONS_SEARCH_PROVIDER(inObject);
	XfdashboardApplicationsSearchProviderPrivate	*priv=self->priv;

	/* Release allocated resouces */
	if(priv->appDB)
	{
		if(priv->applicationAddedID)
		{
			g_signal_handler_disconnect(priv->appDB, priv->applicationAddedID);
			priv->applicationAddedID=0;
		}

		if(priv->applicationRemovedID)
		{
			g_signal_handler_disconnect(priv->appDB, priv->applicationRemovedID);
			priv->applicationRemovedID=0;
		}

		g_object_unref(priv->appDB);
		priv->appDB=NULL;
	}

	if(priv->allApps)
	{
		g_list_free_full(priv->allApps, g_object_unref);
		priv->allApps=NULL;
	}

	if(priv->xfconfSortModeBindingID)
	{
		xfconf_g_property_unbind(priv->xfconfSortModeBindingID);
		priv->xfconfSortModeBindingID=0;
	}

	if(priv->xfconfChannel)
	{
		priv->xfconfChannel=NULL;
	}

	/* Call parent's class dispose method */
	G_OBJECT_CLASS(xfdashboard_applications_search_provider_parent_class)->dispose(inObject);
}

/* Set/get properties */
static void _xfdashboard_applications_search_provider_set_property(GObject *inObject,
																	guint inPropID,
																	const GValue *inValue,
																	GParamSpec *inSpec)
{
	XfdashboardApplicationsSearchProvider			*self=XFDASHBOARD_APPLICATIONS_SEARCH_PROVIDER(inObject);

	switch(inPropID)
	{
		case PROP_SORT_MODE:
			xfdashboard_applications_search_provider_set_sort_mode(self, g_value_get_flags(inValue));
			break;

		default:
			G_OBJECT_WARN_INVALID_PROPERTY_ID(inObject, inPropID, inSpec);
			break;
	}
}

static void _xfdashboard_applications_search_provider_get_property(GObject *inObject,
																	guint inPropID,
																	GValue *outValue,
																	GParamSpec *inSpec)
{
	XfdashboardApplicationsSearchProvider			*self=XFDASHBOARD_APPLICATIONS_SEARCH_PROVIDER(inObject);
	XfdashboardApplicationsSearchProviderPrivate	*priv=self->priv;

	switch(inPropID)
	{
		case PROP_SORT_MODE:
			g_value_set_flags(outValue, priv->nextSortMode);
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
static void xfdashboard_applications_search_provider_class_init(XfdashboardApplicationsSearchProviderClass *klass)
{
	XfdashboardSearchProviderClass		*providerClass=XFDASHBOARD_SEARCH_PROVIDER_CLASS(klass);
	GObjectClass						*gobjectClass=G_OBJECT_CLASS(klass);

	/* Override functions */
	gobjectClass->dispose=_xfdashboard_applications_search_provider_dispose;
	gobjectClass->set_property=_xfdashboard_applications_search_provider_set_property;
	gobjectClass->get_property=_xfdashboard_applications_search_provider_get_property;

	providerClass->initialize=_xfdashboard_applications_search_provider_initialize;
	providerClass->get_name=_xfdashboard_applications_search_provider_get_name;
	providerClass->get_icon=_xfdashboard_applications_search_provider_get_icon;
	providerClass->get_result_set=_xfdashboard_applications_search_provider_get_result_set;
	providerClass->create_result_actor=_xfdashboard_applications_search_provider_create_result_actor;
	providerClass->activate_result=_xfdashboard_applications_search_provider_activate_result;

	/* Define properties */
	XfdashboardApplicationsSearchProviderProperties[PROP_SORT_MODE]=
		g_param_spec_flags("sort-mode",
							_("Sort mode"),
							_("Defines how to sort matching applications"),
							XFDASHBOARD_TYPE_APPLICATIONS_SEARCH_PROVIDER_SORT_MODE,
							XFDASHBOARD_APPLICATIONS_SEARCH_PROVIDER_SORT_MODE_NONE,
							G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS);

	g_object_class_install_properties(gobjectClass, PROP_LAST, XfdashboardApplicationsSearchProviderProperties);
}

/* Object initialization
 * Create private structure and set up default values
 */
static void xfdashboard_applications_search_provider_init(XfdashboardApplicationsSearchProvider *self)
{
	XfdashboardApplicationsSearchProviderPrivate	*priv;

	self->priv=priv=xfdashboard_applications_search_provider_get_instance_private(self);

	/* Set up default values */
	priv->xfconfChannel=xfdashboard_application_get_xfconf_channel(NULL);
	priv->currentSortMode=XFDASHBOARD_APPLICATIONS_SEARCH_PROVIDER_SORT_MODE_NONE;
	priv->nextSortMode=XFDASHBOARD_APPLICATIONS_SEARCH_PROVIDER_SORT_MODE_NONE;

	/* Get application database */
	priv->appDB=xfdashboard_application_database_get_default();
	priv->applicationAddedID=g_signal_connect_swapped(priv->appDB,
														"application-added",
														G_CALLBACK(_xfdashboard_applications_search_provider_on_application_added),
														self);
	priv->applicationRemovedID=g_signal_connect_swapped(priv->appDB,
														"application-removed",
														G_CALLBACK(_xfdashboard_applications_search_provider_on_application_removed),
														self);

	/* Get list of all installed applications */
	priv->allApps=xfdashboard_application_database_get_all_applications(priv->appDB);

	/* Bind to xfconf to react on changes */
	priv->xfconfSortModeBindingID=
		xfconf_g_property_bind(priv->xfconfChannel,
								SORT_MODE_XFCONF_PROP,
								G_TYPE_UINT,
								self,
								"sort-mode");
}

/* IMPLEMENTATION: Public API */

/* Get/set sorting mode */
XfdashboardApplicationsSearchProviderSortMode xfdashboard_applications_search_provider_get_sort_mode(XfdashboardApplicationsSearchProvider *self)
{
	g_return_val_if_fail(XFDASHBOARD_IS_APPLICATIONS_SEARCH_PROVIDER(self), XFDASHBOARD_APPLICATIONS_SEARCH_PROVIDER_SORT_MODE_NONE);

	return(self->priv->nextSortMode);
}

void xfdashboard_applications_search_provider_set_sort_mode(XfdashboardApplicationsSearchProvider *self, const XfdashboardApplicationsSearchProviderSortMode inMode)
{
	XfdashboardApplicationsSearchProviderPrivate	*priv;

	g_return_if_fail(XFDASHBOARD_IS_APPLICATIONS_SEARCH_PROVIDER(self));

	priv=self->priv;

	/* Set value if changed */
	if(priv->nextSortMode!=inMode)
	{
		/* Set value */
		priv->nextSortMode=inMode;

		/* Notify about property change */
		g_object_notify_by_pspec(G_OBJECT(self), XfdashboardApplicationsSearchProviderProperties[PROP_SORT_MODE]);
	}
}
