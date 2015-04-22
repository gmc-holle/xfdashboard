/*
 * window-content: A content to share texture of a window
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

#ifndef __XFOVERVIEW_WINDOW_CONTENT__
#define __XFOVERVIEW_WINDOW_CONTENT__

#include <clutter/clutter.h>

#include "window-tracker-window.h"
#include "types.h"

G_BEGIN_DECLS

#define XFDASHBOARD_TYPE_WINDOW_CONTENT				(xfdashboard_window_content_get_type())
#define XFDASHBOARD_WINDOW_CONTENT(obj)				(G_TYPE_CHECK_INSTANCE_CAST((obj), XFDASHBOARD_TYPE_WINDOW_CONTENT, XfdashboardWindowContent))
#define XFDASHBOARD_IS_WINDOW_CONTENT(obj)			(G_TYPE_CHECK_INSTANCE_TYPE((obj), XFDASHBOARD_TYPE_WINDOW_CONTENT))
#define XFDASHBOARD_WINDOW_CONTENT_CLASS(klass)		(G_TYPE_CHECK_CLASS_CAST((klass), XFDASHBOARD_TYPE_WINDOW_CONTENT, XfdashboardWindowContentClass))
#define XFDASHBOARD_IS_WINDOW_CONTENT_CLASS(klass)	(G_TYPE_CHECK_CLASS_TYPE((klass), XFDASHBOARD_TYPE_WINDOW_CONTENT))
#define XFDASHBOARD_WINDOW_CONTENT_GET_CLASS(obj)	(G_TYPE_INSTANCE_GET_CLASS((obj), XFDASHBOARD_TYPE_WINDOW_CONTENT, XfdashboardWindowContentClass))

typedef struct _XfdashboardWindowContent			XfdashboardWindowContent;
typedef struct _XfdashboardWindowContentClass		XfdashboardWindowContentClass;
typedef struct _XfdashboardWindowContentPrivate		XfdashboardWindowContentPrivate;

struct _XfdashboardWindowContent
{
	/* Parent instance */
	GObject									parent_instance;

	/* Private structure */
	XfdashboardWindowContentPrivate			*priv;
};

struct _XfdashboardWindowContentClass
{
	/*< private >*/
	/* Parent class */
	GObjectClass							parent_class;

	/*< public >*/
	/* Virtual functions */
};

/* Public API */
GType xfdashboard_window_content_get_type(void) G_GNUC_CONST;

ClutterContent* xfdashboard_window_content_new_for_window(XfdashboardWindowTrackerWindow *inWindow);

XfdashboardWindowTrackerWindow* xfdashboard_window_content_get_window(XfdashboardWindowContent *self);

gboolean xfdashboard_window_content_is_suspended(XfdashboardWindowContent *self);

const ClutterColor* xfdashboard_window_content_get_outline_color(XfdashboardWindowContent *self);
void xfdashboard_window_content_set_outline_color(XfdashboardWindowContent *self, const ClutterColor *inColor);

gfloat xfdashboard_window_content_get_outline_width(XfdashboardWindowContent *self);
void xfdashboard_window_content_set_outline_width(XfdashboardWindowContent *self, const gfloat inWidth);

gboolean xfdashboard_window_content_get_include_window_frame(XfdashboardWindowContent *self);
void xfdashboard_window_content_set_include_window_frame(XfdashboardWindowContent *self, const gboolean inIncludeFrame);

gboolean xfdashboard_window_content_get_unmapped_window_icon_x_fill(XfdashboardWindowContent *self);
void xfdashboard_window_content_set_unmapped_window_icon_x_fill(XfdashboardWindowContent *self, const gboolean inFill);

gboolean xfdashboard_window_content_get_unmapped_window_icon_y_fill(XfdashboardWindowContent *self);
void xfdashboard_window_content_set_unmapped_window_icon_y_fill(XfdashboardWindowContent *self, const gboolean inFill);

gfloat xfdashboard_window_content_get_unmapped_window_icon_x_align(XfdashboardWindowContent *self);
void xfdashboard_window_content_set_unmapped_window_icon_x_align(XfdashboardWindowContent *self, const gfloat inAlign);

gfloat xfdashboard_window_content_get_unmapped_window_icon_y_align(XfdashboardWindowContent *self);
void xfdashboard_window_content_set_unmapped_window_icon_y_align(XfdashboardWindowContent *self, const gfloat inAlign);

gfloat xfdashboard_window_content_get_unmapped_window_icon_x_scale(XfdashboardWindowContent *self);
void xfdashboard_window_content_set_unmapped_window_icon_x_scale(XfdashboardWindowContent *self, const gfloat inScale);

gfloat xfdashboard_window_content_get_unmapped_window_icon_y_scale(XfdashboardWindowContent *self);
void xfdashboard_window_content_set_unmapped_window_icon_y_scale(XfdashboardWindowContent *self, const gfloat inScale);

/* DEPRECATED */
ClutterGravity xfdashboard_window_content_get_unmapped_window_icon_gravity(XfdashboardWindowContent *self);
void xfdashboard_window_content_set_unmapped_window_icon_gravity(XfdashboardWindowContent *self, const ClutterGravity inGravity);

XfdashboardAnchorPoint xfdashboard_window_content_get_unmapped_window_icon_anchor_point(XfdashboardWindowContent *self);
void xfdashboard_window_content_set_unmapped_window_icon_anchor_point(XfdashboardWindowContent *self, const XfdashboardAnchorPoint inAnchorPoint);

G_END_DECLS

#endif	/* __XFOVERVIEW_WINDOW_CONTENT__ */
