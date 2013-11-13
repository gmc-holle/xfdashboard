/*
 * applications-menu-model: A list model containing menu items
 *                          of applications
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

#include "applications-menu-model.h"

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
	GarconMenu		*rootMenu;
	GHashTable		*sections;
};

/* Signals */
enum
{
	SIGNAL_LOADED,

	SIGNAL_LAST
};

guint XfdashboardApplicationsMenuModelSignals[SIGNAL_LAST]={ 0, };

/* IMPLEMENTATION: Private variables and methods */

/* Clear all data in model and also release all allocated resources needed for this model */
static void _xfdashboard_applications_menu_model_clear(XfdashboardApplicationsMenuModel *self)
{
	g_return_if_fail(XFDASHBOARD_IS_APPLICATIONS_MENU_MODEL(self));

	XfdashboardApplicationsMenuModelPrivate		*priv=self->priv;

	/* Unset filter (forces all rows being accessible and not being skipped/filtered) */
	clutter_model_set_filter(CLUTTER_MODEL(self), NULL, NULL, NULL);

	/* Remove all rows */
	while(clutter_model_get_n_rows(CLUTTER_MODEL(self))) clutter_model_remove(CLUTTER_MODEL(self), 0);

	/* Clear sections */
	if(priv->sections)
	{
		g_hash_table_destroy(priv->sections);
		priv->sections=NULL;
	}

	/* Destroy root menu */
	if(priv->rootMenu)
	{
		g_object_unref(priv->rootMenu);
		priv->rootMenu=NULL;
	}
}

/* Add a menu to section list */
static void _xfdashboard_applications_menu_model_add_to_section(XfdashboardApplicationsMenuModel *self,
																GarconMenu *inSection,
																GarconMenu *inMenu)
{
	g_return_if_fail(XFDASHBOARD_IS_APPLICATIONS_MENU_MODEL(self));
	g_return_if_fail(GARCON_IS_MENU(inSection));
	g_return_if_fail(GARCON_IS_MENU(inMenu));

	XfdashboardApplicationsMenuModelPrivate		*priv=self->priv;
	GList										*sectionList=NULL;

	/* Get copy of current section list if available */
	if(g_hash_table_contains(priv->sections, inSection))
	{
		GList									*oldSectionList=NULL;

		oldSectionList=(GList*)g_hash_table_lookup(priv->sections, inSection);
		sectionList=g_list_copy(oldSectionList);
	}

	/* Add menu to section list */
	sectionList=g_list_append(sectionList, inMenu);

	/* Store new section list */
	g_hash_table_insert(priv->sections, inSection, sectionList);
}

