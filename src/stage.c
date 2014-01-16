/*
 * stage: A stage for a monitor
 * 
 * Copyright 2012-2014 Stephan Haller <nomad@froevel.de>
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
#include <gtk/gtk.h>

#include "utils.h"
#include "application.h"
#include "viewpad.h"
#include "view-selector.h"
#include "text-box.h"
#include "quicklaunch.h"
#include "applications-view.h"
#include "windows-view.h"
#include "search-view.h"
#include "toggle-button.h"
#include "workspace-selector.h"
#include "collapse-box.h"

/* TODO: Replace by settings/theming object (or check monitor directions if more than one monitor)
 * Meanwhile #undef STAGE_USE_HORIZONTAL_WORKSPACE_SELECTOR for vertical workspace
 */
#undef STAGE_USE_HORIZONTAL_WORKSPACE_SELECTOR

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
	ClutterActor				*quicklaunch;
	ClutterActor				*searchbox;
	ClutterActor				*workspaces;
	ClutterActor				*viewpad;
	ClutterActor				*viewSelector;
	ClutterActor				*notification;

	/* Instance related */
	XfdashboardWindowTracker	*windowTracker;

	gboolean					searchActive;
	gint						lastSearchTextLength;
	XfdashboardView				*viewBeforeSearch;

	guint						notificationTimeoutID;
};

/* Signals */
enum
{
	SIGNAL_SEARCH_STARTED,
	SIGNAL_SEARCH_CHANGED,
	SIGNAL_SEARCH_ENDED,

	SIGNAL_LAST
};

static guint XfdashboardStageSignals[SIGNAL_LAST]={ 0, };


/* IMPLEMENTATION: Private variables and methods */
static ClutterColor		defaultStageColor={ 0x00, 0x00, 0x00, 0xe0 };					// TODO: Replace by settings/theming object
static ClutterColor		defaultButtonHighlightColor={ 0xc0, 0xc0, 0xc0, 0xa0 };			// TODO: Replace by settings/theming object
static ClutterColor		defaultNotificationFillColor={ 0x13, 0x50, 0xff, 0xff };		// TODO: Replace by settings/theming object
static ClutterColor		defaultNotificationOutlineColor={ 0x63, 0xb0, 0xff, 0xff };		// TODO: Replace by settings/theming object

#define DEFAULT_NOTIFICATION_OUTLINE_WIDTH		1.0f									// TODO: Replace by settings/theming object
#define DEFAULT_NOTIFICATION_TIMEOUT			3000
#define NOTIFICATION_TIMEOUT_XFCONF_PROP		"/min-notification-timeout"

/* Notification timeout has been reached */
static void _xfdashboard_stage_on_notification_timeout_destroyed(gpointer inUserData)
{
	XfdashboardStage			*self;
	XfdashboardStagePrivate		*priv;

	g_return_if_fail(XFDASHBOARD_IS_STAGE(inUserData));

	self=XFDASHBOARD_STAGE(inUserData);
	priv=self->priv;

	/* Timeout source was destroy so just reset ID to 0 */
	priv->notificationTimeoutID=0;
}

static gboolean _xfdashboard_stage_on_notification_timeout(gpointer inUserData)
{
	XfdashboardStage			*self;
	XfdashboardStagePrivate		*priv;

	g_return_val_if_fail(XFDASHBOARD_IS_STAGE(inUserData), G_SOURCE_REMOVE);

	self=XFDASHBOARD_STAGE(inUserData);
	priv=self->priv;

	/* Timeout reached so hide notification */
	clutter_actor_hide(priv->notification);

	/* Tell main context to remove this source */
	return(G_SOURCE_REMOVE);
}

/* A pressed key was released */
static gboolean _xfdashboard_stage_on_key_release(XfdashboardStage *self,
													ClutterEvent *inEvent,
													gpointer inUserData)
{
	XfdashboardStagePrivate		*priv;

	g_return_val_if_fail(XFDASHBOARD_IS_STAGE(self), CLUTTER_EVENT_PROPAGATE);

	priv=self->priv;

	/* Handle escape key */
	if(inEvent->key.keyval==CLUTTER_Escape)
	{
		/* If search is active then end search by clearing search box ... */
		if(priv->searchbox &&
			!xfdashboard_text_box_is_empty(XFDASHBOARD_TEXT_BOX(priv->searchbox)))
		{
			xfdashboard_text_box_set_text(XFDASHBOARD_TEXT_BOX(priv->searchbox), NULL);
			return(CLUTTER_EVENT_STOP);
		}
			/* ... otherwise quit application */
			else
			{
				xfdashboard_application_quit();
				return(CLUTTER_EVENT_STOP);
			}
	}

	/* We did not handle this event so let next actor in clutter's chain handle it */
	return(CLUTTER_EVENT_PROPAGATE);
}

