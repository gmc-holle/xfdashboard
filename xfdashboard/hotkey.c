/*
 * hotkey: Single-instance of hotkey tracker
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

#include "hotkey.h"

#include <glib/gi18n-lib.h>
#include <clutter/clutter.h>
#include <clutter/x11/clutter-x11.h>

/* Define this class in GObject system */
G_DEFINE_TYPE(XfdashboardHotkey,
				xfdashboard_hotkey,
				G_TYPE_OBJECT)

/* Private structure - access only by public API if needed */
#define XFDASHBOARD_HOTKEY_GET_PRIVATE(obj) \
	(G_TYPE_INSTANCE_GET_PRIVATE((obj), XFDASHBOARD_TYPE_HOTKEY, XfdashboardHotkeyPrivate))

struct _XfdashboardHotkeyPrivate
{
	/* Instance related */
	Display			*display;
	Window			currentFocus;
	int				currentFocusRevert;

	guint			firstKeyCode;
	guint			lastKeyCode;
	gint			pressedKeys;
	gboolean		keySequenceValid;
};

/* Signals */
enum
{
	SIGNAL_ACTIVATE,

	SIGNAL_LAST
};

static guint XfdashboardHotkeySignals[SIGNAL_LAST]={ 0, };

/* IMPLEMENTATION: Private variables and methods */
#define XFDASHBOARD_HOTKEY_FILTER_MASK		(KeyPressMask | KeyReleaseMask | FocusChangeMask)

static XfdashboardHotkey					*_xfdashboard_hotkey_singleton=NULL;

/* Focus has changed so re-setup hotkey tracker */
static void _xfdashboard_hotkey_on_focus_changed(XfdashboardHotkey *self, gboolean inReleaseOnly)
{
	XfdashboardHotkeyPrivate	*priv;
	Window						newFocus;
	int							newFocusRevert;

	g_return_if_fail(XFDASHBOARD_IS_HOTKEY(self));

	priv=self->priv;

	/* Revert event notifications as current focus has changed */
	if(priv->currentFocus!=None)
	{
		g_debug("Reset event notification at old focus %lu", priv->currentFocus);
		XSelectInput(priv->display, priv->currentFocus, 0);

		priv->currentFocus=None;
		priv->currentFocusRevert=0;
	}

	/* If only a release requested stop here */
	if(inReleaseOnly) return;

	/* Get new current focus */
	XGetInputFocus(priv->display, &newFocus, &newFocusRevert);

	/* Set up event notifications if new focus is not the root window */
	if(newFocus==PointerRoot)
	{
		newFocus=clutter_x11_get_root_window();
		g_debug("Move focus from pointer root at %lu to root window %lu",
					PointerRoot,
					newFocus);
	}

	priv->currentFocus=newFocus;
	priv->currentFocusRevert=newFocusRevert;

	XSelectInput(priv->display, priv->currentFocus, XFDASHBOARD_HOTKEY_FILTER_MASK);
	g_debug("Set up event notification at new focus %lu", priv->currentFocus);
}

/* Check if X key event is for hotkey */
static gboolean _xfdashboard_hotkey_is_hotkey(XfdashboardHotkey *self, XKeyEvent *inXEvent, ClutterEvent *inClutterEvent)
{
	KeySym						keySymCode;

	g_return_val_if_fail(XFDASHBOARD_IS_HOTKEY(self), FALSE);
	g_return_val_if_fail((inXEvent && !inClutterEvent) || (!inXEvent && inClutterEvent), FALSE);

	keySymCode=0;

	/* Check for key-press or key-release event and translate key-code to key-sym-code
	 * if neccessary. Any other event than key events results in invalid key-sym-code 0.
	 */
	if(inXEvent &&
		(inXEvent->type==KeyPress || inXEvent->type==KeyRelease))
	{
		gchar					keySymString[2];
		gint					result G_GNUC_UNUSED;

		result=XLookupString(inXEvent, keySymString, 1, &keySymCode, NULL);
		g_debug("Converted key-code %u to key-sym %lu from X key event",
				((XKeyEvent*)inXEvent)->keycode,
				keySymCode);
	}

	if(inClutterEvent &&
		(clutter_event_type(inClutterEvent)==CLUTTER_KEY_PRESS ||
			clutter_event_type(inClutterEvent)==CLUTTER_KEY_RELEASE))
	{
		keySymCode=((ClutterKeyEvent*)inClutterEvent)->keyval;
		g_debug("Using key-sym %lu for key-code %u from Clutter key event.",
					keySymCode,
					((ClutterKeyEvent*)inClutterEvent)->hardware_keycode);
	}

	/* Return TRUE here if key-sym-code is the hotkey */
	if(keySymCode==CLUTTER_KEY_Super_L ||
		keySymCode==CLUTTER_KEY_Super_R)
	{
		return(TRUE);
	}

	/* If we get here the key was not the hotkey */
	return(FALSE);
}

