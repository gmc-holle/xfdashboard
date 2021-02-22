/*
 * plugin: A plugin class managing loading the shared object as well as
 *         initializing and setting up extensions to this application
 * 
 * Copyright 2012-2020 Stephan Haller <nomad@froevel.de>
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

#include <libxfdashboard/plugin.h>

#include <glib/gi18n-lib.h>

#include <libxfdashboard/enums.h>
#include <libxfdashboard/marshal.h>
#include <libxfdashboard/compat.h>
#include <libxfdashboard/debug.h>


/* Forward declaration */
typedef enum /*< skip,prefix=XFDASHBOARD_PLUGIN_STATE >*/
{
	XFDASHBOARD_PLUGIN_STATE_NONE=0,
	XFDASHBOARD_PLUGIN_STATE_INITIALIZED,
	XFDASHBOARD_PLUGIN_STATE_ENABLED,
} XfdashboardPluginState;


/* Define this class in GObject system */
struct _XfdashboardPluginPrivate
{
	/* Properties related */
	gchar						*id;
	XfdashboardPluginFlag		flags;
	gchar						*name;
	gchar						*description;
	gchar						*author;
	gchar						*copyright;
	gchar						*license;

	/* Instance related */
	gchar						*filename;
	GModule						*module;
	void 						(*initialize)(XfdashboardPlugin *self);
	XfdashboardPluginState		state;
	gchar						*lastLoadingError;

	gpointer					userData;
	GDestroyNotify				userDataDestroyCallback;
};

G_DEFINE_TYPE_WITH_PRIVATE(XfdashboardPlugin,
							xfdashboard_plugin,
							G_TYPE_TYPE_MODULE)

/* Properties */
enum
{
	PROP_0,

	PROP_FILENAME,

	PROP_ID,
	PROP_FLAGS,
	PROP_NAME,
	PROP_DESCRIPTION,
	PROP_AUTHOR,
	PROP_COPYRIGHT,
	PROP_LICENSE,

	PROP_LAST
};

static GParamSpec* XfdashboardPluginProperties[PROP_LAST]={ 0, };

/* Signals */
enum
{
	/* Actions */
	ACTION_ENABLE,
	ACTION_DISABLE,
	ACTION_CONFIGURE,

	SIGNAL_LAST
};

static guint XfdashboardPluginSignals[SIGNAL_LAST]={ 0, };


/* IMPLEMENTATION: Private variables and methods */
#define XFDASHBOARD_PLUGIN_FUNCTION_NAME_INITIALIZE		"plugin_init"

/* Get display name for XFDASHBOARD_PLUGIN_STATE_* enum values */
static const gchar* _xfdashboard_plugin_get_plugin_state_value_name(XfdashboardPluginState inState)
{
	g_return_val_if_fail(inState>XFDASHBOARD_PLUGIN_STATE_ENABLED, NULL);

	/* Lookup name for value and return it */
	switch(inState)
	{
		case XFDASHBOARD_PLUGIN_STATE_NONE:
			return("none");

		case XFDASHBOARD_PLUGIN_STATE_INITIALIZED:
			return("initialized");

		case XFDASHBOARD_PLUGIN_STATE_ENABLED:
			return("enabled");

		default:
			break;
	}

	/* We should normally never get here. But if we do return NULL. */
	return(NULL);
}

/* Return error message of last load attempt */
static const gchar* _xfdashboard_plugin_get_loading_error(XfdashboardPlugin *self)
{
	XfdashboardPluginPrivate	*priv;

	g_return_val_if_fail(XFDASHBOARD_IS_PLUGIN(self), NULL);

	priv=self->priv;

	/* Return error message of last loading attempt */
	return(priv->lastLoadingError);
}

/* Set file name for plugin */
static void _xfdashboard_plugin_set_filename(XfdashboardPlugin *self, const gchar *inFilename)
{
	XfdashboardPluginPrivate		*priv;

	g_return_if_fail(XFDASHBOARD_IS_PLUGIN(self));
	g_return_if_fail(inFilename && *inFilename);
	g_return_if_fail(self->priv->state==XFDASHBOARD_PLUGIN_STATE_NONE);
	g_return_if_fail(self->priv->filename==NULL);

	priv=self->priv;

	/* Set value if changed */
	if(g_strcmp0(priv->filename, inFilename)!=0)
	{
		/* Set value */
		if(priv->filename) g_free(priv->filename);
		priv->filename=g_strdup(inFilename);

		/* Notify about property change */
		g_object_notify_by_pspec(G_OBJECT(self), XfdashboardPluginProperties[PROP_FILENAME]);
	}
}

