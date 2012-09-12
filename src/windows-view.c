/*
 * windows-view.c: A view showing all visible windows
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

#include "windows-view.h"
#include "live-window.h"
#include "scaling-flow-layout.h"
#include "main.h"

/* Define this class in GObject system */
G_DEFINE_TYPE(XfdashboardWindowsView,
				xfdashboard_windows_view,
				XFDASHBOARD_TYPE_VIEW)

/* Private structure - access only by public API if needed */
#define XFDASHBOARD_WINDOWS_VIEW_GET_PRIVATE(obj) \
	(G_TYPE_INSTANCE_GET_PRIVATE((obj), XFDASHBOARD_TYPE_WINDOWS_VIEW, XfdashboardWindowsViewPrivate))

struct _XfdashboardWindowsViewPrivate
{
	/* Active workspace */
	WnckScreen		*screen;
	WnckWorkspace	*workspace;
	gint			signalWorkspaceChangedID;
};

/* Properties */
enum
{
	PROP_0,

	PROP_WORKSPACE,
	
	PROP_LAST
};

static GParamSpec* XfdashboardWindowsViewProperties[PROP_LAST]={ 0, };

/* IMPLEMENTATION: Private variables and methods */

void _set_active_workspace(XfdashboardWindowsView *self, WnckWorkspace *inWorkspace);

/* Workspace was changed */
static void _on_workspace_changed(WnckScreen *inScreen, WnckWorkspace *inPrevWorkspace, gpointer inUserData)
{
	g_return_if_fail(XFDASHBOARD_IS_WINDOWS_VIEW(inUserData));

	XfdashboardWindowsView	*self=XFDASHBOARD_WINDOWS_VIEW(inUserData);
	WnckWorkspace			*workspace;
	
	/* Get new active workspace */
	workspace=wnck_screen_get_active_workspace(inScreen);

	/* Move clutter stage to new active workspace and make active */
	wnck_window_move_to_workspace(xfdashboard_getAppWindow(), workspace);
	wnck_window_activate(xfdashboard_getAppWindow(), CLUTTER_CURRENT_TIME);
	
	/* Update window list */
	_set_active_workspace(self, workspace);
}

/* A live window was clicked */
static gboolean _on_window_clicked(ClutterActor *inActor, gpointer inUserData)
{
	g_return_val_if_fail(XFDASHBOARD_IS_LIVE_WINDOW(inActor), FALSE);

	XfdashboardLiveWindow	*liveWindow=XFDASHBOARD_LIVE_WINDOW(inActor);
	
	wnck_window_activate_transient((WnckWindow*)xfdashboard_live_window_get_window(liveWindow), CLUTTER_CURRENT_TIME);

	clutter_main_quit();

	return(TRUE);
}

/* Set active screen */
void _set_active_workspace(XfdashboardWindowsView *self, WnckWorkspace *inWorkspace)
{
	XfdashboardWindowsViewPrivate	*priv=XFDASHBOARD_WINDOWS_VIEW(self)->priv;
	WnckScreen						*screen=NULL;
	GList							*windowsList;
	
	/* Do not anything if workspace is the same as before */
	if(inWorkspace==priv->workspace) return;

	/* Remove all live window actors */
	xfdashboard_view_remove_all(XFDASHBOARD_VIEW(self));

	/* Set new workspace */
	priv->workspace=inWorkspace;

	/* Set new screen (if it differs from current one) */
	if(inWorkspace!=NULL) screen=wnck_workspace_get_screen(priv->workspace);

	if(priv->screen!=screen)
	{
		if(priv->screen!=NULL)
		{
			if(priv->signalWorkspaceChangedID!=0) g_signal_handler_disconnect(priv->screen, priv->signalWorkspaceChangedID);
			priv->signalWorkspaceChangedID=0;
			priv->screen=NULL;
		}

		if(screen!=NULL)
		{
			priv->screen=screen;
			priv->signalWorkspaceChangedID=g_signal_connect(priv->screen,
															"active-workspace-changed",
															G_CALLBACK(_on_workspace_changed),
															self);
		}
	}

	/* Create live window actors for new workspace */
	if(priv->workspace==NULL) return;

	windowsList=wnck_screen_get_windows_stacked(self->priv->screen);
	for(windowsList=g_list_last(windowsList); windowsList!=NULL; windowsList=windowsList->prev)
	{
		WnckWindow		*window=WNCK_WINDOW(windowsList->data);
		ClutterActor	*liveWindow;
		
		/* Window must be on workspace and must not be flagged to skip tasklist */
		if(wnck_window_is_on_workspace(window, priv->workspace) &&
			!wnck_window_is_skip_tasklist(window))
		{
			/* Create actor */
			liveWindow=xfdashboard_live_window_new(window);
			clutter_container_add_actor(CLUTTER_CONTAINER(self), liveWindow);
			g_signal_connect(liveWindow, "clicked", G_CALLBACK(_on_window_clicked), NULL);
		}
	}
}

