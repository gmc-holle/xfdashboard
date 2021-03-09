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

/**
 * SECTION:core
 * @short_description: The core library class
 * @include: xfdashboard/core.h
 *
 * #XfdashboardCore is a single instance object. Its main purpose
 * is to setup and start-up the library and also to manage other
 * (mainly single instance) objects.
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <libxfdashboard/core.h>

#include <glib/gi18n-lib.h>
#include <clutter/x11/clutter-x11.h>
#include <gtk/gtk.h>
#include <garcon/garcon.h>
#include <libxfce4ui/libxfce4ui.h>

#include <libxfdashboard/stage.h>
#include <libxfdashboard/types.h>
#include <libxfdashboard/applications-view.h>
#include <libxfdashboard/windows-view.h>
#include <libxfdashboard/search-view.h>
#include <libxfdashboard/applications-search-provider.h>
#include <libxfdashboard/utils.h>
#include <libxfdashboard/theme.h>
#include <libxfdashboard/marshal.h>
#include <libxfdashboard/compat.h>
#include <libxfdashboard/debug.h>


/* Define this class in GObject system */
struct _XfdashboardCorePrivate
{
	/* Properties related */
	gboolean							isSuspended;
	gchar								*themeName;
	XfdashboardSettings					*settings;

	/* Instance related */
	gboolean							initialized;
	gboolean							isQuitting;

	XfdashboardStage					*stage;
	XfdashboardViewManager				*viewManager;
	XfdashboardSearchManager			*searchManager;
	XfdashboardFocusManager				*focusManager;

	XfdashboardTheme					*theme;
	GBinding							*themeChangedBinding;

	XfdashboardBindingsPool				*bindings;

	XfdashboardApplicationDatabase		*appDatabase;
	XfdashboardApplicationTracker		*appTracker;

	XfdashboardPluginsManager			*pluginsManager;

	XfdashboardWindowTrackerBackend		*windowTrackerBackend;
};

G_DEFINE_TYPE_WITH_PRIVATE(XfdashboardCore,
							xfdashboard_core,
							G_TYPE_OBJECT)

/* Properties */
enum
{
	PROP_0,

	PROP_SUSPENDED,
	PROP_THEME_NAME,
	PROP_STAGE,
	PROP_SETTINGS,

	PROP_LAST
};

static GParamSpec* XfdashboardCoreProperties[PROP_LAST]={ 0, };

/* Signals */
enum
{
	SIGNAL_INITIALIZED,
	SIGNAL_QUIT,
	SIGNAL_SHUTDOWN,

	SIGNAL_CAN_SUSPEND,
	SIGNAL_SUSPEND,
	SIGNAL_RESUME,

	SIGNAL_THEME_LOADING,
	SIGNAL_THEME_LOADED,
	SIGNAL_THEME_CHANGED,

	SIGNAL_CORE_LAUNCHED,

	/* Actions */
	ACTION_EXIT,

	SIGNAL_LAST
};

static guint XfdashboardCoreSignals[SIGNAL_LAST]={ 0, };


/* IMPLEMENTATION: Private variables and methods */

/* Single instance of application */
static XfdashboardCore*		_xfdashboard_core=NULL;

/* Action "exit" was called at application */
static gboolean _xfdashboard_core_action_exit(XfdashboardCore *self,
												XfdashboardFocusable *inSource,
												const gchar *inAction,
												ClutterEvent *inEvent)
{
	g_return_val_if_fail(XFDASHBOARD_IS_CORE(self), CLUTTER_EVENT_PROPAGATE);

	/* Quit application */
	xfdashboard_core_quit(self);

	/* Prevent the default handler being called */
	return(CLUTTER_EVENT_STOP);
}

/* A stage window should be destroyed */
static gboolean _xfdashboard_core_on_delete_stage(XfdashboardCore *self,
													ClutterEvent *inEvent,
													gpointer inUserData)
{
	g_return_val_if_fail(XFDASHBOARD_IS_CORE(self), CLUTTER_EVENT_PROPAGATE);

	/* Quit application */
	xfdashboard_core_quit(self);

	/* Prevent the default handler being called */
	return(CLUTTER_EVENT_STOP);
}

/* Set theme name and reload theme */
static void _xfdashboard_core_set_theme_name(XfdashboardCore *self, const gchar *inThemeName)
{
	XfdashboardCorePrivate	*priv;
	GError							*error;
	XfdashboardTheme				*theme;

	g_return_if_fail(XFDASHBOARD_IS_CORE(self));
	g_return_if_fail(inThemeName && *inThemeName);

	priv=self->priv;
	error=NULL;

	/* Set value if changed */
	if(g_strcmp0(priv->themeName, inThemeName)!=0)
	{
		/* Create new theme instance */
		theme=xfdashboard_theme_new(inThemeName);

		/* Emit signal that theme is going to be loaded */
		g_signal_emit(self, XfdashboardCoreSignals[SIGNAL_THEME_LOADING], 0, theme);

		/* Load theme */
		if(!xfdashboard_theme_load(theme, &error))
		{
			/* Show critical warning at console */
			g_critical("Could not load theme '%s': %s",
						inThemeName,
						(error && error->message) ? error->message : "unknown error");

			/* Show warning as notification */
			xfdashboard_notify(NULL,
								"dialog-error",
								_("Could not load theme '%s': %s"),
								inThemeName,
								(error && error->message) ? error->message : _("unknown error"));

			/* Release allocated resources */
			if(error!=NULL) g_error_free(error);
			g_object_unref(theme);

			return;
		}

		/* Emit signal that theme was loaded successfully and will soon be applied */
		g_signal_emit(self, XfdashboardCoreSignals[SIGNAL_THEME_LOADED], 0, theme);

		/* Set value */
		if(priv->themeName) g_free(priv->themeName);
		priv->themeName=g_strdup(inThemeName);

		/* Notify about property change */
		g_object_notify_by_pspec(G_OBJECT(self), XfdashboardCoreProperties[PROP_THEME_NAME]);

		/* Release current theme and store new one */
		if(priv->theme) g_object_unref(priv->theme);
		priv->theme=theme;

		/* Emit signal that theme has changed to get all top-level actors
		 * to apply new theme.
		 */
		g_signal_emit(self, XfdashboardCoreSignals[SIGNAL_THEME_CHANGED], 0, priv->theme);
	}
}

/* Set settings object */
static void _xfdashboard_core_set_settings(XfdashboardCore *self, XfdashboardSettings *inSettings)
{
	XfdashboardCorePrivate	*priv;

	g_return_if_fail(XFDASHBOARD_IS_CORE(self));
	g_return_if_fail(XFDASHBOARD_IS_SETTINGS(inSettings));

	priv=self->priv;

	/* Set value if changed */
	if(priv->settings!=inSettings)
	{
		/* Set value */
		if(priv->settings)
		{
			g_object_unref(priv->settings);
			priv->settings=NULL;
		}
		priv->settings=g_object_ref(inSettings);

		/* Notify about property change */
		g_object_notify_by_pspec(G_OBJECT(self), XfdashboardCoreProperties[PROP_SETTINGS]);
	}
}

