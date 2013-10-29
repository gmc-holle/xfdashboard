/*
 * windows-view: A view showing visible windows
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

#include <glib/gi18n-lib.h>

#include "windows-view.h"
#include "live-window.h"
#include "scaled-table-layout.h"
#include "utils.h"
#include "stage.h"
#include "application.h"
#include "view.h"

/* Define this class in GObject system */
G_DEFINE_TYPE(XfdashboardWindowsView,
				xfdashboard_windows_view,
				XFDASHBOARD_TYPE_VIEW)

/* Private structure - access only by public API if needed */
#define XFDASHBOARD_WINDOWS_VIEW_GET_PRIVATE(obj) \
	(G_TYPE_INSTANCE_GET_PRIVATE((obj), XFDASHBOARD_TYPE_WINDOWS_VIEW, XfdashboardWindowsViewPrivate))

struct _XfdashboardWindowsViewPrivate
{
	/* Properties related */
	WnckWorkspace	*workspace;

	/* Instance related */
	WnckScreen		*screen;
};

/* Properties */
enum
{
	PROP_0,

	PROP_WORKSPACE,

	PROP_LAST
};

GParamSpec* XfdashboardWindowsViewProperties[PROP_LAST]={ 0, };

/* Forward declaration */
XfdashboardLiveWindow* _xfdashboard_windows_view_create_actor(XfdashboardWindowsView *self, WnckWindow *inWindow);
void _xfdashboard_windows_view_set_active_workspace(XfdashboardWindowsView *self, WnckWorkspace *inWorkspace);

/* IMPLEMENTATION: Private variables and methods */
#define DEFAULT_SPACING		8.0f					// TODO: Replace by settings/theming object
#define DEFAULT_VIEW_ICON	GTK_STOCK_FULLSCREEN	// TODO: Replace by settings/theming object

/* Check if window is a stage window */
gboolean _xfdashboard_windows_view_is_stage_window(WnckWindow *inWindow)
{
	g_return_val_if_fail(WNCK_IS_WINDOW(inWindow), FALSE);

	GSList					*stages, *entry;

	/* Iterate through stages and check if stage window matches requested one */
	stages=clutter_stage_manager_list_stages(clutter_stage_manager_get_default());
	for(entry=stages; entry!=NULL; entry=g_slist_next(entry))
	{
		XfdashboardStage	*stage;

		stage=XFDASHBOARD_STAGE(clutter_actor_get_stage(CLUTTER_ACTOR(entry->data)));
		if(stage && xfdashboard_stage_get_window(stage)==inWindow)
		{
			g_slist_free(stages);
			return(TRUE);
		}
	}
	g_slist_free(stages);

	return(FALSE);
}

/* Find live window actor by wnck-window */
XfdashboardLiveWindow* _xfdashboard_windows_view_find_by_wnck_window(XfdashboardWindowsView *self,
																		WnckWindow *inWindow)
{
	g_return_val_if_fail(XFDASHBOARD_IS_WINDOWS_VIEW(self), NULL);
	g_return_val_if_fail(WNCK_IS_WINDOW(inWindow), NULL);

	GList			*children;

	/* Iterate through list of current actors and find the one for requested window */
	for(children=clutter_actor_get_children(CLUTTER_ACTOR(self)); children; children=g_list_next(children))
	{
		if(XFDASHBOARD_IS_LIVE_WINDOW(children->data))
		{
			XfdashboardLiveWindow	*liveWindow=XFDASHBOARD_LIVE_WINDOW(children->data);

			if(xfdashboard_live_window_get_window(liveWindow)==inWindow) return(liveWindow);
		}
	}
	g_list_free(children);

	/* If we get here we did not find the window and we return NULL */
	return(NULL);
}

