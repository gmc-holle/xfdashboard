/*
 * common.c: Common function and definitions
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

#include "common.h"

#include <clutter/clutter.h>
#include <clutter/x11/clutter-x11.h>
#include <gdk/gdk.h>
#include <gtk/gtk.h>

/* Private or external variables and methods */
extern ClutterActor				*stage;

/* Private constants */
#define FALLBACK_ICON_NAME		GTK_STOCK_MISSING_IMAGE

/* Get window of application */
WnckWindow* xfdashboard_get_stage_window()
{
	static WnckWindow		*stageWindow=NULL;

	if(!stageWindow)
	{
		Window		xWindow;

		xWindow=clutter_x11_get_stage_window(CLUTTER_STAGE(stage));
		stageWindow=wnck_window_get(xWindow);
	}

	return(stageWindow);
}

/* Get root application menu */
GarconMenu* xfdashboard_get_application_menu()
{
	static GarconMenu		*menu=NULL;

	/* If it is the first time (or if it failed previously)
	 * load the menus now
	 */
	if(!menu)
	{
		GError				*error=NULL;
		
		/* Try to get the root menu */
		menu=garcon_menu_new_applications();

		if(G_UNLIKELY(!garcon_menu_load(menu, NULL, &error)))
		{
			gchar *uri;

			uri=g_file_get_uri(garcon_menu_get_file (menu));
			g_error("Could not load menu from %s: %s", uri, error->message);
			g_free(uri);

			g_error_free(error);

			g_object_unref(menu);
			menu=NULL;
		}
	}

	return(menu);
}

/* Get GdkPixbuf object for themed icon name or absolute icon filename.
 * If icon does not exists a themed fallback icon will be returned.
 * If even the themed fallback icon cannot be found we return NULL.
 * The return GdkPixbuf object (if not NULL) must be unreffed with
 * g_object_unref().
 */
GdkPixbuf* xfdashboard_get_pixbuf_for_icon_name(const gchar *inIconName, gint inSize)
{
	GdkPixbuf		*icon=NULL;
	GtkIconTheme	*iconTheme=gtk_icon_theme_get_default();
	GError			*error=NULL;

	if(inIconName)
	{
		/* Check if we have an absolute filename */
		if(g_path_is_absolute(inIconName) &&
			g_file_test(inIconName, G_FILE_TEST_EXISTS))
		{
			error=NULL;
			icon=gdk_pixbuf_new_from_file_at_scale(inIconName,
													inSize,
													inSize,
													TRUE,
													NULL);

			if(!icon) g_warning("Could not load icon from file %s: %s",
								inIconName, (error && error->message) ?  error->message : "unknown error");

			if(error!=NULL) g_error_free(error);
		}
			else
			{
				/* Try to load the icon name directly using the icon theme */
				error=NULL;
				icon=gtk_icon_theme_load_icon(iconTheme,
												inIconName,
												inSize,
												GTK_ICON_LOOKUP_USE_BUILTIN,
												&error);

				if(!icon) g_warning("Could not load themed icon '%s': %s",
									inIconName, (error && error->message) ?  error->message : "unknown error");

				if(error!=NULL) g_error_free(error);
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

		if(!icon) g_error("Could not load fallback icon for '%s': %s",
							inIconName,
							(error && error->message) ?  error->message : "unknown error");

		if(error!=NULL) g_error_free(error);
	}

	/* Return icon pixbuf */
	return(icon);
}

/* Get scaled GdkPixbuf object for themed icon name or absolute icon filename.
 * See _xfdashboard_get_pixbuf_for_icon_name for more details
 */
GdkPixbuf* xfdashboard_get_pixbuf_for_icon_name_scaled(const gchar *inIconName, gint inSize)
{
	/* Get icon pixbuf */
	GdkPixbuf						*unscaledIcon, *scaledIcon;

	unscaledIcon=xfdashboard_get_pixbuf_for_icon_name(inIconName, inSize);
	if(!unscaledIcon ||
		(gdk_pixbuf_get_width(unscaledIcon)==inSize &&
			gdk_pixbuf_get_height(unscaledIcon)==inSize))
	{
		return(unscaledIcon);
	}
	
	/* Scale icon */
	scaledIcon=gdk_pixbuf_scale_simple(unscaledIcon, inSize, inSize, GDK_INTERP_BILINEAR);
	g_object_unref(unscaledIcon);

	return(scaledIcon);
}
