/*
 * utils: Common functions, helpers and definitions
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

/**
 * SECTION:utils
 * @title: Utilities
 * @short_description: Common functions, helpers, macros and definitions
 * @include: xfdashboard/utils.h
 *
 * Utility functions to ease some common tasks.
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <libxfdashboard/utils.h>

#include <glib/gi18n-lib.h>
#include <clutter/clutter.h>
#include <gtk/gtk.h>
#ifdef XFCONF_LEGACY
#include <dbus/dbus-glib.h>
#endif
#include <gio/gdesktopappinfo.h>

#include <libxfdashboard/stage.h>
#include <libxfdashboard/stage-interface.h>
#include <libxfdashboard/window-tracker.h>
#include <libxfdashboard/application.h>
#include <libxfdashboard/compat.h>
#include <libxfdashboard/debug.h>


/* Gobject type for pointer arrays (GPtrArray) */
GType xfdashboard_pointer_array_get_type(void)
{
	static volatile gsize	type__volatile=0;
	GType					type;

	if(g_once_init_enter(&type__volatile))
	{
#ifdef XFCONF_LEGACY
		type=dbus_g_type_get_collection("GPtrArray", G_TYPE_VALUE);
#else
		type=G_TYPE_PTR_ARRAY;
#endif
		g_once_init_leave(&type__volatile, type);
	}

	return(type__volatile);
}

/* Callback function for xfdashboard_traverse_actor() to find stage interface
 * used in xfdashboard_notify().
 */
