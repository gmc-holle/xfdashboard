/*
 * popup-menu-item-meta: A meta class for menu items in a pop-up menu
 * 
 * Copyright 2012-2016 Stephan Haller <nomad@froevel.de>
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
 * SECTION:popup-menu-item-meta
 * @short_description: A meta class for menu items in a pop-up menu
 * @include: xfdashboard/popup-menu-item-meta.h
 *
 * A #XfdashboardPopupMenuItemMetaItemMeta will be created to set to each menu item
 * added to the items container of a #XfdashboardPopupMenu. This meta class handles
 * the activation of a menu item and calls the callback function with the associated
 * user data.
 *
 * This class should not be used directly.
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <libxfdashboard/popup-menu-item-meta.h>

#include <glib/gi18n-lib.h>

#include <libxfdashboard/stylable.h>
#include <libxfdashboard/click-action.h>
#include <libxfdashboard/compat.h>


/* Define this class in GObject system */
G_DEFINE_TYPE(XfdashboardPopupMenuItemMeta,
				xfdashboard_popup_menu_item_meta,
				G_TYPE_OBJECT);

/* Private structure - access only by public API if needed */
#define XFDASHBOARD_POPUP_MENU_ITEM_META_GET_PRIVATE(obj) \
	(G_TYPE_INSTANCE_GET_PRIVATE((obj), XFDASHBOARD_TYPE_POPUP_MENU_ITEM_META, XfdashboardPopupMenuItemMetaPrivate))

struct _XfdashboardPopupMenuItemMetaPrivate
{
	/* Properties related */
	XfdashboardPopupMenu						*popupMenu;
	ClutterActor								*menuItem;
	XfdashboardPopupMenuItemActivateCallback	callback;
	gpointer									userData;

	/* Instance related */
	ClutterAction								*clickAction;
};

/* Properties */
enum
{
	PROP_0,

	PROP_POPUP_MENU,
	PROP_MENU_ITEM,
	PROP_CALLBACK,
	PROP_USER_DATA,

	PROP_LAST
};

static GParamSpec* XfdashboardPopupMenuItemMetaProperties[PROP_LAST]={ 0, };

/* Signals */
enum
{
	SIGNAL_ACTIVATED,

	SIGNAL_LAST
};

static guint XfdashboardPopupMenuItemMetaSignals[SIGNAL_LAST]={ 0, };


/* IMPLEMENTATION: Private variables and methods */

/* A menu item was clicked */
static void _xfdashboard_popup_menu_item_meta_clicked(XfdashboardPopupMenuItemMeta *self,
														ClutterActor *inActor,
														gpointer inUserData)
{
	g_return_if_fail(XFDASHBOARD_IS_POPUP_MENU_ITEM_META(self));
	g_return_if_fail(CLUTTER_IS_ACTOR(inActor));
	g_return_if_fail(XFDASHBOARD_IS_CLICK_ACTION(inUserData));

	/* Activate menu item */
	xfdashboard_popup_menu_item_meta_activate(self);
}

/* Set pop-up menu */
static void _xfdashboard_popup_menu_item_meta_set_popup_menu(XfdashboardPopupMenuItemMeta *self,
																XfdashboardPopupMenu *inPopupMenu)
{
	XfdashboardPopupMenuItemMetaPrivate		*priv;

	g_return_if_fail(XFDASHBOARD_IS_POPUP_MENU_ITEM_META(self));
	g_return_if_fail(XFDASHBOARD_IS_POPUP_MENU(inPopupMenu));

	priv=self->priv;

	/* This function can only be called once so no pop-up must be set yet */
	if(priv->popupMenu)
	{
		g_critical(_("Attempting to set pop-up menu %s at %s but it is already set."),
					G_OBJECT_TYPE_NAME(inPopupMenu),
					G_OBJECT_TYPE_NAME(self));
		return;
	}

	/* Set value if changed */
	if(priv->popupMenu!=inPopupMenu)
	{
		/* Set value */
		priv->popupMenu=inPopupMenu;
		g_object_add_weak_pointer(G_OBJECT(priv->popupMenu), (gpointer*)&priv->popupMenu);

		/* Notify about property change */
		g_object_notify_by_pspec(G_OBJECT(self), XfdashboardPopupMenuItemMetaProperties[PROP_POPUP_MENU]);
	}
}

