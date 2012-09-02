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

#ifndef __XFOVERVIEW_LIVE_WINDOW__
#define __XFOVERVIEW_LIVE_WINDOW__

#include <clutter/clutter.h>
#include <clutter/x11/clutter-x11.h>

#define WNCK_I_KNOW_THIS_IS_UNSTABLE
#include <libwnck/libwnck.h>

G_BEGIN_DECLS

#define XFDASHBOARD_TYPE_LIVE_WINDOW					(xfdashboard_live_window_get_type())
#define XFDASHBOARD_LIVE_WINDOW(obj)					(G_TYPE_CHECK_INSTANCE_CAST((obj), XFDASHBOARD_TYPE_LIVE_WINDOW, XfdashboardLiveWindow))
#define XFDASHBOARD_IS_LIVE_WINDOW(obj)					(G_TYPE_CHECK_INSTANCE_TYPE((obj), XFDASHBOARD_TYPE_LIVE_WINDOW))
#define XFDASHBOARD_LIVE_WINDOW_CLASS(klass)			(G_TYPE_CHECK_CLASS_CAST((klass), XFDASHBOARD_TYPE_LIVE_WINDOW, XfdashboardLiveWindowClass))
#define XFDASHBOARD_IS_LIVE_WINDOW_CLASS(klass)			(G_TYPE_CHECK_CLASS_TYPE((klass), XFDASHBOARD_TYPE_LIVE_WINDOW))
#define XFDASHBOARD_LIVE_WINDOW_GET_CLASS(obj)			(G_TYPE_INSTANCE_GET_CLASS((obj), XFDASHBOARD_TYPE_LIVE_WINDOW, XfdashboardLiveWindowClass))

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

GType xfdashboard_live_window_get_type(void) G_GNUC_CONST;

ClutterActor* xfdashboard_live_window_new(WnckWindow* inWindow);

const WnckWindow* xfdashboard_live_window_get_window(XfdashboardLiveWindow *self);

const gchar* xfdashboard_live_window_get_label_font(XfdashboardLiveWindow *self);
void xfdashboard_live_window_set_label_font(XfdashboardLiveWindow *self, const gchar *inFont);

const ClutterColor* xfdashboard_live_window_get_label_color(XfdashboardLiveWindow *self);
void xfdashboard_live_window_set_label_color(XfdashboardLiveWindow *self, const ClutterColor *inColor);

const ClutterColor* xfdashboard_live_window_get_label_background_color(XfdashboardLiveWindow *self);
void xfdashboard_live_window_set_label_background_color(XfdashboardLiveWindow *self, const ClutterColor *inColor);

const gfloat xfdashboard_live_window_get_label_margin(XfdashboardLiveWindow *self);
void xfdashboard_live_window_set_label_margin(XfdashboardLiveWindow *self, const gfloat inMargin);

const PangoEllipsizeMode xfdashboard_live_window_get_label_ellipsize_mode(XfdashboardLiveWindow *self);
void xfdashboard_live_window_set_label_ellipsize_mode(XfdashboardLiveWindow *self, const PangoEllipsizeMode inMode);

G_END_DECLS

#endif
