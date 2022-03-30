/*
 * xfconf-settings: A class storing settings in xfconf permanently
 * 
 * Copyright 2012-2021 Stephan Haller <nomad@froevel.de>
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
 * SECTION:xfconf-settings
 * @short_description: The xfconf settings storage driver
 * @include: common/xfconf-settings.h
 *
 * This class #XfdashboardXfconfSettings contains all settings for this
 * library and stores them at xfconf permanently.
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <common/xfconf-settings.h>

#include <xfconf/xfconf.h>

#include <libxfdashboard/debug.h>


/* Define this class in GObject system */
struct _XfdashboardXfconfSettingsPrivate
{
	/* Instance related */
	XfconfChannel		*channel;
	guint				xfconfNotifySignalID;
	GList				*blockedSettings;
};

G_DEFINE_TYPE_WITH_PRIVATE(XfdashboardXfconfSettings,
							xfdashboard_xfconf_settings,
							XFDASHBOARD_TYPE_SETTINGS);


/* IMPLEMENTATION: Private variables and methods */
#define XFDASHBOARD_XFCONF_CHANNEL					"xfdashboard"
#define XFDASHBOARD_XFCONF_SETTINGS_PLUGINS_PATH	"plugins"

#if 0==1

// OLD: #define THEME_NAME_XFCONF_PROP		"/theme"
#define THEME_NAME_XFCONF_PROP				"theme" -> "xfdashboard" (G_TYPE_STRING)

// OLD: #define SORT_MODE_XFCONF_PROP		"/components/applications-search-provider/sort-mode"
#define SORT_MODE_XFCONF_PROP				"applications-search-sort-mode" -> XFDASHBOARD_APPLICATIONS_SEARCH_PROVIDER_SORT_MODE_NONE (enum)

// OLD: #define SHOW_ALL_APPS_XFCONF_PROP	"/components/applications-view/show-all-apps"
#define SHOW_ALL_APPS_XFCONF_PROP			"show-all-applications" -> FALSE (G_TYPE_BOOLEAN)

#define ALLOW_SUBWINDOWS_XFCONF_PROP		"allow-subwindows" -> TRUE (G_TYPE_BOOLEAN)

#define ENABLED_PLUGINS_XFCONF_PROP			"enabled-plugins" -> [] (Type: G_TYPE_STRV - G_TYPE_BOXED)

#define FAVOURITES_XFCONF_PROP				"favourites" -> [ "exo-web-browser.desktop", "exo-mail-reader.desktop", "exo-file-manager.desktop", "exo-terminal-emulator.desktop" ]  (Type: G_TYPE_STRV - G_TYPE_BOXED)
static const gchar*	_xfdashboard_xfconf_settings_default_favourites[]=	{
																	"exo-web-browser.desktop",
																	"exo-mail-reader.desktop",
																	"exo-file-manager.desktop",
																	"exo-terminal-emulator.desktop",
																	NULL
																};

#define LAUNCH_NEW_INSTANCE_XFCONF_PROP		"always-launch-new-instance" -> TRUE (G_TYPE_BOOLEAN)

// OLD: #define DELAY_SEARCH_TIMEOUT_XFCONF_PROP	"/components/search-view/delay-search-timeout"
#define DELAY_SEARCH_TIMEOUT_XFCONF_PROP			"delay-search-timeout" -> 0 (G_TYPE_UINT)
#define DEFAULT_DELAY_SEARCH_TIMEOUT				0

// OLD: #define SCROLL_EVENT_CHANGES_WORKSPACE_XFCONF_PROP		"/components/windows-view/scroll-event-changes-workspace"
#define SCROLL_EVENT_CHANGES_WORKSPACE_XFCONF_PROP				"scroll-event-changes-workspace" -> FALSE (G_TYPE_BOOLEAN)

#define WORKAROUND_UNMAPPED_WINDOW_XFCONF_PROP				"enable-unmapped-window-workaround" -> FALSE (G_TYPE_BOOLEAN)

#define WINDOW_CONTENT_CREATION_PRIORITY_XFCONF_PROP		"window-content-creation-priority" -> "immediate" (XFDASHBOARD_TYPE_WINDOW_CONTENT_X11_WORKAROUND_MODE ? string?)

#define ENABLE_ANIMATIONS_XFCONF_PROP		"enable-animations"

#define NOTIFICATION_TIMEOUT_XFCONF_PROP				"min-notification-timeout"
#define RESET_SEARCH_ON_RESUME_XFCONF_PROP				"reset-search-on-resume"
#define SWITCH_VIEW_ON_RESUME_XFCONF_PROP				"switch-to-view-on-resume"
#define RESELECT_THEME_FOCUS_ON_RESUME_XFCONF_PROP		"reselect-theme-focus-on-resume"

#endif

