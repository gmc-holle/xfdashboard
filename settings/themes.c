/*
 * themes: Theme settings of application
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

#include "themes.h"

#include <glib/gi18n-lib.h>
#include <xfconf/xfconf.h>

/* Define this class in GObject system */
struct _XfdashboardSettingsThemesPrivate
{
	/* Properties related */
	GtkBuilder		*builder;

	/* Instance related */
	XfconfChannel	*xfconfChannel;

	GtkWidget		*widgetThemes;
	GtkWidget		*widgetThemeScreenshot;
	GtkWidget		*widgetThemeNameLabel;
	GtkWidget		*widgetThemeName;
	GtkWidget		*widgetThemeAuthorLabel;
	GtkWidget		*widgetThemeAuthor;
	GtkWidget		*widgetThemeVersionLabel;
	GtkWidget		*widgetThemeVersion;
	GtkWidget		*widgetThemeDescriptionLabel;
	GtkWidget		*widgetThemeDescription;
};

G_DEFINE_TYPE_WITH_PRIVATE(XfdashboardSettingsThemes,
							xfdashboard_settings_themes,
							G_TYPE_OBJECT)

/* Properties */
enum
{
	PROP_0,

	PROP_BUILDER,

	PROP_LAST
};

static GParamSpec* XfdashboardSettingsThemesProperties[PROP_LAST]={ 0, };


/* IMPLEMENTATION: Private variables and methods */
#define XFDASHBOARD_XFCONF_CHANNEL					"xfdashboard"

#define THEME_XFCONF_PROP							"/theme"
#define DEFAULT_THEME								"xfdashboard"

#define XFDASHBOARD_THEME_SUBPATH					"xfdashboard-1.0"
#define XFDASHBOARD_THEME_FILE						"xfdashboard.theme"
#define XFDASHBOARD_THEME_GROUP						"Xfdashboard Theme"
#define MAX_SCREENSHOT_WIDTH						400

enum
{
	XFDASHBOARD_SETTINGS_THEMES_COLUMN_NAME,
	XFDASHBOARD_SETTINGS_THEMES_COLUMN_FILE,

	XFDASHBOARD_SETTINGS_THEMES_COLUMN_DISPLAY_NAME,
	XFDASHBOARD_SETTINGS_THEMES_COLUMN_AUTHORS,
	XFDASHBOARD_SETTINGS_THEMES_COLUMN_VERSION,
	XFDASHBOARD_SETTINGS_THEMES_COLUMN_DESCRIPTION,
	XFDASHBOARD_SETTINGS_THEMES_COLUMN_SCREENSHOTS,

	XFDASHBOARD_SETTINGS_THEMES_COLUMN_LAST
};

