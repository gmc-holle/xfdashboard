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
#include <X11/extensions/record.h>
#include <X11/XKBlib.h>

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
	Display						*controlConnection;
	Display						*dataConnection;
	XRecordRange				*recordRange;
	XRecordClientSpec			recordClients;
	XRecordContext				recordContext;
	guint						idleSourceID;

	KeyCode						firstKey;
	KeyCode						lastKey;
	gboolean					validSequence;
};

/* Signals */
enum
{
	SIGNAL_ACTIVATE,

	SIGNAL_LAST
};

static guint XfdashboardHotkeySignals[SIGNAL_LAST]={ 0, };

/* IMPLEMENTATION: Private variables and methods */
static XfdashboardHotkey					*_xfdashboard_hotkey_singleton=NULL;

/* Check if keycode is hotkey */
static gboolean _xfdashboard_hotkey_is_hotkey(XfdashboardHotkey *self, KeyCode inKeyCode)
{
	XfdashboardHotkeyPrivate	*priv;
	KeySym						keySym;

	g_return_val_if_fail(XFDASHBOARD_IS_HOTKEY(self), FALSE);

	priv=self->priv;

	/* Translate key-code to key-sym-code */
	keySym=XkbKeycodeToKeysym(priv->controlConnection, inKeyCode, 0, 0);
	g_debug("Converted key-code %u to key-sym %lu", inKeyCode, keySym);

	/* Return TRUE here if key-sym-code is the hotkey */
	if(keySym==CLUTTER_KEY_Super_L ||
		keySym==CLUTTER_KEY_Super_R)
	{
		return(TRUE);
	}

	/* If we get here the key was not the hotkey */
	return(FALSE);
}

/* Process collected recorded data by record extenstion when idle */
static gboolean _xfdashboard_hotkey_on_idle(gpointer inUserData)
{
	XfdashboardHotkey			*self;
	XfdashboardHotkeyPrivate	*priv;

	g_return_val_if_fail(XFDASHBOARD_IS_HOTKEY(inUserData), G_SOURCE_CONTINUE);

	self=XFDASHBOARD_HOTKEY(inUserData);
	priv=self->priv;

	/* Process collected recorded data */
	if(priv->dataConnection)
	{
		XRecordProcessReplies(priv->dataConnection);
	}

	/* Keep this source */
	return(G_SOURCE_CONTINUE);
}

/* Record extension got data and called callback */
static void _xfdashboard_hotkey_on_record_data(XPointer inClosure, XRecordInterceptData *inRecordedData)
{
	XfdashboardHotkey			*self;
	XfdashboardHotkeyPrivate	*priv;
	int							eventType;
	KeyCode						eventKeyCode;

	/* We need to free recorded data when returning so do not use
	 * the g_return*_if_fail() macros as usual.
	 */
	if(!XFDASHBOARD_IS_HOTKEY(inClosure))
	{
		XRecordFreeData(inRecordedData);
		return;
	}

	self=XFDASHBOARD_HOTKEY(inClosure);
	priv=self->priv;

	/* Check for record data */
	if(inRecordedData->category!=XRecordFromServer)
	{
		XRecordFreeData(inRecordedData);
		return;
	}

	/* Handle recorded event */
	if(inRecordedData->data)
	{
		eventType=inRecordedData->data[0];
		eventKeyCode=inRecordedData->data[1];
		switch(eventType)
		{
			case KeyPress:
				/* If this is the first key pressed in sequence then remember
				 * the key as first key in sequence to recognize end of
				 * sequence when this key is released. Also remember this key
				 * as last key pressed to recognize key-repeat events.
				 * Set flag if sequence is valid by checking if pressed key
				 * is a hotkey.
				 */
				if(!priv->firstKey)
				{
					priv->firstKey=eventKeyCode;
					priv->lastKey=eventKeyCode;
					priv->validSequence=_xfdashboard_hotkey_is_hotkey(self, eventKeyCode);
					g_message("First key with key-code %u in %s sequence pressed",
								eventKeyCode,
								priv->validSequence ? "valid" : "invalid");
				}
					/* If this pressed key is not the first key then check if it
					 * is the same as the last key pressed. This can happen if a key
					 * is pressed but not released causing a key repeat. In this case
					 * we should not do anything.
					 */
					else if(eventKeyCode!=priv->lastKey)
					{
						/* Check if key sequence is still valid */
						if(priv->validSequence &&
							!_xfdashboard_hotkey_is_hotkey(self, eventKeyCode))
						{
							priv->validSequence=FALSE;
						}

						/* Remember last key pressed to handle key repeat events */
						priv->lastKey=eventKeyCode;
						g_message("Next key with key-code %u in %s sequence pressed",
									eventKeyCode,
									priv->validSequence ? "valid" : "invalid");
					}
				break;

			case KeyRelease:
				/* Forget last key pressed because this is a key release */
				priv->lastKey=0;

				/* Check that this released key in the key sequence was also the
				 * first key pressed in the key sequence. Then check if key sequence
				 * is valid and emit signal.
				 */
				if(eventKeyCode==priv->firstKey)
				{
					/* Check if key sequence is valid and emit signal */
					if(priv->validSequence)
					{
						g_message("Emitting signal because first key in valid sequence was released");
						g_signal_emit(self, XfdashboardHotkeySignals[SIGNAL_ACTIVATE], 0);
					}
						else g_message("Key sequence ended but no signal will be emitted because of invalid sequence");

					/* Forget first key and reset valid flag of key sequence
					 * so another sequence can start.
					 */
					priv->firstKey=0;
					priv->validSequence=FALSE;
				}
				break;

			default:
				/* Should never get here but do nothing - just in case */
				g_message("Got an unexpected event in recorded data in hotkey");
				break;
		}
	}

	/* Free recorded data */
	XRecordFreeData(inRecordedData);
}

