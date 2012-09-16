/*
 * scrollbar.c: A scroll bar
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

#include "scrollbar.h"

/* Define this class in GObject system */
G_DEFINE_TYPE(XfdashboardScrollbar,
				xfdashboard_scrollbar,
				CLUTTER_TYPE_ACTOR)

/* Private structure - access only by public API if needed */
#define XFDASHBOARD_SCROLLBAR_GET_PRIVATE(obj) \
	(G_TYPE_INSTANCE_GET_PRIVATE((obj), XFDASHBOARD_TYPE_SCROLLBAR, XfdashboardScrollbarPrivate))

struct _XfdashboardScrollbarPrivate
{
	/* Scrolling information */
	gfloat			value;
	gfloat			range;

	/* Temporary data (e.g. used while handling an event) */
	gfloat			eventOffset;
	gulong			signalMotionEventID;
	gulong			signalButtonReleasedID;
	
	/* Settings */
	gboolean		isVertical;
	gfloat			thickness;
	gfloat			spacing;

	ClutterColor	*scrollbarColor;
	ClutterColor	*backgroundColor;
};

/* Properties */
enum
{
	PROP_0,

	PROP_VALUE,
	PROP_RANGE,

	PROP_VERTICAL,
	PROP_THICKNESS,
	PROP_SPACING,
	PROP_COLOR,
	PROP_BACKGROUND_COLOR,

	PROP_LAST
};

static GParamSpec* XfdashboardScrollbarProperties[PROP_LAST]={ 0, };

/* Signals */
enum
{
	VALUE_CHANGED,
	RANGE_CHANGED,

	SIGNAL_LAST
};

static guint XfdashboardScrollbarSignals[SIGNAL_LAST]={ 0, };

/* IMPLEMENTATION: Private variables and methods */
#define DEFAULT_THICKNESS		4.0f
#define DEFAULT_SPACING			4.0f

static ClutterColor		_xfdashboard_scrollbar_default_scrollbar_color=
							{ 0xff, 0xff, 0xff, 0xff };

static ClutterColor		_xfdashboard_scrollbar_default_background_color=
							{ 0xff, 0xff, 0xff, 0x40 };

/* Translate global coords to actor's relative coords, calculate value,
 * set new value and emit signal
 */
static gboolean _xfdashboard_scrollbar_set_value_by_coords(XfdashboardScrollbar *self,
															gfloat inGlobalX,
															gfloat inGlobalY)
{
	g_return_val_if_fail(XFDASHBOARD_IS_SCROLLBAR(self), FALSE);

	XfdashboardScrollbarPrivate		*priv=self->priv;
	gfloat							x, y;
	ClutterActorBox					allocation;
	gfloat							width, height;
	gfloat							value;
	gfloat							barSize;

	/* Get coordinates where click happened and translate to client's
	 * relative coordinates (including a possible offset)
	 */
	if(!clutter_actor_transform_stage_point(CLUTTER_ACTOR(self), inGlobalX, inGlobalY, &x, &y))
	{
		g_error("Could not transform click coordinates [%.0f,%.0f] to relative coordinates of actor %p (%s)",
					inGlobalX, inGlobalY, (void*)self, G_OBJECT_TYPE_NAME(self));
		return(FALSE);
	}

	if(priv->isVertical) y+=priv->eventOffset;
		else x+=priv->eventOffset;

	/* Get allocation to calculate new value to set */
	clutter_actor_get_allocation_box(CLUTTER_ACTOR(self), &allocation);
	clutter_actor_box_get_size(&allocation, &width, &height);

	/* Calculate new value and adjust it to fit into range */
	if(priv->isVertical && height>0)
	{
		if(priv->range>height) barSize=(height/priv->range)*height;
			else barSize=height;

		if(y+barSize>height) y=height-barSize;

		value=MAX((y/height)*priv->range, 0);
	}
		else if(!priv->isVertical && width>0)
		{
			if(priv->range>width) barSize=(width/priv->range)*width;
				else barSize=width;

			if(x+barSize>width) x=width-barSize;

			value=MAX((x/width)*priv->range, 0);
		}
			/* To prevent setting a NAN (infinite) value set it to
			 * current one
			 */
			else value=priv->value;

	/* Set new value if it differs from old one */
	if(value!=priv->value) xfdashboard_scrollbar_set_value(self, value);

	return(TRUE);
}

