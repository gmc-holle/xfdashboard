/*
 * application-icon.c: An actor representing an application
 * 
 * Copyright 2012 Stephan Haller <nomad@froevel.de>
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

#include "application-icon.h"
#include "common.h"

#include <gdk/gdk.h>
#include <gtk/gtk.h>
#include <math.h>

/* Define this class in GObject system */
G_DEFINE_TYPE(XfdashboardApplicationIcon,
				xfdashboard_application_icon,
				XFDASHBOARD_TYPE_BUTTON)

/* Private structure - access only by public API if needed */
#define XFDASHBOARD_APPLICATION_ICON_GET_PRIVATE(obj) \
	(G_TYPE_INSTANCE_GET_PRIVATE((obj), XFDASHBOARD_TYPE_APPLICATION_ICON, XfdashboardApplicationIconPrivate))

typedef enum
{
	eTypeNone,
	eTypeDesktopFile,
	eTypeMenuItem,
	eTypeCustom
} XfdashboardApplicationIconType;

struct _XfdashboardApplicationIconPrivate
{
	/* Application information this actor represents */
	XfdashboardApplicationIconType	type;
	GAppInfo						*appInfo;
	GarconMenuElement				*menuElement;
	GarconMenuElement				*customMenuElement;
	gchar							*customIconName;
	gchar							*customTitle;
	gchar							*customDescription;
};

/* Properties */
enum
{
	PROP_0,

	PROP_DESKTOP_FILE,
	PROP_MENU_ELEMENT,
	PROP_CUSTOM_MENU_ELEMENT,
	PROP_CUSTOM_ICON,
	PROP_CUSTOM_TITLE,
	PROP_CUSTOM_DESCRIPTION,
	PROP_APPLICATION_INFO,

	PROP_LAST
};

static GParamSpec* XfdashboardApplicationIconProperties[PROP_LAST]={ 0, };

/* IMPLEMENTATION: Private variables and methods */
#define DEFAULT_ICON_SIZE				64

/* Reset and clear this icon */
void _xfdashboard_application_icon_clear(XfdashboardApplicationIcon *self)
{
	g_return_if_fail(XFDASHBOARD_IS_APPLICATION_ICON(self));

	/* Release any allocated resource and reset type to none */
	XfdashboardApplicationIconPrivate	*priv=XFDASHBOARD_APPLICATION_ICON(self)->priv;

	priv->type=eTypeNone;
	
	if(priv->appInfo) g_object_unref(priv->appInfo);
	priv->appInfo=NULL;

	if(priv->menuElement) g_object_unref(priv->menuElement);
	priv->menuElement=NULL;

	if(priv->customMenuElement) g_object_unref(priv->customMenuElement);
	priv->customMenuElement=NULL;

	if(priv->customIconName) g_free(priv->customIconName);
	priv->customIconName=NULL;

	if(priv->customTitle) g_free(priv->customTitle);
	priv->customTitle=NULL;

	if(priv->customDescription) g_free(priv->customDescription);
	priv->customDescription=NULL;
}

