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

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "common.h"
#include "live-window.h"
#include "scaling-flow-layout.h"
#include "viewpad.h"
#include "view-selector.h"
#include "applications-view.h"
#include "windows-view.h"
#include "quicklaunch.h"
#include "scrollbar.h"

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
													"gajim.desktop"
												};
/* TODO: Replace with xfconf */

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
	gulong					stageDestroySignalID=0L;
	ClutterActor			*group;
	ClutterActor			*quicklaunch;
	ClutterActor			*viewpad, *viewSelector, *view;
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

	/* Create group holding all actors for stage */
	group=clutter_group_new();
	clutter_actor_set_position(group, 0.0f, spacingToStage);
	clutter_actor_add_constraint(group, clutter_bind_constraint_new(stage, CLUTTER_BIND_WIDTH, 0.0f));
	clutter_actor_add_constraint(group, clutter_bind_constraint_new(stage, CLUTTER_BIND_HEIGHT, -(2*spacingToStage)));
	clutter_container_add_actor(CLUTTER_CONTAINER(stage), group);

	/* TODO: Create background by copying background of Xfce */

	/* Create quicklaunch box and add to box */
	quicklaunch=xfdashboard_quicklaunch_new();
	clutter_actor_add_constraint(quicklaunch, clutter_bind_constraint_new(group, CLUTTER_BIND_HEIGHT, 0.0f));
	clutter_container_add_actor(CLUTTER_CONTAINER(group), quicklaunch);

	/* TODO: Remove the following actor(s) for application icons
	 *       in quicklaunch box as soon as xfconf is implemented
	 */
	for(gint i=0; i<(sizeof(quicklaunch_apps)/sizeof(quicklaunch_apps[0])); i++)
	{
		xfdashboard_quicklaunch_add_icon_by_desktop_file(XFDASHBOARD_QUICKLAUNCH(quicklaunch), quicklaunch_apps[i]);
	}

	/* Create viewpad and view selector */
	viewpad=xfdashboard_viewpad_new();
	viewSelector=xfdashboard_view_selector_new(XFDASHBOARD_VIEWPAD(viewpad));

	clutter_actor_add_constraint(viewSelector, clutter_snap_constraint_new(quicklaunch, CLUTTER_SNAP_EDGE_LEFT, CLUTTER_SNAP_EDGE_RIGHT, spacingToStage));
	clutter_actor_add_constraint(viewSelector, clutter_snap_constraint_new(group, CLUTTER_SNAP_EDGE_RIGHT, CLUTTER_SNAP_EDGE_RIGHT, -spacingToStage));
	clutter_actor_add_constraint(viewSelector, clutter_bind_constraint_new(quicklaunch, CLUTTER_BIND_Y, 0.0f));
	clutter_container_add_actor(CLUTTER_CONTAINER(group), viewSelector);

	clutter_actor_add_constraint(viewpad, clutter_snap_constraint_new(quicklaunch, CLUTTER_SNAP_EDGE_LEFT, CLUTTER_SNAP_EDGE_RIGHT, spacingToStage));
	clutter_actor_add_constraint(viewpad, clutter_snap_constraint_new(group, CLUTTER_SNAP_EDGE_RIGHT, CLUTTER_SNAP_EDGE_RIGHT, -spacingToStage));
	clutter_actor_add_constraint(viewpad, clutter_snap_constraint_new(viewSelector, CLUTTER_SNAP_EDGE_TOP, CLUTTER_SNAP_EDGE_BOTTOM, spacingToStage));
	clutter_actor_add_constraint(viewpad, clutter_snap_constraint_new(group, CLUTTER_SNAP_EDGE_BOTTOM, CLUTTER_SNAP_EDGE_BOTTOM, -spacingToStage));
	clutter_container_add_actor(CLUTTER_CONTAINER(group), viewpad);


	/* Create views and add them to viewpad */
	view=xfdashboard_windows_view_new();
	xfdashboard_viewpad_add_view(XFDASHBOARD_VIEWPAD(viewpad), XFDASHBOARD_VIEW(view));
	xfdashboard_viewpad_set_active_view(XFDASHBOARD_VIEWPAD(viewpad), XFDASHBOARD_VIEW(view));

	view=xfdashboard_applications_view_new();
	xfdashboard_viewpad_add_view(XFDASHBOARD_VIEWPAD(viewpad), XFDASHBOARD_VIEW(view));
	xfdashboard_applications_view_set_active_menu(XFDASHBOARD_APPLICATIONS_VIEW(view), xfdashboard_get_application_menu());
	xfdashboard_applications_view_set_list_mode(XFDASHBOARD_APPLICATIONS_VIEW(view), XFDASHBOARD_APPLICATIONS_VIEW_ICON);

	/* Set up event handlers */
	clutter_stage_set_key_focus(CLUTTER_STAGE(stage), NULL);

	stageDestroySignalID=g_signal_connect(stage, "destroy", G_CALLBACK(clutter_main_quit), NULL);
	g_signal_connect(stage, "unfullscreen", G_CALLBACK(clutter_main_quit), NULL);
	g_signal_connect(stage, "key-release-event", G_CALLBACK(xfdashboard_onKeyRelease), NULL);

	/* Show stage and go ;) */
	clutter_actor_show(stage);
	clutter_stage_set_fullscreen(CLUTTER_STAGE(stage), TRUE);

	clutter_main();

	if(stageDestroySignalID &&
		g_signal_handler_is_connected(stage, stageDestroySignalID))
	{
		g_signal_handler_disconnect(stage, stageDestroySignalID);
		stageDestroySignalID=0L;
	}
	clutter_actor_destroy(stage);

	return(0);
}