/* Default implementation of virtual function for signal "can-suspend" */
static gboolean _xfdashboard_core_can_suspend(XfdashboardCore *self)
{
	/* This default implementation of the virtual function for the singal is
	 * to return FALSE as no suspend/resume is supported by core itself. It needs
	 * an application connecting a signal handler to this signal action to tell
	 * that suspend/resume is supported and handled by application using this
	 * library.
	 */
	return(FALSE);
}


/* IMPLEMENTATION: GObject */

/* Dispose this object */
static void _xfdashboard_core_dispose(GObject *inObject)
{
	XfdashboardCore			*self=XFDASHBOARD_CORE(inObject);
	XfdashboardCorePrivate	*priv=self->priv;

	/* Ensure "is-quitting" flag is set just in case someone asks */
	priv->isQuitting=TRUE;

	/* First destroy stage to release all actors and their bindings
	 * to any other sub-system.
	 */
	if(priv->stage)
	{
		clutter_actor_destroy(CLUTTER_ACTOR(priv->stage));
		priv->stage=NULL;
	}

	/* Signal "shutdown-final" of application */
	g_signal_emit(self, XfdashboardCoreSignals[SIGNAL_SHUTDOWN], 0);

	/* Release allocated resources */
	if(priv->windowTrackerBackend)
	{
		g_object_unref(priv->windowTrackerBackend);
		priv->windowTrackerBackend=NULL;
	}

	if(priv->pluginsManager)
	{
		g_object_unref(priv->pluginsManager);
		priv->pluginsManager=NULL;
	}

	if(priv->themeChangedBinding)
	{
		g_object_unref(priv->themeChangedBinding);
		priv->themeChangedBinding=NULL;
	}

	if(priv->viewManager)
	{
		/* Unregisters all remaining registered views - no need to unregister them here */
		g_object_unref(priv->viewManager);
		priv->viewManager=NULL;
	}

	if(priv->searchManager)
	{
		/* Unregisters all remaining registered providers - no need to unregister them here */
		g_object_unref(priv->searchManager);
		priv->searchManager=NULL;
	}

	if(priv->focusManager)
	{
		/* Unregisters all remaining registered focusable actors.
		 * There is no need to unregister them here.
		 */
		g_object_unref(priv->focusManager);
		priv->focusManager=NULL;
	}

	if(priv->bindings)
	{
		g_object_unref(priv->bindings);
		priv->bindings=NULL;
	}

	if(priv->appDatabase)
	{
		g_object_unref(priv->appDatabase);
		priv->appDatabase=NULL;
	}

	if(priv->appTracker)
	{
		g_object_unref(priv->appTracker);
		priv->appTracker=NULL;
	}

	if(priv->theme)
	{
		g_object_unref(priv->theme);
		priv->theme=NULL;
	}

	if(priv->themeName)
	{
		g_free(priv->themeName);
		priv->themeName=NULL;
	}

	if(priv->settings)
	{
		g_object_unref(priv->settings);
		priv->settings=NULL;
	}

	/* Unset singleton */
	if(G_LIKELY(G_OBJECT(_xfdashboard_core)==inObject)) _xfdashboard_core=NULL;

	/* Call parent's class dispose method */
	G_OBJECT_CLASS(xfdashboard_core_parent_class)->dispose(inObject);
}

/* Set/get properties */
static void _xfdashboard_core_set_property(GObject *inObject,
													guint inPropID,
													const GValue *inValue,
													GParamSpec *inSpec)
{
	XfdashboardCore	*self=XFDASHBOARD_CORE(inObject);

	switch(inPropID)
	{
		case PROP_THEME_NAME:
			_xfdashboard_core_set_theme_name(self, g_value_get_string(inValue));
			break;

		case PROP_SETTINGS:
			_xfdashboard_core_set_settings(self, g_value_get_object(inValue));
			break;

		default:
			G_OBJECT_WARN_INVALID_PROPERTY_ID(inObject, inPropID, inSpec);
			break;
	}
}

static void _xfdashboard_core_get_property(GObject *inObject,
													guint inPropID,
													GValue *outValue,
													GParamSpec *inSpec)
{
	XfdashboardCore	*self=XFDASHBOARD_CORE(inObject);

	switch(inPropID)
	{
		case PROP_SUSPENDED:
			g_value_set_boolean(outValue, self->priv->isSuspended);
			break;

		case PROP_STAGE:
			g_value_set_object(outValue, self->priv->stage);
			break;

		case PROP_THEME_NAME:
			g_value_set_string(outValue, self->priv->themeName);
			break;

		case PROP_SETTINGS:
			g_value_set_object(outValue, self->priv->settings);
			break;

		default:
			G_OBJECT_WARN_INVALID_PROPERTY_ID(inObject, inPropID, inSpec);
			break;
	}
}

/* Class initialization
 * Override functions in parent classes and define properties
 * and signals
 */
