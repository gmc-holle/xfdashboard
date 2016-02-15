/*
 * settings: Settings of application
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

#include "settings.h"

#include <glib/gi18n-lib.h>
#include <xfconf/xfconf.h>
#include <math.h>

#include "themes.h"


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
	XfconfChannel				*xfconfChannel;

	GtkBuilder					*builder;

	GtkWidget					*widgetCloseButton;
	GtkWidget					*widgetResetSearchOnResume;
	GtkWidget					*widgetSwitchViewOnResume;
	GtkWidget					*widgetNotificationTimeout;
	GtkWidget					*widgetEnableUnmappedWindowWorkaround;
	GtkWidget					*widgetWindowCreationPriority;
	GtkWidget					*widgetAlwaysLaunchNewInstance;
	GtkWidget					*widgetShowAllApps;
	GtkWidget					*widgetScrollEventChangedWorkspace;
	GtkWidget					*widgetDelaySearchTimeout;

	XfdashboardSettingsThemes	*themes;
};


/* IMPLEMENTATION: Private variables and methods */
#define XFDASHBOARD_XFCONF_CHANNEL					"xfdashboard"
#define PREFERENCES_UI_FILE							"preferences.ui"
#define XFDASHBOARD_THEME_SUBPATH					"xfdashboard-1.0"
#define XFDASHBOARD_THEME_FILE						"xfdashboard.theme"
#define XFDASHBOARD_THEME_GROUP						"Xfdashboard Theme"
#define DEFAULT_DELAY_SEARCH_TIMEOUT				0
#define DEFAULT_NOTIFICATION_TIMEOUT				3000
#define DEFAULT_RESET_SEARCH_ON_RESUME				TRUE
#define DEFAULT_SWITCH_VIEW_ON_RESUME				NULL
#define DEFAULT_THEME								"xfdashboard"
#define DEFAULT_ENABLE_HOTKEY						FALSE
#define DEFAULT_WINDOW_CONTENT_CREATION_PRIORITY	"immediate"
#define DEFAULT_LAUNCH_NEW_INSTANCE					TRUE
#define MAX_SCREENSHOT_WIDTH						400

typedef struct _XfdashboardSettingsResumableViews			XfdashboardSettingsResumableViews;
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

typedef struct _XfdashboardSettingsWindowContentPriority	XfdashboardSettingsWindowContentPriority;
struct _XfdashboardSettingsWindowContentPriority
{
	const gchar		*displayName;
	const gchar		*priorityName;
};

static XfdashboardSettingsWindowContentPriority	windowCreationPriorities[]=
{
	{ N_("Immediately"), "immediate", },
	{ N_("High"), "high"},
	{ N_("Normal"), "normal" },
	{ N_("Low"), "low" },
	{ NULL, NULL },
};


/* Setting '/switch-to-view-on-resume' changed either at widget or at xfconf property */
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
	xfconf_channel_set_string(priv->xfconfChannel, "/switch-to-view-on-resume", value);

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

/* Setting '/window-content-creation-priority' changed either at widget or at xfconf property */
static void _xfdashboard_settings_widget_changed_window_creation_priority(XfdashboardSettings *self, GtkComboBox *inComboBox)
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
	xfconf_channel_set_string(priv->xfconfChannel, "/window-content-creation-priority", value);

	/* Release allocated resources */
	if(value) g_free(value);
}

