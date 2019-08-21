/*
 * search-result-container: An container for results from a search provider
 *                          which has a header and container for items
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

#include <libxfdashboard/search-result-container.h>

#include <glib/gi18n-lib.h>
#include <gdk/gdk.h>
#include <math.h>

#include <libxfdashboard/enums.h>
#include <libxfdashboard/text-box.h>
#include <libxfdashboard/button.h>
#include <libxfdashboard/stylable.h>
#include <libxfdashboard/dynamic-table-layout.h>
#include <libxfdashboard/utils.h>
#include <libxfdashboard/click-action.h>
#include <libxfdashboard/drag-action.h>
#include <libxfdashboard/marshal.h>
#include <libxfdashboard/compat.h>
#include <libxfdashboard/debug.h>


/* Define this class in GObject system */
struct _XfdashboardSearchResultContainerPrivate
{
	/* Properties related */
	XfdashboardSearchProvider	*provider;

	gchar						*icon;
	gchar						*titleFormat;
	XfdashboardViewMode			viewMode;
	gfloat						spacing;
	gfloat						padding;

	gint						initialResultsCount;
	gint						moreResultsCount;

	/* Instance related */
	ClutterLayoutManager		*layout;
	ClutterActor				*titleTextBox;
	ClutterActor				*itemsContainer;

	gpointer					selectedItem;
	guint						selectedItemDestroySignalID;

	GHashTable					*mapping;
	XfdashboardSearchResultSet	*lastResultSet;

	gboolean					maxResultsItemsCountSet;
	gint						maxResultsItemsCount;
	ClutterActor				*moreResultsLabelActor;
	ClutterActor				*allResultsLabelActor;
};

G_DEFINE_TYPE_WITH_PRIVATE(XfdashboardSearchResultContainer,
							xfdashboard_search_result_container,
							XFDASHBOARD_TYPE_ACTOR)

/* Properties */
enum
{
	PROP_0,

	PROP_PROVIDER,

	PROP_ICON,
	PROP_TITLE_FORMAT,
	PROP_VIEW_MODE,
	PROP_SPACING,
	PROP_PADDING,

	PROP_INITIAL_RESULTS_SIZE,
	PROP_MORE_RESULTS_SIZE,

	PROP_LAST
};

static GParamSpec* XfdashboardSearchResultContainerProperties[PROP_LAST]={ 0, };

/* Signals */
enum
{
	SIGNAL_ICON_CLICKED,
	SIGNAL_ITEM_CLICKED,

	SIGNAL_LAST
};

static guint XfdashboardSearchResultContainerSignals[SIGNAL_LAST]={ 0, };

/* IMPLEMENTATION: Private variables and methods */
#define DEFAULT_VIEW_MODE				XFDASHBOARD_VIEW_MODE_LIST
#define DEFAULT_INITIAL_RESULT_SIZE		5
#define DEFAULT_MORE_RESULT_SIZE		5

/* Forward declarations */
static void _xfdashboard_search_result_container_update_selection(XfdashboardSearchResultContainer *self,
																	ClutterActor *inNewSelectedItem);

static void _xfdashboard_search_result_container_update_result_items(XfdashboardSearchResultContainer *self,
																		XfdashboardSearchResultSet *inResultSet,
																		gboolean inShowAllItems);

/* The current selected item will be destroyed so move selection */
static void _xfdashboard_search_result_container_on_destroy_selection(XfdashboardSearchResultContainer *self,
																		gpointer inUserData)
{
	XfdashboardSearchResultContainerPrivate		*priv;
	ClutterActor								*actor;
	ClutterActor								*newSelection;

	g_return_if_fail(XFDASHBOARD_IS_SEARCH_RESULT_CONTAINER(self));
	g_return_if_fail(CLUTTER_IS_ACTOR(inUserData));

	priv=self->priv;
	actor=CLUTTER_ACTOR(inUserData);

	/* Only move selection if destroyed actor is the selected one. */
	if(actor!=priv->selectedItem) return;

	/* Get actor following the destroyed one. If we do not find an actor
	 * get the previous one. If there is no previous one set NULL selection.
	 * This should work well for both - icon and list mode.
	 */
	newSelection=clutter_actor_get_next_sibling(actor);
	if(!newSelection) newSelection=clutter_actor_get_previous_sibling(actor);

	/* Move selection */
	_xfdashboard_search_result_container_update_selection(self, newSelection);
}

/* Set new selection, connect to signals and release old signal connections */
static void _xfdashboard_search_result_container_update_selection(XfdashboardSearchResultContainer *self,
																	ClutterActor *inNewSelectedItem)
{
	XfdashboardSearchResultContainerPrivate		*priv;

	g_return_if_fail(XFDASHBOARD_IS_SEARCH_RESULT_CONTAINER(self));
	g_return_if_fail(!inNewSelectedItem || CLUTTER_IS_ACTOR(inNewSelectedItem));

	priv=self->priv;

	/* Unset current selection and signal handler ID */
	if(priv->selectedItem)
	{
		/* Remove weak reference at current selection */
		g_object_remove_weak_pointer(G_OBJECT(priv->selectedItem), &priv->selectedItem);

		/* Disconnect signal handler */
		if(priv->selectedItemDestroySignalID)
		{
			g_signal_handler_disconnect(priv->selectedItem, priv->selectedItemDestroySignalID);
		}

		/* Unstyle current selection */
		if(XFDASHBOARD_IS_STYLABLE(priv->selectedItem))
		{
			/* Style new selection */
			xfdashboard_stylable_remove_pseudo_class(XFDASHBOARD_STYLABLE(priv->selectedItem), "selected");
		}
	}

	priv->selectedItem=NULL;
	priv->selectedItemDestroySignalID=0;

	/* Set new selection and connect signals if given */
	if(inNewSelectedItem)
	{
		priv->selectedItem=inNewSelectedItem;

		/* Add weak reference at new selection */
		g_object_add_weak_pointer(G_OBJECT(inNewSelectedItem), &priv->selectedItem);

		/* Connect signals */
		g_signal_connect_swapped(inNewSelectedItem,
									"destroy",
									G_CALLBACK(_xfdashboard_search_result_container_on_destroy_selection),
									self);

		/* Style new selection */
		if(XFDASHBOARD_IS_STYLABLE(inNewSelectedItem))
		{
			xfdashboard_stylable_add_pseudo_class(XFDASHBOARD_STYLABLE(inNewSelectedItem), "selected");
		}
	}
}

/* Primary icon (provider icon) in title was clicked */
static void _xfdashboard_search_result_container_on_primary_icon_clicked(XfdashboardSearchResultContainer *self, gpointer inUserData)
{
	g_return_if_fail(XFDASHBOARD_IS_SEARCH_RESULT_CONTAINER(self));

	/* Emit signal for icon clicked */
	g_signal_emit(self, XfdashboardSearchResultContainerSignals[SIGNAL_ICON_CLICKED], 0);
}

/* "More results" label was clicked */
static void _xfdashboard_search_result_container_on_more_results_label_clicked(XfdashboardSearchResultContainer *self, gpointer inUserData)
{
	XfdashboardSearchResultContainerPrivate		*priv;

	g_return_if_fail(XFDASHBOARD_IS_SEARCH_RESULT_CONTAINER(self));

	priv=self->priv;

	/* If this is the first time the maximum number of actors is determined
	 * then set it to initial number.
	 */
	if(!priv->maxResultsItemsCountSet)
	{
		priv->maxResultsItemsCount=priv->initialResultsCount;
		priv->maxResultsItemsCountSet=TRUE;
	}

	/* Increase maximum number of actor by number of more actors */
	priv->maxResultsItemsCount+=priv->moreResultsCount;

	/* Update container */
	_xfdashboard_search_result_container_update_result_items(self, priv->lastResultSet, FALSE);
}

/* "All results" label was clicked */
static void _xfdashboard_search_result_container_on_all_results_label_clicked(XfdashboardSearchResultContainer *self, gpointer inUserData)
{
	XfdashboardSearchResultContainerPrivate		*priv;

	g_return_if_fail(XFDASHBOARD_IS_SEARCH_RESULT_CONTAINER(self));

	priv=self->priv;

	/* Update container */
	_xfdashboard_search_result_container_update_result_items(self, priv->lastResultSet, TRUE);
}

/* Update icon at text box */
static void _xfdashboard_search_result_container_update_icon(XfdashboardSearchResultContainer *self)
{
	XfdashboardSearchResultContainerPrivate		*priv;
	const gchar									*icon;
	const gchar									*providerIcon;

	g_return_if_fail(XFDASHBOARD_IS_SEARCH_RESULT_CONTAINER(self));

	priv=self->priv;

	/* Set icon set via stylable property or use the icon the search provider defines.
	 * If both are not set then icon is NULL which cause the icon in text box to be hidden.
	 */
	icon=priv->icon;
	if(!icon)
	{
		providerIcon=xfdashboard_search_provider_get_icon(priv->provider);
		if(providerIcon) icon=providerIcon;
	}

	xfdashboard_text_box_set_primary_icon(XFDASHBOARD_TEXT_BOX(priv->titleTextBox), icon);
}

