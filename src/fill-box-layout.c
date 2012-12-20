/*
 * fill-box-layout.c: A box layout expanding actors in direction of axis
 *                    (filling) and using natural size in other direction
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

#include "fill-box-layout.h"

/* Define this class in GObject system */
G_DEFINE_TYPE(XfdashboardFillBoxLayout,
				xfdashboard_fill_box_layout,
				CLUTTER_TYPE_LAYOUT_MANAGER)

/* Private structure - access only by public API if needed */
#define XFDASHBOARD_FILL_BOX_LAYOUT_GET_PRIVATE(obj) \
	(G_TYPE_INSTANCE_GET_PRIVATE((obj), XFDASHBOARD_TYPE_FILL_BOX_LAYOUT, XfdashboardFillBoxLayoutPrivate))

struct _XfdashboardFillBoxLayoutPrivate
{
	/* Containter whose children to layout */
	ClutterContainer		*container;

	/* Settings */
	gboolean				isVertical;
	gboolean				isHomogeneous;
	gfloat					spacing;
};

/* Properties */
enum
{
	PROP_0,

	PROP_VERTICAL,
	PROP_HOMOGENOUS,
	PROP_SPACING,
	
	PROP_LAST
};

static GParamSpec* XfdashboardFillBoxLayoutProperties[PROP_LAST]={ 0, };

/* IMPLEMENTATION: Private variables and methods */

/* Find largest width of height among all children */
ClutterActorBox* _xfdashboard_fill_box_layout_get_largest_size(ClutterLayoutManager *inManager,
																ClutterContainer *inContainer,
																gboolean inDetermineMinimum)
{
	g_return_val_if_fail(XFDASHBOARD_IS_FILL_BOX_LAYOUT(inManager), NULL);
	g_return_val_if_fail(CLUTTER_IS_CONTAINER(inContainer), NULL);

	/* Iterate through all children and determine largest width and height */
	GList		*children, *childrenList;
	gfloat		maxWidth, maxHeight;

	maxWidth=maxHeight=0.0f;
	for(childrenList=children=clutter_container_get_children(inContainer); children; children=children->next)
	{
		ClutterActor						*child=CLUTTER_ACTOR(children->data);
		gfloat								childWidth, childHeight;

		/* Is child visible? */
		if(!CLUTTER_ACTOR_IS_VISIBLE(child)) continue;

		if(inDetermineMinimum) clutter_actor_get_preferred_size(child, &childWidth, &childHeight, NULL, NULL);
			else clutter_actor_get_preferred_size(child, NULL, NULL, &childWidth, &childHeight);

		maxWidth=MAX(maxWidth, childWidth);
		maxHeight=MAX(maxHeight, childHeight);
	}
	g_list_free(childrenList);

	/* Create actor box describing largest width and height */
	return(clutter_actor_box_new(0, 0, maxWidth, maxHeight));
}

/* IMPLEMENTATION: ClutterLayoutManager */

