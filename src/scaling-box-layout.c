/*
 * scaling-box-layout.c: A box layout scaling all actor to fit in
 *                       allocation of parent actor
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

#include "scaling-box-layout.h"

#include <math.h>

/* Define this class in GObject system */
G_DEFINE_TYPE(XfdashboardScalingBoxLayout,
				xfdashboard_scaling_box_layout,
				CLUTTER_TYPE_LAYOUT_MANAGER)

/* Private structure - access only by public API if needed */
#define XFDASHBOARD_SCALING_BOX_LAYOUT_GET_PRIVATE(obj) \
	(G_TYPE_INSTANCE_GET_PRIVATE((obj), XFDASHBOARD_TYPE_SCALING_BOX_LAYOUT, XfdashboardScalingBoxLayoutPrivate))

struct _XfdashboardScalingBoxLayoutPrivate
{
	/* Containter whose children to layout */
	ClutterContainer		*container;

	/* Settings */
	gfloat					scaleMin;
	gfloat					scaleMax;
	gfloat					scaleStep;

	gfloat					spacing;

	/* Computed values of last allocation */
	gfloat					scaleCurrent;
	ClutterActorBox			lastAllocation;
};

/* Properties */
enum
{
	PROP_0,

	PROP_SCALE_CURRENT,
	PROP_SCALE_MIN,
	PROP_SCALE_MAX,
	PROP_SCALE_STEP,

	PROP_SPACING,

	PROP_LAST_ALLOCATION,
	
	PROP_LAST
};

static GParamSpec* XfdashboardScalingBoxLayoutProperties[PROP_LAST]={ 0, };

/* IMPLEMENTATION: Private variables and methods */
#define DEFAULT_SCALE_MIN		0.1
#define DEFAULT_SCALE_MAX		1.0
#define DEFAULT_SCALE_STEP		0.1

/* IMPLEMENTATION: ClutterLayoutManager */

/* Get preferred width/height */
static void xfdashboard_scaling_box_layout_get_preferred_width(ClutterLayoutManager *inManager,
																ClutterContainer *inContainer,
																gfloat inForHeight,
																gfloat *outMinWidth,
																gfloat *outNaturalWidth)
{
	g_return_if_fail(XFDASHBOARD_IS_SCALING_BOX_LAYOUT(inManager));

	gfloat									maxMinWidth, maxNaturalWidth;
	GList									*children;

	maxMinWidth=maxNaturalWidth=0.0f;
	for(children=clutter_container_get_children(inContainer); children; children=children->next)
	{
		ClutterActor						*child=CLUTTER_ACTOR(children->data);
		gfloat								childMinWidth, childNaturalWidth;

		if(!CLUTTER_ACTOR_IS_VISIBLE(child)) continue;

		clutter_actor_get_preferred_width(child,
											inForHeight,
											&childMinWidth,
											&childNaturalWidth);

		if(childMinWidth>maxMinWidth) maxMinWidth=childMinWidth;
		if(childNaturalWidth>maxNaturalWidth) maxNaturalWidth=childNaturalWidth;
	}

	if(outMinWidth) *outMinWidth=maxMinWidth;
	if(outNaturalWidth) *outNaturalWidth=maxNaturalWidth;
}

