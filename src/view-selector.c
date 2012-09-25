/*
 * view-selector.c: A selector for views in viewpad as buttons
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

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "view-selector.h"

/* Define this class in GObject system */
G_DEFINE_TYPE(XfdashboardViewSelector,
				xfdashboard_view_selector,
				CLUTTER_TYPE_ACTOR)

/* Private structure - access only by public API if needed */
#define XFDASHBOARD_VIEW_SELECTOR_GET_PRIVATE(obj) \
	(G_TYPE_INSTANCE_GET_PRIVATE((obj), XFDASHBOARD_TYPE_VIEW_SELECTOR, XfdashboardViewSelectorPrivate))

struct _XfdashboardViewSelectorPrivate
{
	/* Viewpad this actor belongs to */
	XfdashboardViewpad		*viewpad;
	
	/* Layoutted actor */
	ClutterActor			*buttons;

	/* Settings */
	ClutterColor			*color;
	ClutterColor			*selectedColor;
};

/* Properties */
enum
{
	PROP_0,

	PROP_VIEWPAD,

	PROP_COLOR,
	PROP_SELECTED_COLOR,

	PROP_LAST
};

static GParamSpec* XfdashboardViewSelectorProperties[PROP_LAST]={ 0, };

/* IMPLEMENTATION: Private variables and methods */
static ClutterColor		_xfdashboard_view_selector_default_color=
							{ 0xa0, 0xa0, 0xa0, 0xff };

static ClutterColor		_xfdashboard_view_selector_default_selected_color=
							{ 0xff, 0xff, 0xff, 0xff };

/* A button in view selector was clicked */
void _xfdashboard_view_selector_clicked(ClutterActor *inActor,
										ClutterEvent *inEvent,
										gpointer inUserData)
{
	g_return_if_fail(XFDASHBOARD_IS_VIEW_SELECTOR(inUserData));

	XfdashboardViewSelectorPrivate	*priv=XFDASHBOARD_VIEW_SELECTOR(inUserData)->priv;
	GList							*views;
	GList							*buttons;

	/* Only change view if actor was left-clicked */
	if(clutter_event_get_button(inEvent)!=1) return;
	
	/* Iterate through button and find the one to which this action
	 * is connected to
	 */
	buttons=clutter_container_get_children(CLUTTER_CONTAINER(priv->buttons));
	views=xfdashboard_viewpad_get_views(priv->viewpad);
	if(G_UNLIKELY(g_list_length(buttons)!=g_list_length(views)))
	{
		g_warning("Number of views (%d) does not match number of buttons (%d)",
					g_list_length(views), g_list_length(buttons));
	}

	for( ; buttons && views; buttons=buttons->next, views=views->next)
	{
		ClutterText			*button=CLUTTER_TEXT(buttons->data);
		XfdashboardView		*view=XFDASHBOARD_VIEW(views->data);

		if(CLUTTER_ACTOR(button)==inActor)
		{
			xfdashboard_viewpad_set_active_view(priv->viewpad, view);
		}
	}
}

