/*
 * window-tracker-window: A window tracked by window tracker and also
 *                        a wrapper class around WnckWindow.
 *                        By wrapping libwnck objects we can use a virtual
 *                        stable API while the API in libwnck changes
 *                        within versions. We only need to use #ifdefs in
 *                        window tracker object and nowhere else in the code.
 * 
 * Copyright 2012-2013 Stephan Haller <nomad@froevel.de>
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

#include "window-tracker-window.h"

#define WNCK_I_KNOW_THIS_IS_UNSTABLE
#include <libwnck/libwnck.h>

#include <glib/gi18n-lib.h>
#include <clutter/x11/clutter-x11.h>

#include "marshal.h"
#include "utils.h"

/* Usually we would define a class in GObject system here but
 * this class is a wrapper around WnckWindow to create a virtual stable
 * libwnck API regardless of its version.
 */

/* Implementation: Public API */

/* Return type of WnckWindow as our type */
GType xfdashboard_window_tracker_window_get_type(void)
{
	return(WNCK_TYPE_WINDOW);
}

/* Determine if window is visible */
gboolean xfdashboard_window_tracker_window_is_visible(XfdashboardWindowTrackerWindow *inWindow)
{
	g_return_val_if_fail(WNCK_IS_WINDOW(inWindow), FALSE);

	return((wnck_window_get_state(WNCK_WINDOW(inWindow)) & WNCK_WINDOW_STATE_HIDDEN) ? FALSE : TRUE);
}

gboolean xfdashboard_window_tracker_window_is_visible_on_workspace(XfdashboardWindowTrackerWindow *inWindow,
																	XfdashboardWindowTrackerWorkspace *inWorkspace)
{
	g_return_val_if_fail(WNCK_IS_WINDOW(inWindow), FALSE);
	g_return_val_if_fail(WNCK_IS_WORKSPACE(inWorkspace), FALSE);

	return(wnck_window_is_visible_on_workspace(WNCK_WINDOW(inWindow), WNCK_WORKSPACE(inWorkspace)));
}

/* Get workspace where window is on */
XfdashboardWindowTrackerWorkspace* xfdashboard_window_tracker_window_get_workspace(XfdashboardWindowTrackerWindow *inWindow)
{
	g_return_val_if_fail(WNCK_IS_WINDOW(inWindow), NULL);

	return(wnck_window_get_workspace(WNCK_WINDOW(inWindow)));
}

/* Determine if window is on requested workspace */
gboolean xfdashboard_window_tracker_window_is_on_workspace(XfdashboardWindowTrackerWindow *inWindow,
															XfdashboardWindowTrackerWorkspace *inWorkspace)
{
	g_return_val_if_fail(WNCK_IS_WINDOW(inWindow), FALSE);
	g_return_val_if_fail(WNCK_IS_WORKSPACE(inWorkspace), FALSE);

	return(wnck_window_is_on_workspace(WNCK_WINDOW(inWindow), WNCK_WORKSPACE(inWorkspace)));
}

/* Move a window to another workspace */
void xfdashboard_window_tracker_window_move_to_workspace(XfdashboardWindowTrackerWindow *inWindow,
															XfdashboardWindowTrackerWorkspace *inWorkspace)
{
	g_return_if_fail(WNCK_IS_WINDOW(inWindow));
	g_return_if_fail(WNCK_IS_WORKSPACE(inWorkspace));

	wnck_window_move_to_workspace(WNCK_WINDOW(inWindow), WNCK_WORKSPACE(inWorkspace));
}

/* Get name (title) of window */
const gchar* xfdashboard_window_tracker_window_get_title(XfdashboardWindowTrackerWindow *inWindow)
{
	g_return_val_if_fail(WNCK_IS_WINDOW(inWindow), NULL);

	if(!wnck_window_has_name(WNCK_WINDOW(inWindow))) return(NULL);

	return(wnck_window_get_name(WNCK_WINDOW(inWindow)));
}

/* Get icon of window */
GdkPixbuf* xfdashboard_window_tracker_window_get_icon(XfdashboardWindowTrackerWindow *inWindow)
{
	g_return_val_if_fail(WNCK_IS_WINDOW(inWindow), NULL);

	return(wnck_window_get_icon(WNCK_WINDOW(inWindow)));
}

const gchar* xfdashboard_window_tracker_window_get_icon_name(XfdashboardWindowTrackerWindow *inWindow)
{
	g_return_val_if_fail(WNCK_IS_WINDOW(inWindow), NULL);

	if(!wnck_window_has_icon_name(WNCK_WINDOW(inWindow))) return(NULL);

	return(wnck_window_get_icon_name(WNCK_WINDOW(inWindow)));
}

/* Get state of window */
gboolean xfdashboard_window_tracker_window_is_skip_pager(XfdashboardWindowTrackerWindow *inWindow)
{
	g_return_val_if_fail(WNCK_IS_WINDOW(inWindow), FALSE);

	return(wnck_window_is_skip_pager(WNCK_WINDOW(inWindow)));
}

gboolean xfdashboard_window_tracker_window_is_skip_tasklist(XfdashboardWindowTrackerWindow *inWindow)
{
	g_return_val_if_fail(WNCK_IS_WINDOW(inWindow), FALSE);

	return(wnck_window_is_skip_tasklist(WNCK_WINDOW(inWindow)));
}

