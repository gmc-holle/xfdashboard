/*
 * stage: Global stage of application
 * 
 * Copyright 2012-2015 Stephan Haller <nomad@froevel.de>
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
#include "stylable.h"
#include "utils.h"
#include "focus-manager.h"
#include "enums.h"
#include "window-tracker.h"
#include "window-content.h"
#include "stage-interface.h"

/* Define this class in GObject system */
G_DEFINE_TYPE(XfdashboardStage,
				xfdashboard_stage,
				CLUTTER_TYPE_STAGE)

/* Private structure - access only by public API if needed */
#define XFDASHBOARD_STAGE_GET_PRIVATE(obj) \
	(G_TYPE_INSTANCE_GET_PRIVATE((obj), XFDASHBOARD_TYPE_STAGE, XfdashboardStagePrivate))

struct _XfdashboardStagePrivate
{
	/* Properties related */
	XfdashboardStageBackgroundImageType		backgroundType;

	ClutterColor							*backgroundColor;

	/* Actors */
	ClutterActor							*backgroundImageLayer;
	ClutterActor							*backgroundColorLayer;

	ClutterActor							*primaryInterface;

	ClutterActor							*quicklaunch;
	ClutterActor							*searchbox;
	ClutterActor							*workspaces;
	ClutterActor							*viewpad;
	ClutterActor							*viewSelector;
	ClutterActor							*notification;
	ClutterActor							*tooltip;

	/* Instance related */
	XfdashboardWindowTracker				*windowTracker;
	XfdashboardWindowTrackerWindow			*stageWindow;

	gboolean								searchActive;
	gint									lastSearchTextLength;
	XfdashboardView							*viewBeforeSearch;

	guint									notificationTimeoutID;

	XfdashboardFocusManager					*focusManager;
};

/* Properties */
enum
{
	PROP_0,

	PROP_BACKGROUND_IMAGE_TYPE,
	PROP_BACKGROUND_COLOR,

	PROP_LAST
};

static GParamSpec* XfdashboardStageProperties[PROP_LAST]={ 0, };

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
#define SWITCH_VIEW_ON_RESUME_XFCONF_PROP		"/switch-to-view-on-resume"
#define DEFAULT_SWITCH_VIEW_ON_RESUME			NULL
#define XFDASHBOARD_THEME_LAYOUT_PRIMARY		"primary"
#define XFDASHBOARD_THEME_LAYOUT_SECONDARY		"secondary"

/* Handle an event */
static gboolean _xfdashboard_stage_event(ClutterActor *inActor, ClutterEvent *inEvent)
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
		/* Handle key */
		switch(inEvent->key.keyval)
		{
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
	result=xfdashboard_focus_manager_handle_key_event(priv->focusManager, inEvent, NULL);
	if(result==CLUTTER_EVENT_STOP) return(result);

	/* If even focus manager did not handle this event send this event to searchbox */
	if(priv->searchbox &&
		XFDASHBOARD_IS_FOCUSABLE(priv->searchbox) &&
		xfdashboard_focus_manager_is_registered(priv->focusManager, XFDASHBOARD_FOCUSABLE(priv->searchbox)))
	{
		/* Ask search to handle this event if it has not the focus currently
		 * because in this case it has already handled the event and we do
		 * not to do this twice.
		 */
		if(xfdashboard_focus_manager_get_focus(priv->focusManager)!=XFDASHBOARD_FOCUSABLE(priv->searchbox))
		{
			result=xfdashboard_focus_manager_handle_key_event(priv->focusManager, inEvent, XFDASHBOARD_FOCUSABLE(priv->searchbox));
			if(result==CLUTTER_EVENT_STOP) return(result);
		}
	}

	/* If we get here there was no searchbox or it could not handle the event
	 * so stop further processing.
	 */
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

		/* Enable search view and set focus to viewpad which will show the
		 * search view so this search view will get the focus finally
		 */
		xfdashboard_view_set_enabled(searchView, TRUE);
		if(priv->viewpad && priv->focusManager)
		{
			xfdashboard_focus_manager_set_focus(priv->focusManager, XFDASHBOARD_FOCUSABLE(priv->viewpad));
		}

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
	XfdashboardStagePrivate				*priv;
	XfdashboardWindowTrackerWindow		*stageWindow;

	g_return_if_fail(XFDASHBOARD_IS_STAGE(self));
	g_return_if_fail(XFDASHBOARD_IS_WINDOW_TRACKER_WINDOW(inWindow));

	priv=self->priv;

	/* Check if window opened is this stage window */
	stageWindow=xfdashboard_window_tracker_window_get_stage_window(CLUTTER_STAGE(self));
	if(stageWindow!=inWindow) return;

	/* Set up window for use as stage window */
	priv->stageWindow=inWindow;
	xfdashboard_window_tracker_window_make_stage_window(priv->stageWindow);

	/* Disconnect signal handler as this is a one-time setup of stage window */
	g_debug("Stage window was opened and set up. Removing signal handler.");
	g_signal_handlers_disconnect_by_func(priv->windowTracker, G_CALLBACK(_xfdashboard_stage_on_window_opened), self);

	/* Set focus */
	_xfdashboard_stage_set_focus(self);
}

/* A window was created
 * Check if window opened is desktop background window
 */
