/*
 * theme-layout: A theme used for build and layout objects by XML files
 * 
 * Copyright 2012-2015 Stephan Haller <nomad@froevel.de>
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

#include "theme-layout.h"

#include <glib/gi18n-lib.h>
#include <glib.h>
#include <gio/gio.h>

#include "utils.h"

/* Define this class in GObject system */
G_DEFINE_TYPE(XfdashboardThemeLayout,
				xfdashboard_theme_layout,
				G_TYPE_OBJECT)

/* Private structure - access only by public API if needed */
#define XFDASHBOARD_THEME_LAYOUT_GET_PRIVATE(obj) \
	(G_TYPE_INSTANCE_GET_PRIVATE((obj), XFDASHBOARD_TYPE_THEME_LAYOUT, XfdashboardThemeLayoutPrivate))

struct _XfdashboardThemeLayoutPrivate
{
	/* Instance related */
	GSList			*interfaces;
};

/* IMPLEMENTATION: Private variables and methods */
enum
{
	TAG_DOCUMENT,
	TAG_INTERFACE,
	TAG_OBJECT,
	TAG_CHILD,
	TAG_PROPERTY,
	TAG_CONSTRAINT,
	TAG_LAYOUT
};

typedef struct _XfdashboardThemeLayoutTagData			XfdashboardThemeLayoutTagData;
struct _XfdashboardThemeLayoutTagData
{
	gint								refCount;

	gint								tagType;

	union
	{
		struct
		{
			gchar						*id;
			gchar						*class;
		} object;

		struct
		{
			gchar						*name;
			gchar						*value;
			gboolean					translatable;
			gchar						*refID;
		} property;
	} tag;
};

typedef struct _XfdashboardThemeLayoutParsedObject		XfdashboardThemeLayoutParsedObject;
struct _XfdashboardThemeLayoutParsedObject
{
	gint								refCount;

	gchar								*id;
	GType								classType;
	GSList								*properties;	/* 0, 1 or more entries of XfdashboardThemeLayoutTagData */
	GSList								*constraints;	/* 0, 1 or more entries of XfdashboardThemeLayoutParsedObject */
	XfdashboardThemeLayoutParsedObject	*layout;		/* 0 or 1 entry of XfdashboardThemeLayoutParsedObject */
	GSList								*children;		/* 0, 1 or more entries of XfdashboardThemeLayoutParsedObject */
};

typedef struct _XfdashboardThemeLayoutParserData		XfdashboardThemeLayoutParserData;
struct _XfdashboardThemeLayoutParserData
{
	XfdashboardThemeLayout				*self;

	XfdashboardThemeLayoutParsedObject	*interface;
	GQueue								*stackObjects;
	GQueue								*stackTags;

	gint								lastLine;
	gint								lastPosition;
	gint								currentLine;
	gint								currentPostition;;
};

typedef struct _XfdashboardThemeLayoutUnresolvedBuildID		XfdashboardThemeLayoutUnresolvedBuildID;
struct _XfdashboardThemeLayoutUnresolvedBuildID
{
	GObject								*targetObject;
	XfdashboardThemeLayoutTagData		*property;
};

/* Forward declarations */
static void _xfdashboard_theme_layout_parse_set_error(XfdashboardThemeLayoutParserData *inParserData,
														GMarkupParseContext *inContext,
														GError **outError,
														XfdashboardThemeLayoutErrorEnum inCode,
														const gchar *inFormat,
														...) G_GNUC_PRINTF (5, 6);

static void _xfdashboard_theme_layout_object_data_unref(XfdashboardThemeLayoutParsedObject *inData);

#ifdef DEBUG
static void _xfdashboard_theme_layout_print_parsed_objects_internal(XfdashboardThemeLayoutParsedObject *inData, gint inDepth, const gchar *inPrefix)
{
#define INDENT(n, s) { gint i; for(i=0; i<n; i++) g_print("%s", s); }
#define LOOP(slist, var, vartype, func) { GSList *entry=slist; while(entry) { var=(vartype*)entry->data; func ; entry=g_slist_next(entry); } }

	XfdashboardThemeLayoutTagData		*tagData;
	XfdashboardThemeLayoutParsedObject	*objectData;
	gint								j;
	gchar								*prefix;
	const gchar							*indention="    ";

	g_return_if_fail(inData);
	g_return_if_fail(inDepth>=0);
	g_return_if_fail(inPrefix);

	INDENT(inDepth, indention);
	g_print("# %s %p[%s] with id '%s' at depth %d (ref-count=%d, properties=%d, constraints=%d, layouts=%d, children=%d)\n",
				inPrefix,
				inData, g_type_name(inData->classType),
				inData->id ? inData->id : "<none>",
				inDepth,
				inData->refCount,
				g_slist_length(inData->properties),
				g_slist_length(inData->constraints),
				inData->layout ? 1 : 0,
				g_slist_length(inData->children));

	j=1;
	LOOP(inData->properties,
			tagData, XfdashboardThemeLayoutTagData,
			{
				INDENT(inDepth+1, indention);
				g_print("# Property %d: '%s'='%s' (ref-count=%d, translatable=%s, refID=%s)\n",
							j++,
							tagData->tag.property.name,
							tagData->tag.property.value,
							tagData->refCount,
							tagData->tag.property.translatable ? "yes" : "no",
							tagData->tag.property.refID);
			});

	j=1;
	LOOP(inData->constraints,
			objectData, XfdashboardThemeLayoutParsedObject,
			{
				prefix=g_strdup_printf("Constraint %d:", j++);
				_xfdashboard_theme_layout_print_parsed_objects_internal(objectData, inDepth+1, prefix);
				g_free(prefix);
			});

	if(inData->layout)
	{
		objectData=inData->layout;

		_xfdashboard_theme_layout_print_parsed_objects_internal(objectData, inDepth+1, "Layout:");
	}

	j=1;
	LOOP(inData->children,
			objectData, XfdashboardThemeLayoutParsedObject,
			{
				prefix=g_strdup_printf("Child %d:", j++);
				_xfdashboard_theme_layout_print_parsed_objects_internal(objectData, inDepth+1, prefix);
				g_free(prefix);
			});

#undef LOOP
#undef INDENT
}

static void _xfdashboard_theme_layout_print_parsed_objects(XfdashboardThemeLayoutParsedObject *inData, gpointer inUserData)
{
	const gchar			*prefix;

	g_return_if_fail(inData);
	g_return_if_fail(inUserData);

	prefix=(const gchar*)inUserData;

	g_print("----\n");
	_xfdashboard_theme_layout_print_parsed_objects_internal(inData, 0, prefix);
	g_print("----\n");
}
#endif