typedef struct _XfdashboardXfconfSettingsBlockedProperty	XfdashboardXfconfSettingsBlockedProperty;
struct _XfdashboardXfconfSettingsBlockedProperty
{
	guint					refCount;
	gchar					*pluginID;
	gchar					*property;
};

typedef struct _XfdashboardXfconfSettingsMapping	XfdashboardXfconfSettingsMapping;
struct _XfdashboardXfconfSettingsMapping
{
	const gchar				*pluginID;
	const gchar				*settingsName;
	const gchar				*xfconfPropertyName;
};

static XfdashboardXfconfSettingsMapping		_xfdashboard_xfconf_settings_mapping[]=
												{
													{ NULL, "applications-search-sort-mode", "/components/applications-search-provider/sort-mode" },
													{ NULL, "show-all-applications", "/components/applications-view/show-all-apps" },
													{ NULL, "delay-search-timeout", "/components/search-view/delay-search-timeout" },
													{ NULL, "scroll-event-changes-workspace", "/components/windows-view/scroll-event-changes-workspace" },
													{ NULL, NULL, NULL }
													
												};

/* Create new or reference existing blocking of property changed notification */
static void _xfdashboard_xfconf_settings_block_property_notification_ref(XfdashboardXfconfSettings *self,
																			const gchar *inPluginID,
																			const gchar *inProperty)
{
	XfdashboardXfconfSettingsPrivate			*priv;
	GList										*iter;
	XfdashboardXfconfSettingsBlockedProperty	*block;

	g_return_if_fail(XFDASHBOARD_IS_XFCONF_SETTINGS(self));
	g_return_if_fail(inPluginID==NULL || *inPluginID);
	g_return_if_fail(inProperty && *inProperty);

	priv=self->priv;

	/* Iterate through list to find exisiting blocking to reference */
	for(iter=priv->blockedSettings; iter; iter=g_list_next(iter))
	{
		/* Get currently iterated blocking */
		block=(XfdashboardXfconfSettingsBlockedProperty*)iter->data;
		if(!block) continue;

		/* Check if currently iterated blocking is matches the property to lookup */
		if(g_strcmp0(block->pluginID, inPluginID)==0 &&
			g_strcmp0(block->property, inProperty)==0)
		{
			/* Found blocking for property, so increase reference counter */
			block->refCount++;

			/* Done */
			return;
		}
	}

	/* No blocking for property found, so create new entry */
	block=g_new0(XfdashboardXfconfSettingsBlockedProperty, 1);
	block->refCount=1;
	block->pluginID=g_strdup(inPluginID);
	block->property=g_strdup(inProperty);

	/* Add new blocking to list */
	priv->blockedSettings=g_list_prepend(priv->blockedSettings, block);
}

/* Frees allocated resources of a property notification blocking */
static void _xfdashboard_xfconf_settings_block_property_notification_free(XfdashboardXfconfSettingsBlockedProperty *inBlock)
{
	g_return_if_fail(inBlock);

	/* Release allocated resources of blocking */
	g_free(inBlock->property);
	g_free(inBlock->pluginID);
	g_free(inBlock);
}

/* Unreference existing blocking of property changed notification and destroy
 * if counter drops to zero.
 */
static void _xfdashboard_xfconf_settings_block_property_notification_unref(XfdashboardXfconfSettings *self,
																			const gchar *inPluginID,
																			const gchar *inProperty)
{
	XfdashboardXfconfSettingsPrivate			*priv;
	GList										*iter;
	XfdashboardXfconfSettingsBlockedProperty	*block;

	g_return_if_fail(XFDASHBOARD_IS_XFCONF_SETTINGS(self));
	g_return_if_fail(inPluginID==NULL || *inPluginID);
	g_return_if_fail(inProperty && *inProperty);

	priv=self->priv;

	/* Iterate through list to find exisiting blocking to reference */
	for(iter=priv->blockedSettings; iter; iter=g_list_next(iter))
	{
		/* Get currently iterated blocking */
		block=(XfdashboardXfconfSettingsBlockedProperty*)iter->data;
		if(!block) continue;

		/* Check if currently iterated blocking is matches the property to lookup */
		if(g_strcmp0(block->pluginID, inPluginID)==0 &&
			g_strcmp0(block->property, inProperty)==0)
		{
			/* Found blocking for property, so decrease reference counter */
			block->refCount--;

			/* If block counter drops to zero, destroy and remove blocking */
			if(block->refCount==0)
			{
				/* Release allocated resources of blocking */
				_xfdashboard_xfconf_settings_block_property_notification_free(block);

				/* Remove from list */
				priv->blockedSettings=g_list_delete_link(priv->blockedSettings, iter);
			}

			/* Done */
			return;
		}
	}
}