/* Helper function to filter model data */
static gboolean _xfdashboard_applications_menu_model_filter_by_menu(ClutterModel *inModel,
																	ClutterModelIter *inIter,
																	gpointer inUserData)
{
	g_return_val_if_fail(XFDASHBOARD_IS_APPLICATIONS_MENU_MODEL(inModel), FALSE);
	g_return_val_if_fail(CLUTTER_IS_MODEL_ITER(inIter), FALSE);
	g_return_val_if_fail(GARCON_IS_MENU(inUserData), FALSE);

	GarconMenu									*parentMenu=GARCON_MENU(inUserData);
	GarconMenuElement							*menuElement=NULL;
	GarconMenuItemPool							*itemPool;
	const gchar									*desktopID;
	gboolean									doShow=FALSE;

	/* Get menu element at iterator */
	clutter_model_iter_get(inIter,
							XFDASHBOARD_APPLICATIONS_MENU_MODEL_COLUMN_MENU_ELEMENT, &menuElement,
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
		if(garcon_menu_get_parent(GARCON_MENU(menuElement))==parentMenu) doShow=TRUE;
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

	/* If we get here return TRUE to show model data item or FALSE to hide */
	g_object_unref(menuElement);
	return(doShow);
}

static gboolean _xfdashboard_applications_menu_model_filter_by_section(ClutterModel *inModel,
																		ClutterModelIter *inIter,
																		gpointer inUserData)
{
	g_return_val_if_fail(XFDASHBOARD_IS_APPLICATIONS_MENU_MODEL(inModel), FALSE);
	g_return_val_if_fail(CLUTTER_IS_MODEL_ITER(inIter), FALSE);
	g_return_val_if_fail(GARCON_IS_MENU(inUserData), FALSE);

	XfdashboardApplicationsMenuModel						*self=XFDASHBOARD_APPLICATIONS_MENU_MODEL(inModel);
	XfdashboardApplicationsMenuModelPrivate					*priv=self->priv;
	GarconMenu												*section=GARCON_MENU(inUserData);
	GarconMenuElement										*menuElement=NULL;
	GarconMenu												*parentMenu=NULL;
	GList													*sectionList=NULL;
	gboolean												doShow=FALSE;

	/* Get menu element at iterator */
	clutter_model_iter_get(inIter,
							XFDASHBOARD_APPLICATIONS_MENU_MODEL_COLUMN_MENU_ELEMENT, &menuElement,
							XFDASHBOARD_APPLICATIONS_MENU_MODEL_COLUMN_PARENT_MENU, &parentMenu,
							-1);

	/* If menu element is a menu item check if its parent menu is member of section ... */
	if(menuElement &&
		GARCON_IS_MENU_ITEM(menuElement) &&
		parentMenu)
	{
		sectionList=(GList*)g_hash_table_lookup(priv->sections, section);
		if(sectionList && g_list_index(sectionList, parentMenu)!=-1) doShow=TRUE;
	}

	/* If menu element is a menu check if root menu is parent menu and root menu is requested */
	if(menuElement &&
		GARCON_IS_MENU(menuElement) &&
		parentMenu==priv->rootMenu &&
		section==priv->rootMenu)
	{
		doShow=TRUE;
	}

	/* Release allocated resources */
	g_object_unref(menuElement);
	g_object_unref(parentMenu);

	/* If we get here return TRUE to show model data item or FALSE to hide */
	return(doShow);
}

/* Fill model */
static void _xfdashboard_applications_menu_model_fill_model_collect_menu(XfdashboardApplicationsMenuModel *self,
																			GarconMenu *inMenu,
																			GarconMenu *inSection,
																			guint *ioSequenceID)
{
	g_return_if_fail(XFDASHBOARD_IS_APPLICATIONS_MENU_MODEL(self));
	g_return_if_fail(GARCON_IS_MENU(inMenu));

	XfdashboardApplicationsMenuModelPrivate			*priv=self->priv;
	GList											*menuElements, *entry;
	GarconMenuElement								*menuElement;
	GList											*sectionList=NULL;

	/* Check if this menu is visible and should be processed.
	 * Root menu is an exception as it must be processed or model is empty.
	 */
	if(inMenu!=priv->rootMenu &&
		(garcon_menu_element_get_show_in_environment(GARCON_MENU_ELEMENT(inMenu))==FALSE ||
			garcon_menu_element_get_visible(GARCON_MENU_ELEMENT(inMenu))==FALSE ||
			garcon_menu_element_get_no_display(GARCON_MENU_ELEMENT(inMenu))==TRUE))
	{
		return;
	}

	/* Increase reference on menu going to be processed */
	g_object_ref(inMenu);

	/* Check if menu is a menu item for a section and add section menu itself to the list */
	if(inSection==NULL && garcon_menu_get_parent(inMenu)==priv->rootMenu)
	{
		inSection=inMenu;
		_xfdashboard_applications_menu_model_add_to_section(self, inSection, inMenu);
	}

	/* Iterate through menu and add menu items and sub-menus */
	menuElements=garcon_menu_get_elements(inMenu);
	for(entry=menuElements; entry; entry=g_list_next(entry))
	{
		/* Get menu element from list */
		menuElement=GARCON_MENU_ELEMENT(entry->data);

		/* Check if menu element is visible */
		if(garcon_menu_element_get_show_in_environment(menuElement)==FALSE ||
			garcon_menu_element_get_visible(menuElement)==FALSE ||
			garcon_menu_element_get_no_display(menuElement)==TRUE)
		{
			continue;
		}

		/* Insert row into model if menu element is a sub-menu or a menu item */
		if(GARCON_IS_MENU(menuElement) ||
			GARCON_IS_MENU_ITEM(menuElement))
		{
			const gchar								*title=garcon_menu_element_get_name(menuElement);
			const gchar								*description=garcon_menu_element_get_comment(menuElement);
			const gchar								*icon=garcon_menu_element_get_icon_name(menuElement);
			const gchar								*command=NULL;

			/* If menu element is a menu item, set up command column also */
			if(GARCON_IS_MENU_ITEM(menuElement)) command=garcon_menu_item_get_command(GARCON_MENU_ITEM(menuElement));

			/* Add menu item to model */
			*ioSequenceID=(*ioSequenceID)+1;
			clutter_model_append(CLUTTER_MODEL(self),
									XFDASHBOARD_APPLICATIONS_MENU_MODEL_COLUMN_SEQUENCE_ID, *ioSequenceID,
									XFDASHBOARD_APPLICATIONS_MENU_MODEL_COLUMN_MENU_ELEMENT, menuElement,
									XFDASHBOARD_APPLICATIONS_MENU_MODEL_COLUMN_PARENT_MENU, inMenu,
									XFDASHBOARD_APPLICATIONS_MENU_MODEL_COLUMN_TITLE, title,
									XFDASHBOARD_APPLICATIONS_MENU_MODEL_COLUMN_DESCRIPTION, description,
									XFDASHBOARD_APPLICATIONS_MENU_MODEL_COLUMN_ICON, icon,
									XFDASHBOARD_APPLICATIONS_MENU_MODEL_COLUMN_COMMAND, command,
									-1);
		}

		/* If menu element is a sub-menu call recursively */
		if(GARCON_IS_MENU(menuElement))
		{
			if(inSection) _xfdashboard_applications_menu_model_add_to_section(self, inSection, GARCON_MENU(menuElement));
			_xfdashboard_applications_menu_model_fill_model_collect_menu(self, GARCON_MENU(menuElement), inSection, ioSequenceID);
		}
	}
	g_list_free(menuElements);

	/* Release allocated resources */
	g_object_unref(inMenu);
}

static void _xfdashboard_applications_menu_model_fill_model(XfdashboardApplicationsMenuModel *self)
{
	g_return_if_fail(XFDASHBOARD_IS_APPLICATIONS_MENU_MODEL(self));

	XfdashboardApplicationsMenuModelPrivate		*priv=self->priv;
	GError										*error=NULL;
	guint										sequenceID=0;

	/* Clear model data */
	_xfdashboard_applications_menu_model_clear(self);

	/* Load root menu */
	priv->rootMenu=garcon_menu_new_applications();
	if(garcon_menu_load(priv->rootMenu, NULL, &error)==FALSE)
	{
		g_warning(_("Could not load applications menu: %s"), error ? error->message : _("Unknown error"));
		if(error) g_error_free(error);
		g_object_unref(priv->rootMenu);
		priv->rootMenu=NULL;

		/* Emit "loaded" signal even if it fails */
		g_signal_emit(self, XfdashboardApplicationsMenuModelSignals[SIGNAL_LOADED], 0);
		return;
	}

	/* Iterate through menus recursively to add them to model */
	priv->sections=g_hash_table_new_full(g_direct_hash,
											g_direct_equal,
											NULL,
											(GDestroyNotify)g_list_free);

	_xfdashboard_applications_menu_model_fill_model_collect_menu(self, priv->rootMenu, NULL, &sequenceID);

	/* Emit signal */
	g_signal_emit(self, XfdashboardApplicationsMenuModelSignals[SIGNAL_LOADED], 0);
}

/* Idle callback to fill model */
#include "utils.h"
static gboolean _xfdashboard_applications_menu_model_init_idle(gpointer inUserData)
{
	g_message("%s: user-data=%p (%s)", __func__, inUserData, DEBUG_OBJECT_NAME(inUserData));
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

			case XFDASHBOARD_APPLICATIONS_MENU_MODEL_COLUMN_TITLE:
			case XFDASHBOARD_APPLICATIONS_MENU_MODEL_COLUMN_DESCRIPTION:
			case XFDASHBOARD_APPLICATIONS_MENU_MODEL_COLUMN_ICON:
			case XFDASHBOARD_APPLICATIONS_MENU_MODEL_COLUMN_COMMAND:
				inSortCallback=_xfdashboard_applications_menu_model_resort_string_callback;
				break;

			default:
				g_critical("Sorting column %d without user-defined function is not possible", sortColumn);
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
	if(priv->sections)
	{
		g_hash_table_destroy(priv->sections);
		priv->sections=NULL;
	}

	if(priv->rootMenu)
	{
		g_object_unref(priv->rootMenu);
		priv->rootMenu=NULL;
	}

	/* Call parent's class dispose method */
	G_OBJECT_CLASS(xfdashboard_applications_menu_model_parent_class)->dispose(inObject);
}

/* Class initialization
 * Override functions in parent classes and define properties
 * and signals
 */
void xfdashboard_applications_menu_model_class_init(XfdashboardApplicationsMenuModelClass *klass)
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
void xfdashboard_applications_menu_model_init(XfdashboardApplicationsMenuModel *self)
{
	XfdashboardApplicationsMenuModelPrivate	*priv;

	priv=self->priv=XFDASHBOARD_APPLICATIONS_MENU_MODEL_GET_PRIVATE(self);

	/* Set up default values */
	priv->rootMenu=NULL;
	priv->sections=NULL;

	/* Set up model */
	GType			columnTypes[]=	{
										G_TYPE_UINT, /* XFDASHBOARD_APPLICATIONS_MENU_MODEL_COLUMN_SEQUENCE_ID */
										GARCON_TYPE_MENU_ELEMENT, /* XFDASHBOARD_APPLICATIONS_MENU_MODEL_COLUMN_MENU_ELEMENT */
										GARCON_TYPE_MENU, /* XFDASHBOARD_APPLICATIONS_MENU_MODEL_COLUMN_PARENT_MENU */
										G_TYPE_STRING, /* XFDASHBOARD_APPLICATIONS_MENU_MODEL_COLUMN_TITLE */
										G_TYPE_STRING, /* XFDASHBOARD_APPLICATIONS_MENU_MODEL_COLUMN_DESCRIPTION */
										G_TYPE_STRING, /* XFDASHBOARD_APPLICATIONS_MENU_MODEL_COLUMN_ICON */
										G_TYPE_STRING /* XFDASHBOARD_APPLICATIONS_MENU_MODEL_COLUMN_COMMAND */
									};
	const gchar*	columnNames[]=	{
										_("ID"), /* XFDASHBOARD_APPLICATIONS_MENU_MODEL_COLUMN_SEQUENCE_ID */
										_("Menu item"), /* XFDASHBOARD_APPLICATIONS_MENU_MODEL_COLUMN_MENU_ELEMENT */
										_("Parent menu"), /* XFDASHBOARD_APPLICATIONS_MENU_MODEL_COLUMN_PARENT_MENU */
										_("Title"), /* XFDASHBOARD_APPLICATIONS_MENU_MODEL_COLUMN_TITLE */
										_("Description"), /* XFDASHBOARD_APPLICATIONS_MENU_MODEL_COLUMN_DESCRIPTION */
										_("Icon"), /* XFDASHBOARD_APPLICATIONS_MENU_MODEL_COLUMN_ICON */
										_("Command") /* XFDASHBOARD_APPLICATIONS_MENU_MODEL_COLUMN_COMMAND */
									};

	clutter_model_set_types(CLUTTER_MODEL(self), XFDASHBOARD_APPLICATIONS_MENU_MODEL_COLUMN_LAST, columnTypes);
	clutter_model_set_names(CLUTTER_MODEL(self), XFDASHBOARD_APPLICATIONS_MENU_MODEL_COLUMN_LAST, columnNames);

	/* Defer filling model */
	clutter_threads_add_idle(_xfdashboard_applications_menu_model_init_idle, self);
}

/* Implementation: Public API */

ClutterModel* xfdashboard_applications_menu_model_new(void)
{
	return(CLUTTER_MODEL(g_object_new(XFDASHBOARD_TYPE_APPLICATIONS_MENU_MODEL, NULL)));
}

/* Filter menu items being a direct child item of requested menu */
void xfdashboard_applications_menu_model_filter_by_menu(XfdashboardApplicationsMenuModel *self, GarconMenu *inMenu)
{
	g_return_if_fail(XFDASHBOARD_IS_APPLICATIONS_MENU_MODEL(self));
	g_return_if_fail(inMenu==NULL || GARCON_IS_MENU(inMenu));

	XfdashboardApplicationsMenuModelPrivate		*priv=self->priv;

	/* If menu is NULL filter root menu */
	if(inMenu==NULL) inMenu=priv->rootMenu;

	/* Filter model data */
	clutter_model_set_filter(CLUTTER_MODEL(self), _xfdashboard_applications_menu_model_filter_by_menu, g_object_ref(inMenu), g_object_unref);
}

/* Filter menu items being an indirect child item of requested section */
void xfdashboard_applications_menu_model_filter_by_section(XfdashboardApplicationsMenuModel *self, GarconMenu *inSection)
{
	g_return_if_fail(XFDASHBOARD_IS_APPLICATIONS_MENU_MODEL(self));
	g_return_if_fail(inSection==NULL || GARCON_IS_MENU(inSection));

	XfdashboardApplicationsMenuModelPrivate		*priv=self->priv;

	/* If requested section is NULL filter root menu */
	if(inSection==NULL) inSection=priv->rootMenu;

	/* Filter model data */
	clutter_model_set_filter(CLUTTER_MODEL(self), _xfdashboard_applications_menu_model_filter_by_section, g_object_ref(inSection), (GDestroyNotify)g_object_unref);
}
