/*
 * application-database: A singelton managing desktop files and menus
 *                       for installed applications
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

#include <libxfdashboard/application-database.h>
#include <libxfdashboard/desktop-app-info.h>
#include <libxfdashboard/compat.h>
#include <libxfdashboard/debug.h>


/* Define this class in GObject system */
struct _XfdashboardApplicationDatabasePrivate
{
	/* Properties related */
	gboolean			isLoaded;

	/* Instance related */
	GList				*searchPaths;

	GarconMenu			*appsMenu;
	guint				appsMenuReloadRequiredID;

	GHashTable			*applications;
	GList				*appDirMonitors;
};

G_DEFINE_TYPE_WITH_PRIVATE(XfdashboardApplicationDatabase,
							xfdashboard_application_database,
							G_TYPE_OBJECT)

/* Properties */
enum
{
	PROP_0,

	PROP_IS_LOADED,

	PROP_LAST
};

static GParamSpec* XfdashboardApplicationDatabaseProperties[PROP_LAST]={ 0, };

/* Signals */
enum
{
	SIGNAL_MENU_RELOAD_REQUIRED,

	SIGNAL_APPLICATION_ADDED,
	SIGNAL_APPLICATION_REMOVED,

	SIGNAL_LAST
};

static guint XfdashboardApplicationDatabaseSignals[SIGNAL_LAST]={ 0, };

/* IMPLEMENTATION: Private variables and methods */

/* Single instance of application database */
static XfdashboardApplicationDatabase*		_xfdashboard_application_database=NULL;

typedef struct _XfdashboardApplicationDatabaseFileMonitorData	XfdashboardApplicationDatabaseFileMonitorData;
struct _XfdashboardApplicationDatabaseFileMonitorData
{
	GFile				*path;
	GFileMonitor		*monitor;
	guint				changedID;
};

/* Forward declarations */
static gboolean _xfdashboard_application_database_load_application_menu(XfdashboardApplicationDatabase *self, GError **outError);

/* Callback function for hash table iterator to add each value to a list of type GList */
static void _xfdashboard_application_database_add_hashtable_item_to_list(gpointer inKey,
																			gpointer inValue,
																			gpointer inUserData)
{
	GList									**applicationsList;

	g_return_if_fail(XFDASHBOARD_DESKTOP_APP_INFO(inValue));

	applicationsList=(GList**)inUserData;

	/* Add a reference of value (a XfdashboardDesktopAppInfo) to list */
	*applicationsList=g_list_prepend(*applicationsList, g_object_ref(G_OBJECT(inValue)));
}

/* Application menu needs to be reloaded */
static void _xfdashboard_application_database_on_application_menu_reload_required(XfdashboardApplicationDatabase *self,
																					gpointer inUserData)
{
	GarconMenu		*menu;
	GError			*error;

	g_return_if_fail(XFDASHBOARD_IS_APPLICATION_DATABASE(self));
	g_return_if_fail(GARCON_IS_MENU(inUserData));

	menu=GARCON_MENU(inUserData);
	error=NULL;

	/* Reload application menu. This also emits all necessary signals. */
	XFDASHBOARD_DEBUG(self, APPLICATIONS,
						"Menu '%s' changed and requires a reload of application menu",
						garcon_menu_element_get_name(GARCON_MENU_ELEMENT(menu)));
	if(!_xfdashboard_application_database_load_application_menu(self, &error))
	{
		g_critical(_("Could not reload application menu: %s"),
					error ? error->message : _("Unknown error"));

		/* Release allocated resources */
		if(error) g_error_free(error);
	}
}

/* Create a new data structure for file monitor */
static XfdashboardApplicationDatabaseFileMonitorData* _xfdashboard_application_database_monitor_data_new(GFile *inPath)
{
	XfdashboardApplicationDatabaseFileMonitorData	*monitorData;

	g_return_val_if_fail(G_IS_FILE(inPath), NULL);

	/* Allocate memory for file monitor data structure */
	monitorData=g_new0(XfdashboardApplicationDatabaseFileMonitorData, 1);
	if(!monitorData) return(NULL);

	monitorData->path=g_object_ref(inPath);

	/* Return newly create and initialized file monitor data structure */
	return(monitorData);
}

/* Free a file monitor data structure */
static void _xfdashboard_application_database_monitor_data_free(XfdashboardApplicationDatabaseFileMonitorData *inData)
{
	g_return_if_fail(inData);

	/* Release each data in file monitor structure */
	if(inData->path) g_object_unref(inData->path);
	if(inData->monitor)
	{
		if(inData->changedID)
		{
			g_signal_handler_disconnect(inData->monitor, inData->changedID);
		}

		g_object_unref(inData->monitor);
	}

	/* Release allocated memory */
	g_free(inData);
}

/* Find file monitor data structure */
static XfdashboardApplicationDatabaseFileMonitorData* _xfdashboard_application_database_monitor_data_find_by_monitor(XfdashboardApplicationDatabase *self,
																														GFileMonitor *inMonitor)
{
	XfdashboardApplicationDatabasePrivate			*priv;
	GList											*iter;
	XfdashboardApplicationDatabaseFileMonitorData	*iterData;

	g_return_val_if_fail(XFDASHBOARD_IS_APPLICATION_DATABASE(self), NULL);
	g_return_val_if_fail(G_IS_FILE_MONITOR(inMonitor), NULL);

	priv=self->priv;

	/* Iterate through list of registered file monitor data structure and
	 * lookup the one matching requested file monitor.
	 */
	for(iter=priv->appDirMonitors; iter; iter=g_list_next(iter))
	{
		iterData=(XfdashboardApplicationDatabaseFileMonitorData*)iter->data;
		if(iterData &&
			iterData->monitor &&
			iterData->monitor==inMonitor)
		{
			return(iterData);
		}
	}

	/* If we get here we could not find a file monitor data structure
	 * matching the requested file monitor so return NULL.
	 */
	return(NULL);
}

static XfdashboardApplicationDatabaseFileMonitorData* _xfdashboard_application_database_monitor_data_find_by_file(XfdashboardApplicationDatabase *self,
																													GFile *inPath)
{
	XfdashboardApplicationDatabasePrivate			*priv;
	GList											*iter;
	XfdashboardApplicationDatabaseFileMonitorData	*iterData;

	g_return_val_if_fail(XFDASHBOARD_IS_APPLICATION_DATABASE(self), NULL);
	g_return_val_if_fail(G_IS_FILE(inPath), NULL);

	priv=self->priv;

	/* Iterate through list of registered file monitor data structure and
	 * lookup the one matching requested path.
	 */
	for(iter=priv->appDirMonitors; iter; iter=g_list_next(iter))
	{
		iterData=(XfdashboardApplicationDatabaseFileMonitorData*)iter->data;
		if(iterData &&
			iterData->path &&
			g_file_equal(iterData->path, inPath))
		{
			return(iterData);
		}
	}

	/* If we get here we could not find a file monitor data structure
	 * matching the requested path so return NULL.
	 */
	return(NULL);
}

