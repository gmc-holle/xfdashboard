/*
 * theme-animation: A theme used for building animations by XML files
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

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <libxfdashboard/theme-animation.h>

#include <glib/gi18n-lib.h>
#include <glib.h>
#include <gio/gio.h>

#include <libxfdashboard/transition-group.h>
#include <libxfdashboard/stylable.h>
#include <libxfdashboard/application.h>
#include <libxfdashboard/enums.h>
#include <libxfdashboard/utils.h>
#include <libxfdashboard/compat.h>
#include <libxfdashboard/debug.h>


/* Define this class in GObject system */
struct _XfdashboardThemeAnimationPrivate
{
	/* Instance related */
	GSList			*specs;
};

G_DEFINE_TYPE_WITH_PRIVATE(XfdashboardThemeAnimation,
							xfdashboard_theme_animation,
							G_TYPE_OBJECT)


/* IMPLEMENTATION: Private variables and methods */
#define ENABLE_ANIMATIONS_XFCONF_PROP		"/enable-animations"
#define DEFAULT_ENABLE_ANIMATIONS			TRUE

enum
{
	XFDASHBOARD_THEME_ANIMATION_APPLY_TO_ORIGIN_SENDER=0,
	XFDASHBOARD_THEME_ANIMATION_APPLY_TO_ORIGIN_STAGE=1
};

typedef struct _XfdashboardThemeAnimationSpec				XfdashboardThemeAnimationSpec;
struct _XfdashboardThemeAnimationSpec
{
	gint					refCount;
	gchar					*id;
	XfdashboardCssSelector	*senderSelector;
	gchar					*signal;
	GSList					*targets;
};

typedef struct _XfdashboardThemeAnimationTargets			XfdashboardThemeAnimationTargets;
struct _XfdashboardThemeAnimationTargets
{
	gint					refCount;
	XfdashboardCssSelector	*targetSelector;
	guint					origin;
	ClutterTimeline			*timeline;
	GSList					*properties;
};

typedef struct _XfdashboardThemeAnimationTargetsProperty	XfdashboardThemeAnimationTargetsProperty;
struct _XfdashboardThemeAnimationTargetsProperty
{
	gint					refCount;
	gchar					*name;
	GValue					from;
	GValue					to;
};

/* Create, destroy, ref and unref animation targets data */
static XfdashboardThemeAnimationTargetsProperty* _xfdashboard_theme_animation_targets_property_new(const gchar *inPropertyName,
																									const gchar *inPropertyFrom,
																									const gchar *inPropertyTo)
{
	XfdashboardThemeAnimationTargetsProperty		*data;

	g_return_val_if_fail(inPropertyName && *inPropertyName, NULL);
	g_return_val_if_fail(!inPropertyFrom || *inPropertyFrom, NULL);
	g_return_val_if_fail(!inPropertyTo || *inPropertyTo, NULL);

	/* Create animation targets property data */
	data=g_new0(XfdashboardThemeAnimationTargetsProperty, 1);
	if(!data)
	{
		g_critical(_("Cannot allocate memory for animation targets property '%s'"), inPropertyName);
		return(NULL);
	}

	data->refCount=1;
	data->name=g_strdup(inPropertyName);

	if(inPropertyFrom)
	{
		g_value_init(&data->from, G_TYPE_STRING);
		g_value_set_string(&data->from, inPropertyFrom);
	}
	
	if(inPropertyTo)
	{
		g_value_init(&data->to, G_TYPE_STRING);
		g_value_set_string(&data->to, inPropertyTo);
	}

	return(data);
}

static void _xfdashboard_theme_animation_targets_property_free(XfdashboardThemeAnimationTargetsProperty *inData)
{
	g_return_if_fail(inData);

	/* Free property data */
	if(inData->name) g_free(inData->name);
	g_value_unset(&inData->from);
	g_value_unset(&inData->to);
	g_free(inData);
}

static XfdashboardThemeAnimationTargetsProperty* _xfdashboard_theme_animation_targets_property_ref(XfdashboardThemeAnimationTargetsProperty *inData)
{
	g_return_val_if_fail(inData, NULL);

	inData->refCount++;
	return(inData);
}

static void _xfdashboard_theme_animation_targets_property_unref(XfdashboardThemeAnimationTargetsProperty *inData)
{
	g_return_if_fail(inData);

	if(inData->refCount==1) _xfdashboard_theme_animation_targets_property_free(inData);
		else inData->refCount--;
}

