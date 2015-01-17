/*
 * focus-manager: Single-instance managing focusable actors
 *                for keyboard navigation
 * 
 * Copyright 2012-2015 Stephan Haller <nomad@froevel.de>
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

#ifndef __XFDASHBOARD_FOCUS_MANAGER__
#define __XFDASHBOARD_FOCUS_MANAGER__

#include <glib-object.h>

#include "focusable.h"

G_BEGIN_DECLS

#define XFDASHBOARD_TYPE_FOCUS_MANAGER				(xfdashboard_focus_manager_get_type())
#define XFDASHBOARD_FOCUS_MANAGER(obj)				(G_TYPE_CHECK_INSTANCE_CAST((obj), XFDASHBOARD_TYPE_FOCUS_MANAGER, XfdashboardFocusManager))
#define XFDASHBOARD_IS_FOCUS_MANAGER(obj)			(G_TYPE_CHECK_INSTANCE_TYPE((obj), XFDASHBOARD_TYPE_FOCUS_MANAGER))
#define XFDASHBOARD_FOCUS_MANAGER_CLASS(klass)		(G_TYPE_CHECK_CLASS_CAST((klass), XFDASHBOARD_TYPE_FOCUS_MANAGER, XfdashboardFocusManagerClass))
#define XFDASHBOARD_IS_FOCUS_MANAGER_CLASS(klass)	(G_TYPE_CHECK_CLASS_TYPE((klass), XFDASHBOARD_TYPE_FOCUS_MANAGER))
#define XFDASHBOARD_FOCUS_MANAGER_GET_CLASS(obj)	(G_TYPE_INSTANCE_GET_CLASS((obj), XFDASHBOARD_TYPE_FOCUS_MANAGER, XfdashboardFocusManagerClass))

typedef struct _XfdashboardFocusManager				XfdashboardFocusManager;
typedef struct _XfdashboardFocusManagerClass		XfdashboardFocusManagerClass;
typedef struct _XfdashboardFocusManagerPrivate		XfdashboardFocusManagerPrivate;

struct _XfdashboardFocusManager
{
	/* Parent instance */
	GObject							parent_instance;

	/* Private structure */
	XfdashboardFocusManagerPrivate	*priv;
};

struct _XfdashboardFocusManagerClass
{
	/*< private >*/
	/* Parent class */
	GObjectClass					parent_class;

	/*< public >*/
	/* Virtual functions */
	void (*registered)(XfdashboardFocusManager *self, XfdashboardFocusable *inActor);
	void (*unregistered)(XfdashboardFocusManager *self, XfdashboardFocusable *inActor);

	void (*changed)(XfdashboardFocusManager *self,
						XfdashboardFocusable *oldActor,
						XfdashboardFocusable *newActor);

	/* Binding actions */
	gboolean (*focus_move_next)(XfdashboardFocusManager *self,
									XfdashboardFocusable *inSource,
									const gchar *inAction,
									ClutterEvent *inEvent);
	gboolean (*focus_move_previous)(XfdashboardFocusManager *self,
									XfdashboardFocusable *inSource,
									const gchar *inAction,
									ClutterEvent *inEvent);
};

/* Public API */
GType xfdashboard_focus_manager_get_type(void) G_GNUC_CONST;

XfdashboardFocusManager* xfdashboard_focus_manager_get_default(void);

void xfdashboard_focus_manager_register(XfdashboardFocusManager *self, XfdashboardFocusable *inFocusable);
void xfdashboard_focus_manager_register_after(XfdashboardFocusManager *self, XfdashboardFocusable *inFocusable, XfdashboardFocusable *inAfterFocusable);
void xfdashboard_focus_manager_unregister(XfdashboardFocusManager *self, XfdashboardFocusable *inFocusable);
GList* xfdashboard_focus_manager_get_registered(XfdashboardFocusManager *self);
gboolean xfdashboard_focus_manager_is_registered(XfdashboardFocusManager *self, XfdashboardFocusable *inFocusable);

gboolean xfdashboard_focus_manager_has_focus(XfdashboardFocusManager *self, XfdashboardFocusable *inFocusable);
XfdashboardFocusable* xfdashboard_focus_manager_get_focus(XfdashboardFocusManager *self);
void xfdashboard_focus_manager_set_focus(XfdashboardFocusManager *self, XfdashboardFocusable *inFocusable);

XfdashboardFocusable* xfdashboard_focus_manager_get_next_focusable(XfdashboardFocusManager *self,
																	XfdashboardFocusable *inBeginFocusable);
XfdashboardFocusable* xfdashboard_focus_manager_get_previous_focusable(XfdashboardFocusManager *self,
																		XfdashboardFocusable *inBeginFocusable);

gboolean xfdashboard_focus_manager_handle_key_event(XfdashboardFocusManager *self, const ClutterEvent *inEvent, XfdashboardFocusable *inFocusable);

G_END_DECLS

#endif	/* __XFDASHBOARD_FOCUS_MANAGER__ */