/* Checks for blocking of changed notification for requested property */
static gboolean _xfdashboard_xfconf_settings_block_property_notification_is_blocked(XfdashboardXfconfSettings *self,
																					const gchar *inPluginID,
																					const gchar *inProperty)
{
	XfdashboardXfconfSettingsPrivate			*priv;
	GList										*iter;
	XfdashboardXfconfSettingsBlockedProperty	*block;

	g_return_val_if_fail(XFDASHBOARD_IS_XFCONF_SETTINGS(self), FALSE);
	g_return_val_if_fail(inPluginID==NULL || *inPluginID, FALSE);
	g_return_val_if_fail(inProperty && *inProperty, FALSE);

	priv=self->priv;

	/* Iterate through list to find exisiting blocking to reference */
	for(iter=priv->blockedSettings; iter; iter=g_list_next(iter))
	{
		/* Get currently iterated blocking */
		block=(XfdashboardXfconfSettingsBlockedProperty*)iter->data;
		if(!block) continue;

		/* Check if currently iterated blocking is matches the property to lookup */
		if(g_strcmp0(block->pluginID, inPluginID)==0 &&
			g_strcmp0(block->property, inProperty)==0)
		{
			 return(TRUE);
		}
	}

	/* No blocking for property found, so it is not blocked */
	return(FALSE);
}

/* Find mapping for xfconf property for requested plugin ID and settings name */
static const XfdashboardXfconfSettingsMapping* _xfdashboard_xfconf_settings_find_xfconf_property(const gchar *inPluginID,
																									const gchar *inSettingsName)
{
	const XfdashboardXfconfSettingsMapping	*iter;

	g_return_val_if_fail(inPluginID==NULL || *inPluginID, NULL);
	g_return_val_if_fail(*inSettingsName, NULL);

	/* Iterate through mapping and lookup match for requested plugin ID and settings
	 * name. If found, return the xfconf property.
	 */
	for(iter=_xfdashboard_xfconf_settings_mapping; iter->settingsName!=NULL; iter++)
	{
		if(g_strcmp0(iter->pluginID, inPluginID)==0 &&
			g_strcmp0(iter->settingsName, inSettingsName)==0)
		{
			return(iter);
		}
	}

	/* If we get here, we did not find a match, so return NULL */
	return(NULL);
}

