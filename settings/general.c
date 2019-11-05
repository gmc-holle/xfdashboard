/*
 * general: General settings of application
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

#include "general.h"

#include <glib/gi18n-lib.h>
#include <xfconf/xfconf.h>
#include <math.h>


/* Define this class in GObject system */
struct _XfdashboardSettingsGeneralPrivate
{
	/* Properties related */
	GtkBuilder		*builder;

	/* Instance related */
	XfconfChannel	*xfconfChannel;

	GtkWidget		*widgetResetSearchOnResume;
	GtkWidget		*widgetSwitchToViewOnResume;
	GtkWidget		*widgetMinNotificationTimeout;
	GtkWidget		*widgetEnableUnmappedWindowWorkaround;
	GtkWidget		*widgetWindowCreationPriority;
	GtkWidget		*widgetAlwaysLaunchNewInstance;
	GtkWidget		*widgetShowAllApps;
	GtkWidget		*widgetScrollEventChangesWorkspace;
	GtkWidget		*widgetDelaySearchTimeout;
	GtkWidget		*widgetAllowSubwindows;
	GtkWidget		*widgetEnableAnimations;
};

G_DEFINE_TYPE_WITH_PRIVATE(XfdashboardSettingsGeneral,
							xfdashboard_settings_general,
							G_TYPE_OBJECT)

/* Properties */
enum
{
	PROP_0,

	PROP_BUILDER,

	PROP_LAST
};

static GParamSpec* XfdashboardSettingsGeneralProperties[PROP_LAST]={ 0, };


/* IMPLEMENTATION: Private variables and methods */
#define XFDASHBOARD_XFCONF_CHANNEL							"xfdashboard"

#define RESET_SEARCH_ON_RESUME_XFCONF_PROP					"/reset-search-on-resume"
#define DEFAULT_RESET_SEARCH_ON_RESUME						TRUE

#define SWITCH_TO_VIEW_ON_RESUME_XFCONF_PROP				"/switch-to-view-on-resume"
#define DEFAULT_SWITCH_TO_VIEW_ON_RESUME					NULL

#define MIN_NOTIFICATION_TIMEOUT_XFCONF_PROP				"/min-notification-timeout"
#define DEFAULT_MIN_NOTIFICATION_TIMEOUT					3000

#define ENABLE_UNMAPPED_WINDOW_WORKAROUND_XFCONF_PROP		"enable-unmapped-window-workaround"
#define DEFAULT_ENABLE_UNMAPPED_WINDOW_WORKAROUND			FALSE

#define ALWAYS_LAUNCH_NEW_INSTANCE							"/always-launch-new-instance"
#define DEFAULT_ALWAYS_LAUNCH_NEW_INSTANCE					TRUE

#define SHOW_ALL_APPS_XFCONF_PROP							"/components/applications-view/show-all-apps"
#define DEFAULT_SHOW_ALL_APPS								FALSE

#define SCROLL_EVENT_CHANGES_WORKSPACE_XFCONF_PROP			"/components/windows-view/scroll-event-changes-workspace"
#define DEFAULT_SCROLL_EVENT_CHANGES_WORKSPACE				FALSE

#define DELAY_SEARCH_TIMEOUT_XFCONF_PROP					"/components/search-view/delay-search-timeout"
#define DEFAULT_DELAY_SEARCH_TIMEOUT						0

#define WINDOW_CONTENT_CREATION_PRIORITY_XFCONF_PROP		"/window-content-creation-priority"
#define DEFAULT_WINDOW_CONTENT_CREATION_PRIORITY			"immediate"

#define ALLOW_SUBWINDOWS_XFCONF_PROP						"/allow-subwindows"
#define DEFAULT_ALLOW_SUBWINDOWS							TRUE

#define ENABLE_ANIMATIONS_XFCONF_PROP						"/enable-animations"
#define DEFAULT_ENABLE_ANIMATIONS							TRUE


typedef struct _XfdashboardSettingsGeneralNameValuePair		XfdashboardSettingsGeneralNameValuePair;
struct _XfdashboardSettingsGeneralNameValuePair
{
	const gchar		*displayName;
	const gchar		*value;
};

static XfdashboardSettingsGeneralNameValuePair				_xfdashboard_settings_general_resumable_views_values[]=
{
	{ N_("Do nothing"), NULL },
	{ N_("Windows view"), "builtin.windows" },
	{ N_("Applications view"), "builtin.applications" },
	{ NULL, NULL }
};