/* Setting '/theme' changed either at widget or at xfconf property */
static void _xfdashboard_settings_themes_theme_changed_by_widget(XfdashboardSettingsThemes *self,
																	GtkTreeSelection *inSelection)
{
	XfdashboardSettingsThemesPrivate		*priv;
	GtkTreeModel							*model;
	GtkTreeIter								iter;
	gchar									*themeDisplayName;
	gchar									*themeComment;
	gchar									*themeAuthor;
	gchar									*themeVersion;
	gchar									*themeScreenshot;
	gchar									*themeFilename;
	gchar									*themeName;

	g_return_if_fail(XFDASHBOARD_IS_SETTINGS_THEMES(self));
	g_return_if_fail(GTK_IS_TREE_SELECTION(inSelection));

	priv=self->priv;
	themeDisplayName=NULL;
	themeComment=NULL;
	themeAuthor=NULL;
	themeVersion=NULL;
	themeScreenshot=NULL;
	themeFilename=NULL;
	themeName=NULL;

	/* Get selected entry from widget */
	if(gtk_tree_selection_get_selected(inSelection, &model, &iter))
	{
		/* Get data from model */
		gtk_tree_model_get(model,
							&iter,
							XFDASHBOARD_SETTINGS_THEMES_COLUMN_NAME, &themeName,
							XFDASHBOARD_SETTINGS_THEMES_COLUMN_FILE, &themeFilename,
							XFDASHBOARD_SETTINGS_THEMES_COLUMN_DISPLAY_NAME, &themeDisplayName,
							XFDASHBOARD_SETTINGS_THEMES_COLUMN_DESCRIPTION, &themeComment,
							XFDASHBOARD_SETTINGS_THEMES_COLUMN_AUTHORS, &themeAuthor,
							XFDASHBOARD_SETTINGS_THEMES_COLUMN_VERSION, &themeVersion,
							XFDASHBOARD_SETTINGS_THEMES_COLUMN_SCREENSHOTS, &themeScreenshot,
							-1);
	}

	/* Set text in labels */
	if(themeDisplayName)
	{
		gtk_label_set_text(GTK_LABEL(priv->widgetThemeName), themeDisplayName);
		gtk_widget_show(priv->widgetThemeName);
		gtk_widget_show(priv->widgetThemeNameLabel);
	}
		else
		{
			gtk_widget_hide(priv->widgetThemeName);
			gtk_widget_hide(priv->widgetThemeNameLabel);
		}

	if(themeComment)
	{
		gtk_label_set_text(GTK_LABEL(priv->widgetThemeDescription), themeComment);
		gtk_widget_show(priv->widgetThemeDescription);
		gtk_widget_show(priv->widgetThemeDescriptionLabel);
	}
		else
		{
			gtk_widget_hide(priv->widgetThemeDescription);
			gtk_widget_hide(priv->widgetThemeDescriptionLabel);
		}

	if(themeAuthor)
	{
		gtk_label_set_text(GTK_LABEL(priv->widgetThemeAuthor), themeAuthor);
		gtk_widget_show(priv->widgetThemeAuthor);
		gtk_widget_show(priv->widgetThemeAuthorLabel);
	}
		else
		{
			gtk_widget_hide(priv->widgetThemeAuthor);
			gtk_widget_hide(priv->widgetThemeAuthorLabel);
		}

	if(themeVersion)
	{
		gtk_label_set_text(GTK_LABEL(priv->widgetThemeVersion), themeVersion);
		gtk_widget_show(priv->widgetThemeVersion);
		gtk_widget_show(priv->widgetThemeVersionLabel);
	}
		else
		{
			gtk_widget_hide(priv->widgetThemeVersion);
			gtk_widget_hide(priv->widgetThemeVersionLabel);
		}

	/* Set screenshot */
	if(themeScreenshot)
	{
		gchar						*screenshotFile;
		GdkPixbuf					*screenshotImage;

		screenshotFile=NULL;
		screenshotImage=NULL;

		/* Get screenshot file but resolve relative path if needed */
		if(!g_path_is_absolute(themeScreenshot))
		{
			GFile					*file;
			GFile					*parentPath;
			gchar					*themePath;

			file=NULL;
			parentPath=NULL;
			themePath=NULL;

			/* Resolve relative path relative to theme path */
			file=g_file_new_for_path(themeFilename);
			if(file) parentPath=g_file_get_parent(file);
			if(parentPath) themePath=g_file_get_path(parentPath);
			if(themePath) screenshotFile=g_build_filename(themePath, themeScreenshot, NULL);

			/* Release allocated resources */
			if(themePath) g_free(themePath);
			if(parentPath) g_object_unref(parentPath);
			if(file) g_object_unref(file);
		}
			else
			{
				/* Path is absolute so just create a copy */
				screenshotFile=g_strdup(themeScreenshot);
			}

		/* If screenshot file exists set up and show image
		 * otherwise hide image.
		 */
		if(screenshotFile &&
			g_file_test(screenshotFile, G_FILE_TEST_EXISTS | G_FILE_TEST_IS_REGULAR))
		{
			GError					*error;
			gint					width;
			gint					height;

			error=NULL;

			/* Check if screenshot fits into widget without scaling or
			 * scale it to maximum size but preserve aspect ratio.
			 */
			if(gdk_pixbuf_get_file_info(screenshotFile, &width, &height))
			{
				if(width<MAX_SCREENSHOT_WIDTH)
				{
					screenshotImage=gdk_pixbuf_new_from_file(screenshotFile,
																&error);
				}
					else
					{
						screenshotImage=gdk_pixbuf_new_from_file_at_scale(screenshotFile,
																			MAX_SCREENSHOT_WIDTH,
																			-1,
																			TRUE,
																			&error);
					}

				if(error)
				{
					g_warning("Could not load screenshot: %s",
								error ? error->message : _("Unknown error"));

					/* Release allocated resources */
					if(error) g_error_free(error);
					if(screenshotImage)
					{
						g_object_unref(screenshotImage);
						screenshotImage=NULL;
					}
				}
			}
		}

		if(screenshotImage)
		{
			gtk_image_set_from_pixbuf(GTK_IMAGE(priv->widgetThemeScreenshot), screenshotImage);
			gtk_widget_show(priv->widgetThemeScreenshot);
		}
			else
			{
				gtk_widget_hide(priv->widgetThemeScreenshot);
			}

		/* Release allocated resources */
		if(screenshotImage) g_object_unref(screenshotImage);
		if(screenshotFile) g_free(screenshotFile);
	}
		else
		{
			gtk_widget_hide(priv->widgetThemeScreenshot);
		}

	/* Set value at xfconf property if it must be changed */
	if(themeName)
	{
		gchar						*currentTheme;

		currentTheme=xfconf_channel_get_string(priv->xfconfChannel, THEME_XFCONF_PROP, DEFAULT_THEME);
		if(g_strcmp0(currentTheme, themeName))
		{
			xfconf_channel_set_string(priv->xfconfChannel, THEME_XFCONF_PROP, themeName);
		}
		g_free(currentTheme);
	}

	/* Release allocated resources */
	if(themeDisplayName) g_free(themeDisplayName);
	if(themeComment) g_free(themeComment);
	if(themeAuthor) g_free(themeAuthor);
	if(themeVersion) g_free(themeVersion);
	if(themeScreenshot) g_free(themeScreenshot);
	if(themeFilename) g_free(themeFilename);
	if(themeName) g_free(themeName);
}