static void xfdashboard_scaling_box_layout_get_preferred_height(ClutterLayoutManager *inManager,
																ClutterContainer *inContainer,
																gfloat inForWidth,
																gfloat *outMinHeight,
																gfloat *outNaturalHeight)
{
	g_return_if_fail(XFDASHBOARD_IS_SCALING_BOX_LAYOUT(inManager));

	XfdashboardScalingBoxLayoutPrivate		*priv=XFDASHBOARD_SCALING_BOX_LAYOUT(inManager)->priv;
	gfloat									maxMinHeight, maxNaturalHeight;
	GList									*children;
	gint									numberChildren=0;

	maxMinHeight=maxNaturalHeight=0.0f;
	for(children=clutter_container_get_children(inContainer);
			children;
			children=children->next)
	{
		ClutterActor						*child=CLUTTER_ACTOR(children->data);
		gfloat								childMinHeight, childNaturalHeight;

		if(!CLUTTER_ACTOR_IS_VISIBLE(child)) continue;
		
		clutter_actor_get_preferred_height(child,
											inForWidth,
											&childMinHeight,
											&childNaturalHeight);

		maxMinHeight+=childMinHeight;
		maxNaturalHeight+=childNaturalHeight;

		numberChildren++;
	}

	if(outMinHeight) *outMinHeight=maxMinHeight+((numberChildren-1)*priv->spacing);
	if(outNaturalHeight) *outNaturalHeight=maxNaturalHeight+((numberChildren-1)*priv->spacing);
}

/* Re-layout and allocate children of container we manage */
static void xfdashboard_scaling_box_layout_allocate(ClutterLayoutManager *inManager,
													ClutterContainer *inContainer,
													const ClutterActorBox *inBox,
													ClutterAllocationFlags inFlags)
{
	XfdashboardScalingBoxLayout			*self=XFDASHBOARD_SCALING_BOX_LAYOUT(inManager);
	XfdashboardScalingBoxLayoutPrivate	*priv=self->priv;
	gfloat								availableWidth, availableHeight;
	gfloat								maxWidth, maxHeight;
	gfloat								iconsWidth, iconsHeight;
	gfloat								iconsScaleWidth, iconsScaleHeight;
	ClutterActorBox						childAllocation={ 0, };
	GList								*children;
	gint								numberChildren;

	/* Get list of children to layout */
	children=clutter_container_get_children(inContainer);
	numberChildren=g_list_length(children);

	/* Get available size */
	clutter_actor_box_get_size(inBox, &availableWidth, &availableHeight);

	/* Get preferred size */
	xfdashboard_scaling_box_layout_get_preferred_width(inManager,
														inContainer,
														availableHeight,
														NULL,
														&iconsWidth);
	xfdashboard_scaling_box_layout_get_preferred_height(inManager,
														inContainer,
														availableWidth,
														NULL,
														&iconsHeight);

	/* Decrease size by all spacings */
	if(numberChildren>0)
	{
		availableHeight-=(numberChildren-1)*self->priv->spacing;
		iconsHeight-=(numberChildren-1)*self->priv->spacing;
	}
	
	/* Find scaling to get all children fit the allocation */
	if(iconsWidth>0) iconsScaleWidth=MIN(availableWidth/iconsWidth, priv->scaleMax);
		else iconsScaleWidth=priv->scaleMax;
		
	if(iconsHeight>0)
	{
		iconsScaleHeight=floorf((availableHeight/iconsHeight)/priv->scaleStep)*priv->scaleStep;
		iconsScaleHeight=MIN(iconsScaleHeight, priv->scaleMax);
	}
		else iconsScaleHeight=priv->scaleMax;
		
	priv->scaleCurrent=MIN(iconsScaleWidth, iconsScaleHeight);

	/* Calculate new position and size of children */
	maxWidth=maxHeight=0.0f;
	for(; children; children=children->next)
	{
		ClutterActor					*child=CLUTTER_ACTOR(children->data);
		gfloat							childWidth, childHeight;

		/* Calculate new position and size of child */
		clutter_actor_get_preferred_width(child, -1, NULL, &childWidth);
		clutter_actor_get_preferred_height(child, -1, NULL, &childHeight);

		childWidth*=priv->scaleCurrent;
		childHeight*=priv->scaleCurrent;
		
		childAllocation.x1=ceil(MAX((availableWidth-childWidth)/2.0f, 0.0f));
		childAllocation.x2=ceil(childAllocation.x1+childWidth);
		childAllocation.y2=ceil(childAllocation.y1+childHeight);

		clutter_actor_allocate(child, &childAllocation, inFlags);

		/* Set up for next child */
		childAllocation.y1=ceil(childAllocation.y2+priv->spacing);

		/* Remember maximum sizes */
		maxWidth=MAX(childAllocation.x2-childAllocation.x1, maxWidth);
		maxHeight=childAllocation.y2;
	}

	/* Store allocation containing all children */
	clutter_actor_box_set_origin(&priv->lastAllocation, 0, 0);
	clutter_actor_box_set_size(&priv->lastAllocation, maxWidth, maxHeight);
}

