/*
 * dynamic-table-layout: Layouts children in a dynamic table grid
 *                       (rows and columns are inserted and deleted
 *                       automatically depending on the number of
 *                       child actors).
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

#include <libxfdashboard/dynamic-table-layout.h>

#include <glib/gi18n-lib.h>
#include <clutter/clutter.h>
#include <math.h>

#include <libxfdashboard/stylable.h>
#include <libxfdashboard/compat.h>


/* Define this class in GObject system */
static void _xfdashboard_dynamic_table_layout_stylable_iface_init(XfdashboardStylableInterface *inInterface);

struct _XfdashboardDynamicTableLayoutPrivate
{
	/* Properties related */
	gfloat				rowSpacing;
	gfloat				columnSpacing;

	/* Instance related */
	gint				numberChildren;
	gint				rows;
	gint				columns;
	gint				fixedColumns;

	GArray				*columnCoords;
	GArray				*rowCoords;

	gpointer			container;
	guint				styleRevalidationSignalID;
};

G_DEFINE_TYPE_WITH_CODE(XfdashboardDynamicTableLayout,
						xfdashboard_dynamic_table_layout,
						CLUTTER_TYPE_LAYOUT_MANAGER,
						G_ADD_PRIVATE(XfdashboardDynamicTableLayout)
						G_IMPLEMENT_INTERFACE(XFDASHBOARD_TYPE_STYLABLE, _xfdashboard_dynamic_table_layout_stylable_iface_init));

/* Properties */
enum
{
	PROP_0,

	PROP_ROW_SPACING,
	PROP_COLUMN_SPACING,

	PROP_NUMBER_CHILDREN,
	PROP_ROWS,
	PROP_COLUMNS,
	PROP_FIXED_COLUMNS,

	/* From interface: XfdashboardStylable */
	PROP_STYLE_CLASSES,
	PROP_STYLE_PSEUDO_CLASSES,

	PROP_LAST
};

static GParamSpec* XfdashboardDynamicTableLayoutProperties[PROP_LAST]={ 0, };

/* IMPLEMENTATION: Private variables and methods */

