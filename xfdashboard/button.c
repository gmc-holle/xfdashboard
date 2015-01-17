/*
 * button: An actor representing a label and an icon (both optional)
 *         and can react on click actions
 * 
 * Copyright 2012-2015 Stephan Haller <nomad@froevel.de>
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

#include <glib/gi18n-lib.h>
#include <gdk/gdk.h>
#include <math.h>

#include "enums.h"
#include "click-action.h"
#include "image-content.h"

/* Define this class in GObject system */
G_DEFINE_TYPE(XfdashboardButton,
				xfdashboard_button,
				XFDASHBOARD_TYPE_BACKGROUND)

/* Private structure - access only by public API if needed */
#define XFDASHBOARD_BUTTON_GET_PRIVATE(obj) \
	(G_TYPE_INSTANCE_GET_PRIVATE((obj), XFDASHBOARD_TYPE_BUTTON, XfdashboardButtonPrivate))

struct _XfdashboardButtonPrivate
{
	/* Properties related */
	gfloat					padding;
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
	gboolean				isSingleLineMode;
	PangoAlignment			textJustification;

	/* Instance related */
	ClutterActor			*actorIcon;
	ClutterText				*actorLabel;
	ClutterAction			*clickAction;

	gboolean				iconNameLoaded;
};

/* Properties */
enum
{
	PROP_0,

	PROP_PADDING,
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
	PROP_TEXT_SINGLE_LINE,
	PROP_TEXT_JUSTIFY,

	PROP_LAST
};

static GParamSpec* XfdashboardButtonProperties[PROP_LAST]={ 0, };

/* Signals */
enum
{
	SIGNAL_CLICKED,

	SIGNAL_LAST
};

static guint XfdashboardButtonSignals[SIGNAL_LAST]={ 0, };

/* IMPLEMENTATION: Private variables and methods */

/* Get preferred width of icon and label child actors
 * We do not respect paddings here so if height is given it must be
 * reduced by padding on all affected sides. The returned sizes are also
 * without these paddings.
 */
static void _xfdashboard_button_get_preferred_width_intern(XfdashboardButton *self,
															gboolean inGetPreferred,
															gfloat inForHeight,
															gfloat *outIconSize,
															gfloat *outLabelSize)
{
	XfdashboardButtonPrivate	*priv;
	gfloat						iconWidth, iconHeight, iconScale;
	gfloat						iconSize, labelSize;
	gfloat						minSize, naturalSize;

	g_return_if_fail(XFDASHBOARD_IS_BUTTON(self));

	priv=self->priv;

	/* Initialize sizes */
	iconSize=labelSize=0.0f;

	/* Calculate sizes
	 * No size given so natural layout is requested */
	if(inForHeight<0.0f)
	{
		/* Special case: both actors visible and icon size
		 * synchronization is turned on
		 */
		if(CLUTTER_ACTOR_IS_VISIBLE(priv->actorLabel) &&
			CLUTTER_ACTOR_IS_VISIBLE(priv->actorIcon) &&
			priv->iconSyncSize==TRUE)
		{
			gfloat		labelHeight;

			/* Get size of label */
			clutter_actor_get_preferred_width(CLUTTER_ACTOR(priv->actorLabel),
												inForHeight,
												&minSize, &naturalSize);
			labelSize=(inGetPreferred==TRUE ? naturalSize : minSize);

			/* Get size of icon depending on orientation */
			if(priv->iconOrientation==XFDASHBOARD_ORIENTATION_LEFT ||
				priv->iconOrientation==XFDASHBOARD_ORIENTATION_RIGHT)
			{
				/* Get both sizes of label to calculate icon size */
				clutter_actor_get_preferred_height(CLUTTER_ACTOR(priv->actorLabel),
													labelSize,
													&minSize, &naturalSize);
				labelHeight=(inGetPreferred==TRUE ? naturalSize : minSize);

				/* Get size of icon depending on opposize size of label */
				if(CLUTTER_IS_CONTENT(clutter_actor_get_content(priv->actorIcon)))
				{
					clutter_content_get_preferred_size(clutter_actor_get_content(priv->actorIcon),
														&iconWidth, &iconHeight);
					iconSize=(iconWidth/iconHeight)*labelHeight;
				}
					else iconSize=labelHeight;
			}
				else iconSize=labelSize;
		}
			/* Just get sizes of visible actors */
			else
			{
				/* Get size of label if visible */
				if(CLUTTER_ACTOR_IS_VISIBLE(priv->actorLabel))
				{
					clutter_actor_get_preferred_width(CLUTTER_ACTOR(priv->actorLabel),
														inForHeight,
														&minSize, &naturalSize);
					labelSize=(inGetPreferred==TRUE ? naturalSize : minSize);
				}

				/* Get size of icon if visible */
				if(CLUTTER_ACTOR_IS_VISIBLE(priv->actorIcon))
				{
					clutter_actor_get_preferred_width(CLUTTER_ACTOR(priv->actorIcon),
														inForHeight,
														&minSize, &naturalSize);
					iconSize=(inGetPreferred==TRUE ? naturalSize : minSize);
				}
			}
	}
		/* Special case: Size is given, both actors visible,
		 * icon size synchronization is turned on
		 */
		else if(CLUTTER_ACTOR_IS_VISIBLE(priv->actorLabel) &&
					CLUTTER_ACTOR_IS_VISIBLE(priv->actorIcon) &&
					priv->iconSyncSize==TRUE &&
					(priv->iconOrientation==XFDASHBOARD_ORIENTATION_TOP ||
						priv->iconOrientation==XFDASHBOARD_ORIENTATION_BOTTOM))
		{
			gfloat		labelMinimumSize;
			gfloat		requestSize, newRequestSize;

			/* Reduce size by padding and spacing */
			inForHeight-=priv->spacing;
			inForHeight-=2*priv->padding;
			inForHeight=MAX(0.0f, inForHeight);

			/* Get scale factor of icon */
			if(CLUTTER_IS_CONTENT(clutter_actor_get_content(priv->actorIcon)))
			{
				clutter_content_get_preferred_size(clutter_actor_get_content(priv->actorIcon),
													&iconWidth, &iconHeight);
				iconScale=(iconWidth/iconHeight);
				iconWidth=(iconHeight/iconWidth)*inForHeight;
				iconHeight=iconWidth/iconScale;
			}
				else iconScale=iconWidth=iconHeight=0.0f;

			/* Get minimum size of label because we should never
			 * go down below this minimum size
			 */
			clutter_actor_get_preferred_height(CLUTTER_ACTOR(priv->actorLabel),
												-1.0f,
												&labelMinimumSize, NULL);

			/* Initialize height with value if it could occupy 100% width and
			 * set icon size to negative value to show that its value was not
			 * found yet
			 */
			iconSize=-1.0f;

			clutter_actor_get_preferred_width(CLUTTER_ACTOR(priv->actorLabel),
												inForHeight,
												&minSize, &naturalSize);
			requestSize=(inGetPreferred==TRUE ? naturalSize : minSize);

			if(priv->labelEllipsize==PANGO_ELLIPSIZE_NONE ||
				clutter_text_get_single_line_mode(priv->actorLabel)==FALSE)
			{
				do
				{
					/* Get size of icon */
					iconHeight=requestSize;
					iconWidth=iconHeight*iconScale;

					/* Reduce size for label by size of icon and
					 * get its opposize size
					 */
					clutter_actor_get_preferred_width(CLUTTER_ACTOR(priv->actorLabel),
														inForHeight-iconHeight,
														&minSize, &naturalSize);
					newRequestSize=(inGetPreferred==TRUE ? naturalSize : minSize);

					/* If new opposite size is equal (or unexpectly lower) than
					 * initial opposize size we found the sizes
					 */
					if(newRequestSize<=requestSize)
					{
						iconSize=iconWidth;
						labelSize=newRequestSize;
					}
					requestSize=newRequestSize;
				}
				while(iconSize<0.0f && (inForHeight-iconHeight)>labelMinimumSize);
			}
				else
				{
					/* Get size of icon */
					iconWidth=requestSize;
					iconHeight=iconWidth/iconScale;
					iconSize=iconWidth;

					/* Adjust label size */
					labelSize=requestSize-iconWidth;
				}
		}
		/* Size is given but nothing special */
		else
		{
			/* Reduce size by padding and if both icon and label are visible
			 * also reduce by spacing
			 */
			if(CLUTTER_ACTOR_IS_VISIBLE(priv->actorIcon) &&
				(CLUTTER_ACTOR_IS_VISIBLE(priv->actorLabel)))
			{
				inForHeight-=priv->spacing;
			}
			inForHeight-=2*priv->padding;
			inForHeight=MAX(0.0f, inForHeight);

			/* Get icon size if visible */
			if(CLUTTER_ACTOR_IS_VISIBLE(priv->actorIcon))
			{
				if(priv->iconSyncSize==TRUE &&
					(priv->iconOrientation==XFDASHBOARD_ORIENTATION_LEFT ||
						priv->iconOrientation==XFDASHBOARD_ORIENTATION_RIGHT))
				{
					if(CLUTTER_IS_CONTENT(clutter_actor_get_content(priv->actorIcon)))
					{
						/* Get scale factor of icon and scale icon */
						clutter_content_get_preferred_size(clutter_actor_get_content(priv->actorIcon),
															&iconWidth, &iconHeight);
						minSize=naturalSize=inForHeight*(iconWidth/iconHeight);
					}
						else minSize=naturalSize=0.0f;
				}
					else
					{
						clutter_actor_get_preferred_width(CLUTTER_ACTOR(priv->actorIcon),
															inForHeight,
															&minSize, &naturalSize);
					}

				iconSize=(inGetPreferred==TRUE ? naturalSize : minSize);
			}

			/* Get label size if visible */
			if(CLUTTER_ACTOR_IS_VISIBLE(priv->actorLabel))
			{
				if(priv->iconOrientation==XFDASHBOARD_ORIENTATION_TOP ||
						priv->iconOrientation==XFDASHBOARD_ORIENTATION_BOTTOM)
				{
					inForHeight-=iconSize;
				}

				clutter_actor_get_preferred_width(CLUTTER_ACTOR(priv->actorLabel),
													inForHeight,
													&minSize, &naturalSize);
				labelSize=(inGetPreferred==TRUE ? naturalSize : minSize);
			}
		}

	/* Set computed sizes */
	if(outIconSize) *outIconSize=iconSize;
	if(outLabelSize) *outLabelSize=labelSize;
}

