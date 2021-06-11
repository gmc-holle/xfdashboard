/*
 * theme: Top-level theme object (parses key file and manages loading
 *        resources like css style files, xml layout files etc.)
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
 * SECTION:theme
 * @short_description: A theme
 * @include: libxfdashboard/theme.h
 *
 * #XfdashboardTheme is used to load a named theme searched at the theme
 * search path as returned by xfdashboard_settings_get_theme_search_paths()
 * and loads the key file, parses it to retrieve the file locations of all
 * resources to load for the theme like CSS, animations, effects, layout,
 * etc.
 *
 * # File location and structure {#XfdashboardTheme.File-location-and-structure}
 *
 * `xfdashboard` will look up the theme at the paths in the order as returned
 * by xfdashboard_settings_get_theme_search_paths(). In case of the application
 * `xfdashboard` this is the following order:
 *
 *   * (if environment variable is set) ${XFDASHBOARD_THEME_PATH}/
 *   * ${XDG_DATA_HOME}/themes/THEME/xfdashboard-1.0/
 *   * ${HOME}/.themes/THEME/xfdashboard-1.0/
 *   * ${SYSTEM-WIDE-DATA}/themes/THEME/xfdashboard-1.0/
 *
 * `xfdashboard` expects at least the theme index file `xfdashboard.theme` at
 * the theme's path which contains information about the theme and the resources
 * to load for styling and layout. The content of the theme file should be
 * similar to the following:
 *
 * |[
 *   [Xfdashboard Theme]
 *   Name=<Display name of theme>
 *   Comment=<A description for the theme>
 *   Author=<A list of authors of the theme seperated by semicolon>
 *   Version=<The version of the theme>
 *   Style=<A list of CSS files for styling separated by semicolon>
 *   Layout=<A list of XML files for layout separated by semicolon>
 *   Effects=<A list of XML files for effects separated by semicolon>
 *   Animation=<A lilst of of XML files for animations seperated by semicolon>
 *   Screenshot=<A list of paths to screenshot images separated by semicolon>
 * ]|
 *
 * The following keys are required: `Name`, `Comment`, `Style` and `Layout`. All
 * other keys are optional.
 *
 * The key `Style` contains the file name of one CSS file or a semicolon-separated
 * list of CSS files (without spaces in the list) which describe how actors should
 * be styled. The files should be located in the theme path or in a sub-folder.
 * See section <link linkend="XfdashboardThemeCSS.Styling">Styling</link> for
 * further documentation about styling. <!-- EXTERNAL: There is also a full list
 * about all [#Elements and stylables properties], [#classes] and [#pseudo-classes].
 * -->
 *
 * The key `Layout` contains the file name of one XML file or a semicolon-separated
 * list of XML files (without spaces in the list) which describes how the different
 * stages are to be built. The files should be located in the theme path or in a
 * sub-folder. See section <link linkend="XfdashboardThemeLayout.Layout">Layout</link>
 * for further documentation about layouting.
 *
 * The key `Effects` contains the file name of one XML file or a semicolon-separated
 * list of XML files (without spaces in the list) which describes effects which can
 * be used in actors and how they are built. The files should be located in the
 * theme path or in a sub-folder. See section<link linkend="XfdashboardThemeEffects.Effects">
 * Effects</link> for further documentation about creating and using effects.
 *
 * The key `Animation` contains the file name of one XML file or a semicolon-separated
 * list of XML files (without spaces in the list) which describes animations which can
 * be used in actors and how they are built. The files should be located in the
 * theme path or in a sub-folder. See section <link linkend="XfdashboardThemeAnimation.Animations">
 * Animations</link> for further documentation about creating and using animations.
 *
 * The key `Screenshot` contains the file name of one image or a semicolon-separated
 * list of images (without spaces in the list) which can be used as preview images
 * for the theme, e.g. in settings application. The files should be located in the
 * theme path or in a sub-folder and given as relative paths.
 *
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <libxfdashboard/theme.h>

#include <glib/gi18n-lib.h>
#include <glib.h>
#include <gio/gio.h>

#include <libxfdashboard/core.h>
#include <libxfdashboard/settings.h>
#include <libxfdashboard/compat.h>
#include <libxfdashboard/debug.h>


/* Define this class in GObject system */
struct _XfdashboardThemePrivate
{
	/* Properties related */
	gchar						*themePath;

	gchar						*themeName;
	gchar						*themeDisplayName;
	gchar						*themeComment;

