/*
 * actor: Abstract base actor
 * 
 * Copyright 2012-2020 Stephan Haller <nomad@froevel.de>
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

#include <libxfdashboard/actor.h>

#include <glib/gi18n-lib.h>

#include <libxfdashboard/application.h>
#include <libxfdashboard/stylable.h>
#include <libxfdashboard/focusable.h>
#include <libxfdashboard/animation.h>
#include <libxfdashboard/utils.h>
#include <libxfdashboard/compat.h>
#include <libxfdashboard/debug.h>
#include <math.h>


/* Define this class in GObject system */
static gpointer				xfdashboard_actor_parent_class=NULL;
static gint					xfdashboard_actor_private_offset=0;

static gpointer xfdashboard_actor_get_instance_private(XfdashboardActor *self);
void xfdashboard_actor_class_init(XfdashboardActorClass *klass);
void xfdashboard_actor_init(XfdashboardActor *self);
void xfdashboard_actor_base_class_finalize(XfdashboardActorClass *klass);

struct _XfdashboardActorPrivate
{
	/* Properties related */
	gboolean						canFocus;
	gchar							*effects;

	gchar							*styleClasses;
	gchar							*stylePseudoClasses;

	/* Instance related */
	gboolean						inDestruction;

	GHashTable						*lastThemeStyleSet;
	gboolean						forceStyleRevalidation;

	gboolean						isFirstParent;

	gboolean						firstTimeMapped;
	XfdashboardAnimation			*firstTimeMappedAnimation;

	GSList							*animations;

	ClutterActorBox					*allocationTrackBox;

	gboolean						disallowAllocationAnimation;
	gboolean						doAllocationAnimation;
	XfdashboardAnimation			*allocationAnimation;
	ClutterActorBox					*allocationInitialBox;
	ClutterActorBox					*allocationFinalBox;
};

/* Properties */
enum
{
	PROP_0,

	PROP_CAN_FOCUS,
	PROP_EFFECTS,

	/* Overriden properties of interface: XfdashboardStylable */
	PROP_STYLE_CLASSES,
	PROP_STYLE_PSEUDO_CLASSES,

	PROP_LAST
};

static GParamSpec* XfdashboardActorProperties[PROP_LAST]={ 0, };

/* IMPLEMENTATION: Private variables and methods */
#define ALLOCATION_ANIMATION_SIGNAL		"move-resize"

typedef struct _XfdashboardActorAnimationEntry		XfdashboardActorAnimationEntry;
struct _XfdashboardActorAnimationEntry
{
	gboolean									inDestruction;
	gchar										*signal;
	XfdashboardAnimation						*animation;
};

#define XFDASHBOARD_ACTOR_PARAM_SPEC_REF		(_xfdashboard_actor_param_spec_ref_quark())

static GParamSpecPool		*_xfdashboard_actor_stylable_properties_pool=NULL;


/* Quark declarations */
static GQuark _xfdashboard_actor_param_spec_ref_quark(void)
{
	return(g_quark_from_static_string("xfdashboard-actor-param-spec-ref-quark"));
}


/* Free an animation entry */
static void _xfdashboard_actor_animation_entry_free(XfdashboardActorAnimationEntry *inData)
{
	g_return_if_fail(inData);

	/* Do not free anything if this entry is already in destruction */
	if(inData->inDestruction) return;

	/* Set flag that this data will be freed now as this function could
	 * be called recursive (e.g. by other signal handlers) resulting
	 * in double-free.
	 */
	inData->inDestruction=TRUE;

	/* Release allocated resources */
	if(inData->animation) g_object_unref(inData->animation);
	if(inData->signal) g_free(inData->signal);
	g_free(inData);
}

/* Invalidate all stylable children recursively beginning at given actor */
static void _xfdashboard_actor_invalidate_recursive(ClutterActor *inActor)
{
	ClutterActor			*child;
	ClutterActorIter		actorIter;

	g_return_if_fail(CLUTTER_IS_ACTOR(inActor));

	/* If actor is stylable invalidate it to get its style recomputed */
	if(XFDASHBOARD_IS_STYLABLE(inActor))
	{
		xfdashboard_stylable_invalidate(XFDASHBOARD_STYLABLE(inActor));
	}

	/* Recompute styles for all children recursively */
	clutter_actor_iter_init(&actorIter, inActor);
	while(clutter_actor_iter_next(&actorIter, &child))
	{
		/* Call ourselve recursive with child as top-level actor.
		 * We return immediately if it has no children but invalidate child
		 * before. If it has children it will first invalidated and will be
		 * iterated over its children then. In both cases the child will
		 * be invalidated.
		 */
		_xfdashboard_actor_invalidate_recursive(child);
	}
}

/* Get parameter specification of stylable properties and add them to hashtable.
 * If requested do it recursively over all parent classes.
 */
static void _xfdashboard_actor_hashtable_get_all_stylable_param_specs(GHashTable *ioHashtable, GObjectClass *inClass, gboolean inRecursive)
{
	GList						*paramSpecs, *entry;
	GObjectClass				*parentClass;

	/* Get list of all parameter specification registered for class */
	paramSpecs=g_param_spec_pool_list_owned(_xfdashboard_actor_stylable_properties_pool, G_OBJECT_CLASS_TYPE(inClass));
	for(entry=paramSpecs; entry; entry=g_list_next(entry))
	{
		GParamSpec				*paramSpec=G_PARAM_SPEC(entry->data);

		/* Only add parameter specification which aren't already in hashtable */
		if(paramSpec &&
			!g_hash_table_lookup_extended(ioHashtable, g_param_spec_get_name(paramSpec), NULL, NULL))
		{
			g_hash_table_insert(ioHashtable,
									g_strdup(g_param_spec_get_name(paramSpec)),
									g_param_spec_ref(paramSpec));
		}
	}
	g_list_free(paramSpecs);

	/* Call us recursive for parent class if it exists and requested */
	parentClass=g_type_class_peek_parent(inClass);
	if(inRecursive && parentClass) _xfdashboard_actor_hashtable_get_all_stylable_param_specs(ioHashtable, parentClass, inRecursive);
}

/* Remove entries from hashtable whose key is a duplicate
 * in other hashtable (given in user data)
 */
static gboolean _xfdashboard_actor_hashtable_is_duplicate_key(gpointer inKey,
																gpointer inValue,
																gpointer inUserData)
{
	GHashTable			*otherHashtable=(GHashTable*)inUserData;

	return(g_hash_table_lookup_extended(otherHashtable, inKey, NULL, NULL));
}

/* 'created' animation has completed */
static void _xfdashboard_actor_first_time_created_animation_done(XfdashboardAnimation *inAnimation,
																	gpointer inUserData)
{
	XfdashboardActor				*self;
	XfdashboardActorPrivate			*priv;

	g_return_if_fail(XFDASHBOARD_IS_ANIMATION(inAnimation));
	g_return_if_fail(XFDASHBOARD_IS_ACTOR(inUserData));

	self=XFDASHBOARD_ACTOR(inUserData);
	priv=self->priv;

	/* Mark completed first-time animation as removed */
	priv->firstTimeMappedAnimation=NULL;

	/* Allow allocation animation again */
	priv->disallowAllocationAnimation=FALSE;
}

/* Actor was mapped or unmapped */
static void _xfdashboard_actor_on_mapped_changed(GObject *inObject,
													GParamSpec *inSpec,
													gpointer inUserData)
{
	XfdashboardActor					*self;
	XfdashboardActorPrivate				*priv;

	g_return_if_fail(XFDASHBOARD_IS_ACTOR(inObject));

	self=XFDASHBOARD_ACTOR(inObject);
	priv=self->priv;

	/* If actor was mapped, invalidate styling and check for first-time animation */
	if(clutter_actor_is_mapped(CLUTTER_ACTOR(self)))
	{
		/* Invalide styling to get it recomputed if actor was mapped */
		xfdashboard_stylable_invalidate(XFDASHBOARD_STYLABLE(self));

		/* If actor was mapped for the first time then check if an animation
		 * should be created and run.
		 */
		if(!priv->firstTimeMapped)
		{
			XfdashboardAnimationValue	**initials;
			XfdashboardAnimationValue	**finals;
			ClutterActorBox				finalBox={ 0, };

			g_assert(!priv->firstTimeMappedAnimation);

			/* Set flag that first-time visible happened at this actor */
			priv->firstTimeMapped=TRUE;

			/* If an animation for "created" signal exists, then set up animation */
			if(xfdashboard_animation_has_animation(self, "created"))
			{
				/* Set up default initial values for animation */
				initials=xfdashboard_animation_defaults_new(4,
															"x", G_TYPE_FLOAT, 0.0f,
															"y", G_TYPE_FLOAT, 0.0f,
															"width", G_TYPE_FLOAT, 0.0f,
															"height", G_TYPE_FLOAT, 0.0f);

				/* Set up default final values for animation */
				clutter_actor_get_allocation_box(CLUTTER_ACTOR(self), &finalBox);
				finals=xfdashboard_animation_defaults_new(4,
															"x", G_TYPE_FLOAT, finalBox.x1,
															"y", G_TYPE_FLOAT, finalBox.y1,
															"width", G_TYPE_FLOAT, clutter_actor_box_get_width(&finalBox),
															"height", G_TYPE_FLOAT, clutter_actor_box_get_height(&finalBox));

				/* Lookup animation for create signal and set up values for allocation */
				priv->firstTimeMappedAnimation=xfdashboard_animation_new_with_values(self, "created", initials, finals);

				/* Free default initial and final values */
				xfdashboard_animation_defaults_free(initials);
				xfdashboard_animation_defaults_free(finals);

				/* Run animation if found */
				if(!priv->firstTimeMappedAnimation ||
					!xfdashboard_animation_get_id(priv->firstTimeMappedAnimation) ||
					xfdashboard_animation_is_empty(priv->firstTimeMappedAnimation))
				{
					/* Empty or invalid animation, so release allocated resources and return */
					if(priv->firstTimeMappedAnimation) g_object_unref(priv->firstTimeMappedAnimation);
					priv->firstTimeMappedAnimation=NULL;

					return;
				}

				/* Disallow allocation animation if not set in 'created' animation */
				priv->disallowAllocationAnimation=TRUE;

				/* Start animation */
				g_signal_connect_after(priv->firstTimeMappedAnimation, "animation-done", G_CALLBACK(_xfdashboard_actor_first_time_created_animation_done), self);
				xfdashboard_animation_run(priv->firstTimeMappedAnimation);
				XFDASHBOARD_DEBUG(self, ANIMATION,
										"Found and starting animation '%s' for created signal at actor %s",
										xfdashboard_animation_get_id(priv->firstTimeMappedAnimation),
										G_OBJECT_TYPE_NAME(self));
			}
		}
	}
}

