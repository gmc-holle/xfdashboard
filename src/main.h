/*
 * main.h: Common functions and shared data
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

#ifndef XFOVERVIEW_MAIN

#define WNCK_I_KNOW_THIS_IS_UNSTABLE
#include <libwnck/libwnck.h>

#include <clutter/clutter.h>
#include <clutter/x11/clutter-x11.h>
#include <clutter/x11/clutter-x11-texture-pixmap.h>


/* Get window of stage */
extern WnckWindow* xfoverview_getStageWindow();


/* SCREEN AND WINDOWS */
extern WnckScreen		*screen;
extern WnckWorkspace	*workspace;
extern GList			*windows;

/* APPLICATION WINDOW */
extern ClutterActor		*stage;
extern ClutterActor		*actor;

#endif