/* Update buttons for views */
void _xfdashboard_view_selector_update(XfdashboardViewSelector *self, gboolean inCreateActors)
{
	g_return_if_fail(XFDASHBOARD_IS_VIEW_SELECTOR(self));

	XfdashboardViewSelectorPrivate	*priv=self->priv;
	XfdashboardView					*activeView;
	GList							*views;
	GList							*buttons;

	/* Check if we should (re)create actors */
	if(inCreateActors)
	{
		/* Destroy all current actors */
		buttons=clutter_container_get_children(CLUTTER_CONTAINER(priv->buttons));
		g_list_foreach(buttons, (GFunc)clutter_actor_destroy, NULL);

		/* Create actors only if we have a viewpad assigned */
		if(!priv->viewpad) return;
		
		/* Create actors */
		views=xfdashboard_viewpad_get_views(priv->viewpad);
		for( ; views; views=views->next)
		{
			ClutterActor		*button;

			/* Create actor */
			button=clutter_text_new();
			clutter_actor_set_reactive(button, TRUE);
			clutter_actor_show(button);
			clutter_box_pack(CLUTTER_BOX(priv->buttons), button,
								"x-align", CLUTTER_BOX_ALIGNMENT_CENTER,
								"y-align", CLUTTER_BOX_ALIGNMENT_CENTER,
								"expand", TRUE,
								NULL);

			/* Connect signals to actor */
			g_signal_connect(button, "button-press-event", G_CALLBACK(_xfdashboard_view_selector_clicked), self);
		}
	}

	/* Update actors only if we have a viewpad assigned */
	if(!priv->viewpad) return;

	/* Iterate through buttons and set color depending on
	 * if view is active (selected) or not
	 */
	buttons=clutter_container_get_children(CLUTTER_CONTAINER(priv->buttons));
	views=xfdashboard_viewpad_get_views(priv->viewpad);
	if(G_UNLIKELY(g_list_length(buttons)!=g_list_length(views)))
	{
		g_warning("Number of views (%d) does not match number of buttons (%d)",
					g_list_length(views), g_list_length(buttons));
	}

	activeView=xfdashboard_viewpad_get_active_view(priv->viewpad);
	for( ; buttons && views; buttons=buttons->next, views=views->next)
	{
		ClutterText			*button=CLUTTER_TEXT(buttons->data);
		XfdashboardView		*view=XFDASHBOARD_VIEW(views->data);

		if(!button)
		{
			g_error("Unexpected null-pointer actor - skipping!");
			continue;
		}

		if(!view)
		{
			g_error("Unexpected null-pointer view - skipping!");
			continue;
		}

		if(activeView && view==activeView)
		{
			gchar			*buttonText;

			buttonText=g_strdup_printf("<b>%s</b>", xfdashboard_view_get_view_name(view));
			clutter_text_set_markup(CLUTTER_TEXT(button), buttonText);
			clutter_text_set_color(button, priv->selectedColor);
			g_free(buttonText);
		}
			else
			{
				clutter_text_set_text(CLUTTER_TEXT(button), xfdashboard_view_get_view_name(view));
				clutter_text_set_color(button, priv->color);
			}
	}
}

/* Another view was activated */
void _xfdashboard_view_selector_view_activated(XfdashboardViewSelector *self,
												XfdashboardView *inView,
												gpointer inUserData)
{
	g_return_if_fail(XFDASHBOARD_IS_VIEW_SELECTOR(self));
	g_return_if_fail(XFDASHBOARD_IS_VIEWPAD(inUserData));

	/* Update buttons */
	_xfdashboard_view_selector_update(self, FALSE);
}

/* A view was added or removed */
void _xfdashboard_view_selector_views_changed(XfdashboardViewSelector *self,
												XfdashboardView *inView,
												gpointer inUserData)
{
	g_return_if_fail(XFDASHBOARD_IS_VIEW_SELECTOR(self));
	g_return_if_fail(XFDASHBOARD_IS_VIEWPAD(inUserData));

	/* Recreate and update buttons */
	_xfdashboard_view_selector_update(self, TRUE);
}

