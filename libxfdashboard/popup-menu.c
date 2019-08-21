/*
 * popup-menu: A pop-up menu with menu items performing an action when an menu
 *             item was clicked
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

/**
 * SECTION:popup-menu
 * @short_description: A pop-up menu showing items and perfoming an action
                       when an item was clicked
 * @include: xfdashboard/popup-menu.h
 *
 * A #XfdashboardPopupMenu implements a drop down menu consisting of a list of
 * #ClutterActor objects as menu items which can be navigated and activated by
 * the user to perform the associated action of the selected menu item.
 *
 * The following example shows how create and activate a #XfdashboardPopupMenu
 * when an actor was clicked.
 * application when clicked:
 *
 * |[<!-- language="C" -->
 * ]|
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <libxfdashboard/popup-menu.h>

#include <glib/gi18n-lib.h>
#include <gtk/gtk.h>
#include <gdk/gdk.h>
#include <math.h>

#include <libxfdashboard/box-layout.h>
#include <libxfdashboard/focusable.h>
#include <libxfdashboard/focus-manager.h>
#include <libxfdashboard/stylable.h>
#include <libxfdashboard/window-tracker.h>
#include <libxfdashboard/application.h>
#include <libxfdashboard/click-action.h>
#include <libxfdashboard/bindings-pool.h>
#include <libxfdashboard/button.h>
#include <libxfdashboard/enums.h>
#include <libxfdashboard/utils.h>
#include <libxfdashboard/compat.h>
#include <libxfdashboard/debug.h>


/* Define this class in GObject system */
static void _xfdashboard_popup_menu_focusable_iface_init(XfdashboardFocusableInterface *iface);

struct _XfdashboardPopupMenuPrivate
{
	/* Properties related */
	gboolean						destroyOnCancel;

	ClutterActor					*source;

	gboolean						showTitle;
	gboolean						showTitleIcon;

	/* Instance related */
	gboolean						isActive;

	ClutterActor					*title;
	ClutterActor					*itemsContainer;

	XfdashboardWindowTracker		*windowTracker;

	XfdashboardFocusManager			*focusManager;
	gpointer						oldFocusable;
	gpointer						selectedItem;

	XfdashboardStage				*stage;
	guint							capturedEventSignalID;

	guint							sourceDestroySignalID;

	guint							suspendSignalID;
};

G_DEFINE_TYPE_WITH_CODE(XfdashboardPopupMenu,
						xfdashboard_popup_menu,
						XFDASHBOARD_TYPE_BACKGROUND,
						G_ADD_PRIVATE(XfdashboardPopupMenu)
						G_IMPLEMENT_INTERFACE(XFDASHBOARD_TYPE_FOCUSABLE, _xfdashboard_popup_menu_focusable_iface_init));

/* Properties */
enum
{
	PROP_0,

	PROP_DESTROY_ON_CANCEL,

	PROP_SOURCE,

	PROP_SHOW_TITLE,
	PROP_TITLE,

	PROP_SHOW_TITLE_ICON,
	PROP_TITLE_ICON_NAME,
	PROP_TITLE_GICON,

	PROP_LAST
};

static GParamSpec* XfdashboardPopupMenuProperties[PROP_LAST]={ 0, };

/* Signals */
enum
{
	SIGNAL_ACTIVATED,
	SIGNAL_CANCELLED,

	SIGNAL_ITEM_ACTIVATED,

	SIGNAL_ITEM_ADDED,
	SIGNAL_ITEM_REMOVED,

	SIGNAL_LAST
};

static guint XfdashboardPopupMenuSignals[SIGNAL_LAST]={ 0, };


/* IMPLEMENTATION: Private variables and methods */

/* Suspension state of application changed */
static void _xfdashboard_popup_menu_on_application_suspended_changed(XfdashboardPopupMenu *self,
																		GParamSpec *inSpec,
																		gpointer inUserData)
{
	XfdashboardPopupMenuPrivate		*priv;
	XfdashboardApplication			*application;
	gboolean						isSuspended;

	g_return_if_fail(XFDASHBOARD_IS_POPUP_MENU(self));
	g_return_if_fail(XFDASHBOARD_IS_APPLICATION(inUserData));

	priv=self->priv;
	application=XFDASHBOARD_APPLICATION(inUserData);

	/* Get application suspend state */
	isSuspended=xfdashboard_application_is_suspended(application);

	/* If application is suspended then cancel pop-up menu */
	if(isSuspended)
	{
		XFDASHBOARD_DEBUG(self, ACTOR,
							"Cancel active pop-up menu '%s' for source %s@%p because application was suspended",
							xfdashboard_popup_menu_get_title(self),
							priv->source ? G_OBJECT_TYPE_NAME(priv->source) : "<nil>",
							priv->source);

		xfdashboard_popup_menu_cancel(self);
	}
}

/* An event occured after a popup menu was activated so check if popup menu should
 * be cancelled because a button was pressed and release outside the popup menu.
 */
static gboolean _xfdashboard_popup_menu_on_captured_event(XfdashboardPopupMenu *self,
															ClutterEvent *inEvent,
															gpointer inUserData)
{
	XfdashboardPopupMenuPrivate		*priv;

	g_return_val_if_fail(XFDASHBOARD_IS_POPUP_MENU(self), CLUTTER_EVENT_PROPAGATE);
	g_return_val_if_fail(XFDASHBOARD_IS_STAGE(inUserData), CLUTTER_EVENT_PROPAGATE);

	priv=self->priv;

	/* Check if popup menu should be cancelled depending on event */
	switch(clutter_event_type(inEvent))
	{
		case CLUTTER_BUTTON_RELEASE:
			/* If button was released outside popup menu cancel this popup menu */
			{
				gfloat							x, y, w, h;

				clutter_actor_get_transformed_position(CLUTTER_ACTOR(self), &x, &y);
				clutter_actor_get_size(CLUTTER_ACTOR(self), &w, &h);
				if(inEvent->button.x<x ||
					inEvent->button.x>=(x+w) ||
					inEvent->button.y<y ||
					inEvent->button.y>=(y+h))
				{
					/* Cancel popup menu */
					xfdashboard_popup_menu_cancel(self);

					/* Do not let this event be handled */
					return(CLUTTER_EVENT_STOP);
				}
			}
			break;

		case CLUTTER_KEY_PRESS:
		case CLUTTER_KEY_RELEASE:
			/* If key press or key release is not a selection action for a focusable
			 * actor then cancel this popup menu.
			 */
			{
				GSList							*targetFocusables;
				const gchar						*action;
				gboolean						cancelPopupMenu;

				/* Lookup action for event and emit action if a binding was found
				 * for this event.
				 */
				targetFocusables=NULL;
				action=NULL;
				cancelPopupMenu=FALSE;

				if(xfdashboard_focus_manager_get_event_targets_and_action(priv->focusManager, inEvent, XFDASHBOARD_FOCUSABLE(self), &targetFocusables, &action))
				{
					if(!targetFocusables ||
						!targetFocusables->data ||
						!XFDASHBOARD_IS_POPUP_MENU(targetFocusables->data))
					{
						cancelPopupMenu=TRUE;
					}

					/* Release allocated resources */
					g_slist_free_full(targetFocusables, g_object_unref);
				}

				/* 'ESC' is a special key as it cannot be determined by focus
				 * manager but it has to be intercepted as this key release
				 * should only cancel popup-menu but not quit application.
				 */
				if(!cancelPopupMenu &&
					clutter_event_type(inEvent)==CLUTTER_KEY_RELEASE &&
					inEvent->key.keyval==CLUTTER_KEY_Escape)
				{
					cancelPopupMenu=TRUE;
				}

				/* Cancel popup-menu if requested */
				if(cancelPopupMenu)
				{
					/* Cancel popup menu */
					xfdashboard_popup_menu_cancel(self);

					/* Do not let this event be handled */
					return(CLUTTER_EVENT_STOP);
				}
			}
			break;

		default:
			/* Let all other event pass through */
			break;
	}

	/* If we get here then this event passed our filter and can be handled normally */
	return(CLUTTER_EVENT_PROPAGATE);
}

/*  Check if menu item is really part of this pop-up menu */
static gboolean _xfdashboard_popup_menu_contains_menu_item(XfdashboardPopupMenu *self,
															XfdashboardPopupMenuItem *inMenuItem)
{
	ClutterActor			*parent;

	g_return_val_if_fail(XFDASHBOARD_IS_POPUP_MENU(self), FALSE);
	g_return_val_if_fail(XFDASHBOARD_IS_POPUP_MENU_ITEM(inMenuItem), FALSE);

	/* Iterate through parents and for each XfdashboardPopupMenu found, check
	 * if it is this pop-up menu and return TRUE if it is.
	 */
	parent=clutter_actor_get_parent(CLUTTER_ACTOR(inMenuItem));
	while(parent)
	{
		/* Check if current iterated parent is a XfdashboardPopupMenu and if it
		 * is this pop-up menu.
		 */
		if(XFDASHBOARD_IS_POPUP_MENU(parent) &&
			XFDASHBOARD_POPUP_MENU(parent)==self)
		{
			/* This one is this pop-up menu, so return TRUE here */
			return(TRUE);
		}

		/* Continue with next parent */
		parent=clutter_actor_get_parent(parent);
	}

	/* If we get here the "menu item" actor is a menu item of this pop-up menu */
	return(FALSE);
}

