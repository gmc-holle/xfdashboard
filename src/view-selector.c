/*
 * view-selector: A selector for registered views
 * 
 * Copyright 2012-2015 Stephan Haller <nomad@froevel.de>
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

#include "view-selector.h"

#include <glib/gi18n-lib.h>

#include "view.h"
#include "tooltip-action.h"

/* Define this class in GObject system */
G_DEFINE_TYPE(XfdashboardViewSelector,
				xfdashboard_view_selector,
				XFDASHBOARD_TYPE_ACTOR)

/* Private structure - access only by public API if needed */
#define XFDASHBOARD_VIEW_SELECTOR_GET_PRIVATE(obj) \
	(G_TYPE_INSTANCE_GET_PRIVATE((obj), XFDASHBOARD_TYPE_VIEW_SELECTOR, XfdashboardViewSelectorPrivate))

struct _XfdashboardViewSelectorPrivate
{
	/* Properties related */
	gfloat					spacing;
	XfdashboardViewpad		*viewpad;
	ClutterOrientation		orientation;

	/* Instance related */
	ClutterLayoutManager	*layout;
};

/* Properties */
enum
{
	PROP_0,

	PROP_VIEWPAD,
	PROP_SPACING,
	PROP_ORIENTATION,

	PROP_LAST
};

static GParamSpec* XfdashboardViewSelectorProperties[PROP_LAST]={ 0, };

/* Signals */
enum
{
	SIGNAL_STATE_CHANGED,

	SIGNAL_LAST
};

static guint XfdashboardViewSelectorSignals[SIGNAL_LAST]={ 0, };

/* IMPLEMENTATION: Private variables and methods */

/* A view button changed its toggle state */
static void _xfdashboard_view_selector_on_toggle_button_state_changed(XfdashboardViewSelector *self, gpointer inUserData)
{
	XfdashboardToggleButton				*button;

	g_return_if_fail(XFDASHBOARD_IS_VIEW_SELECTOR(self));
	g_return_if_fail(XFDASHBOARD_IS_TOGGLE_BUTTON(inUserData));

	button=XFDASHBOARD_TOGGLE_BUTTON(inUserData);
	g_signal_emit(self, XfdashboardViewSelectorSignals[SIGNAL_STATE_CHANGED], 0, button);
}

/* A view button was clicked to activate it */
static void _xfdashboard_view_selector_on_view_button_clicked(XfdashboardViewSelector *self, gpointer inUserData)
{
	XfdashboardViewSelectorPrivate		*priv;
	XfdashboardToggleButton				*button;
	XfdashboardView						*view;

	g_return_if_fail(XFDASHBOARD_IS_VIEW_SELECTOR(self));
	g_return_if_fail(XFDASHBOARD_IS_BUTTON(inUserData));

	priv=self->priv;
	button=XFDASHBOARD_TOGGLE_BUTTON(inUserData);

	view=XFDASHBOARD_VIEW(g_object_get_data(G_OBJECT(button), "view"));

	xfdashboard_viewpad_set_active_view(priv->viewpad, view);
}

/* Called when a view was enabled or will be disabled */
static void _xfdashboard_view_selector_on_view_enable_state_changed(XfdashboardView *inView, gpointer inUserData)
{
	ClutterActor						*button;

	g_return_if_fail(XFDASHBOARD_IS_VIEW(inView));
	g_return_if_fail(CLUTTER_IS_ACTOR(inUserData));

	button=CLUTTER_ACTOR(inUserData);

	if(!xfdashboard_view_get_enabled(inView)) clutter_actor_hide(button);
		else clutter_actor_show(button);
}

/* Called when a view was activated or deactivated */
static void _xfdashboard_view_selector_on_view_activated(XfdashboardView *inView, gpointer inUserData)
{
	XfdashboardToggleButton				*button;

	g_return_if_fail(XFDASHBOARD_IS_VIEW(inView));
	g_return_if_fail(XFDASHBOARD_IS_TOGGLE_BUTTON(inUserData));

	button=XFDASHBOARD_TOGGLE_BUTTON(inUserData);
	xfdashboard_toggle_button_set_toggle_state(button, TRUE);
}

