/*
 * quicklaunch: Quicklaunch box
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

#include "quicklaunch.h"
#include "enums.h"
#include "application.h"
#include "application-button.h"

#include <glib/gi18n-lib.h>
#include <math.h>

/* Define this class in GObject system */
G_DEFINE_TYPE(XfdashboardQuicklaunch,
				xfdashboard_quicklaunch,
				XFDASHBOARD_TYPE_BACKGROUND)
                                                
/* Private structure - access only by public API if needed */
#define XFDASHBOARD_QUICKLAUNCH_GET_PRIVATE(obj) \
	(G_TYPE_INSTANCE_GET_PRIVATE((obj), XFDASHBOARD_TYPE_QUICKLAUNCH, XfdashboardQuicklaunchPrivate))

struct _XfdashboardQuicklaunchPrivate
{
	/* Properties related */
	GPtrArray				*favourites;

	gfloat					scaleMin;
	gfloat					scaleMax;
	gfloat					scaleStep;

	gfloat					spacing;

	/* Instance related */
	XfconfChannel			*xfconfChannel;
	gfloat					scaleCurrent;
};

/* Properties */
enum
{
	PROP_0,

	PROP_FAVOURITES,
	PROP_SPACING,

	PROP_LAST
};

static GParamSpec* XfdashboardQuicklaunchProperties[PROP_LAST]={ 0, };

/* Signals */
enum
{
	SIGNAL_LAST
};

static guint XfdashboardQuicklaunchSignals[SIGNAL_LAST]={ 0, };

/* IMPLEMENTATION: Private variables and methods */
#define DEFAULT_SCALE_MIN		0.1
#define DEFAULT_SCALE_MAX		1.0
#define DEFAULT_SCALE_STEP		0.1

/* Update icons in quicklaunch */
void _xfdashboard_quicklaunch_on_application_button_clicked(XfdashboardQuicklaunch *self, gpointer inUserData)
{
	g_return_if_fail(XFDASHBOARD_IS_QUICKLAUNCH(self));
	g_return_if_fail(XFDASHBOARD_IS_APPLICATION_BUTTON(inUserData));

	XfdashboardApplicationButton		*button=XFDASHBOARD_APPLICATION_BUTTON(inUserData);

	/* Launch application */
	if(xfdashboard_application_button_execute(button))
	{
		/* Launching application seems to be successfuly so quit application */
		xfdashboard_application_quit();
		return;
	}
}

void _xfdashboard_quicklaunch_update_icons(XfdashboardQuicklaunch *self)
{
	g_return_if_fail(XFDASHBOARD_IS_QUICKLAUNCH(self));

	XfdashboardQuicklaunchPrivate	*priv=XFDASHBOARD_QUICKLAUNCH(self)->priv;
	ClutterActor					*child;
	ClutterActorIter				iter;
	gint							i;

	/* Remove all application buttons */
	clutter_actor_iter_init(&iter, CLUTTER_ACTOR(self));
	while(clutter_actor_iter_next(&iter, &child))
	{
		if(XFDASHBOARD_IS_APPLICATION_BUTTON(child)) clutter_actor_iter_destroy(&iter);
	}

	/* Now re-add all application icons for current favourites */
	for(i=0; i<priv->favourites->len; i++)
	{
		/* Create application button from desktop file and hide label in quicklaunch */
		ClutterActor					*actor;
		GValue							*desktopFile;

		desktopFile=(GValue*)g_ptr_array_index(priv->favourites, i);

		actor=xfdashboard_application_button_new_from_desktop_file(g_value_get_string(desktopFile));
		xfdashboard_button_set_icon_size(XFDASHBOARD_BUTTON(actor), 64);
		xfdashboard_button_set_sync_icon_size(XFDASHBOARD_BUTTON(actor), FALSE);
		xfdashboard_button_set_style(XFDASHBOARD_BUTTON(actor), XFDASHBOARD_STYLE_ICON);
		clutter_actor_show(actor);
		clutter_actor_add_child(CLUTTER_ACTOR(self), actor);
		g_signal_connect_swapped(actor, "clicked", G_CALLBACK(_xfdashboard_quicklaunch_on_application_button_clicked), self);
	}
}