/* Create, destroy, ref and unref animation targets data */
static XfdashboardThemeAnimationTargets* _xfdashboard_theme_animation_targets_new(XfdashboardCssSelector *inTargetSelector,
																					gint inOrigin,
																					ClutterTimeline *inTimelineSource)
{
	XfdashboardThemeAnimationTargets		*data;

	g_return_val_if_fail(!inTargetSelector || XFDASHBOARD_IS_CSS_SELECTOR(inTargetSelector), NULL);
	g_return_val_if_fail(inOrigin>=XFDASHBOARD_THEME_ANIMATION_APPLY_TO_ORIGIN_SENDER && inOrigin<=XFDASHBOARD_THEME_ANIMATION_APPLY_TO_ORIGIN_STAGE, NULL);
	g_return_val_if_fail(CLUTTER_IS_TIMELINE(inTimelineSource), NULL);

	/* Create animation targets data */
	data=g_new0(XfdashboardThemeAnimationTargets, 1);
	if(!data)
	{
		gchar							*selector;

		selector=xfdashboard_css_selector_to_string(inTargetSelector);
		g_critical(_("Cannot allocate memory for animation targets data with selector '%s'"), selector);
		g_free(selector);

		return(NULL);
	}

	data->refCount=1;
	data->targetSelector=(inTargetSelector ? g_object_ref(inTargetSelector) : NULL);
	data->origin=inOrigin;
	data->timeline=g_object_ref(inTimelineSource);
	data->properties=NULL;

	return(data);
}

static void _xfdashboard_theme_animation_targets_free(XfdashboardThemeAnimationTargets *inData)
{
	g_return_if_fail(inData);

#ifdef DEBUG
	if(inData->refCount>1)
	{
		g_critical(_("Freeing animation targets data at %p with a reference counter of %d greater than one"),
					inData,
					inData->refCount);
	}
#endif

	/* Release allocated resources */
	if(inData->targetSelector) g_object_unref(inData->targetSelector);
	if(inData->timeline) g_object_unref(inData->timeline);
	if(inData->properties) g_slist_free_full(inData->properties, (GDestroyNotify)_xfdashboard_theme_animation_targets_property_unref);
	g_free(inData);
}

static XfdashboardThemeAnimationTargets* _xfdashboard_theme_animation_targets_ref(XfdashboardThemeAnimationTargets *inData)
{
	g_return_val_if_fail(inData, NULL);

	inData->refCount++;
	return(inData);
}

static void _xfdashboard_theme_animation_targets_unref(XfdashboardThemeAnimationTargets *inData)
{
	g_return_if_fail(inData);

	if(inData->refCount==1) _xfdashboard_theme_animation_targets_free(inData);
		else inData->refCount--;
}

/* Add property name and from-to values to animation targets data */
static void _xfdashboard_theme_animation_targets_add_property(XfdashboardThemeAnimationTargets *inData,
																XfdashboardThemeAnimationTargetsProperty *inProperty)
{
	g_return_if_fail(inData);
	g_return_if_fail(inProperty);

	/* Add property to animation targets data */
	inData->properties=g_slist_prepend(inData->properties, _xfdashboard_theme_animation_targets_property_ref(inProperty));
}


/* Create, destroy, ref and unref animation specification data */
static XfdashboardThemeAnimationSpec* _xfdashboard_theme_animation_spec_new(const gchar *inID,
																			XfdashboardCssSelector *inSenderSelector,
																			const gchar *inSignal)
{
	XfdashboardThemeAnimationSpec		*data;

	g_return_val_if_fail(inID && *inID, NULL);
	g_return_val_if_fail(XFDASHBOARD_IS_CSS_SELECTOR(inSenderSelector), NULL);
	g_return_val_if_fail(inSignal && *inSignal, NULL);

	/* Create animation specification data */
	data=g_new0(XfdashboardThemeAnimationSpec, 1);
	if(!data)
	{
		gchar							*selector;

		selector=xfdashboard_css_selector_to_string(inSenderSelector);
		g_critical(_("Cannot allocate memory for animation specification data with sender '%s' and signal '%s'"),
					selector,
					inSignal);
		g_free(selector);

		return(NULL);
	}

	data->refCount=1;
	data->id=g_strdup(inID);
	data->senderSelector=g_object_ref(inSenderSelector);
	data->signal=g_strdup(inSignal);
	data->targets=NULL;

	return(data);
}

