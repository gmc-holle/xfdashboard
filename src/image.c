/*
 * image: An asynchronous loaded and cached image content
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

#include "image.h"
#include "utils.h"
#include "application.h"

#include <glib/gi18n-lib.h>
#include <gtk/gtk.h>

// TODO: #undef g_debug
// TODO: #define g_debug g_message

/* Define this class in GObject system */
G_DEFINE_TYPE(XfdashboardImage,
				xfdashboard_image,
				CLUTTER_TYPE_IMAGE)

/* Private structure - access only by public API if needed */
#define XFDASHBOARD_IMAGE_GET_PRIVATE(obj) \
	(G_TYPE_INSTANCE_GET_PRIVATE((obj), XFDASHBOARD_TYPE_IMAGE, XfdashboardImagePrivate))

struct _XfdashboardImagePrivate
{
	/* Properties related */
	gchar			*key;
};

/* Properties */
enum
{
	PROP_0,

	PROP_KEY,

	PROP_LAST
};

static GParamSpec* XfdashboardImageProperties[PROP_LAST]={ 0, };

/* IMPLEMENTATION: Private variables and methods */
static GHashTable*	_xfdashboard_image_cache=NULL;
static guint		_xfdashboard_image_cache_shutdownSignalID=0;

/* Destroy cache hashtable */
static void _xfdashboard_image_destroy_cache(void)
{
	XfdashboardApplication		*application;
	gint						cacheSize;

	/* Only an existing cache can be destroyed */
	if(!_xfdashboard_image_cache) return;

	/* Disconnect application "shutdown" signal handler */
	application=xfdashboard_application_get_default();
	g_signal_handler_disconnect(application, _xfdashboard_image_cache_shutdownSignalID);
	_xfdashboard_image_cache_shutdownSignalID=0;

	/* Destroy cache hashtable */
	cacheSize=g_hash_table_size(_xfdashboard_image_cache);
	if(cacheSize>0) g_warning(_("Destroying image cache still containing %d images."), cacheSize);

	g_debug("Destroying image cache hashtable");
	g_hash_table_destroy(_xfdashboard_image_cache);
	_xfdashboard_image_cache=NULL;
}

/* Create cache hashtable if not already set up */
static void _xfdashboard_image_create_cache(void)
{
	XfdashboardApplication		*application;

	/* Cache was already set up */
	if(_xfdashboard_image_cache) return;

	/* Create create hashtable */
	_xfdashboard_image_cache=g_hash_table_new(g_str_hash, g_str_equal);
	g_debug("Created image cache hashtable");

	/* Connect to "shutdown" signal of application to
	 * clean up hashtable
	 */
	application=xfdashboard_application_get_default();
	_xfdashboard_image_cache_shutdownSignalID=g_signal_connect(application, "shutdown-final", G_CALLBACK(_xfdashboard_image_destroy_cache), NULL);
}

/* Remove image from cache */
static void _xfdashboard_image_remove_from_cache(XfdashboardImage *self)
{
	XfdashboardImagePrivate		*priv;

	g_return_if_fail(XFDASHBOARD_IS_IMAGE(self));

	priv=self->priv;

	/* Cannot remove image if cache was not set up yet */
	if(!_xfdashboard_image_cache) return;

	/* Remove from cache */
	g_debug("Removing image '%s' with ref-count %d" , priv->key, G_OBJECT(self)->ref_count);
	g_hash_table_remove(_xfdashboard_image_cache, priv->key);
}

