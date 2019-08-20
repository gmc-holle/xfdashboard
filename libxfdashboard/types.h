/*
 * types: Define application specific but common types
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
 * SECTION:types
 * @title: Enums and types
 * @short_description: Common enums and types
 * @include: xfdashboard/types.h
 *
 * Various miscellaneous utilility functions.
 */

#ifndef __LIBXFDASHBOARD_TYPES__
#define __LIBXFDASHBOARD_TYPES__

#if !defined(__LIBXFDASHBOARD_H_INSIDE__) && !defined(LIBXFDASHBOARD_COMPILATION)
#error "Only <libxfdashboard/libxfdashboard.h> can be included directly."
#endif

#include <glib.h>

G_BEGIN_DECLS

/**
 * XfdashboardViewMode:
 * @XFDASHBOARD_VIEW_MODE_LIST: Show items in view as list
 * @XFDASHBOARD_VIEW_MODE_ICON: Show items in view as icons
 *
 * Determines how to display items of a view.
 */
typedef enum /*< prefix=XFDASHBOARD_VIEW_MODE >*/
{
	XFDASHBOARD_VIEW_MODE_LIST=0,
	XFDASHBOARD_VIEW_MODE_ICON
} XfdashboardViewMode;

/**
 * XfdashboardVisibilityPolicy:
 * @XFDASHBOARD_VISIBILITY_POLICY_NEVER: The actor is always visible.
 * @XFDASHBOARD_VISIBILITY_POLICY_AUTOMATIC: The actor will appear and disappear as necessary. For example, when a view does not fit into viewpad the scrollbar will be visible.
 * @XFDASHBOARD_VISIBILITY_POLICY_ALWAYS: The actor will never appear.
 *
 * Determines when an actor will be visible, e.g. scrollbars in views.
 */
typedef enum /*< prefix=XFDASHBOARD_VISIBILITY_POLICY >*/
{
	XFDASHBOARD_VISIBILITY_POLICY_NEVER=0,
	XFDASHBOARD_VISIBILITY_POLICY_AUTOMATIC,
	XFDASHBOARD_VISIBILITY_POLICY_ALWAYS
} XfdashboardVisibilityPolicy;

/**
 * XfdashboardOrientation:
 * @XFDASHBOARD_ORIENTATION_LEFT: The actor is justified to left boundary.
 * @XFDASHBOARD_ORIENTATION_RIGHT: The actor is justified to right boundary.
 * @XFDASHBOARD_ORIENTATION_TOP: The actor is justified to top boundary.
 * @XFDASHBOARD_ORIENTATION_BOTTOM: The actor is justified to bottom boundary.
 *
 * Determines the side to which an actor is justified to. It can mostly be switched on-the-fly.
 */
typedef enum /*< prefix=XFDASHBOARD_ORIENTATION >*/
{
	XFDASHBOARD_ORIENTATION_LEFT=0,
	XFDASHBOARD_ORIENTATION_RIGHT,
	XFDASHBOARD_ORIENTATION_TOP,
	XFDASHBOARD_ORIENTATION_BOTTOM
} XfdashboardOrientation;

/**
 * XfdashboardCorners:
 * @XFDASHBOARD_CORNERS_NONE: No corner is affected.
 * @XFDASHBOARD_CORNERS_TOP_LEFT: Affects top-left corner of actor.
 * @XFDASHBOARD_CORNERS_TOP_RIGHT: Affects top-right corner of actor.
 * @XFDASHBOARD_CORNERS_BOTTOM_LEFT: Affects bottom-left corner of actor.
 * @XFDASHBOARD_CORNERS_BOTTOM_RIGHT: Affects bottom-right corner of actor.
 * @XFDASHBOARD_CORNERS_TOP: Affects corners at top side of actor - top-left and top-right.
 * @XFDASHBOARD_CORNERS_BOTTOM: Affects corners at bottom side of actor - bottom-left and bottom-right.
 * @XFDASHBOARD_CORNERS_LEFT: Affects corners at left side of actor - top-left and bottom-left.
 * @XFDASHBOARD_CORNERS_RIGHT: Affects corners at right side of actor - top-right and bottom-right.
 * @XFDASHBOARD_CORNERS_ALL: Affects all corners of actor.
 *
 * Specifies which corner of an actor is affected, e.g. used in background for rounded rectangles.
 */