/* Set desktop file and setup actors for display application icon */
void _xfdashboard_application_icon_set_desktop_file(XfdashboardApplicationIcon *self, const gchar *inDesktopFile)
{
	XfdashboardApplicationIconPrivate	*priv;
	GIcon								*icon=NULL;
	GdkPixbuf							*iconPixbuf=NULL;
	GtkIconInfo							*iconInfo=NULL;
	GError								*error=NULL;

	g_return_if_fail(XFDASHBOARD_IS_APPLICATION_ICON(self));

	/* Get private structure */
	priv=XFDASHBOARD_APPLICATION_ICON(self)->priv;

	/* Clear icon */
	_xfdashboard_application_icon_clear(self);

	/* Set type to desktop file */
	priv->type=eTypeDesktopFile;

	/* Get new application information of basename of desktop file given */
	if(inDesktopFile)
	{
		if(g_path_is_absolute(inDesktopFile))
		{
			priv->appInfo=G_APP_INFO(g_desktop_app_info_new_from_filename(inDesktopFile));
		}
			else
			{
				priv->appInfo=G_APP_INFO(g_desktop_app_info_new(inDesktopFile));
			}

		if(!priv->appInfo)
		{
			g_warning("Could not get application info '%s' for quicklaunch icon",
						inDesktopFile);
		}
	}

	/* Set up icon of application */
	if(priv->appInfo)
	{
		icon=g_app_info_get_icon(G_APP_INFO(priv->appInfo));
		if(icon)
		{
			iconInfo=gtk_icon_theme_lookup_by_gicon(gtk_icon_theme_get_default(),
													icon,
													DEFAULT_ICON_SIZE,
													0);
		}
			else g_warning("Could not get icon for desktop file '%s'", inDesktopFile);
	}

	if(iconInfo)
	{
		error=NULL;
		iconPixbuf=gtk_icon_info_load_icon(iconInfo, &error);
		if(!iconPixbuf)
		{
			g_warning("Could not load icon for quicklaunch icon actor: %s",
						(error && error->message) ?  error->message : "unknown error");
			if(error!=NULL) g_error_free(error);
		}
	}
	
	if(iconPixbuf)
	{
		xfdashboard_button_set_icon_pixbuf(XFDASHBOARD_BUTTON(self), iconPixbuf);
		g_object_unref(iconPixbuf);
	}
		else xfdashboard_button_set_icon(XFDASHBOARD_BUTTON(self), GTK_STOCK_MISSING_IMAGE);
	
	if(iconInfo) gtk_icon_info_free(iconInfo);
	
	/* Set up label actors */
	if(priv->appInfo)
	{
		xfdashboard_button_set_text(XFDASHBOARD_BUTTON(self),
									g_app_info_get_name(G_APP_INFO(priv->appInfo)));
	}
		else xfdashboard_button_set_text(XFDASHBOARD_BUTTON(self), "");

	/* Queue a redraw as the actors are now available */
	clutter_actor_queue_redraw(CLUTTER_ACTOR(self));
}

/* Set menu element and setup actors for display application icon */
void _xfdashboard_application_icon_set_menu_element(XfdashboardApplicationIcon *self, const GarconMenuElement *inMenuElement)
{
	XfdashboardApplicationIconPrivate		*priv;

	g_return_if_fail(XFDASHBOARD_IS_APPLICATION_ICON(self));
	g_return_if_fail(GARCON_IS_MENU_ELEMENT(inMenuElement));

	/* Get private structure */
	priv=XFDASHBOARD_APPLICATION_ICON(self)->priv;

	/* Clear icon */
	_xfdashboard_application_icon_clear(self);

	/* Set type to menu item */
	priv->type=eTypeMenuItem;

	/* Set new menu item */
	priv->menuElement=GARCON_MENU_ELEMENT(g_object_ref((gpointer)inMenuElement));

	/* Set up actors */
	if(!GARCON_IS_MENU(priv->menuElement) && !GARCON_IS_MENU_ITEM(priv->menuElement))
	{
		xfdashboard_button_set_text(XFDASHBOARD_BUTTON(self), "");
	}
		else
		{
			const gchar						*iconName;
			GAppInfo						*appInfo;
			const gchar						*appInfoCommand;
			const gchar						*appInfoName;
			GAppInfoCreateFlags				appInfoFlags;
			GError							*error=NULL;

			/* Get and remember icon name */
			iconName=garcon_menu_element_get_icon_name(priv->menuElement);

			if(iconName)
			{
				/* Get icon for icon name and set icon as texture */
				xfdashboard_button_set_icon(XFDASHBOARD_BUTTON(self), iconName);
			}

			/* Set and remember title */
			xfdashboard_button_set_text(XFDASHBOARD_BUTTON(self), garcon_menu_element_get_name(priv->menuElement));

			/* Create application info */
			if(GARCON_IS_MENU_ITEM(priv->menuElement))
			{
				appInfoCommand=garcon_menu_item_get_command(GARCON_MENU_ITEM(priv->menuElement));
				appInfoName=garcon_menu_item_get_name(GARCON_MENU_ITEM(priv->menuElement));

				appInfoFlags=G_APP_INFO_CREATE_NONE;
				if(garcon_menu_item_supports_startup_notification(GARCON_MENU_ITEM(priv->menuElement))) appInfoFlags|=G_APP_INFO_CREATE_SUPPORTS_STARTUP_NOTIFICATION;
				if(garcon_menu_item_requires_terminal(GARCON_MENU_ITEM(priv->menuElement))) appInfoFlags|=G_APP_INFO_CREATE_NEEDS_TERMINAL;

				appInfo=g_app_info_create_from_commandline(appInfoCommand, appInfoName, appInfoFlags, &error);
				if(!appInfo || error)
				{
					g_warning("Could not create application information for command '%s': %s",
								appInfoCommand,
								(error && error->message) ?  error->message : "unknown error");
					if(error) g_error_free(error);
					if(appInfo) g_object_unref(appInfo);
					appInfo=NULL;
				}

				priv->appInfo=appInfo;
			}
				else
				{
					/* Ensure no application information is set to prevent launching by accident */
					if(G_UNLIKELY(priv->appInfo))
					{
						g_warning("Application information available on menu element of type menu.");
						g_object_unref(priv->appInfo);
						priv->appInfo=NULL;
					}
				}
		}

	/* Queue a redraw as the actors are now available */
	clutter_actor_queue_relayout(CLUTTER_ACTOR(self));
}

