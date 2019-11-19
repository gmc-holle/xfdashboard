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
#include <errno.h>

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
	TAG_DOCUMENT,
	TAG_ANIMATIONS,
	TAG_TRIGGER,
	TAG_TIMELINE,
	TAG_APPLY,
	TAG_PROPERTY
};

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

typedef struct _XfdashboardThemeAnimationParserData			XfdashboardThemeAnimationParserData;
struct _XfdashboardThemeAnimationParserData
{
	XfdashboardThemeAnimation			*self;

	GSList								*specs;

	XfdashboardThemeAnimationSpec		*currentSpec;
	ClutterTimeline						*currentTimeline;
	XfdashboardThemeAnimationTargets	*currentTargets;

	gint								lastLine;
	gint								lastPosition;
	gint								currentLine;
	gint								currentPostition;
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

/* Callback to add each successfully parsed animation specifications to list of
 * known animations of this theme.
 */
static void _xfdashboard_theme_animation_ref_and_add_to_theme(gpointer inData, gpointer inUserData)
{
	XfdashboardThemeAnimation				*self;
	XfdashboardThemeAnimationPrivate		*priv;
	XfdashboardThemeAnimationSpec			*spec;

	g_return_if_fail(XFDASHBOARD_IS_THEME_ANIMATION(inUserData));
	g_return_if_fail(inData);

	self=XFDASHBOARD_THEME_ANIMATION(inUserData);
	priv=self->priv;
	spec=(XfdashboardThemeAnimationSpec*)inData;

	/* Increase reference of specified animation specification and add to list
	 * of known ones.
	 */
	priv->specs=g_slist_prepend(priv->specs, _xfdashboard_theme_animation_spec_ref(spec));
}

/* Check if an animation specification with requested ID exists */
static gboolean _xfdashboard_theme_animation_has_id(XfdashboardThemeAnimation *self,
													XfdashboardThemeAnimationParserData *inParserData,
													const gchar *inID)
{
	XfdashboardThemeAnimationPrivate		*priv;
	GSList									*ids;
	gboolean								hasID;
	XfdashboardThemeAnimationSpec			*spec;

	g_return_val_if_fail(XFDASHBOARD_IS_THEME_ANIMATION(self), TRUE);

	priv=self->priv;
	hasID=FALSE;

	/* Check that ID to lookup is specified */
	g_assert(inID && *inID);

	/* Lookup ID first in currently parsed file if specified */
	if(inParserData)
	{
		for(ids=inParserData->specs; !hasID && ids; ids=g_slist_next(ids))
		{
			spec=(XfdashboardThemeAnimationSpec*)ids->data;
			if(spec && g_strcmp0(spec->id, inID)==0) hasID=TRUE;
		}
	}

	/* If ID was not found in currently parsed effects xml file (if specified)
	 * continue search in already parsed and known effects.
	 */
	if(!hasID)
	{
		for(ids=priv->specs; !hasID && ids; ids=g_slist_next(ids))
		{
			spec=(XfdashboardThemeAnimationSpec*)ids->data;
			if(spec && g_strcmp0(spec->id, inID)==0) hasID=TRUE;
		}
	}

	/* Return lookup result */
	return(hasID);
}

/* Convert string to integer and throw error if conversion failed */
static gboolean _xfdashboard_theme_animation_string_to_gint(const gchar *inNumberString, gint *outNumber, GError **outError)
{
	gint64			convertedNumber;
	gchar			*outNumberStringEnd;

	g_return_val_if_fail(inNumberString && *inNumberString, FALSE);
	g_return_val_if_fail(outNumber, FALSE);
	g_return_val_if_fail(outError==NULL || *outError==NULL, FALSE);

	/* Try to convert string to number */
	convertedNumber=g_ascii_strtoll(inNumberString, &outNumberStringEnd, 10);

	/* Check if invalid base was specified */
	if(errno==EINVAL)
	{
		/* Set error */
		g_set_error(outError,
					XFDASHBOARD_THEME_ANIMATION_ERROR,
					XFDASHBOARD_THEME_ANIMATION_ERROR_ERROR,
					_("Invalid base for conversion"));
		return(FALSE);
	}

	/* Check if integer is out of range */
	if(errno==ERANGE)
	{
		/* Set error */
		g_set_error(outError,
					XFDASHBOARD_THEME_ANIMATION_ERROR,
					XFDASHBOARD_THEME_ANIMATION_ERROR_ERROR,
					_("Integer out of range"));
		return(FALSE);
	}

	/* If converted integer resulted in zero check if end pointer
	 * has moved and does not match start pointer and points to a
	 * NULL byte (as NULL-terminated strings must be provided).
	 */
	if(convertedNumber==0 &&
		(outNumberStringEnd==inNumberString || *outNumberStringEnd!=0))
	{
		/* Set error */
		g_set_error(outError,
					XFDASHBOARD_THEME_ANIMATION_ERROR,
					XFDASHBOARD_THEME_ANIMATION_ERROR_ERROR,
					_("Cannot convert string '%s' to integer"),
					inNumberString);
		return(FALSE);
	}

	/* Set converted number - the integer */
	*outNumber=((gint)convertedNumber);

	/* Return TRUE for successful conversion */
	return(TRUE);
}

/* Helper function to set up GError object in this parser */
static void _xfdashboard_theme_animation_parse_set_error(XfdashboardThemeAnimationParserData *inParserData,
															GMarkupParseContext *inContext,
															GError **outError,
															XfdashboardThemeAnimationErrorEnum inCode,
															const gchar *inFormat,
															...)
{
	GError		*tempError;
	gchar		*message;
	va_list		args;

	/* Get error message */
	va_start(args, inFormat);
	message=g_strdup_vprintf(inFormat, args);
	va_end(args);

	/* Create error object */
	tempError=g_error_new_literal(XFDASHBOARD_THEME_ANIMATION_ERROR, inCode, message);
	if(inParserData)
	{
		g_prefix_error(&tempError,
						_("Error on line %d char %d: "),
						inParserData->lastLine,
						inParserData->lastPosition);
	}

	/* Set error */
	g_propagate_error(outError, tempError);

	/* Release allocated resources */
	g_free(message);
}

/* General callbacks which can be used for any tag */
static void _xfdashboard_theme_animation_parse_general_no_text_nodes(GMarkupParseContext *inContext,
																		const gchar *inText,
																		gsize inTextLength,
																		gpointer inUserData,
																		GError **outError)
{
	XfdashboardThemeAnimationParserData			*data=(XfdashboardThemeAnimationParserData*)inUserData;
	gchar										*realText;

	/* Check if text contains only whitespace. If we find any non-whitespace
	 * in text then set error.
	 */
	realText=g_strstrip(g_strdup(inText));
	if(*realText)
	{
		const GSList	*parents;

		parents=g_markup_parse_context_get_element_stack(inContext);
		if(parents) parents=g_slist_next(parents);

		_xfdashboard_theme_animation_parse_set_error(data,
														inContext,
														outError,
														XFDASHBOARD_THEME_ANIMATION_ERROR_MALFORMED,
														_("Unexpected text node '%s' at tag <%s>"),
														realText,
														parents ? (gchar*)parents->data : "document");
	}
	g_free(realText);
}

/* Determine tag name and ID */
static gint _xfdashboard_theme_animation_get_tag_by_name(const gchar *inTag)
{
	g_return_val_if_fail(inTag && *inTag, -1);

	/* Compare string and return type ID */
	if(g_strcmp0(inTag, "animations")==0) return(TAG_ANIMATIONS);
	if(g_strcmp0(inTag, "trigger")==0) return(TAG_TRIGGER);
	if(g_strcmp0(inTag, "timeline")==0) return(TAG_TIMELINE);
	if(g_strcmp0(inTag, "apply")==0) return(TAG_APPLY);
	if(g_strcmp0(inTag, "property")==0) return(TAG_PROPERTY);

	/* If we get here we do not know tag name and return invalid ID */
	return(-1);
}

static const gchar* _xfdashboard_theme_animation_get_tag_by_id(guint inTagType)
{
	/* Compare ID and return string */
	switch(inTagType)
	{
		case TAG_DOCUMENT:
			return("document");

		case TAG_ANIMATIONS:
			return("animations");

		case TAG_TRIGGER:
			return("trigger");

		case TAG_TIMELINE:
			return("timeline");

		case TAG_APPLY:
			return("apply");

		case TAG_PROPERTY:
			return("property");

		default:
			break;
	}

	/* If we get here we do not know tag name and return NULL */
	return(NULL);
}

/* Parser callbacks for <property> node */
static void _xfdashboard_theme_animation_parse_property_start(GMarkupParseContext *inContext,
																const gchar *inElementName,
																const gchar **inAttributeNames,
																const gchar **inAttributeValues,
																gpointer inUserData,
																GError **outError)
{
	XfdashboardThemeAnimationParserData			*data=(XfdashboardThemeAnimationParserData*)inUserData;
	gint										currentTag=TAG_PROPERTY;
	gint										nextTag;

	/* Update last position for more accurate line and position in error messages */
	data->lastLine=data->currentLine;
	data->lastPosition=data->currentPostition;
	g_markup_parse_context_get_position(inContext, &data->currentLine, &data->currentPostition);

	/* Get tag of next element */
	nextTag=_xfdashboard_theme_animation_get_tag_by_name(inElementName);
	if(nextTag==-1)
	{
		_xfdashboard_theme_animation_parse_set_error(data,
													inContext,
													outError,
													XFDASHBOARD_THEME_ANIMATION_ERROR_MALFORMED,
													_("Unknown tag <%s>"),
													inElementName);
		return;
	}

	/* If we get here the given element name cannot follow this tag */
	_xfdashboard_theme_animation_parse_set_error(data,
													inContext,
													outError,
													XFDASHBOARD_THEME_ANIMATION_ERROR_MALFORMED,
													_("Tag <%s> cannot contain tag <%s>"),
													_xfdashboard_theme_animation_get_tag_by_id(currentTag),
													inElementName);
}

/* Parser callbacks for <apply> node */
static void _xfdashboard_theme_animation_parse_apply_start(GMarkupParseContext *inContext,
																const gchar *inElementName,
																const gchar **inAttributeNames,
																const gchar **inAttributeValues,
																gpointer inUserData,
																GError **outError)
{
	XfdashboardThemeAnimationParserData			*data=(XfdashboardThemeAnimationParserData*)inUserData;
	gint										currentTag=TAG_APPLY;
	gint										nextTag;
	GError										*error=NULL;

	/* Update last position for more accurate line and position in error messages */
	data->lastLine=data->currentLine;
	data->lastPosition=data->currentPostition;
	g_markup_parse_context_get_position(inContext, &data->currentLine, &data->currentPostition);

	/* Get tag of next element */
	nextTag=_xfdashboard_theme_animation_get_tag_by_name(inElementName);
	if(nextTag==-1)
	{
		_xfdashboard_theme_animation_parse_set_error(data,
													inContext,
													outError,
													XFDASHBOARD_THEME_ANIMATION_ERROR_MALFORMED,
													_("Unknown tag <%s>"),
													inElementName);
		return;
	}

	/* Check if element name is <property> and follows expected parent tags:
	 * <apply>
	 */
	if(nextTag==TAG_PROPERTY)
	{
		static GMarkupParser						propertyParser=
													{
														_xfdashboard_theme_animation_parse_property_start,
														NULL,
														_xfdashboard_theme_animation_parse_general_no_text_nodes,
														NULL,
														NULL,
													};

		const gchar									*propertyName=NULL;
		const gchar									*propertyFrom=NULL;
		const gchar									*propertyTo=NULL;
		XfdashboardThemeAnimationTargetsProperty	*property=NULL;

		g_assert(data->currentTargets!=NULL);

		/* Get tag's attributes */
		if(!g_markup_collect_attributes(inElementName,
											inAttributeNames,
											inAttributeValues,
											&error,
											G_MARKUP_COLLECT_STRING,
											"name",
											&propertyName,
											G_MARKUP_COLLECT_STRING | G_MARKUP_COLLECT_OPTIONAL,
											"from",
											&propertyFrom,
											G_MARKUP_COLLECT_STRING | G_MARKUP_COLLECT_OPTIONAL,
											"to",
											&propertyTo,
											G_MARKUP_COLLECT_INVALID))
		{
			g_propagate_error(outError, error);
			return;
		}

		/* Check tag's attributes */
		if(strlen(propertyName)==0)
		{
			_xfdashboard_theme_animation_parse_set_error(data,
															inContext,
															outError,
															XFDASHBOARD_THEME_ANIMATION_ERROR_MALFORMED,
															_("Empty 'name' at tag '%s'"),
															inElementName);
			return;
		}

		if(propertyFrom && strlen(propertyFrom)==0)
		{
			_xfdashboard_theme_animation_parse_set_error(data,
															inContext,
															outError,
															XFDASHBOARD_THEME_ANIMATION_ERROR_MALFORMED,
															_("Empty 'from' at tag '%s'"),
															inElementName);
			return;
		}

		if(propertyTo && strlen(propertyTo)==0)
		{
			_xfdashboard_theme_animation_parse_set_error(data,
															inContext,
															outError,
															XFDASHBOARD_THEME_ANIMATION_ERROR_MALFORMED,
															_("Empty 'to' at tag '%s'"),
															inElementName);
			return;
		}

		/* Create new animation property and add to current targets */
		property=_xfdashboard_theme_animation_targets_property_new(propertyName, propertyFrom, propertyTo);
		_xfdashboard_theme_animation_targets_add_property(data->currentTargets, property);
		_xfdashboard_theme_animation_targets_property_unref(property);

		/* Set up context for tag <apply> */
		g_markup_parse_context_push(inContext, &propertyParser, inUserData);
		return;
	}

	/* If we get here the given element name cannot follow this tag */
	_xfdashboard_theme_animation_parse_set_error(data,
													inContext,
													outError,
													XFDASHBOARD_THEME_ANIMATION_ERROR_MALFORMED,
													_("Tag <%s> cannot contain tag <%s>"),
													_xfdashboard_theme_animation_get_tag_by_id(currentTag),
													inElementName);
}

static void _xfdashboard_theme_animation_parse_apply_end(GMarkupParseContext *inContext,
															const gchar *inElementName,
															gpointer inUserData,
															GError **outError)
{
	/* Restore previous parser context */
	g_markup_parse_context_pop(inContext);
}

/* Parser callbacks for <timeline> node */
static void _xfdashboard_theme_animation_parse_timeline_start(GMarkupParseContext *inContext,
																const gchar *inElementName,
																const gchar **inAttributeNames,
																const gchar **inAttributeValues,
																gpointer inUserData,
																GError **outError)
{
	XfdashboardThemeAnimationParserData			*data=(XfdashboardThemeAnimationParserData*)inUserData;
	gint										currentTag=TAG_TIMELINE;
	gint										nextTag;
	GError										*error=NULL;

	/* Update last position for more accurate line and position in error messages */
	data->lastLine=data->currentLine;
	data->lastPosition=data->currentPostition;
	g_markup_parse_context_get_position(inContext, &data->currentLine, &data->currentPostition);

	/* Get tag of next element */
	nextTag=_xfdashboard_theme_animation_get_tag_by_name(inElementName);
	if(nextTag==-1)
	{
		_xfdashboard_theme_animation_parse_set_error(data,
													inContext,
													outError,
													XFDASHBOARD_THEME_ANIMATION_ERROR_MALFORMED,
													_("Unknown tag <%s>"),
													inElementName);
		return;
	}

	/* Check if element name is <apply> and follows expected parent tags:
	 * <timeline>
	 */
	if(nextTag==TAG_APPLY)
	{
		static GMarkupParser					propertyParser=
												{
													_xfdashboard_theme_animation_parse_apply_start,
													_xfdashboard_theme_animation_parse_apply_end,
													_xfdashboard_theme_animation_parse_general_no_text_nodes,
													NULL,
													NULL,
												};

		const gchar								*applyToText=NULL;
		XfdashboardCssSelector					*applyTo=NULL;
		const gchar								*applyOriginText=NULL;
		gint									applyOrigin;

		g_assert(data->currentTargets==NULL);

		/* Get tag's attributes */
		if(!g_markup_collect_attributes(inElementName,
											inAttributeNames,
											inAttributeValues,
											&error,
											G_MARKUP_COLLECT_STRING | G_MARKUP_COLLECT_OPTIONAL,
											"to",
											&applyToText,
											G_MARKUP_COLLECT_STRING | G_MARKUP_COLLECT_OPTIONAL,
											"origin",
											&applyOriginText,
											G_MARKUP_COLLECT_INVALID))
		{
			g_propagate_error(outError, error);
			return;
		}

		/* Check tag's attributes */
		if(applyToText && strlen(applyToText)==0)
		{
			_xfdashboard_theme_animation_parse_set_error(data,
															inContext,
															outError,
															XFDASHBOARD_THEME_ANIMATION_ERROR_MALFORMED,
															_("Empty 'to' at tag '%s'"),
															inElementName);
			return;
		}

		if(applyOriginText && strlen(applyOriginText)==0)
		{
			_xfdashboard_theme_animation_parse_set_error(data,
															inContext,
															outError,
															XFDASHBOARD_THEME_ANIMATION_ERROR_MALFORMED,
															_("Empty 'origin' at tag '%s'"),
															inElementName);
			return;
		}

		/* Convert tag's attributes' value to usable values */
		applyOrigin=XFDASHBOARD_THEME_ANIMATION_APPLY_TO_ORIGIN_SENDER;
		if(applyOriginText)
		{
			if(g_strcmp0(applyOriginText, "sender")==0) applyOrigin=XFDASHBOARD_THEME_ANIMATION_APPLY_TO_ORIGIN_SENDER;
				else if(g_strcmp0(applyOriginText, "stage")==0) applyOrigin=XFDASHBOARD_THEME_ANIMATION_APPLY_TO_ORIGIN_STAGE;
				else
				{
					_xfdashboard_theme_animation_parse_set_error(data,
																	inContext,
																	outError,
																	XFDASHBOARD_THEME_ANIMATION_ERROR_MALFORMED,
																	_("Invalid value '%s' for 'origin' at tag '%s'"),
																	applyOriginText,
																	inElementName);
					return;
				}
		}

		/* Create new animation timeline with timeline data */
		applyTo=NULL;
		if(applyToText)
		{
			applyTo=xfdashboard_css_selector_new_from_string(applyToText);
		}

		data->currentTargets=_xfdashboard_theme_animation_targets_new(applyTo, applyOrigin, data->currentTimeline);

		if(applyTo) g_object_unref(applyTo);

		/* Set up context for tag <apply> */
		g_markup_parse_context_push(inContext, &propertyParser, inUserData);
		return;
	}

	/* If we get here the given element name cannot follow this tag */
	_xfdashboard_theme_animation_parse_set_error(data,
													inContext,
													outError,
													XFDASHBOARD_THEME_ANIMATION_ERROR_MALFORMED,
													_("Tag <%s> cannot contain tag <%s>"),
													_xfdashboard_theme_animation_get_tag_by_id(currentTag),
													inElementName);
}

static void _xfdashboard_theme_animation_parse_timeline_end(GMarkupParseContext *inContext,
															const gchar *inElementName,
															gpointer inUserData,
															GError **outError)
{
	XfdashboardThemeAnimationParserData			*data=(XfdashboardThemeAnimationParserData*)inUserData;

	g_assert(data->currentSpec);
	g_assert(data->currentTargets);

	/* Add targets to animation specification */
	_xfdashboard_theme_animation_spec_add_targets(data->currentSpec, data->currentTargets);
	_xfdashboard_theme_animation_targets_unref(data->currentTargets);
	data->currentTargets=NULL;

	/* Restore previous parser context */
	g_markup_parse_context_pop(inContext);
}

/* Parser callbacks for <trigger> node */
static void _xfdashboard_theme_animation_parse_trigger_start(GMarkupParseContext *inContext,
																const gchar *inElementName,
																const gchar **inAttributeNames,
																const gchar **inAttributeValues,
																gpointer inUserData,
																GError **outError)
{
	XfdashboardThemeAnimationParserData			*data=(XfdashboardThemeAnimationParserData*)inUserData;
	gint										currentTag=TAG_TRIGGER;
	gint										nextTag;
	GError										*error=NULL;

	/* Update last position for more accurate line and position in error messages */
	data->lastLine=data->currentLine;
	data->lastPosition=data->currentPostition;
	g_markup_parse_context_get_position(inContext, &data->currentLine, &data->currentPostition);

	/* Get tag of next element */
	nextTag=_xfdashboard_theme_animation_get_tag_by_name(inElementName);
	if(nextTag==-1)
	{
		_xfdashboard_theme_animation_parse_set_error(data,
													inContext,
													outError,
													XFDASHBOARD_THEME_ANIMATION_ERROR_MALFORMED,
													_("Unknown tag <%s>"),
													inElementName);
		return;
	}

	/* Check if element name is <timeline> and follows expected parent tags:
	 * <trigger>
	 */
	if(nextTag==TAG_TIMELINE)
	{
		static GMarkupParser					propertyParser=
												{
													_xfdashboard_theme_animation_parse_timeline_start,
													_xfdashboard_theme_animation_parse_timeline_end,
													_xfdashboard_theme_animation_parse_general_no_text_nodes,
													NULL,
													NULL,
												};

		const gchar								*timelineDelayText=NULL;
		const gchar								*timelineDurationText=NULL;
		const gchar								*timelineModeText=NULL;
		const gchar								*timelineRepeatText=NULL;
		gint									timelineDelay;
		gint									timelineDuration;
		gint									timelineMode;
		gint									timelineRepeat;

		g_assert(data->currentTimeline==NULL);

		/* Get tag's attributes */
		if(!g_markup_collect_attributes(inElementName,
											inAttributeNames,
											inAttributeValues,
											&error,
											G_MARKUP_COLLECT_STRING,
											"delay",
											&timelineDelayText,
											G_MARKUP_COLLECT_STRING,
											"duration",
											&timelineDurationText,
											G_MARKUP_COLLECT_STRING,
											"mode",
											&timelineModeText,
											G_MARKUP_COLLECT_STRING | G_MARKUP_COLLECT_OPTIONAL,
											"repeat",
											&timelineRepeatText,
											G_MARKUP_COLLECT_INVALID))
		{
			g_propagate_error(outError, error);
			return;
		}

		/* Check tag's attributes */
		if(!timelineDelayText || strlen(timelineDelayText)==0)
		{
			_xfdashboard_theme_animation_parse_set_error(data,
															inContext,
															outError,
															XFDASHBOARD_THEME_ANIMATION_ERROR_MALFORMED,
															_("Missing or empty delay at tag '%s'"),
															inElementName);
			return;
		}

		if(!timelineDurationText || strlen(timelineDurationText)==0)
		{
			_xfdashboard_theme_animation_parse_set_error(data,
															inContext,
															outError,
															XFDASHBOARD_THEME_ANIMATION_ERROR_MALFORMED,
															_("Missing or empty duration at tag '%s'"),
															inElementName);
			return;
		}

		if(!timelineModeText || strlen(timelineModeText)==0)
		{
			_xfdashboard_theme_animation_parse_set_error(data,
															inContext,
															outError,
															XFDASHBOARD_THEME_ANIMATION_ERROR_MALFORMED,
															_("Missing or empty mode at tag '%s'"),
															inElementName);
			return;
		}

		if(timelineRepeatText && strlen(timelineRepeatText)==0)
		{
			_xfdashboard_theme_animation_parse_set_error(data,
															inContext,
															outError,
															XFDASHBOARD_THEME_ANIMATION_ERROR_MALFORMED,
															_("Empty repeat at tag '%s'"),
															inElementName);
			return;
		}

		/* Convert tag's attributes' value to usable values */
		if(!_xfdashboard_theme_animation_string_to_gint(timelineDelayText, &timelineDelay, &error))
		{
			g_propagate_error(outError, error);
			return;
		}

		if(!_xfdashboard_theme_animation_string_to_gint(timelineDurationText, &timelineDuration, &error))
		{
			g_propagate_error(outError, error);
			return;
		}

		if(!_xfdashboard_theme_animation_string_to_gint(timelineDelayText, &timelineDelay, &error))
		{
			g_propagate_error(outError, error);
			return;
		}

		timelineMode=xfdashboard_get_enum_value_from_nickname(CLUTTER_TYPE_ANIMATION_MODE, timelineModeText);
		if(timelineMode==G_MININT)
		{
			/* Set error */
			g_set_error(outError,
						XFDASHBOARD_THEME_ANIMATION_ERROR,
						XFDASHBOARD_THEME_ANIMATION_ERROR_ERROR,
						_("Invalid mode '%s'"),
						timelineModeText);
			return;
		}

		timelineRepeat=0;
		if(timelineRepeatText &&
			!_xfdashboard_theme_animation_string_to_gint(timelineRepeatText, &timelineRepeat, &error))
		{
			g_propagate_error(outError, error);
			return;
		}

		/* Create new animation timeline with timeline data */
		data->currentTimeline=clutter_timeline_new(timelineDuration);
		clutter_timeline_set_delay(data->currentTimeline, timelineDelay);
		clutter_timeline_set_progress_mode(data->currentTimeline, timelineMode);
		clutter_timeline_set_repeat_count(data->currentTimeline, timelineRepeat);

		/* Set up context for tag <timeline> */
		g_markup_parse_context_push(inContext, &propertyParser, inUserData);
		return;
	}

	/* If we get here the given element name cannot follow this tag */
	_xfdashboard_theme_animation_parse_set_error(data,
													inContext,
													outError,
													XFDASHBOARD_THEME_ANIMATION_ERROR_MALFORMED,
													_("Tag <%s> cannot contain tag <%s>"),
													_xfdashboard_theme_animation_get_tag_by_id(currentTag),
													inElementName);
}

static void _xfdashboard_theme_animation_parse_trigger_end(GMarkupParseContext *inContext,
															const gchar *inElementName,
															gpointer inUserData,
															GError **outError)
{
	XfdashboardThemeAnimationParserData			*data=(XfdashboardThemeAnimationParserData*)inUserData;

	g_assert(data->currentTimeline);

	/* Add targets to animation specification */
	g_object_unref(data->currentTimeline);
	data->currentTimeline=NULL;

	/* Restore previous parser context */
	g_markup_parse_context_pop(inContext);
}

/* Parser callbacks for <animations> node */
static void _xfdashboard_theme_animation_parse_animations_start(GMarkupParseContext *inContext,
																const gchar *inElementName,
																const gchar **inAttributeNames,
																const gchar **inAttributeValues,
																gpointer inUserData,
																GError **outError)
{
	XfdashboardThemeAnimationParserData			*data=(XfdashboardThemeAnimationParserData*)inUserData;
	gint										currentTag=TAG_ANIMATIONS;
	gint										nextTag;
	GError										*error=NULL;

	/* Update last position for more accurate line and position in error messages */
	data->lastLine=data->currentLine;
	data->lastPosition=data->currentPostition;
	g_markup_parse_context_get_position(inContext, &data->currentLine, &data->currentPostition);

	/* Get tag of next element */
	nextTag=_xfdashboard_theme_animation_get_tag_by_name(inElementName);
	if(nextTag==-1)
	{
		_xfdashboard_theme_animation_parse_set_error(data,
													inContext,
													outError,
													XFDASHBOARD_THEME_ANIMATION_ERROR_MALFORMED,
													_("Unknown tag <%s>"),
													inElementName);
		return;
	}

	/* Check if element name is <trigger> and follows expected parent tags:
	 * <animations>
	 */
	if(nextTag==TAG_TRIGGER)
	{
		static GMarkupParser					propertyParser=
												{
													_xfdashboard_theme_animation_parse_trigger_start,
													_xfdashboard_theme_animation_parse_trigger_end,
													_xfdashboard_theme_animation_parse_general_no_text_nodes,
													NULL,
													NULL,
												};

		const gchar								*triggerID=NULL;
		const gchar								*triggerSender=NULL;
		const gchar								*triggerSignal=NULL;
		XfdashboardCssSelector					*selector=NULL;

		g_assert(data->currentSpec==NULL);

		/* Get tag's attributes */
		if(!g_markup_collect_attributes(inElementName,
											inAttributeNames,
											inAttributeValues,
											&error,
											G_MARKUP_COLLECT_STRING,
											"id",
											&triggerID,
											G_MARKUP_COLLECT_STRING,
											"sender",
											&triggerSender,
											G_MARKUP_COLLECT_STRING,
											"signal",
											&triggerSignal,
											G_MARKUP_COLLECT_INVALID))
		{
			g_propagate_error(outError, error);
			return;
		}

		/* Check tag's attributes */
		if(!triggerID || strlen(triggerID)==0)
		{
			_xfdashboard_theme_animation_parse_set_error(data,
															inContext,
															outError,
															XFDASHBOARD_THEME_ANIMATION_ERROR_MALFORMED,
															_("Missing or empty ID at tag '%s'"),
															inElementName);
			return;
		}

		if(!triggerSender || strlen(triggerSender)==0)
		{
			_xfdashboard_theme_animation_parse_set_error(data,
															inContext,
															outError,
															XFDASHBOARD_THEME_ANIMATION_ERROR_MALFORMED,
															_("Missing or empty sender at tag '%s'"),
															inElementName);
			return;
		}

		if(!triggerSignal || strlen(triggerSignal)==0)
		{
			_xfdashboard_theme_animation_parse_set_error(data,
															inContext,
															outError,
															XFDASHBOARD_THEME_ANIMATION_ERROR_MALFORMED,
															_("Missing or empty signal at tag '%s'"),
															inElementName);
			return;
		}

		if(!xfdashboard_is_valid_id(triggerID))
		{
			_xfdashboard_theme_animation_parse_set_error(data,
															inContext,
															outError,
															XFDASHBOARD_THEME_ANIMATION_ERROR_MALFORMED,
															_("Invalid ID '%s' at tag '%s'"),
															triggerID,
															inElementName);
			return;
		}

		if(_xfdashboard_theme_animation_has_id(data->self, data, triggerID))
		{
			_xfdashboard_theme_animation_parse_set_error(data,
															inContext,
															outError,
															XFDASHBOARD_THEME_ANIMATION_ERROR_MALFORMED,
															_("Multiple definition of trigger with ID '%s'"),
															triggerID);
			return;
		}

		/* Create new animation specification with trigger data */
		selector=xfdashboard_css_selector_new_from_string(triggerSender);
		data->currentSpec=_xfdashboard_theme_animation_spec_new(triggerID, selector, triggerSignal);
		g_object_unref(selector);

		/* Set up context for tag <trigger> */
		g_markup_parse_context_push(inContext, &propertyParser, inUserData);
		return;
	}

	/* If we get here the given element name cannot follow this tag */
	_xfdashboard_theme_animation_parse_set_error(data,
													inContext,
													outError,
													XFDASHBOARD_THEME_ANIMATION_ERROR_MALFORMED,
													_("Tag <%s> cannot contain tag <%s>"),
													_xfdashboard_theme_animation_get_tag_by_id(currentTag),
													inElementName);
}

static void _xfdashboard_theme_animation_parse_animations_end(GMarkupParseContext *inContext,
																const gchar *inElementName,
																gpointer inUserData,
																GError **outError)
{
	XfdashboardThemeAnimationParserData			*data=(XfdashboardThemeAnimationParserData*)inUserData;

	g_assert(data->currentSpec);

	/* Add animation specification to list of animations */
	data->specs=g_slist_prepend(data->specs, data->currentSpec);
	data->currentSpec=NULL;

	/* Restore previous parser context */
	g_markup_parse_context_pop(inContext);
}

/* Parser callbacks for document root node */
static void _xfdashboard_theme_animation_parse_document_start(GMarkupParseContext *inContext,
																const gchar *inElementName,
																const gchar **inAttributeNames,
																const gchar **inAttributeValues,
																gpointer inUserData,
																GError **outError)
{
	XfdashboardThemeAnimationParserData			*data=(XfdashboardThemeAnimationParserData*)inUserData;
	gint										currentTag=TAG_DOCUMENT;
	gint										nextTag;
	GError										*error=NULL;

	/* Update last position for more accurate line and position in error messages */
	data->lastLine=data->currentLine;
	data->lastPosition=data->currentPostition;
	g_markup_parse_context_get_position(inContext, &data->currentLine, &data->currentPostition);

	/* Get tag of next element */
	nextTag=_xfdashboard_theme_animation_get_tag_by_name(inElementName);
	if(nextTag==-1)
	{
		_xfdashboard_theme_animation_parse_set_error(data,
													inContext,
													outError,
													XFDASHBOARD_THEME_ANIMATION_ERROR_MALFORMED,
													_("Unknown tag <%s>"),
													inElementName);
		return;
	}

	/* Check if element name is <animations> and follows expected parent tags:
	 * <document>
	 */
	if(nextTag==TAG_ANIMATIONS)
	{
		static GMarkupParser					propertyParser=
												{
													_xfdashboard_theme_animation_parse_animations_start,
													_xfdashboard_theme_animation_parse_animations_end,
													_xfdashboard_theme_animation_parse_general_no_text_nodes,
													NULL,
													NULL,
												};

		/* Get tag's attributes */
		if(!g_markup_collect_attributes(inElementName,
											inAttributeNames,
											inAttributeValues,
											&error,
											G_MARKUP_COLLECT_INVALID,
											NULL))
		{
			g_propagate_error(outError, error);
		}

		/* Set up context for tag <animations> */
		g_markup_parse_context_push(inContext, &propertyParser, inUserData);
		return;
	}

	/* If we get here the given element name cannot follow this tag */
	_xfdashboard_theme_animation_parse_set_error(data,
													inContext,
													outError,
													XFDASHBOARD_THEME_ANIMATION_ERROR_MALFORMED,
													_("Tag <%s> cannot contain tag <%s>"),
													_xfdashboard_theme_animation_get_tag_by_id(currentTag),
													inElementName);
}

static void _xfdashboard_theme_animation_parse_document_end(GMarkupParseContext *inContext,
															const gchar *inElementName,
															gpointer inUserData,
															GError **outError)
{
	/* Restore previous parser context */
	g_markup_parse_context_pop(inContext);
}

/* Parse XML from string */
static gboolean _xfdashboard_theme_animation_parse_xml(XfdashboardThemeAnimation *self,
														const gchar *inPath,
														const gchar *inContents,
														GError **outError)
{
	static GMarkupParser					parser=
											{
												_xfdashboard_theme_animation_parse_document_start,
												_xfdashboard_theme_animation_parse_document_end,
												_xfdashboard_theme_animation_parse_general_no_text_nodes,
												NULL,
												NULL,
											};

	XfdashboardThemeAnimationParserData		*data;
	GMarkupParseContext						*context;
	GError									*error;
	gboolean								success;

	g_return_val_if_fail(XFDASHBOARD_IS_THEME_ANIMATION(self), FALSE);
	g_return_val_if_fail(inPath && *inPath, FALSE);
	g_return_val_if_fail(inContents && *inContents, FALSE);
	g_return_val_if_fail(outError==NULL || *outError==NULL, FALSE);

	error=NULL;
	success=TRUE;

	/* Create and set up parser instance */
	data=g_new0(XfdashboardThemeAnimationParserData, 1);
	if(!data)
	{
		/* Set error */
		g_set_error(outError,
					XFDASHBOARD_THEME_ANIMATION_ERROR,
					XFDASHBOARD_THEME_ANIMATION_ERROR_ERROR,
					_("Could not set up parser data for file %s"),
					inPath);
		return(FALSE);
	}

	context=g_markup_parse_context_new(&parser, 0, data, NULL);
	if(!context)
	{
		/* Set error */
		g_set_error(outError,
					XFDASHBOARD_THEME_ANIMATION_ERROR,
					XFDASHBOARD_THEME_ANIMATION_ERROR_ERROR,
					_("Could not create parser for file %s"),
					inPath);

		g_free(data);
		return(FALSE);
	}

	/* Now the parser and its context is set up and we can now
	 * safely initialize data.
	 */
	data->self=self;
	data->specs=NULL;
	data->currentSpec=NULL;
	data->currentTimeline=NULL;
	data->currentTargets=NULL;
	data->lastLine=1;
	data->lastPosition=1;
	data->currentLine=1;
	data->currentPostition=1;

	/* Parse XML string */
	if(success && !g_markup_parse_context_parse(context, inContents, -1, &error))
	{
		g_propagate_error(outError, error);
		success=FALSE;
	}

	if(success && !g_markup_parse_context_end_parse(context, &error))
	{
		g_propagate_error(outError, error);
		success=FALSE;
	}

	/* Handle collected data if parsing was successful */
	if(success)
	{
		g_slist_foreach(data->specs, (GFunc)_xfdashboard_theme_animation_ref_and_add_to_theme, self);
	}

	/* Clean up resources */
#ifdef DEBUG
	if(!success)
	{
		// TODO: g_slist_foreach(data->specs, (GFunc)_xfdashboard_theme_animation_print_parsed_objects, "Animation specs (this file):");
		// TODO: g_slist_foreach(self->priv->specs, (GFunc)_xfdashboard_theme_animation_print_parsed_objects, "Animation specs (parsed before):");
		XFDASHBOARD_DEBUG(self, THEME,
							"PARSER ERROR: %s",
							(outError && *outError) ? (*outError)->message : "unknown error");
	}
#endif

	g_markup_parse_context_free(context);

	g_slist_free_full(data->specs, (GDestroyNotify)_xfdashboard_theme_animation_spec_unref);
	g_free(data);

	return(success);
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
	gchar								*contents;
	gsize								contentsLength;
	GError								*error;

	g_return_val_if_fail(XFDASHBOARD_IS_THEME_ANIMATION(self), FALSE);
	g_return_val_if_fail(inPath!=NULL && *inPath!=0, FALSE);
	g_return_val_if_fail(outError==NULL || *outError==NULL, FALSE);

	/* Load XML file, parse it and build objects from file */
	error=NULL;
	if(!g_file_get_contents(inPath, &contents, &contentsLength, &error))
	{
		g_propagate_error(outError, error);
		return(FALSE);
	}

	_xfdashboard_theme_animation_parse_xml(self, inPath, contents, &error);
	if(error)
	{
		g_propagate_error(outError, error);
		g_free(contents);
		return(FALSE);
	}
	XFDASHBOARD_DEBUG(self, THEME, "Loaded animation file '%s'", inPath);

	/* Release allocated resources */
	g_free(contents);

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

		/* Return empty animation object as user does not want any animation */
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

		/* Return empty animation object as no matching animation specification was found */
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
