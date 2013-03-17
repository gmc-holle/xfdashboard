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

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "enums.h"
#include "applications-view.h"
#include "application-icon.h"
#include "fill-box-layout.h"
#include "viewpad.h"
#include "common.h"
#include "drag-action.h"

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
	gchar					*filter;
	ClutterModel			*model;
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
enum
{
	COLUMN_ACTOR,
	COLUMN_MENU_ITEM,
	COLUMN_PARENT_MENU
};

void _xfdashboard_applications_view_set_active_menu(XfdashboardApplicationsView *self, const GarconMenu *inMenu);

/* Filter model by name */
gboolean _xfdashboard_applications_view_filter_by_name(ClutterModel *inModel,
														ClutterModelIter *inIter,
														gpointer inUserData)
{
	g_return_if_fail(XFDASHBOARD_IS_APPLICATIONS_VIEW(inUserData));

	XfdashboardApplicationsView			*self=XFDASHBOARD_APPLICATIONS_VIEW(inUserData);
	XfdashboardApplicationsViewPrivate	*priv=self->priv;
	GarconMenuElement					*element;
	const gchar							*checkText, *foundPos;
	gchar								*lowerCaseText;
	gboolean							isMatch=FALSE;

	/* Get element from model */
	clutter_model_iter_get(inIter, COLUMN_MENU_ITEM, &element, -1);

	/* If menu element is an item check if it matches filter */
	if(GARCON_IS_MENU_ITEM(element))
	{
		/* Check if title, description or command from menu element matches filter */
		if(!isMatch)
		{
			checkText=garcon_menu_element_get_name(element);
			if(checkText)
			{
				lowerCaseText=g_utf8_strdown(checkText, -1);
				if(g_strstr_len(lowerCaseText, -1, priv->filter)) isMatch=TRUE;
				g_free(lowerCaseText);
			}
		}

		if(!isMatch)
		{
			checkText=garcon_menu_element_get_comment(element);
			if(!isMatch && checkText)
			{
				lowerCaseText=g_utf8_strdown(checkText, -1);
				if(g_strstr_len(lowerCaseText, -1, priv->filter)) isMatch=TRUE;
				g_free(lowerCaseText);
			}
		}

		if(!isMatch)
		{
			checkText=garcon_menu_item_get_command(GARCON_MENU_ITEM(element));
			if(!isMatch && checkText)
			{
				lowerCaseText=g_utf8_strdown(checkText, -1);
				if(foundPos &&
					(foundPos==checkText || (*(foundPos--))=='/')) isMatch=TRUE;
				g_free(lowerCaseText);
			}
		}
	}

	/* Release allocated resources */
	if(element) g_object_unref(element);

	return(isMatch);
}

/* Filter model by menu */
gboolean _xfdashboard_applications_view_filter_by_menu(ClutterModel *inModel,
														ClutterModelIter *inIter,
														gpointer inUserData)
{
	g_return_if_fail(XFDASHBOARD_IS_APPLICATIONS_VIEW(inUserData));

	XfdashboardApplicationsView			*self=XFDASHBOARD_APPLICATIONS_VIEW(inUserData);
	XfdashboardApplicationsViewPrivate	*priv=self->priv;
	GarconMenu							*parentMenu;
	gboolean							isMatch=FALSE;
	GarconMenuElement					*checkMenu;

	/* Get element from model */
	clutter_model_iter_get(inIter, COLUMN_PARENT_MENU, &parentMenu, -1);

	/* Check if parent menu of element matches filter */
	if((priv->activeMenu && parentMenu==priv->activeMenu) ||
		(!priv->activeMenu && parentMenu==xfdashboard_get_application_menu())) isMatch=TRUE;

	/* Release allocated resources */
	if(parentMenu) g_object_unref(parentMenu);

	return(isMatch);
}

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