static void xfdashboard_core_class_init(XfdashboardCoreClass *klass)
{
	GObjectClass		*gobjectClass=G_OBJECT_CLASS(klass);

	/* Override functions */
	klass->can_suspend=_xfdashboard_core_can_suspend;
	klass->exit=_xfdashboard_core_action_exit;

	gobjectClass->dispose=_xfdashboard_core_dispose;
	gobjectClass->set_property=_xfdashboard_core_set_property;
	gobjectClass->get_property=_xfdashboard_core_get_property;

	/* Define properties */
	/**
	 * XfdashboardCore:is-suspended:
	 *
	 * A flag indicating if core was suspended. It is set to %TRUE if core is
	 * suspended. If it is %FALSE then core is resumed, active and visible.
	 */
	XfdashboardCoreProperties[PROP_SUSPENDED]=
		g_param_spec_boolean("is-suspended",
								"Is suspended",
								"Flag indicating if core is suspended currently",
								FALSE,
								G_PARAM_READABLE | G_PARAM_STATIC_STRINGS);

	/**
	 * XfdashboardCore:stage:
	 *
	 * The #XfdashboardStage of core.
	 */
	XfdashboardCoreProperties[PROP_STAGE]=
		g_param_spec_object("stage",
								"Stage",
								"The stage object of core",
								XFDASHBOARD_TYPE_STAGE,
								G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS);

	/**
	 * XfdashboardCore:theme-name:
	 *
	 * The name of current theme used in core.
	 */
	XfdashboardCoreProperties[PROP_THEME_NAME]=
		g_param_spec_string("theme-name",
								"Theme name",
								"Name of current theme",
								NULL,
								G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS);

	/**
	 * XfdashboardCore:settings:
	 *
	 * The #XfdashboardSettings of core.
	 */
	XfdashboardCoreProperties[PROP_SETTINGS]=
		g_param_spec_object("settings",
								"Settings",
								"The settings object of core",
								XFDASHBOARD_TYPE_SETTINGS,
								G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY | G_PARAM_STATIC_STRINGS);

	g_object_class_install_properties(gobjectClass, PROP_LAST, XfdashboardCoreProperties);

	/* Define signals */
	/**
	 * XfdashboardCore::initialized:
	 * @self: The core instance was fully initialized
	 *
	 * The ::initialized signal is emitted when the core instance was fully
	 * initialized.
	 *
	 * The main purpose of this signal is to give other objects and plugins
	 * a chance to initialize properly after core library was fully initialized
	 * and all other components are really available and also initialized.
	 */
	XfdashboardCoreSignals[SIGNAL_INITIALIZED]=
		g_signal_new("initialized",
						G_TYPE_FROM_CLASS(klass),
						G_SIGNAL_RUN_LAST,
						G_STRUCT_OFFSET(XfdashboardCoreClass, initialized),
						NULL,
						NULL,
						g_cclosure_marshal_VOID__VOID,
						G_TYPE_NONE,
						0);

	/**
	 * XfdashboardCore::quit:
	 * @self: The core instance is requested to quit
	 *
	 * The ::quit signal is emitted when the core library is requested to quit.
	 * It up to the connected signal handlers to shutdown or suspend the core
	 * instance.
	 */
	XfdashboardCoreSignals[SIGNAL_QUIT]=
		g_signal_new("quit",
						G_TYPE_FROM_CLASS(klass),
						G_SIGNAL_RUN_LAST,
						G_STRUCT_OFFSET(XfdashboardCoreClass, quit),
						NULL,
						NULL,
						g_cclosure_marshal_VOID__VOID,
						G_TYPE_NONE,
						0);

	/**
	 * XfdashboardCore::shutdown-final:
	 * @self: The core instance will shut down
	 *
	 * The ::shutdown-final signal is emitted when the core library object
	 * is currently destroyed and can be considered as quitted. It is the
	 * last chance or other components and plugins to work with the library.
	 *
	 * The main purpose of this signal is to give the other components,
	 * objects and plugins a chance to clean up properly.
	 */
	XfdashboardCoreSignals[SIGNAL_SHUTDOWN]=
		g_signal_new("shutdown",
						G_TYPE_FROM_CLASS(klass),
						G_SIGNAL_RUN_LAST,
						G_STRUCT_OFFSET(XfdashboardCoreClass, shutdown),
						NULL,
						NULL,
						g_cclosure_marshal_VOID__VOID,
						G_TYPE_NONE,
						0);

	/**
	 * XfdashboardCore::can-suspend:
	 * @self: The core instance is asked for support of suspend
	 *
	 * The ::can-suspend signal is emitted when to check if core can
	 * support suspend and resume. The connected signal handler must
	 * return %TRUE if suspend and resume is supported and %FALSE if
	 * it is not suppoerted.
	 *
	 * Usually only one signal handler should be connected but if more
	 * are connected the first one returning %TRUE if tell that suspend
	 * is supported. If no one is connected the default %FALSE will be
	 * returned as the only result indicating that by default no suspend
	 * is supported.
	 */
	XfdashboardCoreSignals[SIGNAL_CAN_SUSPEND]=
		g_signal_new("can-suspend",
						G_TYPE_FROM_CLASS(klass),
						G_SIGNAL_RUN_CLEANUP,
						G_STRUCT_OFFSET(XfdashboardCoreClass, can_suspend),
						NULL,
						NULL,
						_xfdashboard_marshal_BOOLEAN__VOID,
						G_TYPE_BOOLEAN,
						0);

	/**
	 * XfdashboardCore::suspend:
	 * @self: The core instance which should suspend
	 *
	 * The ::suspend signal is an action and when emitted it causes all
	 * connected signal handlers to prepare their state for suspend, that
	 * means to be send to background and waiting for ::resume.
	 */
	XfdashboardCoreSignals[SIGNAL_SUSPEND]=
		g_signal_new("suspend",
						G_TYPE_FROM_CLASS(klass),
						G_SIGNAL_RUN_LAST,
						G_STRUCT_OFFSET(XfdashboardCoreClass, suspend),
						NULL,
						NULL,
						g_cclosure_marshal_VOID__VOID,
						G_TYPE_NONE,
						0);

	/**
	 * XfdashboardCore::resume:
	 * @self: The core instance which should resume
	 *
	 * The ::resume signal is an action and when emitted it causes all
	 * connected signal handlers to prepare their state for return from
	 * suspend and to resume and to be brought to foreground again.
	 */
	XfdashboardCoreSignals[SIGNAL_RESUME]=
		g_signal_new("resume",
						G_TYPE_FROM_CLASS(klass),
						G_SIGNAL_RUN_LAST,
						G_STRUCT_OFFSET(XfdashboardCoreClass, resume),
						NULL,
						NULL,
						g_cclosure_marshal_VOID__VOID,
						G_TYPE_NONE,
						0);

	/**
	 * XfdashboardCore::theme-loading:
	 * @self: The core instance whose theme is going to change
	 * @inTheme: The new #XfdashboardTheme to use
	 *
	 * The ::theme-loading signal is emitted when a theme at core instance is
	 * going to be loaded. When this signal is received no file was loaded so far
	 * but will be. For example, at this moment it is possible to load a CSS file
	 * by a plugin before the CSS files of the new theme will be loaded to give
	 * the theme a chance to override the default CSS of plugin.
	 */
	XfdashboardCoreSignals[SIGNAL_THEME_LOADING]=
		g_signal_new("theme-loading",
						G_TYPE_FROM_CLASS(klass),
						G_SIGNAL_RUN_LAST,
						G_STRUCT_OFFSET(XfdashboardCoreClass, theme_loading),
						NULL,
						NULL,
						g_cclosure_marshal_VOID__OBJECT,
						G_TYPE_NONE,
						1,
						XFDASHBOARD_TYPE_THEME);

	/**
	 * XfdashboardCore::theme-loaded:
	 * @self: The core instance whose theme has changed
	 * @inTheme: The new #XfdashboardTheme to use
	 * 
	 * The ::theme-loaded signal is emitted when the new theme at core instance
	 * was loaded and will soon be applied. When this signal is received it is
	 * the last chance for other components and plugins to load additionally
	 * resources like CSS, e.g. to override CSS of theme.
	 */
	XfdashboardCoreSignals[SIGNAL_THEME_LOADED]=
		g_signal_new("theme-loaded",
						G_TYPE_FROM_CLASS(klass),
						G_SIGNAL_RUN_LAST,
						G_STRUCT_OFFSET(XfdashboardCoreClass, theme_loaded),
						NULL,
						NULL,
						g_cclosure_marshal_VOID__OBJECT,
						G_TYPE_NONE,
						1,
						XFDASHBOARD_TYPE_THEME);

	/**
	 * XfdashboardCore::theme-changed:
	 * @self: The core instance whose theme has changed
	 * @inTheme: The new #XfdashboardTheme applied
	 *
	 * The ::theme-changed signal is emitted when a new theme at core instance
	 * has been loaded and applied.
	 */
	XfdashboardCoreSignals[SIGNAL_THEME_CHANGED]=
		g_signal_new("theme-changed",
						G_TYPE_FROM_CLASS(klass),
						G_SIGNAL_RUN_LAST,
						G_STRUCT_OFFSET(XfdashboardCoreClass, theme_changed),
						NULL,
						NULL,
						g_cclosure_marshal_VOID__OBJECT,
						G_TYPE_NONE,
						1,
						XFDASHBOARD_TYPE_THEME);

	/**
	 * XfdashboardCore::application-launched:
	 * @self: The core instance
	 * @inAppInfo: The #GAppInfo that was just launched
	 *
	 * The ::application-launched signal is emitted when a #GAppInfo has been
	 * launched successfully, e.g. via any #XfdashboardApplicationButton.
	 */
	XfdashboardCoreSignals[SIGNAL_CORE_LAUNCHED]=
		g_signal_new("application-launched",
						G_TYPE_FROM_CLASS(klass),
						G_SIGNAL_RUN_LAST,
						G_STRUCT_OFFSET(XfdashboardCoreClass, application_launched),
						NULL,
						NULL,
						g_cclosure_marshal_VOID__OBJECT,
						G_TYPE_NONE,
						1,
						G_TYPE_APP_INFO);

	/**
	 * XfdashboardCore::exit:
	 * @self: The application
	 * @inFocusable: The source #XfdashboardFocusable which send this signal
	 * @inAction: A string containing the action (that is this signal name)
	 * @inEvent: The #ClutterEvent associated to this signal and action
	 *
	 * The ::exit signal is an action and when emitted it causes the core to
	 * send the :quit signal like calling xfdashboard_core_quit(). It up to
	 * the connected signal handlers to shutdown or suspend the core library.
	 *
	 * Note: This action can be used at key bindings.
	 */
	XfdashboardCoreSignals[ACTION_EXIT]=
		g_signal_new("exit",
						G_TYPE_FROM_CLASS(klass),
						G_SIGNAL_RUN_LAST | G_SIGNAL_ACTION,
						G_STRUCT_OFFSET(XfdashboardCoreClass, exit),
						g_signal_accumulator_true_handled,
						NULL,
						_xfdashboard_marshal_BOOLEAN__OBJECT_STRING_BOXED,
						G_TYPE_BOOLEAN,
						3,
						XFDASHBOARD_TYPE_FOCUSABLE,
						G_TYPE_STRING,
						CLUTTER_TYPE_EVENT);

	/* Register GValue transformation function not provided by any other library */
	xfdashboard_register_gvalue_transformation_funcs();
}

