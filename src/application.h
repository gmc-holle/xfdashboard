/*
 * application: Single-instance managing application and single-instance
 *              objects like window manager and so on.
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

#ifndef __XFDASHBOARD_APPLICATION__
#define __XFDASHBOARD_APPLICATION__

#include <gio/gio.h>
#include <xfconf/xfconf.h>

#include "theme.h"

G_BEGIN_DECLS

#define XFDASHBOARD_TYPE_APPLICATION				(xfdashboard_application_get_type())
#define XFDASHBOARD_APPLICATION(obj)				(G_TYPE_CHECK_INSTANCE_CAST((obj), XFDASHBOARD_TYPE_APPLICATION, XfdashboardApplication))
#define XFDASHBOARD_IS_APPLICATION(obj)				(G_TYPE_CHECK_INSTANCE_TYPE((obj), XFDASHBOARD_TYPE_APPLICATION))
#define XFDASHBOARD_APPLICATION_CLASS(klass)		(G_TYPE_CHECK_CLASS_CAST((klass), XFDASHBOARD_TYPE_APPLICATION, XfdashboardApplicationClass))
#define XFDASHBOARD_IS_APPLICATION_CLASS(klass)		(G_TYPE_CHECK_CLASS_TYPE((klass), XFDASHBOARD_TYPE_APPLICATION))
#define XFDASHBOARD_APPLICATION_GET_CLASS(obj)		(G_TYPE_INSTANCE_GET_CLASS((obj), XFDASHBOARD_TYPE_APPLICATION, XfdashboardApplicationClass))

typedef struct _XfdashboardApplication				XfdashboardApplication;
typedef struct _XfdashboardApplicationClass			XfdashboardApplicationClass;
typedef struct _XfdashboardApplicationPrivate		XfdashboardApplicationPrivate;

struct _XfdashboardApplication
{
	/* Parent instance */
	GApplication					parent_instance;

	/* Private structure */
	XfdashboardApplicationPrivate	*priv;
};

struct _XfdashboardApplicationClass
{
	/*< private >*/
	/* Parent class */
	GApplicationClass				parent_class;

	/*< public >*/
	/* Virtual functions */
	void (*suspend)(XfdashboardApplication *self);
	void (*resume)(XfdashboardApplication *self);

	void (*quit)(XfdashboardApplication *self);
	void (*shutdown_final)(XfdashboardApplication *self);

	void (*theme_changed)(XfdashboardApplication *self, XfdashboardTheme *inTheme);
};

/* Public API */
GType xfdashboard_application_get_type(void) G_GNUC_CONST;

XfdashboardApplication* xfdashboard_application_get_default(void);

gboolean xfdashboard_application_is_daemonized(XfdashboardApplication *self);
gboolean xfdashboard_application_is_suspended(XfdashboardApplication *self);

gboolean xfdashboard_application_is_quitting(XfdashboardApplication *self);
void xfdashboard_application_quit(void);
void xfdashboard_application_quit_forced(void);

XfconfChannel* xfdashboard_application_get_xfconf_channel(void);

XfdashboardTheme* xfdashboard_application_get_theme(void);

G_END_DECLS

#endif	/* __XFDASHBOARD_APPLICATION__ */
