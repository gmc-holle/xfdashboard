/*
 * search-view: A view showing applications matching search criterias
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

#include "search-view.h"
#include "types.h"
#include "enums.h"
#include "utils.h"
#include "applications-menu-model.h"
#include "application-button.h"

/* Define this class in GObject system */
G_DEFINE_TYPE(XfdashboardSearchView,
				xfdashboard_search_view,
				XFDASHBOARD_TYPE_VIEW)

/* Private structure - access only by public API if needed */
#define XFDASHBOARD_SEARCH_VIEW_GET_PRIVATE(obj) \
	(G_TYPE_INSTANCE_GET_PRIVATE((obj), XFDASHBOARD_TYPE_SEARCH_VIEW, XfdashboardSearchViewPrivate))

struct _XfdashboardSearchViewPrivate
{
	/* Properties related */
	XfdashboardViewMode					viewMode;

	/* Instance related */
	ClutterLayoutManager				*layout;
	XfdashboardApplicationsMenuModel	*apps;
	XfdashboardApplicationButton		*appButton;
};

/* Properties */
enum
{
	PROP_0,

	PROP_VIEW_MODE,

	PROP_LAST
};

GParamSpec* XfdashboardSearchViewProperties[PROP_LAST]={ 0, };

/* IMPLEMENTATION: Private variables and methods */
#define DEFAULT_VIEW_ICON			GTK_STOCK_FIND				// TODO: Replace by settings/theming object
#define DEFAULT_VIEW_MODE			XFDASHBOARD_VIEW_MODE_LIST	// TODO: Replace by settings/theming object
#define DEFAULT_SPACING				4.0f						// TODO: Replace by settings/theming object
#define DEFAULT_MENU_ICON_SIZE		64							// TODO: Replace by settings/theming object

struct _XfdashboardSearchViewFilterData
{
	GHashTable		*pool;
	gchar			*searchText;
};
typedef struct _XfdashboardSearchViewFilterData			XfdashboardSearchViewFilterData; 

/* Filter functions */
static void _xfdashboard_search_view_on_filter_data_destroy(gpointer inUserData)
{
	g_return_if_fail(inUserData);

	XfdashboardSearchViewFilterData		*searchData=(XfdashboardSearchViewFilterData*)inUserData;

	/* Release allocated resources */
	if(searchData->pool)
	{
		g_hash_table_destroy(searchData->pool);
		searchData->pool=NULL;
	}

	if(searchData->searchText)
	{
		g_free(searchData->searchText);
		searchData->searchText=NULL;
	}

	g_free(searchData);
}

static gboolean _xfdashboard_search_view_filter_nothing(ClutterModel *inModel,
														ClutterModelIter *inIter,
														gpointer inUserData)
{
	/* Always return FALSE to hide model item */
	return(FALSE);
}

static gboolean _xfdashboard_search_view_filter_title_only(ClutterModel *inModel,
															ClutterModelIter *inIter,
															gpointer inUserData)
{
	g_return_val_if_fail(XFDASHBOARD_IS_APPLICATIONS_MENU_MODEL(inModel), FALSE);
	g_return_val_if_fail(CLUTTER_IS_MODEL_ITER(inIter), FALSE);
	g_return_val_if_fail(inUserData, FALSE);

	XfdashboardSearchViewFilterData		*searchData=(XfdashboardSearchViewFilterData*)inUserData;
	guint								iterRow, poolRow;
	gboolean							isMatch=FALSE;
	GarconMenuElement					*menuElement=NULL;
	const gchar							*title, *command, *desktopID;
	gchar								*checkText, *foundPos;

	/* Get menu element at iterator */
	clutter_model_iter_get(inIter,
							XFDASHBOARD_APPLICATIONS_MENU_MODEL_COLUMN_SEQUENCE_ID, &iterRow,
							XFDASHBOARD_APPLICATIONS_MENU_MODEL_COLUMN_MENU_ELEMENT, &menuElement,
							-1);
	if(menuElement==NULL) return(FALSE);

	/* Only menu items and sub-menus can be visible */
	if(!GARCON_IS_MENU_ITEM(menuElement))
	{
		g_object_unref(menuElement);
		return(FALSE);
	}

	/* Check if title or command matches search criteria */
	title=garcon_menu_element_get_name(menuElement);
	command=garcon_menu_item_get_command(GARCON_MENU_ITEM(menuElement));
	desktopID=garcon_menu_item_get_desktop_id(GARCON_MENU_ITEM(menuElement));

	if(title && isMatch==FALSE)
	{
		checkText=g_utf8_strdown(title, -1);
		if(g_strstr_len(checkText, -1, searchData->searchText)) isMatch=TRUE;
		g_free(checkText);
	}

	if(command && isMatch==FALSE)
	{
		checkText=g_utf8_strdown(command, -1);
		foundPos=g_strstr_len(checkText, -1, searchData->searchText);
		if(foundPos &&
			(foundPos==checkText || (*(foundPos--))=='/'))
		{
			isMatch=TRUE;
		}
		g_free(checkText);
	}

	/* If menu element matches search criteria determine if it should be shown
	 * by checking if we haven't seen this desktop ID yet or if we have seen it
	 * if the row the given iterator refers is the same row where we have seen
	 * the menu element first.
	 */
	if(isMatch==TRUE)
	{
		if(g_hash_table_contains(searchData->pool, desktopID)!=TRUE)
		{
			g_hash_table_insert(searchData->pool, g_strdup(desktopID), GINT_TO_POINTER(iterRow));
		}
			else
			{
				poolRow=GPOINTER_TO_INT(g_hash_table_lookup(searchData->pool, desktopID));
				if(poolRow!=iterRow) isMatch=FALSE;
			}
	}

	/* If we get here return TRUE to show model data item or FALSE to hide */
	g_object_unref(menuElement);
	return(isMatch);
}