/* Updates data needed for layout in dynamic mode (dynamic number of columns) */
static void _xfdashboard_dynamic_table_layout_update_layout_data_dynamic(XfdashboardDynamicTableLayout *self,
																			ClutterContainer *inContainer,
																			gfloat inWidth,
																			gfloat inHeight)
{
	XfdashboardDynamicTableLayoutPrivate		*priv;
	ClutterActorIter							iter;
	ClutterActor								*child;
	gint										numberChildren;
	gint										rows, columns;
	gfloat										childWidth, childHeight;
	gfloat										largestWidth, largestHeight;
	gint										i;
	gfloat										x, y;
	ClutterRequestMode							requestMode;

	g_return_if_fail(XFDASHBOARD_IS_DYNAMIC_TABLE_LAYOUT(self));
	g_return_if_fail(CLUTTER_IS_CONTAINER(inContainer));
	g_return_if_fail(CLUTTER_IS_ACTOR(inContainer));

	priv=self->priv;

	/* Freeze notification */
	g_object_freeze_notify(G_OBJECT(self));

	/* Step one: Get number of visible child actors and determine largest width
	 * and height of all visible child actors' natural size.
	 */
	numberChildren=0;
	largestWidth=largestHeight=0.0f;
	clutter_actor_iter_init(&iter, CLUTTER_ACTOR(inContainer));
	while(clutter_actor_iter_next(&iter, &child))
	{
		/* Handle only visible actors */
		if(clutter_actor_is_visible(child))
		{
			/* Count visible children */
			numberChildren++;

			/* Check if either width and/or height of child is the largest one */
			clutter_actor_get_preferred_size(child, NULL, NULL, &childWidth, &childHeight);
			largestWidth=MAX(largestWidth, childWidth);
			largestHeight=MAX(largestHeight, childHeight);
		}
	}

	if(numberChildren!=priv->numberChildren)
	{
		priv->numberChildren=numberChildren;
		g_object_notify_by_pspec(G_OBJECT(self), XfdashboardDynamicTableLayoutProperties[PROP_NUMBER_CHILDREN]);
	}

	/* Step two: Get request mode of container to layout. The request will be
	 *           overriden if any of given width or height is not set (< 0)
	 */
	requestMode=clutter_actor_get_request_mode(CLUTTER_ACTOR(inContainer));

	if(inWidth<0.0f) requestMode=CLUTTER_REQUEST_WIDTH_FOR_HEIGHT;
		else if(inHeight<0.0f) requestMode=CLUTTER_REQUEST_HEIGHT_FOR_WIDTH;

	/* Step three: Determine number of rows and columns */
	rows=columns=0;
	if(inWidth<0.0f && inHeight<0.0f)
	{
		columns=priv->numberChildren;
		rows=1;
	}
		else if(requestMode==CLUTTER_REQUEST_HEIGHT_FOR_WIDTH)
		{
			columns=MIN(ceil(inWidth/largestWidth), priv->numberChildren)+1;
			do
			{
				columns--;
				childWidth=(columns*largestWidth)+((columns-1)*priv->columnSpacing);
			}
			while(columns>1 && childWidth>inWidth);

			largestWidth=floor(inWidth-((columns-1)*priv->columnSpacing))/columns;
			rows=ceil((double)priv->numberChildren / (double)columns);
		}
		else if(requestMode==CLUTTER_REQUEST_WIDTH_FOR_HEIGHT)
		{
			rows=MIN(ceil(inHeight/largestHeight), priv->numberChildren)+1;
			do
			{
				rows--;
				childHeight=(rows*largestHeight)+((rows-1)*priv->rowSpacing);
			}
			while(rows>1 && childHeight>inHeight);

			largestHeight=floor(inHeight-((rows-1)*priv->rowSpacing))/rows;
			columns=ceil((double)priv->numberChildren / (double)rows);
		}
		else
		{
			g_assert_not_reached();
		}

	if(rows!=priv->rows)
	{
		priv->rows=rows;
		g_object_notify_by_pspec(G_OBJECT(self), XfdashboardDynamicTableLayoutProperties[PROP_ROWS]);
	}

	if(columns!=priv->columns)
	{
		priv->columns=columns;
		g_object_notify_by_pspec(G_OBJECT(self), XfdashboardDynamicTableLayoutProperties[PROP_COLUMNS]);
	}

	/* Step four: Determine column coordinates */
	if(priv->columnCoords)
	{
		g_array_free(priv->columnCoords, TRUE);
		priv->columnCoords=NULL;
	}

	priv->columnCoords=g_array_new(FALSE, FALSE, sizeof(gfloat));
	x=0.0f;
	i=0;
	clutter_actor_iter_init(&iter, CLUTTER_ACTOR(inContainer));
	while(clutter_actor_iter_next(&iter, &child))
	{
		/* Handle only visible actors */
		if(clutter_actor_is_visible(child))
		{
			g_array_append_val(priv->columnCoords, x);
			x+=(largestWidth+priv->columnSpacing);

			/* Increase counter for visible children */
			i++;
		}
	}

	g_array_append_val(priv->columnCoords, x);

	/* Step five: Determine row coordinates */
	if(priv->rowCoords)
	{
		g_array_free(priv->rowCoords, TRUE);
		priv->rowCoords=NULL;
	}

	priv->rowCoords=g_array_new(FALSE, FALSE, sizeof(gfloat));
	largestHeight=0.0f;
	y=-priv->rowSpacing;
	i=0;
	clutter_actor_iter_init(&iter, CLUTTER_ACTOR(inContainer));
	while(clutter_actor_iter_next(&iter, &child))
	{
		/* Handle only visible actors */
		if(clutter_actor_is_visible(child))
		{
			/* If it is the first columns in row, calculate y-coordinate
			 * and append it to array.
			 */
			if((i % priv->columns)==0)
			{
				y+=largestHeight+priv->rowSpacing;
				g_array_append_val(priv->rowCoords, y);
				largestHeight=0.0f;
			}

			/* Determine largest height in row */
			clutter_actor_get_preferred_size(child, NULL, NULL, NULL, &childHeight);
			largestHeight=MAX(largestHeight, childHeight);

			/* Increase counter for visible children */
			i++;
		}
	}

	y+=largestHeight;
	g_array_append_val(priv->rowCoords, y);

	/* Thaw notification */
	g_object_thaw_notify(G_OBJECT(self));
}

