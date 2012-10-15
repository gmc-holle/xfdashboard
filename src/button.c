/*
 * button.c: An actor representing a label and an icon (both optional)
 *           and can react on click actions
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

#include "button.h"
#include "enums.h"

#include <gdk/gdk.h>
#include <math.h>

/* Define this class in GObject system */

G_DEFINE_TYPE(XfdashboardButton,
				xfdashboard_button,
				CLUTTER_TYPE_ACTOR)
                                                
/* Private structure - access only by public API if needed */
#define XFDASHBOARD_BUTTON_GET_PRIVATE(obj) \
	(G_TYPE_INSTANCE_GET_PRIVATE((obj), XFDASHBOARD_TYPE_BUTTON, XfdashboardButtonPrivate))

struct _XfdashboardButtonPrivate
{
	/* Actors for icon and label of button */
	ClutterTexture			*actorIcon;
	ClutterText				*actorLabel;

	/* Actor actions */
	ClutterAction			*clickAction;

	/* Settings */
	gfloat					margin;
	gfloat					spacing;
	XfdashboardStyle		style;

	gchar					*iconName;
	GdkPixbuf				*iconPixbuf;
	gboolean				iconSyncSize;
	gint					iconSize;
	XfdashboardOrientation	iconOrientation;

	gchar					*font;
	ClutterColor			*labelColor;
	PangoEllipsizeMode		labelEllipsize;

	gboolean				showBackground;
	ClutterColor			*backgroundColor;
};

/* Properties */
enum
{
	PROP_0,

	PROP_MARGIN,
	PROP_SPACING,
	PROP_STYLE,

	PROP_ICON_NAME,
	PROP_ICON_PIXBUF,
	PROP_ICON_SYNC_SIZE,
	PROP_ICON_SIZE,
	PROP_ICON_ORIENTATION,

	PROP_TEXT,
	PROP_TEXT_FONT,
	PROP_TEXT_COLOR,
	PROP_TEXT_ELLIPSIZE_MODE,

	PROP_BACKGROUND_VISIBLE,
	PROP_BACKGROUND_COLOR,

	PROP_LAST
};

static GParamSpec* XfdashboardButtonProperties[PROP_LAST]={ 0, };

/* Signals */
enum
{
	CLICKED,

	SIGNAL_LAST
};

static guint XfdashboardButtonSignals[SIGNAL_LAST]={ 0, };

/* Private constants */
#define DEFAULT_SIZE	64

static ClutterColor		defaultTextColor={ 0xff, 0xff , 0xff, 0xff };
static ClutterColor		defaultBackgroundColor={ 0x00, 0x00, 0x00, 0xd0 };

/* IMPLEMENTATION: Private variables and methods */

/* Update icon */
void _xfdashboard_button_update_icon(XfdashboardButton *self)
{
	g_return_if_fail(XFDASHBOARD_IS_BUTTON(self));

	XfdashboardButtonPrivate	*priv=self->priv;
	gint						size;
	GdkPixbuf					*icon=NULL;
	GError						*error;

	/* Determine size of icon to load by checking if icon should be
	 * synchronized to label height or width
	 */
	if(priv->iconSyncSize)
	{
		gfloat					labelWidth, labelHeight;

		/* Get size of label */
		clutter_actor_get_preferred_size(CLUTTER_ACTOR(priv->actorLabel),
											NULL, NULL,
											&labelWidth, &labelHeight);

		/* Determine size of icon */
		/* TODO: Respect orientation -> height is orientation is left or right otherwise width */
		size=(gint)labelHeight;
	}
		else size=priv->iconSize;

	/* Get scaled icon from themed icon (if icon name is set) or use icon pixbuf set */
	if(priv->iconPixbuf)
	{
		/* If pixbuf is not of requested size scale it */
		if(gdk_pixbuf_get_width(priv->iconPixbuf)==size &&
				gdk_pixbuf_get_height(priv->iconPixbuf)==size)
		{
			icon=g_object_ref(priv->iconPixbuf);
		}
			else
			{
				icon=gdk_pixbuf_scale_simple(priv->iconPixbuf, size, size, GDK_INTERP_BILINEAR);
			}
	}
		else
		{
			icon=xfdashboard_get_pixbuf_for_icon_name_scaled(priv->iconName, size);
		}

	g_return_if_fail(icon);

	/* Update texture of actor */
	error=NULL;
	if(!clutter_texture_set_from_rgb_data(CLUTTER_TEXTURE(priv->actorIcon),
												gdk_pixbuf_get_pixels(icon),
												gdk_pixbuf_get_has_alpha(icon),
												gdk_pixbuf_get_width(icon),
												gdk_pixbuf_get_height(icon),
												gdk_pixbuf_get_rowstride(icon),
												gdk_pixbuf_get_has_alpha(icon) ? 4 : 3,
												CLUTTER_TEXTURE_NONE,
												&error))
	{
		g_warning("Could not update icon of %s: %s",
					G_OBJECT_TYPE_NAME(self),
					(error && error->message) ?  error->message : "unknown error");
		if(error!=NULL) g_error_free(error);
	}
	g_object_unref(icon);

	/* Queue a redraw as the actors are now available */
	clutter_actor_queue_redraw(CLUTTER_ACTOR(self));
}

/* IMPLEMENTATION: ClutterActor */

/* Show all children of this one */
static void xfdashboard_button_show_all(ClutterActor *self)
{
	XfdashboardButtonPrivate	*priv=XFDASHBOARD_BUTTON(self)->priv;

	if(priv->style==XFDASHBOARD_STYLE_ICON ||
		priv->style==XFDASHBOARD_STYLE_BOTH)
	{
		clutter_actor_show(CLUTTER_ACTOR(priv->actorIcon));
	}
		else clutter_actor_hide(CLUTTER_ACTOR(priv->actorIcon));

	if(priv->style==XFDASHBOARD_STYLE_TEXT ||
		priv->style==XFDASHBOARD_STYLE_BOTH)
	{
		clutter_actor_show(CLUTTER_ACTOR(priv->actorLabel));
	}
		else clutter_actor_hide(CLUTTER_ACTOR(priv->actorLabel));

	clutter_actor_show(self);
}