/* IMPLEMENTATION: GObject */

/* Dispose this object */
static void xfdashboard_windows_view_dispose(GObject *inObject)
{
	XfdashboardWindowsView		*self=XFDASHBOARD_WINDOWS_VIEW(inObject);

	/* Release allocated resources */
	_set_active_workspace(self, NULL);

	/* Call parent's class dispose method */
	G_OBJECT_CLASS(xfdashboard_windows_view_parent_class)->dispose(inObject);
}

/* Set/get properties */
static void xfdashboard_windows_view_set_property(GObject *inObject,
													guint inPropID,
													const GValue *inValue,
													GParamSpec *inSpec)
{
	XfdashboardWindowsView		*self=XFDASHBOARD_WINDOWS_VIEW(inObject);
	
	switch(inPropID)
	{
		case PROP_WORKSPACE:
			_set_active_workspace(self, g_value_get_object(inValue));
			break;

		default:
			G_OBJECT_WARN_INVALID_PROPERTY_ID(inObject, inPropID, inSpec);
			break;
	}
}

static void xfdashboard_windows_view_get_property(GObject *inObject,
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
static void xfdashboard_windows_view_class_init(XfdashboardWindowsViewClass *klass)
{
	GObjectClass			*gobjectClass=G_OBJECT_CLASS(klass);

	/* Override functions */
	gobjectClass->dispose=xfdashboard_windows_view_dispose;
	gobjectClass->set_property=xfdashboard_windows_view_set_property;
	gobjectClass->get_property=xfdashboard_windows_view_get_property;

	/* Set up private structure */
	g_type_class_add_private(klass, sizeof(XfdashboardWindowsViewPrivate));

	/* Define properties */
	XfdashboardWindowsViewProperties[PROP_WORKSPACE]=
		g_param_spec_object ("workspace",
								"Current workspace",
								"The current workspace which windows are shown",
								WNCK_TYPE_WORKSPACE,
								G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY);

	g_object_class_install_properties(gobjectClass, PROP_LAST, XfdashboardWindowsViewProperties);
}

/* Object initialization
 * Create private structure and set up default values
 */
static void xfdashboard_windows_view_init(XfdashboardWindowsView *self)
{
	self->priv=XFDASHBOARD_WINDOWS_VIEW_GET_PRIVATE(self);

	/* Set up default values */
	self->priv->screen=NULL;
	self->priv->workspace=NULL;
	self->priv->signalWorkspaceChangedID=0;
}

/* Implementation: Public API */

/* Create new object */
ClutterActor* xfdashboard_windows_view_new()
{
	WnckScreen				*screen;
	WnckWorkspace			*workspace;
	ClutterLayoutManager	*layout;

	/* Get current screen */
	screen=wnck_screen_get_default();
	
	/* Get current workspace but we need to force an update before */
	wnck_screen_force_update(screen);
	workspace=wnck_screen_get_active_workspace(screen);

	/* Create layout manager used in this view */
	layout=xfdashboard_scaling_flow_layout_new();
	xfdashboard_scaling_flow_layout_set_spacing(XFDASHBOARD_SCALING_FLOW_LAYOUT(layout), 8.0f);
	
	/* Create object */
	return(g_object_new(XFDASHBOARD_TYPE_WINDOWS_VIEW,
						"layout-manager", layout,
						"workspace", workspace,
						NULL));
}

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
