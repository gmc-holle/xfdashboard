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

#ifndef __XFDASHBOARD_IMAGE__
#define __XFDASHBOARD_IMAGE__

#include <clutter/clutter.h>
#include <gdk/gdk.h>

G_BEGIN_DECLS

#define XFDASHBOARD_TYPE_IMAGE				(xfdashboard_image_get_type())
#define XFDASHBOARD_IMAGE(obj)				(G_TYPE_CHECK_INSTANCE_CAST((obj), XFDASHBOARD_TYPE_IMAGE, XfdashboardImage))
#define XFDASHBOARD_IS_IMAGE(obj)			(G_TYPE_CHECK_INSTANCE_TYPE((obj), XFDASHBOARD_TYPE_IMAGE))
#define XFDASHBOARD_IMAGE_CLASS(klass)		(G_TYPE_CHECK_CLASS_CAST((klass), XFDASHBOARD_TYPE_IMAGE, XfdashboardImageClass))
#define XFDASHBOARD_IS_IMAGE_CLASS(klass)	(G_TYPE_CHECK_CLASS_TYPE((klass), XFDASHBOARD_TYPE_IMAGE))
#define XFDASHBOARD_IMAGE_GET_CLASS(obj)	(G_TYPE_INSTANCE_GET_CLASS((obj), XFDASHBOARD_TYPE_IMAGE, XfdashboardImageClass))

typedef struct _XfdashboardImage			XfdashboardImage;
typedef struct _XfdashboardImageClass		XfdashboardImageClass;
typedef struct _XfdashboardImagePrivate		XfdashboardImagePrivate;

struct _XfdashboardImage
{
	/* Parent instance */
	ClutterImage				parent_instance;

	/* Private structure */
	XfdashboardImagePrivate		*priv;
};

struct _XfdashboardImageClass
{
	/*< private >*/
	/* Parent class */
	ClutterImageClass			parent_class;

	/*< public >*/
	/* Virtual functions */
	void (*loaded)(XfdashboardImage *self);
	void (*failed)(XfdashboardImage *self);
};

/* Public API */
GType xfdashboard_image_get_type(void) G_GNUC_CONST;

ClutterImage* xfdashboard_image_new_for_icon_name(const gchar *inIconName, gint inSize);
ClutterImage* xfdashboard_image_new_for_gicon(GIcon *inIcon, gint inSize);
ClutterImage* xfdashboard_image_new_for_pixbuf(GdkPixbuf *inPixbuf);

G_END_DECLS

#endif	/* __XFDASHBOARD_IMAGE__ */
