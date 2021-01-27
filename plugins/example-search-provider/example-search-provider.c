/*
 * example-search-provider: An example search provider
 * 
 * Copyright 2012-2020 Stephan Haller <nomad@froevel.de>
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

#include "example-search-provider.h"

#include <glib/gi18n-lib.h>
#include <gio/gio.h>


/* Define this class in GObject system */
struct _XfdashboardExampleSearchProviderPrivate
{
	/* Instance related */
	gint			reserved;
};

G_DEFINE_DYNAMIC_TYPE_EXTENDED(XfdashboardExampleSearchProvider,
								xfdashboard_example_search_provider,
								XFDASHBOARD_TYPE_SEARCH_PROVIDER,
								0,
								G_ADD_PRIVATE_DYNAMIC(XfdashboardExampleSearchProvider))

/* Define this class in this plugin */
XFDASHBOARD_DEFINE_PLUGIN_TYPE(xfdashboard_example_search_provider);


/* IMPLEMENTATION: XfdashboardSearchProvider */

/* One-time initialization of search provider */
static void _xfdashboard_example_search_provider_initialize(XfdashboardSearchProvider *inProvider)
{
	g_return_if_fail(XFDASHBOARD_IS_EXAMPLE_SEARCH_PROVIDER(inProvider));

	/* Initialize search provider */
	/* TODO: Here the search provider is initialized. This function is only called once after
	 *       the search provider was enabled.
	 */
}

/* Get display name for this search provider */
static const gchar* _xfdashboard_example_search_provider_get_name(XfdashboardSearchProvider *inProvider)
{
	return(_("Example search"));
}

/* Get icon-name for this search provider */
static const gchar* _xfdashboard_example_search_provider_get_icon(XfdashboardSearchProvider *inProvider)
{
	return("edit-find");
}

/* Get result set for requested search terms */
static XfdashboardSearchResultSet* _xfdashboard_example_search_provider_get_result_set(XfdashboardSearchProvider *inProvider,
																						const gchar **inSearchTerms,
																						XfdashboardSearchResultSet *inPreviousResultSet)
{
	XfdashboardSearchResultSet						*resultSet;
	GVariant										*resultItem;
	gchar											*resultItemTitle;

	g_return_val_if_fail(XFDASHBOARD_IS_EXAMPLE_SEARCH_PROVIDER(inProvider), NULL);

	/* Create empty result set to store matching result items */
	resultSet=xfdashboard_search_result_set_new();

	/* Create result item */
	/* TOOO: This example just creates one long string from entered search terms
	 *       as search result. More complex data are possible as GVariant is used
	 *       in result set for each result item.
	 */
	resultItemTitle=g_strjoinv(" ", (gchar**)inSearchTerms);
	resultItem=g_variant_new_string(resultItemTitle);
	g_free(resultItemTitle);

	/* Add result item to result set */
	xfdashboard_search_result_set_add_item(resultSet, resultItem);
	/* TODO: This example search provider assumes that each result item is a
	 *       full match and sets a score of 1. This score is used to determine
	 *       the relevance of this result item against the search terms entered.
	 *       The score must be a float value between 0.0f and 1.0f.
	 */
	xfdashboard_search_result_set_set_item_score(resultSet, resultItem, 1.0f);

	/* Return result set */
	return(resultSet);
}

/* Create actor for a result item of the result set returned from a search request */
static ClutterActor* _xfdashboard_example_search_provider_create_result_actor(XfdashboardSearchProvider *inProvider,
																				GVariant *inResultItem)
{
	ClutterActor									*actor;
	gchar											*title;

	g_return_val_if_fail(XFDASHBOARD_IS_EXAMPLE_SEARCH_PROVIDER(inProvider), NULL);
	g_return_val_if_fail(inResultItem, NULL);

	/* Create actor for result item */
	/* TODO: This example search provider will just create a button with a title
	 *       taken from the result item. More complex actors are possible.
	 */
	title=g_markup_printf_escaped("<b>%s</b>\n\nSearch for '%s' with search provider plugin '%s'", g_variant_get_string(inResultItem, NULL), g_variant_get_string(inResultItem, NULL), PLUGIN_ID);
	actor=xfdashboard_button_new_with_text(title);
	g_free(title);

	/* Return created actor */
	return(actor);
}

/* Activate result item */
static gboolean _xfdashboard_example_search_provider_activate_result(XfdashboardSearchProvider* inProvider,
																		GVariant *inResultItem,
																		ClutterActor *inActor,
																		const gchar **inSearchTerms)
{
	g_return_val_if_fail(XFDASHBOARD_IS_EXAMPLE_SEARCH_PROVIDER(inProvider), FALSE);
	g_return_val_if_fail(inResultItem, FALSE);
	
	/* Activate result item */
	/* TODO: Here you have to perform the default action when a result item of
	 *       this search provider was activated, i.e. clicked.
	 */

	/* If we get here activating result item was successful, so return TRUE */
	return(TRUE);
}

/* Launch search in external service or application of search provider */
static gboolean _xfdashboard_example_search_provider_launch_search(XfdashboardSearchProvider* inProvider,
																	const gchar **inSearchTerms)
{
	g_return_val_if_fail(XFDASHBOARD_IS_EXAMPLE_SEARCH_PROVIDER(inProvider), FALSE);
	g_return_val_if_fail(inSearchTerms, FALSE);

	/* Launch selected result item */
	/* TODO: Here you have to launch the application or other external services
	 *       when the provider icon of this search provider was clicked.
	 */

	/* If we get here launching search was successful, so return TRUE */
	return(TRUE);
}

/* IMPLEMENTATION: GObject */

/* Dispose this object */
static void _xfdashboard_example_search_provider_dispose(GObject *inObject)
{
	/* Release allocated resources */
	/* TODO: Release data in private instance if any */

	/* Call parent's class dispose method */
	G_OBJECT_CLASS(xfdashboard_example_search_provider_parent_class)->dispose(inObject);
}

/* Class initialization
 * Override functions in parent classes and define properties
 * and signals
 */
void xfdashboard_example_search_provider_class_init(XfdashboardExampleSearchProviderClass *klass)
{
	XfdashboardSearchProviderClass	*providerClass=XFDASHBOARD_SEARCH_PROVIDER_CLASS(klass);
	GObjectClass					*gobjectClass=G_OBJECT_CLASS(klass);

	/* Override functions */
	gobjectClass->dispose=_xfdashboard_example_search_provider_dispose;

	providerClass->initialize=_xfdashboard_example_search_provider_initialize;
	providerClass->get_icon=_xfdashboard_example_search_provider_get_icon;
	providerClass->get_name=_xfdashboard_example_search_provider_get_name;
	providerClass->get_result_set=_xfdashboard_example_search_provider_get_result_set;
	providerClass->create_result_actor=_xfdashboard_example_search_provider_create_result_actor;
	providerClass->activate_result=_xfdashboard_example_search_provider_activate_result;
	providerClass->launch_search=_xfdashboard_example_search_provider_launch_search;
}

/* Class finalization */
void xfdashboard_example_search_provider_class_finalize(XfdashboardExampleSearchProviderClass *klass)
{
}

/* Object initialization
 * Create private structure and set up default values
 */
void xfdashboard_example_search_provider_init(XfdashboardExampleSearchProvider *self)
{
	self->priv=xfdashboard_example_search_provider_get_instance_private(self);

	/* Set up default values */
	/* TODO: Set up default value in private instance data if any */
}
