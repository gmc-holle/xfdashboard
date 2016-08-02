/*
 * popup-menu-item-meta: A meta class for menu items in a pop-up menu
 * 
 * Copyright 2012-2016 Stephan Haller <nomad@froevel.de>
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

#ifndef __LIBXFDASHBOARD_POPUP_MENU_ITEM_META__
#define __LIBXFDASHBOARD_POPUP_MENU_ITEM_META__

#if !defined(__LIBXFDASHBOARD_H_INSIDE__) && !defined(LIBXFDASHBOARD_COMPILATION)
#error "Only <libxfdashboard/libxfdashboard.h> can be included directly."
#endif

#include <libxfdashboard/popup-menu.h>

G_BEGIN_DECLS

#define XFDASHBOARD_TYPE_POPUP_MENU_ITEM_META				(xfdashboard_popup_menu_item_meta_get_type())
#define XFDASHBOARD_POPUP_MENU_ITEM_META(obj)				(G_TYPE_CHECK_INSTANCE_CAST((obj), XFDASHBOARD_TYPE_POPUP_MENU_ITEM_META, XfdashboardPopupMenuItemMeta))
#define XFDASHBOARD_IS_POPUP_MENU_ITEM_META(obj)			(G_TYPE_CHECK_INSTANCE_TYPE((obj), XFDASHBOARD_TYPE_POPUP_MENU_ITEM_META))
#define XFDASHBOARD_POPUP_MENU_ITEM_META_CLASS(klass)		(G_TYPE_CHECK_CLASS_CAST((klass), XFDASHBOARD_TYPE_POPUP_MENU_ITEM_META, XfdashboardPopupMenuItemMetaClass))
#define XFDASHBOARD_IS_POPUP_MENU_ITEM_META_CLASS(klass)	(G_TYPE_CHECK_CLASS_TYPE((klass), XFDASHBOARD_TYPE_POPUP_MENU_ITEM_META))
#define XFDASHBOARD_POPUP_MENU_ITEM_META_GET_CLASS(obj)		(G_TYPE_INSTANCE_GET_CLASS((obj), XFDASHBOARD_TYPE_POPUP_MENU_ITEM_META, XfdashboardPopupMenuItemMetaClass))

typedef struct _XfdashboardPopupMenuItemMeta				XfdashboardPopupMenuItemMeta;
typedef struct _XfdashboardPopupMenuItemMetaClass			XfdashboardPopupMenuItemMetaClass;
typedef struct _XfdashboardPopupMenuItemMetaPrivate			XfdashboardPopupMenuItemMetaPrivate;

/**
 * XfdashboardPopupMenuItemMeta:
 *
 * The #XfdashboardPopupMenuItemMeta structure contains only private data and
 * should be accessed using the provided API
 */
struct _XfdashboardPopupMenuItemMeta
{
	/*< private >*/
	/* Parent instance */
	GObject										parent_instance;

	/* Private structure */
	XfdashboardPopupMenuItemMetaPrivate			*priv;
};

/**
 * XfdashboardPopupMenuItemMetaClass:
 *
 * The #XfdashboardPopupMenuItemMetaClass structure contains only private data
 */
struct _XfdashboardPopupMenuItemMetaClass
{
	/*< private >*/
	/* Parent class */
	GObjectClass								parent_class;

	/*< public >*/
	/* Virtual functions */
	void (*activated)(XfdashboardPopupMenuItemMeta *self);
};

/* Public API */
GType xfdashboard_popup_menu_item_meta_get_type(void) G_GNUC_CONST;

XfdashboardPopupMenuItemMeta* xfdashboard_popup_menu_item_meta_new(XfdashboardPopupMenu *inPopupMenu,
																	ClutterActor *inMenuItem,
																	XfdashboardPopupMenuItemActivateCallback inCallback,
																	gpointer inUserData);

XfdashboardPopupMenu* xfdashboard_popup_menu_item_meta_get_popup_menu(XfdashboardPopupMenuItemMeta *self);
ClutterActor* xfdashboard_popup_menu_item_meta_get_menu_item(XfdashboardPopupMenuItemMeta *self);
gpointer xfdashboard_popup_menu_item_meta_get_callback(XfdashboardPopupMenuItemMeta *self);
gpointer xfdashboard_popup_menu_item_meta_get_user_data(XfdashboardPopupMenuItemMeta *self);

void xfdashboard_popup_menu_item_meta_activate(XfdashboardPopupMenuItemMeta *self);

G_END_DECLS

#endif	/* __LIBXFDASHBOARD_POPUP_MENU_ITEM_META__ */
