/*
 * view-manager: Single-instance managing views
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

#include "settings.h"

#include <glib/gi18n-lib.h>
#include <xfconf/xfconf.h>
#include <math.h>

/* Define this class in GObject system */
G_DEFINE_TYPE(XfdashboardSettings,
				xfdashboard_settings,
				G_TYPE_OBJECT)

/* Private structure - access only by public API if needed */
#define XFDASHBOARD_SETTINGS_GET_PRIVATE(obj) \
	(G_TYPE_INSTANCE_GET_PRIVATE((obj), XFDASHBOARD_TYPE_SETTINGS, XfdashboardSettingsPrivate))

struct _XfdashboardSettingsPrivate
{
	/* Instance related */
	XfconfChannel	*xfconfChannel;

	GtkBuilder		*builder;
	GtkWidget		*widgetCloseButton;
	GtkWidget		*widgetResetSearchOnResume;
	GtkWidget		*widgetSwitchViewOnResume;
	GtkWidget		*widgetNotificationTimeout;
	GtkWidget		*widgetEnableUnmappedWindowWorkaround;
	GtkWidget		*widgetShowAllApps;
	GtkWidget		*widgetScrollEventChangedWorkspace;
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

/* IMPLEMENTATION: Private variables and methods */
#define XFDASHBOARD_XFCONF_CHANNEL				"xfdashboard"
#define PREFERENCES_UI_FILE						"preferences.ui"
#define XFDASHBOARD_THEME_SUBPATH				"xfdashboard-1.0"
#define XFDASHBOARD_THEME_FILE					"xfdashboard.theme"
#define XFDASHBOARD_THEME_GROUP					"Xfdashboard Theme"
#define DEFAULT_NOTIFICATION_TIMEOUT			3000
#define DEFAULT_RESET_SEARCH_ON_RESUME			TRUE
#define DEFAULT_SWITCH_VIEW_ON_RESUME			NULL
#define DEFAULT_THEME							"xfdashboard"
#define DEFAULT_ENABLE_HOTKEY					FALSE
#define MAX_SCREENSHOT_WIDTH					400

typedef struct _XfdashboardSettingsResumableViews	XfdashboardSettingsResumableViews;
struct _XfdashboardSettingsResumableViews
{
	const gchar		*displayName;
	const gchar		*viewName;
};

static XfdashboardSettingsResumableViews	resumableViews[]=
{
	{ N_("Do nothing"), "" },
	{ N_("Windows view"), "windows" },
	{ N_("Applications view"), "applications" },
	{ NULL, NULL }
};

enum
{
	COLUMN_THEME_NAME,
	COLUMN_THEME_FILE,

	COLUMN_THEME_DISPLAY_NAME,
	COLUMN_THEME_AUTHORS,
	COLUMN_THEME_VERSION,
	COLUMN_THEME_DESCRIPTION,
	COLUMN_THEME_SCREENSHOTS,

