/*
 * search-manager: Single-instance managing search plugins
 * 
 * Copyright 2012-2013 Stephan Haller <nomad@froevel.de>
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
#include "utils.h"

#include <glib/gi18n-lib.h>

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
	GList		*registeredPlugins;
};

/* Signals */
enum
{
	SIGNAL_REGISTERED,
	SIGNAL_UNREGISTERED,

	SIGNAL_LAST
};

guint XfdashboardSearchManagerSignals[SIGNAL_LAST]={ 0, };

/* IMPLEMENTATION: Private variables and methods */

/* Single instance of view manager */
XfdashboardSearchManager*		searchManager=NULL;

/* IMPLEMENTATION: GObject */

/* Dispose this object */
void _xfdashboard_search_manager_dispose_unregister_plugin(gpointer inData, gpointer inUserData)
{
	g_return_if_fail(XFDASHBOARD_IS_SEARCH_MANAGER(inUserData));

	xfdashboard_search_manager_unregister(XFDASHBOARD_SEARCH_MANAGER(inUserData), LISTITEM_TO_GTYPE(inData));
}

void _xfdashboard_search_manager_dispose(GObject *inObject)
{
	XfdashboardSearchManager			*self=XFDASHBOARD_SEARCH_MANAGER(inObject);
	XfdashboardSearchManagerPrivate	*priv=self->priv;

	/* Release allocated resouces */
	if(priv->registeredPlugins)
	{
		g_list_foreach(priv->registeredPlugins, _xfdashboard_search_manager_dispose_unregister_plugin, self);
		g_list_free(priv->registeredPlugins);
		priv->registeredPlugins=NULL;
	}

	/* Unset singleton */
	if(G_LIKELY(G_OBJECT(searchManager)==inObject)) searchManager=NULL;

	/* Call parent's class dispose method */
	G_OBJECT_CLASS(xfdashboard_search_manager_parent_class)->dispose(inObject);
}

/* Class initialization
 * Override functions in parent classes and define properties
 * and signals
 */
void xfdashboard_search_manager_class_init(XfdashboardSearchManagerClass *klass)
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
void xfdashboard_search_manager_init(XfdashboardSearchManager *self)
{
	XfdashboardSearchManagerPrivate	*priv;

	priv=self->priv=XFDASHBOARD_SEARCH_MANAGER_GET_PRIVATE(self);

	/* Set default values */
	priv->registeredPlugins=NULL;
}

/* Implementation: Public API */

/* Get single instance of application */
XfdashboardSearchManager* xfdashboard_search_manager_get_default(void)
{
	if(G_UNLIKELY(searchManager==NULL))
	{
		searchManager=g_object_new(XFDASHBOARD_TYPE_SEARCH_MANAGER, NULL);
	}

	return(searchManager);
}

/* Register a view */
void xfdashboard_search_manager_register(XfdashboardSearchManager *self, GType inPluginType)
{
	g_return_if_fail(XFDASHBOARD_IS_SEARCH_MANAGER(self));

	XfdashboardSearchManagerPrivate	*priv=self->priv;

	/* Check if given type is not a XfdashboardSearchPlugin but a derived type from it */
	if(inPluginType==XFDASHBOARD_TYPE_SEARCH_PLUGIN ||
		g_type_is_a(inPluginType, XFDASHBOARD_TYPE_SEARCH_PLUGIN)!=TRUE)
	{
		g_warning(_("Search plugin %s is not a %s and cannot be registered"),
					g_type_name(inPluginType),
					g_type_name(XFDASHBOARD_TYPE_SEARCH_PLUGIN));
		return;
	}

	/* Register type if not already registered */
	if(g_list_find(priv->registeredPlugins, GTYPE_TO_LISTITEM(inPluginType))==NULL)
	{
		g_debug(_("Registering search plugin %s"), g_type_name(inPluginType));
		priv->registeredPlugins=g_list_append(priv->registeredPlugins, GTYPE_TO_LISTITEM(inPluginType));
		g_signal_emit(self, XfdashboardSearchManagerSignals[SIGNAL_REGISTERED], 0, inPluginType);
	}
}

/* Unregister a view */
void xfdashboard_search_manager_unregister(XfdashboardSearchManager *self, GType inPluginType)
{
	g_return_if_fail(XFDASHBOARD_IS_SEARCH_MANAGER(self));

	XfdashboardSearchManagerPrivate	*priv=self->priv;

	/* Check if given type is not a XfdashboardView but a derived type from it */
	if(inPluginType==XFDASHBOARD_TYPE_SEARCH_PLUGIN ||
		g_type_is_a(inPluginType, XFDASHBOARD_TYPE_SEARCH_PLUGIN)!=TRUE)
	{
		g_warning(_("Search plugin %s is not a %s and cannot be unregistered"),
					g_type_name(inPluginType),
					g_type_name(XFDASHBOARD_TYPE_SEARCH_PLUGIN));
		return;
	}

	/* Register type if not already registered */
	if(g_list_find(priv->registeredPlugins, GTYPE_TO_LISTITEM(inPluginType))!=NULL)
	{
		g_debug(_("Unregistering search plugin %s"), g_type_name(inPluginType));
		priv->registeredPlugins=g_list_remove(priv->registeredPlugins, GTYPE_TO_LISTITEM(inPluginType));
		g_signal_emit(self, XfdashboardSearchManagerSignals[SIGNAL_UNREGISTERED], 0, inPluginType);
	}
}

/* Get list of registered views types.
 * Returned GList must be freed with g_list_free by caller.
 */
GList* xfdashboard_search_manager_get_registered(XfdashboardSearchManager *self)
{
	g_return_val_if_fail(XFDASHBOARD_IS_SEARCH_MANAGER(self), NULL);

	/* Return a copy of list of registered view types */
	GList		*copy;

	copy=g_list_copy(self->priv->registeredPlugins);
	return(copy);
}
