/*
 * utils: Common functions, helpers and definitions
 * 
 * Copyright 2012-2016 Stephan Haller <nomad@froevel.de>
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

#include <libxfdashboard/utils.h>

#include <glib/gi18n-lib.h>
#include <clutter/clutter.h>
#include <gtk/gtk.h>
#include <dbus/dbus-glib.h>
#include <gio/gdesktopappinfo.h>

#include <libxfdashboard/stage.h>
#include <libxfdashboard/stage-interface.h>
#include <libxfdashboard/window-tracker.h>

/**
 * SECTION:utils
 * @title: Utilities
 * @short_description: Common functions, helpers, macros and definitions
 * @include: xfdashboard/utils.h
 *
 * Utility functions to ease some common tasks.
 */

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

/**
 * xfdashboard_notify:
 * @inSender: The sending #ClutterActor or %NULL
 * @inIconName: The icon name to display in notification or %NULL
 * @inFormat: A standard printf() format string for notification text
 * @...: The parameters to insert into the format string
 *
 * Shows a notification with the formatted text as specified in @inFormat
 * and the parameters at the monitor where the sending actor @inSender
 * is placed on.
 *
 * If @inSender is NULL the primary monitor is used.
 *
 * If @inIconName is NULL no icon will be shown in notification.
 */
