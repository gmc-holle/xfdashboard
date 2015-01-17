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
	GPtrArray			*identifiers;
};

/* IMPLEMENTATION: Private variables and methods */
typedef struct _XfdashboardSearchResultSetSortData	XfdashboardSearchResultSetSortData;
struct _XfdashboardSearchResultSetSortData
{
	XfdashboardSearchResultSetCompareFunc	callback;
	gpointer								userData;
};

/* Internal callback function for calling callback functions for sorting */
static gint _xfdashboard_search_result_set_sort_internal(gconstpointer inLeft,
															gconstpointer inRight,
															gpointer inUserData)
{
	GVariant								*left;
	GVariant								*right;
	XfdashboardSearchResultSetSortData		*sortData;

	left=*((GVariant**)inLeft);
	right=*((GVariant**)inRight);
	sortData=(XfdashboardSearchResultSetSortData*)inUserData;

	/* Call callback function now */
	return((sortData->callback)(left, right, sortData->userData));
}

/* IMPLEMENTATION: GObject */

/* Dispose this object */
static void _xfdashboard_search_result_set_dispose(GObject *inObject)
{
	XfdashboardSearchResultSet			*self=XFDASHBOARD_SEARCH_RESULT_SET(inObject);
	XfdashboardSearchResultSetPrivate	*priv=self->priv;

	/* Release allocated resouces */
	if(priv->identifiers)
	{
		g_ptr_array_free(priv->identifiers, TRUE);
		priv->identifiers=NULL;
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
	priv->identifiers=g_ptr_array_new_with_free_func((GDestroyNotify)g_variant_unref);
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

	return(self->priv->identifiers->len);
}

/* Add a result item to result set */
void xfdashboard_search_result_set_add_item(XfdashboardSearchResultSet *self, GVariant *inItem)
{
	XfdashboardSearchResultSetPrivate		*priv;
	guint									index;
	GVariant								*item;

	g_return_if_fail(XFDASHBOARD_IS_SEARCH_RESULT_SET(self));
	g_return_if_fail(inItem);

	priv=self->priv;

	/* Check for duplicates */
	for(index=0; index<priv->identifiers->len; index++)
	{
		item=(GVariant*)g_ptr_array_index(priv->identifiers, index);
		if(g_variant_compare(item, inItem)==0) return;
	}

	/* Add item to list */
	g_ptr_array_add(priv->identifiers, g_variant_ref_sink(inItem));
}

/* Get item from result set */
GVariant* xfdashboard_search_result_set_get_item(XfdashboardSearchResultSet *self, gint inIndex)
{
	g_return_val_if_fail(XFDASHBOARD_IS_SEARCH_RESULT_SET(self), NULL);
	g_return_val_if_fail(inIndex>=0 && (guint)inIndex<self->priv->identifiers->len, NULL);

	return((GVariant*)g_ptr_array_index(self->priv->identifiers, inIndex));
}

/* Find index of an item matching requested one by comparison for equality.
 * Returns -1 if not found.
 */
gint xfdashboard_search_result_set_get_index(XfdashboardSearchResultSet *self, GVariant *inItem)
{
	XfdashboardSearchResultSetPrivate		*priv;
	guint									index;
	GVariant								*item;

	g_return_val_if_fail(XFDASHBOARD_IS_SEARCH_RESULT_SET(self), -1);

	priv=self->priv;

	/* Iterate through list of identifiers and check for equality */
	for(index=0; index<priv->identifiers->len; index++)
	{
		item=(GVariant*)g_ptr_array_index(priv->identifiers, index);
		if(g_variant_equal(item, inItem)) return(index);
	}

	/* If we get here we did not find the requested item in result set */
	return(-1);
}

/* Calls a callback function for each item in result set */
void xfdashboard_search_result_set_foreach(XfdashboardSearchResultSet *self,
											GFunc inCallbackFunc,
											gpointer inUserData)
{
	XfdashboardSearchResultSetPrivate		*priv;

	g_return_if_fail(XFDASHBOARD_IS_SEARCH_RESULT_SET(self));
	g_return_if_fail(inCallbackFunc);

	priv=self->priv;

	/* Call callback for each item in result set */
	g_ptr_array_foreach(priv->identifiers, inCallbackFunc, inUserData);
}

/* Calls a callback function for sorting all items in result set */
void xfdashboard_search_result_set_sort(XfdashboardSearchResultSet *self,
											XfdashboardSearchResultSetCompareFunc inCallbackFunc,
											gpointer inUserData)
{
	XfdashboardSearchResultSetPrivate		*priv;
	XfdashboardSearchResultSetSortData		sortData;

	g_return_if_fail(XFDASHBOARD_IS_SEARCH_RESULT_SET(self));
	g_return_if_fail(inCallbackFunc);

	priv=self->priv;

	/* Call callback for each item in result set */
	sortData.callback=inCallbackFunc;
	sortData.userData=inUserData;
	g_ptr_array_sort_with_data(priv->identifiers, _xfdashboard_search_result_set_sort_internal, &sortData);
}
