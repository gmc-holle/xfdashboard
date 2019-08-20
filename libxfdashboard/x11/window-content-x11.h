/*
 * window-content: A content to share texture of a window
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

#ifndef __LIBXFDASHBOARD_WINDOW_CONTENT_X11__
#define __LIBXFDASHBOARD_WINDOW_CONTENT_X11__

#if !defined(__LIBXFDASHBOARD_H_INSIDE__) && !defined(LIBXFDASHBOARD_COMPILATION)
#error "Only <libxfdashboard/libxfdashboard.h> can be included directly."
#endif

#include <clutter/clutter.h>

#include <libxfdashboard/x11/window-tracker-window-x11.h>
#include <libxfdashboard/window-tracker-window.h>
#include <libxfdashboard/types.h>

G_BEGIN_DECLS

#define XFDASHBOARD_TYPE_WINDOW_CONTENT_X11				(xfdashboard_window_content_x11_get_type())
#define XFDASHBOARD_WINDOW_CONTENT_X11(obj)				(G_TYPE_CHECK_INSTANCE_CAST((obj), XFDASHBOARD_TYPE_WINDOW_CONTENT_X11, XfdashboardWindowContentX11))
#define XFDASHBOARD_IS_WINDOW_CONTENT_X11(obj)			(G_TYPE_CHECK_INSTANCE_TYPE((obj), XFDASHBOARD_TYPE_WINDOW_CONTENT_X11))
#define XFDASHBOARD_WINDOW_CONTENT_X11_CLASS(klass)		(G_TYPE_CHECK_CLASS_CAST((klass), XFDASHBOARD_TYPE_WINDOW_CONTENT_X11, XfdashboardWindowContentX11Class))
#define XFDASHBOARD_IS_WINDOW_CONTENT_X11_CLASS(klass)	(G_TYPE_CHECK_CLASS_TYPE((klass), XFDASHBOARD_TYPE_WINDOW_CONTENT_X11))
#define XFDASHBOARD_WINDOW_CONTENT_X11_GET_CLASS(obj)	(G_TYPE_INSTANCE_GET_CLASS((obj), XFDASHBOARD_TYPE_WINDOW_CONTENT_X11, XfdashboardWindowContentX11Class))

typedef struct _XfdashboardWindowContentX11				XfdashboardWindowContentX11;
typedef struct _XfdashboardWindowContentX11Class		XfdashboardWindowContentX11Class;
typedef struct _XfdashboardWindowContentX11Private		XfdashboardWindowContentX11Private;

struct _XfdashboardWindowContentX11
{
	/*< private >*/
	/* Parent instance */
	GObject										parent_instance;

	/* Private structure */
	XfdashboardWindowContentX11Private			*priv;
};

struct _XfdashboardWindowContentX11Class
{
	/*< private >*/
	/* Parent class */
	GObjectClass								parent_class;

	/*< public >*/
	/* Virtual functions */
};

/* Public API */
GType xfdashboard_window_content_x11_get_type(void) G_GNUC_CONST;

ClutterContent* xfdashboard_window_content_x11_new_for_window(XfdashboardWindowTrackerWindowX11 *inWindow);

XfdashboardWindowTrackerWindow* xfdashboard_window_content_x11_get_window(XfdashboardWindowContentX11 *self);

gboolean xfdashboard_window_content_x11_is_suspended(XfdashboardWindowContentX11 *self);

const ClutterColor* xfdashboard_window_content_x11_get_outline_color(XfdashboardWindowContentX11 *self);
void xfdashboard_window_content_x11_set_outline_color(XfdashboardWindowContentX11 *self, const ClutterColor *inColor);

gfloat xfdashboard_window_content_x11_get_outline_width(XfdashboardWindowContentX11 *self);
void xfdashboard_window_content_x11_set_outline_width(XfdashboardWindowContentX11 *self, const gfloat inWidth);

gboolean xfdashboard_window_content_x11_get_include_window_frame(XfdashboardWindowContentX11 *self);
void xfdashboard_window_content_x11_set_include_window_frame(XfdashboardWindowContentX11 *self, const gboolean inIncludeFrame);

gboolean xfdashboard_window_content_x11_get_unmapped_window_icon_x_fill(XfdashboardWindowContentX11 *self);
void xfdashboard_window_content_x11_set_unmapped_window_icon_x_fill(XfdashboardWindowContentX11 *self, const gboolean inFill);

gboolean xfdashboard_window_content_x11_get_unmapped_window_icon_y_fill(XfdashboardWindowContentX11 *self);
void xfdashboard_window_content_x11_set_unmapped_window_icon_y_fill(XfdashboardWindowContentX11 *self, const gboolean inFill);

gfloat xfdashboard_window_content_x11_get_unmapped_window_icon_x_align(XfdashboardWindowContentX11 *self);
void xfdashboard_window_content_x11_set_unmapped_window_icon_x_align(XfdashboardWindowContentX11 *self, const gfloat inAlign);

gfloat xfdashboard_window_content_x11_get_unmapped_window_icon_y_align(XfdashboardWindowContentX11 *self);
void xfdashboard_window_content_x11_set_unmapped_window_icon_y_align(XfdashboardWindowContentX11 *self, const gfloat inAlign);

gfloat xfdashboard_window_content_x11_get_unmapped_window_icon_x_scale(XfdashboardWindowContentX11 *self);
void xfdashboard_window_content_x11_set_unmapped_window_icon_x_scale(XfdashboardWindowContentX11 *self, const gfloat inScale);

gfloat xfdashboard_window_content_x11_get_unmapped_window_icon_y_scale(XfdashboardWindowContentX11 *self);
void xfdashboard_window_content_x11_set_unmapped_window_icon_y_scale(XfdashboardWindowContentX11 *self, const gfloat inScale);

XfdashboardAnchorPoint xfdashboard_window_content_x11_get_unmapped_window_icon_anchor_point(XfdashboardWindowContentX11 *self);
void xfdashboard_window_content_x11_set_unmapped_window_icon_anchor_point(XfdashboardWindowContentX11 *self, const XfdashboardAnchorPoint inAnchorPoint);

G_END_DECLS

#endif