/* Menu item was activated */
static void _xfdashboard_popup_menu_on_menu_item_activated(XfdashboardPopupMenu *self,
															gpointer inUserData)
{
	XfdashboardPopupMenuItem		*menuItem;

	g_return_if_fail(XFDASHBOARD_IS_POPUP_MENU(self));
	g_return_if_fail(XFDASHBOARD_IS_POPUP_MENU_ITEM(inUserData));

	menuItem=XFDASHBOARD_POPUP_MENU_ITEM(inUserData);

	/* Emit "item-activated" signal */
	g_signal_emit(self, XfdashboardPopupMenuSignals[SIGNAL_ITEM_ACTIVATED], 0, menuItem);

	/* Cancel pop-up menu as menu item was activated and its callback function
	 * was called by its meta object.
	 */
	xfdashboard_popup_menu_cancel(self);
}

/* Update visiblity of title actor depending on if title and/or icon of title
 * should be shown or not.
 */
static void _xfdashboard_popup_menu_update_title_actors_visibility(XfdashboardPopupMenu *self)
{
	XfdashboardPopupMenuPrivate		*priv;
	XfdashboardLabelStyle			oldStyle;
	XfdashboardLabelStyle			newStyle;
	gboolean						oldVisible;
	gboolean						newVisible;

	g_return_if_fail(XFDASHBOARD_IS_POPUP_MENU(self));

	priv=self->priv;

	/* Get current visibility state */
	oldVisible=clutter_actor_is_visible(priv->title);
	oldStyle=xfdashboard_label_get_style(XFDASHBOARD_LABEL(priv->title));

	/* Determine new visibility state depending on if title and/or icon of title
	 * should be shown or not.
	 */
	newStyle=0;
	newVisible=TRUE;
	if(priv->showTitle && priv->showTitleIcon) newStyle=XFDASHBOARD_LABEL_STYLE_BOTH;
		else if(priv->showTitle) newStyle=XFDASHBOARD_LABEL_STYLE_TEXT;
		else if(priv->showTitleIcon) newStyle=XFDASHBOARD_LABEL_STYLE_ICON;
		else newVisible=FALSE;

	/* Set new visibility style if changed and re-layout title actor */
	if(newStyle!=oldStyle)
	{
		xfdashboard_label_set_style(XFDASHBOARD_LABEL(priv->title), newStyle);
		clutter_actor_queue_relayout(priv->title);
	}

	/* Show or hide actor */
	if(newVisible!=oldVisible)
	{
		if(newVisible) clutter_actor_show(priv->title);
			else clutter_actor_hide(priv->title);
	}
}

/* The source actor was destroyed so cancel this pop-up menu if active and
 * destroy it if automatic destruction was turned on.
 */
static void _xfdashboard_popup_menu_on_source_destroy(XfdashboardPopupMenu *self,
														gpointer inUserData)
{
	XfdashboardPopupMenuPrivate		*priv;

	g_return_if_fail(XFDASHBOARD_IS_POPUP_MENU(self));
	g_return_if_fail(CLUTTER_IS_ACTOR(inUserData));

	priv=self->priv;

	/* Unset and clean-up source */
	if(priv->source)
	{
		gchar						*cssClass;

		/* Disconnect signal handler */
		if(priv->sourceDestroySignalID)
		{
			g_signal_handler_disconnect(priv->source, priv->sourceDestroySignalID);
			priv->sourceDestroySignalID=0;
		}

		/* Remove style */
		cssClass=g_strdup_printf("popup-menu-source-%s", G_OBJECT_TYPE_NAME(priv->source));
		xfdashboard_stylable_remove_class(XFDASHBOARD_STYLABLE(self), cssClass);
		g_free(cssClass);

		/* Release source */
		g_object_remove_weak_pointer(G_OBJECT(priv->source), (gpointer*)&priv->source);
		priv->source=NULL;
	}

	/* Enforce that pop-up menu is cancelled either by calling the cancel function
	 * if it is active or by checking and destructing it if automatic destruction
	 * flag is set.
	 */
	if(priv->isActive)
	{
		/* Pop-up menu is active so cancel it. The cancel function will also destroy
		 * it if destroy-on-cancel was enabled.
		 */
		xfdashboard_popup_menu_cancel(self);
	}
		else
		{
			/* Destroy this pop-up menu actor when destroy-on-cancel was enabled */
			if(priv->destroyOnCancel)
			{
				clutter_actor_destroy(CLUTTER_ACTOR(self));
			}
		}
}


/* IMPLEMENTATION: ClutterActor */

/* Allocate position and size of actor and its children */
static void _xfdashboard_popup_menu_allocate(ClutterActor *inActor,
												const ClutterActorBox *inBox,
												ClutterAllocationFlags inFlags)
{
	ClutterAllocationFlags		flags;

	/* Chain up to store the allocation of the actor */
	flags=inFlags | CLUTTER_DELEGATE_LAYOUT;
	CLUTTER_ACTOR_CLASS(xfdashboard_popup_menu_parent_class)->allocate(inActor, inBox, flags);
}


/* IMPLEMENTATION: Interface XfdashboardFocusable */

/* Determine if actor can get the focus */
static gboolean _xfdashboard_popup_menu_focusable_can_focus(XfdashboardFocusable *inFocusable)
{
	XfdashboardPopupMenu			*self;
	XfdashboardPopupMenuPrivate		*priv;
	XfdashboardFocusableInterface	*selfIface;
	XfdashboardFocusableInterface	*parentIface;

	g_return_val_if_fail(XFDASHBOARD_IS_FOCUSABLE(inFocusable), FALSE);
	g_return_val_if_fail(XFDASHBOARD_IS_POPUP_MENU(inFocusable), FALSE);

	self=XFDASHBOARD_POPUP_MENU(inFocusable);
	priv=self->priv;

	/* Call parent class interface function */
	selfIface=XFDASHBOARD_FOCUSABLE_GET_IFACE(inFocusable);
	parentIface=g_type_interface_peek_parent(selfIface);

	if(parentIface && parentIface->can_focus)
	{
		if(!parentIface->can_focus(inFocusable))
		{
			return(FALSE);
		}
	}

	/* Only active pop-up menus can be focused */
	if(!priv->isActive) return(FALSE);

	/* If we get here this actor can be focused */
	return(TRUE);
}

/* Actor lost focus */
static void _xfdashboard_popup_menu_focusable_unset_focus(XfdashboardFocusable *inFocusable)
{
	XfdashboardPopupMenu			*self;
	XfdashboardPopupMenuPrivate		*priv;
	XfdashboardFocusableInterface	*selfIface;
	XfdashboardFocusableInterface	*parentIface;

	g_return_if_fail(XFDASHBOARD_IS_FOCUSABLE(inFocusable));
	g_return_if_fail(XFDASHBOARD_IS_POPUP_MENU(inFocusable));

	self=XFDASHBOARD_POPUP_MENU(inFocusable);
	priv=self->priv;

	/* Call parent class interface function */
	selfIface=XFDASHBOARD_FOCUSABLE_GET_IFACE(inFocusable);
	parentIface=g_type_interface_peek_parent(selfIface);

	if(parentIface && parentIface->unset_focus)
	{
		parentIface->unset_focus(inFocusable);
	}

	/* If this pop-up menu is active (has flag set) then it was not cancelled and
	 * this actor lost its focus in any other way than expected. So do not refocus
	 * old remembered focusable as it may not be the one which has the focus before.
	 */
	if(priv->isActive &&
		priv->oldFocusable)
	{
		g_object_remove_weak_pointer(G_OBJECT(priv->oldFocusable), &priv->oldFocusable);
		priv->oldFocusable=NULL;
	}

	/* This actor lost focus so ensure that this popup menu is cancelled */
	xfdashboard_popup_menu_cancel(self);
}

/* Determine if this actor supports selection */
static gboolean _xfdashboard_popup_menu_focusable_supports_selection(XfdashboardFocusable *inFocusable)
{
	g_return_val_if_fail(XFDASHBOARD_IS_FOCUSABLE(inFocusable), FALSE);
	g_return_val_if_fail(XFDASHBOARD_IS_POPUP_MENU(inFocusable), FALSE);

	/* This actor supports selection */
	return(TRUE);
}

/* Get current selection */
static ClutterActor* _xfdashboard_popup_menu_focusable_get_selection(XfdashboardFocusable *inFocusable)
{
	XfdashboardPopupMenu			*self;
	XfdashboardPopupMenuPrivate		*priv;

	g_return_val_if_fail(XFDASHBOARD_IS_FOCUSABLE(inFocusable), NULL);
	g_return_val_if_fail(XFDASHBOARD_IS_POPUP_MENU(inFocusable), NULL);

	self=XFDASHBOARD_POPUP_MENU(inFocusable);
	priv=self->priv;

	/* Return current selection */
	return(priv->selectedItem);
}