/* Set ID for plugin */
static void _xfdashboard_plugin_set_id(XfdashboardPlugin *self, const gchar *inID)
{
	XfdashboardPluginPrivate		*priv;

	g_return_if_fail(XFDASHBOARD_IS_PLUGIN(self));
	g_return_if_fail(inID && *inID);
	g_return_if_fail(self->priv->id==NULL);
	g_return_if_fail(self->priv->state==XFDASHBOARD_PLUGIN_STATE_NONE);

	priv=self->priv;

	/* Set value if changed */
	if(g_strcmp0(priv->id, inID)!=0)
	{
		/* Set value */
		if(priv->id) g_free(priv->id);
		priv->id=g_strdup(inID);

		/* Notify about property change */
		g_object_notify_by_pspec(G_OBJECT(self), XfdashboardPluginProperties[PROP_ID]);
	}
}

/* Set flags for plugin */
static void _xfdashboard_plugin_set_flags(XfdashboardPlugin *self, XfdashboardPluginFlag inFlags)
{
	XfdashboardPluginPrivate		*priv;

	g_return_if_fail(XFDASHBOARD_IS_PLUGIN(self));
	g_return_if_fail(self->priv->flags==XFDASHBOARD_PLUGIN_FLAG_NONE);
	g_return_if_fail(self->priv->state==XFDASHBOARD_PLUGIN_STATE_NONE);

	priv=self->priv;

	/* Set value if changed */
	if(priv->flags!=inFlags)
	{
		/* Set value */
		priv->flags=inFlags;

		/* Notify about property change */
		g_object_notify_by_pspec(G_OBJECT(self), XfdashboardPluginProperties[PROP_FLAGS]);
	}
}

/* Set name for plugin */
static void _xfdashboard_plugin_set_name(XfdashboardPlugin *self, const gchar *inName)
{
	XfdashboardPluginPrivate		*priv;

	g_return_if_fail(XFDASHBOARD_IS_PLUGIN(self));
	g_return_if_fail(self->priv->name==NULL);
	g_return_if_fail(self->priv->state==XFDASHBOARD_PLUGIN_STATE_NONE);

	priv=self->priv;

	/* Set value if changed */
	if(g_strcmp0(priv->name, inName)!=0)
	{
		/* Set value */
		if(priv->name)
		{
			g_free(priv->name);
			priv->name=NULL;
		}

		if(inName) priv->name=g_strdup(inName);

		/* Notify about property change */
		g_object_notify_by_pspec(G_OBJECT(self), XfdashboardPluginProperties[PROP_NAME]);
	}
}

/* Set description for plugin */
static void _xfdashboard_plugin_set_description(XfdashboardPlugin *self, const gchar *inDescription)
{
	XfdashboardPluginPrivate		*priv;

	g_return_if_fail(XFDASHBOARD_IS_PLUGIN(self));
	g_return_if_fail(self->priv->description==NULL);
	g_return_if_fail(self->priv->state==XFDASHBOARD_PLUGIN_STATE_NONE);

	priv=self->priv;

	/* Set value if changed */
	if(g_strcmp0(priv->description, inDescription)!=0)
	{
		/* Set value */
		if(priv->description)
		{
			g_free(priv->description);
			priv->description=NULL;
		}

		if(inDescription) priv->description=g_strdup(inDescription);

		/* Notify about property change */
		g_object_notify_by_pspec(G_OBJECT(self), XfdashboardPluginProperties[PROP_DESCRIPTION]);
	}
}

/* Set author for plugin */
static void _xfdashboard_plugin_set_author(XfdashboardPlugin *self, const gchar *inAuthor)
{
	XfdashboardPluginPrivate		*priv;

	g_return_if_fail(XFDASHBOARD_IS_PLUGIN(self));
	g_return_if_fail(self->priv->author==NULL);
	g_return_if_fail(self->priv->state==XFDASHBOARD_PLUGIN_STATE_NONE);

	priv=self->priv;

	/* Set value if changed */
	if(g_strcmp0(priv->author, inAuthor)!=0)
	{
		/* Set value */
		if(priv->author)
		{
			g_free(priv->author);
			priv->author=NULL;
		}

		if(inAuthor) priv->author=g_strdup(inAuthor);

		/* Notify about property change */
		g_object_notify_by_pspec(G_OBJECT(self), XfdashboardPluginProperties[PROP_AUTHOR]);
	}
}

/* Set copyright for plugin */
static void _xfdashboard_plugin_set_copyright(XfdashboardPlugin *self, const gchar *inCopyright)
{
	XfdashboardPluginPrivate		*priv;

	g_return_if_fail(XFDASHBOARD_IS_PLUGIN(self));
	g_return_if_fail(self->priv->copyright==NULL);
	g_return_if_fail(self->priv->state==XFDASHBOARD_PLUGIN_STATE_NONE);

	priv=self->priv;

	/* Set value if changed */
	if(g_strcmp0(priv->copyright, inCopyright)!=0)
	{
		/* Set value */
		if(priv->copyright)
		{
			g_free(priv->copyright);
			priv->copyright=NULL;
		}

		if(inCopyright) priv->copyright=g_strdup(inCopyright);

		/* Notify about property change */
		g_object_notify_by_pspec(G_OBJECT(self), XfdashboardPluginProperties[PROP_COPYRIGHT]);
	}
}

