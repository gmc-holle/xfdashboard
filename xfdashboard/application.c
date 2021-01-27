/*
 * application: The xfdashboard application
 * 
 * Copyright 2012-2020 Stephan Haller <nomad@froevel.de>
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

#include "application.h"

#include <glib/gi18n-lib.h>
#include <gtk/gtk.h>
#include <libxfce4ui/libxfce4ui.h>

#include <common/xfconf-settings.h>
#include <libxfdashboard/core.h>
#include <libxfdashboard/compat.h>
#include <libxfdashboard/debug.h>


/* Define this class in GObject system */
struct _XfdashboardApplicationPrivate
{
	/* Properties related */
	gboolean							isDaemon;

	/* Instance related */
	gboolean							initialized;
	gboolean							forcedNewInstance;

	XfdashboardCore						*core;

	XfceSMClient						*sessionManagementClient;
};

G_DEFINE_TYPE_WITH_PRIVATE(XfdashboardApplication,
							xfdashboard_application,
							G_TYPE_APPLICATION)

/* Properties */
enum
{
	PROP_0,

	PROP_DAEMONIZED,

	PROP_LAST
};

static GParamSpec* XfdashboardApplicationProperties[PROP_LAST]={ 0, };


/* IMPLEMENTATION: Private variables and methods */
#define XFDASHBOARD_APP_ID					"de.froevel.nomad.xfdashboard"

/* Forward declarations */
static void _xfdashboard_application_activate(GApplication *inApplication);

/* Quit application depending on daemon mode and force parameter */
static void _xfdashboard_application_quit_intern(XfdashboardApplication *self, gboolean inForceQuit)
{
	XfdashboardApplicationPrivate	*priv;
	gboolean						shouldQuit;

	g_return_if_fail(XFDASHBOARD_IS_APPLICATION(self));

	priv=self->priv;
	shouldQuit=FALSE;

	/* Check if we should really quit this instance */
	if(inForceQuit==TRUE || priv->isDaemon==FALSE) shouldQuit=TRUE;

	/* Do nothing if application is already quitting. This can happen if
	 * application is running in daemon mode (primary instance) and another
	 * instance was called with "quit" or "restart" parameter which would
	 * cause this function to be called twice.
	 */
	if(!priv->core || xfdashboard_core_is_quitting(priv->core)) return;

	/* If application is not in daemon mode or if forced is set to TRUE
	 * destroy all stage windows ...
	 */
	if(shouldQuit==TRUE)
	{
		/* If application is told to quit, set the restart style to something
		 * when it won't restart itself.
		 */
		if(priv->sessionManagementClient &&
			XFCE_IS_SM_CLIENT(priv->sessionManagementClient))
		{
			xfce_sm_client_set_restart_style(priv->sessionManagementClient, XFCE_SM_CLIENT_RESTART_NORMAL);
		}

		/* Really quit application here and now */
		if(priv->initialized)
		{
			/* Release extra reference on application which causes g_application_run()
			 * to exit when returning.
			 */
			g_application_release(G_APPLICATION(self));
		}
	}
		/* ... otherwise emit "suspend" signal */
		else
		{
			/* Only send signal if not suspended already */
			if(!xfdashboard_core_is_suspended(priv->core))
			{
				/* Suspend core instance */
				xfdashboard_core_suspend(priv->core);
			}
		}
}

/* The session is going to quit */
static void _xfdashboard_application_on_session_quit(XfdashboardApplication *self,
														gpointer inUserData)
{
	g_return_if_fail(XFDASHBOARD_IS_APPLICATION(self));
	g_return_if_fail(XFCE_IS_SM_CLIENT(inUserData));

	/* Force application to quit */
	XFDASHBOARD_DEBUG(self, MISC, "Received 'quit' from session management client - initiating shutdown");
	_xfdashboard_application_quit_intern(self, TRUE);
}

/* The core instance asks for suspend/resume support */
static gboolean _xfdashboard_application_on_core_can_suspend(XfdashboardApplication *self, gpointer inUserData)
{
	g_return_val_if_fail(XFDASHBOARD_IS_APPLICATION(self), FALSE);
	g_return_val_if_fail(XFDASHBOARD_IS_CORE(inUserData), FALSE);

	return(TRUE);
}

