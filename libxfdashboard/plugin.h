/*
 * plugin: A plugin class managing loading the shared object as well as
 *         initializing and setting up extensions to this application
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

#ifndef __LIBXFDASHBOARD_PLUGIN__
#define __LIBXFDASHBOARD_PLUGIN__

#if !defined(__LIBXFDASHBOARD_H_INSIDE__) && !defined(LIBXFDASHBOARD_COMPILATION)
#error "Only <libxfdashboard/libxfdashboard.h> can be included directly."
#endif

#include <glib-object.h>
#include <gmodule.h>

G_BEGIN_DECLS

/* Public definitions */
/**
 * XfdashboardPluginFlag:
 * @XFDASHBOARD_PLUGIN_FLAG_NONE: Plugin does not request anything special.
 * @XFDASHBOARD_PLUGIN_FLAG_EARLY_INITIALIZATION: Plugin requests to get enabled before the stage is initialized
 *
 * Flags defining behaviour of this XfdashboardPlugin.
 */
typedef enum /*< flags,prefix=XFDASHBOARD_PLUGIN_FLAG >*/
{
	XFDASHBOARD_PLUGIN_FLAG_NONE=0,

	XFDASHBOARD_PLUGIN_FLAG_EARLY_INITIALIZATION=1 << 0,
} XfdashboardPluginFlag;


/* Helper macros to declare, define and register GObject types in plugins */
#define XFDASHBOARD_DECLARE_PLUGIN_TYPE(inFunctionNamePrefix) \
	void inFunctionNamePrefix##_register_plugin_type(XfdashboardPlugin *inPlugin);

#define XFDASHBOARD_DEFINE_PLUGIN_TYPE(inFunctionNamePrefix) \
	void inFunctionNamePrefix##_register_plugin_type(XfdashboardPlugin *inPlugin) \
	{ \
		inFunctionNamePrefix##_register_type(G_TYPE_MODULE(inPlugin)); \
	}

#define XFDASHBOARD_REGISTER_PLUGIN_TYPE(self, inFunctionNamePrefix) \
	inFunctionNamePrefix##_register_plugin_type(XFDASHBOARD_PLUGIN(self));


/* Object declaration */
#define XFDASHBOARD_TYPE_PLUGIN				(xfdashboard_plugin_get_type())
#define XFDASHBOARD_PLUGIN(obj)				(G_TYPE_CHECK_INSTANCE_CAST((obj), XFDASHBOARD_TYPE_PLUGIN, XfdashboardPlugin))
#define XFDASHBOARD_IS_PLUGIN(obj)			(G_TYPE_CHECK_INSTANCE_TYPE((obj), XFDASHBOARD_TYPE_PLUGIN))
#define XFDASHBOARD_PLUGIN_CLASS(klass)		(G_TYPE_CHECK_CLASS_CAST((klass), XFDASHBOARD_TYPE_PLUGIN, XfdashboardPluginClass))
#define XFDASHBOARD_IS_PLUGIN_CLASS(klass)	(G_TYPE_CHECK_CLASS_TYPE((klass), XFDASHBOARD_TYPE_PLUGIN))
#define XFDASHBOARD_PLUGIN_GET_CLASS(obj)	(G_TYPE_INSTANCE_GET_CLASS((obj), XFDASHBOARD_TYPE_PLUGIN, XfdashboardPluginClass))

typedef struct _XfdashboardPlugin			XfdashboardPlugin;
typedef struct _XfdashboardPluginClass		XfdashboardPluginClass;
typedef struct _XfdashboardPluginPrivate	XfdashboardPluginPrivate;

struct _XfdashboardPlugin
{
	/*< private >*/
	/* Parent instance */
	GTypeModule						parent_instance;

	/* Private structure */
	XfdashboardPluginPrivate		*priv;
};

struct _XfdashboardPluginClass
{
	/*< private >*/
	/* Parent class */
	GTypeModuleClass				parent_class;

	/*< public >*/
	/* Virtual functions */
	void (*enable)(XfdashboardPlugin *self);
	void (*disable)(XfdashboardPlugin *self);

	GObject* (*configure)(XfdashboardPlugin *self);
};

/* Error */
#define XFDASHBOARD_PLUGIN_ERROR					(xfdashboard_plugin_error_quark())

GQuark xfdashboard_plugin_error_quark(void);

typedef enum /*< prefix=XFDASHBOARD_PLUGIN_ERROR >*/
{
	XFDASHBOARD_PLUGIN_ERROR_NONE,
	XFDASHBOARD_PLUGIN_ERROR_ERROR,
} XfdashboardPluginErrorEnum;

/* Public API */
GType xfdashboard_plugin_get_type(void) G_GNUC_CONST;

XfdashboardPlugin* xfdashboard_plugin_new(const gchar *inPluginFilename, GError **outError);

const gchar* xfdashboard_plugin_get_id(XfdashboardPlugin *self);
XfdashboardPluginFlag xfdashboard_plugin_get_flags(XfdashboardPlugin *self);

void xfdashboard_plugin_set_info(XfdashboardPlugin *self,
									const gchar *inFirstPropertyName, ...)
									G_GNUC_NULL_TERMINATED;

gboolean xfdashboard_plugin_is_enabled(XfdashboardPlugin *self);
void xfdashboard_plugin_enable(XfdashboardPlugin *self);
void xfdashboard_plugin_disable(XfdashboardPlugin *self);

const gchar* xfdashboard_plugin_get_config_path(XfdashboardPlugin *self);
const gchar* xfdashboard_plugin_get_cache_path(XfdashboardPlugin *self);
const gchar* xfdashboard_plugin_get_data_path(XfdashboardPlugin *self);

G_END_DECLS

#endif	/* __LIBXFDASHBOARD_PLUGIN__ */
