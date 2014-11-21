/*
 * applications-search-provider: Search provider for searching installed
 *                               applications
 * 
 * Copyright 2012-2014 Stephan Haller <nomad@froevel.de>
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

#include "applications-search-provider.h"

#include <glib/gi18n-lib.h>
#include <gtk/gtk.h>
#include <gio/gdesktopappinfo.h>

#include "applications-menu-model.h"
#include "application-button.h"
#include "application.h"
#include "drag-action.h"

/* Define this class in GObject system */
G_DEFINE_TYPE(XfdashboardApplicationsSearchProvider,
				xfdashboard_applications_search_provider,
				XFDASHBOARD_TYPE_SEARCH_PROVIDER)

/* Private structure - access only by public API if needed */
#define XFDASHBOARD_APPLICATIONS_SEARCH_PROVIDER_GET_PRIVATE(obj) \
	(G_TYPE_INSTANCE_GET_PRIVATE((obj), XFDASHBOARD_TYPE_APPLICATIONS_SEARCH_PROVIDER, XfdashboardApplicationsSearchProviderPrivate))

struct _XfdashboardApplicationsSearchProviderPrivate
{
	/* Instance related */
	XfdashboardApplicationsMenuModel		*apps;
	GHashTable								*desktopAppInfoCache;
};

/* IMPLEMENTATION: Private variables and methods */
#define DEFAULT_DELIMITERS			"\t\n\r "

typedef struct _XfdashboardApplicationsSearchProviderSearchData		XfdashboardApplicationsSearchProviderSearchData;
struct _XfdashboardApplicationsSearchProviderSearchData
{
	XfdashboardApplicationsMenuModel		*model;
	gchar 									*searchTerm;
};

/* Get desktop application information from cache or load it and store it in cache */
static GDesktopAppInfo* _xfdashboard_applications_search_provider_get_desktop_appinfo(XfdashboardApplicationsSearchProvider *self,
																						const gchar *inDesktopID)
{
	XfdashboardApplicationsSearchProviderPrivate		*priv;
	GDesktopAppInfo										*appInfo;

	g_return_val_if_fail(XFDASHBOARD_IS_APPLICATIONS_SEARCH_PROVIDER(self), NULL);
	g_return_val_if_fail(inDesktopID && *inDesktopID, NULL);

	priv=self->priv;

	/* Check if desktop ID is in cache already */
	if(g_hash_table_lookup_extended(priv->desktopAppInfoCache, inDesktopID, NULL, (gpointer*)&appInfo))
	{
		/* Return application information but increase reference counter */
		return(G_DESKTOP_APP_INFO(g_object_ref(appInfo)));
	}

	/* If we get here the application information for this desktop ID is not in cache.
	 * So load it and store it in cache.
	 */
	appInfo=g_desktop_app_info_new_from_filename(inDesktopID);
	if(!appInfo) return(NULL);

	g_hash_table_insert(priv->desktopAppInfoCache, g_strdup(inDesktopID), appInfo);

	/* Return application information but increase reference counter */
	return(G_DESKTOP_APP_INFO(g_object_ref(appInfo)));
}

/* Clear desktop application informationen cache if menu model has been (re)loaded */
static void _xfdashboard_applications_search_provider_on_apps_model_loaded(XfdashboardApplicationsSearchProvider *self,
																			gpointer inUserData)
{
	XfdashboardApplicationsSearchProviderPrivate	*priv;

	g_return_if_fail(XFDASHBOARD_IS_APPLICATIONS_SEARCH_PROVIDER(self));

	priv=self->priv;

	/* Destroy hash table containin desktop application information if available */
	if(priv->desktopAppInfoCache)
	{
		g_hash_table_destroy(priv->desktopAppInfoCache);
		priv->desktopAppInfoCache=NULL;
	}

	/* Create empty hash table for caching desktop application information */
	priv->desktopAppInfoCache=g_hash_table_new_full(g_str_hash, g_str_equal, g_free, g_object_unref);
}

