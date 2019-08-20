/*
 * applications-menu-model: A list model containing menu items
 *                          of applications
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

#ifndef __LIBXFDASHBOARD_APPLICATIONS_MENU_MODEL__
#define __LIBXFDASHBOARD_APPLICATIONS_MENU_MODEL__

#if !defined(__LIBXFDASHBOARD_H_INSIDE__) && !defined(LIBXFDASHBOARD_COMPILATION)
#error "Only <libxfdashboard/libxfdashboard.h> can be included directly."
#endif

#include <garcon/garcon.h>

#include <libxfdashboard/model.h>

G_BEGIN_DECLS

#define XFDASHBOARD_TYPE_APPLICATIONS_MENU_MODEL			(xfdashboard_applications_menu_model_get_type())
#define XFDASHBOARD_APPLICATIONS_MENU_MODEL(obj)			(G_TYPE_CHECK_INSTANCE_CAST((obj), XFDASHBOARD_TYPE_APPLICATIONS_MENU_MODEL, XfdashboardApplicationsMenuModel))
#define XFDASHBOARD_IS_APPLICATIONS_MENU_MODEL(obj)			(G_TYPE_CHECK_INSTANCE_TYPE((obj), XFDASHBOARD_TYPE_APPLICATIONS_MENU_MODEL))
#define XFDASHBOARD_APPLICATIONS_MENU_MODEL_CLASS(klass)	(G_TYPE_CHECK_CLASS_CAST((klass), XFDASHBOARD_TYPE_APPLICATIONS_MENU_MODEL, XfdashboardApplicationsMenuModelClass))
#define XFDASHBOARD_IS_APPLICATIONS_MENU_MODEL_CLASS(klass)	(G_TYPE_CHECK_CLASS_TYPE((klass), XFDASHBOARD_TYPE_APPLICATIONS_MENU_MODEL))
#define XFDASHBOARD_APPLICATIONS_MENU_MODEL_GET_CLASS(obj)	(G_TYPE_INSTANCE_GET_CLASS((obj), XFDASHBOARD_TYPE_APPLICATIONS_MENU_MODEL, XfdashboardApplicationsMenuModelClass))

typedef struct _XfdashboardApplicationsMenuModel			XfdashboardApplicationsMenuModel; 
typedef struct _XfdashboardApplicationsMenuModelPrivate		XfdashboardApplicationsMenuModelPrivate;
typedef struct _XfdashboardApplicationsMenuModelClass		XfdashboardApplicationsMenuModelClass;

struct _XfdashboardApplicationsMenuModel
{
	/*< private >*/
	/* Parent instance */
	XfdashboardModel							parent_instance;

	/* Private structure */
	XfdashboardApplicationsMenuModelPrivate		*priv;
};

struct _XfdashboardApplicationsMenuModelClass
{
	/*< private >*/
	/* Parent class */
	XfdashboardModelClass						parent_class;

	/*< public >*/
	/* Virtual functions */
	void (*loaded)(XfdashboardApplicationsMenuModel *self);
};

/* Public API */

/* Columns of model */
enum
{
	XFDASHBOARD_APPLICATIONS_MENU_MODEL_COLUMN_SEQUENCE_ID,

	XFDASHBOARD_APPLICATIONS_MENU_MODEL_COLUMN_MENU_ELEMENT,
	XFDASHBOARD_APPLICATIONS_MENU_MODEL_COLUMN_PARENT_MENU,
	XFDASHBOARD_APPLICATIONS_MENU_MODEL_COLUMN_SECTION,

	XFDASHBOARD_APPLICATIONS_MENU_MODEL_COLUMN_TITLE,
	XFDASHBOARD_APPLICATIONS_MENU_MODEL_COLUMN_DESCRIPTION,

	XFDASHBOARD_APPLICATIONS_MENU_MODEL_COLUMN_LAST
};

GType xfdashboard_applications_menu_model_get_type(void) G_GNUC_CONST;

XfdashboardModel* xfdashboard_applications_menu_model_new(void);

void xfdashboard_applications_menu_model_get(XfdashboardApplicationsMenuModel *self,
												XfdashboardModelIter *inIter,
												...);

void xfdashboard_applications_menu_model_filter_by_menu(XfdashboardApplicationsMenuModel *self,
														GarconMenu *inMenu);
void xfdashboard_applications_menu_model_filter_by_section(XfdashboardApplicationsMenuModel *self,
															GarconMenu *inSection);

G_END_DECLS

#endif	/* __LIBXFDASHBOARD_APPLICATIONS_MENU_MODEL__ */

