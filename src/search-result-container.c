/*
 * search-result-container: An container for results from a search provider
 *                          which has a header and container for items
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

#include "search-result-container.h"

#include <glib/gi18n-lib.h>
#include <gdk/gdk.h>
#include <math.h>

#include "enums.h"
#include "text-box.h"

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
	gchar						*titleFormat;
	XfdashboardViewMode			viewMode;
	gfloat						spacing;

	/* Instance related */
	ClutterLayoutManager		*layout;
	ClutterActor				*titleTextBox;
	ClutterActor				*itemsContainer;
};

/* Properties */
enum
{
	PROP_0,

	PROP_PROVIDER,

	PROP_TITLE_FORMAT,
	PROP_VIEW_MODE,
	PROP_SPACING,

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

/* Primary icon (provider icon) in title was clicked */
static void _xfdashboard_search_result_container_on_primary_icon_clicked(XfdashboardSearchResultContainer *self, gpointer inUserData)
{
	g_return_if_fail(XFDASHBOARD_IS_SEARCH_RESULT_CONTAINER(self));

	/* Emit signal for icon clicked */
	g_signal_emit(self, XfdashboardSearchResultContainerSignals[SIGNAL_ICON_CLICKED], 0);
}

/* Update title text box */
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
	const gchar									*providerIcon;

	g_return_if_fail(XFDASHBOARD_IS_SEARCH_RESULT_CONTAINER(self));
	g_return_if_fail(XFDASHBOARD_IS_SEARCH_PROVIDER(inProvider));

	priv=self->priv;

	/* Check that no provider was set yet */
	g_return_if_fail(priv->provider==NULL);

	/* Set provider reference */
	priv->provider=XFDASHBOARD_SEARCH_PROVIDER(g_object_ref(inProvider));

	/* Update title */
	_xfdashboard_search_result_container_update_title(self);

	/* If provider has an icon then update primary icon and connect signal */
	providerIcon=xfdashboard_search_provider_get_icon(priv->provider);
	if(providerIcon)
	{
		xfdashboard_text_box_set_primary_icon(XFDASHBOARD_TEXT_BOX(priv->titleTextBox), providerIcon);
		g_signal_connect_swapped(priv->titleTextBox,
									"primary-icon-clicked",
									G_CALLBACK(_xfdashboard_search_result_container_on_primary_icon_clicked),
									self);
	}
}

/* IMPLEMENTATION: GObject */

