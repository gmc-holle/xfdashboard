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
#include "enums.h"

#include <glib/gi18n-lib.h>

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
};

/* Properties */
enum
{
	PROP_0,

	PROP_MENU_ELEMENT,
	PROP_DESKTOP_FILENAME,

	PROP_SHOW_DESCRIPTION,

	PROP_LAST
};

static GParamSpec* XfdashboardApplicationButtonProperties[PROP_LAST]={ 0, };


/* IMPLEMENTATION: Private variables and methods */
#define DEFAULT_ICON_SIZE		64			// TODO: Replace by settings/theming object

/* Reset and release allocated resources of application button */
void _xfdashboard_application_button_clear(XfdashboardApplicationButton *self)
{
	g_return_if_fail(XFDASHBOARD_IS_APPLICATION_BUTTON(self));

	XfdashboardApplicationButtonPrivate		*priv=self->priv;

	/* Release allocated resources */
	if(priv->menuElement)
	{
		g_object_unref(priv->menuElement);
		priv->menuElement=NULL;
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
	}

	/* Reset application button */
	priv->type=XFDASHBOARD_APPLICATION_BUTTON_TYPE_NONE;
}

/* Update button actor */
void _xfdashboard_application_button_update(XfdashboardApplicationButton *self)
{
	g_return_if_fail(XFDASHBOARD_IS_APPLICATION_BUTTON(self));

	XfdashboardApplicationButtonPrivate		*priv=self->priv;
	const gchar								*title=NULL;
	const gchar								*description=NULL;
	gchar									*text=NULL;
	ClutterImage							*icon=NULL;

	/* Get title, description and icon where available */
	switch(priv->type)
	{
		case XFDASHBOARD_APPLICATION_BUTTON_TYPE_NONE:
			/* Do nothing */
			break;

		case XFDASHBOARD_APPLICATION_BUTTON_TYPE_MENU_ITEM:
			title=garcon_menu_element_get_name(priv->menuElement);
			description=garcon_menu_element_get_comment(priv->menuElement);
			icon=xfdashboard_get_image_for_icon_name(garcon_menu_element_get_icon_name(priv->menuElement), DEFAULT_ICON_SIZE);
			break;

		case XFDASHBOARD_APPLICATION_BUTTON_TYPE_DESKTOP_FILE:
			title=g_app_info_get_name(priv->appInfo);
			description=g_app_info_get_description(priv->appInfo);
			icon=xfdashboard_get_image_for_gicon(g_app_info_get_icon(priv->appInfo), DEFAULT_ICON_SIZE);
			break;

		default:
			g_critical(_("Cannot update application icon of unknown type (%d)"), priv->type);
			break;
	}

	/* Create text depending on show-secondary property */
	if(!priv->showDescription) text=g_strdup_printf("%s", title ? title : "");
		else text=g_strdup_printf("<b><big>%s</big></b>\n\n%s", title ? title : "", description ? description : "");

	/* Set up button */
	xfdashboard_button_set_text(XFDASHBOARD_BUTTON(self), text);
	if(icon)
	{
		xfdashboard_button_set_icon_image(XFDASHBOARD_BUTTON(self), icon);
		xfdashboard_button_set_style(XFDASHBOARD_BUTTON(self), XFDASHBOARD_STYLE_BOTH);
	}
		else xfdashboard_button_set_style(XFDASHBOARD_BUTTON(self), XFDASHBOARD_STYLE_TEXT);
	xfdashboard_button_set_icon_size(XFDASHBOARD_BUTTON(self), DEFAULT_ICON_SIZE);

	/* Release allocated resources */
	if(text) g_free(text);
	if(icon) g_object_unref(icon);
}

/* IMPLEMENTATION: GObject */

