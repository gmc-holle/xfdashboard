/*
 * applications-view.c: A view showing all installed applications as menu
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

#define LIST_VIEW

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "enums.h"
#include "applications-view.h"
#include "application-entry.h"
#include "application-icon.h"
#include "fill-box-layout.h"
#include "viewpad.h"
#include "common.h"

/* Define this class in GObject system */
G_DEFINE_TYPE(XfdashboardApplicationsView,
				xfdashboard_applications_view,
				XFDASHBOARD_TYPE_VIEW)

/* Private structure - access only by public API if needed */
#define XFDASHBOARD_APPLICATIONS_VIEW_GET_PRIVATE(obj) \
	(G_TYPE_INSTANCE_GET_PRIVATE((obj), XFDASHBOARD_TYPE_APPLICATIONS_VIEW, XfdashboardApplicationsViewPrivate))

struct _XfdashboardApplicationsViewPrivate
{
	/* List mode */
	gint				listMode;

	gboolean			hasOldHorizontalPolicy;
	GtkPolicyType		oldHorizontalPolicy;
	
	/* Active menu */
	GarconMenu			*activeMenu;
};

/* Properties */
enum
{
	PROP_0,

	PROP_LIST_MODE,
	PROP_ACTIVE_MENU,
	
	PROP_LAST
};

static GParamSpec* XfdashboardApplicationsViewProperties[PROP_LAST]={ 0, };

/* IMPLEMENTATION: Private variables and methods */
void _xfdashboard_applications_view_set_active_menu(XfdashboardApplicationsView *self, const GarconMenu *inMenu);

/* "Go up ..." item was clicked */
void _xfdashboard_applications_view_on_parent_menu_entry_clicked(ClutterActor *inActor, gpointer inUserData)
{
	g_return_if_fail(XFDASHBOARD_IS_APPLICATIONS_VIEW(inUserData));

	/* If sub-menu was clicked change into it otherwise start application
	 * of menu item and quit application
	 */
	XfdashboardApplicationsView			*self=XFDASHBOARD_APPLICATIONS_VIEW(inUserData);
	XfdashboardApplicationsViewPrivate	*priv=XFDASHBOARD_APPLICATIONS_VIEW(self)->priv;
	GarconMenuElement					*element=NULL;

	if(priv->listMode==XFDASHBOARD_APPLICATIONS_VIEW_LIST)
	{
		XfdashboardApplicationMenuEntry	*item;

		g_return_if_fail(XFDASHBOARD_IS_APPLICATION_MENU_ENTRY(inActor));

		item=XFDASHBOARD_APPLICATION_MENU_ENTRY(inActor);
		element=(GarconMenuElement*)xfdashboard_application_menu_entry_get_menu_element(item);
	}
		else
		{
			XfdashboardApplicationIcon		*item;

			g_return_if_fail(XFDASHBOARD_IS_APPLICATION_ICON(inActor));

			item=XFDASHBOARD_APPLICATION_ICON(inActor);
			element=(GarconMenuElement*)xfdashboard_application_icon_get_menu_element(item);
		}

	/* Set new active menu */
	g_return_if_fail(element);
	
	_xfdashboard_applications_view_set_active_menu(self, GARCON_MENU(element));
}

