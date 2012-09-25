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
				CLUTTER_TYPE_ACTOR)

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
	/* Actors */
	ClutterActor					*actorIcon;
	ClutterActor					*actorLabel;
	ClutterActor					*actorLabelBackground;

	/* Application information this actor represents */
	XfdashboardApplicationIconType	type;
	GAppInfo						*appInfo;
	GarconMenuElement				*menuElement;
	GarconMenuElement				*customMenuElement;
	gchar							*customIconName;
	gchar							*customTitle;
	gchar							*customDescription;

	/* Actor actions */
	ClutterAction					*clickAction;

	/* Settings */
	gboolean						showLabel;
	gchar							*labelFont;
	gfloat							labelSpacing;
	gfloat							labelMargin;
	PangoEllipsizeMode				labelEllipsize;
	ClutterColor					*labelTextColor;
	ClutterColor					*labelBackgroundColor;
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

	PROP_LABEL_VISIBILITY,
	PROP_LABEL_FONT,
	PROP_LABEL_COLOR,
	PROP_LABEL_BACKGROUND_COLOR,
	PROP_LABEL_MARGIN,
	PROP_LABEL_ELLIPSIZE_MODE,

	PROP_LAST
};

static GParamSpec* XfdashboardApplicationIconProperties[PROP_LAST]={ 0, };

/* Signals */
enum
{
	CLICKED,

	SIGNAL_LAST
};

static guint XfdashboardApplicationIconSignals[SIGNAL_LAST]={ 0, };

/* Private constants */
#define DEFAULT_FONT	"Cantarell 12px"

static ClutterColor		defaultTextColor={ 0xff, 0xff , 0xff, 0xff };
static ClutterColor		defaultBackgroundColor={ 0x00, 0x00, 0x00, 0xd0 };

/* IMPLEMENTATION: Private variables and methods */
#define DEFAULT_ICON_SIZE		64