/* Dispose this object */
static void _xfdashboard_search_result_container_dispose(GObject *inObject)
{
	XfdashboardSearchResultContainerPrivate		*priv=XFDASHBOARD_SEARCH_RESULT_CONTAINER(inObject)->priv;

	/* Release allocated variables */
	if(priv->provider)
	{
		g_object_unref(priv->provider);
		priv->provider=NULL;
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

		case PROP_TITLE_FORMAT:
			xfdashboard_search_result_container_set_title_format(self, g_value_get_string(inValue));
			break;

		case PROP_VIEW_MODE:
			xfdashboard_search_result_container_set_view_mode(self, g_value_get_enum(inValue));
			break;

		case PROP_SPACING:
			xfdashboard_search_result_container_set_spacing(self, g_value_get_float(inValue));
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
		case PROP_TITLE_FORMAT:
			g_value_set_string(outValue, priv->titleFormat);
			break;

		case PROP_VIEW_MODE:
			g_value_set_enum(outValue, priv->viewMode);
			break;

		case PROP_SPACING:
			g_value_set_float(outValue, priv->spacing);
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
	// TODO: ClutterActorClass		*clutterActorClass=CLUTTER_ACTOR_CLASS(klass);
	GObjectClass			*gobjectClass=G_OBJECT_CLASS(klass);

	/* Override functions */
	gobjectClass->dispose=_xfdashboard_search_result_container_dispose;
	gobjectClass->set_property=_xfdashboard_search_result_container_set_property;
	gobjectClass->get_property=_xfdashboard_search_result_container_get_property;

	// TODO: clutterActorClass->get_preferred_width=_xfdashboard_search_result_container_get_preferred_width;
	// TODO: clutterActorClass->get_preferred_height=_xfdashboard_search_result_container_get_preferred_height;
	// TODO: clutterActorClass->allocate=_xfdashboard_search_result_container_allocate;

	/* Set up private structure */
	g_type_class_add_private(klass, sizeof(XfdashboardSearchResultContainerPrivate));

	/* Define properties */
	XfdashboardSearchResultContainerProperties[PROP_PROVIDER]=
		g_param_spec_object("provider",
							_("Provider"),
							_("The search provider this result container is for"),
							XFDASHBOARD_TYPE_SEARCH_PROVIDER,
							G_PARAM_WRITABLE | G_PARAM_STATIC_STRINGS | G_PARAM_CONSTRUCT_ONLY);

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

	g_object_class_install_properties(gobjectClass, PROP_LAST, XfdashboardSearchResultContainerProperties);

	/* Define stylable properties */
	xfdashboard_actor_install_stylable_property(actorClass, XfdashboardSearchResultContainerProperties[PROP_TITLE_FORMAT]);
	xfdashboard_actor_install_stylable_property(actorClass, XfdashboardSearchResultContainerProperties[PROP_VIEW_MODE]);
	xfdashboard_actor_install_stylable_property(actorClass, XfdashboardSearchResultContainerProperties[PROP_SPACING]);

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
	priv->titleFormat=NULL;
	priv->viewMode=-1;
	priv->spacing=0.0f;

	/* Set up children */
	clutter_actor_set_reactive(CLUTTER_ACTOR(self), FALSE);

	priv->titleTextBox=xfdashboard_text_box_new();
	clutter_actor_set_x_expand(priv->titleTextBox, TRUE);
	xfdashboard_actor_add_style_class(XFDASHBOARD_ACTOR(priv->titleTextBox), "title");

	priv->itemsContainer=xfdashboard_actor_new();
	clutter_actor_set_x_expand(CLUTTER_ACTOR(priv->itemsContainer), TRUE);
	xfdashboard_actor_add_style_class(XFDASHBOARD_ACTOR(priv->itemsContainer), "items-container");
	xfdashboard_search_result_container_set_view_mode(self, DEFAULT_VIEW_MODE);

	/* Set up actor */
	layout=clutter_box_layout_new();
	clutter_box_layout_set_orientation(CLUTTER_BOX_LAYOUT(layout), CLUTTER_ORIENTATION_VERTICAL);

	clutter_actor_set_layout_manager(CLUTTER_ACTOR(self), layout);
	clutter_actor_set_x_expand(CLUTTER_ACTOR(self), TRUE);
	clutter_actor_add_child(CLUTTER_ACTOR(self), priv->titleTextBox);
	clutter_actor_add_child(CLUTTER_ACTOR(self), priv->itemsContainer);
}

/* IMPLEMENTATION: Public API */

/* Create new actor */
ClutterActor* xfdashboard_search_result_container_new(XfdashboardSearchProvider *inProvider)
{
	return(g_object_new(XFDASHBOARD_TYPE_SEARCH_RESULT_CONTAINER,
							"provider", inProvider,
							NULL));
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
				clutter_actor_set_layout_manager(CLUTTER_ACTOR(priv->itemsContainer), priv->layout);

				removeClass="view-mode-icon";
				addClass="view-mode-list";
				break;

			case XFDASHBOARD_VIEW_MODE_ICON:
				priv->layout=clutter_flow_layout_new(CLUTTER_FLOW_HORIZONTAL);
				clutter_flow_layout_set_column_spacing(CLUTTER_FLOW_LAYOUT(priv->layout), priv->spacing);
				clutter_flow_layout_set_row_spacing(CLUTTER_FLOW_LAYOUT(priv->layout), priv->spacing);
				clutter_flow_layout_set_homogeneous(CLUTTER_FLOW_LAYOUT(priv->layout), TRUE);
				clutter_actor_set_layout_manager(CLUTTER_ACTOR(priv->itemsContainer), priv->layout);

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
			if(!XFDASHBOARD_IS_ACTOR(child)) continue;

			xfdashboard_actor_remove_style_class(XFDASHBOARD_ACTOR(child), removeClass);
			xfdashboard_actor_add_style_class(XFDASHBOARD_ACTOR(child), addClass);
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
				clutter_flow_layout_set_column_spacing(CLUTTER_FLOW_LAYOUT(priv->layout), priv->spacing);
				clutter_flow_layout_set_row_spacing(CLUTTER_FLOW_LAYOUT(priv->layout), priv->spacing);
				break;

			default:
				g_assert_not_reached();
		}

		/* Notify about property change */
		g_object_notify_by_pspec(G_OBJECT(self), XfdashboardSearchResultContainerProperties[PROP_SPACING]);
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
	if(XFDASHBOARD_IS_ACTOR(inResultActor))
	{
		if(priv->viewMode==XFDASHBOARD_VIEW_MODE_LIST) xfdashboard_actor_add_style_class(XFDASHBOARD_ACTOR(inResultActor), "view-mode-list");
			else xfdashboard_actor_add_style_class(XFDASHBOARD_ACTOR(inResultActor), "view-mode-icon");

		xfdashboard_actor_add_style_class(XFDASHBOARD_ACTOR(inResultActor), "result-item");
	}

	/* Add actor to items container */
	clutter_actor_set_x_expand(inResultActor, TRUE);

	if(!inInsertAfter) clutter_actor_insert_child_below(priv->itemsContainer, inResultActor, NULL);
		else clutter_actor_insert_child_above(priv->itemsContainer, inResultActor, inInsertAfter);
}
