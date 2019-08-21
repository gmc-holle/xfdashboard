/*
 * search-provider: Abstract class for search providers
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

#ifndef __LIBXFDASHBOARD_SEARCH_PROVIDER__
#define __LIBXFDASHBOARD_SEARCH_PROVIDER__

#if !defined(__LIBXFDASHBOARD_H_INSIDE__) && !defined(LIBXFDASHBOARD_COMPILATION)
#error "Only <libxfdashboard/libxfdashboard.h> can be included directly."
#endif

#include <clutter/clutter.h>

#include <libxfdashboard/search-result-set.h>

G_BEGIN_DECLS

#define XFDASHBOARD_TYPE_SEARCH_PROVIDER				(xfdashboard_search_provider_get_type())
#define XFDASHBOARD_SEARCH_PROVIDER(obj)				(G_TYPE_CHECK_INSTANCE_CAST((obj), XFDASHBOARD_TYPE_SEARCH_PROVIDER, XfdashboardSearchProvider))
#define XFDASHBOARD_IS_SEARCH_PROVIDER(obj)				(G_TYPE_CHECK_INSTANCE_TYPE((obj), XFDASHBOARD_TYPE_SEARCH_PROVIDER))
#define XFDASHBOARD_SEARCH_PROVIDER_CLASS(klass)		(G_TYPE_CHECK_CLASS_CAST((klass), XFDASHBOARD_TYPE_SEARCH_PROVIDER, XfdashboardSearchProviderClass))
#define XFDASHBOARD_IS_SEARCH_PROVIDER_CLASS(klass)		(G_TYPE_CHECK_CLASS_TYPE((klass), XFDASHBOARD_TYPE_SEARCH_PROVIDER))
#define XFDASHBOARD_SEARCH_PROVIDER_GET_CLASS(obj)		(G_TYPE_INSTANCE_GET_CLASS((obj), XFDASHBOARD_TYPE_SEARCH_PROVIDER, XfdashboardSearchProviderClass))

typedef struct _XfdashboardSearchProvider				XfdashboardSearchProvider; 
typedef struct _XfdashboardSearchProviderPrivate		XfdashboardSearchProviderPrivate;
typedef struct _XfdashboardSearchProviderClass			XfdashboardSearchProviderClass;

struct _XfdashboardSearchProvider
{
	/*< private >*/
	/* Parent instance */
	GObject								parent_instance;

	/* Private structure */
	XfdashboardSearchProviderPrivate	*priv;
};

struct _XfdashboardSearchProviderClass
{
	/*< private >*/
	/* Parent class */
	GObjectClass						parent_class;

	/*< public >*/
	/* Virtual functions */
	void (*initialize)(XfdashboardSearchProvider *self);

	const gchar* (*get_name)(XfdashboardSearchProvider *self);
	const gchar* (*get_icon)(XfdashboardSearchProvider *self);

	XfdashboardSearchResultSet* (*get_result_set)(XfdashboardSearchProvider *self,
													const gchar **inSearchTerms,
													XfdashboardSearchResultSet *inPreviousResultSet);

	ClutterActor* (*create_result_actor)(XfdashboardSearchProvider *self,
											GVariant *inResultItem);

	gboolean (*launch_search)(XfdashboardSearchProvider *self,
								const gchar **inSearchTerms);

	gboolean (*activate_result)(XfdashboardSearchProvider* self,
								GVariant *inResultItem,
								ClutterActor *inActor,
								const gchar **inSearchTerms);
};

/* Public API */
GType xfdashboard_search_provider_get_type(void) G_GNUC_CONST;

const gchar* xfdashboard_search_provider_get_id(XfdashboardSearchProvider *self);
gboolean xfdashboard_search_provider_has_id(XfdashboardSearchProvider *self, const gchar *inID);

const gchar* xfdashboard_search_provider_get_name(XfdashboardSearchProvider *self);
const gchar* xfdashboard_search_provider_get_icon(XfdashboardSearchProvider *self);

XfdashboardSearchResultSet* xfdashboard_search_provider_get_result_set(XfdashboardSearchProvider *self,
																		const gchar **inSearchTerms,
																		XfdashboardSearchResultSet *inPreviousResultSet);

ClutterActor* xfdashboard_search_provider_create_result_actor(XfdashboardSearchProvider *self,
																GVariant *inResultItem);

gboolean xfdashboard_search_provider_launch_search(XfdashboardSearchProvider *self,
													const gchar **inSearchTerms);

gboolean xfdashboard_search_provider_activate_result(XfdashboardSearchProvider* self,
														GVariant *inResultItem,
														ClutterActor *inActor,
														const gchar **inSearchTerms);

G_END_DECLS

#endif	/* __LIBXFDASHBOARD_SEARCH_PROVIDER__ */