/* Get preferred width/height */
static void xfdashboard_fill_box_layout_get_preferred_width(ClutterLayoutManager *inManager,
																ClutterContainer *inContainer,
																gfloat inForHeight,
																gfloat *outMinWidth,
																gfloat *outNaturalWidth)
{
	g_return_if_fail(XFDASHBOARD_IS_FILL_BOX_LAYOUT(inManager));
	g_return_if_fail(CLUTTER_IS_CONTAINER(inContainer));

	XfdashboardFillBoxLayoutPrivate		*priv=XFDASHBOARD_FILL_BOX_LAYOUT(inManager)->priv;
	GList								*children, *childrenList;
	gint								numberChildren;
	gfloat								minWidth, naturalWidth;
	gfloat								allMaxMinimumWidth, allMaxNaturalWidth;
	ClutterActorBox						*largestBox;

	/* Get children of container */
	childrenList=children=clutter_container_get_children(inContainer);

	/* Set empty sizes */
	minWidth=naturalWidth=0.0f;
	allMaxMinimumWidth=allMaxNaturalWidth=0.0f;

	/* Determine number of children */
	numberChildren=g_list_length(children);

	/* Determine largest width and height among all children */
	largestBox=_xfdashboard_fill_box_layout_get_largest_size(inManager, inContainer, TRUE);
	if(G_LIKELY(largestBox))
	{
		allMaxMinimumWidth=clutter_actor_box_get_width(largestBox);
		clutter_actor_box_free(largestBox);
	}

	largestBox=_xfdashboard_fill_box_layout_get_largest_size(inManager, inContainer, FALSE);
	if(G_LIKELY(largestBox))
	{
		allMaxNaturalWidth=clutter_actor_box_get_width(largestBox);
		clutter_actor_box_free(largestBox);
	}

	/* Determine minimum sizes */
	if(priv->isVertical)
	{
		minWidth=allMaxMinimumWidth;
		naturalWidth=allMaxNaturalWidth;
	}
		else if(priv->isHomogeneous)
		{
			minWidth=numberChildren*allMaxMinimumWidth;
			minWidth+=(numberChildren>0 ? (numberChildren-1)*priv->spacing : 0.0f);

			naturalWidth=numberChildren*allMaxNaturalWidth;
			naturalWidth+=(numberChildren>0 ? (numberChildren-1)*priv->spacing : 0.0f);
		}
		else
		{
			for(; children; children=children->next)
			{
				ClutterActor			*child=CLUTTER_ACTOR(children->data);
				gfloat					childMinWidth, childNaturalWidth;

				/* Is child visible? */
				if(!CLUTTER_ACTOR_IS_VISIBLE(child)) continue;

				clutter_actor_get_preferred_width(child, -1, &childMinWidth, &childNaturalWidth);
				minWidth+=childMinWidth+priv->spacing;
				naturalWidth+=childNaturalWidth+priv->spacing;
			}

			minWidth-=priv->spacing;
			naturalWidth-=priv->spacing;
		}

	/* Release list of children */
	g_list_free(childrenList);

	/* Return determined sizes */
	if(outMinWidth) *outMinWidth=minWidth;
	if(outNaturalWidth) *outNaturalWidth=naturalWidth;
}

static void xfdashboard_fill_box_layout_get_preferred_height(ClutterLayoutManager *inManager,
																ClutterContainer *inContainer,
																gfloat inForWidth,
																gfloat *outMinHeight,
																gfloat *outNaturalHeight)
{
	g_return_if_fail(XFDASHBOARD_IS_FILL_BOX_LAYOUT(inManager));
	g_return_if_fail(CLUTTER_IS_CONTAINER(inContainer));

	XfdashboardFillBoxLayoutPrivate		*priv=XFDASHBOARD_FILL_BOX_LAYOUT(inManager)->priv;
	GList								*children, *childrenList;
	gint								numberChildren;
	gfloat								minHeight, naturalHeight;
	gfloat								allMaxMinimumHeight, allMaxNaturalHeight;
	ClutterActorBox						*largestBox;

	/* Get children of container */
	childrenList=children=clutter_container_get_children(inContainer);

	/* Set empty sizes */
	minHeight=naturalHeight=0.0f;
	allMaxMinimumHeight=allMaxNaturalHeight=0.0f;

	/* Determine number of children */
	numberChildren=g_list_length(children);

	/* Determine largest width and height among all children */
	largestBox=_xfdashboard_fill_box_layout_get_largest_size(inManager, inContainer, TRUE);
	if(G_LIKELY(largestBox))
	{
		allMaxMinimumHeight=clutter_actor_box_get_height(largestBox);
		clutter_actor_box_free(largestBox);
	}

	largestBox=_xfdashboard_fill_box_layout_get_largest_size(inManager, inContainer, FALSE);
	if(G_LIKELY(largestBox))
	{
		allMaxNaturalHeight=clutter_actor_box_get_height(largestBox);
		clutter_actor_box_free(largestBox);
	}

	/* Determine minimum sizes */
	if(!priv->isVertical)
	{
		minHeight=allMaxMinimumHeight;
		naturalHeight=allMaxNaturalHeight;
	}
		else if(priv->isHomogeneous)
		{
			minHeight=numberChildren*allMaxMinimumHeight;
			minHeight+=(numberChildren>0 ? (numberChildren-1)*priv->spacing : 0.0f);

			naturalHeight=numberChildren*allMaxNaturalHeight;
			naturalHeight+=(numberChildren>0 ? (numberChildren-1)*priv->spacing : 0.0f);
		}
		else
		{
			for(; children; children=children->next)
			{
				ClutterActor			*child=CLUTTER_ACTOR(children->data);
				gfloat					childMinHeight, childNaturalHeight;

				/* Is child visible? */
				if(!CLUTTER_ACTOR_IS_VISIBLE(child)) continue;

				clutter_actor_get_preferred_height(child, -1, &childMinHeight, &childNaturalHeight);
				minHeight+=childMinHeight+priv->spacing;
				naturalHeight+=childNaturalHeight+priv->spacing;
			}

			minHeight-=priv->spacing;
			naturalHeight-=priv->spacing;
		}

	/* Release list of children */
	g_list_free(childrenList);

	/* Return determined sizes */
	if(outMinHeight) *outMinHeight=minHeight;
	if(outNaturalHeight) *outNaturalHeight=naturalHeight;
}

