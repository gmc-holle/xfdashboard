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

#ifndef __XFDASHBOARD_SEARCH_MANAGER__
#define __XFDASHBOARD_SEARCH_MANAGER__

#include <glib-object.h>

#include "search-provider.h"
#include "search-result-set.h"

G_BEGIN_DECLS

#define XFDASHBOARD_TYPE_SEARCH_MANAGER				(xfdashboard_search_manager_get_type())
#define XFDASHBOARD_SEARCH_MANAGER(obj)				(G_TYPE_CHECK_INSTANCE_CAST((obj), XFDASHBOARD_TYPE_SEARCH_MANAGER, XfdashboardSearchManager))
#define XFDASHBOARD_IS_SEARCH_MANAGER(obj)			(G_TYPE_CHECK_INSTANCE_TYPE((obj), XFDASHBOARD_TYPE_SEARCH_MANAGER))
#define XFDASHBOARD_SEARCH_MANAGER_CLASS(klass)		(G_TYPE_CHECK_CLASS_CAST((klass), XFDASHBOARD_TYPE_SEARCH_MANAGER, XfdashboardSearchManagerClass))
#define XFDASHBOARD_IS_SEARCH_MANAGER_CLASS(klass)	(G_TYPE_CHECK_CLASS_TYPE((klass), XFDASHBOARD_TYPE_SEARCH_MANAGER))
#define XFDASHBOARD_SEARCH_MANAGER_GET_CLASS(obj)	(G_TYPE_INSTANCE_GET_CLASS((obj), XFDASHBOARD_TYPE_SEARCH_MANAGER, XfdashboardSearchManagerClass))

typedef struct _XfdashboardSearchManager			XfdashboardSearchManager;
typedef struct _XfdashboardSearchManagerClass		XfdashboardSearchManagerClass;
typedef struct _XfdashboardSearchManagerPrivate		XfdashboardSearchManagerPrivate;

struct _XfdashboardSearchManager
{
	/* Parent instance */
	GObject								parent_instance;

	/* Private structure */
	XfdashboardSearchManagerPrivate		*priv;
};

struct _XfdashboardSearchManagerClass
{
	/*< private >*/
	/* Parent class */
	GObjectClass						parent_class;

	/*< public >*/
	/* Virtual functions */
	void (*registered)(XfdashboardSearchManager *self, XfdashboardSearchProvider *inProvider);
	void (*unregistered)(XfdashboardSearchManager *self, XfdashboardSearchProvider *inProvider);
};

/* Public API */
GType xfdashboard_search_manager_get_type(void) G_GNUC_CONST;

XfdashboardSearchManager* xfdashboard_search_manager_get_default(void);

void xfdashboard_search_manager_register(XfdashboardSearchManager *self, GType inProviderType);
void xfdashboard_search_manager_unregister(XfdashboardSearchManager *self, GType inProviderType);
GList* xfdashboard_search_manager_get_registered(XfdashboardSearchManager *self);

gchar** xfdashboard_search_manager_get_search_terms_from_string(const gchar *inString, const gchar *inDelimiters);

G_END_DECLS

#endif	/* __XFDASHBOARD_SEARCH_MANAGER__ */