/* Set menu item actor */
static void _xfdashboard_popup_menu_item_meta_set_menu_item(XfdashboardPopupMenuItemMeta *self,
															ClutterActor *inMenuItem)
{
	XfdashboardPopupMenuItemMetaPrivate		*priv;

	g_return_if_fail(XFDASHBOARD_IS_POPUP_MENU_ITEM_META(self));
	g_return_if_fail(CLUTTER_IS_ACTOR(inMenuItem));

	priv=self->priv;

	/* This function can only be called once so no menu item must be set yet */
	if(priv->menuItem || priv->clickAction)
	{
		g_critical(_("Attempting to set menu item %s at %s but it is already set."),
					G_OBJECT_TYPE_NAME(inMenuItem),
					G_OBJECT_TYPE_NAME(self));
		return;
	}

	/* Set value if changed */
	if(priv->menuItem!=inMenuItem)
	{
		/* Set value */
		priv->menuItem=inMenuItem;
		g_object_add_weak_pointer(G_OBJECT(priv->menuItem), (gpointer*)&priv->menuItem);

		/* Apply style for menu item if possible */
		if(XFDASHBOARD_IS_STYLABLE(priv->menuItem))
		{
			xfdashboard_stylable_add_class(XFDASHBOARD_STYLABLE(priv->menuItem), "popup-menu-item");
		}

		/* Create click action and add it to menu item actor */
		priv->clickAction=xfdashboard_click_action_new();
		g_signal_connect_swapped(priv->clickAction,
									"clicked",
									G_CALLBACK(_xfdashboard_popup_menu_item_meta_clicked),
									self);
		clutter_actor_add_action(priv->menuItem, priv->clickAction);

		/* Notify about property change */
		g_object_notify_by_pspec(G_OBJECT(self), XfdashboardPopupMenuItemMetaProperties[PROP_MENU_ITEM]);
	}
}

/* Set callback function */
static void _xfdashboard_popup_menu_item_meta_set_callback(XfdashboardPopupMenuItemMeta *self,
															XfdashboardPopupMenuItemActivateCallback inCallback)
{
	XfdashboardPopupMenuItemMetaPrivate		*priv;

	g_return_if_fail(XFDASHBOARD_IS_POPUP_MENU_ITEM_META(self));

	priv=self->priv;

	/* This function can only be called once so no callback function must be set yet */
	if(priv->callback)
	{
		g_critical(_("Attempting to set callback function at %s but it is already set."),
					G_OBJECT_TYPE_NAME(self));
		return;
	}

	/* Set value if changed */
	if(priv->callback!=inCallback)
	{
		/* Set value */
		priv->callback=inCallback;

		/* Notify about property change */
		g_object_notify_by_pspec(G_OBJECT(self), XfdashboardPopupMenuItemMetaProperties[PROP_CALLBACK]);
	}
}

/* Set user-data function */
static void _xfdashboard_popup_menu_item_meta_set_user_data(XfdashboardPopupMenuItemMeta *self,
															gpointer inUserData)
{
	XfdashboardPopupMenuItemMetaPrivate		*priv;

	g_return_if_fail(XFDASHBOARD_IS_POPUP_MENU_ITEM_META(self));

	priv=self->priv;

	/* This function can only be called once so no user-data must be set yet */
	if(priv->userData)
	{
		g_critical(_("Attempting to set user-data at %s but it is already set."),
					G_OBJECT_TYPE_NAME(self));
		return;
	}

	/* Set value if changed */
	if(priv->userData!=inUserData)
	{
		/* Set value */
		priv->userData=inUserData;

		/* Notify about property change */
		g_object_notify_by_pspec(G_OBJECT(self), XfdashboardPopupMenuItemMetaProperties[PROP_USER_DATA]);
	}
}


/* IMPLEMENTATION: GObject */