	/* Instance related */
	gboolean					loaded;

	XfdashboardThemeCSS			*styling;
	XfdashboardThemeLayout		*layout;
	XfdashboardThemeEffects		*effects;
	XfdashboardThemeAnimation	*animation;

	gchar						*userThemeStyleFile;
	gchar						*userGlobalStyleFile;
};

G_DEFINE_TYPE_WITH_PRIVATE(XfdashboardTheme,
							xfdashboard_theme,
							G_TYPE_OBJECT)

/* Properties */
enum
{
	PROP_0,

	PROP_PATH,

	PROP_NAME,
	PROP_DISPLAY_NAME,
	PROP_COMMENT,

	PROP_LAST
};

static GParamSpec* XfdashboardThemeProperties[PROP_LAST]={ 0, };

/* IMPLEMENTATION: Private variables and methods */
#define XFDASHBOARD_THEME_SUBPATH						"xfdashboard-1.0"
#define XFDASHBOARD_THEME_FILE							"xfdashboard.theme"
#define XFDASHBOARD_USER_GLOBAL_CSS_FILE				"global.css"

#define XFDASHBOARD_THEME_GROUP							"Xfdashboard Theme"
#define XFDASHBOARD_THEME_GROUP_KEY_NAME				"Name"
#define XFDASHBOARD_THEME_GROUP_KEY_COMMENT				"Comment"
#define XFDASHBOARD_THEME_GROUP_KEY_STYLE				"Style"
#define XFDASHBOARD_THEME_GROUP_KEY_LAYOUT				"Layout"
#define XFDASHBOARD_THEME_GROUP_KEY_EFFECTS				"Effects"
#define XFDASHBOARD_THEME_GROUP_KEY_ANIMATIONS			"Animations"


