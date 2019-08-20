/*
 * drag-action: Drag action for actors
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

#ifndef __LIBXFDASHBOARD_DRAG_ACTION__
#define __LIBXFDASHBOARD_DRAG_ACTION__

#if !defined(__LIBXFDASHBOARD_H_INSIDE__) && !defined(LIBXFDASHBOARD_COMPILATION)
#error "Only <libxfdashboard/libxfdashboard.h> can be included directly."
#endif

#include <clutter/clutter.h>

G_BEGIN_DECLS

#define XFDASHBOARD_TYPE_DRAG_ACTION				(xfdashboard_drag_action_get_type())
#define XFDASHBOARD_DRAG_ACTION(obj)				(G_TYPE_CHECK_INSTANCE_CAST((obj), XFDASHBOARD_TYPE_DRAG_ACTION, XfdashboardDragAction))
#define XFDASHBOARD_IS_DRAG_ACTION(obj)				(G_TYPE_CHECK_INSTANCE_TYPE((obj), XFDASHBOARD_TYPE_DRAG_ACTION))
#define XFDASHBOARD_DRAG_ACTION_CLASS(klass)		(G_TYPE_CHECK_CLASS_CAST((klass), XFDASHBOARD_TYPE_DRAG_ACTION, XfdashboardDragActionClass))
#define XFDASHBOARD_IS_DRAG_ACTION_CLASS(klass)		(G_TYPE_CHECK_CLASS_TYPE((klass), XFDASHBOARD_TYPE_DRAG_ACTION))
#define XFDASHBOARD_DRAG_ACTION_GET_CLASS(obj)		(G_TYPE_INSTANCE_GET_CLASS((obj), XFDASHBOARD_TYPE_DRAG_ACTION, XfdashboardDragActionClass))

typedef struct _XfdashboardDragAction				XfdashboardDragAction;
typedef struct _XfdashboardDragActionClass			XfdashboardDragActionClass;
typedef struct _XfdashboardDragActionPrivate		XfdashboardDragActionPrivate;

struct _XfdashboardDragAction
{
	/*< private >*/
	/* Parent instance */
	ClutterDragAction				parent_instance;

	/* Private structure */
	XfdashboardDragActionPrivate	*priv;
};

struct _XfdashboardDragActionClass
{
	/*< private >*/
	/* Parent class */
	ClutterDragActionClass			parent_class;

	/*< public >*/
	/* Virtual functions */
	void (*drag_cancel)(XfdashboardDragAction *self, ClutterActor *inActor, gfloat inX, gfloat inY);
};

/* Public API */
GType xfdashboard_drag_action_get_type(void) G_GNUC_CONST;

ClutterAction* xfdashboard_drag_action_new();
ClutterAction* xfdashboard_drag_action_new_with_source(ClutterActor *inSource);

ClutterActor* xfdashboard_drag_action_get_source(XfdashboardDragAction *self);
ClutterActor* xfdashboard_drag_action_get_actor(XfdashboardDragAction *self);

void xfdashboard_drag_action_get_motion_delta(XfdashboardDragAction *self, gfloat *outDeltaX, gfloat *outDeltaY);

G_END_DECLS

#endif	/* __LIBXFDASHBOARD_DRAG_ACTION__ */
