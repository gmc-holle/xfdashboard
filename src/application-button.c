/*
 * application-button: A button representing an application
 *                     (either by menu item or desktop file)
 * 
 * Copyright 2012-2013 Stephan Haller <nomad@froevel.de>
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

#include "enums.h"
#include "utils.h"

/* Define this class in GObject system */
G_DEFINE_TYPE(XfdashboardApplicationButton,
				xfdashboard_application_button,
				XFDASHBOARD_TYPE_BUTTON)

/* Type of application button */
typedef enum /*< skip,prefix=XFDASHBOARD_APPLICATION_BUTTON_TYPE >*/
{
	XFDASHBOARD_APPLICATION_BUTTON_TYPE_NONE=0,
	XFDASHBOARD_APPLICATION_BUTTON_TYPE_MENU_ITEM,
	XFDASHBOARD_APPLICATION_BUTTON_TYPE_DESKTOP_FILE
} XfdashboardApplicationButtonType;

/* Private structure - access only by public API if needed */
#define XFDASHBOARD_APPLICATION_BUTTON_GET_PRIVATE(obj) \
	(G_TYPE_INSTANCE_GET_PRIVATE((obj), XFDASHBOARD_TYPE_APPLICATION_BUTTON, XfdashboardApplicationButtonPrivate))

struct _XfdashboardApplicationButtonPrivate
{
	/* Properties related */
	GarconMenuElement					*menuElement;
	gchar								*desktopFilename;
	gboolean							showDescription;

	/* Instance related */
	XfdashboardApplicationButtonType	type;
	GAppInfo							*appInfo;
	gchar								*formatTitleOnly;
	gchar								*formatTitleDescription;
};

/* Properties */
enum
{
	PROP_0,

	PROP_MENU_ELEMENT,
	PROP_DESKTOP_FILENAME,

	PROP_SHOW_DESCRIPTION,

	PROP_FORMAT_TITLE_ONLY,
	PROP_FORMAT_TITLE_DESCRIPTION,

	PROP_LAST
};

static GParamSpec* XfdashboardApplicationButtonProperties[PROP_LAST]={ 0, };


/* IMPLEMENTATION: Private variables and methods */
#define DEFAULT_ICON_SIZE					64								// TODO: Replace by settings/theming object
#define DEFAULT_FORMAT_TITLE_ONLY			"<b>%s</b>"						// TODO: Replace by settings/theming object
#define DEFAULT_FORMAT_TITLE_DESCRIPTION	"<b><big>%s</big></b>\n\n%s"	// TODO: Replace by settings/theming object

/* Reset and release allocated resources of application button */
static void _xfdashboard_application_button_clear(XfdashboardApplicationButton *self)
{
	XfdashboardApplicationButtonPrivate		*priv;

	g_return_if_fail(XFDASHBOARD_IS_APPLICATION_BUTTON(self));

	priv=self->priv;

	/* Release allocated resources */
	if(priv->menuElement)
	{
		g_object_unref(priv->menuElement);
		priv->menuElement=NULL;

		/* Notify about property change */
		g_object_notify_by_pspec(G_OBJECT(self), XfdashboardApplicationButtonProperties[PROP_MENU_ELEMENT]);
	}

	if(priv->appInfo)
	{
		g_object_unref(priv->appInfo);
		priv->appInfo=NULL;
	}

	if(priv->desktopFilename)
	{
		g_free(priv->desktopFilename);
		priv->desktopFilename=NULL;

		/* Notify about property change */
		g_object_notify_by_pspec(G_OBJECT(self), XfdashboardApplicationButtonProperties[PROP_DESKTOP_FILENAME]);
	}

	/* Reset application button */
	priv->type=XFDASHBOARD_APPLICATION_BUTTON_TYPE_NONE;
}

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
	switch(priv->type)
	{
		case XFDASHBOARD_APPLICATION_BUTTON_TYPE_NONE:
			/* Do nothing */
			break;

		case XFDASHBOARD_APPLICATION_BUTTON_TYPE_MENU_ITEM:
			title=garcon_menu_element_get_name(priv->menuElement);
			description=garcon_menu_element_get_comment(priv->menuElement);
			break;

		case XFDASHBOARD_APPLICATION_BUTTON_TYPE_DESKTOP_FILE:
			if(priv->appInfo)
			{
				title=g_app_info_get_name(priv->appInfo);
				description=g_app_info_get_description(priv->appInfo);
			}
				else
				{
					title=priv->desktopFilename;
					description=_("No information available for application!");
				}
			break;

		default:
			g_critical(_("Cannot update application icon of unknown type (%d)"), priv->type);
			break;
	}

	/* Create text depending on show-secondary property and set up button */
	if(priv->showDescription==FALSE) text=g_markup_printf_escaped(priv->formatTitleOnly, title ? title : "");
		else text=g_markup_printf_escaped(priv->formatTitleDescription, title ? title : "", description ? description : "");
	xfdashboard_button_set_text(XFDASHBOARD_BUTTON(self), text);
	if(text) g_free(text);
}