/* Re-layout and allocate children of container we manage */
static void xfdashboard_fill_box_layout_allocate(ClutterLayoutManager *inManager,
													ClutterContainer *inContainer,
													const ClutterActorBox *inBox,
													ClutterAllocationFlags inFlags)
{
	XfdashboardFillBoxLayout			*self=XFDASHBOARD_FILL_BOX_LAYOUT(inManager);
	XfdashboardFillBoxLayoutPrivate		*priv=self->priv;
	gfloat								availableWidth, availableHeight;
	gfloat								largestWidth, largestHeight;
	GList								*children, *childrenList;
	ClutterActorBox						*box;
	ClutterActorBox						childAllocation={ 0, };

	/* Get list of children to layout */
	childrenList=children=clutter_container_get_children(inContainer);

	/* Get available size */
	clutter_actor_box_get_size(inBox, &availableWidth, &availableHeight);

	/* Get largest width and height of all children */
	box=_xfdashboard_fill_box_layout_get_largest_size(inManager, inContainer, FALSE);
	if(G_LIKELY(box))
	{
		largestWidth=clutter_actor_box_get_width(box);
		largestHeight=clutter_actor_box_get_height(box);
		clutter_actor_box_free(box);
	}
	
	/* Iterate through visible children and calculate their position and size */
	clutter_actor_box_set_origin(&childAllocation, 0, 0);
	
	for(; children; children=children->next)
	{
		ClutterActor					*child=CLUTTER_ACTOR(children->data);
		gfloat							childWidth, childHeight;

		/* Is child visible? */
		if(!CLUTTER_ACTOR_IS_VISIBLE(child)) continue;

		/* Get child's preferred sizes */
		clutter_actor_get_preferred_width(child, -1, NULL, &childWidth);
		clutter_actor_get_preferred_height(child, -1, NULL, &childHeight);

		/* Determine child's new size */
		if(priv->isVertical)
		{
			if(priv->isHomogeneous) childHeight=largestHeight;
			childWidth=availableWidth;
		}
			else
			{
				if(priv->isHomogeneous) childWidth=largestWidth;
				childHeight=availableHeight;
			}
			
		/* Set child's allocation */
		childAllocation.x2=childAllocation.x1+childWidth;
		childAllocation.y2=childAllocation.y1+childHeight;

		clutter_actor_allocate(child, &childAllocation, inFlags);

		/* Set up for next child */
		if(priv->isVertical) childAllocation.y1=childAllocation.y2+priv->spacing;
			else childAllocation.x1=childAllocation.x2+priv->spacing;
	}

	/* Release list of children */
	g_list_free(childrenList);
}

