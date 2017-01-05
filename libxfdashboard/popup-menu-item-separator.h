/*
 * popup-menu-item-separator: A separator menu item
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

#ifndef __LIBXFDASHBOARD_POPUP_MENU_ITEM_SEPARATOR__
#define __LIBXFDASHBOARD_POPUP_MENU_ITEM_SEPARATOR__

#if !defined(__LIBXFDASHBOARD_H_INSIDE__) && !defined(LIBXFDASHBOARD_COMPILATION)
#error "Only <libxfdashboard/libxfdashboard.h> can be included directly."
#endif

#include <libxfdashboard/actor.h>

G_BEGIN_DECLS

#define XFDASHBOARD_TYPE_POPUP_MENU_ITEM_SEPARATOR				(xfdashboard_popup_menu_item_separator_get_type())
#define XFDASHBOARD_POPUP_MENU_ITEM_SEPARATOR(obj)				(G_TYPE_CHECK_INSTANCE_CAST((obj), XFDASHBOARD_TYPE_POPUP_MENU_ITEM_SEPARATOR, XfdashboardPopupMenuItemSeparator))
#define XFDASHBOARD_IS_POPUP_MENU_ITEM_SEPARATOR(obj)			(G_TYPE_CHECK_INSTANCE_TYPE((obj), XFDASHBOARD_TYPE_POPUP_MENU_ITEM_SEPARATOR))
#define XFDASHBOARD_POPUP_MENU_ITEM_SEPARATOR_CLASS(klass)		(G_TYPE_CHECK_CLASS_CAST((klass), XFDASHBOARD_TYPE_POPUP_MENU_ITEM_SEPARATOR, XfdashboardPopupMenuItemSeparatorClass))
#define XFDASHBOARD_IS_POPUP_MENU_ITEM_SEPARATOR_CLASS(klass)	(G_TYPE_CHECK_CLASS_TYPE((klass), XFDASHBOARD_TYPE_POPUP_MENU_ITEM_SEPARATOR))
#define XFDASHBOARD_POPUP_MENU_ITEM_SEPARATOR_GET_CLASS(obj)	(G_TYPE_INSTANCE_GET_CLASS((obj), XFDASHBOARD_TYPE_POPUP_MENU_ITEM_SEPARATOR, XfdashboardPopupMenuItemSeparatorClass))

typedef struct _XfdashboardPopupMenuItemSeparator				XfdashboardPopupMenuItemSeparator;
typedef struct _XfdashboardPopupMenuItemSeparatorClass			XfdashboardPopupMenuItemSeparatorClass;
typedef struct _XfdashboardPopupMenuItemSeparatorPrivate		XfdashboardPopupMenuItemSeparatorPrivate;

/**
 * XfdashboardPopupMenuItemSeparator:
 *
 * The #XfdashboardPopupMenuItemSeparator structure contains only private data and
 * should be accessed using the provided API
 */
struct _XfdashboardPopupMenuItemSeparator
{
	/*< private >*/
	/* Parent instance */
	XfdashboardActor								parent_instance;

	/* Private structure */
	XfdashboardPopupMenuItemSeparatorPrivate		*priv;
};

/**
 * XfdashboardPopupMenuItemSeparatorClass:
 *
 * The #XfdashboardPopupMenuItemSeparatorClass structure contains only private data
 */
struct _XfdashboardPopupMenuItemSeparatorClass
{
	/*< private >*/
	/* Parent class */
	XfdashboardActorClass							parent_class;

	/*< public >*/
	/* Virtual functions */
};

/* Public API */
GType xfdashboard_popup_menu_item_separator_get_type(void) G_GNUC_CONST;

ClutterActor* xfdashboard_popup_menu_item_separator_new(void);

G_END_DECLS

#endif	/* __LIBXFDASHBOARD_POPUP_MENU_ITEM_SEPARATOR__ */
