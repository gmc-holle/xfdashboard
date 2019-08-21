/*
 * applications-view: A view showing all installed applications as menu
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

#ifndef __LIBXFDASHBOARD_APPLICATIONS_VIEW__
#define __LIBXFDASHBOARD_APPLICATIONS_VIEW__

#if !defined(__LIBXFDASHBOARD_H_INSIDE__) && !defined(LIBXFDASHBOARD_COMPILATION)
#error "Only <libxfdashboard/libxfdashboard.h> can be included directly."
#endif

#include <libxfdashboard/view.h>

G_BEGIN_DECLS

#define XFDASHBOARD_TYPE_APPLICATIONS_VIEW				(xfdashboard_applications_view_get_type())
#define XFDASHBOARD_APPLICATIONS_VIEW(obj)				(G_TYPE_CHECK_INSTANCE_CAST((obj), XFDASHBOARD_TYPE_APPLICATIONS_VIEW, XfdashboardApplicationsView))
#define XFDASHBOARD_IS_APPLICATIONS_VIEW(obj)			(G_TYPE_CHECK_INSTANCE_TYPE((obj), XFDASHBOARD_TYPE_APPLICATIONS_VIEW))
#define XFDASHBOARD_APPLICATIONS_VIEW_CLASS(klass)		(G_TYPE_CHECK_CLASS_CAST((klass), XFDASHBOARD_TYPE_APPLICATIONS_VIEW, XfdashboardApplicationsViewClass))
#define XFDASHBOARD_IS_APPLICATIONS_VIEW_CLASS(klass)	(G_TYPE_CHECK_CLASS_TYPE((klass), XFDASHBOARD_TYPE_APPLICATIONS_VIEW))
#define XFDASHBOARD_APPLICATIONS_VIEW_GET_CLASS(obj)	(G_TYPE_INSTANCE_GET_CLASS((obj), XFDASHBOARD_TYPE_APPLICATIONS_VIEW, XfdashboardApplicationsViewClass))

typedef struct _XfdashboardApplicationsView				XfdashboardApplicationsView; 
typedef struct _XfdashboardApplicationsViewPrivate		XfdashboardApplicationsViewPrivate;
typedef struct _XfdashboardApplicationsViewClass		XfdashboardApplicationsViewClass;

struct _XfdashboardApplicationsView
{
	/*< private >*/
	/* Parent instance */
	XfdashboardView						parent_instance;

	/* Private structure */
	XfdashboardApplicationsViewPrivate	*priv;
};

struct _XfdashboardApplicationsViewClass
{
	/*< private >*/
	/* Parent class */
	XfdashboardViewClass				parent_class;
};

/* Public API */
GType xfdashboard_applications_view_get_type(void) G_GNUC_CONST;

XfdashboardViewMode xfdashboard_applications_view_get_view_mode(XfdashboardApplicationsView *self);
void xfdashboard_applications_view_set_view_mode(XfdashboardApplicationsView *self, const XfdashboardViewMode inMode);

gfloat xfdashboard_applications_view_get_spacing(XfdashboardApplicationsView *self);
void xfdashboard_applications_view_set_spacing(XfdashboardApplicationsView *self, const gfloat inSpacing);

const gchar* xfdashboard_applications_view_get_parent_menu_icon(XfdashboardApplicationsView *self);
void xfdashboard_applications_view_set_parent_menu_icon(XfdashboardApplicationsView *self, const gchar *inIconName);

const gchar* xfdashboard_applications_view_get_format_title_only(XfdashboardApplicationsView *self);
void xfdashboard_applications_view_set_format_title_only(XfdashboardApplicationsView *self, const gchar *inFormat);

const gchar* xfdashboard_applications_view_get_format_title_description(XfdashboardApplicationsView *self);
void xfdashboard_applications_view_set_format_title_description(XfdashboardApplicationsView *self, const gchar *inFormat);

gboolean xfdashboard_applications_view_get_show_all_apps(XfdashboardApplicationsView *self);
void xfdashboard_applications_view_set_show_all_apps(XfdashboardApplicationsView *self, gboolean inShowAllApps);

G_END_DECLS

#endif	/* __LIBXFDASHBOARD_APPLICATIONS_VIEW__ */
