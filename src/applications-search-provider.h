/*
 * applications-search-provider: Search provider for searching installed
 *                               applications
 * 
 * Copyright 2012-2015 Stephan Haller <nomad@froevel.de>
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

#ifndef __XFDASHBOARD_APPLICATIONS_SEARCH_PROVIDER__
#define __XFDASHBOARD_APPLICATIONS_SEARCH_PROVIDER__

#include "search-provider.h"

G_BEGIN_DECLS

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

G_END_DECLS

#endif	/* __XFDASHBOARD_APPLICATIONS_SEARCH_PROVIDER__ */
