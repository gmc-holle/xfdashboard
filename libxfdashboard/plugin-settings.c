/*
 * plugin-settings: A generic class containing the settings of a plugin
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
 * SECTION:settings
 * @short_description: The settings class for a plugin
 * @include: xfdashboard/plugin-settings.h
 *
 * This class #XfdashboardPluginSettings is an abstract class for use
 * at plugins to manage configurable settings.
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <libxfdashboard/plugin-settings.h>

#include <glib/gi18n-lib.h>

#include <libxfdashboard/compat.h>
#include <libxfdashboard/debug.h>


/* Define this class in GObject system */
G_DEFINE_ABSTRACT_TYPE(XfdashboardPluginSettings,
						xfdashboard_plugin_settings,
						G_TYPE_INITIALLY_UNOWNED);

/* Signals */
enum
{
	SIGNAL_CHANGED,

	SIGNAL_LAST
};

static guint XfdashboardPluginSettingsSignals[SIGNAL_LAST]={ 0, };


/* IMPLEMENTATION: GObject */

/* Default nofity signal handler */
static void _xfdashboard_plugin_settings_notify(GObject *inObject, GParamSpec *inParamSpec)
{
	XfdashboardPluginSettings			*self=XFDASHBOARD_PLUGIN_SETTINGS(inObject);

	/* Only emit "changed" signal if changed property can be read and is not
	 * construct-only (as this one cannot be changed later at runtime).
	 */
	if((inParamSpec->flags & G_PARAM_READABLE) &&
		G_LIKELY(!(inParamSpec->flags & G_PARAM_CONSTRUCT_ONLY)))
	{
		GParamSpec		*redirectTarget;

		/* If the parameter specification is redirected, notify on the target */
		redirectTarget=g_param_spec_get_redirect_target(inParamSpec);
		if(redirectTarget) inParamSpec=redirectTarget;

		/* Emit "changed" signal but set no plugin ID as a core settings was changed */
		g_signal_emit(self, XfdashboardPluginSettingsSignals[SIGNAL_CHANGED], g_param_spec_get_name_quark(inParamSpec), inParamSpec);
	}
}

/* Class initialization
 * Override functions in parent classes and define properties
 * and signals
 */
static void xfdashboard_plugin_settings_class_init(XfdashboardPluginSettingsClass *klass)
{
	GObjectClass		*gobjectClass=G_OBJECT_CLASS(klass);

	/* Override functions */
	gobjectClass->notify=_xfdashboard_plugin_settings_notify;

	/* Define signals */
	/**
	 * XfdashboardPluginSettings::changed:
	 * @self: The plugin settings whose value has changed
	 * @inParamSpec: the #GParamSpec of the property which changed
	 *
	 * The ::changed signal is emitted on an plugin settings object when one of its properties
	 * has its value set and the GObject::notify signal is emitted as well.
	 */
	XfdashboardPluginSettingsSignals[SIGNAL_CHANGED]=
		g_signal_new("changed",
						G_TYPE_FROM_CLASS(klass),
						G_SIGNAL_RUN_FIRST | G_SIGNAL_NO_RECURSE | G_SIGNAL_DETAILED | G_SIGNAL_NO_HOOKS | G_SIGNAL_ACTION,
						G_STRUCT_OFFSET(XfdashboardPluginSettingsClass, changed),
						NULL,
						NULL,
						g_cclosure_marshal_VOID__OBJECT,
						G_TYPE_NONE,
						1,
						G_TYPE_PARAM);
}

/* Object initialization
 * Create private structure and set up default values
 */
static void xfdashboard_plugin_settings_init(XfdashboardPluginSettings *self)
{
}
