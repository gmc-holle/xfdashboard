/*
 * searchbox.c: An actor representing an editable text-box with icons
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

#include "searchbox.h"
#include "enums.h"

#include <gdk/gdk.h>
#include <math.h>
#include <string.h>

/* Define this class in GObject system */

G_DEFINE_TYPE(XfdashboardSearchBox,
				xfdashboard_searchbox,
				CLUTTER_TYPE_ACTOR)
                                                
/* Private structure - access only by public API if needed */
#define XFDASHBOARD_SEARCHBOX_GET_PRIVATE(obj) \
	(G_TYPE_INSTANCE_GET_PRIVATE((obj), XFDASHBOARD_TYPE_SEARCHBOX, XfdashboardSearchBoxPrivate))

struct _XfdashboardSearchBoxPrivate
{
	/* Actors for search box */
	ClutterText				*actorTextBox;
	ClutterText				*actorHintLabel;
	
	ClutterTexture			*actorPrimaryIcon;
	ClutterAction			*primaryIconClickAction;

	ClutterTexture			*actorSecondaryIcon;
	ClutterAction			*secondaryIconClickAction;

	/* Settings */
	gfloat					margin;
	gfloat					spacing;

	gchar					*primaryIconName;
	gchar					*secondaryIconName;

	gchar					*textFont;
	ClutterColor			*textColor;

	gchar					*hintTextFont;
	ClutterColor			*hintTextColor;

	gboolean				showBackground;
	ClutterColor			*backgroundColor;

	/* Internal variables */
	gint					lastTextLength;
};

/* Properties */
enum
{
	PROP_0,

	PROP_MARGIN,
	PROP_SPACING,

	PROP_PRIMARY_ICON_NAME,
	PROP_SECONDARY_ICON_NAME,

	PROP_TEXT,
	PROP_TEXT_FONT,
	PROP_TEXT_COLOR,

	PROP_HINT_TEXT,
	PROP_HINT_TEXT_FONT,
	PROP_HINT_TEXT_COLOR,

	PROP_BACKGROUND_VISIBLE,
	PROP_BACKGROUND_COLOR,

	PROP_LAST
};

static GParamSpec* XfdashboardSearchBoxProperties[PROP_LAST]={ 0, };

/* Signals */
enum
{
	PRIMARY_ICON_CLICKED,
	SECONDARY_ICON_CLICKED,

	SEARCH_STARTED,
	SEARCH_ENDED,
	TEXT_CHANGED,
	
	SIGNAL_LAST
};

static guint XfdashboardSearchBoxSignals[SIGNAL_LAST]={ 0, };

/* Private constants */
#define DEFAULT_SIZE	64

static ClutterColor		defaultTextColor={ 0xff, 0xff, 0xff, 0xff };
static ClutterColor		defaultHintTextColor={ 0xc0, 0xc0, 0xc0, 0xff };
static ClutterColor		defaultBackgroundColor={ 0x80, 0x80, 0x80, 0xff };

/* IMPLEMENTATION: Private variables and methods */

/* Update children of this actor */
void _xfdashboard_searchbox_update_icons(XfdashboardSearchBox *self)
{
	g_return_if_fail(XFDASHBOARD_IS_SEARCHBOX(self));

	XfdashboardSearchBoxPrivate		*priv=self->priv;
	gint							textLength=strlen(clutter_text_get_text(priv->actorTextBox));
	gfloat							iconSize;
	GdkPixbuf						*icon;
	GError							*error;

	/* Determine size of icon to load by getting label's height or width */
	if(textLength>0)
	{
		clutter_actor_get_preferred_size(CLUTTER_ACTOR(priv->actorTextBox),
											NULL, NULL,
											NULL, &iconSize);
	}
		else
		{
			clutter_actor_get_preferred_size(CLUTTER_ACTOR(priv->actorHintLabel),
												NULL, NULL,
												NULL, &iconSize);
		}

	/* Get scaled primary icon from icon name if set */
	if(priv->primaryIconName)
	{
		icon=xfdashboard_get_pixbuf_for_icon_name_scaled(priv->primaryIconName, iconSize);

		/* Update texture of actor */
		error=NULL;
		if(!clutter_texture_set_from_rgb_data(CLUTTER_TEXTURE(priv->actorPrimaryIcon),
													gdk_pixbuf_get_pixels(icon),
													gdk_pixbuf_get_has_alpha(icon),
													gdk_pixbuf_get_width(icon),
													gdk_pixbuf_get_height(icon),
													gdk_pixbuf_get_rowstride(icon),
													gdk_pixbuf_get_has_alpha(icon) ? 4 : 3,
													CLUTTER_TEXTURE_NONE,
													&error))
		{
			g_warning("Could not update primary icon of %s: %s",
						G_OBJECT_TYPE_NAME(self),
						(error && error->message) ?  error->message : "unknown error");
			if(error!=NULL) g_error_free(error);
		}
		g_object_unref(icon);

		clutter_actor_show(CLUTTER_ACTOR(priv->actorPrimaryIcon));
	}
		else clutter_actor_hide(CLUTTER_ACTOR(priv->actorPrimaryIcon));

	/* Get scaled secondary icon from icon name if set */
	if(priv->secondaryIconName)
	{
		icon=xfdashboard_get_pixbuf_for_icon_name_scaled(priv->secondaryIconName, iconSize);

		/* Update texture of actor */
		error=NULL;
		if(!clutter_texture_set_from_rgb_data(CLUTTER_TEXTURE(priv->actorSecondaryIcon),
													gdk_pixbuf_get_pixels(icon),
													gdk_pixbuf_get_has_alpha(icon),
													gdk_pixbuf_get_width(icon),
													gdk_pixbuf_get_height(icon),
													gdk_pixbuf_get_rowstride(icon),
													gdk_pixbuf_get_has_alpha(icon) ? 4 : 3,
													CLUTTER_TEXTURE_NONE,
													&error))
		{
			g_warning("Could not update secondary icon of %s: %s",
						G_OBJECT_TYPE_NAME(self),
						(error && error->message) ?  error->message : "unknown error");
			if(error!=NULL) g_error_free(error);
		}
		g_object_unref(icon);

		clutter_actor_show(CLUTTER_ACTOR(priv->actorSecondaryIcon));
	}
		else clutter_actor_hide(CLUTTER_ACTOR(priv->actorSecondaryIcon));

	/* Queue a redraw as the actors are now available */
	clutter_actor_queue_redraw(CLUTTER_ACTOR(self));
}

