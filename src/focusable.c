/*
 * focusable: An interface which can be inherited by actors to get
 *            managed by focus manager for keyboard navigation and
 *            selection handling
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

#include "focusable.h"

#include <clutter/clutter.h>
#include <glib/gi18n-lib.h>

#include "utils.h"
#include "stylable.h"
#include "marshal.h"
#include "focus-manager.h"
#include "application.h"

/* Define this interface in GObject system */
G_DEFINE_INTERFACE(XfdashboardFocusable,
					xfdashboard_focusable,
					G_TYPE_OBJECT)

/* Signals */
enum
{
	SIGNAL_FOCUS_SET,
	SIGNAL_FOCUS_UNSET,

	SIGNAL_SELECTION_CHANGED,

	SIGNAL_LAST
};

static guint XfdashboardFocusableSignals[SIGNAL_LAST]={ 0, };

/* IMPLEMENTATION: Private variables and methods */
#define XFDASHBOARD_FOCUSABLE_WARN_NOT_IMPLEMENTED(self, vfunc) \
	g_warning(_("Object of type %s does not implement required virtual function XfdashboardFocusable::%s"), \
				G_OBJECT_TYPE_NAME(self), \
				vfunc);

/* Default implementation of virtual function "can_focus" */
static gboolean _xfdashboard_focusable_real_can_focus(XfdashboardFocusable *self)
{
	g_return_val_if_fail(XFDASHBOARD_IS_FOCUSABLE(self), FALSE);

	/* By default (if not overidden) an focusable actor cannot be focused */
	return(FALSE);
}

/* Default implementation of virtual function "supports_selection" */
static gboolean _xfdashboard_focusable_real_supports_selection(XfdashboardFocusable *self)
{
	g_return_val_if_fail(XFDASHBOARD_IS_FOCUSABLE(self), FALSE);

	/* By default (if not overidden) this actor does not support selection */
	return(FALSE);
}

/* Default implementation of virtual function "activate_selection" */
static gboolean _xfdashboard_focusable_real_activate_selection(XfdashboardFocusable *self, ClutterActor *inSelection)
{
	g_return_val_if_fail(XFDASHBOARD_IS_FOCUSABLE(self), FALSE);

	/* By default (if not overidden) this actor cannot activate any selection */
	return(FALSE);
}

/* Check if this focusable actor has the focus */
static gboolean _xfdashboard_focusable_has_focus(XfdashboardFocusable *self)
{
	XfdashboardFocusManager		*focusManager;
	gboolean					hasFocus;

	g_return_val_if_fail(XFDASHBOARD_IS_FOCUSABLE(self), FALSE);

	hasFocus=FALSE;

	/* Ask focus manager which actor has the current focus and
	 * check if it is this focusable actor.
	 */
	focusManager=xfdashboard_focus_manager_get_default();
	hasFocus=xfdashboard_focus_manager_has_focus(focusManager, self);
	g_object_unref(focusManager);

	/* If focus manager says this focusable has not the focus then
	 * it might a proxy who has the focus (as seen by focus manager)
	 * but in real this focusable actor is the destination of the
	 * proxy so check for style class "focus" being set.
	 */
	if(!hasFocus &&
		XFDASHBOARD_IS_STYLABLE(self) &&
		xfdashboard_stylable_has_class(XFDASHBOARD_STYLABLE(self), "focus"))
	{
		hasFocus=TRUE;
	}

	return(hasFocus);
}

/* The current selection of a focusable actor (if focussed or not) is not available anymore
 * (e.g. hidden or destroyed). So move selection at focusable actor to next available and
 * selectable item.
 */
