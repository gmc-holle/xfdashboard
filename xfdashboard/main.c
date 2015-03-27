/*
 * main: Common functions, shared data and main entry point of application
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
#include <clutter/clutter.h>
#include <clutter/x11/clutter-x11.h>
#include <gtk/gtk.h>
#include <libxfce4util/libxfce4util.h>

#include "application.h"
#include "types.h"

typedef struct _RestartData		RestartData;
struct _RestartData
{
	GMainLoop				*mainLoop;
	XfdashboardApplication	*application;
	gboolean				appHasQuitted;
};

#define DEFAULT_RESTART_WAIT_TIMEOUT	5000	/* Timeout in milliseconds */


/* Timeout to wait for application to disappear has been reached */
static gboolean _on_quit_timeout(gpointer inUserData)
{
	RestartData			*restartData;

	g_return_val_if_fail(inUserData, G_SOURCE_REMOVE);

	restartData=(RestartData*)inUserData;

	/* Enforce flag that application has been disappeared to be unset */
	restartData->appHasQuitted=FALSE;

	 /* Return from main loop */
	g_main_loop_quit(restartData->mainLoop);
	g_debug("Timeout for waiting the application has been reached!");

	/* Remove this source from main loop and prevent getting it called again */
	return(G_SOURCE_REMOVE);
}

/* Idle callback for quitting application has been called */
static gboolean _on_quit_idle(gpointer inUserData)
{
	/* Quit application */
	xfdashboard_application_quit_forced();

	/* Remove this source from main loop and prevent getting it called again */
	return(G_SOURCE_REMOVE);
}

/* Requested name appeared at DBUS */
static void _on_dbus_watcher_name_appeared(GDBusConnection *inConnection,
											const gchar *inName,
											const gchar *inOwner,
											gpointer inUserData)
{
	RestartData			*restartData;
	GSource				*quitIdleSource;

	g_return_if_fail(inUserData);

	restartData=(RestartData*)inUserData;

	/* Add an idle source to main loop to quit running application _after_
	 * the main loop is running and the DBUS watcher was set up.
	 */
	quitIdleSource=g_idle_source_new();
	g_source_set_callback(quitIdleSource, _on_quit_idle, NULL, NULL);
	g_source_attach(quitIdleSource, g_main_loop_get_context(restartData->mainLoop));
	g_source_unref(quitIdleSource);

	g_debug("Name %s appeared at DBUS and is owned by %s", inName, inOwner);
}

/* Requested name vanished from DBUS */
static void _on_dbus_watcher_name_vanished(GDBusConnection *inConnection,
											const gchar *inName,
											gpointer inUserData)
{
	RestartData			*restartData;

	g_return_if_fail(inUserData);

	restartData=(RestartData*)inUserData;

	/* Set flag that application disappeared */
	restartData->appHasQuitted=TRUE;

	/* Now the application is gone and we can safely restart application.
	 * So return from main loop now.
	 */
	g_main_loop_quit(restartData->mainLoop);

	g_debug("Name %s does not exist at DBUS or vanished", inName);
}

/* Quit running application and restart application */
static gboolean _restart(XfdashboardApplication *inApplication)
{
	GMainLoop			*mainLoop;
	guint				dbusWatcherID;
	GSource				*timeoutSource;
	RestartData			restartData;

	g_return_val_if_fail(XFDASHBOARD_IS_APPLICATION(inApplication), FALSE);

	/* Create main loop for watching DBUS for application to disappear */
	mainLoop=g_main_loop_new(NULL, FALSE);

	/* Set up user data for callbacks */
	restartData.mainLoop=mainLoop;
	restartData.application=inApplication;
	restartData.appHasQuitted=FALSE;

	/* Set up DBUS watcher to get noticed when application disappears
	 * which means it is safe to start a new instance. But if it takes
	 * too long assume that either application did not quit and is still
	 * running or we did get notified although we set up DBUS watcher
	 * before quitting application.
	 */
	dbusWatcherID=g_bus_watch_name(G_BUS_TYPE_SESSION,
									g_application_get_application_id(G_APPLICATION(inApplication)),
									G_BUS_NAME_WATCHER_FLAGS_NONE,
									_on_dbus_watcher_name_appeared,
									_on_dbus_watcher_name_vanished,
									&restartData,
									NULL);

	/* Add an idle source to main loop to quit running application _after_
	 * the main loop is running and the DBUS watcher was set up.
	 */
	timeoutSource=g_timeout_source_new(DEFAULT_RESTART_WAIT_TIMEOUT);
	g_source_set_callback(timeoutSource, _on_quit_timeout, &restartData, NULL);
	g_source_attach(timeoutSource, g_main_loop_get_context(mainLoop));
	g_source_unref(timeoutSource);

	/* Run main loop */
	g_debug("Starting main loop for waiting the application to quit");
	g_main_loop_run(mainLoop);
	g_debug("Returned from main loop for waiting the application to quit");

	/* Show warning if timeout had been reached */
	if(!restartData.appHasQuitted)
	{
		g_warning(_("Cannot restart application: Failed to quit running instance"));
	}

	/* Destroy DBUS watcher */
	g_bus_unwatch_name(dbusWatcherID);

	/* Return TRUE if application was quitted successfully
	 * otherwise FALSE.
	 */
	return(restartData.appHasQuitted);
}

