/*
 * theme-layout: A theme used for build and layout objects by XML files
 * 
 * Copyright 2012-2015 Stephan Haller <nomad@froevel.de>
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

#ifndef __XFDASHBOARD_THEME_LAYOUT__
#define __XFDASHBOARD_THEME_LAYOUT__

#include <clutter/clutter.h>

G_BEGIN_DECLS

#define XFDASHBOARD_TYPE_THEME_LAYOUT				(xfdashboard_theme_layout_get_type())
#define XFDASHBOARD_THEME_LAYOUT(obj)				(G_TYPE_CHECK_INSTANCE_CAST((obj), XFDASHBOARD_TYPE_THEME_LAYOUT, XfdashboardThemeLayout))
#define XFDASHBOARD_IS_THEME_LAYOUT(obj)			(G_TYPE_CHECK_INSTANCE_TYPE((obj), XFDASHBOARD_TYPE_THEME_LAYOUT))
#define XFDASHBOARD_THEME_LAYOUT_CLASS(klass)		(G_TYPE_CHECK_CLASS_CAST((klass), XFDASHBOARD_TYPE_THEME_LAYOUT, XfdashboardThemeLayoutClass))
#define XFDASHBOARD_IS_THEME_LAYOUT_CLASS(klass)	(G_TYPE_CHECK_CLASS_TYPE((klass), XFDASHBOARD_TYPE_THEME_LAYOUT))
#define XFDASHBOARD_THEME_LAYOUT_GET_CLASS(obj)		(G_TYPE_INSTANCE_GET_CLASS((obj), XFDASHBOARD_TYPE_THEME_LAYOUT, XfdashboardThemeLayoutClass))

typedef struct _XfdashboardThemeLayout				XfdashboardThemeLayout;
typedef struct _XfdashboardThemeLayoutClass			XfdashboardThemeLayoutClass;
typedef struct _XfdashboardThemeLayoutPrivate		XfdashboardThemeLayoutPrivate;

struct _XfdashboardThemeLayout
{
	/* Parent instance */
	GObject								parent_instance;

	/* Private structure */
	XfdashboardThemeLayoutPrivate		*priv;
};

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
														const gchar *inID);

G_END_DECLS

#endif	/* __XFDASHBOARD_THEME_LAYOUT__ */
