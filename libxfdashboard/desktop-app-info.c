/*
 * desktop-app-info: A GDesktopAppInfo like object for garcon menu
 *                   items implementing and supporting GAppInfo
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

#include <libxfdashboard/desktop-app-info.h>
#include <libxfdashboard/application-database.h>
#include <libxfdashboard/compat.h>
#include <libxfdashboard/debug.h>


/* Define this class in GObject system */
static void _xfdashboard_desktop_app_info_gappinfo_iface_init(GAppInfoIface *iface);

struct _XfdashboardDesktopAppInfoPrivate
{
	/* Properties related */
	gchar				*desktopID;
	GFile				*file;

	/* Instance related */
	gboolean			inited;
	gboolean			isValid;

	GarconMenuItem		*item;
	guint				itemChangedID;

	GKeyFile			*secondarySource;

	gchar				*binaryExecutable;

	gboolean			needActions;
	GList				*actions;

	gboolean			needKeywords;
	GList				*keywords;
};

G_DEFINE_TYPE_WITH_CODE(XfdashboardDesktopAppInfo,
						xfdashboard_desktop_app_info,
						G_TYPE_OBJECT,
						G_ADD_PRIVATE(XfdashboardDesktopAppInfo)
						G_IMPLEMENT_INTERFACE(G_TYPE_APP_INFO, _xfdashboard_desktop_app_info_gappinfo_iface_init))

/* Properties */
enum
{
	PROP_0,

	PROP_VALID,
	PROP_DESKTOP_ID,
	PROP_FILE,

	PROP_LAST
};

static GParamSpec* XfdashboardDesktopAppInfoProperties[PROP_LAST]={ 0, };

/* Signals */
enum
{
	SIGNAL_CHANGED,
	SIGNAL_RELOAD,

	SIGNAL_LAST
};

static guint XfdashboardDesktopAppInfoSignals[SIGNAL_LAST]={ 0, };

/* IMPLEMENTATION: Private variables and methods */
typedef struct
{
	gchar	*display;
	gchar	*startupNotificationID;
	gchar	*desktopFile;
} XfdashboardDesktopAppInfoChildSetupData;

/* Load secondary source file if not already done.
 * Note: It is called secondary source although it is the same file as used
 * for GarconMenuItem. But it is not the same source because the file is loaded
 * via a GKeyFile object to get access to entries not provided by garcon or
 * implemented in an unusable way for xfdashboard.
 */
static gboolean _xfdashboard_desktop_app_info_load_secondary_source(XfdashboardDesktopAppInfo *self)
{
	XfdashboardDesktopAppInfoPrivate		*priv;

	g_return_val_if_fail(XFDASHBOARD_IS_DESKTOP_APP_INFO(self), FALSE);

	priv=self->priv;

	/* Only load secondary source file if not already done and a file is set */
	if(!priv->secondarySource &&
		priv->file)
	{
		GKeyFile							*keyfile;
		gchar								*secondarySourceFilename;
		GError								*error;

		error=NULL;

		/* Get path to secondary source file */
		secondarySourceFilename=g_file_get_path(priv->file);

		/* Load secondary source file */
		keyfile=g_key_file_new();
		if(!g_key_file_load_from_file(keyfile,
										secondarySourceFilename,
										G_KEY_FILE_KEEP_TRANSLATIONS,
										&error))
		{
			/* Show warning */
			g_warning(_("Could not load secondary source %s for desktop ID '%s': %s"),
						secondarySourceFilename,
						priv->desktopID,
						error ? error->message : _("Unknown error"));

			/* Release allocated resources */
			if(error) g_error_free(error);
			if(secondarySourceFilename) g_free(secondarySourceFilename);
			if(keyfile) g_key_file_unref(keyfile);

			/* Return FALSE to indicate error */
			return(FALSE);
		}

		/* Use secondary source */
		priv->secondarySource=g_key_file_ref(keyfile);

		/* Release allocated resources */
		if(secondarySourceFilename) g_free(secondarySourceFilename);
		if(keyfile) g_key_file_unref(keyfile);
	}

	/* If we get here and we have no keyfile for secondary source, return FALSE
	 * to indicate that secondary source is not available.
	 */
	if(G_UNLIKELY(!priv->secondarySource)) return(FALSE);

	/* Return TRUE for success */
	return(TRUE);
}

/* Get or update path to executable file for this application */
static void _xfdashboard_desktop_app_info_update_binary_executable(XfdashboardDesktopAppInfo *self)
{
	XfdashboardDesktopAppInfoPrivate		*priv;

	g_return_if_fail(XFDASHBOARD_IS_DESKTOP_APP_INFO(self));

	priv=self->priv;

	/* Get path to executable file for this application by striping white-space from
	 * the beginning of the command to execute when launching up to first white-space
	 * after the first command-line argument (which is the command).
	 */
	if(priv->binaryExecutable)
	{
		g_free(priv->binaryExecutable);
		priv->binaryExecutable=NULL;
	}

	if(priv->item)
	{
		const gchar						*command;
		const gchar						*commandStart;
		const gchar						*commandEnd;

		command=garcon_menu_item_get_command(priv->item);

		while(*command==' ') command++;
		commandStart=command;

		while(*command && *command!=' ') command++;
		commandEnd=command;

		priv->binaryExecutable=g_strndup(commandStart, commandEnd-commandStart);
	}
}

