/*
 * transition-group: A grouping transition
 * 
 * Copyright 2012-2021 Stephan Haller <nomad@froevel.de>
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

/**
 * SECTION:transition-group
 * @short_description: Group transitions together
 * @include: xfdashboard/transition-group.h
 *
 * The #XfdashboardTransitionGroup allows running multiple #ClutterTransition
 * instances concurrently.
 * 
 * The #XfdashboardTransitionGroup is a one-to-one copy of #ClutterTransitionGroup
 * with the only enhancements that API functions to get a list of all transitions
 * grouped together in that grouping transition was added as well as all timeline
 * configuration, e.g. duration, direction, repeats etc., that are set on an
 * #XfdashboardTransitionGroup will also applied to all transitions added to this
 * group.
 *
 * In addition it will also reset "min-width-set", "min-height-set" and other
 * "*-set" properties to their values before the #XfdashboardTransitionGroup
 * was attached the animatable actor. This will reset commonly statically set
 * size and transformation properties, like "min-width", "min-height", "natural-width"
 * etc., which will prevent a layout manager to use the natural width and/or height
 * of an actor. This behaviour can be turned off via setting property "reset-flags"
 * to boolean FALSE (enabled and set to TRUE by default).
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <libxfdashboard/transition-group.h>

#include <glib/gi18n-lib.h>

#include <libxfdashboard/compat.h>
#include <libxfdashboard/debug.h>


/* Forward declarations */
typedef struct _XfdashboardTransitionActorSetFlags			XfdashboardTransitionActorSetFlags;
struct _XfdashboardTransitionActorSetFlags
{
	/* Value of property "*-set" at animatable actor */
	gboolean	fixedPosition;
	gboolean	minWidth;
	gboolean	minHeight;
	gboolean	naturalWidth;
	gboolean	naturalHeight;
	gboolean	transform;
	gboolean	childTransform;
	gboolean	backgroundColor;
};


/* Define this class in GObject system */
struct _XfdashboardTransitionGroupPrivate
{
	/* Properties related */
	gboolean							resetFlags;

	/* Instance related */
	GHashTable							*transitions;
	XfdashboardTransitionActorSetFlags	flags;
};

G_DEFINE_TYPE_WITH_PRIVATE(XfdashboardTransitionGroup,
							xfdashboard_transition_group,
							CLUTTER_TYPE_TRANSITION);


/* Properties */
enum
{
	PROP_0,

	PROP_RESET_FLAGS,

	PROP_LAST
};

static GParamSpec* XfdashboardTransitionGroupProperties[PROP_LAST]={ 0, };


/* IMPLEMENTATION: ClutterTimeline */

