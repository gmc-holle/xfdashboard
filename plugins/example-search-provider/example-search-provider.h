/*
 * example-search-provider: An example search provider
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

#ifndef __XFDASHBOARD_EXAMPLE_SEARCH_PROVIDER__
#define __XFDASHBOARD_EXAMPLE_SEARCH_PROVIDER__

#include <libxfdashboard/libxfdashboard.h>

G_BEGIN_DECLS

#define XFDASHBOARD_TYPE_EXAMPLE_SEARCH_PROVIDER			(xfdashboard_example_search_provider_get_type())
#define XFDASHBOARD_EXAMPLE_SEARCH_PROVIDER(obj)			(G_TYPE_CHECK_INSTANCE_CAST((obj), XFDASHBOARD_TYPE_EXAMPLE_SEARCH_PROVIDER, XfdashboardExampleSearchProvider))
#define XFDASHBOARD_IS_EXAMPLE_SEARCH_PROVIDER(obj)			(G_TYPE_CHECK_INSTANCE_TYPE((obj), XFDASHBOARD_TYPE_EXAMPLE_SEARCH_PROVIDER))
#define XFDASHBOARD_EXAMPLE_SEARCH_PROVIDER_CLASS(klass)	(G_TYPE_CHECK_CLASS_CAST((klass), XFDASHBOARD_TYPE_EXAMPLE_SEARCH_PROVIDER, XfdashboardExampleSearchProviderClass))
#define XFDASHBOARD_IS_EXAMPLE_SEARCH_PROVIDER_CLASS(klass)	(G_TYPE_CHECK_CLASS_TYPE((klass), XFDASHBOARD_TYPE_EXAMPLE_SEARCH_PROVIDER))
#define XFDASHBOARD_EXAMPLE_SEARCH_PROVIDER_GET_CLASS(obj)	(G_TYPE_INSTANCE_GET_CLASS((obj), XFDASHBOARD_TYPE_EXAMPLE_SEARCH_PROVIDER, XfdashboardExampleSearchProviderClass))

typedef struct _XfdashboardExampleSearchProvider			XfdashboardExampleSearchProvider; 
typedef struct _XfdashboardExampleSearchProviderPrivate		XfdashboardExampleSearchProviderPrivate;
typedef struct _XfdashboardExampleSearchProviderClass		XfdashboardExampleSearchProviderClass;

struct _XfdashboardExampleSearchProvider
{
	/* Parent instance */
	XfdashboardSearchProvider					parent_instance;

	/* Private structure */
	XfdashboardExampleSearchProviderPrivate		*priv;
};

struct _XfdashboardExampleSearchProviderClass
{
	/*< private >*/
	/* Parent class */
	XfdashboardSearchProviderClass				parent_class;
};

/* Public API */
GType xfdashboard_example_search_provider_get_type(void) G_GNUC_CONST;
void xfdashboard_example_search_provider_type_register(GTypeModule *inModule);

XFDASHBOARD_DECLARE_PLUGIN_TYPE(xfdashboard_example_search_provider);

G_END_DECLS

#endif
