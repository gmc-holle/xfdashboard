/*
 * toggle-button: A button which can toggle its state between on and off
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

#ifndef __LIBXFDASHBOARD_TOGGLE_BUTTON__
#define __LIBXFDASHBOARD_TOGGLE_BUTTON__

#if !defined(__LIBXFDASHBOARD_H_INSIDE__) && !defined(LIBXFDASHBOARD_COMPILATION)
#error "Only <libxfdashboard/libxfdashboard.h> can be included directly."
#endif

#include <libxfdashboard/button.h>

G_BEGIN_DECLS

#define XFDASHBOARD_TYPE_TOGGLE_BUTTON				(xfdashboard_toggle_button_get_type())
#define XFDASHBOARD_TOGGLE_BUTTON(obj)				(G_TYPE_CHECK_INSTANCE_CAST((obj), XFDASHBOARD_TYPE_TOGGLE_BUTTON, XfdashboardToggleButton))
#define XFDASHBOARD_IS_TOGGLE_BUTTON(obj)			(G_TYPE_CHECK_INSTANCE_TYPE((obj), XFDASHBOARD_TYPE_TOGGLE_BUTTON))
#define XFDASHBOARD_TOGGLE_BUTTON_CLASS(klass)		(G_TYPE_CHECK_CLASS_CAST((klass), XFDASHBOARD_TYPE_TOGGLE_BUTTON, XfdashboardToggleButtonClass))
#define XFDASHBOARD_IS_TOGGLE_BUTTON_CLASS(klass)	(G_TYPE_CHECK_CLASS_TYPE((klass), XFDASHBOARD_TYPE_TOGGLE_BUTTON))
#define XFDASHBOARD_TOGGLE_BUTTON_GET_CLASS(obj)	(G_TYPE_INSTANCE_GET_CLASS((obj), XFDASHBOARD_TYPE_TOGGLE_BUTTON, XfdashboardToggleButtonClass))

typedef struct _XfdashboardToggleButton				XfdashboardToggleButton;
typedef struct _XfdashboardToggleButtonClass		XfdashboardToggleButtonClass;
typedef struct _XfdashboardToggleButtonPrivate		XfdashboardToggleButtonPrivate;

/**
 * XfdashboardToggleButton:
 *
 * The #XfdashboardToggleButton structure contains only private data and
 * should be accessed using the provided API
 */
struct _XfdashboardToggleButton
{
	/*< private >*/
	/* Parent instance */
	XfdashboardButton				parent_instance;

	/* Private structure */
	XfdashboardToggleButtonPrivate	*priv;
};

/**
 * XfdashboardToggleButtonClass:
 * @toggled: Class handler for the #XfdashboardToggleButtonClass::toggled signal
 *
 * The #XfdashboardToggleButtonClass structure contains only private data
 */
struct _XfdashboardToggleButtonClass
{
	/*< private >*/
	/* Parent class */
	XfdashboardButtonClass			parent_class;

	/*< public >*/
	/* Virtual functions */
	void (*toggled)(XfdashboardToggleButton *self);
};

/* Public API */
GType xfdashboard_toggle_button_get_type(void) G_GNUC_CONST;

ClutterActor* xfdashboard_toggle_button_new(void);
ClutterActor* xfdashboard_toggle_button_new_with_text(const gchar *inText);
ClutterActor* xfdashboard_toggle_button_new_with_icon_name(const gchar *inIconName);
ClutterActor* xfdashboard_toggle_button_new_with_gicon(GIcon *inIcon);
ClutterActor* xfdashboard_toggle_button_new_full_with_icon_name(const gchar *inIconName, const gchar *inText);
ClutterActor* xfdashboard_toggle_button_new_full_with_gicon(GIcon *inIcon, const gchar *inText);

gboolean xfdashboard_toggle_button_get_toggle_state(XfdashboardToggleButton *self);
void xfdashboard_toggle_button_set_toggle_state(XfdashboardToggleButton *self, gboolean inToggleState);

gboolean xfdashboard_toggle_button_get_auto_toggle(XfdashboardToggleButton *self);
void xfdashboard_toggle_button_set_auto_toggle(XfdashboardToggleButton *self, gboolean inAuto);

void xfdashboard_toggle_button_toggle(XfdashboardToggleButton *self);

G_END_DECLS

#endif	/* __LIBXFDASHBOARD_TOGGLE_BUTTON__ */