/* Load theme file and all listed resources in this file */
static gboolean _xfdashboard_theme_load_resources(XfdashboardTheme *self,
													GError **outError)
{
	XfdashboardThemePrivate		*priv;
	GError						*error;
	gchar						*themeFile;
	GKeyFile					*themeKeyFile;
	gchar						**resources, **resource;
	gchar						*resourceFile;
	gint						counter;

	g_return_val_if_fail(XFDASHBOARD_IS_THEME(self), FALSE);
	g_return_val_if_fail(outError==NULL || *outError==NULL, FALSE);

	priv=self->priv;
	error=NULL;

	/* Check that theme was found */
	if(!priv->themePath)
	{
		/* Set error */
		g_set_error(outError,
					XFDASHBOARD_THEME_ERROR,
					XFDASHBOARD_THEME_ERROR_THEME_NOT_FOUND,
					"Theme '%s' not found",
					priv->themeName);

		/* Return FALSE to indicate error */
		return(FALSE);
	}

	/* Load theme file */
	themeFile=g_build_filename(priv->themePath, XFDASHBOARD_THEME_FILE, NULL);

	themeKeyFile=g_key_file_new();
	if(!g_key_file_load_from_file(themeKeyFile,
									themeFile,
									G_KEY_FILE_NONE,
									&error))
	{
		/* Set error */
		g_propagate_error(outError, error);

		/* Release allocated resources */
		if(themeFile) g_free(themeFile);
		if(themeKeyFile) g_key_file_free(themeKeyFile);

		/* Return FALSE to indicate error */
		return(FALSE);
	}

	g_free(themeFile);

	/* Get display name and notify about property change (regardless of success result) */
	priv->themeDisplayName=g_key_file_get_locale_string(themeKeyFile,
														XFDASHBOARD_THEME_GROUP,
														XFDASHBOARD_THEME_GROUP_KEY_NAME,
														NULL,
														&error);
	g_object_notify_by_pspec(G_OBJECT(self), XfdashboardThemeProperties[PROP_DISPLAY_NAME]);

	if(!priv->themeDisplayName)
	{
		/* Set error */
		g_propagate_error(outError, error);

		/* Release allocated resources */
		if(themeKeyFile) g_key_file_free(themeKeyFile);

		/* Return FALSE to indicate error */
		return(FALSE);
	}

	/* Get comment and notify about property change (regardless of success result) */
	priv->themeComment=g_key_file_get_locale_string(themeKeyFile,
														XFDASHBOARD_THEME_GROUP,
														XFDASHBOARD_THEME_GROUP_KEY_COMMENT,
														NULL,
														&error);
	g_object_notify_by_pspec(G_OBJECT(self), XfdashboardThemeProperties[PROP_COMMENT]);

	if(!priv->themeComment)
	{
		/* Set error */
		g_propagate_error(outError, error);

		/* Release allocated resources */
		if(themeKeyFile) g_key_file_free(themeKeyFile);

		/* Return FALSE to indicate error */
		return(FALSE);
	}

	/* Create CSS parser, load style resources first and user stylesheets (theme
	 * unrelated "global.css" and theme related "user-[THEME_NAME].css" in this
	 * order) at last to allow user to override theme styles.
	 */
	resources=g_key_file_get_string_list(themeKeyFile,
											XFDASHBOARD_THEME_GROUP,
											XFDASHBOARD_THEME_GROUP_KEY_STYLE,
											NULL,
											&error);
	if(!resources)
	{
		/* Set error */
		g_propagate_error(outError, error);

		/* Release allocated resources */
		if(themeKeyFile) g_key_file_free(themeKeyFile);

		/* Return FALSE to indicate error */
		return(FALSE);
	}

	counter=0;
	resource=resources;
	while(*resource)
	{
		/* Get path and file for style resource */
		resourceFile=g_build_filename(priv->themePath, *resource, NULL);

		/* Try to load style resource */
		XFDASHBOARD_DEBUG(self, THEME,
							"Loading CSS file %s for theme %s with priority %d",
							resourceFile,
							priv->themeName,
							counter);

		if(!xfdashboard_theme_css_add_file(priv->styling, resourceFile, counter, &error))
		{
			/* Set error */
			g_propagate_error(outError, error);

			/* Release allocated resources */
			if(resources) g_strfreev(resources);
			if(resourceFile) g_free(resourceFile);
			if(themeKeyFile) g_key_file_free(themeKeyFile);

			/* Return FALSE to indicate error */
			return(FALSE);
		}

		/* Release allocated resources */
		if(resourceFile) g_free(resourceFile);

		/* Continue with next entry */
		resource++;
		counter++;
	}
	g_strfreev(resources);

	if(priv->userGlobalStyleFile)
	{
		XFDASHBOARD_DEBUG(self, THEME,
							"Loading user's global CSS file %s for theme %s with priority %d",
							priv->userGlobalStyleFile,
							priv->themeName,
							counter);

		/* Load user's theme unrelated (global) stylesheet as it exists */
		if(!xfdashboard_theme_css_add_file(priv->styling, priv->userGlobalStyleFile, counter, &error))
		{
			/* Set error */
			g_propagate_error(outError, error);

			/* Release allocated resources */
			if(themeKeyFile) g_key_file_free(themeKeyFile);

			/* Return FALSE to indicate error */
			return(FALSE);
		}

		/* Increase counter used for CSS priority for next user CSS file */
		counter++;
	}

	if(priv->userThemeStyleFile)
	{
		XFDASHBOARD_DEBUG(self, THEME,
							"Loading user's theme CSS file %s for theme %s with priority %d",
							priv->userThemeStyleFile,
							priv->themeName,
							counter);

		/* Load user's theme related stylesheet as it exists */
		if(!xfdashboard_theme_css_add_file(priv->styling, priv->userThemeStyleFile, counter, &error))
		{
			/* Set error */
			g_propagate_error(outError, error);

			/* Release allocated resources */
			if(themeKeyFile) g_key_file_free(themeKeyFile);

			/* Return FALSE to indicate error */
			return(FALSE);
		}

		/* Increase counter used for CSS priority for next user CSS file */
		counter++;
	}

	/* Create XML parser and load layout resources */
	resources=g_key_file_get_string_list(themeKeyFile,
											XFDASHBOARD_THEME_GROUP,
											XFDASHBOARD_THEME_GROUP_KEY_LAYOUT,
											NULL,
											&error);
	if(!resources)
	{
		/* Set error */
		g_propagate_error(outError, error);

		/* Release allocated resources */
		if(themeKeyFile) g_key_file_free(themeKeyFile);

		/* Return FALSE to indicate error */
		return(FALSE);
	}

	resource=resources;
	while(*resource)
	{
		/* Get path and file for style resource */
		resourceFile=g_build_filename(priv->themePath, *resource, NULL);

		/* Try to load layout resource */
		XFDASHBOARD_DEBUG(self, THEME,
							"Loading XML layout file %s for theme %s",
							resourceFile,
							priv->themeName);

		if(!xfdashboard_theme_layout_add_file(priv->layout, resourceFile, &error))
		{
			/* Set error */
			g_propagate_error(outError, error);

			/* Release allocated resources */
			if(resources) g_strfreev(resources);
			if(resourceFile) g_free(resourceFile);
			if(themeKeyFile) g_key_file_free(themeKeyFile);

			/* Return FALSE to indicate error */
			return(FALSE);
		}

		/* Release allocated resources */
		if(resourceFile) g_free(resourceFile);

		/* Continue with next entry */
		resource++;
		counter++;
	}
	g_strfreev(resources);

	/* Create XML parser and load effect resources which are optional */
	if(g_key_file_has_key(themeKeyFile,
							XFDASHBOARD_THEME_GROUP,
							XFDASHBOARD_THEME_GROUP_KEY_EFFECTS,
							NULL))
	{
		resources=g_key_file_get_string_list(themeKeyFile,
												XFDASHBOARD_THEME_GROUP,
												XFDASHBOARD_THEME_GROUP_KEY_EFFECTS,
												NULL,
												&error);
		if(!resources)
		{
			/* Set error */
			g_propagate_error(outError, error);

			/* Release allocated resources */
			if(themeKeyFile) g_key_file_free(themeKeyFile);

			/* Return FALSE to indicate error */
			return(FALSE);
		}

		resource=resources;
		while(*resource)
		{
			/* Get path and file for effect resource */
			resourceFile=g_build_filename(priv->themePath, *resource, NULL);

			/* Try to load effects resource */
			XFDASHBOARD_DEBUG(self, THEME,
								"Loading XML effects file %s for theme %s",
								resourceFile,
								priv->themeName);

			if(!xfdashboard_theme_effects_add_file(priv->effects, resourceFile, &error))
			{
				/* Set error */
				g_propagate_error(outError, error);

				/* Release allocated resources */
				if(resources) g_strfreev(resources);
				if(resourceFile) g_free(resourceFile);
				if(themeKeyFile) g_key_file_free(themeKeyFile);

				/* Return FALSE to indicate error */
				return(FALSE);
			}

			/* Release allocated resources */
			if(resourceFile) g_free(resourceFile);

			/* Continue with next entry */
			resource++;
			counter++;
		}
		g_strfreev(resources);
	}

	/* Create XML parser and load animation resources which are optional */
	if(g_key_file_has_key(themeKeyFile,
							XFDASHBOARD_THEME_GROUP,
							XFDASHBOARD_THEME_GROUP_KEY_ANIMATIONS,
							NULL))
	{
		resources=g_key_file_get_string_list(themeKeyFile,
												XFDASHBOARD_THEME_GROUP,
												XFDASHBOARD_THEME_GROUP_KEY_ANIMATIONS,
												NULL,
												&error);
		if(!resources)
		{
			/* Set error */
			g_propagate_error(outError, error);

			/* Release allocated resources */
			if(themeKeyFile) g_key_file_free(themeKeyFile);

			/* Return FALSE to indicate error */
			return(FALSE);
		}

		resource=resources;
		while(*resource)
		{
			/* Get path and file for animation resource */
			resourceFile=g_build_filename(priv->themePath, *resource, NULL);

			/* Try to load animation resource */
			XFDASHBOARD_DEBUG(self, THEME,
								"Loading XML animation file %s for theme %s",
								resourceFile,
								priv->themeName);

			if(!xfdashboard_theme_animation_add_file(priv->animation, resourceFile, &error))
			{
				/* Set error */
				g_propagate_error(outError, error);

				/* Release allocated resources */
				if(resources) g_strfreev(resources);
				if(resourceFile) g_free(resourceFile);
				if(themeKeyFile) g_key_file_free(themeKeyFile);

				/* Return FALSE to indicate error */
				return(FALSE);
			}

			/* Release allocated resources */
			if(resourceFile) g_free(resourceFile);

			/* Continue with next entry */
			resource++;
			counter++;
		}
		g_strfreev(resources);
	}

	/* Release allocated resources */
	if(themeKeyFile) g_key_file_free(themeKeyFile);

	/* Return TRUE to indicate success */
	return(TRUE);
}

