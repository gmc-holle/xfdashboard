/*
 * applications-menu-model: A list model containing menu items
 *                          of applications
 * 
 * Copyright 2012-2015 Stephan Haller <nomad@froevel.de>
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

#include "applications-menu-model.h"
#include "application-database.h"

/* Define these classes in GObject system */
G_DEFINE_TYPE(XfdashboardApplicationsMenuModel,
				xfdashboard_applications_menu_model,
				CLUTTER_TYPE_LIST_MODEL)

/* Private structure - access only by public API if needed */
#define XFDASHBOARD_APPLICATIONS_MENU_MODEL_GET_PRIVATE(obj) \
	(G_TYPE_INSTANCE_GET_PRIVATE((obj), XFDASHBOARD_TYPE_APPLICATIONS_MENU_MODEL, XfdashboardApplicationsMenuModelPrivate))

struct _XfdashboardApplicationsMenuModelPrivate
{
	/* Instance related */
	GarconMenu						*rootMenu;

	XfdashboardApplicationDatabase	*appDB;
	guint							reloadRequiredSignalID;
};

/* Signals */
enum
{
	SIGNAL_LOADED,

	SIGNAL_LAST
};

static guint XfdashboardApplicationsMenuModelSignals[SIGNAL_LAST]={ 0, };

/* IMPLEMENTATION: Private variables and methods */
typedef struct _XfdashboardApplicationsMenuModelFillData		XfdashboardApplicationsMenuModelFillData;
struct _XfdashboardApplicationsMenuModelFillData
{
	gint		sequenceID;
	GSList		*populatedMenus;
};

/* Forward declarations */
static void _xfdashboard_applications_menu_model_fill_model(XfdashboardApplicationsMenuModel *self);

/* A menu was changed and needs to be reloaded */
static void _xfdashboard_applications_menu_model_on_reload_required(XfdashboardApplicationsMenuModel *self,
																	gpointer inUserData)
{
	g_return_if_fail(XFDASHBOARD_IS_APPLICATION_DATABASE(inUserData));

	/* Reload menu by filling it again. This also emits all necessary signals. */
	g_debug("Applications menu has changed and needs to be reloaded.");
	_xfdashboard_applications_menu_model_fill_model(self);
}

/* Clear all data in model and also release all allocated resources needed for this model */
static void _xfdashboard_applications_menu_model_clear(XfdashboardApplicationsMenuModel *self)
{
	XfdashboardApplicationsMenuModelPrivate		*priv;
	ClutterModelIter							*iterator;
	GarconMenuElement							*menuElement;

	g_return_if_fail(XFDASHBOARD_IS_APPLICATIONS_MENU_MODEL(self));

	priv=self->priv;

	/* Unset filter (forces all rows being accessible and not being skipped/filtered) */
	clutter_model_set_filter(CLUTTER_MODEL(self), NULL, NULL, NULL);

	/* Clean up and remove all rows */
	while(clutter_model_get_n_rows(CLUTTER_MODEL(self)))
	{
		/* Get data from model for clean up */
		menuElement=NULL;
		iterator=clutter_model_get_iter_at_row(CLUTTER_MODEL(self), 0);
		clutter_model_iter_get(iterator,
								XFDASHBOARD_APPLICATIONS_MENU_MODEL_COLUMN_MENU_ELEMENT, &menuElement,
								-1);

		/* Remove row */
		clutter_model_remove(CLUTTER_MODEL(self), 0);

		/* Release iterator */
		if(menuElement) g_object_unref(menuElement);
		g_object_unref(iterator);
	}

	/* Destroy root menu */
	if(priv->rootMenu)
	{
		g_object_unref(priv->rootMenu);
		priv->rootMenu=NULL;
	}
}