/* Update title at text box */
static void _xfdashboard_search_result_container_update_title(XfdashboardSearchResultContainer *self)
{
	XfdashboardSearchResultContainerPrivate		*priv;
	const gchar									*providerName;

	g_return_if_fail(XFDASHBOARD_IS_SEARCH_RESULT_CONTAINER(self));

	priv=self->priv;

	/* Get provider name */
	providerName=xfdashboard_search_provider_get_name(priv->provider);

	/* Format title text and set it */
	if(priv->titleFormat)
	{
		xfdashboard_text_box_set_text_va(XFDASHBOARD_TEXT_BOX(priv->titleTextBox), priv->titleFormat, providerName);
	}
		else
		{
			xfdashboard_text_box_set_text(XFDASHBOARD_TEXT_BOX(priv->titleTextBox), providerName);
		}
}

/* A result item actor is going to be destroyed */
static void _xfdashboard_search_result_container_on_result_item_actor_destroyed(ClutterActor *inActor, gpointer inUserData)
{
	XfdashboardSearchResultContainer			*self;
	XfdashboardSearchResultContainerPrivate		*priv;
	GHashTableIter								iter;
	GVariant									*key;
	ClutterActor								*value;

	g_return_if_fail(CLUTTER_IS_ACTOR(inActor));
	g_return_if_fail(XFDASHBOARD_IS_SEARCH_RESULT_CONTAINER(inUserData));

	self=XFDASHBOARD_SEARCH_RESULT_CONTAINER(inUserData);
	priv=self->priv;

	/* First disconnect signal handlers from actor before modifying mapping hash table */
	g_signal_handlers_disconnect_by_data(inActor, self);

	/* Iterate through mapping and remove each key whose value matches actor
	 * going to be destroyed and remove it from mapping hash table.
	 */
	g_hash_table_iter_init(&iter, priv->mapping);
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

/* Activate result item by actor, e.g. actor was clicked */
static void _xfdashboard_search_result_container_activate_result_item_by_actor(XfdashboardSearchResultContainer *self,
																				ClutterActor *inActor)
{
	XfdashboardSearchResultContainerPrivate		*priv;
	GHashTableIter								iter;
	GVariant									*key;
	ClutterActor								*value;

	g_return_if_fail(XFDASHBOARD_IS_SEARCH_RESULT_CONTAINER(self));
	g_return_if_fail(CLUTTER_IS_ACTOR(inActor));

	priv=self->priv;

	/* Iterate through mapping and at first key whose value matching actor
	 * tell provider to activate item.
	 */
	g_hash_table_iter_init(&iter, priv->mapping);
	while(g_hash_table_iter_next(&iter, (gpointer*)&key, (gpointer*)&value))
	{
		/* If value of key iterated matches actor activate item */
		if(value==inActor)
		{
			/* Emit signal that a result item was clicked */
			g_signal_emit(self,
							XfdashboardSearchResultContainerSignals[SIGNAL_ITEM_CLICKED],
							0,
							key,
							inActor);

			/* All done so return here */
			return;
		}
	}
}

/* A result item actor was clicked */
static void _xfdashboard_search_result_container_on_result_item_actor_clicked(XfdashboardClickAction *inAction,
																				ClutterActor *inActor,
																				gpointer inUserData)
{
	XfdashboardSearchResultContainer			*self;

	g_return_if_fail(CLUTTER_IS_ACTOR(inActor));
	g_return_if_fail(XFDASHBOARD_IS_SEARCH_RESULT_CONTAINER(inUserData));

	self=XFDASHBOARD_SEARCH_RESULT_CONTAINER(inUserData);

	/* Only emit any of these signals if click was perform with left button 
	 * or is a short touchscreen touch event.
	 */
	if(xfdashboard_click_action_is_left_button_or_tap(inAction))
	{
		/* Activate result item by actor clicked */
		_xfdashboard_search_result_container_activate_result_item_by_actor(self, inActor);
	}
}

/* Get and set up actor for result item from search provider */
static ClutterActor* _xfdashboard_search_result_container_result_item_actor_new(XfdashboardSearchResultContainer *self,
																				GVariant *inResultItem)
{
	XfdashboardSearchResultContainerPrivate		*priv;
	ClutterActor								*actor;
	ClutterAction								*action;
	GList										*actions;
	GList										*actionsIter;

	g_return_val_if_fail(XFDASHBOARD_IS_SEARCH_RESULT_CONTAINER(self), NULL);
	g_return_val_if_fail(inResultItem, NULL);

	priv=self->priv;

	/* Check if search provider is set */
	g_return_val_if_fail(priv->provider, NULL);

	/* Create actor for item */
	actor=xfdashboard_search_provider_create_result_actor(priv->provider, inResultItem);
	if(!actor)
	{
		gchar			*resultItemText;

		resultItemText=g_variant_print(inResultItem, TRUE);
		g_warning(_("Failed to add actor for result item %s of provider %s: Could not create actor"),
					resultItemText,
					G_OBJECT_TYPE_NAME(priv->provider));
		g_free(resultItemText);

		return(NULL);
	}

	if(!CLUTTER_IS_ACTOR(actor))
	{
		gchar			*resultItemText;

		resultItemText=g_variant_print(inResultItem, TRUE);
		g_critical(_("Failed to add actor for result item %s of provider %s: Actor of type %s is not derived from class %s"),
					resultItemText,
					G_OBJECT_TYPE_NAME(priv->provider),
					G_IS_OBJECT(actor) ? G_OBJECT_TYPE_NAME(actor) : "<unknown>",
					g_type_name(CLUTTER_TYPE_ACTOR));
		g_free(resultItemText);

		/* Release allocated resources */
		clutter_actor_destroy(actor);

		return(NULL);
	}

	/* Connect to 'destroy' signal of actor to remove it from mapping hash table
	 * if actor was destroyed (accidently)
	 */
	g_signal_connect(actor,
						"destroy",
						G_CALLBACK(_xfdashboard_search_result_container_on_result_item_actor_destroyed),
						self);

	/* Add click action to actor and connect signal */
	action=xfdashboard_click_action_new();
	clutter_actor_add_action(actor, action);
	g_signal_connect(action,
						"clicked",
						G_CALLBACK(_xfdashboard_search_result_container_on_result_item_actor_clicked),
						self);

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

	/* Set style depending on view mode */
	if(XFDASHBOARD_IS_STYLABLE(actor))
	{
		if(priv->viewMode==XFDASHBOARD_VIEW_MODE_LIST) xfdashboard_stylable_add_class(XFDASHBOARD_STYLABLE(actor), "view-mode-list");
			else xfdashboard_stylable_add_class(XFDASHBOARD_STYLABLE(actor), "view-mode-icon");

		xfdashboard_stylable_add_class(XFDASHBOARD_STYLABLE(actor), "result-item");
	}

	/* Set up actor for items container */
	clutter_actor_set_x_expand(actor, TRUE);

	/* Return newly created actor */
	return(actor);
}

/* Sets provider this result container is for */
static void _xfdashboard_search_result_container_set_provider(XfdashboardSearchResultContainer *self,
																XfdashboardSearchProvider *inProvider)
{
	XfdashboardSearchResultContainerPrivate		*priv;
	gchar										*styleClass;

	g_return_if_fail(XFDASHBOARD_IS_SEARCH_RESULT_CONTAINER(self));
	g_return_if_fail(XFDASHBOARD_IS_SEARCH_PROVIDER(inProvider));

	priv=self->priv;

	/* Check that no provider was set yet */
	g_return_if_fail(priv->provider==NULL);

	/* Set provider reference */
	priv->provider=XFDASHBOARD_SEARCH_PROVIDER(g_object_ref(inProvider));

	/* Set class name with name of search provider for styling */
	styleClass=g_strdup_printf("search-provider-%s", G_OBJECT_TYPE_NAME(priv->provider));
	xfdashboard_stylable_add_class(XFDASHBOARD_STYLABLE(self), styleClass);
	g_free(styleClass);

	/* Set class name with ID of search provider for styling */
	styleClass=g_strdup_printf("search-provider-id-%s", xfdashboard_search_provider_get_id(priv->provider));
	xfdashboard_stylable_add_class(XFDASHBOARD_STYLABLE(self), styleClass);
	g_free(styleClass);

	/* Update icon */
	_xfdashboard_search_result_container_update_icon(self);

	/* Update title */
	_xfdashboard_search_result_container_update_title(self);
}

/* Update result items in container */
static void _xfdashboard_search_result_container_update_result_items(XfdashboardSearchResultContainer *self, XfdashboardSearchResultSet *inResultSet, gboolean inShowAllItems)
{
	XfdashboardSearchResultContainerPrivate		*priv;
	GList										*allList;
	GList										*removeList;
	GList										*iter;
	GVariant									*resultItem;
	ClutterActor								*actor;

	g_return_if_fail(XFDASHBOARD_IS_SEARCH_RESULT_CONTAINER(self));
	g_return_if_fail(XFDASHBOARD_IS_SEARCH_RESULT_SET(inResultSet));

	priv=self->priv;

	/* Check if search provider is set */
	g_return_if_fail(priv->provider);

	/* Take extra reference on given result set to keep it alive as it may be
	 * exactly the same result set as the one before and if we update the reference
	 * to this result set as last one seen we have to unref the last result set
	 * which is the same is the given one. That could destroy the result set.
	 * To prevent this we take an extra reference here and release it at the end
	 * of this function.
	 */
	g_object_ref(inResultSet);

	/* Determine list of items whose actors to remove from container by checking
	 * which result item was in last known result set but is not in given one
	 * anymore.
	 */
	removeList=NULL;
	if(priv->lastResultSet) removeList=xfdashboard_search_result_set_complement(inResultSet, priv->lastResultSet);

	/* Create actor for each item in list which is new to mapping */
	allList=xfdashboard_search_result_set_get_all(inResultSet);
	if(allList)
	{
		ClutterActor							*lastActor;
		gint									actorsCount;
		gint									allItemsCount;

		/* Get number of all result items */
		allItemsCount=g_list_length(allList);

		/* If this is the first time the maximum number of actors is determined
		 * then set it to initial number.
		 */
		if(!priv->maxResultsItemsCountSet)
		{
			priv->maxResultsItemsCount=priv->initialResultsCount;
			priv->maxResultsItemsCountSet=TRUE;
		}

		/* If maximum number of actors to show in items container is zero
		 * then all result items should be shown.
		 */
		if(priv->maxResultsItemsCount<=0) inShowAllItems=TRUE;

		/* Get current number of result actors but decrease it by the number
		 * of actors which will be removed.
		 */
		actorsCount=clutter_actor_get_n_children(priv->itemsContainer);
		if(removeList)
		{
			for(iter=removeList; iter && actorsCount>0; iter=g_list_next(iter))
			{
				/* Get result item to remove */
				resultItem=(GVariant*)iter->data;

				/* Get actor to remove */
				if(g_hash_table_lookup_extended(priv->mapping, resultItem, NULL, (gpointer*)&actor))
				{
					if(actor) actorsCount--;
				}
			}
		}

		/* Iterate through list of result items and add actor for each result item
		 * which has no actor currently but do not exceed maximum number of actors
		 * we just determined above.
		 */
		lastActor=NULL;
		for(iter=allList; iter && (inShowAllItems || actorsCount<=priv->maxResultsItemsCount); iter=g_list_next(iter))
		{
			/* Get result item to add */
			resultItem=(GVariant*)iter->data;

			/* If result item does not exist in mapping then create actor and
			 * add it to mapping.
			 */
			if(!g_hash_table_lookup_extended(priv->mapping, resultItem, NULL, (gpointer*)&actor))
			{
				/* Increase actor counter and if it exceeds maximum number of
				 * allowed actor continue with next result item in result set
				 * which will not happen as maximum has exceeded.
				 *
				 */
				actorsCount++;
				if(!inShowAllItems && actorsCount>priv->maxResultsItemsCount) continue;

				/* Create actor for result item and add to this container */
				actor=_xfdashboard_search_result_container_result_item_actor_new(self, resultItem);
				if(actor)
				{
					/* Add newly created actor to container of provider */
					if(!lastActor) clutter_actor_insert_child_below(priv->itemsContainer, actor, NULL);
						else clutter_actor_insert_child_above(priv->itemsContainer, actor, lastActor);

					/* Add actor to mapping hash table for result item */
					g_hash_table_insert(priv->mapping, g_variant_ref(resultItem), g_object_ref(actor));
				}
			}

			/* Remember either existing actor from hash table lookup or
			 * the newly created actor as the last one seen.
			 */
			if(actor) lastActor=actor;
		}

		/* If we tried to create at least one more actore than maximum allowed
		 * then set text at "more"-label otherwise set empty text to "hide" it
		 */
		if(!inShowAllItems && actorsCount>priv->maxResultsItemsCount)
		{
			gchar								*labelText;
			gint								moreCount;

			/* Get text to set at "more"-label */
			moreCount=MIN(allItemsCount-priv->maxResultsItemsCount, priv->moreResultsCount);
			labelText=g_strdup_printf(_("Show %d more results..."), moreCount);

			/* Set text at "more"-label */
			xfdashboard_label_set_text(XFDASHBOARD_LABEL(priv->moreResultsLabelActor), labelText);

			/* Release allocated resources */
			if(labelText) g_free(labelText);
		}
			else
			{
				/* Set empty text at "more"-label */
				xfdashboard_label_set_text(XFDASHBOARD_LABEL(priv->moreResultsLabelActor), NULL);
			}

		/* If we have more result items in result set than result items actors shown
		 * then set text at "all"-label otherwise set empty text to "hide" it
		 */
		if(!inShowAllItems && actorsCount<allItemsCount)
		{
			gchar								*labelText;

			/* Get text to set at "all"-label */
			labelText=g_strdup_printf(_("Show all %d results..."), allItemsCount);

			/* Set text at "all"-label */
			xfdashboard_label_set_text(XFDASHBOARD_LABEL(priv->allResultsLabelActor), labelText);

			/* Release allocated resources */
			if(labelText) g_free(labelText);
		}
			else
			{
				/* Set empty text at "all"-label */
				xfdashboard_label_set_text(XFDASHBOARD_LABEL(priv->allResultsLabelActor), NULL);
			}
	}

	/* Remove the actor for each item in remove list */
	if(removeList)
	{
		/* Iterate through list of items to remove and for each one remove actor
		 * and its entry in mapping hash table.
		 */
		for(iter=removeList; iter; iter=g_list_next(iter))
		{
			/* Get result item to remove */
			resultItem=(GVariant*)iter->data;

			/* Get actor to remove */
			if(g_hash_table_lookup_extended(priv->mapping, resultItem, NULL, (gpointer*)&actor))
			{
				/* Check if item has really an actor */
				if(!CLUTTER_IS_ACTOR(actor))
				{
					gchar		*resultItemText;

					resultItemText=g_variant_print(resultItem, TRUE);
					g_critical(_("Failed to remove actor for result item %s of provider %s: Actor of type %s is not derived from class %s"),
								resultItemText,
								G_OBJECT_TYPE_NAME(priv->provider),
								G_IS_OBJECT(actor) ? G_OBJECT_TYPE_NAME(actor) : "<unknown>",
								g_type_name(CLUTTER_TYPE_ACTOR));
					g_free(resultItemText);

					continue;
				}

				/* First disconnect signal handlers from actor before modifying mapping hash table */
				g_signal_handlers_disconnect_by_data(actor, self);

				/* Remove actor from mapping hash table before destroying it */
				g_hash_table_remove(priv->mapping, resultItem);

				/* Destroy actor and remove from hash table */
				clutter_actor_destroy(actor);
			}
		}
	}

	/* Remember new result set for search provider */
	if(priv->lastResultSet)
	{
		g_object_unref(priv->lastResultSet);
		priv->lastResultSet=NULL;
	}

	priv->lastResultSet=XFDASHBOARD_SEARCH_RESULT_SET(g_object_ref(inResultSet));

	/* Release allocated resources */
	if(removeList) g_list_free_full(removeList, (GDestroyNotify)g_variant_unref);
	if(allList) g_list_free_full(allList, (GDestroyNotify)g_variant_unref);

	/* Release extra reference we took at begin of this function */
	g_object_unref(inResultSet);
}

/* Find requested selection target depending of current selection in icon mode */
static ClutterActor* _xfdashboard_search_result_container_find_selection_from_icon_mode(XfdashboardSearchResultContainer *self,
																						ClutterActor *inSelection,
																						XfdashboardSelectionTarget inDirection,
																						XfdashboardView *inView,
																						gboolean inAllowWrap)
{
	XfdashboardSearchResultContainerPrivate		*priv;
	ClutterActor								*selection;
	ClutterActor								*newSelection;
	gint										numberChildren;
	gint										rows;
	gint										columns;
	gint										currentSelectionIndex;
	gint										currentSelectionRow;
	gint										currentSelectionColumn;
	gint										newSelectionIndex;
	ClutterActorIter							iter;
	ClutterActor								*child;
	gboolean									needsWrap;

	g_return_val_if_fail(XFDASHBOARD_IS_SEARCH_RESULT_CONTAINER(self), NULL);
	g_return_val_if_fail(CLUTTER_IS_ACTOR(inSelection), NULL);

	priv=self->priv;
	selection=inSelection;
	newSelection=NULL;
	needsWrap=FALSE;

	/* Get number of rows and columns and also get number of children
	 * of layout manager.
	 */
	numberChildren=xfdashboard_dynamic_table_layout_get_number_children(XFDASHBOARD_DYNAMIC_TABLE_LAYOUT(priv->layout));
	rows=xfdashboard_dynamic_table_layout_get_rows(XFDASHBOARD_DYNAMIC_TABLE_LAYOUT(priv->layout));
	columns=xfdashboard_dynamic_table_layout_get_columns(XFDASHBOARD_DYNAMIC_TABLE_LAYOUT(priv->layout));

	/* Get index of current selection */
	currentSelectionIndex=0;
	clutter_actor_iter_init(&iter, priv->itemsContainer);
	while(clutter_actor_iter_next(&iter, &child) &&
			child!=inSelection)
	{
		currentSelectionIndex++;
	}

	currentSelectionRow=(currentSelectionIndex / columns);
	currentSelectionColumn=(currentSelectionIndex % columns);

	/* Find target selection */
	switch(inDirection)
	{
		case XFDASHBOARD_SELECTION_TARGET_LEFT:
			currentSelectionColumn--;
			if(currentSelectionColumn<0)
			{
				currentSelectionRow++;
				newSelectionIndex=(currentSelectionRow*columns)-1;

				needsWrap=TRUE;
			}
				else newSelectionIndex=currentSelectionIndex-1;

			newSelectionIndex=MIN(newSelectionIndex, numberChildren-1);
			newSelection=clutter_actor_get_child_at_index(priv->itemsContainer, newSelectionIndex);
			break;

		case XFDASHBOARD_SELECTION_TARGET_RIGHT:
			currentSelectionColumn++;
			if(currentSelectionColumn==columns ||
				currentSelectionIndex==numberChildren)
			{
				newSelectionIndex=(currentSelectionRow*columns);
				needsWrap=TRUE;
			}
				else newSelectionIndex=currentSelectionIndex+1;

			newSelectionIndex=MIN(newSelectionIndex, numberChildren-1);
			newSelection=clutter_actor_get_child_at_index(priv->itemsContainer, newSelectionIndex);
			break;

		case XFDASHBOARD_SELECTION_TARGET_UP:
			currentSelectionRow--;
			if(currentSelectionRow<0)
			{
				currentSelectionRow=rows-1;
				needsWrap=TRUE;
			}
			newSelectionIndex=(currentSelectionRow*columns)+currentSelectionColumn;

			newSelectionIndex=MIN(newSelectionIndex, numberChildren-1);
			newSelection=clutter_actor_get_child_at_index(priv->itemsContainer, newSelectionIndex);
			break;

		case XFDASHBOARD_SELECTION_TARGET_DOWN:
			currentSelectionRow++;
			if(currentSelectionRow>=rows)
			{
				currentSelectionRow=0;
				needsWrap=TRUE;
			}
			newSelectionIndex=(currentSelectionRow*columns)+currentSelectionColumn;

			newSelectionIndex=MIN(newSelectionIndex, numberChildren-1);
			newSelection=clutter_actor_get_child_at_index(priv->itemsContainer, newSelectionIndex);
			break;

		case XFDASHBOARD_SELECTION_TARGET_PAGE_LEFT:
			newSelectionIndex=(currentSelectionRow*columns);
			newSelectionIndex=MIN(newSelectionIndex, numberChildren-1);
			newSelection=clutter_actor_get_child_at_index(priv->itemsContainer, newSelectionIndex);
			break;

		case XFDASHBOARD_SELECTION_TARGET_PAGE_RIGHT:
			newSelectionIndex=((currentSelectionRow+1)*columns)-1;
			newSelectionIndex=MIN(newSelectionIndex, numberChildren-1);
			newSelection=clutter_actor_get_child_at_index(priv->itemsContainer, newSelectionIndex);
			break;

		case XFDASHBOARD_SELECTION_TARGET_PAGE_UP:
			newSelectionIndex=currentSelectionColumn;
			newSelectionIndex=MIN(newSelectionIndex, numberChildren-1);
			newSelection=clutter_actor_get_child_at_index(priv->itemsContainer, newSelectionIndex);
			break;

		case XFDASHBOARD_SELECTION_TARGET_PAGE_DOWN:
			newSelectionIndex=((rows-1)*columns)+currentSelectionColumn;
			newSelectionIndex=MIN(newSelectionIndex, numberChildren-1);
			newSelection=clutter_actor_get_child_at_index(priv->itemsContainer, newSelectionIndex);
			break;

		default:
			{
				gchar							*valueName;

				valueName=xfdashboard_get_enum_value_name(XFDASHBOARD_TYPE_SELECTION_TARGET, inDirection);
				g_critical(_("Focusable object %s does not handle selection direction of type %s in icon mode."),
							G_OBJECT_TYPE_NAME(self),
							valueName);
				g_free(valueName);
			}
			break;
	}

	/* If new selection could be found override current selection with it.
	 * But also check if new selection needs to wrap (crossing boundaries
	 * like going to the beginning because it's gone beyond end) and if
	 * wrapping is allowed.
	 */
	if(newSelection) selection=newSelection;

	if(selection && needsWrap && !inAllowWrap) selection=NULL;

	/* Return new selection */
	XFDASHBOARD_DEBUG(self, ACTOR,
						"Selecting %s in icon mode at %s for current selection %s in direction %u with wrapping %s and wrapping %s",
						selection ? G_OBJECT_TYPE_NAME(selection) : "<nil>",
						G_OBJECT_TYPE_NAME(self),
						inSelection ? G_OBJECT_TYPE_NAME(inSelection) : "<nil>",
						inDirection,
						inAllowWrap ? "allowed" : "denied",
						needsWrap ? "needed" : "not needed");

	return(selection);
}

/* Find requested selection target depending of current selection in list mode */
static ClutterActor* _xfdashboard_search_result_container_find_selection_from_list_mode(XfdashboardSearchResultContainer *self,
																						ClutterActor *inSelection,
																						XfdashboardSelectionTarget inDirection,
																						XfdashboardView *inView,
																						gboolean inAllowWrap)
{
	XfdashboardSearchResultContainerPrivate		*priv;
	ClutterActor								*selection;
	ClutterActor								*newSelection;
	gboolean									needsWrap;

	g_return_val_if_fail(XFDASHBOARD_IS_SEARCH_RESULT_CONTAINER(self), NULL);
	g_return_val_if_fail(CLUTTER_IS_ACTOR(inSelection), NULL);

	priv=self->priv;
	selection=inSelection;
	newSelection=NULL;
	needsWrap=FALSE;

	/* Find target selection */
	switch(inDirection)
	{
		case XFDASHBOARD_SELECTION_TARGET_LEFT:
		case XFDASHBOARD_SELECTION_TARGET_RIGHT:
		case XFDASHBOARD_SELECTION_TARGET_PAGE_LEFT:
		case XFDASHBOARD_SELECTION_TARGET_PAGE_RIGHT:
			/* Do nothing here in list mode */
			break;

		case XFDASHBOARD_SELECTION_TARGET_UP:
			newSelection=clutter_actor_get_previous_sibling(inSelection);
			if(!newSelection)
			{
				newSelection=clutter_actor_get_last_child(priv->itemsContainer);
				needsWrap=TRUE;
			}
			break;

		case XFDASHBOARD_SELECTION_TARGET_DOWN:
			newSelection=clutter_actor_get_next_sibling(inSelection);
			if(!newSelection)
			{
				newSelection=clutter_actor_get_first_child(priv->itemsContainer);
				needsWrap=TRUE;
			}
			break;

		case XFDASHBOARD_SELECTION_TARGET_PAGE_UP:
		case XFDASHBOARD_SELECTION_TARGET_PAGE_DOWN:
			{
				ClutterActor				*child;
				gfloat						topY;
				gfloat						bottomY;
				gfloat						pageSize;
				gfloat						currentY;
				gfloat						limitY;
				gfloat						childY1, childY2;
				ClutterActorIter			iter;

				/* Beginning from current selection go up and first child which needs scrolling */
				child=clutter_actor_get_previous_sibling(inSelection);
				while(child && !xfdashboard_view_child_needs_scroll(inView, child))
				{
					child=clutter_actor_get_previous_sibling(child);
				}
				if(!child) child=clutter_actor_get_first_child(priv->itemsContainer);
				topY=clutter_actor_get_y(child);

				/* Beginning from current selection go down and first child which needs scrolling */
				child=clutter_actor_get_next_sibling(inSelection);
				while(child && !xfdashboard_view_child_needs_scroll(inView, child))
				{
					child=clutter_actor_get_next_sibling(child);
				}
				if(!child) child=clutter_actor_get_last_child(priv->itemsContainer);
				bottomY=clutter_actor_get_y(child);

				/* Get distance between top and bottom actor we found because that's the page size */
				pageSize=bottomY-topY;

				/* Find child in distance of page size from current selection */
				currentY=clutter_actor_get_y(inSelection);

				if(inDirection==XFDASHBOARD_SELECTION_TARGET_PAGE_UP) limitY=currentY-pageSize;
					else limitY=currentY+pageSize;

				clutter_actor_iter_init(&iter, priv->itemsContainer);
				while(!newSelection && clutter_actor_iter_next(&iter, &child))
				{
					childY1=clutter_actor_get_y(child);
					childY2=childY1+clutter_actor_get_height(child);
					if(childY1>limitY || childY2>limitY) newSelection=child;
				}

				/* If new selection is the same is current selection
				 * then we did not find a new selection.
				 */
				if(newSelection==inSelection) newSelection=NULL;

				/* If no child could be found select last one */
				if(!newSelection)
				{
					needsWrap=TRUE;
					if(inDirection==XFDASHBOARD_SELECTION_TARGET_PAGE_UP)
					{
						newSelection=clutter_actor_get_first_child(priv->itemsContainer);
					}
						else
						{
							newSelection=clutter_actor_get_last_child(priv->itemsContainer);
						}
				}
			}
			break;

		default:
			{
				gchar						*valueName;

				valueName=xfdashboard_get_enum_value_name(XFDASHBOARD_TYPE_SELECTION_TARGET, inDirection);
				g_critical(_("Focusable object %s does not handle selection direction of type %s in list mode."),
							G_OBJECT_TYPE_NAME(self),
							valueName);
				g_free(valueName);
			}
			break;
	}

	/* If new selection could be found override current selection with it.
	 * But also check if new selection needs to wrap (crossing boundaries
	 * like going to the beginning because it's gone beyond end) and if
	 * wrapping is allowed.
	 */
	if(newSelection) selection=newSelection;

	if(selection && needsWrap && !inAllowWrap) selection=NULL;

	/* Return new selection */
	XFDASHBOARD_DEBUG(self, ACTOR,
						"Selecting %s in list mode at %s for current selection %s in direction %u with wrapping %s and wrapping %s",
						selection ? G_OBJECT_TYPE_NAME(selection) : "<nil>",
						G_OBJECT_TYPE_NAME(self),
						inSelection ? G_OBJECT_TYPE_NAME(inSelection) : "<nil>",
						inDirection,
						inAllowWrap ? "allowed" : "denied",
						needsWrap ? "needed" : "not needed");

	return(selection);
}

/* IMPLEMENTATION: GObject */

/* Dispose this object */
static void _xfdashboard_search_result_container_dispose(GObject *inObject)
{
	XfdashboardSearchResultContainer			*self=XFDASHBOARD_SEARCH_RESULT_CONTAINER(inObject);
	XfdashboardSearchResultContainerPrivate		*priv=self->priv;

	/* Release allocated variables */
	_xfdashboard_search_result_container_update_selection(self, NULL);

	if(priv->selectedItem)
	{
		g_object_remove_weak_pointer(G_OBJECT(priv->selectedItem), &priv->selectedItem);
		priv->selectedItem=NULL;
	}

	if(priv->provider)
	{
		g_object_unref(priv->provider);
		priv->provider=NULL;
	}

	if(priv->icon)
	{
		g_free(priv->icon);
		priv->icon=NULL;
	}

	if(priv->titleFormat)
	{
		g_free(priv->titleFormat);
		priv->titleFormat=NULL;
	}

	if(priv->mapping)
	{
		GHashTableIter							hashIter;
		GVariant								*key;
		ClutterActor							*value;

		g_hash_table_iter_init(&hashIter, priv->mapping);
		while(g_hash_table_iter_next(&hashIter, (gpointer*)&key, (gpointer*)&value))
		{
			/* First disconnect signal handlers from actor before modifying mapping hash table */
			g_signal_handlers_disconnect_by_data(value, self);

			/* Take a reference on value (it is the destroyed actor) because removing
			 * key from mapping causes unrefencing key and value.
			 */
			g_object_ref(value);
			g_hash_table_iter_remove(&hashIter);
		}

		g_hash_table_unref(priv->mapping);
		priv->mapping=NULL;
	}

	if(priv->lastResultSet)
	{
		g_object_unref(priv->lastResultSet);
		priv->lastResultSet=NULL;
	}

	/* Call parent's class dispose method */
	G_OBJECT_CLASS(xfdashboard_search_result_container_parent_class)->dispose(inObject);
}

/* Set/get properties */
static void _xfdashboard_search_result_container_set_property(GObject *inObject,
													guint inPropID,
													const GValue *inValue,
													GParamSpec *inSpec)
{
	XfdashboardSearchResultContainer			*self=XFDASHBOARD_SEARCH_RESULT_CONTAINER(inObject);

	switch(inPropID)
	{
		case PROP_PROVIDER:
			_xfdashboard_search_result_container_set_provider(self, XFDASHBOARD_SEARCH_PROVIDER(g_value_get_object(inValue)));
			break;

		case PROP_ICON:
			xfdashboard_search_result_container_set_icon(self, g_value_get_string(inValue));
			break;

		case PROP_TITLE_FORMAT:
			xfdashboard_search_result_container_set_title_format(self, g_value_get_string(inValue));
			break;

		case PROP_VIEW_MODE:
			xfdashboard_search_result_container_set_view_mode(self, g_value_get_enum(inValue));
			break;

		case PROP_SPACING:
			xfdashboard_search_result_container_set_spacing(self, g_value_get_float(inValue));
			break;

		case PROP_PADDING:
			xfdashboard_search_result_container_set_padding(self, g_value_get_float(inValue));
			break;

		case PROP_INITIAL_RESULTS_SIZE:
			xfdashboard_search_result_container_set_initial_result_size(self, g_value_get_int(inValue));
			break;

		case PROP_MORE_RESULTS_SIZE:
			xfdashboard_search_result_container_set_more_result_size(self, g_value_get_int(inValue));
			break;

		default:
			G_OBJECT_WARN_INVALID_PROPERTY_ID(inObject, inPropID, inSpec);
			break;
	}
}

static void _xfdashboard_search_result_container_get_property(GObject *inObject,
													guint inPropID,
													GValue *outValue,
													GParamSpec *inSpec)
{
	XfdashboardSearchResultContainer			*self=XFDASHBOARD_SEARCH_RESULT_CONTAINER(inObject);
	XfdashboardSearchResultContainerPrivate		*priv=self->priv;

	switch(inPropID)
	{
		case PROP_ICON:
			g_value_set_string(outValue, priv->icon);
			break;

		case PROP_TITLE_FORMAT:
			g_value_set_string(outValue, priv->titleFormat);
			break;

		case PROP_VIEW_MODE:
			g_value_set_enum(outValue, priv->viewMode);
			break;

		case PROP_SPACING:
			g_value_set_float(outValue, priv->spacing);
			break;

		case PROP_PADDING:
			g_value_set_float(outValue, priv->padding);
			break;

		case PROP_INITIAL_RESULTS_SIZE:
			g_value_set_int(outValue, priv->initialResultsCount);
			break;

		case PROP_MORE_RESULTS_SIZE:
			g_value_set_int(outValue, priv->moreResultsCount);
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
static void xfdashboard_search_result_container_class_init(XfdashboardSearchResultContainerClass *klass)
{
	XfdashboardActorClass	*actorClass=XFDASHBOARD_ACTOR_CLASS(klass);
	GObjectClass			*gobjectClass=G_OBJECT_CLASS(klass);

	/* Override functions */
	gobjectClass->dispose=_xfdashboard_search_result_container_dispose;
	gobjectClass->set_property=_xfdashboard_search_result_container_set_property;
	gobjectClass->get_property=_xfdashboard_search_result_container_get_property;

	/* Define properties */
	XfdashboardSearchResultContainerProperties[PROP_PROVIDER]=
		g_param_spec_object("provider",
							_("Provider"),
							_("The search provider this result container is for"),
							XFDASHBOARD_TYPE_SEARCH_PROVIDER,
							G_PARAM_WRITABLE | G_PARAM_STATIC_STRINGS | G_PARAM_CONSTRUCT_ONLY);

	XfdashboardSearchResultContainerProperties[PROP_ICON]=
		g_param_spec_string("icon",
							_("Icon"),
							_("A themed icon name or file name of icon this container will display. If not set the icon the search provider defines will be used."),
							NULL,
							G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS);

	XfdashboardSearchResultContainerProperties[PROP_TITLE_FORMAT]=
		g_param_spec_string("title-format",
							_("Title format"),
							_("Format string for title which will contain the name of search provider"),
							NULL,
							G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS);

	XfdashboardSearchResultContainerProperties[PROP_VIEW_MODE]=
		g_param_spec_enum("view-mode",
							_("View mode"),
							_("View mode of container for result items"),
							XFDASHBOARD_TYPE_VIEW_MODE,
							DEFAULT_VIEW_MODE,
							G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS);

	XfdashboardSearchResultContainerProperties[PROP_SPACING]=
		g_param_spec_float("spacing",
							_("Spacing"),
							_("Spacing between each result item"),
							0.0f, G_MAXFLOAT,
							0.0f,
							G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS);

	XfdashboardSearchResultContainerProperties[PROP_PADDING]=
		g_param_spec_float("padding",
							_("Padding"),
							_("Padding between title and item results container"),
							0.0f, G_MAXFLOAT,
							0.0f,
							G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS);

	XfdashboardSearchResultContainerProperties[PROP_INITIAL_RESULTS_SIZE]=
		g_param_spec_int("initial-results-size",
							_("Initial results size"),
							_("The maximum number of results shown initially. 0 means all results"),
							0, G_MAXINT,
							DEFAULT_INITIAL_RESULT_SIZE,
							G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS);

	XfdashboardSearchResultContainerProperties[PROP_MORE_RESULTS_SIZE]=
		g_param_spec_int("more-results-size",
							_("More results size"),
							_("The number of results to increase current limit by"),
							0, G_MAXINT,
							DEFAULT_MORE_RESULT_SIZE,
							G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS);

	g_object_class_install_properties(gobjectClass, PROP_LAST, XfdashboardSearchResultContainerProperties);

	/* Define stylable properties */
	xfdashboard_actor_install_stylable_property(actorClass, XfdashboardSearchResultContainerProperties[PROP_ICON]);
	xfdashboard_actor_install_stylable_property(actorClass, XfdashboardSearchResultContainerProperties[PROP_TITLE_FORMAT]);
	xfdashboard_actor_install_stylable_property(actorClass, XfdashboardSearchResultContainerProperties[PROP_VIEW_MODE]);
	xfdashboard_actor_install_stylable_property(actorClass, XfdashboardSearchResultContainerProperties[PROP_SPACING]);
	xfdashboard_actor_install_stylable_property(actorClass, XfdashboardSearchResultContainerProperties[PROP_PADDING]);
	xfdashboard_actor_install_stylable_property(actorClass, XfdashboardSearchResultContainerProperties[PROP_INITIAL_RESULTS_SIZE]);
	xfdashboard_actor_install_stylable_property(actorClass, XfdashboardSearchResultContainerProperties[PROP_MORE_RESULTS_SIZE]);

	/* Define signals */
	XfdashboardSearchResultContainerSignals[SIGNAL_ICON_CLICKED]=
		g_signal_new("icon-clicked",
						G_TYPE_FROM_CLASS(klass),
						G_SIGNAL_RUN_LAST,
						G_STRUCT_OFFSET(XfdashboardSearchResultContainerClass, icon_clicked),
						NULL,
						NULL,
						g_cclosure_marshal_VOID__VOID,
						G_TYPE_NONE,
						0);

	XfdashboardSearchResultContainerSignals[SIGNAL_ITEM_CLICKED]=
		g_signal_new("item-clicked",
						G_TYPE_FROM_CLASS(klass),
						G_SIGNAL_RUN_LAST,
						G_STRUCT_OFFSET(XfdashboardSearchResultContainerClass, item_clicked),
						NULL,
						NULL,
						_xfdashboard_marshal_VOID__VARIANT_OBJECT,
						G_TYPE_NONE,
						2,
						G_TYPE_VARIANT,
						CLUTTER_TYPE_ACTOR);
}

/* Object initialization
 * Create private structure and set up default values
 */
static void xfdashboard_search_result_container_init(XfdashboardSearchResultContainer *self)
{
	XfdashboardSearchResultContainerPrivate		*priv;
	ClutterLayoutManager						*layout;
	ClutterActor								*buttonContainer;

	priv=self->priv=xfdashboard_search_result_container_get_instance_private(self);

	/* Set up default values */
	priv->icon=NULL;
	priv->titleFormat=NULL;
	priv->viewMode=-1;
	priv->spacing=0.0f;
	priv->padding=0.0f;
	priv->selectedItem=NULL;
	priv->selectedItemDestroySignalID=0;
	priv->mapping=g_hash_table_new_full(g_variant_hash,
										g_variant_equal,
										(GDestroyNotify)g_variant_unref,
										(GDestroyNotify)g_object_unref);
	priv->lastResultSet=NULL;
	priv->initialResultsCount=DEFAULT_INITIAL_RESULT_SIZE;
	priv->moreResultsCount=DEFAULT_MORE_RESULT_SIZE;
	priv->maxResultsItemsCountSet=FALSE;
	priv->maxResultsItemsCount=0;

	/* Set up children */
	clutter_actor_set_reactive(CLUTTER_ACTOR(self), FALSE);

	priv->titleTextBox=xfdashboard_text_box_new();
	clutter_actor_set_x_expand(priv->titleTextBox, TRUE);
	xfdashboard_stylable_add_class(XFDASHBOARD_STYLABLE(priv->titleTextBox), "title");

	priv->itemsContainer=xfdashboard_actor_new();
	clutter_actor_set_x_expand(priv->itemsContainer, TRUE);
	xfdashboard_stylable_add_class(XFDASHBOARD_STYLABLE(priv->itemsContainer), "items-container");
	xfdashboard_search_result_container_set_view_mode(self, DEFAULT_VIEW_MODE);

	priv->moreResultsLabelActor=xfdashboard_button_new();
	clutter_actor_set_x_expand(priv->moreResultsLabelActor, TRUE);
	xfdashboard_label_set_style(XFDASHBOARD_LABEL(priv->moreResultsLabelActor), XFDASHBOARD_LABEL_STYLE_TEXT);
	xfdashboard_stylable_add_class(XFDASHBOARD_STYLABLE(priv->moreResultsLabelActor), "more-results");

	priv->allResultsLabelActor=xfdashboard_button_new();
	clutter_actor_set_x_expand(priv->allResultsLabelActor, TRUE);
	clutter_actor_set_x_align(priv->allResultsLabelActor, CLUTTER_ACTOR_ALIGN_END);
	xfdashboard_label_set_style(XFDASHBOARD_LABEL(priv->allResultsLabelActor), XFDASHBOARD_LABEL_STYLE_TEXT);
	xfdashboard_stylable_add_class(XFDASHBOARD_STYLABLE(priv->allResultsLabelActor), "all-results");

	layout=clutter_box_layout_new();
	clutter_box_layout_set_orientation(CLUTTER_BOX_LAYOUT(layout), CLUTTER_ORIENTATION_HORIZONTAL);
	clutter_box_layout_set_homogeneous(CLUTTER_BOX_LAYOUT(layout), TRUE);

	buttonContainer=clutter_actor_new();
	clutter_actor_set_layout_manager(buttonContainer, layout);
	clutter_actor_set_x_expand(buttonContainer, TRUE);
	clutter_actor_add_child(buttonContainer, priv->moreResultsLabelActor);
	clutter_actor_add_child(buttonContainer, priv->allResultsLabelActor);

	/* Set up actor */
	xfdashboard_actor_set_can_focus(XFDASHBOARD_ACTOR(self), TRUE);

	layout=clutter_box_layout_new();
	clutter_box_layout_set_orientation(CLUTTER_BOX_LAYOUT(layout), CLUTTER_ORIENTATION_VERTICAL);

	clutter_actor_set_layout_manager(CLUTTER_ACTOR(self), layout);
	clutter_actor_set_x_expand(CLUTTER_ACTOR(self), TRUE);
	clutter_actor_add_child(CLUTTER_ACTOR(self), priv->titleTextBox);
	clutter_actor_add_child(CLUTTER_ACTOR(self), priv->itemsContainer);
	clutter_actor_add_child(CLUTTER_ACTOR(self), buttonContainer);

	/* Connect signals */
	g_signal_connect_swapped(priv->titleTextBox,
								"primary-icon-clicked",
								G_CALLBACK(_xfdashboard_search_result_container_on_primary_icon_clicked),
								self);

	g_signal_connect_swapped(priv->moreResultsLabelActor,
								"clicked",
								G_CALLBACK(_xfdashboard_search_result_container_on_more_results_label_clicked),
								self);

	g_signal_connect_swapped(priv->allResultsLabelActor,
								"clicked",
								G_CALLBACK(_xfdashboard_search_result_container_on_all_results_label_clicked),
								self);
}

/* IMPLEMENTATION: Public API */

/* Create new actor */
ClutterActor* xfdashboard_search_result_container_new(XfdashboardSearchProvider *inProvider)
{
	return(g_object_new(XFDASHBOARD_TYPE_SEARCH_RESULT_CONTAINER,
							"provider", inProvider,
							NULL));
}

/* Get/set icon */
const gchar* xfdashboard_search_result_container_get_icon(XfdashboardSearchResultContainer *self)
{
	g_return_val_if_fail(XFDASHBOARD_IS_SEARCH_RESULT_CONTAINER(self), NULL);

	return(self->priv->icon);
}

void xfdashboard_search_result_container_set_icon(XfdashboardSearchResultContainer *self, const gchar *inIcon)
{
	XfdashboardSearchResultContainerPrivate		*priv;

	g_return_if_fail(XFDASHBOARD_IS_SEARCH_RESULT_CONTAINER(self));

	priv=self->priv;

	/* Set value if changed */
	if(g_strcmp0(priv->icon, inIcon)!=0)
	{
		/* Set value */
		if(priv->icon)
		{
			g_free(priv->icon);
			priv->icon=NULL;
		}

		if(inIcon) priv->icon=g_strdup(inIcon);

		/* Update icon */
		_xfdashboard_search_result_container_update_icon(self);

		/* Notify about property change */
		g_object_notify_by_pspec(G_OBJECT(self), XfdashboardSearchResultContainerProperties[PROP_ICON]);
	}
}

/* Get/set format of header */
const gchar* xfdashboard_search_result_container_get_title_format(XfdashboardSearchResultContainer *self)
{
	g_return_val_if_fail(XFDASHBOARD_IS_SEARCH_RESULT_CONTAINER(self), NULL);

	return(self->priv->titleFormat);
}

void xfdashboard_search_result_container_set_title_format(XfdashboardSearchResultContainer *self, const gchar *inFormat)
{
	XfdashboardSearchResultContainerPrivate		*priv;

	g_return_if_fail(XFDASHBOARD_IS_SEARCH_RESULT_CONTAINER(self));

	priv=self->priv;

	/* Set value if changed */
	if(g_strcmp0(priv->titleFormat, inFormat)!=0)
	{
		/* Set value */
		if(priv->titleFormat)
		{
			g_free(priv->titleFormat);
			priv->titleFormat=NULL;
		}

		if(inFormat) priv->titleFormat=g_strdup(inFormat);

		/* Update title */
		_xfdashboard_search_result_container_update_title(self);

		/* Notify about property change */
		g_object_notify_by_pspec(G_OBJECT(self), XfdashboardSearchResultContainerProperties[PROP_TITLE_FORMAT]);
	}
}

/* Get/set view mode for items */
XfdashboardViewMode xfdashboard_search_result_container_get_view_mode(XfdashboardSearchResultContainer *self)
{
	g_return_val_if_fail(XFDASHBOARD_IS_SEARCH_RESULT_CONTAINER(self), DEFAULT_VIEW_MODE);

	return(self->priv->viewMode);
}

void xfdashboard_search_result_container_set_view_mode(XfdashboardSearchResultContainer *self, const XfdashboardViewMode inMode)
{
	XfdashboardSearchResultContainerPrivate		*priv;
	ClutterActorIter							iter;
	ClutterActor								*child;
	const gchar									*removeClass;
	const gchar									*addClass;

	g_return_if_fail(XFDASHBOARD_IS_SEARCH_RESULT_CONTAINER(self));
	g_return_if_fail(inMode==XFDASHBOARD_VIEW_MODE_LIST || inMode==XFDASHBOARD_VIEW_MODE_ICON);

	priv=self->priv;

	/* Set value if changed */
	if(priv->viewMode!=inMode)
	{
		/* Set value */
		priv->viewMode=inMode;

		/* Set new layout manager */
		switch(priv->viewMode)
		{
			case XFDASHBOARD_VIEW_MODE_LIST:
				priv->layout=clutter_box_layout_new();
				clutter_box_layout_set_orientation(CLUTTER_BOX_LAYOUT(priv->layout), CLUTTER_ORIENTATION_VERTICAL);
				clutter_box_layout_set_spacing(CLUTTER_BOX_LAYOUT(priv->layout), priv->spacing);
				clutter_actor_set_layout_manager(priv->itemsContainer, priv->layout);

				removeClass="view-mode-icon";
				addClass="view-mode-list";
				break;

			case XFDASHBOARD_VIEW_MODE_ICON:
				priv->layout=xfdashboard_dynamic_table_layout_new();
				xfdashboard_dynamic_table_layout_set_spacing(XFDASHBOARD_DYNAMIC_TABLE_LAYOUT(priv->layout), priv->spacing);
				clutter_actor_set_layout_manager(priv->itemsContainer, priv->layout);

				removeClass="view-mode-list";
				addClass="view-mode-icon";
				break;

			default:
				g_assert_not_reached();
		}

		/* Iterate through actors in items container and update style class for new view-mode */
		clutter_actor_iter_init(&iter, priv->itemsContainer);
		while(clutter_actor_iter_next(&iter, &child))
		{
			if(!XFDASHBOARD_IS_STYLABLE(child)) continue;

			xfdashboard_stylable_remove_class(XFDASHBOARD_STYLABLE(child), removeClass);
			xfdashboard_stylable_add_class(XFDASHBOARD_STYLABLE(child), addClass);
		}

		/* Notify about property change */
		g_object_notify_by_pspec(G_OBJECT(self), XfdashboardSearchResultContainerProperties[PROP_VIEW_MODE]);
	}
}

/* Get/set spacing between result item actors */
gfloat xfdashboard_search_result_container_get_spacing(XfdashboardSearchResultContainer *self)
{
	g_return_val_if_fail(XFDASHBOARD_IS_SEARCH_RESULT_CONTAINER(self), 0.0f);

	return(self->priv->spacing);
}

void xfdashboard_search_result_container_set_spacing(XfdashboardSearchResultContainer *self, const gfloat inSpacing)
{
	XfdashboardSearchResultContainerPrivate		*priv;

	g_return_if_fail(XFDASHBOARD_IS_SEARCH_RESULT_CONTAINER(self));
	g_return_if_fail(inSpacing>=0.0f);

	priv=self->priv;

	/* Set value if changed */
	if(priv->spacing!=inSpacing)
	{
		/* Set value */
		priv->spacing=inSpacing;

		/* Update layout manager */
		switch(priv->viewMode)
		{
			case XFDASHBOARD_VIEW_MODE_LIST:
				clutter_box_layout_set_spacing(CLUTTER_BOX_LAYOUT(priv->layout), priv->spacing);
				break;

			case XFDASHBOARD_VIEW_MODE_ICON:
				xfdashboard_dynamic_table_layout_set_spacing(XFDASHBOARD_DYNAMIC_TABLE_LAYOUT(priv->layout), priv->spacing);
				break;

			default:
				g_assert_not_reached();
		}

		/* Notify about property change */
		g_object_notify_by_pspec(G_OBJECT(self), XfdashboardSearchResultContainerProperties[PROP_SPACING]);
	}
}

/* Get/set spacing between result item actors */
gfloat xfdashboard_search_result_container_get_padding(XfdashboardSearchResultContainer *self)
{
	g_return_val_if_fail(XFDASHBOARD_IS_SEARCH_RESULT_CONTAINER(self), 0.0f);

	return(self->priv->padding);
}

void xfdashboard_search_result_container_set_padding(XfdashboardSearchResultContainer *self, const gfloat inPadding)
{
	XfdashboardSearchResultContainerPrivate		*priv;
	ClutterMargin								margin;

	g_return_if_fail(XFDASHBOARD_IS_SEARCH_RESULT_CONTAINER(self));
	g_return_if_fail(inPadding>=0.0f);

	priv=self->priv;

	/* Set value if changed */
	if(priv->padding!=inPadding)
	{
		/* Set value */
		priv->padding=inPadding;

		/* Update actors */
		margin.left=priv->padding;
		margin.right=priv->padding;
		margin.top=priv->padding;
		margin.bottom=priv->padding;

		clutter_actor_set_margin(priv->titleTextBox, &margin);
		clutter_actor_set_margin(priv->itemsContainer, &margin);

		/* Notify about property change */
		g_object_notify_by_pspec(G_OBJECT(self), XfdashboardSearchResultContainerProperties[PROP_PADDING]);
	}
}

/* Get/set number of result items to show initially */
gint xfdashboard_search_result_container_get_initial_result_size(XfdashboardSearchResultContainer *self)
{
	g_return_val_if_fail(XFDASHBOARD_IS_SEARCH_RESULT_CONTAINER(self), 0);

	return(self->priv->initialResultsCount);
}

void xfdashboard_search_result_container_set_initial_result_size(XfdashboardSearchResultContainer *self, const gint inSize)
{
	XfdashboardSearchResultContainerPrivate		*priv;

	g_return_if_fail(XFDASHBOARD_IS_SEARCH_RESULT_CONTAINER(self));
	g_return_if_fail(inSize>=0);

	priv=self->priv;

	/* Set value if changed */
	if(priv->initialResultsCount!=inSize)
	{
		/* Set value */
		priv->initialResultsCount=inSize;

		/* Notify about property change */
		g_object_notify_by_pspec(G_OBJECT(self), XfdashboardSearchResultContainerProperties[PROP_INITIAL_RESULTS_SIZE]);
	}
}

/* Get/set number to increase number of actors of result items to show */
gint xfdashboard_search_result_container_get_more_result_size(XfdashboardSearchResultContainer *self)
{
	g_return_val_if_fail(XFDASHBOARD_IS_SEARCH_RESULT_CONTAINER(self), 0);

	return(self->priv->moreResultsCount);
}

void xfdashboard_search_result_container_set_more_result_size(XfdashboardSearchResultContainer *self, const gint inSize)
{
	XfdashboardSearchResultContainerPrivate		*priv;
	gint										allResultsCount;
	gint										currentResultsCount;
	gint										moreCount;
	gchar										*labelText;

	g_return_if_fail(XFDASHBOARD_IS_SEARCH_RESULT_CONTAINER(self));
	g_return_if_fail(inSize>=0);

	priv=self->priv;

	/* Set value if changed */
	if(priv->moreResultsCount!=inSize)
	{
		/* Set value */
		priv->moreResultsCount=inSize;

		/* Update text of "more"-label */
		allResultsCount=0;
		if(priv->lastResultSet) allResultsCount=(gint)xfdashboard_search_result_set_get_size(priv->lastResultSet);

		currentResultsCount=clutter_actor_get_n_children(priv->itemsContainer);

		moreCount=MIN(allResultsCount-currentResultsCount, priv->moreResultsCount);

		labelText=g_strdup_printf(_("Show %d more results..."), moreCount);
		xfdashboard_label_set_text(XFDASHBOARD_LABEL(priv->moreResultsLabelActor), labelText);
		if(labelText) g_free(labelText);

		/* Notify about property change */
		g_object_notify_by_pspec(G_OBJECT(self), XfdashboardSearchResultContainerProperties[PROP_MORE_RESULTS_SIZE]);
	}
}

/* Set to or unset focus from container */
void xfdashboard_search_result_container_set_focus(XfdashboardSearchResultContainer *self, gboolean inSetFocus)
{
	g_return_if_fail(XFDASHBOARD_IS_SEARCH_RESULT_CONTAINER(self));

	/* Unset selection */
	_xfdashboard_search_result_container_update_selection(self, NULL);
}

/* Get current selection */
ClutterActor* xfdashboard_search_result_container_get_selection(XfdashboardSearchResultContainer *self)
{
	g_return_val_if_fail(XFDASHBOARD_IS_SEARCH_RESULT_CONTAINER(self), NULL);

	return(self->priv->selectedItem);
}

/* Set current selection */
gboolean xfdashboard_search_result_container_set_selection(XfdashboardSearchResultContainer *self,
																	ClutterActor *inSelection)
{
	g_return_val_if_fail(XFDASHBOARD_IS_SEARCH_RESULT_CONTAINER(self), FALSE);
	g_return_val_if_fail(!inSelection || CLUTTER_IS_ACTOR(inSelection), FALSE);

	/* Check that selection is a child of this actor */
	if(inSelection &&
		!clutter_actor_contains(CLUTTER_ACTOR(self), inSelection))
	{
		g_warning(_("%s is not a child of %s and cannot be selected"),
					G_OBJECT_TYPE_NAME(inSelection),
					G_OBJECT_TYPE_NAME(self));

		return(FALSE);
	}

	/* Set selection */
	_xfdashboard_search_result_container_update_selection(self, inSelection);

	/* We could successfully set selection so return success result */
	return(TRUE);
}

/* Find requested selection target depending of current selection */
ClutterActor* xfdashboard_search_result_container_find_selection(XfdashboardSearchResultContainer *self,
																	ClutterActor *inSelection,
																	XfdashboardSelectionTarget inDirection,
																	XfdashboardView *inView,
																	gboolean inAllowWrap)
{
	XfdashboardSearchResultContainerPrivate		*priv;
	ClutterActor								*selection;
	ClutterActor								*newSelection;

	g_return_val_if_fail(XFDASHBOARD_IS_SEARCH_RESULT_CONTAINER(self), NULL);
	g_return_val_if_fail(!inSelection || CLUTTER_IS_ACTOR(inSelection), NULL);
	g_return_val_if_fail(inDirection>=0 && inDirection<=XFDASHBOARD_SELECTION_TARGET_NEXT, NULL);

	priv=self->priv;
	selection=NULL;
	newSelection=NULL;

	/* If first selection is requested, select first actor and return */
	if(inDirection==XFDASHBOARD_SELECTION_TARGET_FIRST)
	{
		newSelection=clutter_actor_get_first_child(priv->itemsContainer);
		return(newSelection);
	}

	/* If last selection is requested, select last actor and return */
	if(inDirection==XFDASHBOARD_SELECTION_TARGET_LAST)
	{
		newSelection=clutter_actor_get_last_child(priv->itemsContainer);
		return(newSelection);
	}

	/* If there is nothing selected, select the first actor and return */
	if(!inSelection)
	{
		newSelection=clutter_actor_get_first_child(priv->itemsContainer);
		XFDASHBOARD_DEBUG(self, ACTOR,
							"No selection for %s, so select first child of result container for provider %s",
							G_OBJECT_TYPE_NAME(self),
							priv->provider ? G_OBJECT_TYPE_NAME(priv->provider) : "<unknown provider>");

		return(newSelection);
	}

	/* Check that selection is a child of this actor otherwise return NULL */
	if(!clutter_actor_contains(CLUTTER_ACTOR(self), inSelection))
	{
		ClutterActor						*parent;

		parent=clutter_actor_get_parent(inSelection);
		g_warning(_("Cannot lookup selection target at %s because %s is a child of %s but not of this container"),
					G_OBJECT_TYPE_NAME(self),
					G_OBJECT_TYPE_NAME(inSelection),
					parent ? G_OBJECT_TYPE_NAME(parent) : "<nil>");

		return(NULL);
	}

	/* Find target selection */
	switch(inDirection)
	{
		case XFDASHBOARD_SELECTION_TARGET_LEFT:
		case XFDASHBOARD_SELECTION_TARGET_RIGHT:
		case XFDASHBOARD_SELECTION_TARGET_UP:
		case XFDASHBOARD_SELECTION_TARGET_DOWN:
		case XFDASHBOARD_SELECTION_TARGET_PAGE_LEFT:
		case XFDASHBOARD_SELECTION_TARGET_PAGE_RIGHT:
		case XFDASHBOARD_SELECTION_TARGET_PAGE_UP:
		case XFDASHBOARD_SELECTION_TARGET_PAGE_DOWN:
			if(priv->viewMode==XFDASHBOARD_VIEW_MODE_LIST)
			{
				newSelection=_xfdashboard_search_result_container_find_selection_from_list_mode(self, inSelection, inDirection, inView, inAllowWrap);
			}
				else
				{
					newSelection=_xfdashboard_search_result_container_find_selection_from_icon_mode(self, inSelection, inDirection, inView, inAllowWrap);
				}
			break;

		case XFDASHBOARD_SELECTION_TARGET_NEXT:
			newSelection=clutter_actor_get_next_sibling(inSelection);
			if(!newSelection && inAllowWrap) newSelection=clutter_actor_get_previous_sibling(inSelection);
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
				g_critical(_("Focusable object %s does not handle selection direction of type %s."),
							G_OBJECT_TYPE_NAME(self),
							valueName);
				g_free(valueName);
			}
			break;
	}

	/* If new selection could be found override current selection with it */
	if(newSelection) selection=newSelection;

	/* Return new selection found */
	XFDASHBOARD_DEBUG(self, ACTOR,
						"Selecting %s at %s for current selection %s in direction %u with wrapping %s",
						selection ? G_OBJECT_TYPE_NAME(selection) : "<nil>",
						G_OBJECT_TYPE_NAME(self),
						inSelection ? G_OBJECT_TYPE_NAME(inSelection) : "<nil>",
						inDirection,
						inAllowWrap ? "allowed" : "denied");

	return(selection);
}

/* An result item should be activated */
void xfdashboard_search_result_container_activate_selection(XfdashboardSearchResultContainer *self,
																	ClutterActor *inSelection)
{
	g_return_if_fail(XFDASHBOARD_IS_SEARCH_RESULT_CONTAINER(self));
	g_return_if_fail(CLUTTER_IS_ACTOR(inSelection));

	/* Check that selection is a child of this actor */
	if(!clutter_actor_contains(CLUTTER_ACTOR(self), inSelection))
	{
		g_warning(_("%s is not a child of %s and cannot be activated"),
					G_OBJECT_TYPE_NAME(inSelection),
					G_OBJECT_TYPE_NAME(self));

		return;
	}

	/* Activate selection */
	_xfdashboard_search_result_container_activate_result_item_by_actor(self, inSelection);
}

/* Update result items in container with given result set */
void xfdashboard_search_result_container_update(XfdashboardSearchResultContainer *self, XfdashboardSearchResultSet *inResultSet)
{
	g_return_if_fail(XFDASHBOARD_IS_SEARCH_RESULT_CONTAINER(self));
	g_return_if_fail(XFDASHBOARD_IS_SEARCH_RESULT_SET(inResultSet));

	_xfdashboard_search_result_container_update_result_items(self, inResultSet, FALSE);
}