static void _xfdashboard_focusable_on_selection_unavailable(XfdashboardFocusable *self,
															gpointer inUserData)
{
	XfdashboardFocusableInterface		*iface;
	ClutterActor						*oldSelection;
	ClutterActor						*newSelection;
	gboolean							success;
	XfdashboardApplication				*application;

	g_return_if_fail(XFDASHBOARD_IS_FOCUSABLE(self));
	g_return_if_fail(CLUTTER_IS_ACTOR(inUserData));

	iface=XFDASHBOARD_FOCUSABLE_GET_IFACE(self);
	oldSelection=CLUTTER_ACTOR(inUserData);
	newSelection=NULL;
	success=FALSE;

	/* If application is not quitting then call virtual function to set selection
	 * which have to be available because this signal handler was set in
	 * xfdashboard_focusable_set_selection() when this virtual function was available
	 * and successfully called.
	 * If setting new selection was unsuccessful we set selection to nothing (NULL);
	 */
	application=xfdashboard_application_get_default();
	if(!xfdashboard_application_is_quitting(application))
	{
		/* Get next selection */
		newSelection=xfdashboard_focusable_find_selection(self, oldSelection, XFDASHBOARD_SELECTION_TARGET_NEXT);

		/* Call virtual function to set selection which have to be available
		 * because this signal handler was set in xfdashboard_focusable_set_selection()
		 * when this virtual function was available and successfully called.
		 * If setting new selection was unsuccessful we set selection to nothing (NULL);
		 */
		success=iface->set_selection(self, newSelection);
		if(!success)
		{
			success=iface->set_selection(self, newSelection);
			if(!success)
			{
				g_critical(_("Old selection %s at %s is unavailable but setting new selection either to %s or nothing failed!"),
							G_OBJECT_TYPE_NAME(oldSelection),
							G_OBJECT_TYPE_NAME(self),
							newSelection ? G_OBJECT_TYPE_NAME(newSelection) : "<nil>");
			}

			/* Now reset new selection to NULL regardless if setting selection at
			 * focusable actor was successful or not. A critical warning was displayed
			 * if is was unsuccessful because setting nothing (NULL) must succeed usually.
			 */
			newSelection=NULL;
		}
	}

	/* Regardless if setting selection was successful, remove signal handlers
	 * and styles from old selection.
	 */
	if(oldSelection)
	{
		/* Remove signal handlers at old selection*/
		g_signal_handlers_disconnect_by_func(oldSelection,
												G_CALLBACK(_xfdashboard_focusable_on_selection_unavailable),
												self);

		/* Remove style from old selection */
		if(XFDASHBOARD_IS_STYLABLE(oldSelection))
		{
			xfdashboard_stylable_remove_pseudo_class(XFDASHBOARD_STYLABLE(oldSelection), "selected");
		}
	}

	/* If setting selection was successful, set up signal handlers and styles at new selection */
	if(success && newSelection)
	{
		/* Set up signal handlers to get notified if new selection
		 * is going to be unavailable (e.g. hidden or destroyed)
		 */
		g_signal_connect_swapped(newSelection,
									"destroy",
									G_CALLBACK(_xfdashboard_focusable_on_selection_unavailable),
									self);
		g_signal_connect_swapped(newSelection,
									"hide",
									G_CALLBACK(_xfdashboard_focusable_on_selection_unavailable),
									self);

		/* Check if this focusable actor has the focus because if it has
		 * the have to style new selection.
		 */
		if(_xfdashboard_focusable_has_focus(self) &&
			XFDASHBOARD_IS_STYLABLE(newSelection))
		{
			xfdashboard_stylable_add_pseudo_class(XFDASHBOARD_STYLABLE(newSelection), "selected");
		}
	}

	/* Emit signal because at least old selection has changed */
	g_signal_emit(self, XfdashboardFocusableSignals[SIGNAL_SELECTION_CHANGED], 0, oldSelection, newSelection);
}

