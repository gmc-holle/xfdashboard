/*
 * common.h: Common function and definitions
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

#ifndef __XFOVERVIEW_COMMON__
#define __XFOVERVIEW_COMMON__

#define WNCK_I_KNOW_THIS_IS_UNSTABLE
#include <libwnck/libwnck.h>

#include <garcon/garcon.h>

#ifndef DEBUG_ALLOC_BOX
#if 1
#define DEBUG_ALLOC_BOX(b) g_message("%s: %s=%.0f,%.0f - %.0f,%.0f [%.2fx%.2f]", __func__, #b, (b)->x1, (b)->y1, (b)->x2, (b)->y2, (b)->x2-(b)->x1, (b)->y2-(b)->y1);
#else
#define DEBUG_ALLOC_BOX(b)
#endif
#endif

G_BEGIN_DECLS

/* Gobject type of pointer array (GPtrArray) */
#define XFDASHBOARD_TYPE_VALUE_ARRAY	(xfdashboard_value_array_get_type())

/* List mode for views */
typedef enum
{
	XFDASHBOARD_VIEW_LIST=0,
	XFDASHBOARD_VIEW_ICON
} XfdashboardViewMode;

/* Visibility policy (e.g. for scroll bars in views) */
typedef enum
{
	XFDASHBOARD_POLICY_NEVER=0,
	XFDASHBOARD_POLICY_AUTOMATIC,
	XFDASHBOARD_POLICY_ALWAYS
} XfdashboardPolicy;

/* Style (e.g. used in buttons) */
typedef enum
{
	XFDASHBOARD_STYLE_TEXT=0,
	XFDASHBOARD_STYLE_ICON,
	XFDASHBOARD_STYLE_BOTH,
} XfdashboardStyle;

/* Orientation (e.g. used in buttons) */
typedef enum
{
	XFDASHBOARD_ORIENTATION_LEFT=0,
	XFDASHBOARD_ORIENTATION_RIGHT,
	XFDASHBOARD_ORIENTATION_TOP,
	XFDASHBOARD_ORIENTATION_BOTTOM,
} XfdashboardOrientation;

/* Public API */
GType xfdashboard_value_array_get_type(void) G_GNUC_CONST;

WnckWindow* xfdashboard_get_stage_window();

GarconMenu* xfdashboard_get_application_menu();

GdkPixbuf* xfdashboard_get_pixbuf_for_icon_name(const gchar *inIconName, gint inSize);
GdkPixbuf* xfdashboard_get_pixbuf_for_icon_name_scaled(const gchar *inIconName, gint inSize);

G_END_DECLS

#endif