/* Main entry point */
int main(int argc, char **argv)
{
	XfdashboardApplication		*app=NULL;
	GError						*error=NULL;
	gint						status;

#ifdef ENABLE_NLS
	/* Set up localization */
	xfce_textdomain(GETTEXT_PACKAGE, PACKAGE_LOCALE_DIR, "UTF-8");
#endif

#if !GLIB_CHECK_VERSION(2, 36, 0)
	/* Initialize GObject type system */
	g_type_init();
#endif

#if CLUTTER_CHECK_VERSION(1, 16, 0)
	/* Enforce X11 backend in Clutter. This function must be called before any
	 * other Clutter API function.
	 */
	clutter_set_windowing_backend("x11");
#endif

	/* Check for running instance (keep only one instance) */
	app=xfdashboard_application_get_default();
	if(!app)
	{
		g_warning(_("Failed to create application instance"));
		return(XFDASHBOARD_APPLICATION_ERROR_FAILED);
	}

	g_application_register(G_APPLICATION(app), NULL, &error);
	if(error!=NULL)
	{
		g_warning(_("Unable to register application: %s"), error->message);
		g_error_free(error);
		error=NULL;
		return(XFDASHBOARD_APPLICATION_ERROR_FAILED);
	}

	if(g_application_get_is_remote(G_APPLICATION(app))==TRUE)
	{
		/* Handle command-line on primary instance of application
		 * and activate primary instance if handling command-line
		 * was successful
		 */
		status=g_application_run(G_APPLICATION(app), argc, argv);
		switch(status)
		{
			case XFDASHBOARD_APPLICATION_ERROR_NONE:
			case XFDASHBOARD_APPLICATION_ERROR_QUIT:
				/* Do nothing at remote instance */
				break;

			case XFDASHBOARD_APPLICATION_ERROR_RESTART:
				/* Try quitting running instance and wait until
				 * application is stopped. If this fails reset
				 * application status from restart to failed to
				 * prevent starting another instance.
				 */
				if(!_restart(app)) status=XFDASHBOARD_APPLICATION_ERROR_FAILED;
				break;

			default:
				g_error(_("Initializing application failed with status code %d"), status);
				break;
		}

		/* Destroy this instance of application and exit if no restart
		 * was requested.
		 */
		if(status!=XFDASHBOARD_APPLICATION_ERROR_RESTART)
		{
			g_object_unref(app);
			return(status);
		}

		/* If we get here a restart was requested so destroy current
		 * instance of application and create a new one.
		 */
		g_object_unref(app);

		app=xfdashboard_application_get_default();
	}

	/* Tell clutter to try to initialize an RGBA visual */
	clutter_x11_set_use_argb_visual(TRUE);

	/* Initialize GTK+ and Clutter */
	gtk_init(&argc, &argv);
	if(!clutter_init(&argc, &argv))
	{
		g_error(_("Initializing clutter failed!"));
		return(1);
	}

	/* Handle command-line on primary instance */
	status=g_application_run(G_APPLICATION(app), argc, argv);
	if(status!=XFDASHBOARD_APPLICATION_ERROR_NONE)
	{
		g_object_unref(app);
		return(status);
	}

	/* Notify that application has started and main loop will be entered */
	gdk_notify_startup_complete();

	/* Start main loop */
	clutter_main();

	/* Clean up, release allocated resources */
	g_object_unref(app);

	return(XFDASHBOARD_APPLICATION_ERROR_NONE);
}
