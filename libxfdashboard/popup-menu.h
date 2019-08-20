/*
 * popup-menu: A pop-up menu with entries performing an action when an entry
 *             was clicked
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

#ifndef __LIBXFDASHBOARD_POPUP_MENU__
#define __LIBXFDASHBOARD_POPUP_MENU__

#if !defined(__LIBXFDASHBOARD_H_INSIDE__) && !defined(LIBXFDASHBOARD_COMPILATION)
#error "Only <libxfdashboard/libxfdashboard.h> can be included directly."
#endif

#include <libxfdashboard/background.h>
#include <libxfdashboard/popup-menu-item.h>

G_BEGIN_DECLS

#define XFDASHBOARD_TYPE_POPUP_MENU				(xfdashboard_popup_menu_get_type())
#define XFDASHBOARD_POPUP_MENU(obj)				(G_TYPE_CHECK_INSTANCE_CAST((obj), XFDASHBOARD_TYPE_POPUP_MENU, XfdashboardPopupMenu))
#define XFDASHBOARD_IS_POPUP_MENU(obj)			(G_TYPE_CHECK_INSTANCE_TYPE((obj), XFDASHBOARD_TYPE_POPUP_MENU))
#define XFDASHBOARD_POPUP_MENU_CLASS(klass)		(G_TYPE_CHECK_CLASS_CAST((klass), XFDASHBOARD_TYPE_POPUP_MENU, XfdashboardPopupMenuClass))
#define XFDASHBOARD_IS_POPUP_MENU_CLASS(klass)	(G_TYPE_CHECK_CLASS_TYPE((klass), XFDASHBOARD_TYPE_POPUP_MENU))
#define XFDASHBOARD_POPUP_MENU_GET_CLASS(obj)	(G_TYPE_INSTANCE_GET_CLASS((obj), XFDASHBOARD_TYPE_POPUP_MENU, XfdashboardPopupMenuClass))

typedef struct _XfdashboardPopupMenu			XfdashboardPopupMenu;
typedef struct _XfdashboardPopupMenuClass		XfdashboardPopupMenuClass;
typedef struct _XfdashboardPopupMenuPrivate		XfdashboardPopupMenuPrivate;

/**
 * XfdashboardPopupMenu:
 *
 * The #XfdashboardPopupMenu structure contains only private data and
 * should be accessed using the provided API
 */
struct _XfdashboardPopupMenu
{
	/*< private >*/
	/* Parent instance */
	XfdashboardBackground				parent_instance;

	/* Private structure */
	XfdashboardPopupMenuPrivate			*priv;
};

/**
 * XfdashboardPopupMenuClass:
 *
 * The #XfdashboardPopupMenuClass structure contains only private data
 */
struct _XfdashboardPopupMenuClass
{
	/*< private >*/
	/* Parent class */
	XfdashboardBackgroundClass			parent_class;

	/*< public >*/
	/* Virtual functions */
	void (*activated)(XfdashboardPopupMenu *self);
	void (*cancelled)(XfdashboardPopupMenu *self);

	void (*item_activated)(XfdashboardPopupMenu *self, XfdashboardPopupMenuItem *inMenuItem);

	void (*item_added)(XfdashboardPopupMenu *self, XfdashboardPopupMenuItem *inMenuItem);
	void (*item_removed)(XfdashboardPopupMenu *self, XfdashboardPopupMenuItem *inMenuItem);
};

/* Public API */
GType xfdashboard_popup_menu_get_type(void) G_GNUC_CONST;

ClutterActor* xfdashboard_popup_menu_new(void);
ClutterActor* xfdashboard_popup_menu_new_for_source(ClutterActor *inSource);

gboolean xfdashboard_popup_menu_get_destroy_on_cancel(XfdashboardPopupMenu *self);
void xfdashboard_popup_menu_set_destroy_on_cancel(XfdashboardPopupMenu *self, gboolean inDestroyOnCancel);

ClutterActor* xfdashboard_popup_menu_get_source(XfdashboardPopupMenu *self);
void xfdashboard_popup_menu_set_source(XfdashboardPopupMenu *self, ClutterActor *inSource);

gboolean xfdashboard_popup_menu_get_show_title(XfdashboardPopupMenu *self);
void xfdashboard_popup_menu_set_show_title(XfdashboardPopupMenu *self, gboolean inShowTitle);

const gchar* xfdashboard_popup_menu_get_title(XfdashboardPopupMenu *self);
void xfdashboard_popup_menu_set_title(XfdashboardPopupMenu *self, const gchar *inMarkupTitle);

gboolean xfdashboard_popup_menu_get_show_title_icon(XfdashboardPopupMenu *self);
void xfdashboard_popup_menu_set_show_title_icon(XfdashboardPopupMenu *self, gboolean inShowTitleIcon);

const gchar* xfdashboard_popup_menu_get_title_icon_name(XfdashboardPopupMenu *self);
void xfdashboard_popup_menu_set_title_icon_name(XfdashboardPopupMenu *self, const gchar *inIconName);

GIcon* xfdashboard_popup_menu_get_title_gicon(XfdashboardPopupMenu *self);
void xfdashboard_popup_menu_set_title_gicon(XfdashboardPopupMenu *self, GIcon *inIcon);


gint xfdashboard_popup_menu_add_item(XfdashboardPopupMenu *self,
										XfdashboardPopupMenuItem *inMenuItem);
gint xfdashboard_popup_menu_insert_item(XfdashboardPopupMenu *self,
										XfdashboardPopupMenuItem *inMenuItem,
										gint inIndex);
gboolean xfdashboard_popup_menu_move_item(XfdashboardPopupMenu *self,
											XfdashboardPopupMenuItem *inMenuItem,
											gint inIndex);
XfdashboardPopupMenuItem* xfdashboard_popup_menu_get_item(XfdashboardPopupMenu *self, gint inIndex);
gint xfdashboard_popup_menu_get_item_index(XfdashboardPopupMenu *self, XfdashboardPopupMenuItem *inMenuItem);
gboolean xfdashboard_popup_menu_remove_item(XfdashboardPopupMenu *self, XfdashboardPopupMenuItem *inMenuItem);


void xfdashboard_popup_menu_activate(XfdashboardPopupMenu *self);
void xfdashboard_popup_menu_cancel(XfdashboardPopupMenu *self);

G_END_DECLS

#endif	/* __LIBXFDASHBOARD_POPUP_MENU__ */
