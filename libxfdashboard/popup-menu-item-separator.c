/*
 * popup-menu-item-separator: A separator menu item
 * 
 * Copyright 2012-2016 Stephan Haller <nomad@froevel.de>
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

#include <libxfdashboard/popup-menu-item-separator.h>

#include <glib/gi18n-lib.h>

#include <libxfdashboard/popup-menu-item.h>
#include <libxfdashboard/click-action.h>
#include <libxfdashboard/compat.h>


/* Define this class in GObject system */
static void _xfdashboard_popup_menu_item_separator_popup_menu_item_iface_init(XfdashboardPopupMenuItemInterface *iface);

G_DEFINE_TYPE_WITH_CODE(XfdashboardPopupMenuItemSeparator,
						xfdashboard_popup_menu_item_separator,
						XFDASHBOARD_TYPE_ACTOR,
						G_IMPLEMENT_INTERFACE(XFDASHBOARD_TYPE_POPUP_MENU_ITEM, _xfdashboard_popup_menu_item_separator_popup_menu_item_iface_init))

/* Private structure - access only by public API if needed */
#define XFDASHBOARD_BUTTON_GET_PRIVATE(obj) \
	(G_TYPE_INSTANCE_GET_PRIVATE((obj), XFDASHBOARD_TYPE_POPUP_MENU_ITEM_SEPARATOR, XfdashboardPopupMenuItemSeparatorPrivate))

struct _XfdashboardPopupMenuItemSeparatorPrivate
{
	/* Instance related */
	gint				minHeight;
};

/* IMPLEMENTATION: Private variables and methods */


/* IMPLEMENTATION: Interface XfdashboardPopupMenuItem */

/* Interface initialization
 * Set up default functions
 */
void _xfdashboard_popup_menu_item_separator_popup_menu_item_iface_init(XfdashboardPopupMenuItemInterface *iface)
{
}

/* IMPLEMENTATION: ClutterActor */

/* Get preferred width/height */
static void _xfdashboard_popup_menu_item_separator_get_preferred_height(ClutterActor *inActor,
																		gfloat inForWidth,
																		gfloat *outMinHeight,
																		gfloat *outNaturalHeight)
{
	XfdashboardPopupMenuItemSeparator			*self=XFDASHBOARD_POPUP_MENU_ITEM_SEPARATOR(inActor);
	XfdashboardPopupMenuItemSeparatorPrivate	*priv=self->priv;
	gfloat										minHeight, naturalHeight;

	minHeight=naturalHeight=priv->minHeight;

	/* Store sizes computed */
	if(outMinHeight) *outMinHeight=minHeight;
	if(outNaturalHeight) *outNaturalHeight=naturalHeight;
}

static void _xfdashboard_popup_menu_item_separator_get_preferred_width(ClutterActor *inActor,
																		gfloat inForHeight,
																		gfloat *outMinWidth,
																		gfloat *outNaturalWidth)
{
	XfdashboardPopupMenuItemSeparator			*self=XFDASHBOARD_POPUP_MENU_ITEM_SEPARATOR(inActor);
	XfdashboardPopupMenuItemSeparatorPrivate	*priv=self->priv;
	gfloat										minWidth, naturalWidth;

	minWidth=naturalWidth=0.0f;

	/* Store sizes computed */
	if(outMinWidth) *outMinWidth=minWidth;
	if(outNaturalWidth) *outNaturalWidth=naturalWidth;
}

/* IMPLEMENTATION: GObject */

/* Class initialization
 * Override functions in parent classes and define properties
 * and signals
 */
static void xfdashboard_popup_menu_item_separator_class_init(XfdashboardPopupMenuItemSeparatorClass *klass)
{
	ClutterActorClass		*clutterActorClass=CLUTTER_ACTOR_CLASS(klass);

	/* Override functions */
	clutterActorClass->get_preferred_width=_xfdashboard_popup_menu_item_separator_get_preferred_width;
	clutterActorClass->get_preferred_height=_xfdashboard_popup_menu_item_separator_get_preferred_height;

	/* Set up private structure */
	g_type_class_add_private(klass, sizeof(XfdashboardPopupMenuItemSeparatorPrivate));
}

/* Object initialization
 * Create private structure and set up default values
 */
static void xfdashboard_popup_menu_item_separator_init(XfdashboardPopupMenuItemSeparator *self)
{
	XfdashboardPopupMenuItemSeparatorPrivate	*priv;

	priv=self->priv=XFDASHBOARD_BUTTON_GET_PRIVATE(self);

	/* Set up default values */
	priv->minHeight=4.0f;

	/* This actor cannot reacts on events */
	clutter_actor_set_reactive(CLUTTER_ACTOR(self), FALSE);
}

/* IMPLEMENTATION: Public API */

/* Create new actor */
ClutterActor* xfdashboard_popup_menu_item_separator_new(void)
{
	return(g_object_new(XFDASHBOARD_TYPE_POPUP_MENU_ITEM_SEPARATOR,
						NULL));
}