/* Text of editable text box has changed */
void _xfdashboard_searchbox_on_text_changed(ClutterText *inTextBox, gpointer inUserData)
{
	g_return_if_fail(XFDASHBOARD_IS_SEARCHBOX(inUserData));

	XfdashboardSearchBox		*self=XFDASHBOARD_SEARCHBOX(inUserData);
	XfdashboardSearchBoxPrivate	*priv=self->priv;
	gint						textLength=strlen(clutter_text_get_text(inTextBox));

	/* Check text length against previous text length as it will determine
	 * if hint label should be hidden. Emit "search-started" signal in this case.
	 */
	if(textLength>0 && priv->lastTextLength==0)
	{
		clutter_actor_hide(CLUTTER_ACTOR(priv->actorHintLabel));
		g_signal_emit(self, XfdashboardSearchBoxSignals[SEARCH_STARTED], 0);
	}

	/* Emit signal for text changed */
	g_signal_emit(self, XfdashboardSearchBoxSignals[TEXT_CHANGED], 0, clutter_text_get_text(inTextBox));

	/* Check text length against previous text length as it will determine
	 * if hint label should be shown. Emit "search-ended" signal in this case.
	 */
	if(textLength==0 && priv->lastTextLength>0)
	{
		clutter_actor_show(CLUTTER_ACTOR(priv->actorHintLabel));
		g_signal_emit(self, XfdashboardSearchBoxSignals[SEARCH_ENDED], 0);
	}

	/* Trace text length changes */
	priv->lastTextLength=textLength;
}

/* Primary icon was clicked */
void _xfdashboard_searchbox_on_primary_icon_clicked(ClutterClickAction *inAction,
													ClutterActor *self,
													gpointer inUserData)
{
	g_return_if_fail(XFDASHBOARD_IS_SEARCHBOX(inUserData));
	
	g_signal_emit(inUserData, XfdashboardSearchBoxSignals[PRIMARY_ICON_CLICKED], 0);
}

/* Secondary icon was clicked */
void _xfdashboard_searchbox_on_secondary_icon_clicked(ClutterClickAction *inAction,
														ClutterActor *self,
														gpointer inUserData)
{
	g_return_if_fail(XFDASHBOARD_IS_SEARCHBOX(inUserData));
	
	g_signal_emit(inUserData, XfdashboardSearchBoxSignals[SECONDARY_ICON_CLICKED], 0);
}

/* IMPLEMENTATION: ClutterActor */

/* Show all children of this one */
static void xfdashboard_searchbox_show_all(ClutterActor *self)
{
	XfdashboardSearchBoxPrivate	*priv=XFDASHBOARD_SEARCHBOX(self)->priv;
	gint						textLength=strlen(clutter_text_get_text(priv->actorTextBox));

	if(priv->primaryIconName) clutter_actor_show(CLUTTER_ACTOR(priv->actorPrimaryIcon));
	if(priv->secondaryIconName) clutter_actor_show(CLUTTER_ACTOR(priv->actorSecondaryIcon));

	clutter_actor_show(CLUTTER_ACTOR(priv->actorTextBox));

	if(textLength>0) clutter_actor_hide(CLUTTER_ACTOR(priv->actorHintLabel));
		else clutter_actor_show(CLUTTER_ACTOR(priv->actorHintLabel));

	clutter_actor_show(self);
}

/* Hide all children of this one */
static void xfdashboard_searchbox_hide_all(ClutterActor *self)
{
	XfdashboardSearchBoxPrivate	*priv=XFDASHBOARD_SEARCHBOX(self)->priv;

	clutter_actor_hide(self);
	clutter_actor_hide(CLUTTER_ACTOR(priv->actorTextBox));
	clutter_actor_hide(CLUTTER_ACTOR(priv->actorHintLabel));
	clutter_actor_hide(CLUTTER_ACTOR(priv->actorPrimaryIcon));
	clutter_actor_hide(CLUTTER_ACTOR(priv->actorSecondaryIcon));
}

/* Get preferred width/height */
static void xfdashboard_searchbox_get_preferred_height(ClutterActor *self,
														gfloat inForWidth,
														gfloat *outMinHeight,
														gfloat *outNaturalHeight)
{
	XfdashboardSearchBoxPrivate	*priv=XFDASHBOARD_SEARCHBOX(self)->priv;
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

	// Add margin
	minHeight+=2*priv->margin;
	naturalHeight+=2*priv->margin;

	/* Store sizes computed */
	if(outMinHeight) *outMinHeight=minHeight;
	if(outNaturalHeight) *outNaturalHeight=naturalHeight;
}

static void xfdashboard_searchbox_get_preferred_width(ClutterActor *self,
														gfloat inForHeight,
														gfloat *outMinWidth,
														gfloat *outNaturalWidth)
{
	XfdashboardSearchBoxPrivate	*priv=XFDASHBOARD_SEARCHBOX(self)->priv;
	gfloat						minWidth, naturalWidth;
	gfloat						childMinWidth, childNaturalWidth;
	gint						numberChildren=0;

	minWidth=naturalWidth=0.0f;

	/* Determine size of primary icon if visible */
	if(CLUTTER_ACTOR_IS_VISIBLE(priv->actorPrimaryIcon))
	{
		clutter_actor_get_preferred_width(CLUTTER_ACTOR(priv->actorPrimaryIcon),
											inForHeight,
											&childMinWidth, &childNaturalWidth);

		minWidth+=childMinWidth;
		naturalWidth+=childNaturalWidth;
		numberChildren++;
	}

	/* Determine size of editable text box if visible */
	if(CLUTTER_ACTOR_IS_VISIBLE(priv->actorTextBox))
	{
		clutter_actor_get_preferred_width(CLUTTER_ACTOR(priv->actorTextBox),
											inForHeight,
											&childMinWidth, &childNaturalWidth);

		minWidth+=childMinWidth;
		naturalWidth+=childNaturalWidth;
		numberChildren++;
	}

	/* Determine size of hint label if visible */
	if(CLUTTER_ACTOR_IS_VISIBLE(priv->actorHintLabel))
	{
		clutter_actor_get_preferred_width(CLUTTER_ACTOR(priv->actorHintLabel),
											inForHeight,
											&childMinWidth, &childNaturalWidth);

		minWidth+=childMinWidth;
		naturalWidth+=childNaturalWidth;
		numberChildren++;
	}

	/* Determine size of secondary icon if visible */
	if(CLUTTER_ACTOR_IS_VISIBLE(priv->actorSecondaryIcon))
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

	// Add margin
	minWidth+=2*priv->margin;
	naturalWidth+=2*priv->margin;

	/* Store sizes computed */
	if(outMinWidth) *outMinWidth=minWidth;
	if(outNaturalWidth) *outNaturalWidth=naturalWidth;
}

