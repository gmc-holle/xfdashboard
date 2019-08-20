/*
 * desktop-app-info-action: An application action defined at desktop entry
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

#ifndef __LIBXFDASHBOARD_DESKTOP_APP_INFO_ACTION__
#define __LIBXFDASHBOARD_DESKTOP_APP_INFO_ACTION__

#if !defined(__LIBXFDASHBOARD_H_INSIDE__) && !defined(LIBXFDASHBOARD_COMPILATION)
#error "Only <libxfdashboard/libxfdashboard.h> can be included directly."
#endif

#include <garcon/garcon.h>

G_BEGIN_DECLS

#define XFDASHBOARD_TYPE_DESKTOP_APP_INFO_ACTION			(xfdashboard_desktop_app_info_action_get_type())
#define XFDASHBOARD_DESKTOP_APP_INFO_ACTION(obj)			(G_TYPE_CHECK_INSTANCE_CAST((obj), XFDASHBOARD_TYPE_DESKTOP_APP_INFO_ACTION, XfdashboardDesktopAppInfoAction))
#define XFDASHBOARD_IS_DESKTOP_APP_INFO_ACTION(obj)			(G_TYPE_CHECK_INSTANCE_TYPE((obj), XFDASHBOARD_TYPE_DESKTOP_APP_INFO_ACTION))
#define XFDASHBOARD_DESKTOP_APP_INFO_ACTION_CLASS(klass)	(G_TYPE_CHECK_CLASS_CAST((klass), XFDASHBOARD_TYPE_DESKTOP_APP_INFO_ACTION, XfdashboardDesktopAppInfoActionClass))
#define XFDASHBOARD_IS_DESKTOP_APP_INFO_ACTION_CLASS(klass)	(G_TYPE_CHECK_CLASS_TYPE((klass), XFDASHBOARD_TYPE_DESKTOP_APP_INFO_ACTION))
#define XFDASHBOARD_DESKTOP_APP_INFO_ACTION_GET_CLASS(obj)	(G_TYPE_INSTANCE_GET_CLASS((obj), XFDASHBOARD_TYPE_DESKTOP_APP_INFO_ACTION, XfdashboardDesktopAppInfoActionClass))

typedef struct _XfdashboardDesktopAppInfoAction				XfdashboardDesktopAppInfoAction;
typedef struct _XfdashboardDesktopAppInfoActionClass		XfdashboardDesktopAppInfoActionClass;
typedef struct _XfdashboardDesktopAppInfoActionPrivate		XfdashboardDesktopAppInfoActionPrivate;

struct _XfdashboardDesktopAppInfoAction
{
	/*< private >*/
	/* Parent instance */
	GObject									parent_instance;

	/* Private structure */
	XfdashboardDesktopAppInfoActionPrivate	*priv;
};

struct _XfdashboardDesktopAppInfoActionClass
{
	/*< private >*/
	/* Parent class */
	GObjectClass							parent_class;

	/*< public >*/
	/* Virtual functions */
	void (*activate)(XfdashboardDesktopAppInfoAction *self);
};

/* Public API */
GType xfdashboard_desktop_app_info_action_get_type(void) G_GNUC_CONST;

const gchar* xfdashboard_desktop_app_info_action_get_name(XfdashboardDesktopAppInfoAction *self);
void xfdashboard_desktop_app_info_action_set_name(XfdashboardDesktopAppInfoAction *self,
													const gchar *inName);

const gchar* xfdashboard_desktop_app_info_action_get_icon_name(XfdashboardDesktopAppInfoAction *self);
void xfdashboard_desktop_app_info_action_set_icon_name(XfdashboardDesktopAppInfoAction *self,
														const gchar *inIconName);

const gchar* xfdashboard_desktop_app_info_action_get_command(XfdashboardDesktopAppInfoAction *self);
void xfdashboard_desktop_app_info_action_set_command(XfdashboardDesktopAppInfoAction *self,
														const gchar *inCommand);

G_END_DECLS

#endif	/* __LIBXFDASHBOARD_DESKTOP_APP_INFO_ACTION__ */
