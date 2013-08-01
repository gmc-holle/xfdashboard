/*
 * live-window: An actor showing the content of a window which will
 *              be updated if changed and visible on active workspace.
 *              It also provides controls to manipulate it.
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

#ifndef __XFOVERVIEW_LIVE_WINDOW__
#define __XFOVERVIEW_LIVE_WINDOW__

#define WNCK_I_KNOW_THIS_IS_UNSTABLE
#include <libwnck/libwnck.h>
#include <clutter/clutter.h>

#include "button.h"

G_BEGIN_DECLS

#define XFDASHBOARD_TYPE_LIVE_WINDOW				(xfdashboard_live_window_get_type())
#define XFDASHBOARD_LIVE_WINDOW(obj)				(G_TYPE_CHECK_INSTANCE_CAST((obj), XFDASHBOARD_TYPE_LIVE_WINDOW, XfdashboardLiveWindow))
#define XFDASHBOARD_IS_LIVE_WINDOW(obj)				(G_TYPE_CHECK_INSTANCE_TYPE((obj), XFDASHBOARD_TYPE_LIVE_WINDOW))
#define XFDASHBOARD_LIVE_WINDOW_CLASS(klass)		(G_TYPE_CHECK_CLASS_CAST((klass), XFDASHBOARD_TYPE_LIVE_WINDOW, XfdashboardLiveWindowClass))
#define XFDASHBOARD_IS_LIVE_WINDOW_CLASS(klass)		(G_TYPE_CHECK_CLASS_TYPE((klass), XFDASHBOARD_TYPE_LIVE_WINDOW))
#define XFDASHBOARD_LIVE_WINDOW_GET_CLASS(obj)		(G_TYPE_INSTANCE_GET_CLASS((obj), XFDASHBOARD_TYPE_LIVE_WINDOW, XfdashboardLiveWindowClass))

typedef struct _XfdashboardLiveWindow				XfdashboardLiveWindow;
typedef struct _XfdashboardLiveWindowClass			XfdashboardLiveWindowClass;
typedef struct _XfdashboardLiveWindowPrivate		XfdashboardLiveWindowPrivate;

struct _XfdashboardLiveWindow
{
	/* Parent instance */
	ClutterActor					parent_instance;

	/* Private structure */
	XfdashboardLiveWindowPrivate	*priv;
};

struct _XfdashboardLiveWindowClass
{
	/*< private >*/
	/* Parent class */
	ClutterActorClass				parent_class;

	/*< public >*/
	/* Virtual functions */
	void (*clicked)(XfdashboardLiveWindow *self);
	void (*close)(XfdashboardLiveWindow *self);
	void (*geometry_changed)(XfdashboardLiveWindow *self);
	void (*visibility_changed)(XfdashboardLiveWindow *self, gboolean inVisible);
	void (*workspace_changed)(XfdashboardLiveWindow *self);
};

/* Public API */
GType xfdashboard_live_window_get_type(void) G_GNUC_CONST;

ClutterActor* xfdashboard_live_window_new();
ClutterActor* xfdashboard_live_window_new_for_window(WnckWindow *inWindow);

WnckWindow* xfdashboard_live_window_get_window(XfdashboardLiveWindow *self);
void xfdashboard_live_window_set_window(XfdashboardLiveWindow *self, WnckWindow *inWindow);

XfdashboardButton* xfdashboard_live_window_get_title_actor(XfdashboardLiveWindow *self);

gfloat xfdashboard_live_window_get_title_actor_margin(XfdashboardLiveWindow *self);
void xfdashboard_live_window_set_title_actor_margin(XfdashboardLiveWindow *self, gfloat inMargin);

XfdashboardButton* xfdashboard_live_window_get_close_button(XfdashboardLiveWindow *self);

gfloat xfdashboard_live_window_get_close_button_margin(XfdashboardLiveWindow *self);
void xfdashboard_live_window_set_close_button_margin(XfdashboardLiveWindow *self, gfloat inMargin);

G_END_DECLS

#endif	/* __XFOVERVIEW_LIVE_WINDOW__ */
