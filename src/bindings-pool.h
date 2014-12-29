/*
 * bindings: Customizable keyboard and pointer bindings for focusable actors
 * 
 * Copyright 2012-2014 Stephan Haller <nomad@froevel.de>
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

#ifndef __XFDASHBOARD_BINDINGS_POOL__
#define __XFDASHBOARD_BINDINGS_POOL__

#include <clutter/clutter.h>

#include "types.h"

G_BEGIN_DECLS

#define XFDASHBOARD_TYPE_BINDINGS_POOL				(xfdashboard_bindings_pool_get_type())
#define XFDASHBOARD_BINDINGS_POOL(obj)				(G_TYPE_CHECK_INSTANCE_CAST((obj), XFDASHBOARD_TYPE_BINDINGS_POOL, XfdashboardBindingsPool))
#define XFDASHBOARD_IS_BINDINGS_POOL(obj)			(G_TYPE_CHECK_INSTANCE_TYPE((obj), XFDASHBOARD_TYPE_BINDINGS_POOL))
#define XFDASHBOARD_BINDINGS_POOL_CLASS(klass)		(G_TYPE_CHECK_CLASS_CAST((klass), XFDASHBOARD_TYPE_BINDINGS_POOL, XfdashboardBindingsPoolClass))
#define XFDASHBOARD_IS_BINDINGS_POOL_CLASS(klass)	(G_TYPE_CHECK_CLASS_TYPE((klass), XFDASHBOARD_TYPE_BINDINGS_POOL))
#define XFDASHBOARD_BINDINGS_POOL_GET_CLASS(obj)		(G_TYPE_INSTANCE_GET_CLASS((obj), XFDASHBOARD_TYPE_BINDINGS_POOL, XfdashboardBindingsPoolClass))

typedef struct _XfdashboardBindingsPool				XfdashboardBindingsPool;
typedef struct _XfdashboardBindingsPoolClass		XfdashboardBindingsPoolClass;
typedef struct _XfdashboardBindingsPoolPrivate		XfdashboardBindingsPoolPrivate;

struct _XfdashboardBindingsPool
{
	/* Parent instance */
	GObject							parent_instance;

	/* Private structure */
	XfdashboardBindingsPoolPrivate		*priv;
};

struct _XfdashboardBindingsPoolClass
{
	/*< private >*/
	/* Parent class */
	GObjectClass					parent_class;
};

/* Errors */
#define XFDASHBOARD_BINDINGS_POOL_ERROR					(xfdashboard_bindings_pool_error_quark())

GQuark xfdashboard_bindings_pool_error_quark(void);

typedef enum /*< prefix=XFDASHBOARD_BINDINGS_POOL_ERROR >*/
{
	XFDASHBOARD_BINDINGS_POOL_ERROR_FILE_NOT_FOUND,
	XFDASHBOARD_BINDINGS_POOL_ERROR_PARSER_INTERNAL_ERROR,
	XFDASHBOARD_BINDINGS_POOL_ERROR_MALFORMED
} XfdashboardBindingsPoolErrorEnum;

/* Public API */
GType xfdashboard_bindings_pool_get_type(void) G_GNUC_CONST;

XfdashboardBindingsPool* xfdashboard_bindings_pool_get_default(void);

gboolean xfdashboard_bindings_pool_load(XfdashboardBindingsPool *self, GError **outError);

const gchar* xfdashboard_bindings_pool_find_for_event(XfdashboardBindingsPool *self, ClutterActor *inActor, const ClutterEvent *inEvent);

G_END_DECLS

#endif	/* __XFDASHBOARD_BINDINGS_POOL__ */
