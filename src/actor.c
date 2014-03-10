/*
 * actor: Abstract base actor
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

#include "actor.h"

#include <glib/gi18n-lib.h>

#include "application.h"
#include "utils.h"

/* Define this class in GObject system */
static gpointer				xfdashboard_actor_parent_class=NULL;

void xfdashboard_actor_class_init(XfdashboardActorClass *klass);
void xfdashboard_actor_init(XfdashboardActor *self);
void xfdashboard_actor_base_class_finalize(XfdashboardActorClass *klass);

/* Private structure - access only by public API if needed */
#define XFDASHBOARD_ACTOR_GET_PRIVATE(obj) \
	(G_TYPE_INSTANCE_GET_PRIVATE((obj), XFDASHBOARD_TYPE_ACTOR, XfdashboardActorPrivate))

struct _XfdashboardActorPrivate
{
	/* Properties related */
	gchar			*styleClasses;
	gchar			*stylePseudoClasses;

	/* Instance related */
	GHashTable		*lastThemeStyleSet;
};

/* Properties */
enum
{
	PROP_0,

	PROP_STYLE_CLASSES,
	PROP_STYLE_PSEUDO_CLASSES,

	PROP_LAST
};

static GParamSpec* XfdashboardActorProperties[PROP_LAST]={ 0, };


/* IMPLEMENTATION: Private variables and methods */
static GParamSpecPool		*_xfdashboard_actor_stylable_properties_pool=NULL;

#define XFDASHBOARD_ACTOR_PARAM_SPEC_REF		(_xfdashboard_actor_param_spec_ref_quark())

/* Quark declarations */
static GQuark _xfdashboard_actor_param_spec_ref_quark(void)
{
	return(g_quark_from_static_string("xfdashboard-actor-param-spec-ref-quark"));
}

/* GValue transformation function for G_TYPE_STRING to various other types */
static void _xfdashboard_actor_gvalue_transform_string_int(const GValue *inSourceValue, GValue *ioDestValue)
{
	ioDestValue->data[0].v_int=g_ascii_strtoll(inSourceValue->data[0].v_pointer, NULL, 10);
}

static void _xfdashboard_actor_gvalue_transform_string_uint(const GValue *inSourceValue, GValue *ioDestValue)
{
	ioDestValue->data[0].v_uint=g_ascii_strtoull(inSourceValue->data[0].v_pointer, NULL, 10);
}

static void _xfdashboard_actor_gvalue_transform_string_long(const GValue *inSourceValue, GValue *ioDestValue)
{
	ioDestValue->data[0].v_long=g_ascii_strtoll(inSourceValue->data[0].v_pointer, NULL, 10);
}

static void _xfdashboard_actor_gvalue_transform_string_ulong(const GValue *inSourceValue, GValue *ioDestValue)
{
	ioDestValue->data[0].v_ulong=g_ascii_strtoull(inSourceValue->data[0].v_pointer, NULL, 10);
}

static void _xfdashboard_actor_gvalue_transform_string_int64(const GValue *inSourceValue, GValue *ioDestValue)
{
	ioDestValue->data[0].v_int64=g_ascii_strtoll(inSourceValue->data[0].v_pointer, NULL, 10);
}

static void _xfdashboard_actor_gvalue_transform_string_uint64(const GValue *inSourceValue, GValue *ioDestValue)
{
	ioDestValue->data[0].v_uint64=g_ascii_strtoull(inSourceValue->data[0].v_pointer, NULL, 10);
}

static void _xfdashboard_actor_gvalue_transform_string_float(const GValue *inSourceValue, GValue *ioDestValue)
{
	ioDestValue->data[0].v_float=g_ascii_strtod(inSourceValue->data[0].v_pointer, NULL);
}

static void _xfdashboard_actor_gvalue_transform_string_double(const GValue *inSourceValue, GValue *ioDestValue)
{
	ioDestValue->data[0].v_double=g_ascii_strtod(inSourceValue->data[0].v_pointer, NULL);
}

