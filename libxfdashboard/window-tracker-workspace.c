/*
 * window-tracker-workspace: A workspace tracked by window tracker.
 * 
 * Copyright 2012-2016 Stephan Haller <nomad@froevel.de>
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

#include <libxfdashboard/window-tracker-workspace.h>

#include <glib/gi18n-lib.h>

#include <libxfdashboard/window-tracker.h>
#include <libxfdashboard/marshal.h>
#include <libxfdashboard/compat.h>


/* Define this class in GObject system */
G_DEFINE_INTERFACE(XfdashboardWindowTrackerWorkspace,
					xfdashboard_window_tracker_workspace,
					G_TYPE_OBJECT)


/* Signals */
enum
{
	SIGNAL_NAME_CHANGED,

	SIGNAL_LAST
};

static guint XfdashboardWindowTrackerWorkspaceSignals[SIGNAL_LAST]={ 0, };


/* IMPLEMENTATION: Private variables and methods */
#define XFDASHBOARD_WINDOWS_TRACKER_WORKSPACE_WARN_NOT_IMPLEMENTED(self, vfunc)\
	g_warning("Object of type %s does not implement required virtual function XfdashboardWindowTrackerWorkspace::%s",\
				G_OBJECT_TYPE_NAME(self), \
				vfunc);

/* Default implementation of virtual function "is_equal" */
static gboolean _xfdashboard_window_tracker_workspace_real_is_equal(XfdashboardWindowTrackerWorkspace *inLeft,
																	XfdashboardWindowTrackerWorkspace *inRight)
{
	gint			leftIndex, rightIndex;

	g_return_val_if_fail(XFDASHBOARD_IS_WINDOW_TRACKER_WORKSPACE(inLeft), FALSE);
	g_return_val_if_fail(XFDASHBOARD_IS_WINDOW_TRACKER_WORKSPACE(inRight), FALSE);

	/* Check if both are the same workspace or refer to same one */
	leftIndex=xfdashboard_window_tracker_workspace_get_number(inLeft);
	rightIndex=xfdashboard_window_tracker_workspace_get_number(inRight);
	if(inLeft==inRight || leftIndex==rightIndex) return(TRUE);

	/* If we get here then they cannot be considered equal */
	return(FALSE);
}


/* IMPLEMENTATION: GObject */

/* Interface initialization
 * Set up default functions
 */
static void xfdashboard_window_tracker_workspace_default_init(XfdashboardWindowTrackerWorkspaceInterface *iface)
{
	static gboolean		initialized=FALSE;

	/* The following virtual functions should be overriden if default
	 * implementation does not fit.
	 */
	iface->is_equal=_xfdashboard_window_tracker_workspace_real_is_equal;

	/* Define signals */
	if(!initialized)
	{
		XfdashboardWindowTrackerWorkspaceSignals[SIGNAL_NAME_CHANGED]=
			g_signal_new("name-changed",
							G_TYPE_FROM_INTERFACE(iface),
							G_SIGNAL_RUN_LAST,
							G_STRUCT_OFFSET(XfdashboardWindowTrackerWorkspaceInterface, name_changed),
							NULL,
							NULL,
							g_cclosure_marshal_VOID__VOID,
							G_TYPE_NONE,
							0);

		/* Set flag that base initialization was done for this interface */
		initialized=TRUE;
	}
}


/* IMPLEMENTATION: Public API */

/* Check if both workspaces are the same */
gboolean xfdashboard_window_tracker_workspace_is_equal(XfdashboardWindowTrackerWorkspace *inLeft,
														XfdashboardWindowTrackerWorkspace *inRight)
{
	XfdashboardWindowTrackerWorkspaceInterface		*iface;

	g_return_val_if_fail(XFDASHBOARD_IS_WINDOW_TRACKER_WORKSPACE(inLeft), FALSE);
	g_return_val_if_fail(XFDASHBOARD_IS_WINDOW_TRACKER_WORKSPACE(inRight), FALSE);

	iface=XFDASHBOARD_WINDOW_TRACKER_WORKSPACE_GET_IFACE(inLeft);

	/* Call virtual function */
	if(iface->is_equal)
	{
		return(iface->is_equal(inLeft, inRight));
	}

	/* If we get here the virtual function was not overridden */
	XFDASHBOARD_WINDOWS_TRACKER_WORKSPACE_WARN_NOT_IMPLEMENTED(inLeft, "is_equal");
	return(FALSE);
}

