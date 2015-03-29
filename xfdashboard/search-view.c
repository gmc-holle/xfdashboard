/*
 * search-view: A view showing applications matching search criterias
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

#include "search-view.h"

#include <glib/gi18n-lib.h>
#include <gtk/gtk.h>

#include "search-manager.h"
#include "focusable.h"
#include "utils.h"
#include "search-result-container.h"
#include "click-action.h"
#include "drag-action.h"
#include "focus-manager.h"
#include "enums.h"

/* Define this class in GObject system */
static void _xfdashboard_search_view_focusable_iface_init(XfdashboardFocusableInterface *iface);

G_DEFINE_TYPE_WITH_CODE(XfdashboardSearchView,
						xfdashboard_search_view,
						XFDASHBOARD_TYPE_VIEW,
						G_IMPLEMENT_INTERFACE(XFDASHBOARD_TYPE_FOCUSABLE, _xfdashboard_search_view_focusable_iface_init))

/* Forward declarations */
typedef struct _XfdashboardSearchViewProviderData	XfdashboardSearchViewProviderData;
typedef struct _XfdashboardSearchViewSearchTerms	XfdashboardSearchViewSearchTerms;

/* Private structure - access only by public API if needed */
#define XFDASHBOARD_SEARCH_VIEW_GET_PRIVATE(obj) \
	(G_TYPE_INSTANCE_GET_PRIVATE((obj), XFDASHBOARD_TYPE_SEARCH_VIEW, XfdashboardSearchViewPrivate))

struct _XfdashboardSearchViewPrivate
{
	/* Instance related */
	XfdashboardSearchManager			*searchManager;
	GList								*providers;

	XfdashboardSearchViewSearchTerms	*lastTerms;

	XfdashboardSearchViewProviderData	*selectionProvider;
	guint								repaintID;

	XfdashboardFocusManager				*focusManager;
};

/* Signals */
enum
{
	SIGNAL_SEARCH_RESET,
	SIGNAL_SEARCH_UPDATED,

	SIGNAL_LAST
};

static guint XfdashboardSearchViewSignals[SIGNAL_LAST]={ 0, };

/* IMPLEMENTATION: Private variables and methods */
struct _XfdashboardSearchViewProviderData
{
	gint								refCount;

	XfdashboardSearchProvider			*provider;

	XfdashboardSearchView				*view;
	XfdashboardSearchViewSearchTerms	*lastTerms;
	XfdashboardSearchResultSet			*lastResultSet;

	ClutterActor						*container;
	GHashTable							*mapping;
};

struct _XfdashboardSearchViewSearchTerms
{
	gint								refCount;

	gchar								*termString;
	gchar								**termList;
};

/* Callback to ensure current selection is visible after search results were updated */
static gboolean _xfdashboard_search_view_on_repaint_after_update_callback(gpointer inUserData)
{
	XfdashboardSearchView					*self;
	XfdashboardSearchViewPrivate			*priv;
	ClutterActor							*selection;

	g_return_val_if_fail(XFDASHBOARD_IS_SEARCH_VIEW(inUserData), G_SOURCE_REMOVE);

	self=XFDASHBOARD_SEARCH_VIEW(inUserData);
	priv=self->priv;

	/* Check if this view has a selection set and ensure it is visible */
	selection=xfdashboard_focusable_get_selection(XFDASHBOARD_FOCUSABLE(self));
	if(selection)
	{
		/* Ensure selection is visible */
		xfdashboard_view_child_ensure_visible(XFDASHBOARD_VIEW(self), selection);
	}

	/* Do not call this callback again */
	priv->repaintID=0;
	return(G_SOURCE_REMOVE);
}

/* Helper to clean up a search provider's container by disconnecting all signals to
 * its result items' actors and clean mapping hash table between actors and
 * result items.
 */
static void _xfdashboard_search_view_clean_provider_container(XfdashboardSearchViewProviderData *inProviderData)
{
	g_return_if_fail(inProviderData);

	/* Iterate through mapping, if available, and for each key disconnect signal
	 * from the value (which is a ClutterActor) and take a also an additional
	 * reference at actor because removing it from mapping hash table will unreference
	 * it again.
	 */
	if(inProviderData->mapping)
	{
		GHashTableIter		hashIter;
		GVariant			*key;
		ClutterActor		*value;

		g_hash_table_iter_init(&hashIter, inProviderData->mapping);
		while(g_hash_table_iter_next(&hashIter, (gpointer*)&key, (gpointer*)&value))
		{
			/* First disconnect signal handlers from actor before modifying mapping hash table */
			g_signal_handlers_disconnect_by_data(value, inProviderData);

			/* Take a reference on value (it is the destroyed actor) because removing
			 * key from mapping causes unrefencing key and value.
			 */
			g_object_ref(value);
			g_hash_table_iter_remove(&hashIter);
		}
	}
}

/* Create search term data for a search string */
static XfdashboardSearchViewSearchTerms* _xfdashboard_search_view_search_terms_new(const gchar *inSearchString)
{
	XfdashboardSearchViewSearchTerms	*data;

	/* Create data for provider */
	data=g_new0(XfdashboardSearchViewSearchTerms, 1);
	data->refCount=1;
	data->termString=g_strdup(inSearchString);
	data->termList=xfdashboard_search_manager_get_search_terms_from_string(inSearchString, NULL);

	return(data);
}

/* Free search term data */
static void _xfdashboard_search_view_search_terms_free(XfdashboardSearchViewSearchTerms *inData)
{
	g_return_if_fail(inData);

#if DEBUG
	/* Print a critical warning if more than one references to this object exist.
	 * This is a debug message and should not be translated.
	 */
	if(inData->refCount>1)
	{
		g_critical("Freeing XfdashboardSearchViewSearchTerms at %p with %d references",
					inData,
					inData->refCount);
	}
#endif

	/* Release allocated resources */
	if(inData->termList) g_strfreev(inData->termList);
	if(inData->termString) g_free(inData->termString);
	g_free(inData);
}

/* Increase/decrease reference count for search term data */
static XfdashboardSearchViewSearchTerms* _xfdashboard_search_view_search_terms_ref(XfdashboardSearchViewSearchTerms *inData)
{
	g_return_val_if_fail(inData, NULL);

	inData->refCount++;

	return(inData);
}

static void _xfdashboard_search_view_search_terms_unref(XfdashboardSearchViewSearchTerms *inData)
{
	g_return_if_fail(inData);
	g_return_if_fail(inData->refCount>0);

	inData->refCount--;
	if(inData->refCount==0) _xfdashboard_search_view_search_terms_free(inData);
}

/* Create data for provider */
static XfdashboardSearchViewProviderData* _xfdashboard_search_view_provider_data_new(XfdashboardSearchView *self,
																						GType inProviderType)
{
	XfdashboardSearchViewProviderData	*data;

	g_return_val_if_fail(XFDASHBOARD_IS_SEARCH_VIEW(self), NULL);
	g_return_val_if_fail(inProviderType!=XFDASHBOARD_TYPE_SEARCH_PROVIDER && g_type_is_a(inProviderType, XFDASHBOARD_TYPE_SEARCH_PROVIDER), NULL);

	/* Create data for provider */
	data=g_new0(XfdashboardSearchViewProviderData, 1);
	data->refCount=1;
	data->view=self;
	data->lastTerms=NULL;
	data->lastResultSet=NULL;
	data->container=NULL;
	data->mapping=g_hash_table_new_full(g_variant_hash,
										g_variant_equal,
										(GDestroyNotify)g_variant_unref,
										(GDestroyNotify)g_object_unref);
	data->provider=XFDASHBOARD_SEARCH_PROVIDER(g_object_new(inProviderType, NULL));

	return(data);
}