/* A directory containing desktop files has changed */
static void _xfdashboard_application_database_on_file_monitor_changed(XfdashboardApplicationDatabase *self,
																		GFile *inFile,
																		GFile *inOtherFile,
																		GFileMonitorEvent inEventType,
																		gpointer inUserData)
{
	XfdashboardApplicationDatabasePrivate				*priv;
	GFileMonitor										*monitor;
	XfdashboardApplicationDatabaseFileMonitorData		*monitorData;
	gchar												*filePath;

	g_return_if_fail(XFDASHBOARD_IS_APPLICATION_DATABASE(self));
	g_return_if_fail(G_IS_FILE_MONITOR(inUserData));

	priv=self->priv;
	monitor=G_FILE_MONITOR(inUserData);

	/* Find file monitor data structure of file monitor emitting this signal */
	monitorData=_xfdashboard_application_database_monitor_data_find_by_monitor(self, monitor);
	if(!monitorData)
	{
		g_warning(_("Received event from unknown file monitor"));
		return;
	}

	/* Get file path */
	filePath=g_file_get_path(inFile);

	/* Check if a new directory was created */
	if(inEventType==G_FILE_MONITOR_EVENT_CREATED &&
		g_file_query_file_type(inFile, G_FILE_QUERY_INFO_NONE, NULL)==G_FILE_TYPE_DIRECTORY)
	{
		XfdashboardApplicationDatabaseFileMonitorData	*fileMonitorData;
		GError											*error;

		error=NULL;

		XFDASHBOARD_DEBUG(self, APPLICATIONS,
							"Directory '%s' in application search paths was created",
							filePath);

		/* A new directory was created so create a file monitor for it,
		 * connect signals and add to list of registered file monitors.
		 */
		fileMonitorData=_xfdashboard_application_database_monitor_data_new(inFile);
		if(!fileMonitorData)
		{
			g_warning(_("Unable to create file monitor for newly created directory '%s'"), filePath);

			/* Release allocated resources */
			if(filePath) g_free(filePath);

			return;
		}

		fileMonitorData->monitor=g_file_monitor(inFile, G_FILE_MONITOR_NONE, NULL, &error);
		if(!fileMonitorData->monitor)
		{
			g_warning(_("Unable to create file monitor for '%s': %s"),
						filePath,
						error ? error->message : _("Unknown error"));

			/* Release allocated resources */
			if(fileMonitorData) _xfdashboard_application_database_monitor_data_free(fileMonitorData);
			if(filePath) g_free(filePath);

			return;
		}

		fileMonitorData->changedID=g_signal_connect_swapped(fileMonitorData->monitor,
															"changed",
															G_CALLBACK(_xfdashboard_application_database_on_file_monitor_changed),
															self);

		priv->appDirMonitors=g_list_prepend(priv->appDirMonitors, fileMonitorData);
	}

	/* Check if a new desktop file was created */
	if(inEventType==G_FILE_MONITOR_EVENT_CREATED &&
		g_file_query_file_type(inFile, G_FILE_QUERY_INFO_NONE, NULL)==G_FILE_TYPE_REGULAR)
	{
		/* Handle only desktop files but that can only be done
		 * if a hash table exists.
		 */
		if(g_str_has_suffix(filePath, ".desktop") &&
			priv->applications)
		{
			gchar										*desktopID;

			XFDASHBOARD_DEBUG(self, APPLICATIONS,
								"Desktop file '%s' in application search paths was created",
								filePath);

			/* Get desktop ID to check */
			desktopID=xfdashboard_application_database_get_desktop_id_from_file(inFile);

			/* If we found the desktop ID for the desktop file created check
			 * if it would replace an existing desktop ID in hash table or
			 * if it is a completely new one.
			 */
			if(desktopID)
			{
				XfdashboardDesktopAppInfo				*currentDesktopAppInfo;

				/* Get current desktop app info */
				if(g_hash_table_lookup_extended(priv->applications, desktopID, NULL, (gpointer*)&currentDesktopAppInfo))
				{
					gchar								*newDesktopFilename;
					GFile								*newDesktopFile;

					/* Check if newly created desktop file would replace current one.
					 * Now as the desktop file is created and exists we can call the
					 * function which searches the first matching desktop file for
					 * this desktop ID. If it is the same as the file created then
					 * it replaces the current desktop app info. Otherwise ignore this
					 * newly created file because id does not affect the hash table.
					 */
					newDesktopFilename=xfdashboard_application_database_get_file_from_desktop_id(desktopID);
					newDesktopFile=g_file_new_for_path(newDesktopFilename);
					if(g_file_equal(newDesktopFile, inFile))
					{
						/* Newly created desktop file replaces current one. Set
						 * new file at desktop app info to emit 'changed' signal.
						 * No need to change anything at hash table.
						 */
						g_object_set(currentDesktopAppInfo, "file", newDesktopFile, NULL);

						XFDASHBOARD_DEBUG(self, APPLICATIONS,
											"Replacing known desktop ID '%s' at desktop file '%s' with new desktop file '%s'",
											desktopID,
											filePath,
											newDesktopFilename);
					}
						else
						{
							XFDASHBOARD_DEBUG(self, APPLICATIONS,
												"Ignoring new desktop file at '%s' for known desktop ID '%s'",
												filePath,
												desktopID);
						}

					/* Release allocated resources */
					if(newDesktopFile) g_object_unref(newDesktopFile);
					if(newDesktopFilename) g_free(newDesktopFilename);
				}
					else
					{
						XfdashboardDesktopAppInfo		*newDesktopAppInfo;

						/* There is no desktop app info for this desktop ID in
						 * hash table. So it is completely new. Create a new
						 * desktop app info, add it to hash table and emit the
						 * signal for application added.
						 */
						newDesktopAppInfo=XFDASHBOARD_DESKTOP_APP_INFO(g_object_new(XFDASHBOARD_TYPE_DESKTOP_APP_INFO,
																					"desktop-id", desktopID,
																					"file", inFile,
																					NULL));

						if(xfdashboard_desktop_app_info_is_valid(newDesktopAppInfo))
						{
							/* Add desktop app info to hash table because creation
							 * was successful.
							 */
							g_hash_table_insert(priv->applications, g_strdup(desktopID), newDesktopAppInfo);

							/* Emit signal that an application has been removed from hash table */
							g_signal_emit(self, XfdashboardApplicationDatabaseSignals[SIGNAL_APPLICATION_ADDED], 0, newDesktopAppInfo);

							XFDASHBOARD_DEBUG(self, APPLICATIONS,
												"Adding new desktop ID '%s' for new desktop file at '%s'",
												desktopID,
												filePath);
						}
							else
							{
								XFDASHBOARD_DEBUG(self, APPLICATIONS,
													"Adding new desktop ID '%s' for new desktop file '%s' failed",
													desktopID,
													filePath);

								/* Release allocated resources */
								g_object_unref(newDesktopAppInfo);
							}
					}
			}
		}
	}

	/* Check if a file was modified */
	if(inEventType==G_FILE_MONITOR_EVENT_CHANGED &&
		g_file_query_file_type(inFile, G_FILE_QUERY_INFO_NONE, NULL)==G_FILE_TYPE_REGULAR)
	{
		/* Handle only desktop files but that can only be done
		 * if a hash table exists.
		 */
		if(g_str_has_suffix(filePath, ".desktop") &&
			priv->applications)
		{
			gchar										*desktopID;
			XfdashboardDesktopAppInfo					*appInfo;

			XFDASHBOARD_DEBUG(self, APPLICATIONS,
								"Desktop file '%s' was modified",
								filePath);

			/* Get desktop ID to check */
			desktopID=xfdashboard_application_database_get_desktop_id_from_file(inFile);

			/* Get current desktop app info in hash table for desktop ID */
			appInfo=NULL;
			if(desktopID)
			{
				/* We have to check if the modified file is exactly the same
				 * as the file used by current desktop app info for desktop ID.
				 * Because a desktop file could be created which overrides
				 * this desktop ID so the changes are not important anymore
				 * and we do not need to emit 'changed' signal.
				 * If a desktop file was added or removed the other handlers
				 * have or will emit the needed signal to keep them up-to-date.
				 */
				if(g_hash_table_lookup_extended(priv->applications, desktopID, NULL, (gpointer*)&appInfo))
				{
					GFile								*appInfoFile;
					gchar								*appInfoFilename;

					/* Get file of desktop app info found */
					appInfoFile=xfdashboard_desktop_app_info_get_file(appInfo);
					appInfoFilename=g_file_get_path(appInfoFile);

					/* If file of desktop app info is the same as the one modified
					 * then reload desktop app info.
					 */
					if(g_file_equal(appInfoFile, inFile))
					{
						/* Reload desktop app info but remove desktop app info
						 * if reload failed or is invalid after reload.
						 */
						if(!xfdashboard_desktop_app_info_reload(appInfo) ||
							!xfdashboard_desktop_app_info_is_valid(appInfo))
						{
							/* Take a extra reference of desktop app info which is
							 * going to be removed from hash table because when removing
							 * desktop ID from hash table could release the last reference
							 * to desktop app info and destroy it. So we are not able
							 * to send object when emitting signal 'application-removed'.
							 */
							g_object_ref(appInfo);

							/* Remove desktop app info from hash table because either
							 * reload failed or it is invalid now.
							 */
							g_hash_table_remove(priv->applications, desktopID);

							XFDASHBOARD_DEBUG(self, APPLICATIONS,
												"Removed desktop ID '%s' with origin desktop file '%s' with modified desktop file '%s' because reload failed or it is invalid",
												desktopID,
												filePath,
												appInfoFilename);

							/* Emit signal that an application has been removed */
							g_signal_emit(self, XfdashboardApplicationDatabaseSignals[SIGNAL_APPLICATION_REMOVED], 0, appInfo);

							/* Release extra reference we took on desktop app info and
							 * let object maybe destroyed now because it was the last one.
							 */
							g_object_unref(appInfo);
						}
							else
							{
								XFDASHBOARD_DEBUG(self, APPLICATIONS,
													"Reloaded desktop ID '%s' with origin desktop file '%s' with modified desktop file '%s'",
													desktopID,
													filePath,
													appInfoFilename);
							}
					}

					/* Release allocated resources */
					if(appInfoFilename) g_free(appInfoFilename);
				}
					else
					{
						XfdashboardDesktopAppInfo		*newDesktopAppInfo;

						/* We have a valid desktop ID but no app info in hash table
						 * so try to create a desktop app info for modified file
						 * and if it is valid add it to hash table and emit the
						 * signal for application added.
						 */
						newDesktopAppInfo=XFDASHBOARD_DESKTOP_APP_INFO(g_object_new(XFDASHBOARD_TYPE_DESKTOP_APP_INFO,
																					"desktop-id", desktopID,
																					"file", inFile,
																					NULL));
						if(xfdashboard_desktop_app_info_is_valid(newDesktopAppInfo))
						{
							g_hash_table_insert(priv->applications, g_strdup(desktopID), newDesktopAppInfo);

							/* Emit signal that an application has been removed from hash table */
							g_signal_emit(self, XfdashboardApplicationDatabaseSignals[SIGNAL_APPLICATION_ADDED], 0, newDesktopAppInfo);

							XFDASHBOARD_DEBUG(self, APPLICATIONS,
												"Adding new desktop ID '%s' for modified desktop file at '%s'",
												desktopID,
												filePath);
						}
							else
							{
								XFDASHBOARD_DEBUG(self, APPLICATIONS,
													"Got valid desktop id '%s' but invalid desktop app info for file '%s'",
													desktopID,
													filePath);
								g_object_unref(newDesktopAppInfo);
							}
					}
			}

			/* Release allocated resources */
			if(desktopID) g_free(desktopID);
		}
	}

	/* Check if a file or directory was removed.
	 * The problem here is that we cannot determine if the removed file
	 * is really a file or a directory because we cannot query the file type
	 * because it is removed at filesystem. So assume it was both:
	 * a file and a directory.
	 */
	if(inEventType==G_FILE_MONITOR_EVENT_DELETED)
	{
		XfdashboardApplicationDatabaseFileMonitorData	*fileMonitorData;

		/* Assume it was a directory so remove file monitor and free
		 * it's file monitor data structure.
		 */
		fileMonitorData=_xfdashboard_application_database_monitor_data_find_by_file(self, inFile);
		if(fileMonitorData)
		{
			XFDASHBOARD_DEBUG(self, APPLICATIONS,
								"Removing file monitor for deleted directory '%s'",
								filePath);

			priv->appDirMonitors=g_list_remove_all(priv->appDirMonitors, fileMonitorData);
			_xfdashboard_application_database_monitor_data_free(fileMonitorData);
			fileMonitorData=NULL;
		}

		/* Assume it was a file so check if it was a desktop file, then check
		 * if any other desktop file in the other paths in list of search paths
		 * matches the removed desktop ID. If so replace desktop app info and
		 * emit signal. Otherwise remove desktop app info from hash table and
		 * emit signal. Check can only be performed if a hash table exists.
		 */
		if(g_str_has_suffix(filePath, ".desktop") &&
			priv->applications)
		{
			gchar										*desktopID;

			XFDASHBOARD_DEBUG(self, APPLICATIONS,
								"Desktop file '%s' was removed",
								filePath);

			/* Get desktop ID to check */
			desktopID=xfdashboard_application_database_get_desktop_id_from_file(inFile);

			/* If we found the desktop ID for the desktop file removed, check if
			 * another desktop file in search paths could replace desktop ID or
			 * if it was removed completely.
			 */
			if(desktopID)
			{
				XfdashboardDesktopAppInfo				*currentDesktopAppInfo;

				/* Get current desktop app info */
				if(g_hash_table_lookup_extended(priv->applications, desktopID, NULL, (gpointer*)&currentDesktopAppInfo))
				{
					/* Check if there is another desktop file which could
					 * replace the desktop ID.
					 */
					gchar								*newDesktopFilename;

					newDesktopFilename=xfdashboard_application_database_get_file_from_desktop_id(desktopID);
					if(newDesktopFilename)
					{
						GFile							*newDesktopFile;

						XFDASHBOARD_DEBUG(self, APPLICATIONS,
											"Replacing known desktop ID '%s' at desktop file '%s' with new desktop file '%s'",
											desktopID,
											filePath,
											newDesktopFilename);

						/* There is another desktop file which could replace the
						 * desktop ID. Set new file at desktop app info which causes
						 * the 'changed' signal to be emitted. No need to change hash table.
						 */
						newDesktopFile=g_file_new_for_path(newDesktopFilename);
						g_object_set(currentDesktopAppInfo, "file", newDesktopFile, NULL);
						g_object_unref(newDesktopFile);

						/* Release allocated resources */
						g_free(newDesktopFilename);
					}
						else
						{
							/* Take a reference on current desktop app info to be able
							 * to emit a 'changed' signal and 'application-removed' signal
							 * when setting the new file at it after it has been removed
							 * from hash table. Otherwise the last reference at this object
							 * could be release which causes the object to be destroyed.
							 */
							g_object_ref(currentDesktopAppInfo);

							/* There is no other desktop file for this desktop ID.
							 * Remove desktop app info and desktop ID from hash table.
							 */
							g_hash_table_remove(priv->applications, desktopID);

							XFDASHBOARD_DEBUG(self, APPLICATIONS,
												"Removing desktop ID '%s'",
												desktopID);

							/* Emit signal that an application has been removed */
							g_signal_emit(self, XfdashboardApplicationDatabaseSignals[SIGNAL_APPLICATION_REMOVED], 0, currentDesktopAppInfo);

							/* Set a NULL file at desktop app info which causes
							 * the 'changed' signal to be emitted.
							 */
							g_object_set(currentDesktopAppInfo, "file", NULL, NULL);

							/* Release the extra reference we took */
							g_object_unref(currentDesktopAppInfo);
						}
				}

				/* Release allocated resources */
				g_free(desktopID);
			}
		}
	}

	/* Release allocated resources */
	if(filePath) g_free(filePath);
}

