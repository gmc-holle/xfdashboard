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
#include "applications-menu-model.h"
#include "types.h"
#include "button.h"
#include "application.h"
#include "application-button.h"
#include "enums.h"

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
	XfdashboardViewMode					viewMode;

	/* Instance related */
	ClutterLayoutManager				*layout;
	XfdashboardApplicationsMenuModel	*apps;
	GarconMenuElement					*currentRootMenuElement;
	XfdashboardApplicationButton		*appButton;
};

/* Properties */
enum
{
	PROP_0,

	PROP_VIEW_MODE,

	PROP_LAST
};

GParamSpec* XfdashboardApplicationsViewProperties[PROP_LAST]={ 0, };

/* IMPLEMENTATION: Private variables and methods */
#define ACTOR_USER_DATA_KEY			"xfdashboard-applications-view-user-data"

#define DEFAULT_VIEW_ICON			GTK_STOCK_HOME				// TODO: Replace by settings/theming object
#define DEFAULT_VIEW_MODE			XFDASHBOARD_VIEW_MODE_LIST	// TODO: Replace by settings/theming object
#define DEFAULT_SPACING				4.0f						// TODO: Replace by settings/theming object
#define DEFAULT_MENU_ICON_SIZE		64							// TODO: Replace by settings/theming object
#define DEFAULT_PARENT_MENU_ICON	GTK_STOCK_GO_UP				// TODO: Replace by settings/theming object

/* Update style of all child actors */
static void _xfdashboard_applications_view_add_button_for_list_mode(XfdashboardApplicationsView *self,
																	XfdashboardButton *inButton)
{
	g_return_if_fail(XFDASHBOARD_IS_APPLICATIONS_VIEW(self));
	g_return_if_fail(XFDASHBOARD_IS_BUTTON(inButton));

	XfdashboardApplicationsViewPrivate	*priv=self->priv;
	const gchar							*actorFormat;
	gchar								*actorText;

	/* If button is a real application button set it up */
	if(XFDASHBOARD_IS_APPLICATION_BUTTON(inButton))
	{
		xfdashboard_application_button_set_show_description(XFDASHBOARD_APPLICATION_BUTTON(inButton), TRUE);
	}
		/* Otherwise it is a normal button for "go back" */
		else
		{
			/* Get text to set and format to use in "parent menu" button */
			actorFormat=xfdashboard_application_button_get_format_title_description(priv->appButton);
			actorText=g_strdup_printf(actorFormat, _("Back"), _("Go back to previous menu"));
			xfdashboard_button_set_text(inButton, actorText);
			g_free(actorText);
		}

	xfdashboard_button_set_style(inButton, XFDASHBOARD_STYLE_BOTH);
	xfdashboard_button_set_icon_size(inButton, DEFAULT_MENU_ICON_SIZE);
	xfdashboard_button_set_single_line_mode(inButton, FALSE);
	xfdashboard_button_set_sync_icon_size(inButton, FALSE);
	xfdashboard_button_set_icon_orientation(inButton, XFDASHBOARD_ORIENTATION_LEFT);
	xfdashboard_button_set_text_justification(inButton, PANGO_ALIGN_LEFT);

	/* Add to view and layout */
	clutter_actor_set_x_expand(CLUTTER_ACTOR(inButton), TRUE);
	clutter_actor_set_y_expand(CLUTTER_ACTOR(inButton), TRUE);
	clutter_actor_add_child(CLUTTER_ACTOR(self), CLUTTER_ACTOR(inButton));
}

