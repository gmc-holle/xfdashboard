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
#include <gdk/gdk.h>
#include <math.h>

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
#include "tooltip-action.h"
#include "layoutable.h"
#include "stylable.h"
#include "utils.h"
#include "focus-manager.h"

/* Define this class in GObject system */
static void _xfdashboard_stage_layoutable_iface_init(XfdashboardLayoutableInterface *iface);
static void _xfdashboard_stage_stylable_iface_init(XfdashboardStylableInterface *iface);

G_DEFINE_TYPE_WITH_CODE(XfdashboardStage,
						xfdashboard_stage,
						CLUTTER_TYPE_STAGE,
						G_IMPLEMENT_INTERFACE(XFDASHBOARD_TYPE_LAYOUTABLE, _xfdashboard_stage_layoutable_iface_init)
						G_IMPLEMENT_INTERFACE(XFDASHBOARD_TYPE_STYLABLE, _xfdashboard_stage_stylable_iface_init))

/* Private structure - access only by public API if needed */
#define XFDASHBOARD_STAGE_GET_PRIVATE(obj) \
	(G_TYPE_INSTANCE_GET_PRIVATE((obj), XFDASHBOARD_TYPE_STAGE, XfdashboardStagePrivate))

struct _XfdashboardStagePrivate
{
	/* Properties related */
	gchar							*styleClasses;
	gchar							*stylePseudoClasses;

	/* Actors */
	ClutterActor					*quicklaunch;
	ClutterActor					*searchbox;
	ClutterActor					*workspaces;
	ClutterActor					*viewpad;
	ClutterActor					*viewSelector;
	ClutterActor					*notification;
	ClutterActor					*tooltip;

	/* Instance related */
	XfdashboardWindowTracker		*windowTracker;
	XfdashboardWindowTrackerWindow	*stageWindow;

	gboolean						searchActive;
	gint							lastSearchTextLength;
	XfdashboardView					*viewBeforeSearch;

	guint							notificationTimeoutID;

	XfdashboardFocusManager			*focusManager;
};

/* Properties */
enum
{
	PROP_0,

	PROP_STYLE_CLASSES,
	PROP_STYLE_PSEUDO_CLASSES,

	PROP_LAST
};

/* Signals */
enum
{
	SIGNAL_SEARCH_STARTED,
	SIGNAL_SEARCH_CHANGED,
	SIGNAL_SEARCH_ENDED,

	SIGNAL_SHOW_TOOLTIP,

	SIGNAL_LAST
};

static guint XfdashboardStageSignals[SIGNAL_LAST]={ 0, };


/* IMPLEMENTATION: Private variables and methods */
#define NOTIFICATION_TIMEOUT_XFCONF_PROP		"/min-notification-timeout"
#define DEFAULT_NOTIFICATION_TIMEOUT			3000
#define RESET_SEARCH_ON_RESUME_XFCONF_PROP		"/reset-search-on-resume"
#define DEFAULT_RESET_SEARCH_ON_RESUME			TRUE

