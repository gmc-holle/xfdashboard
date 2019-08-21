/*
 * applications-menu-model: A list model containing menu items
 *                          of applications
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

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <glib/gi18n-lib.h>

#include <libxfdashboard/applications-menu-model.h>
#include <libxfdashboard/application-database.h>
#include <libxfdashboard/compat.h>
#include <libxfdashboard/debug.h>


/* Define these classes in GObject system */
struct _XfdashboardApplicationsMenuModelPrivate
{
	/* Instance related */
	GarconMenu						*rootMenu;

	XfdashboardApplicationDatabase	*appDB;
	guint							reloadRequiredSignalID;
};

G_DEFINE_TYPE_WITH_PRIVATE(XfdashboardApplicationsMenuModel,
							xfdashboard_applications_menu_model,
							XFDASHBOARD_TYPE_MODEL)

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
	gint				sequenceID;
	GSList				*populatedMenus;
};

typedef struct _XfdashboardApplicationsMenuModelItem			XfdashboardApplicationsMenuModelItem;
struct _XfdashboardApplicationsMenuModelItem
{
	guint				sequenceID;
	GarconMenuElement	*menuElement;
	GarconMenu			*parentMenu;
	GarconMenu			*section;
	gchar				*title;
	gchar				*description;
};

/* Forward declarations */
static void _xfdashboard_applications_menu_model_fill_model(XfdashboardApplicationsMenuModel *self);

/* Free an item of application menu model */
static void _xfdashboard_applications_menu_model_item_free(XfdashboardApplicationsMenuModelItem *inItem)
{
	if(inItem)
	{
		/* Release allocated resources in item */
		if(inItem->menuElement) g_object_unref(inItem->menuElement);
		if(inItem->parentMenu) g_object_unref(inItem->parentMenu);
		if(inItem->section) g_object_unref(inItem->section);
		if(inItem->title) g_free(inItem->title);
		if(inItem->description) g_free(inItem->description);

		/* Free item */
		g_free(inItem);
	}
}

/* Create a new item for application menu model */
static XfdashboardApplicationsMenuModelItem* _xfdashboard_applications_menu_model_item_new(void)
{
	XfdashboardApplicationsMenuModelItem	*item;

	/* Create empty item */
	item=g_new0(XfdashboardApplicationsMenuModelItem, 1);

	/* Return new empty item */
	return(item);
}

/* A menu was changed and needs to be reloaded */
static void _xfdashboard_applications_menu_model_on_reload_required(XfdashboardApplicationsMenuModel *self,
																	gpointer inUserData)
{
	g_return_if_fail(XFDASHBOARD_IS_APPLICATION_DATABASE(inUserData));

	/* Reload menu by filling it again. This also emits all necessary signals. */
	XFDASHBOARD_DEBUG(self, APPLICATIONS, "Applications menu has changed and needs to be reloaded.");
	_xfdashboard_applications_menu_model_fill_model(self);
}

/* Clear all data in model and also release all allocated resources needed for this model */
static void _xfdashboard_applications_menu_model_clear(XfdashboardApplicationsMenuModel *self)
{
	XfdashboardApplicationsMenuModelPrivate		*priv;

	g_return_if_fail(XFDASHBOARD_IS_APPLICATIONS_MENU_MODEL(self));

	priv=self->priv;

	/* Unset filter (forces all rows being accessible and not being skipped/filtered) */
	xfdashboard_model_set_filter(XFDASHBOARD_MODEL(self), NULL, NULL, NULL);

	/* Clean up and remove all rows */
	xfdashboard_model_remove_all(XFDASHBOARD_MODEL(self));

	/* Destroy root menu */
	if(priv->rootMenu)
	{
		g_object_unref(priv->rootMenu);
		priv->rootMenu=NULL;
	}
}

