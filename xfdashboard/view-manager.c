/*
 * view-manager: Single-instance managing views
 * 
 * Copyright 2012-2015 Stephan Haller <nomad@froevel.de>
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

#include "view-manager.h"

#include <glib/gi18n-lib.h>

#include "view.h"
#include "utils.h"

/* Define this class in GObject system */
G_DEFINE_TYPE(XfdashboardViewManager,
				xfdashboard_view_manager,
				G_TYPE_OBJECT)

/* Private structure - access only by public API if needed */
#define XFDASHBOARD_VIEW_MANAGER_GET_PRIVATE(obj) \
	(G_TYPE_INSTANCE_GET_PRIVATE((obj), XFDASHBOARD_TYPE_VIEW_MANAGER, XfdashboardViewManagerPrivate))

struct _XfdashboardViewManagerPrivate
{
	/* Instance related */
	GList		*registeredViews;
};

/* Signals */
enum
{
	SIGNAL_REGISTERED,
	SIGNAL_UNREGISTERED,

	SIGNAL_LAST
};

static guint XfdashboardViewManagerSignals[SIGNAL_LAST]={ 0, };

/* IMPLEMENTATION: Private variables and methods */

/* Single instance of view manager */
static XfdashboardViewManager*		_xfdashboard_view_manager=NULL;

/* IMPLEMENTATION: GObject */

/* Dispose this object */
static void _xfdashboard_view_manager_dispose_unregister_view(gpointer inData, gpointer inUserData)
{
	g_return_if_fail(XFDASHBOARD_IS_VIEW_MANAGER(inUserData));

	xfdashboard_view_manager_unregister(XFDASHBOARD_VIEW_MANAGER(inUserData), GPOINTER_TO_GTYPE(inData));
}

static void _xfdashboard_view_manager_dispose(GObject *inObject)
{
	XfdashboardViewManager			*self=XFDASHBOARD_VIEW_MANAGER(inObject);
	XfdashboardViewManagerPrivate	*priv=self->priv;

	/* Release allocated resouces */
	if(priv->registeredViews)
	{
		g_list_foreach(priv->registeredViews, _xfdashboard_view_manager_dispose_unregister_view, self);
		g_list_free(priv->registeredViews);
		priv->registeredViews=NULL;
	}

	/* Unset singleton */
	if(G_LIKELY(G_OBJECT(_xfdashboard_view_manager)==inObject)) _xfdashboard_view_manager=NULL;

	/* Call parent's class dispose method */
	G_OBJECT_CLASS(xfdashboard_view_manager_parent_class)->dispose(inObject);
}

/* Class initialization
 * Override functions in parent classes and define properties
 * and signals
 */
static void xfdashboard_view_manager_class_init(XfdashboardViewManagerClass *klass)
{
	GObjectClass		*gobjectClass=G_OBJECT_CLASS(klass);

	/* Override functions */
	gobjectClass->dispose=_xfdashboard_view_manager_dispose;

	/* Set up private structure */
	g_type_class_add_private(klass, sizeof(XfdashboardViewManagerPrivate));

	/* Define signals */
	XfdashboardViewManagerSignals[SIGNAL_REGISTERED]=
		g_signal_new("registered",
						G_TYPE_FROM_CLASS(klass),
						G_SIGNAL_RUN_LAST,
						G_STRUCT_OFFSET(XfdashboardViewManagerClass, registered),
						NULL,
						NULL,
						g_cclosure_marshal_VOID__OBJECT,
						G_TYPE_NONE,
						1,
						G_TYPE_GTYPE);

	XfdashboardViewManagerSignals[SIGNAL_UNREGISTERED]=
		g_signal_new("unregistered",
						G_TYPE_FROM_CLASS(klass),
						G_SIGNAL_RUN_LAST,
						G_STRUCT_OFFSET(XfdashboardViewManagerClass, unregistered),
						NULL,
						NULL,
						g_cclosure_marshal_VOID__OBJECT,
						G_TYPE_NONE,
						1,
						G_TYPE_GTYPE);
}

