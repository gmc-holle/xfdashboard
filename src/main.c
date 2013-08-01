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

#define WNCK_I_KNOW_THIS_IS_UNSTABLE
#include <libwnck/libwnck.h>

#include <glib/gi18n-lib.h>
#include <clutter/clutter.h>
#include <clutter/x11/clutter-x11.h>
#include <gtk/gtk.h>

#include "application.h"
#include "types.h"

/* Create and setup stage */
//~ void _xfdashboard_create_stage()
//~ {
	//~ ClutterColor			stageColor={ 0, 0, 0, 0xd0 };
	//~ GdkScreen				*screen;
	//~ gint					primary;
	//~ GdkRectangle			primarySize;
	//~ ClutterActor			*view;
//~ 
	//~ /* Get size of primary monitor to set size of stage */
	//~ screen=gdk_screen_get_default();
	//~ primary=gdk_screen_get_primary_monitor(screen);
	//~ gdk_screen_get_monitor_geometry(screen, primary, &primarySize);
//~ 
	//~ /* Create clutter stage */
	//~ stage=clutter_stage_new();
	//~ clutter_stage_set_color(CLUTTER_STAGE(stage), &stageColor);
	//~ clutter_stage_set_use_alpha(CLUTTER_STAGE(stage), TRUE);
	//~ clutter_actor_set_size(stage, primarySize.width, primarySize.height);
//~ 
	//~ /* Create group holding all actors for stage */
	//~ group=clutter_group_new();
	//~ clutter_actor_set_position(group, 0.0f, spacingToStage);
	//~ clutter_actor_add_constraint(group, clutter_bind_constraint_new(stage, CLUTTER_BIND_WIDTH, 0.0f));
	//~ clutter_actor_add_constraint(group, clutter_bind_constraint_new(stage, CLUTTER_BIND_HEIGHT, -(2*spacingToStage)));
	//~ clutter_container_add_actor(CLUTTER_CONTAINER(stage), group);
//~ 
	//~ /* TODO: Create background by copying background of Xfce */
//~ 
	//~ /* Create quicklaunch box and add to box */
	//~ quicklaunch=xfdashboard_quicklaunch_new();
	//~ xfdashboard_quicklaunch_set_mark_icon(XFDASHBOARD_QUICKLAUNCH(quicklaunch), GTK_STOCK_HOME);
	//~ xfdashboard_quicklaunch_set_marked_text(XFDASHBOARD_QUICKLAUNCH(quicklaunch), "Switch to windows");
	//~ xfdashboard_quicklaunch_set_unmarked_text(XFDASHBOARD_QUICKLAUNCH(quicklaunch), "Switch to applications");
	//~ clutter_actor_add_constraint(quicklaunch, clutter_bind_constraint_new(group, CLUTTER_BIND_HEIGHT, 0.0f));
	//~ clutter_container_add_actor(CLUTTER_CONTAINER(group), quicklaunch);
	//~ xfconf_g_property_bind(xfdashboardChannel, "/favourites", XFDASHBOARD_TYPE_VALUE_ARRAY, quicklaunch, "desktop-files");
//~ 
	//~ /* Create search box */
	//~ searchbox=xfdashboard_searchbox_new();
	//~ xfdashboard_searchbox_set_primary_icon(XFDASHBOARD_SEARCHBOX(searchbox), GTK_STOCK_FIND);
	//~ xfdashboard_searchbox_set_secondary_icon(XFDASHBOARD_SEARCHBOX(searchbox), GTK_STOCK_CLEAR);
	//~ xfdashboard_searchbox_set_hint_text(XFDASHBOARD_SEARCHBOX(searchbox), "Just type to search ...");
	//~ clutter_actor_add_constraint(searchbox, clutter_snap_constraint_new(quicklaunch, CLUTTER_SNAP_EDGE_LEFT, CLUTTER_SNAP_EDGE_RIGHT, spacingToStage));
	//~ clutter_actor_add_constraint(searchbox, clutter_snap_constraint_new(group, CLUTTER_SNAP_EDGE_RIGHT, CLUTTER_SNAP_EDGE_RIGHT, -spacingToStage));
	//~ clutter_actor_add_constraint(searchbox, clutter_bind_constraint_new(quicklaunch, CLUTTER_BIND_Y, 0.0f));
	//~ clutter_container_add_actor(CLUTTER_CONTAINER(group), searchbox);
	//~ g_signal_connect(searchbox, "secondary-icon-clicked", G_CALLBACK(_xfdashboard_on_clear_searchbox), NULL);