/* Object initialization
 * Create private structure and set up default values
 */
static void xfdashboard_core_init(XfdashboardCore *self)
{
	XfdashboardCorePrivate			*priv;

	priv=self->priv=xfdashboard_core_get_instance_private(self);

	// TODO: Setting singleton here is wrong but essential for futher development and testing - Remove later!!!
	if(_xfdashboard_core==NULL)
	{
		_xfdashboard_core=self;
		g_message("%s: set singleton=%s@%p",
					__FUNCTION__,
					_xfdashboard_core ? G_OBJECT_TYPE_NAME(_xfdashboard_core) : "<null>", _xfdashboard_core);
	}

	/* Set default values */
	priv->isSuspended=FALSE;
	priv->themeName=NULL;
	priv->initialized=FALSE;
	priv->settings=NULL;
	priv->stage=NULL;
	priv->viewManager=NULL;
	priv->searchManager=NULL;
	priv->focusManager=NULL;
	priv->theme=NULL;
	priv->themeChangedBinding=NULL;
	priv->isQuitting=FALSE;
	priv->pluginsManager=NULL;
	priv->windowTrackerBackend=NULL;
}


/* IMPLEMENTATION: Errors */

GQuark xfdashboard_core_error_quark(void)
{
	return(g_quark_from_static_string("xfdashboard-core-error-quark"));
}


/* IMPLEMENTATION: Public API */

/** xfdashboard_core_initialize:
 * @self: A #XfdashboardCore
 * @outError: A return location for a #GError or %NULL
 *
 * Initializes the core instance at @self and sets up all other components
 * like plugin manager, search manager, window tracker manager etc.
 *
 * If initialization fails Athe error message will be placed inside error
 * at @outError (if not NULL) instead of being printed on the display.
 */
