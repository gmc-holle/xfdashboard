/*
 * theme-effects: A theme used for build effects by XML files
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

#include <libxfdashboard/theme-effects.h>

#include <glib/gi18n-lib.h>
#include <glib.h>
#include <gio/gio.h>

#include <libxfdashboard/utils.h>
#include <libxfdashboard/compat.h>
#include <libxfdashboard/debug.h>


/* Define this class in GObject system */
struct _XfdashboardThemeEffectsPrivate
{
	/* Instance related */
	GSList			*effects;
};

G_DEFINE_TYPE_WITH_PRIVATE(XfdashboardThemeEffects,
							xfdashboard_theme_effects,
							G_TYPE_OBJECT)

/* IMPLEMENTATION: Private variables and methods */
enum
{
	TAG_DOCUMENT,
	TAG_EFFECTS,
	TAG_OBJECT,
	TAG_PROPERTY
};

typedef struct _XfdashboardThemeEffectsPropertiesCollectData	XfdashboardThemeEffectsPropertiesCollectData;
struct _XfdashboardThemeEffectsPropertiesCollectData
{
	guint			index;
	guint			maxProperties;
	gchar			**names;
	GValue			*values;
};

typedef struct _XfdashboardThemeEffectsParsedObject				XfdashboardThemeEffectsParsedObject;
struct _XfdashboardThemeEffectsParsedObject
{
	gint								refCount;

	gchar								*id;

	gchar								*className;
	GType								classType;

	GHashTable							*properties;	/* 0, 1 or more entries; hashtable's keys are property
														 * names and their values are string representation of
														 * property's value.
														 */
};

typedef struct _XfdashboardThemeEffectsParserData				XfdashboardThemeEffectsParserData;
struct _XfdashboardThemeEffectsParserData
{
	XfdashboardThemeEffects				*self;

	GSList								*effects;

	gint								lastLine;
	gint								lastPosition;
	gint								currentLine;
	gint								currentPostition;

	gchar								*lastPropertyName;
};

/* Forward declarations */
static void _xfdashboard_theme_effects_parse_set_error(XfdashboardThemeEffectsParserData *inParserData,
														GMarkupParseContext *inContext,
														GError **outError,
														XfdashboardThemeEffectsErrorEnum inCode,
														const gchar *inFormat,
														...) G_GNUC_PRINTF (5, 6);

#ifdef DEBUG
static void _xfdashboard_theme_effects_print_parsed_objects_properties(gpointer inKey,
																		gpointer inValue,
																		gpointer inUserData)
{
	gchar		*key;
	gchar		*value;

	g_return_if_fail(inKey);
	g_return_if_fail(inValue);

	key=(gchar*)inKey;
	value=(gchar*)inValue;

	g_print("        Property '%s'='%s'\n", key, value);
}

static void _xfdashboard_theme_effects_print_parsed_objects(XfdashboardThemeEffectsParsedObject *inData, gpointer inUserData)
{
	const gchar			*prefix;

	g_return_if_fail(inData);
	g_return_if_fail(inUserData);

	prefix=(const gchar*)inUserData;

	g_print("----\n");
	g_print("# %s %p[%s] with id '%s' (ref-count=%d, properties=%u)\n",
				prefix,
				inData, g_type_name(inData->classType),
				inData->id ? inData->id : "<none>",
				inData->refCount,
				inData->properties ? g_hash_table_size(inData->properties) : 0);

	if(inData->properties)
	{
		g_hash_table_foreach(inData->properties, _xfdashboard_theme_effects_print_parsed_objects_properties, NULL);
	}

	g_print("----\n");
}
#endif

