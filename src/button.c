/*
 * button: An actor representing a label and an icon (both optional)
 *         and can react on click actions
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

#include "button.h"
#include "enums.h"

#include <glib/gi18n-lib.h>
#include <gdk/gdk.h>
#include <math.h>

/* Define this class in GObject system */
G_DEFINE_TYPE(XfdashboardButton,
				xfdashboard_button,
				XFDASHBOARD_TYPE_BACKGROUND)

/* Private structure - access only by public API if needed */
#define XFDASHBOARD_BUTTON_GET_PRIVATE(obj) \
	(G_TYPE_INSTANCE_GET_PRIVATE((obj), XFDASHBOARD_TYPE_BUTTON, XfdashboardButtonPrivate))

struct _XfdashboardButtonPrivate
{
	/* Actors for icon and label of button */
	ClutterActor			*actorIcon;
	ClutterText				*actorLabel;

	/* Actor actions */
	ClutterAction			*clickAction;

	/* Settings */
	gfloat					margin;
	gfloat					spacing;
	XfdashboardStyle		style;

	gchar					*iconName;
	ClutterImage			*iconImage;
	gboolean				iconSyncSize;
	gint					iconSize;
	XfdashboardOrientation	iconOrientation;

	gchar					*font;
	ClutterColor			*labelColor;
	PangoEllipsizeMode		labelEllipsize;
};

/* Properties */
enum
{
	PROP_0,

	PROP_MARGIN,
	PROP_SPACING,
	PROP_STYLE,

	PROP_ICON_NAME,
	PROP_ICON_IMAGE,
	PROP_ICON_SYNC_SIZE,
	PROP_ICON_SIZE,
	PROP_ICON_ORIENTATION,

	PROP_TEXT,
	PROP_TEXT_FONT,
	PROP_TEXT_COLOR,
	PROP_TEXT_ELLIPSIZE_MODE,

	PROP_LAST
};

GParamSpec* XfdashboardButtonProperties[PROP_LAST]={ 0, };

/* Signals */
enum
{
	SIGNAL_CLICKED,

	SIGNAL_LAST
};

guint XfdashboardButtonSignals[SIGNAL_LAST]={ 0, };

/* Private constants */
#define DEFAULT_SIZE	64													// TODO: Replace by settings/theming object

ClutterColor			defaultTextColor={ 0xff, 0xff , 0xff, 0xff };		// TODO: Replace by settings/theming object
ClutterColor			defaultBackgroundColor={ 0x00, 0x00, 0x00, 0xd0 };	// TODO: Replace by settings/theming object

/* IMPLEMENTATION: Private variables and methods */

/* Update icon */
void _xfdashboard_button_update_icon_image_size(XfdashboardButton *self)
{
	g_return_if_fail(XFDASHBOARD_IS_BUTTON(self));

	XfdashboardButtonPrivate	*priv=self->priv;
	gfloat						iconWidth=-1.0f, iconHeight=-1.0f;
	gfloat						maxSize=0.0f;

	/* Determine maximum size of icon either from label size if icon size
	 * should be synchronized or to icon size set if greater than zero.
	 * Otherwise the default size of icon will be set
	 */
	if(priv->iconSyncSize)
	{
		gfloat					labelWidth, labelHeight;

		/* Get size of label */
		clutter_actor_get_preferred_size(CLUTTER_ACTOR(priv->actorLabel),
											NULL, NULL,
											&labelWidth, &labelHeight);

		if(labelWidth>labelHeight) maxSize=labelWidth;
			else maxSize=labelHeight;
	}
		else if(priv->iconSize>0.0f) maxSize=priv->iconSize;

	/* Get size of icon if maximum size is set*/
	if(maxSize>0.0f)
	{
		/* Get preferred size of icon */
		clutter_content_get_preferred_size(clutter_actor_get_content(priv->actorIcon),
											&iconWidth, &iconHeight);

		/* Determine size of icon */
		if(iconWidth>iconHeight)
		{
			iconHeight=maxSize*(iconHeight/iconWidth);
			iconWidth=maxSize;
		}
			else
			{
				iconWidth=maxSize*(iconWidth/iconHeight);
				iconHeight=maxSize;
			}
	}

	/* Update size of icon actor */
	clutter_actor_set_size(priv->actorIcon, iconWidth, iconHeight);

	/* Queue a redraw as the actors are now available */
	clutter_actor_queue_redraw(CLUTTER_ACTOR(self));
}