/* Time at transition has elapsed, so advance all transitions as well */
static void _xfdashboard_transition_group_timeline_new_frame(ClutterTimeline *inTimeline,
																gint inElapsed)
{
	XfdashboardTransitionGroup			*self;
	XfdashboardTransitionGroupPrivate	*priv;
	GHashTableIter						iter;
	gpointer							element;
	gint								currentLoop;
#ifdef DEBUG
	gboolean							doDebug=FALSE;
#endif

	g_return_if_fail(XFDASHBOARD_IS_TRANSITION_GROUP(inTimeline));

	self=XFDASHBOARD_TRANSITION_GROUP(inTimeline);
	priv=self->priv;

	/* Get current number of loop repeated */
	currentLoop=clutter_timeline_get_repeat_count(inTimeline);

	/* Iterate through transitions and advance their timeline */
#ifdef DEBUG
	if(doDebug)
	{
		ClutterAnimatable				*animatable;

		animatable=clutter_transition_get_animatable(CLUTTER_TRANSITION(self));
		XFDASHBOARD_DEBUG(self, ANIMATION,
							"Processing 'new-frame' signal for %s@%p with state %s and timeline %u ms/%u ms for animatable actor %s@%p",
							G_OBJECT_TYPE_NAME(self),
							self,
							clutter_timeline_is_playing(inTimeline) ? "PLAYING" : "STOPPED",
							clutter_timeline_get_elapsed_time(inTimeline),
							clutter_timeline_get_duration(inTimeline),
							G_OBJECT_TYPE_NAME(animatable),
							animatable);
	}
#endif

	g_hash_table_iter_init(&iter, priv->transitions);
	while(g_hash_table_iter_next(&iter, &element, NULL))
	{
		guint							maxDuration;
		gint							maxLoop;

		maxDuration=clutter_timeline_get_delay(CLUTTER_TIMELINE(element))+clutter_timeline_get_duration(CLUTTER_TIMELINE(element));
		maxLoop=clutter_timeline_get_repeat_count(CLUTTER_TIMELINE(element));
		if(((guint)inElapsed)<=maxDuration &&
			(maxLoop<0 || currentLoop<=maxLoop))
		{
			clutter_timeline_advance(CLUTTER_TIMELINE(element), clutter_timeline_get_elapsed_time(inTimeline));
			g_signal_emit_by_name(element, "new-frame", 0, clutter_timeline_get_elapsed_time(inTimeline));
#ifdef DEBUG
			if(doDebug)
			{
				const gchar				*propertyName=clutter_property_transition_get_property_name(CLUTTER_PROPERTY_TRANSITION(element));
				ClutterInterval			*propertyInterval=clutter_transition_get_interval(CLUTTER_TRANSITION(element));
				GValue					*fromValue=clutter_interval_peek_initial_value(propertyInterval);
				gchar					*fromValueString;
				GValue					*toValue=clutter_interval_peek_final_value(propertyInterval);
				gchar					*toValueString;
				GValue					currentValue=G_VALUE_INIT;
				gchar					*currentValueString;
				ClutterAnimatable		*animatable;

				animatable=clutter_transition_get_animatable(CLUTTER_TRANSITION(element));

				g_value_init(&currentValue, clutter_interval_get_value_type(propertyInterval));
				g_object_get_property(G_OBJECT(animatable), propertyName, &currentValue);

				fromValueString=g_strdup_value_contents(fromValue);
				toValueString=g_strdup_value_contents(toValue);
				currentValueString=g_strdup_value_contents(&currentValue);

				XFDASHBOARD_DEBUG(self, ANIMATION,
									"Processing transition %s@%p with timeline %u ms/%u ms and loop of %d/%d, so set property '%s' to %s (from=%s, to=%s)",
									G_OBJECT_TYPE_NAME(element),
									element,
									clutter_timeline_get_elapsed_time(CLUTTER_TIMELINE(element)),
									maxDuration,
									currentLoop,
									maxLoop,
									propertyName,
									currentValueString,
									fromValueString,
									toValueString);

				g_free(currentValueString);
				g_free(toValueString);
				g_free(fromValueString);
				g_value_unset(&currentValue);
			}
#endif
		}
#ifdef DEBUG
			else
			{
				if(doDebug)
				{
					XFDASHBOARD_DEBUG(self, ANIMATION,
										"Skipping transition %s@%p because elapsed time of %u ms and loop %d is behind its duration of %u ms and max loop of %d",
										G_OBJECT_TYPE_NAME(element),
										element,
										inElapsed,
										currentLoop,
										maxDuration,
										maxLoop);
				}
			}
#endif
	}

#ifdef DEBUG
	if(doDebug)
	{
		ClutterAnimatable				*animatable;

		animatable=clutter_transition_get_animatable(CLUTTER_TRANSITION(self));
		XFDASHBOARD_DEBUG(self, ANIMATION,
							"Processed 'new-frame' signal for %s@%p and animatable actor %s@%p",
							G_OBJECT_TYPE_NAME(self),
							self,
							G_OBJECT_TYPE_NAME(animatable),
							animatable);
	}
#endif
}

