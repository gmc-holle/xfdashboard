/*
 * textbox: An actor representing an editable or read-only text-box
 *          with optional icons 
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

#include <libxfdashboard/text-box.h>

#include <glib/gi18n-lib.h>
#include <math.h>

#include <libxfdashboard/enums.h>
#include <libxfdashboard/button.h>
#include <libxfdashboard/stylable.h>
#include <libxfdashboard/focusable.h>
#include <libxfdashboard/focus-manager.h>
#include <libxfdashboard/compat.h>


/* Define this class in GObject system */
static void _xfdashboard_text_box_focusable_iface_init(XfdashboardFocusableInterface *iface);

struct _XfdashboardTextBoxPrivate
{
	/* Properties related */
	gfloat					padding;
	gfloat					spacing;

	gboolean				isEditable;

	gchar					*primaryIconName;
	gchar					*secondaryIconName;

	gchar					*textFont;
	ClutterColor			*textColor;
	ClutterColor			*selectionTextColor;
	ClutterColor			*selectionBackgroundColor;

	gboolean				hintTextSet;
	gchar					*hintTextFont;
	ClutterColor			*hintTextColor;

	/* Instance related */
	ClutterActor			*actorTextBox;
	ClutterActor			*actorHintLabel;

	gboolean				showPrimaryIcon;
	ClutterActor			*actorPrimaryIcon;

	gboolean				showSecondaryIcon;
	ClutterActor			*actorSecondaryIcon;

	gboolean				selectionColorSet;
};

G_DEFINE_TYPE_WITH_CODE(XfdashboardTextBox,
						xfdashboard_text_box,
						XFDASHBOARD_TYPE_BACKGROUND,
						G_ADD_PRIVATE(XfdashboardTextBox)
						G_IMPLEMENT_INTERFACE(XFDASHBOARD_TYPE_FOCUSABLE, _xfdashboard_text_box_focusable_iface_init))

/* Properties */
enum
{
	PROP_0,

	PROP_PADDING,
	PROP_SPACING,

	PROP_EDITABLE,

	PROP_PRIMARY_ICON_NAME,
	PROP_SECONDARY_ICON_NAME,

	PROP_TEXT,
	PROP_TEXT_FONT,
	PROP_TEXT_COLOR,
	PROP_SELECTION_TEXT_COLOR,
	PROP_SELECTION_BACKGROUND_COLOR,

	PROP_HINT_TEXT,
	PROP_HINT_TEXT_FONT,
	PROP_HINT_TEXT_COLOR,
	PROP_HINT_TEXT_SET,

	PROP_LAST
};

static GParamSpec* XfdashboardTextBoxProperties[PROP_LAST]={ 0, };

/* Signals */
enum
{
	SIGNAL_TEXT_CHANGED,

	SIGNAL_PRIMARY_ICON_CLICKED,
	SIGNAL_SECONDARY_ICON_CLICKED,

	SIGNAL_LAST
};

static guint XfdashboardTextBoxSignals[SIGNAL_LAST]={ 0, };

/* IMPLEMENTATION: Private variables and methods */

/* Text of editable text box has changed */
static void _xfdashboard_text_box_on_text_changed(XfdashboardTextBox *self, gpointer inUserData)
{
	XfdashboardTextBoxPrivate	*priv;
	ClutterText					*actorText;

	g_return_if_fail(XFDASHBOARD_IS_TEXT_BOX(self));
	g_return_if_fail(CLUTTER_IS_TEXT(inUserData));

	priv=self->priv;
	actorText=CLUTTER_TEXT(inUserData);

	/* Show hint label depending if text box is empty or not */
	if(xfdashboard_text_box_is_empty(self) && priv->isEditable)
	{
		clutter_actor_show(priv->actorHintLabel);
	}
		else
		{
			clutter_actor_hide(priv->actorHintLabel);
		}

	/* Emit signal for text changed */
	g_signal_emit(self, XfdashboardTextBoxSignals[SIGNAL_TEXT_CHANGED], 0, clutter_text_get_text(actorText));
}

/* Primary icon was clicked */
static void _xfdashboard_text_box_on_primary_icon_clicked(XfdashboardTextBox *self,
															gpointer inUserData)
{
	g_return_if_fail(XFDASHBOARD_IS_TEXT_BOX(self));

	/* Emit signal for clicking primary icon */
	g_signal_emit(self, XfdashboardTextBoxSignals[SIGNAL_PRIMARY_ICON_CLICKED], 0);
}

/* Secondary icon was clicked */
static void _xfdashboard_text_box_on_secondary_icon_clicked(XfdashboardTextBox *self,
															gpointer inUserData)
{
	g_return_if_fail(XFDASHBOARD_IS_TEXT_BOX(self));

	/* Emit signal for clicking secondary icon */
	g_signal_emit(self, XfdashboardTextBoxSignals[SIGNAL_SECONDARY_ICON_CLICKED], 0);
}

/* A key was pressed */
static gboolean _xfdashboard_text_box_on_key_press_event(ClutterActor *inActor,
															ClutterEvent *inEvent,
															gpointer inUserData)
{
	XfdashboardTextBox			*self;
	XfdashboardTextBoxPrivate	*priv;
	gboolean					result;

	g_return_val_if_fail(XFDASHBOARD_IS_TEXT_BOX(inActor), CLUTTER_EVENT_PROPAGATE);

	self=XFDASHBOARD_TEXT_BOX(inActor);
	priv=self->priv;
	result=CLUTTER_EVENT_PROPAGATE;

	/* Push event to real text box if available */
	if(priv->actorTextBox)
	{
		result=clutter_actor_event(priv->actorTextBox, inEvent, FALSE);
	}

	/* Return event handling result */
	return(result);
}

/* A key was released */
static gboolean _xfdashboard_text_box_on_key_release_event(ClutterActor *inActor,
															ClutterEvent *inEvent,
															gpointer inUserData)
{
	XfdashboardTextBox			*self;
	XfdashboardTextBoxPrivate	*priv;
	gboolean					result;

	g_return_val_if_fail(XFDASHBOARD_IS_TEXT_BOX(inActor), CLUTTER_EVENT_PROPAGATE);

	self=XFDASHBOARD_TEXT_BOX(inActor);
	priv=self->priv;
	result=CLUTTER_EVENT_PROPAGATE;

	/* Push event to real text box if available */
	if(priv->actorTextBox)
	{
		result=clutter_actor_event(priv->actorTextBox, inEvent, FALSE);
	}

	/* Return event handling result */
	return(result);
}


/* IMPLEMENTATION: ClutterActor */

/* Actor got key focus */
static void _xfdashboard_text_box_key_focus_in(ClutterActor *inActor)
{
	XfdashboardTextBox			*self=XFDASHBOARD_TEXT_BOX(inActor);
	XfdashboardTextBoxPrivate	*priv=self->priv;
	ClutterStage				*stage;
	XfdashboardFocusManager		*focusManager;

	/* Push key focus forward to text box */
	stage=CLUTTER_STAGE(clutter_actor_get_stage(inActor));
	clutter_stage_set_key_focus(stage, priv->actorTextBox);

	/* Update focus in focus manager */
	focusManager=xfdashboard_focus_manager_get_default();
	xfdashboard_focus_manager_set_focus(focusManager, XFDASHBOARD_FOCUSABLE(self));
	g_object_unref(focusManager);
}

