/*
 * application: Single-instance managing application and single-instance
 *              objects like window manager and so on.
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

#include "application.h"

#include <glib/gi18n-lib.h>
#include <clutter/x11/clutter-x11.h>
#include <gtk/gtk.h>
#include <garcon/garcon.h>
#include <libxfce4ui/libxfce4ui.h>

#include "stage.h"
#include "types.h"
#include "view-manager.h"
#include "applications-view.h"
#include "windows-view.h"
#include "search-view.h"
#include "search-manager.h"
#include "applications-search-provider.h"
#include "utils.h"
#include "theme.h"
#include "focus-manager.h"
#include "bindings-pool.h"
#include "application-database.h"
#include "application-tracker.h"
#include "plugins-manager.h"
#include "marshal.h"

/* Define this class in GObject system */
G_DEFINE_TYPE(XfdashboardApplication,
				xfdashboard_application,
				G_TYPE_APPLICATION)

/* Private structure - access only by public API if needed */
#define XFDASHBOARD_APPLICATION_GET_PRIVATE(obj) \
	(G_TYPE_INSTANCE_GET_PRIVATE((obj), XFDASHBOARD_TYPE_APPLICATION, XfdashboardApplicationPrivate))

struct _XfdashboardApplicationPrivate
{
	/* Properties related */
	gboolean						isDaemon;
	gboolean						isSuspended;
	gchar							*themeName;

	/* Instance related */
	gboolean						inited;
	gboolean						isQuitting;

	XfconfChannel					*xfconfChannel;
	XfdashboardViewManager			*viewManager;
	XfdashboardSearchManager		*searchManager;
	XfdashboardFocusManager			*focusManager;

	XfdashboardTheme				*theme;
	gulong							xfconfThemeChangedSignalID;

	XfdashboardBindingsPool			*bindings;

	XfdashboardApplicationDatabase	*appDatabase;
	XfdashboardApplicationTracker	*appTracker;

	XfceSMClient					*sessionManagementClient;

	XfdashboardPluginsManager		*pluginManager;
};

/* Properties */
enum
{
	PROP_0,

	PROP_DAEMONIZED,
	PROP_SUSPENDED,
	PROP_THEME_NAME,

	PROP_LAST
};

static GParamSpec* XfdashboardApplicationProperties[PROP_LAST]={ 0, };

/* Signals */
enum
{
	SIGNAL_QUIT,
	SIGNAL_SHUTDOWN_FINAL,
	SIGNAL_SUSPEND,
	SIGNAL_RESUME,
	SIGNAL_THEME_CHANGED,
	SIGNAL_APPLICATION_LAUNCHED,

	/* Actions */
	ACTION_EXIT,

	SIGNAL_LAST
};

static guint XfdashboardApplicationSignals[SIGNAL_LAST]={ 0, };

/* IMPLEMENTATION: Private variables and methods */
#define XFDASHBOARD_APP_ID					"de.froevel.nomad.xfdashboard"
#define XFDASHBOARD_XFCONF_CHANNEL			"xfdashboard"

#define THEME_NAME_XFCONF_PROP				"/theme"
#define DEFAULT_THEME_NAME					"xfdashboard"

/* Single instance of application */
static XfdashboardApplication*		_xfdashboard_application=NULL;

/* Forward declarations */
static void _xfdashboard_application_activate(GApplication *inApplication);

/* Quit application depending on daemon mode and force parameter */
static void _xfdashboard_application_quit(XfdashboardApplication *self, gboolean inForceQuit)
{
	XfdashboardApplicationPrivate	*priv;
	gboolean						shouldQuit;
	GSList							*stages, *entry;

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
	if(priv->isQuitting) return;

	/* If application is not in daemon mode or if forced is set to TRUE
	 * destroy all stage windows ...
	 */
	if(shouldQuit==TRUE)
	{
		/* Set flag that application is going to quit */
		priv->isQuitting=TRUE;

		/* If application is told to quit, set the restart style to something
		 * where it won't restart itself.
		 */
		if(priv->sessionManagementClient &&
			XFCE_IS_SM_CLIENT(priv->sessionManagementClient))
		{
			xfce_sm_client_set_restart_style(priv->sessionManagementClient, XFCE_SM_CLIENT_RESTART_NORMAL);
		}

		/* Destroy stages */
		stages=clutter_stage_manager_list_stages(clutter_stage_manager_get_default());
		for(entry=stages; entry!=NULL; entry=g_slist_next(entry)) clutter_actor_destroy(CLUTTER_ACTOR(entry->data));
		g_slist_free(stages);

		/* Emit "quit" signal */
		g_signal_emit(self, XfdashboardApplicationSignals[SIGNAL_QUIT], 0);

		/* Really quit application here and now */
		if(priv->inited) clutter_main_quit();
	}
		/* ... otherwise emit "suspend" signal */
		else
		{
			/* Only send signal if not suspended already */
			if(!priv->isSuspended)
			{
				/* Send signal */
				g_signal_emit(self, XfdashboardApplicationSignals[SIGNAL_SUSPEND], 0);

				/* Set flag for suspension */
				priv->isSuspended=TRUE;
				g_object_notify_by_pspec(G_OBJECT(self), XfdashboardApplicationProperties[PROP_SUSPENDED]);
			}
		}
}

