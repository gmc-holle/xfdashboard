/*
 * search-provider: Abstract class for search plugins called providers
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

#include "search-provider.h"

#include <glib/gi18n-lib.h>

/* Define this class in GObject system */
G_DEFINE_ABSTRACT_TYPE(XfdashboardSearchProvider,
						xfdashboard_search_provider,
						G_TYPE_OBJECT)

/* Private structure - access only by public API if needed */
#define XFDASHBOARD_SEARCH_PROVIDER_GET_PRIVATE(obj) \
	(G_TYPE_INSTANCE_GET_PRIVATE((obj), XFDASHBOARD_TYPE_SEARCH_PROVIDER, XfdashboardSearchProviderPrivate))

struct _XfdashboardSearchProviderPrivate
{
	/* Instance related */
	void				*dummy;
};

/* Properties */
enum
{
	PROP_0,

	PROP_LAST
};

GParamSpec* XfdashboardSearchProviderProperties[PROP_LAST]={ 0, };

/* Signals */
enum
{
	SIGNAL_LAST
};

guint XfdashboardSearchProviderSignals[SIGNAL_LAST]={ 0, };

/* IMPLEMENTATION: Private variables and methods */

/* IMPLEMENTATION: GObject */

/* Dispose this object */
void _xfdashboard_search_provider_dispose(GObject *inObject)
{
	XfdashboardSearchProvider			*self=XFDASHBOARD_SEARCH_PROVIDER(inObject);
	XfdashboardSearchProviderPrivate	*priv=self->priv;

	/* Release allocated resources */

	/* Call parent's class dispose method */
	G_OBJECT_CLASS(xfdashboard_search_provider_parent_class)->dispose(inObject);
}

/* Set/get properties */
void _xfdashboard_search_provider_set_property(GObject *inObject,
												guint inPropID,
												const GValue *inValue,
												GParamSpec *inSpec)
{
	XfdashboardSearchProvider		*self=XFDASHBOARD_SEARCH_PROVIDER(inObject);
	
	switch(inPropID)
	{
		default:
			G_OBJECT_WARN_INVALID_PROPERTY_ID(inObject, inPropID, inSpec);
			break;
	}
}

void _xfdashboard_search_provider_get_property(GObject *inObject,
									guint inPropID,
									GValue *outValue,
									GParamSpec *inSpec)
{
	XfdashboardSearchProvider		*self=XFDASHBOARD_SEARCH_PROVIDER(inObject);

	switch(inPropID)
	{
		default:
			G_OBJECT_WARN_INVALID_PROPERTY_ID(inObject, inPropID, inSpec);
			break;
	}
}

/* Class initialization
 * Override functions in parent classes and define properties
 * and signals
 */
void xfdashboard_search_provider_class_init(XfdashboardSearchProviderClass *klass)
{
	GObjectClass			*gobjectClass=G_OBJECT_CLASS(klass);

	/* Override functions */
	gobjectClass->set_property=_xfdashboard_search_provider_set_property;
	gobjectClass->get_property=_xfdashboard_search_provider_get_property;
	gobjectClass->dispose=_xfdashboard_search_provider_dispose;

	/* Set up private structure */
	g_type_class_add_private(klass, sizeof(XfdashboardSearchProviderPrivate));
}

/* Object initialization
 * Create private structure and set up default values
 */
void xfdashboard_search_provider_init(XfdashboardSearchProvider *self)
{
	XfdashboardSearchProviderPrivate	*priv;

	priv=self->priv=XFDASHBOARD_SEARCH_PROVIDER_GET_PRIVATE(self);
}

/* Implementation: Public API */