/* Set license for plugin */
static void _xfdashboard_plugin_set_license(XfdashboardPlugin *self, const gchar *inLicense)
{
	XfdashboardPluginPrivate		*priv;

	g_return_if_fail(XFDASHBOARD_IS_PLUGIN(self));
	g_return_if_fail(self->priv->license==NULL);
	g_return_if_fail(self->priv->state==XFDASHBOARD_PLUGIN_STATE_NONE);

	priv=self->priv;

	/* Set value if changed */
	if(g_strcmp0(priv->license, inLicense)!=0)
	{
		/* Set value */
		if(priv->license)
		{
			g_free(priv->license);
			priv->license=NULL;
		}

		if(inLicense) priv->license=g_strdup(inLicense);

		/* Notify about property change */
		g_object_notify_by_pspec(G_OBJECT(self), XfdashboardPluginProperties[PROP_LICENSE]);
	}
}

/* Destroy user data */
static void _xfdashboard_plugin_destroy_user_data(XfdashboardPlugin *self)
{
	XfdashboardPluginPrivate		*priv;

	g_return_if_fail(XFDASHBOARD_IS_PLUGIN(self));

	priv=self->priv;

	/* Check if user data is set and call its destruction callback function
	 * if it was set also.
	 */
	if(priv->userData)
	{
		/* Call destruction callback function with user data if available */
		if(priv->userDataDestroyCallback)
		{
			(priv->userDataDestroyCallback)(priv->userData);
		}

		/* Unset user data and destruction callback */
		priv->userData=NULL;
		priv->userDataDestroyCallback=NULL;
	}
}


/* IMPLEMENTATION: GTypeModule */

/* Load and initialize plugin */
static gboolean _xfdashboard_plugin_load(GTypeModule *inModule)
{
	XfdashboardPlugin			*self;
	XfdashboardPluginPrivate	*priv;
	guint						signalID;
	gulong						handlerID;

	g_return_val_if_fail(XFDASHBOARD_IS_PLUGIN(inModule), FALSE);
	g_return_val_if_fail(G_IS_TYPE_MODULE(inModule), FALSE);

	self=XFDASHBOARD_PLUGIN(inModule);
	priv=self->priv;

	/* Reset last loading error if set */
	if(priv->lastLoadingError)
	{
		g_free(priv->lastLoadingError);
		priv->lastLoadingError=NULL;
	}

	/* Check if path to plugin was set and exists */
	if(!priv->filename)
	{
		priv->lastLoadingError=g_strdup("Missing path to plugin");
		return(FALSE);
	}

	if(!g_file_test(priv->filename, G_FILE_TEST_IS_REGULAR))
	{
		priv->lastLoadingError=g_strdup_printf("Path '%s' does not exist", priv->filename);
		return(FALSE);
	}

	/* Check that plugin is not in any state */
	if(priv->state!=XFDASHBOARD_PLUGIN_STATE_NONE)
	{
		priv->lastLoadingError=
			g_strdup_printf("Bad state '%s' - expected '%s",
							_xfdashboard_plugin_get_plugin_state_value_name(priv->state),
							_xfdashboard_plugin_get_plugin_state_value_name(XFDASHBOARD_PLUGIN_STATE_NONE));
		return(FALSE);
	}

	/* Open plugin module */
	if(priv->module)
	{
		priv->lastLoadingError=g_strdup("Plugin was already initialized");
		return(FALSE);
	}

	priv->module=g_module_open(priv->filename, G_MODULE_BIND_LAZY | G_MODULE_BIND_LOCAL);
	if(!priv->module)
	{
		priv->lastLoadingError=g_strdup(g_module_error());
		return(FALSE);
	}

	/* Check that plugin provides all necessary functions and get the address
	 * to these functions.
	 */
	if(!g_module_symbol(priv->module, XFDASHBOARD_PLUGIN_FUNCTION_NAME_INITIALIZE, (gpointer*)&priv->initialize))
	{
		priv->lastLoadingError=g_strdup(g_module_error());
		return(FALSE);
	}

	/* Initialize plugin */
	if(priv->initialize)
	{
		priv->initialize(self);
	}
		else
		{
			/* If we get here the virtual function was not overridden */
			priv->lastLoadingError=g_strdup_printf("Plugin does not implement required function %s", XFDASHBOARD_PLUGIN_FUNCTION_NAME_INITIALIZE);
			g_critical("Loading plugin at '%s' failed: %s", priv->filename, priv->lastLoadingError);
			return(FALSE);
		}

	/* Check that plugin has required properties set */
	if(!priv->id)
	{
		priv->lastLoadingError=g_strdup("Plugin did not set required ID");
		return(FALSE);
	}

	/* Check if plugin is valid, that means that plugin can be enabled and disabled */
	signalID=g_signal_lookup("enable", XFDASHBOARD_TYPE_PLUGIN);
	handlerID=g_signal_handler_find(self,
									G_SIGNAL_MATCH_ID,
									signalID,
									0,
									NULL,
									NULL,
									NULL);
	if(!handlerID)
	{
		priv->lastLoadingError=g_strdup("Plugin cannot be enabled");
		g_critical("Loading plugin at '%s' failed: %s", priv->filename, priv->lastLoadingError);
		return(FALSE);
	}

	signalID=g_signal_lookup("disable", XFDASHBOARD_TYPE_PLUGIN);
	handlerID=g_signal_handler_find(self,
									G_SIGNAL_MATCH_ID,
									signalID,
									0,
									NULL,
									NULL,
									NULL);
	if(!handlerID)
	{
		priv->lastLoadingError=g_strdup("Plugin cannot be disabled");
		g_critical("Loading plugin at '%s' failed: %s", priv->filename, priv->lastLoadingError);
		return(FALSE);
	}

	/* Set state of plugin */
	priv->state=XFDASHBOARD_PLUGIN_STATE_INITIALIZED;

	/* If we get here then loading and initializing plugin was successful */
	XFDASHBOARD_DEBUG(self, PLUGINS,
						"Loaded plugin '%s' successfully:\n  File: %s\n  Name: %s\n  Description: %s\n  Author: %s\n  Copyright: %s\n  License: %s",
						priv->id,
						priv->filename,
						priv->name ? priv->name : "",
						priv->description ? priv->description : "",
						priv->author ? priv->author : "",
						priv->copyright ? priv->copyright : "",
						priv->license ? priv->license : "");

	return(TRUE);
}