/* Action "exit" was called at application */
static gboolean _xfdashboard_application_action_exit(XfdashboardApplication *self,
														XfdashboardFocusable *inSource,
														const gchar *inAction,
														ClutterEvent *inEvent)
{
	g_return_val_if_fail(XFDASHBOARD_IS_APPLICATION(self), CLUTTER_EVENT_PROPAGATE);

	/* Quit application */
	_xfdashboard_application_quit(self, FALSE);

	/* Prevent the default handler being called */
	return(CLUTTER_EVENT_STOP);
}

/* The session is going to quit */
static void _xfdashboard_application_on_session_quit(XfdashboardApplication *self,
														gpointer inUserData)
{
	g_return_if_fail(XFDASHBOARD_IS_APPLICATION(self));
	g_return_if_fail(XFCE_IS_SM_CLIENT(inUserData));

	/* Force application to quit */
	g_debug("Received 'quit' from session management client - initiating shutdown");
	_xfdashboard_application_quit(self, TRUE);
}

/* A stage window should be destroyed */
static gboolean _xfdashboard_application_on_delete_stage(XfdashboardApplication *self,
															ClutterEvent *inEvent,
															gpointer inUserData)
{
	g_return_val_if_fail(XFDASHBOARD_IS_APPLICATION(self), CLUTTER_EVENT_PROPAGATE);

	/* Quit application */
	_xfdashboard_application_quit(self, FALSE);

	/* Prevent the default handler being called */
	return(CLUTTER_EVENT_STOP);
}

/* Set theme name and reload theme */
static void _xfdashboard_application_set_theme_name(XfdashboardApplication *self, const gchar *inThemeName)
{
	XfdashboardApplicationPrivate	*priv;
	GError							*error;
	XfdashboardTheme				*theme;

	g_return_if_fail(XFDASHBOARD_IS_APPLICATION(self));
	g_return_if_fail(inThemeName && *inThemeName);

	priv=self->priv;
	error=NULL;

	/* Set value if changed */
	if(g_strcmp0(priv->themeName, inThemeName)!=0)
	{
		/* Set value */
		if(priv->themeName) g_free(priv->themeName);
		priv->themeName=g_strdup(inThemeName);

		/* Notify about property change */
		g_object_notify_by_pspec(G_OBJECT(self), XfdashboardApplicationProperties[PROP_THEME_NAME]);

		/* Create new theme instance and load theme */
		theme=xfdashboard_theme_new();
		if(!xfdashboard_theme_load(theme, priv->themeName, &error))
		{
			/* Show critical warning at console */
			g_critical(_("Could not load theme '%s': %s"),
						priv->themeName,
						(error && error->message) ? error->message : _("unknown error"));

			/* Show warning as notification */
			xfdashboard_notify(NULL,
								"dialog-error",
								_("Could not load theme '%s': %s"),
								priv->themeName,
								(error && error->message) ? error->message : _("unknown error"));

			/* Release allocated resources */
			if(error!=NULL) g_error_free(error);
			g_object_unref(theme);

			return;
		}

		/* Release current theme and store new one */
		if(priv->theme) g_object_unref(priv->theme);
		priv->theme=theme;

		/* Emit signal that theme has changed to get all top-level actors
		 * to apply new theme.
		 */
		g_signal_emit(self, XfdashboardApplicationSignals[SIGNAL_THEME_CHANGED], 0, priv->theme);
	}
}

