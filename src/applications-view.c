/*
 * applications-view: A view showing all installed applications as menu
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

#include "applications-view.h"
#include "utils.h"
#include "stage.h"
#include "application.h"
#include "view.h"

/* Define this class in GObject system */
G_DEFINE_TYPE(XfdashboardApplicationsView,
				xfdashboard_applications_view,
				XFDASHBOARD_TYPE_VIEW)

/* Private structure - access only by public API if needed */
#define XFDASHBOARD_APPLICATIONS_VIEW_GET_PRIVATE(obj) \
	(G_TYPE_INSTANCE_GET_PRIVATE((obj), XFDASHBOARD_TYPE_APPLICATIONS_VIEW, XfdashboardApplicationsViewPrivate))

struct _XfdashboardApplicationsViewPrivate
{
	/* Properties related */

	/* Instance related */
	ClutterContent			*canvas;
};

/* Properties */
enum
{
	PROP_0,

	PROP_LAST
};

GParamSpec* XfdashboardApplicationsViewProperties[PROP_LAST]={ 0, };

/* IMPLEMENTATION: Private variables and methods */
#define DEFAULT_VIEW_ICON	GTK_STOCK_HOME	// TODO: Replace by settings/theming object

/* Rectangle canvas should be redrawn */
gboolean _xfdashboard_applications_view_on_draw_canvas(XfdashboardApplicationsView *self,
														cairo_t *inContext,
														int inWidth,
														int inHeight,
														gpointer inUserData)
{
	g_return_val_if_fail(XFDASHBOARD_IS_APPLICATIONS_VIEW(self), TRUE);
	g_return_val_if_fail(CLUTTER_IS_CANVAS(inUserData), TRUE);

	GDateTime		*now;
	gfloat			hours, minutes, seconds;
	ClutterColor	color;

	/* Get the current time and compute the angles */
	now=g_date_time_new_now_local();
	seconds=g_date_time_get_second(now)*G_PI/30;
	minutes=g_date_time_get_minute(now)*G_PI/30;
	hours=g_date_time_get_hour(now)*G_PI/6;
	g_date_time_unref(now);

	/* Clear the contents of the canvas, to avoid painting
	 * over the previous frame
	 */
	cairo_save(inContext);

	cairo_set_operator(inContext, CAIRO_OPERATOR_CLEAR);
	cairo_paint(inContext);

	cairo_restore(inContext);

	cairo_set_operator(inContext, CAIRO_OPERATOR_OVER);

	/* Scale the modelview to the size of the surface */
	cairo_scale(inContext, inWidth, inHeight);

	cairo_set_line_cap(inContext, CAIRO_LINE_CAP_ROUND);
	cairo_set_line_width(inContext, 0.1);

	/* The black rail that holds the seconds indicator */
	clutter_cairo_set_source_color(inContext, CLUTTER_COLOR_Blue);
	cairo_translate(inContext, 0.5, 0.5);
	cairo_arc(inContext, 0, 0, 0.4, 0, G_PI*2);
	cairo_stroke(inContext);

	/* The seconds indicator */
	color=*CLUTTER_COLOR_White;
	color.alpha=128;
	clutter_cairo_set_source_color(inContext, &color);
	cairo_move_to(inContext, 0, 0);
	cairo_arc(inContext, sinf(seconds)*0.4, -cosf(seconds)*0.4, 0.05, 0, G_PI*2);
	cairo_fill(inContext);

	/* The minutes hand */
	color=*CLUTTER_COLOR_LightChameleon;
	color.alpha=196;
	clutter_cairo_set_source_color(inContext, &color);
	cairo_move_to(inContext, 0, 0);
	cairo_line_to(inContext, sinf(minutes)*0.4, -cosf(minutes)*0.4);
	cairo_stroke(inContext);

	/* The hours hand */
	cairo_move_to(inContext, 0, 0);
	cairo_line_to(inContext, sinf(hours)*0.2, -cosf(hours)*0.2);
	cairo_stroke(inContext);

	/* Done drawing */
	return(TRUE);
}

/* IMPLEMENTATION: XfdashboardView */

/* Called when view instance was created */
void _xfdashboard_applications_view_created(XfdashboardView *self)
{
	g_return_if_fail(XFDASHBOARD_IS_VIEW(self));

	/* Set up default values for properties */
	g_object_set(G_OBJECT(self),
					"view-name", "Applications",
					"view-icon", DEFAULT_VIEW_ICON,
					NULL);
}

/* IMPLEMENTATION: ClutterActor */