/* Helper function to set up GError object in this parser */
static void _xfdashboard_theme_layout_parse_set_error(XfdashboardThemeLayoutParserData *inParserData,
														GMarkupParseContext *inContext,
														GError **outError,
														XfdashboardThemeLayoutErrorEnum inCode,
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
	tempError=g_error_new_literal(XFDASHBOARD_THEME_LAYOUT_ERROR, inCode, message);
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
static GType _xfdashboard_theme_layout_resolve_type_lazy(const gchar *inTypeName)
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
static gint _xfdashboard_theme_layout_get_tag_by_name(const gchar *inTag)
{
	g_return_val_if_fail(inTag && *inTag, -1);

	/* Compare string and return type ID */
	if(g_strcmp0(inTag, "interface")==0) return(TAG_INTERFACE);
	if(g_strcmp0(inTag, "object")==0) return(TAG_OBJECT);
	if(g_strcmp0(inTag, "child")==0) return(TAG_CHILD);
	if(g_strcmp0(inTag, "property")==0) return(TAG_PROPERTY);
	if(g_strcmp0(inTag, "constraint")==0) return(TAG_CONSTRAINT);
	if(g_strcmp0(inTag, "layout")==0) return(TAG_LAYOUT);

	/* If we get here we do not know tag name and return invalid ID */
	return(-1);
}

static const gchar* _xfdashboard_theme_layout_get_tag_by_id(guint inTagType)
{
	/* Compare ID and return string */
	switch(inTagType)
	{
		case TAG_DOCUMENT:
			return("document");

		case TAG_INTERFACE:
			return("interface");

		case TAG_OBJECT:
			return("object");

		case TAG_CHILD:
			return("child");

		case TAG_PROPERTY:
			return("property");

		case TAG_CONSTRAINT:
			return("constraint");

		case TAG_LAYOUT:
			return("layout");

		default:
			break;
	}

	/* If we get here we do not know tag name and return NULL */
	return(NULL);
}

/* Create, destroy, ref and unref tag data for a tag */
static XfdashboardThemeLayoutTagData* _xfdashboard_theme_layout_tag_data_new(GMarkupParseContext *inContext,
																				gint inTagType,
																				GError **outError)
{
	const gchar						*tagName;
	XfdashboardThemeLayoutTagData	*tagData;

	g_return_val_if_fail(outError && *outError==NULL, NULL);

	/* Check tag type */
	tagName=_xfdashboard_theme_layout_get_tag_by_id(inTagType);
	if(!tagName)
	{
		_xfdashboard_theme_layout_parse_set_error(NULL,
													inContext,
													outError,
													XFDASHBOARD_THEME_LAYOUT_ERROR_ERROR,
													_("Cannot allocate memory for unknown tag"));
		return(NULL);
	}

	/* Create tag data for given tag type */
	tagData=g_new0(XfdashboardThemeLayoutTagData, 1);
	if(!tagData)
	{
		_xfdashboard_theme_layout_parse_set_error(NULL,
													inContext,
													outError,
													XFDASHBOARD_THEME_LAYOUT_ERROR_ERROR,
													_("Cannot allocate memory for tag '%s'"),
													tagName);
		return(NULL);
	}
	tagData->refCount=1;
	tagData->tagType=inTagType;

	return(tagData);
}

static void _xfdashboard_theme_layout_tag_data_free(XfdashboardThemeLayoutTagData *inData)
{
	g_return_if_fail(inData);

	/* Release allocated resources for specified tag */
	switch(inData->tagType)
	{
		case TAG_OBJECT:
			if(inData->tag.object.id) g_free(inData->tag.object.id);
			if(inData->tag.object.class) g_free(inData->tag.object.class);
			break;

		case TAG_PROPERTY:
			if(inData->tag.property.name) g_free(inData->tag.property.name);
			if(inData->tag.property.value) g_free(inData->tag.property.value);
			if(inData->tag.property.refID) g_free(inData->tag.property.refID);
			break;
	}

	/* Release common allocated resources */
	g_free(inData);
}

static XfdashboardThemeLayoutTagData* _xfdashboard_theme_layout_tag_data_ref(XfdashboardThemeLayoutTagData *inData)
{
	g_return_val_if_fail(inData, NULL);

	inData->refCount++;
	return(inData);
}

static void _xfdashboard_theme_layout_tag_data_unref(XfdashboardThemeLayoutTagData *inData)
{
	g_return_if_fail(inData);

	inData->refCount--;
	if(inData->refCount==0) _xfdashboard_theme_layout_tag_data_free(inData);
}

static void _xfdashboard_theme_layout_tag_data_free_foreach_callback(gpointer inData, gpointer inUserData)
{
	XfdashboardThemeLayoutTagData	*data;

	g_return_if_fail(inData);

	data=(XfdashboardThemeLayoutTagData*)inData;

	/* Free tag data */
	_xfdashboard_theme_layout_tag_data_free(data);
}

/* Create, destroy, ref and unref object data */
static XfdashboardThemeLayoutParsedObject* _xfdashboard_theme_layout_object_data_new(GMarkupParseContext *inContext,
																						gint inParentTagType,
																						GError **outError)
{
	XfdashboardThemeLayoutParsedObject	*objectData;

	g_return_val_if_fail(outError && *outError==NULL, NULL);

	/* Create object data */
	objectData=g_new0(XfdashboardThemeLayoutParsedObject, 1);
	if(!objectData)
	{
		_xfdashboard_theme_layout_parse_set_error(NULL,
													inContext,
													outError,
													XFDASHBOARD_THEME_LAYOUT_ERROR_ERROR,
													_("Cannot allocate memory for object data of tag <%s>"),
													_xfdashboard_theme_layout_get_tag_by_id(inParentTagType));
		return(NULL);
	}
	objectData->refCount=1;
	objectData->classType=G_TYPE_INVALID;

	return(objectData);
}

static void _xfdashboard_theme_layout_object_data_free(XfdashboardThemeLayoutParsedObject *inData)
{
	g_return_if_fail(inData);

	/* Release allocated resources */
	if(inData->id) g_free(inData->id);
	if(inData->properties) g_slist_free_full(inData->properties, (GDestroyNotify)_xfdashboard_theme_layout_tag_data_unref);
	if(inData->constraints) g_slist_free_full(inData->constraints, (GDestroyNotify)_xfdashboard_theme_layout_object_data_unref);
	if(inData->layout) _xfdashboard_theme_layout_object_data_unref(inData->layout);
	if(inData->children) g_slist_free_full(inData->children, (GDestroyNotify)_xfdashboard_theme_layout_object_data_unref);
	g_free(inData);
}

static XfdashboardThemeLayoutParsedObject* _xfdashboard_theme_layout_object_data_ref(XfdashboardThemeLayoutParsedObject *inData)
{
	g_return_val_if_fail(inData, NULL);

	inData->refCount++;
	return(inData);
}

static void _xfdashboard_theme_layout_object_data_unref(XfdashboardThemeLayoutParsedObject *inData)
{
	g_return_if_fail(inData);

	inData->refCount--;
	if(inData->refCount==0) _xfdashboard_theme_layout_object_data_free(inData);
}

static void _xfdashboard_theme_layout_object_data_free_foreach_callback(gpointer inData, gpointer inUserData)
{
	XfdashboardThemeLayoutParsedObject	*data;

	g_return_if_fail(inData);

	data=(XfdashboardThemeLayoutParsedObject*)inData;

	/* Free object data */
	_xfdashboard_theme_layout_object_data_free(data);
}

/* Free data about an unresolved property which refers an other object */
static void _xfdashboard_theme_layout_create_object_free_unresolved(gpointer inData)
{
	XfdashboardThemeLayoutUnresolvedBuildID		*unresolved;

	g_return_if_fail(inData);

	/* Release allocated resources */
	unresolved=(XfdashboardThemeLayoutUnresolvedBuildID*)inData;
	if(unresolved->targetObject) g_object_unref(unresolved->targetObject);
	if(unresolved->property) _xfdashboard_theme_layout_tag_data_unref(unresolved->property);
	g_free(unresolved);
}

/* Resolve all "unresolved" properties (properties which refer to other objects) and
 * set the pointer to resolved object at the associated property.
 */
static void _xfdashboard_theme_layout_create_object_resolve_unresolved(XfdashboardThemeLayout *self,
																		GHashTable *inIDs,
																		GSList *inUnresolvedIDs)
{
	GObject										*refObject;
	GSList										*entry;
	XfdashboardThemeLayoutUnresolvedBuildID		*unresolvedID;

	g_return_if_fail(XFDASHBOARD_IS_THEME_LAYOUT(self));
	g_return_if_fail(inIDs);

	/* Return if no list of unresolved IDs is provided */
	if(!inUnresolvedIDs) return;

	/* Iterate through list of unresolved IDs and their mapping to properties
	 * and get referenced object and set the pointer to this object at the
	 * mapped property of target object.
	 */
	for(entry=inUnresolvedIDs; entry; entry=g_slist_next(entry))
	{
		unresolvedID=(XfdashboardThemeLayoutUnresolvedBuildID*)entry->data;

		/* Get referenced object */
		refObject=g_hash_table_lookup(inIDs, unresolvedID->property->tag.property.refID);

		/* Set pointer to referenced object in property of target object */
		g_object_set(unresolvedID->targetObject,
						unresolvedID->property->tag.property.name,
						refObject,
						NULL);
		g_debug("Set previously unresolved object %s with ID '%s' at target object %s at property '%s'",
					refObject ? G_OBJECT_TYPE_NAME(refObject) : "<unknown object>",
					unresolvedID->property->tag.property.refID,
					unresolvedID->targetObject ? G_OBJECT_TYPE_NAME(unresolvedID->targetObject) : "<unknown object>",
					unresolvedID->property->tag.property.name);
	}
}

/* Create object with all its constraints, layout and children recursively.
 * Set up all properties which do not reference any other object at creation of object
 * but remember all "unresolved" properties (which do reference other objects).
 */
static GObject* _xfdashboard_theme_layout_create_object(XfdashboardThemeLayout *self,
														XfdashboardThemeLayoutParsedObject *inObjectData,
														GHashTable *ioIDs,
														GSList **ioUnresolvedIDs)
{
	GObject									*object;
	GSList									*entry;
	GParameter								*properties;
	gint									maxProperties, usedProperties, i;

	g_return_val_if_fail(XFDASHBOARD_IS_THEME_LAYOUT(self), NULL);
	g_return_val_if_fail(inObjectData, NULL);
	g_return_val_if_fail(ioIDs, NULL);
	g_return_val_if_fail(ioUnresolvedIDs, NULL);

	/* Collect all properties as array which do not refer to other objects */
	usedProperties=0;
	properties=NULL;

	maxProperties=g_slist_length(inObjectData->properties);
	if(maxProperties>0) properties=g_new0(GParameter, maxProperties);

	for(entry=inObjectData->properties; entry; entry=g_slist_next(entry))
	{
		XfdashboardThemeLayoutTagData		*property;

		property=(XfdashboardThemeLayoutTagData*)entry->data;

		/* Check if property refers to an other object, if not add it */
		if(!property->tag.property.refID)
		{
			properties[usedProperties].name=property->tag.property.name;
			g_value_init(&properties[usedProperties].value, G_TYPE_STRING);

			if(!property->tag.property.translatable) g_value_set_string(&properties[usedProperties].value, property->tag.property.value);
				else g_value_set_string(&properties[usedProperties].value, _(property->tag.property.value));

			usedProperties++;
		}
	}

	/* Create instance of object type and before handling error or success
	 * of creation release allocated resources for properties as they are
	 * not needed anymore.
	 */
	object=G_OBJECT(g_object_newv(inObjectData->classType, usedProperties, properties));

	for(i=0; i<usedProperties; i++)
	{
		properties[i].name=NULL;
		g_value_unset(&properties[i].value);
	}
	g_free(properties);

	if(!object)
	{
		g_debug("Failed to create object of type %s with %d properties to set", g_type_name(inObjectData->classType), usedProperties);

		/* Return NULL indicating error */
		return(NULL);
	}

	g_debug("Created object %p of type %s", object, G_OBJECT_TYPE_NAME(object));

	/* If object created has an ID and is derived from ClutterActor
	 * then set ID as name of actor if name was not set by any property.
	 * Otherwise only remember created ID.
	 */
	if(inObjectData->id)
	{
		if(CLUTTER_IS_ACTOR(object))
		{
			gchar							*name;

			g_object_get(object, "name", &name, NULL);
			if(!name || strlen(name)==0)
			{
				g_object_set(object, "name", inObjectData->id, NULL);
				g_debug("Object %s has ID but no name, setting ID '%s' as name",
							G_OBJECT_TYPE_NAME(object),
							inObjectData->id);
			}
			if(name) g_free(name);
		}

		g_hash_table_insert(ioIDs, g_strdup(inObjectData->id), object);
	}

	/* Create children */
	for(entry=inObjectData->children; entry; entry=g_slist_next(entry))
	{
		XfdashboardThemeLayoutParsedObject	*childObjectData;
		GObject								*child;

		childObjectData=(XfdashboardThemeLayoutParsedObject*)entry->data;

		/* Create child actor */
		child=_xfdashboard_theme_layout_create_object(self, childObjectData, ioIDs, ioUnresolvedIDs);
		if(!child || !CLUTTER_IS_ACTOR(child))
		{
			if(child)
			{
				g_debug("Child %s is not an actor and cannot be added to actor %s",
							G_OBJECT_TYPE_NAME(child),
							G_OBJECT_TYPE_NAME(object));
			}
				else
				{
					g_debug("Failed to create child for actor %s", G_OBJECT_TYPE_NAME(object));
				}

			/* Release allocated resources */
			if(child) g_object_unref(child);
			g_object_unref(object);

			/* Return NULL indicating error */
			return(NULL);
		}

		/* Add successfully created child actor to this actor */
		if(childObjectData->id) g_hash_table_insert(ioIDs, g_strdup(childObjectData->id), child);
		clutter_actor_add_child(CLUTTER_ACTOR(object), CLUTTER_ACTOR(child));
		g_debug("Created child %s and added to object %s",
					G_OBJECT_TYPE_NAME(child),
					G_OBJECT_TYPE_NAME(object));
	}

	/* Create layout */
	if(inObjectData->layout)
	{
		XfdashboardThemeLayoutParsedObject	*layoutObjectData;
		GObject								*layout;

		layoutObjectData=(XfdashboardThemeLayoutParsedObject*)inObjectData->layout;

		/* Create layout */
		layout=_xfdashboard_theme_layout_create_object(self, layoutObjectData, ioIDs, ioUnresolvedIDs);
		if(!layout || !CLUTTER_IS_LAYOUT_MANAGER(layout))
		{
			if(layout)
			{
				g_debug("Layout %s is not a layout manager and cannot be set at actor %s",
							G_OBJECT_TYPE_NAME(layout),
							G_OBJECT_TYPE_NAME(object));
			}
				else
				{
					g_debug("Failed to create layout manager for actor %s", G_OBJECT_TYPE_NAME(object));
				}

			/* Release allocated resources */
			if(layout) g_object_unref(layout);
			g_object_unref(object);

			/* Return NULL indicating error */
			return(NULL);
		}

		/* Add successfully created child actor to this actor */
		if(layoutObjectData->id) g_hash_table_insert(ioIDs, g_strdup(layoutObjectData->id), layout);
		clutter_actor_set_layout_manager(CLUTTER_ACTOR(object), CLUTTER_LAYOUT_MANAGER(layout));
		g_debug("Created layout manager %s and set at object %s",
					G_OBJECT_TYPE_NAME(layout),
					G_OBJECT_TYPE_NAME(object));
	}

	/* Create constraints */
	for(entry=inObjectData->constraints; entry; entry=g_slist_next(entry))
	{
		XfdashboardThemeLayoutParsedObject	*constraintObjectData;
		GObject								*constraint;

		constraintObjectData=(XfdashboardThemeLayoutParsedObject*)entry->data;

		/* Create constraint */
		constraint=_xfdashboard_theme_layout_create_object(self, constraintObjectData, ioIDs, ioUnresolvedIDs);
		if(!constraint || !CLUTTER_IS_CONSTRAINT(constraint))
		{
			if(constraint)
			{
				g_debug("Constraint %s is not a constraint and cannot be added to actor %s",
							G_OBJECT_TYPE_NAME(constraint),
							G_OBJECT_TYPE_NAME(object));
			}
				else
				{
					g_debug("Failed to create constraint for actor %s", G_OBJECT_TYPE_NAME(object));
				}

			/* Release allocated resources */
			if(constraint) g_object_unref(constraint);
			g_object_unref(object);

			/* Return NULL indicating error */
			return(NULL);
		}

		/* Add successfully created constraint to this actor */
		if(constraintObjectData->id) g_hash_table_insert(ioIDs, g_strdup(constraintObjectData->id), constraint);
		clutter_actor_add_constraint(CLUTTER_ACTOR(object), CLUTTER_CONSTRAINT(constraint));
		g_debug("Created constraint %s and added to object %s",
					G_OBJECT_TYPE_NAME(constraint),
					G_OBJECT_TYPE_NAME(object));
	}

	/* Set up properties which do reference other objects */
	for(entry=inObjectData->properties; entry; entry=g_slist_next(entry))
	{
		XfdashboardThemeLayoutTagData					*property;

		property=(XfdashboardThemeLayoutTagData*)entry->data;

		/* Check if property refers to an other object, if it does add it */
		if(property->tag.property.refID)
		{
			XfdashboardThemeLayoutUnresolvedBuildID		*unresolved;

			unresolved=g_new0(XfdashboardThemeLayoutUnresolvedBuildID, 1);
			unresolved->targetObject=g_object_ref(object);
			unresolved->property=_xfdashboard_theme_layout_tag_data_ref(property);

			*ioUnresolvedIDs=g_slist_prepend(*ioUnresolvedIDs, unresolved);
		}
	}

	/* Return created actor */
	return(object);
}

/* Callbacks used for <property> tag */
static void _xfdashboard_theme_layout_parse_property_text_node(GMarkupParseContext *inContext,
																const gchar *inText,
																gsize inTextLength,
																gpointer inUserData,
																GError **outError)
{
	XfdashboardThemeLayoutParserData		*data=(XfdashboardThemeLayoutParserData*)inUserData;
	XfdashboardThemeLayoutTagData			*tagData;

	/* Get tag data for property */
	if(g_queue_is_empty(data->stackTags))
	{
		_xfdashboard_theme_layout_parse_set_error(data,
													inContext,
													outError,
													XFDASHBOARD_THEME_LAYOUT_ERROR_ERROR,
													_("Unexpected empty tag stack when parsing property text node"));
		return;
	}

	tagData=(XfdashboardThemeLayoutTagData*)g_queue_peek_tail(data->stackTags);
	if(tagData->tag.property.value)
	{
		_xfdashboard_theme_layout_parse_set_error(data,
													inContext,
													outError,
													XFDASHBOARD_THEME_LAYOUT_ERROR_ERROR,
													_("Value for property '%s' is already set"),
													tagData->tag.property.name);
		return;
	}

	/* Store value for property */
	tagData->tag.property.value=g_strdup(inText);
}

static void _xfdashboard_theme_layout_parse_property_start(GMarkupParseContext *inContext,
															const gchar *inElementName,
															const gchar **inAttributeNames,
															const gchar **inAttributeValues,
															gpointer inUserData,
															GError **outError)
{
	XfdashboardThemeLayoutParserData		*data=(XfdashboardThemeLayoutParserData*)inUserData;
	gint									currentTag=TAG_DOCUMENT;

	/* Update last position for more accurate line and position in error messages */
	data->lastLine=data->currentLine;
	data->lastPosition=data->currentPostition;
	g_markup_parse_context_get_position(inContext, &data->currentLine, &data->currentPostition);

	/* Set error message */
	if(!g_queue_is_empty(data->stackTags))
	{
		XfdashboardThemeLayoutTagData		*tagData;

		tagData=(XfdashboardThemeLayoutTagData*)g_queue_peek_tail(data->stackTags);
		currentTag=tagData->tagType;
	}

	_xfdashboard_theme_layout_parse_set_error(data,
												inContext,
												outError,
												XFDASHBOARD_THEME_LAYOUT_ERROR_MALFORMED,
												_("Tag <%s> cannot contain tag <%s>"),
												_xfdashboard_theme_layout_get_tag_by_id(currentTag),
												inElementName);
}

/* General callbacks which can be used for any tag */
static void _xfdashboard_theme_layout_parse_general_no_text_nodes(GMarkupParseContext *inContext,
																	const gchar *inText,
																	gsize inTextLength,
																	gpointer inUserData,
																	GError **outError)
{
	XfdashboardThemeLayoutParserData		*data=(XfdashboardThemeLayoutParserData*)inUserData;
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

		_xfdashboard_theme_layout_parse_set_error(data,
													inContext,
													outError,
													XFDASHBOARD_THEME_LAYOUT_ERROR_MALFORMED,
													_("Unexpected text node '%s' at tag <%s>"),
													realText,
													parents ? (gchar*)parents->data : "document");
	}
	g_free(realText);
}

