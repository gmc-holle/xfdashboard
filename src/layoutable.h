/*
 * layoutable: An interface which can be inherited by buildable objects
 *             from theme layout to get notified about various state
 *             while building
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

#ifndef __XFDASHBOARD_LAYOUTABLE__
#define __XFDASHBOARD_LAYOUTABLE__

#include <glib-object.h>

G_BEGIN_DECLS

#define XFDASHBOARD_TYPE_LAYOUTABLE					(xfdashboard_layoutable_get_type())
#define XFDASHBOARD_LAYOUTABLE(obj)					(G_TYPE_CHECK_INSTANCE_CAST((obj), XFDASHBOARD_TYPE_LAYOUTABLE, XfdashboardLayoutable))
#define XFDASHBOARD_IS_LAYOUTABLE(obj)				(G_TYPE_CHECK_INSTANCE_TYPE((obj), XFDASHBOARD_TYPE_LAYOUTABLE))
#define XFDASHBOARD_LAYOUTABLE_GET_IFACE(obj)		(G_TYPE_INSTANCE_GET_INTERFACE((obj), XFDASHBOARD_TYPE_LAYOUTABLE, XfdashboardLayoutableInterface))

typedef struct _XfdashboardLayoutable				XfdashboardLayoutable;
typedef struct _XfdashboardLayoutableInterface		XfdashboardLayoutableInterface;

struct _XfdashboardLayoutableInterface
{
	/*< private >*/
	/* Parent interface */
	GTypeInterface				parent_interface;

	/*< public >*/
	/* Virtual functions */
	void (*layout_completed)(XfdashboardLayoutable *self);
};

/* Public API */
GType xfdashboard_layoutable_get_type(void) G_GNUC_CONST;

void xfdashboard_layoutable_layout_completed(XfdashboardLayoutable *self);

G_END_DECLS

#endif	/* __XFDASHBOARD_LAYOUTABLE__ */
