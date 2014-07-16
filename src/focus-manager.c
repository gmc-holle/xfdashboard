/*
 * focus-manager: Single-instance managing focusable actors
 *                for keyboard navigation
 * 
 * Copyright 2012-2014 Stephan Haller <nomad@froevel.de>
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

#include "focus-manager.h"

#include <glib/gi18n-lib.h>

#include "marshal.h"
#include "stylable.h"

/* Define this class in GObject system */
G_DEFINE_TYPE(XfdashboardFocusManager,
				xfdashboard_focus_manager,
				G_TYPE_OBJECT)

/* Private structure - access only by public API if needed */
#define XFDASHBOARD_FOCUS_MANAGER_GET_PRIVATE(obj) \
	(G_TYPE_INSTANCE_GET_PRIVATE((obj), XFDASHBOARD_TYPE_FOCUS_MANAGER, XfdashboardFocusManagerPrivate))

struct _XfdashboardFocusManagerPrivate
{
	/* Instance related */
	GList					*registeredFocusables;
	XfdashboardFocusable	*currentFocus;
};

/* Signals */
enum
{
	SIGNAL_REGISTERED,
	SIGNAL_UNREGISTERED,

	SIGNAL_CHANGED,

	SIGNAL_LAST
};

static guint XfdashboardFocusManagerSignals[SIGNAL_LAST]={ 0, };

/* IMPLEMENTATION: Private variables and methods */

/* Single instance of view manager */
static XfdashboardFocusManager*		_xfdashboard_focus_manager=NULL;

/* A registered focusable actor is going to be destroyed so unregister it */
static void _xfdashboard_focus_manager_on_focusable_destroy(XfdashboardFocusManager *self,
															gpointer inUserData)
{
	XfdashboardFocusable			*focusable;

	g_return_if_fail(XFDASHBOARD_IS_FOCUS_MANAGER(self));
	g_return_if_fail(XFDASHBOARD_IS_FOCUSABLE(inUserData));

	focusable=XFDASHBOARD_FOCUSABLE(inUserData);

	/* Unregister going-to-be-destroyed focusable actor */
	xfdashboard_focus_manager_unregister(self, focusable);
}

/* A registered focusable actor is going to be hidden or unrealized */
static void _xfdashboard_focus_manager_on_focusable_hide(XfdashboardFocusManager *self,
															gpointer inUserData)
{
	XfdashboardFocusManagerPrivate	*priv;
	XfdashboardFocusable			*focusable;
	XfdashboardFocusable			*nextFocusable;

	g_return_if_fail(XFDASHBOARD_IS_FOCUS_MANAGER(self));
	g_return_if_fail(XFDASHBOARD_IS_FOCUSABLE(inUserData));

	priv=self->priv;
	focusable=XFDASHBOARD_FOCUSABLE(inUserData);

	/* Only move focus if hidden or unrealized focusable actor is the one
	 * which has the focus currently.
	 */
	if(priv->currentFocus!=focusable) return;

	if(CLUTTER_ACTOR_IS_MAPPED(CLUTTER_ACTOR(focusable)) &&
		CLUTTER_ACTOR_IS_REALIZED(CLUTTER_ACTOR(focusable)) &&
		CLUTTER_ACTOR_IS_VISIBLE(CLUTTER_ACTOR(focusable)))
	{
		return;
	}

	/* Move focus to next focusable actor if this actor which has the current focus
	 * is going to be unrealized or hidden.
	 */
	nextFocusable=xfdashboard_focus_manager_get_next_focusable(self, priv->currentFocus);
	if(nextFocusable && nextFocusable!=priv->currentFocus) xfdashboard_focus_manager_set_focus(self, nextFocusable);
		else
		{
			xfdashboard_focusable_unset_focus(priv->currentFocus);
			priv->currentFocus=NULL;
		}
}

/* IMPLEMENTATION: GObject */

/* Dispose this object */
static void _xfdashboard_focus_manager_dispose_unregister_focusable(gpointer inData, gpointer inUserData)
{
	XfdashboardFocusManager			*self;
	XfdashboardFocusable			*focusable;

	g_return_if_fail(XFDASHBOARD_IS_FOCUS_MANAGER(inUserData));
	g_return_if_fail(XFDASHBOARD_IS_FOCUSABLE(inData));

	self=XFDASHBOARD_FOCUS_MANAGER(inUserData);
	focusable=XFDASHBOARD_FOCUSABLE(inData);

	/* Unregister focusable actor but do not call general unregister function
	 * to avoid spamming focus changes and to avoid modifying list of focusable
	 * actor while iterating through it.
	 */
	g_signal_handlers_disconnect_by_func(focusable,
											G_CALLBACK(_xfdashboard_focus_manager_on_focusable_destroy),
											self);
	g_signal_handlers_disconnect_by_func(focusable,
											G_CALLBACK(_xfdashboard_focus_manager_on_focusable_hide),
											self);

	g_signal_emit(self, XfdashboardFocusManagerSignals[SIGNAL_UNREGISTERED], 0, focusable);
}