/* The core instance has requested to quit */
static void _xfdashboard_application_on_core_quit(XfdashboardApplication *self, gpointer inUserData)
{
	g_return_if_fail(XFDASHBOARD_IS_APPLICATION(self));
	g_return_if_fail(XFDASHBOARD_IS_CORE(inUserData));

	/* Quit application */
	_xfdashboard_application_quit_intern(self, FALSE);
}

/* The "quit" action was triggered */
static void _xfdashboard_application_on_simple_action_quit(XfdashboardApplication *self,
															GVariant *inParameter,
															gpointer inUserData)
{
	g_return_if_fail(XFDASHBOARD_IS_APPLICATION(self));
	g_return_if_fail(G_IS_SIMPLE_ACTION(inUserData));

	/* Force quit */
	_xfdashboard_application_quit_intern(self, TRUE);
}

/* Create and set up settings object for xfdashboard */
static void _xfdashboard_application_add_settings_path_list(GArray *inSearchPaths, const gchar *inPath, gboolean isFile)
{
	gchar								*normalizedPath;
	guint								i;
	gchar								*entry;

	g_return_if_fail(inSearchPaths);
	g_return_if_fail(inPath && *inPath);

	/* Normalize requested path to add to list of search paths that means
	 * that it should end with a directory seperator.
	 */
	if(!isFile && !g_str_has_suffix(inPath, G_DIR_SEPARATOR_S))
	{
		normalizedPath=g_strjoin(NULL, inPath, G_DIR_SEPARATOR_S, NULL);
	}
		else normalizedPath=g_strdup(inPath);

	/* Check if path is already in list of search paths */
	for(i=0; i<inSearchPaths->len; i++)
	{
		/* Get search path at iterator */
		entry=g_array_index(inSearchPaths, gchar*, i);

		/* If the path at iterator matches the requested one that it is
		 * already in list of search path, so return here with fail result.
		 */
		if(g_strcmp0(entry, normalizedPath)==0)
		{
			/* Release allocated resources */
			if(normalizedPath) g_free(normalizedPath);

			/* Return without modifying search path list */
			return;
		}
	}

	/* If we get here the requested path is not in list of search path and
	 * we can add it now.
	 */
	entry=g_strdup(normalizedPath);
	g_array_append_val(inSearchPaths, entry);

	/* Release allocated resources */
	if(normalizedPath) g_free(normalizedPath);
}