/* Set viewpad */
void _xfdashboard_view_selector_set_viewpad(XfdashboardViewSelector *self, XfdashboardViewpad *inViewpad)
{
	g_return_if_fail(XFDASHBOARD_IS_VIEW_SELECTOR(self));
	g_return_if_fail(!inViewpad || XFDASHBOARD_IS_VIEWPAD(inViewpad));

	XfdashboardViewSelectorPrivate	*priv=self->priv;

	/* Only set new viewpad if it is a different one */
	if(inViewpad==priv->viewpad) return;

	/* Disconnect signals from current viewpad */
	if(priv->viewpad)
	{
		g_signal_handlers_disconnect_by_func(priv->viewpad, (gpointer)_xfdashboard_view_selector_views_changed, self);
		g_signal_handlers_disconnect_by_func(priv->viewpad, (gpointer)_xfdashboard_view_selector_view_activated, self);
		g_object_unref(priv->viewpad);
		priv->viewpad=NULL;
	}

	/* Connect signals to new viewpad */
	if(inViewpad)
	{
		priv->viewpad=inViewpad;
		g_object_ref(priv->viewpad);
		g_signal_connect_swapped(priv->viewpad, "view-added", G_CALLBACK(_xfdashboard_view_selector_views_changed), self);
		g_signal_connect_swapped(priv->viewpad, "view-removed", G_CALLBACK(_xfdashboard_view_selector_views_changed), self);
		g_signal_connect_swapped(priv->viewpad, "view-activated", G_CALLBACK(_xfdashboard_view_selector_view_activated), self);

		_xfdashboard_view_selector_update(self, TRUE);
	}
}

/* Set color of item for unselected views */
void _xfdashboard_view_selector_set_color(XfdashboardViewSelector *self, const ClutterColor *inColor)
{
	g_return_if_fail(XFDASHBOARD_IS_VIEW_SELECTOR(self));
	g_return_if_fail(inColor);

	/* Set color if it differs from current value */
	XfdashboardViewSelectorPrivate		*priv=XFDASHBOARD_VIEW_SELECTOR(self)->priv;

	if(!priv->color || !clutter_color_equal(inColor, priv->color))
	{
		if(priv->color) clutter_color_free(priv->color);
		priv->color=clutter_color_copy(inColor);

		_xfdashboard_view_selector_update(self, FALSE);
	}
}

/* Set color of item for selected view */
void _xfdashboard_view_selector_set_selected_color(XfdashboardViewSelector *self, const ClutterColor *inColor)
{
	g_return_if_fail(XFDASHBOARD_IS_VIEW_SELECTOR(self));
	g_return_if_fail(inColor);

	/* Set color if it differs from current value */
	XfdashboardViewSelectorPrivate		*priv=XFDASHBOARD_VIEW_SELECTOR(self)->priv;

	if(!priv->selectedColor || !clutter_color_equal(inColor, priv->selectedColor))
	{
		if(priv->selectedColor) clutter_color_free(priv->selectedColor);
		priv->selectedColor=clutter_color_copy(inColor);

		_xfdashboard_view_selector_update(self, FALSE);
	}
}

/* IMPLEMENTATION: ClutterActor */

/* Paint actor */
static void xfdashboard_view_selector_paint(ClutterActor *inActor)
{
	XfdashboardViewSelectorPrivate	*priv=XFDASHBOARD_VIEW_SELECTOR(inActor)->priv;

	clutter_actor_paint(CLUTTER_ACTOR(priv->buttons));
}

/* Pick this actor and possibly all the child actors.
 * That means that this function should draw a solid shape of actor's silouhette
 * in the given color. This shape is drawn to an invisible offscreen and is used
 * by Clutter to determine an actor fast by inspecting the color at the position.
 * The default implementation is to draw a solid rectangle covering the allocation
 * of THIS actor.
 * If we could not use this default implementation we have chain up to parent class
 * and call the paint function of any child we know and which can be reactive.
 */
static void xfdashboard_view_selector_pick(ClutterActor *self, const ClutterColor *inColor)
{
	XfdashboardViewSelectorPrivate	*priv=XFDASHBOARD_VIEW_SELECTOR(self)->priv;

	/* It is possible to avoid a costly paint by checking
	 * whether the actor should really be painted in pick mode
	 */
	if(!clutter_actor_should_pick_paint(self)) return;

	/* Chain up so we get a bounding box painted (if we are reactive) */
	CLUTTER_ACTOR_CLASS(xfdashboard_view_selector_parent_class)->pick(self, inColor);

	/* Draw silouhette of buttons */
	clutter_actor_paint(CLUTTER_ACTOR(priv->buttons));
}