static void _xfdashboard_focus_manager_dispose(GObject *inObject)
{
	XfdashboardFocusManager			*self=XFDASHBOARD_FOCUS_MANAGER(inObject);
	XfdashboardFocusManagerPrivate	*priv=self->priv;

	/* Release allocated resouces */
	if(priv->registeredFocusables)
	{
		g_list_foreach(priv->registeredFocusables, _xfdashboard_focus_manager_dispose_unregister_focusable, self);
		g_list_free(priv->registeredFocusables);
		priv->registeredFocusables=NULL;
	}

	/* Unset singleton */
	if(G_LIKELY(G_OBJECT(_xfdashboard_focus_manager)==inObject)) _xfdashboard_focus_manager=NULL;

	/* Call parent's class dispose method */
	G_OBJECT_CLASS(xfdashboard_focus_manager_parent_class)->dispose(inObject);
}

/* Class initialization
 * Override functions in parent classes and define properties
 * and signals
 */
static void xfdashboard_focus_manager_class_init(XfdashboardFocusManagerClass *klass)
{
	GObjectClass		*gobjectClass=G_OBJECT_CLASS(klass);

	/* Override functions */
	gobjectClass->dispose=_xfdashboard_focus_manager_dispose;

	/* Set up private structure */
	g_type_class_add_private(klass, sizeof(XfdashboardFocusManagerPrivate));

	/* Define signals */
	XfdashboardFocusManagerSignals[SIGNAL_REGISTERED]=
		g_signal_new("registered",
						G_TYPE_FROM_CLASS(klass),
						G_SIGNAL_RUN_LAST,
						G_STRUCT_OFFSET(XfdashboardFocusManagerClass, registered),
						NULL,
						NULL,
						g_cclosure_marshal_VOID__OBJECT,
						G_TYPE_NONE,
						1,
						XFDASHBOARD_TYPE_FOCUSABLE);

	XfdashboardFocusManagerSignals[SIGNAL_UNREGISTERED]=
		g_signal_new("unregistered",
						G_TYPE_FROM_CLASS(klass),
						G_SIGNAL_RUN_LAST,
						G_STRUCT_OFFSET(XfdashboardFocusManagerClass, unregistered),
						NULL,
						NULL,
						g_cclosure_marshal_VOID__OBJECT,
						G_TYPE_NONE,
						1,
						XFDASHBOARD_TYPE_FOCUSABLE);

	XfdashboardFocusManagerSignals[SIGNAL_CHANGED]=
		g_signal_new("changed",
						G_TYPE_FROM_CLASS(klass),
						G_SIGNAL_RUN_LAST,
						G_STRUCT_OFFSET(XfdashboardFocusManagerClass, changed),
						NULL,
						NULL,
						_xfdashboard_marshal_VOID__OBJECT_OBJECT,
						G_TYPE_NONE,
						2,
						XFDASHBOARD_TYPE_FOCUSABLE,
						XFDASHBOARD_TYPE_FOCUSABLE);
}

/* Object initialization
 * Create private structure and set up default values
 */
static void xfdashboard_focus_manager_init(XfdashboardFocusManager *self)
{
	XfdashboardFocusManagerPrivate	*priv;

	priv=self->priv=XFDASHBOARD_FOCUS_MANAGER_GET_PRIVATE(self);

	/* Set default values */
	priv->registeredFocusables=NULL;
	priv->currentFocus=NULL;
}

/* IMPLEMENTATION: Public API */

/* Get single instance of manager */
XfdashboardFocusManager* xfdashboard_focus_manager_get_default(void)
{
	if(G_UNLIKELY(_xfdashboard_focus_manager==NULL))
	{
		_xfdashboard_focus_manager=g_object_new(XFDASHBOARD_TYPE_FOCUS_MANAGER, NULL);
	}
		else g_object_ref(_xfdashboard_focus_manager);

	return(_xfdashboard_focus_manager);
}