/* Disable and release record extension */
static void _xfdashboard_hotkey_release(XfdashboardHotkey *self)
{
	XfdashboardHotkeyPrivate	*priv;

	g_return_if_fail(XFDASHBOARD_IS_HOTKEY(self));

	priv=self->priv;

	/* Release allocated resources */
	priv->recordClients=None;

	if(priv->recordContext!=None)
	{
		g_debug("Releasing record context of hotkey");
		XRecordDisableContext(priv->controlConnection, priv->recordContext);
		XRecordFreeContext(priv->controlConnection, priv->recordContext);
		priv->recordContext=None;
	}

	if(priv->recordRange)
	{
		g_debug("Releasing record range used in record context");
		XFree(priv->recordRange);
		priv->recordRange=NULL;
	}

	if(priv->dataConnection)
	{
		g_debug("Closing data connection of hotkey");
		XCloseDisplay(priv->dataConnection);
		priv->dataConnection=NULL;
	}

	if(priv->controlConnection)
	{
		g_debug("Releasing control connection of hotkey");
		XCloseDisplay(priv->controlConnection);
		priv->controlConnection=NULL;
	}

	if(priv->idleSourceID)
	{
		g_debug("Removing idle source");
		g_source_remove(priv->idleSourceID);
		priv->idleSourceID=0;
	}

	priv->firstKey=0;
	priv->lastKey=0;
	priv->validSequence=FALSE;

	g_debug("Disabling record context for hotkey was successful");
}

/* Enable and set up record extension */
static gboolean _xfdashboard_hotkey_setup(XfdashboardHotkey *self)
{
	XfdashboardHotkeyPrivate	*priv;
	int							recordVersionMajor;
	int							recordVersionMinor;

	g_return_val_if_fail(XFDASHBOARD_IS_HOTKEY(self), FALSE);

	priv=self->priv;

	/* Check if anything is released and cleaned otherwise we cannot
	 * set up record.
	 */
	if(priv->controlConnection ||
		priv->dataConnection ||
		priv->recordRange ||
		priv->recordClients!=None ||
		priv->recordContext!=None ||
		priv->idleSourceID)
	{
		g_critical(_("Cannot enable hotkey: Unclean state"));
		return(FALSE);
	}

	/* Reset internal tracking states */
	priv->firstKey=0;
	priv->lastKey=0;
	priv->validSequence=FALSE;

	/* Open connections */
	g_debug("Opening control and data connections for hotkey");
	priv->controlConnection=XOpenDisplay(NULL);
	priv->dataConnection=XOpenDisplay(NULL);
	XSynchronize(priv->controlConnection, True);

	/* Check if record extenstion is available */
	g_debug("Query version of XRECORD extension ");
	if(!XRecordQueryVersion(priv->controlConnection, &recordVersionMajor, &recordVersionMinor))
	{
		g_warning(_("Cannot enable hotkey: X Record Extension is not supported"));
		_xfdashboard_hotkey_release(self);
		return(FALSE);
	}
	g_debug("X Record Extension version is %d.%d", recordVersionMajor, recordVersionMinor);

	/* Allocate and set up record range */
	g_debug("Allocate and set up record range for use in record context");
	priv->recordRange=XRecordAllocRange();
	if(!priv->recordRange)
	{
		g_warning(_("Cannot enable hotkey: Could not allocate record range"));
		_xfdashboard_hotkey_release(self);
		return(FALSE);
	}

	priv->recordRange->device_events.first=KeyPress;
	priv->recordRange->device_events.last=KeyRelease;

	/* Set up clients to record */
	priv->recordClients=XRecordAllClients;

	/* Set up record context */
	g_debug("Creating record context for hotkey");
	priv->recordContext=XRecordCreateContext(priv->controlConnection, 0, &priv->recordClients, 1, &priv->recordRange, 1);
	if(!priv->recordContext)
	{
		g_warning(_("Cannot enable hotkey: Could not create a record context"));
		_xfdashboard_hotkey_release(self);
		return(FALSE);
	}

	/* Start "recording" */
	g_debug("Enabling record context asynchronously for hotkey");
	if(!XRecordEnableContextAsync(priv->dataConnection, priv->recordContext, _xfdashboard_hotkey_on_record_data, (XPointer)self))
	{
		g_warning(_("Cannot enable hotkey: Could not enable record context"));
		_xfdashboard_hotkey_release(self);
		return(FALSE);
	}

	/* Set up idle source to process collected record data every time
	 * the application is idle.
	 */
	g_debug("Adding idle source");
	priv->idleSourceID=clutter_threads_add_idle(_xfdashboard_hotkey_on_idle, self);

	/* If we get here set up and enabling record context was successful. */
	g_debug("Set up and enabling record context for hotkey was successful");
	return(TRUE);
}

/* IMPLEMENTATION: GObject */

/* Dispose this object */
static void _xfdashboard_hotkey_dispose(GObject *inObject)
{
	XfdashboardHotkey			*self=XFDASHBOARD_HOTKEY(inObject);

	/* Release allocated resources */
	_xfdashboard_hotkey_release(self);

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

	priv=self->priv=XFDASHBOARD_HOTKEY_GET_PRIVATE(self);

	/* Set up default values */
	priv->controlConnection=NULL;
	priv->dataConnection=NULL;
	priv->recordRange=NULL;
	priv->recordClients=None;
	priv->recordContext=None;
	priv->idleSourceID=0;
	priv->firstKey=0;
	priv->lastKey=0;
	priv->validSequence=FALSE;

	/* Enable record */
	_xfdashboard_hotkey_setup(self);
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