/* Signal handler for changed settings */
static void _xfdashboard_xfconf_settings_on_settings_changed(XfdashboardSettings *inSettings,
																const gchar *inPluginID,
																GParamSpec *inParamSpec)
{
	XfdashboardXfconfSettings				*self;
	XfdashboardXfconfSettingsPrivate		*priv;
	GObject									*pluginSettings;
	const gchar								*settingsName;
	const XfdashboardXfconfSettingsMapping	*mapping;
	const gchar								*xfconfPropertyName;
	gboolean								success;
	GValue									value=G_VALUE_INIT;
	GType									transformType;

	g_return_if_fail(XFDASHBOARD_IS_XFCONF_SETTINGS(inSettings));
	g_return_if_fail(inPluginID==NULL || *inPluginID);
	g_return_if_fail(G_IS_PARAM_SPEC(inParamSpec));

	self=XFDASHBOARD_XFCONF_SETTINGS(inSettings);
	priv=self->priv;
	pluginSettings=NULL;
	success=TRUE;

	/* Get settings name from parameter specification */
	settingsName=g_param_spec_get_name(inParamSpec);

	/* If settings name of plugin ID is blocked, skip it */
	if(_xfdashboard_xfconf_settings_block_property_notification_is_blocked(self, inPluginID, settingsName))
	{
		XFDASHBOARD_DEBUG(self, MISC,
							"Skipping blocked setting '%s' at %s%s",
							settingsName,
							inPluginID ? "plug-in settings for plug-in " : "core settings",
							inPluginID ? inPluginID : "");
		return;
	}

	/* Check if we need to map changed settings of plugin to another xfconf
	 * property name as usual.
	 */
	xfconfPropertyName=settingsName;
	mapping=_xfdashboard_xfconf_settings_find_xfconf_property(inPluginID, settingsName);
	if(mapping)
	{
		inPluginID=mapping->pluginID;
		xfconfPropertyName=mapping->xfconfPropertyName;
	}

	/* If plugin ID is provided, lookup settings object instance for
	 * plugin ID.
	 */
	if(inPluginID)
	{
		XfdashboardPlugin					*plugin;
		XfdashboardPluginSettings			*settings;

		/* Get plugin for plugin ID provided */
		plugin=xfdashboard_settings_lookup_plugin_by_id(XFDASHBOARD_SETTINGS(self), inPluginID);
		if(!plugin)
		{
			XFDASHBOARD_DEBUG(self, MISC,
								"Could not get settings object for unknown plug-in ID '%s'",
								inPluginID);
			return;
		}

		/* Get plugin settings from plugin. If we did not find a settings object
		 * instance, we cannot modify any settings at it, so return immediately
		 * and do nothing.
		 */
		settings=xfdashboard_plugin_get_settings(plugin);
		if(!settings)
		{
			XFDASHBOARD_DEBUG(self, MISC,
								"No settings object instance for plug-in ID '%s'",
								inPluginID);
			return;
		}

		pluginSettings=G_OBJECT(settings);
	}
		else pluginSettings=G_OBJECT(self);

	/* Get value from changed settings */
	g_object_get_property(pluginSettings, g_param_spec_get_name(inParamSpec), &value);

	/* Some types are not supported by xfconf directly and need
	 * to be converted, e.g. enums to ints and flags to uints.
	 */
	transformType=G_TYPE_INVALID;
	if(G_VALUE_HOLDS_ENUM(&value)) transformType=G_TYPE_INT;
		else if(G_VALUE_HOLDS_FLAGS(&value)) transformType=G_TYPE_UINT;
		else if(CLUTTER_VALUE_HOLDS_COLOR(&value)) transformType=G_TYPE_STRING;

	if(transformType!=G_TYPE_INVALID)
	{
		GValue								convertedValue=G_VALUE_INIT;

		g_value_init(&convertedValue, transformType);
#ifdef DEBUG
		XFDASHBOARD_DEBUG(self, MISC,
							"Need to convert value of settings '%s' of type '%s' to type '%s' for xfconf property '%s'",
							settingsName,
							G_VALUE_TYPE_NAME(&value),
							G_VALUE_TYPE_NAME(&convertedValue),
							xfconfPropertyName);
#endif
		if(g_value_transform(&value, &convertedValue))
		{
			/* Transformation succeeded, so copy converted value to set it */
			g_value_unset(&value);
			g_value_init(&value, transformType);
			g_value_copy(&convertedValue, &value);
		}
			/* Tranformation failed, so show error and set failed state */
			else
			{
				/* Show error */
				g_warning("Cannot transform settings '%s' for xfconf property '%s' from type '%s' to type '%s'",
							settingsName,
							xfconfPropertyName,
							G_VALUE_TYPE_NAME(&value),
							G_VALUE_TYPE_NAME(&convertedValue));

				/* Set failed stat */
				success=FALSE;
			}

		/* Unset converted value */
		g_value_unset(&convertedValue);
	}

	/* Set or reset value at xfconf property */
	if(success)
	{
		gchar		*realXfconfPropertyName;
		gboolean	needsReset;
#ifdef DEBUG
		gchar		*valueText;
#endif

		/* Build xfconf property path for plugins if plugin ID provided and
		 * property names at Xfconf MUST begin with '/'
		 */
		if(inPluginID)
		{
			realXfconfPropertyName=g_strdup_printf("/%s/%s%s%s",
													XFDASHBOARD_XFCONF_SETTINGS_PLUGINS_PATH,
													inPluginID,
													*xfconfPropertyName!='/' ? "/" : "",
													xfconfPropertyName);
		}
			else
			{
				realXfconfPropertyName=g_strdup_printf("%s%s",
														*xfconfPropertyName!='/' ? "/" : "",
														xfconfPropertyName);
			}

		/* Block setting of plugin ID to prevent recursion by signal handling */ 
		_xfdashboard_xfconf_settings_block_property_notification_ref(self, inPluginID, settingsName);

		/* Check if property needs to be resetted, i.e. strings or string list
		 * with NULL pointer.
		 */
		needsReset=FALSE;
		if(G_VALUE_HOLDS_STRING(&value) ||
			G_VALUE_TYPE(&value)==G_TYPE_STRV)
		{
			if(value.data[0].v_pointer==NULL)
			{
				XFDASHBOARD_DEBUG(self, MISC,
									"Need to reset xfconf property '%s' as value of settings '%s' of type '%s' has null pointer",
									xfconfPropertyName,
									settingsName,
									G_VALUE_TYPE_NAME(&value));
				needsReset=TRUE;
			}
		}

		/* Set or reset value at (real) xfconf property */
#ifdef DEBUG
		valueText=g_strdup_value_contents(&value);
		XFDASHBOARD_DEBUG(self, MISC,
							"Setting value '%s' of type '%s' of settings '%s' at xfconf property '%s'",
							valueText,
							g_type_name(G_VALUE_TYPE(&value)),
							settingsName,
							realXfconfPropertyName);
		g_free(valueText);
#endif

		if(G_LIKELY(!needsReset))
		{
			success=xfconf_channel_set_property(priv->channel, realXfconfPropertyName, &value);
		}
			else
			{
				xfconf_channel_reset_property(priv->channel, realXfconfPropertyName, FALSE);
				success=TRUE;
			}

		if(!success)
		{
			g_warning("Could not %s value of settings '%s' at xfconf property '%s' of type '%s'",
						needsReset ? "reset" : "set",
						settingsName,
						realXfconfPropertyName,
						g_type_name(G_VALUE_TYPE(&value)));
		}

		/* Unblock setting of plugin ID */ 
		_xfdashboard_xfconf_settings_block_property_notification_unref(self, inPluginID, settingsName);

		/* Release allocated resources */
		g_free(realXfconfPropertyName);
	}

	/* Release allocated resources */
	g_value_unset(&value);
}