/* A view button in view-selector was toggled */
static void _xfdashboard_stage_on_view_selector_button_toggled(XfdashboardStage *self,
																XfdashboardToggleButton *inViewButton,
																gpointer inUserData)
{
	gboolean					state;

	g_return_if_fail(XFDASHBOARD_IS_STAGE(self));
	g_return_if_fail(XFDASHBOARD_IS_TOGGLE_BUTTON(inViewButton));

	/* Get state of view button and set up new appearance */
	state=xfdashboard_toggle_button_get_toggle_state(inViewButton);
	if(state==FALSE)
	{
		xfdashboard_background_set_background_type(XFDASHBOARD_BACKGROUND(inViewButton), XFDASHBOARD_BACKGROUND_TYPE_NONE);
	}
		else
		{
			xfdashboard_background_set_fill_color(XFDASHBOARD_BACKGROUND(inViewButton), &defaultButtonHighlightColor);
			xfdashboard_background_set_background_type(XFDASHBOARD_BACKGROUND(inViewButton), XFDASHBOARD_BACKGROUND_TYPE_FILL);
		}
}

/* App-button was toggled */
static void _xfdashboard_stage_on_quicklaunch_apps_button_toggled(XfdashboardStage *self, gpointer inUserData)
{
	XfdashboardStagePrivate		*priv;
	XfdashboardToggleButton		*appsButton;
	gboolean					state;
	XfdashboardView				*view;

	g_return_if_fail(XFDASHBOARD_IS_STAGE(self));

	priv=self->priv;

	/* Get state of apps button */
	appsButton=xfdashboard_quicklaunch_get_apps_button(XFDASHBOARD_QUICKLAUNCH(priv->quicklaunch));
	g_return_if_fail(appsButton);

	state=xfdashboard_toggle_button_get_toggle_state(XFDASHBOARD_TOGGLE_BUTTON(appsButton));

	/* Depending on state activate view and set up new appearance of apps button */
	if(state==FALSE)
	{
		/* Find "windows-view" view and activate */
		view=xfdashboard_viewpad_find_view_by_type(XFDASHBOARD_VIEWPAD(priv->viewpad), XFDASHBOARD_TYPE_WINDOWS_VIEW);
		if(view) xfdashboard_viewpad_set_active_view(XFDASHBOARD_VIEWPAD(priv->viewpad), view);

		/* Set up new appearance of apps button */
		xfdashboard_background_set_background_type(XFDASHBOARD_BACKGROUND(appsButton), XFDASHBOARD_BACKGROUND_TYPE_NONE);
	}
		else
		{
			/* Find "applications" or "search" view and activate */
			if(!priv->searchActive) view=xfdashboard_viewpad_find_view_by_type(XFDASHBOARD_VIEWPAD(priv->viewpad), XFDASHBOARD_TYPE_APPLICATIONS_VIEW);
				else view=xfdashboard_viewpad_find_view_by_type(XFDASHBOARD_VIEWPAD(priv->viewpad), XFDASHBOARD_TYPE_SEARCH_VIEW);
			if(view) xfdashboard_viewpad_set_active_view(XFDASHBOARD_VIEWPAD(priv->viewpad), view);

			/* Set up new appearance of apps button */
			xfdashboard_background_set_background_type(XFDASHBOARD_BACKGROUND(appsButton), XFDASHBOARD_BACKGROUND_TYPE_FILL);
		}
}

