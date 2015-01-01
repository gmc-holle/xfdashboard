/*
 * search-manager: Single-instance managing search providers and
 *                 handles search requests
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

#include "search-manager.h"

#include <glib/gi18n-lib.h>

#include "search-provider.h"
#include "utils.h"
#include "marshal.h"

/* Define this class in GObject system */
G_DEFINE_TYPE(XfdashboardSearchManager,
				xfdashboard_search_manager,
				G_TYPE_OBJECT)

/* Private structure - access only by public API if needed */
#define XFDASHBOARD_SEARCH_MANAGER_GET_PRIVATE(obj) \
	(G_TYPE_INSTANCE_GET_PRIVATE((obj), XFDASHBOARD_TYPE_SEARCH_MANAGER, XfdashboardSearchManagerPrivate))

struct _XfdashboardSearchManagerPrivate
{
	/* Instance related */
	GList		*registeredProviders;
};

/* Signals */
enum
{
	SIGNAL_REGISTERED,
	SIGNAL_UNREGISTERED,

	SIGNAL_LAST
};

static guint XfdashboardSearchManagerSignals[SIGNAL_LAST]={ 0, };

/* IMPLEMENTATION: Private variables and methods */

/* Single instance of search manager */
static XfdashboardSearchManager*			_xfdashboard_search_manager=NULL;

#define DEFAULT_SEARCH_TERMS_DELIMITERS		"\t\n\r "

/* IMPLEMENTATION: GObject */

/* Dispose this object */
static void _xfdashboard_search_manager_dispose_unregister_search_provider(gpointer inData, gpointer inUserData)
{
	g_return_if_fail(XFDASHBOARD_IS_SEARCH_MANAGER(inUserData));

	xfdashboard_search_manager_unregister(XFDASHBOARD_SEARCH_MANAGER(inUserData), GPOINTER_TO_GTYPE(inData));
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

	/* Unset singleton */
	if(G_LIKELY(G_OBJECT(_xfdashboard_search_manager)==inObject)) _xfdashboard_search_manager=NULL;

	/* Call parent's class dispose method */
	G_OBJECT_CLASS(xfdashboard_search_manager_parent_class)->dispose(inObject);
}

/* Class initialization
 * Override functions in parent classes and define properties
 * and signals
 */
static void xfdashboard_search_manager_class_init(XfdashboardSearchManagerClass *klass)
{
	GObjectClass		*gobjectClass=G_OBJECT_CLASS(klass);

	/* Override functions */
	gobjectClass->dispose=_xfdashboard_search_manager_dispose;

	/* Set up private structure */
	g_type_class_add_private(klass, sizeof(XfdashboardSearchManagerPrivate));

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

	priv=self->priv=XFDASHBOARD_SEARCH_MANAGER_GET_PRIVATE(self);

	/* Set default values */
	priv->registeredProviders=NULL;
}

/* IMPLEMENTATION: Public API */

/* Get single instance of manager */
XfdashboardSearchManager* xfdashboard_search_manager_get_default(void)
{
	if(G_UNLIKELY(_xfdashboard_search_manager==NULL))
	{
		_xfdashboard_search_manager=g_object_new(XFDASHBOARD_TYPE_SEARCH_MANAGER, NULL);
	}
		else g_object_ref(_xfdashboard_search_manager);

	return(_xfdashboard_search_manager);
}

/* Register a search provider */
void xfdashboard_search_manager_register(XfdashboardSearchManager *self, GType inProviderType)
{
	XfdashboardSearchManagerPrivate		*priv;

	g_return_if_fail(XFDASHBOARD_IS_SEARCH_MANAGER(self));

	priv=self->priv;

	/* Check if given type is not a XfdashboardSearchProvider but a derived type from it */
	if(inProviderType==XFDASHBOARD_TYPE_SEARCH_PROVIDER ||
		g_type_is_a(inProviderType, XFDASHBOARD_TYPE_SEARCH_PROVIDER)!=TRUE)
	{
		g_warning(_("Search provider %s is not a %s and cannot be registered"),
					g_type_name(inProviderType),
					g_type_name(XFDASHBOARD_TYPE_SEARCH_PROVIDER));
		return;
	}

	/* Register type if not already registered */
	if(g_list_find(priv->registeredProviders, GTYPE_TO_POINTER(inProviderType))==NULL)
	{
		g_debug("Registering search provider %s", g_type_name(inProviderType));
		priv->registeredProviders=g_list_append(priv->registeredProviders, GTYPE_TO_POINTER(inProviderType));
		g_signal_emit(self, XfdashboardSearchManagerSignals[SIGNAL_REGISTERED], 0, inProviderType);
	}
}

/* Unregister a search provider */
void xfdashboard_search_manager_unregister(XfdashboardSearchManager *self, GType inProviderType)
{
	XfdashboardSearchManagerPrivate		*priv;

	g_return_if_fail(XFDASHBOARD_IS_SEARCH_MANAGER(self));

	priv=self->priv;

	/* Check if given type is not a XfdashboardView but a derived type from it */
	if(inProviderType==XFDASHBOARD_TYPE_SEARCH_PROVIDER ||
		g_type_is_a(inProviderType, XFDASHBOARD_TYPE_SEARCH_PROVIDER)!=TRUE)
	{
		g_warning(_("Search provider %s is not a %s and cannot be unregistered"),
					g_type_name(inProviderType),
					g_type_name(XFDASHBOARD_TYPE_SEARCH_PROVIDER));
		return;
	}

	/* Unregister type if registered */
	if(g_list_find(priv->registeredProviders, GTYPE_TO_POINTER(inProviderType))!=NULL)
	{
		g_debug("Unregistering search provider %s", g_type_name(inProviderType));
		priv->registeredProviders=g_list_remove(priv->registeredProviders, GTYPE_TO_POINTER(inProviderType));
		g_signal_emit(self, XfdashboardSearchManagerSignals[SIGNAL_UNREGISTERED], 0, inProviderType);
	}
}

/* Get list of registered search provider types.
 * Returned GList must be freed with g_list_free() by caller.
 */
GList* xfdashboard_search_manager_get_registered(XfdashboardSearchManager *self)
{
	GList		*copy;

	g_return_val_if_fail(XFDASHBOARD_IS_SEARCH_MANAGER(self), NULL);

	/* Return a copy of list of registered view types */
	copy=g_list_copy(self->priv->registeredProviders);
	return(copy);
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