/* Load installed and user-overidden application desktop files */
static gboolean _xfdashboard_application_database_load_applications_recursive(XfdashboardApplicationDatabase *self,
																				GFile *inTopLevelPath,
																				GFile *inCurrentPath,
																				GHashTable **ioDesktopAppInfos,
																				GList **ioFileMonitors,
																				GError **outError)
{
	XfdashboardApplicationDatabasePrivate			*priv G_GNUC_UNUSED;
	gchar											*topLevelPath;
	gchar											*path;
	GFileEnumerator									*enumerator;
	GFileInfo										*info;
	XfdashboardApplicationDatabaseFileMonitorData	*monitorData;
	GError											*error;

	g_return_val_if_fail(XFDASHBOARD_IS_APPLICATION_DATABASE(self), FALSE);
	g_return_val_if_fail(G_IS_FILE(inTopLevelPath), FALSE);
	g_return_val_if_fail(G_IS_FILE(inCurrentPath), FALSE);
	g_return_val_if_fail(ioDesktopAppInfos && *ioDesktopAppInfos, FALSE);
	g_return_val_if_fail(ioFileMonitors, FALSE);

	priv=self->priv;
	error=NULL;

	/* Get path of current path to scan for desktop files */
	path=g_file_get_path(inCurrentPath);
	topLevelPath=g_file_get_path(inTopLevelPath);

	XFDASHBOARD_DEBUG(self, APPLICATIONS,
						"Scanning directory '%s' for search path '%s'",
						path,
						topLevelPath);

	/* Create enumerator for current path to iterate through path and
	 * searching for desktop files.
	 */
	enumerator=g_file_enumerate_children(inCurrentPath,
											G_FILE_ATTRIBUTE_STANDARD_TYPE "," G_FILE_ATTRIBUTE_STANDARD_NAME,
											G_FILE_QUERY_INFO_NONE,
											NULL,
											&error);
	if(!enumerator)
	{
		/* Propagate error */
		g_propagate_error(outError, error);

		/* Release allocated resources */
		if(path) g_free(path);
		if(topLevelPath) g_free(topLevelPath);

		return(FALSE);
	}

	/* Iterate through files in current path recursively and for each
	 * desktop file found create a desktop app info object but only
	 * if it is the first occurence of that desktop ID in hash table.
	 */
	while((info=g_file_enumerator_next_file(enumerator, NULL, &error)))
	{
		/* If current file is a directory call this function recursively
		 * with absolute path to directory.
		 */
		if(g_file_info_get_file_type(info)==G_FILE_TYPE_DIRECTORY)
		{
			GFile							*childPath;
			gboolean						childSuccess;

			XFDASHBOARD_DEBUG(self, APPLICATIONS,
								"Suspend scanning directory '%s' at search path '%s' for sub-directory '%s'",
								path,
								topLevelPath,
								g_file_info_get_name(info));

			childPath=g_file_resolve_relative_path(inCurrentPath,
													g_file_info_get_name(info));
			if(!childPath)
			{
				XFDASHBOARD_DEBUG(self, APPLICATIONS,
									"Unable to build path to search for desktop files for path='%s' and file='%s'",
									path,
									g_file_info_get_name(info));

				/* Set error */
				g_set_error(outError,
								G_IO_ERROR,
								G_IO_ERROR_FAILED,
								_("Unable to build path to '%s%s%s' to search for desktop files"),
								path,
								G_DIR_SEPARATOR_S,
								g_file_info_get_name(info));

				/* Release allocated resources */
				if(path) g_free(path);
				if(topLevelPath) g_free(topLevelPath);
				if(enumerator) g_object_unref(enumerator);

				return(FALSE);
			}

			childSuccess=_xfdashboard_application_database_load_applications_recursive(self,
																						inTopLevelPath,
																						childPath,
																						ioDesktopAppInfos,
																						ioFileMonitors,
																						&error);
			if(!childSuccess)
			{
				XFDASHBOARD_DEBUG(self, APPLICATIONS,
									"Unable to iterate desktop files at %s%s%s",
									path,
									G_DIR_SEPARATOR_S,
									g_file_info_get_name(info));

				/* Propagate error */
				g_propagate_error(outError, error);

				/* Release allocated resources */
				if(childPath) g_object_unref(childPath);
				if(path) g_free(path);
				if(topLevelPath) g_free(topLevelPath);
				if(enumerator) g_object_unref(enumerator);

				return(FALSE);
			}

			/* Release allocated resources */
			if(childPath) g_object_unref(childPath);

			XFDASHBOARD_DEBUG(self, APPLICATIONS,
								"Resume scanning directory '%s' at search path '%s'",
								path,
								topLevelPath);
		}

		/* If current file is a regular file and if it is a desktop file
		 * then create desktop app info object if it is the first occurence
		 * of this desktop ID in hash table.
		 */
		if(g_file_info_get_file_type(info)==G_FILE_TYPE_REGULAR &&
			g_str_has_suffix(g_file_info_get_name(info), ".desktop"))
		{
			const gchar								*childName;
			GFile									*childFile;
			gchar									*desktopID;

			/* Get file of current enumerated file */
			childName=g_file_info_get_name(info);
			childFile=g_file_get_child(g_file_enumerator_get_container(enumerator), childName);

			/* Determine desktop ID for file */
			desktopID=g_file_get_relative_path(inTopLevelPath, childFile);
			if(desktopID)
			{
				gchar								*iter;

				iter=desktopID;
				while(*iter)
				{
					/* Replace directory sepearator with dash if needed */
					if(*iter==G_DIR_SEPARATOR) *iter='-';

					/* Continue with next character in desktop ID */
					iter++;
				}
			}
				else
				{
					g_warning(_("Could not determine desktop ID for '%s'"),
								g_file_info_get_name(info));
				}

			/* If no desktop app info with determined desktop ID exists
			 * then create it now.
			 */
			if(desktopID &&
				!g_hash_table_lookup_extended(*ioDesktopAppInfos,
												desktopID,
												NULL,
												NULL))
			{
				XfdashboardDesktopAppInfo			*appInfo;

				appInfo=XFDASHBOARD_DESKTOP_APP_INFO(g_object_new(XFDASHBOARD_TYPE_DESKTOP_APP_INFO,
																	"desktop-id", desktopID,
																	"file", childFile,
																	NULL));
				if(xfdashboard_desktop_app_info_is_valid(appInfo))
				{
					g_hash_table_insert(*ioDesktopAppInfos, g_strdup(desktopID), g_object_ref(appInfo));

					XFDASHBOARD_DEBUG(self, APPLICATIONS,
										"Found desktop file '%s%s%s' with desktop ID '%s' at search path '%s'",
										path,
										G_DIR_SEPARATOR_S,
										childName,
										desktopID,
										topLevelPath);
				}
					else
					{
						/* Although desktop file for desktop ID is invalid, add
						 * it to the database to prevent that a valid desktop file
						 * for the same desktop ID will be found at path of lower
						 * prioritory which will then be store in the database as
						 * no entry exists. The first entry found - valid or invalid -
						 * has the highest priority. Later the caller has to ensure
						 * that all invalid desktop IDs in the database will be removed.
						 */
						g_hash_table_insert(*ioDesktopAppInfos, g_strdup(desktopID), g_object_ref(appInfo));

						XFDASHBOARD_DEBUG(self, APPLICATIONS,
											"Adding and mark invalid desktop file '%s%s%s' with desktop ID '%s' at search path '%s'",
											path,
											G_DIR_SEPARATOR_S,
											childName,
											desktopID,
											topLevelPath);
					}

				g_object_unref(appInfo);
			}

			/* Release allocated resources */
			if(desktopID) g_free(desktopID);
			if(childFile) g_object_unref(childFile);
		}

		g_object_unref(info);
	}

	if(error)
	{
		/* Propagate error */
		g_propagate_error(outError, error);

		/* Release allocated resources */
		if(path) g_free(path);
		if(topLevelPath) g_free(topLevelPath);
		if(enumerator) g_object_unref(enumerator);

		return(FALSE);
	}

	/* Iterating through given path was successful so create file monitor
	 * for this path.
	 */
	monitorData=_xfdashboard_application_database_monitor_data_new(inCurrentPath);
	if(!monitorData)
	{
		XFDASHBOARD_DEBUG(self, APPLICATIONS,
							"Failed to create data object for file monitor for path '%s'",
							path);

		/* Set error */
		g_set_error(outError,
						G_IO_ERROR,
						G_IO_ERROR_FAILED,
						_("Unable to create file monitor for '%s'"),
						path);

		/* Release allocated resources */
		if(path) g_free(path);
		if(topLevelPath) g_free(topLevelPath);
		if(enumerator) g_object_unref(enumerator);

		return(FALSE);
	}

	monitorData->monitor=g_file_monitor(inCurrentPath, G_FILE_MONITOR_NONE, NULL, &error);
	if(!monitorData->monitor && error)
	{
#if defined(__unix__)
		/* Workaround for FreeBSD with Glib bug (file/directory monitors cannot be created) */
		g_warning(_("[workaround for FreeBSD] Cannot initialize file monitor for path '%s' but will not fail: %s"),
					path,
					error ? error->message : _("Unknown error"));

		/* Clear error as this error will not fail at FreeBSD */
		g_clear_error(&error);
#else
		XFDASHBOARD_DEBUG(self, APPLICATIONS,
							"Failed to initialize file monitor for path '%s'",
							path);

		/* Propagate error */
		g_propagate_error(outError, error);

		/* Release allocated resources */
		if(monitorData) _xfdashboard_application_database_monitor_data_free(monitorData);

		if(path) g_free(path);
		if(topLevelPath) g_free(topLevelPath);
		if(enumerator) g_object_unref(enumerator);

		return(FALSE);
#endif
	}

	/* If file monitor could be created, add it to list of file monitors ... */
	if(monitorData && monitorData->monitor)
	{
		*ioFileMonitors=g_list_prepend(*ioFileMonitors, monitorData);

		XFDASHBOARD_DEBUG(self, APPLICATIONS,
							"Added file monitor for path '%s'",
							path);
	}
		/* ... otherwise free file monitor data */
		else
		{
			if(monitorData) _xfdashboard_application_database_monitor_data_free(monitorData);

			XFDASHBOARD_DEBUG(self, APPLICATIONS,
								"Destroying file monitor for path '%s'",
								path);
		}

	XFDASHBOARD_DEBUG(self, APPLICATIONS,
						"Finished scanning directory '%s' for search path '%s'",
						path,
						topLevelPath);

	/* Release allocated resources */
	if(path) g_free(path);
	if(topLevelPath) g_free(topLevelPath);
	if(enumerator) g_object_unref(enumerator);

	/* Return success result */
	return(TRUE);
}