/* Helper function to filter model data */
static gboolean _xfdashboard_applications_menu_model_filter_by_menu(ClutterModel *inModel,
																	ClutterModelIter *inIter,
																	gpointer inUserData)
{
	XfdashboardApplicationsMenuModel			*self;
	XfdashboardApplicationsMenuModelPrivate		*priv;
	GarconMenu									*parentMenu;
	GarconMenu									*requestedParentMenu;
	GarconMenuElement							*menuElement;
	GarconMenuItemPool							*itemPool;
	const gchar									*desktopID;
	gboolean									doShow;

	g_return_val_if_fail(XFDASHBOARD_IS_APPLICATIONS_MENU_MODEL(inModel), FALSE);
	g_return_val_if_fail(CLUTTER_IS_MODEL_ITER(inIter), FALSE);
	g_return_val_if_fail(GARCON_IS_MENU(inUserData), FALSE);

	self=XFDASHBOARD_APPLICATIONS_MENU_MODEL(inModel);
	priv=self->priv;
	requestedParentMenu=GARCON_MENU(inUserData);
	menuElement=NULL;
	doShow=FALSE;

	/* Get menu element at iterator */
	clutter_model_iter_get(inIter,
							XFDASHBOARD_APPLICATIONS_MENU_MODEL_COLUMN_MENU_ELEMENT, &menuElement,
							XFDASHBOARD_APPLICATIONS_MENU_MODEL_COLUMN_PARENT_MENU, &parentMenu,
							-1);
	if(menuElement==NULL) return(FALSE);

	/* Only menu items and sub-menus can be visible */
	if(!GARCON_IS_MENU(menuElement) && !GARCON_IS_MENU_ITEM(menuElement))
	{
		g_object_unref(menuElement);
		return(FALSE);
	}

	/* If menu element is a menu check if it's parent menu is the requested one */
	if(GARCON_IS_MENU(menuElement))
	{
		if(requestedParentMenu==parentMenu ||
			(!requestedParentMenu && parentMenu==priv->rootMenu))
		{
			doShow=TRUE;
		}
	}
		/* Otherwise it is a menu item and check if item is in requested menu */
		else
		{
			/* Get desktop ID of menu item */
			desktopID=garcon_menu_item_get_desktop_id(GARCON_MENU_ITEM(menuElement));

			/* Get menu items of menu */
			itemPool=garcon_menu_get_item_pool(parentMenu);

			/* Determine if menu item at iterator is in menu's item pool */
			if(garcon_menu_item_pool_lookup(itemPool, desktopID)!=FALSE) doShow=TRUE;
		}

	/* Release allocated resources */
	if(parentMenu) g_object_unref(parentMenu);
	g_object_unref(menuElement);

	/* If we get here return TRUE to show model data item or FALSE to hide */
	return(doShow);
}

static gboolean _xfdashboard_applications_menu_model_filter_by_section(ClutterModel *inModel,
																		ClutterModelIter *inIter,
																		gpointer inUserData)
{
	XfdashboardApplicationsMenuModel			*self;
	XfdashboardApplicationsMenuModelPrivate		*priv;
	GarconMenu									*section;
	GarconMenu									*requestedSection;
	gboolean									doShow;

	g_return_val_if_fail(XFDASHBOARD_IS_APPLICATIONS_MENU_MODEL(inModel), FALSE);
	g_return_val_if_fail(CLUTTER_IS_MODEL_ITER(inIter), FALSE);
	g_return_val_if_fail(GARCON_IS_MENU(inUserData), FALSE);

	self=XFDASHBOARD_APPLICATIONS_MENU_MODEL(inModel);
	priv=self->priv;
	requestedSection=GARCON_MENU(inUserData);
	doShow=FALSE;

	/* Check if root section is requested */
	if(!requestedSection) requestedSection=priv->rootMenu;

	/* Get menu element at iterator */
	clutter_model_iter_get(inIter,
							XFDASHBOARD_APPLICATIONS_MENU_MODEL_COLUMN_SECTION, &section,
							-1);

	/* If menu element is a menu check if root menu is parent menu and root menu is requested */
	if((section && section==requestedSection) ||
		(!section && requestedSection==priv->rootMenu))
	{
		doShow=TRUE;
	}

	/* Release allocated resources */
	if(section) g_object_unref(section);

	/* If we get here return TRUE to show model data item or FALSE to hide */
	return(doShow);
}