/* (Re-)Load application actions */
static void _xfdashboard_desktop_app_info_update_actions(XfdashboardDesktopAppInfo *self)
{
	XfdashboardDesktopAppInfoPrivate		*priv;

	g_return_if_fail(XFDASHBOARD_IS_DESKTOP_APP_INFO(self));

	priv=self->priv;

	/* Reload only if needed */
	if(!priv->needActions) return;

	/* Remove old actions loaded */
	if(priv->actions)
	{
		g_list_free_full(priv->actions, g_object_unref);
		priv->actions=NULL;
	}

	/* Get application actions for menu item (desktop entry) */
#if 0 /*GARCON_CHECK_VERSION(0, 6, 3)*/
	if(priv->item)
	{
		GList								*itemActions;
		GList								*iter;
		const gchar							*itemActionName;
		GarconMenuItemAction				*itemAction;
		XfdashboardDesktopAppInfoAction		*action;

		/* Get action from garcon menu item and create desktop info action object
		 * for each action iterated.
		 */
		itemActions=garcon_menu_item_get_actions(priv->item);
		for(iter=itemActions; iter; iter=g_list_next(iter))
		{
			/* Get action currently iterated from garcon menu item */
			itemActionName=(const gchar*)(iter->data);
			if(!itemActionName)
			{
				g_warning(_("Cannot create application action because of empty action name for desktop ID '%s'"),
							priv->desktopID);
				continue;
			}

			itemAction=garcon_menu_item_get_action(priv->item, itemActionName);
			if(!itemAction)
			{
				g_warning(_("Cannot create application action for desktop ID '%s'"),
							priv->desktopID);
				continue;
			}

			/* Create desktop info action object and add to list */
			action=XFDASHBOARD_DESKTOP_APP_INFO_ACTION
					(
						g_object_new(XFDASHBOARD_TYPE_DESKTOP_APP_INFO_ACTION,
										"name", garcon_menu_item_action_get_name(itemAction),
										"icon-name", garcon_menu_item_action_get_icon_name(itemAction),
										"command", garcon_menu_item_action_get_command(itemAction),
										NULL)
					);
			priv->actions=g_list_prepend(priv->actions, action);

			XFDASHBOARD_DEBUG(self, APPLICATIONS,
								"Created application action '%s' for desktop ID '%s'",
								xfdashboard_desktop_app_info_action_get_name(action),
								priv->desktopID);
		}
		priv->actions=g_list_reverse(priv->actions);

		/* Release allocated resources */
		g_list_free(itemActions);
	}
#else
	/* Garcon prior to version 0.6.0 does not provide accessor function for
	 * application actions of a desktop entry and the first version providing
	 * these function return the action in an unpredictable order but not the
	 * order as listed in "Action" keyword of desktop entry. So we need to
	 * load the secondary source and grab them ourselve.
	 */
	if(_xfdashboard_desktop_app_info_load_secondary_source(self))
	{
		gchar								**itemActions;
		gchar								**iter;
		gchar								*itemActionGroup;
		gchar								*itemActionName;
		gchar								*itemActionIcon;
		gchar								*itemActionExec;
		XfdashboardDesktopAppInfoAction		*action;
		GError								*error;

		error=NULL;

		/* Get list of actions and their order from "Action" keyword */
		itemActions=g_key_file_get_string_list(priv->secondarySource,
												G_KEY_FILE_DESKTOP_GROUP,
												G_KEY_FILE_DESKTOP_KEY_ACTIONS,
												NULL,
												&error);
		if(!itemActions)
		{
			XFDASHBOARD_DEBUG(self, APPLICATIONS,
								"Could not fetch list of actions from secondary source for desktop ID '%s': %s",
								priv->desktopID,
								error ? error->message : _("Unknown error"));

			/* Release allocated resources */
			if(error) g_error_free(error);

			/* Return from here as we cannot collect any list of actions */
			return;
		}

		/* Get action currently iterated from string list of secondary source */
		for(iter=itemActions; *iter; iter++)
		{
			/* Determine group name in desktop file to retrieve data about
			 * application action currently iterated.
			 */
			itemActionGroup=g_strdup_printf("Desktop Action %s", *iter);

			/* Get display name of application action. According to the specification,
			 * it says only the "Name" keyword is required. So do not create a
			 * desktop info action object if it is missing, but fail silenty and
			 * continue with next action in list.
			 */
			itemActionName=g_key_file_get_locale_string(priv->secondarySource,
														itemActionGroup,
														G_KEY_FILE_DESKTOP_KEY_NAME,
														NULL,
														&error);
			if(!itemActionName)
			{
				XFDASHBOARD_DEBUG(self, APPLICATIONS,
									"Could not get name of action '%s' from secondary source for desktop ID '%s': %s",
									*iter,
									priv->desktopID,
									error ? error->message : _("Unknown error"));

				/* Release allocated resources */
				if(itemActionGroup) g_free(itemActionGroup);
				if(error)
				{
					g_error_free(error);
					error=NULL;
				}

				/* Continue with next action */
				continue;
			}

			/* Get optional icon name of application action */
			itemActionIcon=g_key_file_get_string(priv->secondarySource,
													itemActionGroup,
													G_KEY_FILE_DESKTOP_KEY_ICON,
													NULL);

			/* Get optional command of application action */
			itemActionExec=g_key_file_get_string(priv->secondarySource,
													itemActionGroup,
													G_KEY_FILE_DESKTOP_KEY_EXEC,
													NULL);

			/* Create desktop info action object and add to list */
			action=XFDASHBOARD_DESKTOP_APP_INFO_ACTION
					(
						g_object_new(XFDASHBOARD_TYPE_DESKTOP_APP_INFO_ACTION,
										"name", itemActionName,
										"icon-name", itemActionIcon,
										"command", itemActionExec,
										NULL)
					);
			priv->actions=g_list_prepend(priv->actions, action);

			XFDASHBOARD_DEBUG(self, APPLICATIONS,
								"Created application action '%s' for desktop ID '%s' from secondary source",
								xfdashboard_desktop_app_info_action_get_name(action),
								priv->desktopID);

			/* Release allocated resources */
			if(itemActionExec) g_free(itemActionExec);
			if(itemActionIcon) g_free(itemActionIcon);
			if(itemActionName) g_free(itemActionName);
			if(itemActionGroup) g_free(itemActionGroup);
		}
		priv->actions=g_list_reverse(priv->actions);

		/* Release allocated resources */
		if(itemActions) g_strfreev(itemActions);
	}
#endif

	/* Set flag that application actions are loaded and do not need futher updates */
	priv->needActions=FALSE;
}

/* (Re-)Load keywords */
static void _xfdashboard_desktop_app_info_update_keywords(XfdashboardDesktopAppInfo *self)
{
	XfdashboardDesktopAppInfoPrivate		*priv;

	g_return_if_fail(XFDASHBOARD_IS_DESKTOP_APP_INFO(self));

	priv=self->priv;

	/* Reload only if needed */
	if(!priv->needKeywords) return;

	/* Remove old actions loaded */
	if(priv->keywords)
	{
		g_list_free_full(priv->keywords, g_free);
		priv->keywords=NULL;
	}

	/* Get application actions for menu item (desktop entry) */
#if 0 /*GARCON_CHECK_VERSION(0, 6, 3)*/
	if(priv->item)
	{
		const GList							*keywords;

		/* Get keywords from garcon menu item and create a deep copy of list */
		keywords=garcon_menu_item_get_keywords(priv->item);
		for(iter=keywords; iter; iter=g_list_next(iter))
		{
			/* Create copy of list entry and prepend to new list */
			priv->keywords=g_list_prepend(priv->keywords, g_strdup((const gchar*)(iter->data)));

			XFDASHBOARD_DEBUG(self, APPLICATIONS,
								"Added keyword '%s' for desktop ID '%s'",
								(const gchar*)(iter->data),
								priv->desktopID);
		}
		priv->keywords=g_list_reverse(priv->keywords);
	}
#else
	/* Garcon does not provide an accessor function to get keywords for desktop
	 * entries in any official released version yet. So load them ourselve from
	 * secondary source.
	 */
	if(_xfdashboard_desktop_app_info_load_secondary_source(self))
	{
		gchar								**keywords;
		gchar								**iter;
		GError								*error;

		error=NULL;

		/* Get keywords from secondary source */
		keywords=g_key_file_get_string_list(priv->secondarySource,
												G_KEY_FILE_DESKTOP_GROUP,
												"Keywords",
												NULL,
												&error);
		if(!keywords)
		{
			XFDASHBOARD_DEBUG(self, APPLICATIONS,
								"Could not fetch list of keywords from secondary source for desktop ID '%s': %s",
								priv->desktopID,
								error ? error->message : _("Unknown error"));

			/* Release allocated resources */
			if(error) g_error_free(error);

			/* Return from here as we cannot collect any list of actions */
			return;
		}

		/* Get action currently iterated from string list of secondary source */
		for(iter=keywords; *iter; iter++)
		{
			/* Create copy a currently iterated keyword and prepend to list */
			priv->keywords=g_list_prepend(priv->keywords, g_strdup(*iter));

			XFDASHBOARD_DEBUG(self, APPLICATIONS,
								"Added keyword '%s' for desktop ID '%s' from secondary source",
								*iter,
								priv->desktopID);
		}
		priv->keywords=g_list_reverse(priv->keywords);

		/* Release allocated resources */
		if(keywords) g_strfreev(keywords);
	}
#endif

	/* Set flag that keywords are loaded and do not need futher updates */
	priv->needKeywords=FALSE;
}

/* Menu item has changed */
static void _xfdashboard_desktop_app_info_on_item_changed(XfdashboardDesktopAppInfo *self,
															gpointer inUserData)
{
	g_return_if_fail(XFDASHBOARD_IS_DESKTOP_APP_INFO(self));

	/* Emit 'changed' signal for this desktop app info */
	g_signal_emit(self, XfdashboardDesktopAppInfoSignals[SIGNAL_CHANGED], 0);
}

/* Set desktop ID */
static void _xfdashboard_desktop_app_info_set_desktop_id(XfdashboardDesktopAppInfo *self,
															const gchar *inDesktopID)
{
	XfdashboardDesktopAppInfoPrivate		*priv;

	g_return_if_fail(XFDASHBOARD_IS_DESKTOP_APP_INFO(self));

	priv=self->priv;

	/* Set value if changed */
	if(g_strcmp0(priv->desktopID, inDesktopID))
	{
		/* Set value */
		if(priv->desktopID)
		{
			g_free(priv->desktopID);
			priv->desktopID=NULL;
		}

		if(inDesktopID) priv->desktopID=g_strdup(inDesktopID);

		/* Notify about property change */
		g_object_notify_by_pspec(G_OBJECT(self), XfdashboardDesktopAppInfoProperties[PROP_DESKTOP_ID]);
	}
}