/* Hide all children of this one */
static void xfdashboard_button_hide_all(ClutterActor *self)
{
	XfdashboardButtonPrivate	*priv=XFDASHBOARD_BUTTON(self)->priv;

	clutter_actor_hide(self);
	clutter_actor_hide(CLUTTER_ACTOR(priv->actorIcon));
	clutter_actor_hide(CLUTTER_ACTOR(priv->actorLabel));
}

/* Get preferred width/height */
static void xfdashboard_button_get_preferred_height(ClutterActor *self,
														gfloat inForWidth,
														gfloat *outMinHeight,
														gfloat *outNaturalHeight)
{
	XfdashboardButtonPrivate	*priv=XFDASHBOARD_BUTTON(self)->priv;
	gfloat						minHeight, naturalHeight;
	gfloat						childMinHeight, childNaturalHeight;
	gfloat						spacing=priv->spacing;
	
	minHeight=naturalHeight=0.0f;

	/* Determine size of label if visible */
	if(CLUTTER_ACTOR_IS_VISIBLE(priv->actorLabel))
	{
		clutter_actor_get_preferred_height(CLUTTER_ACTOR(priv->actorLabel),
											inForWidth,
											&childMinHeight, &childNaturalHeight);

		switch(priv->iconOrientation)
		{
			case XFDASHBOARD_ORIENTATION_TOP:
			case XFDASHBOARD_ORIENTATION_BOTTOM:
				minHeight+=childMinHeight;
				naturalHeight+=childNaturalHeight;
				break;
				
			default:
				if(childMinHeight>minHeight) minHeight=childMinHeight;
				if(childNaturalHeight>naturalHeight) naturalHeight=childNaturalHeight;
				break;
		}
	}
		else spacing=0.0f;

	/* Determine size of icon if visible */
	if(CLUTTER_ACTOR_IS_VISIBLE(priv->actorIcon))
	{
		clutter_actor_get_preferred_height(CLUTTER_ACTOR(priv->actorIcon),
											inForWidth,
											&childMinHeight, &childNaturalHeight);

		switch(priv->iconOrientation)
		{
			case XFDASHBOARD_ORIENTATION_TOP:
			case XFDASHBOARD_ORIENTATION_BOTTOM:
				minHeight+=childMinHeight;
				naturalHeight+=childNaturalHeight;
				break;
				
			default:
				if(childMinHeight>minHeight) minHeight=childMinHeight;
				if(childNaturalHeight>naturalHeight) naturalHeight=childNaturalHeight;
				break;
		}
	}
		else spacing=0.0f;

	/* Add spacing to size if orientation is top or bottom.
	 * Spacing was initially set to spacing in settings but
	 * resetted to zero if either text or icon is not visible.
	 */
	if(priv->iconOrientation==XFDASHBOARD_ORIENTATION_TOP ||
		priv->iconOrientation==XFDASHBOARD_ORIENTATION_BOTTOM)
	{
		minHeight+=spacing;
		naturalHeight+=spacing;
	}

	// Add margin
	minHeight+=2*priv->margin;
	naturalHeight+=2*priv->margin;

	/* Store sizes computed */
	if(outMinHeight) *outMinHeight=minHeight;
	if(outNaturalHeight) *outNaturalHeight=naturalHeight;
}

static void xfdashboard_button_get_preferred_width(ClutterActor *self,
														gfloat inForHeight,
														gfloat *outMinWidth,
														gfloat *outNaturalWidth)
{
	XfdashboardButtonPrivate	*priv=XFDASHBOARD_BUTTON(self)->priv;
	gfloat						minWidth, naturalWidth;
	gfloat						childMinWidth, childNaturalWidth;
	gfloat						spacing=priv->spacing;

	minWidth=naturalWidth=0.0f;

	/* Determine size of label if visible */
	if(CLUTTER_ACTOR_IS_VISIBLE(priv->actorLabel))
	{
		clutter_actor_get_preferred_width(CLUTTER_ACTOR(priv->actorLabel),
											inForHeight,
											&childMinWidth, &childNaturalWidth);

		switch(priv->iconOrientation)
		{
			case XFDASHBOARD_ORIENTATION_LEFT:
			case XFDASHBOARD_ORIENTATION_RIGHT:
				minWidth+=childMinWidth;
				naturalWidth+=childNaturalWidth;
				break;
				
			default:
				if(childMinWidth>minWidth) minWidth=childMinWidth;
				if(childNaturalWidth>naturalWidth) naturalWidth=childNaturalWidth;
				break;
		}
	}
		else spacing=0.0f;
	
	/* Determine size of icon if visible */
	if(CLUTTER_ACTOR_IS_VISIBLE(priv->actorIcon))
	{
		clutter_actor_get_preferred_width(CLUTTER_ACTOR(priv->actorIcon),
											inForHeight,
											&childMinWidth, &childNaturalWidth);

		switch(priv->iconOrientation)
		{
			case XFDASHBOARD_ORIENTATION_LEFT:
			case XFDASHBOARD_ORIENTATION_RIGHT:
				minWidth+=childMinWidth;
				naturalWidth+=childNaturalWidth;
				break;
				
			default:
				if(childMinWidth>minWidth) minWidth=childMinWidth;
				if(childNaturalWidth>naturalWidth) naturalWidth=childNaturalWidth;
				break;
		}
	}
		else spacing=0.0f;

	/* Add spacing to size if orientation is top or bottom.
	 * Spacing was initially set to spacing in settings but
	 * resetted to zero if either text or icon is not visible.
	 */
	if(priv->iconOrientation==XFDASHBOARD_ORIENTATION_LEFT ||
		priv->iconOrientation==XFDASHBOARD_ORIENTATION_RIGHT)
	{
		minWidth+=spacing;
		naturalWidth+=spacing;
	}

	// Add margin
	minWidth+=2*priv->margin;
	naturalWidth+=2*priv->margin;

	/* Store sizes computed */
	if(outMinWidth) *outMinWidth=minWidth;
	if(outNaturalWidth) *outNaturalWidth=naturalWidth;
}