/* Menu item was clicked */
static gboolean _xfdashboard_applications_view_on_entry_clicked(ClutterActor *inActor, gpointer inUserData)
{
	g_return_val_if_fail(XFDASHBOARD_IS_APPLICATIONS_VIEW(inUserData), FALSE);

	/* If sub-menu was clicked change into it otherwise start application
	 * of menu item and quit application
	 */
	XfdashboardApplicationsView			*self=XFDASHBOARD_APPLICATIONS_VIEW(inUserData);
	XfdashboardApplicationsViewPrivate	*priv=XFDASHBOARD_APPLICATIONS_VIEW(self)->priv;
	GarconMenuElement					*element=NULL;

	if(priv->listMode==XFDASHBOARD_APPLICATIONS_VIEW_LIST)
	{
		XfdashboardApplicationMenuEntry	*item;

		g_return_val_if_fail(XFDASHBOARD_IS_APPLICATION_MENU_ENTRY(inActor), FALSE);

		item=XFDASHBOARD_APPLICATION_MENU_ENTRY(inActor);
		element=(GarconMenuElement*)xfdashboard_application_menu_entry_get_menu_element(item);
	}
		else
		{
			XfdashboardApplicationIcon		*item;

			g_return_val_if_fail(XFDASHBOARD_IS_APPLICATION_ICON(inActor), FALSE);

			item=XFDASHBOARD_APPLICATION_ICON(inActor);
			element=(GarconMenuElement*)xfdashboard_application_icon_get_menu_element(item);
		}

	g_return_val_if_fail(element, FALSE);

	if(GARCON_IS_MENU(element))
	{
		_xfdashboard_applications_view_set_active_menu(self, GARCON_MENU(element));
	}
		else
		{
			const gchar				*command=garcon_menu_item_get_command(GARCON_MENU_ITEM(element));
			const gchar				*name=garcon_menu_item_get_name(GARCON_MENU_ITEM(element));
			GAppInfo				*appInfo;
			GAppInfoCreateFlags		flags=G_APP_INFO_CREATE_NONE;
			GError					*error=NULL;

			/* Create application info for launching */
			if(garcon_menu_item_supports_startup_notification(GARCON_MENU_ITEM(element))) flags|=G_APP_INFO_CREATE_SUPPORTS_STARTUP_NOTIFICATION;
			if(garcon_menu_item_requires_terminal(GARCON_MENU_ITEM(element))) flags|=G_APP_INFO_CREATE_NEEDS_TERMINAL;

			appInfo=g_app_info_create_from_commandline(command, name, flags, &error);
			if(!appInfo || error)
			{
				g_warning("Could not create application information for command '%s': %s",
							command,
							(error && error->message) ?  error->message : "unknown error");
				if(error) g_error_free(error);
				if(appInfo) g_object_unref(appInfo);
				return(FALSE);
			}

			/* Launch application */
			error=NULL;
			if(!g_app_info_launch(G_APP_INFO(appInfo), NULL, NULL, &error))
			{
				g_warning("Could not launch application: %s",
							(error && error->message) ?  error->message : "unknown error");
				if(error) g_error_free(error);
				g_object_unref(appInfo);
				return(FALSE);
			}

			/* Clean up allocated resources */
			g_object_unref(appInfo);
			
			/* After successfully spawn new process of application (which does not
			 * indicate successful startup of application) quit application
			 */
			clutter_main_quit();
		}

	return(TRUE);
}

/* Set active menu */
void _xfdashboard_applications_view_refresh(XfdashboardApplicationsView *self)
{
	g_return_if_fail(XFDASHBOARD_IS_APPLICATIONS_VIEW(self));

	XfdashboardApplicationsViewPrivate	*priv=XFDASHBOARD_APPLICATIONS_VIEW(self)->priv;
	GList								*items;

	/* Remove all item actors and reset scroll bars */
	xfdashboard_view_remove_all(XFDASHBOARD_VIEW(self));
	xfdashboard_view_reset_scrollbars(XFDASHBOARD_VIEW(self));

	/* If there is no active menu do nothing after removing item actors */
	if(priv->activeMenu==NULL) return;

	/* If menu shown is not root menu add "Go up ..." item first */
	if(priv->activeMenu!=xfdashboard_get_application_menu())
	{
		ClutterActor					*actor;

		if(priv->listMode==XFDASHBOARD_APPLICATIONS_VIEW_LIST)
		{
			actor=xfdashboard_application_menu_entry_new_with_custom(GARCON_MENU_ELEMENT(garcon_menu_get_parent(priv->activeMenu)),
																		GTK_STOCK_GO_UP,
																		"Back",
																		"Go back to previous menu");
		}
			else
			{
				actor=xfdashboard_application_icon_new_with_custom(GARCON_MENU_ELEMENT(garcon_menu_get_parent(priv->activeMenu)),
																			GTK_STOCK_GO_UP,
																			"Back",
																			"Go back to previous menu");
			}

		clutter_container_add_actor(CLUTTER_CONTAINER(self), actor);
		g_signal_connect(actor, "clicked", G_CALLBACK(_xfdashboard_applications_view_on_parent_menu_entry_clicked), self);
	}

	/* Create item actors for new active menu */
	for(items=garcon_menu_get_elements(priv->activeMenu); items; items=items->next)
	{
		GarconMenuElement				*item=GARCON_MENU_ELEMENT(items->data);
		ClutterActor					*actor;

		/* Only add menu items or sub-menus */
		if((GARCON_IS_MENU(item) || GARCON_IS_MENU_ITEM(item)) &&
			garcon_menu_element_get_visible(item) &&
			!garcon_menu_element_get_no_display(item) &&
			garcon_menu_element_get_show_in_environment(item))
		{
			/* Create actor */
			if(priv->listMode==XFDASHBOARD_APPLICATIONS_VIEW_LIST)
			{
				actor=xfdashboard_application_menu_entry_new_with_menu_item(item);
			}
				else
				{
					actor=xfdashboard_application_icon_new_by_menu_item(item);
				}

			clutter_container_add_actor(CLUTTER_CONTAINER(self), actor);
			g_signal_connect(actor, "clicked", G_CALLBACK(_xfdashboard_applications_view_on_entry_clicked), self);
		}
	}
}