void xfdashboard_notify(ClutterActor *inSender,
							const gchar *inIconName,
							const gchar *inFormat, ...)
{
	XfdashboardStage					*stage;
	ClutterStageManager					*stageManager;
	va_list								args;
	gchar								*text;

	g_return_if_fail(inSender==NULL || CLUTTER_IS_ACTOR(inSender));

	stage=NULL;

	/* Build text to display */
	va_start(args, inFormat);
	text=g_strdup_vprintf(inFormat, args);
	va_end(args);

	/* Get stage of sending actor if available */
	if(inSender) stage=XFDASHBOARD_STAGE(clutter_actor_get_stage(inSender));

	/* No sending actor specified or no stage found so get default stage */
	if(!stage)
	{
		const GSList					*stages;
		const GSList					*stagesIter;
		ClutterActorIter				interfaceIter;
		ClutterActor					*child;
		XfdashboardWindowTrackerMonitor	*stageMonitor;

		/* Get stage manager to iterate through stages to find the one
		 * for primary monitor or at least the first stage.
		 */
		stageManager=clutter_stage_manager_get_default();

		/* Find stage for primary monitor and if we cannot find it
		 * use first stage.
		 */
		if(stageManager &&
			CLUTTER_IS_STAGE_MANAGER(stageManager))
		{
			/* Get list of all stages */
			stages=clutter_stage_manager_peek_stages(stageManager);

			/* Iterate through list of all stage and lookup the one for
			 * primary monitor.
			 */
			for(stagesIter=stages; stagesIter && !stage; stagesIter=stagesIter->next)
			{
				/* Skip this stage if it is not a XfdashboardStage */
				if(!XFDASHBOARD_IS_STAGE(stagesIter->data)) continue;

				/* Iterate through stage's children and lookup stage interfaces */
				clutter_actor_iter_init(&interfaceIter, CLUTTER_ACTOR(stagesIter->data));
				while(clutter_actor_iter_next(&interfaceIter, &child))
				{
					if(XFDASHBOARD_IS_STAGE_INTERFACE(child))
					{
						stageMonitor=xfdashboard_stage_interface_get_monitor(XFDASHBOARD_STAGE_INTERFACE(child));
						if(xfdashboard_window_tracker_monitor_is_primary(stageMonitor))
						{
							stage=XFDASHBOARD_STAGE(clutter_actor_get_stage(child));
						}
					}
				}
			}

			/* If we did not get stage for primary monitor use first stage */
			if(!stage && stages)
			{
				stage=XFDASHBOARD_STAGE(stages->data);
			}
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

/**
 * xfdashboard_create_app_context:
 * @inWorkspace: The workspace where to place application windows on or %NULL
 *
 * Returns a #GAppLaunchContext suitable for launching applications on the given display and workspace by GIO.
 *
 * If @inWorkspace is specified it sets workspace on which applications will be launched when using this context when running under a window manager that supports multiple workspaces.
 * When the workspace is not specified it is up to the window manager to pick one, typically it will be the current workspace.
 *
 * Return value: (transfer full): the newly created #GAppLaunchContext or %NULL in case of an error. Use g_object_unref() to free return value.
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

/**
 * xfdashboard_register_gvalue_transformation_funcs:
 *
 * Registers additional transformation functions used in #GValue to convert
 * values between types.
 */
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

/**
 * xfdashboard_find_actor_by_name:
 * @inActor: The root #ClutterActor where to begin searching
 * @inName: A string containg the name of the #ClutterActor to lookup
 *
 * Iterates through all children of @inActor recursively and looks for
 * the child having the name as specified at @inName.
 *
 * Return value: (transfer none): The #ClutterActor matching the name
 *               to lookup or %NULL if none was found.
 */
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

/**
 * xfdashboard_get_stage_of_actor:
 * @inActor: The #ClutterActor for which to find the stage
 *
 * Gets the #XfdashboardStageInterface of the monitor where
 * @inActor belongs to.
 *
 * Return value: (transfer none): The #XfdashboardStageInterface
 *               found or %NULL if none was found.
 */
XfdashboardStageInterface* xfdashboard_get_stage_of_actor(ClutterActor *inActor)
{
	ClutterActor		*parent;

	g_return_val_if_fail(CLUTTER_IS_ACTOR(inActor), NULL);

	/* Iterate through parents and return first XfdashboardStageInterface
	 * found. That's the stage of the monitor where the requested actor
	 * belongs to.
	 */
	parent=clutter_actor_get_parent(inActor);
	while(parent)
	{
		/* Check if current iterated parent is a XfdashboardStageInterface.
		 * If it is return it.
		 */
		if(XFDASHBOARD_IS_STAGE_INTERFACE(parent)) return(XFDASHBOARD_STAGE_INTERFACE(parent));

		/* Continue with next parent */
		parent=clutter_actor_get_parent(parent);
	}

	/* If we get here we did not find the stage the actor belongs to,
	 * so return NULL.
	 */
	return(NULL);
}

/**
 * xfdashboard_get_global_stage_of_actor:
 * @inActor: The #ClutterActor for which to find the global stage
 *
 * Gets the main #XfdashboardStage where @inActor belongs to.
 *
 * Return value: (transfer none): The #XfdashboardStage found
 *               or %NULL if none was found.
 */
XfdashboardStage* xfdashboard_get_global_stage_of_actor(ClutterActor *inActor)
{
	ClutterActor		*parent;

	g_return_val_if_fail(CLUTTER_IS_ACTOR(inActor), NULL);

	/* Iterate through parents and return first XfdashboardStage
	 * found. That's the main global and all monitors spanning
	 * stage where the requested actor belongs to.
	 */
	parent=clutter_actor_get_parent(inActor);
	while(parent)
	{
		/* Check if current iterated parent is a XfdashboardStage.
		 * If it is return it.
		 */
		if(XFDASHBOARD_IS_STAGE(parent)) return(XFDASHBOARD_STAGE(parent));

		/* Continue with next parent */
		parent=clutter_actor_get_parent(parent);
	}

	/* If we get here we did not find the global stage the actor
	 * belongs to, so return NULL.
	 */
	return(NULL);
}

/**
 * xfdashboard_split_string:
 * @inString: The string to split
 * @inDelimiters: A string containg the delimiters
 *
 * Split the string @inString into a NULL-terminated list of tokens using
 * the delimiters at @inDelimiters and removes white-spaces at the beginning
 * and end of each token. Empty tokens will not be added to list.
 *
 * Return value: A newly-allocated %NULL-terminated array of strings or %NULL
 *               in case of an error. Use g_strfreev() to free it.
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

/**
 * xfdashboard_is_valid_id:
 * @inString: The string containing the ID to check
 *
 * Checks if ID specified at @inString matches the requirements to be a
 * valid ID.
 *
 * To be a valid ID it has to begin either with one or multiple '_' (underscores)
 * following by a character of ASCII character set or it has to begin with a
 * character of ASCII character set directly. Each following character can either
 * be a digit, a character of ASCII character set or any of the following symbols:
 * '_' (underscore) or '-' (minus).
 *
 * Return value: %TRUE if @inString contains a valid ID, otherwise %FALSE
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

/**
 * xfdashboard_get_enum_value_name:
 * @inEnumClass: The #GType of enum class
 * @inValue: The numeric value of enumeration at @inEnumClass
 *
 * Returns textual representation for numeric value @inValue of
 * enumeration class @inEnumClass.
 *
 * Return value: A string containig the textual representation or
 *               %NULL if @inValue is not a value of enumeration
 *               @inEnumClass. Use g_free() to free returned string.
 */
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

/**
 * xfdashboard_dump_actor:
 * @inActor: The #ClutterActor to dump
 *
 * Dumps a textual representation of actor specified in @inActor
 * to console. The dump contains all children recursively displayed
 * in a tree. Each entry contains the object class name, address,
 * position and size of this actor and also the state like is-mapped,
 * is-visible and a number of children it contains.
 *
 * This functions is for debugging purposes and should not be used
 * normally.
 */
void xfdashboard_dump_actor(ClutterActor *inActor)
{
	_xfdashboard_dump_actor_internal(inActor, 0);
}
