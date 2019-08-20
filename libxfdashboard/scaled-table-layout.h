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

#ifndef __LIBXFDASHBOARD_SCALED_TABLE_LAYOUT__
#define __LIBXFDASHBOARD_SCALED_TABLE_LAYOUT__

#if !defined(__LIBXFDASHBOARD_H_INSIDE__) && !defined(LIBXFDASHBOARD_COMPILATION)
#error "Only <libxfdashboard/libxfdashboard.h> can be included directly."
#endif

#include <clutter/clutter.h>

G_BEGIN_DECLS

#define XFDASHBOARD_TYPE_SCALED_TABLE_LAYOUT			(xfdashboard_scaled_table_layout_get_type())
#define XFDASHBOARD_SCALED_TABLE_LAYOUT(obj)			(G_TYPE_CHECK_INSTANCE_CAST((obj), XFDASHBOARD_TYPE_SCALED_TABLE_LAYOUT, XfdashboardScaledTableLayout))
#define XFDASHBOARD_IS_SCALED_TABLE_LAYOUT(obj)			(G_TYPE_CHECK_INSTANCE_TYPE((obj), XFDASHBOARD_TYPE_SCALED_TABLE_LAYOUT))
#define XFDASHBOARD_SCALED_TABLE_LAYOUT_CLASS(klass)	(G_TYPE_CHECK_CLASS_CAST((klass), XFDASHBOARD_TYPE_SCALED_TABLE_LAYOUT, XfdashboardScaledTableLayoutClass))
#define XFDASHBOARD_IS_SCALED_TABLE_LAYOUT_CLASS(klass)	(G_TYPE_CHECK_CLASS_TYPE((klass), XFDASHBOARD_TYPE_SCALED_TABLE_LAYOUT))
#define XFDASHBOARD_SCALED_TABLE_LAYOUT_GET_CLASS(obj)	(G_TYPE_INSTANCE_GET_CLASS((obj), XFDASHBOARD_TYPE_SCALED_TABLE_LAYOUT, XfdashboardScaledTableLayoutClass))

typedef struct _XfdashboardScaledTableLayout			XfdashboardScaledTableLayout;
typedef struct _XfdashboardScaledTableLayoutPrivate		XfdashboardScaledTableLayoutPrivate;
typedef struct _XfdashboardScaledTableLayoutClass		XfdashboardScaledTableLayoutClass;

struct _XfdashboardScaledTableLayout
{
	/*< private >*/
	/* Parent instance */
	ClutterLayoutManager 				parent_instance;

	/* Private structure */
	XfdashboardScaledTableLayoutPrivate	*priv;
};

struct _XfdashboardScaledTableLayoutClass
{
	/*< private >*/
	/* Parent class */
	ClutterLayoutManagerClass			parent_class;
};

/* Public API */
GType xfdashboard_scaled_table_layout_get_type(void) G_GNUC_CONST;

ClutterLayoutManager* xfdashboard_scaled_table_layout_new(void);

gint xfdashboard_scaled_table_layout_get_number_children(XfdashboardScaledTableLayout *self);
gint xfdashboard_scaled_table_layout_get_rows(XfdashboardScaledTableLayout *self);
gint xfdashboard_scaled_table_layout_get_columns(XfdashboardScaledTableLayout *self);

gboolean xfdashboard_scaled_table_layout_get_relative_scale(XfdashboardScaledTableLayout *self);
void xfdashboard_scaled_table_layout_set_relative_scale(XfdashboardScaledTableLayout *self, gboolean inScaling);

gboolean xfdashboard_scaled_table_layout_get_prevent_upscaling(XfdashboardScaledTableLayout *self);
void xfdashboard_scaled_table_layout_set_prevent_upscaling(XfdashboardScaledTableLayout *self, gboolean inPreventUpscaling);

void xfdashboard_scaled_table_layout_set_spacing(XfdashboardScaledTableLayout *self, gfloat inSpacing);

gfloat xfdashboard_scaled_table_layout_get_row_spacing(XfdashboardScaledTableLayout *self);
void xfdashboard_scaled_table_layout_set_row_spacing(XfdashboardScaledTableLayout *self, gfloat inSpacing);

gfloat xfdashboard_scaled_table_layout_get_column_spacing(XfdashboardScaledTableLayout *self);
void xfdashboard_scaled_table_layout_set_column_spacing(XfdashboardScaledTableLayout *self, gfloat inSpacing);

G_END_DECLS

#endif	/* __LIBXFDASHBOARD_SCALED_TABLE_LAYOUT__ */