/* Set active menu */
void _xfdashboard_applications_view_set_active_menu(XfdashboardApplicationsView *self, const GarconMenu *inMenu)
{
	g_return_if_fail(XFDASHBOARD_IS_APPLICATIONS_VIEW(self));
	g_return_if_fail(!inMenu || GARCON_IS_MENU(inMenu));

	XfdashboardApplicationsViewPrivate	*priv=XFDASHBOARD_APPLICATIONS_VIEW(self)->priv;

	/* Do not anything if new active menu is the same as before */
	if(inMenu==priv->activeMenu) return;

	/* Set new active menu */
	if(priv->activeMenu) g_object_unref(priv->activeMenu);
	priv->activeMenu=NULL;
	if(inMenu) priv->activeMenu=GARCON_MENU(g_object_ref((gpointer)inMenu));

	/* Set up items to display */
	_xfdashboard_applications_view_refresh(self);
}

/* Create and return layout manager object for list mode */
ClutterLayoutManager* _xfdashboard_applications_view_get_layout_manager_for_list_mode(XfdashboardApplicationsViewListMode inListMode)
{
	ClutterLayoutManager				*layout=NULL;

	if(inListMode==XFDASHBOARD_APPLICATIONS_VIEW_LIST)
	{
		layout=xfdashboard_fill_box_layout_new();
		xfdashboard_fill_box_layout_set_vertical(XFDASHBOARD_FILL_BOX_LAYOUT(layout), TRUE);
		xfdashboard_fill_box_layout_set_spacing(XFDASHBOARD_FILL_BOX_LAYOUT(layout), 1.0f);
		xfdashboard_fill_box_layout_set_homogenous(XFDASHBOARD_FILL_BOX_LAYOUT(layout), TRUE);
	}
		else
		{
			layout=clutter_flow_layout_new(CLUTTER_FLOW_HORIZONTAL);
			clutter_flow_layout_set_column_spacing(CLUTTER_FLOW_LAYOUT(layout), 4.0f);
			clutter_flow_layout_set_row_spacing(CLUTTER_FLOW_LAYOUT(layout), 4.0f);
			clutter_flow_layout_set_homogeneous(CLUTTER_FLOW_LAYOUT(layout), TRUE);
		}

	return(layout);
}

/* Set or restore policy for horizontal scroll bar */
void _xfdashboard_applications_view_set_scroll_bar_policies(XfdashboardApplicationsView *self, gboolean inDoRestore)
{
	g_return_if_fail(XFDASHBOARD_IS_APPLICATIONS_VIEW(self));

	XfdashboardApplicationsViewPrivate	*priv=XFDASHBOARD_APPLICATIONS_VIEW(self)->priv;
	ClutterActor						*parent;

	/* Dirty hack to prevent a horizontal scroll bar shown although view fits
	 * the allocation
	 */
	parent=clutter_actor_get_parent(CLUTTER_ACTOR(self));
	if(parent && XFDASHBOARD_IS_VIEWPAD(parent))
	{
		XfdashboardViewpad				*viewpad=XFDASHBOARD_VIEWPAD(parent);

		/* Check if we should restore old policies */
		if(inDoRestore && priv->hasOldHorizontalPolicy)
		{
			GtkPolicyType	vPolicy;
		
			xfdashboard_viewpad_get_scrollbar_policy(viewpad, NULL, &vPolicy);
			xfdashboard_viewpad_set_scrollbar_policy(viewpad, priv->oldHorizontalPolicy, vPolicy);
			priv->hasOldHorizontalPolicy=FALSE;
		}

		/* Check if we should set new policies */
		if(!inDoRestore &&
			priv->listMode==XFDASHBOARD_APPLICATIONS_VIEW_ICON &&
			!priv->hasOldHorizontalPolicy)
		{
			GtkPolicyType	vPolicy;
		
			xfdashboard_viewpad_get_scrollbar_policy(viewpad, &priv->oldHorizontalPolicy, &vPolicy);
			xfdashboard_viewpad_set_scrollbar_policy(viewpad, GTK_POLICY_NEVER, vPolicy);
			priv->hasOldHorizontalPolicy=TRUE;
		}
	}
}