/* Show all children of this one */
static void _xfdashboard_text_box_show(ClutterActor *inActor)
{
	XfdashboardTextBox			*self=XFDASHBOARD_TEXT_BOX(inActor);
	XfdashboardTextBoxPrivate	*priv=self->priv;

	/* Show icons */
	if(priv->showPrimaryIcon!=FALSE)
	{
		clutter_actor_show(CLUTTER_ACTOR(priv->actorPrimaryIcon));
	}

	if(priv->showSecondaryIcon!=FALSE)
	{
		clutter_actor_show(CLUTTER_ACTOR(priv->actorSecondaryIcon));
	}

	/* Show hint label depending if text box is empty or not */
	if(xfdashboard_text_box_is_empty(self) && priv->isEditable)
	{
		clutter_actor_show(priv->actorHintLabel);
	}
		else
		{
			clutter_actor_hide(priv->actorHintLabel);
		}

	clutter_actor_show(CLUTTER_ACTOR(self));
}

/* Get preferred width/height */
static void _xfdashboard_text_box_get_preferred_height(ClutterActor *self,
														gfloat inForWidth,
														gfloat *outMinHeight,
														gfloat *outNaturalHeight)
{
	XfdashboardTextBoxPrivate	*priv=XFDASHBOARD_TEXT_BOX(self)->priv;
	gfloat						minHeight, naturalHeight;
	gfloat						childMinHeight, childNaturalHeight;
	
	minHeight=naturalHeight=0.0f;

	/* Regardless if text or hint label is visible or not. Both actors' height
	 * will be requested and the larger one used */
	clutter_actor_get_preferred_height(CLUTTER_ACTOR(priv->actorTextBox),
										inForWidth,
										&minHeight, &naturalHeight);

	clutter_actor_get_preferred_height(CLUTTER_ACTOR(priv->actorHintLabel),
										inForWidth,
										&childMinHeight, &childNaturalHeight);

	if(childMinHeight>minHeight) minHeight=childMinHeight;
	if(childNaturalHeight>naturalHeight) naturalHeight=childNaturalHeight;

	// Add padding
	minHeight+=2*priv->padding;
	naturalHeight+=2*priv->padding;

	/* Store sizes computed */
	if(outMinHeight) *outMinHeight=minHeight;
	if(outNaturalHeight) *outNaturalHeight=naturalHeight;
}

static void _xfdashboard_text_box_get_preferred_width(ClutterActor *self,
														gfloat inForHeight,
														gfloat *outMinWidth,
														gfloat *outNaturalWidth)
{
	XfdashboardTextBoxPrivate	*priv=XFDASHBOARD_TEXT_BOX(self)->priv;
	gfloat						minWidth, naturalWidth;
	gfloat						childMinWidth, childNaturalWidth;
	gint						numberChildren=0;

	minWidth=naturalWidth=0.0f;

	/* Determine size of primary icon if visible */
	if(clutter_actor_is_visible(priv->actorPrimaryIcon))
	{
		clutter_actor_get_preferred_width(CLUTTER_ACTOR(priv->actorPrimaryIcon),
											inForHeight,
											&childMinWidth, &childNaturalWidth);

		minWidth+=childMinWidth;
		naturalWidth+=childNaturalWidth;
		numberChildren++;
	}

	/* Determine size of editable text box if visible */
	if(clutter_actor_is_visible(priv->actorTextBox))
	{
		clutter_actor_get_preferred_width(CLUTTER_ACTOR(priv->actorTextBox),
											inForHeight,
											&childMinWidth, &childNaturalWidth);

		minWidth+=childMinWidth;
		naturalWidth+=childNaturalWidth;
		numberChildren++;
	}

	/* Determine size of hint label if visible */
	if(clutter_actor_is_visible(priv->actorHintLabel))
	{
		clutter_actor_get_preferred_width(CLUTTER_ACTOR(priv->actorHintLabel),
											inForHeight,
											&childMinWidth, &childNaturalWidth);

		minWidth+=childMinWidth;
		naturalWidth+=childNaturalWidth;
		numberChildren++;
	}

	/* Determine size of secondary icon if visible */
	if(clutter_actor_is_visible(priv->actorSecondaryIcon))
	{
		clutter_actor_get_preferred_width(CLUTTER_ACTOR(priv->actorSecondaryIcon),
											inForHeight,
											&childMinWidth, &childNaturalWidth);

		minWidth+=childMinWidth;
		naturalWidth+=childNaturalWidth;
	}

	/* Add spacing for each child except the last one */
	if(numberChildren>1)
	{
		numberChildren--;
		minWidth+=(numberChildren*priv->spacing);
		naturalWidth+=(numberChildren*priv->spacing);
	}

	// Add padding
	minWidth+=2*priv->padding;
	naturalWidth+=2*priv->padding;

	/* Store sizes computed */
	if(outMinWidth) *outMinWidth=minWidth;
	if(outNaturalWidth) *outNaturalWidth=naturalWidth;
}

/* Allocate position and size of actor and its children */
static void _xfdashboard_text_box_allocate(ClutterActor *self,
											const ClutterActorBox *inBox,
											ClutterAllocationFlags inFlags)
{
	XfdashboardTextBoxPrivate	*priv=XFDASHBOARD_TEXT_BOX(self)->priv;
	ClutterActorBox				*box=NULL;
	gfloat						left, right, top, bottom;
	gfloat						iconWidth, iconHeight;

	/* Chain up to store the allocation of the actor */
	CLUTTER_ACTOR_CLASS(xfdashboard_text_box_parent_class)->allocate(self, inBox, inFlags);

	/* Initialize bounding box of allocation used in actors */
	left=top=priv->padding;
	right=clutter_actor_box_get_width(inBox)-priv->padding;
	bottom=clutter_actor_box_get_height(inBox)-priv->padding;

	/* Set allocation of primary icon if visible */
	if(clutter_actor_is_visible(priv->actorPrimaryIcon))
	{
		gfloat					childRight;

		/* Get scale size of primary icon */
		iconWidth=iconHeight=0.0f;
		clutter_actor_get_size(priv->actorPrimaryIcon, &iconWidth, &iconHeight);
		if(iconHeight>0.0f) iconWidth=(bottom-top)*(iconWidth/iconHeight);

		/* Set allocation */
		childRight=left+iconWidth;

		box=clutter_actor_box_new(floor(left), floor(top), floor(childRight), floor(bottom));
		clutter_actor_allocate(CLUTTER_ACTOR(priv->actorPrimaryIcon), box, inFlags);
		clutter_actor_box_free(box);

		/* Adjust bounding box for next actor */
		left=childRight+priv->spacing;
	}

	/* Set allocation of secondary icon if visible */
	if(clutter_actor_is_visible(priv->actorSecondaryIcon))
	{
		gfloat					childLeft;

		/* Get scale size of secondary icon */
		iconWidth=0.0f;
		clutter_actor_get_size(priv->actorSecondaryIcon, &iconWidth, &iconHeight);
		if(iconHeight>0.0f) iconWidth=(bottom-top)*(iconWidth/iconHeight);

		/* Set allocation */
		childLeft=right-iconWidth;

		box=clutter_actor_box_new(floor(childLeft), floor(top), floor(right), floor(bottom));
		clutter_actor_allocate(CLUTTER_ACTOR(priv->actorSecondaryIcon), box, inFlags);
		clutter_actor_box_free(box);

		/* Adjust bounding box for next actor */
		right=childLeft-priv->spacing;
	}

	/* Set allocation of editable text box if visible */
	if(clutter_actor_is_visible(priv->actorTextBox))
	{
		gfloat					textHeight;

		/* Get height of text */
		clutter_actor_get_preferred_size(CLUTTER_ACTOR(priv->actorTextBox),
											NULL, NULL,
											NULL, &textHeight);

		/* Set allocation */
		box=clutter_actor_box_new(floor(left), floor(bottom-textHeight), floor(right), floor(bottom));
		clutter_actor_allocate(CLUTTER_ACTOR(priv->actorTextBox), box, inFlags);
		clutter_actor_box_free(box);
	}

	/* Set allocation of hint label if visible */
	if(clutter_actor_is_visible(priv->actorHintLabel))
	{
		gfloat					textHeight;

		/* Get height of label */
		clutter_actor_get_preferred_size(CLUTTER_ACTOR(priv->actorHintLabel),
											NULL, NULL,
											NULL, &textHeight);

		/* Set allocation */
		box=clutter_actor_box_new(floor(left), floor(bottom-textHeight), floor(right), floor(bottom));
		clutter_actor_allocate(CLUTTER_ACTOR(priv->actorHintLabel), box, inFlags);
		clutter_actor_box_free(box);
	}
}