/* Set desktop file */
static void _xfdashboard_desktop_app_info_set_file(XfdashboardDesktopAppInfo *self,
													GFile *inFile)
{
	XfdashboardDesktopAppInfoPrivate	*priv;

	g_return_if_fail(XFDASHBOARD_IS_DESKTOP_APP_INFO(self));
	g_return_if_fail(!inFile || G_IS_FILE(inFile));

	priv=self->priv;

	/* Set value if changed */
	if(!(priv->file && inFile && g_file_equal(priv->file, inFile)))
	{
		gboolean						valid;

		/* Freeze notification */
		g_object_freeze_notify(G_OBJECT(self));

		/* Release secondard source */
		if(priv->secondarySource)
		{
			g_key_file_unref(priv->secondarySource);
			priv->secondarySource=NULL;
		}

		/* Replace current file to menu item with new one */
		if(priv->file)
		{
			g_object_unref(priv->file);
			priv->file=NULL;
		}
		if(inFile) priv->file=g_object_ref(inFile);

		/* Replace current menu item with new one */
		if(priv->item)
		{
			if(priv->itemChangedID)
			{
				g_signal_handler_disconnect(priv->item, priv->itemChangedID);
				priv->itemChangedID=0;
			}

			g_object_unref(priv->item);
			priv->item=NULL;
		}

		if(priv->file)
		{
			priv->item=garcon_menu_item_new(priv->file);
		}

		/* Connect signal to get notified about changes of menu item */
		if(priv->item)
		{
			priv->itemChangedID=g_signal_connect_swapped(priv->item,
															"changed",
															G_CALLBACK(_xfdashboard_desktop_app_info_on_item_changed),
															self);
		}

		/* Get path to executable file for this application */
		_xfdashboard_desktop_app_info_update_binary_executable(self);

		/* Set flag to reload application actions and keywords. They will be
		 * cleared and (re-)loaded on-demand.
		 */
		priv->needActions=TRUE;
		priv->needKeywords=TRUE;

		/* Notify about property change */
		g_object_notify_by_pspec(G_OBJECT(self), XfdashboardDesktopAppInfoProperties[PROP_FILE]);

		/* Emit 'changed' signal if was inited before */
		if(priv->inited)
		{
			g_signal_emit(self, XfdashboardDesktopAppInfoSignals[SIGNAL_CHANGED], 0);
		}

		/* Desktop file is set, menu item loaded - this desktop app info is inited now */
		priv->inited=TRUE;

		/* Check if this app info is valid (either file is not set or file and menu item is set */
		valid=FALSE;
		if(!priv->file ||
			(priv->file && priv->item))
		{
			valid=TRUE;
		}

		/* Set value if changed */
		if(priv->isValid!=valid)
		{
			/* Set value */
			priv->isValid=valid;

			/* Notify about property change */
			g_object_notify_by_pspec(G_OBJECT(self), XfdashboardDesktopAppInfoProperties[PROP_VALID]);
		}

		/* Thaw notification */
		g_object_thaw_notify(G_OBJECT(self)); 	
	}
		/* If both files (the file already set in this desktop app info and the file to set
		 * are equal then reload desktop file which forces the 'changed' signal to be emitted.
		 */
		else if(priv->inited && priv->file && inFile && g_file_equal(priv->file, inFile))
		{
			gboolean					valid;

			/* Reload desktop file */
			valid=xfdashboard_desktop_app_info_reload(self);

			/* Set value depending on reload success if changed */
			if(priv->isValid!=valid)
			{
				/* Set value */
				priv->isValid=valid;

				/* Notify about property change */
				g_object_notify_by_pspec(G_OBJECT(self), XfdashboardDesktopAppInfoProperties[PROP_VALID]);
			}
		}
}

/* Launch application with URIs in given context */
static void _xfdashboard_desktop_app_info_expand_macros_add_file(const gchar *inURI, GString *ioExpanded)
{
	GFile		*file;
	gchar		*path;
	gchar		*quotedPath;

	g_return_if_fail(inURI && *inURI);
	g_return_if_fail(ioExpanded);

	file=g_file_new_for_uri(inURI);
	path=g_file_get_path(file);
	quotedPath=g_shell_quote(path);

	g_string_append(ioExpanded, quotedPath);
	g_string_append_c(ioExpanded, ' ');

	g_free(quotedPath);
	g_free(path);
	g_object_unref(file);
}

static void _xfdashboard_desktop_app_info_expand_macros_add_uri(const gchar *inURI, GString *ioExpanded)
{
	gchar		*quotedURI;

	g_return_if_fail(inURI && *inURI);
	g_return_if_fail(ioExpanded);

	quotedURI=g_shell_quote(inURI);

	g_string_append(ioExpanded, quotedURI);
	g_string_append_c(ioExpanded, ' ');

	g_free(quotedURI);
}

static gboolean _xfdashboard_desktop_app_info_expand_macros(XfdashboardDesktopAppInfo *self,
															const gchar *inCommand,
															GList *inURIs,
															GString *ioExpanded)
{
	XfdashboardDesktopAppInfoPrivate	*priv;
	gboolean							filesOrUriAdded;

	g_return_val_if_fail(XFDASHBOARD_IS_DESKTOP_APP_INFO(self), FALSE);
	g_return_val_if_fail(inCommand && *inCommand, FALSE);
	g_return_val_if_fail(ioExpanded, FALSE);

	priv=self->priv;

	/* Iterate through command-line char by char and expand known macros */
	filesOrUriAdded=FALSE;

	while(*inCommand)
	{
		/* Check if character is '%' indicating that a macro could follow ... */
		if(*inCommand=='%')
		{
			/* Move to next character to determin which macro to expand
			 * but check also that we have not reached end of inCommand-line.
			 */
			inCommand++;
			if(!*inCommand) break;

			/* Expand macro */
			switch(*inCommand)
			{
				case 'f':
					if(inURIs) _xfdashboard_desktop_app_info_expand_macros_add_file(inURIs->data, ioExpanded);
					filesOrUriAdded=TRUE;
					break;

				case 'F':
					g_list_foreach(inURIs, (GFunc)_xfdashboard_desktop_app_info_expand_macros_add_file, ioExpanded);
					filesOrUriAdded=TRUE;
					break;

				case 'u':
					if(inURIs) _xfdashboard_desktop_app_info_expand_macros_add_uri(inURIs->data, ioExpanded);
					filesOrUriAdded=TRUE;
					break;

				case 'U':
					g_list_foreach(inURIs, (GFunc)_xfdashboard_desktop_app_info_expand_macros_add_uri, ioExpanded);
					filesOrUriAdded=TRUE;
					break;

				case '%':
					g_string_append_c(ioExpanded, '%');
					break;

				case 'i':
				{
					const gchar			*iconName;
					gchar				*quotedIconName;

					iconName=garcon_menu_item_get_icon_name(priv->item);
					if(iconName)
					{
						quotedIconName=g_shell_quote(iconName);

						g_string_append(ioExpanded, "--icon ");
						g_string_append(ioExpanded, quotedIconName);

						g_free(quotedIconName);
					}
					break;
				}

				case 'c':
				{
					const gchar			*name;
					gchar				*quotedName;

					name=garcon_menu_item_get_name(priv->item);
					if(name)
					{
						quotedName=g_shell_quote(name);
						g_string_append(ioExpanded, quotedName);
						g_free(quotedName);
					}
					break;
				}

				case 'k':
				{
					GFile				*desktopFile;
					gchar				*filename;
					gchar				*quotedFilename;

					desktopFile=garcon_menu_item_get_file(priv->item);
					if(desktopFile)
					{
						filename=g_file_get_path(desktopFile);
						if(filename)
						{
							quotedFilename=g_shell_quote(filename);
							g_string_append(ioExpanded, quotedFilename);
							g_free(quotedFilename);

							g_free(filename);
						}

						g_object_unref(desktopFile);
					}

					break;
				}

				default:
					break;
			}
		}
			/* ... otherwise just add the character */
			else g_string_append_c(ioExpanded, *inCommand);

		/* Continue with next character in inCommand-line */
		inCommand++;
	}

	/* If URIs was provided but not used (exec key does not contain %f, %F, %u, %U)
	 * append first URI to expanded inCommand-line.
	 */
	if(inURIs && !filesOrUriAdded)
	{
		g_string_append_c(ioExpanded, ' ');
		 _xfdashboard_desktop_app_info_expand_macros_add_file(inURIs->data, ioExpanded);
	}

	/* If we get here we could expand macros in command-line successfully */
	return(TRUE);
}