static XfdashboardSettings* _xfdashboard_application_create_settings(XfdashboardApplication *self)
{
	XfdashboardSettings			*settings;
	GArray						*pathFileList;
	const gchar					*environmentVariable;
	gchar						**themesSearchPaths;
	gchar						**pluginsSearchPaths;
	gchar						**bindingFilePaths;
	gchar						*entry;
	const gchar					*homeDirectory;

	g_return_val_if_fail(XFDASHBOARD_IS_APPLICATION(self), NULL);

	themesSearchPaths=NULL;
	pluginsSearchPaths=NULL;
	bindingFilePaths=NULL;

	/* Set up search path for themes */
	pathFileList=g_array_new(TRUE, TRUE, sizeof(gchar*));

	environmentVariable=g_getenv("XFDASHBOARD_THEME_PATH");
	if(environmentVariable)
	{
		gchar						**paths;
		gchar						**pathIter;

		pathIter=paths=g_strsplit(environmentVariable, ":", -1);
		while(*pathIter)
		{
			_xfdashboard_application_add_settings_path_list(pathFileList, *pathIter, FALSE);
			pathIter++;
		}
		g_strfreev(paths);
	}

	entry=g_build_filename(g_get_user_data_dir(), "themes", NULL);
	_xfdashboard_application_add_settings_path_list(pathFileList, entry, FALSE);
	g_free(entry);

	homeDirectory=g_get_home_dir();
	if(homeDirectory)
	{
		entry=g_build_filename(homeDirectory, ".themes", NULL);
		_xfdashboard_application_add_settings_path_list(pathFileList, entry, FALSE);
		g_free(entry);
	}

	entry=g_build_filename(PACKAGE_DATADIR, "themes", NULL);
	_xfdashboard_application_add_settings_path_list(pathFileList, entry, FALSE);
	g_free(entry);

	themesSearchPaths=(gchar**)(gpointer)g_array_free(pathFileList, FALSE);

	/* Set up search path for plugins */
	pathFileList=g_array_new(TRUE, TRUE, sizeof(gchar*));

	environmentVariable=g_getenv("XFDASHBOARD_PLUGINS_PATH");
	if(environmentVariable)
	{
		gchar						**paths;
		gchar						**pathIter;

		pathIter=paths=g_strsplit(environmentVariable, ":", -1);
		while(*pathIter)
		{
			_xfdashboard_application_add_settings_path_list(pathFileList, *pathIter, FALSE);
			pathIter++;
		}
		g_strfreev(paths);
	}

	entry=g_build_filename(g_get_user_data_dir(), "xfdashboard", "plugins", NULL);
	_xfdashboard_application_add_settings_path_list(pathFileList, entry, FALSE);
	g_free(entry);

	entry=g_build_filename(PACKAGE_LIBDIR, "xfdashboard", "plugins", NULL);
	_xfdashboard_application_add_settings_path_list(pathFileList, entry, FALSE);
	g_free(entry);

	pluginsSearchPaths=(gchar**)(gpointer)g_array_free(pathFileList, FALSE);

	/* Set up file path for bindings */
	pathFileList=g_array_new(TRUE, TRUE, sizeof(gchar*));

	entry=g_build_filename(PACKAGE_DATADIR, "xfdashboard", "bindings.xml", NULL);
	_xfdashboard_application_add_settings_path_list(pathFileList, entry, TRUE);
	g_free(entry);

	entry=g_build_filename(g_get_user_config_dir(), "xfdashboard", "bindings.xml", NULL);
	_xfdashboard_application_add_settings_path_list(pathFileList, entry, TRUE);
	g_free(entry);

	environmentVariable=g_getenv("XFDASHBOARD_BINDINGS_POOL_FILE");
	if(environmentVariable)
	{
		gchar						**paths;
		gchar						**pathIter;

		pathIter=paths=g_strsplit(environmentVariable, ":", -1);
		while(*pathIter)
		{
			_xfdashboard_application_add_settings_path_list(pathFileList, *pathIter, TRUE);
			pathIter++;
		}
		g_strfreev(paths);
	}

	bindingFilePaths=(gchar**)(gpointer)g_array_free(pathFileList, FALSE);

	/* Create settings instance for Xfconf settings storage */
	settings=g_object_new(XFDASHBOARD_TYPE_XFCONF_SETTINGS,
							"binding-files", bindingFilePaths,
							"theme-search-paths", themesSearchPaths,
							"plugin-search-paths", pluginsSearchPaths,
							NULL);

	/* Release allocated resources */
	if(themesSearchPaths) g_strfreev(themesSearchPaths);
	if(pluginsSearchPaths) g_strfreev(pluginsSearchPaths);
	if(bindingFilePaths) g_strfreev(bindingFilePaths);

	/* Return settings instance regardless if it could be
	 * created or not (NULL pointer).
	 */
	return(settings);
}

/* Perform full initialization of this application instance */
static gboolean _xfdashboard_application_initialize_full(XfdashboardApplication *self)
{
	XfdashboardApplicationPrivate	*priv;
	GError							*error;
	XfceSMClientRestartStyle		sessionManagementRestartStyle;
	XfdashboardSettings				*settings;

	g_return_val_if_fail(XFDASHBOARD_IS_APPLICATION(self), FALSE);

	priv=self->priv;
	error=NULL;

	/* Set up the session management */
	g_assert(priv->sessionManagementClient==NULL);

	sessionManagementRestartStyle=XFCE_SM_CLIENT_RESTART_IMMEDIATELY;
	if(priv->forcedNewInstance) sessionManagementRestartStyle=XFCE_SM_CLIENT_RESTART_NORMAL;

	priv->sessionManagementClient=xfce_sm_client_get();
	xfce_sm_client_set_priority(priv->sessionManagementClient, XFCE_SM_CLIENT_PRIORITY_DEFAULT);
	xfce_sm_client_set_restart_style(priv->sessionManagementClient, sessionManagementRestartStyle);
	g_signal_connect_swapped(priv->sessionManagementClient, "quit", G_CALLBACK(_xfdashboard_application_on_session_quit), self);

	if(!xfce_sm_client_connect(priv->sessionManagementClient, &error))
	{
		g_warning("Failed to connect to session manager: %s",
					(error && error->message) ? error->message : "unknown error");
		g_clear_error(&error);
	}

	/* Set up core instance */
	g_assert(priv->core==NULL);

	settings=_xfdashboard_application_create_settings(self);
	if(!settings)
	{
		g_critical("Could not set up runtime settings for xfdashboard");
		return(FALSE);
	}

	priv->core=g_object_new(XFDASHBOARD_TYPE_CORE,
								"settings", settings,
								NULL);
	if(!priv->core)
	{
		g_critical("Could not create core instance for xfdashboard");
		return(FALSE);
	}

	g_signal_connect_swapped(priv->core, "can-suspend", G_CALLBACK(_xfdashboard_application_on_core_can_suspend), self);
	g_signal_connect_swapped(priv->core, "quit", G_CALLBACK(_xfdashboard_application_on_core_quit), self);

	if(!xfdashboard_core_initialize(priv->core, &error))
	{
		g_warning("Failed to set up core instance for xfdashboard: %s",
					(error && error->message) ? error->message : "unknown error");
		g_clear_error(&error);
		return(FALSE);
	}

	return(TRUE);
}

