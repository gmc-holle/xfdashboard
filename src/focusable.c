/*
 * stylable: An interface which can be inherited by actor and objects
 *           to get styled by a theme
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

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "focusable.h"

#include <clutter/clutter.h>
#include <glib/gi18n-lib.h>

#include "utils.h"

/* Define this interface in GObject system */
G_DEFINE_INTERFACE(XfdashboardFocusable,
					xfdashboard_focusable,
					G_TYPE_OBJECT)

/* IMPLEMENTATION: Private variables and methods */
#define XFDASHBOARD_FOCUSABLE_WARN_NOT_IMPLEMENTED(self, vfunc) \
	g_warning(_("Object of type %s does not implement required virtual function XfdashboardFocusable::%s"), \
				G_OBJECT_TYPE_NAME(self), \
				vfunc);

/* Default implementation of virtual function "can_focus" */
static gboolean _xfdashboard_focusable_real_can_focus(XfdashboardFocusable *self)
{
	g_return_val_if_fail(XFDASHBOARD_IS_FOCUSABLE(self), FALSE);

	/* By default (if not overidden) an focusable actor cannot be focused */
	return(FALSE);
}

/* Default implementation of virtual function "unset_focus" */
static void _xfdashboard_focusable_real_unset_focus(XfdashboardFocusable *self)
{
	g_return_if_fail(XFDASHBOARD_IS_FOCUSABLE(self));

	/* By default (if not overidden) do nothing */
}

/* Default implementation of virtual function "handle_key_event" */
static gboolean _xfdashboard_focusable_real_handle_key_event(XfdashboardFocusable *self, const ClutterEvent *inEvent)
{
	g_return_val_if_fail(XFDASHBOARD_IS_FOCUSABLE(self), CLUTTER_EVENT_PROPAGATE);
	g_return_val_if_fail(CLUTTER_IS_ACTOR(self), CLUTTER_EVENT_PROPAGATE);

	/* By default synthesize event to focusable actor */
	return(clutter_actor_event(CLUTTER_ACTOR(self), inEvent, FALSE));
}

/* IMPLEMENTATION: GObject */

/* Interface initialization
 * Set up default functions
 */
void xfdashboard_focusable_default_init(XfdashboardFocusableInterface *iface)
{
	/* All the following virtual functions must be overridden */
	iface->set_focus=NULL;

	/* The following virtual functions should be overriden if default
	 * implementation does not fit.
	 */
	iface->can_focus=_xfdashboard_focusable_real_can_focus;
	iface->unset_focus=_xfdashboard_focusable_real_unset_focus;
	iface->handle_key_event=_xfdashboard_focusable_real_handle_key_event;
}

/* Implementation: Public API */

/* Call virtual function "can_focus" */
gboolean xfdashboard_focusable_can_focus(XfdashboardFocusable *self)
{
	XfdashboardFocusableInterface		*iface;

	g_return_val_if_fail(XFDASHBOARD_IS_FOCUSABLE(self), FALSE);

	iface=XFDASHBOARD_FOCUSABLE_GET_IFACE(self);

	/* Call virtual function */
	if(iface->can_focus)
	{
		return(iface->can_focus(self));
	}

	/* If we get here the virtual function was not overridden */
	XFDASHBOARD_FOCUSABLE_WARN_NOT_IMPLEMENTED(self, "can_focus");
	return(FALSE);
}

/* Call virtual function "set_focus" */
void xfdashboard_focusable_set_focus(XfdashboardFocusable *self)
{
	XfdashboardFocusableInterface		*iface;

	g_return_if_fail(XFDASHBOARD_IS_FOCUSABLE(self));

	iface=XFDASHBOARD_FOCUSABLE_GET_IFACE(self);

	/* Call virtual function */
	if(iface->set_focus)
	{
		iface->set_focus(self);
		return;
	}

	/* If we get here the virtual function was not overridden */
	XFDASHBOARD_FOCUSABLE_WARN_NOT_IMPLEMENTED(self, "set_focus");
}

/* Call virtual function "unset_focus" */
void xfdashboard_focusable_unset_focus(XfdashboardFocusable *self)
{
	XfdashboardFocusableInterface		*iface;

	g_return_if_fail(XFDASHBOARD_IS_FOCUSABLE(self));

	iface=XFDASHBOARD_FOCUSABLE_GET_IFACE(self);

	/* Call virtual function */
	if(iface->unset_focus)
	{
		iface->unset_focus(self);
		return;
	}

	/* If we get here the virtual function was not overridden */
	XFDASHBOARD_FOCUSABLE_WARN_NOT_IMPLEMENTED(self, "unset_focus");
}

/* Call virtual function "handle_key_event" */
gboolean xfdashboard_focusable_handle_key_event(XfdashboardFocusable *self, const ClutterEvent *inEvent)
{
	XfdashboardFocusableInterface		*iface;

	g_return_val_if_fail(XFDASHBOARD_IS_FOCUSABLE(self), FALSE);

	iface=XFDASHBOARD_FOCUSABLE_GET_IFACE(self);

	/* Call virtual function */
	if(iface->handle_key_event)
	{
		return(iface->handle_key_event(self, inEvent));
	}

	/* If we get here the virtual function was not overridden */
	XFDASHBOARD_FOCUSABLE_WARN_NOT_IMPLEMENTED(self, "handle_key_event");
	return(FALSE);
}