/* Fill model */
static GarconMenu* _xfdashboard_applications_menu_model_find_similar_menu(XfdashboardApplicationsMenuModel *self,
																			GarconMenu *inMenu,
																			XfdashboardApplicationsMenuModelFillData *inFillData)
{
	GarconMenu					*parentMenu;
	GSList						*iter;
	GarconMenu					*foundMenu;

	g_return_val_if_fail(XFDASHBOARD_IS_APPLICATIONS_MENU_MODEL(self), NULL);
	g_return_val_if_fail(GARCON_IS_MENU(inMenu), NULL);
	g_return_val_if_fail(inFillData, NULL);

	/* Check if menu is visible. Hidden menus do not need to be checked. */
	if(!garcon_menu_element_get_visible(GARCON_MENU_ELEMENT(inMenu))) return(NULL);

	/* Get parent menu to look for at each menu we iterate */
	parentMenu=garcon_menu_get_parent(inMenu);
	if(!parentMenu) return(NULL);

	/* Iterate through parent menu up to current menu and lookup similar menu.
	 * A similar menu is identified by either they share the same directory
	 * or match in name, description and icon.
	 */
	foundMenu=NULL;
	for(iter=inFillData->populatedMenus; iter && !foundMenu; iter=g_slist_next(iter))
	{
		GarconMenu				*menu;

		/* Get menu element from list */
		menu=GARCON_MENU(iter->data);

		/* We can only process menus which have the same parent menu as the
		 * requested menu and they need to be visible.
		 */
		if(garcon_menu_get_parent(menu) &&
			garcon_menu_element_get_visible(GARCON_MENU_ELEMENT(menu)))
		{
			gboolean			isSimilar;

			/* Check if both menus share the same directory. That will be the
			 * case if iterator point to the menu which was given as function
			 * parameter. So it's safe just to iterate through.
			 */
			isSimilar=garcon_menu_directory_equal(garcon_menu_get_directory(menu),
													garcon_menu_get_directory(inMenu));

			/* If both menus do not share the same directory, check if they
			 * match in name, description and icon.
			 */
			if(!isSimilar)
			{
				const gchar		*left;
				const gchar		*right;

				/* Reset similar flag to TRUE as it will be set to FALSE again
				 * on first item not matching.
				 */
				isSimilar=TRUE;

				/* Check title */
				if(isSimilar)
				{
					left=garcon_menu_element_get_name(GARCON_MENU_ELEMENT(inMenu));
					right=garcon_menu_element_get_name(GARCON_MENU_ELEMENT(menu));
					if(g_strcmp0(left, right)!=0) isSimilar=FALSE;
				}


				/* Check description */
				if(isSimilar)
				{
					left=garcon_menu_element_get_comment(GARCON_MENU_ELEMENT(inMenu));
					right=garcon_menu_element_get_comment(GARCON_MENU_ELEMENT(menu));
					if(g_strcmp0(left, right)!=0) isSimilar=FALSE;
				}


				/* Check icon */
				if(isSimilar)
				{
					left=garcon_menu_element_get_icon_name(GARCON_MENU_ELEMENT(inMenu));
					right=garcon_menu_element_get_icon_name(GARCON_MENU_ELEMENT(menu));
					if(g_strcmp0(left, right)!=0) isSimilar=FALSE;
				}
			}

			/* If we get and we found a similar menu set result to return */
			if(isSimilar) foundMenu=menu;
		}
	}

	/* Return found menu */
	return(foundMenu);
}

static GarconMenu* _xfdashboard_applications_menu_model_find_section(XfdashboardApplicationsMenuModel *self,
																		GarconMenu *inMenu,
																		XfdashboardApplicationsMenuModelFillData *inFillData)
{
	XfdashboardApplicationsMenuModelPrivate		*priv;
	GarconMenu									*sectionMenu;
	GarconMenu									*parentMenu;

	g_return_val_if_fail(XFDASHBOARD_IS_APPLICATIONS_MENU_MODEL(self), NULL);
	g_return_val_if_fail(GARCON_IS_MENU(inMenu), NULL);

	priv=self->priv;

	/* Finding section is technically the same as looking up similar menu
	 * but only at top-level menus. So iterate through all parent menu
	 * until menu is found which parent menu is root menu. That is the
	 * section and we need to check for a similar one at that level.
	 */
	sectionMenu=inMenu;
	do
	{
		/* Get parent menu */
		parentMenu=garcon_menu_get_parent(sectionMenu);

		/* Check if parent menu is root menu stop here */
		if(!parentMenu || parentMenu==priv->rootMenu) break;

		/* Set current parent menu as found section menu */
		sectionMenu=parentMenu;
	}
	while(parentMenu);

	/* Find similar menu to found section menu */
	if(sectionMenu)
	{
		sectionMenu=_xfdashboard_applications_menu_model_find_similar_menu(self, sectionMenu, inFillData);
	}

	/* Return found section menu */
	return(sectionMenu);
}