/* Actor was (re)named */
static void _xfdashboard_actor_on_name_changed(GObject *inObject,
												GParamSpec *inSpec,
												gpointer inUserData)
{
	XfdashboardActor			*self;

	g_return_if_fail(XFDASHBOARD_IS_ACTOR(inObject));

	self=XFDASHBOARD_ACTOR(inObject);

	/* Invalide styling to get it recomputed because its ID (from point
	 * of view of css) has changed. Also invalidate children as they
	 * might reference the old, invalid ID or the new, valid one.
	 */
	_xfdashboard_actor_invalidate_recursive(CLUTTER_ACTOR(self));
}

/* Actor's reactive state changed */
static void _xfdashboard_actor_on_reactive_changed(GObject *inObject,
													GParamSpec *inSpec,
													gpointer inUserData)
{
	XfdashboardActor			*self;

	g_return_if_fail(XFDASHBOARD_IS_ACTOR(inObject));

	self=XFDASHBOARD_ACTOR(inObject);

	/* Add pseudo-class ':insensitive' if actor is now not reactive
	 * and remove this pseudo-class if actor is now reactive.
	 */
	if(clutter_actor_get_reactive(CLUTTER_ACTOR(self)))
	{
		xfdashboard_stylable_remove_pseudo_class(XFDASHBOARD_STYLABLE(self), "insensitive");
	}
		else
		{
			xfdashboard_stylable_add_pseudo_class(XFDASHBOARD_STYLABLE(self), "insensitive");
		}

	/* Invalide styling to get it recomputed */
	_xfdashboard_actor_invalidate_recursive(CLUTTER_ACTOR(self));
}

/* Update effects of actor with string of list of effect IDs */
static void _xfdashboard_actor_update_effects(XfdashboardActor *self, const gchar *inEffects)
{
	XfdashboardActorPrivate		*priv;
	XfdashboardTheme			*theme;
	XfdashboardThemeEffects		*themeEffects;
	gchar						**effectIDs;
	gchar						**iter;
	gchar						*effectsList;

	g_return_if_fail(XFDASHBOARD_IS_ACTOR(self));

	priv=self->priv;
	effectIDs=NULL;
	effectsList=NULL;

	/* Get theme effect instance which is needed to create effect objects.
	 * Also take a reference on theme effect instance as it needs to be alive
	 * while iterating through list of effect IDs and creating these effects.
	 */
	theme=xfdashboard_application_get_theme(NULL);

	themeEffects=xfdashboard_theme_get_effects(theme);
	g_object_ref(themeEffects);

	/* Get array of effect ID to create */
	if(inEffects) effectIDs=xfdashboard_split_string(inEffects, " \t\r\n");

	/* Remove all effects from actor */
	clutter_actor_clear_effects(CLUTTER_ACTOR(self));

	/* Create effects by their ID, add them to actor and
	 * build result string with new list of effect IDs
	 */
	iter=effectIDs;
	while(iter && *iter)
	{
		ClutterEffect			*effect;

		/* Create effect and if it was created successfully
		 * add it to actor and update final string with list
		 * of effect IDs.
		 */
		effect=xfdashboard_theme_effects_create_effect(themeEffects, *iter);
		if(effect)
		{
			clutter_actor_add_effect(CLUTTER_ACTOR(self), effect);

			if(effectsList)
			{
				gchar			*tempEffectsList;

				tempEffectsList=g_strconcat(effectsList, " ", *iter, NULL);
				g_free(effectsList);
				effectsList=tempEffectsList;
			}
				else effectsList=g_strdup(*iter);
		}

		/* Continue with next ID */
		iter++;
	}

	/* Set new string with list of effects */
	if(priv->effects) g_free(priv->effects);
	priv->effects=g_strdup(effectsList);

	/* Release allocated resources */
	if(effectsList) g_free(effectsList);
	if(effectIDs) g_strfreev(effectIDs);
	g_object_unref(themeEffects);
}

/* IMPLEMENTATION: Interface XfdashboardFocusable */

/* Check if actor can get focus */
static gboolean _xfdashboard_actor_focusable_can_focus(XfdashboardFocusable *inFocusable)
{
	XfdashboardActor			*self;
	XfdashboardActorPrivate		*priv;

	g_return_val_if_fail(XFDASHBOARD_IS_FOCUSABLE(inFocusable), FALSE);
	g_return_val_if_fail(XFDASHBOARD_IS_ACTOR(inFocusable), FALSE);

	self=XFDASHBOARD_ACTOR(inFocusable);
	priv=self->priv;

	/* This actor can only be focused if it is mapped, visible and reactive */
	if(priv->canFocus &&
		clutter_actor_is_mapped(CLUTTER_ACTOR(self)) &&
		clutter_actor_is_visible(CLUTTER_ACTOR(self)) &&
		clutter_actor_get_reactive(CLUTTER_ACTOR(self)))
	{
		return(TRUE);
	}

	/* If we get here this actor does not fulfill the requirements to get focus */
	return(FALSE);
}

/* Interface initialization
 * Set up default functions
 */
static void _xfdashboard_actor_focusable_iface_init(XfdashboardFocusableInterface *iface)
{
	iface->can_focus=_xfdashboard_actor_focusable_can_focus;
}

/* IMPLEMENTATION: Interface XfdashboardStylable */

/* Get stylable properties of actor */
static void _xfdashboard_actor_stylable_get_stylable_properties(XfdashboardStylable *inStylable,
																		GHashTable *ioStylableProperties)
{
	g_return_if_fail(CLUTTER_IS_ACTOR(inStylable));

	/* Set up hashtable of stylable properties for this instance */
	_xfdashboard_actor_hashtable_get_all_stylable_param_specs(ioStylableProperties,
																G_OBJECT_CLASS(inStylable),
																TRUE);
}

/* Get stylable name of actor */
static const gchar* _xfdashboard_actor_stylable_get_name(XfdashboardStylable *inStylable)
{
	g_return_val_if_fail(CLUTTER_IS_ACTOR(inStylable), NULL);

	return(clutter_actor_get_name(CLUTTER_ACTOR(inStylable)));
}

/* Get stylable parent of actor */
static XfdashboardStylable* _xfdashboard_actor_stylable_get_parent(XfdashboardStylable *inStylable)
{
	ClutterActor			*self;
	ClutterActor			*parent;

	g_return_val_if_fail(CLUTTER_IS_ACTOR(inStylable), NULL);

	self=CLUTTER_ACTOR(inStylable);

	/* Get parent and check if stylable. If not return NULL. */
	parent=clutter_actor_get_parent(self);
	if(!XFDASHBOARD_IS_STYLABLE(parent)) return(NULL);

	/* Return stylable parent */
	return(XFDASHBOARD_STYLABLE(parent));
}

/* Get/set style classes of actor */
static const gchar* _xfdashboard_actor_stylable_get_classes(XfdashboardStylable *inStylable)
{
	XfdashboardActor			*self;

	g_return_val_if_fail(XFDASHBOARD_IS_ACTOR(inStylable), NULL);

	self=XFDASHBOARD_ACTOR(inStylable);

	return(self->priv->styleClasses);
}

static void _xfdashboard_actor_stylable_set_classes(XfdashboardStylable *inStylable, const gchar *inStyleClasses)
{
	XfdashboardActor			*self;
	XfdashboardActorPrivate		*priv;

	g_return_if_fail(XFDASHBOARD_IS_ACTOR(inStylable));

	self=XFDASHBOARD_ACTOR(inStylable);
	priv=self->priv;

	/* Set value if changed */
	if(g_strcmp0(priv->styleClasses, inStyleClasses))
	{
		/* Set value */
		if(priv->styleClasses)
		{
			g_free(priv->styleClasses);
			priv->styleClasses=NULL;
		}

		if(inStyleClasses) priv->styleClasses=g_strdup(inStyleClasses);

		/* Invalidate style to get it restyled and redrawn. Also invalidate
		 * children as they might reference the old, invalid classes or
		 * the new, valid ones.
		 */
		_xfdashboard_actor_invalidate_recursive(CLUTTER_ACTOR(self));

		/* Notify about property change */
		g_object_notify(G_OBJECT(self), "style-classes");
	}
}