/* Child process for launching application was spawned but application
 * was not executed yet so we can set up environment etc. now.
 * 
 * Note: Do not use any kind of dynamically allocated memory like
 *       GObject instances or memory allocation functions like g_new,
 *       malloc etc., and also do not ref or unref any GObject instances
 *       because we cannot be sure that memory is cleaned up and references
 *       are incremented/decremented in spawned (forked) child process.
 */
static void _xfdashboard_desktop_app_info_on_child_spawned(gpointer inUserData)
{
	XfdashboardDesktopAppInfoChildSetupData		*data=(XfdashboardDesktopAppInfoChildSetupData*)inUserData;

	g_return_if_fail(data);

	/* Set up environment */
	if(data->display)
	{
		g_setenv("DISPLAY", data->display, TRUE);
	}

	if(data->startupNotificationID)
	{
		g_setenv("DESKTOP_STARTUP_ID", data->startupNotificationID, TRUE);
	}

	if(data->desktopFile)
	{
		gchar									pid[20];

		g_setenv("GIO_LAUNCHED_DESKTOP_FILE", data->desktopFile, TRUE);

		g_snprintf(pid, 20, "%ld", (long)getpid());
		g_setenv("GIO_LAUNCHED_DESKTOP_FILE_PID", pid, TRUE);
	}
}

static gboolean _xfdashboard_desktop_app_info_launch_appinfo_internal(XfdashboardDesktopAppInfo *self,
																		const gchar *inCommand,
																		GList *inURIs,
																		GAppLaunchContext *inContext,
																		GError **outError)
{
	XfdashboardDesktopAppInfoPrivate			*priv;
	GString										*expanded;
	gchar										*display;
	gchar										*startupNotificationID;
	gchar										*desktopFile;
	const gchar									*workingDirectory;
	gboolean									success;
	GPid										launchedPID;
	gint										argc;
	gchar										**argv;
	GError										*error;
	XfdashboardDesktopAppInfoChildSetupData		childSetup;

	g_return_val_if_fail(XFDASHBOARD_IS_DESKTOP_APP_INFO(self), FALSE);
	g_return_val_if_fail(inCommand && *inCommand, FALSE);
	g_return_val_if_fail(!inContext || G_IS_APP_LAUNCH_CONTEXT(inContext), FALSE);
	g_return_val_if_fail(outError && *outError==NULL, FALSE);

	priv=self->priv;
	display=NULL;
	startupNotificationID=NULL;
	desktopFile=NULL;
	success=FALSE;
	argc=0;
	argv=NULL;
	error=NULL;

	/* Get command-line with expanded macros */
	expanded=g_string_new(NULL);
	if(!expanded ||
		!_xfdashboard_desktop_app_info_expand_macros(self, inCommand, inURIs, expanded))
	{
		/* Set error */
		g_set_error_literal(outError,
								G_IO_ERROR,
								G_IO_ERROR_FAILED,
								_("Unable to expand macros at command-line."));

		/* Release allocated resources */
		if(expanded) g_string_free(expanded, TRUE);

		/* Return error state */
		return(FALSE);
	}

	/* If a terminal is required, prepend "exo-open" command.
	 * NOTE: The space at end of command is important to separate
	 *       the command we prepend from command-line of application.
	 */
	if(garcon_menu_item_requires_terminal(priv->item))
	{
		g_string_prepend(expanded, "exo-open --launch TerminalEmulator ");
	}

	/* Get command-line arguments as string list */
	if(!g_shell_parse_argv(expanded->str, &argc, &argv, &error))
	{
		/* Propagate error */
		g_propagate_error(outError, error);

		/* Release allocated resources */
		if(argv) g_strfreev(argv);
		if(expanded) g_string_free(expanded, TRUE);

		/* Return error state */
		return(FALSE);
	}

	/* Set up launch context, e.g. display and startup notification */
	if(inContext)
	{
		GList									*filesToLaunch;
		GList									*iter;

		/* Create GFile objects for URIs */
		filesToLaunch=NULL;
		for(iter=inURIs; iter; iter=g_list_next(iter))
		{
			GFile						*file;

			file=g_file_new_for_uri((const gchar*)iter->data);
			filesToLaunch=g_list_prepend(filesToLaunch, file);
		}
		filesToLaunch=g_list_reverse(filesToLaunch);

		/* Get display where to launch application at */
		display=g_app_launch_context_get_display(inContext,
													G_APP_INFO(self),
													filesToLaunch);

		/* Get startup notification ID if it is supported by application */
		if(garcon_menu_item_supports_startup_notification(priv->item))
		{
			startupNotificationID=g_app_launch_context_get_startup_notify_id(inContext,
																				G_APP_INFO(self),
																				filesToLaunch);
		}

		/* Release allocated resources */
		g_list_free_full(filesToLaunch, g_object_unref);
	}

	/* Get working directory and test if directory exists */
	workingDirectory=garcon_menu_item_get_path(priv->item);
	if(!workingDirectory || !*workingDirectory)
	{
		/* Working directory was either NULL or is an empty string,
		 * so do not set working directory.
		 */
		workingDirectory=NULL;
	}
		else if(!g_file_test(workingDirectory, G_FILE_TEST_IS_DIR))
		{
			/* Working directory does not exist or is not a directory */
			g_warning(_("Working directory '%s' does not exist. It won't be used when launching '%s'."),
						workingDirectory,
						*argv);

			/* Do not set working directory */
			workingDirectory=NULL;
		}

	/* Get desktop file of application to launch */
	desktopFile=g_file_get_path(priv->file);

	/* Launch application */
	childSetup.display=display;
	childSetup.startupNotificationID=startupNotificationID;
	childSetup.desktopFile=desktopFile;
	success=g_spawn_async(workingDirectory,
							argv,
							NULL,
							G_SPAWN_SEARCH_PATH,
							_xfdashboard_desktop_app_info_on_child_spawned,
							&childSetup,
							&launchedPID,
							&error);
	if(success)
	{
		GDBusConnection							*sessionBus;

		XFDASHBOARD_DEBUG(self, APPLICATIONS,
							"Launching %s succeeded with PID %ld.",
							garcon_menu_item_get_name(priv->item),
							(long)launchedPID);

		/* Open connection to DBUS session bus and send notification about
		 * successful launch of application. Then flush and close DBUS
		 * session bus connection.
		 */
		sessionBus=g_bus_get_sync(G_BUS_TYPE_SESSION, NULL, NULL);
		if(sessionBus)
		{
			GDBusMessage						*message;
			GVariantBuilder						uris;
			GVariantBuilder						extras;
			GList								*iter;
			const gchar							*desktopFileID;
			const gchar							*gioDesktopFile;
			const gchar							*programName;

			/* Build list of URIs */
			g_variant_builder_init(&uris, G_VARIANT_TYPE("as"));
			for(iter=inURIs; iter; iter=g_list_next(iter))
			{
				g_variant_builder_add(&uris, "s", (const gchar*)iter->data);
			}

			/* Build list of extra information */
			g_variant_builder_init(&extras, G_VARIANT_TYPE("a{sv}"));
			if(startupNotificationID &&
				g_utf8_validate(startupNotificationID, -1, NULL))
			{
				g_variant_builder_add(&extras,
										"{sv}",
										"startup-id",
										g_variant_new("s", startupNotificationID));
			}

			gioDesktopFile=g_getenv("GIO_LAUNCHED_DESKTOP_FILE");
			if(gioDesktopFile)
			{
				g_variant_builder_add(&extras,
										"{sv}",
										"origin-desktop-file",
										g_variant_new_bytestring(gioDesktopFile));
			}

			programName=g_get_prgname();
			if(programName)
			{
				g_variant_builder_add(&extras,
										"{sv}",
										"origin-prgname",
										g_variant_new_bytestring(programName));
			}

			g_variant_builder_add(&extras,
									"{sv}",
									"origin-pid",
									g_variant_new("x", (gint64)getpid()));

			if(priv->desktopID) desktopFileID=priv->desktopID;
				else if(priv->file) desktopFileID=desktopFile;
				else desktopFileID="";

			message=g_dbus_message_new_signal("/org/gtk/gio/DesktopAppInfo",
												"org.gtk.gio.DesktopAppInfo",
												"Launched");
			g_dbus_message_set_body(message,
										g_variant_new
										(
											"(@aysxasa{sv})",
											g_variant_new_bytestring(desktopFileID),
											display ? display : "",
											(gint64)launchedPID,
											&uris,
											&extras
										));
			g_dbus_connection_send_message(sessionBus,
											message,
											0,
											NULL,
											NULL);

			g_object_unref(message);

			/* It is safe to unreference DBUS session bus object after
			 * calling flush function even if the flush function is
			 * a asynchronous function because it takes its own reference
			 * on the session bus to keep it alive until flush is complete.
			 */
			g_dbus_connection_flush(sessionBus, NULL, NULL, NULL);
			g_object_unref(sessionBus);
		}
	}
		else
		{
			g_warning("Launching %s failed!", garcon_menu_item_get_name(priv->item));

			/* Propagate error */
			g_propagate_error(outError, error);

			/* Tell context about failed application launch */
			if(startupNotificationID)
			{
				g_app_launch_context_launch_failed(inContext, startupNotificationID);
			}
		}

	/* Release allocated resources */
	if(expanded) g_string_free(expanded, TRUE);
	if(argv) g_strfreev(argv);
	if(desktopFile) g_free(desktopFile);
	if(startupNotificationID) g_free(startupNotificationID);
	if(display) g_free(display);

	return(success);
}