/* Free data for provider */
static void _xfdashboard_search_view_provider_data_free(XfdashboardSearchViewProviderData *inData)
{
	g_return_if_fail(inData);

#if DEBUG
	/* Print a critical warning if more than one references to this object exist.
	 * This is a debug message and should not be translated.
	 */
	if(inData->refCount>1)
	{
		g_critical("Freeing XfdashboardSearchViewProviderData at %p with %d references",
					inData,
					inData->refCount);
	}
#endif

	/* Clean provider's container and move focus if necessary */
	_xfdashboard_search_view_clean_provider_container(inData);

	/* Destroy container */
	if(inData->container)
	{
		/* First disconnect signal handlers from actor before destroying container */
		g_signal_handlers_disconnect_by_data(inData->container, inData);

		/* Destroy container */
		clutter_actor_destroy(inData->container);
		inData->container=NULL;
	}

	/* Release allocated resources */
	if(inData->mapping) g_hash_table_unref(inData->mapping);
	if(inData->lastResultSet) g_object_unref(inData->lastResultSet);
	if(inData->lastTerms) _xfdashboard_search_view_search_terms_unref(inData->lastTerms);
	if(inData->provider) g_object_unref(inData->provider);
	g_free(inData);
}

/* Increase/decrease reference count for data of requested provider */
static XfdashboardSearchViewProviderData* _xfdashboard_search_view_provider_data_ref(XfdashboardSearchViewProviderData *inData)
{
	g_return_val_if_fail(inData, NULL);

	inData->refCount++;

	return(inData);
}

static void _xfdashboard_search_view_provider_data_unref(XfdashboardSearchViewProviderData *inData)
{
	g_return_if_fail(inData);
	g_return_if_fail(inData->refCount>0);

	inData->refCount--;
	if(inData->refCount==0) _xfdashboard_search_view_provider_data_free(inData);
}

/* Find data for requested provider type */
static XfdashboardSearchViewProviderData* _xfdashboard_search_view_get_provider_data(XfdashboardSearchView *self,
																						GType inProviderType)
{
	XfdashboardSearchViewPrivate		*priv;
	XfdashboardSearchViewProviderData	*data;
	GList								*iter;

	g_return_val_if_fail(XFDASHBOARD_IS_SEARCH_VIEW(self), NULL);

	priv=self->priv;

	/* Iterate through list of provider data and lookup requested one */
	for(iter=priv->providers; iter; iter=g_list_next(iter))
	{
		data=(XfdashboardSearchViewProviderData*)iter->data;

		if(data->provider &&
			G_OBJECT_TYPE(data->provider)==inProviderType)
		{
			return(_xfdashboard_search_view_provider_data_ref(data));
		}
	}

	/* If we get here we did not find data for requested provider */
	return(NULL);
}

/* Find data of provider by its child actor */
static XfdashboardSearchViewProviderData* _xfdashboard_search_view_get_provider_data_by_actor(XfdashboardSearchView *self,
																								ClutterActor *inChild)
{
	XfdashboardSearchViewPrivate		*priv;
	XfdashboardSearchViewProviderData	*data;
	GList								*iter;
	ClutterActor						*container;

	g_return_val_if_fail(XFDASHBOARD_IS_SEARCH_VIEW(self), NULL);
	g_return_val_if_fail(CLUTTER_IS_ACTOR(inChild), NULL);

	priv=self->priv;

	/* Find container for requested child */
	container=inChild;
	while(container && !XFDASHBOARD_IS_SEARCH_RESULT_CONTAINER(container))
	{
		/* Current child is not a container so try next with parent actor */
		container=clutter_actor_get_parent(container);
	}

	if(!container)
	{
		/* Container for requested child was not found */
		g_debug("Did not find container for actor %p of type %s",
					inChild,
					G_OBJECT_TYPE_NAME(inChild));

		return(NULL);
	}

	/* Iterate through list of provider data and lookup found container */
	for(iter=priv->providers; iter; iter=g_list_next(iter))
	{
		data=(XfdashboardSearchViewProviderData*)iter->data;

		if(data->provider &&
			data->container==container)
		{
			return(_xfdashboard_search_view_provider_data_ref(data));
		}
	}

	/* If we get here we did not find data of provider for requested actor */
	return(NULL);
}

/* A search provider was registered */
static void _xfdashboard_search_view_on_search_provider_registered(XfdashboardSearchView *self,
																	GType inProviderType,
																	gpointer inUserData)
{
	XfdashboardSearchViewPrivate		*priv;
	XfdashboardSearchViewProviderData	*data;

	g_return_if_fail(XFDASHBOARD_IS_SEARCH_VIEW(self));

	priv=self->priv;

	/* Register search provider if not already registered */
	data=_xfdashboard_search_view_get_provider_data(self, inProviderType);
	if(!data)
	{
		/* Create data for new search provider registered
		 * and add to list of active search providers.
		 */
		data=_xfdashboard_search_view_provider_data_new(self, inProviderType);
		priv->providers=g_list_prepend(priv->providers, data);

		g_debug("Created search provider %s of type %s in %s",
					xfdashboard_search_provider_get_name(data->provider),
					G_OBJECT_TYPE_NAME(data->provider),
					G_OBJECT_TYPE_NAME(self));
	}
		else _xfdashboard_search_view_provider_data_unref(data);
}

/* A search provider was unregistered */
static void _xfdashboard_search_view_on_search_provider_unregistered(XfdashboardSearchView *self,
																		GType inProviderType,
																		gpointer inUserData)
{
	XfdashboardSearchViewPrivate		*priv;
	XfdashboardSearchViewProviderData	*data;
	GList								*iter;

	g_return_if_fail(XFDASHBOARD_IS_SEARCH_VIEW(self));

	priv=self->priv;

	/* Unregister search provider if it was registered before */
	data=_xfdashboard_search_view_get_provider_data(self, inProviderType);
	if(data)
	{
		g_debug("Unregistering search provider %s of type %s in %s",
					xfdashboard_search_provider_get_name(data->provider),
					G_OBJECT_TYPE_NAME(data->provider),
					G_OBJECT_TYPE_NAME(self));

		/* Find data of unregistered search provider in list of
		 * active search providers to remove it from that list.
		 */
		iter=g_list_find(priv->providers, data);
		if(iter) priv->providers=g_list_delete_link(priv->providers, iter);

		/* Free provider data */
		_xfdashboard_search_view_provider_data_unref(data);
	}
}

/* A result item actor was clicked */
static void _xfdashboard_search_view_on_provider_item_actor_clicked(XfdashboardClickAction *inAction,
																	ClutterActor *inActor,
																	gpointer inUserData)
{
	XfdashboardSearchView				*self;
	XfdashboardSearchViewPrivate		*priv;
	XfdashboardSearchViewProviderData	*providerData;
	GHashTableIter						iter;
	GVariant							*key;
	ClutterActor						*value;
	const gchar							**searchTerms;

	g_return_if_fail(CLUTTER_IS_ACTOR(inActor));
	g_return_if_fail(inUserData);

	providerData=(XfdashboardSearchViewProviderData*)inUserData;

	/* Iterate through mapping and at first key whose value matching actor
	 * tell provider to activate item.
	 */
	g_hash_table_iter_init(&iter, providerData->mapping);
	while(g_hash_table_iter_next(&iter, (gpointer*)&key, (gpointer*)&value))
	{
		/* If value of key iterated matches actor activate item */
		if(value==inActor)
		{
			/* Get search view and private data of view */
			self=providerData->view;
			priv=self->priv;

			/* Get search terms to pass them to search provider */
			searchTerms=NULL;
			if(priv->lastTerms) searchTerms=(const gchar**)priv->lastTerms->termList;

			/* Tell provider that a result item was clicked */
			xfdashboard_search_provider_activate_result(providerData->provider,
															key,
															inActor,
															searchTerms);

			/* All done so return here */
			return;
		}
	}
}