/* Determine plugin ID and settings name for xfconf property */
static gboolean _xfdashboard_xfconf_settings_find_plugin_and_setting(const gchar *inXfconfProperty,
																		gchar **outPluginID,
																		gchar **outSettingsName)
{
	const XfdashboardXfconfSettingsMapping	*iter;
	gchar									**pathElements;
	guint									pathCount;

	g_return_val_if_fail(inXfconfProperty==NULL || *inXfconfProperty, FALSE);
	g_return_val_if_fail(outPluginID && *outPluginID==NULL, FALSE);
	g_return_val_if_fail(outSettingsName && *outSettingsName==NULL, FALSE);

	/* Iterate through mapping and lookup match for requested xfconf property. If found,
	 * return plugin ID and settings name.
	 */
	for(iter=_xfdashboard_xfconf_settings_mapping; iter->settingsName!=NULL; iter++)
	{
		/* Check if iterated mapping matches xfconf property */
		if(g_strcmp0(iter->xfconfPropertyName, inXfconfProperty)==0)
		{
			/* Copy plugin ID and settings name for result */
			*outPluginID=g_strdup(iter->pluginID);
			*outSettingsName=g_strdup(iter->settingsName);

			/* Match found and returned value set up, so return TRUE */
			return(TRUE);
		}
	}

	/* We found no mapping, so split up xfconf property name in plugin ID and
	 * settings name.
	 * To get plugin ID we need the first path element after '/plugins' and
	 * the last path element will be the settings name, for example
	 * '/plugins/myplugin/mysetting' will return 'myplugin' as plugin ID and
	 * 'mysetting' as settings name.
	 * If xfconf's property name does not start with '/plugins' then core
	 * settings are retrieved, so plugin ID is NULL and the property name
	 * is the settings name.
	 * When splitting property into path elements seperated by '/', then
	 * we need only to check maximal 5 elements, so set limit to 5. We
	 * first element will also be empty as Xfconf properties must be begin
	 * with '/'. If the second one is '/plugins' then retrieve plugin ID
	 * (third element) and settings name (fourth element) and there MUST be
	 * no fifth element. If second one is not '/plugins' then retrieve
	 * settings name (second element) and there MUST be no third element.
	 */
	pathElements=g_strsplit(inXfconfProperty, "/", 5);
	pathCount=g_strv_length(pathElements);

	if(pathCount<1 || *pathElements[0]!=0)
	{
		/* No elements available or first element is not empty, so we
		 * will fail to retrieve the plugin ID and the settings name.
		 * Release allocated memory and return FALSE.
		 */
		g_strfreev(pathElements);

		return(FALSE);
	}

	if(pathCount==2)
	{
		/* We have exactly two elements and first one is empty as checked
		 * before, so it will be a core setting. Copy settings name,
		 * release allocated memory and return TRUE.
		 */
		*outSettingsName=g_strdup(pathElements[1]);

		g_strfreev(pathElements);

		return(TRUE);
	}

	if(pathCount==4)
	{
		/* We have exactly four elements indicating it should be a plugin
		 * setting. The first element is empty as checked before but we
		 * need to check that second one is the sub-path for plugins.
		 * If sub-path is correct, copy plugin ID and settings name,
		 * release allocated memory and return TRUE. If not correct,
		 * return FALSE.
		 */
		if(g_strcmp0(pathElements[1], XFDASHBOARD_XFCONF_SETTINGS_PLUGINS_PATH)!=0)
		{
			/* Is not sub-path for plugin settings, so release allocated
			 * memory and return FALSE.
			 */
			g_strfreev(pathElements);

			return(FALSE);
		}

		*outPluginID=g_strdup(pathElements[2]);
		*outSettingsName=g_strdup(pathElements[3]);

		g_strfreev(pathElements);

		return(TRUE);
	}

	/* Release allocated memory */
	g_strfreev(pathElements);

	/* If we get here, we could not retrieve plugin ID and settings name,
	 * so return FALSE.
	 */
	return(FALSE);
}

