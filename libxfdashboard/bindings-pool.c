/*
 * bindings: Customizable keyboard and pointer bindings for focusable actors
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

#include <libxfdashboard/bindings-pool.h>

#include <glib-object.h>
#include <glib/gi18n-lib.h>
#include <gdk/gdk.h>
#include <gdk/gdkkeysyms.h>

#include <libxfdashboard/utils.h>
#include <libxfdashboard/compat.h>
#include <libxfdashboard/debug.h>


/* Define this class in GObject system */
struct _XfdashboardBindingsPoolPrivate
{
	/* Instance related */
	GHashTable		*bindings;
};

G_DEFINE_TYPE_WITH_PRIVATE(XfdashboardBindingsPool,
							xfdashboard_bindings_pool,
							G_TYPE_OBJECT)

/* IMPLEMENTATION: Private variables and methods */
enum
{
	TAG_DOCUMENT,
	TAG_BINDINGS_POOL,
	TAG_KEY
};

typedef struct _XfdashboardBindingsPoolParserData	XfdashboardBindingsPoolParserData;
struct _XfdashboardBindingsPoolParserData
{
	XfdashboardBindingsPool		*self;

	GHashTable					*bindings;

	gint						lastLine;
	gint						lastPosition;
	gint						currentLine;
	gint						currentPostition;

	XfdashboardBinding			*lastBinding;
};

typedef struct _XfdashboardBindingsPoolModifierMap	XfdashboardBindingsPoolModifierMap;
struct _XfdashboardBindingsPoolModifierMap
{
	const gchar							*name;
	ClutterModifierType					modifier;
};

/* Forward declarations */
static void _xfdashboard_bindings_pool_parse_set_error(XfdashboardBindingsPoolParserData *inParserData,
														GMarkupParseContext *inContext,
														GError **outError,
														XfdashboardBindingsPoolErrorEnum inCode,
														const gchar *inFormat,
														...) G_GNUC_PRINTF (5, 6);

/* Single instance of bindings */
static XfdashboardBindingsPool*				_xfdashboard_bindings_pool=NULL;

/* Modifier map for conversion from string to integer used by parser */
static XfdashboardBindingsPoolModifierMap	_xfdashboard_bindings_pool_modifier_map[]=
											{
												{ "<Shift>", CLUTTER_SHIFT_MASK },
												{ "<Ctrl>", CLUTTER_CONTROL_MASK },
												{ "<Control>", CLUTTER_CONTROL_MASK},
												{ "<Alt>", CLUTTER_MOD1_MASK },
												{ "<Mod1>", CLUTTER_MOD1_MASK },
												{ "<Mod2>", CLUTTER_MOD2_MASK },
												{ "<Mod3>", CLUTTER_MOD3_MASK },
												{ "<Mod4>", CLUTTER_MOD4_MASK },
												{ "<Mod5>", CLUTTER_MOD5_MASK },
												{ "<Super>", CLUTTER_SUPER_MASK },
												{ "<Hyper>", CLUTTER_HYPER_MASK },
												{ "<Meta>", CLUTTER_META_MASK },
												{ NULL, 0 }
											};