/* Helper function to set up GError object in this parser */
static void _xfdashboard_theme_effects_parse_set_error(XfdashboardThemeEffectsParserData *inParserData,
														GMarkupParseContext *inContext,
														GError **outError,
														XfdashboardThemeEffectsErrorEnum inCode,
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
	tempError=g_error_new_literal(XFDASHBOARD_THEME_EFFECTS_ERROR, inCode, message);
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

/* Helper function to create instance of requested type.
 * Returns G_TYPE_INVALID if not found or unavailable.
 */
static GType _xfdashboard_theme_effects_resolve_type_lazy(const gchar *inTypeName)
{
	static GModule		*appModule=NULL;
	GType				(*objectGetTypeFunc)(void);
	GString				*symbolName=g_string_new("");
	char				c, *symbol;
	int					i;
	GType				gtype=G_TYPE_INVALID;

	/* If it is the first call of this function get application as module */
	if(!appModule) appModule=g_module_open(NULL, 0);

	/* Get *_get_type() function for type (in camel-case) */
	for(i=0; inTypeName[i]!='\0'; i++)
	{
		c=inTypeName[i];

		/* Convert type name to lower case and insert an underscore '_'
		 * in front of any upper-case character (before it is converted
		 * to lower-case) but only if it is not the first character of
		 * type name or if the character before this one was also upper-case.
		 */
		if(c==g_ascii_toupper(c) &&
			(i>0 && inTypeName[i-1]!=g_ascii_toupper(inTypeName[i-1])))
		{
			g_string_append_c(symbolName, '_');
		}
		g_string_append_c(symbolName, g_ascii_tolower(c));
	}
	g_string_append(symbolName, "_get_type");

	/* Get pointer to *_get_name() and call it */
	symbol=g_string_free(symbolName, FALSE);

	if(g_module_symbol(appModule, symbol, (gpointer)&objectGetTypeFunc))
	{
		gtype=objectGetTypeFunc();
	}

	g_free (symbol);

	/* Return retrieved GType. Will be G_TYPE_INVALID in case of any error
	 * or if *_get_type() function was unavailable.
	 */
	return(gtype);
}

/* Determine tag name and ID */
static gint _xfdashboard_theme_effects_get_tag_by_name(const gchar *inTag)
{
	g_return_val_if_fail(inTag && *inTag, -1);

	/* Compare string and return type ID */
	if(g_strcmp0(inTag, "effects")==0) return(TAG_EFFECTS);
	if(g_strcmp0(inTag, "object")==0) return(TAG_OBJECT);
	if(g_strcmp0(inTag, "property")==0) return(TAG_PROPERTY);

	/* If we get here we do not know tag name and return invalid ID */
	return(-1);
}

static const gchar* _xfdashboard_theme_effects_get_tag_by_id(guint inTagType)
{
	/* Compare ID and return string */
	switch(inTagType)
	{
		case TAG_DOCUMENT:
			return("document");

		case TAG_EFFECTS:
			return("effects");

		case TAG_OBJECT:
			return("object");

		case TAG_PROPERTY:
			return("property");

		default:
			break;
	}

	/* If we get here we do not know tag name and return NULL */
	return(NULL);
}

/* Check if an effect with requested ID exists */
static gboolean _xfdashboard_theme_effects_has_id(XfdashboardThemeEffects *self,
													XfdashboardThemeEffectsParserData *inParsingEffects,
													const gchar *inID)
{
	XfdashboardThemeEffectsPrivate			*priv;
	GSList									*ids;
	gboolean								hasID;
	XfdashboardThemeEffectsParsedObject		*effect;

	g_return_val_if_fail(XFDASHBOARD_IS_THEME_EFFECTS(self), TRUE);

	priv=self->priv;
	hasID=FALSE;

	/* Check that ID to lookup is specified */
	g_assert(inID && *inID);

	/* Lookup ID first in currently parsed file if specified */
	if(inParsingEffects)
	{
		for(ids=inParsingEffects->effects; !hasID && ids; ids=g_slist_next(ids))
		{
			effect=(XfdashboardThemeEffectsParsedObject*)ids->data;
			if(effect && g_strcmp0(effect->id, inID)==0) hasID=TRUE;
		}
	}

	/* If ID was not found in currently parsed effects xml file (if specified)
	 * continue search in already parsed and known effects.
	 */
	if(!hasID)
	{
		for(ids=priv->effects; !hasID && ids; ids=g_slist_next(ids))
		{
			effect=(XfdashboardThemeEffectsParsedObject*)ids->data;
			if(effect && g_strcmp0(effect->id, inID)==0) hasID=TRUE;
		}
	}

	/* Return lookup result */
	return(hasID);
}

/* Create, destroy, ref and unref object data */
static XfdashboardThemeEffectsParsedObject* _xfdashboard_theme_effects_object_data_new(GMarkupParseContext *inContext,
																						gint inTagType,
																						GError **outError)
{
	XfdashboardThemeEffectsParsedObject		*objectData;

	g_return_val_if_fail(outError && *outError==NULL, NULL);

	/* Create object data */
	objectData=g_new0(XfdashboardThemeEffectsParsedObject, 1);
	if(!objectData)
	{
		_xfdashboard_theme_effects_parse_set_error(NULL,
													inContext,
													outError,
													XFDASHBOARD_THEME_EFFECTS_ERROR_ERROR,
													_("Cannot allocate memory for object data of tag <%s>"),
													_xfdashboard_theme_effects_get_tag_by_id(inTagType));
		return(NULL);
	}
	objectData->refCount=1;
	objectData->classType=G_TYPE_INVALID;
	objectData->properties=g_hash_table_new_full(g_str_hash, g_str_equal, g_free, g_free);

	return(objectData);
}

static void _xfdashboard_theme_effects_object_data_free(XfdashboardThemeEffectsParsedObject *inData)
{
	g_return_if_fail(inData);

#ifdef DEBUG
	if(inData->refCount>1)
	{
		g_critical(_("Freeing effect object parser data at %p with a reference counter of %d greater than one"),
					inData,
					inData->refCount);
	}
#endif

	/* Release allocated resources */
	if(inData->id) g_free(inData->id);
	if(inData->className) g_free(inData->className);
	if(inData->properties) g_hash_table_destroy(inData->properties);
	g_free(inData);
}

static XfdashboardThemeEffectsParsedObject* _xfdashboard_theme_effects_object_data_ref(XfdashboardThemeEffectsParsedObject *inData)
{
	g_return_val_if_fail(inData, NULL);

	inData->refCount++;
	return(inData);
}

static void _xfdashboard_theme_effects_object_data_unref(XfdashboardThemeEffectsParsedObject *inData)
{
	g_return_if_fail(inData);

	if(inData->refCount==1) _xfdashboard_theme_effects_object_data_free(inData);
		else inData->refCount--;
}

static void _xfdashboard_theme_effects_object_data_free_foreach_callback(gpointer inData, gpointer inUserData)
{
	XfdashboardThemeEffectsParsedObject		*data;

	g_return_if_fail(inData);

	data=(XfdashboardThemeEffectsParsedObject*)inData;

	/* Free tag data */
	_xfdashboard_theme_effects_object_data_free(data);
}

/* Callback to add each successfully parsed effect object
 * to list of known effects of this theme.
 */
static void _xfdashboard_theme_effects_add_effects_reference(gpointer inData, gpointer inUserData)
{
	XfdashboardThemeEffects					*self;
	XfdashboardThemeEffectsPrivate			*priv;
	XfdashboardThemeEffectsParsedObject		*data;

	g_return_if_fail(XFDASHBOARD_IS_THEME_EFFECTS(inUserData));
	g_return_if_fail(inData);

	self=XFDASHBOARD_THEME_EFFECTS(inUserData);
	priv=self->priv;
	data=(XfdashboardThemeEffectsParsedObject*)inData;

	/* Increase reference of specified effect and add to list of known ones */
	priv->effects=g_slist_prepend(priv->effects, _xfdashboard_theme_effects_object_data_ref(data));
}

/* Create object and set up all properties */
static void _xfdashboard_theme_effects_create_object_collect_properties(gpointer inKey,
																		gpointer inValue,
																		gpointer inUserData)
{
	const gchar										*name;
	const gchar										*value;
	XfdashboardThemeEffectsPropertiesCollectData	*data;

	g_return_if_fail(inKey);
	g_return_if_fail(inValue);
	g_return_if_fail(inUserData);

	name=(const gchar*)inKey;
	value=(const gchar*)inValue;
	data=(XfdashboardThemeEffectsPropertiesCollectData*)inUserData;

	/* Add property parameter to array */
	data->names[data->index]=g_strdup(name);

	g_value_init(&data->values[data->index], G_TYPE_STRING);
	g_value_set_string(&data->values[data->index], value);

	/* Increase pointer to next parameter in array */
	data->index++;
}

static ClutterEffect* _xfdashboard_theme_effects_create_object(XfdashboardThemeEffectsParsedObject *inObjectData)
{
	XfdashboardThemeEffectsPropertiesCollectData		collectData;
	GObject												*object;

	/* Collect all properties as array */
	collectData.index=0;
	collectData.names=NULL;
	collectData.values=NULL;

	collectData.maxProperties=g_hash_table_size(inObjectData->properties);
	if(collectData.maxProperties>0)
	{
		collectData.names=g_new0(gchar*, collectData.maxProperties);
		collectData.values=g_new0(GValue, collectData.maxProperties);
		g_hash_table_foreach(inObjectData->properties, _xfdashboard_theme_effects_create_object_collect_properties, &collectData);
	}

	/* Create instance of object type and before handling error or success
	 * of creation release allocated resources for properties as they are
	 * not needed anymore.
	 */
	object=g_object_new_with_properties(inObjectData->classType,
										collectData.maxProperties,
										(const gchar **)collectData.names,
										(const GValue *)collectData.values);

	for(collectData.index=0; collectData.index<collectData.maxProperties; collectData.index++)
	{
		g_free(collectData.names[collectData.index]);
		g_value_unset(&collectData.values[collectData.index]);
	}
	g_free(collectData.names);
	g_free(collectData.values);

	if(!object)
	{
		XFDASHBOARD_DEBUG(NULL, THEME,
							"Failed to create object of type %s with %d properties to set",
							g_type_name(inObjectData->classType),
							collectData.maxProperties);

		/* Return NULL indicating error */
		return(NULL);
	}

	/* Check if created object is really an effect */
	if(!CLUTTER_IS_EFFECT(object))
	{
		g_warning(_("Object of type %s is not derived from %s"),
					g_type_name(inObjectData->classType),
					g_type_name(CLUTTER_TYPE_EFFECT));

		/* Destroy newly created object */
		g_object_unref(object);

		return(NULL);
	}

	/* Set name of effect to ID */
	clutter_actor_meta_set_name(CLUTTER_ACTOR_META(object), inObjectData->id);

	/* Return created object */
	return(CLUTTER_EFFECT(object));
}

/* General callbacks which can be used for any tag */
static void _xfdashboard_theme_effects_parse_general_no_text_nodes(GMarkupParseContext *inContext,
																	const gchar *inText,
																	gsize inTextLength,
																	gpointer inUserData,
																	GError **outError)
{
	XfdashboardThemeEffectsParserData			*data=(XfdashboardThemeEffectsParserData*)inUserData;
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

		_xfdashboard_theme_effects_parse_set_error(data,
													inContext,
													outError,
													XFDASHBOARD_THEME_EFFECTS_ERROR_MALFORMED,
													_("Unexpected text node '%s' at tag <%s>"),
													realText,
													parents ? (gchar*)parents->data : "document");
	}
	g_free(realText);
}

