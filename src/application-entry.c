/*
 * application-entry.c: An actor representing an application menu entry
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

#include "application-entry.h"

#include <gdk/gdk.h>
#include <gtk/gtk.h>
#include <string.h>

/* Define this class in GObject system */
G_DEFINE_TYPE(XfdashboardApplicationMenuEntry,
				xfdashboard_application_menu_entry,
				CLUTTER_TYPE_ACTOR)

/* Private structure - access only by public API if needed */
#define XFDASHBOARD_APPLICATION_MENU_ENTRY_GET_PRIVATE(obj) \
	(G_TYPE_INSTANCE_GET_PRIVATE((obj), XFDASHBOARD_TYPE_APPLICATION_MENU_ENTRY, XfdashboardApplicationMenuEntryPrivate))

struct _XfdashboardApplicationMenuEntryPrivate
{
	/* Actors */
	ClutterActor			*actorIcon;
	ClutterActor			*actorTitle;
	ClutterActor			*actorDescription;
	
	/* Application information this actor represents */
	gboolean				isMenu;

	GarconMenuElement		*menuElement;
	gchar					*iconName;
	gchar					*title;
	gchar					*description;

	/* Actor actions */
	ClutterAction			*clickAction;

	/* Settings */
	gfloat					margin;
	gfloat					textSpacing;
	ClutterColor			*backgroundColor;

	gchar					*titleFont;
	PangoEllipsizeMode		titleEllipsize;
	ClutterColor			*titleColor;

	gchar					*descriptionFont;
	PangoEllipsizeMode		descriptionEllipsize;
	ClutterColor			*descriptionColor;
};

/* Properties */
enum
{
	PROP_0,

	PROP_MENU_ELEMENT,
	PROP_ICON_NAME,
	PROP_TITLE,
	PROP_DESCRIPTION,

	PROP_MARGIN,
	PROP_TEXT_SPACING,
	PROP_BACKGROUND_COLOR,
	
	PROP_TITLE_FONT,
	PROP_TITLE_COLOR,
	PROP_TITLE_ELLIPSIZE_MODE,

	PROP_DESCRIPTION_FONT,
	PROP_DESCRIPTION_COLOR,
	PROP_DESCRIPTION_ELLIPSIZE_MODE,

	PROP_LAST
};

static GParamSpec* XfdashboardApplicationMenuEntryProperties[PROP_LAST]={ 0, };

/* Signals */
enum
{
	CLICKED,

	SIGNAL_LAST
};

static guint XfdashboardApplicationMenuEntrySignals[SIGNAL_LAST]={ 0, };

/* Private constants */
#define DEFAULT_TITLE_FONT	"Cantarell 16px"
#define DEFAULT_DESC_FONT	"Cantarell 12px"

static ClutterColor			defaultTitleColor={ 0xff, 0xff , 0xff, 0xff };
static ClutterColor			defaultDescriptionColor={ 0xe0, 0xe0 , 0xe0, 0xff };
static ClutterColor			defaultBackgroundColor={ 0x80, 0x80, 0x80, 0xff };

/* IMPLEMENTATION: Private variables and methods */
#define DEFAULT_ICON_SIZE		64

void _xfdashboard_application_menu_entry_set_menu_item(XfdashboardApplicationMenuEntry *self, const GarconMenuElement *inMenuElement);

/* Set custom icon by themed icon name or absolute file name */
void _xfdashboard_application_menu_entry_set_custom_icon(XfdashboardApplicationMenuEntry *self, const gchar *inIconName)
{
	g_return_if_fail(XFDASHBOARD_IS_APPLICATION_MENU_ENTRY(self));

	/* Set new icon name */
	if(g_strcmp0(self->priv->iconName, inIconName)!=0)
	{
		if(self->priv->iconName) g_free(self->priv->iconName);
		self->priv->iconName=(inIconName ? g_strdup(inIconName) : NULL);

		_xfdashboard_application_menu_entry_set_menu_item(self, self->priv->menuElement);
	}
}

void _xfdashboard_application_menu_entry_set_custom_title(XfdashboardApplicationMenuEntry *self, const gchar *inTitle)
{
	g_return_if_fail(XFDASHBOARD_IS_APPLICATION_MENU_ENTRY(self));

	/* Set new title */
	if(g_strcmp0(self->priv->iconName, inTitle)!=0)
	{
		if(self->priv->title) g_free(self->priv->title);
		self->priv->title=(inTitle ? g_strdup(inTitle) : NULL);

		_xfdashboard_application_menu_entry_set_menu_item(self, self->priv->menuElement);
	}
}

void _xfdashboard_application_menu_entry_set_custom_description(XfdashboardApplicationMenuEntry *self, const gchar *inDescription)
{
	g_return_if_fail(XFDASHBOARD_IS_APPLICATION_MENU_ENTRY(self));

	/* Set new description */
	if(g_strcmp0(self->priv->iconName, inDescription)!=0)
	{
		if(self->priv->description) g_free(self->priv->description);
		self->priv->description=(inDescription ? g_strdup(inDescription) : NULL);

		_xfdashboard_application_menu_entry_set_menu_item(self, self->priv->menuElement);
	}
}

/* Get GdkPixbuf object for themed icon name or absolute icon filename.
 * If icon does not exists the themed icon name in inFallbackIconName
 * will be returned.
 * If even the themed icon name in inFallbackIconName cannot be found
 * we return NULL.
 * The return GdkPixbuf object (if not NULL) must be unreffed with
 * g_object_unref().
 */