/* Set new selection */
static gboolean _xfdashboard_popup_menu_focusable_set_selection(XfdashboardFocusable *inFocusable,
																ClutterActor *inSelection)
{
	XfdashboardPopupMenu			*self;
	XfdashboardPopupMenuPrivate		*priv;

	g_return_val_if_fail(XFDASHBOARD_IS_FOCUSABLE(inFocusable), FALSE);
	g_return_val_if_fail(XFDASHBOARD_IS_POPUP_MENU(inFocusable), FALSE);
	g_return_val_if_fail(!inSelection || CLUTTER_IS_ACTOR(inSelection), FALSE);

	self=XFDASHBOARD_POPUP_MENU(inFocusable);
	priv=self->priv;

	/* Check that selection is a child of this actor */
	if(inSelection &&
		!clutter_actor_contains(CLUTTER_ACTOR(self), inSelection))
	{
		g_warning(_("%s is not a child of %s and cannot be selected"),
					G_OBJECT_TYPE_NAME(inSelection),
					G_OBJECT_TYPE_NAME(self));

		return(FALSE);
	}

	/* Remove weak reference at current selection */
	if(priv->selectedItem)
	{
		g_object_remove_weak_pointer(G_OBJECT(priv->selectedItem), &priv->selectedItem);
	}

	/* Set new selection */
	priv->selectedItem=inSelection;

	/* Add weak reference at new selection */
	if(priv->selectedItem)
	{
		g_object_add_weak_pointer(G_OBJECT(priv->selectedItem), &priv->selectedItem);
	}

	/* New selection was set successfully */
	return(TRUE);
}

/* Find requested selection target depending of current selection */
static ClutterActor* _xfdashboard_popup_menu_focusable_find_selection(XfdashboardFocusable *inFocusable,
																		ClutterActor *inSelection,
																		XfdashboardSelectionTarget inDirection)
{
	XfdashboardPopupMenu				*self;
	XfdashboardPopupMenuPrivate			*priv;
	ClutterActor						*selection;
	ClutterActor						*newSelection;
	gchar								*valueName;

	g_return_val_if_fail(XFDASHBOARD_IS_FOCUSABLE(inFocusable), NULL);
	g_return_val_if_fail(XFDASHBOARD_IS_POPUP_MENU(inFocusable), NULL);
	g_return_val_if_fail(!inSelection || CLUTTER_IS_ACTOR(inSelection), NULL);
	g_return_val_if_fail(inDirection>=0 && inDirection<=XFDASHBOARD_SELECTION_TARGET_NEXT, NULL);

	self=XFDASHBOARD_POPUP_MENU(inFocusable);
	priv=self->priv;
	selection=inSelection;
	newSelection=NULL;

	/* If there is nothing selected, select first actor and return */
	if(!inSelection)
	{
		selection=clutter_actor_get_first_child(CLUTTER_ACTOR(priv->itemsContainer));

		valueName=xfdashboard_get_enum_value_name(XFDASHBOARD_TYPE_SELECTION_TARGET, inDirection);
		XFDASHBOARD_DEBUG(self, ACTOR,
							"No selection at %s, so select first child %s for direction %s",
							G_OBJECT_TYPE_NAME(self),
							selection ? G_OBJECT_TYPE_NAME(selection) : "<nil>",
							valueName);
		g_free(valueName);

		return(selection);
	}

	/* Check that selection is a child of this actor otherwise return NULL */
	if(!clutter_actor_contains(CLUTTER_ACTOR(self), inSelection))
	{
		ClutterActor						*parent;

		parent=clutter_actor_get_parent(inSelection);
		g_warning(_("Cannot lookup selection target at %s because %s is a child of %s"),
					G_OBJECT_TYPE_NAME(self),
					G_OBJECT_TYPE_NAME(inSelection),
					parent ? G_OBJECT_TYPE_NAME(parent) : "<nil>");

		return(NULL);
	}

	/* Find target selection */
	switch(inDirection)
	{
		case XFDASHBOARD_SELECTION_TARGET_UP:
			newSelection=clutter_actor_get_previous_sibling(inSelection);
			break;

		case XFDASHBOARD_SELECTION_TARGET_DOWN:
			newSelection=clutter_actor_get_next_sibling(inSelection);
			break;

		case XFDASHBOARD_SELECTION_TARGET_FIRST:
		case XFDASHBOARD_SELECTION_TARGET_PAGE_UP:
			newSelection=clutter_actor_get_first_child(CLUTTER_ACTOR(priv->itemsContainer));
			break;

		case XFDASHBOARD_SELECTION_TARGET_LAST:
		case XFDASHBOARD_SELECTION_TARGET_PAGE_DOWN:
			newSelection=clutter_actor_get_last_child(CLUTTER_ACTOR(priv->itemsContainer));
			break;

		case XFDASHBOARD_SELECTION_TARGET_NEXT:
			newSelection=clutter_actor_get_next_sibling(inSelection);
			if(!newSelection) newSelection=clutter_actor_get_previous_sibling(inSelection);
			break;

		default:
			{
				valueName=xfdashboard_get_enum_value_name(XFDASHBOARD_TYPE_SELECTION_TARGET, inDirection);
				g_critical(_("Focusable object %s does not handle selection direction of type %s."),
							G_OBJECT_TYPE_NAME(self),
							valueName);
				g_free(valueName);
			}
			break;
	}

	/* If new selection could be found override current selection with it */
	if(newSelection) selection=newSelection;

	/* Return new selection found */
	XFDASHBOARD_DEBUG(self, ACTOR,
						"Selecting %s at %s for current selection %s in direction %u",
						selection ? G_OBJECT_TYPE_NAME(selection) : "<nil>",
						G_OBJECT_TYPE_NAME(self),
						inSelection ? G_OBJECT_TYPE_NAME(inSelection) : "<nil>",
						inDirection);

	return(selection);
}

/* Activate selection */
static gboolean _xfdashboard_popup_menu_focusable_activate_selection(XfdashboardFocusable *inFocusable,
																		ClutterActor *inSelection)
{
	XfdashboardPopupMenu				*self;
	XfdashboardPopupMenuItem			*menuItem;

	g_return_val_if_fail(XFDASHBOARD_IS_FOCUSABLE(inFocusable), FALSE);
	g_return_val_if_fail(XFDASHBOARD_IS_POPUP_MENU(inFocusable), FALSE);
	g_return_val_if_fail(XFDASHBOARD_IS_POPUP_MENU_ITEM(inSelection), FALSE);

	self=XFDASHBOARD_POPUP_MENU(inFocusable);
	menuItem=XFDASHBOARD_POPUP_MENU_ITEM(inSelection);

	/* Check that selection is a child of this actor */
	if(!clutter_actor_contains(CLUTTER_ACTOR(self), inSelection))
	{
		ClutterActor						*parent;

		parent=clutter_actor_get_parent(inSelection);
		g_warning(_("%s is a child of %s and cannot be activated at %s"),
					G_OBJECT_TYPE_NAME(inSelection),
					parent ? G_OBJECT_TYPE_NAME(parent) : "<nil>",
					G_OBJECT_TYPE_NAME(self));

		return(FALSE);
	}

	/* Activate selection */
	xfdashboard_popup_menu_item_activate(menuItem);

	/* If we get here activation of menu item was successful */
	return(TRUE);
}

/* Interface initialization
 * Set up default functions
 */
void _xfdashboard_popup_menu_focusable_iface_init(XfdashboardFocusableInterface *iface)
{
	iface->can_focus=_xfdashboard_popup_menu_focusable_can_focus;
	iface->unset_focus=_xfdashboard_popup_menu_focusable_unset_focus;

	iface->supports_selection=_xfdashboard_popup_menu_focusable_supports_selection;
	iface->get_selection=_xfdashboard_popup_menu_focusable_get_selection;
	iface->set_selection=_xfdashboard_popup_menu_focusable_set_selection;
	iface->find_selection=_xfdashboard_popup_menu_focusable_find_selection;
	iface->activate_selection=_xfdashboard_popup_menu_focusable_activate_selection;
}


/* IMPLEMENTATION: GObject */

/* Dispose this object */
static void _xfdashboard_popup_menu_dispose(GObject *inObject)
{
	XfdashboardPopupMenu			*self=XFDASHBOARD_POPUP_MENU(inObject);
	XfdashboardPopupMenuPrivate		*priv=self->priv;

	/* Cancel this pop-up menu if it is still active */
	xfdashboard_popup_menu_cancel(self);

	/* Release our allocated variables */
	if(priv->suspendSignalID)
	{
		g_signal_handler_disconnect(xfdashboard_application_get_default(), priv->suspendSignalID);
		priv->suspendSignalID=0;
	}

	if(priv->capturedEventSignalID)
	{
		g_signal_handler_disconnect(priv->stage, priv->capturedEventSignalID);
		priv->capturedEventSignalID=0;
	}

	if(priv->source)
	{
		gchar						*cssClass;

		/* Disconnect signal handler */
		if(priv->sourceDestroySignalID)
		{
			g_signal_handler_disconnect(priv->source, priv->sourceDestroySignalID);
			priv->sourceDestroySignalID=0;
		}

		/* Remove style */
		cssClass=g_strdup_printf("popup-menu-source-%s", G_OBJECT_TYPE_NAME(priv->source));
		xfdashboard_stylable_remove_class(XFDASHBOARD_STYLABLE(self), cssClass);
		g_free(cssClass);

		/* Release source */
		g_object_remove_weak_pointer(G_OBJECT(priv->source), (gpointer*)&priv->source);
		priv->source=NULL;
	}

	if(priv->selectedItem)
	{
		g_object_remove_weak_pointer(G_OBJECT(priv->selectedItem), &priv->selectedItem);
		priv->selectedItem=NULL;
	}

	if(priv->oldFocusable)
	{
		g_object_remove_weak_pointer(G_OBJECT(priv->oldFocusable), &priv->oldFocusable);
		priv->oldFocusable=NULL;
	}

	if(priv->itemsContainer)
	{
		clutter_actor_destroy(priv->itemsContainer);
		priv->itemsContainer=NULL;
	}

	if(priv->focusManager)
	{
		xfdashboard_focus_manager_unregister(priv->focusManager, XFDASHBOARD_FOCUSABLE(self));
		g_object_unref(priv->focusManager);
		priv->focusManager=NULL;
	}

	if(priv->windowTracker)
	{
		g_object_unref(priv->windowTracker);
		priv->windowTracker=NULL;
	}

	if(priv->stage)
	{
		priv->stage=NULL;
	}

	/* Call parent's class dispose method */
	G_OBJECT_CLASS(xfdashboard_popup_menu_parent_class)->dispose(inObject);
}