static void _xfdashboard_settings_themes_theme_changed_by_xfconf(XfdashboardSettingsThemes *self,
																	const gchar *inProperty,
																	const GValue *inValue,
																	XfconfChannel *inChannel)
{
	XfdashboardSettingsThemesPrivate		*priv;
	const gchar						*newValue;
	GtkTreeModel					*model;
	GtkTreeIter						iter;
	gboolean						selectionFound;

	g_return_if_fail(XFDASHBOARD_IS_SETTINGS_THEMES(self));
	g_return_if_fail(inValue);
	g_return_if_fail(XFCONF_IS_CHANNEL(inChannel));

	priv=self->priv;
	newValue=DEFAULT_THEME;

	/* Get new value to set at widget */
	if(G_LIKELY(G_VALUE_TYPE(inValue)==G_TYPE_STRING)) newValue=g_value_get_string(inValue);

	/* Iterate through themes' model and lookup matching item
	 * against new theme name and select it
	 */
	selectionFound=FALSE;
	model=gtk_tree_view_get_model(GTK_TREE_VIEW(priv->widgetThemes));
	if(newValue && gtk_tree_model_get_iter_first(model, &iter))
	{
		do
		{
			gchar					*value;

			gtk_tree_model_get(model,
								&iter,
								XFDASHBOARD_SETTINGS_THEMES_COLUMN_NAME, &value,
								-1);
			if(G_UNLIKELY(g_str_equal(value, newValue)))
			{
				GtkTreePath			*selectionPath;

				selectionPath=gtk_tree_model_get_path(GTK_TREE_MODEL(model), &iter);
				gtk_tree_selection_select_iter(gtk_tree_view_get_selection(GTK_TREE_VIEW(priv->widgetThemes)), &iter);
				gtk_tree_view_scroll_to_cell(GTK_TREE_VIEW(priv->widgetThemes), selectionPath, NULL, TRUE, 0.5, 0.5);
				gtk_tree_path_free(selectionPath);

				selectionFound=TRUE;
			}
			g_free(value);
		}
		while(!selectionFound && gtk_tree_model_iter_next(model, &iter));
	}

	/* If no selection was found, deselect any selected entry at widget */
	if(!selectionFound)
	{
		GtkTreeSelection			*selection;

		selection=gtk_tree_view_get_selection(GTK_TREE_VIEW(priv->widgetThemes));
		gtk_tree_selection_unselect_all(selection);
	}
}