/* Switch to requested view */
static void _xfdashboard_application_switch_to_view(XfdashboardApplication *self, const gchar *inInternalViewName)
{
	XfdashboardApplicationPrivate	*priv;
	XfdashboardStage				*stage;

	g_return_if_fail(XFDASHBOARD_IS_APPLICATION(self));

	priv=self->priv;

	/* If no view name was specified then do nothing and return immediately */
	if(!inInternalViewName ||
		!inInternalViewName[0])
	{
		XFDASHBOARD_DEBUG(self, MISC, "No view to switch to specified");
		return;
	}

	/* Tell core to switch requested view */
	XFDASHBOARD_DEBUG(self, MISC,
						"Trying to switch to view '%s'",
						inInternalViewName);

	stage=xfdashboard_core_get_stage(priv->core);
	if(!stage)
	{
		g_critical("No stage found to switch to view '%s'", inInternalViewName);
		return;
	}

	xfdashboard_stage_set_switch_to_view(stage, inInternalViewName);
}

/* Handle command-line on primary instance */
static gint _xfdashboard_application_handle_command_line_arguments(XfdashboardApplication *self,
																	gint inArgc,
																	gchar **inArgv)
{
	XfdashboardApplicationPrivate	*priv;
	GOptionContext					*context;
	gboolean						result;
	GError							*error;
	gboolean						optionDaemonize;
	gboolean						optionQuit;
	gboolean						optionRestart;
	gboolean						optionToggle;
	gchar							*optionSwitchToView;
	gboolean						optionVersion;
	GOptionEntry					entries[]=
									{
										{ "daemonize", 'd', 0, G_OPTION_ARG_NONE, &optionDaemonize, N_("Fork to background"), NULL },
										{ "quit", 'q', 0, G_OPTION_ARG_NONE, &optionQuit, N_("Quit running instance"), NULL },
										{ "restart", 'r', 0, G_OPTION_ARG_NONE, &optionRestart, N_("Restart running instance"), NULL },
										{ "toggle", 't', 0, G_OPTION_ARG_NONE, &optionToggle, N_("Toggles visibility if running instance was started in daemon mode otherwise it quits running non-daemon instance"), NULL },
										{ "view", 0, 0, G_OPTION_ARG_STRING, &optionSwitchToView, N_("The ID of view to switch to on startup or resume"), "ID" },
										{ "version", 'v', 0, G_OPTION_ARG_NONE, &optionVersion, N_("Show version"), NULL },
										{ NULL }
									};

	g_return_val_if_fail(XFDASHBOARD_IS_APPLICATION(self), XFDASHBOARD_APPLICATION_ERROR_FAILED);

	priv=self->priv;
	error=NULL;

	/* Set up options */
	optionDaemonize=FALSE;
	optionQuit=FALSE;
	optionRestart=FALSE;
	optionToggle=FALSE;
	optionSwitchToView=NULL;
	optionVersion=FALSE;

	/* Setup command-line options */
	context=g_option_context_new(N_(""));
	g_option_context_set_summary(context, N_("A Gnome Shell like dashboard for Xfce4 - version " PACKAGE_VERSION));
	g_option_context_set_translation_domain(context, GETTEXT_PACKAGE);
	g_option_context_add_group(context, gtk_get_option_group(TRUE));
	g_option_context_add_group(context, clutter_get_option_group_without_init());
	g_option_context_add_group(context, xfce_sm_client_get_option_group(inArgc, inArgv));
	g_option_context_add_main_entries(context, entries, GETTEXT_PACKAGE);

#ifdef DEBUG
#ifdef XFDASHBOARD_ENABLE_DEBUG
	g_print("** Use environment variable XFDASHBOARD_DEBUG to enable debug messages\n");
	g_print("** To get a list of debug categories set XFDASHBOARD_DEBUG=help\n");
#endif
#endif

	if(!g_option_context_parse(context, &inArgc, &inArgv, &error))
	{
		/* Show error */
		g_print(N_("%s\n"),
				(error && error->message) ? error->message : _("unknown error"));
		if(error)
		{
			g_error_free(error);
			error=NULL;
		}

		/* Release allocated resources */
		if(optionSwitchToView) g_free(optionSwitchToView);
		if(context) g_option_context_free(context);

		return(XFDASHBOARD_APPLICATION_ERROR_FAILED);
	}

	/* If this application instance is a remote instance do not handle any
	 * command-line argument. The arguments will be sent to the primary instance,
	 * handled there and the exit code will be sent back to the remote instance.
	 * We can check for remote instance with g_application_get_is_remote() because
	 * the application was tried to get registered either by local_command_line
	 * signal handler or by g_application_run().
	 */
	if(g_application_get_is_remote(G_APPLICATION(self)))
	{
		XFDASHBOARD_DEBUG(self, MISC, "Do not handle command-line parameters on remote application instance");

		/* One exception is "--version" */
		if(optionVersion)
		{
			g_print("Remote instance: %s-%s\n", PACKAGE_NAME, PACKAGE_VERSION);
		}

		/* Release allocated resources */
		if(optionSwitchToView) g_free(optionSwitchToView);
		if(context) g_option_context_free(context);

		/* No errors so far and no errors will happen as we do not handle
		 * any command-line arguments here.
		 */
		return(XFDASHBOARD_APPLICATION_ERROR_NONE);
	}
	XFDASHBOARD_DEBUG(self, MISC, "Handling command-line parameters on primary application instance");

	/* Handle options: restart
	 *
	 * First handle option 'restart' to quit this application early. Also return
	 * immediately with status XFDASHBOARD_APPLICATION_ERROR_RESTART to tell
	 * remote instance to perform a restart and to become the new primary instance.
	 * This option requires that application was initialized.
	 */
	if(optionRestart &&
		priv->initialized)
	{
		/* Quit existing instance for restart */
		XFDASHBOARD_DEBUG(self, MISC, "Received request to restart application!");
		_xfdashboard_application_quit_intern(self, TRUE);

		/* Release allocated resources */
		if(optionSwitchToView) g_free(optionSwitchToView);
		if(context) g_option_context_free(context);

		/* Return state to restart this applicationa */
		return(XFDASHBOARD_APPLICATION_ERROR_RESTART);
	}

	/* Handle options: quit
	 *
	 * Now check for the next application stopping option 'quit'. We check for it
	 * to quit this application early and to prevent further and unnecessary check
	 * for other options.
	 */
	if(optionQuit)
	{
		/* Quit existing instance */
		XFDASHBOARD_DEBUG(self, MISC, "Received request to quit running instance!");
		xfdashboard_application_quit_forced(self);

		/* Release allocated resources */
		if(optionSwitchToView) g_free(optionSwitchToView);
		if(context) g_option_context_free(context);

		return(XFDASHBOARD_APPLICATION_ERROR_QUIT);
	}

	/* Handle options: toggle
	 *
	 * Now check if we should toggle the state of application. That means
	 * if application is running daemon mode, resume it if suspended otherwise
	 * suspend it. If application is running but not in daemon just quit the
	 * application. If application was not inited yet, skip toggling state and
	 * perform a normal start-up.
	 */
	if(optionToggle &&
		priv->initialized)
	{
		/* If application is running in daemon mode, toggle between suspend/resume ... */
		if(priv->isDaemon)
		{
			if(xfdashboard_core_is_suspended(priv->core))
			{
				/* Switch to view if requested */
				_xfdashboard_application_switch_to_view(self, optionSwitchToView);

				/* Show application again */
				_xfdashboard_application_activate(G_APPLICATION(self));
			}
				else
				{
					/* Hide application */
					_xfdashboard_application_quit_intern(self, FALSE);
				}
		}
			/* ... otherwise if not running in daemon mode, just quit */
			else
			{
				/* Hide application */
				_xfdashboard_application_quit_intern(self, FALSE);
			}

		/* Release allocated resources */
		if(optionSwitchToView) g_free(optionSwitchToView);
		if(context) g_option_context_free(context);

		/* Stop here because option was handled and application does not get initialized */
		return(XFDASHBOARD_APPLICATION_ERROR_NONE);
	}

	/* Handle options: version
	 *
	 * Show the version of this (maybe daemonized) instance.
	 */
	if(optionVersion)
	{
		if(priv->isDaemon)
		{
			g_print("Daemon instance: %s-%s\n", PACKAGE_NAME, PACKAGE_VERSION);
			return(XFDASHBOARD_APPLICATION_ERROR_NONE);
		}
			else
			{
				g_print("Version: %s-%s\n", PACKAGE_NAME, PACKAGE_VERSION);
				return(XFDASHBOARD_APPLICATION_ERROR_QUIT);
			}
	}

	/* Check if this instance needs to be initialized fully and also handle
	 * daemonization if requested.
	 */
	if(!priv->initialized)
	{
		/* Perform full initialization of this application instance */
		result=_xfdashboard_application_initialize_full(self);
		if(result==FALSE) return(XFDASHBOARD_APPLICATION_ERROR_FAILED);

		/* Handle options: daemonize
		 *
		 * Check if application shoud run in daemon mode. A daemonized instance runs in
		 * background and does not present the stage initially. The application must not
		 * be initialized yet as it can only be done on start-up.
		 *
		 * Does not work if a new instance was forced.
		 */
		if(optionDaemonize)
		{
			if(!priv->forcedNewInstance)
			{
				priv->isDaemon=optionDaemonize;
				g_object_notify_by_pspec(G_OBJECT(self), XfdashboardApplicationProperties[PROP_DAEMONIZED]);

				if(priv->isDaemon)
				{
					xfdashboard_core_suspend(priv->core);
				}
			}
				else
				{
					g_warning("Cannot daemonized because a temporary new instance of application was forced.");
				}
		}

		/* Switch to view if requested */
		_xfdashboard_application_switch_to_view(self, optionSwitchToView);

		/* Show application if not started daemonized */
		if(!priv->isDaemon)
		{
			XfdashboardStage* 		stage;

			/* Get stage to show */
			stage=xfdashboard_core_get_stage(priv->core);
			if(!stage)
			{
				g_critical("No stage available to show at start-up of standalone application");

				return(XFDASHBOARD_APPLICATION_ERROR_FAILED);
			}

			/* Show stage */
			clutter_actor_show(CLUTTER_ACTOR(stage));
		}

		/* Take extra reference on the application to keep application in
		 * function g_application_run() alive when returning.
		 */
		g_application_hold(G_APPLICATION(self));
	}

	/* Check if this instance need to be activated. Is should only be done
	 * if instance is initialized
	 */
	if(priv->initialized)
	{
		/* Switch to view if requested */
		_xfdashboard_application_switch_to_view(self, optionSwitchToView);

		/* Show application */
		_xfdashboard_application_activate(G_APPLICATION(self));
	}

	/* Release allocated resources */
	if(optionSwitchToView) g_free(optionSwitchToView);
	if(context) g_option_context_free(context);

	/* All done successfully so return status code 0 for success */
	priv->initialized=TRUE;
	return(XFDASHBOARD_APPLICATION_ERROR_NONE);
}


