/*
 * theme-animation: A theme used for building animations by XML files
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

#ifndef __LIBXFDASHBOARD_THEME_ANIMATION__
#define __LIBXFDASHBOARD_THEME_ANIMATION__

#if !defined(__LIBXFDASHBOARD_H_INSIDE__) && !defined(LIBXFDASHBOARD_COMPILATION)
#error "Only <libxfdashboard/libxfdashboard.h> can be included directly."
#endif

#include <clutter/clutter.h>

#include <libxfdashboard/actor.h>
#include <libxfdashboard/animation.h>

G_BEGIN_DECLS


/* Object declaration */
#define XFDASHBOARD_TYPE_THEME_ANIMATION				(xfdashboard_theme_animation_get_type())
#define XFDASHBOARD_THEME_ANIMATION(obj)				(G_TYPE_CHECK_INSTANCE_CAST((obj), XFDASHBOARD_TYPE_THEME_ANIMATION, XfdashboardThemeAnimation))
#define XFDASHBOARD_IS_THEME_ANIMATION(obj)				(G_TYPE_CHECK_INSTANCE_TYPE((obj), XFDASHBOARD_TYPE_THEME_ANIMATION))
#define XFDASHBOARD_THEME_ANIMATION_CLASS(klass)		(G_TYPE_CHECK_CLASS_CAST((klass), XFDASHBOARD_TYPE_THEME_ANIMATION, XfdashboardThemeAnimationClass))
#define XFDASHBOARD_IS_THEME_ANIMATION_CLASS(klass)		(G_TYPE_CHECK_CLASS_TYPE((klass), XFDASHBOARD_TYPE_THEME_ANIMATION))
#define XFDASHBOARD_THEME_ANIMATION_GET_CLASS(obj)		(G_TYPE_INSTANCE_GET_CLASS((obj), XFDASHBOARD_TYPE_THEME_ANIMATION, XfdashboardThemeAnimationClass))

typedef struct _XfdashboardThemeAnimation				XfdashboardThemeAnimation;
typedef struct _XfdashboardThemeAnimationClass			XfdashboardThemeAnimationClass;
typedef struct _XfdashboardThemeAnimationPrivate		XfdashboardThemeAnimationPrivate;

/**
 * XfdashboardThemeAnimation:
 *
 * The #XfdashboardThemeAnimation structure contains only private data and
 * should be accessed using the provided API
 */
struct _XfdashboardThemeAnimation
{
	/*< private >*/
	/* Parent instance */
	GObject									parent_instance;

	/* Private structure */
	XfdashboardThemeAnimationPrivate		*priv;
};

/**
 * XfdashboardThemeAnimationClass:
 *
 * The #XfdashboardThemeAnimationClass structure contains only private data
 */
struct _XfdashboardThemeAnimationClass
{
	/*< private >*/
	/* Parent class */
	GObjectClass							parent_class;

	/*< public >*/
	/* Virtual functions */
};

/* Errors */
#define XFDASHBOARD_THEME_ANIMATION_ERROR				(xfdashboard_theme_animation_error_quark())

GQuark xfdashboard_theme_animation_error_quark(void);

typedef enum /*< prefix=XFDASHBOARD_THEME_ANIMATION_ERROR >*/
{
	XFDASHBOARD_THEME_ANIMATION_ERROR_ERROR,
	XFDASHBOARD_THEME_ANIMATION_ERROR_MALFORMED,
} XfdashboardThemeAnimationErrorEnum;

/* Public API */
GType xfdashboard_theme_animation_get_type(void) G_GNUC_CONST;

XfdashboardThemeAnimation* xfdashboard_theme_animation_new(void);

gboolean xfdashboard_theme_animation_add_file(XfdashboardThemeAnimation *self,
											const gchar *inPath,
											GError **outError);

XfdashboardAnimation* xfdashboard_theme_animation_create(XfdashboardThemeAnimation *self,
															XfdashboardActor *inSender,
															const gchar *inSignal);

G_END_DECLS

#endif	/* __LIBXFDASHBOARD_THEME_ANIMATION__ */