/* Key was pressed */
static gboolean _xfdashboard_focusable_handle_keypress_event(XfdashboardFocusable *self, const ClutterEvent *inEvent)
{
	g_return_val_if_fail(XFDASHBOARD_IS_FOCUSABLE(self), CLUTTER_EVENT_PROPAGATE);
	g_return_val_if_fail(inEvent, CLUTTER_EVENT_PROPAGATE);
	g_return_val_if_fail(clutter_event_type(inEvent)==CLUTTER_KEY_PRESS, CLUTTER_EVENT_PROPAGATE);

	/* If actor supports selection intercept keys which modify a selection */
	if(xfdashboard_focusable_supports_selection(self))
	{
		XfdashboardSelectionTarget		direction;

		direction=XFDASHBOARD_SELECTION_TARGET_NONE;

		/* Find target direction depending on pressed key but no modifier
		 * must be pressed
		 */
		if(!(inEvent->key.modifier_state & CLUTTER_MODIFIER_MASK))
		{
			switch(inEvent->key.keyval)
			{
				case CLUTTER_KEY_Left:
					direction=XFDASHBOARD_SELECTION_TARGET_LEFT;
					break;

				case CLUTTER_KEY_Right:
					direction=XFDASHBOARD_SELECTION_TARGET_RIGHT;
					break;

				case CLUTTER_KEY_Up:
					direction=XFDASHBOARD_SELECTION_TARGET_UP;
					break;

				case CLUTTER_KEY_Down:
					direction=XFDASHBOARD_SELECTION_TARGET_DOWN;
					break;

				case CLUTTER_KEY_Home:
				case CLUTTER_KEY_KP_Home:
					direction=XFDASHBOARD_SELECTION_TARGET_FIRST;
					break;

				case CLUTTER_KEY_End:
				case CLUTTER_KEY_KP_End:
					direction=XFDASHBOARD_SELECTION_TARGET_LAST;
					break;

				default:
					break;
			}
		}

		/* If we could determine the target direction for new selection,
		 * try to find and set it.
		 */
		if(direction!=XFDASHBOARD_SELECTION_TARGET_NONE)
		{
			ClutterActor				*currentSelection;
			ClutterActor				*newSelection;

			/* Find new selection */
			currentSelection=xfdashboard_focusable_get_selection(self);
			newSelection=xfdashboard_focusable_find_selection(self, currentSelection, direction);

			/* Set new selection */
			xfdashboard_focusable_set_selection(self, newSelection);

			/* All done so return and stop further processing of this event */
			return(CLUTTER_EVENT_STOP);
		}
	}

	/* Event was not handled so synthesize event to this focusable actor */
	return(clutter_actor_event(CLUTTER_ACTOR(self), inEvent, FALSE));
}

/* Key was released */
static gboolean _xfdashboard_focusable_handle_keyrelease_event(XfdashboardFocusable *self, const ClutterEvent *inEvent)
{
	g_return_val_if_fail(XFDASHBOARD_IS_FOCUSABLE(self), CLUTTER_EVENT_PROPAGATE);
	g_return_val_if_fail(inEvent, CLUTTER_EVENT_PROPAGATE);
	g_return_val_if_fail(clutter_event_type(inEvent)==CLUTTER_KEY_RELEASE, CLUTTER_EVENT_PROPAGATE);

	/* If actor supports selection intercept keys which use or activate a selection */
	if(xfdashboard_focusable_supports_selection(self))
	{
		switch(inEvent->key.keyval)
		{
			case CLUTTER_KEY_Return:
			case CLUTTER_KEY_KP_Enter:
			case CLUTTER_KEY_ISO_Enter:
				if(!(inEvent->key.modifier_state & CLUTTER_MODIFIER_MASK))
				{
					ClutterActor		*currentSelection;

					/* Get selection to activate */
					currentSelection=xfdashboard_focusable_get_selection(self);

					/* Activate selection */
					xfdashboard_focusable_activate_selection(self, currentSelection);

					/* All done so return and stop further processing of this event */
					return(CLUTTER_EVENT_STOP);
				}
				break;

			default:
				break;
		}
	}

	/* Event was not handled so synthesize event to this focusable actor */
	return(clutter_actor_event(CLUTTER_ACTOR(self), inEvent, FALSE));
}


/* IMPLEMENTATION: GObject */

/* Interface initialization
 * Set up default functions
 */
void xfdashboard_focusable_default_init(XfdashboardFocusableInterface *iface)
{
	static gboolean		initialized=FALSE;

	/* The following virtual functions should be overriden if default
	 * implementation does not fit.
	 */
	iface->can_focus=_xfdashboard_focusable_real_can_focus;

	iface->supports_selection=_xfdashboard_focusable_real_supports_selection;
	iface->activate_selection=_xfdashboard_focusable_real_activate_selection;

	/* Define signals */
	if(!initialized)
	{
		/* Define signals */
		XfdashboardFocusableSignals[SIGNAL_FOCUS_SET]=
			g_signal_new("focus-set",
							XFDASHBOARD_TYPE_FOCUSABLE,
							G_SIGNAL_RUN_LAST,
							0,
							NULL,
							NULL,
							g_cclosure_marshal_VOID__OBJECT,
							G_TYPE_NONE,
							1,
							XFDASHBOARD_TYPE_FOCUSABLE);

		XfdashboardFocusableSignals[SIGNAL_FOCUS_UNSET]=
			g_signal_new("focus-unset",
							XFDASHBOARD_TYPE_FOCUSABLE,
							G_SIGNAL_RUN_LAST,
							0,
							NULL,
							NULL,
							g_cclosure_marshal_VOID__OBJECT,
							G_TYPE_NONE,
							1,
							XFDASHBOARD_TYPE_FOCUSABLE);

		XfdashboardFocusableSignals[SIGNAL_SELECTION_CHANGED]=
			g_signal_new("selection-changed",
							XFDASHBOARD_TYPE_FOCUSABLE,
							G_SIGNAL_RUN_LAST,
							0,
							NULL,
							NULL,
							_xfdashboard_marshal_VOID__OBJECT_OBJECT_OBJECT,
							G_TYPE_NONE,
							2,
							CLUTTER_TYPE_ACTOR,
							CLUTTER_TYPE_ACTOR);

		/* Set flag that base initialization was done for this interface */
		initialized=TRUE;
	}
}