/* Lookup path for named theme.
 * Caller must free returned path with g_free if not needed anymore.
 */
static gchar* _xfdashboard_theme_lookup_path_for_theme(XfdashboardTheme *self,
														const gchar *inThemeName)
{
	XfdashboardSettings		*settings;
	const gchar				**searchPaths;
	gchar					*themeFile;

	g_return_val_if_fail(XFDASHBOARD_IS_THEME(self), FALSE);
	g_return_val_if_fail(inThemeName!=NULL && *inThemeName!=0, FALSE);

	themeFile=NULL;

	/* Get search path for themes */
	settings=xfdashboard_core_get_settings(NULL);
	searchPaths=xfdashboard_settings_get_theme_search_paths(settings);

	/* Iterate through search paths and look up theme */
	while(searchPaths && *searchPaths && !themeFile)
	{
		/* Restore old behaviour to force a theme path via an environment
		 * variable but this time with theme search paths. The behaviour
		 * was to take the provided path and add the theme file without
		 * any additional subpath like theme name or theme subpath folder.
		 * If this built path provides the theme file use it directly
		 * although it might not match the theme name (that is for sure
		 * as the theme name will not be added to the path built). This
		 * behaviour makes development easier to test themes without
		 * changing theme by settings or changing symlinks in any of
		 * theme searched paths.
		 */
		themeFile=g_build_filename(*searchPaths, XFDASHBOARD_THEME_FILE, NULL);
		XFDASHBOARD_DEBUG(self, THEME,
							"Trying theme file: %s",
							themeFile);
		if(!g_file_test(themeFile, G_FILE_TEST_EXISTS | G_FILE_TEST_IS_REGULAR))
		{
			/* File does not exists so release allocated resources */
			g_free(themeFile);
			themeFile=NULL;

			/* Next build theme file path depending on theme name and
			 * required sub-folders and test for existence.
			 * If it does exist, keep theme file path. If it does not exist,
			 * unset theme file path and continue iterating.
			 */
			themeFile=g_build_filename(*searchPaths, inThemeName, XFDASHBOARD_THEME_SUBPATH, XFDASHBOARD_THEME_FILE, NULL);
			XFDASHBOARD_DEBUG(self, THEME,
								"Trying theme file: %s",
								themeFile);
			if(!g_file_test(themeFile, G_FILE_TEST_EXISTS | G_FILE_TEST_IS_REGULAR))
			{
				g_free(themeFile);
				themeFile=NULL;
			}
		}

		/* Move iterator to next entry */
		searchPaths++;
	}
	
	/* If file was found, get path contaning file and return it */
	if(themeFile)
	{
		gchar			*themePath;

		themePath=g_path_get_dirname(themeFile);
		g_free(themeFile);

		return(themePath);
	}

	/* If we get here theme was not found so return NULL */
	return(NULL);
}