/* Reset and clear this icon */
void _xfdashboard_application_icon_clear(XfdashboardApplicationIcon *self)
{
	g_return_if_fail(XFDASHBOARD_IS_APPLICATION_ICON(self));

	/* Release any allocated resource and reset type to none */
	XfdashboardApplicationIconPrivate		*priv;
	priv=XFDASHBOARD_APPLICATION_ICON(self)->priv;

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
		priv->appInfo=G_APP_INFO(g_desktop_app_info_new(inDesktopFile));
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
	
	if(!iconInfo)
	{
		iconInfo=gtk_icon_theme_lookup_icon(gtk_icon_theme_get_default(),
											GTK_STOCK_MISSING_IMAGE,
											DEFAULT_ICON_SIZE,
											0);
		if(!iconInfo) g_error("Could not lookup fallback icon for '%s'", inDesktopFile);
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
		/* Scale icon */
		GdkPixbuf						*oldIcon=iconPixbuf;

		iconPixbuf=gdk_pixbuf_scale_simple(oldIcon,
											DEFAULT_ICON_SIZE,
											DEFAULT_ICON_SIZE,
											GDK_INTERP_BILINEAR);
		g_object_unref(oldIcon);
		
		/* Set texture */
		error=NULL;
		if(!clutter_texture_set_from_rgb_data(CLUTTER_TEXTURE(priv->actorIcon),
													gdk_pixbuf_get_pixels(iconPixbuf),
													gdk_pixbuf_get_has_alpha(iconPixbuf),
													gdk_pixbuf_get_width(iconPixbuf),
													gdk_pixbuf_get_height(iconPixbuf),
													gdk_pixbuf_get_rowstride(iconPixbuf),
													gdk_pixbuf_get_has_alpha(iconPixbuf) ? 4 : 3,
													CLUTTER_TEXTURE_NONE,
													&error))
		{
			g_warning("Could not create quicklaunch icon actor: %s",
						(error && error->message) ?  error->message : "unknown error");
		}

		if(error!=NULL) g_error_free(error);
		g_object_unref(iconPixbuf);
	}
	
	if(iconInfo) gtk_icon_info_free(iconInfo);
	
	/* Set up label actors */
	if(priv->appInfo)
	{
		clutter_text_set_text(CLUTTER_TEXT(priv->actorLabel),
								g_app_info_get_name(G_APP_INFO(priv->appInfo)));
	}
		else clutter_text_set_text(CLUTTER_TEXT(priv->actorLabel), "");

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
		clutter_text_set_text(CLUTTER_TEXT(priv->actorLabel), "");
	}
		else
		{
			GdkPixbuf						*icon;
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
				icon=xfdashboard_get_pixbuf_for_icon_name_scaled(iconName, DEFAULT_ICON_SIZE);
				if(icon)
				{
					/* Set texture */
					error=NULL;
					if(!clutter_texture_set_from_rgb_data(CLUTTER_TEXTURE(priv->actorIcon),
																gdk_pixbuf_get_pixels(icon),
																gdk_pixbuf_get_has_alpha(icon),
																gdk_pixbuf_get_width(icon),
																gdk_pixbuf_get_height(icon),
																gdk_pixbuf_get_rowstride(icon),
																gdk_pixbuf_get_has_alpha(icon) ? 4 : 3,
																CLUTTER_TEXTURE_NONE,
																&error))
					{
						g_warning("Could not create icon of application entry actor: %s",
									(error && error->message) ?  error->message : "unknown error");
					}

					if(error!=NULL) g_error_free(error);
					g_object_unref(icon);
				}
			}

			/* Set and remember title */
			clutter_text_set_markup(CLUTTER_TEXT(priv->actorLabel), garcon_menu_element_get_name(priv->menuElement));

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
	XfdashboardApplicationIconPrivate		*priv;
	GError									*error;

	g_return_if_fail(XFDASHBOARD_IS_APPLICATION_ICON(self));
	
	/* Get private structure */
	priv=XFDASHBOARD_APPLICATION_ICON(self)->priv;
	g_return_if_fail(priv->type==eTypeCustom);
	
	/* Set up actors */
	GdkPixbuf						*icon;

	/* Get and remember icon name */
	if(priv->customIconName)
	{
		/* Get icon for icon name and set icon as texture*/
		icon=xfdashboard_get_pixbuf_for_icon_name_scaled(priv->customIconName, DEFAULT_ICON_SIZE);
		if(icon)
		{
			/* Set texture */
			error=NULL;
			if(!clutter_texture_set_from_rgb_data(CLUTTER_TEXTURE(priv->actorIcon),
														gdk_pixbuf_get_pixels(icon),
														gdk_pixbuf_get_has_alpha(icon),
														gdk_pixbuf_get_width(icon),
														gdk_pixbuf_get_height(icon),
														gdk_pixbuf_get_rowstride(icon),
														gdk_pixbuf_get_has_alpha(icon) ? 4 : 3,
														CLUTTER_TEXTURE_NONE,
														&error))
			{
				g_warning("Could not create icon of application entry actor: %s",
							(error && error->message) ?  error->message : "unknown error");
			}

			if(error!=NULL) g_error_free(error);
			g_object_unref(icon);
		}
	}

	/* Set and remember title */
	clutter_text_set_markup(CLUTTER_TEXT(priv->actorLabel), priv->customTitle);

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

/* proxy ClickAction signals */
static void xfdashboard_application_icon_clicked(ClutterClickAction *inAction,
													ClutterActor *inActor,
													gpointer inUserData)
{
	g_signal_emit(inActor, XfdashboardApplicationIconSignals[CLICKED], 0);
}

/* IMPLEMENTATION: ClutterActor */

/* Show all children of this one */
static void xfdashboard_application_icon_show_all(ClutterActor *self)
{
	XfdashboardApplicationIconPrivate	*priv=XFDASHBOARD_APPLICATION_ICON(self)->priv;

	if(priv->showLabel)
	{
		clutter_actor_show(priv->actorLabel);
		clutter_actor_show(priv->actorLabelBackground);
	}
	clutter_actor_show(priv->actorIcon);
	clutter_actor_show(self);
}

