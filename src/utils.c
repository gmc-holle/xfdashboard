/*
 * utils: Common functions, helpers and definitions
 * 
 * Copyright 2012 Stephan Haller <nomad@froevel.de>
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

#include "utils.h"

#include <glib/gi18n-lib.h>
#include <clutter/clutter.h>
#include <gtk/gtk.h>
#include <dbus/dbus-glib.h>

#include "image.h"
#include "stage.h"

/* Private constants */
#define FALLBACK_ICON_NAME		GTK_STOCK_MISSING_IMAGE

/* Gobject type for pointer arrays (GPtrArray) */
GType xfdashboard_pointer_array_get_type(void)
{
	static volatile gsize	type__volatile=0;
	GType					type;

	if(g_once_init_enter(&type__volatile))
	{
		type=dbus_g_type_get_collection("GPtrArray", G_TYPE_VALUE);
		g_once_init_leave(&type__volatile, type);
	}

	return(type__volatile);
}

/* Get ClutterImage object for themed icon name or absolute icon filename.
 * If icon does not exists a themed fallback icon will be returned.
 * If even the themed fallback icon cannot be found we return NULL.
 * The return ClutterImage object (if not NULL) must be unreffed with
 * g_object_unref().
 */
ClutterImage* xfdashboard_get_image_for_icon_name(const gchar *inIconName, gint inSize)
{
	ClutterContent	*image;
	GdkPixbuf		*icon;
	GtkIconTheme	*iconTheme;
	GError			*error;
	gchar			*key;

	g_return_val_if_fail(inIconName!=NULL, NULL);
	g_return_val_if_fail(inSize>0, NULL);

	image=NULL;
	icon=NULL;
	iconTheme=gtk_icon_theme_get_default();
	error=NULL;

	/* Check if we have a cache image for icon */
	key=g_strdup_printf("%s,%d", inIconName, inSize);
	image=xfdashboard_image_get_cached_image(key);
	if(image)
	{
		g_free(key);
		return(CLUTTER_IMAGE(image));
	}

	/* Load icon by name */
	if(inIconName)
	{
		/* Check if we have an absolute filename then load this file directly ... */
		if(g_path_is_absolute(inIconName) &&
			g_file_test(inIconName, G_FILE_TEST_EXISTS))
		{
			icon=gdk_pixbuf_new_from_file_at_scale(inIconName,
													inSize,
													inSize,
													TRUE,
													NULL);

			if(!icon)
			{
				g_warning(_("Could not load icon from file %s: %s"),
							inIconName, (error && error->message) ? error->message : _("unknown error"));
			}

			if(error!=NULL)
			{
				g_error_free(error);
				error=NULL;
			}
		}
			/* ... otherwise try to load the icon name directly using the icon theme */
			else
			{
				error=NULL;
				icon=gtk_icon_theme_load_icon(iconTheme,
												inIconName,
												inSize,
												GTK_ICON_LOOKUP_USE_BUILTIN,
												&error);

				if(!icon)
				{
					g_warning(_("Could not load themed icon '%s': %s"),
								inIconName, (error && error->message) ? error->message : _("unknown error"));
				}

				if(error!=NULL)
				{
					g_error_free(error);
					error=NULL;
				}
			}
	}

	/* If no icon could be loaded use fallback */
	if(!icon)
	{
		error=NULL;
		icon=gtk_icon_theme_load_icon(iconTheme,
										FALLBACK_ICON_NAME,
										inSize,
										GTK_ICON_LOOKUP_USE_BUILTIN,
										&error);

		if(!icon)
		{
			g_error(_("Could not load fallback icon for '%s': %s"),
						inIconName, (error && error->message) ? error->message : _("unknown error"));
		}

		if(error!=NULL)
		{
			g_error_free(error);
			error=NULL;
		}
	}

	/* Create ClutterImage for icon loaded and cache it */
	if(icon)
	{
		image=CLUTTER_CONTENT(g_object_new(XFDASHBOARD_TYPE_IMAGE, "key", key, NULL));
		clutter_image_set_data(CLUTTER_IMAGE(image),
								gdk_pixbuf_get_pixels(icon),
								gdk_pixbuf_get_has_alpha(icon) ? COGL_PIXEL_FORMAT_RGBA_8888 : COGL_PIXEL_FORMAT_RGB_888,
								gdk_pixbuf_get_width(icon),
								gdk_pixbuf_get_height(icon),
								gdk_pixbuf_get_rowstride(icon),
								NULL);
		g_object_unref(icon);
	}

	/* Release allocated resources */
	if(key) g_free(key);

	/* Return ClutterImage */
	return(CLUTTER_IMAGE(image));
}