/* Set up favourites array from string array value */
void _xfdashboard_quicklaunch_set_favourites(XfdashboardQuicklaunch *self, const GValue *inValue)
{
	g_return_if_fail(XFDASHBOARD_IS_QUICKLAUNCH(self));
	g_return_if_fail(G_IS_VALUE(inValue));

	XfdashboardQuicklaunchPrivate	*priv=self->priv;
	GPtrArray						*desktopFiles;
	gint							i;
	GValue							*element;
	GValue							*desktopFile;

	/* Free current list of favourites */
	if(priv->favourites) xfconf_array_free(priv->favourites);
	priv->favourites=NULL;

	/* Copy array of string pointing to desktop files */
	desktopFiles=g_value_get_boxed(inValue);
	if(desktopFiles)
	{
		priv->favourites=g_ptr_array_sized_new(desktopFiles->len);
		for(i=0; i<desktopFiles->len; ++i)
		{
			element=(GValue*)g_ptr_array_index(desktopFiles, i);

			/* Filter string value types */
			if(G_VALUE_HOLDS_STRING(element))
			{
				desktopFile=g_value_init(g_new0(GValue, 1), G_TYPE_STRING);
				g_value_copy(element, desktopFile);
				g_ptr_array_add(priv->favourites, desktopFile);
			}
		}
	}
		else priv->favourites=g_ptr_array_new();

	/* Update list of icons for desktop files */
	_xfdashboard_quicklaunch_update_icons(self);
}

/* Get scale factor to fit all children into given height */
gfloat _xfdashboard_quicklaunch_get_scale_for_height(XfdashboardQuicklaunch *self,
														gfloat inForHeight,
														gboolean inDoMinimumSize)
{
	g_return_val_if_fail(XFDASHBOARD_IS_QUICKLAUNCH(self), 0.0f);
	g_return_val_if_fail(inForHeight, 0.0f);

	XfdashboardQuicklaunchPrivate	*priv=self->priv;
	ClutterActor					*child;
	ClutterActorIter				iter;
	gint							numberChildren;
	gfloat							totalHeight, scalableHeight;
	gfloat							childHeight;
	gfloat							childMinHeight, childNaturalHeight;
	gfloat							scale;
	gboolean						recheckHeight;

	/* Count visible children and determine their total height */
	numberChildren=0;
	totalHeight=0.0f;
	clutter_actor_iter_init(&iter, CLUTTER_ACTOR(self));
	while(clutter_actor_iter_next(&iter, &child))
	{
		/* Only check visible children */
		if(!CLUTTER_ACTOR_IS_VISIBLE(child)) continue;

		/* Get height of child */
		clutter_actor_get_preferred_height(child, -1, &childMinHeight, &childNaturalHeight);
		if(inDoMinimumSize==TRUE) childHeight=childMinHeight;
			else childHeight=childNaturalHeight;

		/* Determine total size so far */
		totalHeight+=ceil(childHeight);

		/* Count visible children */
		numberChildren++;
	}
	if(numberChildren==0) return(priv->scaleMax);

	/* Determine scalable height. That is the height without spacing
	 * betwen children and the spacing used as margin.
	 */
	scalableHeight=inForHeight-((numberChildren+1)*priv->spacing);

	/* Get scale factor */
	scale=priv->scaleMax;
	if(totalHeight>0.0f)
	{
		scale=floorf((scalableHeight/totalHeight)/priv->scaleStep)*priv->scaleStep;
		scale=MIN(scale, priv->scaleMax);
		scale=MAX(scale, priv->scaleMin);
	}

	/* Check if all visible children would really fit into height
	 * otherwise we need to decrease scale factor one step down
	 */
	if(scale>priv->scaleMin)
	{
		do
		{
			recheckHeight=FALSE;
			totalHeight=priv->spacing;

			/* Iterate through visible children and sum their scaled
			 * heights. The total height will be initialized with unscaled
			 * spacing and all visible children's scaled height will also
			 * be added with unscaled spacing to have the margin added.
			 */
			clutter_actor_iter_init(&iter, CLUTTER_ACTOR(self));
			while(clutter_actor_iter_next(&iter, &child))
			{
				/* Only check visible children */
				if(!CLUTTER_ACTOR_IS_VISIBLE(child)) continue;

				/* Get scaled size of child and add to total height */
				clutter_actor_get_preferred_height(child, -1, &childMinHeight, &childNaturalHeight);
				if(inDoMinimumSize==TRUE) childHeight=childMinHeight;
					else childHeight=childNaturalHeight;

				childHeight*=scale;
				totalHeight+=ceil(childHeight)+priv->spacing;
			}

			/* If total height is greater than given height
			 * decrease scale factor by one step and recheck
			 */
			if(totalHeight>inForHeight && scale>priv->scaleMin)
			{
				scale-=priv->scaleStep;
				recheckHeight=TRUE;
			}
		}
		while(recheckHeight==TRUE);
	}

	/* Return found scale factor */
	return(scale);
}

