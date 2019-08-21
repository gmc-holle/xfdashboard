/*
 * application-button: A button representing an application
 *                     (either by menu item or desktop file)
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

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <libxfdashboard/application-button.h>

#include <glib/gi18n-lib.h>
#include <gtk/gtk.h>
#include <gio/gdesktopappinfo.h>

#include <libxfdashboard/enums.h>
#include <libxfdashboard/utils.h>
#include <libxfdashboard/application-tracker.h>
#include <libxfdashboard/stylable.h>
#include <libxfdashboard/application.h>
#include <libxfdashboard/window-tracker.h>
#include <libxfdashboard/popup-menu-item-button.h>
#include <libxfdashboard/popup-menu-item-separator.h>
#include <libxfdashboard/desktop-app-info.h>
#include <libxfdashboard/compat.h>


/* Define this class in GObject system */
struct _XfdashboardApplicationButtonPrivate
{
	/* Properties related */
	GAppInfo							*appInfo;

	gboolean							showDescription;
	gchar								*formatTitleOnly;
	gchar								*formatTitleDescription;

	/* Instance related */
	guint								appInfoChangedID;

	XfdashboardApplicationTracker		*appTracker;
	guint								runningStateChangedID;
};

G_DEFINE_TYPE_WITH_PRIVATE(XfdashboardApplicationButton,
							xfdashboard_application_button,
							XFDASHBOARD_TYPE_BUTTON)

/* Properties */
enum
{
	PROP_0,

	PROP_APP_INFO,

	PROP_SHOW_DESCRIPTION,

	PROP_FORMAT_TITLE_ONLY,
	PROP_FORMAT_TITLE_DESCRIPTION,

	PROP_LAST
};

static GParamSpec* XfdashboardApplicationButtonProperties[PROP_LAST]={ 0, };


/* IMPLEMENTATION: Private variables and methods */

/* Update text of button actor */
static void _xfdashboard_application_button_update_text(XfdashboardApplicationButton *self)
{
	XfdashboardApplicationButtonPrivate		*priv;
	const gchar								*title;
	const gchar								*description;
	gchar									*text;

	g_return_if_fail(XFDASHBOARD_IS_APPLICATION_BUTTON(self));

	priv=self->priv;
	title=NULL;
	description=NULL;
	text=NULL;

	/* Get title and description where available */
	if(priv->appInfo)
	{
		title=g_app_info_get_name(priv->appInfo);
		description=g_app_info_get_description(priv->appInfo);
	}

	/* Create text depending on show-secondary property and set up button */
	if(priv->showDescription==FALSE)
	{
		if(priv->formatTitleOnly) text=g_markup_printf_escaped(priv->formatTitleOnly, title ? title : "");
			else text=g_markup_escape_text(title ? title : "", -1);
	}
		else
		{
			if(priv->formatTitleDescription) text=g_markup_printf_escaped(priv->formatTitleDescription, title ? title : "", description ? description : "");
				else text=g_markup_printf_escaped("%s\n%s", title ? title : "", description ? description : "");
		}

	xfdashboard_label_set_text(XFDASHBOARD_LABEL(self), text);

	if(text) g_free(text);
}

/* Update icon of button actor */
static void _xfdashboard_application_button_update_icon(XfdashboardApplicationButton *self)
{
	XfdashboardApplicationButtonPrivate		*priv;
	GIcon									*gicon;

	g_return_if_fail(XFDASHBOARD_IS_APPLICATION_BUTTON(self));

	priv=self->priv;
	gicon=NULL;

	/* Get icon and set up button icon*/
	if(priv->appInfo)
	{
		gicon=g_app_info_get_icon(G_APP_INFO(priv->appInfo));
	}

	if(gicon) xfdashboard_label_set_gicon(XFDASHBOARD_LABEL(self), gicon);
		else xfdashboard_label_set_icon_name(XFDASHBOARD_LABEL(self), "image-missing");

	/* Release allocated resources */
	if(gicon) g_object_unref(gicon);
}

