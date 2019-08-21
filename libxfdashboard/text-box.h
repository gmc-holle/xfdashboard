/*
 * textbox: An actor representing an editable or read-only text-box
 *          with optinal icons
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

#ifndef __LIBXFDASHBOARD_TEXT_BOX__
#define __LIBXFDASHBOARD_TEXT_BOX__

#if !defined(__LIBXFDASHBOARD_H_INSIDE__) && !defined(LIBXFDASHBOARD_COMPILATION)
#error "Only <libxfdashboard/libxfdashboard.h> can be included directly."
#endif

#include <libxfdashboard/background.h>

G_BEGIN_DECLS

#define XFDASHBOARD_TYPE_TEXT_BOX				(xfdashboard_text_box_get_type())
#define XFDASHBOARD_TEXT_BOX(obj)				(G_TYPE_CHECK_INSTANCE_CAST((obj), XFDASHBOARD_TYPE_TEXT_BOX, XfdashboardTextBox))
#define XFDASHBOARD_IS_TEXT_BOX(obj)			(G_TYPE_CHECK_INSTANCE_TYPE((obj), XFDASHBOARD_TYPE_TEXT_BOX))
#define XFDASHBOARD_TEXT_BOX_CLASS(klass)		(G_TYPE_CHECK_CLASS_CAST((klass), XFDASHBOARD_TYPE_TEXT_BOX, XfdashboardTextBoxClass))
#define XFDASHBOARD_IS_TEXT_BOX_CLASS(klass)	(G_TYPE_CHECK_CLASS_TYPE((klass), XFDASHBOARD_TYPE_TEXT_BOX))
#define XFDASHBOARD_TEXT_BOX_GET_CLASS(obj)		(G_TYPE_INSTANCE_GET_CLASS((obj), XFDASHBOARD_TYPE_TEXT_BOX, XfdashboardTextBoxClass))

typedef struct _XfdashboardTextBox				XfdashboardTextBox;
typedef struct _XfdashboardTextBoxClass			XfdashboardTextBoxClass;
typedef struct _XfdashboardTextBoxPrivate		XfdashboardTextBoxPrivate;

struct _XfdashboardTextBox
{
	/*< private >*/
	/* Parent instance */
	XfdashboardBackground			parent_instance;

	/* Private structure */
	XfdashboardTextBoxPrivate		*priv;
};

struct _XfdashboardTextBoxClass
{
	/*< private >*/
	/* Parent class */
	XfdashboardBackgroundClass		parent_class;

	/*< public >*/
	/* Virtual functions */
	void (*text_changed)(XfdashboardTextBox *self, gchar *inText);
	
	void (*primary_icon_clicked)(XfdashboardTextBox *self);
	void (*secondary_icon_clicked)(XfdashboardTextBox *self);
};

/* Public API */
GType xfdashboard_text_box_get_type(void) G_GNUC_CONST;

ClutterActor* xfdashboard_text_box_new(void);

gfloat xfdashboard_text_box_get_padding(XfdashboardTextBox *self);
void xfdashboard_text_box_set_padding(XfdashboardTextBox *self, gfloat inPadding);

gfloat xfdashboard_text_box_get_spacing(XfdashboardTextBox *self);
void xfdashboard_text_box_set_spacing(XfdashboardTextBox *self, gfloat inSpacing);

gboolean xfdashboard_text_box_get_editable(XfdashboardTextBox *self);
void xfdashboard_text_box_set_editable(XfdashboardTextBox *self, gboolean isEditable);

gboolean xfdashboard_text_box_is_empty(XfdashboardTextBox *self);
gint xfdashboard_text_box_get_length(XfdashboardTextBox *self);
const gchar* xfdashboard_text_box_get_text(XfdashboardTextBox *self);
void xfdashboard_text_box_set_text(XfdashboardTextBox *self, const gchar *inMarkupText);
void xfdashboard_text_box_set_text_va(XfdashboardTextBox *self, const gchar *inFormat, ...) G_GNUC_PRINTF(2, 3);

const gchar* xfdashboard_text_box_get_text_font(XfdashboardTextBox *self);
void xfdashboard_text_box_set_text_font(XfdashboardTextBox *self, const gchar *inFont);

const ClutterColor* xfdashboard_text_box_get_text_color(XfdashboardTextBox *self);
void xfdashboard_text_box_set_text_color(XfdashboardTextBox *self, const ClutterColor *inColor);

const ClutterColor* xfdashboard_text_box_get_selection_text_color(XfdashboardTextBox *self);
void xfdashboard_text_box_set_selection_text_color(XfdashboardTextBox *self, const ClutterColor *inColor);

const ClutterColor* xfdashboard_text_box_get_selection_background_color(XfdashboardTextBox *self);
void xfdashboard_text_box_set_selection_background_color(XfdashboardTextBox *self, const ClutterColor *inColor);

gboolean xfdashboard_text_box_is_hint_text_set(XfdashboardTextBox *self);
const gchar* xfdashboard_text_box_get_hint_text(XfdashboardTextBox *self);
void xfdashboard_text_box_set_hint_text(XfdashboardTextBox *self, const gchar *inMarkupText);
void xfdashboard_text_box_set_hint_text_va(XfdashboardTextBox *self, const gchar *inFormat, ...) G_GNUC_PRINTF(2, 3);

const gchar* xfdashboard_text_box_get_hint_text_font(XfdashboardTextBox *self);
void xfdashboard_text_box_set_hint_text_font(XfdashboardTextBox *self, const gchar *inFont);

const ClutterColor* xfdashboard_text_box_get_hint_text_color(XfdashboardTextBox *self);
void xfdashboard_text_box_set_hint_text_color(XfdashboardTextBox *self, const ClutterColor *inColor);

const gchar* xfdashboard_text_box_get_primary_icon(XfdashboardTextBox *self);
void xfdashboard_text_box_set_primary_icon(XfdashboardTextBox *self, const gchar *inIconName);

const gchar* xfdashboard_text_box_get_secondary_icon(XfdashboardTextBox *self);
void xfdashboard_text_box_set_secondary_icon(XfdashboardTextBox *self, const gchar *inIconName);

G_END_DECLS

#endif	/* __LIBXFDASHBOARD_TEXT_BOX__ */
