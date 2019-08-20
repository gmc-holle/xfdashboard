/*
 * view-manager: Single-instance managing views
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

#ifndef __LIBXFDASHBOARD_VIEW_MANAGER__
#define __LIBXFDASHBOARD_VIEW_MANAGER__

#if !defined(__LIBXFDASHBOARD_H_INSIDE__) && !defined(LIBXFDASHBOARD_COMPILATION)
#error "Only <libxfdashboard/libxfdashboard.h> can be included directly."
#endif

#include <glib-object.h>

G_BEGIN_DECLS

#define XFDASHBOARD_TYPE_VIEW_MANAGER				(xfdashboard_view_manager_get_type())
#define XFDASHBOARD_VIEW_MANAGER(obj)				(G_TYPE_CHECK_INSTANCE_CAST((obj), XFDASHBOARD_TYPE_VIEW_MANAGER, XfdashboardViewManager))
#define XFDASHBOARD_IS_VIEW_MANAGER(obj)			(G_TYPE_CHECK_INSTANCE_TYPE((obj), XFDASHBOARD_TYPE_VIEW_MANAGER))
#define XFDASHBOARD_VIEW_MANAGER_CLASS(klass)		(G_TYPE_CHECK_CLASS_CAST((klass), XFDASHBOARD_TYPE_VIEW_MANAGER, XfdashboardViewManagerClass))
#define XFDASHBOARD_IS_VIEW_MANAGER_CLASS(klass)	(G_TYPE_CHECK_CLASS_TYPE((klass), XFDASHBOARD_TYPE_VIEW_MANAGER))
#define XFDASHBOARD_VIEW_MANAGER_GET_CLASS(obj)		(G_TYPE_INSTANCE_GET_CLASS((obj), XFDASHBOARD_TYPE_VIEW_MANAGER, XfdashboardViewManagerClass))

typedef struct _XfdashboardViewManager				XfdashboardViewManager;
typedef struct _XfdashboardViewManagerClass			XfdashboardViewManagerClass;
typedef struct _XfdashboardViewManagerPrivate		XfdashboardViewManagerPrivate;

struct _XfdashboardViewManager
{
	/*< private >*/
	/* Parent instance */
	GObject							parent_instance;

	/* Private structure */
	XfdashboardViewManagerPrivate	*priv;
};

struct _XfdashboardViewManagerClass
{
	/*< private >*/
	/* Parent class */
	GObjectClass					parent_class;

	/*< public >*/
	/* Virtual functions */
	void (*registered)(XfdashboardViewManager *self, const gchar *inID);
	void (*unregistered)(XfdashboardViewManager *self, const gchar *inID);
};

/* Public API */
GType xfdashboard_view_manager_get_type(void) G_GNUC_CONST;

XfdashboardViewManager* xfdashboard_view_manager_get_default(void);

gboolean xfdashboard_view_manager_register(XfdashboardViewManager *self, const gchar *inID, GType inViewType);
gboolean xfdashboard_view_manager_unregister(XfdashboardViewManager *self, const gchar *inID);
GList* xfdashboard_view_manager_get_registered(XfdashboardViewManager *self);
gboolean xfdashboard_view_manager_has_registered_id(XfdashboardViewManager *self, const gchar *inID);

GObject* xfdashboard_view_manager_create_view(XfdashboardViewManager *self, const gchar *inID);

G_END_DECLS

#endif	/* __LIBXFDASHBOARD_VIEW_MANAGER__ */
