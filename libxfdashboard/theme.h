/*
 * theme: Top-level theme object (parses key file and manages loading
 *        resources like css style files, xml layout files etc.)
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

#ifndef __LIBXFDASHBOARD_THEME__
#define __LIBXFDASHBOARD_THEME__

#if !defined(__LIBXFDASHBOARD_H_INSIDE__) && !defined(LIBXFDASHBOARD_COMPILATION)
#error "Only <libxfdashboard/libxfdashboard.h> can be included directly."
#endif

#include <clutter/clutter.h>

#include <libxfdashboard/theme-css.h>
#include <libxfdashboard/theme-layout.h>
#include <libxfdashboard/theme-effects.h>
#include <libxfdashboard/theme-animation.h>

G_BEGIN_DECLS

#define XFDASHBOARD_TYPE_THEME					(xfdashboard_theme_get_type())
#define XFDASHBOARD_THEME(obj)					(G_TYPE_CHECK_INSTANCE_CAST((obj), XFDASHBOARD_TYPE_THEME, XfdashboardTheme))
#define XFDASHBOARD_IS_THEME(obj)				(G_TYPE_CHECK_INSTANCE_TYPE((obj), XFDASHBOARD_TYPE_THEME))
#define XFDASHBOARD_THEME_CLASS(klass)			(G_TYPE_CHECK_CLASS_CAST((klass), XFDASHBOARD_TYPE_THEME, XfdashboardThemeClass))
#define XFDASHBOARD_IS_THEME_CLASS(klass)		(G_TYPE_CHECK_CLASS_TYPE((klass), XFDASHBOARD_TYPE_THEME))
#define XFDASHBOARD_THEME_GET_CLASS(obj)		(G_TYPE_INSTANCE_GET_CLASS((obj), XFDASHBOARD_TYPE_THEME, XfdashboardThemeClass))

typedef struct _XfdashboardTheme				XfdashboardTheme;
typedef struct _XfdashboardThemeClass			XfdashboardThemeClass;
typedef struct _XfdashboardThemePrivate			XfdashboardThemePrivate;

struct _XfdashboardTheme
{
	/*< private >*/
	/* Parent instance */
	GObject						parent_instance;

	/* Private structure */
	XfdashboardThemePrivate		*priv;
};

struct _XfdashboardThemeClass
{
	/*< private >*/
	/* Parent class */
	GObjectClass				parent_class;

	/*< public >*/
	/* Virtual functions */
};

/* Errors */
#define XFDASHBOARD_THEME_ERROR					(xfdashboard_theme_error_quark())

GQuark xfdashboard_theme_error_quark(void);

typedef enum /*< prefix=XFDASHBOARD_THEME_ERROR >*/
{
	XFDASHBOARD_THEME_ERROR_THEME_NOT_FOUND,
	XFDASHBOARD_THEME_ERROR_ALREADY_LOADED
} XfdashboardThemeErrorEnum;

/* Public API */
GType xfdashboard_theme_get_type(void) G_GNUC_CONST;

XfdashboardTheme* xfdashboard_theme_new(const gchar *inThemeName);

const gchar* xfdashboard_theme_get_path(XfdashboardTheme *self);

const gchar* xfdashboard_theme_get_theme_name(XfdashboardTheme *self);
const gchar* xfdashboard_theme_get_display_name(XfdashboardTheme *self);
const gchar* xfdashboard_theme_get_comment(XfdashboardTheme *self);

gboolean xfdashboard_theme_load(XfdashboardTheme *self,
								GError **outError);

XfdashboardThemeCSS* xfdashboard_theme_get_css(XfdashboardTheme *self);
XfdashboardThemeLayout* xfdashboard_theme_get_layout(XfdashboardTheme *self);
XfdashboardThemeEffects* xfdashboard_theme_get_effects(XfdashboardTheme *self);
XfdashboardThemeAnimation* xfdashboard_theme_get_animation(XfdashboardTheme *self);

G_END_DECLS

#endif	/* __LIBXFDASHBOARD_THEME__ */
