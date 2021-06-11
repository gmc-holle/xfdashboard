/*
 * plugin: Plugin functions for 'autopin-windows'
 *
 * Copyright 2012-2021 Stephan Haller <nomad@froevel.de>
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

#include <libxfce4util/libxfce4util.h>
#include <gtk/gtk.h>

#include "autopin-windows.h"


/* Forward declarations */
G_MODULE_EXPORT void plugin_init(XfdashboardPlugin *self);


/* IMPLEMENTATION: XfdashboardPlugin */

static XfdashboardAutopinWindows			*autopinWindows=NULL;

/* Plugin enable function */
static void plugin_enable(XfdashboardPlugin *self, gpointer inUserData)
{
	/* Create instance of hot corner */
	if(!autopinWindows)
	{
		autopinWindows=xfdashboard_autopin_windows_new();
	}
}

/* Plugin disable function */
static void plugin_disable(XfdashboardPlugin *self, gpointer inUserData)
{
	/* Destroy instance of hot corner */
	if(autopinWindows)
	{
		g_object_unref(autopinWindows);
		autopinWindows=NULL;
	}
}

/* Plugin initialization function */
G_MODULE_EXPORT void plugin_init(XfdashboardPlugin *self)
{
	/* Set up localization */
	xfce_textdomain(GETTEXT_PACKAGE, PACKAGE_LOCALE_DIR, "UTF-8");

	/* Set plugin info */
	xfdashboard_plugin_set_info(self,
								"name", _("Auto-pin windows"),
								"description", _("Pins windows on secondary windows or unpins windows on primary windows automatically"),
								"author", "Stephan Haller <nomad@froevel.de>",
								NULL);

	/* Register GObject types of this plugin */
	XFDASHBOARD_REGISTER_PLUGIN_TYPE(self, xfdashboard_autopin_windows);

	/* Connect plugin action handlers */
	g_signal_connect(self, "enable", G_CALLBACK(plugin_enable), NULL);
	g_signal_connect(self, "disable", G_CALLBACK(plugin_disable), NULL);
}
