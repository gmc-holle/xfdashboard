/*
 * notification: A notification actor
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

#include "notification.h"

#include <glib/gi18n-lib.h>
#include <math.h>

#include "enums.h"

/* Define this class in GObject system */
G_DEFINE_TYPE(XfdashboardNotification,
				xfdashboard_notification,
				XFDASHBOARD_TYPE_TEXT_BOX)
                                                
/* Private structure - access only by public API if needed */
#define XFDASHBOARD_NOTIFICATION_GET_PRIVATE(obj) \
	(G_TYPE_INSTANCE_GET_PRIVATE((obj), XFDASHBOARD_TYPE_NOTIFICATION, XfdashboardNotificationPrivate))

struct _XfdashboardNotificationPrivate
{
	/* Properties related */
	XfdashboardNotificationPlacement	placement;
	gfloat								margin;
};

/* Properties */
enum
{
	PROP_0,

	PROP_PLACEMENT,
	PROP_MARGIN,

	PROP_LAST
};

static GParamSpec* XfdashboardNotificationProperties[PROP_LAST]={ 0, };

/* IMPLEMENTATION: Private variables and methods */
#define DEFAULT_PLACEMENT		XFDASHBOARD_NOTIFICATION_PLACEMENT_BOTTOM			// TODO: Replace by settings/theming object
#define DEFAULT_MARGIN			16.0f												// TODO: Replace by settings/theming object
static ClutterColor				defaultFillColor={ 0x13, 0x50, 0xff, 0xff };		// TODO: Replace by settings/theming object
static ClutterColor				defaultOutlineColor={ 0x63, 0xb0, 0xff, 0xff };		// TODO: Replace by settings/theming object
#define DEFAULT_OUTLINE_WIDTH	1.0f												// TODO: Replace by settings/theming object

/* IMPLEMENTATION: ClutterActor */

/* Get preferred width/height */
static void _xfdashboard_notification_get_preferred_height(ClutterActor *inActor,
															gfloat inForWidth,
															gfloat *outMinHeight,
															gfloat *outNaturalHeight)
{
	XfdashboardNotification				*self=XFDASHBOARD_NOTIFICATION(inActor);
	XfdashboardNotificationPrivate		*priv=self->priv;
	gfloat								minHeight, naturalHeight;
	ClutterActorClass					*actorClass=CLUTTER_ACTOR_CLASS(xfdashboard_notification_parent_class);
	ClutterActor						*stage;
	gfloat								maximumHeight;

	/* Set up default values */
	minHeight=naturalHeight=0.0f;

	/* Determine preferred height of derived object */
	if(actorClass->get_preferred_height)
	{
		actorClass->get_preferred_height(CLUTTER_ACTOR(self),
											inForWidth,
											&minHeight,
											&naturalHeight);
	}

	/* Get maximum available height which is stage height reduced by margin */
	maximumHeight=0.0f;
	stage=clutter_actor_get_stage(CLUTTER_ACTOR(self));
	if(stage)
	{
		maximumHeight=clutter_actor_get_height(stage);
		maximumHeight-=(2*priv->margin);
		maximumHeight=MAX(0.0f, maximumHeight);
	}

	/* Check if actor's preferred height fits into maximum height available */
	minHeight=MIN(minHeight, maximumHeight);
	naturalHeight=MIN(naturalHeight, maximumHeight);

	/* Store sizes computed */
	if(outMinHeight) *outMinHeight=minHeight;
	if(outNaturalHeight) *outNaturalHeight=naturalHeight;
}

static void _xfdashboard_notification_get_preferred_width(ClutterActor *inActor,
																gfloat inForHeight,
																gfloat *outMinWidth,
																gfloat *outNaturalWidth)
{
	XfdashboardNotification				*self=XFDASHBOARD_NOTIFICATION(inActor);
	XfdashboardNotificationPrivate		*priv=self->priv;
	gfloat								minWidth, naturalWidth;
	ClutterActorClass					*actorClass=CLUTTER_ACTOR_CLASS(xfdashboard_notification_parent_class);
	ClutterActor						*stage;
	gfloat								maximumWidth;

	/* Set up default values */
	minWidth=naturalWidth=0.0f;

	/* Determine preferred width of derived object */
	if(actorClass->get_preferred_width)
	{
		actorClass->get_preferred_width(CLUTTER_ACTOR(self),
										inForHeight,
										&minWidth,
										&naturalWidth);
	}

	/* Get maximum available width which is stage width reduced by margin */
	maximumWidth=0.0f;
	stage=clutter_actor_get_stage(CLUTTER_ACTOR(self));
	if(stage)
	{
		maximumWidth=clutter_actor_get_width(stage);
		maximumWidth-=(2*priv->margin);
		maximumWidth=MAX(0.0f, maximumWidth);
	}

	/* Check if actor's preferred width fits into maximum width available */
	minWidth=MIN(minWidth, maximumWidth);
	naturalWidth=MIN(naturalWidth, maximumWidth);

	/* Store sizes computed */
	if(outMinWidth) *outMinWidth=minWidth;
	if(outNaturalWidth) *outNaturalWidth=naturalWidth;
}

