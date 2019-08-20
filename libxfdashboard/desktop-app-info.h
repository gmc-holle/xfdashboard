/*
 * desktop-app-info: A GDesktopAppInfo like object for garcon menu
 *                   items implementing and supporting GAppInfo
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

#ifndef __LIBXFDASHBOARD_DESKTOP_APP_INFO__
#define __LIBXFDASHBOARD_DESKTOP_APP_INFO__

#if !defined(__LIBXFDASHBOARD_H_INSIDE__) && !defined(LIBXFDASHBOARD_COMPILATION)
#error "Only <libxfdashboard/libxfdashboard.h> can be included directly."
#endif

#include <libxfdashboard/desktop-app-info-action.h>
#include <garcon/garcon.h>

G_BEGIN_DECLS

#define XFDASHBOARD_TYPE_DESKTOP_APP_INFO				(xfdashboard_desktop_app_info_get_type())
#define XFDASHBOARD_DESKTOP_APP_INFO(obj)				(G_TYPE_CHECK_INSTANCE_CAST((obj), XFDASHBOARD_TYPE_DESKTOP_APP_INFO, XfdashboardDesktopAppInfo))
#define XFDASHBOARD_IS_DESKTOP_APP_INFO(obj)			(G_TYPE_CHECK_INSTANCE_TYPE((obj), XFDASHBOARD_TYPE_DESKTOP_APP_INFO))
#define XFDASHBOARD_DESKTOP_APP_INFO_CLASS(klass)		(G_TYPE_CHECK_CLASS_CAST((klass), XFDASHBOARD_TYPE_DESKTOP_APP_INFO, XfdashboardDesktopAppInfoClass))
#define XFDASHBOARD_IS_DESKTOP_APP_INFO_CLASS(klass)	(G_TYPE_CHECK_CLASS_TYPE((klass), XFDASHBOARD_TYPE_DESKTOP_APP_INFO))
#define XFDASHBOARD_DESKTOP_APP_INFO_GET_CLASS(obj)		(G_TYPE_INSTANCE_GET_CLASS((obj), XFDASHBOARD_TYPE_DESKTOP_APP_INFO, XfdashboardDesktopAppInfoClass))

typedef struct _XfdashboardDesktopAppInfo				XfdashboardDesktopAppInfo;
typedef struct _XfdashboardDesktopAppInfoClass			XfdashboardDesktopAppInfoClass;
typedef struct _XfdashboardDesktopAppInfoPrivate		XfdashboardDesktopAppInfoPrivate;

struct _XfdashboardDesktopAppInfo
{
	/*< private >*/
	/* Parent instance */
	GObject								parent_instance;

	/* Private structure */
	XfdashboardDesktopAppInfoPrivate	*priv;
};

struct _XfdashboardDesktopAppInfoClass
{
	/*< private >*/
	/* Parent class */
	GObjectClass						parent_class;

	/*< public >*/
	/* Virtual functions */
	void (*changed)(XfdashboardDesktopAppInfo *self);
};

/* Public API */
GType xfdashboard_desktop_app_info_get_type(void) G_GNUC_CONST;

GAppInfo* xfdashboard_desktop_app_info_new_from_desktop_id(const gchar *inDesktopID);
GAppInfo* xfdashboard_desktop_app_info_new_from_path(const gchar *inPath);
GAppInfo* xfdashboard_desktop_app_info_new_from_file(GFile *inFile);
GAppInfo* xfdashboard_desktop_app_info_new_from_menu_item(GarconMenuItem *inMenuItem);

gboolean xfdashboard_desktop_app_info_is_valid(XfdashboardDesktopAppInfo *self);

GFile* xfdashboard_desktop_app_info_get_file(XfdashboardDesktopAppInfo *self);
gboolean xfdashboard_desktop_app_info_reload(XfdashboardDesktopAppInfo *self);

GList* xfdashboard_desktop_app_info_get_actions(XfdashboardDesktopAppInfo *self);
gboolean xfdashboard_desktop_app_info_launch_action(XfdashboardDesktopAppInfo *self,
													XfdashboardDesktopAppInfoAction *inAction,
													GAppLaunchContext *inContext,
													GError **outError);
gboolean xfdashboard_desktop_app_info_launch_action_by_name(XfdashboardDesktopAppInfo *self,
															const gchar *inActionName,
															GAppLaunchContext *inContext,
															GError **outError);

GList* xfdashboard_desktop_app_info_get_keywords(XfdashboardDesktopAppInfo *self);

G_END_DECLS

#endif	/* __LIBXFDASHBOARD_DESKTOP_APP_INFO__ */