/* Perform full initialization of this application instance */
static gboolean _xfdashboard_application_initialize_full(XfdashboardApplication *self, XfdashboardStage **outStage)
{
	XfdashboardApplicationPrivate	*priv;
	GError							*error;
	ClutterActor					*stage;
#if !GARCON_CHECK_VERSION(0,3,0)
	const gchar						*desktop;
#endif

	g_return_val_if_fail(XFDASHBOARD_IS_APPLICATION(self), FALSE);
	g_return_val_if_fail(outStage==NULL || *outStage==NULL, FALSE);

	priv=self->priv;
	error=NULL;

	/* Initialize garcon for current desktop environment */
#if !GARCON_CHECK_VERSION(0,3,0)
	desktop=g_getenv("XDG_CURRENT_DESKTOP");
	if(G_LIKELY(desktop==NULL))
	{
		/* If we could not determine current desktop environment
		 * assume Xfce as this application is developed for this DE.
		 */
		desktop="XFCE";
	}
		/* If desktop enviroment was found but has no name
		 * set NULL to get all menu items shown.
		 */
		else if(*desktop==0) desktop=NULL;

	garcon_set_environment(desktop);
#else
	garcon_set_environment_xdg(GARCON_ENVIRONMENT_XFCE);
#endif

	/* Setup the session management */
	priv->sessionManagementClient=xfce_sm_client_get();
	xfce_sm_client_set_priority(priv->sessionManagementClient, XFCE_SM_CLIENT_PRIORITY_DEFAULT);
	xfce_sm_client_set_restart_style(priv->sessionManagementClient, XFCE_SM_CLIENT_RESTART_IMMEDIATELY);
	g_signal_connect_swapped(priv->sessionManagementClient, "quit", G_CALLBACK(_xfdashboard_application_on_session_quit), self);

	if(!xfce_sm_client_connect(priv->sessionManagementClient, &error))
	{
		g_warning("Failed to connect to session manager: %s",
					(error && error->message) ? error->message : _("unknown error"));
		g_clear_error(&error);
	}

	/* Initialize xfconf */
	if(!xfconf_init(&error))
	{
		g_critical(_("Could not initialize xfconf: %s"),
					(error && error->message) ? error->message : _("unknown error"));
		if(error) g_error_free(error);
		return(FALSE);
	}

	priv->xfconfChannel=xfconf_channel_get(XFDASHBOARD_XFCONF_CHANNEL);

	/* Set up keyboard and pointer bindings */
	priv->bindings=xfdashboard_bindings_pool_get_default();
	if(!priv->bindings)
	{
		g_critical(_("Could not initialize bindings"));
		return(FALSE);
	}

	if(!xfdashboard_bindings_pool_load(priv->bindings, &error))
	{
		g_critical(_("Could not load bindings: %s"),
					(error && error->message) ? error->message : _("unknown error"));
		if(error!=NULL) g_error_free(error);
		return(FALSE);
	}

	/* Set up application database */
	priv->appDatabase=xfdashboard_application_database_get_default();
	if(!priv->appDatabase)
	{
		g_critical(_("Could not initialize application database"));
		return(FALSE);
	}

	if(!xfdashboard_application_database_load(priv->appDatabase, &error))
	{
		g_critical(_("Could not load application database: %s"),
					(error && error->message) ? error->message : _("unknown error"));
		if(error!=NULL) g_error_free(error);
		return(FALSE);
	}

	/* Set up application tracker */
	priv->appTracker=xfdashboard_application_tracker_get_default();
	if(!priv->appTracker)
	{
		g_critical(_("Could not initialize application tracker"));
		return(FALSE);
	}

	/* Set up and load theme */
	priv->xfconfThemeChangedSignalID=xfconf_g_property_bind(priv->xfconfChannel,
															THEME_NAME_XFCONF_PROP,
															G_TYPE_STRING,
															self,
															"theme-name");
	if(!priv->xfconfThemeChangedSignalID)
	{
		g_warning(_("Could not create binding between xfconf property and local resource for theme change notification."));
	}

	/* Set up default theme in Xfcond if property in channel does not exist
	 * because it indicates first start.
	 */
	if(!xfconf_channel_has_property(priv->xfconfChannel, THEME_NAME_XFCONF_PROP))
	{
		xfconf_channel_set_string(priv->xfconfChannel,
									THEME_NAME_XFCONF_PROP,
									DEFAULT_THEME_NAME);
	}

	/* At this time the theme must have been loaded, either because we
	 * set the default theme name because of missing theme property in
	 * xfconf channel or the value of xfconf channel property has been read
	 * and set when setting up binding (between xfconf property and local property)
	 * what caused a call to function to set theme name in this object
	 * and also caused a reload of theme.
	 * So if no theme object is set in this object then loading theme has
	 * failed and we have to return FALSE.
	 */
	if(!priv->theme) return(FALSE);

	/* Register built-in views (order of registration is important) */
	priv->viewManager=xfdashboard_view_manager_get_default();

	xfdashboard_view_manager_register(priv->viewManager, "windows", XFDASHBOARD_TYPE_WINDOWS_VIEW);
	xfdashboard_view_manager_register(priv->viewManager, "applications", XFDASHBOARD_TYPE_APPLICATIONS_VIEW);
	xfdashboard_view_manager_register(priv->viewManager, "search", XFDASHBOARD_TYPE_SEARCH_VIEW);

	/* Register built-in search providers */
	priv->searchManager=xfdashboard_search_manager_get_default();

	xfdashboard_search_manager_register(priv->searchManager, "applications", XFDASHBOARD_TYPE_APPLICATIONS_SEARCH_PROVIDER);

	/* Create single-instance of plugin manager to keep it alive while
	 * application is running.
	 */
	priv->pluginManager=xfdashboard_plugins_manager_get_default();
	if(!priv->pluginManager)
	{
		g_critical(_("Could not initialize plugin manager"));
		return(FALSE);
	}

	if(!xfdashboard_plugins_manager_setup(priv->pluginManager))
	{
		g_critical(_("Could not setup plugin manager"));
		return(FALSE);
	}

	/* Create single-instance of focus manager to keep it alive while
	 * application is running.
	 */
	priv->focusManager=xfdashboard_focus_manager_get_default();

	/* Create stage containing all monitors */
	stage=xfdashboard_stage_new();
	g_signal_connect_swapped(stage, "delete-event", G_CALLBACK(_xfdashboard_application_on_delete_stage), self);

	/* Emit signal 'theme-changed' to get current theme loaded at each stage created */
	g_signal_emit(self, XfdashboardApplicationSignals[SIGNAL_THEME_CHANGED], 0, priv->theme);

	/* Set return results */
	if(outStage) *outStage=XFDASHBOARD_STAGE(stage);

	/* Initialization was successful so return TRUE */
#ifdef DEBUG
	xfdashboard_notify(NULL, NULL, _("Welcome to %s (%s)!"), PACKAGE_NAME, PACKAGE_VERSION);
#else
	xfdashboard_notify(NULL, NULL, _("Welcome to %s!"), PACKAGE_NAME);
#endif
	return(TRUE);
}