/* This transition group was started */
static void _xfdashboard_transition_group_timeline_started(ClutterTimeline *inTimeline)
{
	XfdashboardTransitionGroup			*self;
	XfdashboardTransitionGroupPrivate	*priv;
	GHashTableIter						iter;
	gpointer							element;

	g_return_if_fail(XFDASHBOARD_IS_TRANSITION_GROUP(inTimeline));

	self=XFDASHBOARD_TRANSITION_GROUP(inTimeline);
	priv=self->priv;

	/* Iterate through transitions and emit start signal */
	g_hash_table_iter_init(&iter, priv->transitions);
	while(g_hash_table_iter_next(&iter, &element, NULL))
	{
		g_signal_emit_by_name(element, "started");
	}
}


/* IMPLEMENTATION: ClutterTransition */

/* Transition group was attached to an animatable actor */
static void _xfdashboard_transition_group_transition_attached(ClutterTransition *inTransition,
																ClutterAnimatable *inAnimatable)
{
	XfdashboardTransitionGroup				*self;
	XfdashboardTransitionGroupPrivate		*priv;
	GHashTableIter							iter;
	gpointer								element;

	g_return_if_fail(XFDASHBOARD_IS_TRANSITION_GROUP(inTransition));
	g_return_if_fail(CLUTTER_IS_ANIMATABLE(inAnimatable));

	self=XFDASHBOARD_TRANSITION_GROUP(inTransition);
	priv=self->priv;

	/* Iterate through transitions and set the animatable actor
	 * it is attached to.
	 */
	g_hash_table_iter_init(&iter, priv->transitions);
	while(g_hash_table_iter_next(&iter, &element, NULL))
	{
		clutter_transition_set_animatable(CLUTTER_TRANSITION(element), inAnimatable);
	}

	/* Get current value of "*-set" properties */
	memset(&priv->flags, 0, sizeof(priv->flags));
	g_object_get(inAnimatable,
					"fixed-position-set", &priv->flags.fixedPosition,
					"min-width-set", &priv->flags.minWidth,
					"min-height-set", &priv->flags.minHeight,
					"natural-width-set", &priv->flags.naturalWidth,
					"natural-height-set", &priv->flags.naturalHeight,
					"transform-set", &priv->flags.transform,
					"child-transform-set", &priv->flags.childTransform,
					"background-color-set", &priv->flags.backgroundColor,
					NULL);
}