/* Allocate position and size of actor and its children*/
static void xfdashboard_button_allocate(ClutterActor *self,
											const ClutterActorBox *inBox,
											ClutterAllocationFlags inFlags)
{
	XfdashboardButtonPrivate	*priv=XFDASHBOARD_BUTTON(self)->priv;
	ClutterActorBox				*boxLabel=NULL;
	ClutterActorBox				*boxIcon=NULL;
	gfloat						left, right, top, bottom;
	gfloat						textWidth, textHeight;
	gfloat						iconWidth, iconHeight;
	gfloat						spacing=priv->spacing;

	/* Chain up to store the allocation of the actor */
	CLUTTER_ACTOR_CLASS(xfdashboard_button_parent_class)->allocate(self, inBox, inFlags);

	/* Get sizes of children and determine if we need
	 * to add spacing between text and icon. If either
	 * icon or text is not visible reset its size to zero
	 * and also reset spacing to zero.
	 */
	if(CLUTTER_ACTOR_IS_VISIBLE(priv->actorLabel))
	{
		clutter_actor_get_preferred_size(CLUTTER_ACTOR(priv->actorLabel),
											NULL, NULL,
											&textWidth, &textHeight);
	}
		else spacing=textWidth=textHeight=0.0f;

	if(CLUTTER_ACTOR_IS_VISIBLE(priv->actorIcon))
	{
		clutter_actor_get_preferred_size(CLUTTER_ACTOR(priv->actorIcon),
											NULL, NULL,
											&iconWidth, &iconHeight);

		iconWidth=MIN(iconWidth, clutter_actor_box_get_width(inBox)-2*priv->margin);
		iconHeight=MIN(iconHeight, clutter_actor_box_get_height(inBox)-2*priv->margin);
	}
		else spacing=iconWidth=iconHeight=0.0f;

	/* Set allocation of label if visible*/
	if(CLUTTER_ACTOR_IS_VISIBLE(priv->actorLabel))
	{
		switch(priv->iconOrientation)
		{
			case XFDASHBOARD_ORIENTATION_TOP:
				textWidth=MIN(clutter_actor_box_get_width(inBox)-(2*priv->margin), textWidth);
				
				left=((clutter_actor_box_get_width(inBox)-textWidth)/2.0f);
				right=left+textWidth;
				top=priv->margin+iconHeight+spacing;
				bottom=top+textHeight;
				break;

			case XFDASHBOARD_ORIENTATION_BOTTOM:
				textWidth=MIN(clutter_actor_box_get_width(inBox)-(2*priv->margin), textWidth);

				left=((clutter_actor_box_get_width(inBox)-textWidth)/2.0f);
				right=left+textWidth;
				top=priv->margin;
				bottom=top+textHeight;
				break;

			case XFDASHBOARD_ORIENTATION_RIGHT:
				textWidth=MIN(clutter_actor_box_get_width(inBox)-(2*priv->margin)-iconWidth-spacing, textWidth);
				
				left=priv->margin;
				right=left+textWidth;
				top=priv->margin;
				bottom=top+textHeight;
				break;

			case XFDASHBOARD_ORIENTATION_LEFT:
			default:
				textWidth=MIN(clutter_actor_box_get_width(inBox)-(2*priv->margin)-iconWidth-spacing, textWidth);
				
				left=priv->margin+iconWidth+spacing;
				right=left+textWidth;
				top=priv->margin;
				bottom=top+textHeight;
				break;
		}

		boxLabel=clutter_actor_box_new(floor(left), floor(top), floor(right), floor(bottom));
		clutter_actor_allocate(CLUTTER_ACTOR(priv->actorLabel), boxLabel, inFlags);
	}

	/* Set allocation of icon if visible*/
	if(CLUTTER_ACTOR_IS_VISIBLE(priv->actorIcon))
	{
		switch(priv->iconOrientation)
		{
			case XFDASHBOARD_ORIENTATION_TOP:
				left=((clutter_actor_box_get_width(inBox)-iconWidth)/2.0f);
				right=left+iconWidth;
				top=priv->margin;
				bottom=top+iconHeight;
				break;

			case XFDASHBOARD_ORIENTATION_BOTTOM:
				left=((clutter_actor_box_get_width(inBox)-iconWidth)/2.0f);
				right=left+iconWidth;
				top=priv->margin+textHeight+spacing;
				bottom=top+iconHeight;
				break;

			case XFDASHBOARD_ORIENTATION_RIGHT:
				left=clutter_actor_box_get_width(inBox)-priv->margin-iconWidth;
				right=clutter_actor_box_get_width(inBox)-priv->margin;
				top=priv->margin;
				bottom=top+iconHeight;
				break;

			case XFDASHBOARD_ORIENTATION_LEFT:
			default:
				left=priv->margin;
				right=left+iconWidth;
				top=priv->margin;
				bottom=top+iconHeight;
				break;
		}

		boxIcon=clutter_actor_box_new(floor(left), floor(top), floor(right), floor(bottom));
		clutter_actor_allocate(CLUTTER_ACTOR(priv->actorIcon), boxIcon, inFlags);
	}

	/* Release allocated memory */
	if(boxLabel) clutter_actor_box_free(boxLabel);
	if(boxIcon) clutter_actor_box_free(boxIcon);
}