typedef enum /*< flags,prefix=XFDASHBOARD_CORNERS >*/
{
	XFDASHBOARD_CORNERS_NONE=0,

	XFDASHBOARD_CORNERS_TOP_LEFT=1 << 0,
	XFDASHBOARD_CORNERS_TOP_RIGHT=1 << 1,
	XFDASHBOARD_CORNERS_BOTTOM_LEFT=1 << 2,
	XFDASHBOARD_CORNERS_BOTTOM_RIGHT=1 << 3,

	XFDASHBOARD_CORNERS_TOP=(XFDASHBOARD_CORNERS_TOP_LEFT | XFDASHBOARD_CORNERS_TOP_RIGHT),
	XFDASHBOARD_CORNERS_BOTTOM=(XFDASHBOARD_CORNERS_BOTTOM_LEFT | XFDASHBOARD_CORNERS_BOTTOM_RIGHT),
	XFDASHBOARD_CORNERS_LEFT=(XFDASHBOARD_CORNERS_TOP_LEFT | XFDASHBOARD_CORNERS_BOTTOM_LEFT),
	XFDASHBOARD_CORNERS_RIGHT=(XFDASHBOARD_CORNERS_TOP_RIGHT | XFDASHBOARD_CORNERS_BOTTOM_RIGHT),

	XFDASHBOARD_CORNERS_ALL=(XFDASHBOARD_CORNERS_TOP_LEFT | XFDASHBOARD_CORNERS_TOP_RIGHT | XFDASHBOARD_CORNERS_BOTTOM_LEFT | XFDASHBOARD_CORNERS_BOTTOM_RIGHT)
} XfdashboardCorners;

/**
 * XfdashboardBorders:
 * @XFDASHBOARD_BORDERS_NONE: No side is affected.
 * @XFDASHBOARD_BORDERS_LEFT: Affects left side of actor.
 * @XFDASHBOARD_BORDERS_TOP: Affects top side of actor.
 * @XFDASHBOARD_BORDERS_RIGHT: Affects right side of actor.
 * @XFDASHBOARD_BORDERS_BOTTOM: Affects bottom side of actor.
 * @XFDASHBOARD_BORDERS_ALL: Affects all sides of actor.
 *
 * Determines which side of an actor is affected, e.g. used in outlines.
 */
typedef enum /*< flags,prefix=XFDASHBOARD_BORDERS >*/
{
	XFDASHBOARD_BORDERS_NONE=0,

	XFDASHBOARD_BORDERS_LEFT=1 << 0,
	XFDASHBOARD_BORDERS_TOP=1 << 1,
	XFDASHBOARD_BORDERS_RIGHT=1 << 2,
	XFDASHBOARD_BORDERS_BOTTOM=1 << 3,

	XFDASHBOARD_BORDERS_ALL=(XFDASHBOARD_BORDERS_LEFT | XFDASHBOARD_BORDERS_TOP | XFDASHBOARD_BORDERS_RIGHT | XFDASHBOARD_BORDERS_BOTTOM)
} XfdashboardBorders;

/**
 * XfdashboardStageBackgroundImageType:
 * @XFDASHBOARD_STAGE_BACKGROUND_IMAGE_TYPE_NONE: Do not show anything at background of stage actor.
 * @XFDASHBOARD_STAGE_BACKGROUND_IMAGE_TYPE_DESKTOP: Show current desktop image at background of stage actor.
 *
 * Determine what to show at background of a stage actor.
 */
typedef enum /*< prefix=XFDASHBOARD_STAGE_BACKGROUND_IMAGE_TYPE >*/
{
	XFDASHBOARD_STAGE_BACKGROUND_IMAGE_TYPE_NONE=0,
	XFDASHBOARD_STAGE_BACKGROUND_IMAGE_TYPE_DESKTOP
} XfdashboardStageBackgroundImageType;