/* Update icon of button actor */
static void _xfdashboard_application_button_update_icon(XfdashboardApplicationButton *self)
{
	XfdashboardApplicationButtonPrivate		*priv;
	const gchar								*iconName;

	g_return_if_fail(XFDASHBOARD_IS_APPLICATION_BUTTON(self));

	priv=self->priv;
	iconName=NULL;

	/* Get icon where available */
	switch(priv->type)
	{
		case XFDASHBOARD_APPLICATION_BUTTON_TYPE_NONE:
			/* Do nothing */
			break;

		case XFDASHBOARD_APPLICATION_BUTTON_TYPE_MENU_ITEM:
			iconName=garcon_menu_element_get_icon_name(priv->menuElement);
			break;

		case XFDASHBOARD_APPLICATION_BUTTON_TYPE_DESKTOP_FILE:
			if(priv->appInfo)
			{
				GIcon						*gicon;

				gicon=g_app_info_get_icon(priv->appInfo);
				if(gicon) iconName=g_icon_to_string(gicon);
			}
				else iconName=GTK_STOCK_MISSING_IMAGE;
			break;

		default:
			g_critical(_("Cannot update application icon of unknown type (%d)"), priv->type);
			break;
	}

	/* Set up button and release allocated resources */
	if(iconName) xfdashboard_button_set_icon(XFDASHBOARD_BUTTON(self), iconName);
}

/* The icon-size in button has changed */
static void _xfdashboard_application_button_on_icon_size_changed(XfdashboardApplicationButton *self,
																	GParamSpec *inSpec,
																	gpointer inUserData)
{
	_xfdashboard_application_button_update_icon(self);
}

/* IMPLEMENTATION: GObject */

/* Dispose this object */
static void _xfdashboard_application_button_dispose(GObject *inObject)
{
	XfdashboardApplicationButton		*self=XFDASHBOARD_APPLICATION_BUTTON(inObject);

	/* Release our allocated variables */
	_xfdashboard_application_button_clear(self);

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
		case PROP_MENU_ELEMENT:
			xfdashboard_application_button_set_menu_element(self, GARCON_MENU_ELEMENT(g_value_get_object(inValue)));
			break;

		case PROP_DESKTOP_FILENAME:
			xfdashboard_application_button_set_desktop_filename(self, g_value_get_string(inValue));
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
		case PROP_MENU_ELEMENT:
			g_value_set_object(outValue, priv->menuElement);
			break;

		case PROP_DESKTOP_FILENAME:
			g_value_set_string(outValue, priv->desktopFilename);
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
	GObjectClass		*gobjectClass=G_OBJECT_CLASS(klass);

	/* Override functions */
	gobjectClass->dispose=_xfdashboard_application_button_dispose;
	gobjectClass->set_property=_xfdashboard_application_button_set_property;
	gobjectClass->get_property=_xfdashboard_application_button_get_property;

	/* Set up private structure */
	g_type_class_add_private(klass, sizeof(XfdashboardApplicationButtonPrivate));

	/* Define properties */
	XfdashboardApplicationButtonProperties[PROP_MENU_ELEMENT]=
		g_param_spec_object("menu-element",
								_("Menu element"),
								_("The menu element whose title and description to display"),
								GARCON_TYPE_MENU_ELEMENT,
								G_PARAM_READWRITE);

	XfdashboardApplicationButtonProperties[PROP_DESKTOP_FILENAME]=
		g_param_spec_string("desktop-filename",
								_("Desktop file name"),
								_("File name of desktop file whose title and description to display"),
								NULL,
								G_PARAM_READWRITE);

	XfdashboardApplicationButtonProperties[PROP_SHOW_DESCRIPTION]=
		g_param_spec_boolean("show-description",
								_("Show description"),
								_("Show also description next to tile"),
								FALSE,
								G_PARAM_READWRITE);

	XfdashboardApplicationButtonProperties[PROP_FORMAT_TITLE_ONLY]=
		g_param_spec_string("format-title-only",
								_("Format title only"),
								_("Format string used when only title is display"),
								DEFAULT_FORMAT_TITLE_ONLY,
								G_PARAM_READWRITE | G_PARAM_CONSTRUCT);

	XfdashboardApplicationButtonProperties[PROP_FORMAT_TITLE_DESCRIPTION]=
		g_param_spec_string("format-title-description",
								_("Format title and description"),
								_("Format string used when title and description is display. First argument is title and second one is description."),
								DEFAULT_FORMAT_TITLE_DESCRIPTION,
								G_PARAM_READWRITE | G_PARAM_CONSTRUCT);

	g_object_class_install_properties(gobjectClass, PROP_LAST, XfdashboardApplicationButtonProperties);
}

