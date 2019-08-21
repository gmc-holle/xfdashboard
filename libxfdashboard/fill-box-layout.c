/*
 * fill-box-layout: A box layout expanding actors in one direction
 *                  (fill to fit parent's size) and using natural
 *                  size in other direction
 * 
 * Copyright 2012-2019 Stephan Haller <nomad@froevel.de>
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

#include <libxfdashboard/fill-box-layout.h>

#include <glib/gi18n-lib.h>
#include <clutter/clutter.h>
#include <math.h>

#include <libxfdashboard/compat.h>


/* Define this class in GObject system */
struct _XfdashboardFillBoxLayoutPrivate
{
	/* Properties related */
	ClutterOrientation	orientation;
	gfloat				spacing;
	gboolean			isHomogeneous;
	gboolean			keepAspect;
};

G_DEFINE_TYPE_WITH_PRIVATE(XfdashboardFillBoxLayout,
							xfdashboard_fill_box_layout,
							CLUTTER_TYPE_LAYOUT_MANAGER)

/* Properties */
enum
{
	PROP_0,

	PROP_ORIENTATION,
	PROP_SPACING,
	PROP_HOMOGENEOUS,
	PROP_KEEP_ASPECT,

	PROP_LAST
};

static GParamSpec* XfdashboardFillBoxLayoutProperties[PROP_LAST]={ 0, };

/* IMPLEMENTATION: Private variables and methods */

/* Get largest minimum and natural size of all visible children
 * for calculation of one child and returns the number of visible ones
 */
static gint _xfdashboard_fill_box_layout_get_largest_sizes(XfdashboardFillBoxLayout *self,
															ClutterContainer *inContainer,
															gfloat *outMinWidth,
															gfloat *outNaturalWidth,
															gfloat *outMinHeight,
															gfloat *outNaturalHeight)
{
	XfdashboardFillBoxLayoutPrivate		*priv;
	ClutterActor						*child;
	ClutterActorIter					iter;
	gint								numberChildren;
	gfloat								largestMinWidth, largestNaturalWidth;
	gfloat								largestMinHeight, largestNaturalHeight;
	gfloat								childMinWidth, childNaturalWidth;
	gfloat								childMinHeight, childNaturalHeight;
	ClutterActor						*parent;
	gfloat								parentWidth, parentHeight;
	gfloat								aspectRatio;

	g_return_val_if_fail(XFDASHBOARD_IS_FILL_BOX_LAYOUT(self), 0);
	g_return_val_if_fail(CLUTTER_IS_CONTAINER(inContainer), 0);
	g_return_val_if_fail(CLUTTER_IS_ACTOR(inContainer), 0);

	priv=self->priv;

	/* Iterate through all children and determine sizes */
	numberChildren=0;
	largestMinWidth=largestNaturalWidth=largestMinHeight=largestNaturalHeight=0.0f;

	clutter_actor_iter_init(&iter, CLUTTER_ACTOR(inContainer));
	while(clutter_actor_iter_next(&iter, &child))
	{
		/* Only check visible children */
		if(!clutter_actor_is_visible(child)) continue;

		/* Check for largest size */
		clutter_actor_get_preferred_size(child,
											&childMinWidth, &childNaturalWidth,
											&childMinHeight, &childNaturalHeight);

		if(childMinWidth>largestMinWidth) largestMinWidth=childMinWidth;
		if(childNaturalWidth>largestNaturalWidth) largestNaturalWidth=childNaturalWidth;
		if(childMinHeight>largestMinHeight) largestMinHeight=childMinHeight;
		if(childNaturalHeight>largestNaturalHeight) largestNaturalHeight=childNaturalHeight;

		/* Count visible children */
		numberChildren++;
	}

	/* Depending on orientation set sizes to fit into parent actor */
	parent=clutter_actor_get_parent(CLUTTER_ACTOR(inContainer));
	if(parent)
	{
		aspectRatio=1.0f;

		clutter_actor_get_size(CLUTTER_ACTOR(parent), &parentWidth, &parentHeight);
		if(priv->orientation==CLUTTER_ORIENTATION_HORIZONTAL)
		{
			if(priv->keepAspect==TRUE)
			{
				aspectRatio=largestMinWidth/largestMinHeight;
				largestMinHeight=parentHeight;
				largestMinWidth=largestMinHeight*aspectRatio;

				aspectRatio=largestNaturalWidth/largestNaturalHeight;
				largestNaturalHeight=parentHeight;
				largestNaturalWidth=largestNaturalHeight*aspectRatio;
			}
				else
				{
					largestMinHeight=parentHeight;
					largestNaturalHeight=parentHeight;
				}
		}
			else
			{
				if(priv->keepAspect==TRUE)
				{
					aspectRatio=largestMinHeight/largestMinWidth;
					largestMinWidth=parentWidth;
					largestMinHeight=largestMinWidth*aspectRatio;

					aspectRatio=largestNaturalHeight/largestNaturalWidth;
					largestNaturalWidth=parentWidth;
					largestNaturalHeight=largestNaturalWidth*aspectRatio;
				}
					else
					{
						largestMinWidth=parentWidth;
						largestNaturalWidth=parentWidth;
					}
			}
	}

	/* Set return values */
	if(outMinWidth) *outMinWidth=largestMinWidth;
	if(outNaturalWidth) *outNaturalWidth=largestNaturalWidth;
	if(outMinHeight) *outMinHeight=largestMinHeight;
	if(outNaturalHeight) *outNaturalHeight=largestNaturalHeight;

	/* Return number of visible children */
	return(numberChildren);
}

