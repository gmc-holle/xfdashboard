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

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <libxfdashboard/search-provider.h>

#include <glib/gi18n-lib.h>

#include <libxfdashboard/actor.h>
#include <libxfdashboard/text-box.h>
#include <libxfdashboard/compat.h>
#include <libxfdashboard/debug.h>


/* Define this class in GObject system */
struct _XfdashboardSearchProviderPrivate
{
	/* Properties related */
	gchar					*providerID;
};

G_DEFINE_ABSTRACT_TYPE_WITH_PRIVATE(XfdashboardSearchProvider,
									xfdashboard_search_provider,
									G_TYPE_OBJECT)

/* Properties */
enum
{
	PROP_0,

	PROP_PROVIDER_ID,

	PROP_LAST
};

static GParamSpec* XfdashboardSearchProviderProperties[PROP_LAST]={ 0, };

/* IMPLEMENTATION: Private variables and methods */
#define XFDASHBOARD_SEARCH_PROVIDER_WARN_NOT_IMPLEMENTED(self, vfunc) \
	g_warning(_("Search provider of type %s does not implement required virtual function XfdashboardSearchProvider::%s"), \
				G_OBJECT_TYPE_NAME(self), \
				vfunc);

#define XFDASHBOARD_SEARCH_PROVIDER_NOTE_NOT_IMPLEMENTED(self, vfunc) \
	XFDASHBOARD_DEBUG(self, MISC,                                              \
						"Search provider of type %s does not implement virtual function XfdashboardSearchProvider::%s",\
						G_OBJECT_TYPE_NAME(self),                              \
						vfunc);

/* Set search provider ID */
static void _xfdashboard_search_provider_set_id(XfdashboardSearchProvider *self, const gchar *inID)
{
	XfdashboardSearchProviderPrivate	*priv=self->priv;

	g_return_if_fail(XFDASHBOARD_IS_SEARCH_PROVIDER(self));
	g_return_if_fail(inID && *inID);

	priv=self->priv;

	/* Set value if changed */
	if(g_strcmp0(priv->providerID, inID)!=0)
	{
		if(priv->providerID) g_free(priv->providerID);
		priv->providerID=g_strdup(inID);

		/* Notify about property change */
		g_object_notify_by_pspec(G_OBJECT(self), XfdashboardSearchProviderProperties[PROP_PROVIDER_ID]);
	}
}

/* IMPLEMENTATION: GObject */

/* Dispose this object */
static void _xfdashboard_search_provider_dispose(GObject *inObject)
{
	XfdashboardSearchProvider			*self=XFDASHBOARD_SEARCH_PROVIDER(inObject);
	XfdashboardSearchProviderPrivate	*priv=self->priv;

	/* Release allocated resources */
	if(priv->providerID)
	{
		g_free(priv->providerID);
		priv->providerID=NULL;
	}

	/* Call parent's class dispose method */
	G_OBJECT_CLASS(xfdashboard_search_provider_parent_class)->dispose(inObject);
}

/* Set/get properties */
static void _xfdashboard_search_provider_set_property(GObject *inObject,
														guint inPropID,
														const GValue *inValue,
														GParamSpec *inSpec)
{
	XfdashboardSearchProvider			*self=XFDASHBOARD_SEARCH_PROVIDER(inObject);

	switch(inPropID)
	{
		case PROP_PROVIDER_ID:
			_xfdashboard_search_provider_set_id(self, g_value_get_string(inValue));
			break;

		default:
			G_OBJECT_WARN_INVALID_PROPERTY_ID(inObject, inPropID, inSpec);
			break;
	}
}

static void _xfdashboard_search_provider_get_property(GObject *inObject,
														guint inPropID,
														GValue *outValue,
														GParamSpec *inSpec)
{
	XfdashboardSearchProvider			*self=XFDASHBOARD_SEARCH_PROVIDER(inObject);

	switch(inPropID)
	{
		case PROP_PROVIDER_ID:
			g_value_set_string(outValue, self->priv->providerID);
			break;

		default:
			G_OBJECT_WARN_INVALID_PROPERTY_ID(inObject, inPropID, inSpec);
			break;
	}
}

