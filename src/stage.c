/*
 * stage: A stage for a monitor
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

#include "stage.h"

#include <glib/gi18n-lib.h>
#include <clutter/clutter.h>
#include <clutter/x11/clutter-x11.h>

#include "common.h"
#include "application.h"
#include "viewpad.h"

/* Define this class in GObject system */
G_DEFINE_TYPE(XfdashboardStage,
				xfdashboard_stage,
				CLUTTER_TYPE_STAGE)

/* Private structure - access only by public API if needed */
#define XFDASHBOARD_STAGE_GET_PRIVATE(obj) \
	(G_TYPE_INSTANCE_GET_PRIVATE((obj), XFDASHBOARD_TYPE_STAGE, XfdashboardStagePrivate))

struct _XfdashboardStagePrivate
{
	/* Actors */
	ClutterActor		*quicklaunch;
	ClutterActor		*searchbox;
	ClutterActor		*workspaces;
	ClutterActor		*viewpad;

	/* Instance related */
	WnckScreen			*screen;
	WnckWindow			*window;
};

/* Properties */
/* TODO:
enum
{
	PROP_0,

	PROP_LAST
};

GParamSpec* XfdashboardStageProperties[PROP_LAST]={ 0, };
*/

/* IMPLEMENTATION: Private variables and methods */

ClutterColor		stageColor={ 0x00, 0x00, 0x00, 0xe0 }; // TODO: Replace by settings/theming object

/* Set up stage */
void _xfdashboard_stage_setup(XfdashboardStage *self)
{
	g_return_if_fail(XFDASHBOARD_IS_STAGE(self));

	// TODO: Implement missing actors, do setup nicer and themable

	XfdashboardStagePrivate		*priv=self->priv;
	ClutterActor				*groupHorizontal;
	ClutterActor				*groupVertical;
	ClutterLayoutManager		*layout;
	ClutterColor				color={ 0x00, 0x00, 0x00, 0x80 };

	/* Set up layout objects */
	layout=clutter_box_layout_new();
	clutter_box_layout_set_orientation(CLUTTER_BOX_LAYOUT(layout), CLUTTER_ORIENTATION_VERTICAL);
	clutter_box_layout_set_spacing(CLUTTER_BOX_LAYOUT(layout), 8.0f);
	groupVertical=clutter_actor_new();
	clutter_actor_set_x_expand(groupVertical, TRUE);
	clutter_actor_set_y_expand(groupVertical, TRUE);
	clutter_actor_set_layout_manager(groupVertical, layout);

	/* Searchbox */
	priv->searchbox=clutter_actor_new();
	clutter_actor_set_size(priv->searchbox, 32, 32);
	clutter_color_init(&color, 0x00, 0xff, 0x00, 0x80);
	clutter_actor_set_background_color(priv->searchbox, &color);
	clutter_actor_set_x_expand(priv->searchbox, TRUE);
	clutter_actor_add_child(groupVertical, priv->searchbox);

	/* Views */
	priv->viewpad=xfdashboard_viewpad_new();
	clutter_actor_set_x_expand(priv->viewpad, TRUE);
	clutter_actor_set_y_expand(priv->viewpad, TRUE);
	clutter_actor_add_child(groupVertical, priv->viewpad);

	/* Set up layout objects */
	layout=clutter_box_layout_new();
	clutter_box_layout_set_orientation(CLUTTER_BOX_LAYOUT(layout), CLUTTER_ORIENTATION_HORIZONTAL);
	clutter_box_layout_set_spacing(CLUTTER_BOX_LAYOUT(layout), 8.0f);
	groupHorizontal=clutter_actor_new();
	clutter_actor_set_x_expand(groupHorizontal, TRUE);
	clutter_actor_set_y_expand(groupHorizontal, TRUE);
	clutter_actor_set_layout_manager(groupHorizontal, layout);

	/* Quicklaunch */
	priv->quicklaunch=clutter_actor_new();
	clutter_actor_set_size(priv->quicklaunch, 64, 64);
	clutter_color_init(&color, 0xff, 0x00, 0x00, 0x80);
	clutter_actor_set_background_color(priv->quicklaunch, &color);
	clutter_actor_set_y_expand(priv->quicklaunch, TRUE);
	clutter_actor_add_child(groupHorizontal, priv->quicklaunch);

	/* Set up layout objects */
	clutter_actor_add_child(groupHorizontal, groupVertical);

	/* Workspaces selector */
	priv->workspaces=clutter_actor_new();
	clutter_actor_set_size(priv->workspaces, 48, 48);
	clutter_color_init(&color, 0x00, 0x00, 0xff, 0x80);
	clutter_actor_set_background_color(priv->workspaces, &color);
	clutter_actor_set_y_expand(priv->workspaces, TRUE);
	clutter_actor_add_child(groupHorizontal, priv->workspaces);

	/* Set up layout objects */
	clutter_actor_add_constraint(groupHorizontal, clutter_bind_constraint_new(CLUTTER_ACTOR(self), CLUTTER_BIND_X, 0.0f));
	clutter_actor_add_constraint(groupHorizontal, clutter_bind_constraint_new(CLUTTER_ACTOR(self), CLUTTER_BIND_Y, 8.0f));
	clutter_actor_add_constraint(groupHorizontal, clutter_bind_constraint_new(CLUTTER_ACTOR(self), CLUTTER_BIND_WIDTH, 0.0f));
	clutter_actor_add_constraint(groupHorizontal, clutter_bind_constraint_new(CLUTTER_ACTOR(self), CLUTTER_BIND_HEIGHT, -16.0f));
	clutter_actor_add_child(CLUTTER_ACTOR(self), groupHorizontal);
}

