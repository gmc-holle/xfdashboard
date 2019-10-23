/*
 * transition-group: A grouping transition
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

#ifndef __LIBXFDASHBOARD_TRANSITION_GROUP__
#define __LIBXFDASHBOARD_TRANSITION_GROUP__

#if !defined(__LIBXFDASHBOARD_H_INSIDE__) && !defined(LIBXFDASHBOARD_COMPILATION)
#error "Only <libxfdashboard/libxfdashboard.h> can be included directly."
#endif

#include <clutter/clutter.h>

#include <libxfdashboard/actor.h>

G_BEGIN_DECLS

/* Object declaration */
#define XFDASHBOARD_TYPE_TRANSITION_GROUP				(xfdashboard_transition_group_get_type())
#define XFDASHBOARD_TRANSITION_GROUP(obj)				(G_TYPE_CHECK_INSTANCE_CAST((obj), XFDASHBOARD_TYPE_TRANSITION_GROUP, XfdashboardTransitionGroup))
#define XFDASHBOARD_IS_TRANSITION_GROUP(obj)			(G_TYPE_CHECK_INSTANCE_TYPE((obj), XFDASHBOARD_TYPE_TRANSITION_GROUP))
#define XFDASHBOARD_TRANSITION_GROUP_CLASS(klass)		(G_TYPE_CHECK_CLASS_CAST((klass), XFDASHBOARD_TYPE_TRANSITION_GROUP, XfdashboardTransitionGroupClass))
#define XFDASHBOARD_IS_TRANSITION_GROUP_CLASS(klass)	(G_TYPE_CHECK_CLASS_TYPE((klass), XFDASHBOARD_TYPE_TRANSITION_GROUP))
#define XFDASHBOARD_TRANSITION_GROUP_GET_CLASS(obj)	(G_TYPE_INSTANCE_GET_CLASS((obj), XFDASHBOARD_TYPE_TRANSITION_GROUP, XfdashboardTransitionGroupClass))

typedef struct _XfdashboardTransitionGroup			XfdashboardTransitionGroup;
typedef struct _XfdashboardTransitionGroupClass		XfdashboardTransitionGroupClass;
typedef struct _XfdashboardTransitionGroupPrivate		XfdashboardTransitionGroupPrivate;

/**
 * XfdashboardTransitionGroup:
 *
 * The #XfdashboardTransitionGroup structure contains only private data and
 * should be accessed using the provided API
 */
 struct _XfdashboardTransitionGroup
{
	/*< private >*/
	/* Parent instance */
	ClutterTransition					parent_instance;

	/* Private structure */
	XfdashboardTransitionGroupPrivate	*priv;
};

/**
 * XfdashboardTransitionGroupClass:
 *
 * The #XfdashboardTransitionGroupClass structure contains only private data
 */
struct _XfdashboardTransitionGroupClass
{
	/*< private >*/
	/* Parent class */
	ClutterTransitionClass				parent_class;

	/*< public >*/
	/* Virtual functions */
};

/* Public API */
GType xfdashboard_transition_group_get_type(void) G_GNUC_CONST;

ClutterTransition* xfdashboard_transition_group_new(void);

void xfdashboard_transition_group_add_transition(XfdashboardTransitionGroup *self,
													ClutterTransition *inTransition);
void xfdashboard_transition_group_remove_transition(XfdashboardTransitionGroup *self,
													ClutterTransition *inTransition);
void xfdashboard_transition_group_remove_all(XfdashboardTransitionGroup *self);

GSList* xfdashboard_transition_group_get_transitions(XfdashboardTransitionGroup *self);

G_END_DECLS

#endif	/* __LIBXFDASHBOARD_TRANSITION_GROUP__ */