/* Get preferred height of icon and label child actors
 * We do not respect paddings here so if width is given it must be
 * reduced by paddings and spacing. The returned sizes are alsowithout
 * these paddings and spacing.
 */
static void _xfdashboard_button_get_preferred_height_intern(XfdashboardButton *self,
															gboolean inGetPreferred,
															gfloat inForWidth,
															gfloat *outIconSize,
															gfloat *outLabelSize)
{
	XfdashboardButtonPrivate	*priv;
	gfloat						iconWidth, iconHeight, iconScale;
	gfloat						iconSize, labelSize;
	gfloat						minSize, naturalSize;

	g_return_if_fail(XFDASHBOARD_IS_BUTTON(self));

	priv=self->priv;

	/* Initialize sizes */
	iconSize=labelSize=0.0f;

	/* Calculate sizes
	 * No size given so natural layout is requested */
	if(inForWidth<0.0f)
	{
		/* Special case: both actors visible and icon size
		 * synchronization is turned on
		 */
		if(CLUTTER_ACTOR_IS_VISIBLE(priv->actorLabel) &&
			CLUTTER_ACTOR_IS_VISIBLE(priv->actorIcon) &&
			priv->iconSyncSize==TRUE)
		{
			gfloat		labelWidth;

			/* Get size of label */
			clutter_actor_get_preferred_height(CLUTTER_ACTOR(priv->actorLabel),
												inForWidth,
												&minSize, &naturalSize);
			labelSize=(inGetPreferred==TRUE ? naturalSize : minSize);

			/* Get size of icon depending on orientation */
			if(priv->iconOrientation==XFDASHBOARD_ORIENTATION_TOP ||
				priv->iconOrientation==XFDASHBOARD_ORIENTATION_BOTTOM)
			{
				/* Get both sizes of label to calculate icon size */
				clutter_actor_get_preferred_width(CLUTTER_ACTOR(priv->actorLabel),
													labelSize,
													&minSize, &naturalSize);
				labelWidth=(inGetPreferred==TRUE ? naturalSize : minSize);

				/* Get size of icon depending on opposize size of label */
				if(CLUTTER_IS_CONTENT(clutter_actor_get_content(priv->actorIcon)))
				{
					clutter_content_get_preferred_size(clutter_actor_get_content(priv->actorIcon),
														&iconWidth, &iconHeight);
					iconSize=(iconHeight/iconWidth)*labelWidth;
				}
					else iconSize=labelWidth;
			}
				else iconSize=labelSize;
		}
			/* Just get sizes of visible actors */
			else
			{
				/* Get sizes of visible actors */
				if(CLUTTER_ACTOR_IS_VISIBLE(priv->actorIcon))
				{
					clutter_actor_get_preferred_height(CLUTTER_ACTOR(priv->actorIcon),
														inForWidth,
														&minSize, &naturalSize);
					iconSize=(inGetPreferred==TRUE ? naturalSize : minSize);
				}

				if(CLUTTER_ACTOR_IS_VISIBLE(priv->actorLabel))
				{
					clutter_actor_get_preferred_height(CLUTTER_ACTOR(priv->actorLabel),
														inForWidth,
														&minSize, &naturalSize);
					labelSize=(inGetPreferred==TRUE ? naturalSize : minSize);
				}
			}
	}
		/* Special case: Size is given, both actors visible,
		 * icon size synchronization is turned on
		 */
		else if(CLUTTER_ACTOR_IS_VISIBLE(priv->actorLabel) &&
					CLUTTER_ACTOR_IS_VISIBLE(priv->actorIcon) &&
					priv->iconSyncSize==TRUE &&
					(priv->iconOrientation==XFDASHBOARD_ORIENTATION_LEFT ||
						priv->iconOrientation==XFDASHBOARD_ORIENTATION_RIGHT))
		{
			gfloat		labelMinimumSize;
			gfloat		requestSize, newRequestSize;

			/* Reduce size by padding and spacing */
			inForWidth-=priv->spacing;
			inForWidth-=2*priv->padding;
			inForWidth=MAX(0.0f, inForWidth);

			/* Get scale factor of icon */
			if(CLUTTER_IS_CONTENT(clutter_actor_get_content(priv->actorIcon)))
			{
				clutter_content_get_preferred_size(clutter_actor_get_content(priv->actorIcon),
													&iconWidth, &iconHeight);
				iconScale=(iconWidth/iconHeight);
				iconWidth=(iconHeight/iconWidth)*inForWidth;
				iconHeight=iconWidth/iconScale;
			}
				else iconScale=iconWidth=iconHeight=0.0f;

			/* Get minimum size of label because we should never
			 * go down below this minimum size
			 */
			clutter_actor_get_preferred_width(CLUTTER_ACTOR(priv->actorLabel),
												-1.0f,
												&labelMinimumSize, NULL);

			/* Initialize height with value if it could occupy 100% width and
			 * set icon size to negative value to show that its value was not
			 * found yet
			 */
			iconSize=-1.0f;

			clutter_actor_get_preferred_height(CLUTTER_ACTOR(priv->actorLabel),
												inForWidth,
												&minSize, &naturalSize);
			requestSize=(inGetPreferred==TRUE ? naturalSize : minSize);

			if(priv->labelEllipsize==PANGO_ELLIPSIZE_NONE ||
				clutter_text_get_single_line_mode(priv->actorLabel)==FALSE)
			{
				do
				{
					/* Get size of icon */
					iconHeight=requestSize;
					iconWidth=iconHeight*iconScale;

					/* Reduce size for label by size of icon and
					 * get its opposize size
					 */
					clutter_actor_get_preferred_height(CLUTTER_ACTOR(priv->actorLabel),
														inForWidth-iconWidth,
														&minSize, &naturalSize);
					newRequestSize=(inGetPreferred==TRUE ? naturalSize : minSize);

					/* If new opposite size is equal (or unexpectly lower) than
					 * initial opposize size we found the sizes
					 */
					if(newRequestSize<=requestSize)
					{
						iconSize=iconHeight;
						labelSize=newRequestSize;
					}
					requestSize=newRequestSize;
				}
				while(iconSize<0.0f && (inForWidth-iconWidth)>labelMinimumSize);
			}
				else
				{
					/* Get size of icon */
					iconHeight=requestSize;
					iconWidth=iconHeight*iconScale;
					iconSize=iconHeight;

					/* Adjust label size */
					labelSize=requestSize-iconHeight;
				}
		}
		/* Size is given but nothing special */
		else
		{
			/* Reduce size by padding and if both icon and label are visible
			 * also reduce by spacing
			 */
			if(CLUTTER_ACTOR_IS_VISIBLE(priv->actorIcon) &&
				(CLUTTER_ACTOR_IS_VISIBLE(priv->actorLabel)))
			{
				inForWidth-=priv->spacing;
			}
			inForWidth-=2*priv->padding;
			inForWidth=MAX(0.0f, inForWidth);

			/* Get icon size if visible */
			if(CLUTTER_ACTOR_IS_VISIBLE(priv->actorIcon))
			{
				if(priv->iconSyncSize==TRUE &&
					(priv->iconOrientation==XFDASHBOARD_ORIENTATION_TOP ||
						priv->iconOrientation==XFDASHBOARD_ORIENTATION_BOTTOM))
				{
					if(CLUTTER_IS_CONTENT(clutter_actor_get_content(priv->actorIcon)))
					{
						/* Get scale factor of icon and scale icon */
						clutter_content_get_preferred_size(clutter_actor_get_content(priv->actorIcon),
															&iconWidth, &iconHeight);
						minSize=naturalSize=inForWidth*(iconHeight/iconWidth);
					}
						else minSize=naturalSize=0.0f;
				}
					else
					{
						clutter_actor_get_preferred_height(CLUTTER_ACTOR(priv->actorIcon),
															inForWidth,
															&minSize, &naturalSize);
					}
				iconSize=(inGetPreferred==TRUE ? naturalSize : minSize);
			}

			/* Get label size if visible */
			if(CLUTTER_ACTOR_IS_VISIBLE(priv->actorLabel))
			{
				if(priv->iconOrientation==XFDASHBOARD_ORIENTATION_LEFT ||
						priv->iconOrientation==XFDASHBOARD_ORIENTATION_RIGHT)
				{
					inForWidth-=iconSize;
				}

				clutter_actor_get_preferred_height(CLUTTER_ACTOR(priv->actorLabel),
													inForWidth,
													&minSize, &naturalSize);
				labelSize=(inGetPreferred==TRUE ? naturalSize : minSize);
			}
		}

	/* Set computed sizes */
	if(outIconSize) *outIconSize=iconSize;
	if(outLabelSize) *outLabelSize=labelSize;
}