/* Set custom values and setup/update actors for display application icon */
void _xfdashboard_application_icon_update_custom(XfdashboardApplicationIcon *self)
{
	g_return_if_fail(XFDASHBOARD_IS_APPLICATION_ICON(self));

	XfdashboardApplicationIconPrivate		*priv;

	/* Get private structure */
	priv=XFDASHBOARD_APPLICATION_ICON(self)->priv;
	g_return_if_fail(priv->type==eTypeCustom);

	/* Get and remember icon name */
	if(priv->customIconName)
	{
		/* Get icon for icon name and set icon as texture*/
		xfdashboard_button_set_icon(XFDASHBOARD_BUTTON(self), priv->customIconName);
	}

	/* Set and remember title */
	if(priv->customTitle) xfdashboard_button_set_text(XFDASHBOARD_BUTTON(self), priv->customTitle);
		else xfdashboard_button_set_text(XFDASHBOARD_BUTTON(self), "");

	/* Ensure no application information is set to prevent launching by accident */
	if(G_UNLIKELY(priv->appInfo))
	{
		g_warning("Application information available on custom icon.");
		g_object_unref(priv->appInfo);
		priv->appInfo=NULL;
	}

	/* Queue a redraw as the actors are now available */
	clutter_actor_queue_relayout(CLUTTER_ACTOR(self));
}

void _xfdashboard_application_icon_set_custom_menu_element(XfdashboardApplicationIcon *self, GarconMenuElement *inMenuElement)
{
	XfdashboardApplicationIconPrivate		*priv;
	GarconMenuElement						*menuElement;
	gchar									*iconName, *title, *description;

	g_return_if_fail(XFDASHBOARD_IS_APPLICATION_ICON(self));
	
	/* Get private structure */
	priv=XFDASHBOARD_APPLICATION_ICON(self)->priv;

	/* Check if value changed */
	if(priv->type==eTypeCustom &&
		inMenuElement &&
		priv->customMenuElement &&
		garcon_menu_element_equal(inMenuElement, priv->customMenuElement)) return;

	/* Get values to set */
	menuElement=GARCON_MENU_ELEMENT(g_object_ref((gpointer)inMenuElement));
	iconName=g_strdup(priv->customIconName);
	title=g_strdup(priv->customTitle);
	description=g_strdup(priv->customDescription);

	/* Clear icon */
	_xfdashboard_application_icon_clear(self);

	/* Set values and set type to custom */
	priv->type=eTypeCustom;
	priv->customMenuElement=menuElement;
	priv->customIconName=iconName;
	priv->customTitle=title;
	priv->customDescription=description;

	/* Update icon actor */
	_xfdashboard_application_icon_update_custom(self);
}