/* Text in search text-box has changed */
static void _xfdashboard_stage_on_searchbox_text_changed(XfdashboardStage *self,
															gchar *inText,
															gpointer inUserData)
{
	XfdashboardStagePrivate		*priv;
	XfdashboardTextBox			*textBox=XFDASHBOARD_TEXT_BOX(inUserData);
	XfdashboardView				*searchView;
	gint						textLength;
	const gchar					*text;
	XfdashboardToggleButton		*appsButton;

	g_return_if_fail(XFDASHBOARD_IS_STAGE(self));
	g_return_if_fail(XFDASHBOARD_IS_TEXT_BOX(inUserData));

	priv=self->priv;

	/* Get search view */
	searchView=xfdashboard_viewpad_find_view_by_type(XFDASHBOARD_VIEWPAD(priv->viewpad), XFDASHBOARD_TYPE_SEARCH_VIEW);
	if(searchView==NULL)
	{
		g_critical(_("Cannot perform search because search view was not found in viewpad."));
		return;
	}

	/* Get text and length of text in text-box */
	text=xfdashboard_text_box_get_text(textBox);
	textLength=xfdashboard_text_box_get_length(textBox);

	/* Get apps button of quicklaunch */
	appsButton=xfdashboard_quicklaunch_get_apps_button(XFDASHBOARD_QUICKLAUNCH(priv->quicklaunch));

	/* Check if current text length if greater than zero and previous text length
	 * was zero. If check is successful it marks the start of a search. Emit the
	 * "search-started" signal. There is no need to start a search a search over
	 * all search providers as it will be done later by updating search criterias.
	 * There is also no need to activate search view because we will ensure that
	 * search view is activate on any change in search text box but we enable that
	 * view to be able to activate it ;)
	 */
	if(textLength>0 && priv->lastSearchTextLength==0)
	{
		/* Remember current active view to restore it when search ended */
		priv->viewBeforeSearch=XFDASHBOARD_VIEW(g_object_ref(xfdashboard_viewpad_get_active_view(XFDASHBOARD_VIEWPAD(priv->viewpad))));

		/* Enable search view */
		xfdashboard_view_set_enabled(searchView, TRUE);

		/* Activate "clear" button on text box */
		xfdashboard_text_box_set_secondary_icon(XFDASHBOARD_TEXT_BOX(priv->searchbox), GTK_STOCK_CLEAR);

		/* Change apps button appearance */
		if(appsButton) xfdashboard_button_set_icon(XFDASHBOARD_BUTTON(appsButton), GTK_STOCK_FIND);

		/* Emit "search-started" signal */
		g_signal_emit(self, XfdashboardStageSignals[SIGNAL_SEARCH_STARTED], 0);
		priv->searchActive=TRUE;
	}

	/* Ensure that search view is active, emit signal for text changed,
	 * update search criterias and set active toggle state at apps button
	 */
	xfdashboard_viewpad_set_active_view(XFDASHBOARD_VIEWPAD(priv->viewpad), searchView);
	xfdashboard_search_view_update_search(XFDASHBOARD_SEARCH_VIEW(searchView), text);
	g_signal_emit(self, XfdashboardStageSignals[SIGNAL_SEARCH_CHANGED], 0, text);

	if(appsButton) xfdashboard_toggle_button_set_toggle_state(appsButton, TRUE);

	/* Check if current text length is zero and previous text length was greater
	 * than zero. If check is successful it marks the end of current search. Emit
	 * the "search-ended" signal, reactivate view before search was started and
	 * disable search view.
	 */
	if(textLength==0 && priv->lastSearchTextLength>0)
	{
		/* Reactivate active view before search has started */
		if(priv->viewBeforeSearch)
		{
			xfdashboard_viewpad_set_active_view(XFDASHBOARD_VIEWPAD(priv->viewpad), priv->viewBeforeSearch);
			g_object_unref(priv->viewBeforeSearch);
			priv->viewBeforeSearch=NULL;
		}

		/* Deactivate "clear" button on text box */
		xfdashboard_text_box_set_secondary_icon(XFDASHBOARD_TEXT_BOX(priv->searchbox), NULL);

		/* Disable search view */
		xfdashboard_view_set_enabled(searchView, FALSE);

		/* Change apps button appearance */
		if(appsButton) xfdashboard_button_set_icon(XFDASHBOARD_BUTTON(appsButton), GTK_STOCK_HOME);

		/* Emit "search-ended" signal */
		g_signal_emit(self, XfdashboardStageSignals[SIGNAL_SEARCH_ENDED], 0);
		priv->searchActive=FALSE;
	}

	/* Trace text length changes */
	priv->lastSearchTextLength=textLength;
}

/* Secondary icon ("clear") on text box was clicked */
static void _xfdashboard_stage_on_searchbox_secondary_icon_clicked(XfdashboardStage *self, gpointer inUserData)
{
	XfdashboardTextBox			*textBox;

	g_return_if_fail(XFDASHBOARD_IS_STAGE(self));
	g_return_if_fail(XFDASHBOARD_IS_TEXT_BOX(inUserData));

	textBox=XFDASHBOARD_TEXT_BOX(inUserData);

	/* Clear search text box */
	xfdashboard_text_box_set_text(textBox, NULL);
}