/* Get/set style pseudo-classes of actor */
static const gchar* _xfdashboard_actor_stylable_get_pseudo_classes(XfdashboardStylable *inStylable)
{
	XfdashboardActor			*self;

	g_return_val_if_fail(XFDASHBOARD_IS_ACTOR(inStylable), NULL);

	self=XFDASHBOARD_ACTOR(inStylable);

	return(self->priv->stylePseudoClasses);
}

static void _xfdashboard_actor_stylable_set_pseudo_classes(XfdashboardStylable *inStylable, const gchar *inStylePseudoClasses)
{
	XfdashboardActor			*self;
	XfdashboardActorPrivate		*priv;

	g_return_if_fail(XFDASHBOARD_IS_ACTOR(inStylable));

	self=XFDASHBOARD_ACTOR(inStylable);
	priv=self->priv;

	/* Set value if changed */
	if(g_strcmp0(priv->stylePseudoClasses, inStylePseudoClasses))
	{
		/* Set value */
		if(priv->stylePseudoClasses)
		{
			g_free(priv->stylePseudoClasses);
			priv->stylePseudoClasses=NULL;
		}

		if(inStylePseudoClasses) priv->stylePseudoClasses=g_strdup(inStylePseudoClasses);

		/* Invalidate style to get it restyled and redrawn. Also invalidate
		 * children as they might reference the old, invalid pseudo-classes
		 * or the new, valid ones.
		 */
		_xfdashboard_actor_invalidate_recursive(CLUTTER_ACTOR(self));

		/* Notify about property change */
		g_object_notify(G_OBJECT(self), "style-pseudo-classes");
	}
}

/* Invalidate style to recompute styles */
static void _xfdashboard_actor_stylable_invalidate(XfdashboardStylable *inStylable)
{
	XfdashboardActor			*self;
	XfdashboardActorPrivate		*priv;
	XfdashboardActorClass		*klass;
	XfdashboardTheme			*theme;
	XfdashboardThemeCSS			*themeCSS;
	GHashTable					*possibleStyleSet;
	GParamSpec					*paramSpec;
	GHashTableIter				hashIter;
	GHashTable					*themeStyleSet;
	gchar						*styleName;
	XfdashboardThemeCSSValue	*styleValue;
	gboolean					didChange;
#ifdef DEBUG
	gboolean					doDebug=FALSE;
#endif

	g_return_if_fail(XFDASHBOARD_IS_ACTOR(inStylable));

	self=XFDASHBOARD_ACTOR(inStylable);
	priv=self->priv;
	klass=XFDASHBOARD_ACTOR_GET_CLASS(self);
	didChange=FALSE;

	/* Only recompute style for mapped actors or if revalidation was forced */
	if(!priv->forceStyleRevalidation && !clutter_actor_is_mapped(CLUTTER_ACTOR(self))) return;

	/* Get theme */
	theme=xfdashboard_application_get_theme(NULL);

	/* Get theme CSS */
	themeCSS=xfdashboard_theme_get_css(theme);

	/* First get list of all stylable properties of this and parent classes.
	 * It is used to determine if key in theme style sets are valid.
	 */
	possibleStyleSet=xfdashboard_actor_get_stylable_properties_full(klass);

#ifdef DEBUG
	if(doDebug)
	{
		gint					i=0;
		gchar					*defaultsKey;
		GValue					defaultsVal=G_VALUE_INIT;
		gchar					*defaultsValStr;
		GParamSpec				*realParamSpec;

		XFDASHBOARD_DEBUG(self, STYLE,
							"Got param specs for %p (%s) with class '%s' and pseudo-class '%s'",
							self,
							G_OBJECT_TYPE_NAME(self),
							priv->styleClasses,
							priv->stylePseudoClasses);

		g_hash_table_iter_init(&hashIter, possibleStyleSet);
		while(g_hash_table_iter_next(&hashIter, (gpointer*)&defaultsKey, (gpointer*)&paramSpec))
		{
			realParamSpec=(GParamSpec*)g_param_spec_get_qdata(paramSpec, XFDASHBOARD_ACTOR_PARAM_SPEC_REF);

			g_value_init(&defaultsVal, G_PARAM_SPEC_VALUE_TYPE(realParamSpec));
			g_param_value_set_default(realParamSpec, &defaultsVal);

			defaultsValStr=g_strdup_value_contents(&defaultsVal);
			XFDASHBOARD_DEBUG(self, STYLE,
								"%d: param spec [%s] %s=%s\n",
								++i,
								G_OBJECT_CLASS_NAME(klass),
								defaultsKey,
								defaultsValStr);
			g_free(defaultsValStr);

			g_value_unset(&defaultsVal);
		}

		XFDASHBOARD_DEBUG(self, STYLE, "End of param specs");
	}
#endif

	/* Get style information from theme */
	themeStyleSet=xfdashboard_theme_css_get_properties(themeCSS, XFDASHBOARD_STYLABLE(self));

#ifdef DEBUG
	if(doDebug)
	{
		gint					i=0;

		XFDASHBOARD_DEBUG(self, STYLE,
							"Got styles from theme for %p (%s) with class '%s' and pseudo-class '%s'",
							self,
							G_OBJECT_TYPE_NAME(self),
							priv->styleClasses,
							priv->stylePseudoClasses);

		g_hash_table_iter_init(&hashIter, themeStyleSet);
		while(g_hash_table_iter_next(&hashIter, (gpointer*)&styleName, (gpointer*)&styleValue))
		{
			XFDASHBOARD_DEBUG(self, STYLE,
								"%d: [%s] %s=%s\n",
								++i,
								styleValue->source,
								(gchar*)styleName,
								styleValue->string);
		}

		XFDASHBOARD_DEBUG(self, STYLE, "End of styles from theme");
	}
#endif

	/* The 'property-changed' notification will be freezed and thawed
	 * (fired at once) after all stylable properties of this instance are set.
	 */
	g_object_freeze_notify(G_OBJECT(self));

	/* Iterate through style information retrieved from theme and
	 * set the corresponding property in object instance if key
	 * is valid.
	 */
	g_hash_table_iter_init(&hashIter, themeStyleSet);
	while(g_hash_table_iter_next(&hashIter, (gpointer*)&styleName, (gpointer*)&styleValue))
	{
		GValue					cssValue=G_VALUE_INIT;
		GValue					propertyValue=G_VALUE_INIT;
		GParamSpec				*realParamSpec;

		/* Check if key is a valid object property name */
		if(!g_hash_table_lookup_extended(possibleStyleSet, styleName, NULL, (gpointer*)&paramSpec)) continue;

		/* Get original referenced parameter specification. It does not need
		 * to be referenced while converting because it is valid as this
		 * value is stored in hashtable.
		 */
		realParamSpec=(GParamSpec*)g_param_spec_get_qdata(paramSpec, XFDASHBOARD_ACTOR_PARAM_SPEC_REF);

		/* Convert style value to type of object property and set value
		 * if conversion was successful. Otherwise do nothing.
		 */
		g_value_init(&cssValue, G_TYPE_STRING);
		g_value_set_string(&cssValue, styleValue->string);

		g_value_init(&propertyValue, G_PARAM_SPEC_VALUE_TYPE(realParamSpec));

		if(g_param_value_convert(realParamSpec, &cssValue, &propertyValue, FALSE))
		{
			g_object_set_property(G_OBJECT(self), styleName, &propertyValue);
			didChange=TRUE;
#ifdef DEBUG
			if(doDebug)
			{
				gchar					*valstr;

				valstr=g_strdup_value_contents(&propertyValue);
				XFDASHBOARD_DEBUG(self, STYLE,
									"Setting theme value of style property [%s] %s=%s\n",
									G_OBJECT_CLASS_NAME(klass),
									styleName,
									valstr);
				g_free(valstr);
			}
#endif
		}
			else
			{
				g_warning("Could not transform CSS string value for property '%s' to type %s of class %s",
							styleName, g_type_name(G_PARAM_SPEC_VALUE_TYPE(realParamSpec)), G_OBJECT_CLASS_NAME(klass));
			}

		/* Release allocated resources */
		g_value_unset(&propertyValue);
		g_value_unset(&cssValue);
	}

	/* Now remove all duplicate keys in set of properties changed we set the last
	 * time. The remaining keys determine the properties which were set the last
	 * time but not this time and should be restored to their default values.
	 */
	if(priv->lastThemeStyleSet)
	{
		/* Remove duplicate keys from set of last changed properties */
		g_hash_table_foreach_remove(priv->lastThemeStyleSet, (GHRFunc)_xfdashboard_actor_hashtable_is_duplicate_key, themeStyleSet);

		/* Iterate through remaining key and restore corresponding object properties
		 * to their default values.
		 */
		g_hash_table_iter_init(&hashIter, priv->lastThemeStyleSet);
		while(g_hash_table_iter_next(&hashIter, (gpointer*)&styleName, (gpointer*)&paramSpec))
		{
			GValue				propertyValue=G_VALUE_INIT;
			GParamSpec			*realParamSpec;

			/* Check if key is a valid object property name */
			if(!g_hash_table_lookup_extended(possibleStyleSet, styleName, NULL, (gpointer*)&paramSpec)) continue;

			/* Get original referenced parameter specification. It does not need
			 * to be referenced while converting because it is valid as this
			 * value is stored in hashtable.
			 */
			realParamSpec=(GParamSpec*)g_param_spec_get_qdata(paramSpec, XFDASHBOARD_ACTOR_PARAM_SPEC_REF);

			/* Initialize property value to its type and default value */
			g_value_init(&propertyValue, G_PARAM_SPEC_VALUE_TYPE(realParamSpec));
			g_param_value_set_default(realParamSpec, &propertyValue);

			/* Set value at object property */
			g_object_set_property(G_OBJECT(self), styleName, &propertyValue);
			didChange=TRUE;
#ifdef DEBUG
			if(doDebug)
			{
				gchar					*valstr;

				valstr=g_strdup_value_contents(&propertyValue);
				XFDASHBOARD_DEBUG(self, STYLE,
									"Restoring default value of style property [%s] %s=%s\n",
									G_OBJECT_CLASS_NAME(klass),
									styleName,
									valstr);
				g_free(valstr);
			}
#endif

			/* Release allocated resources */
			g_value_unset(&propertyValue);
		}

		/* Release resources of set of last changed properties as we do not need
		 * it anymore.
		 */
		g_hash_table_destroy(priv->lastThemeStyleSet);
		priv->lastThemeStyleSet=NULL;
	}

	/* Remember this set of changed properties for next time to determine properties
	 * which need to be restored to their default value.
	 */
	priv->lastThemeStyleSet=themeStyleSet;

	/* Release allocated resources */
	g_hash_table_destroy(possibleStyleSet);

	/* Force a redraw if any change was made at this actor */
	if(didChange) clutter_actor_queue_redraw(CLUTTER_ACTOR(self));

	/* Reset force style revalidation flag because it's done now */
	priv->forceStyleRevalidation=FALSE;

	/* All stylable properties are set now. So thaw 'property-changed'
	 * notification now and fire all notifications at once.
	 */
	g_object_thaw_notify(G_OBJECT(self));
}