static void _xfdashboard_theme_animation_spec_free(XfdashboardThemeAnimationSpec *inData)
{
	g_return_if_fail(inData);

#ifdef DEBUG
	if(inData->refCount>1)
	{
		g_critical(_("Freeing animation specification data at %p with a reference counter of %d greater than one"),
					inData,
					inData->refCount);
	}
#endif

	/* Release allocated resources */
	if(inData->id) g_free(inData->id);
	if(inData->senderSelector) g_object_unref(inData->senderSelector);
	if(inData->signal) g_free(inData->signal);
	if(inData->targets) g_slist_free_full(inData->targets, (GDestroyNotify)_xfdashboard_theme_animation_targets_unref);
	g_free(inData);
}

static XfdashboardThemeAnimationSpec* _xfdashboard_theme_animation_spec_ref(XfdashboardThemeAnimationSpec *inData)
{
	g_return_val_if_fail(inData, NULL);

	inData->refCount++;
	return(inData);
}

static void _xfdashboard_theme_animation_spec_unref(XfdashboardThemeAnimationSpec *inData)
{
	g_return_if_fail(inData);

	if(inData->refCount==1) _xfdashboard_theme_animation_spec_free(inData);
		else inData->refCount--;
}

/* Add a animation target to animation specification */
static void _xfdashboard_theme_animation_spec_add_targets(XfdashboardThemeAnimationSpec *inData,
															XfdashboardThemeAnimationTargets *inTargets)
{
	g_return_if_fail(inData);
	g_return_if_fail(inTargets);

	/* Add target to specification */
	inData->targets=g_slist_prepend(inData->targets, _xfdashboard_theme_animation_targets_ref(inTargets));
}

/* Find best matching animation for sender and signal */
static XfdashboardThemeAnimationSpec* _xfdashboard_theme_animation_find_animation_spec(XfdashboardThemeAnimation *self,
																						XfdashboardStylable *inSender,
																						const gchar *inSignal)
{
	XfdashboardThemeAnimationPrivate			*priv;
	GSList										*iter;
	gint										score;
	XfdashboardThemeAnimationSpec				*spec;
	gint										bestScore;
	XfdashboardThemeAnimationSpec				*bestAnimation;

	g_return_val_if_fail(XFDASHBOARD_IS_THEME_ANIMATION(self), NULL);
	g_return_val_if_fail(XFDASHBOARD_IS_STYLABLE(inSender), NULL);
	g_return_val_if_fail(inSignal && *inSignal, NULL);

	priv=self->priv;
	bestScore=0;
	bestAnimation=NULL;

	/* Iterate through all animation specification and get its score against sender.
	 * If the iterated specification gets a higher score than the previous one,
	 * remember this specification for return value.
	 */
	for(iter=priv->specs; iter; iter=g_slist_next(iter))
	{
		/* Get currently iterated specification */
		spec=(XfdashboardThemeAnimationSpec*)iter->data;
		if(!spec) continue;

		/* Skip animation specification if its signal definition does not match
		 * the emitted signal.
		 */
		if(g_strcmp0(spec->signal, inSignal)!=0) continue;

		/* Get score of currently iterated specification against sender.
		 * Skip this iterated specification if score is zero or lower.
		 */
		score=xfdashboard_css_selector_score(spec->senderSelector, inSender);
		if(score<=0) continue;

		/* If score is higher than the previous best matching one, then release
		 * old specificationr remembered and ref new one.
		 */
		if(score>bestScore)
		{
			/* Release old remembered specification */
			if(bestAnimation) _xfdashboard_theme_animation_spec_unref(bestAnimation);

			/* Remember new one and take a reference */
			bestScore=score;
			bestAnimation=_xfdashboard_theme_animation_spec_ref(spec);
		}
	}

	/* Return best matching animation specification found */
	return(bestAnimation);
}

/* Find actors matching an animation target data */
static gboolean _xfdashboard_theme_animation_find_actors_for_animation_targets_traverse_callback(ClutterActor *inActor, gpointer inUserData)
{
	GSList		**actors=(GSList**)inUserData;

	*actors=g_slist_prepend(*actors, inActor);

	return(XFDASHBOARD_TRAVERSAL_CONTINUE);
}

