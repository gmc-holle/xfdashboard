/*
 * window-tracker-workspace: A workspace tracked by window tracker and
 *                           also a wrapper class around WnckWorkspace.
 *                           By wrapping libwnck objects we can use a 
 *                           virtual stable API while the API in libwnck
 *                           changes within versions. We only need to
 *                           use #ifdefs in window tracker object and
 *                           nowhere else in the code.
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

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "window-tracker-workspace.h"

#define WNCK_I_KNOW_THIS_IS_UNSTABLE
#include <libwnck/libwnck.h>

#include <glib/gi18n-lib.h>

#include "marshal.h"

/* Usually we found define a class in GObject system here but
 * this class is a wrapper around WnckWorkspace to create a virtual stable
 * libwnck API regardless of its version.
 */

/* Implementation: Public API */

/* Return type of WnckWorkspace as our type */
GType xfdashboard_window_tracker_workspace_get_type(void)
{
	return(WNCK_TYPE_WORKSPACE);
}