static XfdashboardSettingsGeneralNameValuePair				_xfdashboard_settings_general_window_creation_priorities_values[]=
{
	{ N_("Immediately"), "immediate", },
	{ N_("High"), "high"},
	{ N_("Normal"), "normal" },
	{ N_("Low"), "low" },
	{ NULL, NULL },
};


/* Setting '/switch-to-view-on-resume' changed either at widget or at xfconf property */
static void _xfdashboard_settings_general_switch_to_view_on_resume_changed_by_widget(XfdashboardSettingsGeneral *self,
																						GtkComboBox *inComboBox)
{
	XfdashboardSettingsGeneralPrivate		*priv;
	GtkTreeModel							*model;
	GtkTreeIter								iter;
	gchar									*value;

	g_return_if_fail(XFDASHBOARD_IS_SETTINGS_GENERAL(self));
	g_return_if_fail(GTK_IS_COMBO_BOX(inComboBox));

	priv=self->priv;

	/* Get selected entry from combo box */
	model=gtk_combo_box_get_model(inComboBox);
	gtk_combo_box_get_active_iter(inComboBox, &iter);
	gtk_tree_model_get(model, &iter, 1, &value, -1);

	/* Set value at xfconf property */
	if(value)
	{
		xfconf_channel_set_string(priv->xfconfChannel, SWITCH_TO_VIEW_ON_RESUME_XFCONF_PROP, value);
	}
		else
		{
			xfconf_channel_reset_property(priv->xfconfChannel, SWITCH_TO_VIEW_ON_RESUME_XFCONF_PROP, FALSE);
		}

	/* Release allocated resources */
	if(value) g_free(value);
}

static void _xfdashboard_settings_general_switch_to_view_on_resume_changed_by_xfconf(XfdashboardSettingsGeneral *self,
																						const gchar *inProperty,
																						const GValue *inValue,
																						XfconfChannel *inChannel)
{
	XfdashboardSettingsGeneralPrivate		*priv;
	GtkTreeModel							*model;
	GtkTreeIter								iter;
	gchar									*value;
	const gchar								*newValue;

	g_return_if_fail(XFDASHBOARD_IS_SETTINGS_GENERAL(self));
	g_return_if_fail(inValue);
	g_return_if_fail(XFCONF_IS_CHANNEL(inChannel));

	priv=self->priv;

	/* Get new value to lookup and set at combo box */
	if(G_UNLIKELY(G_VALUE_TYPE(inValue)!=G_TYPE_STRING)) newValue=NULL;
		else newValue=g_value_get_string(inValue);

	/* Iterate through combo box value and set new value if match is found */
	model=gtk_combo_box_get_model(GTK_COMBO_BOX(priv->widgetSwitchToViewOnResume));
	if(gtk_tree_model_get_iter_first(model, &iter))
	{
		do
		{
			gtk_tree_model_get(model, &iter, 1, &value, -1);
			if(G_UNLIKELY((!newValue && !value)) ||
				G_UNLIKELY(!g_strcmp0(value, newValue)))
			{
				g_free(value);
				gtk_combo_box_set_active_iter(GTK_COMBO_BOX(priv->widgetSwitchToViewOnResume), &iter);
				break;
			}
			g_free(value);
		}
		while(gtk_tree_model_iter_next(model, &iter));
	}
}

/* Setting '/window-content-creation-priority' changed either at widget or at xfconf property */
static void _xfdashboard_settings_general_window_creation_priority_changed_by_widget(XfdashboardSettingsGeneral *self,
																						GtkComboBox *inComboBox)
{
	XfdashboardSettingsGeneralPrivate		*priv;
	GtkTreeModel							*model;
	GtkTreeIter								iter;
	gchar									*value;

	g_return_if_fail(XFDASHBOARD_IS_SETTINGS_GENERAL(self));
	g_return_if_fail(GTK_IS_COMBO_BOX(inComboBox));

	priv=self->priv;

	/* Get selected entry from combo box */
	model=gtk_combo_box_get_model(inComboBox);
	gtk_combo_box_get_active_iter(inComboBox, &iter);
	gtk_tree_model_get(model, &iter, 1, &value, -1);

	/* Set value at xfconf property */
	xfconf_channel_set_string(priv->xfconfChannel, WINDOW_CONTENT_CREATION_PRIORITY_XFCONF_PROP, value);

	/* Release allocated resources */
	if(value) g_free(value);
}