gboolean xfdashboard_window_tracker_window_is_pinned(XfdashboardWindowTrackerWindow *inWindow)
{
	g_return_val_if_fail(WNCK_IS_WINDOW(inWindow), FALSE);

	return(wnck_window_is_pinned(WNCK_WINDOW(inWindow)));
}

/* Get possible actions of window */
gboolean xfdashboard_window_tracker_window_has_close_action(XfdashboardWindowTrackerWindow *inWindow)
{
	g_return_val_if_fail(WNCK_IS_WINDOW(inWindow), FALSE);

	return((wnck_window_get_actions(WNCK_WINDOW(inWindow)) & WNCK_WINDOW_ACTION_CLOSE) ? TRUE : FALSE);
}

/* Activate window with its transient windows */
void xfdashboard_window_tracker_window_activate(XfdashboardWindowTrackerWindow *inWindow)
{
	g_return_if_fail(WNCK_IS_WINDOW(inWindow));

	wnck_window_activate_transient(WNCK_WINDOW(inWindow), xfdashboard_get_current_time());
}

/* Close window */
void xfdashboard_window_tracker_window_close(XfdashboardWindowTrackerWindow *inWindow)
{
	g_return_if_fail(WNCK_IS_WINDOW(inWindow));

	wnck_window_close(WNCK_WINDOW(inWindow), xfdashboard_get_current_time());
}

/* Move and resize window */
void xfdashboard_window_tracker_window_move(XfdashboardWindowTrackerWindow *inWindow,
												gint inX,
												gint inY)
{
	xfdashboard_window_tracker_window_move_resize(inWindow, inX, inY, -1, -1);
}

void xfdashboard_window_tracker_window_resize(XfdashboardWindowTrackerWindow *inWindow,
												gint inWidth,
												gint inHeight)
{
	xfdashboard_window_tracker_window_move_resize(inWindow, -1, -1, inWidth, inHeight);
}

void xfdashboard_window_tracker_window_move_resize(XfdashboardWindowTrackerWindow *inWindow,
													gint inX,
													gint inY,
													gint inWidth,
													gint inHeight)
{
	WnckWindowMoveResizeMask	flags;

	g_return_if_fail(WNCK_IS_WINDOW(inWindow));

	/* Get modification flags */
	flags=0;
	if(inX>=0) flags|=WNCK_WINDOW_CHANGE_X;
	if(inY>=0) flags|=WNCK_WINDOW_CHANGE_Y;
	if(inWidth>=0) flags|=WNCK_WINDOW_CHANGE_WIDTH;
	if(inHeight>=0) flags|=WNCK_WINDOW_CHANGE_HEIGHT;

	/* Set geometry */
	wnck_window_set_geometry(WNCK_WINDOW(inWindow),
								WNCK_WINDOW_GRAVITY_STATIC,
								flags,
								inX, inY, inWidth, inHeight);
}

/* Find stage by requested window */
ClutterStage* xfdashboard_window_tracker_window_find_stage(XfdashboardWindowTrackerWindow *inWindow)
{
	ClutterStage			*foundStage;
	ClutterStage			*stage;
	Window					stageXWindow;
	GSList					*stages, *entry;

	g_return_val_if_fail(WNCK_IS_WINDOW(inWindow), FALSE);

	/* Iterate through stages and check if stage window matches requested one */
	foundStage=NULL;
	stages=clutter_stage_manager_list_stages(clutter_stage_manager_get_default());
	for(entry=stages; !foundStage && entry; entry=g_slist_next(entry))
	{
		stage=CLUTTER_STAGE(entry->data);
		if(stage)
		{
			stageXWindow=clutter_x11_get_stage_window(stage);
			if(stageXWindow==wnck_window_get_xid(WNCK_WINDOW(inWindow))) foundStage=stage;
		}
	}
	g_slist_free(stages);

	return(foundStage);
}

/* Get window of stage */
XfdashboardWindowTrackerWindow* xfdashboard_window_tracker_window_get_stage_window(ClutterStage *inStage)
{
	Window					stageXWindow;
	WnckWindow				*window;

	g_return_val_if_fail(CLUTTER_IS_STAGE(inStage), NULL);

	/* Get stage X window and translate to needed window type */
	stageXWindow=clutter_x11_get_stage_window(inStage);
	window=wnck_window_get(stageXWindow);

	return(window);
}


/* Set up window for use as stage window */
void xfdashboard_window_tracker_window_make_stage_window(XfdashboardWindowTrackerWindow *inWindow)
{
	g_return_if_fail(WNCK_IS_WINDOW(inWindow));

	/* Window of stage should always be above all other windows,
	 * pinned to all workspaces and not be listed in window pager
	 */
	wnck_window_set_skip_tasklist(WNCK_WINDOW(inWindow), TRUE);
	wnck_window_set_skip_pager(WNCK_WINDOW(inWindow), TRUE);
	wnck_window_make_above(WNCK_WINDOW(inWindow));
	wnck_window_pin(WNCK_WINDOW(inWindow));
}

/* Get X window ID of window */
gulong xfdashboard_window_tracker_window_get_xid(XfdashboardWindowTrackerWindow *inWindow)
{
	g_return_val_if_fail(WNCK_IS_WINDOW(inWindow), None);

	return(wnck_window_get_xid(WNCK_WINDOW(inWindow)));
}
