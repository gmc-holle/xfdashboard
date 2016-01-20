/*
 * applications-search-provider: Search provider for searching installed
 *                               applications
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

#include "applications-search-provider.h"

#include <glib/gi18n-lib.h>
#include <gtk/gtk.h>
#include <errno.h>

#include "application-database.h"
#include "application-button.h"
#include "application.h"
#include "drag-action.h"
#include "utils.h"

/* Define this class in GObject system */
G_DEFINE_TYPE(XfdashboardApplicationsSearchProvider,
				xfdashboard_applications_search_provider,
				XFDASHBOARD_TYPE_SEARCH_PROVIDER)

/* Private structure - access only by public API if needed */
#define XFDASHBOARD_APPLICATIONS_SEARCH_PROVIDER_GET_PRIVATE(obj) \
	(G_TYPE_INSTANCE_GET_PRIVATE((obj), XFDASHBOARD_TYPE_APPLICATIONS_SEARCH_PROVIDER, XfdashboardApplicationsSearchProviderPrivate))

struct _XfdashboardApplicationsSearchProviderPrivate
{
	/* Instance related */
	XfdashboardApplicationDatabase		*appDB;
	guint								applicationAddedID;
	guint								applicationRemovedID;

	GList								*allApps;
};

/* IMPLEMENTATION: Private variables and methods */
#define DEFAULT_DELIMITERS														"\t\n\r "

#define XFDASHBOARD_APPLICATIONS_SEARCH_PROVIDER_APPDATA_STATE_FILE				"app-datas-state"
#define XFDASHBOARD_APPLICATIONS_SEARCH_PROVIDER_APPDATA_LAUNCH_COUNT_GROUP		"Launch Counts"

G_LOCK_DEFINE_STATIC(_xfdashboard_applications_search_provider_app_datas_lock);
static GHashTable*		_xfdashboard_applications_search_provider_app_datas=NULL;
static gchar*			_xfdashboard_applications_search_provider_app_datas_filename=NULL;
static guint			_xfdashboard_applications_search_provider_app_datas_shutdownSignalID=0;
static guint			_xfdashboard_applications_search_provider_app_datas_applicationLaunchedSignalID=0;

typedef struct _XfdashboardApplicationsSearchProviderAppData		XfdashboardApplicationsSearchProviderAppData;
struct _XfdashboardApplicationsSearchProviderAppData
{
	gint								refCount;

	guint								launchCounter;
};

/* Create, destroy, ref and unref tag data for a tag */
static XfdashboardApplicationsSearchProviderAppData* _xfdashboard_applications_search_provider_app_data_new(void)
{
	XfdashboardApplicationsSearchProviderAppData	*data;

	/* Create data for app-data */
	data=g_new0(XfdashboardApplicationsSearchProviderAppData, 1);
	if(!data) return(NULL);

	/* Set up app-data */
	data->refCount=1;

	return(data);
}

static void _xfdashboard_applications_search_provider_app_data_free(XfdashboardApplicationsSearchProviderAppData *inData)
{
	g_return_if_fail(inData);

	/* Release common allocated resources */
	g_free(inData);
}

static XfdashboardApplicationsSearchProviderAppData* _xfdashboard_applications_search_provider_app_data_ref(XfdashboardApplicationsSearchProviderAppData *inData)
{
	g_return_val_if_fail(inData, NULL);

	inData->refCount++;
	return(inData);
}

static void _xfdashboard_applications_search_provider_app_data_unref(XfdashboardApplicationsSearchProviderAppData *inData)
{
	g_return_if_fail(inData);

	inData->refCount--;
	if(inData->refCount==0) _xfdashboard_applications_search_provider_app_data_free(inData);
}

