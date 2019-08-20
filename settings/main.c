/*
 * main: Common functions, shared data and main entry point of application
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

#include <libxfdashboard/utils.h>
#include <glib/gi18n-lib.h>
#include <gdk/gdkx.h>
#include <gtk/gtk.h>
#include <gtk/gtkx.h>
#include <libxfce4util/libxfce4util.h>
#include <xfconf/xfconf.h>

#include "settings.h"

/* Main entry point */
int main(int argc, char **argv)
{
	XfdashboardSettings		*settings=NULL;
	GError					*error=NULL;
	gboolean				optionsVersion=FALSE;
	Window					optionsSocketID=0;
	GOptionEntry			options[]=	{
											{ "socket-id", 's', G_OPTION_FLAG_IN_MAIN, G_OPTION_ARG_INT, &optionsSocketID, N_("Settings manager socket"), N_("SOCKET ID") },
											{ "version", 'V', G_OPTION_FLAG_IN_MAIN, G_OPTION_ARG_NONE, &optionsVersion, N_("Version information"), NULL },
											{ NULL }
										};

#ifdef ENABLE_NLS
	/* Set up localization */
	xfce_textdomain(GETTEXT_PACKAGE, PACKAGE_LOCALE_DIR, "UTF-8");
#endif

	/* Initialize GTK+ */
	if(G_UNLIKELY(!gtk_init_with_args(&argc, &argv, _("."), options, PACKAGE, &error)))
	{
		if(G_LIKELY(error))
		{
			g_print(_("%s: %s\nTry %s --help to see a full list of available command line options.\n"),
						PACKAGE,
						error ? error->message : _("Unknown error"),
						PACKAGE_NAME);
			if(error) g_error_free(error);
		}

		return(1);
	}

	/* If option for showing version is set, print version and exit */
	if(G_UNLIKELY(optionsVersion))
	{
		g_print("%s\n", PACKAGE_STRING);

		return(0);
	}

	/* Initialize Xfconf */
	if(G_UNLIKELY(!xfconf_init(&error)))
	{
		if(G_LIKELY(error))
		{
			g_error(_("Failed to initialize xfconf: %s"),
					error ? error->message : _("Unknown error"));
			if(error) g_error_free(error);
		}

		return(1);
	}

	/* Create settings instance */
	settings=xfdashboard_settings_new();
	if(G_UNLIKELY(!settings))
	{
		g_error(_("Could not create the settings dialog."));

		/* Shutdown xfconf */
		xfconf_shutdown();

		return(1);
	}

	/* Register GValue transformation functions */
	xfdashboard_register_gvalue_transformation_funcs();

	/* Create and show settings dialog as normal application window
	 * if no socket ID for xfce settings manager is given ...
	 */
	if(G_UNLIKELY(!optionsSocketID))
	{
		GtkWidget			*dialog;

		/* Create and show dialog */
		dialog=xfdashboard_settings_create_dialog(settings);
		gtk_widget_show(dialog);

		/* Connect signals */
		g_signal_connect(dialog, "delete-event", G_CALLBACK(gtk_main_quit), NULL);

		/* Prevent settings dialog to be saved in the session */
		gdk_x11_set_sm_client_id("FAKE ID");

		/* Run application only if we have a widget to show */
		if(dialog) gtk_main();

		/* Release allocated resources */
		gtk_widget_destroy(dialog);
	}
		/* ... otherwise show dialog in xfce settings manager
		 * by plugging in the dialog via given socket ID
		 */
		else
		{
			GtkWidget		*plug;

			/* Create "pluggable" dialog for xfce settings manager */
			plug=xfdashboard_settings_create_plug(settings, optionsSocketID);
			gtk_widget_show(plug);

			/* Connect signals */
			g_signal_connect(plug, "delete-event", G_CALLBACK(gtk_main_quit), NULL);

			/* Prevent settings dialog to be saved in the session */
			gdk_x11_set_sm_client_id("FAKE ID");

			/* Stop startup notification */
			gdk_notify_startup_complete();

			/* Run application only if we have a widget to show */
			if(plug) gtk_main();
		}

	/* Release allocated resources */
	g_object_unref(settings);

	/* Shutdown xfconf */
	xfconf_shutdown();

	/* Return from application */
	return(0);
}