/* Destroy this actor */
static void _xfdashboard_text_box_destroy(ClutterActor *self)
{
	/* Destroy each child actor when this actor is destroyed */
	XfdashboardTextBoxPrivate	*priv=XFDASHBOARD_TEXT_BOX(self)->priv;

	if(priv->actorTextBox)
	{
		clutter_actor_destroy(CLUTTER_ACTOR(priv->actorTextBox));
		priv->actorTextBox=NULL;
	}

	if(priv->actorHintLabel)
	{
		clutter_actor_destroy(CLUTTER_ACTOR(priv->actorHintLabel));
		priv->actorHintLabel=NULL;
	}

	if(priv->actorPrimaryIcon)
	{
		clutter_actor_destroy(CLUTTER_ACTOR(priv->actorPrimaryIcon));
		priv->actorPrimaryIcon=NULL;
	}

	if(priv->actorSecondaryIcon)
	{
		clutter_actor_destroy(CLUTTER_ACTOR(priv->actorSecondaryIcon));
		priv->actorSecondaryIcon=NULL;
	}

	/* Call parent's class destroy method */
	CLUTTER_ACTOR_CLASS(xfdashboard_text_box_parent_class)->destroy(self);
}

/* IMPLEMENTATION: Interface XfdashboardFocusable */

/* Determine if actor can get the focus */
static gboolean _xfdashboard_text_box_focusable_can_focus(XfdashboardFocusable *inFocusable)
{
	XfdashboardTextBox				*self;
	XfdashboardTextBoxPrivate		*priv;
	XfdashboardFocusableInterface	*selfIface;
	XfdashboardFocusableInterface	*parentIface;

	g_return_val_if_fail(XFDASHBOARD_IS_TEXT_BOX(inFocusable), CLUTTER_EVENT_PROPAGATE);

	self=XFDASHBOARD_TEXT_BOX(inFocusable);
	priv=self->priv;

	/* Check if actor could be focused at all ... */
	selfIface=XFDASHBOARD_FOCUSABLE_GET_IFACE(inFocusable);
	parentIface=g_type_interface_peek_parent(selfIface);

	if(parentIface && parentIface->can_focus)
	{
		if(!parentIface->can_focus(inFocusable)) return(FALSE);
	}

	/* ... then only an editable text box can be focused */
	if(priv->isEditable) return(TRUE);

	/* If we get here this actor cannot be focused */
	return(FALSE);
}

/* Interface initialization
 * Set up default functions
 */
void _xfdashboard_text_box_focusable_iface_init(XfdashboardFocusableInterface *iface)
{
	iface->can_focus=_xfdashboard_text_box_focusable_can_focus;
}

/* IMPLEMENTATION: GObject */

/* Dispose this object */
static void _xfdashboard_text_box_dispose(GObject *inObject)
{
	XfdashboardTextBox			*self=XFDASHBOARD_TEXT_BOX(inObject);
	XfdashboardTextBoxPrivate	*priv=self->priv;

	/* Release our allocated variables */
	if(priv->primaryIconName)
	{
		g_free(priv->primaryIconName);
		priv->primaryIconName=NULL;
	}

	if(priv->secondaryIconName)
	{
		g_free(priv->secondaryIconName);
		priv->secondaryIconName=NULL;
	}

	if(priv->textFont)
	{
		g_free(priv->textFont);
		priv->textFont=NULL;
	}

	if(priv->textColor)
	{
		clutter_color_free(priv->textColor);
		priv->textColor=NULL;
	}

	if(priv->selectionTextColor)
	{
		clutter_color_free(priv->selectionTextColor);
		priv->selectionTextColor=NULL;
	}

	if(priv->selectionBackgroundColor)
	{
		clutter_color_free(priv->selectionBackgroundColor);
		priv->selectionBackgroundColor=NULL;
	}

	if(priv->hintTextFont)
	{
		g_free(priv->hintTextFont);
		priv->hintTextFont=NULL;
	}

	if(priv->hintTextColor)
	{
		clutter_color_free(priv->hintTextColor);
		priv->hintTextColor=NULL;
	}

	/* Call parent's class dispose method */
	G_OBJECT_CLASS(xfdashboard_text_box_parent_class)->dispose(inObject);
}

/* Set/get properties */
static void _xfdashboard_text_box_set_property(GObject *inObject,
												guint inPropID,
												const GValue *inValue,
												GParamSpec *inSpec)
{
	XfdashboardTextBox			*self=XFDASHBOARD_TEXT_BOX(inObject);

	switch(inPropID)
	{
		case PROP_PADDING:
			xfdashboard_text_box_set_padding(self, g_value_get_float(inValue));
			break;

		case PROP_SPACING:
			xfdashboard_text_box_set_spacing(self, g_value_get_float(inValue));
			break;

		case PROP_EDITABLE:
			xfdashboard_text_box_set_editable(self, g_value_get_boolean(inValue));
			break;

		case PROP_PRIMARY_ICON_NAME:
			xfdashboard_text_box_set_primary_icon(self, g_value_get_string(inValue));
			break;

		case PROP_SECONDARY_ICON_NAME:
			xfdashboard_text_box_set_secondary_icon(self, g_value_get_string(inValue));
			break;

		case PROP_TEXT:
			xfdashboard_text_box_set_text(self, g_value_get_string(inValue));
			break;

		case PROP_TEXT_FONT:
			xfdashboard_text_box_set_text_font(self, g_value_get_string(inValue));
			break;

		case PROP_TEXT_COLOR:
			xfdashboard_text_box_set_text_color(self, clutter_value_get_color(inValue));
			break;

		case PROP_SELECTION_TEXT_COLOR:
			xfdashboard_text_box_set_selection_text_color(self, clutter_value_get_color(inValue));
			break;

		case PROP_SELECTION_BACKGROUND_COLOR:
			xfdashboard_text_box_set_selection_background_color(self, clutter_value_get_color(inValue));
			break;

		case PROP_HINT_TEXT:
			xfdashboard_text_box_set_hint_text(self, g_value_get_string(inValue));
			break;

		case PROP_HINT_TEXT_FONT:
			xfdashboard_text_box_set_hint_text_font(self, g_value_get_string(inValue));
			break;

		case PROP_HINT_TEXT_COLOR:
			xfdashboard_text_box_set_hint_text_color(self, clutter_value_get_color(inValue));
			break;

		default:
			G_OBJECT_WARN_INVALID_PROPERTY_ID(inObject, inPropID, inSpec);
			break;
	}
}