/* Allocate position and size of actor and its children */
static void _xfdashboard_notification_allocate(ClutterActor *inActor,
												const ClutterActorBox *inBox,
												ClutterAllocationFlags inFlags)
{
	XfdashboardNotification				*self=XFDASHBOARD_NOTIFICATION(inActor);
	XfdashboardNotificationPrivate		*priv=self->priv;
	gfloat								left, right, top, bottom;
	ClutterActor						*stage;
	gfloat								stageWidth, stageHeight;
	ClutterActorBox						*boxActor=NULL;

	/* Determine only position of this actor at stage and use size as-is */
	stage=clutter_actor_get_stage(inActor);
	stageWidth=clutter_actor_get_width(stage);
	stageHeight=clutter_actor_get_height(stage);

	left=((stageWidth/2.0f)-(clutter_actor_box_get_width(inBox)/2.0f));
	right=((stageWidth/2.0f)+(clutter_actor_box_get_width(inBox)/2.0f));

	top=bottom=0.0f;
	switch(priv->placement)
	{
		case XFDASHBOARD_NOTIFICATION_PLACEMENT_TOP:
			top=priv->margin;
			bottom=top+clutter_actor_box_get_height(inBox);
			break;

		case XFDASHBOARD_NOTIFICATION_PLACEMENT_BOTTOM:
			bottom=stageHeight-priv->margin;
			top=bottom-clutter_actor_box_get_height(inBox);
			break;

		case XFDASHBOARD_NOTIFICATION_PLACEMENT_CENTER:
			top=((stageHeight/2.0f)-(clutter_actor_box_get_height(inBox)/2.0f));
			bottom=top+clutter_actor_box_get_height(inBox);
			break;
	}

	boxActor=clutter_actor_box_new(floor(left), floor(top), floor(right), floor(bottom));

	/* Chain up to store the allocation of the actor */
	CLUTTER_ACTOR_CLASS(xfdashboard_notification_parent_class)->allocate(inActor, boxActor, inFlags);

	/* Release allocated memory */
	if(boxActor) clutter_actor_box_free(boxActor);
}

/* IMPLEMENTATION: GObject */

/* Set/get properties */
static void _xfdashboard_notification_set_property(GObject *inObject,
													guint inPropID,
													const GValue *inValue,
													GParamSpec *inSpec)
{
	XfdashboardNotification			*self=XFDASHBOARD_NOTIFICATION(inObject);

	switch(inPropID)
	{
		case PROP_PLACEMENT:
			xfdashboard_notification_set_placement(self, g_value_get_enum(inValue));
			break;

		case PROP_MARGIN:
			xfdashboard_notification_set_margin(self, g_value_get_float(inValue));
			break;

		default:
			G_OBJECT_WARN_INVALID_PROPERTY_ID(inObject, inPropID, inSpec);
			break;
	}
}