/* Update icon */
static void _xfdashboard_button_update_icon_image_size(XfdashboardButton *self)
{
	XfdashboardButtonPrivate	*priv;
	gfloat						iconWidth, iconHeight;
	gfloat						maxSize;

	g_return_if_fail(XFDASHBOARD_IS_BUTTON(self));

	priv=self->priv;
	iconWidth=iconHeight=-1.0f;
	maxSize=0.0f;

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

		if(priv->iconOrientation==XFDASHBOARD_ORIENTATION_TOP ||
			priv->iconOrientation==XFDASHBOARD_ORIENTATION_BOTTOM)
		{
			maxSize=labelWidth;
		}
			else
			{
				maxSize=labelHeight;
			}
	}
		else if(priv->iconSize>0.0f) maxSize=priv->iconSize;

	/* Get size of icon if maximum size is set */
	if(maxSize>0.0f && CLUTTER_IS_CONTENT(clutter_actor_get_content(priv->actorIcon)))
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

/* Actor was mapped or unmapped */
static void _xfdashboard_button_on_mapped_changed(XfdashboardButton *self,
													GParamSpec *inSpec,
													gpointer inUserData)
{
	XfdashboardButtonPrivate	*priv;

	g_return_if_fail(XFDASHBOARD_IS_BUTTON(self));

	priv=self->priv;

	/* If actor is mapped now and an image by icon name was set but not
	 * loaded yet then set icon image now
	 */
	if(CLUTTER_ACTOR_IS_MAPPED(self) &&
		priv->iconNameLoaded==FALSE &&
		priv->iconName)
	{
		ClutterImage	*image;

		/* Set icon image */
		image=xfdashboard_image_content_new_for_icon_name(priv->iconName, priv->iconSize);
		clutter_actor_set_content(priv->actorIcon, CLUTTER_CONTENT(image));
		g_object_unref(image);

		priv->iconNameLoaded=TRUE;

		/* Calculate icon size as image content is now available */
		_xfdashboard_button_update_icon_image_size(self);

		g_debug("Loaded and set deferred image '%s' at size %d for %s@%p ", priv->iconName, priv->iconSize, G_OBJECT_TYPE_NAME(self), self);
	}
}