/* Update running state of button actor */
static void _xfdashboard_application_button_update_running_state(XfdashboardApplicationButton *self)
{
	XfdashboardApplicationButtonPrivate		*priv;

	g_return_if_fail(XFDASHBOARD_IS_APPLICATION_BUTTON(self));

	priv=self->priv;

	/* Get current running state of application and set class "running" if it running.
	 * At all other cases remove this class.
	 */
	if(priv->appTracker &&
		priv->appInfo &&
		xfdashboard_application_tracker_is_running_by_app_info(priv->appTracker, priv->appInfo))
	{
		xfdashboard_stylable_add_class(XFDASHBOARD_STYLABLE(self), "running");
	}
		else
		{
			xfdashboard_stylable_remove_class(XFDASHBOARD_STYLABLE(self), "running");
		}
}

/* The app info has changed */
static void _xfdashboard_application_button_on_app_info_changed(XfdashboardApplicationButton *self,
																	gpointer inUserData)
{
	g_return_if_fail(XFDASHBOARD_IS_APPLICATION_BUTTON(self));

	_xfdashboard_application_button_update_text(self);
	_xfdashboard_application_button_update_icon(self);
}

/* The icon-size in button has changed */
static void _xfdashboard_application_button_on_icon_size_changed(XfdashboardApplicationButton *self,
																	GParamSpec *inSpec,
																	gpointer inUserData)
{
	g_return_if_fail(XFDASHBOARD_IS_APPLICATION_BUTTON(self));

	_xfdashboard_application_button_update_icon(self);
}

/* The running state of application has changed */
static void _xfdashboard_application_button_on_running_state_changed(XfdashboardApplicationButton *self,
																		const gchar *inDesktopID,
																		gboolean inRunning,
																		gpointer inUserData)
{
	g_return_if_fail(XFDASHBOARD_IS_APPLICATION_BUTTON(self));

	_xfdashboard_application_button_update_running_state(self);
}

/* User selected to activate a window at pop-up menu */
static void _xfdashboard_application_button_on_popup_menu_item_activate_window(XfdashboardPopupMenuItem *inMenuItem,
																				gpointer inUserData)
{
	XfdashboardWindowTrackerWindow		*window;

	g_return_if_fail(XFDASHBOARD_IS_POPUP_MENU_ITEM(inMenuItem));
	g_return_if_fail(XFDASHBOARD_IS_WINDOW_TRACKER_WINDOW(inUserData));

	window=XFDASHBOARD_WINDOW_TRACKER_WINDOW(inUserData);

	/* Activate window */
	xfdashboard_window_tracker_window_activate(window);

	/* Quit application */
	xfdashboard_application_suspend_or_quit(NULL);
}

/* User selected to execute an application action */
static void _xfdashboard_application_button_on_popup_menu_item_application_action(XfdashboardPopupMenuItem *inMenuItem,
																					gpointer inUserData)
{
	XfdashboardApplicationButton			*self;
	XfdashboardApplicationButtonPrivate		*priv;
	XfdashboardDesktopAppInfoAction			*action;
	GError									*error;

	g_return_if_fail(XFDASHBOARD_IS_POPUP_MENU_ITEM(inMenuItem));
	g_return_if_fail(XFDASHBOARD_IS_APPLICATION_BUTTON(inUserData));

	self=XFDASHBOARD_APPLICATION_BUTTON(inUserData);
	priv=self->priv;
	error=NULL;

	/* Get action to execute */
	if(!XFDASHBOARD_IS_DESKTOP_APP_INFO(priv->appInfo))
	{
		g_warning(_("Could not get information about application '%s'"),
					g_app_info_get_display_name(priv->appInfo));
		return;
	}

	action=XFDASHBOARD_DESKTOP_APP_INFO_ACTION(g_object_get_data(G_OBJECT(inMenuItem), "popup-menu-item-app-action"));
	if(!action)
	{
		g_warning(_("Could not get application action for application '%s'"),
					g_app_info_get_display_name(priv->appInfo));
		return;
	}

	/* Execute action */
	if(!xfdashboard_desktop_app_info_launch_action(XFDASHBOARD_DESKTOP_APP_INFO(priv->appInfo), action, NULL, &error))
	{
		/* Show notification about failed launch of action */
		xfdashboard_notify(CLUTTER_ACTOR(self),
							"dialog-error",
							_("Could not execute action '%s' for application '%s': %s"),
							xfdashboard_desktop_app_info_action_get_name(action),
							g_app_info_get_display_name(priv->appInfo),
							error ? error->message : _("Unknown error"));
		g_error_free(error);
	}
		else
		{
			GIcon						*gicon;
			const gchar					*iconName;

			/* Get icon of application */
			iconName=NULL;

			gicon=g_app_info_get_icon(priv->appInfo);
			if(gicon) iconName=g_icon_to_string(gicon);

			/* Show notification about successful launch of action */
			xfdashboard_notify(CLUTTER_ACTOR(self),
								iconName,
								_("Executed action '%s' for application '%s'"),
								xfdashboard_desktop_app_info_action_get_name(action),
								g_app_info_get_display_name(priv->appInfo));

			/* Quit application */
			xfdashboard_application_suspend_or_quit(NULL);

			/* Release allocated resources */
			g_object_unref(gicon);
		}
}