/* Object initialization
 * Create private structure and set up default values
 */
static void xfdashboard_application_button_init(XfdashboardApplicationButton *self)
{
	XfdashboardApplicationButtonPrivate	*priv;

	priv=self->priv=XFDASHBOARD_APPLICATION_BUTTON_GET_PRIVATE(self);

	/* This actor is react on events */
	clutter_actor_set_reactive(CLUTTER_ACTOR(self), TRUE);

	/* Set up default values */
	priv->type=XFDASHBOARD_APPLICATION_BUTTON_TYPE_NONE;
	priv->menuElement=NULL;
	priv->appInfo=NULL;
	priv->desktopFilename=NULL;
	priv->showDescription=FALSE;

	/* Connect signals */
	g_signal_connect(self, "notify::icon-size", G_CALLBACK(_xfdashboard_application_button_on_icon_size_changed), NULL);
}

/* IMPLEMENTATION: Public API */

/* Create new actor */
ClutterActor* xfdashboard_application_button_new(void)
{
	return(g_object_new(XFDASHBOARD_TYPE_APPLICATION_BUTTON,
							"style", XFDASHBOARD_STYLE_BOTH,
							"single-line", FALSE,
							NULL));
}

ClutterActor* xfdashboard_application_button_new_from_desktop_file(const gchar *inDesktopFilename)
{
	g_return_val_if_fail(inDesktopFilename, NULL);

	return(g_object_new(XFDASHBOARD_TYPE_APPLICATION_BUTTON,
							"style", XFDASHBOARD_STYLE_BOTH,
							"single-line", FALSE,
							"desktop-filename", inDesktopFilename,
							NULL));
}

ClutterActor* xfdashboard_application_button_new_from_menu(GarconMenuElement *inMenuElement)
{
	g_return_val_if_fail(GARCON_IS_MENU_ELEMENT(inMenuElement), NULL);

	return(g_object_new(XFDASHBOARD_TYPE_APPLICATION_BUTTON,
							"style", XFDASHBOARD_STYLE_BOTH,
							"single-line", FALSE,
							"menu-element", inMenuElement,
							NULL));
}

/* Get/set menu element of application button */
GarconMenuElement* xfdashboard_application_button_get_menu_element(XfdashboardApplicationButton *self)
{
	g_return_val_if_fail(XFDASHBOARD_IS_APPLICATION_BUTTON(self), NULL);

	return(self->priv->menuElement);
}

void xfdashboard_application_button_set_menu_element(XfdashboardApplicationButton *self, GarconMenuElement *inMenuElement)
{
	XfdashboardApplicationButtonPrivate		*priv;
	const gchar								*desktopName;
	GDesktopAppInfo							*appInfo;

	g_return_if_fail(XFDASHBOARD_IS_APPLICATION_BUTTON(self));
	g_return_if_fail(GARCON_IS_MENU_ELEMENT(inMenuElement));

	priv=self->priv;

	/* Set value if changed */
	if(priv->type!=XFDASHBOARD_APPLICATION_BUTTON_TYPE_MENU_ITEM ||
		garcon_menu_element_equal(inMenuElement, priv->menuElement)==FALSE)
	{
		/* Freeze notifications and collect them */
		g_object_freeze_notify(G_OBJECT(self));

		/* Clear application button */
		_xfdashboard_application_button_clear(self);

		/* Set value */
		priv->type=XFDASHBOARD_APPLICATION_BUTTON_TYPE_MENU_ITEM;
		priv->menuElement=g_object_ref(GARCON_MENU_ELEMENT(inMenuElement));

		/* Update actor */
		_xfdashboard_application_button_update_text(self);
		_xfdashboard_application_button_update_icon(self);

		/* Notify about property change */
		g_object_notify_by_pspec(G_OBJECT(self), XfdashboardApplicationButtonProperties[PROP_MENU_ELEMENT]);

		/* Get desktop ID if available */
		if(GARCON_IS_MENU_ITEM(inMenuElement))
		{
			/* Get desktop file. First we try desktop ID but it may be unresolvable by GIO,
			 * e.g. Wine uses multi-level subdirectories. GIO (and MenuSpec?) only resolve
			 * subdirectories in one level. If resolving by desktop ID does not work we
			 * fallback to use the full absolute URI (converted to path) to get desktop file.
			 */
			desktopName=garcon_menu_item_get_desktop_id(GARCON_MENU_ITEM(inMenuElement));
			if(desktopName)
			{
				appInfo=g_desktop_app_info_new(desktopName);
				if(!appInfo)
				{
					desktopName=garcon_menu_item_get_uri(GARCON_MENU_ITEM(inMenuElement));
					if(desktopName) priv->desktopFilename=g_filename_from_uri(desktopName, NULL, NULL);
				}
					else
					{
						g_object_unref(appInfo);
						priv->desktopFilename=g_strdup(desktopName);
					}
			}

			/* Notify about property change */
			g_object_notify_by_pspec(G_OBJECT(self), XfdashboardApplicationButtonProperties[PROP_DESKTOP_FILENAME]);
		}

		/* Thaw notifications and send them now */
		g_object_thaw_notify(G_OBJECT(self));
	}
}

