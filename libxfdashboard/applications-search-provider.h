/*
 * applications-search-provider: Search provider for searching installed
 *                               applications
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

#ifndef __LIBXFDASHBOARD_APPLICATIONS_SEARCH_PROVIDER__
#define __LIBXFDASHBOARD_APPLICATIONS_SEARCH_PROVIDER__

#if !defined(__LIBXFDASHBOARD_H_INSIDE__) && !defined(LIBXFDASHBOARD_COMPILATION)
#error "Only <libxfdashboard/libxfdashboard.h> can be included directly."
#endif

#include <libxfdashboard/search-provider.h>

G_BEGIN_DECLS

/* Public definitions */
typedef enum /*< flags,prefix=XFDASHBOARD_APPLICATIONS_SEARCH_PROVIDER_SORT_MODE >*/
{
	XFDASHBOARD_APPLICATIONS_SEARCH_PROVIDER_SORT_MODE_NONE=0,

	XFDASHBOARD_APPLICATIONS_SEARCH_PROVIDER_SORT_MODE_NAMES=1 << 0,
	XFDASHBOARD_APPLICATIONS_SEARCH_PROVIDER_SORT_MODE_MOST_USED=1 << 1,
} XfdashboardApplicationsSearchProviderSortMode;


/* Object declaration */
#define XFDASHBOARD_TYPE_APPLICATIONS_SEARCH_PROVIDER				(xfdashboard_applications_search_provider_get_type())
#define XFDASHBOARD_APPLICATIONS_SEARCH_PROVIDER(obj)				(G_TYPE_CHECK_INSTANCE_CAST((obj), XFDASHBOARD_TYPE_APPLICATIONS_SEARCH_PROVIDER, XfdashboardApplicationsSearchProvider))
#define XFDASHBOARD_IS_APPLICATIONS_SEARCH_PROVIDER(obj)			(G_TYPE_CHECK_INSTANCE_TYPE((obj), XFDASHBOARD_TYPE_APPLICATIONS_SEARCH_PROVIDER))
#define XFDASHBOARD_APPLICATIONS_SEARCH_PROVIDER_CLASS(klass)		(G_TYPE_CHECK_CLASS_CAST((klass), XFDASHBOARD_TYPE_APPLICATIONS_SEARCH_PROVIDER, XfdashboardApplicationsSearchProviderClass))
#define XFDASHBOARD_IS_APPLICATIONS_SEARCH_PROVIDER_CLASS(klass)	(G_TYPE_CHECK_CLASS_TYPE((klass), XFDASHBOARD_TYPE_APPLICATIONS_SEARCH_PROVIDER))
#define XFDASHBOARD_APPLICATIONS_SEARCH_PROVIDER_GET_CLASS(obj)		(G_TYPE_INSTANCE_GET_CLASS((obj), XFDASHBOARD_TYPE_APPLICATIONS_SEARCH_PROVIDER, XfdashboardApplicationsSearchProviderClass))

typedef struct _XfdashboardApplicationsSearchProvider				XfdashboardApplicationsSearchProvider; 
typedef struct _XfdashboardApplicationsSearchProviderPrivate		XfdashboardApplicationsSearchProviderPrivate;
typedef struct _XfdashboardApplicationsSearchProviderClass			XfdashboardApplicationsSearchProviderClass;

struct _XfdashboardApplicationsSearchProvider
{
	/*< private >*/
	/* Parent instance */
	XfdashboardSearchProvider						parent_instance;

	/* Private structure */
	XfdashboardApplicationsSearchProviderPrivate	*priv;
};

struct _XfdashboardApplicationsSearchProviderClass
{
	/*< private >*/
	/* Parent class */
	XfdashboardSearchProviderClass					parent_class;

	/*< public >*/
	/* Virtual functions */
};


/* Public API */
GType xfdashboard_applications_search_provider_get_type(void) G_GNUC_CONST;

XfdashboardApplicationsSearchProviderSortMode xfdashboard_applications_search_provider_get_sort_mode(XfdashboardApplicationsSearchProvider *self);
void xfdashboard_applications_search_provider_set_sort_mode(XfdashboardApplicationsSearchProvider *self, const XfdashboardApplicationsSearchProviderSortMode inMode);

G_END_DECLS

#endif	/* __LIBXFDASHBOARD_APPLICATIONS_SEARCH_PROVIDER__ */
