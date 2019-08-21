/*
 * search-manager: Single-instance managing search providers and
 *                 handles search requests
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

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <libxfdashboard/search-manager.h>

#include <glib/gi18n-lib.h>

#include <libxfdashboard/search-provider.h>
#include <libxfdashboard/utils.h>
#include <libxfdashboard/marshal.h>
#include <libxfdashboard/compat.h>
#include <libxfdashboard/debug.h>


/* Define this class in GObject system */
struct _XfdashboardSearchManagerPrivate
{
	/* Instance related */
	GList		*registeredProviders;
};

G_DEFINE_TYPE_WITH_PRIVATE(XfdashboardSearchManager,
							xfdashboard_search_manager,
							G_TYPE_OBJECT)

/* Signals */
enum
{
	SIGNAL_REGISTERED,
	SIGNAL_UNREGISTERED,

	SIGNAL_LAST
};

static guint XfdashboardSearchManagerSignals[SIGNAL_LAST]={ 0, };

/* IMPLEMENTATION: Private variables and methods */
typedef struct _XfdashboardSearchManagerData		XfdashboardSearchManagerData;
struct _XfdashboardSearchManagerData
{
	gchar		*ID;
	GType		gtype;
};


/* Single instance of search manager */
static XfdashboardSearchManager*			_xfdashboard_search_manager=NULL;

#define DEFAULT_SEARCH_TERMS_DELIMITERS		"\t\n\r "

/* Free an registered view entry */
static void _xfdashboard_search_manager_entry_free(XfdashboardSearchManagerData *inData)
{
	g_return_if_fail(inData);

	/* Release allocated resources */
	if(inData->ID) g_free(inData->ID);
	g_free(inData);
}

/* Create an entry for a registered view */
static XfdashboardSearchManagerData* _xfdashboard_search_manager_entry_new(const gchar *inID, GType inType)
{
	XfdashboardSearchManagerData		*data;

	g_return_val_if_fail(inID && *inID, NULL);

	/* Create new entry */
	data=g_new0(XfdashboardSearchManagerData, 1);
	if(!data) return(NULL);

	data->ID=g_strdup(inID);
	data->gtype=inType;

	/* Return newly created entry */
	return(data);
}

/* Find entry for a registered view by ID */
static GList* _xfdashboard_search_manager_entry_find_list_entry_by_id(XfdashboardSearchManager *self,
																		const gchar *inID)
{
	XfdashboardSearchManagerPrivate		*priv;
	GList								*iter;
	XfdashboardSearchManagerData		*data;

	g_return_val_if_fail(XFDASHBOARD_IS_SEARCH_MANAGER(self), NULL);
	g_return_val_if_fail(inID && *inID, NULL);

	priv=self->priv;

	/* Iterate through list and lookup list entry whose data has requested ID */
	for(iter=priv->registeredProviders; iter; iter=g_list_next(iter))
	{
		/* Get data of currently iterated list entry */
		data=(XfdashboardSearchManagerData*)(iter->data);
		if(!data) continue;

		/* Check if ID of data matches requested one and
		 * return list entry if it does.
		 */
		if(g_strcmp0(data->ID, inID)==0) return(iter);
	}

	/* If we get here we did not find a matching list entry */
	return(NULL);
}

static XfdashboardSearchManagerData* _xfdashboard_search_manager_entry_find_data_by_id(XfdashboardSearchManager *self,
																						const gchar *inID)
{
	GList								*iter;
	XfdashboardSearchManagerData		*data;

	g_return_val_if_fail(XFDASHBOARD_IS_SEARCH_MANAGER(self), NULL);
	g_return_val_if_fail(inID && *inID, NULL);

	/* Find list entry matching requested ID */
	iter=_xfdashboard_search_manager_entry_find_list_entry_by_id(self, inID);
	if(!iter) return(NULL);

	/* We found a matching list entry so return its data */
	data=(XfdashboardSearchManagerData*)(iter->data);

	/* Return data of matching list entry */
	return(data);
}

/* IMPLEMENTATION: GObject */

/* Construct this object */
static GObject* _xfdashboard_search_manager_constructor(GType inType,
														guint inNumberConstructParams,
														GObjectConstructParam *inConstructParams)
{
	GObject									*object;

	if(!_xfdashboard_search_manager)
	{
		object=G_OBJECT_CLASS(xfdashboard_search_manager_parent_class)->constructor(inType, inNumberConstructParams, inConstructParams);
		_xfdashboard_search_manager=XFDASHBOARD_SEARCH_MANAGER(object);
	}
		else
		{
			object=g_object_ref(G_OBJECT(_xfdashboard_search_manager));
		}

	return(object);
}

/* Dispose this object */
static void _xfdashboard_search_manager_dispose_unregister_search_provider(gpointer inData, gpointer inUserData)
{
	XfdashboardSearchManagerData		*data;

	g_return_if_fail(XFDASHBOARD_IS_SEARCH_MANAGER(inUserData));

	data=(XfdashboardSearchManagerData*)inData;
	xfdashboard_search_manager_unregister(XFDASHBOARD_SEARCH_MANAGER(inUserData), data->ID);
}