static void _xfdashboard_theme_layout_parse_general_start(GMarkupParseContext *inContext,
															const gchar *inElementName,
															const gchar **inAttributeNames,
															const gchar **inAttributeValues,
															gpointer inUserData,
															GError **outError)
{
	XfdashboardThemeLayoutParserData		*data=(XfdashboardThemeLayoutParserData*)inUserData;
	gint									currentTag=TAG_DOCUMENT;
	gint									nextTag;
	GError									*error=NULL;

	/* Update last position for more accurate line and position in error messages */
	data->lastLine=data->currentLine;
	data->lastPosition=data->currentPostition;
	g_markup_parse_context_get_position(inContext, &data->currentLine, &data->currentPostition);

	/* If we have not seen any tag yet then it is the document root */
	if(!g_queue_is_empty(data->stackTags))
	{
		XfdashboardThemeLayoutTagData		*tagData;

		tagData=(XfdashboardThemeLayoutTagData*)g_queue_peek_tail(data->stackTags);
		currentTag=tagData->tagType;
	}

	/* Get tag of next element */
	nextTag=_xfdashboard_theme_layout_get_tag_by_name(inElementName);
	if(nextTag==-1)
	{
		_xfdashboard_theme_layout_parse_set_error(data,
													inContext,
													outError,
													XFDASHBOARD_THEME_LAYOUT_ERROR_MALFORMED,
													_("Unknown tag <%s>"),
													inElementName);
		return;
	}

	/* Check if element name is <interface> and follows expected parent tags:
	 * <document>
	 */
	if(nextTag==TAG_INTERFACE &&
		currentTag==TAG_DOCUMENT)
	{
		XfdashboardThemeLayoutTagData		*tagData;

		/* Create tag data */
		tagData=_xfdashboard_theme_layout_tag_data_new(inContext, nextTag, &error);
		if(!tagData)
		{
			g_propagate_error(outError, error);
			return;
		}

		/* Get tag's attributes */
		if(!g_markup_collect_attributes(inElementName,
											inAttributeNames,
											inAttributeValues,
											&error,
											G_MARKUP_COLLECT_INVALID,
											NULL))
		{
			g_propagate_error(outError, error);
			_xfdashboard_theme_layout_tag_data_unref(tagData);
			return;
		}

		/* Push tag onto stack */
		g_queue_push_tail(data->stackTags, tagData);
		return;
	}

	/* Check if element name is <object> and follows expected parent tags:
	 * <interface>, <child>, <constraint>, <layout>
	 */
	if(nextTag==TAG_OBJECT &&
		(currentTag==TAG_INTERFACE ||
			currentTag==TAG_CHILD ||
			currentTag==TAG_CONSTRAINT ||
			currentTag==TAG_LAYOUT))
	{
		XfdashboardThemeLayoutTagData		*tagData;
		XfdashboardThemeLayoutParsedObject	*objectData;
		GType								expectedClassType=G_TYPE_INVALID;

		/* Create tag and object data */
		tagData=_xfdashboard_theme_layout_tag_data_new(inContext, nextTag, &error);
		if(!tagData)
		{
			g_propagate_error(outError, error);
			return;
		}

		objectData=_xfdashboard_theme_layout_object_data_new(inContext, currentTag, &error);
		if(!objectData)
		{
			g_propagate_error(outError, error);
			_xfdashboard_theme_layout_tag_data_unref(tagData);
			return;
		}

		/* Get tag's attributes */
		if(!g_markup_collect_attributes(inElementName,
											inAttributeNames,
											inAttributeValues,
											&error,
											G_MARKUP_COLLECT_STRDUP | G_MARKUP_COLLECT_OPTIONAL,
											"id",
											&tagData->tag.object.id,
											G_MARKUP_COLLECT_STRDUP,
											"class",
											&tagData->tag.object.class,
											G_MARKUP_COLLECT_INVALID))
		{
			g_propagate_error(outError, error);
			_xfdashboard_theme_layout_tag_data_unref(tagData);
			_xfdashboard_theme_layout_object_data_unref(objectData);
			return;
		}

		/* Check tag's attributes */
		if(tagData->tag.object.id)
		{
			objectData->id=g_strdup(tagData->tag.object.id);
			if(strlen(objectData->id)==0)
			{
				_xfdashboard_theme_layout_parse_set_error(data,
															inContext,
															outError,
															XFDASHBOARD_THEME_LAYOUT_ERROR_MALFORMED,
															_("Empty ID at tag '%s'"),
															inElementName);
				_xfdashboard_theme_layout_tag_data_unref(tagData);
				_xfdashboard_theme_layout_object_data_unref(objectData);
				return;
			}

			if(!xfdashboard_is_valid_id(tagData->tag.object.id))
			{
				_xfdashboard_theme_layout_parse_set_error(data,
															inContext,
															outError,
															XFDASHBOARD_THEME_LAYOUT_ERROR_MALFORMED,
															_("Invalid ID '%s' at tag '%s'"),
															tagData->tag.object.id,
															inElementName);
				_xfdashboard_theme_layout_tag_data_unref(tagData);
				_xfdashboard_theme_layout_object_data_unref(objectData);
				return;
			}
		}

		objectData->classType=_xfdashboard_theme_layout_resolve_type_lazy(tagData->tag.object.class);
		if(objectData->classType==G_TYPE_INVALID)
		{
			_xfdashboard_theme_layout_parse_set_error(data,
														inContext,
														outError,
														XFDASHBOARD_THEME_LAYOUT_ERROR_MALFORMED,
														_("Unknown object class %s for tag '%s'"),
														tagData->tag.object.class,
														inElementName);
			_xfdashboard_theme_layout_tag_data_unref(tagData);
			_xfdashboard_theme_layout_object_data_unref(objectData);
			return;
		}

		if(currentTag==TAG_INTERFACE) expectedClassType=CLUTTER_TYPE_ACTOR;
			else if(currentTag==TAG_CHILD) expectedClassType=CLUTTER_TYPE_ACTOR;
			else if(currentTag==TAG_CONSTRAINT) expectedClassType=CLUTTER_TYPE_CONSTRAINT;
			else if(currentTag==TAG_LAYOUT) expectedClassType=CLUTTER_TYPE_LAYOUT_MANAGER;

		g_assert(expectedClassType!=G_TYPE_INVALID);
		if(!g_type_is_a(objectData->classType, expectedClassType))
		{
			_xfdashboard_theme_layout_parse_set_error(data,
														inContext,
														outError,
														XFDASHBOARD_THEME_LAYOUT_ERROR_MALFORMED,
														_("Invalid class %s in object for parent tag <%s> - expecting class derived from %s"),
														tagData->tag.object.class,
														_xfdashboard_theme_layout_get_tag_by_id(currentTag),
														g_type_name(expectedClassType));
			_xfdashboard_theme_layout_tag_data_unref(tagData);
			_xfdashboard_theme_layout_object_data_unref(objectData);
			return;
		}

		/* Push tag onto stack */
		g_queue_push_tail(data->stackTags, tagData);

		/* Push object onto stack */
		g_queue_push_tail(data->stackObjects, objectData);
		return;
	}

	/* Check if element name is <child>, <layout> or <constraint> and follows expected parent tags:
	 * <object>
	 */
	if((nextTag==TAG_CHILD || nextTag==TAG_LAYOUT || nextTag==TAG_CONSTRAINT) &&
		currentTag==TAG_OBJECT)
	{
		XfdashboardThemeLayoutTagData		*tagData;
		XfdashboardThemeLayoutParsedObject	*parentObject;

		/* <layout> and <constraint> can only be set for parent objects derived of type ClutterActor */
		parentObject=(XfdashboardThemeLayoutParsedObject*)g_queue_peek_tail(data->stackObjects);
		if(!parentObject ||
			!g_type_is_a(parentObject->classType, CLUTTER_TYPE_ACTOR))
		{
			_xfdashboard_theme_layout_parse_set_error(data,
														inContext,
														outError,
														XFDASHBOARD_THEME_LAYOUT_ERROR_MALFORMED,
														_("Tag <%s> can only be set at <%s> creating objects derived from class %s"),
														inElementName,
														_xfdashboard_theme_layout_get_tag_by_id(currentTag),
														g_type_name(CLUTTER_TYPE_ACTOR));
			return;
		}

		/* Create tag data */
		tagData=_xfdashboard_theme_layout_tag_data_new(inContext, nextTag, &error);
		if(!tagData)
		{
			g_propagate_error(outError, error);
			return;
		}

		/* Get tag's attributes */
		if(!g_markup_collect_attributes(inElementName,
											inAttributeNames,
											inAttributeValues,
											&error,
											G_MARKUP_COLLECT_INVALID,
											NULL))
		{
			g_propagate_error(outError, error);
			_xfdashboard_theme_layout_tag_data_unref(tagData);
			return;
		}

		/* Push tag onto stack */
		g_queue_push_tail(data->stackTags, tagData);
		return;
	}

	/* Check if element name is <property> and follows expected parent tags:
	 * <object>
	 */
	if(nextTag==TAG_PROPERTY && currentTag==TAG_OBJECT)
	{
		XfdashboardThemeLayoutTagData		*tagData;
		static GMarkupParser				propertyParser=
											{
												_xfdashboard_theme_layout_parse_property_start,
												NULL,
												_xfdashboard_theme_layout_parse_property_text_node,
												NULL,
												NULL,
											};

		/* Create tag data */
		tagData=_xfdashboard_theme_layout_tag_data_new(inContext, nextTag, &error);
		if(!tagData)
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
											"name",
											&tagData->tag.property.name,
											G_MARKUP_COLLECT_BOOLEAN | G_MARKUP_COLLECT_OPTIONAL,
											"translatable",
											&tagData->tag.property.translatable,
											G_MARKUP_COLLECT_STRDUP | G_MARKUP_COLLECT_OPTIONAL,
											"ref",
											&tagData->tag.property.refID,
											G_MARKUP_COLLECT_INVALID))
		{
			g_propagate_error(outError, error);
			_xfdashboard_theme_layout_tag_data_unref(tagData);
			return;
		}

		/* Check tag's attributes */
		if(tagData->tag.property.refID &&
			strlen(tagData->tag.property.refID)==0)
		{
			_xfdashboard_theme_layout_parse_set_error(data,
														inContext,
														outError,
														XFDASHBOARD_THEME_LAYOUT_ERROR_MALFORMED,
														_("Attribute 'ref' cannot be empty at tag <%s>"),
														inElementName);
			_xfdashboard_theme_layout_tag_data_unref(tagData);
			return;
		}

		/* Push tag onto stack */
		g_queue_push_tail(data->stackTags, tagData);

		/* Properties can have text nodes but no children so set up context */
		g_markup_parse_context_push(inContext, &propertyParser, inUserData);
		return;
	}

	/* If we get here the given element name cannot follow this tag */
	_xfdashboard_theme_layout_parse_set_error(data,
												inContext,
												outError,
												XFDASHBOARD_THEME_LAYOUT_ERROR_MALFORMED,
												_("Tag <%s> cannot contain tag <%s>"),
												_xfdashboard_theme_layout_get_tag_by_id(currentTag),
												inElementName);
}

