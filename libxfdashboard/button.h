/*
 * button: A label actor which can react on click actions
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

#ifndef __LIBXFDASHBOARD_BUTTON__
#define __LIBXFDASHBOARD_BUTTON__

#if !defined(__LIBXFDASHBOARD_H_INSIDE__) && !defined(LIBXFDASHBOARD_COMPILATION)
#error "Only <libxfdashboard/libxfdashboard.h> can be included directly."
#endif

#include <clutter/clutter.h>

#include <libxfdashboard/label.h>

G_BEGIN_DECLS

/* Object declaration */
#define XFDASHBOARD_TYPE_BUTTON					(xfdashboard_button_get_type())
#define XFDASHBOARD_BUTTON(obj)					(G_TYPE_CHECK_INSTANCE_CAST((obj), XFDASHBOARD_TYPE_BUTTON, XfdashboardButton))
#define XFDASHBOARD_IS_BUTTON(obj)				(G_TYPE_CHECK_INSTANCE_TYPE((obj), XFDASHBOARD_TYPE_BUTTON))
#define XFDASHBOARD_BUTTON_CLASS(klass)			(G_TYPE_CHECK_CLASS_CAST((klass), XFDASHBOARD_TYPE_BUTTON, XfdashboardButtonClass))
#define XFDASHBOARD_IS_BUTTON_CLASS(klass)		(G_TYPE_CHECK_CLASS_TYPE((klass), XFDASHBOARD_TYPE_BUTTON))
#define XFDASHBOARD_BUTTON_GET_CLASS(obj)		(G_TYPE_INSTANCE_GET_CLASS((obj), XFDASHBOARD_TYPE_BUTTON, XfdashboardButtonClass))

typedef struct _XfdashboardButton				XfdashboardButton;
typedef struct _XfdashboardButtonClass			XfdashboardButtonClass;
typedef struct _XfdashboardButtonPrivate		XfdashboardButtonPrivate;

struct _XfdashboardButton
{
	/*< private >*/
	/* Parent instance */
	XfdashboardLabel			parent_instance;

	/* Private structure */
	XfdashboardButtonPrivate	*priv;
};

struct _XfdashboardButtonClass
{
	/*< private >*/
	/* Parent class */
	XfdashboardLabelClass		parent_class;

	/*< public >*/
	/* Virtual functions */
	void (*clicked)(XfdashboardButton *self);
};

/* Public API */
GType xfdashboard_button_get_type(void) G_GNUC_CONST;

ClutterActor* xfdashboard_button_new(void);
ClutterActor* xfdashboard_button_new_with_text(const gchar *inText);
ClutterActor* xfdashboard_button_new_with_icon_name(const gchar *inIconName);
ClutterActor* xfdashboard_button_new_with_gicon(GIcon *inIcon);
ClutterActor* xfdashboard_button_new_full_with_icon_name(const gchar *inIconName, const gchar *inText);
ClutterActor* xfdashboard_button_new_full_with_gicon(GIcon *inIcon, const gchar *inText);

G_END_DECLS

#endif	/* __LIBXFDASHBOARD_BUTTON__ */
