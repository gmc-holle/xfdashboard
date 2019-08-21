/*
 * theme-css: A theme used for rendering xfdashboard actors with CSS.
 *            The parser and the handling of CSS files is heavily based
 *            on mx-css, mx-style and mx-stylable of library mx
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

#ifndef __LIBXFDASHBOARD_THEME_CSS__
#define __LIBXFDASHBOARD_THEME_CSS__

#if !defined(__LIBXFDASHBOARD_H_INSIDE__) && !defined(LIBXFDASHBOARD_COMPILATION)
#error "Only <libxfdashboard/libxfdashboard.h> can be included directly."
#endif

#include <glib-object.h>

#include <libxfdashboard/stylable.h>

G_BEGIN_DECLS

#define XFDASHBOARD_TYPE_THEME_CSS					(xfdashboard_theme_css_get_type())
#define XFDASHBOARD_THEME_CSS(obj)					(G_TYPE_CHECK_INSTANCE_CAST((obj), XFDASHBOARD_TYPE_THEME_CSS, XfdashboardThemeCSS))
#define XFDASHBOARD_IS_THEME_CSS(obj)				(G_TYPE_CHECK_INSTANCE_TYPE((obj), XFDASHBOARD_TYPE_THEME_CSS))
#define XFDASHBOARD_THEME_CSS_CLASS(klass)			(G_TYPE_CHECK_CLASS_CAST((klass), XFDASHBOARD_TYPE_THEME_CSS, XfdashboardThemeCSSClass))
#define XFDASHBOARD_IS_THEME_CSS_CLASS(klass)		(G_TYPE_CHECK_CLASS_TYPE((klass), XFDASHBOARD_TYPE_THEME_CSS))
#define XFDASHBOARD_THEME_CSS_GET_CLASS(obj)		(G_TYPE_INSTANCE_GET_CLASS((obj), XFDASHBOARD_TYPE_THEME_CSS, XfdashboardThemeCSSClass))

typedef struct _XfdashboardThemeCSS					XfdashboardThemeCSS;
typedef struct _XfdashboardThemeCSSClass			XfdashboardThemeCSSClass;
typedef struct _XfdashboardThemeCSSPrivate			XfdashboardThemeCSSPrivate;

struct _XfdashboardThemeCSS
{
	/*< private >*/
	/* Parent instance */
	GObject							parent_instance;

	/* Private structure */
	XfdashboardThemeCSSPrivate		*priv;
};

struct _XfdashboardThemeCSSClass
{
	/*< private >*/
	/* Parent class */
	GObjectClass					parent_class;

	/*< public >*/
	/* Virtual functions */
};

/* Errors */
#define XFDASHBOARD_THEME_CSS_ERROR					(xfdashboard_theme_css_error_quark())

GQuark xfdashboard_theme_css_error_quark(void);

typedef enum /*< prefix=XFDASHBOARD_THEME_CSS_ERROR >*/
{
	XFDASHBOARD_THEME_CSS_ERROR_INVALID_ARGUMENT,
	XFDASHBOARD_THEME_CSS_ERROR_UNSUPPORTED_STREAM,
	XFDASHBOARD_THEME_CSS_ERROR_PARSER_ERROR,
	XFDASHBOARD_THEME_CSS_ERROR_FUNCTION_ERROR
} XfdashboardThemeCSSErrorEnum;

/* Public declarations */
typedef struct _XfdashboardThemeCSSValue			XfdashboardThemeCSSValue;
struct _XfdashboardThemeCSSValue
{
	const gchar						*string;
	const gchar						*source;
};

/* Public API */
GType xfdashboard_theme_css_get_type(void) G_GNUC_CONST;

XfdashboardThemeCSS* xfdashboard_theme_css_new(const gchar *inThemePath);

gboolean xfdashboard_theme_css_add_file(XfdashboardThemeCSS *self,
											const gchar *inPath,
											gint inPriority,
											GError **outError);

GHashTable* xfdashboard_theme_css_get_properties(XfdashboardThemeCSS *self,
													XfdashboardStylable *inStylable);

G_END_DECLS

#endif	/* __LIBXFDASHBOARD_THEME_CSS__ */
