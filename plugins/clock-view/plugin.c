/*
 * plugin: Plugin functions for 'clock-view'
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
#include <config.h>
#endif

#include <libxfce4util/libxfce4util.h>
#include <gtk/gtk.h>

#include "clock-view.h"
#include "clock-view-settings.h"


/* Forward declarations */
G_MODULE_EXPORT void plugin_init(XfdashboardPlugin *self);


/* IMPLEMENTATION: XfdashboardPlugin */

/* Color has changed at settings */
static void _plugin_on_settings_color_change(GObject *inObject,
												GParamSpec *inSpec,
												gpointer inUserData)
{
	XfdashboardClockViewSettings	*settings;
	GtkColorButton					*button;
	ClutterColor					*settingsColor;
	GdkRGBA							widgetColor;

	g_return_if_fail(XFDASHBOARD_IS_CLOCK_VIEW_SETTINGS(inObject));
	g_return_if_fail(GTK_IS_COLOR_BUTTON(inUserData));

	settings=XFDASHBOARD_CLOCK_VIEW_SETTINGS(inObject);
	button=GTK_COLOR_BUTTON(inUserData);

	/* Get current color from settings */
	g_object_get(G_OBJECT(settings), g_param_spec_get_name(inSpec), &settingsColor, NULL);

	/* Convert color for color button */
	widgetColor.red=settingsColor->red/255.0f;
	widgetColor.green=settingsColor->green/255.0f;
	widgetColor.blue=settingsColor->blue/255.0f;
	widgetColor.alpha=settingsColor->alpha/255.0f;

	/* Set converted color at color button */
#if GTK_CHECK_VERSION(3, 4, 0)
	gtk_color_chooser_set_rgba(GTK_COLOR_CHOOSER(button), &widgetColor);
#else
	gtk_color_button_set_rgba(button, &widgetColor);
#endif
}

/* A new color was chosen at color button */
static void _plugin_on_color_button_color_chosen(GtkColorButton *inButton,
													gpointer inUserData)
{
	GdkRGBA							widgetColor;
	ClutterColor					settingsColor;
	const gchar						*property;
	XfdashboardClockViewSettings	*settings;

	g_return_if_fail(GTK_IS_COLOR_BUTTON(inButton));
	g_return_if_fail(inUserData);

	property=(const gchar*)inUserData;

	/* Get color from color button */
#if GTK_CHECK_VERSION(3, 4, 0)
	gtk_color_chooser_get_rgba(GTK_COLOR_CHOOSER(inButton), &widgetColor);
#else
	gtk_color_button_get_rgba(inButton, &widgetColor);
#endif

	/* Convert color for settings */
	settingsColor.red=MIN(255, (gint)(widgetColor.red*255.0f));
	settingsColor.green=MIN(255, (gint)(widgetColor.green*255.0f));
	settingsColor.blue=MIN(255, (gint)(widgetColor.blue*255.0f));
	settingsColor.alpha=MIN(255, (gint)(widgetColor.alpha*255.0f));

	/* Set converted color at settings */
	settings=xfdashboard_clock_view_settings_new();
	g_object_set(settings, property, &settingsColor, NULL);
	g_object_unref(settings);
}

/* A color button is going to be destroyed */
static void _plugin_on_widget_value_destroy(GtkWidget *inWidget,
											gpointer inUserData)
{
	XfdashboardClockViewSettings	*settings;
	guint							signalID;

	g_return_if_fail(GTK_IS_WIDGET(inWidget));
	g_return_if_fail(inUserData);

	signalID=GPOINTER_TO_UINT(inUserData);

	/* Disconnect signal from setting as the widget where to update
	 * the updated settings value at will be destroyed. So it will
	 * not exist anymore when the signal handler is called.
	 */
	settings=xfdashboard_clock_view_settings_new();
	g_signal_handler_disconnect(settings, signalID);
	g_object_unref(settings);
}

/* Set up color button, e.g. set initial color from settings, connect signals
 * to store newly chosen color or to reflect changed color from settings
 * in widget etc.
 */
