/*
 * window-content: A content to share texture of a window
 *
 * Copyright 2012-2017 Stephan Haller <nomad@froevel.de>
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

#ifndef __LIBXFDASHBOARD_WINDOW_CONTENT_GDK__
#define __LIBXFDASHBOARD_WINDOW_CONTENT_GDK__

#if !defined(__LIBXFDASHBOARD_H_INSIDE__) && !defined(LIBXFDASHBOARD_COMPILATION)
#error "Only <libxfdashboard/libxfdashboard.h> can be included directly."
#endif

#include <clutter/clutter.h>

#include <libxfdashboard/gdk/window-tracker-window-gdk.h>
#include <libxfdashboard/window-tracker-window.h>
#include <libxfdashboard/types.h>

G_BEGIN_DECLS

#define XFDASHBOARD_TYPE_WINDOW_CONTENT_GDK				(xfdashboard_window_content_gdk_get_type())
#define XFDASHBOARD_WINDOW_CONTENT_GDK(obj)				(G_TYPE_CHECK_INSTANCE_CAST((obj), XFDASHBOARD_TYPE_WINDOW_CONTENT_GDK, XfdashboardWindowContentGDK))
#define XFDASHBOARD_IS_WINDOW_CONTENT_GDK(obj)			(G_TYPE_CHECK_INSTANCE_TYPE((obj), XFDASHBOARD_TYPE_WINDOW_CONTENT_GDK))
#define XFDASHBOARD_WINDOW_CONTENT_GDK_CLASS(klass)		(G_TYPE_CHECK_CLASS_CAST((klass), XFDASHBOARD_TYPE_WINDOW_CONTENT_GDK, XfdashboardWindowContentGDKClass))
#define XFDASHBOARD_IS_WINDOW_CONTENT_GDK_CLASS(klass)	(G_TYPE_CHECK_CLASS_TYPE((klass), XFDASHBOARD_TYPE_WINDOW_CONTENT_GDK))
#define XFDASHBOARD_WINDOW_CONTENT_GDK_GET_CLASS(obj)	(G_TYPE_INSTANCE_GET_CLASS((obj), XFDASHBOARD_TYPE_WINDOW_CONTENT_GDK, XfdashboardWindowContentGDKClass))

typedef struct _XfdashboardWindowContentGDK				XfdashboardWindowContentGDK;
typedef struct _XfdashboardWindowContentGDKClass		XfdashboardWindowContentGDKClass;
typedef struct _XfdashboardWindowContentGDKPrivate		XfdashboardWindowContentGDKPrivate;

struct _XfdashboardWindowContentGDK
{
	/*< private >*/
	/* Parent instance */
	GObject										parent_instance;

	/* Private structure */
	XfdashboardWindowContentGDKPrivate			*priv;
};

struct _XfdashboardWindowContentGDKClass
{
	/*< private >*/
	/* Parent class */
	GObjectClass								parent_class;

	/*< public >*/
	/* Virtual functions */
};

/* Public API */
GType xfdashboard_window_content_gdk_get_type(void) G_GNUC_CONST;

ClutterContent* xfdashboard_window_content_gdk_new_for_window(XfdashboardWindowTrackerWindowGDK *inWindow);

XfdashboardWindowTrackerWindow* xfdashboard_window_content_gdk_get_window(XfdashboardWindowContentGDK *self);

gboolean xfdashboard_window_content_gdk_is_suspended(XfdashboardWindowContentGDK *self);

const ClutterColor* xfdashboard_window_content_gdk_get_outline_color(XfdashboardWindowContentGDK *self);
void xfdashboard_window_content_gdk_set_outline_color(XfdashboardWindowContentGDK *self, const ClutterColor *inColor);

gfloat xfdashboard_window_content_gdk_get_outline_width(XfdashboardWindowContentGDK *self);
void xfdashboard_window_content_gdk_set_outline_width(XfdashboardWindowContentGDK *self, const gfloat inWidth);

gboolean xfdashboard_window_content_gdk_get_include_window_frame(XfdashboardWindowContentGDK *self);
void xfdashboard_window_content_gdk_set_include_window_frame(XfdashboardWindowContentGDK *self, const gboolean inIncludeFrame);

gboolean xfdashboard_window_content_gdk_get_unmapped_window_icon_x_fill(XfdashboardWindowContentGDK *self);
void xfdashboard_window_content_gdk_set_unmapped_window_icon_x_fill(XfdashboardWindowContentGDK *self, const gboolean inFill);

gboolean xfdashboard_window_content_gdk_get_unmapped_window_icon_y_fill(XfdashboardWindowContentGDK *self);
void xfdashboard_window_content_gdk_set_unmapped_window_icon_y_fill(XfdashboardWindowContentGDK *self, const gboolean inFill);

gfloat xfdashboard_window_content_gdk_get_unmapped_window_icon_x_align(XfdashboardWindowContentGDK *self);
void xfdashboard_window_content_gdk_set_unmapped_window_icon_x_align(XfdashboardWindowContentGDK *self, const gfloat inAlign);

gfloat xfdashboard_window_content_gdk_get_unmapped_window_icon_y_align(XfdashboardWindowContentGDK *self);
void xfdashboard_window_content_gdk_set_unmapped_window_icon_y_align(XfdashboardWindowContentGDK *self, const gfloat inAlign);

gfloat xfdashboard_window_content_gdk_get_unmapped_window_icon_x_scale(XfdashboardWindowContentGDK *self);
void xfdashboard_window_content_gdk_set_unmapped_window_icon_x_scale(XfdashboardWindowContentGDK *self, const gfloat inScale);

gfloat xfdashboard_window_content_gdk_get_unmapped_window_icon_y_scale(XfdashboardWindowContentGDK *self);
void xfdashboard_window_content_gdk_set_unmapped_window_icon_y_scale(XfdashboardWindowContentGDK *self, const gfloat inScale);

XfdashboardAnchorPoint xfdashboard_window_content_gdk_get_unmapped_window_icon_anchor_point(XfdashboardWindowContentGDK *self);
void xfdashboard_window_content_gdk_set_unmapped_window_icon_anchor_point(XfdashboardWindowContentGDK *self, const XfdashboardAnchorPoint inAnchorPoint);

G_END_DECLS

#endif