/* IMPLEMENTATION: ClutterActor */

/* Get preferred width/height */
void _xfdashboard_quicklaunch_get_preferred_height(ClutterActor *inActor,
													gfloat inForWidth,
													gfloat *outMinHeight,
													gfloat *outNaturalHeight)
{
	XfdashboardQuicklaunchPrivate	*priv=XFDASHBOARD_QUICKLAUNCH(inActor)->priv;
	gfloat							minHeight, naturalHeight;
	ClutterActor					*child;
	ClutterActorIter				iter;
	gfloat							childMinHeight, childNaturalHeight;
	gint							numberChildren;

	/* Set up default values */
	minHeight=naturalHeight=0.0f;

	/* Iterate through visible children and determine height */
	numberChildren=0;
	clutter_actor_iter_init(&iter, CLUTTER_ACTOR(inActor));
	while(clutter_actor_iter_next(&iter, &child))
	{
		/* Only check visible children */
		if(!CLUTTER_ACTOR_IS_VISIBLE(child)) continue;

		/* Get child's size */
		clutter_actor_get_preferred_height(child,
											inForWidth,
											&childMinHeight,
											&childNaturalHeight);

		/* Determine height */
		minHeight+=childMinHeight;
		naturalHeight+=childNaturalHeight;

		/* Count visible children */
		numberChildren++;
	}

	/* Add spacing between children and spacing as margin */
	if(numberChildren>0)
	{
		minHeight+=(numberChildren+1)*priv->spacing;
		naturalHeight+=(numberChildren+1)*priv->spacing;
	}

	/* Store sizes computed */
	if(outMinHeight) *outMinHeight=minHeight;
	if(outNaturalHeight) *outNaturalHeight=naturalHeight;
}

void _xfdashboard_quicklaunch_get_preferred_width(ClutterActor *inActor,
													gfloat inForHeight,
													gfloat *outMinWidth,
													gfloat *outNaturalWidth)
{
	XfdashboardQuicklaunch			*self=XFDASHBOARD_QUICKLAUNCH(inActor);
	XfdashboardQuicklaunchPrivate	*priv=self->priv;
	gfloat							minWidth, naturalWidth;
	ClutterActor					*child;
	ClutterActorIter				iter;
	gfloat							childMinWidth, childNaturalWidth;
	gint							numberChildren;
	gfloat							scale;

	/* Set up default values */
	minWidth=naturalWidth=0.0f;

	/* Iterate through visible children and determine widths */
	numberChildren=0;
	clutter_actor_iter_init(&iter, CLUTTER_ACTOR(inActor));
	while(clutter_actor_iter_next(&iter, &child))
	{
		/* Only check visible children */
		if(!CLUTTER_ACTOR_IS_VISIBLE(child)) continue;

		/* Get sizes of child */
		clutter_actor_get_preferred_width(child,
											-1,
											&childMinWidth,
											&childNaturalWidth);

		/* Determine width and height */
		minWidth=MAX(minWidth, childMinWidth);
		naturalWidth=MAX(naturalWidth, childNaturalWidth);

		/* Count visible children */
		numberChildren++;
	}

	/* Check if we need to scale width because of the need to fit
	 * all visible children into given limiting height
	 */
	if(inForHeight>=0.0f)
	{
		scale=_xfdashboard_quicklaunch_get_scale_for_height(self, inForHeight, TRUE);
		minWidth*=scale;

		scale=_xfdashboard_quicklaunch_get_scale_for_height(self, inForHeight, FALSE);
		naturalWidth*=scale;
	}

	/* Add spacing as margin */
	if(numberChildren>0)
	{
		minWidth+=2*priv->spacing;
		naturalWidth+=2*priv->spacing;
	}

	/* Store sizes computed */
	if(outMinWidth) *outMinWidth=minWidth;
	if(outNaturalWidth) *outNaturalWidth=naturalWidth;
}

