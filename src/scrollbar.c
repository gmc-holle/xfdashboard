/*
 * scrollbar: A scroll bar
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

#include "scrollbar.h"

/* Define this class in GObject system */
G_DEFINE_TYPE(XfdashboardScrollbar,
				xfdashboard_scrollbar,
				XFDASHBOARD_TYPE_BACKGROUND)

/* Private structure - access only by public API if needed */
#define XFDASHBOARD_SCROLLBAR_GET_PRIVATE(obj) \
	(G_TYPE_INSTANCE_GET_PRIVATE((obj), XFDASHBOARD_TYPE_SCROLLBAR, XfdashboardScrollbarPrivate))

struct _XfdashboardScrollbarPrivate
{
	/* Properties related */
	ClutterOrientation		orientation;
	gfloat					value;
	gfloat					valueRange;
	gfloat					range;
	gfloat					spacing;
	gfloat					sliderWidth;
	gfloat					sliderRadius;
	ClutterColor			*sliderColor;

	/* Instance related */
	ClutterContent			*slider;
};

/* Properties */
enum
{
	PROP_0,

	PROP_ORIENTATION,
	PROP_VALUE,
	PROP_RANGE,
	PROP_SPACING,
	PROP_SLIDER_WIDTH,
	PROP_SLIDER_RADIUS,
	PROP_SLIDER_COLOR,

	PROP_LAST
};

GParamSpec* XfdashboardScrollbarProperties[PROP_LAST]={ 0, };

/* Signals */
enum
{
	SIGNAL_VALUE_CHANGED,

	SIGNAL_LAST
};

guint XfdashboardScrollbarSignals[SIGNAL_LAST]={ 0, };

/* IMPLEMENTATION: Private variables and methods */
#define DEFAULT_SPACING			2.0f
#define DEFAULT_SLIDER_WIDTH	4.0f
#define DEFAULT_SLIDER_RADIUS	(DEFAULT_SLIDER_WIDTH/2.0f)
#define DEFAULT_ORIENTATION		CLUTTER_ORIENTATION_HORIZONTAL

ClutterColor		defaultSliderColor={ 0xff, 0xff, 0xff, 0xff };

/* Rectangle canvas should be redrawn */
gboolean _xfdashboard_scrollbar_on_draw_slider(XfdashboardScrollbar *self,
												cairo_t *inContext,
												int inWidth,
												int inHeight,
												gpointer inUserData)
{
	g_return_val_if_fail(XFDASHBOARD_IS_SCROLLBAR(self), TRUE);
	g_return_val_if_fail(CLUTTER_IS_CANVAS(inUserData), TRUE);

	XfdashboardScrollbarPrivate		*priv=self->priv;
	gdouble							radius;
	gdouble							top, left, bottom, right;
	gdouble							barPosition, barSize;
	gdouble							sliderWidth, sliderHeight;

	/* Clear current contents of the canvas */
	cairo_save(inContext);
	cairo_set_operator(inContext, CAIRO_OPERATOR_CLEAR);
	cairo_paint(inContext);
	cairo_restore(inContext);

	cairo_set_operator(inContext, CAIRO_OPERATOR_OVER);

	/* Set color for slider */
	if(priv->sliderColor) clutter_cairo_set_source_color(inContext, priv->sliderColor);

	/* Determine radius for rounded corners */
	radius=MIN(priv->sliderRadius, inWidth/2.0f);
	radius=MIN(radius, inHeight/2.0f);

	/* Calculate bounding coordinates for slider */
	sliderWidth=MAX(0, inWidth-(2*priv->spacing));
	sliderHeight=MAX(0, inHeight-(2*priv->spacing));

	if(priv->orientation==CLUTTER_ORIENTATION_HORIZONTAL)
	{
		if(priv->range>sliderWidth) barSize=(sliderWidth/priv->range)*sliderWidth;
			else barSize=sliderWidth;

		barPosition=MIN(MAX(0, (priv->value/priv->range)*sliderWidth), sliderWidth);
		if(barPosition+barSize>sliderWidth) barPosition=sliderWidth-barSize;

		top=priv->spacing;
		bottom=(gdouble)sliderHeight;
		left=barPosition;
		right=barPosition+barSize;
	}
		else
		{
			if(priv->range>sliderHeight) barSize=(sliderHeight/priv->range)*sliderHeight;
				else barSize=sliderHeight;

			barPosition=MIN(MAX(0, (priv->value/priv->range)*sliderHeight), sliderHeight);
			if(barPosition+barSize>sliderHeight) barPosition=sliderHeight-barSize;

			left=priv->spacing;
			right=sliderWidth;
			top=barPosition;
			bottom=barPosition+barSize;
		}

	/* Draw slider */
	if(radius>0.0f)
	{
		cairo_move_to(inContext, left, top+radius);
		cairo_arc(inContext, left+radius, top+radius, radius, G_PI, G_PI*1.5);

		cairo_line_to(inContext, right-radius, 0);
		cairo_arc(inContext, right-radius, top+radius, radius, G_PI*1.5, 0);

		cairo_line_to(inContext, right, bottom-radius);
		cairo_arc(inContext, right-radius, bottom-radius, radius, 0, G_PI/2.0);

		cairo_line_to(inContext, left+radius, bottom);
		cairo_arc(inContext, left+radius, bottom-radius, radius, G_PI/2.0, G_PI);

		cairo_line_to(inContext, left, radius);
	}
		else
		{
			cairo_rectangle(inContext, left, top, right, bottom);
		}

	cairo_fill(inContext);

	/* Done drawing */
	return(TRUE);
}