/* IMPLEMENTATION: GObject */

/* Dispose this object */
static void _xfdashboard_application_button_dispose(GObject *inObject)
{
	XfdashboardApplicationButton			*self=XFDASHBOARD_APPLICATION_BUTTON(inObject);
	XfdashboardApplicationButtonPrivate		*priv=self->priv;

	/* Release our allocated variables */
	if(priv->appInfo)
	{
		if(priv->appInfoChangedID)
		{
			g_signal_handler_disconnect(priv->appInfo, priv->appInfoChangedID);
			priv->appInfoChangedID=0;
		}

		g_object_unref(priv->appInfo);
		priv->appInfo=NULL;
	}

	if(priv->appTracker)
	{
		if(priv->runningStateChangedID)
		{
			g_signal_handler_disconnect(priv->appTracker, priv->runningStateChangedID);
			priv->runningStateChangedID=0;
		}

		g_object_unref(priv->appTracker);
		priv->appTracker=NULL;
	}

	if(priv->formatTitleOnly)
	{
		g_free(priv->formatTitleOnly);
		priv->formatTitleOnly=NULL;
	}

	if(priv->formatTitleDescription)
	{
		g_free(priv->formatTitleDescription);
		priv->formatTitleDescription=NULL;
	}

	/* Call parent's class dispose method */
	G_OBJECT_CLASS(xfdashboard_application_button_parent_class)->dispose(inObject);
}

/* Set/get properties */
static void _xfdashboard_application_button_set_property(GObject *inObject,
															guint inPropID,
															const GValue *inValue,
															GParamSpec *inSpec)
{
	XfdashboardApplicationButton			*self=XFDASHBOARD_APPLICATION_BUTTON(inObject);

	switch(inPropID)
	{
		case PROP_APP_INFO:
			xfdashboard_application_button_set_app_info(self, G_APP_INFO(g_value_get_object(inValue)));
			break;

		case PROP_SHOW_DESCRIPTION:
			xfdashboard_application_button_set_show_description(self, g_value_get_boolean(inValue));
			break;

		case PROP_FORMAT_TITLE_ONLY:
			xfdashboard_application_button_set_format_title_only(self, g_value_get_string(inValue));
			break;

		case PROP_FORMAT_TITLE_DESCRIPTION:
			xfdashboard_application_button_set_format_title_description(self, g_value_get_string(inValue));
			break;

		default:
			G_OBJECT_WARN_INVALID_PROPERTY_ID(inObject, inPropID, inSpec);
			break;
	}
}

