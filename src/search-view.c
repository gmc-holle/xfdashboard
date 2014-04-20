/*
 * search-view: A view showing applications matching search criterias
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

#include "search-view.h"

#include <glib/gi18n-lib.h>
#include <gtk/gtk.h>

#include "search-manager.h"
#include "search-result-container.h"
#include "click-action.h"
#include "drag-action.h"
#include "utils.h"
#include "focusable.h"
#include "stylable.h"

/* Define this class in GObject system */
static void _xfdashboard_search_view_focusable_iface_init(XfdashboardFocusableInterface *iface);

G_DEFINE_TYPE_WITH_CODE(XfdashboardSearchView,
						xfdashboard_search_view,
						XFDASHBOARD_TYPE_VIEW,
						G_IMPLEMENT_INTERFACE(XFDASHBOARD_TYPE_FOCUSABLE, _xfdashboard_search_view_focusable_iface_init))

/* IMPLEMENTATION: Private structures */
typedef struct _XfdashboardSearchViewProviderData	XfdashboardSearchViewProviderData;
struct _XfdashboardSearchViewProviderData
{
	XfdashboardSearchView				*view;

	XfdashboardSearchProvider			*provider;
	XfdashboardSearchResultSet			*lastResultSet;
	gulong								lastSearchTimestamp;

	ClutterActor						*container;
	GList								*mappings;

	ClutterActor						*lastResultItemActorSeen;
};

typedef struct _XfdashboardSearchViewProviderItemsMapping	XfdashboardSearchViewProviderItemsMapping;
struct _XfdashboardSearchViewProviderItemsMapping
{
	XfdashboardSearchViewProviderData	*providerData;
	GVariant							*item;
	ClutterActor						*actor;
	guint								actorDestroyID;
};

/* Private structure - access only by public API if needed */
#define XFDASHBOARD_SEARCH_VIEW_GET_PRIVATE(obj) \
	(G_TYPE_INSTANCE_GET_PRIVATE((obj), XFDASHBOARD_TYPE_SEARCH_VIEW, XfdashboardSearchViewPrivate))

struct _XfdashboardSearchViewPrivate
{
	/* Instance related */
	XfdashboardSearchManager			*searchManager;
	GList								*providers;
	gchar								*lastSearchString;
	gchar								**lastSearchTermsList;

