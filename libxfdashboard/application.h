/*
 * application: Single-instance managing application and single-instance
 *              objects like window manager and so on.
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

#ifndef __LIBXFDASHBOARD_APPLICATION__
#define __LIBXFDASHBOARD_APPLICATION__

#if !defined(__LIBXFDASHBOARD_H_INSIDE__) && !defined(LIBXFDASHBOARD_COMPILATION)
#error "Only <libxfdashboard/libxfdashboard.h> can be included directly."
#endif

#include <gio/gio.h>
#include <xfconf/xfconf.h>

#include <libxfdashboard/theme.h>
#include <libxfdashboard/focusable.h>
#include <libxfdashboard/stage.h>

G_BEGIN_DECLS

/* Public definitions */
/**
 * XfdashboardApplicationErrorCode:
 * @XFDASHBOARD_APPLICATION_ERROR_NONE: Application started successfully without any problems
 * @XFDASHBOARD_APPLICATION_ERROR_FAILED: Application failed to start
 * @XFDASHBOARD_APPLICATION_ERROR_RESTART: Application needs to be restarted to start-up successfully
 * @XFDASHBOARD_APPLICATION_ERROR_QUIT: Application was quitted and shuts down
 *
 * The start-up status codes returned by XfdashboardApplication.
 */
typedef enum /*< skip,prefix=XFDASHBOARD_APPLICATION_ERROR >*/
{
	XFDASHBOARD_APPLICATION_ERROR_NONE=0,
	XFDASHBOARD_APPLICATION_ERROR_FAILED,
	XFDASHBOARD_APPLICATION_ERROR_RESTART,
	XFDASHBOARD_APPLICATION_ERROR_QUIT
} XfdashboardApplicationErrorCode;


/* Object declaration */
#define XFDASHBOARD_TYPE_APPLICATION				(xfdashboard_application_get_type())
#define XFDASHBOARD_APPLICATION(obj)				(G_TYPE_CHECK_INSTANCE_CAST((obj), XFDASHBOARD_TYPE_APPLICATION, XfdashboardApplication))
#define XFDASHBOARD_IS_APPLICATION(obj)				(G_TYPE_CHECK_INSTANCE_TYPE((obj), XFDASHBOARD_TYPE_APPLICATION))
#define XFDASHBOARD_APPLICATION_CLASS(klass)		(G_TYPE_CHECK_CLASS_CAST((klass), XFDASHBOARD_TYPE_APPLICATION, XfdashboardApplicationClass))
#define XFDASHBOARD_IS_APPLICATION_CLASS(klass)		(G_TYPE_CHECK_CLASS_TYPE((klass), XFDASHBOARD_TYPE_APPLICATION))
#define XFDASHBOARD_APPLICATION_GET_CLASS(obj)		(G_TYPE_INSTANCE_GET_CLASS((obj), XFDASHBOARD_TYPE_APPLICATION, XfdashboardApplicationClass))

typedef struct _XfdashboardApplication				XfdashboardApplication;
typedef struct _XfdashboardApplicationClass			XfdashboardApplicationClass;
typedef struct _XfdashboardApplicationPrivate		XfdashboardApplicationPrivate;

/**
 * XfdashboardApplication:
 *
 * The #XfdashboardApplication structure contains only private data and
 * should be accessed using the provided API
 */
struct _XfdashboardApplication
{
	/*< private >*/
	/* Parent instance */
	GApplication					parent_instance;

	/* Private structure */
	XfdashboardApplicationPrivate	*priv;
};

/**
 * XfdashboardApplicationClass:
 * @initialized: Class handler for the #XfdashboardApplicationClass::initialized signal
 * @suspend: Class handler for the #XfdashboardApplicationClass::suspend signal
 * @resume: Class handler for the #XfdashboardApplicationClass::resume signal
 * @quit: Class handler for the #XfdashboardApplicationClass::quit signal
 * @shutdown_final: Class handler for the #XfdashboardApplicationClass::shutdown_final signal
 * @theme_changed: Class handler for the #XfdashboardApplicationClass::theme_changed signal
 * @application_launched: Class handler for the #XfdashboardApplicationClass::application_launched signal
 * @exit: Class handler for the #XfdashboardApplicationClass::exit signal
 *
 * The #XfdashboardApplicationClass structure contains only private data
 */
struct _XfdashboardApplicationClass
{
	/*< private >*/
	/* Parent class */
	GApplicationClass				parent_class;

	/*< public >*/
	/* Virtual functions */
	void (*initialized)(XfdashboardApplication *self);

	void (*suspend)(XfdashboardApplication *self);
	void (*resume)(XfdashboardApplication *self);

	void (*quit)(XfdashboardApplication *self);
	void (*shutdown_final)(XfdashboardApplication *self);

	void (*theme_loading)(XfdashboardApplication *self, XfdashboardTheme *inTheme);
	void (*theme_loaded)(XfdashboardApplication *self, XfdashboardTheme *inTheme);
	void (*theme_changed)(XfdashboardApplication *self, XfdashboardTheme *inTheme);

	void (*application_launched)(XfdashboardApplication *self, GAppInfo *inAppInfo);

	/* Binding actions */
	gboolean (*exit)(XfdashboardApplication *self,
						XfdashboardFocusable *inSource,
						const gchar *inAction,
						ClutterEvent *inEvent);
};


/* Public API */
GType xfdashboard_application_get_type(void) G_GNUC_CONST;

gboolean xfdashboard_application_has_default(void);
XfdashboardApplication* xfdashboard_application_get_default(void);

gboolean xfdashboard_application_is_daemonized(XfdashboardApplication *self);
gboolean xfdashboard_application_is_suspended(XfdashboardApplication *self);

gboolean xfdashboard_application_is_quitting(XfdashboardApplication *self);
void xfdashboard_application_resume(XfdashboardApplication *self);
void xfdashboard_application_suspend_or_quit(XfdashboardApplication *self);
void xfdashboard_application_quit_forced(XfdashboardApplication *self);

XfdashboardStage* xfdashboard_application_get_stage(XfdashboardApplication *self);
XfdashboardTheme* xfdashboard_application_get_theme(XfdashboardApplication *self);

XfconfChannel* xfdashboard_application_get_xfconf_channel(XfdashboardApplication *self);

G_END_DECLS

#endif	/* __LIBXFDASHBOARD_APPLICATION__ */