static void _xfdashboard_applications_menu_model_fill_model_collect_menu(XfdashboardApplicationsMenuModel *self,
																			GarconMenu *inMenu,
																			GarconMenu *inParentMenu,
																			XfdashboardApplicationsMenuModelFillData *inFillData)
{
	XfdashboardApplicationsMenuModelPrivate			*priv;
	GarconMenu										*menu;
	GarconMenu										*section;
	GList											*elements, *element;

	g_return_if_fail(XFDASHBOARD_IS_APPLICATIONS_MENU_MODEL(self));
	g_return_if_fail(GARCON_IS_MENU(inMenu));

	priv=self->priv;
	section=NULL;
	menu=priv->rootMenu;

	/* Increase reference on menu going to be processed to keep it alive */
	g_object_ref(inMenu);

	/* Skip additional check on root menu as it must be processed normally and non-disruptively */
	if(inMenu!=priv->rootMenu)
	{
		/* Find section to add menu to */
		section=_xfdashboard_applications_menu_model_find_section(self, inMenu, inFillData);

		/* Add menu to model if no duplicate or similar menu exist */
		menu=_xfdashboard_applications_menu_model_find_similar_menu(self, inMenu, inFillData);
		if(!menu)
		{
			gchar									*title;
			gchar									*description;
			const gchar								*temp;

			/* To increase performance when sorting of filtering this model by title or description
			 * in a case-insensitive way store title and description in lower case.
			 */
			temp=garcon_menu_element_get_name(GARCON_MENU_ELEMENT(inMenu));
			if(temp) title=g_utf8_strdown(temp, -1);
				else title=NULL;

			temp=garcon_menu_element_get_comment(GARCON_MENU_ELEMENT(inMenu));
			if(temp) description=g_utf8_strdown(temp, -1);
				else description=NULL;

			/* Insert row into model because there is no duplicate
			 * and no similar menu
			 */
			inFillData->sequenceID++;
			clutter_model_append(CLUTTER_MODEL(self),
									XFDASHBOARD_APPLICATIONS_MENU_MODEL_COLUMN_SEQUENCE_ID, inFillData->sequenceID,
									XFDASHBOARD_APPLICATIONS_MENU_MODEL_COLUMN_MENU_ELEMENT, inMenu,
									XFDASHBOARD_APPLICATIONS_MENU_MODEL_COLUMN_PARENT_MENU, inParentMenu,
									XFDASHBOARD_APPLICATIONS_MENU_MODEL_COLUMN_SECTION, section,
									XFDASHBOARD_APPLICATIONS_MENU_MODEL_COLUMN_TITLE, title,
									XFDASHBOARD_APPLICATIONS_MENU_MODEL_COLUMN_DESCRIPTION, description,
									-1);

			/* Add menu to list of populated ones */
			inFillData->populatedMenus=g_slist_prepend(inFillData->populatedMenus, inMenu);

			/* All menu items should be added to this newly created menu */
			menu=inMenu;

			/* Find section of newly created menu to */
			section=_xfdashboard_applications_menu_model_find_section(self, menu, inFillData);

			/* Release allocated resources */
			g_free(title);
			g_free(description);
		}
	}

	/* Iterate through menu and add menu items and sub-menus */
	elements=garcon_menu_get_elements(inMenu);
	for(element=elements; element; element=g_list_next(element))
	{
		GarconMenuElement							*menuElement;

		/* Get menu element from list */
		menuElement=GARCON_MENU_ELEMENT(element->data);

		/* Check if menu element is visible */
		if(!menuElement || !garcon_menu_element_get_visible(menuElement)) continue;

		/* If element is a menu call this function recursively */
		if(GARCON_IS_MENU(menuElement))
		{
			_xfdashboard_applications_menu_model_fill_model_collect_menu(self, GARCON_MENU(menuElement), menu, inFillData);
		}

		/* Insert row into model if menu element is a menu item if it does not
		 * belong to root menu.
		 */
		if(GARCON_IS_MENU_ITEM(menuElement) &&
			menu!=priv->rootMenu)
		{
			gchar									*title;
			gchar									*description;
			const gchar								*temp;

			/* To increase performance when sorting of filtering this model by title or description
			 * in a case-insensitive way store title and description in lower case.
			 */
			temp=garcon_menu_element_get_name(GARCON_MENU_ELEMENT(menuElement));
			if(temp) title=g_utf8_strdown(temp, -1);
				else title=NULL;

			temp=garcon_menu_element_get_comment(GARCON_MENU_ELEMENT(menuElement));
			if(temp) description=g_utf8_strdown(temp, -1);
				else description=NULL;

			/* Add menu item to model */
			inFillData->sequenceID++;
			clutter_model_append(CLUTTER_MODEL(self),
									XFDASHBOARD_APPLICATIONS_MENU_MODEL_COLUMN_SEQUENCE_ID, inFillData->sequenceID,
									XFDASHBOARD_APPLICATIONS_MENU_MODEL_COLUMN_MENU_ELEMENT, menuElement,
									XFDASHBOARD_APPLICATIONS_MENU_MODEL_COLUMN_PARENT_MENU, menu,
									XFDASHBOARD_APPLICATIONS_MENU_MODEL_COLUMN_SECTION, section,
									XFDASHBOARD_APPLICATIONS_MENU_MODEL_COLUMN_TITLE, title,
									XFDASHBOARD_APPLICATIONS_MENU_MODEL_COLUMN_DESCRIPTION, description,
									-1);

			/* Release allocated resources */
			g_free(title);
			g_free(description);
		}
	}
	g_list_free(elements);

	/* Release allocated resources */
	g_object_unref(inMenu);
}