gboolean xfdashboard_core_initialize(XfdashboardCore *self, GError **outError)
{
	XfdashboardCorePrivate			*priv;
	GError							*error;
#if !GARCON_CHECK_VERSION(0,3,0)
	const gchar						*desktop;
#endif

	g_return_val_if_fail(XFDASHBOARD_IS_CORE(self), FALSE);
	g_return_val_if_fail(outError==NULL || *outError==NULL, FALSE);

	priv=self->priv;
	error=NULL;

	/* Skip if initilization was done already and set error as well as
	 * return FALSE as this is an error.
	 */
	if(priv->initialized)
	{
		/* Set error */
		g_set_error(outError,
					XFDASHBOARD_CORE_ERROR,
					XFDASHBOARD_CORE_ERROR_INITIALIZATION_FAILED,
					"Core instance was already initiliazed");

		/* Return failed state */
		return(FALSE);
	}

	/* Set flag that this core instance was initialized regardless if
	 * it succeeds or fails.
	 */
	priv->initialized=TRUE;

#ifdef DEBUG
#ifdef XFDASHBOARD_ENABLE_DEBUG
	g_print("** Use environment variable XFDASHBOARD_DEBUG to enable debug messages\n");
	g_print("** To get a list of debug categories set XFDASHBOARD_DEBUG=help\n");
#endif
#endif

#ifdef XFDASHBOARD_ENABLE_DEBUG
	/* Set up debug flags */
	{
		const gchar					*environment;
		static const GDebugKey		debugKeys[]=
									{
										{ "misc", XFDASHBOARD_DEBUG_MISC },
										{ "actor", XFDASHBOARD_DEBUG_ACTOR },
										{ "style", XFDASHBOARD_DEBUG_STYLE },
										{ "styling", XFDASHBOARD_DEBUG_STYLE },
										{ "theme", XFDASHBOARD_DEBUG_THEME },
										{ "apps", XFDASHBOARD_DEBUG_APPLICATIONS },
										{ "applications", XFDASHBOARD_DEBUG_APPLICATIONS },
										{ "images", XFDASHBOARD_DEBUG_IMAGES },
										{ "windows", XFDASHBOARD_DEBUG_WINDOWS },
										{ "window-tracker", XFDASHBOARD_DEBUG_WINDOWS },
										{ "animation", XFDASHBOARD_DEBUG_ANIMATION },
										{ "animations", XFDASHBOARD_DEBUG_ANIMATION },
										{ "plugin", XFDASHBOARD_DEBUG_PLUGINS },
										{ "plugins", XFDASHBOARD_DEBUG_PLUGINS },
									};

		/* Parse debug flags */
		environment=g_getenv("XFDASHBOARD_DEBUG");
		if(environment)
		{
			xfdashboard_debug_flags=
				g_parse_debug_string(environment,
										debugKeys,
										G_N_ELEMENTS(debugKeys));
			environment=NULL;
		}

		/* Parse object names to debug */
		environment=g_getenv("XFDASHBOARD_DEBUG");
		if(environment)
		{
			if(G_UNLIKELY(xfdashboard_debug_classes))
			{
				g_strfreev(xfdashboard_debug_classes);
				xfdashboard_debug_classes=NULL;
			}

			xfdashboard_debug_classes=g_strsplit(environment, ",", -1);
			environment=NULL;
		}
	}
#endif

	/* Initialize garcon for current desktop environment */
#if !GARCON_CHECK_VERSION(0,3,0)
	desktop=g_getenv("XDG_CURRENT_DESKTOP");
	if(G_LIKELY(desktop==NULL))
	{
		/* If we could not determine current desktop environment
		 * assume Xfce as this application is developed for this DE.
		 */
		desktop="XFCE";
	}
		/* If desktop environment was found but has no name
		 * set NULL to get all menu items shown.
		 */
		else if(*desktop==0) desktop=NULL;

	garcon_set_environment(desktop);
#else
	garcon_set_environment_xdg(GARCON_ENVIRONMENT_XFCE);
#endif

	/* Check for settings */
	if(!priv->settings)
	{
		/* Set error */
		g_set_error(outError,
					XFDASHBOARD_CORE_ERROR,
					XFDASHBOARD_CORE_ERROR_INITIALIZATION_FAILED,
					"No settings provided");

		/* Return failed state */
		return(FALSE);
	}

	if(!XFDASHBOARD_IS_SETTINGS(priv->settings))
	{
		/* Set error */
		g_set_error(outError,
					XFDASHBOARD_CORE_ERROR,
					XFDASHBOARD_CORE_ERROR_INITIALIZATION_FAILED,
					"Expected settings of type %s but got %s",
					G_OBJECT_TYPE_NAME(priv->settings),
					g_type_name(XFDASHBOARD_TYPE_SETTINGS));

		/* Return failed state */
		return(FALSE);
	}

	/* Set up keyboard and pointer bindings */
	priv->bindings=XFDASHBOARD_BINDINGS_POOL(g_object_new(XFDASHBOARD_TYPE_BINDINGS_POOL, NULL));
	if(!priv->bindings)
	{
		/* Set error */
		g_set_error(outError,
					XFDASHBOARD_CORE_ERROR,
					XFDASHBOARD_CORE_ERROR_INITIALIZATION_FAILED,
					"Could not initialize bindings");

		/* Return failed state */
		return(FALSE);
	}

	if(!xfdashboard_bindings_pool_load(priv->bindings, &error))
	{
		/* Set error */
		g_propagate_error(outError, error);

		/* Return failed state */
		return(FALSE);
	}

	/* Create single-instance of window tracker backend to keep it alive while
	 * application is running and to avoid multiple reinitializations. It must
	 * be create before any class using a window tracker.
	 */
	priv->windowTrackerBackend=xfdashboard_window_tracker_backend_create();
	if(!priv->windowTrackerBackend)
	{
		/* Set error */
		g_set_error(outError,
					XFDASHBOARD_CORE_ERROR,
					XFDASHBOARD_CORE_ERROR_INITIALIZATION_FAILED,
					"Could not setup window tracker backend");

		/* Return failed state */
		return(FALSE);
	}

	/* Set up application database */
	priv->appDatabase=XFDASHBOARD_APPLICATION_DATABASE(g_object_new(XFDASHBOARD_TYPE_APPLICATION_DATABASE, NULL));
	if(!priv->appDatabase)
	{
		/* Set error */
		g_set_error(outError,
					XFDASHBOARD_CORE_ERROR,
					XFDASHBOARD_CORE_ERROR_INITIALIZATION_FAILED,
					"Could not initialize application database");

		/* Return failed state */
		return(FALSE);
	}

	if(!xfdashboard_application_database_load(priv->appDatabase, &error))
	{
		/* Set error */
		g_propagate_error(outError, error);

		/* Return failed state */
		return(FALSE);
	}

	/* Set up application tracker */
	priv->appTracker=XFDASHBOARD_APPLICATION_TRACKER(g_object_new(XFDASHBOARD_TYPE_APPLICATION_TRACKER, NULL));
	if(!priv->appTracker)
	{
		/* Set error */
		g_set_error(outError,
					XFDASHBOARD_CORE_ERROR,
					XFDASHBOARD_CORE_ERROR_INITIALIZATION_FAILED,
					"Could not initialize application tracker");

		/* Return failed state */
		return(FALSE);
	}

	/* Register built-in views (order of registration is important) */
	priv->viewManager=XFDASHBOARD_VIEW_MANAGER(g_object_new(XFDASHBOARD_TYPE_VIEW_MANAGER, NULL));

	xfdashboard_view_manager_register(priv->viewManager, "builtin.windows", XFDASHBOARD_TYPE_WINDOWS_VIEW);
	xfdashboard_view_manager_register(priv->viewManager, "builtin.applications", XFDASHBOARD_TYPE_APPLICATIONS_VIEW);
	xfdashboard_view_manager_register(priv->viewManager, "builtin.search", XFDASHBOARD_TYPE_SEARCH_VIEW);

	/* Register built-in search providers */
	priv->searchManager=XFDASHBOARD_SEARCH_MANAGER(g_object_new(XFDASHBOARD_TYPE_SEARCH_MANAGER, NULL));

	xfdashboard_search_manager_register(priv->searchManager, "builtin.applications", XFDASHBOARD_TYPE_APPLICATIONS_SEARCH_PROVIDER);

	/* Create single-instance of focus manager to keep it alive while
	 * application is running.
	 */
	priv->focusManager=XFDASHBOARD_FOCUS_MANAGER(g_object_new(XFDASHBOARD_TYPE_FOCUS_MANAGER, NULL));

	/* Create single-instance of plugin manager to keep it alive while
	 * application is running.
	 */
	priv->pluginsManager=XFDASHBOARD_PLUGINS_MANAGER(g_object_new(XFDASHBOARD_TYPE_PLUGINS_MANAGER, NULL));
	if(!priv->pluginsManager)
	{
		/* Set error */
		g_set_error(outError,
					XFDASHBOARD_CORE_ERROR,
					XFDASHBOARD_CORE_ERROR_INITIALIZATION_FAILED,
					"Could not initialize plugin manager");

		/* Return failed state */
		return(FALSE);
	}

	if(!xfdashboard_plugins_manager_setup(priv->pluginsManager))
	{
		/* Set error */
		g_set_error(outError,
					XFDASHBOARD_CORE_ERROR,
					XFDASHBOARD_CORE_ERROR_INITIALIZATION_FAILED,
					"Could not setup plugin manager");

		/* Return failed state */
		return(FALSE);
	}

	/* Set up and load theme */
	priv->themeChangedBinding=g_object_bind_property(priv->settings,
																"theme",
																self,
																"theme-name",
																G_BINDING_SYNC_CREATE);
	if(!priv->themeChangedBinding)
	{
		g_warning("Could not create binding between settings property and local resource for theme change notification.");
	}

	/* At this time the theme must have been loaded, either because we
	 * set the theme name when creating the property binding to settings
	 * what caused a call to function to set theme name in this object
	 * and also caused a reload of theme.
	 * So if no theme object is set in this object then loading theme has
	 * failed and we have to return FALSE.
	 */
	if(!priv->theme) return(FALSE);

	/* Create stage containing all monitors */
	priv->stage=XFDASHBOARD_STAGE(xfdashboard_stage_new());
	g_signal_connect_swapped(priv->stage, "delete-event", G_CALLBACK(_xfdashboard_core_on_delete_stage), self);

	/* Emit signal 'theme-changed' to get current theme loaded at each stage created */
	g_signal_emit(self, XfdashboardCoreSignals[SIGNAL_THEME_CHANGED], 0, priv->theme);

	/* Initialization was successful so send signal and return TRUE */
	g_signal_emit(self, XfdashboardCoreSignals[SIGNAL_INITIALIZED], 0);

#ifdef DEBUG
	xfdashboard_notify(NULL, NULL, _("Welcome to %s (%s)!"), PACKAGE_NAME, PACKAGE_VERSION);
#else
	xfdashboard_notify(NULL, NULL, _("Welcome to %s!"), PACKAGE_NAME);
#endif

	return(TRUE);
}