/* Helper function to filter model data */
static gboolean _xfdashboard_applications_menu_model_filter_by_menu(XfdashboardModelIter *inIter,
																	gpointer inUserData)
{
	XfdashboardApplicationsMenuModel			*model;
	XfdashboardApplicationsMenuModelPrivate		*modelPriv;
	gboolean									doShow;
	GarconMenu									*requestedParentMenu;
	XfdashboardApplicationsMenuModelItem		*item;
	GarconMenuItemPool							*itemPool;
	const gchar									*desktopID;

	g_return_val_if_fail(XFDASHBOARD_IS_MODEL_ITER(inIter), FALSE);
	g_return_val_if_fail(GARCON_IS_MENU(inUserData), FALSE);
	g_return_val_if_fail(XFDASHBOARD_IS_APPLICATIONS_MENU_MODEL(xfdashboard_model_iter_get_model(inIter)), FALSE);

	doShow=FALSE;
	requestedParentMenu=GARCON_MENU(inUserData);
	model=XFDASHBOARD_APPLICATIONS_MENU_MODEL(xfdashboard_model_iter_get_model(inIter));
	modelPriv=model->priv;

	/* Get menu element at iterator */
	item=(XfdashboardApplicationsMenuModelItem*)xfdashboard_model_iter_get(inIter);
	if(item->menuElement==NULL) return(FALSE);

	/* Only menu items and sub-menus can be visible */
	if(!GARCON_IS_MENU(item->menuElement) && !GARCON_IS_MENU_ITEM(item->menuElement))
	{
		return(FALSE);
	}

	/* If menu element is a menu check if it's parent menu is the requested one */
	if(GARCON_IS_MENU(item->menuElement))
	{
		if(requestedParentMenu==item->parentMenu ||
			(!requestedParentMenu && item->parentMenu==modelPriv->rootMenu))
		{
			doShow=TRUE;
		}
	}
		/* Otherwise it is a menu item and check if item is in requested menu */
		else
		{
			/* Get desktop ID of menu item */
			desktopID=garcon_menu_item_get_desktop_id(GARCON_MENU_ITEM(item->menuElement));

			/* Get menu items of menu */
			itemPool=garcon_menu_get_item_pool(item->parentMenu);

			/* Determine if menu item at iterator is in menu's item pool */
			if(garcon_menu_item_pool_lookup(itemPool, desktopID)!=FALSE) doShow=TRUE;
		}

	/* If we get here return TRUE to show model data item or FALSE to hide */
	return(doShow);
}

static gboolean _xfdashboard_applications_menu_model_filter_by_section(XfdashboardModelIter *inIter,
																		gpointer inUserData)
{
	XfdashboardApplicationsMenuModel			*model;
	XfdashboardApplicationsMenuModelPrivate		*modelPriv;
	gboolean									doShow;
	GarconMenu									*requestedSection;
	XfdashboardApplicationsMenuModelItem		*item;

	g_return_val_if_fail(XFDASHBOARD_IS_MODEL_ITER(inIter), FALSE);
	g_return_val_if_fail(XFDASHBOARD_IS_APPLICATIONS_MENU_MODEL(xfdashboard_model_iter_get_model(inIter)), FALSE);
	g_return_val_if_fail(GARCON_IS_MENU(inUserData), FALSE);

	doShow=FALSE;
	requestedSection=GARCON_MENU(inUserData);
	model=XFDASHBOARD_APPLICATIONS_MENU_MODEL(xfdashboard_model_iter_get_model(inIter));
	modelPriv=model->priv;

	/* Check if root section is requested */
	if(!requestedSection) requestedSection=modelPriv->rootMenu;

	/* Get menu element at iterator */
	item=(XfdashboardApplicationsMenuModelItem*)xfdashboard_model_iter_get(inIter);

	/* If menu element is a menu check if root menu is parent menu and root menu is requested */
	if((item->section && item->section==requestedSection) ||
		(!item->section && requestedSection==modelPriv->rootMenu))
	{
		doShow=TRUE;
	}

	/* If we get here return TRUE to show model data item or FALSE to hide */
	return(doShow);
}

