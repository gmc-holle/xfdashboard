/*
 * quicklaunch: Quicklaunch box
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

#ifndef __LIBXFDASHBOARD_QUICKLAUNCH__
#define __LIBXFDASHBOARD_QUICKLAUNCH__

#if !defined(__LIBXFDASHBOARD_H_INSIDE__) && !defined(LIBXFDASHBOARD_COMPILATION)
#error "Only <libxfdashboard/libxfdashboard.h> can be included directly."
#endif

#include <libxfdashboard/background.h>
#include <libxfdashboard/toggle-button.h>
#include <libxfdashboard/focusable.h>

G_BEGIN_DECLS

#define XFDASHBOARD_TYPE_QUICKLAUNCH				(xfdashboard_quicklaunch_get_type())
#define XFDASHBOARD_QUICKLAUNCH(obj)				(G_TYPE_CHECK_INSTANCE_CAST((obj), XFDASHBOARD_TYPE_QUICKLAUNCH, XfdashboardQuicklaunch))
#define XFDASHBOARD_IS_QUICKLAUNCH(obj)				(G_TYPE_CHECK_INSTANCE_TYPE((obj), XFDASHBOARD_TYPE_QUICKLAUNCH))
#define XFDASHBOARD_QUICKLAUNCH_CLASS(klass)		(G_TYPE_CHECK_CLASS_CAST((klass), XFDASHBOARD_TYPE_QUICKLAUNCH, XfdashboardQuicklaunchClass))
#define XFDASHBOARD_IS_QUICKLAUNCH_CLASS(klass)		(G_TYPE_CHECK_CLASS_TYPE((klass), XFDASHBOARD_TYPE_QUICKLAUNCH))
#define XFDASHBOARD_QUICKLAUNCH_GET_CLASS(obj)		(G_TYPE_INSTANCE_GET_CLASS((obj), XFDASHBOARD_TYPE_QUICKLAUNCH, XfdashboardQuicklaunchClass))

typedef struct _XfdashboardQuicklaunch				XfdashboardQuicklaunch;
typedef struct _XfdashboardQuicklaunchClass			XfdashboardQuicklaunchClass;
typedef struct _XfdashboardQuicklaunchPrivate		XfdashboardQuicklaunchPrivate;

struct _XfdashboardQuicklaunch
{
	/*< private >*/
	/* Parent instance */
	XfdashboardBackground			parent_instance;

	/* Private structure */
	XfdashboardQuicklaunchPrivate	*priv;
};

struct _XfdashboardQuicklaunchClass
{
	/*< private >*/
	/* Parent class */
	XfdashboardBackgroundClass		parent_class;

	/*< public >*/
	/* Virtual functions */
	void (*favourite_added)(XfdashboardQuicklaunch *self, GAppInfo *inAppInfo);
	void (*favourite_removed)(XfdashboardQuicklaunch *self, GAppInfo *inAppInfo);

	/* Binding actions */
	gboolean (*selection_add_favourite)(XfdashboardQuicklaunch *self,
											XfdashboardFocusable *inSource,
											const gchar *inAction,
											ClutterEvent *inEvent);
	gboolean (*selection_remove_favourite)(XfdashboardQuicklaunch *self,
											XfdashboardFocusable *inSource,
											const gchar *inAction,
											ClutterEvent *inEvent);

	gboolean (*favourite_reorder_left)(XfdashboardQuicklaunch *self,
											XfdashboardFocusable *inSource,
											const gchar *inAction,
											ClutterEvent *inEvent);
	gboolean (*favourite_reorder_right)(XfdashboardQuicklaunch *self,
											XfdashboardFocusable *inSource,
											const gchar *inAction,
											ClutterEvent *inEvent);
	gboolean (*favourite_reorder_up)(XfdashboardQuicklaunch *self,
											XfdashboardFocusable *inSource,
											const gchar *inAction,
											ClutterEvent *inEvent);
	gboolean (*favourite_reorder_down)(XfdashboardQuicklaunch *self,
											XfdashboardFocusable *inSource,
											const gchar *inAction,
											ClutterEvent *inEvent);
};

/* Public API */
GType xfdashboard_quicklaunch_get_type(void) G_GNUC_CONST;

ClutterActor* xfdashboard_quicklaunch_new(void);
ClutterActor* xfdashboard_quicklaunch_new_with_orientation(ClutterOrientation inOrientation);

gfloat xfdashboard_quicklaunch_get_normal_icon_size(XfdashboardQuicklaunch *self);
void xfdashboard_quicklaunch_set_normal_icon_size(XfdashboardQuicklaunch *self, const gfloat inIconSize);

gfloat xfdashboard_quicklaunch_get_spacing(XfdashboardQuicklaunch *self);
void xfdashboard_quicklaunch_set_spacing(XfdashboardQuicklaunch *self, const gfloat inSpacing);

ClutterOrientation xfdashboard_quicklaunch_get_orientation(XfdashboardQuicklaunch *self);
void xfdashboard_quicklaunch_set_orientation(XfdashboardQuicklaunch *self, ClutterOrientation inOrientation);

XfdashboardToggleButton* xfdashboard_quicklaunch_get_apps_button(XfdashboardQuicklaunch *self);

G_END_DECLS

#endif	/* __LIBXFDASHBOARD_QUICKLAUNCH__ */