/* A scroll event occured in scroll bar (e.g. by mouse-wheel) */
static gboolean _xfdashboard_scrollbar_scroll_event(ClutterActor *inActor,
														ClutterEvent *inEvent,
														gpointer inUserData)
{
	XfdashboardScrollbarPrivate		*priv=XFDASHBOARD_SCROLLBAR(inActor)->priv;
	gfloat							directionFactor;
	ClutterActorBox					allocation;
	gfloat							width, height;
	gfloat							barSize;
	gfloat							newValue, maxValue;

	/* Get direction of scroll event */
	switch(clutter_event_get_scroll_direction(inEvent))
	{
		case CLUTTER_SCROLL_UP:
		case CLUTTER_SCROLL_LEFT:
			directionFactor=-1.0f;
			break;

		case CLUTTER_SCROLL_DOWN:
		case CLUTTER_SCROLL_RIGHT:
			directionFactor=1.0f;
			break;

		/* Unhandled directions */
		default:
			return(FALSE);
	}
	
	/* Determine new value by increasing or decreasing value by scroll
	 * step size. Adjust new value to fit range.
	 */
	clutter_actor_get_allocation_box(CLUTTER_ACTOR(inActor), &allocation);
	clutter_actor_box_get_size(&allocation, &width, &height);

	if(priv->isVertical)
	{
		if(priv->range>0.0f && priv->range>height) barSize=(height/priv->range)*height;
			else barSize=height;

		if(height>0.0f) maxValue=((height-barSize)/height)*priv->range;
			else maxValue=0.0f;
	}
		else
		{
			if(priv->range>0.0f && priv->range>width) barSize=(width/priv->range)*width;
				else barSize=width;

			if(width>0.0f) maxValue=((width-barSize)/width)*priv->range;
				else maxValue=0.0f;
		}

	maxValue=MAX(0, MIN(priv->range, maxValue));
	
	newValue=priv->value+((barSize/2.0f)*directionFactor);
	newValue=MAX(0, MIN(newValue, maxValue));

	/* Set new value if it differs from old one */
	if(newValue!=priv->value)
		xfdashboard_scrollbar_set_value(XFDASHBOARD_SCROLLBAR(inActor), newValue);

	return(TRUE);
}

/* Pointer moved inside scroll bar (usually called after button-pressed event) */
static gboolean _xfdashboard_scrollbar_motion_event(ClutterActor *inActor,
														ClutterEvent *inEvent,
														gpointer inUserData)
{
	/* Get coords where event happened and set value*/
	gfloat				eventX, eventY;

	clutter_event_get_coords(inEvent, &eventX, &eventY);
	_xfdashboard_scrollbar_set_value_by_coords(XFDASHBOARD_SCROLLBAR(inActor), eventX, eventY);

	return(TRUE);
}

/* User released button in scroll bar */
static gboolean _xfdashboard_scrollbar_button_released(ClutterActor *inActor,
														ClutterEvent *inEvent,
														gpointer inUserData)
{
	g_return_val_if_fail(XFDASHBOARD_IS_SCROLLBAR(inActor), FALSE);
	g_return_val_if_fail(inEvent, FALSE);

	XfdashboardScrollbarPrivate		*priv=XFDASHBOARD_SCROLLBAR(inActor)->priv;

	/* If user did not release a left-click do nothing */
	if(clutter_event_get_button(inEvent)!=1) return(FALSE);

	/* Release exclusive handling motiven and button-release events by this actor */
	if(priv->signalMotionEventID) g_signal_handler_disconnect(inActor, priv->signalMotionEventID);
	priv->signalMotionEventID=0L;
	
	if(priv->signalButtonReleasedID) g_signal_handler_disconnect(inActor, priv->signalButtonReleasedID);
	priv->signalButtonReleasedID=0L;
	
	clutter_ungrab_pointer();

	/* Get coords where event happened and set value*/
	gfloat					eventX, eventY;

	clutter_event_get_coords(inEvent, &eventX, &eventY);
	_xfdashboard_scrollbar_set_value_by_coords(XFDASHBOARD_SCROLLBAR(inActor), eventX, eventY);

	return(TRUE);
}

