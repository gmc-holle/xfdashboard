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

/**
 * SECTION:debug
 * @short_description: Debug functions and macros
 * @include: xfdashboard/debug.h
 *
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <libxfdashboard/debug.h>


/* IMPLEMENTATION: Public API */
guint		xfdashboard_debug_flags=0;
gchar		**xfdashboard_debug_classes=NULL;

void xfdashboard_debug_messagev(const char *inFormat, va_list inArgs)
{
	static gint64		beginDebugTimestamp=-1;
	gchar				*timestamp;
	gchar				*format;
	gint64				currentTime;
	gfloat				debugTimestamp;

	/* Get current time */
	currentTime=g_get_monotonic_time();
	if(beginDebugTimestamp<0) beginDebugTimestamp=currentTime;
	debugTimestamp=(((gfloat)currentTime)-((gfloat)beginDebugTimestamp)) / G_USEC_PER_SEC;
	timestamp=g_strdup_printf("[%+16.4f]", debugTimestamp);

	/* Create new format used for message containing the timestamp */
	format=g_strconcat(timestamp, ":", inFormat, NULL);
	g_logv(G_LOG_DOMAIN, G_LOG_LEVEL_MESSAGE, format, inArgs);

	/* Release allocated ressources */
	g_free(format);
	g_free(timestamp);
}

void xfdashboard_debug_message(const char *inFormat, ...)
{
	va_list		args;

	va_start(args, inFormat);
	xfdashboard_debug_messagev(inFormat, args);
	va_end(args);
}