/* Parser callbacks for <property> node */
static void _xfdashboard_theme_effects_parse_property_start(GMarkupParseContext *inContext,
															const gchar *inElementName,
															const gchar **inAttributeNames,
															const gchar **inAttributeValues,
															gpointer inUserData,
															GError **outError)
{
	XfdashboardThemeEffectsParserData			*data=(XfdashboardThemeEffectsParserData*)inUserData;
	gint										currentTag=TAG_PROPERTY;
	gint										nextTag;

	/* Update last position for more accurate line and position in error messages */
	data->lastLine=data->currentLine;
	data->lastPosition=data->currentPostition;
	g_markup_parse_context_get_position(inContext, &data->currentLine, &data->currentPostition);

	/* Get tag of next element */
	nextTag=_xfdashboard_theme_effects_get_tag_by_name(inElementName);
	if(nextTag==-1)
	{
		_xfdashboard_theme_effects_parse_set_error(data,
													inContext,
													outError,
													XFDASHBOARD_THEME_EFFECTS_ERROR_MALFORMED,
													_("Unknown tag <%s>"),
													inElementName);
		return;
	}

	/* If we get here the given element name cannot follow this tag */
	_xfdashboard_theme_effects_parse_set_error(data,
												inContext,
												outError,
												XFDASHBOARD_THEME_EFFECTS_ERROR_MALFORMED,
												_("Tag <%s> cannot contain tag <%s>"),
												_xfdashboard_theme_effects_get_tag_by_id(currentTag),
												inElementName);
}