/* IMPLEMENTATION: ClutterActor */

/* Get preferred width/height */
void _xfdashboard_scrollbar_get_preferred_height(ClutterActor *self,
													gfloat inForWidth,
													gfloat *outMinHeight,
													gfloat *outNaturalHeight)
{
	XfdashboardScrollbarPrivate		*priv=XFDASHBOARD_SCROLLBAR(self)->priv;
	gfloat							minHeight, naturalHeight;

	minHeight=naturalHeight=0.0f;

	/* Determine sizes */
	if(priv->orientation==CLUTTER_ORIENTATION_HORIZONTAL)
	{
		minHeight=naturalHeight=(2*priv->spacing)+priv->sliderWidth;
	}
		else
		{
			/* Call parent's class method to determine sizes */
			ClutterActorClass		*actorClass=CLUTTER_ACTOR_CLASS(xfdashboard_scrollbar_parent_class);

			if(actorClass->get_preferred_height)
			{
				actorClass->get_preferred_height(self,
												inForWidth,
												&minHeight,
												&naturalHeight);
			}

			/* If parent's class method did not set sizes set defaults */
			if(minHeight==0.0f || naturalHeight==0.0f)
			{
				minHeight=naturalHeight=(2*priv->spacing)+priv->sliderWidth;
			}
		}

	/* Store sizes computed */
	if(outMinHeight) *outMinHeight=minHeight;
	if(outNaturalHeight) *outNaturalHeight=naturalHeight;
}

void _xfdashboard_scrollbar_get_preferred_width(ClutterActor *self,
												gfloat inForHeight,
												gfloat *outMinWidth,
												gfloat *outNaturalWidth)
{
	XfdashboardScrollbarPrivate		*priv=XFDASHBOARD_SCROLLBAR(self)->priv;
	gfloat							minWidth, naturalWidth;

	minWidth=naturalWidth=0.0f;

	/* Determine sizes */
	if(priv->orientation==CLUTTER_ORIENTATION_VERTICAL)
	{
		minWidth=naturalWidth=(2*priv->spacing)+priv->sliderWidth;
	}
		else
		{
			/* Call parent's class method to determine sizes */
			ClutterActorClass		*actorClass=CLUTTER_ACTOR_CLASS(xfdashboard_scrollbar_parent_class);

			if(actorClass->get_preferred_width)
			{
				actorClass->get_preferred_width(self,
												inForHeight,
												&minWidth,
												&naturalWidth);
			}

			/* If parent's class method did not set sizes set defaults */
			if(minWidth==0.0f || naturalWidth==0.0f)
			{
				minWidth=naturalWidth=(2*priv->spacing)+priv->sliderWidth;
			}
		}

	/* Store sizes computed */
	if(outMinWidth) *outMinWidth=minWidth;
	if(outNaturalWidth) *outNaturalWidth=naturalWidth;
}

/* Allocate position and size of actor and its children*/
void _xfdashboard_scrollbar_allocate(ClutterActor *self,
										const ClutterActorBox *inBox,
										ClutterAllocationFlags inFlags)
{
	XfdashboardScrollbarPrivate		*priv=XFDASHBOARD_SCROLLBAR(self)->priv;

	/* Chain up to store the allocation of the actor */
	CLUTTER_ACTOR_CLASS(xfdashboard_scrollbar_parent_class)->allocate(self, inBox, inFlags);

	/* Set size of slider canvas */
	clutter_canvas_set_size(CLUTTER_CANVAS(priv->slider),
								clutter_actor_box_get_width(inBox),
								clutter_actor_box_get_height(inBox));
}

