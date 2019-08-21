/*
 * view-selector: A selector for registered views
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

#ifndef __LIBXFDASHBOARD_VIEW_SELECTOR__
#define __LIBXFDASHBOARD_VIEW_SELECTOR__

#if !defined(__LIBXFDASHBOARD_H_INSIDE__) && !defined(LIBXFDASHBOARD_COMPILATION)
#error "Only <libxfdashboard/libxfdashboard.h> can be included directly."
#endif

#include <clutter/clutter.h>

#include <libxfdashboard/actor.h>
#include <libxfdashboard/viewpad.h>
#include <libxfdashboard/toggle-button.h>

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

/**
 * XfdashboardViewSelector:
 *
 * The #XfdashboardViewSelector structure contains only private data and
 * should be accessed using the provided API
 */
struct _XfdashboardViewSelector
{
	/*< private >*/
	/* Parent instance */
	XfdashboardActor				parent_instance;

	/* Private structure */
	XfdashboardViewSelectorPrivate	*priv;
};

/**
 * XfdashboardViewSelectorClass:
 * @state_changed: Class handler for the #XfdashboardViewSelectorClass::state_changed signal
 *
 * The #XfdashboardViewSelectorClass structure contains only private data
 */
struct _XfdashboardViewSelectorClass
{
	/*< private >*/
	/* Parent class */
	XfdashboardActorClass			parent_class;

	/*< public >*/
	/* Virtual functions */
	void (*state_changed)(XfdashboardViewSelector *self, XfdashboardToggleButton *inButton);
};

/* Public API */
GType xfdashboard_view_selector_get_type(void) G_GNUC_CONST;

ClutterActor* xfdashboard_view_selector_new(void);
ClutterActor* xfdashboard_view_selector_new_for_viewpad(XfdashboardViewpad *inViewpad);

XfdashboardViewpad* xfdashboard_view_selector_get_viewpad(XfdashboardViewSelector *self);
void xfdashboard_view_selector_set_viewpad(XfdashboardViewSelector *self, XfdashboardViewpad *inViewpad);

gfloat xfdashboard_view_selector_get_spacing(XfdashboardViewSelector *self);
void xfdashboard_view_selector_set_spacing(XfdashboardViewSelector *self, gfloat inSpacing);

ClutterOrientation xfdashboard_view_selector_get_orientation(XfdashboardViewSelector *self);
void xfdashboard_view_selector_set_orientation(XfdashboardViewSelector *self, ClutterOrientation inOrientation);

G_END_DECLS

#endif	/* __LIBXFDASHBOARD_VIEW_SELECTOR__ */