	COLUMN_THEME_LAST
};

/* Setting '/switch-view-on-resume' changed either at widget or at xfconf property */
static void _xfdashboard_settings_widget_changed_switch_view_on_resume(XfdashboardSettings *self, GtkComboBox *inComboBox)
{
	XfdashboardSettingsPrivate		*priv;
	GtkTreeModel					*model;
	GtkTreeIter						iter;
	gchar							*value;

	g_return_if_fail(XFDASHBOARD_IS_SETTINGS(self));
	g_return_if_fail(GTK_IS_COMBO_BOX(inComboBox));

	priv=self->priv;

	/* Get selected entry from combo box */
	model=gtk_combo_box_get_model(inComboBox);
	gtk_combo_box_get_active_iter(inComboBox, &iter);
	gtk_tree_model_get(model, &iter, 1, &value, -1);

	/* Set value at xfconf property */
	xfconf_channel_set_string(priv->xfconfChannel, "/switch-view-on-resume", value);

	/* Release allocated resources */
	if(value) g_free(value);
}

static void _xfdashboard_settings_xfconf_changed_switch_view_on_resume(XfdashboardSettings *self,
																		const gchar *inProperty,
																		const GValue *inValue,
																		XfconfChannel *inChannel)
{
	XfdashboardSettingsPrivate		*priv;
	GtkTreeModel					*model;
	GtkTreeIter						iter;
	gchar							*value;
	const gchar						*newValue;

	g_return_if_fail(XFDASHBOARD_IS_SETTINGS(self));
	g_return_if_fail(inValue);
	g_return_if_fail(XFCONF_IS_CHANNEL(inChannel));

	priv=self->priv;

	/* Get new value to lookup and set at combo box */
	if(G_UNLIKELY(G_VALUE_TYPE(inValue)!=G_TYPE_STRING)) newValue="";
		else newValue=g_value_get_string(inValue);

	/* Iterate through combo box value and set new value if match is found */
	model=gtk_combo_box_get_model(GTK_COMBO_BOX(priv->widgetSwitchViewOnResume));
	if(gtk_tree_model_get_iter_first(model, &iter))
	{
		do
		{
			gtk_tree_model_get(model, &iter, 1, &value, -1);
			if(G_UNLIKELY(g_str_equal(value, newValue)))
			{
				g_free(value);
				gtk_combo_box_set_active_iter(GTK_COMBO_BOX(priv->widgetSwitchViewOnResume), &iter);
				break;
			}
			g_free(value);
		}
		while(gtk_tree_model_iter_next(model, &iter));
	}
}

/* Setting '/min-notification-timeout' changed either at widget or at xfconf property */
static void _xfdashboard_settings_widget_changed_notification_timeout(XfdashboardSettings *self, GtkRange *inRange)
{
	XfdashboardSettingsPrivate		*priv;
	guint							value;

	g_return_if_fail(XFDASHBOARD_IS_SETTINGS(self));
	g_return_if_fail(GTK_IS_RANGE(inRange));

	priv=self->priv;

	/* Get value from widget */
	value=floor(gtk_range_get_value(inRange)*1000);

	/* Set value at xfconf property */
	xfconf_channel_set_uint(priv->xfconfChannel, "/min-notification-timeout", value);
}

static void _xfdashboard_settings_xfconf_changed_notification_timeout(XfdashboardSettings *self,
																		const gchar *inProperty,
																		const GValue *inValue,
																		XfconfChannel *inChannel)
{
	XfdashboardSettingsPrivate		*priv;
	guint							newValue;

	g_return_if_fail(XFDASHBOARD_IS_SETTINGS(self));
	g_return_if_fail(inValue);
	g_return_if_fail(XFCONF_IS_CHANNEL(inChannel));

	priv=self->priv;

	/* Get new value to set at widget */
	if(G_UNLIKELY(G_VALUE_TYPE(inValue)!=G_TYPE_UINT)) newValue=DEFAULT_NOTIFICATION_TIMEOUT;
		else newValue=g_value_get_uint(inValue);

	/* Set new value at widget */
	gtk_range_set_value(GTK_RANGE(priv->widgetNotificationTimeout), newValue/1000.0);
}

/* Format value to show in notification timeout slider */
static gchar* _xfdashboard_settings_on_format_notification_timeout_value(GtkScale *inWidget,
																			gdouble inValue,
																			gpointer inUserData)
{
	gchar		*text;

	text=g_strdup_printf("%.1f %s", inValue, _("seconds"));

	return(text);
}

/* Setting '/theme' changed either at widget or at xfconf property */
static void _xfdashboard_settings_widget_changed_theme(XfdashboardSettings *self, GtkTreeSelection *inSelection)
{
	XfdashboardSettingsPrivate		*priv;
	GtkTreeModel					*model;
	GtkTreeIter						iter;
	gchar							*themeDisplayName;
	gchar							*themeComment;
	gchar							*themeAuthor;
	gchar							*themeVersion;
	gchar							*themeScreenshot;
	gchar							*themeFilename;
	gchar							*themeName;

	g_return_if_fail(XFDASHBOARD_IS_SETTINGS(self));
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
							COLUMN_THEME_NAME, &themeName,
							COLUMN_THEME_FILE, &themeFilename,
							COLUMN_THEME_DISPLAY_NAME, &themeDisplayName,
							COLUMN_THEME_DESCRIPTION, &themeComment,
							COLUMN_THEME_AUTHORS, &themeAuthor,
							COLUMN_THEME_VERSION, &themeVersion,
							COLUMN_THEME_SCREENSHOTS, &themeScreenshot,
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

		currentTheme=xfconf_channel_get_string(priv->xfconfChannel, "/theme", DEFAULT_THEME);
		if(g_strcmp0(currentTheme, themeName))
		{
			xfconf_channel_set_string(priv->xfconfChannel, "/theme", themeName);
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

static void _xfdashboard_settings_xfconf_changed_theme(XfdashboardSettings *self,
														const gchar *inProperty,
														const GValue *inValue,
														XfconfChannel *inChannel)
{
	XfdashboardSettingsPrivate		*priv;
	const gchar						*newValue;
	GtkTreeModel					*model;
	GtkTreeIter						iter;
	gboolean						selectionFound;

	g_return_if_fail(XFDASHBOARD_IS_SETTINGS(self));
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
								COLUMN_THEME_NAME, &value,
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
static gint _xfdashboard_settings_sort_themes_list_model(GtkTreeModel *inModel,
															GtkTreeIter *inLeft,
															GtkTreeIter *inRight)
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
static void _xfdashboard_settings_populate_themes_list(XfdashboardSettings *self,
														GtkWidget *inWidget)
{
	GHashTable						*themes;
	GArray							*themesPaths;
	GDir							*directory;
	GtkListStore					*model;
	GtkTreeIter						iter;
	guint							i;

	g_return_if_fail(XFDASHBOARD_IS_SETTINGS(self));
	g_return_if_fail(GTK_IS_TREE_VIEW(inWidget));

	/* Create hash-table to keep track of duplicates (e.g. user overrides standard themes) */
	themes=g_hash_table_new_full(g_str_hash, g_str_equal, g_free, NULL);

	/* Get model of widget to fill */
	model=gtk_list_store_new(COLUMN_THEME_LAST,
								G_TYPE_STRING, /* COLUMN_THEME_NAME */
								G_TYPE_STRING, /* COLUMN_THEME_FILE */
								G_TYPE_STRING, /* COLUMN_THEME_DISPLAY_NAME */
								G_TYPE_STRING, /* COLUMN_THEME_AUTHORS */
								G_TYPE_STRING, /* COLUMN_THEME_VERSION */
								G_TYPE_STRING, /* COLUMN_THEME_DESCRIPTION */
								G_TYPE_STRING  /* COLUMN_THEME_SCREENSHOTS */
							);
	gtk_tree_sortable_set_sort_func(GTK_TREE_SORTABLE(model),
									COLUMN_THEME_DISPLAY_NAME,
									(GtkTreeIterCompareFunc)_xfdashboard_settings_sort_themes_list_model,
									NULL,
									NULL);
	gtk_tree_sortable_set_sort_column_id(GTK_TREE_SORTABLE(model),
											COLUMN_THEME_DISPLAY_NAME,
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
								COLUMN_THEME_NAME, themeName,
								COLUMN_THEME_FILE, themeIndexFile,
								COLUMN_THEME_DISPLAY_NAME, themeDisplayName,
								COLUMN_THEME_AUTHORS, realThemeAuthor,
								COLUMN_THEME_VERSION, themeVersion,
								COLUMN_THEME_DESCRIPTION, themeComment,
								COLUMN_THEME_SCREENSHOTS, realThemeScreenshot,
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

/* Close button was clicked */
static void _xfdashboard_settings_on_close_clicked(XfdashboardSettings *self,
													GtkWidget *inWidget)
{
	g_return_if_fail(XFDASHBOARD_IS_SETTINGS(self));

	/* Quit main loop */
	gtk_main_quit();
}

/* Create and set up GtkBuilder */
static gboolean _xfdashboard_settings_create_builder(XfdashboardSettings *self)
{
	XfdashboardSettingsPrivate				*priv;
	gchar									*builderFile;
	GtkBuilder								*builder;
	GError									*error;

	g_return_val_if_fail(XFDASHBOARD_IS_SETTINGS(self), FALSE);

	priv=self->priv;
	builderFile=NULL;
	builder=NULL;
	error=NULL;

	/* If builder is already set up return immediately */
	if(priv->builder) return(TRUE);

	/* Find UI file */
	builderFile=g_build_filename(PACKAGE_DATADIR, "xfdashboard", PREFERENCES_UI_FILE, NULL);
	g_debug("Trying UI file: %s", builderFile);
	if(!g_file_test(builderFile, G_FILE_TEST_EXISTS | G_FILE_TEST_IS_REGULAR))
	{
		g_critical(_("Could not find UI file '%s'."), builderFile);

		/* Release allocated resources */
		g_free(builderFile);

		/* Return fail result */
		return(FALSE);
	}

	/* Create builder */
	builder=gtk_builder_new();
	if(!gtk_builder_add_from_file(builder, builderFile, &error))
	{
		g_critical(_("Could not load UI resources from '%s': %s"),
					builderFile,
					error ? error->message : _("Unknown error"));

		/* Release allocated resources */
		g_free(builderFile);
		g_object_unref(builder);
		if(error) g_error_free(error);

		/* Return fail result */
		return(FALSE);
	}

	/* Loading UI resource was successful so take extra reference
	 * from builder object to keep it alive. Also get widget, set up
	 * xfconf bindings and connect signals.
	 * REMEMBER: Set (widget's) default value _before_ setting up
	 * xfconf binding.
	 */
	priv->builder=GTK_BUILDER(g_object_ref(builder));
	g_debug("Loaded UI resources from '%s' successfully.", builderFile);

	/* Tab: General */
	priv->widgetResetSearchOnResume=GTK_WIDGET(gtk_builder_get_object(priv->builder, "reset-search-on-resume"));
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(priv->widgetResetSearchOnResume), DEFAULT_RESET_SEARCH_ON_RESUME);
	xfconf_g_property_bind(priv->xfconfChannel,
							"/reset-search-on-resume",
							G_TYPE_BOOLEAN,
							priv->widgetResetSearchOnResume,
							"active");

	priv->widgetSwitchViewOnResume=GTK_WIDGET(gtk_builder_get_object(priv->builder, "switch-view-on-resume"));
	if(priv->widgetSwitchViewOnResume)
	{
		GtkCellRenderer						*renderer;
		GtkListStore						*listStore;
		GtkTreeIter							listStoreIter;
		GtkTreeIter							*defaultListStoreIter;
		XfdashboardSettingsResumableViews	*iter;
		gchar								*defaultValue;

		/* Get default value from settings */
		defaultValue=xfconf_channel_get_string(priv->xfconfChannel, "/switch-view-on-resume", DEFAULT_SWITCH_VIEW_ON_RESUME);
		if(!defaultValue) defaultValue=g_strdup("");

		/* Clear combo box */
		gtk_cell_layout_clear(GTK_CELL_LAYOUT(priv->widgetSwitchViewOnResume));

		/* Set up renderer for combo box */
		renderer=gtk_cell_renderer_text_new();
		gtk_cell_layout_pack_start(GTK_CELL_LAYOUT(priv->widgetSwitchViewOnResume), renderer, TRUE);
		gtk_cell_layout_add_attribute(GTK_CELL_LAYOUT(priv->widgetSwitchViewOnResume), renderer, "text", 0);

		/* Set up list to show at combo box */
		defaultListStoreIter=NULL;
		listStore=gtk_list_store_new(2, G_TYPE_STRING, G_TYPE_STRING);
		for(iter=resumableViews; iter->displayName; ++iter)
		{
			gtk_list_store_append(listStore, &listStoreIter);
			gtk_list_store_set(listStore, &listStoreIter, 0, _(iter->displayName), 1, iter->viewName, -1);
			if(!g_strcmp0(iter->viewName, defaultValue))
			{
				defaultListStoreIter=gtk_tree_iter_copy(&listStoreIter);
			}
		}
		gtk_combo_box_set_model(GTK_COMBO_BOX(priv->widgetSwitchViewOnResume), GTK_TREE_MODEL(listStore));
		g_object_unref(G_OBJECT(listStore));

		/* Set up default value */
		if(defaultListStoreIter)
		{
			gtk_combo_box_set_active_iter(GTK_COMBO_BOX(priv->widgetSwitchViewOnResume), defaultListStoreIter);
			gtk_tree_iter_free(defaultListStoreIter);
			defaultListStoreIter=NULL;
		}

		/* Connect signals */
		g_signal_connect_swapped(priv->widgetSwitchViewOnResume,
									"changed",
									G_CALLBACK(_xfdashboard_settings_widget_changed_switch_view_on_resume),
									self);
		g_signal_connect_swapped(priv->xfconfChannel,
									"property-changed::/switch-view-on-resume",
									G_CALLBACK(_xfdashboard_settings_xfconf_changed_switch_view_on_resume),
									self);

		/* Release allocated resources */
		if(defaultValue) g_free(defaultValue);
	}

	priv->widgetNotificationTimeout=GTK_WIDGET(gtk_builder_get_object(priv->builder, "notification-timeout"));
	if(priv->widgetNotificationTimeout)
	{
		GtkAdjustment						*adjustment;
		gdouble								defaultValue;

		/* Get default value */
		defaultValue=xfconf_channel_get_uint(priv->xfconfChannel, "/min-notification-timeout", DEFAULT_NOTIFICATION_TIMEOUT)/1000.0;

		/* Set up scaling settings of widget */
		adjustment=GTK_ADJUSTMENT(gtk_builder_get_object(priv->builder, "notification-timeout-adjustment"));
		gtk_range_set_adjustment(GTK_RANGE(priv->widgetNotificationTimeout), adjustment);

		/* Set up default value */
		gtk_range_set_value(GTK_RANGE(priv->widgetNotificationTimeout), defaultValue);

		/* Connect signals */
		g_signal_connect(priv->widgetNotificationTimeout,
							"format-value",
							G_CALLBACK(_xfdashboard_settings_on_format_notification_timeout_value),
							NULL);
		g_signal_connect_swapped(priv->widgetNotificationTimeout,
									"value-changed",
									G_CALLBACK(_xfdashboard_settings_widget_changed_notification_timeout),
									self);
		g_signal_connect_swapped(priv->xfconfChannel,
									"property-changed::/min-notification-timeout",
									G_CALLBACK(_xfdashboard_settings_xfconf_changed_notification_timeout),
									self);
	}

	priv->widgetEnableUnmappedWindowWorkaround=GTK_WIDGET(gtk_builder_get_object(priv->builder, "enable-unmapped-window-workaround"));
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(priv->widgetEnableUnmappedWindowWorkaround), FALSE);
	xfconf_g_property_bind(priv->xfconfChannel,
							"/enable-unmapped-window-workaround",
							G_TYPE_BOOLEAN,
							priv->widgetEnableUnmappedWindowWorkaround,
							"active");

	priv->widgetShowAllApps=GTK_WIDGET(gtk_builder_get_object(priv->builder, "show-all-apps"));
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(priv->widgetShowAllApps), FALSE);
	xfconf_g_property_bind(priv->xfconfChannel,
							"/components/applications-view/show-all-apps",
							G_TYPE_BOOLEAN,
							priv->widgetShowAllApps,
							"active");

	priv->widgetScrollEventChangedWorkspace=GTK_WIDGET(gtk_builder_get_object(priv->builder, "scroll-event-changes-workspace"));
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(priv->widgetScrollEventChangedWorkspace), FALSE);
	xfconf_g_property_bind(priv->xfconfChannel,
							"/components/windows-view/scroll-event-changes-workspace",
							G_TYPE_BOOLEAN,
							priv->widgetScrollEventChangedWorkspace,
							"active");

	priv->widgetCloseButton=GTK_WIDGET(gtk_builder_get_object(priv->builder, "close-button"));
	g_signal_connect_swapped(priv->widgetCloseButton,
								"clicked",
								G_CALLBACK(_xfdashboard_settings_on_close_clicked),
								self);

	/* Tab: Themes */
	priv->widgetThemeScreenshot=GTK_WIDGET(gtk_builder_get_object(priv->builder, "theme-screenshot"));
	priv->widgetThemeNameLabel=GTK_WIDGET(gtk_builder_get_object(priv->builder, "theme-name-label"));
	priv->widgetThemeName=GTK_WIDGET(gtk_builder_get_object(priv->builder, "theme-name"));
	priv->widgetThemeAuthorLabel=GTK_WIDGET(gtk_builder_get_object(priv->builder, "theme-author-label"));
	priv->widgetThemeAuthor=GTK_WIDGET(gtk_builder_get_object(priv->builder, "theme-author"));
	priv->widgetThemeVersionLabel=GTK_WIDGET(gtk_builder_get_object(priv->builder, "theme-version-label"));
	priv->widgetThemeVersion=GTK_WIDGET(gtk_builder_get_object(priv->builder, "theme-version"));
	priv->widgetThemeDescriptionLabel=GTK_WIDGET(gtk_builder_get_object(priv->builder, "theme-description-label"));
	priv->widgetThemeDescription=GTK_WIDGET(gtk_builder_get_object(priv->builder, "theme-description"));

	priv->widgetThemes=GTK_WIDGET(gtk_builder_get_object(priv->builder, "themes"));
	if(priv->widgetThemes)
	{
		gchar								*currentTheme;
		GValue								defaultValue=G_VALUE_INIT;
		GtkTreeSelection					*selection;
		GtkCellRenderer						*renderer;

		/* Get default value */
		currentTheme=xfconf_channel_get_string(priv->xfconfChannel, "/theme", DEFAULT_THEME);
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
													COLUMN_THEME_NAME,
													NULL);

		/* Ensure only one selection at time is possible */
		selection=gtk_tree_view_get_selection(GTK_TREE_VIEW(priv->widgetThemes));
		gtk_tree_selection_set_mode(selection, GTK_SELECTION_SINGLE);

		/* Populate list of available themes */
		_xfdashboard_settings_populate_themes_list(self, priv->widgetThemes);

		/* Select default value */
		_xfdashboard_settings_xfconf_changed_theme(self,
													"/theme",
													&defaultValue,
													priv->xfconfChannel);
		_xfdashboard_settings_widget_changed_theme(self, selection);

		/* Connect signals */
		g_signal_connect_swapped(selection,
									"changed",
									G_CALLBACK(_xfdashboard_settings_widget_changed_theme),
									self);
		g_signal_connect_swapped(priv->xfconfChannel,
									"property-changed::/theme",
									G_CALLBACK(_xfdashboard_settings_xfconf_changed_theme),
									self);

		/* Release allocated resources */
		g_value_unset(&defaultValue);
	}

	/* Release allocated resources */
	g_free(builderFile);
	g_object_unref(builder);

	/* Return success result */
	return(TRUE);
}

/* IMPLEMENTATION: GObject */

/* Dispose this object */
static void _xfdashboard_settings_dispose(GObject *inObject)
{
	XfdashboardSettings			*self=XFDASHBOARD_SETTINGS(inObject);
	XfdashboardSettingsPrivate	*priv=self->priv;

	/* Release allocated resouces */
	priv->widgetCloseButton=NULL;
	priv->widgetResetSearchOnResume=NULL;
	priv->widgetSwitchViewOnResume=NULL;
	priv->widgetNotificationTimeout=NULL;
	priv->widgetEnableUnmappedWindowWorkaround=NULL;
	priv->widgetScrollEventChangedWorkspace=NULL;
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

	if(priv->xfconfChannel)
	{
		// TODO: xfconf_g_property_unbind_all(priv->xfconfChannel);
		priv->xfconfChannel=NULL;
	}

	if(priv->builder)
	{
		g_object_unref(priv->builder);
		priv->builder=NULL;
	}

	/* Call parent's class dispose method */
	G_OBJECT_CLASS(xfdashboard_settings_parent_class)->dispose(inObject);
}

/* Class initialization
 * Override functions in parent classes and define properties
 * and signals
 */
static void xfdashboard_settings_class_init(XfdashboardSettingsClass *klass)
{
	GObjectClass		*gobjectClass=G_OBJECT_CLASS(klass);

	/* Override functions */
	gobjectClass->dispose=_xfdashboard_settings_dispose;

	/* Set up private structure */
	g_type_class_add_private(klass, sizeof(XfdashboardSettingsPrivate));
}

/* Object initialization
 * Create private structure and set up default values
 */
static void xfdashboard_settings_init(XfdashboardSettings *self)
{
	XfdashboardSettingsPrivate	*priv;

	priv=self->priv=XFDASHBOARD_SETTINGS_GET_PRIVATE(self);

	/* Set default values */
	priv->xfconfChannel=xfconf_channel_get(XFDASHBOARD_XFCONF_CHANNEL);

	priv->builder=NULL;
	priv->widgetCloseButton=NULL;
	priv->widgetResetSearchOnResume=NULL;
	priv->widgetSwitchViewOnResume=NULL;
	priv->widgetNotificationTimeout=NULL;
	priv->widgetEnableUnmappedWindowWorkaround=NULL;
	priv->widgetScrollEventChangedWorkspace=NULL;
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
XfdashboardSettings* xfdashboard_settings_new(void)
{
	return(XFDASHBOARD_SETTINGS(g_object_new(XFDASHBOARD_TYPE_SETTINGS, NULL)));
}

/* Create standalone dialog for this settings instance */
GtkWidget* xfdashboard_settings_create_dialog(XfdashboardSettings *self)
{
	XfdashboardSettingsPrivate	*priv;
	GObject						*dialog;

	g_return_val_if_fail(XFDASHBOARD_IS_SETTINGS(self), NULL);

	priv=self->priv;

	/* Get builder if not available */
	if(!_xfdashboard_settings_create_builder(self))
	{
		/* An critical error message should be displayed so just return NULL */
		return(NULL);
	}

	/* Get dialog object */
	dialog=gtk_builder_get_object(priv->builder, "preferences-dialog");
	if(!dialog)
	{
		g_critical(_("Could not get dialog from UI file."));
		return(NULL);
	}

	/* Return widget */
	return(GTK_WIDGET(dialog));
}

/* Create "pluggable" dialog for this settings instance */
GtkWidget* xfdashboard_settings_create_plug(XfdashboardSettings *self, Window inSocketID)
{
	XfdashboardSettingsPrivate	*priv;
	GtkWidget					*plug;
	GObject						*dialogChild;
#if GTK_CHECK_VERSION(3, 14 ,0)
	GtkWidget					*dialogParent;
#endif

	g_return_val_if_fail(XFDASHBOARD_IS_SETTINGS(self), NULL);
	g_return_val_if_fail(inSocketID, NULL);

	priv=self->priv;

	/* Get builder if not available */
	if(!_xfdashboard_settings_create_builder(self))
	{
		/* An critical error message should be displayed so just return NULL */
		return(NULL);
	}

	/* Get dialog object */
	dialogChild=gtk_builder_get_object(priv->builder, "preferences-plug-child");
	if(!dialogChild)
	{
		g_critical(_("Could not get dialog from UI file."));
		return(NULL);
	}

	/* Create plug widget and reparent dialog object to it */
	plug=gtk_plug_new(inSocketID);
#if GTK_CHECK_VERSION(3, 14 ,0)
	g_object_ref(G_OBJECT(dialogChild));

	dialogParent=gtk_widget_get_parent(GTK_WIDGET(dialogChild));
	gtk_container_remove(GTK_CONTAINER(dialogParent), GTK_WIDGET(dialogChild));
	gtk_container_add(GTK_CONTAINER(plug), GTK_WIDGET(dialogChild));

	g_object_unref(G_OBJECT(dialogChild));
#else
	gtk_widget_reparent(GTK_WIDGET(dialogChild), plug);
#endif
	gtk_widget_show(GTK_WIDGET(dialogChild));

	/* Return widget */
	return(GTK_WIDGET(plug));
}