/* An application was launched successfully */
static void _xfdashboard_applications_search_provider_on_application_launched(XfdashboardApplication *inApplication,
																				GAppInfo *inAppInfo,
																				gpointer inUserData)
{
	const gchar										*appID;
	XfdashboardApplicationsSearchProviderAppData	*appData;

	g_return_if_fail(G_IS_APP_INFO(inAppInfo));

	/* Lock for thread-safety */
	G_LOCK(_xfdashboard_applications_search_provider_app_datas_lock);

	/* Get application ID which is used to lookup and store app-datas */
	appID=g_app_info_get_id(inAppInfo);

	/* Create new app-data if application is new, otherwise take an extra
	 * reference on app-data to keep it alive as it will be removed and
	 * re-added when updating and the removal may decrease the reference
	 * counter to zero which destroys the app-data.
	 */
	if(!g_hash_table_lookup_extended(_xfdashboard_applications_search_provider_app_datas, appID, NULL, (gpointer*)&appData))
	{
		appData=_xfdashboard_applications_search_provider_app_data_new();
	}
		else
		{
			_xfdashboard_applications_search_provider_app_data_ref(appData);
		}

	/* Increase launch counter */
	appData->launchCounter++;

	/* Store updated app-data */
	g_hash_table_insert(_xfdashboard_applications_search_provider_app_datas, g_strdup(appID), _xfdashboard_applications_search_provider_app_data_ref(appData));

	/* Release extra reference we took to keep this app-data alive */
	_xfdashboard_applications_search_provider_app_data_unref(appData);

	/* Unlock for thread-safety */
	G_UNLOCK(_xfdashboard_applications_search_provider_app_datas_lock);
}

/* Destroy app-datas for this search provider */
static void _xfdashboard_applications_search_provider_destroy_app_datas(void)
{
	XfdashboardApplication								*application;

	/* Only existing app-datas can be destroyed */
	if(!_xfdashboard_applications_search_provider_app_datas) return;

	/* Lock for thread-safety */
	G_LOCK(_xfdashboard_applications_search_provider_app_datas_lock);

	/* Get application instance */
	application=xfdashboard_application_get_default();

	/* Disconnect application "shutdown" signal handler */
	g_signal_handler_disconnect(application, _xfdashboard_applications_search_provider_app_datas_shutdownSignalID);
	_xfdashboard_applications_search_provider_app_datas_shutdownSignalID=0;

	/* Disconnect application "application-launched" signal handler */
	g_signal_handler_disconnect(application, _xfdashboard_applications_search_provider_app_datas_applicationLaunchedSignalID);
	_xfdashboard_applications_search_provider_app_datas_applicationLaunchedSignalID=0;

	/* Save app-datas to state file */
	if(_xfdashboard_applications_search_provider_app_datas_filename)
	{
		GKeyFile										*keyFile;
		gchar											*keyFileData;
		gsize											keyFileLength;
		GHashTableIter									iter;
		gchar											*appID;
		XfdashboardApplicationsSearchProviderAppData	*appData;
		GError											*error;
		gchar											*fileFolder;

		/* Create parent folders for key file if not available */
		fileFolder=g_path_get_dirname(_xfdashboard_applications_search_provider_app_datas_filename);

		if(g_mkdir_with_parents(fileFolder, 0700)<0)
		{
			int											errno_save;

			/* Get error code */
			errno_save=errno;

			/* Show error message */
			g_critical(_("Could not create folders to store app-datas of applications search provider to %s: %s"),
						_xfdashboard_applications_search_provider_app_datas_filename,
						g_strerror(errno_save));
		}

		/* Create and set up key file for app-datas */
		keyFile=g_key_file_new();

		g_hash_table_iter_init(&iter, _xfdashboard_applications_search_provider_app_datas);
		while(g_hash_table_iter_next(&iter, (gpointer*)&appID, (gpointer*)&appData))
		{
			g_key_file_set_uint64(keyFile,
									XFDASHBOARD_APPLICATIONS_SEARCH_PROVIDER_APPDATA_LAUNCH_COUNT_GROUP,
									appID,
									appData->launchCounter);
		}

		/* Store key file for app-datas */
		error=NULL;

		keyFileData=g_key_file_to_data(keyFile, &keyFileLength, NULL);
		if(!g_file_set_contents(_xfdashboard_applications_search_provider_app_datas_filename, keyFileData, keyFileLength, &error))
		{
			g_critical(_("Failed to save app-datas of applications search provider to %s: %s"),
						_xfdashboard_applications_search_provider_app_datas_filename,
						error ? error->message : _("Unknown error"));
			if(error) g_error_free(error);
		}

		/* Release allocated resources */
		if(fileFolder) g_free(fileFolder);
		if(keyFileData) g_free(keyFileData);
		if(keyFile) g_key_file_free(keyFile);
	}

	/* Destroy app-datas */
	g_debug("Destroying app-datas of applications search provider");
	g_hash_table_destroy(_xfdashboard_applications_search_provider_app_datas);
	_xfdashboard_applications_search_provider_app_datas=NULL;

	/* Destroy filename for app-datas */
	if(_xfdashboard_applications_search_provider_app_datas_filename)
	{
		g_free(_xfdashboard_applications_search_provider_app_datas_filename);
		_xfdashboard_applications_search_provider_app_datas_filename=NULL;
	}

	/* Unlock for thread-safety */
	G_UNLOCK(_xfdashboard_applications_search_provider_app_datas_lock);
}