/* Workspace was changed */
void _xfdashboard_windows_view_on_active_workspace_changed(XfdashboardWindowsView *self,
															WnckWorkspace *inPrevWorkspace,
															gpointer inUserData)
{
	g_return_if_fail(XFDASHBOARD_IS_WINDOWS_VIEW(self));
	g_return_if_fail(WNCK_IS_SCREEN(inUserData));

	WnckScreen				*screen=WNCK_SCREEN(inUserData);
	WnckWorkspace			*workspace;
	XfdashboardStage		*stage;

	/* Get new active workspace */
	workspace=wnck_screen_get_active_workspace(screen);

	/* Get stage */
	stage=XFDASHBOARD_STAGE(clutter_actor_get_stage(CLUTTER_ACTOR(self)));

	/* Move clutter stage to new active workspace and make active */
	if(self->priv->workspace!=NULL)
	{
		wnck_window_move_to_workspace(xfdashboard_stage_get_window(stage), workspace);
		wnck_window_activate(xfdashboard_stage_get_window(stage), xfdashboard_get_current_time());
	}

	/* Update window list */
	_xfdashboard_windows_view_set_active_workspace(self, workspace);
}

/* A window was opened */
void _xfdashboard_windows_view_on_window_opened(XfdashboardWindowsView *self,
												WnckWindow *inWindow,
												gpointer inUserData)
{
	g_return_if_fail(XFDASHBOARD_IS_WINDOWS_VIEW(self));
	g_return_if_fail(WNCK_IS_SCREEN(inUserData));
	g_return_if_fail(WNCK_IS_WINDOW(inWindow));

	XfdashboardWindowsViewPrivate	*priv=self->priv;
	WnckScreen						*screen=WNCK_SCREEN(inUserData);
	WnckWorkspace					*workspace;
	XfdashboardLiveWindow			*liveWindow;

	/* Check if event happened on active screen and active workspace */
	if(screen!=priv->screen) return;

	workspace=wnck_window_get_workspace(inWindow);
	if(workspace==NULL || workspace!=priv->workspace) return;

	/* Create actor */
	liveWindow=_xfdashboard_windows_view_create_actor(self, inWindow);
	if(liveWindow) clutter_actor_add_child(CLUTTER_ACTOR(self), CLUTTER_ACTOR(liveWindow));
}

/* A window was closed */
void _xfdashboard_windows_view_on_window_closed(XfdashboardWindowsView *self,
												WnckWindow *inWindow,
												gpointer inUserData)
{
	g_return_if_fail(XFDASHBOARD_IS_WINDOWS_VIEW(self));
	g_return_if_fail(WNCK_IS_SCREEN(inUserData));
	g_return_if_fail(WNCK_IS_WINDOW(inWindow));

	WnckScreen						*screen=WNCK_SCREEN(inUserData);
	XfdashboardWindowsViewPrivate	*priv=self->priv;
	XfdashboardLiveWindow			*liveWindow;

	/* Check if event happened on active screen and active workspace */
	if(screen!=priv->screen) return;
	if(wnck_window_get_workspace(inWindow)!=priv->workspace) return;

	/* Find live window for window just being closed and destroy it */
	liveWindow=_xfdashboard_windows_view_find_by_wnck_window(self, inWindow);
	if(G_LIKELY(liveWindow)) clutter_actor_destroy(CLUTTER_ACTOR(liveWindow));
}

/* A live window was clicked */
void _xfdashboard_windows_view_on_window_clicked(XfdashboardWindowsView *self,
														gpointer inUserData)
{
	g_return_if_fail(XFDASHBOARD_IS_WINDOWS_VIEW(self));
	g_return_if_fail(XFDASHBOARD_IS_LIVE_WINDOW(inUserData));

	XfdashboardLiveWindow	*liveWindow=XFDASHBOARD_LIVE_WINDOW(inUserData);

	wnck_window_activate_transient((WnckWindow*)xfdashboard_live_window_get_window(liveWindow), xfdashboard_get_current_time());

	xfdashboard_application_quit();
}

