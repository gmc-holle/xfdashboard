/*
 * image: A synchronous loaded and cached image content
 * 
 * Copyright 2012-2014 Stephan Haller <nomad@froevel.de>
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
#include "application.h"

#include <glib/gi18n-lib.h>
#include <gtk/gtk.h>

/* Define this class in GObject system */
G_DEFINE_TYPE(XfdashboardImage,
				xfdashboard_image,
				CLUTTER_TYPE_IMAGE)

/* Local definitions */
typedef enum
{
	IMAGE_TYPE_NONE=0,
	IMAGE_TYPE_FILE,
	IMAGE_TYPE_ICON_NAME,
	IMAGE_TYPE_GICON,
} ImageType;

/* Private structure - access only by public API if needed */
#define XFDASHBOARD_IMAGE_GET_PRIVATE(obj) \
	(G_TYPE_INSTANCE_GET_PRIVATE((obj), XFDASHBOARD_TYPE_IMAGE, XfdashboardImagePrivate))

struct _XfdashboardImagePrivate
{
	/* Properties related */
	gchar			*key;
	gchar			*iconName;
	GIcon			*gicon;
	guint			iconSize;

	/* Instance related */
	ImageType		type;
	gboolean		isLoaded;
	GtkIconTheme	*iconTheme;