/* Handle an event */
static gboolean xfdashboard_stage_event(ClutterActor *inActor, ClutterEvent *inEvent)
{
	XfdashboardStage			*self;
	XfdashboardStagePrivate		*priv;
	gboolean					result;

	g_return_val_if_fail(XFDASHBOARD_IS_STAGE(inActor), CLUTTER_EVENT_PROPAGATE);

	self=XFDASHBOARD_STAGE(inActor);
	priv=self->priv;

	/* Do only intercept any event if a focus manager is available */
	if(!priv->focusManager) return(CLUTTER_EVENT_PROPAGATE);

	/* Do only intercept "key-press" and "key-release" events */
	if(clutter_event_type(inEvent)!=CLUTTER_KEY_PRESS &&
		clutter_event_type(inEvent)!=CLUTTER_KEY_RELEASE)
	{
		return(CLUTTER_EVENT_PROPAGATE);
	}

	/* Handle key release event */
	if(clutter_event_type(inEvent)==CLUTTER_KEY_RELEASE)
	{
		gboolean		isShift=FALSE;

		/* Get modifiers we are interested in */
		if(clutter_event_has_shift_modifier(inEvent)) isShift=TRUE;

		/* Handle key */
		switch(inEvent->key.keyval)
		{
			/* Handle TAB and SHIFT-TAB directly to move focus */
			case CLUTTER_KEY_Tab:
			case CLUTTER_KEY_ISO_Left_Tab:
			{
				XfdashboardFocusable	*currentFocusable;
				XfdashboardFocusable	*newFocusable;

				/* Get current focus */
				currentFocusable=xfdashboard_focus_manager_get_focus(priv->focusManager);

				/* If TAB is pressed without SHIFT move focus to next focusable actor.
				 * If TAB is pressed with SHIFT move focus to previous focusable actor.
				 */
				if(!isShift)
				{
					newFocusable=xfdashboard_focus_manager_get_next_focusable(priv->focusManager, currentFocusable);
					if(newFocusable) xfdashboard_focus_manager_set_focus(priv->focusManager, newFocusable);

					return(CLUTTER_EVENT_STOP);
				}
					else
					{
						newFocusable=xfdashboard_focus_manager_get_previous_focusable(priv->focusManager, currentFocusable);
						if(newFocusable) xfdashboard_focus_manager_set_focus(priv->focusManager, newFocusable);

						return(CLUTTER_EVENT_STOP);
					}
			}
			break;

			/* Handle ESC key to clear search box or quit/suspend application */
			case CLUTTER_KEY_Escape:
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
			break;

			default:
				/* Fallthrough */
				break;
		}
	}

	/* Ask focus manager to handle this event */
	result=xfdashboard_focus_manager_handle_key_event(priv->focusManager, inEvent);
	if(result==CLUTTER_EVENT_STOP) return(result);

	/* If even focus manager did not handle this event
	 * send even to searchbox.
	 */
	if(priv->searchbox &&
		XFDASHBOARD_IS_FOCUSABLE(priv->searchbox) &&
		xfdashboard_focus_manager_is_registered(priv->focusManager, XFDASHBOARD_FOCUSABLE(priv->searchbox)))
	{
		result=xfdashboard_focusable_handle_key_event(XFDASHBOARD_FOCUSABLE(priv->searchbox), inEvent);
		if(result==CLUTTER_EVENT_STOP) return(result);
	}

	/* If we get here there was no searchbox or it could not handle the event
	 * so stop further processing.
	 */
	// TODO: g_message("%s: UNHANDLED -> actor=%p[%s], event=%p", __func__, inActor, G_OBJECT_TYPE_NAME(inActor), inEvent);
	return(CLUTTER_EVENT_STOP);
}

/* Set focus in stage */
static void _xfdashboard_stage_set_focus(XfdashboardStage *self)
{
	XfdashboardStagePrivate		*priv;
	ClutterActor				*actor;

	g_return_if_fail(XFDASHBOARD_IS_STAGE(self));

	priv=self->priv;

	/* Set focus if no focus is set */
	actor=clutter_stage_get_key_focus(CLUTTER_STAGE(self));
	if(!XFDASHBOARD_IS_FOCUSABLE(actor) ||
		!xfdashboard_focus_manager_is_registered(priv->focusManager, XFDASHBOARD_FOCUSABLE(actor)))
	{
		XfdashboardFocusable	*focusable;

		/* First try to set focus to searchbox ... */
		if(XFDASHBOARD_IS_FOCUSABLE(priv->searchbox) &&
			xfdashboard_focusable_can_focus(XFDASHBOARD_FOCUSABLE(priv->searchbox)))
		{
			xfdashboard_focus_manager_set_focus(priv->focusManager, XFDASHBOARD_FOCUSABLE(priv->searchbox));
		}
			/* ... then lookup first focusable actor */
			else
			{
				focusable=xfdashboard_focus_manager_get_next_focusable(priv->focusManager, NULL);
				if(focusable) xfdashboard_focus_manager_set_focus(priv->focusManager, focusable);
			}
	}
}

/* The pointer has been moved after a tooltip was shown */
static gboolean _xfdashboard_stage_on_captured_event_after_tooltip(XfdashboardStage *self,
																	ClutterEvent *inEvent,
																	gpointer inUserData)
{
	XfdashboardStagePrivate		*priv;

	g_return_val_if_fail(XFDASHBOARD_IS_STAGE(self), CLUTTER_EVENT_PROPAGATE);

	priv=self->priv;

	/* Disconnect signal */
	g_signal_handlers_disconnect_by_func(self,
											G_CALLBACK(_xfdashboard_stage_on_captured_event_after_tooltip),
											NULL);

	/* Hide tooltip */
	clutter_actor_hide(priv->tooltip);

	return(CLUTTER_EVENT_PROPAGATE);
}

