/*
 * theme: Top-level theme object (parses key file and manages loading
 *        resources like css style files, xml layout files etc.)
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

#include "theme.h"

#include <glib/gi18n-lib.h>
#include <glib.h>
#include <gio/gio.h>

/* Define this class in GObject system */
G_DEFINE_TYPE(XfdashboardTheme,
				xfdashboard_theme,
				G_TYPE_OBJECT)

/* Private structure - access only by public API if needed */
#define XFDASHBOARD_THEME_GET_PRIVATE(obj) \
	(G_TYPE_INSTANCE_GET_PRIVATE((obj), XFDASHBOARD_TYPE_THEME, XfdashboardThemePrivate))

struct _XfdashboardThemePrivate
{
	/* Properties related */
	gchar						*themePath;

	gchar						*themeName;
	gchar						*themeDisplayName;
	gchar						*themeComment;

	/* Instance related */
	XfdashboardThemeCSS			*styling;
	XfdashboardThemeLayout		*layout;
	XfdashboardThemeEffects		*effects;
};

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
#define XFDASHBOARD_THEME_GROUP							"Xfdashboard Theme"

/* Release allocated resources in theme instance */
static void _xfdashboard_theme_clean(XfdashboardTheme *self)
{
	XfdashboardThemePrivate		*priv;

	g_return_if_fail(XFDASHBOARD_IS_THEME(self));

	priv=self->priv;

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
}

