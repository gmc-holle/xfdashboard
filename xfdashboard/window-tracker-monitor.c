/*
 * window-tracker-monitor: A monitor object tracked by window tracker.
 *                         It provides information about position and
 *                         size of monitor within screen and also a flag
 *                         if this monitor is the primary one.
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

#include "window-tracker-monitor.h"

#include <glib/gi18n-lib.h>
#include <gdk/gdkx.h>

/* Define this class in GObject system */
G_DEFINE_TYPE(XfdashboardWindowTrackerMonitor,
				xfdashboard_window_tracker_monitor,
				G_TYPE_OBJECT)

/* Private structure - access only by public API if needed */
#define XFDASHBOARD_WINDOW_TRACKER_MONITOR_GET_PRIVATE(obj) \
	(G_TYPE_INSTANCE_GET_PRIVATE((obj), XFDASHBOARD_TYPE_WINDOW_TRACKER_MONITOR, XfdashboardWindowTrackerMonitorPrivate))

struct _XfdashboardWindowTrackerMonitorPrivate
{
	/* Properties related */
	gint				monitorIndex;
	gboolean			isPrimary;

	/* Instance related */
	GdkScreen			*screen;
	GdkRectangle		geometry;
};

/* Properties */
enum
{
	PROP_0,

	PROP_MONITOR_INDEX,
	PROP_IS_PRIMARY,

	PROP_LAST
};

static GParamSpec* XfdashboardWindowTrackerMonitorProperties[PROP_LAST]={ 0, };

/* Signals */
enum
{
	SIGNAL_PRIMARY_CHANGED,
	SIGNAL_GEOMETRY_CHANGED,

	SIGNAL_LAST
};

static guint XfdashboardWindowTrackerMonitorSignals[SIGNAL_LAST]={ 0, };


/* IMPLEMENTATION: Private variables and methods */

/* Set primary monitor flag */
static void _xfdashboard_window_tracker_monitor_update_primary(XfdashboardWindowTrackerMonitor *self)
{
	XfdashboardWindowTrackerMonitorPrivate		*priv;
	gint										primaryIndex;
	gboolean									isPrimary;

	g_return_if_fail(XFDASHBOARD_IS_WINDOW_TRACKER_MONITOR(self));

	priv=self->priv;

	/* Get primary flag */
	primaryIndex=gdk_screen_get_primary_monitor(priv->screen);
	if(primaryIndex==priv->monitorIndex) isPrimary=TRUE;
		else isPrimary=FALSE;

	/* Set value if changed */
	if(priv->isPrimary!=isPrimary)
	{
		g_debug("Monitor %d changes primary state from %s to %s",
					priv->monitorIndex,
					isPrimary ? "yes" : "no",
					priv->isPrimary ? "yes" : "no");

		/* Set value */
		priv->isPrimary=isPrimary;

		/* Notify about property change */
		g_object_notify_by_pspec(G_OBJECT(self), XfdashboardWindowTrackerMonitorProperties[PROP_IS_PRIMARY]);

		/* Emit signal */
		g_signal_emit(self, XfdashboardWindowTrackerMonitorSignals[SIGNAL_PRIMARY_CHANGED], 0);
	}
}

/* Update monitor geometry */
static void _xfdashboard_window_tracker_monitor_update_geometry(XfdashboardWindowTrackerMonitor *self)
{
	XfdashboardWindowTrackerMonitorPrivate		*priv;
	GdkRectangle								geometry;

	g_return_if_fail(XFDASHBOARD_IS_WINDOW_TRACKER_MONITOR(self));

	priv=self->priv;

	/* Check if monitor is valid */
	if(priv->monitorIndex>=gdk_screen_get_n_monitors(priv->screen)) return;

	/* Get monitor geometry */
	gdk_screen_get_monitor_geometry(priv->screen, priv->monitorIndex, &geometry);

	/* Set value if changed */
	if(geometry.x!=priv->geometry.x ||
		geometry.y!=priv->geometry.y ||
		geometry.width!=priv->geometry.width ||
		geometry.height!=priv->geometry.height)
	{
		/* Set value */
		priv->geometry.x=geometry.x;
		priv->geometry.y=geometry.y;
		priv->geometry.width=geometry.width;
		priv->geometry.height=geometry.height;

		/* Emit signal */
		g_signal_emit(self, XfdashboardWindowTrackerMonitorSignals[SIGNAL_GEOMETRY_CHANGED], 0);
		g_debug("Monitor %d moved to %d,%d and resized to %dx%d",
					priv->monitorIndex,
					priv->geometry.x, priv->geometry.y,
					priv->geometry.width, priv->geometry.height);
	}
}

