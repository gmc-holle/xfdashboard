/*
 * plugins-manager: Single-instance managing plugins
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

#ifndef __LIBXFDASHBOARD_PLUGINS_MANAGER__
#define __LIBXFDASHBOARD_PLUGINS_MANAGER__

#if !defined(__LIBXFDASHBOARD_H_INSIDE__) && !defined(LIBXFDASHBOARD_COMPILATION)
#error "Only <libxfdashboard/libxfdashboard.h> can be included directly."
#endif

#include <glib-object.h>

G_BEGIN_DECLS

#define XFDASHBOARD_TYPE_PLUGINS_MANAGER			(xfdashboard_plugins_manager_get_type())
#define XFDASHBOARD_PLUGINS_MANAGER(obj)			(G_TYPE_CHECK_INSTANCE_CAST((obj), XFDASHBOARD_TYPE_PLUGINS_MANAGER, XfdashboardPluginsManager))
#define XFDASHBOARD_IS_PLUGINS_MANAGER(obj)			(G_TYPE_CHECK_INSTANCE_TYPE((obj), XFDASHBOARD_TYPE_PLUGINS_MANAGER))
#define XFDASHBOARD_PLUGINS_MANAGER_CLASS(klass)	(G_TYPE_CHECK_CLASS_CAST((klass), XFDASHBOARD_TYPE_PLUGINS_MANAGER, XfdashboardPluginsManagerClass))
#define XFDASHBOARD_IS_PLUGINS_MANAGER_CLASS(klass)	(G_TYPE_CHECK_CLASS_TYPE((klass), XFDASHBOARD_TYPE_PLUGINS_MANAGER))
#define XFDASHBOARD_PLUGINS_MANAGER_GET_CLASS(obj)	(G_TYPE_INSTANCE_GET_CLASS((obj), XFDASHBOARD_TYPE_PLUGINS_MANAGER, XfdashboardPluginsManagerClass))

typedef struct _XfdashboardPluginsManager			XfdashboardPluginsManager;
typedef struct _XfdashboardPluginsManagerClass		XfdashboardPluginsManagerClass;
typedef struct _XfdashboardPluginsManagerPrivate	XfdashboardPluginsManagerPrivate;

/**
 * XfdashboardPluginsManager:
 *
 * The #XfdashboardPluginsManager structure contains only private data and
 * should be accessed using the provided API
 */
struct _XfdashboardPluginsManager
{
	/*< private >*/
	/* Parent instance */
	GObject								parent_instance;

	/* Private structure */
	XfdashboardPluginsManagerPrivate	*priv;
};

/**
 * XfdashboardPluginsManagerClass:
 *
 * The #XfdashboardPluginsManagerClass structure contains only private data
 */
struct _XfdashboardPluginsManagerClass
{
	/*< private >*/
	/* Parent class */
	GObjectClass						parent_class;
};

/* Public API */
GType xfdashboard_plugins_manager_get_type(void) G_GNUC_CONST;

XfdashboardPluginsManager* xfdashboard_plugins_manager_get_default(void);

gboolean xfdashboard_plugins_manager_setup(XfdashboardPluginsManager *self);

G_END_DECLS

#endif	/* __LIBXFDASHBOARD_PLUGINS_MANAGER__ */
