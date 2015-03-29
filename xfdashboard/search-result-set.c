/*
 * search-result-set: Contains and manages set of identifiers of a search
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

#include "search-result-set.h"

#include <glib/gi18n-lib.h>

/* Define this class in GObject system */
G_DEFINE_TYPE(XfdashboardSearchResultSet,
				xfdashboard_search_result_set,
				G_TYPE_OBJECT)

/* Private structure - access only by public API if needed */
#define XFDASHBOARD_SEARCH_RESULT_SET_GET_PRIVATE(obj) \
	(G_TYPE_INSTANCE_GET_PRIVATE((obj), XFDASHBOARD_TYPE_SEARCH_RESULT_SET, XfdashboardSearchResultSetPrivate))

struct _XfdashboardSearchResultSetPrivate
{
	/* Instance related */
	GHashTable								*set;

	XfdashboardSearchResultSetCompareFunc	sortCallback;
	gpointer								sortUserData;
	GDestroyNotify							sortUserDataDestroyFunc;
};

/* IMPLEMENTATION: Private variables and methods */

/* Internal callback function for calling callback functions for sorting */
static gint _xfdashboard_search_result_set_sort_internal(gconstpointer inLeft,
															gconstpointer inRight,
															gpointer inUserData)
{
	XfdashboardSearchResultSet				*self=XFDASHBOARD_SEARCH_RESULT_SET(inUserData);
	XfdashboardSearchResultSetPrivate		*priv=self->priv;
	GVariant								*left;
	GVariant								*right;

	/* Get items to compare */
	left=(GVariant*)inLeft;
	right=(GVariant*)inRight;

	/* Call sorting callback function now */
	return((priv->sortCallback)(left, right, priv->sortUserData));
}

/* IMPLEMENTATION: GObject */

/* Dispose this object */
static void _xfdashboard_search_result_set_dispose(GObject *inObject)
{
	XfdashboardSearchResultSet			*self=XFDASHBOARD_SEARCH_RESULT_SET(inObject);
	XfdashboardSearchResultSetPrivate	*priv=self->priv;

	/* Release allocated resouces */
	if(priv->sortUserData)
	{
		/* Release old sort user data with destroy function if set previously
		 * and reset destroy function.
		 */
		if(priv->sortUserDataDestroyFunc)
		{
			priv->sortUserDataDestroyFunc(priv->sortUserData);
			priv->sortUserDataDestroyFunc=NULL;
		}

		/* Reset sort user data */
		priv->sortUserData=NULL;
	}

	priv->sortCallback=NULL;

	if(priv->set)
	{
		g_hash_table_unref(priv->set);
		priv->set=NULL;
	}

	/* Call parent's class dispose method */
	G_OBJECT_CLASS(xfdashboard_search_result_set_parent_class)->dispose(inObject);
}

/* Class initialization
 * Override functions in parent classes and define properties
 * and signals
 */
static void xfdashboard_search_result_set_class_init(XfdashboardSearchResultSetClass *klass)
{
	GObjectClass		*gobjectClass=G_OBJECT_CLASS(klass);

	/* Override functions */
	gobjectClass->dispose=_xfdashboard_search_result_set_dispose;

	/* Set up private structure */
	g_type_class_add_private(klass, sizeof(XfdashboardSearchResultSetPrivate));
}

/* Object initialization
 * Create private structure and set up default values
 */
static void xfdashboard_search_result_set_init(XfdashboardSearchResultSet *self)
{
	XfdashboardSearchResultSetPrivate	*priv;

	priv=self->priv=XFDASHBOARD_SEARCH_RESULT_SET_GET_PRIVATE(self);

	/* Set default values */
	priv->set=g_hash_table_new_full(g_variant_hash,
									g_variant_equal,
									(GDestroyNotify)g_variant_unref,
									NULL);
}

/* IMPLEMENTATION: Public API */

/* Create new instance */
XfdashboardSearchResultSet* xfdashboard_search_result_set_new(void)
{
	return((XfdashboardSearchResultSet*)g_object_new(XFDASHBOARD_TYPE_SEARCH_RESULT_SET, NULL));
}

/* Get size of result set */
guint xfdashboard_search_result_set_get_size(XfdashboardSearchResultSet *self)
{
	g_return_val_if_fail(XFDASHBOARD_IS_SEARCH_RESULT_SET(self), 0);

	return(g_hash_table_size(self->priv->set));
}

/* Add a result item to result set */
void xfdashboard_search_result_set_add_item(XfdashboardSearchResultSet *self, GVariant *inItem)
{
	XfdashboardSearchResultSetPrivate		*priv;

	g_return_if_fail(XFDASHBOARD_IS_SEARCH_RESULT_SET(self));
	g_return_if_fail(inItem);

	priv=self->priv;

	/* Add or replace item in hash table */
	g_hash_table_insert(priv->set, g_variant_ref_sink(inItem), GINT_TO_POINTER(1));
}

/* Check if a result item exists already in result set */
gboolean xfdashboard_search_result_set_has_item(XfdashboardSearchResultSet *self, GVariant *inItem)
{
	XfdashboardSearchResultSetPrivate		*priv;

	g_return_val_if_fail(XFDASHBOARD_IS_SEARCH_RESULT_SET(self), FALSE);
	g_return_val_if_fail(inItem, FALSE);

	priv=self->priv;

	/* Return result indicating existence of item in this result set */
	return(g_hash_table_lookup_extended(priv->set, inItem, NULL, NULL));
}

/* Get list of all items in this result sets.
 * Returned list should be freed with g_list_free_full(result, g_variant_unref)
 */