static void _xfdashboard_search_manager_dispose(GObject *inObject)
{
	XfdashboardSearchManager			*self=XFDASHBOARD_SEARCH_MANAGER(inObject);
	XfdashboardSearchManagerPrivate		*priv=self->priv;

	/* Release allocated resouces */
	if(priv->registeredProviders)
	{
		g_list_foreach(priv->registeredProviders, _xfdashboard_search_manager_dispose_unregister_search_provider, self);
		g_list_free(priv->registeredProviders);
		priv->registeredProviders=NULL;
	}

	/* Call parent's class dispose method */
	G_OBJECT_CLASS(xfdashboard_search_manager_parent_class)->dispose(inObject);
}

/* Finalize this object */
static void _xfdashboard_search_manager_finalize(GObject *inObject)
{
	/* Release allocated resources finally, e.g. unset singleton */
	if(G_LIKELY(G_OBJECT(_xfdashboard_search_manager)==inObject))
	{
		_xfdashboard_search_manager=NULL;
	}

	/* Call parent's class dispose method */
	G_OBJECT_CLASS(xfdashboard_search_manager_parent_class)->finalize(inObject);
}

/* Class initialization
 * Override functions in parent classes and define properties
 * and signals
 */
static void xfdashboard_search_manager_class_init(XfdashboardSearchManagerClass *klass)
{
	GObjectClass		*gobjectClass=G_OBJECT_CLASS(klass);

	/* Override functions */
	gobjectClass->constructor=_xfdashboard_search_manager_constructor;
	gobjectClass->dispose=_xfdashboard_search_manager_dispose;
	gobjectClass->finalize=_xfdashboard_search_manager_finalize;

	/* Define signals */
	XfdashboardSearchManagerSignals[SIGNAL_REGISTERED]=
		g_signal_new("registered",
						G_TYPE_FROM_CLASS(klass),
						G_SIGNAL_RUN_LAST,
						G_STRUCT_OFFSET(XfdashboardSearchManagerClass, registered),
						NULL,
						NULL,
						g_cclosure_marshal_VOID__OBJECT,
						G_TYPE_NONE,
						1,
						G_TYPE_GTYPE);

	XfdashboardSearchManagerSignals[SIGNAL_UNREGISTERED]=
		g_signal_new("unregistered",
						G_TYPE_FROM_CLASS(klass),
						G_SIGNAL_RUN_LAST,
						G_STRUCT_OFFSET(XfdashboardSearchManagerClass, unregistered),
						NULL,
						NULL,
						g_cclosure_marshal_VOID__OBJECT,
						G_TYPE_NONE,
						1,
						G_TYPE_GTYPE);
}

/* Object initialization
 * Create private structure and set up default values
 */
static void xfdashboard_search_manager_init(XfdashboardSearchManager *self)
{
	XfdashboardSearchManagerPrivate		*priv;

	priv=self->priv=xfdashboard_search_manager_get_instance_private(self);

	/* Set default values */
	priv->registeredProviders=NULL;
}

/* IMPLEMENTATION: Public API */

/* Get single instance of manager */
XfdashboardSearchManager* xfdashboard_search_manager_get_default(void)
{
	GObject									*singleton;

	singleton=g_object_new(XFDASHBOARD_TYPE_SEARCH_MANAGER, NULL);
	return(XFDASHBOARD_SEARCH_MANAGER(singleton));
}

/* Register a search provider */
gboolean xfdashboard_search_manager_register(XfdashboardSearchManager *self, const gchar *inID, GType inProviderType)
{
	XfdashboardSearchManagerPrivate		*priv;
	XfdashboardSearchManagerData		*data;

	g_return_val_if_fail(XFDASHBOARD_IS_SEARCH_MANAGER(self), FALSE);
	g_return_val_if_fail(inID && *inID, FALSE);

	priv=self->priv;

	/* Check if given type is not a XfdashboardSearchProvider but a derived type from it */
	if(inProviderType==XFDASHBOARD_TYPE_SEARCH_PROVIDER ||
		g_type_is_a(inProviderType, XFDASHBOARD_TYPE_SEARCH_PROVIDER)!=TRUE)
	{
		g_warning(_("Search provider %s of type %s is not a %s and cannot be registered"),
					inID,
					g_type_name(inProviderType),
					g_type_name(XFDASHBOARD_TYPE_SEARCH_PROVIDER));
		return(FALSE);
	}

	/* Check if search provider is registered already */
	if(_xfdashboard_search_manager_entry_find_list_entry_by_id(self, inID))
	{
		g_warning(_("Search provider %s of type %s is registered already"),
					inID,
					g_type_name(inProviderType));
		return(FALSE);
	}

	/* Register search provider */
	XFDASHBOARD_DEBUG(self, MISC,
						"Registering search provider %s of type %s",
						inID,
						g_type_name(inProviderType));

	data=_xfdashboard_search_manager_entry_new(inID, inProviderType);
	if(!data)
	{
		g_warning(_("Failed to register search provider %s of type %s"),
					inID,
					g_type_name(inProviderType));
		return(FALSE);
	}

	priv->registeredProviders=g_list_append(priv->registeredProviders, data);
	g_signal_emit(self, XfdashboardSearchManagerSignals[SIGNAL_REGISTERED], 0, data->ID);

	/* Search provider was registered successfully so return TRUE here */
	return(TRUE);
}

