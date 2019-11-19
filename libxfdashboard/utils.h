/*
 * utils: Common functions, helpers and definitions
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

#ifndef __LIBXFDASHBOARD_UTILS__
#define __LIBXFDASHBOARD_UTILS__

#if !defined(__LIBXFDASHBOARD_H_INSIDE__) && !defined(LIBXFDASHBOARD_COMPILATION)
#error "Only <libxfdashboard/libxfdashboard.h> can be included directly."
#endif

#include <clutter/clutter.h>
#include <gio/gio.h>

#include <libxfdashboard/window-tracker-workspace.h>
#include <libxfdashboard/stage-interface.h>
#include <libxfdashboard/stage.h>
#include <libxfdashboard/css-selector.h>

G_BEGIN_DECLS

/* Debug macros */
#define _DEBUG_OBJECT_NAME(x) \
	((x)!=NULL ? G_OBJECT_TYPE_NAME((x)) : "<nil>")

#define _DEBUG_BOX(msg, box) \
	g_message("%s: %s: x1=%.2f, y1=%.2f, x2=%.2f, y2=%.2f [%.2fx%.2f]", \
				__func__, \
				msg, \
				(box).x1, (box).y1, (box).x2, (box).y2, \
				(box).x2-(box).x1, (box).y2-(box).y1)

#define _DEBUG_NOTIFY(self, property, format, ...) \
	g_message("%s: Property '%s' of %p (%s) changed to "format, \
				__func__, \
				property, \
				(self), (self) ? G_OBJECT_TYPE_NAME((self)) : "<nil>", \
				## __VA_ARGS__)

/* Gobject type of pointer array (GPtrArray) */
#define XFDASHBOARD_TYPE_POINTER_ARRAY		(xfdashboard_pointer_array_get_type())

/* Public API */

/**
 * GTYPE_TO_POINTER:
 * @gtype: A #GType to stuff into the pointer
 *
 * Stuffs the #GType specified at @gtype into a pointer type.
 */
#define GTYPE_TO_POINTER(gtype) \
	(GSIZE_TO_POINTER(gtype))

/**
 * GPOINTER_TO_GTYPE:
 * @pointer: A pointer to extract a #GType from
 *
 * Extracts a #GType from a pointer. The #GType must have been stored in the pointer with GTYPE_TO_POINTER().
 */
#define GPOINTER_TO_GTYPE(pointer) \
	((GType)GPOINTER_TO_SIZE(pointer))

GType xfdashboard_pointer_array_get_type(void);

void xfdashboard_notify(ClutterActor *inSender,
							const gchar *inIconName,
							const gchar *inFormat, ...)
							G_GNUC_PRINTF(3, 4);

GAppLaunchContext* xfdashboard_create_app_context(XfdashboardWindowTrackerWorkspace *inWorkspace);

void xfdashboard_register_gvalue_transformation_funcs(void);

ClutterActor* xfdashboard_find_actor_by_name(ClutterActor *inActor, const gchar *inName);

/**
 * XfdashboardTraversalCallback:
 * @inActor: The actor currently processed and has matched the selector in traversal
 * @inUserData: Data passed to the function, set with xfdashboard_traverse_actor()
 *
 * A callback called each time an actor matches the provided css selector
 * in xfdashboard_traverse_actor().
 *
 * Returns: %FALSE if the traversal should be stopped. #XFDASHBOARD_TRAVERSAL_STOP
 * and #XFDASHBOARD_TRAVERSAL_CONTINUE are more memorable names for the return value.
 */
typedef gboolean (*XfdashboardTraversalCallback)(ClutterActor *inActor, gpointer inUserData);

/**
 * XFDASHBOARD_TRAVERSAL_STOP:
 *
 * Use this macro as the return value of a #XfdashboardTraversalCallback to stop
 * further traversal in xfdashboard_traverse_actor().
 */
#define XFDASHBOARD_TRAVERSAL_STOP			(FALSE)

/**
 * XFDASHBOARD_TRAVERSAL_CONTINUE:
 *
 * Use this macro as the return value of a #XfdashboardTraversalCallback to continue
 * further traversal in xfdashboard_traverse_actor().
 */
#define XFDASHBOARD_TRAVERSAL_CONTINUE		(TRUE)

void xfdashboard_traverse_actor(ClutterActor *inRootActor,
								XfdashboardCssSelector *inSelector,
								XfdashboardTraversalCallback inCallback,
								gpointer inUserData);

XfdashboardStageInterface* xfdashboard_get_stage_of_actor(ClutterActor *inActor);

gchar** xfdashboard_split_string(const gchar *inString, const gchar *inDelimiters);

gboolean xfdashboard_is_valid_id(const gchar *inString);

gchar* xfdashboard_get_enum_value_name(GType inEnumClass, gint inValue);
gint xfdashboard_get_enum_value_from_nickname(GType inEnumClass, const gchar *inNickname);

void xfdashboard_dump_actor(ClutterActor *inActor);

G_END_DECLS

#endif	/* __LIBXFDASHBOARD_UTILS__ */
