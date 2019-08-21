/*
 * stylable: An interface which can be inherited by actor and objects
 *           to get styled by a theme
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

#ifndef __LIBXFDASHBOARD_STYLABLE__
#define __LIBXFDASHBOARD_STYLABLE__

#if !defined(__LIBXFDASHBOARD_H_INSIDE__) && !defined(LIBXFDASHBOARD_COMPILATION)
#error "Only <libxfdashboard/libxfdashboard.h> can be included directly."
#endif

#include <glib-object.h>

G_BEGIN_DECLS

#define XFDASHBOARD_TYPE_STYLABLE				(xfdashboard_stylable_get_type())
#define XFDASHBOARD_STYLABLE(obj)				(G_TYPE_CHECK_INSTANCE_CAST((obj), XFDASHBOARD_TYPE_STYLABLE, XfdashboardStylable))
#define XFDASHBOARD_IS_STYLABLE(obj)			(G_TYPE_CHECK_INSTANCE_TYPE((obj), XFDASHBOARD_TYPE_STYLABLE))
#define XFDASHBOARD_STYLABLE_GET_IFACE(obj)		(G_TYPE_INSTANCE_GET_INTERFACE((obj), XFDASHBOARD_TYPE_STYLABLE, XfdashboardStylableInterface))

typedef struct _XfdashboardStylable				XfdashboardStylable;
typedef struct _XfdashboardStylableInterface	XfdashboardStylableInterface;

struct _XfdashboardStylableInterface
{
	/*< private >*/
	/* Parent interface */
	GTypeInterface				parent_interface;

	/*< public >*/
	/* Virtual functions */
	void (*get_stylable_properties)(XfdashboardStylable *self, GHashTable *ioStylableProperties);

	const gchar* (*get_name)(XfdashboardStylable *self);

	XfdashboardStylable* (*get_parent)(XfdashboardStylable *self);

	const gchar* (*get_classes)(XfdashboardStylable *self);
	void (*set_classes)(XfdashboardStylable *self, const gchar *inClasses);

	const gchar* (*get_pseudo_classes)(XfdashboardStylable *self);
	void (*set_pseudo_classes)(XfdashboardStylable *self, const gchar *inClasses);

	void (*invalidate)(XfdashboardStylable *self);
};

/* Public API */
GType xfdashboard_stylable_get_type(void) G_GNUC_CONST;

GHashTable* xfdashboard_stylable_get_stylable_properties(XfdashboardStylable *self);
gboolean xfdashboard_stylable_add_stylable_property(XfdashboardStylable *self,
													GHashTable *ioStylableProperties,
													const gchar *inProperty);

const gchar* xfdashboard_stylable_get_name(XfdashboardStylable *self);

XfdashboardStylable* xfdashboard_stylable_get_parent(XfdashboardStylable *self);

const gchar* xfdashboard_stylable_get_classes(XfdashboardStylable *self);
void xfdashboard_stylable_set_classes(XfdashboardStylable *self, const gchar *inClasses);
gboolean xfdashboard_stylable_has_class(XfdashboardStylable *self, const gchar *inClass);
void xfdashboard_stylable_add_class(XfdashboardStylable *self, const gchar *inClass);
void xfdashboard_stylable_remove_class(XfdashboardStylable *self, const gchar *inClass);

const gchar* xfdashboard_stylable_get_pseudo_classes(XfdashboardStylable *self);
void xfdashboard_stylable_set_pseudo_classes(XfdashboardStylable *self, const gchar *inClasses);
gboolean xfdashboard_stylable_has_pseudo_class(XfdashboardStylable *self, const gchar *inClass);
void xfdashboard_stylable_add_pseudo_class(XfdashboardStylable *self, const gchar *inClass);
void xfdashboard_stylable_remove_pseudo_class(XfdashboardStylable *self, const gchar *inClass);

void xfdashboard_stylable_invalidate(XfdashboardStylable *self);

G_END_DECLS

#endif	/* __LIBXFDASHBOARD_STYLABLE__ */