/* IMPLEMENTATION: Interface GAppInfo */

/* Create a duplicate of a GAppInfo */
static GAppInfo* _xfdashboard_desktop_app_info_gappinfo_dup(GAppInfo *inAppInfo)
{
	XfdashboardDesktopAppInfo			*self;
	XfdashboardDesktopAppInfoPrivate	*priv;

	g_return_val_if_fail(XFDASHBOARD_IS_DESKTOP_APP_INFO(inAppInfo), NULL);

	self=XFDASHBOARD_DESKTOP_APP_INFO(inAppInfo);
	priv=self->priv;

	/* Create and return a newly allocated copy */
	return(G_APP_INFO(g_object_new(XFDASHBOARD_TYPE_DESKTOP_APP_INFO,
									"desktop-id", priv->desktopID,
									"file", priv->file,
									NULL)));
}

/*  Check if two GAppInfos are equal */
static gboolean _xfdashboard_desktop_app_info_gappinfo_equal(GAppInfo *inLeft, GAppInfo *inRight)
{
	XfdashboardDesktopAppInfo			*left;
	XfdashboardDesktopAppInfo			*right;

	g_return_val_if_fail(XFDASHBOARD_IS_DESKTOP_APP_INFO(inLeft), FALSE);
	g_return_val_if_fail(XFDASHBOARD_IS_DESKTOP_APP_INFO(inRight), FALSE);

	/* Get object instance for this class of both GAppInfos */
	left=XFDASHBOARD_DESKTOP_APP_INFO(inLeft);
	right=XFDASHBOARD_DESKTOP_APP_INFO(inRight);

	/* If one of both instance do not have a menu item return FALSE */
	if(!left->priv->item || !right->priv->item) return(FALSE);

	/* Return result of check if menu item of both GAppInfos are equal */
	return(garcon_menu_element_equal(GARCON_MENU_ELEMENT(left->priv->item),
										GARCON_MENU_ELEMENT(right->priv->item)));
}

/* Get ID of GAppInfo */
static const gchar* _xfdashboard_desktop_app_info_gappinfo_get_id(GAppInfo *inAppInfo)
{
	XfdashboardDesktopAppInfo			*self;
	XfdashboardDesktopAppInfoPrivate	*priv;

	g_return_val_if_fail(XFDASHBOARD_IS_DESKTOP_APP_INFO(inAppInfo), NULL);

	self=XFDASHBOARD_DESKTOP_APP_INFO(inAppInfo);
	priv=self->priv;

	/* Return ID of menu item */
	return(priv->desktopID);
}

/* Get name of GAppInfo */
static const gchar* _xfdashboard_desktop_app_info_gappinfo_get_name(GAppInfo *inAppInfo)
{
	XfdashboardDesktopAppInfo			*self;
	XfdashboardDesktopAppInfoPrivate	*priv;

	g_return_val_if_fail(XFDASHBOARD_IS_DESKTOP_APP_INFO(inAppInfo), NULL);

	self=XFDASHBOARD_DESKTOP_APP_INFO(inAppInfo);
	priv=self->priv;

	/* If desktop app info has no item return NULL here */
	if(!priv->item) return(NULL);

	/* Return name of menu item */
	return(garcon_menu_item_get_name(priv->item));
}

/* Get description of GAppInfo */
static const gchar* _xfdashboard_desktop_app_info_gappinfo_get_description(GAppInfo *inAppInfo)
{
	XfdashboardDesktopAppInfo			*self;
	XfdashboardDesktopAppInfoPrivate	*priv;

	g_return_val_if_fail(XFDASHBOARD_IS_DESKTOP_APP_INFO(inAppInfo), NULL);

	self=XFDASHBOARD_DESKTOP_APP_INFO(inAppInfo);
	priv=self->priv;

	/* If desktop app info has no item return NULL here */
	if(!priv->item) return(NULL);

	/* Return comment of menu item as description */
	return(garcon_menu_item_get_comment(priv->item));
}

/* Get path to executable binary of GAppInfo */
static const gchar* _xfdashboard_desktop_app_info_gappinfo_get_executable(GAppInfo *inAppInfo)
{
	XfdashboardDesktopAppInfo			*self;
	XfdashboardDesktopAppInfoPrivate	*priv;

	g_return_val_if_fail(XFDASHBOARD_IS_DESKTOP_APP_INFO(inAppInfo), NULL);

	self=XFDASHBOARD_DESKTOP_APP_INFO(inAppInfo);
	priv=self->priv;

	/* Return comment of menu item as description */
	return(priv->binaryExecutable);
}

/* Get icon of GAppInfo */
static GIcon* _xfdashboard_desktop_app_info_gappinfo_get_icon(GAppInfo *inAppInfo)
{
	XfdashboardDesktopAppInfo			*self;
	XfdashboardDesktopAppInfoPrivate	*priv;
	GIcon								*icon;
	const gchar							*iconFilename;

	g_return_val_if_fail(XFDASHBOARD_IS_DESKTOP_APP_INFO(inAppInfo), NULL);

	self=XFDASHBOARD_DESKTOP_APP_INFO(inAppInfo);
	priv=self->priv;
	icon=NULL;

	/* Create icon from path of menu item */
	if(priv->item)
	{
		iconFilename=garcon_menu_item_get_icon_name(priv->item);
		if(iconFilename)
		{
			if(!g_path_is_absolute(iconFilename)) icon=g_themed_icon_new(iconFilename);
				else
				{
					GFile						*file;

					file=g_file_new_for_path(iconFilename);
					icon=g_file_icon_new(file);
					g_object_unref(file);
				}
		}
	}

	/* Return icon created */
	return(icon);
}