/* Sorting function for theme list's model */
static gint _xfdashboard_settings_themes_sort_themes_list_model(GtkTreeModel *inModel,
																GtkTreeIter *inLeft,
																GtkTreeIter *inRight,
																gpointer inUserData)
{
	gchar	*leftName;
	gchar	*rightName;
	gint	result;

	/* Get theme names from model */
	leftName=NULL;
	gtk_tree_model_get(inModel, inLeft, 0, &leftName, -1);
	if(!leftName) leftName=g_strdup("");

	rightName=NULL;
	gtk_tree_model_get(inModel, inRight, 0, &rightName, -1);
	if(!rightName) rightName=g_strdup("");

	result=g_utf8_collate(leftName, rightName);

	/* Release allocated resources */
	if(leftName) g_free(leftName);
	if(rightName) g_free(rightName);

	/* Return sorting result */
	return(result);
}

/* Populate list of available themes */
static void _xfdashboard_settings_themes_populate_themes_list(XfdashboardSettingsThemes *self,
																GtkWidget *inWidget)
{
	GHashTable						*themes;
	GArray							*themesPaths;
	GDir							*directory;
	GtkListStore					*model;
	GtkTreeIter						iter;
	guint							i;

	g_return_if_fail(XFDASHBOARD_IS_SETTINGS_THEMES(self));
	g_return_if_fail(GTK_IS_TREE_VIEW(inWidget));

	/* Create hash-table to keep track of duplicates (e.g. user overrides standard themes) */
	themes=g_hash_table_new_full(g_str_hash, g_str_equal, g_free, NULL);

	/* Get model of widget to fill */
	model=gtk_list_store_new(XFDASHBOARD_SETTINGS_THEMES_COLUMN_LAST,
								G_TYPE_STRING, /* XFDASHBOARD_SETTINGS_THEMES_COLUMN_NAME */
								G_TYPE_STRING, /* XFDASHBOARD_SETTINGS_THEMES_COLUMN_FILE */
								G_TYPE_STRING, /* XFDASHBOARD_SETTINGS_THEMES_COLUMN_DISPLAY_NAME */
								G_TYPE_STRING, /* XFDASHBOARD_SETTINGS_THEMES_COLUMN_AUTHORS */
								G_TYPE_STRING, /* XFDASHBOARD_SETTINGS_THEMES_COLUMN_VERSION */
								G_TYPE_STRING, /* XFDASHBOARD_SETTINGS_THEMES_COLUMN_DESCRIPTION */
								G_TYPE_STRING  /* XFDASHBOARD_SETTINGS_THEMES_COLUMN_SCREENSHOTS */
							);
	gtk_tree_sortable_set_sort_func(GTK_TREE_SORTABLE(model),
									XFDASHBOARD_SETTINGS_THEMES_COLUMN_DISPLAY_NAME,
									(GtkTreeIterCompareFunc)_xfdashboard_settings_themes_sort_themes_list_model,
									NULL,
									NULL);
	gtk_tree_sortable_set_sort_column_id(GTK_TREE_SORTABLE(model),
											XFDASHBOARD_SETTINGS_THEMES_COLUMN_DISPLAY_NAME,
											GTK_SORT_ASCENDING);

	/* Get paths to iterate through to find themes */
	themesPaths=g_array_new(TRUE, TRUE, sizeof(gchar*));
	if(themesPaths)
	{
		gchar						*themePath;
		const gchar					*homeDirectory;

		/* First add user's data dir */
		themePath=g_build_filename(g_get_user_data_dir(), "themes", NULL);
		g_array_append_val(themesPaths, themePath);
		g_debug("Adding to theme search path: %s", themePath);

		/* Then add user's home directory */
		homeDirectory=g_get_home_dir();
		if(homeDirectory)
		{
			themePath=g_build_filename(homeDirectory, ".themes", NULL);
			g_array_append_val(themesPaths, themePath);
			g_debug("Adding to theme search path: %s", themePath);
		}

		/* At last add system-wide path */
		themePath=g_build_filename(PACKAGE_DATADIR, "themes", NULL);
		g_array_append_val(themesPaths, themePath);
		g_debug("Adding to theme search path: %s", themePath);
	}

	/* Iterate through all themes at all theme paths */
	for(i=0; i<themesPaths->len; ++i)
	{
		const gchar					*themePath;
		const gchar					*themeName;

		/* Get theme path to iterate through */
		themePath=g_array_index(themesPaths, const gchar*, i);

		/* Open handle to directory to iterate through
		 * but skip NULL paths or directory objects
		 */
		directory=g_dir_open(themePath, 0, NULL);
		if(G_UNLIKELY(directory==NULL)) continue;

		/* Iterate through directory and find available themes */
		while((themeName=g_dir_read_name(directory)))
		{
			gchar					*themeIndexFile;
			GKeyFile				*themeKeyFile;
			GError					*error;
			gchar					*themeDisplayName;
			gchar					*themeComment;
			gchar					**themeAuthors;
			gchar					*realThemeAuthor;
			gchar					*themeVersion;
			gchar					**themeScreenshots;
			gchar					*realThemeScreenshot;

			error=NULL;
			themeDisplayName=NULL;
			themeComment=NULL;
			themeAuthors=NULL;
			themeVersion=NULL;
			themeScreenshots=NULL;
			realThemeAuthor=NULL;
			realThemeScreenshot=NULL;

			/* Check if theme description file exists and add it if there is no theme
			 * with same name.
			 */
			themeIndexFile=g_build_filename(themePath, themeName, XFDASHBOARD_THEME_SUBPATH, XFDASHBOARD_THEME_FILE, NULL);
			if(!g_file_test(themeIndexFile, G_FILE_TEST_EXISTS | G_FILE_TEST_IS_REGULAR))
			{
				g_debug("Invalid theme '%s': Missing theme index file at %s",
							themeName,
							themeIndexFile);

				/* Release allocated resources */
				if(themeIndexFile) g_free(themeIndexFile);

				/* Continue with next entry */
				continue;
			}

			if(g_hash_table_lookup(themes, themeName))
			{
				g_debug("Invalid theme '%s': Duplicate theme at %s",
							themeName,
							themeIndexFile);

				/* Release allocated resources */
				if(themeIndexFile) g_free(themeIndexFile);

				/* Continue with next entry */
				continue;
			}

			/* Get theme file to retrieve data of it */
			themeKeyFile=g_key_file_new();
			if(!g_key_file_load_from_file(themeKeyFile,
											themeIndexFile,
											G_KEY_FILE_NONE,
											&error))
			{
				g_warning("Invalid theme '%s' at %s: %s",
							themeName,
							themeIndexFile,
							error ? error->message : _("Unknown error"));

				/* Release allocated resources */
				if(error) g_error_free(error);
				if(themeKeyFile) g_key_file_free(themeKeyFile);
				if(themeIndexFile) g_free(themeIndexFile);

				/* Continue with next entry */
				continue;
			}

			/* Check if theme is valid by checking if all essential
			 * keys are defined: Name, Comment, Style and Layout
			 */
			if(!g_key_file_has_key(themeKeyFile,
									XFDASHBOARD_THEME_GROUP,
									"Name",
									&error))
			{
				g_warning("Invalid theme '%s' at %s: %s",
							themeName,
							themeIndexFile,
							error ? error->message : _("Unknown error"));

				/* Release allocated resources */
				if(error) g_error_free(error);
				if(themeKeyFile) g_key_file_free(themeKeyFile);
				if(themeIndexFile) g_free(themeIndexFile);

				/* Continue with next entry */
				continue;
			}

			if(!g_key_file_has_key(themeKeyFile,
									XFDASHBOARD_THEME_GROUP,
									"Comment",
									&error))
			{
				g_warning("Invalid theme '%s' at %s: %s",
							themeName,
							themeIndexFile,
							error ? error->message : _("Unknown error"));

				/* Release allocated resources */
				if(error) g_error_free(error);
				if(themeKeyFile) g_key_file_free(themeKeyFile);
				if(themeIndexFile) g_free(themeIndexFile);

				/* Continue with next entry */
				continue;
			}

			if(!g_key_file_has_key(themeKeyFile,
									XFDASHBOARD_THEME_GROUP,
									"Style",
									&error))
			{
				g_warning("Invalid theme '%s' at %s: %s",
							themeName,
							themeIndexFile,
							error ? error->message : _("Unknown error"));

				/* Release allocated resources */
				if(error) g_error_free(error);
				if(themeKeyFile) g_key_file_free(themeKeyFile);
				if(themeIndexFile) g_free(themeIndexFile);

				/* Continue with next entry */
				continue;
			}

			if(!g_key_file_has_key(themeKeyFile,
									XFDASHBOARD_THEME_GROUP,
									"Layout",
									&error))
			{
				g_warning("Invalid theme '%s' at %s: %s",
							themeName,
							themeIndexFile,
							error ? error->message : _("Unknown error"));

				/* Release allocated resources */
				if(error) g_error_free(error);
				if(themeKeyFile) g_key_file_free(themeKeyFile);
				if(themeIndexFile) g_free(themeIndexFile);

				/* Continue with next entry */
				continue;
			}

			/* Theme is valid so get theme data which includes
			 * optional field: Author, Version and Screenshot
			 */
			themeDisplayName=g_key_file_get_locale_string(themeKeyFile,
															XFDASHBOARD_THEME_GROUP,
															"Name",
															NULL,
															NULL);

			themeComment=g_key_file_get_locale_string(themeKeyFile,
															XFDASHBOARD_THEME_GROUP,
															"Comment",
															NULL,
															NULL);

			themeAuthors=g_key_file_get_string_list(themeKeyFile,
													XFDASHBOARD_THEME_GROUP,
													"Author",
													NULL,
													NULL);
			if(themeAuthors)
			{
				/* Replace list of string with authors with one string
				 * containing all authors seperated by new-line.
				 */
				realThemeAuthor=g_strjoinv("\n", themeAuthors);
			}

			themeVersion=g_key_file_get_string(themeKeyFile,
												XFDASHBOARD_THEME_GROUP,
												"Version",
												NULL);

			themeScreenshots=g_key_file_get_string_list(themeKeyFile,
														XFDASHBOARD_THEME_GROUP,
														"Screenshot",
														NULL,
														NULL);
			if(themeScreenshots)
			{
				/* Replace list of string with filenames to screenshots
				 * with one string containing the first screenshot's filename.
				 */
				realThemeScreenshot=g_strdup(themeScreenshots[0]);
			}

			/* Add to widget's list */
			gtk_list_store_append(GTK_LIST_STORE(model), &iter);
			gtk_list_store_set(GTK_LIST_STORE(model), &iter,
								XFDASHBOARD_SETTINGS_THEMES_COLUMN_NAME, themeName,
								XFDASHBOARD_SETTINGS_THEMES_COLUMN_FILE, themeIndexFile,
								XFDASHBOARD_SETTINGS_THEMES_COLUMN_DISPLAY_NAME, themeDisplayName,
								XFDASHBOARD_SETTINGS_THEMES_COLUMN_AUTHORS, realThemeAuthor,
								XFDASHBOARD_SETTINGS_THEMES_COLUMN_VERSION, themeVersion,
								XFDASHBOARD_SETTINGS_THEMES_COLUMN_DESCRIPTION, themeComment,
								XFDASHBOARD_SETTINGS_THEMES_COLUMN_SCREENSHOTS, realThemeScreenshot,
								-1);

			/* Remember theme to avoid duplicates (and allow overrides by user */
			g_hash_table_insert(themes, g_strdup(themeName), GINT_TO_POINTER(1));
			g_debug("Added theme '%s' from %s", themeName, themeIndexFile);

			/* Release allocated resources */
			if(realThemeAuthor) g_free(realThemeAuthor);
			if(realThemeScreenshot) g_free(realThemeScreenshot);
			if(themeDisplayName) g_free(themeDisplayName);
			if(themeComment) g_free(themeComment);
			if(themeAuthors) g_strfreev(themeAuthors);
			if(themeVersion) g_free(themeVersion);
			if(themeScreenshots) g_strfreev(themeScreenshots);
			if(themeKeyFile) g_key_file_free(themeKeyFile);
			if(themeIndexFile) g_free(themeIndexFile);
		}

		/* Close handle to directory */
		g_dir_close(directory);
	}

	/* Set new list store at widget */
	gtk_tree_view_set_model(GTK_TREE_VIEW(inWidget), GTK_TREE_MODEL(model));
	g_object_unref(model);

	/* Release allocated resources */
	if(themesPaths)
	{
		for(i=0; i<themesPaths->len; ++i)
		{
			gchar						*themePath;

			themePath=g_array_index(themesPaths, gchar*, i);
			g_free(themePath);
		}
		g_array_free(themesPaths, TRUE);
	}
	if(themes) g_hash_table_destroy(themes);
}

