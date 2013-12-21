/*
 * notification: A notification actor
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

#ifndef __XFDASHBOARD_NOTIFICATION__
#define __XFDASHBOARD_NOTIFICATION__

#include "textbox.h"
#include "types.h"

G_BEGIN_DECLS

#define XFDASHBOARD_TYPE_NOTIFICATION				(xfdashboard_notification_get_type())
#define XFDASHBOARD_NOTIFICATION(obj)				(G_TYPE_CHECK_INSTANCE_CAST((obj), XFDASHBOARD_TYPE_NOTIFICATION, XfdashboardNotification))
#define XFDASHBOARD_IS_NOTIFICATION(obj)			(G_TYPE_CHECK_INSTANCE_TYPE((obj), XFDASHBOARD_TYPE_NOTIFICATION))
#define XFDASHBOARD_NOTIFICATION_CLASS(klass)		(G_TYPE_CHECK_CLASS_CAST((klass), XFDASHBOARD_TYPE_NOTIFICATION, XfdashboardNotificationClass))
#define XFDASHBOARD_IS_NOTIFICATION_CLASS(klass)	(G_TYPE_CHECK_CLASS_TYPE((klass), XFDASHBOARD_TYPE_NOTIFICATION))
#define XFDASHBOARD_NOTIFICATION_GET_CLASS(obj)		(G_TYPE_INSTANCE_GET_CLASS((obj), XFDASHBOARD_TYPE_NOTIFICATION, XfdashboardNotificationClass))

typedef struct _XfdashboardNotification				XfdashboardNotification;
typedef struct _XfdashboardNotificationClass		XfdashboardNotificationClass;
typedef struct _XfdashboardNotificationPrivate		XfdashboardNotificationPrivate;

struct _XfdashboardNotification
{
	/* Parent instance */
	XfdashboardTextBox					parent_instance;

	/* Private structure */
	XfdashboardNotificationPrivate		*priv;
};

struct _XfdashboardNotificationClass
{
	/*< private >*/
	/* Parent class */
	XfdashboardTextBoxClass				parent_class;

	/*< public >*/
	/* Virtual functions */
};

/* Public API */
GType xfdashboard_notification_get_type(void) G_GNUC_CONST;

ClutterActor* xfdashboard_notification_new(void);
ClutterActor* xfdashboard_notification_new_with_text(const gchar *inText);
ClutterActor* xfdashboard_notification_new_with_icon(const gchar *inIconName);
ClutterActor* xfdashboard_notification_new_full(const gchar *inText, const gchar *inIconName);

XfdashboardNotificationPlacement xfdashboard_notification_get_placement(XfdashboardNotification *self);
void xfdashboard_notification_set_placement(XfdashboardNotification *self, XfdashboardNotificationPlacement inPlacement);

gfloat xfdashboard_notification_get_margin(XfdashboardNotification *self);
void xfdashboard_notification_set_margin(XfdashboardNotification *self, gfloat inMargin);

G_END_DECLS

#endif	/* __XFDASHBOARD_NOTIFICATION__ */
