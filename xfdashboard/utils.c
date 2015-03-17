/*
 * utils: Common functions, helpers and definitions
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

#include "utils.h"

#include <glib/gi18n-lib.h>
#include <clutter/clutter.h>
#include <gtk/gtk.h>
#include <dbus/dbus-glib.h>
#include <gio/gdesktopappinfo.h>

#include "stage.h"
#include "window-tracker.h"

/* Gobject type for pointer arrays (GPtrArray) */
GType xfdashboard_pointer_array_get_type(void)
{
	static volatile gsize	type__volatile=0;
	GType					type;

	if(g_once_init_enter(&type__volatile))
	{
		type=dbus_g_type_get_collection("GPtrArray", G_TYPE_VALUE);
		g_once_init_leave(&type__volatile, type);
	}

	return(type__volatile);
}

/* Show a notification */
void xfdashboard_notify(ClutterActor *inSender, const gchar *inIconName, const gchar *inFormatText, ...)
{
	XfdashboardStage				*stage;
	ClutterStageManager				*stageManager;
	const GSList					*stages;
	va_list							args;
	gchar							*text;

	g_return_if_fail(inSender==NULL || CLUTTER_IS_ACTOR(inSender));

	stage=NULL;

	/* Build text to display */
	va_start(args, inFormatText);
	text=g_strdup_vprintf(inFormatText, args);
	va_end(args);

	/* Get stage of sending actor if available */
	if(inSender) stage=XFDASHBOARD_STAGE(clutter_actor_get_stage(inSender));

	/* No sending actor specified or no stage found so get default stage */
	if(!stage)
	{
		stageManager=clutter_stage_manager_get_default();
		if(CLUTTER_IS_STAGE_MANAGER(stageManager))
		{
			stage=XFDASHBOARD_STAGE(clutter_stage_manager_get_default_stage(stageManager));
		}

		/* If we did not get default stage use first stage from manager */
		if(!stage && stageManager)
		{
			stages=clutter_stage_manager_peek_stages(stageManager);
			if(stages) stage=XFDASHBOARD_STAGE(stages->data);
		}

		/* If we still do not have found a stage to show notification
		 * stop further processing and show notification text as a critical
		 * warning in addition to the critical warning that we could not
		 * find any stage.
		 */
		if(!stage)
		{
			g_critical(_("Could find any stage to show notification: %s"), text);
		}
	}

	/* Show notification on stage (if any found) */
	if(stage) xfdashboard_stage_show_notification(stage, inIconName, text);

	/* Release allocated resources */
	g_free(text);
}

/* Create a application context for launching application by GIO.
 * Object returned must be freed with g_object_unref().
 */
GAppLaunchContext* xfdashboard_create_app_context(XfdashboardWindowTrackerWorkspace *inWorkspace)
{
	GdkAppLaunchContext			*context;
	const ClutterEvent			*event;
	XfdashboardWindowTracker	*tracker;

	g_return_val_if_fail(inWorkspace==NULL || XFDASHBOARD_IS_WINDOW_TRACKER_WORKSPACE(inWorkspace), NULL);

	/* Get last event for timestamp */
	event=clutter_get_current_event();

	/* Get active workspace if not specified */
	if(!inWorkspace)
	{
		tracker=xfdashboard_window_tracker_get_default();
		inWorkspace=xfdashboard_window_tracker_get_active_workspace(tracker);
		g_object_unref(tracker);
	}

	/* Create and set up application context to use either the workspace specified
	 * or otherwise current active workspace. We will even set the current active
	 * explicitly to launch the application on current workspace even if user changes
	 * workspace in the time between launching application and showing first window.
	 */
	context=gdk_display_get_app_launch_context(gdk_display_get_default());
	if(event) gdk_app_launch_context_set_timestamp(context, clutter_event_get_time(event));
	gdk_app_launch_context_set_desktop(context, xfdashboard_window_tracker_workspace_get_number(inWorkspace));

	/* Return application context */
	return(G_APP_LAUNCH_CONTEXT(context));
}

/* GValue transformation function for G_TYPE_STRING to various other types */
static void _xfdashboard_gvalue_transform_string_int(const GValue *inSourceValue, GValue *ioDestValue)
{
	ioDestValue->data[0].v_int=g_ascii_strtoll(inSourceValue->data[0].v_pointer, NULL, 10);
}