/* The active window changed. Reselect stage window as active if it is visible */
void _xfdashboard_stage_on_active_window_changed(XfdashboardStage *self, WnckWindow *inPreviousWindow, gpointer inUserData)
{
	g_return_if_fail(XFDASHBOARD_IS_STAGE(self));
	g_return_if_fail(inPreviousWindow==NULL || WNCK_IS_WINDOW(inPreviousWindow));

	XfdashboardStagePrivate		*priv=self->priv;
	WnckWindow					*stageWindow;

	/* Check if active window deactivated is this stage window */
	stageWindow=xfdashboard_stage_get_window(self);
	if(stageWindow!=inPreviousWindow) return;

	/* Check if stage window should be visible */
	if(CLUTTER_ACTOR_IS_VISIBLE(CLUTTER_ACTOR(self))==TRUE)
	{
		g_debug("Reselect stage window as active window because it is still visible!");
		wnck_window_activate(stageWindow, xfdashboard_get_current_time());
	}
}

/* A window was created
 * Check for stage window and set up window properties
 */
void _xfdashboard_stage_on_window_opened(XfdashboardStage *self, WnckWindow *inWindow, gpointer inUserData)
{
	g_return_if_fail(XFDASHBOARD_IS_STAGE(self));
	g_return_if_fail(WNCK_IS_WINDOW(inWindow));

	XfdashboardStagePrivate		*priv=self->priv;
	WnckWindow					*stageWindow;
	// TODO: gint						x, y;

	/* Check if window opened is this stage window */
	stageWindow=xfdashboard_stage_get_window(self);
	if(stageWindow!=inWindow) return;

	/* Move window to position of monitor it belongs to */
	// TODO: if(xfdashboard_window_manager_get_monitor_allocation(priv->windowManager,
															// TODO: priv->monitorIndex,
															// TODO: &x, &y, NULL, NULL))
	// TODO: {
		// TODO: wnck_window_set_geometry(inWindow,
									// TODO: WNCK_WINDOW_GRAVITY_CURRENT,
									// TODO: WNCK_WINDOW_CHANGE_X | WNCK_WINDOW_CHANGE_Y,
									// TODO: x, y, 0, 0);
		// TODO: g_message("%s: Moved stage window %p of stage %p to %d,%d", __func__, inWindow, inUserData, x, y);
	// TODO: }
		// TODO: else g_warning("Could not get position of monitor %d", priv->monitorIndex);

	/* Window of stage should always be above all other windows,
	 * pinned to all workspaces and not be listed in window pager
	 */
	wnck_window_set_skip_tasklist(inWindow, TRUE);
	wnck_window_set_skip_pager(inWindow, TRUE);
	wnck_window_make_above(inWindow);
	wnck_window_pin(inWindow);

	/* Disconnect signal handler as this is a one-time setup of stage window */
	g_debug(_("Stage window was opened and set up. Removing signal handler."));
	g_signal_handlers_disconnect_by_func(priv->screen, G_CALLBACK(_xfdashboard_stage_on_window_opened), self);
}

/* IMPLEMENTATION: GObject */

