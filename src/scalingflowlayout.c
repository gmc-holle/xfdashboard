/*
 * scalingflowlayout.c: A flow layout scaling all actor to fit in
 *                      allocation of parent actor
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

#include "scalingflowlayout.h"

#include <math.h>

/* Define this class in GObject system */
G_DEFINE_TYPE(XfdashboardScalingFlowLayout,
				xfdashboard_scalingflow_layout,
				CLUTTER_TYPE_LAYOUT_MANAGER)

/* Private structure - access only by public API if needed */
#define XFDASHBOARD_SCALINGFLOW_LAYOUT_GET_PRIVATE(obj) \
	(G_TYPE_INSTANCE_GET_PRIVATE((obj), XFDASHBOARD_TYPE_SCALINGFLOW_LAYOUT, XfdashboardScalingFlowLayoutPrivate))

struct _XfdashboardScalingFlowLayoutPrivate
{
	/* Containter whose children to layout */
	ClutterContainer		*container;
	
	/* Settings */
	gboolean				relativeScale;

	gfloat					rowSpacing;
	gfloat					columnSpacing;
};

/* Properties */
enum
{
	PROP_0,

	PROP_RELATIVE_SCALE,

	PROP_ROW_SPACING,
	PROP_COLUMN_SPACING,

	PROP_LAST
};

static GParamSpec* XfdashboardScalingFlowLayoutProperties[PROP_LAST]={ 0, };

/* IMPLEMENTATION: ClutterLayoutManager */

