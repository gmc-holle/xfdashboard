/*
 * windows-view: A view showing visible windows
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

#ifndef __LIBXFDASHBOARD_WINDOWS_VIEW__
#define __LIBXFDASHBOARD_WINDOWS_VIEW__

#if !defined(__LIBXFDASHBOARD_H_INSIDE__) && !defined(LIBXFDASHBOARD_COMPILATION)
#error "Only <libxfdashboard/libxfdashboard.h> can be included directly."
#endif

#include <libxfdashboard/view.h>
#include <libxfdashboard/focusable.h>

G_BEGIN_DECLS

#define XFDASHBOARD_TYPE_WINDOWS_VIEW				(xfdashboard_windows_view_get_type())
#define XFDASHBOARD_WINDOWS_VIEW(obj)				(G_TYPE_CHECK_INSTANCE_CAST((obj), XFDASHBOARD_TYPE_WINDOWS_VIEW, XfdashboardWindowsView))
#define XFDASHBOARD_IS_WINDOWS_VIEW(obj)			(G_TYPE_CHECK_INSTANCE_TYPE((obj), XFDASHBOARD_TYPE_WINDOWS_VIEW))
#define XFDASHBOARD_WINDOWS_VIEW_CLASS(klass)		(G_TYPE_CHECK_CLASS_CAST((klass), XFDASHBOARD_TYPE_WINDOWS_VIEW, XfdashboardWindowsViewClass))
#define XFDASHBOARD_IS_WINDOWS_VIEW_CLASS(klass)	(G_TYPE_CHECK_CLASS_TYPE((klass), XFDASHBOARD_TYPE_WINDOWS_VIEW))
#define XFDASHBOARD_WINDOWS_VIEW_GET_CLASS(obj)		(G_TYPE_INSTANCE_GET_CLASS((obj), XFDASHBOARD_TYPE_WINDOWS_VIEW, XfdashboardWindowsViewClass))

typedef struct _XfdashboardWindowsView				XfdashboardWindowsView; 
typedef struct _XfdashboardWindowsViewPrivate		XfdashboardWindowsViewPrivate;
typedef struct _XfdashboardWindowsViewClass			XfdashboardWindowsViewClass;

struct _XfdashboardWindowsView
{
	/*< private >*/
	/* Parent instance */
	XfdashboardView					parent_instance;

	/* Private structure */
	XfdashboardWindowsViewPrivate	*priv;
};

struct _XfdashboardWindowsViewClass
{
	/*< private >*/
	/* Parent class */
	XfdashboardViewClass			parent_class;

	/*< public >*/
	/* Virtual functions */

	/* Binding actions */
	gboolean (*window_close)(XfdashboardWindowsView *self,
								XfdashboardFocusable *inSource,
								const gchar *inAction,
								ClutterEvent *inEvent);
	gboolean (*windows_show_numbers)(XfdashboardWindowsView *self,
										XfdashboardFocusable *inSource,
										const gchar *inAction,
										ClutterEvent *inEvent);
	gboolean (*windows_hide_numbers)(XfdashboardWindowsView *self,
										XfdashboardFocusable *inSource,
										const gchar *inAction,
										ClutterEvent *inEvent);
	gboolean (*windows_activate_window_one)(XfdashboardWindowsView *self,
												XfdashboardFocusable *inSource,
												const gchar *inAction,
												ClutterEvent *inEvent);
	gboolean (*windows_activate_window_two)(XfdashboardWindowsView *self,
												XfdashboardFocusable *inSource,
												const gchar *inAction,
												ClutterEvent *inEvent);
	gboolean (*windows_activate_window_three)(XfdashboardWindowsView *self,
												XfdashboardFocusable *inSource,
												const gchar *inAction,
												ClutterEvent *inEvent);
	gboolean (*windows_activate_window_four)(XfdashboardWindowsView *self,
												XfdashboardFocusable *inSource,
												const gchar *inAction,
												ClutterEvent *inEvent);
	gboolean (*windows_activate_window_five)(XfdashboardWindowsView *self,
												XfdashboardFocusable *inSource,
												const gchar *inAction,
												ClutterEvent *inEvent);
	gboolean (*windows_activate_window_six)(XfdashboardWindowsView *self,
												XfdashboardFocusable *inSource,
												const gchar *inAction,
												ClutterEvent *inEvent);
	gboolean (*windows_activate_window_seven)(XfdashboardWindowsView *self,
												XfdashboardFocusable *inSource,
												const gchar *inAction,
												ClutterEvent *inEvent);
	gboolean (*windows_activate_window_eight)(XfdashboardWindowsView *self,
												XfdashboardFocusable *inSource,
												const gchar *inAction,
												ClutterEvent *inEvent);
	gboolean (*windows_activate_window_nine)(XfdashboardWindowsView *self,
												XfdashboardFocusable *inSource,
												const gchar *inAction,
												ClutterEvent *inEvent);
	gboolean (*windows_activate_window_ten)(XfdashboardWindowsView *self,
												XfdashboardFocusable *inSource,
												const gchar *inAction,
												ClutterEvent *inEvent);

};

/* Public API */
GType xfdashboard_windows_view_get_type(void) G_GNUC_CONST;

gfloat xfdashboard_windows_view_get_spacing(XfdashboardWindowsView *self);
void xfdashboard_windows_view_set_spacing(XfdashboardWindowsView *self, const gfloat inSpacing);

gboolean xfdashboard_windows_view_get_prevent_upscaling(XfdashboardWindowsView *self);
void xfdashboard_windows_view_set_prevent_upscaling(XfdashboardWindowsView *self, gboolean inPreventUpscaling);

G_END_DECLS

#endif	/* __LIBXFDASHBOARD_WINDOWS_VIEW__ */