/* Hide all children of this one */
static void xfdashboard_application_icon_hide_all(ClutterActor *self)
{
	XfdashboardApplicationIconPrivate	*priv=XFDASHBOARD_APPLICATION_ICON(self)->priv;

	clutter_actor_hide(self);
	clutter_actor_hide(priv->actorIcon);
	clutter_actor_hide(priv->actorLabel);
	clutter_actor_hide(priv->actorLabelBackground);
}

/* Get preferred width/height */
static void xfdashboard_application_icon_get_preferred_height(ClutterActor *self,
														gfloat inForWidth,
														gfloat *outMinHeight,
														gfloat *outNaturalHeight)
{
	XfdashboardApplicationIconPrivate	*priv=XFDASHBOARD_APPLICATION_ICON(self)->priv;
	gfloat								minHeight, naturalHeight;

	clutter_actor_get_preferred_height(priv->actorIcon,
										inForWidth,
										&minHeight,
										&naturalHeight);

	/* If labels are visible add their height and spacing */
	if(priv->showLabel)
	{
		gfloat							labelMinHeight, labelNaturalHeight;
		gfloat							backgroundMinHeight, backgroundNaturalHeight;
		
		clutter_actor_get_preferred_height(priv->actorLabel,
											inForWidth,
											&labelMinHeight,
											&labelNaturalHeight);

		clutter_actor_get_preferred_height(priv->actorLabelBackground,
											inForWidth,
											&backgroundMinHeight,
											&backgroundNaturalHeight);

		minHeight+=priv->labelSpacing+MAX(labelMinHeight, backgroundMinHeight);
		naturalHeight+=priv->labelSpacing+MAX(labelNaturalHeight, backgroundNaturalHeight);
	}

	/* Store sizes computed */
	if(outMinHeight) *outMinHeight=minHeight;
	if(outNaturalHeight) *outNaturalHeight=naturalHeight;
}

static void xfdashboard_application_icon_get_preferred_width(ClutterActor *self,
														gfloat inForHeight,
														gfloat *outMinWidth,
														gfloat *outNaturalWidth)
{
	XfdashboardApplicationIconPrivate	*priv=XFDASHBOARD_APPLICATION_ICON(self)->priv;
	gfloat								minWidth, naturalWidth;

	clutter_actor_get_preferred_width(priv->actorIcon,
										inForHeight,
										&minWidth,
										&naturalWidth);

	/* If labels are visible check their width */
	if(priv->showLabel)
	{
		gfloat							labelMinWidth, labelNaturalWidth;
		gfloat							backgroundMinWidth, backgroundNaturalWidth;
		
		clutter_actor_get_preferred_width(priv->actorLabel,
											inForHeight,
											&labelMinWidth,
											&labelNaturalWidth);

		clutter_actor_get_preferred_width(priv->actorLabelBackground,
											inForHeight,
											&backgroundMinWidth,
											&backgroundNaturalWidth);

		minWidth=MAX(MAX(labelMinWidth, backgroundMinWidth), minWidth);
		naturalWidth=MAX(MAX(labelNaturalWidth, backgroundNaturalWidth), naturalWidth);
	}

	/* Store sizes computed */
	if(outMinWidth) *outMinWidth=minWidth;
	if(outNaturalWidth) *outNaturalWidth=naturalWidth;
}

