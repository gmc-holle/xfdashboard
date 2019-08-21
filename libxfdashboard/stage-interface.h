/*
 * stage-interface: A top-level actor for a monitor at stage
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

#ifndef __LIBXFDASHBOARD_STAGE_INTERFACE__
#define __LIBXFDASHBOARD_STAGE_INTERFACE__

#if !defined(__LIBXFDASHBOARD_H_INSIDE__) && !defined(LIBXFDASHBOARD_COMPILATION)
#error "Only <libxfdashboard/libxfdashboard.h> can be included directly."
#endif

#include <clutter/clutter.h>

#include <libxfdashboard/stage.h>
#include <libxfdashboard/types.h>
#include <libxfdashboard/window-tracker-monitor.h>

G_BEGIN_DECLS

#define XFDASHBOARD_TYPE_STAGE_INTERFACE			(xfdashboard_stage_interface_get_type())
#define XFDASHBOARD_STAGE_INTERFACE(obj)			(G_TYPE_CHECK_INSTANCE_CAST((obj), XFDASHBOARD_TYPE_STAGE_INTERFACE, XfdashboardStageInterface))
#define XFDASHBOARD_IS_STAGE_INTERFACE(obj)			(G_TYPE_CHECK_INSTANCE_TYPE((obj), XFDASHBOARD_TYPE_STAGE_INTERFACE))
#define XFDASHBOARD_STAGE_INTERFACE_CLASS(klass)	(G_TYPE_CHECK_CLASS_CAST((klass), XFDASHBOARD_TYPE_STAGE_INTERFACE, XfdashboardStageInterfaceClass))
#define XFDASHBOARD_IS_STAGE_INTERFACE_CLASS(klass)	(G_TYPE_CHECK_CLASS_TYPE((klass), XFDASHBOARD_TYPE_STAGE_INTERFACE))
#define XFDASHBOARD_STAGE_INTERFACE_GET_CLASS(obj)	(G_TYPE_INSTANCE_GET_CLASS((obj), XFDASHBOARD_TYPE_STAGE_INTERFACE, XfdashboardStageInterfaceClass))

typedef struct _XfdashboardStageInterface			XfdashboardStageInterface;
typedef struct _XfdashboardStageInterfaceClass		XfdashboardStageInterfaceClass;
typedef struct _XfdashboardStageInterfacePrivate	XfdashboardStageInterfacePrivate;

struct _XfdashboardStageInterface
{
	/*< private >*/
	/* Parent instance */
	XfdashboardStage						parent_instance;

	/* Private structure */
	XfdashboardStageInterfacePrivate		*priv;
};

struct _XfdashboardStageInterfaceClass
{
	/*< private >*/
	/* Parent class */
	XfdashboardStageClass					parent_class;
};

/* Public API */
GType xfdashboard_stage_interface_get_type(void) G_GNUC_CONST;

ClutterActor* xfdashboard_stage_interface_new(void);

XfdashboardWindowTrackerMonitor* xfdashboard_stage_interface_get_monitor(XfdashboardStageInterface *self);
void xfdashboard_stage_interface_set_monitor(XfdashboardStageInterface *self, XfdashboardWindowTrackerMonitor *inMonitor);

XfdashboardStageBackgroundImageType xfdashboard_stage_interface_get_background_image_type(XfdashboardStageInterface *self);
void xfdashboard_stage_interface_set_background_image_type(XfdashboardStageInterface *self, XfdashboardStageBackgroundImageType inType);

ClutterColor* xfdashboard_stage_interface_get_background_color(XfdashboardStageInterface *self);
void xfdashboard_stage_interface_set_background_color(XfdashboardStageInterface *self, const ClutterColor *inColor);

G_END_DECLS

#endif	/* __LIBXFDASHBOARD_STAGE_INTERFACE__ */