/* Allocate position and size of actor and its children*/
void _xfdashboard_applications_view_allocate(ClutterActor *self,
												const ClutterActorBox *inBox,
												ClutterAllocationFlags inFlags)
{
	XfdashboardApplicationsViewPrivate		*priv=XFDASHBOARD_APPLICATIONS_VIEW(self)->priv;

	/* Chain up to store the allocation of the actor */
	CLUTTER_ACTOR_CLASS(xfdashboard_applications_view_parent_class)->allocate(self, inBox, inFlags);

	/* Set size of slider canvas */
	clutter_content_invalidate(priv->canvas);
	//~ clutter_canvas_set_size(CLUTTER_CANVAS(priv->canvas),
								//~ clutter_actor_box_get_width(inBox),
								//~ clutter_actor_box_get_height(inBox));
}

/* IMPLEMENTATION: GObject */

/* Dispose this object */
void _xfdashboard_applications_view_dispose(GObject *inObject)
{
	XfdashboardApplicationsView			*self=XFDASHBOARD_APPLICATIONS_VIEW(inObject);
	XfdashboardApplicationsViewPrivate	*priv=self->priv;

	/* Release allocated resources */
	if(priv->canvas)
	{
		g_object_unref(priv->canvas);
		priv->canvas=NULL;
	}

	/* Call parent's class dispose method */
	G_OBJECT_CLASS(xfdashboard_applications_view_parent_class)->dispose(inObject);
}

/* Set/get properties */
void _xfdashboard_applications_view_set_property(GObject *inObject,
											guint inPropID,
											const GValue *inValue,
											GParamSpec *inSpec)
{
	XfdashboardApplicationsView		*self=XFDASHBOARD_APPLICATIONS_VIEW(inObject);
	
	switch(inPropID)
	{
		default:
			G_OBJECT_WARN_INVALID_PROPERTY_ID(inObject, inPropID, inSpec);
			break;
	}
}

void _xfdashboard_applications_view_get_property(GObject *inObject,
											guint inPropID,
											GValue *outValue,
											GParamSpec *inSpec)
{
	XfdashboardApplicationsView		*self=XFDASHBOARD_APPLICATIONS_VIEW(inObject);

	switch(inPropID)
	{
		default:
			G_OBJECT_WARN_INVALID_PROPERTY_ID(inObject, inPropID, inSpec);
			break;
	}
}

/* Class initialization
 * Override functions in parent classes and define properties
 * and signals
 */
void xfdashboard_applications_view_class_init(XfdashboardApplicationsViewClass *klass)
{
	XfdashboardViewClass	*viewClass=XFDASHBOARD_VIEW_CLASS(klass);
	ClutterActorClass		*actorClass=CLUTTER_ACTOR_CLASS(klass);
	GObjectClass			*gobjectClass=G_OBJECT_CLASS(klass);

	/* Override functions */
	gobjectClass->dispose=_xfdashboard_applications_view_dispose;
	gobjectClass->set_property=_xfdashboard_applications_view_set_property;
	gobjectClass->get_property=_xfdashboard_applications_view_get_property;

	actorClass->allocate=_xfdashboard_applications_view_allocate;

	viewClass->created=_xfdashboard_applications_view_created;

	/* Set up private structure */
	g_type_class_add_private(klass, sizeof(XfdashboardApplicationsViewPrivate));
}

/* Object initialization
 * Create private structure and set up default values
 */
void xfdashboard_applications_view_init(XfdashboardApplicationsView *self)
{
	XfdashboardApplicationsViewPrivate	*priv;

	self->priv=priv=XFDASHBOARD_APPLICATIONS_VIEW_GET_PRIVATE(self);

#define ACTOR_WIDTH 1600.0f
#define ACTOR_HEIGHT 900.0f

	/* Set up default values */
	priv->canvas=clutter_canvas_new();
	clutter_canvas_set_size(CLUTTER_CANVAS(priv->canvas), ACTOR_WIDTH, ACTOR_HEIGHT);
	g_signal_connect_swapped(priv->canvas, "draw", G_CALLBACK(_xfdashboard_applications_view_on_draw_canvas), self);

	clutter_actor_set_content(CLUTTER_ACTOR(self), priv->canvas);
	clutter_actor_set_size(CLUTTER_ACTOR(self), ACTOR_WIDTH, ACTOR_HEIGHT);
#undef ACTOR_HEIGHT
#undef ACTOR_WIDTH
}

/* Implementation: Public API */

/* Create new actor */
ClutterActor* xfdashboard_applications_view_new(void)
{
	return(g_object_new(XFDASHBOARD_TYPE_APPLICATIONS_VIEW, NULL));
}