/* Set settings at core or plugin */
static gboolean _xfdashboard_xfconf_settings_set_settings_value(XfdashboardXfconfSettings *self,
																	const gchar *inPluginID,
																	const gchar *inSettingsName,
																	const GValue *inValue)
{
	GObject								*pluginSettings;
	GParamSpec							*paramSpec;
	GValue								value=G_VALUE_INIT;
	gboolean							success;

	g_return_val_if_fail(XFDASHBOARD_IS_XFCONF_SETTINGS(self), FALSE);
	g_return_val_if_fail(inPluginID==NULL || *inPluginID, FALSE);
	g_return_val_if_fail(inSettingsName && *inSettingsName, FALSE);
	g_return_val_if_fail(inValue, FALSE);

	pluginSettings=NULL;
	success=TRUE;

	/* If settings name of plugin ID is blocked, skip it but indicate success */
	if(_xfdashboard_xfconf_settings_block_property_notification_is_blocked(self, inPluginID, inSettingsName))
	{
		XFDASHBOARD_DEBUG(self, MISC,
							"Skipping blocked setting '%s' at %s%s",
							inSettingsName,
							inPluginID ? "plug-in settings for plug-in " : "core settings",
							inPluginID ? inPluginID : "");
		return(TRUE);
	}

	/* If plugin ID is provided, lookup settings object instance for
	 * plugin ID.
	 */
	if(inPluginID)
	{
		XfdashboardPlugin					*plugin;
		XfdashboardPluginSettings			*settings;

		/* Get plugin for plugin ID provided */
		plugin=xfdashboard_settings_lookup_plugin_by_id(XFDASHBOARD_SETTINGS(self), inPluginID);
		if(!plugin)
		{
			XFDASHBOARD_DEBUG(self, MISC,
								"Could not get settings object for unknown plug-in ID '%s'",
								inPluginID);
			return(FALSE);
		}

		/* If we did not find a settings object instance, we cannot
		 * modify any settings at it, so return FALSE.
		 */
		settings=xfdashboard_plugin_get_settings(plugin);
		if(!settings)
		{
			XFDASHBOARD_DEBUG(self, MISC,
								"No settings object instance for plug-in ID '%s'",
								inPluginID);
			return(FALSE);
		}

		pluginSettings=G_OBJECT(settings);
	}
		else pluginSettings=G_OBJECT(self);

	/* Check that settings name at settings object instance exists */
	paramSpec=g_object_class_find_property(G_OBJECT_GET_CLASS(pluginSettings), inSettingsName);
	if(!paramSpec)
	{
		XFDASHBOARD_DEBUG(self, MISC,
							"No setting '%s' found at %s%s",
							inSettingsName,
							inPluginID ? "plug-in settings for plug-in " : "core settings",
							inPluginID ? inPluginID : "");
		return(FALSE);
	}

	/* Special case is that property at settings object (destination) is a GStrv
	 * then the provided GValue from xfconf can be a GPtrArray containing GValues
	 * holding strings ... which must be converted in a special non-common way.
	 */
	if(G_PARAM_SPEC_VALUE_TYPE(paramSpec)==G_TYPE_STRV &&
		G_VALUE_TYPE(inValue)==G_TYPE_PTR_ARRAY)
	{
		GPtrArray						*sourceArray;
		guint							sourceArrayLength;
		gchar							**destStringList;
		guint							i;
		const GValue					*iterValue;

		/* Get GPtrArray from source value */
		sourceArray=(GPtrArray*)g_value_get_boxed(inValue);
		sourceArrayLength=sourceArray->len;

		/* Copy GPtrArray to GStrv */
		destStringList=g_new0(gchar *, sourceArrayLength+1);

		/* Iterate through source GPtrArray, retrieve GValues and
		 * store each string helt by GValue at GStrv.
		 */
		for(i=0; i<sourceArrayLength; i++)
		{
			/* Get GValue from iterated index of source GPtrArray */
			iterValue=(const GValue*)g_ptr_array_index(sourceArray, i);

			/* Check that GValue holds a string */
			if(!G_VALUE_HOLDS_STRING(iterValue))
			{
				/* Show error */
				g_warning("Cannot transform value for settings '%s' of %s%s from type '%s' to type '%s' because GValue at index #%u holds %s",
							inSettingsName,
							inPluginID ? "plug-in " : "core settings",
							inPluginID ? inPluginID : "",
							g_type_name(G_VALUE_TYPE(inValue)),
							g_type_name(G_TYPE_STRV),
							i,
							G_VALUE_TYPE_NAME(iterValue));

				/* Set failed state */
				success=FALSE;

				/* Stop further processing */
				break;
			}

			/* Store string of GValue at destination GStrv */
			destStringList[i]=g_value_dup_string(iterValue);
		}

		/* Set value */
		if(success)
		{
			g_value_init(&value, G_TYPE_STRV);
			g_value_set_boxed(&value, destStringList);
		}

		/* Release allocated memory */
		g_strfreev(destStringList);
	}

	/* When a xfconf property was resetted then this function may be called with
	 * a GValue of type G_TYPE_INVALID. This can neither be set nor converted
	 * (in next step if required) so we use the default value of parameter
	 * specification of (plugin) settings object.
	 */
	if(G_VALUE_TYPE(inValue)==G_TYPE_INVALID)
	{
		inValue=g_param_spec_get_default_value(paramSpec);
		XFDASHBOARD_DEBUG(self, MISC,
							"Using default value of type %s for settings '%s' of %s%s as the provided value was of invalid type",
							G_VALUE_TYPE_NAME(inValue),
							inSettingsName,
							inPluginID ? "plug-in settings for plug-in " : "core settings",
							inPluginID ? inPluginID : "");
	}

	/* Some parameters specification types of object properties are not supported
	 * by xfconf directly and needed to be converted. So convert back from xfconf
	 * type to object property type.
	 */
	if(success &&
		G_VALUE_TYPE(&value)==G_TYPE_INVALID)
	{
		g_value_init(&value, G_VALUE_TYPE(inValue));
		g_value_copy(inValue, &value);

		if(!g_type_is_a(G_VALUE_TYPE(inValue), paramSpec->value_type))
		{
			GValue							convertedValue=G_VALUE_INIT;

			g_value_init(&convertedValue, paramSpec->value_type);

#ifdef DEBUG
			XFDASHBOARD_DEBUG(self, MISC,
								"Need to convert xfconf value for setting '%s' of %s%s from type '%s' to type '%s'",
								inSettingsName,
								inPluginID ? "plug-in " : "core settings",
								inPluginID ? inPluginID : "",
								g_type_name(G_VALUE_TYPE(&value)),
								g_type_name(G_VALUE_TYPE(&convertedValue)));
#endif

			if(g_value_transform(&value, &convertedValue))
			{
				/* Transformation succeeded, so copy converted value to set it */
				g_value_unset(&value);
				g_value_init(&value, G_VALUE_TYPE(&convertedValue));
				g_value_copy(&convertedValue, &value);
			}
				/* Tranformation failed, so show error and set failed state */
				else
				{
					/* Show error */
					g_warning("Cannot transform value for settings '%s' of %s%s from type '%s' to type '%s'",
								inSettingsName,
								inPluginID ? "plug-in " : "core settings",
								inPluginID ? inPluginID : "",
								g_type_name(G_VALUE_TYPE(&value)),
								g_type_name(G_VALUE_TYPE(&convertedValue)));

					/* Set failed stat */
					success=FALSE;
				}

			/* Unset converted value */
			g_value_unset(&convertedValue);
		}
	}

	/* Set (converted) value if successful converted */
	if(success)
	{
		/* Block setting of plugin ID to prevent recursion by signal handling */ 
		_xfdashboard_xfconf_settings_block_property_notification_ref(self, inPluginID, inSettingsName);

		/* Set value at setting of plugin */
		g_object_set_property(pluginSettings, inSettingsName, &value);

		/* Unblock setting of plugin ID */ 
		_xfdashboard_xfconf_settings_block_property_notification_unref(self, inPluginID, inSettingsName);
	}

	/* Release allocated resources */
	g_value_unset(&value);

	/* If we get here, we set value at requested setting (at least we tried) */
	return(TRUE);
}