/* Animation has completed, so remove from list */
static void _xfdashboard_actor_animation_done(XfdashboardAnimation *inAnimation,
												gpointer inUserData)
{
	XfdashboardActor				*self;
	XfdashboardActorPrivate			*priv;
	GSList							*iter;
	XfdashboardActorAnimationEntry	*data;

	g_return_if_fail(XFDASHBOARD_IS_ANIMATION(inAnimation));
	g_return_if_fail(XFDASHBOARD_IS_ACTOR(inUserData));

	self=XFDASHBOARD_ACTOR(inUserData);
	priv=self->priv;

	/* Lookup animation done in list of animation and remove animation entry */
	for(iter=priv->animations; iter; iter=g_slist_next(iter))
	{
		/* Get animation entry at iterator */
		data=(XfdashboardActorAnimationEntry*)iter->data;
		if(!data) continue;

		/* Check if animation entry matches the animation done */
		if(data->animation==inAnimation)
		{
			XFDASHBOARD_DEBUG(self, ANIMATION,
									"Removing stopped animation '%s'",
									xfdashboard_animation_get_id(data->animation));

			/* Remove entry from list, but set pointer to animation in entry
			 * to NULL to avoid unreffing an already disposed or finalized
			 * object instance.
			 */
			priv->animations=g_slist_remove_link(priv->animations, iter);

			data->animation=NULL;
			_xfdashboard_actor_animation_entry_free(data);
			g_slist_free_1(iter);
		}
	}
}

/* Remove animations for signal and (pseudo-)class */
static void _xfdashboard_actor_remove_animation(XfdashboardActor *self,
												const gchar *inAnimationSignal)
{
	XfdashboardActorPrivate			*priv;
	GSList							*iter;
	XfdashboardActorAnimationEntry	*data;

	g_return_if_fail(XFDASHBOARD_IS_ACTOR(self));
	g_return_if_fail(inAnimationSignal && *inAnimationSignal);

	priv=self->priv;

	/* Iterate through list of animation and lookup animation for signal. If an
	 * animation is found, stop it. It will be remove from list of animations
	 * automatically.
	 */
	for(iter=priv->animations; iter; iter=g_slist_next(iter))
	{
		/* Get animation entry at iterator */
		data=(XfdashboardActorAnimationEntry*)iter->data;
		if(!data) continue;

		/* Check if animation entry matches the signal to lookup */
		if(g_strcmp0(data->signal, inAnimationSignal)==0)
		{
			XFDASHBOARD_DEBUG(self, ANIMATION,
									"Stopping and removing animation '%s' for signal '%s'",
									xfdashboard_animation_get_id(data->animation),
									inAnimationSignal);

			/* Stop animation by unreffing object instance which calls the
			 * done callback _xfdashboard_actor_animation_done() of animation.
			 */
			g_object_unref(data->animation);
		}
	}
}

/* Lookup animations for signal and (pseudo-)class and run animation at actor */
static XfdashboardAnimation* _xfdashboard_actor_add_animation(XfdashboardActor *self,
																const gchar *inAnimationSignal)
{
	XfdashboardActorPrivate			*priv;
	XfdashboardAnimation			*animation;
	XfdashboardActorAnimationEntry	*data;

	g_return_val_if_fail(XFDASHBOARD_IS_ACTOR(self), NULL);
	g_return_val_if_fail(inAnimationSignal && *inAnimationSignal, NULL);

	priv=self->priv;

	/* Do not lookup and add animations if actor is in destruction now */
	if(priv->inDestruction) return(NULL);

	/* Lookup animation for signal-(pseudo-)class combination and if any found
	 * (i.e. has an ID) add it to list of animations of actor and run it.
	 */
	animation=xfdashboard_animation_new(XFDASHBOARD_ACTOR(self), inAnimationSignal);
	if(!animation ||
		xfdashboard_animation_is_empty(animation))
	{
		/* Release animation and return NULL */
		if(animation) g_object_unref(animation);
		return(NULL);
	}

	/* Check for duplicate animation */
	if(clutter_actor_get_transition(CLUTTER_ACTOR(self), xfdashboard_animation_get_id(animation)))
	{
		XFDASHBOARD_DEBUG(self, ANIMATION,
								"Duplicate animation found for signal '%s'",
								inAnimationSignal);

		/* Release animation and return NULL */
		g_object_unref(animation);
		return(NULL);
	}

	/* Create animation entry data and add to list of animations */
	data=g_new0(XfdashboardActorAnimationEntry, 1);
	if(!data)
	{
		g_critical("Cannot allocate memory for animation entry for animation '%s' with signal '%s'",
					xfdashboard_animation_get_id(animation),
					inAnimationSignal);

		/* Release animation and return NULL */
		g_object_unref(animation);
		return(NULL);
	}

	data->signal=g_strdup(inAnimationSignal);
	data->animation=animation;

	priv->animations=g_slist_prepend(priv->animations, data);

	/* Start animation */
	g_signal_connect_after(animation, "animation-done", G_CALLBACK(_xfdashboard_actor_animation_done), self);
	xfdashboard_animation_run(animation);
	XFDASHBOARD_DEBUG(self, ANIMATION,
							"Found and starting animation '%s' for signal '%s'",
							xfdashboard_animation_get_id(animation),
							inAnimationSignal);

	return(animation);
}

/* Signal handler for "class-added" signal of stylable interface */
static void _xfdashboard_actor_stylable_class_added(XfdashboardStylable *inStylable, const gchar *inClass)
{
	gchar			*animationSignal;

	g_return_if_fail(XFDASHBOARD_IS_ACTOR(inStylable));

	/* Remove any animation that was added when this class was removed */
	animationSignal=g_strdup_printf("class-removed:%s", inClass);
	_xfdashboard_actor_remove_animation(XFDASHBOARD_ACTOR(inStylable), animationSignal);
	g_free(animationSignal);

	/* Create animation for this class added */
	animationSignal=g_strdup_printf("class-added:%s", inClass);
	_xfdashboard_actor_add_animation(XFDASHBOARD_ACTOR(inStylable), animationSignal);
	g_free(animationSignal);
}

/* Signal handler for "class-removed" signal of stylable interface */
static void _xfdashboard_actor_stylable_class_removed(XfdashboardStylable *inStylable, const gchar *inClass)
{
	gchar			*animationSignal;

	g_return_if_fail(XFDASHBOARD_IS_ACTOR(inStylable));

	/* Remove any animation that was added when this class was added */
	animationSignal=g_strdup_printf("class-added:%s", inClass);
	_xfdashboard_actor_remove_animation(XFDASHBOARD_ACTOR(inStylable), animationSignal);
	g_free(animationSignal);

	/* Create animation for this class removed */
	animationSignal=g_strdup_printf("class-removed:%s", inClass);
	_xfdashboard_actor_add_animation(XFDASHBOARD_ACTOR(inStylable), animationSignal);
	g_free(animationSignal);
}