/* IMPLEMENTATION: GApplication */

/* Received "activate" signal on primary instance */
static void _xfdashboard_application_activate(GApplication *inApplication)
{
	XfdashboardApplication			*self;
	XfdashboardApplicationPrivate	*priv;

	g_return_if_fail(XFDASHBOARD_IS_APPLICATION(inApplication));

	self=XFDASHBOARD_APPLICATION(inApplication);
	priv=self->priv;

	/* Resume core instance */
	if(xfdashboard_core_is_suspended(priv->core))
	{
		xfdashboard_core_resume(priv->core);
	}
}

/* Handle command-line on primary instance */
static int _xfdashboard_application_command_line(GApplication *inApplication, GApplicationCommandLine *inCommandLine)
{
	XfdashboardApplication			*self;
	gint							argc;
	gchar							**argv;
	gint							exitStatus;

	g_return_val_if_fail(XFDASHBOARD_IS_APPLICATION(inApplication), XFDASHBOARD_APPLICATION_ERROR_FAILED);

	self=XFDASHBOARD_APPLICATION(inApplication);

	/* Get number of command-line arguments and the arguments */
	argv=g_application_command_line_get_arguments(inCommandLine, &argc);

	/* Parse command-line and get exit status code */
	exitStatus=_xfdashboard_application_handle_command_line_arguments(self, argc, argv);

	/* Release allocated resources */
	if(argv) g_strfreev(argv);

	/* Return exit status code */
	return(exitStatus);
}