/* Create and set up GtkBuilder */
static void _xfdashboard_settings_themes_set_builder(XfdashboardSettingsThemes *self,
														GtkBuilder *inBuilder)
{
	XfdashboardSettingsThemesPrivate	*priv;

	g_return_if_fail(XFDASHBOARD_IS_SETTINGS_THEMES(self));
	g_return_if_fail(GTK_IS_BUILDER(inBuilder));

	priv=self->priv;

	/* Set builder object which must not be set yet */
	g_assert(!priv->builder);

	priv->builder=g_object_ref(inBuilder);

	/* Get widgets from builder */
	priv->widgetThemes=GTK_WIDGET(gtk_builder_get_object(priv->builder, "themes"));
	priv->widgetThemeScreenshot=GTK_WIDGET(gtk_builder_get_object(priv->builder, "theme-screenshot"));
	priv->widgetThemeNameLabel=GTK_WIDGET(gtk_builder_get_object(priv->builder, "theme-name-label"));
	priv->widgetThemeName=GTK_WIDGET(gtk_builder_get_object(priv->builder, "theme-name"));
	priv->widgetThemeAuthorLabel=GTK_WIDGET(gtk_builder_get_object(priv->builder, "theme-author-label"));
	priv->widgetThemeAuthor=GTK_WIDGET(gtk_builder_get_object(priv->builder, "theme-author"));
	priv->widgetThemeVersionLabel=GTK_WIDGET(gtk_builder_get_object(priv->builder, "theme-version-label"));
	priv->widgetThemeVersion=GTK_WIDGET(gtk_builder_get_object(priv->builder, "theme-version"));
	priv->widgetThemeDescriptionLabel=GTK_WIDGET(gtk_builder_get_object(priv->builder, "theme-description-label"));
	priv->widgetThemeDescription=GTK_WIDGET(gtk_builder_get_object(priv->builder, "theme-description"));

	/* Set up theme list */
	if(priv->widgetThemes)
	{
		gchar							*currentTheme;
		GValue							defaultValue=G_VALUE_INIT;
		GtkTreeSelection				*selection;
		GtkCellRenderer					*renderer;

		/* Get default value */
		currentTheme=xfconf_channel_get_string(priv->xfconfChannel, THEME_XFCONF_PROP, DEFAULT_THEME);
		g_value_init(&defaultValue, G_TYPE_STRING);
		g_value_set_string(&defaultValue, currentTheme);
		g_free(currentTheme);

		/* Themes widget has only one column displaying theme's name.
		 * Set up column and renderer.
		 */
		renderer=gtk_cell_renderer_text_new();
		gtk_tree_view_insert_column_with_attributes(GTK_TREE_VIEW(priv->widgetThemes),
													0,
													_("Theme"),
													renderer,
													"text",
													XFDASHBOARD_SETTINGS_THEMES_COLUMN_NAME,
													NULL);

		/* Ensure only one selection at time is possible */
		selection=gtk_tree_view_get_selection(GTK_TREE_VIEW(priv->widgetThemes));
		gtk_tree_selection_set_mode(selection, GTK_SELECTION_SINGLE);

		/* Populate list of available themes */
		_xfdashboard_settings_themes_populate_themes_list(self, priv->widgetThemes);

		/* Select default value */
		_xfdashboard_settings_themes_theme_changed_by_xfconf(self,
																THEME_XFCONF_PROP,
																&defaultValue,
																priv->xfconfChannel);
		_xfdashboard_settings_themes_theme_changed_by_widget(self, selection);

		/* Connect signals */
		g_signal_connect_swapped(selection,
									"changed",
									G_CALLBACK(_xfdashboard_settings_themes_theme_changed_by_widget),
									self);
		g_signal_connect_swapped(priv->xfconfChannel,
									"property-changed::/theme",
									G_CALLBACK(_xfdashboard_settings_themes_theme_changed_by_xfconf),
									self);

		/* Release allocated resources */
		g_value_unset(&defaultValue);
	}
}

