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

#ifndef __LIBXFDASHBOARD_SEARCH_RESULT_CONTAINER__
#define __LIBXFDASHBOARD_SEARCH_RESULT_CONTAINER__

#if !defined(__LIBXFDASHBOARD_H_INSIDE__) && !defined(LIBXFDASHBOARD_COMPILATION)
#error "Only <libxfdashboard/libxfdashboard.h> can be included directly."
#endif

#include <clutter/clutter.h>

#include <libxfdashboard/actor.h>
#include <libxfdashboard/search-provider.h>
#include <libxfdashboard/types.h>
#include <libxfdashboard/view.h>

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
	/*< private >*/
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
	void (*item_clicked)(XfdashboardSearchResultContainer *self, GVariant *inItem, ClutterActor *inActor);
};

/* Public API */
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

gint xfdashboard_search_result_container_get_initial_result_size(XfdashboardSearchResultContainer *self);
void xfdashboard_search_result_container_set_initial_result_size(XfdashboardSearchResultContainer *self, const gint inSize);

gint xfdashboard_search_result_container_get_more_result_size(XfdashboardSearchResultContainer *self);
void xfdashboard_search_result_container_set_more_result_size(XfdashboardSearchResultContainer *self, const gint inSize);

void xfdashboard_search_result_container_set_focus(XfdashboardSearchResultContainer *self, gboolean inSetFocus);

ClutterActor* xfdashboard_search_result_container_get_selection(XfdashboardSearchResultContainer *self);
gboolean xfdashboard_search_result_container_set_selection(XfdashboardSearchResultContainer *self,
																	ClutterActor *inSelection);
ClutterActor* xfdashboard_search_result_container_find_selection(XfdashboardSearchResultContainer *self,
																	ClutterActor *inSelection,
																	XfdashboardSelectionTarget inDirection,
																	XfdashboardView *inView,
																	gboolean inAllowWrap);
void xfdashboard_search_result_container_activate_selection(XfdashboardSearchResultContainer *self,
																	ClutterActor *inSelection);

void xfdashboard_search_result_container_update(XfdashboardSearchResultContainer *self, XfdashboardSearchResultSet *inResults);

G_END_DECLS

#endif	/* __LIBXFDASHBOARD_SEARCH_RESULT_CONTAINER__ */