/* Drag of an menu item begins */
static void _xfdashboard_applications_search_provider_on_drag_begin(ClutterDragAction *inAction,
																	ClutterActor *inActor,
																	gfloat inStageX,
																	gfloat inStageY,
																	ClutterModifierType inModifiers,
																	gpointer inUserData)
{
	const gchar							*desktopName;
	ClutterActor						*dragHandle;
	ClutterStage						*stage;

	g_return_if_fail(CLUTTER_IS_DRAG_ACTION(inAction));
	g_return_if_fail(XFDASHBOARD_IS_APPLICATION_BUTTON(inActor));
	g_return_if_fail(XFDASHBOARD_IS_APPLICATIONS_SEARCH_PROVIDER(inUserData));

	/* Get stage */
	stage=CLUTTER_STAGE(clutter_actor_get_stage(inActor));

	/* Create a application icon for drag handle */
	desktopName=xfdashboard_application_button_get_desktop_filename(XFDASHBOARD_APPLICATION_BUTTON(inActor));

	dragHandle=xfdashboard_application_button_new_from_desktop_file(desktopName);
	clutter_actor_set_position(dragHandle, inStageX, inStageY);
	clutter_actor_add_child(CLUTTER_ACTOR(stage), dragHandle);

	clutter_drag_action_set_drag_handle(inAction, dragHandle);
}

/* Drag of a result item ends */
static void _xfdashboard_applications_search_provider_on_drag_end(ClutterDragAction *inAction,
																	ClutterActor *inActor,
																	gfloat inStageX,
																	gfloat inStageY,
																	ClutterModifierType inModifiers,
																	gpointer inUserData)
{
	ClutterActor					*dragHandle;

	g_return_if_fail(CLUTTER_IS_DRAG_ACTION(inAction));
	g_return_if_fail(XFDASHBOARD_IS_APPLICATION_BUTTON(inActor));
	g_return_if_fail(XFDASHBOARD_IS_APPLICATIONS_SEARCH_PROVIDER(inUserData));

	/* Destroy clone of application icon used as drag handle */
	dragHandle=clutter_drag_action_get_drag_handle(inAction);
	if(dragHandle)
	{
#if CLUTTER_CHECK_VERSION(1, 14, 0)
		/* Only unset drag handle if not running Clutter in version
		 * 1.12. This prevents a critical warning message in 1.12.
		 * Later versions of Clutter are fixed already.
		 */
		clutter_drag_action_set_drag_handle(inAction, NULL);
#endif
		clutter_actor_destroy(dragHandle);
	}
}

