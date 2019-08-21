/*
 * scrollbar: A scroll bar
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

#ifndef __LIBXFDASHBOARD_SCROLLBAR__
#define __LIBXFDASHBOARD_SCROLLBAR__

#if !defined(__LIBXFDASHBOARD_H_INSIDE__) && !defined(LIBXFDASHBOARD_COMPILATION)
#error "Only <libxfdashboard/libxfdashboard.h> can be included directly."
#endif

#include <clutter/clutter.h>

#include <libxfdashboard/background.h>

G_BEGIN_DECLS

#define XFDASHBOARD_TYPE_SCROLLBAR				(xfdashboard_scrollbar_get_type())
#define XFDASHBOARD_SCROLLBAR(obj)				(G_TYPE_CHECK_INSTANCE_CAST((obj), XFDASHBOARD_TYPE_SCROLLBAR, XfdashboardScrollbar))
#define XFDASHBOARD_IS_SCROLLBAR(obj)			(G_TYPE_CHECK_INSTANCE_TYPE((obj), XFDASHBOARD_TYPE_SCROLLBAR))
#define XFDASHBOARD_SCROLLBAR_CLASS(klass)		(G_TYPE_CHECK_CLASS_CAST((klass), XFDASHBOARD_TYPE_SCROLLBAR, XfdashboardScrollbarClass))
#define XFDASHBOARD_IS_SCROLLBAR_CLASS(klass)	(G_TYPE_CHECK_CLASS_TYPE((klass), XFDASHBOARD_TYPE_SCROLLBAR))
#define XFDASHBOARD_SCROLLBAR_GET_CLASS(obj)	(G_TYPE_INSTANCE_GET_CLASS((obj), XFDASHBOARD_TYPE_SCROLLBAR, XfdashboardScrollbarClass))

typedef struct _XfdashboardScrollbar			XfdashboardScrollbar; 
typedef struct _XfdashboardScrollbarPrivate		XfdashboardScrollbarPrivate;
typedef struct _XfdashboardScrollbarClass		XfdashboardScrollbarClass;

struct _XfdashboardScrollbar
{
	/*< private >*/
	/* Parent instance */
	XfdashboardBackground			parent_instance;

	/* Private structure */
	XfdashboardScrollbarPrivate		*priv;
};

struct _XfdashboardScrollbarClass
{
	/*< private >*/
	/* Parent class */
	XfdashboardBackgroundClass		parent_class;

	/*< public >*/
	/* Virtual functions */
	void (*value_changed)(XfdashboardScrollbar *self, gfloat inValue);
};

/* Public API */
GType xfdashboard_scrollbar_get_type(void) G_GNUC_CONST;

ClutterActor* xfdashboard_scrollbar_new(ClutterOrientation inOrientation);

gfloat xfdashboard_scrollbar_get_orientation(XfdashboardScrollbar *self);
void xfdashboard_scrollbar_set_orientation(XfdashboardScrollbar *self, ClutterOrientation inOrientation);

gfloat xfdashboard_scrollbar_get_value(XfdashboardScrollbar *self);
void xfdashboard_scrollbar_set_value(XfdashboardScrollbar *self, gfloat inValue);

gfloat xfdashboard_scrollbar_get_value_range(XfdashboardScrollbar *self);

gfloat xfdashboard_scrollbar_get_range(XfdashboardScrollbar *self);
void xfdashboard_scrollbar_set_range(XfdashboardScrollbar *self, gfloat inRange);

gfloat xfdashboard_scrollbar_get_page_size_factor(XfdashboardScrollbar *self);
void xfdashboard_scrollbar_set_page_size_factor(XfdashboardScrollbar *self, gfloat inFactor);

gfloat xfdashboard_scrollbar_get_spacing(XfdashboardScrollbar *self);
void xfdashboard_scrollbar_set_spacing(XfdashboardScrollbar *self, gfloat inSpacing);

gfloat xfdashboard_scrollbar_get_slider_width(XfdashboardScrollbar *self);
void xfdashboard_scrollbar_set_slider_width(XfdashboardScrollbar *self, gfloat inWidth);

gfloat xfdashboard_scrollbar_get_slider_radius(XfdashboardScrollbar *self);
void xfdashboard_scrollbar_set_slider_radius(XfdashboardScrollbar *self, gfloat inRadius);

const ClutterColor* xfdashboard_scrollbar_get_slider_color(XfdashboardScrollbar *self);
void xfdashboard_scrollbar_set_slider_color(XfdashboardScrollbar *self, const ClutterColor *inColor);

G_END_DECLS

#endif	/* __LIBXFDASHBOARD_SCROLLBAR__ */
