/*
 * live-workspace: An actor showing the content of a workspace which will
 *                 be updated if changed.
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

#ifndef __XFOVERVIEW_LIVE_WORKSPACE__
#define __XFOVERVIEW_LIVE_WORKSPACE__

#include <clutter/clutter.h>

#include "background.h"
#include "window-tracker-workspace.h"

G_BEGIN_DECLS

#define XFDASHBOARD_TYPE_LIVE_WORKSPACE				(xfdashboard_live_workspace_get_type())
#define XFDASHBOARD_LIVE_WORKSPACE(obj)				(G_TYPE_CHECK_INSTANCE_CAST((obj), XFDASHBOARD_TYPE_LIVE_WORKSPACE, XfdashboardLiveWorkspace))
#define XFDASHBOARD_IS_LIVE_WORKSPACE(obj)			(G_TYPE_CHECK_INSTANCE_TYPE((obj), XFDASHBOARD_TYPE_LIVE_WORKSPACE))
#define XFDASHBOARD_LIVE_WORKSPACE_CLASS(klass)		(G_TYPE_CHECK_CLASS_CAST((klass), XFDASHBOARD_TYPE_LIVE_WORKSPACE, XfdashboardLiveWorkspaceClass))
#define XFDASHBOARD_IS_LIVE_WORKSPACE_CLASS(klass)	(G_TYPE_CHECK_CLASS_TYPE((klass), XFDASHBOARD_TYPE_LIVE_WORKSPACE))
#define XFDASHBOARD_LIVE_WORKSPACE_GET_CLASS(obj)	(G_TYPE_INSTANCE_GET_CLASS((obj), XFDASHBOARD_TYPE_LIVE_WORKSPACE, XfdashboardLiveWorkspaceClass))

typedef struct _XfdashboardLiveWorkspace			XfdashboardLiveWorkspace;
typedef struct _XfdashboardLiveWorkspaceClass		XfdashboardLiveWorkspaceClass;
typedef struct _XfdashboardLiveWorkspacePrivate		XfdashboardLiveWorkspacePrivate;

struct _XfdashboardLiveWorkspace
{
	/* Parent instance */
	XfdashboardBackground			parent_instance;

	/* Private structure */
	XfdashboardLiveWorkspacePrivate	*priv;
};

struct _XfdashboardLiveWorkspaceClass
{
	/*< private >*/
	/* Parent class */
	XfdashboardBackgroundClass		parent_class;

	/*< public >*/
	/* Virtual functions */
	void (*clicked)(XfdashboardLiveWorkspace *self);
};

/* Public API */
GType xfdashboard_live_workspace_get_type(void) G_GNUC_CONST;

ClutterActor* xfdashboard_live_workspace_new(void);
ClutterActor* xfdashboard_live_workspace_new_for_workspace(XfdashboardWindowTrackerWorkspace *inWorkspace);

XfdashboardWindowTrackerWorkspace* xfdashboard_live_workspace_get_workspace(XfdashboardLiveWorkspace *self);
void xfdashboard_live_workspace_set_workspace(XfdashboardLiveWorkspace *self, XfdashboardWindowTrackerWorkspace *inWorkspace);

G_END_DECLS

#endif	/* __XFOVERVIEW_LIVE_WORKSPACE__ */