/* IMPLEMENTATION: ClutterActor */

/* Show all children of this actor */
static void _xfdashboard_button_show_all(ClutterActor *self)
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
static void _xfdashboard_button_hide_all(ClutterActor *self)
{
	XfdashboardButtonPrivate	*priv=XFDASHBOARD_BUTTON(self)->priv;

	clutter_actor_hide(self);
	clutter_actor_hide(CLUTTER_ACTOR(priv->actorIcon));
	clutter_actor_hide(CLUTTER_ACTOR(priv->actorLabel));
}

/* Get preferred width/height */
static void _xfdashboard_button_get_preferred_height(ClutterActor *inActor,
														gfloat inForWidth,
														gfloat *outMinHeight,
														gfloat *outNaturalHeight)
{
	XfdashboardButton			*self=XFDASHBOARD_BUTTON(inActor);
	XfdashboardButtonPrivate	*priv=self->priv;
	gfloat						minHeight, naturalHeight;
	gfloat						minIconHeight, naturalIconHeight;
	gfloat						minLabelHeight, naturalLabelHeight;
	gfloat						spacing=priv->spacing;

	/* Initialize sizes */
	minHeight=naturalHeight=0.0f;

	/* Calculate sizes for requested one (means which can and will be stored) */
	if(outMinHeight)
	{
		_xfdashboard_button_get_preferred_height_intern(self,
															FALSE,
															inForWidth,
															&minIconHeight,
															&minLabelHeight);
	}

	if(outNaturalHeight)
	{
		_xfdashboard_button_get_preferred_height_intern(self,
															TRUE,
															inForWidth,
															&naturalIconHeight,
															&naturalLabelHeight);
	}

	if(CLUTTER_ACTOR_IS_VISIBLE(priv->actorLabel)!=TRUE ||
		CLUTTER_ACTOR_IS_VISIBLE(priv->actorIcon)!=TRUE)
	{
		spacing=0.0f;
	}

	switch(priv->iconOrientation)
	{
		case XFDASHBOARD_ORIENTATION_TOP:
		case XFDASHBOARD_ORIENTATION_BOTTOM:
			minHeight=minIconHeight+minLabelHeight;
			naturalHeight=naturalIconHeight+naturalLabelHeight;
			break;
			
		default:
			minHeight=MAX(minIconHeight, minLabelHeight);
			naturalHeight=MAX(naturalIconHeight, naturalLabelHeight);
			break;
	}

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

	/* Add padding */
	minHeight+=2*priv->padding;
	naturalHeight+=2*priv->padding;

	/* Store sizes computed */
	if(outMinHeight) *outMinHeight=minHeight;
	if(outNaturalHeight) *outNaturalHeight=naturalHeight;
}

static void _xfdashboard_button_get_preferred_width(ClutterActor *inActor,
													gfloat inForHeight,
													gfloat *outMinWidth,
													gfloat *outNaturalWidth)
{
	XfdashboardButton			*self=XFDASHBOARD_BUTTON(inActor);
	XfdashboardButtonPrivate	*priv=self->priv;
	gfloat						minWidth, naturalWidth;
	gfloat						minIconWidth, naturalIconWidth;
	gfloat						minLabelWidth, naturalLabelWidth;
	gfloat						spacing=priv->spacing;

	/* Initialize sizes */
	minWidth=naturalWidth=0.0f;

	/* Calculate sizes for requested one (means which can and will be stored) */
	if(outMinWidth)
	{
		_xfdashboard_button_get_preferred_width_intern(self,
															FALSE,
															inForHeight,
															&minIconWidth,
															&minLabelWidth);
	}

	if(outNaturalWidth)
	{
		_xfdashboard_button_get_preferred_width_intern(self,
															TRUE,
															inForHeight,
															&naturalIconWidth,
															&naturalLabelWidth);
	}

	if(CLUTTER_ACTOR_IS_VISIBLE(priv->actorLabel)!=TRUE ||
		CLUTTER_ACTOR_IS_VISIBLE(priv->actorIcon)!=TRUE)
	{
		spacing=0.0f;
	}

	switch(priv->iconOrientation)
	{
		case XFDASHBOARD_ORIENTATION_LEFT:
		case XFDASHBOARD_ORIENTATION_RIGHT:
			minWidth=minIconWidth+minLabelWidth;
			naturalWidth=naturalIconWidth+naturalLabelWidth;
			break;
			
		default:
			minWidth=MAX(minIconWidth, minLabelWidth);
			naturalWidth=MAX(naturalIconWidth, naturalLabelWidth);
			break;
	}

	/* Add spacing to size if orientation is left or right.
	 * Spacing was initially set to spacing in settings but
	 * resetted to zero if either text or icon is not visible.
	 */
	if(priv->iconOrientation==XFDASHBOARD_ORIENTATION_LEFT ||
		priv->iconOrientation==XFDASHBOARD_ORIENTATION_RIGHT)
	{
		minWidth+=spacing;
		naturalWidth+=spacing;
	}

	/* Add padding */
	minWidth+=2*priv->padding;
	naturalWidth+=2*priv->padding;

	/* Store sizes computed */
	if(outMinWidth) *outMinWidth=minWidth;
	if(outNaturalWidth) *outNaturalWidth=naturalWidth;
}

