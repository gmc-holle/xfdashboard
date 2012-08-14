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

#ifndef WNCK_CHECK_VERSION
/* If macro WNCK_CHECK_VERSION is missing we have a
 * version prior 3.0 and we need the version given in
 * WNCK_MAJOR_VERSION, WNCK_MINOR_VERSION, WNCK_MICRO_VERSION
 * and provide our own WNCK_CHECK_VERSION macto
 */
#ifndef WNCK_MAJOR_VERSION
#error "WNCK_MAJOR_VERSION must be set"
#endif

#ifndef WNCK_MINOR_VERSION
#error "WNCK_MINOR_VERSION must be set"
#endif

#ifndef WNCK_MICRO_VERSION
#error "WNCK_MICRO_VERSION must be set"
#endif

#define WNCK_CHECK_VERSION(major,minor,micro)                           \
    (WNCK_MAJOR_VERSION > (major) ||                                    \
     (WNCK_MAJOR_VERSION == (major) && WNCK_MINOR_VERSION > (minor)) || \
     (WNCK_MAJOR_VERSION == (major) && WNCK_MINOR_VERSION == (minor) && \
      WNCK_MICRO_VERSION >= (micro)))
#endif

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
