/*
 * general: General settings of application
 * 
 * Copyright 2012-2020 Stephan Haller <nomad@froevel.de>
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

#include "settings.h"
#include <glib/gi18n-lib.h>
#include <math.h>


/* Define this class in GObject system */
struct _XfdashboardSettingsGeneralPrivate
{
	/* Properties related */
	GtkBuilder				*builder;
	XfdashboardSettings		*settings;

	/* Instance related */
	GtkWidget				*widgetResetSearchOnResume;
	GtkWidget				*widgetSwitchToViewOnResume;
	GtkWidget				*widgetMinNotificationTimeout;
	GtkWidget				*widgetEnableUnmappedWindowWorkaround;
	GtkWidget				*widgetWindowCreationPriority;
	GtkWidget				*widgetAlwaysLaunchNewInstance;
	GtkWidget				*widgetShowAllApps;
	GtkWidget				*widgetScrollEventChangesWorkspace;
	GtkWidget				*widgetDelaySearchTimeout;
	GtkWidget				*widgetAllowSubwindows;
	GtkWidget				*widgetEnableAnimations;
};

G_DEFINE_TYPE_WITH_PRIVATE(XfdashboardSettingsGeneral,
							xfdashboard_settings_general,
							G_TYPE_OBJECT)

/* Properties */
enum
{
	PROP_0,

	PROP_BUILDER,
	PROP_SETTINGS,

	PROP_LAST
};

static GParamSpec* XfdashboardSettingsGeneralProperties[PROP_LAST]={ 0, };


/* IMPLEMENTATION: Private variables and methods */
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
	xfdashboard_settings_set_switch_to_view_on_resume(priv->settings, value);

	/* Release allocated resources */
	if(value) g_free(value);
}

static void _xfdashboard_settings_general_switch_to_view_on_resume_changed_by_settings(XfdashboardSettingsGeneral *self,
																						GParamSpec *inParamSpec,
																						GObject *inObject)
{
	XfdashboardSettingsGeneralPrivate		*priv;
	GtkTreeModel							*model;
	GtkTreeIter								iter;
	gchar									*value;
	const gchar								*newValue;

	g_return_if_fail(XFDASHBOARD_IS_SETTINGS_GENERAL(self));

	priv=self->priv;

	/* Get new value to lookup and set at combo box */
	newValue=xfdashboard_settings_get_switch_to_view_on_resume(priv->settings);

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
	xfdashboard_settings_set_window_content_creation_priority(priv->settings, value);

	/* Release allocated resources */
	if(value) g_free(value);
}

static void _xfdashboard_settings_general_window_creation_priority_changed_by_settings(XfdashboardSettingsGeneral *self,
																						GParamSpec *inParamSpec,
																						GObject *inObject)
{
	XfdashboardSettingsGeneralPrivate		*priv;
	GtkTreeModel							*model;
	GtkTreeIter								iter;
	gchar									*value;
	const gchar								*newValue;

	g_return_if_fail(XFDASHBOARD_IS_SETTINGS_GENERAL(self));

	priv=self->priv;

	/* Get new value to lookup and set at combo box */
	newValue=xfdashboard_settings_get_window_content_creation_priority(priv->settings);

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
	xfdashboard_settings_set_notification_timeout(priv->settings, value);
}

static void _xfdashboard_settings_general_notification_timeout_changed_by_settings(XfdashboardSettingsGeneral *self,
																					GParamSpec *inParamSpec,
																					GObject *inObject)
{
	XfdashboardSettingsGeneralPrivate		*priv;
	guint									newValue;

	g_return_if_fail(XFDASHBOARD_IS_SETTINGS_GENERAL(self));

	priv=self->priv;

	/* Get new value to set at widget */
	newValue=xfdashboard_settings_get_notification_timeout(priv->settings);

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
	xfdashboard_settings_set_delay_search_timeout(priv->settings, value);
}