/* IMPLEMENTATION: ClutterActor */

/* Show all children of this actor */
void _xfdashboard_button_show_all(ClutterActor *self)
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

/* Hide all children of this actor */
void _xfdashboard_button_hide_all(ClutterActor *self)
{
	XfdashboardButtonPrivate	*priv=XFDASHBOARD_BUTTON(self)->priv;

	clutter_actor_hide(self);
	clutter_actor_hide(CLUTTER_ACTOR(priv->actorIcon));
	clutter_actor_hide(CLUTTER_ACTOR(priv->actorLabel));
}

/* Get preferred width/height */
void _xfdashboard_button_get_preferred_height(ClutterActor *self,
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

void _xfdashboard_button_get_preferred_width(ClutterActor *self,
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
void _xfdashboard_button_allocate(ClutterActor *self,
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
				if(textWidth<0.0f) textWidth=0.0f;

				left=((clutter_actor_box_get_width(inBox)-textWidth)/2.0f);
				right=left+textWidth;
				top=priv->margin+iconHeight+spacing;
				bottom=top+textHeight;
				break;

			case XFDASHBOARD_ORIENTATION_BOTTOM:
				textWidth=MIN(clutter_actor_box_get_width(inBox)-(2*priv->margin), textWidth);
				if(textWidth<0.0f) textWidth=0.0f;

				left=((clutter_actor_box_get_width(inBox)-textWidth)/2.0f);
				right=left+textWidth;
				top=priv->margin;
				bottom=top+textHeight;
				break;

			case XFDASHBOARD_ORIENTATION_RIGHT:
				textWidth=MIN(clutter_actor_box_get_width(inBox)-(2*priv->margin)-iconWidth-spacing, textWidth);
				if(textWidth<0.0f) textWidth=0.0f;

				left=priv->margin;
				right=left+textWidth;
				top=priv->margin;
				bottom=top+textHeight;
				break;

			case XFDASHBOARD_ORIENTATION_LEFT:
			default:
				textWidth=MIN(clutter_actor_box_get_width(inBox)-(2*priv->margin)-iconWidth-spacing, textWidth);
				if(textWidth<0.0f) textWidth=0.0f;

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

/* proxy ClickAction signals */
void xfdashboard_button_clicked(ClutterClickAction *inAction,
											ClutterActor *self,
											gpointer inUserData)
{
	g_signal_emit(self, XfdashboardButtonSignals[SIGNAL_CLICKED], 0);
}

/* Destroy this actor */
void _xfdashboard_button_destroy(ClutterActor *self)
{
	/* Destroy each child actor when this actor is destroyed */
	XfdashboardButtonPrivate	*priv=XFDASHBOARD_BUTTON(self)->priv;

	if(priv->actorIcon)
	{
		clutter_actor_destroy(CLUTTER_ACTOR(priv->actorIcon));
		priv->actorIcon=NULL;
	}

	if(priv->actorLabel)
	{
		clutter_actor_destroy(CLUTTER_ACTOR(priv->actorLabel));
		priv->actorLabel=NULL;
	}

	/* Call parent's class destroy method */
	CLUTTER_ACTOR_CLASS(xfdashboard_button_parent_class)->destroy(self);
}

/* IMPLEMENTATION: GObject */

/* Dispose this object */
void _xfdashboard_button_dispose(GObject *inObject)
{
	/* Release our allocated variables */
	XfdashboardButton			*self=XFDASHBOARD_BUTTON(inObject);
	XfdashboardButtonPrivate	*priv=self->priv;

	if(priv->iconName)
	{
		g_free(priv->iconName);
		priv->iconName=NULL;
	}

	if(priv->iconImage)
	{
		g_object_unref(priv->iconImage);
		priv->iconImage=NULL;
	}

	if(priv->font)
	{
		g_free(priv->font);
		priv->font=NULL;
	}

	if(priv->labelColor)
	{
		clutter_color_free(priv->labelColor);
		priv->labelColor=NULL;
	}

	/* Call parent's class dispose method */
	G_OBJECT_CLASS(xfdashboard_button_parent_class)->dispose(inObject);
}

/* Set/get properties */
void _xfdashboard_button_set_property(GObject *inObject,
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

		case PROP_ICON_IMAGE:
			xfdashboard_button_set_icon_image(self, g_value_get_object(inValue));
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

		default:
			G_OBJECT_WARN_INVALID_PROPERTY_ID(inObject, inPropID, inSpec);
			break;
	}
}

void _xfdashboard_button_get_property(GObject *inObject,
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

		case PROP_ICON_IMAGE:
			g_value_set_object(outValue, priv->iconImage);
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

		default:
			G_OBJECT_WARN_INVALID_PROPERTY_ID(inObject, inPropID, inSpec);
			break;
	}
}

/* Class initialization
 * Override functions in parent classes and define properties
 * and signals
 */
void xfdashboard_button_class_init(XfdashboardButtonClass *klass)
{
	ClutterActorClass	*actorClass=CLUTTER_ACTOR_CLASS(klass);
	GObjectClass		*gobjectClass=G_OBJECT_CLASS(klass);

	/* Override functions */
	gobjectClass->dispose=_xfdashboard_button_dispose;
	gobjectClass->set_property=_xfdashboard_button_set_property;
	gobjectClass->get_property=_xfdashboard_button_get_property;

	actorClass->show_all=_xfdashboard_button_show_all;
	actorClass->hide_all=_xfdashboard_button_hide_all;
	actorClass->get_preferred_width=_xfdashboard_button_get_preferred_width;
	actorClass->get_preferred_height=_xfdashboard_button_get_preferred_height;
	actorClass->allocate=_xfdashboard_button_allocate;
	actorClass->destroy=_xfdashboard_button_destroy;

	/* Set up private structure */
	g_type_class_add_private(klass, sizeof(XfdashboardButtonPrivate));

	/* Define properties */
	XfdashboardButtonProperties[PROP_MARGIN]=
		g_param_spec_float("margin",
							_("Margin"),
							_("Margin between background and elements"),
							0.0f, G_MAXFLOAT,
							4.0f,
							G_PARAM_READWRITE | G_PARAM_CONSTRUCT);

	XfdashboardButtonProperties[PROP_SPACING]=
		g_param_spec_float("spacing",
							_("Spacing"),
							_("Spacing between text and icon"),
							0.0f, G_MAXFLOAT,
							4.0f,
							G_PARAM_READWRITE | G_PARAM_CONSTRUCT);

	XfdashboardButtonProperties[PROP_STYLE]=
		g_param_spec_enum("style",
							_("Style"),
							_("Style of button showing text and/or icon"),
							XFDASHBOARD_TYPE_STYLE,
							XFDASHBOARD_STYLE_TEXT,
							G_PARAM_READWRITE | G_PARAM_CONSTRUCT);

	XfdashboardButtonProperties[PROP_ICON_NAME]=
		g_param_spec_string("icon-name",
							_("Icon name"),
							_("Themed icon name or file name of icon"),
							N_(""),
							G_PARAM_READWRITE);

	XfdashboardButtonProperties[PROP_ICON_IMAGE]=
		g_param_spec_object("icon-image",
							_("Icon image"),
							_("Image of icon"),
							CLUTTER_TYPE_IMAGE,
							G_PARAM_READWRITE);

	XfdashboardButtonProperties[PROP_ICON_SYNC_SIZE]=
		g_param_spec_boolean("sync-icon-size",
								_("Synchronize icon size"),
								_("Synchronize icon size with text size"),
								TRUE,
								G_PARAM_READWRITE);

	XfdashboardButtonProperties[PROP_ICON_SIZE]=
		g_param_spec_uint("icon-size",
							_("Icon size"),
							_("Size of icon if size of icon is not synchronized. -1 is valid for icon images and sets icon image's default size."),
							1, G_MAXUINT,
							DEFAULT_SIZE,
							G_PARAM_READWRITE);

	XfdashboardButtonProperties[PROP_ICON_ORIENTATION]=
		g_param_spec_enum("icon-orientation",
							_("Icon orientation"),
							_("Orientation of icon to label"),
							XFDASHBOARD_TYPE_ORIENTATION,
							XFDASHBOARD_ORIENTATION_LEFT,
							G_PARAM_READWRITE | G_PARAM_CONSTRUCT);

	XfdashboardButtonProperties[PROP_TEXT]=
		g_param_spec_string("text",
							_("Label text"),
							_("Text of label"),
							N_(""),
							G_PARAM_READWRITE);

	XfdashboardButtonProperties[PROP_TEXT_FONT]=
		g_param_spec_string("font",
							_("Font"),
							_("Font of label"),
							NULL,
							G_PARAM_READWRITE);

	XfdashboardButtonProperties[PROP_TEXT_COLOR]=
		clutter_param_spec_color("color",
									_("Color"),
									_("Color of label"),
									&defaultTextColor,
									G_PARAM_READWRITE | G_PARAM_CONSTRUCT);

	XfdashboardButtonProperties[PROP_TEXT_ELLIPSIZE_MODE]=
		g_param_spec_enum("ellipsize-mode",
							_("Ellipsize mode"),
							_("Mode of ellipsize if text in label is too long"),
							PANGO_TYPE_ELLIPSIZE_MODE,
							PANGO_ELLIPSIZE_MIDDLE,
							G_PARAM_READWRITE | G_PARAM_CONSTRUCT);

	g_object_class_install_properties(gobjectClass, PROP_LAST, XfdashboardButtonProperties);

	/* Define signals */
	XfdashboardButtonSignals[SIGNAL_CLICKED]=
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
void xfdashboard_button_init(XfdashboardButton *self)
{
	XfdashboardButtonPrivate	*priv;

	priv=self->priv=XFDASHBOARD_BUTTON_GET_PRIVATE(self);

	/* This actor reacts on events */
	clutter_actor_set_reactive(CLUTTER_ACTOR(self), TRUE);

	/* Set up default values */
	priv->margin=0.0f;
	priv->spacing=0.0f;
	priv->style=-1;
	priv->iconName=NULL;
	priv->iconImage=NULL;
	priv->iconSyncSize=TRUE;
	priv->iconSize=DEFAULT_SIZE;
	priv->iconOrientation=-1;
	priv->font=NULL;
	priv->labelColor=NULL;
	priv->labelEllipsize=-1;

	/* Create actors */
	priv->actorIcon=clutter_actor_new();
	clutter_actor_add_child(CLUTTER_ACTOR(self), priv->actorIcon);
	clutter_actor_set_reactive(priv->actorIcon, FALSE);

	priv->actorLabel=CLUTTER_TEXT(clutter_text_new());
	clutter_actor_add_child(CLUTTER_ACTOR(self), CLUTTER_ACTOR(priv->actorLabel));
	clutter_actor_set_reactive(CLUTTER_ACTOR(priv->actorLabel), FALSE);
	clutter_text_set_selectable(priv->actorLabel, FALSE);

	/* Connect signals */
	priv->clickAction=clutter_click_action_new();
	clutter_actor_add_action(CLUTTER_ACTOR(self), priv->clickAction);
	g_signal_connect(priv->clickAction, "clicked", G_CALLBACK(xfdashboard_button_clicked), NULL);
}

/* Implementation: Public API */

/* Create new actor */
ClutterActor* xfdashboard_button_new(void)
{
	return(g_object_new(XFDASHBOARD_TYPE_BUTTON,
						"text", N_(""),
						"style", XFDASHBOARD_STYLE_TEXT,
						NULL));
}

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
gfloat xfdashboard_button_get_margin(XfdashboardButton *self)
{
	g_return_val_if_fail(XFDASHBOARD_IS_BUTTON(self), 0);

	return(self->priv->margin);
}

void xfdashboard_button_set_margin(XfdashboardButton *self, const gfloat inMargin)
{
	g_return_if_fail(XFDASHBOARD_IS_BUTTON(self));
	g_return_if_fail(inMargin>=0.0f);

	XfdashboardButtonPrivate	*priv=self->priv;

	/* Set value if changed */
	if(priv->margin!=inMargin)
	{
		/* Set value */
		priv->margin=inMargin;
		clutter_actor_queue_relayout(CLUTTER_ACTOR(self));

		/* Update actor */
		xfdashboard_background_set_corner_radius(XFDASHBOARD_BACKGROUND(self), priv->margin);

		/* Notify about property change */
		g_object_notify_by_pspec(G_OBJECT(self), XfdashboardButtonProperties[PROP_MARGIN]);
	}
}

/* Get/set spacing between text and icon actors */
gfloat xfdashboard_button_get_spacing(XfdashboardButton *self)
{
	g_return_val_if_fail(XFDASHBOARD_IS_BUTTON(self), 0);

	return(self->priv->spacing);
}

void xfdashboard_button_set_spacing(XfdashboardButton *self, const gfloat inSpacing)
{
	g_return_if_fail(XFDASHBOARD_IS_BUTTON(self));
	g_return_if_fail(inSpacing>=0.0f);

	XfdashboardButtonPrivate	*priv=self->priv;

	/* Set value if changed */
	if(priv->spacing!=inSpacing)
	{
		/* Set value */
		priv->spacing=inSpacing;
		clutter_actor_queue_relayout(CLUTTER_ACTOR(self));

		/* Notify about property change */
		g_object_notify_by_pspec(G_OBJECT(self), XfdashboardButtonProperties[PROP_SPACING]);
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

	XfdashboardButtonPrivate	*priv=self->priv;

	/* Set value if changed */
	if(priv->style!=inStyle)
	{
		/* Set value */
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

		/* Notify about property change */
		g_object_notify_by_pspec(G_OBJECT(self), XfdashboardButtonProperties[PROP_STYLE]);
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

	XfdashboardButtonPrivate	*priv=self->priv;
	ClutterImage				*image;

	/* Set value if changed */
	if(priv->iconImage || g_strcmp0(priv->iconName, inIconName)!=0)
	{
		/* Set value */
		if(priv->iconName) g_free(priv->iconName);
		priv->iconName=g_strdup(inIconName);

		if(priv->iconImage)
		{
			clutter_actor_set_content(priv->actorIcon, NULL);
			g_object_unref(priv->iconImage);
			priv->iconImage=NULL;
		}

		image=xfdashboard_get_image_for_icon_name(priv->iconName, priv->iconSize);
		clutter_actor_set_content(priv->actorIcon, CLUTTER_CONTENT(image));
		g_object_unref(image);

		_xfdashboard_button_update_icon_image_size(self);

		/* Notify about property change */
		g_object_notify_by_pspec(G_OBJECT(self), XfdashboardButtonProperties[PROP_ICON_NAME]);
	}
}

ClutterImage* xfdashboard_button_get_icon_image(XfdashboardButton *self)
{
	g_return_val_if_fail(XFDASHBOARD_IS_BUTTON(self), NULL);

	return(self->priv->iconImage);
}

void xfdashboard_button_set_icon_image(XfdashboardButton *self, ClutterImage *inIconImage)
{
	g_return_if_fail(XFDASHBOARD_IS_BUTTON(self));
	g_return_if_fail(CLUTTER_IS_IMAGE(inIconImage));

	XfdashboardButtonPrivate	*priv=self->priv;

	/* Set value if changed */
	if(priv->iconName || inIconImage!=priv->iconImage)
	{
		/* Set value */
		if(priv->iconName) g_free(priv->iconName);
		priv->iconName=NULL;

		if(priv->iconImage)
		{
			clutter_actor_set_content(CLUTTER_ACTOR(self), NULL);
			g_object_unref(priv->iconImage);
		}

		priv->iconImage=g_object_ref(inIconImage);
		clutter_actor_set_content(priv->actorIcon, CLUTTER_CONTENT(priv->iconImage));

		_xfdashboard_button_update_icon_image_size(self);

		/* Notify about property change */
		g_object_notify_by_pspec(G_OBJECT(self), XfdashboardButtonProperties[PROP_ICON_IMAGE]);
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
	g_return_if_fail(inSize==-1 || inSize>0);

	XfdashboardButtonPrivate	*priv=self->priv;

	/* Set value if changed */
	if(priv->iconSize!=inSize)
	{
		/* Set value */
		priv->iconSize=inSize;

		if(priv->iconName)
		{
			ClutterImage		*image;

			image=xfdashboard_get_image_for_icon_name(priv->iconName, priv->iconSize);
			clutter_actor_set_content(priv->actorIcon, CLUTTER_CONTENT(image));
			g_object_unref(image);
		}

		_xfdashboard_button_update_icon_image_size(self);

		/* Notify about property change */
		g_object_notify_by_pspec(G_OBJECT(self), XfdashboardButtonProperties[PROP_ICON_SIZE]);
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

	XfdashboardButtonPrivate	*priv=self->priv;

	/* Set value if changed */
	if(priv->iconSyncSize!=inSync)
	{
		/* Set value */
		priv->iconSyncSize=inSync;

		_xfdashboard_button_update_icon_image_size(self);

		/* Notify about property change */
		g_object_notify_by_pspec(G_OBJECT(self), XfdashboardButtonProperties[PROP_ICON_SYNC_SIZE]);
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

	XfdashboardButtonPrivate	*priv=self->priv;

	/* Set value if changed */
	if(priv->iconOrientation!=inOrientation)
	{
		/* Set value */
		priv->iconOrientation=inOrientation;

		clutter_actor_queue_relayout(CLUTTER_ACTOR(self));

		/* Notify about property change */
		g_object_notify_by_pspec(G_OBJECT(self), XfdashboardButtonProperties[PROP_ICON_ORIENTATION]);
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

	XfdashboardButtonPrivate	*priv=self->priv;

	/* Set value if changed */
	if(g_strcmp0(clutter_text_get_text(priv->actorLabel), inMarkupText)!=0)
	{
		/* Set value */
		clutter_text_set_markup(priv->actorLabel, inMarkupText);
		clutter_actor_queue_relayout(CLUTTER_ACTOR(priv->actorLabel));

		/* Notify about property change */
		g_object_notify_by_pspec(G_OBJECT(self), XfdashboardButtonProperties[PROP_TEXT]);
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

	XfdashboardButtonPrivate	*priv=self->priv;

	/* Set value if changed */
	if(g_strcmp0(priv->font, inFont)!=0)
	{
		/* Set value */
		if(priv->font) g_free(priv->font);
		priv->font=(inFont ? g_strdup(inFont) : NULL);

		clutter_text_set_font_name(priv->actorLabel, priv->font);
		clutter_actor_queue_redraw(CLUTTER_ACTOR(self));

		/* Notify about property change */
		g_object_notify_by_pspec(G_OBJECT(self), XfdashboardButtonProperties[PROP_TEXT_FONT]);
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

	XfdashboardButtonPrivate	*priv=self->priv;

	/* Set value if changed */
	if(!priv->labelColor || !clutter_color_equal(inColor, priv->labelColor))
	{
		/* Set value */
		if(priv->labelColor) clutter_color_free(priv->labelColor);
		priv->labelColor=clutter_color_copy(inColor);

		clutter_text_set_color(priv->actorLabel, priv->labelColor);
		clutter_actor_queue_redraw(CLUTTER_ACTOR(self));

		/* Notify about property change */
		g_object_notify_by_pspec(G_OBJECT(self), XfdashboardButtonProperties[PROP_TEXT_COLOR]);
	}
}

/* Get/set ellipsize mode if label's text is getting too long */
PangoEllipsizeMode xfdashboard_button_get_ellipsize_mode(XfdashboardButton *self)
{
	g_return_val_if_fail(XFDASHBOARD_IS_BUTTON(self), 0);

	return(self->priv->labelEllipsize);
}

void xfdashboard_button_set_ellipsize_mode(XfdashboardButton *self, const PangoEllipsizeMode inMode)
{
	g_return_if_fail(XFDASHBOARD_IS_BUTTON(self));

	XfdashboardButtonPrivate	*priv=self->priv;

	/* Set value if changed */
	if(priv->labelEllipsize!=inMode)
	{
		/* Set value */
		priv->labelEllipsize=inMode;

		clutter_text_set_ellipsize(priv->actorLabel, priv->labelEllipsize);
		clutter_actor_queue_relayout(CLUTTER_ACTOR(self));

		/* Notify about property change */
		g_object_notify_by_pspec(G_OBJECT(self), XfdashboardButtonProperties[PROP_TEXT_ELLIPSIZE_MODE]);
	}
}
