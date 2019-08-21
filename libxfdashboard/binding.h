/*
 * binding: A keyboard or pointer binding
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

#ifndef __LIBXFDASHBOARD_BINDING__
#define __LIBXFDASHBOARD_BINDING__

#if !defined(__LIBXFDASHBOARD_H_INSIDE__) && !defined(LIBXFDASHBOARD_COMPILATION)
#error "Only <libxfdashboard/libxfdashboard.h> can be included directly."
#endif

#include <clutter/clutter.h>

G_BEGIN_DECLS

#define XFDASHBOARD_TYPE_BINDING				(xfdashboard_binding_get_type())
#define XFDASHBOARD_BINDING(obj)				(G_TYPE_CHECK_INSTANCE_CAST((obj), XFDASHBOARD_TYPE_BINDING, XfdashboardBinding))
#define XFDASHBOARD_IS_BINDING(obj)				(G_TYPE_CHECK_INSTANCE_TYPE((obj), XFDASHBOARD_TYPE_BINDING))
#define XFDASHBOARD_BINDING_CLASS(klass)		(G_TYPE_CHECK_CLASS_CAST((klass), XFDASHBOARD_TYPE_BINDING, XfdashboardBindingClass))
#define XFDASHBOARD_IS_BINDING_CLASS(klass)		(G_TYPE_CHECK_CLASS_TYPE((klass), XFDASHBOARD_TYPE_BINDING))
#define XFDASHBOARD_BINDING_GET_CLASS(obj)		(G_TYPE_INSTANCE_GET_CLASS((obj), XFDASHBOARD_TYPE_BINDING, XfdashboardBindingClass))

typedef struct _XfdashboardBinding				XfdashboardBinding;
typedef struct _XfdashboardBindingClass			XfdashboardBindingClass;
typedef struct _XfdashboardBindingPrivate		XfdashboardBindingPrivate;

struct _XfdashboardBinding
{
	/*< private >*/
	/* Parent instance */
	GObject							parent_instance;

	/* Private structure */
	XfdashboardBindingPrivate		*priv;
};

struct _XfdashboardBindingClass
{
	/*< private >*/
	/* Parent class */
	GObjectClass					parent_class;
};

/* Public API */
#define XFDASHBOARD_BINDING_MODIFIERS_MASK		(CLUTTER_SHIFT_MASK | \
													CLUTTER_CONTROL_MASK | \
													CLUTTER_MOD1_MASK | \
													CLUTTER_MOD2_MASK | \
													CLUTTER_MOD3_MASK | \
													CLUTTER_MOD4_MASK | \
													CLUTTER_MOD5_MASK | \
													CLUTTER_SUPER_MASK | \
													CLUTTER_HYPER_MASK | \
													CLUTTER_META_MASK)

typedef enum /*< flags,prefix=XFDASHBOARD_BINDING_FLAGS >*/
{
	XFDASHBOARD_BINDING_FLAGS_NONE=0,
	XFDASHBOARD_BINDING_FLAGS_ALLOW_UNFOCUSABLE_TARGET=1 << 0,
} XfdashboardBindingFlags;

GType xfdashboard_binding_get_type(void) G_GNUC_CONST;

XfdashboardBinding* xfdashboard_binding_new(void);
XfdashboardBinding* xfdashboard_binding_new_for_event(const ClutterEvent *inEvent);

guint xfdashboard_binding_hash(gconstpointer inValue);
gboolean xfdashboard_binding_compare(gconstpointer inLeft, gconstpointer inRight);

ClutterEventType xfdashboard_binding_get_event_type(const XfdashboardBinding *self);
void xfdashboard_binding_set_event_type(XfdashboardBinding *self, ClutterEventType inType);

const gchar* xfdashboard_binding_get_class_name(const XfdashboardBinding *self);
void xfdashboard_binding_set_class_name(XfdashboardBinding *self, const gchar *inClassName);

guint xfdashboard_binding_get_key(const XfdashboardBinding *self);
void xfdashboard_binding_set_key(XfdashboardBinding *self, guint inKey);

ClutterModifierType xfdashboard_binding_get_modifiers(const XfdashboardBinding *self);
void xfdashboard_binding_set_modifiers(XfdashboardBinding *self, ClutterModifierType inModifiers);

const gchar* xfdashboard_binding_get_target(const XfdashboardBinding *self);
void xfdashboard_binding_set_target(XfdashboardBinding *self, const gchar *inTarget);

const gchar* xfdashboard_binding_get_action(const XfdashboardBinding *self);
void xfdashboard_binding_set_action(XfdashboardBinding *self, const gchar *inAction);

XfdashboardBindingFlags xfdashboard_binding_get_flags(const XfdashboardBinding *self);
void xfdashboard_binding_set_flags(XfdashboardBinding *self, XfdashboardBindingFlags inFlags);

G_END_DECLS

#endif	/* __LIBXFDASHBOARD_BINDING__ */
