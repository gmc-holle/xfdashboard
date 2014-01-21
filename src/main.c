/*
 * main: Common functions, shared data and main entry point of application
 * 
 * Copyright 2012 Stephan Haller <nomad@froevel.de>
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

#include "application.h"
#include "types.h"

/* Main entry point */
int main(int argc, char **argv)
{
	XfdashboardApplication		*app=NULL;
	GError						*error=NULL;
	gint						status;

#if !defined(GLIB_CHECK_VERSION) || !GLIB_CHECK_VERSION(2, 36, 0)
	/* Initialize GObject type system */
	g_type_init();
#endif

	/* Check for running instance (keep only one instance) */
	app=xfdashboard_application_get_default();

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
				g_application_activate(G_APPLICATION(app));
				break;

			case XFDASHBOARD_APPLICATION_ERROR_QUIT:
				/* Do nothing at remote instance */
				break;

			default:
				g_error(_("Initializing application failed with status code %d"), status);
				break;
		}

		/* Exit this instance of application */
		g_object_unref(app);
		return(status);
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

	/* Start main loop */
	clutter_main();

	/* Clean up, release allocated resources */
	g_object_unref(app);

	return(XFDASHBOARD_APPLICATION_ERROR_NONE);
}
