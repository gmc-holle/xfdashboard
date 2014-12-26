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

#ifndef __XFDASHBOARD_BINDINGS__
#define __XFDASHBOARD_BINDINGS__

#include <clutter/clutter.h>

#include "types.h"

G_BEGIN_DECLS

#define XFDASHBOARD_TYPE_BINDINGS				(xfdashboard_bindings_get_type())
#define XFDASHBOARD_BINDINGS(obj)				(G_TYPE_CHECK_INSTANCE_CAST((obj), XFDASHBOARD_TYPE_BINDINGS, XfdashboardBindings))
#define XFDASHBOARD_IS_BINDINGS(obj)			(G_TYPE_CHECK_INSTANCE_TYPE((obj), XFDASHBOARD_TYPE_BINDINGS))
#define XFDASHBOARD_BINDINGS_CLASS(klass)		(G_TYPE_CHECK_CLASS_CAST((klass), XFDASHBOARD_TYPE_BINDINGS, XfdashboardBindingsClass))
#define XFDASHBOARD_IS_BINDINGS_CLASS(klass)	(G_TYPE_CHECK_CLASS_TYPE((klass), XFDASHBOARD_TYPE_BINDINGS))
#define XFDASHBOARD_BINDINGS_GET_CLASS(obj)		(G_TYPE_INSTANCE_GET_CLASS((obj), XFDASHBOARD_TYPE_BINDINGS, XfdashboardBindingsClass))

typedef struct _XfdashboardBindings				XfdashboardBindings;
typedef struct _XfdashboardBindingsClass		XfdashboardBindingsClass;
typedef struct _XfdashboardBindingsPrivate		XfdashboardBindingsPrivate;

struct _XfdashboardBindings
{
	/* Parent instance */
	GObject							parent_instance;

	/* Private structure */
	XfdashboardBindingsPrivate		*priv;
};

struct _XfdashboardBindingsClass
{
	/*< private >*/
	/* Parent class */
	GObjectClass					parent_class;
};

/* Errors */
#define XFDASHBOARD_BINDINGS_ERROR					(xfdashboard_bindings_error_quark())

GQuark xfdashboard_bindings_error_quark(void);

typedef enum /*< prefix=XFDASHBOARD_BINDINGS_ERROR >*/
{
	XFDASHBOARD_BINDINGS_ERROR_FILE_NOT_FOUND,
	XFDASHBOARD_BINDINGS_ERROR_PARSER_INTERNAL_ERROR,
	XFDASHBOARD_BINDINGS_ERROR_MALFORMED
} XfdashboardBindingsErrorEnum;

/* Public API */
GType xfdashboard_bindings_get_type(void) G_GNUC_CONST;

XfdashboardBindings* xfdashboard_bindings_get_default(void);

gboolean xfdashboard_bindings_load(XfdashboardBindings *self, GError **outError);

const gchar* xfdashboard_bindings_find_for_event(XfdashboardBindings *self, ClutterActor *inActor, const ClutterEvent *inEvent);

G_END_DECLS

#endif	/* __XFDASHBOARD_BINDINGS__ */
