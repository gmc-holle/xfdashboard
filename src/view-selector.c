/*
 * view-selector: A selector for registered views
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

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <glib/gi18n-lib.h>

#include "view-selector.h"
#include "view.h"
#include "button.h"

/* Define this class in GObject system */
G_DEFINE_TYPE(XfdashboardViewSelector,
				xfdashboard_view_selector,
				CLUTTER_TYPE_ACTOR)

/* Private structure - access only by public API if needed */
#define XFDASHBOARD_VIEW_SELECTOR_GET_PRIVATE(obj) \
	(G_TYPE_INSTANCE_GET_PRIVATE((obj), XFDASHBOARD_TYPE_VIEW_SELECTOR, XfdashboardViewSelectorPrivate))

struct _XfdashboardViewSelectorPrivate
{
	/* Properties related */
	gfloat					spacing;
	XfdashboardViewpad		*viewpad;

	/* Instance related */
	ClutterLayoutManager	*layout;
};

/* Properties */
enum
{
	PROP_0,

	PROP_VIEWPAD,
	PROP_SPACING,

	PROP_LAST
};

GParamSpec* XfdashboardViewSelectorProperties[PROP_LAST]={ 0, };

/* IMPLEMENTATION: Private variables and methods */
#define DEFAULT_SPACING				0.0f
#define DEFAULT_BUTTON_STYLE		XFDASHBOARD_STYLE_ICON

/* A view button was clicked to activate it */
void _xfdashboard_view_selector_on_view_button_clicked(XfdashboardViewSelector *self, gpointer inUserData)
{
	g_return_if_fail(XFDASHBOARD_IS_VIEW_SELECTOR(self));
	g_return_if_fail(XFDASHBOARD_IS_BUTTON(inUserData));

	XfdashboardViewSelectorPrivate		*priv=self->priv;
	XfdashboardButton					*button=XFDASHBOARD_BUTTON(inUserData);
	XfdashboardView						*view;

	view=XFDASHBOARD_VIEW(g_object_get_data(G_OBJECT(button), "view"));

	xfdashboard_viewpad_set_active_view(priv->viewpad, view);
}

/* Called when a new view was added to viewpad */
void _xfdashboard_view_selector_on_view_added(XfdashboardViewSelector *self,
												XfdashboardView *inView,
												gpointer inUserData)
{
	g_return_if_fail(XFDASHBOARD_IS_VIEW_SELECTOR(self));
	g_return_if_fail(XFDASHBOARD_IS_VIEW(inView));

	ClutterActor						*button;
	const gchar							*viewName, *viewIcon;

	/* Create button for newly added view */
	viewName=xfdashboard_view_get_name(inView);
	viewIcon=xfdashboard_view_get_icon(inView);

	button=xfdashboard_button_new();
	xfdashboard_button_set_text(XFDASHBOARD_BUTTON(button), viewName);
	xfdashboard_button_set_icon(XFDASHBOARD_BUTTON(button), viewIcon);
	xfdashboard_button_set_style(XFDASHBOARD_BUTTON(button), DEFAULT_BUTTON_STYLE);
	xfdashboard_button_set_sync_icon_size(XFDASHBOARD_BUTTON(button), TRUE);
	g_object_set_data(G_OBJECT(button), "view", inView);

	g_signal_connect_swapped(button, "clicked", G_CALLBACK(_xfdashboard_view_selector_on_view_button_clicked), self);

	/* Add button as child actor */
	clutter_actor_add_child(CLUTTER_ACTOR(self), button);
}

/* Called when a view was removed to viewpad */
void _xfdashboard_view_selector_on_view_removed(XfdashboardViewSelector *self,
												XfdashboardView *inView,
												gpointer inUserData)
{
	g_return_if_fail(XFDASHBOARD_IS_VIEW_SELECTOR(self));

	ClutterActorIter					iter;
	ClutterActor						*child;
	gpointer							view;

	/* Iterate through create views and lookup view of given type */
	clutter_actor_iter_init(&iter, CLUTTER_ACTOR(self));
	while(clutter_actor_iter_next(&iter, &child))
	{
		/* Check if child is a button otherwise continue iterating */
		if(XFDASHBOARD_IS_BUTTON(child)!=TRUE) continue;

		/* If button has reference to view destroy it */
		view=g_object_get_data(G_OBJECT(child), "view");
		if(XFDASHBOARD_IS_VIEW(view) && XFDASHBOARD_VIEW(view)==inView)
		{
			clutter_actor_destroy(child);
		}
	}
}