/* Set container whose children to layout */
static void xfdashboard_fill_box_layout_set_container(ClutterLayoutManager *inManager,
															ClutterContainer *inContainer)
{
	XfdashboardFillBoxLayoutPrivate		*priv=XFDASHBOARD_FILL_BOX_LAYOUT(inManager)->priv;
	ClutterLayoutManagerClass			*parentClass;

	priv->container=inContainer;
	if(priv->container!=NULL)
	{
		/* We need to change the :request-mode of the container
		 * to match the orientation of this manager
		 */
		ClutterRequestMode				requestMode;

		requestMode=(priv->isVertical ? CLUTTER_REQUEST_HEIGHT_FOR_WIDTH : CLUTTER_REQUEST_WIDTH_FOR_HEIGHT);
		clutter_actor_set_request_mode(CLUTTER_ACTOR(priv->container), requestMode);
	}

	parentClass=CLUTTER_LAYOUT_MANAGER_CLASS(xfdashboard_fill_box_layout_parent_class);
	parentClass->set_container(inManager, inContainer);
}

/* IMPLEMENTATION: GObject */

/* Finally free up all resource of this object as it will be destroyed */
static void xfdashboard_fill_box_layout_finalize(GObject *inObject)
{
	G_OBJECT_CLASS(xfdashboard_fill_box_layout_parent_class)->finalize(inObject);
}

/* Set/get properties */
static void xfdashboard_fill_box_layout_set_property(GObject *inObject,
														guint inPropID,
														const GValue *inValue,
														GParamSpec *inSpec)
{
	XfdashboardFillBoxLayout	*self=XFDASHBOARD_FILL_BOX_LAYOUT(inObject);
	
	switch(inPropID)
	{
		case PROP_VERTICAL:
			xfdashboard_fill_box_layout_set_vertical(self, g_value_get_boolean(inValue));
			break;

		case PROP_HOMOGENOUS:
			xfdashboard_fill_box_layout_set_homogenous(self, g_value_get_boolean(inValue));
			break;

		case PROP_SPACING:
			xfdashboard_fill_box_layout_set_spacing(self, g_value_get_float(inValue));
			break;

		default:
			G_OBJECT_WARN_INVALID_PROPERTY_ID(inObject, inPropID, inSpec);
			break;
	}
}

