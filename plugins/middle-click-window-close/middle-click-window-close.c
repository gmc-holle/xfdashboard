/*
 * middle-click-window-close: Closes windows in window by middle-click
 * 
 * Copyright 2012-2019 Stephan Haller <nomad@froevel.de>
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

#include "middle-click-window-close.h"

#include <libxfdashboard/libxfdashboard.h>
#include <glib/gi18n-lib.h>
#include <gtk/gtk.h>
#include <math.h>


/* Define this class in GObject system */
struct _XfdashboardMiddleClickWindowClosePrivate
{
	/* Instance related */
	XfdashboardStage						*stage;
	guint									stageActorCreatedSignalID;
	guint									stageDestroySignalID;

	XfdashboardCssSelector					*liveWindowSelector;
};

G_DEFINE_DYNAMIC_TYPE_EXTENDED(XfdashboardMiddleClickWindowClose,
								xfdashboard_middle_click_window_close,
								G_TYPE_OBJECT,
								0,
								G_ADD_PRIVATE_DYNAMIC(XfdashboardMiddleClickWindowClose))

/* Define this class in this plugin */
XFDASHBOARD_DEFINE_PLUGIN_TYPE(xfdashboard_middle_click_window_close);

/* IMPLEMENTATION: Private variables and methods */
#define DEFAULT_WINDOW_CLOSE_BUTTON							XFDASHBOARD_CLICK_ACTION_MIDDLE_BUTTON
#define XFDASHBOARD_MIDDLE_CLICK_WINDOW_CLOSE_ACTION_NAME	"middle-click-window-close-action"

/* A configured live window actor was clicked */
static void _xfdashboard_middle_click_window_close_on_clicked(XfdashboardMiddleClickWindowClose *self,
																ClutterActor *inActor,
																gpointer inUserData)
{
	XfdashboardLiveWindowSimple						*liveWindow;
	XfdashboardClickAction							*action;
	guint											button;
	XfdashboardWindowTrackerWindow					*window;

	g_return_if_fail(XFDASHBOARD_IS_MIDDLE_CLICK_WINDOW_CLOSE(self));
	g_return_if_fail(XFDASHBOARD_IS_LIVE_WINDOW(inActor));
	g_return_if_fail(XFDASHBOARD_IS_CLICK_ACTION(inUserData));

	liveWindow=XFDASHBOARD_LIVE_WINDOW_SIMPLE(inActor);
	action=XFDASHBOARD_CLICK_ACTION(inUserData);

	/* Get button used for click action */
	button=xfdashboard_click_action_get_button(action);
	if(button==DEFAULT_WINDOW_CLOSE_BUTTON)
	{
		window=xfdashboard_live_window_simple_get_window(liveWindow);
		xfdashboard_window_tracker_window_close(window);
	}
}

/* An actor was created so check if we are interested at this one */
static void _xfdashboard_middle_click_window_close_on_actor_created(XfdashboardMiddleClickWindowClose *self,
																	ClutterActor *inActor,
																	gpointer inUserData)
{
	XfdashboardMiddleClickWindowClosePrivate		*priv;
	gint											score;
	ClutterAction									*action;

	g_return_if_fail(XFDASHBOARD_IS_MIDDLE_CLICK_WINDOW_CLOSE(self));
	g_return_if_fail(CLUTTER_IS_ACTOR(inActor));

	priv=self->priv;

	/* Check if we are interested in this newly created actor and set it up */
	if(XFDASHBOARD_IS_STYLABLE(inActor))
	{
		score=xfdashboard_css_selector_score(priv->liveWindowSelector, XFDASHBOARD_STYLABLE(inActor));
		if(score>0)
		{
			action=xfdashboard_click_action_new();
			clutter_actor_add_action_with_name(inActor, XFDASHBOARD_MIDDLE_CLICK_WINDOW_CLOSE_ACTION_NAME, action);
			g_signal_connect_swapped(action, "clicked", G_CALLBACK(_xfdashboard_middle_click_window_close_on_clicked), self);
		}
	}
}