/* User pressed button in scroll bar */
static gboolean _xfdashboard_scrollbar_button_pressed(ClutterActor *inActor,
														ClutterEvent *inEvent,
														gpointer inUserData)
{
	g_return_val_if_fail(XFDASHBOARD_IS_SCROLLBAR(inActor), FALSE);
	g_return_val_if_fail(inEvent, FALSE);

	XfdashboardScrollbarPrivate		*priv=XFDASHBOARD_SCROLLBAR(inActor)->priv;
	gfloat							eventX, eventY;
	gfloat							x, y;
	ClutterActorBox					allocation;
	gfloat							width, height;
	gfloat							barPosition, barSize;

	/* If user left-clicked into scroll bar adjust value to point
	 * where the click happened
	 */
	if(clutter_event_get_button(inEvent)!=1) return(FALSE);

	/* Get coords where event happened. If coords are not in bar handle set value
	 * otherwise only determine offset used in event handlers and do not adjust
	 * value as this would cause a movement of bar handle */
	clutter_event_get_coords(inEvent, &eventX, &eventY);
	if(!clutter_actor_transform_stage_point(CLUTTER_ACTOR(inActor), eventX, eventY, &x, &y))
		return(FALSE);

	clutter_actor_get_allocation_box(CLUTTER_ACTOR(inActor), &allocation);
	clutter_actor_box_get_size(&allocation, &width, &height);
	
	priv->eventOffset=0.0f;
	if(priv->isVertical)
	{
		if(priv->range>height) barSize=(height/priv->range)*height;
			else barSize=height;

		barPosition=MIN(MAX(0, (priv->value/priv->range)*height), height);
		if(barPosition+barSize>height) barPosition=height-barSize;

		if(y<barPosition || y>(barPosition+barSize))
		{
			priv->eventOffset=-barSize/2.0f;
			if(!_xfdashboard_scrollbar_set_value_by_coords(XFDASHBOARD_SCROLLBAR(inActor), eventX, eventY))
				return(TRUE);
		}
			else priv->eventOffset=barPosition-y;
	}
		else
		{
			if(priv->range>width) barSize=(width/priv->range)*width;
				else barSize=width;

			barPosition=MIN(MAX(0, (priv->value/priv->range)*width), width);
			if(barPosition+barSize>width) barPosition=width-barSize;

			if(x<barPosition || x>(barPosition+barSize))
			{
				priv->eventOffset=-barSize/2.0f;
				if(!_xfdashboard_scrollbar_set_value_by_coords(XFDASHBOARD_SCROLLBAR(inActor), eventX, eventY))
					return(TRUE);
			}
				else priv->eventOffset=barPosition-x;
		}

	/* Connect signals for motion and button-release events and
	 * handle them in this actor exclusively
	 */
	priv->signalMotionEventID=g_signal_connect_after(inActor,
														"motion-event",
														G_CALLBACK(_xfdashboard_scrollbar_motion_event),
														NULL);
	priv->signalButtonReleasedID=g_signal_connect_after(inActor,
														"button-release-event",
														G_CALLBACK(_xfdashboard_scrollbar_button_released),
														NULL);
	clutter_grab_pointer(inActor);

	return(TRUE);
}

/* IMPLEMENTATION: ClutterActor */

/* Paint actor */
static void xfdashboard_scrollbar_paint(ClutterActor *inActor)
{
	XfdashboardScrollbarPrivate		*priv=XFDASHBOARD_SCROLLBAR(inActor)->priv;
	ClutterActorBox					allocation={ 0, };
	gfloat							width, height;
	gfloat							barPosition, barSize;
	
	/* Get allocation to paint background and scrollbar */
	clutter_actor_get_allocation_box(inActor, &allocation);
	clutter_actor_box_get_size(&allocation, &width, &height);

	/* Draw background */
	if(G_LIKELY(priv->backgroundColor))
	{
		cogl_path_new();
		cogl_set_source_color4ub(priv->backgroundColor->red,
									priv->backgroundColor->green,
									priv->backgroundColor->blue,
									priv->backgroundColor->alpha);
		cogl_path_rectangle(0, 0, width, height);
		cogl_path_fill();
	}

	/* Draw scrollbar */
	if(G_LIKELY(priv->scrollbarColor))
	{
		width=MAX(width-(2*priv->spacing), 0);
		height=MAX(height-(2*priv->spacing), 0);

		cogl_path_new();
		cogl_set_source_color4ub(priv->scrollbarColor->red,
									priv->scrollbarColor->green,
									priv->scrollbarColor->blue,
									priv->scrollbarColor->alpha);
		if(priv->isVertical)
		{
			if(priv->range>height) barSize=(height/priv->range)*height;
				else barSize=height;

			barPosition=MIN(MAX(0, (priv->value/priv->range)*height), height);
			if(barPosition+barSize>height) barPosition=height-barSize;

			cogl_path_round_rectangle(priv->spacing,
										priv->spacing+barPosition,
										priv->spacing+priv->thickness,
										priv->spacing+barPosition+barSize,
										priv->thickness/2.0f, 0.1f);
		}
			else
			{
				if(priv->range>width) barSize=(width/priv->range)*width;
					else barSize=width;

				barPosition=MIN(MAX(0, (priv->value/priv->range)*width), width);
				if(barPosition+barSize>width) barPosition=width-barSize;

				cogl_path_round_rectangle(priv->spacing+barPosition,
											priv->spacing,
											priv->spacing+barPosition+barSize,
											priv->spacing+priv->thickness,
											priv->thickness/2.0f, 0.1f);
			}
		cogl_path_fill();
	}
}

