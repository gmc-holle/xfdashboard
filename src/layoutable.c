/*
 * layoutable: An interface which can be inherited by buildable objects
 *             from theme layout to get notified about various state
 *             while building
 * 
 * Copyright 2012-2014 Stephan Haller <nomad@froevel.de>
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

#include "layoutable.h"

#include <glib/gi18n-lib.h>

/* Define this interface in GObject system */
G_DEFINE_INTERFACE(XfdashboardLayoutable,
					xfdashboard_layoutable,
					G_TYPE_OBJECT)

/* IMPLEMENTATION: Private variables and methods */

/* Default function: layout_completed */
static void _xfdashboard_layoutable_real_layout_completed(XfdashboardLayoutable *self)
{
	g_return_if_fail(XFDASHBOARD_IS_LAYOUTABLE(self));

	/* Do nothing by default */
	g_message("%s: %p[%s]", __func__, self, G_OBJECT_TYPE_NAME(self));
}

/* IMPLEMENTATION: GObject */

/* Interface initialization
 * Set up default functions
 */
void xfdashboard_layoutable_default_init(XfdashboardLayoutableInterface *iface)
{
	iface->layout_completed=_xfdashboard_layoutable_real_layout_completed;
}

/* Implementation: Public API */

/* Call virtual function "layout_completed" */
void xfdashboard_layoutable_layout_completed(XfdashboardLayoutable *self)
{
	XfdashboardLayoutableInterface		*iface;

	g_return_if_fail(XFDASHBOARD_IS_LAYOUTABLE(self));

	iface=XFDASHBOARD_LAYOUTABLE_GET_IFACE(self);

	/* Call virtual function */
	if(iface->layout_completed)
	{
		iface->layout_completed(self);
		return;
	}

	/* If we get here the virtual function was not overridden */
	g_warning(_("Object of type %s does not implement virtual function of XfdashboardLayoutableInterface::%s"),
				G_OBJECT_TYPE_NAME(self),
				"layout_completed");
}