/* Allocate position and size of actor and its children*/
void _xfdashboard_quicklaunch_allocate(ClutterActor *inActor,
										const ClutterActorBox *inBox,
										ClutterAllocationFlags inFlags)
{
	XfdashboardQuicklaunch			*self=XFDASHBOARD_QUICKLAUNCH(inActor);
	XfdashboardQuicklaunchPrivate	*priv=self->priv;
	gfloat							availableWidth, availableHeight;
	gfloat							childWidth, childHeight;
	ClutterActor					*child;
	ClutterActorIter				iter;
	ClutterActorBox					childAllocation={ 0, };

	/* Chain up to store the allocation of the actor */
	CLUTTER_ACTOR_CLASS(xfdashboard_quicklaunch_parent_class)->allocate(inActor, inBox, inFlags);

	/* Get available size */
	clutter_actor_box_get_size(inBox, &availableWidth, &availableHeight);

	/* Find scaling to get all children fit the allocation */
	priv->scaleCurrent=_xfdashboard_quicklaunch_get_scale_for_height(self, availableHeight, FALSE);

	/* Calculate new position and size of visible children */
	childAllocation.y1=priv->spacing;
	clutter_actor_iter_init(&iter, CLUTTER_ACTOR(inActor));
	while(clutter_actor_iter_next(&iter, &child))
	{
		/* Is child visible? */
		if(!CLUTTER_ACTOR_IS_VISIBLE(child)) continue;

		/* Calculate new position and size of child. Because we will set
		 * a scale factor to the actor we have to set the real unscaled
		 * width and height but the position should be "translated" to
		 * scaled sizes.
		 */
		clutter_actor_get_preferred_size(child, NULL, &childWidth, NULL, &childHeight);

		childAllocation.x1=ceil(MAX(((availableWidth-(childWidth*priv->scaleCurrent))/2.0f), priv->spacing));
		childAllocation.x2=ceil(childAllocation.x1+childWidth);
		childAllocation.y2=ceil(childAllocation.y1+childHeight);

		clutter_actor_set_scale(child, priv->scaleCurrent, priv->scaleCurrent);
		clutter_actor_allocate(child, &childAllocation, inFlags);

		/* Set up for next child */
		childWidth*=priv->scaleCurrent;
		childHeight*=priv->scaleCurrent;
		childAllocation.y1=ceil(childAllocation.y1+childHeight+priv->spacing);
	}
}

/* IMPLEMENTATION: GObject */

/* Dispose this object */
void _xfdashboard_quicklaunch_dispose(GObject *inObject)
{
	XfdashboardQuicklaunchPrivate	*priv=XFDASHBOARD_QUICKLAUNCH(inObject)->priv;

	/* Release our allocated variables */
	priv->xfconfChannel=NULL;

	/* Call parent's class dispose method */
	G_OBJECT_CLASS(xfdashboard_quicklaunch_parent_class)->dispose(inObject);
}

/* Set/get properties */
void _xfdashboard_quicklaunch_set_property(GObject *inObject,
											guint inPropID,
											const GValue *inValue,
											GParamSpec *inSpec)
{
	XfdashboardQuicklaunch			*self=XFDASHBOARD_QUICKLAUNCH(inObject);

	switch(inPropID)
	{
		case PROP_FAVOURITES:
			_xfdashboard_quicklaunch_set_favourites(self, inValue);
			break;

		case PROP_SPACING:
			xfdashboard_quicklaunch_set_spacing(self, g_value_get_float(inValue));
			break;

		default:
			G_OBJECT_WARN_INVALID_PROPERTY_ID(inObject, inPropID, inSpec);
			break;
	}
}

