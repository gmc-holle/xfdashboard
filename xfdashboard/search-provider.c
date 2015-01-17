/*
 * search-provider: Abstract class for search providers
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

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "search-provider.h"

#include <glib/gi18n-lib.h>

#include "actor.h"
#include "text-box.h"

/* Define this class in GObject system */
G_DEFINE_ABSTRACT_TYPE(XfdashboardSearchProvider,
						xfdashboard_search_provider,
						G_TYPE_OBJECT)

/* IMPLEMENTATION: Private variables and methods */
#define XFDASHBOARD_SEARCH_PROVIDER_WARN_NOT_IMPLEMENTED(self, vfunc) \
	g_warning(_("Search provider of type %s does not implement required virtual function XfdashboardSearchProvider::%s"), \
				G_OBJECT_TYPE_NAME(self), \
				vfunc);

#define XFDASHBOARD_SEARCH_PROVIDER_NOTE_NOT_IMPLEMENTED(self, vfunc) \
	g_debug("Search provider of type %s does not implement virtual function XfdashboardSearchProvider::%s", \
				G_OBJECT_TYPE_NAME(self), \
				vfunc);

/* IMPLEMENTATION: GObject */

/* Class initialization
 * Override functions in parent classes and define properties
 * and signals
 */
static void xfdashboard_search_provider_class_init(XfdashboardSearchProviderClass *klass)
{
}

/* Object initialization
 * Create private structure and set up default values
 */
static void xfdashboard_search_provider_init(XfdashboardSearchProvider *self)
{
}

/* IMPLEMENTATION: Public API */

/* Get name of search provider */
const gchar* xfdashboard_search_provider_get_name(XfdashboardSearchProvider *self)
{
	XfdashboardSearchProviderClass	*klass;

	g_return_val_if_fail(XFDASHBOARD_IS_SEARCH_PROVIDER(self), NULL);

	klass=XFDASHBOARD_SEARCH_PROVIDER_GET_CLASS(self);

	/* Return name of search provider */
	if(klass->get_name)
	{
		return(klass->get_name(self));
	}

	/* If we get here the virtual function was not overridden */
	XFDASHBOARD_SEARCH_PROVIDER_WARN_NOT_IMPLEMENTED(self, "get_name");
	return(G_OBJECT_TYPE_NAME(self));
}

/* Get icon name of search provider */
const gchar* xfdashboard_search_provider_get_icon(XfdashboardSearchProvider *self)
{
	XfdashboardSearchProviderClass	*klass;

	g_return_val_if_fail(XFDASHBOARD_IS_SEARCH_PROVIDER(self), NULL);

	klass=XFDASHBOARD_SEARCH_PROVIDER_GET_CLASS(self);

	/* Return icon of search provider */
	if(klass->get_icon)
	{
		return(klass->get_icon(self));
	}

	/* If we get here the virtual function was not overridden */
	XFDASHBOARD_SEARCH_PROVIDER_NOTE_NOT_IMPLEMENTED(self, "get_icon");
	return(NULL);
}

/* Get result set for list of search terms from search provider. If a previous result set
 * is provided do an incremental search on basis of provided result set. The returned
 * result set must be a new allocated object and its entries must already be sorted
 * in order in which they should be displayed.
 */
XfdashboardSearchResultSet* xfdashboard_search_provider_get_result_set(XfdashboardSearchProvider *self,
																		const gchar **inSearchTerms,
																		XfdashboardSearchResultSet *inPreviousResultSet)
{
	XfdashboardSearchProviderClass	*klass;

	g_return_val_if_fail(XFDASHBOARD_IS_SEARCH_PROVIDER(self), NULL);
	g_return_val_if_fail(inSearchTerms, NULL);
	g_return_val_if_fail(!inPreviousResultSet || XFDASHBOARD_IS_SEARCH_RESULT_SET(inPreviousResultSet), NULL);

	klass=XFDASHBOARD_SEARCH_PROVIDER_GET_CLASS(self);

	/* Return result set of search provider */
	if(klass->get_result_set)
	{
		return(klass->get_result_set(self, inSearchTerms, inPreviousResultSet));
	}

	/* If we get here the virtual function was not overridden */
	XFDASHBOARD_SEARCH_PROVIDER_WARN_NOT_IMPLEMENTED(self, "get_result_set");
	return(NULL);
}

/* Returns an actor for requested result item */
ClutterActor* xfdashboard_search_provider_create_result_actor(XfdashboardSearchProvider *self,
																GVariant *inResultItem)
{
	XfdashboardSearchProviderClass	*klass;

	g_return_val_if_fail(XFDASHBOARD_IS_SEARCH_PROVIDER(self), NULL);
	g_return_val_if_fail(inResultItem, NULL);

	klass=XFDASHBOARD_SEARCH_PROVIDER_GET_CLASS(self);

	/* Return actor created by search provider */
	if(klass->create_result_actor)
	{
		return(klass->create_result_actor(self, inResultItem));
	}

	/* If we get here the virtual function was not overridden */
	XFDASHBOARD_SEARCH_PROVIDER_WARN_NOT_IMPLEMENTED(self, "create_result_actor");
	return(NULL);
}

/* Launch search in external service or application the search provider relies on
 * with provided list of search terms.
 */
void xfdashboard_search_provider_launch_search(XfdashboardSearchProvider *self,
												const gchar **inSearchTerms)
{
	XfdashboardSearchProviderClass	*klass;

	g_return_if_fail(XFDASHBOARD_IS_SEARCH_PROVIDER(self));
	g_return_if_fail(inSearchTerms);

	klass=XFDASHBOARD_SEARCH_PROVIDER_GET_CLASS(self);

	/* Launch search by search provider */
	if(klass->launch_search)
	{
		klass->launch_search(self, inSearchTerms);
		return;
	}

	/* If we get here the virtual function was not overridden */
	XFDASHBOARD_SEARCH_PROVIDER_NOTE_NOT_IMPLEMENTED(self, "launch_search");
}

/* A result item actor was clicked so ask search provider to handle it */
void xfdashboard_search_provider_activate_result(XfdashboardSearchProvider* self,
													GVariant *inResultItem,
													ClutterActor *inActor,
													const gchar **inSearchTerms)
{
	XfdashboardSearchProviderClass	*klass;

	g_return_if_fail(XFDASHBOARD_IS_SEARCH_PROVIDER(self));
	g_return_if_fail(inResultItem);
	g_return_if_fail(CLUTTER_IS_ACTOR(inActor));
	g_return_if_fail(inSearchTerms);

	klass=XFDASHBOARD_SEARCH_PROVIDER_GET_CLASS(self);

	/* Handle click action at result item actor by search provider */
	if(klass->activate_result)
	{
		klass->activate_result(self, inResultItem, inActor, inSearchTerms);
		return;
	}

	/* If we get here the virtual function was not overridden */
	XFDASHBOARD_SEARCH_PROVIDER_NOTE_NOT_IMPLEMENTED(self, "activate_result");
}
