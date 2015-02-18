/*
 * search-result-container: An container for results from a search provider
 *                          which has a header and container for items
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

#include "search-result-container.h"

#include <glib/gi18n-lib.h>
#include <gdk/gdk.h>
#include <math.h>

#include "enums.h"
#include "text-box.h"
#include "stylable.h"
#include "dynamic-table-layout.h"
#include "utils.h"

/* Define this class in GObject system */
G_DEFINE_TYPE(XfdashboardSearchResultContainer,
				xfdashboard_search_result_container,
				XFDASHBOARD_TYPE_ACTOR)

/* Private structure - access only by public API if needed */
#define XFDASHBOARD_SEARCH_RESULT_CONTAINER_GET_PRIVATE(obj) \
	(G_TYPE_INSTANCE_GET_PRIVATE((obj), XFDASHBOARD_TYPE_SEARCH_RESULT_CONTAINER, XfdashboardSearchResultContainerPrivate))

struct _XfdashboardSearchResultContainerPrivate
{
	/* Properties related */
	XfdashboardSearchProvider	*provider;

	gchar						*icon;
	gchar						*titleFormat;
	XfdashboardViewMode			viewMode;
	gfloat						spacing;
	gfloat						padding;

	/* Instance related */
	ClutterLayoutManager		*layout;
	ClutterActor				*titleTextBox;
	ClutterActor				*itemsContainer;

	ClutterActor				*selectedItem;
	guint						selectedItemDestroySignalID;
};

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

	PROP_LAST
};

static GParamSpec* XfdashboardSearchResultContainerProperties[PROP_LAST]={ 0, };

/* Signals */
enum
{
	SIGNAL_ICON_CLICKED,

	SIGNAL_LAST
};

static guint XfdashboardSearchResultContainerSignals[SIGNAL_LAST]={ 0, };

/* IMPLEMENTATION: Private variables and methods */
#define DEFAULT_VIEW_MODE		XFDASHBOARD_VIEW_MODE_LIST

/* Forward declarations */
static void _xfdashboard_search_result_container_update_selection(XfdashboardSearchResultContainer *self,
																	ClutterActor *inNewSelectedItem);

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

		/* Connect signals */
		g_signal_connect_swapped(inNewSelectedItem,
									"destroy",
									G_CALLBACK(_xfdashboard_search_result_container_on_destroy_selection),
									self);

		/* Style new selection */
		if(XFDASHBOARD_IS_STYLABLE(priv->selectedItem))
		{
			xfdashboard_stylable_add_pseudo_class(XFDASHBOARD_STYLABLE(priv->selectedItem), "selected");
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

	/* Update icon */
	_xfdashboard_search_result_container_update_icon(self);

	/* Update title */
	_xfdashboard_search_result_container_update_title(self);
}

/* Find requested selection target depending of current selection in icon mode */
static ClutterActor* _xfdashboard_search_result_container_find_selection_from_icon_mode(XfdashboardSearchResultContainer *self,
																						ClutterActor *inSelection,
																						XfdashboardSelectionTarget inDirection,
																						XfdashboardView *inView)
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

	g_return_val_if_fail(XFDASHBOARD_IS_SEARCH_RESULT_CONTAINER(self), NULL);
	g_return_val_if_fail(CLUTTER_IS_ACTOR(inSelection), NULL);

	priv=self->priv;
	selection=inSelection;
	newSelection=NULL;

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
			}
				else newSelectionIndex=currentSelectionIndex+1;

			newSelectionIndex=MIN(newSelectionIndex, numberChildren-1);
			newSelection=clutter_actor_get_child_at_index(priv->itemsContainer, newSelectionIndex);
			break;

		case XFDASHBOARD_SELECTION_TARGET_UP:
			currentSelectionRow--;
			if(currentSelectionRow<0) currentSelectionRow=rows-1;
			newSelectionIndex=(currentSelectionRow*columns)+currentSelectionColumn;

			newSelectionIndex=MIN(newSelectionIndex, numberChildren-1);
			newSelection=clutter_actor_get_child_at_index(priv->itemsContainer, newSelectionIndex);
			break;

		case XFDASHBOARD_SELECTION_TARGET_DOWN:
			currentSelectionRow++;
			if(currentSelectionRow>=rows) currentSelectionRow=0;
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

	/* If new selection could be found override current selection with it */
	if(newSelection) selection=newSelection;

	/* Return new selection */
	g_debug("Selecting %s at %s for current selection %s in direction %u",
			selection ? G_OBJECT_TYPE_NAME(selection) : "<nil>",
			G_OBJECT_TYPE_NAME(self),
			inSelection ? G_OBJECT_TYPE_NAME(inSelection) : "<nil>",
			inDirection);
	return(selection);
}