static GSList* _xfdashboard_theme_animation_find_actors_for_animation_targets(XfdashboardThemeAnimation *self,
																				XfdashboardThemeAnimationTargets *inTargetSpec,
																				ClutterActor *inSender)
{
	ClutterActor								*rootNode;
	GSList										*actors;

	g_return_val_if_fail(XFDASHBOARD_IS_THEME_ANIMATION(self), NULL);
	g_return_val_if_fail(inTargetSpec, NULL);
	g_return_val_if_fail(CLUTTER_IS_ACTOR(inSender), NULL);


	/* Depending on origin at animation target data select root node to start
	 * traversal and collecting matching actors.
	 */
	rootNode=NULL;
	if(inTargetSpec->origin==XFDASHBOARD_THEME_ANIMATION_APPLY_TO_ORIGIN_SENDER)
	{
		rootNode=inSender;
	}

	/* Traverse through actors beginning at the root node and collect each actor
	 * matching the target selector but only if a target selector is set. If
	 * target selector is NULL then set up a single-item list containing only
	 * the sender actor as list of actors found.
	 */
	actors=NULL;
	if(inTargetSpec->targetSelector)
	{
		xfdashboard_traverse_actor(rootNode,
									inTargetSpec->targetSelector,
									_xfdashboard_theme_animation_find_actors_for_animation_targets_traverse_callback,
									&actors);
	}
		else
		{
			actors=g_slist_prepend(actors, inSender);
		}

	/* Correct order in list */
	actors=g_slist_reverse(actors);

	/* Return list of actors found */
	return(actors);
}


/* IMPLEMENTATION: GObject */

/* Dispose this object */
static void _xfdashboard_theme_animation_dispose(GObject *inObject)
{
	XfdashboardThemeAnimation				*self=XFDASHBOARD_THEME_ANIMATION(inObject);
	XfdashboardThemeAnimationPrivate		*priv=self->priv;

	/* Release allocated resources */
	if(priv->specs)
	{
		g_slist_free_full(priv->specs, (GDestroyNotify)_xfdashboard_theme_animation_spec_unref);
		priv->specs=NULL;
	}

	/* Call parent's class dispose method */
	G_OBJECT_CLASS(xfdashboard_theme_animation_parent_class)->dispose(inObject);
}

/* Class initialization
 * Override functions in parent classes and define properties
 * and signals
 */
void xfdashboard_theme_animation_class_init(XfdashboardThemeAnimationClass *klass)
{
	GObjectClass		*gobjectClass=G_OBJECT_CLASS(klass);

	/* Override functions */
	gobjectClass->dispose=_xfdashboard_theme_animation_dispose;
}

/* Object initialization
 * Create private structure and set up default values
 */
void xfdashboard_theme_animation_init(XfdashboardThemeAnimation *self)
{
	XfdashboardThemeAnimationPrivate		*priv;

	priv=self->priv=xfdashboard_theme_animation_get_instance_private(self);

	/* Set default values */
	priv->specs=NULL;
}

/* IMPLEMENTATION: Errors */

GQuark xfdashboard_theme_animation_error_quark(void)
{
	return(g_quark_from_static_string("xfdashboard-theme-animation-error-quark"));
}

/* IMPLEMENTATION: Public API */

/* Create new instance */
XfdashboardThemeAnimation* xfdashboard_theme_animation_new(void)
{
	return(XFDASHBOARD_THEME_ANIMATION(g_object_new(XFDASHBOARD_TYPE_THEME_ANIMATION, NULL)));
}


