/*
 * plugin: Plugin functions for 'clock-view'
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
#include <config.h>
#endif

#include "plugin.h"

#include <libxfce4util/libxfce4util.h>

#include "view-manager.h"
#include "clock-view.h"


/* IMPLEMENTATION: XfdashboardPlugin */

/* Forward declarations */
G_MODULE_EXPORT void plugin_init(XfdashboardPlugin *self);
G_MODULE_EXPORT void plugin_enable(XfdashboardPlugin *self);
G_MODULE_EXPORT void plugin_disable(XfdashboardPlugin *self);

/* Plugin initialization function */
G_MODULE_EXPORT void plugin_init(XfdashboardPlugin *self)
{
	/* Set up localization */
	xfce_textdomain(GETTEXT_PACKAGE, PACKAGE_LOCALE_DIR, "UTF-8");

	/* Set plugin info */
	xfdashboard_plugin_set_info(self,
								"id", "clock-view",
								"name", _("Clock"),
								"description", _("Adds new a view showing a clock"),
								"author", "Stephan Haller <nomad@froevel.de>",
								NULL);

	/* Register GObject types of this plugin */
	XFDASHBOARD_REGISTER_PLUGIN_TYPE(self, xfdashboard_clock_view);
}

/* Plugin enable function */
G_MODULE_EXPORT void plugin_enable(XfdashboardPlugin *self)
{
	XfdashboardViewManager	*viewManager;

	/* Register view */
	viewManager=xfdashboard_view_manager_get_default();

	xfdashboard_view_manager_register(viewManager, "clock", XFDASHBOARD_TYPE_CLOCK_VIEW);

	g_object_unref(viewManager);
}

/* Plugin disable function */
G_MODULE_EXPORT void plugin_disable(XfdashboardPlugin *self)
{
	XfdashboardViewManager	*viewManager;

	/* Unregister view */
	viewManager=xfdashboard_view_manager_get_default();

	xfdashboard_view_manager_unregister(viewManager, "clock");

	g_object_unref(viewManager);
}