static void _xfdashboard_theme_layout_parse_general_end(GMarkupParseContext *inContext,
															const gchar *inElementName,
															gpointer inUserData,
															GError **outError)
{
	XfdashboardThemeLayoutParserData			*data=(XfdashboardThemeLayoutParserData*)inUserData;
	XfdashboardThemeLayoutTagData				*tagData;
	XfdashboardThemeLayoutTagData				*subTagData;

	/* Get last tag from stack */
	subTagData=(XfdashboardThemeLayoutTagData*)g_queue_pop_tail(data->stackTags);
	if(!subTagData)
	{
		_xfdashboard_theme_layout_parse_set_error(data,
													inContext,
													outError,
													XFDASHBOARD_THEME_LAYOUT_ERROR_ERROR,
													_("Internal error when handling end of tag <%s>"),
													inElementName);
		return;
	}

	/* Get tag data for this element */
	tagData=(XfdashboardThemeLayoutTagData*)g_queue_peek_tail(data->stackTags);

	/* Handle end of element <object> */
	if(subTagData->tagType==TAG_OBJECT)
	{
		XfdashboardThemeLayoutParsedObject		*objectData;
		XfdashboardThemeLayoutParsedObject		*parentObjectData;

		objectData=(XfdashboardThemeLayoutParsedObject*)g_queue_pop_tail(data->stackObjects);
		parentObjectData=(XfdashboardThemeLayoutParsedObject*)g_queue_peek_tail(data->stackObjects);

		/* Object is a child of <interface> */
		if(tagData->tagType==TAG_INTERFACE)
		{
			g_assert(parentObjectData==NULL);

			/* There can be only one <interface> per file */
			if(data->interface)
			{
				_xfdashboard_theme_layout_parse_set_error(data,
															inContext,
															outError,
															XFDASHBOARD_THEME_LAYOUT_ERROR_ERROR,
															_("Document can have only one <%s>"),
															_xfdashboard_theme_layout_get_tag_by_id(subTagData->tagType));
				_xfdashboard_theme_layout_tag_data_unref(subTagData);
				_xfdashboard_theme_layout_object_data_unref(objectData);
				return;
			}

			/* Append interface to list of known interface */
			data->interface=_xfdashboard_theme_layout_object_data_ref(objectData);
		}

		/* Object is a child of <child> */
		if(tagData->tagType==TAG_CHILD)
		{
			g_assert(parentObjectData!=NULL);

			parentObjectData->children=g_slist_append(parentObjectData->children, _xfdashboard_theme_layout_object_data_ref(objectData));
		}

		/* Object is a child of <constraint> */
		if(tagData->tagType==TAG_CONSTRAINT)
		{
			g_assert(parentObjectData!=NULL);

			parentObjectData->constraints=g_slist_append(parentObjectData->constraints, _xfdashboard_theme_layout_object_data_ref(objectData));
		}

		/* Object is a child of <layout> */
		if(tagData->tagType==TAG_LAYOUT)
		{
			g_assert(parentObjectData!=NULL);

			if(parentObjectData->layout)
			{
				_xfdashboard_theme_layout_parse_set_error(data,
															inContext,
															outError,
															XFDASHBOARD_THEME_LAYOUT_ERROR_ERROR,
															_("Object can have only one <%s>"),
															_xfdashboard_theme_layout_get_tag_by_id(subTagData->tagType));
				_xfdashboard_theme_layout_tag_data_unref(subTagData);
				_xfdashboard_theme_layout_object_data_unref(objectData);
				return;
			}

			parentObjectData->layout=_xfdashboard_theme_layout_object_data_ref(objectData);
		}

		/* Unreference last object's data */
		_xfdashboard_theme_layout_object_data_unref(objectData);
	}

	/* Handle end of element <property> */
	if(subTagData->tagType==TAG_PROPERTY)
	{
		XfdashboardThemeLayoutParsedObject		*objectData;

		/* Add property to object */
		objectData=(XfdashboardThemeLayoutParsedObject*)g_queue_peek_tail(data->stackObjects);
		objectData->properties=g_slist_append(objectData->properties, _xfdashboard_theme_layout_tag_data_ref(subTagData));
		g_debug("Adding property '%s' with %s '%s' to object %s",
					subTagData->tag.property.name,
					subTagData->tag.property.refID ? "referenced object of ID" : "value",
					subTagData->tag.property.refID ? subTagData->tag.property.refID : subTagData->tag.property.value,
					g_type_name(objectData->classType));

		/* Restore previous parser context */
		g_markup_parse_context_pop(inContext);
	}

	/* Unreference last tag's data */
	_xfdashboard_theme_layout_tag_data_unref(subTagData);
}