/* Get minimum and natural size of all visible children */
static void _xfdashboard_fill_box_layout_get_sizes_for_all(XfdashboardFillBoxLayout *self,
															ClutterContainer *inContainer,
															gfloat *outMinWidth,
															gfloat *outNaturalWidth,
															gfloat *outMinHeight,
															gfloat *outNaturalHeight)
{
	XfdashboardFillBoxLayoutPrivate		*priv;
	ClutterActor						*child;
	ClutterActorIter					iter;
	gint								numberChildren;
	gfloat								minWidth, naturalWidth;
	gfloat								minHeight, naturalHeight;
	gfloat								childMinWidth, childNaturalWidth;
	gfloat								childMinHeight, childNaturalHeight;
	ClutterActor						*parent;
	gfloat								parentWidth, parentHeight;
	gfloat								aspectRatio;

	g_return_if_fail(XFDASHBOARD_IS_FILL_BOX_LAYOUT(self));
	g_return_if_fail(CLUTTER_IS_CONTAINER(inContainer));
	g_return_if_fail(CLUTTER_IS_ACTOR(inContainer));

	priv=self->priv;

	/* Initialize return values */
	numberChildren=0;
	minWidth=naturalWidth=minHeight=naturalHeight=0.0f;

	/* If not homogeneous then iterate through all children and determine sizes ... */
	if(priv->isHomogeneous==FALSE)
	{
		/* Iterate through children and calculate sizes */
		clutter_actor_iter_init(&iter, CLUTTER_ACTOR(inContainer));
		while(clutter_actor_iter_next(&iter, &child))
		{
			/* Only get sizes of visible children */
			if(!clutter_actor_is_visible(child)) continue;

			/* Count visible children */
			numberChildren++;

			/* Determine sizes of visible child */
			clutter_actor_get_preferred_size(child,
												&childMinWidth, &childNaturalWidth,
												&childMinHeight, &childNaturalHeight);

			if(priv->orientation==CLUTTER_ORIENTATION_HORIZONTAL)
			{
				minWidth+=childMinWidth;
				naturalWidth+=childNaturalWidth;
				if(childMinHeight>minHeight) minHeight=childMinHeight;
				if(childNaturalHeight>naturalHeight) naturalHeight=childNaturalHeight;
			}
				else
				{
					minHeight+=childMinHeight;
					naturalHeight+=childNaturalHeight;
					if(childMinWidth>naturalWidth) minWidth=naturalWidth;
					if(childNaturalWidth>naturalHeight) naturalHeight=childNaturalWidth;
				}
		}
	}
		/* ... otherwise get largest minimum and natural size and add spacing */
		else
		{
			/* Get number of visible children and also largest minimum
			 * and natural size
			 */
			numberChildren=_xfdashboard_fill_box_layout_get_largest_sizes(self,
																			inContainer,
																			&childMinWidth, &childNaturalWidth,
																			&childMinHeight, &childNaturalHeight);

			/* Multiply largest sizes with number visible children */
			if(priv->orientation==CLUTTER_ORIENTATION_HORIZONTAL)
			{
				minWidth=(numberChildren*childMinWidth);
				naturalWidth=(numberChildren*childNaturalWidth);

				minHeight=childMinHeight;
				naturalHeight=childNaturalHeight;
			}
				else
				{
					minWidth=childMinWidth;
					naturalWidth=childNaturalWidth;

					minHeight=(numberChildren*childMinHeight);
					naturalHeight=(numberChildren*childNaturalHeight);
				}
		}

	/* Add spacing */
	if(numberChildren>0)
	{
		numberChildren--;
		if(priv->orientation==CLUTTER_ORIENTATION_HORIZONTAL)
		{
			minWidth+=numberChildren*priv->spacing;
			naturalWidth+=numberChildren*priv->spacing;
		}
			else
			{
				minHeight+=numberChildren*priv->spacing;
				naturalHeight+=numberChildren*priv->spacing;
			}
	}

	/* Depending on orientation set sizes to fit into parent actor */
	parent=clutter_actor_get_parent(CLUTTER_ACTOR(inContainer));
	if(parent)
	{
		aspectRatio=1.0f;

		clutter_actor_get_size(CLUTTER_ACTOR(parent), &parentWidth, &parentHeight);
		if(priv->orientation==CLUTTER_ORIENTATION_HORIZONTAL)
		{
			if(priv->keepAspect==TRUE)
			{
				aspectRatio=minWidth/minHeight;
				minHeight=parentHeight;
				minWidth=minHeight*aspectRatio;

				aspectRatio=naturalWidth/naturalHeight;
				naturalHeight=parentHeight;
				naturalWidth=naturalHeight*aspectRatio;
			}
				else
				{
					minHeight=parentHeight;
					naturalHeight=parentHeight;
				}
		}
			else
			{
				if(priv->keepAspect==TRUE)
				{
					aspectRatio=minHeight/minWidth;
					minWidth=parentWidth;
					minHeight=minWidth*aspectRatio;

					aspectRatio=naturalHeight/naturalWidth;
					naturalWidth=parentWidth;
					naturalHeight=naturalWidth*aspectRatio;
				}
					else
					{
						minWidth=parentWidth;
						naturalWidth=parentWidth;
					}
			}
	}

	/* Set return values */
	if(outMinWidth) *outMinWidth=minWidth;
	if(outNaturalWidth) *outNaturalWidth=naturalWidth;
	if(outMinHeight) *outMinHeight=minHeight;
	if(outNaturalHeight) *outNaturalHeight=naturalHeight;
}