static void _plugin_configure_setup_color_button(GtkColorButton *inButton,
													XfdashboardClockViewSettings *inSettings,
													const gchar *inProperty)
{
	ClutterColor					*settingsColor;
	GdkRGBA							widgetColor;
	gchar							*signalName;
	guint							signalID;

	g_return_if_fail(GTK_IS_COLOR_BUTTON(inButton));
	g_return_if_fail(XFDASHBOARD_IS_CLOCK_VIEW_SETTINGS(inSettings));
	g_return_if_fail(inProperty && *inProperty);

	/* Get current color from settings */
	g_object_get(G_OBJECT(inSettings), inProperty, &settingsColor, NULL);

	/* Convert color for color button */
	widgetColor.red=settingsColor->red/255.0f;
	widgetColor.green=settingsColor->green/255.0f;
	widgetColor.blue=settingsColor->blue/255.0f;
	widgetColor.alpha=settingsColor->alpha/255.0f;

	/* Set converted color at color button */
#if GTK_CHECK_VERSION(3, 4, 0)
	gtk_color_chooser_set_rgba(GTK_COLOR_CHOOSER(inButton), &widgetColor);
#else
	gtk_color_button_set_rgba(inButton, &widgetColor);
#endif

	/* Connect signal to store color of new one was chosen at color button.
	 * We connect to "destroy" signal of widget to release the signal handler
	 * on settings object which will survive and will try to work on the
	 * widget just being destroyed.
	 */
	g_signal_connect(inButton,
						"color-set",
						G_CALLBACK(_plugin_on_color_button_color_chosen),
						(gpointer)inProperty);

	signalName=g_strdup_printf("notify::%s", inProperty);
	signalID=g_signal_connect(inSettings,
								signalName,
								G_CALLBACK(_plugin_on_settings_color_change),
								inButton);

	g_signal_connect(inButton,
						"destroy",
						G_CALLBACK(_plugin_on_widget_value_destroy),
						GUINT_TO_POINTER(signalID));

	/* Release allocated resources */
	if(settingsColor) clutter_color_free(settingsColor);
	if(signalName) g_free(signalName);
}