/* Check for duplicate IDs and unresolvable refIDs */
static void _xfdashboard_theme_layout_check_refids(gpointer inData, gpointer inUserData)
{
	XfdashboardThemeLayoutParsedObject	*object;
	GHashTable							*ids;
	gpointer							value;
	GSList								*entry;
	XfdashboardThemeLayoutTagData		*property;

	g_return_if_fail(inData);
	g_return_if_fail(inUserData);

	object=(XfdashboardThemeLayoutParsedObject*)inData;
	ids=(GHashTable*)inUserData;

	/* Iterate through properties and check referenced IDs */
	for(entry=object->properties; entry; entry=g_slist_next(entry))
	{
		property=(XfdashboardThemeLayoutTagData*)entry->data;

		/* If property references an ID then lookup key in hashtable.
		 * If not found create key with ID and value of 1.
		 * If found do nothing (keep value at zero).
		 */
		if(property->tag.property.refID &&
			!g_hash_table_lookup_extended(ids, property->tag.property.refID, NULL, &value))
		{
			g_hash_table_insert(ids, property->tag.property.refID, GINT_TO_POINTER(1));
			g_debug("Could not resolve referenced ID '%s', set counter to 1", property->tag.property.refID);
		}
			else if(property->tag.property.refID)
			{
				g_debug("Referenced ID '%s' resolved successfully", property->tag.property.refID);
			}
	}

	/* Call ourselve for each constraint object */
	g_slist_foreach(object->constraints, _xfdashboard_theme_layout_check_refids, inUserData);

	/* Call ourselve for layout object */
	if(object->layout) _xfdashboard_theme_layout_check_refids(object->layout, inUserData);

	/* Call ourselve for each child object */
	g_slist_foreach(object->children, _xfdashboard_theme_layout_check_refids, inUserData);
}