static void _xfdashboard_settings_general_window_creation_priority_changed_by_xfconf(XfdashboardSettingsGeneral *self,
																						const gchar *inProperty,
																						const GValue *inValue,
																						XfconfChannel *inChannel)
{
	XfdashboardSettingsGeneralPrivate		*priv;
	GtkTreeModel							*model;
	GtkTreeIter								iter;
	gchar									*value;
	const gchar								*newValue;

	g_return_if_fail(XFDASHBOARD_IS_SETTINGS_GENERAL(self));
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
static void _xfdashboard_settings_general_notification_timeout_changed_by_widget(XfdashboardSettingsGeneral *self,
																					GtkRange *inRange)
{
	XfdashboardSettingsGeneralPrivate		*priv;
	guint									value;

	g_return_if_fail(XFDASHBOARD_IS_SETTINGS_GENERAL(self));
	g_return_if_fail(GTK_IS_RANGE(inRange));

	priv=self->priv;

	/* Get value from widget */
	value=floor(gtk_range_get_value(inRange)*1000);

	/* Set value at xfconf property */
	xfconf_channel_set_uint(priv->xfconfChannel, MIN_NOTIFICATION_TIMEOUT_XFCONF_PROP, value);
}

static void _xfdashboard_settings_general_notification_timeout_changed_by_xfconf(XfdashboardSettingsGeneral *self,
																					const gchar *inProperty,
																					const GValue *inValue,
																					XfconfChannel *inChannel)
{
	XfdashboardSettingsGeneralPrivate		*priv;
	guint									newValue;

	g_return_if_fail(XFDASHBOARD_IS_SETTINGS_GENERAL(self));
	g_return_if_fail(inValue);
	g_return_if_fail(XFCONF_IS_CHANNEL(inChannel));

	priv=self->priv;

	/* Get new value to set at widget */
	if(G_UNLIKELY(G_VALUE_TYPE(inValue)!=G_TYPE_UINT)) newValue=DEFAULT_MIN_NOTIFICATION_TIMEOUT;
		else newValue=g_value_get_uint(inValue);

	/* Set new value at widget */
	gtk_range_set_value(GTK_RANGE(priv->widgetMinNotificationTimeout), newValue/1000.0);
}

/* Format value to show in notification timeout slider */
static gchar* _xfdashboard_settings_general_on_format_notification_timeout_value(GtkScale *inWidget,
																					gdouble inValue,
																					gpointer inUserData)
{
	gchar		*text;

	text=g_strdup_printf("%.1f %s", inValue, _("seconds"));

	return(text);
}

/* Setting '/components/search-view/delay-search-timeout' changed either at widget or at xfconf property */
static void _xfdashboard_settings_general_delay_search_timeout_changed_by_widget(XfdashboardSettingsGeneral *self,
																					GtkRange *inRange)
{
	XfdashboardSettingsGeneralPrivate		*priv;
	guint									value;

	g_return_if_fail(XFDASHBOARD_IS_SETTINGS_GENERAL(self));
	g_return_if_fail(GTK_IS_RANGE(inRange));

	priv=self->priv;

	/* Get value from widget */
	value=floor(gtk_range_get_value(inRange));

	/* Set value at xfconf property */
	xfconf_channel_set_uint(priv->xfconfChannel, DELAY_SEARCH_TIMEOUT_XFCONF_PROP, value);
}

static void _xfdashboard_settings_general_delay_search_timeout_changed_by_xfconf(XfdashboardSettingsGeneral *self,
																					const gchar *inProperty,
																					const GValue *inValue,
																					XfconfChannel *inChannel)
{
	XfdashboardSettingsGeneralPrivate		*priv;
	guint									newValue;

	g_return_if_fail(XFDASHBOARD_IS_SETTINGS_GENERAL(self));
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
static gchar* _xfdashboard_settings_general_on_format_delay_search_timeout_value(GtkScale *inWidget,
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

/* Create and set up GtkBuilder */
static void _xfdashboard_settings_general_set_builder(XfdashboardSettingsGeneral *self,
														GtkBuilder *inBuilder)
{
	XfdashboardSettingsGeneralPrivate				*priv;

	g_return_if_fail(XFDASHBOARD_IS_SETTINGS_GENERAL(self));
	g_return_if_fail(GTK_IS_BUILDER(inBuilder));

	priv=self->priv;

	/* Set builder object which must not be set yet */
	g_assert(!priv->builder);

	priv->builder=g_object_ref(inBuilder);

	/* Get widgets from builder */
	priv->widgetResetSearchOnResume=GTK_WIDGET(gtk_builder_get_object(priv->builder, "reset-search-on-resume"));
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(priv->widgetResetSearchOnResume), DEFAULT_RESET_SEARCH_ON_RESUME);
	xfconf_g_property_bind(priv->xfconfChannel,
							RESET_SEARCH_ON_RESUME_XFCONF_PROP,
							G_TYPE_BOOLEAN,
							priv->widgetResetSearchOnResume,
							"active");

	priv->widgetSwitchToViewOnResume=GTK_WIDGET(gtk_builder_get_object(priv->builder, "switch-to-view-on-resume"));
	if(priv->widgetSwitchToViewOnResume)
	{
		GtkCellRenderer								*renderer;
		GtkListStore								*listStore;
		GtkTreeIter									listStoreIter;
		GtkTreeIter									*defaultListStoreIter;
		XfdashboardSettingsGeneralNameValuePair		*iter;
		gchar										*defaultValue;

		/* Get default value from settings */
		defaultValue=xfconf_channel_get_string(priv->xfconfChannel, SWITCH_TO_VIEW_ON_RESUME_XFCONF_PROP, DEFAULT_SWITCH_TO_VIEW_ON_RESUME);

		/* Clear combo box */
		gtk_cell_layout_clear(GTK_CELL_LAYOUT(priv->widgetSwitchToViewOnResume));

		/* Set up renderer for combo box */
		renderer=gtk_cell_renderer_text_new();
		gtk_cell_layout_pack_start(GTK_CELL_LAYOUT(priv->widgetSwitchToViewOnResume), renderer, TRUE);
		gtk_cell_layout_add_attribute(GTK_CELL_LAYOUT(priv->widgetSwitchToViewOnResume), renderer, "text", 0);

		/* Set up list to show at combo box */
		defaultListStoreIter=NULL;
		listStore=gtk_list_store_new(2, G_TYPE_STRING, G_TYPE_STRING);
		for(iter=_xfdashboard_settings_general_resumable_views_values; iter->displayName; ++iter)
		{
			gtk_list_store_append(listStore, &listStoreIter);
			gtk_list_store_set(listStore, &listStoreIter, 0, _(iter->displayName), 1, iter->value, -1);
			if((!defaultValue && !iter->value) ||
				!g_strcmp0(iter->value, defaultValue))
			{
				defaultListStoreIter=gtk_tree_iter_copy(&listStoreIter);
			}
		}
		gtk_combo_box_set_model(GTK_COMBO_BOX(priv->widgetSwitchToViewOnResume), GTK_TREE_MODEL(listStore));
		g_object_unref(G_OBJECT(listStore));

		/* Set up default value */
		if(defaultListStoreIter)
		{
			gtk_combo_box_set_active_iter(GTK_COMBO_BOX(priv->widgetSwitchToViewOnResume), defaultListStoreIter);
			gtk_tree_iter_free(defaultListStoreIter);
			defaultListStoreIter=NULL;
		}

		/* Connect signals */
		g_signal_connect_swapped(priv->widgetSwitchToViewOnResume,
									"changed",
									G_CALLBACK(_xfdashboard_settings_general_switch_to_view_on_resume_changed_by_widget),
									self);
		g_signal_connect_swapped(priv->xfconfChannel,
									"property-changed::"SWITCH_TO_VIEW_ON_RESUME_XFCONF_PROP,
									G_CALLBACK(_xfdashboard_settings_general_switch_to_view_on_resume_changed_by_xfconf),
									self);

		/* Release allocated resources */
		if(defaultValue) g_free(defaultValue);
	}

	priv->widgetMinNotificationTimeout=GTK_WIDGET(gtk_builder_get_object(priv->builder, "notification-timeout"));
	if(priv->widgetMinNotificationTimeout)
	{
		GtkAdjustment								*adjustment;
		gdouble										defaultValue;

		/* Get default value */
		defaultValue=xfconf_channel_get_uint(priv->xfconfChannel, MIN_NOTIFICATION_TIMEOUT_XFCONF_PROP, DEFAULT_MIN_NOTIFICATION_TIMEOUT)/1000.0;

		/* Set up scaling settings of widget */
		adjustment=GTK_ADJUSTMENT(gtk_builder_get_object(priv->builder, "notification-timeout-adjustment"));
		gtk_range_set_adjustment(GTK_RANGE(priv->widgetMinNotificationTimeout), adjustment);

		/* Set up default value */
		gtk_range_set_value(GTK_RANGE(priv->widgetMinNotificationTimeout), defaultValue);

		/* Connect signals */
		g_signal_connect(priv->widgetMinNotificationTimeout,
							"format-value",
							G_CALLBACK(_xfdashboard_settings_general_on_format_notification_timeout_value),
							NULL);
		g_signal_connect_swapped(priv->widgetMinNotificationTimeout,
									"value-changed",
									G_CALLBACK(_xfdashboard_settings_general_notification_timeout_changed_by_widget),
									self);
		g_signal_connect_swapped(priv->xfconfChannel,
									"property-changed::"MIN_NOTIFICATION_TIMEOUT_XFCONF_PROP,
									G_CALLBACK(_xfdashboard_settings_general_notification_timeout_changed_by_xfconf),
									self);
	}

	priv->widgetEnableUnmappedWindowWorkaround=GTK_WIDGET(gtk_builder_get_object(priv->builder, ENABLE_UNMAPPED_WINDOW_WORKAROUND_XFCONF_PROP));
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(priv->widgetEnableUnmappedWindowWorkaround), DEFAULT_ENABLE_UNMAPPED_WINDOW_WORKAROUND);
	xfconf_g_property_bind(priv->xfconfChannel,
							"/enable-unmapped-window-workaround",
							G_TYPE_BOOLEAN,
							priv->widgetEnableUnmappedWindowWorkaround,
							"active");

	priv->widgetAlwaysLaunchNewInstance=GTK_WIDGET(gtk_builder_get_object(priv->builder, "always-launch-new-instance"));
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(priv->widgetAlwaysLaunchNewInstance), DEFAULT_ALWAYS_LAUNCH_NEW_INSTANCE);
	xfconf_g_property_bind(priv->xfconfChannel,
							ALWAYS_LAUNCH_NEW_INSTANCE,
							G_TYPE_BOOLEAN,
							priv->widgetAlwaysLaunchNewInstance,
							"active");

	priv->widgetShowAllApps=GTK_WIDGET(gtk_builder_get_object(priv->builder, "show-all-apps"));
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(priv->widgetShowAllApps), DEFAULT_SHOW_ALL_APPS);
	xfconf_g_property_bind(priv->xfconfChannel,
							SHOW_ALL_APPS_XFCONF_PROP,
							G_TYPE_BOOLEAN,
							priv->widgetShowAllApps,
							"active");

	priv->widgetScrollEventChangesWorkspace=GTK_WIDGET(gtk_builder_get_object(priv->builder, "scroll-event-changes-workspace"));
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(priv->widgetScrollEventChangesWorkspace), DEFAULT_SCROLL_EVENT_CHANGES_WORKSPACE);
	xfconf_g_property_bind(priv->xfconfChannel,
							SCROLL_EVENT_CHANGES_WORKSPACE_XFCONF_PROP,
							G_TYPE_BOOLEAN,
							priv->widgetScrollEventChangesWorkspace,
							"active");

	priv->widgetDelaySearchTimeout=GTK_WIDGET(gtk_builder_get_object(priv->builder, "delay-search-timeout"));
	if(priv->widgetDelaySearchTimeout)
	{
		GtkAdjustment								*adjustment;
		gdouble										defaultValue;

		/* Get default value */
		defaultValue=xfconf_channel_get_uint(priv->xfconfChannel, DELAY_SEARCH_TIMEOUT_XFCONF_PROP, DEFAULT_DELAY_SEARCH_TIMEOUT);

		/* Set up scaling settings of widget */
		adjustment=GTK_ADJUSTMENT(gtk_builder_get_object(priv->builder, "delay-search-timeout-adjustment"));
		gtk_range_set_adjustment(GTK_RANGE(priv->widgetDelaySearchTimeout), adjustment);

		/* Set up default value */
		gtk_range_set_value(GTK_RANGE(priv->widgetDelaySearchTimeout), defaultValue);

		/* Connect signals */
		g_signal_connect(priv->widgetDelaySearchTimeout,
							"format-value",
							G_CALLBACK(_xfdashboard_settings_general_on_format_delay_search_timeout_value),
							NULL);
		g_signal_connect_swapped(priv->widgetDelaySearchTimeout,
									"value-changed",
									G_CALLBACK(_xfdashboard_settings_general_delay_search_timeout_changed_by_widget),
									self);
		g_signal_connect_swapped(priv->xfconfChannel,
									"property-changed::"DELAY_SEARCH_TIMEOUT_XFCONF_PROP,
									G_CALLBACK(_xfdashboard_settings_general_delay_search_timeout_changed_by_xfconf),
									self);
	}

	priv->widgetWindowCreationPriority=GTK_WIDGET(gtk_builder_get_object(priv->builder, "window-creation-priority"));
	if(priv->widgetWindowCreationPriority)
	{
		GtkCellRenderer								*renderer;
		GtkListStore								*listStore;
		GtkTreeIter									listStoreIter;
		GtkTreeIter									*defaultListStoreIter;
		XfdashboardSettingsGeneralNameValuePair		*iter;
		gchar										*defaultValue;

		/* Get default value from settings */
		defaultValue=xfconf_channel_get_string(priv->xfconfChannel, WINDOW_CONTENT_CREATION_PRIORITY_XFCONF_PROP, DEFAULT_WINDOW_CONTENT_CREATION_PRIORITY);
		if(!defaultValue) defaultValue=g_strdup(_xfdashboard_settings_general_window_creation_priorities_values[0].value);

		/* Clear combo box */
		gtk_cell_layout_clear(GTK_CELL_LAYOUT(priv->widgetWindowCreationPriority));

		/* Set up renderer for combo box */
		renderer=gtk_cell_renderer_text_new();
		gtk_cell_layout_pack_start(GTK_CELL_LAYOUT(priv->widgetWindowCreationPriority), renderer, TRUE);
		gtk_cell_layout_add_attribute(GTK_CELL_LAYOUT(priv->widgetWindowCreationPriority), renderer, "text", 0);

		/* Set up list to show at combo box */
		defaultListStoreIter=NULL;
		listStore=gtk_list_store_new(2, G_TYPE_STRING, G_TYPE_STRING);
		for(iter=_xfdashboard_settings_general_window_creation_priorities_values; iter->displayName; ++iter)
		{
			gtk_list_store_append(listStore, &listStoreIter);
			gtk_list_store_set(listStore, &listStoreIter, 0, _(iter->displayName), 1, iter->value, -1);
			if(!g_strcmp0(iter->value, defaultValue))
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
									G_CALLBACK(_xfdashboard_settings_general_window_creation_priority_changed_by_widget),
									self);
		g_signal_connect_swapped(priv->xfconfChannel,
									"property-changed::"WINDOW_CONTENT_CREATION_PRIORITY_XFCONF_PROP,
									G_CALLBACK(_xfdashboard_settings_general_window_creation_priority_changed_by_xfconf),
									self);

		/* Release allocated resources */
		if(defaultValue) g_free(defaultValue);
	}

	priv->widgetAllowSubwindows=GTK_WIDGET(gtk_builder_get_object(priv->builder, "allow-subwindows"));
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(priv->widgetAllowSubwindows), DEFAULT_ALLOW_SUBWINDOWS);
	xfconf_g_property_bind(priv->xfconfChannel,
							ALLOW_SUBWINDOWS_XFCONF_PROP,
							G_TYPE_BOOLEAN,
							priv->widgetAllowSubwindows,
							"active");

	priv->widgetEnableAnimations=GTK_WIDGET(gtk_builder_get_object(priv->builder, "enable-animations"));
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(priv->widgetEnableAnimations), DEFAULT_ENABLE_ANIMATIONS);
	xfconf_g_property_bind(priv->xfconfChannel,
							ENABLE_ANIMATIONS_XFCONF_PROP,
							G_TYPE_BOOLEAN,
							priv->widgetEnableAnimations,
							"active");
}