/* Theme's name was set so lookup pathes and initialize but do not load resources */
static void _xfdashboard_theme_set_theme_name(XfdashboardTheme *self, const gchar *inThemeName)
{
	XfdashboardThemePrivate		*priv;
	gchar						*themePath;
	gchar						*resourceFile;
	gchar						*userThemeStylesheet;
	XfdashboardSettings			*settings;
	const gchar					*configPath;

	g_return_if_fail(XFDASHBOARD_IS_THEME(self));
	g_return_if_fail(inThemeName && *inThemeName);

	priv=self->priv;

	/* The theme name must not be set already */
	if(priv->themeName)
	{
		/* Show error message */
		g_critical("Cannot change theme name to '%s' because it is already set to '%s'",
					inThemeName,
					priv->themeName);

		return;
	}

	/* Lookup path of theme by lookup at all possible paths for theme file */
	themePath=_xfdashboard_theme_lookup_path_for_theme(self, inThemeName);
	if(!themePath)
	{
		g_critical("Theme '%s' not found", inThemeName);

		/* Return here because looking up path failed */
		return;
	}

	/* Set up internal variable and notify about property changes */
	priv->themeName=g_strdup(inThemeName);
	g_object_notify_by_pspec(G_OBJECT(self), XfdashboardThemeProperties[PROP_NAME]);

	priv->themePath=g_strdup(themePath);
	g_object_notify_by_pspec(G_OBJECT(self), XfdashboardThemeProperties[PROP_PATH]);

	/* Initialize theme resources */
	priv->styling=xfdashboard_theme_css_new(priv->themePath);
	priv->layout=xfdashboard_theme_layout_new();
	priv->effects=xfdashboard_theme_effects_new();
	priv->animation=xfdashboard_theme_animation_new();

	/* Check for user resource files */
	settings=xfdashboard_core_get_settings(NULL);
	configPath=xfdashboard_settings_get_config_path(settings);
	if(configPath)
	{
		resourceFile=g_build_filename(configPath, "themes", XFDASHBOARD_USER_GLOBAL_CSS_FILE, NULL);
		if(g_file_test(resourceFile, G_FILE_TEST_EXISTS | G_FILE_TEST_IS_REGULAR))
		{
			priv->userGlobalStyleFile=g_strdup(resourceFile);
		}
			else
			{
				XFDASHBOARD_DEBUG(self, THEME,
									"No user global stylesheet found at %s for theme %s - skipping",
									resourceFile,
									priv->themeName);
			}
		g_free(resourceFile);

		userThemeStylesheet=g_strdup_printf("user-%s.css", priv->themeName);
		resourceFile=g_build_filename(configPath, "themes", userThemeStylesheet, NULL);
		if(g_file_test(resourceFile, G_FILE_TEST_EXISTS | G_FILE_TEST_IS_REGULAR))
		{
			priv->userThemeStyleFile=g_strdup(resourceFile);
		}
			else
			{
				XFDASHBOARD_DEBUG(self, THEME,
									"No user theme stylesheet found at %s for theme %s - skipping",
									resourceFile,
									priv->themeName);
			}
		g_free(resourceFile);
	}

	/* Release allocated resources */
	if(themePath) g_free(themePath);
}