/* Dispose this object */
void _xfdashboard_stage_dispose(GObject *inObject)
{
	XfdashboardStage			*self=XFDASHBOARD_STAGE(inObject);
	XfdashboardStagePrivate		*priv=self->priv;

	/* Release allocated resources */
	if(priv->screen)
	{
		g_signal_handlers_disconnect_by_data(priv->screen, self);
		priv->screen=NULL;
	}

	if(priv->quicklaunch)
	{
		clutter_actor_destroy(priv->quicklaunch);
		priv->quicklaunch=NULL;
	}

	if(priv->searchbox)
	{
		clutter_actor_destroy(priv->searchbox);
		priv->searchbox=NULL;
	}

	if(priv->workspaces)
	{
		clutter_actor_destroy(priv->workspaces);
		priv->workspaces=NULL;
	}

	if(priv->viewpad)
	{
		clutter_actor_destroy(priv->viewpad);
		priv->viewpad=NULL;
	}

	/* Call parent's class dispose method */
	G_OBJECT_CLASS(xfdashboard_stage_parent_class)->dispose(inObject);
}

/* Set/get properties */
void _xfdashboard_stage_set_property(GObject *inObject,
										guint inPropID,
										const GValue *inValue,
										GParamSpec *inSpec)
{
	XfdashboardStage			*self=XFDASHBOARD_STAGE(inObject);

	switch(inPropID)
	{
		default:
			G_OBJECT_WARN_INVALID_PROPERTY_ID(inObject, inPropID, inSpec);
			break;
	}
}

void _xfdashboard_stage_get_property(GObject *inObject,
										guint inPropID,
										GValue *outValue,
										GParamSpec *inSpec)
{
	XfdashboardStage	*self=XFDASHBOARD_STAGE(inObject);

	switch(inPropID)
	{
		default:
			G_OBJECT_WARN_INVALID_PROPERTY_ID(inObject, inPropID, inSpec);
			break;
	}
}

/* Class initialization
 * Override functions in parent classes and define properties
 * and signals
 */
void xfdashboard_stage_class_init(XfdashboardStageClass *klass)
{
	GObjectClass		*gobjectClass=G_OBJECT_CLASS(klass);

	/* Override functions */
	gobjectClass->dispose=_xfdashboard_stage_dispose;
	gobjectClass->set_property=_xfdashboard_stage_set_property;
	gobjectClass->get_property=_xfdashboard_stage_get_property;

	/* Set up private structure */
	g_type_class_add_private(klass, sizeof(XfdashboardStagePrivate));

	/* Define properties */
}

/* Object initialization
 * Create private structure and set up default values
 */
void xfdashboard_stage_init(XfdashboardStage *self)
{
	XfdashboardStagePrivate		*priv;
	XfdashboardApplication		*application;

	priv=self->priv=XFDASHBOARD_STAGE_GET_PRIVATE(self);

	/* Set default values */
	priv->screen=wnck_screen_get_default();

	priv->quicklaunch=NULL;
	priv->searchbox=NULL;
	priv->workspaces=NULL;
	priv->viewpad=NULL;

	/* Set up stage */
	clutter_actor_set_background_color(CLUTTER_ACTOR(self), &stageColor);
	clutter_stage_set_use_alpha(CLUTTER_STAGE(self), TRUE);
	clutter_stage_set_user_resizable(CLUTTER_STAGE(self), FALSE);
	// TODO: clutter_stage_set_fullscreen(CLUTTER_STAGE(self), TRUE);

	_xfdashboard_stage_setup(self);

	/* Connect signals */
	g_signal_connect_swapped(priv->screen, "window-opened", G_CALLBACK(_xfdashboard_stage_on_window_opened), self);
	g_signal_connect_swapped(priv->screen, "active-window-changed", G_CALLBACK(_xfdashboard_stage_on_active_window_changed), self);
}

/* Implementation: Public API */

/* Create new instance */
ClutterActor* xfdashboard_stage_new(void)
{
	return(CLUTTER_ACTOR(g_object_new(XFDASHBOARD_TYPE_STAGE, NULL)));
}

/* Get window of application */
WnckWindow* xfdashboard_stage_get_window(XfdashboardStage *self)
{
	g_return_val_if_fail(XFDASHBOARD_IS_STAGE(self), NULL);

	XfdashboardStagePrivate		*priv=self->priv;

	/* Determine window object if not done already */
	if(priv->window==NULL)
	{
		Window					xWindow;

		xWindow=clutter_x11_get_stage_window(CLUTTER_STAGE(self));
		priv->window=wnck_window_get(xWindow);
	}

	return(priv->window);
}
