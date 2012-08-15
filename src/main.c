/*
 * main.h: Common functions, shared data
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

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "main.h"
#include "winlist.h"

#include <stdlib.h>


/*----------------------------------------------------------------------------*/
/* CONSTANTS */

/* Error codes for exit() */
#define	kFatalErrorInitialization		1


/*----------------------------------------------------------------------------*/
/* GLOBAL VARIABLES */

WnckScreen		*screen;
WnckWorkspace	*workspace;
GList			*windows=NULL;

ClutterActor	*stage=NULL;
ClutterActor	*actor=NULL;


/*----------------------------------------------------------------------------*/
/* FUNCTIONS */

/* Get window of stage */
WnckWindow* xfdashboard_getStageWindow()
{
	WnckWindow		*stageWindow=NULL;

	if(!stageWindow)
	{
		Window		xWindow;

		xWindow=clutter_x11_get_stage_window(CLUTTER_STAGE(stage));
		stageWindow=wnck_window_get(xWindow);
	}

	return(stageWindow);
}

/* Stage has gone to fullscreen */
static void xfdashboard_onStageFullscreen(ClutterStage *stage)
{
	winlist_layoutActors();
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

/* Workspace was changed */
static void xfdashboard_onWorkspaceChanged(WnckScreen *inScreen, WnckWorkspace *inPrevWorkspace, gpointer inUserData)
{
	/* Get new active workspace */
	workspace=wnck_screen_get_active_workspace(inScreen);
	
	/* Move clutter stage to new active workspace and make active */
	wnck_window_move_to_workspace(xfdashboard_getStageWindow(), workspace);
	wnck_window_activate(xfdashboard_getStageWindow(), CLUTTER_CURRENT_TIME);
	
	/* Re-new window list */
	winlist_createActors(inScreen, workspace);
}

/* Main entry point */
int main(int argc, char **argv)
{
	ClutterColor	stageColor={ 0, 0, 0, 0xd0 };

	/* Tell clutter to try to initialize an RGBA visual */
	clutter_x11_set_use_argb_visual(TRUE);
	
	/* Initialize GTK+ and Clutter */
	gtk_init(&argc, &argv);
	if(!clutter_init(&argc, &argv))
	{
		g_error("Initializing clutter failed!");
		exit(kFatalErrorInitialization);
	}

	/* TODO: Check for running instance (libunique?) */
	
	/* Get current screen */
	screen=wnck_screen_get_default();
	
	/* Get current workspace but we need to force an update before */
	wnck_screen_force_update(screen);
	workspace=wnck_screen_get_active_workspace(screen);

	/* Create clutter stage */
	stage=clutter_stage_new();
	clutter_stage_set_color(CLUTTER_STAGE(stage), &stageColor);
	clutter_stage_set_use_alpha(CLUTTER_STAGE(stage), TRUE);
	
	/* TODO: Create background by copying background of Xfce */
	
	/* Create initial set of actors for open windows on active workspace */
	winlist_createActors(screen, workspace);
	
	/* Set up event handlers */
	clutter_stage_set_key_focus(CLUTTER_STAGE(stage), NULL);
	
	g_signal_connect(screen, "active-workspace-changed", G_CALLBACK(xfdashboard_onWorkspaceChanged), NULL);
	g_signal_connect(stage, "destroy", G_CALLBACK(clutter_main_quit), NULL);
	g_signal_connect(stage, "fullscreen", G_CALLBACK(xfdashboard_onStageFullscreen), NULL);
	g_signal_connect(stage, "unfullscreen", G_CALLBACK(clutter_main_quit), NULL);
	g_signal_connect(stage, "key-release-event", G_CALLBACK(xfdashboard_onKeyRelease), NULL);
	
	/* Show stage and go ;) */
	clutter_actor_show_all(stage);
	clutter_stage_set_fullscreen(CLUTTER_STAGE(stage), TRUE);

	clutter_main();
    
	return(0);
}
