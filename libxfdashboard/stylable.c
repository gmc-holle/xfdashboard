/*
 * stylable: An interface which can be inherited by actor and objects
 *           to get styled by a theme
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

#include <libxfdashboard/stylable.h>

#include <glib/gi18n-lib.h>

#include <libxfdashboard/application.h>
#include <libxfdashboard/compat.h>


/* Define this interface in GObject system */
G_DEFINE_INTERFACE(XfdashboardStylable,
					xfdashboard_stylable,
					G_TYPE_OBJECT)

/* Signals */
enum
{
	/* Signals */
	SIGNAL_STYLE_REVALIDATED,

	SIGNAL_LAST
};

static guint XfdashboardStylableSignals[SIGNAL_LAST]={ 0, };

/* IMPLEMENTATION: Private variables and methods */
#define XFDASHBOARD_STYLABLE_WARN_NOT_IMPLEMENTED(self, vfunc) \
	g_warning(_("Object of type %s does not implement required virtual function XfdashboardStylable::%s"), \
				G_OBJECT_TYPE_NAME(self), \
				vfunc);

/* Check if haystack contains needle.
 * The haystack is a string representing a list which entries is seperated
 * by a seperator character. This function looks up the haystack if it
 * contains an entry matching the needle and returns TRUE in this case.
 * Otherwise FALSE is returned. A needle length of -1 signals that needle
 * is a NULL-terminated string and length should be determine automatically.
 */