	XfdashboardSearchViewProviderData	*selectionProvider;
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

/* Remove a mapping between result set item and actor from stored mapping */
static void _xfdashboard_search_view_provider_item_mapping_free(XfdashboardSearchViewProviderItemsMapping *inMapping)
{
	g_return_if_fail(inMapping);

	/* Free mapping */
	if(inMapping->item) g_variant_unref(inMapping->item);
	if(inMapping->actor && inMapping->actorDestroyID) g_signal_handler_disconnect(inMapping->actor, inMapping->actorDestroyID);
	g_free(inMapping);
}

/* Create a mapping between result set item and actor and store it */
static XfdashboardSearchViewProviderItemsMapping* _xfdashboard_search_view_provider_item_mapping_new(GVariant *inItem, ClutterActor *inActor)
{
	XfdashboardSearchViewProviderItemsMapping	*mapping;

	g_return_val_if_fail(inItem, NULL);
	g_return_val_if_fail(CLUTTER_IS_ACTOR(inActor), NULL);

	/* Set up mapping and add to array */
	mapping=g_new0(XfdashboardSearchViewProviderItemsMapping, 1);
	mapping->item=g_variant_ref(inItem);
	mapping->actor=inActor;

	return(mapping);
}

/* Free data for provider */
static void _xfdashboard_search_view_provider_data_free(XfdashboardSearchViewProviderData *inData)
{
	g_return_if_fail(inData);

	/* Release allocated resources */
	if(inData->mappings) g_list_free_full(inData->mappings, (GDestroyNotify)_xfdashboard_search_view_provider_item_mapping_free);
	if(inData->lastResultSet) g_object_unref(inData->lastResultSet);
	if(inData->container) clutter_actor_destroy(inData->container);
	if(inData->provider) g_object_unref(inData->provider);
	g_free(inData);
}

/* Create data for provider */
static XfdashboardSearchViewProviderData* _xfdashboard_search_view_provider_data_new(GType inProviderType)
{
	XfdashboardSearchViewProviderData	*data;

	g_return_val_if_fail(inProviderType!=XFDASHBOARD_TYPE_SEARCH_PROVIDER && g_type_is_a(inProviderType, XFDASHBOARD_TYPE_SEARCH_PROVIDER), NULL);

	/* Create data for provider */
	data=g_new0(XfdashboardSearchViewProviderData, 1);
	data->view=NULL;
	data->provider=XFDASHBOARD_SEARCH_PROVIDER(g_object_new(inProviderType, NULL));
	data->lastResultSet=NULL;
	data->lastSearchTimestamp=0L;
	data->container=NULL;
	data->mappings=NULL;

	return(data);
}

/* Get data for requested provider */
static XfdashboardSearchViewProviderData* _xfdashboard_search_view_get_provider_data(XfdashboardSearchView *self,
																						GType inProviderType)
{
	XfdashboardSearchViewPrivate		*priv;
	XfdashboardSearchViewProviderData	*data;
	GList								*entry;

	g_return_val_if_fail(XFDASHBOARD_IS_SEARCH_VIEW(self), NULL);

	priv=self->priv;

	/* Iterate through list of provider data and lookup requested one */
	for(entry=priv->providers; entry; entry=g_list_next(entry))
	{
		data=(XfdashboardSearchViewProviderData*)entry->data;

		if(G_OBJECT_TYPE(data->provider)==inProviderType) return(data);
	}

	/* If we get here we did not find data for requested provider */
	return(NULL);
}

/* Remove mapping to provider data */
static void _xfdashboard_search_view_provider_data_remove_mapping(XfdashboardSearchViewProviderItemsMapping *inMapping)
{
	XfdashboardSearchViewProviderData	*providerData;
	GList								*entry;

	g_return_if_fail(inMapping);

	/* Get provider data */
	providerData=inMapping->providerData;

	/* Find list entry */
	entry=g_list_find(providerData->mappings, inMapping);
	if(!entry) return;

	/* Remove mapping from list in provider data */
	providerData->mappings=g_list_remove_link(providerData->mappings, entry);
}

/* Add mapping to provider data */
static gboolean _xfdashboard_search_view_provider_data_add_mapping(XfdashboardSearchViewProviderData *inProviderData,
																	XfdashboardSearchViewProviderItemsMapping *inMapping)
{
	g_return_val_if_fail(inProviderData, FALSE);
	g_return_val_if_fail(inMapping, FALSE);
	g_return_val_if_fail(inMapping->providerData==NULL, FALSE);

	/* Reference provider data in mapping */
	inMapping->providerData=inProviderData;

	/* Add to list of mapping in provider data */
	inProviderData->mappings=g_list_prepend(inProviderData->mappings, inMapping);

	return(TRUE);
}

/* Find mapping in provider data */
static XfdashboardSearchViewProviderItemsMapping* _xfdashboard_search_view_provider_data_get_mapping_by_item(XfdashboardSearchViewProviderData *inProviderData,
																												GVariant *inResultItem)
{
	XfdashboardSearchViewProviderItemsMapping	*mapping;
	GList										*entry;

	g_return_val_if_fail(inProviderData, NULL);
	g_return_val_if_fail(inResultItem, NULL);

	/* Iterate through mappings in provider data and return mapping
	 * if the result set item in mapping is equal to given one
	 */
	for(entry=inProviderData->mappings; entry; entry=g_list_next(entry))
	{
		mapping=(XfdashboardSearchViewProviderItemsMapping*)entry->data;

		if(g_variant_compare(inResultItem, mapping->item)==0) return(mapping);
	}

	/* If we get here we did not find a match */
	return(NULL);
}

/* A provider icon was clicked */
static void _xfdashboard_search_view_on_provider_icon_clicked(XfdashboardSearchResultContainer *inContainer,
																gpointer inUserData)
{
	XfdashboardSearchViewProviderData			*providerData;
	XfdashboardSearchView						*self;
	XfdashboardSearchViewPrivate				*priv;

	g_return_if_fail(XFDASHBOARD_IS_SEARCH_RESULT_CONTAINER(inContainer));
	g_return_if_fail(inUserData);

	providerData=(XfdashboardSearchViewProviderData*)inUserData;

	/* Get search view and private data of view */
	self=providerData->view;
	priv=self->priv;

	/* Tell provider to launch search */
	xfdashboard_search_provider_launch_search(providerData->provider,
												(const gchar**)priv->lastSearchTermsList);
}

/* A result item actor was clicked */
static void _xfdashboard_search_view_on_provider_item_actor_clicked(ClutterClickAction *inAction,
																		ClutterActor *inActor,
																		gpointer inUserData)
{
	XfdashboardSearchViewProviderItemsMapping	*mapping;
	XfdashboardSearchViewProviderData			*providerData;
	XfdashboardSearchView						*self;
	XfdashboardSearchViewPrivate				*priv;

	g_return_if_fail(CLUTTER_IS_ACTOR(inActor));
	g_return_if_fail(inUserData);

	mapping=(XfdashboardSearchViewProviderItemsMapping*)inUserData;

	/* Get provider data */
	providerData=mapping->providerData;

	/* Get search view and private data of view */
	self=providerData->view;
	priv=self->priv;

	/* Tell provider that a result item was clicked */
	xfdashboard_search_provider_activate_result(providerData->provider,
													mapping->item,
													mapping->actor,
													(const gchar**)priv->lastSearchTermsList);
}

/* A result item actor is going to be destroyed */
static void _xfdashboard_search_view_on_provider_item_actor_destroy(ClutterActor *inActor, gpointer inUserData)
{
	XfdashboardSearchViewProviderItemsMapping	*mapping;

	g_return_if_fail(CLUTTER_IS_ACTOR(inActor));
	g_return_if_fail(inUserData);

	mapping=(XfdashboardSearchViewProviderItemsMapping*)inUserData;

	/* Remove mapping from list of mappings in provider */
	_xfdashboard_search_view_provider_data_remove_mapping(mapping);

	/* Disconnect signal handlers from actor and unset actor in mapping
	 * because it is destroyed and cannot be cleaned up anymore
	 */
	if(mapping->actorDestroyID)
	{
		g_signal_handler_disconnect(mapping->actor, mapping->actorDestroyID);
		mapping->actorDestroyID=0;
	}
	mapping->actor=NULL;

	/* Release allocated resources used by mapping */
	_xfdashboard_search_view_provider_item_mapping_free(mapping);
}

/* Destroys actors in container of provider for items being not in result set anymore */
static void _xfdashboard_search_view_update_provider_actor_destroy(gpointer inData, gpointer inUserData)
{
	XfdashboardSearchViewProviderItemsMapping	*mapping;
	XfdashboardSearchResultSet					*resultSet;

	g_return_if_fail(inData);
	g_return_if_fail(XFDASHBOARD_IS_SEARCH_RESULT_SET(inUserData));

	mapping=(XfdashboardSearchViewProviderItemsMapping*)inData;
	resultSet=XFDASHBOARD_SEARCH_RESULT_SET(inUserData);

	/* Check if result item in mapping is still available in result set */
	if(xfdashboard_search_result_set_get_index(resultSet, mapping->item)!=-1) return;

	/* Remove mapping from list of mappings in provider */
	_xfdashboard_search_view_provider_data_remove_mapping(mapping);

	/* Disconnect signal handlers from actor, destroy actor and
	 * unset actor in mapping because it is cleaned up already
	 */
	if(mapping->actorDestroyID)
	{
		g_signal_handler_disconnect(mapping->actor, mapping->actorDestroyID);
		mapping->actorDestroyID=0;
	}
	clutter_actor_destroy(mapping->actor);
	mapping->actor=NULL;

	/* Release allocated resources used by mapping */
	_xfdashboard_search_view_provider_item_mapping_free(mapping);
}

/* Create new actors in container of provider for items being new in result set */
static void _xfdashboard_search_view_update_provider_actor_new(gpointer inData,
																gpointer inUserData)
{
	GVariant									*resultSetItem;
	XfdashboardSearchViewProviderData			*providerData;
	XfdashboardSearchViewProviderItemsMapping	*mapping;
	ClutterActor								*actor;
	ClutterAction								*action;
	XfdashboardSearchView						*self;
	GList										*actionsList, *actionsEntry;

	g_return_if_fail(inData);
	g_return_if_fail(inUserData);

	resultSetItem=(GVariant*)inData;
	providerData=(XfdashboardSearchViewProviderData*)inUserData;

	/* Get search view and private data of view */
	self=providerData->view;

	/* Check if an actor for result set item exists already */
	mapping=_xfdashboard_search_view_provider_data_get_mapping_by_item(providerData, resultSetItem);
	if(mapping)
	{
		/* Remember this actor which is kept as last one seen for next actor added */
		providerData->lastResultItemActorSeen=mapping->actor;

		return;
	}

	/* Create actor for result set item and store in mapping */
	actor=xfdashboard_search_provider_create_result_actor(providerData->provider, resultSetItem);
	if(!actor)
	{
		g_warning(_("Failed to add actor for result item [%s] of provider %s: %s"),
					g_variant_print(resultSetItem, TRUE),
					G_OBJECT_TYPE_NAME(providerData->provider),
					_("Failed to created actor"));
		return;
	}

	mapping=_xfdashboard_search_view_provider_item_mapping_new(resultSetItem, actor);
	if(!mapping)
	{
		g_warning(_("Failed to add actor for result item [%s] of provider %s: %s"),
					g_variant_print(resultSetItem, TRUE),
					G_OBJECT_TYPE_NAME(providerData->provider),
					_("Failed to create item-actor mapping"));
		clutter_actor_destroy(actor);
		return;
	}

	if(!_xfdashboard_search_view_provider_data_add_mapping(providerData, mapping))
	{
		g_warning(_("Failed to add actor for result item [%s] of provider %s: %s"),
					g_variant_print(resultSetItem, TRUE),
					G_OBJECT_TYPE_NAME(providerData->provider),
					_("Failed to store item-actor mapping"));
		clutter_actor_destroy(actor);
		return;
	}

	/* Connect actions and signals */
	mapping->actorDestroyID=g_signal_connect(actor,
												"destroy",
												G_CALLBACK(_xfdashboard_search_view_on_provider_item_actor_destroy),
												mapping);

	action=xfdashboard_click_action_new();
	clutter_actor_add_action(actor, action);
	g_signal_connect(action,
						"clicked",
						G_CALLBACK(_xfdashboard_search_view_on_provider_item_actor_clicked),
						mapping);

	/* If actor has an action of type XfdashboardDragAction and has no source set
	 * then set this view as source
	 */
	actionsList=clutter_actor_get_actions(actor);
	if(actionsList)
	{
		for(actionsEntry=actionsList; actionsEntry; actionsEntry=g_list_next(actionsEntry))
		{
			if(XFDASHBOARD_IS_DRAG_ACTION(actionsEntry->data) &&
				!xfdashboard_drag_action_get_source(XFDASHBOARD_DRAG_ACTION(actionsEntry->data)))
			{
				g_object_set(actionsEntry->data, "source", self, NULL);
			}
		}
		g_list_free(actionsList);
	}

	/* Add newly created actor to container of provider */
	xfdashboard_search_result_container_add_result_actor(XFDASHBOARD_SEARCH_RESULT_CONTAINER(providerData->container), actor, providerData->lastResultItemActorSeen);

	/* Remember newly created and added actor as last one seen for next actor added */
	providerData->lastResultItemActorSeen=actor;
}

/* Updates container of provider with new result set and also creates or destroys
 * this container if needed
 */
static void _xfdashboard_search_view_update_provider_container(XfdashboardSearchView *self,
																XfdashboardSearchViewProviderData *inProviderData)
{
	g_return_if_fail(XFDASHBOARD_IS_SEARCH_VIEW(self));
	g_return_if_fail(inProviderData);

	/* If result set for provider is given then check if we need to create a container
	 * or if we have to update one ...
	 */
	if(inProviderData->lastResultSet &&
		xfdashboard_search_result_set_get_size(inProviderData->lastResultSet)>0)
	{
		/* Create container for provider if it does not exist yet */
		if(!inProviderData->container)
		{
			ClutterActor		*actor;

			actor=xfdashboard_search_result_container_new(inProviderData->provider);
			if(!actor) return;

			inProviderData->container=actor;
			clutter_actor_add_child(CLUTTER_ACTOR(self), inProviderData->container);

			g_signal_connect(inProviderData->container,
								"icon-clicked",
								G_CALLBACK(_xfdashboard_search_view_on_provider_icon_clicked),
								inProviderData);
		}

		/* Create actors for new result set items in container */
		inProviderData->lastResultItemActorSeen=NULL;
		xfdashboard_search_result_set_foreach(inProviderData->lastResultSet,
												_xfdashboard_search_view_update_provider_actor_new,
												inProviderData);

		/* Destroy actors for result set items in container which are not existent anymore */
		if(inProviderData->mappings)
		{
			g_list_foreach(inProviderData->mappings,
							(GFunc)_xfdashboard_search_view_update_provider_actor_destroy,
							inProviderData->lastResultSet);
		}
	}
		/* ... but if no result set for provider is given then destroy existing container */
		else
		{
			/* Destroy mappings (releases allocated resources and disconnects signal handlers) */
			if(inProviderData->mappings)
			{
				g_list_free_full(inProviderData->mappings, (GDestroyNotify)_xfdashboard_search_view_provider_item_mapping_free);
				inProviderData->mappings=NULL;
			}

			/* Destroy container (also destroys actors for result items still alive) */
			if(inProviderData->container)
			{
				clutter_actor_destroy(inProviderData->container);
				inProviderData->container=NULL;
			}
		}
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
		data=_xfdashboard_search_view_provider_data_new(inProviderType);
		data->view=self;
		priv->providers=g_list_prepend(priv->providers, data);

		g_debug("Created search provider %s of type %s in %s",
					xfdashboard_search_provider_get_name(data->provider),
					G_OBJECT_TYPE_NAME(data->provider),
					G_OBJECT_TYPE_NAME(self));
	}
}