/* Object initialization
 * Create private structure and set up default values
 */
static void xfdashboard_view_manager_init(XfdashboardViewManager *self)
{
	XfdashboardViewManagerPrivate	*priv;

	priv=self->priv=XFDASHBOARD_VIEW_MANAGER_GET_PRIVATE(self);

	/* Set default values */
	priv->registeredViews=NULL;
}

/* IMPLEMENTATION: Public API */

/* Get single instance of manager */
XfdashboardViewManager* xfdashboard_view_manager_get_default(void)
{
	if(G_UNLIKELY(_xfdashboard_view_manager==NULL))
	{
		_xfdashboard_view_manager=g_object_new(XFDASHBOARD_TYPE_VIEW_MANAGER, NULL);
	}
		else g_object_ref(_xfdashboard_view_manager);

	return(_xfdashboard_view_manager);
}

/* Register a view */
void xfdashboard_view_manager_register(XfdashboardViewManager *self, GType inViewType)
{
	XfdashboardViewManagerPrivate	*priv;

	g_return_if_fail(XFDASHBOARD_IS_VIEW_MANAGER(self));

	priv=self->priv;

	/* Check if given type is not a XfdashboardView but a derived type from it */
	if(inViewType==XFDASHBOARD_TYPE_VIEW ||
		g_type_is_a(inViewType, XFDASHBOARD_TYPE_VIEW)!=TRUE)
	{
		g_warning(_("View %s is not a %s and cannot be registered"),
					g_type_name(inViewType),
					g_type_name(XFDASHBOARD_TYPE_VIEW));
		return;
	}

	/* Register type if not already registered */
	if(g_list_find(priv->registeredViews, GTYPE_TO_POINTER(inViewType))==NULL)
	{
		g_debug("Registering view %s", g_type_name(inViewType));
		priv->registeredViews=g_list_append(priv->registeredViews, GTYPE_TO_POINTER(inViewType));
		g_signal_emit(self, XfdashboardViewManagerSignals[SIGNAL_REGISTERED], 0, inViewType);
	}
}

/* Unregister a view */
void xfdashboard_view_manager_unregister(XfdashboardViewManager *self, GType inViewType)
{
	XfdashboardViewManagerPrivate	*priv;

	g_return_if_fail(XFDASHBOARD_IS_VIEW_MANAGER(self));

	priv=self->priv;

	/* Check if given type is not a XfdashboardView but a derived type from it */
	if(inViewType==XFDASHBOARD_TYPE_VIEW ||
		g_type_is_a(inViewType, XFDASHBOARD_TYPE_VIEW)!=TRUE)
	{
		g_warning(_("View %s is not a %s and cannot be unregistered"),
					g_type_name(inViewType),
					g_type_name(XFDASHBOARD_TYPE_VIEW));
		return;
	}

	/* Unregister type if registered */
	if(g_list_find(priv->registeredViews, GTYPE_TO_POINTER(inViewType))!=NULL)
	{
		g_debug("Unregistering view %s", g_type_name(inViewType));
		priv->registeredViews=g_list_remove(priv->registeredViews, GTYPE_TO_POINTER(inViewType));
		g_signal_emit(self, XfdashboardViewManagerSignals[SIGNAL_UNREGISTERED], 0, inViewType);
	}
}

/* Get list of registered views types.
 * Returned GList must be freed with g_list_free() by caller.
 */
GList* xfdashboard_view_manager_get_registered(XfdashboardViewManager *self)
{
	GList		*copy;

	g_return_val_if_fail(XFDASHBOARD_IS_VIEW_MANAGER(self), NULL);

	/* Return a copy of list of registered view types */
	copy=g_list_copy(self->priv->registeredViews);
	return(copy);
}