static void _xfdashboard_theme_effects_parse_property_text_node(GMarkupParseContext *inContext,
																const gchar *inText,
																gsize inTextLength,
																gpointer inUserData,
																GError **outError)
{
	XfdashboardThemeEffectsParserData		*data=(XfdashboardThemeEffectsParserData*)inUserData;
	XfdashboardThemeEffectsParsedObject		*objectData;

	objectData=NULL;

	/* Check for property name */
	if(!data->lastPropertyName)
	{
		_xfdashboard_theme_effects_parse_set_error(data,
													inContext,
													outError,
													XFDASHBOARD_THEME_EFFECTS_ERROR_MALFORMED,
													_("Missing property name to set value for"));
		return;
	}

	/* Get object data */
	if(data->effects) objectData=(XfdashboardThemeEffectsParsedObject*)(data->effects->data);
	if(!objectData)
	{
		_xfdashboard_theme_effects_parse_set_error(data,
													inContext,
													outError,
													XFDASHBOARD_THEME_EFFECTS_ERROR_MALFORMED,
													_("Missing object data to set value of property '%s'"),
													data->lastPropertyName);
		return;
	}

	/* Store value for property */
	g_hash_table_insert(objectData->properties, g_strdup(data->lastPropertyName), g_strdup(inText));
	XFDASHBOARD_DEBUG(data->self, THEME,
						"Setting property '%s' to value '%s' at object with id '%s' of type %s",
						data->lastPropertyName,
						inText,
						objectData->id, g_type_name(objectData->classType));

	/* The property's value is set now so do not remember its name anymore */
	g_free(data->lastPropertyName);
	data->lastPropertyName=NULL;
}

