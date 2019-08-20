/*
 * plugin: Plugin functions for 'hot-corner'
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
#include <config.h>
#endif

#include <libxfce4util/libxfce4util.h>
#include <gtk/gtk.h>

#include "hot-corner.h"
#include "hot-corner-settings.h"


/* Forward declarations */
G_MODULE_EXPORT void plugin_init(XfdashboardPlugin *self);


/* IMPLEMENTATION: XfdashboardPlugin */

#define CONFIGURATION_MAPPING		"xfdashboard-plugin-hot_corner-configuration-settings"

typedef struct _PluginWidgetSettingsMap		PluginWidgetSettingsMap;
struct _PluginWidgetSettingsMap
{
	XfdashboardHotCornerSettings	*settings;
	gchar							*property;
	guint							settingsPropertyChangedSignalID;
	GCallback						settingsPropertyCallback;
	GtkWidget						*widget;
};

typedef void (*PluginWidgetSettingsMapValueChangedCallback)(PluginWidgetSettingsMap *inMapping);

static XfdashboardHotCorner			*hotCorner=NULL;

/* Free mapping data */
static void _plugin_widget_settings_map_free(PluginWidgetSettingsMap *inData)
{
	g_return_if_fail(inData);

	/* Release allocated resources */
	if(inData->settingsPropertyChangedSignalID) g_signal_handler_disconnect(inData->settings, inData->settingsPropertyChangedSignalID);
	if(inData->property) g_free(inData->property);
	if(inData->settings) g_object_unref(inData->settings);
	g_free(inData);
}

/* A bound property at settings changed its value so call registered callback handler
 * for the widget.
 */
static void _plugin_on_widget_settings_map_settings_value_changed(GObject *inObject,
																	GParamSpec *inSpec,
																	gpointer inUserData)
{
	PluginWidgetSettingsMap							*mapping;

	g_return_if_fail(XFDASHBOARD_IS_HOT_CORNER_SETTINGS(inObject));
	g_return_if_fail(inUserData);

	mapping=(PluginWidgetSettingsMap*)inUserData;


	/* Call callback function */
	if(mapping->settingsPropertyCallback)
	{
		((PluginWidgetSettingsMapValueChangedCallback)mapping->settingsPropertyCallback)(mapping);
	}
}

/* Create mapping data and bind to widget */
static PluginWidgetSettingsMap* _plugin_widget_settings_map_bind(GtkWidget *inWidget,
																	XfdashboardHotCornerSettings *inSettings,
																	const gchar *inProperty,
																	GCallback inCallback)
{
	PluginWidgetSettingsMap			*mapping;
	gchar							*signalName;
	guint							signalID;

	g_return_val_if_fail(GTK_IS_WIDGET(inWidget), NULL);
	g_return_val_if_fail(XFDASHBOARD_IS_HOT_CORNER_SETTINGS(inSettings), NULL);
	g_return_val_if_fail(inProperty && *inProperty, NULL);

	/* Create new mapping */
	mapping=g_new0(PluginWidgetSettingsMap, 1);
	if(!mapping)
	{
		g_critical(_("Cannot allocate memory for mapping"));
		return(NULL);
	}

	/* Connect signal to get notified if value at settings changed */
	signalName=g_strdup_printf("notify::%s", inProperty);
	signalID=g_signal_connect(inSettings,
								signalName,
								G_CALLBACK(_plugin_on_widget_settings_map_settings_value_changed),
								mapping);
	g_free(signalName);

	/* Set up mapping */
	mapping->settings=g_object_ref(inSettings);
	mapping->property=g_strdup(inProperty);
	mapping->settingsPropertyChangedSignalID=signalID;
	mapping->settingsPropertyCallback=inCallback;
	mapping->widget=inWidget;

	/* Bind to widget */
	g_object_set_data_full(G_OBJECT(inWidget),
							CONFIGURATION_MAPPING,
							mapping,
							(GDestroyNotify)_plugin_widget_settings_map_free);

	/* Return mapping */
	return(mapping);
}