static gboolean _xfdashboard_applications_menu_model_filter_empty(XfdashboardModelIter *inIter,
																	gpointer inUserData)
{
	g_return_val_if_fail(XFDASHBOARD_IS_MODEL_ITER(inIter), FALSE);
	g_return_val_if_fail(GARCON_IS_MENU(inUserData), FALSE);

	/* This functions always returns FALSE because each entry is considered empty and hidden */
	return(FALSE);
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
		GarconMenu				*iterMenu;

		/* Get menu element from list */
		iterMenu=GARCON_MENU(iter->data);

		/* We can only process menus which have the same parent menu as the
		 * requested menu and they need to be visible.
		 */
		if(garcon_menu_get_parent(iterMenu) &&
			garcon_menu_element_get_visible(GARCON_MENU_ELEMENT(iterMenu)))
		{
			gboolean			isSimilar;
			GarconMenuDirectory	*iterMenuDirectory;
			GarconMenuDirectory	*menuDirectory;

			/* Check if both menus share the same directory. That will be the
			 * case if iterator point to the menu which was given as function
			 * parameter. So it's safe just to iterate through.
			 */
			iterMenuDirectory=garcon_menu_get_directory(iterMenu);
			menuDirectory=garcon_menu_get_directory(inMenu);

			isSimilar=FALSE;
			if(iterMenuDirectory && menuDirectory)
			{
				isSimilar=garcon_menu_directory_equal(iterMenuDirectory, menuDirectory);
			}

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
					right=garcon_menu_element_get_name(GARCON_MENU_ELEMENT(iterMenu));
					if(g_strcmp0(left, right)!=0) isSimilar=FALSE;
				}


				/* Check description */
				if(isSimilar)
				{
					left=garcon_menu_element_get_comment(GARCON_MENU_ELEMENT(inMenu));
					right=garcon_menu_element_get_comment(GARCON_MENU_ELEMENT(iterMenu));
					if(g_strcmp0(left, right)!=0) isSimilar=FALSE;
				}


				/* Check icon */
				if(isSimilar)
				{
					left=garcon_menu_element_get_icon_name(GARCON_MENU_ELEMENT(inMenu));
					right=garcon_menu_element_get_icon_name(GARCON_MENU_ELEMENT(iterMenu));
					if(g_strcmp0(left, right)!=0) isSimilar=FALSE;
				}
			}

			/* If we get and we found a similar menu set result to return */
			if(isSimilar) foundMenu=iterMenu;
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
	XfdashboardApplicationsMenuModelItem			*item;

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

			item=_xfdashboard_applications_menu_model_item_new();
			item->sequenceID=inFillData->sequenceID;
			if(inMenu) item->menuElement=GARCON_MENU_ELEMENT(g_object_ref(inMenu));
			if(inParentMenu) item->parentMenu=g_object_ref(inParentMenu);
			if(section) item->section=g_object_ref(section);
			if(title) item->title=g_strdup(title);
			if(description) item->description=g_strdup(description);

			xfdashboard_model_append(XFDASHBOARD_MODEL(self), item, NULL);

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

			item=_xfdashboard_applications_menu_model_item_new();
			item->sequenceID=inFillData->sequenceID;
			if(menuElement) item->menuElement=g_object_ref(menuElement);
			if(menu) item->parentMenu=g_object_ref(menu);
			if(section) item->section=g_object_ref(section);
			if(title) item->title=g_strdup(title);
			if(description) item->description=g_strdup(description);

			xfdashboard_model_append(XFDASHBOARD_MODEL(self), item, NULL);

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
	GObjectClass			*gobjectClass=G_OBJECT_CLASS(klass);

	gobjectClass->dispose=_xfdashboard_applications_menu_model_dispose;

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

	priv=self->priv=xfdashboard_applications_menu_model_get_instance_private(self);

	/* Set up default values */
	priv->rootMenu=NULL;
	priv->appDB=NULL;
	priv->reloadRequiredSignalID=0;

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

/* Create a new instance of application menu model */
XfdashboardModel* xfdashboard_applications_menu_model_new(void)
{
	GObject		*model;

	/* Create instance */
	model=g_object_new(XFDASHBOARD_TYPE_APPLICATIONS_MENU_MODEL,
						"free-data-callback", _xfdashboard_applications_menu_model_item_free,
						NULL);
	if(!model) return(NULL);

	/* Return new instance */
	return(XFDASHBOARD_MODEL(model));
}

