/*
 * stage: Global stage of application
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

#ifndef __LIBXFDASHBOARD_STAGE__
#define __LIBXFDASHBOARD_STAGE__

#if !defined(__LIBXFDASHBOARD_H_INSIDE__) && !defined(LIBXFDASHBOARD_COMPILATION)
#error "Only <libxfdashboard/libxfdashboard.h> can be included directly."
#endif

#include <clutter/clutter.h>

#include <libxfdashboard/types.h>

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
	/*< private >*/
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
	void (*actor_created)(XfdashboardStage *self, ClutterActor *inActor);

	void (*search_started)(XfdashboardStage *self);
	void (*search_changed)(XfdashboardStage *self, gchar *inText);
	void (*search_ended)(XfdashboardStage *self);

	void (*show_tooltip)(XfdashboardStage *self, ClutterAction *inAction);
	void (*hide_tooltip)(XfdashboardStage *self, ClutterAction *inAction);
};

/* Public API */
GType xfdashboard_stage_get_type(void) G_GNUC_CONST;

ClutterActor* xfdashboard_stage_new(void);

XfdashboardStageBackgroundImageType xfdashboard_stage_get_background_image_type(XfdashboardStage *self);
void xfdashboard_stage_set_background_image_type(XfdashboardStage *self, XfdashboardStageBackgroundImageType inType);

ClutterColor* xfdashboard_stage_get_background_color(XfdashboardStage *self);
void xfdashboard_stage_set_background_color(XfdashboardStage *self, const ClutterColor *inColor);

const gchar* xfdashboard_stage_get_switch_to_view(XfdashboardStage *self);
void xfdashboard_stage_set_switch_to_view(XfdashboardStage *self, const gchar *inViewInternalName);

void xfdashboard_stage_show_notification(XfdashboardStage *self, const gchar *inIconName, const gchar *inText);

G_END_DECLS

#endif	/* __LIBXFDASHBOARD_STAGE__ */