/* Allocate position and size of actor and its children */
static void _xfdashboard_button_allocate(ClutterActor *inActor,
											const ClutterActorBox *inBox,
											ClutterAllocationFlags inFlags)
{
	XfdashboardButton			*self=XFDASHBOARD_BUTTON(inActor);
	XfdashboardButtonPrivate	*priv=self->priv;
	ClutterActorBox				*boxLabel=NULL;
	ClutterActorBox				*boxIcon=NULL;
	gfloat						left, right, top, bottom;
	gfloat						textWidth, textHeight;
	gfloat						iconWidth, iconHeight;
	gfloat						spacing=priv->spacing;

	/* Chain up to store the allocation of the actor */
	CLUTTER_ACTOR_CLASS(xfdashboard_button_parent_class)->allocate(inActor, inBox, inFlags);

	/* Get sizes of children and determine if we need
	 * to add spacing between text and icon. If either
	 * icon or text is not visible reset its size to zero
	 * and also reset spacing to zero.
	 */
	if(!CLUTTER_ACTOR_IS_VISIBLE(priv->actorIcon) ||
			!CLUTTER_ACTOR_IS_VISIBLE(priv->actorLabel))
	{
		spacing=0.0f;
	}

	/* Get icon sizes */
	iconWidth=iconHeight=0.0f;
	if(CLUTTER_ACTOR_IS_VISIBLE(priv->actorIcon))
	{
		gfloat					iconScale=1.0f;

		if(priv->iconSyncSize==TRUE &&
			CLUTTER_IS_CONTENT(clutter_actor_get_content(priv->actorIcon)))
		{
			clutter_content_get_preferred_size(clutter_actor_get_content(priv->actorIcon),
												&iconWidth, &iconHeight);
			iconScale=(iconWidth/iconHeight);
		}

		if(clutter_actor_get_request_mode(CLUTTER_ACTOR(self))==CLUTTER_REQUEST_HEIGHT_FOR_WIDTH)
		{
			_xfdashboard_button_get_preferred_height_intern(self,
															TRUE,
															clutter_actor_box_get_width(inBox),
															&iconHeight,
															NULL);
			if(priv->iconSyncSize==TRUE) iconWidth=iconHeight*iconScale;
				else clutter_actor_get_preferred_width(CLUTTER_ACTOR(priv->actorIcon), iconHeight, NULL, &iconWidth);
		}
			else
			{
				_xfdashboard_button_get_preferred_width_intern(self,
																TRUE,
																clutter_actor_box_get_height(inBox),
																&iconWidth,
																NULL);
				if(priv->iconSyncSize==TRUE) iconHeight=iconWidth/iconScale;
					else clutter_actor_get_preferred_height(CLUTTER_ACTOR(priv->actorIcon), iconWidth, NULL, &iconHeight);
			}
	}

	/* Set allocation of label if visible*/
	textWidth=textHeight=0.0f;
	if(CLUTTER_ACTOR_IS_VISIBLE(priv->actorLabel))
	{
		switch(priv->iconOrientation)
		{
			case XFDASHBOARD_ORIENTATION_TOP:
				textWidth=MAX(0.0f, clutter_actor_box_get_width(inBox)-2*priv->padding);

				textHeight=clutter_actor_box_get_height(inBox)-iconHeight-2*priv->padding;
				if(CLUTTER_ACTOR_IS_VISIBLE(priv->actorIcon)) textHeight-=priv->spacing;
				textHeight=MAX(0.0f, textHeight);

				left=((clutter_actor_box_get_width(inBox)-textWidth)/2.0f);
				right=left+textWidth;
				top=priv->padding+iconHeight+spacing;
				bottom=top+textHeight;
				break;

			case XFDASHBOARD_ORIENTATION_BOTTOM:
				textWidth=MAX(0.0f, clutter_actor_box_get_width(inBox)-2*priv->padding);

				textHeight=clutter_actor_box_get_height(inBox)-iconHeight-2*priv->padding;
				if(CLUTTER_ACTOR_IS_VISIBLE(priv->actorIcon)) textHeight-=priv->spacing;
				textHeight=MAX(0.0f, textHeight);

				left=((clutter_actor_box_get_width(inBox)-textWidth)/2.0f);
				right=left+textWidth;
				top=priv->padding;
				bottom=top+textHeight;
				break;

			case XFDASHBOARD_ORIENTATION_RIGHT:
				textWidth=clutter_actor_box_get_width(inBox)-iconWidth-2*priv->padding;
				if(CLUTTER_ACTOR_IS_VISIBLE(priv->actorIcon)) textWidth-=priv->spacing;
				textWidth=MAX(0.0f, textWidth);

				textHeight=MAX(0.0f, clutter_actor_box_get_height(inBox)-2*priv->padding);

				left=priv->padding;
				right=left+textWidth;
				top=priv->padding;
				bottom=top+textHeight;
				break;

			case XFDASHBOARD_ORIENTATION_LEFT:
			default:
				textWidth=clutter_actor_box_get_width(inBox)-iconWidth-2*priv->padding;
				if(CLUTTER_ACTOR_IS_VISIBLE(priv->actorIcon)) textWidth-=priv->spacing;
				textWidth=MAX(0.0f, textWidth);

				textHeight=MAX(0.0f, clutter_actor_box_get_height(inBox)-2*priv->padding);

				left=priv->padding+iconWidth+spacing;
				right=left+textWidth;
				top=priv->padding;
				bottom=top+textHeight;
				break;
		}

		right=MAX(left, right);
		bottom=MAX(top, bottom);

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
				top=priv->padding;
				bottom=top+iconHeight;
				break;

			case XFDASHBOARD_ORIENTATION_BOTTOM:
				left=((clutter_actor_box_get_width(inBox)-iconWidth)/2.0f);
				right=left+iconWidth;
				top=priv->padding+textHeight+spacing;
				bottom=top+iconHeight;
				break;

			case XFDASHBOARD_ORIENTATION_RIGHT:
				left=clutter_actor_box_get_width(inBox)-priv->padding-iconWidth;
				right=clutter_actor_box_get_width(inBox)-priv->padding;
				top=priv->padding;
				bottom=top+iconHeight;
				break;

			case XFDASHBOARD_ORIENTATION_LEFT:
			default:
				left=priv->padding;
				right=left+iconWidth;
				top=priv->padding;
				bottom=top+iconHeight;
				break;
		}

		right=MAX(left, right);
		bottom=MAX(top, bottom);

		boxIcon=clutter_actor_box_new(floor(left), floor(top), floor(right), floor(bottom));
		clutter_actor_allocate(CLUTTER_ACTOR(priv->actorIcon), boxIcon, inFlags);
	}

	/* Release allocated memory */
	if(boxLabel) clutter_actor_box_free(boxLabel);
	if(boxIcon) clutter_actor_box_free(boxIcon);
}