/* Check if model data at iterator matches search terms */
static gboolean _xfdashboard_applications_search_provider_is_match(XfdashboardApplicationsSearchProvider *self,
																	ClutterModelIter *inIter,
																	gchar **inSearchTerms,
																	XfdashboardSearchResultSet *inLimitSet,
																	GHashTable *ioPool)
{
	guint					iterRow, poolRow;
	gboolean				isMatch;
	GarconMenuElement		*menuElement;
	gchar					*title, *description;
	const gchar				*command;
	gint					matchesFound, matchesExpected;

	g_return_val_if_fail(XFDASHBOARD_IS_APPLICATIONS_SEARCH_PROVIDER(self), FALSE);
	g_return_val_if_fail(CLUTTER_IS_MODEL_ITER(inIter), FALSE);
	g_return_val_if_fail(!inLimitSet || XFDASHBOARD_IS_SEARCH_RESULT_SET(inLimitSet), FALSE);
	g_return_val_if_fail(ioPool, FALSE);

	isMatch=FALSE;
	menuElement=NULL;

	/* Get menu element at iterator */
	clutter_model_iter_get(inIter,
							XFDASHBOARD_APPLICATIONS_MENU_MODEL_COLUMN_SEQUENCE_ID, &iterRow,
							XFDASHBOARD_APPLICATIONS_MENU_MODEL_COLUMN_MENU_ELEMENT, &menuElement,
							XFDASHBOARD_APPLICATIONS_MENU_MODEL_COLUMN_TITLE, &title,
							XFDASHBOARD_APPLICATIONS_MENU_MODEL_COLUMN_DESCRIPTION, &description,
							-1);
	if(!menuElement) return(FALSE);

	/* Only menu items can be searched */
	if(!GARCON_IS_MENU_ITEM(menuElement))
	{
		g_object_unref(menuElement);
		g_free(title);
		g_free(description);
		return(FALSE);
	}

	/* If a limiting set of row identifiers is provided check if this row
	 * is in this set. If not this menu item can not be a match.
	 */
	if(inLimitSet)
	{
		GFile							*fileDesktopID;
		gchar							*pathDesktopID;
		GVariant						*limitSetRow;
		gint							index;

		/* Set up item to lookup */
		fileDesktopID=garcon_menu_item_get_file(GARCON_MENU_ITEM(menuElement));
		pathDesktopID=g_file_get_path(fileDesktopID);

		limitSetRow=g_variant_new_string(pathDesktopID);

		g_free(pathDesktopID);
		g_object_unref(fileDesktopID);

		/* Lookup item and if not found return FALSE for no match */
		index=xfdashboard_search_result_set_get_index(inLimitSet, limitSetRow);
		g_variant_unref(limitSetRow);

		if(index==-1)
		{
			g_object_unref(menuElement);
			g_free(title);
			g_free(description);
			return(FALSE);
		}
	}

	/* Empty search term matches no menu item */
	if(!inSearchTerms)
	{
		g_object_unref(menuElement);
		g_free(title);
		g_free(description);
		return(FALSE);
	}

	matchesExpected=g_strv_length(inSearchTerms);
	if(matchesExpected==0)
	{
		g_object_unref(menuElement);
		g_free(title);
		g_free(description);
		return(FALSE);
	}

	/* Check if title, description or command matches all search terms */
	command=garcon_menu_item_get_command(GARCON_MENU_ITEM(menuElement));

	matchesFound=0;
	while(*inSearchTerms)
	{
		gboolean						termMatch;
		gchar							*commandPos;

		/* Reset "found" indicator */
		termMatch=FALSE;

		/* Check for current search term */
		if(!termMatch &&
			title &&
			g_strstr_len(title, -1, *inSearchTerms))
		{
			termMatch=TRUE;
		}

		if(!termMatch &&
			description &&
			g_strstr_len(description, -1, *inSearchTerms))
		{
			termMatch=TRUE;
		}

		if(!termMatch && command)
		{
			commandPos=g_strstr_len(command, -1, *inSearchTerms);
			if(commandPos &&
				(commandPos==command || *(commandPos-1)==G_DIR_SEPARATOR))
			{
				termMatch=TRUE;
			}
		}

		/* Increase match counter if we found a match */
		if(termMatch) matchesFound++;

		/* Continue with next search term */
		inSearchTerms++;
	}

	if(matchesFound>=matchesExpected) isMatch=TRUE;

	/* If menu element is a match check if it is a duplicate. It is a duplicate
	 * if uri to desktop file is already in hashtable provided or if the row where the
	 * uri of desktop file was found is not the current row.
	 */
	if(isMatch==TRUE)
	{
		GFile							*fileDesktopID;
		gchar							*pathDesktopID;
		GDesktopAppInfo					*appInfo;

		fileDesktopID=garcon_menu_item_get_file(GARCON_MENU_ITEM(menuElement));
		pathDesktopID=g_file_get_path(fileDesktopID);

		/* Test if desktop file could be loaded and parsed. If not do not add. */
		appInfo=_xfdashboard_applications_search_provider_get_desktop_appinfo(self, pathDesktopID);
		if(appInfo)
		{
			if(g_hash_table_lookup_extended(ioPool, pathDesktopID, NULL, NULL)!=TRUE)
			{
				g_hash_table_insert(ioPool, g_strdup(pathDesktopID), GINT_TO_POINTER(iterRow));
			}
				else
				{
					poolRow=GPOINTER_TO_INT(g_hash_table_lookup(ioPool, pathDesktopID));
					if(poolRow!=iterRow) isMatch=FALSE;
				}

			g_object_unref(appInfo);
		}

		g_free(pathDesktopID);
		g_object_unref(fileDesktopID);
	}

	/* Release allocated resources */
	g_object_unref(menuElement);
	g_free(title);
	g_free(description);

	/* If we get here return TRUE to show model data item or FALSE to hide */
	return(isMatch);
}

