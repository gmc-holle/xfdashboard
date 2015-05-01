/*
 * live-workspace-window: An actor used at workspace actor showing
 *                        a window and its content or its window icon
 *                        
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

#ifndef __XFDASHBOARD_LIVE_WORKSPACE_WINDOW__
#define __XFDASHBOARD_LIVE_WORKSPACE_WINDOW__

#include <clutter/clutter.h>

#include "background.h"
#include "types.h"
#include "window-tracker-window.h"

G_BEGIN_DECLS

#define XFDASHBOARD_TYPE_LIVE_WORKSPACE_WINDOW					(xfdashboard_live_workspace_window_get_type())
#define XFDASHBOARD_LIVE_WORKSPACE_WINDOW(obj)					(G_TYPE_CHECK_INSTANCE_CAST((obj), XFDASHBOARD_TYPE_LIVE_WORKSPACE_WINDOW, XfdashboardLiveWorkspaceWindow))
#define XFDASHBOARD_IS_LIVE_WORKSPACE_WINDOW(obj)				(G_TYPE_CHECK_INSTANCE_TYPE((obj), XFDASHBOARD_TYPE_LIVE_WORKSPACE_WINDOW))
#define XFDASHBOARD_LIVE_WORKSPACE_WINDOW_CLASS(klass)			(G_TYPE_CHECK_CLASS_CAST((klass), XFDASHBOARD_TYPE_LIVE_WORKSPACE_WINDOW, XfdashboardLiveWorkspaceWindowClass))
#define XFDASHBOARD_IS_LIVE_WORKSPACE_WINDOW_CLASS(klass)		(G_TYPE_CHECK_CLASS_TYPE((klass), XFDASHBOARD_TYPE_LIVE_WORKSPACE_WINDOW))
#define XFDASHBOARD_LIVE_WORKSPACE_WINDOW_GET_CLASS(obj)		(G_TYPE_INSTANCE_GET_CLASS((obj), XFDASHBOARD_TYPE_LIVE_WORKSPACE_WINDOW, XfdashboardLiveWorkspaceWindowClass))

typedef struct _XfdashboardLiveWorkspaceWindow					XfdashboardLiveWorkspaceWindow;
typedef struct _XfdashboardLiveWorkspaceWindowClass				XfdashboardLiveWorkspaceWindowClass;
typedef struct _XfdashboardLiveWorkspaceWindowPrivate			XfdashboardLiveWorkspaceWindowPrivate;

struct _XfdashboardLiveWorkspaceWindow
{
	/* Parent instance */
	XfdashboardBackground					parent_instance;

	/* Private structure */
	XfdashboardLiveWorkspaceWindowPrivate	*priv;
};

struct _XfdashboardLiveWorkspaceWindowClass
{
	/*< private >*/
	/* Parent class */
	XfdashboardBackgroundClass				parent_class;

	/*< public >*/
	/* Virtual functions */
};

/* Public API */
GType xfdashboard_live_workspace_window_get_type(void) G_GNUC_CONST;

ClutterActor* xfdashboard_live_workspace_window_new(void);
ClutterActor* xfdashboard_live_workspace_window_new_for_window(XfdashboardWindowTrackerWindow *inWindow);

XfdashboardWindowTrackerWindow* xfdashboard_live_workspace_window_get_window(XfdashboardLiveWorkspaceWindow *self);
void xfdashboard_live_workspace_window_set_window(XfdashboardLiveWorkspaceWindow *self, XfdashboardWindowTrackerWindow *inWindow);

gboolean xfdashboard_live_workspace_window_get_show_window_content(XfdashboardLiveWorkspaceWindow *self);
void xfdashboard_live_workspace_window_set_show_window_content(XfdashboardLiveWorkspaceWindow *self, gboolean inShowWindowContent);

gboolean xfdashboard_live_workspace_window_get_window_icon_fill_keep_aspect(XfdashboardLiveWorkspaceWindow *self);
void xfdashboard_live_workspace_window_set_window_icon_fill_keep_aspect(XfdashboardLiveWorkspaceWindow *self, const gboolean inKeepAspect);

gboolean xfdashboard_live_workspace_window_get_window_icon_x_fill(XfdashboardLiveWorkspaceWindow *self);
void xfdashboard_live_workspace_window_set_window_icon_x_fill(XfdashboardLiveWorkspaceWindow *self, const gboolean inFill);

gboolean xfdashboard_live_workspace_window_get_window_icon_y_fill(XfdashboardLiveWorkspaceWindow *self);
void xfdashboard_live_workspace_window_set_window_icon_y_fill(XfdashboardLiveWorkspaceWindow *self, const gboolean inFill);

gfloat xfdashboard_live_workspace_window_get_window_icon_x_align(XfdashboardLiveWorkspaceWindow *self);
void xfdashboard_live_workspace_window_set_window_icon_x_align(XfdashboardLiveWorkspaceWindow *self, const gfloat inAlign);

gfloat xfdashboard_live_workspace_window_get_window_icon_y_align(XfdashboardLiveWorkspaceWindow *self);
void xfdashboard_live_workspace_window_set_window_icon_y_align(XfdashboardLiveWorkspaceWindow *self, const gfloat inAlign);

gfloat xfdashboard_live_workspace_window_get_window_icon_x_scale(XfdashboardLiveWorkspaceWindow *self);
void xfdashboard_live_workspace_window_set_window_icon_x_scale(XfdashboardLiveWorkspaceWindow *self, const gfloat inScale);

gfloat xfdashboard_live_workspace_window_get_window_icon_y_scale(XfdashboardLiveWorkspaceWindow *self);
void xfdashboard_live_workspace_window_set_window_icon_y_scale(XfdashboardLiveWorkspaceWindow *self, const gfloat inScale);

XfdashboardAnchorPoint xfdashboard_live_workspace_window_get_window_icon_anchor_point(XfdashboardLiveWorkspaceWindow *self);
void xfdashboard_live_workspace_window_set_window_icon_anchor_point(XfdashboardLiveWorkspaceWindow *self, const XfdashboardAnchorPoint inAnchorPoint);

G_END_DECLS

#endif	/* __XFDASHBOARD_LIVE_WORKSPACE_WINDOW__ */