/* Unregister a search provider */
gboolean xfdashboard_search_manager_unregister(XfdashboardSearchManager *self, const gchar *inID)
{
	XfdashboardSearchManagerPrivate		*priv;
	GList								*iter;
	XfdashboardSearchManagerData		*data;

	g_return_val_if_fail(XFDASHBOARD_IS_SEARCH_MANAGER(self), FALSE);
	g_return_val_if_fail(inID && *inID, FALSE);

	priv=self->priv;

	/* Check if search provider is registered  */
	iter=_xfdashboard_search_manager_entry_find_list_entry_by_id(self, inID);
	if(!iter)
	{
		g_warning(_("Search provider %s is not registered and cannot be unregistered"), inID);
		return(FALSE);
	}

	/* Get data from found list entry */
	data=(XfdashboardSearchManagerData*)(iter->data);

	/* Remove from list of registered providers */
	XFDASHBOARD_DEBUG(self, MISC,
						"Unregistering search provider %s of type %s",
						data->ID,
						g_type_name(data->gtype));

	priv->registeredProviders=g_list_remove_link(priv->registeredProviders, iter);
	g_signal_emit(self, XfdashboardSearchManagerSignals[SIGNAL_UNREGISTERED], 0, data->ID);

	/* Free data entry and list element at iterator */
	_xfdashboard_search_manager_entry_free(data);
	g_list_free(iter);

	/* Search provider was unregistered successfully so return TRUE here */
	return(TRUE);
}

/* Get list of registered views types.
 * Returned GList must be freed with g_list_free_full(result, g_free) by caller.
 */
GList* xfdashboard_search_manager_get_registered(XfdashboardSearchManager *self)
{
	GList							*copy;
	GList							*iter;
	XfdashboardSearchManagerData	*data;

	g_return_val_if_fail(XFDASHBOARD_IS_SEARCH_MANAGER(self), NULL);

	/* Return a copy of all IDs stored in list of registered search provider types */
	copy=NULL;
	for(iter=self->priv->registeredProviders; iter; iter=g_list_next(iter))
	{
		data=(XfdashboardSearchManagerData*)(iter->data);

		copy=g_list_prepend(copy, g_strdup(data->ID));
	}

	/* Restore order in copied list to match origin */
	copy=g_list_reverse(copy);

	/* Return copied list of IDs of registered search providers */
	return(copy);
}

/* Check if a search provider for requested ID is registered */
gboolean xfdashboard_search_manager_has_registered_id(XfdashboardSearchManager *self, const gchar *inID)
{
	GList							*iter;

	g_return_val_if_fail(XFDASHBOARD_IS_SEARCH_MANAGER(self), FALSE);
	g_return_val_if_fail(inID && *inID, FALSE);

	/* Check if search provider is registered by getting pointer to list element
	 * in list of registered providers.
	 */
	iter=_xfdashboard_search_manager_entry_find_list_entry_by_id(self, inID);
	if(iter) return(TRUE);

	/* If we get here we did not find a search provider for requested ID */
	return(FALSE);
}

/* Create search provider for requested ID */
GObject* xfdashboard_search_manager_create_provider(XfdashboardSearchManager *self, const gchar *inID)
{
	XfdashboardSearchManagerData	*data;
	GObject							*provider;

	g_return_val_if_fail(XFDASHBOARD_IS_SEARCH_MANAGER(self), NULL);
	g_return_val_if_fail(inID && *inID, NULL);

	/* Check if search provider is registered and get its data */
	data=_xfdashboard_search_manager_entry_find_data_by_id(self, inID);
	if(!data)
	{
		g_warning(_("Cannot create search provider %s because it is not registered"), inID);
		return(NULL);
	}

	/* Create search provider */
	provider=g_object_new(data->gtype, "provider-id", data->ID, NULL);
	if(provider &&
		XFDASHBOARD_SEARCH_PROVIDER_GET_CLASS(provider)->initialize)
	{
		XFDASHBOARD_SEARCH_PROVIDER_GET_CLASS(provider)->initialize(XFDASHBOARD_SEARCH_PROVIDER(provider));
	}

	/* Return newly created search provider */
	return(provider);
}

/* Split a string into a NULL-terminated list of tokens using the delimiters and remove
 * white-spaces at the beginning and end of each token. Empty tokens will not be added.
 * Caller is responsible to free result with g_strfreev() if not NULL.
 */
gchar** xfdashboard_search_manager_get_search_terms_from_string(const gchar *inString, const gchar *inDelimiters)
{
	const gchar			*delimiters;

	g_return_val_if_fail(inString, NULL);

	/* If no delimiters are specified use default ones */
	if(inDelimiters && *inDelimiters) delimiters=inDelimiters;
		else delimiters=DEFAULT_SEARCH_TERMS_DELIMITERS;

	/* Split string */
	return(xfdashboard_split_string(inString, delimiters));
}