/* Helper function to set up GError object in this parser */
static void _xfdashboard_bindings_pool_parse_set_error(XfdashboardBindingsPoolParserData *inParserData,
														GMarkupParseContext *inContext,
														GError **outError,
														XfdashboardBindingsPoolErrorEnum inCode,
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
	tempError=g_error_new_literal(XFDASHBOARD_BINDINGS_POOL_ERROR, inCode, message);
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

/* Determine tag name and ID */
static gint _xfdashboard_bindings_pool_get_tag_by_name(const gchar *inTag)
{
	g_return_val_if_fail(inTag && *inTag, -1);

	/* Compare string and return type ID */
	if(g_strcmp0(inTag, "bindings")==0) return(TAG_BINDINGS_POOL);
	if(g_strcmp0(inTag, "key")==0) return(TAG_KEY);

	/* If we get here we do not know tag name and return invalid ID */
	return(-1);
}

static const gchar* _xfdashboard_bindings_pool_get_tag_by_id(guint inTagType)
{
	/* Compare ID and return string */
	switch(inTagType)
	{
		case TAG_DOCUMENT:
			return("document");

		case TAG_BINDINGS_POOL:
			return("bindings");

		case TAG_KEY:
			return("key");

		default:
			break;
	}

	/* If we get here we do not know tag name and return NULL */
	return(NULL);
}

/* Parse a string representing a key binding */
static gboolean _xfdashboard_bindings_pool_parse_keycode(const gchar *inText,
															guint *outKey,
															ClutterModifierType *outModifiers)
{
	guint					key;
	ClutterModifierType		modifiers;
	gchar					**parts;
	gchar					**iter;

	g_return_val_if_fail(inText && *inText, FALSE);

	key=0;
	modifiers=0;

	XFDASHBOARD_DEBUG(_xfdashboard_bindings_pool, MISC,
						"Trying to translating key-binding '%s' to keycode and modifiers",
						inText);

	/* Split text into parts. Valid delimiters are '+', '-', white-spaces. */
	parts=xfdashboard_split_string(inText, "+- \t");
	if(!parts)
	{
		g_warning(_("Could not parse empty key-binding '%s'."), inText);
		return(FALSE);
	}

	/* Iterate through text parts and convert to them keycodes and modifiers.
	 * Modifiers must be enclosed by '<...>' and keys must be named like the
	 * GDK_KEY_* macros but without the leading "GDK_KEY_" prefix.
	 * See also the section 'Key Values' at GDK documentation under
	 * https://developer.gnome.org/gdk2/stable/gdk2-Keyboard-Handling.html#gdk-keyval-from-name
	 */
	iter=parts;
	while(*iter)
	{
		XfdashboardBindingsPoolModifierMap	*mapIter;
		gboolean							wasModifier;

		/* Determine if text part is a modifier by checking if first character
		 * in text part is '<' indicating the begin of a modifier.
		 */
		wasModifier=FALSE;
		if(*iter[0]=='<')
		{
			/* Check that modifier ends at '>' so it is surrounded by '<' and '>' */
			if(!g_str_has_suffix(*iter, ">"))
			{
				g_warning(_("Could not parse modifier '%s' of key-binding '%s' because it is not enclosed by '<...>'"),
							*iter,
							inText);

				/* Free allocated resources */
				g_strfreev(parts);

				/* Return failure result */
				return(FALSE);
			}

			/* Lookup modifier and stop further processing if it is not translatable */
			for(mapIter=_xfdashboard_bindings_pool_modifier_map; !wasModifier && mapIter->name; mapIter++)
			{
				if(g_strcmp0(mapIter->name, *iter)==0)
				{
					modifiers|=mapIter->modifier;
					wasModifier=TRUE;
				}
			}

			if(!wasModifier)
			{
				g_warning(_("Could not parse unknown modifier '%s' of key-binding '%s'"),
							*iter,
							inText);
							
				/* Free allocated resources */
				g_strfreev(parts);

				/* Return failure result */
				return(FALSE);
			}
		}

		/* If text part was not a modifier it must be a translatable key name.
		 * But it can not be more than one single key in a binding text.
		 */
		if(!wasModifier)
		{
			/* A key binding can only have one key assinged.
			 * So translated a text part to key value again is an error.
			 */
			if(key)
			{
				g_warning(_("Could not parse '%s' of key-binding '%s' because it is already a key assigned."),
							*iter,
							inText);

				/* Free allocated resources */
				g_strfreev(parts);

				/* Return failure result */
				return(FALSE);
			}

			/* Translate text part to key value */
			key=gdk_keyval_from_name(*iter);
			if(key==GDK_KEY_VoidSymbol || key==0)
			{
				g_warning(_("Could not parse '%s' of key-binding '%s'"), *iter, inText);

				/* Free allocated resources */
				g_strfreev(parts);

				/* Return failure result */
				return(FALSE);
			}
		}

		/* Move iterator to next text part */
		iter++;
	}

	/* Free allocated resources */
	g_strfreev(parts);

	if(!key && !modifiers)
	{
		g_warning(_("Invalid key-binding '%s' as neither a key nor a modifier was assigned."), inText);
		return(FALSE);
	}

	/* Set result */
	if(outKey) *outKey=key;
	if(outModifiers) *outModifiers=modifiers;

	XFDASHBOARD_DEBUG(_xfdashboard_bindings_pool, MISC,
						"Translated key-binding '%s' to keycode %04x and modifiers %04x",
						inText,
						key,
						modifiers);

	/* Return with success result */
	return(TRUE);
}

/* General callbacks which can be used for any tag */
static void _xfdashboard_bindings_pool_parse_general_no_text_nodes(GMarkupParseContext *inContext,
																	const gchar *inText,
																	gsize inTextLength,
																	gpointer inUserData,
																	GError **outError)
{
	XfdashboardBindingsPoolParserData			*data=(XfdashboardBindingsPoolParserData*)inUserData;
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

		_xfdashboard_bindings_pool_parse_set_error(data,
												inContext,
												outError,
												XFDASHBOARD_BINDINGS_POOL_ERROR_MALFORMED,
												_("Unexpected text node '%s' at tag <%s>"),
												realText,
												parents ? (gchar*)parents->data : "document");
	}
	g_free(realText);
}

