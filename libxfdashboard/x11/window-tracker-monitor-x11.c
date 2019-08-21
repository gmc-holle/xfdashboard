/*
 * window-tracker-monitor: A monitor object tracked by window tracker.
 *                         It provides information about position and
 *                         size of monitor within screen and also a flag
 *                         if this monitor is the primary one.
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

#include <libxfdashboard/x11/window-tracker-monitor-x11.h>

#include <glib/gi18n-lib.h>
#include <gtk/gtk.h>
#include <gdk/gdkx.h>

#include <libxfdashboard/window-tracker-monitor.h>
#include <libxfdashboard/compat.h>
#include <libxfdashboard/debug.h>


/* Define this class in GObject system */
static void _xfdashboard_window_tracker_monitor_x11_x11_window_tracker_monitor_iface_init(XfdashboardWindowTrackerMonitorInterface *iface);

struct _XfdashboardWindowTrackerMonitorX11Private
{
	/* Properties related */
	gint				monitorIndex;
	gboolean			isPrimary;

	/* Instance related */
	GdkScreen			*screen;
	GdkRectangle		geometry;
};

G_DEFINE_TYPE_WITH_CODE(XfdashboardWindowTrackerMonitorX11,
						xfdashboard_window_tracker_monitor_x11,
						G_TYPE_OBJECT,
						G_ADD_PRIVATE(XfdashboardWindowTrackerMonitorX11)
						G_IMPLEMENT_INTERFACE(XFDASHBOARD_TYPE_WINDOW_TRACKER_MONITOR, _xfdashboard_window_tracker_monitor_x11_x11_window_tracker_monitor_iface_init))

/* Properties */
enum
{
	PROP_0,

	/* Overriden properties of interface: XfdashboardWindowTrackerMonitor */
	PROP_MONITOR_INDEX,
	PROP_IS_PRIMARY,

	PROP_LAST
};

static GParamSpec* XfdashboardWindowTrackerMonitorX11Properties[PROP_LAST]={ 0, };


/* IMPLEMENTATION: Private variables and methods */

/* Set primary monitor flag */
static void _xfdashboard_window_tracker_monitor_x11_update_primary(XfdashboardWindowTrackerMonitorX11 *self)
{
	XfdashboardWindowTrackerMonitorX11Private	*priv;
	gboolean									isPrimary;
#if GTK_CHECK_VERSION(3, 22, 0)
	GdkMonitor									*primaryMonitor;
#else
	gint										primaryMonitor;
#endif

	g_return_if_fail(XFDASHBOARD_IS_WINDOW_TRACKER_MONITOR(self));
	g_return_if_fail(self->priv->monitorIndex>=0);

	priv=self->priv;

	/* Get primary flag */
#if GTK_CHECK_VERSION(3, 22, 0)
	primaryMonitor=gdk_display_get_monitor(gdk_screen_get_display(priv->screen), priv->monitorIndex);
	isPrimary=gdk_monitor_is_primary(primaryMonitor);
#else
	primaryMonitor=gdk_screen_get_primary_monitor(priv->screen);
	if(primaryMonitor==priv->monitorIndex) isPrimary=TRUE;
		else isPrimary=FALSE;
#endif

	/* Set value if changed */
	if(priv->isPrimary!=isPrimary)
	{
		XFDASHBOARD_DEBUG(self, WINDOWS,
							"Monitor %d changes primary state from %s to %s",
							priv->monitorIndex,
							priv->isPrimary ? "yes" : "no",
							isPrimary ? "yes" : "no");

		/* Set value */
		priv->isPrimary=isPrimary;

		/* Notify about property change */
		g_object_notify_by_pspec(G_OBJECT(self), XfdashboardWindowTrackerMonitorX11Properties[PROP_IS_PRIMARY]);

		/* Emit signal */
		g_signal_emit_by_name(self, "primary-changed");
	}
}

/* Update monitor geometry */
static void _xfdashboard_window_tracker_monitor_x11_update_geometry(XfdashboardWindowTrackerMonitorX11 *self)
{
	XfdashboardWindowTrackerMonitorX11Private	*priv;
	GdkRectangle								geometry;
	gint										numberMonitors;
#if GTK_CHECK_VERSION(3, 22, 0)
	GdkDisplay									*display;
	GdkMonitor									*monitor;
#endif

	g_return_if_fail(XFDASHBOARD_IS_WINDOW_TRACKER_MONITOR_X11(self));
	g_return_if_fail(self->priv->monitorIndex>=0);

	priv=self->priv;

	/* Get number of monitors */
#if GTK_CHECK_VERSION(3, 22, 0)
	display=gdk_screen_get_display(priv->screen);
	numberMonitors=gdk_display_get_n_monitors(display);
#else
	numberMonitors=gdk_screen_get_n_monitors(priv->screen);
#endif

	/* Check if monitor is valid */
	if(priv->monitorIndex>=numberMonitors) return;

	/* Get monitor geometry */
#if GTK_CHECK_VERSION(3, 22, 0)
	monitor=gdk_display_get_monitor(display, priv->monitorIndex);
	gdk_monitor_get_geometry(monitor, &geometry);
#else
	gdk_screen_get_monitor_geometry(priv->screen, priv->monitorIndex, &geometry);
#endif

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
		g_signal_emit_by_name(self, "geometry-changed");
		XFDASHBOARD_DEBUG(self, WINDOWS,
							"Monitor %d moved to %d,%d and resized to %dx%d",
							priv->monitorIndex,
							priv->geometry.x, priv->geometry.y,
							priv->geometry.width, priv->geometry.height);
	}
}