/* A search provider was unregistered */
static void _xfdashboard_search_view_on_search_provider_unregistered(XfdashboardSearchView *self,
																		GType inProviderType,
																		gpointer inUserData)
{
	XfdashboardSearchViewPrivate		*priv;
	XfdashboardSearchViewProviderData	*data;
	GList								*entry;

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

		/* If provider data to free is the current selection reset selection to NULL.
		 * There is no need to unset focus and to unstyle selected item in search result container
		 * because it will be destroyed.
		 */
		if(data==priv->selectionProvider) priv->selectionProvider=NULL;

		/* Find list entry to remove */
		entry=g_list_find(priv->providers, data);
		if(entry) priv->providers=g_list_delete_link(priv->providers, entry);

		_xfdashboard_search_view_provider_data_free(data);
	}
}

/* IMPLEMENTATION: Interface XfdashboardFocusable */

/* Determine if actor can get the focus */
static gboolean _xfdashboard_search_view_focusable_can_focus(XfdashboardFocusable *inFocusable)
{
	XfdashboardSearchView			*self;
	XfdashboardFocusableInterface	*selfIface;
	XfdashboardFocusableInterface	*parentIface;

	g_return_val_if_fail(XFDASHBOARD_IS_FOCUSABLE(inFocusable), CLUTTER_EVENT_PROPAGATE);
	g_return_val_if_fail(XFDASHBOARD_IS_SEARCH_VIEW(inFocusable), CLUTTER_EVENT_PROPAGATE);

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

	/* If we get here this actor can be focused */
	return(TRUE);
}

