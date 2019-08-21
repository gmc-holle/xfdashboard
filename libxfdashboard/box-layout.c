/*
 * box-layout: A ClutterBoxLayout derived layout manager disregarding
 *             text direction and enforcing left-to-right layout in
 *             horizontal orientation
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
 * SECTION:box-layout
 * @short_description: A layout manager arranging children on a single line
 * @include: xfdashboard/box-layout.h
 *
 * The #XfdashboardBoxLayout layout manager is a #ClutterBoxLayout derived layout
 * manager arranging children on a single line but disregards any text direction.
 *
 * It behaves like #ClutterBoxLayout but it enforces a left-to-right layout of
 * all children when set to horizontal orientation. This is the difference of the
 * #XfdashboardBoxLayout layout manager to the #ClutterBoxLayout layout manager.
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <libxfdashboard/box-layout.h>

#include <glib/gi18n-lib.h>
#include <clutter/clutter.h>
#include <math.h>

#include <libxfdashboard/compat.h>


/* Define this class in GObject system */
G_DEFINE_TYPE(XfdashboardBoxLayout,
				xfdashboard_box_layout,
				CLUTTER_TYPE_BOX_LAYOUT)


/* IMPLEMENTATION: ClutterLayoutManager */

/* Re-layout and allocate children of container we manage */
static void _xfdashboard_box_layout_allocate(ClutterLayoutManager *inLayoutManager,
													ClutterContainer *inContainer,
													const ClutterActorBox *inAllocation,
													ClutterAllocationFlags inFlags)
{
	ClutterTextDirection				textDirection;
	ClutterActor						*child;
	ClutterActorIter					iter;
	ClutterActorBox						childBox;
	gfloat								containerWidth;

	g_return_if_fail(XFDASHBOARD_IS_BOX_LAYOUT(inLayoutManager));
	g_return_if_fail(CLUTTER_IS_CONTAINER(inContainer));


	/* Chain up to calculate and store the allocation of children */
	CLUTTER_LAYOUT_MANAGER_CLASS(xfdashboard_box_layout_parent_class)->allocate(inLayoutManager,
																				inContainer,
																				inAllocation,
																				inFlags);

	/* Right-to-left text direction only affects horizontal orientation.
	 * If orientation is not horizontal or text direction is not right-to-left
	 * then there is nothing to do.
	 */
	if(clutter_box_layout_get_orientation(CLUTTER_BOX_LAYOUT(inLayoutManager))!=CLUTTER_ORIENTATION_HORIZONTAL)
	{
		return;
	}

	textDirection=clutter_actor_get_text_direction(CLUTTER_ACTOR(inContainer));
	if(textDirection==CLUTTER_TEXT_DIRECTION_DEFAULT) textDirection=clutter_get_default_text_direction();
	if(textDirection!=CLUTTER_TEXT_DIRECTION_RTL)
	{
		return;
	}

	/* Iterate through children and recalculate x-coordination of each
	 * children allocation by "mirroring" x-coordinate.
	 */
	containerWidth=clutter_actor_box_get_width(inAllocation);

	clutter_actor_iter_init(&iter, CLUTTER_ACTOR(inContainer));
	while(clutter_actor_iter_next(&iter, &child))
	{
		gfloat							x1, x2;

		/* Get position and size of child */
		clutter_actor_get_allocation_box(child, &childBox);

		/* Set new allocation of child */
		x1=containerWidth-childBox.x2;
		x2=containerWidth-childBox.x1;

		childBox.x1=x1;
		childBox.x2=x2;

		clutter_actor_allocate(child, &childBox, inFlags);
	}
}

/* IMPLEMENTATION: GObject */

/* Class initialization
 * Override functions in parent classes and define properties
 * and signals
 */
static void xfdashboard_box_layout_class_init(XfdashboardBoxLayoutClass *klass)
{
	ClutterLayoutManagerClass	*layoutClass=CLUTTER_LAYOUT_MANAGER_CLASS(klass);

	/* Override functions */
	layoutClass->allocate=_xfdashboard_box_layout_allocate;
}

/* Object initialization
 * Create private structure and set up default values
 */
static void xfdashboard_box_layout_init(XfdashboardBoxLayout *self)
{
}

/* IMPLEMENTATION: Public API */

/* Create new instance */
/**
 * xfdashboard_box_layout_new:
 *
 * Creates a new #XfdashboardBoxLayout layout manager
 *
 * Return value: The newly created #XfdashboardBoxLayout
 */
ClutterLayoutManager* xfdashboard_box_layout_new(void)
{
	return(CLUTTER_LAYOUT_MANAGER(g_object_new(XFDASHBOARD_TYPE_BOX_LAYOUT, NULL)));
}