/* Transition group was detached from an animatable actor */
static void _xfdashboard_transition_group_transition_detached(ClutterTransition *inTransition,
																ClutterAnimatable *inAnimatable)
{
	XfdashboardTransitionGroup				*self;
	XfdashboardTransitionGroupPrivate		*priv;
	GHashTableIter							iter;
	gpointer								element;

	g_return_if_fail(XFDASHBOARD_IS_TRANSITION_GROUP(inTransition));
	g_return_if_fail(CLUTTER_IS_ANIMATABLE(inAnimatable));

	self=XFDASHBOARD_TRANSITION_GROUP(inTransition);
	priv=self->priv;

	/* Iterate through transitions and reset the animatable actor */
	g_hash_table_iter_init(&iter, priv->transitions);
	while(g_hash_table_iter_next(&iter, &element, NULL))
	{
		clutter_transition_set_animatable(CLUTTER_TRANSITION(element), NULL);
	}

	/* Restore "*-set" properties if requested */
	if(priv->resetFlags)
	{
		XfdashboardTransitionActorSetFlags	newFlags;

		/* Get current value of "*-set" properties */
		memset(&newFlags, 0, sizeof(newFlags));
		g_object_get(inAnimatable,
						"fixed-position-set", &newFlags.fixedPosition,
						"min-width-set", &newFlags.minWidth,
						"min-height-set", &newFlags.minHeight,
						"natural-width-set", &newFlags.naturalWidth,
						"natural-height-set", &newFlags.naturalHeight,
						"transform-set", &newFlags.transform,
						"child-transform-set", &newFlags.childTransform,
						"background-color-set", &newFlags.backgroundColor,
						NULL);

		/* Restore the values which have changed to their old value */
		if(newFlags.fixedPosition!=priv->flags.fixedPosition)
		{
			g_object_set(inAnimatable, "fixed-position-set", priv->flags.fixedPosition, NULL);
			XFDASHBOARD_DEBUG(self, ANIMATION,
								"Restoring property 'fixed-position-set' at actor %s@%p",
								G_OBJECT_TYPE_NAME(inAnimatable), inAnimatable);
		}

		if(newFlags.minWidth!=priv->flags.minWidth)
		{
			g_object_set(inAnimatable, "min-width-set", priv->flags.minWidth, NULL);
			XFDASHBOARD_DEBUG(self, ANIMATION,
								"Restoring property 'min-width-set' at actor %s@%p",
								G_OBJECT_TYPE_NAME(inAnimatable), inAnimatable);
		}

		if(newFlags.minHeight!=priv->flags.minHeight)
		{
			g_object_set(inAnimatable, "min-height-set", priv->flags.minHeight, NULL);
			XFDASHBOARD_DEBUG(self, ANIMATION,
								"Restoring property 'min-height-set' at actor %s@%p",
								G_OBJECT_TYPE_NAME(inAnimatable), inAnimatable);
		}

		if(newFlags.naturalWidth!=priv->flags.naturalWidth)
		{
			g_object_set(inAnimatable, "natural-width-set", priv->flags.naturalWidth, NULL);
			XFDASHBOARD_DEBUG(self, ANIMATION,
								"Restoring property 'natural-width-set' at actor %s@%p",
								G_OBJECT_TYPE_NAME(inAnimatable), inAnimatable);
		}

		if(newFlags.naturalHeight!=priv->flags.naturalHeight)
		{
			g_object_set(inAnimatable, "natural-height-set", priv->flags.naturalHeight, NULL);
			XFDASHBOARD_DEBUG(self, ANIMATION,
								"Restoring property 'natural-height-set' at actor %s@%p",
								G_OBJECT_TYPE_NAME(inAnimatable), inAnimatable);
		}

		if(newFlags.transform!=priv->flags.transform)
		{
			g_object_set(inAnimatable, "transform-set", priv->flags.transform, NULL);
			XFDASHBOARD_DEBUG(self, ANIMATION,
								"Restoring property 'transform-set' at actor %s@%p",
								G_OBJECT_TYPE_NAME(inAnimatable), inAnimatable);
		}

		if(newFlags.childTransform!=priv->flags.childTransform)
		{
			g_object_set(inAnimatable, "child-transform-set", priv->flags.childTransform, NULL);
			XFDASHBOARD_DEBUG(self, ANIMATION,
								"Restoring property 'child-transform-set' at actor %s@%p",
								G_OBJECT_TYPE_NAME(inAnimatable), inAnimatable);
		}

		if(newFlags.backgroundColor!=priv->flags.backgroundColor)
		{
			g_object_set(inAnimatable, "background-color-set", priv->flags.backgroundColor, NULL);
			XFDASHBOARD_DEBUG(self, ANIMATION,
								"Restoring property 'background-color-set' at actor %s@%p",
								G_OBJECT_TYPE_NAME(inAnimatable), inAnimatable);
		}
	}
}


/* IMPLEMENTATION: GObject */

/* Dispose this object */
static void _xfdashboard_transition_group_dispose(GObject *inObject)
{
	XfdashboardTransitionGroup			*self=XFDASHBOARD_TRANSITION_GROUP(inObject);
	ClutterAnimatable					*animatable;

	/* Restore "*-set" properties */
	animatable=clutter_transition_get_animatable(CLUTTER_TRANSITION(self));
	if(animatable) _xfdashboard_transition_group_transition_detached(CLUTTER_TRANSITION(self), animatable);

	/* Call parent's class dispose method */
	G_OBJECT_CLASS(xfdashboard_transition_group_parent_class)->dispose(inObject);
}