GdkPixbuf* _get_pixbuf_for_icon_name(const gchar *inIconName, const gchar *inFallbackIconName)
{
	GdkPixbuf		*icon=NULL;
	GtkIconTheme	*iconTheme=gtk_icon_theme_get_default();
	GError			*error=NULL;
	
	/* Check if we have an absolute filename */
	if(g_path_is_absolute(inIconName) &&
		g_file_test(inIconName, G_FILE_TEST_EXISTS))
	{
		error=NULL;
		icon=gdk_pixbuf_new_from_file_at_scale(inIconName,
												DEFAULT_ICON_SIZE,
												DEFAULT_ICON_SIZE,
												TRUE,
												NULL);

		if(!icon) g_warning("Could not load icon '%s' for application entry actor: %s",
							inIconName, (error && error->message) ?  error->message : "unknown error");

		if(error!=NULL) g_error_free(error);
	}
		else
		{
			/* Try to load the icon name directly using the icon theme */
			error=NULL;
			icon=gtk_icon_theme_load_icon(iconTheme,
											inIconName,
											DEFAULT_ICON_SIZE,
											GTK_ICON_LOOKUP_USE_BUILTIN,
											&error);

			if(!icon) g_warning("Could not load themed icon '%s' for application entry actor: %s",
								inIconName, (error && error->message) ?  error->message : "unknown error");

			if(error!=NULL) g_error_free(error);
		}

	/* If no icon could be loaded use fallback */
	if(!icon && inFallbackIconName)
	{
		error=NULL;
		icon=gtk_icon_theme_load_icon(iconTheme,
										inFallbackIconName,
										DEFAULT_ICON_SIZE,
										GTK_ICON_LOOKUP_USE_BUILTIN,
										&error);

		if(!icon) g_error("Could not load fallback icon for application entry actor: %s",
							(error && error->message) ?  error->message : "unknown error");

		if(error!=NULL) g_error_free(error);
	}

	/* Return icon pixbuf */
	return(icon);
}

