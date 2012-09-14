/*
 * scrollbar.h: A scroll bar
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

#ifndef __XFDASHBOARD_SCROLLBAR__
#define __XFDASHBOARD_SCROLLBAR__

#include <clutter/clutter.h>

G_BEGIN_DECLS

#define XFDASHBOARD_TYPE_SCROLLBAR				(xfdashboard_scrollbar_get_type())
#define XFDASHBOARD_SCROLLBAR(obj)				(G_TYPE_CHECK_INSTANCE_CAST((obj), XFDASHBOARD_TYPE_SCROLLBAR, XfdashboardScrollbar))
#define XFDASHBOARD_IS_SCROLLBAR(obj)			(G_TYPE_CHECK_INSTANCE_TYPE((obj), XFDASHBOARD_TYPE_SCROLLBAR))
#define XFDASHBOARD_SCROLLBAR_CLASS(klass)		(G_TYPE_CHECK_CLASS_CAST((klass), XFDASHBOARD_TYPE_SCROLLBAR, XfdashboardScrollbarClass))
#define XFDASHBOARD_IS_SCROLLBAR_CLASS(klass)	(G_TYPE_CHECK_CLASS_TYPE((klass), XFDASHBOARD_TYPE_SCROLLBAR))
#define XFDASHBOARD_SCROLLBAR_GET_CLASS(obj)	(G_TYPE_INSTANCE_GET_CLASS((obj), XFDASHBOARD_TYPE_SCROLLBAR, XfdashboardScrollbarClass))

typedef struct _XfdashboardScrollbar				XfdashboardScrollbar; 
typedef struct _XfdashboardScrollbarPrivate		XfdashboardScrollbarPrivate;
typedef struct _XfdashboardScrollbarClass		XfdashboardScrollbarClass;

struct _XfdashboardScrollbar
{
	/* Parent instance */
	ClutterActor			parent_instance;

	/* Private structure */
	XfdashboardScrollbarPrivate	*priv;
};

struct _XfdashboardScrollbarClass
{
	/* Parent class */
	ClutterActorClass		parent_class;

	/* Virtual functions */
	void (*value_changed)(XfdashboardScrollbar *self, gfloat inNewValue);
	void (*range_changed)(XfdashboardScrollbar *self, gfloat inNewValue);
};

GType xfdashboard_scrollbar_get_type(void) G_GNUC_CONST;

ClutterActor* xfdashboard_scrollbar_new(void);
ClutterActor* xfdashboard_scrollbar_new_with_thickness(gfloat inThickness);

gfloat xfdashboard_scrollbar_get_value(XfdashboardScrollbar *self);
void xfdashboard_scrollbar_set_value(XfdashboardScrollbar *self, gfloat inValue);

gfloat xfdashboard_scrollbar_get_range(XfdashboardScrollbar *self);
void xfdashboard_scrollbar_set_range(XfdashboardScrollbar *self, gfloat inRange);

gboolean xfdashboard_scrollbar_get_vertical(XfdashboardScrollbar *self);
void xfdashboard_scrollbar_set_vertical(XfdashboardScrollbar *self, gboolean inIsVertical);

gfloat xfdashboard_scrollbar_get_thickness(XfdashboardScrollbar *self);
void xfdashboard_scrollbar_set_thickness(XfdashboardScrollbar *self, gfloat inThickness);

gfloat xfdashboard_scrollbar_get_spacing(XfdashboardScrollbar *self);
void xfdashboard_scrollbar_set_spacing(XfdashboardScrollbar *self, gfloat inSpacing);

const ClutterColor* xfdashboard_scrollbar_get_color(XfdashboardScrollbar *self);
void xfdashboard_scrollbar_set_color(XfdashboardScrollbar *self, const ClutterColor *inColor);

const ClutterColor* xfdashboard_scrollbar_get_background_color(XfdashboardScrollbar *self);
void xfdashboard_scrollbar_set_background_color(XfdashboardScrollbar *self, const ClutterColor *inColor);

G_END_DECLS

#endif