/* Paint actor */
static void xfdashboard_button_paint(ClutterActor *self)
{
	XfdashboardButtonPrivate	*priv=XFDASHBOARD_BUTTON(self)->priv;

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
	
	if(priv->actorIcon &&
		CLUTTER_ACTOR_IS_VISIBLE(priv->actorIcon)) clutter_actor_paint(CLUTTER_ACTOR(priv->actorIcon));

	if(priv->actorLabel &&
		CLUTTER_ACTOR_IS_VISIBLE(priv->actorLabel)) clutter_actor_paint(CLUTTER_ACTOR(priv->actorLabel));
}

/* Pick this actor and possibly all the child actors.
 * That means this function should paint its silouhette as a solid shape in the
 * given color and call the paint function of its children. But never call the
 * paint function of itself especially if the paint function sets a different
 * color, e.g. by cogl_set_source_color* function family.
 */
static void xfdashboard_button_pick(ClutterActor *self, const ClutterColor *inColor)
{
	XfdashboardButtonPrivate	*priv=XFDASHBOARD_BUTTON(self)->priv;

	/* It is possible to avoid a costly paint by checking
	 * whether the actor should really be painted in pick mode
	 */
	if(!clutter_actor_should_pick_paint(self)) return;

	/* Chain up so we get a bounding box painted (if we are reactive) */
	CLUTTER_ACTOR_CLASS(xfdashboard_button_parent_class)->pick(self, inColor);

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
	
	if(priv->actorIcon &&
		CLUTTER_ACTOR_IS_VISIBLE(priv->actorIcon)) clutter_actor_paint(CLUTTER_ACTOR(priv->actorIcon));

	if(priv->actorLabel &&
		CLUTTER_ACTOR_IS_VISIBLE(priv->actorLabel)) clutter_actor_paint(CLUTTER_ACTOR(priv->actorLabel));
}

/* proxy ClickAction signals */
static void xfdashboard_button_clicked(ClutterClickAction *inAction,
											ClutterActor *self,
											gpointer inUserData)
{
	g_signal_emit(self, XfdashboardButtonSignals[CLICKED], 0);
}

/* Destroy this actor */
static void xfdashboard_button_destroy(ClutterActor *self)
{
	/* Destroy each child actor when this actor is destroyed */
	XfdashboardButtonPrivate	*priv=XFDASHBOARD_BUTTON(self)->priv;

	if(priv->actorIcon) clutter_actor_destroy(CLUTTER_ACTOR(priv->actorIcon));
	priv->actorIcon=NULL;

	if(priv->actorLabel) clutter_actor_destroy(CLUTTER_ACTOR(priv->actorLabel));
	priv->actorLabel=NULL;

	/* Call parent's class destroy method */
	if(CLUTTER_ACTOR_CLASS(xfdashboard_button_parent_class)->destroy)
	{
		CLUTTER_ACTOR_CLASS(xfdashboard_button_parent_class)->destroy(self);
	}
}

/* IMPLEMENTATION: GObject */

/* Dispose this object */
static void xfdashboard_button_dispose(GObject *inObject)
{
	/* Release our allocated variables */
	XfdashboardButtonPrivate	*priv=XFDASHBOARD_BUTTON(inObject)->priv;

	if(priv->iconName) g_free(priv->iconName);
	priv->iconName=NULL;

	if(priv->iconPixbuf) g_object_unref(priv->iconPixbuf);
	priv->iconPixbuf=NULL;
	
	if(priv->font) g_free(priv->font);
	priv->font=NULL;

	if(priv->labelColor) clutter_color_free(priv->labelColor);
	priv->labelColor=NULL;

	if(priv->backgroundColor) clutter_color_free(priv->backgroundColor);
	priv->backgroundColor=NULL;

	/* Call parent's class dispose method */
	G_OBJECT_CLASS(xfdashboard_button_parent_class)->dispose(inObject);
}

/* Set/get properties */
static void xfdashboard_button_set_property(GObject *inObject,
												guint inPropID,
												const GValue *inValue,
												GParamSpec *inSpec)
{
	XfdashboardButton			*self=XFDASHBOARD_BUTTON(inObject);
	
	switch(inPropID)
	{
		case PROP_MARGIN:
			xfdashboard_button_set_margin(self, g_value_get_float(inValue));
			break;

		case PROP_SPACING:
			xfdashboard_button_set_spacing(self, g_value_get_float(inValue));
			break;

		case PROP_STYLE:
			xfdashboard_button_set_style(self, g_value_get_enum(inValue));
			break;

		case PROP_ICON_NAME:
			xfdashboard_button_set_icon(self, g_value_get_string(inValue));
			break;

		case PROP_ICON_PIXBUF:
			xfdashboard_button_set_icon_pixbuf(self, GDK_PIXBUF(g_value_get_object(inValue)));
			break;

		case PROP_ICON_SYNC_SIZE:
			xfdashboard_button_set_sync_icon_size(self, g_value_get_boolean(inValue));
			break;

		case PROP_ICON_SIZE:
			xfdashboard_button_set_icon_size(self, g_value_get_uint(inValue));
			break;

		case PROP_ICON_ORIENTATION:
			xfdashboard_button_set_icon_orientation(self, g_value_get_enum(inValue));
			break;

		case PROP_TEXT:
			xfdashboard_button_set_text(self, g_value_get_string(inValue));
			break;

		case PROP_TEXT_FONT:
			xfdashboard_button_set_font(self, g_value_get_string(inValue));
			break;

		case PROP_TEXT_COLOR:
			xfdashboard_button_set_color(self, clutter_value_get_color(inValue));
			break;

		case PROP_TEXT_ELLIPSIZE_MODE:
			xfdashboard_button_set_ellipsize_mode(self, g_value_get_enum(inValue));
			break;

		case PROP_BACKGROUND_VISIBLE:
			xfdashboard_button_set_background_visibility(self, g_value_get_boolean(inValue));
			break;

		case PROP_BACKGROUND_COLOR:
			xfdashboard_button_set_background_color(self, clutter_value_get_color(inValue));
			break;

		default:
			G_OBJECT_WARN_INVALID_PROPERTY_ID(inObject, inPropID, inSpec);
			break;
	}
}

