/*
 * plugin: Plugin functions for 'hot-corner'
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

static XfdashboardHotCorner			*hotCorner=NULL;

/* Value for activation corner was changed at widget */
static void _plugin_on_corner_widget_value_changed(GtkWidget *inWidget,
													gpointer inUserData,
													gpointer dummy1,
													gpointer dummy2)
{
	XfdashboardHotCornerSettings	*settings;
	GtkTreeModel					*model;
	GtkTreeIter						iter;
	gint							value;

	g_return_if_fail(GTK_IS_COMBO_BOX(inWidget));

	/* Get new value from widget */
	model=gtk_combo_box_get_model(GTK_COMBO_BOX(inWidget));
	gtk_combo_box_get_active_iter(GTK_COMBO_BOX(inWidget), &iter);
	gtk_tree_model_get(model, &iter, 1, &value, -1);

	/* Store new value at settings */
	settings=xfdashboard_hot_corner_settings_new();
	xfdashboard_hot_corner_settings_set_activation_corner(settings, value);
	g_object_unref(settings);
}

/* Value for activation corner was changed at settings */
static void _plugin_on_corner_settings_value_changed(GObject *inObject,
														GParamSpec *inSpec,
														gpointer inUserData)
{
	XfdashboardHotCornerSettings					*settings;
	GtkComboBox										*widget;
	XfdashboardHotCornerSettingsActivationCorner	value;
	guint											modelValue;
	GtkTreeModel									*model;
	GtkTreeIter										iter;

	g_return_if_fail(XFDASHBOARD_IS_HOT_CORNER_SETTINGS(inObject));
	g_return_if_fail(GTK_IS_COMBO_BOX(inUserData));

	settings=XFDASHBOARD_HOT_CORNER_SETTINGS(inObject);
	widget=GTK_COMBO_BOX(inUserData);

	/* Get new value from settings */
	value=xfdashboard_hot_corner_settings_get_activation_corner(settings);

	/* Iterate through combo box value and set new value if match is found */
	model=gtk_combo_box_get_model(widget);
	if(gtk_tree_model_get_iter_first(model, &iter))
	{
		do
		{
			gtk_tree_model_get(model, &iter, 1, &modelValue, -1);
			if(G_UNLIKELY(modelValue==value))
			{
				gtk_combo_box_set_active_iter(widget, &iter);
				break;
			}
		}
		while(gtk_tree_model_iter_next(model, &iter));
	}
}

/* Value for activation radius was changed at widget */
static void _plugin_on_radius_widget_value_changed(GtkWidget *inWidget,
													gpointer inUserData)
{
	XfdashboardHotCornerSettings	*settings;
	gint							value;

	g_return_if_fail(GTK_IS_SPIN_BUTTON(inWidget));

	/* Get new value from widget */
	value=gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(inWidget));

	/* Store new value at settings */
	settings=xfdashboard_hot_corner_settings_new();
	xfdashboard_hot_corner_settings_set_activation_radius(settings, value);
	g_object_unref(settings);
}

/* Value for activation radius was changed at settings */
static void _plugin_on_radius_settings_value_changed(GObject *inObject,
														GParamSpec *inSpec,
														gpointer inUserData)
{
	XfdashboardHotCornerSettings	*settings;
	GtkSpinButton					*widget;
	gint							value;

	g_return_if_fail(XFDASHBOARD_IS_HOT_CORNER_SETTINGS(inObject));
	g_return_if_fail(GTK_IS_SPIN_BUTTON(inUserData));

	settings=XFDASHBOARD_HOT_CORNER_SETTINGS(inObject);
	widget=GTK_SPIN_BUTTON(inUserData);

	/* Get new value from settings */
	value=xfdashboard_hot_corner_settings_get_activation_radius(settings);

	/* Set new value at widget */
	gtk_spin_button_set_value(widget, value);
}

/* Value for activation duration was changed at widget */
static void _plugin_on_duration_widget_value_changed(GtkWidget *inWidget,
														gpointer inUserData)
{
	XfdashboardHotCornerSettings	*settings;
	gint64							value;

	g_return_if_fail(GTK_IS_RANGE(inWidget));

	/* Get new value from widget */
	value=gtk_range_get_value(GTK_RANGE(inWidget));

	/* Store new value at settings */
	settings=xfdashboard_hot_corner_settings_new();
	xfdashboard_hot_corner_settings_set_activation_duration(settings, value);
	g_object_unref(settings);
}

