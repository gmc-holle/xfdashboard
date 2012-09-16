/*
 * application-entry.h: An actor representing an application menu entry
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

#ifndef __XFOVERVIEW_APPLICATION_MENU_ENTRY__
#define __XFOVERVIEW_APPLICATION_MENU_ENTRY__

#include <clutter/clutter.h>
#include <clutter/x11/clutter-x11.h>
#include <garcon/garcon.h>

G_BEGIN_DECLS

#define XFDASHBOARD_TYPE_APPLICATION_MENU_ENTRY				(xfdashboard_application_menu_entry_get_type())
#define XFDASHBOARD_APPLICATION_MENU_ENTRY(obj)				(G_TYPE_CHECK_INSTANCE_CAST((obj), XFDASHBOARD_TYPE_APPLICATION_MENU_ENTRY, XfdashboardApplicationMenuEntry))
#define XFDASHBOARD_IS_APPLICATION_MENU_ENTRY(obj)			(G_TYPE_CHECK_INSTANCE_TYPE((obj), XFDASHBOARD_TYPE_APPLICATION_MENU_ENTRY))
#define XFDASHBOARD_APPLICATION_MENU_ENTRY_CLASS(klass)		(G_TYPE_CHECK_CLASS_CAST((klass), XFDASHBOARD_TYPE_APPLICATION_MENU_ENTRY, XfdashboardApplicationMenuEntryClass))
#define XFDASHBOARD_IS_APPLICATION_MENU_ENTRY_CLASS(klass)	(G_TYPE_CHECK_CLASS_TYPE((klass), XFDASHBOARD_TYPE_APPLICATION_MENU_ENTRY))
#define XFDASHBOARD_APPLICATION_MENU_ENTRY_GET_CLASS(obj)	G_TYPE_INSTANCE_GET_CLASS((obj), XFDASHBOARD_TYPE_APPLICATION_MENU_ENTRY, XfdashboardApplicationMenuEntryClass))

typedef struct _XfdashboardApplicationMenuEntry				XfdashboardApplicationMenuEntry;
typedef struct _XfdashboardApplicationMenuEntryClass		XfdashboardApplicationMenuEntryClass;
typedef struct _XfdashboardApplicationMenuEntryPrivate		XfdashboardApplicationMenuEntryPrivate;

struct _XfdashboardApplicationMenuEntry
{
	/* Parent instance */
	ClutterActor							parent_instance;

	/* Private structure */
	XfdashboardApplicationMenuEntryPrivate	*priv;
};

struct _XfdashboardApplicationMenuEntryClass
{
	/* Parent class */
	ClutterActorClass						parent_class;

	/* Virtual functions */
	void (*clicked)(XfdashboardApplicationMenuEntry *self);
};

GType xfdashboard_application_menu_entry_get_type(void) G_GNUC_CONST;

/* Public API */
ClutterActor* xfdashboard_application_menu_entry_new(void);
ClutterActor* xfdashboard_application_menu_entry_new_with_menu_item(const GarconMenuElement *inMenuElement);
ClutterActor* xfdashboard_application_menu_entry_new_with_custom(const GarconMenuElement *inMenuElement,
																	const gchar *inIconName,
																	const gchar *inTitle,
																	const gchar *inDescription);

gboolean xfdashboard_application_menu_entry_is_submenu(XfdashboardApplicationMenuEntry *self);

const GarconMenuElement* xfdashboard_application_menu_entry_get_menu_element(XfdashboardApplicationMenuEntry *self);
void xfdashboard_application_menu_entry_set_menu_element(XfdashboardApplicationMenuEntry *self, const GarconMenuElement *inMenuElement);

const gfloat xfdashboard_application_menu_entry_get_margin(XfdashboardApplicationMenuEntry *self);
void xfdashboard_application_menu_entry_set_margin(XfdashboardApplicationMenuEntry *self, const gfloat inMargin);

const gfloat xfdashboard_application_menu_entry_get_text_spacing(XfdashboardApplicationMenuEntry *self);
void xfdashboard_application_menu_entry_set_text_spacing(XfdashboardApplicationMenuEntry *self, const gfloat inSpacing);

const ClutterColor* xfdashboard_application_menu_entry_get_background_color(XfdashboardApplicationMenuEntry *self);
void xfdashboard_application_menu_entry_set_background_color(XfdashboardApplicationMenuEntry *self, const ClutterColor *inColor);

const gchar* xfdashboard_application_menu_entry_get_title_font(XfdashboardApplicationMenuEntry *self);
void xfdashboard_application_menu_entry_set_title_font(XfdashboardApplicationMenuEntry *self, const gchar *inFont);

const ClutterColor* xfdashboard_application_menu_entry_get_title_color(XfdashboardApplicationMenuEntry *self);
void xfdashboard_application_menu_entry_set_title_color(XfdashboardApplicationMenuEntry *self, const ClutterColor *inColor);

const PangoEllipsizeMode xfdashboard_application_menu_entry_get_title_ellipsize_mode(XfdashboardApplicationMenuEntry *self);
void xfdashboard_application_menu_entry_set_title_ellipsize_mode(XfdashboardApplicationMenuEntry *self, const PangoEllipsizeMode inMode);

const gchar* xfdashboard_application_menu_entry_get_description_font(XfdashboardApplicationMenuEntry *self);
void xfdashboard_application_menu_entry_set_description_font(XfdashboardApplicationMenuEntry *self, const gchar *inFont);

const ClutterColor* xfdashboard_application_menu_entry_get_description_color(XfdashboardApplicationMenuEntry *self);
void xfdashboard_application_menu_entry_set_description_color(XfdashboardApplicationMenuEntry *self, const ClutterColor *inColor);

const PangoEllipsizeMode xfdashboard_application_menu_entry_get_description_ellipsize_mode(XfdashboardApplicationMenuEntry *self);
void xfdashboard_application_menu_entry_set_description_ellipsize_mode(XfdashboardApplicationMenuEntry *self, const PangoEllipsizeMode inMode);

G_END_DECLS

#endif
