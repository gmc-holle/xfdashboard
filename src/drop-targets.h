/*
 * drop-targets.h: Drop targets management
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

#ifndef __XFDASHBOARD_DROP_TARGETS__
#define __XFDASHBOARD_DROP_TARGETS__

#include "drop-action.h"

G_BEGIN_DECLS

#define XFDASHBOARD_TYPE_DROP_TARGETS				(xfdashboard_drop_targets_get_type())
#define XFDASHBOARD_DROP_TARGETS(obj)				(G_TYPE_CHECK_INSTANCE_CAST((obj), XFDASHBOARD_TYPE_DROP_TARGETS, XfdashboardDropTargets))
#define XFDASHBOARD_IS_DROP_TARGETS(obj)			(G_TYPE_CHECK_INSTANCE_TYPE((obj), XFDASHBOARD_TYPE_DROP_TARGETS))
#define XFDASHBOARD_DROP_TARGETS_CLASS(klass)		(G_TYPE_CHECK_CLASS_CAST((klass), XFDASHBOARD_TYPE_DROP_TARGETS, XfdashboardDropTargetsClass))
#define XFDASHBOARD_IS_DROP_TARGETS_CLASS(klass)	(G_TYPE_CHECK_CLASS_TYPE((klass), XFDASHBOARD_TYPE_DROP_TARGETS))
#define XFDASHBOARD_DROP_TARGETS_GET_CLASS(obj)		(G_TYPE_INSTANCE_GET_CLASS((obj), XFDASHBOARD_TYPE_DROP_TARGETS, XfdashboardDropTargetsClass))

typedef struct _XfdashboardDropTargets				XfdashboardDropTargets; 
typedef struct _XfdashboardDropTargetsPrivate		XfdashboardDropTargetsPrivate;
typedef struct _XfdashboardDropTargetsClass			XfdashboardDropTargetsClass;

struct _XfdashboardDropTargets
{
	/* Parent instance */
	GObject							parent_instance;

	/* Private structure */
	XfdashboardDropTargetsPrivate	*priv;
};

struct _XfdashboardDropTargetsClass
{
	/* Parent class */
	GObjectClass					parent_class;
};

/* Public API */
GType xfdashboard_drop_targets_get_type(void) G_GNUC_CONST;

XfdashboardDropTargets* xfdashboard_drop_targets_get_default(void);

void xfdashboard_drop_targets_register(XfdashboardDropAction *inTarget);
void xfdashboard_drop_targets_unregister(XfdashboardDropAction *inTarget);

GSList* xfdashboard_drop_targets_get_all(void);

G_END_DECLS

#endif
