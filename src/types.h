/*
 * types: Define application specific but common types
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

#ifndef __XFOVERVIEW_TYPES__
#define __XFOVERVIEW_TYPES__

#include <glib.h>

G_BEGIN_DECLS

/* Application start-up error codes */
typedef enum /*< skip,prefix=XFDASHBOARD_APPLICATION_ERROR >*/
{
	XFDASHBOARD_APPLICATION_ERROR_NONE=0,
	XFDASHBOARD_APPLICATION_ERROR_FAILED,
	XFDASHBOARD_APPLICATION_ERROR_RESTART,
	XFDASHBOARD_APPLICATION_ERROR_QUIT
} XfdashboardApplicationErrorCode;

/* List mode for views */
typedef enum /*< prefix=XFDASHBOARD_VIEW >*/
{
	XFDASHBOARD_VIEW_LIST=0,
	XFDASHBOARD_VIEW_ICON
} XfdashboardViewMode;

/* Visibility policy (e.g. for scroll bars in views) */
typedef enum /*< prefix=XFDASHBOARD_POLICY >*/
{
	XFDASHBOARD_POLICY_NEVER=0,
	XFDASHBOARD_POLICY_AUTOMATIC,
	XFDASHBOARD_POLICY_ALWAYS
} XfdashboardPolicy;

/* Style (e.g. used in buttons) */
typedef enum /*< prefix=XFDASHBOARD_STYLE >*/
{
	XFDASHBOARD_STYLE_TEXT=0,
	XFDASHBOARD_STYLE_ICON,
	XFDASHBOARD_STYLE_BOTH,
} XfdashboardStyle;

/* Orientation (e.g. used in buttons) */
typedef enum /*< prefix=XFDASHBOARD_ORIENTATION >*/
{
	XFDASHBOARD_ORIENTATION_LEFT=0,
	XFDASHBOARD_ORIENTATION_RIGHT,
	XFDASHBOARD_ORIENTATION_TOP,
	XFDASHBOARD_ORIENTATION_BOTTOM,
} XfdashboardOrientation;

/* Background types */
typedef enum /*< prefix=XFDASHBOARD_BACKGROUND_TYPE >*/
{
	XFDASHBOARD_BACKGROUND_TYPE_NONE=0,
	XFDASHBOARD_BACKGROUND_TYPE_RECTANGLE,
	XFDASHBOARD_BACKGROUND_TYPE_IMAGE,
} XfdashboardBackgroundType;

/* Corners (e.g. used in background for rounded rectangles) */
typedef enum /*< flags,prefix=XFDASHBOARD_CORNERS >*/
{
	XFDASHBOARD_CORNERS_NONE=0,

	XFDASHBOARD_CORNERS_TOP_LEFT=1 << 1,
	XFDASHBOARD_CORNERS_TOP_RIGHT=1 << 2,
	XFDASHBOARD_CORNERS_BOTTOM_LEFT=1 << 3,
	XFDASHBOARD_CORNERS_BOTTOM_RIGHT=1 << 4,

	XFDASHBOARD_CORNERS_TOP=(XFDASHBOARD_CORNERS_TOP_LEFT | XFDASHBOARD_CORNERS_TOP_RIGHT),
	XFDASHBOARD_CORNERS_BOTTOM=(XFDASHBOARD_CORNERS_BOTTOM_LEFT | XFDASHBOARD_CORNERS_BOTTOM_RIGHT),
	XFDASHBOARD_CORNERS_LEFT=(XFDASHBOARD_CORNERS_TOP_LEFT | XFDASHBOARD_CORNERS_BOTTOM_LEFT),
	XFDASHBOARD_CORNERS_RIGHT=(XFDASHBOARD_CORNERS_TOP_RIGHT | XFDASHBOARD_CORNERS_BOTTOM_RIGHT),

	XFDASHBOARD_CORNERS_ALL=(XFDASHBOARD_CORNERS_TOP_LEFT | XFDASHBOARD_CORNERS_TOP_RIGHT | XFDASHBOARD_CORNERS_BOTTOM_LEFT | XFDASHBOARD_CORNERS_BOTTOM_RIGHT),
} XfdashboardCorners;

G_END_DECLS

#endif	/* __XFOVERVIEW_TYPES__ */