void _xfdashboard_application_icon_set_custom_icon(XfdashboardApplicationIcon *self, const gchar *inIconName)
{
	XfdashboardApplicationIconPrivate		*priv;
	GarconMenuElement						*menuElement;
	gchar									*iconName, *title, *description;

	g_return_if_fail(XFDASHBOARD_IS_APPLICATION_ICON(self));
	
	/* Get private structure */
	priv=XFDASHBOARD_APPLICATION_ICON(self)->priv;

	/* Check if value changed */
	if(priv->type==eTypeCustom && g_strcmp0(inIconName, priv->customIconName)==0) return;

	/* Get values to set */
	menuElement=GARCON_MENU_ELEMENT(g_object_ref((gpointer)priv->customMenuElement));
	iconName=g_strdup(inIconName);
	title=g_strdup(priv->customTitle);
	description=g_strdup(priv->customDescription);

	/* Clear icon */
	_xfdashboard_application_icon_clear(self);

	/* Set values and set type to custom */
	priv->type=eTypeCustom;
	priv->customMenuElement=menuElement;
	priv->customIconName=iconName;
	priv->customTitle=title;
	priv->customDescription=description;

	/* Update icon actor */
	_xfdashboard_application_icon_update_custom(self);
}

void _xfdashboard_application_icon_set_custom_title(XfdashboardApplicationIcon *self, const gchar *inTitle)
{
	XfdashboardApplicationIconPrivate		*priv;
	GarconMenuElement						*menuElement;
	gchar									*iconName, *title, *description;

	g_return_if_fail(XFDASHBOARD_IS_APPLICATION_ICON(self));
	
	/* Get private structure */
	priv=XFDASHBOARD_APPLICATION_ICON(self)->priv;

	/* Check if value changed */
	if(priv->type==eTypeCustom && g_strcmp0(inTitle, priv->customTitle)==0) return;

	/* Get values to set */
	menuElement=GARCON_MENU_ELEMENT(g_object_ref((gpointer)priv->customMenuElement));
	iconName=g_strdup(priv->customIconName);
	title=g_strdup(inTitle);
	description=g_strdup(priv->customDescription);

	/* Clear icon */
	_xfdashboard_application_icon_clear(self);

	/* Set values and set type to custom */
	priv->type=eTypeCustom;
	priv->customMenuElement=menuElement;
	priv->customIconName=iconName;
	priv->customTitle=title;
	priv->customDescription=description;

	/* Update icon actor */
	_xfdashboard_application_icon_update_custom(self);
}

void _xfdashboard_application_icon_set_custom_description(XfdashboardApplicationIcon *self, const gchar *inDescription)
{
	XfdashboardApplicationIconPrivate		*priv;
	GarconMenuElement						*menuElement;
	gchar									*iconName, *title, *description;

	g_return_if_fail(XFDASHBOARD_IS_APPLICATION_ICON(self));
	
	/* Get private structure */
	priv=XFDASHBOARD_APPLICATION_ICON(self)->priv;

	/* Check if value changed */
	if(priv->type==eTypeCustom && g_strcmp0(inDescription, priv->customDescription)==0) return;

	/* Get values to set */
	menuElement=GARCON_MENU_ELEMENT(g_object_ref((gpointer)priv->customMenuElement));
	iconName=g_strdup(priv->customIconName);
	title=g_strdup(priv->customTitle);
	description=g_strdup(inDescription);

	/* Set values and set type to custom */
	priv->type=eTypeCustom;
	priv->customMenuElement=menuElement;
	priv->customIconName=iconName;
	priv->customTitle=title;
	priv->customDescription=description;

	/* Update icon actor */
	_xfdashboard_application_icon_update_custom(self);
}