/* Allocate position and size of actor and its children*/
static void xfdashboard_application_icon_allocate(ClutterActor *self,
													const ClutterActorBox *inBox,
													ClutterAllocationFlags inFlags)
{
	XfdashboardApplicationIconPrivate	*priv=XFDASHBOARD_APPLICATION_ICON(self)->priv;
	ClutterActorBox						*availableIcon;

	/* Chain up to store the allocation of the actor */
	CLUTTER_ACTOR_CLASS(xfdashboard_application_icon_parent_class)->allocate(self, inBox, inFlags);

	/* Do nothing if icon actor is not available */
	if(G_UNLIKELY(!priv->actorIcon)) return;

	/* Get available allocation for icon. It will get smaller if label is shown. */
	availableIcon=clutter_actor_box_copy(inBox);

	/* Set allocation of label */
	if(priv->showLabel &&
		G_LIKELY(priv->actorLabel) &&
		G_LIKELY(priv->actorLabelBackground))
	{
		ClutterActorBox				*allocationChild;
		gfloat						textWidth, textHeight;
		gfloat						left, right, top, bottom;

		clutter_actor_get_preferred_width(priv->actorLabel, -1, NULL, &textWidth);
		textHeight=clutter_actor_get_height(priv->actorLabel);
		textWidth=MIN(clutter_actor_box_get_width(inBox), textWidth);

		left=(clutter_actor_box_get_width(inBox)-textWidth)/2.0f;
		right=left+textWidth;
		bottom=clutter_actor_box_get_height(inBox)-priv->labelMargin;
		top=bottom-textHeight;
		if(left>=right) left=right-1.0f;
		allocationChild=clutter_actor_box_new(floor(left), floor(top), floor(right), floor(bottom));
		clutter_actor_allocate(priv->actorLabel, allocationChild, inFlags);
		clutter_actor_box_free(allocationChild);
		
		left-=priv->labelMargin;
		top-=priv->labelMargin;
		right+=priv->labelMargin;
		bottom+=priv->labelMargin;
		allocationChild=clutter_actor_box_new(floor(left), floor(top), floor(right), floor(bottom));
		clutter_actor_allocate(priv->actorLabelBackground, allocationChild, inFlags);
		clutter_actor_box_free(allocationChild);

		/* Make allocation for icon smaller to avoid icon painted over label */
		clutter_actor_box_set_size(availableIcon,
									clutter_actor_box_get_width(inBox),
									top-priv->labelSpacing);
	}
	
	/* Set allocation of icon */
	clutter_actor_box_set_origin(availableIcon, 0, 0);
	if(!clutter_actor_box_equal(availableIcon, inBox))
	{
		/* Icon can not occupy whole allocation so scale it down but respect aspect */
		gfloat						left, top, width, height;
		gfloat						iconW, iconH;
		gfloat						aspect;
		gfloat						size;

		clutter_actor_get_preferred_width(priv->actorIcon, -1, NULL, &iconW);
		clutter_actor_get_preferred_height(priv->actorIcon, -1, NULL, &iconH);

		width=clutter_actor_box_get_width(availableIcon);
		height=clutter_actor_box_get_height(availableIcon);
		aspect=(iconH>0 ? iconW/iconH : 1.0f);
		size=MIN(width, height);
		if(width<height)
		{
			height=size;
			width=height/aspect;
		}
			else
			{
				width=size;
				height=width*aspect;
			}
		
		left=((clutter_actor_box_get_width(availableIcon)-width)/2.0f);
		top=((clutter_actor_box_get_height(availableIcon)-height)/2.0f);
		clutter_actor_box_set_origin(availableIcon, floor(left), floor(top));
		clutter_actor_box_set_size(availableIcon, floor(width), floor(height));
	}
	clutter_actor_allocate(priv->actorIcon, availableIcon, inFlags);

	/* Release allocated resources */
	clutter_actor_box_free(availableIcon);
}

/* Paint actor */
static void xfdashboard_application_icon_paint(ClutterActor *self)
{
	XfdashboardApplicationIconPrivate	*priv=XFDASHBOARD_APPLICATION_ICON(self)->priv;

	/* Order of actors being painted is important! */
	if(priv->actorIcon &&
		CLUTTER_ACTOR_IS_MAPPED(priv->actorIcon)) clutter_actor_paint(priv->actorIcon);

	if(priv->showLabel)
	{
		if(priv->actorLabelBackground &&
			CLUTTER_ACTOR_IS_MAPPED(priv->actorLabelBackground)) clutter_actor_paint(priv->actorLabelBackground);

		if(priv->actorLabel &&
			CLUTTER_ACTOR_IS_MAPPED(priv->actorLabel)) clutter_actor_paint(priv->actorLabel);
	}
}