static gboolean _xfdashboard_application_database_load_applications(XfdashboardApplicationDatabase *self, GError **outError)
{
	XfdashboardApplicationDatabasePrivate			*priv;
	GHashTable										*apps;
	GList											*fileMonitors;
	GError											*error;
	GList											*iter;
	XfdashboardApplicationDatabaseFileMonitorData	*monitorData;

	g_return_val_if_fail(XFDASHBOARD_IS_APPLICATION_DATABASE(self), FALSE);
	g_return_val_if_fail(outError && *outError==NULL, FALSE);

	priv=self->priv;
	error=NULL;

	/* Iterate through enumerated files at each path in list of search paths
	 * and add only the first occurence of each desktop ID. Also set up
	 * file monitors to get notified if a desktop file changes, was removed
	 * or a new one added.
	 */
	fileMonitors=NULL;
	apps=g_hash_table_new_full(g_str_hash,
								g_str_equal,
								g_free,
								g_object_unref);

	for(iter=priv->searchPaths; iter; iter=g_list_next(iter))
	{
		const gchar							*path;
		GFile								*directory;

		/* Iterate through files in current search path recursively and
		 * for each desktop file create a desktop app info object but only
		 * if it is the first occurence of that desktop ID.
		 */
		path=(const gchar*)iter->data;
		directory=g_file_new_for_path(path);

		/* Only scan current search path if path exists and is a directory.
		 * Otherwise the called function will fail and then this function
		 * will fail also. But not all search path must exist so check.
		 */
		if(g_file_query_file_type(directory, G_FILE_QUERY_INFO_NONE, NULL)==G_FILE_TYPE_DIRECTORY &&
			!_xfdashboard_application_database_load_applications_recursive(self, directory, directory, &apps, &fileMonitors, &error))
		{
			/* Propagate error */
			g_propagate_error(outError, error);

			/* Release allocated resources */
			if(fileMonitors) g_list_free_full(fileMonitors, g_object_unref);
			if(apps) g_hash_table_unref(apps);
			if(directory) g_object_unref(directory);

			return(FALSE);
		}

		if(directory) g_object_unref(directory);
	}

	/* Remove invalid desktop IDs from database */
	if(apps)
	{
		GHashTableIter								appsIter;
		const gchar									*desktopID;
		XfdashboardDesktopAppInfo					*appInfo;

		g_hash_table_iter_init(&appsIter, apps);
		while(g_hash_table_iter_next(&appsIter, (gpointer*)&desktopID, (gpointer*)&appInfo))
		{
			/* If value for key is invalid then remove this entry now */
			if(!xfdashboard_desktop_app_info_is_valid(appInfo))
			{
				XFDASHBOARD_DEBUG(self, APPLICATIONS,
									"Removing invalid desktop ID '%s' from application database",
									desktopID);

				/* Remove entry from hash table via the iterator */
				g_hash_table_iter_remove(&appsIter);
			}
		}
	}

	XFDASHBOARD_DEBUG(self, APPLICATIONS,
						"Loaded %u applications desktop files",
						g_hash_table_size(apps));

	/* Release old list of installed applications and set new one */
	if(priv->applications)
	{
		g_hash_table_unref(priv->applications);
		priv->applications=NULL;
	}

	priv->applications=apps;

	/* Release old list of installed applications and set new one.
	 * Now ee can also connect signals to all file monitors created.
	 */
	if(priv->appDirMonitors)
	{
		for(iter=priv->appDirMonitors; iter; iter=g_list_next(iter))
		{
			_xfdashboard_application_database_monitor_data_free((XfdashboardApplicationDatabaseFileMonitorData*)iter->data);
		}
		g_list_free(priv->appDirMonitors);
		priv->appDirMonitors=NULL;
	}

	priv->appDirMonitors=fileMonitors;
	for(iter=priv->appDirMonitors; iter; iter=g_list_next(iter))
	{
		monitorData=(XfdashboardApplicationDatabaseFileMonitorData*)iter->data;
		if(monitorData->monitor)
		{
			monitorData->changedID=g_signal_connect_swapped(monitorData->monitor,
														"changed",
														G_CALLBACK(_xfdashboard_application_database_on_file_monitor_changed),
														self);
		}
	}

	/* Desktop files were loaded successfully */
	return(TRUE);
}

