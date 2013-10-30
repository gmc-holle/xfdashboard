/*
 * quicklaunch: Quicklaunch box
 * 
 * Copyright 2012-2013 Stephan Haller <nomad@froevel.de>
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

#ifndef __XFOVERVIEW_QUICKLAUNCH__
#define __XFOVERVIEW_QUICKLAUNCH__

#include "background.h"

G_BEGIN_DECLS

#define XFDASHBOARD_TYPE_QUICKLAUNCH				(xfdashboard_quicklaunch_get_type())
#define XFDASHBOARD_QUICKLAUNCH(obj)				(G_TYPE_CHECK_INSTANCE_CAST((obj), XFDASHBOARD_TYPE_QUICKLAUNCH, XfdashboardQuicklaunch))
#define XFDASHBOARD_IS_QUICKLAUNCH(obj)				(G_TYPE_CHECK_INSTANCE_TYPE((obj), XFDASHBOARD_TYPE_QUICKLAUNCH))
#define XFDASHBOARD_QUICKLAUNCH_CLASS(klass)		(G_TYPE_CHECK_CLASS_CAST((klass), XFDASHBOARD_TYPE_QUICKLAUNCH, XfdashboardQuicklaunchClass))
#define XFDASHBOARD_IS_QUICKLAUNCH_CLASS(klass)		(G_TYPE_CHECK_CLASS_TYPE((klass), XFDASHBOARD_TYPE_QUICKLAUNCH))
#define XFDASHBOARD_QUICKLAUNCH_GET_CLASS(obj)		(G_TYPE_INSTANCE_GET_CLASS((obj), XFDASHBOARD_TYPE_QUICKLAUNCH, XfdashboardQuicklaunchClass))

typedef struct _XfdashboardQuicklaunch				XfdashboardQuicklaunch;
typedef struct _XfdashboardQuicklaunchClass			XfdashboardQuicklaunchClass;
typedef struct _XfdashboardQuicklaunchPrivate		XfdashboardQuicklaunchPrivate;

struct _XfdashboardQuicklaunch
{
	/* Parent instance */
	XfdashboardBackground			parent_instance;

	/* Private structure */
	XfdashboardQuicklaunchPrivate		*priv;
};

struct _XfdashboardQuicklaunchClass
{
	/*< private >*/
	/* Parent class */
	XfdashboardBackgroundClass		parent_class;

	/*< public >*/
	/* Virtual functions */
};

/* Public API */
GType xfdashboard_quicklaunch_get_type(void) G_GNUC_CONST;

ClutterActor* xfdashboard_quicklaunch_new(void);
ClutterActor* xfdashboard_quicklaunch_new_with_orientation(ClutterOrientation inOrientation);

gfloat xfdashboard_quicklaunch_get_spacing(XfdashboardQuicklaunch *self);
void xfdashboard_quicklaunch_set_spacing(XfdashboardQuicklaunch *self, const gfloat inSpacing);

ClutterOrientation xfdashboard_quicklaunch_get_orientation(XfdashboardQuicklaunch *self);
void xfdashboard_quicklaunch_set_orientation(XfdashboardQuicklaunch *self, ClutterOrientation inOrientation);

G_END_DECLS

#endif	/* __XFOVERVIEW_QUICKLAUNCH__ */