/* IMPLEMENTATION: GObject */

/* Dispose this object */
void _xfdashboard_view_selector_dispose(GObject *inObject)
{
	XfdashboardViewSelector			*self=XFDASHBOARD_VIEW_SELECTOR(inObject);
	XfdashboardViewSelectorPrivate	*priv=self->priv;

	/* Release allocated resources */
	if(priv->viewpad)
	{
		g_signal_handlers_disconnect_by_data(priv->viewpad, self);
		g_object_unref(priv->viewpad);
		priv->viewpad=NULL;
	}

	/* Call parent's class dispose method */
	G_OBJECT_CLASS(xfdashboard_view_selector_parent_class)->dispose(inObject);
}

/* Set/get properties */
void _xfdashboard_view_selector_set_property(GObject *inObject,
												guint inPropID,
												const GValue *inValue,
												GParamSpec *inSpec)
{
	XfdashboardViewSelector		*self=XFDASHBOARD_VIEW_SELECTOR(inObject);
	
	switch(inPropID)
	{
		case PROP_VIEWPAD:
			xfdashboard_view_selector_set_viewpad(self, XFDASHBOARD_VIEWPAD(g_value_get_object(inValue)));
			break;

		case PROP_SPACING:
			xfdashboard_view_selector_set_spacing(self, g_value_get_float(inValue));
			break;

		default:
			G_OBJECT_WARN_INVALID_PROPERTY_ID(inObject, inPropID, inSpec);
			break;
	}
}

void _xfdashboard_view_selector_get_property(GObject *inObject,
										guint inPropID,
										GValue *outValue,
										GParamSpec *inSpec)
{
	XfdashboardViewSelector		*self=XFDASHBOARD_VIEW_SELECTOR(inObject);

	switch(inPropID)
	{
		case PROP_VIEWPAD:
			g_value_set_object(outValue, self->priv->viewpad);
			break;

		case PROP_SPACING:
			g_value_set_float(outValue, self->priv->spacing);
			break;

		default:
			G_OBJECT_WARN_INVALID_PROPERTY_ID(inObject, inPropID, inSpec);
			break;
	}
}

/* Class initialization
 * Override functions in parent classes and define properties
 * and signals
 */
void xfdashboard_view_selector_class_init(XfdashboardViewSelectorClass *klass)
{
	GObjectClass			*gobjectClass=G_OBJECT_CLASS(klass);

	/* Override functions */
	gobjectClass->set_property=_xfdashboard_view_selector_set_property;
	gobjectClass->get_property=_xfdashboard_view_selector_get_property;
	gobjectClass->dispose=_xfdashboard_view_selector_dispose;

	/* Set up private structure */
	g_type_class_add_private(klass, sizeof(XfdashboardViewSelectorPrivate));

	/* Define properties */
	XfdashboardViewSelectorProperties[PROP_VIEWPAD]=
		g_param_spec_object("viewpad",
								_("Viewpad"),
								_("The viewpad this selector belongs to"),
								XFDASHBOARD_TYPE_VIEWPAD,
								G_PARAM_READWRITE);

	XfdashboardViewSelectorProperties[PROP_SPACING]=
		g_param_spec_float("spacing",
							_("Spacing"),
							_("The spacing between views and scrollbars"),
							0.0f, G_MAXFLOAT,
							DEFAULT_SPACING,
							G_PARAM_READWRITE);

	g_object_class_install_properties(gobjectClass, PROP_LAST, XfdashboardViewSelectorProperties);
}

/* Object initialization
 * Create private structure and set up default values
 */
void xfdashboard_view_selector_init(XfdashboardViewSelector *self)
{
	XfdashboardViewSelectorPrivate		*priv;

	priv=self->priv=XFDASHBOARD_VIEW_SELECTOR_GET_PRIVATE(self);

	/* Set up default values */
	priv->viewpad=NULL;
	priv->spacing=DEFAULT_SPACING;
	priv->layout=clutter_box_layout_new();

	/* Set up actor */
	clutter_actor_set_reactive(CLUTTER_ACTOR(self), TRUE);
	clutter_actor_set_layout_manager(CLUTTER_ACTOR(self), priv->layout);
	// TODO: if(priv->orientation==CLUTTER_ORIENTATION_HORIZONTAL) clutter_box_layout_set_vertical(CLUTTER_BOX_LAYOUT(priv->layout), FALSE);
		// TODO: else clutter_box_layout_set_vertical(CLUTTER_BOX_LAYOUT(priv->layout), TRUE);
}