/* Implementation: Public API */

/* Call virtual function "can_focus" */
gboolean xfdashboard_focusable_can_focus(XfdashboardFocusable *self)
{
	XfdashboardFocusableInterface		*iface;

	g_return_val_if_fail(XFDASHBOARD_IS_FOCUSABLE(self), FALSE);

	iface=XFDASHBOARD_FOCUSABLE_GET_IFACE(self);

	/* Call virtual function */
	if(iface->can_focus)
	{
		return(iface->can_focus(self));
	}

	/* If we get here the virtual function was not overridden */
	XFDASHBOARD_FOCUSABLE_WARN_NOT_IMPLEMENTED(self, "can_focus");
	return(FALSE);
}

/* Call virtual function "set_focus" */
void xfdashboard_focusable_set_focus(XfdashboardFocusable *self)
{
	XfdashboardFocusableInterface		*iface;
	ClutterActor						*selection;

	g_return_if_fail(XFDASHBOARD_IS_FOCUSABLE(self));

	iface=XFDASHBOARD_FOCUSABLE_GET_IFACE(self);

	/* Call virtual function */
	if(iface->set_focus)
	{
		iface->set_focus(self);
	}

	/* Style newly focused actor */
	if(XFDASHBOARD_IS_STYLABLE(self))
	{
		xfdashboard_stylable_add_class(XFDASHBOARD_STYLABLE(self), "focus");
	}

	/* If actor supports selection get current selection and style it */
	if(xfdashboard_focusable_supports_selection(self))
	{
		/* Get current selection. If no selection is available then select first item. */
		selection=xfdashboard_focusable_get_selection(self);
		if(!selection)
		{
			selection=xfdashboard_focusable_find_selection(self, NULL, XFDASHBOARD_SELECTION_TARGET_FIRST);
			if(selection) xfdashboard_focusable_set_selection(self, selection);
		}

		/* Style selection if available */
		if(selection &&
			XFDASHBOARD_IS_STYLABLE(selection))
		{
			xfdashboard_stylable_add_pseudo_class(XFDASHBOARD_STYLABLE(selection), "selected");
		}

		g_debug("Set selection to %s for focused actor %s",
				G_OBJECT_TYPE_NAME(self),
				selection ? G_OBJECT_TYPE_NAME(selection) : "<nil>");
	}

	/* Emit signal */
	g_signal_emit(self, XfdashboardFocusableSignals[SIGNAL_FOCUS_SET], 0, self);
	g_debug("Emitted signal 'focus-set' for focused actor %s", G_OBJECT_TYPE_NAME(self));
}

/* Call virtual function "unset_focus" */
void xfdashboard_focusable_unset_focus(XfdashboardFocusable *self)
{
	XfdashboardFocusableInterface		*iface;
	ClutterActor						*selection;

	g_return_if_fail(XFDASHBOARD_IS_FOCUSABLE(self));

	iface=XFDASHBOARD_FOCUSABLE_GET_IFACE(self);

	/* Call virtual function */
	if(iface->unset_focus)
	{
		iface->unset_focus(self);
	}

	/* Remove style from unfocused actor */
	if(XFDASHBOARD_IS_STYLABLE(self))
	{
		xfdashboard_stylable_remove_class(XFDASHBOARD_STYLABLE(self), "focus");
	}

	/* If actor supports selection get current selection and unstyle it */
	if(xfdashboard_focusable_supports_selection(self))
	{
		/* Get current selection */
		selection=xfdashboard_focusable_get_selection(self);

		/* unstyle selection if available */
		if(selection &&
			XFDASHBOARD_IS_STYLABLE(selection))
		{
			xfdashboard_stylable_remove_pseudo_class(XFDASHBOARD_STYLABLE(selection), "selected");
		}

		g_debug("Unstyled selection %s for focus loosing actor %s",
				G_OBJECT_TYPE_NAME(self),
				selection ? G_OBJECT_TYPE_NAME(selection) : "<nil>");
	}

	/* Emit signal */
	g_signal_emit(self, XfdashboardFocusableSignals[SIGNAL_FOCUS_UNSET], 0, self);
	g_debug("Emitted signal 'focus-unset' for focused actor %s", G_OBJECT_TYPE_NAME(self));
}