/* Stage got signal to show a tooltip */
static void _xfdashboard_stage_show_tooltip(XfdashboardStage *self, ClutterAction *inAction)
{
	XfdashboardStagePrivate		*priv;
	XfdashboardTooltipAction	*tooltipAction;
	const gchar					*tooltipText;
	gfloat						tooltipX, tooltipY;
	gfloat						tooltipWidth, tooltipHeight;
	guint						cursorSize;
	gfloat						x, y;
	gfloat						stageWidth, stageHeight;

	g_return_if_fail(XFDASHBOARD_IS_STAGE(self));
	g_return_if_fail(XFDASHBOARD_IS_TOOLTIP_ACTION(inAction));
	g_return_if_fail(self->priv->tooltip);

	priv=self->priv;
	tooltipAction=XFDASHBOARD_TOOLTIP_ACTION(inAction);

	/* Disconnect any signal handler for hiding tooltip because
	 * we are going to resetup tooltip. Hide tooltip while setup
	 * to avoid flicker.
	 */
	g_signal_handlers_disconnect_by_func(self,
											G_CALLBACK(_xfdashboard_stage_on_captured_event_after_tooltip),
											NULL);
	clutter_actor_hide(priv->tooltip);

	/* Get tooltip text and update text in tooltip actor */
	tooltipText=xfdashboard_tooltip_action_get_text(tooltipAction);
	xfdashboard_text_box_set_text(XFDASHBOARD_TEXT_BOX(priv->tooltip), tooltipText);

	/* Determine coordinates where to show tooltip at */
	xfdashboard_tooltip_action_get_position(tooltipAction, &tooltipX, &tooltipY);
	clutter_actor_get_size(priv->tooltip, &tooltipWidth, &tooltipHeight);

	cursorSize=gdk_display_get_default_cursor_size(gdk_display_get_default());

	clutter_actor_get_size(CLUTTER_ACTOR(self), &stageWidth, &stageHeight);

	x=tooltipX+cursorSize;
	y=tooltipY+cursorSize;
	if((x+tooltipWidth)>stageWidth) x=tooltipX-tooltipWidth;
	if((y+tooltipHeight)>stageHeight) y=tooltipY-tooltipHeight;

	clutter_actor_set_position(priv->tooltip, floor(x), floor(y));

	/* Show tooltip */
	clutter_actor_show(priv->tooltip);

	/* Connect signal to hide tooltip on next movement of pointer */
	g_signal_connect(self,
						"captured-event",
						G_CALLBACK(_xfdashboard_stage_on_captured_event_after_tooltip),
						NULL);
}

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

