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
#include "application-icon.h"
#include "fill-box-layout.h"
#include "viewpad.h"
#include "common.h"

#include <string.h>

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
	gint					listMode;

	gboolean				hasOldHorizontalPolicy;
	XfdashboardPolicy		oldHorizontalPolicy;
	
	/* Active menu */
	GarconMenu				*activeMenu;

	/* Filter */
	gchar					*filterText;
	GList					*filteredItems;
};

/* Properties */
enum
{
	PROP_0,

	PROP_LIST_MODE,
	PROP_ACTIVE_MENU,
	PROP_FILTER_TEXT,
	
	PROP_LAST
};

static GParamSpec* XfdashboardApplicationsViewProperties[PROP_LAST]={ 0, };

/* IMPLEMENTATION: Private variables and methods */
void _xfdashboard_applications_view_set_active_menu(XfdashboardApplicationsView *self, const GarconMenu *inMenu);

/* "Go up ..." item was clicked */
void _xfdashboard_applications_view_on_parent_menu_entry_clicked(ClutterActor *inActor, gpointer inUserData)
{
	g_return_if_fail(XFDASHBOARD_IS_APPLICATIONS_VIEW(inUserData));
	g_return_if_fail(XFDASHBOARD_IS_APPLICATION_ICON(inActor));

	/* Change to parent menu */
	XfdashboardApplicationsView			*self=XFDASHBOARD_APPLICATIONS_VIEW(inUserData);
	XfdashboardApplicationIcon			*item=XFDASHBOARD_APPLICATION_ICON(inActor);
	GarconMenuElement					*element=NULL;

	/* Set new active menu */
	element=(GarconMenuElement*)xfdashboard_application_icon_get_menu_element(item);
	g_return_if_fail(element);
	
	_xfdashboard_applications_view_set_active_menu(self, GARCON_MENU(element));
}