/* Number of monitors, primary monitor or size of any monitor changed */
static void _xfdashboard_window_tracker_monitor_x11_on_monitors_changed(XfdashboardWindowTrackerMonitorX11 *self,
																		gpointer inUserData)
{
	g_return_if_fail(XFDASHBOARD_IS_WINDOW_TRACKER_MONITOR_X11(self));
	g_return_if_fail(GDK_IS_SCREEN(inUserData));

	/* Update primary monitor flag */
	_xfdashboard_window_tracker_monitor_x11_update_primary(self);

	/* Update geometry of monitor */
	_xfdashboard_window_tracker_monitor_x11_update_geometry(self);
}

/* Set monitor index this object belongs to and to monitor */
static void _xfdashboard_window_tracker_monitor_x11_set_index(XfdashboardWindowTrackerMonitorX11 *self,
																gint inIndex)
{
	XfdashboardWindowTrackerMonitorX11Private		*priv;
	gint											numberMonitors;

	g_return_if_fail(XFDASHBOARD_IS_WINDOW_TRACKER_MONITOR_X11(self));
	g_return_if_fail(inIndex>=0);

	priv=self->priv;

	/* Get number of monitors */
#if GTK_CHECK_VERSION(3, 22, 0)
	numberMonitors=gdk_display_get_n_monitors(gdk_screen_get_display(priv->screen));
#else
	numberMonitors=gdk_screen_get_n_monitors(priv->screen);
#endif
	g_return_if_fail(inIndex<numberMonitors);

	/* Freeze notification */
	g_object_freeze_notify(G_OBJECT(self));

	/* Set value if changed */
	if(priv->monitorIndex!=inIndex)
	{
		/* Set value */
		priv->monitorIndex=inIndex;

		/* Update primary monitor flag */
		_xfdashboard_window_tracker_monitor_x11_update_primary(self);

		/* Update geometry of monitor */
		_xfdashboard_window_tracker_monitor_x11_update_geometry(self);

		/* Connect signals now we have a valid monitor index set */
		g_signal_connect_swapped(priv->screen, "monitors-changed", G_CALLBACK(_xfdashboard_window_tracker_monitor_x11_on_monitors_changed), self);

		/* Notify about property change */
		g_object_notify_by_pspec(G_OBJECT(self), XfdashboardWindowTrackerMonitorX11Properties[PROP_MONITOR_INDEX]);
	}

	/* Thaw notification */
	g_object_thaw_notify(G_OBJECT(self));
}


/* IMPLEMENTATION: Interface XfdashboardWindowTrackerMonitor */

/* Determine if monitor is primary one */
static gboolean _xfdashboard_window_tracker_monitor_x11_x11_window_tracker_monitor_is_primary(XfdashboardWindowTrackerMonitor *inMonitor)
{
	XfdashboardWindowTrackerMonitorX11			*self;
	XfdashboardWindowTrackerMonitorX11Private	*priv;

	g_return_val_if_fail(XFDASHBOARD_IS_WINDOW_TRACKER_MONITOR_X11(inMonitor), FALSE);

	self=XFDASHBOARD_WINDOW_TRACKER_MONITOR_X11(inMonitor);
	priv=self->priv;

	return(priv->isPrimary);
}

/* Get monitor index */
static gint _xfdashboard_window_tracker_monitor_x11_x11_window_tracker_monitor_get_number(XfdashboardWindowTrackerMonitor *inMonitor)
{
	XfdashboardWindowTrackerMonitorX11			*self;
	XfdashboardWindowTrackerMonitorX11Private	*priv;

	g_return_val_if_fail(XFDASHBOARD_IS_WINDOW_TRACKER_MONITOR_X11(inMonitor), 0);

	self=XFDASHBOARD_WINDOW_TRACKER_MONITOR_X11(inMonitor);
	priv=self->priv;

	return(priv->monitorIndex);
}

/* Get geometry of monitor */
static void _xfdashboard_window_tracker_monitor_x11_x11_window_tracker_monitor_get_geometry(XfdashboardWindowTrackerMonitor *inMonitor,
																							gint *outX,
																							gint *outY,
																							gint *outWidth,
																							gint *outHeight)
{
	XfdashboardWindowTrackerMonitorX11			*self;
	XfdashboardWindowTrackerMonitorX11Private	*priv;

	g_return_if_fail(XFDASHBOARD_IS_WINDOW_TRACKER_MONITOR_X11(inMonitor));

	self=XFDASHBOARD_WINDOW_TRACKER_MONITOR_X11(inMonitor);
	priv=self->priv;

	/* Set position and size of monitor */
	if(outX) *outX=priv->geometry.x;
	if(outY) *outY=priv->geometry.y;
	if(outWidth) *outWidth=priv->geometry.width;
	if(outHeight) *outHeight=priv->geometry.height;
}