/* Signal handler for property changed at xfconf */
static void _xfdashboard_xfconf_settings_on_xfconf_property_changed(XfdashboardXfconfSettings *self,
																	const gchar *inProperty,
																	const GValue *inValue,
																	gpointer inUserData)
{
	gchar								*pluginID;
	gchar								*settingsName;

	g_return_if_fail(XFDASHBOARD_IS_XFCONF_SETTINGS(self));
	g_return_if_fail(inProperty && *inProperty);
	g_return_if_fail(inValue);

	pluginID=NULL;
	settingsName=NULL;

	/* Determine plug-in ID and settings name from xfconf property */
	if(_xfdashboard_xfconf_settings_find_plugin_and_setting(inProperty, &pluginID, &settingsName))
	{
		/* Set new value from xfconf property at plug-in ID and settings name */
		_xfdashboard_xfconf_settings_set_settings_value(self, pluginID, settingsName, inValue);

		/* Release allocated memory */
		g_free(pluginID);
		g_free(settingsName);
	}
		else
		{
			g_warning("Could not determine plug-in ID and settings name from Xfconf property '%s'",
						inProperty);
		}

}

/* Initialize initial settings from xfconf */
static void _xfdashboard_xfconf_settings_initialize_settings(XfdashboardXfconfSettings *self, const gchar *inPluginID)
{
	XfdashboardXfconfSettingsPrivate	*priv;
	GHashTable							*xfconfProperties;
	GHashTableIter						iter;
	const gchar							*iterKey;
	const GValue						*iterValue;

	g_return_if_fail(XFDASHBOARD_IS_XFCONF_SETTINGS(self));

	priv=self->priv;

	/* Get list of properties from xfconf but return from here and do not
	 * iterate through properties if we got no property at all.
	 */
	xfconfProperties=xfconf_channel_get_properties(priv->channel, NULL);
	if(!xfconfProperties) return;

	/* Iterate through retrieved properties and set settings but skip plugin settings */
	g_hash_table_iter_init(&iter, xfconfProperties);
	while(g_hash_table_iter_next(&iter, (gpointer*)&iterKey, (gpointer*)&iterValue))
	{
		gchar							*pluginID;
		gchar							*settingsName;

		pluginID=NULL;
		settingsName=NULL;

		/* Determine plug-in ID and settings name from xfconf property */
		if(_xfdashboard_xfconf_settings_find_plugin_and_setting(iterKey, &pluginID, &settingsName))
		{
			/* Plugin ID must be NULL to be a core setting and skip plugin settings */
			if((!inPluginID && !pluginID) ||
				g_strcmp0(pluginID, inPluginID)==0)
			{
				_xfdashboard_xfconf_settings_set_settings_value(self, pluginID, settingsName, iterValue);
			}

			/* Release allocated memory */
			g_free(pluginID);
			g_free(settingsName);
		}
			else
			{
				g_warning("Could not determine plug-in ID and settings name from Xfconf property '%s' for initialization",
							iterKey);
			}
	}
	g_hash_table_destroy(xfconfProperties);
}