/* Get preferred width/height */
static void xfdashboard_view_selector_get_preferred_width(ClutterActor *inActor,
															gfloat inForHeight,
															gfloat *outMinWidth,
															gfloat *outNaturalWidth)
{
	XfdashboardViewSelectorPrivate	*priv=XFDASHBOARD_VIEW_SELECTOR(inActor)->priv;

	clutter_actor_get_preferred_height(priv->buttons, inForHeight, outMinWidth, outNaturalWidth);
}

static void xfdashboard_view_selector_get_preferred_height(ClutterActor *inActor,
															gfloat inForWidth,
															gfloat *outMinHeight,
															gfloat *outNaturalWidth)
{
	XfdashboardViewSelectorPrivate	*priv=XFDASHBOARD_VIEW_SELECTOR(inActor)->priv;

	clutter_actor_get_preferred_height(priv->buttons, inForWidth, outMinHeight, outNaturalWidth);
}

/* Allocate position and size of actor and its children*/
static void xfdashboard_view_selector_allocate(ClutterActor *inActor,
												const ClutterActorBox *inAllocation,
												ClutterAllocationFlags inFlags)
{
	XfdashboardViewSelectorPrivate	*priv=XFDASHBOARD_VIEW_SELECTOR(inActor)->priv;
	ClutterActorClass				*klass;
	ClutterActorBox					box;
	gfloat							w, h;

	klass=CLUTTER_ACTOR_CLASS(xfdashboard_view_selector_parent_class);
	klass->allocate(inActor, inAllocation, inFlags);

	clutter_actor_box_get_size(inAllocation, &w, &h);

	clutter_actor_box_set_origin(&box, 0.0f, 0.0f);
	clutter_actor_box_set_size(&box, w, h);
	clutter_actor_allocate(priv->buttons, &box, inFlags);
}

/* Destroy this actor */
static void xfdashboard_view_selector_destroy(ClutterActor *inActor)
{
	XfdashboardViewSelectorPrivate		*priv=XFDASHBOARD_VIEW_SELECTOR(inActor)->priv;

	/* Destroy all our children */
	if(priv->buttons) clutter_actor_destroy(priv->buttons);
	priv->buttons=NULL;
	
	/* Call parent's class destroy method */
	if(CLUTTER_ACTOR_CLASS(xfdashboard_view_selector_parent_class)->destroy)
	{
		CLUTTER_ACTOR_CLASS(xfdashboard_view_selector_parent_class)->destroy(inActor);
	}
}

/* IMPLEMENTATION: GObject */

/* Dispose this object */
static void xfdashboard_view_selector_dispose(GObject *inObject)
{
	XfdashboardViewSelector			*self=XFDASHBOARD_VIEW_SELECTOR(inObject);
	XfdashboardViewSelectorPrivate	*priv=self->priv;
	
	/* Release allocated resources */
	_xfdashboard_view_selector_set_viewpad(self, NULL);
	
	if(priv->color) clutter_color_free(priv->color);
	priv->color=NULL;

	if(priv->selectedColor) clutter_color_free(priv->selectedColor);
	priv->selectedColor=NULL;

	/* Call parent's class dispose method */
	G_OBJECT_CLASS(xfdashboard_view_selector_parent_class)->dispose(inObject);
}

/* Set/get properties */
static void xfdashboard_view_selector_set_property(GObject *inObject,
													guint inPropID,
													const GValue *inValue,
													GParamSpec *inSpec)
{
	XfdashboardViewSelector		*self=XFDASHBOARD_VIEW_SELECTOR(inObject);
	
	switch(inPropID)
	{
		case PROP_VIEWPAD:
			_xfdashboard_view_selector_set_viewpad(self, XFDASHBOARD_VIEWPAD(g_value_get_object(inValue)));
			break;

		case PROP_COLOR:
			_xfdashboard_view_selector_set_color(self, clutter_value_get_color(inValue));
			break;

		case PROP_SELECTED_COLOR:
			_xfdashboard_view_selector_set_selected_color(self, clutter_value_get_color(inValue));
			break;

		default:
			G_OBJECT_WARN_INVALID_PROPERTY_ID(inObject, inPropID, inSpec);
			break;
	}
}