static void _xfdashboard_text_box_get_property(GObject *inObject,
												guint inPropID,
												GValue *outValue,
												GParamSpec *inSpec)
{
	XfdashboardTextBox				*self=XFDASHBOARD_TEXT_BOX(inObject);
	XfdashboardTextBoxPrivate		*priv=self->priv;

	switch(inPropID)
	{
		case PROP_PADDING:
			g_value_set_float(outValue, priv->padding);
			break;

		case PROP_SPACING:
			g_value_set_float(outValue, priv->spacing);
			break;

		case PROP_EDITABLE:
			g_value_set_boolean(outValue, priv->isEditable);
			break;

		case PROP_PRIMARY_ICON_NAME:
			g_value_set_string(outValue, priv->primaryIconName);
			break;

		case PROP_SECONDARY_ICON_NAME:
			g_value_set_string(outValue, priv->secondaryIconName);
			break;

		case PROP_TEXT:
			g_value_set_string(outValue, clutter_text_get_text(CLUTTER_TEXT(priv->actorTextBox)));
			break;

		case PROP_TEXT_FONT:
			g_value_set_string(outValue, priv->textFont);
			break;

		case PROP_TEXT_COLOR:
			clutter_value_set_color(outValue, priv->textColor);
			break;

		case PROP_SELECTION_TEXT_COLOR:
			clutter_value_set_color(outValue, priv->selectionTextColor);
			break;

		case PROP_SELECTION_BACKGROUND_COLOR:
			clutter_value_set_color(outValue, priv->selectionBackgroundColor);
			break;

		case PROP_HINT_TEXT:
			g_value_set_string(outValue, clutter_text_get_text(CLUTTER_TEXT(priv->actorHintLabel)));
			break;

		case PROP_HINT_TEXT_FONT:
			g_value_set_string(outValue, priv->hintTextFont);
			break;

		case PROP_HINT_TEXT_COLOR:
			clutter_value_set_color(outValue, priv->hintTextColor);
			break;

		case PROP_HINT_TEXT_SET:
			g_value_set_boolean(outValue, priv->hintTextSet);
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
static void xfdashboard_text_box_class_init(XfdashboardTextBoxClass *klass)
{
	XfdashboardActorClass	*actorClass=XFDASHBOARD_ACTOR_CLASS(klass);
	ClutterActorClass		*clutterActorClass=CLUTTER_ACTOR_CLASS(klass);
	GObjectClass			*gobjectClass=G_OBJECT_CLASS(klass);

	/* Override functions */
	gobjectClass->dispose=_xfdashboard_text_box_dispose;
	gobjectClass->set_property=_xfdashboard_text_box_set_property;
	gobjectClass->get_property=_xfdashboard_text_box_get_property;

	clutterActorClass->show_all=_xfdashboard_text_box_show;
	clutterActorClass->get_preferred_width=_xfdashboard_text_box_get_preferred_width;
	clutterActorClass->get_preferred_height=_xfdashboard_text_box_get_preferred_height;
	clutterActorClass->allocate=_xfdashboard_text_box_allocate;
	clutterActorClass->destroy=_xfdashboard_text_box_destroy;
	clutterActorClass->key_focus_in=_xfdashboard_text_box_key_focus_in;

	/* Define properties */
	XfdashboardTextBoxProperties[PROP_PADDING]=
		g_param_spec_float("padding",
							_("Padding"),
							_("Padding between background and elements"),
							0.0f, G_MAXFLOAT,
							0.0f,
							G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS);

	XfdashboardTextBoxProperties[PROP_SPACING]=
		g_param_spec_float("spacing",
							_("Spacing"),
							_("Spacing between text and icon"),
							0.0f, G_MAXFLOAT,
							0.0f,
							G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS);

	XfdashboardTextBoxProperties[PROP_EDITABLE]=
		g_param_spec_boolean("editable",
								_("Editable"),
								_("Flag to set if text is editable"),
								FALSE,
								G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS);

	XfdashboardTextBoxProperties[PROP_PRIMARY_ICON_NAME]=
		g_param_spec_string("primary-icon-name",
							_("Primary icon name"),
							_("Themed icon name or file name of primary icon shown left of text box"),
							NULL,
							G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS);

	XfdashboardTextBoxProperties[PROP_SECONDARY_ICON_NAME]=
		g_param_spec_string("secondary-icon-name",
							_("Secondary icon name"),
							_("Themed icon name or file name of secondary icon shown right of text box"),
							NULL,
							G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS);

	XfdashboardTextBoxProperties[PROP_TEXT]=
		g_param_spec_string("text",
							_("Text"),
							_("Text of editable text box"),
							N_(""),
							G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS);

	XfdashboardTextBoxProperties[PROP_TEXT_FONT]=
		g_param_spec_string("text-font",
							_("Text font"),
							_("Font of editable text box"),
							NULL,
							G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS);

	XfdashboardTextBoxProperties[PROP_TEXT_COLOR]=
		clutter_param_spec_color("text-color",
									_("Text color"),
									_("Color of text in editable text box"),
									NULL,
									G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS);


	XfdashboardTextBoxProperties[PROP_SELECTION_TEXT_COLOR]=
		clutter_param_spec_color("selection-text-color",
									_("Selection text color"),
									_("Color of text of selected text"),
									NULL,
									G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS);

	XfdashboardTextBoxProperties[PROP_SELECTION_BACKGROUND_COLOR]=
		clutter_param_spec_color("selection-background-color",
									_("Selection background color"),
									_("Color of background of selected text"),
									NULL,
									G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS);

	XfdashboardTextBoxProperties[PROP_HINT_TEXT]=
		g_param_spec_string("hint-text",
							_("Hint text"),
							_("Hint text shown if editable text box is empty"),
							NULL,
							G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS);

	XfdashboardTextBoxProperties[PROP_HINT_TEXT_FONT]=
		g_param_spec_string("hint-text-font",
							_("Hint text font"),
							_("Font of hint text shown if editable text box is empty"),
							NULL,
							G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS);

	XfdashboardTextBoxProperties[PROP_HINT_TEXT_COLOR]=
		clutter_param_spec_color("hint-text-color",
									_("Hint text color"),
									_("Color of hint text shown if editable text box is empty"),
									NULL,
									G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS);

	XfdashboardTextBoxProperties[PROP_HINT_TEXT_SET]=
		g_param_spec_boolean("hint-text-set",
								_("Hint text set"),
								_("Flag indicating if a hint text was set"),
								FALSE,
								G_PARAM_READABLE | G_PARAM_STATIC_STRINGS);

	g_object_class_install_properties(gobjectClass, PROP_LAST, XfdashboardTextBoxProperties);

	/* Define stylable properties */
	xfdashboard_actor_install_stylable_property(actorClass, XfdashboardTextBoxProperties[PROP_PADDING]);
	xfdashboard_actor_install_stylable_property(actorClass, XfdashboardTextBoxProperties[PROP_SPACING]);
	xfdashboard_actor_install_stylable_property(actorClass, XfdashboardTextBoxProperties[PROP_PRIMARY_ICON_NAME]);
	xfdashboard_actor_install_stylable_property(actorClass, XfdashboardTextBoxProperties[PROP_SECONDARY_ICON_NAME]);
	xfdashboard_actor_install_stylable_property(actorClass, XfdashboardTextBoxProperties[PROP_TEXT_FONT]);
	xfdashboard_actor_install_stylable_property(actorClass, XfdashboardTextBoxProperties[PROP_TEXT_COLOR]);
	xfdashboard_actor_install_stylable_property(actorClass, XfdashboardTextBoxProperties[PROP_SELECTION_BACKGROUND_COLOR]);
	xfdashboard_actor_install_stylable_property(actorClass, XfdashboardTextBoxProperties[PROP_HINT_TEXT_FONT]);
	xfdashboard_actor_install_stylable_property(actorClass, XfdashboardTextBoxProperties[PROP_HINT_TEXT_COLOR]);

	/* Define signals */
	XfdashboardTextBoxSignals[SIGNAL_TEXT_CHANGED]=
		g_signal_new("text-changed",
						G_TYPE_FROM_CLASS(klass),
						G_SIGNAL_RUN_LAST,
						G_STRUCT_OFFSET(XfdashboardTextBoxClass, text_changed),
						NULL,
						NULL,
						g_cclosure_marshal_VOID__STRING,
						G_TYPE_NONE,
						1,
						G_TYPE_STRING);

	XfdashboardTextBoxSignals[SIGNAL_PRIMARY_ICON_CLICKED]=
		g_signal_new("primary-icon-clicked",
						G_TYPE_FROM_CLASS(klass),
						G_SIGNAL_RUN_LAST,
						G_STRUCT_OFFSET(XfdashboardTextBoxClass, primary_icon_clicked),
						NULL,
						NULL,
						g_cclosure_marshal_VOID__VOID,
						G_TYPE_NONE,
						0);

	XfdashboardTextBoxSignals[SIGNAL_SECONDARY_ICON_CLICKED]=
		g_signal_new("secondary-icon-clicked",
						G_TYPE_FROM_CLASS(klass),
						G_SIGNAL_RUN_LAST,
						G_STRUCT_OFFSET(XfdashboardTextBoxClass, secondary_icon_clicked),
						NULL,
						NULL,
						g_cclosure_marshal_VOID__VOID,
						G_TYPE_NONE,
						0);
}

/* Object initialization
 * Create private structure and set up default values
 */
static void xfdashboard_text_box_init(XfdashboardTextBox *self)
{
	XfdashboardTextBoxPrivate	*priv;

	priv=self->priv=xfdashboard_text_box_get_instance_private(self);

	/* This actor is react on events */
	clutter_actor_set_reactive(CLUTTER_ACTOR(self), TRUE);

	/* Set up default values */
	priv->padding=0.0f;
	priv->spacing=0.0f;
	priv->isEditable=FALSE;
	priv->primaryIconName=NULL;
	priv->secondaryIconName=NULL;
	priv->textFont=NULL;
	priv->textColor=NULL;
	priv->selectionTextColor=NULL;
	priv->selectionBackgroundColor=NULL;
	priv->hintTextFont=NULL;
	priv->hintTextColor=NULL;
	priv->showPrimaryIcon=FALSE;
	priv->showSecondaryIcon=FALSE;
	priv->selectionColorSet=FALSE;
	priv->hintTextSet=FALSE;

	/* Create actors */
	g_signal_connect(self, "key-press-event", G_CALLBACK(_xfdashboard_text_box_on_key_press_event), NULL);
	g_signal_connect(self, "key-release-event", G_CALLBACK(_xfdashboard_text_box_on_key_release_event), NULL);

	priv->actorPrimaryIcon=xfdashboard_button_new();
	xfdashboard_stylable_add_class(XFDASHBOARD_STYLABLE(priv->actorPrimaryIcon), "primary-icon");
	clutter_actor_set_reactive(priv->actorPrimaryIcon, TRUE);
	clutter_actor_hide(priv->actorPrimaryIcon);
	clutter_actor_add_child(CLUTTER_ACTOR(self), priv->actorPrimaryIcon);
	g_signal_connect_swapped(priv->actorPrimaryIcon, "clicked", G_CALLBACK(_xfdashboard_text_box_on_primary_icon_clicked), self);

	priv->actorSecondaryIcon=xfdashboard_button_new();
	xfdashboard_stylable_add_class(XFDASHBOARD_STYLABLE(priv->actorSecondaryIcon), "secondary-icon");
	clutter_actor_set_reactive(priv->actorSecondaryIcon, TRUE);
	clutter_actor_hide(priv->actorSecondaryIcon);
	clutter_actor_add_child(CLUTTER_ACTOR(self), priv->actorSecondaryIcon);
	g_signal_connect_swapped(priv->actorSecondaryIcon, "clicked", G_CALLBACK(_xfdashboard_text_box_on_secondary_icon_clicked), self);

	priv->actorTextBox=clutter_text_new();
	clutter_actor_add_child(CLUTTER_ACTOR(self), priv->actorTextBox);
	clutter_actor_set_reactive(priv->actorTextBox, TRUE);
	clutter_text_set_selectable(CLUTTER_TEXT(priv->actorTextBox), FALSE);
	clutter_text_set_editable(CLUTTER_TEXT(priv->actorTextBox), FALSE);
	clutter_text_set_single_line_mode(CLUTTER_TEXT(priv->actorTextBox), TRUE);
	g_signal_connect_swapped(priv->actorTextBox, "text-changed", G_CALLBACK(_xfdashboard_text_box_on_text_changed), self);

	priv->actorHintLabel=clutter_text_new();
	clutter_actor_add_child(CLUTTER_ACTOR(self), priv->actorHintLabel);
	clutter_actor_set_reactive(priv->actorHintLabel, FALSE);
	clutter_text_set_selectable(CLUTTER_TEXT(priv->actorHintLabel), FALSE);
	clutter_text_set_editable(CLUTTER_TEXT(priv->actorHintLabel), FALSE);
	clutter_text_set_single_line_mode(CLUTTER_TEXT(priv->actorHintLabel), TRUE);
	clutter_actor_hide(priv->actorHintLabel);
}

/* IMPLEMENTATION: Public API */

/* Create new actor */
ClutterActor* xfdashboard_text_box_new(void)
{
	return(g_object_new(XFDASHBOARD_TYPE_TEXT_BOX, NULL));
}

/* Get/set padding of background to text and icon actors */
gfloat xfdashboard_text_box_get_padding(XfdashboardTextBox *self)
{
	g_return_val_if_fail(XFDASHBOARD_IS_TEXT_BOX(self), 0);

	return(self->priv->padding);
}

void xfdashboard_text_box_set_padding(XfdashboardTextBox *self, gfloat inPadding)
{
	XfdashboardTextBoxPrivate	*priv;

	g_return_if_fail(XFDASHBOARD_IS_TEXT_BOX(self));
	g_return_if_fail(inPadding>=0.0f);

	priv=self->priv;

	/* Set value if changed */
	if(priv->padding!=inPadding)
	{
		priv->padding=inPadding;
		clutter_actor_queue_relayout(CLUTTER_ACTOR(self));

		/* Notify about property change */
		g_object_notify_by_pspec(G_OBJECT(self), XfdashboardTextBoxProperties[PROP_PADDING]);
	}
}

/* Get/set spacing between text and icon actors */
gfloat xfdashboard_text_box_get_spacing(XfdashboardTextBox *self)
{
	g_return_val_if_fail(XFDASHBOARD_IS_TEXT_BOX(self), 0);

	return(self->priv->spacing);
}

void xfdashboard_text_box_set_spacing(XfdashboardTextBox *self, gfloat inSpacing)
{
	XfdashboardTextBoxPrivate	*priv;

	g_return_if_fail(XFDASHBOARD_IS_TEXT_BOX(self));
	g_return_if_fail(inSpacing>=0.0f);

	priv=self->priv;

	/* Set value if changed */
	if(priv->spacing!=inSpacing)
	{
		priv->spacing=inSpacing;
		clutter_actor_queue_relayout(CLUTTER_ACTOR(self));

		/* Notify about property change */
		g_object_notify_by_pspec(G_OBJECT(self), XfdashboardTextBoxProperties[PROP_SPACING]);
	}
}

/* Get/set flag to mark text editable */
gboolean xfdashboard_text_box_get_editable(XfdashboardTextBox *self)
{
	g_return_val_if_fail(XFDASHBOARD_IS_TEXT_BOX(self), FALSE);

	return(self->priv->isEditable);
}

void xfdashboard_text_box_set_editable(XfdashboardTextBox *self, gboolean isEditable)
{
	XfdashboardTextBoxPrivate	*priv;
	const gchar					*text;

	g_return_if_fail(XFDASHBOARD_IS_TEXT_BOX(self));

	priv=self->priv;

	/* Set value if changed */
	if(priv->isEditable!=isEditable)
	{
		priv->isEditable=isEditable;
		if(priv->isEditable) xfdashboard_stylable_add_pseudo_class(XFDASHBOARD_STYLABLE(self), "editable");
			else xfdashboard_stylable_remove_pseudo_class(XFDASHBOARD_STYLABLE(self), "editable");

		/* Set up actors */
		clutter_text_set_selectable(CLUTTER_TEXT(priv->actorTextBox), priv->isEditable);
		clutter_text_set_editable(CLUTTER_TEXT(priv->actorTextBox), priv->isEditable);

		text=clutter_text_get_text(CLUTTER_TEXT(priv->actorTextBox));
		if((text==NULL || *text==0) && priv->isEditable)
		{
			clutter_actor_show(priv->actorHintLabel);
		}
			else
			{
				clutter_actor_hide(priv->actorHintLabel);
			}

		clutter_actor_queue_relayout(CLUTTER_ACTOR(self));

		/* Notify about property change */
		g_object_notify_by_pspec(G_OBJECT(self), XfdashboardTextBoxProperties[PROP_EDITABLE]);
	}
}

/* Get/set text of editable text box */
gboolean xfdashboard_text_box_is_empty(XfdashboardTextBox *self)
{
	const gchar					*text;

	g_return_val_if_fail(XFDASHBOARD_IS_TEXT_BOX(self), TRUE);

	text=clutter_text_get_text(CLUTTER_TEXT(self->priv->actorTextBox));

	return(text==NULL || *text==0);
}

gint xfdashboard_text_box_get_length(XfdashboardTextBox *self)
{
	const gchar					*text;
	gint						textLength=0;

	g_return_val_if_fail(XFDASHBOARD_IS_TEXT_BOX(self), 0);

	text=clutter_text_get_text(CLUTTER_TEXT(self->priv->actorTextBox));
	if(text) textLength=strlen(text);

	return(textLength);
}

const gchar* xfdashboard_text_box_get_text(XfdashboardTextBox *self)
{
	g_return_val_if_fail(XFDASHBOARD_IS_TEXT_BOX(self), NULL);

	return(clutter_text_get_text(CLUTTER_TEXT(self->priv->actorTextBox)));
}

void xfdashboard_text_box_set_text(XfdashboardTextBox *self, const gchar *inMarkupText)
{
	XfdashboardTextBoxPrivate	*priv;
	const gchar					*text;

	g_return_if_fail(XFDASHBOARD_IS_TEXT_BOX(self));

	priv=self->priv;

	/* Set value if changed */
	if(g_strcmp0(clutter_text_get_text(CLUTTER_TEXT(priv->actorTextBox)), inMarkupText)!=0)
	{
		clutter_text_set_markup(CLUTTER_TEXT(priv->actorTextBox), inMarkupText);

		text=clutter_text_get_text(CLUTTER_TEXT(priv->actorTextBox));
		if((text==NULL || *text==0) && priv->isEditable)
		{
			clutter_actor_show(priv->actorHintLabel);
		}
			else
			{
				clutter_actor_hide(priv->actorHintLabel);
			}

		clutter_actor_queue_relayout(CLUTTER_ACTOR(self));

		/* Notify about property change */
		g_object_notify_by_pspec(G_OBJECT(self), XfdashboardTextBoxProperties[PROP_TEXT]);
	}
}

void xfdashboard_text_box_set_text_va(XfdashboardTextBox *self, const gchar *inFormat, ...)
{
	gchar						*text;
	va_list						args;

	g_return_if_fail(XFDASHBOARD_IS_TEXT_BOX(self));

	/* Build formatted text to set */
	va_start(args, inFormat);
	text=g_strdup_vprintf(inFormat, args);
	va_end(args);

	/* Set formatted text */
	xfdashboard_text_box_set_text(self, text);

	/* Release allocated resources */
	g_free(text);
}

/* Get/set font of editable text box */
const gchar* xfdashboard_text_box_get_text_font(XfdashboardTextBox *self)
{
	g_return_val_if_fail(XFDASHBOARD_IS_TEXT_BOX(self), NULL);

	return(self->priv->actorTextBox!=NULL ? self->priv->textFont : NULL);
}

void xfdashboard_text_box_set_text_font(XfdashboardTextBox *self, const gchar *inFont)
{
	XfdashboardTextBoxPrivate	*priv;

	g_return_if_fail(XFDASHBOARD_IS_TEXT_BOX(self));

	priv=self->priv;

	/* Set value if changed */
	if(g_strcmp0(priv->textFont, inFont)!=0)
	{
		if(priv->textFont) g_free(priv->textFont);
		priv->textFont=g_strdup(inFont);

		clutter_text_set_font_name(CLUTTER_TEXT(priv->actorTextBox), priv->textFont);
		clutter_actor_queue_relayout(CLUTTER_ACTOR(self));

		/* Notify about property change */
		g_object_notify_by_pspec(G_OBJECT(self), XfdashboardTextBoxProperties[PROP_TEXT_FONT]);
	}
}

/* Get/set color of text in editable text box */
const ClutterColor* xfdashboard_text_box_get_text_color(XfdashboardTextBox *self)
{
	g_return_val_if_fail(XFDASHBOARD_IS_TEXT_BOX(self), NULL);

	return(self->priv->textColor);
}

void xfdashboard_text_box_set_text_color(XfdashboardTextBox *self, const ClutterColor *inColor)
{
	XfdashboardTextBoxPrivate	*priv;
	ClutterColor				selectionColor;

	g_return_if_fail(XFDASHBOARD_IS_TEXT_BOX(self));
	g_return_if_fail(inColor);

	priv=self->priv;

	/* Set value if changed */
	if(!priv->textColor || !clutter_color_equal(inColor, priv->textColor))
	{
		if(priv->textColor) clutter_color_free(priv->textColor);
		priv->textColor=clutter_color_copy(inColor);

		clutter_text_set_color(CLUTTER_TEXT(priv->actorTextBox), priv->textColor);

		/* Selection text and background color is inverted text color if not set */
		if(!priv->selectionColorSet)
		{
			selectionColor.red=0xff-priv->textColor->red;
			selectionColor.green=0xff-priv->textColor->green;
			selectionColor.blue=0xff-priv->textColor->blue;
			selectionColor.alpha=priv->textColor->alpha;
			clutter_text_set_selected_text_color(CLUTTER_TEXT(priv->actorTextBox), &selectionColor);

			/* Selection color is the same as text color */
			clutter_text_set_selection_color(CLUTTER_TEXT(priv->actorTextBox), priv->textColor);
		}

		/* Redraw actor in new color */
		clutter_actor_queue_redraw(CLUTTER_ACTOR(self));

		/* Notify about property change */
		g_object_notify_by_pspec(G_OBJECT(self), XfdashboardTextBoxProperties[PROP_TEXT_COLOR]);
	}
}

/* Get/set color of text of selected text */
const ClutterColor* xfdashboard_text_box_get_selection_text_color(XfdashboardTextBox *self)
{
	g_return_val_if_fail(XFDASHBOARD_IS_TEXT_BOX(self), NULL);

	return(self->priv->selectionTextColor);
}

void xfdashboard_text_box_set_selection_text_color(XfdashboardTextBox *self, const ClutterColor *inColor)
{
	XfdashboardTextBoxPrivate	*priv;
	ClutterColor				selectionColor;

	g_return_if_fail(XFDASHBOARD_IS_TEXT_BOX(self));

	priv=self->priv;

	/* Set value if changed */
	if(priv->selectionTextColor!=inColor ||
		(priv->selectionTextColor &&
			inColor &&
			!clutter_color_equal(inColor, priv->selectionTextColor)))
	{
		/* Freeze notifications and collect them */
		g_object_freeze_notify(G_OBJECT(self));

		/* Release old color */
		if(priv->selectionTextColor)
		{
			clutter_color_free(priv->selectionTextColor);
			priv->selectionTextColor=NULL;

			/* Check if any selection color is set */
			priv->selectionColorSet=((priv->selectionTextColor && priv->selectionBackgroundColor) ? TRUE : FALSE);

			/* Notify about property change */
			g_object_notify_by_pspec(G_OBJECT(self), XfdashboardTextBoxProperties[PROP_SELECTION_TEXT_COLOR]);
		}

		/* Set new color if available */
		if(inColor)
		{
			priv->selectionTextColor=clutter_color_copy(inColor);
			clutter_text_set_selected_text_color(CLUTTER_TEXT(priv->actorTextBox), priv->selectionTextColor);

			/* Any selection color was set */
			priv->selectionColorSet=TRUE;

			/* Notify about property change */
			g_object_notify_by_pspec(G_OBJECT(self), XfdashboardTextBoxProperties[PROP_SELECTION_TEXT_COLOR]);
		}

		/* Selection text and background color is inverted text color if not set */
		if(!priv->selectionColorSet)
		{
			selectionColor.red=0xff-priv->textColor->red;
			selectionColor.green=0xff-priv->textColor->green;
			selectionColor.blue=0xff-priv->textColor->blue;
			selectionColor.alpha=priv->textColor->alpha;
			clutter_text_set_selected_text_color(CLUTTER_TEXT(priv->actorTextBox), &selectionColor);

			/* Selection color is the same as text color */
			clutter_text_set_selection_color(CLUTTER_TEXT(priv->actorTextBox), priv->textColor);
		}

		/* Redraw actor in new color */
		clutter_actor_queue_redraw(CLUTTER_ACTOR(self));

		/* Thaw notifications and send them now */
		g_object_thaw_notify(G_OBJECT(self));
	}
}

/* Get/set color of background of selected text */
const ClutterColor* xfdashboard_text_box_get_selection_background_color(XfdashboardTextBox *self)
{
	g_return_val_if_fail(XFDASHBOARD_IS_TEXT_BOX(self), NULL);

	return(self->priv->selectionBackgroundColor);
}

void xfdashboard_text_box_set_selection_background_color(XfdashboardTextBox *self, const ClutterColor *inColor)
{
	XfdashboardTextBoxPrivate	*priv;
	ClutterColor				selectionColor;

	g_return_if_fail(XFDASHBOARD_IS_TEXT_BOX(self));

	priv=self->priv;

	/* Set value if changed */
	if(priv->selectionBackgroundColor!=inColor ||
		(priv->selectionBackgroundColor &&
			inColor &&
			!clutter_color_equal(inColor, priv->selectionBackgroundColor)))
	{
		/* Freeze notifications and collect them */
		g_object_freeze_notify(G_OBJECT(self));

		/* Release old color */
		if(priv->selectionBackgroundColor)
		{
			clutter_color_free(priv->selectionBackgroundColor);
			priv->selectionBackgroundColor=NULL;

			/* Check if any selection color is set */
			priv->selectionColorSet=((priv->selectionTextColor && priv->selectionBackgroundColor) ? TRUE : FALSE);

			/* Notify about property change */
			g_object_notify_by_pspec(G_OBJECT(self), XfdashboardTextBoxProperties[PROP_SELECTION_BACKGROUND_COLOR]);
		}

		/* Set new color if available */
		if(inColor)
		{
			priv->selectionBackgroundColor=clutter_color_copy(inColor);
			clutter_text_set_selection_color(CLUTTER_TEXT(priv->actorTextBox), priv->selectionBackgroundColor);

			/* Any selection color was set */
			priv->selectionColorSet=TRUE;

			/* Notify about property change */
			g_object_notify_by_pspec(G_OBJECT(self), XfdashboardTextBoxProperties[PROP_SELECTION_BACKGROUND_COLOR]);
		}

		/* Selection text and background color is inverted text color if not set */
		if(!priv->selectionColorSet)
		{
			selectionColor.red=0xff-priv->textColor->red;
			selectionColor.green=0xff-priv->textColor->green;
			selectionColor.blue=0xff-priv->textColor->blue;
			selectionColor.alpha=priv->textColor->alpha;
			clutter_text_set_selected_text_color(CLUTTER_TEXT(priv->actorTextBox), &selectionColor);

			/* Selection color is the same as text color */
			clutter_text_set_selection_color(CLUTTER_TEXT(priv->actorTextBox), priv->textColor);
		}

		/* Redraw actor in new color */
		clutter_actor_queue_redraw(CLUTTER_ACTOR(self));

		/* Thaw notifications and send them now */
		g_object_thaw_notify(G_OBJECT(self));
	}
}

/* Determine if a hint label was set */
gboolean xfdashboard_text_box_is_hint_text_set(XfdashboardTextBox *self)
{
	g_return_val_if_fail(XFDASHBOARD_IS_TEXT_BOX(self), FALSE);

	return(self->priv->hintTextSet);
}

/* Get/set text of hint label */
const gchar* xfdashboard_text_box_get_hint_text(XfdashboardTextBox *self)
{
	g_return_val_if_fail(XFDASHBOARD_IS_TEXT_BOX(self), NULL);

	return(clutter_text_get_text(CLUTTER_TEXT(self->priv->actorHintLabel)));
}

void xfdashboard_text_box_set_hint_text(XfdashboardTextBox *self, const gchar *inMarkupText)
{
	XfdashboardTextBoxPrivate	*priv;
	gboolean					newHintTextSet;

	g_return_if_fail(XFDASHBOARD_IS_TEXT_BOX(self));

	priv=self->priv;
	newHintTextSet=FALSE;

	/* Freeze notification */
	g_object_freeze_notify(G_OBJECT(self));

	/* Set value if changed */
	if(g_strcmp0(clutter_text_get_text(CLUTTER_TEXT(priv->actorHintLabel)), inMarkupText)!=0)
	{
		/* Set value */
		clutter_text_set_markup(CLUTTER_TEXT(priv->actorHintLabel), inMarkupText);
		clutter_actor_queue_relayout(CLUTTER_ACTOR(self));

		/* Notify about property change */
		g_object_notify_by_pspec(G_OBJECT(self), XfdashboardTextBoxProperties[PROP_HINT_TEXT]);
	}

	/* Check if a hint text was set and if read-only property changes.
	 * Note: NULL string will unset "hint-text-set" property (set to FALSE)
	 */
	if(inMarkupText) newHintTextSet=TRUE;
	if(newHintTextSet!=priv->hintTextSet)
	{
		/* Set flag */
		priv->hintTextSet=newHintTextSet;

		/* Notify about property change */
		g_object_notify_by_pspec(G_OBJECT(self), XfdashboardTextBoxProperties[PROP_HINT_TEXT_SET]);
	}

	/* Thaw notification */
	g_object_thaw_notify(G_OBJECT(self));
}

void xfdashboard_text_box_set_hint_text_va(XfdashboardTextBox *self, const gchar *inFormat, ...)
{
	gchar						*text;
	va_list						args;

	g_return_if_fail(XFDASHBOARD_IS_TEXT_BOX(self));

	/* Build formatted text to set */
	va_start(args, inFormat);
	text=g_strdup_vprintf(inFormat, args);
	va_end(args);

	/* Set formatted text */
	xfdashboard_text_box_set_hint_text(self, text);

	/* Release allocated resources */
	g_free(text);
}

/* Get/set font of hint label */
const gchar* xfdashboard_text_box_get_hint_text_font(XfdashboardTextBox *self)
{
	g_return_val_if_fail(XFDASHBOARD_IS_TEXT_BOX(self), NULL);

	return(self->priv->hintTextFont);
}

void xfdashboard_text_box_set_hint_text_font(XfdashboardTextBox *self, const gchar *inFont)
{
	XfdashboardTextBoxPrivate	*priv;

	g_return_if_fail(XFDASHBOARD_IS_TEXT_BOX(self));

	priv=self->priv;

	/* Set value if changed */
	if(g_strcmp0(priv->hintTextFont, inFont)!=0)
	{
		if(priv->hintTextFont) g_free(priv->hintTextFont);
		priv->hintTextFont=g_strdup(inFont);

		clutter_text_set_font_name(CLUTTER_TEXT(priv->actorHintLabel), priv->hintTextFont);
		clutter_actor_queue_relayout(CLUTTER_ACTOR(self));

		/* Notify about property change */
		g_object_notify_by_pspec(G_OBJECT(self), XfdashboardTextBoxProperties[PROP_HINT_TEXT_FONT]);
	}
}

/* Get/set color of text in hint label */
const ClutterColor* xfdashboard_text_box_get_hint_text_color(XfdashboardTextBox *self)
{
	g_return_val_if_fail(XFDASHBOARD_IS_TEXT_BOX(self), NULL);

	return(self->priv->hintTextColor);
}

void xfdashboard_text_box_set_hint_text_color(XfdashboardTextBox *self, const ClutterColor *inColor)
{
	XfdashboardTextBoxPrivate	*priv;

	g_return_if_fail(XFDASHBOARD_IS_TEXT_BOX(self));
	g_return_if_fail(inColor);

	priv=self->priv;

	/* Set value if changed */
	if(!priv->hintTextColor || !clutter_color_equal(inColor, priv->hintTextColor))
	{
		if(priv->hintTextColor) clutter_color_free(priv->hintTextColor);
		priv->hintTextColor=clutter_color_copy(inColor);

		clutter_text_set_color(CLUTTER_TEXT(priv->actorHintLabel), priv->hintTextColor);
		clutter_actor_queue_redraw(CLUTTER_ACTOR(self));

		/* Notify about property change */
		g_object_notify_by_pspec(G_OBJECT(self), XfdashboardTextBoxProperties[PROP_HINT_TEXT_COLOR]);
	}
}

/* Get/set primary icon */
const gchar* xfdashboard_text_box_get_primary_icon(XfdashboardTextBox *self)
{
	g_return_val_if_fail(XFDASHBOARD_IS_TEXT_BOX(self), NULL);

	return(self->priv->primaryIconName);
}

void xfdashboard_text_box_set_primary_icon(XfdashboardTextBox *self, const gchar *inIconName)
{
	XfdashboardTextBoxPrivate	*priv;

	g_return_if_fail(XFDASHBOARD_IS_TEXT_BOX(self));
	g_return_if_fail(!inIconName || strlen(inIconName)>0);

	priv=self->priv;

	/* Set themed icon name or icon file name for primary icon */
	if(g_strcmp0(priv->primaryIconName, inIconName)!=0)
	{
		/* Set new primary icon name */
		if(priv->primaryIconName)
		{
			g_free(priv->primaryIconName);
			priv->primaryIconName=NULL;
		}

		if(inIconName)
		{
			/* Load and set new icon */
			priv->primaryIconName=g_strdup(inIconName);
			xfdashboard_label_set_icon_name(XFDASHBOARD_LABEL(priv->actorPrimaryIcon), priv->primaryIconName);

			/* Show icon */
			priv->showPrimaryIcon=TRUE;
			clutter_actor_show(priv->actorPrimaryIcon);
			clutter_actor_queue_relayout(CLUTTER_ACTOR(self));
		}
			else
			{
				/* Hide icon */
				priv->showPrimaryIcon=FALSE;
				clutter_actor_hide(priv->actorPrimaryIcon);
				clutter_actor_queue_relayout(CLUTTER_ACTOR(self));
			}

		/* Notify about property change */
		g_object_notify_by_pspec(G_OBJECT(self), XfdashboardTextBoxProperties[PROP_PRIMARY_ICON_NAME]);
	}
}

/* Get/set secondary icon */
const gchar* xfdashboard_text_box_get_secondary_icon(XfdashboardTextBox *self)
{
	g_return_val_if_fail(XFDASHBOARD_IS_TEXT_BOX(self), NULL);

	return(self->priv->secondaryIconName);
}

void xfdashboard_text_box_set_secondary_icon(XfdashboardTextBox *self, const gchar *inIconName)
{
	XfdashboardTextBoxPrivate	*priv;

	g_return_if_fail(XFDASHBOARD_IS_TEXT_BOX(self));
	g_return_if_fail(!inIconName || strlen(inIconName)>0);

	priv=self->priv;

	/* Set themed icon name or icon file name for primary icon */
	if(g_strcmp0(priv->secondaryIconName, inIconName)!=0)
	{
		/* Set new primary icon name */
		if(priv->secondaryIconName)
		{
			g_free(priv->secondaryIconName);
			priv->secondaryIconName=NULL;
		}

		if(inIconName)
		{
			/* Load and set new icon */
			priv->secondaryIconName=g_strdup(inIconName);
			xfdashboard_label_set_icon_name(XFDASHBOARD_LABEL(priv->actorSecondaryIcon), priv->secondaryIconName);

			/* Show icon */
			priv->showSecondaryIcon=TRUE;
			clutter_actor_show(priv->actorSecondaryIcon);
			clutter_actor_queue_relayout(CLUTTER_ACTOR(self));
		}
			else
			{
				/* Hide icon */
				priv->showSecondaryIcon=FALSE;
				clutter_actor_hide(priv->actorSecondaryIcon);
				clutter_actor_queue_relayout(CLUTTER_ACTOR(self));
			}

		/* Notify about property change */
		g_object_notify_by_pspec(G_OBJECT(self), XfdashboardTextBoxProperties[PROP_SECONDARY_ICON_NAME]);
	}
}
