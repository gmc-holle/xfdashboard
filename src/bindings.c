/*
 * bindings: Customizable keyboard and pointer bindings for focusable actors
 * 
 * Copyright 2012-2014 Stephan Haller <nomad@froevel.de>
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

#include "bindings.h"

#include <glib-object.h>
#include <glib/gi18n-lib.h>
#include <gdk/gdk.h>
#include <gdk/gdkkeysyms.h>

#include "utils.h"

/* Define this class in GObject system */
G_DEFINE_TYPE(XfdashboardBindings,
				xfdashboard_bindings,
				G_TYPE_OBJECT)

/* Private structure - access only by public API if needed */
#define XFDASHBOARD_BINDINGS_GET_PRIVATE(obj) \
	(G_TYPE_INSTANCE_GET_PRIVATE((obj), XFDASHBOARD_TYPE_BINDINGS, XfdashboardBindingsPrivate))

struct _XfdashboardBindingsPrivate
{
	/* Instance related */
	GHashTable		*bindings;
};

/* IMPLEMENTATION: Private variables and methods */
#define XFDASHBOARD_BINDINGS_MODIFIERS_MASK		(CLUTTER_SHIFT_MASK | \
													CLUTTER_CONTROL_MASK | \
													CLUTTER_MOD1_MASK | \
													CLUTTER_MOD2_MASK | \
													CLUTTER_MOD3_MASK | \
													CLUTTER_MOD4_MASK | \
													CLUTTER_MOD5_MASK | \
													CLUTTER_SUPER_MASK | \
													CLUTTER_HYPER_MASK | \
													CLUTTER_META_MASK | \
													CLUTTER_RELEASE_MASK)

enum
{
	TAG_DOCUMENT,
	TAG_BINDINGS,
	TAG_KEY,
	TAG_POINTER
};

typedef enum
{
	BINDING_TYPE_KEY,
	BINDING_TYPE_POINTER,
	BINDING_TYPE_MAX
} XfdashboardBindingsBindingType;

typedef struct _XfdashboardBindingsBinding		XfdashboardBindingsBinding;
struct _XfdashboardBindingsBinding
{
	gint								refCount;

	XfdashboardBindingsBindingType		type;
	gchar								*className;

	union
	{
		struct
		{
			guint						key;
			ClutterModifierType			modifiers;
		} key;

		struct
		{
			guint						button;
			ClutterModifierType			modifiers;
		} pointer;
	};
};

typedef struct _XfdashboardBindingsModifierMap	XfdashboardBindingsModifierMap;
struct _XfdashboardBindingsModifierMap
{
	const gchar							*name;
	ClutterModifierType					modifier;
};

typedef struct _XfdashboardBindingsParserData	XfdashboardBindingsParserData;
struct _XfdashboardBindingsParserData
{
	XfdashboardBindings					*self;

	GHashTable							*bindings;

	gint								lastLine;
	gint								lastPosition;
	gint								currentLine;
	gint								currentPostition;

	XfdashboardBindingsBinding			*lastBinding;
};

/* Forward declarations */
static void _xfdashboard_bindings_parse_set_error(XfdashboardBindingsParserData *inParserData,
													GMarkupParseContext *inContext,
													GError **outError,
													XfdashboardBindingsErrorEnum inCode,
													const gchar *inFormat,
													...) G_GNUC_PRINTF (5, 6);