/* View was activated or deactivated */
static void _xfdashboard_applications_view_on_activation_changed(ClutterActor *inActor, gpointer inUserData)
{
	g_return_if_fail(XFDASHBOARD_IS_APPLICATIONS_VIEW(inActor));

	_xfdashboard_applications_view_set_scroll_bar_policies(XFDASHBOARD_APPLICATIONS_VIEW(inActor), GPOINTER_TO_INT(inUserData));
}

/* Set list mode and refresh display of this actor */
void _xfdashboard_applications_view_set_list_mode(XfdashboardApplicationsView *self, XfdashboardApplicationsViewListMode inListMode)
{
	g_return_if_fail(XFDASHBOARD_IS_APPLICATIONS_VIEW(self));

	XfdashboardApplicationsViewPrivate	*priv=XFDASHBOARD_APPLICATIONS_VIEW(self)->priv;
	ClutterLayoutManager				*layout=NULL;

	/* Only set new list mode if it differs from current one
	 * This prevents also expensive relayouting of all items.
	 */
	if(inListMode==priv->listMode) return;

	priv->listMode=inListMode;
	layout=_xfdashboard_applications_view_get_layout_manager_for_list_mode(priv->listMode);
	xfdashboard_view_set_layout_manager(XFDASHBOARD_VIEW(self), layout);
	_xfdashboard_applications_view_set_scroll_bar_policies(self, FALSE);
	
	/* Set up items to display */
	_xfdashboard_applications_view_refresh(self);
}

/* IMPLEMENTATION: GObject */

/* Dispose this object */
static void xfdashboard_applications_view_dispose(GObject *inObject)
{
	XfdashboardApplicationsView		*self=XFDASHBOARD_APPLICATIONS_VIEW(inObject);

	/* Release allocated resources */

	/* Reset scroll bar policy if necessary by setting view to list view.
	 * That's because of the dirty hack we need to do when switching to icon view
	 */
	_xfdashboard_applications_view_set_list_mode(self, XFDASHBOARD_APPLICATIONS_VIEW_LIST);

	/* Set to no-active-menu to remove all item actors and releasing reference to menu */
	_xfdashboard_applications_view_set_active_menu(self, NULL);

	/* Call parent's class dispose method */
	G_OBJECT_CLASS(xfdashboard_applications_view_parent_class)->dispose(inObject);
}

/* Set/get properties */
static void xfdashboard_applications_view_set_property(GObject *inObject,
													guint inPropID,
													const GValue *inValue,
													GParamSpec *inSpec)
{
	XfdashboardApplicationsView		*self=XFDASHBOARD_APPLICATIONS_VIEW(inObject);
	
	switch(inPropID)
	{
		case PROP_LIST_MODE:
			_xfdashboard_applications_view_set_list_mode(self, g_value_get_enum(inValue));
			break;

		case PROP_ACTIVE_MENU:
			_xfdashboard_applications_view_set_active_menu(self, g_value_get_object(inValue));
			break;

		default:
			G_OBJECT_WARN_INVALID_PROPERTY_ID(inObject, inPropID, inSpec);
			break;
	}
}