/* Switch to requested view */
static void _xfdashboard_application_switch_to_view(XfdashboardApplication *self, const gchar *inInternalViewName)
{
	GSList							*stages, *iter;

	g_return_if_fail(XFDASHBOARD_IS_APPLICATION(self));

	/* If no view name was specified then do nothing and return immediately */
	if(!inInternalViewName ||
		!inInternalViewName[0])
	{
		g_debug("No view to switch to specified");
		return;
	}

	/* Iterate through list of stages and set specified view at stage */
	g_debug("Trying to switch to view '%s'", inInternalViewName);

	stages=clutter_stage_manager_list_stages(clutter_stage_manager_get_default());
	for(iter=stages; iter; iter=g_slist_next(iter))
	{
		/* Tell stage to switch view */
		if(XFDASHBOARD_IS_STAGE(iter->data))
		{
			xfdashboard_stage_set_switch_to_view(XFDASHBOARD_STAGE(iter->data),
													inInternalViewName);
		}
	}
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

	/* Emit "resume" signal */
	g_signal_emit(self, XfdashboardApplicationSignals[SIGNAL_RESUME], 0);

	/* Unset flag for suspension */
	if(priv->isSuspended)
	{
		priv->isSuspended=FALSE;
		g_object_notify_by_pspec(G_OBJECT(self), XfdashboardApplicationProperties[PROP_SUSPENDED]);
	}
}