/* Updates data needed for layout in fixed mode (fixed number of columns) */
static void _xfdashboard_dynamic_table_layout_update_layout_data_fixed(XfdashboardDynamicTableLayout *self,
																		ClutterContainer *inContainer,
																		gfloat inWidth,
																		gfloat inHeight)
{
	XfdashboardDynamicTableLayoutPrivate		*priv;
	ClutterActorIter							iter;
	ClutterActor								*child;
	gint										numberChildren;
	gint										rows, columns;
	gfloat										childHeight;
	gfloat										fixedWidth;
	gfloat										largestHeight;
	gint										i;
	gfloat										x, y;
	ClutterRequestMode							requestMode;

	g_return_if_fail(XFDASHBOARD_IS_DYNAMIC_TABLE_LAYOUT(self));
	g_return_if_fail(CLUTTER_IS_CONTAINER(inContainer));
	g_return_if_fail(CLUTTER_IS_ACTOR(inContainer));

	priv=self->priv;

	/* Freeze notification */
	g_object_freeze_notify(G_OBJECT(self));

	/* Step one: Get number of visible child actors */
	numberChildren=0;
	clutter_actor_iter_init(&iter, CLUTTER_ACTOR(inContainer));
	while(clutter_actor_iter_next(&iter, &child))
	{
		/* Handle only visible actors */
		if(clutter_actor_is_visible(child))
		{
			/* Count visible children */
			numberChildren++;
		}
	}

	if(numberChildren!=priv->numberChildren)
	{
		priv->numberChildren=numberChildren;
		g_object_notify_by_pspec(G_OBJECT(self), XfdashboardDynamicTableLayoutProperties[PROP_NUMBER_CHILDREN]);
	}

	/* Step two: Get request mode of container to layout. The request will be
	 *           overriden if any of given width or height is not set (< 0)
	 */
	requestMode=clutter_actor_get_request_mode(CLUTTER_ACTOR(inContainer));

	if(inWidth<0.0f) requestMode=CLUTTER_REQUEST_WIDTH_FOR_HEIGHT;
		else if(inHeight<0.0f) requestMode=CLUTTER_REQUEST_HEIGHT_FOR_WIDTH;


	/* Step three: Determine number of rows as number of columns is fixed */
	rows=0;
	columns=0;
	rows=columns=0;
	if(inWidth<0.0f && inHeight<0.0f)
	{
		columns=priv->numberChildren;
		rows=1;
	}
		else if(requestMode==CLUTTER_REQUEST_HEIGHT_FOR_WIDTH)
		{
			columns=priv->fixedColumns;
			rows=ceil((double)priv->numberChildren / (double)columns);
		}
		else if(requestMode==CLUTTER_REQUEST_WIDTH_FOR_HEIGHT)
		{
			rows=priv->fixedColumns;
			columns=ceil((double)priv->numberChildren / (double)rows);
		}
		else
		{
			g_assert_not_reached();
		}


	if(rows!=priv->rows)
	{
		priv->rows=rows;
		g_object_notify_by_pspec(G_OBJECT(self), XfdashboardDynamicTableLayoutProperties[PROP_ROWS]);
	}

	if(columns!=priv->columns)
	{
		priv->columns=columns;
		g_object_notify_by_pspec(G_OBJECT(self), XfdashboardDynamicTableLayoutProperties[PROP_COLUMNS]);
	}

	/* Step four: Determine column coordinates */
	if(priv->columnCoords)
	{
		g_array_free(priv->columnCoords, TRUE);
		priv->columnCoords=NULL;
	}

	priv->columnCoords=g_array_new(FALSE, FALSE, sizeof(gfloat));
	x=0.0f;
	i=0;
	fixedWidth=(inWidth>0.0f ? inWidth/priv->columns : 0.0f);
	clutter_actor_iter_init(&iter, CLUTTER_ACTOR(inContainer));
	while(clutter_actor_iter_next(&iter, &child))
	{
		/* Handle only visible actors */
		if(clutter_actor_is_visible(child))
		{
			g_array_append_val(priv->columnCoords, x);
			x+=(fixedWidth+priv->columnSpacing);

			/* Increase counter for visible children */
			i++;
		}
	}

	g_array_append_val(priv->columnCoords, x);

	/* Step five: Determine row coordinates */
	if(priv->rowCoords)
	{
		g_array_free(priv->rowCoords, TRUE);
		priv->rowCoords=NULL;
	}

	priv->rowCoords=g_array_new(FALSE, FALSE, sizeof(gfloat));
	largestHeight=0.0f;
	y=-priv->rowSpacing;
	i=0;
	clutter_actor_iter_init(&iter, CLUTTER_ACTOR(inContainer));
	while(clutter_actor_iter_next(&iter, &child))
	{
		/* Handle only visible actors */
		if(clutter_actor_is_visible(child))
		{
			/* If it is the first columns in row, calculate y-coordinate
			 * and append it to array.
			 */
			if((i % priv->columns)==0)
			{
				y+=largestHeight+priv->rowSpacing;
				g_array_append_val(priv->rowCoords, y);
				largestHeight=0.0f;
			}

			/* Determine largest height in row */
			clutter_actor_get_preferred_size(child, NULL, NULL, NULL, &childHeight);
			largestHeight=MAX(largestHeight, childHeight);

			/* Increase counter for visible children */
			i++;
		}
	}

	y+=largestHeight;
	g_array_append_val(priv->rowCoords, y);

	/* Thaw notification */
	g_object_thaw_notify(G_OBJECT(self));
}

/* Switch to updates data needed for layout in dynamic or fixed mode */
static void _xfdashboard_dynamic_table_layout_update_layout_data(XfdashboardDynamicTableLayout *self,
																	ClutterContainer *inContainer,
																	gfloat inWidth,
																	gfloat inHeight)
{
	XfdashboardDynamicTableLayoutPrivate	*priv;

	g_return_if_fail(XFDASHBOARD_IS_DYNAMIC_TABLE_LAYOUT(self));
	g_return_if_fail(CLUTTER_IS_CONTAINER(inContainer));
	g_return_if_fail(CLUTTER_IS_ACTOR(inContainer));

	priv=self->priv;

	/* If a fixed number of column is set call the update data function for fixed
	 * mode otherwise the one for dynamic mode.
	 */
	if(priv->fixedColumns>0)
	{
		_xfdashboard_dynamic_table_layout_update_layout_data_fixed(self,
																	inContainer,
																	inWidth,
																	inHeight);
	}
		else
		{
			_xfdashboard_dynamic_table_layout_update_layout_data_dynamic(self,
																			inContainer,
																			inWidth,
																			inHeight);
		}
}

