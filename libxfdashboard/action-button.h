/*
 * action-button: A button representing an action to execute when clicked
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

#ifndef __LIBXFDASHBOARD_ACTION_BUTTON__
#define __LIBXFDASHBOARD_ACTION_BUTTON__

#if !defined(__LIBXFDASHBOARD_H_INSIDE__) && !defined(LIBXFDASHBOARD_COMPILATION)
#error "Only <libxfdashboard/libxfdashboard.h> can be included directly."
#endif

#include <libxfdashboard/button.h>

G_BEGIN_DECLS

#define XFDASHBOARD_TYPE_ACTION_BUTTON				(xfdashboard_action_button_get_type())
#define XFDASHBOARD_ACTION_BUTTON(obj)				(G_TYPE_CHECK_INSTANCE_CAST((obj), XFDASHBOARD_TYPE_ACTION_BUTTON, XfdashboardActionButton))
#define XFDASHBOARD_IS_ACTION_BUTTON(obj)			(G_TYPE_CHECK_INSTANCE_TYPE((obj), XFDASHBOARD_TYPE_ACTION_BUTTON))
#define XFDASHBOARD_ACTION_BUTTON_CLASS(klass)		(G_TYPE_CHECK_CLASS_CAST((klass), XFDASHBOARD_TYPE_ACTION_BUTTON, XfdashboardActionButtonClass))
#define XFDASHBOARD_IS_ACTION_BUTTON_CLASS(klass)	(G_TYPE_CHECK_CLASS_TYPE((klass), XFDASHBOARD_TYPE_ACTION_BUTTON))
#define XFDASHBOARD_ACTION_BUTTON_GET_CLASS(obj)	(G_TYPE_INSTANCE_GET_CLASS((obj), XFDASHBOARD_TYPE_ACTION_BUTTON, XfdashboardActionButtonClass))

typedef struct _XfdashboardActionButton				XfdashboardActionButton;
typedef struct _XfdashboardActionButtonClass		XfdashboardActionButtonClass;
typedef struct _XfdashboardActionButtonPrivate		XfdashboardActionButtonPrivate;

/**
 * XfdashboardActionButton:
 *
 * The #XfdashboardActionButton structure contains only private data and
 * should be accessed using the provided API
 */
struct _XfdashboardActionButton
{
	/*< private >*/
	/* Parent instance */
	XfdashboardButton						parent_instance;

	/* Private structure */
	XfdashboardActionButtonPrivate			*priv;
};

/**
 * XfdashboardActionButtonClass:
 *
 * The #XfdashboardActionButtonClass structure contains only private data
 */
struct _XfdashboardActionButtonClass
{
	/*< private >*/
	/* Parent class */
	XfdashboardButtonClass					parent_class;

	/*< public >*/
	/* Virtual functions */
};

/* Public API */
GType xfdashboard_action_button_get_type(void) G_GNUC_CONST;

ClutterActor* xfdashboard_action_button_new(void);

const gchar* xfdashboard_action_button_get_target(XfdashboardActionButton *self);
void xfdashboard_action_button_set_target(XfdashboardActionButton *self, const gchar *inTarget);

const gchar* xfdashboard_action_button_get_action(XfdashboardActionButton *self);
void xfdashboard_action_button_set_action(XfdashboardActionButton *self, const gchar *inAction);

G_END_DECLS

#endif	/* __LIBXFDASHBOARD_ACTION_BUTTON__ */