/* IMPLEMENTATION: GObject */

/* Dispose this object */
static void _xfdashboard_settings_general_dispose(GObject *inObject)
{
	XfdashboardSettingsGeneral			*self=XFDASHBOARD_SETTINGS_GENERAL(inObject);
	XfdashboardSettingsGeneralPrivate	*priv=self->priv;

	/* Release allocated resouces */
	priv->widgetResetSearchOnResume=NULL;
	priv->widgetSwitchToViewOnResume=NULL;
	priv->widgetMinNotificationTimeout=NULL;
	priv->widgetEnableUnmappedWindowWorkaround=NULL;
	priv->widgetWindowCreationPriority=NULL;
	priv->widgetAlwaysLaunchNewInstance=NULL;
	priv->widgetScrollEventChangesWorkspace=NULL;
	priv->widgetDelaySearchTimeout=NULL;

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
	G_OBJECT_CLASS(xfdashboard_settings_general_parent_class)->dispose(inObject);
}

/* Set/get properties */
static void _xfdashboard_settings_general_set_property(GObject *inObject,
														guint inPropID,
														const GValue *inValue,
														GParamSpec *inSpec)
{
	XfdashboardSettingsGeneral				*self=XFDASHBOARD_SETTINGS_GENERAL(inObject);

	switch(inPropID)
	{
		case PROP_BUILDER:
			_xfdashboard_settings_general_set_builder(self, GTK_BUILDER(g_value_get_object(inValue)));
			break;

		default:
			G_OBJECT_WARN_INVALID_PROPERTY_ID(inObject, inPropID, inSpec);
			break;
	}
}