/* Get preferred width/height */
static void xfdashboard_scrollbar_get_preferred_width(ClutterActor *inActor,
														gfloat inForHeight,
														gfloat *outMinWidth,
														gfloat *outNaturalWidth)
{
	XfdashboardScrollbarPrivate	*priv=XFDASHBOARD_SCROLLBAR(inActor)->priv;
	gfloat						minWidth, naturalWidth;

	/* Determine sizes */
	if(priv->isVertical)
	{
		minWidth=naturalWidth=(2*priv->spacing)+priv->thickness;
	}
		else
		{
			/* Call parent's class method to determine sizes */
			if(CLUTTER_ACTOR_CLASS(xfdashboard_scrollbar_parent_class)->get_preferred_width)
			{
				CLUTTER_ACTOR_CLASS(xfdashboard_scrollbar_parent_class)->
					get_preferred_width(inActor,
										inForHeight,
										&minWidth,
										&naturalWidth);
			}
		}

	/* Store sizes calculated */
	if(outMinWidth) *outMinWidth=minWidth;
	if(outNaturalWidth) *outNaturalWidth=naturalWidth;
}

static void xfdashboard_scrollbar_get_preferred_height(ClutterActor *inActor,
														gfloat inForWidth,
														gfloat *outMinHeight,
														gfloat *outNaturalWidth)
{
	XfdashboardScrollbarPrivate	*priv=XFDASHBOARD_SCROLLBAR(inActor)->priv;
	gfloat						minHeight, naturalHeight;

	/* Determine sizes */
	if(priv->isVertical)
	{
		/* Call parent's class method to determine sizes */
		if(CLUTTER_ACTOR_CLASS(xfdashboard_scrollbar_parent_class)->get_preferred_height)
		{
			CLUTTER_ACTOR_CLASS(xfdashboard_scrollbar_parent_class)->
				get_preferred_height(inActor,
										inForWidth,
										&minHeight,
										&naturalHeight);
		}
	}
		else
		{
			minHeight=naturalHeight=(2*priv->spacing)+priv->thickness;
		}

	/* Store sizes calculated */
	if(outMinHeight) *outMinHeight=minHeight;
	if(outNaturalWidth) *outNaturalWidth=naturalHeight;
}

/* IMPLEMENTATION: GObject */

/* Dispose this object */
static void xfdashboard_scrollbar_dispose(GObject *inObject)
{
	XfdashboardScrollbar			*self=XFDASHBOARD_SCROLLBAR(inObject);
	XfdashboardScrollbarPrivate		*priv=self->priv;

	/* Release allocated resources */
	if(priv->scrollbarColor) clutter_color_free(priv->scrollbarColor);
	priv->scrollbarColor=NULL;

	if(priv->backgroundColor) clutter_color_free(priv->backgroundColor);
	priv->backgroundColor=NULL;

	/* Call parent's class dispose method */
	G_OBJECT_CLASS(xfdashboard_scrollbar_parent_class)->dispose(inObject);
}