/* A provider icon was clicked */
static void _xfdashboard_search_view_on_provider_icon_clicked(XfdashboardSearchResultContainer *inContainer,
																gpointer inUserData)
{
	XfdashboardSearchView				*self;
	XfdashboardSearchViewPrivate		*priv;
	XfdashboardSearchViewProviderData	*providerData;
	const gchar							**searchTerms;

	g_return_if_fail(XFDASHBOARD_IS_SEARCH_RESULT_CONTAINER(inContainer));
	g_return_if_fail(inUserData);

	providerData=(XfdashboardSearchViewProviderData*)inUserData;

	/* Get search view and private data of view */
	self=providerData->view;
	priv=self->priv;

	/* Get search terms to pass them to search provider */
	searchTerms=NULL;
	if(priv->lastTerms) searchTerms=(const gchar**)priv->lastTerms->termList;

	/* Tell provider to launch search */
	xfdashboard_search_provider_launch_search(providerData->provider, searchTerms);
}

/* A container of a provider is going to be destroyed */
static void _xfdashboard_search_view_on_provider_container_destroyed(ClutterActor *inActor, gpointer inUserData)
{
	XfdashboardSearchView				*self;
	XfdashboardSearchViewPrivate		*priv;
	XfdashboardSearchViewProviderData	*providerData;

	g_return_if_fail(XFDASHBOARD_IS_SEARCH_RESULT_CONTAINER(inActor));
	g_return_if_fail(inUserData);

	providerData=(XfdashboardSearchViewProviderData*)inUserData;

	/* Get search view and private data of view */
	self=providerData->view;
	priv=self->priv;

	/* Move selection to first selectable actor at next available container
	 * if this provider whose container to destroy is the currently selected one.
	 * This avoids reselecting the next available actor in container when
	 * we container's children will get destroyed (one of the actors is the
	 * current selection and from then on reselection will happen.)
	 */
	if(priv->selectionProvider==providerData)
	{
		ClutterActor						*oldSelection;
		ClutterActor						*newSelection;
		XfdashboardSearchViewProviderData	*newSelectionProvider;
		GList								*currentProviderIter;
		GList								*iter;
		XfdashboardSearchViewProviderData	*iterProviderData;

		newSelection=NULL;
		newSelectionProvider=NULL;

		/* Find position of currently selected provider in the list of providers */
		currentProviderIter=NULL;
		for(iter=priv->providers; iter && !currentProviderIter; iter=g_list_next(iter))
		{
			iterProviderData=(XfdashboardSearchViewProviderData*)iter->data;

			/* Check if provider at iterator is the one we want to find */
			if(iterProviderData &&
				iterProviderData->provider==priv->selectionProvider->provider)
			{
				currentProviderIter=iter;
			}
		}

		/* To find next provider with existing container and a selectable actor
		 * after the currently selected one iterate forwards from the found position.
		 * If we find a match set the new selection to first selectable actor
		 * at found provider.
		 */
		for(iter=g_list_next(currentProviderIter); iter && !newSelection; iter=g_list_next(iter))
		{
			iterProviderData=(XfdashboardSearchViewProviderData*)iter->data;

			/* Check if provider at iterator has a container and a selectable actor */
			if(iterProviderData &&
				iterProviderData->container)
			{
				ClutterActor				*selectableActor;

				selectableActor=xfdashboard_search_result_container_find_selection(XFDASHBOARD_SEARCH_RESULT_CONTAINER(iterProviderData->container),
																					NULL,
																					XFDASHBOARD_SELECTION_TARGET_FIRST,
																					XFDASHBOARD_VIEW(self));
				if(selectableActor)
				{
					newSelection=selectableActor;
					newSelectionProvider=iterProviderData;
				}
			}
		}

		/* If we did not find a match when iterating forwards from found position,
		 * then do the same but iterate backwards from the found position.
		 */
		for(iter=g_list_previous(currentProviderIter); iter && !newSelection; iter=g_list_previous(iter))
		{
			iterProviderData=(XfdashboardSearchViewProviderData*)iter->data;

			/* Check if provider at iterator has a container and a selectable actor */
			if(iterProviderData &&
				iterProviderData->container)
			{
				ClutterActor				*selectableActor;

				selectableActor=xfdashboard_search_result_container_find_selection(XFDASHBOARD_SEARCH_RESULT_CONTAINER(iterProviderData->container),
																					NULL,
																					XFDASHBOARD_SELECTION_TARGET_FIRST,
																					XFDASHBOARD_VIEW(self));
				if(selectableActor)
				{
					newSelection=selectableActor;
					newSelectionProvider=iterProviderData;
				}
			}
		}

		/* If we still do not find a match the new selection is NULL because it
		 * was initialized with. So new selection will set to NULL which means
		 * nothing is selected anymore. Otherwise new selection contains the
		 * new selection found and will be set.
		 */
		oldSelection=xfdashboard_focusable_get_selection(XFDASHBOARD_FOCUSABLE(self));
		g_debug("Container of provider %s is destroyed but holds current selection %p of type %s - so selecting %p of type %s of provider %s",
					providerData->provider ? G_OBJECT_TYPE_NAME(providerData->provider) : "<nil>",
					oldSelection, oldSelection ? G_OBJECT_TYPE_NAME(oldSelection) : "<nil>",
					newSelection, newSelection ? G_OBJECT_TYPE_NAME(newSelection) : "<nil>",
					newSelectionProvider && newSelectionProvider->provider ? G_OBJECT_TYPE_NAME(newSelectionProvider->provider) : "<nil>");

		xfdashboard_focusable_set_selection(XFDASHBOARD_FOCUSABLE(self), newSelection);
	}

	/* Clean provider's container and move focus if necessary */
	_xfdashboard_search_view_clean_provider_container(providerData);

	/* Container will be destroyed so unset pointer to it at provider */
	providerData->container=NULL;
}

/* A result item actor is going to be destroyed */
static void _xfdashboard_search_view_on_provider_item_actor_destroy(ClutterActor *inActor, gpointer inUserData)
{
	XfdashboardSearchViewProviderData	*providerData;
	GHashTableIter						iter;
	GVariant							*key;
	ClutterActor						*value;

	g_return_if_fail(CLUTTER_IS_ACTOR(inActor));
	g_return_if_fail(inUserData);

	providerData=(XfdashboardSearchViewProviderData*)inUserData;

	/* First disconnect signal handlers from actor before modifying mapping hash table */
	g_signal_handlers_disconnect_by_data(inActor, providerData);

	/* Iterate through mapping and remove each key whose value matches actor
	 * going to be destroyed and remove it from mapping hash table.
	 */
	g_hash_table_iter_init(&iter, providerData->mapping);
	while(g_hash_table_iter_next(&iter, (gpointer*)&key, (gpointer*)&value))
	{
		/* If value of key iterated matches destroyed actor remove it from mapping */
		if(value==inActor)
		{
			/* Take a reference on value (it is the destroyed actor) because removing
			 * key from mapping causes unrefencing key and value.
			 */
			g_object_ref(inActor);
			g_hash_table_iter_remove(&iter);
		}
	}
}

/* Updates container of provider with new result set from a last search.
 * Also creates or destroys the container for search provider if needed.
 */