/* Parser callbacks for <object> node */
static void _xfdashboard_theme_effects_parse_object_start(GMarkupParseContext *inContext,
															const gchar *inElementName,
															const gchar **inAttributeNames,
															const gchar **inAttributeValues,
															gpointer inUserData,
															GError **outError)
{
	XfdashboardThemeEffectsParserData			*data=(XfdashboardThemeEffectsParserData*)inUserData;
	gint										currentTag=TAG_OBJECT;
	gint										nextTag;
	GError										*error=NULL;
	XfdashboardThemeEffectsParsedObject			*objectData=NULL;

	/* Update last position for more accurate line and position in error messages */
	data->lastLine=data->currentLine;
	data->lastPosition=data->currentPostition;
	g_markup_parse_context_get_position(inContext, &data->currentLine, &data->currentPostition);

	/* Get tag of next element */
	nextTag=_xfdashboard_theme_effects_get_tag_by_name(inElementName);
	if(nextTag==-1)
	{
		_xfdashboard_theme_effects_parse_set_error(data,
													inContext,
													outError,
													XFDASHBOARD_THEME_EFFECTS_ERROR_MALFORMED,
													_("Unknown tag <%s>"),
													inElementName);
		return;
	}

	/* Get object data */
	if(data->effects) objectData=(XfdashboardThemeEffectsParsedObject*)(data->effects->data);
	if(!objectData)
	{
		_xfdashboard_theme_effects_parse_set_error(data,
													inContext,
													outError,
													XFDASHBOARD_THEME_EFFECTS_ERROR_MALFORMED,
													_("Missing parser data for <%s> tag"),
													inElementName);
		return;
	}

	/* Check if element name is <property> and follows expected parent tags:
	 * <effects>
	 */
	if(nextTag==TAG_PROPERTY)
	{
		static GMarkupParser					propertyParser=
												{
													_xfdashboard_theme_effects_parse_property_start,
													NULL,
													_xfdashboard_theme_effects_parse_property_text_node,
													NULL,
													NULL,
												};
		gchar									*name=NULL;

		/* Get tag's attributes */
		if(!g_markup_collect_attributes(inElementName,
											inAttributeNames,
											inAttributeValues,
											&error,
											G_MARKUP_COLLECT_STRDUP,
											"name",
											&name,
											G_MARKUP_COLLECT_INVALID))
		{
			g_propagate_error(outError, error);
			if(name) g_free(name);
			return;
		}

		/* Check tag's attributes */
		if(g_hash_table_lookup_extended(objectData->properties, name, NULL, NULL))
		{
			_xfdashboard_theme_effects_parse_set_error(data,
														inContext,
														outError,
														XFDASHBOARD_THEME_EFFECTS_ERROR_MALFORMED,
														_("Multiple definition of property '%s' at object with ID '%s'"),
														name,
														objectData->id);
			if(name) g_free(name);
			return;
		}

		/* Remember property name for text node parsing of <property> */
		data->lastPropertyName=g_strdup(name);
		g_free(name);

		/* Set up context for tag <property> */
		g_markup_parse_context_push(inContext, &propertyParser, inUserData);
		return;
	}

	/* If we get here the given element name cannot follow this tag */
	_xfdashboard_theme_effects_parse_set_error(data,
												inContext,
												outError,
												XFDASHBOARD_THEME_EFFECTS_ERROR_MALFORMED,
												_("Tag <%s> cannot contain tag <%s>"),
												_xfdashboard_theme_effects_get_tag_by_id(currentTag),
												inElementName);
}

static void _xfdashboard_theme_effects_parse_object_end(GMarkupParseContext *inContext,
															const gchar *inElementName,
															gpointer inUserData,
															GError **outError)
{
	XfdashboardThemeEffectsParserData			*data=(XfdashboardThemeEffectsParserData*)inUserData;

	/* Check if parser is still in valid state */
	if(data->lastPropertyName)
	{
		XfdashboardThemeEffectsParsedObject		*objectData;

		/* Get object data */
		if(!data->effects)
		{
			_xfdashboard_theme_effects_parse_set_error(data,
														inContext,
														outError,
														XFDASHBOARD_THEME_EFFECTS_ERROR_MALFORMED,
														_("Missing parser data for <%s> tag"),
														inElementName);
		}
		objectData=(XfdashboardThemeEffectsParsedObject*)(data->effects->data);

		/* Add property name with empty value to hash-table if last known
		 * property name parsed was not set. We assume that no text node
		 * was defined in XML that means that text node callback function
		 * did not add a value with property name as key to properties.
		 * But we have to check this to ensure a valid state.
		 */
		if(g_hash_table_lookup_extended(objectData->properties, data->lastPropertyName, NULL, NULL))
		{
			_xfdashboard_theme_effects_parse_set_error(data,
														inContext,
														outError,
														XFDASHBOARD_THEME_EFFECTS_ERROR_MALFORMED,
														_("Invalid state of parser after parsing property '%s' at object with id '%s' of type %s"),
														data->lastPropertyName,
														objectData->id,
														g_type_name(objectData->classType));

			/* Restore previous parser context */
			g_markup_parse_context_pop(inContext);

			return;
		}

		g_hash_table_insert(objectData->properties, g_strdup(data->lastPropertyName), "");
		XFDASHBOARD_DEBUG(data->self, THEME,
							"Adding property '%s' with empty value to object with id '%s' of type %s",
							data->lastPropertyName,
							objectData->id,
							g_type_name(objectData->classType));
		g_free(data->lastPropertyName);
		data->lastPropertyName=NULL;
	}

	/* Restore previous parser context */
	g_markup_parse_context_pop(inContext);
}