/* The close button of a live window was clicked */
void _xfdashboard_windows_view_on_window_close_clicked(XfdashboardWindowsView *self,
															gpointer inUserData)
{
	g_return_if_fail(XFDASHBOARD_IS_WINDOWS_VIEW(self));
	g_return_if_fail(XFDASHBOARD_IS_LIVE_WINDOW(inUserData));

	XfdashboardLiveWindow	*liveWindow=XFDASHBOARD_LIVE_WINDOW(inUserData);
	WnckWindow				*window;

	window=WNCK_WINDOW(xfdashboard_live_window_get_window(liveWindow));
	wnck_window_close(window, xfdashboard_get_current_time());
}

/* A window was moved or resized */
void _xfdashboard_windows_view_on_window_geometry_changed(XfdashboardWindowsView *self, gpointer inUserData)
{
	g_return_if_fail(XFDASHBOARD_IS_WINDOWS_VIEW(self));
	g_return_if_fail(XFDASHBOARD_IS_LIVE_WINDOW(inUserData));

	XfdashboardLiveWindow	*liveWindow=XFDASHBOARD_LIVE_WINDOW(inUserData);

	/* Force a relayout to reflect new size of window */
	clutter_actor_queue_relayout(CLUTTER_ACTOR(liveWindow));
}

/* A window was hidden or shown */
void _xfdashboard_windows_view_on_window_visibility_changed(XfdashboardWindowsView *self,
															gboolean inIsVisible,
															gpointer inUserData)
{
	g_return_if_fail(XFDASHBOARD_IS_WINDOWS_VIEW(self));
	g_return_if_fail(XFDASHBOARD_IS_LIVE_WINDOW(inUserData));

	XfdashboardLiveWindow	*liveWindow=XFDASHBOARD_LIVE_WINDOW(inUserData);

	/* If window is shown, show it in window list - otherwise hide it.
	 * We should not destroy the live window actor as the window might
	 * get visible again.
	 */
	if(inIsVisible) clutter_actor_show(CLUTTER_ACTOR(liveWindow));
		else clutter_actor_hide(CLUTTER_ACTOR(liveWindow));
}

/* A window changed workspace or was pinned to all workspaces */
void _xfdashboard_windows_view_on_window_workspace_changed(XfdashboardWindowsView *self, gpointer inUserData)
{
	g_return_if_fail(XFDASHBOARD_IS_WINDOWS_VIEW(self));
	g_return_if_fail(XFDASHBOARD_IS_LIVE_WINDOW(inUserData));

	XfdashboardWindowsViewPrivate	*priv=self->priv;
	XfdashboardLiveWindow			*liveWindow=XFDASHBOARD_LIVE_WINDOW(inUserData);
	WnckWindow						*window=xfdashboard_live_window_get_window(liveWindow);

	/* If window is neither on this workspace nor pinned then destroy it */
	if(!wnck_window_is_pinned(window) &&
		wnck_window_get_workspace(window)!=priv->workspace)
	{
		clutter_actor_destroy(CLUTTER_ACTOR(liveWindow));
	}
}

/* Create actor for wnck-window and connect signals */
XfdashboardLiveWindow* _xfdashboard_windows_view_create_actor(XfdashboardWindowsView *self,
																WnckWindow *inWindow)
{
	g_return_val_if_fail(XFDASHBOARD_IS_WINDOWS_VIEW(self), NULL);
	g_return_val_if_fail(WNCK_IS_WINDOW(inWindow), NULL);

	/* Check if window opened is a stage window */
	if(_xfdashboard_windows_view_is_stage_window(inWindow)==TRUE)
	{
		g_debug("Will not create live-window actor for stage window.");
		return(NULL);
	}

	/* Create actor and connect signals */
	ClutterActor	*actor;

	actor=xfdashboard_live_window_new();
	g_signal_connect_swapped(actor, "clicked", G_CALLBACK(_xfdashboard_windows_view_on_window_clicked), self);
	g_signal_connect_swapped(actor, "close", G_CALLBACK(_xfdashboard_windows_view_on_window_close_clicked), self);
	g_signal_connect_swapped(actor, "geometry-changed", G_CALLBACK(_xfdashboard_windows_view_on_window_geometry_changed), self);
	g_signal_connect_swapped(actor, "visibility-changed", G_CALLBACK(_xfdashboard_windows_view_on_window_visibility_changed), self);
	g_signal_connect_swapped(actor, "workspace-changed", G_CALLBACK(_xfdashboard_windows_view_on_window_workspace_changed), self);
	xfdashboard_live_window_set_window(XFDASHBOARD_LIVE_WINDOW(actor), inWindow);

	return(XFDASHBOARD_LIVE_WINDOW(actor));
}