/* Check if GAppInfo supports URIs as command-line parameters */
static gboolean _xfdashboard_desktop_app_info_gappinfo_supports_uris(GAppInfo *inAppInfo)
{
	XfdashboardDesktopAppInfo			*self;
	XfdashboardDesktopAppInfoPrivate	*priv;
	gboolean							result;
	const gchar							*command;

	g_return_val_if_fail(XFDASHBOARD_IS_DESKTOP_APP_INFO(inAppInfo), FALSE);

	self=XFDASHBOARD_DESKTOP_APP_INFO(inAppInfo);
	priv=self->priv;
	result=FALSE;

	/* Check if command at menu item contains "%u" or "%U"
	 * indicating URIs as command-line parameters.
	 */
	if(priv->item)
	{
		command=garcon_menu_item_get_command(priv->item);
		if(command)
		{
			if(!result && strstr(command, "%u")) result=TRUE;
			if(!result && strstr(command, "%U")) result=TRUE;
		}
	}

	/* Return result of check */
	return(result);
}

/* Check if GAppInfo supports file paths as command-line parameters */
static gboolean _xfdashboard_desktop_app_info_gappinfo_supports_files(GAppInfo *inAppInfo)
{
	XfdashboardDesktopAppInfo			*self;
	XfdashboardDesktopAppInfoPrivate	*priv;
	gboolean							result;
	const gchar							*command;

	g_return_val_if_fail(XFDASHBOARD_IS_DESKTOP_APP_INFO(inAppInfo), FALSE);

	self=XFDASHBOARD_DESKTOP_APP_INFO(inAppInfo);
	priv=self->priv;
	result=FALSE;

	/* Check if command at menu item contains "%f" or "%F"
	 * indicating file paths as command-line parameters.
	 */
	if(priv->item)
	{
		command=garcon_menu_item_get_command(priv->item);
		if(command)
		{
			if(!result && strstr(command, "%f")) result=TRUE;
			if(!result && strstr(command, "%F")) result=TRUE;
		}
	}

	/* Return result of check */
	return(result);
}

/* Launch application of GAppInfo with file paths */
static gboolean _xfdashboard_desktop_app_info_gappinfo_launch(GAppInfo *inAppInfo,
																GList *inFiles,
																GAppLaunchContext *inContext,
																GError **outError)
{
	XfdashboardDesktopAppInfo			*self;
	XfdashboardDesktopAppInfoPrivate	*priv;
	GList								*iter;
	GList								*uris;
	gchar								*uri;
	gboolean							result;

	g_return_val_if_fail(XFDASHBOARD_IS_DESKTOP_APP_INFO(inAppInfo), FALSE);
	g_return_val_if_fail(!inContext || G_IS_APP_LAUNCH_CONTEXT(inContext), FALSE);
	g_return_val_if_fail(outError && *outError==NULL, FALSE);

	self=XFDASHBOARD_DESKTOP_APP_INFO(inAppInfo);
	priv=self->priv;
	uris=NULL;

	/* Create list of URIs for files */
	for(iter=inFiles; iter; iter=g_list_next(iter))
	{
		uri=g_file_get_uri(iter->data);
		uris=g_list_prepend(uris, uri);
	}
	uris=g_list_reverse(uris);

	/* Call function to launch application of XfdashboardDesktopAppInfo with URIs */
	result=_xfdashboard_desktop_app_info_launch_appinfo_internal(self,
																	garcon_menu_item_get_command(priv->item),
																	uris,
																	inContext,
																	outError);

	/* Release allocated resources */
	g_list_free_full(uris, g_free);

	return(result);
}

/* Launch application of GAppInfo with URIs */
static gboolean _xfdashboard_desktop_app_info_gappinfo_launch_uris(GAppInfo *inAppInfo,
																	GList *inURIs,
																	GAppLaunchContext *inContext,
																	GError **outError)
{
	XfdashboardDesktopAppInfo			*self;
	XfdashboardDesktopAppInfoPrivate	*priv;
	gboolean							result;

	g_return_val_if_fail(XFDASHBOARD_IS_DESKTOP_APP_INFO(inAppInfo), FALSE);
	g_return_val_if_fail(!inContext || G_IS_APP_LAUNCH_CONTEXT(inContext), FALSE);
	g_return_val_if_fail(outError && *outError==NULL, FALSE);

	self=XFDASHBOARD_DESKTOP_APP_INFO(inAppInfo);
	priv=self->priv;

	/* Call function to launch application of XfdashboardDesktopAppInfo with URIs */
	result=_xfdashboard_desktop_app_info_launch_appinfo_internal(self,
																	garcon_menu_item_get_command(priv->item),
																	inURIs,
																	inContext,
																	outError);

	return(result);
}

/* Check if the application info should be shown */
static gboolean _xfdashboard_desktop_app_info_gappinfo_should_show(GAppInfo *inAppInfo)
{
	XfdashboardDesktopAppInfo			*self;
	XfdashboardDesktopAppInfoPrivate	*priv;

	g_return_val_if_fail(XFDASHBOARD_IS_DESKTOP_APP_INFO(inAppInfo), FALSE);

	self=XFDASHBOARD_DESKTOP_APP_INFO(inAppInfo);
	priv=self->priv;

	/* If desktop app info has no item return FALSE here */
	if(!priv->item) return(FALSE);

	/* Check if menu item is visible and therefore can be shown */
	return(garcon_menu_element_get_visible(GARCON_MENU_ELEMENT(priv->item)));
}

/* Get command-line of GAppInfo with which the application will be started */
static const gchar* _xfdashboard_desktop_app_info_gappinfo_get_commandline(GAppInfo *inAppInfo)
{
	XfdashboardDesktopAppInfo			*self;
	XfdashboardDesktopAppInfoPrivate	*priv;

	g_return_val_if_fail(XFDASHBOARD_IS_DESKTOP_APP_INFO(inAppInfo), NULL);

	self=XFDASHBOARD_DESKTOP_APP_INFO(inAppInfo);
	priv=self->priv;

	/* If desktop app info has no item return NULL here */
	if(!priv->item) return(NULL);

	/* Return command of menu item */
	return(garcon_menu_item_get_command(priv->item));
}

/* Get display name of GAppInfo */
static const gchar* _xfdashboard_desktop_app_info_gappinfo_get_display_name(GAppInfo *inAppInfo)
{
	XfdashboardDesktopAppInfo			*self;
	XfdashboardDesktopAppInfoPrivate	*priv;

	g_return_val_if_fail(XFDASHBOARD_IS_DESKTOP_APP_INFO(inAppInfo), NULL);

	self=XFDASHBOARD_DESKTOP_APP_INFO(inAppInfo);
	priv=self->priv;

	/* If desktop app info has no item return NULL here */
	if(!priv->item) return(NULL);

	/* Return name of menu item */
	return(garcon_menu_item_get_name(priv->item));
}

/* Interface initialization
 * Set up default functions
 */
static void _xfdashboard_desktop_app_info_gappinfo_iface_init(GAppInfoIface *iface)
{
	iface->dup=_xfdashboard_desktop_app_info_gappinfo_dup;
	iface->equal=_xfdashboard_desktop_app_info_gappinfo_equal;
	iface->get_id=_xfdashboard_desktop_app_info_gappinfo_get_id;
	iface->get_name=_xfdashboard_desktop_app_info_gappinfo_get_name;
	iface->get_description=_xfdashboard_desktop_app_info_gappinfo_get_description;
	iface->get_executable=_xfdashboard_desktop_app_info_gappinfo_get_executable;
	iface->get_icon=_xfdashboard_desktop_app_info_gappinfo_get_icon;
	iface->launch=_xfdashboard_desktop_app_info_gappinfo_launch;
	iface->supports_uris=_xfdashboard_desktop_app_info_gappinfo_supports_uris;
	iface->supports_files=_xfdashboard_desktop_app_info_gappinfo_supports_files;
	iface->launch_uris=_xfdashboard_desktop_app_info_gappinfo_launch_uris;
	iface->should_show=_xfdashboard_desktop_app_info_gappinfo_should_show;
	iface->get_commandline=_xfdashboard_desktop_app_info_gappinfo_get_commandline;
	iface->get_display_name=_xfdashboard_desktop_app_info_gappinfo_get_display_name;
}