static void _xfdashboard_actor_gvalue_transform_string_boolean(const GValue *inSourceValue, GValue *ioDestValue)
{
	/* Convert case-insentive "true" to TRUE */
	if(!g_ascii_strncasecmp(inSourceValue->data[0].v_pointer, "true", 4))
	{
		ioDestValue->data[0].v_int=TRUE;
	}
		/* Convert case-insentive "false" to FALSE */
		else if(!g_ascii_strncasecmp(inSourceValue->data[0].v_pointer, "false", 5))
		{
			ioDestValue->data[0].v_int=FALSE;
		}
		/* Convert to unsigned integer if set destination to TRUE if non-zero
		 * otherweise set destination to FALSE
		 */
		else
		{
			ioDestValue->data[0].v_int=(g_ascii_strtoull(inSourceValue->data[0].v_pointer, NULL, 10)!=0 ? TRUE : FALSE);
		}
}

static void _xfdashboard_actor_gvalue_transform_string_enum(const GValue *inSourceValue, GValue *ioDestValue)
{
	GEnumClass		*enumClass;
	GEnumValue		*enumValue;
	const gchar		*value;

	/* Reference enum class to keep it alive for transformation */
	enumClass=g_type_class_ref(G_VALUE_TYPE(ioDestValue));

	/* Get value to convert */
	value=(const gchar*)inSourceValue->data[0].v_pointer;

	/* Get enum value either by name or by nickname (whatever matches first) */
	enumValue=g_enum_get_value_by_name(enumClass, value);
	if(!enumValue) enumValue=g_enum_get_value_by_nick(enumClass, value);

	/* Set value if enum could be found otherwise set 0 */
	if(enumValue) ioDestValue->data[0].v_int=enumValue->value;
		else
		{
			ioDestValue->data[0].v_int=0;
			g_debug("Cannot get value for unknown enum '%s' for type %s",
						value, g_type_name(G_VALUE_TYPE(ioDestValue)));
		}

	/* Release allocated resources */
	g_type_class_unref(enumClass);
}