static void _xfdashboard_stage_on_desktop_window_opened(XfdashboardStage *self,
														XfdashboardWindowTrackerWindow *inWindow,
														gpointer inUserData)
{
	XfdashboardStagePrivate				*priv;
	XfdashboardWindowTrackerWindow		*desktopWindow;
	ClutterContent						*windowContent;

	g_return_if_fail(XFDASHBOARD_IS_STAGE(self));
	g_return_if_fail(XFDASHBOARD_IS_WINDOW_TRACKER_WINDOW(inWindow));

	priv=self->priv;

	/* Get desktop background window and check if it is the new window opened */
	desktopWindow=xfdashboard_window_tracker_get_root_window(priv->windowTracker);
	if(desktopWindow)
	{
		windowContent=xfdashboard_window_content_new_for_window(desktopWindow);
		clutter_actor_set_content(priv->backgroundImageLayer, windowContent);
		clutter_actor_show(priv->backgroundImageLayer);
		g_object_unref(windowContent);

		g_signal_handlers_disconnect_by_func(priv->windowTracker, G_CALLBACK(_xfdashboard_stage_on_desktop_window_opened), self);
		g_debug("Found desktop window with signal 'window-opened', so disconnecting signal handler");
	}
}

/* The application will be suspended */
static void _xfdashboard_stage_on_application_suspend(XfdashboardStage *self, gpointer inUserData)
{
	XfdashboardStagePrivate				*priv;

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
	XfdashboardStagePrivate				*priv;

	g_return_if_fail(XFDASHBOARD_IS_STAGE(self));
	g_return_if_fail(XFDASHBOARD_IS_APPLICATION(inUserData));

	priv=self->priv;

	/* If stage window is known just show it again ... */
	if(priv->stageWindow)
	{
		gchar							*resumeViewInternalName;
		gboolean						doResetSearch;
		XfdashboardView					*searchView;
		XfdashboardView					*resumeView;

		/* Get configured options */
		doResetSearch=xfconf_channel_get_bool(xfdashboard_application_get_xfconf_channel(),
												RESET_SEARCH_ON_RESUME_XFCONF_PROP,
												DEFAULT_RESET_SEARCH_ON_RESUME);
		resumeViewInternalName=xfconf_channel_get_string(xfdashboard_application_get_xfconf_channel(),
															SWITCH_VIEW_ON_RESUME_XFCONF_PROP,
															DEFAULT_SWITCH_VIEW_ON_RESUME);

		/* Find search view */
		searchView=xfdashboard_viewpad_find_view_by_type(XFDASHBOARD_VIEWPAD(priv->viewpad), XFDASHBOARD_TYPE_SEARCH_VIEW);
		if(!searchView) g_critical(_("Cannot find search viewin viewpad to reset view."));

		/* Find view to switch to if requested */
		resumeView=NULL;
		if(resumeViewInternalName)
		{
			/* Lookup view by its internal name */
			resumeView=xfdashboard_viewpad_find_view_by_name(XFDASHBOARD_VIEWPAD(priv->viewpad), resumeViewInternalName);
			if(!resumeView) g_warning(_("Cannot switch to unknown view '%s'"), resumeViewInternalName);

			/* If view to switch to is the search view behave like we did not find the view
			 * because it does not make sense to switch to a view which might be hidden,
			 * e.g. when resetting search on resume which causes the search view to be hidden
			 * and the previous view to be shown.
			 */
			if(resumeView &&
				searchView &&
				resumeView==searchView)
			{
				resumeView=NULL;
			}

			/* Release allocated resources */
			g_free(resumeViewInternalName);
		}

		/* If search is active then end search by clearing search box if requested ... */
		if(priv->searchbox &&
			doResetSearch &&
			!xfdashboard_text_box_is_empty(XFDASHBOARD_TEXT_BOX(priv->searchbox)))
		{
			/* If user wants to switch to a specific view set it as "previous" view now.
			 * It will be restored automatically when search box is cleared.
			 */
			if(resumeView)
			{
				/* Release old remembered view */
				if(priv->viewBeforeSearch) g_object_unref(priv->viewBeforeSearch);

				/* Remember new active view */
				priv->viewBeforeSearch=XFDASHBOARD_VIEW(g_object_ref(resumeView));
			}

			/* Reset search in search view */
			if(searchView) xfdashboard_search_view_reset_search(XFDASHBOARD_SEARCH_VIEW(searchView));

			/* Reset text in search box */
			xfdashboard_text_box_set_text(XFDASHBOARD_TEXT_BOX(priv->searchbox), NULL);
		}
			/* ... otherwise just switch to view if requested */
			else if(resumeView)
			{
				xfdashboard_viewpad_set_active_view(XFDASHBOARD_VIEWPAD(priv->viewpad), resumeView);
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

	/* In any case force a redraw */
	clutter_actor_queue_redraw(CLUTTER_ACTOR(self));
}

/* Callback function to to set reference to NULL in stage object
 * when an actor of special interest like searchbox, notification etc.
 * is going to be destroyed.
 */
static void _xfdashboard_stage_reset_reference_on_destroy(ClutterActor *inActor,
															gpointer inUserData)
{
	ClutterActor			**actorReference;

	g_return_if_fail(CLUTTER_IS_ACTOR(inActor));
	g_return_if_fail(inUserData);

	actorReference=(ClutterActor**)inUserData;

	/* User data points to address in stage object which holds a pointer to address
	 * of the actor which is going to be destroyed. So we should check that the value
	 * at the address in user data contains the address of the actor which is going
	 * to be destroyed. If it is reset value at address in user data to NULL otherwise
	 * leave value untouched but show critical warning because this function should
	 * never be called in this case.
	 */
	if(*actorReference!=inActor)
	{
		g_critical("Will not reset reference to actor of type %s at address %p because address in stage object points to %p and does not match actor's address.",
					G_OBJECT_TYPE_NAME(inActor),
					inActor,
					*actorReference);
	}
		else *actorReference=NULL;
}

/* Theme in application has changed */
static void _xfdashboard_stage_on_application_theme_changed(XfdashboardStage *self,
															XfdashboardTheme *inTheme,
															gpointer inUserData)
{
	XfdashboardStagePrivate				*priv;
	XfdashboardThemeLayout				*themeLayout;
	GList								*interfaces;
	ClutterActor						*interface;
	GList								*iter;
	GList								*monitors;
	XfdashboardWindowTrackerMonitor		*monitor;
	ClutterActorIter					childIter;
	ClutterActor						*child;

	g_return_if_fail(XFDASHBOARD_IS_STAGE(self));
	g_return_if_fail(XFDASHBOARD_IS_THEME(inTheme));
	g_return_if_fail(XFDASHBOARD_IS_APPLICATION(inUserData));

	priv=self->priv;

	/* Get theme layout */
	themeLayout=xfdashboard_theme_get_layout(inTheme);

	/* Create interface for each monitor if multiple monitors are supported */
	interfaces=NULL;
	if(xfdashboard_window_tracker_supports_multiple_monitors(priv->windowTracker))
	{
		monitors=xfdashboard_window_tracker_get_monitors(priv->windowTracker);
		for(iter=monitors; iter; iter=g_list_next(iter))
		{
			/* Get monitor */
			monitor=XFDASHBOARD_WINDOW_TRACKER_MONITOR(iter->data);

			/* Get interface  */
			if(xfdashboard_window_tracker_monitor_is_primary(monitor))
			{
				/* Get interface for primary monitor */
				interface=xfdashboard_theme_layout_build_interface(themeLayout, XFDASHBOARD_THEME_LAYOUT_PRIMARY);
				if(!interface)
				{
					g_critical(_("Could not build interface '%s' from theme '%s'"),
								XFDASHBOARD_THEME_LAYOUT_PRIMARY,
								xfdashboard_theme_get_theme_name(inTheme));

					/* Release allocated resources */
					g_list_foreach(interfaces, (GFunc)g_object_unref, NULL);
					g_list_free(interfaces);

					return;
				}

				if(!XFDASHBOARD_IS_STAGE_INTERFACE(interface))
				{
					g_critical(_("Interface '%s' from theme '%s' must be an actor of type %s"),
								XFDASHBOARD_THEME_LAYOUT_PRIMARY,
								xfdashboard_theme_get_theme_name(inTheme),
								g_type_name(XFDASHBOARD_TYPE_STAGE_INTERFACE));

					/* Release allocated resources */
					g_list_foreach(interfaces, (GFunc)g_object_unref, NULL);
					g_list_free(interfaces);

					return;
				}
			}
				else
				{
					/* Get interface for non-primary monitors. If no interface
					 * is defined in theme then create an empty interface.
					 */
					interface=xfdashboard_theme_layout_build_interface(themeLayout, XFDASHBOARD_THEME_LAYOUT_SECONDARY);
					if(!interface)
					{
						interface=xfdashboard_stage_interface_new();
					}

					if(!XFDASHBOARD_IS_STAGE_INTERFACE(interface))
					{
						g_critical(_("Interface '%s' from theme '%s' must be an actor of type %s"),
									XFDASHBOARD_THEME_LAYOUT_SECONDARY,
									xfdashboard_theme_get_theme_name(inTheme),
									g_type_name(XFDASHBOARD_TYPE_STAGE_INTERFACE));

						/* Release allocated resources */
						g_list_foreach(interfaces, (GFunc)g_object_unref, NULL);
						g_list_free(interfaces);

						return;
					}
				}

			/* Set monitor at interface */
			xfdashboard_stage_interface_set_monitor(XFDASHBOARD_STAGE_INTERFACE(interface), monitor);

			/* Add interface to list of interfaces */
			interfaces=g_list_prepend(interfaces, interface);
		}
	}
		/* Otherwise create only a primary stage interface and set no monitor
		 * because no one is available if multiple monitors are not supported.
		 */
		else
		{
			/* Get interface for primary monitor */
			interface=xfdashboard_theme_layout_build_interface(themeLayout, XFDASHBOARD_THEME_LAYOUT_PRIMARY);
			if(!interface)
			{
				g_critical(_("Could not build interface '%s' from theme '%s'"),
							XFDASHBOARD_THEME_LAYOUT_PRIMARY,
							xfdashboard_theme_get_theme_name(inTheme));
				return;
			}

			if(!XFDASHBOARD_IS_STAGE_INTERFACE(interface))
			{
				g_critical(_("Interface '%s' from theme '%s' must be an actor of type %s"),
							XFDASHBOARD_THEME_LAYOUT_PRIMARY,
							xfdashboard_theme_get_theme_name(inTheme),
							g_type_name(XFDASHBOARD_TYPE_STAGE_INTERFACE));
				return;
			}

			/* Add interface to list of interfaces */
			interfaces=g_list_prepend(interfaces, interface);

			g_debug("Creating primary interface only because of no support for multiple monitors");
		}

	/* Destroy all interfaces from stage.
	 * There is no need to reset pointer variables to quicklaunch, searchbox etc.
	 * because they should be set NULL to by calling _xfdashboard_stage_reset_reference_on_destroy
	 * when stage actor was destroyed.
	 */
	clutter_actor_iter_init(&childIter, CLUTTER_ACTOR(self));
	while(clutter_actor_iter_next(&childIter, &child))
	{
		if(XFDASHBOARD_IS_STAGE_INTERFACE(child))
		{
			clutter_actor_iter_destroy(&childIter);
		}
	}

	/* Add all new interfaces to stage */
	for(iter=interfaces; iter; iter=g_list_next(iter))
	{
		/* Get interface to add to stage */
		interface=CLUTTER_ACTOR(iter->data);
		if(!interface) continue;

		/* Add interface to stage */
		clutter_actor_add_child(CLUTTER_ACTOR(self), interface);

		/* Only check children, set up pointer variables to quicklaunch, searchbox etc.
		 * and connect signals for primary monitor.
		 */
		monitor=xfdashboard_stage_interface_get_monitor(XFDASHBOARD_STAGE_INTERFACE(interface));
		if(!monitor || xfdashboard_window_tracker_monitor_is_primary(monitor))
		{
			/* Remember primary interface */
			if(!priv->primaryInterface)
			{
				priv->primaryInterface=interface;
				g_signal_connect(priv->primaryInterface, "destroy", G_CALLBACK(_xfdashboard_stage_reset_reference_on_destroy), &priv->primaryInterface);
			}
				else g_critical(_("Invalid multiple stages for primary monitor"));

			/* Get children from built stage and connect signals */
			priv->viewSelector=NULL;
			child=xfdashboard_find_actor_by_name(CLUTTER_ACTOR(self), "view-selector");
			if(child && XFDASHBOARD_IS_VIEW_SELECTOR(child))
			{
				priv->viewSelector=child;
				g_signal_connect(child, "destroy", G_CALLBACK(_xfdashboard_stage_reset_reference_on_destroy), &priv->viewSelector);

				/* Register this focusable actor if it is focusable */
				if(XFDASHBOARD_IS_FOCUSABLE(priv->viewSelector))
				{
					xfdashboard_focus_manager_register(priv->focusManager,
														XFDASHBOARD_FOCUSABLE(priv->viewSelector));
				}
			}

			priv->searchbox=NULL;
			child=xfdashboard_find_actor_by_name(CLUTTER_ACTOR(self), "searchbox");
			if(child && XFDASHBOARD_IS_TEXT_BOX(child))
			{
				priv->searchbox=child;
				g_signal_connect(child, "destroy", G_CALLBACK(_xfdashboard_stage_reset_reference_on_destroy), &priv->searchbox);

				/* If no hint-text was defined, set default one */
				if(!xfdashboard_text_box_is_hint_text_set(XFDASHBOARD_TEXT_BOX(priv->searchbox)))
				{
					xfdashboard_text_box_set_hint_text(XFDASHBOARD_TEXT_BOX(priv->searchbox),
														_("Just type to search..."));
				}

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
				}
			}

			priv->viewpad=NULL;
			child=xfdashboard_find_actor_by_name(CLUTTER_ACTOR(self), "viewpad");
			if(child && XFDASHBOARD_IS_VIEWPAD(child))
			{
				priv->viewpad=child;
				g_signal_connect(child, "destroy", G_CALLBACK(_xfdashboard_stage_reset_reference_on_destroy), &priv->viewpad);

				/* Connect signals */
				g_signal_connect_swapped(priv->viewpad, "view-activated", G_CALLBACK(_xfdashboard_stage_on_view_activated), self);

				/* Register this focusable actor if it is focusable */
				if(XFDASHBOARD_IS_FOCUSABLE(priv->viewpad))
				{
					xfdashboard_focus_manager_register(priv->focusManager,
														XFDASHBOARD_FOCUSABLE(priv->viewpad));

					/* Check if viewpad can be focused to enforce all focusable views
					 * will be registered too. We need to do it now to get all focusable
					 * views registered before first use of any function of focus manager.
					 */
					xfdashboard_focusable_can_focus(XFDASHBOARD_FOCUSABLE(priv->viewpad));
				}
			}

			priv->quicklaunch=NULL;
			child=xfdashboard_find_actor_by_name(CLUTTER_ACTOR(self), "quicklaunch");
			if(child && XFDASHBOARD_IS_QUICKLAUNCH(child))
			{
				XfdashboardToggleButton		*appsButton;

				priv->quicklaunch=child;
				g_signal_connect(child, "destroy", G_CALLBACK(_xfdashboard_stage_reset_reference_on_destroy), &priv->quicklaunch);

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

			priv->workspaces=NULL;
			child=xfdashboard_find_actor_by_name(CLUTTER_ACTOR(self), "workspace-selector");
			if(child && XFDASHBOARD_IS_WORKSPACE_SELECTOR(child))
			{
				priv->workspaces=child;
				g_signal_connect(child, "destroy", G_CALLBACK(_xfdashboard_stage_reset_reference_on_destroy), &priv->workspaces);

				/* Register this focusable actor if it is focusable */
				if(XFDASHBOARD_IS_FOCUSABLE(priv->workspaces))
				{
					xfdashboard_focus_manager_register(priv->focusManager,
														XFDASHBOARD_FOCUSABLE(priv->workspaces));
				}
			}

			priv->notification=NULL;
			child=xfdashboard_find_actor_by_name(CLUTTER_ACTOR(self), "notification");
			if(child && XFDASHBOARD_IS_TEXT_BOX(child))
			{
				priv->notification=child;
				g_signal_connect(child, "destroy", G_CALLBACK(_xfdashboard_stage_reset_reference_on_destroy), &priv->notification);

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

			priv->tooltip=NULL;
			child=xfdashboard_find_actor_by_name(CLUTTER_ACTOR(self), "tooltip");
			if(child && XFDASHBOARD_IS_TEXT_BOX(child))
			{
				priv->tooltip=child;
				g_signal_connect(child, "destroy", G_CALLBACK(_xfdashboard_stage_reset_reference_on_destroy), &priv->tooltip);

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
		}
	}

	/* Release allocated resources */
	g_list_free(interfaces);

	/* Set focus */
	_xfdashboard_stage_set_focus(self);
}

/* Primary monitor changed */
static void _xfdashboard_stage_on_primary_monitor_changed(XfdashboardStage *self,
															XfdashboardWindowTrackerMonitor *inOldMonitor,
															XfdashboardWindowTrackerMonitor *inNewMonitor,
															gpointer inUserData)
{
	XfdashboardStagePrivate					*priv;
	XfdashboardStageInterface				*oldStageInterface;
	XfdashboardWindowTrackerMonitor			*oldPrimaryStageInterfaceMonitor;
	ClutterActorIter						childIter;
	ClutterActor							*child;

	g_return_if_fail(XFDASHBOARD_IS_STAGE(self));
	g_return_if_fail(!inOldMonitor || XFDASHBOARD_IS_WINDOW_TRACKER_MONITOR(inOldMonitor));
	g_return_if_fail(XFDASHBOARD_IS_WINDOW_TRACKER_MONITOR(inNewMonitor));
	g_return_if_fail(XFDASHBOARD_IS_WINDOW_TRACKER(inUserData));

	priv=self->priv;

	/* If we do not have a primary stage interface yet do nothing */
	if(!priv->primaryInterface) return;

	/* If primary stage interface has already new monitor set do nothing */
	oldPrimaryStageInterfaceMonitor=xfdashboard_stage_interface_get_monitor(XFDASHBOARD_STAGE_INTERFACE(priv->primaryInterface));
	if(oldPrimaryStageInterfaceMonitor==inNewMonitor) return;

	/* Find stage interface currently using the new primary monitor */
	oldStageInterface=NULL;

	clutter_actor_iter_init(&childIter, CLUTTER_ACTOR(self));
	while(!oldStageInterface && clutter_actor_iter_next(&childIter, &child))
	{
		XfdashboardStageInterface			*interface;

		/* Check for stage interface */
		if(!XFDASHBOARD_IS_STAGE_INTERFACE(child)) continue;

		/* Get stage interface */
		interface=XFDASHBOARD_STAGE_INTERFACE(child);

		/* Check if stage interface is using new primary monitor then remember it */
		if(xfdashboard_stage_interface_get_monitor(interface)==inNewMonitor)
		{
			oldStageInterface=interface;
		}
	}

	/* Set old primary monitor at stage interface which is using new primary monitor */
	if(oldStageInterface)
	{
		/* Set old monitor at found stage interface */
		xfdashboard_stage_interface_set_monitor(oldStageInterface, oldPrimaryStageInterfaceMonitor);
	}

	/* Set new primary monitor at primary stage interface */
	xfdashboard_stage_interface_set_monitor(XFDASHBOARD_STAGE_INTERFACE(priv->primaryInterface), inNewMonitor);
	g_debug("Primary monitor changed from %d to %d",
				xfdashboard_window_tracker_monitor_get_number(oldPrimaryStageInterfaceMonitor),
				xfdashboard_window_tracker_monitor_get_number(inNewMonitor));
}

/* A monitor was added */
static void _xfdashboard_stage_on_monitor_added(XfdashboardStage *self,
												XfdashboardWindowTrackerMonitor *inMonitor,
												gpointer inUserData)
{
	ClutterActor							*interface;
	XfdashboardTheme						*theme;
	XfdashboardThemeLayout					*themeLayout;

	g_return_if_fail(XFDASHBOARD_IS_STAGE(self));
	g_return_if_fail(XFDASHBOARD_IS_WINDOW_TRACKER_MONITOR(inMonitor));
	g_return_if_fail(XFDASHBOARD_IS_WINDOW_TRACKER(inUserData));

	/* Get theme and theme layout */
	theme=xfdashboard_application_get_theme();
	themeLayout=xfdashboard_theme_get_layout(theme);

	/* Create interface for non-primary monitors. If no interface is defined in theme
	 * then create an empty interface.
	 */
	interface=xfdashboard_theme_layout_build_interface(themeLayout, XFDASHBOARD_THEME_LAYOUT_SECONDARY);
	if(!interface)
	{
		interface=xfdashboard_stage_interface_new();
	}

	if(!XFDASHBOARD_IS_STAGE_INTERFACE(interface))
	{
		g_critical(_("Interface '%s' from theme '%s' must be an actor of type %s"),
					XFDASHBOARD_THEME_LAYOUT_SECONDARY,
					xfdashboard_theme_get_theme_name(theme),
					g_type_name(XFDASHBOARD_TYPE_STAGE_INTERFACE));
		return;
	}

	/* Set monitor at interface */
	xfdashboard_stage_interface_set_monitor(XFDASHBOARD_STAGE_INTERFACE(interface), inMonitor);

	/* Add interface to stage */
	clutter_actor_add_child(CLUTTER_ACTOR(self), interface);
	g_debug("Added stage interface for new monitor %d",
				xfdashboard_window_tracker_monitor_get_number(inMonitor));

	/* If monitor added is the primary monitor then swap now the stage interfaces */
	if(xfdashboard_window_tracker_monitor_is_primary(inMonitor))
	{
		_xfdashboard_stage_on_primary_monitor_changed(self, NULL, inMonitor, inUserData);
	}
}

/* A monitor was removed */
static void _xfdashboard_stage_on_monitor_removed(XfdashboardStage *self,
													XfdashboardWindowTrackerMonitor *inMonitor,
													gpointer inUserData)
{
	XfdashboardStagePrivate					*priv;
	ClutterActorIter						childIter;
	ClutterActor							*child;

	g_return_if_fail(XFDASHBOARD_IS_STAGE(self));
	g_return_if_fail(XFDASHBOARD_IS_WINDOW_TRACKER_MONITOR(inMonitor));
	g_return_if_fail(XFDASHBOARD_IS_WINDOW_TRACKER(inUserData));

	priv=self->priv;

	/* If monitor removed is the primary monitor swap primary interface with first
	 * stage interface to keep it alive. We should afterward receive a signal that
	 * primary monitor has changed, then the primary interface will be set to its
	 * right place.
	 */
	if(xfdashboard_window_tracker_monitor_is_primary(inMonitor))
	{
		XfdashboardWindowTrackerMonitor*	firstMonitor;

		/* Get first monitor */
		firstMonitor=xfdashboard_window_tracker_get_monitor_by_number(priv->windowTracker, 0);

		/* Swp stage interfaces */
		_xfdashboard_stage_on_primary_monitor_changed(self, inMonitor, firstMonitor, inUserData);
	}

	/* Look up stage interface for removed monitor and destroy it */
	clutter_actor_iter_init(&childIter, CLUTTER_ACTOR(self));
	while(clutter_actor_iter_next(&childIter, &child))
	{
		XfdashboardStageInterface			*interface;

		/* Only check stage interfaces */
		if(!XFDASHBOARD_IS_STAGE_INTERFACE(child)) continue;

		/* If stage interface is the one for this monitor then destroy it */
		interface=XFDASHBOARD_STAGE_INTERFACE(child);
		if(xfdashboard_stage_interface_get_monitor(interface)==inMonitor)
		{
			clutter_actor_iter_destroy(&childIter);
			g_debug("Removed stage interface for removed monitor %d",
						xfdashboard_window_tracker_monitor_get_number(inMonitor));
		}
	}
}

/* Screen size has changed */
static void _xfdashboard_stage_on_screen_size_changed(XfdashboardStage *self,
														gint inWidth,
														gint inHeight,
														gpointer inUserData)
{
	gfloat				stageWidth, stageHeight;

	g_return_if_fail(XFDASHBOARD_IS_STAGE(self));
	g_return_if_fail(inWidth>0);
	g_return_if_fail(inHeight>0);

	/* Get current size of stage */
	clutter_actor_get_size(CLUTTER_ACTOR(self), &stageWidth, &stageHeight);

	/* If either stage's width or height does not match screen's width or height
	 * resize the stage.
	 */
	if((gint)stageWidth!=inWidth ||
		(gint)stageHeight!=inHeight)
	{
		g_debug("Screen resized to %dx%d but stage has size of %dx%d - resizing stage",
					inWidth, inHeight,
					(gint)stageWidth, (gint)stageHeight);

		clutter_actor_set_size(CLUTTER_ACTOR(self), inWidth, inHeight);
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

	if(priv->backgroundColor)
	{
		clutter_color_free(priv->backgroundColor);
		priv->backgroundColor=NULL;
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

	if(priv->primaryInterface)
	{
		clutter_actor_destroy(priv->primaryInterface);
		priv->primaryInterface=NULL;
	}

	if(priv->viewBeforeSearch)
	{
		g_object_unref(priv->viewBeforeSearch);
		priv->viewBeforeSearch=NULL;
	}

	if(priv->backgroundImageLayer)
	{
		clutter_actor_destroy(priv->backgroundImageLayer);
		priv->backgroundImageLayer=NULL;
	}

	if(priv->backgroundColorLayer)
	{
		clutter_actor_destroy(priv->backgroundColorLayer);
		priv->backgroundColorLayer=NULL;
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
		case PROP_BACKGROUND_IMAGE_TYPE:
			xfdashboard_stage_set_background_image_type(self, g_value_get_enum(inValue));
			break;

		case PROP_BACKGROUND_COLOR:
			xfdashboard_stage_set_background_color(self, clutter_value_get_color(inValue));
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
		case PROP_BACKGROUND_IMAGE_TYPE:
			g_value_set_enum(outValue, priv->backgroundType);
			break;

		case PROP_BACKGROUND_COLOR:
			clutter_value_set_color(outValue, priv->backgroundColor);
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
	ClutterActorClass				*actorClass=CLUTTER_ACTOR_CLASS(klass);
	GObjectClass					*gobjectClass=G_OBJECT_CLASS(klass);

	/* Override functions */
	klass->show_tooltip=_xfdashboard_stage_show_tooltip;

	actorClass->show=_xfdashboard_stage_show;
	actorClass->event=_xfdashboard_stage_event;

	gobjectClass->dispose=_xfdashboard_stage_dispose;
	gobjectClass->set_property=_xfdashboard_stage_set_property;
	gobjectClass->get_property=_xfdashboard_stage_get_property;

	/* Set up private structure */
	g_type_class_add_private(klass, sizeof(XfdashboardStagePrivate));

	/* Define properties */
	XfdashboardStageProperties[PROP_BACKGROUND_IMAGE_TYPE]=
		g_param_spec_enum("background-image-type",
							_("Background image type"),
							_("Background image type"),
							XFDASHBOARD_TYPE_STAGE_BACKGROUND_IMAGE_TYPE,
							XFDASHBOARD_STAGE_BACKGROUND_IMAGE_TYPE_NONE,
							G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS);

	XfdashboardStageProperties[PROP_BACKGROUND_COLOR]=
		clutter_param_spec_color("background-color",
									_("Background color"),
									_("Color of stage's background"),
									NULL,
									G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS);

	g_object_class_install_properties(gobjectClass, PROP_LAST, XfdashboardStageProperties);

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
	ClutterConstraint			*widthConstraint;
	ClutterConstraint			*heightConstraint;
	ClutterColor				transparent;

	priv=self->priv=XFDASHBOARD_STAGE_GET_PRIVATE(self);

	/* Set default values */
	priv->focusManager=xfdashboard_focus_manager_get_default();
	priv->windowTracker=xfdashboard_window_tracker_get_default();
	priv->stageWindow=NULL;
	priv->primaryInterface=NULL;
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
	priv->backgroundType=XFDASHBOARD_STAGE_BACKGROUND_IMAGE_TYPE_NONE;
	priv->backgroundColor=NULL;
	priv->backgroundColorLayer=NULL;
	priv->backgroundImageLayer=NULL;

	/* Create background actors but order of adding background children is important */
	widthConstraint=clutter_bind_constraint_new(CLUTTER_ACTOR(self), CLUTTER_BIND_WIDTH, 0.0f);
	heightConstraint=clutter_bind_constraint_new(CLUTTER_ACTOR(self), CLUTTER_BIND_HEIGHT, 0.0f);
	priv->backgroundImageLayer=clutter_actor_new();
	clutter_actor_hide(priv->backgroundImageLayer);
	clutter_actor_add_constraint(priv->backgroundImageLayer, widthConstraint);
	clutter_actor_add_constraint(priv->backgroundImageLayer, heightConstraint);
	clutter_actor_add_child(CLUTTER_ACTOR(self), priv->backgroundImageLayer);

	widthConstraint=clutter_bind_constraint_new(CLUTTER_ACTOR(self), CLUTTER_BIND_WIDTH, 0.0f);
	heightConstraint=clutter_bind_constraint_new(CLUTTER_ACTOR(self), CLUTTER_BIND_HEIGHT, 0.0f);
	priv->backgroundColorLayer=clutter_actor_new();
	clutter_actor_hide(priv->backgroundColorLayer);
	clutter_actor_add_constraint(priv->backgroundColorLayer, widthConstraint);
	clutter_actor_add_constraint(priv->backgroundColorLayer, heightConstraint);
	clutter_actor_add_child(CLUTTER_ACTOR(self), priv->backgroundColorLayer);

	/* Set up stage and style it */
	clutter_color_init(&transparent, 0, 0, 0, 0);
	clutter_actor_set_background_color(CLUTTER_ACTOR(self), &transparent);

	clutter_stage_set_use_alpha(CLUTTER_STAGE(self), TRUE);
	clutter_stage_set_user_resizable(CLUTTER_STAGE(self), FALSE);
	clutter_stage_set_fullscreen(CLUTTER_STAGE(self), TRUE);

	/* Connect signals to window tracker */
	g_signal_connect_swapped(priv->windowTracker,
								"monitor-added",
								G_CALLBACK(_xfdashboard_stage_on_monitor_added),
								self);
	g_signal_connect_swapped(priv->windowTracker,
								"monitor-removed",
								G_CALLBACK(_xfdashboard_stage_on_monitor_removed),
								self);
	g_signal_connect_swapped(priv->windowTracker,
								"primary-monitor-changed",
								G_CALLBACK(_xfdashboard_stage_on_primary_monitor_changed),
								self);

	/* Connect signal to application */
	application=xfdashboard_application_get_default();
	g_signal_connect_swapped(application,
								"suspend",
								G_CALLBACK(_xfdashboard_stage_on_application_suspend),
								self);

	g_signal_connect_swapped(application,
								"resume",
								G_CALLBACK(_xfdashboard_stage_on_application_resume),
								self);

	g_signal_connect_swapped(application,
								"theme-changed",
								G_CALLBACK(_xfdashboard_stage_on_application_theme_changed),
								self);

	/* Resize stage to match screen size and listen for futher screen size changes
	 * to resize stage again.
	 * This should only be needed when compiled against Clutter prior to 1.17.2
	 * because this version or newer ones seem to handle window resizes correctly.
	 */
	if(clutter_major_version<1 ||
		(clutter_major_version==1 && clutter_minor_version<17) ||
		(clutter_major_version==1 && clutter_minor_version==17 && clutter_micro_version<2))
	{
		gint					screenWidth, screenHeight;

		screenWidth=xfdashboard_window_tracker_get_screen_width(priv->windowTracker);
		screenHeight=xfdashboard_window_tracker_get_screen_height(priv->windowTracker);
		_xfdashboard_stage_on_screen_size_changed(self, screenWidth, screenHeight, priv->windowTracker);

		g_signal_connect_swapped(priv->windowTracker,
									"screen-size-changed",
									G_CALLBACK(_xfdashboard_stage_on_screen_size_changed),
									self);

		g_debug("Tracking screen resizes to resize stage");
	}
}

/* IMPLEMENTATION: Public API */

/* Create new instance */
ClutterActor* xfdashboard_stage_new(void)
{
	return(CLUTTER_ACTOR(g_object_new(XFDASHBOARD_TYPE_STAGE, NULL)));
}

/* Get/set background type */
XfdashboardStageBackgroundImageType xfdashboard_stage_get_background_image_type(XfdashboardStage *self)
{
	g_return_val_if_fail(XFDASHBOARD_IS_STAGE(self), XFDASHBOARD_STAGE_BACKGROUND_IMAGE_TYPE_NONE);

	return(self->priv->backgroundType);
}

void xfdashboard_stage_set_background_image_type(XfdashboardStage *self, XfdashboardStageBackgroundImageType inType)
{
	XfdashboardStagePrivate		*priv;

	g_return_if_fail(XFDASHBOARD_IS_STAGE(self));
	g_return_if_fail(inType<=XFDASHBOARD_STAGE_BACKGROUND_IMAGE_TYPE_DESKTOP);

	priv=self->priv;


	/* Set value if changed */
	if(priv->backgroundType!=inType)
	{
		/* Set value */
		priv->backgroundType=inType;

		/* Set up background actor depending on type */
		if(priv->backgroundImageLayer)
		{
			switch(priv->backgroundType)
			{
				case XFDASHBOARD_STAGE_BACKGROUND_IMAGE_TYPE_DESKTOP:
					{
						XfdashboardWindowTrackerWindow	*backgroundWindow;

						backgroundWindow=xfdashboard_window_tracker_get_root_window(priv->windowTracker);
						if(backgroundWindow)
						{
							ClutterContent				*backgroundContent;

							backgroundContent=xfdashboard_window_content_new_for_window(backgroundWindow);
							clutter_actor_show(priv->backgroundImageLayer);
							clutter_actor_set_content(priv->backgroundImageLayer, backgroundContent);
							g_object_unref(backgroundContent);

							g_debug("Desktop window was found and set up as background image for stage");
						}
							else
							{
								g_signal_connect_swapped(priv->windowTracker,
															"window-opened",
															G_CALLBACK(_xfdashboard_stage_on_desktop_window_opened),
															self);
								g_debug("Desktop window was not found. Setting up signal to get notified when desktop window might be opened.");
							}
					}
					break;

				default:
					clutter_actor_hide(priv->backgroundImageLayer);
					clutter_actor_set_content(priv->backgroundImageLayer, NULL);
					break;
			}
		}

		/* Notify about property change */
		g_object_notify_by_pspec(G_OBJECT(self), XfdashboardStageProperties[PROP_BACKGROUND_IMAGE_TYPE]);
	}
}

/* Get/set background color */
ClutterColor* xfdashboard_stage_get_background_color(XfdashboardStage *self)
{
	g_return_val_if_fail(XFDASHBOARD_IS_STAGE(self), NULL);

	return(self->priv->backgroundColor);
}

void xfdashboard_stage_set_background_color(XfdashboardStage *self, const ClutterColor *inColor)
{
	XfdashboardStagePrivate		*priv;

	g_return_if_fail(XFDASHBOARD_IS_STAGE(self));

	priv=self->priv;


	/* Set value if changed */
	if((priv->backgroundColor && !inColor) ||
		(!priv->backgroundColor && inColor) ||
		(inColor && clutter_color_equal(inColor, priv->backgroundColor)==FALSE))
	{
		/* Set value */
		if(priv->backgroundColor)
		{
			clutter_color_free(priv->backgroundColor);
			priv->backgroundColor=NULL;
		}

		if(inColor) priv->backgroundColor=clutter_color_copy(inColor);

		/* If a color is provided set background color and show background actor
		 * otherwise hide background actor
		 */
		if(priv->backgroundColorLayer)
		{
			if(priv->backgroundColor)
			{
				clutter_actor_set_background_color(priv->backgroundColorLayer,
													priv->backgroundColor);
				clutter_actor_show(priv->backgroundColorLayer);
			}
				else clutter_actor_hide(priv->backgroundColorLayer);
		}

		/* Notify about property change */
		g_object_notify_by_pspec(G_OBJECT(self), XfdashboardStageProperties[PROP_BACKGROUND_COLOR]);
	}
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