static void _xfdashboard_bindings_pool_parse_general_action_text_node(GMarkupParseContext *inContext,
																		const gchar *inText,
																		gsize inTextLength,
																		gpointer inUserData,
																		GError **outError)
{
	XfdashboardBindingsPoolParserData		*data=(XfdashboardBindingsPoolParserData*)inUserData;
	gchar									*action;

	/* Check if an action was specified */
	action=g_strstrip(g_strdup(inText));
	if(!action || strlen(action)<=0)
	{
		/* Update last position for more accurate line and position in error messages */
		data->lastLine=data->currentLine;
		data->lastPosition=data->currentPostition;
		g_markup_parse_context_get_position(inContext, &data->currentLine, &data->currentPostition);

		/* Set error */
		_xfdashboard_bindings_pool_parse_set_error(data,
												inContext,
												outError,
												XFDASHBOARD_BINDINGS_POOL_ERROR_MALFORMED,
												_("Missing action"));

		/* Release allocated resources */
		if(data->lastBinding)
		{
			g_object_unref(data->lastBinding);
			data->lastBinding=NULL;
		}
		if(action) g_free(action);

		return;
	}

	/* Check if a binding was created for this action text node */
	if(!data->lastBinding)
	{
		/* Update last position for more accurate line and position in error messages */
		data->lastLine=data->currentLine;
		data->lastPosition=data->currentPostition;
		g_markup_parse_context_get_position(inContext, &data->currentLine, &data->currentPostition);

		/* Set error */
		_xfdashboard_bindings_pool_parse_set_error(data,
												inContext,
												outError,
												XFDASHBOARD_BINDINGS_POOL_ERROR_MALFORMED,
												_("Missing binding to set action '%s'"), action);

		/* Release allocated resources */
		if(data->lastBinding)
		{
			g_object_unref(data->lastBinding);
			data->lastBinding=NULL;
		}
		if(action) g_free(action);

		return;
	}

	/* Set action at binding */
	xfdashboard_binding_set_action(data->lastBinding, action);

	/* Add or replace binding in class hash-table */
	g_hash_table_insert(data->bindings, g_object_ref(data->lastBinding), NULL);

	/* Release allocated resources */
	g_free(action);

	/* The binding's action is set now so do not remember it anymore */
	g_object_unref(data->lastBinding);
	data->lastBinding=NULL;
}

