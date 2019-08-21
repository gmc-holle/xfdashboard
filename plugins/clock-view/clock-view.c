/*
 * clock-view: A view showing a clock
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

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "clock-view.h"

#include <libxfdashboard/libxfdashboard.h>
#include <glib/gi18n-lib.h>
#include <gtk/gtk.h>
#include <math.h>

#include "clock-view-settings.h"


/* Define this class in GObject system */
struct _XfdashboardClockViewPrivate
{
	/* Instance related */
	ClutterActor					*clockActor;
	ClutterContent					*clockCanvas;
	guint							timeoutID;

	XfdashboardClockViewSettings	*settings;
};

G_DEFINE_DYNAMIC_TYPE_EXTENDED(XfdashboardClockView,
								xfdashboard_clock_view,
								XFDASHBOARD_TYPE_VIEW,
								0,
								G_ADD_PRIVATE_DYNAMIC(XfdashboardClockView))

/* Define this class in this plugin */
XFDASHBOARD_DEFINE_PLUGIN_TYPE(xfdashboard_clock_view);

/* IMPLEMENTATION: Private variables and methods */

/* Rectangle canvas should be redrawn */
static gboolean _xfdashboard_clock_view_on_draw_canvas(XfdashboardClockView *self,
														cairo_t *inContext,
														int inWidth,
														int inHeight,
														gpointer inUserData)
{
	XfdashboardClockViewPrivate		*priv;
	GDateTime						*now;
	gfloat							hours, minutes, seconds;

	g_return_val_if_fail(XFDASHBOARD_IS_CLOCK_VIEW(self), TRUE);
	g_return_val_if_fail(CLUTTER_IS_CANVAS(inUserData), TRUE);

	priv=self->priv;

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

	/* Scale the modelview to the size of the surface and
	 * center clock in view.
	 */
	if(inHeight<inWidth)
	{
		cairo_scale(inContext, inHeight, inHeight);
		cairo_translate(inContext, (inWidth/2.0f)/(gfloat)inHeight, 0.5f);
	}
		else
		{
			cairo_scale(inContext, inWidth, inWidth);
			cairo_translate(inContext, 0.5f, (inHeight/2.0f)/(gfloat)inWidth);
		}

	cairo_set_line_cap(inContext, CAIRO_LINE_CAP_ROUND);
	cairo_set_line_width(inContext, 0.1f);

	/* The blue circle that holds the seconds indicator */
	clutter_cairo_set_source_color(inContext,
									xfdashboard_clock_view_settings_get_background_color(priv->settings));
	cairo_arc(inContext, 0.0f, 0.0f, 0.4f, 0.0f, G_PI*2.0f);
	cairo_stroke(inContext);

	/* The seconds indicator */
	clutter_cairo_set_source_color(inContext,
									xfdashboard_clock_view_settings_get_second_color(priv->settings));
	cairo_move_to(inContext, 0.0f, 0.0f);
	cairo_arc(inContext, sinf(seconds)*0.4f, -cosf(seconds)*0.4f, 0.05f, 0.0f, G_PI*2);
	cairo_fill(inContext);

	/* The minutes indicator */
	clutter_cairo_set_source_color(inContext,
									xfdashboard_clock_view_settings_get_minute_color(priv->settings));
	cairo_move_to(inContext, 0.0f, 0.0f);
	cairo_line_to(inContext, sinf(minutes)*0.4f, -cosf(minutes)*0.4f);
	cairo_stroke(inContext);

	/* The hours indicator */
	clutter_cairo_set_source_color(inContext,
									xfdashboard_clock_view_settings_get_hour_color(priv->settings));
	cairo_move_to(inContext, 0.0f, 0.0f);
	cairo_line_to(inContext, sinf(hours)*0.2f, -cosf(hours)*0.2f);
	cairo_stroke(inContext);

	/* Done drawing */
	return(CLUTTER_EVENT_STOP);
}

/* Timeout source callback which invalidate clock canvas */
static gboolean _xfdashboard_clock_view_on_timeout(gpointer inUserData)
{
	XfdashboardClockView			*self;
	XfdashboardClockViewPrivate		*priv;

	g_return_val_if_fail(XFDASHBOARD_IS_CLOCK_VIEW(inUserData), FALSE);

	self=XFDASHBOARD_CLOCK_VIEW(inUserData);
	priv=self->priv;

	/* Invalidate clock canvas which force a redraw with current time */
	clutter_content_invalidate(CLUTTER_CONTENT(priv->clockCanvas));

	return(TRUE);
}

/* IMPLEMENTATION: XfdashboardView */

/* View was activated */
static void _xfdashboard_clock_view_activated(XfdashboardView *inView)
{
	XfdashboardClockView			*self;
	XfdashboardClockViewPrivate		*priv;

	g_return_if_fail(XFDASHBOARD_IS_CLOCK_VIEW(inView));

	self=XFDASHBOARD_CLOCK_VIEW(inView);
	priv=self->priv;

	/* Create timeout source will invalidate canvas each second */
	priv->timeoutID=clutter_threads_add_timeout(1000, _xfdashboard_clock_view_on_timeout, self);
}

