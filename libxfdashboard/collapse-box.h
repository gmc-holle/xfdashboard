/*
 * collapse-box: A collapsable container for one actor
 *               with capability to expand
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

#ifndef __LIBXFDASHBOARD_COLLAPSE_BOX__
#define __LIBXFDASHBOARD_COLLAPSE_BOX__

#if !defined(__LIBXFDASHBOARD_H_INSIDE__) && !defined(LIBXFDASHBOARD_COMPILATION)
#error "Only <libxfdashboard/libxfdashboard.h> can be included directly."
#endif

#include <clutter/clutter.h>

#include <libxfdashboard/actor.h>
#include <libxfdashboard/types.h>

G_BEGIN_DECLS

#define XFDASHBOARD_TYPE_COLLAPSE_BOX				(xfdashboard_collapse_box_get_type())
#define XFDASHBOARD_COLLAPSE_BOX(obj)				(G_TYPE_CHECK_INSTANCE_CAST((obj), XFDASHBOARD_TYPE_COLLAPSE_BOX, XfdashboardCollapseBox))
#define XFDASHBOARD_IS_COLLAPSE_BOX(obj)			(G_TYPE_CHECK_INSTANCE_TYPE((obj), XFDASHBOARD_TYPE_COLLAPSE_BOX))
#define XFDASHBOARD_COLLAPSE_BOX_CLASS(klass)		(G_TYPE_CHECK_CLASS_CAST((klass), XFDASHBOARD_TYPE_COLLAPSE_BOX, XfdashboardCollapseBoxClass))
#define XFDASHBOARD_IS_COLLAPSE_BOX_CLASS(klass)	(G_TYPE_CHECK_CLASS_TYPE((klass), XFDASHBOARD_TYPE_COLLAPSE_BOX))
#define XFDASHBOARD_COLLAPSE_BOX_GET_CLASS(obj)		(G_TYPE_INSTANCE_GET_CLASS((obj), XFDASHBOARD_TYPE_COLLAPSE_BOX, XfdashboardCollapseBoxClass))

typedef struct _XfdashboardCollapseBox				XfdashboardCollapseBox; 
typedef struct _XfdashboardCollapseBoxPrivate		XfdashboardCollapseBoxPrivate;
typedef struct _XfdashboardCollapseBoxClass			XfdashboardCollapseBoxClass;

struct _XfdashboardCollapseBox
{
	/*< private >*/
	/* Parent instance */
	XfdashboardActor				parent_instance;

	/* Private structure */
	XfdashboardCollapseBoxPrivate	*priv;
};

struct _XfdashboardCollapseBoxClass
{
	/*< private >*/
	/* Parent class */
	XfdashboardActorClass			parent_class;

	/*< public >*/
	/* Virtual functions */
	void (*collapse_changed)(XfdashboardCollapseBox *self, gboolean isCollapsed);
};

/* Public API */
GType xfdashboard_collapse_box_get_type(void) G_GNUC_CONST;

ClutterActor* xfdashboard_collapse_box_new(void);

gboolean xfdashboard_collapse_box_get_collapsed(XfdashboardCollapseBox *self);
void xfdashboard_collapse_box_set_collapsed(XfdashboardCollapseBox *self, gboolean inCollapsed);

gfloat xfdashboard_collapse_box_get_collapsed_size(XfdashboardCollapseBox *self);
void xfdashboard_collapse_box_set_collapsed_size(XfdashboardCollapseBox *self, gfloat inCollapsedSize);

XfdashboardOrientation xfdashboard_collapse_box_get_collapse_orientation(XfdashboardCollapseBox *self);
void xfdashboard_collapse_box_set_collapse_orientation(XfdashboardCollapseBox *self, XfdashboardOrientation inOrientation);

G_END_DECLS

#endif	/* __LIBXFDASHBOARD_COLLAPSE_BOX__ */
