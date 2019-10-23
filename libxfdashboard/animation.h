/*
 * animation: A animation for an actor
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

#ifndef __LIBXFDASHBOARD_ANIMATION__
#define __LIBXFDASHBOARD_ANIMATION__

#if !defined(__LIBXFDASHBOARD_H_INSIDE__) && !defined(LIBXFDASHBOARD_COMPILATION)
#error "Only <libxfdashboard/libxfdashboard.h> can be included directly."
#endif

#include <clutter/clutter.h>

#include <libxfdashboard/actor.h>

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
 */struct _XfdashboardAnimation
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
	void (*started)(XfdashboardAnimation *self);
	void (*stopped)(XfdashboardAnimation *self);
};

/* Public API */
GType xfdashboard_animation_get_type(void) G_GNUC_CONST;

XfdashboardAnimation* xfdashboard_animation_new(XfdashboardActor *inSender, const gchar *inSignal);

const gchar* xfdashboard_animation_get_id(XfdashboardAnimation *self);

/**
 * XfdashboardAnimationDoneCallback:
 * @inAnimation: The animation which completed
 * @inUserData: Data passed to the function, set with xfdashboard_animation_run()
 *
 * A callback called when animation, started by xfdashboard_animation_run(),
 * has completed and will be destroyed.
 */
typedef void (*XfdashboardAnimationDoneCallback)(XfdashboardAnimation *inAnimation, gpointer inUserData);

void xfdashboard_animation_run(XfdashboardAnimation *self,
								XfdashboardAnimationDoneCallback inCallback,
								gpointer inUserData);

G_END_DECLS

#endif	/* __LIBXFDASHBOARD_ANIMATION__ */