/* Set/get properties */
static void _xfdashboard_popup_menu_set_property(GObject *inObject,
													guint inPropID,
													const GValue *inValue,
													GParamSpec *inSpec)
{
	XfdashboardPopupMenu			*self=XFDASHBOARD_POPUP_MENU(inObject);

	switch(inPropID)
	{
		case PROP_DESTROY_ON_CANCEL:
			xfdashboard_popup_menu_set_destroy_on_cancel(self, g_value_get_boolean(inValue));
			break;

		case PROP_SOURCE:
			xfdashboard_popup_menu_set_source(self, CLUTTER_ACTOR(g_value_get_object(inValue)));
			break;

		case PROP_SHOW_TITLE:
			xfdashboard_popup_menu_set_show_title(self, g_value_get_boolean(inValue));
			break;

		case PROP_TITLE:
			xfdashboard_popup_menu_set_title(self, g_value_get_string(inValue));
			break;

		case PROP_SHOW_TITLE_ICON:
			xfdashboard_popup_menu_set_show_title_icon(self, g_value_get_boolean(inValue));
			break;

		case PROP_TITLE_ICON_NAME:
			xfdashboard_popup_menu_set_title_icon_name(self, g_value_get_string(inValue));
			break;

		case PROP_TITLE_GICON:
			xfdashboard_popup_menu_set_title_gicon(self, G_ICON(g_value_get_object(inValue)));
			break;

		default:
			G_OBJECT_WARN_INVALID_PROPERTY_ID(inObject, inPropID, inSpec);
			break;
	}
}