static void _xfdashboard_gvalue_transform_string_uint(const GValue *inSourceValue, GValue *ioDestValue)
{
	ioDestValue->data[0].v_uint=g_ascii_strtoull(inSourceValue->data[0].v_pointer, NULL, 10);
}

static void _xfdashboard_gvalue_transform_string_long(const GValue *inSourceValue, GValue *ioDestValue)
{
	ioDestValue->data[0].v_long=g_ascii_strtoll(inSourceValue->data[0].v_pointer, NULL, 10);
}

static void _xfdashboard_gvalue_transform_string_ulong(const GValue *inSourceValue, GValue *ioDestValue)
{
	ioDestValue->data[0].v_ulong=g_ascii_strtoull(inSourceValue->data[0].v_pointer, NULL, 10);
}

static void _xfdashboard_gvalue_transform_string_int64(const GValue *inSourceValue, GValue *ioDestValue)
{
	ioDestValue->data[0].v_int64=g_ascii_strtoll(inSourceValue->data[0].v_pointer, NULL, 10);
}

static void _xfdashboard_gvalue_transform_string_uint64(const GValue *inSourceValue, GValue *ioDestValue)
{
	ioDestValue->data[0].v_uint64=g_ascii_strtoull(inSourceValue->data[0].v_pointer, NULL, 10);
}

static void _xfdashboard_gvalue_transform_string_float(const GValue *inSourceValue, GValue *ioDestValue)
{
	ioDestValue->data[0].v_float=g_ascii_strtod(inSourceValue->data[0].v_pointer, NULL);
}

static void _xfdashboard_gvalue_transform_string_double(const GValue *inSourceValue, GValue *ioDestValue)
{
	ioDestValue->data[0].v_double=g_ascii_strtod(inSourceValue->data[0].v_pointer, NULL);
}

static void _xfdashboard_gvalue_transform_string_boolean(const GValue *inSourceValue, GValue *ioDestValue)
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

static void _xfdashboard_gvalue_transform_string_enum(const GValue *inSourceValue, GValue *ioDestValue)
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

static void _xfdashboard_gvalue_transform_string_flags(const GValue *inSourceValue, GValue *ioDestValue)
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

void xfdashboard_register_gvalue_transformation_funcs(void)
{
	/* Register GValue transformation functions not provided by Glib */
	g_value_register_transform_func(G_TYPE_STRING, G_TYPE_INT, _xfdashboard_gvalue_transform_string_int);
	g_value_register_transform_func(G_TYPE_STRING, G_TYPE_UINT, _xfdashboard_gvalue_transform_string_uint);
	g_value_register_transform_func(G_TYPE_STRING, G_TYPE_LONG, _xfdashboard_gvalue_transform_string_long);
	g_value_register_transform_func(G_TYPE_STRING, G_TYPE_ULONG, _xfdashboard_gvalue_transform_string_ulong);
	g_value_register_transform_func(G_TYPE_STRING, G_TYPE_INT64, _xfdashboard_gvalue_transform_string_int64);
	g_value_register_transform_func(G_TYPE_STRING, G_TYPE_UINT64, _xfdashboard_gvalue_transform_string_uint64);
	g_value_register_transform_func(G_TYPE_STRING, G_TYPE_FLOAT, _xfdashboard_gvalue_transform_string_float);
	g_value_register_transform_func(G_TYPE_STRING, G_TYPE_DOUBLE, _xfdashboard_gvalue_transform_string_double);
	g_value_register_transform_func(G_TYPE_STRING, G_TYPE_BOOLEAN, _xfdashboard_gvalue_transform_string_boolean);
	g_value_register_transform_func(G_TYPE_STRING, G_TYPE_FLAGS, _xfdashboard_gvalue_transform_string_flags);
	g_value_register_transform_func(G_TYPE_STRING, G_TYPE_ENUM, _xfdashboard_gvalue_transform_string_enum);
}