static void xfdashboard_applications_view_get_property(GObject *inObject,
													guint inPropID,
													GValue *outValue,
													GParamSpec *inSpec)
{
	XfdashboardApplicationsView		*self=XFDASHBOARD_APPLICATIONS_VIEW(inObject);

	switch(inPropID)
	{
		case PROP_LIST_MODE:
			g_value_set_enum(outValue, self->priv->listMode);
			break;

		case PROP_ACTIVE_MENU:
			g_value_set_object(outValue, self->priv->activeMenu);
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
static void xfdashboard_applications_view_class_init(XfdashboardApplicationsViewClass *klass)
{
	GObjectClass			*gobjectClass=G_OBJECT_CLASS(klass);

	/* Override functions */
	gobjectClass->dispose=xfdashboard_applications_view_dispose;
	gobjectClass->set_property=xfdashboard_applications_view_set_property;
	gobjectClass->get_property=xfdashboard_applications_view_get_property;

	/* Set up private structure */
	g_type_class_add_private(klass, sizeof(XfdashboardApplicationsViewPrivate));

	/* Define properties */
	XfdashboardApplicationsViewProperties[PROP_LIST_MODE]=
		g_param_spec_enum("list-mode",
							"List mode",
							"List mode to use in view",
							XFDASHBOARD_TYPE_APPLICATIONS_VIEW_LIST_MODE,
							XFDASHBOARD_APPLICATIONS_VIEW_ICON,
							G_PARAM_READWRITE | G_PARAM_CONSTRUCT);

	XfdashboardApplicationsViewProperties[PROP_ACTIVE_MENU]=
		g_param_spec_object("active-menu",
							"Current active menu",
							"The active menu shown currently",
							GARCON_TYPE_MENU,
							G_PARAM_READWRITE | G_PARAM_CONSTRUCT);

	g_object_class_install_properties(gobjectClass, PROP_LAST, XfdashboardApplicationsViewProperties);
}

/* Object initialization
 * Create private structure and set up default values
 */
static void xfdashboard_applications_view_init(XfdashboardApplicationsView *self)
{
	self->priv=XFDASHBOARD_APPLICATIONS_VIEW_GET_PRIVATE(self);

	/* Set up default values */
	self->priv->activeMenu=NULL;
	self->priv->listMode=-1;
	self->priv->hasOldHorizontalPolicy=FALSE;

	/* Connect signals */
	g_signal_connect(self, "activated", G_CALLBACK(_xfdashboard_applications_view_on_activation_changed), GINT_TO_POINTER(FALSE));
	g_signal_connect(self, "deactivated", G_CALLBACK(_xfdashboard_applications_view_on_activation_changed), GINT_TO_POINTER(TRUE));
}

/* Implementation: Public API */

/* Create new actor */
ClutterActor* xfdashboard_applications_view_new()
{
	/* Create layout manager for default list mode */
	ClutterLayoutManager				*layout=NULL;

	layout=_xfdashboard_applications_view_get_layout_manager_for_list_mode(XFDASHBOARD_APPLICATIONS_VIEW_LIST);

	/* Create actor */
	return(g_object_new(XFDASHBOARD_TYPE_APPLICATIONS_VIEW,
						"view-name", "Applications",
						"list-mode", XFDASHBOARD_APPLICATIONS_VIEW_LIST,
						//"layout-manager", layout,
						NULL));
}

/* Get/set list mode */
XfdashboardApplicationsViewListMode xfdashboard_applications_view_get_list_mode(XfdashboardApplicationsView *self)
{
	g_return_val_if_fail(XFDASHBOARD_IS_APPLICATIONS_VIEW(self), XFDASHBOARD_APPLICATIONS_VIEW_ICON);

	return(self->priv->listMode);
}

void xfdashboard_applications_view_set_list_mode(XfdashboardApplicationsView *self, XfdashboardApplicationsViewListMode inListMode)
{
	g_return_if_fail(XFDASHBOARD_IS_APPLICATIONS_VIEW(self));
	
	/* Set list mode */
	_xfdashboard_applications_view_set_list_mode(self, inListMode);
}

/* Get/set active menu */
const GarconMenu* xfdashboard_applications_view_get_active_menu(XfdashboardApplicationsView *self)
{
	g_return_val_if_fail(XFDASHBOARD_IS_APPLICATIONS_VIEW(self), NULL);

	return(self->priv->activeMenu);
}

void xfdashboard_applications_view_set_active_menu(XfdashboardApplicationsView *self, const GarconMenu *inMenu)
{
	g_return_if_fail(XFDASHBOARD_IS_APPLICATIONS_VIEW(self));
	g_return_if_fail(GARCON_IS_MENU(inMenu));
	
	/* Set active menu */
	_xfdashboard_applications_view_set_active_menu(self, inMenu);
}