/* Disable and unload plugin */
static void _xfdashboard_plugin_unload(GTypeModule *inModule)
{
	XfdashboardPlugin			*self;
	XfdashboardPluginPrivate	*priv;

	g_return_if_fail(XFDASHBOARD_IS_PLUGIN(inModule));
	g_return_if_fail(G_IS_TYPE_MODULE(inModule));

	self=XFDASHBOARD_PLUGIN(inModule);
	priv=self->priv;

	/* Disable plugin if it is still enabled */
	if(priv->state==XFDASHBOARD_PLUGIN_STATE_ENABLED)
	{
		XFDASHBOARD_DEBUG(self, PLUGINS,
							"Disabing plugin '%s' before unloading module",
							priv->id);
		xfdashboard_plugin_disable(self);
	}

	/* Close plugin module */
	if(priv->module)
	{
		/* Close module */
		if(!g_module_close(priv->module))
		{
			g_warning("Plugin '%s' could not be unloaded successfully: %s",
						priv->id ? priv->id : "Unknown",
						g_module_error());
			return;
		}

		/* Unset module and function pointers from plugin module */
		priv->initialize=NULL;

		priv->module=NULL;
	}

	/* Set state of plugin */
	priv->state=XFDASHBOARD_PLUGIN_STATE_NONE;
}

/* IMPLEMENTATION: GObject */

/* Dispose this object */
static void _xfdashboard_plugin_dispose(GObject *inObject)
{
	XfdashboardPlugin			*self=XFDASHBOARD_PLUGIN(inObject);
	XfdashboardPluginPrivate	*priv=self->priv;

	/* First of all destroy user data as long as this object was not disposed */
	_xfdashboard_plugin_destroy_user_data(self);

	/* Release allocated resources */
	if(priv->module)
	{
		_xfdashboard_plugin_unload(G_TYPE_MODULE(self));
	}

	if(priv->lastLoadingError)
	{
		g_free(priv->lastLoadingError);
		priv->lastLoadingError=NULL;
	}

	if(priv->id)
	{
		g_free(priv->id);
		priv->id=NULL;
	}

	if(priv->name)
	{
		g_free(priv->name);
		priv->name=NULL;
	}

	if(priv->description)
	{
		g_free(priv->description);
		priv->description=NULL;
	}

	if(priv->author)
	{
		g_free(priv->author);
		priv->author=NULL;
	}

	if(priv->copyright)
	{
		g_free(priv->copyright);
		priv->copyright=NULL;
	}

	if(priv->license)
	{
		g_free(priv->license);
		priv->license=NULL;
	}

	/* Sanity checks that module was unloaded - at least by us */
	g_assert(priv->initialize==NULL);

	/* Call parent's class dispose method */
	G_OBJECT_CLASS(xfdashboard_plugin_parent_class)->dispose(inObject);
}