/* IMPLEMENTATION: GObject */

/* Dispose this object */
static void _xfdashboard_settings_themes_dispose(GObject *inObject)
{
	XfdashboardSettingsThemes			*self=XFDASHBOARD_SETTINGS_THEMES(inObject);
	XfdashboardSettingsThemesPrivate	*priv=self->priv;

	/* Release allocated resouces */
	priv->widgetThemes=NULL;
	priv->widgetThemeScreenshot=NULL;
	priv->widgetThemeNameLabel=NULL;
	priv->widgetThemeName=NULL;
	priv->widgetThemeAuthorLabel=NULL;
	priv->widgetThemeAuthor=NULL;
	priv->widgetThemeVersionLabel=NULL;
	priv->widgetThemeVersion=NULL;
	priv->widgetThemeDescriptionLabel=NULL;
	priv->widgetThemeDescription=NULL;

	if(priv->builder)
	{
		g_object_unref(priv->builder);
		priv->builder=NULL;
	}

	if(priv->xfconfChannel)
	{
		priv->xfconfChannel=NULL;
	}

	/* Call parent's class dispose method */
	G_OBJECT_CLASS(xfdashboard_settings_themes_parent_class)->dispose(inObject);
}

/* Set/get properties */
static void _xfdashboard_settings_themes_set_property(GObject *inObject,
														guint inPropID,
														const GValue *inValue,
														GParamSpec *inSpec)
{
	XfdashboardSettingsThemes				*self=XFDASHBOARD_SETTINGS_THEMES(inObject);

	switch(inPropID)
	{
		case PROP_BUILDER:
			_xfdashboard_settings_themes_set_builder(self, GTK_BUILDER(g_value_get_object(inValue)));
			break;

		default:
			G_OBJECT_WARN_INVALID_PROPERTY_ID(inObject, inPropID, inSpec);
			break;
	}
}