/* Callback to sort each item in result set */
static gint _xfdashboard_applications_search_provider_sort_result_set(GVariant *inLeft,
																		GVariant *inRight,
																		gpointer inUserData)
{
	XfdashboardApplicationsSearchProvider				*self;
	const gchar											*leftID;
	const gchar											*rightID;
	GDesktopAppInfo										*leftAppInfo;
	GDesktopAppInfo										*rightAppInfo;
	const gchar											*leftName;
	const gchar											*rightName;
	gchar												*lowerLeftName;
	gchar												*lowerRightName;
	gint												result;

	g_return_val_if_fail(inLeft, 0);
	g_return_val_if_fail(inRight, 0);
	g_return_val_if_fail(XFDASHBOARD_IS_APPLICATIONS_SEARCH_PROVIDER(inUserData), 0);

	self=XFDASHBOARD_APPLICATIONS_SEARCH_PROVIDER(inUserData);

	/* Get desktop IDs of both items */
	leftID=g_variant_get_string(inLeft, NULL);
	rightID=g_variant_get_string(inRight, NULL);

	/* Get desktop application information of both items */
	leftAppInfo=_xfdashboard_applications_search_provider_get_desktop_appinfo(self, leftID);
	if(leftAppInfo) leftName=g_app_info_get_display_name(G_APP_INFO(leftAppInfo));
		else leftName=NULL;

	rightAppInfo=_xfdashboard_applications_search_provider_get_desktop_appinfo(self, rightID);
	if(rightAppInfo) rightName=g_app_info_get_display_name(G_APP_INFO(rightAppInfo));
		else rightName=NULL;

	/* Get result of comparing both desktop application information objects */
	if(leftName) lowerLeftName=g_utf8_strdown(leftName, -1);
		else lowerLeftName=NULL;

	if(rightName) lowerRightName=g_utf8_strdown(rightName, -1);
		else lowerRightName=NULL;

	result=g_strcmp0(lowerLeftName, lowerRightName);

	/* Release allocated resources */
	if(rightAppInfo) g_object_unref(rightAppInfo);
	if(leftAppInfo) g_object_unref(leftAppInfo);
	if(lowerLeftName) g_free(lowerLeftName);
	if(lowerRightName) g_free(lowerRightName);

	/* Return result */
	return(result);
}

/* Callback called for each item in hash-table to set up result set */
static void _xfdashboard_applications_search_provider_build_result_set_from_hashtable(gpointer inKey,
																						gpointer inValue,
																						gpointer inUserData)
{
	XfdashboardSearchResultSet		*resultSet=(XfdashboardSearchResultSet*)inUserData;
	GVariant						*resultItem;

	g_return_if_fail(XFDASHBOARD_IS_SEARCH_RESULT_SET(inUserData));

	/* Create result item and add to result set */
	resultItem=g_variant_new_string(inKey);
	xfdashboard_search_result_set_add_item(resultSet, resultItem);
}

/* IMPLEMENTATION: XfdashboardSearchProvider */

/* Overriden virtual function: get_name */
static const gchar* _xfdashboard_applications_search_provider_get_name(XfdashboardSearchProvider *inProvider)
{
	return(_("Applications"));
}

/* Overriden virtual function: get_icon */
static const gchar* _xfdashboard_applications_search_provider_get_icon(XfdashboardSearchProvider *inProvider)
{
	return(GTK_STOCK_HOME);
}