/* Active view in viewpad has changed */
static void _xfdashboard_stage_on_view_activated(XfdashboardStage *self, XfdashboardView *inView, gpointer inUserData)
{
	XfdashboardStagePrivate		*priv;
	XfdashboardViewpad			*viewpad G_GNUC_UNUSED;
	XfdashboardToggleButton		*appsButton;

	g_return_if_fail(XFDASHBOARD_IS_STAGE(self));
	g_return_if_fail(XFDASHBOARD_IS_VIEWPAD(inUserData));

	priv=self->priv;
	viewpad=XFDASHBOARD_VIEWPAD(inUserData);

	/* If we have remembered a view "before-search" then a search is going on.
	 * If user switches between views while a search is going on remember the
	 * last one activated to restore it when search ends but do not remember
	 * the search view!
	 */
	if(priv->viewBeforeSearch &&
		G_OBJECT_TYPE(inView)!=XFDASHBOARD_TYPE_SEARCH_VIEW)
	{
		/* Release old remembered view */
		g_object_unref(priv->viewBeforeSearch);

		/* Remember new active view */
		priv->viewBeforeSearch=XFDASHBOARD_VIEW(g_object_ref(inView));
	}

	/* Update toggle state of apps button */
	appsButton=xfdashboard_quicklaunch_get_apps_button(XFDASHBOARD_QUICKLAUNCH(priv->quicklaunch));
	if(appsButton)
	{
		if(G_OBJECT_TYPE(inView)==XFDASHBOARD_TYPE_SEARCH_VIEW ||
			G_OBJECT_TYPE(inView)==XFDASHBOARD_TYPE_APPLICATIONS_VIEW)
		{
			xfdashboard_toggle_button_set_toggle_state(appsButton, TRUE);
		}
			else
			{
				xfdashboard_toggle_button_set_toggle_state(appsButton, FALSE);
			}
	}
}