/* View will be deactivated */
static void _xfdashboard_clock_view_deactivating(XfdashboardView *inView)
{
	XfdashboardClockView			*self;
	XfdashboardClockViewPrivate		*priv;

	g_return_if_fail(XFDASHBOARD_IS_CLOCK_VIEW(inView));

	self=XFDASHBOARD_CLOCK_VIEW(inView);
	priv=self->priv;

	/* Remove timeout source if available */
	if(priv->timeoutID)
	{
		g_source_remove(priv->timeoutID);
		priv->timeoutID=0;
	}
}

/* IMPLEMENTATION: ClutterActor */

/* Allocate position and size of actor and its children*/
static void _xfdashboard_clock_view_allocate(ClutterActor *inActor,
												const ClutterActorBox *inBox,
												ClutterAllocationFlags inFlags)
{
	XfdashboardClockView			*self=XFDASHBOARD_CLOCK_VIEW(inActor);
	XfdashboardClockViewPrivate		*priv=self->priv;

	/* Chain up to store the allocation of the actor */
	CLUTTER_ACTOR_CLASS(xfdashboard_clock_view_parent_class)->allocate(inActor, inBox, inFlags);

	/* Set size of actor and canvas */
	clutter_actor_allocate(priv->clockActor, inBox, inFlags);

	clutter_canvas_set_size(CLUTTER_CANVAS(priv->clockCanvas),
								clutter_actor_box_get_width(inBox),
								clutter_actor_box_get_height(inBox));
}

/* IMPLEMENTATION: GObject */

/* Dispose this object */
static void _xfdashboard_clock_view_dispose(GObject *inObject)
{
	XfdashboardClockView			*self=XFDASHBOARD_CLOCK_VIEW(inObject);
	XfdashboardClockViewPrivate		*priv=self->priv;

	/* Release allocated resources */
	if(priv->timeoutID)
	{
		g_source_remove(priv->timeoutID);
		priv->timeoutID=0;
	}

	if(priv->clockActor)
	{
		clutter_actor_destroy(priv->clockActor);
		priv->clockActor=NULL;
	}

	if(priv->clockCanvas)
	{
		g_object_unref(priv->clockCanvas);
		priv->clockCanvas=NULL;
	}

	if(priv->settings)
	{
		g_object_unref(priv->settings);
		priv->settings=NULL;
	}

	/* Call parent's class dispose method */
	G_OBJECT_CLASS(xfdashboard_clock_view_parent_class)->dispose(inObject);
}

/* Class initialization
 * Override functions in parent classes and define properties
 * and signals
 */
static void xfdashboard_clock_view_class_init(XfdashboardClockViewClass *klass)
{
	XfdashboardViewClass	*viewClass=XFDASHBOARD_VIEW_CLASS(klass);
	ClutterActorClass		*actorClass=CLUTTER_ACTOR_CLASS(klass);
	GObjectClass			*gobjectClass=G_OBJECT_CLASS(klass);

	/* Override functions */
	gobjectClass->dispose=_xfdashboard_clock_view_dispose;

	actorClass->allocate=_xfdashboard_clock_view_allocate;

	viewClass->activated=_xfdashboard_clock_view_activated;
	viewClass->deactivating=_xfdashboard_clock_view_deactivating;
}

/* Class finalization */
static void xfdashboard_clock_view_class_finalize(XfdashboardClockViewClass *klass)
{
}

/* Object initialization
 * Create private structure and set up default values
 */
void xfdashboard_clock_view_init(XfdashboardClockView *self)
{
	XfdashboardClockViewPrivate		*priv;

	self->priv=priv=xfdashboard_clock_view_get_instance_private(self);

	/* Set up default values */
	priv->timeoutID=0;

	/* Set up settings */
	priv->settings=xfdashboard_clock_view_settings_new();

	/* Set up this actor */
	xfdashboard_view_set_view_fit_mode(XFDASHBOARD_VIEW(self), XFDASHBOARD_VIEW_FIT_MODE_BOTH);

	priv->clockCanvas=clutter_canvas_new();
	clutter_canvas_set_size(CLUTTER_CANVAS(priv->clockCanvas), 100.0f, 100.0f);
	g_signal_connect_swapped(priv->clockCanvas, "draw", G_CALLBACK(_xfdashboard_clock_view_on_draw_canvas), self);

	priv->clockActor=clutter_actor_new();
	clutter_actor_show(priv->clockActor);
	clutter_actor_set_content(priv->clockActor, priv->clockCanvas);
	clutter_actor_set_size(priv->clockActor, 100.0f, 100.0f);
	clutter_actor_add_child(CLUTTER_ACTOR(self), priv->clockActor);

	/* Set up view */
	xfdashboard_view_set_name(XFDASHBOARD_VIEW(self), _("Clock"));
	xfdashboard_view_set_icon(XFDASHBOARD_VIEW(self), "appointment-soon");
}
