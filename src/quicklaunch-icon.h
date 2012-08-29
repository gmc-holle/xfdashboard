/*
 * quicklaunch-icon.h: An actor used in quicklaunch showing
 *                     icon of an application
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

#ifndef __XFOVERVIEW_QUICKLAUNCH_ICON__
#define __XFOVERVIEW_QUICKLAUNCH_ICON__

#include <clutter/clutter.h>
#include <clutter/x11/clutter-x11.h>
#include <gio/gdesktopappinfo.h>

G_BEGIN_DECLS

#define XFDASHBOARD_TYPE_QUICKLAUNCH_ICON				(xfdashboard_quicklaunch_icon_get_type())
#define XFDASHBOARD_QUICKLAUNCH_ICON(obj)				(G_TYPE_CHECK_INSTANCE_CAST((obj), XFDASHBOARD_TYPE_QUICKLAUNCH_ICON, XfdashboardQuicklaunchIcon))
#define XFDASHBOARD_IS_QUICKLAUNCH_ICON(obj)			(G_TYPE_CHECK_INSTANCE_TYPE((obj), XFDASHBOARD_TYPE_QUICKLAUNCH_ICON))
#define XFDASHBOARD_QUICKLAUNCH_ICON_CLASS(klass)		(G_TYPE_CHECK_CLASS_CAST((klass), XFDASHBOARD_TYPE_QUICKLAUNCH_ICON, XfdashboardQuicklaunchIconClass))
#define XFDASHBOARD_IS_QUICKLAUNCH_ICON_CLASS(klass)	(G_TYPE_CHECK_CLASS_TYPE((klass), XFDASHBOARD_TYPE_QUICKLAUNCH_ICON))
#define XFDASHBOARD_QUICKLAUNCH_ICON_GET_CLASS(obj)		G_TYPE_INSTANCE_GET_CLASS((obj), XFDASHBOARD_TYPE_QUICKLAUNCH_ICON, XfdashboardQuicklaunchIconClass))

typedef struct _XfdashboardQuicklaunchIcon				XfdashboardQuicklaunchIcon;
typedef struct _XfdashboardQuicklaunchIconClass			XfdashboardQuicklaunchIconClass;
typedef struct _XfdashboardQuicklaunchIconPrivate		XfdashboardQuicklaunchIconPrivate;

struct _XfdashboardQuicklaunchIcon
{
	/* Parent instance */
	ClutterTexture						parent_instance;

	/* Private structure */
	XfdashboardQuicklaunchIconPrivate	*priv;
};

struct _XfdashboardQuicklaunchIconClass
{
	/* Parent class */
	ClutterTextureClass					parent_class;

	/* Virtual functions */
	void (*clicked)(XfdashboardQuicklaunchIcon *self);
};

GType xfdashboard_quicklaunch_icon_get_type(void) G_GNUC_CONST;

ClutterActor* xfdashboard_quicklaunch_icon_new(void);
ClutterActor* xfdashboard_quicklaunch_icon_new_full(const gchar *inDesktopFile);

const gchar* xfdashboard_quicklaunch_icon_get_desktop_file(XfdashboardQuicklaunchIcon *self);
void xfdashboard_quicklaunch_icon_set_desktop_file(XfdashboardQuicklaunchIcon *self, const gchar *inDesktopFile);

const GDesktopAppInfo* xfdashboard_quicklaunch_icon_get_desktop_application_info(XfdashboardQuicklaunchIcon *self);

G_END_DECLS

#endif