/**
 * xfdashboard_core_has_default:
 *
 * Determine if the singleton instance of #XfdashboardCore was created.
 * If the singleton instance of #XfdashboardCore was created, it can be
 * retrieved with xfdashboard_core_get_default().
 *
 * This function is useful if only the availability of the singleton instance
 * wants to be checked as xfdashboard_core_get_default() will create this
 * singleton instance if not available.
 *
 * Return value: %TRUE if singleton instance of #XfdashboardCore was
 *   created or %FALSE if not.
 */
gboolean xfdashboard_core_has_default(void)
{
	if(G_LIKELY(_xfdashboard_core)) return(TRUE);

	return(FALSE);
}

/**
 * xfdashboard_core_get_default:
 *
 * Retrieves the singleton instance of #XfdashboardCore.
 *
 * Return value: (transfer none): The instance of #XfdashboardCore.
 *   The returned object is owned by Xfdashboard and it should not be
 *   unreferenced directly.
 */
XfdashboardCore* xfdashboard_core_get_default(void)
{
	if(G_UNLIKELY(!_xfdashboard_core))
	{
		_xfdashboard_core=g_object_new(XFDASHBOARD_TYPE_CORE, NULL);
	}

	return(_xfdashboard_core);
}

/**
 * xfdashboard_core_is_quitting:
 * @self: A #XfdashboardCore
 *
 * Checks if application is in progress to quit.
 *
 * Return value: %TRUE if @self is going to quit, otherwise %FALSE.
 */
gboolean xfdashboard_core_is_quitting(XfdashboardCore *self)
{
	g_return_val_if_fail(XFDASHBOARD_IS_CORE(self), FALSE);

	return(self->priv->isQuitting);
}

/**
 * xfdashboard_core_quit:
 * @self: A #XfdashboardCore or %NULL
 *
 * Requests the core instance at @self to quit by sending the ::quit signal.
 * The connected signal handlers are responsible to handle the request and
 * to shutdown or suspend the core instance.
 *
 * If @self is %NULL the default singleton is used if it was created.
 */
void xfdashboard_core_quit(XfdashboardCore *self)
{
	g_return_if_fail(self==NULL || XFDASHBOARD_IS_CORE(self));

	/* Get default single instance if NULL is requested */
	if(!self) self=_xfdashboard_core;

	/* Send quit request */
	if(G_LIKELY(self))
	{
		g_signal_emit(self, XfdashboardCoreSignals[SIGNAL_QUIT], 0);
	}
}

/**
 * xfdashboard_core_can_suspend:
 * @self: A #XfdashboardCore or %NULL
 *
 * Asks the core instance at @self if suspend/resume is supported.
 * As the core itself does not provide any support of suspend/resume
 * it will emit the ::can-suspend signal to ask the connected signal
 * handlers (usally connected by the application using this library)
 * if they support suspend/resume.
 *
 * If any signal handler returns %TRUE, suspend/resume is supported
 * and %TRUE is also returned by this functions. Otherwise %FALSE
 * is returned indicating no support for suspend/resume.
 *
 * If @self is %NULL the default singleton is used if it was created.
 *
 * Return value: %TRUE if suspend/resume is supported, otherwise %FALSE.
 */
gboolean xfdashboard_core_can_suspend(XfdashboardCore *self)
{
	gboolean				suspendSupported;

	g_return_val_if_fail(self==NULL || XFDASHBOARD_IS_CORE(self), FALSE);

	suspendSupported=FALSE;

	/* Get default single instance if NULL is requested */
	if(!self) self=_xfdashboard_core;

	/* Emit "can-suspend" signal and collect return values */
	if(G_LIKELY(self))
	{
		g_signal_emit(self, XfdashboardCoreSignals[SIGNAL_CAN_SUSPEND], 0, &suspendSupported);
	}

	/* Return collected value for support of suspend/resume */
	return(suspendSupported);
}

/**
 * xfdashboard_core_is_suspended:
 * @self: A #XfdashboardCore
 *
 * Checks if core instance at @self is suspended.
 *
 * Note: This state can only be checked when @self supports suspend/resume
 * which be checked by calling xfdashboard_core_can_suspend() before.
 *
 * Return value: %TRUE if @self is suspended and %FALSE if it is resumed.
 */
