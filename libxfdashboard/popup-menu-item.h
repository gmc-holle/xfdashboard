/*
 * popup-menu-item: An interface implemented by actors used as pop-up menu item
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

#ifndef __LIBXFDASHBOARD_POPUP_MENU_ITEM__
#define __LIBXFDASHBOARD_POPUP_MENU_ITEM__

#if !defined(__LIBXFDASHBOARD_H_INSIDE__) && !defined(LIBXFDASHBOARD_COMPILATION)
#error "Only <libxfdashboard/libxfdashboard.h> can be included directly."
#endif

#include <glib-object.h>

G_BEGIN_DECLS

#define XFDASHBOARD_TYPE_POPUP_MENU_ITEM				(xfdashboard_popup_menu_item_get_type())
#define XFDASHBOARD_POPUP_MENU_ITEM(obj)				(G_TYPE_CHECK_INSTANCE_CAST((obj), XFDASHBOARD_TYPE_POPUP_MENU_ITEM, XfdashboardPopupMenuItem))
#define XFDASHBOARD_IS_POPUP_MENU_ITEM(obj)				(G_TYPE_CHECK_INSTANCE_TYPE((obj), XFDASHBOARD_TYPE_POPUP_MENU_ITEM))
#define XFDASHBOARD_POPUP_MENU_ITEM_GET_IFACE(obj)		(G_TYPE_INSTANCE_GET_INTERFACE((obj), XFDASHBOARD_TYPE_POPUP_MENU_ITEM, XfdashboardPopupMenuItemInterface))

typedef struct _XfdashboardPopupMenuItem				XfdashboardPopupMenuItem;
typedef struct _XfdashboardPopupMenuItemInterface		XfdashboardPopupMenuItemInterface;

/**
 * XfdashboardPopupMenuItemInterface:
 * @parent_interface: The parent interface.
 * @get_enabled: Retrieve state if pop-up menu item is enabled or disabled
 * @set_enabled: Set state if pop-up menu item is enabled or disabled
 *
 * Provides an interface implemented by actors which will be used as pop-up menu
 * items in a #XfdashboardPopupMenu.
 */
struct _XfdashboardPopupMenuItemInterface
{
	/*< private >*/
	/* Parent interface */
	GTypeInterface				parent_interface;

	/*< public >*/
	/* Virtual functions */
	gboolean (*get_enabled)(XfdashboardPopupMenuItem *self);
	void (*set_enabled)(XfdashboardPopupMenuItem *self, gboolean inEnabled);
};

/* Public API */
GType xfdashboard_popup_menu_item_get_type(void) G_GNUC_CONST;

gboolean xfdashboard_popup_menu_item_get_enabled(XfdashboardPopupMenuItem *self);
void xfdashboard_popup_menu_item_set_enabled(XfdashboardPopupMenuItem *self, gboolean inEnabled);

void xfdashboard_popup_menu_item_activate(XfdashboardPopupMenuItem *self);

G_END_DECLS

#endif	/* __LIBXFDASHBOARD_POPUP_MENU_ITEM__ */