/* Value for activation corner was changed at widget */
static void _plugin_on_corner_widget_value_changed(GtkComboBox *inComboBox,
													gpointer inUserData,
													gpointer dummy1,
													gpointer dummy2)
{
	PluginWidgetSettingsMap			*mapping;
	GtkTreeModel					*model;
	GtkTreeIter						iter;
	gint							value;

	g_return_if_fail(GTK_IS_COMBO_BOX(inComboBox));
	g_return_if_fail(inUserData);

	mapping=(PluginWidgetSettingsMap*)inUserData;

	/* Get new value from widget */
	model=gtk_combo_box_get_model(inComboBox);
	gtk_combo_box_get_active_iter(inComboBox, &iter);
	gtk_tree_model_get(model, &iter, 1, &value, -1);

	/* Store new value at settings */
	xfdashboard_hot_corner_settings_set_activation_corner(mapping->settings, value);
}

/* Value for activation corner was changed at settings */
static void _plugin_on_corner_settings_value_changed(PluginWidgetSettingsMap *inMapping)
{
	XfdashboardHotCornerSettingsActivationCorner	value;
	guint											modelValue;
	GtkTreeModel									*model;
	GtkTreeIter										iter;

	g_return_if_fail(inMapping);

	/* Get new value from settings */
	value=xfdashboard_hot_corner_settings_get_activation_corner(inMapping->settings);

	/* Iterate through combo box value and set new value if match is found */
	model=gtk_combo_box_get_model(GTK_COMBO_BOX(inMapping->widget));
	if(gtk_tree_model_get_iter_first(model, &iter))
	{
		do
		{
			gtk_tree_model_get(model, &iter, 1, &modelValue, -1);
			if(G_UNLIKELY(modelValue==value))
			{
				gtk_combo_box_set_active_iter(GTK_COMBO_BOX(inMapping->widget), &iter);
				break;
			}
		}
		while(gtk_tree_model_iter_next(model, &iter));
	}
}

/* Value for activation radius was changed at widget */
static void _plugin_on_radius_widget_value_changed(GtkSpinButton *inButton,
													gpointer inUserData)
{
	PluginWidgetSettingsMap			*mapping;
	gint							value;

	g_return_if_fail(GTK_IS_SPIN_BUTTON(inButton));
	g_return_if_fail(inUserData);

	mapping=(PluginWidgetSettingsMap*)inUserData;

	/* Get new value from widget */
	value=gtk_spin_button_get_value_as_int(inButton);

	/* Store new value at settings */
	xfdashboard_hot_corner_settings_set_activation_radius(mapping->settings, value);
}

/* Value for activation radius was changed at settings */
static void _plugin_on_radius_settings_value_changed(PluginWidgetSettingsMap *inMapping)
{
	gint							value;

	g_return_if_fail(inMapping);

	/* Get new value from settings */
	value=xfdashboard_hot_corner_settings_get_activation_radius(inMapping->settings);

	/* Set new value at widget */
	gtk_spin_button_set_value(GTK_SPIN_BUTTON(inMapping->widget), value);
}

/* Value for activation duration was changed at widget */
static void _plugin_on_duration_widget_value_changed(GtkRange *inRange,
														gpointer inUserData)
{
	PluginWidgetSettingsMap			*mapping;
	gint64							value;

	g_return_if_fail(GTK_IS_RANGE(inRange));
	g_return_if_fail(inUserData);

	mapping=(PluginWidgetSettingsMap*)inUserData;

	/* Get new value from widget */
	value=gtk_range_get_value(inRange);

	/* Store new value at settings */
	xfdashboard_hot_corner_settings_set_activation_duration(mapping->settings, value);
}