/* Handle command-line on primary instance */
static int _xfdashboard_application_command_line(GApplication *inApplication, GApplicationCommandLine *inCommandLine)
{
	XfdashboardApplication			*self;
	XfdashboardApplicationPrivate	*priv;
	XfdashboardStage				*stage;
	GOptionContext					*context;
	gboolean						result;
	gint							argc;
	gchar							**argv;
	GError							*error;
	gboolean						optionDaemonize;
	gboolean						optionQuit;
	gboolean						optionRestart;
	gboolean						optionToggle;
	gchar							*optionSwitchToView;
	GOptionEntry					XfdashboardApplicationOptions[]=
									{
										{ "daemonize", 'd', 0, G_OPTION_ARG_NONE, &optionDaemonize, N_("Fork to background"), NULL },
										{ "quit", 'q', 0, G_OPTION_ARG_NONE, &optionQuit, N_("Quit running instance"), NULL },
										{ "restart", 'r', 0, G_OPTION_ARG_NONE, &optionRestart, N_("Restart running instance"), NULL },
										{ "toggle", 't', 0, G_OPTION_ARG_NONE, &optionToggle, N_("Toggles suspend/resume state if running instance was started in daemon mode otherwise it quits running non-daemon instance"), NULL },
										{ "view", 0, 0, G_OPTION_ARG_STRING, &optionSwitchToView, N_(""), NULL },
										{ NULL }
									};

	g_return_val_if_fail(XFDASHBOARD_IS_APPLICATION(inApplication), 1);

	self=XFDASHBOARD_APPLICATION(inApplication);
	priv=self->priv;
	error=NULL;
	stage=NULL;

	/* Set up options */
	optionDaemonize=FALSE;
	optionQuit=FALSE;
	optionRestart=FALSE;
	optionToggle=FALSE;
	optionSwitchToView=NULL;

	/* Parse command-line arguments */
	argv=g_application_command_line_get_arguments(inCommandLine, &argc);

	/* Setup command-line options */
	context=g_option_context_new(N_("- A Gnome Shell like dashboard for Xfce4"));
	g_option_context_add_group(context, gtk_get_option_group(TRUE));
	g_option_context_add_group(context, clutter_get_option_group_without_init());
	g_option_context_add_group(context, xfce_sm_client_get_option_group(argc, argv));
	g_option_context_add_main_entries(context, XfdashboardApplicationOptions, GETTEXT_PACKAGE);

#ifdef DEBUG
	/* I always forget the name of the environment variable to get the debug
	 * message display which are emitted with g_debug(). So display a hint
	 * if application was compiled with debug enabled.
	 */
	g_print("** To get debug messages set environment variable G_MESSAGES_DEBUG to %s\n", PACKAGE_NAME);
	g_print("** e.g.: G_MESSAGES_DEBUG=%s %s\n", PACKAGE_NAME, argv[0]);
#endif

	result=g_option_context_parse(context, &argc, &argv, &error);
	g_strfreev(argv);
	g_option_context_free(context);
	if(result==FALSE)
	{
		/* Show error */
		g_print(N_("%s\n"), (error && error->message) ? error->message : _("unknown error"));
		if(error) g_error_free(error);

		/* Release allocated resources */
		if(optionSwitchToView) g_free(optionSwitchToView);

		return(XFDASHBOARD_APPLICATION_ERROR_FAILED);
	}

	/* Handle options: restart
	 * - Only handle option if application was inited already
	 */
	if(priv->inited && optionRestart)
	{
		/* Return state to restart this applicationa */
		g_debug("Received request to restart application!");

		/* Release allocated resources */
		if(optionSwitchToView) g_free(optionSwitchToView);

		return(XFDASHBOARD_APPLICATION_ERROR_RESTART);
	}

	/* Handle options: quit */
	if(optionQuit)
	{
		/* Quit existing instance */
		g_debug("Quitting running instance!");
		_xfdashboard_application_quit(self, TRUE);

		/* Release allocated resources */
		if(optionSwitchToView) g_free(optionSwitchToView);

		return(XFDASHBOARD_APPLICATION_ERROR_QUIT);
	}

	/* Handle options: toggle
	 * - If application was not inited yet, perform normal start-up as usual
	 *   with command-line options given
	 * - If running in daemon mode, resume if suspended otherwise suspend
	 * - If not running in daemon mode, quit application
	 */
	if(priv->inited && optionToggle)
	{
		/* If application is running in daemon mode, toggle between suspend/resume ... */
		if(priv->isDaemon)
		{
			if(priv->isSuspended)
			{
				/* Switch to view if requested */
				_xfdashboard_application_switch_to_view(self, optionSwitchToView);

				/* Show application again */
				_xfdashboard_application_activate(inApplication);
			}
				else
				{
					/* Hide application */
					_xfdashboard_application_quit(self, FALSE);
				}
		}
			/* ... otherwise if not running in daemon mode, just quit */
			else
			{
				/* Hide application */
				_xfdashboard_application_quit(self, FALSE);
			}

		/* Release allocated resources */
		if(optionSwitchToView) g_free(optionSwitchToView);

		/* Stop here because option was handled and application does not get initialized */
		return(XFDASHBOARD_APPLICATION_ERROR_NONE);
	}

	/* Handle options: daemonize */
	if(!priv->inited)
	{
		priv->isDaemon=optionDaemonize;
		g_object_notify_by_pspec(G_OBJECT(self), XfdashboardApplicationProperties[PROP_DAEMONIZED]);

		if(priv->isDaemon)
		{
			priv->isSuspended=TRUE;
			g_object_notify_by_pspec(G_OBJECT(self), XfdashboardApplicationProperties[PROP_SUSPENDED]);
		}
	}

	/* Check if this instance needs to be initialized fully */
	if(!priv->inited)
	{
		/* Perform full initialization of this application instance */
		result=_xfdashboard_application_initialize_full(self, &stage);
		if(result==FALSE) return(XFDASHBOARD_APPLICATION_ERROR_FAILED);

		/* Switch to view if requested */
		_xfdashboard_application_switch_to_view(self, optionSwitchToView);

		/* Show application if not started daemonized */
		if(!priv->isDaemon) clutter_actor_show(CLUTTER_ACTOR(stage));
	}

	/* Check if this instance need to be activated. Is should only be done
	 * if instance is initialized
	 */
	if(priv->inited)
	{
		/* Switch to view if requested */
		_xfdashboard_application_switch_to_view(self, optionSwitchToView);

		/* Show application */
		_xfdashboard_application_activate(inApplication);
	}

	/* Release allocated resources */
	if(optionSwitchToView) g_free(optionSwitchToView);

	/* All done successfully so return status code 0 for success */
	priv->inited=TRUE;
	return(XFDASHBOARD_APPLICATION_ERROR_NONE);
}