static void xfdashboard_fill_box_layout_get_property(GObject *inObject,
														guint inPropID,
														GValue *outValue,
														GParamSpec *inSpec)
{
	XfdashboardFillBoxLayout	*self=XFDASHBOARD_FILL_BOX_LAYOUT(inObject);

	switch(inPropID)
	{
		case PROP_VERTICAL:
			g_value_set_boolean(outValue, self->priv->isVertical);
			break;

		case PROP_HOMOGENOUS:
			g_value_set_boolean(outValue, self->priv->isHomogeneous);
			break;

		case PROP_SPACING:
			g_value_set_float(outValue, self->priv->spacing);
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
static void xfdashboard_fill_box_layout_class_init(XfdashboardFillBoxLayoutClass *klass)
{
	ClutterLayoutManagerClass	*layoutClass=CLUTTER_LAYOUT_MANAGER_CLASS(klass);
	GObjectClass				*gobjectClass=G_OBJECT_CLASS(klass);

	/* Override functions */
	gobjectClass->finalize=xfdashboard_fill_box_layout_finalize;
	gobjectClass->set_property=xfdashboard_fill_box_layout_set_property;
	gobjectClass->get_property=xfdashboard_fill_box_layout_get_property;

	layoutClass->get_preferred_width=xfdashboard_fill_box_layout_get_preferred_width;
	layoutClass->get_preferred_height=xfdashboard_fill_box_layout_get_preferred_height;
	layoutClass->allocate=xfdashboard_fill_box_layout_allocate;
	layoutClass->set_container=xfdashboard_fill_box_layout_set_container;

	/* Set up private structure */
	g_type_class_add_private(klass, sizeof(XfdashboardFillBoxLayoutPrivate));

	/* Define properties */
	XfdashboardFillBoxLayoutProperties[PROP_VERTICAL]=
		g_param_spec_boolean("vertical",
								"Vertical",
								"Whether the layout should be vertical, rather than horizontal",
								FALSE,
								G_PARAM_READWRITE | G_PARAM_CONSTRUCT);

	XfdashboardFillBoxLayoutProperties[PROP_HOMOGENOUS]=
		g_param_spec_boolean("homogeneous",
								"Homogeneous",
								"Whether the layout should be homogeneous, i.e. all childs get the same size",
								FALSE,
								G_PARAM_READWRITE | G_PARAM_CONSTRUCT);

	XfdashboardFillBoxLayoutProperties[PROP_SPACING]=
		g_param_spec_float("spacing",
								"Spacing",
								"The spacing between children",
								0.0, G_MAXFLOAT,
								0.0,
								G_PARAM_READWRITE | G_PARAM_CONSTRUCT);

	g_object_class_install_properties(gobjectClass, PROP_LAST, XfdashboardFillBoxLayoutProperties);
}

/* Object initialization
 * Create private structure and set up default values
 */
static void xfdashboard_fill_box_layout_init(XfdashboardFillBoxLayout *self)
{
	XfdashboardFillBoxLayoutPrivate	*priv;

	/* Set default values */
	priv=self->priv=XFDASHBOARD_FILL_BOX_LAYOUT_GET_PRIVATE(self);
	
	priv->isVertical=FALSE;
	priv->isHomogeneous=FALSE;
	priv->spacing=0.0f;
}

/* Implementation: Public API */

ClutterLayoutManager* xfdashboard_fill_box_layout_new()
{
	return(g_object_new(XFDASHBOARD_TYPE_FILL_BOX_LAYOUT, NULL));
}

/* Get/set orientation */
gboolean xfdashboard_fill_box_layout_get_vertical(XfdashboardFillBoxLayout *self)
{
	g_return_val_if_fail(XFDASHBOARD_IS_FILL_BOX_LAYOUT(self), FALSE);

	return(self->priv->isVertical);
}

void xfdashboard_fill_box_layout_set_vertical(XfdashboardFillBoxLayout *self, gboolean inIsVertical)
{
	g_return_if_fail(XFDASHBOARD_IS_FILL_BOX_LAYOUT(self));

	/* Set orientation */
	if(self->priv->isVertical!=inIsVertical)
	{
		self->priv->isVertical=inIsVertical;
		clutter_layout_manager_layout_changed(CLUTTER_LAYOUT_MANAGER(self));
	}
}

/* Get/set homogenous */
gboolean xfdashboard_fill_box_layout_get_homogenous(XfdashboardFillBoxLayout *self)
{
	g_return_val_if_fail(XFDASHBOARD_IS_FILL_BOX_LAYOUT(self), FALSE);

	return(self->priv->isHomogeneous);
}

void xfdashboard_fill_box_layout_set_homogenous(XfdashboardFillBoxLayout *self, gboolean inIsHomogenous)
{
	g_return_if_fail(XFDASHBOARD_IS_FILL_BOX_LAYOUT(self));

	/* Set homogenous */
	if(self->priv->isHomogeneous!=inIsHomogenous)
	{
		self->priv->isHomogeneous=inIsHomogenous;
		clutter_layout_manager_layout_changed(CLUTTER_LAYOUT_MANAGER(self));
	}
}

/* Get/set spacing */
gfloat xfdashboard_fill_box_layout_get_spacing(XfdashboardFillBoxLayout *self)
{
	g_return_val_if_fail(XFDASHBOARD_IS_FILL_BOX_LAYOUT(self), 0.0f);

	return(self->priv->spacing);
}

void xfdashboard_fill_box_layout_set_spacing(XfdashboardFillBoxLayout *self, gfloat inSpacing)
{
	g_return_if_fail(XFDASHBOARD_IS_FILL_BOX_LAYOUT(self));
	g_return_if_fail(inSpacing>=0.0f);

	/* Set spacing */
	if(self->priv->spacing!=inSpacing)
	{
		self->priv->spacing=inSpacing;
		clutter_layout_manager_layout_changed(CLUTTER_LAYOUT_MANAGER(self));
	}
}