static void _xfdashboard_actor_gvalue_transform_string_flags(const GValue *inSourceValue, GValue *ioDestValue)
{
	GFlagsClass		*flagsClass;
	GFlagsValue		*flagsValue;
	guint			finalValue;
	gchar			**values, **entry;

	/* Reference flags class to keep it alive for transformation */
	flagsClass=g_type_class_ref(G_VALUE_TYPE(ioDestValue));

	/* Split string into space-separated needles and lookup each needle
	 * for a match and add found values OR'ed to final value
	 */
	finalValue=0;
	entry=values=g_strsplit(inSourceValue->data[0].v_pointer, " ", 0);
	while(*entry)
	{
		/* Do not look-up empty values */
		if(!entry[0]) continue;

		/* Get flags value either by name or by nickname (whatever matches first) */
		flagsValue=g_flags_get_value_by_name(flagsClass, *entry);
		if(!flagsValue) flagsValue=g_flags_get_value_by_nick(flagsClass, *entry);

		/* Add value OR'ed if flags could be found */
		if(flagsValue) finalValue|=flagsValue->value;
			else
			{
				g_debug("Cannot get value for unknown flag '%s' for type %s",
							*entry, g_type_name(G_VALUE_TYPE(ioDestValue)));
			}

		/* Continue with next entry */
		entry++;
	}
	g_strfreev(values);

	/* Set value */
	ioDestValue->data[0].v_uint=finalValue;

	/* Release allocated resources */
	g_type_class_unref(flagsClass);
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

/* Actor was mapped or unmapped */
static void _xfdashboard_actor_on_mapped_changed(GObject *inObject,
													GParamSpec *inSpec,
													gpointer inUserData)
{
	XfdashboardActor		*self;

	g_return_if_fail(XFDASHBOARD_IS_ACTOR(inObject));

	self=XFDASHBOARD_ACTOR(inObject);

	/* Invalide styling to get it recomputed */
	xfdashboard_actor_style_invalidate(self);
}

/* Actor was (re)named */
static void _xfdashboard_actor_on_name_changed(GObject *inObject,
												GParamSpec *inSpec,
												gpointer inUserData)
{
	XfdashboardActor		*self;

	g_return_if_fail(XFDASHBOARD_IS_ACTOR(inObject));

	self=XFDASHBOARD_ACTOR(inObject);

	/* Invalide styling to get it recomputed because its ID (from point
	 * of view of css) has changed
	 */
	xfdashboard_actor_style_invalidate(self);
}

/* Check if haystack contains needle.
 * The haystack is a string representing a list which entries is seperated
 * by a seperator character. This function looks up the haystack if it
 * contains an entry matching the needle and returns TRUE in this case.
 * Otherwise FALSE is returned. A needle length of -1 signals that needle
 * is a NULL-terminated string and length should be determine automatically.
 */
static gboolean _xfdashboard_actor_list_contains(const gchar *inNeedle,
													gint inNeedleLength,
													const gchar *inHaystack,
													gchar inSeperator)
{
	const gchar		*start;

	g_return_val_if_fail(inNeedle && *inNeedle!=0, FALSE);
	g_return_val_if_fail(inNeedleLength>0 || inNeedleLength==-1, FALSE);
	g_return_val_if_fail(inHaystack && *inHaystack!=0, FALSE);
	g_return_val_if_fail(inSeperator, FALSE);

	/* If given length of needle is negative it is a NULL-terminated string */
	if(inNeedleLength<0) inNeedleLength=strlen(inNeedle);

	/* Lookup needle in haystack */
	for(start=inHaystack; start; start=strchr(start, inSeperator))
	{
		gint		length;
		gchar		*nextEntry;

		/* Move to character after separator */
		if(start[0]==inSeperator) start++;

		/* Find end of this haystack entry */
		nextEntry=strchr(start, inSeperator);
		if(!nextEntry) length=strlen(start);
			else length=nextEntry-start;

		/* If enrty in haystack is not of same length as needle,
		 * then it is not a match
		 */
		if(length!=inNeedleLength) continue;

		if(!strncmp(inNeedle, start, inNeedleLength)) return(TRUE);
	}

	/* Needle was not found */
	return(FALSE);
}

/* IMPLEMENTATION: ClutterActor */

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
	xfdashboard_actor_remove_style_pseudo_class(self, "hover");

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
	xfdashboard_actor_add_style_pseudo_class(self, "hover");

	return(CLUTTER_EVENT_PROPAGATE);
}

/* Actor was (re)parented */
static void _xfdashboard_actor_parent_set(ClutterActor *inActor, ClutterActor *inOldParent)
{
	XfdashboardActor		*self;
	ClutterActorClass		*parentClass;

	g_return_if_fail(XFDASHBOARD_IS_ACTOR(inActor));

	self=XFDASHBOARD_ACTOR(inActor);

	/* Call parent's virtual function */
	parentClass=CLUTTER_ACTOR_CLASS(xfdashboard_actor_parent_class);
	if(parentClass->parent_set)
	{
		parentClass->parent_set(inActor, inOldParent);
	}

	/* Invalide styling to get it recomputed because its ID (from point
	 * of view of css) has changed
	 */
	xfdashboard_actor_style_invalidate(self);
}

/* IMPLEMENTATION: GObject */