/* Finalize this object */
static void _xfdashboard_transition_group_finalize(GObject *inObject)
{
	XfdashboardTransitionGroup			*self=XFDASHBOARD_TRANSITION_GROUP(inObject);
	XfdashboardTransitionGroupPrivate	*priv=self->priv;

	/* Release our allocated variables */
	if(priv->transitions)
	{
		g_hash_table_unref(priv->transitions);
		priv->transitions=NULL;
	}

	/* Call parent's class dispose method */
	G_OBJECT_CLASS(xfdashboard_transition_group_parent_class)->finalize(inObject);
}

/* Set/get properties */
static void _xfdashboard_transition_group_set_property(GObject *inObject,
														guint inPropID,
														const GValue *inValue,
														GParamSpec *inSpec)
{
	XfdashboardTransitionGroup			*self=XFDASHBOARD_TRANSITION_GROUP(inObject);
	XfdashboardTransitionGroupPrivate	*priv=self->priv;

	switch(inPropID)
	{
		case PROP_RESET_FLAGS:
			priv->resetFlags=g_value_get_boolean(inValue);
			break;

		default:
			G_OBJECT_WARN_INVALID_PROPERTY_ID(inObject, inPropID, inSpec);
			break;
	}
}

static void _xfdashboard_transition_group_get_property(GObject *inObject,
														guint inPropID,
														GValue *outValue,
														GParamSpec *inSpec)
{
	XfdashboardTransitionGroup			*self=XFDASHBOARD_TRANSITION_GROUP(inObject);
	XfdashboardTransitionGroupPrivate	*priv=self->priv;

	switch(inPropID)
	{
		case PROP_RESET_FLAGS:
			g_value_set_boolean(outValue, priv->resetFlags);
			break;

		default:
			G_OBJECT_WARN_INVALID_PROPERTY_ID(inObject, inPropID, inSpec);
			break;
	}
}

/* Class initialization
 * Override functions in parent classes and define properties
 * and signals
 */
void xfdashboard_transition_group_class_init(XfdashboardTransitionGroupClass *klass)
{
	GObjectClass			*gobjectClass=G_OBJECT_CLASS(klass);
	ClutterTimelineClass	*timelineClass=CLUTTER_TIMELINE_CLASS(klass);
	ClutterTransitionClass	*transitionClass=CLUTTER_TRANSITION_CLASS(klass);

	/* Override functions */
	gobjectClass->set_property=_xfdashboard_transition_group_set_property;
	gobjectClass->get_property=_xfdashboard_transition_group_get_property;
	gobjectClass->dispose=_xfdashboard_transition_group_dispose;
	gobjectClass->finalize=_xfdashboard_transition_group_finalize;

	timelineClass->started=_xfdashboard_transition_group_timeline_started;
	timelineClass->new_frame=_xfdashboard_transition_group_timeline_new_frame;

	transitionClass->attached=_xfdashboard_transition_group_transition_attached;
	transitionClass->detached=_xfdashboard_transition_group_transition_detached;

	/* Define properties */
	XfdashboardTransitionGroupProperties[PROP_RESET_FLAGS]=
		g_param_spec_boolean("reset-flags",
								"Reset flags",
								"If TRUE the flags for static sizes, transformations etc. at animatable actor is resetted to old state",
								TRUE,
								G_PARAM_READWRITE | G_PARAM_CONSTRUCT | G_PARAM_STATIC_STRINGS);

	g_object_class_install_properties(gobjectClass, PROP_LAST, XfdashboardTransitionGroupProperties);
}

/* Object initialization
 * Create private structure and set up default values
 */
void xfdashboard_transition_group_init(XfdashboardTransitionGroup *self)
{
	XfdashboardTransitionGroupPrivate		*priv;

	priv=self->priv=xfdashboard_transition_group_get_instance_private(self);

	/* Set up default values */
	priv->transitions=g_hash_table_new_full(NULL,
											NULL,
											(GDestroyNotify)g_object_unref,
											NULL);
}


/* IMPLEMENTATION: Public API */
/**
 * xfdashboard_transition_group_new:
 *
 * Creates a new #XfdashboardTransitionGroup instance.
 *
 * Return value: The newly created #XfdashboardTransitionGroup. Use
 *   g_object_unref() when done to deallocate the resources it uses
 */