/* Find requested selection target depending of current selection in list mode */
static ClutterActor* _xfdashboard_search_result_container_find_selection_from_list_mode(XfdashboardSearchResultContainer *self,
																						ClutterActor *inSelection,
																						XfdashboardSelectionTarget inDirection,
																						XfdashboardView *inView)
{
	XfdashboardSearchResultContainerPrivate		*priv;
	ClutterActor								*selection;
	ClutterActor								*newSelection;

	g_return_val_if_fail(XFDASHBOARD_IS_SEARCH_RESULT_CONTAINER(self), NULL);
	g_return_val_if_fail(CLUTTER_IS_ACTOR(inSelection), NULL);

	priv=self->priv;
	selection=inSelection;
	newSelection=NULL;

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
			if(!newSelection) newSelection=clutter_actor_get_last_child(priv->itemsContainer);
			break;

		case XFDASHBOARD_SELECTION_TARGET_DOWN:
			newSelection=clutter_actor_get_next_sibling(inSelection);
			if(!newSelection) newSelection=clutter_actor_get_first_child(priv->itemsContainer);
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

				/* If no child could be found select last one */
				if(!newSelection)
				{
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

	/* If new selection could be found override current selection with it */
	if(newSelection) selection=newSelection;

	/* Return new selection */
	g_debug("Selecting %s at %s for current selection %s in direction %u",
			selection ? G_OBJECT_TYPE_NAME(selection) : "<nil>",
			G_OBJECT_TYPE_NAME(self),
			inSelection ? G_OBJECT_TYPE_NAME(inSelection) : "<nil>",
			inDirection);
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

	/* Set up private structure */
	g_type_class_add_private(klass, sizeof(XfdashboardSearchResultContainerPrivate));

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

	g_object_class_install_properties(gobjectClass, PROP_LAST, XfdashboardSearchResultContainerProperties);

	/* Define stylable properties */
	xfdashboard_actor_install_stylable_property(actorClass, XfdashboardSearchResultContainerProperties[PROP_ICON]);
	xfdashboard_actor_install_stylable_property(actorClass, XfdashboardSearchResultContainerProperties[PROP_TITLE_FORMAT]);
	xfdashboard_actor_install_stylable_property(actorClass, XfdashboardSearchResultContainerProperties[PROP_VIEW_MODE]);
	xfdashboard_actor_install_stylable_property(actorClass, XfdashboardSearchResultContainerProperties[PROP_SPACING]);
	xfdashboard_actor_install_stylable_property(actorClass, XfdashboardSearchResultContainerProperties[PROP_PADDING]);

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
}

/* Object initialization
 * Create private structure and set up default values
 */
static void xfdashboard_search_result_container_init(XfdashboardSearchResultContainer *self)
{
	XfdashboardSearchResultContainerPrivate		*priv;
	ClutterLayoutManager						*layout;

	priv=self->priv=XFDASHBOARD_SEARCH_RESULT_CONTAINER_GET_PRIVATE(self);

	/* Set up default values */
	priv->icon=NULL;
	priv->titleFormat=NULL;
	priv->viewMode=-1;
	priv->spacing=0.0f;
	priv->padding=0.0f;
	priv->selectedItem=NULL;
	priv->selectedItemDestroySignalID=0;

	/* Set up children */
	clutter_actor_set_reactive(CLUTTER_ACTOR(self), FALSE);

	priv->titleTextBox=xfdashboard_text_box_new();
	clutter_actor_set_x_expand(priv->titleTextBox, TRUE);
	xfdashboard_stylable_add_class(XFDASHBOARD_STYLABLE(priv->titleTextBox), "title");

	priv->itemsContainer=xfdashboard_actor_new();
	clutter_actor_set_x_expand(priv->itemsContainer, TRUE);
	xfdashboard_stylable_add_class(XFDASHBOARD_STYLABLE(priv->itemsContainer), "items-container");
	xfdashboard_search_result_container_set_view_mode(self, DEFAULT_VIEW_MODE);

	/* Set up actor */
	xfdashboard_actor_set_can_focus(XFDASHBOARD_ACTOR(self), TRUE);

	layout=clutter_box_layout_new();
	clutter_box_layout_set_orientation(CLUTTER_BOX_LAYOUT(layout), CLUTTER_ORIENTATION_VERTICAL);

	clutter_actor_set_layout_manager(CLUTTER_ACTOR(self), layout);
	clutter_actor_set_x_expand(CLUTTER_ACTOR(self), TRUE);
	clutter_actor_add_child(CLUTTER_ACTOR(self), priv->titleTextBox);
	clutter_actor_add_child(CLUTTER_ACTOR(self), priv->itemsContainer);

	/* Connect signals */
	g_signal_connect_swapped(priv->titleTextBox,
								"primary-icon-clicked",
								G_CALLBACK(_xfdashboard_search_result_container_on_primary_icon_clicked),
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

/* Add actor for a result item to items container */
void xfdashboard_search_result_container_add_result_actor(XfdashboardSearchResultContainer *self,
															ClutterActor *inResultActor,
															ClutterActor *inInsertAfter)
{
	XfdashboardSearchResultContainerPrivate		*priv;

	g_return_if_fail(XFDASHBOARD_IS_SEARCH_RESULT_CONTAINER(self));
	g_return_if_fail(CLUTTER_IS_ACTOR(inResultActor));
	g_return_if_fail(!inInsertAfter || CLUTTER_IS_ACTOR(inInsertAfter));

	priv=self->priv;

	/* Set style depending on view mode */
	if(XFDASHBOARD_IS_STYLABLE(inResultActor))
	{
		if(priv->viewMode==XFDASHBOARD_VIEW_MODE_LIST) xfdashboard_stylable_add_class(XFDASHBOARD_STYLABLE(inResultActor), "view-mode-list");
			else xfdashboard_stylable_add_class(XFDASHBOARD_STYLABLE(inResultActor), "view-mode-icon");

		xfdashboard_stylable_add_class(XFDASHBOARD_STYLABLE(inResultActor), "result-item");
	}

	/* Add actor to items container */
	clutter_actor_set_x_expand(inResultActor, TRUE);

	if(!inInsertAfter) clutter_actor_insert_child_below(priv->itemsContainer, inResultActor, NULL);
		else clutter_actor_insert_child_above(priv->itemsContainer, inResultActor, inInsertAfter);
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
	XfdashboardSearchResultContainerPrivate		*priv;

	g_return_val_if_fail(XFDASHBOARD_IS_SEARCH_RESULT_CONTAINER(self), FALSE);
	g_return_val_if_fail(!inSelection || CLUTTER_IS_ACTOR(inSelection), FALSE);

	priv=self->priv;

	/* Check that selection is a child of this actor */
	if(inSelection && !xfdashboard_actor_contains_child_deep(CLUTTER_ACTOR(self), inSelection))
	{
		g_warning(_("%s is not a child of %s and cannot be selected"),
					G_OBJECT_TYPE_NAME(inSelection),
					G_OBJECT_TYPE_NAME(self));

		return(FALSE);
	}

	/* Set selection */
	priv->selectedItem=inSelection;

	/* We could successfully set selection so return success result */
	return(TRUE);
}

/* Find requested selection target depending of current selection */
ClutterActor* xfdashboard_search_result_container_find_selection(XfdashboardSearchResultContainer *self,
																	ClutterActor *inSelection,
																	XfdashboardSelectionTarget inDirection,
																	XfdashboardView *inView)
{
	XfdashboardSearchResultContainerPrivate		*priv;
	ClutterActor								*selection;
	ClutterActor								*newSelection;

	g_return_val_if_fail(XFDASHBOARD_IS_SEARCH_RESULT_CONTAINER(self), NULL);
	g_return_val_if_fail(!inSelection || CLUTTER_IS_ACTOR(inSelection), NULL);
	g_return_val_if_fail(inDirection>XFDASHBOARD_SELECTION_TARGET_NONE, NULL);
	g_return_val_if_fail(inDirection<=XFDASHBOARD_SELECTION_TARGET_NEXT, NULL);

	priv=self->priv;
	selection=inSelection;
	newSelection=NULL;

	/* If there is nothing selected, select first actor and return */
	if(!inSelection)
	{
		newSelection=clutter_actor_get_first_child(priv->itemsContainer);
		g_debug("No selection for %s, so select first child of result container for provider %s",
				G_OBJECT_TYPE_NAME(self),
				priv->provider ? G_OBJECT_TYPE_NAME(priv->provider) : "<unknown provider>");

		return(newSelection);
	}

	/* Check that selection is a child of this actor otherwise return NULL */
	if(!xfdashboard_actor_contains_child_deep(CLUTTER_ACTOR(self), inSelection))
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
				newSelection=_xfdashboard_search_result_container_find_selection_from_list_mode(self, inSelection, inDirection, inView);
			}
				else
				{
					newSelection=_xfdashboard_search_result_container_find_selection_from_icon_mode(self, inSelection, inDirection, inView);
				}
			break;

		case XFDASHBOARD_SELECTION_TARGET_FIRST:
			newSelection=clutter_actor_get_first_child(priv->itemsContainer);
			break;

		case XFDASHBOARD_SELECTION_TARGET_LAST:
			newSelection=clutter_actor_get_last_child(priv->itemsContainer);
			break;

		case XFDASHBOARD_SELECTION_TARGET_NEXT:
			newSelection=clutter_actor_get_next_sibling(inSelection);
			if(!newSelection) newSelection=clutter_actor_get_previous_sibling(inSelection);
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
	g_debug("Selecting %s at %s for current selection %s in direction %u",
			selection ? G_OBJECT_TYPE_NAME(selection) : "<nil>",
			G_OBJECT_TYPE_NAME(self),
			inSelection ? G_OBJECT_TYPE_NAME(inSelection) : "<nil>",
			inDirection);

	return(selection);
}