/* Parser callbacks for <effects> node */
static void _xfdashboard_theme_effects_parse_effects_start(GMarkupParseContext *inContext,
															const gchar *inElementName,
															const gchar **inAttributeNames,
															const gchar **inAttributeValues,
															gpointer inUserData,
															GError **outError)
{
	XfdashboardThemeEffectsParserData			*data=(XfdashboardThemeEffectsParserData*)inUserData;
	gint										currentTag=TAG_EFFECTS;
	gint										nextTag;
	GError										*error=NULL;

	/* Update last position for more accurate line and position in error messages */
	data->lastLine=data->currentLine;
	data->lastPosition=data->currentPostition;
	g_markup_parse_context_get_position(inContext, &data->currentLine, &data->currentPostition);

	/* Get tag of next element */
	nextTag=_xfdashboard_theme_effects_get_tag_by_name(inElementName);
	if(nextTag==-1)
	{
		_xfdashboard_theme_effects_parse_set_error(data,
													inContext,
													outError,
													XFDASHBOARD_THEME_EFFECTS_ERROR_MALFORMED,
													_("Unknown tag <%s>"),
													inElementName);
		return;
	}

	/* Check if element name is <object> and follows expected parent tags:
	 * <effects>
	 */
	if(nextTag==TAG_OBJECT)
	{
		static GMarkupParser					propertyParser=
												{
													_xfdashboard_theme_effects_parse_object_start,
													_xfdashboard_theme_effects_parse_object_end,
													_xfdashboard_theme_effects_parse_general_no_text_nodes,
													NULL,
													NULL,
												};
		XfdashboardThemeEffectsParsedObject		*objectData;
		GType									expectedClassType;

		/* Create tag data */
		objectData=_xfdashboard_theme_effects_object_data_new(inContext, nextTag, &error);
		if(!objectData)
		{
			g_propagate_error(outError, error);
			return;
		}

		/* Get tag's attributes */
		if(!g_markup_collect_attributes(inElementName,
											inAttributeNames,
											inAttributeValues,
											&error,
											G_MARKUP_COLLECT_STRDUP,
											"id",
											&objectData->id,
											G_MARKUP_COLLECT_STRDUP,
											"class",
											&objectData->className,
											G_MARKUP_COLLECT_INVALID))
		{
			g_propagate_error(outError, error);
			_xfdashboard_theme_effects_object_data_unref(objectData);
			return;
		}

		/* Check tag's attributes */
		if(strlen(objectData->id)==0)
		{
			_xfdashboard_theme_effects_parse_set_error(data,
														inContext,
														outError,
														XFDASHBOARD_THEME_EFFECTS_ERROR_MALFORMED,
														_("Empty ID at tag '%s'"),
														inElementName);
			_xfdashboard_theme_effects_object_data_unref(objectData);
			return;
		}

		if(!xfdashboard_is_valid_id(objectData->id))
		{
			_xfdashboard_theme_effects_parse_set_error(data,
														inContext,
														outError,
														XFDASHBOARD_THEME_EFFECTS_ERROR_MALFORMED,
														_("Invalid ID '%s' at tag '%s'"),
														objectData->id,
														inElementName);
			_xfdashboard_theme_effects_object_data_unref(objectData);
			return;
		}

		if(_xfdashboard_theme_effects_has_id(data->self, data, objectData->id))
		{
			_xfdashboard_theme_effects_parse_set_error(data,
														inContext,
														outError,
														XFDASHBOARD_THEME_EFFECTS_ERROR_MALFORMED,
														_("Multiple definition of effect with id '%s'"),
														objectData->id);
			_xfdashboard_theme_effects_object_data_unref(objectData);
			return;
		}

		objectData->classType=_xfdashboard_theme_effects_resolve_type_lazy(objectData->className);
		if(objectData->classType==G_TYPE_INVALID)
		{
			_xfdashboard_theme_effects_parse_set_error(data,
														inContext,
														outError,
														XFDASHBOARD_THEME_EFFECTS_ERROR_MALFORMED,
														_("Unknown object class %s for tag '%s'"),
														objectData->className,
														inElementName);
			_xfdashboard_theme_effects_object_data_unref(objectData);
			return;
		}

		expectedClassType=CLUTTER_TYPE_EFFECT;
		if(!g_type_is_a(objectData->classType, expectedClassType))
		{
			_xfdashboard_theme_effects_parse_set_error(data,
														inContext,
														outError,
														XFDASHBOARD_THEME_EFFECTS_ERROR_MALFORMED,
														_("Invalid class %s in object for parent tag <%s> - expecting class derived from %s"),
														objectData->className,
														_xfdashboard_theme_effects_get_tag_by_id(currentTag),
														g_type_name(expectedClassType));
			_xfdashboard_theme_effects_object_data_unref(objectData);
			return;
		}

		/* Add new effect object to list of effect but to the beginning of list
		 * for faster adding and accessing this new effect object in futher parsing
		 */
		data->effects=g_slist_prepend(data->effects, objectData);

		/* Set up context for tag <object> */
		g_markup_parse_context_push(inContext, &propertyParser, inUserData);
		return;
	}

	/* If we get here the given element name cannot follow this tag */
	_xfdashboard_theme_effects_parse_set_error(data,
												inContext,
												outError,
												XFDASHBOARD_THEME_EFFECTS_ERROR_MALFORMED,
												_("Tag <%s> cannot contain tag <%s>"),
												_xfdashboard_theme_effects_get_tag_by_id(currentTag),
												inElementName);
}