void _xfdashboard_quicklaunch_get_property(GObject *inObject,
											guint inPropID,
											GValue *outValue,
											GParamSpec *inSpec)
{
	XfdashboardQuicklaunch			*self=XFDASHBOARD_QUICKLAUNCH(inObject);
	XfdashboardQuicklaunchPrivate	*priv=self->priv;

	switch(inPropID)
	{
		case PROP_FAVOURITES:
			g_value_set_boxed(outValue, priv->favourites);
			break;

		case PROP_SPACING:
			g_value_set_float(outValue, priv->spacing);
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
void xfdashboard_quicklaunch_class_init(XfdashboardQuicklaunchClass *klass)
{
	ClutterActorClass	*actorClass=CLUTTER_ACTOR_CLASS(klass);
	GObjectClass		*gobjectClass=G_OBJECT_CLASS(klass);

	/* Override functions */
	gobjectClass->dispose=_xfdashboard_quicklaunch_dispose;
	gobjectClass->set_property=_xfdashboard_quicklaunch_set_property;
	gobjectClass->get_property=_xfdashboard_quicklaunch_get_property;

	actorClass->get_preferred_width=_xfdashboard_quicklaunch_get_preferred_width;
	actorClass->get_preferred_height=_xfdashboard_quicklaunch_get_preferred_height;
	actorClass->allocate=_xfdashboard_quicklaunch_allocate;

	/* Set up private structure */
	g_type_class_add_private(klass, sizeof(XfdashboardQuicklaunchPrivate));

	/* Define properties */
	XfdashboardQuicklaunchProperties[PROP_FAVOURITES]=
		g_param_spec_boxed("favourites",
							_("Favourites"),
							_("An array of strings pointing to desktop files shown as icons"),
							XFDASHBOARD_TYPE_POINTER_ARRAY,
							G_PARAM_READWRITE);

	XfdashboardQuicklaunchProperties[PROP_SPACING]=
		g_param_spec_float("spacing",
								_("Spacing"),
								_("The spacing between children"),
								0.0, G_MAXFLOAT,
								0.0,
								G_PARAM_READWRITE);

	g_object_class_install_properties(gobjectClass, PROP_LAST, XfdashboardQuicklaunchProperties);

	/* Define signals */
	// TODO
}

/* Object initialization
 * Create private structure and set up default values
 */
void xfdashboard_quicklaunch_init(XfdashboardQuicklaunch *self)
{
	XfdashboardQuicklaunchPrivate	*priv;

	priv=self->priv=XFDASHBOARD_QUICKLAUNCH_GET_PRIVATE(self);

	/* Set up default values */
	priv->favourites=NULL;
	priv->spacing=0.0f;
	priv->scaleCurrent=DEFAULT_SCALE_MAX;
	priv->scaleMin=DEFAULT_SCALE_MIN;
	priv->scaleMax=DEFAULT_SCALE_MAX;
	priv->scaleStep=DEFAULT_SCALE_STEP;
	priv->xfconfChannel=XFCONF_CHANNEL(g_object_ref(xfdashboard_application_get_xfconf_channel()));

	/* Set up this actor */
	clutter_actor_set_reactive(CLUTTER_ACTOR(self), TRUE);

	/* Bind to xfconf to react on changes */
	xfconf_g_property_bind(priv->xfconfChannel, "/favourites", XFDASHBOARD_TYPE_POINTER_ARRAY, self, "favourites");
}

/* Implementation: Public API */

/* Create new actor */
ClutterActor* xfdashboard_quicklaunch_new(void)
{
	return(g_object_new(XFDASHBOARD_TYPE_QUICKLAUNCH, NULL));
}

/* Get/set spacing between children */
gfloat xfdashboard_quicklaunch_get_spacing(XfdashboardQuicklaunch *self)
{
	g_return_val_if_fail(XFDASHBOARD_IS_QUICKLAUNCH(self), 0.0f);

	return(self->priv->spacing);
}

void xfdashboard_quicklaunch_set_spacing(XfdashboardQuicklaunch *self, const gfloat inSpacing)
{
	g_return_if_fail(XFDASHBOARD_IS_QUICKLAUNCH(self));
	g_return_if_fail(inSpacing>=0.0f);

	XfdashboardQuicklaunchPrivate	*priv=self->priv;

	/* Set value if changed */
	if(priv->spacing!=inSpacing)
	{
		/* Set value */
		priv->spacing=inSpacing;
		clutter_actor_queue_relayout(CLUTTER_ACTOR(self));

		xfdashboard_background_set_corner_radius(XFDASHBOARD_BACKGROUND(self), priv->spacing);

		/* Notify about property change */
		g_object_notify_by_pspec(G_OBJECT(self), XfdashboardQuicklaunchProperties[PROP_SPACING]);
	}
}