static void _xfdashboard_theme_layout_check_ids(gpointer inData, gpointer inUserData)
{
	XfdashboardThemeLayoutParsedObject	*object;
	GHashTable							*ids;
	gpointer							value;

	g_return_if_fail(inData);
	g_return_if_fail(inUserData);

	object=(XfdashboardThemeLayoutParsedObject*)inData;
	ids=(GHashTable*)inUserData;

	/* If ID at object is set then lookup key in hashtable.
	 * If not found create key with ID and value of 1.
	 * If found increase value.
	 */
	if(object->id)
	{
		/* If key does not exist create it with value of 1 ... */
		if(!g_hash_table_lookup_extended(ids, object->id, NULL, &value))
		{
			g_hash_table_insert(ids, object->id, GINT_TO_POINTER(1));
			g_debug("First occurence of ID '%s', set counter to 1", object->id);
		}
			/* ... otherwise increase value of key */
			else
			{
				gint					count;

				count=GPOINTER_TO_INT(value);
				count++;
				g_hash_table_replace(ids, object->id, GINT_TO_POINTER(count));
				g_debug("Found ID '%s' and increased counter to %d", object->id, count);
			}
	}

	/* Call ourselve for each constraint object */
	g_slist_foreach(object->constraints, _xfdashboard_theme_layout_check_ids, inUserData);

	/* Call ourselve for layout object */
	if(object->layout) _xfdashboard_theme_layout_check_ids(object->layout, inUserData);

	/* Call ourselve for each child object */
	g_slist_foreach(object->children, _xfdashboard_theme_layout_check_ids, inUserData);
}