/* Filter X events to keep track of key events for hotkey */
static ClutterX11FilterReturn _xfdashboard_hotkey_on_x_event(XEvent *inXEvent, ClutterEvent *inEvent, gpointer inUserData)
{
	XfdashboardHotkey			*self;
	XfdashboardHotkeyPrivate	*priv;

	g_return_val_if_fail(XFDASHBOARD_IS_HOTKEY(inUserData), CLUTTER_X11_FILTER_CONTINUE);

	self=XFDASHBOARD_HOTKEY(inUserData);
	priv=self->priv;

	/* Handle X event */
	switch(inXEvent->type)
	{
		case FocusOut:
			/* Focus changed so restore old event settings and
			 * setup event handler for new window.
			 */
			_xfdashboard_hotkey_on_focus_changed(self, FALSE);

			/* Reset counters */
			priv->pressedKeys=0;
			priv->keySequenceValid=FALSE;
			break;

		case KeyPress:
			/* If this is the first key pressed in key sequence then remember
			 * the key and set up counters.
			 */
			if(priv->pressedKeys==0)
			{
				priv->firstKeyCode=((XKeyEvent*)inXEvent)->keycode;
				priv->lastKeyCode=priv->firstKeyCode;
				priv->pressedKeys=1;
				priv->keySequenceValid=_xfdashboard_hotkey_is_hotkey(self, (XKeyEvent*)inXEvent, NULL);
			}
				/* If this is another key event in the key sequence check if key
				 * is the same as the last key we handled. This can happen if a key
				 * is pressed but not release causing a key repeat. In this case
				 * we should not increase the counters.
				 */
				else if(((XKeyEvent*)inXEvent)->keycode!=priv->firstKeyCode)
				{
					/* Check if key sequence is still valid */
					if(priv->keySequenceValid &&
						!_xfdashboard_hotkey_is_hotkey(self, (XKeyEvent*)inXEvent, NULL))
					{
						priv->keySequenceValid=FALSE;
					}

					/* Remember last key pressed (to handle key repeat events) and
					 * increase counters.
					 */
					priv->lastKeyCode=((XKeyEvent*)inXEvent)->keycode;
					priv->pressedKeys++;
				}
			break;

		case KeyRelease:
			/* Decrease counters and forget last key pressed because this is a key release */
			priv->pressedKeys--;
			priv->lastKeyCode=0;

			/* If this is the last key in the key sequence which is being released
			 * check if key sequence is still valid and then emit signal.
			 */
			if(priv->pressedKeys==0)
			{
				/* Check that the last key released in the key sequence was also the
				 * first key pressed in the key sequence. Then check if key sequence
				 * is valid and emit signal.
				 */
				if(priv->firstKeyCode==((XKeyEvent*)inXEvent)->keycode &&
					priv->keySequenceValid)
				{
					/* Emit signal */
					g_debug("Last key was released and hotkey key sequence is valid - emitting signal");
					g_signal_emit(self, XfdashboardHotkeySignals[SIGNAL_ACTIVATE], 0);
				}
			}

			/* Should not happen but if we track a negative number of pressed keys
			 * then reset pressed keys counter to 0 but also ensure that hotkey was
			 * never pressed and key sequence is invalid.
			 */
			if(priv->pressedKeys<0)
			{
				priv->pressedKeys=0;
				priv->firstKeyCode=0;
				priv->keySequenceValid=FALSE;
			}
			break;

		default:
			break;
	}

	return(CLUTTER_X11_FILTER_CONTINUE);
}