/* Set/get properties */
static void xfdashboard_scrollbar_set_property(GObject *inObject,
												guint inPropID,
												const GValue *inValue,
												GParamSpec *inSpec)
{
	XfdashboardScrollbar			*self=XFDASHBOARD_SCROLLBAR(inObject);
	
	switch(inPropID)
	{
		case PROP_VALUE:
			xfdashboard_scrollbar_set_value(self, g_value_get_float(inValue));
			break;

		case PROP_RANGE:
			xfdashboard_scrollbar_set_range(self, g_value_get_float(inValue));
			break;

		case PROP_VERTICAL:
			xfdashboard_scrollbar_set_vertical(self, g_value_get_boolean(inValue));
			break;

		case PROP_THICKNESS:
			xfdashboard_scrollbar_set_thickness(self, g_value_get_float(inValue));
			break;

		case PROP_SPACING:
			xfdashboard_scrollbar_set_spacing(self, g_value_get_float(inValue));
			break;

		case PROP_COLOR:
			xfdashboard_scrollbar_set_color(self, clutter_value_get_color(inValue));
			break;

		case PROP_BACKGROUND_COLOR:
			xfdashboard_scrollbar_set_background_color(self, clutter_value_get_color(inValue));
			break;

		default:
			G_OBJECT_WARN_INVALID_PROPERTY_ID(inObject, inPropID, inSpec);
			break;
	}
}