/* Value for activation duration was changed at settings */
static void _plugin_on_duration_settings_value_changed(GObject *inObject,
														GParamSpec *inSpec,
														gpointer inUserData)
{
	XfdashboardHotCornerSettings	*settings;
	GtkRange						*widget;
	gint64							value;

	g_return_if_fail(XFDASHBOARD_IS_HOT_CORNER_SETTINGS(inObject));
	g_return_if_fail(GTK_IS_SPIN_BUTTON(inUserData));

	settings=XFDASHBOARD_HOT_CORNER_SETTINGS(inObject);
	widget=GTK_RANGE(inUserData);

	/* Get new value from settings */
	value=xfdashboard_hot_corner_settings_get_activation_duration(settings);

	/* Set new value at widget */
	gtk_range_set_value(widget, value);
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

/* Value for primary-monitor-only was changed at widget */
static void _plugin_on_primary_monitor_only_widget_value_changed(GtkWidget *inWidget,
																	gpointer inUserData)
{
	XfdashboardHotCornerSettings	*settings;
	gboolean						value;

	g_return_if_fail(GTK_IS_TOGGLE_BUTTON(inWidget));

	/* Get new value from widget */
	value=gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(inWidget));

	/* Store new value at settings */
	settings=xfdashboard_hot_corner_settings_new();
	xfdashboard_hot_corner_settings_set_primary_monitor_only(settings, value);
	g_object_unref(settings);
}

/* Value for activation duration was changed at settings */
static void _plugin_on_primary_monitor_only_settings_value_changed(GObject *inObject,
																	GParamSpec *inSpec,
																	gpointer inUserData)
{
	XfdashboardHotCornerSettings	*settings;
	GtkToggleButton					*widget;
	gboolean						value;

	g_return_if_fail(XFDASHBOARD_IS_HOT_CORNER_SETTINGS(inObject));
	g_return_if_fail(GTK_IS_TOGGLE_BUTTON(inUserData));

	settings=XFDASHBOARD_HOT_CORNER_SETTINGS(inObject);
	widget=GTK_TOGGLE_BUTTON(inUserData);

	/* Get new value from settings */
	value=xfdashboard_hot_corner_settings_get_primary_monitor_only(settings);

	/* Set new value at widget */
	gtk_toggle_button_set_active(widget, value);
}

/* A widget is going to be destroyed */
static void _plugin_on_widget_value_destroy(GtkWidget *inWidget,
											gpointer inUserData)
{
	XfdashboardHotCornerSettings	*settings;
	guint							signalID;

	g_return_if_fail(GTK_IS_WIDGET(inWidget));
	g_return_if_fail(inUserData);

	signalID=GPOINTER_TO_UINT(inUserData);

	/* Disconnect signal from setting as the widget where to update
	 * the updated settings value at will be destroyed. So it will
	 * not exist anymore when the signal handler is called.
	 */
	settings=xfdashboard_hot_corner_settings_new();
	g_signal_handler_disconnect(settings, signalID);
	g_object_unref(settings);
}

/* Connect signals and bind to widget */
static void _plugin_configure_setup_widget(GtkWidget *inWidget,
											XfdashboardHotCornerSettings *inSettings,
											const gchar *inProperty,
											GCallback inCallback)
{
	gchar							*signalName;
	guint							signalID;

	g_return_if_fail(GTK_IS_WIDGET(inWidget));
	g_return_if_fail(XFDASHBOARD_IS_HOT_CORNER_SETTINGS(inSettings));
	g_return_if_fail(inProperty && *inProperty);
	g_return_if_fail(inCallback);

	/* Connect signal to get notified if value at settings changed.
	 * We connect to "destroy" signal of widget to release the signal handler
	 * on settings object which will survive and will try to work on the
	 * widget just being destroyed.
	 */
	signalName=g_strdup_printf("notify::%s", inProperty);
	signalID=g_signal_connect(inSettings,
								signalName,
								inCallback,
								inWidget);

	g_signal_connect(inWidget,
						"destroy",
						G_CALLBACK(_plugin_on_widget_value_destroy),
						GUINT_TO_POINTER(signalID));

	/* Release allocated resources */
	if(signalName) g_free(signalName);
}

