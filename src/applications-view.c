/*
 * applications-view: A view showing all installed applications as menu
 * 
 * Copyright 2012-2013 Stephan Haller <nomad@froevel.de>
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

#include <glib/gi18n-lib.h>
#include <gtk/gtk.h>

#include "applications-view.h"
#include "utils.h"
#include "view.h"
#include "fill-box-layout.h"
#include "applications-menu-model.h"
#include "types.h"
#include "button.h"
#include "application.h"
#include "application-button.h"

/* Define this class in GObject system */
G_DEFINE_TYPE(XfdashboardApplicationsView,
				xfdashboard_applications_view,
				XFDASHBOARD_TYPE_VIEW)

/* Private structure - access only by public API if needed */
#define XFDASHBOARD_APPLICATIONS_VIEW_GET_PRIVATE(obj) \
	(G_TYPE_INSTANCE_GET_PRIVATE((obj), XFDASHBOARD_TYPE_APPLICATIONS_VIEW, XfdashboardApplicationsViewPrivate))

struct _XfdashboardApplicationsViewPrivate
{
	/* Properties related */

	/* Instance related */
	ClutterLayoutManager				*layout;
	XfdashboardApplicationsMenuModel	*apps;
	GarconMenuElement					*currentRootMenuElement;
};

/* Properties */
enum
{
	PROP_0,

	PROP_LAST
};

GParamSpec* XfdashboardApplicationsViewProperties[PROP_LAST]={ 0, };

/* IMPLEMENTATION: Private variables and methods */
#define ACTOR_USER_DATA_KEY			"xfdashboard-applications-view-user-data"

#define DEFAULT_VIEW_ICON			GTK_STOCK_HOME			// TODO: Replace by settings/theming object
#define DEFAULT_SPACING				4.0f					// TODO: Replace by settings/theming object
#define DEFAULT_MENU_ICON_SIZE		64						// TODO: Replace by settings/theming object

/* Filter of applications data model has changed */
void _xfdashboard_applications_view_on_parent_menu_clicked(XfdashboardApplicationsView *self, gpointer inUserData)
{
g_message("%s: self=%p (%s), user-data=%p (%s)", __func__, self, DEBUG_OBJECT_NAME(self), inUserData, DEBUG_OBJECT_NAME(inUserData));
	g_return_if_fail(XFDASHBOARD_IS_APPLICATIONS_VIEW(self));
	g_return_if_fail(XFDASHBOARD_IS_BUTTON(inUserData));

	XfdashboardApplicationsViewPrivate	*priv=self->priv;
	XfdashboardButton					*button=XFDASHBOARD_BUTTON(inUserData);
	GarconMenuElement					*element;

	/* Get associated menu element of button */
	element=GARCON_MENU_ELEMENT(g_object_get_data(G_OBJECT(button), ACTOR_USER_DATA_KEY));

	if(GARCON_IS_MENU(element))
	{
		priv->currentRootMenuElement=element;
		xfdashboard_applications_menu_model_filter_by_section(priv->apps, GARCON_MENU(element));
		xfdashboard_view_scroll_to(XFDASHBOARD_VIEW(self), -1, 0);
	}
}

void _xfdashboard_applications_view_on_item_clicked(XfdashboardApplicationsView *self, gpointer inUserData)
{
	g_return_if_fail(XFDASHBOARD_IS_APPLICATIONS_VIEW(self));
	g_return_if_fail(XFDASHBOARD_IS_APPLICATION_BUTTON(inUserData));

	XfdashboardApplicationsViewPrivate	*priv=self->priv;
	XfdashboardApplicationButton		*button=XFDASHBOARD_APPLICATION_BUTTON(inUserData);
	GarconMenuElement					*element;

	/* Get associated menu element of button */
	element=xfdashboard_application_button_get_menu_element(button);

	/* If clicked item is a menu set it as new parent one */
	if(GARCON_IS_MENU(element))
	{
		priv->currentRootMenuElement=element;
		xfdashboard_applications_menu_model_filter_by_section(priv->apps, GARCON_MENU(element));
		xfdashboard_view_scroll_to(XFDASHBOARD_VIEW(self), -1, 0);
	}
		/* If clicked item is a menu item execute command of menu item clicked
		 * and quit application
		 */
		else if(GARCON_IS_MENU_ITEM(element))
		{
			/* Launch application */
			if(xfdashboard_application_button_execute(button))
			{
				/* Launching application seems to be successfuly so quit application */
				xfdashboard_application_quit();
				return;
			}
		}
}