/* Pick this actor and possibly all the child actors.
 * That means that this function should draw a solid shape of actor's silouhette
 * in the given color. This shape is drawn to an invisible offscreen and is used
 * by Clutter to determine an actor fast by inspecting the color at the position.
 * The default implementation is to draw a solid rectangle covering the allocation
 * of THIS actor.
 * If we could not use this default implementation we have chain up to parent class
 * and call the paint function of any child we know and which can be reactive.
 */
static void xfdashboard_application_icon_pick(ClutterActor *self, const ClutterColor *inColor)
{
	/* It is possible to avoid a costly paint by checking
	 * whether the actor should really be painted in pick mode
	 */
	if(!clutter_actor_should_pick_paint(self)) return;

	/* Chain up so we get a bounding box painted (if we are reactive) */
	CLUTTER_ACTOR_CLASS(xfdashboard_application_icon_parent_class)->pick(self, inColor);
}

/* Destroy this actor */
static void xfdashboard_application_icon_destroy(ClutterActor *self)
{
	/* Destroy each child actor when this actor is destroyed */
	XfdashboardApplicationIconPrivate	*priv=XFDASHBOARD_APPLICATION_ICON(self)->priv;

	if(priv->actorIcon) clutter_actor_destroy(priv->actorIcon);
	priv->actorIcon=NULL;

	if(priv->actorLabel) clutter_actor_destroy(priv->actorLabel);
	priv->actorLabel=NULL;

	if(priv->actorLabelBackground) clutter_actor_destroy(priv->actorLabelBackground);
	priv->actorLabelBackground=NULL;

	/* Call parent's class destroy method */
	if(CLUTTER_ACTOR_CLASS(xfdashboard_application_icon_parent_class)->destroy)
	{
		CLUTTER_ACTOR_CLASS(xfdashboard_application_icon_parent_class)->destroy(self);
	}
}

/* IMPLEMENTATION: GObject */