/* Implementation: Public API */

/* Create new instance */
ClutterActor* xfdashboard_view_selector_new(void)
{
	return(CLUTTER_ACTOR(g_object_new(XFDASHBOARD_TYPE_VIEW_SELECTOR, NULL)));
}

ClutterActor* xfdashboard_view_selector_new_for_viewpad(XfdashboardViewpad *inViewpad)
{
	return(CLUTTER_ACTOR(g_object_new(XFDASHBOARD_TYPE_VIEW_SELECTOR,
										"viewpad", inViewpad,
										NULL)));
}

/* Get/set viewpad */
XfdashboardViewpad* xfdashboard_view_selector_get_viewpad(XfdashboardViewSelector *self)
{
	g_return_val_if_fail(XFDASHBOARD_IS_VIEW_SELECTOR(self), NULL);

	return(self->priv->viewpad);
}

void xfdashboard_view_selector_set_viewpad(XfdashboardViewSelector *self, XfdashboardViewpad *inViewpad)
{
	g_return_if_fail(XFDASHBOARD_IS_VIEW_SELECTOR(self));
	g_return_if_fail(XFDASHBOARD_IS_VIEWPAD(inViewpad));

	XfdashboardViewSelectorPrivate		*priv=self->priv;
	GList								*views, *entry;

	/* Only set new value if it differs from current value */
	if(priv->viewpad==inViewpad) return;

	/* Release old viewpad if available */
	if(priv->viewpad)
	{
		/* Destroy all children */
		clutter_actor_destroy_all_children(CLUTTER_ACTOR(self));

		/* Release old viewpad */
		g_signal_handlers_disconnect_by_data(priv->viewpad, self);
		g_object_unref(priv->viewpad);
		priv->viewpad=NULL;
	}

	/* Set new value */
	priv->viewpad=XFDASHBOARD_VIEWPAD(g_object_ref(inViewpad));
	g_signal_connect_swapped(priv->viewpad, "view-added", G_CALLBACK(_xfdashboard_view_selector_on_view_added), self);
	g_signal_connect_swapped(priv->viewpad, "view-removed", G_CALLBACK(_xfdashboard_view_selector_on_view_removed), self);

	/* Create instance of each registered view type and add it to this actor
	 * and connect signals
	 */
	views=xfdashboard_viewpad_get_views(priv->viewpad);
	for(entry=views; entry; entry=g_list_next(entry))
	{
		_xfdashboard_view_selector_on_view_added(self, XFDASHBOARD_VIEW(entry->data), NULL);
	}
	g_list_free(views);

	/* Notify about property change */
	g_object_notify_by_pspec(G_OBJECT(self), XfdashboardViewSelectorProperties[PROP_VIEWPAD]);
}

/* Get/set spacing */
gfloat xfdashboard_view_selector_get_spacing(XfdashboardViewSelector *self)
{
	g_return_val_if_fail(XFDASHBOARD_IS_VIEW_SELECTOR(self), DEFAULT_SPACING);

	return(self->priv->spacing);
}

void xfdashboard_view_selector_set_spacing(XfdashboardViewSelector *self, gfloat inSpacing)
{
	g_return_if_fail(XFDASHBOARD_IS_VIEW_SELECTOR(self));
	g_return_if_fail(inSpacing>=0.0f);

	XfdashboardViewSelectorPrivate	*priv=self->priv;

	/* Only set value if it changes */
	if(inSpacing==priv->spacing) return;

	/* Set new value */
	priv->spacing=inSpacing;
	if(priv->layout) clutter_box_layout_set_spacing(CLUTTER_BOX_LAYOUT(priv->layout), priv->spacing);
	clutter_actor_queue_relayout(CLUTTER_ACTOR(self));

	/* Notify about property change */
	g_object_notify_by_pspec(G_OBJECT(self), XfdashboardViewSelectorProperties[PROP_SPACING]);
}