/* IMPLEMENTATION: ClutterLayoutManager */

/* Get preferred width/height */
static void _xfdashboard_fill_box_layout_get_preferred_width(ClutterLayoutManager *inLayoutManager,
																ClutterContainer *inContainer,
																gfloat inForHeight,
																gfloat *outMinWidth,
																gfloat *outNaturalWidth)
{
	XfdashboardFillBoxLayout			*self;
	gfloat								maxMinWidth, maxNaturalWidth;

	g_return_if_fail(XFDASHBOARD_IS_FILL_BOX_LAYOUT(inLayoutManager));
	g_return_if_fail(CLUTTER_IS_CONTAINER(inContainer));

	self=XFDASHBOARD_FILL_BOX_LAYOUT(inLayoutManager);

	/* Set up default values */
	maxMinWidth=0.0f;
	maxNaturalWidth=0.0f;

	/* Get sizes */
	_xfdashboard_fill_box_layout_get_sizes_for_all(self, inContainer, &maxMinWidth, &maxNaturalWidth, NULL, NULL);

	/* Set return values */
	if(outMinWidth) *outMinWidth=maxMinWidth;
	if(outNaturalWidth) *outNaturalWidth=maxNaturalWidth;
}

static void _xfdashboard_fill_box_layout_get_preferred_height(ClutterLayoutManager *inLayoutManager,
																ClutterContainer *inContainer,
																gfloat inForWidth,
																gfloat *outMinHeight,
																gfloat *outNaturalHeight)
{
	XfdashboardFillBoxLayout			*self;
	gfloat								maxMinHeight, maxNaturalHeight;

	g_return_if_fail(XFDASHBOARD_IS_FILL_BOX_LAYOUT(inLayoutManager));
	g_return_if_fail(CLUTTER_IS_CONTAINER(inContainer));

	self=XFDASHBOARD_FILL_BOX_LAYOUT(inLayoutManager);

	/* Set up default values */
	maxMinHeight=0.0f;
	maxNaturalHeight=0.0f;

	/* Get sizes */
	_xfdashboard_fill_box_layout_get_sizes_for_all(self, inContainer, NULL, NULL, &maxMinHeight, &maxNaturalHeight);

	/* Set return values */
	if(outMinHeight) *outMinHeight=maxMinHeight;
	if(outNaturalHeight) *outNaturalHeight=maxNaturalHeight;
}

