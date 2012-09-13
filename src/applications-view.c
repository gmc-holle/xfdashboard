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

#include "applications-view.h"
#include "application-entry.h"
#include "fill-box-layout.h"
#include "main.h"

/* Define this class in GObject system */
G_DEFINE_TYPE(XfdashboardApplicationsView,
				xfdashboard_applications_view,
				XFDASHBOARD_TYPE_VIEW)

/* Private structure - access only by public API if needed */
#define XFDASHBOARD_APPLICATIONS_VIEW_GET_PRIVATE(obj) \
	(G_TYPE_INSTANCE_GET_PRIVATE((obj), XFDASHBOARD_TYPE_APPLICATIONS_VIEW, XfdashboardApplicationsViewPrivate))

struct _XfdashboardApplicationsViewPrivate
{
	/* Active menu */
	GarconMenu			*activeMenu;
};

/* Properties */
enum
{
	PROP_0,

	PROP_ACTIVE_MENU,
	
	PROP_LAST
};

static GParamSpec* XfdashboardApplicationsViewProperties[PROP_LAST]={ 0, };

/* IMPLEMENTATION: Private variables and methods */
void _set_active_menu(XfdashboardApplicationsView *self, const GarconMenu *inMenu);

/* "Go up ..." item was clicked */
void _on_parent_menu_entry_clicked(ClutterActor *inActor, gpointer inUserData)
{
	g_return_if_fail(XFDASHBOARD_IS_APPLICATION_MENU_ENTRY(inActor));
	g_return_if_fail(XFDASHBOARD_IS_APPLICATIONS_VIEW(inUserData));

	/* If sub-menu was clicked change into it otherwise start application
	 * of menu item and quit application
	 */
	XfdashboardApplicationsView		*self=XFDASHBOARD_APPLICATIONS_VIEW(inUserData);
	XfdashboardApplicationMenuEntry	*item=XFDASHBOARD_APPLICATION_MENU_ENTRY(inActor);
	GarconMenuElement				*element;

	element=(GarconMenuElement*)xfdashboard_application_menu_entry_get_menu_element(item);
	_set_active_menu(self, GARCON_MENU(element));
}

/* Menu item was clicked */
static gboolean _on_entry_clicked(ClutterActor *inActor, gpointer inUserData)
{
	g_return_val_if_fail(XFDASHBOARD_IS_APPLICATION_MENU_ENTRY(inActor), FALSE);
	g_return_val_if_fail(XFDASHBOARD_IS_APPLICATIONS_VIEW(inUserData), FALSE);

	/* If sub-menu was clicked change into it otherwise start application
	 * of menu item and quit application
	 */
	XfdashboardApplicationsView		*self=XFDASHBOARD_APPLICATIONS_VIEW(inUserData);
	XfdashboardApplicationMenuEntry	*item=XFDASHBOARD_APPLICATION_MENU_ENTRY(inActor);
	GarconMenuElement				*element;

	element=(GarconMenuElement*)xfdashboard_application_menu_entry_get_menu_element(item);
	if(xfdashboard_application_menu_entry_is_submenu(item))
	{
		_set_active_menu(self, GARCON_MENU(element));
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
void _set_active_menu(XfdashboardApplicationsView *self, const GarconMenu *inMenu)
{
	XfdashboardApplicationsViewPrivate	*priv=XFDASHBOARD_APPLICATIONS_VIEW(self)->priv;
	GList								*items;

	/* Do not anything if new active menu is the same as before */
	if(inMenu==priv->activeMenu) return;

	/* Remove all application entry actors and reset scroll bars */
	xfdashboard_view_remove_all(XFDASHBOARD_VIEW(self));
	xfdashboard_view_reset_scrollbars(XFDASHBOARD_VIEW(self));

	/* Set new active menu */
	priv->activeMenu=(GarconMenu*)inMenu;
	if(priv->activeMenu==NULL) return;

	/* If menu shown is not root menu add "Go up ..." item first */
	if(priv->activeMenu!=xfdashboard_getApplicationMenu())
	{
		ClutterActor					*actor;

		actor=xfdashboard_application_menu_entry_new_with_custom(GARCON_MENU_ELEMENT(garcon_menu_get_parent(priv->activeMenu)),
																	GTK_STOCK_GO_UP,
																	"Back",
																	"Go back to previous menu");
		clutter_container_add_actor(CLUTTER_CONTAINER(self), actor);
		g_signal_connect(actor, "clicked", G_CALLBACK(_on_parent_menu_entry_clicked), self);
	}

	/* Create application entry actors for new active menu */
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
			actor=xfdashboard_application_menu_entry_new_with_menu_item(item);
			clutter_container_add_actor(CLUTTER_CONTAINER(self), actor);
			g_signal_connect(actor, "clicked", G_CALLBACK(_on_entry_clicked), self);
		}
	}
}

/* IMPLEMENTATION: GObject */

/* Dispose this object */
static void xfdashboard_applications_view_dispose(GObject *inObject)
{
	XfdashboardApplicationsView		*self=XFDASHBOARD_APPLICATIONS_VIEW(inObject);

	_set_active_menu(self, NULL);

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
		case PROP_ACTIVE_MENU:
			_set_active_menu(self, g_value_get_object(inValue));
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
}

/* Implementation: Public API */

/* Create new object */
ClutterActor* xfdashboard_applications_view_new()
{
	ClutterLayoutManager	*layout;

	/* Create layout manager used in this view */
	layout=xfdashboard_fill_box_layout_new();
	xfdashboard_fill_box_layout_set_vertical(XFDASHBOARD_FILL_BOX_LAYOUT(layout), TRUE);
	xfdashboard_fill_box_layout_set_spacing(XFDASHBOARD_FILL_BOX_LAYOUT(layout), 1.0f);
	xfdashboard_fill_box_layout_set_homogenous(XFDASHBOARD_FILL_BOX_LAYOUT(layout), TRUE);
	
	/* Create object */
	return(g_object_new(XFDASHBOARD_TYPE_APPLICATIONS_VIEW,
						"view-name", "Applications",
						"layout-manager", layout,
						NULL));
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
	_set_active_menu(self, inMenu);
}
