/*
 * utils: Common functions, helpers and definitions
 * 
 * Copyrigt 2012-2015 Stephan Haller <nomad@froevel.de>
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

#ifndef __XFDASHBOARD_UTILS__
#define __XFDASHBOARD_UTILS__

#define DEBUG_OBJECT_NAME(x) ((x)!=NULL ? G_OBJECT_TYPE_NAME((x)) : "<nil>")
#define DEBUG_BOX(msg, box) g_message("%s: %s: x1=%.2f, y1=%.2f, x2=%.2f, y2=%.2f [%.2fx%.2f]", __func__, msg, (box).x1, (box).y1, (box).x2, (box).y2, (box).x2-(box).x1, (box).y2-(box).y1)
#define DEBUG_NOTIFY(self, property, format, ...) g_message("%s: Property '%s' of %p (%s) changed to "format, __func__, property, (self), (self) ? G_OBJECT_TYPE_NAME((self)) : "<nil>", ## __VA_ARGS__)

#include <clutter/clutter.h>
#include <gio/gio.h>
#include <garcon/garcon.h>

#include "window-tracker-workspace.h"

G_BEGIN_DECLS

/* Gobject type of pointer array (GPtrArray) */
#define XFDASHBOARD_TYPE_POINTER_ARRAY		(xfdashboard_pointer_array_get_type())

/* Public API */
#define GTYPE_TO_POINTER(gtype)		(GSIZE_TO_POINTER(gtype))
#define GPOINTER_TO_GTYPE(item)		((GType)GPOINTER_TO_SIZE(item))

GType xfdashboard_pointer_array_get_type(void);

void xfdashboard_notify(ClutterActor *inSender, const gchar *inIconName, const gchar *inFormatText, ...) G_GNUC_PRINTF(3, 4);

GAppLaunchContext* xfdashboard_create_app_context(XfdashboardWindowTrackerWorkspace *inWorkspace);

void xfdashboard_register_gvalue_transformation_funcs(void);

gboolean xfdashboard_actor_contains_child_deep(ClutterActor *inActor, ClutterActor *inChild);
ClutterActor* xfdashboard_find_actor_by_name(ClutterActor *inActor, const gchar *inName);

gchar** xfdashboard_split_string(const gchar *inString, const gchar *inDelimiters);

gboolean xfdashboard_is_valid_id(const gchar *inString);

gchar* xfdashboard_get_enum_value_name(GType inEnumClass, gint inValue);

void xfdashboard_dump_actor(ClutterActor *inActor);

G_END_DECLS

#endif	/* __XFDASHBOARD_UTILS__ */