/* Dispose this object */
static void xfdashboard_application_button_dispose(GObject *inObject)
{
	XfdashboardApplicationButton		*self=XFDASHBOARD_APPLICATION_BUTTON(inObject);

	/* Release our allocated variables */
	_xfdashboard_application_button_clear(self);

	/* Call parent's class dispose method */
	G_OBJECT_CLASS(xfdashboard_application_button_parent_class)->dispose(inObject);
}

/* Set/get properties */
static void xfdashboard_application_button_set_property(GObject *inObject,
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

		default:
			G_OBJECT_WARN_INVALID_PROPERTY_ID(inObject, inPropID, inSpec);
			break;
	}
}

static void xfdashboard_application_button_get_property(GObject *inObject,
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

		default:
			G_OBJECT_WARN_INVALID_PROPERTY_ID(inObject, inPropID, inSpec);
			break;
	}
}

/* Class initialization
 * Override functions in parent classes and define properties
 * and signals
 */
void xfdashboard_application_button_class_init(XfdashboardApplicationButtonClass *klass)
{
	GObjectClass		*gobjectClass=G_OBJECT_CLASS(klass);

	/* Override functions */
	gobjectClass->dispose=xfdashboard_application_button_dispose;
	gobjectClass->set_property=xfdashboard_application_button_set_property;
	gobjectClass->get_property=xfdashboard_application_button_get_property;

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

	g_object_class_install_properties(gobjectClass, PROP_LAST, XfdashboardApplicationButtonProperties);
}

/* Object initialization
 * Create private structure and set up default values
 */
void xfdashboard_application_button_init(XfdashboardApplicationButton *self)
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
}

/* Implementation: Public API */

/* Create new actor */
ClutterActor* xfdashboard_application_button_new(void)
{
	return(g_object_new(XFDASHBOARD_TYPE_APPLICATION_BUTTON,
							"single-line", FALSE,
							NULL));
}

ClutterActor* xfdashboard_application_button_new_from_desktop_file(const gchar *inDesktopFilename)
{
	g_return_val_if_fail(inDesktopFilename, NULL);

	return(g_object_new(XFDASHBOARD_TYPE_APPLICATION_BUTTON,
							"single-line", FALSE,
							"desktop-filename", inDesktopFilename,
							NULL));
}