/* Set monitor index this object belongs to and to monitor */
static void _xfdashboard_window_tracker_monitor_set_index(XfdashboardWindowTrackerMonitor *self, gint inIndex)
{
	XfdashboardWindowTrackerMonitorPrivate		*priv;

	g_return_if_fail(XFDASHBOARD_IS_WINDOW_TRACKER_MONITOR(self));
	g_return_if_fail(inIndex>=0);
	g_return_if_fail(inIndex<gdk_screen_get_n_monitors(self->priv->screen));

	priv=self->priv;

	/* Freeze notification */
	g_object_freeze_notify(G_OBJECT(self));

	/* Set value if changed */
	if(priv->monitorIndex!=inIndex)
	{
		/* Set value */
		priv->monitorIndex=inIndex;

		/* Update primary monitor flag */
		_xfdashboard_window_tracker_monitor_update_primary(self);

		/* Update geometry of monitor */
		_xfdashboard_window_tracker_monitor_update_geometry(self);
	}

	/* Thaw notification */
	g_object_thaw_notify(G_OBJECT(self));
}

/* Number of monitors, primary monitor or size of any monitor changed */
static void _xfdashboard_window_tracker_monitor_on_monitors_changed(XfdashboardWindowTrackerMonitor *self,
																	gpointer inUserData)
{
	g_return_if_fail(XFDASHBOARD_IS_WINDOW_TRACKER_MONITOR(self));
	g_return_if_fail(GDK_IS_SCREEN(inUserData));

	/* Update primary monitor flag */
	_xfdashboard_window_tracker_monitor_update_primary(self);

	/* Update geometry of monitor */
	_xfdashboard_window_tracker_monitor_update_geometry(self);
}

/* IMPLEMENTATION: GObject */

/* Dispose this object */
static void _xfdashboard_window_tracker_monitor_dispose(GObject *inObject)
{
	XfdashboardWindowTrackerMonitor			*self=XFDASHBOARD_WINDOW_TRACKER_MONITOR(inObject);
	XfdashboardWindowTrackerMonitorPrivate	*priv=self->priv;

	/* Release allocated resources */
	if(priv->screen)
	{
		g_signal_handlers_disconnect_by_data(priv->screen, self);
		priv->screen=NULL;
	}

	/* Call parent's class dispose method */
	G_OBJECT_CLASS(xfdashboard_window_tracker_monitor_parent_class)->dispose(inObject);
}

/* Set/get properties */
static void _xfdashboard_window_tracker_monitor_set_property(GObject *inObject,
																guint inPropID,
																const GValue *inValue,
																GParamSpec *inSpec)
{
	XfdashboardWindowTrackerMonitor			*self=XFDASHBOARD_WINDOW_TRACKER_MONITOR(inObject);

	switch(inPropID)
	{
		case PROP_MONITOR_INDEX:
			_xfdashboard_window_tracker_monitor_set_index(self, g_value_get_int(inValue));
			break;

		default:
			G_OBJECT_WARN_INVALID_PROPERTY_ID(inObject, inPropID, inSpec);
			break;
	}
}