/* Dispose this object */
static void _xfdashboard_actor_dispose(GObject *inObject)
{
	XfdashboardActor			*self=XFDASHBOARD_ACTOR(inObject);
	XfdashboardActorPrivate		*priv=self->priv;

	/* Release allocated variables */
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
		case PROP_STYLE_CLASSES:
			xfdashboard_actor_set_style_classes(self, g_value_get_string(inValue));
			break;

		case PROP_STYLE_PSEUDO_CLASSES:
			xfdashboard_actor_set_style_pseudo_classes(self, g_value_get_string(inValue));
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

	/* Override functions */
	gobjectClass->dispose=_xfdashboard_actor_dispose;
	gobjectClass->set_property=_xfdashboard_actor_set_property;
	gobjectClass->get_property=_xfdashboard_actor_get_property;

	clutterActorClass->parent_set=_xfdashboard_actor_parent_set;
	clutterActorClass->enter_event=_xfdashboard_actor_enter_event;
	clutterActorClass->leave_event=_xfdashboard_actor_leave_event;

	/* Set up private structure */
	g_type_class_add_private(klass, sizeof(XfdashboardActorPrivate));

	/* Define properties */
	XfdashboardActorProperties[PROP_STYLE_CLASSES]=
		g_param_spec_string("style-classes",
							_("Style classes"),
							_("String representing list of classes separated by '.'"),
							NULL,
							G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS);

	XfdashboardActorProperties[PROP_STYLE_PSEUDO_CLASSES]=
		g_param_spec_string("style-pseudo-classes",
							_("Style pseudo-classes"),
							_("String representing list of pseudo-classes, e.g. current state, separated by ':'"),
							NULL,
							G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS);

	g_object_class_install_properties(gobjectClass, PROP_LAST, XfdashboardActorProperties);

	/* Create param-spec pool for themable properties */
	g_assert(_xfdashboard_actor_stylable_properties_pool==NULL);
	_xfdashboard_actor_stylable_properties_pool=g_param_spec_pool_new(FALSE);

	/* Register GValue transformation functions not provided by Glib */
	g_value_register_transform_func(G_TYPE_STRING, G_TYPE_INT, _xfdashboard_actor_gvalue_transform_string_int);
	g_value_register_transform_func(G_TYPE_STRING, G_TYPE_UINT, _xfdashboard_actor_gvalue_transform_string_uint);
	g_value_register_transform_func(G_TYPE_STRING, G_TYPE_LONG, _xfdashboard_actor_gvalue_transform_string_long);
	g_value_register_transform_func(G_TYPE_STRING, G_TYPE_ULONG, _xfdashboard_actor_gvalue_transform_string_ulong);
	g_value_register_transform_func(G_TYPE_STRING, G_TYPE_INT64, _xfdashboard_actor_gvalue_transform_string_int64);
	g_value_register_transform_func(G_TYPE_STRING, G_TYPE_UINT64, _xfdashboard_actor_gvalue_transform_string_uint64);
	g_value_register_transform_func(G_TYPE_STRING, G_TYPE_FLOAT, _xfdashboard_actor_gvalue_transform_string_float);
	g_value_register_transform_func(G_TYPE_STRING, G_TYPE_DOUBLE, _xfdashboard_actor_gvalue_transform_string_double);
	g_value_register_transform_func(G_TYPE_STRING, G_TYPE_BOOLEAN, _xfdashboard_actor_gvalue_transform_string_boolean);
	g_value_register_transform_func(G_TYPE_STRING, G_TYPE_FLAGS, _xfdashboard_actor_gvalue_transform_string_flags);
	g_value_register_transform_func(G_TYPE_STRING, G_TYPE_ENUM, _xfdashboard_actor_gvalue_transform_string_enum);
}

/* Object initialization
 * Create private structure and set up default values
 */
void xfdashboard_actor_init(XfdashboardActor *self)
{
	XfdashboardActorPrivate		*priv;

	priv=self->priv=XFDASHBOARD_ACTOR_GET_PRIVATE(self);

	/* Set up default values */
	priv->styleClasses=NULL;
	priv->stylePseudoClasses=NULL;
	priv->lastThemeStyleSet=NULL;

	/* Connect signals */
	g_signal_connect(self, "notify::mapped", G_CALLBACK(_xfdashboard_actor_on_mapped_changed), NULL);
	g_signal_connect(self, "notify::name", G_CALLBACK(_xfdashboard_actor_on_name_changed), NULL);
}