/* Class initialization
 * Override functions in parent classes and define properties
 * and signals
 */
static void xfdashboard_search_provider_class_init(XfdashboardSearchProviderClass *klass)
{
	GObjectClass			*gobjectClass=G_OBJECT_CLASS(klass);

	/* Override functions */
	gobjectClass->set_property=_xfdashboard_search_provider_set_property;
	gobjectClass->get_property=_xfdashboard_search_provider_get_property;
	gobjectClass->dispose=_xfdashboard_search_provider_dispose;

	/* Define properties */
	XfdashboardSearchProviderProperties[PROP_PROVIDER_ID]=
		g_param_spec_string("provider-id",
							_("Provider ID"),
							_("The internal ID used to register this type of search provider"),
							NULL,
							G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS | G_PARAM_CONSTRUCT_ONLY);

	g_object_class_install_properties(gobjectClass, PROP_LAST, XfdashboardSearchProviderProperties);
}

/* Object initialization
 * Create private structure and set up default values
 */
static void xfdashboard_search_provider_init(XfdashboardSearchProvider *self)
{
	XfdashboardSearchProviderPrivate	*priv;

	priv=self->priv=xfdashboard_search_provider_get_instance_private(self);

	/* Set up default values */
	priv->providerID=NULL;
}

/* IMPLEMENTATION: Public API */

/* Get view ID */
const gchar* xfdashboard_search_provider_get_id(XfdashboardSearchProvider *self)
{
	g_return_val_if_fail(XFDASHBOARD_IS_SEARCH_PROVIDER(self), NULL);

	return(self->priv->providerID);
}

/* Check if view has requested ID */
gboolean xfdashboard_search_provider_has_id(XfdashboardSearchProvider *self, const gchar *inID)
{
	XfdashboardSearchProviderPrivate	*priv;

	g_return_val_if_fail(XFDASHBOARD_IS_SEARCH_PROVIDER(self), FALSE);
	g_return_val_if_fail(inID && *inID, FALSE);

	priv=self->priv;

	/* Check if requested ID matches the ID of this search provider */
	if(g_strcmp0(priv->providerID, inID)!=0) return(FALSE);

	/* If we get here the requested ID matches search provider's ID */
	return(TRUE);
}

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
gboolean xfdashboard_search_provider_launch_search(XfdashboardSearchProvider *self,
													const gchar **inSearchTerms)
{
	XfdashboardSearchProviderClass	*klass;

	g_return_val_if_fail(XFDASHBOARD_IS_SEARCH_PROVIDER(self), FALSE);
	g_return_val_if_fail(inSearchTerms, FALSE);

	klass=XFDASHBOARD_SEARCH_PROVIDER_GET_CLASS(self);

	/* Launch search by search provider */
	if(klass->launch_search)
	{
		return(klass->launch_search(self, inSearchTerms));
	}

	/* If we get here the virtual function was not overridden */
	XFDASHBOARD_SEARCH_PROVIDER_NOTE_NOT_IMPLEMENTED(self, "launch_search");
	return(FALSE);
}

/* A result item actor was clicked so ask search provider to handle it */
gboolean xfdashboard_search_provider_activate_result(XfdashboardSearchProvider* self,
														GVariant *inResultItem,
														ClutterActor *inActor,
														const gchar **inSearchTerms)
{
	XfdashboardSearchProviderClass	*klass;

	g_return_val_if_fail(XFDASHBOARD_IS_SEARCH_PROVIDER(self), FALSE);
	g_return_val_if_fail(inResultItem, FALSE);
	g_return_val_if_fail(CLUTTER_IS_ACTOR(inActor), FALSE);
	g_return_val_if_fail(inSearchTerms, FALSE);

	klass=XFDASHBOARD_SEARCH_PROVIDER_GET_CLASS(self);

	/* Handle click action at result item actor by search provider */
	if(klass->activate_result)
	{
		return(klass->activate_result(self, inResultItem, inActor, inSearchTerms));
	}

	/* If we get here the virtual function was not overridden */
	XFDASHBOARD_SEARCH_PROVIDER_NOTE_NOT_IMPLEMENTED(self, "activate_result");
	return(FALSE);
}