/* Callback for traversal to setup live window for use with this plugin */
static gboolean _xfdashboard_middle_click_window_close_traverse_acquire(ClutterActor *inActor,
																		gpointer inUserData)
{
	XfdashboardMiddleClickWindowClose				*self;
	ClutterAction									*action;

	g_return_val_if_fail(XFDASHBOARD_IS_LIVE_WINDOW(inActor), XFDASHBOARD_TRAVERSAL_CONTINUE);
	g_return_val_if_fail(XFDASHBOARD_IS_MIDDLE_CLICK_WINDOW_CLOSE(inUserData), XFDASHBOARD_TRAVERSAL_CONTINUE);

	self=XFDASHBOARD_MIDDLE_CLICK_WINDOW_CLOSE(inUserData);

	/* Set up live window */
	action=xfdashboard_click_action_new();
	clutter_actor_add_action_with_name(inActor, XFDASHBOARD_MIDDLE_CLICK_WINDOW_CLOSE_ACTION_NAME, action);
	g_signal_connect_swapped(action, "clicked", G_CALLBACK(_xfdashboard_middle_click_window_close_on_clicked), self);

	/* All done continue with traversal */
	return(XFDASHBOARD_TRAVERSAL_CONTINUE);
}

/* Callback for traversal to deconfigure live window from use at this plugin */
static gboolean _xfdashboard_middle_click_window_close_traverse_release(ClutterActor *inActor,
																		gpointer inUserData)
{
	g_return_val_if_fail(XFDASHBOARD_IS_LIVE_WINDOW(inActor), XFDASHBOARD_TRAVERSAL_CONTINUE);
	g_return_val_if_fail(XFDASHBOARD_IS_MIDDLE_CLICK_WINDOW_CLOSE(inUserData), XFDASHBOARD_TRAVERSAL_CONTINUE);

	/* Set up live window */
	clutter_actor_remove_action_by_name(inActor, XFDASHBOARD_MIDDLE_CLICK_WINDOW_CLOSE_ACTION_NAME);

	/* All done continue with traversal */
	return(XFDASHBOARD_TRAVERSAL_CONTINUE);
}

/* Stage is going to be destroyed */
static void _xfdashboard_middle_click_window_close_on_stage_destroyed(XfdashboardMiddleClickWindowClose *self,
																		gpointer inUserData)
{
	XfdashboardMiddleClickWindowClosePrivate		*priv;
	XfdashboardStage								*stage;

	g_return_if_fail(XFDASHBOARD_IS_MIDDLE_CLICK_WINDOW_CLOSE(self));
	g_return_if_fail(XFDASHBOARD_IS_STAGE(inUserData));

	priv=self->priv;
	stage=XFDASHBOARD_STAGE(inUserData);

	/* Iterate through all existing live window actors that may still exist
	 * and deconfigure them from use at this plugin. We traverse the stage
	 * which is going to be destroyed and provided as function parameter
	 * regardless if it the stage we have set up initially or if it is any other.
	 */
	xfdashboard_traverse_actor(CLUTTER_ACTOR(stage),
								priv->liveWindowSelector,
								_xfdashboard_middle_click_window_close_traverse_release,
								self);

	/* Disconnect signals from stage as it will be destroyed and reset variables
	 * but only if it the stage we are handling right now (this should always be
	 * the case!)
	 */
	if(priv->stage==stage)
	{
		/* Disconnect signals */
		if(priv->stageActorCreatedSignalID)
		{
			g_signal_handler_disconnect(priv->stage, priv->stageActorCreatedSignalID);
			priv->stageActorCreatedSignalID=0;
		}

		if(priv->stageDestroySignalID)
		{
			g_signal_handler_disconnect(priv->stage, priv->stageDestroySignalID);
			priv->stageDestroySignalID=0;
		}

		/* Release stage */
		priv->stage=NULL;
	}
}

/* IMPLEMENTATION: GObject */