/* Interface initialization
 * Set up default functions
 */
static void _xfdashboard_window_tracker_monitor_x11_x11_window_tracker_monitor_iface_init(XfdashboardWindowTrackerMonitorInterface *iface)
{
	iface->is_primary=_xfdashboard_window_tracker_monitor_x11_x11_window_tracker_monitor_is_primary;
	iface->get_number=_xfdashboard_window_tracker_monitor_x11_x11_window_tracker_monitor_get_number;
	iface->get_geometry=_xfdashboard_window_tracker_monitor_x11_x11_window_tracker_monitor_get_geometry;
}


/* IMPLEMENTATION: GObject */

/* Dispose this object */
static void _xfdashboard_window_tracker_monitor_x11_dispose(GObject *inObject)
{
	XfdashboardWindowTrackerMonitorX11			*self=XFDASHBOARD_WINDOW_TRACKER_MONITOR_X11(inObject);
	XfdashboardWindowTrackerMonitorX11Private	*priv=self->priv;

	/* Release allocated resources */
	if(priv->screen)
	{
		g_signal_handlers_disconnect_by_data(priv->screen, self);
		priv->screen=NULL;
	}

	/* Call parent's class dispose method */
	G_OBJECT_CLASS(xfdashboard_window_tracker_monitor_x11_parent_class)->dispose(inObject);
}

/* Set/get properties */
static void _xfdashboard_window_tracker_monitor_x11_set_property(GObject *inObject,
																	guint inPropID,
																	const GValue *inValue,
																	GParamSpec *inSpec)
{
	XfdashboardWindowTrackerMonitorX11			*self=XFDASHBOARD_WINDOW_TRACKER_MONITOR_X11(inObject);

	switch(inPropID)
	{
		case PROP_MONITOR_INDEX:
			_xfdashboard_window_tracker_monitor_x11_set_index(self, g_value_get_int(inValue));
			break;

		default:
			G_OBJECT_WARN_INVALID_PROPERTY_ID(inObject, inPropID, inSpec);
			break;
	}
}

static void _xfdashboard_window_tracker_monitor_x11_get_property(GObject *inObject,
																	guint inPropID,
																	GValue *outValue,
																	GParamSpec *inSpec)
{
	XfdashboardWindowTrackerMonitorX11			*self=XFDASHBOARD_WINDOW_TRACKER_MONITOR_X11(inObject);
	XfdashboardWindowTrackerMonitorX11Private	*priv=self->priv;

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
static void xfdashboard_window_tracker_monitor_x11_class_init(XfdashboardWindowTrackerMonitorX11Class *klass)
{
	GObjectClass						*gobjectClass=G_OBJECT_CLASS(klass);
	XfdashboardWindowTrackerMonitor		*monitorIface;
	GParamSpec							*paramSpec;

	/* Reference interface type to lookup properties etc. */
	monitorIface=g_type_default_interface_ref(XFDASHBOARD_TYPE_WINDOW_TRACKER_MONITOR);

	/* Override functions */
	gobjectClass->dispose=_xfdashboard_window_tracker_monitor_x11_dispose;
	gobjectClass->set_property=_xfdashboard_window_tracker_monitor_x11_set_property;
	gobjectClass->get_property=_xfdashboard_window_tracker_monitor_x11_get_property;

	/* Define properties */
	paramSpec=g_object_interface_find_property(monitorIface, "is-primary");
	XfdashboardWindowTrackerMonitorX11Properties[PROP_IS_PRIMARY]=
		g_param_spec_override("is-primary", paramSpec);

	paramSpec=g_object_interface_find_property(monitorIface, "monitor-index");
	XfdashboardWindowTrackerMonitorX11Properties[PROP_MONITOR_INDEX]=
		g_param_spec_override("monitor-index", paramSpec);

	g_object_class_install_properties(gobjectClass, PROP_LAST, XfdashboardWindowTrackerMonitorX11Properties);

	/* Release allocated resources */
	g_type_default_interface_unref(monitorIface);
}

/* Object initialization
 * Create private structure and set up default values
 */
static void xfdashboard_window_tracker_monitor_x11_init(XfdashboardWindowTrackerMonitorX11 *self)
{
	XfdashboardWindowTrackerMonitorX11Private		*priv;

	priv=self->priv=xfdashboard_window_tracker_monitor_x11_get_instance_private(self);

	/* Set default values */
	priv->monitorIndex=-1;
	priv->isPrimary=FALSE;
	priv->screen=gdk_screen_get_default();
	priv->geometry.x=0;
	priv->geometry.y=0;
	priv->geometry.width=0;
	priv->geometry.height=0;
}
