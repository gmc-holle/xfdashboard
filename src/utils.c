/*
 * utils: Common functions, helpers and definitions
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

#include "utils.h"

#include <glib/gi18n-lib.h>
#include <clutter/clutter.h>
#include <gtk/gtk.h>
#include <dbus/dbus-glib.h>

#include "image.h"
#include "stage.h"

/* Gobject type for pointer arrays (GPtrArray) */
GType xfdashboard_pointer_array_get_type(void)
{
	static volatile gsize	type__volatile=0;
	GType					type;

	if(g_once_init_enter(&type__volatile))
	{
		type=dbus_g_type_get_collection("GPtrArray", G_TYPE_VALUE);
		g_once_init_leave(&type__volatile, type);
	}

	return(type__volatile);
}

/* Show a notification */
void xfdashboard_notify(ClutterActor *inSender, const gchar *inIconName, const gchar *inFormatText, ...)
{
	XfdashboardStage				*stage;
	ClutterStageManager				*stageManager;
	const GSList					*stages;
	va_list							args;
	gchar							*text;

	g_return_if_fail(inSender==NULL || CLUTTER_IS_ACTOR(inSender));

	stage=NULL;

	/* Get stage of sending actor if available */
	if(inSender) stage=XFDASHBOARD_STAGE(clutter_actor_get_stage(inSender));

	/* No sending actor specified or no stage found so get default stage */
	if(!stage)
	{
		stageManager=clutter_stage_manager_get_default();
		stage=XFDASHBOARD_STAGE(clutter_stage_manager_get_default_stage(stageManager));

		/* If we did not get default stage use first stage from manager */
		if(!stage)
		{
			stages=clutter_stage_manager_peek_stages(stageManager);
			stage=XFDASHBOARD_STAGE(stages->data);
		}
	}

	/* Build text to display */
	va_start(args, inFormatText);
	text=g_strdup_vprintf(inFormatText, args);
	va_end(args);

	/* Show notification on stage */
	xfdashboard_stage_show_notification(stage, inIconName, text);

	/* Release allocated resources */
	g_free(text);
}

/* Create a application context for launching application by GIO.
 * Object returned must be freed with g_object_unref().
 */
GAppLaunchContext* xfdashboard_create_app_context(XfdashboardWindowTrackerWorkspace *inWorkspace)
{
	GdkAppLaunchContext			*context;
	const ClutterEvent			*event;
	XfdashboardWindowTracker	*tracker;

	g_return_val_if_fail(inWorkspace==NULL || XFDASHBOARD_IS_WINDOW_TRACKER_WORKSPACE(inWorkspace), NULL);

	/* Get last event for timestamp */
	event=clutter_get_current_event();

	/* Get active workspace if not specified */
	if(!inWorkspace)
	{
		tracker=xfdashboard_window_tracker_get_default();
		inWorkspace=xfdashboard_window_tracker_get_active_workspace(tracker);
		g_object_unref(tracker);
	}

	/* Create and set up application context to use either the workspace specified
	 * or otherwise current active workspace. We will even set the current active
	 * explicitly to launch the application on current workspace even if user changes
	 * workspace in the time between launching application and showing first window.
	 */
	context=gdk_app_launch_context_new();
	if(event) gdk_app_launch_context_set_timestamp(context, clutter_event_get_time(event));
	gdk_app_launch_context_set_desktop(context, xfdashboard_window_tracker_workspace_get_number(inWorkspace));

	/* Return application context */
	return(G_APP_LAUNCH_CONTEXT(context));
}