/* Implementation: GType */

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

			g_debug("Unregistered stylable property named '%s' for class '%s'",
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
		const GTypeInfo actorInfo=
		{
			sizeof(XfdashboardActorClass),
			NULL,
			(GBaseFinalizeFunc)xfdashboard_actor_base_class_finalize,
			(GClassInitFunc)xfdashboard_actor_class_init,
			NULL, /* class_finalize */
			NULL, /* class_data */
			sizeof(XfdashboardActor),
			0,    /* n_preallocs */
			(GInstanceInitFunc)xfdashboard_actor_init,
			NULL, /* value_table */
		};

		actorType=g_type_register_static(CLUTTER_TYPE_ACTOR,
											g_intern_static_string("XfdashboardActor"),
											&actorInfo,
											0);
	}

	return(actorType);
}

/* Implementation: Public API */

/* Create new actor */
ClutterActor* xfdashboard_actor_new(void)
{
	return(CLUTTER_ACTOR(g_object_new(XFDASHBOARD_TYPE_ACTOR, NULL)));
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
	g_debug("Registered stylable property '%s' for class '%s'",
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

/* Get/set style classes of actor */
const gchar* xfdashboard_actor_get_style_classes(XfdashboardActor *self)
{
	g_return_val_if_fail(XFDASHBOARD_IS_ACTOR(self), NULL);

	return(self->priv->styleClasses);
}

void xfdashboard_actor_set_style_classes(XfdashboardActor *self, const gchar *inStyleClasses)
{
	XfdashboardActorPrivate		*priv;

	g_return_if_fail(XFDASHBOARD_IS_ACTOR(self));

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

		/* Invalidate style to get it restyled and redrawn */
		xfdashboard_actor_style_invalidate(self);

		/* Notify about property change */
		g_object_notify_by_pspec(G_OBJECT(self), XfdashboardActorProperties[PROP_STYLE_CLASSES]);
	}
}

void xfdashboard_actor_add_style_class(XfdashboardActor *self, const gchar *inStyleClass)
{
	XfdashboardActorPrivate		*priv;

	g_return_if_fail(XFDASHBOARD_IS_ACTOR(self));
	g_return_if_fail(inStyleClass && inStyleClass[0]);

	priv=self->priv;

	/* If class is already in list of classes do nothing otherwise set new value */
	if(!priv->styleClasses ||
		!_xfdashboard_actor_list_contains(inStyleClass, -1, priv->styleClasses, '.'))
	{
		gchar					*newClasses;

		/* Create new temporary string by concatenating current classes
		 * and new class with dot separator. Set this new string representing
		 * list of classes.
		 */
		if(priv->styleClasses) newClasses=g_strconcat(priv->styleClasses, ".", inStyleClass, NULL);
			else newClasses=g_strdup(inStyleClass);

		xfdashboard_actor_set_style_classes(self, newClasses);

		g_free(newClasses);
	}
}

void xfdashboard_actor_remove_style_class(XfdashboardActor *self, const gchar *inStyleClass)
{
	XfdashboardActorPrivate		*priv;

	g_return_if_fail(XFDASHBOARD_IS_ACTOR(self));
	g_return_if_fail(inStyleClass && inStyleClass[0]);

	priv=self->priv;

	/* If class is not in list of classes do nothing otherwise set new value */
	if(priv->styleClasses &&
		_xfdashboard_actor_list_contains(inStyleClass, -1, priv->styleClasses, '.'))
	{
		gchar					**oldClasses, **entry;
		gchar					*newClasses, *newClassesTemp;

		/* Create new temporary string with all current classes separated by dot
		 * not matching class to remove. Set this new string representing list
		 * of classes.
		 */
		entry=oldClasses=g_strsplit(priv->styleClasses, ".", -1);
		newClasses=NULL;
		while(*entry)
		{
			if(!strcmp(*entry, inStyleClass))
			{
				entry++;
				continue;
			}

			if(newClasses)
			{
				newClassesTemp=g_strconcat(newClasses, ".", *entry, NULL);
				g_free(newClasses);
				newClasses=newClassesTemp;
			}
				else newClasses=g_strdup(*entry);

			entry++;
		}

		xfdashboard_actor_set_style_classes(self, newClasses);

		g_strfreev(oldClasses);
		g_free(newClasses);
	}
}

/* Get/set style pseudo-classes of actor */
const gchar* xfdashboard_actor_get_style_pseudo_classes(XfdashboardActor *self)
{
	g_return_val_if_fail(XFDASHBOARD_IS_ACTOR(self), NULL);

	return(self->priv->stylePseudoClasses);
}

void xfdashboard_actor_set_style_pseudo_classes(XfdashboardActor *self, const gchar *inStylePseudoClasses)
{
	XfdashboardActorPrivate		*priv;

	g_return_if_fail(XFDASHBOARD_IS_ACTOR(self));

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

		/* Invalidate style to get it restyled and redrawn */
		xfdashboard_actor_style_invalidate(self);

		/* Notify about property change */
		g_object_notify_by_pspec(G_OBJECT(self), XfdashboardActorProperties[PROP_STYLE_PSEUDO_CLASSES]);
	}
}