static void _xfdashboard_theme_effects_parse_effects_end(GMarkupParseContext *inContext,
															const gchar *inElementName,
															gpointer inUserData,
															GError **outError)
{
	/* Restore previous parser context */
	g_markup_parse_context_pop(inContext);
}

/* Parser callbacks for document root node */
static void _xfdashboard_theme_effects_parse_document_start(GMarkupParseContext *inContext,
															const gchar *inElementName,
															const gchar **inAttributeNames,
															const gchar **inAttributeValues,
															gpointer inUserData,
															GError **outError)
{
	XfdashboardThemeEffectsParserData			*data=(XfdashboardThemeEffectsParserData*)inUserData;
	gint										currentTag=TAG_DOCUMENT;
	gint										nextTag;
	GError										*error=NULL;

	/* Update last position for more accurate line and position in error messages */
	data->lastLine=data->currentLine;
	data->lastPosition=data->currentPostition;
	g_markup_parse_context_get_position(inContext, &data->currentLine, &data->currentPostition);

	/* Get tag of next element */
	nextTag=_xfdashboard_theme_effects_get_tag_by_name(inElementName);
	if(nextTag==-1)
	{
		_xfdashboard_theme_effects_parse_set_error(data,
													inContext,
													outError,
													XFDASHBOARD_THEME_EFFECTS_ERROR_MALFORMED,
													_("Unknown tag <%s>"),
													inElementName);
		return;
	}

	/* Check if element name is <effects> and follows expected parent tags:
	 * <document>
	 */
	if(nextTag==TAG_EFFECTS)
	{
		static GMarkupParser					propertyParser=
												{
													_xfdashboard_theme_effects_parse_effects_start,
													_xfdashboard_theme_effects_parse_effects_end,
													_xfdashboard_theme_effects_parse_general_no_text_nodes,
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

		/* Set up context for tag <effects> */
		g_markup_parse_context_push(inContext, &propertyParser, inUserData);
		return;
	}

	/* If we get here the given element name cannot follow this tag */
	_xfdashboard_theme_effects_parse_set_error(data,
												inContext,
												outError,
												XFDASHBOARD_THEME_EFFECTS_ERROR_MALFORMED,
												_("Tag <%s> cannot contain tag <%s>"),
												_xfdashboard_theme_effects_get_tag_by_id(currentTag),
												inElementName);
}

static void _xfdashboard_theme_effects_parse_document_end(GMarkupParseContext *inContext,
															const gchar *inElementName,
															gpointer inUserData,
															GError **outError)
{
	/* Restore previous parser context */
	g_markup_parse_context_pop(inContext);
}

/* Parse XML from string */
static gboolean _xfdashboard_theme_effects_parse_xml(XfdashboardThemeEffects *self,
														const gchar *inPath,
														const gchar *inContents,
														GError **outError)
{
	static GMarkupParser				parser=
										{
											_xfdashboard_theme_effects_parse_document_start,
											_xfdashboard_theme_effects_parse_document_end,
											_xfdashboard_theme_effects_parse_general_no_text_nodes,
											NULL,
											NULL,
										};

	XfdashboardThemeEffectsParserData	*data;
	GMarkupParseContext					*context;
	GError								*error;
	gboolean							success;

	g_return_val_if_fail(XFDASHBOARD_IS_THEME_EFFECTS(self), FALSE);
	g_return_val_if_fail(inPath && *inPath, FALSE);
	g_return_val_if_fail(inContents && *inContents, FALSE);
	g_return_val_if_fail(outError==NULL || *outError==NULL, FALSE);

	error=NULL;
	success=TRUE;

	/* Create and set up parser instance */
	data=g_new0(XfdashboardThemeEffectsParserData, 1);
	if(!data)
	{
		/* Set error */
		g_set_error(outError,
					XFDASHBOARD_THEME_EFFECTS_ERROR,
					XFDASHBOARD_THEME_EFFECTS_ERROR_ERROR,
					_("Could not set up parser data for file %s"),
					inPath);
		return(FALSE);
	}

	context=g_markup_parse_context_new(&parser, 0, data, NULL);
	if(!context)
	{
		/* Set error */
		g_set_error(outError,
					XFDASHBOARD_THEME_EFFECTS_ERROR,
					XFDASHBOARD_THEME_EFFECTS_ERROR_ERROR,
					_("Could not create parser for file %s"),
					inPath);

		g_free(data);
		return(FALSE);
	}

	/* Now the parser and its context is set up and we can now
	 * safely initialize data.
	 */
	data->self=self;
	data->effects=NULL;
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
		g_slist_foreach(data->effects, (GFunc)_xfdashboard_theme_effects_add_effects_reference, self);
	}

	/* Clean up resources */
#ifdef DEBUG
	if(!success)
	{
		g_slist_foreach(data->effects, (GFunc)_xfdashboard_theme_effects_print_parsed_objects, "Effects (this file):");
		g_slist_foreach(self->priv->effects, (GFunc)_xfdashboard_theme_effects_print_parsed_objects, "Effects (parsed before):");
		XFDASHBOARD_DEBUG(self, THEME,
							"PARSER ERROR: %s",
							(outError && *outError) ? (*outError)->message : "unknown error");
	}
#endif

	g_markup_parse_context_free(context);

	g_slist_free_full(data->effects, (GDestroyNotify)_xfdashboard_theme_effects_object_data_unref);
	if(data->lastPropertyName) g_free(data->lastPropertyName);
	g_free(data);

	return(success);
}