/* Register a focusable actor */
void xfdashboard_focus_manager_register(XfdashboardFocusManager *self, XfdashboardFocusable *inFocusable)
{
	XfdashboardFocusManagerPrivate	*priv;

	g_return_if_fail(XFDASHBOARD_IS_FOCUS_MANAGER(self));
	g_return_if_fail(inFocusable);

	priv=self->priv;

	/* Check if given focusable actor is really focusable and stylable */
	if(!XFDASHBOARD_IS_FOCUSABLE(inFocusable))
	{
		g_warning(_("Object %s does not inherit %s and cannot be registered"),
					G_OBJECT_TYPE_NAME(inFocusable),
					g_type_name(XFDASHBOARD_TYPE_FOCUSABLE));
		return;
	}

	if(!XFDASHBOARD_IS_STYLABLE(inFocusable))
	{
		g_warning(_("Object %s does not inherit %s and cannot be registered"),
					G_OBJECT_TYPE_NAME(inFocusable),
					g_type_name(XFDASHBOARD_TYPE_STYLABLE));
		return;
	}

	/* Register focusable actor if not already registered */
	if(g_list_find(priv->registeredFocusables, inFocusable)==NULL)
	{
		g_debug("Registering focusable %s", G_OBJECT_TYPE_NAME(inFocusable));

		/* Add focusable actor to list of registered focusable actors */
		priv->registeredFocusables=g_list_append(priv->registeredFocusables, inFocusable);

		/* Connect to signals to get notified if actor is going to be destroy,
		 * unrealized or hidden to remove it from list of focusable actors.
		 */
		g_signal_connect_swapped(inFocusable,
									"destroy",
									G_CALLBACK(_xfdashboard_focus_manager_on_focusable_destroy),
									self);
		g_signal_connect_swapped(inFocusable,
									"realize",
									G_CALLBACK(_xfdashboard_focus_manager_on_focusable_hide),
									self);
		g_signal_connect_swapped(inFocusable,
									"hide",
									G_CALLBACK(_xfdashboard_focus_manager_on_focusable_hide),
									self);

		/* Emit signal */
		g_signal_emit(self, XfdashboardFocusManagerSignals[SIGNAL_REGISTERED], 0, inFocusable);
	}
}

/* Unregister a focusable actor */
void xfdashboard_focus_manager_unregister(XfdashboardFocusManager *self, XfdashboardFocusable *inFocusable)
{
	XfdashboardFocusManagerPrivate	*priv;

	g_return_if_fail(XFDASHBOARD_IS_FOCUS_MANAGER(self));
	g_return_if_fail(inFocusable);

	priv=self->priv;

	/* Unregister type if registered.
	 * We do not need to check if the given actor is focusable or stylable
	 * because it could not be registered if it is not.
	 */
	if(g_list_find(priv->registeredFocusables, inFocusable)!=NULL)
	{
		g_debug("Unregistering focusable %s", G_OBJECT_TYPE_NAME(inFocusable));

		/* If we unregister the focusable actor which has the focus currently
		 * move focus to next focusable actor first but check that we will not
		 * reselect the focusable actor which should be unregistered. That can
		 * happen because this actor is not yet removed from list of registered
		 * focusable actor and is the only selectable one. But it needs to be
		 * still in the list otherwise we could not find the next actor to
		 * focus appropiately.
		 */
		if(inFocusable==priv->currentFocus)
		{
			XfdashboardFocusable	*focusable;

			focusable=xfdashboard_focus_manager_get_next_focusable(self, priv->currentFocus);
			if(focusable && focusable!=priv->currentFocus) xfdashboard_focus_manager_set_focus(self, focusable);
				else
				{
					xfdashboard_focusable_unset_focus(priv->currentFocus);
					priv->currentFocus=NULL;
				}
		}

		/* Remove focusable actor from list of registered focusable actors */
		priv->registeredFocusables=g_list_remove(priv->registeredFocusables, inFocusable);

		/* Disconnect from signals because we are not interested in this actor anymore */
		g_signal_handlers_disconnect_by_func(inFocusable,
												G_CALLBACK(_xfdashboard_focus_manager_on_focusable_destroy),
												self);
		g_signal_handlers_disconnect_by_func(inFocusable,
												G_CALLBACK(_xfdashboard_focus_manager_on_focusable_hide),
												self);

		/* Emit signal */
		g_signal_emit(self, XfdashboardFocusManagerSignals[SIGNAL_UNREGISTERED], 0, inFocusable);
	}
}