gboolean xfdashboard_core_is_suspended(XfdashboardCore *self)
{
	g_return_val_if_fail(XFDASHBOARD_IS_CORE(self), FALSE);

	return(self->priv->isSuspended);
}

/**
 * xfdashboard_core_suspend:
 * @self: A #XfdashboardCore or %NULL
 *
 * Requests the core instance at @self to suspend by sending the ::suspend
 * signal. The connected signal handlers are responsible to handle the
 * request, e.g. hiding stage window etc..
 *
 * If @self is %NULL the default singleton is used if it was created.
 */
void xfdashboard_core_suspend(XfdashboardCore *self)
{
	g_return_if_fail(self==NULL || XFDASHBOARD_IS_CORE(self));

	/* Get default single instance if NULL is requested */
	if(!self) self=_xfdashboard_core;

	/* Send suspend request */
	if(G_LIKELY(self))
	{
		/* Emit signal */
		g_signal_emit(self, XfdashboardCoreSignals[SIGNAL_SUSPEND], 0);

		/* Set flag for suspension */
		self->priv->isSuspended=TRUE;
		g_object_notify_by_pspec(G_OBJECT(self), XfdashboardCoreProperties[PROP_SUSPENDED]);
	}
}

/**
 * xfdashboard_core_resume:
 * @self: A #XfdashboardCore or %NULL
 *
 * Requests the core instance at @self to resume by sending the ::suspend
 * signal. The connected signal handlers are responsible to handle the
 * request, e.g. hiding stage window etc..
 *
 * If @self is %NULL the default singleton is used if it was created.
 */
void xfdashboard_core_resume(XfdashboardCore *self)
{
	g_return_if_fail(self==NULL || XFDASHBOARD_IS_CORE(self));

	/* Get default single instance if NULL is requested */
	if(!self) self=_xfdashboard_core;

	/* Send suspend request */
	if(G_LIKELY(self))
	{
		/* Emit signal */
		g_signal_emit(self, XfdashboardCoreSignals[SIGNAL_RESUME], 0);

		/* Clear flag for suspension */
		self->priv->isSuspended=FALSE;
		g_object_notify_by_pspec(G_OBJECT(self), XfdashboardCoreProperties[PROP_SUSPENDED]);
	}
}

/**
 * xfdashboard_core_get_stage:
 * @self: A #XfdashboardCore or %NULL
 *
 * Retrieve #XfdashboardStage of @self.
 *
 * If @self is %NULL the default singleton is used if it was created.
 *
 * Return value: (transfer none): The #XfdashboardStage of application at @self.
 */
XfdashboardStage* xfdashboard_core_get_stage(XfdashboardCore *self)
{
	XfdashboardStage					*stage;

	g_return_val_if_fail(self==NULL || XFDASHBOARD_IS_CORE(self), NULL);

	stage=NULL;

	/* Get default single instance if NULL is requested */
	if(!self) self=_xfdashboard_core;

	/* Get xfconf channel */
	if(G_LIKELY(self)) stage=self->priv->stage;

	return(stage);
}

/**
 * xfdashboard_core_get_theme:
 * @self: A #XfdashboardCore or %NULL
 *
 * Retrieve the current #XfdashboardTheme of @self.
 *
 * If @self is %NULL the default singleton is used if it was created.
 *
 * Return value: (transfer none): The current #XfdashboardTheme of application at @self.
 */
XfdashboardTheme* xfdashboard_core_get_theme(XfdashboardCore *self)
{
	XfdashboardTheme					*theme;

	g_return_val_if_fail(self==NULL || XFDASHBOARD_IS_CORE(self), NULL);

	theme=NULL;

	/* Get default single instance if NULL is requested */
	if(!self) self=_xfdashboard_core;

	/* Get theme */
	if(G_LIKELY(self)) theme=self->priv->theme;

	return(theme);
}

/**
 * xfdashboard_core_get_settings:
 * @self: A #XfdashboardCore or %NULL
 *
 * Retrieve the #XfdashboardSettings class of @self used to query or modify settings.
 *
 * If @self is %NULL the default singleton is used if it was created.
 *
 * Return value: (transfer none): The #XfdashboardSettings of application at @self.
 */
XfdashboardSettings* xfdashboard_core_get_settings(XfdashboardCore *self)
{
	XfdashboardSettings					*settings;

	g_return_val_if_fail(self==NULL || XFDASHBOARD_IS_CORE(self), NULL);

	settings=NULL;

	/* Get default single instance if NULL is requested */
	if(!self) self=_xfdashboard_core;

	/* Get settings */
	if(G_LIKELY(self)) settings=self->priv->settings;

	return(settings);
}

/**
 * xfdashboard_core_get_application_database:
 *
 * Retrieves the singleton instance of #XfdashboardApplicationDatabase from
 * core instance at @self.
 *
 * Return value: (transfer full): The instance of #XfdashboardApplicationDatabase.
 *   Use g_object_unref() when done.
 */
XfdashboardApplicationDatabase* xfdashboard_core_get_application_database(XfdashboardCore *self)
{
	XfdashboardApplicationDatabase		*applicationDatabase;

	g_return_val_if_fail(self==NULL || XFDASHBOARD_IS_CORE(self), NULL);

	applicationDatabase=NULL;

	/* Get default single instance if NULL is requested */
	if(!self) self=_xfdashboard_core;

	/* Get application database */
	if(G_LIKELY(self)) applicationDatabase=self->priv->appDatabase;

	/* Take extra reference on object */
	if(applicationDatabase) g_object_ref(applicationDatabase);

	return(applicationDatabase);
}

/**
 * xfdashboard_core_get_application_tracker:
 *
 * Retrieves the singleton instance of #XfdashboardApplicationTracker from
 * core instance at @self.
 *
 * Return value: (transfer full): The instance of #XfdashboardApplicationTracker.
 *   Use g_object_unref() when done.
 */
XfdashboardApplicationTracker* xfdashboard_core_get_application_tracker(XfdashboardCore *self)
{
	XfdashboardApplicationTracker		*applicationTracker;

	g_return_val_if_fail(self==NULL || XFDASHBOARD_IS_CORE(self), NULL);

	applicationTracker=NULL;

	/* Get default single instance if NULL is requested */
	if(!self) self=_xfdashboard_core;

	/* Get application tracker */
	if(G_LIKELY(self)) applicationTracker=self->priv->appTracker;

	/* Take extra reference on object */
	if(applicationTracker) g_object_ref(applicationTracker);

	return(applicationTracker);
}

/**
 * xfdashboard_core_get_bindings_pool:
 *
 * Retrieves the singleton instance of #XfdashboardBindingsPool from
 * core instance at @self.
 *
 * Return value: (transfer full): The instance of #XfdashboardBindingsPool.
 *   Use g_object_unref() when done.
 */
