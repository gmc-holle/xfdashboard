/*
 * middle-click-window-close: Closes windows in window by middle-click
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

#ifndef __XFDASHBOARD_MIDDLE_CLICK_WINDOW_CLOSE__
#define __XFDASHBOARD_MIDDLE_CLICK_WINDOW_CLOSE__

#include <libxfdashboard/libxfdashboard.h>

G_BEGIN_DECLS

#define XFDASHBOARD_TYPE_MIDDLE_CLICK_WINDOW_CLOSE				(xfdashboard_middle_click_window_close_get_type())
#define XFDASHBOARD_MIDDLE_CLICK_WINDOW_CLOSE(obj)				(G_TYPE_CHECK_INSTANCE_CAST((obj), XFDASHBOARD_TYPE_MIDDLE_CLICK_WINDOW_CLOSE, XfdashboardMiddleClickWindowClose))
#define XFDASHBOARD_IS_MIDDLE_CLICK_WINDOW_CLOSE(obj)			(G_TYPE_CHECK_INSTANCE_TYPE((obj), XFDASHBOARD_TYPE_MIDDLE_CLICK_WINDOW_CLOSE))
#define XFDASHBOARD_MIDDLE_CLICK_WINDOW_CLOSE_CLASS(klass)		(G_TYPE_CHECK_CLASS_CAST((klass), XFDASHBOARD_TYPE_MIDDLE_CLICK_WINDOW_CLOSE, XfdashboardMiddleClickWindowCloseClass))
#define XFDASHBOARD_IS_MIDDLE_CLICK_WINDOW_CLOSE_CLASS(klass)	(G_TYPE_CHECK_CLASS_TYPE((klass), XFDASHBOARD_TYPE_MIDDLE_CLICK_WINDOW_CLOSE))
#define XFDASHBOARD_MIDDLE_CLICK_WINDOW_CLOSE_GET_CLASS(obj)	(G_TYPE_INSTANCE_GET_CLASS((obj), XFDASHBOARD_TYPE_MIDDLE_CLICK_WINDOW_CLOSE, XfdashboardMiddleClickWindowCloseClass))

typedef struct _XfdashboardMiddleClickWindowClose				XfdashboardMiddleClickWindowClose; 
typedef struct _XfdashboardMiddleClickWindowClosePrivate		XfdashboardMiddleClickWindowClosePrivate;
typedef struct _XfdashboardMiddleClickWindowCloseClass			XfdashboardMiddleClickWindowCloseClass;

struct _XfdashboardMiddleClickWindowClose
{
	/* Parent instance */
	GObject										parent_instance;

	/* Private structure */
	XfdashboardMiddleClickWindowClosePrivate	*priv;
};

struct _XfdashboardMiddleClickWindowCloseClass
{
	/*< private >*/
	/* Parent class */
	GObjectClass								parent_class;
};

/* Public API */
GType xfdashboard_middle_click_window_close_get_type(void) G_GNUC_CONST;

XFDASHBOARD_DECLARE_PLUGIN_TYPE(xfdashboard_middle_click_window_close);

XfdashboardMiddleClickWindowClose* xfdashboard_middle_click_window_close_new(void);

G_END_DECLS

#endif