/* Dispose this object */
static void _xfdashboard_middle_click_window_close_dispose(GObject *inObject)
{
	XfdashboardMiddleClickWindowClose				*self=XFDASHBOARD_MIDDLE_CLICK_WINDOW_CLOSE(inObject);
	XfdashboardMiddleClickWindowClosePrivate		*priv=self->priv;

	/* Release allocated resources */
	if(priv->stage)
	{
		/* Iterate through all existing live window actors that may still exist
		 * and deconfigure them from use at this plugin.
		 */
		xfdashboard_traverse_actor(CLUTTER_ACTOR(priv->stage),
									priv->liveWindowSelector,
									_xfdashboard_middle_click_window_close_traverse_release,
									self);

		/* Disconnect signals from stage */
		if(priv->stageActorCreatedSignalID)
		{
			g_signal_handler_disconnect(priv->stage, priv->stageActorCreatedSignalID);
			priv->stageActorCreatedSignalID=0;
		}

		if(priv->stageDestroySignalID)
		{
			g_signal_handler_disconnect(priv->stage, priv->stageDestroySignalID);
			priv->stageDestroySignalID=0;
		}

		/* Release stage */
		priv->stage=NULL;
	}

	if(priv->liveWindowSelector)
	{
		g_object_unref(priv->liveWindowSelector);
		priv->liveWindowSelector=NULL;
	}

	/* Call parent's class dispose method */
	G_OBJECT_CLASS(xfdashboard_middle_click_window_close_parent_class)->dispose(inObject);
}

/* Class initialization
 * Override functions in parent classes and define properties
 * and signals
 */
void xfdashboard_middle_click_window_close_class_init(XfdashboardMiddleClickWindowCloseClass *klass)
{
	GObjectClass			*gobjectClass=G_OBJECT_CLASS(klass);

	/* Override functions */
	gobjectClass->dispose=_xfdashboard_middle_click_window_close_dispose;
}

/* Class finalization */
void xfdashboard_middle_click_window_close_class_finalize(XfdashboardMiddleClickWindowCloseClass *klass)
{
}

/* Object initialization
 * Create private structure and set up default values
 */
void xfdashboard_middle_click_window_close_init(XfdashboardMiddleClickWindowClose *self)
{
	XfdashboardMiddleClickWindowClosePrivate		*priv;

	self->priv=priv=xfdashboard_middle_click_window_close_get_instance_private(self);

	/* Set up default values */
	priv->stage=xfdashboard_application_get_stage(NULL);
	priv->stageActorCreatedSignalID=0;
	priv->stageDestroySignalID=0;
	priv->liveWindowSelector=xfdashboard_css_selector_new_from_string("XfdashboardWindowsView XfdashboardLiveWindow");

	/* Iterate through all already existing live window actors and configure
	 * them for use with this plugin.
	 */
	xfdashboard_traverse_actor(CLUTTER_ACTOR(priv->stage),
								priv->liveWindowSelector,
								_xfdashboard_middle_click_window_close_traverse_acquire,
								self);

	/* Connect signal to get notified about actor creations  and filter out
	 * and set up the ones we are interested in.
	 */
	priv->stageActorCreatedSignalID=
		g_signal_connect_swapped(priv->stage,
									"actor-created",
									G_CALLBACK(_xfdashboard_middle_click_window_close_on_actor_created),
									self);

	/* Connect signal to get notified when stage is getting destoyed */
	priv->stageDestroySignalID=
		g_signal_connect_swapped(priv->stage,
									"destroy",
									G_CALLBACK(_xfdashboard_middle_click_window_close_on_stage_destroyed),
									self);
}


/* IMPLEMENTATION: Public API */

/* Create new instance */
XfdashboardMiddleClickWindowClose* xfdashboard_middle_click_window_close_new(void)
{
	GObject		*middleClickWindowClose;

	middleClickWindowClose=g_object_new(XFDASHBOARD_TYPE_MIDDLE_CLICK_WINDOW_CLOSE, NULL);
	if(!middleClickWindowClose) return(NULL);

	return(XFDASHBOARD_MIDDLE_CLICK_WINDOW_CLOSE(middleClickWindowClose));
}
