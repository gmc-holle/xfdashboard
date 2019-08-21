/*
 * scaled-table-layout: Layouts children in a dynamic table grid
 *                      (rows and columns are inserted and deleted
 *                      automatically depending on the number of
 *                      child actors) and scaled to fit the allocation
 *                      of the actor holding all child actors.
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

#include <libxfdashboard/scaled-table-layout.h>

#include <glib/gi18n-lib.h>
#include <clutter/clutter.h>
#include <math.h>

#include <libxfdashboard/compat.h>


/* Define this class in GObject system */
struct _XfdashboardScaledTableLayoutPrivate
{
	/* Properties related */
	gfloat		rowSpacing;
	gfloat		columnSpacing;
	gboolean	relativeScale;
	gboolean	preventUpscaling;

	/* Instance related */
	gint		rows;
	gint		columns;
	gint		numberChildren;

	gboolean	reentrantDetermineWidth;
	gboolean	reentrantDetermineHeight;
};

G_DEFINE_TYPE_WITH_PRIVATE(XfdashboardScaledTableLayout,
							xfdashboard_scaled_table_layout,
							CLUTTER_TYPE_LAYOUT_MANAGER)

/* Properties */
enum
{
	PROP_0,

	PROP_ROW_SPACING,
	PROP_COLUMN_SPACING,
	PROP_RELATIVE_SCALE,
	PROP_PREVENT_UPSCALING,

	PROP_NUMBER_CHILDREN,
	PROP_ROWS,
	PROP_COLUMNS,

	PROP_LAST
};

static GParamSpec* XfdashboardScaledTableLayoutProperties[PROP_LAST]={ 0, };

/* IMPLEMENTATION: Private variables and methods */

/* Updates the minimum number of rows and columns needed for layout */
static void _xfdashboard_scaled_table_layout_update_rows_and_columns(XfdashboardScaledTableLayout *self,
																		ClutterContainer *inContainer)
{
	XfdashboardScaledTableLayoutPrivate		*priv;
	ClutterRequestMode						requestMode;
	ClutterActorIter						iter;
	ClutterActor							*child;
	gint									numberChildren;
	gint									rows;
	gint									columns;

	g_return_if_fail(XFDASHBOARD_IS_SCALED_TABLE_LAYOUT(self));
	g_return_if_fail(CLUTTER_IS_CONTAINER(inContainer));
	g_return_if_fail(CLUTTER_IS_ACTOR(inContainer));

	priv=self->priv;

	/* Freeze notification */
	g_object_freeze_notify(G_OBJECT(self));

	/* Get number of visible child actors */
	numberChildren=0;
	clutter_actor_iter_init(&iter, CLUTTER_ACTOR(inContainer));
	while(clutter_actor_iter_next(&iter, &child))
	{
		if(clutter_actor_is_visible(child)) numberChildren++;
	}

	if(numberChildren!=priv->numberChildren)
	{
		priv->numberChildren=numberChildren;
		g_object_notify_by_pspec(G_OBJECT(self), XfdashboardScaledTableLayoutProperties[PROP_NUMBER_CHILDREN]);
	}

	/* Get request mode to determine if more rows than columns are needed
	 * or the opposite
	 */
	requestMode=clutter_actor_get_request_mode(CLUTTER_ACTOR(inContainer));

	/* Calculate and update number of rows and columns */
	if(requestMode==CLUTTER_REQUEST_HEIGHT_FOR_WIDTH)
	{
		rows=ceil(sqrt((double)priv->numberChildren));
		columns=ceil((double)priv->numberChildren / (double)priv->rows);
	}
		else
		{
			columns=ceil(sqrt((double)priv->numberChildren));
			rows=ceil((double)priv->numberChildren / (double)priv->columns);
		}

	if(rows!=priv->rows)
	{
		priv->rows=rows;
		g_object_notify_by_pspec(G_OBJECT(self), XfdashboardScaledTableLayoutProperties[PROP_ROWS]);
	}

	if(columns!=priv->columns)
	{
		priv->columns=columns;
		g_object_notify_by_pspec(G_OBJECT(self), XfdashboardScaledTableLayoutProperties[PROP_COLUMNS]);
	}

	/* Thaw notification */
	g_object_thaw_notify(G_OBJECT(self));
}

/* IMPLEMENTATION: ClutterLayoutManager */