/* A style revalidation happened at container this layout manager is attached to */
static void _xfdashboard_dynamic_table_layout_on_style_revalidated(XfdashboardDynamicTableLayout *self,
																	gpointer inUserData)
{
	g_return_if_fail(XFDASHBOARD_IS_DYNAMIC_TABLE_LAYOUT(self));

	/* Invalidate style to get new possible styles applied */
	xfdashboard_stylable_invalidate(XFDASHBOARD_STYLABLE(self));
}


/* IMPLEMENTATION: ClutterLayoutManager */

/* Get preferred width/height */
static void _xfdashboard_dynamic_table_layout_get_preferred_width(ClutterLayoutManager *self,
																	ClutterContainer *inContainer,
																	gfloat inForHeight,
																	gfloat *outMinWidth,
																	gfloat *outNaturalWidth)
{
	XfdashboardDynamicTableLayoutPrivate	*priv;
	gfloat									maxMinWidth, maxNaturalWidth;

	g_return_if_fail(XFDASHBOARD_IS_DYNAMIC_TABLE_LAYOUT(self));
	g_return_if_fail(CLUTTER_IS_CONTAINER(inContainer));

	priv=XFDASHBOARD_DYNAMIC_TABLE_LAYOUT(self)->priv;

	/* Set up default values */
	maxMinWidth=0.0f;
	maxNaturalWidth=0.0f;

	/* Update data needed for layout */
	_xfdashboard_dynamic_table_layout_update_layout_data(XFDASHBOARD_DYNAMIC_TABLE_LAYOUT(self), inContainer, -1.0f, inForHeight);

	/* Calculate width */
	if(priv->columns>0)
	{
		maxMinWidth=(priv->columns-1)*priv->columnSpacing;
		maxNaturalWidth=g_array_index(priv->columnCoords, gfloat, priv->columns);
	}

	/* Set return values */
	if(outMinWidth) *outMinWidth=maxMinWidth;
	if(outNaturalWidth) *outNaturalWidth=maxNaturalWidth;
}

static void _xfdashboard_dynamic_table_layout_get_preferred_height(ClutterLayoutManager *self,
																	ClutterContainer *inContainer,
																	gfloat inForWidth,
																	gfloat *outMinHeight,
																	gfloat *outNaturalHeight)
{
	XfdashboardDynamicTableLayoutPrivate	*priv;
	gfloat									maxMinHeight, maxNaturalHeight;

	g_return_if_fail(XFDASHBOARD_IS_DYNAMIC_TABLE_LAYOUT(self));
	g_return_if_fail(CLUTTER_IS_CONTAINER(inContainer));

	priv=XFDASHBOARD_DYNAMIC_TABLE_LAYOUT(self)->priv;

	/* Set up default values */
	maxMinHeight=0.0f;
	maxNaturalHeight=0.0f;

	/* Update data needed for layout */
	_xfdashboard_dynamic_table_layout_update_layout_data(XFDASHBOARD_DYNAMIC_TABLE_LAYOUT(self), inContainer, inForWidth, -1.0f);

	/* Calculate height */
	if(priv->rows>0)
	{
		maxMinHeight=(priv->rows-1)*priv->rowSpacing;
		maxNaturalHeight=g_array_index(priv->rowCoords, gfloat, priv->rows);
	}

	/* Set return values */
	if(outMinHeight) *outMinHeight=maxMinHeight;
	if(outNaturalHeight) *outNaturalHeight=maxNaturalHeight;
}

