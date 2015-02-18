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

#ifndef __XFDASHBOARD_SEARCH_RESULT_CONTAINER__
#define __XFDASHBOARD_SEARCH_RESULT_CONTAINER__

#include <clutter/clutter.h>

#include "actor.h"
#include "search-provider.h"
#include "types.h"
#include "view.h"

G_BEGIN_DECLS

#define XFDASHBOARD_TYPE_SEARCH_RESULT_CONTAINER				(xfdashboard_search_result_container_get_type())
#define XFDASHBOARD_SEARCH_RESULT_CONTAINER(obj)				(G_TYPE_CHECK_INSTANCE_CAST((obj), XFDASHBOARD_TYPE_SEARCH_RESULT_CONTAINER, XfdashboardSearchResultContainer))
#define XFDASHBOARD_IS_SEARCH_RESULT_CONTAINER(obj)				(G_TYPE_CHECK_INSTANCE_TYPE((obj), XFDASHBOARD_TYPE_SEARCH_RESULT_CONTAINER))
#define XFDASHBOARD_SEARCH_RESULT_CONTAINER_CLASS(klass)		(G_TYPE_CHECK_CLASS_CAST((klass), XFDASHBOARD_TYPE_SEARCH_RESULT_CONTAINER, XfdashboardSearchResultContainerClass))
#define XFDASHBOARD_IS_SEARCH_RESULT_CONTAINER_CLASS(klass)		(G_TYPE_CHECK_CLASS_TYPE((klass), XFDASHBOARD_TYPE_SEARCH_RESULT_CONTAINER))
#define XFDASHBOARD_SEARCH_RESULT_CONTAINER_GET_CLASS(obj)		(G_TYPE_INSTANCE_GET_CLASS((obj), XFDASHBOARD_TYPE_SEARCH_RESULT_CONTAINER, XfdashboardSearchResultContainerClass))

typedef struct _XfdashboardSearchResultContainer				XfdashboardSearchResultContainer;
typedef struct _XfdashboardSearchResultContainerClass			XfdashboardSearchResultContainerClass;
typedef struct _XfdashboardSearchResultContainerPrivate			XfdashboardSearchResultContainerPrivate;

struct _XfdashboardSearchResultContainer
{
	/* Parent instance */
	XfdashboardActor							parent_instance;

	/* Private structure */
	XfdashboardSearchResultContainerPrivate		*priv;
};

struct _XfdashboardSearchResultContainerClass
{
	/*< private >*/
	/* Parent class */
	XfdashboardActorClass						parent_class;

	/*< public >*/
	/* Virtual functions */
	void (*icon_clicked)(XfdashboardSearchResultContainer *self);
};

/* Public API */
typedef enum /*< skip,prefix=XFDASHBOARD_SEARCH_RESULT_CONTAINER_SELECTION_STEP_SIZE >*/
{
	XFDASHBOARD_SEARCH_RESULT_CONTAINER_SELECTION_STEP_SIZE_BEGIN_END,	/* Set to first or last item */
	XFDASHBOARD_SEARCH_RESULT_CONTAINER_SELECTION_STEP_SIZE_COLUMN,		/* Arrow left or arrow right key */
	XFDASHBOARD_SEARCH_RESULT_CONTAINER_SELECTION_STEP_SIZE_ROW			/* Arrow up or arrow down key */
} XfdashboardSearchResultContainerSelectionStepSize;

typedef enum /*< skip,prefix=XFDASHBOARD_SEARCH_RESULT_CONTAINER_SELECTION_DIRECTIO >*/
{
	XFDASHBOARD_SEARCH_RESULT_CONTAINER_SELECTION_DIRECTION_FORWARD,
	XFDASHBOARD_SEARCH_RESULT_CONTAINER_SELECTION_DIRECTION_BACKWARD
} XfdashboardSearchResultContainerSelectionDirection;

GType xfdashboard_search_result_container_get_type(void) G_GNUC_CONST;

ClutterActor* xfdashboard_search_result_container_new(XfdashboardSearchProvider *inProvider);

const gchar* xfdashboard_search_result_container_get_icon(XfdashboardSearchResultContainer *self);
void xfdashboard_search_result_container_set_icon(XfdashboardSearchResultContainer *self, const gchar *inIcon);

const gchar* xfdashboard_search_result_container_get_title_format(XfdashboardSearchResultContainer *self);
void xfdashboard_search_result_container_set_title_format(XfdashboardSearchResultContainer *self, const gchar *inFormat);

XfdashboardViewMode xfdashboard_search_result_container_get_view_mode(XfdashboardSearchResultContainer *self);
void xfdashboard_search_result_container_set_view_mode(XfdashboardSearchResultContainer *self, const XfdashboardViewMode inMode);

gfloat xfdashboard_search_result_container_get_spacing(XfdashboardSearchResultContainer *self);
void xfdashboard_search_result_container_set_spacing(XfdashboardSearchResultContainer *self, const gfloat inSpacing);

gfloat xfdashboard_search_result_container_get_padding(XfdashboardSearchResultContainer *self);
void xfdashboard_search_result_container_set_padding(XfdashboardSearchResultContainer *self, const gfloat inPadding);

void xfdashboard_search_result_container_add_result_actor(XfdashboardSearchResultContainer *self,
															ClutterActor *inResultActor,
															ClutterActor *inInsertAfter);

void xfdashboard_search_result_container_set_focus(XfdashboardSearchResultContainer *self, gboolean inSetFocus);

ClutterActor* xfdashboard_search_result_container_get_selection(XfdashboardSearchResultContainer *self);
gboolean xfdashboard_search_result_container_set_selection(XfdashboardSearchResultContainer *self,
																	ClutterActor *inSelection);
ClutterActor* xfdashboard_search_result_container_find_selection(XfdashboardSearchResultContainer *self,
																	ClutterActor *inSelection,
																	XfdashboardSelectionTarget inDirection,
																	XfdashboardView *inView);

G_END_DECLS

#endif	/* __XFDASHBOARD_SEARCH_RESULT_CONTAINER__ */