/* Set focus to actor */
static void _xfdashboard_search_view_focusable_set_focus(XfdashboardFocusable *inFocusable)
{
	XfdashboardSearchView			*self;
	XfdashboardSearchViewPrivate	*priv;
	XfdashboardFocusableInterface	*selfIface;
	XfdashboardFocusableInterface	*parentIface;

	g_return_if_fail(XFDASHBOARD_IS_FOCUSABLE(inFocusable));
	g_return_if_fail(XFDASHBOARD_IS_SEARCH_VIEW(inFocusable));

	self=XFDASHBOARD_SEARCH_VIEW(inFocusable);
	priv=self->priv;

	/* Call parent class interface function */
	selfIface=XFDASHBOARD_FOCUSABLE_GET_IFACE(inFocusable);
	parentIface=g_type_interface_peek_parent(selfIface);

	if(parentIface && parentIface->set_focus)
	{
		parentIface->set_focus(inFocusable);
	}

	/* Reset selected item to first one */
	if(!priv->selectionProvider)
	{
		priv->selectionProvider=(XfdashboardSearchViewProviderData*)g_list_nth_data(priv->providers, 0);
	}

	if(priv->selectionProvider &&
		priv->selectionProvider->container)
	{
		ClutterActor				*item;

		/* Set focus to search result container of selected provider */
		xfdashboard_search_result_container_set_focus(XFDASHBOARD_SEARCH_RESULT_CONTAINER(priv->selectionProvider->container),
														TRUE);

		/* Get current selectionr and style it */
		item=xfdashboard_search_result_container_set_next_selection(XFDASHBOARD_SEARCH_RESULT_CONTAINER(priv->selectionProvider->container),
																	XFDASHBOARD_SEARCH_RESULT_CONTAINER_SELECTION_DIRECTION_BEGIN_END);
		if(item &&
			XFDASHBOARD_IS_STYLABLE(item))
		{
			xfdashboard_stylable_add_pseudo_class(XFDASHBOARD_STYLABLE(item), "selected");
		}
	}
}

