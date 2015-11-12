/*
 * plugin-manager: Single-instance managing plugins
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

#ifndef __XFDASHBOARD_PLUGIN_MANAGER__
#define __XFDASHBOARD_PLUGIN_MANAGER__

#include <glib-object.h>

G_BEGIN_DECLS

#define XFDASHBOARD_TYPE_PLUGIN_MANAGER				(xfdashboard_plugin_manager_get_type())
#define XFDASHBOARD_PLUGIN_MANAGER(obj)				(G_TYPE_CHECK_INSTANCE_CAST((obj), XFDASHBOARD_TYPE_PLUGIN_MANAGER, XfdashboardPluginManager))
#define XFDASHBOARD_IS_PLUGIN_MANAGER(obj)			(G_TYPE_CHECK_INSTANCE_TYPE((obj), XFDASHBOARD_TYPE_PLUGIN_MANAGER))
#define XFDASHBOARD_PLUGIN_MANAGER_CLASS(klass)		(G_TYPE_CHECK_CLASS_CAST((klass), XFDASHBOARD_TYPE_PLUGIN_MANAGER, XfdashboardPluginManagerClass))
#define XFDASHBOARD_IS_PLUGIN_MANAGER_CLASS(klass)	(G_TYPE_CHECK_CLASS_TYPE((klass), XFDASHBOARD_TYPE_PLUGIN_MANAGER))
#define XFDASHBOARD_PLUGIN_MANAGER_GET_CLASS(obj)		(G_TYPE_INSTANCE_GET_CLASS((obj), XFDASHBOARD_TYPE_PLUGIN_MANAGER, XfdashboardPluginManagerClass))

typedef struct _XfdashboardPluginManager			XfdashboardPluginManager;
typedef struct _XfdashboardPluginManagerClass		XfdashboardPluginManagerClass;
typedef struct _XfdashboardPluginManagerPrivate		XfdashboardPluginManagerPrivate;

struct _XfdashboardPluginManager
{
	/* Parent instance */
	GObject							parent_instance;

	/* Private structure */
	XfdashboardPluginManagerPrivate	*priv;
};

struct _XfdashboardPluginManagerClass
{
	/*< private >*/
	/* Parent class */
	GObjectClass					parent_class;
};

/* Public API */
GType xfdashboard_plugin_manager_get_type(void) G_GNUC_CONST;

XfdashboardPluginManager* xfdashboard_plugin_manager_get_default(void);

gboolean xfdashboard_plugin_manager_setup(XfdashboardPluginManager *self);

G_END_DECLS

#endif	/* __XFDASHBOARD_PLUGIN_MANAGER__ */
