/*
 * quicklaunch.c: Quicklaunch box
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

#include "common.h"
#include "quicklaunch.h"
#include "scaling-box-layout.h"

#include <xfconf/xfconf.h>
#include <math.h>

/* Define this class in GObject system */
G_DEFINE_TYPE(XfdashboardQuicklaunch,
				xfdashboard_quicklaunch,
				CLUTTER_TYPE_ACTOR)

/* Private structure - access only by public API if needed */
#define XFDASHBOARD_QUICKLAUNCH_GET_PRIVATE(obj) \
	(G_TYPE_INSTANCE_GET_PRIVATE((obj), XFDASHBOARD_TYPE_QUICKLAUNCH, XfdashboardQuicklaunchPrivate))

struct _XfdashboardQuicklaunchPrivate
{
	/* Layout manager for ClutterBox */
	ClutterLayoutManager	*layoutManager;

	/* Mark button */
	ClutterActor			*markButton;
	gchar					*markIcon;
	gchar					*markedText;
	gchar					*unmarkedText;

	/* Icons (of type XfdashboardQuicklaunchIcon) */
	GPtrArray				*desktopFiles;
	ClutterActor			*icons;
	guint					itemsCount;
	guint					maxItemsCount;
	guint					normalIconSize;

	/* Background */
	ClutterColor			*backgroundColor;

	/* Spacing between icons (is also margin to background */
	gfloat					spacing;
};

/* Properties */
enum
{
	PROP_0,

	PROP_DESKTOP_FILES,

	PROP_ICONS_COUNT,
	PROP_ICONS_MAX_COUNT,
	PROP_ICONS_NORMAL_SIZE,

	PROP_BACKGROUND_COLOR,
	PROP_SPACING,

	PROP_MARK_ICON,
	PROP_MARKED_TEXT,
	PROP_UNMARKED_TEXT,

	PROP_LAST
};

static GParamSpec* XfdashboardQuicklaunchProperties[PROP_LAST]={ 0, };

/* Signals */
enum
{
	MARKED,
	UNMARKED,

	SIGNAL_LAST
};

static guint XfdashboardQuicklaunchSignals[SIGNAL_LAST]={ 0, };

/* IMPLEMENTATION: Private variables and methods */
static ClutterColor		_xfdashboard_quicklaunch_default_background_color=
							{ 0xff, 0xff, 0xff, 0x40 };

/* Get number of icons */
static guint _xfdashboard_quicklaunch_get_number_icons(XfdashboardQuicklaunch *self)
{
	GList		*children=clutter_container_get_children(CLUTTER_CONTAINER(self->priv->icons));

	return(g_list_length(children));
}

/* "Switch to view" button was clicked */
void _xfdashboard_quicklaunch_on_mark_button_clicked(ClutterActor *inSelf, gpointer inUserData)
{
	g_return_if_fail(XFDASHBOARD_IS_QUICKLAUNCH(inSelf));
	
	XfdashboardQuicklaunch			*self=XFDASHBOARD_QUICKLAUNCH(inSelf);
	XfdashboardQuicklaunchPrivate	*priv=self->priv;
	gboolean						isMarked;

	/* Toggle mark state */
	isMarked=xfdashboard_button_get_background_visibility(XFDASHBOARD_BUTTON(priv->markButton));

	if(isMarked) xfdashboard_quicklaunch_set_mark_state(self, FALSE);
		else xfdashboard_quicklaunch_set_mark_state(self, TRUE);
}

/* Application icon was clicked */
gboolean _xfdashboard_quicklaunch_on_application_icon_clicked(ClutterActor *inActor, gpointer inUserData)
{
	g_return_val_if_fail(XFDASHBOARD_IS_APPLICATION_ICON(inActor), FALSE);
	g_return_val_if_fail(XFDASHBOARD_IS_QUICKLAUNCH(inUserData), FALSE);

	XfdashboardApplicationIcon	*icon=XFDASHBOARD_APPLICATION_ICON(inActor);
	const GAppInfo				*appInfo;
	GError						*error=NULL;

	/* Get application information object from icon */
	appInfo=xfdashboard_application_icon_get_application_info(icon);
	if(!appInfo)
	{
		g_warning("Could not launch application: NULL-application-info-object");
		return(FALSE);
	}

	/* Launch application */
	if(!g_app_info_launch(G_APP_INFO(appInfo), NULL, NULL, &error))
	{
		g_warning("Could not launch application %s",
					(error && error->message) ?  error->message : "unknown error");
		if(error) g_error_free(error);
		return(FALSE);
	}

	/* After successfully spawn new process of application (which does not
	 * indicate successful startup of application) quit application
	 */
	clutter_main_quit();

	return(TRUE);
}