/* Signal handler for "pseudo-class-added" signal of stylable interface */
static void _xfdashboard_actor_stylable_pseudo_class_added(XfdashboardStylable *inStylable, const gchar *inClass)
{
	gchar			*animationSignal;

	g_return_if_fail(XFDASHBOARD_IS_ACTOR(inStylable));

	/* Remove any animation that was added when this pseudo-class was removed */
	animationSignal=g_strdup_printf("pseudo-class-removed:%s", inClass);
	_xfdashboard_actor_remove_animation(XFDASHBOARD_ACTOR(inStylable), animationSignal);
	g_free(animationSignal);

	/* Create animation for this pseudo-class added */
	animationSignal=g_strdup_printf("pseudo-class-added:%s", inClass);
	_xfdashboard_actor_add_animation(XFDASHBOARD_ACTOR(inStylable), animationSignal);
	g_free(animationSignal);
}

/* Signal handler for "pseudo-class-removed" signal of stylable interface */
static void _xfdashboard_actor_stylable_pseudo_class_removed(XfdashboardStylable *inStylable, const gchar *inClass)
{
	gchar			*animationSignal;

	g_return_if_fail(XFDASHBOARD_IS_ACTOR(inStylable));

	/* Remove any animation that was added when this pseudo-class was added */
	animationSignal=g_strdup_printf("pseudo-class-added:%s", inClass);
	_xfdashboard_actor_remove_animation(XFDASHBOARD_ACTOR(inStylable), animationSignal);
	g_free(animationSignal);

	/* Create animation for this pseudo-class removed */
	animationSignal=g_strdup_printf("pseudo-class-removed:%s", inClass);
	_xfdashboard_actor_add_animation(XFDASHBOARD_ACTOR(inStylable), animationSignal);
	g_free(animationSignal);
}

/* Interface initialization
 * Set up default functions
 */
static void _xfdashboard_actor_stylable_iface_init(XfdashboardStylableInterface *iface)
{
	iface->get_stylable_properties=_xfdashboard_actor_stylable_get_stylable_properties;
	iface->get_name=_xfdashboard_actor_stylable_get_name;
	iface->get_parent=_xfdashboard_actor_stylable_get_parent;
	iface->get_classes=_xfdashboard_actor_stylable_get_classes;
	iface->set_classes=_xfdashboard_actor_stylable_set_classes;
	iface->class_added=_xfdashboard_actor_stylable_class_added;
	iface->class_removed=_xfdashboard_actor_stylable_class_removed;
	iface->get_pseudo_classes=_xfdashboard_actor_stylable_get_pseudo_classes;
	iface->set_pseudo_classes=_xfdashboard_actor_stylable_set_pseudo_classes;
	iface->pseudo_class_added=_xfdashboard_actor_stylable_pseudo_class_added;
	iface->pseudo_class_removed=_xfdashboard_actor_stylable_pseudo_class_removed;
	iface->invalidate=_xfdashboard_actor_stylable_invalidate;
}

/* IMPLEMENTATION: ClutterActor */

/* Allocation animation ended */
static void _xfdashboard_actor_on_allocation_animation_done(XfdashboardAnimation *inAnimation,
															gpointer inUserData)
{
	XfdashboardActor				*self;
	XfdashboardActorPrivate			*priv;

	g_return_if_fail(XFDASHBOARD_IS_ANIMATION(inAnimation));
	g_return_if_fail(XFDASHBOARD_IS_ACTOR(inUserData));

	self=XFDASHBOARD_ACTOR(inUserData);
	priv=self->priv;

	/* Release allocated resources */
	priv->allocationAnimation=NULL;
}

/* Actor's allocation changed */
static void _xfdashboard_actor_on_allocation_changed(ClutterActor *inActor,
														const ClutterActorBox *inAllocationBox,
														ClutterAllocationFlags inFlags,
														gpointer inUserData)
{
	XfdashboardActor				*self;
	XfdashboardActorPrivate			*priv;

	g_return_if_fail(XFDASHBOARD_IS_ACTOR(inActor));
	g_return_if_fail(inAllocationBox);

	self=XFDASHBOARD_ACTOR(inActor);
	priv=self->priv;

	/* Track allocation changes */
	priv->allocationTrackBox->x1=inAllocationBox->x1;
	priv->allocationTrackBox->x2=inAllocationBox->x2;
	priv->allocationTrackBox->y1=inAllocationBox->y1;
	priv->allocationTrackBox->y2=inAllocationBox->y2;

		/* Check if allocation animation was requested explicitly */
	if(priv->doAllocationAnimation &&
		!priv->disallowAllocationAnimation)
	{
		XfdashboardAnimation		*animation;
		XfdashboardAnimationValue	**initials;
		XfdashboardAnimationValue	**finals;

		/* Stop currently running animation if any */
		if(priv->allocationAnimation)
		{
			/* Stop animation */
			g_object_unref(priv->allocationAnimation);
		}

		/* Set up default initial values for animation */
		g_assert(priv->allocationInitialBox!=NULL);
		initials=xfdashboard_animation_defaults_new(4,
													"x", G_TYPE_FLOAT, priv->allocationInitialBox->x1,
													"y", G_TYPE_FLOAT, priv->allocationInitialBox->y1,
													"width", G_TYPE_FLOAT, clutter_actor_box_get_width(priv->allocationInitialBox),
													"height", G_TYPE_FLOAT, clutter_actor_box_get_height(priv->allocationInitialBox));

		/* Set up default final values for animation */
		finals=xfdashboard_animation_defaults_new(4,
													"x", G_TYPE_FLOAT, inAllocationBox->x1,
													"y", G_TYPE_FLOAT, inAllocationBox->y1,
													"width", G_TYPE_FLOAT, clutter_actor_box_get_width(inAllocationBox),
													"height", G_TYPE_FLOAT, clutter_actor_box_get_height(inAllocationBox));

		/* Create and start animation */
		animation=xfdashboard_animation_new_with_values(self,
														ALLOCATION_ANIMATION_SIGNAL,
														initials,
														finals);
		if(animation)
		{
			/* If animation is not empty, start it now */
			if(!xfdashboard_animation_is_empty(animation))
			{
				/* Remember the allocation animation */
				priv->allocationAnimation=animation;

				/* Start animation */
				g_signal_connect_after(animation,
										"animation-done",
										G_CALLBACK(_xfdashboard_actor_on_allocation_animation_done),
										inActor);
				xfdashboard_animation_run(priv->allocationAnimation);

				/* Take an extra reference on animation to keep it alive
				 * when it is unreferenced later this function.
				 */
				g_object_ref(priv->allocationAnimation);
			}

			/* Unreference animation, so it will either be disposed
			 * because it was empty or it will survive because it
			 * was started and an extra reference was taken.
			 */
			g_object_unref(animation);
		}

		/* Free default initial and final values */
		xfdashboard_animation_defaults_free(initials);
		xfdashboard_animation_defaults_free(finals);
	}

	/* Unset flag indicating an allocation animation was requested,
	 * as it was handled now or prevented if disallowed.
	 */
	priv->doAllocationAnimation=FALSE;
}

/* Pointer left actor */
static gboolean _xfdashboard_actor_leave_event(ClutterActor *inActor, ClutterCrossingEvent *inEvent)
{
	XfdashboardActor		*self;
	ClutterActorClass		*parentClass;

	g_return_val_if_fail(XFDASHBOARD_IS_ACTOR(inActor), CLUTTER_EVENT_PROPAGATE);

	self=XFDASHBOARD_ACTOR(inActor);

	/* Call parent's virtual function */
	parentClass=CLUTTER_ACTOR_CLASS(xfdashboard_actor_parent_class);
	if(parentClass->leave_event)
	{
		parentClass->leave_event(inActor, inEvent);
	}

	/* Remove pseudo-class ":hover" because pointer left actor */
	xfdashboard_stylable_remove_pseudo_class(XFDASHBOARD_STYLABLE(self), "hover");

	return(CLUTTER_EVENT_PROPAGATE);
}

/* Pointer entered actor */
static gboolean _xfdashboard_actor_enter_event(ClutterActor *inActor, ClutterCrossingEvent *inEvent)
{
	XfdashboardActor		*self;
	ClutterActorClass		*parentClass;

	g_return_val_if_fail(XFDASHBOARD_IS_ACTOR(inActor), CLUTTER_EVENT_PROPAGATE);

	self=XFDASHBOARD_ACTOR(inActor);

	/* Call parent's virtual function */
	parentClass=CLUTTER_ACTOR_CLASS(xfdashboard_actor_parent_class);
	if(parentClass->enter_event)
	{
		parentClass->enter_event(inActor, inEvent);
	}

	/* Add pseudo-class ":hover" because pointer entered actor */
	xfdashboard_stylable_add_pseudo_class(XFDASHBOARD_STYLABLE(self), "hover");

	return(CLUTTER_EVENT_PROPAGATE);
}