void _xfdashboard_applications_view_on_filter_changed(XfdashboardApplicationsView *self, gpointer inUserData)
{
	g_return_if_fail(XFDASHBOARD_IS_APPLICATIONS_VIEW(self));

	XfdashboardApplicationsViewPrivate	*priv=XFDASHBOARD_APPLICATIONS_VIEW(self)->priv;
	ClutterModelIter					*iterator;
	ClutterActor						*actor;
	GarconMenuElement					*menuElement=NULL;
	GarconMenu							*parentMenu=NULL;

	/* Destroy all children */
	clutter_actor_destroy_all_children(CLUTTER_ACTOR(self));

	/* Get parent menu */
	if(priv->currentRootMenuElement &&
		GARCON_IS_MENU(priv->currentRootMenuElement))
	{
		parentMenu=garcon_menu_get_parent(GARCON_MENU(priv->currentRootMenuElement));
	}

	/* If menu element to filter by is not the root menu element, add an "up ..." entry */
	if(parentMenu!=NULL)
	{
		gchar							*actorText=NULL;

		/* Create actor for menu element */
		actorText=g_strdup_printf("<b><big>%s</big></b>\n\n%s", _("Back"), _("Go back to previous menu"));
		actor=xfdashboard_button_new_full(GTK_STOCK_GO_UP, actorText);
		xfdashboard_button_set_icon_size(XFDASHBOARD_BUTTON(actor), DEFAULT_MENU_ICON_SIZE);
		xfdashboard_button_set_single_line_mode(XFDASHBOARD_BUTTON(actor), FALSE);
		xfdashboard_button_set_sync_icon_size(XFDASHBOARD_BUTTON(actor), FALSE);
		g_object_set_data_full(G_OBJECT(actor), ACTOR_USER_DATA_KEY, g_object_ref(parentMenu), (GDestroyNotify)g_object_unref);
		clutter_actor_show(actor);
		g_signal_connect_swapped(actor, "clicked", G_CALLBACK(_xfdashboard_applications_view_on_parent_menu_clicked), self);
		g_free(actorText);

		/* Add actor to view */
		clutter_box_layout_pack(CLUTTER_BOX_LAYOUT(priv->layout),
									actor,
									TRUE,
									TRUE,
									TRUE,
									CLUTTER_BOX_ALIGNMENT_START,
									CLUTTER_BOX_ALIGNMENT_START);
	}

	/* Iterate through (filtered) data model and create actor for each entry */
	iterator=clutter_model_get_first_iter(CLUTTER_MODEL(priv->apps));
	if(iterator && CLUTTER_IS_MODEL_ITER(iterator))
	{
		while(!clutter_model_iter_is_last(iterator))
		{
			/* Get data from model */
			clutter_model_iter_get(iterator,
									XFDASHBOARD_APPLICATIONS_MENU_MODEL_COLUMN_MENU_ELEMENT, &menuElement,
									-1);

			if(menuElement)
			{
				/* Create actor for menu element */
				actor=xfdashboard_application_button_new_from_menu(menuElement);
				xfdashboard_application_button_set_show_description(XFDASHBOARD_APPLICATION_BUTTON(actor), TRUE);
				xfdashboard_button_set_sync_icon_size(XFDASHBOARD_BUTTON(actor), FALSE);
				clutter_actor_show(actor);
				g_signal_connect_swapped(actor, "clicked", G_CALLBACK(_xfdashboard_applications_view_on_item_clicked), self);

				/* Add actor to view */
				clutter_box_layout_pack(CLUTTER_BOX_LAYOUT(priv->layout),
											actor,
											TRUE,
											TRUE,
											TRUE,
											CLUTTER_BOX_ALIGNMENT_START,
											CLUTTER_BOX_ALIGNMENT_START);
			}

			/* Release allocated resources */
			if(menuElement)
			{
				g_object_unref(menuElement);
				menuElement=NULL;
			}

			/* Go to next entry in model */
			iterator=clutter_model_iter_next(iterator);
		}
		g_object_unref(iterator);
	}
}

