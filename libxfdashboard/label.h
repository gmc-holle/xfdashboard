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

#ifndef __LIBXFDASHBOARD_LABEL__
#define __LIBXFDASHBOARD_LABEL__

#if !defined(__LIBXFDASHBOARD_H_INSIDE__) && !defined(LIBXFDASHBOARD_COMPILATION)
#error "Only <libxfdashboard/libxfdashboard.h> can be included directly."
#endif

#include <clutter/clutter.h>

#include <libxfdashboard/background.h>
#include <libxfdashboard/types.h>

G_BEGIN_DECLS

/* Public definitions */
/**
 * XfdashboardLabelStyle:
 * @XFDASHBOARD_LABEL_STYLE_TEXT: The actor will show only text labels.
 * @XFDASHBOARD_LABEL_STYLE_ICON: The actor will show only icons.
 * @XFDASHBOARD_LABEL_STYLE_BOTH: The actor will show both, text labels and icons.
 *
 * Determines the style of an actor, e.g. text labels and icons at labels.
 */
typedef enum /*< prefix=XFDASHBOARD_LABEL_STYLE >*/
{
	XFDASHBOARD_LABEL_STYLE_TEXT=0,
	XFDASHBOARD_LABEL_STYLE_ICON,
	XFDASHBOARD_LABEL_STYLE_BOTH
} XfdashboardLabelStyle;


/* Object declaration */
#define XFDASHBOARD_TYPE_LABEL					(xfdashboard_label_get_type())
#define XFDASHBOARD_LABEL(obj)					(G_TYPE_CHECK_INSTANCE_CAST((obj), XFDASHBOARD_TYPE_LABEL, XfdashboardLabel))
#define XFDASHBOARD_IS_LABEL(obj)				(G_TYPE_CHECK_INSTANCE_TYPE((obj), XFDASHBOARD_TYPE_LABEL))
#define XFDASHBOARD_LABEL_CLASS(klass)			(G_TYPE_CHECK_CLASS_CAST((klass), XFDASHBOARD_TYPE_LABEL, XfdashboardLabelClass))
#define XFDASHBOARD_IS_LABEL_CLASS(klass)		(G_TYPE_CHECK_CLASS_TYPE((klass), XFDASHBOARD_TYPE_LABEL))
#define XFDASHBOARD_LABEL_GET_CLASS(obj)		(G_TYPE_INSTANCE_GET_CLASS((obj), XFDASHBOARD_TYPE_LABEL, XfdashboardLabelClass))

typedef struct _XfdashboardLabel				XfdashboardLabel;
typedef struct _XfdashboardLabelClass			XfdashboardLabelClass;
typedef struct _XfdashboardLabelPrivate			XfdashboardLabelPrivate;

struct _XfdashboardLabel
{
	/*< private >*/
	/* Parent instance */
	XfdashboardBackground		parent_instance;

	/* Private structure */
	XfdashboardLabelPrivate	*priv;
};

struct _XfdashboardLabelClass
{
	/*< private >*/
	/* Parent class */
	XfdashboardBackgroundClass	parent_class;

	/*< public >*/
	/* Virtual functions */
	void (*clicked)(XfdashboardLabel *self);
};

/* Public API */
GType xfdashboard_label_get_type(void) G_GNUC_CONST;

ClutterActor* xfdashboard_label_new(void);
ClutterActor* xfdashboard_label_new_with_text(const gchar *inText);
ClutterActor* xfdashboard_label_new_with_icon_name(const gchar *inIconName);
ClutterActor* xfdashboard_label_new_with_gicon(GIcon *inIcon);
ClutterActor* xfdashboard_label_new_full_with_icon_name(const gchar *inIconName, const gchar *inText);
ClutterActor* xfdashboard_label_new_full_with_gicon(GIcon *inIcon, const gchar *inText);

/* General functions */
gfloat xfdashboard_label_get_padding(XfdashboardLabel *self);
void xfdashboard_label_set_padding(XfdashboardLabel *self, const gfloat inPadding);

gfloat xfdashboard_label_get_spacing(XfdashboardLabel *self);
void xfdashboard_label_set_spacing(XfdashboardLabel *self, const gfloat inSpacing);

XfdashboardLabelStyle xfdashboard_label_get_style(XfdashboardLabel *self);
void xfdashboard_label_set_style(XfdashboardLabel *self, const XfdashboardLabelStyle inStyle);

/* Icon functions */
const gchar* xfdashboard_label_get_icon_name(XfdashboardLabel *self);
void xfdashboard_label_set_icon_name(XfdashboardLabel *self, const gchar *inIconName);

GIcon* xfdashboard_label_get_gicon(XfdashboardLabel *self);
void xfdashboard_label_set_gicon(XfdashboardLabel *self, GIcon *inIcon);

ClutterImage* xfdashboard_label_get_icon_image(XfdashboardLabel *self);
void xfdashboard_label_set_icon_image(XfdashboardLabel *self, ClutterImage *inIconImage);

gint xfdashboard_label_get_icon_size(XfdashboardLabel *self);
void xfdashboard_label_set_icon_size(XfdashboardLabel *self, gint inSize);

gboolean xfdashboard_label_get_sync_icon_size(XfdashboardLabel *self);
void xfdashboard_label_set_sync_icon_size(XfdashboardLabel *self, gboolean inSync);

XfdashboardOrientation xfdashboard_label_get_icon_orientation(XfdashboardLabel *self);
void xfdashboard_label_set_icon_orientation(XfdashboardLabel *self, const XfdashboardOrientation inOrientation);

/* Label functions */
const gchar* xfdashboard_label_get_text(XfdashboardLabel *self);
void xfdashboard_label_set_text(XfdashboardLabel *self, const gchar *inMarkupText);

const gchar* xfdashboard_label_get_font(XfdashboardLabel *self);
void xfdashboard_label_set_font(XfdashboardLabel *self, const gchar *inFont);

const ClutterColor* xfdashboard_label_get_color(XfdashboardLabel *self);
void xfdashboard_label_set_color(XfdashboardLabel *self, const ClutterColor *inColor);

PangoEllipsizeMode xfdashboard_label_get_ellipsize_mode(XfdashboardLabel *self);
void xfdashboard_label_set_ellipsize_mode(XfdashboardLabel *self, const PangoEllipsizeMode inMode);

gboolean xfdashboard_label_get_single_line_mode(XfdashboardLabel *self);
void xfdashboard_label_set_single_line_mode(XfdashboardLabel *self, const gboolean inSingleLine);

PangoAlignment xfdashboard_label_get_text_justification(XfdashboardLabel *self);
void xfdashboard_label_set_text_justification(XfdashboardLabel *self, const PangoAlignment inJustification);

G_END_DECLS

#endif	/* __LIBXFDASHBOARD_LABEL__ */