/* Value for activation duration was changed at settings */
static void _plugin_on_duration_settings_value_changed(PluginWidgetSettingsMap *inMapping)
{
	gint64							value;

	g_return_if_fail(inMapping);

	/* Get new value from settings */
	value=xfdashboard_hot_corner_settings_get_activation_duration(inMapping->settings);

	/* Set new value at widget */
	gtk_range_set_value(GTK_RANGE(inMapping->widget), value);
}

/* Format value to show activation duration time in milliseconds */
static gchar* _plugin_on_duration_settings_format_value(GtkScale *inWidget,
														gdouble inValue,
														gpointer inUserData)
{
	gchar		*text;

	if(inValue>=1000.0)
	{
		text=g_strdup_printf("%.1f %s", inValue/1000.0, _("s"));
	}
		else if(inValue>0.0)
		{
			text=g_strdup_printf("%u %s", (guint)inValue, _("ms"));
		}
		else
		{
			text=g_strdup(_("Immediately"));
		}

	return(text);
}
/* Plugin configuration function */
static GObject* plugin_configure(XfdashboardPlugin *self, gpointer inUserData)
{
	GtkWidget						*layout;
	GtkWidget						*widgetLabel;
	GtkWidget						*widgetValue;
	XfdashboardHotCornerSettings	*settings;
	PluginWidgetSettingsMap			*mapping;
	GtkListStore					*listModel;
	GtkTreeIter						modelIter;
	GEnumClass						*enumClass;
	guint							i;
	GtkCellRenderer					*renderer;

	/* Get settings of plugin */
	settings=xfdashboard_hot_corner_settings_new();

	/* Create layout widget */
	layout=gtk_grid_new();
	gtk_grid_set_row_spacing(GTK_GRID(layout), 8);
	gtk_grid_set_column_spacing(GTK_GRID(layout), 8);

	/* Add widget to choose activation corner */
	widgetLabel=gtk_label_new(_("Activation corner:"));
	gtk_widget_set_halign(widgetLabel, GTK_ALIGN_END);
	gtk_grid_attach(GTK_GRID(layout), widgetLabel, 0, 0, 1, 1);

	widgetValue=gtk_combo_box_new();
	mapping=_plugin_widget_settings_map_bind(widgetValue,
												settings,
												"activation-corner",
												G_CALLBACK(_plugin_on_corner_settings_value_changed));
	g_signal_connect(widgetValue,
						"changed",
						G_CALLBACK(_plugin_on_corner_widget_value_changed),
						mapping);
	gtk_grid_attach_next_to(GTK_GRID(layout), widgetValue, widgetLabel, GTK_POS_RIGHT, 1, 1);

	listModel=gtk_list_store_new(2, G_TYPE_STRING, G_TYPE_INT);
	enumClass=g_type_class_ref(XFDASHBOARD_TYPE_HOT_CORNER_ACTIVATION_CORNER);
	for(i=0; i<enumClass->n_values; i++)
	{
		gtk_list_store_append(listModel, &modelIter);
		gtk_list_store_set(listModel, &modelIter,
							0, enumClass->values[i].value_nick,
							1, enumClass->values[i].value,
							-1);
	}
	gtk_combo_box_set_model(GTK_COMBO_BOX(widgetValue), GTK_TREE_MODEL(listModel));
	g_type_class_unref(enumClass);
	g_object_unref(G_OBJECT(listModel));

	renderer=gtk_cell_renderer_text_new();
	gtk_cell_layout_pack_start(GTK_CELL_LAYOUT(widgetValue), renderer, TRUE);
	gtk_cell_layout_add_attribute(GTK_CELL_LAYOUT(widgetValue), renderer, "text", 0);

	_plugin_on_corner_settings_value_changed(mapping);

	/* Add widget to choose activation radius */
	widgetLabel=gtk_label_new(_("Radius of activation corner:"));
	gtk_widget_set_halign(widgetLabel, GTK_ALIGN_END);
	gtk_grid_attach(GTK_GRID(layout), widgetLabel, 0, 1, 1, 1);

	widgetValue=gtk_spin_button_new_with_range(1.0, 999.0, 1.0);
	mapping=_plugin_widget_settings_map_bind(widgetValue,
												settings,
												"activation-radius",
												G_CALLBACK(_plugin_on_radius_settings_value_changed));
	g_signal_connect(widgetValue,
						"value-changed",
						G_CALLBACK(_plugin_on_radius_widget_value_changed),
						mapping);
	gtk_spin_button_set_value(GTK_SPIN_BUTTON(widgetValue),
								xfdashboard_hot_corner_settings_get_activation_radius(settings));
	gtk_grid_attach_next_to(GTK_GRID(layout), widgetValue, widgetLabel, GTK_POS_RIGHT, 1, 1);

	/* Add widget to choose activation duration */
	widgetLabel=gtk_label_new(_("Timeout to activate:"));
	gtk_widget_set_halign(widgetLabel, GTK_ALIGN_END);
	gtk_grid_attach(GTK_GRID(layout), widgetLabel, 0, 2, 1, 1);

	widgetValue=gtk_scale_new_with_range(GTK_ORIENTATION_HORIZONTAL, 100.0, 10000.0, 100.0);
	mapping=_plugin_widget_settings_map_bind(widgetValue,
												settings,
												"activation-duration",
												G_CALLBACK(_plugin_on_duration_settings_value_changed));
	g_signal_connect(widgetValue,
						"value-changed",
						G_CALLBACK(_plugin_on_duration_widget_value_changed),
						mapping);
	g_signal_connect(widgetValue,
						"format-value",
						G_CALLBACK(_plugin_on_duration_settings_format_value),
						NULL);
	gtk_range_set_value(GTK_RANGE(widgetValue),
						xfdashboard_hot_corner_settings_get_activation_duration(settings));
	gtk_grid_attach_next_to(GTK_GRID(layout), widgetValue, widgetLabel, GTK_POS_RIGHT, 1, 1);

	/* Release allocated resources */
	if(settings) g_object_unref(settings);

	/* Make all widgets visible */
	gtk_widget_show_all(layout);

	/* Return layout widget containing all other widgets */
	return(G_OBJECT(layout));
}

