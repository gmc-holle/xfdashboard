/*
 * popup-menu-item-button: A button pop-up menu item
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

#ifndef __LIBXFDASHBOARD_POPUP_MENU_ITEM_BUTTON__
#define __LIBXFDASHBOARD_POPUP_MENU_ITEM_BUTTON__

#if !defined(__LIBXFDASHBOARD_H_INSIDE__) && !defined(LIBXFDASHBOARD_COMPILATION)
#error "Only <libxfdashboard/libxfdashboard.h> can be included directly."
#endif

#include <libxfdashboard/label.h>

G_BEGIN_DECLS

#define XFDASHBOARD_TYPE_POPUP_MENU_ITEM_BUTTON				(xfdashboard_popup_menu_item_button_get_type())
#define XFDASHBOARD_POPUP_MENU_ITEM_BUTTON(obj)				(G_TYPE_CHECK_INSTANCE_CAST((obj), XFDASHBOARD_TYPE_POPUP_MENU_ITEM_BUTTON, XfdashboardPopupMenuItemButton))
#define XFDASHBOARD_IS_POPUP_MENU_ITEM_BUTTON(obj)			(G_TYPE_CHECK_INSTANCE_TYPE((obj), XFDASHBOARD_TYPE_POPUP_MENU_ITEM_BUTTON))
#define XFDASHBOARD_POPUP_MENU_ITEM_BUTTON_CLASS(klass)		(G_TYPE_CHECK_CLASS_CAST((klass), XFDASHBOARD_TYPE_POPUP_MENU_ITEM_BUTTON, XfdashboardPopupMenuItemButtonClass))
#define XFDASHBOARD_IS_POPUP_MENU_ITEM_BUTTON_CLASS(klass)	(G_TYPE_CHECK_CLASS_TYPE((klass), XFDASHBOARD_TYPE_POPUP_MENU_ITEM_BUTTON))
#define XFDASHBOARD_POPUP_MENU_ITEM_BUTTON_GET_CLASS(obj)	(G_TYPE_INSTANCE_GET_CLASS((obj), XFDASHBOARD_TYPE_POPUP_MENU_ITEM_BUTTON, XfdashboardPopupMenuItemButtonClass))

typedef struct _XfdashboardPopupMenuItemButton				XfdashboardPopupMenuItemButton;
typedef struct _XfdashboardPopupMenuItemButtonClass			XfdashboardPopupMenuItemButtonClass;
typedef struct _XfdashboardPopupMenuItemButtonPrivate		XfdashboardPopupMenuItemButtonPrivate;

/**
 * XfdashboardPopupMenuItemButton:
 *
 * The #XfdashboardPopupMenuItemButton structure contains only private data and
 * should be accessed using the provided API
 */
struct _XfdashboardPopupMenuItemButton
{
	/*< private >*/
	/* Parent instance */
	XfdashboardLabel							parent_instance;

	/* Private structure */
	XfdashboardPopupMenuItemButtonPrivate		*priv;
};

/**
 * XfdashboardPopupMenuItemButtonClass:
 *
 * The #XfdashboardPopupMenuItemButtonClass structure contains only private data
 */
struct _XfdashboardPopupMenuItemButtonClass
{
	/*< private >*/
	/* Parent class */
	XfdashboardLabelClass						parent_class;

	/*< public >*/
	/* Virtual functions */
};

/* Public API */
GType xfdashboard_popup_menu_item_button_get_type(void) G_GNUC_CONST;

ClutterActor* xfdashboard_popup_menu_item_button_new(void);
ClutterActor* xfdashboard_popup_menu_item_button_new_with_text(const gchar *inText);

G_END_DECLS

#endif	/* __LIBXFDASHBOARD_POPUP_MENU_ITEM_BUTTON__ */