/* Get preferred width/height */
static void _xfdashboard_scaled_table_layout_get_preferred_width(ClutterLayoutManager *self,
																	ClutterContainer *inContainer,
																	gfloat inForHeight,
																	gfloat *outMinWidth,
																	gfloat *outNaturalWidth)
{
	XfdashboardScaledTableLayoutPrivate		*priv;
	gfloat									maxMinWidth, maxNaturalWidth;
	ClutterActor							*parent;

	g_return_if_fail(XFDASHBOARD_IS_SCALED_TABLE_LAYOUT(self));
	g_return_if_fail(CLUTTER_IS_CONTAINER(inContainer));

	priv=XFDASHBOARD_SCALED_TABLE_LAYOUT(self)->priv;

	/* Set up default values */
	maxMinWidth=0.0f;
	maxNaturalWidth=0.0f;

	/* Update number of rows and columns needed for layout */
	_xfdashboard_scaled_table_layout_update_rows_and_columns(XFDASHBOARD_SCALED_TABLE_LAYOUT(self), inContainer);

	/* Get size of parent if this child is parented and if it is not reentrant */
	parent=clutter_actor_get_parent(CLUTTER_ACTOR(inContainer));
	if(parent && !priv->reentrantDetermineWidth)
	{
		/* Prevent getting size of parent (reentrance) */
		priv->reentrantDetermineWidth=TRUE;

		/* Get size of parent */
		clutter_actor_get_size(CLUTTER_ACTOR(parent), &maxNaturalWidth, NULL);

		/* Allow getting size of parent again */
		priv->reentrantDetermineWidth=FALSE;
	}

	/* Calculate width */
	if(priv->columns>0)
	{
		maxMinWidth=(priv->columns-1)*priv->columnSpacing;
		if(maxNaturalWidth==0.0f) maxNaturalWidth=(priv->columns-1)*priv->columnSpacing;
	}

	/* Set return values */
	if(outMinWidth) *outMinWidth=maxMinWidth;
	if(outNaturalWidth) *outNaturalWidth=maxNaturalWidth;
}

static void _xfdashboard_scaled_table_layout_get_preferred_height(ClutterLayoutManager *self,
																	ClutterContainer *inContainer,
																	gfloat inForWidth,
																	gfloat *outMinHeight,
																	gfloat *outNaturalHeight)
{
	XfdashboardScaledTableLayoutPrivate		*priv;
	gfloat									maxMinHeight, maxNaturalHeight;
	ClutterActor							*parent;

	g_return_if_fail(XFDASHBOARD_IS_SCALED_TABLE_LAYOUT(self));
	g_return_if_fail(CLUTTER_IS_CONTAINER(inContainer));

	priv=XFDASHBOARD_SCALED_TABLE_LAYOUT(self)->priv;

	/* Set up default values */
	maxMinHeight=0.0f;
	maxNaturalHeight=0.0f;

	/* Update number of rows and columns needed for layout */
	_xfdashboard_scaled_table_layout_update_rows_and_columns(XFDASHBOARD_SCALED_TABLE_LAYOUT(self), inContainer);

	/* Get size of parent if this child is parented and if it is not reentrant */
	parent=clutter_actor_get_parent(CLUTTER_ACTOR(inContainer));
	if(parent && !priv->reentrantDetermineHeight)
	{
		/* Prevent getting size of parent (reentrance) */
		priv->reentrantDetermineHeight=TRUE;

		/* Get size of parent */
		clutter_actor_get_size(CLUTTER_ACTOR(parent), NULL, &maxNaturalHeight);

		/* Allow getting size of parent again */
		priv->reentrantDetermineHeight=FALSE;
	}

	/* Calculate height */
	if(priv->rows>0)
	{
		maxMinHeight=(priv->rows-1)*priv->rowSpacing;
		if(maxNaturalHeight==0.0f) maxNaturalHeight=(priv->rows-1)*priv->rowSpacing;
	}

	/* Set return values */
	if(outMinHeight) *outMinHeight=maxMinHeight;
	if(outNaturalHeight) *outNaturalHeight=maxNaturalHeight;
}

