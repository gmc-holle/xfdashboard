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

#ifndef __LIBXFDASHBOARD_DYNAMIC_TABLE_LAYOUT__
#define __LIBXFDASHBOARD_DYNAMIC_TABLE_LAYOUT__

#if !defined(__LIBXFDASHBOARD_H_INSIDE__) && !defined(LIBXFDASHBOARD_COMPILATION)
#error "Only <libxfdashboard/libxfdashboard.h> can be included directly."
#endif

#include <clutter/clutter.h>

G_BEGIN_DECLS

#define XFDASHBOARD_TYPE_DYNAMIC_TABLE_LAYOUT				(xfdashboard_dynamic_table_layout_get_type())
#define XFDASHBOARD_DYNAMIC_TABLE_LAYOUT(obj)				(G_TYPE_CHECK_INSTANCE_CAST((obj), XFDASHBOARD_TYPE_DYNAMIC_TABLE_LAYOUT, XfdashboardDynamicTableLayout))
#define XFDASHBOARD_IS_DYNAMIC_TABLE_LAYOUT(obj)			(G_TYPE_CHECK_INSTANCE_TYPE((obj), XFDASHBOARD_TYPE_DYNAMIC_TABLE_LAYOUT))
#define XFDASHBOARD_DYNAMIC_TABLE_LAYOUT_CLASS(klass)		(G_TYPE_CHECK_CLASS_CAST((klass), XFDASHBOARD_TYPE_DYNAMIC_TABLE_LAYOUT, XfdashboardDynamicTableLayoutClass))
#define XFDASHBOARD_IS_DYNAMIC_TABLE_LAYOUT_CLASS(klass)	(G_TYPE_CHECK_CLASS_TYPE((klass), XFDASHBOARD_TYPE_DYNAMIC_TABLE_LAYOUT))
#define XFDASHBOARD_DYNAMIC_TABLE_LAYOUT_GET_CLASS(obj)		(G_TYPE_INSTANCE_GET_CLASS((obj), XFDASHBOARD_TYPE_DYNAMIC_TABLE_LAYOUT, XfdashboardDynamicTableLayoutClass))

typedef struct _XfdashboardDynamicTableLayout				XfdashboardDynamicTableLayout;
typedef struct _XfdashboardDynamicTableLayoutPrivate		XfdashboardDynamicTableLayoutPrivate;
typedef struct _XfdashboardDynamicTableLayoutClass			XfdashboardDynamicTableLayoutClass;

struct _XfdashboardDynamicTableLayout
{
	/*< private >*/
	/* Parent instance */
	ClutterLayoutManager 					parent_instance;

	/* Private structure */
	XfdashboardDynamicTableLayoutPrivate	*priv;
};

struct _XfdashboardDynamicTableLayoutClass
{
	/*< private >*/
	/* Parent class */
	ClutterLayoutManagerClass				parent_class;
};

/* Public API */
GType xfdashboard_dynamic_table_layout_get_type(void) G_GNUC_CONST;

ClutterLayoutManager* xfdashboard_dynamic_table_layout_new(void);

gint xfdashboard_dynamic_table_layout_get_number_children(XfdashboardDynamicTableLayout *self);
gint xfdashboard_dynamic_table_layout_get_rows(XfdashboardDynamicTableLayout *self);
gint xfdashboard_dynamic_table_layout_get_columns(XfdashboardDynamicTableLayout *self);

void xfdashboard_dynamic_table_layout_set_spacing(XfdashboardDynamicTableLayout *self, gfloat inSpacing);

gfloat xfdashboard_dynamic_table_layout_get_row_spacing(XfdashboardDynamicTableLayout *self);
void xfdashboard_dynamic_table_layout_set_row_spacing(XfdashboardDynamicTableLayout *self, gfloat inSpacing);

gfloat xfdashboard_dynamic_table_layout_get_column_spacing(XfdashboardDynamicTableLayout *self);
void xfdashboard_dynamic_table_layout_set_column_spacing(XfdashboardDynamicTableLayout *self, gfloat inSpacing);

gint xfdashboard_dynamic_table_layout_get_fixed_columns(XfdashboardDynamicTableLayout *self);
void xfdashboard_dynamic_table_layout_set_fixed_columns(XfdashboardDynamicTableLayout *self, gint inColumns);

G_END_DECLS

#endif	/* __LIBXFDASHBOARD_DYNAMIC_TABLE_LAYOUT__ */
