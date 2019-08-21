/*
 * window: A managed window of window manager
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

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <libxfdashboard/window-content.h>

#include <glib/gi18n-lib.h>

#include <libxfdashboard/x11/window-content-x11.h>
#include <libxfdashboard/compat.h>


/* Define this class in GObject system */
G_DEFINE_ABSTRACT_TYPE(XfdashboardWindowContent,
						xfdashboard_window_content,
						G_TYPE_OBJECT);

/* IMPLEMENTATION: GObject */

/* Class initialization
 * Override functions in parent classes and define properties
 * and signals
 */
void xfdashboard_window_content_class_init(XfdashboardWindowContentClass *klass)
{
}

/* Object initialization
 * Create private structure and set up default values
 */
void xfdashboard_window_content_init(XfdashboardWindowContent *self)
{
}
