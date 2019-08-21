/*
 * binding: A keyboard or pointer binding
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

#include <glib/gi18n-lib.h>
#include <gdk/gdk.h>

#include <libxfdashboard/binding.h>
#include <libxfdashboard/enums.h>
#include <libxfdashboard/compat.h>
#include <libxfdashboard/debug.h>


/* Define this class in GObject system */
struct _XfdashboardBindingPrivate
{
	/* Instance related */
	ClutterEventType		eventType;
	gchar					*className;
	guint					key;
	ClutterModifierType		modifiers;
	gchar					*target;
	gchar					*action;
	XfdashboardBindingFlags	flags;
};

G_DEFINE_TYPE_WITH_PRIVATE(XfdashboardBinding,
							xfdashboard_binding,
							G_TYPE_OBJECT)

/* Properties */
enum
{
	PROP_0,

	PROP_EVENT_TYPE,
	PROP_CLASS_NAME,
	PROP_KEY,
	PROP_MODIFIERS,
	PROP_TARGET,
	PROP_ACTION,
	PROP_FLAGS,

	PROP_LAST
};

static GParamSpec* XfdashboardBindingProperties[PROP_LAST]={ 0, };

/* IMPLEMENTATION: Private variables and methods */


/* IMPLEMENTATION: GObject */

/* Dispose this object */
static void _xfdashboard_binding_dispose(GObject *inObject)
{
	/* Release allocated variables */
	XfdashboardBinding			*self=XFDASHBOARD_BINDING(inObject);
	XfdashboardBindingPrivate	*priv=self->priv;

	if(priv->className)
	{
		g_free(priv->className);
		priv->className=NULL;
	}

	if(priv->target)
	{
		g_free(priv->target);
		priv->target=NULL;
	}

	if(priv->action)
	{
		g_free(priv->action);
		priv->action=NULL;
	}

	/* Call parent's class dispose method */
	G_OBJECT_CLASS(xfdashboard_binding_parent_class)->dispose(inObject);
}

/* Set/get properties */
static void _xfdashboard_binding_set_property(GObject *inObject,
												guint inPropID,
												const GValue *inValue,
												GParamSpec *inSpec)
{
	XfdashboardBinding			*self=XFDASHBOARD_BINDING(inObject);

	switch(inPropID)
	{
		case PROP_EVENT_TYPE:
			xfdashboard_binding_set_event_type(self, g_value_get_enum(inValue));
			break;

		case PROP_CLASS_NAME:
			xfdashboard_binding_set_class_name(self, g_value_get_string(inValue));
			break;

		case PROP_KEY:
			xfdashboard_binding_set_key(self, g_value_get_uint(inValue));
			break;

		case PROP_MODIFIERS:
			xfdashboard_binding_set_modifiers(self, g_value_get_flags(inValue));
			break;

		case PROP_TARGET:
			xfdashboard_binding_set_target(self, g_value_get_string(inValue));
			break;

		case PROP_ACTION:
			xfdashboard_binding_set_action(self, g_value_get_string(inValue));
			break;

		case PROP_FLAGS:
			xfdashboard_binding_set_flags(self, g_value_get_flags(inValue));
			break;

		default:
			G_OBJECT_WARN_INVALID_PROPERTY_ID(inObject, inPropID, inSpec);
			break;
	}
}