/* Get preferred width/height */
static void xfdashboard_scalingflow_layout_get_preferred_width(ClutterLayoutManager *inManager,
																ClutterContainer *inContainer,
																gfloat inForHeight,
																gfloat *outMinWidth,
																gfloat *outNaturalWidth)
{
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

static void xfdashboard_scalingflow_layout_get_preferred_height(ClutterLayoutManager *inManager,
																ClutterContainer *inContainer,
																gfloat inForWidth,
																gfloat *outMinHeight,
																gfloat *outNaturalHeight)
{
	gfloat									maxMinHeight, maxNaturalHeight;
	GList									*children;

	maxMinHeight=maxNaturalHeight=0.0f;
	for(children=clutter_container_get_children(inContainer); children; children=children->next)
	{
		ClutterActor						*child=CLUTTER_ACTOR(children->data);
		gfloat								childMinHeight, childNaturalHeight;

		if(!CLUTTER_ACTOR_IS_VISIBLE(child)) continue;
		
		clutter_actor_get_preferred_height(child,
											inForWidth,
											&childMinHeight,
											&childNaturalHeight);

		if(childMinHeight>maxMinHeight) maxMinHeight=childMinHeight;
		if(childNaturalHeight>maxMinHeight) maxNaturalHeight=childNaturalHeight;
	}

	*outMinHeight=maxMinHeight;
	*outNaturalHeight=maxNaturalHeight;
}

/* Re-layout and allocate children of container we manage */
static void xfdashboard_scalingflow_layout_allocate(ClutterLayoutManager *inManager,
													ClutterContainer *inContainer,
													const ClutterActorBox *inAllocation,
													ClutterAllocationFlags inFlags)
{
	XfdashboardScalingFlowLayout	*self=XFDASHBOARD_SCALINGFLOW_LAYOUT(inManager);
	int								numberRows, numberCols;
	int								row=0, col=0;
	int								cellWidth, cellHeight;
	gfloat							maxWidth, maxHeight;
	gfloat							largestWidth, largestHeight;
	ClutterActorBox					childAllocation;
	GList							*children;

	/* Get list of children to layout */
	children=clutter_container_get_children(inContainer);
	
	/* Find best fitting number of rows and colums for layout */
	numberCols=ceil(sqrt((double)g_list_length(children)));
	numberRows=ceil((double)g_list_length(children) / (double)numberCols);

	/* Get size of container holding children to layout */
	clutter_actor_get_size(CLUTTER_ACTOR(inContainer), &maxWidth, &maxHeight);
	maxWidth-=((numberCols-1)*self->priv->columnSpacing);
	maxHeight-=((numberRows-1)*self->priv->rowSpacing);

	/* Get size of cell */
	cellWidth=floor(maxWidth/numberCols);
	cellHeight=floor(maxHeight/numberRows);

	/* Find largest width and height of children for scaling children proportional
	 * to largest width and height found if relative scaling is TRUE */
	if(self->priv->relativeScale)
	{
		for(children=clutter_container_get_children(inContainer); children; children=children->next)
		{
			gfloat			childWidth, childHeight;

			clutter_actor_get_preferred_width(CLUTTER_ACTOR(children->data), -1, NULL, &childWidth);
			clutter_actor_get_preferred_height(CLUTTER_ACTOR(children->data), -1, NULL, &childHeight);

			if(childWidth>largestWidth) largestWidth=childWidth;
			if(childHeight>largestHeight) largestHeight=childHeight;
		}
	}
	
	/* Calculate new position and size of children */
	for(children=clutter_container_get_children(inContainer) ; children; children=children->next)
	{
		ClutterActor	*child;
		gfloat			childWidth, childHeight;

		/* Get child */
		child=CLUTTER_ACTOR(children->data);
		
		/* Calculate new position and size of child */		
		childWidth=cellWidth;
		childHeight=cellHeight;
		
		if(self->priv->relativeScale)
		{
			clutter_actor_get_preferred_width(child, -1, NULL, &childWidth);
			clutter_actor_get_preferred_height(child, -1, NULL, &childHeight);

			childWidth=(childWidth/largestWidth)*cellWidth;
			childHeight=(childHeight/largestHeight)*cellHeight;
		}
		
		childAllocation.x1=ceil(col*cellWidth+((cellWidth-childWidth)/2.0f)+(col*self->priv->columnSpacing));
		childAllocation.y1=ceil(row*cellHeight+((cellHeight-childHeight)/2.0f)+(row*self->priv->rowSpacing));
		childAllocation.x2=ceil(childAllocation.x1+childWidth);
		childAllocation.y2=ceil(childAllocation.y1+childHeight);

		clutter_actor_allocate(child, &childAllocation, inFlags);
		
		/* Set up for next child */
		col=(col+1) % numberCols;
		if(col==0) row++;
	}
}

/* Set container whose children to layout */
static void xfdashboard_scalingflow_layout_set_container(ClutterLayoutManager *inManager,
															ClutterContainer *inContainer)
{
	XfdashboardScalingFlowLayoutPrivate		*priv=XFDASHBOARD_SCALINGFLOW_LAYOUT(inManager)->priv;
	ClutterLayoutManagerClass				*parentClass;

	priv->container=inContainer;
	if(priv->container!=NULL)
	{
		/* We need to change the :request-mode of the container
		 * to match the horizontal orientation of this manager
		 */
		clutter_actor_set_request_mode(CLUTTER_ACTOR(priv->container), CLUTTER_REQUEST_HEIGHT_FOR_WIDTH);
	}

	parentClass=CLUTTER_LAYOUT_MANAGER_CLASS(xfdashboard_scalingflow_layout_parent_class);
	parentClass->set_container(inManager, inContainer);
}

/* IMPLEMENTATION: GObject */

/* Finally free up all resource of this object as it will be destroyed */
static void xfdashboard_scalingflow_layout_finalize(GObject *inObject)
{
	G_OBJECT_CLASS(xfdashboard_scalingflow_layout_parent_class)->finalize(inObject);
}

/* Set/get properties */
static void xfdashboard_scalingflow_layout_set_property(GObject *inObject,
														guint inPropID,
														const GValue *inValue,
														GParamSpec *inSpec)
{
	XfdashboardScalingFlowLayout	*self=XFDASHBOARD_SCALINGFLOW_LAYOUT(inObject);
	
	switch(inPropID)
	{
		case PROP_RELATIVE_SCALE:
			xfdashboard_scalingflow_layout_set_relative_scale(self, g_value_get_boolean(inValue));
			break;

		case PROP_ROW_SPACING:
			xfdashboard_scalingflow_layout_set_row_spacing(self, g_value_get_float(inValue));
			break;

		case PROP_COLUMN_SPACING:
			xfdashboard_scalingflow_layout_set_column_spacing(self, g_value_get_float(inValue));
			break;

		default:
			G_OBJECT_WARN_INVALID_PROPERTY_ID(inObject, inPropID, inSpec);
			break;
	}
}

static void xfdashboard_scalingflow_layout_get_property(GObject *inObject,
														guint inPropID,
														GValue *outValue,
														GParamSpec *inSpec)
{
	XfdashboardScalingFlowLayout	*self=XFDASHBOARD_SCALINGFLOW_LAYOUT(inObject);

	switch(inPropID)
	{
		case PROP_RELATIVE_SCALE:
			g_value_set_boolean(outValue, self->priv->relativeScale);
			break;

		case PROP_ROW_SPACING:
			g_value_set_float(outValue, self->priv->rowSpacing);
			break;

		case PROP_COLUMN_SPACING:
			g_value_set_float(outValue, self->priv->columnSpacing);
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
static void xfdashboard_scalingflow_layout_class_init(XfdashboardScalingFlowLayoutClass *klass)
{
	ClutterLayoutManagerClass	*layoutClass=CLUTTER_LAYOUT_MANAGER_CLASS(klass);
	GObjectClass				*gobjectClass=G_OBJECT_CLASS(klass);

	/* Override functions */
	gobjectClass->finalize=xfdashboard_scalingflow_layout_finalize;
	gobjectClass->set_property=xfdashboard_scalingflow_layout_set_property;
	gobjectClass->get_property=xfdashboard_scalingflow_layout_get_property;

	layoutClass->get_preferred_width=xfdashboard_scalingflow_layout_get_preferred_width;
	layoutClass->get_preferred_height=xfdashboard_scalingflow_layout_get_preferred_height;
	layoutClass->allocate=xfdashboard_scalingflow_layout_allocate;
	layoutClass->set_container=xfdashboard_scalingflow_layout_set_container;

	/* Set up private structure */
	g_type_class_add_private(klass, sizeof(XfdashboardScalingFlowLayoutPrivate));

	/* Define properties */
	XfdashboardScalingFlowLayoutProperties[PROP_RELATIVE_SCALE]=
		g_param_spec_boolean("relative-scale",
								"Relative scale",
								"Whether all children should be scaled relatively to largest child",
								TRUE,
								G_PARAM_READWRITE | G_PARAM_CONSTRUCT);

	XfdashboardScalingFlowLayoutProperties[PROP_ROW_SPACING]=
		g_param_spec_float("row-spacing",
								"Row Spacing",
								"The spacing between rows",
								0.0, G_MAXFLOAT,
								0.0,
								G_PARAM_READWRITE | G_PARAM_CONSTRUCT);

	XfdashboardScalingFlowLayoutProperties[PROP_COLUMN_SPACING]=
		g_param_spec_float("column-spacing",
								"Column Spacing",
								"The spacing between columns",
								0.0, G_MAXFLOAT,
								0.0,
								G_PARAM_READWRITE | G_PARAM_CONSTRUCT);

	g_object_class_install_properties(gobjectClass, PROP_LAST, XfdashboardScalingFlowLayoutProperties);
}

/* Object initialization
 * Create private structure and set up default values
 */
static void xfdashboard_scalingflow_layout_init(XfdashboardScalingFlowLayout *self)
{
	XfdashboardScalingFlowLayoutPrivate	*priv;

	priv=self->priv=XFDASHBOARD_SCALINGFLOW_LAYOUT_GET_PRIVATE(self);
}

/* Implementation: Public API */

ClutterLayoutManager* xfdashboard_scalingflow_layout_new()
{
	return(g_object_new(XFDASHBOARD_TYPE_SCALINGFLOW_LAYOUT, NULL));
}

/* Get/set relative scaling of all children to largest one */
gboolean xfdashboard_scalingflow_layout_get_relative_scale(XfdashboardScalingFlowLayout *self)
{
	g_return_val_if_fail(XFDASHBOARD_IS_SCALINGFLOW_LAYOUT(self), FALSE);

	return(self->priv->relativeScale);
}

void xfdashboard_scalingflow_layout_set_relative_scale(XfdashboardScalingFlowLayout *self, gboolean inScaling)
{
	g_return_if_fail(XFDASHBOARD_IS_SCALINGFLOW_LAYOUT(self));

	/* Set relative scaling */
	self->priv->relativeScale=inScaling;

	clutter_layout_manager_layout_changed(CLUTTER_LAYOUT_MANAGER(self));
}

/* Set relative row and column spacing to same value at once */
void xfdashboard_scalingflow_layout_set_spacing(XfdashboardScalingFlowLayout *self, gfloat inSpacing)
{
	g_return_if_fail(XFDASHBOARD_IS_SCALINGFLOW_LAYOUT(self));

	/* Set row and column spacing */
	self->priv->rowSpacing=inSpacing;
	self->priv->columnSpacing=inSpacing;

	clutter_layout_manager_layout_changed(CLUTTER_LAYOUT_MANAGER(self));
}

/* Get/set row spacing */
gfloat xfdashboard_scalingflow_layout_get_row_spacing(XfdashboardScalingFlowLayout *self)
{
	g_return_val_if_fail(XFDASHBOARD_IS_SCALINGFLOW_LAYOUT(self), 0.0f);

	return(self->priv->rowSpacing);
}

void xfdashboard_scalingflow_layout_set_row_spacing(XfdashboardScalingFlowLayout *self, gfloat inSpacing)
{
	g_return_if_fail(XFDASHBOARD_IS_SCALINGFLOW_LAYOUT(self));

	/* Set row spacing */
	self->priv->rowSpacing=inSpacing;

	clutter_layout_manager_layout_changed(CLUTTER_LAYOUT_MANAGER(self));
}

/* Get/set columns spacing */
gfloat xfdashboard_scalingflow_layout_get_column_spacing(XfdashboardScalingFlowLayout *self)
{
	g_return_val_if_fail(XFDASHBOARD_IS_SCALINGFLOW_LAYOUT(self), 0.0f);

	return(self->priv->columnSpacing);
}

void xfdashboard_scalingflow_layout_set_column_spacing(XfdashboardScalingFlowLayout *self, gfloat inSpacing)
{
	g_return_if_fail(XFDASHBOARD_IS_SCALINGFLOW_LAYOUT(self));

	/* Set column spacing */
	self->priv->columnSpacing=inSpacing;

	clutter_layout_manager_layout_changed(CLUTTER_LAYOUT_MANAGER(self));
}