/* Load a XML file into theme */
gboolean xfdashboard_theme_animation_add_file(XfdashboardThemeAnimation *self,
												const gchar *inPath,
												GError **outError)
{
	XfdashboardThemeAnimationPrivate			*priv;
	XfdashboardThemeAnimationSpec				*spec;
	gchar										*id;
	gchar										*sender;
	gchar										*signal;
	XfdashboardCssSelector						*senderSelector;
	ClutterTimeline								*timeline;
	gint										timelineDelay;
	gint										timelineDuration;
	guint										timelineMode;
	gint										timelineRepeat;
	XfdashboardThemeAnimationTargets			*targets;
	XfdashboardCssSelector						*targetsSelector;
	gint										targetsOrigin;
	XfdashboardThemeAnimationTargetsProperty	*property;
	gchar										*propertyName;
	gchar										*propertyFrom;
	gchar										*propertyTo;

	g_return_val_if_fail(XFDASHBOARD_IS_THEME_ANIMATION(self), FALSE);
	g_return_val_if_fail(inPath!=NULL && *inPath!=0, FALSE);
	g_return_val_if_fail(outError==NULL || *outError==NULL, FALSE);

	priv=self->priv;

	/* TODO: Replace with real loading code ;) */

	/* <trigger id="expand-workspace" sender="#workspace-selector-collapse-box" signal="expand"> */
	id="expand-workspace";
	sender="XfdashboardCollapseBox";
	signal="expand";

	senderSelector=xfdashboard_css_selector_new_from_string(sender);
	spec=_xfdashboard_theme_animation_spec_new(id, senderSelector, signal);
	g_object_unref(senderSelector);

	/*   <timeline delay="0" duration="500" mode=ease-out-cubic" repeat="0" (optional, default=0) > */
	timelineDelay=0;
	timelineDuration=500;
	timelineMode=CLUTTER_EASE_OUT_CUBIC;
	timelineRepeat=0;

	timeline=clutter_timeline_new(timelineDuration);
	clutter_timeline_set_delay(timeline, timelineDelay);
	clutter_timeline_set_progress_mode(timeline, timelineMode);
	clutter_timeline_set_repeat_count(timeline, timelineRepeat);

	/*     <apply to="" (optional, default="" -> sender) origin="sender" (optional, default="sender") > */
	targetsSelector=NULL;
	targetsOrigin=XFDASHBOARD_THEME_ANIMATION_APPLY_TO_ORIGIN_SENDER;

	targets=_xfdashboard_theme_animation_targets_new(targetsSelector, targetsOrigin, timeline);

	/*       <property name="collapse-fraction" from="" (optional, default="") to="" (optional, default="")> */
	propertyName="width";
	propertyFrom=NULL;
	propertyTo=NULL;

	property=_xfdashboard_theme_animation_targets_property_new(propertyName, propertyFrom, propertyTo);

	/*       </property> */
	_xfdashboard_theme_animation_targets_add_property(targets, property);
	_xfdashboard_theme_animation_targets_property_unref(property);
	property=NULL;

	/*     </apply> */
	_xfdashboard_theme_animation_spec_add_targets(spec, targets);
	_xfdashboard_theme_animation_targets_unref(targets);
	targets=NULL;

	/*   </timeline> */
	g_object_unref(timeline);
	timeline=NULL;

	/* </trigger> */
	priv->specs=g_slist_prepend(priv->specs, spec);



	/* <trigger id="collapse-workspace" sender="#workspace-selector-collapse-box" signal="collapse"> */
	id="collapse-workspace";
	sender="XfdashboardCollapseBox";
	signal="collapse";

	senderSelector=xfdashboard_css_selector_new_from_string(sender);
	spec=_xfdashboard_theme_animation_spec_new(id, senderSelector, signal);
	g_object_unref(senderSelector);

	/*   <timeline delay="0" duration="500" mode=ease-in-cubic" repeat="0" (optional, default=0) > */
	timelineDelay=0;
	timelineDuration=500;
	timelineMode=CLUTTER_EASE_OUT_CUBIC;
	timelineRepeat=0;

	timeline=clutter_timeline_new(timelineDuration);
	clutter_timeline_set_delay(timeline, timelineDelay);
	clutter_timeline_set_progress_mode(timeline, timelineMode);
	clutter_timeline_set_repeat_count(timeline, timelineRepeat);

	/*     <apply to="" (optional, default="" -> sender) origin="sender" (optional, default="sender") > */
	targetsSelector=NULL;
	targetsOrigin=XFDASHBOARD_THEME_ANIMATION_APPLY_TO_ORIGIN_SENDER;

	targets=_xfdashboard_theme_animation_targets_new(targetsSelector, targetsOrigin, timeline);

	/*       <property name="collapse-fraction" from="" (optional, default="") to="" (optional, default="")> */
	propertyName="width";
	propertyFrom=NULL;
	propertyTo=NULL;

	property=_xfdashboard_theme_animation_targets_property_new(propertyName, propertyFrom, propertyTo);

	/*       </property> */
	_xfdashboard_theme_animation_targets_add_property(targets, property);
	_xfdashboard_theme_animation_targets_property_unref(property);
	property=NULL;

	/*     </apply> */
	_xfdashboard_theme_animation_spec_add_targets(spec, targets);
	_xfdashboard_theme_animation_targets_unref(targets);
	targets=NULL;

	/*   </timeline> */
	g_object_unref(timeline);
	timeline=NULL;

	/* </trigger> */
	priv->specs=g_slist_prepend(priv->specs, spec);


	/* TODO: Just for testing animations */
	g_message("Loading animations done!");

	/* If we get here loading and parsing XML file was successful
	 * so return TRUE here
	 */
	return(TRUE);
}

