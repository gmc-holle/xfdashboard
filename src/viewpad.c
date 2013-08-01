/*
 * viewpad: A viewpad managing views
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
#include <gtk/gtk.h>

#include "viewpad.h"
#include "common.h"

/* Define this class in GObject system */
G_DEFINE_TYPE(XfdashboardViewpad,
				xfdashboard_viewpad,
				CLUTTER_TYPE_ACTOR)

/* Private structure - access only by public API if needed */
#define XFDASHBOARD_VIEWPAD_GET_PRIVATE(obj) \
	(G_TYPE_INSTANCE_GET_PRIVATE((obj), XFDASHBOARD_TYPE_VIEWPAD, XfdashboardViewpadPrivate))

struct _XfdashboardViewpadPrivate
{
	/* Properties related */
	gfloat					spacing;
	XfdashboardView			*activeView;

	/* Instance related */
	ClutterLayoutManager	*layout;
	ClutterActor			*container;
	ClutterActor			*scrollbarHorizontal;
	ClutterActor			*scrollbarVertical;
};

/* Properties */
enum
{
	PROP_0,

	PROP_SPACING,
	PROP_ACTIVE_VIEW,

	PROP_LAST
};

static GParamSpec* XfdashboardViewpadProperties[PROP_LAST]={ 0, };

/* Signals */
enum
{
	SIGNAL_LAST
};

static guint XfdashboardViewpadSignals[SIGNAL_LAST]={ 0, };

/* IMPLEMENTATION: Private variables and methods */
#define DEFAULT_SPACING		4.0f			// TODO: Replace by settings/theming object

/* IMPLEMENTATION: ClutterActor */

/* Show all children of this actor */
void _xfdashboard_viewpad_show_all(ClutterActor *self)
{
	XfdashboardViewpadPrivate	*priv=XFDASHBOARD_VIEWPAD(self)->priv;

	/* Only show active view again */
	if(priv->activeView) clutter_actor_show(CLUTTER_ACTOR(priv->activeView));

	/* Show this actor again */
	clutter_actor_show(self);
}

/* IMPLEMENTATION: GObject */

/* Dispose this object */
void _xfdashboard_viewpad_dispose(GObject *inObject)
{
	XfdashboardViewpad			*self=XFDASHBOARD_VIEWPAD(inObject);
	XfdashboardViewpadPrivate	*priv=self->priv;

	/* Call parent's class dispose method */
	G_OBJECT_CLASS(xfdashboard_viewpad_parent_class)->dispose(inObject);
}

/* Set/get properties */
void _xfdashboard_viewpad_set_property(GObject *inObject,
												guint inPropID,
												const GValue *inValue,
												GParamSpec *inSpec)
{
	XfdashboardViewpad		*self=XFDASHBOARD_VIEWPAD(inObject);
	
	switch(inPropID)
	{
		case PROP_SPACING:
			xfdashboard_viewpad_set_spacing(self, g_value_get_float(inValue));
			break;

		default:
			G_OBJECT_WARN_INVALID_PROPERTY_ID(inObject, inPropID, inSpec);
			break;
	}
}