static void _xfdashboard_settings_xfconf_changed_window_creation_priority(XfdashboardSettings *self,
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
	if(G_UNLIKELY(G_VALUE_TYPE(inValue)!=G_TYPE_STRING)) newValue=DEFAULT_WINDOW_CONTENT_CREATION_PRIORITY;
		else newValue=g_value_get_string(inValue);

	/* Iterate through combo box value and set new value if match is found */
	model=gtk_combo_box_get_model(GTK_COMBO_BOX(priv->widgetWindowCreationPriority));
	if(gtk_tree_model_get_iter_first(model, &iter))
	{
		do
		{
			gtk_tree_model_get(model, &iter, 1, &value, -1);
			if(G_UNLIKELY(g_str_equal(value, newValue)))
			{
				g_free(value);
				gtk_combo_box_set_active_iter(GTK_COMBO_BOX(priv->widgetWindowCreationPriority), &iter);
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

/* Setting '/components/search-view/delay-search-timeout' changed either at widget or at xfconf property */
static void _xfdashboard_settings_widget_changed_delay_search_timeout(XfdashboardSettings *self, GtkRange *inRange)
{
	XfdashboardSettingsPrivate		*priv;
	guint							value;

	g_return_if_fail(XFDASHBOARD_IS_SETTINGS(self));
	g_return_if_fail(GTK_IS_RANGE(inRange));

	priv=self->priv;

	/* Get value from widget */
	value=floor(gtk_range_get_value(inRange));

	/* Set value at xfconf property */
	xfconf_channel_set_uint(priv->xfconfChannel, "/components/search-view/delay-search-timeout", value);
}

static void _xfdashboard_settings_xfconf_changed_delay_search_timeout(XfdashboardSettings *self,
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
	if(G_UNLIKELY(G_VALUE_TYPE(inValue)!=G_TYPE_UINT)) newValue=DEFAULT_DELAY_SEARCH_TIMEOUT;
		else newValue=g_value_get_uint(inValue);

	/* Set new value at widget */
	gtk_range_set_value(GTK_RANGE(priv->widgetDelaySearchTimeout), (gdouble)newValue);
}

/* Format value to show in delay search timeout slider */
static gchar* _xfdashboard_settings_on_format_delay_search_timeout_value(GtkScale *inWidget,
																			gdouble inValue,
																			gpointer inUserData)
{
	gchar		*text;

	if(inValue>0.0)
	{
		text=g_strdup_printf("%u %s", (guint)inValue, _("ms"));
	}
		else
		{
			text=g_strdup(_("Immediately"));
		}

	return(text);
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

	/* Search UI file in given environment variable if set.
	 * This makes development easier to test modifications at UI file.
	 */
	if(!builderFile)
	{
		const gchar		*envPath;

		envPath=g_getenv("XFDASHBOARD_UI_PATH");
		if(envPath)
		{
			builderFile=g_build_filename(envPath, PREFERENCES_UI_FILE, NULL);
			g_debug("Trying UI file: %s", builderFile);
			if(!g_file_test(builderFile, G_FILE_TEST_EXISTS | G_FILE_TEST_IS_REGULAR))
			{
				g_free(builderFile);
				builderFile=NULL;
			}
		}
	}

	/* Find UI file at install path */
	if(!builderFile)
	{
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


	priv->widgetSwitchViewOnResume=GTK_WIDGET(gtk_builder_get_object(priv->builder, "switch-to-view-on-resume"));
	if(priv->widgetSwitchViewOnResume)
	{
		GtkCellRenderer						*renderer;
		GtkListStore						*listStore;
		GtkTreeIter							listStoreIter;
		GtkTreeIter							*defaultListStoreIter;
		XfdashboardSettingsResumableViews	*iter;
		gchar								*defaultValue;

		/* Get default value from settings */
		defaultValue=xfconf_channel_get_string(priv->xfconfChannel, "/switch-to-view-on-resume", DEFAULT_SWITCH_VIEW_ON_RESUME);
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
									"property-changed::/switch-to-view-on-resume",
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

	priv->widgetAlwaysLaunchNewInstance=GTK_WIDGET(gtk_builder_get_object(priv->builder, "always-launch-new-instance"));
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(priv->widgetAlwaysLaunchNewInstance), DEFAULT_LAUNCH_NEW_INSTANCE);
	xfconf_g_property_bind(priv->xfconfChannel,
							"/always-launch-new-instance",
							G_TYPE_BOOLEAN,
							priv->widgetAlwaysLaunchNewInstance,
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

	priv->widgetDelaySearchTimeout=GTK_WIDGET(gtk_builder_get_object(priv->builder, "delay-search-timeout"));
	if(priv->widgetDelaySearchTimeout)
	{
		GtkAdjustment						*adjustment;
		gdouble								defaultValue;

		/* Get default value */
		defaultValue=xfconf_channel_get_uint(priv->xfconfChannel, "/components/search-view/delay-search-timeout", DEFAULT_DELAY_SEARCH_TIMEOUT);

		/* Set up scaling settings of widget */
		adjustment=GTK_ADJUSTMENT(gtk_builder_get_object(priv->builder, "delay-search-timeout-adjustment"));
		gtk_range_set_adjustment(GTK_RANGE(priv->widgetDelaySearchTimeout), adjustment);

		/* Set up default value */
		gtk_range_set_value(GTK_RANGE(priv->widgetDelaySearchTimeout), defaultValue);

		/* Connect signals */
		g_signal_connect(priv->widgetDelaySearchTimeout,
							"format-value",
							G_CALLBACK(_xfdashboard_settings_on_format_delay_search_timeout_value),
							NULL);
		g_signal_connect_swapped(priv->widgetDelaySearchTimeout,
									"value-changed",
									G_CALLBACK(_xfdashboard_settings_widget_changed_delay_search_timeout),
									self);
		g_signal_connect_swapped(priv->xfconfChannel,
									"property-changed::/components/search-view/delay-search-timeout",
									G_CALLBACK(_xfdashboard_settings_xfconf_changed_delay_search_timeout),
									self);
	}

	priv->widgetCloseButton=GTK_WIDGET(gtk_builder_get_object(priv->builder, "close-button"));
	g_signal_connect_swapped(priv->widgetCloseButton,
								"clicked",
								G_CALLBACK(_xfdashboard_settings_on_close_clicked),
								self);

	priv->widgetWindowCreationPriority=GTK_WIDGET(gtk_builder_get_object(priv->builder, "window-creation-priority"));
	if(priv->widgetWindowCreationPriority)
	{
		GtkCellRenderer								*renderer;
		GtkListStore								*listStore;
		GtkTreeIter									listStoreIter;
		GtkTreeIter									*defaultListStoreIter;
		XfdashboardSettingsWindowContentPriority	*iter;
		gchar										*defaultValue;

		/* Get default value from settings */
		defaultValue=xfconf_channel_get_string(priv->xfconfChannel, "/window-content-creation-priority", DEFAULT_WINDOW_CONTENT_CREATION_PRIORITY);
		if(!defaultValue) defaultValue=g_strdup(windowCreationPriorities[0].priorityName);

		/* Clear combo box */
		gtk_cell_layout_clear(GTK_CELL_LAYOUT(priv->widgetWindowCreationPriority));

		/* Set up renderer for combo box */
		renderer=gtk_cell_renderer_text_new();
		gtk_cell_layout_pack_start(GTK_CELL_LAYOUT(priv->widgetWindowCreationPriority), renderer, TRUE);
		gtk_cell_layout_add_attribute(GTK_CELL_LAYOUT(priv->widgetWindowCreationPriority), renderer, "text", 0);

		/* Set up list to show at combo box */
		defaultListStoreIter=NULL;
		listStore=gtk_list_store_new(2, G_TYPE_STRING, G_TYPE_STRING);
		for(iter=windowCreationPriorities; iter->displayName; ++iter)
		{
			gtk_list_store_append(listStore, &listStoreIter);
			gtk_list_store_set(listStore, &listStoreIter, 0, _(iter->displayName), 1, iter->priorityName, -1);
			if(!g_strcmp0(iter->priorityName, defaultValue))
			{
				defaultListStoreIter=gtk_tree_iter_copy(&listStoreIter);
			}
		}
		gtk_combo_box_set_model(GTK_COMBO_BOX(priv->widgetWindowCreationPriority), GTK_TREE_MODEL(listStore));
		g_object_unref(G_OBJECT(listStore));

		/* Set up default value */
		if(defaultListStoreIter)
		{
			gtk_combo_box_set_active_iter(GTK_COMBO_BOX(priv->widgetWindowCreationPriority), defaultListStoreIter);
			gtk_tree_iter_free(defaultListStoreIter);
			defaultListStoreIter=NULL;
		}

		/* Connect signals */
		g_signal_connect_swapped(priv->widgetWindowCreationPriority,
									"changed",
									G_CALLBACK(_xfdashboard_settings_widget_changed_window_creation_priority),
									self);
		g_signal_connect_swapped(priv->xfconfChannel,
									"property-changed::/window-content-creation-priority",
									G_CALLBACK(_xfdashboard_settings_xfconf_changed_window_creation_priority),
									self);

		/* Release allocated resources */
		if(defaultValue) g_free(defaultValue);
	}

	/* Tab: Themes */
	priv->themes=xfdashboard_settings_themes_new(builder);

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
	priv->widgetWindowCreationPriority=NULL;
	priv->widgetAlwaysLaunchNewInstance=NULL;
	priv->widgetScrollEventChangedWorkspace=NULL;
	priv->widgetDelaySearchTimeout=NULL;

	if(priv->themes)
	{
		g_object_unref(priv->themes);
		priv->themes=NULL;
	}

	if(priv->xfconfChannel)
	{
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
	priv->widgetWindowCreationPriority=NULL;
	priv->widgetAlwaysLaunchNewInstance=NULL;
	priv->widgetScrollEventChangedWorkspace=NULL;
	priv->widgetDelaySearchTimeout=NULL;
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