/* Unset focus from actor */
static void _xfdashboard_search_view_focusable_unset_focus(XfdashboardFocusable *inFocusable)
{
	XfdashboardSearchView			*self;
	XfdashboardSearchViewPrivate	*priv;
	XfdashboardFocusableInterface	*selfIface;
	XfdashboardFocusableInterface	*parentIface;

	g_return_if_fail(XFDASHBOARD_IS_FOCUSABLE(inFocusable));
	g_return_if_fail(XFDASHBOARD_IS_SEARCH_VIEW(inFocusable));

	self=XFDASHBOARD_SEARCH_VIEW(inFocusable);
	priv=self->priv;

	/* Call parent class interface function */
	selfIface=XFDASHBOARD_FOCUSABLE_GET_IFACE(inFocusable);
	parentIface=g_type_interface_peek_parent(selfIface);

	if(parentIface && parentIface->set_focus)
	{
		parentIface->unset_focus(inFocusable);
	}

	/* Unstyle selected item */
	if(priv->selectionProvider &&
		priv->selectionProvider->container)
	{
		ClutterActor				*item;

		/* Get current selectionr and unstyle it */
		item=xfdashboard_search_result_container_get_current_selection(XFDASHBOARD_SEARCH_RESULT_CONTAINER(priv->selectionProvider->container));
		if(item &&
			XFDASHBOARD_IS_STYLABLE(item))
		{
			xfdashboard_stylable_remove_pseudo_class(XFDASHBOARD_STYLABLE(item), "selected");
		}

		/* Unset focus from search result container of selected provider */
		xfdashboard_search_result_container_set_focus(XFDASHBOARD_SEARCH_RESULT_CONTAINER(priv->selectionProvider->container),
														FALSE);
	}
}