/* Re-layout and allocate children of container we manage */
static void _xfdashboard_fill_box_layout_allocate(ClutterLayoutManager *inLayoutManager,
													ClutterContainer *inContainer,
													const ClutterActorBox *inAllocation,
													ClutterAllocationFlags inFlags)
{
	XfdashboardFillBoxLayout			*self;
	XfdashboardFillBoxLayoutPrivate		*priv;
	ClutterActor						*child;
	ClutterActorIter					iter;
	gfloat								parentWidth, parentHeight;
	gfloat								homogeneousSize;
	gfloat								x, y, w, h;
	gfloat								aspectRatio;
	ClutterActorBox						childAllocation;

	g_return_if_fail(XFDASHBOARD_IS_FILL_BOX_LAYOUT(inLayoutManager));
	g_return_if_fail(CLUTTER_IS_CONTAINER(inContainer));

	self=XFDASHBOARD_FILL_BOX_LAYOUT(inLayoutManager);
	priv=self->priv;

	/* Get dimension of allocation */
	parentWidth=clutter_actor_box_get_width(inAllocation);
	parentHeight=clutter_actor_box_get_height(inAllocation);

	/* If homogeneous determine sizes for all children */
	if(priv->isHomogeneous==TRUE)
	{
		if(priv->orientation==CLUTTER_ORIENTATION_HORIZONTAL)
		{
			_xfdashboard_fill_box_layout_get_largest_sizes(self, inContainer, NULL, &homogeneousSize, NULL, NULL);
		}
			else
			{
				_xfdashboard_fill_box_layout_get_largest_sizes(self, inContainer, NULL, NULL, NULL, &homogeneousSize);
			}
	}

	/* Iterate through children and set their sizes */
	x=y=0.0f;

	clutter_actor_iter_init(&iter, CLUTTER_ACTOR(inContainer));
	while(clutter_actor_iter_next(&iter, &child))
	{
		/* Only set sizes on visible children */
		if(!clutter_actor_is_visible(child)) continue;

		/* Calculate and set new allocation of child */
		if(priv->isHomogeneous==FALSE)
		{
			clutter_actor_get_size(CLUTTER_ACTOR(inContainer), &w, &h);
			if(priv->orientation==CLUTTER_ORIENTATION_HORIZONTAL)
			{
				aspectRatio=w/h;
				h=parentHeight;
				w=h*aspectRatio;
			}
				else
				{
					aspectRatio=h/w;
					w=parentWidth;
					h=w*aspectRatio;
				}
		}
			else
			{
				if(priv->orientation==CLUTTER_ORIENTATION_HORIZONTAL)
				{
					w=homogeneousSize;
					h=parentHeight;
				}
					else
					{
						w=parentWidth;
						h=homogeneousSize;
					}
			}

		/* Set new allocation of child */
		childAllocation.x1=ceil(x);
		childAllocation.y1=ceil(y);
		childAllocation.x2=ceil(childAllocation.x1+w);
		childAllocation.y2=ceil(childAllocation.y1+h);
		clutter_actor_allocate(child, &childAllocation, inFlags);

		/* Set up for next child */
		if(priv->orientation==CLUTTER_ORIENTATION_HORIZONTAL) x+=(w+priv->spacing);
			else y+=(h+priv->spacing);
	}
}

/* IMPLEMENTATION: GObject */

