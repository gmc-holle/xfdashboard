/*
 * gnome-shell-search-provider: A search provider using GnomeShell
 *                              search providers
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

#ifndef __XFDASHBOARD_GNOME_SHELL_SEARCH_PROVIDER__
#define __XFDASHBOARD_GNOME_SHELL_SEARCH_PROVIDER__

#include <libxfdashboard/libxfdashboard.h>

G_BEGIN_DECLS

#define XFDASHBOARD_TYPE_GNOME_SHELL_SEARCH_PROVIDER			(xfdashboard_gnome_shell_search_provider_get_type())
#define XFDASHBOARD_GNOME_SHELL_SEARCH_PROVIDER(obj)			(G_TYPE_CHECK_INSTANCE_CAST((obj), XFDASHBOARD_TYPE_GNOME_SHELL_SEARCH_PROVIDER, XfdashboardGnomeShellSearchProvider))
#define XFDASHBOARD_IS_GNOME_SHELL_SEARCH_PROVIDER(obj)			(G_TYPE_CHECK_INSTANCE_TYPE((obj), XFDASHBOARD_TYPE_GNOME_SHELL_SEARCH_PROVIDER))
#define XFDASHBOARD_GNOME_SHELL_SEARCH_PROVIDER_CLASS(klass)	(G_TYPE_CHECK_CLASS_CAST((klass), XFDASHBOARD_TYPE_GNOME_SHELL_SEARCH_PROVIDER, XfdashboardGnomeShellSearchProviderClass))
#define XFDASHBOARD_IS_GNOME_SHELL_SEARCH_PROVIDER_CLASS(klass)	(G_TYPE_CHECK_CLASS_TYPE((klass), XFDASHBOARD_TYPE_GNOME_SHELL_SEARCH_PROVIDER))
#define XFDASHBOARD_GNOME_SHELL_SEARCH_PROVIDER_GET_CLASS(obj)	(G_TYPE_INSTANCE_GET_CLASS((obj), XFDASHBOARD_TYPE_GNOME_SHELL_SEARCH_PROVIDER, XfdashboardGnomeShellSearchProviderClass))

typedef struct _XfdashboardGnomeShellSearchProvider				XfdashboardGnomeShellSearchProvider; 
typedef struct _XfdashboardGnomeShellSearchProviderPrivate		XfdashboardGnomeShellSearchProviderPrivate;
typedef struct _XfdashboardGnomeShellSearchProviderClass		XfdashboardGnomeShellSearchProviderClass;

struct _XfdashboardGnomeShellSearchProvider
{
	/* Parent instance */
	XfdashboardSearchProvider					parent_instance;

	/* Private structure */
	XfdashboardGnomeShellSearchProviderPrivate	*priv;
};

struct _XfdashboardGnomeShellSearchProviderClass
{
	/*< private >*/
	/* Parent class */
	XfdashboardSearchProviderClass				parent_class;
};

/* Public API */
GType xfdashboard_gnome_shell_search_provider_get_type(void) G_GNUC_CONST;
void xfdashboard_gnome_shell_search_provider_type_register(GTypeModule *inModule);

XFDASHBOARD_DECLARE_PLUGIN_TYPE(xfdashboard_gnome_shell_search_provider);

G_END_DECLS

#endif