/* Virtual function "handle_key_event" was called */
static gboolean _xfdashboard_search_view_focusable_handle_key_event(XfdashboardFocusable *inFocusable,
																	const ClutterEvent *inEvent)
{
	XfdashboardSearchView			*self;
	XfdashboardSearchViewPrivate	*priv;
	gboolean						handledEvent;

	g_return_val_if_fail(XFDASHBOARD_IS_FOCUSABLE(inFocusable), CLUTTER_EVENT_PROPAGATE);
	g_return_val_if_fail(XFDASHBOARD_IS_SEARCH_VIEW(inFocusable), CLUTTER_EVENT_PROPAGATE);

	self=XFDASHBOARD_SEARCH_VIEW(inFocusable);
	priv=self->priv;
	handledEvent=CLUTTER_EVENT_PROPAGATE;

	/* Handle key events at container of selected provider */
	if(priv->selectionProvider &&
		priv->selectionProvider->container &&
		clutter_event_type(inEvent)==CLUTTER_KEY_RELEASE)
	{
		ClutterActor				*currentSelection;
		ClutterActor				*newSelection;
		gboolean					doGetPrevious;
		gint						selectionDirection;

		// TODO: Check for current selection and for ENTER released
		//       to emit click signal on current selection

		/* Move selection if an corresponding key was pressed */
		switch(inEvent->key.keyval)
		{
			case CLUTTER_KEY_Up:
				selectionDirection=XFDASHBOARD_SEARCH_RESULT_CONTAINER_SELECTION_DIRECTION_ROW;
				doGetPrevious=TRUE;
				handledEvent=CLUTTER_EVENT_STOP;
				break;

			case CLUTTER_KEY_Down:
				selectionDirection=XFDASHBOARD_SEARCH_RESULT_CONTAINER_SELECTION_DIRECTION_ROW;
				doGetPrevious=FALSE;
				handledEvent=CLUTTER_EVENT_STOP;
				break;

			default:
				break;
		}

		if(handledEvent==CLUTTER_EVENT_STOP)
		{
			/* Get current selection to determine if selection has changed */
			currentSelection=xfdashboard_search_result_container_get_current_selection(XFDASHBOARD_SEARCH_RESULT_CONTAINER(priv->selectionProvider->container));

			/* Get new selection */
			if(doGetPrevious)
			{
				newSelection=xfdashboard_search_result_container_set_previous_selection(XFDASHBOARD_SEARCH_RESULT_CONTAINER(priv->selectionProvider->container),
																						selectionDirection);
			}
				else
				{
					newSelection=xfdashboard_search_result_container_set_next_selection(XFDASHBOARD_SEARCH_RESULT_CONTAINER(priv->selectionProvider->container),
																						selectionDirection);
				}

			/* If new selection is NULL move to next or previous container */
			if(!newSelection)
			{
				GList								*newSelectionProviderInList;
				XfdashboardSearchViewProviderData	*newSelectionProvider;

				/* Get new selected provider to focus */
				newSelectionProviderInList=g_list_find(priv->providers, priv->selectionProvider);
				if(doGetPrevious)
				{
					/* Get previous provider to focus */
					newSelectionProviderInList=g_list_previous(newSelectionProviderInList);
					if(!newSelectionProviderInList) newSelectionProviderInList=g_list_last(priv->providers);
					newSelectionProvider=(XfdashboardSearchViewProviderData*)newSelectionProviderInList->data;

					/* Unset focus at current provider and set focus to new one if changed */
					if(newSelectionProvider!=priv->selectionProvider)
					{
						xfdashboard_search_result_container_set_focus(XFDASHBOARD_SEARCH_RESULT_CONTAINER(priv->selectionProvider->container),
																		FALSE);

						priv->selectionProvider=newSelectionProvider;
						xfdashboard_search_result_container_set_focus(XFDASHBOARD_SEARCH_RESULT_CONTAINER(priv->selectionProvider->container),
																		TRUE);
					}

					/* Get last selectable item at newly selected provider */
					newSelection=xfdashboard_search_result_container_set_previous_selection(XFDASHBOARD_SEARCH_RESULT_CONTAINER(priv->selectionProvider->container),
																							XFDASHBOARD_SEARCH_RESULT_CONTAINER_SELECTION_DIRECTION_BEGIN_END);
				}
					else
					{
						/* Get previous provider to focus */
						newSelectionProviderInList=g_list_next(newSelectionProviderInList);
						if(!newSelectionProviderInList) newSelectionProviderInList=g_list_first(priv->providers);
						newSelectionProvider=(XfdashboardSearchViewProviderData*)newSelectionProviderInList->data;

						/* Unset focus at current provider and set focus to new one if changed */
						if(newSelectionProvider!=priv->selectionProvider)
						{
							xfdashboard_search_result_container_set_focus(XFDASHBOARD_SEARCH_RESULT_CONTAINER(priv->selectionProvider->container),
																			FALSE);

							priv->selectionProvider=newSelectionProvider;
							xfdashboard_search_result_container_set_focus(XFDASHBOARD_SEARCH_RESULT_CONTAINER(priv->selectionProvider->container),
																			TRUE);
						}

						/* Get last selectable item at newly selected provider */
						newSelection=xfdashboard_search_result_container_set_next_selection(XFDASHBOARD_SEARCH_RESULT_CONTAINER(priv->selectionProvider->container),
																							XFDASHBOARD_SEARCH_RESULT_CONTAINER_SELECTION_DIRECTION_BEGIN_END);
					}
			}

			/* Unstyle current selected item and style new selected item */
			if(currentSelection) xfdashboard_stylable_remove_pseudo_class(XFDASHBOARD_STYLABLE(currentSelection), "selected");
			if(newSelection) xfdashboard_stylable_add_pseudo_class(XFDASHBOARD_STYLABLE(newSelection), "selected");

			/* Event was handled */
			return(CLUTTER_EVENT_STOP);
		}
	}

	/* Return result of key handling */
	return(handledEvent);
}

