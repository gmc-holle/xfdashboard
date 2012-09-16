/*
 * application-icon.h: An actor representing an application
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

#ifndef __XFOVERVIEW_APPLICATION_ICON__
#define __XFOVERVIEW_APPLICATION_ICON__

#include <clutter/clutter.h>
#include <clutter/x11/clutter-x11.h>
#include <gio/gdesktopappinfo.h>
#include <garcon/garcon.h>

G_BEGIN_DECLS

#define XFDASHBOARD_TYPE_APPLICATION_ICON				(xfdashboard_application_icon_get_type())
#define XFDASHBOARD_APPLICATION_ICON(obj)				(G_TYPE_CHECK_INSTANCE_CAST((obj), XFDASHBOARD_TYPE_APPLICATION_ICON, XfdashboardApplicationIcon))
#define XFDASHBOARD_IS_APPLICATION_ICON(obj)			(G_TYPE_CHECK_INSTANCE_TYPE((obj), XFDASHBOARD_TYPE_APPLICATION_ICON))
#define XFDASHBOARD_APPLICATION_ICON_CLASS(klass)		(G_TYPE_CHECK_CLASS_CAST((klass), XFDASHBOARD_TYPE_APPLICATION_ICON, XfdashboardApplicationIconClass))
#define XFDASHBOARD_IS_APPLICATION_ICON_CLASS(klass)	(G_TYPE_CHECK_CLASS_TYPE((klass), XFDASHBOARD_TYPE_APPLICATION_ICON))
#define XFDASHBOARD_APPLICATION_ICON_GET_CLASS(obj)		G_TYPE_INSTANCE_GET_CLASS((obj), XFDASHBOARD_TYPE_APPLICATION_ICON, XfdashboardApplicationIconClass))

typedef struct _XfdashboardApplicationIcon				XfdashboardApplicationIcon;
typedef struct _XfdashboardApplicationIconClass			XfdashboardApplicationIconClass;
typedef struct _XfdashboardApplicationIconPrivate		XfdashboardApplicationIconPrivate;

struct _XfdashboardApplicationIcon
{
	/* Parent instance */
	ClutterActor						parent_instance;

	/* Private structure */
	XfdashboardApplicationIconPrivate	*priv;
};

struct _XfdashboardApplicationIconClass
{
	/* Parent class */
	ClutterActorClass					parent_class;

	/* Virtual functions */
	void (*clicked)(XfdashboardApplicationIcon *self);
};

/* Public API */
GType xfdashboard_application_icon_get_type(void) G_GNUC_CONST;

ClutterActor* xfdashboard_application_icon_new(void);
ClutterActor* xfdashboard_application_icon_new_by_desktop_file(const gchar *inDesktopFile);
ClutterActor* xfdashboard_application_icon_new_by_menu_item(const GarconMenuElement *inMenuElement);
ClutterActor* xfdashboard_application_icon_new_with_custom(const GarconMenuElement *inMenuElement,
															const gchar *inIconName,
															const gchar *inTitle,
															const gchar *inDescription);

const gchar* xfdashboard_application_icon_get_desktop_file(XfdashboardApplicationIcon *self);
void xfdashboard_application_icon_set_desktop_file(XfdashboardApplicationIcon *self, const gchar *inDesktopFile);

const GarconMenuElement* xfdashboard_application_icon_get_menu_element(XfdashboardApplicationIcon *self);
void xfdashboard_application_icon_set_menu_element(XfdashboardApplicationIcon *self, GarconMenuElement *inMenuElement);

const GAppInfo* xfdashboard_application_icon_get_application_info(XfdashboardApplicationIcon *self);

const gboolean xfdashboard_application_icon_get_label_visible(XfdashboardApplicationIcon *self);
void xfdashboard_application_icon_set_label_visible(XfdashboardApplicationIcon *self, const gboolean inVisible);

const gchar* xfdashboard_application_icon_get_label_font(XfdashboardApplicationIcon *self);
void xfdashboard_application_icon_set_label_font(XfdashboardApplicationIcon *self, const gchar *inFont);

const ClutterColor* xfdashboard_application_icon_get_label_color(XfdashboardApplicationIcon *self);
void xfdashboard_application_icon_set_label_color(XfdashboardApplicationIcon *self, const ClutterColor *inColor);

const ClutterColor* xfdashboard_application_icon_get_label_background_color(XfdashboardApplicationIcon *self);
void xfdashboard_application_icon_set_label_background_color(XfdashboardApplicationIcon *self, const ClutterColor *inColor);

const gfloat xfdashboard_application_icon_get_label_margin(XfdashboardApplicationIcon *self);
void xfdashboard_application_icon_set_label_margin(XfdashboardApplicationIcon *self, const gfloat inMargin);

const PangoEllipsizeMode xfdashboard_application_icon_get_label_ellipsize_mode(XfdashboardApplicationIcon *self);
void xfdashboard_application_icon_set_label_ellipsize_mode(XfdashboardApplicationIcon *self, const PangoEllipsizeMode inMode);

G_END_DECLS

#endif