static void xfdashboard_button_get_property(GObject *inObject,
												guint inPropID,
												GValue *outValue,
												GParamSpec *inSpec)
{
	XfdashboardButton			*self=XFDASHBOARD_BUTTON(inObject);
	XfdashboardButtonPrivate	*priv=self->priv;

	switch(inPropID)
	{
		case PROP_MARGIN:
			g_value_set_float(outValue, priv->margin);
			break;

		case PROP_SPACING:
			g_value_set_float(outValue, priv->spacing);
			break;

		case PROP_STYLE:
			g_value_set_enum(outValue, priv->style);
			break;

		case PROP_ICON_NAME:
			g_value_set_string(outValue, priv->iconName);
			break;

		case PROP_ICON_PIXBUF:
			g_value_set_object(outValue, priv->iconPixbuf);
			break;

		case PROP_ICON_SYNC_SIZE:
			g_value_set_boolean(outValue, priv->iconSyncSize);
			break;

		case PROP_ICON_SIZE:
			g_value_set_uint(outValue, priv->iconSize);
			break;

		case PROP_ICON_ORIENTATION:
			g_value_set_enum(outValue, priv->iconOrientation);
			break;

		case PROP_TEXT:
			g_value_set_string(outValue, clutter_text_get_text(priv->actorLabel));
			break;

		case PROP_TEXT_FONT:
			g_value_set_string(outValue, priv->font);
			break;

		case PROP_TEXT_COLOR:
			clutter_value_set_color(outValue, priv->labelColor);
			break;

		case PROP_TEXT_ELLIPSIZE_MODE:
			g_value_set_enum(outValue, priv->labelEllipsize);
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
static void xfdashboard_button_class_init(XfdashboardButtonClass *klass)
{
	ClutterActorClass	*actorClass=CLUTTER_ACTOR_CLASS(klass);
	GObjectClass		*gobjectClass=G_OBJECT_CLASS(klass);

	/* Override functions */
	gobjectClass->dispose=xfdashboard_button_dispose;
	gobjectClass->set_property=xfdashboard_button_set_property;
	gobjectClass->get_property=xfdashboard_button_get_property;

	actorClass->show_all=xfdashboard_button_show_all;
	actorClass->hide_all=xfdashboard_button_hide_all;
	actorClass->paint=xfdashboard_button_paint;
	actorClass->pick=xfdashboard_button_pick;
	actorClass->get_preferred_width=xfdashboard_button_get_preferred_width;
	actorClass->get_preferred_height=xfdashboard_button_get_preferred_height;
	actorClass->allocate=xfdashboard_button_allocate;
	actorClass->destroy=xfdashboard_button_destroy;

	/* Set up private structure */
	g_type_class_add_private(klass, sizeof(XfdashboardButtonPrivate));

	/* Define properties */
	XfdashboardButtonProperties[PROP_MARGIN]=
		g_param_spec_float("margin",
							"Margin",
							"Margin between background and elements",
							0.0f, G_MAXFLOAT,
							4.0f,
							G_PARAM_READWRITE | G_PARAM_CONSTRUCT);

	XfdashboardButtonProperties[PROP_SPACING]=
		g_param_spec_float("spacing",
							"Spacing",
							"Spacing between text and icon",
							0.0f, G_MAXFLOAT,
							4.0f,
							G_PARAM_READWRITE | G_PARAM_CONSTRUCT);

	XfdashboardButtonProperties[PROP_STYLE]=
		g_param_spec_enum("style",
							"Style",
							"Style of button showing text and/or icon",
							XFDASHBOARD_TYPE_STYLE,
							XFDASHBOARD_STYLE_TEXT,
							G_PARAM_READWRITE | G_PARAM_CONSTRUCT);

	XfdashboardButtonProperties[PROP_ICON_NAME]=
		g_param_spec_string("icon-name",
							"Icon name",
							"Themed icon name or file name of icon",
							"",
							G_PARAM_READWRITE);

	XfdashboardButtonProperties[PROP_ICON_PIXBUF]=
		g_param_spec_object("icon-pixbuf",
							"Icon Pixbuf",
							"Pixbuf of icon",
							GDK_TYPE_PIXBUF,
							G_PARAM_READWRITE);

	XfdashboardButtonProperties[PROP_ICON_SYNC_SIZE]=
		g_param_spec_boolean("sync-icon-size",
								"Synchronize icon size",
								"Synchronize icon size with text height or width depending on orientation",
								TRUE,
								G_PARAM_READWRITE);

	XfdashboardButtonProperties[PROP_ICON_SIZE]=
		g_param_spec_uint("icon-size",
							"Icon size",
							"Size of icon if size of icon is not synchronized",
							1, G_MAXUINT,
							DEFAULT_SIZE,
							G_PARAM_READWRITE);

	XfdashboardButtonProperties[PROP_ICON_ORIENTATION]=
		g_param_spec_enum("icon-orientation",
							"Icon orientation",
							"Orientation of icon to label",
							XFDASHBOARD_TYPE_ORIENTATION,
							XFDASHBOARD_ORIENTATION_LEFT,
							G_PARAM_READWRITE | G_PARAM_CONSTRUCT);

	XfdashboardButtonProperties[PROP_TEXT]=
		g_param_spec_string("text",
							"Label text",
							"Text of label",
							"",
							G_PARAM_READWRITE);

	XfdashboardButtonProperties[PROP_TEXT_FONT]=
		g_param_spec_string("font",
							"Font",
							"Font of label",
							NULL,
							G_PARAM_READWRITE);

	XfdashboardButtonProperties[PROP_TEXT_COLOR]=
		clutter_param_spec_color("color",
									"Color",
									"Color of label",
									&defaultTextColor,
									G_PARAM_READWRITE | G_PARAM_CONSTRUCT);

	XfdashboardButtonProperties[PROP_TEXT_ELLIPSIZE_MODE]=
		g_param_spec_enum("ellipsize-mode",
							"Ellipsize mode",
							"Mode of ellipsize if text in label is too long",
							PANGO_TYPE_ELLIPSIZE_MODE,
							PANGO_ELLIPSIZE_MIDDLE,
							G_PARAM_READWRITE | G_PARAM_CONSTRUCT);

	XfdashboardButtonProperties[PROP_BACKGROUND_VISIBLE]=
		g_param_spec_boolean("background-visible",
								"Background visibility",
								"Should background be shown",
								TRUE,
								G_PARAM_READWRITE);

	XfdashboardButtonProperties[PROP_BACKGROUND_COLOR]=
		clutter_param_spec_color("background-color",
									"Background color",
									"Background color of icon and text",
									&defaultBackgroundColor,
									G_PARAM_READWRITE | G_PARAM_CONSTRUCT);

	g_object_class_install_properties(gobjectClass, PROP_LAST, XfdashboardButtonProperties);

	/* Define signals */
	XfdashboardButtonSignals[CLICKED]=
		g_signal_new("clicked",
						G_TYPE_FROM_CLASS(klass),
						G_SIGNAL_RUN_LAST,
						G_STRUCT_OFFSET(XfdashboardButtonClass, clicked),
						NULL,
						NULL,
						g_cclosure_marshal_VOID__VOID,
						G_TYPE_NONE,
						0);
}

/* Object initialization
 * Create private structure and set up default values
 */
static void xfdashboard_button_init(XfdashboardButton *self)
{
	XfdashboardButtonPrivate	*priv;

	priv=self->priv=XFDASHBOARD_BUTTON_GET_PRIVATE(self);

	/* This actor is react on events */
	clutter_actor_set_reactive(CLUTTER_ACTOR(self), TRUE);

	/* Set up default values */
	priv->margin=0.0f;
	priv->spacing=0.0f;
	priv->style=-1;
	priv->iconName=NULL;
	priv->iconPixbuf=NULL;
	priv->iconSyncSize=TRUE;
	priv->iconSize=DEFAULT_SIZE;
	priv->iconOrientation=-1;
	priv->font=NULL;
	priv->labelColor=NULL;
	priv->labelEllipsize=-1;
	priv->showBackground=TRUE;
	priv->backgroundColor=NULL;

	/* Create actors */
	priv->actorIcon=CLUTTER_TEXTURE(clutter_texture_new());
	clutter_actor_set_parent(CLUTTER_ACTOR(priv->actorIcon), CLUTTER_ACTOR(self));
	clutter_actor_set_reactive(CLUTTER_ACTOR(priv->actorIcon), FALSE);

	priv->actorLabel=CLUTTER_TEXT(clutter_text_new());
	clutter_actor_set_parent(CLUTTER_ACTOR(priv->actorLabel), CLUTTER_ACTOR(self));
	clutter_actor_set_reactive(CLUTTER_ACTOR(priv->actorLabel), FALSE);
	clutter_text_set_selectable(priv->actorLabel, FALSE);

	/* Connect signals */
	priv->clickAction=clutter_click_action_new();
	clutter_actor_add_action(CLUTTER_ACTOR(self), priv->clickAction);
	g_signal_connect(priv->clickAction, "clicked", G_CALLBACK(xfdashboard_button_clicked), NULL);
}

/* Implementation: Public API */

/* Create new actor */
ClutterActor* xfdashboard_button_new_with_text(const gchar *inText)
{
	return(g_object_new(XFDASHBOARD_TYPE_BUTTON,
						"text", inText,
						"style", XFDASHBOARD_STYLE_TEXT,
						NULL));
}

ClutterActor* xfdashboard_button_new_with_icon(const gchar *inIconName)
{
	return(g_object_new(XFDASHBOARD_TYPE_BUTTON,
						"icon-name", inIconName,
						"style", XFDASHBOARD_STYLE_ICON,
						NULL));
}

ClutterActor* xfdashboard_button_new_full(const gchar *inIconName, const gchar *inText)
{
	return(g_object_new(XFDASHBOARD_TYPE_BUTTON,
						"text", inText,
						"icon-name", inIconName,
						"style", XFDASHBOARD_STYLE_BOTH,
						NULL));
}

/* Get/set margin of background to text and icon actors */
const gfloat xfdashboard_button_get_margin(XfdashboardButton *self)
{
	g_return_val_if_fail(XFDASHBOARD_IS_BUTTON(self), 0);

	return(self->priv->margin);
}

void xfdashboard_button_set_margin(XfdashboardButton *self, const gfloat inMargin)
{
	g_return_if_fail(XFDASHBOARD_IS_BUTTON(self));
	g_return_if_fail(inMargin>=0.0f);

	/* Set margin */
	XfdashboardButtonPrivate	*priv=self->priv;

	if(priv->margin!=inMargin)
	{
		priv->margin=inMargin;
		clutter_actor_queue_relayout(CLUTTER_ACTOR(self));
	}
}

/* Get/set spacing between text and icon actors */
const gfloat xfdashboard_button_get_spacing(XfdashboardButton *self)
{
	g_return_val_if_fail(XFDASHBOARD_IS_BUTTON(self), 0);

	return(self->priv->spacing);
}

void xfdashboard_button_set_spacing(XfdashboardButton *self, const gfloat inSpacing)
{
	g_return_if_fail(XFDASHBOARD_IS_BUTTON(self));
	g_return_if_fail(inSpacing>=0.0f);

	/* Set spacing */
	XfdashboardButtonPrivate	*priv=self->priv;

	if(priv->spacing!=inSpacing)
	{
		priv->spacing=inSpacing;
		clutter_actor_queue_relayout(CLUTTER_ACTOR(self));
	}
}

/* Get/set style of button */
XfdashboardStyle xfdashboard_button_get_style(XfdashboardButton *self)
{
	g_return_val_if_fail(XFDASHBOARD_IS_BUTTON(self), XFDASHBOARD_STYLE_TEXT);

	return(self->priv->style);
}

void xfdashboard_button_set_style(XfdashboardButton *self, const XfdashboardStyle inStyle)
{
	g_return_if_fail(XFDASHBOARD_IS_BUTTON(self));

	/* Set style */
	XfdashboardButtonPrivate	*priv=self->priv;

	if(priv->style!=inStyle)
	{
		priv->style=inStyle;

		/* Show actors depending on style */
		if(priv->style==XFDASHBOARD_STYLE_TEXT ||
			priv->style==XFDASHBOARD_STYLE_BOTH)
		{
			clutter_actor_show(CLUTTER_ACTOR(priv->actorLabel));
		}
			else clutter_actor_hide(CLUTTER_ACTOR(priv->actorLabel));

		if(priv->style==XFDASHBOARD_STYLE_ICON ||
			priv->style==XFDASHBOARD_STYLE_BOTH)
		{
			clutter_actor_show(CLUTTER_ACTOR(priv->actorIcon));
		}
			else clutter_actor_hide(CLUTTER_ACTOR(priv->actorIcon));

		clutter_actor_queue_relayout(CLUTTER_ACTOR(self));
	}
}

/* Get/set icon */
const gchar* xfdashboard_button_get_icon(XfdashboardButton *self)
{
	g_return_val_if_fail(XFDASHBOARD_IS_BUTTON(self), NULL);

	return(self->priv->iconName);
}

void xfdashboard_button_set_icon(XfdashboardButton *self, const gchar *inIconName)
{
	g_return_if_fail(XFDASHBOARD_IS_BUTTON(self));
	g_return_if_fail(inIconName);

	/* Set themed icon name or icon file name */
	XfdashboardButtonPrivate	*priv=self->priv;

	if(priv->iconPixbuf || g_strcmp0(priv->iconName, inIconName)!=0)
	{
		if(priv->iconName) g_free(priv->iconName);
		priv->iconName=g_strdup(inIconName);

		if(priv->iconPixbuf) g_object_unref(priv->iconPixbuf);
		priv->iconPixbuf=NULL;

		_xfdashboard_button_update_icon(self);
	}
}

GdkPixbuf* xfdashboard_button_get_icon_pixbuf(XfdashboardButton *self)
{
	g_return_val_if_fail(XFDASHBOARD_IS_BUTTON(self), NULL);

	return(self->priv->iconPixbuf);
}

void xfdashboard_button_set_icon_pixbuf(XfdashboardButton *self, GdkPixbuf *inIcon)
{
	g_return_if_fail(XFDASHBOARD_IS_BUTTON(self));
	g_return_if_fail(GDK_IS_PIXBUF(inIcon));

	/* Set themed icon name or icon file name */
	XfdashboardButtonPrivate	*priv=self->priv;

	if(priv->iconName || inIcon!=priv->iconPixbuf)
	{
		if(priv->iconName) g_free(priv->iconName);
		priv->iconName=NULL;

		if(priv->iconPixbuf) g_object_unref(priv->iconPixbuf);
		priv->iconPixbuf=g_object_ref(inIcon);

		_xfdashboard_button_update_icon(self);
	}
}

/* Get/set size of icon */
gint xfdashboard_button_get_icon_size(XfdashboardButton *self)
{
	g_return_val_if_fail(XFDASHBOARD_IS_BUTTON(self), 0);

	return(self->priv->iconSize);
}

void xfdashboard_button_set_icon_size(XfdashboardButton *self, gint inSize)
{
	g_return_if_fail(XFDASHBOARD_IS_BUTTON(self));
	g_return_if_fail(inSize>0);

	/* Set size of icon */
	XfdashboardButtonPrivate	*priv=self->priv;

	if(priv->iconSize!=inSize)
	{
		priv->iconSize=inSize;

		_xfdashboard_button_update_icon(self);
	}
}

/* Get/set state if icon size will be synchronized */
gboolean xfdashboard_button_get_sync_icon_size(XfdashboardButton *self)
{
	g_return_val_if_fail(XFDASHBOARD_IS_BUTTON(self), FALSE);

	return(self->priv->iconSyncSize);
}

void xfdashboard_button_set_sync_icon_size(XfdashboardButton *self, gboolean inSync)
{
	g_return_if_fail(XFDASHBOARD_IS_BUTTON(self));

	/* Set if icon size will be synchronized */
	XfdashboardButtonPrivate	*priv=self->priv;

	if(priv->iconSyncSize!=inSync)
	{
		priv->iconSyncSize=inSync;

		_xfdashboard_button_update_icon(self);
	}
}

/* Get/set orientation of icon to label */
XfdashboardOrientation xfdashboard_button_get_icon_orientation(XfdashboardButton *self)
{
	g_return_val_if_fail(XFDASHBOARD_IS_BUTTON(self), XFDASHBOARD_ORIENTATION_LEFT);

	return(self->priv->iconOrientation);
}

void xfdashboard_button_set_icon_orientation(XfdashboardButton *self, const XfdashboardOrientation inOrientation)
{
	g_return_if_fail(XFDASHBOARD_IS_BUTTON(self));

	/* Set orientation of icon */
	XfdashboardButtonPrivate	*priv=self->priv;

	if(priv->iconOrientation!=inOrientation)
	{
		priv->iconOrientation=inOrientation;

		clutter_actor_queue_relayout(CLUTTER_ACTOR(self));
	}
}

/* Get/set text of label */
const gchar* xfdashboard_button_get_text(XfdashboardButton *self)
{
	g_return_val_if_fail(XFDASHBOARD_IS_BUTTON(self), NULL);

	return(clutter_text_get_text(self->priv->actorLabel));
}

void xfdashboard_button_set_text(XfdashboardButton *self, const gchar *inMarkupText)
{
	g_return_if_fail(XFDASHBOARD_IS_BUTTON(self));
	g_return_if_fail(inMarkupText);

	/* Set text of label */
	XfdashboardButtonPrivate	*priv=self->priv;

	if(g_strcmp0(clutter_text_get_text(priv->actorLabel), inMarkupText)!=0)
	{
		clutter_text_set_markup(priv->actorLabel, inMarkupText);
		clutter_actor_queue_relayout(CLUTTER_ACTOR(priv->actorLabel));
	}
}

/* Get/set font of label */
const gchar* xfdashboard_button_get_font(XfdashboardButton *self)
{
	g_return_val_if_fail(XFDASHBOARD_IS_BUTTON(self), NULL);

	if(self->priv->actorLabel) return(self->priv->font);
	return(NULL);
}

void xfdashboard_button_set_font(XfdashboardButton *self, const gchar *inFont)
{
	g_return_if_fail(XFDASHBOARD_IS_BUTTON(self));

	/* Set font of label */
	XfdashboardButtonPrivate	*priv=self->priv;

	if(g_strcmp0(priv->font, inFont)!=0)
	{
		if(priv->font) g_free(priv->font);
		priv->font=(inFont ? g_strdup(inFont) : NULL);

		clutter_text_set_font_name(priv->actorLabel, priv->font);
		clutter_actor_queue_redraw(CLUTTER_ACTOR(self));
	}
}

/* Get/set color of text in label */
const ClutterColor* xfdashboard_button_get_color(XfdashboardButton *self)
{
	g_return_val_if_fail(XFDASHBOARD_IS_BUTTON(self), NULL);

	return(self->priv->labelColor);
}

void xfdashboard_button_set_color(XfdashboardButton *self, const ClutterColor *inColor)
{
	g_return_if_fail(XFDASHBOARD_IS_BUTTON(self));
	g_return_if_fail(inColor);

	/* Set text color of label */
	XfdashboardButtonPrivate	*priv=self->priv;

	if(!priv->labelColor || !clutter_color_equal(inColor, priv->labelColor))
	{
		if(priv->labelColor) clutter_color_free(priv->labelColor);
		priv->labelColor=clutter_color_copy(inColor);

		clutter_text_set_color(priv->actorLabel, priv->labelColor);
		clutter_actor_queue_redraw(CLUTTER_ACTOR(self));
	}
}

/* Get/set ellipsize mode if label's text is getting too long */
const PangoEllipsizeMode xfdashboard_button_get_ellipsize_mode(XfdashboardButton *self)
{
	g_return_val_if_fail(XFDASHBOARD_IS_BUTTON(self), 0);

	return(self->priv->labelEllipsize);
}

void xfdashboard_button_set_ellipsize_mode(XfdashboardButton *self, const PangoEllipsizeMode inMode)
{
	g_return_if_fail(XFDASHBOARD_IS_BUTTON(self));

	/* Set ellipsize mode */
	XfdashboardButtonPrivate	*priv=self->priv;

	if(priv->labelEllipsize!=inMode)
	{
		priv->labelEllipsize=inMode;

		clutter_text_set_ellipsize(priv->actorLabel, priv->labelEllipsize);
		clutter_actor_queue_relayout(CLUTTER_ACTOR(self));
	}
}

/* Get/set visibility of background */
gboolean xfdashboard_button_get_background_visibility(XfdashboardButton *self)
{
	g_return_val_if_fail(XFDASHBOARD_IS_BUTTON(self), FALSE);

	return(self->priv->showBackground);
}

void xfdashboard_button_set_background_visibility(XfdashboardButton *self, gboolean inVisible)
{
	g_return_if_fail(XFDASHBOARD_IS_BUTTON(self));

	/* Set background visibility */
	XfdashboardButtonPrivate	*priv=self->priv;

	if(priv->showBackground!=inVisible)
	{
		priv->showBackground=inVisible;
		clutter_actor_queue_redraw(CLUTTER_ACTOR(self));
	}
}

/* Get/set color of background */
const ClutterColor* xfdashboard_button_get_background_color(XfdashboardButton *self)
{
	g_return_val_if_fail(XFDASHBOARD_IS_BUTTON(self), NULL);

	return(self->priv->backgroundColor);
}

void xfdashboard_button_set_background_color(XfdashboardButton *self, const ClutterColor *inColor)
{
	g_return_if_fail(XFDASHBOARD_IS_BUTTON(self));
	g_return_if_fail(inColor);

	/* Set background color */
	XfdashboardButtonPrivate	*priv=self->priv;

	if(!priv->backgroundColor || !clutter_color_equal(inColor, priv->backgroundColor))
	{
		if(priv->backgroundColor) clutter_color_free(priv->backgroundColor);
		priv->backgroundColor=clutter_color_copy(inColor);

		clutter_actor_queue_redraw(CLUTTER_ACTOR(self));
	}
}
