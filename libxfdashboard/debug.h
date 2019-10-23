/*
 * debug: Helpers for debugging
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

#ifndef __LIBXFDASHBOARD_DEBUG__
#define __LIBXFDASHBOARD_DEBUG__

#if !defined(__LIBXFDASHBOARD_H_INSIDE__) && !defined(LIBXFDASHBOARD_COMPILATION)
#error "Only <libxfdashboard/libxfdashboard.h> can be included directly."
#endif

#include <glib.h>
#include "compat.h"

G_BEGIN_DECLS

/* Public definitions */
/**
 * XfdashboardDebugFlags:
 * @XFDASHBOARD_DEBUG_MISC: Miscellaneous, if debug message does not fit in any other category
 * @XFDASHBOARD_DEBUG_ACTOR: Actor related debug messages
 * @XFDASHBOARD_DEBUG_STYLE: Style at actor debug messages (resolving CSS, applying style etc.)
 * @XFDASHBOARD_DEBUG_THEME: Theme related debug messages (loading theme and resources etc.)
 * @XFDASHBOARD_DEBUG_APPLICATIONS: Application related debug message (spawing application process, application database and tracker etc.)
 * @XFDASHBOARD_DEBUG_IMAGES: Images related debug message (image cache etc.)
 * @XFDASHBOARD_DEBUG_WINDOWS: Windows related debug message (window tracker, workspaces, windows, monitors etc.)
 * @XFDASHBOARD_DEBUG_PLUGINS: Plug-ins related debug message (plugin manager and plugin base class)
 * @XFDASHBOARD_DEBUG_ANIMATION: Animation related debug message
 *
 * Debug categories
 */
typedef enum /*< skip,flags,prefix=XFDASHBOARD_DEBUG >*/
{
	XFDASHBOARD_DEBUG_MISC			= 1 << 0,
	XFDASHBOARD_DEBUG_ACTOR			= 1 << 1,
	XFDASHBOARD_DEBUG_STYLE			= 1 << 2,
	XFDASHBOARD_DEBUG_THEME			= 1 << 3,
	XFDASHBOARD_DEBUG_APPLICATIONS	= 1 << 4,
	XFDASHBOARD_DEBUG_IMAGES		= 1 << 5,
	XFDASHBOARD_DEBUG_WINDOWS		= 1 << 6,
	XFDASHBOARD_DEBUG_PLUGINS		= 1 << 7,
	XFDASHBOARD_DEBUG_ANIMATION		= 1 << 8
} XfdashboardDebugFlags;

#ifdef XFDASHBOARD_ENABLE_DEBUG

#define XFDASHBOARD_HAS_DEBUG(inCategory) \
	((xfdashboard_debug_flags & XFDASHBOARD_DEBUG_##inCategory) != FALSE)

#ifndef __GNUC__

/* Try the GCC extension for valists in macros */
#define XFDASHBOARD_DEBUG(self, inCategory, inMessage, inArgs...)              \
	G_STMT_START \
	{ \
		if(G_UNLIKELY(XFDASHBOARD_HAS_DEBUG(inCategory)) ||                    \
			G_UNLIKELY(xfdashboard_debug_classes && self && g_strv_contains((const gchar * const *)xfdashboard_debug_classes, G_OBJECT_TYPE_NAME(self))))\
		{ \
			xfdashboard_debug_message("[%s@%p]:[" #inCategory "]:" G_STRLOC ": " inMessage, (self ? G_OBJECT_TYPE_NAME(self) : ""), self, ##inArgs);\
		} \
	} \
	G_STMT_END

#else /* !__GNUC__ */

/* Try the C99 version; unfortunately, this does not allow us to pass empty
 * arguments to the macro, which means we have to do an intemediate printf.
 */
#define XFDASHBOARD_DEBUG(self, inCategory, ...)                               \
	G_STMT_START \
	{ \
		if(G_UNLIKELY(XFDASHBOARD_HAS_DEBUG(inCategory)) ||                    \
			G_UNLIKELY(xfdashboard_debug_classes && self && g_strv_contains((const gchar * const *)xfdashboard_debug_classes, G_OBJECT_TYPE_NAME(self))))\
		{ \
			gchar	*_xfdashboard_debug_format=g_strdup_printf(__VA_ARGS__);     \
			xfdashboard_debug_message("[%s@%p]:[" #inCategory "]:" G_STRLOC ": %s", (self ? G_OBJECT_TYPE_NAME(self) : ""), self, _xfdashboard_debug_format);\
			g_free(_xfdashboard_debug_format);                                 \
		} \
	} \
	G_STMT_END
#endif

#else /* !XFDASHBOARD_ENABLE_DEBUG */

#define XFDASHBOARD_HAS_DEBUG(inCategory)		FALSE
#define XFDASHBOARD_DEBUG(inCategory, ...)		G_STMT_START { } G_STMT_END

#endif /* XFDASHBOARD_ENABLE_DEBUG */

/* Public data */

extern guint xfdashboard_debug_flags;
extern gchar **xfdashboard_debug_classes;

/* Public API */

void xfdashboard_debug_messagev(const char *inFormat, va_list inArgs);
void xfdashboard_debug_message(const char *inFormat, ...) G_GNUC_PRINTF(1, 2);

G_END_DECLS

#endif	/* __LIBXFDASHBOARD_DEBUG__ */