/* Load menus and applications */
static gboolean _xfdashboard_application_database_load_application_menu(XfdashboardApplicationDatabase *self, GError **outError)
{
	XfdashboardApplicationDatabasePrivate	*priv;
	GarconMenu								*appsMenu;
	GError									*error;

	g_return_val_if_fail(XFDASHBOARD_IS_APPLICATION_DATABASE(self), FALSE);
	g_return_val_if_fail(outError && *outError==NULL, FALSE);

	priv=self->priv;
	error=NULL;

	/* Load menu */
	appsMenu=garcon_menu_new_applications();
	if(!garcon_menu_load(appsMenu, NULL, &error))
	{
		/* Propagate error */
		g_propagate_error(outError, error);

		/* Release allocated resources */
		g_object_unref(appsMenu);

		return(FALSE);
	}
	XFDASHBOARD_DEBUG(self, APPLICATIONS,
						"Loaded application menu '%s'",
						garcon_menu_element_get_name(GARCON_MENU_ELEMENT(appsMenu)));

	/* Release old menus and set new one */
	if(priv->appsMenu)
	{
		if(priv->appsMenuReloadRequiredID)
		{
			g_signal_handler_disconnect(priv->appsMenu, priv->appsMenuReloadRequiredID);
			priv->appsMenuReloadRequiredID=0;
		}

		g_object_unref(priv->appsMenu);
		priv->appsMenu=NULL;
	}

	priv->appsMenu=appsMenu;
	priv->appsMenuReloadRequiredID=g_signal_connect_swapped(priv->appsMenu,
																	"reload-required",
																	G_CALLBACK(_xfdashboard_application_database_on_application_menu_reload_required),
																	self);

	/* Emit signal 'menu-reload-required' */
	g_signal_emit(self, XfdashboardApplicationDatabaseSignals[SIGNAL_MENU_RELOAD_REQUIRED], 0);

	/* Menu was loaded successfully */
	return(TRUE);
}