/* IMPLEMENTATION: GObject */

/* Dispose this object */
void _xfdashboard_scrollbar_dispose(GObject *inObject)
{
	XfdashboardScrollbar			*self=XFDASHBOARD_SCROLLBAR(inObject);
	XfdashboardScrollbarPrivate		*priv=self->priv;

	/* Release allocated resources */
	if(priv->sliderColor)
	{
		clutter_color_free(priv->sliderColor);
		priv->sliderColor=NULL;
	}

	if(priv->slider)
	{
		g_object_unref(priv->slider);
		priv->slider=NULL;
	}

	/* Call parent's class dispose method */
	G_OBJECT_CLASS(xfdashboard_scrollbar_parent_class)->dispose(inObject);
}

/* Set/get properties */
void _xfdashboard_scrollbar_set_property(GObject *inObject,
												guint inPropID,
												const GValue *inValue,
												GParamSpec *inSpec)
{
	XfdashboardScrollbar		*self=XFDASHBOARD_SCROLLBAR(inObject);
	
	switch(inPropID)
	{
		case PROP_ORIENTATION:
			xfdashboard_scrollbar_set_orientation(self, g_value_get_enum(inValue));
			break;

		case PROP_VALUE:
			xfdashboard_scrollbar_set_value(self, g_value_get_float(inValue));
			break;

		case PROP_RANGE:
			xfdashboard_scrollbar_set_range(self, g_value_get_float(inValue));
			break;

		case PROP_SPACING:
			xfdashboard_scrollbar_set_spacing(self, g_value_get_float(inValue));
			break;

		case PROP_SLIDER_WIDTH:
			xfdashboard_scrollbar_set_slider_width(self, g_value_get_float(inValue));
			break;

		case PROP_SLIDER_RADIUS:
			xfdashboard_scrollbar_set_slider_radius(self, g_value_get_float(inValue));
			break;

		case PROP_SLIDER_COLOR:
			xfdashboard_scrollbar_set_slider_color(self, clutter_value_get_color(inValue));
			break;

		default:
			G_OBJECT_WARN_INVALID_PROPERTY_ID(inObject, inPropID, inSpec);
			break;
	}
}