/* Single instance of bindings */
static XfdashboardBindings*				_xfdashboard_bindings=NULL;
static XfdashboardBindingsModifierMap	_xfdashboard_bindings_modifier_map[]=
										{
											{ "<Shift>", CLUTTER_SHIFT_MASK },
											{ "<Ctrl>", CLUTTER_CONTROL_MASK },
											{ "<Control>", CLUTTER_CONTROL_MASK},
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

/* Create a new binding */
static XfdashboardBindingsBinding* _xfdashboard_bindings_key_binding_new(const gchar *inClass,
																			guint inKey,
																			ClutterModifierType inModifiers)
{
	XfdashboardBindingsBinding	*binding;

	g_return_val_if_fail(inClass && *inClass, NULL);
	g_return_val_if_fail(inKey || inModifiers, NULL);

	/* Create new binding entry */
	binding=g_new(XfdashboardBindingsBinding, 1);
	if(!binding)
	{
		g_warning(_("Could not create key binding for class '%s' and keycode %u and modifiers %u."),
					inClass,
					inKey,
					inModifiers);
		return(NULL);
	}

	binding->type=BINDING_TYPE_KEY;
	binding->refCount=1;
	binding->className=g_strdup(inClass);
	binding->key.key=inKey;
	binding->key.modifiers=(inModifiers & XFDASHBOARD_BINDINGS_MODIFIERS_MASK);

	return(binding);
}

static XfdashboardBindingsBinding* _xfdashboard_bindings_pointer_binding_new(const gchar *inClass,
																				guint inButton,
																				ClutterModifierType inModifiers)
{
	XfdashboardBindingsBinding	*binding;

	g_return_val_if_fail(inClass && *inClass, NULL);
	g_return_val_if_fail(inButton || inModifiers, NULL);

	/* Create new binding entry */
	binding=g_new(XfdashboardBindingsBinding, 1);
	if(!binding)
	{
		g_warning(_("Could not create pointer binding for class '%s' and button %u and modifiers %u."),
					inClass,
					inButton,
					inModifiers);
		return(NULL);
	}

	binding->type=BINDING_TYPE_POINTER;
	binding->refCount=1;
	binding->className=g_strdup(inClass);
	binding->pointer.button=inButton;
	binding->pointer.modifiers=(inModifiers & XFDASHBOARD_BINDINGS_MODIFIERS_MASK);

	return(binding);
}

/* Free a binding */
static void _xfdashboard_bindings_binding_free(XfdashboardBindingsBinding *inBinding)
{
	g_return_if_fail(inBinding);

	/* Free allocated resources by this key binding */
	if(inBinding->className) g_free(inBinding->className);
	g_free(inBinding);
}

/* Increase reference of a binding */
static XfdashboardBindingsBinding* _xfdashboard_bindings_binding_ref(XfdashboardBindingsBinding *inBinding)
{
	g_return_val_if_fail(inBinding, NULL);

	inBinding->refCount++;

	return(inBinding);
}

/* Decrease reference of a binding and free it if it reaches zero */
static void _xfdashboard_bindings_binding_unref(XfdashboardBindingsBinding *inBinding)
{
	g_return_if_fail(inBinding);

	inBinding->refCount--;
	if(inBinding->refCount==0) _xfdashboard_bindings_binding_free(inBinding);
}

/* Get hash value for binding */
static guint _xfdashboard_bindings_binding_hash(gconstpointer inValue)
{
	XfdashboardBindingsBinding		*binding=(XfdashboardBindingsBinding*)inValue;
	guint							hash;

	hash=g_str_hash(binding->className);

	switch(binding->type)
	{
		case BINDING_TYPE_KEY:
			hash^=binding->key.key;
			hash^=binding->key.modifiers;
			break;

		case BINDING_TYPE_POINTER:
			hash^=binding->pointer.button;
			hash^=binding->pointer.modifiers;
			break;

		default:
			g_assert_not_reached();
			break;
	}

	return(hash);
}

/* Check if two bindings are equal */
static gboolean _xfdashboard_bindings_binding_compare(gconstpointer inLeft, gconstpointer inRight)
{
	XfdashboardBindingsBinding	*left=(XfdashboardBindingsBinding*)inLeft;
	XfdashboardBindingsBinding	*right=(XfdashboardBindingsBinding*)inRight;


	/* Check if type of bindings are equal */
	if(left->type!=right->type) return(FALSE);

	/* Check if class of bindings are equal */
	if(g_strcmp0(left->className, right->className)) return(FALSE);

	/* Check if other values of bindings are equal - depending on their type */
	switch(left->type)
	{
		case BINDING_TYPE_KEY:
			if(left->key.key!=right->key.key ||
				left->key.modifiers!=right->key.modifiers)
			{
				return(FALSE);
			}
			break;

		case BINDING_TYPE_POINTER:
			if(left->pointer.button!=right->pointer.button ||
				left->pointer.modifiers!=right->pointer.modifiers)
			{
				return(FALSE);
			}
			break;

		default:
			/* We should never get here but if we do return FALSE
			 * to indicate that both binding are not equal.
			 */
			g_assert_not_reached();
			return(FALSE);
	}

	/* If we get here all check succeeded so return TRUE */
	return(TRUE);
}

/* Helper function to set up GError object in this parser */
static void _xfdashboard_bindings_parse_set_error(XfdashboardBindingsParserData *inParserData,
													GMarkupParseContext *inContext,
													GError **outError,
													XfdashboardBindingsErrorEnum inCode,
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
	tempError=g_error_new_literal(XFDASHBOARD_BINDINGS_ERROR, inCode, message);
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
static gint _xfdashboard_bindings_get_tag_by_name(const gchar *inTag)
{
	g_return_val_if_fail(inTag && *inTag, -1);

	/* Compare string and return type ID */
	if(g_strcmp0(inTag, "bindings")==0) return(TAG_BINDINGS);
	if(g_strcmp0(inTag, "key")==0) return(TAG_KEY);
	if(g_strcmp0(inTag, "pointer")==0) return(TAG_POINTER);

	/* If we get here we do not know tag name and return invalid ID */
	return(-1);
}

static const gchar* _xfdashboard_bindings_get_tag_by_id(guint inTagType)
{
	/* Compare ID and return string */
	switch(inTagType)
	{
		case TAG_DOCUMENT:
			return("document");

		case TAG_BINDINGS:
			return("bindings");

		case TAG_KEY:
			return("key");

		case TAG_POINTER:
			return("pointer");

		default:
			break;
	}

	/* If we get here we do not know tag name and return NULL */
	return(NULL);
}

/* Parse a string representing a key binding */
static gboolean _xfdashboard_bindings_parse_keycode(const gchar *inText, guint *outKey, ClutterModifierType *outModifiers)
{
	guint					key;
	ClutterModifierType		modifiers;
	gchar					**parts;
	gchar					**iter;

	g_return_val_if_fail(inText && *inText, FALSE);

	key=0;
	modifiers=0;

	g_debug("Trying to translating key-binding '%s' to keycode and modifiers", inText);

	/* Split text into parts. Valid delimiters are '+', '-', white-spaces. */
	parts=xfdashboard_split_string(inText, "+- \t\r\n");
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
		XfdashboardBindingsModifierMap	*mapIter;
		gboolean						wasModifier;

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
			for(mapIter=_xfdashboard_bindings_modifier_map; !wasModifier && mapIter->name; mapIter++)
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

	/* A key-binding can have no modifiers but at least a key must be assigned */
	if(!key)
	{
		g_warning(_("Invalid key-binding '%s' as no key was assigned."), inText);
		return(FALSE);
	}

	/* Set result */
	if(outKey) *outKey=key;
	if(outModifiers) *outModifiers=modifiers;

	g_debug("Translated key-binding '%s' to keycode %04x and modifiers %04x", inText, key, modifiers);

	/* Return with success result */
	return(TRUE);
}

/* General callbacks which can be used for any tag */
static void _xfdashboard_bindings_parse_general_no_text_nodes(GMarkupParseContext *inContext,
																const gchar *inText,
																gsize inTextLength,
																gpointer inUserData,
																GError **outError)
{
	XfdashboardBindingsParserData			*data=(XfdashboardBindingsParserData*)inUserData;
	gchar									*realText;

	/* Check if text contains only whitespace. If we find any non-whitespace
	 * in text then set error.
	 */
	realText=g_strstrip(g_strdup(inText));
	if(*realText)
	{
		const GSList	*parents;

		parents=g_markup_parse_context_get_element_stack(inContext);
		if(parents) parents=g_slist_next(parents);

		_xfdashboard_bindings_parse_set_error(data,
												inContext,
												outError,
												XFDASHBOARD_BINDINGS_ERROR_MALFORMED,
												_("Unexpected text node '%s' at tag <%s>"),
												realText,
												parents ? (gchar*)parents->data : "document");
	}
	g_free(realText);
}

static void _xfdashboard_bindings_parse_general_action_text_node(GMarkupParseContext *inContext,
																	const gchar *inText,
																	gsize inTextLength,
																	gpointer inUserData,
																	GError **outError)
{
	XfdashboardBindingsParserData		*data=(XfdashboardBindingsParserData*)inUserData;
	gchar								*action;

	/* Check if an action was specified */
	action=g_strstrip(g_strdup(inText));
	if(!action || strlen(action)<=0)
	{
		/* Update last position for more accurate line and position in error messages */
		data->lastLine=data->currentLine;
		data->lastPosition=data->currentPostition;
		g_markup_parse_context_get_position(inContext, &data->currentLine, &data->currentPostition);

		/* Set error */
		_xfdashboard_bindings_parse_set_error(data,
												inContext,
												outError,
												XFDASHBOARD_BINDINGS_ERROR_MALFORMED,
												_("Missing action"));

		/* Release allocated resources */
		if(data->lastBinding)
		{
			_xfdashboard_bindings_binding_unref(data->lastBinding);
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
		_xfdashboard_bindings_parse_set_error(data,
												inContext,
												outError,
												XFDASHBOARD_BINDINGS_ERROR_MALFORMED,
												_("Missing binding to set action '%s'"), action);

		/* Release allocated resources */
		if(data->lastBinding)
		{
			_xfdashboard_bindings_binding_unref(data->lastBinding);
			data->lastBinding=NULL;
		}
		if(action) g_free(action);

		return;
	}


	/* Add or replace binding in class hash-table */
	g_hash_table_insert(data->bindings, _xfdashboard_bindings_binding_ref(data->lastBinding), g_strdup(action));

	/* Release allocated resources */
	g_free(action);

	/* The binding's action is set now so do not remember it anymore */
	_xfdashboard_bindings_binding_unref(data->lastBinding);
	data->lastBinding=NULL;
}

/* Parser callbacks for document root node */
static void _xfdashboard_bindings_parse_bindings_start(GMarkupParseContext *inContext,
														const gchar *inElementName,
														const gchar **inAttributeNames,
														const gchar **inAttributeValues,
														gpointer inUserData,
														GError **outError)
{
	XfdashboardBindingsParserData		*data=(XfdashboardBindingsParserData*)inUserData;
	gint								currentTag=TAG_BINDINGS;
	gint								nextTag;
	GError								*error=NULL;

	/* Update last position for more accurate line and position in error messages */
	data->lastLine=data->currentLine;
	data->lastPosition=data->currentPostition;
	g_markup_parse_context_get_position(inContext, &data->currentLine, &data->currentPostition);

	/* Get tag of next element */
	nextTag=_xfdashboard_bindings_get_tag_by_name(inElementName);
	if(nextTag==-1)
	{
		_xfdashboard_bindings_parse_set_error(data,
												inContext,
												outError,
												XFDASHBOARD_BINDINGS_ERROR_MALFORMED,
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
													_xfdashboard_bindings_parse_general_action_text_node,
													NULL,
													NULL,
												};
		gchar									*keycode=NULL;
		gchar									*source=NULL;
		gchar									*when=NULL;
		guint									key;
		ClutterModifierType						modifiers;

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
											G_MARKUP_COLLECT_INVALID))
		{
			g_propagate_error(outError, error);
			if(keycode) g_free(keycode);
			if(source) g_free(source);
			if(when) g_free(when);
			return;
		}

		/* Check tag's attributes */
		if(!keycode)
		{
			/* Set error */
			_xfdashboard_bindings_parse_set_error(data,
													inContext,
													outError,
													XFDASHBOARD_BINDINGS_ERROR_MALFORMED,
													_("Missing attribute 'code' for key"));

			/* Release allocated resources */
			if(keycode) g_free(keycode);
			if(source) g_free(source);
			if(when) g_free(when);

			return;
		}

		if(!_xfdashboard_bindings_parse_keycode(keycode, &key, &modifiers))
		{
			/* Set error */
			_xfdashboard_bindings_parse_set_error(data,
													inContext,
													outError,
													XFDASHBOARD_BINDINGS_ERROR_MALFORMED,
													_("Could not translate key '%s'"),
													keycode);

			/* Release allocated resources */
			if(keycode) g_free(keycode);
			if(source) g_free(source);
			if(when) g_free(when);

			return;
		}

		if(when)
		{
			if(g_strcmp0(when, "pressed")==0)
			{
				/* Unset release bit in modifiers */
				modifiers&=!CLUTTER_RELEASE_MASK;
			}
				else if(g_strcmp0(when, "released")==0)
				{
					/* Set release bit in modifiers */
					modifiers|=CLUTTER_RELEASE_MASK;
				}
				else
				{
					/* Set error */
					_xfdashboard_bindings_parse_set_error(data,
															inContext,
															outError,
															XFDASHBOARD_BINDINGS_ERROR_MALFORMED,
															_("Unknown value '%s' for attribute 'when'"),
															when);

					/* Release allocated resources */
					if(keycode) g_free(keycode);
					if(source) g_free(source);
					if(when) g_free(when);

					return;
				}
		}

		/* Parse keycode */
		data->lastBinding=_xfdashboard_bindings_key_binding_new(source, key, modifiers);
		if(!data->lastBinding)
		{
			/* Set error */
			_xfdashboard_bindings_parse_set_error(data,
													inContext,
													outError,
													XFDASHBOARD_BINDINGS_ERROR_PARSER_INTERNAL_ERROR,
													_("Could not initialize binding for key-binding"));

			/* Release allocated resources */
			if(keycode) g_free(keycode);
			if(source) g_free(source);
			if(when) g_free(when);

			return;
		}

		/* Release allocated resources */
		if(keycode) g_free(keycode);
		if(source) g_free(source);
		if(when) g_free(when);

		/* Set up context for tag <key> */
		g_markup_parse_context_push(inContext, &keyParser, inUserData);
		return;
	}

#if 0
	/* Check if element name is <pointer> and follows expected parent tags:
	 * <document>
	 */
	if(nextTag==TAG_POINTER)
	{
		static GMarkupParser					propertyParser=
												{
													_xfdashboard_bindings_parse_bindings_no_deep_node,
													NULL,
													_xfdashboard_bindings_parse_general_action_text_node,
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

		/* Set up context for tag <pointer> */
		g_markup_parse_context_push(inContext, &propertyParser, inUserData);
		return;
	}
#endif

	/* If we get here the given element name cannot follow this tag */
	_xfdashboard_bindings_parse_set_error(data,
											inContext,
											outError,
											XFDASHBOARD_BINDINGS_ERROR_MALFORMED,
											_("Tag <%s> cannot contain tag <%s>"),
											_xfdashboard_bindings_get_tag_by_id(currentTag),
											inElementName);
}

static void _xfdashboard_bindings_parse_bindings_end(GMarkupParseContext *inContext,
														const gchar *inElementName,
														gpointer inUserData,
														GError **outError)
{
	/* Restore previous parser context */
	g_markup_parse_context_pop(inContext);
}

/* Parser callbacks for document root node */
static void _xfdashboard_bindings_parse_document_start(GMarkupParseContext *inContext,
														const gchar *inElementName,
														const gchar **inAttributeNames,
														const gchar **inAttributeValues,
														gpointer inUserData,
														GError **outError)
{
	XfdashboardBindingsParserData		*data=(XfdashboardBindingsParserData*)inUserData;
	gint								currentTag=TAG_DOCUMENT;
	gint								nextTag;
	GError								*error=NULL;

	/* Update last position for more accurate line and position in error messages */
	data->lastLine=data->currentLine;
	data->lastPosition=data->currentPostition;
	g_markup_parse_context_get_position(inContext, &data->currentLine, &data->currentPostition);

	/* Get tag of next element */
	nextTag=_xfdashboard_bindings_get_tag_by_name(inElementName);
	if(nextTag==-1)
	{
		_xfdashboard_bindings_parse_set_error(data,
												inContext,
												outError,
												XFDASHBOARD_BINDINGS_ERROR_MALFORMED,
												_("Unknown tag <%s>"),
												inElementName);
		return;
	}

	/* Check if element name is <bindings> and follows expected parent tags:
	 * <document>
	 */
	if(nextTag==TAG_BINDINGS)
	{
		static GMarkupParser					propertyParser=
												{
													_xfdashboard_bindings_parse_bindings_start,
													_xfdashboard_bindings_parse_bindings_end,
													_xfdashboard_bindings_parse_general_no_text_nodes,
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
	_xfdashboard_bindings_parse_set_error(data,
											inContext,
											outError,
											XFDASHBOARD_BINDINGS_ERROR_MALFORMED,
											_("Tag <%s> cannot contain tag <%s>"),
											_xfdashboard_bindings_get_tag_by_id(currentTag),
											inElementName);
}

static void _xfdashboard_bindings_parse_document_end(GMarkupParseContext *inContext,
														const gchar *inElementName,
														gpointer inUserData,
														GError **outError)
{
	/* Restore previous parser context */
	g_markup_parse_context_pop(inContext);
}

/* Load bindings from XML file */
static gboolean _xfdashboard_bindings_load_bindings_from_file(XfdashboardBindings *self,
																const gchar *inPath,
																GError **outError)
{
	static GMarkupParser			parser=
									{
										_xfdashboard_bindings_parse_document_start,
										_xfdashboard_bindings_parse_document_end,
										_xfdashboard_bindings_parse_general_no_text_nodes,
										NULL,
										NULL,
									};

	XfdashboardBindingsPrivate		*priv;
	gchar							*contents;
	gsize							contentsLength;
	GMarkupParseContext				*context;
	XfdashboardBindingsParserData	*data;
	GError							*error;
	gboolean						success;

	g_return_val_if_fail(XFDASHBOARD_IS_BINDINGS(self), FALSE);
	g_return_val_if_fail(inPath && *inPath, FALSE);
	g_return_val_if_fail(outError==NULL || *outError==NULL, FALSE);

	priv=self->priv;
	data=NULL;
	error=NULL;
	success=TRUE;

	g_debug("Loading bindings from %s'", inPath);

	/* Load XML file into memory */
	error=NULL;
	if(!g_file_get_contents(inPath, &contents, &contentsLength, &error))
	{
		g_propagate_error(outError, error);
		return(FALSE);
	}

	/* Create and set up parser instance */
	data=g_new0(XfdashboardBindingsParserData, 1);
	if(!data)
	{
		/* Set error */
		g_set_error(outError,
					XFDASHBOARD_BINDINGS_ERROR,
					XFDASHBOARD_BINDINGS_ERROR_PARSER_INTERNAL_ERROR,
					_("Could not set up parser data for file %s"),
					inPath);
		return(FALSE);
	}

	context=g_markup_parse_context_new(&parser, 0, data, NULL);
	if(!context)
	{
		/* Set error */
		g_set_error(outError,
					XFDASHBOARD_BINDINGS_ERROR,
					XFDASHBOARD_BINDINGS_ERROR_PARSER_INTERNAL_ERROR,
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
	data->bindings=g_hash_table_new_full(_xfdashboard_bindings_binding_hash,
											_xfdashboard_bindings_binding_compare,
											(GDestroyNotify)_xfdashboard_bindings_binding_unref,
											g_free);
	if(!data->bindings)
	{
		/* Set error */
		g_set_error(outError,
					XFDASHBOARD_BINDINGS_ERROR,
					XFDASHBOARD_BINDINGS_ERROR_PARSER_INTERNAL_ERROR,
					_("Could not set up hash-table at parser data for file %s"),
					inPath);

		/* Release allocated resources */
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
					XFDASHBOARD_BINDINGS_ERROR,
					XFDASHBOARD_BINDINGS_ERROR_PARSER_INTERNAL_ERROR,
					_("Unexpected binding state set at parser data for file %s"),
					inPath);
		success=FALSE;
	}

	/* Handle collected data if parsing was successful */
	if(success)
	{
		/* Destroy bindings used currently */
		if(priv->bindings)
		{
			g_hash_table_destroy(priv->bindings);
			priv->bindings=NULL;
		}

		/* Take a reference at newly read-in bindings now as it will be unreffed
		 * in clean-up code block within this function.
		 */
		priv->bindings=g_hash_table_ref(data->bindings);
	}

	/* Clean up resources */
	g_hash_table_unref(data->bindings);
	g_free(data);

	g_free(contents);

	/* Return success or error result */
	return(success);
}

/* IMPLEMENTATION: GObject */

/* Dispose this object */
static void _xfdashboard_bindings_dispose(GObject *inObject)
{
	/* Release allocated variables */
	XfdashboardBindings				*self=XFDASHBOARD_BINDINGS(inObject);
	XfdashboardBindingsPrivate		*priv=self->priv;

	if(priv->bindings)
	{
		g_hash_table_destroy(priv->bindings);
		priv->bindings=NULL;
	}

	/* Call parent's class dispose method */
	G_OBJECT_CLASS(xfdashboard_bindings_parent_class)->dispose(inObject);
}

/* Class initialization
 * Override functions in parent classes and define properties
 * and signals
 */
static void xfdashboard_bindings_class_init(XfdashboardBindingsClass *klass)
{
	GObjectClass					*gobjectClass=G_OBJECT_CLASS(klass);

	/* Override functions */
	gobjectClass->dispose=_xfdashboard_bindings_dispose;

	/* Set up private structure */
	g_type_class_add_private(klass, sizeof(XfdashboardBindingsPrivate));
}

/* Object initialization
 * Create private structure and set up default values
 */
static void xfdashboard_bindings_init(XfdashboardBindings *self)
{
	XfdashboardBindingsPrivate		*priv;

	priv=self->priv=XFDASHBOARD_BINDINGS_GET_PRIVATE(self);

	/* Set up default values */
	priv->bindings=NULL;
}

/* Implementation: Errors */

GQuark xfdashboard_bindings_error_quark(void)
{
	return(g_quark_from_static_string("xfdashboard-bindings-error-quark"));
}

/* IMPLEMENTATION: Public API */

/* Get single instance of manager */
XfdashboardBindings* xfdashboard_bindings_get_default(void)
{
	if(G_UNLIKELY(_xfdashboard_bindings==NULL))
	{
		_xfdashboard_bindings=g_object_new(XFDASHBOARD_TYPE_BINDINGS, NULL);
	}
		else g_object_ref(_xfdashboard_bindings);

	return(_xfdashboard_bindings);
}

/* Load bindings from configuration file */
gboolean xfdashboard_bindings_load(XfdashboardBindings *self, GError **outError)
{
	gchar							*configFile;
	GError							*error;
	gboolean						success;

	g_return_val_if_fail(XFDASHBOARD_IS_BINDINGS(self), FALSE);
	g_return_val_if_fail(outError==NULL || *outError==NULL, FALSE);

	configFile=NULL;
	error=NULL;
	success=TRUE;

#ifdef DEBUG
	/* If compile with debug mode enabled allow specifiing an alternate
	 * configuration path. The path must contain the path and file to load.
	 */
	if(!configFile)
	{
		const gchar					*envFile;

		envFile=g_getenv("XFDASHBOARD_BINDINGS_FILE");
		if(envFile)
		{
			configFile=g_strdup(envFile);
			g_debug("Trying bindings configuration file: %s", configFile);
			if(!g_file_test(configFile, G_FILE_TEST_EXISTS | G_FILE_TEST_IS_REGULAR))
			{
				g_free(configFile);
				configFile=NULL;
			}
		}
	}
#endif

	/* User configuration should be tried first. This file is located in
	 * the folder 'xfdashboard' at configuration directory in user's home
	 * path (usually ~/.config/xfdashboard). The file is called "bindings.xml".
	 */
	if(!configFile)
	{
		configFile=g_build_filename(g_get_user_config_dir(), "xfdashboard", "bindings.xml", NULL);
		g_debug("Trying bindings configuration file: %s", configFile);
		if(!g_file_test(configFile, G_FILE_TEST_EXISTS | G_FILE_TEST_IS_REGULAR))
		{
			g_free(configFile);
			configFile=NULL;
		}
	}

	/* Next try to load bindings configuration file from system-wide path,
	 * usually located at /usr/share/xfdashboard. The file is called "bindings.xml".
	 */
	if(!configFile)
	{
		configFile=g_build_filename(PACKAGE_DATADIR, "xfdashboard", "bindings.xml", NULL);
		g_debug("Trying bindings configuration file: %s", configFile);
		if(!g_file_test(configFile, G_FILE_TEST_EXISTS | G_FILE_TEST_IS_REGULAR))
		{
			g_free(configFile);
			configFile=NULL;
		}
	}

	/* If we get here and if we have still not found any bindings file we could load
	 * then return error.
	 */
	if(!configFile)
	{
		/* Set error */
		g_set_error(outError,
					XFDASHBOARD_BINDINGS_ERROR,
					XFDASHBOARD_BINDINGS_ERROR_FILE_NOT_FOUND,
					_("No bindings configuration file found."));

		/* Return error result */
		return(FALSE);
	}

	/* Load, parse and set up bindings from configuration file found */
	if(!_xfdashboard_bindings_load_bindings_from_file(self, configFile, &error))
	{
		/* Propagate error if available */
		if(error) g_propagate_error(outError, error);

		/* Set error status */
		success=FALSE;
	}

	/* Release allocated resources */
	g_free(configFile);

	/* Return success or error result */
	return(success);
}

/* Find action for event and actor */
const gchar* xfdashboard_bindings_find_for_event(XfdashboardBindings *self, ClutterActor *inActor, const ClutterEvent *inEvent)
{
	XfdashboardBindingsPrivate		*priv;
	XfdashboardBindingsBinding		*lookupBinding;
	const gchar						*foundAction;
	GType							classType;
	GSList							*interfaces;

	g_return_val_if_fail(XFDASHBOARD_IS_BINDINGS(self), NULL);
	g_return_val_if_fail(inEvent, NULL);

	priv=self->priv;
	classType=G_TYPE_FROM_INSTANCE(inActor);
	lookupBinding=NULL;
	foundAction=NULL;
	interfaces=NULL;

	/* If no bindings was set then we do not need to check this event */
	if(!priv->bindings) return(NULL);

	/* Check if event if either a keyboard or pointer related event */
	switch(clutter_event_type(inEvent))
	{
		case CLUTTER_KEY_PRESS:
			lookupBinding=_xfdashboard_bindings_key_binding_new(g_type_name(classType),
																((ClutterKeyEvent*)inEvent)->keyval,
																(((ClutterKeyEvent*)inEvent)->modifier_state & XFDASHBOARD_BINDINGS_MODIFIERS_MASK) & !CLUTTER_RELEASE_MASK);
			break;

		case CLUTTER_KEY_RELEASE:
			lookupBinding=_xfdashboard_bindings_key_binding_new(g_type_name(classType),
																((ClutterKeyEvent*)inEvent)->keyval,
																(((ClutterKeyEvent*)inEvent)->modifier_state & XFDASHBOARD_BINDINGS_MODIFIERS_MASK) | CLUTTER_RELEASE_MASK);
			break;

		case CLUTTER_BUTTON_PRESS:
			lookupBinding=_xfdashboard_bindings_pointer_binding_new(g_type_name(classType),
																	((ClutterButtonEvent*)inEvent)->button,
																	(((ClutterButtonEvent*)inEvent)->modifier_state & XFDASHBOARD_BINDINGS_MODIFIERS_MASK) & !CLUTTER_RELEASE_MASK);
			break;

		case CLUTTER_BUTTON_RELEASE:
			lookupBinding=_xfdashboard_bindings_pointer_binding_new(g_type_name(classType),
																	((ClutterButtonEvent*)inEvent)->button,
																	(((ClutterButtonEvent*)inEvent)->modifier_state & XFDASHBOARD_BINDINGS_MODIFIERS_MASK) | CLUTTER_RELEASE_MASK);
			break;

		default:
			lookupBinding=NULL;
			break;
	}

	if(!lookupBinding) return(NULL);

	/* Beginning at class of actor check if we have a binding stored matching
	 * the binding to lookup. If any matches return the binding found.
	 * If it does not match and the class has interfaces collect them but
	 * avoid duplicates. Then get parent class and repeat the checks as described
	 * before.
	 * If we reach top class without a matching binding check if we have
	 * a binding for any interface we collected on our way through parent classes
	 * and return the matching binding if found.
	 */
	do
	{
		GType						*classInterfaces;
		GType						*iter;

		/* Class name is already set in lookup binding when checking for event
		 * type or within this loop when looking up parent class so just check.
		 */
		if(g_hash_table_lookup_extended(priv->bindings, lookupBinding, NULL, (gpointer*)&foundAction))
		{
			g_debug("Found binding for class=%s, key/button=%04x, mods=%04x",
					lookupBinding->className,
					lookupBinding->key.key,
					lookupBinding->key.modifiers);

			/* Found a binding so stop iterating through classes of actor */
			if(interfaces) g_slist_free(interfaces);
			if(lookupBinding) _xfdashboard_bindings_binding_unref(lookupBinding);
			return(foundAction);
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

		/* Get parent class and set it in lookup binding */
		classType=g_type_parent(classType);
		if(classType)
		{
			g_free(lookupBinding->className);
			lookupBinding->className=g_strdup(g_type_name(classType));
		}
	}
	while(classType);

	/* If we get here no matching binding for any class was found.
	 * Try interfaces now.
	 */
	if(interfaces)
	{
		GSList						*iter;
		GType						classInterface;

		iter=interfaces;
		do
		{
			/* Get type of interface ans set it in lookup binding */
			classInterface=GPOINTER_TO_GTYPE(iter->data);

			g_free(lookupBinding->className);
			lookupBinding->className=g_strdup(g_type_name(classInterface));

			/* Check for matching binding */
			if(g_hash_table_lookup_extended(priv->bindings, lookupBinding, NULL, (gpointer*)&foundAction))
			{
				g_debug("Found binding for interface=%s for key/button=%04x, mods=%04x",
						lookupBinding->className,
						lookupBinding->key.key,
						lookupBinding->key.modifiers);

				/* Found a binding so stop iterating through classes of actor */
				if(interfaces) g_slist_free(interfaces);
				if(lookupBinding) _xfdashboard_bindings_binding_unref(lookupBinding);
				return(foundAction);
			}
		}
		while((iter=g_slist_next(iter)));
	}

	/* Release allocated resources */
	if(interfaces) g_slist_free(interfaces);
	if(lookupBinding) _xfdashboard_bindings_binding_unref(lookupBinding);

	/* If we get here we did not find a matching binding so return NULL */
	return(NULL);
}