static gboolean _xfdashboard_search_view_filter_title_and_description(ClutterModel *inModel,
																		ClutterModelIter *inIter,
																		gpointer inUserData)
{
	g_return_val_if_fail(XFDASHBOARD_IS_APPLICATIONS_MENU_MODEL(inModel), FALSE);
	g_return_val_if_fail(CLUTTER_IS_MODEL_ITER(inIter), FALSE);
	g_return_val_if_fail(inUserData, FALSE);

	XfdashboardSearchViewFilterData		*searchData=(XfdashboardSearchViewFilterData*)inUserData;
	guint								iterRow, poolRow;
	gboolean							isMatch=FALSE;
	GarconMenuElement					*menuElement=NULL;
	const gchar							*title, *description, *command, *desktopID;
	gchar								*checkText, *foundPos;

	/* Get menu element at iterator */
	clutter_model_iter_get(inIter,
							XFDASHBOARD_APPLICATIONS_MENU_MODEL_COLUMN_SEQUENCE_ID, &iterRow,
							XFDASHBOARD_APPLICATIONS_MENU_MODEL_COLUMN_MENU_ELEMENT, &menuElement,
							-1);
	if(menuElement==NULL) return(FALSE);

	/* Only menu items and sub-menus can be visible */
	if(!GARCON_IS_MENU_ITEM(menuElement))
	{
		g_object_unref(menuElement);
		return(FALSE);
	}

	/* Check if title, description or command matches search criteria */
	title=garcon_menu_element_get_name(menuElement);
	description=garcon_menu_element_get_comment(menuElement);
	command=garcon_menu_item_get_command(GARCON_MENU_ITEM(menuElement));
	desktopID=garcon_menu_item_get_desktop_id(GARCON_MENU_ITEM(menuElement));

	if(title && isMatch==FALSE)
	{
		checkText=g_utf8_strdown(title, -1);
		if(g_strstr_len(checkText, -1, searchData->searchText)) isMatch=TRUE;
		g_free(checkText);
	}

	if(description && isMatch==FALSE)
	{
		checkText=g_utf8_strdown(description, -1);
		if(g_strstr_len(checkText, -1, searchData->searchText)) isMatch=TRUE;
		g_free(checkText);
	}

	if(command && isMatch==FALSE)
	{
		checkText=g_utf8_strdown(command, -1);
		foundPos=g_strstr_len(checkText, -1, searchData->searchText);
		if(foundPos &&
			(foundPos==checkText || (*(foundPos--))=='/'))
		{
			isMatch=TRUE;
		}
		g_free(checkText);
	}

	/* If menu element matches search criteria determine if it should be shown
	 * by checking if we haven't seen this desktop ID yet or if we have seen it
	 * if the row the given iterator refers is the same row where we have seen
	 * the menu element first.
	 */
	if(isMatch==TRUE)
	{
		if(g_hash_table_contains(searchData->pool, desktopID)!=TRUE)
		{
			g_hash_table_insert(searchData->pool, g_strdup(desktopID), GINT_TO_POINTER(iterRow));
		}
			else
			{
				poolRow=GPOINTER_TO_INT(g_hash_table_lookup(searchData->pool, desktopID));
				if(poolRow!=iterRow) isMatch=FALSE;
			}
	}

	/* If we get here return TRUE to show model data item or FALSE to hide */
	g_object_unref(menuElement);
	return(isMatch);
}

