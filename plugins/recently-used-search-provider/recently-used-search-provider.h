/*
 * recently-used-search-provider: A search provider using GTK+ recent
 *   manager as search source
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

#ifndef __XFDASHBOARD_RECENTLY_USED_SEARCH_PROVIDER__
#define __XFDASHBOARD_RECENTLY_USED_SEARCH_PROVIDER__

#include <libxfdashboard/libxfdashboard.h>

G_BEGIN_DECLS

#define XFDASHBOARD_TYPE_RECENTLY_USED_SEARCH_PROVIDER				(xfdashboard_recently_used_search_provider_get_type())
#define XFDASHBOARD_RECENTLY_USED_SEARCH_PROVIDER(obj)				(G_TYPE_CHECK_INSTANCE_CAST((obj), XFDASHBOARD_TYPE_RECENTLY_USED_SEARCH_PROVIDER, XfdashboardRecentlyUsedSearchProvider))
#define XFDASHBOARD_IS_RECENTLY_USED_SEARCH_PROVIDER(obj)			(G_TYPE_CHECK_INSTANCE_TYPE((obj), XFDASHBOARD_TYPE_RECENTLY_USED_SEARCH_PROVIDER))
#define XFDASHBOARD_RECENTLY_USED_SEARCH_PROVIDER_CLASS(klass)		(G_TYPE_CHECK_CLASS_CAST((klass), XFDASHBOARD_TYPE_RECENTLY_USED_SEARCH_PROVIDER, XfdashboardRecentlyUsedSearchProviderClass))
#define XFDASHBOARD_IS_RECENTLY_USED_SEARCH_PROVIDER_CLASS(klass)	(G_TYPE_CHECK_CLASS_TYPE((klass), XFDASHBOARD_TYPE_RECENTLY_USED_SEARCH_PROVIDER))
#define XFDASHBOARD_RECENTLY_USED_SEARCH_PROVIDER_GET_CLASS(obj)	(G_TYPE_INSTANCE_GET_CLASS((obj), XFDASHBOARD_TYPE_RECENTLY_USED_SEARCH_PROVIDER, XfdashboardRecentlyUsedSearchProviderClass))

typedef struct _XfdashboardRecentlyUsedSearchProvider				XfdashboardRecentlyUsedSearchProvider; 
typedef struct _XfdashboardRecentlyUsedSearchProviderPrivate		XfdashboardRecentlyUsedSearchProviderPrivate;
typedef struct _XfdashboardRecentlyUsedSearchProviderClass			XfdashboardRecentlyUsedSearchProviderClass;

struct _XfdashboardRecentlyUsedSearchProvider
{
	/* Parent instance */
	XfdashboardSearchProvider						parent_instance;

	/* Private structure */
	XfdashboardRecentlyUsedSearchProviderPrivate	*priv;
};

struct _XfdashboardRecentlyUsedSearchProviderClass
{
	/*< private >*/
	/* Parent class */
	XfdashboardSearchProviderClass					parent_class;
};

/* Public API */
GType xfdashboard_recently_used_search_provider_get_type(void) G_GNUC_CONST;
void xfdashboard_recently_used_search_provider_type_register(GTypeModule *inModule);

XFDASHBOARD_DECLARE_PLUGIN_TYPE(xfdashboard_recently_used_search_provider);

G_END_DECLS

#endif
