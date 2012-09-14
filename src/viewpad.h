/*
 * viewpad.h: A viewpad managing views
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

#ifndef __XFDASHBOARD_VIEWPAD__
#define __XFDASHBOARD_VIEWPAD__

#include <clutter/clutter.h>
#include <gtk/gtk.h>

#include "view.h"
#include "scrollbar.h"

G_BEGIN_DECLS

#define XFDASHBOARD_TYPE_VIEWPAD				(xfdashboard_viewpad_get_type())
#define XFDASHBOARD_VIEWPAD(obj)				(G_TYPE_CHECK_INSTANCE_CAST((obj), XFDASHBOARD_TYPE_VIEWPAD, XfdashboardViewpad))
#define XFDASHBOARD_IS_VIEWPAD(obj)			(G_TYPE_CHECK_INSTANCE_TYPE((obj), XFDASHBOARD_TYPE_VIEWPAD))
#define XFDASHBOARD_VIEWPAD_CLASS(klass)		(G_TYPE_CHECK_CLASS_CAST((klass), XFDASHBOARD_TYPE_VIEWPAD, XfdashboardViewpadClass))
#define XFDASHBOARD_IS_VIEWPAD_CLASS(klass)	(G_TYPE_CHECK_CLASS_TYPE((klass), XFDASHBOARD_TYPE_VIEWPAD))
#define XFDASHBOARD_VIEWPAD_GET_CLASS(obj)		(G_TYPE_INSTANCE_GET_CLASS((obj), XFDASHBOARD_TYPE_VIEWPAD, XfdashboardViewpadClass))

typedef struct _XfdashboardViewpad				XfdashboardViewpad; 
typedef struct _XfdashboardViewpadPrivate		XfdashboardViewpadPrivate;
typedef struct _XfdashboardViewpadClass		XfdashboardViewpadClass;

struct _XfdashboardViewpad
{
	/* Parent instance */
	ClutterActor				parent_instance;

	/* Private structure */
	XfdashboardViewpadPrivate	*priv;
};

struct _XfdashboardViewpadClass
{
	/* Parent class */
	ClutterActorClass			parent_class;

	/* Virtual functions */
	void (*view_added)(XfdashboardViewpad *self, XfdashboardView *inView);
	void (*view_removed)(XfdashboardViewpad *self, XfdashboardView *inView);
	void (*view_activated)(XfdashboardViewpad *self, XfdashboardView *inView);
	void (*view_deactivated)(XfdashboardViewpad *self, XfdashboardView *inView);
};

GType xfdashboard_viewpad_get_type(void) G_GNUC_CONST;

ClutterActor* xfdashboard_viewpad_new();

GList* xfdashboard_viewpad_get_views(XfdashboardViewpad *self);
void xfdashboard_viewpad_add_view(XfdashboardViewpad *self, XfdashboardView *inView);
void xfdashboard_viewpad_remove_view(XfdashboardViewpad *self, XfdashboardView *inView);

XfdashboardView* xfdashboard_viewpad_get_active_view(XfdashboardViewpad *self);
void xfdashboard_viewpad_set_active_view(XfdashboardViewpad *self, XfdashboardView *inView);

XfdashboardScrollbar* xfdashboard_viewpad_get_vertical_scrollbar(XfdashboardViewpad *self);
XfdashboardScrollbar* xfdashboard_viewpad_get_horizontal_scrollbar(XfdashboardViewpad *self);

void xfdashboard_viewpad_get_scrollbar_policy(XfdashboardViewpad *self,
												GtkPolicyType *outHorizontalPolicy,
												GtkPolicyType *outVerticalPolicy);
void xfdashboard_viewpad_set_scrollbar_policy(XfdashboardViewpad *self,
												GtkPolicyType inHorizontalPolicy,
												GtkPolicyType inVerticalPolicy);

gfloat xfdashboard_viewpad_get_thickness(XfdashboardViewpad *self);
void xfdashboard_viewpad_set_thickness(XfdashboardViewpad *self, gfloat inThickness);

G_END_DECLS

#endif