/* Re-layout and allocate children of container we manage */
static void _xfdashboard_scaled_table_layout_allocate(ClutterLayoutManager *self,
														ClutterContainer *inContainer,
														const ClutterActorBox *inAllocation,
														ClutterAllocationFlags inFlags)
{
	XfdashboardScaledTableLayoutPrivate		*priv;
	gint									row, col;
	ClutterActor							*child;
	ClutterActorIter						iter;
	gfloat									cellWidth, cellHeight;
	gfloat									childWidth, childHeight;
	gfloat									scaledChildWidth, scaledChildHeight;
	gfloat									largestWidth, largestHeight;
	gfloat									scaleWidth, scaleHeight;
	gfloat									aspectRatio;
	gfloat									x, y;
	ClutterActorBox							childAllocation;

	g_return_if_fail(XFDASHBOARD_IS_SCALED_TABLE_LAYOUT(self));
	g_return_if_fail(CLUTTER_IS_CONTAINER(inContainer));

	priv=XFDASHBOARD_SCALED_TABLE_LAYOUT(self)->priv;

	/* Get size of container holding children to layout and
	 * determine size of a cell
	 */
	clutter_actor_get_size(CLUTTER_ACTOR(inContainer), &childWidth, &childHeight);

	cellWidth=childWidth-((priv->columns-1)*priv->columnSpacing);
	cellWidth=floor(cellWidth/priv->columns);

	cellHeight=childHeight-((priv->rows-1)*priv->rowSpacing);
	cellHeight=floor(cellHeight/priv->rows);

	/* Iterate through children and find largest one
	 * if relative scale was set
	 */
	largestWidth=largestHeight=0.0f;
	if(priv->relativeScale==TRUE)
	{
		gfloat								w, h;

		clutter_actor_iter_init(&iter, CLUTTER_ACTOR(inContainer));
		while(clutter_actor_iter_next(&iter, &child))
		{
			if(!clutter_actor_is_visible(child)) continue;

			clutter_actor_get_preferred_size(child, NULL, NULL, &w, &h);
			if(w>largestWidth) largestWidth=w;
			if(h>largestHeight) largestHeight=h;
		}
	}

	/* Iterate through child actors and set their new allocation */
	row=col=0;
	x=y=0.0f;
	clutter_actor_iter_init(&iter, CLUTTER_ACTOR(inContainer));
	while(clutter_actor_iter_next(&iter, &child))
	{
		if(!clutter_actor_is_visible(child)) continue;

		/* Get natural size of actor */
		clutter_actor_get_preferred_size(child, NULL, NULL, &childWidth, &childHeight);

		/* If either width or height is 0 then it is visually hidden and we
		 * skip expensive calculation. This also has the nice effect that
		 * do not perform invalid divisions by zero ;)
		 */
		if(childWidth>0.0f && childHeight>0.0f)
		{
			/* Get scale factor needed to apply to width and height.
			 * If no relative scaling should be performed the scale is always 1.0
			 * otherwise it is the scale factor for this actor to the largest one.
			 */
			if(priv->relativeScale==TRUE)
			{
				/* Get scale factors */
				scaleWidth=childWidth/largestWidth;
				scaleHeight=childHeight/largestHeight;
			}
				else scaleWidth=scaleHeight=1.0f;

			/* Get aspect ratio factor */
			aspectRatio=childHeight/childWidth;

			/* Calculate new size of child */
			scaledChildWidth=cellWidth*scaleWidth;
			scaledChildHeight=scaledChildWidth*aspectRatio;
			if(scaledChildHeight>cellHeight)
			{
				scaledChildHeight=cellHeight*scaleHeight;
				scaledChildWidth=scaledChildHeight/aspectRatio;
			}

			/* If upscaling should be prevent check if we are upscaling now */
			if(priv->preventUpscaling)
			{
				if(scaledChildWidth>childWidth)
				{
					scaledChildWidth=childWidth;
					scaledChildHeight=childWidth*aspectRatio;
				}

				if(scaledChildHeight>childHeight)
				{
					scaledChildHeight=childHeight;
					scaledChildWidth=childHeight/aspectRatio;
				}
			}
		}
			else
			{
				/* Visually hidden so do not allocate any space */
				scaledChildWidth=0.0f;
				scaledChildHeight=0.0f;
			}

		/* Set new allocation of child */
		childAllocation.x1=ceil(x+((cellWidth-scaledChildWidth)/2.0f));
		childAllocation.y1=ceil(y+((cellHeight-scaledChildHeight)/2.0f));
		childAllocation.x2=ceil(childAllocation.x1+scaledChildWidth);
		childAllocation.y2=ceil(childAllocation.y1+scaledChildHeight);
		clutter_actor_allocate(child, &childAllocation, inFlags);

		/* Set up for next child */
		col=(col+1) % priv->columns;
		if(col==0) row++;
		x=col*(cellWidth+priv->columnSpacing);
		y=row*(cellHeight+priv->rowSpacing);
	}
}