/* Set up stage */
static void _xfdashboard_stage_setup(XfdashboardStage *self)
{
	/* TODO: Implement missing actors, do setup nicer and themable/layoutable */
	/* TODO: Create background by copying background of Xfce */

	XfdashboardStagePrivate		*priv;
	ClutterActor				*groupHorizontal;
	ClutterActor				*groupVertical;
	ClutterActor				*collapseBox;
	ClutterLayoutManager		*layout;
	ClutterColor				color;
	XfdashboardToggleButton		*appsButton;

	g_return_if_fail(XFDASHBOARD_IS_STAGE(self));

	priv=self->priv;

	/* Set up layout objects */
	layout=clutter_box_layout_new();
	clutter_box_layout_set_orientation(CLUTTER_BOX_LAYOUT(layout), CLUTTER_ORIENTATION_VERTICAL);
	clutter_box_layout_set_spacing(CLUTTER_BOX_LAYOUT(layout), 8.0f);
	clutter_box_layout_set_homogeneous(CLUTTER_BOX_LAYOUT(layout), FALSE);
	groupVertical=clutter_actor_new();
	clutter_actor_set_x_expand(groupVertical, TRUE);
	clutter_actor_set_y_expand(groupVertical, TRUE);
	clutter_actor_set_layout_manager(groupVertical, layout);

	/* Searchbox and view selector */
	layout=clutter_box_layout_new();
	clutter_box_layout_set_orientation(CLUTTER_BOX_LAYOUT(layout), CLUTTER_ORIENTATION_HORIZONTAL);
	clutter_box_layout_set_spacing(CLUTTER_BOX_LAYOUT(layout), 8.0f);
	groupHorizontal=clutter_actor_new();
	clutter_actor_set_x_expand(groupHorizontal, TRUE);
	clutter_actor_set_layout_manager(groupHorizontal, layout);

	priv->viewSelector=xfdashboard_view_selector_new();
	clutter_actor_add_child(groupHorizontal, priv->viewSelector);
	g_signal_connect_swapped(priv->viewSelector, "state-changed", G_CALLBACK(_xfdashboard_stage_on_view_selector_button_toggled), self);

	priv->searchbox=xfdashboard_text_box_new();
	clutter_actor_set_x_expand(priv->searchbox, TRUE);
	xfdashboard_background_set_background_type(XFDASHBOARD_BACKGROUND(priv->searchbox),
												XFDASHBOARD_BACKGROUND_TYPE_FILL |
												XFDASHBOARD_BACKGROUND_TYPE_OUTLINE |
												XFDASHBOARD_BACKGROUND_TYPE_ROUNDED_CORNERS);
	clutter_color_init(&color, 0xff, 0xff, 0xff, 0x18);
	xfdashboard_background_set_fill_color(XFDASHBOARD_BACKGROUND(priv->searchbox), &color);
	clutter_color_init(&color, 0x80, 0x80, 0x80, 0xff);
	xfdashboard_background_set_outline_color(XFDASHBOARD_BACKGROUND(priv->searchbox), &color);
	xfdashboard_background_set_outline_width(XFDASHBOARD_BACKGROUND(priv->searchbox), 0.5f);
	xfdashboard_background_set_corners(XFDASHBOARD_BACKGROUND(priv->searchbox), XFDASHBOARD_CORNERS_ALL);
	xfdashboard_background_set_corner_radius(XFDASHBOARD_BACKGROUND(priv->searchbox), 4.0f);
	xfdashboard_text_box_set_padding(XFDASHBOARD_TEXT_BOX(priv->searchbox), 4.0f);
	xfdashboard_text_box_set_editable(XFDASHBOARD_TEXT_BOX(priv->searchbox), TRUE);
	xfdashboard_text_box_set_hint_text(XFDASHBOARD_TEXT_BOX(priv->searchbox), _("Just type to search..."));
	xfdashboard_text_box_set_primary_icon(XFDASHBOARD_TEXT_BOX(priv->searchbox), GTK_STOCK_FIND);
	g_signal_connect_swapped(priv->searchbox, "text-changed", G_CALLBACK(_xfdashboard_stage_on_searchbox_text_changed), self);
	g_signal_connect_swapped(priv->searchbox, "secondary-icon-clicked", G_CALLBACK(_xfdashboard_stage_on_searchbox_secondary_icon_clicked), self);
	clutter_actor_add_child(groupHorizontal, priv->searchbox);

	clutter_actor_add_child(groupVertical, groupHorizontal);

	/* Views */
	priv->viewpad=xfdashboard_viewpad_new();
	clutter_actor_set_x_expand(priv->viewpad, TRUE);
	clutter_actor_set_y_expand(priv->viewpad, TRUE);
	clutter_actor_add_child(groupVertical, priv->viewpad);
	g_signal_connect_swapped(priv->viewpad, "view-activated", G_CALLBACK(_xfdashboard_stage_on_view_activated), self);
	xfdashboard_view_selector_set_viewpad(XFDASHBOARD_VIEW_SELECTOR(priv->viewSelector), XFDASHBOARD_VIEWPAD(priv->viewpad));

#ifdef STAGE_USE_HORIZONTAL_WORKSPACE_SELECTOR
	/* Workspaces selector */
	priv->workspaces=xfdashboard_workspace_selector_new();
	xfdashboard_workspace_selector_set_orientation(XFDASHBOARD_WORKSPACE_SELECTOR(priv->workspaces), CLUTTER_ORIENTATION_HORIZONTAL);
	xfdashboard_workspace_selector_set_spacing(XFDASHBOARD_WORKSPACE_SELECTOR(priv->workspaces), 4.0f);
	xfdashboard_background_set_background_type(XFDASHBOARD_BACKGROUND(priv->workspaces),
												XFDASHBOARD_BACKGROUND_TYPE_FILL |
												XFDASHBOARD_BACKGROUND_TYPE_OUTLINE |
												XFDASHBOARD_BACKGROUND_TYPE_ROUNDED_CORNERS);
	clutter_color_init(&color, 0xff, 0xff, 0xff, 0x18);
	xfdashboard_background_set_fill_color(XFDASHBOARD_BACKGROUND(priv->workspaces), &color);
	xfdashboard_background_set_outline_width(XFDASHBOARD_BACKGROUND(priv->workspaces), 0.5f);
	xfdashboard_background_set_corners(XFDASHBOARD_BACKGROUND(priv->workspaces), XFDASHBOARD_CORNERS_TOP);
	clutter_actor_set_x_expand(priv->workspaces, TRUE);

	collapseBox=xfdashboard_collapse_box_new();
	xfdashboard_collapse_box_set_collapsed_size(XFDASHBOARD_COLLAPSE_BOX(collapseBox), 64.0f);
	xfdashboard_collapse_box_set_collapse_orientation(XFDASHBOARD_COLLAPSE_BOX(collapseBox), XFDASHBOARD_ORIENTATION_TOP);
	clutter_actor_set_x_expand(collapseBox, TRUE);
	clutter_actor_add_child(collapseBox, priv->workspaces);
	clutter_actor_add_child(groupVertical, collapseBox);
#endif

	/* Set up layout objects */
	layout=clutter_box_layout_new();
	clutter_box_layout_set_orientation(CLUTTER_BOX_LAYOUT(layout), CLUTTER_ORIENTATION_HORIZONTAL);
	clutter_box_layout_set_spacing(CLUTTER_BOX_LAYOUT(layout), 8.0f);
	clutter_box_layout_set_homogeneous(CLUTTER_BOX_LAYOUT(layout), FALSE);
	groupHorizontal=clutter_actor_new();
	clutter_actor_set_x_expand(groupHorizontal, TRUE);
	clutter_actor_set_y_expand(groupHorizontal, TRUE);
	clutter_actor_set_layout_manager(groupHorizontal, layout);

	/* Quicklaunch */
	priv->quicklaunch=xfdashboard_quicklaunch_new_with_orientation(CLUTTER_ORIENTATION_VERTICAL);
	xfdashboard_quicklaunch_set_spacing(XFDASHBOARD_QUICKLAUNCH(priv->quicklaunch), 4.0f);
	xfdashboard_background_set_background_type(XFDASHBOARD_BACKGROUND(priv->quicklaunch),
												XFDASHBOARD_BACKGROUND_TYPE_FILL |
												XFDASHBOARD_BACKGROUND_TYPE_OUTLINE |
												XFDASHBOARD_BACKGROUND_TYPE_ROUNDED_CORNERS);
	clutter_color_init(&color, 0xff, 0xff, 0xff, 0x18);
	xfdashboard_background_set_fill_color(XFDASHBOARD_BACKGROUND(priv->quicklaunch), &color);
	xfdashboard_background_set_outline_width(XFDASHBOARD_BACKGROUND(priv->quicklaunch), 0.5f);
	xfdashboard_background_set_corners(XFDASHBOARD_BACKGROUND(priv->quicklaunch), XFDASHBOARD_CORNERS_RIGHT);
	clutter_actor_set_y_expand(priv->quicklaunch, TRUE);
	clutter_actor_add_child(groupHorizontal, priv->quicklaunch);

	appsButton=xfdashboard_quicklaunch_get_apps_button(XFDASHBOARD_QUICKLAUNCH(priv->quicklaunch));
	if(appsButton)
	{
		xfdashboard_background_set_fill_color(XFDASHBOARD_BACKGROUND(appsButton), &defaultButtonHighlightColor);
		g_signal_connect_swapped(appsButton, "toggled", G_CALLBACK(_xfdashboard_stage_on_quicklaunch_apps_button_toggled), self);
	}

	/* Set up layout objects */
	clutter_actor_add_child(groupHorizontal, groupVertical);

#ifndef STAGE_USE_HORIZONTAL_WORKSPACE_SELECTOR
	/* Workspaces selector */
	priv->workspaces=xfdashboard_workspace_selector_new();
	xfdashboard_workspace_selector_set_spacing(XFDASHBOARD_WORKSPACE_SELECTOR(priv->workspaces), 4.0f);
	xfdashboard_background_set_background_type(XFDASHBOARD_BACKGROUND(priv->workspaces),
												XFDASHBOARD_BACKGROUND_TYPE_FILL |
												XFDASHBOARD_BACKGROUND_TYPE_OUTLINE |
												XFDASHBOARD_BACKGROUND_TYPE_ROUNDED_CORNERS);
	clutter_color_init(&color, 0xff, 0xff, 0xff, 0x18);
	xfdashboard_background_set_fill_color(XFDASHBOARD_BACKGROUND(priv->workspaces), &color);
	xfdashboard_background_set_outline_width(XFDASHBOARD_BACKGROUND(priv->workspaces), 0.5f);
	xfdashboard_background_set_corners(XFDASHBOARD_BACKGROUND(priv->workspaces), XFDASHBOARD_CORNERS_LEFT);
	clutter_actor_set_y_expand(priv->workspaces, TRUE);

	collapseBox=xfdashboard_collapse_box_new();
	xfdashboard_collapse_box_set_collapsed_size(XFDASHBOARD_COLLAPSE_BOX(collapseBox), 64.0f);
	xfdashboard_collapse_box_set_collapse_orientation(XFDASHBOARD_COLLAPSE_BOX(collapseBox), XFDASHBOARD_ORIENTATION_LEFT);
	clutter_actor_set_y_expand(collapseBox, TRUE);
	clutter_actor_add_child(collapseBox, priv->workspaces);
	clutter_actor_add_child(groupHorizontal, collapseBox);
#endif

	/* Set up layout objects */
	clutter_actor_add_constraint(groupHorizontal, clutter_bind_constraint_new(CLUTTER_ACTOR(self), CLUTTER_BIND_X, 0.0f));
	clutter_actor_add_constraint(groupHorizontal, clutter_bind_constraint_new(CLUTTER_ACTOR(self), CLUTTER_BIND_Y, 8.0f));
	clutter_actor_add_constraint(groupHorizontal, clutter_bind_constraint_new(CLUTTER_ACTOR(self), CLUTTER_BIND_WIDTH, 0.0f));
	clutter_actor_add_constraint(groupHorizontal, clutter_bind_constraint_new(CLUTTER_ACTOR(self), CLUTTER_BIND_HEIGHT, -16.0f));
	clutter_actor_add_child(CLUTTER_ACTOR(self), groupHorizontal);

	/* Notification actor (this actor is above all others and hidden by default) */
	priv->notification=xfdashboard_text_box_new();
	clutter_actor_hide(priv->notification);
	clutter_actor_set_reactive(priv->notification, FALSE);
	clutter_actor_set_fixed_position_set(priv->notification, TRUE);
	clutter_actor_set_z_position(priv->notification, 0.1f);
	clutter_actor_set_request_mode(priv->notification, CLUTTER_REQUEST_HEIGHT_FOR_WIDTH);
	xfdashboard_background_set_background_type(XFDASHBOARD_BACKGROUND(priv->notification),
												XFDASHBOARD_BACKGROUND_TYPE_FILL | XFDASHBOARD_BACKGROUND_TYPE_OUTLINE);
	xfdashboard_background_set_fill_color(XFDASHBOARD_BACKGROUND(priv->notification), &defaultNotificationFillColor);
	xfdashboard_background_set_outline_color(XFDASHBOARD_BACKGROUND(priv->notification), &defaultNotificationOutlineColor);
	xfdashboard_background_set_outline_width(XFDASHBOARD_BACKGROUND(priv->notification), DEFAULT_NOTIFICATION_OUTLINE_WIDTH);
	clutter_actor_add_constraint(priv->notification, clutter_align_constraint_new(CLUTTER_ACTOR(self), CLUTTER_ALIGN_X_AXIS, 0.5f));
	clutter_actor_add_constraint(priv->notification, clutter_align_constraint_new(groupHorizontal, CLUTTER_ALIGN_Y_AXIS, 1.0f));
	clutter_actor_add_child(CLUTTER_ACTOR(self), priv->notification);

	/* Set key focus to searchbox */
	clutter_stage_set_accept_focus(CLUTTER_STAGE(self), TRUE);
	clutter_stage_set_key_focus(CLUTTER_STAGE(self), priv->searchbox);
}