static void _xfdashboard_applications_menu_model_fill_model(XfdashboardApplicationsMenuModel *self)
{
	XfdashboardApplicationsMenuModelPrivate		*priv;
	GarconMenuItemCache							*cache;
	XfdashboardApplicationsMenuModelFillData	fillData;

	g_return_if_fail(XFDASHBOARD_IS_APPLICATIONS_MENU_MODEL(self));

	priv=self->priv;

	/* Clear model data */
	_xfdashboard_applications_menu_model_clear(self);

	/* Clear garcon's menu item cache otherwise some items will not be loaded
	 * if this is a reload of the model or a second(, third, ...) instance of model
	 */
	cache=garcon_menu_item_cache_get_default();
	garcon_menu_item_cache_invalidate(cache);
	g_object_unref(cache);

	/* Load root menu */
	priv->rootMenu=xfdashboard_application_database_get_application_menu(priv->appDB);

	/* Iterate through menus recursively to add them to model */
	fillData.sequenceID=0;
	fillData.populatedMenus=NULL;
	_xfdashboard_applications_menu_model_fill_model_collect_menu(self, priv->rootMenu, NULL, &fillData);

	/* Emit signal */
	g_signal_emit(self, XfdashboardApplicationsMenuModelSignals[SIGNAL_LOADED], 0);

	/* Release allocated resources at fill data structure */
	if(fillData.populatedMenus) g_slist_free(fillData.populatedMenus);
}

/* Idle callback to fill model */
static gboolean _xfdashboard_applications_menu_model_init_idle(gpointer inUserData)
{
	_xfdashboard_applications_menu_model_fill_model(XFDASHBOARD_APPLICATIONS_MENU_MODEL(inUserData));
	return(G_SOURCE_REMOVE);
}


/* IMPLEMENTATION: ClutterModel */

/* Resort model */
static gint _xfdashboard_applications_menu_model_resort_menu_element_callback(ClutterModel *inModel,
																				const GValue *inLeft,
																				const GValue *inRight,
																				gpointer inUserData)
{
	GarconMenuElement		*leftValue=GARCON_MENU_ELEMENT(g_value_get_object(inLeft));
	GarconMenuElement		*rightValue=GARCON_MENU_ELEMENT(g_value_get_object(inRight));
	const gchar				*leftName=garcon_menu_element_get_name(leftValue);
	const gchar				*rightName=garcon_menu_element_get_name(rightValue);

	return(g_strcmp0(leftName, rightName));
}

