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

#include "livewindow.h"
#include "scalingflowlayout.h"
#include "view.h"
#include "windows-view.h"

/* The one and only stage for clutter */
ClutterActor	*stage=NULL;

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
	ClutterActor	*box;
	ClutterColor	stageColor={ 0, 0, 0, 0xd0 };

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

	/* TODO: Create viewpad and add view(s) to viewpad */

	/* Create windows view and add to stage */
	box=xfdashboard_windows_view_new();
	clutter_actor_set_size(box, clutter_actor_get_width(stage), clutter_actor_get_height(stage));
	clutter_actor_add_constraint(box, clutter_bind_constraint_new(stage, CLUTTER_BIND_SIZE, 0.0));
	clutter_container_add_actor(CLUTTER_CONTAINER(stage), box);

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