/* Set/get properties */
static void _xfdashboard_fill_box_layout_set_property(GObject *inObject,
														guint inPropID,
														const GValue *inValue,
														GParamSpec *inSpec)
{
	XfdashboardFillBoxLayout			*self=XFDASHBOARD_FILL_BOX_LAYOUT(inObject);
	
	switch(inPropID)
	{
		case PROP_ORIENTATION:
			xfdashboard_fill_box_layout_set_orientation(self, g_value_get_enum(inValue));
			break;

		case PROP_SPACING:
			xfdashboard_fill_box_layout_set_spacing(self, g_value_get_float(inValue));
			break;

		case PROP_HOMOGENEOUS:
			xfdashboard_fill_box_layout_set_homogeneous(self, g_value_get_boolean(inValue));
			break;

		case PROP_KEEP_ASPECT:
			xfdashboard_fill_box_layout_set_keep_aspect(self, g_value_get_boolean(inValue));
			break;

		default:
			G_OBJECT_WARN_INVALID_PROPERTY_ID(inObject, inPropID, inSpec);
			break;
	}
}

static void _xfdashboard_fill_box_layout_get_property(GObject *inObject,
														guint inPropID,
														GValue *outValue,
														GParamSpec *inSpec)
{
	XfdashboardFillBoxLayout	*self=XFDASHBOARD_FILL_BOX_LAYOUT(inObject);

	switch(inPropID)
	{
		case PROP_ORIENTATION:
			g_value_set_enum(outValue, self->priv->orientation);
			break;

		case PROP_SPACING:
			g_value_set_float(outValue, self->priv->spacing);
			break;

		case PROP_HOMOGENEOUS:
			g_value_set_boolean(outValue, self->priv->isHomogeneous);
			break;

		case PROP_KEEP_ASPECT:
			g_value_set_boolean(outValue, self->priv->keepAspect);
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
	layoutClass->get_preferred_width=_xfdashboard_fill_box_layout_get_preferred_width;
	layoutClass->get_preferred_height=_xfdashboard_fill_box_layout_get_preferred_height;
	layoutClass->allocate=_xfdashboard_fill_box_layout_allocate;

	gobjectClass->set_property=_xfdashboard_fill_box_layout_set_property;
	gobjectClass->get_property=_xfdashboard_fill_box_layout_get_property;

	/* Define properties */
	XfdashboardFillBoxLayoutProperties[PROP_ORIENTATION]=
		g_param_spec_enum("orientation",
							_("Orientation"),
							_("The orientation to layout children"),
							CLUTTER_TYPE_ORIENTATION,
							CLUTTER_ORIENTATION_HORIZONTAL,
							G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS);

	XfdashboardFillBoxLayoutProperties[PROP_SPACING]=
		g_param_spec_float("spacing",
								_("spacing"),
								_("The spacing between children"),
								0.0f,
								G_MAXFLOAT,
								0.0f,
								G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS);

	XfdashboardFillBoxLayoutProperties[PROP_HOMOGENEOUS]=
		g_param_spec_boolean("homogeneous",
								_("Homogeneous"),
								_("Whether the layout should be homogeneous, i.e. all children get the same size"),
								FALSE,
								G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS | G_PARAM_CONSTRUCT);

	XfdashboardFillBoxLayoutProperties[PROP_KEEP_ASPECT]=
		g_param_spec_boolean("keep-aspect",
								_("Keep aspect"),
								_("Whether all children should keep their aspect"),
								FALSE,
								G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS);

	g_object_class_install_properties(gobjectClass, PROP_LAST, XfdashboardFillBoxLayoutProperties);
}

/* Object initialization
 * Create private structure and set up default values
 */
static void xfdashboard_fill_box_layout_init(XfdashboardFillBoxLayout *self)
{
	XfdashboardFillBoxLayoutPrivate	*priv;

	priv=self->priv=xfdashboard_fill_box_layout_get_instance_private(self);

	/* Set default values */
	priv->orientation=CLUTTER_ORIENTATION_HORIZONTAL;
	priv->spacing=0.0f;
	priv->isHomogeneous=FALSE;
	priv->keepAspect=FALSE;
}

/* IMPLEMENTATION: Public API */

/* Create new instance */
ClutterLayoutManager* xfdashboard_fill_box_layout_new(void)
{
	return(CLUTTER_LAYOUT_MANAGER(g_object_new(XFDASHBOARD_TYPE_FILL_BOX_LAYOUT, NULL)));
}

ClutterLayoutManager* xfdashboard_fill_box_layout_new_with_orientation(ClutterOrientation inOrientation)
{
	return(CLUTTER_LAYOUT_MANAGER(g_object_new(XFDASHBOARD_TYPE_FILL_BOX_LAYOUT,
												"orientation", inOrientation,
												NULL)));
}

/* Get/set orientation */
ClutterOrientation xfdashboard_fill_box_layout_get_orientation(XfdashboardFillBoxLayout *self)
{
	g_return_val_if_fail(XFDASHBOARD_IS_FILL_BOX_LAYOUT(self), CLUTTER_ORIENTATION_HORIZONTAL);

	return(self->priv->orientation);
}

void xfdashboard_fill_box_layout_set_orientation(XfdashboardFillBoxLayout *self, ClutterOrientation inOrientation)
{
	XfdashboardFillBoxLayoutPrivate		*priv;

	g_return_if_fail(XFDASHBOARD_IS_FILL_BOX_LAYOUT(self));
	g_return_if_fail(inOrientation==CLUTTER_ORIENTATION_HORIZONTAL ||
						inOrientation==CLUTTER_ORIENTATION_VERTICAL);

	priv=self->priv;

	/* Set new values if changed */
	if(priv->orientation!=inOrientation)
	{
		/* Set new values and notify about properties changes */
		priv->orientation=inOrientation;
		g_object_notify_by_pspec(G_OBJECT(self), XfdashboardFillBoxLayoutProperties[PROP_ORIENTATION]);

		/* Notify for upcoming layout changes */
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
	XfdashboardFillBoxLayoutPrivate		*priv;

	g_return_if_fail(XFDASHBOARD_IS_FILL_BOX_LAYOUT(self));
	g_return_if_fail(inSpacing>=0.0f);

	priv=self->priv;

	/* Set new values if changed */
	if(priv->spacing!=inSpacing)
	{
		/* Set new values and notify about properties changes */
		priv->spacing=inSpacing;
		g_object_notify_by_pspec(G_OBJECT(self), XfdashboardFillBoxLayoutProperties[PROP_SPACING]);

		/* Notify for upcoming layout changes */
		clutter_layout_manager_layout_changed(CLUTTER_LAYOUT_MANAGER(self));
	}
}

/* Get/set homogenous */
gboolean xfdashboard_fill_box_layout_get_homogeneous(XfdashboardFillBoxLayout *self)
{
	g_return_val_if_fail(XFDASHBOARD_IS_FILL_BOX_LAYOUT(self), FALSE);

	return(self->priv->isHomogeneous);
}

void xfdashboard_fill_box_layout_set_homogeneous(XfdashboardFillBoxLayout *self, gboolean inIsHomogeneous)
{
	XfdashboardFillBoxLayoutPrivate		*priv;

	g_return_if_fail(XFDASHBOARD_IS_FILL_BOX_LAYOUT(self));

	priv=self->priv;

	/* Set new values if changed */
	if(priv->isHomogeneous!=inIsHomogeneous)
	{
		/* Set new values and notify about properties changes */
		priv->isHomogeneous=inIsHomogeneous;
		g_object_notify_by_pspec(G_OBJECT(self), XfdashboardFillBoxLayoutProperties[PROP_HOMOGENEOUS]);

		/* Notify for upcoming layout changes */
		clutter_layout_manager_layout_changed(CLUTTER_LAYOUT_MANAGER(self));
	}
}

/* Get/set keep aspect ratio */
gboolean xfdashboard_fill_box_layout_get_keep_aspect(XfdashboardFillBoxLayout *self)
{
	g_return_val_if_fail(XFDASHBOARD_IS_FILL_BOX_LAYOUT(self), FALSE);

	return(self->priv->keepAspect);
}

void xfdashboard_fill_box_layout_set_keep_aspect(XfdashboardFillBoxLayout *self, gboolean inKeepAspect)
{
	XfdashboardFillBoxLayoutPrivate		*priv;

	g_return_if_fail(XFDASHBOARD_IS_FILL_BOX_LAYOUT(self));

	priv=self->priv;

	/* Set new values if changed */
	if(priv->keepAspect!=inKeepAspect)
	{
		/* Set new values and notify about properties changes */
		priv->keepAspect=inKeepAspect;
		g_object_notify_by_pspec(G_OBJECT(self), XfdashboardFillBoxLayoutProperties[PROP_KEEP_ASPECT]);

		/* Notify for upcoming layout changes */
		clutter_layout_manager_layout_changed(CLUTTER_LAYOUT_MANAGER(self));
	}
}