/* Dispose this object */
static void _xfdashboard_popup_menu_item_meta_dispose(GObject *inObject)
{
	XfdashboardPopupMenuItemMeta			*self=XFDASHBOARD_POPUP_MENU_ITEM_META(inObject);
	XfdashboardPopupMenuItemMetaPrivate		*priv=self->priv;

	/* Release our allocated variables */
	if(priv->popupMenu)
	{
		g_object_remove_weak_pointer(G_OBJECT(priv->popupMenu), (gpointer*)&priv->popupMenu);
		priv->popupMenu=NULL;
	}

	if(priv->menuItem)
	{
		/* Remove style from menu item if possible */
		if(XFDASHBOARD_IS_STYLABLE(priv->menuItem))
		{
			xfdashboard_stylable_remove_class(XFDASHBOARD_STYLABLE(priv->menuItem), "popup-menu-item");
		}

		/* Remove click action from menu item actor */
		if(priv->clickAction)
		{
			clutter_actor_remove_action(priv->menuItem, priv->clickAction);
			priv->clickAction=NULL;
		}

		/* Release menu item actor */
		g_object_remove_weak_pointer(G_OBJECT(priv->menuItem), (gpointer*)&priv->menuItem);
		priv->menuItem=NULL;
	}

	if(priv->callback)
	{
		priv->callback=NULL;
	}

	if(priv->userData)
	{
		priv->userData=NULL;
	}

	/* Call parent's class dispose method */
	G_OBJECT_CLASS(xfdashboard_popup_menu_item_meta_parent_class)->dispose(inObject);
}

/* Set/get properties */
static void _xfdashboard_popup_menu_item_meta_set_property(GObject *inObject,
															guint inPropID,
															const GValue *inValue,
															GParamSpec *inSpec)
{
	XfdashboardPopupMenuItemMeta			*self=XFDASHBOARD_POPUP_MENU_ITEM_META(inObject);

	switch(inPropID)
	{
		case PROP_POPUP_MENU:
			_xfdashboard_popup_menu_item_meta_set_popup_menu(self, g_value_get_object(inValue));
			break;

		case PROP_MENU_ITEM:
			_xfdashboard_popup_menu_item_meta_set_menu_item(self, g_value_get_object(inValue));
			break;

		case PROP_CALLBACK:
			_xfdashboard_popup_menu_item_meta_set_callback(self, g_value_get_pointer(inValue));
			break;

		case PROP_USER_DATA:
			_xfdashboard_popup_menu_item_meta_set_user_data(self, g_value_get_pointer(inValue));
			break;

		default:
			G_OBJECT_WARN_INVALID_PROPERTY_ID(inObject, inPropID, inSpec);
			break;
	}
}