/* An event at stage was received */
static gboolean _xfdashboard_hotkey_on_stage_event(XfdashboardHotkey *self,
													ClutterEvent *inEvent,
													gpointer inUserData)
{
	XfdashboardHotkeyPrivate	*priv;

	g_return_val_if_fail(XFDASHBOARD_IS_HOTKEY(self), CLUTTER_EVENT_PROPAGATE);
	g_return_val_if_fail(inEvent, CLUTTER_EVENT_PROPAGATE);
	g_return_val_if_fail(CLUTTER_IS_STAGE(inUserData), CLUTTER_EVENT_PROPAGATE);

	priv=self->priv;

	/* Only handle "key-press" and "key-release" events */
	switch(clutter_event_type(inEvent))
	{
		case CLUTTER_KEY_PRESS:
			/* If this is the first key pressed in key sequence then remember
			 * the key and set up counters.
			 */
			if(priv->pressedKeys==0)
			{
				priv->firstKeyCode=((ClutterKeyEvent*)inEvent)->hardware_keycode;
				priv->lastKeyCode=priv->firstKeyCode;
				priv->pressedKeys=1;
				priv->keySequenceValid=_xfdashboard_hotkey_is_hotkey(self, NULL, inEvent);
			}
				/* If this is another key event in the key sequence check if key
				 * is the same as the last key we handled. This can happen if a key
				 * is pressed but not release causing a key repeat. In this case
				 * we should not increase the counters.
				 */
				else if(((ClutterKeyEvent*)inEvent)->hardware_keycode!=priv->firstKeyCode)
				{
					if(priv->keySequenceValid &&
						!_xfdashboard_hotkey_is_hotkey(self, NULL, inEvent))
					{
						priv->keySequenceValid=FALSE;
					}

					priv->lastKeyCode=((ClutterKeyEvent*)inEvent)->hardware_keycode;
					priv->pressedKeys++;
				}
			break;

		case CLUTTER_KEY_RELEASE:
			/* Decrease counters and forget last key pressed because this is a key release */
			priv->pressedKeys--;
			priv->lastKeyCode=0;

			/* If this is the last key in the key sequence which is being released
			 * check if key sequence is still valid and then emit signal.
			 */
			if(priv->pressedKeys==0)
			{
				/* Check that the last key released in the key sequence was also the
				 * first key pressed in the key sequence. Then check if key sequence
				 * is valid and emit signal.
				 */
				if(priv->firstKeyCode==((ClutterKeyEvent*)inEvent)->hardware_keycode &&
					priv->keySequenceValid)
				{
					/* Emit signal*/
					g_debug("Last key was released and hotkey key sequence is valid - emitting signal");
					g_signal_emit(self, XfdashboardHotkeySignals[SIGNAL_ACTIVATE], 0);
				}
			}

			/* Should not happen but if we track a negative number of pressed keys
			 * then reset pressed keys counter to 0 but also ensure that hotkey was
			 * never pressed and key sequence is invalid.
			 */
			if(priv->pressedKeys<0)
			{
				priv->pressedKeys=0;
				priv->firstKeyCode=0;
				priv->keySequenceValid=FALSE;
			}
			break;

		default:
			break;
	}

	/* Do not prevent other actors to handle this event even if we did it */
	return(CLUTTER_EVENT_PROPAGATE);
}

/* A stage was added */
static void _xfdashboard_hotkey_on_stage_added(XfdashboardHotkey *self, ClutterStage *inStage, gpointer inUserData)
{
	g_return_if_fail(XFDASHBOARD_IS_HOTKEY(self));
	g_return_if_fail(CLUTTER_IS_STAGE(inStage));

	/* Connect signals to new stage for tracking key events */
	g_signal_connect_swapped(inStage, "event", G_CALLBACK(_xfdashboard_hotkey_on_stage_event), self);
}