/* Re-layout and allocate children of container we manage */
static void _xfdashboard_dynamic_table_layout_allocate(ClutterLayoutManager *self,
														ClutterContainer *inContainer,
														const ClutterActorBox *inAllocation,
														ClutterAllocationFlags inFlags)
{
	XfdashboardDynamicTableLayoutPrivate	*priv;
	gfloat									width, height;
	ClutterActorIter						iter;
	ClutterActor							*child;
	gint									column, row, i;
	ClutterActorBox							childAllocation;

	g_return_if_fail(XFDASHBOARD_IS_DYNAMIC_TABLE_LAYOUT(self));
	g_return_if_fail(CLUTTER_IS_CONTAINER(inContainer));
	g_return_if_fail(CLUTTER_IS_ACTOR(inContainer));

	priv=XFDASHBOARD_DYNAMIC_TABLE_LAYOUT(self)->priv;

	/* Get size of container holding children to layout */
	width=clutter_actor_box_get_width(inAllocation);
	height=clutter_actor_box_get_height(inAllocation);

	/* Update data needed for layout */
	_xfdashboard_dynamic_table_layout_update_layout_data(XFDASHBOARD_DYNAMIC_TABLE_LAYOUT(self),
															inContainer,
															width,
															height);

	/* Determine allocation for each visible child */
	i=0;
	clutter_actor_iter_init(&iter, CLUTTER_ACTOR(inContainer));
	while(clutter_actor_iter_next(&iter, &child))
	{
		/* Handle only visible actors */
		if(clutter_actor_is_visible(child))
		{
			/* Get column and row for child */
			column=floor(i % priv->columns);
			row=floor(i / priv->columns);

			/* Get available allocation space for child*/
			childAllocation.x1=g_array_index(priv->columnCoords, gfloat, column);
			childAllocation.x2=g_array_index(priv->columnCoords, gfloat, column+1)-priv->columnSpacing;
			childAllocation.y1=g_array_index(priv->rowCoords, gfloat, row);
			childAllocation.y2=g_array_index(priv->rowCoords, gfloat, row+1)-priv->rowSpacing;

			/* Set allocation at child */
			clutter_actor_allocate(child, &childAllocation, inFlags);

			/* Increase counter for visible children */
			i++;
		}
	}
}

/* Container was set at this layout manager */
static void _xfdashboard_dynamic_table_layout_set_container(ClutterLayoutManager *self,
															ClutterContainer *inContainer)
{
	XfdashboardDynamicTableLayoutPrivate	*priv;

	g_return_if_fail(XFDASHBOARD_IS_DYNAMIC_TABLE_LAYOUT(self));
	g_return_if_fail(inContainer==NULL || CLUTTER_IS_CONTAINER(inContainer));

	priv=XFDASHBOARD_DYNAMIC_TABLE_LAYOUT(self)->priv;

	/* Remove weak reference at old container */
	if(priv->container)
	{
		/* Disconnect signal handler to get notified about style revalidations */
		if(priv->styleRevalidationSignalID)
		{
			g_signal_handler_disconnect(priv->container, priv->styleRevalidationSignalID);
			priv->styleRevalidationSignalID=0;
		}

		/* Remove weak reference from container */
		g_object_remove_weak_pointer(G_OBJECT(priv->container), &priv->container);
		priv->container=NULL;
	}

	/* Add weak reference at new container */
	if(inContainer)
	{
		/* Remember new container set and add weak reference */
		priv->container=inContainer;
		g_object_add_weak_pointer(G_OBJECT(priv->container), &priv->container);

		/* Connect signal handler to get notified about style revalidations */
		priv->styleRevalidationSignalID=g_signal_connect_swapped(priv->container,
																	"style-revalidated",
																	G_CALLBACK(_xfdashboard_dynamic_table_layout_on_style_revalidated),
																	self);
	}

	/* Invalidate style to get new possible styles applied */
	xfdashboard_stylable_invalidate(XFDASHBOARD_STYLABLE(self));
}


/* IMPLEMENTATION: Interface XfdashboardStylable */

/* Get stylable properties of this stylable object */
static void _xfdashboard_dynamic_table_layout_stylable_get_stylable_properties(XfdashboardStylable *inStylable,
																				GHashTable *ioStylableProperties)
{
	g_return_if_fail(XFDASHBOARD_IS_STYLABLE(inStylable));

	/* Add stylable properties to hashtable */
	xfdashboard_stylable_add_stylable_property(inStylable, ioStylableProperties, "fixed-columns");
}

/* Get parent stylable actor of this stylable object */
static XfdashboardStylable* _xfdashboard_dynamic_table_layout_stylable_get_parent(XfdashboardStylable *inStylable)
{
	XfdashboardDynamicTableLayoutPrivate	*priv;

	g_return_val_if_fail(XFDASHBOARD_IS_STYLABLE(inStylable), NULL);
	g_return_val_if_fail(XFDASHBOARD_IS_DYNAMIC_TABLE_LAYOUT(inStylable), NULL);

	priv=XFDASHBOARD_DYNAMIC_TABLE_LAYOUT(inStylable)->priv;

	/* Check if container set in this layout manager is stylable and return the container as parent */
	if(priv->container &&
		XFDASHBOARD_IS_STYLABLE(priv->container))
	{
		return(XFDASHBOARD_STYLABLE(priv->container));
	}

	/* If we get here either no container was set or it is not stylable */
	return(NULL);
}

/* Get/set style classes of this stylable object */
static const gchar* _xfdashboard_dynamic_table_layout_stylable_get_classes(XfdashboardStylable *inStylable)
{
	/* Not implemented */
	return(NULL);
}

static void _xfdashboard_dynamic_table_layout_stylable_set_classes(XfdashboardStylable *inStylable, const gchar *inStyleClasses)
{
	/* Not implemented */
}

/* Get/set style pseudo-classes of this stylable object */
static const gchar* _xfdashboard_dynamic_table_layout_stylable_get_pseudo_classes(XfdashboardStylable *inStylable)
{
	/* Not implemented */
	return(NULL);
}