/* Interface initialization
 * Set up default functions
 */
void _xfdashboard_search_view_focusable_iface_init(XfdashboardFocusableInterface *iface)
{
	iface->can_focus=_xfdashboard_search_view_focusable_can_focus;
	iface->set_focus=_xfdashboard_search_view_focusable_set_focus;
	iface->unset_focus=_xfdashboard_search_view_focusable_unset_focus;
	iface->handle_key_event=_xfdashboard_search_view_focusable_handle_key_event;
}


/* IMPLEMENTATION: GObject */

/* Dispose this object */
static void _xfdashboard_search_view_dispose(GObject *inObject)
{
	XfdashboardSearchView			*self=XFDASHBOARD_SEARCH_VIEW(inObject);
	XfdashboardSearchViewPrivate	*priv=self->priv;

	/* Release allocated resources */
	if(priv->selectionProvider)
	{
		priv->selectionProvider=NULL;
	}

	if(priv->providers)
	{
		g_list_free_full(priv->providers, (GDestroyNotify)_xfdashboard_search_view_provider_data_free);
		priv->providers=NULL;
	}

	if(priv->lastSearchString)
	{
		g_free(priv->lastSearchString);
		priv->lastSearchString=NULL;
	}

	if(priv->lastSearchTermsList)
	{
		g_strfreev(priv->lastSearchTermsList);
		priv->lastSearchTermsList=NULL;
	}

	if(priv->searchManager)
	{
		g_signal_handlers_disconnect_by_data(priv->searchManager, self);
		g_object_unref(priv->searchManager);
		priv->searchManager=NULL;
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
	priv->lastSearchString=NULL;
	priv->lastSearchTermsList=NULL;
	priv->selectionProvider=NULL;

	/* Set up view (Note: Search view is disabled by default!) */
	xfdashboard_view_set_internal_name(XFDASHBOARD_VIEW(self), "search");
	xfdashboard_view_set_name(XFDASHBOARD_VIEW(self), _("Search"));
	xfdashboard_view_set_icon(XFDASHBOARD_VIEW(self), GTK_STOCK_FIND);
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

		providerType=(GType)LISTITEM_TO_GTYPE(entry->data);
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
	gint64							searchTimestamp;

	g_return_if_fail(XFDASHBOARD_IS_SEARCH_VIEW(self));

	priv=self->priv;

	/* Release all allocated resources used for each search provider */
	searchTimestamp=g_get_real_time();
	for(providers=priv->providers; providers; providers=g_list_next(providers))
	{
		XfdashboardSearchViewProviderData	*providerData;

		/* Get data for provider to reset */
		providerData=(XfdashboardSearchViewProviderData*)providers->data;

		/* Update search timestamp */
		providerData->lastSearchTimestamp=searchTimestamp;

		/* Update view for empty result set of search provider:
		 * Destroy all item mappings first, then destroy container for provider
		 * (will also destroy actors for result items which will not trigger
		 * the "destroy" signal handler because it was disconnected before
		 * when item mapping had been destroyed), and at last destroy last result set
		 */
		if(providerData->mappings)
		{
			g_list_free_full(providerData->mappings, (GDestroyNotify)_xfdashboard_search_view_provider_item_mapping_free);
			providerData->mappings=NULL;
		}

		if(providerData->container)
		{
			clutter_actor_destroy(providerData->container);
			providerData->container=NULL;
		}

		if(providerData->lastResultSet)
		{
			g_object_unref(providerData->lastResultSet);
			providerData->lastResultSet=NULL;
		}

		g_debug("Resetting result set for %s", DEBUG_OBJECT_NAME(providerData->provider));
	}

	/* Unset last search terms */
	if(priv->lastSearchString)
	{
		g_free(priv->lastSearchString);
		priv->lastSearchString=NULL;
	}

	if(priv->lastSearchTermsList)
	{
		g_strfreev(priv->lastSearchTermsList);
		priv->lastSearchTermsList=NULL;
	}

	/* Emit signal that search was resetted */
	g_signal_emit(self, XfdashboardSearchViewSignals[SIGNAL_SEARCH_RESET], 0);
}

/* Start a new search or update an ongoing one */
void xfdashboard_search_view_update_search(XfdashboardSearchView *self, const gchar *inSearchString)
{
	XfdashboardSearchViewPrivate	*priv;
	gchar							**splittedSearchTerms;
	gboolean						canSubsearch;
	GList							*providers;
	gint64							searchTimestamp;
#ifdef DEBUG
	GTimer								*timer=NULL;
#endif

	g_return_if_fail(XFDASHBOARD_IS_SEARCH_VIEW(self));

	priv=self->priv;

	/* Only perform a search if new search term differs from old one */
	if(g_strcmp0(inSearchString, priv->lastSearchString)==0) return;

	/* Searching for NULL or an empty string is like resetting search */
	if(!inSearchString || strlen(inSearchString)==0)
	{
		xfdashboard_search_view_reset_search(self);
		return;
	}

	/* Split search terms into seperate trimmed strings */
	splittedSearchTerms=xfdashboard_search_manager_get_search_terms_from_string(inSearchString, NULL);
	if(!splittedSearchTerms)
	{
		xfdashboard_search_view_reset_search(self);
		return;
	}

	/* Searching empty search terms is like resetting search */
	if(g_strv_length(splittedSearchTerms)==0)
	{
		/* Release allocated resources */
		g_strfreev(splittedSearchTerms);

		/* Reset search */
		xfdashboard_search_view_reset_search(self);
		return;
	}

#ifdef DEBUG
	/* Start timer for debug search performance */
	timer=g_timer_new();
#endif

	/* Check for sub-search. A sub-search can be done if a (initial) search
	 * was done before, the order of search terms has not changed and each term
	 * in new search terms is a case-sensitive prefix of the term previously used.
	 */
	canSubsearch=FALSE;
	if(splittedSearchTerms && priv->lastSearchTermsList)
	{
		gchar						**subsearchLeft, **subsearchRight;

		subsearchLeft=priv->lastSearchTermsList;
		subsearchRight=splittedSearchTerms;
		while(*subsearchLeft && *subsearchRight)
		{
			if(g_strcmp0(*subsearchLeft, *subsearchRight)>0) break;

			subsearchLeft++;
			subsearchRight++;
		}

		if(*subsearchLeft==NULL && *subsearchRight==NULL) canSubsearch=TRUE;
	}

	/* Perform a full search or sub-search at all registered providers */
	searchTimestamp=g_get_real_time();
	for(providers=priv->providers; providers; providers=g_list_next(providers))
	{
		XfdashboardSearchViewProviderData		*providerData;
		XfdashboardSearchResultSet				*providerNewResultSet;
		XfdashboardSearchResultSet				*providerLastResultSet;

		/* Get data for provider to perform search at */
		providerData=(XfdashboardSearchViewProviderData*)providers->data;

		/* Perform search */
		if(canSubsearch) providerLastResultSet=providerData->lastResultSet;
			else providerLastResultSet=NULL;

		providerNewResultSet=xfdashboard_search_provider_get_result_set(providerData->provider,
																		splittedSearchTerms,
																		providerLastResultSet);

		/* Remember new result set */
		if(providerData->lastResultSet) g_object_unref(providerData->lastResultSet);
		providerData->lastResultSet=providerNewResultSet;

		/* Update search timestamp */
		providerData->lastSearchTimestamp=searchTimestamp;

		/* Update view for new result set of search provider */
		_xfdashboard_search_view_update_provider_container(self, providerData);
	}

	/* Remember new search terms as last one */
	if(priv->lastSearchString) g_free(priv->lastSearchString);
	priv->lastSearchString=g_strdup(inSearchString);

	if(priv->lastSearchTermsList) g_strfreev(priv->lastSearchTermsList);
	priv->lastSearchTermsList=g_strdupv(splittedSearchTerms);

	/* Release allocated resources */
	g_strfreev(splittedSearchTerms);

	/* Emit signal that search was updated */
	g_signal_emit(self, XfdashboardSearchViewSignals[SIGNAL_SEARCH_UPDATED], 0);

#ifdef DEBUG
	g_debug("Updating search for '%s' took %f seconds", inSearchString, g_timer_elapsed(timer, NULL));
	g_timer_destroy(timer);
#endif
}