static gboolean _xfdashboard_notify_traverse_callback(ClutterActor *inActor, gpointer inUserData)
{
	XfdashboardStage					**outStageInterface;
	XfdashboardWindowTrackerMonitor		*stageMonitor;

	g_return_val_if_fail(CLUTTER_IS_ACTOR(inActor), XFDASHBOARD_TRAVERSAL_CONTINUE);
	g_return_val_if_fail(inUserData, XFDASHBOARD_TRAVERSAL_CONTINUE);

	outStageInterface=(XfdashboardStage**)inUserData;

	/* If actor currently traverse is a stage interface then store
	 * the actor in user-data and stop further traversal.
	 */
	if(XFDASHBOARD_IS_STAGE_INTERFACE(inActor))
	{
		stageMonitor=xfdashboard_stage_interface_get_monitor(XFDASHBOARD_STAGE_INTERFACE(inActor));
		if(xfdashboard_window_tracker_monitor_is_primary(stageMonitor))
		{
			*outStageInterface=XFDASHBOARD_STAGE(clutter_actor_get_stage(inActor));
			return(XFDASHBOARD_TRAVERSAL_STOP);
		}
	}

	/* If we get here a stage interface was not found so continue traversal */
	return(XFDASHBOARD_TRAVERSAL_CONTINUE);
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
		XfdashboardCssSelector			*selector;

		/* Traverse through actors to find stage */
		selector=xfdashboard_css_selector_new_from_string("XfdashboardStageInterface");
		xfdashboard_traverse_actor(NULL, selector, _xfdashboard_notify_traverse_callback, &stage);
		g_object_unref(selector);

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
 * Returns a #GAppLaunchContext suitable for launching applications on the
 * given display and workspace by GIO.
 *
 * If @inWorkspace is specified it sets workspace on which applications will
 * be launched when using this context when running under a window manager
 * that supports multiple workspaces.
 *
 * When the workspace is not specified it is up to the window manager to pick
 * one, typically it will be the current workspace.
 *
 * Return value: (transfer full): the newly created #GAppLaunchContext or %NULL
 *   in case of an error. Use g_object_unref() to free return value.
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
			XFDASHBOARD_DEBUG(NULL, MISC,
								"Cannot get value for unknown enum '%s' for type %s",
								value,
								g_type_name(G_VALUE_TYPE(ioDestValue)));
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
				XFDASHBOARD_DEBUG(NULL, MISC,
									"Cannot get value for unknown flag '%s' for type %s",
									*entry,
									g_type_name(G_VALUE_TYPE(ioDestValue)));
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
 *   to lookup or %NULL if none was found.
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

/* Internal function to traverse an actor which can be call recursively */
static gboolean _xfdashboard_traverse_actor_internal(ClutterActor *inActor,
														XfdashboardCssSelector *inSelector,
														XfdashboardTraversalCallback inCallback,
														gpointer inUserData)
{
	ClutterActorIter	iter;
	ClutterActor		*child;
	gint				score;
	gboolean			doContinueTraversal;

	g_return_val_if_fail(CLUTTER_IS_ACTOR(inActor), XFDASHBOARD_TRAVERSAL_CONTINUE);
	g_return_val_if_fail(XFDASHBOARD_IS_CSS_SELECTOR(inSelector), XFDASHBOARD_TRAVERSAL_CONTINUE);
	g_return_val_if_fail(inCallback, XFDASHBOARD_TRAVERSAL_CONTINUE);

	/* Check if given actor matches selector if a selector is provided
	 * otherwise each child will match. Call callback for matching children.
	 */
	if(XFDASHBOARD_IS_STYLABLE(inActor))
	{
		score=xfdashboard_css_selector_score(inSelector, XFDASHBOARD_STYLABLE(inActor));
		if(score>=0)
		{
			doContinueTraversal=(inCallback)(inActor, inUserData);
			if(!doContinueTraversal) return(doContinueTraversal);
		}
	}

	/* For each child of actor call ourselve recursive */
	clutter_actor_iter_init(&iter, inActor);
	while(clutter_actor_iter_next(&iter, &child))
	{
		doContinueTraversal=_xfdashboard_traverse_actor_internal(child, inSelector, inCallback, inUserData);
		if(!doContinueTraversal) return(doContinueTraversal);
	}

	/* If we get here return and continue traversal */
	return(XFDASHBOARD_TRAVERSAL_CONTINUE);
}

/**
 * xfdashboard_traverse_actor:
 * @inRootActor: The root #ClutterActor where to begin traversing
 * @inSelector: A #XfdashboardCssSelector to filter actors while traversing or
 *   %NULL to disable filterting
 * @inCallback: Function to call on matching children
 * @inUserData: Data to pass to callback function
 *
 * Iterates through all children of @inRootActor recursively beginning at
 * @inRootActor and for each child matching the selector @inSelector it calls the
 * callback function @inCallback with the matching child and the user-data at
 * @inUserData.
 *
 * If @inRootActor is %NULL it begins at the global stage.
 *
 * If the selector @inSelector is %NULL all children will match and the callback
 * function @inCallback is called for all children.
 */
void xfdashboard_traverse_actor(ClutterActor *inRootActor,
								XfdashboardCssSelector *inSelector,
								XfdashboardTraversalCallback inCallback,
								gpointer inUserData)
{
	g_return_if_fail(!inRootActor || CLUTTER_IS_ACTOR(inRootActor));
	g_return_if_fail(!inSelector || XFDASHBOARD_IS_CSS_SELECTOR(inSelector));
	g_return_if_fail(inCallback);

	/* If root actor where begin traversal is NULL then begin at stage */
	if(!inRootActor)
	{
		inRootActor=CLUTTER_ACTOR(xfdashboard_application_get_stage(NULL));

		/* If root actor is still NULL then no stage was found and we cannot
		 * start the traversal.
		 */
		if(!inRootActor)
		{
			XFDASHBOARD_DEBUG(NULL, MISC, "No root actor to begin traversal at was provided and no stage available");
			return;
		}
	}

	/* If no selector is provider create a seletor matching all actors.
	 * Otherwise take an extra ref on provided selector to prevent
	 * destruction when we unref it later.
	 */
	if(!inSelector) inSelector=xfdashboard_css_selector_new_from_string("*");
		else g_object_ref(inSelector);

	/* Do traversal */
	_xfdashboard_traverse_actor_internal(inRootActor, inSelector, inCallback, inUserData);

	/* Release reference on selector */
	g_object_unref(inSelector);
}

/**
 * xfdashboard_get_stage_of_actor:
 * @inActor: The #ClutterActor for which to find the stage
 *
 * Gets the #XfdashboardStageInterface of the monitor where
 * @inActor belongs to.
 *
 * Return value: (transfer none): The #XfdashboardStageInterface
 *   found or %NULL if none was found.
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
 * xfdashboard_split_string:
 * @inString: The string to split
 * @inDelimiters: A string containg the delimiters
 *
 * Split the string @inString into a NULL-terminated list of tokens using
 * the delimiters at @inDelimiters and removes white-spaces at the beginning
 * and end of each token. Empty tokens will not be added to list.
 *
 * Return value: A newly-allocated %NULL-terminated array of strings or %NULL
 *   in case of an error. Use g_strfreev() to free it.
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

/**
 * xfdashboard_get_enum_value_from_nickname:
 * @inEnumClass: The #GType of enum class
 * @inNickname: The nickname for value of enumeration at @inEnumClass
 *
 * Returns integer value for nickname @inNickname of
 * enumeration class @inEnumClass.
 *
 * NOTE: %G_MININT will be returned even if nickname was not found but it can
 *       match the value of enumeration. So do not use this function if
 *       %G_INTMIN could be a valid value for enumeration
 *
 * Return value: An integer value for nickname of enumeration or %G_MININT
 *               if nickname was not found.
 */
gint xfdashboard_get_enum_value_from_nickname(GType inEnumClass, const gchar *inNickname)
{
	GEnumClass		*enumClass;
	GEnumValue		*enumValue;
	gint			value;

	enumClass=NULL;
	enumValue=NULL;
	value=G_MININT;

	/* Reference enum class to keep it alive for transformation */
	enumClass=g_type_class_ref(inEnumClass);

	/* Get enum value */
	if(enumClass) enumValue=g_enum_get_value_by_nick(enumClass, inNickname);

	/* Get a copy of value's name if it could be found */
	if(enumValue)
	{
		value=enumValue->value;
	}

	/* Release allocated resources */
	if(enumClass) g_type_class_unref(enumClass);

	/* Return integer value */
	return(value);
}
/* Dump actors */
static void _xfdashboard_dump_actor_print(ClutterActor *inActor, gint inLevel)
{
	XfdashboardStylable		*stylable;
	ClutterActorBox			allocation;
	gint					i;

	g_return_if_fail(CLUTTER_IS_ACTOR(inActor));
	g_return_if_fail(inLevel>=0);

	/* Check if actor is stylable to retrieve style configuration */
	stylable=NULL;
	if(XFDASHBOARD_IS_STYLABLE(inActor)) stylable=XFDASHBOARD_STYLABLE(inActor);

	/* Dump actor */
	for(i=0; i<inLevel; i++) g_print("  ");
	clutter_actor_get_allocation_box(inActor, &allocation);
	g_print("+- %s@%p [%s%s%s%s%s%s] - geometry: %.2f,%.2f [%.2fx%.2f], mapped: %s, visible: %s, layout: %s, children: %d\n",
				G_OBJECT_TYPE_NAME(inActor), inActor,
				clutter_actor_get_name(inActor) ? " #" : "",
				clutter_actor_get_name(inActor) ? clutter_actor_get_name(inActor) : "",
				stylable && xfdashboard_stylable_get_classes(stylable) ? "." : "",
				stylable && xfdashboard_stylable_get_classes(stylable) ? xfdashboard_stylable_get_classes(stylable) : "",
				stylable && xfdashboard_stylable_get_pseudo_classes(stylable) ? ":" : "",
				stylable && xfdashboard_stylable_get_pseudo_classes(stylable) ? xfdashboard_stylable_get_pseudo_classes(stylable) : "",
				allocation.x1,
				allocation.y1,
				allocation.x2-allocation.x1,
				allocation.y2-allocation.y1,
				clutter_actor_is_mapped(inActor) ? "yes" : "no",
				clutter_actor_is_visible(inActor) ? "yes" : "no",
				clutter_actor_get_layout_manager(inActor) ? G_OBJECT_TYPE_NAME(clutter_actor_get_layout_manager(inActor)) : "none",
				clutter_actor_get_n_children(inActor));

}

static void _xfdashboard_dump_actor_internal(ClutterActor *inActor, gint inLevel)
{
	ClutterActorIter		iter;
	ClutterActor			*child;

	g_return_if_fail(CLUTTER_IS_ACTOR(inActor));
	g_return_if_fail(inLevel>0);

	/* Dump children */
	clutter_actor_iter_init(&iter, CLUTTER_ACTOR(inActor));
	while(clutter_actor_iter_next(&iter, &child))
	{
		_xfdashboard_dump_actor_print(child, inLevel);
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
	g_return_if_fail(CLUTTER_IS_ACTOR(inActor));

	/* Dump the requested top-level actor */
	_xfdashboard_dump_actor_print(inActor, 0);

	/* Dump children of top-level actor which calls itself recursive
	 * if any child has children.
	 */
	_xfdashboard_dump_actor_internal(inActor, 1);
}
