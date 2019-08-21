/*
 * drop-action: Drop action for drop targets
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

#ifndef __LIBXFDASHBOARD_DROP_ACTION__
#define __LIBXFDASHBOARD_DROP_ACTION__

#if !defined(__LIBXFDASHBOARD_H_INSIDE__) && !defined(LIBXFDASHBOARD_COMPILATION)
#error "Only <libxfdashboard/libxfdashboard.h> can be included directly."
#endif

#include <clutter/clutter.h>

#include <libxfdashboard/drag-action.h>

G_BEGIN_DECLS

#define XFDASHBOARD_TYPE_DROP_ACTION				(xfdashboard_drop_action_get_type())
#define XFDASHBOARD_DROP_ACTION(obj)				(G_TYPE_CHECK_INSTANCE_CAST((obj), XFDASHBOARD_TYPE_DROP_ACTION, XfdashboardDropAction))
#define XFDASHBOARD_IS_DROP_ACTION(obj)				(G_TYPE_CHECK_INSTANCE_TYPE((obj), XFDASHBOARD_TYPE_DROP_ACTION))
#define XFDASHBOARD_DROP_ACTION_CLASS(klass)		(G_TYPE_CHECK_CLASS_CAST((klass), XFDASHBOARD_TYPE_DROP_ACTION, XfdashboardDropActionClass))
#define XFDASHBOARD_IS_DROP_ACTION_CLASS(klass)		(G_TYPE_CHECK_CLASS_TYPE((klass), XFDASHBOARD_TYPE_DROP_ACTION))
#define XFDASHBOARD_DROP_ACTION_GET_CLASS(obj)		(G_TYPE_INSTANCE_GET_CLASS((obj), XFDASHBOARD_TYPE_DROP_ACTION, XfdashboardDropActionClass))

typedef struct _XfdashboardDropAction				XfdashboardDropAction;
typedef struct _XfdashboardDropActionClass			XfdashboardDropActionClass;
typedef struct _XfdashboardDropActionPrivate		XfdashboardDropActionPrivate;

struct _XfdashboardDropAction
{
	/*< private >*/
	/* Parent instance */
	ClutterAction					parent_instance;

	/* Private structure */
	XfdashboardDropActionPrivate	*priv;
};

struct _XfdashboardDropActionClass
{
	/*< private >*/
	/* Parent class */
	ClutterActionClass				parent_class;

	/*< public >*/
	/* Virtual functions */
	gboolean (*begin)(XfdashboardDropAction *self, XfdashboardDragAction *inDragAction);
	gboolean (*can_drop)(XfdashboardDropAction *self, XfdashboardDragAction *inDragAction, gfloat inX, gfloat inY);
	void (*drop)(XfdashboardDropAction *self, XfdashboardDragAction *inDragAction, gfloat inX, gfloat inY);
	void (*end)(XfdashboardDropAction *self, XfdashboardDragAction *inDragAction);

	void (*drag_enter)(XfdashboardDropAction *self, XfdashboardDragAction *inDragAction);
	void (*drag_motion)(XfdashboardDropAction *self, XfdashboardDragAction *inDragAction, gfloat inX, gfloat inY);
	void (*drag_leave)(XfdashboardDropAction *self, XfdashboardDragAction *inDragAction);
};

/* Public API */
GType xfdashboard_drop_action_get_type(void) G_GNUC_CONST;

ClutterAction* xfdashboard_drop_action_new();

GSList* xfdashboard_drop_action_get_targets(void);

G_END_DECLS

#endif	/* __LIBXFDASHBOARD_DROP_ACTION__ */
