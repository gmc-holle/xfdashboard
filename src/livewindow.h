/*
 * livewindow.h: An actor showing and updating a window live
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

#ifndef __XFOVERVIEW_LIVEWINDOW__
#define __XFOVERVIEW_LIVEWINDOW__

#include <clutter/clutter.h>
#include <clutter/x11/clutter-x11.h>

#define WNCK_I_KNOW_THIS_IS_UNSTABLE
#include <libwnck/libwnck.h>

G_BEGIN_DECLS

#define XFDASHBOARD_TYPE_LIVEWINDOW						(xfdashboard_livewindow_get_type())
#define XFDASHBOARD_LIVEWINDOW(obj)						(G_TYPE_CHECK_INSTANCE_CAST((obj), XFDASHBOARD_TYPE_LIVEWINDOW, XfdashboardLiveWindow))
#define XFDASHBOARD_IS_LIVEWINDOW(obj)					(G_TYPE_CHECK_INSTANCE_TYPE((obj), XFDASHBOARD_TYPE_LIVEWINDOW))
#define XFDASHBOARD_LIVEWINDOW_CLASS(klass)				(G_TYPE_CHECK_CLASS_CAST((klass), XFDASHBOARD_TYPE_LIVEWINDOW, XfdashboardLiveWindowClass))
#define XFDASHBOARD_IS_LIVEWINDOW_CLASS(klass)			(G_TYPE_CHECK_CLASS_TYPE((klass), XFDASHBOARD_TYPE_LIVEWINDOW))
#define XFDASHBOARD_LIVEWINDOW_GET_CLASS(obj)			(G_TYPE_INSTANCE_GET_CLASS((obj), XFDASHBOARD_TYPE_LIVEWINDOW, XfdashboardLiveWindowClass))

typedef struct _XfdashboardLiveWindow					XfdashboardLiveWindow;
typedef struct _XfdashboardLiveWindowClass				XfdashboardLiveWindowClass;
typedef struct _XfdashboardLiveWindowPrivate			XfdashboardLiveWindowPrivate;

struct _XfdashboardLiveWindow
{
	/* Parent instance */
	ClutterActor					parent_instance;

	/* Private structure */
	XfdashboardLiveWindowPrivate	*priv;
};

struct _XfdashboardLiveWindowClass
{
	/* Parent class */
	ClutterActorClass				parent_class;

	/* Virtual functions */
	void (*clicked)(XfdashboardLiveWindow *self);
};

GType xfdashboard_livewindow_get_type(void) G_GNUC_CONST;

ClutterActor* xfdashboard_livewindow_new(WnckWindow* inWindow);

void xfdashboard_livewindow_pack(XfdashboardLiveWindow *self, ClutterActor *inActor);

const WnckWindow* xfdashboard_livewindow_get_window(XfdashboardLiveWindow *self);

const gchar* xfdashboard_livewindow_get_font(XfdashboardLiveWindow *self);
void xfdashboard_livewindow_set_font(XfdashboardLiveWindow *self, const gchar *inFont);

const ClutterColor* xfdashboard_livewindow_get_color(XfdashboardLiveWindow *self);
void xfdashboard_livewindow_set_color(XfdashboardLiveWindow *self, const ClutterColor *inColor);

const ClutterColor* xfdashboard_livewindow_get_background_color(XfdashboardLiveWindow *self);
void xfdashboard_livewindow_set_background_color(XfdashboardLiveWindow *self, const ClutterColor *inColor);

const gint xfdashboard_livewindow_get_margin(XfdashboardLiveWindow *self);
void xfdashboard_livewindow_set_margin(XfdashboardLiveWindow *self, const gint inMargin);

const PangoEllipsizeMode xfdashboard_livewindow_get_ellipsize_mode(XfdashboardLiveWindow *self);
void xfdashboard_livewindow_set_ellipsize_mode(XfdashboardLiveWindow *self, const PangoEllipsizeMode inMode);

G_END_DECLS

#endif
