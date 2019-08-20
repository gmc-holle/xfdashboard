/*
 * application-button: A button representing an application
 *                     (either by menu item or desktop file)
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

#ifndef __LIBXFDASHBOARD_APPLICATION_BUTTON__
#define __LIBXFDASHBOARD_APPLICATION_BUTTON__

#if !defined(__LIBXFDASHBOARD_H_INSIDE__) && !defined(LIBXFDASHBOARD_COMPILATION)
#error "Only <libxfdashboard/libxfdashboard.h> can be included directly."
#endif

#include <garcon/garcon.h>

#include <libxfdashboard/button.h>
#include <libxfdashboard/desktop-app-info.h>
#include <libxfdashboard/popup-menu.h>

G_BEGIN_DECLS

#define XFDASHBOARD_TYPE_APPLICATION_BUTTON				(xfdashboard_application_button_get_type())
#define XFDASHBOARD_APPLICATION_BUTTON(obj)				(G_TYPE_CHECK_INSTANCE_CAST((obj), XFDASHBOARD_TYPE_APPLICATION_BUTTON, XfdashboardApplicationButton))
#define XFDASHBOARD_IS_APPLICATION_BUTTON(obj)			(G_TYPE_CHECK_INSTANCE_TYPE((obj), XFDASHBOARD_TYPE_APPLICATION_BUTTON))
#define XFDASHBOARD_APPLICATION_BUTTON_CLASS(klass)		(G_TYPE_CHECK_CLASS_CAST((klass), XFDASHBOARD_TYPE_APPLICATION_BUTTON, XfdashboardApplicationButtonClass))
#define XFDASHBOARD_IS_APPLICATION_BUTTON_CLASS(klass)	(G_TYPE_CHECK_CLASS_TYPE((klass), XFDASHBOARD_TYPE_APPLICATION_BUTTON))
#define XFDASHBOARD_APPLICATION_BUTTON_GET_CLASS(obj)	(G_TYPE_INSTANCE_GET_CLASS((obj), XFDASHBOARD_TYPE_APPLICATION_BUTTON, XfdashboardApplicationButtonClass))

typedef struct _XfdashboardApplicationButton			XfdashboardApplicationButton;
typedef struct _XfdashboardApplicationButtonClass		XfdashboardApplicationButtonClass;
typedef struct _XfdashboardApplicationButtonPrivate		XfdashboardApplicationButtonPrivate;

struct _XfdashboardApplicationButton
{
	/*< private >*/
	/* Parent instance */
	XfdashboardButton						parent_instance;

	/* Private structure */
	XfdashboardApplicationButtonPrivate		*priv;
};

struct _XfdashboardApplicationButtonClass
{
	/*< private >*/
	/* Parent class */
	XfdashboardButtonClass					parent_class;

	/*< public >*/
	/* Virtual functions */
};

/* Public API */
GType xfdashboard_application_button_get_type(void) G_GNUC_CONST;

ClutterActor* xfdashboard_application_button_new(void);
ClutterActor* xfdashboard_application_button_new_from_app_info(GAppInfo *inAppInfo);

GAppInfo* xfdashboard_application_button_get_app_info(XfdashboardApplicationButton *self);
void xfdashboard_application_button_set_app_info(XfdashboardApplicationButton *self,
													GAppInfo *inAppInfo);

gboolean xfdashboard_application_button_get_show_description(XfdashboardApplicationButton *self);
void xfdashboard_application_button_set_show_description(XfdashboardApplicationButton *self,
															gboolean inShowDescription);

const gchar* xfdashboard_application_button_get_format_title_only(XfdashboardApplicationButton *self);
void xfdashboard_application_button_set_format_title_only(XfdashboardApplicationButton *self,
															const gchar *inFormat);

const gchar* xfdashboard_application_button_get_format_title_description(XfdashboardApplicationButton *self);
void xfdashboard_application_button_set_format_title_description(XfdashboardApplicationButton *self,
																	const gchar *inFormat);

const gchar* xfdashboard_application_button_get_display_name(XfdashboardApplicationButton *self);
const gchar* xfdashboard_application_button_get_icon_name(XfdashboardApplicationButton *self);

gboolean xfdashboard_application_button_execute(XfdashboardApplicationButton *self, GAppLaunchContext *inContext);

guint xfdashboard_application_button_add_popup_menu_items_for_windows(XfdashboardApplicationButton *self,
																		XfdashboardPopupMenu *inMenu);
guint xfdashboard_application_button_add_popup_menu_items_for_actions(XfdashboardApplicationButton *self,
																		XfdashboardPopupMenu *inMenu);

G_END_DECLS

#endif	/* __LIBXFDASHBOARD_APPLICATION_BUTTON__ */