/* Check and handle command-line on local instance regardless if this one
 * is the primary instance or a remote one. This functions checks for arguments
 * which can be handled locally, e.g. '--help'. Otherwise they will be send to
 * the primary instance.
 */
static gboolean _xfdashboard_application_local_command_line(GApplication *inApplication,
															gchar ***ioArguments,
															int *outExitStatus)
{
	XfdashboardApplication			*self;
	GError							*error;

	g_return_val_if_fail(XFDASHBOARD_IS_APPLICATION(inApplication), TRUE);

	self=XFDASHBOARD_APPLICATION(inApplication);
	error=NULL;

	/* Set exit status code initially to success result but can be changed later */
	if(outExitStatus) *outExitStatus=XFDASHBOARD_APPLICATION_ERROR_NONE;

	/* Try to register application to determine early if this instance will be
	 * the primary application instance or a remote one.
	 */
	if(!g_application_register(inApplication, NULL, &error))
	{
		g_critical("Unable to register application: %s",
					(error && error->message) ? error->message : "Unknown error");
		if(error)
		{
			g_error_free(error);
			error=NULL;
		}

		/* Set error status code */
		if(outExitStatus) *outExitStatus=XFDASHBOARD_APPLICATION_ERROR_FAILED;

		/* Return TRUE to indicate that command-line does not need further processing */
		return(TRUE);
	}

	/* If this is a remote instance we need to parse command-line now */
	if(g_application_get_is_remote(inApplication))
	{
		gint						argc;
		gchar						**argv;
		gchar						**originArgv;
		gint						exitStatus;
		gint						i;

		/* We have to make a extra copy of the command-line arguments, since
		 * g_option_context_parse() in command-line argument handling function
		 * might remove parameters from the arguments list and maybe we need
		 * them to sent the arguments to primary instance if not handled locally
		 * like '--help'.
		 */
		originArgv=*ioArguments;

		argc=g_strv_length(originArgv);
		argv=g_new0(gchar*, argc+1);
		for(i=0; i<=argc; i++) argv[i]=g_strdup(originArgv[i]);

		/* Parse command-line and store exit status code */
		exitStatus=_xfdashboard_application_handle_command_line_arguments(self, argc, argv);
		if(outExitStatus) *outExitStatus=exitStatus;

		/* Release allocated resources */
		if(argv) g_strfreev(argv);

		/* If exit status code indicates an error then return TRUE here to indicate
		 * that command-line does not need further processing.
		 */
		if(exitStatus==XFDASHBOARD_APPLICATION_ERROR_FAILED) return(TRUE);
	}

	/* Return FALSE to indicate that command-line was not completely handled
	 * and needs further processing, e.g. this is the primary instance or a remote
	 * instance which could not handle the arguments locally.
	 */
	return(FALSE);
}