/* Get values from application menu model at requested iterator and columns */
void xfdashboard_applications_menu_model_get(XfdashboardApplicationsMenuModel *self,
												XfdashboardModelIter *inIter,
												...)
{
	XfdashboardModel							*model;
	XfdashboardApplicationsMenuModelItem		*item;
	va_list										args;
	gint										column;
	gpointer									*storage;

	g_return_if_fail(XFDASHBOARD_IS_APPLICATIONS_MENU_MODEL(self));
	g_return_if_fail(XFDASHBOARD_IS_MODEL_ITER(inIter));

	/* Check if iterator belongs to this model */
	model=xfdashboard_model_iter_get_model(inIter);
	if(!XFDASHBOARD_IS_APPLICATIONS_MENU_MODEL(model) ||
		XFDASHBOARD_APPLICATIONS_MENU_MODEL(model)!=self)
	{
		g_critical(_("Iterator does not belong to application menu model."));
		return;
	}

	/* Get item from iterator */
	item=(XfdashboardApplicationsMenuModelItem*)xfdashboard_model_iter_get(inIter);
	g_assert(item);

	/* Iterate through column index and pointer where to store value until
	 * until end of list (marked with -1) is reached.
	 */
	va_start(args, inIter);

	column=va_arg(args, gint);
	while(column!=-1)
	{
		if(column<0 || column>=XFDASHBOARD_APPLICATIONS_MENU_MODEL_COLUMN_LAST)
		{
			g_warning(_("Invalid column number %d added to iter (remember to end your list of columns with a -1)"),
						column);
			break;
		}

		/* Get generic pointer to storage as it will be casted as necessary
		 * when determining which column is requested.
		 */
		storage=va_arg(args, gpointer*);
		if(!storage)
		{
			g_warning(_("No storage pointer provided to store value of column number %d"),
						column);
			break;
		}

		/* Check which column is requested and store value at pointer */
		switch(column)
		{
			case XFDASHBOARD_APPLICATIONS_MENU_MODEL_COLUMN_SEQUENCE_ID:
				*((gint*)storage)=item->sequenceID;
				break;

			case XFDASHBOARD_APPLICATIONS_MENU_MODEL_COLUMN_MENU_ELEMENT:
				if(item->menuElement) *storage=g_object_ref(item->menuElement);
					else *storage=NULL;
				break;

			case XFDASHBOARD_APPLICATIONS_MENU_MODEL_COLUMN_PARENT_MENU:
				if(item->parentMenu) *storage=g_object_ref(item->parentMenu);
					else *storage=NULL;
				break;

			case XFDASHBOARD_APPLICATIONS_MENU_MODEL_COLUMN_SECTION:
				if(item->section) *storage=g_object_ref(item->section);
					else *storage=NULL;
				break;

			case XFDASHBOARD_APPLICATIONS_MENU_MODEL_COLUMN_TITLE:
				if(item->title) *((gchar**)storage)=g_strdup(item->title);
					else *((gchar**)storage)=g_strdup("");
				break;

			case XFDASHBOARD_APPLICATIONS_MENU_MODEL_COLUMN_DESCRIPTION:
				if(item->description) *((gchar**)storage)=g_strdup(item->description);
					else *((gchar**)storage)=g_strdup("");
				break;

			default:
				g_assert_not_reached();
				break;
		}

		/* Continue with next column and storage pointer */
		column=va_arg(args, gint);
	}

	va_end(args);
}

/* Filter menu items being a direct child item of requested menu */
void xfdashboard_applications_menu_model_filter_by_menu(XfdashboardApplicationsMenuModel *self,
														GarconMenu *inMenu)
{
	XfdashboardApplicationsMenuModelPrivate		*priv;

	g_return_if_fail(XFDASHBOARD_IS_APPLICATIONS_MENU_MODEL(self));
	g_return_if_fail(inMenu==NULL || GARCON_IS_MENU(inMenu));

	priv=self->priv;

	/* If menu is NULL filter root menu */
	if(inMenu==NULL) inMenu=priv->rootMenu;

	/* Filter model data */
	xfdashboard_model_set_filter(XFDASHBOARD_MODEL(self),
									_xfdashboard_applications_menu_model_filter_by_menu,
									g_object_ref(inMenu),
									g_object_unref);
}

/* Filter menu items being an indirect child item of requested section */
void xfdashboard_applications_menu_model_filter_by_section(XfdashboardApplicationsMenuModel *self,
															GarconMenu *inSection)
{
	XfdashboardApplicationsMenuModelPrivate		*priv;

	g_return_if_fail(XFDASHBOARD_IS_APPLICATIONS_MENU_MODEL(self));
	g_return_if_fail(inSection==NULL || GARCON_IS_MENU(inSection));

	priv=self->priv;

	/* If requested section is NULL filter root menu */
	if(!inSection) inSection=priv->rootMenu;

	/* Filter model data */
	if(inSection)
	{
		XFDASHBOARD_DEBUG(self, APPLICATIONS,
							"Filtering section '%s'",
							garcon_menu_element_get_name(GARCON_MENU_ELEMENT(inSection)));
		xfdashboard_model_set_filter(XFDASHBOARD_MODEL(self),
										_xfdashboard_applications_menu_model_filter_by_section,
										g_object_ref(inSection),
										g_object_unref);
	}
		else
		{
			XFDASHBOARD_DEBUG(self, APPLICATIONS, "Filtering root section because no section requested");
			xfdashboard_model_set_filter(XFDASHBOARD_MODEL(self),
											_xfdashboard_applications_menu_model_filter_empty,
											NULL,
											NULL);
		}
}