/* App-button was toggled */
static void _xfdashboard_stage_on_quicklaunch_apps_button_toggled(XfdashboardStage *self, gpointer inUserData)
{
	XfdashboardStagePrivate		*priv;
	XfdashboardToggleButton		*appsButton;
	gboolean					state;
	XfdashboardView				*view;

	g_return_if_fail(XFDASHBOARD_IS_STAGE(self));
	g_return_if_fail(XFDASHBOARD_IS_TOGGLE_BUTTON(inUserData));

	priv=self->priv;
	appsButton=XFDASHBOARD_TOGGLE_BUTTON(inUserData);

	/* Get state of apps button */
	state=xfdashboard_toggle_button_get_toggle_state(appsButton);

	/* Depending on state activate views */
	if(state==FALSE)
	{
		/* Find "windows-view" view and activate */
		view=xfdashboard_viewpad_find_view_by_type(XFDASHBOARD_VIEWPAD(priv->viewpad), XFDASHBOARD_TYPE_WINDOWS_VIEW);
		if(view) xfdashboard_viewpad_set_active_view(XFDASHBOARD_VIEWPAD(priv->viewpad), view);
	}
		else
		{
			/* Find "applications" or "search" view and activate */
			if(!priv->searchActive) view=xfdashboard_viewpad_find_view_by_type(XFDASHBOARD_VIEWPAD(priv->viewpad), XFDASHBOARD_TYPE_APPLICATIONS_VIEW);
				else view=xfdashboard_viewpad_find_view_by_type(XFDASHBOARD_VIEWPAD(priv->viewpad), XFDASHBOARD_TYPE_SEARCH_VIEW);
			if(view) xfdashboard_viewpad_set_active_view(XFDASHBOARD_VIEWPAD(priv->viewpad), view);
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
		xfdashboard_stylable_add_class(XFDASHBOARD_STYLABLE(priv->searchbox), "search-active");

		/* Change apps button appearance */
		if(appsButton) xfdashboard_stylable_add_class(XFDASHBOARD_STYLABLE(appsButton), "search-active");

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
		xfdashboard_stylable_remove_class(XFDASHBOARD_STYLABLE(priv->searchbox), "search-active");

		/* Disable search view */
		xfdashboard_view_set_enabled(searchView, FALSE);

		/* Change apps button appearance */
		if(appsButton) xfdashboard_stylable_remove_class(XFDASHBOARD_STYLABLE(appsButton), "search-active");

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

/* A window was created
 * Check for stage window and set up window properties
 */
static void _xfdashboard_stage_on_window_opened(XfdashboardStage *self,
													XfdashboardWindowTrackerWindow *inWindow,
													gpointer inUserData)
{
	XfdashboardStagePrivate				*priv=self->priv;
	XfdashboardWindowTrackerWindow		*stageWindow;
#if !defined(CHECK_CLUTTER_VERSION) || !CLUTTER_CHECK_VERSION(1, 16, 0)
	GdkScreen							*screen;
	gint								primaryMonitor;
	GdkRectangle						geometry;
#endif

	g_return_if_fail(XFDASHBOARD_IS_STAGE(self));
	g_return_if_fail(XFDASHBOARD_IS_WINDOW_TRACKER_WINDOW(inWindow));

	priv=self->priv;

	/* Check if window opened is this stage window */
	stageWindow=xfdashboard_window_tracker_window_get_stage_window(CLUTTER_STAGE(self));
	if(stageWindow!=inWindow) return;

	/* Set up window for use as stage window */
	priv->stageWindow=inWindow;
	xfdashboard_window_tracker_window_make_stage_window(priv->stageWindow);

#if !defined(CHECK_CLUTTER_VERSION) || !CLUTTER_CHECK_VERSION(1, 16, 0)
	/* TODO: As long as we do not support multi-monitors
	 *       use this hack to ensure stage is in right size
	 */
	screen=gdk_screen_get_default();
	primaryMonitor=gdk_screen_get_primary_monitor(screen);
	gdk_screen_get_monitor_geometry(screen, primaryMonitor, &geometry);
	clutter_actor_set_size(CLUTTER_ACTOR(self), geometry.width, geometry.height);
#endif

	/* Disconnect signal handler as this is a one-time setup of stage window */
	g_debug("Stage window was opened and set up. Removing signal handler.");
	g_signal_handlers_disconnect_by_func(priv->windowTracker, G_CALLBACK(_xfdashboard_stage_on_window_opened), self);

	/* Set focus */
	_xfdashboard_stage_set_focus(self);
}

/* The application will be suspended */
static void _xfdashboard_stage_on_application_suspend(XfdashboardStage *self, gpointer inUserData)
{
	XfdashboardStagePrivate				*priv=self->priv;

	g_return_if_fail(XFDASHBOARD_IS_STAGE(self));
	g_return_if_fail(XFDASHBOARD_IS_APPLICATION(inUserData));

	priv=self->priv;

	/* Instead of hiding stage actor just hide stage's window. It should be safe
	 * to just hide the window as it should be listed on any task list and is not
	 * selectable by user. The advantage should be that the window is already mapped
	 * and its state is already set up like fullscreen, sticky and so on. This
	 * prevents that window will not be shown in fullscreen again (just maximized
	 * and flickers) if we use clutter_actor_show to show stage actor and its window
	 * again. But we can only do this if the window is known and set up ;)
	 */
	if(priv->stageWindow)
	{
		xfdashboard_window_tracker_window_unmake_stage_window(priv->stageWindow);
		xfdashboard_window_tracker_window_hide(priv->stageWindow);
	}

	/* Hide tooltip and clean up */
	if(priv->tooltip) clutter_actor_hide(priv->tooltip);
	g_signal_handlers_disconnect_by_func(self,
											G_CALLBACK(_xfdashboard_stage_on_captured_event_after_tooltip),
											NULL);
}

/* The application will be resumed */
static void _xfdashboard_stage_on_application_resume(XfdashboardStage *self, gpointer inUserData)
{
	XfdashboardStagePrivate				*priv=self->priv;
	gboolean							doResetSearch;

	g_return_if_fail(XFDASHBOARD_IS_STAGE(self));
	g_return_if_fail(XFDASHBOARD_IS_APPLICATION(inUserData));

	priv=self->priv;

	/* If stage window is known just show it again ... */
	if(priv->stageWindow)
	{
		/* If search is active then end search by clearing search box */
		doResetSearch=xfconf_channel_get_bool(xfdashboard_application_get_xfconf_channel(),
												RESET_SEARCH_ON_RESUME_XFCONF_PROP,
												DEFAULT_RESET_SEARCH_ON_RESUME);

		if(priv->searchbox &&
			doResetSearch &&
			!xfdashboard_text_box_is_empty(XFDASHBOARD_TEXT_BOX(priv->searchbox)))
		{
			XfdashboardView				*searchView;

			/* Reset search in search view */
			searchView=xfdashboard_viewpad_find_view_by_type(XFDASHBOARD_VIEWPAD(priv->viewpad), XFDASHBOARD_TYPE_SEARCH_VIEW);
			if(searchView) xfdashboard_search_view_reset_search(XFDASHBOARD_SEARCH_VIEW(searchView));
				else g_critical(_("Cannot reset search because search view was not found in viewpad."));

			/* Reset text in search box */
			xfdashboard_text_box_set_text(XFDASHBOARD_TEXT_BOX(priv->searchbox), NULL);
		}

		/* Set up stage and show it */
		xfdashboard_window_tracker_window_make_stage_window(priv->stageWindow);
		xfdashboard_window_tracker_window_show(priv->stageWindow);
	}
		/* ... otherwise set it up by calling clutter_actor_show() etc. */
		else
		{
			/* Show stage and force window creation */
			clutter_actor_show(CLUTTER_ACTOR(self));
		}
}

/* IMPLEMENTATION: ClutterActor */

/* The stage actor should be shown */
static void _xfdashboard_stage_show(ClutterActor *inActor)
{
	XfdashboardStage			*self;
	XfdashboardStagePrivate		*priv;

	g_return_if_fail(XFDASHBOARD_IS_STAGE(inActor));

	self=XFDASHBOARD_STAGE(inActor);
	priv=self->priv;

	/* Set stage to fullscreen just in case it forgot about it */
	clutter_stage_set_fullscreen(CLUTTER_STAGE(self), TRUE);

	/* If we do not know the stage window connect signal to find it */
	if(!priv->stageWindow)
	{
		/* Connect signals */
		g_signal_connect_swapped(priv->windowTracker, "window-opened", G_CALLBACK(_xfdashboard_stage_on_window_opened), self);
	}

	/* Call parent's show method */
	if(CLUTTER_ACTOR_CLASS(xfdashboard_stage_parent_class)->show)
	{
		CLUTTER_ACTOR_CLASS(xfdashboard_stage_parent_class)->show(inActor);
	}
}

/* IMPLEMENTATION: Interface XfdashboardLayoutable */

/* Virtual function "layout_completed" was called */
static void _xfdashboard_stage_layoutable_layout_completed(XfdashboardLayoutable *inLayoutable)
{
	XfdashboardStage			*self;
	XfdashboardStagePrivate		*priv;
	ClutterActor				*actor;

	g_return_if_fail(XFDASHBOARD_IS_LAYOUTABLE(inLayoutable));
	g_return_if_fail(XFDASHBOARD_IS_STAGE(inLayoutable));

	self=XFDASHBOARD_STAGE(inLayoutable);
	priv=self->priv;

	/* Get children from built stage and connect signals */
	actor=xfdashboard_find_actor_by_name(CLUTTER_ACTOR(self), "view-selector");
	if(actor && XFDASHBOARD_IS_VIEW_SELECTOR(actor))
	{
		priv->viewSelector=actor;

		/* Register this focusable actor if it is focusable */
		if(XFDASHBOARD_IS_FOCUSABLE(priv->viewSelector))
		{
			xfdashboard_focus_manager_register(priv->focusManager,
												XFDASHBOARD_FOCUSABLE(priv->viewSelector));
		}
	}

	actor=xfdashboard_find_actor_by_name(CLUTTER_ACTOR(self), "searchbox");
	if(actor && XFDASHBOARD_IS_TEXT_BOX(actor))
	{
		priv->searchbox=actor;

		/* Connect signals */
		g_signal_connect_swapped(priv->searchbox,
									"text-changed",
									G_CALLBACK(_xfdashboard_stage_on_searchbox_text_changed),
									self);
		g_signal_connect_swapped(priv->searchbox,
									"secondary-icon-clicked",
									G_CALLBACK(_xfdashboard_stage_on_searchbox_secondary_icon_clicked),
									self);

		/* Register this focusable actor if it is focusable */
		if(XFDASHBOARD_IS_FOCUSABLE(priv->searchbox))
		{
			xfdashboard_focus_manager_register(priv->focusManager,
												XFDASHBOARD_FOCUSABLE(priv->searchbox));

			/* Set stage focus to searchbox to get cursor shown */
			clutter_stage_set_key_focus(CLUTTER_STAGE(self), CLUTTER_ACTOR(priv->searchbox));
		}
	}

	actor=xfdashboard_find_actor_by_name(CLUTTER_ACTOR(self), "viewpad");
	if(actor && XFDASHBOARD_IS_VIEWPAD(actor))
	{
		priv->viewpad=actor;

		/* Connect signals */
		g_signal_connect_swapped(priv->viewpad, "view-activated", G_CALLBACK(_xfdashboard_stage_on_view_activated), self);

		/* Register this focusable actor if it is focusable */
		if(XFDASHBOARD_IS_FOCUSABLE(priv->viewpad))
		{
			xfdashboard_focus_manager_register(priv->focusManager,
												XFDASHBOARD_FOCUSABLE(priv->viewpad));
		}
	}

	actor=xfdashboard_find_actor_by_name(CLUTTER_ACTOR(self), "quicklaunch");
	if(actor && XFDASHBOARD_IS_QUICKLAUNCH(actor))
	{
		XfdashboardToggleButton		*appsButton;

		priv->quicklaunch=actor;

		/* Connect signals */
		appsButton=xfdashboard_quicklaunch_get_apps_button(XFDASHBOARD_QUICKLAUNCH(priv->quicklaunch));
		if(appsButton)
		{
			g_signal_connect_swapped(appsButton,
										"toggled",
										G_CALLBACK(_xfdashboard_stage_on_quicklaunch_apps_button_toggled),
										self);
		}

		/* Register this focusable actor if it is focusable */
		if(XFDASHBOARD_IS_FOCUSABLE(priv->quicklaunch))
		{
			xfdashboard_focus_manager_register(priv->focusManager,
												XFDASHBOARD_FOCUSABLE(priv->quicklaunch));
		}
	}

	actor=xfdashboard_find_actor_by_name(CLUTTER_ACTOR(self), "workspace-selector");
	if(actor && XFDASHBOARD_IS_WORKSPACE_SELECTOR(actor))
	{
		priv->workspaces=actor;

		/* Register this focusable actor if it is focusable */
		if(XFDASHBOARD_IS_FOCUSABLE(priv->workspaces))
		{
			xfdashboard_focus_manager_register(priv->focusManager,
												XFDASHBOARD_FOCUSABLE(priv->workspaces));
		}
	}

	actor=xfdashboard_find_actor_by_name(CLUTTER_ACTOR(self), "notification");
	if(actor && XFDASHBOARD_IS_TEXT_BOX(actor))
	{
		priv->notification=actor;

		/* Register this focusable actor if it is focusable */
		if(XFDASHBOARD_IS_FOCUSABLE(priv->notification))
		{
			xfdashboard_focus_manager_register(priv->focusManager,
												XFDASHBOARD_FOCUSABLE(priv->notification));
		}

		/* Hide notification by default */
		clutter_actor_hide(priv->notification);
		clutter_actor_set_reactive(priv->notification, FALSE);
	}

	actor=xfdashboard_find_actor_by_name(CLUTTER_ACTOR(self), "tooltip");
	if(actor && XFDASHBOARD_IS_TEXT_BOX(actor))
	{
		priv->tooltip=actor;

		/* Register this focusable actor if it is focusable */
		if(XFDASHBOARD_IS_FOCUSABLE(priv->tooltip))
		{
			xfdashboard_focus_manager_register(priv->focusManager,
												XFDASHBOARD_FOCUSABLE(priv->tooltip));
		}

		/* Hide tooltip by default */
		clutter_actor_hide(priv->tooltip);
		clutter_actor_set_reactive(priv->tooltip, FALSE);
	}

	/* Set focus */
	_xfdashboard_stage_set_focus(self);
}

/* Interface initialization
 * Set up default functions
 */
void _xfdashboard_stage_layoutable_iface_init(XfdashboardLayoutableInterface *iface)
{
	iface->layout_completed=_xfdashboard_stage_layoutable_layout_completed;
}

/* IMPLEMENTATION: Interface XfdashboardStylable */

/* Get stylable properties of stage */
static void _xfdashboard_stage_stylable_get_stylable_properties(XfdashboardStylable *self,
																GHashTable *ioStylableProperties)
{
	g_return_if_fail(XFDASHBOARD_IS_STYLABLE(self));

	/* Add stylable properties to hashtable */
	xfdashboard_stylable_add_stylable_property(self, ioStylableProperties, "background-color");
}

/* Get/set style classes of stage */
static const gchar* _xfdashboard_stage_stylable_get_classes(XfdashboardStylable *inStylable)
{
	/* Not implemented */
	return(NULL);
}

static void _xfdashboard_stage_stylable_set_classes(XfdashboardStylable *inStylable, const gchar *inStyleClasses)
{
	/* Not implemented */
}

/* Get/set style pseudo-classes of stage */
static const gchar* _xfdashboard_stage_stylable_get_pseudo_classes(XfdashboardStylable *inStylable)
{
	/* Not implemented */
	return(NULL);
}

static void _xfdashboard_stage_stylable_set_pseudo_classes(XfdashboardStylable *inStylable, const gchar *inStylePseudoClasses)
{
	/* Not implemented */
}

/* Interface initialization
 * Set up default functions
 */
void _xfdashboard_stage_stylable_iface_init(XfdashboardStylableInterface *iface)
{
	iface->get_stylable_properties=_xfdashboard_stage_stylable_get_stylable_properties;
	iface->get_classes=_xfdashboard_stage_stylable_get_classes;
	iface->set_classes=_xfdashboard_stage_stylable_set_classes;
	iface->get_pseudo_classes=_xfdashboard_stage_stylable_get_pseudo_classes;
	iface->set_pseudo_classes=_xfdashboard_stage_stylable_set_pseudo_classes;
}

/* IMPLEMENTATION: GObject */

/* Dispose this object */
static void _xfdashboard_stage_dispose(GObject *inObject)
{
	XfdashboardStage			*self=XFDASHBOARD_STAGE(inObject);
	XfdashboardStagePrivate		*priv=self->priv;

	/* Release allocated resources */
	if(priv->stageWindow)
	{
		xfdashboard_window_tracker_window_unmake_stage_window(priv->stageWindow);
		priv->stageWindow=NULL;
	}

	if(priv->focusManager)
	{
		g_object_unref(priv->focusManager);
		priv->focusManager=NULL;
	}

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

	if(priv->styleClasses)
	{
		g_free(priv->styleClasses);
		priv->styleClasses=NULL;
	}

	if(priv->stylePseudoClasses)
	{
		g_free(priv->stylePseudoClasses);
		priv->stylePseudoClasses=NULL;
	}

	if(priv->notification)
	{
		clutter_actor_destroy(CLUTTER_ACTOR(priv->notification));
		priv->notification=NULL;
	}

	if(priv->tooltip)
	{
		clutter_actor_destroy(CLUTTER_ACTOR(priv->tooltip));
		priv->tooltip=NULL;
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

/* Set/get properties */
static void _xfdashboard_stage_set_property(GObject *inObject,
											guint inPropID,
											const GValue *inValue,
											GParamSpec *inSpec)
{
	XfdashboardStage			*self=XFDASHBOARD_STAGE(inObject);

	switch(inPropID)
	{
		case PROP_STYLE_CLASSES:
			_xfdashboard_stage_stylable_set_classes(XFDASHBOARD_STYLABLE(self),
													g_value_get_string(inValue));
			break;

		case PROP_STYLE_PSEUDO_CLASSES:
			_xfdashboard_stage_stylable_set_pseudo_classes(XFDASHBOARD_STYLABLE(self),
															g_value_get_string(inValue));
			break;

		default:
			G_OBJECT_WARN_INVALID_PROPERTY_ID(inObject, inPropID, inSpec);
			break;
	}
}

static void _xfdashboard_stage_get_property(GObject *inObject,
											guint inPropID,
											GValue *outValue,
											GParamSpec *inSpec)
{
	XfdashboardStage			*self=XFDASHBOARD_STAGE(inObject);
	XfdashboardStagePrivate		*priv=self->priv;

	switch(inPropID)
	{
		case PROP_STYLE_CLASSES:
			g_value_set_string(outValue, priv->styleClasses);
			break;

		case PROP_STYLE_PSEUDO_CLASSES:
			g_value_set_string(outValue, priv->stylePseudoClasses);
			break;

		default:
			G_OBJECT_WARN_INVALID_PROPERTY_ID(inObject, inPropID, inSpec);
			break;
	}
}

/* Class initialization
 * Override functions in parent classes and define properties
 * and signals
 */
static void xfdashboard_stage_class_init(XfdashboardStageClass *klass)
{
	ClutterActorClass	*actorClass=CLUTTER_ACTOR_CLASS(klass);
	GObjectClass		*gobjectClass=G_OBJECT_CLASS(klass);

	/* Override functions */
	klass->show_tooltip=_xfdashboard_stage_show_tooltip;

	actorClass->show=_xfdashboard_stage_show;
	actorClass->event=xfdashboard_stage_event;

	gobjectClass->dispose=_xfdashboard_stage_dispose;
	gobjectClass->set_property=_xfdashboard_stage_set_property;
	gobjectClass->get_property=_xfdashboard_stage_get_property;

	/* Set up private structure */
	g_type_class_add_private(klass, sizeof(XfdashboardStagePrivate));

	/* Define properties */
	g_object_class_override_property(gobjectClass, PROP_STYLE_CLASSES, "style-classes");
	g_object_class_override_property(gobjectClass, PROP_STYLE_PSEUDO_CLASSES, "style-pseudo-classes");

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

	XfdashboardStageSignals[SIGNAL_SHOW_TOOLTIP]=
		g_signal_new("show-tooltip",
						G_TYPE_FROM_CLASS(klass),
						G_SIGNAL_RUN_LAST | G_SIGNAL_ACTION,
						G_STRUCT_OFFSET(XfdashboardStageClass, show_tooltip),
						NULL,
						NULL,
						g_cclosure_marshal_VOID__OBJECT,
						G_TYPE_NONE,
						1,
						CLUTTER_TYPE_ACTION);
}

/* Object initialization
 * Create private structure and set up default values
 */
static void xfdashboard_stage_init(XfdashboardStage *self)
{
	XfdashboardStagePrivate		*priv;
	XfdashboardApplication		*application;
#if defined(CLUTTER_CHECK_VERSION) && CLUTTER_CHECK_VERSION(1, 16, 0)
	GdkScreen					*screen;
	gint						primaryMonitor;
	GdkRectangle				geometry;
#endif

	priv=self->priv=XFDASHBOARD_STAGE_GET_PRIVATE(self);

	/* Set default values */
	priv->focusManager=xfdashboard_focus_manager_get_default();
	priv->windowTracker=xfdashboard_window_tracker_get_default();
	priv->stageWindow=NULL;
	priv->styleClasses=NULL;
	priv->stylePseudoClasses=NULL;

	priv->quicklaunch=NULL;
	priv->searchbox=NULL;
	priv->workspaces=NULL;
	priv->viewpad=NULL;
	priv->viewSelector=NULL;
	priv->notification=NULL;
	priv->tooltip=NULL;
	priv->lastSearchTextLength=0;
	priv->viewBeforeSearch=NULL;
	priv->searchActive=FALSE;
	priv->notificationTimeoutID=0;

	/* Set up stage and style it */
	clutter_stage_set_use_alpha(CLUTTER_STAGE(self), TRUE);
	clutter_stage_set_user_resizable(CLUTTER_STAGE(self), FALSE);
	clutter_stage_set_fullscreen(CLUTTER_STAGE(self), TRUE);
	xfdashboard_stylable_invalidate(XFDASHBOARD_STYLABLE(self));

#if defined(CLUTTER_CHECK_VERSION) && CLUTTER_CHECK_VERSION(1, 16, 0)
	/* TODO: As long as we do not support multi-monitors
	 *       use this hack to ensure stage is in right size
	 */
	screen=gdk_screen_get_default();
	primaryMonitor=gdk_screen_get_primary_monitor(screen);
	gdk_screen_get_monitor_geometry(screen, primaryMonitor, &geometry);
	clutter_actor_set_size(CLUTTER_ACTOR(self), geometry.width, geometry.height);
#endif

	/* Connect signal to application */
	application=xfdashboard_application_get_default();
	g_signal_connect_swapped(application, "suspend", G_CALLBACK(_xfdashboard_stage_on_application_suspend), self);
	g_signal_connect_swapped(application, "resume", G_CALLBACK(_xfdashboard_stage_on_application_resume), self);
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