void xfdashboard_actor_add_style_pseudo_class(XfdashboardActor *self, const gchar *inStylePseudoClass)
{
	XfdashboardActorPrivate		*priv;

	g_return_if_fail(XFDASHBOARD_IS_ACTOR(self));
	g_return_if_fail(inStylePseudoClass && inStylePseudoClass[0]);

	priv=self->priv;

	/* If class is already in list of classes do nothing otherwise set new value */
	if(!priv->stylePseudoClasses ||
		!_xfdashboard_actor_list_contains(inStylePseudoClass, -1, priv->stylePseudoClasses, ':'))
	{
		gchar					*newClasses;

		/* Create new temporary string by concatenating current classes
		 * and new class with dot separator. Set this new string representing
		 * list of classes.
		 */
		if(priv->stylePseudoClasses) newClasses=g_strconcat(priv->stylePseudoClasses, ":", inStylePseudoClass, NULL);
			else newClasses=g_strdup(inStylePseudoClass);

		xfdashboard_actor_set_style_pseudo_classes(self, newClasses);

		g_free(newClasses);
	}
}

void xfdashboard_actor_remove_style_pseudo_class(XfdashboardActor *self, const gchar *inStylePseudoClass)
{
	XfdashboardActorPrivate		*priv;

	g_return_if_fail(XFDASHBOARD_IS_ACTOR(self));
	g_return_if_fail(inStylePseudoClass && inStylePseudoClass[0]);

	priv=self->priv;

	/* If class is not in list of classes do nothing otherwise set new value */
	if(priv->stylePseudoClasses &&
		_xfdashboard_actor_list_contains(inStylePseudoClass, -1, priv->stylePseudoClasses, ':'))
	{
		gchar					**oldClasses, **entry;
		gchar					*newClasses, *newClassesTemp;

		/* Create new temporary string with all current classes separated by dot
		 * not matching class to remove. Set this new string representing list
		 * of classes.
		 */
		entry=oldClasses=g_strsplit(priv->stylePseudoClasses, ":", -1);
		newClasses=NULL;
		while(*entry)
		{
			if(!strcmp(*entry, inStylePseudoClass))
			{
				entry++;
				continue;
			}

			if(newClasses)
			{
				newClassesTemp=g_strconcat(newClasses, ":", *entry, NULL);
				g_free(newClasses);
				newClasses=newClassesTemp;
			}
				else newClasses=g_strdup(*entry);

			entry++;
		}

		xfdashboard_actor_set_style_pseudo_classes(self, newClasses);

		g_strfreev(oldClasses);
		g_free(newClasses);
	}
}