static ClutterActor* _xfdashboard_search_view_update_provider_actor_new(XfdashboardSearchView *self,
																			XfdashboardSearchViewProviderData *inProviderData,
																			GVariant *inItem)
{
	ClutterActor		*actor;
	ClutterAction		*action;
	GList				*actions;
	GList				*actionsIter;

	g_return_val_if_fail(XFDASHBOARD_IS_SEARCH_VIEW(self), NULL);
	g_return_val_if_fail(inProviderData, NULL);
	g_return_val_if_fail(inItem, NULL);

	/* Create actor for item */
	actor=xfdashboard_search_provider_create_result_actor(inProviderData->provider, inItem);
	if(!actor)
	{
		gchar			*itemText;

		itemText=g_variant_print(inItem, TRUE);
		g_warning(_("Failed to add actor for result item %s of provider %s: Could not create actor"),
					itemText,
					G_OBJECT_TYPE_NAME(inProviderData->provider));
		g_free(itemText);

		return(NULL);
	}

	if(!CLUTTER_IS_ACTOR(actor))
	{
		gchar			*itemText;

		itemText=g_variant_print(inItem, TRUE);
		g_critical(_("Failed to add actor for result item %s of provider %s: Actor of type %s is not derived from class %s"),
					itemText,
					G_OBJECT_TYPE_NAME(inProviderData->provider),
					G_IS_OBJECT(actor) ? G_OBJECT_TYPE_NAME(actor) : "<unknown>",
					g_type_name(CLUTTER_TYPE_ACTOR));
		g_free(itemText);

		/* Release allocated resources */
		clutter_actor_destroy(actor);

		return(NULL);
	}

	/* Connect to 'destroy' signal of actor to remove it from mapping hash table
	 * if actor was destroyed (accidently)
	 */
	g_signal_connect(actor,
						"destroy",
						G_CALLBACK(_xfdashboard_search_view_on_provider_item_actor_destroy),
						inProviderData);

	/* Add click action to actor and connect signal */
	action=xfdashboard_click_action_new();
	clutter_actor_add_action(actor, action);
	g_signal_connect(action,
						"clicked",
						G_CALLBACK(_xfdashboard_search_view_on_provider_item_actor_clicked),
						inProviderData);

	/* Iterate through all actions of actor and if it has an action of type
	 * XfdashboardDragAction and has no source set then set this view as source
	 */
	actions=clutter_actor_get_actions(actor);
	for(actionsIter=actions; actionsIter; actionsIter=g_list_next(actionsIter))
	{
		if(XFDASHBOARD_IS_DRAG_ACTION(actionsIter->data) &&
			!xfdashboard_drag_action_get_source(XFDASHBOARD_DRAG_ACTION(actionsIter->data)))
		{
			g_object_set(actionsIter->data, "source", self, NULL);
		}
	}
	if(actions) g_list_free(actions);

	/* Return newly created and set up actor */
	return(actor);
}

static void _xfdashboard_search_view_update_provider_container(XfdashboardSearchView *self,
																XfdashboardSearchViewProviderData *inProviderData,
																XfdashboardSearchResultSet *inNewResultSet)
{
	g_return_if_fail(XFDASHBOARD_IS_SEARCH_VIEW(self));
	g_return_if_fail(inProviderData);
	g_return_if_fail(!inNewResultSet || XFDASHBOARD_IS_SEARCH_RESULT_SET(inNewResultSet));

	/* If result set for provider is given then check if we need to create a container
	 * or if we have to update one ...
	 */
	if(inNewResultSet &&
		xfdashboard_search_result_set_get_size(inNewResultSet)>0)
	{
		GList					*allList;
		GList					*removeList;
		GList					*iter;
		ClutterActor			*actor;
		ClutterActor			*lastActor;

		/* Create container for provider if it does not exist yet */
		if(!inProviderData->container)
		{
			actor=xfdashboard_search_result_container_new(inProviderData->provider);
			if(!actor) return;

			inProviderData->container=actor;
			clutter_actor_add_child(CLUTTER_ACTOR(self), inProviderData->container);

			g_signal_connect(inProviderData->container,
								"icon-clicked",
								G_CALLBACK(_xfdashboard_search_view_on_provider_icon_clicked),
								inProviderData);

			g_signal_connect(inProviderData->container,
								"destroy",
								G_CALLBACK(_xfdashboard_search_view_on_provider_container_destroyed),
								inProviderData);
		}

		/* Get list of all items in result set for and a list of items whose actors
		 * to remove from container.
		 */
		allList=xfdashboard_search_result_set_get_all(inNewResultSet);

		removeList=NULL;
		if(inProviderData->lastResultSet)
		{
			removeList=xfdashboard_search_result_set_complement(inNewResultSet, inProviderData->lastResultSet);
		}

		/* Create actor for each item in list which is new to mapping */
		lastActor=NULL;
		for(iter=allList; iter; iter=g_list_next(iter))
		{
			GVariant			*item;

			/* Get item to add */
			item=(GVariant*)iter->data;

			/* If item does not exist in mapping then create actor and add it to mapping */
			if(!g_hash_table_lookup_extended(inProviderData->mapping, item, NULL, (gpointer*)&actor))
			{
				actor=_xfdashboard_search_view_update_provider_actor_new(self, inProviderData, item);
				if(actor)
				{
					/* Add newly created actor to container of provider */
					xfdashboard_search_result_container_add_result_actor(XFDASHBOARD_SEARCH_RESULT_CONTAINER(inProviderData->container),
																			actor,
																			lastActor);
					g_hash_table_insert(inProviderData->mapping, g_variant_ref(item), g_object_ref(actor));
				}
			}

			/* Remember either existing actor from hash table lookup or
			 * the newly created actor as the last one seen.
			 */
			if(actor) lastActor=actor;
		}

		/* Remove actors for each item in list of removing items */
		for(iter=removeList; iter; iter=g_list_next(iter))
		{
			GVariant			*item;

			/* Get item to add */
			item=(GVariant*)iter->data;

			/* Get actor to remove */
			if(g_hash_table_lookup_extended(inProviderData->mapping, item, NULL, (gpointer*)&actor))
			{
				if(!CLUTTER_IS_ACTOR(actor))
				{
					gchar		*itemText;

					itemText=g_variant_print(item, TRUE);
					g_critical(_("Failed to remove actor for result item %s of provider %s: Actor of type %s is not derived from class %s"),
								itemText,
								G_OBJECT_TYPE_NAME(inProviderData->provider),
								G_IS_OBJECT(actor) ? G_OBJECT_TYPE_NAME(actor) : "<unknown>",
								g_type_name(CLUTTER_TYPE_ACTOR));
					g_free(itemText);

					continue;
				}

				/* First disconnect signal handlers from actor before modifying mapping hash table */
				g_signal_handlers_disconnect_by_data(actor, inProviderData);

				/* Remove actor from mapping hash table before destroying it */
				g_hash_table_remove(inProviderData->mapping, item);

				/* Destroy actor and remove from hash table */
				clutter_actor_destroy(actor);
			}
		}

		/* Release allocated resources */
		if(removeList) g_list_free_full(removeList, (GDestroyNotify)g_variant_unref);
		if(allList) g_list_free_full(allList, (GDestroyNotify)g_variant_unref);
	}
		/* ... but if no result set for provider is given then destroy existing container */
		else
		{
			/* Clean provider's container and move focus if necessary */
			_xfdashboard_search_view_clean_provider_container(inProviderData);

			/* Destroy container */
			if(inProviderData->container)
			{
				/* First disconnect signal handlers from actor before destroying container */
				g_signal_handlers_disconnect_by_data(inProviderData->container, inProviderData);

				/* Destroy container */
				clutter_actor_destroy(inProviderData->container);
				inProviderData->container=NULL;
			}
		}

	/* Remember new result set for search provider */
	if(inProviderData->lastResultSet)
	{
		g_object_unref(inProviderData->lastResultSet);
		inProviderData->lastResultSet=NULL;
	}

	if(inNewResultSet) inProviderData->lastResultSet=g_object_ref(inNewResultSet);
}

/* Check if we can perform an incremental search at search provider for requested search terms */
static gboolean _xfdashboard_search_view_can_do_incremental_search(XfdashboardSearchViewSearchTerms *inProviderLastTerms,
																	XfdashboardSearchViewSearchTerms *inCurrentSearchTerms)
{
	gchar						**iterProvider;
	gchar						**iterCurrent;

	g_return_val_if_fail(inCurrentSearchTerms, FALSE);

	/* If no last search terms for search provider was provided
	 * then return FALSE to perform full search.
	 */
	if(!inProviderLastTerms) return(FALSE);

	/* Check for incremental search. An incremental search can be done
	 * if the last search terms for a search provider is given, the order
	 * in last search terms of search provider and the current search terms
	 * has not changed and each term in both search terms is a case-sensitive
	 * prefix of the term previously used.
	 */
	iterProvider=inProviderLastTerms->termList;
	iterCurrent=inCurrentSearchTerms->termList;
	while(*iterProvider && *iterCurrent)
	{
		if(g_strcmp0(*iterProvider, *iterCurrent)>0) return(FALSE);

		iterProvider++;
		iterCurrent++;
	}

	/* If we are at end of list of terms in both search term
	 * then both terms list are equal and return TRUE here.
	 */
	if(!(*iterProvider) && !(*iterCurrent)) return(TRUE);

	/* If we get here both terms list the criterias do not match
	 * and an incremental search cannot be done. Return FALSE
	 * to indicate that a full search is needed.
	 */
	return(FALSE);
}