/* Get ClutterImage object for GIcon object.
 * The return ClutterImage object (if not NULL) must be unreffed with
 * g_object_unref().
 */
ClutterImage* xfdashboard_get_image_for_gicon(GIcon *inIcon, gint inSize)
{
	ClutterContent		*image;
	GtkIconInfo			*iconInfo;
	GdkPixbuf			*iconPixbuf;
	GError				*error;
	gchar				*key;

	g_return_val_if_fail(G_IS_ICON(inIcon), NULL);
	g_return_val_if_fail(inSize>0, NULL);

	error=NULL;

	/* Check if we have a cache image for icon */
	key=g_strdup_printf("%s,%d", g_icon_to_string(inIcon), inSize);
	image=xfdashboard_image_get_cached_image(key);
	if(image)
	{
		g_free(key);
		return(CLUTTER_IMAGE(image));
	}

	/* Get icon information */
	iconInfo=gtk_icon_theme_lookup_by_gicon(gtk_icon_theme_get_default(),
												inIcon,
												inSize,
												GTK_ICON_LOOKUP_USE_BUILTIN);
	if(!iconInfo)
	{
		g_warning(_("Could not lookup icon for gicon '%s'"), g_icon_to_string(inIcon));
		return(NULL);
	}

	/* Load icon */
	iconPixbuf=gtk_icon_info_load_icon(iconInfo, &error);
	if(!iconPixbuf)
	{
		g_warning(_("Could not load icon for gicon '%s': %s"),
					g_icon_to_string(inIcon), (error && error->message) ? error->message : _("unknown error"));
		if(error!=NULL) g_error_free(error);
		return(NULL);
	}

	/* Create ClutterImage for icon loaded and cache it */
	image=CLUTTER_CONTENT(g_object_new(XFDASHBOARD_TYPE_IMAGE, "key", key, NULL));
	clutter_image_set_data(CLUTTER_IMAGE(image),
							gdk_pixbuf_get_pixels(iconPixbuf),
							gdk_pixbuf_get_has_alpha(iconPixbuf) ? COGL_PIXEL_FORMAT_RGBA_8888 : COGL_PIXEL_FORMAT_RGB_888,
							gdk_pixbuf_get_width(iconPixbuf),
							gdk_pixbuf_get_height(iconPixbuf),
							gdk_pixbuf_get_rowstride(iconPixbuf),
							NULL);

	/* Release allocated resources */
	if(key) g_free(key);

	/* Return ClutterImage */
	return(CLUTTER_IMAGE(image));
}

/* Get ClutterImage object for GdkPixbuf object.
 * The return ClutterImage object (if not NULL) must be unreffed with
 * g_object_unref().
 */
ClutterImage* xfdashboard_get_image_for_pixbuf(GdkPixbuf *inPixbuf)
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

/* Show a notification */
void xfdashboard_notify(ClutterActor *inSender, const gchar *inIconName, const gchar *inFormatText, ...)
{
	XfdashboardStage				*stage;
	ClutterStageManager				*stageManager;
	const GSList					*stages;
	va_list							args;
	gchar							*text;

	g_return_if_fail(inSender==NULL || CLUTTER_IS_ACTOR(inSender));

	stage=NULL;

	/* Get stage of sending actor if available */
	if(inSender) stage=XFDASHBOARD_STAGE(clutter_actor_get_stage(inSender));

	/* No sending actor specified or no stage found so get default stage */
	if(!stage)
	{
		stageManager=clutter_stage_manager_get_default();
		stage=XFDASHBOARD_STAGE(clutter_stage_manager_get_default_stage(stageManager));

		/* If we did not get default stage use first stage from manager */
		if(!stage)
		{
			stages=clutter_stage_manager_peek_stages(stageManager);
			stage=XFDASHBOARD_STAGE(stages->data);
		}
	}

	/* Build text to display */
	va_start(args, inFormatText);
	text=g_strdup_vprintf(inFormatText, args);
	va_end(args);

	/* Show notification on stage */
	xfdashboard_stage_show_notification(stage, inIconName, text);

	/* Release allocated resources */
	g_free(text);
}