/* Update style of all child actors */
static void _xfdashboard_search_view_add_button_for_list_mode(XfdashboardSearchView *self,
																XfdashboardButton *inButton)
{
	g_return_if_fail(XFDASHBOARD_IS_SEARCH_VIEW(self));
	g_return_if_fail(XFDASHBOARD_IS_BUTTON(inButton));

	XfdashboardSearchViewPrivate	*priv=self->priv;
	const gchar						*actorFormat;
	gchar							*actorText;

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

static void _xfdashboard_search_view_add_button_for_icon_mode(XfdashboardSearchView *self,
																XfdashboardButton *inButton)
{
	g_return_if_fail(XFDASHBOARD_IS_SEARCH_VIEW(self));
	g_return_if_fail(XFDASHBOARD_IS_BUTTON(inButton));

	XfdashboardSearchViewPrivate	*priv=self->priv;
	const gchar						*actorFormat;
	gchar							*actorText;

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
static void _xfdashboard_search_view_on_item_clicked(XfdashboardSearchView *self, gpointer inUserData)
{
	g_return_if_fail(XFDASHBOARD_IS_SEARCH_VIEW(self));
	g_return_if_fail(XFDASHBOARD_IS_APPLICATION_BUTTON(inUserData));

	XfdashboardSearchViewPrivate	*priv=self->priv;
	XfdashboardApplicationButton	*button=XFDASHBOARD_APPLICATION_BUTTON(inUserData);
	GarconMenuElement				*element;

	/* Get associated menu element of button */
	element=xfdashboard_application_button_get_menu_element(button);

	/* If clicked item is a menu item execute command of menu item clicked
	 * and quit application
	 */
	if(GARCON_IS_MENU_ITEM(element) &&
		xfdashboard_application_button_execute(button))
	{
		/* Launching application seems to be successfuly so quit application */
		xfdashboard_application_quit();
		return;
	}
}

static void _xfdashboard_search_view_on_filter_changed(XfdashboardSearchView *self, gpointer inUserData)
{
	g_return_if_fail(XFDASHBOARD_IS_SEARCH_VIEW(self));

	XfdashboardSearchViewPrivate	*priv=XFDASHBOARD_SEARCH_VIEW(self)->priv;
	ClutterModelIter				*iterator;
	ClutterActor					*actor;
	GarconMenuElement				*menuElement=NULL;

	/* Destroy all children */
	clutter_actor_destroy_all_children(CLUTTER_ACTOR(self));
	clutter_layout_manager_layout_changed(priv->layout);

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
				_xfdashboard_search_view_add_button_for_list_mode(self, XFDASHBOARD_BUTTON(actor));
			}
				else
				{
					_xfdashboard_search_view_add_button_for_icon_mode(self, XFDASHBOARD_BUTTON(actor));
				}
			clutter_actor_show(actor);
			g_signal_connect_swapped(actor, "clicked", G_CALLBACK(_xfdashboard_search_view_on_item_clicked), self);

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
static void _xfdashboard_search_view_dispose(GObject *inObject)
{
	XfdashboardSearchView			*self=XFDASHBOARD_SEARCH_VIEW(inObject);
	XfdashboardSearchViewPrivate	*priv=self->priv;

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
	G_OBJECT_CLASS(xfdashboard_search_view_parent_class)->dispose(inObject);
}

/* Set/get properties */
static void _xfdashboard_search_view_set_property(GObject *inObject,
													guint inPropID,
													const GValue *inValue,
													GParamSpec *inSpec)
{
	XfdashboardSearchView				*self=XFDASHBOARD_SEARCH_VIEW(inObject);

	switch(inPropID)
	{
		case PROP_VIEW_MODE:
			xfdashboard_search_view_set_view_mode(self, (XfdashboardViewMode)g_value_get_enum(inValue));
			break;

		default:
			G_OBJECT_WARN_INVALID_PROPERTY_ID(inObject, inPropID, inSpec);
			break;
	}
}

static void _xfdashboard_search_view_get_property(GObject *inObject,
													guint inPropID,
													GValue *outValue,
													GParamSpec *inSpec)
{
	XfdashboardSearchView				*self=XFDASHBOARD_SEARCH_VIEW(inObject);
	XfdashboardSearchViewPrivate		*priv=self->priv;

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
void xfdashboard_search_view_class_init(XfdashboardSearchViewClass *klass)
{
	GObjectClass			*gobjectClass=G_OBJECT_CLASS(klass);

	/* Override functions */
	gobjectClass->dispose=_xfdashboard_search_view_dispose;
	gobjectClass->set_property=_xfdashboard_search_view_set_property;
	gobjectClass->get_property=_xfdashboard_search_view_get_property;

	/* Set up private structure */
	g_type_class_add_private(klass, sizeof(XfdashboardSearchViewPrivate));

	/* Define properties */
	XfdashboardSearchViewProperties[PROP_VIEW_MODE]=
		g_param_spec_enum("view-mode",
							_("View mode"),
							_("The view mode used in this view"),
							XFDASHBOARD_TYPE_VIEW_MODE,
							DEFAULT_VIEW_MODE,
							G_PARAM_READWRITE);

	g_object_class_install_properties(gobjectClass, PROP_LAST, XfdashboardSearchViewProperties);
}

/* Object initialization
 * Create private structure and set up default values
 */
void xfdashboard_search_view_init(XfdashboardSearchView *self)
{
	XfdashboardSearchViewPrivate	*priv;

	self->priv=priv=XFDASHBOARD_SEARCH_VIEW_GET_PRIVATE(self);

	/* Set up default values */
	priv->apps=XFDASHBOARD_APPLICATIONS_MENU_MODEL(xfdashboard_applications_menu_model_new());
	priv->viewMode=-1;
	priv->appButton=XFDASHBOARD_APPLICATION_BUTTON(xfdashboard_application_button_new());

	/* Set up view (Note: Search view is disabled by default!) */
	xfdashboard_view_set_internal_name(XFDASHBOARD_VIEW(self), "search");
	xfdashboard_view_set_name(XFDASHBOARD_VIEW(self), _("Search"));
	xfdashboard_view_set_icon(XFDASHBOARD_VIEW(self), DEFAULT_VIEW_ICON);
	xfdashboard_view_set_enabled(XFDASHBOARD_VIEW(self), FALSE);

	/* Set up actor */
	xfdashboard_view_set_fit_mode(XFDASHBOARD_VIEW(self), XFDASHBOARD_FIT_MODE_HORIZONTAL);
	xfdashboard_search_view_set_view_mode(self, DEFAULT_VIEW_MODE);

	/* Set up application model */
	clutter_model_set_sorting_column(CLUTTER_MODEL(priv->apps), XFDASHBOARD_APPLICATIONS_MENU_MODEL_COLUMN_TITLE);
	g_signal_connect_swapped(priv->apps, "filter-changed", G_CALLBACK(_xfdashboard_search_view_on_filter_changed), self);
}

/* Implementation: Public API */

/* Get/set view mode of view */
XfdashboardViewMode xfdashboard_search_view_get_view_mode(XfdashboardSearchView *self)
{
	g_return_val_if_fail(XFDASHBOARD_IS_SEARCH_VIEW(self), DEFAULT_VIEW_MODE);

	return(self->priv->viewMode);
}

void xfdashboard_search_view_set_view_mode(XfdashboardSearchView *self, const XfdashboardViewMode inMode)
{
	g_return_if_fail(XFDASHBOARD_IS_SEARCH_VIEW(self));
	g_return_if_fail(inMode<=XFDASHBOARD_VIEW_MODE_ICON);

	XfdashboardSearchViewPrivate	*priv=self->priv;

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
		_xfdashboard_search_view_on_filter_changed(self, NULL);

		/* Notify about property change */
		g_object_notify_by_pspec(G_OBJECT(self), XfdashboardSearchViewProperties[PROP_VIEW_MODE]);
	}
}

/* Update search view by looking up result for new search text */
void xfdashboard_search_view_update_search(XfdashboardSearchView *self, const gchar *inText)
{
	g_return_if_fail(XFDASHBOARD_IS_SEARCH_VIEW(self));

	XfdashboardSearchViewPrivate		*priv=self->priv;
	gint								searchTextLength;
	ClutterModelFilterFunc				filterFunc=_xfdashboard_search_view_filter_nothing;
	XfdashboardSearchViewFilterData		*searchData=NULL;

	/* Initialize search data */
	searchData=g_try_new0(XfdashboardSearchViewFilterData, 1);
	g_return_if_fail(searchData);

	searchData->pool=g_hash_table_new_full(g_str_hash, g_str_equal, g_free, NULL);
	searchData->searchText=g_strstrip(g_strdup(inText));

	/* Get length of search text and determine filter function */
	searchTextLength=g_utf8_strlen(searchData->searchText, -1);

	if(searchTextLength>0)
	{
		if(searchTextLength<3) filterFunc=_xfdashboard_search_view_filter_title_only;
			else filterFunc=_xfdashboard_search_view_filter_title_and_description;
	}

	/* Filter model data */
	clutter_model_set_filter(CLUTTER_MODEL(priv->apps), filterFunc, searchData, (GDestroyNotify)_xfdashboard_search_view_on_filter_data_destroy);
}