static gint _xfdashboard_applications_menu_model_resort_parent_menu_callback(ClutterModel *inModel,
																				const GValue *inLeft,
																				const GValue *inRight,
																				gpointer inUserData)
{
	GarconMenu				*leftValue=GARCON_MENU(g_value_get_object(inLeft));
	GarconMenu				*rightValue=GARCON_MENU(g_value_get_object(inRight));
	gint					result=0;

	/* If both menus have the same parent menu sort them by name ... */
	if(garcon_menu_get_parent(leftValue)==garcon_menu_get_parent(rightValue))
	{
		const gchar			*leftName=garcon_menu_element_get_name(GARCON_MENU_ELEMENT(leftValue));
		const gchar			*rightName=garcon_menu_element_get_name(GARCON_MENU_ELEMENT(rightValue));

		result=g_strcmp0(leftName, rightName);
	}
		/* ... otherwise get depth of each value and compare name of upper menu */
		else
		{
			GList			*leftPath=NULL;
			GList			*rightPath=NULL;
			GarconMenu		*leftMenu=leftValue;
			GarconMenu		*rightMenu=rightValue;
			const gchar		*leftName;
			const gchar		*rightName;
			gint			upperLevel;

			/* Build path of left value */
			while(leftMenu)
			{
				leftPath=g_list_prepend(leftPath, leftMenu);
				leftMenu=garcon_menu_get_parent(leftMenu);
			}

			/* Build path of right value */
			while(rightMenu)
			{
				rightPath=g_list_prepend(rightPath, rightMenu);
				rightMenu=garcon_menu_get_parent(rightMenu);
			}

			/* Find level of upper path of both values */
			upperLevel=MIN(g_list_length(leftPath), g_list_length(rightPath));
			if(upperLevel>0) upperLevel--;

			/* Get name of both values at upper path */
			leftName=garcon_menu_element_get_name(GARCON_MENU_ELEMENT(g_list_nth_data(leftPath, upperLevel)));
			rightName=garcon_menu_element_get_name(GARCON_MENU_ELEMENT(g_list_nth_data(rightPath, upperLevel)));

			/* Compare name of both value at upper path */
			result=g_strcmp0(leftName, rightName);
		}

	/* Return result */
	return(result);
}

static gint _xfdashboard_applications_menu_model_resort_section_callback(ClutterModel *inModel,
																				const GValue *inLeft,
																				const GValue *inRight,
																				gpointer inUserData)
{
	GObject					*leftValue=g_value_get_object(inLeft);
	GObject					*rightValue=g_value_get_object(inLeft);
	const gchar				*leftName=NULL;
	const gchar				*rightName=NULL;

	if(leftValue &&
		GARCON_IS_MENU_ELEMENT(leftValue))
	{
		leftName=garcon_menu_element_get_name(GARCON_MENU_ELEMENT(leftValue));
	}

	if(rightValue &&
		GARCON_IS_MENU_ELEMENT(rightValue))
	{
		rightName=garcon_menu_element_get_name(GARCON_MENU_ELEMENT(rightValue));
	}

	return(g_strcmp0(leftName, rightName));
}

static gint _xfdashboard_applications_menu_model_resort_string_callback(ClutterModel *inModel,
																		const GValue *inLeft,
																		const GValue *inRight,
																		gpointer inUserData)
{
	const gchar		*leftValue=g_value_get_string(inLeft);
	const gchar		*rightValue=g_value_get_string(inRight);

	return(g_strcmp0(leftValue, rightValue));
}

static gint _xfdashboard_applications_menu_model_resort_uint_callback(ClutterModel *inModel,
																		const GValue *inLeft,
																		const GValue *inRight,
																		gpointer inUserData)
{
	guint		leftValue=g_value_get_uint(inLeft);
	guint		rightValue=g_value_get_uint(inRight);

	if(leftValue<rightValue) return(-1);
		else if(leftValue>rightValue) return(1);
	return(0);
}