/* Create and load app-datas for this search provider if not done already */
static void _xfdashboard_applications_search_provider_create_app_datas(XfdashboardApplicationsSearchProvider *self)
{
	XfdashboardApplication									*application;

	g_return_if_fail(XFDASHBOARD_IS_APPLICATIONS_SEARCH_PROVIDER(self));

	/* App-datas were already set up */
	if(_xfdashboard_applications_search_provider_app_datas) return;

	g_assert(!_xfdashboard_applications_search_provider_app_datas_shutdownSignalID);
	g_assert(!_xfdashboard_applications_search_provider_app_datas_applicationLaunchedSignalID);

	/* Lock for thread-safety */
	G_LOCK(_xfdashboard_applications_search_provider_app_datas_lock);

	/* Load app-datas from state file */
	_xfdashboard_applications_search_provider_app_datas_filename=xfdashboard_get_data_path(self, XFDASHBOARD_APPLICATIONS_SEARCH_PROVIDER_APPDATA_STATE_FILE);
	if(!_xfdashboard_applications_search_provider_app_datas_filename)
	{
		/* Show error message */
		g_critical(_("Could not get file name for app-datas of applications search provider"));

		/* Unlock for thread-safety */
		G_UNLOCK(_xfdashboard_applications_search_provider_app_datas_lock);

		return;
	}

	/* Create hash-table for app-datas */
	_xfdashboard_applications_search_provider_app_datas=
		g_hash_table_new_full(g_str_hash,
								g_str_equal,
								g_free,
								(GDestroyNotify)_xfdashboard_applications_search_provider_app_data_unref);
	g_debug("Created app-datas of applications search provider");

	/* Load app-datas and store into hash-table */
	if(g_file_test(_xfdashboard_applications_search_provider_app_datas_filename, G_FILE_TEST_EXISTS | G_FILE_TEST_IS_REGULAR))
	{
		GKeyFile											*keyFile;
		GError												*error;
		gchar												*startGroup;
		gchar												**keys;

		error=NULL;
		startGroup=NULL;
		keys=NULL;

		/* Load app-datas state file */
		keyFile=g_key_file_new();
		if(!g_key_file_load_from_file(keyFile, _xfdashboard_applications_search_provider_app_datas_filename, G_KEY_FILE_NONE, &error))
		{
			/* Show error message and release error */
			g_critical(_("Could not load app-datas state file of applications search provider at %s: %s"),
						_xfdashboard_applications_search_provider_app_datas_filename,
						error ? error->message : _("Unknown error"));
			if(error)
			{
				g_error_free(error);
				error=NULL;
			}

			/* Release key file to stop further processing */
			g_key_file_free(keyFile);
			keyFile=NULL;
		}

		/* Get keys to lookup in app-datas state file */
		if(keyFile)
		{
			startGroup=g_key_file_get_start_group(keyFile);
			if(!startGroup)
			{
				g_critical(_("Could get list of app-datas from state file of applications search provider at %s"),
							_xfdashboard_applications_search_provider_app_datas_filename);

				/* Release key file to stop further processing */
				g_key_file_free(keyFile);
				keyFile=NULL;
			}
		}

		if(keyFile)
		{
			keys=g_key_file_get_keys(keyFile, startGroup, NULL, &error);
			if(!keys)
			{
				g_critical(_("Could get list of app-datas from state file of applications search provider at %s"),
							_xfdashboard_applications_search_provider_app_datas_filename);

				/* Release key file to stop further processing */
				g_key_file_free(keyFile);
				keyFile=NULL;
			}
		}

		/* Read app-datas from state file */
		if(keyFile)
		{
			gchar											**iter;
			gchar											*appID;
			XfdashboardApplicationsSearchProviderAppData	*appData;

			for(iter=keys; *iter; iter++)
			{
				/* Get application ID */
				appID=*iter;

				/* Set up application data for application ID */
				appData=_xfdashboard_applications_search_provider_app_data_new();
				appData->launchCounter=g_key_file_get_uint64(keyFile,
																XFDASHBOARD_APPLICATIONS_SEARCH_PROVIDER_APPDATA_LAUNCH_COUNT_GROUP,
																appID,
																NULL);

				/* Store application data into hash-table */
				g_hash_table_insert(_xfdashboard_applications_search_provider_app_datas, g_strdup(appID), _xfdashboard_applications_search_provider_app_data_ref(appData));

				/* Release application data */
				_xfdashboard_applications_search_provider_app_data_unref(appData);
			}

			g_debug("Loaded %d app-data entries from '%s' at applications search provider",
						g_hash_table_size(_xfdashboard_applications_search_provider_app_datas),
						_xfdashboard_applications_search_provider_app_datas_filename);
		}

		/* Release allocated resources */
		if(keys) g_strfreev(keys);
		if(startGroup) g_free(startGroup);
		if(keyFile) g_key_file_free(keyFile);
	}

	/* Get application instance */
	application=xfdashboard_application_get_default();

	/* Connect to "shutdown" signal of application to clean up app-datas */
	_xfdashboard_applications_search_provider_app_datas_shutdownSignalID=
		g_signal_connect(application,
							"shutdown-final",
							G_CALLBACK(_xfdashboard_applications_search_provider_destroy_app_datas),
							NULL);

	/* Connect to "application-launched" signal of application to track app launches */
	_xfdashboard_applications_search_provider_app_datas_applicationLaunchedSignalID=
		g_signal_connect(application,
							"application-launched",
							G_CALLBACK(_xfdashboard_applications_search_provider_on_application_launched),
							NULL);

	/* Unlock for thread-safety */
	G_UNLOCK(_xfdashboard_applications_search_provider_app_datas_lock);
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

/* Check if given app info matches search terms */
static gboolean _xfdashboard_applications_search_provider_is_match(XfdashboardApplicationsSearchProvider *self,
																	gchar **inSearchTerms,
																	GAppInfo *inAppInfo)
{
	gboolean						isMatch;
	gchar							*title;
	gchar							*description;
	const gchar						*command;
	const gchar						*value;
	gint							matchesFound, matchesExpected;

	g_return_val_if_fail(XFDASHBOARD_IS_APPLICATIONS_SEARCH_PROVIDER(self), FALSE);
	g_return_val_if_fail(G_IS_APP_INFO(inAppInfo), FALSE);

	isMatch=FALSE;

	/* Empty search term matches no menu item */
	if(!inSearchTerms) return(FALSE);

	matchesExpected=g_strv_length(inSearchTerms);
	if(matchesExpected==0) return(FALSE);

	/* Check if title, description or command matches all search terms */
	value=g_app_info_get_display_name(inAppInfo);
	if(value) title=g_utf8_strdown(value, -1);
		else title=NULL;

	value=g_app_info_get_description(inAppInfo);
	if(value) description=g_utf8_strdown(value, -1);
		else description=NULL;

	command=g_app_info_get_executable(inAppInfo);

	matchesFound=0;
	while(*inSearchTerms)
	{
		gboolean						termMatch;
		gchar							*commandPos;

		/* Reset "found" indicator */
		termMatch=FALSE;

		/* Check for current search term */
		if(!termMatch &&
			title &&
			g_strstr_len(title, -1, *inSearchTerms))
		{
			termMatch=TRUE;
		}

		if(!termMatch &&
			description &&
			g_strstr_len(description, -1, *inSearchTerms))
		{
			termMatch=TRUE;
		}

		if(!termMatch && command)
		{
			commandPos=g_strstr_len(command, -1, *inSearchTerms);
			if(commandPos &&
				(commandPos==command || *(commandPos-1)==G_DIR_SEPARATOR))
			{
				termMatch=TRUE;
			}
		}

		/* Increase match counter if we found a match */
		if(termMatch) matchesFound++;

		/* Continue with next search term */
		inSearchTerms++;
	}

	if(matchesFound>=matchesExpected) isMatch=TRUE;

	/* Release allocated resources */
	if(description) g_free(description);
	if(title) g_free(title);

	/* If we get here return TRUE to show model data item or FALSE to hide */
	return(isMatch);
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

	/* Create and load app-data hash-table (will only be done once) */
	_xfdashboard_applications_search_provider_create_app_datas(XFDASHBOARD_APPLICATIONS_SEARCH_PROVIDER(inProvider));
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
	GVariant											*resultItem;
	XfdashboardDesktopAppInfo							*appInfo;

	g_return_val_if_fail(XFDASHBOARD_IS_APPLICATIONS_SEARCH_PROVIDER(inProvider), NULL);

	self=XFDASHBOARD_APPLICATIONS_SEARCH_PROVIDER(inProvider);
	priv=self->priv;

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
		g_critical(_("Could not allocate memory to copy search criterias for case-insensitive search"));
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
		if(xfdashboard_desktop_app_info_get_hidden(appInfo) ||
			xfdashboard_desktop_app_info_get_nodisplay(appInfo))
		{
			continue;
		}

		/* Get result item */
		resultItem=g_variant_new_string(g_app_info_get_id(G_APP_INFO(appInfo)));

		/* Check if current app info is in previous result set and should be checked
		 * if a previous result set is provided.
		 */
		if(!inPreviousResultSet ||
			xfdashboard_search_result_set_has_item(inPreviousResultSet, resultItem))
		{
			/* Check for a match against search terms */
			if(_xfdashboard_applications_search_provider_is_match(self, terms, G_APP_INFO(appInfo)))
			{
				xfdashboard_search_result_set_add_item(resultSet, g_variant_ref(resultItem));
			}
		}

		/* Release allocated resources */
		g_variant_unref(resultItem);
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

	/* Call parent's class dispose method */
	G_OBJECT_CLASS(xfdashboard_applications_search_provider_parent_class)->dispose(inObject);
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

	providerClass->initialize=_xfdashboard_applications_search_provider_initialize;
	providerClass->get_name=_xfdashboard_applications_search_provider_get_name;
	providerClass->get_icon=_xfdashboard_applications_search_provider_get_icon;
	providerClass->get_result_set=_xfdashboard_applications_search_provider_get_result_set;
	providerClass->create_result_actor=_xfdashboard_applications_search_provider_create_result_actor;
	providerClass->activate_result=_xfdashboard_applications_search_provider_activate_result;

	/* Set up private structure */
	g_type_class_add_private(klass, sizeof(XfdashboardApplicationsSearchProviderPrivate));
}

/* Object initialization
 * Create private structure and set up default values
 */
static void xfdashboard_applications_search_provider_init(XfdashboardApplicationsSearchProvider *self)
{
	XfdashboardApplicationsSearchProviderPrivate	*priv;

	self->priv=priv=XFDASHBOARD_APPLICATIONS_SEARCH_PROVIDER_GET_PRIVATE(self);

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
}