static void _xfdashboard_dynamic_table_layout_stylable_set_pseudo_classes(XfdashboardStylable *inStylable, const gchar *inStylePseudoClasses)
{
	/* Not implemented */
}

/* Interface initialization
 * Set up default functions
 */
void _xfdashboard_dynamic_table_layout_stylable_iface_init(XfdashboardStylableInterface *iface)
{
	iface->get_stylable_properties=_xfdashboard_dynamic_table_layout_stylable_get_stylable_properties;
	iface->get_parent=_xfdashboard_dynamic_table_layout_stylable_get_parent;
	iface->get_classes=_xfdashboard_dynamic_table_layout_stylable_get_classes;
	iface->set_classes=_xfdashboard_dynamic_table_layout_stylable_set_classes;
	iface->get_pseudo_classes=_xfdashboard_dynamic_table_layout_stylable_get_pseudo_classes;
	iface->set_pseudo_classes=_xfdashboard_dynamic_table_layout_stylable_set_pseudo_classes;
}


/* IMPLEMENTATION: GObject */

/* Dispose this object */
static void _xfdashboard_dynamic_table_layout_dispose(GObject *inObject)
{
	XfdashboardDynamicTableLayout			*self=XFDASHBOARD_DYNAMIC_TABLE_LAYOUT(inObject);
	XfdashboardDynamicTableLayoutPrivate	*priv=self->priv;

	/* Dispose allocated resources */
	if(priv->columnCoords)
	{
		g_array_free(priv->columnCoords, TRUE);
		priv->columnCoords=NULL;
	}

	if(priv->rowCoords)
	{
		g_array_free(priv->rowCoords, TRUE);
		priv->rowCoords=NULL;
	}

	if(priv->container)
	{
		/* Disconnect signal handler to get notified about style revalidations */
		if(priv->styleRevalidationSignalID)
		{
			g_signal_handler_disconnect(priv->container, priv->styleRevalidationSignalID);
			priv->styleRevalidationSignalID=0;
		}

		/* Remove weak reference from container */
		g_object_remove_weak_pointer(G_OBJECT(priv->container), &priv->container);
		priv->container=NULL;
	}

	/* Call parent's class dispose method */
	G_OBJECT_CLASS(xfdashboard_dynamic_table_layout_parent_class)->dispose(inObject);
}

/* Set/get properties */
static void _xfdashboard_dynamic_table_layout_set_property(GObject *inObject,
															guint inPropID,
															const GValue *inValue,
															GParamSpec *inSpec)
{
	XfdashboardDynamicTableLayout	*self=XFDASHBOARD_DYNAMIC_TABLE_LAYOUT(inObject);
	
	switch(inPropID)
	{
		case PROP_ROW_SPACING:
			xfdashboard_dynamic_table_layout_set_row_spacing(self, g_value_get_float(inValue));
			break;

		case PROP_COLUMN_SPACING:
			xfdashboard_dynamic_table_layout_set_column_spacing(self, g_value_get_float(inValue));
			break;

		case PROP_FIXED_COLUMNS:
			xfdashboard_dynamic_table_layout_set_fixed_columns(self, g_value_get_int(inValue));
			break;

		default:
			G_OBJECT_WARN_INVALID_PROPERTY_ID(inObject, inPropID, inSpec);
			break;
	}
}