/* IMPLEMENTATION: GObject */

/* Dispose this object */
static void _xfdashboard_theme_effects_dispose(GObject *inObject)
{
	XfdashboardThemeEffects				*self=XFDASHBOARD_THEME_EFFECTS(inObject);
	XfdashboardThemeEffectsPrivate		*priv=self->priv;

	/* Release allocated resources */
	if(priv->effects)
	{
		g_slist_foreach(priv->effects, _xfdashboard_theme_effects_object_data_free_foreach_callback, NULL);
		g_slist_free(priv->effects);
		priv->effects=NULL;
	}

	/* Call parent's class dispose method */
	G_OBJECT_CLASS(xfdashboard_theme_effects_parent_class)->dispose(inObject);
}

/* Class initialization
 * Override functions in parent classes and define properties
 * and signals
 */
void xfdashboard_theme_effects_class_init(XfdashboardThemeEffectsClass *klass)
{
	GObjectClass		*gobjectClass=G_OBJECT_CLASS(klass);

	/* Override functions */
	gobjectClass->dispose=_xfdashboard_theme_effects_dispose;
}

/* Object initialization
 * Create private structure and set up default values
 */
void xfdashboard_theme_effects_init(XfdashboardThemeEffects *self)
{
	XfdashboardThemeEffectsPrivate		*priv;

	priv=self->priv=xfdashboard_theme_effects_get_instance_private(self);

	/* Set default values */
	priv->effects=NULL;
}

/* IMPLEMENTATION: Errors */

GQuark xfdashboard_theme_effects_error_quark(void)
{
	return(g_quark_from_static_string("xfdashboard-theme-effects-error-quark"));
}

/* IMPLEMENTATION: Public API */

/* Create new instance */
XfdashboardThemeEffects* xfdashboard_theme_effects_new(void)
{
	return(XFDASHBOARD_THEME_EFFECTS(g_object_new(XFDASHBOARD_TYPE_THEME_EFFECTS, NULL)));
}

/* Load a XML file into theme */
gboolean xfdashboard_theme_effects_add_file(XfdashboardThemeEffects *self,
											const gchar *inPath,
											GError **outError)
{
	gchar								*contents;
	gsize								contentsLength;
	GError								*error;

	g_return_val_if_fail(XFDASHBOARD_IS_THEME_EFFECTS(self), FALSE);
	g_return_val_if_fail(inPath!=NULL && *inPath!=0, FALSE);
	g_return_val_if_fail(outError==NULL || *outError==NULL, FALSE);

	/* Load XML file, parse it and build objects from file */
	error=NULL;
	if(!g_file_get_contents(inPath, &contents, &contentsLength, &error))
	{
		g_propagate_error(outError, error);
		return(FALSE);
	}

	_xfdashboard_theme_effects_parse_xml(self, inPath, contents, &error);
	if(error)
	{
		g_propagate_error(outError, error);
		g_free(contents);
		return(FALSE);
	}

	/* Release allocated resources */
	g_free(contents);

	/* If we get here loading and parsing XML file was successful
	 * so return TRUE here
	 */
	return(TRUE);
}

/* Create requested effect */
ClutterEffect* xfdashboard_theme_effects_create_effect(XfdashboardThemeEffects *self,
														const gchar *inID)
{
	XfdashboardThemeEffectsPrivate			*priv;
	GSList									*entry;
	XfdashboardThemeEffectsParsedObject		*objectData;
	ClutterEffect							*effect;

	g_return_val_if_fail(XFDASHBOARD_IS_THEME_EFFECTS(self), FALSE);
	g_return_val_if_fail(inID && *inID, FALSE);

	priv=self->priv;

	/* Lookup object data of effect by its ID which should be created */
	entry=priv->effects;
	while(entry)
	{
		objectData=(XfdashboardThemeEffectsParsedObject*)entry->data;

		/* If ID matches requested one create object */
		if(g_strcmp0(objectData->id, inID)==0)
		{
			/* Create object */
			effect=_xfdashboard_theme_effects_create_object(objectData);
			return(effect);
		}

		/* Continue with next effect */
		entry=g_slist_next(entry);
	}

	/* If we get here we did not find an object with requested ID */
	g_warning(_("Could not find effect with ID '%s'"), inID);

	return(NULL);
}