static void _xfdashboard_applications_view_add_button_for_icon_mode(XfdashboardApplicationsView *self,
																	XfdashboardButton *inButton)
{
	g_return_if_fail(XFDASHBOARD_IS_APPLICATIONS_VIEW(self));
	g_return_if_fail(XFDASHBOARD_IS_BUTTON(inButton));

	XfdashboardApplicationsViewPrivate	*priv=self->priv;
	const gchar							*actorFormat;
	gchar								*actorText;

	/* If button is a real application button set it up */
	if(XFDASHBOARD_IS_APPLICATION_BUTTON(inButton))
	{
		xfdashboard_application_button_set_show_description(XFDASHBOARD_APPLICATION_BUTTON(inButton), FALSE);
	}
		/* Otherwise it is a normal button for "go back" */
		else
		{
			/* Get text to set and format to use in "parent menu" button */
			actorFormat=xfdashboard_application_button_get_format_title_only(priv->appButton);
			actorText=g_strdup_printf(actorFormat, _("Back"));
			xfdashboard_button_set_text(inButton, actorText);
			g_free(actorText);
		}

	xfdashboard_button_set_icon_size(inButton, DEFAULT_MENU_ICON_SIZE);
	xfdashboard_button_set_single_line_mode(inButton, FALSE);
	xfdashboard_button_set_sync_icon_size(inButton, FALSE);
	xfdashboard_button_set_icon_orientation(inButton, XFDASHBOARD_ORIENTATION_TOP);
	xfdashboard_button_set_text_justification(inButton, PANGO_ALIGN_CENTER);

	/* Add to view and layout */
	clutter_actor_set_x_expand(CLUTTER_ACTOR(inButton), TRUE);
	clutter_actor_set_y_expand(CLUTTER_ACTOR(inButton), TRUE);
	clutter_actor_add_child(CLUTTER_ACTOR(self), CLUTTER_ACTOR(inButton));
}

/* Filter of applications data model has changed */
static void _xfdashboard_applications_view_on_parent_menu_clicked(XfdashboardApplicationsView *self, gpointer inUserData)
{
	g_return_if_fail(XFDASHBOARD_IS_APPLICATIONS_VIEW(self));

	XfdashboardApplicationsViewPrivate	*priv=self->priv;
	GarconMenuElement					*element;

	/* Get associated menu element of button */
	if(priv->currentRootMenuElement &&
		GARCON_IS_MENU(priv->currentRootMenuElement))
	{
		element=GARCON_MENU_ELEMENT(garcon_menu_get_parent(GARCON_MENU(priv->currentRootMenuElement)));

		priv->currentRootMenuElement=element;
		xfdashboard_applications_menu_model_filter_by_section(priv->apps, GARCON_MENU(element));
		xfdashboard_view_scroll_to(XFDASHBOARD_VIEW(self), -1, 0);
	}
}

static void _xfdashboard_applications_view_on_item_clicked(XfdashboardApplicationsView *self, gpointer inUserData)
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

static void _xfdashboard_applications_view_on_filter_changed(XfdashboardApplicationsView *self, gpointer inUserData)
{
	g_return_if_fail(XFDASHBOARD_IS_APPLICATIONS_VIEW(self));

	XfdashboardApplicationsViewPrivate	*priv=XFDASHBOARD_APPLICATIONS_VIEW(self)->priv;
	ClutterModelIter					*iterator;
	ClutterActor						*actor;
	GarconMenuElement					*menuElement=NULL;
	GarconMenu							*parentMenu=NULL;

	/* Destroy all children */
	clutter_actor_destroy_all_children(CLUTTER_ACTOR(self));
	clutter_layout_manager_layout_changed(priv->layout);

	/* Get parent menu */
	if(priv->currentRootMenuElement &&
		GARCON_IS_MENU(priv->currentRootMenuElement))
	{
		parentMenu=garcon_menu_get_parent(GARCON_MENU(priv->currentRootMenuElement));
	}

	/* If menu element to filter by is not the root menu element, add an "up ..." entry */
	if(parentMenu!=NULL)
	{
		/* Create and adjust of "parent menu" button to application buttons */
		actor=xfdashboard_button_new_with_icon(DEFAULT_PARENT_MENU_ICON);
		if(priv->viewMode==XFDASHBOARD_VIEW_MODE_LIST)
		{
			_xfdashboard_applications_view_add_button_for_list_mode(self, XFDASHBOARD_BUTTON(actor));
		}
			else
			{
				_xfdashboard_applications_view_add_button_for_icon_mode(self, XFDASHBOARD_BUTTON(actor));
			}
		clutter_actor_show(actor);
		g_signal_connect_swapped(actor, "clicked", G_CALLBACK(_xfdashboard_applications_view_on_parent_menu_clicked), self);
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

			if(!menuElement) continue;

			/* Create actor for menu element */
			actor=xfdashboard_application_button_new_from_menu(menuElement);
			if(priv->viewMode==XFDASHBOARD_VIEW_MODE_LIST)
			{
				_xfdashboard_applications_view_add_button_for_list_mode(self, XFDASHBOARD_BUTTON(actor));
			}
				else
				{
					_xfdashboard_applications_view_add_button_for_icon_mode(self, XFDASHBOARD_BUTTON(actor));
				}
			clutter_actor_show(actor);
			g_signal_connect_swapped(actor, "clicked", G_CALLBACK(_xfdashboard_applications_view_on_item_clicked), self);

			/* Release allocated resources */
			g_object_unref(menuElement);
			menuElement=NULL;

			/* Go to next entry in model */
			iterator=clutter_model_iter_next(iterator);
		}
		g_object_unref(iterator);
	}
}

