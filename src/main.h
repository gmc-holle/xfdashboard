/*
 * main.h: Common functions, shared data
 *         and main entry point of application
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

#ifndef __XFOVERVIEW_MAIN__
#define __XFOVERVIEW_MAIN__

#define WNCK_I_KNOW_THIS_IS_UNSTABLE
#include <libwnck/libwnck.h>

#include <garcon/garcon.h>

#ifndef DEBUG_ALLOC_BOX
#if 1
#define DEBUG_ALLOC_BOX(b) g_message("%s: %s=%.0f,%.0f - %.0f,%.0f [%.2fx%.2f]", __func__, #b, b->x1, b->y1, b->x2, b->y2, b->x2-b->x1, b->y2-b->y1);
#else
#define DEBUG_ALLOC_BOX(b)
#endif
#endif

G_BEGIN_DECLS

WnckWindow* xfdashboard_getAppWindow();

GarconMenu* xfdashboard_getApplicationMenu();

G_END_DECLS

#endif
