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

#ifndef __XFDASHBOARD_SEARCH_VIEW__
#define __XFDASHBOARD_SEARCH_VIEW__

#include "view.h"

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

struct _XfdashboardSearchView
{
	/* Parent instance */
	XfdashboardView					parent_instance;

	/* Private structure */
	XfdashboardSearchViewPrivate	*priv;
};

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

void xfdashboard_search_view_reset_search_selection(XfdashboardSearchView *self);

G_END_DECLS

#endif	/* __XFDASHBOARD_SEARCH_VIEW__ */