/* Release all allocated resources of this object and clean up */
static void _xfdashboard_application_database_clean(XfdashboardApplicationDatabase *self)
{
	XfdashboardApplicationDatabasePrivate					*priv;

	g_return_if_fail(XFDASHBOARD_IS_APPLICATION_DATABASE(self));

	priv=self->priv;

	/* Release allocated resources */
	if(priv->appDirMonitors)
	{
		GList								*iter;

		for(iter=priv->appDirMonitors; iter; iter=g_list_next(iter))
		{
			_xfdashboard_application_database_monitor_data_free((XfdashboardApplicationDatabaseFileMonitorData*)iter->data);
		}
		g_list_free(priv->appDirMonitors);
		priv->appDirMonitors=NULL;
	}

	if(priv->appsMenu)
	{
		if(priv->appsMenuReloadRequiredID)
		{
			g_signal_handler_disconnect(priv->appsMenu, priv->appsMenuReloadRequiredID);
			priv->appsMenuReloadRequiredID=0;
		}

		g_object_unref(priv->appsMenu);
		priv->appsMenu=NULL;
	}

	if(priv->applications)
	{
		g_hash_table_unref(priv->applications);
		priv->applications=NULL;
	}

	/* Now as all allocated resources are released, this database is not loaded anymore */
	priv->isLoaded=FALSE;

	/* Notify about property change */
	g_object_notify_by_pspec(G_OBJECT(self), XfdashboardApplicationDatabaseProperties[PROP_IS_LOADED]);
}

/* Callback function to check for duplicate search path entries used in object instance initialization */
static void _xfdashboard_application_database_check_search_path_duplicate(gpointer inData, gpointer inUserData)
{
	const gchar		*searchPath;
	gchar			**pathLookup;

	g_return_if_fail(inData);
	g_return_if_fail(inUserData);

	searchPath=(const gchar*)inData;
	pathLookup=(gchar**)inUserData;

	/* Check if both paths are equal then reset pointer to pointer containing
	 * search path to lookup to NULL indicating a duplicate entry.
	 */
	if(g_strcmp0(*pathLookup, searchPath)==0) *pathLookup=NULL;
}


/* IMPLEMENTATION: GObject */

/* Construct this object */
static GObject* _xfdashboard_application_database_constructor(GType inType,
																guint inNumberConstructParams,
																GObjectConstructParam *inConstructParams)
{
	GObject									*object;

	if(!_xfdashboard_application_database)
	{
		object=G_OBJECT_CLASS(xfdashboard_application_database_parent_class)->constructor(inType, inNumberConstructParams, inConstructParams);
		_xfdashboard_application_database=XFDASHBOARD_APPLICATION_DATABASE(object);
	}
		else
		{
			object=g_object_ref(G_OBJECT(_xfdashboard_application_database));
		}

	return(object);
}

/* Dispose this object */
static void _xfdashboard_application_database_dispose(GObject *inObject)
{
	XfdashboardApplicationDatabase			*self=XFDASHBOARD_APPLICATION_DATABASE(inObject);
	XfdashboardApplicationDatabasePrivate	*priv=self->priv;

	/* Release allocated resources */
	_xfdashboard_application_database_clean(self);

	if(priv->searchPaths)
	{
		g_list_free_full(priv->searchPaths, g_free);
		priv->searchPaths=NULL;
	}

	/* Call parent's class dispose method */
	G_OBJECT_CLASS(xfdashboard_application_database_parent_class)->dispose(inObject);
}

/* Finalize this object */
static void _xfdashboard_application_database_finalize(GObject *inObject)
{
	/* Release allocated resources finally, e.g. unset singleton */
	if(G_LIKELY(G_OBJECT(_xfdashboard_application_database)==inObject))
	{
		_xfdashboard_application_database=NULL;
	}

	/* Call parent's class dispose method */
	G_OBJECT_CLASS(xfdashboard_application_database_parent_class)->finalize(inObject);
}

