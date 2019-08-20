/*
 * tooltip-action: An action to display a tooltip after a short timeout
 *                 without movement at the referred actor
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

#ifndef __LIBXFDASHBOARD_TOOLTIP_ACTION__
#define __LIBXFDASHBOARD_TOOLTIP_ACTION__

#if !defined(__LIBXFDASHBOARD_H_INSIDE__) && !defined(LIBXFDASHBOARD_COMPILATION)
#error "Only <libxfdashboard/libxfdashboard.h> can be included directly."
#endif

#include <clutter/clutter.h>

G_BEGIN_DECLS

#define XFDASHBOARD_TYPE_TOOLTIP_ACTION				(xfdashboard_tooltip_action_get_type ())
#define XFDASHBOARD_TOOLTIP_ACTION(obj)				(G_TYPE_CHECK_INSTANCE_CAST ((obj), XFDASHBOARD_TYPE_TOOLTIP_ACTION, XfdashboardTooltipAction))
#define XFDASHBOARD_IS_TOOLTIP_ACTION(obj)			(G_TYPE_CHECK_INSTANCE_TYPE ((obj), XFDASHBOARD_TYPE_TOOLTIP_ACTION))
#define XFDASHBOARD_TOOLTIP_ACTION_CLASS(klass)		(G_TYPE_CHECK_CLASS_CAST ((klass), XFDASHBOARD_TYPE_TOOLTIP_ACTION, XfdashboardTooltipActionClass))
#define XFDASHBOARD_IS_TOOLTIP_ACTION_CLASS(klass)	(G_TYPE_CHECK_CLASS_TYPE ((klass), XFDASHBOARD_TYPE_TOOLTIP_ACTION))
#define XFDASHBOARD_TOOLTIP_ACTION_GET_CLASS(obj)	(G_TYPE_INSTANCE_GET_CLASS ((obj), XFDASHBOARD_TYPE_TOOLTIP_ACTION, XfdashboardTooltipActionClass))

typedef struct _XfdashboardTooltipAction			XfdashboardTooltipAction;
typedef struct _XfdashboardTooltipActionPrivate		XfdashboardTooltipActionPrivate;
typedef struct _XfdashboardTooltipActionClass		XfdashboardTooltipActionClass;

struct _XfdashboardTooltipAction
{
	/*< private >*/
	/* Parent instance */
	ClutterAction						parent_instance;

	/* Private structure */
	XfdashboardTooltipActionPrivate		*priv;
};

struct _XfdashboardTooltipActionClass
{
	/*< private >*/
	/* Parent class */
	ClutterActionClass					parent_class;

	/*< public >*/
	/* Virtual functions */
	void (*activating)(XfdashboardTooltipAction *self);
};

/* Public API */
GType xfdashboard_tooltip_action_get_type(void) G_GNUC_CONST;

ClutterAction* xfdashboard_tooltip_action_new(void);

const gchar* xfdashboard_tooltip_action_get_text(XfdashboardTooltipAction *self);
void xfdashboard_tooltip_action_set_text(XfdashboardTooltipAction *self, const gchar *inTooltipText);

void xfdashboard_tooltip_action_get_position(XfdashboardTooltipAction *self, gfloat *outX, gfloat *outY);

G_END_DECLS

#endif	/* __LIBXFDASHBOARD_TOOLTIP_ACTION__ */