/* IMPLEMENTATION: GObject */

/* Dispose this object */
static void xfdashboard_application_icon_dispose(GObject *inObject)
{
	/* Dispose allocated resources */
	_xfdashboard_application_icon_clear(XFDASHBOARD_APPLICATION_ICON(inObject));

	/* Call parent's class dispose method */
	G_OBJECT_CLASS(xfdashboard_application_icon_parent_class)->dispose(inObject);
}

/* Set/get properties */
static void xfdashboard_application_icon_set_property(GObject *inObject,
												guint inPropID,
												const GValue *inValue,
												GParamSpec *inSpec)
{
	XfdashboardApplicationIcon			*self=XFDASHBOARD_APPLICATION_ICON(inObject);
	
	switch(inPropID)
	{
		case PROP_DESKTOP_FILE:
			_xfdashboard_application_icon_set_desktop_file(self, g_value_get_string(inValue));
			break;

		case PROP_MENU_ELEMENT:
			xfdashboard_application_icon_set_menu_element(self, GARCON_MENU_ELEMENT(g_value_get_object(inValue)));
			break;

		case PROP_CUSTOM_MENU_ELEMENT:
			_xfdashboard_application_icon_set_custom_menu_element(self, GARCON_MENU_ELEMENT(g_value_get_object(inValue)));
			break;

		case PROP_CUSTOM_ICON:
			_xfdashboard_application_icon_set_custom_icon(self, g_value_get_string(inValue));
			break;

		case PROP_CUSTOM_TITLE:
			_xfdashboard_application_icon_set_custom_title(self, g_value_get_string(inValue));
			break;

		case PROP_CUSTOM_DESCRIPTION:
			_xfdashboard_application_icon_set_custom_description(self, g_value_get_string(inValue));
			break;

		default:
			G_OBJECT_WARN_INVALID_PROPERTY_ID(inObject, inPropID, inSpec);
			break;
	}
}