/**
 * XfdashboardSelectionTarget:
 * @XFDASHBOARD_SELECTION_TARGET_LEFT: Move to next selectable actor at left side.
 * @XFDASHBOARD_SELECTION_TARGET_RIGHT: Move to next selectable actor at right side.
 * @XFDASHBOARD_SELECTION_TARGET_UP: Move to next selectable actor at top side.
 * @XFDASHBOARD_SELECTION_TARGET_DOWN: Move to next selectable actor at bottom side.
 * @XFDASHBOARD_SELECTION_TARGET_FIRST: Move to first selectable actor.
 * @XFDASHBOARD_SELECTION_TARGET_LAST: Move to last selectable actor.
 * @XFDASHBOARD_SELECTION_TARGET_PAGE_LEFT: Move to next selectable actor at left side page-width.
 * @XFDASHBOARD_SELECTION_TARGET_PAGE_RIGHT: Move to next selectable actor at right side page-width.
 * @XFDASHBOARD_SELECTION_TARGET_PAGE_UP: Move to next selectable actor at top side page-width.
 * @XFDASHBOARD_SELECTION_TARGET_PAGE_DOWN: Move to next selectable actor at bottom side page-width.
 * @XFDASHBOARD_SELECTION_TARGET_NEXT: Move to next selectable actor to current one.
 *
 * Determines the movement of selection within an actor which supports selections.
 */
typedef enum /*< prefix=XFDASHBOARD_SELECTION_TARGET >*/
{
	XFDASHBOARD_SELECTION_TARGET_LEFT=0,
	XFDASHBOARD_SELECTION_TARGET_RIGHT,
	XFDASHBOARD_SELECTION_TARGET_UP,
	XFDASHBOARD_SELECTION_TARGET_DOWN,

	XFDASHBOARD_SELECTION_TARGET_FIRST,
	XFDASHBOARD_SELECTION_TARGET_LAST,

	XFDASHBOARD_SELECTION_TARGET_PAGE_LEFT,
	XFDASHBOARD_SELECTION_TARGET_PAGE_RIGHT,
	XFDASHBOARD_SELECTION_TARGET_PAGE_UP,
	XFDASHBOARD_SELECTION_TARGET_PAGE_DOWN,

	XFDASHBOARD_SELECTION_TARGET_NEXT
} XfdashboardSelectionTarget;

/* Anchor points */
/**
 * XfdashboardAnchorPoint:
 * @XFDASHBOARD_ANCHOR_POINT_NONE: Use default anchor of actor, usually top-left.
 * @XFDASHBOARD_ANCHOR_POINT_NORTH_WEST: The anchor is at the top-left of the object. 
 * @XFDASHBOARD_ANCHOR_POINT_NORTH: The anchor is at the top of the object, centered horizontally. 
 * @XFDASHBOARD_ANCHOR_POINT_NORTH_EAST: The anchor is at the top-right of the object. 
 * @XFDASHBOARD_ANCHOR_POINT_EAST: The anchor is on the right of the object, centered vertically. 
 * @XFDASHBOARD_ANCHOR_POINT_SOUTH_EAST:  The anchor is at the bottom-right of the object. 
 * @XFDASHBOARD_ANCHOR_POINT_SOUTH: The anchor is at the bottom of the object, centered horizontally. 
 * @XFDASHBOARD_ANCHOR_POINT_SOUTH_WEST:  The anchor is at the bottom-left of the object. 
 * @XFDASHBOARD_ANCHOR_POINT_WEST: The anchor is on the left of the object, centered vertically. 
 * @XFDASHBOARD_ANCHOR_POINT_CENTER: The anchor is in the center of the object. 
 * 
 * Specifys the position of an object relative to a particular anchor point.
 */
typedef enum /*< prefix=XFDASHBOARD_ANCHOR_POINT >*/
{
	XFDASHBOARD_ANCHOR_POINT_NONE=0,
	XFDASHBOARD_ANCHOR_POINT_NORTH_WEST,
	XFDASHBOARD_ANCHOR_POINT_NORTH,
	XFDASHBOARD_ANCHOR_POINT_NORTH_EAST,
	XFDASHBOARD_ANCHOR_POINT_EAST,
	XFDASHBOARD_ANCHOR_POINT_SOUTH_EAST,
	XFDASHBOARD_ANCHOR_POINT_SOUTH,
	XFDASHBOARD_ANCHOR_POINT_SOUTH_WEST,
	XFDASHBOARD_ANCHOR_POINT_WEST,
	XFDASHBOARD_ANCHOR_POINT_CENTER
} XfdashboardAnchorPoint;

G_END_DECLS

#endif	/* __LIBXFDASHBOARD_TYPES__ */