/* IMPLEMENTATION: GObject */

/* Dispose this object */
static void _xfdashboard_theme_dispose(GObject *inObject)
{
	XfdashboardTheme			*self=XFDASHBOARD_THEME(inObject);
	XfdashboardThemePrivate		*priv=self->priv;

	/* Release allocated resources */
	if(priv->themeName)
	{
		g_free(priv->themeName);
		priv->themeName=NULL;
		g_object_notify_by_pspec(G_OBJECT(self), XfdashboardThemeProperties[PROP_NAME]);
	}

	if(priv->themePath)
	{
		g_free(priv->themePath);
		priv->themePath=NULL;
		g_object_notify_by_pspec(G_OBJECT(self), XfdashboardThemeProperties[PROP_PATH]);
	}

	if(priv->themeDisplayName)
	{
		g_free(priv->themeDisplayName);
		priv->themeDisplayName=NULL;
		g_object_notify_by_pspec(G_OBJECT(self), XfdashboardThemeProperties[PROP_DISPLAY_NAME]);
	}

	if(priv->themeComment)
	{
		g_free(priv->themeComment);
		priv->themeComment=NULL;
		g_object_notify_by_pspec(G_OBJECT(self), XfdashboardThemeProperties[PROP_COMMENT]);
	}

	if(priv->userThemeStyleFile)
	{
		g_free(priv->userThemeStyleFile);
		priv->userThemeStyleFile=NULL;
	}

	if(priv->userGlobalStyleFile)
	{
		g_free(priv->userGlobalStyleFile);
		priv->userGlobalStyleFile=NULL;
	}

	if(priv->styling)
	{
		g_object_unref(priv->styling);
		priv->styling=NULL;
	}

	if(priv->layout)
	{
		g_object_unref(priv->layout);
		priv->layout=NULL;
	}

	if(priv->effects)
	{
		g_object_unref(priv->effects);
		priv->effects=NULL;
	}

	if(priv->animation)
	{
		g_object_unref(priv->animation);
		priv->animation=NULL;
	}

	/* Call parent's class dispose method */
	G_OBJECT_CLASS(xfdashboard_theme_parent_class)->dispose(inObject);
}

/* Set/get properties */
static void _xfdashboard_theme_set_property(GObject *inObject,
											guint inPropID,
											const GValue *inValue,
											GParamSpec *inSpec)
{
	XfdashboardTheme			*self=XFDASHBOARD_THEME(inObject);

	switch(inPropID)
	{
		case PROP_NAME:
			_xfdashboard_theme_set_theme_name(self, g_value_get_string(inValue));
			break;

		default:
			G_OBJECT_WARN_INVALID_PROPERTY_ID(inObject, inPropID, inSpec);
			break;
	}
}

