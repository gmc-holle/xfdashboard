/*
 * view: Abstract class for views, optional with scrollbars
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

#ifndef __XFDASHBOARD_VIEW__
#define __XFDASHBOARD_VIEW__

#include <clutter/clutter.h>

#include "actor.h"
#include "types.h"
#include "focusable.h"

G_BEGIN_DECLS

#define XFDASHBOARD_TYPE_VIEW				(xfdashboard_view_get_type())
#define XFDASHBOARD_VIEW(obj)				(G_TYPE_CHECK_INSTANCE_CAST((obj), XFDASHBOARD_TYPE_VIEW, XfdashboardView))
#define XFDASHBOARD_IS_VIEW(obj)			(G_TYPE_CHECK_INSTANCE_TYPE((obj), XFDASHBOARD_TYPE_VIEW))
#define XFDASHBOARD_VIEW_CLASS(klass)		(G_TYPE_CHECK_CLASS_CAST((klass), XFDASHBOARD_TYPE_VIEW, XfdashboardViewClass))
#define XFDASHBOARD_IS_VIEW_CLASS(klass)	(G_TYPE_CHECK_CLASS_TYPE((klass), XFDASHBOARD_TYPE_VIEW))
#define XFDASHBOARD_VIEW_GET_CLASS(obj)		(G_TYPE_INSTANCE_GET_CLASS((obj), XFDASHBOARD_TYPE_VIEW, XfdashboardViewClass))

typedef struct _XfdashboardView				XfdashboardView; 
typedef struct _XfdashboardViewPrivate		XfdashboardViewPrivate;
typedef struct _XfdashboardViewClass		XfdashboardViewClass;

struct _XfdashboardView
{
	/* Parent instance */
	XfdashboardActor			parent_instance;

	/* Private structure */
	XfdashboardViewPrivate		*priv;
};

struct _XfdashboardViewClass
{
	/*< private >*/
	/* Parent class */
	XfdashboardActorClass		parent_class;

	/*< public >*/
	/* Virtual functions */
	void (*set_fit_mode)(XfdashboardView *self, XfdashboardFitMode inFitMode);

	void (*activating)(XfdashboardView *self);
	void (*activated)(XfdashboardView *self);
	void (*deactivating)(XfdashboardView *self);
	void (*deactivated)(XfdashboardView *self);

	void (*enabling)(XfdashboardView *self);
	void (*enabled)(XfdashboardView *self);
	void (*disabling)(XfdashboardView *self);
	void (*disabled)(XfdashboardView *self);

	void (*name_changed)(XfdashboardView *self, gchar *inName);
	void (*icon_changed)(XfdashboardView *self, ClutterImage *inIcon);

	void (*scroll_to)(XfdashboardView *self, gfloat inX, gfloat inY);
	gboolean (*child_needs_scroll)(XfdashboardView *self, ClutterActor *inActor);
	void (*child_ensure_visible)(XfdashboardView *self, ClutterActor *inActor);

	/* Binding actions */
	gboolean (*view_activate)(XfdashboardView *self,
								XfdashboardFocusable *inSource,
								const gchar *inAction,
								ClutterEvent *inEvent);
};

/* Public API */
GType xfdashboard_view_get_type(void) G_GNUC_CONST;

const gchar* xfdashboard_view_get_internal_name(XfdashboardView *self);
void xfdashboard_view_set_internal_name(XfdashboardView *self, const gchar *inInternalName);

const gchar* xfdashboard_view_get_name(XfdashboardView *self);
void xfdashboard_view_set_name(XfdashboardView *self, const gchar *inName);

const gchar* xfdashboard_view_get_icon(XfdashboardView *self);
void xfdashboard_view_set_icon(XfdashboardView *self, const gchar *inIcon);

XfdashboardFitMode xfdashboard_view_get_fit_mode(XfdashboardView *self);
void xfdashboard_view_set_fit_mode(XfdashboardView *self, XfdashboardFitMode inFitMode);

gboolean xfdashboard_view_get_enabled(XfdashboardView *self);
void xfdashboard_view_set_enabled(XfdashboardView *self, gboolean inIsEnabled);

void xfdashboard_view_scroll_to(XfdashboardView *self, gfloat inX, gfloat inY);
gboolean xfdashboard_view_child_needs_scroll(XfdashboardView *self, ClutterActor *inActor);
void xfdashboard_view_child_ensure_visible(XfdashboardView *self, ClutterActor *inActor);

gboolean xfdashboard_view_has_focus(XfdashboardView *self);

G_END_DECLS

#endif	/* __XFDASHBOARD_VIEW__ */
