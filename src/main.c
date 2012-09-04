/*
 * main.c: Common functions, shared data
 *         and main entry point of application
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

#include "live-window.h"
#include "scaling-flow-layout.h"
#include "view.h"
#include "windows-view.h"
#include "quicklaunch.h"

/* The one and only stage for clutter */
ClutterActor	*stage=NULL;
const gfloat	spacingToStage=8.0f;

/* TODO: Replace with xfconf */
static gchar			*quicklaunch_apps[]=	{
													"firefox.desktop",
													"evolution.desktop",
													"Terminal.desktop",
													"Thunar.desktop",
													"geany.desktop",
													"unavailable"
												};
/* TODO: Replace with xfconf */

/* Get root application menu */
GarconMenu* xfdashboard_getApplicationMenu()
{
	static GarconMenu		*menu=NULL;

	/* If it is the first time (or if it failed previously)
	 * load the menus now
	 */
	if(!menu)
	{
		/* Try to get the root menu */
		menu=garcon_menu_new_applications();

		if(G_UNLIKELY(!garcon_menu_load(menu, NULL, &error)))
		{
			gchar *uri;

			uri=g_file_get_uri(garcon_menu_get_file (menu));
			g_error("Could not load menu from %s: %s", uri, error->message);
			g_free(uri);

			g_error_free(error);

			g_object_unref(menu);
			menu=NULL;
		}
	}

	return(menu);
}

/* Get window of application */
WnckWindow* xfdashboard_getAppWindow()
{
	static WnckWindow		*stageWindow=NULL;

	if(!stageWindow)
	{
		Window		xWindow;

		xWindow=clutter_x11_get_stage_window(CLUTTER_STAGE(stage));
		stageWindow=wnck_window_get(xWindow);
	}

	return(stageWindow);
}

/* A pressed key was released */
static gboolean xfdashboard_onKeyRelease(ClutterActor *inActor, ClutterEvent *inEvent, gpointer inUserData)
{
	ClutterKeyEvent		*keyEvent=(ClutterKeyEvent*)inEvent;

	/* Quit on ESC without activating any window. Window manager should choose
	 * the most recent window by itself */
	if(keyEvent->keyval==CLUTTER_Escape)
	{
		clutter_main_quit();
		return(TRUE);
	}

	/* We did not handle this event so let next in clutter's chain handle it */
	return(FALSE);
}

/* Main entry point */
int main(int argc, char **argv)
{
	ClutterActor			*box;
	ClutterLayoutManager	*boxLayout;
	ClutterActor			*actor;
	ClutterColor			stageColor={ 0, 0, 0, 0xd0 };

	/* Tell clutter to try to initialize an RGBA visual */
	clutter_x11_set_use_argb_visual(TRUE);

	/* Initialize GTK+ and Clutter */
	gtk_init(&argc, &argv);
	if(!clutter_init(&argc, &argv))
	{
		g_error("Initializing clutter failed!");
		return(1);
	}

	/* TODO: Check for running instance (libunique?) */

	/* Create clutter stage */
	stage=clutter_stage_new();
	clutter_stage_set_color(CLUTTER_STAGE(stage), &stageColor);
	clutter_stage_set_use_alpha(CLUTTER_STAGE(stage), TRUE);

	/* TODO: Create background by copying background of Xfce */

	/* Create box holding all main elements of stage */
	boxLayout=clutter_box_layout_new();
	clutter_box_layout_set_spacing(CLUTTER_BOX_LAYOUT(boxLayout), spacingToStage);

	box=clutter_box_new(boxLayout);
	clutter_actor_add_constraint(box, clutter_bind_constraint_new(stage, CLUTTER_BIND_Y, spacingToStage));
	clutter_actor_add_constraint(box, clutter_bind_constraint_new(stage, CLUTTER_BIND_SIZE, -(2*spacingToStage)));
	clutter_container_add_actor(CLUTTER_CONTAINER(stage), box);

	/* Create quicklaunch box and add to box */
	actor=xfdashboard_quicklaunch_new();
	clutter_actor_add_constraint(actor, clutter_bind_constraint_new(box, CLUTTER_BIND_HEIGHT, 0.0));
	clutter_box_layout_pack(CLUTTER_BOX_LAYOUT(boxLayout),
								actor,
								TRUE,
								FALSE,
								TRUE,
								CLUTTER_BOX_ALIGNMENT_START,
								CLUTTER_BOX_ALIGNMENT_CENTER);

	/* TODO: Remove the following actor(s) for application icons
	 *       in quicklaunch box as soon as xfconf is implemented
	 */
	for(gint i=0; i<(sizeof(quicklaunch_apps)/sizeof(quicklaunch_apps[0])); i++)
	{
		xfdashboard_quicklaunch_add_icon_by_desktop_file(XFDASHBOARD_QUICKLAUNCH(actor), quicklaunch_apps[i]);
	}

	/* TODO: Create viewpad and add view(s) to viewpad */

	/* Create windows view and add to box */
	actor=xfdashboard_windows_view_new();
	clutter_actor_add_constraint(actor, clutter_bind_constraint_new(box, CLUTTER_BIND_HEIGHT, 0.0));
	clutter_box_layout_pack(CLUTTER_BOX_LAYOUT(boxLayout),
								actor,
								TRUE,
								TRUE,
								TRUE,
								CLUTTER_BOX_ALIGNMENT_CENTER,
								CLUTTER_BOX_ALIGNMENT_CENTER);

	/* Set up event handlers */
	clutter_stage_set_key_focus(CLUTTER_STAGE(stage), NULL);

	g_signal_connect(stage, "destroy", G_CALLBACK(clutter_main_quit), NULL);
	g_signal_connect(stage, "unfullscreen", G_CALLBACK(clutter_main_quit), NULL);
	g_signal_connect(stage, "key-release-event", G_CALLBACK(xfdashboard_onKeyRelease), NULL);

	/* Show stage and go ;) */
	clutter_actor_show(stage);
	clutter_stage_set_fullscreen(CLUTTER_STAGE(stage), TRUE);

	clutter_main();

	return 0;
}