/* Allocate position and size of actor and its children*/
static void xfdashboard_searchbox_allocate(ClutterActor *self,
											const ClutterActorBox *inBox,
											ClutterAllocationFlags inFlags)
{
	XfdashboardSearchBoxPrivate	*priv=XFDASHBOARD_SEARCHBOX(self)->priv;
	ClutterActorBox				*box=NULL;
	gfloat						left, right, top, bottom;
	gfloat						iconWidth, iconHeight;

	/* Chain up to store the allocation of the actor */
	CLUTTER_ACTOR_CLASS(xfdashboard_searchbox_parent_class)->allocate(self, inBox, inFlags);

	/* Initialize bounding box (except right side) of allocation used in actors */
	left=top=priv->margin;
	right=clutter_actor_box_get_width(inBox)-priv->margin;
	bottom=clutter_actor_box_get_height(inBox)-priv->margin;

	/* Set allocation of primary icon if visible */
	if(CLUTTER_ACTOR_IS_VISIBLE(priv->actorPrimaryIcon))
	{
		gfloat					childRight;
		
		/* Get scale size of primary icon */
		clutter_actor_get_preferred_size(CLUTTER_ACTOR(priv->actorPrimaryIcon),
											NULL, NULL,
											&iconWidth, &iconHeight);
		iconWidth=(bottom-top)*(iconWidth/iconHeight);

		/* Set allocation */
		childRight=left+iconWidth;

		box=clutter_actor_box_new(floor(left), floor(top), floor(childRight), floor(bottom));
		clutter_actor_allocate(CLUTTER_ACTOR(priv->actorPrimaryIcon), box, inFlags);
		clutter_actor_box_free(box);

		/* Adjust bounding box for next actor */
		left=childRight+priv->spacing;
	}

	/* Set allocation of secondary icon if visible */
	if(CLUTTER_ACTOR_IS_VISIBLE(priv->actorSecondaryIcon))
	{
		gfloat					childLeft;
		
		/* Get scale size of primary icon */
		clutter_actor_get_preferred_size(CLUTTER_ACTOR(priv->actorSecondaryIcon),
											NULL, NULL,
											&iconWidth, &iconHeight);
		iconWidth=(bottom-top)*(iconWidth/iconHeight);

		/* Set allocation */
		childLeft=right-iconWidth;

		box=clutter_actor_box_new(floor(childLeft), floor(top), floor(right), floor(bottom));
		clutter_actor_allocate(CLUTTER_ACTOR(priv->actorSecondaryIcon), box, inFlags);
		clutter_actor_box_free(box);

		/* Adjust bounding box for next actor */
		right=childLeft-priv->spacing;
	}

	/* Set allocation of editable text box if visible */
	if(CLUTTER_ACTOR_IS_VISIBLE(priv->actorTextBox))
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
	if(CLUTTER_ACTOR_IS_VISIBLE(priv->actorHintLabel))
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

/* Paint actor */
static void xfdashboard_searchbox_paint(ClutterActor *self)
{
	XfdashboardSearchBoxPrivate	*priv=XFDASHBOARD_SEARCHBOX(self)->priv;

	/* Order of actors being painted is important! */
	if(priv->showBackground && priv->backgroundColor)
	{
		ClutterActorBox			allocation={ 0, };
		gfloat					width, height;

		/* Get allocation to paint background */
		clutter_actor_get_allocation_box(self, &allocation);
		clutter_actor_box_get_size(&allocation, &width, &height);

		/* Draw rectangle with round edges if margin is not zero */
		cogl_path_new();

		cogl_set_source_color4ub(priv->backgroundColor->red,
									priv->backgroundColor->green,
									priv->backgroundColor->blue,
									priv->backgroundColor->alpha);

		if(priv->margin>0.0f)
		{
			cogl_path_round_rectangle(0, 0, width, height, priv->margin, 0.1f);
		}
			else cogl_path_rectangle(0, 0, width, height);

		cogl_path_fill();
	}

	if(priv->actorPrimaryIcon &&
		CLUTTER_ACTOR_IS_VISIBLE(priv->actorPrimaryIcon)) clutter_actor_paint(CLUTTER_ACTOR(priv->actorPrimaryIcon));

	if(priv->actorSecondaryIcon &&
		CLUTTER_ACTOR_IS_VISIBLE(priv->actorSecondaryIcon)) clutter_actor_paint(CLUTTER_ACTOR(priv->actorSecondaryIcon));

	if(priv->actorHintLabel &&
		CLUTTER_ACTOR_IS_VISIBLE(priv->actorHintLabel)) clutter_actor_paint(CLUTTER_ACTOR(priv->actorHintLabel));

	if(priv->actorTextBox &&
		CLUTTER_ACTOR_IS_VISIBLE(priv->actorTextBox)) clutter_actor_paint(CLUTTER_ACTOR(priv->actorTextBox));
}

/* Pick this actor and possibly all the child actors.
 * That means this function should paint its silouhette as a solid shape in the
 * given color and call the paint function of its children. But never call the
 * paint function of itself especially if the paint function sets a different
 * color, e.g. by cogl_set_source_color* function family.
 */
static void xfdashboard_searchbox_pick(ClutterActor *self, const ClutterColor *inColor)
{
	XfdashboardSearchBoxPrivate	*priv=XFDASHBOARD_SEARCHBOX(self)->priv;

	/* It is possible to avoid a costly paint by checking
	 * whether the actor should really be painted in pick mode
	 */
	if(!clutter_actor_should_pick_paint(self)) return;

	/* Chain up so we get a bounding box painted (if we are reactive) */
	CLUTTER_ACTOR_CLASS(xfdashboard_searchbox_parent_class)->pick(self, inColor);

	/* Pick children */
	if(priv->showBackground && priv->backgroundColor)
	{
		ClutterActorBox			allocation={ 0, };
		gfloat					width, height;

		clutter_actor_get_allocation_box(self, &allocation);
		clutter_actor_box_get_size(&allocation, &width, &height);

		cogl_path_new();
		if(priv->margin>0.0f)
		{
			cogl_path_round_rectangle(0, 0, width, height, priv->margin, 0.1f);
		}
			else cogl_path_rectangle(0, 0, width, height);
		cogl_path_fill();
	}
	
	if(priv->actorPrimaryIcon &&
		CLUTTER_ACTOR_IS_VISIBLE(priv->actorPrimaryIcon)) clutter_actor_paint(CLUTTER_ACTOR(priv->actorPrimaryIcon));

	if(priv->actorSecondaryIcon &&
		CLUTTER_ACTOR_IS_VISIBLE(priv->actorSecondaryIcon)) clutter_actor_paint(CLUTTER_ACTOR(priv->actorSecondaryIcon));

	if(priv->actorHintLabel &&
		CLUTTER_ACTOR_IS_VISIBLE(priv->actorHintLabel)) clutter_actor_paint(CLUTTER_ACTOR(priv->actorHintLabel));

	if(priv->actorTextBox &&
		CLUTTER_ACTOR_IS_VISIBLE(priv->actorTextBox)) clutter_actor_paint(CLUTTER_ACTOR(priv->actorTextBox));
}

/* Destroy this actor */
static void xfdashboard_searchbox_destroy(ClutterActor *self)
{
	/* Destroy each child actor when this actor is destroyed */
	XfdashboardSearchBoxPrivate	*priv=XFDASHBOARD_SEARCHBOX(self)->priv;

	if(priv->actorTextBox) clutter_actor_destroy(CLUTTER_ACTOR(priv->actorTextBox));
	priv->actorTextBox=NULL;

	if(priv->actorHintLabel) clutter_actor_destroy(CLUTTER_ACTOR(priv->actorHintLabel));
	priv->actorHintLabel=NULL;

	if(priv->actorPrimaryIcon) clutter_actor_destroy(CLUTTER_ACTOR(priv->actorPrimaryIcon));
	priv->actorPrimaryIcon=NULL;

	if(priv->actorSecondaryIcon) clutter_actor_destroy(CLUTTER_ACTOR(priv->actorSecondaryIcon));
	priv->actorSecondaryIcon=NULL;

	/* Call parent's class destroy method */
	if(CLUTTER_ACTOR_CLASS(xfdashboard_searchbox_parent_class)->destroy)
	{
		CLUTTER_ACTOR_CLASS(xfdashboard_searchbox_parent_class)->destroy(self);
	}
}

/* This actor got keyboard focus */
static void xfdashboard_searchbox_key_focus_in(ClutterActor *self)
{
	/* Move keyboard focus to editable text box */
	XfdashboardSearchBoxPrivate	*priv=XFDASHBOARD_SEARCHBOX(self)->priv;
	ClutterActor				*stage=clutter_actor_get_stage(self);

	clutter_stage_set_key_focus(CLUTTER_STAGE(stage), CLUTTER_ACTOR(priv->actorTextBox));
}

/* IMPLEMENTATION: GObject */

/* Dispose this object */
static void xfdashboard_searchbox_dispose(GObject *inObject)
{
	/* Release our allocated variables */
	XfdashboardSearchBoxPrivate	*priv=XFDASHBOARD_SEARCHBOX(inObject)->priv;

	if(priv->primaryIconName) g_free(priv->primaryIconName);
	priv->primaryIconName=NULL;

	if(priv->secondaryIconName) g_free(priv->secondaryIconName);
	priv->secondaryIconName=NULL;
	
	if(priv->textFont) g_free(priv->textFont);
	priv->textFont=NULL;

	if(priv->textColor) clutter_color_free(priv->textColor);
	priv->textColor=NULL;

	if(priv->hintTextFont) g_free(priv->hintTextFont);
	priv->hintTextFont=NULL;

	if(priv->hintTextColor) clutter_color_free(priv->hintTextColor);
	priv->hintTextColor=NULL;

	if(priv->backgroundColor) clutter_color_free(priv->backgroundColor);
	priv->backgroundColor=NULL;

	/* Call parent's class dispose method */
	G_OBJECT_CLASS(xfdashboard_searchbox_parent_class)->dispose(inObject);
}

/* Set/get properties */
static void xfdashboard_searchbox_set_property(GObject *inObject,
												guint inPropID,
												const GValue *inValue,
												GParamSpec *inSpec)
{
	XfdashboardSearchBox			*self=XFDASHBOARD_SEARCHBOX(inObject);

	switch(inPropID)
	{
		case PROP_MARGIN:
			xfdashboard_searchbox_set_margin(self, g_value_get_float(inValue));
			break;

		case PROP_SPACING:
			xfdashboard_searchbox_set_spacing(self, g_value_get_float(inValue));
			break;

		case PROP_PRIMARY_ICON_NAME:
			xfdashboard_searchbox_set_primary_icon(self, g_value_get_string(inValue));
			break;

		case PROP_SECONDARY_ICON_NAME:
			xfdashboard_searchbox_set_secondary_icon(self, g_value_get_string(inValue));
			break;

		case PROP_TEXT:
			xfdashboard_searchbox_set_text(self, g_value_get_string(inValue));
			break;

		case PROP_TEXT_FONT:
			xfdashboard_searchbox_set_text_font(self, g_value_get_string(inValue));
			break;

		case PROP_TEXT_COLOR:
			xfdashboard_searchbox_set_text_color(self, clutter_value_get_color(inValue));
			break;

		case PROP_HINT_TEXT:
			xfdashboard_searchbox_set_hint_text(self, g_value_get_string(inValue));
			break;

		case PROP_HINT_TEXT_FONT:
			xfdashboard_searchbox_set_hint_text_font(self, g_value_get_string(inValue));
			break;

		case PROP_HINT_TEXT_COLOR:
			xfdashboard_searchbox_set_hint_text_color(self, clutter_value_get_color(inValue));
			break;

		case PROP_BACKGROUND_VISIBLE:
			xfdashboard_searchbox_set_background_visibility(self, g_value_get_boolean(inValue));
			break;

		case PROP_BACKGROUND_COLOR:
			xfdashboard_searchbox_set_background_color(self, clutter_value_get_color(inValue));
			break;

		default:
			G_OBJECT_WARN_INVALID_PROPERTY_ID(inObject, inPropID, inSpec);
			break;
	}
}

static void xfdashboard_searchbox_get_property(GObject *inObject,
												guint inPropID,
												GValue *outValue,
												GParamSpec *inSpec)
{
	XfdashboardSearchBox			*self=XFDASHBOARD_SEARCHBOX(inObject);
	XfdashboardSearchBoxPrivate		*priv=self->priv;

	switch(inPropID)
	{
		case PROP_MARGIN:
			g_value_set_float(outValue, priv->margin);
			break;

		case PROP_SPACING:
			g_value_set_float(outValue, priv->spacing);
			break;

		case PROP_PRIMARY_ICON_NAME:
			g_value_set_string(outValue, priv->primaryIconName);
			break;

		case PROP_SECONDARY_ICON_NAME:
			g_value_set_string(outValue, priv->secondaryIconName);
			break;

		case PROP_TEXT:
			g_value_set_string(outValue, clutter_text_get_text(priv->actorTextBox));
			break;

		case PROP_TEXT_FONT:
			g_value_set_string(outValue, priv->textFont);
			break;

		case PROP_TEXT_COLOR:
			clutter_value_set_color(outValue, priv->textColor);
			break;

		case PROP_HINT_TEXT:
			g_value_set_string(outValue, clutter_text_get_text(priv->actorHintLabel));
			break;

		case PROP_HINT_TEXT_FONT:
			g_value_set_string(outValue, priv->hintTextFont);
			break;

		case PROP_HINT_TEXT_COLOR:
			clutter_value_set_color(outValue, priv->hintTextColor);
			break;

		case PROP_BACKGROUND_VISIBLE:
			g_value_set_boolean(outValue, priv->showBackground);
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
static void xfdashboard_searchbox_class_init(XfdashboardSearchBoxClass *klass)
{
	ClutterActorClass	*actorClass=CLUTTER_ACTOR_CLASS(klass);
	GObjectClass		*gobjectClass=G_OBJECT_CLASS(klass);

	/* Override functions */
	gobjectClass->dispose=xfdashboard_searchbox_dispose;
	gobjectClass->set_property=xfdashboard_searchbox_set_property;
	gobjectClass->get_property=xfdashboard_searchbox_get_property;

	actorClass->show_all=xfdashboard_searchbox_show_all;
	actorClass->hide_all=xfdashboard_searchbox_hide_all;
	actorClass->paint=xfdashboard_searchbox_paint;
	actorClass->pick=xfdashboard_searchbox_pick;
	actorClass->get_preferred_width=xfdashboard_searchbox_get_preferred_width;
	actorClass->get_preferred_height=xfdashboard_searchbox_get_preferred_height;
	actorClass->allocate=xfdashboard_searchbox_allocate;
	actorClass->destroy=xfdashboard_searchbox_destroy;
	actorClass->key_focus_in=xfdashboard_searchbox_key_focus_in;

	/* Set up private structure */
	g_type_class_add_private(klass, sizeof(XfdashboardSearchBoxPrivate));

	/* Define properties */
	XfdashboardSearchBoxProperties[PROP_MARGIN]=
		g_param_spec_float("margin",
							"Margin",
							"Margin between background and elements",
							0.0f, G_MAXFLOAT,
							4.0f,
							G_PARAM_READWRITE | G_PARAM_CONSTRUCT);

	XfdashboardSearchBoxProperties[PROP_SPACING]=
		g_param_spec_float("spacing",
							"Spacing",
							"Spacing between text and icon",
							0.0f, G_MAXFLOAT,
							4.0f,
							G_PARAM_READWRITE | G_PARAM_CONSTRUCT);

	XfdashboardSearchBoxProperties[PROP_PRIMARY_ICON_NAME]=
		g_param_spec_string("primary-icon-name",
							"Primary icon name",
							"Themed icon name or file name of primary icon shown left of text box",
							"",
							G_PARAM_READWRITE);

	XfdashboardSearchBoxProperties[PROP_SECONDARY_ICON_NAME]=
		g_param_spec_string("secondary-icon-name",
							"Secondary icon name",
							"Themed icon name or file name of secondary icon shown right of text box",
							"",
							G_PARAM_READWRITE);

	XfdashboardSearchBoxProperties[PROP_TEXT]=
		g_param_spec_string("text",
							"Text",
							"Text of editable text box",
							"",
							G_PARAM_READWRITE);

	XfdashboardSearchBoxProperties[PROP_TEXT_FONT]=
		g_param_spec_string("text-font",
							"Text font",
							"Font of editable text box",
							NULL,
							G_PARAM_READWRITE);

	XfdashboardSearchBoxProperties[PROP_TEXT_COLOR]=
		clutter_param_spec_color("text-color",
									"Text color",
									"Color of text in editable text box",
									&defaultTextColor,
									G_PARAM_READWRITE | G_PARAM_CONSTRUCT);

	XfdashboardSearchBoxProperties[PROP_HINT_TEXT]=
		g_param_spec_string("hint-text",
							"Hint text",
							"Hint text shown if editable text box is empty",
							"",
							G_PARAM_READWRITE);

	XfdashboardSearchBoxProperties[PROP_HINT_TEXT_FONT]=
		g_param_spec_string("hint-text-font",
							"Hint text font",
							"Font of hint text shown if editable text box is empty",
							NULL,
							G_PARAM_READWRITE);

	XfdashboardSearchBoxProperties[PROP_HINT_TEXT_COLOR]=
		clutter_param_spec_color("hint-text-color",
									"Hint text color",
									"Color of hint text shown if editable text box is empty",
									&defaultHintTextColor,
									G_PARAM_READWRITE | G_PARAM_CONSTRUCT);

	XfdashboardSearchBoxProperties[PROP_BACKGROUND_VISIBLE]=
		g_param_spec_boolean("background-visible",
								"Background visibility",
								"Should background be shown",
								TRUE,
								G_PARAM_READWRITE);

	XfdashboardSearchBoxProperties[PROP_BACKGROUND_COLOR]=
		clutter_param_spec_color("background-color",
									"Background color",
									"Background color of icon and text",
									&defaultBackgroundColor,
									G_PARAM_READWRITE | G_PARAM_CONSTRUCT);

	g_object_class_install_properties(gobjectClass, PROP_LAST, XfdashboardSearchBoxProperties);

	/* Define signals */
	XfdashboardSearchBoxSignals[PRIMARY_ICON_CLICKED]=
		g_signal_new("primary-icon-clicked",
						G_TYPE_FROM_CLASS(klass),
						G_SIGNAL_RUN_LAST,
						G_STRUCT_OFFSET(XfdashboardSearchBoxClass, primary_icon_clicked),
						NULL,
						NULL,
						g_cclosure_marshal_VOID__VOID,
						G_TYPE_NONE,
						0);

	XfdashboardSearchBoxSignals[SECONDARY_ICON_CLICKED]=
		g_signal_new("secondary-icon-clicked",
						G_TYPE_FROM_CLASS(klass),
						G_SIGNAL_RUN_LAST,
						G_STRUCT_OFFSET(XfdashboardSearchBoxClass, secondary_icon_clicked),
						NULL,
						NULL,
						g_cclosure_marshal_VOID__VOID,
						G_TYPE_NONE,
						0);

	XfdashboardSearchBoxSignals[SEARCH_STARTED]=
		g_signal_new("search-started",
						G_TYPE_FROM_CLASS(klass),
						G_SIGNAL_RUN_LAST,
						G_STRUCT_OFFSET(XfdashboardSearchBoxClass, search_started),
						NULL,
						NULL,
						g_cclosure_marshal_VOID__VOID,
						G_TYPE_NONE,
						0);

	XfdashboardSearchBoxSignals[SEARCH_ENDED]=
		g_signal_new("search-ended",
						G_TYPE_FROM_CLASS(klass),
						G_SIGNAL_RUN_LAST,
						G_STRUCT_OFFSET(XfdashboardSearchBoxClass, search_ended),
						NULL,
						NULL,
						g_cclosure_marshal_VOID__VOID,
						G_TYPE_NONE,
						0);

	XfdashboardSearchBoxSignals[TEXT_CHANGED]=
		g_signal_new("text-changed",
						G_TYPE_FROM_CLASS(klass),
						G_SIGNAL_RUN_LAST,
						G_STRUCT_OFFSET(XfdashboardSearchBoxClass, text_changed),
						NULL,
						NULL,
						g_cclosure_marshal_VOID__STRING,
						G_TYPE_NONE,
						1,
						G_TYPE_STRING);
}

/* Object initialization
 * Create private structure and set up default values
 */
static void xfdashboard_searchbox_init(XfdashboardSearchBox *self)
{
	XfdashboardSearchBoxPrivate	*priv;

	priv=self->priv=XFDASHBOARD_SEARCHBOX_GET_PRIVATE(self);

	/* This actor is react on events */
	clutter_actor_set_reactive(CLUTTER_ACTOR(self), TRUE);

	/* Set up default values */
	priv->margin=0.0f;
	priv->spacing=0.0f;
	priv->primaryIconName=NULL;
	priv->secondaryIconName=NULL;
	priv->textFont=NULL;
	priv->textColor=NULL;
	priv->hintTextFont=NULL;
	priv->hintTextColor=NULL;
	priv->showBackground=TRUE;
	priv->backgroundColor=NULL;
	priv->lastTextLength=0;

	/* Create actors */
	priv->actorPrimaryIcon=CLUTTER_TEXTURE(clutter_texture_new());
	clutter_actor_set_parent(CLUTTER_ACTOR(priv->actorPrimaryIcon), CLUTTER_ACTOR(self));
	clutter_actor_set_reactive(CLUTTER_ACTOR(priv->actorPrimaryIcon), TRUE);

	priv->primaryIconClickAction=clutter_click_action_new();
	clutter_actor_add_action(CLUTTER_ACTOR(priv->actorPrimaryIcon), priv->primaryIconClickAction);
	g_signal_connect(priv->primaryIconClickAction, "clicked", G_CALLBACK(_xfdashboard_searchbox_on_primary_icon_clicked), self);

	priv->actorSecondaryIcon=CLUTTER_TEXTURE(clutter_texture_new());
	clutter_actor_set_parent(CLUTTER_ACTOR(priv->actorSecondaryIcon), CLUTTER_ACTOR(self));
	clutter_actor_set_reactive(CLUTTER_ACTOR(priv->actorSecondaryIcon), TRUE);

	priv->secondaryIconClickAction=clutter_click_action_new();
	clutter_actor_add_action(CLUTTER_ACTOR(priv->actorSecondaryIcon), priv->secondaryIconClickAction);
	g_signal_connect(priv->secondaryIconClickAction, "clicked", G_CALLBACK(_xfdashboard_searchbox_on_secondary_icon_clicked), self);

	priv->actorTextBox=CLUTTER_TEXT(clutter_text_new());
	clutter_actor_set_parent(CLUTTER_ACTOR(priv->actorTextBox), CLUTTER_ACTOR(self));
	clutter_actor_set_reactive(CLUTTER_ACTOR(priv->actorTextBox), TRUE);
	clutter_text_set_selectable(priv->actorTextBox, TRUE);
	clutter_text_set_editable(priv->actorTextBox, TRUE);
	clutter_text_set_single_line_mode(priv->actorTextBox, TRUE);
	g_signal_connect(priv->actorTextBox, "text-changed", G_CALLBACK(_xfdashboard_searchbox_on_text_changed), self);

	priv->actorHintLabel=CLUTTER_TEXT(clutter_text_new());
	clutter_actor_set_parent(CLUTTER_ACTOR(priv->actorHintLabel), CLUTTER_ACTOR(self));
	clutter_actor_set_reactive(CLUTTER_ACTOR(priv->actorHintLabel), FALSE);
	clutter_text_set_selectable(priv->actorHintLabel, FALSE);
	clutter_text_set_editable(priv->actorHintLabel, FALSE);
}

/* Implementation: Public API */

/* Create new actor */
ClutterActor* xfdashboard_searchbox_new()
{
	return(g_object_new(XFDASHBOARD_TYPE_SEARCHBOX, NULL));
}

/* Get/set margin of background to text and icon actors */
const gfloat xfdashboard_searchbox_get_margin(XfdashboardSearchBox *self)
{
	g_return_val_if_fail(XFDASHBOARD_IS_SEARCHBOX(self), 0);

	return(self->priv->margin);
}

void xfdashboard_searchbox_set_margin(XfdashboardSearchBox *self, const gfloat inMargin)
{
	g_return_if_fail(XFDASHBOARD_IS_SEARCHBOX(self));
	g_return_if_fail(inMargin>=0.0f);

	/* Set margin */
	XfdashboardSearchBoxPrivate	*priv=self->priv;

	if(priv->margin!=inMargin)
	{
		priv->margin=inMargin;
		clutter_actor_queue_relayout(CLUTTER_ACTOR(self));
	}
}

/* Get/set spacing between text and icon actors */
const gfloat xfdashboard_searchbox_get_spacing(XfdashboardSearchBox *self)
{
	g_return_val_if_fail(XFDASHBOARD_IS_SEARCHBOX(self), 0);

	return(self->priv->spacing);
}

void xfdashboard_searchbox_set_spacing(XfdashboardSearchBox *self, const gfloat inSpacing)
{
	g_return_if_fail(XFDASHBOARD_IS_SEARCHBOX(self));
	g_return_if_fail(inSpacing>=0.0f);

	/* Set spacing */
	XfdashboardSearchBoxPrivate	*priv=self->priv;

	if(priv->spacing!=inSpacing)
	{
		priv->spacing=inSpacing;
		clutter_actor_queue_relayout(CLUTTER_ACTOR(self));
	}
}

/* Get/set text of editable text box */
gboolean xfdashboard_searchbox_is_empty_text(XfdashboardSearchBox *self)
{
	g_return_val_if_fail(XFDASHBOARD_IS_SEARCHBOX(self), TRUE);

	const gchar			*text=clutter_text_get_text(self->priv->actorTextBox);

	return((text && strlen(text)>0) ? FALSE : TRUE);
}

const gchar* xfdashboard_searchbox_get_text(XfdashboardSearchBox *self)
{
	g_return_val_if_fail(XFDASHBOARD_IS_SEARCHBOX(self), NULL);

	return(clutter_text_get_text(self->priv->actorTextBox));
}

void xfdashboard_searchbox_set_text(XfdashboardSearchBox *self, const gchar *inMarkupText)
{
	g_return_if_fail(XFDASHBOARD_IS_SEARCHBOX(self));

	/* Set text of editable text box */
	XfdashboardSearchBoxPrivate	*priv=self->priv;
	gint						textLength;

	if(g_strcmp0(clutter_text_get_text(priv->actorTextBox), inMarkupText)!=0)
	{
		clutter_text_set_markup(priv->actorTextBox, inMarkupText);

		textLength=strlen(clutter_text_get_text(priv->actorTextBox));
		if(textLength>0) clutter_actor_hide(CLUTTER_ACTOR(priv->actorHintLabel));
			else clutter_actor_show(CLUTTER_ACTOR(priv->actorHintLabel));

		clutter_actor_queue_relayout(CLUTTER_ACTOR(self));
	}
}

/* Get/set font of editable text box */
const gchar* xfdashboard_searchbox_get_text_font(XfdashboardSearchBox *self)
{
	g_return_val_if_fail(XFDASHBOARD_IS_SEARCHBOX(self), NULL);

	if(self->priv->actorTextBox) return(self->priv->textFont);
	return(NULL);
}

void xfdashboard_searchbox_set_text_font(XfdashboardSearchBox *self, const gchar *inFont)
{
	g_return_if_fail(XFDASHBOARD_IS_SEARCHBOX(self));

	/* Set font of editable text box */
	XfdashboardSearchBoxPrivate	*priv=self->priv;

	if(g_strcmp0(priv->textFont, inFont)!=0)
	{
		if(priv->textFont) g_free(priv->textFont);
		priv->textFont=(inFont ? g_strdup(inFont) : NULL);

		clutter_text_set_font_name(priv->actorTextBox, priv->textFont);
		_xfdashboard_searchbox_update_icons(self);
	}
}

/* Get/set color of text in editable text box */
const ClutterColor* xfdashboard_searchbox_get_text_color(XfdashboardSearchBox *self)
{
	g_return_val_if_fail(XFDASHBOARD_IS_SEARCHBOX(self), NULL);

	return(self->priv->textColor);
}

void xfdashboard_searchbox_set_text_color(XfdashboardSearchBox *self, const ClutterColor *inColor)
{
	g_return_if_fail(XFDASHBOARD_IS_SEARCHBOX(self));
	g_return_if_fail(inColor);

	/* Set text color of editable text box */
	XfdashboardSearchBoxPrivate	*priv=self->priv;
	ClutterColor				selectionColor;
	
	if(!priv->textColor || !clutter_color_equal(inColor, priv->textColor))
	{
		if(priv->textColor) clutter_color_free(priv->textColor);
		priv->textColor=clutter_color_copy(inColor);

		clutter_text_set_color(priv->actorTextBox, priv->textColor);

		/* Selection text color is inverted text color */
		selectionColor.red=0xff-priv->textColor->red;
		selectionColor.green=0xff-priv->textColor->green;
		selectionColor.blue=0xff-priv->textColor->blue;
		selectionColor.alpha=priv->textColor->alpha;
		clutter_text_set_selected_text_color(priv->actorTextBox, &selectionColor);

		/* Selection color is the same as text color */
		clutter_text_set_selection_color(priv->actorTextBox, priv->textColor);

		clutter_actor_queue_redraw(CLUTTER_ACTOR(self));
	}
}

/* Get/set text of hint label */
const gchar* xfdashboard_searchbox_get_hint_text(XfdashboardSearchBox *self)
{
	g_return_val_if_fail(XFDASHBOARD_IS_SEARCHBOX(self), NULL);

	return(clutter_text_get_text(self->priv->actorHintLabel));
}

void xfdashboard_searchbox_set_hint_text(XfdashboardSearchBox *self, const gchar *inMarkupText)
{
	g_return_if_fail(XFDASHBOARD_IS_SEARCHBOX(self));
	g_return_if_fail(inMarkupText);

	/* Set text of hint label */
	XfdashboardSearchBoxPrivate	*priv=self->priv;

	if(g_strcmp0(clutter_text_get_text(priv->actorHintLabel), inMarkupText)!=0)
	{
		clutter_text_set_markup(priv->actorHintLabel, inMarkupText);
		clutter_actor_queue_relayout(CLUTTER_ACTOR(self));
	}
}

/* Get/set font of hint label */
const gchar* xfdashboard_searchbox_get_hint_text_font(XfdashboardSearchBox *self)
{
	g_return_val_if_fail(XFDASHBOARD_IS_SEARCHBOX(self), NULL);

	return(self->priv->hintTextFont);
}

void xfdashboard_searchbox_set_hint_text_font(XfdashboardSearchBox *self, const gchar *inFont)
{
	g_return_if_fail(XFDASHBOARD_IS_SEARCHBOX(self));

	/* Set font of hint label */
	XfdashboardSearchBoxPrivate	*priv=self->priv;

	if(g_strcmp0(priv->hintTextFont, inFont)!=0)
	{
		if(priv->hintTextFont) g_free(priv->hintTextFont);
		priv->hintTextFont=(inFont ? g_strdup(inFont) : NULL);

		clutter_text_set_font_name(priv->actorHintLabel, priv->hintTextFont);
		_xfdashboard_searchbox_update_icons(self);
	}
}

/* Get/set color of text in hint label */
const ClutterColor* xfdashboard_searchbox_get_hint_text_color(XfdashboardSearchBox *self)
{
	g_return_val_if_fail(XFDASHBOARD_IS_SEARCHBOX(self), NULL);

	return(self->priv->hintTextColor);
}

void xfdashboard_searchbox_set_hint_text_color(XfdashboardSearchBox *self, const ClutterColor *inColor)
{
	g_return_if_fail(XFDASHBOARD_IS_SEARCHBOX(self));
	g_return_if_fail(inColor);

	/* Set text color of hint label */
	XfdashboardSearchBoxPrivate	*priv=self->priv;

	if(!priv->hintTextColor || !clutter_color_equal(inColor, priv->hintTextColor))
	{
		if(priv->hintTextColor) clutter_color_free(priv->hintTextColor);
		priv->hintTextColor=clutter_color_copy(inColor);

		clutter_text_set_color(priv->actorHintLabel, priv->hintTextColor);
		clutter_actor_queue_redraw(CLUTTER_ACTOR(self));
	}
}

/* Get/set primary icon */
const gchar* xfdashboard_searchbox_get_primary_icon(XfdashboardSearchBox *self)
{
	g_return_val_if_fail(XFDASHBOARD_IS_SEARCHBOX(self), NULL);

	return(self->priv->primaryIconName);
}

void xfdashboard_searchbox_set_primary_icon(XfdashboardSearchBox *self, const gchar *inIconName)
{
	g_return_if_fail(XFDASHBOARD_IS_SEARCHBOX(self));
	g_return_if_fail(!inIconName || strlen(inIconName)>0);

	/* Set themed icon name or icon file name for primary icon */
	XfdashboardSearchBoxPrivate	*priv=self->priv;

	if(g_strcmp0(priv->primaryIconName, inIconName)!=0)
	{
		if(priv->primaryIconName) g_free(priv->primaryIconName);
		priv->primaryIconName=g_strdup(inIconName);

		_xfdashboard_searchbox_update_icons(self);
	}
}

/* Get/set secondary icon */
const gchar* xfdashboard_searchbox_get_secondary_icon(XfdashboardSearchBox *self)
{
	g_return_val_if_fail(XFDASHBOARD_IS_SEARCHBOX(self), NULL);

	return(self->priv->secondaryIconName);
}

void xfdashboard_searchbox_set_secondary_icon(XfdashboardSearchBox *self, const gchar *inIconName)
{
	g_return_if_fail(XFDASHBOARD_IS_SEARCHBOX(self));
	g_return_if_fail(!inIconName || strlen(inIconName)>0);

	/* Set themed icon name or icon file name for primary icon */
	XfdashboardSearchBoxPrivate	*priv=self->priv;

	if(g_strcmp0(priv->secondaryIconName, inIconName)!=0)
	{
		if(priv->secondaryIconName) g_free(priv->secondaryIconName);
		priv->secondaryIconName=g_strdup(inIconName);

		_xfdashboard_searchbox_update_icons(self);
	}
}

/* Get/set visibility of background */
gboolean xfdashboard_searchbox_get_background_visibility(XfdashboardSearchBox *self)
{
	g_return_val_if_fail(XFDASHBOARD_IS_SEARCHBOX(self), FALSE);

	return(self->priv->showBackground);
}

void xfdashboard_searchbox_set_background_visibility(XfdashboardSearchBox *self, gboolean inVisible)
{
	g_return_if_fail(XFDASHBOARD_IS_SEARCHBOX(self));

	/* Set background visibility */
	XfdashboardSearchBoxPrivate	*priv=self->priv;

	if(priv->showBackground!=inVisible)
	{
		priv->showBackground=inVisible;
		clutter_actor_queue_redraw(CLUTTER_ACTOR(self));
	}
}

/* Get/set color of background */
const ClutterColor* xfdashboard_searchbox_get_background_color(XfdashboardSearchBox *self)
{
	g_return_val_if_fail(XFDASHBOARD_IS_SEARCHBOX(self), NULL);

	return(self->priv->backgroundColor);
}

void xfdashboard_searchbox_set_background_color(XfdashboardSearchBox *self, const ClutterColor *inColor)
{
	g_return_if_fail(XFDASHBOARD_IS_SEARCHBOX(self));
	g_return_if_fail(inColor);

	/* Set background color */
	XfdashboardSearchBoxPrivate	*priv=self->priv;

	if(!priv->backgroundColor || !clutter_color_equal(inColor, priv->backgroundColor))
	{
		if(priv->backgroundColor) clutter_color_free(priv->backgroundColor);
		priv->backgroundColor=clutter_color_copy(inColor);

		clutter_actor_queue_redraw(CLUTTER_ACTOR(self));
	}
}
