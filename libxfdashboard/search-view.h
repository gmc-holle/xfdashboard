/*
 * search-view: A view showing applications matching search criteria
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

#ifndef __LIBXFDASHBOARD_SEARCH_VIEW__
#define __LIBXFDASHBOARD_SEARCH_VIEW__

#if !defined(__LIBXFDASHBOARD_H_INSIDE__) && !defined(LIBXFDASHBOARD_COMPILATION)
#error "Only <libxfdashboard/libxfdashboard.h> can be included directly."
#endif

#include <libxfdashboard/view.h>

G_BEGIN_DECLS

#define XFDASHBOARD_TYPE_SEARCH_VIEW			(xfdashboard_search_view_get_type())
#define XFDASHBOARD_SEARCH_VIEW(obj)			(G_TYPE_CHECK_INSTANCE_CAST((obj), XFDASHBOARD_TYPE_SEARCH_VIEW, XfdashboardSearchView))
#define XFDASHBOARD_IS_SEARCH_VIEW(obj)			(G_TYPE_CHECK_INSTANCE_TYPE((obj), XFDASHBOARD_TYPE_SEARCH_VIEW))
#define XFDASHBOARD_SEARCH_VIEW_CLASS(klass)	(G_TYPE_CHECK_CLASS_CAST((klass), XFDASHBOARD_TYPE_SEARCH_VIEW, XfdashboardSearchViewClass))
#define XFDASHBOARD_IS_SEARCH_VIEW_CLASS(klass)	(G_TYPE_CHECK_CLASS_TYPE((klass), XFDASHBOARD_TYPE_SEARCH_VIEW))
#define XFDASHBOARD_SEARCH_VIEW_GET_CLASS(obj)	(G_TYPE_INSTANCE_GET_CLASS((obj), XFDASHBOARD_TYPE_SEARCH_VIEW, XfdashboardSearchViewClass))

typedef struct _XfdashboardSearchView			XfdashboardSearchView; 
typedef struct _XfdashboardSearchViewPrivate	XfdashboardSearchViewPrivate;
typedef struct _XfdashboardSearchViewClass		XfdashboardSearchViewClass;

/**
 * XfdashboardSearchView:
 *
 * The #XfdashboardSearchView structure contains only private data and
 * should be accessed using the provided API
 */
struct _XfdashboardSearchView
{
	/*< private >*/
	/* Parent instance */
	XfdashboardView					parent_instance;

	/* Private structure */
	XfdashboardSearchViewPrivate	*priv;
};

/**
 * XfdashboardSearchViewClass:
 * @search_reset: Class handler for the #XfdashboardSearchViewClass::search-reset signal
 * @search_updated: Class handler for the #XfdashboardSearchViewClass::search-updated signal
 *
 * The #XfdashboardSearchViewClass structure contains only private data
 */
struct _XfdashboardSearchViewClass
{
	/*< private >*/
	/* Parent class */
	XfdashboardViewClass			parent_class;

	/*< public >*/
	/* Virtual functions */
	void (*search_reset)(XfdashboardSearchView *self);
	void (*search_updated)(XfdashboardSearchView *self);
};

/* Public API */
GType xfdashboard_search_view_get_type(void) G_GNUC_CONST;

void xfdashboard_search_view_reset_search(XfdashboardSearchView *self);
void xfdashboard_search_view_update_search(XfdashboardSearchView *self, const gchar *inSearchString);

G_END_DECLS

#endif	/* __LIBXFDASHBOARD_SEARCH_VIEW__ */