/* Set/get properties */
static void _xfdashboard_plugin_set_property(GObject *inObject,
											guint inPropID,
											const GValue *inValue,
											GParamSpec *inSpec)
{
	XfdashboardPlugin			*self=XFDASHBOARD_PLUGIN(inObject);
	
	switch(inPropID)
	{
		case PROP_FILENAME:
			_xfdashboard_plugin_set_filename(self, g_value_get_string(inValue));
			break;

		case PROP_ID:
			_xfdashboard_plugin_set_id(self, g_value_get_string(inValue));
			break;

		case PROP_FLAGS:
			_xfdashboard_plugin_set_flags(self, g_value_get_flags(inValue));
			break;

		case PROP_NAME:
			_xfdashboard_plugin_set_name(self, g_value_get_string(inValue));
			break;

		case PROP_DESCRIPTION:
			_xfdashboard_plugin_set_description(self, g_value_get_string(inValue));
			break;

		case PROP_AUTHOR:
			_xfdashboard_plugin_set_author(self, g_value_get_string(inValue));
			break;

		case PROP_COPYRIGHT:
			_xfdashboard_plugin_set_copyright(self, g_value_get_string(inValue));
			break;

		case PROP_LICENSE:
			_xfdashboard_plugin_set_license(self, g_value_get_string(inValue));
			break;

		default:
			G_OBJECT_WARN_INVALID_PROPERTY_ID(inObject, inPropID, inSpec);
			break;
	}
}