/* Actor was (re)parented */
static void _xfdashboard_actor_parent_set(ClutterActor *inActor, ClutterActor *inOldParent)
{
	XfdashboardActor				*self;
	XfdashboardActorPrivate			*priv;
	ClutterActorClass				*parentClass;
	ClutterActor					*parent;

	g_return_if_fail(XFDASHBOARD_IS_ACTOR(inActor));

	self=XFDASHBOARD_ACTOR(inActor);
	priv=self->priv;

	/* Call parent's virtual function */
	parentClass=CLUTTER_ACTOR_CLASS(xfdashboard_actor_parent_class);
	if(parentClass->parent_set)
	{
		parentClass->parent_set(inActor, inOldParent);
	}

	/* Get new parent of actor */
	parent=clutter_actor_get_parent(inActor);

	/* Check if it is a newly created actor which is parented for the first time.
	 * Then emit 'actor-created' signal on stage.
	 */
	if(priv->isFirstParent &&
		!inOldParent &&
		parent)
	{
		ClutterActor			*stage;

		/* Get stage where this actor belongs to and emit signal at stage */
		stage=clutter_actor_get_stage(inActor);
		if(XFDASHBOARD_IS_STAGE(stage))
		{
			g_signal_emit_by_name(stage, "actor-created", inActor, NULL);
		}

		/* Set flag that a parent set and signal was emitted */
		priv->isFirstParent=FALSE;
	}

	/* Invalide styling to get it recomputed because its ID (from point
	 * of view of css) has changed. Also invalidate children as they might
	 * reference the old, invalid parent or the new, valid one.
	 */
	_xfdashboard_actor_invalidate_recursive(CLUTTER_ACTOR(self));
}

/* Actor will be shown */
static XfdashboardAnimation* _xfdashboard_actor_replace_animation(XfdashboardActor *self,
																	const gchar *inOldSignal,
																	const gchar *inNewSignal)
{
	XfdashboardActorPrivate				*priv;
	XfdashboardAnimation				*oldAnimation;
	XfdashboardAnimation				*newAnimation;
	GSList								*iter;
	XfdashboardActorAnimationEntry		*data;

	g_return_val_if_fail(XFDASHBOARD_IS_ACTOR(self), NULL);
	g_return_val_if_fail(inOldSignal && *inOldSignal, NULL);
	g_return_val_if_fail(inNewSignal && *inNewSignal, NULL);

	priv=self->priv;
	oldAnimation=NULL;
	newAnimation=NULL;

	/* Iterate through list of animation and lookup animation for old signal */
	for(iter=priv->animations; iter; iter=g_slist_next(iter))
	{
		/* Get animation entry at iterator */
		data=(XfdashboardActorAnimationEntry*)iter->data;
		if(!data) continue;

		/* Check if animation entry matches the signal to lookup */
		if(g_strcmp0(data->signal, inOldSignal)!=0) continue;

		/* Found animation, so stop further iterations */
		oldAnimation=data->animation;
	}

	/* Get animation for new signal to replace old one */
	newAnimation=_xfdashboard_actor_add_animation(self, inNewSignal);

	/* If an animation for old signal, stop it */
	if(oldAnimation)
	{
		/* If no new animation will be started, ensure old one completes
		 * before it will be removed.
		 */
		if(!newAnimation ||
			!xfdashboard_animation_get_id(newAnimation))
		{
			xfdashboard_animation_ensure_complete(oldAnimation);
		}

		/* Stop old animation by unreffing object instance which calls the
		 * done callback _xfdashboard_actor_animation_done() of animation.
		 */
		g_object_unref(oldAnimation);
	}

	/* Return new animation which replaced old one */
	return(newAnimation);
}

static void _xfdashboard_actor_show(ClutterActor *inActor)
{
	XfdashboardActor		*self;
	ClutterActorClass		*parentClass;

	g_return_if_fail(XFDASHBOARD_IS_ACTOR(inActor));

	self=XFDASHBOARD_ACTOR(inActor);

	/* Call parent's virtual function */
	parentClass=CLUTTER_ACTOR_CLASS(xfdashboard_actor_parent_class);
	if(parentClass->show)
	{
		parentClass->show(inActor);
	}

	/* If actor is visible now check if pointer is inside this actor
	 * then add pseudo-class ":hover" to it
	 */
	if(clutter_actor_has_pointer(inActor))
	{
		xfdashboard_stylable_add_pseudo_class(XFDASHBOARD_STYLABLE(self), "hover");
	}

	/* Stop any animation started for "hiding" actor which may be still running,
	 * and lookup the one should be run when actor gets visible */
	_xfdashboard_actor_replace_animation(self, "hide", "show");
}

/* Actor will be hidden */
static void _xfdashboard_actor_hide_on_animation_done(XfdashboardActor *inActor, gpointer inUserData)
{
	ClutterActorClass		*parentClass;

	g_return_if_fail(XFDASHBOARD_IS_ACTOR(inActor));

	/* Call parent's virtual function to hide actor */
	parentClass=CLUTTER_ACTOR_CLASS(xfdashboard_actor_parent_class);
	if(parentClass->hide)
	{
		parentClass->hide(CLUTTER_ACTOR(inActor));
	}
}

static void _xfdashboard_actor_hide(ClutterActor *inActor)
{
	XfdashboardActor		*self;
	XfdashboardAnimation	*animation;

	g_return_if_fail(XFDASHBOARD_IS_ACTOR(inActor));

	self=XFDASHBOARD_ACTOR(inActor);

	/* Actor is hidden now so remove pseudo-class ":hover" because pointer cannot
	 * be in an actor hidden.
	 */
	xfdashboard_stylable_remove_pseudo_class(XFDASHBOARD_STYLABLE(self), "hover");

	/* Stop any animation started for "showing" actor which may be still running,
	 * and lookup the one should be run when actor gets hidden.
	 */
	animation=_xfdashboard_actor_replace_animation(self, "show", "hide");
	if(animation) g_signal_connect_swapped(animation, "animation-done", G_CALLBACK(_xfdashboard_actor_hide_on_animation_done), self);
		else _xfdashboard_actor_hide_on_animation_done(self, NULL);
}


/* IMPLEMENTATION: GObject */

/* Dispose this object */
static void _xfdashboard_actor_dispose(GObject *inObject)
{
	XfdashboardActor			*self=XFDASHBOARD_ACTOR(inObject);
	XfdashboardActorPrivate		*priv=self->priv;

	/* Set flag that actor will be destructed */
	priv->inDestruction=TRUE;

	/* Release allocated variables */
	if(priv->effects)
	{
		g_free(priv->effects);
		priv->effects=NULL;
	}

	if(priv->styleClasses)
	{
		g_free(priv->styleClasses);
		priv->styleClasses=NULL;
	}

	if(priv->stylePseudoClasses)
	{
		g_free(priv->stylePseudoClasses);
		priv->stylePseudoClasses=NULL;
	}

	if(priv->lastThemeStyleSet)
	{
		g_hash_table_destroy(priv->lastThemeStyleSet);
		priv->lastThemeStyleSet=NULL;
	}

	if(priv->firstTimeMappedAnimation)
	{
		g_object_unref(priv->firstTimeMappedAnimation);
		priv->firstTimeMappedAnimation=NULL;
	}

	if(priv->animations)
	{
		g_slist_free_full(priv->animations, (GDestroyNotify)_xfdashboard_actor_animation_entry_free);
		priv->animations=NULL;
	}

	if(priv->allocationAnimation)
	{
		g_object_unref(priv->allocationAnimation);
		priv->allocationAnimation=NULL;
	}

	if(priv->allocationInitialBox)
	{
		clutter_actor_box_free(priv->allocationInitialBox);
		priv->allocationInitialBox=NULL;
	}

	if(priv->allocationTrackBox)
	{
		clutter_actor_box_free(priv->allocationTrackBox);
		priv->allocationTrackBox=NULL;
	}

	priv->doAllocationAnimation=FALSE;

	/* Call parent's class dispose method */
	G_OBJECT_CLASS(xfdashboard_actor_parent_class)->dispose(inObject);
}

/* Set/get properties */
static void _xfdashboard_actor_set_property(GObject *inObject,
											guint inPropID,
											const GValue *inValue,
											GParamSpec *inSpec)
{
	XfdashboardActor			*self=XFDASHBOARD_ACTOR(inObject);

	switch(inPropID)
	{
		case PROP_CAN_FOCUS:
			xfdashboard_actor_set_can_focus(self, g_value_get_boolean(inValue));
			break;

		case PROP_EFFECTS:
			xfdashboard_actor_set_effects(self, g_value_get_string(inValue));
			break;

		case PROP_STYLE_CLASSES:
			_xfdashboard_actor_stylable_set_classes(XFDASHBOARD_STYLABLE(self),
													g_value_get_string(inValue));
			break;

		case PROP_STYLE_PSEUDO_CLASSES:
			_xfdashboard_actor_stylable_set_pseudo_classes(XFDASHBOARD_STYLABLE(self),
															g_value_get_string(inValue));
			break;

		default:
			G_OBJECT_WARN_INVALID_PROPERTY_ID(inObject, inPropID, inSpec);
			break;
	}
}

static void _xfdashboard_actor_get_property(GObject *inObject,
											guint inPropID,
											GValue *outValue,
											GParamSpec *inSpec)
{
	XfdashboardActor			*self=XFDASHBOARD_ACTOR(inObject);
	XfdashboardActorPrivate		*priv=self->priv;

	switch(inPropID)
	{
		case PROP_CAN_FOCUS:
			g_value_set_boolean(outValue, priv->canFocus);
			break;

		case PROP_EFFECTS:
			g_value_set_string(outValue, priv->effects);
			break;

		case PROP_STYLE_CLASSES:
			g_value_set_string(outValue, priv->styleClasses);
			break;

		case PROP_STYLE_PSEUDO_CLASSES:
			g_value_set_string(outValue, priv->stylePseudoClasses);
			break;

		default:
			G_OBJECT_WARN_INVALID_PROPERTY_ID(inObject, inPropID, inSpec);
			break;
	}
}