static void _xfdashboard_settings_general_delay_search_timeout_changed_by_settings(XfdashboardSettingsGeneral *self,
																					GParamSpec *inParamSpec,
																					GObject *inObject)
{
	XfdashboardSettingsGeneralPrivate		*priv;
	guint									newValue;

	g_return_if_fail(XFDASHBOARD_IS_SETTINGS_GENERAL(self));

	priv=self->priv;

	/* Get new value to set at widget */
	newValue=xfdashboard_settings_get_delay_search_timeout(priv->settings);

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

/* Set up this tab */
static void _xfdashboard_settings_general_setup(XfdashboardSettingsGeneral *self)
{
	XfdashboardSettingsGeneralPrivate				*priv;
	static gboolean									setupDone=FALSE;

	g_return_if_fail(XFDASHBOARD_IS_SETTINGS_GENERAL(self));

	priv=self->priv;

	/* Do nothing if builder or settings is not set yet */
	if(!priv->settings || !priv->builder) return;

	/* Do nothing if set up was done already */
	if(setupDone) return;

	/* Mark that set up is done */
	setupDone=TRUE;

	/* Get widgets from builder */
	priv->widgetResetSearchOnResume=GTK_WIDGET(gtk_builder_get_object(priv->builder, "reset-search-on-resume"));
	g_object_bind_property(priv->settings,
							"reset-search-on-resume",
							priv->widgetResetSearchOnResume,
							"active",
							G_BINDING_BIDIRECTIONAL | G_BINDING_SYNC_CREATE);

	priv->widgetSwitchToViewOnResume=GTK_WIDGET(gtk_builder_get_object(priv->builder, "switch-to-view-on-resume"));
	if(priv->widgetSwitchToViewOnResume)
	{
		GtkCellRenderer								*renderer;
		GtkListStore								*listStore;
		GtkTreeIter									listStoreIter;
		GtkTreeIter									*defaultListStoreIter;
		XfdashboardSettingsGeneralNameValuePair		*iter;
		const gchar									*defaultValue;

		/* Get default value from settings */
		defaultValue=xfdashboard_settings_get_switch_to_view_on_resume(priv->settings);

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
		g_signal_connect_swapped(priv->settings,
									"notify::switch-to-view-on-resume",
									G_CALLBACK(_xfdashboard_settings_general_switch_to_view_on_resume_changed_by_settings),
									self);
	}

	priv->widgetMinNotificationTimeout=GTK_WIDGET(gtk_builder_get_object(priv->builder, "notification-timeout"));
	if(priv->widgetMinNotificationTimeout)
	{
		GtkAdjustment								*adjustment;
		gdouble										defaultValue;

		/* Get default value */
		defaultValue=xfdashboard_settings_get_notification_timeout(priv->settings)/1000.0;

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
		g_signal_connect_swapped(priv->settings,
									"notify::min-notification-timeout",
									G_CALLBACK(_xfdashboard_settings_general_notification_timeout_changed_by_settings),
									self);
	}

	priv->widgetEnableUnmappedWindowWorkaround=GTK_WIDGET(gtk_builder_get_object(priv->builder, "enable-unmapped-window-workaround"));
	g_object_bind_property(priv->settings,
							"enable-unmapped-window-workaround",
							priv->widgetEnableUnmappedWindowWorkaround,
							"active",
							G_BINDING_BIDIRECTIONAL | G_BINDING_SYNC_CREATE);

	priv->widgetAlwaysLaunchNewInstance=GTK_WIDGET(gtk_builder_get_object(priv->builder, "always-launch-new-instance"));
	g_object_bind_property(priv->settings,
							"always-launch-new-instance",
							priv->widgetAlwaysLaunchNewInstance,
							"active",
							G_BINDING_BIDIRECTIONAL | G_BINDING_SYNC_CREATE);

	priv->widgetShowAllApps=GTK_WIDGET(gtk_builder_get_object(priv->builder, "show-all-apps"));
	g_object_bind_property(priv->settings,
							"show-all-applications",
							priv->widgetShowAllApps,
							"active",
							G_BINDING_BIDIRECTIONAL | G_BINDING_SYNC_CREATE);

	priv->widgetScrollEventChangesWorkspace=GTK_WIDGET(gtk_builder_get_object(priv->builder, "scroll-event-changes-workspace"));
	g_object_bind_property(priv->settings,
							"scroll-event-changes-workspace",
							priv->widgetScrollEventChangesWorkspace,
							"active",
							G_BINDING_BIDIRECTIONAL | G_BINDING_SYNC_CREATE);

	priv->widgetDelaySearchTimeout=GTK_WIDGET(gtk_builder_get_object(priv->builder, "delay-search-timeout"));
	if(priv->widgetDelaySearchTimeout)
	{
		GtkAdjustment								*adjustment;
		gdouble										defaultValue;

		/* Get default value */
		defaultValue=xfdashboard_settings_get_delay_search_timeout(priv->settings);

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
		g_signal_connect_swapped(priv->settings,
									"notify::delay-search-timeout",
									G_CALLBACK(_xfdashboard_settings_general_delay_search_timeout_changed_by_settings),
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
		const gchar									*defaultValue;

		/* Get default value from settings */
		defaultValue=xfdashboard_settings_get_window_content_creation_priority(priv->settings);

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
		g_signal_connect_swapped(priv->settings,
									"notify::window-content-creation-priority",
									G_CALLBACK(_xfdashboard_settings_general_window_creation_priority_changed_by_settings),
									self);
	}

	priv->widgetAllowSubwindows=GTK_WIDGET(gtk_builder_get_object(priv->builder, "allow-subwindows"));
	g_object_bind_property(priv->settings,
							"allow-subwindows",
							priv->widgetAllowSubwindows,
							"active",
							G_BINDING_BIDIRECTIONAL | G_BINDING_SYNC_CREATE);

	priv->widgetEnableAnimations=GTK_WIDGET(gtk_builder_get_object(priv->builder, "enable-animations"));
	g_object_bind_property(priv->settings,
							"enable-animations",
							priv->widgetEnableAnimations,
							"active",
							G_BINDING_BIDIRECTIONAL | G_BINDING_SYNC_CREATE);
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

	/* If both builder and settings are set, then set up tab */
	_xfdashboard_settings_general_setup(self);
}

/* Set settings object instance */
static void _xfdashboard_settings_general_set_settings(XfdashboardSettingsGeneral *self,
														XfdashboardSettings *inSettings)
{
	XfdashboardSettingsGeneralPrivate				*priv;

	g_return_if_fail(XFDASHBOARD_IS_SETTINGS_GENERAL(self));
	g_return_if_fail(XFDASHBOARD_IS_SETTINGS(inSettings));

	priv=self->priv;

	/* Set settings object which must not be set yet */
	g_assert(!priv->settings);

	priv->settings=g_object_ref(inSettings);

	/* If both builder and settings are set, then set up tab */
	_xfdashboard_settings_general_setup(self);
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

	if(priv->settings)
	{
		g_object_unref(priv->settings);
		priv->settings=NULL;
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

		case PROP_SETTINGS:
			_xfdashboard_settings_general_set_settings(self, XFDASHBOARD_SETTINGS(g_value_get_object(inValue)));
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

		case PROP_SETTINGS:
			g_value_set_object(outValue, priv->settings);
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
								"Builder",
								"The initialized GtkBuilder object where to set up themes tab from",
								GTK_TYPE_BUILDER,
								G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY | G_PARAM_STATIC_STRINGS);

	XfdashboardSettingsGeneralProperties[PROP_SETTINGS]=
		g_param_spec_object("settings",
								"Settings",
								"The settings object of application",
								XFDASHBOARD_TYPE_SETTINGS,
								G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY | G_PARAM_STATIC_STRINGS);

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
	priv->settings=NULL;

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
XfdashboardSettingsGeneral* xfdashboard_settings_general_new(XfdashboardSettingsApp *inApp)
{
	GObject		*instance;

	g_return_val_if_fail(XFDASHBOARD_IS_SETTINGS_APP(inApp), NULL);

	/* Create instance */
	instance=g_object_new(XFDASHBOARD_TYPE_SETTINGS_GENERAL,
							"builder", xfdashboard_settings_app_get_builder(inApp),
							"settings", xfdashboard_settings_app_get_settings(inApp),
							NULL);

	/* Return newly created instance */
	return(XFDASHBOARD_SETTINGS_GENERAL(instance));
}
