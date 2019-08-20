/*
 * viewpad: A viewpad managing views
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

#ifndef __LIBXFDASHBOARD_VIEWPAD__
#define __LIBXFDASHBOARD_VIEWPAD__

#if !defined(__LIBXFDASHBOARD_H_INSIDE__) && !defined(LIBXFDASHBOARD_COMPILATION)
#error "Only <libxfdashboard/libxfdashboard.h> can be included directly."
#endif

#include <clutter/clutter.h>

#include <libxfdashboard/background.h>
#include <libxfdashboard/view.h>
#include <libxfdashboard/types.h>

G_BEGIN_DECLS

#define XFDASHBOARD_TYPE_VIEWPAD				(xfdashboard_viewpad_get_type())
#define XFDASHBOARD_VIEWPAD(obj)				(G_TYPE_CHECK_INSTANCE_CAST((obj), XFDASHBOARD_TYPE_VIEWPAD, XfdashboardViewpad))
#define XFDASHBOARD_IS_VIEWPAD(obj)				(G_TYPE_CHECK_INSTANCE_TYPE((obj), XFDASHBOARD_TYPE_VIEWPAD))
#define XFDASHBOARD_VIEWPAD_CLASS(klass)		(G_TYPE_CHECK_CLASS_CAST((klass), XFDASHBOARD_TYPE_VIEWPAD, XfdashboardViewpadClass))
#define XFDASHBOARD_IS_VIEWPAD_CLASS(klass)		(G_TYPE_CHECK_CLASS_TYPE((klass), XFDASHBOARD_TYPE_VIEWPAD))
#define XFDASHBOARD_VIEWPAD_GET_CLASS(obj)		(G_TYPE_INSTANCE_GET_CLASS((obj), XFDASHBOARD_TYPE_VIEWPAD, XfdashboardViewpadClass))

typedef struct _XfdashboardViewpad				XfdashboardViewpad; 
typedef struct _XfdashboardViewpadPrivate		XfdashboardViewpadPrivate;
typedef struct _XfdashboardViewpadClass			XfdashboardViewpadClass;

struct _XfdashboardViewpad
{
	/*< private >*/
	/* Parent instance */
	XfdashboardBackground		parent_instance;

	/* Private structure */
	XfdashboardViewpadPrivate	*priv;
};

struct _XfdashboardViewpadClass
{
	/*< private >*/
	/* Parent class */
	XfdashboardBackgroundClass	parent_class;

	/*< public >*/
	/* Virtual functions */
	void (*view_added)(XfdashboardViewpad *self, XfdashboardView *inView);
	void (*view_removed)(XfdashboardViewpad *self, XfdashboardView *inView);

	void (*view_activating)(XfdashboardViewpad *self, XfdashboardView *inView);
	void (*view_activated)(XfdashboardViewpad *self, XfdashboardView *inView);
	void (*view_deactivating)(XfdashboardViewpad *self, XfdashboardView *inView);
	void (*view_deactivated)(XfdashboardViewpad *self, XfdashboardView *inView);
};

/* Public API */
GType xfdashboard_viewpad_get_type(void) G_GNUC_CONST;

ClutterActor* xfdashboard_viewpad_new(void);

gfloat xfdashboard_viewpad_get_spacing(XfdashboardViewpad *self);
void xfdashboard_viewpad_set_spacing(XfdashboardViewpad *self, gfloat inSpacing);

GList* xfdashboard_viewpad_get_views(XfdashboardViewpad *self);
gboolean xfdashboard_viewpad_has_view(XfdashboardViewpad *self, XfdashboardView *inView);
XfdashboardView* xfdashboard_viewpad_find_view_by_type(XfdashboardViewpad *self, GType inType);
XfdashboardView* xfdashboard_viewpad_find_view_by_id(XfdashboardViewpad *self, const gchar *inID);

XfdashboardView* xfdashboard_viewpad_get_active_view(XfdashboardViewpad *self);
void xfdashboard_viewpad_set_active_view(XfdashboardViewpad *self, XfdashboardView *inView);

gboolean xfdashboard_viewpad_get_horizontal_scrollbar_visible(XfdashboardViewpad *self);
gboolean xfdashboard_viewpad_get_vertical_scrollbar_visible(XfdashboardViewpad *self);

XfdashboardVisibilityPolicy xfdashboard_viewpad_get_horizontal_scrollbar_policy(XfdashboardViewpad *self);
void xfdashboard_viewpad_set_horizontal_scrollbar_policy(XfdashboardViewpad *self, XfdashboardVisibilityPolicy inPolicy);

XfdashboardVisibilityPolicy xfdashboard_viewpad_get_vertical_scrollbar_policy(XfdashboardViewpad *self);
void xfdashboard_viewpad_set_vertical_scrollbar_policy(XfdashboardViewpad *self, XfdashboardVisibilityPolicy inPolicy);

G_END_DECLS

#endif	/* __LIBXFDASHBOARD_VIEWPAD__ */