/* Call key handling function depending on key event type */
gboolean xfdashboard_focusable_handle_key_event(XfdashboardFocusable *self, const ClutterEvent *inEvent)
{
	gboolean			result;

	g_return_val_if_fail(XFDASHBOARD_IS_FOCUSABLE(self), CLUTTER_EVENT_PROPAGATE);
	g_return_val_if_fail(inEvent, CLUTTER_EVENT_PROPAGATE);
	g_return_val_if_fail(clutter_event_type(inEvent)==CLUTTER_KEY_PRESS ||
							clutter_event_type(inEvent)==CLUTTER_KEY_RELEASE, CLUTTER_EVENT_PROPAGATE);

	result=CLUTTER_EVENT_PROPAGATE;

	/* Call subsequent function to handle key event depending on
	 * if key was pressed or released.
	 */
	switch(clutter_event_type(inEvent))
	{
		case CLUTTER_KEY_PRESS:
			result=_xfdashboard_focusable_handle_keypress_event(self, inEvent);
			break;

		case CLUTTER_KEY_RELEASE:
			result=_xfdashboard_focusable_handle_keyrelease_event(self, inEvent);
			break;

		default:
			/* We should never get here */
			g_assert_not_reached();
			break;
	}

	return(result);
}

/* Call virtual function "supports_selection" */
gboolean xfdashboard_focusable_supports_selection(XfdashboardFocusable *self)
{
	XfdashboardFocusableInterface		*iface;

	g_return_val_if_fail(XFDASHBOARD_IS_FOCUSABLE(self), FALSE);

	iface=XFDASHBOARD_FOCUSABLE_GET_IFACE(self);

	/* Call virtual function */
	if(iface->supports_selection)
	{
		return(iface->supports_selection(self));
	}

	/* If we get here the virtual function was not overridden */
	XFDASHBOARD_FOCUSABLE_WARN_NOT_IMPLEMENTED(self, "supports_selection");
	return(FALSE);
}

/* Call virtual function "get_selection" */
ClutterActor* xfdashboard_focusable_get_selection(XfdashboardFocusable *self)
{
	XfdashboardFocusableInterface		*iface;

	g_return_val_if_fail(XFDASHBOARD_IS_FOCUSABLE(self), NULL);

	iface=XFDASHBOARD_FOCUSABLE_GET_IFACE(self);

	/* If this focusable actor does not support selection we should ask for
	 * the current selection and avoid the warning being printed if this
	 * virtual function was not overridden.
	 */
	if(!xfdashboard_focusable_supports_selection(self)) return(NULL);

	/* Call virtual function */
	if(iface->get_selection)
	{
		return(iface->get_selection(self));
	}

	/* If we get here the virtual function was not overridden */
	XFDASHBOARD_FOCUSABLE_WARN_NOT_IMPLEMENTED(self, "get_selection");
	return(NULL);
}