#if GLIB_CHECK_VERSION(2, 40, 0)
/* Override local command-line handling in Glib 2.40 or higher to
 * get old behaviour of command-line handling as in Glib prior to
 * version 2.40 and as it is used in this application.
 */
static gboolean _xfdashboard_application_local_command_line(GApplication *inApplication,
															gchar ***ioArguments,
															int *outExitStatus)
{
	/* Return FALSE to indicate that command-line was not completely handled
	 * and need further processing.
	 */
	return(FALSE);
}
#endif


/* IMPLEMENTATION: GObject */

/* Dispose this object */
static void _xfdashboard_application_dispose(GObject *inObject)
{
	XfdashboardApplication			*self=XFDASHBOARD_APPLICATION(inObject);
	XfdashboardApplicationPrivate	*priv=self->priv;

	/* Ensure "is-quitting" flag is set just in case someone asks */
	priv->isQuitting=TRUE;

	/* Signal "shutdown-final" of application */
	g_signal_emit(self, XfdashboardApplicationSignals[SIGNAL_SHUTDOWN_FINAL], 0);

	/* Release allocated resources */
	if(priv->pluginManager)
	{
		g_object_unref(priv->pluginManager);
		priv->pluginManager=NULL;
	}

	if(priv->xfconfThemeChangedSignalID)
	{
		xfconf_g_property_unbind(priv->xfconfThemeChangedSignalID);
		priv->xfconfThemeChangedSignalID=0L;
	}

	if(priv->theme)
	{
		g_object_unref(priv->theme);
		priv->theme=NULL;
	}

	if(priv->themeName)
	{
		g_free(priv->themeName);
		priv->themeName=NULL;
	}

	if(priv->viewManager)
	{
		/* Unregisters all remaining registered views - no need to unregister them here */
		g_object_unref(priv->viewManager);
		priv->viewManager=NULL;
	}

	if(priv->searchManager)
	{
		/* Unregisters all remaining registered providers - no need to unregister them here */
		g_object_unref(priv->searchManager);
		priv->searchManager=NULL;
	}

	if(priv->focusManager)
	{
		/* Unregisters all remaining registered focusable actors.
		 * There is no need to unregister them here.
		 */
		g_object_unref(priv->focusManager);
		priv->focusManager=NULL;
	}

	if(priv->bindings)
	{
		g_object_unref(priv->bindings);
		priv->bindings=NULL;
	}

	if(priv->appDatabase)
	{
		g_object_unref(priv->appDatabase);
		priv->appDatabase=NULL;
	}

	if(priv->appTracker)
	{
		g_object_unref(priv->appTracker);
		priv->appTracker=NULL;
	}

	/* Shutdown session management */
	if(priv->sessionManagementClient)
	{
		priv->sessionManagementClient=NULL;
	}

	/* Shutdown xfconf */
	priv->xfconfChannel=NULL;
	xfconf_shutdown();

	/* Unset singleton */
	if(G_LIKELY(G_OBJECT(_xfdashboard_application)==inObject)) _xfdashboard_application=NULL;

	/* Call parent's class dispose method */
	G_OBJECT_CLASS(xfdashboard_application_parent_class)->dispose(inObject);
}

/* Set/get properties */
static void _xfdashboard_application_set_property(GObject *inObject,
													guint inPropID,
													const GValue *inValue,
													GParamSpec *inSpec)
{
	XfdashboardApplication	*self=XFDASHBOARD_APPLICATION(inObject);

	switch(inPropID)
	{
		case PROP_THEME_NAME:
			_xfdashboard_application_set_theme_name(self, g_value_get_string(inValue));
			break;

		default:
			G_OBJECT_WARN_INVALID_PROPERTY_ID(inObject, inPropID, inSpec);
			break;
	}
}

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

		case PROP_SUSPENDED:
			g_value_set_boolean(outValue, self->priv->isSuspended);
			break;

		case PROP_THEME_NAME:
			g_value_set_string(outValue, self->priv->themeName);
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
	klass->exit=_xfdashboard_application_action_exit;

	appClass->activate=_xfdashboard_application_activate;
	appClass->command_line=_xfdashboard_application_command_line;
#if GLIB_CHECK_VERSION(2, 40, 0)
	appClass->local_command_line=_xfdashboard_application_local_command_line;