/* proxy ClickAction signals */
static void _xfdashboard_button_clicked(ClutterClickAction *inAction,
										ClutterActor *self,
										gpointer inUserData)
{
	g_signal_emit(self, XfdashboardButtonSignals[SIGNAL_CLICKED], 0);
}

/* Destroy this actor */
static void _xfdashboard_button_destroy(ClutterActor *self)
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
static void _xfdashboard_button_dispose(GObject *inObject)
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
static void _xfdashboard_button_set_property(GObject *inObject,
												guint inPropID,
												const GValue *inValue,
												GParamSpec *inSpec)
{
	XfdashboardButton			*self=XFDASHBOARD_BUTTON(inObject);
	
	switch(inPropID)
	{
		case PROP_PADDING:
			xfdashboard_button_set_padding(self, g_value_get_float(inValue));
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

		case PROP_TEXT_SINGLE_LINE:
			xfdashboard_button_set_single_line_mode(self, g_value_get_boolean(inValue));
			break;

		case PROP_TEXT_JUSTIFY:
			xfdashboard_button_set_text_justification(self, g_value_get_enum(inValue));
			break;

		default:
			G_OBJECT_WARN_INVALID_PROPERTY_ID(inObject, inPropID, inSpec);
			break;
	}
}

static void _xfdashboard_button_get_property(GObject *inObject,
												guint inPropID,
												GValue *outValue,
												GParamSpec *inSpec)
{
	XfdashboardButton			*self=XFDASHBOARD_BUTTON(inObject);
	XfdashboardButtonPrivate	*priv=self->priv;

	switch(inPropID)
	{
		case PROP_PADDING:
			g_value_set_float(outValue, priv->padding);
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

		case PROP_TEXT_SINGLE_LINE:
			g_value_set_boolean(outValue, priv->isSingleLineMode);
			break;

		case PROP_TEXT_JUSTIFY:
			g_value_set_enum(outValue, priv->textJustification);
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
	XfdashboardActorClass	*actorClass=XFDASHBOARD_ACTOR_CLASS(klass);
	ClutterActorClass		*clutterActorClass=CLUTTER_ACTOR_CLASS(klass);
	GObjectClass			*gobjectClass=G_OBJECT_CLASS(klass);

	/* Override functions */
	gobjectClass->dispose=_xfdashboard_button_dispose;
	gobjectClass->set_property=_xfdashboard_button_set_property;
	gobjectClass->get_property=_xfdashboard_button_get_property;

	clutterActorClass->show_all=_xfdashboard_button_show_all;
	clutterActorClass->hide_all=_xfdashboard_button_hide_all;
	clutterActorClass->get_preferred_width=_xfdashboard_button_get_preferred_width;
	clutterActorClass->get_preferred_height=_xfdashboard_button_get_preferred_height;
	clutterActorClass->allocate=_xfdashboard_button_allocate;
	clutterActorClass->destroy=_xfdashboard_button_destroy;

	/* Set up private structure */
	g_type_class_add_private(klass, sizeof(XfdashboardButtonPrivate));

	/* Define properties */
	XfdashboardButtonProperties[PROP_PADDING]=
		g_param_spec_float("padding",
							_("Padding"),
							_("Padding between background and elements"),
							0.0f, G_MAXFLOAT,
							4.0f,
							G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS | G_PARAM_CONSTRUCT);

	XfdashboardButtonProperties[PROP_SPACING]=
		g_param_spec_float("spacing",
							_("Spacing"),
							_("Spacing between text and icon"),
							0.0f, G_MAXFLOAT,
							4.0f,
							G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS | G_PARAM_CONSTRUCT);

	XfdashboardButtonProperties[PROP_STYLE]=
		g_param_spec_enum("button-style",
							_("Button style"),
							_("Style of button showing text and/or icon"),
							XFDASHBOARD_TYPE_STYLE,
							XFDASHBOARD_STYLE_TEXT,
							G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS | G_PARAM_CONSTRUCT);

	XfdashboardButtonProperties[PROP_ICON_NAME]=
		g_param_spec_string("icon-name",
							_("Icon name"),
							_("Themed icon name or file name of icon"),
							N_(""),
							G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS);

	XfdashboardButtonProperties[PROP_ICON_IMAGE]=
		g_param_spec_object("icon-image",
							_("Icon image"),
							_("Image of icon"),
							CLUTTER_TYPE_IMAGE,
							G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS);

	XfdashboardButtonProperties[PROP_ICON_SYNC_SIZE]=
		g_param_spec_boolean("sync-icon-size",
								_("Synchronize icon size"),
								_("Synchronize icon size with text size"),
								TRUE,
								G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS);

	XfdashboardButtonProperties[PROP_ICON_SIZE]=
		g_param_spec_uint("icon-size",
							_("Icon size"),
							_("Size of icon if size of icon is not synchronized. -1 is valid for icon images and sets icon image's default size."),
							1, G_MAXUINT,
							16,
							G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS);

	XfdashboardButtonProperties[PROP_ICON_ORIENTATION]=
		g_param_spec_enum("icon-orientation",
							_("Icon orientation"),
							_("Orientation of icon to label"),
							XFDASHBOARD_TYPE_ORIENTATION,
							XFDASHBOARD_ORIENTATION_LEFT,
							G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS | G_PARAM_CONSTRUCT);

	XfdashboardButtonProperties[PROP_TEXT]=
		g_param_spec_string("text",
							_("Label text"),
							_("Text of label"),
							N_(""),
							G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS);

	XfdashboardButtonProperties[PROP_TEXT_FONT]=
		g_param_spec_string("font",
							_("Font"),
							_("Font of label"),
							NULL,
							G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS);

	XfdashboardButtonProperties[PROP_TEXT_COLOR]=
		clutter_param_spec_color("color",
									_("Color"),
									_("Color of label"),
									NULL,
									G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS);

	XfdashboardButtonProperties[PROP_TEXT_ELLIPSIZE_MODE]=
		g_param_spec_enum("ellipsize-mode",
							_("Ellipsize mode"),
							_("Mode of ellipsize if text in label is too long"),
							PANGO_TYPE_ELLIPSIZE_MODE,
							PANGO_ELLIPSIZE_MIDDLE,
							G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS | G_PARAM_CONSTRUCT);

	XfdashboardButtonProperties[PROP_TEXT_SINGLE_LINE]=
		g_param_spec_boolean("single-line",
								_("Single line"),
								_("Flag to determine if text can only be in one or multiple lines"),
								TRUE,
								G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS);

	XfdashboardButtonProperties[PROP_TEXT_JUSTIFY]=
		g_param_spec_enum("text-justify",
							_("Text justify"),
							_("Justification (line alignment) of label"),
							PANGO_TYPE_ALIGNMENT,
							PANGO_ALIGN_LEFT,
							G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS);

	g_object_class_install_properties(gobjectClass, PROP_LAST, XfdashboardButtonProperties);

	/* Define stylable properties */
	xfdashboard_actor_install_stylable_property(actorClass, XfdashboardButtonProperties[PROP_PADDING]);
	xfdashboard_actor_install_stylable_property(actorClass, XfdashboardButtonProperties[PROP_SPACING]);
	xfdashboard_actor_install_stylable_property(actorClass, XfdashboardButtonProperties[PROP_STYLE]);
	xfdashboard_actor_install_stylable_property(actorClass, XfdashboardButtonProperties[PROP_ICON_NAME]);
	xfdashboard_actor_install_stylable_property(actorClass, XfdashboardButtonProperties[PROP_ICON_IMAGE]);
	xfdashboard_actor_install_stylable_property(actorClass, XfdashboardButtonProperties[PROP_ICON_SYNC_SIZE]);
	xfdashboard_actor_install_stylable_property(actorClass, XfdashboardButtonProperties[PROP_ICON_SIZE]);
	xfdashboard_actor_install_stylable_property(actorClass, XfdashboardButtonProperties[PROP_ICON_ORIENTATION]);
	xfdashboard_actor_install_stylable_property(actorClass, XfdashboardButtonProperties[PROP_TEXT]);
	xfdashboard_actor_install_stylable_property(actorClass, XfdashboardButtonProperties[PROP_TEXT_FONT]);
	xfdashboard_actor_install_stylable_property(actorClass, XfdashboardButtonProperties[PROP_TEXT_COLOR]);
	xfdashboard_actor_install_stylable_property(actorClass, XfdashboardButtonProperties[PROP_TEXT_ELLIPSIZE_MODE]);
	xfdashboard_actor_install_stylable_property(actorClass, XfdashboardButtonProperties[PROP_TEXT_SINGLE_LINE]);
	xfdashboard_actor_install_stylable_property(actorClass, XfdashboardButtonProperties[PROP_TEXT_JUSTIFY]);

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
static void xfdashboard_button_init(XfdashboardButton *self)
{
	XfdashboardButtonPrivate	*priv;

	priv=self->priv=XFDASHBOARD_BUTTON_GET_PRIVATE(self);

	/* This actor reacts on events */
	clutter_actor_set_reactive(CLUTTER_ACTOR(self), TRUE);

	/* Set up default values */
	priv->padding=0.0f;
	priv->spacing=0.0f;
	priv->style=-1;
	priv->iconName=NULL;
	priv->iconImage=NULL;
	priv->iconSyncSize=TRUE;
	priv->iconSize=16;
	priv->iconOrientation=-1;
	priv->font=NULL;
	priv->labelColor=NULL;
	priv->labelEllipsize=-1;
	priv->isSingleLineMode=TRUE;

	/* Create actors */
	priv->actorIcon=clutter_actor_new();
	clutter_actor_add_child(CLUTTER_ACTOR(self), priv->actorIcon);
	clutter_actor_set_reactive(priv->actorIcon, FALSE);

	priv->actorLabel=CLUTTER_TEXT(clutter_text_new());
	clutter_actor_add_child(CLUTTER_ACTOR(self), CLUTTER_ACTOR(priv->actorLabel));
	clutter_actor_set_reactive(CLUTTER_ACTOR(priv->actorLabel), FALSE);
	clutter_text_set_selectable(priv->actorLabel, FALSE);
	clutter_text_set_line_wrap(priv->actorLabel, TRUE);
	clutter_text_set_single_line_mode(priv->actorLabel, priv->isSingleLineMode);

	/* Connect signals */
	g_signal_connect(self, "notify::mapped", G_CALLBACK(_xfdashboard_button_on_mapped_changed), NULL);

	priv->clickAction=xfdashboard_click_action_new();
	clutter_actor_add_action(CLUTTER_ACTOR(self), priv->clickAction);
	g_signal_connect(priv->clickAction, "clicked", G_CALLBACK(_xfdashboard_button_clicked), NULL);
}

/* IMPLEMENTATION: Public API */

/* Create new actor */
ClutterActor* xfdashboard_button_new(void)
{
	return(g_object_new(XFDASHBOARD_TYPE_BUTTON,
						"text", N_(""),
						"button-style", XFDASHBOARD_STYLE_TEXT,
						NULL));
}

ClutterActor* xfdashboard_button_new_with_text(const gchar *inText)
{
	return(g_object_new(XFDASHBOARD_TYPE_BUTTON,
						"text", inText,
						"button-style", XFDASHBOARD_STYLE_TEXT,
						NULL));
}

ClutterActor* xfdashboard_button_new_with_icon(const gchar *inIconName)
{
	return(g_object_new(XFDASHBOARD_TYPE_BUTTON,
						"icon-name", inIconName,
						"button-style", XFDASHBOARD_STYLE_ICON,
						NULL));
}

ClutterActor* xfdashboard_button_new_full(const gchar *inIconName, const gchar *inText)
{
	return(g_object_new(XFDASHBOARD_TYPE_BUTTON,
						"text", inText,
						"icon-name", inIconName,
						"button-style", XFDASHBOARD_STYLE_BOTH,
						NULL));
}

/* Get/set padding of background to text and icon actors */
gfloat xfdashboard_button_get_padding(XfdashboardButton *self)
{
	g_return_val_if_fail(XFDASHBOARD_IS_BUTTON(self), 0);

	return(self->priv->padding);
}

void xfdashboard_button_set_padding(XfdashboardButton *self, const gfloat inPadding)
{
	XfdashboardButtonPrivate	*priv;

	g_return_if_fail(XFDASHBOARD_IS_BUTTON(self));
	g_return_if_fail(inPadding>=0.0f);

	priv=self->priv;

	/* Set value if changed */
	if(priv->padding!=inPadding)
	{
		/* Set value */
		priv->padding=inPadding;
		clutter_actor_queue_relayout(CLUTTER_ACTOR(self));

		/* Update actor */
		xfdashboard_background_set_corner_radius(XFDASHBOARD_BACKGROUND(self), priv->padding);

		/* Notify about property change */
		g_object_notify_by_pspec(G_OBJECT(self), XfdashboardButtonProperties[PROP_PADDING]);
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
	XfdashboardButtonPrivate	*priv;

	g_return_if_fail(XFDASHBOARD_IS_BUTTON(self));
	g_return_if_fail(inSpacing>=0.0f);

	priv=self->priv;

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
	XfdashboardButtonPrivate	*priv;

	g_return_if_fail(XFDASHBOARD_IS_BUTTON(self));

	priv=self->priv;

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
	XfdashboardButtonPrivate	*priv;
	ClutterImage				*image;

	g_return_if_fail(XFDASHBOARD_IS_BUTTON(self));
	g_return_if_fail(inIconName);

	priv=self->priv;

	/* Set value if changed */
	if(priv->iconImage || g_strcmp0(priv->iconName, inIconName)!=0)
	{
		/* Set value */
		if(priv->iconName) g_free(priv->iconName);
		priv->iconName=g_strdup(inIconName);
		priv->iconNameLoaded=FALSE;

		if(priv->iconImage)
		{
			clutter_actor_set_content(priv->actorIcon, NULL);
			g_object_unref(priv->iconImage);
			priv->iconImage=NULL;
		}

		if(CLUTTER_ACTOR_IS_MAPPED(self))
		{
			/* Actor is mapped so we cannot defer loading and setting image */
			image=xfdashboard_image_content_new_for_icon_name(priv->iconName, priv->iconSize);
			clutter_actor_set_content(priv->actorIcon, CLUTTER_CONTENT(image));
			g_object_unref(image);

			priv->iconNameLoaded=TRUE;
		}

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
	XfdashboardButtonPrivate	*priv;

	g_return_if_fail(XFDASHBOARD_IS_BUTTON(self));
	g_return_if_fail(CLUTTER_IS_IMAGE(inIconImage));

	priv=self->priv;

	/* Set value if changed */
	if(priv->iconName || inIconImage!=priv->iconImage)
	{
		/* Set value */
		if(priv->iconName) g_free(priv->iconName);
		priv->iconName=NULL;
		priv->iconNameLoaded=FALSE;

		if(priv->iconImage)
		{
			clutter_actor_set_content(CLUTTER_ACTOR(priv->actorIcon), NULL);
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
	XfdashboardButtonPrivate	*priv;

	g_return_if_fail(XFDASHBOARD_IS_BUTTON(self));
	g_return_if_fail(inSize==-1 || inSize>0);

	priv=self->priv;

	/* Set value if changed */
	if(priv->iconSize!=inSize)
	{
		/* Set value */
		priv->iconSize=inSize;

		if(priv->iconName)
		{
			ClutterImage		*image;

			image=xfdashboard_image_content_new_for_icon_name(priv->iconName, priv->iconSize);
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
	XfdashboardButtonPrivate	*priv;

	g_return_if_fail(XFDASHBOARD_IS_BUTTON(self));

	priv=self->priv;

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
	XfdashboardButtonPrivate	*priv;

	g_return_if_fail(XFDASHBOARD_IS_BUTTON(self));

	priv=self->priv;

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
	XfdashboardButtonPrivate	*priv;

	g_return_if_fail(XFDASHBOARD_IS_BUTTON(self));

	priv=self->priv;

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
	XfdashboardButtonPrivate	*priv;

	g_return_if_fail(XFDASHBOARD_IS_BUTTON(self));

	priv=self->priv;

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
	XfdashboardButtonPrivate	*priv;

	g_return_if_fail(XFDASHBOARD_IS_BUTTON(self));
	g_return_if_fail(inColor);

	priv=self->priv;

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
	XfdashboardButtonPrivate	*priv;

	g_return_if_fail(XFDASHBOARD_IS_BUTTON(self));

	priv=self->priv;

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

/* Get/set single line mode */
gboolean xfdashboard_button_get_single_line_mode(XfdashboardButton *self)
{
	g_return_val_if_fail(XFDASHBOARD_IS_BUTTON(self), FALSE);

	return(self->priv->isSingleLineMode);
}

void xfdashboard_button_set_single_line_mode(XfdashboardButton *self, const gboolean inSingleLineMode)
{
	XfdashboardButtonPrivate	*priv;

	g_return_if_fail(XFDASHBOARD_IS_BUTTON(self));

	priv=self->priv;

	/* Set value if changed */
	if(priv->isSingleLineMode!=inSingleLineMode)
	{
		/* Set value */
		priv->isSingleLineMode=inSingleLineMode;

		clutter_text_set_single_line_mode(priv->actorLabel, priv->isSingleLineMode);
		clutter_actor_queue_relayout(CLUTTER_ACTOR(self));

		/* Notify about property change */
		g_object_notify_by_pspec(G_OBJECT(self), XfdashboardButtonProperties[PROP_TEXT_SINGLE_LINE]);
	}
}

/* Get/set justification (line alignment) of label */
PangoAlignment xfdashboard_button_get_text_justification(XfdashboardButton *self)
{
	g_return_val_if_fail(XFDASHBOARD_IS_BUTTON(self), PANGO_ALIGN_LEFT);

	return(self->priv->textJustification);
}

void xfdashboard_button_set_text_justification(XfdashboardButton *self, const PangoAlignment inJustification)
{
	XfdashboardButtonPrivate	*priv;

	g_return_if_fail(XFDASHBOARD_IS_BUTTON(self));

	priv=self->priv;

	/* Set value if changed */
	if(priv->textJustification!=inJustification)
	{
		/* Set value */
		priv->textJustification=inJustification;

		clutter_text_set_line_alignment(priv->actorLabel, priv->textJustification);
		clutter_actor_queue_redraw(CLUTTER_ACTOR(self));

		/* Notify about property change */
		g_object_notify_by_pspec(G_OBJECT(self), XfdashboardButtonProperties[PROP_TEXT_SINGLE_LINE]);
	}
}