/* IMPLEMENTATION: GObject */

/* Set/get properties */
static void _xfdashboard_scaled_table_layout_set_property(GObject *inObject,
															guint inPropID,
															const GValue *inValue,
															GParamSpec *inSpec)
{
	XfdashboardScaledTableLayout			*self=XFDASHBOARD_SCALED_TABLE_LAYOUT(inObject);
	
	switch(inPropID)
	{
		case PROP_ROW_SPACING:
			xfdashboard_scaled_table_layout_set_row_spacing(self, g_value_get_float(inValue));
			break;

		case PROP_COLUMN_SPACING:
			xfdashboard_scaled_table_layout_set_column_spacing(self, g_value_get_float(inValue));
			break;

		case PROP_RELATIVE_SCALE:
			xfdashboard_scaled_table_layout_set_relative_scale(self, g_value_get_boolean(inValue));
			break;

		case PROP_PREVENT_UPSCALING:
			xfdashboard_scaled_table_layout_set_prevent_upscaling(self, g_value_get_boolean(inValue));
			break;

		default:
			G_OBJECT_WARN_INVALID_PROPERTY_ID(inObject, inPropID, inSpec);
			break;
	}
}

static void _xfdashboard_scaled_table_layout_get_property(GObject *inObject,
															guint inPropID,
															GValue *outValue,
															GParamSpec *inSpec)
{
	XfdashboardScaledTableLayout	*self=XFDASHBOARD_SCALED_TABLE_LAYOUT(inObject);

	switch(inPropID)
	{
		case PROP_ROW_SPACING:
			g_value_set_float(outValue, self->priv->rowSpacing);
			break;

		case PROP_COLUMN_SPACING:
			g_value_set_float(outValue, self->priv->columnSpacing);
			break;

		case PROP_RELATIVE_SCALE:
			g_value_set_boolean(outValue, self->priv->relativeScale);
			break;

		case PROP_PREVENT_UPSCALING:
			g_value_set_boolean(outValue, self->priv->preventUpscaling);
			break;

		case PROP_NUMBER_CHILDREN:
			g_value_set_int(outValue, self->priv->numberChildren);
			break;

		case PROP_ROWS:
			g_value_set_int(outValue, self->priv->rows);
			break;

		case PROP_COLUMNS:
			g_value_set_int(outValue, self->priv->columns);
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
static void xfdashboard_scaled_table_layout_class_init(XfdashboardScaledTableLayoutClass *klass)
{
	ClutterLayoutManagerClass	*layoutClass=CLUTTER_LAYOUT_MANAGER_CLASS(klass);
	GObjectClass				*gobjectClass=G_OBJECT_CLASS(klass);

	/* Override functions */
	layoutClass->get_preferred_width=_xfdashboard_scaled_table_layout_get_preferred_width;
	layoutClass->get_preferred_height=_xfdashboard_scaled_table_layout_get_preferred_height;
	layoutClass->allocate=_xfdashboard_scaled_table_layout_allocate;

	gobjectClass->set_property=_xfdashboard_scaled_table_layout_set_property;
	gobjectClass->get_property=_xfdashboard_scaled_table_layout_get_property;

	/* Define properties */
	XfdashboardScaledTableLayoutProperties[PROP_ROW_SPACING]=
		g_param_spec_float("row-spacing",
								_("Row spacing"),
								_("The spacing between rows in table"),
								0.0f,
								G_MAXFLOAT,
								0.0f,
								G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS);

	XfdashboardScaledTableLayoutProperties[PROP_COLUMN_SPACING]=
		g_param_spec_float("column-spacing",
								_("Column spacing"),
								_("The spacing between columns in table"),
								0.0f,
								G_MAXFLOAT,
								0.0f,
								G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS);

	XfdashboardScaledTableLayoutProperties[PROP_RELATIVE_SCALE]=
		g_param_spec_boolean("relative-scale",
								_("Relative scale"),
								_("Whether all children should be scaled relatively to largest child"),
								FALSE,
								G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS);

	XfdashboardScaledTableLayoutProperties[PROP_PREVENT_UPSCALING]=
		g_param_spec_boolean("prevent-upscaling",
								_("Prevent upscaling"),
								_("Whether this layout manager should prevent upsclaing any child beyond its real size"),
								FALSE,
								G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS);

	XfdashboardScaledTableLayoutProperties[PROP_NUMBER_CHILDREN]=
		g_param_spec_float("number-children",
								_("Number children"),
								_("Current number of child actors in this layout"),
								0,
								G_MAXINT,
								0,
								G_PARAM_READABLE | G_PARAM_STATIC_STRINGS);

	XfdashboardScaledTableLayoutProperties[PROP_ROWS]=
		g_param_spec_float("rows",
								_("Rows"),
								_("Current number of rows in this layout"),
								0,
								G_MAXINT,
								0,
								G_PARAM_READABLE | G_PARAM_STATIC_STRINGS);

	XfdashboardScaledTableLayoutProperties[PROP_COLUMNS]=
		g_param_spec_float("columns",
								_("Columns"),
								_("Current number of columns in this layout"),
								0,
								G_MAXINT,
								0,
								G_PARAM_READABLE | G_PARAM_STATIC_STRINGS);
	
	g_object_class_install_properties(gobjectClass, PROP_LAST, XfdashboardScaledTableLayoutProperties);
}

/* Object initialization
 * Create private structure and set up default values
 */
static void xfdashboard_scaled_table_layout_init(XfdashboardScaledTableLayout *self)
{
	XfdashboardScaledTableLayoutPrivate	*priv;

	priv=self->priv=xfdashboard_scaled_table_layout_get_instance_private(self);

	/* Set default values */
	priv->rowSpacing=0.0f;
	priv->columnSpacing=0.0f;
	priv->relativeScale=FALSE;
	priv->preventUpscaling=FALSE;

	priv->rows=0;
	priv->columns=0;
	priv->numberChildren=0;

	priv->reentrantDetermineWidth=FALSE;
	priv->reentrantDetermineHeight=FALSE;
}

/* IMPLEMENTATION: Public API */

/* Create new instance */
ClutterLayoutManager* xfdashboard_scaled_table_layout_new(void)
{
	return(CLUTTER_LAYOUT_MANAGER(g_object_new(XFDASHBOARD_TYPE_SCALED_TABLE_LAYOUT, NULL)));
}

/* Get number of (visible) children which will be layouted */
gint xfdashboard_scaled_table_layout_get_number_children(XfdashboardScaledTableLayout *self)
{
	g_return_val_if_fail(XFDASHBOARD_IS_SCALED_TABLE_LAYOUT(self), 0);

	return(self->priv->numberChildren);
}

/* Get number of rows */
gint xfdashboard_scaled_table_layout_get_rows(XfdashboardScaledTableLayout *self)
{
	g_return_val_if_fail(XFDASHBOARD_IS_SCALED_TABLE_LAYOUT(self), 0);

	return(self->priv->rows);
}

/* Get number of columns */
gint xfdashboard_scaled_table_layout_get_columns(XfdashboardScaledTableLayout *self)
{
	g_return_val_if_fail(XFDASHBOARD_IS_SCALED_TABLE_LAYOUT(self), 0);

	return(self->priv->columns);
}

/* Get/set relative scaling of all children to largest one */
gboolean xfdashboard_scaled_table_layout_get_relative_scale(XfdashboardScaledTableLayout *self)
{
	g_return_val_if_fail(XFDASHBOARD_IS_SCALED_TABLE_LAYOUT(self), FALSE);

	return(self->priv->relativeScale);
}

void xfdashboard_scaled_table_layout_set_relative_scale(XfdashboardScaledTableLayout *self, gboolean inScaling)
{
	XfdashboardScaledTableLayoutPrivate		*priv;

	g_return_if_fail(XFDASHBOARD_IS_SCALED_TABLE_LAYOUT(self));

	priv=self->priv;

	/* Set new value if changed */
	if(priv->relativeScale!=inScaling)
	{
		/* Set new value and notify about property change */
		priv->relativeScale=inScaling;
		g_object_notify_by_pspec(G_OBJECT(self), XfdashboardScaledTableLayoutProperties[PROP_RELATIVE_SCALE]);

		/* Notify for upcoming layout changes */
		clutter_layout_manager_layout_changed(CLUTTER_LAYOUT_MANAGER(self));
	}
}

/* Get/set if layout manager should prevent to size any child larger than its real size */
gboolean xfdashboard_scaled_table_layout_get_prevent_upscaling(XfdashboardScaledTableLayout *self)
{
	g_return_val_if_fail(XFDASHBOARD_IS_SCALED_TABLE_LAYOUT(self), FALSE);

	return(self->priv->preventUpscaling);
}

void xfdashboard_scaled_table_layout_set_prevent_upscaling(XfdashboardScaledTableLayout *self, gboolean inPreventUpscaling)
{
	XfdashboardScaledTableLayoutPrivate		*priv;

	g_return_if_fail(XFDASHBOARD_IS_SCALED_TABLE_LAYOUT(self));

	priv=self->priv;

	/* Set new value if changed */
	if(priv->preventUpscaling!=inPreventUpscaling)
	{
		/* Set new value and notify about property change */
		priv->preventUpscaling=inPreventUpscaling;
		g_object_notify_by_pspec(G_OBJECT(self), XfdashboardScaledTableLayoutProperties[PROP_PREVENT_UPSCALING]);

		/* Notify for upcoming layout changes */
		clutter_layout_manager_layout_changed(CLUTTER_LAYOUT_MANAGER(self));
	}
}

/* Set relative row and column spacing to same value at once */
void xfdashboard_scaled_table_layout_set_spacing(XfdashboardScaledTableLayout *self, gfloat inSpacing)
{
	XfdashboardScaledTableLayoutPrivate		*priv;

	g_return_if_fail(XFDASHBOARD_IS_SCALED_TABLE_LAYOUT(self));
	g_return_if_fail(inSpacing>=0.0f);

	priv=self->priv;

	/* Set new values if changed */
	if(priv->rowSpacing!=inSpacing || priv->columnSpacing!=inSpacing)
	{
		/* Set new values and notify about properties changes */
		priv->rowSpacing=inSpacing;
		g_object_notify_by_pspec(G_OBJECT(self), XfdashboardScaledTableLayoutProperties[PROP_ROW_SPACING]);

		priv->columnSpacing=inSpacing;
		g_object_notify_by_pspec(G_OBJECT(self), XfdashboardScaledTableLayoutProperties[PROP_COLUMN_SPACING]);

		/* Notify for upcoming layout changes */
		clutter_layout_manager_layout_changed(CLUTTER_LAYOUT_MANAGER(self));
	}
}

/* Get/set row spacing */
gfloat xfdashboard_scaled_table_layout_get_row_spacing(XfdashboardScaledTableLayout *self)
{
	g_return_val_if_fail(XFDASHBOARD_IS_SCALED_TABLE_LAYOUT(self), 0.0f);

	return(self->priv->rowSpacing);
}

void xfdashboard_scaled_table_layout_set_row_spacing(XfdashboardScaledTableLayout *self, gfloat inSpacing)
{
	XfdashboardScaledTableLayoutPrivate		*priv;

	g_return_if_fail(XFDASHBOARD_IS_SCALED_TABLE_LAYOUT(self));
	g_return_if_fail(inSpacing>=0.0f);

	priv=self->priv;

	/* Set new value if changed */
	if(priv->rowSpacing!=inSpacing)
	{
		/* Set new value and notify about property change */
		priv->rowSpacing=inSpacing;
		g_object_notify_by_pspec(G_OBJECT(self), XfdashboardScaledTableLayoutProperties[PROP_ROW_SPACING]);

		/* Notify for upcoming layout changes */
		clutter_layout_manager_layout_changed(CLUTTER_LAYOUT_MANAGER(self));
	}
}

/* Get/set columns spacing */
gfloat xfdashboard_scaled_table_layout_get_column_spacing(XfdashboardScaledTableLayout *self)
{
	g_return_val_if_fail(XFDASHBOARD_IS_SCALED_TABLE_LAYOUT(self), 0.0f);

	return(self->priv->columnSpacing);
}

void xfdashboard_scaled_table_layout_set_column_spacing(XfdashboardScaledTableLayout *self, gfloat inSpacing)
{
	XfdashboardScaledTableLayoutPrivate		*priv;

	g_return_if_fail(XFDASHBOARD_IS_SCALED_TABLE_LAYOUT(self));
	g_return_if_fail(inSpacing>=0.0f);

	priv=self->priv;

	/* Set new value if changed */
	if(priv->columnSpacing!=inSpacing)
	{
		/* Set new value and notify about property change */
		priv->columnSpacing=inSpacing;
		g_object_notify_by_pspec(G_OBJECT(self), XfdashboardScaledTableLayoutProperties[PROP_COLUMN_SPACING]);

		/* Notify for upcoming layout changes */
		clutter_layout_manager_layout_changed(CLUTTER_LAYOUT_MANAGER(self));
	}
}