/* IMPLEMENTATION: GObject */

/* Dispose this object */
static void _xfdashboard_desktop_app_info_dispose(GObject *inObject)
{
	XfdashboardDesktopAppInfo			*self=XFDASHBOARD_DESKTOP_APP_INFO(inObject);
	XfdashboardDesktopAppInfoPrivate	*priv=self->priv;

	/* Release allocated variables */
	if(priv->keywords)
	{
		g_list_free_full(priv->keywords, g_free);
		priv->keywords=NULL;
	}
	priv->needKeywords=TRUE;

	if(priv->actions)
	{
		g_list_free_full(priv->actions, g_object_unref);
		priv->actions=NULL;
	}
	priv->needActions=TRUE;

	if(priv->binaryExecutable)
	{
		g_free(priv->binaryExecutable);
		priv->binaryExecutable=NULL;
	}

	if(priv->item)
	{
		if(priv->itemChangedID)
		{
			g_signal_handler_disconnect(priv->item, priv->itemChangedID);
			priv->itemChangedID=0;
		}

		g_object_unref(priv->item);
		priv->item=NULL;
	}

	if(priv->secondarySource)
	{
		g_key_file_unref(priv->secondarySource);
		priv->secondarySource=NULL;
	}

	if(priv->file)
	{
		g_object_unref(priv->file);
		priv->file=NULL;
	}

	if(priv->desktopID)
	{
		g_free(priv->desktopID);
		priv->desktopID=NULL;
	}

	/* Call parent's class dispose method */
	G_OBJECT_CLASS(xfdashboard_desktop_app_info_parent_class)->dispose(inObject);
}

/* Set/get properties */
static void _xfdashboard_desktop_app_info_set_property(GObject *inObject,
														guint inPropID,
														const GValue *inValue,
														GParamSpec *inSpec)
{
	XfdashboardDesktopAppInfo			*self=XFDASHBOARD_DESKTOP_APP_INFO(inObject);

	switch(inPropID)
	{
		case PROP_DESKTOP_ID:
			_xfdashboard_desktop_app_info_set_desktop_id(self, g_value_get_string(inValue));
			break;

		case PROP_FILE:
			_xfdashboard_desktop_app_info_set_file(self, G_FILE(g_value_get_object(inValue)));
			break;

		default:
			G_OBJECT_WARN_INVALID_PROPERTY_ID(inObject, inPropID, inSpec);
			break;
	}
}