static void _xfdashboard_application_button_get_property(GObject *inObject,
															guint inPropID,
															GValue *outValue,
															GParamSpec *inSpec)
{
	XfdashboardApplicationButton			*self=XFDASHBOARD_APPLICATION_BUTTON(inObject);
	XfdashboardApplicationButtonPrivate		*priv=self->priv;

	switch(inPropID)
	{
		case PROP_APP_INFO:
			g_value_set_object(outValue, priv->appInfo);
			break;

		case PROP_SHOW_DESCRIPTION:
			g_value_set_boolean(outValue, priv->showDescription);
			break;

		case PROP_FORMAT_TITLE_ONLY:
			g_value_set_string(outValue, priv->formatTitleOnly);
			break;

		case PROP_FORMAT_TITLE_DESCRIPTION:
			g_value_set_string(outValue, priv->formatTitleDescription);
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
static void xfdashboard_application_button_class_init(XfdashboardApplicationButtonClass *klass)
{
	XfdashboardActorClass	*actorClass=XFDASHBOARD_ACTOR_CLASS(klass);
	GObjectClass			*gobjectClass=G_OBJECT_CLASS(klass);

	/* Override functions */
	gobjectClass->dispose=_xfdashboard_application_button_dispose;
	gobjectClass->set_property=_xfdashboard_application_button_set_property;
	gobjectClass->get_property=_xfdashboard_application_button_get_property;

	/* Define properties */
	XfdashboardApplicationButtonProperties[PROP_APP_INFO]=
		g_param_spec_object("app-info",
								_("Application information"),
								_("The application information whose title and description to display"),
								G_TYPE_APP_INFO,
								G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS);

	XfdashboardApplicationButtonProperties[PROP_SHOW_DESCRIPTION]=
		g_param_spec_boolean("show-description",
								_("Show description"),
								_("Show also description next to tile"),
								FALSE,
								G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS);

	XfdashboardApplicationButtonProperties[PROP_FORMAT_TITLE_ONLY]=
		g_param_spec_string("format-title-only",
								_("Format title only"),
								_("Format string used when only title is display"),
								NULL,
								G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS);

	XfdashboardApplicationButtonProperties[PROP_FORMAT_TITLE_DESCRIPTION]=
		g_param_spec_string("format-title-description",
								_("Format title and description"),
								_("Format string used when title and description is display. First argument is title and second one is description."),
								NULL,
								G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS);

	g_object_class_install_properties(gobjectClass, PROP_LAST, XfdashboardApplicationButtonProperties);

	/* Define stylable properties */
	xfdashboard_actor_install_stylable_property(actorClass, XfdashboardApplicationButtonProperties[PROP_SHOW_DESCRIPTION]);
	xfdashboard_actor_install_stylable_property(actorClass, XfdashboardApplicationButtonProperties[PROP_FORMAT_TITLE_ONLY]);
	xfdashboard_actor_install_stylable_property(actorClass, XfdashboardApplicationButtonProperties[PROP_FORMAT_TITLE_DESCRIPTION]);
}

/* Object initialization
 * Create private structure and set up default values
 */
static void xfdashboard_application_button_init(XfdashboardApplicationButton *self)
{
	XfdashboardApplicationButtonPrivate		*priv;

	priv=self->priv=xfdashboard_application_button_get_instance_private(self);

	/* This actor is react on events */
	clutter_actor_set_reactive(CLUTTER_ACTOR(self), TRUE);

	/* Set up default values */
	priv->appInfo=NULL;
	priv->showDescription=FALSE;
	priv->formatTitleOnly=NULL;
	priv->formatTitleDescription=NULL;
	priv->appTracker=xfdashboard_application_tracker_get_default();
	priv->runningStateChangedID=0;

	/* Connect signals */
	g_signal_connect(self,
						"notify::icon-size",
						G_CALLBACK(_xfdashboard_application_button_on_icon_size_changed),
						NULL);
}

/* IMPLEMENTATION: Public API */

/* Create new actor */
ClutterActor* xfdashboard_application_button_new(void)
{
	return(g_object_new(XFDASHBOARD_TYPE_APPLICATION_BUTTON,
							"label-style", XFDASHBOARD_LABEL_STYLE_BOTH,
							"single-line", FALSE,
							NULL));
}

ClutterActor* xfdashboard_application_button_new_from_app_info(GAppInfo *inAppInfo)
{
	g_return_val_if_fail(G_IS_APP_INFO(inAppInfo), NULL);

	return(g_object_new(XFDASHBOARD_TYPE_APPLICATION_BUTTON,
							"label-style", XFDASHBOARD_LABEL_STYLE_BOTH,
							"single-line", FALSE,
							"app-info", inAppInfo,
							NULL));
}

/* Get/set application information for this application button */
GAppInfo* xfdashboard_application_button_get_app_info(XfdashboardApplicationButton *self)
{
	g_return_val_if_fail(XFDASHBOARD_IS_APPLICATION_BUTTON(self), NULL);

	return(self->priv->appInfo);
}

void xfdashboard_application_button_set_app_info(XfdashboardApplicationButton *self, GAppInfo *inAppInfo)
{
	XfdashboardApplicationButtonPrivate		*priv;

	g_return_if_fail(XFDASHBOARD_IS_APPLICATION_BUTTON(self));
	g_return_if_fail(G_IS_APP_INFO(inAppInfo));

	priv=self->priv;

	/* Set value if changed */
	if(!priv->appInfo ||
		!g_app_info_equal(G_APP_INFO(priv->appInfo), G_APP_INFO(inAppInfo)))
	{
		/* Set value */
		if(priv->appInfo)
		{
			if(priv->appInfoChangedID)
			{
				g_signal_handler_disconnect(priv->appInfo, priv->appInfoChangedID);
				priv->appInfoChangedID=0;
			}

			g_object_unref(priv->appInfo);
			priv->appInfo=NULL;
		}

		priv->appInfo=g_object_ref(inAppInfo);
		if(XFDASHBOARD_IS_DESKTOP_APP_INFO(priv->appInfo))
		{
			priv->appInfoChangedID=g_signal_connect_swapped(priv->appInfo,
															"changed",
															G_CALLBACK(_xfdashboard_application_button_on_app_info_changed),
															self);
		}

		/* Disconnect any signal handler for running state changes of old application
		 * and connect to new one.
		 */
		if(priv->appTracker &&
			priv->runningStateChangedID)
		{
			g_signal_handler_disconnect(priv->appTracker, priv->runningStateChangedID);
			priv->runningStateChangedID=0;
		}

		if(priv->appTracker &&
			priv->appInfo)
		{
			gchar							*signalName;

			/* Build signal name to connect to for running state changes */
			signalName=g_strdup_printf("state-changed::%s", g_app_info_get_id(priv->appInfo));
			priv->runningStateChangedID=g_signal_connect_swapped(priv->appTracker,
																	signalName,
																	G_CALLBACK(_xfdashboard_application_button_on_running_state_changed),
																	self);
			g_free(signalName);
		}

		/* Update actor */
		_xfdashboard_application_button_update_text(self);
		_xfdashboard_application_button_update_icon(self);
		_xfdashboard_application_button_update_running_state(self);

		/* Notify about property change */
		g_object_notify_by_pspec(G_OBJECT(self), XfdashboardApplicationButtonProperties[PROP_APP_INFO]);
	}
}

/* Get/set flag to show description */
gboolean xfdashboard_application_button_get_show_description(XfdashboardApplicationButton *self)
{
	g_return_val_if_fail(XFDASHBOARD_IS_APPLICATION_BUTTON(self), FALSE);

	return(self->priv->showDescription);
}

void xfdashboard_application_button_set_show_description(XfdashboardApplicationButton *self, gboolean inShowDescription)
{
	XfdashboardApplicationButtonPrivate		*priv;

	g_return_if_fail(XFDASHBOARD_IS_APPLICATION_BUTTON(self));

	priv=self->priv;

	/* Set value if changed */
	if(priv->showDescription!=inShowDescription)
	{
		/* Set value */
		priv->showDescription=inShowDescription;

		/* Update actor */
		_xfdashboard_application_button_update_text(self);

		/* Notify about property change */
		g_object_notify_by_pspec(G_OBJECT(self), XfdashboardApplicationButtonProperties[PROP_SHOW_DESCRIPTION]);
	}
}

/* Get/set format string to use when displaying only title */
const gchar* xfdashboard_application_button_get_format_title_only(XfdashboardApplicationButton *self)
{
	g_return_val_if_fail(XFDASHBOARD_IS_APPLICATION_BUTTON(self), NULL);

	return(self->priv->formatTitleOnly);
}

void xfdashboard_application_button_set_format_title_only(XfdashboardApplicationButton *self, const gchar *inFormat)
{
	XfdashboardApplicationButtonPrivate		*priv;

	g_return_if_fail(XFDASHBOARD_IS_APPLICATION_BUTTON(self));
	g_return_if_fail(inFormat);

	priv=self->priv;

	/* Set value if changed */
	if(g_strcmp0(priv->formatTitleOnly, inFormat)!=0)
	{
		/* Set value */
		if(priv->formatTitleOnly) g_free(priv->formatTitleOnly);
		priv->formatTitleOnly=g_strdup(inFormat);

		/* Update actor */
		_xfdashboard_application_button_update_text(self);

		/* Notify about property change */
		g_object_notify_by_pspec(G_OBJECT(self), XfdashboardApplicationButtonProperties[PROP_FORMAT_TITLE_ONLY]);
	}
}

/* Get/set format string to use when displaying title and description */
const gchar* xfdashboard_application_button_get_format_title_description(XfdashboardApplicationButton *self)
{
	g_return_val_if_fail(XFDASHBOARD_IS_APPLICATION_BUTTON(self), NULL);

	return(self->priv->formatTitleDescription);
}

void xfdashboard_application_button_set_format_title_description(XfdashboardApplicationButton *self, const gchar *inFormat)
{
	XfdashboardApplicationButtonPrivate		*priv;

	g_return_if_fail(XFDASHBOARD_IS_APPLICATION_BUTTON(self));
	g_return_if_fail(inFormat);

	priv=self->priv;

	/* Set value if changed */
	if(g_strcmp0(priv->formatTitleDescription, inFormat)!=0)
	{
		/* Set value */
		if(priv->formatTitleOnly) g_free(priv->formatTitleDescription);
		priv->formatTitleDescription=g_strdup(inFormat);

		/* Update actor */
		_xfdashboard_application_button_update_text(self);

		/* Notify about property change */
		g_object_notify_by_pspec(G_OBJECT(self), XfdashboardApplicationButtonProperties[PROP_FORMAT_TITLE_DESCRIPTION]);
	}
}

/* Get display name of application button */
const gchar* xfdashboard_application_button_get_display_name(XfdashboardApplicationButton *self)
{
	XfdashboardApplicationButtonPrivate		*priv;
	const gchar								*title;

	g_return_val_if_fail(XFDASHBOARD_IS_APPLICATION_BUTTON(self), NULL);

	priv=self->priv;
	title=NULL;

	/* Get title */
	if(priv->appInfo)
	{
		title=g_app_info_get_name(priv->appInfo);
	}

	/* Return display name */
	return(title);
}

/* Get icon name of application button */
const gchar* xfdashboard_application_button_get_icon_name(XfdashboardApplicationButton *self)
{
	XfdashboardApplicationButtonPrivate		*priv;
	const gchar								*iconName;

	g_return_val_if_fail(XFDASHBOARD_IS_APPLICATION_BUTTON(self), NULL);

	priv=self->priv;
	iconName=NULL;

	/* Get icon where available */
	if(priv->appInfo)
	{
		GIcon						*gicon;

		gicon=g_app_info_get_icon(priv->appInfo);
		if(gicon) iconName=g_icon_to_string(gicon);
	}

	/* Return display name */
	return(iconName);
}

/* Execute command of represented application by application button */
gboolean xfdashboard_application_button_execute(XfdashboardApplicationButton *self, GAppLaunchContext *inContext)
{
	XfdashboardApplicationButtonPrivate		*priv;
	GError									*error;
	gboolean								started;
	GAppLaunchContext						*context;

	g_return_val_if_fail(XFDASHBOARD_IS_APPLICATION_BUTTON(self), FALSE);
	g_return_val_if_fail(!inContext || G_IS_APP_LAUNCH_CONTEXT(inContext), FALSE);

	priv=self->priv;
	started=FALSE;

	/* Launch application */
	if(!priv->appInfo)
	{
		xfdashboard_notify(CLUTTER_ACTOR(self),
							"dialog-error",
							_("Launching application '%s' failed: %s"),
							xfdashboard_application_button_get_display_name(self),
							_("No information available for application"));
		g_warning(_("Launching application '%s' failed: %s"),
					xfdashboard_application_button_get_display_name(self),
					_("No information available for application"));
		return(FALSE);
	}

	if(inContext) context=G_APP_LAUNCH_CONTEXT(g_object_ref(inContext));
		else context=xfdashboard_create_app_context(NULL);

	error=NULL;
	if(!g_app_info_launch(priv->appInfo, NULL, context, &error))
	{
		/* Show notification about failed application launch */
		xfdashboard_notify(CLUTTER_ACTOR(self),
							xfdashboard_application_button_get_icon_name(self),
							_("Launching application '%s' failed: %s"),
							xfdashboard_application_button_get_display_name(self),
							(error && error->message) ? error->message : _("unknown error"));
		g_warning(_("Launching application '%s' failed: %s"),
					xfdashboard_application_button_get_display_name(self),
					(error && error->message) ? error->message : _("unknown error"));
		if(error) g_error_free(error);
	}
		else
		{
			/* Show notification about successful application launch */
			xfdashboard_notify(CLUTTER_ACTOR(self),
								xfdashboard_application_button_get_icon_name(self),
								_("Application '%s' launched"),
								xfdashboard_application_button_get_display_name(self));

			/* Emit signal for successful application launch */
			g_signal_emit_by_name(xfdashboard_application_get_default(), "application-launched", priv->appInfo);

			/* Set status that application has been started successfully */
			started=TRUE;
		}

	/* Clean up allocated resources */
	g_object_unref(context);

	/* Return status */
	return(started);
}

/* Add each open window of applicatio as menu item to pop-up menu*/
guint xfdashboard_application_button_add_popup_menu_items_for_windows(XfdashboardApplicationButton *self,
																		XfdashboardPopupMenu *inMenu)
{
	XfdashboardApplicationButtonPrivate		*priv;
	guint									numberItems;
	const GList								*windows;

	g_return_val_if_fail(XFDASHBOARD_IS_APPLICATION_BUTTON(self), 0);
	g_return_val_if_fail(XFDASHBOARD_IS_POPUP_MENU(inMenu), 0);

	priv=self->priv;
	numberItems=0;

	/* Add each open window to pop-up of application */
	windows=xfdashboard_application_tracker_get_window_list_by_app_info(priv->appTracker, priv->appInfo);
	if(windows)
	{
		const GList							*iter;
		GList								*sortedList;
		XfdashboardWindowTracker			*windowTracker;
		XfdashboardWindowTrackerWindow		*window;
		XfdashboardWindowTrackerWorkspace	*activeWorkspace;
		XfdashboardWindowTrackerWorkspace	*windowWorkspace;
		gboolean							separatorAdded;
		ClutterActor						*menuItem;
		gchar								*windowName;

		/* Create sorted list of windows. The window is added to begin
		 * of list if it is on active workspace and to end of list if it
		 * is on any other workspace.
		 */
		windowTracker=xfdashboard_window_tracker_get_default();
		activeWorkspace=xfdashboard_window_tracker_get_active_workspace(windowTracker);

		sortedList=NULL;
		for(iter=windows; iter; iter=g_list_next(iter))
		{
			/* Get window currently iterated */
			window=XFDASHBOARD_WINDOW_TRACKER_WINDOW(iter->data);
			if(!window) continue;

			/* Get workspace of window */
			windowWorkspace=xfdashboard_window_tracker_window_get_workspace(window);

			/* If window is on active workspace add to begin of sorted list,
			 * otherwise add to end of sorted list.
			 */
			if(windowWorkspace==activeWorkspace)
			{
				sortedList=g_list_prepend(sortedList, window);
			}
				else
				{
					sortedList=g_list_append(sortedList, window);
				}
		}

		/* Now add menu items for each window in sorted list */
		separatorAdded=FALSE;
		for(iter=sortedList; iter; iter=g_list_next(iter))
		{
			/* Get window currently iterated */
			window=XFDASHBOARD_WINDOW_TRACKER_WINDOW(iter->data);
			if(!window) continue;

			/* Get workspace of window */
			windowWorkspace=xfdashboard_window_tracker_window_get_workspace(window);

			/* Add separator if currently iterated window is not on active
			 * workspace then all following windows are not on active workspace
			 * anymore and a separator is added to split them from the ones
			 * on active workspace. But add this separator only once.
			 */
			if(windowWorkspace!=activeWorkspace &&
				!separatorAdded)
			{
				menuItem=xfdashboard_popup_menu_item_separator_new();
				clutter_actor_set_x_expand(menuItem, TRUE);
				xfdashboard_popup_menu_add_item(inMenu, XFDASHBOARD_POPUP_MENU_ITEM(menuItem));

				separatorAdded=TRUE;
			}

			/* Create menu item for window */
			menuItem=xfdashboard_popup_menu_item_button_new();
			clutter_actor_set_x_expand(menuItem, TRUE);
			xfdashboard_popup_menu_add_item(inMenu, XFDASHBOARD_POPUP_MENU_ITEM(menuItem));

			windowName=g_markup_printf_escaped("%s", xfdashboard_window_tracker_window_get_name(window));
			xfdashboard_label_set_text(XFDASHBOARD_LABEL(menuItem), windowName);
			g_free(windowName);

			g_signal_connect(menuItem,
								"activated",
								G_CALLBACK(_xfdashboard_application_button_on_popup_menu_item_activate_window),
								window);

			/* Count number of menu items created */
			numberItems++;
		}

		/* Release allocated resources */
		g_list_free(sortedList);
		g_object_unref(windowTracker);
	}

	/* Return number of menu items added to pop-up menu */
	return(numberItems);
}

/* Add application actions as menu items to pop-up menu */
guint xfdashboard_application_button_add_popup_menu_items_for_actions(XfdashboardApplicationButton *self,
																		XfdashboardPopupMenu *inMenu)
{
	XfdashboardApplicationButtonPrivate			*priv;
	guint										numberItems;
	GList										*actions;
	const GList									*iter;

	g_return_val_if_fail(XFDASHBOARD_IS_APPLICATION_BUTTON(self), 0);
	g_return_val_if_fail(XFDASHBOARD_IS_POPUP_MENU(inMenu), 0);

	priv=self->priv;
	numberItems=0;

	/* Check if application actions can be requested */
	if(XFDASHBOARD_IS_DESKTOP_APP_INFO(priv->appInfo))
	{
		/* Get actions and create a menu item for each one */
		actions=xfdashboard_desktop_app_info_get_actions(XFDASHBOARD_DESKTOP_APP_INFO(priv->appInfo));
		for(iter=actions; iter; iter=g_list_next(iter))
		{
			XfdashboardDesktopAppInfoAction		*action;
			const gchar							*iconName;
			ClutterActor						*menuItem;

			/* Get currently iterated application action */
			action=XFDASHBOARD_DESKTOP_APP_INFO_ACTION(iter->data);
			if(!action) continue;

			/* Get icon name to determine style of pop-up menu item */
			iconName=xfdashboard_desktop_app_info_action_get_icon_name(action);

			/* Create pop-up menu item. If icon name is available then set
			 * style to both (text+icon) and set icon. If icon name is NULL
			 * then keep style at text-only.
			 */
			menuItem=xfdashboard_popup_menu_item_button_new();
			xfdashboard_label_set_text(XFDASHBOARD_LABEL(menuItem), xfdashboard_desktop_app_info_action_get_name(action));
			if(iconName)
			{
				xfdashboard_label_set_icon_name(XFDASHBOARD_LABEL(menuItem), iconName);
				xfdashboard_label_set_style(XFDASHBOARD_LABEL(menuItem), XFDASHBOARD_LABEL_STYLE_BOTH);
			}
			clutter_actor_set_x_expand(menuItem, TRUE);
			xfdashboard_popup_menu_add_item(inMenu, XFDASHBOARD_POPUP_MENU_ITEM(menuItem));

			g_object_set_data_full(G_OBJECT(menuItem), "popup-menu-item-app-action", g_object_ref(action), g_object_unref);

			g_signal_connect(menuItem,
								"activated",
								G_CALLBACK(_xfdashboard_application_button_on_popup_menu_item_application_action),
								self);

			/* Count number of menu items created */
			numberItems++;
		}
	}

	/* Return number of menu items added to pop-up menu */
	return(numberItems);
}