/* Add an icon or button to this quicklaunch box */
gboolean _xfdashboard_quicklaunch_add_button(XfdashboardQuicklaunch *self,
												XfdashboardButton *inButton)
{
	g_return_val_if_fail(XFDASHBOARD_IS_QUICKLAUNCH(self), FALSE);
	g_return_val_if_fail(XFDASHBOARD_IS_BUTTON(inButton), FALSE);

	/* Check if quicklaunch has reached its limit and warn.
	 * But it does only make sense if max item counter was set.
	 * That means it is beyond zero.
	 */
	if(self->priv->maxItemsCount>0 &&
		self->priv->itemsCount>=self->priv->maxItemsCount)
	{
		g_warning("Quicklaunch has reached its limit of %d items. Item might not be visible!",
					self->priv->maxItemsCount);
	}
	
	/* Add and count item to quicklaunch box */
	clutter_actor_set_size(CLUTTER_ACTOR(inButton), self->priv->normalIconSize, self->priv->normalIconSize);
	clutter_container_add_actor(CLUTTER_CONTAINER(self->priv->icons), CLUTTER_ACTOR(inButton));

	self->priv->itemsCount++;

	/* Queue a relayout of all children as a new icon was added */
	clutter_actor_queue_relayout(self->priv->icons);
	
	return(TRUE);
}

gboolean _xfdashboard_quicklaunch_add_application_icon(XfdashboardQuicklaunch *self,
														XfdashboardApplicationIcon *inIcon)
{
	g_return_val_if_fail(XFDASHBOARD_IS_QUICKLAUNCH(self), FALSE);
	g_return_val_if_fail(XFDASHBOARD_IS_APPLICATION_ICON(inIcon), FALSE);

	/* Add icon to quicklaunch box and connect "clicked" signal */
	if(!_xfdashboard_quicklaunch_add_button(self, XFDASHBOARD_BUTTON(inIcon))) return(FALSE);
	g_signal_connect(inIcon, "clicked", G_CALLBACK(_xfdashboard_quicklaunch_on_application_icon_clicked), self);

	return(TRUE);
}

/* Update icons in quicklaunch */
void _xfdashboard_quicklaunch_update_icons_destroy_application_icons(ClutterActor *inIcon, gpointer inUserData)
{
	/* Only remove and destroy actor if it is an application icon */
	if(XFDASHBOARD_IS_APPLICATION_ICON(inIcon)) clutter_actor_destroy(inIcon);
}

void _xfdashboard_quicklaunch_update_icons(XfdashboardQuicklaunch *self)
{
	g_return_if_fail(XFDASHBOARD_IS_QUICKLAUNCH(self));

	XfdashboardQuicklaunchPrivate	*priv=XFDASHBOARD_QUICKLAUNCH(self)->priv;
	gint							i;
	
	/* Remove all application icons */
	clutter_container_foreach(CLUTTER_CONTAINER(priv->icons),
								CLUTTER_CALLBACK(_xfdashboard_quicklaunch_update_icons_destroy_application_icons),
								self);
	self->priv->itemsCount=0;

	/* Now re-add all application icons for current list of desktop files */
	for(i=0; i<priv->desktopFiles->len; i++)
	{
		/* Create icon from desktop file and hide label in quicklaunch */
		ClutterActor				*actor;
		GValue						*desktopFile;

		desktopFile=(GValue*)g_ptr_array_index(priv->desktopFiles, i);

		actor=xfdashboard_application_icon_new_by_desktop_file(g_value_get_string(desktopFile));
		xfdashboard_button_set_style(XFDASHBOARD_BUTTON(actor), XFDASHBOARD_STYLE_ICON);
		_xfdashboard_quicklaunch_add_application_icon(self, XFDASHBOARD_APPLICATION_ICON(actor));
	}
}

/* IMPLEMENTATION: ClutterActor */