/* IMPLEMENTATION: Interface XfdashboardFocusable */

/* Determine if actor can get the focus */
static gboolean _xfdashboard_search_view_focusable_can_focus(XfdashboardFocusable *inFocusable)
{
	XfdashboardSearchView			*self;
	XfdashboardFocusableInterface	*selfIface;
	XfdashboardFocusableInterface	*parentIface;

	g_return_val_if_fail(XFDASHBOARD_IS_FOCUSABLE(inFocusable), FALSE);
	g_return_val_if_fail(XFDASHBOARD_IS_SEARCH_VIEW(inFocusable), FALSE);

	self=XFDASHBOARD_SEARCH_VIEW(inFocusable);

	/* Call parent class interface function */
	selfIface=XFDASHBOARD_FOCUSABLE_GET_IFACE(inFocusable);
	parentIface=g_type_interface_peek_parent(selfIface);

	if(parentIface && parentIface->can_focus)
	{
		if(!parentIface->can_focus(inFocusable)) return(FALSE);
	}

	/* If this view is not enabled it is not focusable */
	if(!xfdashboard_view_get_enabled(XFDASHBOARD_VIEW(self))) return(FALSE);

	// TODO: We can only focus this view if at least one provider has an existing container

	/* If we get here this actor can be focused */
	return(TRUE);
}

/* Determine if this actor supports selection */
static gboolean _xfdashboard_search_view_focusable_supports_selection(XfdashboardFocusable *inFocusable)
{
	g_return_val_if_fail(XFDASHBOARD_IS_FOCUSABLE(inFocusable), FALSE);
	g_return_val_if_fail(XFDASHBOARD_IS_SEARCH_VIEW(inFocusable), FALSE);

	/* This actor supports selection */
	return(TRUE);
}

/* Get current selection */
static ClutterActor* _xfdashboard_search_view_focusable_get_selection(XfdashboardFocusable *inFocusable)
{
	XfdashboardSearchView			*self;
	XfdashboardSearchViewPrivate	*priv;

	g_return_val_if_fail(XFDASHBOARD_IS_FOCUSABLE(inFocusable), NULL);
	g_return_val_if_fail(XFDASHBOARD_IS_SEARCH_VIEW(inFocusable), NULL);

	self=XFDASHBOARD_SEARCH_VIEW(inFocusable);
	priv=self->priv;

	/* If we have no provider selected (the selection for this view) or
	 * if no container exists then return NULL for no selection.
	 */
	if(!priv->selectionProvider ||
		!priv->selectionProvider->container)
	{
		return(NULL);
	}

	/* Return current selection of selected provider's container */
	return(xfdashboard_search_result_container_get_selection(XFDASHBOARD_SEARCH_RESULT_CONTAINER(priv->selectionProvider->container)));
}

/* Set new selection */
static gboolean _xfdashboard_search_view_focusable_set_selection(XfdashboardFocusable *inFocusable,
																	ClutterActor *inSelection)
{
	XfdashboardSearchView					*self;
	XfdashboardSearchViewPrivate			*priv;
	XfdashboardSearchViewProviderData		*data;
	gboolean								success;

	g_return_val_if_fail(XFDASHBOARD_IS_FOCUSABLE(inFocusable), FALSE);
	g_return_val_if_fail(XFDASHBOARD_IS_SEARCH_VIEW(inFocusable), FALSE);
	g_return_val_if_fail(!inSelection || CLUTTER_IS_ACTOR(inSelection), FALSE);

	self=XFDASHBOARD_SEARCH_VIEW(inFocusable);
	priv=self->priv;
	success=FALSE;

	/* If selection to set is NULL, reset internal variables and selection at current selected
	 * container and return TRUE.
	 */
	if(!inSelection)
	{
		/* Reset selection at container of currently selected provider */
		if(priv->selectionProvider &&
			priv->selectionProvider->container)
		{
			xfdashboard_search_result_container_set_selection(XFDASHBOARD_SEARCH_RESULT_CONTAINER(priv->selectionProvider->container), NULL);
		}

		/* Reset internal variables */
		if(priv->selectionProvider)
		{
			_xfdashboard_search_view_provider_data_unref(priv->selectionProvider);
			priv->selectionProvider=NULL;
		}

		/* Return success */
		return(TRUE);
	}

	/* Find data of provider for requested selected actor */
	data=_xfdashboard_search_view_get_provider_data_by_actor(self, inSelection);
	if(!data)
	{
		g_warning(_("%s is not a child of any provider at %s and cannot be selected"),
					G_OBJECT_TYPE_NAME(inSelection),
					G_OBJECT_TYPE_NAME(self));

		return(FALSE);
	}

	/* Set selection at container of provider */
	if(data->container)
	{
		success=xfdashboard_search_result_container_set_selection(XFDASHBOARD_SEARCH_RESULT_CONTAINER(data->container),
																	inSelection);

		/* If we could set selection successfully remember its provider and ensure
		 * that selection is visible.
		 */
		if(success)
		{
			if(priv->selectionProvider)
			{
				_xfdashboard_search_view_provider_data_unref(priv->selectionProvider);
				priv->selectionProvider=NULL;
			}

			priv->selectionProvider=_xfdashboard_search_view_provider_data_ref(data);

			/* Ensure new selection is visible */
			xfdashboard_view_child_ensure_visible(XFDASHBOARD_VIEW(self), inSelection);
		}
	}

	/* Release allocated resources */
	_xfdashboard_search_view_provider_data_unref(data);

	/* Return success result */
	return(success);
}