/* Set desktop file and setup actors for display application icon */
void _xfdashboard_application_menu_entry_set_menu_item(XfdashboardApplicationMenuEntry *self, const GarconMenuElement *inMenuElement)
{
	XfdashboardApplicationMenuEntryPrivate	*priv;
	GError									*error;

	g_return_if_fail(XFDASHBOARD_IS_APPLICATION_MENU_ENTRY(self));
	g_return_if_fail(GARCON_IS_MENU_ELEMENT(inMenuElement));

	priv=XFDASHBOARD_APPLICATION_MENU_ENTRY(self)->priv;

	/* Set new menu item */
	priv->menuElement=(GarconMenuElement*)inMenuElement;

	/* Set up actors */
	if(!GARCON_IS_MENU(priv->menuElement) && !GARCON_IS_MENU_ITEM(priv->menuElement))
	{
		clutter_text_set_text(CLUTTER_TEXT(priv->actorTitle), "");
		clutter_text_set_text(CLUTTER_TEXT(priv->actorDescription), "");
	}
		else
		{
			GdkPixbuf						*icon;
			const gchar						*iconName;
			gchar							*title;

			/* Determine if menu element is an item or a menu.
			 * Menus will have no description text shown
			 */
			priv->isMenu=GARCON_IS_MENU(priv->menuElement);

			/* Get and remember icon name */
			iconName=(priv->iconName && strlen(priv->iconName)>0 ?
						priv->iconName :
						garcon_menu_element_get_icon_name(priv->menuElement));

			if(iconName)
			{
				/* Get icon for icon name */
				icon=_get_pixbuf_for_icon_name(iconName, GTK_STOCK_MISSING_IMAGE);

				/* Set icon as texture */
				if(icon)
				{
					/* Scale icon */
					GdkPixbuf				*oldIcon=icon;
					
					icon=gdk_pixbuf_scale_simple(oldIcon,
													DEFAULT_ICON_SIZE, DEFAULT_ICON_SIZE,
													GDK_INTERP_BILINEAR);
					g_object_unref(oldIcon);

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
			title=g_strdup_printf("<b>%s</b>", (priv->title && strlen(priv->title)>0) ?
												priv->title :
												garcon_menu_element_get_name(priv->menuElement));
			clutter_text_set_markup(CLUTTER_TEXT(priv->actorTitle), title);
			g_free(title);

			/* Set and remember description */
			if(!priv->isMenu || priv->description)
			{
				clutter_text_set_text(CLUTTER_TEXT(priv->actorDescription),
										(priv->description && strlen(priv->description)>0) ?
											priv->description :
											garcon_menu_element_get_comment(priv->menuElement));
			}
				else
				{
					clutter_text_set_text(CLUTTER_TEXT(priv->actorDescription), "");
				}
		}

	/* Queue a redraw as the actors are now available */
	clutter_actor_queue_relayout(CLUTTER_ACTOR(self));
}

/* proxy ClickAction signals */
static void xfdashboard_application_menu_entry_clicked(ClutterClickAction *inAction,
														ClutterActor *inActor,
														gpointer inUserData)
{
	g_signal_emit(inActor, XfdashboardApplicationMenuEntrySignals[CLICKED], 0);
}

/* IMPLEMENTATION: ClutterActor */

/* Show all children of this one */
static void xfdashboard_application_menu_entry_show_all(ClutterActor *self)
{
	XfdashboardApplicationMenuEntryPrivate	*priv=XFDASHBOARD_APPLICATION_MENU_ENTRY(self)->priv;

	if(!priv->isMenu ||
		(priv->description && strlen(priv->description)>0))
	{
		clutter_actor_hide(priv->actorDescription);
	}
		else clutter_actor_show(priv->actorDescription);
	clutter_actor_show(priv->actorTitle);
	clutter_actor_show(priv->actorIcon);
	clutter_actor_show(self);
}

/* Hide all children of this one */
static void xfdashboard_application_menu_entry_hide_all(ClutterActor *self)
{
	XfdashboardApplicationMenuEntryPrivate	*priv=XFDASHBOARD_APPLICATION_MENU_ENTRY(self)->priv;

	clutter_actor_hide(self);
	clutter_actor_hide(priv->actorIcon);
	clutter_actor_hide(priv->actorTitle);
	clutter_actor_hide(priv->actorDescription);
}

/* Get preferred width/height */
static void xfdashboard_application_menu_entry_get_preferred_height(ClutterActor *self,
														gfloat inForWidth,
														gfloat *outMinHeight,
														gfloat *outNaturalHeight)
{
	XfdashboardApplicationMenuEntryPrivate	*priv=XFDASHBOARD_APPLICATION_MENU_ENTRY(self)->priv;
	gfloat									iconMinHeight, iconNaturalHeight;

	/* Get sizes */
	clutter_actor_get_preferred_height(priv->actorIcon,
										inForWidth,
										&iconMinHeight,
										&iconNaturalHeight);

	iconMinHeight+=2*priv->margin;
	iconNaturalHeight+=2*priv->margin;
	
	/* Store sizes computed */
	if(outMinHeight) *outMinHeight=iconMinHeight;
	if(outNaturalHeight) *outNaturalHeight=iconNaturalHeight;
}

static void xfdashboard_application_menu_entry_get_preferred_width(ClutterActor *self,
														gfloat inForHeight,
														gfloat *outMinWidth,
														gfloat *outNaturalWidth)
{
	XfdashboardApplicationMenuEntryPrivate	*priv=XFDASHBOARD_APPLICATION_MENU_ENTRY(self)->priv;
	gfloat									iconMinWidth, iconNaturalWidth;
	gfloat									textMinWidth, textNaturalWidth;

	clutter_actor_get_preferred_width(priv->actorIcon,
										inForHeight,
										&iconMinWidth,
										&iconNaturalWidth);

	/* Take text actors into account and add their width and spacing */
	clutter_actor_get_preferred_width(priv->actorTitle,
										inForHeight,
										&textMinWidth,
										&textNaturalWidth);
										
	if(!priv->isMenu ||
		(priv->description && strlen(priv->description)>0))
	{
		gfloat								descMinWidth, descNaturalWidth;

		clutter_actor_get_preferred_width(priv->actorDescription,
											inForHeight,
											&descMinWidth,
											&descNaturalWidth);

		textMinWidth=MAX(textMinWidth, descMinWidth);
		textNaturalWidth=MAX(textNaturalWidth, descNaturalWidth);
	}

	/* Compute sizes */
	iconMinWidth=(2*priv->margin)+iconMinWidth+textMinWidth+(textMinWidth>0 ? priv->margin : 0);
	iconNaturalWidth=(2*priv->margin)+iconNaturalWidth+textNaturalWidth+(textNaturalWidth>0 ? priv->margin : 0);
	
	/* Store sizes computed */
	if(outMinWidth) *outMinWidth=iconMinWidth;
	if(outNaturalWidth) *outNaturalWidth=iconNaturalWidth;
}

/* Allocate position and size of actor and its children*/
static void xfdashboard_application_menu_entry_allocate(ClutterActor *self,
													const ClutterActorBox *inBox,
													ClutterAllocationFlags inFlags)
{
	XfdashboardApplicationMenuEntryPrivate	*priv=XFDASHBOARD_APPLICATION_MENU_ENTRY(self)->priv;
	gfloat									left, top, width, height;
	gfloat									childWidth, childHeight;
	ClutterActorBox							*boxIcon;
	ClutterActorBox							*boxTitle;

	/* Chain up to store the allocation of the actor */
	CLUTTER_ACTOR_CLASS(xfdashboard_application_menu_entry_parent_class)->allocate(self, inBox, inFlags);

	/* Do nothing if icon actor is not available */
	if(G_UNLIKELY(!priv->actorIcon)) return;

	/* Set allocation of icon */
	clutter_actor_get_preferred_width(priv->actorIcon, -1, NULL, &width);
	clutter_actor_get_preferred_height(priv->actorIcon, -1, NULL, &height);

	left=top=priv->margin;
	boxIcon=clutter_actor_box_new(left, top, left+width, top+height);
	clutter_actor_allocate(priv->actorIcon, boxIcon, inFlags);

	/* Set allocation of title */
	clutter_actor_get_preferred_width(priv->actorTitle, -1, NULL, &childWidth);
	clutter_actor_get_preferred_height(priv->actorTitle, -1, NULL, &childHeight);

	left=boxIcon->x2+priv->margin;
	top=priv->margin;
	width=clutter_actor_box_get_width(inBox)-clutter_actor_box_get_width(boxIcon)-(3*priv->margin);
	width=MAX(MIN(childWidth, width), 0);
	height=childHeight;
	boxTitle=clutter_actor_box_new(left, top, left+width, top+height);
	clutter_actor_allocate(priv->actorTitle, boxTitle, inFlags);

	/* Set allocation of description if menu element is not a menu */
	if(!priv->isMenu ||
		(priv->description && strlen(priv->description)>0))
	{
		ClutterActorBox						*boxDescription;

		clutter_actor_get_preferred_width(priv->actorDescription, -1, NULL, &childWidth);
		clutter_actor_get_preferred_height(priv->actorDescription, -1, NULL, &childHeight);

		left=boxIcon->x2+priv->margin;
		top=boxTitle->y2+priv->textSpacing;
		width=clutter_actor_box_get_width(inBox)-clutter_actor_box_get_width(boxIcon)-(3*priv->margin);
		width=MAX(MIN(childWidth, width), 0);
		height=clutter_actor_box_get_height(inBox)-top;
		height=MAX(MIN(childHeight, height), 0);
		boxDescription=clutter_actor_box_new(left, top, left+width, top+height);
		clutter_actor_allocate(priv->actorDescription, boxDescription, inFlags);
		clutter_actor_box_free(boxDescription);
	}

	/* Release allocated resources */
	clutter_actor_box_free(boxTitle);
	clutter_actor_box_free(boxIcon);
}

/* Paint actor */
static void xfdashboard_application_menu_entry_paint(ClutterActor *self)
{
	XfdashboardApplicationMenuEntryPrivate	*priv=XFDASHBOARD_APPLICATION_MENU_ENTRY(self)->priv;
	ClutterActorBox							allocation={ 0, };
	gfloat									width, height;

	/* Get allocation to paint background */
	clutter_actor_get_allocation_box(self, &allocation);
	clutter_actor_box_get_size(&allocation, &width, &height);

	/* Start a new path */
	cogl_path_new();

	/* Set color to use when painting background */
	cogl_set_source_color4ub(priv->backgroundColor->red,
								priv->backgroundColor->green,
								priv->backgroundColor->blue,
								priv->backgroundColor->alpha);

	/* Order of actors being painted is important! */
	/* Paint background */
	cogl_path_rectangle(0, 0, width, height);
	cogl_path_fill();

	if(priv->actorIcon &&
		CLUTTER_ACTOR_IS_MAPPED(priv->actorIcon)) clutter_actor_paint(priv->actorIcon);

	if(priv->actorTitle &&
		CLUTTER_ACTOR_IS_MAPPED(priv->actorTitle)) clutter_actor_paint(priv->actorTitle);

	if(priv->actorDescription &&
		CLUTTER_ACTOR_IS_MAPPED(priv->actorDescription))
	{
		if(!priv->isMenu ||
			(priv->description && strlen(priv->description)>0))
		{
			clutter_actor_paint(priv->actorDescription);
		}
	}
}

/* Pick all the child actors */
static void xfdashboard_application_menu_entry_pick(ClutterActor *self, const ClutterColor *inColor)
{
	XfdashboardApplicationMenuEntryPrivate	*priv=XFDASHBOARD_APPLICATION_MENU_ENTRY(self)->priv;

	/* Chain up so we get a bounding box painted (if we are reactive) */
	CLUTTER_ACTOR_CLASS(xfdashboard_application_menu_entry_parent_class)->pick(self, inColor);

	/* clutter_actor_pick() might be deprecated by clutter_actor_paint().
	 * Do not know what to with ClutterColor here.
	 * Order of actors being painted is important!
	 */
	if(priv->actorIcon &&
		CLUTTER_ACTOR_IS_MAPPED(priv->actorIcon)) clutter_actor_paint(priv->actorIcon);

	if(priv->actorTitle &&
		CLUTTER_ACTOR_IS_MAPPED(priv->actorTitle)) clutter_actor_paint(priv->actorTitle);

	if(priv->actorDescription &&
		CLUTTER_ACTOR_IS_MAPPED(priv->actorDescription))
	{
		if(!priv->isMenu ||
			(priv->description && strlen(priv->description)>0))
		{
			clutter_actor_paint(priv->actorDescription);
		}
	}
}

/* Destroy this actor */
static void xfdashboard_application_menu_entry_destroy(ClutterActor *self)
{
	/* Destroy each child actor when this actor is destroyed */
	XfdashboardApplicationMenuEntryPrivate	*priv=XFDASHBOARD_APPLICATION_MENU_ENTRY(self)->priv;

	if(priv->actorIcon) clutter_actor_destroy(priv->actorIcon);
	priv->actorIcon=NULL;

	if(priv->actorTitle) clutter_actor_destroy(priv->actorTitle);
	priv->actorTitle=NULL;

	if(priv->actorDescription) clutter_actor_destroy(priv->actorDescription);
	priv->actorDescription=NULL;

	/* Call parent's class destroy method */
	if(CLUTTER_ACTOR_CLASS(xfdashboard_application_menu_entry_parent_class)->destroy)
	{
		CLUTTER_ACTOR_CLASS(xfdashboard_application_menu_entry_parent_class)->destroy(self);
	}
}

/* IMPLEMENTATION: GObject */

/* Dispose this object */
static void xfdashboard_application_menu_entry_dispose(GObject *inObject)
{
	XfdashboardApplicationMenuEntryPrivate	*priv=XFDASHBOARD_APPLICATION_MENU_ENTRY(inObject)->priv;

	/* Dispose allocated resources */
	if(priv->iconName) g_free(priv->iconName);
	priv->iconName=NULL;
	
	if(priv->title) g_free(priv->title);
	priv->title=NULL;

	if(priv->description) g_free(priv->description);
	priv->description=NULL;
	
	if(priv->titleFont) g_free(priv->titleFont);
	priv->titleFont=NULL;

	if(priv->descriptionFont) g_free(priv->descriptionFont);
	priv->descriptionFont=NULL;

	if(priv->backgroundColor) clutter_color_free(priv->backgroundColor);
	priv->backgroundColor=NULL;

	if(priv->titleColor) clutter_color_free(priv->titleColor);
	priv->titleColor=NULL;

	if(priv->descriptionColor) clutter_color_free(priv->descriptionColor);
	priv->descriptionColor=NULL;

	/* Call parent's class dispose method */
	G_OBJECT_CLASS(xfdashboard_application_menu_entry_parent_class)->dispose(inObject);
}

/* Set/get properties */
static void xfdashboard_application_menu_entry_set_property(GObject *inObject,
												guint inPropID,
												const GValue *inValue,
												GParamSpec *inSpec)
{
	XfdashboardApplicationMenuEntry			*self=XFDASHBOARD_APPLICATION_MENU_ENTRY(inObject);
	
	switch(inPropID)
	{
		case PROP_MENU_ELEMENT:
			xfdashboard_application_menu_entry_set_menu_element(self, GARCON_MENU_ELEMENT(g_value_get_object(inValue)));
			break;

		case PROP_ICON_NAME:
			_xfdashboard_application_menu_entry_set_custom_icon(self, g_value_get_string(inValue));
			break;

		case PROP_TITLE:
			_xfdashboard_application_menu_entry_set_custom_title(self, g_value_get_string(inValue));
			break;

		case PROP_DESCRIPTION:
			_xfdashboard_application_menu_entry_set_custom_description(self, g_value_get_string(inValue));
			break;

		case PROP_MARGIN:
			xfdashboard_application_menu_entry_set_margin(self, g_value_get_float(inValue));
			break;

		case PROP_TEXT_SPACING:
			xfdashboard_application_menu_entry_set_text_spacing(self, g_value_get_float(inValue));
			break;

		case PROP_BACKGROUND_COLOR:
			xfdashboard_application_menu_entry_set_background_color(self, clutter_value_get_color(inValue));
			break;

		case PROP_TITLE_FONT:
			xfdashboard_application_menu_entry_set_title_font(self, g_value_get_string(inValue));
			break;

		case PROP_TITLE_COLOR:
			xfdashboard_application_menu_entry_set_title_color(self, clutter_value_get_color(inValue));
			break;

		case PROP_TITLE_ELLIPSIZE_MODE:
			xfdashboard_application_menu_entry_set_title_ellipsize_mode(self, g_value_get_enum(inValue));
			break;

		case PROP_DESCRIPTION_FONT:
			xfdashboard_application_menu_entry_set_description_font(self, g_value_get_string(inValue));
			break;

		case PROP_DESCRIPTION_COLOR:
			xfdashboard_application_menu_entry_set_description_color(self, clutter_value_get_color(inValue));
			break;

		case PROP_DESCRIPTION_ELLIPSIZE_MODE:
			xfdashboard_application_menu_entry_set_description_ellipsize_mode(self, g_value_get_enum(inValue));
			break;

		default:
			G_OBJECT_WARN_INVALID_PROPERTY_ID(inObject, inPropID, inSpec);
			break;
	}
}

static void xfdashboard_application_menu_entry_get_property(GObject *inObject,
												guint inPropID,
												GValue *outValue,
												GParamSpec *inSpec)
{
	XfdashboardApplicationMenuEntry	*self=XFDASHBOARD_APPLICATION_MENU_ENTRY(inObject);

	switch(inPropID)
	{
		case PROP_MENU_ELEMENT:
			g_value_set_object(outValue, self->priv->menuElement);
			break;

		case PROP_ICON_NAME:
			g_value_set_string(outValue, self->priv->iconName);
			break;

		case PROP_TITLE:
			g_value_set_string(outValue, self->priv->title);
			break;

		case PROP_DESCRIPTION:
			g_value_set_string(outValue, self->priv->description);
			break;

		case PROP_MARGIN:
			g_value_set_float(outValue, self->priv->margin);
			break;

		case PROP_TEXT_SPACING:
			g_value_set_float(outValue, self->priv->textSpacing);
			break;

		case PROP_BACKGROUND_COLOR:
			clutter_value_set_color(outValue, self->priv->backgroundColor);
			break;

		case PROP_TITLE_FONT:
			g_value_set_string(outValue, self->priv->titleFont);
			break;

		case PROP_TITLE_COLOR:
			clutter_value_set_color(outValue, self->priv->titleColor);
			break;

		case PROP_TITLE_ELLIPSIZE_MODE:
			g_value_set_enum(outValue, self->priv->titleEllipsize);
			break;

		case PROP_DESCRIPTION_FONT:
			g_value_set_string(outValue, self->priv->descriptionFont);
			break;

		case PROP_DESCRIPTION_COLOR:
			clutter_value_set_color(outValue, self->priv->descriptionColor);
			break;

		case PROP_DESCRIPTION_ELLIPSIZE_MODE:
			g_value_set_enum(outValue, self->priv->descriptionEllipsize);
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
static void xfdashboard_application_menu_entry_class_init(XfdashboardApplicationMenuEntryClass *klass)
{
	ClutterActorClass	*actorClass=CLUTTER_ACTOR_CLASS(klass);
	GObjectClass		*gobjectClass=G_OBJECT_CLASS(klass);

	/* Override functions */
	gobjectClass->dispose=xfdashboard_application_menu_entry_dispose;
	gobjectClass->set_property=xfdashboard_application_menu_entry_set_property;
	gobjectClass->get_property=xfdashboard_application_menu_entry_get_property;

	actorClass->show_all=xfdashboard_application_menu_entry_show_all;
	actorClass->hide_all=xfdashboard_application_menu_entry_hide_all;
	actorClass->paint=xfdashboard_application_menu_entry_paint;
	actorClass->pick=xfdashboard_application_menu_entry_pick;
	actorClass->get_preferred_width=xfdashboard_application_menu_entry_get_preferred_width;
	actorClass->get_preferred_height=xfdashboard_application_menu_entry_get_preferred_height;
	actorClass->allocate=xfdashboard_application_menu_entry_allocate;
	actorClass->destroy=xfdashboard_application_menu_entry_destroy;

	/* Set up private structure */
	g_type_class_add_private(klass, sizeof(XfdashboardApplicationMenuEntryPrivate));

	/* Define properties */
	XfdashboardApplicationMenuEntryProperties[PROP_MENU_ELEMENT]=
		g_param_spec_object("menu-element",
							"Menu element",
							"Menu element to display",
							GARCON_TYPE_MENU_ELEMENT,
							G_PARAM_READWRITE | G_PARAM_CONSTRUCT);

	XfdashboardApplicationMenuEntryProperties[PROP_ICON_NAME]=
		g_param_spec_string("icon-name",
							"Icon name",
							"The themed name or full path to this icon",
							"",
							G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY);

	XfdashboardApplicationMenuEntryProperties[PROP_TITLE]=
		g_param_spec_string("title",
							"Title",
							"Title of this application entry",
							"",
							G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY);

	XfdashboardApplicationMenuEntryProperties[PROP_DESCRIPTION]=
		g_param_spec_string("description",
							"Description",
							"Description of this application entry",
							"",
							G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY);

	XfdashboardApplicationMenuEntryProperties[PROP_MARGIN]=
		g_param_spec_float("margin",
							"Margin",
							"Margin of actor in pixels",
							0.0f, G_MAXFLOAT,
							4.0f,
							G_PARAM_READWRITE | G_PARAM_CONSTRUCT);

	XfdashboardApplicationMenuEntryProperties[PROP_TEXT_SPACING]=
		g_param_spec_float("text-spacing",
							"Text spacing",
							"Spacing between title and description texts in pixels",
							0.0f, G_MAXFLOAT,
							4.0f,
							G_PARAM_READWRITE | G_PARAM_CONSTRUCT);

	XfdashboardApplicationMenuEntryProperties[PROP_BACKGROUND_COLOR]=
		clutter_param_spec_color("background-color",
									"Background color",
									"Background color of actor",
									&defaultBackgroundColor,
									G_PARAM_READWRITE | G_PARAM_CONSTRUCT);

	XfdashboardApplicationMenuEntryProperties[PROP_TITLE_FONT]=
		g_param_spec_string("title-font",
							"Title font",
							"Font description to use in title text",
							DEFAULT_TITLE_FONT,
							G_PARAM_READWRITE | G_PARAM_CONSTRUCT);

	XfdashboardApplicationMenuEntryProperties[PROP_TITLE_COLOR]=
		clutter_param_spec_color("title-color",
									"Title text color",
									"Text color of title",
									&defaultTitleColor,
									G_PARAM_READWRITE | G_PARAM_CONSTRUCT);

	XfdashboardApplicationMenuEntryProperties[PROP_TITLE_ELLIPSIZE_MODE]=
		g_param_spec_enum("title-ellipsize-mode",
							"Title text ellipsize mode",
							"Mode of ellipsize if text in title is too long",
							PANGO_TYPE_ELLIPSIZE_MODE,
							PANGO_ELLIPSIZE_END,
							G_PARAM_READWRITE | G_PARAM_CONSTRUCT);

	XfdashboardApplicationMenuEntryProperties[PROP_DESCRIPTION_FONT]=
		g_param_spec_string("description-font",
							"Description font",
							"Font description to use in description text",
							DEFAULT_DESC_FONT,
							G_PARAM_READWRITE | G_PARAM_CONSTRUCT);

	XfdashboardApplicationMenuEntryProperties[PROP_DESCRIPTION_COLOR]=
		clutter_param_spec_color("description-color",
									"Description text color",
									"Text color of description",
									&defaultDescriptionColor,
									G_PARAM_READWRITE | G_PARAM_CONSTRUCT);

	XfdashboardApplicationMenuEntryProperties[PROP_DESCRIPTION_ELLIPSIZE_MODE]=
		g_param_spec_enum("description-ellipsize-mode",
							"Description text ellipsize mode",
							"Mode of ellipsize if text in description is too long",
							PANGO_TYPE_ELLIPSIZE_MODE,
							PANGO_ELLIPSIZE_END,
							G_PARAM_READWRITE | G_PARAM_CONSTRUCT);

	g_object_class_install_properties(gobjectClass, PROP_LAST, XfdashboardApplicationMenuEntryProperties);

	/* Define signals */
	XfdashboardApplicationMenuEntrySignals[CLICKED]=
		g_signal_new("clicked",
						G_TYPE_FROM_CLASS(klass),
						G_SIGNAL_RUN_LAST,
						G_STRUCT_OFFSET(XfdashboardApplicationMenuEntryClass, clicked),
						NULL,
						NULL,
						g_cclosure_marshal_VOID__VOID,
						G_TYPE_NONE,
						0);

}

/* Object initialization
 * Create private structure and set up default values
 */
static void xfdashboard_application_menu_entry_init(XfdashboardApplicationMenuEntry *self)
{
	XfdashboardApplicationMenuEntryPrivate	*priv;

	priv=self->priv=XFDASHBOARD_APPLICATION_MENU_ENTRY_GET_PRIVATE(self);

	/* This actor is react on events */
	clutter_actor_set_reactive(CLUTTER_ACTOR(self), TRUE);

	/* Set up default values */
	priv->isMenu=FALSE;
	priv->menuElement=NULL;
	priv->iconName=NULL;
	priv->title=NULL;
	priv->description=NULL;
	priv->backgroundColor=NULL;
	priv->titleFont=NULL;
	priv->titleColor=NULL;
	priv->descriptionFont=NULL;
	priv->descriptionColor=NULL;

	/* Create icon actor */
	priv->actorIcon=clutter_texture_new();
	clutter_texture_set_sync_size(CLUTTER_TEXTURE(priv->actorIcon), TRUE);
	clutter_actor_set_parent(priv->actorIcon, CLUTTER_ACTOR(self));

	/* Create text actors */
	priv->actorTitle=clutter_text_new();
	clutter_text_set_single_line_mode(CLUTTER_TEXT(priv->actorTitle), TRUE);
	clutter_actor_set_parent(priv->actorTitle, CLUTTER_ACTOR(self));

	priv->actorDescription=clutter_text_new();
	clutter_text_set_single_line_mode(CLUTTER_TEXT(priv->actorDescription), FALSE);
	clutter_actor_set_parent(priv->actorDescription, CLUTTER_ACTOR(self));

	/* Connect signals */
	priv->clickAction=clutter_click_action_new();
	clutter_actor_add_action(CLUTTER_ACTOR(self), priv->clickAction);
	g_signal_connect(priv->clickAction, "clicked", G_CALLBACK(xfdashboard_application_menu_entry_clicked), NULL);
}

/* Implementation: Public API */

/* Create new actor */
ClutterActor* xfdashboard_application_menu_entry_new()
{
	return(g_object_new(XFDASHBOARD_TYPE_APPLICATION_MENU_ENTRY,
						NULL));
}

ClutterActor* xfdashboard_application_menu_entry_new_with_menu_item(const GarconMenuElement *inMenuElement)
{
	return(g_object_new(XFDASHBOARD_TYPE_APPLICATION_MENU_ENTRY,
						"menu-element", inMenuElement,
						NULL));
}

ClutterActor* xfdashboard_application_menu_entry_new_with_custom(const GarconMenuElement *inMenuElement,
																	const gchar *inIconName,
																	const gchar *inTitle,
																	const gchar *inDescription)
{
	return(g_object_new(XFDASHBOARD_TYPE_APPLICATION_MENU_ENTRY,
						"menu-element", inMenuElement,
						"icon-name", inIconName,
						"title", inTitle,
						"description", inDescription,
						NULL));
}

/* Determine if menu element is a sub-menu */
gboolean xfdashboard_application_menu_entry_is_submenu(XfdashboardApplicationMenuEntry *self)
{
	g_return_val_if_fail(XFDASHBOARD_IS_APPLICATION_MENU_ENTRY(self), FALSE);

	return(self->priv->isMenu);
}

/* Get/set menu element to display */
const GarconMenuElement* xfdashboard_application_menu_entry_get_menu_element(XfdashboardApplicationMenuEntry *self)
{
	g_return_val_if_fail(XFDASHBOARD_IS_APPLICATION_MENU_ENTRY(self), NULL);

	return(self->priv->menuElement);
}

void xfdashboard_application_menu_entry_set_menu_element(XfdashboardApplicationMenuEntry *self, const GarconMenuElement *inMenuElement)
{
	g_return_if_fail(XFDASHBOARD_IS_APPLICATION_MENU_ENTRY(self));
	g_return_if_fail(!inMenuElement || GARCON_IS_MENU_ELEMENT(inMenuElement));

	/* Set menu element */
	_xfdashboard_application_menu_entry_set_menu_item(self, inMenuElement);
}

/* Get/set margin of actor */
const gfloat xfdashboard_application_menu_entry_get_margin(XfdashboardApplicationMenuEntry *self)
{
	g_return_val_if_fail(XFDASHBOARD_IS_APPLICATION_MENU_ENTRY(self), 0);

	return(self->priv->margin);
}

void xfdashboard_application_menu_entry_set_margin(XfdashboardApplicationMenuEntry *self, const gfloat inMargin)
{
	g_return_if_fail(XFDASHBOARD_IS_APPLICATION_MENU_ENTRY(self));
	g_return_if_fail(inMargin>=0.0f);

	/* Set margin */
	XfdashboardApplicationMenuEntryPrivate	*priv=XFDASHBOARD_APPLICATION_MENU_ENTRY(self)->priv;

	if(priv->margin!=inMargin)
	{
		priv->margin=inMargin;
		clutter_actor_queue_redraw(CLUTTER_ACTOR(self));
	}
}

/* Get/set margin of actor */
const gfloat xfdashboard_application_menu_entry_get_text_spacing(XfdashboardApplicationMenuEntry *self)
{
	g_return_val_if_fail(XFDASHBOARD_IS_APPLICATION_MENU_ENTRY(self), 0);

	return(self->priv->textSpacing);
}

void xfdashboard_application_menu_entry_set_text_spacing(XfdashboardApplicationMenuEntry *self, const gfloat inSpacing)
{
	g_return_if_fail(XFDASHBOARD_IS_APPLICATION_MENU_ENTRY(self));
	g_return_if_fail(inSpacing>=0.0f);
	
	/* Set margin */
	XfdashboardApplicationMenuEntryPrivate	*priv=XFDASHBOARD_APPLICATION_MENU_ENTRY(self)->priv;

	if(priv->textSpacing!=inSpacing)
	{
		priv->textSpacing=inSpacing;
		clutter_actor_queue_redraw(CLUTTER_ACTOR(self));
	}
}

/* Get/set background color */
const ClutterColor* xfdashboard_application_menu_entry_get_background_color(XfdashboardApplicationMenuEntry *self)
{
	g_return_val_if_fail(XFDASHBOARD_IS_APPLICATION_MENU_ENTRY(self), NULL);

	return(self->priv->backgroundColor);
}

void xfdashboard_application_menu_entry_set_background_color(XfdashboardApplicationMenuEntry *self, const ClutterColor *inColor)
{
	g_return_if_fail(XFDASHBOARD_IS_APPLICATION_MENU_ENTRY(self));
	g_return_if_fail(inColor);

	/* Set background color */
	XfdashboardApplicationMenuEntryPrivate	*priv=XFDASHBOARD_APPLICATION_MENU_ENTRY(self)->priv;

	if(!priv->backgroundColor || !clutter_color_equal(inColor, priv->backgroundColor))
	{
		if(priv->backgroundColor) clutter_color_free(priv->backgroundColor);
		priv->backgroundColor=clutter_color_copy(inColor);

		clutter_actor_queue_redraw(CLUTTER_ACTOR(self));
	}
}

/* Get/set font to use in title */
const gchar* xfdashboard_application_menu_entry_get_title_font(XfdashboardApplicationMenuEntry *self)
{
	g_return_val_if_fail(XFDASHBOARD_IS_APPLICATION_MENU_ENTRY(self), NULL);

	return(self->priv->titleFont);
}

void xfdashboard_application_menu_entry_set_title_font(XfdashboardApplicationMenuEntry *self, const gchar *inFont)
{
	g_return_if_fail(XFDASHBOARD_IS_APPLICATION_MENU_ENTRY(self));
	g_return_if_fail(inFont);

	/* Set font of title */
	XfdashboardApplicationMenuEntryPrivate	*priv=XFDASHBOARD_APPLICATION_MENU_ENTRY(self)->priv;

	if(!priv->titleFont || g_strcmp0(priv->titleFont, inFont)!=0)
	{
		if(priv->titleFont) g_free(priv->titleFont);
		priv->titleFont=g_strdup(inFont);

		clutter_text_set_font_name(CLUTTER_TEXT(priv->actorTitle), priv->titleFont);
		clutter_actor_queue_redraw(CLUTTER_ACTOR(self));
	}
}

/* Get/set color of text in title */
const ClutterColor* xfdashboard_application_menu_entry_get_title_color(XfdashboardApplicationMenuEntry *self)
{
	g_return_val_if_fail(XFDASHBOARD_IS_APPLICATION_MENU_ENTRY(self), NULL);

	return(self->priv->titleColor);
}

void xfdashboard_application_menu_entry_set_title_color(XfdashboardApplicationMenuEntry *self, const ClutterColor *inColor)
{
	g_return_if_fail(XFDASHBOARD_IS_APPLICATION_MENU_ENTRY(self));
	g_return_if_fail(inColor);

	/* Set text color of title */
	XfdashboardApplicationMenuEntryPrivate	*priv=XFDASHBOARD_APPLICATION_MENU_ENTRY(self)->priv;

	if(!priv->titleColor || !clutter_color_equal(inColor, priv->titleColor))
	{
		if(priv->titleColor) clutter_color_free(priv->titleColor);
		priv->titleColor=clutter_color_copy(inColor);

		clutter_text_set_color(CLUTTER_TEXT(priv->actorTitle), priv->titleColor);
		clutter_actor_queue_redraw(CLUTTER_ACTOR(self));
	}
}

/* Get/set ellipsize mode if title text is getting too long */
const PangoEllipsizeMode xfdashboard_application_menu_entry_get_title_ellipsize_mode(XfdashboardApplicationMenuEntry *self)
{
	g_return_val_if_fail(XFDASHBOARD_IS_APPLICATION_MENU_ENTRY(self), 0);

	return(self->priv->titleEllipsize);
}

void xfdashboard_application_menu_entry_set_title_ellipsize_mode(XfdashboardApplicationMenuEntry *self, const PangoEllipsizeMode inMode)
{
	g_return_if_fail(XFDASHBOARD_IS_APPLICATION_MENU_ENTRY(self));

	/* Set ellipsize mode of description */
	XfdashboardApplicationMenuEntryPrivate	*priv=XFDASHBOARD_APPLICATION_MENU_ENTRY(self)->priv;

	if(priv->titleEllipsize!=inMode)
	{
		priv->titleEllipsize=inMode;

		clutter_text_set_ellipsize(CLUTTER_TEXT(priv->actorTitle), priv->titleEllipsize);
		clutter_actor_queue_redraw(CLUTTER_ACTOR(self));
	}
}

/* Get/set font to use in description */
const gchar* xfdashboard_application_menu_entry_get_description_font(XfdashboardApplicationMenuEntry *self)
{
	g_return_val_if_fail(XFDASHBOARD_IS_APPLICATION_MENU_ENTRY(self), NULL);

	return(self->priv->descriptionFont);
}

void xfdashboard_application_menu_entry_set_description_font(XfdashboardApplicationMenuEntry *self, const gchar *inFont)
{
	g_return_if_fail(XFDASHBOARD_IS_APPLICATION_MENU_ENTRY(self));
	g_return_if_fail(inFont);

	/* Set font of description */
	XfdashboardApplicationMenuEntryPrivate	*priv=XFDASHBOARD_APPLICATION_MENU_ENTRY(self)->priv;

	if(!priv->descriptionFont || g_strcmp0(priv->descriptionFont, inFont)!=0)
	{
		if(priv->descriptionFont) g_free(priv->descriptionFont);
		priv->descriptionFont=g_strdup(inFont);

		clutter_text_set_font_name(CLUTTER_TEXT(priv->actorDescription), priv->descriptionFont);
		clutter_actor_queue_redraw(CLUTTER_ACTOR(self));
	}
}

/* Get/set color of text in description */
const ClutterColor* xfdashboard_application_menu_entry_get_description_color(XfdashboardApplicationMenuEntry *self)
{
	g_return_val_if_fail(XFDASHBOARD_IS_APPLICATION_MENU_ENTRY(self), NULL);

	return(self->priv->descriptionColor);
}

void xfdashboard_application_menu_entry_set_description_color(XfdashboardApplicationMenuEntry *self, const ClutterColor *inColor)
{
	g_return_if_fail(XFDASHBOARD_IS_APPLICATION_MENU_ENTRY(self));
	g_return_if_fail(inColor);

	/* Set text color of description */
	XfdashboardApplicationMenuEntryPrivate	*priv=XFDASHBOARD_APPLICATION_MENU_ENTRY(self)->priv;

	if(!priv->descriptionColor || !clutter_color_equal(inColor, priv->descriptionColor))
	{
		if(priv->descriptionColor) clutter_color_free(priv->descriptionColor);
		priv->descriptionColor=clutter_color_copy(inColor);

		clutter_text_set_color(CLUTTER_TEXT(priv->actorDescription), priv->descriptionColor);
		clutter_actor_queue_redraw(CLUTTER_ACTOR(self));
	}
}

/* Get/set ellipsize mode if description text is getting too long */
const PangoEllipsizeMode xfdashboard_application_menu_entry_get_description_ellipsize_mode(XfdashboardApplicationMenuEntry *self)
{
	g_return_val_if_fail(XFDASHBOARD_IS_APPLICATION_MENU_ENTRY(self), 0);

	return(self->priv->descriptionEllipsize);
}

void xfdashboard_application_menu_entry_set_description_ellipsize_mode(XfdashboardApplicationMenuEntry *self, const PangoEllipsizeMode inMode)
{
	g_return_if_fail(XFDASHBOARD_IS_APPLICATION_MENU_ENTRY(self));

	/* Set ellipsize mode of description */
	XfdashboardApplicationMenuEntryPrivate	*priv=XFDASHBOARD_APPLICATION_MENU_ENTRY(self)->priv;

	if(priv->descriptionEllipsize!=inMode)
	{
		priv->descriptionEllipsize=inMode;

		clutter_text_set_ellipsize(CLUTTER_TEXT(priv->actorDescription), priv->descriptionEllipsize);
		clutter_actor_queue_redraw(CLUTTER_ACTOR(self));
	}
}