/* Plugin configuration function */
static GObject* plugin_configure(XfdashboardPlugin *self, gpointer inUserData)
{
	GtkWidget						*layout;
	GtkWidget						*widgetLabel;
	GtkWidget						*widgetValue;
	XfdashboardHotCornerSettings	*settings;
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
	_plugin_configure_setup_widget(widgetValue,
										settings,
										"activation-corner",
										G_CALLBACK(_plugin_on_corner_settings_value_changed));
	g_signal_connect(widgetValue,
						"changed",
						G_CALLBACK(_plugin_on_corner_widget_value_changed),
						NULL);
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

	_plugin_on_corner_settings_value_changed(G_OBJECT(settings), NULL, widgetValue);

	/* Add widget to choose activation radius */
	widgetLabel=gtk_label_new(_("Radius of activation corner:"));
	gtk_widget_set_halign(widgetLabel, GTK_ALIGN_END);
	gtk_grid_attach(GTK_GRID(layout), widgetLabel, 0, 1, 1, 1);

	widgetValue=gtk_spin_button_new_with_range(1.0, 999.0, 1.0);
	_plugin_configure_setup_widget(widgetValue,
										settings,
										"activation-radius",
										G_CALLBACK(_plugin_on_radius_settings_value_changed));
	g_signal_connect(widgetValue,
						"value-changed",
						G_CALLBACK(_plugin_on_radius_widget_value_changed),
						NULL);
	gtk_spin_button_set_value(GTK_SPIN_BUTTON(widgetValue),
								xfdashboard_hot_corner_settings_get_activation_radius(settings));
	gtk_grid_attach_next_to(GTK_GRID(layout), widgetValue, widgetLabel, GTK_POS_RIGHT, 1, 1);

	/* Add widget to choose activation duration */
	widgetLabel=gtk_label_new(_("Timeout to activate:"));
	gtk_widget_set_halign(widgetLabel, GTK_ALIGN_END);
	gtk_grid_attach(GTK_GRID(layout), widgetLabel, 0, 2, 1, 1);

	widgetValue=gtk_scale_new_with_range(GTK_ORIENTATION_HORIZONTAL, 100.0, 10000.0, 100.0);
	_plugin_configure_setup_widget(widgetValue,
										settings,
										"activation-duration",
										G_CALLBACK(_plugin_on_duration_settings_value_changed));
	g_signal_connect(widgetValue,
						"value-changed",
						G_CALLBACK(_plugin_on_duration_widget_value_changed),
						NULL);
	g_signal_connect(widgetValue,
						"format-value",
						G_CALLBACK(_plugin_on_duration_settings_format_value),
						NULL);
	gtk_range_set_value(GTK_RANGE(widgetValue),
						xfdashboard_hot_corner_settings_get_activation_duration(settings));
	gtk_grid_attach_next_to(GTK_GRID(layout), widgetValue, widgetLabel, GTK_POS_RIGHT, 1, 1);

	/* Add widget to limit checks to primary monitor only or to all monitors */
	widgetValue=gtk_check_button_new_with_label(_("Primary monitor only"));
	_plugin_configure_setup_widget(widgetValue,
										settings,
										"primary-monitor-only",
										G_CALLBACK(_plugin_on_primary_monitor_only_settings_value_changed));
	g_signal_connect(widgetValue,
						"toggled",
						G_CALLBACK(_plugin_on_primary_monitor_only_widget_value_changed),
						NULL);
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(widgetValue),
								xfdashboard_hot_corner_settings_get_primary_monitor_only(settings));
	gtk_grid_attach(GTK_GRID(layout), widgetValue, 0, 3, 2, 1);

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

	/* Register GObject types of this plugin */
	XFDASHBOARD_REGISTER_PLUGIN_TYPE(self, xfdashboard_hot_corner);
	XFDASHBOARD_REGISTER_PLUGIN_TYPE(self, xfdashboard_hot_corner_settings);

	/* Set plugin info */
	xfdashboard_plugin_set_info(self,
								"name", _("Hot corner"),
								"description", _("Activates xfdashboard when pointer is moved to a configured corner of monitor"),
								"author", "Stephan Haller <nomad@froevel.de>",
								"settings", xfdashboard_hot_corner_settings_new(),
								NULL);

	/* Connect plugin action handlers */
	g_signal_connect(self, "enable", G_CALLBACK(plugin_enable), NULL);
	g_signal_connect(self, "disable", G_CALLBACK(plugin_disable), NULL);
	g_signal_connect(self, "configure", G_CALLBACK(plugin_configure), NULL);
}
