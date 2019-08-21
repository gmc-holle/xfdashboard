/*
 * label: An actor representing a label and an icon (both optional)
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

#include <libxfdashboard/label.h>

#include <glib/gi18n-lib.h>
#include <gdk/gdk.h>
#include <math.h>

#include <libxfdashboard/enums.h>
#include <libxfdashboard/image-content.h>
#include <libxfdashboard/compat.h>


/* Forward declarations */
typedef enum /*< skip,prefix=XFDASHBOARD_LABEL_ICON_TYPE >*/
{
	XFDASHBOARD_LABEL_ICON_TYPE_ICON_NONE,
	XFDASHBOARD_LABEL_ICON_TYPE_ICON_NAME,
	XFDASHBOARD_LABEL_ICON_TYPE_ICON_IMAGE,
	XFDASHBOARD_LABEL_ICON_TYPE_ICON_GICON
} XfdashboardLabelIconType;

/* Define this class in GObject system */
struct _XfdashboardLabelPrivate
{
	/* Properties related */
	gfloat						padding;
	gfloat						spacing;
	XfdashboardLabelStyle		style;

	gchar						*iconName;
	ClutterImage				*iconImage;
	GIcon						*iconGIcon;
	gboolean					iconSyncSize;
	gint						iconSize;
	XfdashboardOrientation		iconOrientation;

	gchar						*font;
	ClutterColor				*labelColor;
	PangoEllipsizeMode			labelEllipsize;
	gboolean					isSingleLineMode;
	PangoAlignment				textJustification;

	/* Instance related */
	ClutterActor				*actorIcon;
	ClutterActor				*actorLabel;

	XfdashboardLabelIconType	iconType;
};

G_DEFINE_TYPE_WITH_PRIVATE(XfdashboardLabel,
							xfdashboard_label,
							XFDASHBOARD_TYPE_BACKGROUND)

/* Properties */
enum
{
	PROP_0,

	PROP_PADDING,
	PROP_SPACING,
	PROP_STYLE,

