/*
 * stage: A stage for a monitor
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

#ifndef __XFOVERVIEW_STAGE__
#define __XFOVERVIEW_STAGE__

#include <clutter/clutter.h>

#define WNCK_I_KNOW_THIS_IS_UNSTABLE
#include <libwnck/libwnck.h>

G_BEGIN_DECLS

#define XFDASHBOARD_TYPE_STAGE				(xfdashboard_stage_get_type())
#define XFDASHBOARD_STAGE(obj)				(G_TYPE_CHECK_INSTANCE_CAST((obj), XFDASHBOARD_TYPE_STAGE, XfdashboardStage))
#define XFDASHBOARD_IS_STAGE(obj)			(G_TYPE_CHECK_INSTANCE_TYPE((obj), XFDASHBOARD_TYPE_STAGE))
#define XFDASHBOARD_STAGE_CLASS(klass)		(G_TYPE_CHECK_CLASS_CAST((klass), XFDASHBOARD_TYPE_STAGE, XfdashboardStageClass))
#define XFDASHBOARD_IS_STAGE_CLASS(klass)	(G_TYPE_CHECK_CLASS_TYPE((klass), XFDASHBOARD_TYPE_STAGE))
#define XFDASHBOARD_STAGE_GET_CLASS(obj)	(G_TYPE_INSTANCE_GET_CLASS((obj), XFDASHBOARD_TYPE_STAGE, XfdashboardStageClass))

typedef struct _XfdashboardStage			XfdashboardStage;
typedef struct _XfdashboardStageClass		XfdashboardStageClass;
typedef struct _XfdashboardStagePrivate		XfdashboardStagePrivate;

struct _XfdashboardStage
{
	/* Parent instance */
	ClutterStage							parent_instance;

	/* Private structure */
	XfdashboardStagePrivate					*priv;
};

struct _XfdashboardStageClass
{
	/*< private >*/
	/* Parent class */
	ClutterStageClass						parent_class;

	/*< public >*/
	/* Virtual functions */
};

/* Public API */
GType xfdashboard_stage_get_type(void) G_GNUC_CONST;

ClutterActor* xfdashboard_stage_new(void);

WnckWindow* xfdashboard_stage_get_window(XfdashboardStage *self);

G_END_DECLS

#endif	/* __XFOVERVIEW_STAGE__ */
