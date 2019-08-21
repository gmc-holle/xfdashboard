/*
 * application-database: A singelton managing desktop files and menus
 *                       for installed applications
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

#ifndef __LIBXFDASHBOARD_APPLICATION_DATABASE__
#define __LIBXFDASHBOARD_APPLICATION_DATABASE__

#if !defined(__LIBXFDASHBOARD_H_INSIDE__) && !defined(LIBXFDASHBOARD_COMPILATION)
#error "Only <libxfdashboard/libxfdashboard.h> can be included directly."
#endif

#include <garcon/garcon.h>

G_BEGIN_DECLS

#define XFDASHBOARD_TYPE_APPLICATION_DATABASE				(xfdashboard_application_database_get_type())
#define XFDASHBOARD_APPLICATION_DATABASE(obj)				(G_TYPE_CHECK_INSTANCE_CAST((obj), XFDASHBOARD_TYPE_APPLICATION_DATABASE, XfdashboardApplicationDatabase))
#define XFDASHBOARD_IS_APPLICATION_DATABASE(obj)			(G_TYPE_CHECK_INSTANCE_TYPE((obj), XFDASHBOARD_TYPE_APPLICATION_DATABASE))
#define XFDASHBOARD_APPLICATION_DATABASE_CLASS(klass)		(G_TYPE_CHECK_CLASS_CAST((klass), XFDASHBOARD_TYPE_APPLICATION_DATABASE, XfdashboardApplicationDatabaseClass))
#define XFDASHBOARD_IS_APPLICATION_DATABASE_CLASS(klass)	(G_TYPE_CHECK_CLASS_TYPE((klass), XFDASHBOARD_TYPE_APPLICATION_DATABASE))
#define XFDASHBOARD_APPLICATION_DATABASE_GET_CLASS(obj)		(G_TYPE_INSTANCE_GET_CLASS((obj), XFDASHBOARD_TYPE_APPLICATION_DATABASE, XfdashboardApplicationDatabaseClass))

typedef struct _XfdashboardApplicationDatabase				XfdashboardApplicationDatabase;
typedef struct _XfdashboardApplicationDatabaseClass			XfdashboardApplicationDatabaseClass;
typedef struct _XfdashboardApplicationDatabasePrivate		XfdashboardApplicationDatabasePrivate;

struct _XfdashboardApplicationDatabase
{
	/*< private >*/
	/* Parent instance */
	GObject									parent_instance;

	/* Private structure */
	XfdashboardApplicationDatabasePrivate	*priv;
};

struct _XfdashboardApplicationDatabaseClass
{
	/*< private >*/
	/* Parent class */
	GObjectClass							parent_class;

	/*< public >*/
	/* Virtual functions */
	void (*menu_reload_required)(XfdashboardApplicationDatabase *self);

	void (*application_added)(XfdashboardApplicationDatabase *self, GAppInfo *inAppInfo);
	void (*application_removed)(XfdashboardApplicationDatabase *self, GAppInfo *inAppInfo);
};

/* Public API */
GType xfdashboard_application_database_get_type(void) G_GNUC_CONST;

XfdashboardApplicationDatabase* xfdashboard_application_database_get_default(void);

gboolean xfdashboard_application_database_is_loaded(const XfdashboardApplicationDatabase *self);
gboolean xfdashboard_application_database_load(XfdashboardApplicationDatabase *self, GError **outError);

const GList* xfdashboard_application_database_get_application_search_paths(const XfdashboardApplicationDatabase *self);

GarconMenu* xfdashboard_application_database_get_application_menu(XfdashboardApplicationDatabase *self);
GList* xfdashboard_application_database_get_all_applications(XfdashboardApplicationDatabase *self);

GAppInfo* xfdashboard_application_database_lookup_desktop_id(XfdashboardApplicationDatabase *self,
																const gchar *inDesktopID);

gchar* xfdashboard_application_database_get_file_from_desktop_id(const gchar *inDesktopID);
gchar* xfdashboard_application_database_get_desktop_id_from_path(const gchar *inFilename);
gchar* xfdashboard_application_database_get_desktop_id_from_file(GFile *inFile);

G_END_DECLS

#endif	/* __LIBXFDASHBOARD_APPLICATION_DATABASE__ */