static void xfdashboard_scrollbar_get_property(GObject *inObject,
												guint inPropID,
												GValue *outValue,
												GParamSpec *inSpec)
{
	XfdashboardScrollbarPrivate		*priv=XFDASHBOARD_SCROLLBAR(inObject)->priv;

	switch(inPropID)
	{
		case PROP_VALUE:
			g_value_set_float(outValue, priv->value);
			break;

		case PROP_RANGE:
			g_value_set_float(outValue, priv->range);
			break;

		case PROP_VERTICAL:
			g_value_set_boolean(outValue, priv->isVertical);
			break;

		case PROP_THICKNESS:
			g_value_set_float(outValue, priv->thickness);
			break;

		case PROP_SPACING:
			g_value_set_float(outValue, priv->spacing);
			break;

		case PROP_COLOR:
			clutter_value_set_color(outValue, priv->scrollbarColor);
			break;

		case PROP_BACKGROUND_COLOR:
			clutter_value_set_color(outValue, priv->backgroundColor);
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
static void xfdashboard_scrollbar_class_init(XfdashboardScrollbarClass *klass)
{
	GObjectClass			*gobjectClass=G_OBJECT_CLASS(klass);
	ClutterActorClass		*actorClass=CLUTTER_ACTOR_CLASS(klass);

	/* Override functions */
	actorClass->paint=xfdashboard_scrollbar_paint;
	actorClass->get_preferred_width=xfdashboard_scrollbar_get_preferred_width;
	actorClass->get_preferred_height=xfdashboard_scrollbar_get_preferred_height;

	gobjectClass->set_property=xfdashboard_scrollbar_set_property;
	gobjectClass->get_property=xfdashboard_scrollbar_get_property;
	gobjectClass->dispose=xfdashboard_scrollbar_dispose;

	/* Set up private structure */
	g_type_class_add_private(klass, sizeof(XfdashboardScrollbarPrivate));

	/* Define properties */
	XfdashboardScrollbarProperties[PROP_VALUE]=
		g_param_spec_float("value",
							"Value",
							"Current value of scroll bar start position within range",
							0.0f, G_MAXFLOAT,
							0.0f,
							G_PARAM_READWRITE);

	XfdashboardScrollbarProperties[PROP_RANGE]=
		g_param_spec_float("range",
							"Range",
							"Range to scroll within",
							1.0f, G_MAXFLOAT,
							1.0f,
							G_PARAM_READWRITE);

	XfdashboardScrollbarProperties[PROP_VERTICAL]=
		g_param_spec_boolean("vertical",
								"Vertical",
								"Whether the scroll bar should be vertical, rather than horizontal",
								FALSE,
								G_PARAM_READWRITE | G_PARAM_CONSTRUCT);

	XfdashboardScrollbarProperties[PROP_THICKNESS]=
		g_param_spec_float("thickness",
							"Thickness",
							"Thickness of scroll bar",
							0.0f, G_MAXFLOAT,
							DEFAULT_THICKNESS,
							G_PARAM_READWRITE);

	XfdashboardScrollbarProperties[PROP_SPACING]=
		g_param_spec_float("spacing",
							"Spacing",
							"Spacing between background and scroll bar",
							0.0f, G_MAXFLOAT,
							DEFAULT_SPACING,
							G_PARAM_READWRITE);

	XfdashboardScrollbarProperties[PROP_COLOR]=
		clutter_param_spec_color("color",
									"Scroll bar color",
									"Color of scroll bar",
									&_xfdashboard_scrollbar_default_scrollbar_color,
									G_PARAM_READWRITE | G_PARAM_CONSTRUCT);

	XfdashboardScrollbarProperties[PROP_BACKGROUND_COLOR]=
		clutter_param_spec_color("background-color",
									"Background color",
									"Background color of scroll bar",
									&_xfdashboard_scrollbar_default_background_color,
									G_PARAM_READWRITE | G_PARAM_CONSTRUCT);

	g_object_class_install_properties(gobjectClass, PROP_LAST, XfdashboardScrollbarProperties);

	/* Define signals */
	XfdashboardScrollbarSignals[VALUE_CHANGED]=
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

	XfdashboardScrollbarSignals[RANGE_CHANGED]=
		g_signal_new("range-changed",
						G_TYPE_FROM_CLASS(klass),
						G_SIGNAL_RUN_LAST,
						G_STRUCT_OFFSET(XfdashboardScrollbarClass, range_changed),
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
static void xfdashboard_scrollbar_init(XfdashboardScrollbar *self)
{
	XfdashboardScrollbarPrivate	*priv;

	priv=self->priv=XFDASHBOARD_SCROLLBAR_GET_PRIVATE(self);

	/* Set up default values */
	priv->value=0.0f;
	priv->range=1.0f;
	priv->isVertical=FALSE;
	priv->thickness=DEFAULT_THICKNESS;
	priv->spacing=DEFAULT_SPACING;
	priv->scrollbarColor=NULL;
	priv->backgroundColor=NULL;

	/* Connect signals */
	clutter_actor_set_reactive(CLUTTER_ACTOR(self), TRUE);
	g_signal_connect(self, "button-press-event", G_CALLBACK(_xfdashboard_scrollbar_button_pressed), NULL);
	g_signal_connect(self, "scroll-event", G_CALLBACK(_xfdashboard_scrollbar_scroll_event), NULL);
}

/* Implementation: Public API */

/* Create new actor */
ClutterActor* xfdashboard_scrollbar_new(void)
{
	return(g_object_new(XFDASHBOARD_TYPE_SCROLLBAR, NULL));
}

ClutterActor* xfdashboard_scrollbar_new_with_thickness(gfloat inThickness)
{
	return(g_object_new(XFDASHBOARD_TYPE_SCROLLBAR,
						"thickness", inThickness,
						NULL));
}

/* Get/set value represented by position of scroll bar */
gfloat xfdashboard_scrollbar_get_value(XfdashboardScrollbar *self)
{
	g_return_val_if_fail(XFDASHBOARD_IS_SCROLLBAR(self), 0.0f);

	return(self->priv->value);
}

void xfdashboard_scrollbar_set_value(XfdashboardScrollbar *self, gfloat inValue)
{
	g_return_if_fail(XFDASHBOARD_IS_SCROLLBAR(self));
	g_return_if_fail(inValue>=0.0f);

	/* Only set new value if it differs from current one */
	XfdashboardScrollbarPrivate		*priv=XFDASHBOARD_SCROLLBAR_GET_PRIVATE(self);

	if(inValue>priv->range)
	{
		g_warning("Value %.2f is not within range [0-%.2f]", inValue, priv->range);
		inValue=priv->range;
	}

	if(inValue!=priv->value)
	{
		/* Set new value, emit signal and redraw scroll bar */
		priv->value=inValue;
		g_signal_emit_by_name(self, "value-changed", priv->value);
		clutter_actor_queue_redraw(CLUTTER_ACTOR(self));
	}
}

/* Get/set range to scroll within */
gfloat xfdashboard_scrollbar_get_range(XfdashboardScrollbar *self)
{
	g_return_val_if_fail(XFDASHBOARD_IS_SCROLLBAR(self), 1.0f);

	return(self->priv->range);
}

void xfdashboard_scrollbar_set_range(XfdashboardScrollbar *self, gfloat inRange)
{
	g_return_if_fail(XFDASHBOARD_IS_SCROLLBAR(self));
	g_return_if_fail(inRange>=1.0f);

	/* Only set new value if it differs from current value */
	XfdashboardScrollbarPrivate	*priv=XFDASHBOARD_SCROLLBAR_GET_PRIVATE(self);

	if(inRange!=priv->range)
	{
		priv->range=inRange;

		g_signal_emit_by_name(self, "range-changed", priv->value);
		clutter_actor_queue_redraw(CLUTTER_ACTOR(self));
	}
}

/* Get/set orientation */
gboolean xfdashboard_scrollbar_get_vertical(XfdashboardScrollbar *self)
{
	g_return_val_if_fail(XFDASHBOARD_IS_SCROLLBAR(self), FALSE);

	return(self->priv->isVertical);
}

void xfdashboard_scrollbar_set_vertical(XfdashboardScrollbar *self, gboolean inIsVertical)
{
	g_return_if_fail(XFDASHBOARD_IS_SCROLLBAR(self));

	/* Set orientation if it differs from current value */
	if(inIsVertical!=self->priv->isVertical)
	{
		self->priv->isVertical=inIsVertical;
	
		clutter_actor_queue_relayout(CLUTTER_ACTOR(self));
	}
}

/* Get/set thickness of scroll bar */
gfloat xfdashboard_scrollbar_get_thickness(XfdashboardScrollbar *self)
{
	g_return_val_if_fail(XFDASHBOARD_IS_SCROLLBAR(self), 0.0f);

	return(self->priv->thickness);
}

void xfdashboard_scrollbar_set_thickness(XfdashboardScrollbar *self, gfloat inThickness)
{
	g_return_if_fail(XFDASHBOARD_IS_SCROLLBAR(self));
	g_return_if_fail(inThickness>=1.0f);
	
	/* Set spacing if it differs from current value */
	if(inThickness!=self->priv->thickness)
	{
		self->priv->thickness=inThickness;

		clutter_actor_queue_relayout(CLUTTER_ACTOR(self));
	}
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
	
	/* Set spacing if it differs from current value */
	if(inSpacing!=self->priv->spacing)
	{
		self->priv->spacing=inSpacing;

		clutter_actor_queue_relayout(CLUTTER_ACTOR(self));
	}
}

/* Get/set color of scroll bar */
const ClutterColor* xfdashboard_scrollbar_get_color(XfdashboardScrollbar *self)
{
	g_return_val_if_fail(XFDASHBOARD_IS_SCROLLBAR(self), NULL);

	return(self->priv->scrollbarColor);
}

void xfdashboard_scrollbar_set_color(XfdashboardScrollbar *self, const ClutterColor *inColor)
{
	g_return_if_fail(XFDASHBOARD_IS_SCROLLBAR(self));
	g_return_if_fail(inColor);

	/* Set scrollbar color if it differs from current value */
	XfdashboardScrollbarPrivate		*priv=XFDASHBOARD_SCROLLBAR(self)->priv;

	if(!priv->scrollbarColor || !clutter_color_equal(inColor, priv->scrollbarColor))
	{
		if(priv->scrollbarColor) clutter_color_free(priv->scrollbarColor);
		priv->scrollbarColor=NULL;
		priv->scrollbarColor=clutter_color_copy(inColor);

		clutter_actor_queue_redraw(CLUTTER_ACTOR(self));
	}
}

/* Get/set color of background */
const ClutterColor* xfdashboard_scrollbar_get_background_color(XfdashboardScrollbar *self)
{
	g_return_val_if_fail(XFDASHBOARD_IS_SCROLLBAR(self), NULL);

	return(self->priv->backgroundColor);
}

void xfdashboard_scrollbar_set_background_color(XfdashboardScrollbar *self, const ClutterColor *inColor)
{
	g_return_if_fail(XFDASHBOARD_IS_SCROLLBAR(self));
	g_return_if_fail(inColor);

	/* Set background color if it differs from current value */
	XfdashboardScrollbarPrivate		*priv=XFDASHBOARD_SCROLLBAR(self)->priv;

	if(!priv->backgroundColor || !clutter_color_equal(inColor, priv->backgroundColor))
	{
		if(priv->backgroundColor) clutter_color_free(priv->backgroundColor);
		priv->backgroundColor=NULL;
		priv->backgroundColor=clutter_color_copy(inColor);

		clutter_actor_queue_redraw(CLUTTER_ACTOR(self));
	}
}