/* Find requested selection target depending of current selection */
static ClutterActor* _xfdashboard_search_view_focusable_find_selection(XfdashboardFocusable *inFocusable,
																				ClutterActor *inSelection,
																				XfdashboardSelectionTarget inDirection)
{
	XfdashboardSearchView					*self;
	XfdashboardSearchViewPrivate			*priv;
	ClutterActor							*newSelection;
	XfdashboardSearchViewProviderData		*newSelectionProvider;
	GList									*currentProviderIter;
	GList									*iter;
	XfdashboardSearchViewProviderData		*providerData;

	g_return_val_if_fail(XFDASHBOARD_IS_FOCUSABLE(inFocusable), NULL);
	g_return_val_if_fail(XFDASHBOARD_IS_SEARCH_VIEW(inFocusable), NULL);
	g_return_val_if_fail(!inSelection || CLUTTER_IS_ACTOR(inSelection), NULL);
	g_return_val_if_fail(inDirection>XFDASHBOARD_SELECTION_TARGET_NONE, NULL);
	g_return_val_if_fail(inDirection<=XFDASHBOARD_SELECTION_TARGET_NEXT, NULL);

	self=XFDASHBOARD_SEARCH_VIEW(inFocusable);
	priv=self->priv;
	newSelection=NULL;
	newSelectionProvider=NULL;

	/* If nothing is selected, select the first selectable actor of the first provider
	 * having an existing container.
	 */
	if(!inSelection)
	{
		/* Find first provider having an existing container and having a selectable
		 * actor in its container.
		 */
		for(iter=priv->providers; iter && !newSelection; iter=g_list_next(iter))
		{
			providerData=(XfdashboardSearchViewProviderData*)iter->data;

			if(providerData &&
				providerData->container)
			{
				newSelection=xfdashboard_search_result_container_find_selection(XFDASHBOARD_SEARCH_RESULT_CONTAINER(providerData->container),
																				NULL,
																				XFDASHBOARD_SELECTION_TARGET_FIRST,
																				XFDASHBOARD_VIEW(self));
				if(newSelection) newSelectionProvider=providerData;
			}
		}

		g_debug("No selection for %s, so select first selectable actor of provider %s",
					G_OBJECT_TYPE_NAME(self),
					newSelectionProvider && newSelectionProvider->provider ? G_OBJECT_TYPE_NAME(newSelectionProvider->provider) : "<unknown provider>");

		return(newSelection);
	}

	/* If first selection is request, select the first selectable actor of the first provider
	 * having an existing container.
	 */
	if(inDirection==XFDASHBOARD_SELECTION_TARGET_FIRST)
	{
		/* Find first provider having an existing container and having a selectable
		 * actor in its container.
		 */
		for(iter=priv->providers; iter && !newSelection; iter=g_list_next(iter))
		{
			providerData=(XfdashboardSearchViewProviderData*)iter->data;

			if(providerData &&
				providerData->container)
			{
				newSelection=xfdashboard_search_result_container_find_selection(XFDASHBOARD_SEARCH_RESULT_CONTAINER(providerData->container),
																				inSelection,
																				XFDASHBOARD_SELECTION_TARGET_FIRST,
																				XFDASHBOARD_VIEW(self));
				if(newSelection) newSelectionProvider=providerData;
			}
		}

		g_debug("First selection requested at %s, so select first selectable actor of provider %s",
					G_OBJECT_TYPE_NAME(self),
					newSelectionProvider && newSelectionProvider->provider ? G_OBJECT_TYPE_NAME(newSelectionProvider->provider) : "<unknown provider>");

		return(newSelection);
	}

	/* If last selection is request, select the last selectable actor of the last provider
	 * having an existing container.
	 */
	if(inDirection==XFDASHBOARD_SELECTION_TARGET_LAST)
	{
		/* Find last provider having an existing container and having a selectable
		 * actor in its container.
		 */
		for(iter=g_list_last(priv->providers); iter && !newSelection; iter=g_list_previous(iter))
		{
			providerData=(XfdashboardSearchViewProviderData*)iter->data;

			if(providerData &&
				providerData->container)
			{
				newSelection=xfdashboard_search_result_container_find_selection(XFDASHBOARD_SEARCH_RESULT_CONTAINER(providerData->container),
																				inSelection,
																				XFDASHBOARD_SELECTION_TARGET_LAST,
																				XFDASHBOARD_VIEW(self));
				if(newSelection) newSelectionProvider=providerData;
			}
		}

		g_debug("Last selection requested at %s, so select last selectable actor of provider %s",
					G_OBJECT_TYPE_NAME(self),
					newSelectionProvider && newSelectionProvider->provider ? G_OBJECT_TYPE_NAME(newSelectionProvider->provider) : "<unknown provider>");

		return(newSelection);
	}

	/* Find provider data for selection requested. If we do not find provider data
	 * for requested selection then we cannot perform find request.
	 */
	newSelectionProvider=_xfdashboard_search_view_get_provider_data_by_actor(self, inSelection);
	if(!newSelectionProvider)
	{
		g_debug("Could not find provider for selection %p of type %s",
					inSelection,
					inSelection ? G_OBJECT_TYPE_NAME(inSelection) : "<nil>");
		return(NULL);
	}

	currentProviderIter=g_list_find(priv->providers, newSelectionProvider);
	if(!currentProviderIter)
	{
		g_debug("Could not find position of provider %s",
					newSelectionProvider->provider ? G_OBJECT_TYPE_NAME(newSelectionProvider->provider) : "<unknown provider>");

		/* Release allocated resources */
		_xfdashboard_search_view_provider_data_unref(newSelectionProvider);

		return(NULL);
	}

	/* Ask current provider to find selection for requested direction. If a matching
	 * selection could not be found then ask next providers depending on direction.
	 */
	switch(inDirection)
	{
		case XFDASHBOARD_SELECTION_TARGET_LEFT:
		case XFDASHBOARD_SELECTION_TARGET_UP:
		case XFDASHBOARD_SELECTION_TARGET_PAGE_LEFT:
		case XFDASHBOARD_SELECTION_TARGET_PAGE_UP:
			/* Ask current provider to find selection for requested direction */
			newSelection=xfdashboard_search_result_container_find_selection(XFDASHBOARD_SEARCH_RESULT_CONTAINER(priv->selectionProvider->container),
																			inSelection,
																			inDirection,
																			XFDASHBOARD_VIEW(self));

			/* If current provider does not return a matching selection for requested,
			 * iterate backwards through providers beginning at current provider and
			 * return the last actor of first provider having an existing container
			 * while iterating.
			 */
			if(!newSelection)
			{
				for(iter=g_list_previous(currentProviderIter); iter && !newSelection; iter=g_list_previous(iter))
				{
					providerData=(XfdashboardSearchViewProviderData*)iter->data;

					if(providerData &&
						providerData->container)
					{
						newSelection=xfdashboard_search_result_container_find_selection(XFDASHBOARD_SEARCH_RESULT_CONTAINER(providerData->container),
																						NULL,
																						XFDASHBOARD_SELECTION_TARGET_LAST,
																						XFDASHBOARD_VIEW(self));
					}
				}
			}
			break;

		case XFDASHBOARD_SELECTION_TARGET_RIGHT:
		case XFDASHBOARD_SELECTION_TARGET_DOWN:
		case XFDASHBOARD_SELECTION_TARGET_PAGE_RIGHT:
		case XFDASHBOARD_SELECTION_TARGET_PAGE_DOWN:
			/* Ask current provider to find selection for requested direction */
			newSelection=xfdashboard_search_result_container_find_selection(XFDASHBOARD_SEARCH_RESULT_CONTAINER(priv->selectionProvider->container),
																			inSelection,
																			inDirection,
																			XFDASHBOARD_VIEW(self));

			/* If current provider does not return a matching selection for requested,
			 * iterate forwards through providers beginning at current provider and
			 * return the first actor of first provider having an existing container
			 * while iterating.
			 */
			if(!newSelection)
			{
				for(iter=g_list_next(currentProviderIter); iter && !newSelection; iter=g_list_next(iter))
				{
					providerData=(XfdashboardSearchViewProviderData*)iter->data;

					if(providerData &&
						providerData->container)
					{
						newSelection=xfdashboard_search_result_container_find_selection(XFDASHBOARD_SEARCH_RESULT_CONTAINER(providerData->container),
																						NULL,
																						XFDASHBOARD_SELECTION_TARGET_FIRST,
																						XFDASHBOARD_VIEW(self));
					}
				}
			}
			break;

		case XFDASHBOARD_SELECTION_TARGET_NEXT:
			/* Ask current provider to find selection for requested direction */
			newSelection=xfdashboard_search_result_container_find_selection(XFDASHBOARD_SEARCH_RESULT_CONTAINER(priv->selectionProvider->container),
																			inSelection,
																			inDirection,
																			XFDASHBOARD_VIEW(self));

			/* If current provider does not return a matching selection for requested,
			 * iterate forwards through providers beginning at current provider and
			 * return the first actor of first provider having an existing container
			 * while iterating.
			 */
			if(!newSelection)
			{
				for(iter=g_list_next(currentProviderIter); iter && !newSelection; iter=g_list_next(iter))
				{
					providerData=(XfdashboardSearchViewProviderData*)iter->data;

					if(providerData &&
						providerData->container)
					{
						newSelection=xfdashboard_search_result_container_find_selection(XFDASHBOARD_SEARCH_RESULT_CONTAINER(providerData->container),
																						NULL,
																						XFDASHBOARD_SELECTION_TARGET_FIRST,
																						XFDASHBOARD_VIEW(self));
					}
				}
			}

			/* If still no matching selection was found then iterate backwards through
			 * providers beginning at current provider and return the last actor of first
			 * provider having an existing container while iterating.
			 */
			if(!newSelection)
			{
				for(iter=g_list_previous(currentProviderIter); iter && !newSelection; iter=g_list_previous(iter))
				{
					providerData=(XfdashboardSearchViewProviderData*)iter->data;

					if(providerData &&
						providerData->container)
					{
						newSelection=xfdashboard_search_result_container_find_selection(XFDASHBOARD_SEARCH_RESULT_CONTAINER(providerData->container),
																						NULL,
																						XFDASHBOARD_SELECTION_TARGET_LAST,
																						XFDASHBOARD_VIEW(self));
					}
				}
			}
			break;

		case XFDASHBOARD_SELECTION_TARGET_FIRST:
		case XFDASHBOARD_SELECTION_TARGET_LAST:
			/* These directions should be handled at beginning of this function
			 * and therefore should never be reached!
			 */
			g_assert_not_reached();
			break;

		default:
			{
				gchar					*valueName;

				valueName=xfdashboard_get_enum_value_name(XFDASHBOARD_TYPE_SELECTION_TARGET, inDirection);
				g_critical(_("Focusable object %s and provider %s do not handle selection direction of type %s."),
							G_OBJECT_TYPE_NAME(self),
							newSelectionProvider->provider ? G_OBJECT_TYPE_NAME(newSelectionProvider->provider) : "<unknown provider>",
							valueName);
				g_free(valueName);

				/* Ensure new selection is invalid */
				newSelection=NULL;
			}
			break;
	}

	/* Release allocated resources */
	_xfdashboard_search_view_provider_data_unref(newSelectionProvider);

	/* Return new selection found in other search providers */
	return(newSelection);
}