	guint			contentAttachedSignalID;
	guint			iconThemeChangedSignalID;
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

#define XFDASHBOARD_IMAGE_FALLBACK_ICON_NAME		GTK_STOCK_MISSING_IMAGE

/* Get image from cache if available */
static ClutterImage* _xfdashboard_image_get_cached_image(const gchar *inKey)
{
	ClutterImage		*image;

	/* If no key is given the image is not stored */
	if(!inKey || *inKey==0) return(NULL);

	/* If we have no hash table no image is cached */
	if(!_xfdashboard_image_cache) return(NULL);

	/* Lookup key in cache and return image if found */
	if(!g_hash_table_contains(_xfdashboard_image_cache, inKey)) return(NULL);

	/* Get loaded image and reference it */
	image=CLUTTER_IMAGE(g_hash_table_lookup(_xfdashboard_image_cache, inKey));
	g_object_ref(image);
	g_debug("Using cached image '%s' - ref-count is now %d" , inKey, G_OBJECT(image)->ref_count);

	return(image);
}

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

/* Load image from file */
static void _xfdashboard_image_load_from_file(XfdashboardImage *self)
{
	XfdashboardImagePrivate		*priv;
	GdkPixbuf					*iconPixbuf;
	GError						*error;

	g_return_if_fail(XFDASHBOARD_IS_IMAGE(self));

	priv=self->priv;
	iconPixbuf=NULL;
	error=NULL;

	/* Check if type of image is valid and all needed parameters are set */
	g_return_if_fail(priv->type==IMAGE_TYPE_FILE);
	g_return_if_fail(priv->iconName);
	g_return_if_fail(priv->iconSize>0);

	/* If file exists then load this file and setup pixbuf */
	if(g_file_test(priv->iconName, G_FILE_TEST_EXISTS))
	{
		iconPixbuf=gdk_pixbuf_new_from_file_at_scale(priv->iconName,
														priv->iconSize,
														priv->iconSize,
														TRUE,
														NULL);

		if(!iconPixbuf)
		{
			g_warning(_("Could not load icon from file %s: %s"),
						priv->iconName, (error && error->message) ? error->message : _("unknown error"));
		}

		if(error!=NULL)
		{
			g_error_free(error);
			error=NULL;
		}
	}

	/* If no icon pixbuf is available we have to load fallback icon */
	if(!iconPixbuf)
	{
		error=NULL;
		iconPixbuf=gtk_icon_theme_load_icon(priv->iconTheme,
												XFDASHBOARD_IMAGE_FALLBACK_ICON_NAME,
												priv->iconSize,
												GTK_ICON_LOOKUP_USE_BUILTIN,
												&error);

		if(!iconPixbuf)
		{
			g_error(_("Could not load fallback icon for '%s': %s"),
						priv->iconName, (error && error->message) ? error->message : _("unknown error"));
		}

		if(error!=NULL)
		{
			g_error_free(error);
			error=NULL;
		}
	}

	/* Create ClutterImage for icon loaded and cache it */
	if(iconPixbuf)
	{
		clutter_image_set_data(CLUTTER_IMAGE(self),
								gdk_pixbuf_get_pixels(iconPixbuf),
								gdk_pixbuf_get_has_alpha(iconPixbuf) ? COGL_PIXEL_FORMAT_RGBA_8888 : COGL_PIXEL_FORMAT_RGB_888,
								gdk_pixbuf_get_width(iconPixbuf),
								gdk_pixbuf_get_height(iconPixbuf),
								gdk_pixbuf_get_rowstride(iconPixbuf),
								NULL);
		g_object_unref(iconPixbuf);
	}

	g_debug("Loaded image with key '%s' from icon name '%s' at size %u", priv->key, priv->iconName, priv->iconSize);
}

/* Load image from icon theme */
static void _xfdashboard_image_load_from_icon_name(XfdashboardImage *self)
{
	XfdashboardImagePrivate		*priv;
	GdkPixbuf					*iconPixbuf;
	GError						*error;

	g_return_if_fail(XFDASHBOARD_IS_IMAGE(self));

	priv=self->priv;
	iconPixbuf=NULL;
	error=NULL;

	/* Check if type of image is valid and all needed parameters are set */
	g_return_if_fail(priv->type==IMAGE_TYPE_ICON_NAME);
	g_return_if_fail(priv->iconName);
	g_return_if_fail(priv->iconSize>0);

	/* Try to load the icon name directly using the icon theme */
	iconPixbuf=gtk_icon_theme_load_icon(priv->iconTheme,
											priv->iconName,
											priv->iconSize,
											GTK_ICON_LOOKUP_USE_BUILTIN,
											&error);

	if(!iconPixbuf)
	{
		g_warning(_("Could not load themed icon '%s': %s"),
					priv->iconName, (error && error->message) ? error->message : _("unknown error"));
	}

	if(error!=NULL)
	{
		g_error_free(error);
		error=NULL;
	}

	/* If no icon pixbuf is available we have to load fallback icon */
	if(!iconPixbuf)
	{
		error=NULL;
		iconPixbuf=gtk_icon_theme_load_icon(priv->iconTheme,
												XFDASHBOARD_IMAGE_FALLBACK_ICON_NAME,
												priv->iconSize,
												GTK_ICON_LOOKUP_USE_BUILTIN,
												&error);

		if(!iconPixbuf)
		{
			g_error(_("Could not load fallback icon for '%s': %s"),
						priv->iconName, (error && error->message) ? error->message : _("unknown error"));
		}

		if(error!=NULL)
		{
			g_error_free(error);
			error=NULL;
		}
	}

	/* Create ClutterImage for icon loaded and cache it */
	if(iconPixbuf)
	{
		clutter_image_set_data(CLUTTER_IMAGE(self),
								gdk_pixbuf_get_pixels(iconPixbuf),
								gdk_pixbuf_get_has_alpha(iconPixbuf) ? COGL_PIXEL_FORMAT_RGBA_8888 : COGL_PIXEL_FORMAT_RGB_888,
								gdk_pixbuf_get_width(iconPixbuf),
								gdk_pixbuf_get_height(iconPixbuf),
								gdk_pixbuf_get_rowstride(iconPixbuf),
								NULL);
		g_object_unref(iconPixbuf);
	}

	g_debug("Loaded image with key '%s' from icon name '%s' at size %u", priv->key, priv->iconName, priv->iconSize);
}

/* Load image from GIcon */
static void _xfdashboard_image_load_from_gicon(XfdashboardImage *self)
{
	XfdashboardImagePrivate		*priv;
	GtkIconInfo					*iconInfo;
	GdkPixbuf					*iconPixbuf;
	GError						*error;

	g_return_if_fail(XFDASHBOARD_IS_IMAGE(self));

	priv=self->priv;
	iconInfo=NULL;
	iconPixbuf=NULL;
	error=NULL;

	/* Check if type of image is valid and all needed parameters are set */
	g_return_if_fail(priv->type==IMAGE_TYPE_GICON);
	g_return_if_fail(priv->gicon);
	g_return_if_fail(priv->iconSize>0);

	/* Get icon information */
	iconInfo=gtk_icon_theme_lookup_by_gicon(priv->iconTheme,
												priv->gicon,
												priv->iconSize,
												GTK_ICON_LOOKUP_USE_BUILTIN);
	if(iconInfo)
	{
		/* Load icon */
		iconPixbuf=gtk_icon_info_load_icon(iconInfo, &error);
		if(!iconPixbuf)
		{
			g_warning(_("Could not load icon for gicon '%s': %s"),
						g_icon_to_string(priv->gicon), (error && error->message) ? error->message : _("unknown error"));

			if(error!=NULL)
			{
				g_error_free(error);
				error=NULL;
			}
		}
	}
		else g_warning(_("Could not lookup icon for gicon '%s'"), g_icon_to_string(priv->gicon));

	/* If no icon could be loaded use fallback */
	if(!iconPixbuf)
	{
		error=NULL;
		iconPixbuf=gtk_icon_theme_load_icon(priv->iconTheme,
												XFDASHBOARD_IMAGE_FALLBACK_ICON_NAME,
												priv->iconSize,
												GTK_ICON_LOOKUP_USE_BUILTIN,
												&error);

		if(!iconPixbuf)
		{
			g_error(_("Could not load fallback gicon for '%s': %s"),
						g_icon_to_string(priv->gicon), (error && error->message) ? error->message : _("unknown error"));
		}

		if(error!=NULL)
		{
			g_error_free(error);
			error=NULL;
		}
	}

	/* Create ClutterImage for icon loaded and cache it */
	if(iconPixbuf)
	{
		clutter_image_set_data(CLUTTER_IMAGE(self),
								gdk_pixbuf_get_pixels(iconPixbuf),
								gdk_pixbuf_get_has_alpha(iconPixbuf) ? COGL_PIXEL_FORMAT_RGBA_8888 : COGL_PIXEL_FORMAT_RGB_888,
								gdk_pixbuf_get_width(iconPixbuf),
								gdk_pixbuf_get_height(iconPixbuf),
								gdk_pixbuf_get_rowstride(iconPixbuf),
								NULL);
		g_object_unref(iconPixbuf);
	}

	/* Release allocated resources */
	if(iconInfo) gtk_icon_info_free(iconInfo);

	g_debug("Loaded image with key '%s' from gicon '%s' at size %u", priv->key, g_icon_to_string(priv->gicon), priv->iconSize);
}

/* Icon theme has changed */
static void _xfdashboard_image_on_icon_theme_changed(XfdashboardImage *self,
														gpointer inUserData)
{
	XfdashboardImagePrivate		*priv;

	g_return_if_fail(XFDASHBOARD_IS_IMAGE(self));

	priv=self->priv;

	/* If icon has not been loaded yet then there is no need to do it now */
	if(!priv->isLoaded) return;

	/* Reload image */
	switch(priv->type)
	{
		case IMAGE_TYPE_NONE:
			g_warning(_("Cannot load image '%s' without type"), priv->key);
			break;

		case IMAGE_TYPE_FILE:
			_xfdashboard_image_load_from_file(self);
			break;

		case IMAGE_TYPE_ICON_NAME:
			_xfdashboard_image_load_from_icon_name(self);
			break;

		case IMAGE_TYPE_GICON:
			_xfdashboard_image_load_from_gicon(self);
			break;

		default:
			g_warning(_("Cannot load image '%s' of unknown type %d"), priv->key, priv->type);
			break;
	}
}

/* Setup image for loading icon from icon theme by name or absolute file name */
static void _xfdashboard_image_setup_for_icon(XfdashboardImage *self,
												const gchar *inIconName,
												guint inSize)
{
	XfdashboardImagePrivate		*priv;

	g_return_if_fail(XFDASHBOARD_IS_IMAGE(self));
	g_return_if_fail(inIconName && *inIconName);
	g_return_if_fail(inSize>0);

	priv=self->priv;

	/* Image must not be setup already */
	g_return_if_fail(priv->type==IMAGE_TYPE_NONE);

	/* Determine type of image to load icon from theme or file */
	if(g_path_is_absolute(inIconName)) priv->type=IMAGE_TYPE_FILE;
		else priv->type=IMAGE_TYPE_ICON_NAME;

	/* Set up image */
	priv->iconName=g_strdup(inIconName);
	priv->iconSize=inSize;
}

/* Setup image for loading icon from GIcon */
static void _xfdashboard_image_setup_for_gicon(XfdashboardImage *self,
												GIcon *inIcon,
												guint inSize)
{
	XfdashboardImagePrivate		*priv;

	g_return_if_fail(XFDASHBOARD_IS_IMAGE(self));
	g_return_if_fail(G_IS_ICON(inIcon));
	g_return_if_fail(inSize>0);

	priv=self->priv;

	/* Image must not be setup already */
	g_return_if_fail(priv->type==IMAGE_TYPE_NONE);

	/* Set up image */
	priv->type=IMAGE_TYPE_GICON;
	priv->gicon=G_ICON(g_object_ref(inIcon));
	priv->iconSize=inSize;
}

/* IMPLEMENTATION: ClutterContent */

/* Image was attached to an actor */
static void _xfdashboard_image_on_attached(ClutterContent *inContent,
											ClutterActor *inActor,
											gpointer inUserData)
{
	XfdashboardImage			*self;
	XfdashboardImagePrivate		*priv;

	g_return_if_fail(XFDASHBOARD_IS_IMAGE(inContent));

	self=XFDASHBOARD_IMAGE(inContent);
	priv=self->priv;

	/* Check if image was already loaded */
	if(priv->isLoaded) return;

	/* Mark image loaded regardless if loading will succeed or fail. */
	priv->isLoaded=TRUE;

	/* Also disconnect signal handler as it should not be called anymore. */
	g_signal_handler_disconnect(self, priv->contentAttachedSignalID);
	priv->contentAttachedSignalID=0;

	/* Load icon */
	switch(priv->type)
	{
		case IMAGE_TYPE_NONE:
			g_warning(_("Cannot load image '%s' without type"), priv->key);
			break;

		case IMAGE_TYPE_FILE:
			_xfdashboard_image_load_from_file(self);
			break;

		case IMAGE_TYPE_ICON_NAME:
			_xfdashboard_image_load_from_icon_name(self);
			break;

		case IMAGE_TYPE_GICON:
			_xfdashboard_image_load_from_gicon(self);
			break;

		default:
			g_warning(_("Cannot load image '%s' of unknown type %d"), priv->key, priv->type);
			break;
	}
}

/* IMPLEMENTATION: GObject */

/* Dispose this object */
static void _xfdashboard_image_dispose(GObject *inObject)
{
	XfdashboardImage			*self=XFDASHBOARD_IMAGE(inObject);
	XfdashboardImagePrivate		*priv=self->priv;

	/* Release allocated resources */
	priv->type=IMAGE_TYPE_NONE;

	if(priv->contentAttachedSignalID)
	{
		g_signal_handler_disconnect(self, priv->contentAttachedSignalID);
		priv->contentAttachedSignalID=0;
	}

	if(priv->iconThemeChangedSignalID)
	{
		g_signal_handler_disconnect(priv->iconTheme, priv->iconThemeChangedSignalID);
		priv->iconThemeChangedSignalID=0;
	}

	if(priv->key)
	{
		_xfdashboard_image_remove_from_cache(self);
		g_free(priv->key);
		priv->key=NULL;
	}

	if(priv->iconName)
	{
		g_free(priv->iconName);
		priv->iconName=NULL;
	}

	if(priv->gicon)
	{
		g_object_unref(priv->gicon);
		priv->gicon=NULL;
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
							G_PARAM_WRITABLE | G_PARAM_STATIC_STRINGS | G_PARAM_CONSTRUCT_ONLY);

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
	priv->type=IMAGE_TYPE_NONE;
	priv->iconName=NULL;
	priv->gicon=NULL;
	priv->iconSize=0;
	priv->isLoaded=FALSE;
	priv->iconTheme=gtk_icon_theme_get_default();

	/* Connect to "attached" signal of ClutterContent to get notified
	 * when this image is used. We will load image when this image is
	 * attached the first time.
	 */
	priv->contentAttachedSignalID=g_signal_connect(self,
													"attached",
													G_CALLBACK(_xfdashboard_image_on_attached),
													NULL);

	/* Connect to "changed" signal of GtkIconTheme to get notified
	 * when icon theme has changed to reload loaded images.
	 */
	priv->iconThemeChangedSignalID=g_signal_connect_swapped(priv->iconTheme,
															"changed",
															G_CALLBACK(_xfdashboard_image_on_icon_theme_changed),
															self);
}

/* IMPLEMENTATION: Public API */

/* Create new instance or use cached one for themed icon name or absolute icon filename.
 * If icon does not exists a themed fallback icon will be returned.
 * If even the themed fallback icon cannot be found we return NULL.
 * The returned ClutterImage object (if not NULL) must be unreffed with
 * g_object_unref().
 */
ClutterImage* xfdashboard_image_new_for_icon_name(const gchar *inIconName, gint inSize)
{
	ClutterImage		*image;
	gchar				*key;

	g_return_val_if_fail(inIconName!=NULL, NULL);
	g_return_val_if_fail(inSize>0, NULL);

	/* Check if we have a cache image for icon otherwise create image instance */
	key=g_strdup_printf("%s,%d", inIconName, inSize);
	if(!key)
	{
		g_warning(_("Could not create key for icon name '%s' at size %u"), inIconName, inSize);
		return(NULL);
	}

	image=_xfdashboard_image_get_cached_image(key);
	if(!image)
	{
		image=CLUTTER_IMAGE(g_object_new(XFDASHBOARD_TYPE_IMAGE,
											"key", key,
											NULL));
		_xfdashboard_image_setup_for_icon(XFDASHBOARD_IMAGE(image), inIconName, inSize);
	}

	g_free(key);

	/* Return ClutterImage */
	return(image);
}

/* Create new instance or use cached one for GIcon object.
 * If icon does not exists a themed fallback icon will be returned.
 * If even the themed fallback icon cannot be found we return NULL.
 * The return ClutterImage object (if not NULL) must be unreffed with
 * g_object_unref().
 */
ClutterImage* xfdashboard_image_new_for_gicon(GIcon *inIcon, gint inSize)
{
	ClutterImage		*image;
	gchar				*key;

	g_return_val_if_fail(G_IS_ICON(inIcon), NULL);
	g_return_val_if_fail(inSize>0, NULL);

	/* Check if we have a cache image for icon otherwise create image instance */
	key=g_strdup_printf("%s,%d", g_icon_to_string(inIcon), inSize);
	if(!key)
	{
		g_warning(_("Could not create key for gicon '%s' at size %u"), g_icon_to_string(inIcon), inSize);
		return(NULL);
	}

	image=_xfdashboard_image_get_cached_image(key);
	if(!image)
	{
		image=CLUTTER_IMAGE(g_object_new(XFDASHBOARD_TYPE_IMAGE, 
											"key", key,
											NULL));
		_xfdashboard_image_setup_for_gicon(XFDASHBOARD_IMAGE(image), inIcon, inSize);
	}

	g_free(key);

	/* Return ClutterImage */
	return(image);
}

/* Create a new instance for GdkPixbuf object.
 * An image of GdkPixbuf will never be cached as the pixbuf at given
 * pointer may change and we do not get notified. The pointer may also
 * be re-used for a completely new and difference instance of GdkPixbuf.
 * The return ClutterImage object (if not NULL) must be unreffed with
 * g_object_unref().
 */
ClutterImage* xfdashboard_image_new_for_pixbuf(GdkPixbuf *inPixbuf)
{
	ClutterContent		*image;

	g_return_val_if_fail(GDK_IS_PIXBUF(inPixbuf), NULL);

	image=NULL;

	/* Create ClutterImage for pixbuf directly because images
	 * from GdkPixbuf will not be cached
	 */
	image=clutter_image_new();
	clutter_image_set_data(CLUTTER_IMAGE(image),
							gdk_pixbuf_get_pixels(inPixbuf),
							gdk_pixbuf_get_has_alpha(inPixbuf) ? COGL_PIXEL_FORMAT_RGBA_8888 : COGL_PIXEL_FORMAT_RGB_888,
							gdk_pixbuf_get_width(inPixbuf),
							gdk_pixbuf_get_height(inPixbuf),
							gdk_pixbuf_get_rowstride(inPixbuf),
							NULL);

	/* Return ClutterImage */
	return(CLUTTER_IMAGE(image));
}