ClutterTransition* xfdashboard_transition_group_new(void)
{
	return(CLUTTER_TRANSITION(g_object_new(XFDASHBOARD_TYPE_TRANSITION_GROUP, NULL)));
}

/**
 * xfdashboard_transition_group_add_transition:
 * @self: A #XfdashboardTransitionGroup
 * @inTransition: A #ClutterTransition
 *
 * Adds transition @inTransition to group @self.
 *
 * This function acquires a reference on @inTransition that will be released
 * when calling xfdashboard_transition_group_remove_transition() or
 * xfdashboard_transition_group_remove_all().
 */
void xfdashboard_transition_group_add_transition(XfdashboardTransitionGroup *self,
													ClutterTransition *inTransition)
{
	XfdashboardTransitionGroupPrivate			*priv;

	g_return_if_fail(XFDASHBOARD_IS_TRANSITION_GROUP(self));
	g_return_if_fail(CLUTTER_IS_TRANSITION(inTransition));

	priv=self->priv;

	/* Add and set up transition */
	g_hash_table_add(priv->transitions, g_object_ref(inTransition));
}

/**
 * xfdashboard_transition_group_remove_transition:
 * @self: A #XfdashboardTransitionGroup
 * @inTransition: A #ClutterTransition
 *
 * Removes transition @inTransition from group @self.
 *
 * This function releases the reference acquired on @inTransition when
 * calling xfdashboard_transition_group_add_transition().
 */
void xfdashboard_transition_group_remove_transition(XfdashboardTransitionGroup *self,
													ClutterTransition *inTransition)
{
	XfdashboardTransitionGroupPrivate	*priv;

	g_return_if_fail(XFDASHBOARD_IS_TRANSITION_GROUP(self));
	g_return_if_fail(CLUTTER_IS_TRANSITION(inTransition));

	priv=self->priv;

	/* Remove transition */
	g_hash_table_remove(priv->transitions, inTransition);
}

/**
 * xfdashboard_transition_group_remove_all:
 * @self: A #XfdashboardTransitionGroup
 *
 * Removes all transitions from group @self.
 *
 * This function releases all references acquired when calling
 * xfdashboard_transition_group_add_transition().
 */
void xfdashboard_transition_group_remove_all(XfdashboardTransitionGroup *self)
{
	XfdashboardTransitionGroupPrivate	*priv;

	g_return_if_fail(XFDASHBOARD_IS_TRANSITION_GROUP(self));

	priv=self->priv;

	/* Remove all transitions */
	g_hash_table_remove_all(priv->transitions);
}

/**
 * xfdashboard_transition_group_get_transitions:
 * @self: A #XfdashboardTransitionGroup
 *
 * Returns a list of all transitions of group @self. The list is a #GSList
 * and each element contains a #ClutterTransition. When creating the list
 * an additional reference is taken on the element, so the caller is responsible
 * to release the reference.
 *
 * Return value: (element-type XfdashboardWindowTrackerWindow) (transfer none):
 *   The list of #ClutterTransition added to this group.
 *   Use g_slist_free_full(list, g_object_unref) when done to release the
 *   additional reference taken.
 */
GSList* xfdashboard_transition_group_get_transitions(XfdashboardTransitionGroup *self)
{
	XfdashboardTransitionGroupPrivate	*priv;
	GSList								*list;
	GHashTableIter						iter;
	gpointer							element;

	g_return_val_if_fail(XFDASHBOARD_IS_TRANSITION_GROUP(self), NULL);

	priv=self->priv;
	list=NULL;

	/* Iterate through transitions and add to list but take a extra reference */
	g_hash_table_iter_init(&iter, priv->transitions);
	while(g_hash_table_iter_next(&iter, &element, NULL))
	{
		list=g_slist_prepend(list, g_object_ref(element));
	}
	list=g_slist_reverse(list);

	/* Return created list */
	return(list);
}