//~ 
	//~ /* Create viewpad */
	//~ viewpad=xfdashboard_viewpad_new();
	//~ clutter_actor_add_constraint(viewpad, clutter_snap_constraint_new(quicklaunch, CLUTTER_SNAP_EDGE_LEFT, CLUTTER_SNAP_EDGE_RIGHT, spacingToStage));
	//~ clutter_actor_add_constraint(viewpad, clutter_snap_constraint_new(group, CLUTTER_SNAP_EDGE_RIGHT, CLUTTER_SNAP_EDGE_RIGHT, -spacingToStage));
	//~ clutter_actor_add_constraint(viewpad, clutter_snap_constraint_new(searchbox, CLUTTER_SNAP_EDGE_TOP, CLUTTER_SNAP_EDGE_BOTTOM, spacingToStage));
	//~ clutter_actor_add_constraint(viewpad, clutter_snap_constraint_new(group, CLUTTER_SNAP_EDGE_BOTTOM, CLUTTER_SNAP_EDGE_BOTTOM, -spacingToStage));
	//~ clutter_container_add_actor(CLUTTER_CONTAINER(group), viewpad);
//~ 
	//~ /* Create views and add them to viewpad */
	//~ view=xfdashboard_windows_view_new();
	//~ xfdashboard_viewpad_add_view(XFDASHBOARD_VIEWPAD(viewpad), XFDASHBOARD_VIEW(view));
	//~ xfdashboard_viewpad_set_active_view(XFDASHBOARD_VIEWPAD(viewpad), XFDASHBOARD_VIEW(view));
	//~ g_signal_connect(view, "activated", G_CALLBACK(_xfdashboard_on_windows_view_activated), quicklaunch);
	//~ g_signal_connect_swapped(quicklaunch, "unmarked", G_CALLBACK(_xfdashboard_on_switch_to_view_clicked), view);
//~ 
	//~ view=xfdashboard_applications_view_new();
	//~ xfdashboard_viewpad_add_view(XFDASHBOARD_VIEWPAD(viewpad), XFDASHBOARD_VIEW(view));
	//~ g_signal_connect(view, "activated", G_CALLBACK(_xfdashboard_on_application_view_activated), quicklaunch);
	//~ g_signal_connect_swapped(quicklaunch, "marked", G_CALLBACK(_xfdashboard_on_switch_to_view_clicked), view);
	//~ g_signal_connect(searchbox, "search-started", G_CALLBACK(_xfdashboard_on_search_started), view);
	//~ g_signal_connect(searchbox, "text-changed", G_CALLBACK(_xfdashboard_on_search_update), view);
//~ 
	//~ /* Set up event handlers and connect signals */
	//~ clutter_stage_set_key_focus(CLUTTER_STAGE(stage), searchbox);
//~ 
	//~ signalStageDestroyID=g_signal_connect(stage, "destroy", G_CALLBACK(clutter_main_quit), NULL);
	//~ g_signal_connect(stage, "unfullscreen", G_CALLBACK(clutter_main_quit), NULL);
	//~ g_signal_connect(stage, "key-release-event", G_CALLBACK(_xfdashboard_on_key_release), NULL);
//~ 
	//~ /* Show stage and go ;) */
	//~ clutter_actor_show(stage);
	//~ clutter_stage_set_fullscreen(CLUTTER_STAGE(stage), TRUE);
//~ }

/* Destroy and clean up stage */
//~ void _xfdashboard_destroy_stage()
//~ {
	//~ /* Remove signal handler handling stage destruction to prevent
	 //~ * event handling loop
	 //~ */
	//~ if(signalStageDestroyID &&
		//~ g_signal_handler_is_connected(stage, signalStageDestroyID))
	//~ {
		//~ g_signal_handler_disconnect(stage, signalStageDestroyID);
		//~ signalStageDestroyID=0L;
	//~ }
	//~ clutter_actor_destroy(stage);
//~ }

/* Main entry point */
int main(int argc, char **argv)
{
	XfdashboardApplication		*app=NULL;
	GError						*error=NULL;
	gint						status;

	/* Initialize GObject type system */
	g_type_init();

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
		/* Handle command-line on primary instance of application */
		status=g_application_run(G_APPLICATION(app), argc, argv);
		if(status!=XFDASHBOARD_APPLICATION_ERROR_RESTART)
		{
			/* Activate primary instance of application */
			if(status==XFDASHBOARD_APPLICATION_ERROR_NONE) g_application_activate(G_APPLICATION(app));

			/* Exit this instance of application */
			g_object_unref(app);
			return(status);
		}

		/* If we get here we should replace running instance */
		g_debug(_("Replacing existing instance!"));

		g_object_unref(app);

		app=xfdashboard_application_get_default();
		g_application_register(G_APPLICATION(app), NULL, &error);
		if(error!=NULL)
		{
			g_warning(_("Unable to register application: %s"), error->message);
			g_error_free(error);
			error=NULL;
			return(XFDASHBOARD_APPLICATION_ERROR_FAILED);
		}
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

	/* Set up libwnck library (must be done _after_ gtk_init) */
	wnck_set_client_type(WNCK_CLIENT_TYPE_PAGER);

	/* Handle command-line on primary instance */
	status=g_application_run(G_APPLICATION(app), argc, argv);
	if(status==XFDASHBOARD_APPLICATION_ERROR_RESTART) status=XFDASHBOARD_APPLICATION_ERROR_NONE;
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