/* Overriden virtual function: get_result_set */
static XfdashboardSearchResultSet* _xfdashboard_applications_search_provider_get_result_set(XfdashboardSearchProvider *inProvider,
																							const gchar **inSearchTerms,
																							XfdashboardSearchResultSet *inPreviousResultSet)
{
	XfdashboardApplicationsSearchProvider				*self;
	XfdashboardApplicationsSearchProviderPrivate		*priv;
	XfdashboardSearchResultSet							*resultSet;
	ClutterModelIter									*iterator;
	guint												numberTerms;
	gchar												**terms, **termsIter;

	g_return_val_if_fail(XFDASHBOARD_IS_APPLICATIONS_SEARCH_PROVIDER(inProvider), NULL);

	self=XFDASHBOARD_APPLICATIONS_SEARCH_PROVIDER(inProvider);
	priv=self->priv;
	resultSet=NULL;

	/* To perform case-insensitive searches through model convert all search terms
	 * to lower-case before starting search.
	 */
	numberTerms=g_strv_length((gchar**)inSearchTerms);
	if(numberTerms==0)
	{
		/* If we get here no search term is given, return no result set */
		return(NULL);
	}

	terms=g_slice_alloc0(numberTerms*sizeof(gchar*));
	if(!terms)
	{
		g_critical(_("Could not allocate memory to copy search criterias for case-insensitive search"));
		return(NULL);
	}

	termsIter=terms;
	while(*inSearchTerms)
	{
		*termsIter=g_utf8_strdown(*inSearchTerms, -1);
		termsIter++;
		inSearchTerms++;
	}

	/* Perform search */
	iterator=clutter_model_get_first_iter(CLUTTER_MODEL(priv->apps));
	if(iterator && CLUTTER_IS_MODEL_ITER(iterator))
	{
		GHashTable										*pool;

		pool=g_hash_table_new_full(g_str_hash, g_str_equal, g_free, NULL);

		while(!clutter_model_iter_is_last(iterator))
		{
			/* Check for match */
			_xfdashboard_applications_search_provider_is_match(self, iterator, terms, inPreviousResultSet, pool);

			/* Continue with next row in model */
			iterator=clutter_model_iter_next(iterator);
		}

		/* Build result set from pool */
		resultSet=xfdashboard_search_result_set_new();
		g_hash_table_foreach(pool,
								_xfdashboard_applications_search_provider_build_result_set_from_hashtable,
								resultSet);
		xfdashboard_search_result_set_sort(resultSet,
											_xfdashboard_applications_search_provider_sort_result_set,
											self);

		/* Release allocated resources */
		g_hash_table_destroy(pool);
		g_object_unref(iterator);
	}

	/* Release allocated resources */
	if(terms)
	{
		termsIter=terms;
		while(*termsIter)
		{
			g_free(*termsIter);
			termsIter++;
		}
		g_slice_free1(numberTerms*sizeof(gchar*), terms);
	}

	/* Return result set */
	return(resultSet);
}

/* Overriden virtual function: create_result_actor */
static ClutterActor* _xfdashboard_applications_search_provider_create_result_actor(XfdashboardSearchProvider *inProvider,
																					GVariant *inResultItem)
{
	XfdashboardApplicationsSearchProvider			*self;
	ClutterActor									*actor;
	ClutterAction									*dragAction;

	g_return_val_if_fail(XFDASHBOARD_IS_APPLICATIONS_SEARCH_PROVIDER(inProvider), NULL);
	g_return_val_if_fail(inResultItem, NULL);

	self=XFDASHBOARD_APPLICATIONS_SEARCH_PROVIDER(inProvider);

	/* Create actor for menu element */
	actor=xfdashboard_application_button_new_from_desktop_file(g_variant_get_string(inResultItem, NULL));
	clutter_actor_show(actor);

	dragAction=xfdashboard_drag_action_new();
	clutter_drag_action_set_drag_threshold(CLUTTER_DRAG_ACTION(dragAction), -1, -1);
	clutter_actor_add_action(actor, dragAction);
	g_signal_connect(dragAction, "drag-begin", G_CALLBACK(_xfdashboard_applications_search_provider_on_drag_begin), self);
	g_signal_connect(dragAction, "drag-end", G_CALLBACK(_xfdashboard_applications_search_provider_on_drag_end), self);


	/* Return created actor */
	return(actor);
}