static void xfdashboard_application_icon_get_property(GObject *inObject,
												guint inPropID,
												GValue *outValue,
												GParamSpec *inSpec)
{
	XfdashboardApplicationIcon	*self=XFDASHBOARD_APPLICATION_ICON(inObject);

	switch(inPropID)
	{
		case PROP_DESKTOP_FILE:
			g_value_set_string(outValue, g_desktop_app_info_get_filename(G_DESKTOP_APP_INFO(self->priv->appInfo)));
			break;

		case PROP_MENU_ELEMENT:
			g_value_set_object(outValue, self->priv->menuElement);
			break;

		case PROP_CUSTOM_MENU_ELEMENT:
			g_value_set_object(outValue, self->priv->customMenuElement);
			break;

		case PROP_CUSTOM_ICON:
			g_value_set_string(outValue, self->priv->customIconName);
			break;

		case PROP_CUSTOM_TITLE:
			g_value_set_string(outValue, self->priv->customTitle);
			break;

		case PROP_CUSTOM_DESCRIPTION:
			g_value_set_string(outValue, self->priv->customDescription);
			break;

		case PROP_APPLICATION_INFO:
			g_value_set_object(outValue, self->priv->appInfo);
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
static void xfdashboard_application_icon_class_init(XfdashboardApplicationIconClass *klass)
{
	GObjectClass		*gobjectClass=G_OBJECT_CLASS(klass);

	/* Override functions */
	gobjectClass->dispose=xfdashboard_application_icon_dispose;
	gobjectClass->set_property=xfdashboard_application_icon_set_property;
	gobjectClass->get_property=xfdashboard_application_icon_get_property;

	/* Set up private structure */
	g_type_class_add_private(klass, sizeof(XfdashboardApplicationIconPrivate));

	/* Define properties */
	XfdashboardApplicationIconProperties[PROP_DESKTOP_FILE]=
		g_param_spec_string("desktop-file",
							"Desktop file",
							"Basename or absolute path of desktop file containing all information about this application",
							"",
							G_PARAM_READWRITE);

	XfdashboardApplicationIconProperties[PROP_MENU_ELEMENT]=
		g_param_spec_object("menu-element",
							"Menu element",
							"Menu element to display",
							GARCON_TYPE_MENU_ELEMENT,
							G_PARAM_READWRITE);

	XfdashboardApplicationIconProperties[PROP_CUSTOM_MENU_ELEMENT]=
		g_param_spec_object("custom-menu-element",
							"Custom menu element",
							"Menu element representing application or menu if it is a custom icon",
							GARCON_TYPE_MENU_ELEMENT,
							G_PARAM_READWRITE);

	XfdashboardApplicationIconProperties[PROP_CUSTOM_ICON]=
		g_param_spec_string("custom-icon-name",
							"Icon name",
							"The themed name or full path to this icon if it is a custom icon",
							"",
							G_PARAM_READWRITE);
							
	XfdashboardApplicationIconProperties[PROP_CUSTOM_TITLE]=
		g_param_spec_string("custom-title",
							"Title",
							"Title of this application icon if it is a custom icon",
							"",
							G_PARAM_READWRITE);

	XfdashboardApplicationIconProperties[PROP_CUSTOM_DESCRIPTION]=
		g_param_spec_string("custom-description",
							"Description",
							"Description of this application icon if it is a custom icon",
							"",
							G_PARAM_READWRITE);

	XfdashboardApplicationIconProperties[PROP_APPLICATION_INFO]=
		g_param_spec_object("application-info",
							"Application information",
							"GAppInfo object containing all information about this application ",
							G_TYPE_DESKTOP_APP_INFO,
							G_PARAM_READABLE);

	g_object_class_install_properties(gobjectClass, PROP_LAST, XfdashboardApplicationIconProperties);
}

/* Object initialization
 * Create private structure and set up default values
 */
static void xfdashboard_application_icon_init(XfdashboardApplicationIcon *self)
{
	XfdashboardApplicationIconPrivate	*priv;

	priv=self->priv=XFDASHBOARD_APPLICATION_ICON_GET_PRIVATE(self);

	/* This actor is react on events */
	clutter_actor_set_reactive(CLUTTER_ACTOR(self), TRUE);

	/* Set up default values */
	priv->type=eTypeNone;
	priv->appInfo=NULL;
	priv->menuElement=NULL;
	priv->customMenuElement=NULL;
	priv->customIconName=NULL;
	priv->customTitle=NULL;
	priv->customDescription=NULL;
}

/* Implementation: Public API */

/* Create new actor */
ClutterActor* xfdashboard_application_icon_new()
{
	return(g_object_new(XFDASHBOARD_TYPE_APPLICATION_ICON,
						NULL));
}

ClutterActor* xfdashboard_application_icon_new_copy(XfdashboardApplicationIcon *inIcon)
{
	g_return_val_if_fail(XFDASHBOARD_IS_APPLICATION_ICON(inIcon), NULL);

	ClutterActor			*newIcon=NULL;
	const gchar				*desktopFile;

	/* We will only copy the source the given application icon is set up from. The new
	 * icon will have default style, orientation and other properties.
	 */
	switch(inIcon->priv->type)
	{
		case eTypeDesktopFile:
			desktopFile=xfdashboard_application_icon_get_desktop_file(inIcon);
			newIcon=xfdashboard_application_icon_new_by_desktop_file(desktopFile);
			break;

		case eTypeMenuItem:
			newIcon=xfdashboard_application_icon_new_by_menu_item(inIcon->priv->menuElement);
			break;

		case eTypeCustom:
			newIcon=xfdashboard_application_icon_new_with_custom(inIcon->priv->customMenuElement,
																	inIcon->priv->customIconName,
																	inIcon->priv->customTitle,
																	inIcon->priv->customDescription);
			break;

		default:
			g_error("Cannot create new application icon from existing one of type %d", inIcon->priv->type);
			break;
	}

	return(newIcon);
}

ClutterActor* xfdashboard_application_icon_new_by_desktop_file(const gchar *inDesktopFile)
{
	return(g_object_new(XFDASHBOARD_TYPE_APPLICATION_ICON,
						"desktop-file", inDesktopFile,
						"style", XFDASHBOARD_STYLE_BOTH,
						"icon-orientation", XFDASHBOARD_ORIENTATION_TOP,
						"sync-icon-size", FALSE,
						"background-visible", FALSE,
						NULL));
}

ClutterActor* xfdashboard_application_icon_new_by_menu_item(const GarconMenuElement *inMenuElement)
{
	return(g_object_new(XFDASHBOARD_TYPE_APPLICATION_ICON,
						"menu-element", inMenuElement,
						"style", XFDASHBOARD_STYLE_BOTH,
						"icon-orientation", XFDASHBOARD_ORIENTATION_TOP,
						"sync-icon-size", FALSE,
						"background-visible", FALSE,
						NULL));
}

ClutterActor* xfdashboard_application_icon_new_with_custom(const GarconMenuElement *inMenuElement,
															const gchar *inIconName,
															const gchar *inTitle,
															const gchar *inDescription)
{
	return(g_object_new(XFDASHBOARD_TYPE_APPLICATION_ICON,
						"custom-menu-element", inMenuElement,
						"custom-icon-name", inIconName,
						"custom-title", inTitle,
						"custom-description", inDescription,
						"style", XFDASHBOARD_STYLE_BOTH,
						"icon-orientation", XFDASHBOARD_ORIENTATION_TOP,
						"sync-icon-size", FALSE,
						"background-visible", FALSE,
						NULL));
}

/* Get/set desktop file */
const gchar* xfdashboard_application_icon_get_desktop_file(XfdashboardApplicationIcon *self)
{
	g_return_val_if_fail(XFDASHBOARD_IS_APPLICATION_ICON(self), NULL);

	return(g_desktop_app_info_get_filename(G_DESKTOP_APP_INFO(self->priv->appInfo)));
}

void xfdashboard_application_icon_set_desktop_file(XfdashboardApplicationIcon *self, const gchar *inDesktopFile)
{
	g_return_if_fail(XFDASHBOARD_IS_APPLICATION_ICON(self));

	_xfdashboard_application_icon_set_desktop_file(self, inDesktopFile);
}

/* Get/set menu element */
const GarconMenuElement* xfdashboard_application_icon_get_menu_element(XfdashboardApplicationIcon *self)
{
	g_return_val_if_fail(XFDASHBOARD_IS_APPLICATION_ICON(self), NULL);

	return(self->priv->type==eTypeCustom ? self->priv->customMenuElement : self->priv->menuElement);
}

void xfdashboard_application_icon_set_menu_element(XfdashboardApplicationIcon *self, GarconMenuElement *inMenuElement)
{
	g_return_if_fail(XFDASHBOARD_IS_APPLICATION_ICON(self));
	g_return_if_fail(GARCON_IS_MENU_ELEMENT(inMenuElement));

	_xfdashboard_application_icon_set_menu_element(self, GARCON_MENU_ELEMENT(inMenuElement));
}

/* Get/set desktop application info */
const GAppInfo* xfdashboard_application_icon_get_application_info(XfdashboardApplicationIcon *self)
{
	g_return_val_if_fail(XFDASHBOARD_IS_APPLICATION_ICON(self), NULL);

	return(self->priv->appInfo);
}
