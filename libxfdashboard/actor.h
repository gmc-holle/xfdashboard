/*
 * actor: Abstract base actor
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

#ifndef __LIBXFDASHBOARD_ACTOR__
#define __LIBXFDASHBOARD_ACTOR__

#if !defined(__LIBXFDASHBOARD_H_INSIDE__) && !defined(LIBXFDASHBOARD_COMPILATION)
#error "Only <libxfdashboard/libxfdashboard.h> can be included directly."
#endif

#include <clutter/clutter.h>

G_BEGIN_DECLS

#define XFDASHBOARD_TYPE_ACTOR					(xfdashboard_actor_get_type())
#define XFDASHBOARD_ACTOR(obj)					(G_TYPE_CHECK_INSTANCE_CAST((obj), XFDASHBOARD_TYPE_ACTOR, XfdashboardActor))
#define XFDASHBOARD_IS_ACTOR(obj)				(G_TYPE_CHECK_INSTANCE_TYPE((obj), XFDASHBOARD_TYPE_ACTOR))
#define XFDASHBOARD_ACTOR_CLASS(klass)			(G_TYPE_CHECK_CLASS_CAST((klass), XFDASHBOARD_TYPE_ACTOR, XfdashboardActorClass))
#define XFDASHBOARD_IS_ACTOR_CLASS(klass)		(G_TYPE_CHECK_CLASS_TYPE((klass), XFDASHBOARD_TYPE_ACTOR))
#define XFDASHBOARD_ACTOR_GET_CLASS(obj)		(G_TYPE_INSTANCE_GET_CLASS((obj), XFDASHBOARD_TYPE_ACTOR, XfdashboardActorClass))

typedef struct _XfdashboardActor				XfdashboardActor;
typedef struct _XfdashboardActorClass			XfdashboardActorClass;
typedef struct _XfdashboardActorPrivate			XfdashboardActorPrivate;

struct _XfdashboardActor
{
	/*< private >*/
	/* Parent instance */
	ClutterActor				parent_instance;

	/* Private structure */
	XfdashboardActorPrivate		*priv;
};

struct _XfdashboardActorClass
{
	/*< private >*/
	/* Parent class */
	ClutterActorClass			parent_class;

	/*< public >*/
	/* Virtual functions */
};

/* Public API */
GType xfdashboard_actor_get_type(void) G_GNUC_CONST;

ClutterActor* xfdashboard_actor_new(void);

gboolean xfdashboard_actor_get_can_focus(XfdashboardActor *self);
void xfdashboard_actor_set_can_focus(XfdashboardActor *self, gboolean inCanFous);

const gchar* xfdashboard_actor_get_effects(XfdashboardActor *self);
void xfdashboard_actor_set_effects(XfdashboardActor *self, const gchar *inEffects);

void xfdashboard_actor_install_stylable_property(XfdashboardActorClass *klass, GParamSpec *inParamSpec);
void xfdashboard_actor_install_stylable_property_by_name(XfdashboardActorClass *klass, const gchar *inParamName);
GHashTable* xfdashboard_actor_get_stylable_properties(XfdashboardActorClass *klass);
GHashTable* xfdashboard_actor_get_stylable_properties_full(XfdashboardActorClass *klass);

void xfdashboard_actor_invalidate(XfdashboardActor *self);

G_END_DECLS

#endif