/* Load theme file and all listed resources in this file */
static gboolean _xfdashboard_theme_load_resources(XfdashboardTheme *self,
													const gchar *inThemePath,
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
	g_return_val_if_fail(inThemePath!=NULL && *inThemePath!=0, FALSE);
	g_return_val_if_fail(outError==NULL || *outError==NULL, FALSE);

	priv=self->priv;
	error=NULL;

	/* Load theme file */
	themeFile=g_build_filename(inThemePath, XFDASHBOARD_THEME_FILE, NULL);

	themeKeyFile=g_key_file_new();
	if(!g_key_file_load_from_file(themeKeyFile,
									themeFile,
									G_KEY_FILE_NONE,
									&error))
	{
		/* Set error */
		g_propagate_error(outError, error);

		/* Release allocated resources */
		_xfdashboard_theme_clean(self);

		if(themeFile) g_free(themeFile);

		/* Return FALSE to indicate error */
		return(FALSE);
	}

	g_free(themeFile);

	/* Get display name and notify about property change (regardless of success result) */
	priv->themeDisplayName=g_key_file_get_locale_string(themeKeyFile,
														XFDASHBOARD_THEME_GROUP,
														"Name",
														NULL,
														&error);
	g_object_notify_by_pspec(G_OBJECT(self), XfdashboardThemeProperties[PROP_DISPLAY_NAME]);

	if(!priv->themeDisplayName)
	{
		/* Set error */
		g_propagate_error(outError, error);

		/* Release allocated resources */
		_xfdashboard_theme_clean(self);

		if(themeKeyFile) g_key_file_free(themeKeyFile);

		/* Return FALSE to indicate error */
		return(FALSE);
	}

	/* Get comment and notify about property change (regardless of success result) */
	priv->themeComment=g_key_file_get_locale_string(themeKeyFile,
														XFDASHBOARD_THEME_GROUP,
														"Comment",
														NULL,
														&error);
	g_object_notify_by_pspec(G_OBJECT(self), XfdashboardThemeProperties[PROP_COMMENT]);

	if(!priv->themeComment)
	{
		/* Set error */
		g_propagate_error(outError, error);

		/* Release allocated resources */
		_xfdashboard_theme_clean(self);

		if(themeKeyFile) g_key_file_free(themeKeyFile);

		/* Return FALSE to indicate error */
		return(FALSE);
	}

	/* Create CSS parser and load style resources */
	resources=g_key_file_get_string_list(themeKeyFile,
											XFDASHBOARD_THEME_GROUP,
											"Style",
											NULL,
											&error);
	if(!resources)
	{
		/* Set error */
		g_propagate_error(outError, error);

		/* Release allocated resources */
		_xfdashboard_theme_clean(self);

		if(themeKeyFile) g_key_file_free(themeKeyFile);

		/* Return FALSE to indicate error */
		return(FALSE);
	}

	priv->styling=xfdashboard_theme_css_new(inThemePath);
	counter=0;
	resource=resources;
	while(*resource)
	{
		/* Get path and file for style resource */
		resourceFile=g_build_filename(inThemePath, *resource, NULL);

		/* Try to load style resource */
		if(!xfdashboard_theme_css_add_file(priv->styling, resourceFile, counter, &error))
		{
			/* Set error */
			g_propagate_error(outError, error);

			/* Release allocated resources */
			_xfdashboard_theme_clean(self);

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

	/* Create XML parser and load layout resources */
	resources=g_key_file_get_string_list(themeKeyFile,
											XFDASHBOARD_THEME_GROUP,
											"Layout",
											NULL,
											&error);
	if(!resources)
	{
		/* Set error */
		g_propagate_error(outError, error);

		/* Release allocated resources */
		_xfdashboard_theme_clean(self);

		if(themeKeyFile) g_key_file_free(themeKeyFile);

		/* Return FALSE to indicate error */
		return(FALSE);
	}

	priv->layout=xfdashboard_theme_layout_new();
	resource=resources;
	while(*resource)
	{
		/* Get path and file for style resource */
		resourceFile=g_build_filename(inThemePath, *resource, NULL);

		/* Try to load style resource */
		if(!xfdashboard_theme_layout_add_file(priv->layout, resourceFile, &error))
		{
			/* Set error */
			g_propagate_error(outError, error);

			/* Release allocated resources */
			_xfdashboard_theme_clean(self);

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
							"Effects",
							NULL))
	{
		resources=g_key_file_get_string_list(themeKeyFile,
												XFDASHBOARD_THEME_GROUP,
												"Effects",
												NULL,
												&error);
		if(!resources)
		{
			/* Set error */
			g_propagate_error(outError, error);

			/* Release allocated resources */
			_xfdashboard_theme_clean(self);

			if(themeKeyFile) g_key_file_free(themeKeyFile);

			/* Return FALSE to indicate error */
			return(FALSE);
		}

		priv->effects=xfdashboard_theme_effects_new();
		resource=resources;
		while(*resource)
		{
			/* Get path and file for style resource */
			resourceFile=g_build_filename(inThemePath, *resource, NULL);

			/* Try to load style resource */
			if(!xfdashboard_theme_effects_add_file(priv->effects, resourceFile, &error))
			{
				/* Set error */
				g_propagate_error(outError, error);

				/* Release allocated resources */
				_xfdashboard_theme_clean(self);

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
	gchar				*themeFile;

	g_return_val_if_fail(XFDASHBOARD_IS_THEME(self), FALSE);
	g_return_val_if_fail(inThemeName!=NULL && *inThemeName!=0, FALSE);

	themeFile=NULL;

	/* Search theme file in given environment variable if set.
	 * This makes development easier when theme changes are needed
	 * without changing theme or changing symlinks in any of below
	 * searched paths.
	 */
	if(!themeFile)
	{
		const gchar		*envPath;

		envPath=g_getenv("XFDASHBOARD_THEME_PATH");
		if(envPath)
		{
			themeFile=g_build_filename(envPath, XFDASHBOARD_THEME_FILE, NULL);
			g_debug("Trying theme file: %s", themeFile);
			if(!g_file_test(themeFile, G_FILE_TEST_EXISTS | G_FILE_TEST_IS_REGULAR))
			{
				g_free(themeFile);
				themeFile=NULL;
			}
		}
	}

	/* If file not found search in user's config directory */
	if(!themeFile)
	{
		themeFile=g_build_filename(g_get_user_data_dir(), "themes", inThemeName, XFDASHBOARD_THEME_SUBPATH, XFDASHBOARD_THEME_FILE, NULL);
		g_debug("Trying theme file: %s", themeFile);
		if(!g_file_test(themeFile, G_FILE_TEST_EXISTS | G_FILE_TEST_IS_REGULAR))
		{
			g_free(themeFile);
			themeFile=NULL;
		}
	}

	/* If file not found search in user's home directory */
	if(!themeFile)
	{
		const gchar		*homeDirectory;

		homeDirectory=g_get_home_dir();
		if(homeDirectory)
		{
			themeFile=g_build_filename(homeDirectory, ".themes", inThemeName, XFDASHBOARD_THEME_SUBPATH, XFDASHBOARD_THEME_FILE, NULL);
			g_debug("Trying theme file: %s", themeFile);
			if(!g_file_test(themeFile, G_FILE_TEST_EXISTS | G_FILE_TEST_IS_REGULAR))
			{
				g_free(themeFile);
				themeFile=NULL;
			}
		}
	}

	/* If file not found search in system-wide paths */
	if(!themeFile)
	{
		themeFile=g_build_filename(PACKAGE_DATADIR, "themes", inThemeName, XFDASHBOARD_THEME_SUBPATH, XFDASHBOARD_THEME_FILE, NULL);
		g_debug("Trying theme file: %s", themeFile);
		if(!g_file_test(themeFile, G_FILE_TEST_EXISTS | G_FILE_TEST_IS_REGULAR))
		{
			g_free(themeFile);
			themeFile=NULL;
		}
	}

	/* If file was found get path contaning file and return it */
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

/* IMPLEMENTATION: GObject */

/* Dispose this object */
static void _xfdashboard_theme_dispose(GObject *inObject)
{
	XfdashboardTheme			*self=XFDASHBOARD_THEME(inObject);

	/* Release allocated resources */
	_xfdashboard_theme_clean(self);

	/* Call parent's class dispose method */
	G_OBJECT_CLASS(xfdashboard_theme_parent_class)->dispose(inObject);
}

/* Set/get properties */
static void _xfdashboard_theme_set_property(GObject *inObject,
											guint inPropID,
											const GValue *inValue,
											GParamSpec *inSpec)
{
	switch(inPropID)
	{
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

	/* Set up private structure */
	g_type_class_add_private(klass, sizeof(XfdashboardThemePrivate));

	/* Define properties */
	XfdashboardThemeProperties[PROP_PATH]=
		g_param_spec_string("theme-path",
							_("Theme path"),
							_("Path where theme was found and loaded from"),
							NULL,
							G_PARAM_READABLE | G_PARAM_STATIC_STRINGS);

	XfdashboardThemeProperties[PROP_NAME]=
		g_param_spec_string("theme-name",
							_("Theme name"),
							_("Short name of theme which was used to lookup theme and folder name where theme is stored in"),
							NULL,
							G_PARAM_READABLE | G_PARAM_STATIC_STRINGS);

	XfdashboardThemeProperties[PROP_DISPLAY_NAME]=
		g_param_spec_string("theme-display-name",
							_("Theme display name"),
							_("The name of theme"),
							NULL,
							G_PARAM_READABLE | G_PARAM_STATIC_STRINGS);

	XfdashboardThemeProperties[PROP_COMMENT]=
		g_param_spec_string("theme-comment",
							_("Theme comment"),
							_("The comment of theme used as description"),
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

	priv=self->priv=XFDASHBOARD_THEME_GET_PRIVATE(self);

	/* Set default values */
	priv->themeName=NULL;
	priv->themePath=NULL;
	priv->themeDisplayName=NULL;
	priv->themeComment=NULL;
	priv->styling=NULL;
	priv->layout=NULL;
	priv->effects=NULL;
}

/* Implementation: Errors */

GQuark xfdashboard_theme_error_quark(void)
{
	return(g_quark_from_static_string("xfdashboard-theme-error-quark"));
}

/* Implementation: Public API */

/* Create new instance */
XfdashboardTheme* xfdashboard_theme_new(void)
{
	return(XFDASHBOARD_THEME(g_object_new(XFDASHBOARD_TYPE_THEME, NULL)));
}

/* Get path where this theme was found and loaded from */
const gchar* xfdashboard_theme_get_path(XfdashboardTheme *self)
{
	g_return_val_if_fail(XFDASHBOARD_IS_THEME(self), NULL);

	return(self->priv->themePath);
}

/* Get theme name (as used when loading theme) */
const gchar* xfdashboard_theme_get_theme_name(XfdashboardTheme *self)
{
	g_return_val_if_fail(XFDASHBOARD_IS_THEME(self), NULL);

	return(self->priv->themeName);
}

/* Get display name of theme */
const gchar* xfdashboard_theme_get_display_name(XfdashboardTheme *self)
{
	g_return_val_if_fail(XFDASHBOARD_IS_THEME(self), NULL);

	return(self->priv->themeDisplayName);
}

/* Get comment of theme */
const gchar* xfdashboard_theme_get_comment(XfdashboardTheme *self)
{
	g_return_val_if_fail(XFDASHBOARD_IS_THEME(self), NULL);

	return(self->priv->themeComment);
}

/* Lookup named theme and load resources */
gboolean xfdashboard_theme_load(XfdashboardTheme *self,
								const gchar *inThemeName,
								GError **outError)
{
	XfdashboardThemePrivate		*priv;
	GError						*error;
	gchar						*themePath;

	g_return_val_if_fail(XFDASHBOARD_IS_THEME(self), FALSE);
	g_return_val_if_fail(inThemeName!=NULL && *inThemeName!=0, FALSE);
	g_return_val_if_fail(outError==NULL || *outError==NULL, FALSE);

	priv=self->priv;
	error=NULL;

	/* Freeze notifications about property changes and collect them */
	g_object_freeze_notify(G_OBJECT(self));

	/* Check if a theme was already loaded */
	if(priv->themeName)
	{
		g_set_error(outError,
					XFDASHBOARD_THEME_ERROR,
					XFDASHBOARD_THEME_ERROR_ALREADY_LOADED,
					_("Theme '%s' requested but '%s' was already loaded"),
					inThemeName,
					priv->themeName);

		/* Thaw notifications and send them now */
		g_object_thaw_notify(G_OBJECT(self));

		return(FALSE);
	}

	/* Lookup path of theme by lookup at all possible paths for theme file */
	themePath=_xfdashboard_theme_lookup_path_for_theme(self, inThemeName);
	if(!themePath)
	{
		g_set_error(outError,
					XFDASHBOARD_THEME_ERROR,
					XFDASHBOARD_THEME_ERROR_THEME_NOT_FOUND,
					_("Theme '%s' not found"),
					inThemeName);

		/* Thaw notifications and send them now */
		g_object_thaw_notify(G_OBJECT(self));

		/* Return FALSE to indicate error */
		return(FALSE);
	}

	/* Load theme key file */
	if(!_xfdashboard_theme_load_resources(self, themePath, &error))
	{
		/* Set returned error */
		g_propagate_error(outError, error);

		/* Release allocated resources */
		if(themePath) g_free(themePath);

		/* Thaw notifications and send them now */
		g_object_thaw_notify(G_OBJECT(self));

		/* Return FALSE to indicate error */
		return(FALSE);
	}

	/* Set up internal variables because theme was loaded successfully
	 * and notify about property changes
	 */
	priv->themePath=g_strdup(themePath);
	g_object_notify_by_pspec(G_OBJECT(self), XfdashboardThemeProperties[PROP_PATH]);

	priv->themeName=g_strdup(inThemeName);
	g_object_notify_by_pspec(G_OBJECT(self), XfdashboardThemeProperties[PROP_NAME]);

	/* Release allocated resources */
	if(themePath) g_free(themePath);

	/* Thaw notifications and send them now */
	g_object_thaw_notify(G_OBJECT(self));

	/* If we found named themed and could load all resources successfully */
	return(TRUE);
}

/* Get theme CSS */
XfdashboardThemeCSS* xfdashboard_theme_get_css(XfdashboardTheme *self)
{
	g_return_val_if_fail(XFDASHBOARD_IS_THEME(self), NULL);

	return(self->priv->styling);
}

/* Get theme layout */
XfdashboardThemeLayout* xfdashboard_theme_get_layout(XfdashboardTheme *self)
{
	g_return_val_if_fail(XFDASHBOARD_IS_THEME(self), NULL);

	return(self->priv->layout);
}

/* Get theme effects */
XfdashboardThemeEffects* xfdashboard_theme_get_effects(XfdashboardTheme *self)
{
	g_return_val_if_fail(XFDASHBOARD_IS_THEME(self), NULL);

	return(self->priv->effects);
}
