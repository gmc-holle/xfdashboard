/*
 * core: Single-instance core library object managing the library and
 *       single-instance objects like window manager and so on.
 * 
 * Copyright 2012-2020 Stephan Haller <nomad@froevel.de>
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

#ifndef __LIBXFDASHBOARD_CORE__
#define __LIBXFDASHBOARD_CORE__

#if !defined(__LIBXFDASHBOARD_H_INSIDE__) && !defined(LIBXFDASHBOARD_COMPILATION)
#error "Only <libxfdashboard/libxfdashboard.h> can be included directly."
#endif

#include <gio/gio.h>

#include <libxfdashboard/theme.h>
#include <libxfdashboard/focusable.h>
#include <libxfdashboard/stage.h>
#include <libxfdashboard/settings.h>
#include <libxfdashboard/application-database.h>
#include <libxfdashboard/application-tracker.h>
#include <libxfdashboard/bindings-pool.h>
#include <libxfdashboard/focus-manager.h>
#include <libxfdashboard/plugins-manager.h>
#include <libxfdashboard/search-manager.h>
#include <libxfdashboard/view-manager.h>
#include <libxfdashboard/window-tracker-backend.h>
#include <libxfdashboard/window-tracker.h>

G_BEGIN_DECLS


/* Object declaration */
#define XFDASHBOARD_TYPE_CORE					(xfdashboard_core_get_type())
#define XFDASHBOARD_CORE(obj)					(G_TYPE_CHECK_INSTANCE_CAST((obj), XFDASHBOARD_TYPE_CORE, XfdashboardCore))
#define XFDASHBOARD_IS_CORE(obj)				(G_TYPE_CHECK_INSTANCE_TYPE((obj), XFDASHBOARD_TYPE_CORE))
#define XFDASHBOARD_CORE_CLASS(klass)			(G_TYPE_CHECK_CLASS_CAST((klass), XFDASHBOARD_TYPE_CORE, XfdashboardCoreClass))
#define XFDASHBOARD_IS_CORE_CLASS(klass)		(G_TYPE_CHECK_CLASS_TYPE((klass), XFDASHBOARD_TYPE_CORE))
#define XFDASHBOARD_CORE_GET_CLASS(obj)			(G_TYPE_INSTANCE_GET_CLASS((obj), XFDASHBOARD_TYPE_CORE, XfdashboardCoreClass))

typedef struct _XfdashboardCore					XfdashboardCore;
typedef struct _XfdashboardCoreClass			XfdashboardCoreClass;
typedef struct _XfdashboardCorePrivate			XfdashboardCorePrivate;

/**
 * XfdashboardCore:
 *
 * The #XfdashboardCore structure contains only private data and
 * should be accessed using the provided API
 */
struct _XfdashboardCore
{
	/*< private >*/
	/* Parent instance */
	GObject							parent_instance;

	/* Private structure */
	XfdashboardCorePrivate			*priv;
};

/**
 * XfdashboardCoreClass:
 * @initialized: Class handler for the #XfdashboardCoreClass::initialized signal
 * @suspend: Class handler for the #XfdashboardCoreClass::suspend signal
 * @resume: Class handler for the #XfdashboardCoreClass::resume signal
 * @quit: Class handler for the #XfdashboardCoreClass::quit signal
 * @shutdown_final: Class handler for the #XfdashboardCoreClass::shutdown_final signal
 * @theme_changed: Class handler for the #XfdashboardCoreClass::theme_changed signal
 * @application_launched: Class handler for the #XfdashboardCoreClass::application_launched signal
 * @exit: Class handler for the #XfdashboardCoreClass::exit signal
 *
 * The #XfdashboardCoreClass structure contains only private data
 */
struct _XfdashboardCoreClass
{
	/*< private >*/
	/* Parent class */
	GObjectClass					parent_class;

	/*< public >*/
	/* Virtual functions */
	void (*initialized)(XfdashboardCore *self);

	void (*quit)(XfdashboardCore *self);
	void (*shutdown)(XfdashboardCore *self);

	gboolean (*can_suspend)(XfdashboardCore *self);
	void (*suspend)(XfdashboardCore *self);
	void (*resume)(XfdashboardCore *self);

	void (*theme_loading)(XfdashboardCore *self, XfdashboardTheme *inTheme);
	void (*theme_loaded)(XfdashboardCore *self, XfdashboardTheme *inTheme);
	void (*theme_changed)(XfdashboardCore *self, XfdashboardTheme *inTheme);

	void (*application_launched)(XfdashboardCore *self, GAppInfo *inAppInfo);

	/* Binding actions */
	gboolean (*exit)(XfdashboardCore *self,
						XfdashboardFocusable *inSource,
						const gchar *inAction,
						ClutterEvent *inEvent);
};


/* Errors */
/**
 * XFDASHBOARD_CORE_ERROR:
 *
 * Error domain for core errors.
 * Errors in this domain will be from the #XfdashboardCoreError enumeration.
 */
#define XFDASHBOARD_CORE_ERROR				(xfdashboard_core_error_quark())

GQuark xfdashboard_core_error_quark(void);

/**
 * XfdashboardCoreError:
 * @XFDASHBOARD_CORE_ERROR_INITIALIZATION_FAILED: Initialization of core
 *   object failed and cannot be used
 *
 * Error codes returned by core.
 */
typedef enum /*< prefix=XFDASHBOARD_CORE_ERROR >*/
{
	XFDASHBOARD_CORE_ERROR_INITIALIZATION_FAILED,
} XfdashboardCoreError;


/* Public API */
GType xfdashboard_core_get_type(void) G_GNUC_CONST;

gboolean xfdashboard_core_initialize(XfdashboardCore *self, GError **outError);

gboolean xfdashboard_core_has_default(void);
XfdashboardCore* xfdashboard_core_get_default(void);

/* Quit functions */
gboolean xfdashboard_core_is_quitting(XfdashboardCore *self);
void xfdashboard_core_quit(XfdashboardCore *self);

/* Suspend/resume functions */
gboolean xfdashboard_core_can_suspend(XfdashboardCore *self);
gboolean xfdashboard_core_is_suspended(XfdashboardCore *self);
void xfdashboard_core_suspend(XfdashboardCore *self);
void xfdashboard_core_resume(XfdashboardCore *self);

/* Access function for components and subsystems */
XfdashboardSettings* xfdashboard_core_get_settings(XfdashboardCore *self);
XfdashboardStage* xfdashboard_core_get_stage(XfdashboardCore *self);
XfdashboardTheme* xfdashboard_core_get_theme(XfdashboardCore *self);

XfdashboardApplicationDatabase* xfdashboard_core_get_application_database(XfdashboardCore *self);
XfdashboardApplicationTracker* xfdashboard_core_get_application_tracker(XfdashboardCore *self);
XfdashboardBindingsPool* xfdashboard_core_get_bindings_pool(XfdashboardCore *self);
XfdashboardFocusManager* xfdashboard_core_get_focus_manager(XfdashboardCore *self);
XfdashboardPluginsManager* xfdashboard_core_get_plugins_manager(XfdashboardCore *self);
XfdashboardSearchManager* xfdashboard_core_get_search_manager(XfdashboardCore *self);
XfdashboardViewManager* xfdashboard_core_get_view_manager(XfdashboardCore *self);
XfdashboardWindowTracker* xfdashboard_core_get_window_tracker(XfdashboardCore *self);
XfdashboardWindowTrackerBackend* xfdashboard_core_get_window_tracker_backend(XfdashboardCore *self);

G_END_DECLS

#endif	/* __LIBXFDASHBOARD_CORE__ */