/* IMPLEMENTATION: GObject */

/* Dispose this object */
void _xfdashboard_applications_view_dispose(GObject *inObject)
{
	XfdashboardApplicationsView			*self=XFDASHBOARD_APPLICATIONS_VIEW(inObject);
	XfdashboardApplicationsViewPrivate	*priv=self->priv;

	/* Release allocated resources */
	if(priv->layout) priv->layout=NULL;

	if(priv->apps)
	{
		g_object_unref(priv->apps);
		priv->apps=NULL;
	}

	/* Call parent's class dispose method */
	G_OBJECT_CLASS(xfdashboard_applications_view_parent_class)->dispose(inObject);
}

/* Class initialization
 * Override functions in parent classes and define properties
 * and signals
 */
void xfdashboard_applications_view_class_init(XfdashboardApplicationsViewClass *klass)
{
	GObjectClass			*gobjectClass=G_OBJECT_CLASS(klass);

	/* Override functions */
	gobjectClass->dispose=_xfdashboard_applications_view_dispose;

	/* Set up private structure */
	g_type_class_add_private(klass, sizeof(XfdashboardApplicationsViewPrivate));
}

/* Object initialization
 * Create private structure and set up default values
 */
void xfdashboard_applications_view_init(XfdashboardApplicationsView *self)
{
	XfdashboardApplicationsViewPrivate	*priv;

	self->priv=priv=XFDASHBOARD_APPLICATIONS_VIEW_GET_PRIVATE(self);

	/* Set up default values */
	priv->apps=XFDASHBOARD_APPLICATIONS_MENU_MODEL(xfdashboard_applications_menu_model_new());
	priv->currentRootMenuElement=NULL;

	/* Set up view */
	xfdashboard_view_set_internal_name(XFDASHBOARD_VIEW(self), "applications");
	xfdashboard_view_set_name(XFDASHBOARD_VIEW(self), _("Applications"));
	xfdashboard_view_set_icon(XFDASHBOARD_VIEW(self), DEFAULT_VIEW_ICON);

	/* Set up actor */
	xfdashboard_view_set_fit_mode(XFDASHBOARD_VIEW(self), XFDASHBOARD_FIT_MODE_HORIZONTAL);

	xfdashboard_applications_menu_model_filter_by_section(priv->apps, GARCON_MENU(priv->currentRootMenuElement));
	clutter_model_set_sorting_column(CLUTTER_MODEL(priv->apps), XFDASHBOARD_APPLICATIONS_MENU_MODEL_COLUMN_TITLE);
	_xfdashboard_applications_view_on_filter_changed(self, priv->apps);

	priv->layout=clutter_box_layout_new();
	clutter_box_layout_set_orientation(CLUTTER_BOX_LAYOUT(priv->layout), CLUTTER_ORIENTATION_VERTICAL);
	clutter_box_layout_set_spacing(CLUTTER_BOX_LAYOUT(priv->layout), DEFAULT_SPACING);
	clutter_actor_set_layout_manager(CLUTTER_ACTOR(self), priv->layout);

	/* Connect signals */
	g_signal_connect_swapped(priv->apps, "filter-changed", G_CALLBACK(_xfdashboard_applications_view_on_filter_changed), self);
}