static void _xfdashboard_view_selector_on_view_deactivated(XfdashboardView *inView, gpointer inUserData)
{
	XfdashboardToggleButton				*button;

	g_return_if_fail(XFDASHBOARD_IS_VIEW(inView));
	g_return_if_fail(XFDASHBOARD_IS_TOGGLE_BUTTON(inUserData));

	button=XFDASHBOARD_TOGGLE_BUTTON(inUserData);
	xfdashboard_toggle_button_set_toggle_state(button, FALSE);
}

/* Called when an icon of a view has changed */
static void _xfdashboard_view_selector_on_view_icon_changed(XfdashboardView *inView, ClutterImage *inImage, gpointer inUserData)
{
	XfdashboardButton					*button;

	g_return_if_fail(XFDASHBOARD_IS_VIEW(inView));
	g_return_if_fail(XFDASHBOARD_IS_TOGGLE_BUTTON(inUserData));

	button=XFDASHBOARD_BUTTON(inUserData);
	xfdashboard_button_set_icon(button, xfdashboard_view_get_icon(inView));
}

/* Called when the name of a view has changed */
static void _xfdashboard_view_selector_on_view_name_changed(XfdashboardView *inView, const gchar *inName, gpointer inUserData)
{
	XfdashboardTooltipAction			*action;

	g_return_if_fail(XFDASHBOARD_IS_VIEW(inView));
	g_return_if_fail(XFDASHBOARD_IS_TOOLTIP_ACTION(inUserData));

	action=XFDASHBOARD_TOOLTIP_ACTION(inUserData);
	xfdashboard_tooltip_action_set_text(action, inName);
}

/* Called when a new view was added to viewpad */
static void _xfdashboard_view_selector_on_view_added(XfdashboardViewSelector *self,
														XfdashboardView *inView,
														gpointer inUserData)
{
	XfdashboardViewSelectorPrivate		*priv;
	ClutterActor						*button;
	gchar								*viewName;
	const gchar							*viewIcon;
	gboolean							isActive;
	ClutterAction						*action;

	g_return_if_fail(XFDASHBOARD_IS_VIEW_SELECTOR(self));
	g_return_if_fail(XFDASHBOARD_IS_VIEW(inView));

	priv=self->priv;

	/* Create button for newly added view */
	viewName=g_markup_printf_escaped("%s", xfdashboard_view_get_name(inView));
	viewIcon=xfdashboard_view_get_icon(inView);

	button=xfdashboard_toggle_button_new_full(viewIcon, viewName);
	xfdashboard_toggle_button_set_auto_toggle(XFDASHBOARD_TOGGLE_BUTTON(button), FALSE);
	g_object_set_data(G_OBJECT(button), "view", inView);
	g_signal_connect_swapped(button, "clicked", G_CALLBACK(_xfdashboard_view_selector_on_view_button_clicked), self);

	/* Set toggle state depending of if view is active or not and connect
	 * signal to get notified if toggle state changes to proxy signal
	 */
	g_signal_connect_swapped(button, "toggled", G_CALLBACK(_xfdashboard_view_selector_on_toggle_button_state_changed), self);

	isActive=(xfdashboard_viewpad_get_active_view(priv->viewpad)==inView);
	xfdashboard_toggle_button_set_toggle_state(XFDASHBOARD_TOGGLE_BUTTON(button), isActive);

	/* Add tooltip */
	action=xfdashboard_tooltip_action_new();
	xfdashboard_tooltip_action_set_text(XFDASHBOARD_TOOLTIP_ACTION(action), viewName);
	clutter_actor_add_action(button, action);

	/* If view is disabled hide button otherwise show and connect signals
	 * to get notified if enabled state has changed
	 */
	if(!xfdashboard_view_get_enabled(inView)) clutter_actor_hide(button);
		else clutter_actor_show(button);

	g_signal_connect(inView, "disabled", G_CALLBACK(_xfdashboard_view_selector_on_view_enable_state_changed), button);
	g_signal_connect(inView, "enabled", G_CALLBACK(_xfdashboard_view_selector_on_view_enable_state_changed), button);
	g_signal_connect(inView, "activated", G_CALLBACK(_xfdashboard_view_selector_on_view_activated), button);
	g_signal_connect(inView, "deactivated", G_CALLBACK(_xfdashboard_view_selector_on_view_deactivated), button);
	g_signal_connect(inView, "icon-changed", G_CALLBACK(_xfdashboard_view_selector_on_view_icon_changed), button);
	g_signal_connect(inView, "name-changed", G_CALLBACK(_xfdashboard_view_selector_on_view_name_changed), action);

	/* Add button as child actor */
	clutter_actor_add_child(CLUTTER_ACTOR(self), button);

	/* Release allocated resources */
	g_free(viewName);
}