static void _xfdashboard_popup_menu_item_meta_get_property(GObject *inObject,
															guint inPropID,
															GValue *outValue,
															GParamSpec *inSpec)
{
	XfdashboardPopupMenuItemMeta			*self=XFDASHBOARD_POPUP_MENU_ITEM_META(inObject);
	XfdashboardPopupMenuItemMetaPrivate		*priv=self->priv;

	switch(inPropID)
	{
		case PROP_POPUP_MENU:
			g_value_set_object(outValue, priv->popupMenu);
			break;

		case PROP_MENU_ITEM:
			g_value_set_object(outValue, priv->menuItem);
			break;

		case PROP_CALLBACK:
			g_value_set_pointer(outValue, priv->callback);
			break;

		case PROP_USER_DATA:
			g_value_set_pointer(outValue, priv->userData);
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
static void xfdashboard_popup_menu_item_meta_class_init(XfdashboardPopupMenuItemMetaClass *klass)
{
	GObjectClass			*gobjectClass=G_OBJECT_CLASS(klass);

	/* Override functions */
	gobjectClass->dispose=_xfdashboard_popup_menu_item_meta_dispose;
	gobjectClass->set_property=_xfdashboard_popup_menu_item_meta_set_property;
	gobjectClass->get_property=_xfdashboard_popup_menu_item_meta_get_property;

	/* Set up private structure */
	g_type_class_add_private(klass, sizeof(XfdashboardPopupMenuItemMetaPrivate));

	/* Define properties */
	/**
	 * XfdashboardPopupMenuItemMeta:popup-menu:
	 *
	 * The pop-up menu which contains the menu item where this meta belongs to.
	 */
	XfdashboardPopupMenuItemMetaProperties[PROP_POPUP_MENU]=
		g_param_spec_object("popup-menu",
							_("Pop-up menu"),
							_("The pop-up menu containing the menu item where this meta belongs to"),
							XFDASHBOARD_TYPE_POPUP_MENU,
							G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS | G_PARAM_CONSTRUCT_ONLY);

	/**
	 * XfdashboardPopupMenuItemMeta:menu-item:
	 *
	 * The menu item where this meta belongs to.
	 */
	XfdashboardPopupMenuItemMetaProperties[PROP_MENU_ITEM]=
		g_param_spec_object("menu-item",
							_("Menu item"),
							_("The menu item where this meta belongs to"),
							CLUTTER_TYPE_ACTOR,
							G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS | G_PARAM_CONSTRUCT_ONLY);

	/**
	 * XfdashboardPopupMenuItemMeta:callback:
	 *
	 * The callback function to call when this meta for a menu item is activated.
	 */
	XfdashboardPopupMenuItemMetaProperties[PROP_CALLBACK]=
		g_param_spec_pointer("callback",
								_("Callback"),
								_("The callback function to call when this meta is activated"),
								G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS | G_PARAM_CONSTRUCT_ONLY);

	/**
	 * XfdashboardPopupMenuItemMeta:user-data:
	 *
	 * The user-data to pass to callback function which is called when this meta
	 * for a menu item is activated.
	 */
	XfdashboardPopupMenuItemMetaProperties[PROP_USER_DATA]=
		g_param_spec_pointer("user-data",
								_("User data"),
								_("The user data to pass to callback function"),
								G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS | G_PARAM_CONSTRUCT_ONLY);

	g_object_class_install_properties(gobjectClass, PROP_LAST, XfdashboardPopupMenuItemMetaProperties);

	/* Define signals */
	/**
	 * XfdashboardPopupMenuItemMeta::activated:
	 * @self: The pop-up menu which was activated
	 *
	 * The ::activated signal is emitted when the pop-up menu is shown and the
	 * user can perform an action by selecting an item.
	 */
	XfdashboardPopupMenuItemMetaSignals[SIGNAL_ACTIVATED]=
		g_signal_new("activated",
						G_TYPE_FROM_CLASS(klass),
						G_SIGNAL_RUN_LAST,
						G_STRUCT_OFFSET(XfdashboardPopupMenuItemMetaClass, activated),
						NULL,
						NULL,
						g_cclosure_marshal_VOID__VOID,
						G_TYPE_NONE,
						0);
}

/* Object initialization
 * Create private structure and set up default values
 */
static void xfdashboard_popup_menu_item_meta_init(XfdashboardPopupMenuItemMeta *self)
{
	XfdashboardPopupMenuItemMetaPrivate		*priv;

	priv=self->priv=XFDASHBOARD_POPUP_MENU_ITEM_META_GET_PRIVATE(self);

	/* Set up default values */
	priv->popupMenu=NULL;
	priv->menuItem=NULL;
	priv->callback=NULL;
	priv->userData=NULL;
	priv->clickAction=NULL;
}

/* IMPLEMENTATION: Public API */

/**
 * xfdashboard_popup_menu_item_meta_new:
 * @inPopupMenu: The #XfdashboardPopupMenu where the menu item belongs to
 * @inMenuItem: A #ClutterActor menu item of pop-up menu
 * @inCallback: (type XfdashboardPopupMenuItemActivateCallback): The function to
 *   be called when the menu item is activated
 * @inUserData: Data to be passed to @inCallback
 *
 * Creates a new #XfdashboardPopupMenuItemMeta meta object instance for the menu
 * item actor @inMenuItem at pop-up menu @inPopupMenu. When the menu item is
 * clicked or xfdashboard_popup_menu_item_meta_activate() is called for the menu
 * item the callback function @inCallback is called and the data @inUserData is
 * passed to that callback function.
 * 
 * Return value: The newly created #XfdashboardPopupMenuItemMeta
 */
XfdashboardPopupMenuItemMeta* xfdashboard_popup_menu_item_meta_new(XfdashboardPopupMenu *inPopupMenu,
																	ClutterActor *inMenuItem,
																	XfdashboardPopupMenuItemActivateCallback inCallback,
																	gpointer inUserData)
{
	GObject				*meta;

	g_return_val_if_fail(XFDASHBOARD_IS_POPUP_MENU(inPopupMenu), NULL);
	g_return_val_if_fail(CLUTTER_IS_ACTOR(inMenuItem), NULL);

	/* Create meta object */
	meta=g_object_new(XFDASHBOARD_TYPE_POPUP_MENU_ITEM_META,
						"popup-menu", inPopupMenu,
						"menu-item", inMenuItem,
						"callback", inCallback,
						"user-data", inUserData,
						NULL);

	/* Return newly created meta object */
	return(XFDASHBOARD_POPUP_MENU_ITEM_META(meta));
}

/**
 * xfdashboard_popup_menu_item_meta_new:
 * @self: A #XfdashboardPopupMenuItemMeta
 *
 * Activates the menu item associated with this #XfdashboardPopupMenuItemMeta by
 * calling the callback function and passing the user data to that function. Also
 * the signal "activated" will be emitted after the callback function was called.
 */
void xfdashboard_popup_menu_item_meta_activate(XfdashboardPopupMenuItemMeta *self)
{
	XfdashboardPopupMenuItemMetaPrivate		*priv;

	g_return_if_fail(XFDASHBOARD_IS_POPUP_MENU_ITEM_META(self));

	priv=self->priv;

	/* Call the callback handler and pass data and associated user data to it */
	if(priv->callback)
	{
		(priv->callback)(priv->popupMenu, priv->menuItem, priv->userData);
	}

	/* Emit signal for activation */
	g_signal_emit(self, XfdashboardPopupMenuItemMetaSignals[SIGNAL_ACTIVATED], 0);
}

/**
 * xfdashboard_popup_menu_item_meta_get_popup_menu:
 * @self: A #XfdashboardPopupMenuItemMeta
 *
 * Retrieves the associated pop-up menu of @self.
 *
 * Return value: (transfer none): The associated pop-up menu of type #XfdashboardPopupMenu
 */
XfdashboardPopupMenu* xfdashboard_popup_menu_item_meta_get_popup_menu(XfdashboardPopupMenuItemMeta *self)
{
	g_return_val_if_fail(XFDASHBOARD_IS_POPUP_MENU_ITEM_META(self), NULL);

	return(self->priv->popupMenu);
}

/**
 * xfdashboard_popup_menu_item_meta_get_menu_item:
 * @self: A #XfdashboardPopupMenuItemMeta
 *
 * Retrieves the associated menu item actor of @self.
 *
 * Return value: (transfer none): The associated menu item actor
 */
ClutterActor* xfdashboard_popup_menu_item_meta_get_menu_item(XfdashboardPopupMenuItemMeta *self)
{
	g_return_val_if_fail(XFDASHBOARD_IS_POPUP_MENU_ITEM_META(self), NULL);

	return(self->priv->menuItem);
}

/**
 * xfdashboard_popup_menu_item_meta_get_callback:
 * @self: A #XfdashboardPopupMenuItemMeta
 *
 * Retrieves the associated callback function of @self which is called when the
 * menu item is activated.
 *
 * Return value: The pointer to callback function
 */
gpointer xfdashboard_popup_menu_item_meta_get_callback(XfdashboardPopupMenuItemMeta *self)
{
	g_return_val_if_fail(XFDASHBOARD_IS_POPUP_MENU_ITEM_META(self), NULL);

	return(self->priv->callback);
}

/**
 * xfdashboard_popup_menu_item_meta_get_user_data:
 * @self: A #XfdashboardPopupMenuItemMeta
 *
 * Retrieves the associated user data passed to callback function of @self which
 * is called when the menu item is activated.
 *
 * Return value: The associated user data
 */
gpointer xfdashboard_popup_menu_item_meta_get_user_data(XfdashboardPopupMenuItemMeta *self)
{
	g_return_val_if_fail(XFDASHBOARD_IS_POPUP_MENU_ITEM_META(self), NULL);

	return(self->priv->userData);
}