/* Get list of registered views types.
 * Returned GList must be freed with g_list_free() by caller.
 */
GList* xfdashboard_focus_manager_get_registered(XfdashboardFocusManager *self)
{
	XfdashboardFocusManagerPrivate	*priv;
	GList							*copy;

	g_return_val_if_fail(XFDASHBOARD_IS_FOCUS_MANAGER(self), NULL);

	priv=self->priv;

	/* Return a copy of list of registered view types */
	copy=g_list_copy(priv->registeredFocusables);
	return(copy);
}

/* Check if given focusable actor is registered */
gboolean xfdashboard_focus_manager_is_registered(XfdashboardFocusManager *self, XfdashboardFocusable *inFocusable)
{
	XfdashboardFocusManagerPrivate	*priv;

	g_return_val_if_fail(XFDASHBOARD_IS_FOCUS_MANAGER(self), FALSE);
	g_return_val_if_fail(XFDASHBOARD_IS_FOCUSABLE(inFocusable), FALSE);

	priv=self->priv;

	/* If given focusable actor is in list of registered ones return TRUE */
	if(g_list_find(priv->registeredFocusables, inFocusable)!=NULL) return(TRUE);

	/* If here get here the given focusable actor is not registered */
	return(FALSE);
}

/* Determine if a specific actor has the focus */
gboolean xfdashboard_focus_manager_has_focus(XfdashboardFocusManager *self, XfdashboardFocusable *inFocusable)
{
	XfdashboardFocusManagerPrivate	*priv;

	g_return_val_if_fail(XFDASHBOARD_IS_FOCUS_MANAGER(self), FALSE);
	g_return_val_if_fail(XFDASHBOARD_IS_FOCUSABLE(inFocusable), FALSE);

	priv=self->priv;

	/* Return TRUE if given actor has the focus otherwise return FALSE */
	return(priv->currentFocus==inFocusable ? TRUE : FALSE);
}

/* Get focusable actor which has the focus currently */
XfdashboardFocusable* xfdashboard_focus_manager_get_focus(XfdashboardFocusManager *self)
{
	XfdashboardFocusManagerPrivate	*priv;

	g_return_val_if_fail(XFDASHBOARD_IS_FOCUS_MANAGER(self), NULL);

	priv=self->priv;

	/* Return found focused focusable actor */
	return(priv->currentFocus);
}

/* Set focus to a registered focusable actor */
void xfdashboard_focus_manager_set_focus(XfdashboardFocusManager *self, XfdashboardFocusable *inFocusable)
{
	XfdashboardFocusManagerPrivate	*priv;
	XfdashboardFocusable			*oldFocusable;

	g_return_if_fail(XFDASHBOARD_IS_FOCUS_MANAGER(self));
	g_return_if_fail(XFDASHBOARD_IS_FOCUSABLE(inFocusable));

	priv=self->priv;
	oldFocusable=NULL;

	/* Check if focusable actor is really registered */
	if(g_list_find(priv->registeredFocusables, inFocusable)==NULL)
	{
		g_warning(_("Trying to focus an unregistered focusable actor"));
		return;
	}

	/* Unset focus at current focused actor */
	oldFocusable=priv->currentFocus;
	if(priv->currentFocus)
	{
		xfdashboard_focusable_unset_focus(priv->currentFocus);
	}

	/* Set focus */
	priv->currentFocus=inFocusable;
	xfdashboard_focusable_set_focus(priv->currentFocus);

	/* Emit signal for changed focus */
	g_signal_emit(self, XfdashboardFocusManagerSignals[SIGNAL_CHANGED], 0, oldFocusable, priv->currentFocus);
}

