/*
 * view: Abstract class for views, optional with scrollbars
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

#ifndef __LIBXFDASHBOARD_VIEW__
#define __LIBXFDASHBOARD_VIEW__

#if !defined(__LIBXFDASHBOARD_H_INSIDE__) && !defined(LIBXFDASHBOARD_COMPILATION)
#error "Only <libxfdashboard/libxfdashboard.h> can be included directly."
#endif

#include <clutter/clutter.h>

#include <libxfdashboard/actor.h>
#include <libxfdashboard/types.h>
#include <libxfdashboard/focusable.h>

G_BEGIN_DECLS

/* Public definitions */
/**
 * XfdashboardViewFitMode:
 * @XFDASHBOARD_VIEW_FIT_MODE_NONE: Do not try to fit view into viewpad.
 * @XFDASHBOARD_VIEW_FIT_MODE_HORIZONTAL: Try to fit view into viewpad horizontally.
 * @XFDASHBOARD_VIEW_FIT_MODE_VERTICAL: Try to fit view into viewpad vertically.
 * @XFDASHBOARD_VIEW_FIT_MODE_BOTH: Try to fit view into viewpad horizontally and vertically.
 *
 * Determines how a view should fit into a viewpad.
 */
typedef enum /*< prefix=XFDASHBOARD_VIEW_FIT_MODE >*/
{
	XFDASHBOARD_VIEW_FIT_MODE_NONE=0,
	XFDASHBOARD_VIEW_FIT_MODE_HORIZONTAL,
	XFDASHBOARD_VIEW_FIT_MODE_VERTICAL,
	XFDASHBOARD_VIEW_FIT_MODE_BOTH
} XfdashboardViewFitMode;


/* Object declaration */
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
	/*< private >*/
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
	void (*set_view_fit_mode)(XfdashboardView *self, XfdashboardViewFitMode inFitMode);

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

const gchar* xfdashboard_view_get_id(XfdashboardView *self);
gboolean xfdashboard_view_has_id(XfdashboardView *self, const gchar *inID);

const gchar* xfdashboard_view_get_name(XfdashboardView *self);
void xfdashboard_view_set_name(XfdashboardView *self, const gchar *inName);

const gchar* xfdashboard_view_get_icon(XfdashboardView *self);
void xfdashboard_view_set_icon(XfdashboardView *self, const gchar *inIcon);

XfdashboardViewFitMode xfdashboard_view_get_view_fit_mode(XfdashboardView *self);
void xfdashboard_view_set_view_fit_mode(XfdashboardView *self, XfdashboardViewFitMode inFitMode);

gboolean xfdashboard_view_get_enabled(XfdashboardView *self);
void xfdashboard_view_set_enabled(XfdashboardView *self, gboolean inIsEnabled);

void xfdashboard_view_scroll_to(XfdashboardView *self, gfloat inX, gfloat inY);
gboolean xfdashboard_view_child_needs_scroll(XfdashboardView *self, ClutterActor *inActor);
void xfdashboard_view_child_ensure_visible(XfdashboardView *self, ClutterActor *inActor);

gboolean xfdashboard_view_has_focus(XfdashboardView *self);

G_END_DECLS

#endif	/* __LIBXFDASHBOARD_VIEW__ */