static void _xfdashboard_settings_themes_get_property(GObject *inObject,
														guint inPropID,
														GValue *outValue,
														GParamSpec *inSpec)
{
	XfdashboardSettingsThemes				*self=XFDASHBOARD_SETTINGS_THEMES(inObject);
	XfdashboardSettingsThemesPrivate		*priv=self->priv;

	switch(inPropID)
	{
		case PROP_BUILDER:
			g_value_set_object(outValue, priv->builder);
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
static void xfdashboard_settings_themes_class_init(XfdashboardSettingsThemesClass *klass)
{
	GObjectClass		*gobjectClass=G_OBJECT_CLASS(klass);

	/* Override functions */
	gobjectClass->dispose=_xfdashboard_settings_themes_dispose;
	gobjectClass->set_property=_xfdashboard_settings_themes_set_property;
	gobjectClass->get_property=_xfdashboard_settings_themes_get_property;

	/* Define properties */
	XfdashboardSettingsThemesProperties[PROP_BUILDER]=
		g_param_spec_object("builder",
								_("Builder"),
								_("The initialized GtkBuilder object where to set up themes tab from"),
								GTK_TYPE_BUILDER,
								G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS | G_PARAM_CONSTRUCT_ONLY);

	g_object_class_install_properties(gobjectClass, PROP_LAST, XfdashboardSettingsThemesProperties);
}

/* Object initialization
 * Create private structure and set up default values
 */
static void xfdashboard_settings_themes_init(XfdashboardSettingsThemes *self)
{
	XfdashboardSettingsThemesPrivate	*priv;

	priv=self->priv=xfdashboard_settings_themes_get_instance_private(self);

	/* Set default values */
	priv->builder=NULL;

	priv->xfconfChannel=xfconf_channel_get(XFDASHBOARD_XFCONF_CHANNEL);

	priv->widgetThemes=NULL;
	priv->widgetThemeScreenshot=NULL;
	priv->widgetThemeNameLabel=NULL;
	priv->widgetThemeName=NULL;
	priv->widgetThemeAuthorLabel=NULL;
	priv->widgetThemeAuthor=NULL;
	priv->widgetThemeVersionLabel=NULL;
	priv->widgetThemeVersion=NULL;
	priv->widgetThemeDescriptionLabel=NULL;
	priv->widgetThemeDescription=NULL;
}

/* IMPLEMENTATION: Public API */

/* Create instance of this class */
XfdashboardSettingsThemes* xfdashboard_settings_themes_new(GtkBuilder *inBuilder)
{
	GObject		*instance;

	g_return_val_if_fail(GTK_IS_BUILDER(inBuilder), NULL);

	/* Create instance */
	instance=g_object_new(XFDASHBOARD_TYPE_SETTINGS_THEMES,
							"builder", inBuilder,
							NULL);

	/* Return newly created instance */
	return(XFDASHBOARD_SETTINGS_THEMES(instance));
}