/* Plugin configuration function */
static GObject* plugin_configure(XfdashboardPlugin *self, gpointer inUserData)
{
	GtkWidget						*layout;
	GtkWidget						*widgetLabel;
	GtkWidget						*widgetValue;
	XfdashboardClockViewSettings	*settings;

	/* Get settings */
	settings=xfdashboard_clock_view_settings_new();

	/* Create layout widget */
	layout=gtk_grid_new();
	gtk_grid_set_column_spacing(GTK_GRID(layout), 8);

	/* Add widget to choose hour color */
	widgetLabel=gtk_label_new(_("Hour color:"));
	gtk_widget_set_halign(widgetLabel, GTK_ALIGN_END);
	gtk_grid_attach(GTK_GRID(layout), widgetLabel, 0, 0, 1, 1);

	widgetValue=gtk_color_button_new();
#if GTK_CHECK_VERSION(3, 4, 0)
	gtk_color_chooser_set_use_alpha(GTK_COLOR_CHOOSER(widgetValue), TRUE);
#else
	gtk_color_button_set_use_alpha(GTK_COLOR_BUTTON(widgetValue), TRUE);
#endif
	gtk_color_button_set_title(GTK_COLOR_BUTTON(widgetValue), _("Choose color for hour hand"));
	gtk_grid_attach_next_to(GTK_GRID(layout), widgetValue, widgetLabel, GTK_POS_RIGHT, 1, 1);
	_plugin_configure_setup_color_button(GTK_COLOR_BUTTON(widgetValue), settings, "hour-color");

	/* Add widget to choose minute color */
	widgetLabel=gtk_label_new(_("Minute color:"));
	gtk_widget_set_halign(widgetLabel, GTK_ALIGN_END);
	gtk_grid_attach(GTK_GRID(layout), widgetLabel, 0, 1, 1, 1);

	widgetValue=gtk_color_button_new();
#if GTK_CHECK_VERSION(3, 4, 0)
	gtk_color_chooser_set_use_alpha(GTK_COLOR_CHOOSER(widgetValue), TRUE);
#else
	gtk_color_button_set_use_alpha(GTK_COLOR_BUTTON(widgetValue), TRUE);
#endif
	gtk_color_button_set_title(GTK_COLOR_BUTTON(widgetValue), _("Choose color for minute hand"));
	_plugin_configure_setup_color_button(GTK_COLOR_BUTTON(widgetValue), settings, "minute-color");
	gtk_grid_attach_next_to(GTK_GRID(layout), widgetValue, widgetLabel, GTK_POS_RIGHT, 1, 1);

	/* Add widget to choose second color */
	widgetLabel=gtk_label_new(_("Second color:"));
	gtk_widget_set_halign(widgetLabel, GTK_ALIGN_END);
	gtk_grid_attach(GTK_GRID(layout), widgetLabel, 0, 2, 1, 1);

	widgetValue=gtk_color_button_new();
#if GTK_CHECK_VERSION(3, 4, 0)
	gtk_color_chooser_set_use_alpha(GTK_COLOR_CHOOSER(widgetValue), TRUE);
#else
	gtk_color_button_set_use_alpha(GTK_COLOR_BUTTON(widgetValue), TRUE);
#endif
	gtk_color_button_set_title(GTK_COLOR_BUTTON(widgetValue), _("Choose color for second hand"));
	_plugin_configure_setup_color_button(GTK_COLOR_BUTTON(widgetValue), settings, "second-color");
	gtk_grid_attach_next_to(GTK_GRID(layout), widgetValue, widgetLabel, GTK_POS_RIGHT, 1, 1);

	/* Add widget to choose minute color */
	widgetLabel=gtk_label_new(_("Background color:"));
	gtk_widget_set_halign(widgetLabel, GTK_ALIGN_END);
	gtk_grid_attach(GTK_GRID(layout), widgetLabel, 0, 3, 1, 1);

	widgetValue=gtk_color_button_new();
#if GTK_CHECK_VERSION(3, 4, 0)
	gtk_color_chooser_set_use_alpha(GTK_COLOR_CHOOSER(widgetValue), TRUE);
#else
	gtk_color_button_set_use_alpha(GTK_COLOR_BUTTON(widgetValue), TRUE);
#endif
	gtk_color_button_set_title(GTK_COLOR_BUTTON(widgetValue), _("Choose color for background of second hand"));
	_plugin_configure_setup_color_button(GTK_COLOR_BUTTON(widgetValue), settings, "background-color");
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
	XfdashboardViewManager	*viewManager;

	/* Register view */
	viewManager=xfdashboard_core_get_view_manager(NULL);

	xfdashboard_view_manager_register(viewManager, PLUGIN_ID, XFDASHBOARD_TYPE_CLOCK_VIEW);

	g_object_unref(viewManager);
}

/* Plugin disable function */
static void plugin_disable(XfdashboardPlugin *self, gpointer inUserData)
{
	XfdashboardViewManager	*viewManager;

	/* Unregister view */
	viewManager=xfdashboard_core_get_view_manager(NULL);

	xfdashboard_view_manager_unregister(viewManager, PLUGIN_ID);

	g_object_unref(viewManager);
}

/* Plugin initialization function */
G_MODULE_EXPORT void plugin_init(XfdashboardPlugin *self)
{
	/* Set up localization */
	xfce_textdomain(GETTEXT_PACKAGE, PACKAGE_LOCALE_DIR, "UTF-8");

	/* Register GObject types of this plugin */
	XFDASHBOARD_REGISTER_PLUGIN_TYPE(self, xfdashboard_clock_view);
	XFDASHBOARD_REGISTER_PLUGIN_TYPE(self, xfdashboard_clock_view_settings);

	/* Set plugin info */
	xfdashboard_plugin_set_info(self,
								"flags", XFDASHBOARD_PLUGIN_FLAG_EARLY_INITIALIZATION,
								"name", _("Clock"),
								"description", _("Adds a new view showing a clock"),
								"author", "Stephan Haller <nomad@froevel.de>",
								"settings", xfdashboard_clock_view_settings_new(),
								NULL);

	/* Connect plugin action handlers */
	g_signal_connect(self, "enable", G_CALLBACK(plugin_enable), NULL);
	g_signal_connect(self, "disable", G_CALLBACK(plugin_disable), NULL);
	g_signal_connect(self, "configure", G_CALLBACK(plugin_configure), NULL);
}
