/*
 * animation: A animation for an actor
 * 
 * Copyright 2012-2020 Stephan Haller <nomad@froevel.de>
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

#ifndef __LIBXFDASHBOARD_ANIMATION__
#define __LIBXFDASHBOARD_ANIMATION__

#if !defined(__LIBXFDASHBOARD_H_INSIDE__) && !defined(LIBXFDASHBOARD_COMPILATION)
#error "Only <libxfdashboard/libxfdashboard.h> can be included directly."
#endif

#include <clutter/clutter.h>

#include <libxfdashboard/actor.h>
#include <libxfdashboard/css-selector.h>


G_BEGIN_DECLS

/* Object declaration */
#define XFDASHBOARD_TYPE_ANIMATION				(xfdashboard_animation_get_type())
#define XFDASHBOARD_ANIMATION(obj)				(G_TYPE_CHECK_INSTANCE_CAST((obj), XFDASHBOARD_TYPE_ANIMATION, XfdashboardAnimation))
#define XFDASHBOARD_IS_ANIMATION(obj)			(G_TYPE_CHECK_INSTANCE_TYPE((obj), XFDASHBOARD_TYPE_ANIMATION))
#define XFDASHBOARD_ANIMATION_CLASS(klass)		(G_TYPE_CHECK_CLASS_CAST((klass), XFDASHBOARD_TYPE_ANIMATION, XfdashboardAnimationClass))
#define XFDASHBOARD_IS_ANIMATION_CLASS(klass)	(G_TYPE_CHECK_CLASS_TYPE((klass), XFDASHBOARD_TYPE_ANIMATION))
#define XFDASHBOARD_ANIMATION_GET_CLASS(obj)	(G_TYPE_INSTANCE_GET_CLASS((obj), XFDASHBOARD_TYPE_ANIMATION, XfdashboardAnimationClass))

typedef struct _XfdashboardAnimation			XfdashboardAnimation;
typedef struct _XfdashboardAnimationClass		XfdashboardAnimationClass;
typedef struct _XfdashboardAnimationPrivate		XfdashboardAnimationPrivate;

/**
 * XfdashboardAnimation:
 *
 * The #XfdashboardAnimation structure contains only private data and
 * should be accessed using the provided API
 */
struct _XfdashboardAnimation
{
	/*< private >*/
	/* Parent instance */
	GObject								parent_instance;

	/* Private structure */
	XfdashboardAnimationPrivate			*priv;
};

/**
 * XfdashboardAnimationClass:
 *
 * The #XfdashboardAnimationClass structure contains only private data
 */
struct _XfdashboardAnimationClass
{
	/*< private >*/
	/* Parent class */
	GObjectClass						parent_class;

	/*< public >*/
	/* Virtual functions */
	void (*add_animation)(XfdashboardAnimation *self, ClutterActor *inActor, ClutterTransition *inTransition);
	void (*animation_done)(XfdashboardAnimation *self);
};

/**
 * XfdashboardAnimationValue:
 * @selector: A #XfdashboardCssSelector to find matchhing actors for the property's value in animation
 * @property: A string containing the name of the property this value belongs to
 * @value: A #GValue containing the value for the property
 *
 */
struct _XfdashboardAnimationValue
{
	XfdashboardCssSelector				*selector;
	gchar								*property;
	GValue								*value;
};

typedef struct _XfdashboardAnimationValue		XfdashboardAnimationValue;

/* Public API */
GType xfdashboard_animation_get_type(void) G_GNUC_CONST;

XfdashboardAnimation* xfdashboard_animation_new(XfdashboardActor *inSender, const gchar *inSignal);
XfdashboardAnimation* xfdashboard_animation_new_with_values(XfdashboardActor *inSender,
															const gchar *inSignal,
															XfdashboardAnimationValue **inDefaultInitialValues,
															XfdashboardAnimationValue **inDefaultFinalValues);

XfdashboardAnimation* xfdashboard_animation_new_by_id(XfdashboardActor *inSender, const gchar *inID);
XfdashboardAnimation* xfdashboard_animation_new_by_id_with_values(XfdashboardActor *inSender,
															const gchar *inID,
															XfdashboardAnimationValue **inDefaultInitialValues,
															XfdashboardAnimationValue **inDefaultFinalValues);

const gchar* xfdashboard_animation_get_id(XfdashboardAnimation *self);

gboolean xfdashboard_animation_is_empty(XfdashboardAnimation *self);

void xfdashboard_animation_run(XfdashboardAnimation *self);

void xfdashboard_animation_ensure_complete(XfdashboardAnimation *self);

void xfdashboard_animation_dump(XfdashboardAnimation *self);

G_END_DECLS

#endif	/* __LIBXFDASHBOARD_ANIMATION__ */