static gboolean _xfdashboard_theme_layout_check_ids_and_refids(XfdashboardThemeLayout *self,
																XfdashboardThemeLayoutParsedObject *inInterfaceObject,
																GError **outError)
{
	gboolean			success;
	GHashTable			*ids;
	GHashTableIter		iter;
	gpointer			key, value;

	g_return_val_if_fail(XFDASHBOARD_IS_THEME_LAYOUT(self), FALSE);
	g_return_val_if_fail(inInterfaceObject, FALSE);
	g_return_val_if_fail(outError==NULL || *outError==NULL, FALSE);

	success=TRUE;

	/* The idea to check for duplicate IDs and if all referenced IDs are
	 * resolvable is a two-step full iteration over all objects with help
	 * of a hashtable which uses the ID as key and the value indicates
	 * check results.
	 */
	g_object_ref(self);
	ids=g_hash_table_new(g_str_hash, g_str_equal);

	/* Step one: Iterate through all objects and their contraints, layouts
	 * and children recursively beginning at interface object. For each ID
	 * specified lookup key (containing ID) in hashtable. If key is found
	 * increase counter (which is an error because ID is used more than once)
	 * or create key with integer value 1.
	 */
	_xfdashboard_theme_layout_check_ids(inInterfaceObject, ids);

	/* Check if any key in hashtable has a value greater than one which means
	 * that this ID is used more than once and is an error. Reset each key
	 * check successfully to zero.
	 */
	g_hash_table_iter_init(&iter, ids);
	while(success && g_hash_table_iter_next(&iter, &key, &value)) 
	{
		if(GPOINTER_TO_INT(value)>1)
		{
			g_set_error(outError,
						XFDASHBOARD_THEME_LAYOUT_ERROR,
						XFDASHBOARD_THEME_LAYOUT_ERROR_MALFORMED,
						_("ID '%s' was specified more than once (%d times)"),
						(const gchar*)key,
						GPOINTER_TO_INT(value));
			success=FALSE;
		}
			else
			{
				g_hash_table_iter_replace(&iter, GINT_TO_POINTER(0));
			}
	}

	/* Step two: Iterate through all properties of all objects and their
	 * children recursively. For each property using a reference ID lookup
	 * key (containing ID) in hashtable. If found do nothing and if was
	 * not found create key with integer value 1.
	 */
	if(success) _xfdashboard_theme_layout_check_refids(inInterfaceObject, ids);

	/* Check if any key in hashtable has a value greater than zero which
	 * means that this referenced ID could not be resolved and is an error.
	 */
	if(success)
	{
		g_hash_table_iter_init(&iter, ids);
		while(success && g_hash_table_iter_next(&iter, &key, &value)) 
		{
			if(GPOINTER_TO_INT(value)>0)
			{
				g_set_error(outError,
							XFDASHBOARD_THEME_LAYOUT_ERROR,
							XFDASHBOARD_THEME_LAYOUT_ERROR_MALFORMED,
							_("Referenced ID '%s' could not be resolved"),
							(const gchar*)key);
				success=FALSE;
			}
		}
	}

	/* Release allocated resources */
	g_hash_table_destroy(ids);
	g_object_unref(self);

	/* Return result of checks */
	return(success);
}