/* A stage was removed */
static void _xfdashboard_hotkey_on_stage_removed(XfdashboardHotkey *self, ClutterStage *inStage, gpointer inUserData)
{
	g_return_if_fail(XFDASHBOARD_IS_HOTKEY(self));
	g_return_if_fail(CLUTTER_IS_STAGE(inStage));

	/* Disconnect signals from removed stage */
	g_signal_handlers_disconnect_by_func(inStage, G_CALLBACK(_xfdashboard_hotkey_on_stage_event), self);
}

/* IMPLEMENTATION: GObject */

/* Dispose this object */
static void _xfdashboard_hotkey_dispose(GObject *inObject)
{
	XfdashboardHotkey			*self=XFDASHBOARD_HOTKEY(inObject);
	XfdashboardHotkeyPrivate	*priv=self->priv;

	/* Release allocated resources */
	_xfdashboard_hotkey_on_focus_changed(self, TRUE);

	if(priv->display)
	{
		priv->display=NULL;
	}

	/* Call parent's class dispose method */
	G_OBJECT_CLASS(xfdashboard_hotkey_parent_class)->dispose(inObject);
}

/* Class initialization
 * Override functions in parent classes and define properties
 * and signals
 */
static void xfdashboard_hotkey_class_init(XfdashboardHotkeyClass *klass)
{
	GObjectClass					*gobjectClass=G_OBJECT_CLASS(klass);

	/* Override functions */
	gobjectClass->dispose=_xfdashboard_hotkey_dispose;

	/* Set up private structure */
	g_type_class_add_private(klass, sizeof(XfdashboardHotkeyPrivate));

	/* Define signals */
	XfdashboardHotkeySignals[SIGNAL_ACTIVATE]=
		g_signal_new("activate",
						G_TYPE_FROM_CLASS(klass),
						G_SIGNAL_RUN_LAST,
						G_STRUCT_OFFSET(XfdashboardHotkeyClass, activate),
						NULL,
						NULL,
						g_cclosure_marshal_VOID__VOID,
						G_TYPE_NONE,
						0);
}

/* Object initialization
 * Create private structure and set up default values
 */
static void xfdashboard_hotkey_init(XfdashboardHotkey *self)
{
	XfdashboardHotkeyPrivate	*priv;
	ClutterStageManager			*stageManager;
	GSList						*stages, *iter;

	priv=self->priv=XFDASHBOARD_HOTKEY_GET_PRIVATE(self);

	/* Set up default values */
	priv->display=clutter_x11_get_default_display();
	priv->currentFocus=None;
	priv->currentFocusRevert=0;
	priv->firstKeyCode=0;
	priv->lastKeyCode=0;
	priv->pressedKeys=0;
	priv->keySequenceValid=FALSE;

	/* Add and set up event filter for this instance */
	_xfdashboard_hotkey_on_focus_changed(self, FALSE);
	clutter_x11_add_filter(_xfdashboard_hotkey_on_x_event, self);

	/* Connect signals for stage creation and deletion events */
	stageManager=clutter_stage_manager_get_default();
	g_signal_connect_swapped(stageManager, "stage-added", G_CALLBACK(_xfdashboard_hotkey_on_stage_added), self);
	g_signal_connect_swapped(stageManager, "stage-removed", G_CALLBACK(_xfdashboard_hotkey_on_stage_removed), self);

	stages=clutter_stage_manager_list_stages(stageManager);
	for(iter=stages; iter; iter=g_slist_next(iter))
	{
		_xfdashboard_hotkey_on_stage_added(self, CLUTTER_STAGE(iter->data), stageManager);
	}
	g_slist_free(stages);
}

/* IMPLEMENTATION: Public API */

/* Get single instance of hotkey tracker */
XfdashboardHotkey* xfdashboard_hotkey_get_default(void)
{
	if(G_UNLIKELY(_xfdashboard_hotkey_singleton==NULL))
	{
		_xfdashboard_hotkey_singleton=g_object_new(XFDASHBOARD_TYPE_HOTKEY, NULL);
	}

	return(_xfdashboard_hotkey_singleton);
}