/* IMPLEMENTATION: GObject */

/* Dispose this object */
static void _xfdashboard_applications_view_dispose(GObject *inObject)
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

	if(priv->appButton)
	{
		clutter_actor_destroy(CLUTTER_ACTOR(priv->appButton));
		priv->appButton=NULL;
	}

	/* Call parent's class dispose method */
	G_OBJECT_CLASS(xfdashboard_applications_view_parent_class)->dispose(inObject);
}

/* Set/get properties */
static void _xfdashboard_applications_view_set_property(GObject *inObject,
														guint inPropID,
														const GValue *inValue,
														GParamSpec *inSpec)
{
	XfdashboardApplicationsView				*self=XFDASHBOARD_APPLICATIONS_VIEW(inObject);

	switch(inPropID)
	{
		case PROP_VIEW_MODE:
			xfdashboard_applications_view_set_view_mode(self, (XfdashboardViewMode)g_value_get_enum(inValue));
			break;

		default:
			G_OBJECT_WARN_INVALID_PROPERTY_ID(inObject, inPropID, inSpec);
			break;
	}
}

static void _xfdashboard_applications_view_get_property(GObject *inObject,
														guint inPropID,
														GValue *outValue,
														GParamSpec *inSpec)
{
	XfdashboardApplicationsView				*self=XFDASHBOARD_APPLICATIONS_VIEW(inObject);
	XfdashboardApplicationsViewPrivate		*priv=self->priv;

	switch(inPropID)
	{
		case PROP_VIEW_MODE:
			g_value_set_enum(outValue, priv->viewMode);
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
void xfdashboard_applications_view_class_init(XfdashboardApplicationsViewClass *klass)
{
	GObjectClass			*gobjectClass=G_OBJECT_CLASS(klass);

	/* Override functions */
	gobjectClass->dispose=_xfdashboard_applications_view_dispose;
	gobjectClass->set_property=_xfdashboard_applications_view_set_property;
	gobjectClass->get_property=_xfdashboard_applications_view_get_property;

	/* Set up private structure */
	g_type_class_add_private(klass, sizeof(XfdashboardApplicationsViewPrivate));

	/* Define properties */
	XfdashboardApplicationsViewProperties[PROP_VIEW_MODE]=
		g_param_spec_enum("view-mode",
							_("View mode"),
							_("The view mode used in this view"),
							XFDASHBOARD_TYPE_VIEW_MODE,
							DEFAULT_VIEW_MODE,
							G_PARAM_READWRITE);

	g_object_class_install_properties(gobjectClass, PROP_LAST, XfdashboardApplicationsViewProperties);
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
	priv->viewMode=-1;
	priv->appButton=XFDASHBOARD_APPLICATION_BUTTON(xfdashboard_application_button_new());

	/* Set up view */
	xfdashboard_view_set_internal_name(XFDASHBOARD_VIEW(self), "applications");
	xfdashboard_view_set_name(XFDASHBOARD_VIEW(self), _("Applications"));
	xfdashboard_view_set_icon(XFDASHBOARD_VIEW(self), DEFAULT_VIEW_ICON);

	/* Set up actor */
	xfdashboard_view_set_fit_mode(XFDASHBOARD_VIEW(self), XFDASHBOARD_FIT_MODE_HORIZONTAL);
	xfdashboard_applications_view_set_view_mode(self, DEFAULT_VIEW_MODE);

	xfdashboard_applications_menu_model_filter_by_section(priv->apps, GARCON_MENU(priv->currentRootMenuElement));
	clutter_model_set_sorting_column(CLUTTER_MODEL(priv->apps), XFDASHBOARD_APPLICATIONS_MENU_MODEL_COLUMN_TITLE);
	_xfdashboard_applications_view_on_filter_changed(self, priv->apps);

	/* Connect signals */
	g_signal_connect_swapped(priv->apps, "filter-changed", G_CALLBACK(_xfdashboard_applications_view_on_filter_changed), self);
}

/* Get/set view mode of view */
XfdashboardViewMode xfdashboard_applications_view_get_view_mode(XfdashboardApplicationsView *self)
{
	g_return_val_if_fail(XFDASHBOARD_IS_APPLICATIONS_VIEW(self), DEFAULT_VIEW_MODE);

	return(self->priv->viewMode);
}

void xfdashboard_applications_view_set_view_mode(XfdashboardApplicationsView *self, const XfdashboardViewMode inMode)
{
	g_return_if_fail(XFDASHBOARD_IS_APPLICATIONS_VIEW(self));
	g_return_if_fail(inMode<=XFDASHBOARD_VIEW_MODE_ICON);

	XfdashboardApplicationsViewPrivate	*priv=self->priv;

	/* Set value if changed */
	if(priv->viewMode!=inMode)
	{
		/* Set value */
		if(priv->layout)
		{
			clutter_actor_set_layout_manager(CLUTTER_ACTOR(self), NULL);
			priv->layout=NULL;
		}

		priv->viewMode=inMode;

		/* Set new layout manager */
		switch(priv->viewMode)
		{
			case XFDASHBOARD_VIEW_MODE_LIST:
				priv->layout=clutter_box_layout_new();
				clutter_box_layout_set_orientation(CLUTTER_BOX_LAYOUT(priv->layout), CLUTTER_ORIENTATION_VERTICAL);
				clutter_box_layout_set_spacing(CLUTTER_BOX_LAYOUT(priv->layout), DEFAULT_SPACING);
				clutter_actor_set_layout_manager(CLUTTER_ACTOR(self), priv->layout);
				break;

			case XFDASHBOARD_VIEW_MODE_ICON:
				priv->layout=clutter_flow_layout_new(CLUTTER_FLOW_HORIZONTAL);
				clutter_flow_layout_set_column_spacing(CLUTTER_FLOW_LAYOUT(priv->layout), DEFAULT_SPACING);
				clutter_flow_layout_set_row_spacing(CLUTTER_FLOW_LAYOUT(priv->layout), DEFAULT_SPACING);
				clutter_flow_layout_set_homogeneous(CLUTTER_FLOW_LAYOUT(priv->layout), TRUE);
				clutter_actor_set_layout_manager(CLUTTER_ACTOR(self), priv->layout);
				break;

			default:
				g_assert_not_reached();
		}

		/* Rebuild view */
		_xfdashboard_applications_view_on_filter_changed(self, NULL);

		/* Notify about property change */
		g_object_notify_by_pspec(G_OBJECT(self), XfdashboardApplicationsViewProperties[PROP_VIEW_MODE]);
	}
}