/* A window was created
 * Check for stage window and set up window properties
 */
static void _xfdashboard_stage_on_window_opened(XfdashboardStage *self,
													XfdashboardWindowTrackerWindow *inWindow,
													gpointer inUserData)
{
	XfdashboardStagePrivate				*priv=self->priv;
	XfdashboardWindowTrackerWindow		*stageWindow;
	GdkScreen							*screen;
	gint								primaryMonitor;
	GdkRectangle						geometry;

	g_return_if_fail(XFDASHBOARD_IS_STAGE(self));
	g_return_if_fail(XFDASHBOARD_IS_WINDOW_TRACKER_WINDOW(inWindow));

	priv=self->priv;

	/* Check if window opened is this stage window */
	stageWindow=xfdashboard_window_tracker_window_get_stage_window(CLUTTER_STAGE(self));
	if(stageWindow!=inWindow) return;

	/* TODO: As long as we do not support multi-monitors
	 *       use this hack to ensure stage is in right size
	 */
	screen=gdk_screen_get_default();
	primaryMonitor=gdk_screen_get_primary_monitor(screen);
	gdk_screen_get_monitor_geometry(screen, primaryMonitor, &geometry);
	clutter_actor_set_size(CLUTTER_ACTOR(self), geometry.width, geometry.height);
	xfdashboard_window_tracker_window_move_resize(stageWindow,
													geometry.x, geometry.y,
													geometry.width, geometry.height);

	/* Set up window for use as stage window */
	xfdashboard_window_tracker_window_make_stage_window(inWindow);

	/* Disconnect signal handler as this is a one-time setup of stage window */
	g_debug(_("Stage window was opened and set up. Removing signal handler."));
	g_signal_handlers_disconnect_by_func(priv->windowTracker, G_CALLBACK(_xfdashboard_stage_on_window_opened), self);
}