static void _xfdashboard_binding_get_property(GObject *inObject,
												guint inPropID,
												GValue *outValue,
												GParamSpec *inSpec)
{
	XfdashboardBinding			*self=XFDASHBOARD_BINDING(inObject);
	XfdashboardBindingPrivate	*priv=self->priv;

	switch(inPropID)
	{
		case PROP_EVENT_TYPE:
			g_value_set_enum(outValue, priv->eventType);
			break;

		case PROP_CLASS_NAME:
			g_value_set_string(outValue, priv->className);
			break;

		case PROP_KEY:
			g_value_set_uint(outValue, priv->key);
			break;

		case PROP_MODIFIERS:
			g_value_set_flags(outValue, priv->modifiers);
			break;

		case PROP_TARGET:
			g_value_set_string(outValue, priv->target);
			break;

		case PROP_ACTION:
			g_value_set_string(outValue, priv->action);
			break;

		case PROP_FLAGS:
			g_value_set_flags(outValue, priv->flags);
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
static void xfdashboard_binding_class_init(XfdashboardBindingClass *klass)
{
	GObjectClass					*gobjectClass=G_OBJECT_CLASS(klass);

	/* Override functions */
	gobjectClass->dispose=_xfdashboard_binding_dispose;
	gobjectClass->set_property=_xfdashboard_binding_set_property;
	gobjectClass->get_property=_xfdashboard_binding_get_property;

	/* Define properties */
	XfdashboardBindingProperties[PROP_EVENT_TYPE]=
		g_param_spec_enum("event-type",
							_("Event type"),
							_("The event type this binding is bound to"),
							CLUTTER_TYPE_EVENT_TYPE,
							CLUTTER_NOTHING,
							G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS);

	XfdashboardBindingProperties[PROP_CLASS_NAME]=
		g_param_spec_string("class-name",
								_("Class name"),
								_("Class name of object this binding is bound to"),
								NULL,
								G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS);

	XfdashboardBindingProperties[PROP_KEY]=
		g_param_spec_uint("key",
							_("Key"),
							_("Key code of a keyboard event this binding is bound to"),
							0, G_MAXUINT,
							0,
							G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS);

	XfdashboardBindingProperties[PROP_MODIFIERS]=
		g_param_spec_flags("modifiers",
							_("Modifiers"),
							_("Modifiers this binding is bound to"),
							CLUTTER_TYPE_MODIFIER_TYPE,
							0,
							G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS);

	XfdashboardBindingProperties[PROP_TARGET]=
		g_param_spec_string("target",
								_("Target"),
								_("Class name of target of this binding"),
								NULL,
								G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS);

	XfdashboardBindingProperties[PROP_ACTION]=
		g_param_spec_string("action",
								_("Action"),
								_("Action assigned to this binding"),
								NULL,
								G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS);

	XfdashboardBindingProperties[PROP_FLAGS]=
		g_param_spec_flags("flags",
								_("Flags"),
								_("Flags assigned to this binding"),
								XFDASHBOARD_TYPE_BINDING_FLAGS,
								0,
								G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS);

	g_object_class_install_properties(gobjectClass, PROP_LAST, XfdashboardBindingProperties);
}

/* Object initialization
 * Create private structure and set up default values
 */
static void xfdashboard_binding_init(XfdashboardBinding *self)
{
	XfdashboardBindingPrivate	*priv;

	priv=self->priv=xfdashboard_binding_get_instance_private(self);

	/* Set up default values */
	priv->eventType=CLUTTER_NOTHING;
	priv->className=NULL;
	priv->key=0;
	priv->modifiers=0;
	priv->target=NULL;
	priv->action=NULL;
}

/* IMPLEMENTATION: Public API */

/* Create new instance */
XfdashboardBinding* xfdashboard_binding_new(void)
{
	return(XFDASHBOARD_BINDING(g_object_new(XFDASHBOARD_TYPE_BINDING, NULL)));
}

XfdashboardBinding* xfdashboard_binding_new_for_event(const ClutterEvent *inEvent)
{
	XfdashboardBinding		*binding;
	ClutterEventType		eventType;

	g_return_val_if_fail(inEvent, NULL);

	/* Create instance */
	binding=XFDASHBOARD_BINDING(g_object_new(XFDASHBOARD_TYPE_BINDING, NULL));
	if(!binding)
	{
		g_warning(_("Failed to create binding instance"));
		return(NULL);
	}

	/* Set up instance */
	eventType=clutter_event_type(inEvent);
	switch(eventType)
	{
		case CLUTTER_KEY_PRESS:
			xfdashboard_binding_set_event_type(binding, eventType);
			xfdashboard_binding_set_key(binding, ((ClutterKeyEvent*)inEvent)->keyval);
			xfdashboard_binding_set_modifiers(binding, ((ClutterKeyEvent*)inEvent)->modifier_state);
			break;

		case CLUTTER_KEY_RELEASE:
			/* We assume that a key event with a key value and a modifier state but no unicode
			 * value is the release of a single key which is a modifier. In this case do not
			 * set the modifier state in this binding which is created for this event.
			 * This means: Only set modifier state in this binding if key valule, modifier state
			 * and a unicode value is set.
			 */
			xfdashboard_binding_set_event_type(binding, eventType);
			xfdashboard_binding_set_key(binding, ((ClutterKeyEvent*)inEvent)->keyval);
			if(((ClutterKeyEvent*)inEvent)->keyval &&
				((ClutterKeyEvent*)inEvent)->modifier_state &&
				((ClutterKeyEvent*)inEvent)->unicode_value)
			{
				xfdashboard_binding_set_modifiers(binding, ((ClutterKeyEvent*)inEvent)->modifier_state);
			}
			break;

		default:
			XFDASHBOARD_DEBUG(binding, MISC,
								"Cannot create binding instance for unsupported or invalid event type %d",
								eventType);

			/* Release allocated resources */
			g_object_unref(binding);

			return(NULL);
	}

	/* Return create binding instance */
	return(binding);
}

/* Get hash value for binding */
guint xfdashboard_binding_hash(gconstpointer inValue)
{
	XfdashboardBindingPrivate		*priv;
	guint							hash;

	g_return_val_if_fail(XFDASHBOARD_IS_BINDING(inValue), 0);

	priv=XFDASHBOARD_BINDING(inValue)->priv;
	hash=0;

	/* Create hash value */
	if(priv->className) hash=g_str_hash(priv->className);

	switch(priv->eventType)
	{
		case CLUTTER_KEY_PRESS:
		case CLUTTER_KEY_RELEASE:
			hash^=priv->key;
			hash^=priv->modifiers;
			break;

		default:
			g_assert_not_reached();
			break;
	}

	return(hash);
}

/* Check if two bindings are equal */
gboolean xfdashboard_binding_compare(gconstpointer inLeft, gconstpointer inRight)
{
	XfdashboardBindingPrivate		*leftPriv;
	XfdashboardBindingPrivate		*rightPriv;

	g_return_val_if_fail(XFDASHBOARD_IS_BINDING(inLeft), FALSE);
	g_return_val_if_fail(XFDASHBOARD_IS_BINDING(inRight), FALSE);

	leftPriv=XFDASHBOARD_BINDING(inLeft)->priv;
	rightPriv=XFDASHBOARD_BINDING(inRight)->priv;

	/* Check if event type of bindings are equal */
	if(leftPriv->eventType!=rightPriv->eventType) return(FALSE);

	/* Check if class of bindings are equal */
	if(g_strcmp0(leftPriv->className, rightPriv->className)) return(FALSE);

	/* Check if other values of bindings are equal - depending on their type */
	switch(leftPriv->eventType)
	{
		case CLUTTER_KEY_PRESS:
		case CLUTTER_KEY_RELEASE:
			if(leftPriv->key!=rightPriv->key ||
				leftPriv->modifiers!=rightPriv->modifiers)
			{
				return(FALSE);
			}
			break;

		default:
			/* We should never get here but if we do return FALSE
			 * to indicate that both binding are not equal.
			 */
			g_assert_not_reached();
			return(FALSE);
	}

	/* If we get here all check succeeded so return TRUE */
	return(TRUE);
}

/* Get/set event type of binding */
ClutterEventType xfdashboard_binding_get_event_type(const XfdashboardBinding *self)
{
	g_return_val_if_fail(XFDASHBOARD_IS_BINDING(self), CLUTTER_NOTHING);

	return(self->priv->eventType);
}

void xfdashboard_binding_set_event_type(XfdashboardBinding *self, ClutterEventType inType)
{
	XfdashboardBindingPrivate	*priv;

	g_return_if_fail(XFDASHBOARD_IS_BINDING(self));

	priv=self->priv;

	/* Only key or pointer events can be handled by binding */
	if(inType!=CLUTTER_KEY_PRESS &&
		inType!=CLUTTER_KEY_RELEASE)
	{
		GEnumClass				*eventEnumClass;
		GEnumValue				*eventEnumValue;

		eventEnumClass=g_type_class_ref(CLUTTER_TYPE_EVENT);

		eventEnumValue=g_enum_get_value(eventEnumClass, inType);
		if(eventEnumValue)
		{
			g_warning(_("Cannot set unsupported event type %s at binding"),
						eventEnumValue->value_name);
		}
			else
			{
				g_warning(_("Cannot set invalid event type at binding"));
			}

		g_type_class_unref(eventEnumClass);

		return;
	}

	/* Set value if changed */
	if(priv->eventType!=inType)
	{
		/* Set value */
		priv->eventType=inType;

		/* Notify about property change */
		g_object_notify_by_pspec(G_OBJECT(self), XfdashboardBindingProperties[PROP_EVENT_TYPE]);
	}
}

/* Get/set class name of binding */
const gchar* xfdashboard_binding_get_class_name(const XfdashboardBinding *self)
{
	g_return_val_if_fail(XFDASHBOARD_IS_BINDING(self), NULL);

	return(self->priv->className);
}

void xfdashboard_binding_set_class_name(XfdashboardBinding *self, const gchar *inClassName)
{
	XfdashboardBindingPrivate	*priv;

	g_return_if_fail(XFDASHBOARD_IS_BINDING(self));
	g_return_if_fail(inClassName && *inClassName);

	priv=self->priv;

	/* Set value if changed */
	if(g_strcmp0(priv->className, inClassName)!=0)
	{
		/* Set value */
		if(priv->className)
		{
			g_free(priv->className);
			priv->className=NULL;
		}

		if(inClassName) priv->className=g_strdup(inClassName);

		/* Notify about property change */
		g_object_notify_by_pspec(G_OBJECT(self), XfdashboardBindingProperties[PROP_CLASS_NAME]);
	}
}

/* Get/set key code of binding */
guint xfdashboard_binding_get_key(const XfdashboardBinding *self)
{
	g_return_val_if_fail(XFDASHBOARD_IS_BINDING(self), 0);

	return(self->priv->key);
}

void xfdashboard_binding_set_key(XfdashboardBinding *self, guint inKey)
{
	XfdashboardBindingPrivate	*priv;

	g_return_if_fail(XFDASHBOARD_IS_BINDING(self));
	g_return_if_fail(inKey>0);

	priv=self->priv;

	/* Set value if changed */
	if(priv->key!=inKey)
	{
		/* Set value */
		priv->key=inKey;

		/* Notify about property change */
		g_object_notify_by_pspec(G_OBJECT(self), XfdashboardBindingProperties[PROP_KEY]);
	}
}

/* Get/set modifiers of binding */
ClutterModifierType xfdashboard_binding_get_modifiers(const XfdashboardBinding *self)
{
	g_return_val_if_fail(XFDASHBOARD_IS_BINDING(self), 0);

	return(self->priv->modifiers);
}

void xfdashboard_binding_set_modifiers(XfdashboardBinding *self, ClutterModifierType inModifiers)
{
	XfdashboardBindingPrivate	*priv;

	g_return_if_fail(XFDASHBOARD_IS_BINDING(self));

	priv=self->priv;

	/* Reduce modifiers to supported ones */
	inModifiers=inModifiers & XFDASHBOARD_BINDING_MODIFIERS_MASK;

	/* Set value if changed */
	if(priv->modifiers!=inModifiers)
	{
		/* Set value */
		priv->modifiers=inModifiers;

		/* Notify about property change */
		g_object_notify_by_pspec(G_OBJECT(self), XfdashboardBindingProperties[PROP_MODIFIERS]);
	}
}

/* Get/set target of binding */
const gchar* xfdashboard_binding_get_target(const XfdashboardBinding *self)
{
	g_return_val_if_fail(XFDASHBOARD_IS_BINDING(self), NULL);

	return(self->priv->target);
}

void xfdashboard_binding_set_target(XfdashboardBinding *self, const gchar *inTarget)
{
	XfdashboardBindingPrivate	*priv;

	g_return_if_fail(XFDASHBOARD_IS_BINDING(self));
	g_return_if_fail(inTarget && *inTarget);

	priv=self->priv;

	/* Set value if changed */
	if(g_strcmp0(priv->target, inTarget)!=0)
	{
		/* Set value */
		if(priv->target)
		{
			g_free(priv->target);
			priv->target=NULL;
		}

		if(inTarget) priv->target=g_strdup(inTarget);

		/* Notify about property change */
		g_object_notify_by_pspec(G_OBJECT(self), XfdashboardBindingProperties[PROP_TARGET]);
	}
}

/* Get/set action of binding */
const gchar* xfdashboard_binding_get_action(const XfdashboardBinding *self)
{
	g_return_val_if_fail(XFDASHBOARD_IS_BINDING(self), NULL);

	return(self->priv->action);
}

void xfdashboard_binding_set_action(XfdashboardBinding *self, const gchar *inAction)
{
	XfdashboardBindingPrivate	*priv;

	g_return_if_fail(XFDASHBOARD_IS_BINDING(self));
	g_return_if_fail(inAction && *inAction);

	priv=self->priv;

	/* Set value if changed */
	if(g_strcmp0(priv->action, inAction)!=0)
	{
		/* Set value */
		if(priv->action)
		{
			g_free(priv->action);
			priv->action=NULL;
		}

		if(inAction) priv->action=g_strdup(inAction);

		/* Notify about property change */
		g_object_notify_by_pspec(G_OBJECT(self), XfdashboardBindingProperties[PROP_ACTION]);
	}
}

/* Get/set flags of binding */
XfdashboardBindingFlags xfdashboard_binding_get_flags(const XfdashboardBinding *self)
{
	g_return_val_if_fail(XFDASHBOARD_IS_BINDING(self), 0);

	return(self->priv->flags);
}

void xfdashboard_binding_set_flags(XfdashboardBinding *self, XfdashboardBindingFlags inFlags)
{
	XfdashboardBindingPrivate	*priv;

	g_return_if_fail(XFDASHBOARD_IS_BINDING(self));
	g_return_if_fail(inFlags<=XFDASHBOARD_BINDING_FLAGS_ALLOW_UNFOCUSABLE_TARGET);

	priv=self->priv;

	/* Set value if changed */
	if(priv->flags!=inFlags)
	{
		/* Set value */
		priv->flags=inFlags;

		/* Notify about property change */
		g_object_notify_by_pspec(G_OBJECT(self), XfdashboardBindingProperties[PROP_FLAGS]);
	}
}
