/*
 * image-content: An asynchronous loaded and cached image content
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

#ifndef __XFDASHBOARD_IMAGE_CONTENT__
#define __XFDASHBOARD_IMAGE_CONTENT__

#include <clutter/clutter.h>
#include <gdk/gdk.h>

G_BEGIN_DECLS

#define XFDASHBOARD_TYPE_IMAGE_CONTENT				(xfdashboard_image_content_get_type())
#define XFDASHBOARD_IMAGE_CONTENT(obj)				(G_TYPE_CHECK_INSTANCE_CAST((obj), XFDASHBOARD_TYPE_IMAGE_CONTENT, XfdashboardImageContent))
#define XFDASHBOARD_IS_IMAGE_CONTENT(obj)			(G_TYPE_CHECK_INSTANCE_TYPE((obj), XFDASHBOARD_TYPE_IMAGE_CONTENT))
#define XFDASHBOARD_IMAGE_CONTENT_CLASS(klass)		(G_TYPE_CHECK_CLASS_CAST((klass), XFDASHBOARD_TYPE_IMAGE_CONTENT, XfdashboardImageContentClass))
#define XFDASHBOARD_IS_IMAGE_CONTENT_CLASS(klass)	(G_TYPE_CHECK_CLASS_TYPE((klass), XFDASHBOARD_TYPE_IMAGE_CONTENT))
#define XFDASHBOARD_IMAGE_CONTENT_GET_CLASS(obj)	(G_TYPE_INSTANCE_GET_CLASS((obj), XFDASHBOARD_TYPE_IMAGE_CONTENT, XfdashboardImageContentClass))

typedef struct _XfdashboardImageContent				XfdashboardImageContent;
typedef struct _XfdashboardImageContentClass		XfdashboardImageContentClass;
typedef struct _XfdashboardImageContentPrivate		XfdashboardImageContentPrivate;

struct _XfdashboardImageContent
{
	/* Parent instance */
	ClutterImage						parent_instance;

	/* Private structure */
	XfdashboardImageContentPrivate		*priv;
};

struct _XfdashboardImageContentClass
{
	/*< private >*/
	/* Parent class */
	ClutterImageClass					parent_class;

	/*< public >*/
	/* Virtual functions */
	void (*loaded)(XfdashboardImageContent *self);
	void (*loading_failed)(XfdashboardImageContent *self);
};

/* Public API */
GType xfdashboard_image_content_get_type(void) G_GNUC_CONST;

ClutterImage* xfdashboard_image_content_new_for_icon_name(const gchar *inIconName, gint inSize);
ClutterImage* xfdashboard_image_content_new_for_gicon(GIcon *inIcon, gint inSize);
ClutterImage* xfdashboard_image_content_new_for_pixbuf(GdkPixbuf *inPixbuf);

gint xfdashboard_image_content_get_size(XfdashboardImageContent *self);
void xfdashboard_image_content_get_real_size(XfdashboardImageContent *self, gint *outWidth, gint *outHeight);

G_END_DECLS

#endif	/* __XFDASHBOARD_IMAGE_CONTENT__ */