/* Called when a view was removed to viewpad */
static void _xfdashboard_view_selector_on_view_removed(XfdashboardViewSelector *self,
														XfdashboardView *inView,
														gpointer inUserData)
{
	ClutterActorIter					iter;
	ClutterActor						*child;
	gpointer							view;

	g_return_if_fail(XFDASHBOARD_IS_VIEW_SELECTOR(self));

	/* Iterate through create views and lookup view of given type */
	clutter_actor_iter_init(&iter, CLUTTER_ACTOR(self));
	while(clutter_actor_iter_next(&iter, &child))
	{
		/* Check if child is a button otherwise continue iterating */
		if(!XFDASHBOARD_IS_TOGGLE_BUTTON(child)) continue;

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
static void _xfdashboard_view_selector_dispose(GObject *inObject)
{
	XfdashboardViewSelector			*self=XFDASHBOARD_VIEW_SELECTOR(inObject);
	XfdashboardViewSelectorPrivate	*priv=self->priv;
	ClutterActorIter					iter;
	ClutterActor						*child;
	gpointer							view;

	/* Release allocated resources */
	clutter_actor_iter_init(&iter, CLUTTER_ACTOR(self));
	while(clutter_actor_iter_next(&iter, &child))
	{
		/* Check if child is a button otherwise continue iterating */
		if(!XFDASHBOARD_IS_TOGGLE_BUTTON(child)) continue;

		/* If button has reference to a view remove signal handlers it */
		view=g_object_get_data(G_OBJECT(child), "view");
		if(view) g_signal_handlers_disconnect_by_data(view, child);
	}

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
static void _xfdashboard_view_selector_set_property(GObject *inObject,
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

		case PROP_ORIENTATION:
			xfdashboard_view_selector_set_orientation(self, g_value_get_enum(inValue));
			break;

		default:
			G_OBJECT_WARN_INVALID_PROPERTY_ID(inObject, inPropID, inSpec);
			break;
	}
}

static void _xfdashboard_view_selector_get_property(GObject *inObject,
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

		case PROP_ORIENTATION:
			g_value_set_enum(outValue, self->priv->orientation);
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
static void xfdashboard_view_selector_class_init(XfdashboardViewSelectorClass *klass)
{
	XfdashboardActorClass	*actorClass=XFDASHBOARD_ACTOR_CLASS(klass);
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
								G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS);

	XfdashboardViewSelectorProperties[PROP_SPACING]=
		g_param_spec_float("spacing",
							_("Spacing"),
							_("The spacing between views and scrollbars"),
							0.0f, G_MAXFLOAT,
							0.0f,
							G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS);

	XfdashboardViewSelectorProperties[PROP_ORIENTATION]=
		g_param_spec_enum("orientation",
							_("Orientation"),
							_("Orientation of view selector"),
							CLUTTER_TYPE_ORIENTATION,
							CLUTTER_ORIENTATION_HORIZONTAL,
							G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS);

	g_object_class_install_properties(gobjectClass, PROP_LAST, XfdashboardViewSelectorProperties);

	/* Define stylable properties */
	xfdashboard_actor_install_stylable_property(actorClass, XfdashboardViewSelectorProperties[PROP_SPACING]);
	xfdashboard_actor_install_stylable_property(actorClass, XfdashboardViewSelectorProperties[PROP_ORIENTATION]);

	/* Define signals */
	XfdashboardViewSelectorSignals[SIGNAL_STATE_CHANGED]=
		g_signal_new("state-changed",
						G_TYPE_FROM_CLASS(klass),
						G_SIGNAL_RUN_LAST,
						G_STRUCT_OFFSET(XfdashboardViewSelectorClass, state_changed),
						NULL,
						NULL,
						g_cclosure_marshal_VOID__OBJECT,
						G_TYPE_NONE,
						1,
						XFDASHBOARD_TYPE_TOGGLE_BUTTON);
}

/* Object initialization
 * Create private structure and set up default values
 */
static void xfdashboard_view_selector_init(XfdashboardViewSelector *self)
{
	XfdashboardViewSelectorPrivate		*priv;

	priv=self->priv=XFDASHBOARD_VIEW_SELECTOR_GET_PRIVATE(self);

	/* Set up default values */
	priv->viewpad=NULL;
	priv->spacing=0.0f;
	priv->orientation=CLUTTER_ORIENTATION_HORIZONTAL;

	priv->layout=clutter_box_layout_new();
	clutter_box_layout_set_orientation(CLUTTER_BOX_LAYOUT(priv->layout), priv->orientation);

	/* Set up actor */
	clutter_actor_set_reactive(CLUTTER_ACTOR(self), TRUE);
	clutter_actor_set_layout_manager(CLUTTER_ACTOR(self), priv->layout);
}

/* IMPLEMENTATION: Public API */

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
	XfdashboardViewSelectorPrivate		*priv;
	GList								*views, *entry;

	g_return_if_fail(XFDASHBOARD_IS_VIEW_SELECTOR(self));
	g_return_if_fail(XFDASHBOARD_IS_VIEWPAD(inViewpad));

	priv=self->priv;

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
	g_return_val_if_fail(XFDASHBOARD_IS_VIEW_SELECTOR(self), 0.0f);

	return(self->priv->spacing);
}

void xfdashboard_view_selector_set_spacing(XfdashboardViewSelector *self, gfloat inSpacing)
{
	XfdashboardViewSelectorPrivate	*priv;

	g_return_if_fail(XFDASHBOARD_IS_VIEW_SELECTOR(self));
	g_return_if_fail(inSpacing>=0.0f);

	priv=self->priv;

	/* Only set value if it changes */
	if(inSpacing==priv->spacing) return;

	/* Set new value */
	priv->spacing=inSpacing;
	if(priv->layout) clutter_box_layout_set_spacing(CLUTTER_BOX_LAYOUT(priv->layout), priv->spacing);
	clutter_actor_queue_relayout(CLUTTER_ACTOR(self));

	/* Notify about property change */
	g_object_notify_by_pspec(G_OBJECT(self), XfdashboardViewSelectorProperties[PROP_SPACING]);
}

/* Get/set orientation */
ClutterOrientation xfdashboard_view_selector_get_orientation(XfdashboardViewSelector *self)
{
	g_return_val_if_fail(XFDASHBOARD_IS_VIEW_SELECTOR(self), CLUTTER_ORIENTATION_HORIZONTAL);

	return(self->priv->orientation);
}

void xfdashboard_view_selector_set_orientation(XfdashboardViewSelector *self, ClutterOrientation inOrientation)
{
	XfdashboardViewSelectorPrivate	*priv;

	g_return_if_fail(XFDASHBOARD_IS_VIEW_SELECTOR(self));
	g_return_if_fail(inOrientation!=CLUTTER_ORIENTATION_HORIZONTAL || inOrientation!=CLUTTER_ORIENTATION_VERTICAL);

	priv=self->priv;

	/* Only set value if it changes */
	if(inOrientation==priv->orientation) return;

	/* Set new value */
	priv->orientation=inOrientation;
	if(priv->layout) clutter_box_layout_set_orientation(CLUTTER_BOX_LAYOUT(priv->layout), priv->orientation);
	clutter_actor_queue_relayout(CLUTTER_ACTOR(self));

	/* Notify about property change */
	g_object_notify_by_pspec(G_OBJECT(self), XfdashboardViewSelectorProperties[PROP_ORIENTATION]);
}