/* Overriden virtual function: create_result_actor */
static void _xfdashboard_applications_search_provider_activate_result(XfdashboardSearchProvider* inProvider,
																		GVariant *inResultItem,
																		ClutterActor *inActor,
																		const gchar **inSearchTerms)
{
	XfdashboardApplicationButton		*button;

	g_return_if_fail(XFDASHBOARD_IS_APPLICATIONS_SEARCH_PROVIDER(inProvider));
	g_return_if_fail(XFDASHBOARD_IS_APPLICATION_BUTTON(inActor));

	button=XFDASHBOARD_APPLICATION_BUTTON(inActor);

	/* Launch application */
	if(xfdashboard_application_button_execute(button, NULL))
	{
		/* Launching application seems to be successfuly so quit application */
		xfdashboard_application_quit();
		return;
	}
}

/* IMPLEMENTATION: GObject */

/* Dispose this object */
static void _xfdashboard_applications_search_provider_dispose(GObject *inObject)
{
	XfdashboardApplicationsSearchProvider			*self=XFDASHBOARD_APPLICATIONS_SEARCH_PROVIDER(inObject);
	XfdashboardApplicationsSearchProviderPrivate	*priv=self->priv;

	/* Release allocated resouces */
	if(priv->apps)
	{
		g_object_unref(priv->apps);
		priv->apps=NULL;
	}

	if(priv->desktopAppInfoCache)
	{
		g_hash_table_destroy(priv->desktopAppInfoCache);
		priv->desktopAppInfoCache=NULL;
	}

	/* Call parent's class dispose method */
	G_OBJECT_CLASS(xfdashboard_applications_search_provider_parent_class)->dispose(inObject);
}

/* Class initialization
 * Override functions in parent classes and define properties
 * and signals
 */
static void xfdashboard_applications_search_provider_class_init(XfdashboardApplicationsSearchProviderClass *klass)
{
	XfdashboardSearchProviderClass		*providerClass=XFDASHBOARD_SEARCH_PROVIDER_CLASS(klass);
	GObjectClass						*gobjectClass=G_OBJECT_CLASS(klass);

	/* Override functions */
	gobjectClass->dispose=_xfdashboard_applications_search_provider_dispose;

	providerClass->get_name=_xfdashboard_applications_search_provider_get_name;
	providerClass->get_icon=_xfdashboard_applications_search_provider_get_icon;
	providerClass->get_result_set=_xfdashboard_applications_search_provider_get_result_set;
	providerClass->create_result_actor=_xfdashboard_applications_search_provider_create_result_actor;
	providerClass->activate_result=_xfdashboard_applications_search_provider_activate_result;

	/* Set up private structure */
	g_type_class_add_private(klass, sizeof(XfdashboardApplicationsSearchProviderPrivate));
}

/* Object initialization
 * Create private structure and set up default values
 */
static void xfdashboard_applications_search_provider_init(XfdashboardApplicationsSearchProvider *self)
{
	XfdashboardApplicationsSearchProviderPrivate	*priv;

	self->priv=priv=XFDASHBOARD_APPLICATIONS_SEARCH_PROVIDER_GET_PRIVATE(self);

	/* Set up default values */
	priv->desktopAppInfoCache=NULL;

	priv->apps=XFDASHBOARD_APPLICATIONS_MENU_MODEL(xfdashboard_applications_menu_model_new());
	clutter_model_set_sorting_column(CLUTTER_MODEL(priv->apps), XFDASHBOARD_APPLICATIONS_MENU_MODEL_COLUMN_TITLE);
	g_signal_connect_swapped(priv->apps, "loaded", G_CALLBACK(_xfdashboard_applications_search_provider_on_apps_model_loaded), self);
}