static void _xfdashboard_applications_menu_model_resort(ClutterModel *inModel,
														ClutterModelSortFunc inSortCallback,
														gpointer inUserData)
{
	g_return_if_fail(XFDASHBOARD_IS_APPLICATIONS_MENU_MODEL(inModel));

	/* If given sort function is NULL use default one */
	if(inSortCallback==NULL)
	{
		gint	sortColumn=clutter_model_get_sorting_column(inModel);

		switch(sortColumn)
		{
			case XFDASHBOARD_APPLICATIONS_MENU_MODEL_COLUMN_SEQUENCE_ID:
				inSortCallback=_xfdashboard_applications_menu_model_resort_uint_callback;
				break;

			case XFDASHBOARD_APPLICATIONS_MENU_MODEL_COLUMN_MENU_ELEMENT:
				inSortCallback=_xfdashboard_applications_menu_model_resort_menu_element_callback;
				break;

			case XFDASHBOARD_APPLICATIONS_MENU_MODEL_COLUMN_PARENT_MENU:
				inSortCallback=_xfdashboard_applications_menu_model_resort_parent_menu_callback;
				break;

			case XFDASHBOARD_APPLICATIONS_MENU_MODEL_COLUMN_SECTION:
				inSortCallback=_xfdashboard_applications_menu_model_resort_section_callback;
				break;

			case XFDASHBOARD_APPLICATIONS_MENU_MODEL_COLUMN_TITLE:
			case XFDASHBOARD_APPLICATIONS_MENU_MODEL_COLUMN_DESCRIPTION:
				inSortCallback=_xfdashboard_applications_menu_model_resort_string_callback;
				break;

			default:
				g_critical(_("Sorting column %d without user-defined function is not possible"), sortColumn);
				g_assert_not_reached();
				break;
		}
	}

	/* Call parent's class resort method */
	CLUTTER_MODEL_CLASS(xfdashboard_applications_menu_model_parent_class)->resort(inModel, inSortCallback, inUserData);
}

/* IMPLEMENTATION: GObject */

/* Dispose this object */
static void _xfdashboard_applications_menu_model_dispose(GObject *inObject)
{
	XfdashboardApplicationsMenuModel			*self=XFDASHBOARD_APPLICATIONS_MENU_MODEL(inObject);
	XfdashboardApplicationsMenuModelPrivate		*priv=self->priv;

	/* Release allocated resources */
	if(priv->rootMenu)
	{
		g_object_unref(priv->rootMenu);
		priv->rootMenu=NULL;
	}

	if(priv->appDB)
	{
		if(priv->reloadRequiredSignalID)
		{
			g_signal_handler_disconnect(priv->appDB, priv->reloadRequiredSignalID);
			priv->reloadRequiredSignalID=0;
		}

		g_object_unref(priv->appDB);
		priv->appDB=NULL;
	}

	/* Call parent's class dispose method */
	G_OBJECT_CLASS(xfdashboard_applications_menu_model_parent_class)->dispose(inObject);
}

/* Class initialization
 * Override functions in parent classes and define properties
 * and signals
 */
static void xfdashboard_applications_menu_model_class_init(XfdashboardApplicationsMenuModelClass *klass)
{
	ClutterModelClass		*modelClass=CLUTTER_MODEL_CLASS(klass);
	GObjectClass			*gobjectClass=G_OBJECT_CLASS(klass);

	/* Override functions */
	modelClass->resort=_xfdashboard_applications_menu_model_resort;

	gobjectClass->dispose=_xfdashboard_applications_menu_model_dispose;

	/* Set up private structure */
	g_type_class_add_private(klass, sizeof(XfdashboardApplicationsMenuModelPrivate));

	/* Define signals */
	XfdashboardApplicationsMenuModelSignals[SIGNAL_LOADED]=
		g_signal_new("loaded",
						G_TYPE_FROM_CLASS(klass),
						G_SIGNAL_RUN_LAST,
						G_STRUCT_OFFSET(XfdashboardApplicationsMenuModelClass, loaded),
						NULL,
						NULL,
						g_cclosure_marshal_VOID__VOID,
						G_TYPE_NONE,
						0);
}

/* Object initialization
 * Create private structure and set up default values
 */
