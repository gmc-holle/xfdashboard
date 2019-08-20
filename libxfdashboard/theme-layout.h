/*
 * theme-layout: A theme used for build and layout objects by XML files
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

#ifndef __LIBXFDASHBOARD_THEME_LAYOUT__
#define __LIBXFDASHBOARD_THEME_LAYOUT__

#if !defined(__LIBXFDASHBOARD_H_INSIDE__) && !defined(LIBXFDASHBOARD_COMPILATION)
#error "Only <libxfdashboard/libxfdashboard.h> can be included directly."
#endif

#include <clutter/clutter.h>

G_BEGIN_DECLS

/* Public definitions */
/**
 * XfdashboardThemeLayoutBuildGet:
 * @XFDASHBOARD_THEME_LAYOUT_BUILD_GET_FOCUSABLES: Get #GPtrArray of pointer to defined focusable actors. Caller must free returned #GPtrArray with g_ptr_array_unref().
 * @XFDASHBOARD_THEME_LAYOUT_BUILD_GET_SELECTED_FOCUS: Get #ClutterActor which should gain the focus. Caller must unref returned #ClutterActor with g_object_unref().
 *
 * The extra data to fetch when building an object from theme layout.
 */
typedef enum /*< prefix=XFDASHBOARD_THEME_LAYOUT_BUILD_GET >*/
{
	XFDASHBOARD_THEME_LAYOUT_BUILD_GET_FOCUSABLES=0,
	XFDASHBOARD_THEME_LAYOUT_BUILD_GET_SELECTED_FOCUS
} XfdashboardThemeLayoutBuildGet;

/* Object declaration */
#define XFDASHBOARD_TYPE_THEME_LAYOUT				(xfdashboard_theme_layout_get_type())
#define XFDASHBOARD_THEME_LAYOUT(obj)				(G_TYPE_CHECK_INSTANCE_CAST((obj), XFDASHBOARD_TYPE_THEME_LAYOUT, XfdashboardThemeLayout))
#define XFDASHBOARD_IS_THEME_LAYOUT(obj)			(G_TYPE_CHECK_INSTANCE_TYPE((obj), XFDASHBOARD_TYPE_THEME_LAYOUT))
#define XFDASHBOARD_THEME_LAYOUT_CLASS(klass)		(G_TYPE_CHECK_CLASS_CAST((klass), XFDASHBOARD_TYPE_THEME_LAYOUT, XfdashboardThemeLayoutClass))
#define XFDASHBOARD_IS_THEME_LAYOUT_CLASS(klass)	(G_TYPE_CHECK_CLASS_TYPE((klass), XFDASHBOARD_TYPE_THEME_LAYOUT))
#define XFDASHBOARD_THEME_LAYOUT_GET_CLASS(obj)		(G_TYPE_INSTANCE_GET_CLASS((obj), XFDASHBOARD_TYPE_THEME_LAYOUT, XfdashboardThemeLayoutClass))

typedef struct _XfdashboardThemeLayout				XfdashboardThemeLayout;
typedef struct _XfdashboardThemeLayoutClass			XfdashboardThemeLayoutClass;
typedef struct _XfdashboardThemeLayoutPrivate		XfdashboardThemeLayoutPrivate;

/**
 * XfdashboardThemeLayout:
 *
 * The #XfdashboardThemeLayout structure contains only private data and
 * should be accessed using the provided API
 */
struct _XfdashboardThemeLayout
{
	/*< private >*/
	/* Parent instance */
	GObject								parent_instance;

	/* Private structure */
	XfdashboardThemeLayoutPrivate		*priv;
};

/**
 * XfdashboardThemeLayoutClass:
 *
 * The #XfdashboardThemeLayoutClass structure contains only private data
 */
struct _XfdashboardThemeLayoutClass
{
	/*< private >*/
	/* Parent class */
	GObjectClass						parent_class;

	/*< public >*/
	/* Virtual functions */
};

/* Errors */
#define XFDASHBOARD_THEME_LAYOUT_ERROR				(xfdashboard_theme_layout_error_quark())

GQuark xfdashboard_theme_layout_error_quark(void);

typedef enum /*< prefix=XFDASHBOARD_THEME_LAYOUT_ERROR >*/
{
	XFDASHBOARD_THEME_LAYOUT_ERROR_ERROR,
	XFDASHBOARD_THEME_LAYOUT_ERROR_MALFORMED,
} XfdashboardThemeLayoutErrorEnum;

/* Public API */
GType xfdashboard_theme_layout_get_type(void) G_GNUC_CONST;

XfdashboardThemeLayout* xfdashboard_theme_layout_new(void);

gboolean xfdashboard_theme_layout_add_file(XfdashboardThemeLayout *self,
											const gchar *inPath,
											GError **outError);

ClutterActor* xfdashboard_theme_layout_build_interface(XfdashboardThemeLayout *self,
														const gchar *inID,
														...);

G_END_DECLS

#endif	/* __LIBXFDASHBOARD_THEME_LAYOUT__ */