#endif

	gobjectClass->dispose=_xfdashboard_application_dispose;
	gobjectClass->set_property=_xfdashboard_application_set_property;
	gobjectClass->get_property=_xfdashboard_application_get_property;

	/* Set up private structure */
	g_type_class_add_private(klass, sizeof(XfdashboardApplicationPrivate));

	/* Define properties */
	XfdashboardApplicationProperties[PROP_DAEMONIZED]=
		g_param_spec_boolean("is-daemonized",
								_("Is daemonized"),
								_("Flag indicating if application is daemonized"),
								FALSE,
								G_PARAM_READABLE | G_PARAM_STATIC_STRINGS);

	XfdashboardApplicationProperties[PROP_SUSPENDED]=
		g_param_spec_boolean("is-suspended",
								_("Is suspended"),
								_("Flag indicating if application is suspended currently"),
								FALSE,
								G_PARAM_READABLE | G_PARAM_STATIC_STRINGS);

	XfdashboardApplicationProperties[PROP_THEME_NAME]=
		g_param_spec_string("theme-name",
								_("Theme name"),
								_("Name of current theme"),
								DEFAULT_THEME_NAME,
								G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS);

	g_object_class_install_properties(gobjectClass, PROP_LAST, XfdashboardApplicationProperties);

	/* Define signals */
	XfdashboardApplicationSignals[SIGNAL_QUIT]=
		g_signal_new("quit",
						G_TYPE_FROM_CLASS(klass),
						G_SIGNAL_RUN_LAST,
						G_STRUCT_OFFSET(XfdashboardApplicationClass, quit),
						NULL,
						NULL,
						g_cclosure_marshal_VOID__VOID,
						G_TYPE_NONE,
						0);

	XfdashboardApplicationSignals[SIGNAL_SHUTDOWN_FINAL]=
		g_signal_new("shutdown-final",
						G_TYPE_FROM_CLASS(klass),
						G_SIGNAL_RUN_LAST,
						G_STRUCT_OFFSET(XfdashboardApplicationClass, shutdown_final),
						NULL,
						NULL,
						g_cclosure_marshal_VOID__VOID,
						G_TYPE_NONE,
						0);

	XfdashboardApplicationSignals[SIGNAL_SUSPEND]=
		g_signal_new("suspend",
						G_TYPE_FROM_CLASS(klass),
						G_SIGNAL_RUN_LAST,
						G_STRUCT_OFFSET(XfdashboardApplicationClass, suspend),
						NULL,
						NULL,
						g_cclosure_marshal_VOID__VOID,
						G_TYPE_NONE,
						0);

	XfdashboardApplicationSignals[SIGNAL_RESUME]=
		g_signal_new("resume",
						G_TYPE_FROM_CLASS(klass),
						G_SIGNAL_RUN_LAST,
						G_STRUCT_OFFSET(XfdashboardApplicationClass, resume),
						NULL,
						NULL,
						g_cclosure_marshal_VOID__VOID,
						G_TYPE_NONE,
						0);

	XfdashboardApplicationSignals[SIGNAL_THEME_CHANGED]=
		g_signal_new("theme-changed",
						G_TYPE_FROM_CLASS(klass),
						G_SIGNAL_RUN_LAST,
						G_STRUCT_OFFSET(XfdashboardApplicationClass, theme_changed),
						NULL,
						NULL,
						g_cclosure_marshal_VOID__OBJECT,
						G_TYPE_NONE,
						1,
						XFDASHBOARD_TYPE_THEME);

	XfdashboardApplicationSignals[SIGNAL_APPLICATION_LAUNCHED]=
		g_signal_new("application-launched",
						G_TYPE_FROM_CLASS(klass),
						G_SIGNAL_RUN_LAST,
						G_STRUCT_OFFSET(XfdashboardApplicationClass, application_launched),
						NULL,
						NULL,
						g_cclosure_marshal_VOID__OBJECT,
						G_TYPE_NONE,
						1,
						G_TYPE_APP_INFO);

	XfdashboardApplicationSignals[ACTION_EXIT]=
		g_signal_new("exit",
						G_TYPE_FROM_CLASS(klass),
						G_SIGNAL_RUN_LAST | G_SIGNAL_ACTION,
						G_STRUCT_OFFSET(XfdashboardApplicationClass, exit),
						g_signal_accumulator_true_handled,
						NULL,
						_xfdashboard_marshal_BOOLEAN__OBJECT_STRING_OBJECT,
						G_TYPE_BOOLEAN,
						3,
						XFDASHBOARD_TYPE_FOCUSABLE,
						G_TYPE_STRING,
						CLUTTER_TYPE_EVENT);

	/* Register GValue transformation function not provided by any other library */
	xfdashboard_register_gvalue_transformation_funcs();
}

/* Object initialization
 * Create private structure and set up default values
 */