/* Drag of an icon begins */
void _xfdashboard_applications_view_on_icon_drag_begin(ClutterDragAction *inAction,
														ClutterActor *inActor,
														gfloat inStageX,
														gfloat inStageY,
														ClutterModifierType inModifiers,
														gpointer inUserData)
{
	g_return_if_fail(CLUTTER_IS_DRAG_ACTION(inAction));
	g_return_if_fail(XFDASHBOARD_IS_APPLICATION_ICON(inActor));
	g_return_if_fail(XFDASHBOARD_IS_APPLICATIONS_VIEW(inUserData));

	XfdashboardApplicationsView		*self=XFDASHBOARD_APPLICATIONS_VIEW(inUserData);
	ClutterActor					*dragHandle;
	ClutterStage					*stage;

	/* Prevent signal "clicked" from being emitted on dragged icon */
	g_signal_handlers_block_by_func(inActor, _xfdashboard_applications_view_on_entry_clicked, self);

	/* Get stage */
	stage=CLUTTER_STAGE(clutter_actor_get_stage(inActor));

	/* Create a copy of application icon just showing the icon for drag handle */
	dragHandle=xfdashboard_application_icon_new_copy(XFDASHBOARD_APPLICATION_ICON(inActor));
	xfdashboard_button_set_style(XFDASHBOARD_BUTTON(dragHandle), XFDASHBOARD_STYLE_ICON);
	clutter_actor_set_position(dragHandle, inStageX, inStageY);
	clutter_container_add_actor(CLUTTER_CONTAINER(stage), dragHandle);

	clutter_drag_action_set_drag_handle(inAction, dragHandle);
}

/* Drag of an icon ends */
void _xfdashboard_applications_view_on_icon_drag_end(ClutterDragAction *inAction,
														ClutterActor *inActor,
														gfloat inStageX,
														gfloat inStageY,
														ClutterModifierType inModifiers,
														gpointer inUserData)
{
	g_return_if_fail(CLUTTER_IS_DRAG_ACTION(inAction));
	g_return_if_fail(XFDASHBOARD_IS_APPLICATION_ICON(inActor));
	g_return_if_fail(XFDASHBOARD_IS_APPLICATIONS_VIEW(inUserData));

	XfdashboardApplicationsView		*self=XFDASHBOARD_APPLICATIONS_VIEW(inUserData);
	ClutterActor					*dragHandle=NULL;

	/* Destroy application icon used as drag handle */
	dragHandle=clutter_drag_action_get_drag_handle(inAction);
	clutter_drag_action_set_drag_handle(inAction, NULL);
	clutter_actor_destroy(dragHandle);

	/* Allow signal "clicked" from being emitted again */
	g_signal_handlers_unblock_by_func(inActor, _xfdashboard_applications_view_on_entry_clicked, inUserData);
}

/* Refresh view on filtered model */
void _xfdashboard_applications_view_on_filter_changed(XfdashboardApplicationsView *self, gpointer inUserData)
{
	g_return_if_fail(XFDASHBOARD_IS_APPLICATIONS_VIEW(self));

	XfdashboardApplicationsViewPrivate	*priv=XFDASHBOARD_APPLICATIONS_VIEW(self)->priv;
	ClutterModelIter					*iter;

	/* Remove all item actors and reset scroll bars */
	xfdashboard_view_hide_all(XFDASHBOARD_VIEW(self));
	xfdashboard_view_reset_scrollbars(XFDASHBOARD_VIEW(self));

	/* Create item actors for current (filtered) items */
	iter=clutter_model_get_first_iter(priv->model);
	while(iter && !clutter_model_iter_is_last(iter))
	{
		ClutterActor					*actor;

		/* Get menu element from model */
		clutter_model_iter_get(iter, COLUMN_ACTOR, &actor, -1);

		/* Update actor depending on list mode */
		if(priv->listMode==XFDASHBOARD_VIEW_LIST)
		{
			xfdashboard_button_set_background_visibility(XFDASHBOARD_BUTTON(actor), TRUE);
			xfdashboard_button_set_style(XFDASHBOARD_BUTTON(actor), XFDASHBOARD_STYLE_BOTH);
			xfdashboard_button_set_icon_orientation(XFDASHBOARD_BUTTON(actor), XFDASHBOARD_ORIENTATION_LEFT);
			xfdashboard_button_set_sync_icon_size(XFDASHBOARD_BUTTON(actor), FALSE);
		}
			else
			{
				xfdashboard_button_set_background_visibility(XFDASHBOARD_BUTTON(actor), FALSE);
				xfdashboard_button_set_style(XFDASHBOARD_BUTTON(actor), XFDASHBOARD_STYLE_ICON);
				xfdashboard_button_set_icon_orientation(XFDASHBOARD_BUTTON(actor), XFDASHBOARD_ORIENTATION_LEFT);
				xfdashboard_button_set_sync_icon_size(XFDASHBOARD_BUTTON(actor), TRUE);
			}

		/* Update label of actor */
		if(XFDASHBOARD_IS_APPLICATION_ICON(actor))
		{
			xfdashboard_application_icon_set_show_secondary(XFDASHBOARD_APPLICATION_ICON(actor),
																priv->listMode==XFDASHBOARD_VIEW_LIST ? TRUE : FALSE);
		}

		/* Show actor */
		clutter_actor_show(actor);

		/* Release allocated resources */
		if(actor) g_object_unref(actor);

		/* Go to next item in model */
		iter=clutter_model_iter_next(iter);
	}
	g_object_unref(iter);
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
	if(priv->activeMenu)
	{
		g_object_unref(priv->activeMenu);
		priv->activeMenu=NULL;
	}

	if(inMenu) priv->activeMenu=GARCON_MENU(g_object_ref((gpointer)inMenu));

	/* Filter model by new active menu */
	clutter_model_set_filter(priv->model, _xfdashboard_applications_view_filter_by_menu, self, NULL);
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

	/* Refresh view by telling that filter changed */
	_xfdashboard_applications_view_on_filter_changed(self, priv->model);
}