/* Plugin enable function */
static void plugin_enable(XfdashboardPlugin *self, gpointer inUserData)
{
	/* Create instance of hot corner */
	if(!hotCorner)
	{
		hotCorner=xfdashboard_hot_corner_new();
	}
}

/* Plugin disable function */
static void plugin_disable(XfdashboardPlugin *self, gpointer inUserData)
{
	/* Destroy instance of hot corner */
	if(hotCorner)
	{
		g_object_unref(hotCorner);
		hotCorner=NULL;
	}
}

/* Plugin initialization function */
G_MODULE_EXPORT void plugin_init(XfdashboardPlugin *self)
{
	/* Set up localization */
	xfce_textdomain(GETTEXT_PACKAGE, PACKAGE_LOCALE_DIR, "UTF-8");

	/* Set plugin info */
	xfdashboard_plugin_set_info(self,
								"name", _("Hot corner"),
								"description", _("Activates xfdashboard when pointer is moved to a configured corner of monitor"),
								"author", "Stephan Haller <nomad@froevel.de>",
								NULL);

	/* Register GObject types of this plugin */
	XFDASHBOARD_REGISTER_PLUGIN_TYPE(self, xfdashboard_hot_corner);
	XFDASHBOARD_REGISTER_PLUGIN_TYPE(self, xfdashboard_hot_corner_settings);

	/* Connect plugin action handlers */
	g_signal_connect(self, "enable", G_CALLBACK(plugin_enable), NULL);
	g_signal_connect(self, "disable", G_CALLBACK(plugin_disable), NULL);
	g_signal_connect(self, "configure", G_CALLBACK(plugin_configure), NULL);
}