/* Activate selection */
static gboolean _xfdashboard_search_view_focusable_activate_selection(XfdashboardFocusable *inFocusable,
																		ClutterActor *inSelection)
{
	XfdashboardSearchView					*self;
	XfdashboardSearchViewProviderData		*providerData;

	g_return_val_if_fail(XFDASHBOARD_IS_FOCUSABLE(inFocusable), FALSE);
	g_return_val_if_fail(XFDASHBOARD_IS_SEARCH_VIEW(inFocusable), FALSE);
	g_return_val_if_fail(CLUTTER_IS_ACTOR(inSelection), FALSE);

	self=XFDASHBOARD_SEARCH_VIEW(inFocusable);

	/* Find data of provider for requested selected actor */
	providerData=_xfdashboard_search_view_get_provider_data_by_actor(self, inSelection);
	if(!providerData)
	{
		g_warning(_("%s is not a child of any provider at %s and cannot be activated"),
					G_OBJECT_TYPE_NAME(inSelection),
					G_OBJECT_TYPE_NAME(self));

		return(FALSE);
	}

	/* Activate selection */
	_xfdashboard_search_view_on_provider_item_actor_clicked(NULL, inSelection, providerData);

	/* Release allocated resources */
	_xfdashboard_search_view_provider_data_unref(providerData);

	return(TRUE);
}

/* Interface initialization
 * Set up default functions
 */
void _xfdashboard_search_view_focusable_iface_init(XfdashboardFocusableInterface *iface)
{
	iface->can_focus=_xfdashboard_search_view_focusable_can_focus;
	iface->supports_selection=_xfdashboard_search_view_focusable_supports_selection;
	iface->get_selection=_xfdashboard_search_view_focusable_get_selection;
	iface->set_selection=_xfdashboard_search_view_focusable_set_selection;
	iface->find_selection=_xfdashboard_search_view_focusable_find_selection;
	iface->activate_selection=_xfdashboard_search_view_focusable_activate_selection;
}

/* IMPLEMENTATION: GObject */

/* Dispose this object */
static void _xfdashboard_search_view_dispose(GObject *inObject)
{
	XfdashboardSearchView			*self=XFDASHBOARD_SEARCH_VIEW(inObject);
	XfdashboardSearchViewPrivate	*priv=self->priv;

	/* Release allocated resources */
	if(priv->repaintID)
	{
		g_source_remove(priv->repaintID);
		priv->repaintID=0;
	}

	if(priv->searchManager)
	{
		g_signal_handlers_disconnect_by_data(priv->searchManager, self);
		g_object_unref(priv->searchManager);
		priv->searchManager=NULL;
	}

	if(priv->providers)
	{
		g_list_free_full(priv->providers, (GDestroyNotify)_xfdashboard_search_view_provider_data_unref);
		priv->providers=NULL;
	}

	if(priv->lastTerms)
	{
		_xfdashboard_search_view_search_terms_unref(priv->lastTerms);
		priv->lastTerms=NULL;
	}

	if(priv->selectionProvider)
	{
		_xfdashboard_search_view_provider_data_unref(priv->selectionProvider);
		priv->selectionProvider=NULL;
	}

	if(priv->focusManager)
	{
		g_object_unref(priv->focusManager);
		priv->focusManager=NULL;
	}

	/* Call parent's class dispose method */
	G_OBJECT_CLASS(xfdashboard_search_view_parent_class)->dispose(inObject);
}

/* Class initialization
 * Override functions in parent classes and define properties
 * and signals
 */
static void xfdashboard_search_view_class_init(XfdashboardSearchViewClass *klass)
{
	GObjectClass			*gobjectClass=G_OBJECT_CLASS(klass);

	/* Override functions */
	gobjectClass->dispose=_xfdashboard_search_view_dispose;

	/* Set up private structure */
	g_type_class_add_private(klass, sizeof(XfdashboardSearchViewPrivate));

	/* Define signals */
	XfdashboardSearchViewSignals[SIGNAL_SEARCH_RESET]=
		g_signal_new("search-reset",
						G_TYPE_FROM_CLASS(klass),
						G_SIGNAL_RUN_LAST,
						G_STRUCT_OFFSET(XfdashboardSearchViewClass, search_reset),
						NULL,
						NULL,
						g_cclosure_marshal_VOID__OBJECT,
						G_TYPE_NONE,
						0);

	XfdashboardSearchViewSignals[SIGNAL_SEARCH_UPDATED]=
		g_signal_new("search-updated",
						G_TYPE_FROM_CLASS(klass),
						G_SIGNAL_RUN_LAST,
						G_STRUCT_OFFSET(XfdashboardSearchViewClass, search_updated),
						NULL,
						NULL,
						g_cclosure_marshal_VOID__VOID,
						G_TYPE_NONE,
						0);
}

/* Object initialization
 * Create private structure and set up default values
 */
static void xfdashboard_search_view_init(XfdashboardSearchView *self)
{
	XfdashboardSearchViewPrivate	*priv;
	GList							*providers, *entry;
	ClutterLayoutManager			*layout;

	self->priv=priv=XFDASHBOARD_SEARCH_VIEW_GET_PRIVATE(self);

	/* Set up default values */
	priv->searchManager=xfdashboard_search_manager_get_default();
	priv->providers=NULL;
	priv->lastTerms=NULL;
	priv->selectionProvider=NULL;
	priv->focusManager=xfdashboard_focus_manager_get_default();
	priv->repaintID=0;

	/* Set up view (Note: Search view is disabled by default!) */
	xfdashboard_view_set_internal_name(XFDASHBOARD_VIEW(self), "search");
	xfdashboard_view_set_name(XFDASHBOARD_VIEW(self), _("Search"));
	xfdashboard_view_set_icon(XFDASHBOARD_VIEW(self), "gtk-find");
	xfdashboard_view_set_enabled(XFDASHBOARD_VIEW(self), FALSE);

	/* Set up actor */
	xfdashboard_actor_set_can_focus(XFDASHBOARD_ACTOR(self), TRUE);

	layout=clutter_box_layout_new();
	clutter_box_layout_set_orientation(CLUTTER_BOX_LAYOUT(layout), CLUTTER_ORIENTATION_VERTICAL);
	clutter_actor_set_layout_manager(CLUTTER_ACTOR(self), layout);

	xfdashboard_view_set_fit_mode(XFDASHBOARD_VIEW(self), XFDASHBOARD_FIT_MODE_HORIZONTAL);

	/* Create instance of each registered view type and add it to this actor
	 * and connect signals
	 */
	providers=entry=xfdashboard_search_manager_get_registered(priv->searchManager);
	for(; entry; entry=g_list_next(entry))
	{
		GType					providerType;

		providerType=(GType)GPOINTER_TO_GTYPE(entry->data);
		_xfdashboard_search_view_on_search_provider_registered(self, providerType, priv->searchManager);
	}
	g_list_free(providers);

	g_signal_connect_swapped(priv->searchManager,
								"registered",
								G_CALLBACK(_xfdashboard_search_view_on_search_provider_registered),
								self);
	g_signal_connect_swapped(priv->searchManager,
								"unregistered",
								G_CALLBACK(_xfdashboard_search_view_on_search_provider_unregistered),
								self);
}