void _xfdashboard_scrollbar_get_property(GObject *inObject,
											guint inPropID,
											GValue *outValue,
											GParamSpec *inSpec)
{
	XfdashboardScrollbar		*self=XFDASHBOARD_SCROLLBAR(inObject);

	switch(inPropID)
	{
		case PROP_ORIENTATION:
			g_value_set_enum(outValue, self->priv->orientation);
			break;

		case PROP_VALUE:
			g_value_set_float(outValue, self->priv->value);
			break;

		case PROP_RANGE:
			g_value_set_float(outValue, self->priv->range);
			break;

		case PROP_SPACING:
			g_value_set_float(outValue, self->priv->spacing);
			break;

		case PROP_SLIDER_WIDTH:
			g_value_set_float(outValue, self->priv->sliderWidth);
			break;

		case PROP_SLIDER_RADIUS:
			g_value_set_float(outValue, self->priv->sliderRadius);
			break;

		case PROP_SLIDER_COLOR:
			clutter_value_set_color(outValue, self->priv->sliderColor);
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
void xfdashboard_scrollbar_class_init(XfdashboardScrollbarClass *klass)
{
	ClutterActorClass		*actorClass=CLUTTER_ACTOR_CLASS(klass);
	GObjectClass			*gobjectClass=G_OBJECT_CLASS(klass);

	/* Override functions */
	gobjectClass->set_property=_xfdashboard_scrollbar_set_property;
	gobjectClass->get_property=_xfdashboard_scrollbar_get_property;
	gobjectClass->dispose=_xfdashboard_scrollbar_dispose;

	actorClass->get_preferred_width=_xfdashboard_scrollbar_get_preferred_width;
	actorClass->get_preferred_height=_xfdashboard_scrollbar_get_preferred_height;
	actorClass->allocate=_xfdashboard_scrollbar_allocate;

	/* Set up private structure */
	g_type_class_add_private(klass, sizeof(XfdashboardScrollbarPrivate));

	/* Define properties */
	XfdashboardScrollbarProperties[PROP_ORIENTATION]=
		g_param_spec_enum("orientation",
							_("Orientation"),
							_("Defines if scrollbar is horizontal or vertical"),
							CLUTTER_TYPE_ORIENTATION,
							DEFAULT_ORIENTATION,
							G_PARAM_READWRITE);

	XfdashboardScrollbarProperties[PROP_VALUE]=
		g_param_spec_float("value",
							_("Value"),
							_("Current value of scroll bar within range"),
							0.0f, G_MAXFLOAT,
							0.0f,
							G_PARAM_READWRITE);

	XfdashboardScrollbarProperties[PROP_RANGE]=
		g_param_spec_float("range",
							_("Range"),
							_("Range to scroll within"),
							0.0f, G_MAXFLOAT,
							1.0f,
							G_PARAM_READWRITE);

	XfdashboardScrollbarProperties[PROP_SPACING]=
		g_param_spec_float("spacing",
							_("Spacing"),
							_("The spacing between scrollbar and background"),
							0.0f, G_MAXFLOAT,
							DEFAULT_SPACING,
							G_PARAM_READWRITE);

	XfdashboardScrollbarProperties[PROP_SLIDER_WIDTH]=
		g_param_spec_float("slider-width",
							_("Slider width"),
							_("The width of slider"),
							1.0f, G_MAXFLOAT,
							DEFAULT_SLIDER_WIDTH,
							G_PARAM_READWRITE);

	XfdashboardScrollbarProperties[PROP_SLIDER_RADIUS]=
		g_param_spec_float("slider-radius",
							_("Slider radius"),
							_("The radius of slider's rounded corners"),
							0.0f, G_MAXFLOAT,
							DEFAULT_SLIDER_RADIUS,
							G_PARAM_READWRITE);

	XfdashboardScrollbarProperties[PROP_SLIDER_COLOR]=
		clutter_param_spec_color("slider-color",
									_("Slider color"),
									_("Color of slider"),
									&defaultSliderColor,
									G_PARAM_READWRITE | G_PARAM_CONSTRUCT);

	g_object_class_install_properties(gobjectClass, PROP_LAST, XfdashboardScrollbarProperties);

	/* Define signals */
	XfdashboardScrollbarSignals[SIGNAL_VALUE_CHANGED]=
		g_signal_new("value-changed",
						G_TYPE_FROM_CLASS(klass),
						G_SIGNAL_RUN_LAST,
						G_STRUCT_OFFSET(XfdashboardScrollbarClass, value_changed),
						NULL,
						NULL,
						g_cclosure_marshal_VOID__FLOAT,
						G_TYPE_NONE,
						1,
						G_TYPE_FLOAT);
}

/* Object initialization
 * Create private structure and set up default values
 */
void xfdashboard_scrollbar_init(XfdashboardScrollbar *self)
{
	XfdashboardScrollbarPrivate		*priv;

	priv=self->priv=XFDASHBOARD_SCROLLBAR_GET_PRIVATE(self);

	/* Set up default values */
	priv->orientation=CLUTTER_ORIENTATION_HORIZONTAL;
	priv->value=0.0f;
	priv->valueRange=0.0f;
	priv->range=1.0f;
	priv->spacing=DEFAULT_SPACING;
	priv->sliderWidth=DEFAULT_SLIDER_WIDTH;
	priv->sliderRadius=DEFAULT_SLIDER_RADIUS;
	priv->sliderColor=NULL;
	priv->slider=clutter_canvas_new();

	/* Set up actor */
	clutter_actor_set_reactive(CLUTTER_ACTOR(self), TRUE);
	clutter_actor_set_content(CLUTTER_ACTOR(self), priv->slider);
	if(priv->orientation==CLUTTER_ORIENTATION_HORIZONTAL) clutter_actor_set_request_mode(CLUTTER_ACTOR(self), CLUTTER_REQUEST_HEIGHT_FOR_WIDTH);
		else clutter_actor_set_request_mode(CLUTTER_ACTOR(self), CLUTTER_REQUEST_WIDTH_FOR_HEIGHT);

	/* Connect signals */
	g_signal_connect_swapped(priv->slider, "draw", G_CALLBACK(_xfdashboard_scrollbar_on_draw_slider), self);
}

/* Implementation: Public API */

/* Create new actor */
ClutterActor* xfdashboard_scrollbar_new(ClutterOrientation inOrientation)
{
	return(g_object_new(XFDASHBOARD_TYPE_SCROLLBAR,
							"orientation", inOrientation,
							NULL));
}

/* Get/set orientation */
gfloat xfdashboard_scrollbar_get_orientation(XfdashboardScrollbar *self)
{
	g_return_val_if_fail(XFDASHBOARD_IS_SCROLLBAR(self), DEFAULT_ORIENTATION);

	return(self->priv->spacing);
}

void xfdashboard_scrollbar_set_orientation(XfdashboardScrollbar *self, ClutterOrientation inOrientation)
{
	g_return_if_fail(XFDASHBOARD_IS_SCROLLBAR(self));
	g_return_if_fail(inOrientation==CLUTTER_ORIENTATION_HORIZONTAL || inOrientation==CLUTTER_ORIENTATION_VERTICAL);

	XfdashboardScrollbarPrivate		*priv=self->priv;

	/* Only set value if it changes */
	if(inOrientation==priv->orientation) return;

	/* Set new value */
	priv->orientation=inOrientation;

	if(priv->orientation==CLUTTER_ORIENTATION_HORIZONTAL) clutter_actor_set_request_mode(CLUTTER_ACTOR(self), CLUTTER_REQUEST_HEIGHT_FOR_WIDTH);
		else clutter_actor_set_request_mode(CLUTTER_ACTOR(self), CLUTTER_REQUEST_WIDTH_FOR_HEIGHT);

	if(priv->slider) clutter_content_invalidate(priv->slider);
	clutter_actor_queue_relayout(CLUTTER_ACTOR(self));

	/* Notify about property change */
	g_object_notify_by_pspec(G_OBJECT(self), XfdashboardScrollbarProperties[PROP_ORIENTATION]);
}

/* Get/set value */
gfloat xfdashboard_scrollbar_get_value(XfdashboardScrollbar *self)
{
	g_return_val_if_fail(XFDASHBOARD_IS_SCROLLBAR(self), 0.0f);

	return(self->priv->value);
}

void xfdashboard_scrollbar_set_value(XfdashboardScrollbar *self, gfloat inValue)
{
	g_return_if_fail(XFDASHBOARD_IS_SCROLLBAR(self));
	g_return_if_fail(inValue>=0.0f);

	XfdashboardScrollbarPrivate		*priv=self->priv;

	/* Check if value is within range */
	if(inValue>priv->range)
	{
		g_warning(_("Adjusting value %.2f in scrollbar to fit range %.2f"), inValue, priv->range);
		inValue=priv->range;
	}

	/* Only set value if it changes */
	if(inValue==priv->value) return;

	/* Set new value */
	priv->value=inValue;
	if(priv->slider) clutter_content_invalidate(priv->slider);
	clutter_actor_queue_redraw(CLUTTER_ACTOR(self));

	/* Notify about property change */
	g_object_notify_by_pspec(G_OBJECT(self), XfdashboardScrollbarProperties[PROP_VALUE]);
}

/* Get value range */
gfloat xfdashboard_scrollbar_get_value_range(XfdashboardScrollbar *self)
{
	g_return_val_if_fail(XFDASHBOARD_IS_SCROLLBAR(self), 0.0f);

	return(self->priv->valueRange);
}

/* Get/set range */
gfloat xfdashboard_scrollbar_get_range(XfdashboardScrollbar *self)
{
	g_return_val_if_fail(XFDASHBOARD_IS_SCROLLBAR(self), 0.0f);

	return(self->priv->range);
}

void xfdashboard_scrollbar_set_range(XfdashboardScrollbar *self, gfloat inRange)
{
	g_return_if_fail(XFDASHBOARD_IS_SCROLLBAR(self));
	g_return_if_fail(inRange>=0.0f);

	XfdashboardScrollbarPrivate		*priv=self->priv;

	/* Only set value if it changes */
	if(inRange==priv->range) return;

	/* Freeze notification */
	g_object_freeze_notify(G_OBJECT(self));

	/* Set new value */
	priv->range=inRange;
	if(priv->slider) clutter_content_invalidate(priv->slider);
	clutter_actor_queue_redraw(CLUTTER_ACTOR(self));

	/* Notify about property change */
	g_object_notify_by_pspec(G_OBJECT(self), XfdashboardScrollbarProperties[PROP_RANGE]);

	/* Check if value is still within new range otherwise adjust value */
	if(priv->value>priv->range)
	{
		g_warning(_("Adjusting value %.2f in scrollbar to fit new range %.2f"), priv->value, priv->range);
		xfdashboard_scrollbar_set_value(self, priv->range);
	}

	/* Thaw notification */
	g_object_thaw_notify(G_OBJECT(self));
}

/* Get/set spacing */
gfloat xfdashboard_scrollbar_get_spacing(XfdashboardScrollbar *self)
{
	g_return_val_if_fail(XFDASHBOARD_IS_SCROLLBAR(self), 0.0f);

	return(self->priv->spacing);
}

void xfdashboard_scrollbar_set_spacing(XfdashboardScrollbar *self, gfloat inSpacing)
{
	g_return_if_fail(XFDASHBOARD_IS_SCROLLBAR(self));
	g_return_if_fail(inSpacing>=0.0f);

	XfdashboardScrollbarPrivate	*priv=self->priv;

	/* Only set value if it changes */
	if(inSpacing==priv->spacing) return;

	/* Set new value */
	priv->spacing=inSpacing;
	clutter_actor_queue_relayout(CLUTTER_ACTOR(self));

	/* Notify about property change */
	g_object_notify_by_pspec(G_OBJECT(self), XfdashboardScrollbarProperties[PROP_SPACING]);
}

/* Get/set slider width (thickness) */
gfloat xfdashboard_scrollbar_get_slider_width(XfdashboardScrollbar *self)
{
	g_return_val_if_fail(XFDASHBOARD_IS_SCROLLBAR(self), 0.0f);

	return(self->priv->sliderWidth);
}

void xfdashboard_scrollbar_set_slider_width(XfdashboardScrollbar *self, gfloat inWidth)
{
	g_return_if_fail(XFDASHBOARD_IS_SCROLLBAR(self));
	g_return_if_fail(inWidth>=1.0f);

	XfdashboardScrollbarPrivate	*priv=self->priv;

	/* Only set value if it changes */
	if(inWidth==priv->sliderWidth) return;

	/* Set new value */
	priv->sliderWidth=inWidth;
	clutter_actor_queue_relayout(CLUTTER_ACTOR(self));

	/* Notify about property change */
	g_object_notify_by_pspec(G_OBJECT(self), XfdashboardScrollbarProperties[PROP_SLIDER_WIDTH]);
}

/* Get/set radius of rounded corners of slider */
gfloat xfdashboard_scrollbar_get_slider_radius(XfdashboardScrollbar *self)
{
	g_return_val_if_fail(XFDASHBOARD_IS_SCROLLBAR(self), 0.0f);

	return(self->priv->sliderWidth);
}

void xfdashboard_scrollbar_set_slider_radius(XfdashboardScrollbar *self, gfloat inRadius)
{
	g_return_if_fail(XFDASHBOARD_IS_SCROLLBAR(self));
	g_return_if_fail(inRadius>=0.0f);

	XfdashboardScrollbarPrivate	*priv=self->priv;

	/* Only set value if it changes */
	if(inRadius==priv->sliderRadius) return;

	/* Set new value */
	priv->sliderRadius=inRadius;
	if(priv->slider) clutter_content_invalidate(priv->slider);

	/* Notify about property change */
	g_object_notify_by_pspec(G_OBJECT(self), XfdashboardScrollbarProperties[PROP_SLIDER_RADIUS]);
}

/* Get/set color of slider */
const ClutterColor* xfdashboard_scrollbar_get_slider_color(XfdashboardScrollbar *self)
{
	g_return_val_if_fail(XFDASHBOARD_IS_SCROLLBAR(self), NULL);

	return(self->priv->sliderColor);
}

void xfdashboard_scrollbar_set_slider_color(XfdashboardScrollbar *self, const ClutterColor *inColor)
{
	g_return_if_fail(XFDASHBOARD_IS_SCROLLBAR(self));
	g_return_if_fail(inColor);

	XfdashboardScrollbarPrivate	*priv=self->priv;

	/* Set value if changed */
	if(priv->sliderColor==NULL || clutter_color_equal(inColor, priv->sliderColor)==FALSE)
	{
		/* Set value */
		if(priv->sliderColor) clutter_color_free(priv->sliderColor);
		priv->sliderColor=clutter_color_copy(inColor);

		/* Invalidate canvas to get it redrawn */
		if(priv->slider) clutter_content_invalidate(priv->slider);

		/* Notify about property change */
		g_object_notify_by_pspec(G_OBJECT(self), XfdashboardScrollbarProperties[PROP_SLIDER_COLOR]);
	}
}