/* A plugin was added to settings */
static void _xfdashboard_xfconf_settings_plugin_added(XfdashboardSettings *inSettings, XfdashboardPlugin *inPlugin)
{
	g_return_if_fail(XFDASHBOARD_IS_XFCONF_SETTINGS(inSettings));
	g_return_if_fail(XFDASHBOARD_IS_PLUGIN(inPlugin));

	_xfdashboard_xfconf_settings_initialize_settings(XFDASHBOARD_XFCONF_SETTINGS(inSettings), xfdashboard_plugin_get_id(inPlugin));
}


/* IMPLEMENTATION: GObject */

/* Dispose this object */
static void _xfdashboard_xfconf_settings_dispose(GObject *inObject)
{
	XfdashboardXfconfSettings			*self=XFDASHBOARD_XFCONF_SETTINGS(inObject);
	XfdashboardXfconfSettingsPrivate	*priv=self->priv;

	/* Release allocated resources */
	if(priv->channel)
	{
		if(priv->xfconfNotifySignalID)
		{
			g_signal_handler_disconnect(priv->channel, priv->xfconfNotifySignalID);
			priv->xfconfNotifySignalID=0;
		}

		priv->channel=NULL;
	}

	if(priv->blockedSettings)
	{
		g_list_free_full(priv->blockedSettings, (GDestroyNotify)_xfdashboard_xfconf_settings_block_property_notification_free);
		priv->blockedSettings=NULL;
	}

	/* Call parent's class dispose method */
	G_OBJECT_CLASS(xfdashboard_xfconf_settings_parent_class)->dispose(inObject);
}

/* Class initialization
 * Override functions in parent classes and define properties
 * and signals
 */
static void xfdashboard_xfconf_settings_class_init(XfdashboardXfconfSettingsClass *klass)
{
	GObjectClass				*gobjectClass=G_OBJECT_CLASS(klass);
	XfdashboardSettingsClass	*settingsClass=XFDASHBOARD_SETTINGS_CLASS(klass);

	/* Override functions */
	gobjectClass->dispose=_xfdashboard_xfconf_settings_dispose;

	settingsClass->changed=_xfdashboard_xfconf_settings_on_settings_changed;
	settingsClass->plugin_added=_xfdashboard_xfconf_settings_plugin_added;
}

/* Object initialization
 * Create private structure and set up default values
 */
static void xfdashboard_xfconf_settings_init(XfdashboardXfconfSettings *self)
{
	XfdashboardXfconfSettingsPrivate	*priv;

	priv=self->priv=xfdashboard_xfconf_settings_get_instance_private(self);

	/* Set default values */
	priv->channel=xfconf_channel_get(XFDASHBOARD_XFCONF_CHANNEL);
	priv->xfconfNotifySignalID=0;
	priv->blockedSettings=NULL;

	/* Connect to "property-changed" signal of xfconf */
	priv->xfconfNotifySignalID=
		g_signal_connect_swapped(priv->channel,
									"property-changed",
									G_CALLBACK(_xfdashboard_xfconf_settings_on_xfconf_property_changed),
									self);

	/* Initialize initial core settings from xfconf */
	_xfdashboard_xfconf_settings_initialize_settings(self, NULL);
}


/* IMPLEMENTATION: Public API */

