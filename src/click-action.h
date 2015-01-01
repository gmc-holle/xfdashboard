/*
 * click-action: Bad workaround for click action which prevent drag actions
 *               to work properly since clutter version 1.12 at least.
 *               This object/file is a complete copy of the original
 *               clutter-click-action.{c,h} files of clutter 1.12 except
 *               for one line, the renamed function names and the applied
 *               coding style. The clutter-click-action.{c,h} files of
 *               later clutter versions do not differ much from this one
 *               so this object should work also for this versions.
 *
 *               See bug: https://bugzilla.gnome.org/show_bug.cgi?id=714993
 * 
 * Copyright 2012-2015 Stephan Haller <nomad@froevel.de>
 *         original by Emmanuele Bassi <ebassi@linux.intel.com>
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

#ifndef __XFDASHBOARD_CLICK_ACTION__
#define __XFDASHBOARD_CLICK_ACTION__

#include <clutter/clutter.h>

G_BEGIN_DECLS

#define XFDASHBOARD_TYPE_CLICK_ACTION				(xfdashboard_click_action_get_type ())
#define XFDASHBOARD_CLICK_ACTION(obj)				(G_TYPE_CHECK_INSTANCE_CAST ((obj), XFDASHBOARD_TYPE_CLICK_ACTION, XfdashboardClickAction))
#define XFDASHBOARD_IS_CLICK_ACTION(obj)			(G_TYPE_CHECK_INSTANCE_TYPE ((obj), XFDASHBOARD_TYPE_CLICK_ACTION))
#define XFDASHBOARD_CLICK_ACTION_CLASS(klass)		(G_TYPE_CHECK_CLASS_CAST ((klass), XFDASHBOARD_TYPE_CLICK_ACTION, XfdashboardClickActionClass))
#define XFDASHBOARD_IS_CLICK_ACTION_CLASS(klass)	(G_TYPE_CHECK_CLASS_TYPE ((klass), XFDASHBOARD_TYPE_CLICK_ACTION))
#define XFDASHBOARD_CLICK_ACTION_GET_CLASS(obj)		(G_TYPE_INSTANCE_GET_CLASS ((obj), XFDASHBOARD_TYPE_CLICK_ACTION, XfdashboardClickActionClass))

typedef struct _XfdashboardClickAction				XfdashboardClickAction;
typedef struct _XfdashboardClickActionPrivate		XfdashboardClickActionPrivate;
typedef struct _XfdashboardClickActionClass			XfdashboardClickActionClass;

struct _XfdashboardClickAction
{
	/* Parent instance */
	ClutterAction					parent_instance;

	XfdashboardClickActionPrivate	*priv;
};

struct _XfdashboardClickActionClass
{
	/*< private >*/
	/* Parent class */
	ClutterActionClass				parent_class;

	/*< public >*/
	/* Virtual functions */
	void (*clicked)(XfdashboardClickAction *self, ClutterActor *inActor);
	gboolean (*long_press)(XfdashboardClickAction *self, ClutterActor *inActor, ClutterLongPressState inState);
};

/* Public API */
GType xfdashboard_click_action_get_type(void) G_GNUC_CONST;

ClutterAction* xfdashboard_click_action_new(void);

guint xfdashboard_click_action_get_button(XfdashboardClickAction *self);
ClutterModifierType xfdashboard_click_action_get_state(XfdashboardClickAction *self);
void xfdashboard_click_action_get_coords(XfdashboardClickAction *self, gfloat *outPressX, gfloat *outPressY);

void xfdashboard_click_action_release(XfdashboardClickAction *self);

G_END_DECLS

#endif	/* __XFDASHBOARD_CLICK_ACTION__ */