static void xfdashboard_application_init(XfdashboardApplication *self)
{
	XfdashboardApplicationPrivate	*priv;
	GSimpleAction					*action;

	priv=self->priv=XFDASHBOARD_APPLICATION_GET_PRIVATE(self);

	/* Set default values */
	priv->isDaemon=FALSE;
	priv->isSuspended=FALSE;
	priv->themeName=NULL;
	priv->inited=FALSE;
	priv->xfconfChannel=NULL;
	priv->viewManager=NULL;
	priv->searchManager=NULL;
	priv->focusManager=NULL;
	priv->theme=NULL;
	priv->xfconfThemeChangedSignalID=0L;
	priv->isQuitting=FALSE;
	priv->sessionManagementClient=NULL;
	priv->pluginManager=NULL;

	/* Add callable DBUS actions for this application */
	action=g_simple_action_new("Quit", NULL);
	g_signal_connect(action, "activate", G_CALLBACK(xfdashboard_application_quit_forced), NULL);
	g_action_map_add_action(G_ACTION_MAP(self), G_ACTION(action));
	g_object_unref(action);
}

/* IMPLEMENTATION: Public API */

/* Get single instance of application */
XfdashboardApplication* xfdashboard_application_get_default(void)
{
	if(G_UNLIKELY(!_xfdashboard_application))
	{
		gchar			*appID;
		const gchar		*forceNewInstance=NULL;

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

		_xfdashboard_application=g_object_new(XFDASHBOARD_TYPE_APPLICATION,
												"application-id", appID,
												"flags", G_APPLICATION_HANDLES_COMMAND_LINE,
												NULL);

		/* Release allocated resources */
		if(appID) g_free(appID);
	}

	return(_xfdashboard_application);
}

/* Get flag if application is running in daemonized mode */
gboolean xfdashboard_application_is_daemonized(XfdashboardApplication *self)
{
	g_return_val_if_fail(XFDASHBOARD_IS_APPLICATION(self), FALSE);

	return(self->priv->isDaemon);
}

/* Get flag if application is suspended or resumed */
gboolean xfdashboard_application_is_suspended(XfdashboardApplication *self)
{
	g_return_val_if_fail(XFDASHBOARD_IS_APPLICATION(self), FALSE);

	return(self->priv->isSuspended);
}

/* Get flag if application is going to quit */
gboolean xfdashboard_application_is_quitting(XfdashboardApplication *self)
{
	g_return_val_if_fail(XFDASHBOARD_IS_APPLICATION(self), FALSE);

	return(self->priv->isQuitting);
}

/* Resume application */
void xfdashboard_application_resume(XfdashboardApplication *self)
{
	g_return_if_fail(self==NULL || XFDASHBOARD_IS_APPLICATION(self));

	/* Get default single instance if NULL is requested */
	if(!self) self=_xfdashboard_application;

	/* Resume application */
	if(G_LIKELY(self))
	{
		_xfdashboard_application_activate(G_APPLICATION(self));
	}
}

/* Quit application */
void xfdashboard_application_suspend_or_quit(XfdashboardApplication *self)
{
	g_return_if_fail(self==NULL || XFDASHBOARD_IS_APPLICATION(self));

	/* Get default single instance if NULL is requested */
	if(!self) self=_xfdashboard_application;

	/* Quit application */
	if(G_LIKELY(self))
	{
		_xfdashboard_application_quit(self, FALSE);
	}
}

void xfdashboard_application_quit_forced(XfdashboardApplication *self)
{
	g_return_if_fail(self==NULL || XFDASHBOARD_IS_APPLICATION(self));

	/* Get default single instance if NULL is requested */
	if(!self) self=_xfdashboard_application;

	/* Force application to quit */
	if(G_LIKELY(self))
	{
		/* Quit also any other running instance */
		if(g_application_get_is_remote(G_APPLICATION(self))==TRUE)
		{
			g_action_group_activate_action(G_ACTION_GROUP(self), "Quit", NULL);
		}

		/* Quit this instance */
		_xfdashboard_application_quit(self, TRUE);
	}
		else clutter_main_quit();
}

/* Get xfconf channel for this application */
XfconfChannel* xfdashboard_application_get_xfconf_channel(XfdashboardApplication *self)
{
	XfconfChannel			*channel=NULL;

	g_return_if_fail(self==NULL || XFDASHBOARD_IS_APPLICATION(self));

	/* Get default single instance if NULL is requested */
	if(!self) self=_xfdashboard_application;

	/* Get xfconf channel */
	if(G_LIKELY(self)) channel=self->priv->xfconfChannel;

	return(channel);
}

/* Get current theme used */
XfdashboardTheme* xfdashboard_application_get_theme(XfdashboardApplication *self)
{
	XfdashboardTheme		*theme=NULL;

	/* Get default single instance if NULL is requested */
	if(!self) self=_xfdashboard_application;

	/* Get theme */
	if(G_LIKELY(self)) theme=self->priv->theme;

	return(theme);
}