/* Paint actor */
static void xfdashboard_quicklaunch_paint(ClutterActor *self)
{
	XfdashboardQuicklaunchPrivate	*priv=XFDASHBOARD_QUICKLAUNCH(self)->priv;
	ClutterActorBox					allocation={ 0, };
	gfloat							width, height;

	if(priv->icons && CLUTTER_ACTOR_IS_MAPPED(priv->icons))
	{
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

		/* Paint background */
		cogl_path_move_to(0, 0);
		cogl_path_line_to(width-priv->spacing, 0);
		cogl_path_arc(width-priv->spacing, priv->spacing,
						priv->spacing, priv->spacing,
						-90, 0);
		cogl_path_line_to(width, height-priv->spacing-1);
		cogl_path_arc(width-priv->spacing, height-priv->spacing-1,
						priv->spacing, priv->spacing,
						0, 90);
		cogl_path_line_to(0, height-1);
		cogl_path_close();
		cogl_path_fill();

		/* Paint icons */
		clutter_actor_paint(priv->icons);
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
static void xfdashboard_quicklaunch_pick(ClutterActor *self, const ClutterColor *inColor)
{
	XfdashboardQuicklaunchPrivate	*priv=XFDASHBOARD_QUICKLAUNCH(self)->priv;

	/* It is possible to avoid a costly paint by checking
	 * whether the actor should really be painted in pick mode
	 */
	if(!clutter_actor_should_pick_paint(self)) return;

	/* Chain up so we get a bounding box painted (if we are reactive) */
	CLUTTER_ACTOR_CLASS(xfdashboard_quicklaunch_parent_class)->pick(self, inColor);

	/* Draw silouhette of icons */
	if(priv->icons &&
		CLUTTER_ACTOR_IS_MAPPED(priv->icons))
	{
		clutter_actor_paint(priv->icons);
	}
}

/* Get preferred width/height */
static void xfdashboard_quicklaunch_get_preferred_width(ClutterActor *self,
														gfloat inForHeight,
														gfloat *outMinWidth,
														gfloat *outNaturalWidth)
{
	XfdashboardQuicklaunchPrivate	*priv=XFDASHBOARD_QUICKLAUNCH(self)->priv;
	gfloat							minWidth, naturalWidth;

	/* Get minimum and natural size of icons */
	minWidth=naturalWidth=0.0f;
	if(priv->icons && CLUTTER_ACTOR_IS_VISIBLE(priv->icons))
	{
		clutter_actor_get_preferred_width(priv->icons,
											inForHeight,
											&minWidth, &naturalWidth);
	}

	/* Now add spacing to determined minimum and natural size */
	minWidth+=(2*priv->spacing);
	naturalWidth+=(2*priv->spacing);

	/* Return sizes */
	if(outMinWidth) *outMinWidth=minWidth;
	if(outNaturalWidth) *outNaturalWidth=naturalWidth;
}

static void xfdashboard_quicklaunch_get_preferred_height(ClutterActor *self,
															gfloat inForWidth,
															gfloat *outMinHeight,
															gfloat *outNaturalHeight)
{
	XfdashboardQuicklaunchPrivate	*priv=XFDASHBOARD_QUICKLAUNCH(self)->priv;
	gfloat							minHeight, naturalHeight;

	/* Get minimum and natural size of icons */
	minHeight=naturalHeight=0.0f;
	if(priv->icons && CLUTTER_ACTOR_IS_VISIBLE(priv->icons))
	{
		clutter_actor_get_preferred_height(priv->icons,
											inForWidth,
											&minHeight, &naturalHeight);
	}

	/* Now add spacing to determined minimum and natural size */
	minHeight+=(2*priv->spacing);
	naturalHeight+=(2*priv->spacing);

	/* Return sizes */
	if(outMinHeight) *outMinHeight=minHeight;
	if(outNaturalHeight) *outNaturalHeight=naturalHeight;
}

/* Allocate position and size of actor and its children*/
static void xfdashboard_quicklaunch_allocate(ClutterActor *self,
												const ClutterActorBox *inBox,
												ClutterAllocationFlags inFlags)
{
	XfdashboardQuicklaunchPrivate	*priv=XFDASHBOARD_QUICKLAUNCH(self)->priv;
	ClutterActorBox					box={ 0, };
	ClutterActorBox					boxIcons={ 0, };
	gfloat							availableWidth, availableHeight;

	/* Get available size */
	clutter_actor_box_get_size(inBox, &availableWidth, &availableHeight);

	/* Allocate icons */
	if(priv->icons)
	{
		gfloat						availIconWidth, availIconHeight;
		gfloat						iconsMinScale;
		const ClutterActorBox		*boxIconsReal;
		XfdashboardScalingBoxLayout	*layout=XFDASHBOARD_SCALING_BOX_LAYOUT(priv->layoutManager);

		/* Update number of icons for later calculation */
		priv->itemsCount=_xfdashboard_quicklaunch_get_number_icons(XFDASHBOARD_QUICKLAUNCH(self));
		
		/* Get available space for icons */
		availIconWidth=availableWidth-(2*priv->spacing);
		availIconHeight=availableHeight-(2*priv->spacing);

		/* Set allocation for icons */
		clutter_actor_box_set_origin(&boxIcons, priv->spacing, priv->spacing);
		clutter_actor_box_set_size(&boxIcons, availIconWidth, availIconHeight);
		clutter_actor_allocate(priv->icons, &boxIcons, inFlags);

		/* Get allocated size of icons and set real allocation size for
		 * icons because one direction might be too large and should be
		 * shrinked
		 */
		boxIconsReal=xfdashboard_scaling_box_layout_get_last_allocation(layout);
		clutter_actor_box_set_size(&boxIcons,
									clutter_actor_box_get_width(boxIconsReal),
									clutter_actor_box_get_height(boxIconsReal));
		clutter_actor_allocate(priv->icons, &boxIcons, inFlags);

		/* Update available size */
		availableWidth=clutter_actor_box_get_width(&boxIcons)+(2*priv->spacing);

		/* Determine maximum number of icons at lowest scale */
		iconsMinScale=xfdashboard_scaling_box_layout_get_scale_minimum(layout);
		priv->maxItemsCount=floor(availIconHeight/((priv->normalIconSize*iconsMinScale)+priv->spacing));
	}

	/* Create new allocation about available size and
	 * chain up to store the allocation of the actor
	 */
	clutter_actor_box_set_origin(&box, 0, 0);
	clutter_actor_box_set_size(&box, availableWidth, availableHeight);
	CLUTTER_ACTOR_CLASS(xfdashboard_quicklaunch_parent_class)->allocate(self, &box, inFlags);
}

/* Destroy this actor */
static void xfdashboard_quicklaunch_destroy(ClutterActor *self)
{
	XfdashboardQuicklaunchPrivate		*priv=XFDASHBOARD_QUICKLAUNCH(self)->priv;

	/* Destroy background and all our children */
	if(priv->icons)
	{
		clutter_container_foreach(CLUTTER_CONTAINER(priv->icons), CLUTTER_CALLBACK(clutter_actor_destroy), self);
		clutter_actor_destroy(priv->icons);
		priv->icons=NULL;
	}

	/* Call parent's class destroy method */
	if(CLUTTER_ACTOR_CLASS(xfdashboard_quicklaunch_parent_class)->destroy)
	{
		CLUTTER_ACTOR_CLASS(xfdashboard_quicklaunch_parent_class)->destroy(self);
	}
}

/* IMPLEMENTATION: GObject */

/* Dispose this object */
static void xfdashboard_quicklaunch_dispose(GObject *inObject)
{
	XfdashboardQuicklaunchPrivate	*priv=XFDASHBOARD_QUICKLAUNCH(inObject)->priv;

	/* Release allocated resources */
	if(priv->desktopFiles) xfconf_array_free(priv->desktopFiles);
	priv->desktopFiles=NULL;

	if(priv->backgroundColor) clutter_color_free(priv->backgroundColor);
	priv->backgroundColor=NULL;

	if(priv->markedText) g_free(priv->markedText);
	priv->markedText=NULL;

	if(priv->unmarkedText) g_free(priv->unmarkedText);
	priv->unmarkedText=NULL;

	if(priv->markIcon) g_free(priv->markIcon);
	priv->markIcon=NULL;

	/* Call parent's class dispose method */
	G_OBJECT_CLASS(xfdashboard_quicklaunch_parent_class)->dispose(inObject);
}

/* Set/get properties */
static void xfdashboard_quicklaunch_set_property(GObject *inObject,
													guint inPropID,
													const GValue *inValue,
													GParamSpec *inSpec)
{
	XfdashboardQuicklaunch			*self=XFDASHBOARD_QUICKLAUNCH(inObject);
	XfdashboardQuicklaunchPrivate	*priv=self->priv;

	switch(inPropID)
	{
		case PROP_DESKTOP_FILES:
		{
			GPtrArray				*desktopFiles;
			gint					i;
			GValue					*element;
			GValue					*desktopFile;

			/* Free current list of desktop files */
			if(priv->desktopFiles) xfconf_array_free(priv->desktopFiles);
			priv->desktopFiles=NULL;

			/* Copy array of string pointing to desktop files */
			desktopFiles=g_value_get_boxed(inValue);
			if(desktopFiles)
			{
				priv->desktopFiles=g_ptr_array_sized_new(desktopFiles->len);
				for(i=0; i<desktopFiles->len; ++i)
				{
					element=(GValue*)g_ptr_array_index(desktopFiles, i);

					/* Filter string value types */
					if(G_VALUE_HOLDS_STRING(element))
					{
						desktopFile=g_value_init(g_new0(GValue, 1), G_TYPE_STRING);
						g_value_copy(element, desktopFile);
						g_ptr_array_add(priv->desktopFiles, desktopFile);
					}
				}
			}
				else priv->desktopFiles=g_ptr_array_new();

			/* Update list of icons for desktop files */
			_xfdashboard_quicklaunch_update_icons(self);
			break;
		}

		case PROP_ICONS_NORMAL_SIZE:
			xfdashboard_quicklaunch_set_normal_icon_size(self, g_value_get_uint(inValue));
			break;

		case PROP_BACKGROUND_COLOR:
			xfdashboard_quicklaunch_set_background_color(self, clutter_value_get_color(inValue));
			break;

		case PROP_SPACING:
			xfdashboard_quicklaunch_set_spacing(self, g_value_get_float(inValue));
			break;

		case PROP_MARK_ICON:
			xfdashboard_quicklaunch_set_mark_icon(self, g_value_get_string(inValue));
			break;

		case PROP_MARKED_TEXT:
			xfdashboard_quicklaunch_set_marked_text(self, g_value_get_string(inValue));
			break;

		case PROP_UNMARKED_TEXT:
			xfdashboard_quicklaunch_set_unmarked_text(self, g_value_get_string(inValue));
			break;

		default:
			G_OBJECT_WARN_INVALID_PROPERTY_ID(inObject, inPropID, inSpec);
			break;
	}
}

static void xfdashboard_quicklaunch_get_property(GObject *inObject,
													guint inPropID,
													GValue *outValue,
													GParamSpec *inSpec)
{
	XfdashboardQuicklaunchPrivate	*priv=XFDASHBOARD_QUICKLAUNCH(inObject)->priv;

	switch(inPropID)
	{
		case PROP_DESKTOP_FILES:
			g_value_set_boxed(outValue, priv->desktopFiles);
			break;

		case PROP_ICONS_COUNT:
			g_value_set_uint(outValue, priv->itemsCount);
			break;

		case PROP_ICONS_MAX_COUNT:
			g_value_set_uint(outValue, priv->maxItemsCount);
			break;

		case PROP_ICONS_NORMAL_SIZE:
			g_value_set_uint(outValue, priv->normalIconSize);
			break;

		case PROP_BACKGROUND_COLOR:
			clutter_value_set_color(outValue, priv->backgroundColor);
			break;

		case PROP_SPACING:
			g_value_set_float(outValue, priv->spacing);
			break;

		case PROP_MARK_ICON:
			g_value_set_string(outValue, priv->markIcon);
			break;

		case PROP_MARKED_TEXT:
			g_value_set_string(outValue, priv->markedText);
			break;

		case PROP_UNMARKED_TEXT:
			g_value_set_string(outValue, priv->unmarkedText);
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
static void xfdashboard_quicklaunch_class_init(XfdashboardQuicklaunchClass *klass)
{
	GObjectClass			*gobjectClass=G_OBJECT_CLASS(klass);
	ClutterActorClass		*actorClass=CLUTTER_ACTOR_CLASS(klass);

	/* Override functions */
	actorClass->paint=xfdashboard_quicklaunch_paint;
	actorClass->pick=xfdashboard_quicklaunch_pick;
	actorClass->get_preferred_width=xfdashboard_quicklaunch_get_preferred_width;
	actorClass->get_preferred_height=xfdashboard_quicklaunch_get_preferred_height;
	actorClass->allocate=xfdashboard_quicklaunch_allocate;
	actorClass->destroy=xfdashboard_quicklaunch_destroy;

	gobjectClass->set_property=xfdashboard_quicklaunch_set_property;
	gobjectClass->get_property=xfdashboard_quicklaunch_get_property;
	gobjectClass->dispose=xfdashboard_quicklaunch_dispose;

	/* Set up private structure */
	g_type_class_add_private(klass, sizeof(XfdashboardQuicklaunchPrivate));

	/* Define properties */
	XfdashboardQuicklaunchProperties[PROP_DESKTOP_FILES]=
		g_param_spec_boxed("desktop-files",
							"Desktop files",
							"An array of strings pointing to desktop files shown as icons in quicklaunch",
							XFDASHBOARD_TYPE_VALUE_ARRAY,
							G_PARAM_READWRITE);

	XfdashboardQuicklaunchProperties[PROP_ICONS_COUNT]=
		g_param_spec_uint("icons-count",
							"Number of icons",
							"Number of icons in quicklaunch",
							0,
							G_MAXUINT,
							0,
							G_PARAM_READABLE);

	XfdashboardQuicklaunchProperties[PROP_ICONS_MAX_COUNT]=
		g_param_spec_uint("icons-max-count",
							"Maximum number of icons",
							"Maximum number of icons the quicklaunch box can hold at current scale",
							0,
							G_MAXUINT,
							0,
							G_PARAM_READABLE);

	XfdashboardQuicklaunchProperties[PROP_ICONS_NORMAL_SIZE]=
		g_param_spec_uint("icons-normal-size",
							"Normal size of an icon",
							"The normal size in pixels of a single icon in quicklaunch at maximum scale factor",
							0,
							G_MAXUINT,
							64,
							G_PARAM_READWRITE | G_PARAM_CONSTRUCT);

	XfdashboardQuicklaunchProperties[PROP_BACKGROUND_COLOR]=
		clutter_param_spec_color("background-color",
									"Background color",
									"Background color of quicklaunch",
									&_xfdashboard_quicklaunch_default_background_color,
									G_PARAM_READWRITE | G_PARAM_CONSTRUCT);

	XfdashboardQuicklaunchProperties[PROP_SPACING]=
		g_param_spec_float("spacing",
							"Spacing",
							"Spacing in pixels between all elements in quicklaunch",
							0.0,
							G_MAXFLOAT,
							8.0,
							G_PARAM_READWRITE | G_PARAM_CONSTRUCT);

	XfdashboardQuicklaunchProperties[PROP_MARK_ICON]=
		g_param_spec_string("mark-icon",
							"Mark button icon",
							"Icon to show in mark button",
							NULL,
							G_PARAM_READWRITE);

	XfdashboardQuicklaunchProperties[PROP_MARKED_TEXT]=
		g_param_spec_string("marked-text",
							"Marked text",
							"Text for marked button",
							NULL,
							G_PARAM_READWRITE);

	XfdashboardQuicklaunchProperties[PROP_UNMARKED_TEXT]=
		g_param_spec_string("unmarked-text",
							"Unmarked text",
							"Text for unmarked button",
							NULL,
							G_PARAM_READWRITE);

	g_object_class_install_properties(gobjectClass, PROP_LAST, XfdashboardQuicklaunchProperties);

	/* Define signals */
	XfdashboardQuicklaunchSignals[MARKED]=
		g_signal_new("marked",
						G_TYPE_FROM_CLASS(klass),
						G_SIGNAL_RUN_LAST,
						G_STRUCT_OFFSET(XfdashboardQuicklaunchClass, view_show),
						NULL,
						NULL,
						g_cclosure_marshal_VOID__VOID,
						G_TYPE_NONE,
						0);

	XfdashboardQuicklaunchSignals[UNMARKED]=
		g_signal_new("unmarked",
						G_TYPE_FROM_CLASS(klass),
						G_SIGNAL_RUN_LAST,
						G_STRUCT_OFFSET(XfdashboardQuicklaunchClass, view_show),
						NULL,
						NULL,
						g_cclosure_marshal_VOID__VOID,
						G_TYPE_NONE,
						0);
}

/* Object initialization
 * Create private structure and set up default values
 */
static void xfdashboard_quicklaunch_init(XfdashboardQuicklaunch *self)
{
	XfdashboardQuicklaunchPrivate	*priv;

	/* Set up default values */
	priv=self->priv=XFDASHBOARD_QUICKLAUNCH_GET_PRIVATE(self);

	priv->desktopFiles=g_ptr_array_new();
	priv->itemsCount=0;
	priv->maxItemsCount=0;
	priv->normalIconSize=64;
	priv->spacing=0.0f;
	priv->backgroundColor=NULL;

	priv->markedText=NULL;
	priv->unmarkedText=NULL;
	priv->markIcon=NULL;

	/* Set up this actor */
	clutter_actor_set_reactive(CLUTTER_ACTOR(self), TRUE);

	/* Set up ClutterBox to hold quicklaunch icons */
	priv->layoutManager=xfdashboard_scaling_box_layout_new();
	xfdashboard_scaling_box_layout_set_spacing(XFDASHBOARD_SCALING_BOX_LAYOUT(priv->layoutManager),
												priv->spacing);

	priv->icons=clutter_box_new(priv->layoutManager);
	clutter_actor_set_parent(priv->icons, CLUTTER_ACTOR(self));

	/* Add mark button as first icon to box */
	priv->markButton=xfdashboard_button_new_with_text(priv->unmarkedText);
	xfdashboard_button_set_icon_orientation(XFDASHBOARD_BUTTON(priv->markButton), XFDASHBOARD_ORIENTATION_TOP);
	xfdashboard_button_set_style(XFDASHBOARD_BUTTON(priv->markButton), XFDASHBOARD_STYLE_ICON);
	xfdashboard_button_set_sync_icon_size(XFDASHBOARD_BUTTON(priv->markButton), FALSE);
	xfdashboard_button_set_background_visibility(XFDASHBOARD_BUTTON(priv->markButton), FALSE);
	_xfdashboard_quicklaunch_add_button(self, XFDASHBOARD_BUTTON(priv->markButton));
	g_signal_connect_swapped(priv->markButton, "clicked", G_CALLBACK(_xfdashboard_quicklaunch_on_mark_button_clicked), self);
}

/* Implementation: Public API */

/* Create new actor */
ClutterActor* xfdashboard_quicklaunch_new(void)
{
	return(g_object_new(XFDASHBOARD_TYPE_QUICKLAUNCH, NULL));
}

/* Get number of icons in quicklaunch */
guint xfdashboard_quicklaunch_get_icon_count(XfdashboardQuicklaunch *self)
{
	g_return_val_if_fail(XFDASHBOARD_IS_QUICKLAUNCH(self), 0);

	return(self->priv->itemsCount);
}

/* Get maximum number of icons quicklaunch can hold at smallest scale */
guint xfdashboard_quicklaunch_get_max_icon_count(XfdashboardQuicklaunch *self)
{
	g_return_val_if_fail(XFDASHBOARD_IS_QUICKLAUNCH(self), 0);

	return(self->priv->maxItemsCount);
}

/* Get/set icons' normal size */
guint xfdashboard_quicklaunch_get_normal_icon_size(XfdashboardQuicklaunch *self)
{
	g_return_val_if_fail(XFDASHBOARD_IS_QUICKLAUNCH(self), 0);

	return(self->priv->normalIconSize);
}

void xfdashboard_quicklaunch_set_normal_icon_size(XfdashboardQuicklaunch *self, guint inSize)
{
	g_return_if_fail(XFDASHBOARD_IS_QUICKLAUNCH(self));
	g_return_if_fail(inSize>0);

	/* Set normal icon size (defines size at scale of 1.0) */
	XfdashboardQuicklaunchPrivate	*priv=XFDASHBOARD_QUICKLAUNCH(self)->priv;

	if(priv->normalIconSize!=inSize)
	{
		priv->normalIconSize=inSize;
		clutter_actor_queue_redraw(CLUTTER_ACTOR(self));
	}
}

/* Get/set color of background */
const ClutterColor* xfdashboard_quicklaunch_get_background_color(XfdashboardQuicklaunch *self)
{
	g_return_val_if_fail(XFDASHBOARD_IS_QUICKLAUNCH(self), NULL);

	return(self->priv->backgroundColor);
}

void xfdashboard_quicklaunch_set_background_color(XfdashboardQuicklaunch *self, const ClutterColor *inColor)
{
	g_return_if_fail(XFDASHBOARD_IS_QUICKLAUNCH(self));
	g_return_if_fail(inColor);

	/* Set background color */
	XfdashboardQuicklaunchPrivate	*priv=XFDASHBOARD_QUICKLAUNCH(self)->priv;

	if(!priv->backgroundColor || !clutter_color_equal(inColor, priv->backgroundColor))
	{
		if(priv->backgroundColor) clutter_color_free(priv->backgroundColor);
		priv->backgroundColor=clutter_color_copy(inColor);

		clutter_actor_queue_redraw(CLUTTER_ACTOR(self));
	}
}

/* Get/set spacing */
gfloat xfdashboard_quicklaunch_get_spacing(XfdashboardQuicklaunch *self)
{
	g_return_val_if_fail(XFDASHBOARD_IS_QUICKLAUNCH(self), 0);

	XfdashboardQuicklaunchPrivate	*priv=XFDASHBOARD_QUICKLAUNCH(self)->priv;
	XfdashboardScalingBoxLayout		*layout=XFDASHBOARD_SCALING_BOX_LAYOUT(priv->layoutManager);

	return(xfdashboard_scaling_box_layout_get_spacing(layout));
}

void xfdashboard_quicklaunch_set_spacing(XfdashboardQuicklaunch *self, gfloat inSpacing)
{
	g_return_if_fail(XFDASHBOARD_IS_QUICKLAUNCH(self));
	g_return_if_fail(inSpacing>=0.0f);

	/* Set spacing */
	XfdashboardQuicklaunchPrivate	*priv=XFDASHBOARD_QUICKLAUNCH(self)->priv;
	XfdashboardScalingBoxLayout		*layout=XFDASHBOARD_SCALING_BOX_LAYOUT(priv->layoutManager);

	if(priv->spacing!=inSpacing)
	{
		priv->spacing=inSpacing;

		xfdashboard_scaling_box_layout_set_spacing(layout, priv->spacing);
		clutter_actor_queue_redraw(CLUTTER_ACTOR(self));
	}
}

/* Get/set text for marked button */
const gchar* xfdashboard_quicklaunch_get_marked_text(XfdashboardQuicklaunch *self)
{
	g_return_val_if_fail(XFDASHBOARD_IS_QUICKLAUNCH(self), NULL);

	return(self->priv->markedText);
}

void xfdashboard_quicklaunch_set_marked_text(XfdashboardQuicklaunch *self, const gchar *inText)
{
	g_return_if_fail(XFDASHBOARD_IS_QUICKLAUNCH(self));

	/* Check if mark has changed */
	XfdashboardQuicklaunchPrivate	*priv=XFDASHBOARD_QUICKLAUNCH(self)->priv;
	gboolean						isMarked;

	if(g_strcmp0(inText, priv->markedText)==0) return;

	/* Get mark state */
	isMarked=xfdashboard_button_get_background_visibility(XFDASHBOARD_BUTTON(priv->markButton));

	/* Store new text */
	if(priv->markedText) g_free(priv->markedText);
	priv->markedText=g_strdup(inText);
	
	/* Set new text if button is in right mark state */
	if(isMarked) xfdashboard_button_set_text(XFDASHBOARD_BUTTON(priv->markButton), priv->markedText);
}

/* Get/set text for unmarked button */
const gchar* xfdashboard_quicklaunch_get_unmarked_text(XfdashboardQuicklaunch *self)
{
	g_return_val_if_fail(XFDASHBOARD_IS_QUICKLAUNCH(self), NULL);

	return(self->priv->unmarkedText);
}

void xfdashboard_quicklaunch_set_unmarked_text(XfdashboardQuicklaunch *self, const gchar *inText)
{
	g_return_if_fail(XFDASHBOARD_IS_QUICKLAUNCH(self));

	/* Check if mark has changed */
	XfdashboardQuicklaunchPrivate	*priv=XFDASHBOARD_QUICKLAUNCH(self)->priv;
	gboolean						isMarked;

	if(g_strcmp0(inText, priv->unmarkedText)==0) return;

	/* Get mark state */
	isMarked=xfdashboard_button_get_background_visibility(XFDASHBOARD_BUTTON(priv->markButton));

	/* Store new text */
	if(priv->unmarkedText) g_free(priv->unmarkedText);
	priv->unmarkedText=g_strdup(inText);
	
	/* Set new text if button is in right mark state */
	if(!isMarked) xfdashboard_button_set_text(XFDASHBOARD_BUTTON(priv->markButton), priv->unmarkedText);
}

/* Get/set icon for "mark button" */
const gchar* xfdashboard_quicklaunch_get_mark_icon(XfdashboardQuicklaunch *self)
{
	g_return_val_if_fail(XFDASHBOARD_IS_QUICKLAUNCH(self), NULL);

	return(self->priv->unmarkedText);
}

void xfdashboard_quicklaunch_set_mark_icon(XfdashboardQuicklaunch *self, const gchar *inIcon)
{
	g_return_if_fail(XFDASHBOARD_IS_QUICKLAUNCH(self));

	XfdashboardQuicklaunchPrivate	*priv=XFDASHBOARD_QUICKLAUNCH(self)->priv;

	/* Check if icon changes */
	if(g_strcmp0(inIcon, priv->markIcon)==0) return;

	/* Change icon */
	xfdashboard_button_set_icon(XFDASHBOARD_BUTTON(priv->markButton), inIcon);
}

/* Mark or unmark "mark button" */
void xfdashboard_quicklaunch_set_mark_state(XfdashboardQuicklaunch *self, gboolean inIsMarked)
{
	g_return_if_fail(XFDASHBOARD_IS_QUICKLAUNCH(self));

	XfdashboardQuicklaunchPrivate	*priv=XFDASHBOARD_QUICKLAUNCH(self)->priv;

	/* Check if mark changes */
	gboolean						isMarked=xfdashboard_button_get_background_visibility(XFDASHBOARD_BUTTON(priv->markButton));

	if(inIsMarked==isMarked) return;

	/* Emit signal "show" or "hide" of "view button" and set mark */
	if(inIsMarked)
	{
		g_signal_emit(self, XfdashboardQuicklaunchSignals[MARKED], 0);
		xfdashboard_button_set_background_visibility(XFDASHBOARD_BUTTON(priv->markButton), TRUE);
		xfdashboard_button_set_text(XFDASHBOARD_BUTTON(priv->markButton), priv->markedText);
	}
		else
		{
			g_signal_emit(self, XfdashboardQuicklaunchSignals[UNMARKED], 0);
			xfdashboard_button_set_background_visibility(XFDASHBOARD_BUTTON(priv->markButton), FALSE);
			xfdashboard_button_set_text(XFDASHBOARD_BUTTON(priv->markButton), priv->unmarkedText);
		}
}
