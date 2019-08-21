/*
 * focusable: An interface which can be inherited by actors to get
 *            managed by focus manager for keyboard navigation and
 *            selection handling
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

#ifndef __LIBXFDASHBOARD_FOCUSABLE__
#define __LIBXFDASHBOARD_FOCUSABLE__

#if !defined(__LIBXFDASHBOARD_H_INSIDE__) && !defined(LIBXFDASHBOARD_COMPILATION)
#error "Only <libxfdashboard/libxfdashboard.h> can be included directly."
#endif

#include <clutter/clutter.h>

#include <libxfdashboard/types.h>

G_BEGIN_DECLS

#define XFDASHBOARD_TYPE_FOCUSABLE				(xfdashboard_focusable_get_type())
#define XFDASHBOARD_FOCUSABLE(obj)				(G_TYPE_CHECK_INSTANCE_CAST((obj), XFDASHBOARD_TYPE_FOCUSABLE, XfdashboardFocusable))
#define XFDASHBOARD_IS_FOCUSABLE(obj)			(G_TYPE_CHECK_INSTANCE_TYPE((obj), XFDASHBOARD_TYPE_FOCUSABLE))
#define XFDASHBOARD_FOCUSABLE_GET_IFACE(obj)	(G_TYPE_INSTANCE_GET_INTERFACE((obj), XFDASHBOARD_TYPE_FOCUSABLE, XfdashboardFocusableInterface))

typedef struct _XfdashboardFocusable			XfdashboardFocusable;
typedef struct _XfdashboardFocusableInterface	XfdashboardFocusableInterface;

struct _XfdashboardFocusableInterface
{
	/*< private >*/
	/* Parent interface */
	GTypeInterface				parent_interface;

	/*< public >*/
	/* Virtual functions */
	gboolean (*can_focus)(XfdashboardFocusable *self);
	void (*set_focus)(XfdashboardFocusable *self);
	void (*unset_focus)(XfdashboardFocusable *self);

	gboolean (*supports_selection)(XfdashboardFocusable *self);
	ClutterActor* (*get_selection)(XfdashboardFocusable *self);
	gboolean (*set_selection)(XfdashboardFocusable *self, ClutterActor *inSelection);
	ClutterActor* (*find_selection)(XfdashboardFocusable *self, ClutterActor *inSelection, XfdashboardSelectionTarget inDirection);
	gboolean (*activate_selection)(XfdashboardFocusable *self, ClutterActor *inSelection);

	/* Binding actions */
	gboolean (*selection_move_left)(XfdashboardFocusable *self,
											XfdashboardFocusable *inSource,
											const gchar *inAction,
											ClutterEvent *inEvent);
	gboolean (*selection_move_right)(XfdashboardFocusable *self,
											XfdashboardFocusable *inSource,
											const gchar *inAction,
											ClutterEvent *inEvent);
	gboolean (*selection_move_up)(XfdashboardFocusable *self,
											XfdashboardFocusable *inSource,
											const gchar *inAction,
											ClutterEvent *inEvent);
	gboolean (*selection_move_down)(XfdashboardFocusable *self,
											XfdashboardFocusable *inSource,
											const gchar *inAction,
											ClutterEvent *inEvent);
	gboolean (*selection_move_first)(XfdashboardFocusable *self,
											XfdashboardFocusable *inSource,
											const gchar *inAction,
											ClutterEvent *inEvent);
	gboolean (*selection_move_last)(XfdashboardFocusable *self,
											XfdashboardFocusable *inSource,
											const gchar *inAction,
											ClutterEvent *inEvent);
	gboolean (*selection_move_next)(XfdashboardFocusable *self,
											XfdashboardFocusable *inSource,
											const gchar *inAction,
											ClutterEvent *inEvent);
	gboolean (*selection_move_previous)(XfdashboardFocusable *self,
											XfdashboardFocusable *inSource,
											const gchar *inAction,
											ClutterEvent *inEvent);
	gboolean (*selection_move_page_left)(XfdashboardFocusable *self,
											XfdashboardFocusable *inSource,
											const gchar *inAction,
											ClutterEvent *inEvent);
	gboolean (*selection_move_page_right)(XfdashboardFocusable *self,
											XfdashboardFocusable *inSource,
											const gchar *inAction,
											ClutterEvent *inEvent);
	gboolean (*selection_move_page_up)(XfdashboardFocusable *self,
											XfdashboardFocusable *inSource,
											const gchar *inAction,
											ClutterEvent *inEvent);
	gboolean (*selection_move_page_down)(XfdashboardFocusable *self,
											XfdashboardFocusable *inSource,
											const gchar *inAction,
											ClutterEvent *inEvent);
	gboolean (*selection_activate)(XfdashboardFocusable *self,
											XfdashboardFocusable *inSource,
											const gchar *inAction,
											ClutterEvent *inEvent);

	gboolean (*focus_move_to)(XfdashboardFocusable *self,
								XfdashboardFocusable *inSource,
								const gchar *inAction,
								ClutterEvent *inEvent);
};

/* Public API */
GType xfdashboard_focusable_get_type(void) G_GNUC_CONST;

gboolean xfdashboard_focusable_can_focus(XfdashboardFocusable *self);
void xfdashboard_focusable_set_focus(XfdashboardFocusable *self);
void xfdashboard_focusable_unset_focus(XfdashboardFocusable *self);

gboolean xfdashboard_focusable_supports_selection(XfdashboardFocusable *self);
ClutterActor* xfdashboard_focusable_get_selection(XfdashboardFocusable *self);
gboolean xfdashboard_focusable_set_selection(XfdashboardFocusable *self, ClutterActor *inSelection);
ClutterActor* xfdashboard_focusable_find_selection(XfdashboardFocusable *self, ClutterActor *inSelection, XfdashboardSelectionTarget inDirection);
gboolean xfdashboard_focusable_activate_selection(XfdashboardFocusable *self, ClutterActor *inSelection);

gboolean xfdashboard_focusable_move_focus_to(XfdashboardFocusable *self);

G_END_DECLS

#endif	/* __LIBXFDASHBOARD_FOCUSABLE__ */