/* Menu item was clicked */
static gboolean _xfdashboard_applications_view_on_entry_clicked(ClutterActor *inActor, gpointer inUserData)
{
	g_return_val_if_fail(XFDASHBOARD_IS_APPLICATIONS_VIEW(inUserData), FALSE);
	g_return_val_if_fail(XFDASHBOARD_IS_APPLICATION_ICON(inActor), FALSE);
	
	/* If sub-menu was clicked change into it otherwise start application
	 * of menu item and quit application
	 */
	XfdashboardApplicationsView			*self=XFDASHBOARD_APPLICATIONS_VIEW(inUserData);
	XfdashboardApplicationIcon			*item=XFDASHBOARD_APPLICATION_ICON(inActor);
	GarconMenuElement					*element=NULL;

	element=(GarconMenuElement*)xfdashboard_application_icon_get_menu_element(item);
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

/* Create search result (if current list of found items is NULL) or
 * update existing search result
 */
void _xfdashboard_applications_view_filter_items(XfdashboardApplicationsView *self,
													GList *inMenuItems,
													GList **outFoundItems)
{
	g_return_if_fail(XFDASHBOARD_IS_APPLICATIONS_VIEW(self));
	g_return_if_fail(outFoundItems!=NULL);

	XfdashboardApplicationsViewPrivate	*priv=XFDASHBOARD_APPLICATIONS_VIEW(self)->priv;

	/* Iterate through list and check for matches */
	for( ; inMenuItems; inMenuItems=inMenuItems->next)
	{
		GarconMenuElement				*item=GARCON_MENU_ELEMENT(inMenuItems->data);

		/* Only search displayed menu elements */
		if(garcon_menu_element_get_visible(item) &&
			!garcon_menu_element_get_no_display(item) &&
			garcon_menu_element_get_show_in_environment(item))
		{
			/* If menu element is a sub-menu step down into it and search it */
			if(GARCON_IS_MENU(item))
			{
				GList					*list=garcon_menu_get_elements(GARCON_MENU(item));

				_xfdashboard_applications_view_filter_items(self, list, outFoundItems);
			}
				/* If menu element is an item check if it matches filter text */
				else if(GARCON_IS_MENU_ITEM(item))
				{
					const gchar			*title, *description, *command;
					gchar				*lowerCaseText;
					const gchar			*foundPos;
					gboolean			isMatch=FALSE;

					/* Get title, description and command from menu element */
					title=garcon_menu_element_get_name(item);
					description=garcon_menu_element_get_comment(item);
					command=garcon_menu_item_get_command(GARCON_MENU_ITEM(item));

					/* Check if filter text exists in title or description at any position */
					if(title)
					{
						lowerCaseText=g_utf8_strdown(title, -1);
						if(g_strstr_len(lowerCaseText, -1, priv->filterText)) isMatch=TRUE;
						g_free(lowerCaseText);
					}

					if(!isMatch && description)
					{
						lowerCaseText=g_utf8_strdown(description, -1);
						if(g_strstr_len(lowerCaseText, -1, priv->filterText)) isMatch=TRUE;
						g_free(lowerCaseText);
					}

					/* Check if filter text occures in command at the beginning
					 * or directly after a path seperator
					 */
					if(!isMatch && command)
					{
						lowerCaseText=g_utf8_strdown(command, -1);
						foundPos=g_strstr_len(lowerCaseText, -1, priv->filterText);
						if(foundPos &&
							(foundPos==command || (*(foundPos--))=='/')) isMatch=TRUE;
						g_free(lowerCaseText);
					}

					/* If we found a match add menu element to list of found items */
					if(isMatch)
					{
						*outFoundItems=g_list_prepend(*outFoundItems, g_object_ref(G_OBJECT(item)));
					}
				}
		}
	}
}

/* Clear filtered items list */
void _xfdashboard_applications_view_clear_filtered_items(XfdashboardApplicationsView *self)
{
	g_return_if_fail(XFDASHBOARD_IS_APPLICATIONS_VIEW(self));

	XfdashboardApplicationsViewPrivate	*priv=XFDASHBOARD_APPLICATIONS_VIEW(self)->priv;

	/* Clear filtered items list */
	if(priv->filteredItems)
	{
		GList							*list;
		
		for(list=priv->filteredItems; list; list=list->next) g_object_unref(GARCON_MENU_ITEM(list->data));
		g_list_free(priv->filteredItems);
		priv->filteredItems=NULL;
	}
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

	/* If filter text is set filter all menu items */
	if(priv->filterText)
	{
		GarconMenu				*rootMenu;
		GList					*menuItems;

		/* Clear filtered items list */
		_xfdashboard_applications_view_clear_filtered_items(self);
		
		/* Filter all menu items */
		rootMenu=xfdashboard_get_application_menu();
		menuItems=garcon_menu_get_elements(rootMenu);

		_xfdashboard_applications_view_filter_items(self, menuItems, &priv->filteredItems);

		/* Set pointer to list of actors to create */
		items=priv->filteredItems;
	}
		else
		{
			/* If there is no active menu do nothing after removing item actors */
			if(priv->activeMenu==NULL) return;

			/* If menu shown is not root menu add "Go up ..." item first */
			if(priv->activeMenu!=xfdashboard_get_application_menu())
			{
				ClutterActor					*actor;

				actor=xfdashboard_application_icon_new_with_custom(GARCON_MENU_ELEMENT(garcon_menu_get_parent(priv->activeMenu)),
																	GTK_STOCK_GO_UP,
																	"Back",
																	"Go back to previous menu");

				if(priv->listMode==XFDASHBOARD_VIEW_LIST)
				{
					gchar						*newText, *title, *description;

					/* Get title and description from icon */
					g_object_get(G_OBJECT(actor),
									"custom-title", &title,
									"custom-description", &description,
									NULL);

					/* Setup actor for view */
					xfdashboard_button_set_style(XFDASHBOARD_BUTTON(actor), XFDASHBOARD_STYLE_BOTH);
					xfdashboard_button_set_icon_orientation(XFDASHBOARD_BUTTON(actor), XFDASHBOARD_ORIENTATION_LEFT);
					xfdashboard_button_set_sync_icon_size(XFDASHBOARD_BUTTON(actor), FALSE);

					/* Override text in icon */
					newText=g_strdup_printf("<b><big>%s</big></b>\n%s", title, description);
					xfdashboard_button_set_text(XFDASHBOARD_BUTTON(actor), newText);

					/* Release allocated resources */
					g_free(newText);
					g_free(title);
					g_free(description);
				}

				clutter_container_add_actor(CLUTTER_CONTAINER(self), actor);
				g_signal_connect(actor, "clicked", G_CALLBACK(_xfdashboard_applications_view_on_parent_menu_entry_clicked), self);
			}

			/* Set pointer to list of actors to create */
			items=garcon_menu_get_elements(priv->activeMenu);
		}

	/* Create item actors for new active menu */
	for(; items; items=items->next)
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
			actor=xfdashboard_application_icon_new_by_menu_item(item);

			if(priv->listMode==XFDASHBOARD_VIEW_LIST)
			{
				gchar						*newText;
				const gchar					*title, *description;

				/* Get title and description from menu element */
				title=garcon_menu_element_get_name(item);
				description=garcon_menu_element_get_comment(item);

				/* Setup actor for view */
				xfdashboard_button_set_style(XFDASHBOARD_BUTTON(actor), XFDASHBOARD_STYLE_BOTH);
				xfdashboard_button_set_icon_orientation(XFDASHBOARD_BUTTON(actor), XFDASHBOARD_ORIENTATION_LEFT);
				xfdashboard_button_set_sync_icon_size(XFDASHBOARD_BUTTON(actor), FALSE);

				/* Override text from menu element in icon */
				newText=g_strdup_printf("<b><big>%s</big></b>\n%s", title, description ? description : "");
				xfdashboard_button_set_text(XFDASHBOARD_BUTTON(actor), newText);

				/* Release allocated resources */
				g_free(newText);
			}
				else
				{
					/* Setup actor for view */
					xfdashboard_button_set_background_visibility(XFDASHBOARD_BUTTON(actor), FALSE);
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
ClutterLayoutManager* _xfdashboard_applications_view_get_layout_manager_for_list_mode(XfdashboardViewMode inListMode)
{
	ClutterLayoutManager				*layout=NULL;

	if(inListMode==XFDASHBOARD_VIEW_LIST)
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
			XfdashboardPolicy			vPolicy;
		
			xfdashboard_viewpad_get_scrollbar_policy(viewpad, NULL, &vPolicy);
			xfdashboard_viewpad_set_scrollbar_policy(viewpad, priv->oldHorizontalPolicy, vPolicy);
			priv->hasOldHorizontalPolicy=FALSE;
		}

		/* Check if we should set new policies */
		if(!inDoRestore &&
			priv->listMode==XFDASHBOARD_VIEW_ICON &&
			!priv->hasOldHorizontalPolicy)
		{
			XfdashboardPolicy			vPolicy;
		
			xfdashboard_viewpad_get_scrollbar_policy(viewpad, &priv->oldHorizontalPolicy, &vPolicy);
			xfdashboard_viewpad_set_scrollbar_policy(viewpad, XFDASHBOARD_POLICY_NEVER, vPolicy);
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
void _xfdashboard_applications_view_set_list_mode(XfdashboardApplicationsView *self, XfdashboardViewMode inListMode)
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
	if(self->priv->filterText) g_free(self->priv->filterText);
	self->priv->filterText=NULL;

	/* Reset scroll bar policy if necessary by setting view to list view.
	 * That's because of the dirty hack we need to do when switching to icon view
	 */
	_xfdashboard_applications_view_set_list_mode(self, XFDASHBOARD_VIEW_LIST);

	/* Clear filtered items list */
	_xfdashboard_applications_view_clear_filtered_items(self);

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

		case PROP_FILTER_TEXT:
			xfdashboard_applications_view_set_filter_text(self, g_value_get_string(inValue));
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

		case PROP_FILTER_TEXT:
			g_value_set_string(outValue, self->priv->filterText);
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
							XFDASHBOARD_TYPE_VIEW_MODE,
							XFDASHBOARD_VIEW_ICON,
							G_PARAM_READWRITE);

	XfdashboardApplicationsViewProperties[PROP_ACTIVE_MENU]=
		g_param_spec_object("active-menu",
							"Current active menu",
							"The active menu shown currently",
							GARCON_TYPE_MENU,
							G_PARAM_READWRITE | G_PARAM_CONSTRUCT);

	XfdashboardApplicationsViewProperties[PROP_FILTER_TEXT]=
		g_param_spec_string("filter-text",
							"Current filter text",
							"The text to filter all menu item",
							NULL,
							G_PARAM_READWRITE);

	g_object_class_install_properties(gobjectClass, PROP_LAST, XfdashboardApplicationsViewProperties);
}

/* Object initialization
 * Create private structure and set up default values
 */
static void xfdashboard_applications_view_init(XfdashboardApplicationsView *self)
{
	XfdashboardApplicationsViewPrivate	*priv;

	priv=self->priv=XFDASHBOARD_APPLICATIONS_VIEW_GET_PRIVATE(self);

	/* Set up default values */
	priv->activeMenu=NULL;
	priv->filterText=NULL;
	priv->filteredItems=NULL;
	priv->listMode=-1;
	priv->hasOldHorizontalPolicy=FALSE;

	/* Connect signals */
	g_signal_connect(self, "activated", G_CALLBACK(_xfdashboard_applications_view_on_activation_changed), GINT_TO_POINTER(FALSE));
	g_signal_connect(self, "deactivated", G_CALLBACK(_xfdashboard_applications_view_on_activation_changed), GINT_TO_POINTER(TRUE));
}

/* Implementation: Public API */

/* Create new actor */
ClutterActor* xfdashboard_applications_view_new()
{
	/* Create actor */
	return(g_object_new(XFDASHBOARD_TYPE_APPLICATIONS_VIEW,
						"view-name", "Applications",
						"list-mode", XFDASHBOARD_VIEW_LIST,
						NULL));
}

/* Get/set list mode */
XfdashboardViewMode xfdashboard_applications_view_get_list_mode(XfdashboardApplicationsView *self)
{
	g_return_val_if_fail(XFDASHBOARD_IS_APPLICATIONS_VIEW(self), XFDASHBOARD_VIEW_ICON);

	return(self->priv->listMode);
}

void xfdashboard_applications_view_set_list_mode(XfdashboardApplicationsView *self, XfdashboardViewMode inListMode)
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

/* Get/set filter text */
const gchar* xfdashboard_applications_view_get_filter_text(XfdashboardApplicationsView *self)
{
	g_return_val_if_fail(XFDASHBOARD_IS_APPLICATIONS_VIEW(self), NULL);

	return(self->priv->filterText);
}

void xfdashboard_applications_view_set_filter_text(XfdashboardApplicationsView *self, const gchar *inFilterText)
{
	g_return_if_fail(XFDASHBOARD_IS_APPLICATIONS_VIEW(self));

	XfdashboardApplicationsViewPrivate	*priv=XFDASHBOARD_APPLICATIONS_VIEW(self)->priv;
	gchar								*filterText=NULL;
	GarconMenu							*rootMenu=NULL;

	/* Get lower-case and stripped filter text if set */
	if(inFilterText)
	{
		gchar		*filterTextStripped;
		gchar		*filterTextLowerCase;

		/* Copy filter text for futher string manipulation */
		filterText=g_strdup(inFilterText);

		/* Strip white-spaces from beginning and end of filter text
		 * (This string _MUST_ not be free as function only moves
		 *  pointer and sets new NULL at end of string) */
		filterTextStripped=g_strstrip(filterText);

		/* Get lower-case of stripped filter text */
		filterTextLowerCase=g_utf8_strdown(filterTextStripped, -1);

		/* Release allocated memory */
		g_free(filterText);

		/* Set filter text to new stripped and lower-case filter text */
		filterText=filterTextLowerCase;
	}

	/* If length of filter text is zero handle it as a NULL-pointer */
	if(filterText && strlen(filterText)==0)
	{
		if(filterText) g_free(filterText);
		filterText=NULL;
	}

	/* Only change filter text if it changes */
	if(g_strcmp0(filterText, priv->filterText)==0) return;

	/* Set new filter text */
	if(priv->filterText) g_free(priv->filterText);
	
	if(filterText) priv->filterText=g_strdup(filterText);
		else priv->filterText=NULL;

	/* Release allocated memory */
	if(filterText) g_free(filterText);

	/* Set active menu back to root menu to get it display when search ended */
	rootMenu=xfdashboard_get_application_menu();
	if(priv->activeMenu!=rootMenu)
	{
		if(priv->activeMenu) g_object_unref(priv->activeMenu);
		priv->activeMenu=GARCON_MENU(g_object_ref((gpointer)rootMenu));
	}
	
	/* Update filter list and set up items to display */
	_xfdashboard_applications_view_refresh(self);
}
