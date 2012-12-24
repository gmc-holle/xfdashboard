/*
 * drop-targets.c: Drop targets management
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

#include "drop-targets.h"

/* Define this class in GObject system */
G_DEFINE_TYPE(XfdashboardDropTargets,
				xfdashboard_drop_targets,
				G_TYPE_OBJECT)

/* Private structure - access only by public API if needed */
#define XFDASHBOARD_DROP_TARGETS_GET_PRIVATE(obj) \
	(G_TYPE_INSTANCE_GET_PRIVATE((obj), XFDASHBOARD_TYPE_DROP_TARGETS, XfdashboardDropTargetsPrivate))

struct _XfdashboardDropTargetsPrivate
{
	/* List of drop targets */
	GSList	*targets;
};

/* IMPLEMENTATION: GObject */

/* Dispose this object */
static void xfdashboard_drop_targets_dispose(GObject *inObject)
{
	XfdashboardDropTargets			*self=XFDASHBOARD_DROP_TARGETS(inObject);
	XfdashboardDropTargetsPrivate	*priv=self->priv;

	/* Release allocated resources */
	if(priv->targets)
	{
		GSList	*list;

		for(list=priv->targets; list; list=list->next) xfdashboard_drop_targets_unregister(list->data);
		g_slist_free(priv->targets);
	}
	priv->targets=NULL;

	/* Call parent's class dispose method */
	G_OBJECT_CLASS(xfdashboard_drop_targets_parent_class)->dispose(inObject);
}

/* Class initialization
 * Override functions in parent classes and define properties
 * and signals
 */
static void xfdashboard_drop_targets_class_init(XfdashboardDropTargetsClass *klass)
{
	GObjectClass			*gobjectClass=G_OBJECT_CLASS(klass);

	/* Override functions */
	gobjectClass->dispose=xfdashboard_drop_targets_dispose;

	/* Set up private structure */
	g_type_class_add_private(klass, sizeof(XfdashboardDropTargetsPrivate));
}

/* Object initialization
 * Create private structure and set up default values
 */
static void xfdashboard_drop_targets_init(XfdashboardDropTargets *self)
{
	XfdashboardDropTargetsPrivate	*priv;

	/* Set up default values */
	priv=self->priv=XFDASHBOARD_DROP_TARGETS_GET_PRIVATE(self);

	priv->targets=NULL;
}

/* Implementation: Public API */

/* Get default instance (there is and should be only one) */
XfdashboardDropTargets* xfdashboard_drop_targets_get_default(void)
{
	static XfdashboardDropTargets	*defaultInstance=NULL;

	if(defaultInstance==NULL)
	{
		defaultInstance=g_object_new(XFDASHBOARD_TYPE_DROP_TARGETS, NULL);
	}

	return(defaultInstance);
}

/* Register a new drop target */
void xfdashboard_drop_targets_register(XfdashboardDropAction *inTarget)
{
	XfdashboardDropTargets			*self=xfdashboard_drop_targets_get_default();
	XfdashboardDropTargetsPrivate	*priv;
	ClutterActor					*targetActor;

	g_return_if_fail(self!=NULL);
	g_return_if_fail(XFDASHBOARD_IS_DROP_TARGETS(self));
	g_return_if_fail(XFDASHBOARD_IS_DROP_ACTION(inTarget));

	/* Get private structure */
	priv=self->priv;

	/* Check if target is already registered */
	if(g_slist_find(priv->targets, inTarget))
	{
		g_warning("Target %p (%s) is already registered", (void*)inTarget, G_OBJECT_TYPE_NAME(inTarget));
		return;
	}

	/* Add object to list of dropable targets */
	priv->targets=g_slist_prepend(priv->targets, inTarget);
}

/* Unregister a drop target */
void xfdashboard_drop_targets_unregister(XfdashboardDropAction *inTarget)
{
	XfdashboardDropTargets			*self=xfdashboard_drop_targets_get_default();
	XfdashboardDropTargetsPrivate	*priv;
	XfdashboardDropAction			*dropAction=XFDASHBOARD_DROP_ACTION(inTarget);
	ClutterActor					*targetActor;

	g_return_if_fail(self!=NULL);
	g_return_if_fail(XFDASHBOARD_IS_DROP_TARGETS(self));
	g_return_if_fail(XFDASHBOARD_IS_DROP_ACTION(dropAction));

	/* Get private structure */
	priv=self->priv;

	/* Remove target from list of dropable targets */
	priv->targets=g_slist_remove(priv->targets, inTarget);
}

/* Returns a new list of all drop targets. Caller is responsible to unref each
 * drop target in list and to free list with g_slist_free(), e.g.
 * g_slist_free_full(returned_list, g_object_unref);
 */
GSList* xfdashboard_drop_targets_get_all(void)
{
	XfdashboardDropTargets			*self=xfdashboard_drop_targets_get_default();
	XfdashboardDropTargetsPrivate	*priv;
	GSList							*list;
	GSList							*copy;

	g_return_val_if_fail(self!=NULL, NULL);
	g_return_val_if_fail(XFDASHBOARD_IS_DROP_TARGETS(self), NULL);

	/* Get private structure */
	priv=self->priv;

	/* Create a deeply copied list of currently registered drop targets */
	copy=NULL;
	for(list=priv->targets; list; list=list->next)
	{
		copy=g_slist_prepend(copy, g_object_ref(list->data));
	}

	return(copy);
}