/* Set active screen */
void _xfdashboard_windows_view_set_active_workspace(XfdashboardWindowsView *self,
													WnckWorkspace *inWorkspace)
{
	g_return_if_fail(XFDASHBOARD_IS_WINDOWS_VIEW(self));
	g_return_if_fail(inWorkspace==NULL || WNCK_IS_WORKSPACE(inWorkspace));

	XfdashboardWindowsViewPrivate	*priv=XFDASHBOARD_WINDOWS_VIEW(self)->priv;
	GList							*windowsList;

	/* Do not anything if workspace is the same as before */
	if(inWorkspace==priv->workspace) return;

	/* Set new workspace */
	priv->workspace=inWorkspace;

	/* Destroy all actors */
	clutter_actor_destroy_all_children(CLUTTER_ACTOR(self));

	/* Create live window actors for new workspace */
	if(priv->workspace!=NULL)
	{
		windowsList=wnck_screen_get_windows_stacked(priv->screen);
		for(windowsList=g_list_last(windowsList); windowsList; windowsList=g_list_previous(windowsList))
		{
			WnckWindow				*window=WNCK_WINDOW(windowsList->data);
			XfdashboardLiveWindow	*liveWindow;

			/* Window must be on workspace and must not be flagged to skip tasklist */
			if(wnck_window_is_visible_on_workspace(window, priv->workspace) &&
				wnck_window_is_skip_pager(window)==FALSE &&
				wnck_window_is_skip_tasklist(window)==FALSE)
			{
				/* Create actor */
				liveWindow=_xfdashboard_windows_view_create_actor(XFDASHBOARD_WINDOWS_VIEW(self), window);
				if(liveWindow) clutter_actor_add_child(CLUTTER_ACTOR(self), CLUTTER_ACTOR(liveWindow));
			}
		}
	}

	/* Notify about property change */
	g_object_notify_by_pspec(G_OBJECT(self), XfdashboardWindowsViewProperties[PROP_WORKSPACE]);
}

/* IMPLEMENTATION: GObject */

/* Dispose this object */
void _xfdashboard_windows_view_dispose(GObject *inObject)
{
	XfdashboardWindowsView			*self=XFDASHBOARD_WINDOWS_VIEW(inObject);
	XfdashboardWindowsViewPrivate	*priv=XFDASHBOARD_WINDOWS_VIEW(self)->priv;

	/* Release allocated resources */
	_xfdashboard_windows_view_set_active_workspace(self, NULL);

	if(priv->screen)
	{
		g_signal_handlers_disconnect_by_data(priv->screen, self);
		priv->screen=NULL;
	}

	/* Call parent's class dispose method */
	G_OBJECT_CLASS(xfdashboard_windows_view_parent_class)->dispose(inObject);
}

/* Set/get properties */
void _xfdashboard_windows_view_set_property(GObject *inObject,
											guint inPropID,
											const GValue *inValue,
											GParamSpec *inSpec)
{
	XfdashboardWindowsView		*self=XFDASHBOARD_WINDOWS_VIEW(inObject);
	
	switch(inPropID)
	{
		case PROP_WORKSPACE:
			_xfdashboard_windows_view_set_active_workspace(self, g_value_get_object(inValue));
			break;

		default:
			G_OBJECT_WARN_INVALID_PROPERTY_ID(inObject, inPropID, inSpec);
			break;
	}
}