/* Populate model */
void _xfdashboard_applications_view_populate_model_with_menu(XfdashboardApplicationsView *self, GarconMenu *inMenu, GarconMenu *inParentMenu)
{
	g_return_if_fail(XFDASHBOARD_IS_APPLICATIONS_VIEW(self));
	g_return_if_fail(GARCON_IS_MENU(inMenu));
	g_return_if_fail(!inParentMenu || GARCON_IS_MENU(inParentMenu));

	XfdashboardApplicationsViewPrivate		*priv=self->priv;
	GList									*items, *iter;
	GarconMenuElement						*element;
	ClutterActor							*actor;
	ClutterAction							*dragAction;

	/* First of all add "Go up ..." item for menu if parent menu is available */
	if(inParentMenu)
	{
		actor=xfdashboard_application_icon_new_with_custom(GARCON_MENU_ELEMENT(inParentMenu),
															GTK_STOCK_GO_UP,
															"Back",
															"Go back to previous menu");
		xfdashboard_application_icon_set_show_secondary(XFDASHBOARD_APPLICATION_ICON(actor),
															priv->listMode==XFDASHBOARD_VIEW_LIST ? TRUE : FALSE);
		g_signal_connect(actor, "clicked", G_CALLBACK(_xfdashboard_applications_view_on_parent_menu_entry_clicked), self);

		clutter_model_append(priv->model, COLUMN_ACTOR, actor, COLUMN_MENU_ITEM, NULL, COLUMN_PARENT_MENU, inMenu, -1);
		clutter_container_add_actor(CLUTTER_CONTAINER(self), actor);
	}

	/* Iterate through items and add to model */
	items=garcon_menu_get_elements(inMenu);
	for(iter=items; iter; iter=iter->next)
	{
		element=GARCON_MENU_ELEMENT(iter->data);

		/* Create actor and connect signals */
		actor=xfdashboard_application_icon_new_by_menu_item(element);
		xfdashboard_application_icon_set_show_secondary(XFDASHBOARD_APPLICATION_ICON(actor),
															priv->listMode==XFDASHBOARD_VIEW_LIST ? TRUE : FALSE);
		g_signal_connect(actor, "clicked", G_CALLBACK(_xfdashboard_applications_view_on_entry_clicked), self);
		clutter_container_add_actor(CLUTTER_CONTAINER(self), actor);

		/* Set up drag action for actor */
		dragAction=xfdashboard_drag_action_new(CLUTTER_ACTOR(self));
		clutter_drag_action_set_drag_threshold(CLUTTER_DRAG_ACTION(dragAction), -1, -1);
		clutter_actor_add_action(actor, dragAction);
		g_signal_connect(dragAction, "drag-begin", G_CALLBACK(_xfdashboard_applications_view_on_icon_drag_begin), self);
		g_signal_connect(dragAction, "drag-end", G_CALLBACK(_xfdashboard_applications_view_on_icon_drag_end), self);

		/* If menu element is a menu call ourselve recursively */
		if(GARCON_IS_MENU(element))
		{
			clutter_model_append(priv->model, COLUMN_ACTOR, actor, COLUMN_MENU_ITEM, element, COLUMN_PARENT_MENU, inMenu, -1);
			_xfdashboard_applications_view_populate_model_with_menu(self, GARCON_MENU(element), inMenu);
		}
			/* Otherwise check if element is menu item and add to model */
			else if(GARCON_IS_MENU_ITEM(element))
			{
				clutter_model_append(priv->model, COLUMN_ACTOR, actor, COLUMN_MENU_ITEM, element, COLUMN_PARENT_MENU, inMenu, -1);
			}
	}
	g_list_free(items);
}