static void xfdashboard_view_selector_get_property(GObject *inObject,
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

		case PROP_COLOR:
			clutter_value_set_color(outValue, self->priv->color);
			break;

		case PROP_SELECTED_COLOR:
			clutter_value_set_color(outValue, self->priv->selectedColor);
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
	GObjectClass			*gobjectClass=G_OBJECT_CLASS(klass);
	ClutterActorClass		*actorClass=CLUTTER_ACTOR_CLASS(klass);

	/* Override functions */
	actorClass->paint=xfdashboard_view_selector_paint;
	actorClass->pick=xfdashboard_view_selector_pick;
	actorClass->get_preferred_width=xfdashboard_view_selector_get_preferred_width;
	actorClass->get_preferred_height=xfdashboard_view_selector_get_preferred_height;
	actorClass->allocate=xfdashboard_view_selector_allocate;
	actorClass->destroy=xfdashboard_view_selector_destroy;

	gobjectClass->set_property=xfdashboard_view_selector_set_property;
	gobjectClass->get_property=xfdashboard_view_selector_get_property;
	gobjectClass->dispose=xfdashboard_view_selector_dispose;

	/* Set up private structure */
	g_type_class_add_private(klass, sizeof(XfdashboardViewSelectorPrivate));

	/* Define properties */
	XfdashboardViewSelectorProperties[PROP_VIEWPAD]=
		g_param_spec_object("viewpad",
							"Viewpad",
							"The viewpad this selector belongs to",
							XFDASHBOARD_TYPE_VIEWPAD,
							G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY);

	XfdashboardViewSelectorProperties[PROP_COLOR]=
		clutter_param_spec_color("color",
									"Default color",
									"Default color for unselected view buttons",
									&_xfdashboard_view_selector_default_color,
									G_PARAM_READWRITE | G_PARAM_CONSTRUCT);

	XfdashboardViewSelectorProperties[PROP_SELECTED_COLOR]=
		clutter_param_spec_color("selected-color",
									"Selected color",
									"Color for selected view button",
									&_xfdashboard_view_selector_default_selected_color,
									G_PARAM_READWRITE | G_PARAM_CONSTRUCT);

	g_object_class_install_properties(gobjectClass, PROP_LAST, XfdashboardViewSelectorProperties);
}

/* Object initialization
 * Create private structure and set up default values
 */
static void xfdashboard_view_selector_init(XfdashboardViewSelector *self)
{
	XfdashboardViewSelectorPrivate	*priv;
	ClutterLayoutManager			*layout;

	priv=self->priv=XFDASHBOARD_VIEW_SELECTOR_GET_PRIVATE(self);

	/* Set up default values */
	priv->viewpad=NULL;
	priv->buttons=NULL;
	priv->color=NULL;
	priv->selectedColor=NULL;

	/* Set up ClutterBox to hold quicklaunch icons */
	clutter_actor_set_reactive(CLUTTER_ACTOR(self), TRUE);

	layout=clutter_box_layout_new();
	clutter_box_layout_set_spacing(CLUTTER_BOX_LAYOUT(layout), 8.0f);

	priv->buttons=clutter_box_new(layout);
	clutter_actor_set_parent(priv->buttons, CLUTTER_ACTOR(self));
	clutter_actor_show(priv->buttons);
}

/* Implementation: Public API */

/* Create new actor */
ClutterActor* xfdashboard_view_selector_new(const XfdashboardViewpad *inViewpad)
{
	g_return_val_if_fail(XFDASHBOARD_IS_VIEWPAD(inViewpad), NULL);
	
	return(g_object_new(XFDASHBOARD_TYPE_VIEW_SELECTOR,
							"viewpad", inViewpad,
							NULL));
}