static void _xfdashboard_theme_get_property(GObject *inObject,
											guint inPropID,
											GValue *outValue,
											GParamSpec *inSpec)
{
	XfdashboardTheme			*self=XFDASHBOARD_THEME(inObject);
	XfdashboardThemePrivate		*priv=self->priv;

	switch(inPropID)
	{
		case PROP_PATH:
			g_value_set_string(outValue, priv->themePath);
			break;

		case PROP_NAME:
			g_value_set_string(outValue, priv->themeName);
			break;

		case PROP_DISPLAY_NAME:
			g_value_set_string(outValue, priv->themeDisplayName);
			break;

		case PROP_COMMENT:
			g_value_set_string(outValue, priv->themeComment);
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
void xfdashboard_theme_class_init(XfdashboardThemeClass *klass)
{
	GObjectClass		*gobjectClass=G_OBJECT_CLASS(klass);

	/* Override functions */
	gobjectClass->dispose=_xfdashboard_theme_dispose;
	gobjectClass->set_property=_xfdashboard_theme_set_property;
	gobjectClass->get_property=_xfdashboard_theme_get_property;

	/* Define properties */
	XfdashboardThemeProperties[PROP_NAME]=
		g_param_spec_string("theme-name",
							"Theme name",
							"Short name of theme which was used to lookup theme and folder name where theme is stored in",
							NULL,
							G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS | G_PARAM_CONSTRUCT_ONLY);

	XfdashboardThemeProperties[PROP_PATH]=
		g_param_spec_string("theme-path",
							"Theme path",
							"Path where theme was found and loaded from",
							NULL,
							G_PARAM_READABLE | G_PARAM_STATIC_STRINGS);

	XfdashboardThemeProperties[PROP_DISPLAY_NAME]=
		g_param_spec_string("theme-display-name",
							"Theme display name",
							"The name of theme",
							NULL,
							G_PARAM_READABLE | G_PARAM_STATIC_STRINGS);

	XfdashboardThemeProperties[PROP_COMMENT]=
		g_param_spec_string("theme-comment",
							"Theme comment",
							"The comment of theme used as description",
							NULL,
							G_PARAM_READABLE | G_PARAM_STATIC_STRINGS);

	g_object_class_install_properties(gobjectClass, PROP_LAST, XfdashboardThemeProperties);
}

/* Object initialization
 * Create private structure and set up default values
 */
void xfdashboard_theme_init(XfdashboardTheme *self)
{
	XfdashboardThemePrivate		*priv;

	priv=self->priv=xfdashboard_theme_get_instance_private(self);

	/* Set default values */
	priv->loaded=FALSE;
	priv->themeName=NULL;
	priv->themePath=NULL;
	priv->themeDisplayName=NULL;
	priv->themeComment=NULL;
	priv->userThemeStyleFile=NULL;
	priv->userGlobalStyleFile=NULL;
	priv->styling=NULL;
	priv->layout=NULL;
	priv->effects=NULL;
	priv->animation=NULL;
}

/* IMPLEMENTATION: Errors */

G_DEFINE_QUARK(xfdashboard-theme-error-quark, xfdashboard_theme_error);


/* IMPLEMENTATION: Public API */

/**
 * xfdashboard_theme_new:
 * @inThemeName: The name of theme
 *
 * Creates a new #XfdashboardTheme object and initializes the object instance.
 * It will not load any resources of the theme. It is neccessary to call
 * xfdashboard_theme_load() to load its resources.
 *
 * Return value: An initialized #XfdashboardTheme
 */
XfdashboardTheme* xfdashboard_theme_new(const gchar *inThemeName)
{
	return(XFDASHBOARD_THEME(g_object_new(XFDASHBOARD_TYPE_THEME,
											"theme-name", inThemeName,
											NULL)));
}

/**
 * xfdashboard_theme_get_path:
 * @self: A #XfdashboardTheme
 *
 * Returns the base path where the theme at @self was found and will load all
 * its resources from.
 *
 * Return value: The theme's base path
 */
const gchar* xfdashboard_theme_get_path(XfdashboardTheme *self)
{
	g_return_val_if_fail(XFDASHBOARD_IS_THEME(self), NULL);

	return(self->priv->themePath);
}

/**
 * xfdashboard_theme_get_theme_name:
 * @self: A #XfdashboardTheme
 *
 * Returns the name of theme at @self.
 *
 * Return value: The theme's name
 */
const gchar* xfdashboard_theme_get_theme_name(XfdashboardTheme *self)
{
	g_return_val_if_fail(XFDASHBOARD_IS_THEME(self), NULL);

	return(self->priv->themeName);
}

/**
 * xfdashboard_theme_get_display_name:
 * @self: A #XfdashboardTheme
 *
 * Returns the display name of theme at @self.
 *
 * Return value: The theme's name to display
 */
const gchar* xfdashboard_theme_get_display_name(XfdashboardTheme *self)
{
	g_return_val_if_fail(XFDASHBOARD_IS_THEME(self), NULL);

	return(self->priv->themeDisplayName);
}

/**
 * xfdashboard_theme_get_comment:
 * @self: A #XfdashboardTheme
 *
 * Returns the comment of theme at @self.
 *
 * Return value: The theme's comment
 */
const gchar* xfdashboard_theme_get_comment(XfdashboardTheme *self)
{
	g_return_val_if_fail(XFDASHBOARD_IS_THEME(self), NULL);

	return(self->priv->themeComment);
}

/**
 * xfdashboard_theme_load:
 * @self: A #XfdashboardTheme
 * @outError: A return location for a #GError or %NULL
 *
 * Lookups named theme at @self and loads all its resources like CSS, layout,
 * animation etc.
 *
 * If loading theme and its resources fails, the error message will be placed
 * inside error at @outError (if not %NULL).
 *
 * Return value: %TRUE if instance of #XfdashboardTheme could be loaded fully
 *   or %FALSE if not and error is stored at @outError.
 */
gboolean xfdashboard_theme_load(XfdashboardTheme *self,
								GError **outError)
{
	XfdashboardThemePrivate		*priv;
	GError						*error;

	g_return_val_if_fail(XFDASHBOARD_IS_THEME(self), FALSE);
	g_return_val_if_fail(outError==NULL || *outError==NULL, FALSE);

	priv=self->priv;
	error=NULL;

	/* Check if a theme was already loaded */
	if(priv->loaded)
	{
		g_set_error(outError,
					XFDASHBOARD_THEME_ERROR,
					XFDASHBOARD_THEME_ERROR_ALREADY_LOADED,
					"Theme '%s' was already loaded",
					priv->themeName);

		return(FALSE);
	}

	/* We set the loaded flag regardless if loading will be successful or not
	 * because if loading theme fails this object is in undefined state for
	 * re-using it to load theme again.
	 */
	priv->loaded=TRUE;

	/* Load theme key file */
	if(!_xfdashboard_theme_load_resources(self, &error))
	{
		/* Set returned error */
		g_propagate_error(outError, error);

		/* Return FALSE to indicate error */
		return(FALSE);
	}

	/* If we found named themed and could load all resources successfully */
	return(TRUE);
}

/**
 * xfdashboard_theme_get_css:
 * @self: A #XfdashboardTheme
 *
 * Returns the CSS resources of type #XfdashboardThemeCSS of theme at @self.
 *
 * Return value: The theme's CSS resources of type #XfdashboardThemeCSS
 */
XfdashboardThemeCSS* xfdashboard_theme_get_css(XfdashboardTheme *self)
{
	g_return_val_if_fail(XFDASHBOARD_IS_THEME(self), NULL);

	return(self->priv->styling);
}

/**
 * xfdashboard_theme_get_layout:
 * @self: A #XfdashboardTheme
 *
 * Returns the layout resources of type #XfdashboardThemeLayout of theme at @self.
 *
 * Return value: The theme's layout resources of type #XfdashboardThemeLayout
 */
XfdashboardThemeLayout* xfdashboard_theme_get_layout(XfdashboardTheme *self)
{
	g_return_val_if_fail(XFDASHBOARD_IS_THEME(self), NULL);

	return(self->priv->layout);
}

/**
 * xfdashboard_theme_get_effects:
 * @self: A #XfdashboardTheme
 *
 * Returns the effect resources of type #XfdashboardThemeEffects of theme at @self.
 *
 * Return value: The theme's effect resources of type #XfdashboardThemeEffects
 */
XfdashboardThemeEffects* xfdashboard_theme_get_effects(XfdashboardTheme *self)
{
	g_return_val_if_fail(XFDASHBOARD_IS_THEME(self), NULL);

	return(self->priv->effects);
}

/**
 * xfdashboard_theme_get_animation:
 * @self: A #XfdashboardTheme
 *
 * Returns the animation resources of type #XfdashboardThemeAnimation of theme at @self.
 *
 * Return value: The theme's animation resources of type #XfdashboardThemeAnimation
 */
XfdashboardThemeAnimation* xfdashboard_theme_get_animation(XfdashboardTheme *self)
{
	g_return_val_if_fail(XFDASHBOARD_IS_THEME(self), NULL);

	return(self->priv->animation);
}