static void _xfdashboard_plugin_get_property(GObject *inObject,
												guint inPropID,
												GValue *outValue,
												GParamSpec *inSpec)
{
	XfdashboardPlugin			*self=XFDASHBOARD_PLUGIN(inObject);
	XfdashboardPluginPrivate	*priv=self->priv;

	switch(inPropID)
	{
		case PROP_FILENAME:
			g_value_set_string(outValue, priv->filename);
			break;

		case PROP_ID:
			g_value_set_string(outValue, priv->id);
			break;

		case PROP_FLAGS:
			g_value_set_flags(outValue, priv->flags);
			break;

		case PROP_NAME:
			g_value_set_string(outValue, priv->name);
			break;

		case PROP_DESCRIPTION:
			g_value_set_string(outValue, priv->description);
			break;

		case PROP_AUTHOR:
			g_value_set_string(outValue, priv->author);
			break;

		case PROP_COPYRIGHT:
			g_value_set_string(outValue, priv->copyright);
			break;

		case PROP_LICENSE:
			g_value_set_string(outValue, priv->license);
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
static void xfdashboard_plugin_class_init(XfdashboardPluginClass *klass)
{
	GTypeModuleClass		*moduleClass=G_TYPE_MODULE_CLASS(klass);
	GObjectClass			*gobjectClass=G_OBJECT_CLASS(klass);

	/* Override functions */
	moduleClass->load=_xfdashboard_plugin_load;
	moduleClass->unload=_xfdashboard_plugin_unload;

	gobjectClass->set_property=_xfdashboard_plugin_set_property;
	gobjectClass->get_property=_xfdashboard_plugin_get_property;
	gobjectClass->dispose=_xfdashboard_plugin_dispose;

	/* Define properties */
	XfdashboardPluginProperties[PROP_FILENAME]=
		g_param_spec_string("filename",
							"File name",
							"Path and file name of this plugin",
							NULL,
							G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS | G_PARAM_CONSTRUCT_ONLY);

	XfdashboardPluginProperties[PROP_ID]=
		g_param_spec_string("id",
							"ID",
							"The unique ID for this plugin",
							NULL,
							G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS | G_PARAM_CONSTRUCT_ONLY);

	XfdashboardPluginProperties[PROP_FLAGS]=
		g_param_spec_flags("flags",
							"Flags",
							"Flags defining behaviour of this plugin",
							XFDASHBOARD_TYPE_PLUGIN_FLAG,
							XFDASHBOARD_PLUGIN_FLAG_NONE,
							G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS);

	XfdashboardPluginProperties[PROP_NAME]=
		g_param_spec_string("name",
							"name",
							"Name of plugin",
							NULL,
							G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS);

	XfdashboardPluginProperties[PROP_DESCRIPTION]=
		g_param_spec_string("description",
							"Description",
							"A short description about this plugin",
							NULL,
							G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS);

	XfdashboardPluginProperties[PROP_AUTHOR]=
		g_param_spec_string("author",
							"Author",
							"The author of this plugin",
							NULL,
							G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS);

	XfdashboardPluginProperties[PROP_COPYRIGHT]=
		g_param_spec_string("copyright",
							"Copyright",
							"The copyright of this plugin which usually contains year of development",
							NULL,
							G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS);

	XfdashboardPluginProperties[PROP_LICENSE]=
		g_param_spec_string("license",
							"License",
							"The license of this plugin",
							NULL,
							G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS);

	g_object_class_install_properties(gobjectClass, PROP_LAST, XfdashboardPluginProperties);

	/* Define signals */
	XfdashboardPluginSignals[ACTION_ENABLE]=
		g_signal_new("enable",
						G_TYPE_FROM_CLASS(klass),
						G_SIGNAL_RUN_LAST | G_SIGNAL_ACTION,
						G_STRUCT_OFFSET(XfdashboardPluginClass, enable),
						NULL,
						NULL,
						g_cclosure_marshal_VOID__VOID,
						G_TYPE_NONE,
						0);

	XfdashboardPluginSignals[ACTION_DISABLE]=
		g_signal_new("disable",
						G_TYPE_FROM_CLASS(klass),
						G_SIGNAL_RUN_LAST | G_SIGNAL_ACTION,
						G_STRUCT_OFFSET(XfdashboardPluginClass, disable),
						NULL,
						NULL,
						g_cclosure_marshal_VOID__VOID,
						G_TYPE_NONE,
						0);

	XfdashboardPluginSignals[ACTION_CONFIGURE]=
		g_signal_new("configure",
						G_TYPE_FROM_CLASS(klass),
						G_SIGNAL_RUN_LAST | G_SIGNAL_ACTION,
						G_STRUCT_OFFSET(XfdashboardPluginClass, configure),
						NULL,
						NULL,
						_xfdashboard_marshal_OBJECT__VOID,
						G_TYPE_OBJECT,
						0);
}

/* Object initialization
 * Create private structure and set up default values
 */
static void xfdashboard_plugin_init(XfdashboardPlugin *self)
{
	XfdashboardPluginPrivate	*priv;

	priv=self->priv=xfdashboard_plugin_get_instance_private(self);

	/* Set up default values */
	priv->filename=NULL;
	priv->module=NULL;
	priv->initialize=NULL;
	priv->state=XFDASHBOARD_PLUGIN_STATE_NONE;
	priv->lastLoadingError=NULL;
	priv->userData=NULL;
	priv->userDataDestroyCallback=NULL;

	priv->id=NULL;
	priv->flags=XFDASHBOARD_PLUGIN_FLAG_NONE;
	priv->name=NULL;
	priv->description=NULL;
	priv->author=NULL;
	priv->copyright=NULL;
	priv->license=NULL;
}


/* IMPLEMENTATION: Errors */

GQuark xfdashboard_plugin_error_quark(void)
{
	return(g_quark_from_static_string("xfdashboard-plugin-error-quark"));
}


/* IMPLEMENTATION: Public API */

/**
 * xfdashboard_plugin_new:
 *
 * Creates a new uninitialized plugin instance of type #XfdashboardPlugin
 * which will be loaded from path at @inPluginFilename. The plugin ID is
 * retrieved from the basename of plugin's path, i.e. the file name without
 * the absolute path and without file extension.
 *
 * The plugin instance created is uninitialized, i.e. it is neither loaded
 * nor initialized. This has to be done after successful creation by calling
 * xfdashboard_plugin_enable().
 *
 * In case of errors %NULL will be returned and the error is set at
 * @outError.
 *
 * Return value: (transfer full): The instance of #XfdashboardPlugin.
 *   Use g_object_unref() when done.
 */
XfdashboardPlugin* xfdashboard_plugin_new(const gchar *inPluginFilename, GError **outError)
{
	GObject			*plugin;
	gchar			*pluginBasename;
	gchar			*pluginID;

	g_return_val_if_fail(inPluginFilename && *inPluginFilename, NULL);
	g_return_val_if_fail(outError==NULL || *outError==NULL, FALSE);

	/* Get plugin ID from filename */
	pluginBasename=g_filename_display_basename(inPluginFilename);
	if(!pluginBasename)
	{
		/* Set error */
		g_set_error(outError,
					XFDASHBOARD_PLUGIN_ERROR,
					XFDASHBOARD_PLUGIN_ERROR_ERROR,
					"Could not get plugin ID for file %s",
					inPluginFilename);

		/* Return NULL to indicate failure */
		return(NULL);
	}

	if(g_str_has_suffix(pluginBasename, G_MODULE_SUFFIX))
	{
		pluginID=g_strndup(pluginBasename, strlen(pluginBasename)-strlen(G_MODULE_SUFFIX)-1);
	}
		else
		{
			pluginID=g_strdup(pluginBasename);
		}

	/* Create object instance */
	plugin=g_object_new(XFDASHBOARD_TYPE_PLUGIN,
						"filename", inPluginFilename,
						"id", pluginID,
						NULL);
	if(!plugin)
	{
		/* Set error */
		g_set_error(outError,
					XFDASHBOARD_PLUGIN_ERROR,
					XFDASHBOARD_PLUGIN_ERROR_ERROR,
					"Could not create plugin instance");

		/* Release allocated resources */
		if(pluginID) g_free(pluginID);
		if(pluginBasename) g_free(pluginBasename);

		/* Return NULL to indicate failure */
		return(NULL);
	}

	/* Load plugin */
	if(!g_type_module_use(G_TYPE_MODULE(plugin)))
	{
		/* Set error */
		g_set_error(outError,
					XFDASHBOARD_PLUGIN_ERROR,
					XFDASHBOARD_PLUGIN_ERROR_ERROR,
					"%s",
					_xfdashboard_plugin_get_loading_error(XFDASHBOARD_PLUGIN(plugin)));

		/* Release allocated resources */
		if(pluginID) g_free(pluginID);
		if(pluginBasename) g_free(pluginBasename);

		/* At this point we return NULL to indicate failure although the object
		 * instance (subclassing GTypeModule) now exists and it was tried to
		 * use it (via g_type_module_use). As describe in GObject documentation
		 * the object must not be unreffed via g_object_unref() but we also should
		 * not call g_type_module_unuse() because loading failed and the reference
		 * counter was not increased.
		 */
		return(NULL);
	}

	/* Release allocated resources */
	if(pluginID) g_free(pluginID);
	if(pluginBasename) g_free(pluginBasename);

	/* Plugin loaded so return it */
	return(XFDASHBOARD_PLUGIN(plugin));
}

/**
 * xfdashboard_plugin_get_id:
 * @self: A #XfdashboardPlugin
 *
 * Retrieve the ID of plugin at @self.
 *
 * Return value: The plugin ID
 */
const gchar* xfdashboard_plugin_get_id(XfdashboardPlugin *self)
{
	g_return_val_if_fail(XFDASHBOARD_IS_PLUGIN(self), NULL);

	return(self->priv->id);
}

/**
 * xfdashboard_plugin_get_flags:
 * @self: A #XfdashboardPlugin
 *
 * Retrieve the flags associated with plugin at @self.
 *
 * Return value: The flags of type #XfdashboardPluginFlag
 */
XfdashboardPluginFlag xfdashboard_plugin_get_flags(XfdashboardPlugin *self)
{
	g_return_val_if_fail(XFDASHBOARD_IS_PLUGIN(self), XFDASHBOARD_PLUGIN_FLAG_NONE);

	return(self->priv->flags);
}

/* Set plugin information */

/**
 * xfdashboard_plugin_set_info:
 * @self: A #XfdashboardPlugin
 * @inFirstPropertyName: The name of the first informational property to set
 * @...: The value for the first informational property to set, followed
 *   optionally by more name/value pairs, followed by NULL
 *
 * Sets informational properties for plugin at @self. The plugin must not be
 * initialized or enabled.
 */
void xfdashboard_plugin_set_info(XfdashboardPlugin *self,
									const gchar *inFirstPropertyName, ...)
{
	XfdashboardPluginPrivate		*priv;
	va_list							args;

	g_return_if_fail(XFDASHBOARD_IS_PLUGIN(self));

	priv=self->priv;

	/* Check that plugin is not initialized already */
	if(priv->state!=XFDASHBOARD_PLUGIN_STATE_NONE)
	{
		g_critical("Setting plugin information for plugin '%s' at path '%s' failed: Plugin is already initialized",
					priv->id ? priv->id : "Unknown",
					priv->filename);
		return;
	}

	/* Set up properties */
	va_start(args, inFirstPropertyName);
	g_object_set_valist(G_OBJECT(self), inFirstPropertyName, args);
	va_end(args);
}

/**
 * xfdashboard_plugin_is_enabled:
 * @self: A #XfdashboardPlugin
 *
 * Retrieve if plugin at @self was successfully loaded and enabled.
 *
 * Return value: %TRUE if plugin was enabled, otherwise %FALSE
 */
gboolean xfdashboard_plugin_is_enabled(XfdashboardPlugin *self)
{
	XfdashboardPluginPrivate		*priv;

	g_return_val_if_fail(XFDASHBOARD_IS_PLUGIN(self), FALSE);

	priv=self->priv;

	/* Only return TRUE if state is enabled */
	if(priv->state==XFDASHBOARD_PLUGIN_STATE_ENABLED) return(TRUE);

	/* If we get here plugin is not in enabled state so return FALSE */
	return(FALSE);
}

/**
 * xfdashboard_plugin_enable:
 * @self: A #XfdashboardPlugin
 *
 * Enables the plugin at @self.
 */
void xfdashboard_plugin_enable(XfdashboardPlugin *self)
{
	XfdashboardPluginPrivate		*priv;
	gboolean						result;

	g_return_if_fail(XFDASHBOARD_IS_PLUGIN(self));

	priv=self->priv;
	result=FALSE;

	/* Do nothing and return immediately if plugin is enabled already */
	if(priv->state==XFDASHBOARD_PLUGIN_STATE_ENABLED)
	{
		XFDASHBOARD_DEBUG(self, PLUGINS,
							"Plugin '%s' is already enabled",
							priv->id);
		return;
	}

	/* Check that plugin is initialized */
	if(priv->state!=XFDASHBOARD_PLUGIN_STATE_INITIALIZED)
	{
		g_critical("Enabling plugin '%s' failed: Bad state '%s' - expected '%s'",
					priv->id ? priv->id : "Unknown",
					_xfdashboard_plugin_get_plugin_state_value_name(priv->state),
					_xfdashboard_plugin_get_plugin_state_value_name(XFDASHBOARD_PLUGIN_STATE_INITIALIZED));
		return;
	}

	/* Emit signal action 'enable' to enable plugin */
	g_signal_emit(self, XfdashboardPluginSignals[ACTION_ENABLE], 0, &result);
	XFDASHBOARD_DEBUG(self, PLUGINS,
						"Plugin '%s' enabled",
						priv->id);

	/* Set enabled state */
	priv->state=XFDASHBOARD_PLUGIN_STATE_ENABLED;
}

/**
 * xfdashboard_plugin_disable:
 * @self: A #XfdashboardPlugin
 *
 * Disables the plugin at @self.
 */
void xfdashboard_plugin_disable(XfdashboardPlugin *self)
{
	XfdashboardPluginPrivate		*priv;
	gboolean						result;

	g_return_if_fail(XFDASHBOARD_IS_PLUGIN(self));

	priv=self->priv;

	/* Do nothing and return immediately if plugin is not enabled */
	if(priv->state!=XFDASHBOARD_PLUGIN_STATE_ENABLED)
	{
		XFDASHBOARD_DEBUG(self, PLUGINS,
							"Plugin '%s' is already disabled",
							priv->id);
		return;
	}

	/* Emit signal action 'disable' to disable plugin */
	g_signal_emit(self, XfdashboardPluginSignals[ACTION_DISABLE], 0, &result);
	XFDASHBOARD_DEBUG(self, PLUGINS,
						"Plugin '%s' disabled",
						priv->id);

	/* Set disabled state, i.e. revert to initialized state */
	priv->state=XFDASHBOARD_PLUGIN_STATE_INITIALIZED;
}

/**
 * xfdashboard_plugin_get_user_data:
 * @self: A #XfdashboardPlugin
 *
 * Retrieve the user data of plugin at @self.
 *
 * Return value: The user data
 */
gpointer xfdashboard_plugin_get_user_data(XfdashboardPlugin *self)
{
	g_return_val_if_fail(XFDASHBOARD_IS_PLUGIN(self), NULL);

	return(self->priv->userData);
}

/**
 * xfdashboard_plugin_set_user_data:
 * @self: A #XfdashboardPlugin
 * @inUserData: The user data to set
 *
 * Sets the user data @inUserData at plugin @self but without a callback
 * function to call when user data is destroyed.
 */
void xfdashboard_plugin_set_user_data(XfdashboardPlugin *self, gpointer inUserData)
{
	xfdashboard_plugin_set_user_data_full(self, inUserData, NULL);
}

/**
 * xfdashboard_plugin_set_user_data_full:
 * @self: A #XfdashboardPlugin
 * @inUserData: The user data to set
 * @inDestroyCallback: The callback to call when destructing user data
 *
 * Sets the user data at @inUserData at plugin at @self with a callback
 * function at @inDestroyCallback which is called when user data will be
 * destroyed. The destruction callback will always be set even if user
 * data did not change. The current user data at plugin at @self will be
 * destroyed with the current destruction callback if user data changes
 * before the new user data at @inUserData and the new destruction
 * callback at @inDestroyCallback is set.
 */
void xfdashboard_plugin_set_user_data_full(XfdashboardPlugin *self, gpointer inUserData, GDestroyNotify inDestroyCallback)
{
	XfdashboardPluginPrivate		*priv;

	g_return_if_fail(XFDASHBOARD_IS_PLUGIN(self));

	priv=self->priv;

	/* Set value if changed */
	if(priv->userData!=inUserData)
	{
		/* Destroy old user data */
		_xfdashboard_plugin_destroy_user_data(self);

		/* Set value */
		priv->userData=inUserData;
	}

	/* Set new destroy callback functions */
	priv->userDataDestroyCallback=inDestroyCallback;
}