/* Parse XML from string */
static gboolean _xfdashboard_theme_layout_parse_xml(XfdashboardThemeLayout *self,
													const gchar *inPath,
													const gchar *inContents,
													GError **outError)
{
	static GMarkupParser				parser=
										{
											_xfdashboard_theme_layout_parse_general_start,
											_xfdashboard_theme_layout_parse_general_end,
											_xfdashboard_theme_layout_parse_general_no_text_nodes,
											NULL,
											NULL,
										};

	XfdashboardThemeLayoutPrivate		*priv;
	XfdashboardThemeLayoutParserData	*data;
	GMarkupParseContext					*context;
	GError								*error;
	gboolean							success;

	g_return_val_if_fail(XFDASHBOARD_IS_THEME_LAYOUT(self), FALSE);
	g_return_val_if_fail(inPath && *inPath, FALSE);
	g_return_val_if_fail(inContents && *inContents, FALSE);
	g_return_val_if_fail(outError==NULL || *outError==NULL, FALSE);

	priv=self->priv;
	error=NULL;
	success=TRUE;

	/* Create and set up parser instance */
	data=g_new0(XfdashboardThemeLayoutParserData, 1);
	if(!data)
	{
		/* Set error */
		g_set_error(outError,
					XFDASHBOARD_THEME_LAYOUT_ERROR,
					XFDASHBOARD_THEME_LAYOUT_ERROR_ERROR,
					_("Could not set up parser data for file %s"),
					inPath);
		return(FALSE);
	}

	context=g_markup_parse_context_new(&parser, 0, data, NULL);
	if(!context)
	{
		/* Set error */
		g_set_error(outError,
					XFDASHBOARD_THEME_LAYOUT_ERROR,
					XFDASHBOARD_THEME_LAYOUT_ERROR_ERROR,
					_("Could not create parser for file %s"),
					inPath);

		g_free(data);
		return(FALSE);
	}

	/* Now the parser and its context is set up and we can now
	 * safely initialize data.
	 */
	data->self=self;
	data->stackObjects=g_queue_new();
	data->stackTags=g_queue_new();
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

	if(success && !data->interface)
	{
		g_set_error(outError,
					XFDASHBOARD_THEME_LAYOUT_ERROR,
					XFDASHBOARD_THEME_LAYOUT_ERROR_ERROR,
					_("File %s does not contain an interface"),
					inPath);
		success=FALSE;
	}

	if(success && !data->interface->id)
	{
		g_set_error(outError,
					XFDASHBOARD_THEME_LAYOUT_ERROR,
					XFDASHBOARD_THEME_LAYOUT_ERROR_ERROR,
					_("Interface at file %s has no ID"),
					inPath);
		success=FALSE;
	}

	if(success && !_xfdashboard_theme_layout_check_ids_and_refids(self, data->interface, &error))
	{
		g_propagate_error(outError, error);
		success=FALSE;
	}

	/* Handle collected data if parsing was successful */
	if(success)
	{
		priv->interfaces=g_slist_append(priv->interfaces, _xfdashboard_theme_layout_object_data_ref(data->interface));
	}

	/* Release allocated resources */
	g_markup_parse_context_free(context);

	if(data->interface) _xfdashboard_theme_layout_object_data_unref(data->interface);

	if(!g_queue_is_empty(data->stackObjects))
	{
		g_assert(!success);

		g_queue_foreach(data->stackObjects, (GFunc)_xfdashboard_theme_layout_object_data_free_foreach_callback, self);
	}
	g_queue_free(data->stackObjects);

	if(!g_queue_is_empty(data->stackTags))
	{
		g_assert(!success);

		g_queue_foreach(data->stackTags, (GFunc)_xfdashboard_theme_layout_tag_data_free_foreach_callback, self);
	}
	g_queue_free(data->stackTags);

	g_free(data);

	/* Return success result */
#ifdef DEBUG
	if(!success)
	{
		g_slist_foreach(priv->interfaces, (GFunc)_xfdashboard_theme_layout_print_parsed_objects, "Interface:");
		g_debug("PARSER ERROR: %s", outError ? (*outError)->message : "unknown error");
	}
#endif

	return(success);
}

/* IMPLEMENTATION: GObject */

/* Dispose this object */
static void _xfdashboard_theme_layout_dispose(GObject *inObject)
{
	XfdashboardThemeLayout				*self=XFDASHBOARD_THEME_LAYOUT(inObject);
	XfdashboardThemeLayoutPrivate		*priv=self->priv;

	/* Release allocated resources */
	if(priv->interfaces)
	{
		g_slist_foreach(priv->interfaces, _xfdashboard_theme_layout_object_data_free_foreach_callback, NULL);
		g_slist_free(priv->interfaces);
		priv->interfaces=NULL;
	}

	/* Call parent's class dispose method */
	G_OBJECT_CLASS(xfdashboard_theme_layout_parent_class)->dispose(inObject);
}

/* Class initialization
 * Override functions in parent classes and define properties
 * and signals
 */
void xfdashboard_theme_layout_class_init(XfdashboardThemeLayoutClass *klass)
{
	GObjectClass		*gobjectClass=G_OBJECT_CLASS(klass);

	/* Override functions */
	gobjectClass->dispose=_xfdashboard_theme_layout_dispose;

	/* Set up private structure */
	g_type_class_add_private(klass, sizeof(XfdashboardThemeLayoutPrivate));
}

/* Object initialization
 * Create private structure and set up default values
 */
void xfdashboard_theme_layout_init(XfdashboardThemeLayout *self)
{
	XfdashboardThemeLayoutPrivate		*priv;

	priv=self->priv=XFDASHBOARD_THEME_LAYOUT_GET_PRIVATE(self);

	/* Set default values */
	priv->interfaces=NULL;
}

/* Implementation: Errors */

GQuark xfdashboard_theme_layout_error_quark(void)
{
	return(g_quark_from_static_string("xfdashboard-theme-layout-error-quark"));
}

/* Implementation: Public API */

/* Create new instance */
XfdashboardThemeLayout* xfdashboard_theme_layout_new(void)
{
	return(XFDASHBOARD_THEME_LAYOUT(g_object_new(XFDASHBOARD_TYPE_THEME_LAYOUT, NULL)));
}

/* Load a XML file into theme */
gboolean xfdashboard_theme_layout_add_file(XfdashboardThemeLayout *self,
											const gchar *inPath,
											GError **outError)
{
	gchar								*contents;
	gsize								contentsLength;
	GError								*error;

	g_return_val_if_fail(XFDASHBOARD_IS_THEME_LAYOUT(self), FALSE);
	g_return_val_if_fail(inPath!=NULL && *inPath!=0, FALSE);
	g_return_val_if_fail(outError==NULL || *outError==NULL, FALSE);

	/* Load XML file, parse it and build objects from file */
	error=NULL;
	if(!g_file_get_contents(inPath, &contents, &contentsLength, &error))
	{
		g_propagate_error(outError, error);
		return(FALSE);
	}

	_xfdashboard_theme_layout_parse_xml(self, inPath, contents, &error);
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

/* Build requested interface */
ClutterActor* xfdashboard_theme_layout_build_interface(XfdashboardThemeLayout *self,
														const gchar *inID)
{
	XfdashboardThemeLayoutPrivate				*priv;
	GSList										*entry;
	XfdashboardThemeLayoutParsedObject			*interfaceData;
	ClutterActor								*actor;
	GHashTable									*ids;
	GSList										*unresolved;

	g_return_val_if_fail(XFDASHBOARD_IS_THEME_LAYOUT(self), NULL);
	g_return_val_if_fail(inID!=NULL && *inID!=0, NULL);

	priv=self->priv;
	interfaceData=NULL;
	unresolved=NULL;

	/* Find parsed object data for requested interface by its ID */
	entry=priv->interfaces;
	while(entry && !interfaceData)
	{
		XfdashboardThemeLayoutParsedObject		*objectData;

		objectData=(XfdashboardThemeLayoutParsedObject*)entry->data;

		/* Check if this object data is for the requested interface */
		if(g_strcmp0(objectData->id, inID)==0) interfaceData=objectData;

		/* Continue with next entry */
		entry=g_slist_next(entry);
	}

	/* If we get here and we did not found the requested interface
	 * object data, return NULL to indicate error.
	 */
	if(!interfaceData)
	{
		g_debug("Could not find object data for interface '%s'", inID);
		return(NULL);
	}

	/* Create actor */
	_xfdashboard_theme_layout_object_data_ref(interfaceData);

	ids=g_hash_table_new_full(g_str_hash, g_str_equal, g_free, NULL);
	
	actor=CLUTTER_ACTOR(_xfdashboard_theme_layout_create_object(self, interfaceData, ids, &unresolved));
	if(actor)
	{
		g_debug("Created actor %s for interface '%s'", G_OBJECT_TYPE_NAME(actor), inID);

		/* Resolved unresolved properties of newly created object */
		_xfdashboard_theme_layout_create_object_resolve_unresolved(self, ids, unresolved);
	}
		else g_debug("Failed to create actor for interface '%s'", inID);

	if(ids) g_hash_table_destroy(ids);
	if(unresolved) g_slist_free_full(unresolved, _xfdashboard_theme_layout_create_object_free_unresolved);

	_xfdashboard_theme_layout_object_data_unref(interfaceData);

	/* Return created actor */
	return(actor);
}