ClutterActor* xfdashboard_application_button_new_from_menu(GarconMenuElement *inMenuElement)
{
	g_return_val_if_fail(GARCON_IS_MENU_ELEMENT(inMenuElement), NULL);

	return(g_object_new(XFDASHBOARD_TYPE_APPLICATION_BUTTON,
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
	g_return_if_fail(XFDASHBOARD_IS_APPLICATION_BUTTON(self));
	g_return_if_fail(GARCON_IS_MENU_ELEMENT(inMenuElement));

	XfdashboardApplicationButtonPrivate		*priv=self->priv;

	/* Set value if changed */
	if(priv->type!=XFDASHBOARD_APPLICATION_BUTTON_TYPE_MENU_ITEM ||
		garcon_menu_element_equal(inMenuElement, priv->menuElement)==FALSE)
	{
		/* Clear application button */
		_xfdashboard_application_button_clear(self);

		/* Set value */
		priv->type=XFDASHBOARD_APPLICATION_BUTTON_TYPE_MENU_ITEM;
		priv->menuElement=g_object_ref(GARCON_MENU_ELEMENT(inMenuElement));

		/* Update actor */
		_xfdashboard_application_button_update(self);

		/* Notify about property change */
		g_object_notify_by_pspec(G_OBJECT(self), XfdashboardApplicationButtonProperties[PROP_MENU_ELEMENT]);
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
	g_return_if_fail(XFDASHBOARD_IS_APPLICATION_BUTTON(self));
	g_return_if_fail(inDesktopFilename!=NULL);

	XfdashboardApplicationButtonPrivate		*priv=self->priv;

	/* Set value if changed */
	if(priv->type!=XFDASHBOARD_APPLICATION_BUTTON_TYPE_DESKTOP_FILE ||
		g_strcmp0(inDesktopFilename, priv->desktopFilename)!=0)
	{
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
				g_warning(_("Could not get application info '%s'."), priv->desktopFilename);
			}
		}

		/* Update actor */
		_xfdashboard_application_button_update(self);

		/* Notify about property change */
		g_object_notify_by_pspec(G_OBJECT(self), XfdashboardApplicationButtonProperties[PROP_DESKTOP_FILENAME]);
	}
}

/* Get/set flag to show description */
gboolean xfdashboard_application_button_get_show_description(XfdashboardApplicationButton *self)
{
	g_return_val_if_fail(XFDASHBOARD_IS_APPLICATION_BUTTON(self), NULL);

	return(self->priv->showDescription);
}

void xfdashboard_application_button_set_show_description(XfdashboardApplicationButton *self, gboolean inShowDescription)
{
	g_return_if_fail(XFDASHBOARD_IS_APPLICATION_BUTTON(self));

	XfdashboardApplicationButtonPrivate		*priv=self->priv;

	/* Set value if changed */
	if(priv->showDescription!=inShowDescription)
	{
		/* Set value */
		priv->showDescription=inShowDescription;

		/* Update actor */
		_xfdashboard_application_button_update(self);

		/* Notify about property change */
		g_object_notify_by_pspec(G_OBJECT(self), XfdashboardApplicationButtonProperties[PROP_SHOW_DESCRIPTION]);
	}
}

/* Execute command of represented application by application button */
gboolean xfdashboard_application_button_execute(XfdashboardApplicationButton *self)
{
	g_return_val_if_fail(XFDASHBOARD_IS_APPLICATION_BUTTON(self), FALSE);

	XfdashboardApplicationButtonPrivate		*priv=self->priv;
	GAppInfo								*appInfo=NULL;
	GAppInfoCreateFlags						flags=G_APP_INFO_CREATE_NONE;
	GError									*error=NULL;
	gboolean								started=FALSE;

	switch(priv->type)
	{
		case XFDASHBOARD_APPLICATION_BUTTON_TYPE_NONE:
			g_warning(_("Cannot execute command of an unconfigured application button."));
			return(FALSE);

		case XFDASHBOARD_APPLICATION_BUTTON_TYPE_MENU_ITEM:
			{
				GarconMenuItem			*menuItem=GARCON_MENU_ITEM(priv->menuElement);
				const gchar				*command=garcon_menu_item_get_command(priv->menuElement);
				const gchar				*name=garcon_menu_item_get_name(priv->menuElement);

				/* Create application info for launching */
				if(garcon_menu_item_supports_startup_notification(menuItem)) flags|=G_APP_INFO_CREATE_SUPPORTS_STARTUP_NOTIFICATION;
				if(garcon_menu_item_requires_terminal(menuItem)) flags|=G_APP_INFO_CREATE_NEEDS_TERMINAL;

				appInfo=g_app_info_create_from_commandline(command, name, flags, &error);
				if(error)
				{
					g_warning(_("Could not create application information for command '%s': %s"),
								command,
								error->message ? error->message : "unknown error");
					g_error_free(error);
				}
			}
			break;

		case XFDASHBOARD_APPLICATION_BUTTON_TYPE_DESKTOP_FILE:
			appInfo=G_APP_INFO(g_object_ref(priv->appInfo));
			break;

		default:
			g_critical(_("Cannot execute command of application button of unknown type (%d)"), priv->type);
			return(FALSE);
	}

	/* Launch application */
	error=NULL;
	if(!g_app_info_launch(appInfo, NULL, NULL, &error))
	{
		g_warning(_("Could not launch application: %s"),
					(error && error->message) ? error->message : "unknown error");
		if(error) g_error_free(error);
	}
		else started=TRUE;

	/* Clean up allocated resources */
	g_object_unref(appInfo);

	/* Return status */
	return(started);
}