/* Dispose this object */
static void xfdashboard_application_icon_dispose(GObject *inObject)
{
	XfdashboardApplicationIconPrivate	*priv=XFDASHBOARD_APPLICATION_ICON(inObject)->priv;

	/* Dispose allocated resources */
	_xfdashboard_application_icon_clear(XFDASHBOARD_APPLICATION_ICON(inObject));
	
	if(priv->labelFont) g_free(priv->labelFont);
	priv->labelFont=NULL;

	if(priv->labelTextColor) clutter_color_free(priv->labelTextColor);
	priv->labelTextColor=NULL;

	if(priv->labelBackgroundColor) clutter_color_free(priv->labelBackgroundColor);
	priv->labelBackgroundColor=NULL;

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

		case PROP_LABEL_VISIBILITY:
			xfdashboard_application_icon_set_label_visible(self, g_value_get_boolean(inValue));
			break;

		case PROP_LABEL_FONT:
			xfdashboard_application_icon_set_label_font(self, g_value_get_string(inValue));
			break;

		case PROP_LABEL_COLOR:
			xfdashboard_application_icon_set_label_color(self, clutter_value_get_color(inValue));
			break;

		case PROP_LABEL_BACKGROUND_COLOR:
			xfdashboard_application_icon_set_label_background_color(self, clutter_value_get_color(inValue));
			break;

		case PROP_LABEL_MARGIN:
			xfdashboard_application_icon_set_label_margin(self, g_value_get_float(inValue));
			break;

		case PROP_LABEL_ELLIPSIZE_MODE:
			xfdashboard_application_icon_set_label_ellipsize_mode(self, g_value_get_enum(inValue));
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

		case PROP_LABEL_VISIBILITY:
			g_value_set_boolean(outValue, self->priv->showLabel);
			break;

		case PROP_LABEL_FONT:
			g_value_set_string(outValue, self->priv->labelFont);
			break;

		case PROP_LABEL_COLOR:
			clutter_value_set_color(outValue, self->priv->labelTextColor);
			break;

		case PROP_LABEL_BACKGROUND_COLOR:
			clutter_value_set_color(outValue, self->priv->labelBackgroundColor);
			break;

		case PROP_LABEL_MARGIN:
			g_value_set_float(outValue, self->priv->labelMargin);
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
	ClutterActorClass	*actorClass=CLUTTER_ACTOR_CLASS(klass);
	GObjectClass		*gobjectClass=G_OBJECT_CLASS(klass);

	/* Override functions */
	gobjectClass->dispose=xfdashboard_application_icon_dispose;
	gobjectClass->set_property=xfdashboard_application_icon_set_property;
	gobjectClass->get_property=xfdashboard_application_icon_get_property;

	actorClass->show_all=xfdashboard_application_icon_show_all;
	actorClass->hide_all=xfdashboard_application_icon_hide_all;
	actorClass->paint=xfdashboard_application_icon_paint;
	actorClass->pick=xfdashboard_application_icon_pick;
	actorClass->get_preferred_width=xfdashboard_application_icon_get_preferred_width;
	actorClass->get_preferred_height=xfdashboard_application_icon_get_preferred_height;
	actorClass->allocate=xfdashboard_application_icon_allocate;
	actorClass->destroy=xfdashboard_application_icon_destroy;

	/* Set up private structure */
	g_type_class_add_private(klass, sizeof(XfdashboardApplicationIconPrivate));

	/* Define properties */
	XfdashboardApplicationIconProperties[PROP_DESKTOP_FILE]=
		g_param_spec_string("desktop-file",
							"Basename of desktop file",
							"Basename of desktop file containing all information about this application",
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

	XfdashboardApplicationIconProperties[PROP_LABEL_VISIBILITY]=
		g_param_spec_boolean("label-visible",
								"Label visible",
								"Determines if label describing application is shown",
								TRUE,
								G_PARAM_READWRITE | G_PARAM_CONSTRUCT);

	XfdashboardApplicationIconProperties[PROP_LABEL_FONT]=
		g_param_spec_string("label-font",
							"Label font",
							"Font description to use in label",
							DEFAULT_FONT,
							G_PARAM_READWRITE | G_PARAM_CONSTRUCT);

	XfdashboardApplicationIconProperties[PROP_LABEL_COLOR]=
		clutter_param_spec_color("label-color",
									"Label text color",
									"Text color of label",
									&defaultTextColor,
									G_PARAM_READWRITE | G_PARAM_CONSTRUCT);

	XfdashboardApplicationIconProperties[PROP_LABEL_BACKGROUND_COLOR]=
		clutter_param_spec_color("label-background-color",
									"Label background color",
									"Background color of label",
									&defaultBackgroundColor,
									G_PARAM_READWRITE | G_PARAM_CONSTRUCT);

	XfdashboardApplicationIconProperties[PROP_LABEL_MARGIN]=
		g_param_spec_float("label-margin",
							"Label background margin",
							"Margin of label's background in pixels",
							0.0f, G_MAXFLOAT,
							4.0f,
							G_PARAM_READWRITE | G_PARAM_CONSTRUCT);

	XfdashboardApplicationIconProperties[PROP_LABEL_ELLIPSIZE_MODE]=
		g_param_spec_enum("label-ellipsize-mode",
							"Label ellipsize mode",
							"Mode of ellipsize if text in label is too long",
							PANGO_TYPE_ELLIPSIZE_MODE,
							PANGO_ELLIPSIZE_MIDDLE,
							G_PARAM_READWRITE | G_PARAM_CONSTRUCT);

	g_object_class_install_properties(gobjectClass, PROP_LAST, XfdashboardApplicationIconProperties);

	/* Define signals */
	XfdashboardApplicationIconSignals[CLICKED]=
		g_signal_new("clicked",
						G_TYPE_FROM_CLASS(klass),
						G_SIGNAL_RUN_LAST,
						G_STRUCT_OFFSET(XfdashboardApplicationIconClass, clicked),
						NULL,
						NULL,
						g_cclosure_marshal_VOID__VOID,
						G_TYPE_NONE,
						0);

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
	priv->labelFont=NULL;

	/* Create icon actor */
	priv->actorIcon=clutter_texture_new();
	clutter_texture_set_sync_size(CLUTTER_TEXTURE(priv->actorIcon), TRUE);
	clutter_actor_set_parent(priv->actorIcon, CLUTTER_ACTOR(self));

	/* Create label actors */
	priv->actorLabel=clutter_text_new();
	clutter_text_set_line_wrap(CLUTTER_TEXT(priv->actorLabel), TRUE);
	clutter_text_set_single_line_mode(CLUTTER_TEXT(priv->actorLabel), FALSE);
	clutter_text_set_ellipsize(CLUTTER_TEXT(priv->actorLabel), priv->labelEllipsize);
	clutter_actor_set_parent(priv->actorLabel, CLUTTER_ACTOR(self));

	priv->actorLabelBackground=clutter_rectangle_new();
	clutter_actor_set_parent(priv->actorLabelBackground, CLUTTER_ACTOR(self));

	/* Connect signals */
	priv->clickAction=clutter_click_action_new();
	clutter_actor_add_action(CLUTTER_ACTOR(self), priv->clickAction);
	g_signal_connect(priv->clickAction, "clicked", G_CALLBACK(xfdashboard_application_icon_clicked), NULL);
}

/* Implementation: Public API */

/* Create new actor */
ClutterActor* xfdashboard_application_icon_new()
{
	return(g_object_new(XFDASHBOARD_TYPE_APPLICATION_ICON,
						NULL));
}

ClutterActor* xfdashboard_application_icon_new_by_desktop_file(const gchar *inDesktopFile)
{
	return(g_object_new(XFDASHBOARD_TYPE_APPLICATION_ICON,
						"desktop-file", inDesktopFile,
						NULL));
}

ClutterActor* xfdashboard_application_icon_new_by_menu_item(const GarconMenuElement *inMenuElement)
{
	return(g_object_new(XFDASHBOARD_TYPE_APPLICATION_ICON,
						"menu-element", inMenuElement,
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

/* Get/set label's visibility */
const gboolean xfdashboard_application_icon_get_label_visible(XfdashboardApplicationIcon *self)
{
	g_return_val_if_fail(XFDASHBOARD_IS_APPLICATION_ICON(self), FALSE);

	return(self->priv->showLabel);
}

void xfdashboard_application_icon_set_label_visible(XfdashboardApplicationIcon *self, const gboolean inVisible)
{
	g_return_if_fail(XFDASHBOARD_IS_APPLICATION_ICON(self));

	/* Set visibility of label */
	XfdashboardApplicationIconPrivate	*priv=XFDASHBOARD_APPLICATION_ICON(self)->priv;

	if(priv->showLabel!=inVisible)
	{
		priv->showLabel=inVisible;
		clutter_actor_queue_relayout(CLUTTER_ACTOR(self));
	}
}

/* Get/set font to use in label */
const gchar* xfdashboard_application_icon_get_label_font(XfdashboardApplicationIcon *self)
{
	g_return_val_if_fail(XFDASHBOARD_IS_APPLICATION_ICON(self), NULL);

	return(self->priv->labelFont);
}

void xfdashboard_application_icon_set_label_font(XfdashboardApplicationIcon *self, const gchar *inFont)
{
	g_return_if_fail(XFDASHBOARD_IS_APPLICATION_ICON(self));
	g_return_if_fail(inFont);

	/* Set font of label */
	XfdashboardApplicationIconPrivate	*priv=XFDASHBOARD_APPLICATION_ICON(self)->priv;

	if(g_strcmp0(priv->labelFont, inFont)!=0)
	{
		if(priv->labelFont) g_free(priv->labelFont);
		priv->labelFont=g_strdup(inFont);

		clutter_text_set_font_name(CLUTTER_TEXT(priv->actorLabel), priv->labelFont);
		clutter_actor_queue_redraw(CLUTTER_ACTOR(self));
	}
}

/* Get/set color of text in label */
const ClutterColor* xfdashboard_application_icon_get_label_color(XfdashboardApplicationIcon *self)
{
	g_return_val_if_fail(XFDASHBOARD_IS_APPLICATION_ICON(self), NULL);

	return(self->priv->labelTextColor);
}

void xfdashboard_application_icon_set_label_color(XfdashboardApplicationIcon *self, const ClutterColor *inColor)
{
	g_return_if_fail(XFDASHBOARD_IS_APPLICATION_ICON(self));
	g_return_if_fail(inColor);
	
	/* Set text color of label */
	XfdashboardApplicationIconPrivate	*priv=XFDASHBOARD_APPLICATION_ICON(self)->priv;

	if(!priv->labelTextColor || !clutter_color_equal(inColor, priv->labelTextColor))
	{
		if(priv->labelTextColor) clutter_color_free(priv->labelTextColor);
		priv->labelTextColor=clutter_color_copy(inColor);

		clutter_text_set_color(CLUTTER_TEXT(priv->actorLabel), priv->labelTextColor);

		clutter_actor_queue_redraw(CLUTTER_ACTOR(self));
	}
}

/* Get/set color of label's background */
const ClutterColor* xfdashboard_application_icon_get_label_background_color(XfdashboardApplicationIcon *self)
{
	g_return_val_if_fail(XFDASHBOARD_IS_APPLICATION_ICON(self), NULL);

	return(self->priv->labelBackgroundColor);
}

void xfdashboard_application_icon_set_label_background_color(XfdashboardApplicationIcon *self, const ClutterColor *inColor)
{
	g_return_if_fail(XFDASHBOARD_IS_APPLICATION_ICON(self));
	g_return_if_fail(inColor);

	/* Set background color of label */
	XfdashboardApplicationIconPrivate	*priv=XFDASHBOARD_APPLICATION_ICON(self)->priv;

	if(!priv->labelBackgroundColor || !clutter_color_equal(inColor, priv->labelBackgroundColor))
	{
		if(priv->labelBackgroundColor) clutter_color_free(priv->labelBackgroundColor);
		priv->labelBackgroundColor=clutter_color_copy(inColor);

		clutter_rectangle_set_color(CLUTTER_RECTANGLE(priv->actorLabelBackground), priv->labelBackgroundColor);
		clutter_actor_queue_redraw(CLUTTER_ACTOR(self));
	}
}

/* Get/set margin of background to label */
const gfloat xfdashboard_application_icon_get_label_margin(XfdashboardApplicationIcon *self)
{
	g_return_val_if_fail(XFDASHBOARD_IS_APPLICATION_ICON(self), 0);

	return(self->priv->labelMargin);
}

void xfdashboard_application_icon_set_label_margin(XfdashboardApplicationIcon *self, const gfloat inMargin)
{
	g_return_if_fail(XFDASHBOARD_IS_APPLICATION_ICON(self));
	g_return_if_fail(inMargin>=0.0f);

	/* Set window to display */
	XfdashboardApplicationIconPrivate	*priv=XFDASHBOARD_APPLICATION_ICON(self)->priv;

	if(priv->labelMargin!=inMargin)
	{
		priv->labelMargin=inMargin;
		clutter_actor_queue_redraw(CLUTTER_ACTOR(self));
	}
}

/* Get/set ellipsize mode if label's text is getting too long */
const PangoEllipsizeMode xfdashboard_application_icon_get_label_ellipsize_mode(XfdashboardApplicationIcon *self)
{
	g_return_val_if_fail(XFDASHBOARD_IS_APPLICATION_ICON(self), 0);

	return(self->priv->labelEllipsize);
}

void xfdashboard_application_icon_set_label_ellipsize_mode(XfdashboardApplicationIcon *self, const PangoEllipsizeMode inMode)
{
	g_return_if_fail(XFDASHBOARD_IS_APPLICATION_ICON(self));

	/* Set window to display */
	XfdashboardApplicationIconPrivate	*priv=XFDASHBOARD_APPLICATION_ICON(self)->priv;

	if(priv->labelEllipsize!=inMode)
	{
		priv->labelEllipsize=inMode;

		clutter_text_set_ellipsize(CLUTTER_TEXT(priv->actorLabel), priv->labelEllipsize);
		clutter_actor_queue_redraw(CLUTTER_ACTOR(self));
	}
}