GList* xfdashboard_search_result_set_get_all(XfdashboardSearchResultSet *self)
{
	XfdashboardSearchResultSetPrivate		*priv;
	GHashTableIter							iter;
	GList									*list;
	GVariant								*key;

	g_return_val_if_fail(XFDASHBOARD_IS_SEARCH_RESULT_SET(self), NULL);

	priv=self->priv;

	/* Iterate through hash table of this result set, take a reference of
	 * key of this result set's hash table item and add list to result list.
	 */
	list=NULL;
	g_hash_table_iter_init(&iter, priv->set);
	while(g_hash_table_iter_next(&iter, (gpointer*)&key, NULL))
	{
		list=g_list_prepend(list, g_variant_ref(key));
	}

	/* If a sorting function was set then sort result list */
	if(list && priv->sortCallback)
	{
		list=g_list_sort_with_data(list, _xfdashboard_search_result_set_sort_internal, self);
	}

	/* Return result */
	return(list);
}
/* Get list of all items existing in both result sets.
 * Returned list should be freed with g_list_free_full(result, g_variant_unref)
 */
GList* xfdashboard_search_result_set_intersect(XfdashboardSearchResultSet *self, XfdashboardSearchResultSet *inOtherSet)
{
	XfdashboardSearchResultSetPrivate		*priv;
	GHashTableIter							iter;
	GList									*list;
	GVariant								*key;

	g_return_val_if_fail(XFDASHBOARD_IS_SEARCH_RESULT_SET(self), NULL);
	g_return_val_if_fail(XFDASHBOARD_IS_SEARCH_RESULT_SET(inOtherSet), NULL);

	priv=self->priv;

	/* Iterate through hash table of this result set and lookup its keys
	 * at other result set's hash table. If it exists take a reference of
	 * key of this result set's hash table item and add list to result list.
	 */
	list=NULL;
	g_hash_table_iter_init(&iter, priv->set);
	while(g_hash_table_iter_next(&iter, (gpointer*)&key, NULL))
	{
		/* Lookup key in other result set's hash table */
		if(g_hash_table_lookup_extended(inOtherSet->priv->set, key, NULL, NULL))
		{
			list=g_list_prepend(list, g_variant_ref(key));
		}
	}

	/* If a sorting function was set then sort result list */
	if(list && priv->sortCallback)
	{
		list=g_list_sort_with_data(list, _xfdashboard_search_result_set_sort_internal, self);
	}

	/* Return result */
	return(list);
}

/* Get list of all items existing in other result set but not in this result set.
 * Returned list should be freed with g_list_free_full(result, g_variant_unref)
 */
GList* xfdashboard_search_result_set_complement(XfdashboardSearchResultSet *self, XfdashboardSearchResultSet *inOtherSet)
{
	XfdashboardSearchResultSetPrivate		*priv;
	GHashTableIter							iter;
	GList									*list;
	GVariant								*key;

	g_return_val_if_fail(XFDASHBOARD_IS_SEARCH_RESULT_SET(self), NULL);
	g_return_val_if_fail(XFDASHBOARD_IS_SEARCH_RESULT_SET(inOtherSet), NULL);

	priv=self->priv;

	/* Iterate through hash table of other result set and lookup its keys
	 * at this result set's hash table. If it does not exists take a reference
	 * of key of this result set's hash table item and add list to result list.
	 */
	list=NULL;
	g_hash_table_iter_init(&iter, inOtherSet->priv->set);
	while(g_hash_table_iter_next(&iter, (gpointer*)&key, NULL))
	{
		/* Lookup key in other result set's hash table and in case it does not
		 * exist then add a reference of the key to result.
		 */
		if(!g_hash_table_lookup_extended(priv->set, key, NULL, NULL))
		{
			list=g_list_prepend(list, g_variant_ref(key));
		}
	}

	/* If a sorting function was set then sort result list */
	if(list && priv->sortCallback)
	{
		list=g_list_sort_with_data(list, _xfdashboard_search_result_set_sort_internal, self);
	}

	/* Return result */
	return(list);
}

/* Sets a callback function for sorting all items in result set */
void xfdashboard_search_result_set_set_sort_func(XfdashboardSearchResultSet *self,
													XfdashboardSearchResultSetCompareFunc inCallbackFunc,
													gpointer inUserData)
{
	xfdashboard_search_result_set_set_sort_func_full(self, inCallbackFunc, inUserData, NULL);
}

void xfdashboard_search_result_set_set_sort_func_full(XfdashboardSearchResultSet *self,
														XfdashboardSearchResultSetCompareFunc inCallbackFunc,
														gpointer inUserData,
														GDestroyNotify inUserDataDestroyFunc)
{
	XfdashboardSearchResultSetPrivate		*priv;

	g_return_if_fail(XFDASHBOARD_IS_SEARCH_RESULT_SET(self));

	priv=self->priv;

	/* Release old sort callback data */
	if(priv->sortUserData)
	{
		/* Release old sort user data with destroy function if set previously
		 * and reset destroy function.
		 */
		if(priv->sortUserDataDestroyFunc)
		{
			priv->sortUserDataDestroyFunc(priv->sortUserData);
			priv->sortUserDataDestroyFunc=NULL;
		}

		/* Reset sort user data */
		priv->sortUserData=NULL;
	}
	priv->sortCallback=NULL;

	/* Set sort callback */
	if(inCallbackFunc)
	{
		priv->sortCallback=inCallbackFunc;
		priv->sortUserData=inUserData;
		priv->sortUserDataDestroyFunc=inUserDataDestroyFunc;
	}
}