/* Invalidate style to recompute styles */
void xfdashboard_actor_style_invalidate(XfdashboardActor *self)
{
	XfdashboardActorPrivate		*priv;
	XfdashboardActorClass		*klass;
	XfdashboardThemeCSS			*theme;
	GHashTable					*possibleStyleSet;
	GParamSpec					*paramSpec;
	GHashTableIter				hashIter;
	GHashTable					*themeStyleSet;
	gchar						*styleName;
	XfdashboardThemeCSSValue	*styleValue;
	ClutterActorIter			actorIter;
	ClutterActor				*child;
#ifdef DEBUG
	gboolean					doDebug=FALSE;
#endif

	g_return_if_fail(XFDASHBOARD_IS_ACTOR(self));

	priv=self->priv;
	klass=XFDASHBOARD_ACTOR_GET_CLASS(self);
	theme=xfdashboard_application_get_theme();

	/* Only recompute style for mapped actors */
	if(!CLUTTER_ACTOR_IS_MAPPED(self)) return;

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

		g_debug("%s: Got param specs for %p (%s) with class '%s' and pseudo-class '%s'", __func__, self, G_OBJECT_TYPE_NAME(self), priv->styleClasses, priv->stylePseudoClasses);

		g_hash_table_iter_init(&hashIter, possibleStyleSet);
		while(g_hash_table_iter_next(&hashIter, (gpointer*)&defaultsKey, (gpointer*)&paramSpec))
		{
			realParamSpec=(GParamSpec*)g_param_spec_get_qdata(paramSpec, XFDASHBOARD_ACTOR_PARAM_SPEC_REF);

			g_value_init(&defaultsVal, G_PARAM_SPEC_VALUE_TYPE(realParamSpec));
			g_param_value_set_default(realParamSpec, &defaultsVal);

			defaultsValStr=g_strdup_value_contents(&defaultsVal);
			g_debug("%d: param spec [%s] %s=%s\n", ++i, G_OBJECT_CLASS_NAME(klass), defaultsKey, defaultsValStr);
			g_free(defaultsValStr);

			g_value_unset(&defaultsVal);
		}

		g_debug("%s: End of param specs", __func__);
	}
#endif

	/* Get style information from theme */
	themeStyleSet=xfdashboard_theme_css_get_properties(theme, self);

#ifdef DEBUG
	if(doDebug)
	{
		gint					i=0;

		g_debug("%s: Got styles from theme for %p (%s) with class '%s' and pseudo-class '%s'", __func__, self, G_OBJECT_TYPE_NAME(self), priv->styleClasses, priv->stylePseudoClasses);

		g_hash_table_iter_init(&hashIter, themeStyleSet);
		while(g_hash_table_iter_next(&hashIter, (gpointer*)&styleName, (gpointer*)&styleValue))
		{
			g_debug("%d: [%s] %s=%s\n", ++i, styleValue->source, (gchar*)styleName, styleValue->string);
		}

		g_debug("%s: End of styles from theme", __func__);
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
#ifdef DEBUG
			if(doDebug)
			{
				gchar					*valstr;

				valstr=g_strdup_value_contents(&propertyValue);
				g_debug("Setting theme value of style property [%s] %s=%s\n", G_OBJECT_CLASS_NAME(klass), styleName, valstr);
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
#ifdef DEBUG
			if(doDebug)
			{
				gchar					*valstr;

				valstr=g_strdup_value_contents(&propertyValue);
				g_debug("Restoring default value of style property [%s] %s=%s\n", G_OBJECT_CLASS_NAME(klass), styleName, valstr);
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

	/* All stylable properties are set now. So thaw 'property-changed'
	 * notification now and fire all notifications at once.
	 */
	g_object_thaw_notify(G_OBJECT(self));

	/* Recompute styles for all children recursively */
	clutter_actor_iter_init(&actorIter, CLUTTER_ACTOR(self));
	while(clutter_actor_iter_next(&actorIter, &child))
	{
		if(!XFDASHBOARD_IS_ACTOR(child)) continue;

		xfdashboard_actor_style_invalidate(XFDASHBOARD_ACTOR(child));
	}
}