static void _xfdashboard_window_tracker_monitor_get_property(GObject *inObject,
																guint inPropID,
																GValue *outValue,
																GParamSpec *inSpec)
{
	XfdashboardWindowTrackerMonitor			*self=XFDASHBOARD_WINDOW_TRACKER_MONITOR(inObject);
	XfdashboardWindowTrackerMonitorPrivate	*priv=self->priv;

	switch(inPropID)
	{
		case PROP_IS_PRIMARY:
			g_value_set_boolean(outValue, priv->isPrimary);
			break;

		case PROP_MONITOR_INDEX:
			g_value_set_uint(outValue, priv->monitorIndex);
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
static void xfdashboard_window_tracker_monitor_class_init(XfdashboardWindowTrackerMonitorClass *klass)
{
	GObjectClass					*gobjectClass=G_OBJECT_CLASS(klass);

	/* Override functions */
	gobjectClass->dispose=_xfdashboard_window_tracker_monitor_dispose;
	gobjectClass->set_property=_xfdashboard_window_tracker_monitor_set_property;
	gobjectClass->get_property=_xfdashboard_window_tracker_monitor_get_property;

	/* Set up private structure */
	g_type_class_add_private(klass, sizeof(XfdashboardWindowTrackerMonitorPrivate));

	/* Define properties */
	XfdashboardWindowTrackerMonitorProperties[PROP_MONITOR_INDEX]=
		g_param_spec_int("monitor-index",
							_("Monitor index"),
							_("The index of this monitor"),
							0, G_MAXINT,
							0,
							G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS | G_PARAM_CONSTRUCT_ONLY);

	XfdashboardWindowTrackerMonitorProperties[PROP_IS_PRIMARY]=
		g_param_spec_boolean("is-primary",
								_("Is primary"),
								_("Whether this monitor is the primary one"),
								FALSE,
								G_PARAM_READABLE | G_PARAM_STATIC_STRINGS);

	g_object_class_install_properties(gobjectClass, PROP_LAST, XfdashboardWindowTrackerMonitorProperties);

	/* Define signals */
	XfdashboardWindowTrackerMonitorSignals[SIGNAL_PRIMARY_CHANGED]=
		g_signal_new("primary-changed",
						G_TYPE_FROM_CLASS(klass),
						G_SIGNAL_RUN_LAST,
						G_STRUCT_OFFSET(XfdashboardWindowTrackerMonitorClass, primary_changed),
						NULL,
						NULL,
						g_cclosure_marshal_VOID__VOID,
						G_TYPE_NONE,
						0);

	XfdashboardWindowTrackerMonitorSignals[SIGNAL_GEOMETRY_CHANGED]=
		g_signal_new("geometry-changed",
						G_TYPE_FROM_CLASS(klass),
						G_SIGNAL_RUN_LAST,
						G_STRUCT_OFFSET(XfdashboardWindowTrackerMonitorClass, geometry_changed),
						NULL,
						NULL,
						g_cclosure_marshal_VOID__VOID,
						G_TYPE_NONE,
						0);
}

/* Object initialization
 * Create private structure and set up default values
 */
static void xfdashboard_window_tracker_monitor_init(XfdashboardWindowTrackerMonitor *self)
{
	XfdashboardWindowTrackerMonitorPrivate		*priv;

	priv=self->priv=XFDASHBOARD_WINDOW_TRACKER_MONITOR_GET_PRIVATE(self);

	/* Set default values */
	priv->monitorIndex=0;
	priv->isPrimary=FALSE;
	priv->screen=gdk_screen_get_default();
	priv->geometry.x=0;
	priv->geometry.y=0;
	priv->geometry.width=0;
	priv->geometry.height=0;

	/* Get initial primary monitor flag */
	_xfdashboard_window_tracker_monitor_update_primary(self);

	/* Get initial geometry of monitor */
	_xfdashboard_window_tracker_monitor_update_geometry(self);

	/* Connect signals */
	g_signal_connect_swapped(priv->screen, "monitors-changed", G_CALLBACK(_xfdashboard_window_tracker_monitor_on_monitors_changed), self);
}

/* IMPLEMENTATION: Public API */

/* Get monitor index */
gint xfdashboard_window_tracker_monitor_get_number(XfdashboardWindowTrackerMonitor *self)
{
	g_return_val_if_fail(XFDASHBOARD_IS_WINDOW_TRACKER_MONITOR(self), 0);

	return(self->priv->monitorIndex);
}

/* Determine if monitor is primary one */
gboolean xfdashboard_window_tracker_monitor_is_primary(XfdashboardWindowTrackerMonitor *self)
{
	g_return_val_if_fail(XFDASHBOARD_IS_WINDOW_TRACKER_MONITOR(self), FALSE);

	return(self->priv->isPrimary);
}

/* Get geometry of monitor */
gint xfdashboard_window_tracker_monitor_get_x(XfdashboardWindowTrackerMonitor *self)
{
	g_return_val_if_fail(XFDASHBOARD_IS_WINDOW_TRACKER_MONITOR(self), 0);

	return(self->priv->geometry.x);
}

gint xfdashboard_window_tracker_monitor_get_y(XfdashboardWindowTrackerMonitor *self)
{
	g_return_val_if_fail(XFDASHBOARD_IS_WINDOW_TRACKER_MONITOR(self), 0);

	return(self->priv->geometry.y);
}

gint xfdashboard_window_tracker_monitor_get_width(XfdashboardWindowTrackerMonitor *self)
{
	g_return_val_if_fail(XFDASHBOARD_IS_WINDOW_TRACKER_MONITOR(self), 0);

	return(self->priv->geometry.width);
}

gint xfdashboard_window_tracker_monitor_get_height(XfdashboardWindowTrackerMonitor *self)
{
	g_return_val_if_fail(XFDASHBOARD_IS_WINDOW_TRACKER_MONITOR(self), 0);

	return(self->priv->geometry.height);
}

void xfdashboard_window_tracker_monitor_get_geometry(XfdashboardWindowTrackerMonitor *self,
														gint *outX,
														gint *outY,
														gint *outWidth,
														gint *outHeight)
{
	XfdashboardWindowTrackerMonitorPrivate		*priv;

	g_return_if_fail(XFDASHBOARD_IS_WINDOW_TRACKER_MONITOR(self));

	priv=self->priv;

	/* Set position and size of monitor */
	if(outX) *outX=priv->geometry.x;
	if(outY) *outY=priv->geometry.y;
	if(outWidth) *outWidth=priv->geometry.width;
	if(outHeight) *outHeight=priv->geometry.height;
}
