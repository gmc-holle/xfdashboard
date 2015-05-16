/*
 * application-button: A button representing an application
 *                     (either by menu item or desktop file)
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

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "application-button.h"

#include <glib/gi18n-lib.h>
#include <gtk/gtk.h>
#include <gio/gdesktopappinfo.h>

#include "enums.h"
#include "utils.h"

/* Define this class in GObject system */
G_DEFINE_TYPE(XfdashboardApplicationButton,
				xfdashboard_application_button,
				XFDASHBOARD_TYPE_BUTTON)

/* Private structure - access only by public API if needed */
#define XFDASHBOARD_APPLICATION_BUTTON_GET_PRIVATE(obj) \
	(G_TYPE_INSTANCE_GET_PRIVATE((obj), XFDASHBOARD_TYPE_APPLICATION_BUTTON, XfdashboardApplicationButtonPrivate))

struct _XfdashboardApplicationButtonPrivate
{
	/* Properties related */
	GAppInfo							*appInfo;

	gboolean							showDescription;
	gchar								*formatTitleOnly;
	gchar								*formatTitleDescription;

	/* Instance related */
	guint								appInfoChangedID;
};

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
			else text=g_strdup(title ? title : "");
	}
		else
		{
			if(priv->formatTitleDescription) text=g_markup_printf_escaped(priv->formatTitleDescription, title ? title : "", description ? description : "");
				else text=g_strdup_printf("%s\n%s", title ? title : "", description ? description : "");
		}

	xfdashboard_button_set_text(XFDASHBOARD_BUTTON(self), text);

	if(text) g_free(text);
}

/* Update icon of button actor */
static void _xfdashboard_application_button_update_icon(XfdashboardApplicationButton *self)
{
	XfdashboardApplicationButtonPrivate		*priv;
	gchar									*iconName;

	g_return_if_fail(XFDASHBOARD_IS_APPLICATION_BUTTON(self));

	priv=self->priv;
	iconName=NULL;

	/* Get icon */
	if(priv->appInfo)
	{
		GIcon								*gicon;

		gicon=g_app_info_get_icon(G_APP_INFO(priv->appInfo));
		if(gicon)
		{
			iconName=g_icon_to_string(gicon);
			g_object_unref(gicon);
		}
	}

	if(!iconName) iconName=g_strdup("gtk-missing-image");

	/* Set up button and release allocated resources */
	if(iconName) xfdashboard_button_set_icon(XFDASHBOARD_BUTTON(self), iconName);

	/* Release allocated resources */
	if(iconName) g_free(iconName);
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

	/* Set up private structure */
	g_type_class_add_private(klass, sizeof(XfdashboardApplicationButtonPrivate));

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

	priv=self->priv=XFDASHBOARD_APPLICATION_BUTTON_GET_PRIVATE(self);

	/* This actor is react on events */
	clutter_actor_set_reactive(CLUTTER_ACTOR(self), TRUE);

	/* Set up default values */
	priv->appInfo=NULL;
	priv->showDescription=FALSE;
	priv->formatTitleOnly=NULL;
	priv->formatTitleDescription=NULL;

	/* Connect signals */
	g_signal_connect(self, "notify::icon-size", G_CALLBACK(_xfdashboard_application_button_on_icon_size_changed), NULL);
}

/* IMPLEMENTATION: Public API */

/* Create new actor */
ClutterActor* xfdashboard_application_button_new(void)
{
	return(g_object_new(XFDASHBOARD_TYPE_APPLICATION_BUTTON,
							"button-style", XFDASHBOARD_STYLE_BOTH,
							"single-line", FALSE,
							NULL));
}

ClutterActor* xfdashboard_application_button_new_from_desktop_file(const gchar *inDesktopFilename)
{
	g_return_val_if_fail(inDesktopFilename, NULL);

	g_warning("[BAD] %s called to create application button for desktop file '%s'", __func__, inDesktopFilename);

/*	return(g_object_new(XFDASHBOARD_TYPE_APPLICATION_BUTTON,
							"button-style", XFDASHBOARD_STYLE_BOTH,
							"single-line", FALSE,
							"desktop-filename", inDesktopFilename,
							NULL));*/
	return(NULL);
}

ClutterActor* xfdashboard_application_button_new_from_menu(GarconMenuElement *inMenuElement)
{
	g_return_val_if_fail(GARCON_IS_MENU_ELEMENT(inMenuElement), NULL);

	g_warning("[BAD] %s called to create application button for menu element '%s'", __func__, garcon_menu_element_get_name(inMenuElement));

/*	return(g_object_new(XFDASHBOARD_TYPE_APPLICATION_BUTTON,
							"button-style", XFDASHBOARD_STYLE_BOTH,
							"single-line", FALSE,
							"menu-element", inMenuElement,
							NULL));*/
	return(NULL);
}

ClutterActor* xfdashboard_application_button_new_from_app_info(GAppInfo *inAppInfo)
{
	g_return_val_if_fail(G_IS_APP_INFO(inAppInfo), NULL);

	return(g_object_new(XFDASHBOARD_TYPE_APPLICATION_BUTTON,
							"button-style", XFDASHBOARD_STYLE_BOTH,
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

		/* Update actor */
		_xfdashboard_application_button_update_text(self);
		_xfdashboard_application_button_update_icon(self);

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
							"gtk-dialog-error",
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
			xfdashboard_notify(CLUTTER_ACTOR(self),
								xfdashboard_application_button_get_icon_name(self),
								_("Application '%s' launched"),
								xfdashboard_application_button_get_display_name(self));
			started=TRUE;
		}

	/* Clean up allocated resources */
	g_object_unref(context);

	/* Return status */
	return(started);
}
