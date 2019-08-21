/*
 * compat: Compatibility layer for various libraries and versions
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

#ifndef __LIBXFDASHBOARD_COMPAT__
#define __LIBXFDASHBOARD_COMPAT__

#if !defined(__LIBXFDASHBOARD_H_INSIDE__) && !defined(LIBXFDASHBOARD_COMPILATION)
#error "Only <libxfdashboard/libxfdashboard.h> can be included directly."
#endif

#include <clutter/clutter.h>

G_BEGIN_DECLS

#if !CLUTTER_CHECK_VERSION(1, 23, 4)
#define clutter_actor_is_visible(actor)		CLUTTER_ACTOR_IS_VISIBLE( (actor) )
#define clutter_actor_is_mapped(actor)		CLUTTER_ACTOR_IS_MAPPED( (actor) )
#define clutter_actor_is_realized(actor)	CLUTTER_ACTOR_IS_REALIZED( (actor) )
#define clutter_actor_get_reactive(actor)	CLUTTER_ACTOR_IS_REACTIVE( (actor) )
#endif

#if !GLIB_CHECK_VERSION(2, 44, 0)
inline static gboolean g_strv_contains(const gchar * const *inStringList, const gchar *inString)
{
	g_return_val_if_fail(inStringList, FALSE);
	g_return_val_if_fail(inString, FALSE);

	for(; *inStringList; inStringList++)
	{
		if(g_str_equal(inString, *inStringList)) return(TRUE);
	}

	return(FALSE);
}
#endif

G_END_DECLS

#endif	/* __LIBXFDASHBOARD_COMPAT__ */