/* Parser callbacks for document root node */
static void _xfdashboard_bindings_pool_parse_bindings_start(GMarkupParseContext *inContext,
															const gchar *inElementName,
															const gchar **inAttributeNames,
															const gchar **inAttributeValues,
															gpointer inUserData,
															GError **outError)
{
	XfdashboardBindingsPoolParserData		*data=(XfdashboardBindingsPoolParserData*)inUserData;
	gint									currentTag=TAG_BINDINGS_POOL;
	gint									nextTag;
	GError									*error=NULL;

	/* Update last position for more accurate line and position in error messages */
	data->lastLine=data->currentLine;
	data->lastPosition=data->currentPostition;
	g_markup_parse_context_get_position(inContext, &data->currentLine, &data->currentPostition);

	/* Get tag of next element */
	nextTag=_xfdashboard_bindings_pool_get_tag_by_name(inElementName);
	if(nextTag==-1)
	{
		_xfdashboard_bindings_pool_parse_set_error(data,
												inContext,
												outError,
												XFDASHBOARD_BINDINGS_POOL_ERROR_MALFORMED,
												_("Unknown tag <%s>"),
												inElementName);
		return;
	}

	/* Check if element name is <key> and follows expected parent tags:
	 * <document>
	 */
	if(nextTag==TAG_KEY)
	{
		static GMarkupParser					keyParser=
												{
													NULL,
													NULL,
													_xfdashboard_bindings_pool_parse_general_action_text_node,
													NULL,
													NULL,
												};
		gchar									*keycode=NULL;
		gchar									*source=NULL;
		gchar									*when=NULL;
		gchar									*target=NULL;
		gchar									*allowUnfocusableTargets=NULL;
		ClutterEventType						eventType=CLUTTER_NOTHING;
		guint									key=0;
		ClutterModifierType						modifiers=0;
		XfdashboardBindingFlags					flags=0;

		/* Get tag's attributes */
		if(!g_markup_collect_attributes(inElementName,
											inAttributeNames,
											inAttributeValues,
											&error,
											G_MARKUP_COLLECT_STRDUP,
											"code",
											&keycode,
											G_MARKUP_COLLECT_STRDUP,
											"source",
											&source,
											G_MARKUP_COLLECT_STRDUP | G_MARKUP_COLLECT_OPTIONAL,
											"when",
											&when,
											G_MARKUP_COLLECT_STRDUP | G_MARKUP_COLLECT_OPTIONAL,
											"target",
											&target,
											G_MARKUP_COLLECT_STRDUP | G_MARKUP_COLLECT_OPTIONAL,
											"allow-unfocusable-targets",
											&allowUnfocusableTargets,
											G_MARKUP_COLLECT_INVALID))
		{
			/* Propagate error */
			g_propagate_error(outError, error);

			/* Release allocated resources */
			if(keycode) g_free(keycode);
			if(source) g_free(source);
			if(when) g_free(when);
			if(target) g_free(target);
			if(allowUnfocusableTargets) g_free(allowUnfocusableTargets);

			return;
		}

		/* Check tag's attributes */
		if(!keycode)
		{
			/* Set error */
			_xfdashboard_bindings_pool_parse_set_error(data,
														inContext,
														outError,
														XFDASHBOARD_BINDINGS_POOL_ERROR_MALFORMED,
														_("Missing attribute 'code' for key"));

			/* Release allocated resources */
			if(keycode) g_free(keycode);
			if(source) g_free(source);
			if(when) g_free(when);
			if(target) g_free(target);
			if(allowUnfocusableTargets) g_free(allowUnfocusableTargets);

			return;
		}

		/* Parse keycode */
		if(!_xfdashboard_bindings_pool_parse_keycode(keycode, &key, &modifiers))
		{
			/* Set error */
			_xfdashboard_bindings_pool_parse_set_error(data,
														inContext,
														outError,
														XFDASHBOARD_BINDINGS_POOL_ERROR_MALFORMED,
														_("Could not translate key '%s'"),
														keycode);

			/* Release allocated resources */
			if(keycode) g_free(keycode);
			if(source) g_free(source);
			if(when) g_free(when);
			if(target) g_free(target);
			if(allowUnfocusableTargets) g_free(allowUnfocusableTargets);

			return;
		}

		/* Resolve event type to key press or release event
		 * which defaults to key press if missed.
		 */
		eventType=CLUTTER_KEY_PRESS;
		if(when)
		{
			if(g_strcmp0(when, "pressed")==0)
			{
				/* Set event type */
				eventType=CLUTTER_KEY_PRESS;
			}
				else if(g_strcmp0(when, "released")==0)
				{
					/* Set event type */
					eventType=CLUTTER_KEY_RELEASE;
				}
				else
				{
					/* Set error */
					_xfdashboard_bindings_pool_parse_set_error(data,
																inContext,
																outError,
																XFDASHBOARD_BINDINGS_POOL_ERROR_MALFORMED,
																_("Unknown value '%s' for attribute 'when'"),
																when);

					/* Release allocated resources */
					if(keycode) g_free(keycode);
					if(source) g_free(source);
					if(when) g_free(when);
					if(target) g_free(target);
					if(allowUnfocusableTargets) g_free(allowUnfocusableTargets);

					return;
				}
		}

		/* Parse keycode */
		data->lastBinding=xfdashboard_binding_new();
		if(!data->lastBinding)
		{
			/* Set error */
			_xfdashboard_bindings_pool_parse_set_error(data,
														inContext,
														outError,
														XFDASHBOARD_BINDINGS_POOL_ERROR_PARSER_INTERNAL_ERROR,
														_("Could not initialize binding for key-binding"));

			/* Release allocated resources */
			if(keycode) g_free(keycode);
			if(source) g_free(source);
			if(when) g_free(when);
			if(target) g_free(target);
			if(allowUnfocusableTargets) g_free(allowUnfocusableTargets);

			return;
		}

		/* Parse optional attribute "allow-unfocusable-target" */
		if(allowUnfocusableTargets)
		{
			GValue								allowUnfocusableTargetsInitialValue=G_VALUE_INIT;
			GValue								allowUnfocusableTargetsTransformedValue=G_VALUE_INIT;

			g_value_init(&allowUnfocusableTargetsInitialValue, G_TYPE_STRING);
			g_value_set_string(&allowUnfocusableTargetsInitialValue, allowUnfocusableTargets);

			g_value_init(&allowUnfocusableTargetsTransformedValue, G_TYPE_BOOLEAN);

			if(!g_value_transform(&allowUnfocusableTargetsInitialValue, &allowUnfocusableTargetsTransformedValue))
			{
				/* Set error */
				_xfdashboard_bindings_pool_parse_set_error(data,
															inContext,
															outError,
															XFDASHBOARD_BINDINGS_POOL_ERROR_PARSER_INTERNAL_ERROR,
															_("Cannot transform attribute 'allow-unfocusable-target' from type '%s' to type '%s'"),
															g_type_name(G_VALUE_TYPE(&allowUnfocusableTargetsInitialValue)),
															g_type_name(G_VALUE_TYPE(&allowUnfocusableTargetsTransformedValue)));

				/* Release allocated resources */
				g_value_unset(&allowUnfocusableTargetsTransformedValue);
				g_value_unset(&allowUnfocusableTargetsInitialValue);

				if(keycode) g_free(keycode);
				if(source) g_free(source);
				if(when) g_free(when);
				if(target) g_free(target);
				if(allowUnfocusableTargets) g_free(allowUnfocusableTargets);

				return;
			}

			if(g_value_get_boolean(&allowUnfocusableTargetsTransformedValue))
			{
				flags|=XFDASHBOARD_BINDING_FLAGS_ALLOW_UNFOCUSABLE_TARGET;
			}

			/* Release allocated resources */
			g_value_unset(&allowUnfocusableTargetsTransformedValue);
			g_value_unset(&allowUnfocusableTargetsInitialValue);
		}

		xfdashboard_binding_set_event_type(data->lastBinding, eventType);
		xfdashboard_binding_set_class_name(data->lastBinding, source);
		xfdashboard_binding_set_key(data->lastBinding, key);
		xfdashboard_binding_set_modifiers(data->lastBinding, modifiers);
		if(target) xfdashboard_binding_set_target(data->lastBinding, target);
		xfdashboard_binding_set_flags(data->lastBinding, flags);

		/* Release allocated resources */
		if(keycode) g_free(keycode);
		if(source) g_free(source);
		if(when) g_free(when);
		if(target) g_free(target);
		if(allowUnfocusableTargets) g_free(allowUnfocusableTargets);

		/* Set up context for tag <key> */
		g_markup_parse_context_push(inContext, &keyParser, inUserData);
		return;
	}

	/* If we get here the given element name cannot follow this tag */
	_xfdashboard_bindings_pool_parse_set_error(data,
											inContext,
											outError,
											XFDASHBOARD_BINDINGS_POOL_ERROR_MALFORMED,
											_("Tag <%s> cannot contain tag <%s>"),
											_xfdashboard_bindings_pool_get_tag_by_id(currentTag),
											inElementName);
}