/* Determine if child is a sibling of actor deeply */
gboolean xfdashboard_actor_contains_child_deep(ClutterActor *inActor, ClutterActor *inChild)
{
	ClutterActorIter	iter;
	ClutterActor		*child;

	g_return_val_if_fail(CLUTTER_IS_ACTOR(inActor), FALSE);
	g_return_val_if_fail(CLUTTER_IS_ACTOR(inActor), FALSE);

	/* For each child of actor call ourselve recursive */
	clutter_actor_iter_init(&iter, inActor);
	while(clutter_actor_iter_next(&iter, &child))
	{
		/* First check if current child of iterator is the one to lookup */
		if(child==inChild) return(TRUE);

		/* Then call ourselve with child as "top-parent" actor
		 * to lookup recursively.
		 */
		if(xfdashboard_actor_contains_child_deep(child, inChild))
		{
			return(TRUE);
		}
	}

	/* If we get here the child was not found deeply */
	return(FALSE);
}

/* Find child by name deeply beginning at given actor */
ClutterActor* xfdashboard_find_actor_by_name(ClutterActor *inActor, const gchar *inName)
{
	ClutterActorIter	iter;
	ClutterActor		*child;
	ClutterActor		*result;

	g_return_val_if_fail(CLUTTER_IS_ACTOR(inActor), NULL);
	g_return_val_if_fail(inName && *inName, NULL);

	/* Check if given actor is the one we should lookup */
	if(g_strcmp0(clutter_actor_get_name(inActor), inName)==0) return(inActor);

	/* For each child of actor call ourselve recursive */
	clutter_actor_iter_init(&iter, inActor);
	while(clutter_actor_iter_next(&iter, &child))
	{
		result=xfdashboard_find_actor_by_name(child, inName);
		if(result) return(result);
	}

	/* If we get here you could not find actor having this name set */
	return(NULL);
}

/* Split a string into a NULL-terminated list of tokens using the delimiters and remove
 * white-spaces at the beginning and end of each token. Empty tokens will not be added.
 * Caller is responsible to free result with g_strfreev() if not NULL.
 */
gchar** xfdashboard_split_string(const gchar *inString, const gchar *inDelimiters)
{
	GSList			*tokens, *delimiters, *l;
	const gchar		*s, *tokenBegin;
	gunichar		c;
	guint			numberTokens;
	gchar			**result;

	g_return_val_if_fail(inString, NULL);
	g_return_val_if_fail(inDelimiters && *inDelimiters, NULL);

	tokens=NULL;
	delimiters=NULL;
	numberTokens=0;

	/* Build list of delimiters */
	s=inDelimiters;
	while(*s)
	{
		/* Get and check UTF-8 delimiter */
		c=g_utf8_get_char_validated(s, -1);
		s=g_utf8_next_char(s);
		if(c==0 || c==(gunichar)-1 || c==(gunichar)-2) continue;

		/* Add UTF-8 delimiter */
		delimiters=g_slist_prepend(delimiters, GINT_TO_POINTER(c));
	}

	/* Find beginning of each token */
	s=inString;
	tokenBegin=NULL;
	while(*s)
	{
		gboolean	isDelimiter;

		/* Get and check UTF-8 character */
		c=g_utf8_get_char_validated(s, -1);
		if(c==0 || c==(gunichar)-1 || c==(gunichar)-2)
		{
			s=g_utf8_next_char(s);
			continue;
		}

		/* Check if character is a delimiter */
		isDelimiter=FALSE;
		for(l=delimiters; !isDelimiter && l; l=g_slist_next(l))
		{
			if(c==(gunichar)GPOINTER_TO_INT(l->data)) isDelimiter=TRUE;
		}

		/* If character is a delimiter retrieve token, trim white-spaces
		 * and add to result list
		 */
		if(isDelimiter && tokenBegin)
		{
			tokens=g_slist_prepend(tokens, g_strndup(tokenBegin, s-tokenBegin));
			numberTokens++;
			tokenBegin=NULL;
		}

		/* If character is not delimiter remember current position in string
		 * as begin of next token if we have not remember one already
		 */
		if(!isDelimiter && !tokenBegin) tokenBegin=s;

		/* Move to next character in string */
		s=g_utf8_next_char(s);
	}

	/* Now we are out of loop to split string into token. But if we have
	 * still remembered a position of a beginning token it was NULL-terminated
	 * and not delimiter-terminated. Add it also to result list.
	 */
	if(tokenBegin)
	{
		tokens=g_slist_prepend(tokens, g_strdup(tokenBegin));
		numberTokens++;
	}

	/* Build result list as a NULL-terminated list of strings */
	result=g_new(gchar*, numberTokens+1);
	result[numberTokens]=NULL;
	for(l=tokens; l; l=g_slist_next(l))
	{
		numberTokens--;
		result[numberTokens]=l->data;
	}

	/* Release allocated resources */
	g_slist_free(delimiters);
	g_slist_free(tokens);

	/* Return result list */
	return(result);
}

