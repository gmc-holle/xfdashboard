/*
 * theme-effects: A theme used for build effects by XML files
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

#ifndef __LIBXFDASHBOARD_THEME_EFFECTS__
#define __LIBXFDASHBOARD_THEME_EFFECTS__

#if !defined(__LIBXFDASHBOARD_H_INSIDE__) && !defined(LIBXFDASHBOARD_COMPILATION)
#error "Only <libxfdashboard/libxfdashboard.h> can be included directly."
#endif

#include <clutter/clutter.h>

G_BEGIN_DECLS

#define XFDASHBOARD_TYPE_THEME_EFFECTS				(xfdashboard_theme_effects_get_type())
#define XFDASHBOARD_THEME_EFFECTS(obj)				(G_TYPE_CHECK_INSTANCE_CAST((obj), XFDASHBOARD_TYPE_THEME_EFFECTS, XfdashboardThemeEffects))
#define XFDASHBOARD_IS_THEME_EFFECTS(obj)			(G_TYPE_CHECK_INSTANCE_TYPE((obj), XFDASHBOARD_TYPE_THEME_EFFECTS))
#define XFDASHBOARD_THEME_EFFECTS_CLASS(klass)		(G_TYPE_CHECK_CLASS_CAST((klass), XFDASHBOARD_TYPE_THEME_EFFECTS, XfdashboardThemeEffectsClass))
#define XFDASHBOARD_IS_THEME_EFFECTS_CLASS(klass)	(G_TYPE_CHECK_CLASS_TYPE((klass), XFDASHBOARD_TYPE_THEME_EFFECTS))
#define XFDASHBOARD_THEME_EFFECTS_GET_CLASS(obj)		(G_TYPE_INSTANCE_GET_CLASS((obj), XFDASHBOARD_TYPE_THEME_EFFECTS, XfdashboardThemeEffectsClass))

typedef struct _XfdashboardThemeEffects				XfdashboardThemeEffects;
typedef struct _XfdashboardThemeEffectsClass		XfdashboardThemeEffectsClass;
typedef struct _XfdashboardThemeEffectsPrivate		XfdashboardThemeEffectsPrivate;

struct _XfdashboardThemeEffects
{
	/*< private >*/
	/* Parent instance */
	GObject								parent_instance;

	/* Private structure */
	XfdashboardThemeEffectsPrivate		*priv;
};

struct _XfdashboardThemeEffectsClass
{
	/*< private >*/
	/* Parent class */
	GObjectClass						parent_class;

	/*< public >*/
	/* Virtual functions */
};

/* Errors */
#define XFDASHBOARD_THEME_EFFECTS_ERROR				(xfdashboard_theme_effects_error_quark())

GQuark xfdashboard_theme_effects_error_quark(void);

typedef enum /*< prefix=XFDASHBOARD_THEME_EFFECTS_ERROR >*/
{
	XFDASHBOARD_THEME_EFFECTS_ERROR_ERROR,
	XFDASHBOARD_THEME_EFFECTS_ERROR_MALFORMED,
} XfdashboardThemeEffectsErrorEnum;

/* Public API */
GType xfdashboard_theme_effects_get_type(void) G_GNUC_CONST;

XfdashboardThemeEffects* xfdashboard_theme_effects_new(void);

gboolean xfdashboard_theme_effects_add_file(XfdashboardThemeEffects *self,
											const gchar *inPath,
											GError **outError);

ClutterEffect* xfdashboard_theme_effects_create_effect(XfdashboardThemeEffects *self,
														const gchar *inID);

G_END_DECLS

#endif	/* __LIBXFDASHBOARD_THEME_EFFECTS__ */