void _xfdashboard_applications_view_populate_model(XfdashboardApplicationsView *self)
{
	g_return_if_fail(XFDASHBOARD_IS_APPLICATIONS_VIEW(self));

	XfdashboardApplicationsViewPrivate		*priv=self->priv;
	ClutterActor							*actor;

	/* If available, destroy current model */
	if(priv->model)
	{
		g_object_unref(priv->model);
		priv->model=NULL;
	}

	/* Create new model and connect signals */
	priv->model=clutter_list_model_new(3,
										XFDASHBOARD_TYPE_APPLICATION_ICON, "actor",	/* COLUMN_ACTOR */
										GARCON_TYPE_MENU_ELEMENT, "menu-element",	/* COLUMN_MENU_ITEM */
										GARCON_TYPE_MENU, "parent-menu");			/* COLUMN_PARENT_MENU */
	g_signal_connect_swapped(priv->model, "filter-changed", G_CALLBACK(_xfdashboard_applications_view_on_filter_changed), self);

	/* Fill model with application data */
	_xfdashboard_applications_view_populate_model_with_menu(self, xfdashboard_get_application_menu(), NULL);
}

/* IMPLEMENTATION: GObject */

/* Dispose this object */
static void xfdashboard_applications_view_dispose(GObject *inObject)
{
	XfdashboardApplicationsView		*self=XFDASHBOARD_APPLICATIONS_VIEW(inObject);

	/* Release allocated resources */
	if(self->priv->filter)
	{
		g_free(self->priv->filter);
		self->priv->filter=NULL;
	}

	if(self->priv->model)
	{
		g_object_unref(self->priv->model);
		self->priv->model=NULL;
	}

	/* Reset scroll bar policy if necessary by setting view to list view.
	 * That's because of the dirty hack we need to do when switching to icon view
	 */
	_xfdashboard_applications_view_set_list_mode(self, XFDASHBOARD_VIEW_LIST);

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
			g_value_set_string(outValue, self->priv->filter);
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
	priv->filter=NULL;
	priv->model=NULL;
	priv->listMode=-1;
	priv->hasOldHorizontalPolicy=FALSE;

	/* Create model */
	_xfdashboard_applications_view_populate_model(self);

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

	return(self->priv->filter);
}

void xfdashboard_applications_view_set_filter_text(XfdashboardApplicationsView *self, const gchar *inFilterText)
{
	g_return_if_fail(XFDASHBOARD_IS_APPLICATIONS_VIEW(self));

	XfdashboardApplicationsViewPrivate	*priv=XFDASHBOARD_APPLICATIONS_VIEW(self)->priv;
	gchar								*filter=NULL;

	/* Get lower-case and stripped filter text if set */
	if(inFilterText)
	{
		gchar		*filterStripped;
		gchar		*filterLowerCase;

		/* Copy filter text for futher string manipulation */
		filter=g_strdup(inFilterText);

		/* Strip white-spaces from beginning and end of filter text
		 * (This string _MUST_ not be free because the function only
		 *  moves pointer and sets new NULL at end of string) */
		filterStripped=g_strstrip(filter);

		/* Get lower-case of stripped filter text */
		filterLowerCase=g_utf8_strdown(filterStripped, -1);

		/* Release allocated memory */
		g_free(filter);

		/* Set filter text to new stripped and lower-case filter text */
		filter=filterLowerCase;
	}

	/* If length of filter text is zero handle it as a NULL-pointer */
	if(filter && strlen(filter)==0)
	{
		if(filter) g_free(filter);
		filter=NULL;
	}

	/* Only change filter text if it changes */
	if(g_strcmp0(filter, priv->filter)!=0)
	{
		/* Set active menu back to root menu to get it displayed when search ended */
		if(priv->activeMenu)
		{
			g_object_unref(priv->activeMenu);
			priv->activeMenu=NULL;
		}

		/* Set new filter text and filter function depending on
		 * length of filter text
		 */
		if(priv->filter) g_free(priv->filter);
		
		if(filter)
		{
			priv->filter=g_strdup(filter);
			clutter_model_set_filter(priv->model, _xfdashboard_applications_view_filter_by_name, self, NULL);
		}
			else
			{
				priv->filter=NULL;
				clutter_model_set_filter(priv->model, _xfdashboard_applications_view_filter_by_menu, self, NULL);
			}
	}

	/* Release allocated memory */
	if(filter) g_free(filter);
}