/* Set container whose children to layout */
static void xfdashboard_scaling_box_layout_set_container(ClutterLayoutManager *inManager,
															ClutterContainer *inContainer)
{
	XfdashboardScalingBoxLayoutPrivate		*priv=XFDASHBOARD_SCALING_BOX_LAYOUT(inManager)->priv;
	ClutterLayoutManagerClass				*parentClass;

	priv->container=inContainer;
	if(priv->container!=NULL)
	{
		/* We need to change the :request-mode of the container
		 * to match the horizontal orientation of this manager
		 */
		clutter_actor_set_request_mode(CLUTTER_ACTOR(priv->container), CLUTTER_REQUEST_WIDTH_FOR_HEIGHT);
	}

	parentClass=CLUTTER_LAYOUT_MANAGER_CLASS(xfdashboard_scaling_box_layout_parent_class);
	parentClass->set_container(inManager, inContainer);
}

/* IMPLEMENTATION: GObject */

/* Finally free up all resource of this object as it will be destroyed */
static void xfdashboard_scaling_box_layout_finalize(GObject *inObject)
{
	G_OBJECT_CLASS(xfdashboard_scaling_box_layout_parent_class)->finalize(inObject);
}

/* Set/get properties */
static void xfdashboard_scaling_box_layout_set_property(GObject *inObject,
														guint inPropID,
														const GValue *inValue,
														GParamSpec *inSpec)
{
	XfdashboardScalingBoxLayout	*self=XFDASHBOARD_SCALING_BOX_LAYOUT(inObject);
	
	switch(inPropID)
	{
		case PROP_SCALE_MIN:
			xfdashboard_scaling_box_layout_set_scale_minimum(self, g_value_get_float(inValue));
			break;

		case PROP_SCALE_MAX:
			xfdashboard_scaling_box_layout_set_scale_maximum(self, g_value_get_float(inValue));
			break;

		case PROP_SCALE_STEP:
			xfdashboard_scaling_box_layout_set_scale_step(self, g_value_get_float(inValue));
			break;

		case PROP_SPACING:
			xfdashboard_scaling_box_layout_set_spacing(self, g_value_get_float(inValue));
			break;

		default:
			G_OBJECT_WARN_INVALID_PROPERTY_ID(inObject, inPropID, inSpec);
			break;
	}
}