static void _xfdashboard_popup_menu_get_property(GObject *inObject,
													guint inPropID,
													GValue *outValue,
													GParamSpec *inSpec)
{
	XfdashboardPopupMenu			*self=XFDASHBOARD_POPUP_MENU(inObject);
	XfdashboardPopupMenuPrivate		*priv=self->priv;

	switch(inPropID)
	{
		case PROP_DESTROY_ON_CANCEL:
			g_value_set_boolean(outValue, priv->destroyOnCancel);
			break;

		case PROP_SOURCE:
			g_value_set_object(outValue, priv->source);
			break;

		case PROP_SHOW_TITLE:
			g_value_set_boolean(outValue, priv->showTitle);
			break;

		case PROP_TITLE:
			g_value_set_string(outValue, xfdashboard_popup_menu_get_title(self));
			break;

		case PROP_SHOW_TITLE_ICON:
			g_value_set_boolean(outValue, priv->showTitleIcon);
			break;

		case PROP_TITLE_ICON_NAME:
			g_value_set_string(outValue, xfdashboard_popup_menu_get_title_icon_name(self));
			break;

		case PROP_TITLE_GICON:
			g_value_set_object(outValue, xfdashboard_popup_menu_get_title_gicon(self));
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
static void xfdashboard_popup_menu_class_init(XfdashboardPopupMenuClass *klass)
{
	XfdashboardActorClass	*actorClass=XFDASHBOARD_ACTOR_CLASS(klass);
	ClutterActorClass		*clutterActorClass=CLUTTER_ACTOR_CLASS(klass);
	GObjectClass			*gobjectClass=G_OBJECT_CLASS(klass);

	/* Override functions */
	gobjectClass->dispose=_xfdashboard_popup_menu_dispose;
	gobjectClass->set_property=_xfdashboard_popup_menu_set_property;
	gobjectClass->get_property=_xfdashboard_popup_menu_get_property;

	clutterActorClass->allocate=_xfdashboard_popup_menu_allocate;

	/* Define properties */
	/**
	 * XfdashboardPopupMenu:destroy-on-cancel:
	 *
	 * A flag indicating if this pop-up menu should be destroyed automatically
	 * when it is cancelled.
	 */
	XfdashboardPopupMenuProperties[PROP_DESTROY_ON_CANCEL]=
		g_param_spec_boolean("destroy-on-cancel",
								_("Destroy on cancel"),
								_("Flag indicating this pop-up menu should be destroyed automatically when it is cancelled"),
								FALSE,
								G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS);

	/**
	 * XfdashboardPopupMenu:source:
	 *
	 * The #ClutterActor on which this pop-up menu depends on. If this actor is
	 * destroyed then this pop-up menu is cancelled when active. 
	 */
	XfdashboardPopupMenuProperties[PROP_SOURCE]=
		g_param_spec_object("source",
							_("Source"),
							_("The object on which this pop-up menu depends on"),
							CLUTTER_TYPE_ACTOR,
							G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS);

	/**
	 * XfdashboardPopupMenu:show-title:
	 *
	 * A flag indicating if the title of this pop-up menu should be shown.
	 */
	XfdashboardPopupMenuProperties[PROP_SHOW_TITLE]=
		g_param_spec_boolean("show-title",
								_("Show title"),
								_("Flag indicating if the title of this pop-up menu should be shown"),
								FALSE,
								G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS);

	/**
	 * XfdashboardPopupMenu:title:
	 *
	 * A string containing the title of this pop-up menu.
	 */
	XfdashboardPopupMenuProperties[PROP_TITLE]=
		g_param_spec_string("title",
							_("Title"),
							_("Title of pop-up menu"),
							N_(""),
							G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS);

	/**
	 * XfdashboardPopupMenu:show-title-icon:
	 *
	 * A flag indicating if the icon of the title of this pop-up menu should be shown.
	 */
	XfdashboardPopupMenuProperties[PROP_SHOW_TITLE_ICON]=
		g_param_spec_boolean("show-title-icon",
								_("Show title icon"),
								_("Flag indicating if the icon of title of this pop-up menu should be shown"),
								FALSE,
								G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS);

	/**
	 * XfdashboardPopupMenu:title-icon-name:
	 *
	 * A string containing the stock icon name or file name for the icon to use
	 * at title of this pop-up menu.
	 */
	XfdashboardPopupMenuProperties[PROP_TITLE_ICON_NAME]=
		g_param_spec_string("title-icon-name",
							_("Title icon name"),
							_("Themed icon name or file name of icon used in title"),
							N_(""),
							G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS);

	/**
	 * XfdashboardPopupMenu:title-gicon:
	 *
	 * A #GIcon containing the icon image to use at title of this pop-up menu.
	 */
	XfdashboardPopupMenuProperties[PROP_TITLE_GICON]=
		g_param_spec_object("title-gicon",
							_("Title GIcon"),
							_("The GIcon of icon used in title"),
							G_TYPE_ICON,
							G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS);

	g_object_class_install_properties(gobjectClass, PROP_LAST, XfdashboardPopupMenuProperties);

	/* Define stylable properties */
	xfdashboard_actor_install_stylable_property(actorClass, XfdashboardPopupMenuProperties[PROP_SHOW_TITLE]);
	xfdashboard_actor_install_stylable_property(actorClass, XfdashboardPopupMenuProperties[PROP_SHOW_TITLE_ICON]);

	/* Define signals */
	/**
	 * XfdashboardPopupMenu::activated:
	 * @self: The pop-up menu which was activated
	 *
	 * The ::activated signal is emitted when the pop-up menu is shown and the
	 * user can perform an action by selecting an item.
	 */
	XfdashboardPopupMenuSignals[SIGNAL_ACTIVATED]=
		g_signal_new("activated",
						G_TYPE_FROM_CLASS(klass),
						G_SIGNAL_RUN_LAST,
						G_STRUCT_OFFSET(XfdashboardPopupMenuClass, activated),
						NULL,
						NULL,
						g_cclosure_marshal_VOID__VOID,
						G_TYPE_NONE,
						0);

	/**
	 * XfdashboardPopupMenu::cancelled:
	 * @self: The pop-up menu which was cancelled
	 *
	 * The ::cancelled signal is emitted when the pop-up menu is hidden. This
	 * signal is emitted regardless the user has chosen an item and perform the
	 * associated action or not.
	 *
	 * Note: This signal does not indicate if a selection was made or not.
	 */
	XfdashboardPopupMenuSignals[SIGNAL_CANCELLED]=
		g_signal_new("cancelled",
						G_TYPE_FROM_CLASS(klass),
						G_SIGNAL_RUN_LAST,
						G_STRUCT_OFFSET(XfdashboardPopupMenuClass, cancelled),
						NULL,
						NULL,
						g_cclosure_marshal_VOID__VOID,
						G_TYPE_NONE,
						0);

	/**
	 * XfdashboardPopupMenu::item-activated:
	 * @self: The pop-up menu containing the activated menu item
	 * @inMenuItem: The menu item which was activated
	 *
	 * The ::item-activated signal is emitted when a menu item at pop-up menu
	 * was activated either by key-press or by clicking on it.
	 */
	XfdashboardPopupMenuSignals[SIGNAL_ITEM_ACTIVATED]=
		g_signal_new("item-activated",
						G_TYPE_FROM_CLASS(klass),
						G_SIGNAL_RUN_LAST,
						G_STRUCT_OFFSET(XfdashboardPopupMenuClass, item_activated),
						NULL,
						NULL,
						g_cclosure_marshal_VOID__OBJECT,
						G_TYPE_NONE,
						1,
						XFDASHBOARD_TYPE_POPUP_MENU_ITEM);

	/**
	 * XfdashboardPopupMenu::item-added:
	 * @self: The pop-up menu containing the activated menu item
	 * @inMenuItem: The menu item which was added
	 *
	 * The ::item-added signal is emitted when a menu item was added to pop-up menu.
	 */
	XfdashboardPopupMenuSignals[SIGNAL_ITEM_ADDED]=
		g_signal_new("item-added",
						G_TYPE_FROM_CLASS(klass),
						G_SIGNAL_RUN_LAST,
						G_STRUCT_OFFSET(XfdashboardPopupMenuClass, item_added),
						NULL,
						NULL,
						g_cclosure_marshal_VOID__OBJECT,
						G_TYPE_NONE,
						1,
						XFDASHBOARD_TYPE_POPUP_MENU_ITEM);

	/**
	 * XfdashboardPopupMenu::item-removed:
	 * @self: The pop-up menu containing the activated menu item
	 * @inMenuItem: The menu item which was added
	 *
	 * The ::item-added signal is emitted when a menu item was added to pop-up menu.
	 */
	XfdashboardPopupMenuSignals[SIGNAL_ITEM_REMOVED]=
		g_signal_new("item-removed",
						G_TYPE_FROM_CLASS(klass),
						G_SIGNAL_RUN_LAST,
						G_STRUCT_OFFSET(XfdashboardPopupMenuClass, item_removed),
						NULL,
						NULL,
						g_cclosure_marshal_VOID__OBJECT,
						G_TYPE_NONE,
						1,
						XFDASHBOARD_TYPE_POPUP_MENU_ITEM);
}

/* Object initialization
 * Create private structure and set up default values
 */
static void xfdashboard_popup_menu_init(XfdashboardPopupMenu *self)
{
	XfdashboardPopupMenuPrivate		*priv;
	ClutterLayoutManager			*layout;

	priv=self->priv=xfdashboard_popup_menu_get_instance_private(self);

	/* Set up default values */
	priv->destroyOnCancel=FALSE;
	priv->source=NULL;
	priv->showTitle=FALSE;
	priv->showTitleIcon=FALSE;
	priv->isActive=FALSE;
	priv->title=NULL;
	priv->itemsContainer=NULL;
	priv->windowTracker=xfdashboard_window_tracker_get_default();
	priv->focusManager=xfdashboard_focus_manager_get_default();
	priv->oldFocusable=NULL;
	priv->selectedItem=NULL;
	priv->stage=NULL;
	priv->capturedEventSignalID=0;
	priv->sourceDestroySignalID=0;
	priv->suspendSignalID=0;

	/* This actor is react on events */
	clutter_actor_set_reactive(CLUTTER_ACTOR(self), TRUE);

	/* Set up title actor */
	priv->title=xfdashboard_button_new();
	xfdashboard_label_set_style(XFDASHBOARD_LABEL(priv->title), XFDASHBOARD_LABEL_STYLE_TEXT);
	xfdashboard_label_set_text(XFDASHBOARD_LABEL(priv->title), NULL);
	clutter_actor_set_x_expand(priv->title, TRUE);
	clutter_actor_set_y_expand(priv->title, TRUE);
	clutter_actor_hide(priv->title);
	xfdashboard_stylable_add_class(XFDASHBOARD_STYLABLE(priv->title), "popup-menu-title");

	/* Set up items container which will hold all menu items */
	layout=xfdashboard_box_layout_new();
	clutter_box_layout_set_orientation(CLUTTER_BOX_LAYOUT(layout), CLUTTER_ORIENTATION_VERTICAL);

	priv->itemsContainer=xfdashboard_actor_new();
	clutter_actor_set_x_expand(priv->itemsContainer, TRUE);
	clutter_actor_set_y_expand(priv->itemsContainer, TRUE);
	clutter_actor_set_layout_manager(priv->itemsContainer, layout);

	/* Set up this actor */
	layout=xfdashboard_box_layout_new();
	clutter_box_layout_set_orientation(CLUTTER_BOX_LAYOUT(layout), CLUTTER_ORIENTATION_VERTICAL);
	clutter_actor_set_layout_manager(CLUTTER_ACTOR(self), layout);

	clutter_actor_add_child(CLUTTER_ACTOR(self), priv->title);
	clutter_actor_add_child(CLUTTER_ACTOR(self), priv->itemsContainer);
	xfdashboard_stylable_add_class(XFDASHBOARD_STYLABLE(self), "popup-menu");

	/* Register this actor at focus manager but ensure that this actor is
	 * not focusable initially */
	xfdashboard_actor_set_can_focus(XFDASHBOARD_ACTOR(self), FALSE);
	xfdashboard_focus_manager_register(priv->focusManager, XFDASHBOARD_FOCUSABLE(self));

	/* Add popup menu to stage */
	priv->stage=xfdashboard_application_get_stage(xfdashboard_application_get_default());
	clutter_actor_insert_child_above(CLUTTER_ACTOR(priv->stage), CLUTTER_ACTOR(self), NULL);

	/* Connect signal to get notified when application suspends to cancel pop-up menu */
	priv->suspendSignalID=g_signal_connect_swapped(xfdashboard_application_get_default(),
													"notify::is-suspended",
													G_CALLBACK(_xfdashboard_popup_menu_on_application_suspended_changed),
													self);
}

/* IMPLEMENTATION: Public API */

/**
 * xfdashboard_popup_menu_new:
 *
 * Creates a new #XfdashboardPopupMenu actor
 *
 * Return value: The newly created #XfdashboardPopupMenu
 */
ClutterActor* xfdashboard_popup_menu_new(void)
{
	return(g_object_new(XFDASHBOARD_TYPE_POPUP_MENU, NULL));
}

/**
 * xfdashboard_popup_menu_new_for_source:
 * @inSource: A #ClutterActor which this pop-up menu should depend on
 *
 * Creates a new #XfdashboardPopupMenu actor which depends on actor @inSource.
 * When the actor @inSource is destroyed and the pop-up menu is active then it
 * will be cancelled automatically.
 *
 * Return value: The newly created #XfdashboardPopupMenu
 */
ClutterActor* xfdashboard_popup_menu_new_for_source(ClutterActor *inSource)
{
	g_return_val_if_fail(CLUTTER_IS_ACTOR(inSource), NULL);

	return(g_object_new(XFDASHBOARD_TYPE_POPUP_MENU,
						"source", inSource,
						NULL));
}

/**
 * xfdashboard_popup_menu_get_destroy_on_cancel:
 * @self: A #XfdashboardPopupMenu
 *
 * Retrieves the automatic destruction mode of @self. If automatic destruction mode
 * is %TRUE then the pop-up menu will be destroy by calling clutter_actor_destroy()
 * when it is cancelled, e.g. by calling xfdashboard_popup_menu_cancel().
 *
 * Return value: Returns %TRUE if automatic destruction mode is enabled, otherwise
 *   %FALSE.
 */
gboolean xfdashboard_popup_menu_get_destroy_on_cancel(XfdashboardPopupMenu *self)
{
	g_return_val_if_fail(XFDASHBOARD_IS_POPUP_MENU(self), FALSE);

	return(self->priv->destroyOnCancel);
}

/**
 * xfdashboard_popup_menu_set_destroy_on_cancel:
 * @self: A #XfdashboardPopupMenu
 * @inDestroyOnCancel: The automatic destruction mode to set at @self
 *
 * Sets the automatic destruction mode of @self. If @inDestroyOnCancel is set to
 * %TRUE then the pop-up menu will automatically destroyed by calling clutter_actor_destroy()
 * when it is cancelled, e.g. by calling xfdashboard_popup_menu_cancel().
 */
void xfdashboard_popup_menu_set_destroy_on_cancel(XfdashboardPopupMenu *self, gboolean inDestroyOnCancel)
{
	XfdashboardPopupMenuPrivate			*priv;

	g_return_if_fail(XFDASHBOARD_IS_POPUP_MENU(self));

	priv=self->priv;

	/* Set value if changed */
	if(priv->destroyOnCancel!=inDestroyOnCancel)
	{
		/* Set value */
		priv->destroyOnCancel=inDestroyOnCancel;

		/* Notify about property change */
		g_object_notify_by_pspec(G_OBJECT(self), XfdashboardPopupMenuProperties[PROP_DESTROY_ON_CANCEL]);
	}
}

/**
 * xfdashboard_popup_menu_get_source:
 * @self: A #XfdashboardPopupMenu
 *
 * Retrieves the source actor to @inSource which the pop-up menu at @self depends on.
 *
 * Return value: (transfer none): Returns #ClutterActor which the pop-up menu or
 *   %NULL if no source actor is set.
 */
ClutterActor* xfdashboard_popup_menu_get_source(XfdashboardPopupMenu *self)
{
	g_return_val_if_fail(XFDASHBOARD_IS_POPUP_MENU(self), NULL);

	return(self->priv->source);
}

/**
 * xfdashboard_popup_menu_set_source:
 * @self: A #XfdashboardPopupMenu
 * @inSource: A #ClutterActor which this pop-up menu should depend on or %NULL
 *   if it should not depend on any actor
 *
 * Sets the source actor to @inSource which the pop-up menu at @self depends on.
 * When the actor @inSource is destroyed and the pop-up menu at @self is active
 * then it will be cancelled automatically.
 *
 * In addition the CSS class "popup-menu-source-SOURCE_CLASS_NAME" will be set
 * on pop-up menu at @self, e.g. if source is of type ClutterActor the CSS class
 * "popup-menu-source-ClutterActor" will be set.
 */
void xfdashboard_popup_menu_set_source(XfdashboardPopupMenu *self, ClutterActor *inSource)
{
	XfdashboardPopupMenuPrivate			*priv;
	gchar								*cssClass;

	g_return_if_fail(XFDASHBOARD_IS_POPUP_MENU(self));
	g_return_if_fail(!inSource || CLUTTER_IS_ACTOR(inSource));

	priv=self->priv;

	/* Set value if changed */
	if(priv->source!=inSource)
	{
		/* Release old source if set */
		if(priv->source)
		{
			/* Disconnect signal handler */
			g_signal_handler_disconnect(priv->source, priv->sourceDestroySignalID);
			priv->sourceDestroySignalID=0;

			/* Remove style */
			cssClass=g_strdup_printf("popup-menu-source-%s", G_OBJECT_TYPE_NAME(priv->source));
			xfdashboard_stylable_remove_class(XFDASHBOARD_STYLABLE(self), cssClass);
			g_free(cssClass);

			/* Release source */
			g_object_remove_weak_pointer(G_OBJECT(priv->source), (gpointer*)&priv->source);
			priv->source=NULL;
		}

		/* Set value */
		if(inSource)
		{
			/* Set up source */
			priv->source=inSource;
			g_object_add_weak_pointer(G_OBJECT(priv->source), (gpointer*)&priv->source);

			/* Add style */
			cssClass=g_strdup_printf("popup-menu-source-%s", G_OBJECT_TYPE_NAME(priv->source));
			xfdashboard_stylable_add_class(XFDASHBOARD_STYLABLE(self), cssClass);
			g_free(cssClass);

			/* Connect signal handler */
			priv->sourceDestroySignalID=g_signal_connect_swapped(priv->source,
																	"destroy",
																	G_CALLBACK(_xfdashboard_popup_menu_on_source_destroy),
																	self);
		}

		/* Notify about property change */
		g_object_notify_by_pspec(G_OBJECT(self), XfdashboardPopupMenuProperties[PROP_SOURCE]);
	}
}

/**
 * xfdashboard_popup_menu_get_show_title:
 * @self: A #XfdashboardPopupMenu
 *
 * Retrieves the state if the title of pop-up menu at @self should be shown or not.
 *
 * Return value: Returns %TRUE if title of pop-up menu should be shown and
 *   %FALSE if not.
 */
gboolean xfdashboard_popup_menu_get_show_title(XfdashboardPopupMenu *self)
{
	g_return_val_if_fail(XFDASHBOARD_IS_POPUP_MENU(self), FALSE);

	return(self->priv->showTitle);
}

/**
 * xfdashboard_popup_menu_set_show_title:
 * @self: A #XfdashboardPopupMenu
 * @inShowTitle: Flag indicating if the title of pop-up menu should be shown.
 *
 * If @inShowTitle is %TRUE then the title of the pop-up menu at @self will be
 * shown. If @inShowTitle is %FALSE it will be hidden.
 */
void xfdashboard_popup_menu_set_show_title(XfdashboardPopupMenu *self, gboolean inShowTitle)
{
	XfdashboardPopupMenuPrivate		*priv;

	g_return_if_fail(XFDASHBOARD_IS_POPUP_MENU(self));

	priv=self->priv;

	/* Set value if changed */
	if(priv->showTitle!=inShowTitle)
	{
		/* Set value */
		priv->showTitle=inShowTitle;

		/* Update visibility state of title actor */
		_xfdashboard_popup_menu_update_title_actors_visibility(self);

		/* Notify about property change */
		g_object_notify_by_pspec(G_OBJECT(self), XfdashboardPopupMenuProperties[PROP_SHOW_TITLE]);
	}
}

/**
 * xfdashboard_popup_menu_get_title:
 * @self: A #XfdashboardPopupMenu
 *
 * Retrieves the title of pop-up menu.
 *
 * Return value: Returns string with title of pop-up menu.
 */
const gchar* xfdashboard_popup_menu_get_title(XfdashboardPopupMenu *self)
{
	g_return_val_if_fail(XFDASHBOARD_IS_POPUP_MENU(self), NULL);

	return(xfdashboard_label_get_text(XFDASHBOARD_LABEL(self->priv->title)));
}

/**
 * xfdashboard_popup_menu_set_title:
 * @self: A #XfdashboardPopupMenu
 * @inMarkupTitle: The title to set
 *
 * Sets @inMarkupTitle as title of pop-up menu at @self. The title string can
 * contain markup.
 */
void xfdashboard_popup_menu_set_title(XfdashboardPopupMenu *self, const gchar *inMarkupTitle)
{
	XfdashboardPopupMenuPrivate		*priv;

	g_return_if_fail(XFDASHBOARD_IS_POPUP_MENU(self));
	g_return_if_fail(inMarkupTitle);

	priv=self->priv;

	/* Set value if changed */
	if(g_strcmp0(xfdashboard_label_get_text(XFDASHBOARD_LABEL(priv->title)), inMarkupTitle)!=0)
	{
		/* Set value */
		xfdashboard_label_set_text(XFDASHBOARD_LABEL(priv->title), inMarkupTitle);

		/* Notify about property change */
		g_object_notify_by_pspec(G_OBJECT(self), XfdashboardPopupMenuProperties[PROP_TITLE]);
	}
}

/**
 * xfdashboard_popup_menu_get_show_title_icon:
 * @self: A #XfdashboardPopupMenu
 *
 * Retrieves the state if the icon of title of pop-up menu at @self should be
 * shown or not.
 *
 * Return value: Returns %TRUE if icon of title of pop-up menu should be shown
 *   and %FALSE if not.
 */
gboolean xfdashboard_popup_menu_get_show_title_icon(XfdashboardPopupMenu *self)
{
	g_return_val_if_fail(XFDASHBOARD_IS_POPUP_MENU(self), FALSE);

	return(self->priv->showTitleIcon);
}

/**
 * xfdashboard_popup_menu_set_show_title_icon:
 * @self: A #XfdashboardPopupMenu
 * @inShowTitle: Flag indicating if the icon of title of pop-up menu should be shown.
 *
 * If @inShowTitle is %TRUE then the icon of title of the pop-up menu at @self will be
 * shown. If @inShowTitle is %FALSE it will be hidden.
 */
void xfdashboard_popup_menu_set_show_title_icon(XfdashboardPopupMenu *self, gboolean inShowTitleIcon)
{
	XfdashboardPopupMenuPrivate		*priv;

	g_return_if_fail(XFDASHBOARD_IS_POPUP_MENU(self));

	priv=self->priv;

	/* Set value if changed */
	if(priv->showTitleIcon!=inShowTitleIcon)
	{
		/* Set value */
		priv->showTitleIcon=inShowTitleIcon;

		/* Update visibility state of title actor */
		_xfdashboard_popup_menu_update_title_actors_visibility(self);

		/* Notify about property change */
		g_object_notify_by_pspec(G_OBJECT(self), XfdashboardPopupMenuProperties[PROP_SHOW_TITLE_ICON]);
	}
}

/**
 * xfdashboard_popup_menu_get_title_icon_name:
 * @self: A #XfdashboardPopupMenu
 *
 * Retrieves the stock icon name or file name of title icon of pop-up menu at @self.
 *
 * Return value: Returns string with icon name or file name of pop-up menu's title.
 */
const gchar* xfdashboard_popup_menu_get_title_icon_name(XfdashboardPopupMenu *self)
{
	g_return_val_if_fail(XFDASHBOARD_IS_POPUP_MENU(self), NULL);

	return(xfdashboard_label_get_icon_name(XFDASHBOARD_LABEL(self->priv->title)));
}

/**
 * xfdashboard_popup_menu_set_title_icon_name:
 * @self: A #XfdashboardPopupMenu
 * @inIconName: A string containing the stock icon name or file name for the icon
 *   to be place in the toogle button
 *
 * Sets the icon in title to icon at @inIconName of pop-up menu at @self. If set to
 * %NULL the title icon is hidden.
 */
void xfdashboard_popup_menu_set_title_icon_name(XfdashboardPopupMenu *self, const gchar *inIconName)
{
	XfdashboardPopupMenuPrivate		*priv;

	g_return_if_fail(XFDASHBOARD_IS_POPUP_MENU(self));
	g_return_if_fail(inIconName);

	priv=self->priv;

	/* Set value if changed */
	if(g_strcmp0(xfdashboard_label_get_icon_name(XFDASHBOARD_LABEL(priv->title)), inIconName)!=0)
	{
		/* Set value */
		xfdashboard_label_set_icon_name(XFDASHBOARD_LABEL(priv->title), inIconName);

		/* Notify about property change */
		g_object_notify_by_pspec(G_OBJECT(self), XfdashboardPopupMenuProperties[PROP_TITLE_ICON_NAME]);
	}
}

/**
 * xfdashboard_popup_menu_get_title_gicon:
 * @self: A #XfdashboardPopupMenu
 *
 * Retrieves the title's icon of type #GIcon of pop-up menu at @self.
 *
 * Return value: Returns #GIcon of pop-up menu's title.
 */
GIcon* xfdashboard_popup_menu_get_title_gicon(XfdashboardPopupMenu *self)
{
	g_return_val_if_fail(XFDASHBOARD_IS_POPUP_MENU(self), NULL);

	return(xfdashboard_label_get_gicon(XFDASHBOARD_LABEL(self->priv->title)));
}

/**
 * xfdashboard_popup_menu_set_title_gicon:
 * @self: A #XfdashboardPopupMenu
 * @inIcon: A #GIcon containing the icon image
 *
 * Sets the icon in title to icon at @inIcon of pop-up menu at @self. If set to
 * %NULL the title icon is hidden.
 */
void xfdashboard_popup_menu_set_title_gicon(XfdashboardPopupMenu *self, GIcon *inIcon)
{
	XfdashboardPopupMenuPrivate		*priv;
	GIcon							*icon;

	g_return_if_fail(XFDASHBOARD_IS_POPUP_MENU(self));
	g_return_if_fail(G_IS_ICON(inIcon));

	priv=self->priv;

	/* Set value if changed */
	icon=xfdashboard_label_get_gicon(XFDASHBOARD_LABEL(priv->title));
	if(icon!=inIcon ||
		(icon && inIcon && !g_icon_equal(icon, inIcon)))
	{
		/* Set value */
		xfdashboard_label_set_gicon(XFDASHBOARD_LABEL(priv->title), inIcon);

		/* Notify about property change */
		g_object_notify_by_pspec(G_OBJECT(self), XfdashboardPopupMenuProperties[PROP_TITLE_GICON]);
	}
}

/**
 * xfdashboard_popup_menu_add_item:
 * @self: A #XfdashboardPopupMenu
 * @inMenuItem: A #XfdashboardPopupMenuItem to add to pop-up menu
 *
 * Adds the actor @inMenuItem to end of pop-up menu.
 * 
 * If menu item actor implements the #XfdashboardStylable interface the CSS class
 * popup-menu-item will be added.
 * 
 * Return value: Returns index where item was inserted at or -1 if it failed.
 */
gint xfdashboard_popup_menu_add_item(XfdashboardPopupMenu *self,
										XfdashboardPopupMenuItem *inMenuItem)
{
	return(xfdashboard_popup_menu_insert_item(self, inMenuItem, -1));
}

/**
 * xfdashboard_popup_menu_insert_item:
 * @self: A #XfdashboardPopupMenu
 * @inMenuItem: A #XfdashboardPopupMenuItem to add to pop-up menu
 * @inIndex: The position where to insert this item at
 *
 * Inserts the actor @inMenuItem at position @inIndex into pop-up menu.
 *
 * If position @inIndex is greater than the number of menu items in @self or is
 * less than 0, then the menu item actor @inMenuItem is added to end to
 * pop-up menu.
 *
 * If menu item actor implements the #XfdashboardStylable interface the CSS class
 * popup-menu-item will be added.
 *
 * Return value: Returns index where item was inserted at or -1 if it failed.
 */
gint xfdashboard_popup_menu_insert_item(XfdashboardPopupMenu *self,
										XfdashboardPopupMenuItem *inMenuItem,
										gint inIndex)
{
	XfdashboardPopupMenuPrivate		*priv;

	g_return_val_if_fail(XFDASHBOARD_IS_POPUP_MENU(self), -1);
	g_return_val_if_fail(XFDASHBOARD_IS_POPUP_MENU_ITEM(inMenuItem), -1);
	g_return_val_if_fail(clutter_actor_get_parent(CLUTTER_ACTOR(inMenuItem))==NULL, -1);

	priv=self->priv;

	/* Insert menu item actor to container at requested position */
	clutter_actor_insert_child_at_index(priv->itemsContainer, CLUTTER_ACTOR(inMenuItem), inIndex);

	/* Add CSS class 'popup-menu-item' to newly added menu item */
	if(XFDASHBOARD_IS_STYLABLE(inMenuItem))
	{
		xfdashboard_stylable_add_class(XFDASHBOARD_STYLABLE(inMenuItem), "popup-menu-item");
	}

	/* Connect signal to get notified when user made a selection to cancel pop-up
	 * menu but ensure that it is called nearly at last because the pop-up menu
	 * could be configured to get destroyed automatically when user selected an
	 * item (or cancelled the menu). In this case other signal handler may not be
	 * called if pop-up menu's signal handler is called before. By calling it at
	 * last all other normally connected signal handlers will get be called.
	 */
	g_signal_connect_data(inMenuItem,
							"activated",
							G_CALLBACK(_xfdashboard_popup_menu_on_menu_item_activated),
							self,
							NULL,
							G_CONNECT_AFTER | G_CONNECT_SWAPPED);

	/* Emit signal */
	g_signal_emit(self, XfdashboardPopupMenuSignals[SIGNAL_ITEM_ADDED], 0, inMenuItem);

	/* Get index where menu item actor was inserted at */
	return(xfdashboard_popup_menu_get_item_index(self, inMenuItem));
}

/**
 * xfdashboard_popup_menu_move_item:
 * @self: A #XfdashboardPopupMenu
 * @inMenuItem: A #XfdashboardPopupMenuItem menu item to move
 * @inIndex: The position where to insert this item at
 *
 * Moves the actor @inMenuItem to position @inIndex at pop-up menu @self. If position
 * @inIndex is greater than the number of menu items in @self or is less than 0,
 * then the menu item actor @inMenuItem is added to end to pop-up menu.
 *
 * Return value: Returns %TRUE if moving menu item was successful, otherwise %FALSE.
 */
gboolean xfdashboard_popup_menu_move_item(XfdashboardPopupMenu *self,
											XfdashboardPopupMenuItem *inMenuItem,
											gint inIndex)
{
	XfdashboardPopupMenuPrivate		*priv;

	g_return_val_if_fail(XFDASHBOARD_IS_POPUP_MENU(self), FALSE);
	g_return_val_if_fail(XFDASHBOARD_IS_POPUP_MENU_ITEM(inMenuItem), FALSE);

	priv=self->priv;

	/* Check if menu item is really part of this pop-up menu */
	if(!_xfdashboard_popup_menu_contains_menu_item(self, inMenuItem))
	{
		g_warning(_("%s is not a child of %s and cannot be moved"),
					G_OBJECT_TYPE_NAME(inMenuItem),
					G_OBJECT_TYPE_NAME(self));
		return(FALSE);
	}

	/* Move menu item actor to new position */
	g_object_ref(inMenuItem);
	clutter_actor_remove_child(priv->itemsContainer, CLUTTER_ACTOR(inMenuItem));
	clutter_actor_insert_child_at_index(priv->itemsContainer, CLUTTER_ACTOR(inMenuItem), inIndex);
	g_object_unref(inMenuItem);

	/* If we get here moving menu item actor was successful */
	return(TRUE);
}

/**
 * xfdashboard_popup_menu_get_item:
 * @self: A #XfdashboardPopupMenu
 * @inIndex: The position whose menu item to get
 *
 * Returns the menu item actor at position @inIndex at pop-up menu @self.
 *
 * Return value: Returns #XfdashboardPopupMenuItem of the menu item at position @inIndex or
 *   %NULL in case of errors or if index is out of range that means it is greater
 *   than the number of menu items in @self or is less than 0.
 */
XfdashboardPopupMenuItem* xfdashboard_popup_menu_get_item(XfdashboardPopupMenu *self, gint inIndex)
{
	XfdashboardPopupMenuPrivate		*priv;
	ClutterActor					*menuItem;

	g_return_val_if_fail(XFDASHBOARD_IS_POPUP_MENU(self), NULL);
	g_return_val_if_fail(inIndex>=0 && inIndex<clutter_actor_get_n_children(self->priv->itemsContainer), NULL);

	priv=self->priv;

	/* Get and return child at requested position at items container */
	menuItem=clutter_actor_get_child_at_index(priv->itemsContainer, inIndex);
	return(XFDASHBOARD_POPUP_MENU_ITEM(menuItem));
}

/**
 * xfdashboard_popup_menu_get_item_index:
 * @self: A #XfdashboardPopupMenu
 * @inMenuItem: The #XfdashboardPopupMenuItem menu item whose index to lookup
 *
 * Returns the position for menu item actor @inMenuItem of pop-up menu @self.
 *
 * Return value: Returns the position of the menu item or -1 in case of errors
 *   or if pop-up menu does not have the menu item
 */
gint xfdashboard_popup_menu_get_item_index(XfdashboardPopupMenu *self, XfdashboardPopupMenuItem *inMenuItem)
{
	XfdashboardPopupMenuPrivate		*priv;
	gint							index;
	ClutterActorIter				iter;
	ClutterActor					*child;

	g_return_val_if_fail(XFDASHBOARD_IS_POPUP_MENU(self), -1);
	g_return_val_if_fail(XFDASHBOARD_IS_POPUP_MENU_ITEM(inMenuItem), -1);

	priv=self->priv;

	/* Iterate through menu item and return the index if the requested one was found */
	index=0;

	clutter_actor_iter_init(&iter, priv->itemsContainer);
	while(clutter_actor_iter_next(&iter, &child))
	{
		/* If this child is the one we are looking for return index now */
		if(child==CLUTTER_ACTOR(inMenuItem)) return(index);

		/* Increase index */
		index++;
	}

	/* If we get here we did not find the requested menu item */
	return(-1);
}

/**
 * xfdashboard_popup_menu_remove_item:
 * @self: A #XfdashboardPopupMenu
 * @inMenuItem: A #XfdashboardPopupMenuItem menu item to remove
 *
 * Removes the actor @inMenuItem from pop-up menu @self. When the pop-up menu holds
 * the last reference on that menu item actor then it will be destroyed otherwise
 * it will only be removed from pop-up menu.
 *
 * If the removed menu item actor implements the #XfdashboardStylable interface
 * the CSS class popup-menu-item will be removed also.
 *
 * Return value: Returns %TRUE if moving menu item was successful, otherwise %FALSE.
 */
gboolean xfdashboard_popup_menu_remove_item(XfdashboardPopupMenu *self, XfdashboardPopupMenuItem *inMenuItem)
{
	XfdashboardPopupMenuPrivate		*priv;

	g_return_val_if_fail(XFDASHBOARD_IS_POPUP_MENU(self), FALSE);
	g_return_val_if_fail(XFDASHBOARD_IS_POPUP_MENU_ITEM(inMenuItem), FALSE);

	priv=self->priv;

	/* Check if menu item is really part of this pop-up menu */
	if(!_xfdashboard_popup_menu_contains_menu_item(self, inMenuItem))
	{
		g_warning(_("%s is not a child of %s and cannot be removed"),
					G_OBJECT_TYPE_NAME(inMenuItem),
					G_OBJECT_TYPE_NAME(self));
		return(FALSE);
	}

	/* Take extra reference on actor to remove to keep it alive while working with it */
	g_object_ref(inMenuItem);

	/* Remove CSS class 'popup-menu-item' from menu item going to be removed */
	if(XFDASHBOARD_IS_STYLABLE(inMenuItem))
	{
		xfdashboard_stylable_remove_class(XFDASHBOARD_STYLABLE(inMenuItem), "popup-menu-item");
	}

	/* Remove menu item actor from pop-up menu */
	clutter_actor_remove_child(priv->itemsContainer, CLUTTER_ACTOR(inMenuItem));

	/* Disconnect signal handlers from removed menu item */
	g_signal_handlers_disconnect_by_func(inMenuItem, G_CALLBACK(_xfdashboard_popup_menu_on_menu_item_activated), self);

	/* Emit signal */
	g_signal_emit(self, XfdashboardPopupMenuSignals[SIGNAL_ITEM_REMOVED], 0, inMenuItem);

	/* Release extra reference on actor to took to keep it alive */
	g_object_unref(inMenuItem);

	/* If we get here we removed the menu item actor successfully */
	return(TRUE);
}

/**
 * xfdashboard_popup_menu_activate:
 * @self: A #XfdashboardPopupMenu
 *
 * Displays the pop-up menu at @self and makes it available for selection.
 *
 * This actor will gain the focus automatically and will select the first menu item.
 */
void xfdashboard_popup_menu_activate(XfdashboardPopupMenu *self)
{
	XfdashboardPopupMenuPrivate			*priv;
	GdkDisplay							*display;
#if GTK_CHECK_VERSION(3, 20, 0)
	GdkSeat								*seat;
#else
	GdkDeviceManager					*deviceManager;
#endif
	GdkDevice							*pointerDevice;
	gint								pointerX, pointerY;
	XfdashboardWindowTrackerMonitor		*monitor;
	gint								monitorX, monitorY, monitorWidth, monitorHeight;
	gfloat								x, y, w, h;

	g_return_if_fail(XFDASHBOARD_IS_POPUP_MENU(self));

	priv=self->priv;

	/* If this actor is already active, then do nothing */
	if(priv->isActive) return;

	/* Move popup menu next to pointer similar to tooltips but keep it on current monitor */
	display=gdk_display_get_default();
#if GTK_CHECK_VERSION(3, 20, 0)
	seat=gdk_display_get_default_seat(display);
	pointerDevice=gdk_seat_get_pointer(seat);
#else
	deviceManager=gdk_display_get_device_manager(display);
	pointerDevice=gdk_device_manager_get_client_pointer(deviceManager);
#endif
	gdk_device_get_position(pointerDevice, NULL, &pointerX, &pointerY);
	XFDASHBOARD_DEBUG(self, ACTOR,
						"Pointer is at position %d,%d",
						pointerX, pointerY);

	monitor=xfdashboard_window_tracker_get_monitor_by_position(priv->windowTracker, pointerX, pointerY);
	if(!monitor)
	{
		/* Show error message */
		g_critical(_("Could not find monitor at pointer position %d,%d"),
					pointerX,
					pointerY);

		return;
	}

	xfdashboard_window_tracker_monitor_get_geometry(monitor, &monitorX, &monitorY, &monitorWidth, &monitorHeight);
	XFDASHBOARD_DEBUG(self, ACTOR,
						"Pointer is on monitor %d with position at %d,%d and size of %dx%d",
						xfdashboard_window_tracker_monitor_get_number(monitor),
						monitorX, monitorY,
						monitorWidth, monitorHeight);

	x=pointerX;
	y=pointerY;
	clutter_actor_get_size(CLUTTER_ACTOR(self), &w, &h);
	if(x<monitorX) x=monitorX;
	if((x+w)>=(monitorX+monitorWidth)) x=(monitorX+monitorWidth)-w;
	if(y<monitorY) y=monitorY;
	if((y+h)>=(monitorY+monitorHeight)) y=(monitorY+monitorHeight)-h;
	clutter_actor_set_position(CLUTTER_ACTOR(self), floor(x), floor(y));

	/* Now start capturing event in "capture" phase to stop propagating event to
	 * other actors except this one while popup menu is active.
	 */
	priv->capturedEventSignalID=
		g_signal_connect_swapped(priv->stage,
									"captured-event",
									G_CALLBACK(_xfdashboard_popup_menu_on_captured_event),
									self);

	/* Show popup menu */
	clutter_actor_show(CLUTTER_ACTOR(self));

	/* Set flag that this pop-up menu is now active otherwise we cannot focus
	 * this actor.
	 */
	priv->isActive=TRUE;

	/* Make popup menu focusable as this also marks this actor to be active */
	xfdashboard_actor_set_can_focus(XFDASHBOARD_ACTOR(self), TRUE);

	/* Move focus to popup menu but remember the actor which has current focus */
	priv->oldFocusable=xfdashboard_focus_manager_get_focus(priv->focusManager);
	if(priv->oldFocusable) g_object_add_weak_pointer(G_OBJECT(priv->oldFocusable), &priv->oldFocusable);

	xfdashboard_focus_manager_set_focus(priv->focusManager, XFDASHBOARD_FOCUSABLE(self));
}

/**
 * xfdashboard_popup_menu_cancel:
 * @self: A #XfdashboardPopupMenu
 *
 * Hides the pop-up menu if displayed and stops making it available for selection.
 *
 * The actor tries to refocus the actor which had the focus before this pop-up
 * menu was displayed. If that actor cannot be focused it move the focus to the
 * next focusable actor.
 */
void xfdashboard_popup_menu_cancel(XfdashboardPopupMenu *self)
{
	XfdashboardPopupMenuPrivate			*priv;

	g_return_if_fail(XFDASHBOARD_IS_POPUP_MENU(self));

	priv=self->priv;

	/* Do nothing if pop-up menu is not active */
	if(!priv->isActive) return;

	/* Unset flag that pop-up menu is active to prevent recursive calls on this
	 * function, e.g. if pop-up menu is cancelled because the object instance
	 * is disposed.
	 */
	priv->isActive=FALSE;

	/* Stop capturing events in "capture" phase as this popup menu actor will not
	 * be active anymore.
	 */
	if(priv->capturedEventSignalID)
	{
		g_signal_handler_disconnect(priv->stage, priv->capturedEventSignalID);
		priv->capturedEventSignalID=0;
	}

	/* Move focus to actor which had the focus previously */
	if(priv->oldFocusable)
	{
		/* Remove weak pointer from previously focused actor */
		g_object_remove_weak_pointer(G_OBJECT(priv->oldFocusable), &priv->oldFocusable);

		/* Move focus to previous focussed actor */
		xfdashboard_focus_manager_set_focus(priv->focusManager, priv->oldFocusable);

		/* Forget previous focussed actor */
		priv->oldFocusable=NULL;
	}

	/* Hide popup menu */
	clutter_actor_hide(CLUTTER_ACTOR(self));

	/* Reset popup menu to be not focusable as this also marks this actor is
	 * not active anymore.
	 */
	xfdashboard_actor_set_can_focus(XFDASHBOARD_ACTOR(self), FALSE);

	/* Destroy this pop-up menu actor when destroy-on-cancel was enabled */
	if(priv->destroyOnCancel)
	{
		clutter_actor_destroy(CLUTTER_ACTOR(self));
	}
}