/* Set/get properties */
static void _xfdashboard_application_database_get_property(GObject *inObject,
															guint inPropID,
															GValue *outValue,
															GParamSpec *inSpec)
{
	XfdashboardApplicationDatabase			*self=XFDASHBOARD_APPLICATION_DATABASE(inObject);
	XfdashboardApplicationDatabasePrivate	*priv=self->priv;;

	switch(inPropID)
	{
		case PROP_IS_LOADED:
			g_value_set_boolean(outValue, priv->isLoaded);
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
static void xfdashboard_application_database_class_init(XfdashboardApplicationDatabaseClass *klass)
{
	GObjectClass		*gobjectClass=G_OBJECT_CLASS(klass);

	/* Override functions */
	gobjectClass->constructor=_xfdashboard_application_database_constructor;
	gobjectClass->dispose=_xfdashboard_application_database_dispose;
	gobjectClass->finalize=_xfdashboard_application_database_finalize;
	gobjectClass->get_property=_xfdashboard_application_database_get_property;

	/* Define properties */
	XfdashboardApplicationDatabaseProperties[PROP_IS_LOADED]=
		g_param_spec_boolean("is-loaded",
								_("Is loaded"),
								_("Flag indicating if application database has been initialized and loaded successfully"),
								FALSE,
								G_PARAM_READABLE | G_PARAM_STATIC_STRINGS);

	/* Define signals */
	XfdashboardApplicationDatabaseSignals[SIGNAL_MENU_RELOAD_REQUIRED]=
		g_signal_new("menu-reload-required",
						G_TYPE_FROM_CLASS(klass),
						G_SIGNAL_RUN_LAST | G_SIGNAL_NO_HOOKS,
						G_STRUCT_OFFSET(XfdashboardApplicationDatabaseClass, menu_reload_required),
						NULL,
						NULL,
						g_cclosure_marshal_VOID__VOID,
						G_TYPE_NONE,
						0);

	XfdashboardApplicationDatabaseSignals[SIGNAL_APPLICATION_ADDED]=
		g_signal_new("application-added",
						G_TYPE_FROM_CLASS(klass),
						G_SIGNAL_RUN_LAST | G_SIGNAL_NO_HOOKS,
						G_STRUCT_OFFSET(XfdashboardApplicationDatabaseClass, application_added),
						NULL,
						NULL,
						g_cclosure_marshal_VOID__OBJECT,
						G_TYPE_NONE,
						1,
						G_TYPE_APP_INFO);

	XfdashboardApplicationDatabaseSignals[SIGNAL_APPLICATION_REMOVED]=
		g_signal_new("application-removed",
						G_TYPE_FROM_CLASS(klass),
						G_SIGNAL_RUN_LAST | G_SIGNAL_NO_HOOKS,
						G_STRUCT_OFFSET(XfdashboardApplicationDatabaseClass, application_removed),
						NULL,
						NULL,
						g_cclosure_marshal_VOID__OBJECT,
						G_TYPE_NONE,
						1,
						G_TYPE_APP_INFO);
}

/* Object initialization
 * Create private structure and set up default values
 */
static void xfdashboard_application_database_init(XfdashboardApplicationDatabase *self)
{
	XfdashboardApplicationDatabasePrivate	*priv;
	const gchar* const						*systemPaths;
	gchar									*path;

	priv=self->priv=xfdashboard_application_database_get_instance_private(self);

	/* Set default values */
	priv->isLoaded=FALSE;
	priv->searchPaths=NULL;
	priv->appsMenu=NULL;
	priv->appsMenuReloadRequiredID=0;
	priv->applications=NULL;
	priv->appDirMonitors=NULL;

	/* Set up search paths but eliminate duplicates */
	path=g_build_filename(g_get_user_data_dir(), "applications", NULL);
	priv->searchPaths=g_list_append(priv->searchPaths, path);
	XFDASHBOARD_DEBUG(self, APPLICATIONS,
						"Added search path '%s' to application database",
						path);

	systemPaths=g_get_system_data_dirs();
	while(*systemPaths)
	{
		gchar								*searchPathUnique;

		/* Build path to applications directory and add to list of search paths */
		path=g_build_filename(*systemPaths, "applications", NULL);

		/* Check for duplicate search path by calling a callback functions
		 * for each search path already in list. The callback function expects
		 * a pointer to a pointer containing the search path to lookup. If
		 * the callback find a search path matching the requested one
		 * the pointer to the pointer will be set to NULL.
		 */
		searchPathUnique=path;
		g_list_foreach(priv->searchPaths, _xfdashboard_application_database_check_search_path_duplicate, &searchPathUnique);
		if(searchPathUnique)
		{
			priv->searchPaths=g_list_append(priv->searchPaths, g_strdup(searchPathUnique));
			XFDASHBOARD_DEBUG(self, APPLICATIONS,
								"Added search path '%s' to application database",
								searchPathUnique);
		}

		/* Release allocated resources */
		g_free(path);

		/* Continue with next system path */
		systemPaths++;
	}
}

/* IMPLEMENTATION: Public API */

/* Get single instance of application */
XfdashboardApplicationDatabase* xfdashboard_application_database_get_default(void)
{
	GObject									*singleton;

	singleton=g_object_new(XFDASHBOARD_TYPE_APPLICATION_DATABASE, NULL);
	return(XFDASHBOARD_APPLICATION_DATABASE(singleton));
}

/* Determine if menu and applications has been loaded successfully */
gboolean xfdashboard_application_database_is_loaded(const XfdashboardApplicationDatabase *self)
{
	g_return_val_if_fail(XFDASHBOARD_IS_APPLICATION_DATABASE(self), FALSE);

	return(self->priv->isLoaded);
}

/* Load menu and applications */
gboolean xfdashboard_application_database_load(XfdashboardApplicationDatabase *self, GError **outError)
{
	XfdashboardApplicationDatabasePrivate	*priv;
	GError									*error;

	g_return_val_if_fail(XFDASHBOARD_IS_APPLICATION_DATABASE(self), FALSE);
	g_return_val_if_fail(outError && *outError==NULL, FALSE);

	priv=self->priv;
	error=NULL;

	/* Load menu */
	if(!_xfdashboard_application_database_load_application_menu(self, &error))
	{
		/* Propagate error */
		g_propagate_error(outError, error);

		/* Clean up this instance by releasing all allocated resources */
		_xfdashboard_application_database_clean(self);

		return(FALSE);
	}

	/* Load installed and user-overidden application desktop files */
	if(!_xfdashboard_application_database_load_applications(self, &error))
	{
		/* Propagate error */
		g_propagate_error(outError, error);

		/* Clean up this instance by releasing all allocated resources */
		_xfdashboard_application_database_clean(self);

		return(FALSE);
	}

	/* Loading was successful */
	priv->isLoaded=TRUE;

	/* Notify about property change */
	g_object_notify_by_pspec(G_OBJECT(self), XfdashboardApplicationDatabaseProperties[PROP_IS_LOADED]);

	/* Loading was successful so return TRUE here */
	return(TRUE);
}

/* Get list of search paths where desktop files could be stored at */
const GList* xfdashboard_application_database_get_application_search_paths(const XfdashboardApplicationDatabase *self)
{
	g_return_val_if_fail(XFDASHBOARD_IS_APPLICATION_DATABASE(self), NULL);

	return(self->priv->searchPaths);
}

/* Get application menu.
 * The return object has to be freed with g_object_unref().
 */
GarconMenu* xfdashboard_application_database_get_application_menu(XfdashboardApplicationDatabase *self)
{
	g_return_val_if_fail(XFDASHBOARD_IS_APPLICATION_DATABASE(self), NULL);

	if(self->priv->appsMenu)
	{
		return(GARCON_MENU(g_object_ref(self->priv->appsMenu)));
	}

	/* If we get here no application menu is available, so return NULL */
	return(NULL);
}

/* Get list of all installed applications in database */
GList* xfdashboard_application_database_get_all_applications(XfdashboardApplicationDatabase *self)
{
	XfdashboardApplicationDatabasePrivate	*priv;
	GList									*applicationsList;

	g_return_val_if_fail(XFDASHBOARD_IS_APPLICATION_DATABASE(self), NULL);

	priv=self->priv;
	applicationsList=NULL;

	/* Add each item in hash table to list */
	if(priv->applications)
	{
		g_hash_table_foreach(priv->applications,
								_xfdashboard_application_database_add_hashtable_item_to_list,
								&applicationsList);

		applicationsList=g_list_reverse(applicationsList);
	}

	/* Return list of installed applications */
	return(applicationsList);
}

/* Get GAppInfo for desktop ID from cache which was built while iterating
 * through search paths and scanning for desktop files.
 * If a GAppInfo object for desktop ID was found the return object has to
 * be freed with g_object_unref().
 * If a GAppInfo object for desktop ID was not found, NULL will be returned.
 */
GAppInfo* xfdashboard_application_database_lookup_desktop_id(XfdashboardApplicationDatabase *self,
																const gchar *inDesktopID)
{
	XfdashboardApplicationDatabasePrivate	*priv;
	gpointer								appInfo;

	g_return_val_if_fail(XFDASHBOARD_IS_APPLICATION_DATABASE(self), NULL);
	g_return_val_if_fail(inDesktopID && *inDesktopID, NULL);

	priv=self->priv;

	/* Lookup desktop ID in cache */
	if(priv->applications &&
		g_hash_table_lookup_extended(priv->applications, inDesktopID, NULL, &appInfo))
	{
		g_object_ref(G_OBJECT(appInfo));
		return(G_APP_INFO(appInfo));
	}

	/* Desktop ID not found, so return NULL */
	return(NULL);
}

/* Get path to desktop file for requested desktop ID.
 * Returns NULL if no desktop file at any search path can be found.
 */
gchar* xfdashboard_application_database_get_file_from_desktop_id(const gchar *inDesktopID)
{
	XfdashboardApplicationDatabase	*appDB;
	const GList						*searchPaths;
	gchar							*foundDesktopFile;

	g_return_val_if_fail(inDesktopID && *inDesktopID, NULL);
	g_return_val_if_fail(g_str_has_suffix(inDesktopID, ".desktop"), NULL);

	/* Find the desktop file for a desktop ID isn't as easy as it sounds.
	 * Especially if the desktop file contains at least one dash. The dash
	 * could either be part of the desktop file's file name or is a
	 * directory seperator or it is part of the directory name where to
	 * continue to lookup the desktop file.
	 * So we have to perform a lot of check to find (hopefully) the right
	 * desktop file for requested desktop ID. We also let matching file
	 * name win over matching directory names and shorter directory names
	 * over longer ones.
	 *
	 * 1.) Check if a file name with desktop ID and the suffix ".desktop"
	 *     exists at current search path. If it does return it as result.
	 * 2.) Split desktop ID into parts at the dashes. Check if a directory
	 *     with the first part exists. If it does not exists append the
	 *     next part with a dash to the first part and check if a directory
	 *     with this name exists. If necessary repeat until all parts are
	 *     consumed. If no directory was found get next search path and
	 *     repeat from step 1.
	 * 3.) If a directory was found combine the remaining parts with dashes
	 *     to form a new desktop ID which needs to be found at the directory
	 *     found. Repeat at step 1.
	 * 4.) Get next path from search path and repeat from step 1 if there
	 *     is any search path left.
	 * 5.) If this step is reached, no desktop file was found.
	 */

	/* Get singleton of application database */
	appDB=xfdashboard_application_database_get_default();

	/* Get search paths */
	searchPaths=xfdashboard_application_database_get_application_search_paths(appDB);
	if(!searchPaths)
	{
		/* Release allocated resources */
		g_object_unref(appDB);

		return(NULL);
	}

	/* Iterate through search paths and try to find desktop file for
	 * desktop ID.
	 */
	foundDesktopFile=NULL;
	for(; searchPaths && !foundDesktopFile; searchPaths=g_list_next(searchPaths))
	{
		const gchar			*searchPath;
		GFile				*directory;
		gchar				*desktopID;
		gchar				*desktopIDIter;

		/* Get search path */
		searchPath=(const gchar*)searchPaths->data;
		if(!searchPath) continue;

		/* Set up variable to query desktop file for requested desktop ID */
		directory=g_file_new_for_path(searchPath);
		desktopID=g_strdup(inDesktopID);

		/* Try to find desktop file */
		desktopIDIter=desktopID;
		while(!foundDesktopFile && desktopIDIter && *desktopIDIter)
		{
			GFile			*desktopFile;

			/* Check if a file with desktop ID and suffix ".desktop" exists
			 * at current directory path.
			 */
			desktopFile=g_file_get_child(directory, desktopIDIter);
			if(g_file_query_exists(desktopFile, NULL))
			{
				foundDesktopFile=g_file_get_path(desktopFile);
			}
			g_object_unref(desktopFile);

			/* There was no desktop file at directory so lookup next possible
			 * directory to lookup desktop file at.
			 */
			if(!foundDesktopFile)
			{
				gchar		*p;
				gboolean	foundDirectory;
				GFile		*subDirectory;

				/* Iterate through remaining desktop ID and lookup dash.
				 * If a dash is found check if directory with this name exists.
				 * If a directory exists set up new base directory and repeat
				 * with outer loop (lookup file, then sub-directories) again.
				 */
				foundDirectory=FALSE;
				p=desktopIDIter;
				while(!foundDirectory && *p)
				{
					/* Found a dash so check if a sub-directory with this name
					 * up to this dash exists.
					 */
					if(*p=='-')
					{
						/* Truncate desktop ID to current position */
						*p=0;

						/* Check if sub-directory with truncated desktop ID
						 * as name exists.
						 */
						subDirectory=g_file_get_child(directory, desktopIDIter);
						if(g_file_query_exists(subDirectory, NULL))
						{
							/* Directory with this name exists. So set up
							 * sub-directory as new base directory to lookup
							 * remaining desktop ID at. Also set up desktop ID
							 * iterator to remaining desktop ID to lookup.
							 */
							g_object_unref(directory);
							directory=g_object_ref(subDirectory);

							desktopIDIter=p;
							desktopIDIter++;

							foundDirectory=TRUE;
						}
						g_object_unref(subDirectory);

						/* Restore desktop ID by setting dash again we removed
						 * when checking sub-directory existance.
						 */
						*p='-';
					}

					/* Continue with next character in remaining desktop ID */
					p++;
				}

				/* If we have not found a sub-directory then set desktop ID
				 * iterator to NULL to break loop and to continue with next
				 * path in list of search paths.
				 */
				if(!foundDirectory) desktopIDIter=NULL;
			}
		}

		/* Release allocated resources */
		g_object_unref(directory);
		g_free(desktopID);
	}

	/* Release allocated resources */
	g_object_unref(appDB);

	/* Return found desktop file */
	return(foundDesktopFile);
}

/* Get desktop ID from requested desktop file path
 * Returns NULL if desktop file is not in any search path.
 */
gchar* xfdashboard_application_database_get_desktop_id_from_path(const gchar *inFilename)
{
	XfdashboardApplicationDatabase	*appDB;
	const GList						*searchPaths;
	gchar							*foundDesktopID;

	g_return_val_if_fail(inFilename && *inFilename, NULL);
	g_return_val_if_fail(g_str_has_suffix(inFilename, ".desktop"), NULL);

	foundDesktopID=NULL;

	/* Get singleton of application database */
	appDB=xfdashboard_application_database_get_default();

	/* Get search paths */
	searchPaths=xfdashboard_application_database_get_application_search_paths(appDB);
	if(!searchPaths)
	{
		/* Release allocated resources */
		g_object_unref(appDB);

		return(NULL);
	}

	/* Iterate through search paths and check if filename of desktop file
	 * begins with search path. If it does then strip search path from
	 * desktop file's filename and replace all directory seperators with
	 * dashes.
	 */
	for(; searchPaths && !foundDesktopID; searchPaths=g_list_next(searchPaths))
	{
		const gchar					*searchPath;

		/* Get search path */
		searchPath=(const gchar*)searchPaths->data;
		if(!searchPath) continue;

		/* Check if filename path begins with current search path */
		if(g_str_has_prefix(inFilename, searchPath))
		{
			const gchar		*begin;

			/* Strip search path from desktop file's path */
			begin=inFilename+strlen(searchPath);
			while(*begin==G_DIR_SEPARATOR) begin++;

			foundDesktopID=g_strdup(begin);
		}
	}

	/* If desktop ID was found replace directory sepearators with dashes */
	if(foundDesktopID)
	{
		gchar						*separatorIter;

		separatorIter=foundDesktopID;
		while(*separatorIter)
		{
			/* Replace directory sepearator with dash if needed */
			if(*separatorIter==G_DIR_SEPARATOR) *separatorIter='-';

			/* Continue with next character in desktop ID */
			separatorIter++;
		}
	}

	/* Release allocated resources */
	g_object_unref(appDB);

	/* Return found desktop ID */
	return(foundDesktopID);
}

/* Get desktop ID from requested desktop file object
 * Returns NULL if desktop file is not in any search path.
 */
gchar* xfdashboard_application_database_get_desktop_id_from_file(GFile *inFile)
{
	gchar							*path;
	gchar							*foundDesktopID;

	g_return_val_if_fail(G_IS_FILE(inFile), NULL);

	/* Get path of file object */
	path=g_file_get_path(inFile);

	/* Get desktop ID for path */
	foundDesktopID=xfdashboard_application_database_get_desktop_id_from_path(path);

	/* Release allocated resources */
	if(path) g_free(path);

	/* Return found desktop ID */
	return(foundDesktopID);
}