/* Call virtual function "set_selection" */
gboolean xfdashboard_focusable_set_selection(XfdashboardFocusable *self, ClutterActor *inSelection)
{
	XfdashboardFocusableInterface		*iface;
	ClutterActor						*oldSelection;
	gboolean							success;

	g_return_val_if_fail(XFDASHBOARD_IS_FOCUSABLE(self), FALSE);
	g_return_val_if_fail(!inSelection || CLUTTER_IS_ACTOR(inSelection), FALSE);

	iface=XFDASHBOARD_FOCUSABLE_GET_IFACE(self);

	/* If this focusable actor does not support selection we should ask for
	 * the current selection and avoid the warning being printed if this
	 * virtual function was not overridden.
	 */
	if(!xfdashboard_focusable_supports_selection(self)) return(FALSE);

	/* First get current selection */
	oldSelection=xfdashboard_focusable_get_selection(self);

	/* Do nothing if new selection is the same as the current one */
	if(inSelection==oldSelection) return(TRUE);

	/* Call virtual function */
	if(iface->set_selection)
	{
		/* Call virtual function to set selection */
		success=iface->set_selection(self, inSelection);

		/* If new selection could be set successfully, remove signal handlers
		 * from old selection and set up signal handlers for new selection.
		 */
		if(success)
		{
			/* Remove signal handlers and styles from old selection */
			if(oldSelection)
			{
				/* Remove signal handlers at old selection*/
				g_signal_handlers_disconnect_by_func(oldSelection,
														G_CALLBACK(_xfdashboard_focusable_on_selection_unavailable),
														self);

				/* Remove style from old selection */
				if(XFDASHBOARD_IS_STYLABLE(oldSelection))
				{
					xfdashboard_stylable_remove_pseudo_class(XFDASHBOARD_STYLABLE(oldSelection), "selected");
				}
			}

			/* Set up signal handlers and styles at new selection */
			if(inSelection)
			{
				/* Set up signal handlers to get notified if new selection
				 * is going to be unavailable (e.g. hidden or destroyed)
				 */
				g_signal_connect_swapped(inSelection,
											"destroy",
											G_CALLBACK(_xfdashboard_focusable_on_selection_unavailable),
											self);
				g_signal_connect_swapped(inSelection,
											"hide",
											G_CALLBACK(_xfdashboard_focusable_on_selection_unavailable),
											self);

				/* Style new selection if this focusable actor has the focus */
				if(_xfdashboard_focusable_has_focus(self) &&
					XFDASHBOARD_IS_STYLABLE(inSelection))
				{
					xfdashboard_stylable_add_pseudo_class(XFDASHBOARD_STYLABLE(inSelection), "selected");
				}
			}

			/* Emit signal */
			g_signal_emit(self, XfdashboardFocusableSignals[SIGNAL_SELECTION_CHANGED], 0, oldSelection, inSelection);
		}

		/* Return result of calling virtual function */
		return(success);
	}

	/* If we get here the virtual function was not overridden */
	XFDASHBOARD_FOCUSABLE_WARN_NOT_IMPLEMENTED(self, "set_selection");
	return(FALSE);
}

/* Call virtual function "find_selection" */
ClutterActor* xfdashboard_focusable_find_selection(XfdashboardFocusable *self, ClutterActor *inSelection, XfdashboardSelectionTarget inDirection)
{
	XfdashboardFocusableInterface		*iface;

	g_return_val_if_fail(XFDASHBOARD_IS_FOCUSABLE(self), NULL);
	g_return_val_if_fail(!inSelection || CLUTTER_IS_ACTOR(inSelection), NULL);
	g_return_val_if_fail(inDirection>XFDASHBOARD_SELECTION_TARGET_NONE, NULL);
	g_return_val_if_fail(inDirection<=XFDASHBOARD_SELECTION_TARGET_NEXT, NULL);

	iface=XFDASHBOARD_FOCUSABLE_GET_IFACE(self);

	/* If this focusable actor does not support selection we should ask for
	 * the current selection and avoid the warning being printed if this
	 * virtual function was not overridden.
	 */
	if(!xfdashboard_focusable_supports_selection(self)) return(NULL);

	/* Call virtual function */
	if(iface->find_selection)
	{
		return(iface->find_selection(self, inSelection, inDirection));
	}

	/* If we get here the virtual function was not overridden */
	XFDASHBOARD_FOCUSABLE_WARN_NOT_IMPLEMENTED(self, "find_selection");
	return(NULL);
}

/* Call virtual function "activate_selection" */
gboolean xfdashboard_focusable_activate_selection(XfdashboardFocusable *self, ClutterActor *inSelection)
{
	XfdashboardFocusableInterface		*iface;

	g_return_val_if_fail(XFDASHBOARD_IS_FOCUSABLE(self), FALSE);
	g_return_val_if_fail(CLUTTER_IS_ACTOR(inSelection), FALSE);

	iface=XFDASHBOARD_FOCUSABLE_GET_IFACE(self);

	/* If this focusable actor does not support selection we should ask for
	 * the current selection and avoid the warning being printed if this
	 * virtual function was not overridden.
	 */
	if(!xfdashboard_focusable_supports_selection(self)) return(FALSE);

	/* Call virtual function */
	if(iface->activate_selection)
	{
		return(iface->activate_selection(self, inSelection));
	}

	/* If we get here the virtual function was not overridden */
	XFDASHBOARD_FOCUSABLE_WARN_NOT_IMPLEMENTED(self, "activate_selection");
	return(FALSE);
}