/* IMPLEMENTATION: GObject */

/* Dispose this object */
static void _xfdashboard_application_dispose(GObject *inObject)
{
	XfdashboardApplication			*self=XFDASHBOARD_APPLICATION(inObject);
	XfdashboardApplicationPrivate	*priv=self->priv;

	/* Release allocated resources */
	if(priv->core)
	{
		g_object_unref(priv->core);
		priv->core=NULL;
	}

	/* Shutdown session management */
	if(priv->sessionManagementClient)
	{
		/* This instance looks like to be disposed normally and not like a crash
		 * so set the restart style at session management to something that it
		 * will not restart itself but shutting down.
		 */
		if(XFCE_IS_SM_CLIENT(priv->sessionManagementClient))
		{
			xfce_sm_client_set_restart_style(priv->sessionManagementClient, XFCE_SM_CLIENT_RESTART_NORMAL);
		}

		priv->sessionManagementClient=NULL;
	}

	/* Call parent's class dispose method */
	G_OBJECT_CLASS(xfdashboard_application_parent_class)->dispose(inObject);
}

/* Set/get properties */
static void _xfdashboard_application_get_property(GObject *inObject,
													guint inPropID,
													GValue *outValue,
													GParamSpec *inSpec)
{
	XfdashboardApplication	*self=XFDASHBOARD_APPLICATION(inObject);

	switch(inPropID)
	{
		case PROP_DAEMONIZED:
			g_value_set_boolean(outValue, self->priv->isDaemon);
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
static void xfdashboard_application_class_init(XfdashboardApplicationClass *klass)
{
	GApplicationClass	*appClass=G_APPLICATION_CLASS(klass);
	GObjectClass		*gobjectClass=G_OBJECT_CLASS(klass);

	/* Override functions */
	appClass->activate=_xfdashboard_application_activate;
	appClass->command_line=_xfdashboard_application_command_line;
	appClass->local_command_line=_xfdashboard_application_local_command_line;

	gobjectClass->dispose=_xfdashboard_application_dispose;
	gobjectClass->get_property=_xfdashboard_application_get_property;

	/* Define properties */
	/**
	 * XfdashboardApplication:is-daemonized:
	 *
	 * A flag indicating if application is running in daemon mode. It is set to
	 * %TRUE if it running in daemon mode otherwise %FALSE as it running in
	 * standalone application.
	 */
	XfdashboardApplicationProperties[PROP_DAEMONIZED]=
		g_param_spec_boolean("is-daemonized",
								"Is daemonized",
								"Flag indicating if application is daemonized",
								FALSE,
								G_PARAM_READABLE | G_PARAM_STATIC_STRINGS);
}

/* Object initialization
 * Create private structure and set up default values
 */
static void xfdashboard_application_init(XfdashboardApplication *self)
{
	XfdashboardApplicationPrivate	*priv;
	GSimpleAction					*action;

	priv=self->priv=xfdashboard_application_get_instance_private(self);

	/* Set default values */
	priv->isDaemon=FALSE;
	priv->initialized=FALSE;
	priv->sessionManagementClient=NULL;
	priv->forcedNewInstance=FALSE;

	/* Add callable DBUS actions for this application */
	action=g_simple_action_new("Quit", NULL);
	g_signal_connect_swapped(action, "activate", G_CALLBACK(_xfdashboard_application_on_simple_action_quit), self);
	g_action_map_add_action(G_ACTION_MAP(self), G_ACTION(action));
	g_object_unref(action);
}


/* IMPLEMENTATION: Public API */

/**
 * xfdashboard_application_new:
 *
 * Creates an instance of #XfdashboardApplication.
 *
 * Return value: (transfer none): The instance of #XfdashboardApplication.
 */
XfdashboardApplication* xfdashboard_application_new(void)
{
	XfdashboardApplication		*application;
	gchar						*appID;
	const gchar					*forceNewInstance;

	application=NULL;
	forceNewInstance=NULL;

#ifdef DEBUG
	/* If a new instance of xfdashboard is forced, e.g. for debugging purposes,
	 * then create a unique application ID.
	 */
	forceNewInstance=g_getenv("XFDASHBOARD_FORCE_NEW_INSTANCE");
#endif

	if(forceNewInstance)
	{
		appID=g_strdup_printf("%s-%u", XFDASHBOARD_APP_ID, getpid());
		g_message("Forcing new application instance with ID '%s'", appID);
	}
		else appID=g_strdup(XFDASHBOARD_APP_ID);

	application=XFDASHBOARD_APPLICATION(g_object_new(XFDASHBOARD_TYPE_APPLICATION,
														"application-id", appID,
														"flags", G_APPLICATION_HANDLES_COMMAND_LINE,
														NULL));

	/* Release allocated resources */
	if(appID) g_free(appID);

	/* Return newly created application instance */
	return(application);
}

/**
 * xfdashboard_application_is_daemonized:
 * @self: A #XfdashboardApplication
 *
 * Checks if application is running in background (daemon mode).
 *
 * Return value: %TRUE if @self is running in background and
 *   %FALSE if it is running as standalone application.
 */
gboolean xfdashboard_application_is_daemonized(XfdashboardApplication *self)
{
	g_return_val_if_fail(XFDASHBOARD_IS_APPLICATION(self), FALSE);

	return(self->priv->isDaemon);
}

/**
 * xfdashboard_application_quit_forced:
 * @self: A #XfdashboardApplication
 *
 * Quits @self regardless if it is running as standalone application
 * or in daemon mode. The application is really quitted after calling
 * this function.
 */
void xfdashboard_application_quit_forced(XfdashboardApplication *self)
{
	g_return_if_fail(XFDASHBOARD_IS_APPLICATION(self));

	/* Quit also any other running instance */
	if(g_application_get_is_remote(G_APPLICATION(self))==TRUE)
	{
		g_action_group_activate_action(G_ACTION_GROUP(self), "Quit", NULL);
	}

	/* Quit this instance */
	_xfdashboard_application_quit_intern(self, TRUE);
}