/* Store image in cache */
static void _xfdashboard_image_store_in_cache(XfdashboardImage *self, const gchar *inKey)
{
	XfdashboardImagePrivate		*priv;

	g_return_if_fail(XFDASHBOARD_IS_IMAGE(self));
	g_return_if_fail(inKey && *inKey!=0);

	priv=self->priv;

	/* Create cache hashtable */
	if(!_xfdashboard_image_cache) _xfdashboard_image_create_cache();

	/* Set key */
	if(priv->key)
	{
		g_critical(_("Image has already key '%s' set and will be replaced with '%s'"), priv->key, inKey);
		g_free(priv->key);
		priv->key=NULL;
	}
	priv->key=g_strdup(inKey);

	/* Store image in cache */
	if(g_hash_table_contains(_xfdashboard_image_cache, priv->key))
	{
		ClutterContent		*content;

		g_critical(_("An image with key '%s' is already cache and will be replaced."), priv->key);

		/* Unreference current cached image */
		content=CLUTTER_CONTENT(g_hash_table_lookup(_xfdashboard_image_cache, inKey));
		if(content)
		{
			g_object_unref(content);
			g_debug("Replacing image '%s' which had ref-count %u", priv->key, G_OBJECT(content)->ref_count);
		}
	}

	/* Store new image in cache */
	g_hash_table_insert(_xfdashboard_image_cache, priv->key, self);
	g_debug("Added image '%s' with ref-count %d" , priv->key, G_OBJECT(self)->ref_count);
}

/* IMPLEMENTATION: GObject */

/* Dispose this object */
static void _xfdashboard_image_dispose(GObject *inObject)
{
	XfdashboardImage			*self=XFDASHBOARD_IMAGE(inObject);
	XfdashboardImagePrivate		*priv=self->priv;

	/* Release allocated resources */
	if(priv->key)
	{
		_xfdashboard_image_remove_from_cache(self);
		g_free(priv->key);
		priv->key=NULL;
	}

	/* Call parent's class dispose method */
	G_OBJECT_CLASS(xfdashboard_image_parent_class)->dispose(inObject);
}

/* Set properties */
static void _xfdashboard_image_set_property(GObject *inObject,
											guint inPropID,
											const GValue *inValue,
											GParamSpec *inSpec)
{
	XfdashboardImage			*self=XFDASHBOARD_IMAGE(inObject);

	switch(inPropID)
	{
		case PROP_KEY:
			_xfdashboard_image_store_in_cache(self, g_value_get_string(inValue));
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
void xfdashboard_image_class_init(XfdashboardImageClass *klass)
{
	GObjectClass			*gobjectClass=G_OBJECT_CLASS(klass);

	/* Override functions */
	gobjectClass->dispose=_xfdashboard_image_dispose;
	gobjectClass->set_property=_xfdashboard_image_set_property;

	/* Set up private structure */
	g_type_class_add_private(klass, sizeof(XfdashboardImagePrivate));

	/* Define properties */
	XfdashboardImageProperties[PROP_KEY]=
		g_param_spec_string("key",
							_("Key"),
							_("The hash key for caching this image"),
							N_(""),
							G_PARAM_WRITABLE | G_PARAM_CONSTRUCT_ONLY);

	g_object_class_install_properties(gobjectClass, PROP_LAST, XfdashboardImageProperties);
}

/* Object initialization
 * Create private structure and set up default values
 */
void xfdashboard_image_init(XfdashboardImage *self)
{
	XfdashboardImagePrivate	*priv G_GNUC_UNUSED;

	priv=self->priv=XFDASHBOARD_IMAGE_GET_PRIVATE(self);

	/* Set up default values */
	priv->key=NULL;
}

/* Implementation: Public API */

/* Get image from cache if available */
ClutterContent* xfdashboard_image_get_cached_image(const gchar *inKey)
{
	ClutterContent		*content;

	/* If no key is given the image is not stored */
	if(!inKey || *inKey==0) return(NULL);

	/* If we have no hash table no image is cached */
	if(!_xfdashboard_image_cache) return(NULL);

	/* Lookup key in cache and return image if found */
	if(!g_hash_table_contains(_xfdashboard_image_cache, inKey)) return(NULL);

	/* Get loaded image and reference it */
	content=CLUTTER_CONTENT(g_hash_table_lookup(_xfdashboard_image_cache, inKey));
	g_object_ref(content);
	g_debug("Using cached image '%s' - ref-count is now %d" , inKey, G_OBJECT(content)->ref_count);

	return(content);
}