/* Find next focusable actor from given focusable actor */
XfdashboardFocusable* xfdashboard_focus_manager_get_next_focusable(XfdashboardFocusManager *self,
																	XfdashboardFocusable *inBeginFocusable)
{
	XfdashboardFocusManagerPrivate	*priv;
	GList							*startIteration;
	GList							*iter;
	XfdashboardFocusable			*focusable;

	g_return_val_if_fail(XFDASHBOARD_IS_FOCUS_MANAGER(self), NULL);
	g_return_val_if_fail(!inBeginFocusable || XFDASHBOARD_IS_FOCUSABLE(inBeginFocusable), NULL);

	priv=self->priv;
	startIteration=NULL;

	/* Find starting point of iteration.
	 * If starting focusable actor for search for next focusable actor is NULL
	 * or if it is not registered start search at begin of list of focusable actors.
	 */
	if(inBeginFocusable) startIteration=g_list_find(priv->registeredFocusables, inBeginFocusable);
	if(startIteration) startIteration=g_list_next(startIteration);
		else startIteration=priv->registeredFocusables;

	/* Iterate through list of registered focusable actors beginning at
	 * given focusable actor (might be begin of this list) and return
	 * the first focusable actor which can be focused.
	 */
	for(iter=startIteration; iter; iter=g_list_next(iter))
	{
		focusable=(XfdashboardFocusable*)iter->data;

		/* If focusable can be focused then return it */
		if(xfdashboard_focusable_can_focus(focusable)) return(focusable);
	}

	/* If we get here we have to continue search at the beginning of list
	 * of registered focusable actors. Iterate through list of registered
	 * focusable actors from the beginning of that list up to the given
	 * focusable actor and return the first focusable actor which is focusable.
	 */
	for(iter=priv->registeredFocusables; iter!=startIteration; iter=g_list_next(iter))
	{
		focusable=(XfdashboardFocusable*)iter->data;

		/* If focusable can be focused then return it */
		if(xfdashboard_focusable_can_focus(focusable)) return(focusable);
	}

	/* If we get here we could not find next focusable actor */
	return(NULL);
}

/* Find previous focusable actor from given focusable actor */
XfdashboardFocusable* xfdashboard_focus_manager_get_previous_focusable(XfdashboardFocusManager *self,
																		XfdashboardFocusable *inBeginFocusable)
{
	XfdashboardFocusManagerPrivate	*priv;
	GList							*startIteration;
	GList							*iter;
	XfdashboardFocusable			*focusable;

	g_return_val_if_fail(XFDASHBOARD_IS_FOCUS_MANAGER(self), NULL);
	g_return_val_if_fail(!inBeginFocusable || XFDASHBOARD_IS_FOCUSABLE(inBeginFocusable), NULL);

	priv=self->priv;
	startIteration=NULL;

	/* Find starting point of iteration.
	 * If starting focusable actor for search for next focusable actor is NULL
	 * or if it is not registered start search at begin of list of focusable actors.
	 */
	if(inBeginFocusable) startIteration=g_list_find(priv->registeredFocusables, inBeginFocusable);
	if(startIteration) startIteration=g_list_previous(startIteration);
		else startIteration=priv->registeredFocusables;

	/* Iterate reverse through list of registered focusable actors beginning
	 * at given focusable actor (might be begin of this list) and return
	 * the first focusable actor which can be focused.
	 */
	for(iter=startIteration; iter; iter=g_list_previous(iter))
	{
		focusable=(XfdashboardFocusable*)iter->data;

		/* If focusable can be focused then return it */
		if(xfdashboard_focusable_can_focus(focusable)) return(focusable);
	}

	/* If we get here we have to continue search at the end of list
	 * of registered focusable actors. Iterate reverse through list of
	 * registered focusable actors from the beginning of that list up
	 * to the given focusable actor and return the first focusable actor
	 * which is focusable.
	 */
	for(iter=g_list_last(priv->registeredFocusables); iter!=startIteration; iter=g_list_previous(iter))
	{
		focusable=(XfdashboardFocusable*)iter->data;

		/* If focusable can be focused then return it */
		if(xfdashboard_focusable_can_focus(focusable)) return(focusable);
	}

	/* If we get here we could not find next focusable actor */
	return(NULL);
}

/* Handle key event (it is either key-press or key-release) by focusable actor
 * which has the focus.
 */
gboolean xfdashboard_focus_manager_handle_key_event(XfdashboardFocusManager *self, const ClutterEvent *inEvent)
{
	XfdashboardFocusManagerPrivate	*priv;

	g_return_val_if_fail(XFDASHBOARD_IS_FOCUS_MANAGER(self), CLUTTER_EVENT_PROPAGATE);
	g_return_val_if_fail(inEvent, CLUTTER_EVENT_PROPAGATE);
	g_return_val_if_fail(clutter_event_type(inEvent)==CLUTTER_KEY_PRESS || clutter_event_type(inEvent)==CLUTTER_KEY_RELEASE, CLUTTER_EVENT_PROPAGATE);

	priv=self->priv;

	/* Synthesize event for current focused focusable actor */
	if(priv->currentFocus)
	{
		return(xfdashboard_focusable_handle_key_event(priv->currentFocus, inEvent));
	}

	/* If we get here there is no focus set */
	return(CLUTTER_EVENT_PROPAGATE);
}