/* IMPLEMENTATION: Public API */

/* Reset an ongoing search */
void xfdashboard_search_view_reset_search(XfdashboardSearchView *self)
{
	XfdashboardSearchViewPrivate	*priv;
	GList							*providers;
	GList							*iter;

	g_return_if_fail(XFDASHBOARD_IS_SEARCH_VIEW(self));

	priv=self->priv;

	/* Reset all search providers by destroying actors, destroying containers,
	 * clearing mappings and release all other allocated resources used.
	 */
	providers=g_list_copy(priv->providers);
	g_list_foreach(providers, (GFunc)_xfdashboard_search_view_provider_data_ref, NULL);
	for(iter=providers; iter; iter=g_list_next(iter))
	{
		XfdashboardSearchViewProviderData		*providerData;

		/* Get data for provider to reset */
		providerData=((XfdashboardSearchViewProviderData*)(iter->data));

		/* Clean provider's container and move focus if necessary */
		_xfdashboard_search_view_clean_provider_container(providerData);

		/* Destroy container */
		if(providerData->container)
		{
			/* First disconnect signal handlers from actor before destroying container */
			g_signal_handlers_disconnect_by_data(providerData->container, providerData);

			/* Destroy container */
			clutter_actor_destroy(providerData->container);
			providerData->container=NULL;
		}

		/* Release last result set as provider has no results anymore */
		if(providerData->lastResultSet)
		{
			g_object_unref(providerData->lastResultSet);
			providerData->lastResultSet=NULL;
		}

		/* Release last terms used in last search of provider */
		if(providerData->lastTerms)
		{
			_xfdashboard_search_view_search_terms_unref(providerData->lastTerms);
			providerData->lastTerms=NULL;
		}
	}
	g_list_free_full(providers, (GDestroyNotify)_xfdashboard_search_view_provider_data_unref);

	/* Reset last search terms used in this view */
	if(priv->lastTerms)
	{
		_xfdashboard_search_view_search_terms_unref(priv->lastTerms);
		priv->lastTerms=NULL;
	}

	/* Emit signal that search was resetted */
	g_signal_emit(self, XfdashboardSearchViewSignals[SIGNAL_SEARCH_RESET], 0);
}

/* Start a new search or update an ongoing one */
void xfdashboard_search_view_update_search(XfdashboardSearchView *self, const gchar *inSearchString)
{
	XfdashboardSearchViewPrivate				*priv;
	XfdashboardSearchViewSearchTerms			*searchTerms;
	GList										*providers;
	GList										*iter;
#ifdef DEBUG
	GTimer										*timer=NULL;
#endif

	g_return_if_fail(XFDASHBOARD_IS_SEARCH_VIEW(self));

	priv=self->priv;

	/* Only perform a search if new search term differs from old one */
	if(priv->lastTerms &&
		g_strcmp0(inSearchString, priv->lastTerms->termString)==0)
	{
		return;
	}

	/* Searching for NULL or an empty string is like resetting search */
	if(!inSearchString || strlen(inSearchString)==0)
	{
		xfdashboard_search_view_reset_search(self);
		return;
	}

	/* Get search terms for search string. If we could not split search
	 * string into terms then reset current search.
	 */
	searchTerms=_xfdashboard_search_view_search_terms_new(inSearchString);
	if(!searchTerms)
	{
		/* Splitting search string into terms failed so reset search */
		xfdashboard_search_view_reset_search(self);
		return;
	}

#ifdef DEBUG
	/* Start timer for debug search performance */
	timer=g_timer_new();
#endif

	/* Perform a search at all registered search providers */
	providers=g_list_copy(priv->providers);
	g_list_foreach(providers, (GFunc)_xfdashboard_search_view_provider_data_ref, NULL);
	for(iter=providers; iter; iter=g_list_next(iter))
	{
		XfdashboardSearchViewProviderData		*providerData;
		gboolean								canDoIncrementalSearch;
		XfdashboardSearchResultSet				*providerNewResultSet;
		XfdashboardSearchResultSet				*providerLastResultSet;

		/* Get data for provider to perform search at */
		providerData=((XfdashboardSearchViewProviderData*)(iter->data));

		/* Check if we can do an incremental search based on previous
		 * results or if we have to do a full search.
		 */
		canDoIncrementalSearch=FALSE;
		providerLastResultSet=NULL;
		if(providerData->lastTerms &&
			_xfdashboard_search_view_can_do_incremental_search(providerData->lastTerms, searchTerms))
		{
			canDoIncrementalSearch=TRUE;
			providerLastResultSet=g_object_ref(providerData->lastResultSet);
		}

		/* Perform search */
		providerNewResultSet=xfdashboard_search_provider_get_result_set(providerData->provider,
																		(const gchar**)searchTerms->termList,
																		providerLastResultSet);
		g_debug("Performed %s search at search provider %s and got %u result items",
					canDoIncrementalSearch==TRUE ? "incremental" : "full",
					G_OBJECT_TYPE_NAME(providerData->provider),
					xfdashboard_search_result_set_get_size(providerNewResultSet));


		/* Update view of search provider for new result set */
		_xfdashboard_search_view_update_provider_container(self, providerData, providerNewResultSet);

		/* Remember new search term as last one at search provider */
		if(providerData->lastTerms) _xfdashboard_search_view_search_terms_unref(providerData->lastTerms);
		providerData->lastTerms=_xfdashboard_search_view_search_terms_ref(searchTerms);

		/* Release allocated resources */
		if(providerLastResultSet) g_object_unref(providerLastResultSet);
		if(providerNewResultSet) g_object_unref(providerNewResultSet);
	}
	g_list_free_full(providers, (GDestroyNotify)_xfdashboard_search_view_provider_data_unref);

	/* Remember new search terms as last one */
	if(priv->lastTerms) _xfdashboard_search_view_search_terms_unref(priv->lastTerms);
	priv->lastTerms=_xfdashboard_search_view_search_terms_ref(searchTerms);

#ifdef DEBUG
	/* Get time for this search for debug performance */
	g_debug("Updating search for '%s' took %f seconds", inSearchString, g_timer_elapsed(timer, NULL));
	g_timer_destroy(timer);
#endif

	/* If this view has the focus then check if this view has a selection set currently.
	 * If not select the first selectable actor otherwise just ensure the current
	 * selection is visible.
	 */
	if(xfdashboard_focus_manager_has_focus(priv->focusManager, XFDASHBOARD_FOCUSABLE(self)))
	{
		ClutterActor							*selection;

		/* Check if this view has a selection set */
		selection=xfdashboard_focusable_get_selection(XFDASHBOARD_FOCUSABLE(self));
		if(!selection)
		{
			/* Select first selectable item */
			selection=xfdashboard_focusable_find_selection(XFDASHBOARD_FOCUSABLE(self),
															NULL,
															XFDASHBOARD_SELECTION_TARGET_FIRST);
			xfdashboard_focusable_set_selection(XFDASHBOARD_FOCUSABLE(self), selection);
		}

		/* Ensure selection is visible. But we have to have for a repaint because
		 * allocation of this view has not changed yet.
		 */
		if(selection &&
			priv->repaintID==0)
		{
			priv->repaintID=clutter_threads_add_repaint_func_full(CLUTTER_REPAINT_FLAGS_QUEUE_REDRAW_ON_ADD | CLUTTER_REPAINT_FLAGS_POST_PAINT,
																	_xfdashboard_search_view_on_repaint_after_update_callback,
																	self,
																	NULL);
		}
	}

	/* Emit signal that search was updated */
	g_signal_emit(self, XfdashboardSearchViewSignals[SIGNAL_SEARCH_UPDATED], 0);

	/* Release allocated resources */
	if(searchTerms) _xfdashboard_search_view_search_terms_unref(searchTerms);
}
