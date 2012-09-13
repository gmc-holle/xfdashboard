/*
 * view-selector.h: A selector for views in viewpad as buttons
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

#ifndef __XFDASHBOARD_VIEW_SELECTOR__
#define __XFDASHBOARD_VIEW_SELECTOR__

#include <clutter/clutter.h>

#include "viewpad.h"

G_BEGIN_DECLS

#define XFDASHBOARD_TYPE_VIEW_SELECTOR				(xfdashboard_view_selector_get_type())
#define XFDASHBOARD_VIEW_SELECTOR(obj)				(G_TYPE_CHECK_INSTANCE_CAST((obj), XFDASHBOARD_TYPE_VIEW_SELECTOR, XfdashboardViewSelector))
#define XFDASHBOARD_IS_VIEW_SELECTOR(obj)			(G_TYPE_CHECK_INSTANCE_TYPE((obj), XFDASHBOARD_TYPE_VIEW_SELECTOR))
#define XFDASHBOARD_VIEW_SELECTOR_CLASS(klass)		(G_TYPE_CHECK_CLASS_CAST((klass), XFDASHBOARD_TYPE_VIEW_SELECTOR, XfdashboardViewSelectorClass))
#define XFDASHBOARD_IS_VIEW_SELECTOR_CLASS(klass)	(G_TYPE_CHECK_CLASS_TYPE((klass), XFDASHBOARD_TYPE_VIEW_SELECTOR))
#define XFDASHBOARD_VIEW_SELECTOR_GET_CLASS(obj)	(G_TYPE_INSTANCE_GET_CLASS((obj), XFDASHBOARD_TYPE_VIEW_SELECTOR, XfdashboardViewSelectorClass))

typedef struct _XfdashboardViewSelector				XfdashboardViewSelector; 
typedef struct _XfdashboardViewSelectorPrivate		XfdashboardViewSelectorPrivate;
typedef struct _XfdashboardViewSelectorClass		XfdashboardViewSelectorClass;

struct _XfdashboardViewSelector
{
	/* Parent instance */
	ClutterActor					parent_instance;

	/* Private structure */
	XfdashboardViewSelectorPrivate	*priv;
};

struct _XfdashboardViewSelectorClass
{
	/* Parent class */
	ClutterActorClass				parent_class;
};

GType xfdashboard_view_selector_get_type(void) G_GNUC_CONST;

ClutterActor* xfdashboard_view_selector_new(const XfdashboardViewpad *inViewpad);

G_END_DECLS

#endif