static void _xfdashboard_settings_general_get_property(GObject *inObject,
														guint inPropID,
														GValue *outValue,
														GParamSpec *inSpec)
{
	XfdashboardSettingsGeneral				*self=XFDASHBOARD_SETTINGS_GENERAL(inObject);
	XfdashboardSettingsGeneralPrivate		*priv=self->priv;

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
static void xfdashboard_settings_general_class_init(XfdashboardSettingsGeneralClass *klass)
{
	GObjectClass		*gobjectClass=G_OBJECT_CLASS(klass);

	/* Override functions */
	gobjectClass->dispose=_xfdashboard_settings_general_dispose;
	gobjectClass->set_property=_xfdashboard_settings_general_set_property;
	gobjectClass->get_property=_xfdashboard_settings_general_get_property;

	/* Define properties */
	XfdashboardSettingsGeneralProperties[PROP_BUILDER]=
		g_param_spec_object("builder",
								_("Builder"),
								_("The initialized GtkBuilder object where to set up themes tab from"),
								GTK_TYPE_BUILDER,
								G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS | G_PARAM_CONSTRUCT_ONLY);

	g_object_class_install_properties(gobjectClass, PROP_LAST, XfdashboardSettingsGeneralProperties);
}

/* Object initialization
 * Create private structure and set up default values
 */
static void xfdashboard_settings_general_init(XfdashboardSettingsGeneral *self)
{
	XfdashboardSettingsGeneralPrivate	*priv;

	priv=self->priv=xfdashboard_settings_general_get_instance_private(self);

	/* Set default values */
	priv->builder=NULL;

	priv->xfconfChannel=xfconf_channel_get(XFDASHBOARD_XFCONF_CHANNEL);

	priv->widgetResetSearchOnResume=NULL;
	priv->widgetSwitchToViewOnResume=NULL;
	priv->widgetMinNotificationTimeout=NULL;
	priv->widgetEnableUnmappedWindowWorkaround=NULL;
	priv->widgetWindowCreationPriority=NULL;
	priv->widgetAlwaysLaunchNewInstance=NULL;
	priv->widgetScrollEventChangesWorkspace=NULL;
	priv->widgetDelaySearchTimeout=NULL;
}

/* IMPLEMENTATION: Public API */

/* Create instance of this class */
XfdashboardSettingsGeneral* xfdashboard_settings_general_new(GtkBuilder *inBuilder)
{
	GObject		*instance;

	g_return_val_if_fail(GTK_IS_BUILDER(inBuilder), NULL);

	/* Create instance */
	instance=g_object_new(XFDASHBOARD_TYPE_SETTINGS_GENERAL,
							"builder", inBuilder,
							NULL);

	/* Return newly created instance */
	return(XFDASHBOARD_SETTINGS_GENERAL(instance));
}
