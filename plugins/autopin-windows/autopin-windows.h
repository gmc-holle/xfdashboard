/*
 * autopin-windows: Pins or unpins windows depending on the monitor
 *                  it is located
 * 
 * Copyright 2012-2020 Stephan Haller <nomad@froevel.de>
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

#ifndef __XFDASHBOARD_AUTOPIN_WINDOWS__
#define __XFDASHBOARD_AUTOPIN_WINDOWS__

#include <libxfdashboard/libxfdashboard.h>

G_BEGIN_DECLS

#define XFDASHBOARD_TYPE_AUTOPIN_WINDOWS				(xfdashboard_autopin_windows_get_type())
#define XFDASHBOARD_AUTOPIN_WINDOWS(obj)				(G_TYPE_CHECK_INSTANCE_CAST((obj), XFDASHBOARD_TYPE_AUTOPIN_WINDOWS, XfdashboardAutopinWindows))
#define XFDASHBOARD_IS_AUTOPIN_WINDOWS(obj)				(G_TYPE_CHECK_INSTANCE_TYPE((obj), XFDASHBOARD_TYPE_AUTOPIN_WINDOWS))
#define XFDASHBOARD_AUTOPIN_WINDOWS_CLASS(klass)		(G_TYPE_CHECK_CLASS_CAST((klass), XFDASHBOARD_TYPE_AUTOPIN_WINDOWS, XfdashboardAutopinWindowsClass))
#define XFDASHBOARD_IS_AUTOPIN_WINDOWS_CLASS(klass)		(G_TYPE_CHECK_CLASS_TYPE((klass), XFDASHBOARD_TYPE_AUTOPIN_WINDOWS))
#define XFDASHBOARD_AUTOPIN_WINDOWS_GET_CLASS(obj)		(G_TYPE_INSTANCE_GET_CLASS((obj), XFDASHBOARD_TYPE_AUTOPIN_WINDOWS, XfdashboardAutopinWindowsClass))

typedef struct _XfdashboardAutopinWindows				XfdashboardAutopinWindows; 
typedef struct _XfdashboardAutopinWindowsPrivate		XfdashboardAutopinWindowsPrivate;
typedef struct _XfdashboardAutopinWindowsClass			XfdashboardAutopinWindowsClass;

struct _XfdashboardAutopinWindows
{
	/* Parent instance */
	GObject								parent_instance;

	/* Private structure */
	XfdashboardAutopinWindowsPrivate	*priv;
};

struct _XfdashboardAutopinWindowsClass
{
	/*< private >*/
	/* Parent class */
	GObjectClass						parent_class;
};

/* Public API */
GType xfdashboard_autopin_windows_get_type(void) G_GNUC_CONST;

XFDASHBOARD_DECLARE_PLUGIN_TYPE(xfdashboard_autopin_windows);

XfdashboardAutopinWindows* xfdashboard_autopin_windows_new(void);

G_END_DECLS

#endif
