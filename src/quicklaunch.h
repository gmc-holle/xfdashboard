/*
 * quicklaunch.h: Quicklaunch box
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

#ifndef __XFDASHBOARD_QUICKLAUNCH__
#define __XFDASHBOARD_QUICKLAUNCH__

#include "application-icon.h"

#include <clutter/clutter.h>

G_BEGIN_DECLS

#define XFDASHBOARD_TYPE_QUICKLAUNCH				(xfdashboard_quicklaunch_get_type())
#define XFDASHBOARD_QUICKLAUNCH(obj)				(G_TYPE_CHECK_INSTANCE_CAST((obj), XFDASHBOARD_TYPE_QUICKLAUNCH, XfdashboardQuicklaunch))
#define XFDASHBOARD_IS_QUICKLAUNCH(obj)				(G_TYPE_CHECK_INSTANCE_TYPE((obj), XFDASHBOARD_TYPE_QUICKLAUNCH))
#define XFDASHBOARD_QUICKLAUNCH_CLASS(klass)		(G_TYPE_CHECK_CLASS_CAST((klass), XFDASHBOARD_TYPE_QUICKLAUNCH, XfdashboardQuicklaunchClass))
#define XFDASHBOARD_IS_QUICKLAUNCH_CLASS(klass)		(G_TYPE_CHECK_CLASS_TYPE((klass), XFDASHBOARD_TYPE_QUICKLAUNCH))
#define XFDASHBOARD_QUICKLAUNCH_GET_CLASS(obj)		(G_TYPE_INSTANCE_GET_CLASS((obj), XFDASHBOARD_TYPE_QUICKLAUNCH, XfdashboardQuicklaunchClass))

typedef struct _XfdashboardQuicklaunch				XfdashboardQuicklaunch; 
typedef struct _XfdashboardQuicklaunchPrivate		XfdashboardQuicklaunchPrivate;
typedef struct _XfdashboardQuicklaunchClass			XfdashboardQuicklaunchClass;

struct _XfdashboardQuicklaunch
{
	/* Parent instance */
	ClutterActor					parent_instance;

	/* Private structure */
	XfdashboardQuicklaunchPrivate	*priv;
};

struct _XfdashboardQuicklaunchClass
{
	/* Parent class */
	ClutterActorClass				parent_class;
};

/* Public API */
GType xfdashboard_quicklaunch_get_type(void) G_GNUC_CONST;

ClutterActor* xfdashboard_quicklaunch_new(void);

guint xfdashboard_quicklaunch_get_icon_count(XfdashboardQuicklaunch *self);

guint xfdashboard_quicklaunch_get_max_icon_count(XfdashboardQuicklaunch *self);

guint xfdashboard_quicklaunch_get_normal_icon_size(XfdashboardQuicklaunch *self);
void xfdashboard_quicklaunch_set_normal_icon_size(XfdashboardQuicklaunch *self, guint inSize);

const ClutterColor* xfdashboard_quicklaunch_get_background_color(XfdashboardQuicklaunch *self);
void xfdashboard_quicklaunch_set_background_color(XfdashboardQuicklaunch *self, const ClutterColor *inColor);

gfloat xfdashboard_quicklaunch_get_spacing(XfdashboardQuicklaunch *self);
void xfdashboard_quicklaunch_set_spacing(XfdashboardQuicklaunch *self, gfloat inSpacing);

gboolean xfdashboard_quicklaunch_add_icon(XfdashboardQuicklaunch *self, XfdashboardApplicationIcon *inIcon);
gboolean xfdashboard_quicklaunch_add_icon_by_desktop_file(XfdashboardQuicklaunch *self, const gchar *inDesktopFile);

G_END_DECLS

#endif