XfdashboardBindingsPool* xfdashboard_core_get_bindings_pool(XfdashboardCore *self)
{
	XfdashboardBindingsPool				*bindingsPool;

	g_return_val_if_fail(self==NULL || XFDASHBOARD_IS_CORE(self), NULL);

	bindingsPool=NULL;

	/* Get default single instance if NULL is requested */
	if(!self) self=_xfdashboard_core;

	/* Get bindings pool */
	if(G_LIKELY(self)) bindingsPool=self->priv->bindings;

	/* Take extra reference on object */
	if(bindingsPool) g_object_ref(bindingsPool);

	return(bindingsPool);
}

/**
 * xfdashboard_core_get_focus_manager:
 *
 * Retrieves the singleton instance of #XfdashboardFocusManager from
 * core instance at @self.
 *
 * Return value: (transfer full): The instance of #XfdashboardFocusManager.
 *   Use g_object_unref() when done.
 */
XfdashboardFocusManager* xfdashboard_core_get_focus_manager(XfdashboardCore *self)
{
	XfdashboardFocusManager				*focusManager;

	g_return_val_if_fail(self==NULL || XFDASHBOARD_IS_CORE(self), NULL);

	focusManager=NULL;

	/* Get default single instance if NULL is requested */
	if(!self) self=_xfdashboard_core;

	/* Get focus manager */
	if(G_LIKELY(self)) focusManager=self->priv->focusManager;

	/* Take extra reference on object */
	if(focusManager) g_object_ref(focusManager);

	return(focusManager);
}

/**
 * xfdashboard_core_get_plugins_manager:
 *
 * Retrieves the singleton instance of #XfdashboardPluginsManager from
 * core instance at @self.
 *
 * Return value: (transfer full): The instance of #XfdashboardPluginsManager.
 *   Use g_object_unref() when done.
 */
XfdashboardPluginsManager* xfdashboard_core_get_plugins_manager(XfdashboardCore *self)
{
	XfdashboardPluginsManager			*pluginsManager;

	g_return_val_if_fail(self==NULL || XFDASHBOARD_IS_CORE(self), NULL);

	pluginsManager=NULL;

	/* Get default single instance if NULL is requested */
	if(!self) self=_xfdashboard_core;

	/* Get plugins manager */
	if(G_LIKELY(self)) pluginsManager=self->priv->pluginsManager;

	/* Take extra reference on object */
	if(pluginsManager) g_object_ref(pluginsManager);

	return(pluginsManager);
}

/**
 * xfdashboard_core_get_search_manager:
 *
 * Retrieves the singleton instance of #XfdashboardSearchManager from
 * core instance at @self.
 *
 * Return value: (transfer full): The instance of #XfdashboardSearchManager.
 *   Use g_object_unref() when done.
 */
XfdashboardSearchManager* xfdashboard_core_get_search_manager(XfdashboardCore *self)
{
	XfdashboardSearchManager			*searchManager;

	g_return_val_if_fail(self==NULL || XFDASHBOARD_IS_CORE(self), NULL);

	searchManager=NULL;

	/* Get default single instance if NULL is requested */
	if(!self) self=_xfdashboard_core;

	/* Get search manager */
	if(G_LIKELY(self)) searchManager=self->priv->searchManager;

	/* Take extra reference on object */
	if(searchManager) g_object_ref(searchManager);

	return(searchManager);
}

/**
 * xfdashboard_core_get_view_manager:
 *
 * Retrieves the singleton instance of #XfdashboardViewManager from
 * core instance at @self.
 *
 * Return value: (transfer full): The instance of #XfdashboardViewManager.
 *   Use g_object_unref() when done.
 */
XfdashboardViewManager* xfdashboard_core_get_view_manager(XfdashboardCore *self)
{
	XfdashboardViewManager				*viewManager;

	g_return_val_if_fail(self==NULL || XFDASHBOARD_IS_CORE(self), NULL);

	viewManager=NULL;

	/* Get default single instance if NULL is requested */
	if(!self) self=_xfdashboard_core;

	/* Get view manager */
	if(G_LIKELY(self)) viewManager=self->priv->viewManager;

	/* Take extra reference on object */
	if(viewManager) g_object_ref(viewManager);

	return(viewManager);
}

/**
 * xfdashboard_core_get_window_tracker:
 *
 * Retrieves the singleton instance of #XfdashboardWindowTracker from
 * core instance at @self.
 *
 * This function is the logical equivalent of:
 *
 * |[<!-- language="C" -->
 *   XfdashboardWindowTrackerBackend *backend;
 *   XfdashboardWindowTracker        *tracker;
 *
 *   backend=xfdashboard_core_get_window_tracker_backend(core);
 *   tracker=xfdashboard_window_tracker_backend_get_window_tracker(backend);
 *   g_object_unref(backend);
 * ]|
 *
 * Return value: (transfer full): The instance of #XfdashboardWindowTracker.
 *   Use g_object_unref() when done.
 */
XfdashboardWindowTracker* xfdashboard_core_get_window_tracker(XfdashboardCore *self)
{
	XfdashboardWindowTracker			*windowTracker;

	g_return_val_if_fail(self==NULL || XFDASHBOARD_IS_CORE(self), NULL);

	windowTracker=NULL;

	/* Get default single instance if NULL is requested */
	if(!self) self=_xfdashboard_core;

	/* Get window tracker */
	if(G_LIKELY(self))
	{
		windowTracker=xfdashboard_window_tracker_backend_get_window_tracker(self->priv->windowTrackerBackend);
	}

	/* Take extra reference on object */
	if(windowTracker) g_object_ref(windowTracker);

	return(windowTracker);
}

/**
 * xfdashboard_core_get_window_tracker_backend:
 *
 * Retrieves the singleton instance of #XfdashboardWindowTrackerBackend from
 * core instance at @self.
 *
 * Return value: (transfer full): The instance of #XfdashboardWindowTrackerBackend.
 *   Use g_object_unref() when done.
 */
XfdashboardWindowTrackerBackend* xfdashboard_core_get_window_tracker_backend(XfdashboardCore *self)
{
	XfdashboardWindowTrackerBackend		*windowTrackerBackend;

	g_return_val_if_fail(self==NULL || XFDASHBOARD_IS_CORE(self), NULL);

	windowTrackerBackend=NULL;

	/* Get default single instance if NULL is requested */
	if(!self) self=_xfdashboard_core;

	/* Get window tracker backend */
	if(G_LIKELY(self)) windowTrackerBackend=self->priv->windowTrackerBackend;

	/* Take extra reference on object */
	if(windowTrackerBackend) g_object_ref(windowTrackerBackend);

	return(windowTrackerBackend);
}