static void _xfdashboard_desktop_app_info_get_property(GObject *inObject,
														guint inPropID,
														GValue *outValue,
														GParamSpec *inSpec)
{
	XfdashboardDesktopAppInfo			*self=XFDASHBOARD_DESKTOP_APP_INFO(inObject);
	XfdashboardDesktopAppInfoPrivate	*priv=self->priv;

	switch(inPropID)
	{
		case PROP_VALID:
			g_value_set_boolean(outValue, priv->isValid);
			break;

		case PROP_DESKTOP_ID:
			g_value_set_string(outValue, priv->desktopID);
			break;

		case PROP_FILE:
			g_value_set_object(outValue, priv->file);
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
static void xfdashboard_desktop_app_info_class_init(XfdashboardDesktopAppInfoClass *klass)
{
	GObjectClass					*gobjectClass=G_OBJECT_CLASS(klass);

	/* Override functions */
	gobjectClass->dispose=_xfdashboard_desktop_app_info_dispose;
	gobjectClass->set_property=_xfdashboard_desktop_app_info_set_property;
	gobjectClass->get_property=_xfdashboard_desktop_app_info_get_property;

	/* Define properties */
	XfdashboardDesktopAppInfoProperties[PROP_VALID]=
		g_param_spec_boolean("valid",
								_("Valid"),
								_("Flag indicating whether this desktop application information is valid or not"),
								FALSE,
								G_PARAM_READABLE | G_PARAM_STATIC_STRINGS);

	XfdashboardDesktopAppInfoProperties[PROP_DESKTOP_ID]=
		g_param_spec_string("desktop-id",
								_("Desktop ID"),
								_("Name of desktop ID"),
								NULL,
								G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS);

	XfdashboardDesktopAppInfoProperties[PROP_FILE]=
		g_param_spec_object("file",
							_("File"),
							_("The desktop file"),
							G_TYPE_FILE,
							G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS);

	g_object_class_install_properties(gobjectClass, PROP_LAST, XfdashboardDesktopAppInfoProperties);

	/* Define signals */
	XfdashboardDesktopAppInfoSignals[SIGNAL_CHANGED]=
		g_signal_new("changed",
						G_TYPE_FROM_CLASS(klass),
						G_SIGNAL_RUN_FIRST,
						G_STRUCT_OFFSET(XfdashboardDesktopAppInfoClass, changed),
						NULL,
						NULL,
						g_cclosure_marshal_VOID__VOID,
						G_TYPE_NONE,
						0);
}

/* Object initialization
 * Create private structure and set up default values
 */
static void xfdashboard_desktop_app_info_init(XfdashboardDesktopAppInfo *self)
{
	XfdashboardDesktopAppInfoPrivate	*priv;

	priv=self->priv=xfdashboard_desktop_app_info_get_instance_private(self);

	/* Set up default values */
	priv->inited=FALSE;
	priv->isValid=FALSE;
	priv->desktopID=NULL;
	priv->file=NULL;
	priv->item=NULL;
	priv->itemChangedID=0;
	priv->binaryExecutable=NULL;
	priv->actions=NULL;
	priv->needActions=TRUE;
	priv->keywords=NULL;
	priv->needKeywords=TRUE;
}

/* IMPLEMENTATION: Public API */

/* Create new instance */
GAppInfo* xfdashboard_desktop_app_info_new_from_desktop_id(const gchar *inDesktopID)
{
	XfdashboardDesktopAppInfo		*instance;
	gchar							*desktopFilename;
	GFile							*file;

	g_return_val_if_fail(inDesktopID && *inDesktopID, NULL);

	instance=NULL;

	/* Find desktop file for desktop ID */
	desktopFilename=xfdashboard_application_database_get_file_from_desktop_id(inDesktopID);
	if(!desktopFilename)
	{
		g_warning(_("Desktop ID '%s' not found"), inDesktopID);
		return(NULL);
	}

	/* Create this class instance for desktop file found */
	file=g_file_new_for_path(desktopFilename);
	instance=XFDASHBOARD_DESKTOP_APP_INFO(g_object_new(XFDASHBOARD_TYPE_DESKTOP_APP_INFO,
														"desktop-id", inDesktopID,
														"file", file,
														NULL));
	XFDASHBOARD_DEBUG(instance, APPLICATIONS,
						"Created %s desktop file '%s' for desktop ID '%s'",
						G_OBJECT_TYPE_NAME(instance),
						desktopFilename,
						inDesktopID);
	if(file) g_object_unref(file);

	/* Release allocated resources */
	if(desktopFilename) g_free(desktopFilename);

	/* Return created instance */
	return(G_APP_INFO(instance));
}

GAppInfo* xfdashboard_desktop_app_info_new_from_path(const gchar *inPath)
{
	XfdashboardDesktopAppInfo	*instance;
	GFile						*file;

	g_return_val_if_fail(inPath && *inPath, NULL);

	/* Create this class instance for given URI */
	file=g_file_new_for_path(inPath);
	instance=XFDASHBOARD_DESKTOP_APP_INFO(g_object_new(XFDASHBOARD_TYPE_DESKTOP_APP_INFO,
														"file", file,
														NULL));
	if(file) g_object_unref(file);

	/* Return created instance */
	return(G_APP_INFO(instance));
}

GAppInfo* xfdashboard_desktop_app_info_new_from_file(GFile *inFile)
{
	g_return_val_if_fail(G_IS_FILE(inFile), NULL);

	/* Return created instance for given file object */
	return(G_APP_INFO(g_object_new(XFDASHBOARD_TYPE_DESKTOP_APP_INFO,
									"file", inFile,
									NULL)));
}

GAppInfo* xfdashboard_desktop_app_info_new_from_menu_item(GarconMenuItem *inMenuItem)
{
	XfdashboardDesktopAppInfo	*instance;
	const gchar					*desktopID;
	GFile						*file;

	g_return_val_if_fail(GARCON_IS_MENU_ITEM(inMenuItem), NULL);

	/* Create this class instance from menu item loaded from given URI */
	instance=XFDASHBOARD_DESKTOP_APP_INFO(g_object_new(XFDASHBOARD_TYPE_DESKTOP_APP_INFO, NULL));

	/* Set menu item but increase reference counter */
	instance->priv->item=GARCON_MENU_ITEM(g_object_ref(inMenuItem));

	/* Copy desktop ID from menu item if available */
	desktopID=garcon_menu_item_get_desktop_id(inMenuItem);
	if(desktopID) g_object_set(instance, "desktop-id", desktopID, NULL);

	/* Copy file object and do not use g_object_set to set it
	 * in created instance to prevent the property setter function
	 * _xfdashboard_desktop_app_info_set_file to be called which
	 * would unreference the just referenced menu item.
	 */
	file=garcon_menu_item_get_file(inMenuItem);
	instance->priv->file=G_FILE(g_object_ref(file));
	g_object_unref(file);

	/* Desktop app info is inited now */
	instance->priv->inited=TRUE;

	/* Return created instance */
	return(G_APP_INFO(instance));
}

/* Determine if desktop app info is valid */
gboolean xfdashboard_desktop_app_info_is_valid(XfdashboardDesktopAppInfo *self)
{
	g_return_val_if_fail(XFDASHBOARD_IS_DESKTOP_APP_INFO(self), FALSE);

	return(self->priv->isValid);
}

/* Get file of desktop app info */
GFile* xfdashboard_desktop_app_info_get_file(XfdashboardDesktopAppInfo *self)
{
	g_return_val_if_fail(XFDASHBOARD_IS_DESKTOP_APP_INFO(self), NULL);

	return(self->priv->file);
}

/* Reload desktop app info */
gboolean xfdashboard_desktop_app_info_reload(XfdashboardDesktopAppInfo *self)
{
	XfdashboardDesktopAppInfoPrivate	*priv;
	gboolean							success;

	g_return_val_if_fail(XFDASHBOARD_IS_DESKTOP_APP_INFO(self), FALSE);

	priv=self->priv;
	success=FALSE;

	/* Release secondary source if available to enforce reload when updating
	 * data depending secondard source.
	 */
	if(priv->secondarySource)
	{
		g_key_file_unref(priv->secondarySource);
		priv->secondarySource=NULL;
	}

	/* Reload menu item */
	if(priv->item)
	{
		GError							*error;

		error=NULL;
		success=garcon_menu_item_reload(priv->item, NULL, &error);
		if(!success)
		{
			g_warning(_("Could not reload desktop application information for '%s': %s"),
						garcon_menu_item_get_name(priv->item),
						error ? error->message : _("Unknown error"));
			if(error) g_error_free(error);
		}

		/* Update path to executable file for this application */
		_xfdashboard_desktop_app_info_update_binary_executable(self);

		/* Set flag to reload application actions and keywords. They will be
		 * cleared and (re-)loaded on-demand.
		 */
		priv->needActions=TRUE;
		priv->needKeywords=TRUE;
	}

	/* If reload was successful emit changed signal */
	if(success)
	{
		g_signal_emit(self, XfdashboardDesktopAppInfoSignals[SIGNAL_CHANGED], 0);
	}

	/* Set valid flag depending on reload success but only if changed */
	if(priv->isValid!=success)
	{
		/* Set value */
		priv->isValid=success;

		/* Notify about property change */
		g_object_notify_by_pspec(G_OBJECT(self), XfdashboardDesktopAppInfoProperties[PROP_VALID]);
	}

	/* Return success result */
	return(success);
}

/* Get list of application actions of desktop app info */
GList* xfdashboard_desktop_app_info_get_actions(XfdashboardDesktopAppInfo *self)
{
	g_return_val_if_fail(XFDASHBOARD_IS_DESKTOP_APP_INFO(self), FALSE);

	/* Update list of application actions */
	_xfdashboard_desktop_app_info_update_actions(self);

	/* Return the list of application actions */
	return(self->priv->actions);
}

/* Launch application action of desktop app info */
gboolean xfdashboard_desktop_app_info_launch_action(XfdashboardDesktopAppInfo *self,
													XfdashboardDesktopAppInfoAction *inAction,
													GAppLaunchContext *inContext,
													GError **outError)
{
	const gchar						*actionName;
	gboolean						success;

	g_return_val_if_fail(XFDASHBOARD_IS_DESKTOP_APP_INFO(self), FALSE);
	g_return_val_if_fail(XFDASHBOARD_IS_DESKTOP_APP_INFO_ACTION(inAction), FALSE);
	g_return_val_if_fail(!inContext || G_IS_APP_LAUNCH_CONTEXT(inContext), FALSE);
	g_return_val_if_fail(outError && *outError==NULL, FALSE);

	/* Launch by application action's name as it will lookup a maybe updated
	 * action, e.g. when reloaded in the meantime.
	 */
	actionName=xfdashboard_desktop_app_info_action_get_name(inAction);
	success=xfdashboard_desktop_app_info_launch_action_by_name(self,
																actionName,
																inContext,
																outError);

	/* Return success result */
	return(success);
}

gboolean xfdashboard_desktop_app_info_launch_action_by_name(XfdashboardDesktopAppInfo *self,
															const gchar *inActionName,
															GAppLaunchContext *inContext,
															GError **outError)
{
	XfdashboardDesktopAppInfoPrivate	*priv;
	XfdashboardDesktopAppInfoAction		*action;
	GList								*iter;
	gboolean							success;

	g_return_val_if_fail(XFDASHBOARD_IS_DESKTOP_APP_INFO(self), FALSE);
	g_return_val_if_fail(inActionName && *inActionName, FALSE);
	g_return_val_if_fail(!inContext || G_IS_APP_LAUNCH_CONTEXT(inContext), FALSE);
	g_return_val_if_fail(outError && *outError==NULL, FALSE);

	priv=self->priv;

	/* Find application action data by name */
	action=NULL;
	for(iter=priv->actions; iter && !action; iter=g_list_next(iter))
	{
		XfdashboardDesktopAppInfoAction	*iterAction;

		iterAction=XFDASHBOARD_DESKTOP_APP_INFO_ACTION(iter->data);
		if(!iterAction) continue;

		if(g_strcmp0(xfdashboard_desktop_app_info_action_get_name(iterAction), inActionName)==0)
		{
			action=iterAction;
		}
	}

	if(!action)
	{
		/* Set error */
		g_set_error(outError,
					G_IO_ERROR,
					G_IO_ERROR_NOT_FOUND,
					_("Invalid application action '%s' to execute for desktop ID '%s'"),
					inActionName,
					priv->desktopID);

		/* Return fail status */
		return(FALSE);
	}

	/* Launch application action found */
	success=_xfdashboard_desktop_app_info_launch_appinfo_internal(self,
																	xfdashboard_desktop_app_info_action_get_command(action),
																	NULL,
																	inContext,
																	outError);
	if(!success)
	{
		g_warning(_("Could launch action '%s' for desktop ID '%s': %s"),
					xfdashboard_desktop_app_info_action_get_name(action),
					self->priv->desktopID,
					(outError && *outError) ? (*outError)->message : _("Unknown error"));
	}

	/* Return success result of launching action */
	return(success);
}

/* Get list of keywords of desktop app info */
GList* xfdashboard_desktop_app_info_get_keywords(XfdashboardDesktopAppInfo *self)
{
	g_return_val_if_fail(XFDASHBOARD_IS_DESKTOP_APP_INFO(self), FALSE);

	/* Update list of keywords */
	_xfdashboard_desktop_app_info_update_keywords(self);

	/* Return the list of keywords */
	return(self->priv->keywords);
}