/* Access instance's private structure */
static inline gpointer xfdashboard_actor_get_instance_private(XfdashboardActor *self)
{
	return(G_STRUCT_MEMBER_P(self, xfdashboard_actor_private_offset));
}
 
/* Class initialization
 * Override functions in parent classes and define properties
 * and signals
 */
void xfdashboard_actor_class_init(XfdashboardActorClass *klass)
{
	ClutterActorClass			*clutterActorClass=CLUTTER_ACTOR_CLASS(klass);
	GObjectClass				*gobjectClass=G_OBJECT_CLASS(klass);

	/* Get parent class */
	xfdashboard_actor_parent_class=g_type_class_peek_parent(klass);

	/* Adjust offset to instance private structure */
	if(xfdashboard_actor_private_offset!=0)
	{
		g_type_class_adjust_private_offset(klass, &xfdashboard_actor_private_offset);
	}

	/* Override functions */
	gobjectClass->dispose=_xfdashboard_actor_dispose;
	gobjectClass->set_property=_xfdashboard_actor_set_property;
	gobjectClass->get_property=_xfdashboard_actor_get_property;

	clutterActorClass->parent_set=_xfdashboard_actor_parent_set;
	clutterActorClass->enter_event=_xfdashboard_actor_enter_event;
	clutterActorClass->leave_event=_xfdashboard_actor_leave_event;
	clutterActorClass->show=_xfdashboard_actor_show;
	clutterActorClass->hide=_xfdashboard_actor_hide;

	/* Create param-spec pool for themable properties */
	g_assert(_xfdashboard_actor_stylable_properties_pool==NULL);
	_xfdashboard_actor_stylable_properties_pool=g_param_spec_pool_new(FALSE);

	/* Define properties */
	XfdashboardActorProperties[PROP_CAN_FOCUS]=
		g_param_spec_boolean("can-focus",
								"Can focus",
								"This flag indicates if this actor can be focused",
								FALSE,
								G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS);
	g_object_class_install_property(G_OBJECT_CLASS(klass), PROP_CAN_FOCUS, XfdashboardActorProperties[PROP_CAN_FOCUS]);

	XfdashboardActorProperties[PROP_EFFECTS]=
		g_param_spec_string("effects",
								"Effects",
								"List of space-separated strings with IDs of effects set at this actor",
								NULL,
								G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS);
	g_object_class_install_property(G_OBJECT_CLASS(klass), PROP_EFFECTS, XfdashboardActorProperties[PROP_EFFECTS]);

	g_object_class_override_property(gobjectClass, PROP_STYLE_CLASSES, "style-classes");
	g_object_class_override_property(gobjectClass, PROP_STYLE_PSEUDO_CLASSES, "style-pseudo-classes");

	/* Define stylable properties */
	xfdashboard_actor_install_stylable_property_by_name(klass, "effects");
	xfdashboard_actor_install_stylable_property_by_name(klass, "x-expand");
	xfdashboard_actor_install_stylable_property_by_name(klass, "y-expand");
	xfdashboard_actor_install_stylable_property_by_name(klass, "x-align");
	xfdashboard_actor_install_stylable_property_by_name(klass, "y-align");
	xfdashboard_actor_install_stylable_property_by_name(klass, "margin-top");
	xfdashboard_actor_install_stylable_property_by_name(klass, "margin-bottom");
	xfdashboard_actor_install_stylable_property_by_name(klass, "margin-left");
	xfdashboard_actor_install_stylable_property_by_name(klass, "margin-right");
}

/* Object initialization
 * Create private structure and set up default values
 */
void xfdashboard_actor_init(XfdashboardActor *self)
{
	XfdashboardActorPrivate		*priv;

	priv=self->priv=xfdashboard_actor_get_instance_private(self);

	/* Set up default values */
	priv->inDestruction=FALSE;
	priv->canFocus=FALSE;
	priv->effects=NULL;
	priv->styleClasses=NULL;
	priv->stylePseudoClasses=NULL;
	priv->lastThemeStyleSet=NULL;
	priv->isFirstParent=TRUE;
	priv->firstTimeMapped=FALSE;
	priv->firstTimeMappedAnimation=NULL;
	priv->animations=NULL;
	priv->disallowAllocationAnimation=FALSE;
	priv->doAllocationAnimation=FALSE;
	priv->allocationAnimation=NULL;
	priv->allocationInitialBox=NULL;
	priv->allocationTrackBox=clutter_actor_box_new(0, 0, 0, 0);

	/* Connect signals */
	g_signal_connect(self, "notify::mapped", G_CALLBACK(_xfdashboard_actor_on_mapped_changed), NULL);
	g_signal_connect(self, "notify::name", G_CALLBACK(_xfdashboard_actor_on_name_changed), NULL);
	g_signal_connect(self, "notify::reactive", G_CALLBACK(_xfdashboard_actor_on_reactive_changed), NULL);
	g_signal_connect(self, "allocation-changed", G_CALLBACK(_xfdashboard_actor_on_allocation_changed), NULL);
}

/* IMPLEMENTATION: GType */

/* Base class finalization */
void xfdashboard_actor_base_class_finalize(XfdashboardActorClass *klass)
{
	GList				*paramSpecs, *entry;

	paramSpecs=g_param_spec_pool_list_owned(_xfdashboard_actor_stylable_properties_pool, G_OBJECT_CLASS_TYPE(klass));
	for(entry=paramSpecs; entry; entry=g_list_next(entry))
	{
		GParamSpec		*paramSpec=G_PARAM_SPEC(entry->data);

		if(paramSpec)
		{
			g_param_spec_pool_remove(_xfdashboard_actor_stylable_properties_pool, paramSpec);

			XFDASHBOARD_DEBUG(NULL, STYLE,
								"Unregistered stylable property named '%s' for class '%s'",
								g_param_spec_get_name(paramSpec), G_OBJECT_CLASS_NAME(klass));

			g_param_spec_unref(paramSpec);
		}
	}
	g_list_free(paramSpecs);
}

/* Register type to GObject system */
GType xfdashboard_actor_get_type(void)
{
	static GType		actorType=0;

	if(G_UNLIKELY(actorType==0))
	{
		/* Class info */
		const GTypeInfo 		actorInfo=
		{
			sizeof(XfdashboardActorClass),
			NULL,
			(GBaseFinalizeFunc)(void*)xfdashboard_actor_base_class_finalize,
			(GClassInitFunc)(void*)xfdashboard_actor_class_init,
			NULL, /* class_finalize */
			NULL, /* class_data */
			sizeof(XfdashboardActor),
			0,    /* n_preallocs */
			(GInstanceInitFunc)(void*)xfdashboard_actor_init,
			NULL, /* value_table */
		};

		/* Implemented interfaces info */
		const GInterfaceInfo	actorStylableInterfaceInfo=
		{
			(GInterfaceInitFunc)(void*)_xfdashboard_actor_stylable_iface_init,
			NULL,
			NULL
		};

		const GInterfaceInfo	actorFocusableInterfaceInfo=
		{
			(GInterfaceInitFunc)(void*)_xfdashboard_actor_focusable_iface_init,
			NULL,
			NULL
		};

		/* Register class */
		actorType=g_type_register_static(CLUTTER_TYPE_ACTOR,
											g_intern_static_string("XfdashboardActor"),
											&actorInfo,
											0);

		/* Add private structure */
		xfdashboard_actor_private_offset=g_type_add_instance_private(actorType, sizeof(XfdashboardActorPrivate));

		/* Add implemented interfaces */
		g_type_add_interface_static(actorType,
									XFDASHBOARD_TYPE_STYLABLE,
									&actorStylableInterfaceInfo);

		g_type_add_interface_static(actorType,
									XFDASHBOARD_TYPE_FOCUSABLE,
									&actorFocusableInterfaceInfo);
	}

	return(actorType);
}

/* IMPLEMENTATION: Public API */

/* Create new actor */
ClutterActor* xfdashboard_actor_new(void)
{
	return(CLUTTER_ACTOR(g_object_new(XFDASHBOARD_TYPE_ACTOR, NULL)));
}

/* Get/set "can-focus" flag of actor to indicate
 * if this actor is focusable or not.
 */
gboolean xfdashboard_actor_get_can_focus(XfdashboardActor *self)
{
	g_return_val_if_fail(XFDASHBOARD_IS_ACTOR(self), FALSE);

	return(self->priv->canFocus);
}

void xfdashboard_actor_set_can_focus(XfdashboardActor *self, gboolean inCanFous)
{
	XfdashboardActorPrivate		*priv;

	g_return_if_fail(XFDASHBOARD_IS_ACTOR(self));

	priv=self->priv;

	/* Set value if changed */
	if(priv->canFocus!=inCanFous)
	{
		/* Set value */
		priv->canFocus=inCanFous;

		/* Notify about property change */
		g_object_notify_by_pspec(G_OBJECT(self), XfdashboardActorProperties[PROP_CAN_FOCUS]);
	}
}

/* Get/set space-seperated list of IDs of effects used by this actor */
const gchar* xfdashboard_actor_get_effects(XfdashboardActor *self)
{
	g_return_val_if_fail(XFDASHBOARD_IS_ACTOR(self), NULL);

	return(self->priv->effects);
}