void _xfdashboard_windows_view_get_property(GObject *inObject,
											guint inPropID,
											GValue *outValue,
											GParamSpec *inSpec)
{
	XfdashboardWindowsView		*self=XFDASHBOARD_WINDOWS_VIEW(inObject);

	switch(inPropID)
	{
		case PROP_WORKSPACE:
			g_value_set_object(outValue, self->priv->workspace);
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
void xfdashboard_windows_view_class_init(XfdashboardWindowsViewClass *klass)
{
	XfdashboardViewClass	*viewClass=XFDASHBOARD_VIEW_CLASS(klass);
	GObjectClass			*gobjectClass=G_OBJECT_CLASS(klass);

	/* Override functions */
	gobjectClass->dispose=_xfdashboard_windows_view_dispose;
	gobjectClass->set_property=_xfdashboard_windows_view_set_property;
	gobjectClass->get_property=_xfdashboard_windows_view_get_property;

	/* Set up private structure */
	g_type_class_add_private(klass, sizeof(XfdashboardWindowsViewPrivate));

	/* Define properties */
	XfdashboardWindowsViewProperties[PROP_WORKSPACE]=
		g_param_spec_object ("workspace",
								_("Current workspace"),
								_("The current workspace whose windows are shown"),
								WNCK_TYPE_WORKSPACE,
								G_PARAM_READABLE);

	g_object_class_install_properties(gobjectClass, PROP_LAST, XfdashboardWindowsViewProperties);
}

/* Object initialization
 * Create private structure and set up default values
 */
void xfdashboard_windows_view_init(XfdashboardWindowsView *self)
{
	XfdashboardWindowsViewPrivate	*priv;
	ClutterLayoutManager			*layout;

	self->priv=priv=XFDASHBOARD_WINDOWS_VIEW_GET_PRIVATE(self);

	/* Set up default values */
	priv->screen=wnck_screen_get_default();
	priv->workspace=NULL;

	/* Set up view */
	xfdashboard_view_set_internal_name(XFDASHBOARD_VIEW(self), "windows");
	xfdashboard_view_set_name(XFDASHBOARD_VIEW(self), _("Windows"));
	xfdashboard_view_set_icon(XFDASHBOARD_VIEW(self), DEFAULT_VIEW_ICON);

	/* Setup actor */
	layout=xfdashboard_scaled_table_layout_new();
	xfdashboard_scaled_table_layout_set_spacing(XFDASHBOARD_SCALED_TABLE_LAYOUT(layout), DEFAULT_SPACING);
	xfdashboard_scaled_table_layout_set_relative_scale(XFDASHBOARD_SCALED_TABLE_LAYOUT(layout), TRUE);
	clutter_actor_set_layout_manager(CLUTTER_ACTOR(self), layout);

	/* Connect signals */
	g_signal_connect_swapped(priv->screen,
								"active-workspace-changed",
								G_CALLBACK(_xfdashboard_windows_view_on_active_workspace_changed),
								self);

	g_signal_connect_swapped(priv->screen,
								"window-opened",
								G_CALLBACK(_xfdashboard_windows_view_on_window_opened),
								self);

	g_signal_connect_swapped(priv->screen,
								"window-closed",
								G_CALLBACK(_xfdashboard_windows_view_on_window_closed),
								self);
}

/* Implementation: Public API */

/* Get active screen */
WnckScreen* xfdashboard_windows_view_get_screen(XfdashboardWindowsView *self)
{
	g_return_val_if_fail(XFDASHBOARD_IS_WINDOWS_VIEW(self), NULL);

	return(self->priv->screen);
}

/* Get active workspace */
WnckWorkspace* xfdashboard_windows_view_get_workspace(XfdashboardWindowsView *self)
{
	g_return_val_if_fail(XFDASHBOARD_IS_WINDOWS_VIEW(self), NULL);

	return(self->priv->workspace);
}