/* Get number of workspace */
gint xfdashboard_window_tracker_workspace_get_number(XfdashboardWindowTrackerWorkspace *self)
{
	XfdashboardWindowTrackerWorkspaceInterface		*iface;

	g_return_val_if_fail(XFDASHBOARD_IS_WINDOW_TRACKER_WORKSPACE(self), 0);

	iface=XFDASHBOARD_WINDOW_TRACKER_WORKSPACE_GET_IFACE(self);

	/* Call virtual function */
	if(iface->get_number)
	{
		return(iface->get_number(self));
	}

	/* If we get here the virtual function was not overridden */
	XFDASHBOARD_WINDOWS_TRACKER_WORKSPACE_WARN_NOT_IMPLEMENTED(self, "get_number");
	return(0);
}

/* Get name of workspace */
const gchar* xfdashboard_window_tracker_workspace_get_name(XfdashboardWindowTrackerWorkspace *self)
{
	XfdashboardWindowTrackerWorkspaceInterface		*iface;

	g_return_val_if_fail(XFDASHBOARD_IS_WINDOW_TRACKER_WORKSPACE(self), NULL);

	iface=XFDASHBOARD_WINDOW_TRACKER_WORKSPACE_GET_IFACE(self);

	/* Call virtual function */
	if(iface->get_name)
	{
		return(iface->get_name(self));
	}

	/* If we get here the virtual function was not overridden */
	XFDASHBOARD_WINDOWS_TRACKER_WORKSPACE_WARN_NOT_IMPLEMENTED(self, "get_name");
	return(NULL);
}

/* Get size of workspace */
void xfdashboard_window_tracker_workspace_get_size(XfdashboardWindowTrackerWorkspace *self,
													gint *outWidth,
													gint *outHeight)
{
	XfdashboardWindowTrackerWorkspaceInterface		*iface;
	gint											width, height;

	g_return_if_fail(XFDASHBOARD_IS_WINDOW_TRACKER_WORKSPACE(self));

	iface=XFDASHBOARD_WINDOW_TRACKER_WORKSPACE_GET_IFACE(self);

	/* Call virtual function */
	if(iface->get_size)
	{
		/* Get geometry of workspace */
		iface->get_size(self, &width, &height);

		/* Store result */
		if(outWidth) *outWidth=width;
		if(outHeight) *outHeight=height;

		return;
	}

	/* If we get here the virtual function was not overridden */
	XFDASHBOARD_WINDOWS_TRACKER_WORKSPACE_WARN_NOT_IMPLEMENTED(self, "get_size");
}

/* Determine if requested workspace is the active one */
gint xfdashboard_window_tracker_workspace_is_active(XfdashboardWindowTrackerWorkspace *self)
{
	XfdashboardWindowTrackerWorkspaceInterface		*iface;

	g_return_val_if_fail(XFDASHBOARD_IS_WINDOW_TRACKER_WORKSPACE(self), FALSE);

	iface=XFDASHBOARD_WINDOW_TRACKER_WORKSPACE_GET_IFACE(self);

	/* Call virtual function */
	if(iface->is_active)
	{
		return(iface->is_active(self));
	}

	/* If we get here the virtual function was not overridden */
	XFDASHBOARD_WINDOWS_TRACKER_WORKSPACE_WARN_NOT_IMPLEMENTED(self, "is_active");
	return(FALSE);
}

/* Activate workspace */
void xfdashboard_window_tracker_workspace_activate(XfdashboardWindowTrackerWorkspace *self)
{
	XfdashboardWindowTrackerWorkspaceInterface		*iface;

	g_return_if_fail(XFDASHBOARD_IS_WINDOW_TRACKER_WORKSPACE(self));

	iface=XFDASHBOARD_WINDOW_TRACKER_WORKSPACE_GET_IFACE(self);

	/* Call virtual function */
	if(iface->activate)
	{
		iface->activate(self);
		return;
	}

	/* If we get here the virtual function was not overridden */
	XFDASHBOARD_WINDOWS_TRACKER_WORKSPACE_WARN_NOT_IMPLEMENTED(self, "activate");
}