/* Check if ID matches requirements that it has to begin either with one or
 * multiple '_' (underscore) following by a character of ASCII character set
 * or it has to begin with a character of ASCII character set directly. Each
 * following character in string can either be a digit, a character of
 * ASCII character set or any of the following symbols: '_' (underscore) or
 * '-' (minus).
 */
gboolean xfdashboard_is_valid_id(const gchar *inString)
{
	const gchar			*iter;

	g_return_val_if_fail(inString && *inString, FALSE);

	/* Check that all characters in string matches the allowed symbols,
	 * digits and characters. Find first one _NOT_ matching this requirement.
	 */
	iter=inString;
	while(*iter)
	{
		if(!g_ascii_isalnum(*iter) &&
			*iter!='_' &&
			*iter!='-')
		{
			return(FALSE);
		}

		/* Check next character in string */
		iter++;
	}

	/* Check if string begins either with '_' (underscore) or with a
	 * character of ASCII character set.
	 */
	if(*inString!='_' &&
		!g_ascii_isalpha(*inString))
	{
		return(FALSE);
	}

	/* If string begins with '_' (underscore) check that the first character
	 * which is not an underscore is a character of ASCII character set.
	 */
	iter=inString;
	while(*iter=='_') iter++;
	if(!g_ascii_isalpha(*iter)) return(FALSE);

	/* If we get here the ID matches the requirements so return TRUE. */
	return(TRUE);
}

/* Get textual representation for an enumeration value */
gchar* xfdashboard_get_enum_value_name(GType inEnumClass, gint inValue)
{
	GEnumClass		*enumClass;
	GEnumValue		*enumValue;
	gchar			*valueName;

	enumClass=NULL;
	enumValue=NULL;
	valueName=NULL;

	/* Reference enum class to keep it alive for transformation */
	enumClass=g_type_class_ref(inEnumClass);

	/* Get enum value */
	if(enumClass) enumValue=g_enum_get_value(enumClass, inValue);

	/* Get a copy of value's name if it could be found */
	if(enumValue)
	{
		valueName=g_strdup(enumValue->value_name);
	}

	/* Release allocated resources */
	if(enumClass) g_type_class_unref(enumClass);

	/* Return name */
	return(valueName);
}

/* Dump actors */
static void _xfdashboard_dump_actor_internal(ClutterActor *inActor, gint inLevel)
{
	ClutterActorIter	iter;
	ClutterActor		*child;
	gint				i;

	g_return_if_fail(CLUTTER_IS_ACTOR(inActor));
	g_return_if_fail(inLevel>=0);

	clutter_actor_iter_init(&iter, CLUTTER_ACTOR(inActor));
	while(clutter_actor_iter_next(&iter, &child))
	{
		for(i=0; i<inLevel; i++) g_print("  ");
		g_print("+- %s@%p - name: %s - geometry: %.2f,%.2f [%.2fx%.2f], mapped: %s, visible: %s, children: %d\n",
					G_OBJECT_TYPE_NAME(child), child,
					clutter_actor_get_name(child),
					clutter_actor_get_x(child),
					clutter_actor_get_y(child),
					clutter_actor_get_width(child),
					clutter_actor_get_height(child),
					CLUTTER_ACTOR_IS_MAPPED(child) ? "yes" : "no",
					CLUTTER_ACTOR_IS_VISIBLE(child) ? "yes" : "no",
					clutter_actor_get_n_children(child));
		if(clutter_actor_get_n_children(child)>0) _xfdashboard_dump_actor_internal(child, inLevel+1);
	}
}

void xfdashboard_dump_actor(ClutterActor *inActor)
{
	_xfdashboard_dump_actor_internal(inActor, 0);
}