/* Build requested animation */
XfdashboardAnimation* xfdashboard_theme_animation_create(XfdashboardThemeAnimation *self,
															XfdashboardActor *inSender,
															const gchar *inSignal)
{
	XfdashboardThemeAnimationSpec							*spec;
	XfdashboardAnimation									*animation;
	XfdashboardAnimationClass								*animationClass;
	GSList													*iterTargets;
	gint													counterTargets;
	gboolean												animationEnabled;
#ifdef DEBUG
	gboolean												doDebug=TRUE;
#endif

	g_return_val_if_fail(XFDASHBOARD_IS_THEME_ANIMATION(self), NULL);
	g_return_val_if_fail(XFDASHBOARD_IS_ACTOR(inSender), NULL);
	g_return_val_if_fail(inSignal && *inSignal, NULL);

	animation=NULL;

	/* Create empty animation */
	animation=g_object_new(XFDASHBOARD_TYPE_ANIMATION,
							NULL);
	if(!animation)
	{
		g_critical(_("Cannot allocate memory for animation of sender '%s' and signal '%s'"),
					G_OBJECT_TYPE_NAME(inSender),
					inSignal);
		return(NULL);
	}

	/* Check if user wants animation at all. If user does not want any animation,
	 * return the empty one.
	 */
	animationEnabled=xfconf_channel_get_bool(xfdashboard_application_get_xfconf_channel(NULL),
												ENABLE_ANIMATIONS_XFCONF_PROP,
												DEFAULT_ENABLE_ANIMATIONS);
	if(!animationEnabled)
	{
		XFDASHBOARD_DEBUG(self, ANIMATION, "User disabled animations");

		/* Return NULL as user does not want any animation */
		return(animation);
	}

	/* Get best matching animation specification for sender and signal.
	 * If no matching animation specification is found, return the empty one.
	 */
	spec=_xfdashboard_theme_animation_find_animation_spec(self, XFDASHBOARD_STYLABLE(inSender), inSignal);
	if(!spec)
	{
		XFDASHBOARD_DEBUG(self, ANIMATION,
							"Could not find an animation specification for sender '%s' and signal '%s'",
							G_OBJECT_TYPE_NAME(inSender),
							inSignal);

		/* Return NULL as no matching animation specification was found */
		return(animation);
	}

	/* Create new animation for animation specification */
	g_object_unref(animation);

	animation=g_object_new(XFDASHBOARD_TYPE_ANIMATION,
							"id", spec->id,
							NULL);
	if(!animation)
	{
		g_critical(_("Cannot allocate memory for animation of sender '%s' and signal '%s'"),
					G_OBJECT_TYPE_NAME(inSender),
					inSignal);
		return(NULL);
	}

	animationClass=XFDASHBOARD_ANIMATION_GET_CLASS(animation);
	if(!animationClass->add_animation)
	{
		g_warning(_("Will not be able to add animations to actors as object of type %s does not implement required virtual function XfdashboardAnimation::%s"), \
					G_OBJECT_TYPE_NAME(self), \
					"add_animation");
	}

	XFDASHBOARD_DEBUG(self, ANIMATION,
						"Found animation specification '%s' for sender '%s' and signal '%s' with %d targets",
						spec->id,
						G_OBJECT_TYPE_NAME(inSender),
						inSignal,
						g_slist_length(spec->targets));

	/* Iterate through animation targets of animation specification and create
	 * property transition for each target and property found.
	 */
	for(counterTargets=0, iterTargets=spec->targets; iterTargets; iterTargets=g_slist_next(iterTargets), counterTargets++)
	{
		XfdashboardThemeAnimationTargets					*targets;
		GSList												*actors;
		GSList												*iterActors;
		gint												counterActors;

		/* Get currently iterate animation targets */
		targets=(XfdashboardThemeAnimationTargets*)iterTargets->data;
		if(!targets) continue;

		/* Find targets to apply property transitions to */
		actors=_xfdashboard_theme_animation_find_actors_for_animation_targets(self, targets, CLUTTER_ACTOR(inSender));
		if(!actors) continue;
		XFDASHBOARD_DEBUG(self, ANIMATION,
							"Target #%d of animation specification '%s' applies to %d actors",
							counterTargets,
							spec->id,
							g_slist_length(actors));

		/* Iterate through actor, create a transition group to collect
		 * property transitions at, create a property transition for each
		 * property specified in animation target, determine "from" value
		 * where missing, add property transition to transition group and
		 * finally add transition group with currently iterated actor to
		 * animation object.
		 */
		for(counterActors=0, iterActors=actors; iterActors; iterActors=g_slist_next(iterActors), counterActors++)
		{
			GSList											*iterProperties;
			ClutterActor									*actor;
			ClutterTransition								*transitionGroup;
			int												counterProperties;

			/* Get actor */
			actor=(ClutterActor*)iterActors->data;
			if(!actor) continue;

			/* Create transition group to collect property transitions at */
			transitionGroup=xfdashboard_transition_group_new();
			if(!transitionGroup)
			{
				g_critical(_("Cannot allocate memory for transition group of animation specification '%s'"),
							spec->id);
				
				/* Release allocated resources */
				g_slist_free(actors);
				g_object_unref(animation);

				return(NULL);
			}

			/* Clone timeline configuration from animation target */
			clutter_timeline_set_duration(CLUTTER_TIMELINE(transitionGroup), clutter_timeline_get_duration(targets->timeline));
			clutter_timeline_set_delay(CLUTTER_TIMELINE(transitionGroup), clutter_timeline_get_delay(targets->timeline));
			clutter_timeline_set_progress_mode(CLUTTER_TIMELINE(transitionGroup), clutter_timeline_get_progress_mode(targets->timeline));
			clutter_timeline_set_repeat_count(CLUTTER_TIMELINE(transitionGroup), clutter_timeline_get_repeat_count(targets->timeline));

			XFDASHBOARD_DEBUG(self, ANIMATION,
								"Created transition group at %p for %d properties for target #%d and actor #%d (%s@%p) of animation specification '%s'",
								transitionGroup,
								g_slist_length(targets->properties),
								counterTargets,
								counterActors,
								G_OBJECT_TYPE_NAME(actor),
								actor,
								spec->id);

			/* Iterate through properties and create a property transition
			 * with cloned timeline from animation target. Determine "from"
			 * value if missing in animation targets property specification
			 * and add the property transition to transition group.
			 */
			for(counterProperties=0, iterProperties=targets->properties; iterProperties; iterProperties=g_slist_next(iterProperties), counterProperties++)
			{
				XfdashboardThemeAnimationTargetsProperty	*propertyTargetSpec;
				GParamSpec									*propertySpec;
				GValue										fromValue=G_VALUE_INIT;
				GValue										toValue=G_VALUE_INIT;

				/* Get target's property data to animate */
				propertyTargetSpec=(XfdashboardThemeAnimationTargetsProperty*)iterProperties->data;
				if(!propertyTargetSpec) continue;

				/* Check if actor has property to animate */
				propertySpec=g_object_class_find_property(G_OBJECT_GET_CLASS(actor), propertyTargetSpec->name);
				if(!propertySpec)
				{
					g_warning(_("Cannot create animation for non-existing property '%s' at actor of type '%s'"),
								propertyTargetSpec->name,
								G_OBJECT_TYPE_NAME(actor));

					/* Skip this property as it does not exist at actor */
					continue;
				}

				/* If no "from" value is set at target's property data, get
				 * current value. Otherwise convert "from" value from target's
				 * property data to expected type of property.
				 */
				if(G_VALUE_TYPE(&propertyTargetSpec->from)!=G_TYPE_INVALID)
				{
					g_value_init(&fromValue, G_PARAM_SPEC_VALUE_TYPE(propertySpec));
					if(!g_value_transform(&propertyTargetSpec->from, &fromValue))
					{
						g_warning(_("Could not transform 'from'-value of '%s' for property '%s' to type %s of class %s"),
									g_value_get_string(&propertyTargetSpec->from),
									propertyTargetSpec->name,
									g_type_name(G_PARAM_SPEC_VALUE_TYPE(propertySpec)),
									G_OBJECT_CLASS_NAME(G_OBJECT_GET_CLASS(actor)));

						/* Unset "from" value to skip it, means it will no transition will be
						 * create and it will not be added to transition group.
						 */
						g_value_unset(&fromValue);
					}
#ifdef DEBUG
					if(doDebug)
					{
						gchar	*valueString;

						valueString=g_strdup_value_contents(&propertyTargetSpec->from);
						XFDASHBOARD_DEBUG(self, ANIMATION,
											"%s 'from'-value %s of type %s for property '%s' to type %s of class %s for target #%d and actor #%d (%s@%p) of animation specification '%s'",
											(G_VALUE_TYPE(&fromValue)!=G_TYPE_INVALID) ? "Converted" : "Could not convert",
											valueString,
											G_VALUE_TYPE_NAME(&propertyTargetSpec->from),
											propertyTargetSpec->name,
											g_type_name(G_PARAM_SPEC_VALUE_TYPE(propertySpec)),
											G_OBJECT_CLASS_NAME(G_OBJECT_GET_CLASS(actor)),
											counterTargets,
											counterActors,
											G_OBJECT_TYPE_NAME(actor),
											actor,
											spec->id);
						g_free(valueString);
					}
#endif
				}
					else
					{
						g_value_init(&fromValue, G_PARAM_SPEC_VALUE_TYPE(propertySpec));
						g_object_get_property(G_OBJECT(actor),
												propertyTargetSpec->name,
												&fromValue);
#ifdef DEBUG
						if(doDebug)
						{
							gchar	*valueString;

							valueString=g_strdup_value_contents(&fromValue);
							XFDASHBOARD_DEBUG(self, ANIMATION,
												"Fetching current 'from'-value %s for property '%s' from target #%d and actor #%d (%s@%p) of animation specification '%s' as no 'from' value was specified",
												valueString,
												propertyTargetSpec->name,
												counterTargets,
												counterActors,
												G_OBJECT_TYPE_NAME(actor),
												actor,
												spec->id);
							g_free(valueString);
						}
#endif
					}

				/* If "to" value is set at target's property data, convert it
				 * from target's property data to expected type of property.
				 */
				if(G_VALUE_TYPE(&propertyTargetSpec->to)!=G_TYPE_INVALID)
				{
					g_value_init(&toValue, G_PARAM_SPEC_VALUE_TYPE(propertySpec));
					if(!g_value_transform(&propertyTargetSpec->to, &toValue))
					{
						g_warning(_("Could not transform 'to'-value of '%s' for property '%s' to type %s of class %s"),
									g_value_get_string(&propertyTargetSpec->to),
									propertyTargetSpec->name,
									g_type_name(G_PARAM_SPEC_VALUE_TYPE(propertySpec)),
									G_OBJECT_CLASS_NAME(G_OBJECT_GET_CLASS(actor)));

						/* Unset "to" value to prevent setting it at transition.
						 * The animation will set a value when starting the
						 * animation.
						 */
						g_value_unset(&toValue);
					}
#ifdef DEBUG
					if(doDebug)
					{
						gchar	*valueString;

						valueString=g_strdup_value_contents(&propertyTargetSpec->to);
						XFDASHBOARD_DEBUG(self, ANIMATION,
											"%s 'to'-value %s of type %s for property '%s' to type %s of class %s for target #%d and actor #%d (%s@%p) of animation specification '%s'",
											(G_VALUE_TYPE(&toValue)!=G_TYPE_INVALID) ? "Converted" : "Could not convert",
											valueString,
											G_VALUE_TYPE_NAME(&propertyTargetSpec->to),
											propertyTargetSpec->name,
											g_type_name(G_PARAM_SPEC_VALUE_TYPE(propertySpec)),
											G_OBJECT_CLASS_NAME(G_OBJECT_GET_CLASS(actor)),
											counterTargets,
											counterActors,
											G_OBJECT_TYPE_NAME(actor),
											actor,
											spec->id);
						g_free(valueString);
					}
#endif
				}

				/* Create property transition for property with cloned timeline
				 * and add this new transition to transition group if from value
				 * is not invalid.
				 */
				if(G_VALUE_TYPE(&fromValue)!=G_TYPE_INVALID)
				{
					ClutterTransition						*propertyTransition;

					/* Create property transition */
					propertyTransition=clutter_property_transition_new(propertyTargetSpec->name);
					if(!propertyTransition)
					{
						g_critical(_("Cannot allocate memory for transition of property '%s' of animation specification '%s'"),
									propertyTargetSpec->name,
									spec->id);

						/* Release allocated resources */
						g_value_unset(&fromValue);
						g_value_unset(&toValue);

						/* Skip this property transition */
						continue;
					}

					/* Set "from" value */
					clutter_transition_set_from_value(propertyTransition, &fromValue);

					/* Set "to" value if valid */
					if(G_VALUE_TYPE(&toValue)!=G_TYPE_INVALID)
					{
						clutter_transition_set_to_value(propertyTransition, &toValue);
					}

					/* Add property transition to transition group */
					xfdashboard_transition_group_add_transition(XFDASHBOARD_TRANSITION_GROUP(transitionGroup), propertyTransition);

					/* Release allocated resources */
					g_object_unref(propertyTransition);
					XFDASHBOARD_DEBUG(self, ANIMATION,
										"Created transition for property '%s' at target #%d and actor #%d (%s@%p) of animation specification '%s'",
										propertyTargetSpec->name,
										counterTargets,
										counterActors,
										G_OBJECT_TYPE_NAME(actor),
										actor,
										spec->id);
				}

				/* Release allocated resources */
				g_value_unset(&fromValue);
				g_value_unset(&toValue);
			}

			/* Add transition group with collected property transitions
			 * to actor.
			 */
			if(animationClass->add_animation)
			{
				animationClass->add_animation(animation, actor, transitionGroup);
			}

			/* Release allocated resources */
			g_object_unref(transitionGroup);
		}

		/* Release allocated resources */
		g_slist_free(actors);
	}

	/* Release allocated resources */
	if(spec) _xfdashboard_theme_animation_spec_unref(spec);

	return(animation);
}
