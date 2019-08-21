/*
 * live-window-simple: An actor showing the content of a window which will
 *                     be updated if changed and visible on active workspace.
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

#ifndef __LIBXFDASHBOARD_LIVE_WINDOW_SIMPLE__
#define __LIBXFDASHBOARD_LIVE_WINDOW_SIMPLE__

#if !defined(__LIBXFDASHBOARD_H_INSIDE__) && !defined(LIBXFDASHBOARD_COMPILATION)
#error "Only <libxfdashboard/libxfdashboard.h> can be included directly."
#endif

#include <clutter/clutter.h>

#include <libxfdashboard/background.h>
#include <libxfdashboard/window-tracker.h>

G_BEGIN_DECLS

/* Public definitions */
/**
 * XfdashboardLiveWindowSimpleDisplayType:
 * @XFDASHBOARD_LIVE_WINDOW_SIMPLE_DISPLAY_TYPE_LIVE_PREVIEW: The actor will show a live preview of window
 * @XFDASHBOARD_LIVE_WINDOW_SIMPLE_DISPLAY_TYPE_ICON: The actor will show the window's icon in size of window
 *
 * Determines how the window will be displayed.
 */
typedef enum /*< prefix=XFDASHBOARD_LIVE_WINDOW_SIMPLE_DISPLAY_TYPE >*/
{
	XFDASHBOARD_LIVE_WINDOW_SIMPLE_DISPLAY_TYPE_LIVE_PREVIEW=0,
	XFDASHBOARD_LIVE_WINDOW_SIMPLE_DISPLAY_TYPE_ICON,
} XfdashboardLiveWindowSimpleDisplayType;


/* Object declaration */
#define XFDASHBOARD_TYPE_LIVE_WINDOW_SIMPLE				(xfdashboard_live_window_simple_get_type())
#define XFDASHBOARD_LIVE_WINDOW_SIMPLE(obj)				(G_TYPE_CHECK_INSTANCE_CAST((obj), XFDASHBOARD_TYPE_LIVE_WINDOW_SIMPLE, XfdashboardLiveWindowSimple))
#define XFDASHBOARD_IS_LIVE_WINDOW_SIMPLE(obj)			(G_TYPE_CHECK_INSTANCE_TYPE((obj), XFDASHBOARD_TYPE_LIVE_WINDOW_SIMPLE))
#define XFDASHBOARD_LIVE_WINDOW_SIMPLE_CLASS(klass)		(G_TYPE_CHECK_CLASS_CAST((klass), XFDASHBOARD_TYPE_LIVE_WINDOW_SIMPLE, XfdashboardLiveWindowSimpleClass))
#define XFDASHBOARD_IS_LIVE_WINDOW_SIMPLE_CLASS(klass)	(G_TYPE_CHECK_CLASS_TYPE((klass), XFDASHBOARD_TYPE_LIVE_WINDOW_SIMPLE))
#define XFDASHBOARD_LIVE_WINDOW_SIMPLE_GET_CLASS(obj)	(G_TYPE_INSTANCE_GET_CLASS((obj), XFDASHBOARD_TYPE_LIVE_WINDOW_SIMPLE, XfdashboardLiveWindowSimpleClass))

typedef struct _XfdashboardLiveWindowSimple				XfdashboardLiveWindowSimple;
typedef struct _XfdashboardLiveWindowSimpleClass		XfdashboardLiveWindowSimpleClass;
typedef struct _XfdashboardLiveWindowSimplePrivate		XfdashboardLiveWindowSimplePrivate;

struct _XfdashboardLiveWindowSimple
{
	/*< private >*/
	/* Parent instance */
	XfdashboardBackground			parent_instance;

	/* Private structure */
	XfdashboardLiveWindowSimplePrivate	*priv;
};

struct _XfdashboardLiveWindowSimpleClass
{
	/*< private >*/
	/* Parent class */
	XfdashboardBackgroundClass		parent_class;

	/*< public >*/
	/* Virtual functions */
	void (*geometry_changed)(XfdashboardLiveWindowSimple *self);
	void (*visibility_changed)(XfdashboardLiveWindowSimple *self, gboolean inVisible);
	void (*workspace_changed)(XfdashboardLiveWindowSimple *self);
};

/* Public API */
GType xfdashboard_live_window_simple_get_type(void) G_GNUC_CONST;

ClutterActor* xfdashboard_live_window_simple_new(void);
ClutterActor* xfdashboard_live_window_simple_new_for_window(XfdashboardWindowTrackerWindow *inWindow);

XfdashboardWindowTrackerWindow* xfdashboard_live_window_simple_get_window(XfdashboardLiveWindowSimple *self);
void xfdashboard_live_window_simple_set_window(XfdashboardLiveWindowSimple *self, XfdashboardWindowTrackerWindow *inWindow);

XfdashboardLiveWindowSimpleDisplayType xfdashboard_live_window_simple_get_display_type(XfdashboardLiveWindowSimple *self);
void xfdashboard_live_window_simple_set_display_type(XfdashboardLiveWindowSimple *self, XfdashboardLiveWindowSimpleDisplayType inType);

gboolean xfdashboard_live_window_simple_get_destroy_on_close(XfdashboardLiveWindowSimple *self);
void xfdashboard_live_window_simple_set_destroy_on_close(XfdashboardLiveWindowSimple *self, gboolean inDestroyOnClose);

G_END_DECLS

#endif	/* __LIBXFDASHBOARD_LIVE_WINDOW_SIMPLE__ */
