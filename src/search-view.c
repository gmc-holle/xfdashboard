/*
 * search-view: A view showing search results
 * 
 * Copyright 2012-2013 Stephan Haller <nomad@froevel.de>
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

#include <glib/gi18n-lib.h>
#include <gtk/gtk.h>

#include "search-view.h"
#include "utils.h"

/* Define this class in GObject system */
G_DEFINE_TYPE(XfdashboardSearchView,
				xfdashboard_search_view,
				XFDASHBOARD_TYPE_VIEW)

/* Private structure - access only by public API if needed */
#define XFDASHBOARD_SEARCH_VIEW_GET_PRIVATE(obj) \
	(G_TYPE_INSTANCE_GET_PRIVATE((obj), XFDASHBOARD_TYPE_SEARCH_VIEW, XfdashboardSearchViewPrivate))

struct _XfdashboardSearchViewPrivate
{
	/* Instance related */
	void *dummy;
};

/* IMPLEMENTATION: Private variables and methods */
#define DEFAULT_VIEW_ICON	GTK_STOCK_FIND

/* IMPLEMENTATION: GObject */

/* Dispose this object */
void _xfdashboard_search_view_dispose(GObject *inObject)
{
	XfdashboardSearchView			*self=XFDASHBOARD_SEARCH_VIEW(inObject);
	XfdashboardSearchViewPrivate	*priv=self->priv;

	/* Call parent's class dispose method */
	G_OBJECT_CLASS(xfdashboard_search_view_parent_class)->dispose(inObject);
}

/* Class initialization
 * Override functions in parent classes and define properties
 * and signals
 */
void xfdashboard_search_view_class_init(XfdashboardSearchViewClass *klass)
{
	GObjectClass			*gobjectClass=G_OBJECT_CLASS(klass);

	/* Override functions */
	gobjectClass->dispose=_xfdashboard_search_view_dispose;

	/* Set up private structure */
	g_type_class_add_private(klass, sizeof(XfdashboardSearchViewPrivate));
}

/* Object initialization
 * Create private structure and set up default values
 */
void xfdashboard_search_view_init(XfdashboardSearchView *self)
{
	XfdashboardSearchViewPrivate	*priv;

	self->priv=priv=XFDASHBOARD_SEARCH_VIEW_GET_PRIVATE(self);

	/* Set up view (Note: Search view is disabled by default!) */
	xfdashboard_view_set_internal_name(XFDASHBOARD_VIEW(self), "search");
	xfdashboard_view_set_name(XFDASHBOARD_VIEW(self), _("Search"));
	xfdashboard_view_set_icon(XFDASHBOARD_VIEW(self), DEFAULT_VIEW_ICON);
	xfdashboard_view_set_enabled(XFDASHBOARD_VIEW(self), FALSE);
}

/* Implementation: Public API */

/* Update search view by looking up result for new search text */
void xfdashboard_search_view_update_search(XfdashboardSearchView *self, const gchar *inText)
{
	g_return_if_fail(XFDASHBOARD_IS_SEARCH_VIEW(self));

	g_message("*TODO* Updating search results for new criteria '%s'", inText);
}