/* Get/set desktop filename of application button */
const gchar* xfdashboard_application_button_get_desktop_filename(XfdashboardApplicationButton *self)
{
	g_return_val_if_fail(XFDASHBOARD_IS_APPLICATION_BUTTON(self), NULL);

	return(self->priv->desktopFilename);
}

void xfdashboard_application_button_set_desktop_filename(XfdashboardApplicationButton *self, const gchar *inDesktopFilename)
{
	XfdashboardApplicationButtonPrivate		*priv;

	g_return_if_fail(XFDASHBOARD_IS_APPLICATION_BUTTON(self));
	g_return_if_fail(inDesktopFilename!=NULL);

	priv=self->priv;

	/* Set value if changed */
	if(priv->type!=XFDASHBOARD_APPLICATION_BUTTON_TYPE_DESKTOP_FILE ||
		g_strcmp0(inDesktopFilename, priv->desktopFilename)!=0)
	{
		/* Freeze notifications and collect them */
		g_object_freeze_notify(G_OBJECT(self));

		/* Clear application button */
		_xfdashboard_application_button_clear(self);

		/* Set value */
		priv->type=XFDASHBOARD_APPLICATION_BUTTON_TYPE_DESKTOP_FILE;
		priv->desktopFilename=g_strdup(inDesktopFilename);

		/* Get new application information of basename of desktop file given */
		if(priv->desktopFilename)
		{
			if(g_path_is_absolute(priv->desktopFilename))
			{
				priv->appInfo=G_APP_INFO(g_desktop_app_info_new_from_filename(priv->desktopFilename));
			}
				else
				{
					priv->appInfo=G_APP_INFO(g_desktop_app_info_new(priv->desktopFilename));
				}

			if(!priv->appInfo)
			{
				g_warning(_("Could not get application info for '%s'"), priv->desktopFilename);
			}
		}

		/* Update actor */
		_xfdashboard_application_button_update_text(self);
		_xfdashboard_application_button_update_icon(self);

		/* Notify about property change */
		g_object_notify_by_pspec(G_OBJECT(self), XfdashboardApplicationButtonProperties[PROP_DESKTOP_FILENAME]);

		/* Thaw notifications and send them now */
		g_object_thaw_notify(G_OBJECT(self));
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

/* Get application information */
GAppInfo* xfdashboard_application_button_get_app_info(XfdashboardApplicationButton *self)
{
	XfdashboardApplicationButtonPrivate		*priv;
	GAppInfo								*appInfo;

	g_return_val_if_fail(XFDASHBOARD_IS_APPLICATION_BUTTON(self), FALSE);

	priv=self->priv;
	appInfo=NULL;

	/* Get GAppInfo depending on type */
	switch(priv->type)
	{
		case XFDASHBOARD_APPLICATION_BUTTON_TYPE_NONE:
			g_warning(_("No application information for an unconfigured application button."));
			break;

		case XFDASHBOARD_APPLICATION_BUTTON_TYPE_MENU_ITEM:
			{
				GarconMenuItem			*menuItem=GARCON_MENU_ITEM(priv->menuElement);
				const gchar				*command=garcon_menu_item_get_command(menuItem);
				const gchar				*name=garcon_menu_item_get_name(menuItem);
				GAppInfoCreateFlags		flags=G_APP_INFO_CREATE_NONE;
				GError					*error=NULL;

				/* Create application info for launching */
				if(garcon_menu_item_supports_startup_notification(menuItem)) flags|=G_APP_INFO_CREATE_SUPPORTS_STARTUP_NOTIFICATION;
				if(garcon_menu_item_requires_terminal(menuItem)) flags|=G_APP_INFO_CREATE_NEEDS_TERMINAL;

				appInfo=g_app_info_create_from_commandline(command, name, flags, &error);
				if(error)
				{
					g_warning(_("Could not create application information for menu item '%s': %s"),
								name,
								error->message ? error->message : "unknown error");
					g_error_free(error);
				}
			}
			break;

		case XFDASHBOARD_APPLICATION_BUTTON_TYPE_DESKTOP_FILE:
			if(priv->appInfo) appInfo=G_APP_INFO(g_object_ref(priv->appInfo));
			break;

		default:
			g_assert_not_reached();
			break;
	}

	/* Return GAppInfo */
	return(appInfo);
}

/* Get display name of application button */
const gchar* xfdashboard_application_button_get_display_name(XfdashboardApplicationButton *self)
{
	XfdashboardApplicationButtonPrivate		*priv;
	const gchar								*title;

	g_return_val_if_fail(XFDASHBOARD_IS_APPLICATION_BUTTON(self), NULL);

	priv=self->priv;
	title=NULL;

	/* Get title where available */
	switch(priv->type)
	{
		case XFDASHBOARD_APPLICATION_BUTTON_TYPE_NONE:
			/* Do nothing */
			break;

		case XFDASHBOARD_APPLICATION_BUTTON_TYPE_MENU_ITEM:
			title=garcon_menu_element_get_name(priv->menuElement);
			break;

		case XFDASHBOARD_APPLICATION_BUTTON_TYPE_DESKTOP_FILE:
			if(priv->appInfo) title=g_app_info_get_name(priv->appInfo);
				else title=priv->desktopFilename;
			break;

		default:
			g_critical(_("Cannot get display name of application icon because of unknown type (%d)"), priv->type);
			break;
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
	switch(priv->type)
	{
		case XFDASHBOARD_APPLICATION_BUTTON_TYPE_NONE:
			/* Do nothing */
			break;

		case XFDASHBOARD_APPLICATION_BUTTON_TYPE_MENU_ITEM:
			iconName=garcon_menu_element_get_icon_name(priv->menuElement);
			break;

		case XFDASHBOARD_APPLICATION_BUTTON_TYPE_DESKTOP_FILE:
			if(priv->appInfo)
			{
				GIcon						*gicon;

				gicon=g_app_info_get_icon(priv->appInfo);
				if(gicon) iconName=g_icon_to_string(gicon);
			}
			break;

		default:
			g_critical(_("Cannot get icon name of application icon because of unknown type (%d)"), priv->type);
			break;
	}

	/* Return display name */
	return(iconName);
}

/* Execute command of represented application by application button */
gboolean xfdashboard_application_button_execute(XfdashboardApplicationButton *self, GAppLaunchContext *inContext)
{
	XfdashboardApplicationButtonPrivate		*priv;
	GAppInfo								*appInfo;
	GError									*error;
	gboolean								started;
	GAppLaunchContext						*context;

	g_return_val_if_fail(XFDASHBOARD_IS_APPLICATION_BUTTON(self), FALSE);
	g_return_val_if_fail(inContext==NULL || G_IS_APP_LAUNCH_CONTEXT(inContext), FALSE);

	priv=self->priv;
	started=FALSE;

	/* Launch application */
	appInfo=xfdashboard_application_button_get_app_info(self);
	if(!appInfo)
	{
		xfdashboard_notify(CLUTTER_ACTOR(self),
							GTK_STOCK_DIALOG_ERROR,
							_("Launching application '%s' failed: %s"),
							priv->desktopFilename,
							_("No information available for application"));
		g_warning(_("Launching application '%s' failed: %s"),
					priv->desktopFilename,
					_("No information available for application"));
		return(FALSE);
	}

	if(inContext) context=G_APP_LAUNCH_CONTEXT(g_object_ref(inContext));
		else context=xfdashboard_create_app_context(NULL);

	error=NULL;
	if(!g_app_info_launch(appInfo, NULL, context, &error))
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
	g_object_unref(appInfo);
	g_object_unref(context);

	/* Return status */
	return(started);
}