void xfdashboard_actor_set_effects(XfdashboardActor *self, const gchar *inEffects)
{
	XfdashboardActorPrivate		*priv;

	g_return_if_fail(XFDASHBOARD_IS_ACTOR(self));

	priv=self->priv;

	/* Set value if changed */
	if(g_strcmp0(priv->effects, inEffects)!=0)
	{
		/* Set value */
		_xfdashboard_actor_update_effects(self, inEffects);

		/* Notify about property change */
		g_object_notify_by_pspec(G_OBJECT(self), XfdashboardActorProperties[PROP_EFFECTS]);
	}
}

/* Register stylable property of a class */
void xfdashboard_actor_install_stylable_property(XfdashboardActorClass *klass, GParamSpec *inParamSpec)
{
	GParamSpec		*stylableParamSpec;

	g_return_if_fail(XFDASHBOARD_IS_ACTOR_CLASS(klass));
	g_return_if_fail(G_IS_PARAM_SPEC(inParamSpec));
	g_return_if_fail(inParamSpec->flags & G_PARAM_WRITABLE);
	g_return_if_fail(!(inParamSpec->flags & G_PARAM_CONSTRUCT_ONLY));

	/* Check if param-spec is already registered */
	if(g_param_spec_pool_lookup(_xfdashboard_actor_stylable_properties_pool, g_param_spec_get_name(inParamSpec), G_OBJECT_CLASS_TYPE(klass), FALSE))
	{
		g_warning("Class '%s' already contains a stylable property '%s'",
					G_OBJECT_CLASS_NAME(klass),
					g_param_spec_get_name(inParamSpec));
		return;
	}

	/* Add param-spec to pool of themable properties */
	stylableParamSpec=g_param_spec_internal(G_PARAM_SPEC_TYPE(inParamSpec),
												g_param_spec_get_name(inParamSpec),
												NULL,
												NULL,
												0);
	g_param_spec_set_qdata_full(stylableParamSpec,
									XFDASHBOARD_ACTOR_PARAM_SPEC_REF,
									g_param_spec_ref(inParamSpec),
									(GDestroyNotify)g_param_spec_unref);
	g_param_spec_pool_insert(_xfdashboard_actor_stylable_properties_pool, stylableParamSpec, G_OBJECT_CLASS_TYPE(klass));
	XFDASHBOARD_DEBUG(NULL, STYLE,
						"Registered stylable property '%s' for class '%s'",
						g_param_spec_get_name(inParamSpec), G_OBJECT_CLASS_NAME(klass));
}

void xfdashboard_actor_install_stylable_property_by_name(XfdashboardActorClass *klass, const gchar *inParamName)
{
	GParamSpec		*paramSpec;

	g_return_if_fail(XFDASHBOARD_IS_ACTOR_CLASS(klass));
	g_return_if_fail(inParamName && inParamName[0]);

	/* Find parameter specification for property name and register it as stylable */
	paramSpec=g_object_class_find_property(G_OBJECT_CLASS(klass), inParamName);
	if(paramSpec) xfdashboard_actor_install_stylable_property(klass, paramSpec);
		else
		{
			g_warning("Cannot register non-existent property '%s' of class '%s'",
						inParamName, G_OBJECT_CLASS_NAME(klass));
		}
}

/* Get hash-table with all stylable properties of this class or
 * recursively of this and all parent classes
 */
GHashTable* xfdashboard_actor_get_stylable_properties(XfdashboardActorClass *klass)
{
	GHashTable		*stylableProps;

	g_return_val_if_fail(XFDASHBOARD_IS_ACTOR_CLASS(klass), NULL);

	/* Create hashtable and insert stylable properties */
	stylableProps=g_hash_table_new_full(g_str_hash,
											g_str_equal,
											g_free,
											(GDestroyNotify)g_param_spec_unref);
	_xfdashboard_actor_hashtable_get_all_stylable_param_specs(stylableProps, G_OBJECT_CLASS(klass), FALSE);

	return(stylableProps);
}

GHashTable* xfdashboard_actor_get_stylable_properties_full(XfdashboardActorClass *klass)
{
	GHashTable		*stylableProps;

	g_return_val_if_fail(XFDASHBOARD_IS_ACTOR_CLASS(klass), NULL);

	/* Create hashtable and insert stylable properties */
	stylableProps=g_hash_table_new_full(g_str_hash,
											g_str_equal,
											g_free,
											(GDestroyNotify)g_param_spec_unref);
	_xfdashboard_actor_hashtable_get_all_stylable_param_specs(stylableProps, G_OBJECT_CLASS(klass), TRUE);

	return(stylableProps);
}

/* Force restyling actor by theme next time stylable invalidation
 * function of this actor is called
 */
void xfdashboard_actor_invalidate(XfdashboardActor *self)
{
	g_return_if_fail(XFDASHBOARD_IS_ACTOR(self));

	self->priv->forceStyleRevalidation=TRUE;
}

/* Requests to start an animation at next allocation change
 * if theme defines an animation for move/resize.
 */
void xfdashboard_actor_enable_allocation_animation_once(XfdashboardActor *self)
{
	XfdashboardActorPrivate		*priv;

	g_return_if_fail(XFDASHBOARD_IS_ACTOR(self));

	priv=self->priv;

	/* Do nothing if this flag for requesting animation was already set */
	if(priv->doAllocationAnimation) return;

	/* Set flag to create and start animation at next allocation change */
	priv->doAllocationAnimation=TRUE;

	/* Use last tracked allocation as initial allocation box */
	if(priv->allocationInitialBox)
	{
		clutter_actor_box_free(priv->allocationInitialBox);
		priv->allocationInitialBox=NULL;
	}

	priv->allocationInitialBox=clutter_actor_box_copy(priv->allocationTrackBox);
}

/* Calling clutter_actor_get_allocation_box() is dangerous with animationa
 * because it may modify the internal state of the position and size of
 * an actor used for initial values for position and size at the animation
 * to run. Therefore this safe function exists which returns the last allocation
 * set to this actor.
 */
const ClutterActorBox* xfdashboard_actor_get_allocation_box(XfdashboardActor *self)
{
	g_return_val_if_fail(XFDASHBOARD_IS_ACTOR(self), NULL);

	return(self->priv->allocationTrackBox);
}

/* Destroys an actor but checks first if an animation should be played.
 * If an animation for this actor exists, it will be played and after it
 * has ended, it will be destroyed. If no animation exists, the actor will
 * be destroyed immediately.
 */
static void _xfdashboard_actor_on_destroy_animation_done(XfdashboardAnimation *inAnimation,
															gpointer inUserData)
{
	ClutterActor				*self;

	g_return_if_fail(XFDASHBOARD_IS_ANIMATION(inAnimation));
	g_return_if_fail(CLUTTER_IS_ACTOR(inUserData));

	self=CLUTTER_ACTOR(inUserData);

	/* Destroy animation has ended, so destroy actor now for real */
	clutter_actor_destroy(self);
}

gboolean xfdashboard_actor_destroy(ClutterActor *self)
{
	XfdashboardAnimation		*animation;
	gboolean					animationStarted;

	g_return_val_if_fail(CLUTTER_IS_ACTOR(self), FALSE);

	animation=NULL;
	animationStarted=FALSE;

	/* Check if an animation exists but only for actors derived from XfdashboardActor */
	if(XFDASHBOARD_IS_ACTOR(self))
	{
		animation=xfdashboard_animation_new(XFDASHBOARD_ACTOR(self), "destroy");
	}

	if(animation &&
		!xfdashboard_animation_is_empty(animation))
	{
		/* Connect signal to destroy actor when animation has ended */
		g_signal_connect_after(animation,
								"animation-done",
								G_CALLBACK(_xfdashboard_actor_on_destroy_animation_done),
								self);

		/* Set CSS pseudo-class that this actor is going to be destroyed */
		xfdashboard_stylable_add_pseudo_class(XFDASHBOARD_STYLABLE(self), "destroying");

		/* Start destroy animation and set flag that animation was
		 * found and started.
		 */
		xfdashboard_animation_run(animation);
		animationStarted=TRUE;

		/* Take extra reference to keep animation alive as it will be
		 * unreferenced when cleaning allocated resources.
		 */
		g_object_ref(animation);
	}
		else
		{
			/* No animation exists so destroy actor immediately */
			clutter_actor_destroy(self);
		}

	/* Clean allocated resources */
	if(animation) g_object_unref(animation);

	/* Return TRUE if animation was found and started or FALSE if no
	 * animation was found and actor as destroyed immediately.
	 */
	return(animationStarted);
}

void xfdashboard_actor_destroy_all_children(ClutterActor *self)
{
	ClutterActorIter		iter;

	g_return_if_fail(CLUTTER_IS_ACTOR(self));

	/* Iterate through children and destroy them */
	g_object_freeze_notify(G_OBJECT(self));

	clutter_actor_iter_init(&iter, self);
	while(clutter_actor_iter_next(&iter, NULL))
	{
		xfdashboard_actor_iter_destroy(&iter);
	}

	g_object_thaw_notify(G_OBJECT(self));
}

gboolean xfdashboard_actor_iter_destroy(ClutterActorIter *self)
{
	/* Do not know currently how to wrap this safely, so just call
	 * clutter_actor_iter_destroy() and return FALSE as no animation
	 * was found (because we did not check ;) ).
	 */
	clutter_actor_iter_destroy(self);

	/* Return TRUE if animation was found and started or FALSE if no
	 * animation was found and actor as destroyed immediately.
	 */
	return(FALSE);
}