static void _xfdashboard_viewpad_get_property(GObject *inObject,
												guint inPropID,
												GValue *outValue,
												GParamSpec *inSpec)
{
	XfdashboardViewpad		*self=XFDASHBOARD_VIEWPAD(inObject);

	switch(inPropID)
	{
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
void xfdashboard_viewpad_class_init(XfdashboardViewpadClass *klass)
{
	ClutterActorClass		*actorClass=CLUTTER_ACTOR_CLASS(klass);
	GObjectClass			*gobjectClass=G_OBJECT_CLASS(klass);

	/* Override functions */
	gobjectClass->set_property=_xfdashboard_viewpad_set_property;
	gobjectClass->get_property=_xfdashboard_viewpad_get_property;
	gobjectClass->dispose=_xfdashboard_viewpad_dispose;

	actorClass->show_all=_xfdashboard_viewpad_show_all;

	/* Set up private structure */
	g_type_class_add_private(klass, sizeof(XfdashboardViewpadPrivate));

	/* Define properties */
	XfdashboardViewpadProperties[PROP_SPACING]=
		g_param_spec_float("spacing",
							_("Spacing"),
							_("The spacing between views and scrollbars"),
							0.0f, G_MAXFLOAT,
							DEFAULT_SPACING,
							G_PARAM_READWRITE);

	XfdashboardViewpadProperties[PROP_ACTIVE_VIEW]=
		g_param_spec_object("active-view",
								_("Active view"),
								_("The current active view in viewpad"),
								XFDASHBOARD_TYPE_VIEW,
								G_PARAM_READWRITE);

	g_object_class_install_properties(gobjectClass, PROP_LAST, XfdashboardViewpadProperties);
}

/* Object initialization
 * Create private structure and set up default values
 */
static void xfdashboard_viewpad_init(XfdashboardViewpad *self)
{
	XfdashboardViewpadPrivate	*priv;
	GList						*views;
	ClutterLayoutManager		*layout;

	priv=self->priv=XFDASHBOARD_VIEWPAD_GET_PRIVATE(self);

	/* Set up default values */
	priv->activeView=NULL;
	priv->spacing=DEFAULT_SPACING;

	/* Set up actor */
	priv->container=clutter_actor_new();
	clutter_actor_set_x_expand(priv->container, TRUE);
	clutter_actor_set_y_expand(priv->container, TRUE);
	clutter_actor_set_x_align(priv->container, 0.5f);
	clutter_actor_set_y_align(priv->container, 0.5f);
	priv->layout=clutter_bin_layout_new(CLUTTER_BIN_ALIGNMENT_FILL, CLUTTER_BIN_ALIGNMENT_FILL);
	clutter_actor_set_layout_manager(priv->container, priv->layout);

	/* TODO: BEGIN - Implement scrollbars and remove color variable as scrollbars should derive XfdashboardBackground */
	ClutterColor				color={ 0xd0, 0xd0, 0xd0, 0xff };

	priv->scrollbarHorizontal=clutter_actor_new();
	clutter_actor_set_background_color(priv->scrollbarHorizontal, &color);
	clutter_actor_set_size(priv->scrollbarHorizontal, 8, 8);
	clutter_actor_set_x_expand(priv->scrollbarHorizontal, TRUE);
	clutter_actor_set_y_expand(priv->scrollbarHorizontal, FALSE);

	priv->scrollbarVertical=clutter_actor_new();
	clutter_actor_set_background_color(priv->scrollbarVertical, &color);
	clutter_actor_set_size(priv->scrollbarVertical, 8, 8);
	clutter_actor_set_x_expand(priv->scrollbarHorizontal, FALSE);
	clutter_actor_set_y_expand(priv->scrollbarHorizontal, TRUE);
	/* TODO: END - Implement scrollbars and remove color variable as scrollbars should derive XfdashboardBackground */

	priv->layout=clutter_grid_layout_new();
	clutter_grid_layout_set_column_spacing(CLUTTER_GRID_LAYOUT(priv->layout), priv->spacing);
	clutter_grid_layout_set_row_spacing(CLUTTER_GRID_LAYOUT(priv->layout), priv->spacing);
	clutter_actor_set_layout_manager(CLUTTER_ACTOR(self), priv->layout);
	clutter_actor_set_reactive(CLUTTER_ACTOR(self), FALSE);
	clutter_grid_layout_attach(CLUTTER_GRID_LAYOUT(priv->layout), priv->container, 0, 0, 1, 1);
	clutter_grid_layout_attach(CLUTTER_GRID_LAYOUT(priv->layout), priv->scrollbarVertical, 0, 1, 1, 1);
	clutter_grid_layout_attach(CLUTTER_GRID_LAYOUT(priv->layout), priv->scrollbarHorizontal, 1, 0, 1, 1);

	/* Create instance of each registered view type and add it to this actor */
	for(views=xfdashboard_view_get_registered(); views; views=g_list_next(views))
	{
		GObject					*view;
		GType					viewType;

		/* Create instance and check if it is a view */
		viewType=(GType)GPOINTER_TO_INT(views->data);
		g_debug("Creating view %s for viewpad", g_type_name(viewType));

		view=g_object_new(viewType, NULL);
		if(view==NULL)
		{
			g_critical(_("Failed to create view instance of %s"), g_type_name(viewType));
			continue;
		}

		if(XFDASHBOARD_IS_VIEW(view)!=TRUE)
		{
			g_critical(_("Instance %s is not a %s and cannot be added to %s"),
						g_type_name(viewType), g_type_name(XFDASHBOARD_TYPE_VIEW), G_OBJECT_TYPE_NAME(self));
			continue;
		}

		/* Add new view instance to this actor */
		// TODO: clutter_actor_set_x_expand(CLUTTER_ACTOR(view), TRUE);
		// TODO: clutter_actor_set_y_expand(CLUTTER_ACTOR(view), TRUE);
		// TODO: clutter_actor_set_x_align(CLUTTER_ACTOR(view), CLUTTER_ACTOR_ALIGN_CENTER);
		// TODO: clutter_actor_set_y_align(CLUTTER_ACTOR(view), CLUTTER_ACTOR_ALIGN_CENTER);
		clutter_actor_add_child(priv->container, CLUTTER_ACTOR(view));
	}

}

/* Implementation: Public API */

/* Create new instance */
ClutterActor* xfdashboard_viewpad_new(void)
{
	return(CLUTTER_ACTOR(g_object_new(XFDASHBOARD_TYPE_VIEWPAD, NULL)));
}

/* Get/set spacing */
gfloat xfdashboard_viewpad_get_spacing(XfdashboardViewpad *self)
{
	g_return_val_if_fail(XFDASHBOARD_IS_VIEWPAD(self), 0.0f);

	return(self->priv->spacing);
}

void xfdashboard_viewpad_set_spacing(XfdashboardViewpad *self, gfloat inSpacing)
{
	g_return_if_fail(XFDASHBOARD_IS_VIEWPAD(self));
	g_return_if_fail(inSpacing>=0.0f);

	XfdashboardViewpadPrivate	*priv=self->priv;

	/* Only set value if it changes */
	if(inSpacing==priv->spacing) return;

	/* Set new value */
	priv->spacing=inSpacing;
	if(priv->layout)
	{
		clutter_grid_layout_set_column_spacing(CLUTTER_GRID_LAYOUT(priv->layout), priv->spacing);
		clutter_grid_layout_set_row_spacing(CLUTTER_GRID_LAYOUT(priv->layout), priv->spacing);
	}
	clutter_actor_queue_relayout(CLUTTER_ACTOR(self));

	/* Notify about property change */
	g_object_notify_by_pspec(G_OBJECT(self), XfdashboardViewpadProperties[PROP_SPACING]);
}