static void _xfdashboard_bindings_pool_parse_bindings_end(GMarkupParseContext *inContext,
															const gchar *inElementName,
															gpointer inUserData,
															GError **outError)
{
	/* Restore previous parser context */
	g_markup_parse_context_pop(inContext);
}

/* Parser callbacks for document root node */
static void _xfdashboard_bindings_pool_parse_document_start(GMarkupParseContext *inContext,
														const gchar *inElementName,
														const gchar **inAttributeNames,
														const gchar **inAttributeValues,
														gpointer inUserData,
														GError **outError)
{
	XfdashboardBindingsPoolParserData	*data=(XfdashboardBindingsPoolParserData*)inUserData;
	gint								currentTag=TAG_DOCUMENT;
	gint								nextTag;
	GError								*error=NULL;

	/* Update last position for more accurate line and position in error messages */
	data->lastLine=data->currentLine;
	data->lastPosition=data->currentPostition;
	g_markup_parse_context_get_position(inContext, &data->currentLine, &data->currentPostition);

	/* Get tag of next element */
	nextTag=_xfdashboard_bindings_pool_get_tag_by_name(inElementName);
	if(nextTag==-1)
	{
		_xfdashboard_bindings_pool_parse_set_error(data,
												inContext,
												outError,
												XFDASHBOARD_BINDINGS_POOL_ERROR_MALFORMED,
												_("Unknown tag <%s>"),
												inElementName);
		return;
	}

	/* Check if element name is <bindings> and follows expected parent tags:
	 * <document>
	 */
	if(nextTag==TAG_BINDINGS_POOL)
	{
		static GMarkupParser					propertyParser=
												{
													_xfdashboard_bindings_pool_parse_bindings_start,
													_xfdashboard_bindings_pool_parse_bindings_end,
													_xfdashboard_bindings_pool_parse_general_no_text_nodes,
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

		/* Set up context for tag <bindings> */
		g_markup_parse_context_push(inContext, &propertyParser, inUserData);
		return;
	}

	/* If we get here the given element name cannot follow this tag */
	_xfdashboard_bindings_pool_parse_set_error(data,
											inContext,
											outError,
											XFDASHBOARD_BINDINGS_POOL_ERROR_MALFORMED,
											_("Tag <%s> cannot contain tag <%s>"),
											_xfdashboard_bindings_pool_get_tag_by_id(currentTag),
											inElementName);
}

static void _xfdashboard_bindings_pool_parse_document_end(GMarkupParseContext *inContext,
														const gchar *inElementName,
														gpointer inUserData,
														GError **outError)
{
	/* Restore previous parser context */
	g_markup_parse_context_pop(inContext);
}

/* Merge bindings from one hash-table into another one */
static void _xfdashboard_bindings_pool_merge_bindings(gpointer inKey,
														gpointer inValue,
														gpointer inUserData)
{
	GHashTable						*targetBindingsHashTable;
	XfdashboardBinding				*binding;

	g_return_if_fail(XFDASHBOARD_IS_BINDING(inKey));
	g_return_if_fail(inUserData);

	targetBindingsHashTable=(GHashTable*)inUserData;
	binding=XFDASHBOARD_BINDING(inKey);

	/* Insert binding into target hash table which will either insert a new key
	 * with the binding value or replaces the binding value at the existing key.
	 */
	g_hash_table_replace(targetBindingsHashTable, g_object_ref(binding), inValue);
}

/* Load bindings from XML file */
static gboolean _xfdashboard_bindings_pool_load_bindings_from_file(XfdashboardBindingsPool *self,
																	const gchar *inPath,
																	GError **outError)
{
	static GMarkupParser				parser=
										{
											_xfdashboard_bindings_pool_parse_document_start,
											_xfdashboard_bindings_pool_parse_document_end,
											_xfdashboard_bindings_pool_parse_general_no_text_nodes,
											NULL,
											NULL,
										};

	XfdashboardBindingsPoolPrivate		*priv;
	gchar								*contents;
	gsize								contentsLength;
	GMarkupParseContext					*context;
	XfdashboardBindingsPoolParserData	*data;
	GError								*error;
	gboolean							success;

	g_return_val_if_fail(XFDASHBOARD_IS_BINDINGS_POOL(self), FALSE);
	g_return_val_if_fail(inPath && *inPath, FALSE);
	g_return_val_if_fail(outError==NULL || *outError==NULL, FALSE);

	priv=self->priv;
	data=NULL;
	error=NULL;
	success=TRUE;

	XFDASHBOARD_DEBUG(self, MISC,
						"Loading bindings from %s'",
						inPath);

	/* Load XML file into memory */
	error=NULL;
	if(!g_file_get_contents(inPath, &contents, &contentsLength, &error))
	{
		g_propagate_error(outError, error);
		return(FALSE);
	}

	/* Create and set up parser instance */
	data=g_new0(XfdashboardBindingsPoolParserData, 1);
	if(!data)
	{
		/* Set error */
		g_set_error(outError,
					XFDASHBOARD_BINDINGS_POOL_ERROR,
					XFDASHBOARD_BINDINGS_POOL_ERROR_PARSER_INTERNAL_ERROR,
					_("Could not set up parser data for file %s"),
					inPath);
		return(FALSE);
	}

	context=g_markup_parse_context_new(&parser, 0, data, NULL);
	if(!context)
	{
		/* Set error */
		g_set_error(outError,
					XFDASHBOARD_BINDINGS_POOL_ERROR,
					XFDASHBOARD_BINDINGS_POOL_ERROR_PARSER_INTERNAL_ERROR,
					_("Could not create parser for file %s"),
					inPath);

		/* Release allocated resources */
		g_free(contents);
		g_free(data);

		/* Return error result */
		return(FALSE);
	}

	/* Now the parser and its context is set up and we can now
	 * safely initialize data.
	 */
	data->self=self;
	data->lastLine=1;
	data->lastPosition=1;
	data->currentLine=1;
	data->currentPostition=1;
	data->bindings=g_hash_table_new_full(xfdashboard_binding_hash,
											xfdashboard_binding_compare,
											(GDestroyNotify)g_object_unref,
											NULL);
	if(!data->bindings)
	{
		/* Set error */
		g_set_error(outError,
					XFDASHBOARD_BINDINGS_POOL_ERROR,
					XFDASHBOARD_BINDINGS_POOL_ERROR_PARSER_INTERNAL_ERROR,
					_("Could not set up hash-table at parser data for file %s"),
					inPath);

		/* Release allocated resources */
		g_markup_parse_context_free(context);
		g_free(contents);
		g_free(data);

		/* Return error result */
		return(FALSE);
	}

	/* Parse XML string */
	if(success && !g_markup_parse_context_parse(context, contents, -1, &error))
	{
		g_propagate_error(outError, error);
		success=FALSE;
	}

	if(success && !g_markup_parse_context_end_parse(context, &error))
	{
		g_propagate_error(outError, error);
		success=FALSE;
	}

	if(data->lastBinding)
	{
		g_set_error(outError,
					XFDASHBOARD_BINDINGS_POOL_ERROR,
					XFDASHBOARD_BINDINGS_POOL_ERROR_PARSER_INTERNAL_ERROR,
					_("Unexpected binding state set at parser data for file %s"),
					inPath);
		success=FALSE;
	}

	/* Handle collected data if parsing was successful */
	if(success)
	{
		guint							oldBindingsCount;

		/* Get current number of bindings */
		oldBindingsCount=g_hash_table_size(priv->bindings);

		/* Merge newly read-in bindings into global ones */
		g_hash_table_foreach(data->bindings, _xfdashboard_bindings_pool_merge_bindings, priv->bindings);
		XFDASHBOARD_DEBUG(self, MISC,
							"Merged %u bindings from file '%s', now having a total of %u bindings (was %u bindings before)",
							g_hash_table_size(data->bindings),
							inPath,
							g_hash_table_size(priv->bindings),
							oldBindingsCount);
	}

	/* Clean up resources */
	g_hash_table_unref(data->bindings);
	g_free(data);

	g_free(contents);

	g_markup_parse_context_free(context);

	/* Return success or error result */
	return(success);
}

/* IMPLEMENTATION: GObject */

/* Construct this object */
static GObject* _xfdashboard_bindings_pool_constructor(GType inType,
														guint inNumberConstructParams,
														GObjectConstructParam *inConstructParams)
{
	GObject									*object;

	if(!_xfdashboard_bindings_pool)
	{
		object=G_OBJECT_CLASS(xfdashboard_bindings_pool_parent_class)->constructor(inType, inNumberConstructParams, inConstructParams);
		_xfdashboard_bindings_pool=XFDASHBOARD_BINDINGS_POOL(object);
	}
		else
		{
			object=g_object_ref(G_OBJECT(_xfdashboard_bindings_pool));
		}

	return(object);
}

/* Dispose this object */
static void _xfdashboard_bindings_pool_dispose(GObject *inObject)
{
	/* Release allocated variables */
	XfdashboardBindingsPool				*self=XFDASHBOARD_BINDINGS_POOL(inObject);
	XfdashboardBindingsPoolPrivate		*priv=self->priv;

	if(priv->bindings)
	{
		g_hash_table_destroy(priv->bindings);
		priv->bindings=NULL;
	}

	/* Call parent's class dispose method */
	G_OBJECT_CLASS(xfdashboard_bindings_pool_parent_class)->dispose(inObject);
}

/* Finalize this object */
static void _xfdashboard_bindings_pool_finalize(GObject *inObject)
{
	/* Release allocated resources finally, e.g. unset singleton */
	if(G_LIKELY(G_OBJECT(_xfdashboard_bindings_pool)==inObject))
	{
		_xfdashboard_bindings_pool=NULL;
	}

	/* Call parent's class dispose method */
	G_OBJECT_CLASS(xfdashboard_bindings_pool_parent_class)->finalize(inObject);
}

/* Class initialization
 * Override functions in parent classes and define properties
 * and signals
 */
static void xfdashboard_bindings_pool_class_init(XfdashboardBindingsPoolClass *klass)
{
	GObjectClass					*gobjectClass=G_OBJECT_CLASS(klass);

	/* Override functions */
	gobjectClass->constructor=_xfdashboard_bindings_pool_constructor;
	gobjectClass->dispose=_xfdashboard_bindings_pool_dispose;
	gobjectClass->finalize=_xfdashboard_bindings_pool_finalize;
}

/* Object initialization
 * Create private structure and set up default values
 */
static void xfdashboard_bindings_pool_init(XfdashboardBindingsPool *self)
{
	XfdashboardBindingsPoolPrivate		*priv;

	priv=self->priv=xfdashboard_bindings_pool_get_instance_private(self);

	/* Set up default values */
	priv->bindings=NULL;
}

/* IMPLEMENTATION: Errors */

GQuark xfdashboard_bindings_pool_error_quark(void)
{
	return(g_quark_from_static_string("xfdashboard-bindings-error-quark"));
}

/* IMPLEMENTATION: Public API */

/* Get single instance of manager */
XfdashboardBindingsPool* xfdashboard_bindings_pool_get_default(void)
{
	GObject									*singleton;

	singleton=g_object_new(XFDASHBOARD_TYPE_BINDINGS_POOL, NULL);
	return(XFDASHBOARD_BINDINGS_POOL(singleton));
}

/* Load bindings from configuration file */
gboolean xfdashboard_bindings_pool_load(XfdashboardBindingsPool *self, GError **outError)
{
	XfdashboardBindingsPoolPrivate		*priv;
	gchar								*configFile;
	GError								*error;
	gboolean							success;
	gint								numberSources;

	g_return_val_if_fail(XFDASHBOARD_IS_BINDINGS_POOL(self), FALSE);
	g_return_val_if_fail(outError==NULL || *outError==NULL, FALSE);

	priv=self->priv;
	configFile=NULL;
	error=NULL;
	success=TRUE;
	numberSources=0;

	/* Destroy bindings used currently */
	if(priv->bindings)
	{
		g_hash_table_destroy(priv->bindings);
		priv->bindings=NULL;

		XFDASHBOARD_DEBUG(self, MISC, "Removed current bindings because of reloading bindings configuration files");
	}

	/* Create hash-table to store bindings at */
	priv->bindings=g_hash_table_new_full(xfdashboard_binding_hash,
											xfdashboard_binding_compare,
											(GDestroyNotify)g_object_unref,
											NULL);
	if(!priv->bindings)
	{
		/* Set error */
		g_set_error(outError,
					XFDASHBOARD_BINDINGS_POOL_ERROR,
					XFDASHBOARD_BINDINGS_POOL_ERROR_INTERNAL_ERROR,
					_("Could not set up hash-table to store bindings"));

		/* Return error result */
		return(FALSE);
	}

	/* First try to load bindings configuration file from system-wide path,
	 * usually located at /usr/share/xfdashboard. The file is called "bindings.xml".
	 */
	if(success)
	{
		configFile=g_build_filename(PACKAGE_DATADIR, "xfdashboard", "bindings.xml", NULL);
		XFDASHBOARD_DEBUG(self, MISC,
							"Trying system bindings configuration file: %s",
							configFile);
		if(g_file_test(configFile, G_FILE_TEST_EXISTS | G_FILE_TEST_IS_REGULAR))
		{
			if(!_xfdashboard_bindings_pool_load_bindings_from_file(self, configFile, &error))
			{
				/* Propagate error if available */
				if(error) g_propagate_error(outError, error);

				/* Set error status */
				success=FALSE;
			}

			/* Increase source counter regardless if loading succeeded or failed */
			numberSources++;
		}

		g_free(configFile);
		configFile=NULL;
	}

	/* Next try to load user configuration file. This file is located in
	 * the folder 'xfdashboard' at configuration directory in user's home
	 * path (usually ~/.config/xfdashboard). The file is called "bindings.xml".
	 */
	if(success)
	{
		configFile=g_build_filename(g_get_user_config_dir(), "xfdashboard", "bindings.xml", NULL);
		XFDASHBOARD_DEBUG(self, MISC,
							"Trying user bindings configuration file: %s",
							configFile);
		if(g_file_test(configFile, G_FILE_TEST_EXISTS | G_FILE_TEST_IS_REGULAR))
		{
			if(!_xfdashboard_bindings_pool_load_bindings_from_file(self, configFile, &error))
			{
				/* Propagate error if available */
				if(error) g_propagate_error(outError, error);

				/* Set error status */
				success=FALSE;
			}

			/* Increase source counter regardless if loading succeeded or failed */
			numberSources++;
		}

		g_free(configFile);
		configFile=NULL;
	}

	/* At last tro to load a user defined configuration file from an alternate
	 * configuration path provided by an environment variable. This environment
	 * variable must contain the full path (path and file name) to load.
	 */
	if(success)
	{
		const gchar					*envFile;

		envFile=g_getenv("XFDASHBOARD_BINDINGS_POOL_FILE");
		if(envFile)
		{
			configFile=g_strdup(envFile);
			XFDASHBOARD_DEBUG(self, MISC,
								"Trying alternate bindings configuration file: %s",
								configFile);
			if(g_file_test(configFile, G_FILE_TEST_EXISTS | G_FILE_TEST_IS_REGULAR))
			{
				if(!_xfdashboard_bindings_pool_load_bindings_from_file(self, configFile, &error))
				{
					/* Propagate error if available */
					if(error) g_propagate_error(outError, error);

					/* Set error status */
					success=FALSE;
				}

				/* Increase source counter regardless if loading succeeded or failed */
				numberSources++;
			}

			g_free(configFile);
			configFile=NULL;
		}
	}

	/* If we get here and if we have still not found any bindings file we could load
	 * then return error.
	 */
	if(numberSources==0)
	{
		/* Set error */
		g_set_error(outError,
					XFDASHBOARD_BINDINGS_POOL_ERROR,
					XFDASHBOARD_BINDINGS_POOL_ERROR_FILE_NOT_FOUND,
					_("No bindings configuration file found."));

		/* Return error result */
		return(FALSE);
	}

	/* Release allocated resources */
	g_free(configFile);

	/* Return success or error result */
	return(success);
}

/* Find action for event and actor */
const XfdashboardBinding* xfdashboard_bindings_pool_find_for_event(XfdashboardBindingsPool *self, ClutterActor *inActor, const ClutterEvent *inEvent)
{
	XfdashboardBindingsPoolPrivate		*priv;
	XfdashboardBinding					*lookupBinding;
	const XfdashboardBinding			*foundBinding;
	GType								classType;
	GSList								*interfaces;

	g_return_val_if_fail(XFDASHBOARD_IS_BINDINGS_POOL(self), NULL);
	g_return_val_if_fail(inEvent, NULL);

	priv=self->priv;
	classType=G_TYPE_FROM_INSTANCE(inActor);
	lookupBinding=NULL;
	foundBinding=NULL;
	interfaces=NULL;

	/* If no bindings was set then we do not need to check this event */
	if(!priv->bindings) return(NULL);

	/* Get a binding instance for given event used to lookup registered bindings */
	lookupBinding=xfdashboard_binding_new_for_event(inEvent);
	if(!lookupBinding) return(NULL);

	/* Beginning at class of actor, check if we have a binding stored matching
	 * the binding to lookup. If any matches return the binding found.
	 * If it does not match and the class has interfaces collect them but
	 * avoid duplicates. Then get parent class and repeat the checks as described
	 * before.
	 * If we reach top class without a matching binding, check if we have
	 * a binding for any interface we collected on our way through parent classes
	 * and return the matching binding if found.
	 */
	while(classType)
	{
		GType							*classInterfaces;
		GType							*iter;

		/* Set class name in lookup binding */
		xfdashboard_binding_set_class_name(lookupBinding, g_type_name(classType));

		/* Check if we have a binding matching lookup binding */
		if(g_hash_table_lookup_extended(priv->bindings, lookupBinding, (gpointer*)&foundBinding, NULL))
		{
			XFDASHBOARD_DEBUG(self, MISC,
								"Found binding for class=%s, key=%04x, mods=%04x",
								xfdashboard_binding_get_class_name(lookupBinding),
								xfdashboard_binding_get_key(lookupBinding),
								xfdashboard_binding_get_modifiers(lookupBinding));

			/* Found a binding so stop iterating through classes of actor */
			if(interfaces) g_slist_free(interfaces);
			if(lookupBinding) g_object_unref(lookupBinding);
			return(foundBinding);
		}

		/* Collect interfaces */
		classInterfaces=iter=g_type_interfaces(classType, NULL);
		while(iter && *iter)
		{
			/* Avoid duplicates */
			if(g_slist_index(interfaces, GTYPE_TO_POINTER(*iter))==-1)
			{
				interfaces=g_slist_prepend(interfaces, GTYPE_TO_POINTER(*iter));
			}

			/* Check next interface */
			iter++;
		}
		g_free(classInterfaces);

		/* Get parent class */
		classType=g_type_parent(classType);
	}

	/* If we get here no matching binding for any class was found.
	 * Try interfaces now.
	 */
	if(interfaces)
	{
		GSList							*iter;
		GType							classInterface;

		iter=interfaces;
		do
		{
			/* Get type of interface and set it in lookup binding */
			classInterface=GPOINTER_TO_GTYPE(iter->data);
			xfdashboard_binding_set_class_name(lookupBinding, g_type_name(classInterface));

			/* Check for matching binding */
			if(g_hash_table_lookup_extended(priv->bindings, lookupBinding, (gpointer*)&foundBinding, NULL))
			{
				XFDASHBOARD_DEBUG(self, MISC,
									"Found binding for interface=%s for key=%04x, mods=%04x",
									xfdashboard_binding_get_class_name(lookupBinding),
									xfdashboard_binding_get_key(lookupBinding),
									xfdashboard_binding_get_modifiers(lookupBinding));

				/* Found a binding so stop iterating through classes of actor */
				if(interfaces) g_slist_free(interfaces);
				if(lookupBinding) g_object_unref(lookupBinding);
				return(foundBinding);
			}
		}
		while((iter=g_slist_next(iter)));
	}

	/* Release allocated resources */
	if(interfaces) g_slist_free(interfaces);
	if(lookupBinding) g_object_unref(lookupBinding);

	/* If we get here we did not find a matching binding so return NULL */
	return(NULL);
}