/* IMPLEMENTATION: GObject */

/* Dispose this object */
static void _xfdashboard_stage_dispose(GObject *inObject)
{
	XfdashboardStage			*self=XFDASHBOARD_STAGE(inObject);
	XfdashboardStagePrivate		*priv=self->priv;

	/* Release allocated resources */
	if(priv->notificationTimeoutID)
	{
		g_source_remove(priv->notificationTimeoutID);
		priv->notificationTimeoutID=0;
	}

	if(priv->windowTracker)
	{
		g_signal_handlers_disconnect_by_data(priv->windowTracker, self);
		g_object_unref(priv->windowTracker);
		priv->windowTracker=NULL;
	}

	if(priv->notification)
	{
		clutter_actor_destroy(CLUTTER_ACTOR(priv->notification));
		priv->notification=NULL;
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

	if(priv->viewSelector)
	{
		clutter_actor_destroy(priv->viewSelector);
		priv->viewSelector=NULL;
	}

	if(priv->viewpad)
	{
		clutter_actor_destroy(priv->viewpad);
		priv->viewpad=NULL;
	}

	if(priv->viewBeforeSearch)
	{
		g_object_unref(priv->viewBeforeSearch);
		priv->viewBeforeSearch=NULL;
	}

	/* Call parent's class dispose method */
	G_OBJECT_CLASS(xfdashboard_stage_parent_class)->dispose(inObject);
}

/* Class initialization
 * Override functions in parent classes and define properties
 * and signals
 */
static void xfdashboard_stage_class_init(XfdashboardStageClass *klass)
{
	GObjectClass		*gobjectClass=G_OBJECT_CLASS(klass);

	/* Override functions */
	gobjectClass->dispose=_xfdashboard_stage_dispose;

	/* Set up private structure */
	g_type_class_add_private(klass, sizeof(XfdashboardStagePrivate));

	/* Define signals */
	XfdashboardStageSignals[SIGNAL_SEARCH_STARTED]=
		g_signal_new("search-started",
						G_TYPE_FROM_CLASS(klass),
						G_SIGNAL_RUN_LAST,
						G_STRUCT_OFFSET(XfdashboardStageClass, search_started),
						NULL,
						NULL,
						g_cclosure_marshal_VOID__VOID,
						G_TYPE_NONE,
						0);

	XfdashboardStageSignals[SIGNAL_SEARCH_CHANGED]=
		g_signal_new("search-changed",
						G_TYPE_FROM_CLASS(klass),
						G_SIGNAL_RUN_LAST,
						G_STRUCT_OFFSET(XfdashboardStageClass, search_changed),
						NULL,
						NULL,
						g_cclosure_marshal_VOID__STRING,
						G_TYPE_NONE,
						1,
						G_TYPE_STRING);

	XfdashboardStageSignals[SIGNAL_SEARCH_ENDED]=
		g_signal_new("search-ended",
						G_TYPE_FROM_CLASS(klass),
						G_SIGNAL_RUN_LAST,
						G_STRUCT_OFFSET(XfdashboardStageClass, search_ended),
						NULL,
						NULL,
						g_cclosure_marshal_VOID__VOID,
						G_TYPE_NONE,
						0);
}

/* Object initialization
 * Create private structure and set up default values
 */
static void xfdashboard_stage_init(XfdashboardStage *self)
{
	XfdashboardStagePrivate		*priv;

	priv=self->priv=XFDASHBOARD_STAGE_GET_PRIVATE(self);

	/* Set default values */
	priv->windowTracker=xfdashboard_window_tracker_get_default();

	priv->quicklaunch=NULL;
	priv->searchbox=NULL;
	priv->workspaces=NULL;
	priv->viewpad=NULL;
	priv->viewSelector=NULL;
	priv->notification=NULL;
	priv->lastSearchTextLength=0;
	priv->viewBeforeSearch=NULL;
	priv->searchActive=FALSE;
	priv->notificationTimeoutID=0;

	/* Set up stage */
	clutter_actor_set_background_color(CLUTTER_ACTOR(self), &defaultStageColor);
	clutter_stage_set_use_alpha(CLUTTER_STAGE(self), TRUE);
	clutter_stage_set_user_resizable(CLUTTER_STAGE(self), FALSE);
	clutter_stage_set_fullscreen(CLUTTER_STAGE(self), TRUE);

	_xfdashboard_stage_setup(self);

	g_signal_connect_swapped(self, "key-release-event", G_CALLBACK(_xfdashboard_stage_on_key_release), self);

	/* Connect signals */
	g_signal_connect_swapped(priv->windowTracker, "window-opened", G_CALLBACK(_xfdashboard_stage_on_window_opened), self);
}

/* IMPLEMENTATION: Public API */

/* Create new instance */
ClutterActor* xfdashboard_stage_new(void)
{
	return(CLUTTER_ACTOR(g_object_new(XFDASHBOARD_TYPE_STAGE, NULL)));
}

/* Show a notification on stage */
void xfdashboard_stage_show_notification(XfdashboardStage *self, const gchar *inIconName, const gchar *inText)
{
	XfdashboardStagePrivate		*priv;
	gint						interval;

	g_return_if_fail(XFDASHBOARD_IS_STAGE(self));

	priv=self->priv;

	/* Stop current running timeout source because it would hide this
	 * new notification to soon.
	 */
	if(priv->notificationTimeoutID)
	{
		g_source_remove(priv->notificationTimeoutID);
		priv->notificationTimeoutID=0;
	}

	/* Show notification on stage */
	xfdashboard_text_box_set_text(XFDASHBOARD_TEXT_BOX(priv->notification), inText);
	xfdashboard_text_box_set_primary_icon(XFDASHBOARD_TEXT_BOX(priv->notification), inIconName);
	clutter_actor_show(CLUTTER_ACTOR(priv->notification));

	/* Set up timeout source. The timeout interval differs and depends on the length
	 * of the notification text to show but never drops below the minimum timeout configured.
	 * The interval is calculated by one second for 30 characters.
	 */
	interval=xfconf_channel_get_uint(xfdashboard_application_get_xfconf_channel(),
										NOTIFICATION_TIMEOUT_XFCONF_PROP,
										DEFAULT_NOTIFICATION_TIMEOUT);
	interval=MAX((gint)((strlen(inText)/30.0f)*1000.0f), interval);

	priv->notificationTimeoutID=clutter_threads_add_timeout_full(G_PRIORITY_DEFAULT,
																	interval,
																	_xfdashboard_stage_on_notification_timeout,
																	self,
																	_xfdashboard_stage_on_notification_timeout_destroyed);
}