static void _xfdashboard_dynamic_table_layout_get_property(GObject *inObject,
															guint inPropID,
															GValue *outValue,
															GParamSpec *inSpec)
{
	XfdashboardDynamicTableLayout	*self=XFDASHBOARD_DYNAMIC_TABLE_LAYOUT(inObject);

	switch(inPropID)
	{
		case PROP_ROW_SPACING:
			g_value_set_float(outValue, self->priv->rowSpacing);
			break;

		case PROP_COLUMN_SPACING:
			g_value_set_float(outValue, self->priv->columnSpacing);
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

		case PROP_FIXED_COLUMNS:
			g_value_set_int(outValue, self->priv->fixedColumns);
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
static void xfdashboard_dynamic_table_layout_class_init(XfdashboardDynamicTableLayoutClass *klass)
{
	ClutterLayoutManagerClass		*layoutClass=CLUTTER_LAYOUT_MANAGER_CLASS(klass);
	GObjectClass					*gobjectClass=G_OBJECT_CLASS(klass);
	XfdashboardStylableInterface	*stylableIface;
	GParamSpec						*paramSpec;

	/* Override functions */
	layoutClass->set_container=_xfdashboard_dynamic_table_layout_set_container;
	layoutClass->get_preferred_width=_xfdashboard_dynamic_table_layout_get_preferred_width;
	layoutClass->get_preferred_height=_xfdashboard_dynamic_table_layout_get_preferred_height;
	layoutClass->allocate=_xfdashboard_dynamic_table_layout_allocate;

	gobjectClass->dispose=_xfdashboard_dynamic_table_layout_dispose;
	gobjectClass->set_property=_xfdashboard_dynamic_table_layout_set_property;
	gobjectClass->get_property=_xfdashboard_dynamic_table_layout_get_property;

	stylableIface=g_type_default_interface_ref(XFDASHBOARD_TYPE_STYLABLE);

	/* Define properties */
	XfdashboardDynamicTableLayoutProperties[PROP_ROW_SPACING]=
		g_param_spec_float("row-spacing",
								_("Row spacing"),
								_("The spacing between rows in table"),
								0.0f,
								G_MAXFLOAT,
								0.0f,
								G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS);

	XfdashboardDynamicTableLayoutProperties[PROP_COLUMN_SPACING]=
		g_param_spec_float("column-spacing",
								_("Column spacing"),
								_("The spacing between columns in table"),
								0.0f,
								G_MAXFLOAT,
								0.0f,
								G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS);

	XfdashboardDynamicTableLayoutProperties[PROP_NUMBER_CHILDREN]=
		g_param_spec_int("number-children",
								_("Number children"),
								_("Current number of child actors in this layout"),
								0,
								G_MAXINT,
								0,
								G_PARAM_READABLE | G_PARAM_STATIC_STRINGS);

	XfdashboardDynamicTableLayoutProperties[PROP_ROWS]=
		g_param_spec_int("rows",
								_("Rows"),
								_("Current number of rows in this layout"),
								0,
								G_MAXINT,
								0,
								G_PARAM_READABLE | G_PARAM_STATIC_STRINGS);

	XfdashboardDynamicTableLayoutProperties[PROP_COLUMNS]=
		g_param_spec_int("columns",
								_("Columns"),
								_("Current number of columns in this layout"),
								0,
								G_MAXINT,
								0,
								G_PARAM_READABLE | G_PARAM_STATIC_STRINGS);

	XfdashboardDynamicTableLayoutProperties[PROP_FIXED_COLUMNS]=
		g_param_spec_int("fixed-columns",
								_("Fixed columns"),
								_("Fixed number of columns to use in this layout"),
								0,
								G_MAXINT,
								0,
								G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS);

	paramSpec=g_object_interface_find_property(stylableIface, "style-classes");
	XfdashboardDynamicTableLayoutProperties[PROP_STYLE_CLASSES]=
		g_param_spec_override("style-classes", paramSpec);

	paramSpec=g_object_interface_find_property(stylableIface, "style-pseudo-classes");
	XfdashboardDynamicTableLayoutProperties[PROP_STYLE_PSEUDO_CLASSES]=
		g_param_spec_override("style-pseudo-classes", paramSpec);

	g_object_class_install_properties(gobjectClass, PROP_LAST, XfdashboardDynamicTableLayoutProperties);

	/* Release allocated resources */
	g_type_default_interface_unref(stylableIface);
}

/* Object initialization
 * Create private structure and set up default values
 */
static void xfdashboard_dynamic_table_layout_init(XfdashboardDynamicTableLayout *self)
{
	XfdashboardDynamicTableLayoutPrivate	*priv;

	priv=self->priv=xfdashboard_dynamic_table_layout_get_instance_private(self);

	/* Set default values */
	priv->rowSpacing=0.0f;
	priv->columnSpacing=0.0f;

	priv->numberChildren=0;
	priv->rows=0;
	priv->columns=0;
	priv->fixedColumns=0;
	priv->columnCoords=NULL;
	priv->rowCoords=NULL;
	priv->container=NULL;
	priv->styleRevalidationSignalID=0;

	/* Style layout manager */
	xfdashboard_stylable_invalidate(XFDASHBOARD_STYLABLE(self));
}

/* IMPLEMENTATION: Public API */

/* Create new instance */
ClutterLayoutManager* xfdashboard_dynamic_table_layout_new(void)
{
	return(CLUTTER_LAYOUT_MANAGER(g_object_new(XFDASHBOARD_TYPE_DYNAMIC_TABLE_LAYOUT, NULL)));
}

/* Get number of (visible) children which will be layouted */
gint xfdashboard_dynamic_table_layout_get_number_children(XfdashboardDynamicTableLayout *self)
{
	g_return_val_if_fail(XFDASHBOARD_IS_DYNAMIC_TABLE_LAYOUT(self), 0);

	return(self->priv->numberChildren);
}

/* Get number of rows */
gint xfdashboard_dynamic_table_layout_get_rows(XfdashboardDynamicTableLayout *self)
{
	g_return_val_if_fail(XFDASHBOARD_IS_DYNAMIC_TABLE_LAYOUT(self), 0);

	return(self->priv->rows);
}

/* Get number of columns */
gint xfdashboard_dynamic_table_layout_get_columns(XfdashboardDynamicTableLayout *self)
{
	g_return_val_if_fail(XFDASHBOARD_IS_DYNAMIC_TABLE_LAYOUT(self), 0);

	return(self->priv->columns);
}

/* Set relative row and column spacing to same value at once */
void xfdashboard_dynamic_table_layout_set_spacing(XfdashboardDynamicTableLayout *self, gfloat inSpacing)
{
	XfdashboardDynamicTableLayoutPrivate		*priv;

	g_return_if_fail(XFDASHBOARD_IS_DYNAMIC_TABLE_LAYOUT(self));
	g_return_if_fail(inSpacing>=0.0f);

	priv=self->priv;

	/* Set new values if changed */
	if(priv->rowSpacing!=inSpacing || priv->columnSpacing!=inSpacing)
	{
		/* Set new values and notify about properties changes */
		priv->rowSpacing=inSpacing;
		g_object_notify_by_pspec(G_OBJECT(self), XfdashboardDynamicTableLayoutProperties[PROP_ROW_SPACING]);

		priv->columnSpacing=inSpacing;
		g_object_notify_by_pspec(G_OBJECT(self), XfdashboardDynamicTableLayoutProperties[PROP_COLUMN_SPACING]);

		/* Notify for upcoming layout changes */
		clutter_layout_manager_layout_changed(CLUTTER_LAYOUT_MANAGER(self));
	}
}

/* Get/set row spacing */
gfloat xfdashboard_dynamic_table_layout_get_row_spacing(XfdashboardDynamicTableLayout *self)
{
	g_return_val_if_fail(XFDASHBOARD_IS_DYNAMIC_TABLE_LAYOUT(self), 0.0f);

	return(self->priv->rowSpacing);
}

void xfdashboard_dynamic_table_layout_set_row_spacing(XfdashboardDynamicTableLayout *self, gfloat inSpacing)
{
	XfdashboardDynamicTableLayoutPrivate		*priv;

	g_return_if_fail(XFDASHBOARD_IS_DYNAMIC_TABLE_LAYOUT(self));
	g_return_if_fail(inSpacing>=0.0f);

	priv=self->priv;

	/* Set new value if changed */
	if(priv->rowSpacing!=inSpacing)
	{
		/* Set new value and notify about property change */
		priv->rowSpacing=inSpacing;
		g_object_notify_by_pspec(G_OBJECT(self), XfdashboardDynamicTableLayoutProperties[PROP_ROW_SPACING]);

		/* Notify for upcoming layout changes */
		clutter_layout_manager_layout_changed(CLUTTER_LAYOUT_MANAGER(self));
	}
}

/* Get/set columns spacing */
gfloat xfdashboard_dynamic_table_layout_get_column_spacing(XfdashboardDynamicTableLayout *self)
{
	g_return_val_if_fail(XFDASHBOARD_IS_DYNAMIC_TABLE_LAYOUT(self), 0.0f);

	return(self->priv->columnSpacing);
}

void xfdashboard_dynamic_table_layout_set_column_spacing(XfdashboardDynamicTableLayout *self, gfloat inSpacing)
{
	XfdashboardDynamicTableLayoutPrivate		*priv;

	g_return_if_fail(XFDASHBOARD_IS_DYNAMIC_TABLE_LAYOUT(self));
	g_return_if_fail(inSpacing>=0.0f);

	priv=self->priv;

	/* Set new value if changed */
	if(priv->columnSpacing!=inSpacing)
	{
		/* Set new value and notify about property change */
		priv->columnSpacing=inSpacing;
		g_object_notify_by_pspec(G_OBJECT(self), XfdashboardDynamicTableLayoutProperties[PROP_COLUMN_SPACING]);

		/* Notify for upcoming layout changes */
		clutter_layout_manager_layout_changed(CLUTTER_LAYOUT_MANAGER(self));
	}
}

/* Get/set fixed number of columns */
gint xfdashboard_dynamic_table_layout_get_fixed_columns(XfdashboardDynamicTableLayout *self)
{
	g_return_val_if_fail(XFDASHBOARD_IS_DYNAMIC_TABLE_LAYOUT(self), 0.0f);

	return(self->priv->fixedColumns);
}

void xfdashboard_dynamic_table_layout_set_fixed_columns(XfdashboardDynamicTableLayout *self, gint inColumns)
{
	XfdashboardDynamicTableLayoutPrivate		*priv;

	g_return_if_fail(XFDASHBOARD_IS_DYNAMIC_TABLE_LAYOUT(self));
	g_return_if_fail(inColumns>=0);

	priv=self->priv;

	/* Set new value if changed */
	if(priv->fixedColumns!=inColumns)
	{
		/* Set new value and notify about property change */
		priv->fixedColumns=inColumns;
		g_object_notify_by_pspec(G_OBJECT(self), XfdashboardDynamicTableLayoutProperties[PROP_FIXED_COLUMNS]);

		/* Notify for upcoming layout changes */
		clutter_layout_manager_layout_changed(CLUTTER_LAYOUT_MANAGER(self));
	}
}
