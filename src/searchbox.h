/*
 * searchbox.h: An actor representing an editable text-box with icons
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

#ifndef __XFOVERVIEW_SEARCHBOX__
#define __XFOVERVIEW_SEARCHBOX__

#include "common.h"

#include <clutter/clutter.h>

G_BEGIN_DECLS

#define XFDASHBOARD_TYPE_SEARCHBOX					(xfdashboard_searchbox_get_type())
#define XFDASHBOARD_SEARCHBOX(obj)					(G_TYPE_CHECK_INSTANCE_CAST((obj), XFDASHBOARD_TYPE_SEARCHBOX, XfdashboardSearchBox))
#define XFDASHBOARD_IS_SEARCHBOX(obj)				(G_TYPE_CHECK_INSTANCE_TYPE((obj), XFDASHBOARD_TYPE_SEARCHBOX))
#define XFDASHBOARD_SEARCHBOX_CLASS(klass)			(G_TYPE_CHECK_CLASS_CAST((klass), XFDASHBOARD_TYPE_SEARCHBOX, XfdashboardSearchBoxClass))
#define XFDASHBOARD_IS_SEARCHBOX_CLASS(klass)		(G_TYPE_CHECK_CLASS_TYPE((klass), XFDASHBOARD_TYPE_SEARCHBOX))
#define XFDASHBOARD_SEARCHBOX_GET_CLASS(obj)		(G_TYPE_INSTANCE_GET_CLASS((obj), XFDASHBOARD_TYPE_SEARCHBOX, XfdashboardSearchBoxClass))

typedef struct _XfdashboardSearchBox				XfdashboardSearchBox;
typedef struct _XfdashboardSearchBoxClass			XfdashboardSearchBoxClass;
typedef struct _XfdashboardSearchBoxPrivate			XfdashboardSearchBoxPrivate;

struct _XfdashboardSearchBox
{
	/* Parent instance */
	ClutterActor				parent_instance;

	/* Private structure */
	XfdashboardSearchBoxPrivate	*priv;
};

struct _XfdashboardSearchBoxClass
{
	/* Parent class */
	ClutterActorClass			parent_class;

	/* Virtual functions */
	void (*primary_icon_clicked)(XfdashboardSearchBox *self);
	void (*secondary_icon_clicked)(XfdashboardSearchBox *self);

	void (*search_started)(XfdashboardSearchBox *self);
	void (*search_ended)(XfdashboardSearchBox *self);
	void (*text_changed)(XfdashboardSearchBox *self, gchar *inText);
};

/* Public API */
GType xfdashboard_searchbox_get_type(void) G_GNUC_CONST;

ClutterActor* xfdashboard_searchbox_new();

/* General functions */
const gfloat xfdashboard_searchbox_get_margin(XfdashboardSearchBox *self);
void xfdashboard_searchbox_set_margin(XfdashboardSearchBox *self, const gfloat inMargin);

const gfloat xfdashboard_searchbox_get_spacing(XfdashboardSearchBox *self);
void xfdashboard_searchbox_set_spacing(XfdashboardSearchBox *self, const gfloat inSpacing);

/* Text functions */
gboolean xfdashboard_searchbox_is_empty_text(XfdashboardSearchBox *self);
const gchar* xfdashboard_searchbox_get_text(XfdashboardSearchBox *self);
void xfdashboard_searchbox_set_text(XfdashboardSearchBox *self, const gchar *inMarkupText);

const gchar* xfdashboard_searchbox_get_text_font(XfdashboardSearchBox *self);
void xfdashboard_searchbox_set_text_font(XfdashboardSearchBox *self, const gchar *inFont);

const ClutterColor* xfdashboard_searchbox_get_text_color(XfdashboardSearchBox *self);
void xfdashboard_searchbox_set_text_color(XfdashboardSearchBox *self, const ClutterColor *inColor);

/* Hint text functions */
const gchar* xfdashboard_searchbox_get_hint_text(XfdashboardSearchBox *self);
void xfdashboard_searchbox_set_hint_text(XfdashboardSearchBox *self, const gchar *inMarkupText);

const gchar* xfdashboard_searchbox_get_hint_text_font(XfdashboardSearchBox *self);
void xfdashboard_searchbox_set_hint_text_font(XfdashboardSearchBox *self, const gchar *inFont);

const ClutterColor* xfdashboard_searchbox_get_hint_text_color(XfdashboardSearchBox *self);
void xfdashboard_searchbox_set_hint_text_color(XfdashboardSearchBox *self, const ClutterColor *inColor);

/* Icon functions */
const gchar* xfdashboard_searchbox_get_primary_icon(XfdashboardSearchBox *self);
void xfdashboard_searchbox_set_primary_icon(XfdashboardSearchBox *self, const gchar *inIconName);

const gchar* xfdashboard_searchbox_get_secondary_icon(XfdashboardSearchBox *self);
void xfdashboard_searchbox_set_secondary_icon(XfdashboardSearchBox *self, const gchar *inIconName);

/* Background functions */
gboolean xfdashboard_searchbox_get_background_visibility(XfdashboardSearchBox *self);
void xfdashboard_searchbox_set_background_visibility(XfdashboardSearchBox *self, gboolean inVisible);

const ClutterColor* xfdashboard_searchbox_get_background_color(XfdashboardSearchBox *self);
void xfdashboard_searchbox_set_background_color(XfdashboardSearchBox *self, const ClutterColor *inColor);

G_END_DECLS

#endif