static gboolean _xfdashboard_stylable_list_contains(const gchar *inNeedle,
													gint inNeedleLength,
													const gchar *inHaystack,
													gchar inSeperator)
{
	const gchar					*start;

	g_return_val_if_fail(inNeedle && *inNeedle!=0, FALSE);
	g_return_val_if_fail(inNeedleLength>0 || inNeedleLength==-1, FALSE);
	g_return_val_if_fail(inHaystack && *inHaystack!=0, FALSE);
	g_return_val_if_fail(inSeperator, FALSE);

	/* If given length of needle is negative it is a NULL-terminated string */
	if(inNeedleLength<0) inNeedleLength=strlen(inNeedle);

	/* Lookup needle in haystack */
	for(start=inHaystack; start; start=strchr(start, inSeperator))
	{
		gint					length;
		gchar					*nextEntry;

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

/* Default implementation of virtual function "get_name" */
static const gchar* _xfdashboard_stylable_real_get_name(XfdashboardStylable *self)
{
	const gchar			*name;

	g_return_val_if_fail(XFDASHBOARD_IS_STYLABLE(self), NULL);

	name=NULL;

	/* If object implementing this interface is derived from ClutterActor
	 * get actor's name.
	 */
	if(CLUTTER_IS_ACTOR(self)) name=clutter_actor_get_name(CLUTTER_ACTOR(self));

	/* Return determined name for stylable object */
	return(name);
}

/* Default implementation of virtual function "get_parent" */
static XfdashboardStylable* _xfdashboard_stylable_real_get_parent(XfdashboardStylable *self)
{
	XfdashboardStylable		*parent;

	g_return_val_if_fail(XFDASHBOARD_IS_STYLABLE(self), NULL);

	parent=NULL;

	/* If object implementing this interface is derived from ClutterActor
	 * get actor's parent actor.
	 */
	if(CLUTTER_IS_ACTOR(self))
	{
		ClutterActor		*parentActor;

		/* Get parent and if parent stylable set parent as result */
		parentActor=clutter_actor_get_parent(CLUTTER_ACTOR(self));
		if(parentActor &&
			XFDASHBOARD_IS_STYLABLE(parentActor))
		{
			parent=XFDASHBOARD_STYLABLE(parentActor);
		}
	}

	/* Return stylable parent */
	return(parent);
}

/* Default implementation of virtual function "invalidate" */
static void _xfdashboard_stylable_real_invalidate(XfdashboardStylable *self)
{
	XfdashboardTheme			*theme;
	XfdashboardThemeCSS			*themeCSS;
	GHashTable					*stylableProperties;
	GHashTable					*themeStyleSet;
	GHashTableIter				hashIter;
	gchar						*propertyName;
	GParamSpec					*propertyValueParamSpec;
	XfdashboardThemeCSSValue	*styleValue;

	g_return_if_fail(XFDASHBOARD_IS_STYLABLE(self));

	/* Get hashtable with all stylable properties and their parameter
	 * specification for default values.
	 */
	stylableProperties=xfdashboard_stylable_get_stylable_properties(self);
	if(!stylableProperties) return;

	/* Get theme CSS */
	theme=xfdashboard_application_get_theme(NULL);
	themeCSS=xfdashboard_theme_get_css(theme);

	/* Get styled properties from theme CSS */
	themeStyleSet=xfdashboard_theme_css_get_properties(themeCSS, self);

	/* The 'property-changed' notification will be freezed and thawed
	 * (fired at once) after all stylable properties of this instance are set.
	 */
	g_object_freeze_notify(G_OBJECT(self));

	/* Iterate through stylable properties and check if we got a style of
	 * that name from theme CSS. If we find such a style set the corresponding
	 * property in object otherwise set default value to override any
	 * previous value set by theme CSS to reset it.
	 */
	g_hash_table_iter_init(&hashIter, stylableProperties);
	while(g_hash_table_iter_next(&hashIter, (gpointer*)&propertyName, (gpointer*)&propertyValueParamSpec))
	{
		/* Check if we got a style with this name from theme CSS and
		 * set style's value if found ...
		 */
		if(g_hash_table_lookup_extended(themeStyleSet, propertyName, NULL, (gpointer*)&styleValue))
		{
			GValue				cssValue=G_VALUE_INIT;
			GValue				propertyValue=G_VALUE_INIT;

			/* Convert style value to type of object property and set value
			 * if conversion was successful. Otherwise do nothing.
			 */
			g_value_init(&cssValue, G_TYPE_STRING);
			g_value_set_string(&cssValue, styleValue->string);

			g_value_init(&propertyValue, G_PARAM_SPEC_VALUE_TYPE(propertyValueParamSpec));

			if(g_param_value_convert(propertyValueParamSpec, &cssValue, &propertyValue, FALSE))
			{
				g_object_set_property(G_OBJECT(self), propertyName, &propertyValue);
			}
				else
				{
					g_warning(_("Could not transform CSS string value for property '%s' to type %s of class %s"),
								propertyName,
								g_type_name(G_PARAM_SPEC_VALUE_TYPE(propertyValueParamSpec)),
								G_OBJECT_TYPE_NAME(self));
				}

			/* Release allocated resources */
			g_value_unset(&propertyValue);
			g_value_unset(&cssValue);
		}
			/* ... otherwise set property's default value we got from
			 * stylable interface of object.
			 */
			else
			{
				GValue			propertyValue=G_VALUE_INIT;

				/* Initialize property value to its type and default value */
				g_value_init(&propertyValue, G_PARAM_SPEC_VALUE_TYPE(propertyValueParamSpec));
				g_param_value_set_default(propertyValueParamSpec, &propertyValue);

				/* Set value at object property */
				g_object_set_property(G_OBJECT(self), propertyName, &propertyValue);

				/* Release allocated resources */
				g_value_unset(&propertyValue);
			}
	}

	/* All stylable properties are set now. So thaw 'property-changed'
	 * notification now and fire all notifications at once.
	 */
	g_object_thaw_notify(G_OBJECT(self));

	/* Release allocated resources */
	g_hash_table_destroy(themeStyleSet);
	g_hash_table_destroy(stylableProperties);

	/* Emit 'style-revalidated' signal to notify other objects about it's done */
	g_signal_emit(self, XfdashboardStylableSignals[SIGNAL_STYLE_REVALIDATED], 0);
}

/* IMPLEMENTATION: GObject */

/* Interface initialization
 * Set up default functions
 */
void xfdashboard_stylable_default_init(XfdashboardStylableInterface *iface)
{
	static gboolean		initialized=FALSE;
	GParamSpec			*property;

	/* All the following virtual functions must be overridden */
	iface->get_stylable_properties=NULL;
	iface->get_classes=NULL;
	iface->set_classes=NULL;
	iface->get_pseudo_classes=NULL;
	iface->set_pseudo_classes=NULL;

	/* The following virtual functions should be overriden if default
	 * implementation does not fit.
	 */
	iface->get_name=_xfdashboard_stylable_real_get_name;
	iface->get_parent=_xfdashboard_stylable_real_get_parent;
	iface->invalidate=_xfdashboard_stylable_real_invalidate;

	/* Define properties, signals and actions */
	if(!initialized)
	{
		/* Define properties */
		property=g_param_spec_string("style-classes",
										_("Style classes"),
										_("String representing list of classes separated by '.'"),
										NULL,
										G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS);
		g_object_interface_install_property(iface, property);

		property=g_param_spec_string("style-pseudo-classes",
										_("Style pseudo-classes"),
										_("String representing list of pseudo-classes, e.g. current state, separated by ':'"),
										NULL,
										G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS);
		g_object_interface_install_property(iface, property);

		/* Define signals */
		/**
		 * XfdashboardStylable::style-revalidated:
		 * @self: A #XfdashboardStylable
		 *
		 * The ::style-revalidated signal is emitted when the style information
		 * for @self were invalidated and the new style information were calculated
		 * and applied.
		 *
		 * Connecting to this signal is mostly useful for stylable non-actor object,
		 * i.e. objects which are no sub-class of #XfdashboardActor (neither directly
		 * not indirectly) but inherit the #XfdashboardStylable interface, to invalidate
		 * their style information and to get them recalculated and applied.
		 */
		XfdashboardStylableSignals[SIGNAL_STYLE_REVALIDATED]=
			g_signal_new("style-revalidated",
							G_TYPE_FROM_INTERFACE(iface),
							G_SIGNAL_RUN_LAST,
							0,
							NULL,
							NULL,
							g_cclosure_marshal_VOID__VOID,
							G_TYPE_NONE,
							0);

		/* Set flag that base initialization was done for this interface */
		initialized=TRUE;
	}
}

/* IMPLEMENTATION: Public API */

/* Call virtual function "get_stylable_properties" */
GHashTable* xfdashboard_stylable_get_stylable_properties(XfdashboardStylable *self)
{
	XfdashboardStylableInterface		*iface;
	GHashTable							*stylableProperties;

	g_return_val_if_fail(XFDASHBOARD_IS_STYLABLE(self), NULL);

	iface=XFDASHBOARD_STYLABLE_GET_IFACE(self);

	/* Call virtual function */
	if(iface->get_stylable_properties)
	{
		/* Create hashtable to insert stylable properties at */
		stylableProperties=g_hash_table_new_full(g_str_hash,
													g_str_equal,
													g_free,
													(GDestroyNotify)g_param_spec_unref);

		/* Get stylable properties */
		iface->get_stylable_properties(self, stylableProperties);

		/* If hashtable is empty, destroy it and return NULL */
		if(g_hash_table_size(stylableProperties)==0)
		{
			g_hash_table_destroy(stylableProperties);
			stylableProperties=NULL;
		}

		/* Return hashtable with stylable properties */
		return(stylableProperties);
	}

	/* If we get here the virtual function was not overridden */
	XFDASHBOARD_STYLABLE_WARN_NOT_IMPLEMENTED(self, "get_stylable_properties");
	return(NULL);
}

/* Find and add parameter specification of a stylable property to hashtable */
gboolean xfdashboard_stylable_add_stylable_property(XfdashboardStylable *self,
													GHashTable *ioStylableProperties,
													const gchar *inProperty)
{
	GParamSpec							*spec;

	g_return_val_if_fail(XFDASHBOARD_IS_STYLABLE(self), FALSE);

	/* Find property in instance */
	spec=g_object_class_find_property(G_OBJECT_GET_CLASS(self), inProperty);
	if(!spec)
	{
		g_warning(_("Could not find property '%s' for class %s"),
					inProperty,
					G_OBJECT_TYPE_NAME(self));
		return(FALSE);
	}

	/* Add parameter specification of found property to hashtable */
	g_hash_table_insert(ioStylableProperties, g_strdup(inProperty), g_param_spec_ref(spec));

	return(TRUE);
}

/* Call virtual function "get_name" */
const gchar* xfdashboard_stylable_get_name(XfdashboardStylable *self)
{
	XfdashboardStylableInterface		*iface;

	g_return_val_if_fail(XFDASHBOARD_IS_STYLABLE(self), NULL);

	iface=XFDASHBOARD_STYLABLE_GET_IFACE(self);

	/* Call virtual function */
	if(iface->get_name)
	{
		return(iface->get_name(self));
	}

	/* If we get here the virtual function was not overridden */
	XFDASHBOARD_STYLABLE_WARN_NOT_IMPLEMENTED(self, "get_name");
	return(NULL);
}

/* Call virtual function "get_parent" */
XfdashboardStylable* xfdashboard_stylable_get_parent(XfdashboardStylable *self)
{
	XfdashboardStylableInterface		*iface;

	g_return_val_if_fail(XFDASHBOARD_IS_STYLABLE(self), NULL);

	iface=XFDASHBOARD_STYLABLE_GET_IFACE(self);

	/* Call virtual function */
	if(iface->get_parent)
	{
		return(XFDASHBOARD_STYLABLE(iface->get_parent(self)));
	}

	/* If we get here the virtual function was not overridden */
	XFDASHBOARD_STYLABLE_WARN_NOT_IMPLEMENTED(self, "get_parent");
	return(NULL);
}

/* Call virtual function "get_classes" */
const gchar* xfdashboard_stylable_get_classes(XfdashboardStylable *self)
{
	XfdashboardStylableInterface		*iface;

	g_return_val_if_fail(XFDASHBOARD_IS_STYLABLE(self), NULL);

	iface=XFDASHBOARD_STYLABLE_GET_IFACE(self);

	/* Call virtual function */
	if(iface->get_classes)
	{
		return(iface->get_classes(self));
	}

	/* If we get here the virtual function was not overridden */
	XFDASHBOARD_STYLABLE_WARN_NOT_IMPLEMENTED(self, "get_classes");
	return(NULL);
}

/* Call virtual function "set_classes" */
void xfdashboard_stylable_set_classes(XfdashboardStylable *self, const gchar *inClasses)
{
	XfdashboardStylableInterface		*iface;

	g_return_if_fail(XFDASHBOARD_IS_STYLABLE(self));

	iface=XFDASHBOARD_STYLABLE_GET_IFACE(self);

	/* Call virtual function */
	if(iface->set_classes)
	{
		iface->set_classes(self, inClasses);
		return;
	}

	/* If we get here the virtual function was not overridden */
	XFDASHBOARD_STYLABLE_WARN_NOT_IMPLEMENTED(self, "set_classes");
}

/* Determine if a specific class is being set at object */
gboolean xfdashboard_stylable_has_class(XfdashboardStylable *self, const gchar *inClass)
{
	const gchar		*classes;
	gboolean		result;

	g_return_val_if_fail(XFDASHBOARD_IS_STYLABLE(self), FALSE);
	g_return_val_if_fail(inClass && inClass[0], FALSE);

	result=FALSE;

	/* Get classes set at object and check if it has the expected one */
	classes=xfdashboard_stylable_get_classes(self);
	if(classes &&
		_xfdashboard_stylable_list_contains(inClass, -1, classes, '.'))
	{
		result=TRUE;
	}

	/* Return result */
	return(result);
}

/* Adds a class to existing classes of an object */
void xfdashboard_stylable_add_class(XfdashboardStylable *self, const gchar *inClass)
{
	const gchar		*classes;

	g_return_if_fail(XFDASHBOARD_IS_STYLABLE(self));
	g_return_if_fail(inClass && inClass[0]);

	/* If class is already in list of classes do nothing otherwise set new value */
	classes=xfdashboard_stylable_get_classes(self);
	if(!classes ||
		!_xfdashboard_stylable_list_contains(inClass, -1, classes, '.'))
	{
		gchar					*newClasses;

		/* Create new temporary string by concatenating current classes
		 * and new class with dot separator. Set this new string representing
		 * list of classes.
		 */
		if(classes) newClasses=g_strconcat(classes, ".", inClass, NULL);
			else newClasses=g_strdup(inClass);

		xfdashboard_stylable_set_classes(self, newClasses);

		g_free(newClasses);
	}
}

/* Removed a class to existing classes of an object */
void xfdashboard_stylable_remove_class(XfdashboardStylable *self, const gchar *inClass)
{
	const gchar		*classes;

	g_return_if_fail(XFDASHBOARD_IS_STYLABLE(self));
	g_return_if_fail(inClass && inClass[0]);

	/* If class is not in list of classes do nothing otherwise set new value */
	classes=xfdashboard_stylable_get_classes(self);
	if(classes &&
		_xfdashboard_stylable_list_contains(inClass, -1, classes, '.'))
	{
		gchar					**oldClasses, **entry;
		gchar					*newClasses, *newClassesTemp;

		/* Create new temporary string with all current classes separated by dot
		 * not matching class to remove. Set this new string representing list
		 * of classes.
		 */
		entry=oldClasses=g_strsplit(classes, ".", -1);
		newClasses=NULL;
		while(*entry)
		{
			if(!strcmp(*entry, inClass))
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

		xfdashboard_stylable_set_classes(self, newClasses);

		g_strfreev(oldClasses);
		g_free(newClasses);
	}
}

/* Call virtual function "get_pseudo_classes" */
const gchar* xfdashboard_stylable_get_pseudo_classes(XfdashboardStylable *self)
{
	XfdashboardStylableInterface		*iface;

	g_return_val_if_fail(XFDASHBOARD_IS_STYLABLE(self), NULL);

	iface=XFDASHBOARD_STYLABLE_GET_IFACE(self);

	/* Call virtual function */
	if(iface->get_pseudo_classes)
	{
		return(iface->get_pseudo_classes(self));
	}

	/* If we get here the virtual function was not overridden */
	XFDASHBOARD_STYLABLE_WARN_NOT_IMPLEMENTED(self, "get_pseudo_classes");
	return(NULL);
}

/* Call virtual function "set_pseudo_classes" */
void xfdashboard_stylable_set_pseudo_classes(XfdashboardStylable *self, const gchar *inClasses)
{
	XfdashboardStylableInterface		*iface;

	g_return_if_fail(XFDASHBOARD_IS_STYLABLE(self));

	iface=XFDASHBOARD_STYLABLE_GET_IFACE(self);

	/* Call virtual function */
	if(iface->set_pseudo_classes)
	{
		iface->set_pseudo_classes(self, inClasses);
		return;
	}

	/* If we get here the virtual function was not overridden */
	XFDASHBOARD_STYLABLE_WARN_NOT_IMPLEMENTED(self, "set_pseudo_classes");
}

/* Determine if a specific pseudo-class is being set at object */
gboolean xfdashboard_stylable_has_pseudo_class(XfdashboardStylable *self, const gchar *inClass)
{
	const gchar		*classes;
	gboolean		result;

	g_return_val_if_fail(XFDASHBOARD_IS_STYLABLE(self), FALSE);
	g_return_val_if_fail(inClass && inClass[0], FALSE);

	result=FALSE;

	/* Get classes set at object and check if it has the expected one */
	classes=xfdashboard_stylable_get_pseudo_classes(self);
	if(classes &&
		_xfdashboard_stylable_list_contains(inClass, -1, classes, ':'))
	{
		result=TRUE;
	}

	/* Return result */
	return(result);
}


/* Adds a pseudo-class to existing pseudo-classes of an object */
void xfdashboard_stylable_add_pseudo_class(XfdashboardStylable *self, const gchar *inClass)
{
	const gchar		*classes;

	g_return_if_fail(XFDASHBOARD_IS_STYLABLE(self));
	g_return_if_fail(inClass && inClass[0]);

	/* If pesudo-class is already in list of pseudo-classes do nothing
	 * otherwise set new value.
	 */
	classes=xfdashboard_stylable_get_pseudo_classes(self);
	if(!classes ||
		!_xfdashboard_stylable_list_contains(inClass, -1, classes, ':'))
	{
		gchar					*newClasses;

		/* Create new temporary string by concatenating current pseudo-classes
		 * and new psuedo-class with colon separator. Set this new string
		 * representing list of pseudo-classes.
		 */
		if(classes) newClasses=g_strconcat(classes, ":", inClass, NULL);
			else newClasses=g_strdup(inClass);

		xfdashboard_stylable_set_pseudo_classes(self, newClasses);

		g_free(newClasses);
	}
}

/* Removed a pseudo-class to existing classes of an object */
void xfdashboard_stylable_remove_pseudo_class(XfdashboardStylable *self, const gchar *inClass)
{
	const gchar		*classes;

	g_return_if_fail(XFDASHBOARD_IS_STYLABLE(self));
	g_return_if_fail(inClass && inClass[0]);

	/* If pseudo-class is not in list of pseudo-classes do nothing
	 * otherwise set new value.
	 */
	classes=xfdashboard_stylable_get_pseudo_classes(self);
	if(classes &&
		_xfdashboard_stylable_list_contains(inClass, -1, classes, ':'))
	{
		gchar					**oldClasses, **entry;
		gchar					*newClasses, *newClassesTemp;

		/* Create new temporary string with all current pseudo-classes
		 * separated by colon not matching pseudo-class to remove.
		 * Set this new string representing list of pseudo-classes.
		 */
		entry=oldClasses=g_strsplit(classes, ":", -1);
		newClasses=NULL;
		while(*entry)
		{
			if(!strcmp(*entry, inClass))
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

		xfdashboard_stylable_set_pseudo_classes(self, newClasses);

		g_strfreev(oldClasses);
		g_free(newClasses);
	}
}

/* Call virtual function "invalidate" */
void xfdashboard_stylable_invalidate(XfdashboardStylable *self)
{
	XfdashboardStylableInterface		*iface;

	g_return_if_fail(XFDASHBOARD_IS_STYLABLE(self));

	iface=XFDASHBOARD_STYLABLE_GET_IFACE(self);

	/* Call virtual function */
	if(iface->invalidate)
	{
		iface->invalidate(self);
		return;
	}

	/* If we get here the virtual function was not overridden */
	XFDASHBOARD_STYLABLE_WARN_NOT_IMPLEMENTED(self, "invalidate");
}