static void xfdashboard_applications_menu_model_init(XfdashboardApplicationsMenuModel *self)
{
	XfdashboardApplicationsMenuModelPrivate	*priv;
	GType									columnTypes[]=	{
																G_TYPE_UINT, /* XFDASHBOARD_APPLICATIONS_MENU_MODEL_COLUMN_SEQUENCE_ID */
																GARCON_TYPE_MENU_ELEMENT, /* XFDASHBOARD_APPLICATIONS_MENU_MODEL_COLUMN_MENU_ELEMENT */
																GARCON_TYPE_MENU, /* XFDASHBOARD_APPLICATIONS_MENU_MODEL_COLUMN_PARENT_MENU */
																GARCON_TYPE_MENU, /* XFDASHBOARD_APPLICATIONS_MENU_MODEL_COLUMN_SECTION */
																G_TYPE_STRING, /* XFDASHBOARD_APPLICATIONS_MENU_MODEL_COLUMN_TITLE */
																G_TYPE_STRING /* XFDASHBOARD_APPLICATIONS_MENU_MODEL_COLUMN_DESCRIPTION */
															};
	const gchar*							columnNames[]=	{
																_("ID"), /* XFDASHBOARD_APPLICATIONS_MENU_MODEL_COLUMN_SEQUENCE_ID */
																_("Menu item"), /* XFDASHBOARD_APPLICATIONS_MENU_MODEL_COLUMN_MENU_ELEMENT */
																_("Parent menu"), /* XFDASHBOARD_APPLICATIONS_MENU_MODEL_COLUMN_PARENT_MENU */
																_("Section"), /* XFDASHBOARD_APPLICATIONS_MENU_MODEL_COLUMN_SECTION */
																_("Title"), /* XFDASHBOARD_APPLICATIONS_MENU_MODEL_COLUMN_TITLE */
																_("Description"), /* XFDASHBOARD_APPLICATIONS_MENU_MODEL_COLUMN_DESCRIPTION */
															};

	priv=self->priv=XFDASHBOARD_APPLICATIONS_MENU_MODEL_GET_PRIVATE(self);

	/* Set up default values */
	priv->rootMenu=NULL;
	priv->appDB=NULL;
	priv->reloadRequiredSignalID=0;

	/* Set up model */
	clutter_model_set_types(CLUTTER_MODEL(self), XFDASHBOARD_APPLICATIONS_MENU_MODEL_COLUMN_LAST, columnTypes);
	clutter_model_set_names(CLUTTER_MODEL(self), XFDASHBOARD_APPLICATIONS_MENU_MODEL_COLUMN_LAST, columnNames);

	/* Get application database and connect signals */
	priv->appDB=xfdashboard_application_database_get_default();
	priv->reloadRequiredSignalID=g_signal_connect_swapped(priv->appDB,
															"menu-reload-required",
															G_CALLBACK(_xfdashboard_applications_menu_model_on_reload_required),
															self);
	/* Defer filling model */
	clutter_threads_add_idle(_xfdashboard_applications_menu_model_init_idle, self);
}

/* IMPLEMENTATION: Public API */

ClutterModel* xfdashboard_applications_menu_model_new(void)
{
	return(CLUTTER_MODEL(g_object_new(XFDASHBOARD_TYPE_APPLICATIONS_MENU_MODEL, NULL)));
}

/* Filter menu items being a direct child item of requested menu */
void xfdashboard_applications_menu_model_filter_by_menu(XfdashboardApplicationsMenuModel *self, GarconMenu *inMenu)
{
	XfdashboardApplicationsMenuModelPrivate		*priv;

	g_return_if_fail(XFDASHBOARD_IS_APPLICATIONS_MENU_MODEL(self));
	g_return_if_fail(inMenu==NULL || GARCON_IS_MENU(inMenu));

	priv=self->priv;

	/* If menu is NULL filter root menu */
	if(inMenu==NULL) inMenu=priv->rootMenu;

	/* Filter model data */
	clutter_model_set_filter(CLUTTER_MODEL(self), _xfdashboard_applications_menu_model_filter_by_menu, g_object_ref(inMenu), g_object_unref);
}

/* Filter menu items being an indirect child item of requested section */
void xfdashboard_applications_menu_model_filter_by_section(XfdashboardApplicationsMenuModel *self, GarconMenu *inSection)
{
	XfdashboardApplicationsMenuModelPrivate		*priv;

	g_return_if_fail(XFDASHBOARD_IS_APPLICATIONS_MENU_MODEL(self));
	g_return_if_fail(inSection==NULL || GARCON_IS_MENU(inSection));

	priv=self->priv;

	/* If requested section is NULL filter root menu */
	if(inSection==NULL) inSection=priv->rootMenu;

	/* Filter model data */
	clutter_model_set_filter(CLUTTER_MODEL(self), _xfdashboard_applications_menu_model_filter_by_section, g_object_ref(inSection), (GDestroyNotify)g_object_unref);
}