	PROP_ICON_NAME,
	PROP_ICON_IMAGE,
	PROP_ICON_GICON,
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

static GParamSpec* XfdashboardLabelProperties[PROP_LAST]={ 0, };


/* IMPLEMENTATION: Private variables and methods */

/* Get preferred width of icon and label child actors
 * We do not respect paddings here so if height is given it must be
 * reduced by padding on all affected sides. The returned sizes are also
 * without these paddings.
 */
static void _xfdashboard_label_get_preferred_width_intern(XfdashboardLabel *self,
															gboolean inGetPreferred,
															gfloat inForHeight,
															gfloat *outIconSize,
															gfloat *outLabelSize)
{
	XfdashboardLabelPrivate		*priv;
	gfloat						iconWidth, iconHeight, iconScale;
	gfloat						iconSize, labelSize;
	gfloat						minSize, naturalSize;

	g_return_if_fail(XFDASHBOARD_IS_LABEL(self));

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
		if(clutter_actor_is_visible(priv->actorLabel) &&
			clutter_actor_is_visible(priv->actorIcon) &&
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
				if(clutter_actor_is_visible(priv->actorLabel))
				{
					clutter_actor_get_preferred_width(CLUTTER_ACTOR(priv->actorLabel),
														inForHeight,
														&minSize, &naturalSize);
					labelSize=(inGetPreferred==TRUE ? naturalSize : minSize);
				}

				/* Get size of icon if visible */
				if(clutter_actor_is_visible(priv->actorIcon))
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
		else if(clutter_actor_is_visible(priv->actorLabel) &&
					clutter_actor_is_visible(priv->actorIcon) &&
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
				clutter_text_get_single_line_mode(CLUTTER_TEXT(priv->actorLabel))==FALSE)
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
			if(clutter_actor_is_visible(priv->actorIcon) &&
				(clutter_actor_is_visible(priv->actorLabel)))
			{
				inForHeight-=priv->spacing;
			}
			inForHeight-=2*priv->padding;
			inForHeight=MAX(0.0f, inForHeight);

			/* Get icon size if visible */
			if(clutter_actor_is_visible(priv->actorIcon))
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
			if(clutter_actor_is_visible(priv->actorLabel))
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
static void _xfdashboard_label_get_preferred_height_intern(XfdashboardLabel *self,
															gboolean inGetPreferred,
															gfloat inForWidth,
															gfloat *outIconSize,
															gfloat *outLabelSize)
{
	XfdashboardLabelPrivate		*priv;
	gfloat						iconWidth, iconHeight, iconScale;
	gfloat						iconSize, labelSize;
	gfloat						minSize, naturalSize;

	g_return_if_fail(XFDASHBOARD_IS_LABEL(self));

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
		if(clutter_actor_is_visible(priv->actorLabel) &&
			clutter_actor_is_visible(priv->actorIcon) &&
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
				if(clutter_actor_is_visible(priv->actorIcon))
				{
					clutter_actor_get_preferred_height(CLUTTER_ACTOR(priv->actorIcon),
														inForWidth,
														&minSize, &naturalSize);
					iconSize=(inGetPreferred==TRUE ? naturalSize : minSize);
				}

				if(clutter_actor_is_visible(priv->actorLabel))
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
		else if(clutter_actor_is_visible(priv->actorLabel) &&
					clutter_actor_is_visible(priv->actorIcon) &&
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
				clutter_text_get_single_line_mode(CLUTTER_TEXT(priv->actorLabel))==FALSE)
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
			if(clutter_actor_is_visible(priv->actorIcon) &&
				(clutter_actor_is_visible(priv->actorLabel)))
			{
				inForWidth-=priv->spacing;
			}
			inForWidth-=2*priv->padding;
			inForWidth=MAX(0.0f, inForWidth);

			/* Get icon size if visible */
			if(clutter_actor_is_visible(priv->actorIcon))
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
			if(clutter_actor_is_visible(priv->actorLabel))
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
static void _xfdashboard_label_update_icon_image_size(XfdashboardLabel *self)
{
	XfdashboardLabelPrivate		*priv;
	gfloat						iconWidth, iconHeight;
	gfloat						maxSize;

	g_return_if_fail(XFDASHBOARD_IS_LABEL(self));

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


/* IMPLEMENTATION: ClutterActor */

/* Show all children of this actor */
static void _xfdashboard_label_show_all(ClutterActor *self)
{
	XfdashboardLabelPrivate		*priv=XFDASHBOARD_LABEL(self)->priv;

	if(priv->style==XFDASHBOARD_LABEL_STYLE_ICON ||
		priv->style==XFDASHBOARD_LABEL_STYLE_BOTH)
	{
		clutter_actor_show(CLUTTER_ACTOR(priv->actorIcon));
	}
		else clutter_actor_hide(CLUTTER_ACTOR(priv->actorIcon));

	if(priv->style==XFDASHBOARD_LABEL_STYLE_TEXT ||
		priv->style==XFDASHBOARD_LABEL_STYLE_BOTH)
	{
		clutter_actor_show(CLUTTER_ACTOR(priv->actorLabel));
	}
		else clutter_actor_hide(CLUTTER_ACTOR(priv->actorLabel));

	clutter_actor_show(self);
}

/* Hide all children of this actor */
static void _xfdashboard_label_hide_all(ClutterActor *self)
{
	XfdashboardLabelPrivate		*priv=XFDASHBOARD_LABEL(self)->priv;

	clutter_actor_hide(self);
	clutter_actor_hide(CLUTTER_ACTOR(priv->actorIcon));
	clutter_actor_hide(CLUTTER_ACTOR(priv->actorLabel));
}

/* Get preferred width/height */
static void _xfdashboard_label_get_preferred_height(ClutterActor *inActor,
														gfloat inForWidth,
														gfloat *outMinHeight,
														gfloat *outNaturalHeight)
{
	XfdashboardLabel			*self=XFDASHBOARD_LABEL(inActor);
	XfdashboardLabelPrivate		*priv=self->priv;
	gfloat						minHeight, naturalHeight;
	gfloat						minIconHeight, naturalIconHeight;
	gfloat						minLabelHeight, naturalLabelHeight;
	gfloat						spacing=priv->spacing;

	/* Initialize sizes */
	minHeight=naturalHeight=0.0f;
	minIconHeight=naturalIconHeight=0.0f;
	minLabelHeight=naturalLabelHeight=0.0f;

	/* Calculate sizes for requested one (means which can and will be stored) */
	if(outMinHeight)
	{
		_xfdashboard_label_get_preferred_height_intern(self,
															FALSE,
															inForWidth,
															&minIconHeight,
															&minLabelHeight);
	}

	if(outNaturalHeight)
	{
		_xfdashboard_label_get_preferred_height_intern(self,
															TRUE,
															inForWidth,
															&naturalIconHeight,
															&naturalLabelHeight);
	}

	if(clutter_actor_is_visible(priv->actorLabel)!=TRUE ||
		clutter_actor_is_visible(priv->actorIcon)!=TRUE)
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

static void _xfdashboard_label_get_preferred_width(ClutterActor *inActor,
													gfloat inForHeight,
													gfloat *outMinWidth,
													gfloat *outNaturalWidth)
{
	XfdashboardLabel			*self=XFDASHBOARD_LABEL(inActor);
	XfdashboardLabelPrivate		*priv=self->priv;
	gfloat						minWidth, naturalWidth;
	gfloat						minIconWidth, naturalIconWidth;
	gfloat						minLabelWidth, naturalLabelWidth;
	gfloat						spacing=priv->spacing;

	/* Initialize sizes */
	minWidth=naturalWidth=0.0f;
	minIconWidth=naturalIconWidth=0.0f;
	minLabelWidth=naturalLabelWidth=0.0f;

	/* Calculate sizes for requested one (means which can and will be stored) */
	if(outMinWidth)
	{
		_xfdashboard_label_get_preferred_width_intern(self,
															FALSE,
															inForHeight,
															&minIconWidth,
															&minLabelWidth);
	}

	if(outNaturalWidth)
	{
		_xfdashboard_label_get_preferred_width_intern(self,
															TRUE,
															inForHeight,
															&naturalIconWidth,
															&naturalLabelWidth);
	}

	if(clutter_actor_is_visible(priv->actorLabel)!=TRUE ||
		clutter_actor_is_visible(priv->actorIcon)!=TRUE)
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
static void _xfdashboard_label_allocate(ClutterActor *inActor,
											const ClutterActorBox *inBox,
											ClutterAllocationFlags inFlags)
{
	XfdashboardLabel			*self=XFDASHBOARD_LABEL(inActor);
	XfdashboardLabelPrivate		*priv=self->priv;
	ClutterActorBox				*boxLabel=NULL;
	ClutterActorBox				*boxIcon=NULL;
	gfloat						left, right, top, bottom;
	gfloat						textWidth, textHeight;
	gfloat						iconWidth, iconHeight;
	gfloat						spacing=priv->spacing;

	/* Chain up to store the allocation of the actor */
	CLUTTER_ACTOR_CLASS(xfdashboard_label_parent_class)->allocate(inActor, inBox, inFlags);

	/* Get sizes of children and determine if we need
	 * to add spacing between text and icon. If either
	 * icon or text is not visible reset its size to zero
	 * and also reset spacing to zero.
	 */
	if(!clutter_actor_is_visible(priv->actorIcon) ||
			!clutter_actor_is_visible(priv->actorLabel))
	{
		spacing=0.0f;
	}

	/* Get icon sizes */
	iconWidth=iconHeight=0.0f;
	if(clutter_actor_is_visible(priv->actorIcon))
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
			_xfdashboard_label_get_preferred_height_intern(self,
															TRUE,
															clutter_actor_box_get_width(inBox),
															&iconHeight,
															NULL);
			if(priv->iconSyncSize==TRUE) iconWidth=iconHeight*iconScale;
				else clutter_actor_get_preferred_width(CLUTTER_ACTOR(priv->actorIcon), iconHeight, NULL, &iconWidth);
		}
			else
			{
				_xfdashboard_label_get_preferred_width_intern(self,
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
	if(clutter_actor_is_visible(priv->actorLabel))
	{
		switch(priv->iconOrientation)
		{
			case XFDASHBOARD_ORIENTATION_TOP:
				textWidth=MAX(0.0f, clutter_actor_box_get_width(inBox)-2*priv->padding);

				textHeight=clutter_actor_box_get_height(inBox)-iconHeight-2*priv->padding;
				if(clutter_actor_is_visible(priv->actorIcon)) textHeight-=priv->spacing;
				textHeight=MAX(0.0f, textHeight);

				left=((clutter_actor_box_get_width(inBox)-textWidth)/2.0f);
				right=left+textWidth;
				top=priv->padding+iconHeight+spacing;
				bottom=top+textHeight;
				break;

			case XFDASHBOARD_ORIENTATION_BOTTOM:
				textWidth=MAX(0.0f, clutter_actor_box_get_width(inBox)-2*priv->padding);

				textHeight=clutter_actor_box_get_height(inBox)-iconHeight-2*priv->padding;
				if(clutter_actor_is_visible(priv->actorIcon)) textHeight-=priv->spacing;
				textHeight=MAX(0.0f, textHeight);

				left=((clutter_actor_box_get_width(inBox)-textWidth)/2.0f);
				right=left+textWidth;
				top=priv->padding;
				bottom=top+textHeight;
				break;

			case XFDASHBOARD_ORIENTATION_RIGHT:
				textWidth=clutter_actor_box_get_width(inBox)-iconWidth-2*priv->padding;
				if(clutter_actor_is_visible(priv->actorIcon)) textWidth-=priv->spacing;
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
				if(clutter_actor_is_visible(priv->actorIcon)) textWidth-=priv->spacing;
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
	if(clutter_actor_is_visible(priv->actorIcon))
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

/* Destroy this actor */
static void _xfdashboard_label_destroy(ClutterActor *self)
{
	/* Destroy each child actor when this actor is destroyed */
	XfdashboardLabelPrivate		*priv=XFDASHBOARD_LABEL(self)->priv;

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
	CLUTTER_ACTOR_CLASS(xfdashboard_label_parent_class)->destroy(self);
}

/* IMPLEMENTATION: GObject */

/* Dispose this object */
static void _xfdashboard_label_dispose(GObject *inObject)
{
	/* Release our allocated variables */
	XfdashboardLabel			*self=XFDASHBOARD_LABEL(inObject);
	XfdashboardLabelPrivate		*priv=self->priv;

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
	G_OBJECT_CLASS(xfdashboard_label_parent_class)->dispose(inObject);
}

/* Set/get properties */
static void _xfdashboard_label_set_property(GObject *inObject,
												guint inPropID,
												const GValue *inValue,
												GParamSpec *inSpec)
{
	XfdashboardLabel			*self=XFDASHBOARD_LABEL(inObject);
	
	switch(inPropID)
	{
		case PROP_PADDING:
			xfdashboard_label_set_padding(self, g_value_get_float(inValue));
			break;

		case PROP_SPACING:
			xfdashboard_label_set_spacing(self, g_value_get_float(inValue));
			break;

		case PROP_STYLE:
			xfdashboard_label_set_style(self, g_value_get_enum(inValue));
			break;

		case PROP_ICON_NAME:
			xfdashboard_label_set_icon_name(self, g_value_get_string(inValue));
			break;

		case PROP_ICON_GICON:
			xfdashboard_label_set_gicon(self, G_ICON(g_value_get_object(inValue)));
			break;

		case PROP_ICON_IMAGE:
			xfdashboard_label_set_icon_image(self, g_value_get_object(inValue));
			break;

		case PROP_ICON_SYNC_SIZE:
			xfdashboard_label_set_sync_icon_size(self, g_value_get_boolean(inValue));
			break;

		case PROP_ICON_SIZE:
			xfdashboard_label_set_icon_size(self, g_value_get_uint(inValue));
			break;

		case PROP_ICON_ORIENTATION:
			xfdashboard_label_set_icon_orientation(self, g_value_get_enum(inValue));
			break;

		case PROP_TEXT:
			xfdashboard_label_set_text(self, g_value_get_string(inValue));
			break;

		case PROP_TEXT_FONT:
			xfdashboard_label_set_font(self, g_value_get_string(inValue));
			break;

		case PROP_TEXT_COLOR:
			xfdashboard_label_set_color(self, clutter_value_get_color(inValue));
			break;

		case PROP_TEXT_ELLIPSIZE_MODE:
			xfdashboard_label_set_ellipsize_mode(self, g_value_get_enum(inValue));
			break;

		case PROP_TEXT_SINGLE_LINE:
			xfdashboard_label_set_single_line_mode(self, g_value_get_boolean(inValue));
			break;

		case PROP_TEXT_JUSTIFY:
			xfdashboard_label_set_text_justification(self, g_value_get_enum(inValue));
			break;

		default:
			G_OBJECT_WARN_INVALID_PROPERTY_ID(inObject, inPropID, inSpec);
			break;
	}
}

static void _xfdashboard_label_get_property(GObject *inObject,
												guint inPropID,
												GValue *outValue,
												GParamSpec *inSpec)
{
	XfdashboardLabel			*self=XFDASHBOARD_LABEL(inObject);
	XfdashboardLabelPrivate		*priv=self->priv;

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

		case PROP_ICON_GICON:
			g_value_set_object(outValue, priv->iconGIcon);
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
			g_value_set_string(outValue, clutter_text_get_text(CLUTTER_TEXT(priv->actorLabel)));
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
static void xfdashboard_label_class_init(XfdashboardLabelClass *klass)
{
	XfdashboardActorClass	*actorClass=XFDASHBOARD_ACTOR_CLASS(klass);
	ClutterActorClass		*clutterActorClass=CLUTTER_ACTOR_CLASS(klass);
	GObjectClass			*gobjectClass=G_OBJECT_CLASS(klass);

	/* Override functions */
	gobjectClass->dispose=_xfdashboard_label_dispose;
	gobjectClass->set_property=_xfdashboard_label_set_property;
	gobjectClass->get_property=_xfdashboard_label_get_property;

	clutterActorClass->show_all=_xfdashboard_label_show_all;
	clutterActorClass->hide_all=_xfdashboard_label_hide_all;
	clutterActorClass->get_preferred_width=_xfdashboard_label_get_preferred_width;
	clutterActorClass->get_preferred_height=_xfdashboard_label_get_preferred_height;
	clutterActorClass->allocate=_xfdashboard_label_allocate;
	clutterActorClass->destroy=_xfdashboard_label_destroy;

	/* Define properties */
	XfdashboardLabelProperties[PROP_PADDING]=
		g_param_spec_float("padding",
							_("Padding"),
							_("Padding between background and elements"),
							0.0f, G_MAXFLOAT,
							4.0f,
							G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS | G_PARAM_CONSTRUCT);

	XfdashboardLabelProperties[PROP_SPACING]=
		g_param_spec_float("spacing",
							_("Spacing"),
							_("Spacing between text and icon"),
							0.0f, G_MAXFLOAT,
							4.0f,
							G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS | G_PARAM_CONSTRUCT);

	XfdashboardLabelProperties[PROP_STYLE]=
		g_param_spec_enum("label-style",
							_("Label style"),
							_("Style of button showing text and/or icon"),
							XFDASHBOARD_TYPE_LABEL_STYLE,
							XFDASHBOARD_LABEL_STYLE_TEXT,
							G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS | G_PARAM_CONSTRUCT);

	XfdashboardLabelProperties[PROP_ICON_NAME]=
		g_param_spec_string("icon-name",
							_("Icon name"),
							_("Themed icon name or file name of icon"),
							N_(""),
							G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS);

	XfdashboardLabelProperties[PROP_ICON_GICON]=
		g_param_spec_object("icon-gicon",
							_("Icon GIcon"),
							_("The GIcon of icon"),
							G_TYPE_ICON,
							G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS);

	XfdashboardLabelProperties[PROP_ICON_IMAGE]=
		g_param_spec_object("icon-image",
							_("Icon image"),
							_("Image of icon"),
							CLUTTER_TYPE_IMAGE,
							G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS);

	XfdashboardLabelProperties[PROP_ICON_SYNC_SIZE]=
		g_param_spec_boolean("sync-icon-size",
								_("Synchronize icon size"),
								_("Synchronize icon size with text size"),
								TRUE,
								G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS);

	XfdashboardLabelProperties[PROP_ICON_SIZE]=
		g_param_spec_uint("icon-size",
							_("Icon size"),
							_("Size of icon if size of icon is not synchronized. -1 is valid for icon images and sets icon image's default size."),
							1, G_MAXUINT,
							16,
							G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS);

	XfdashboardLabelProperties[PROP_ICON_ORIENTATION]=
		g_param_spec_enum("icon-orientation",
							_("Icon orientation"),
							_("Orientation of icon to label"),
							XFDASHBOARD_TYPE_ORIENTATION,
							XFDASHBOARD_ORIENTATION_LEFT,
							G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS | G_PARAM_CONSTRUCT);

	XfdashboardLabelProperties[PROP_TEXT]=
		g_param_spec_string("text",
							_("Label text"),
							_("Text of label"),
							N_(""),
							G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS);

	XfdashboardLabelProperties[PROP_TEXT_FONT]=
		g_param_spec_string("font",
							_("Font"),
							_("Font of label"),
							NULL,
							G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS);

	XfdashboardLabelProperties[PROP_TEXT_COLOR]=
		clutter_param_spec_color("color",
									_("Color"),
									_("Color of label"),
									NULL,
									G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS);

	XfdashboardLabelProperties[PROP_TEXT_ELLIPSIZE_MODE]=
		g_param_spec_enum("ellipsize-mode",
							_("Ellipsize mode"),
							_("Mode of ellipsize if text in label is too long"),
							PANGO_TYPE_ELLIPSIZE_MODE,
							PANGO_ELLIPSIZE_MIDDLE,
							G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS | G_PARAM_CONSTRUCT);

	XfdashboardLabelProperties[PROP_TEXT_SINGLE_LINE]=
		g_param_spec_boolean("single-line",
								_("Single line"),
								_("Flag to determine if text can only be in one or multiple lines"),
								TRUE,
								G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS);

	XfdashboardLabelProperties[PROP_TEXT_JUSTIFY]=
		g_param_spec_enum("text-justify",
							_("Text justify"),
							_("Justification (line alignment) of label"),
							PANGO_TYPE_ALIGNMENT,
							PANGO_ALIGN_LEFT,
							G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS);

	g_object_class_install_properties(gobjectClass, PROP_LAST, XfdashboardLabelProperties);

	/* Define stylable properties */
	xfdashboard_actor_install_stylable_property(actorClass, XfdashboardLabelProperties[PROP_PADDING]);
	xfdashboard_actor_install_stylable_property(actorClass, XfdashboardLabelProperties[PROP_SPACING]);
	xfdashboard_actor_install_stylable_property(actorClass, XfdashboardLabelProperties[PROP_STYLE]);
	xfdashboard_actor_install_stylable_property(actorClass, XfdashboardLabelProperties[PROP_ICON_NAME]);
	xfdashboard_actor_install_stylable_property(actorClass, XfdashboardLabelProperties[PROP_ICON_IMAGE]);
	xfdashboard_actor_install_stylable_property(actorClass, XfdashboardLabelProperties[PROP_ICON_SYNC_SIZE]);
	xfdashboard_actor_install_stylable_property(actorClass, XfdashboardLabelProperties[PROP_ICON_SIZE]);
	xfdashboard_actor_install_stylable_property(actorClass, XfdashboardLabelProperties[PROP_ICON_ORIENTATION]);
	xfdashboard_actor_install_stylable_property(actorClass, XfdashboardLabelProperties[PROP_TEXT]);
	xfdashboard_actor_install_stylable_property(actorClass, XfdashboardLabelProperties[PROP_TEXT_FONT]);
	xfdashboard_actor_install_stylable_property(actorClass, XfdashboardLabelProperties[PROP_TEXT_COLOR]);
	xfdashboard_actor_install_stylable_property(actorClass, XfdashboardLabelProperties[PROP_TEXT_ELLIPSIZE_MODE]);
	xfdashboard_actor_install_stylable_property(actorClass, XfdashboardLabelProperties[PROP_TEXT_SINGLE_LINE]);
	xfdashboard_actor_install_stylable_property(actorClass, XfdashboardLabelProperties[PROP_TEXT_JUSTIFY]);
}

/* Object initialization
 * Create private structure and set up default values
 */
static void xfdashboard_label_init(XfdashboardLabel *self)
{
	XfdashboardLabelPrivate	*priv;

	priv=self->priv=xfdashboard_label_get_instance_private(self);

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
	priv->iconType=XFDASHBOARD_LABEL_ICON_TYPE_ICON_NONE;

	/* Create actors */
	priv->actorIcon=clutter_actor_new();
	clutter_actor_add_child(CLUTTER_ACTOR(self), priv->actorIcon);
	clutter_actor_set_reactive(priv->actorIcon, FALSE);

	priv->actorLabel=clutter_text_new();
	clutter_actor_add_child(CLUTTER_ACTOR(self), priv->actorLabel);
	clutter_actor_set_reactive(CLUTTER_ACTOR(priv->actorLabel), FALSE);
	clutter_text_set_selectable(CLUTTER_TEXT(priv->actorLabel), FALSE);
	clutter_text_set_line_wrap(CLUTTER_TEXT(priv->actorLabel), TRUE);
	clutter_text_set_single_line_mode(CLUTTER_TEXT(priv->actorLabel), priv->isSingleLineMode);
}

/* IMPLEMENTATION: Public API */

/* Create new actor */
ClutterActor* xfdashboard_label_new(void)
{
	return(g_object_new(XFDASHBOARD_TYPE_LABEL,
						"text", N_(""),
						"label-style", XFDASHBOARD_LABEL_STYLE_TEXT,
						NULL));
}

ClutterActor* xfdashboard_label_new_with_text(const gchar *inText)
{
	return(g_object_new(XFDASHBOARD_TYPE_LABEL,
						"text", inText,
						"label-style", XFDASHBOARD_LABEL_STYLE_TEXT,
						NULL));
}

ClutterActor* xfdashboard_label_new_with_icon_name(const gchar *inIconName)
{
	return(g_object_new(XFDASHBOARD_TYPE_LABEL,
						"icon-name", inIconName,
						"label-style", XFDASHBOARD_LABEL_STYLE_ICON,
						NULL));
}

ClutterActor* xfdashboard_label_new_with_gicon(GIcon *inIcon)
{
	return(g_object_new(XFDASHBOARD_TYPE_LABEL,
						"icon-gicon", inIcon,
						"label-style", XFDASHBOARD_LABEL_STYLE_ICON,
						NULL));
}

ClutterActor* xfdashboard_label_new_full_with_icon_name(const gchar *inIconName, const gchar *inText)
{
	return(g_object_new(XFDASHBOARD_TYPE_LABEL,
						"text", inText,
						"icon-name", inIconName,
						"label-style", XFDASHBOARD_LABEL_STYLE_BOTH,
						NULL));
}

ClutterActor* xfdashboard_label_new_full_with_gicon(GIcon *inIcon, const gchar *inText)
{
	return(g_object_new(XFDASHBOARD_TYPE_LABEL,
						"text", inText,
						"icon-gicon", inIcon,
						"label-style", XFDASHBOARD_LABEL_STYLE_BOTH,
						NULL));
}

/* Get/set padding of background to text and icon actors */
gfloat xfdashboard_label_get_padding(XfdashboardLabel *self)
{
	g_return_val_if_fail(XFDASHBOARD_IS_LABEL(self), 0);

	return(self->priv->padding);
}

void xfdashboard_label_set_padding(XfdashboardLabel *self, const gfloat inPadding)
{
	XfdashboardLabelPrivate		*priv;

	g_return_if_fail(XFDASHBOARD_IS_LABEL(self));
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
		g_object_notify_by_pspec(G_OBJECT(self), XfdashboardLabelProperties[PROP_PADDING]);
	}
}

/* Get/set spacing between text and icon actors */
gfloat xfdashboard_label_get_spacing(XfdashboardLabel *self)
{
	g_return_val_if_fail(XFDASHBOARD_IS_LABEL(self), 0);

	return(self->priv->spacing);
}

void xfdashboard_label_set_spacing(XfdashboardLabel *self, const gfloat inSpacing)
{
	XfdashboardLabelPrivate		*priv;

	g_return_if_fail(XFDASHBOARD_IS_LABEL(self));
	g_return_if_fail(inSpacing>=0.0f);

	priv=self->priv;

	/* Set value if changed */
	if(priv->spacing!=inSpacing)
	{
		/* Set value */
		priv->spacing=inSpacing;
		clutter_actor_queue_relayout(CLUTTER_ACTOR(self));

		/* Notify about property change */
		g_object_notify_by_pspec(G_OBJECT(self), XfdashboardLabelProperties[PROP_SPACING]);
	}
}

/* Get/set style of button */
XfdashboardLabelStyle xfdashboard_label_get_style(XfdashboardLabel *self)
{
	g_return_val_if_fail(XFDASHBOARD_IS_LABEL(self), XFDASHBOARD_LABEL_STYLE_TEXT);

	return(self->priv->style);
}

void xfdashboard_label_set_style(XfdashboardLabel *self, const XfdashboardLabelStyle inStyle)
{
	XfdashboardLabelPrivate		*priv;

	g_return_if_fail(XFDASHBOARD_IS_LABEL(self));

	priv=self->priv;

	/* Set value if changed */
	if(priv->style!=inStyle)
	{
		/* Set value */
		priv->style=inStyle;

		/* Show actors depending on style */
		if(priv->style==XFDASHBOARD_LABEL_STYLE_TEXT ||
			priv->style==XFDASHBOARD_LABEL_STYLE_BOTH)
		{
			clutter_actor_show(CLUTTER_ACTOR(priv->actorLabel));
		}
			else clutter_actor_hide(CLUTTER_ACTOR(priv->actorLabel));

		if(priv->style==XFDASHBOARD_LABEL_STYLE_ICON ||
			priv->style==XFDASHBOARD_LABEL_STYLE_BOTH)
		{
			clutter_actor_show(CLUTTER_ACTOR(priv->actorIcon));
		}
			else clutter_actor_hide(CLUTTER_ACTOR(priv->actorIcon));

		clutter_actor_queue_relayout(CLUTTER_ACTOR(self));

		/* Notify about property change */
		g_object_notify_by_pspec(G_OBJECT(self), XfdashboardLabelProperties[PROP_STYLE]);
	}
}

/* Get/set icon */
const gchar* xfdashboard_label_get_icon_name(XfdashboardLabel *self)
{
	g_return_val_if_fail(XFDASHBOARD_IS_LABEL(self), NULL);

	return(self->priv->iconName);
}

void xfdashboard_label_set_icon_name(XfdashboardLabel *self, const gchar *inIconName)
{
	XfdashboardLabelPrivate		*priv;
	ClutterContent				*image;

	g_return_if_fail(XFDASHBOARD_IS_LABEL(self));
	g_return_if_fail(inIconName);

	priv=self->priv;

	/* Set value if changed */
	if(priv->iconType!=XFDASHBOARD_LABEL_ICON_TYPE_ICON_NAME ||
		g_strcmp0(priv->iconName, inIconName)!=0)
	{
		/* Release old values and icons */
		if(priv->iconName)
		{
			g_free(priv->iconName);
			priv->iconName=NULL;
		}

		if(priv->iconGIcon)
		{
			g_object_unref(priv->iconGIcon);
			priv->iconGIcon=NULL;
		}

		if(priv->iconImage)
		{
			g_object_unref(priv->iconImage);
			priv->iconImage=NULL;
		}

		/* Set value */
		priv->iconName=g_strdup(inIconName);
		priv->iconType=XFDASHBOARD_LABEL_ICON_TYPE_ICON_NAME;

		/* Setup icon image */
		image=xfdashboard_image_content_new_for_icon_name(priv->iconName, priv->iconSize);
		clutter_actor_set_content(priv->actorIcon, image);
		g_object_unref(image);

		_xfdashboard_label_update_icon_image_size(self);

		/* Notify about property change */
		g_object_notify_by_pspec(G_OBJECT(self), XfdashboardLabelProperties[PROP_ICON_NAME]);
	}
}

GIcon* xfdashboard_label_get_gicon(XfdashboardLabel *self)
{
	g_return_val_if_fail(XFDASHBOARD_IS_LABEL(self), NULL);

	return(self->priv->iconGIcon);
}

void xfdashboard_label_set_gicon(XfdashboardLabel *self, GIcon *inIcon)
{
	XfdashboardLabelPrivate		*priv;
	ClutterContent				*image;

	g_return_if_fail(XFDASHBOARD_IS_LABEL(self));
	g_return_if_fail(G_IS_ICON(inIcon));

	priv=self->priv;

	/* Set value if changed */
	if(priv->iconType!=XFDASHBOARD_LABEL_ICON_TYPE_ICON_GICON ||
		!g_icon_equal(priv->iconGIcon, inIcon))
	{
		/* Release old values and icons */
		if(priv->iconName)
		{
			g_free(priv->iconName);
			priv->iconName=NULL;
		}

		if(priv->iconGIcon)
		{
			g_object_unref(priv->iconGIcon);
			priv->iconGIcon=NULL;
		}

		if(priv->iconImage)
		{
			g_object_unref(priv->iconImage);
			priv->iconImage=NULL;
		}

		/* Set value */
		priv->iconGIcon=G_ICON(g_object_ref(inIcon));
		priv->iconType=XFDASHBOARD_LABEL_ICON_TYPE_ICON_GICON;

		/* Setup icon image */
		image=xfdashboard_image_content_new_for_gicon(priv->iconGIcon, priv->iconSize);
		clutter_actor_set_content(priv->actorIcon, image);
		g_object_unref(image);

		_xfdashboard_label_update_icon_image_size(self);

		/* Notify about property change */
		g_object_notify_by_pspec(G_OBJECT(self), XfdashboardLabelProperties[PROP_ICON_GICON]);
	}
}

ClutterImage* xfdashboard_label_get_icon_image(XfdashboardLabel *self)
{
	g_return_val_if_fail(XFDASHBOARD_IS_LABEL(self), NULL);

	return(self->priv->iconImage);
}

void xfdashboard_label_set_icon_image(XfdashboardLabel *self, ClutterImage *inIconImage)
{
	XfdashboardLabelPrivate		*priv;

	g_return_if_fail(XFDASHBOARD_IS_LABEL(self));
	g_return_if_fail(CLUTTER_IS_IMAGE(inIconImage));

	priv=self->priv;

	/* Set value if changed */
	if(priv->iconType!=XFDASHBOARD_LABEL_ICON_TYPE_ICON_IMAGE ||
		inIconImage!=priv->iconImage)
	{
		/* Release old values and icons */
		if(priv->iconName)
		{
			g_free(priv->iconName);
			priv->iconName=NULL;
		}

		if(priv->iconGIcon)
		{
			g_object_unref(priv->iconGIcon);
			priv->iconGIcon=NULL;
		}

		if(priv->iconImage)
		{
			g_object_unref(priv->iconImage);
			priv->iconImage=NULL;
		}

		/* Set value */
		priv->iconImage=g_object_ref(inIconImage);
		priv->iconType=XFDASHBOARD_LABEL_ICON_TYPE_ICON_IMAGE;

		/* Setup icon image */
		clutter_actor_set_content(priv->actorIcon, CLUTTER_CONTENT(priv->iconImage));

		_xfdashboard_label_update_icon_image_size(self);

		/* Notify about property change */
		g_object_notify_by_pspec(G_OBJECT(self), XfdashboardLabelProperties[PROP_ICON_IMAGE]);
	}
}

/* Get/set size of icon */
gint xfdashboard_label_get_icon_size(XfdashboardLabel *self)
{
	g_return_val_if_fail(XFDASHBOARD_IS_LABEL(self), 0);

	return(self->priv->iconSize);
}

void xfdashboard_label_set_icon_size(XfdashboardLabel *self, gint inSize)
{
	XfdashboardLabelPrivate		*priv;

	g_return_if_fail(XFDASHBOARD_IS_LABEL(self));
	g_return_if_fail(inSize==-1 || inSize>0);

	priv=self->priv;

	/* Set value if changed */
	if(priv->iconSize!=inSize)
	{
		/* Set value */
		priv->iconSize=inSize;

		/* Setup icon image */
		if(priv->iconType==XFDASHBOARD_LABEL_ICON_TYPE_ICON_NAME)
		{
			ClutterContent		*image;

			image=xfdashboard_image_content_new_for_icon_name(priv->iconName, priv->iconSize);
			clutter_actor_set_content(priv->actorIcon, image);
			g_object_unref(image);
		}

		if(priv->iconType==XFDASHBOARD_LABEL_ICON_TYPE_ICON_GICON)
		{
			ClutterContent		*image;

			image=xfdashboard_image_content_new_for_gicon(priv->iconGIcon, priv->iconSize);
			clutter_actor_set_content(priv->actorIcon, image);
			g_object_unref(image);
		}

		_xfdashboard_label_update_icon_image_size(self);

		/* Notify about property change */
		g_object_notify_by_pspec(G_OBJECT(self), XfdashboardLabelProperties[PROP_ICON_SIZE]);
	}
}

/* Get/set state if icon size will be synchronized */
gboolean xfdashboard_label_get_sync_icon_size(XfdashboardLabel *self)
{
	g_return_val_if_fail(XFDASHBOARD_IS_LABEL(self), FALSE);

	return(self->priv->iconSyncSize);
}

void xfdashboard_label_set_sync_icon_size(XfdashboardLabel *self, gboolean inSync)
{
	XfdashboardLabelPrivate		*priv;

	g_return_if_fail(XFDASHBOARD_IS_LABEL(self));

	priv=self->priv;

	/* Set value if changed */
	if(priv->iconSyncSize!=inSync)
	{
		/* Set value */
		priv->iconSyncSize=inSync;

		_xfdashboard_label_update_icon_image_size(self);

		/* Notify about property change */
		g_object_notify_by_pspec(G_OBJECT(self), XfdashboardLabelProperties[PROP_ICON_SYNC_SIZE]);
	}
}

/* Get/set orientation of icon to label */
XfdashboardOrientation xfdashboard_label_get_icon_orientation(XfdashboardLabel *self)
{
	g_return_val_if_fail(XFDASHBOARD_IS_LABEL(self), XFDASHBOARD_ORIENTATION_LEFT);

	return(self->priv->iconOrientation);
}

void xfdashboard_label_set_icon_orientation(XfdashboardLabel *self, const XfdashboardOrientation inOrientation)
{
	XfdashboardLabelPrivate		*priv;

	g_return_if_fail(XFDASHBOARD_IS_LABEL(self));

	priv=self->priv;

	/* Set value if changed */
	if(priv->iconOrientation!=inOrientation)
	{
		/* Set value */
		priv->iconOrientation=inOrientation;

		clutter_actor_queue_relayout(CLUTTER_ACTOR(self));

		/* Notify about property change */
		g_object_notify_by_pspec(G_OBJECT(self), XfdashboardLabelProperties[PROP_ICON_ORIENTATION]);
	}
}

/* Get/set text of label */
const gchar* xfdashboard_label_get_text(XfdashboardLabel *self)
{
	g_return_val_if_fail(XFDASHBOARD_IS_LABEL(self), NULL);

	return(clutter_text_get_text(CLUTTER_TEXT(self->priv->actorLabel)));
}

void xfdashboard_label_set_text(XfdashboardLabel *self, const gchar *inMarkupText)
{
	XfdashboardLabelPrivate		*priv;

	g_return_if_fail(XFDASHBOARD_IS_LABEL(self));

	priv=self->priv;

	/* Set value if changed */
	if(g_strcmp0(clutter_text_get_text(CLUTTER_TEXT(priv->actorLabel)), inMarkupText)!=0)
	{
		/* Set value */
		clutter_text_set_markup(CLUTTER_TEXT(priv->actorLabel), inMarkupText);
		clutter_actor_queue_relayout(CLUTTER_ACTOR(priv->actorLabel));

		/* Notify about property change */
		g_object_notify_by_pspec(G_OBJECT(self), XfdashboardLabelProperties[PROP_TEXT]);
	}
}

/* Get/set font of label */
const gchar* xfdashboard_label_get_font(XfdashboardLabel *self)
{
	g_return_val_if_fail(XFDASHBOARD_IS_LABEL(self), NULL);

	if(self->priv->actorLabel) return(self->priv->font);
	return(NULL);
}

void xfdashboard_label_set_font(XfdashboardLabel *self, const gchar *inFont)
{
	XfdashboardLabelPrivate		*priv;

	g_return_if_fail(XFDASHBOARD_IS_LABEL(self));

	priv=self->priv;

	/* Set value if changed */
	if(g_strcmp0(priv->font, inFont)!=0)
	{
		/* Set value */
		if(priv->font) g_free(priv->font);
		priv->font=(inFont ? g_strdup(inFont) : NULL);

		clutter_text_set_font_name(CLUTTER_TEXT(priv->actorLabel), priv->font);
		clutter_actor_queue_redraw(CLUTTER_ACTOR(self));

		/* Notify about property change */
		g_object_notify_by_pspec(G_OBJECT(self), XfdashboardLabelProperties[PROP_TEXT_FONT]);
	}
}

/* Get/set color of text in label */
const ClutterColor* xfdashboard_label_get_color(XfdashboardLabel *self)
{
	g_return_val_if_fail(XFDASHBOARD_IS_LABEL(self), NULL);

	return(self->priv->labelColor);
}

void xfdashboard_label_set_color(XfdashboardLabel *self, const ClutterColor *inColor)
{
	XfdashboardLabelPrivate		*priv;

	g_return_if_fail(XFDASHBOARD_IS_LABEL(self));
	g_return_if_fail(inColor);

	priv=self->priv;

	/* Set value if changed */
	if(!priv->labelColor || !clutter_color_equal(inColor, priv->labelColor))
	{
		/* Set value */
		if(priv->labelColor) clutter_color_free(priv->labelColor);
		priv->labelColor=clutter_color_copy(inColor);

		clutter_text_set_color(CLUTTER_TEXT(priv->actorLabel), priv->labelColor);
		clutter_actor_queue_redraw(CLUTTER_ACTOR(self));

		/* Notify about property change */
		g_object_notify_by_pspec(G_OBJECT(self), XfdashboardLabelProperties[PROP_TEXT_COLOR]);
	}
}

/* Get/set ellipsize mode if label's text is getting too long */
PangoEllipsizeMode xfdashboard_label_get_ellipsize_mode(XfdashboardLabel *self)
{
	g_return_val_if_fail(XFDASHBOARD_IS_LABEL(self), 0);

	return(self->priv->labelEllipsize);
}

void xfdashboard_label_set_ellipsize_mode(XfdashboardLabel *self, const PangoEllipsizeMode inMode)
{
	XfdashboardLabelPrivate		*priv;

	g_return_if_fail(XFDASHBOARD_IS_LABEL(self));

	priv=self->priv;

	/* Set value if changed */
	if(priv->labelEllipsize!=inMode)
	{
		/* Set value */
		priv->labelEllipsize=inMode;

		clutter_text_set_ellipsize(CLUTTER_TEXT(priv->actorLabel), priv->labelEllipsize);
		clutter_actor_queue_relayout(CLUTTER_ACTOR(self));

		/* Notify about property change */
		g_object_notify_by_pspec(G_OBJECT(self), XfdashboardLabelProperties[PROP_TEXT_ELLIPSIZE_MODE]);
	}
}

/* Get/set single line mode */
gboolean xfdashboard_label_get_single_line_mode(XfdashboardLabel *self)
{
	g_return_val_if_fail(XFDASHBOARD_IS_LABEL(self), FALSE);

	return(self->priv->isSingleLineMode);
}

void xfdashboard_label_set_single_line_mode(XfdashboardLabel *self, const gboolean inSingleLineMode)
{
	XfdashboardLabelPrivate		*priv;

	g_return_if_fail(XFDASHBOARD_IS_LABEL(self));

	priv=self->priv;

	/* Set value if changed */
	if(priv->isSingleLineMode!=inSingleLineMode)
	{
		/* Set value */
		priv->isSingleLineMode=inSingleLineMode;

		clutter_text_set_single_line_mode(CLUTTER_TEXT(priv->actorLabel), priv->isSingleLineMode);
		clutter_actor_queue_relayout(CLUTTER_ACTOR(self));

		/* Notify about property change */
		g_object_notify_by_pspec(G_OBJECT(self), XfdashboardLabelProperties[PROP_TEXT_SINGLE_LINE]);
	}
}

/* Get/set justification (line alignment) of label */
PangoAlignment xfdashboard_label_get_text_justification(XfdashboardLabel *self)
{
	g_return_val_if_fail(XFDASHBOARD_IS_LABEL(self), PANGO_ALIGN_LEFT);

	return(self->priv->textJustification);
}

void xfdashboard_label_set_text_justification(XfdashboardLabel *self, const PangoAlignment inJustification)
{
	XfdashboardLabelPrivate		*priv;

	g_return_if_fail(XFDASHBOARD_IS_LABEL(self));

	priv=self->priv;

	/* Set value if changed */
	if(priv->textJustification!=inJustification)
	{
		/* Set value */
		priv->textJustification=inJustification;

		clutter_text_set_line_alignment(CLUTTER_TEXT(priv->actorLabel), priv->textJustification);
		clutter_actor_queue_redraw(CLUTTER_ACTOR(self));

		/* Notify about property change */
		g_object_notify_by_pspec(G_OBJECT(self), XfdashboardLabelProperties[PROP_TEXT_SINGLE_LINE]);
	}
}