static void _xfdashboard_notification_get_property(GObject *inObject,
													guint inPropID,
													GValue *outValue,
													GParamSpec *inSpec)
{
	XfdashboardNotification			*self=XFDASHBOARD_NOTIFICATION(inObject);
	XfdashboardNotificationPrivate	*priv=self->priv;

	switch(inPropID)
	{
		case PROP_PLACEMENT:
			g_value_set_enum(outValue, priv->placement);
			break;

		case PROP_MARGIN:
			g_value_set_float(outValue, priv->margin);
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
static void xfdashboard_notification_class_init(XfdashboardNotificationClass *klass)
{
	ClutterActorClass	*actorClass=CLUTTER_ACTOR_CLASS(klass);
	GObjectClass		*gobjectClass=G_OBJECT_CLASS(klass);

	/* Override functions */
	gobjectClass->set_property=_xfdashboard_notification_set_property;
	gobjectClass->get_property=_xfdashboard_notification_get_property;

	actorClass->get_preferred_width=_xfdashboard_notification_get_preferred_width;
	actorClass->get_preferred_height=_xfdashboard_notification_get_preferred_height;
	actorClass->allocate=_xfdashboard_notification_allocate;

	/* Set up private structure */
	g_type_class_add_private(klass, sizeof(XfdashboardNotificationPrivate));

	/* Define properties */
	XfdashboardNotificationProperties[PROP_PLACEMENT]=
		g_param_spec_enum("placement",
							_("Placement"),
							_("Where to place notification at"),
							XFDASHBOARD_TYPE_NOTIFICATION_PLACEMENT,
							DEFAULT_PLACEMENT,
							G_PARAM_READWRITE);

	XfdashboardNotificationProperties[PROP_MARGIN]=
		g_param_spec_float("margin",
							_("Margin"),
							_("Distance to all sides"),
							0.0f, G_MAXFLOAT,
							DEFAULT_MARGIN,
							G_PARAM_READWRITE);
}

/* Object initialization
 * Create private structure and set up default values
 */
static void xfdashboard_notification_init(XfdashboardNotification *self)
{
	XfdashboardNotificationPrivate	*priv;

	priv=self->priv=XFDASHBOARD_NOTIFICATION_GET_PRIVATE(self);

	/* Set up default values */
	priv->placement=DEFAULT_PLACEMENT;
	priv->margin=DEFAULT_MARGIN;

	/* Set up this actor */
	clutter_actor_set_reactive(CLUTTER_ACTOR(self), FALSE);
	clutter_actor_set_fixed_position_set(CLUTTER_ACTOR(self), TRUE);
	clutter_actor_set_request_mode(CLUTTER_ACTOR(self), CLUTTER_REQUEST_HEIGHT_FOR_WIDTH);
	xfdashboard_background_set_background_type(XFDASHBOARD_BACKGROUND(self),
												XFDASHBOARD_BACKGROUND_TYPE_FILL | XFDASHBOARD_BACKGROUND_TYPE_OUTLINE);
	xfdashboard_background_set_fill_color(XFDASHBOARD_BACKGROUND(self), &defaultFillColor);
	xfdashboard_background_set_outline_color(XFDASHBOARD_BACKGROUND(self), &defaultOutlineColor);
	xfdashboard_background_set_outline_width(XFDASHBOARD_BACKGROUND(self), DEFAULT_OUTLINE_WIDTH);
}

/* IMPLEMENTATION: Public API */

/* Create new actor */
ClutterActor* xfdashboard_notification_new(void)
{
	return(g_object_new(XFDASHBOARD_TYPE_NOTIFICATION, NULL));
}

ClutterActor* xfdashboard_notification_new_with_text(const gchar *inText)
{
	return(g_object_new(XFDASHBOARD_TYPE_NOTIFICATION,
							"text", inText,
							NULL));
}

ClutterActor* xfdashboard_notification_new_with_icon(const gchar *inIconName)
{
	return(g_object_new(XFDASHBOARD_TYPE_NOTIFICATION,
							"primary-icon-name", inIconName,
							NULL));
}

ClutterActor* xfdashboard_notification_new_full(const gchar *inText, const gchar *inIconName)
{
	return(g_object_new(XFDASHBOARD_TYPE_NOTIFICATION,
							"text", inText,
							"primary-icon-name", inIconName,
							NULL));
}

/* Get/set placement of notification */
XfdashboardNotificationPlacement xfdashboard_notification_get_placement(XfdashboardNotification *self)
{
	g_return_val_if_fail(XFDASHBOARD_IS_NOTIFICATION(self), XFDASHBOARD_NOTIFICATION_PLACEMENT_BOTTOM);

	return(self->priv->placement);
}

void xfdashboard_notification_set_placement(XfdashboardNotification *self, XfdashboardNotificationPlacement inPlacement)
{
	XfdashboardNotificationPrivate	*priv;

	g_return_if_fail(XFDASHBOARD_IS_NOTIFICATION(self));

	priv=self->priv;

	/* Set value if changed */
	if(priv->placement!=inPlacement)
	{
		/* Set value */
		priv->placement=inPlacement;
		clutter_actor_queue_relayout(CLUTTER_ACTOR(self));

		/* Notify about property change */
		g_object_notify_by_pspec(G_OBJECT(self), XfdashboardNotificationProperties[PROP_PLACEMENT]);
	}
}

/* Get/set margin of notification (distance to all sides) */
gfloat xfdashboard_notification_get_margin(XfdashboardNotification *self)
{
	g_return_val_if_fail(XFDASHBOARD_IS_NOTIFICATION(self), 0.0f);

	return(self->priv->margin);
}

void xfdashboard_notification_set_margin(XfdashboardNotification *self, gfloat inMargin)
{
	XfdashboardNotificationPrivate	*priv;

	g_return_if_fail(XFDASHBOARD_IS_NOTIFICATION(self));

	priv=self->priv;

	/* Set value if changed */
	if(priv->margin!=inMargin)
	{
		/* Set value */
		priv->margin=inMargin;
		clutter_actor_queue_relayout(CLUTTER_ACTOR(self));

		/* Notify about property change */
		g_object_notify_by_pspec(G_OBJECT(self), XfdashboardNotificationProperties[PROP_MARGIN]);
	}
}