static void xfdashboard_scaling_box_layout_get_property(GObject *inObject,
														guint inPropID,
														GValue *outValue,
														GParamSpec *inSpec)
{
	XfdashboardScalingBoxLayout	*self=XFDASHBOARD_SCALING_BOX_LAYOUT(inObject);

	switch(inPropID)
	{
		case PROP_SCALE_CURRENT:
			g_value_set_float(outValue, self->priv->scaleCurrent);
			break;

		case PROP_SCALE_MIN:
			g_value_set_float(outValue, self->priv->scaleMin);
			break;

		case PROP_SCALE_MAX:
			g_value_set_float(outValue, self->priv->scaleMin);
			break;

		case PROP_SCALE_STEP:
			g_value_set_float(outValue, self->priv->scaleMin);
			break;

		case PROP_SPACING:
			g_value_set_float(outValue, self->priv->spacing);
			break;

		case PROP_LAST_ALLOCATION:
			g_value_set_boxed(outValue, &self->priv->lastAllocation);
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
static void xfdashboard_scaling_box_layout_class_init(XfdashboardScalingBoxLayoutClass *klass)
{
	ClutterLayoutManagerClass	*layoutClass=CLUTTER_LAYOUT_MANAGER_CLASS(klass);
	GObjectClass				*gobjectClass=G_OBJECT_CLASS(klass);

	/* Override functions */
	gobjectClass->finalize=xfdashboard_scaling_box_layout_finalize;
	gobjectClass->set_property=xfdashboard_scaling_box_layout_set_property;
	gobjectClass->get_property=xfdashboard_scaling_box_layout_get_property;

	layoutClass->get_preferred_width=xfdashboard_scaling_box_layout_get_preferred_width;
	layoutClass->get_preferred_height=xfdashboard_scaling_box_layout_get_preferred_height;
	layoutClass->allocate=xfdashboard_scaling_box_layout_allocate;
	layoutClass->set_container=xfdashboard_scaling_box_layout_set_container;

	/* Set up private structure */
	g_type_class_add_private(klass, sizeof(XfdashboardScalingBoxLayoutPrivate));

	/* Define properties */
	XfdashboardScalingBoxLayoutProperties[PROP_SCALE_CURRENT]=
		g_param_spec_float("scale",
								"Current scale",
								"Current scale to used to get children fit allocation",
								0.0, 1.0,
								DEFAULT_SCALE_MAX,
								G_PARAM_READABLE);

	XfdashboardScalingBoxLayoutProperties[PROP_SCALE_MIN]=
		g_param_spec_float("min-scale",
								"Minimum scale",
								"Smallest scale to use when children do not fit allocation",
								0.0, 1.0,
								DEFAULT_SCALE_MIN,
								G_PARAM_READWRITE);

	XfdashboardScalingBoxLayoutProperties[PROP_SCALE_MAX]=
		g_param_spec_float("max-scale",
								"Maximum scale",
								"Largest scale to use when layouting children in allocation",
								0.0, 1.0,
								DEFAULT_SCALE_MAX,
								G_PARAM_READWRITE);

	XfdashboardScalingBoxLayoutProperties[PROP_SCALE_STEP]=
		g_param_spec_float("scale-step",
								"Steps of down-scaling",
								"The steps which scale is decreased until children fit allocation or minimum scale reached",
								0.0, 1.0,
								DEFAULT_SCALE_STEP,
								G_PARAM_READWRITE);

	XfdashboardScalingBoxLayoutProperties[PROP_SPACING]=
		g_param_spec_float("spacing",
								"Spacing",
								"The spacing between children",
								0.0, G_MAXFLOAT,
								0.0,
								G_PARAM_READWRITE | G_PARAM_CONSTRUCT);

	XfdashboardScalingBoxLayoutProperties[PROP_LAST_ALLOCATION]=
		g_param_spec_boxed("last-allocation",
							"Last allocation",
							"The allocation containing all children computed on last allocation run",
							CLUTTER_TYPE_ACTOR_BOX,
							G_PARAM_READABLE);

	g_object_class_install_properties(gobjectClass, PROP_LAST, XfdashboardScalingBoxLayoutProperties);
}

/* Object initialization
 * Create private structure and set up default values
 */
static void xfdashboard_scaling_box_layout_init(XfdashboardScalingBoxLayout *self)
{
	XfdashboardScalingBoxLayoutPrivate	*priv;

	/* Set default values */
	priv=self->priv=XFDASHBOARD_SCALING_BOX_LAYOUT_GET_PRIVATE(self);

	priv->scaleCurrent=DEFAULT_SCALE_MAX;
	priv->scaleMin=DEFAULT_SCALE_MIN;
	priv->scaleMax=DEFAULT_SCALE_MAX;
	priv->scaleStep=DEFAULT_SCALE_STEP;
}

/* Implementation: Public API */

ClutterLayoutManager* xfdashboard_scaling_box_layout_new()
{
	return(g_object_new(XFDASHBOARD_TYPE_SCALING_BOX_LAYOUT, NULL));
}

/* Get current scale */
gfloat xfdashboard_scaling_box_layout_get_scale(XfdashboardScalingBoxLayout *self)
{
	g_return_val_if_fail(XFDASHBOARD_IS_SCALING_BOX_LAYOUT(self), 0.0f);

	return(self->priv->scaleCurrent);
}

/* Get/set min/max/step scale values */
gfloat xfdashboard_scaling_box_layout_get_scale_minimum(XfdashboardScalingBoxLayout *self)
{
	g_return_val_if_fail(XFDASHBOARD_IS_SCALING_BOX_LAYOUT(self), 0.0f);

	return(self->priv->scaleMin);
}

void xfdashboard_scaling_box_layout_set_scale_minimum(XfdashboardScalingBoxLayout *self, gfloat inScale)
{
	g_return_if_fail(XFDASHBOARD_IS_SCALING_BOX_LAYOUT(self));

	/* Set scale */
	if(inScale>self->priv->scaleMax)
	{
		self->priv->scaleMin=self->priv->scaleMax;
		self->priv->scaleMax=inScale;
	}
		else self->priv->scaleMin=inScale;

	clutter_layout_manager_layout_changed(CLUTTER_LAYOUT_MANAGER(self));
}

gfloat xfdashboard_scaling_box_layout_get_scale_maximum(XfdashboardScalingBoxLayout *self)
{
	g_return_val_if_fail(XFDASHBOARD_IS_SCALING_BOX_LAYOUT(self), 0.0f);

	return(self->priv->scaleMax);
}

void xfdashboard_scaling_box_layout_set_scale_maximum(XfdashboardScalingBoxLayout *self, gfloat inScale)
{
	g_return_if_fail(XFDASHBOARD_IS_SCALING_BOX_LAYOUT(self));

	/* Set scale */
	if(inScale<self->priv->scaleMin)
	{
		self->priv->scaleMax=self->priv->scaleMin;
		self->priv->scaleMin=inScale;
	}
		else self->priv->scaleMax=inScale;

	clutter_layout_manager_layout_changed(CLUTTER_LAYOUT_MANAGER(self));
}

gfloat xfdashboard_scaling_box_layout_get_scale_step(XfdashboardScalingBoxLayout *self)
{
	g_return_val_if_fail(XFDASHBOARD_IS_SCALING_BOX_LAYOUT(self), 0.0f);

	return(self->priv->scaleStep);
}

void xfdashboard_scaling_box_layout_set_scale_step(XfdashboardScalingBoxLayout *self, gfloat inScale)
{
	g_return_if_fail(XFDASHBOARD_IS_SCALING_BOX_LAYOUT(self));
	g_return_if_fail(inScale<=(self->priv->scaleMax-self->priv->scaleMin));
	
	/* Set scale */
	self->priv->scaleStep=inScale;

	clutter_layout_manager_layout_changed(CLUTTER_LAYOUT_MANAGER(self));
}

/* Get/set spacing */
gfloat xfdashboard_scaling_box_layout_get_spacing(XfdashboardScalingBoxLayout *self)
{
	g_return_val_if_fail(XFDASHBOARD_IS_SCALING_BOX_LAYOUT(self), 0.0f);

	return(self->priv->spacing);
}

void xfdashboard_scaling_box_layout_set_spacing(XfdashboardScalingBoxLayout *self, gfloat inSpacing)
{
	g_return_if_fail(XFDASHBOARD_IS_SCALING_BOX_LAYOUT(self));

	/* Set spacing */
	self->priv->spacing=inSpacing;

	clutter_layout_manager_layout_changed(CLUTTER_LAYOUT_MANAGER(self));
}

/* Get allocation computed on last allocation run */
const ClutterActorBox* xfdashboard_scaling_box_layout_get_last_allocation(XfdashboardScalingBoxLayout *self)
{
	g_return_val_if_fail(XFDASHBOARD_IS_SCALING_BOX_LAYOUT(self), NULL);

	return(&self->priv->lastAllocation);
}
