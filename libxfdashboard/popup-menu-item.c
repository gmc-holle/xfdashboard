/*
 * popup-menu-item: An interface implemented by actors used as pop-up menu item
 * 
 * Copyright 2012-2017 Stephan Haller <nomad@froevel.de>
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

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <libxfdashboard/popup-menu-item.h>

#include <glib/gi18n-lib.h>

#include <libxfdashboard/compat.h>


/* Define this interface in GObject system */
G_DEFINE_INTERFACE(XfdashboardPopupMenuItem,
					xfdashboard_popup_menu_item,
					G_TYPE_OBJECT)

/* Signals */
enum
{
	/* Signals */
	SIGNAL_ACTIVATED,

	SIGNAL_LAST
};

static guint XfdashboardPopupMenuItemSignals[SIGNAL_LAST]={ 0, };

/* IMPLEMENTATION: Private variables and methods */
#define XFDASHBOARD_POPUP_MENU_ITEM_WARN_NOT_IMPLEMENTED(self, vfunc) \
	g_warning(_("Object of type %s does not implement required virtual function XfdashboardPopupMenuItem::%s"), \
				G_OBJECT_TYPE_NAME(self), \
				vfunc);


/* IMPLEMENTATION: GObject */

/* Interface initialization
 * Set up default functions
 */
void xfdashboard_popup_menu_item_default_init(XfdashboardPopupMenuItemInterface *iface)
{
	static gboolean		initialized=FALSE;
	// TODO: GParamSpec			*property;

	/* Define properties */
	// TODO: property=g_param_spec_boolean("enabled",
									// TODO: _("Enabled"),
									// TODO: _("Whether this pop-up menu item is enabled"),
									// TODO: TRUE,
									// TODO: G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS);
	// TODO: g_object_interface_install_property(iface, property);

	/* Define signals and actions */
	if(!initialized)
	{
		/* Define signals */
		/**
		 * XfdashboardPopupMenuItem::activated:
		 * @self: The pop-up menu item which was activated
		 *
		 * The ::activated signal is emitted for the item the user selected in
		 * the pop-up menu.
		 */
		XfdashboardPopupMenuItemSignals[SIGNAL_ACTIVATED]=
			g_signal_new("activated",
							G_TYPE_FROM_INTERFACE(iface),
							G_SIGNAL_RUN_LAST,
							0,
							NULL,
							NULL,
							g_cclosure_marshal_VOID__VOID,
							G_TYPE_NONE,
							0);

		/* Set flag that base initialization was done for this interface */
		initialized=TRUE;
	}
}

/* IMPLEMENTATION: Public API */

/**
 * xfdashboard_popup_menu_item_activate:
 * @self: A #XfdashboardPopupMenuItem
 *
 * Activates the menu item at @self by emitting the signal "activated".
 */
void xfdashboard_popup_menu_item_activate(XfdashboardPopupMenuItem *self)
{
	g_return_if_fail(XFDASHBOARD_IS_POPUP_MENU_ITEM(self));

	// TODO: Check if popup menu item is enabled and return if disabled

	/* Emit signal for activation */
	g_signal_emit(self, XfdashboardPopupMenuItemSignals[SIGNAL_ACTIVATED], 0);
}
