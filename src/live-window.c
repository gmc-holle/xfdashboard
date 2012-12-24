/*
 * live-window.c: An actor showing and updating a window live
 * 
 * Copyright 2012 Stephan Haller <nomad@froevel.de>
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

#include "live-window.h"
#include "button.h"

#include <clutter/x11/clutter-x11.h>
#include <math.h>

/* Define this class in GObject system */

G_DEFINE_TYPE(XfdashboardLiveWindow,
				xfdashboard_live_window,
				CLUTTER_TYPE_ACTOR)
                                                
/* Private structure - access only by public API if needed */
#define XFDASHBOARD_LIVE_WINDOW_GET_PRIVATE(obj) \
	(G_TYPE_INSTANCE_GET_PRIVATE((obj), XFDASHBOARD_TYPE_LIVE_WINDOW, XfdashboardLiveWindowPrivate))

struct _XfdashboardLiveWindowPrivate
{
	/* Actors for live window */
	ClutterActor		*actorWindow;
	ClutterActor		*actorLabel;
	ClutterActor		*actorClose;

	/* Window the actors belong to */
	WnckWindow			*window;
	gulong				signalActionsChangedID;
	gulong				signalGeometryChangedID;
	gulong				signalIconChangedID;
	gulong				signalNameChangedID;
	gulong				signalStateChangedID;
	gulong				signalWorkspaceChangedID;
	gboolean			canClose;
	gboolean			isVisible;

	/* Actor actions */
	ClutterAction		*clickAction;
	gboolean			wasClosedClicked;

	/* Settings */
	gchar				*labelFont;
	ClutterColor		*labelTextColor;
	ClutterColor		*labelBackgroundColor;
	gfloat				labelMargin;
	PangoEllipsizeMode	labelEllipsize;
};

/* Properties */
enum
{
	PROP_0,

	PROP_WINDOW,
	PROP_LABEL_FONT,
	PROP_LABEL_COLOR,
	PROP_LABEL_BACKGROUND_COLOR,
	PROP_LABEL_MARGIN,
	PROP_LABEL_ELLIPSIZE_MODE,
	
	PROP_LAST
};

static GParamSpec* XfdashboardLiveWindowProperties[PROP_LAST]={ 0, };

/* Signals */
enum
{
	CLICKED,
	CLOSE,
	GEOMETRY_CHANGED,
	VISIBILITY_CHANGED,
	WORKSPACE_CHANGED,

	SIGNAL_LAST
};

static guint XfdashboardLiveWindowSignals[SIGNAL_LAST]={ 0, };

/* Private constants */
static ClutterColor		defaultTextColor={ 0xff, 0xff , 0xff, 0xff };
static ClutterColor		defaultBackgroundColor={ 0x00, 0x00, 0x00, 0xd0 };
static gint				interestingVisibilityStates=(WNCK_WINDOW_STATE_SKIP_PAGER |
														WNCK_WINDOW_STATE_SKIP_TASKLIST |
														WNCK_WINDOW_STATE_HIDDEN);

/* IMPLEMENTATION: Private variables and methods */

/* Window's action has changed */
void _xfdashboard_live_window_on_actions_changed(WnckWindow *inWindow,
													WnckWindowActions inChangedMask,
													WnckWindowActions inNewState,
													gpointer inUserData)
{
	g_return_if_fail(XFDASHBOARD_IS_LIVE_WINDOW(inUserData));

	XfdashboardLiveWindow			*self=XFDASHBOARD_LIVE_WINDOW(inUserData);
	XfdashboardLiveWindowPrivate	*priv=self->priv;

	/* Check if action on "close" has changed */
	if(inChangedMask & WNCK_WINDOW_ACTION_CLOSE)
	{
		gboolean	canClose;
		
		canClose=((inNewState & WNCK_WINDOW_ACTION_CLOSE) ? TRUE : FALSE);
		if(priv->canClose!=canClose)
		{
			priv->canClose=canClose;
			clutter_actor_queue_redraw(CLUTTER_ACTOR(self));
		}
	}
}

/* Window's position and/or size (geometry) has changed */
void _xfdashboard_live_window_on_geometry_changed(WnckWindow *inWindow, gpointer inUserData)
{
	g_return_if_fail(XFDASHBOARD_IS_LIVE_WINDOW(inUserData));

	XfdashboardLiveWindow			*self=XFDASHBOARD_LIVE_WINDOW(inUserData);

	/* Emit "geometry-changed" signal */
	g_signal_emit(self, XfdashboardLiveWindowSignals[GEOMETRY_CHANGED], 0);
}

/* Window icon changed */
void _xfdashboard_live_window_on_icon_changed(WnckWindow *inWindow, gpointer inUserData)
{
	g_return_if_fail(XFDASHBOARD_IS_LIVE_WINDOW(inUserData));

	XfdashboardLiveWindow			*self=XFDASHBOARD_LIVE_WINDOW(inUserData);
	XfdashboardLiveWindowPrivate	*priv=self->priv;

	/* Set new window icon in label actor */
	xfdashboard_button_set_icon_pixbuf(XFDASHBOARD_BUTTON(priv->actorLabel), wnck_window_get_icon(priv->window));
}

/* Window title changed */
void _xfdashboard_live_window_on_title_changed(WnckWindow *inWindow, gpointer inUserData)
{
	g_return_if_fail(XFDASHBOARD_IS_LIVE_WINDOW(inUserData));

	XfdashboardLiveWindow			*self=XFDASHBOARD_LIVE_WINDOW(inUserData);
	XfdashboardLiveWindowPrivate	*priv=self->priv;

	/* Set new window title in label actor */
	xfdashboard_button_set_text(XFDASHBOARD_BUTTON(priv->actorLabel), wnck_window_get_name(priv->window));
}

/* Window's state has changed */
void _xfdashboard_live_window_on_state_changed(WnckWindow *inWindow,
												WnckWindowState inChangedMask,
												WnckWindowState inNewState,
												gpointer inUserData)
{
	g_return_if_fail(XFDASHBOARD_IS_LIVE_WINDOW(inUserData));

	XfdashboardLiveWindow			*self=XFDASHBOARD_LIVE_WINDOW(inUserData);
	XfdashboardLiveWindowPrivate	*priv=self->priv;

	/* Check if state in interessant states has changed */
	if(inChangedMask & interestingVisibilityStates)
	{
		gboolean	isVisible;
		
		isVisible=((inNewState & interestingVisibilityStates) ? FALSE : TRUE);
		if(priv->isVisible!=isVisible)
		{
			priv->isVisible=isVisible;
			g_signal_emit(self, XfdashboardLiveWindowSignals[VISIBILITY_CHANGED], 0);
		}
	}
}

/* Window's workspace has changed */
void _xfdashboard_live_window_on_workspace_changed(WnckWindow *inWindow, gpointer inUserData)
{
	g_return_if_fail(XFDASHBOARD_IS_LIVE_WINDOW(inUserData));

	XfdashboardLiveWindow			*self=XFDASHBOARD_LIVE_WINDOW(inUserData);

	/* Emit "workspace-changed" signal */
	g_signal_emit(self, XfdashboardLiveWindowSignals[WORKSPACE_CHANGED], 0);
}

/* "Close" button of this actor was clicked */
void _xfdashboard_live_window_on_close_clicked(ClutterActor *inActor, gpointer inUserData)
{
	XfdashboardLiveWindow			*self=XFDASHBOARD_LIVE_WINDOW(inActor);
	XfdashboardLiveWindowPrivate	*priv=self->priv;

	/* Emit "close" signal */
	g_signal_emit(inActor, XfdashboardLiveWindowSignals[CLOSE], 0);

	/* Set flag that closed button was clicked to prevent
	 * a click-fallthrough to live window */
	priv->wasClosedClicked=TRUE;
}

/* Set window and setup actors for display live image of window with accessiors */
void _xfdashboard_live_window_set_window(XfdashboardLiveWindow *self, const WnckWindow *inWindow)
{
	g_return_if_fail(XFDASHBOARD_IS_LIVE_WINDOW(self));
	g_return_if_fail(WNCK_IS_WINDOW(inWindow));

	XfdashboardLiveWindowPrivate	*priv=XFDASHBOARD_LIVE_WINDOW(self)->priv;
	XfdashboardButton				*button=NULL;

	/* Set window and connect signals to window */
	g_return_if_fail(priv->window==NULL);

	priv->window=(WnckWindow*)inWindow;
	priv->canClose=((wnck_window_get_actions(priv->window) & WNCK_WINDOW_ACTION_CLOSE) ? TRUE : FALSE);
	priv->isVisible=((wnck_window_get_state(priv->window) & interestingVisibilityStates) ? FALSE : TRUE);

	priv->signalActionsChangedID=g_signal_connect(priv->window,
													"actions-changed",
													G_CALLBACK(_xfdashboard_live_window_on_actions_changed),
													self);
	priv->signalGeometryChangedID=g_signal_connect(priv->window,
													"icon-changed",
													G_CALLBACK(_xfdashboard_live_window_on_geometry_changed),
													self);
	priv->signalIconChangedID=g_signal_connect(priv->window,
												"icon-changed",
												G_CALLBACK(_xfdashboard_live_window_on_icon_changed),
												self);
	priv->signalNameChangedID=g_signal_connect(priv->window,
												"name-changed",
												G_CALLBACK(_xfdashboard_live_window_on_title_changed),
												self);
	priv->signalWorkspaceChangedID=g_signal_connect(priv->window,
													"workspace-changed",
													G_CALLBACK(_xfdashboard_live_window_on_workspace_changed),
													self);
	priv->signalStateChangedID=g_signal_connect(priv->window,
													"state-changed",
													G_CALLBACK(_xfdashboard_live_window_on_state_changed),
													self);

	/* Create live-window */
	priv->actorWindow=clutter_x11_texture_pixmap_new_with_window(wnck_window_get_xid(priv->window));
	clutter_actor_set_parent(priv->actorWindow, CLUTTER_ACTOR(self));

	clutter_x11_texture_pixmap_set_automatic(CLUTTER_X11_TEXTURE_PIXMAP(priv->actorWindow), TRUE);

	/* Create label with icon and background*/
	priv->actorLabel=xfdashboard_button_new_with_text(wnck_window_get_name(priv->window));
	button=XFDASHBOARD_BUTTON(priv->actorLabel);
	
	xfdashboard_button_set_style(button, XFDASHBOARD_STYLE_BOTH);
	xfdashboard_button_set_icon_pixbuf(button, wnck_window_get_icon(priv->window));
	xfdashboard_button_set_margin(button, priv->labelMargin);
	xfdashboard_button_set_font(button, priv->labelFont);
	if(priv->labelTextColor) xfdashboard_button_set_color(button, priv->labelTextColor);
	xfdashboard_button_set_ellipsize_mode(button, priv->labelEllipsize);
	xfdashboard_button_set_background_visibility(button, TRUE);
	if(priv->labelBackgroundColor) xfdashboard_button_set_background_color(button, priv->labelBackgroundColor);
	clutter_actor_set_reactive(priv->actorLabel, FALSE);
	clutter_actor_set_parent(priv->actorLabel, CLUTTER_ACTOR(self));

	/* Create close button */
	priv->actorClose=xfdashboard_button_new_with_icon(GTK_STOCK_CLOSE);
	button=XFDASHBOARD_BUTTON(priv->actorClose);

	xfdashboard_button_set_background_visibility(button, TRUE);
	clutter_actor_set_parent(priv->actorClose, CLUTTER_ACTOR(self));
	g_signal_connect_swapped(priv->actorClose, "clicked", G_CALLBACK(_xfdashboard_live_window_on_close_clicked), self);

	/* Queue a redraw as the actors are now available */
	clutter_actor_queue_redraw(CLUTTER_ACTOR(self));
}

/* IMPLEMENTATION: ClutterActor */

/* Show all children of this one */
static void xfdashboard_live_window_show_all(ClutterActor *self)
{
	XfdashboardLiveWindowPrivate	*priv=XFDASHBOARD_LIVE_WINDOW(self)->priv;

	clutter_actor_show(priv->actorWindow);
	clutter_actor_show(priv->actorLabel);
	clutter_actor_show(priv->actorClose);
	clutter_actor_show(self);
}

/* Hide all children of this one */
static void xfdashboard_live_window_hide_all(ClutterActor *self)
{
	XfdashboardLiveWindowPrivate	*priv=XFDASHBOARD_LIVE_WINDOW(self)->priv;

	clutter_actor_hide(self);
	clutter_actor_hide(priv->actorWindow);
	clutter_actor_hide(priv->actorLabel);
	clutter_actor_hide(priv->actorClose);
}

/* Get preferred width/height */
static void xfdashboard_live_window_get_preferred_height(ClutterActor *self,
														gfloat inForWidth,
														gfloat *outMinHeight,
														gfloat *outNaturalHeight)
{
	XfdashboardLiveWindowPrivate	*priv=XFDASHBOARD_LIVE_WINDOW(self)->priv;
	gfloat							minHeight, naturalHeight;
	gfloat							childMinHeight, childNaturalHeight;

	minHeight=naturalHeight=0.0f;

	/* Determine size of window if visible (should usually be the largest actor) */
	if(CLUTTER_ACTOR_IS_VISIBLE(priv->actorWindow))
	{
		clutter_actor_get_preferred_height(priv->actorWindow,
											inForWidth,
											&childMinHeight,
											&childNaturalHeight);
		if(childMinHeight>minHeight) minHeight=childMinHeight;
		if(childNaturalHeight>naturalHeight) naturalHeight=childNaturalHeight;
	}

	/* Determine size of label if visible */
	if(CLUTTER_ACTOR_IS_VISIBLE(priv->actorLabel))
	{
		clutter_actor_get_preferred_height(priv->actorLabel,
											inForWidth,
											&childMinHeight,
											&childNaturalHeight);
		if(childMinHeight>minHeight) minHeight=childMinHeight;
		if(childNaturalHeight>naturalHeight) naturalHeight=childNaturalHeight;
	}

	/* Determine size of close button if visible */
	if(CLUTTER_ACTOR_IS_VISIBLE(priv->actorClose))
	{
		clutter_actor_get_preferred_height(priv->actorClose,
											inForWidth,
											&childMinHeight,
											&childNaturalHeight);
		if(childMinHeight>minHeight) minHeight=childMinHeight;
		if(childNaturalHeight>naturalHeight) naturalHeight=childNaturalHeight;
	}

	/* Store sizes computed */
	if(outMinHeight) *outMinHeight=minHeight;
	if(outNaturalHeight) *outNaturalHeight=naturalHeight;
}

static void xfdashboard_live_window_get_preferred_width(ClutterActor *self,
														gfloat inForHeight,
														gfloat *outMinWidth,
														gfloat *outNaturalWidth)
{
	XfdashboardLiveWindowPrivate	*priv=XFDASHBOARD_LIVE_WINDOW(self)->priv;
	gfloat							minWidth, naturalWidth;
	gfloat							childMinWidth, childNaturalWidth;

	minWidth=naturalWidth=0.0f;

	/* Determine size of window if visible (should usually be the largest actor) */
	if(CLUTTER_ACTOR_IS_VISIBLE(priv->actorWindow))
	{
		clutter_actor_get_preferred_width(priv->actorWindow,
											inForHeight,
											&childMinWidth,
											&childNaturalWidth);
		if(childMinWidth>minWidth) minWidth=childMinWidth;
		if(childNaturalWidth>naturalWidth) naturalWidth=childNaturalWidth;
	}

	/* Determine size of label if visible */
	if(CLUTTER_ACTOR_IS_VISIBLE(priv->actorLabel))
	{
		clutter_actor_get_preferred_width(priv->actorLabel,
											inForHeight,
											&childMinWidth,
											&childNaturalWidth);
		if(childMinWidth>minWidth) minWidth=childMinWidth;
		if(childNaturalWidth>naturalWidth) naturalWidth=childNaturalWidth;
	}

	/* Determine size of close button if visible */
	if(CLUTTER_ACTOR_IS_VISIBLE(priv->actorClose))
	{
		clutter_actor_get_preferred_width(priv->actorClose,
											inForHeight,
											&childMinWidth,
											&childNaturalWidth);
		if(childMinWidth>minWidth) minWidth=childMinWidth;
		if(childNaturalWidth>naturalWidth) naturalWidth=childNaturalWidth;
	}

	/* Store sizes computed */
	if(outMinWidth) *outMinWidth=minWidth;
	if(outNaturalWidth) *outNaturalWidth=naturalWidth;
}

/* Allocate position and size of actor and its children*/
static void xfdashboard_live_window_allocate(ClutterActor *self,
											const ClutterActorBox *inBox,
											ClutterAllocationFlags inFlags)
{
	XfdashboardLiveWindowPrivate	*priv=XFDASHBOARD_LIVE_WINDOW(self)->priv;

	/* Chain up to store the allocation of the actor */
	CLUTTER_ACTOR_CLASS(xfdashboard_live_window_parent_class)->allocate(self, inBox, inFlags);

	/* Set window actor by getting geometry of window and
	 * determining position and size */
	ClutterActorBox				*boxActorWindow;
	int							winX, winY, winW, winH;
	gfloat						scaleW=1.0f, scaleH=1.0f;
	gfloat						newW, newH;
	gfloat						left, top, right, bottom;

	wnck_window_get_client_window_geometry(priv->window, &winX, &winY, &winW, &winH);
	if(winW>winH)
	{
		newW=clutter_actor_box_get_width(inBox);
		newH=clutter_actor_box_get_width(inBox)*((gfloat)winH/(gfloat)winW);
	}
		else
		{
			newW=clutter_actor_box_get_height(inBox)*((gfloat)winW/(gfloat)winH);
			newH=clutter_actor_box_get_height(inBox);
		}

	if(newW>clutter_actor_box_get_width(inBox)) scaleW=(clutter_actor_box_get_width(inBox)/newW);
	if(newH>clutter_actor_box_get_height(inBox)) scaleH=(clutter_actor_box_get_height(inBox)/newH);
	newW*=(scaleW<scaleH ? scaleW : scaleH);
	newH*=(scaleW<scaleH ? scaleW : scaleH);
	left=(clutter_actor_box_get_width(inBox)-newW)/2;
	right=left+newW;
	top=(clutter_actor_box_get_height(inBox)-newH)/2;
	bottom=top+newH;
	boxActorWindow=clutter_actor_box_new(left, top, right, bottom);
	clutter_actor_allocate(priv->actorWindow, boxActorWindow, inFlags);

	/* Set label actors */
	ClutterActorBox				*boxActorLabel;
	gfloat						labelWidth, labelHeight, maxWidth;

	clutter_actor_get_preferred_size(priv->actorLabel,
										NULL, NULL,
										&labelWidth, &labelHeight);

	maxWidth=clutter_actor_box_get_width(boxActorWindow)-(2*priv->labelMargin);
	if(labelWidth>maxWidth) labelWidth=maxWidth;

	left=clutter_actor_box_get_x(boxActorWindow)+((clutter_actor_box_get_width(boxActorWindow)-labelWidth)/2.0f);
	right=left+labelWidth;
	bottom=clutter_actor_box_get_y(boxActorWindow)+clutter_actor_box_get_height(boxActorWindow)-(2*priv->labelMargin);
	top=bottom-labelHeight;
	if(left>right) left=right-1.0f;
	boxActorLabel=clutter_actor_box_new(floor(left), floor(top), floor(right), floor(bottom));
	clutter_actor_allocate(priv->actorLabel, boxActorLabel, inFlags);

	/* Set close actor */
	ClutterActorBox				*boxActorClose;
	gfloat						buttonWidth, buttonHeight;

	clutter_actor_get_preferred_size(priv->actorClose,
										NULL, NULL,
										&buttonWidth, &buttonHeight);
										
	right=clutter_actor_box_get_x(boxActorWindow)+clutter_actor_box_get_width(boxActorWindow)-priv->labelMargin;
	left=MAX(right-buttonWidth, priv->labelMargin);
	top=clutter_actor_box_get_y(boxActorWindow)+priv->labelMargin;
	bottom=top+buttonHeight;
	boxActorClose=clutter_actor_box_new(floor(left), floor(top), floor(right), floor(bottom));
	clutter_actor_allocate(priv->actorClose, boxActorClose, inFlags);

	/* Release allocated memory */
	clutter_actor_box_free(boxActorWindow);
	clutter_actor_box_free(boxActorLabel);
	clutter_actor_box_free(boxActorClose);
}

/* Paint actor */
static void xfdashboard_live_window_paint(ClutterActor *self)
{
	XfdashboardLiveWindowPrivate	*priv=XFDASHBOARD_LIVE_WINDOW(self)->priv;

	/* Order of actors being painted is important! */
	if(priv->actorWindow &&
		CLUTTER_ACTOR_IS_MAPPED(priv->actorWindow)) clutter_actor_paint(priv->actorWindow);

	if(priv->actorLabel &&
		CLUTTER_ACTOR_IS_MAPPED(priv->actorLabel)) clutter_actor_paint(priv->actorLabel);

	if(priv->actorClose &&
		CLUTTER_ACTOR_IS_MAPPED(priv->actorClose) &&
		priv->canClose) clutter_actor_paint(priv->actorClose);
}

/* Pick this actor and possibly all the child actors.
 * That means this function should paint its silouhette as a solid shape in the
 * given color and call the paint function of its children. But never call the
 * paint function of itself especially if the paint function sets a different
 * color, e.g. by cogl_set_source_color* function family.
 */
static void xfdashboard_live_window_pick(ClutterActor *self, const ClutterColor *inColor)
{
	XfdashboardLiveWindowPrivate	*priv=XFDASHBOARD_LIVE_WINDOW(self)->priv;

	/* It is possible to avoid a costly paint by checking
	 * whether the actor should really be painted in pick mode
	 */
	if(!clutter_actor_should_pick_paint(self)) return;

	/* Chain up so we get a bounding box painted (if we are reactive) */
	CLUTTER_ACTOR_CLASS(xfdashboard_live_window_parent_class)->pick(self, inColor);

	/* Pick children */
	if(priv->actorWindow &&
		CLUTTER_ACTOR_IS_MAPPED(priv->actorWindow)) clutter_actor_paint(priv->actorWindow);

	if(priv->actorLabel &&
		CLUTTER_ACTOR_IS_MAPPED(priv->actorLabel)) clutter_actor_paint(priv->actorLabel);

	if(priv->actorClose &&
		CLUTTER_ACTOR_IS_MAPPED(priv->actorClose) &&
		priv->canClose) clutter_actor_paint(priv->actorClose);
}

/* proxy ClickAction signals */
static void xfdashboard_live_window_clicked(ClutterClickAction *inAction,
											ClutterActor *inActor,
											gpointer inUserData)
{
	g_return_if_fail(XFDASHBOARD_IS_LIVE_WINDOW(inActor));

	/* Only emit click signal if close button was not pressed before. If the closed button
	 * was pressed before its click signal was handled but clutter keeps falling through
	 * this element and causes the click action for live window to be emitted and handled here */
	XfdashboardLiveWindowPrivate	*priv=XFDASHBOARD_LIVE_WINDOW(inActor)->priv;

	if(!priv->wasClosedClicked) g_signal_emit(inActor, XfdashboardLiveWindowSignals[CLICKED], 0);
	priv->wasClosedClicked=FALSE;
}

/* Destroy this actor */
static void xfdashboard_live_window_destroy(ClutterActor *self)
{
	/* Destroy each child actor when this actor is destroyed */
	XfdashboardLiveWindowPrivate	*priv=XFDASHBOARD_LIVE_WINDOW(self)->priv;

	if(priv->actorWindow) clutter_actor_destroy(priv->actorWindow);
	priv->actorWindow=NULL;

	if(priv->actorLabel) clutter_actor_destroy(priv->actorLabel);
	priv->actorLabel=NULL;

	if(priv->actorClose) clutter_actor_destroy(priv->actorClose);
	priv->actorClose=NULL;

	/* Call parent's class destroy method */
	if(CLUTTER_ACTOR_CLASS(xfdashboard_live_window_parent_class)->destroy)
	{
		CLUTTER_ACTOR_CLASS(xfdashboard_live_window_parent_class)->destroy(self);
	}
}

/* IMPLEMENTATION: GObject */

/* Dispose this object */
static void xfdashboard_live_window_dispose(GObject *inObject)
{
	/* Release our allocated variables */
	XfdashboardLiveWindowPrivate	*priv=XFDASHBOARD_LIVE_WINDOW(inObject)->priv;

	if(priv->window)
	{
		if(priv->signalActionsChangedID &&
			g_signal_handler_is_connected(priv->window, priv->signalActionsChangedID))
		{
			g_signal_handler_disconnect(priv->window, priv->signalActionsChangedID);
			priv->signalActionsChangedID=0L;
		}

		if(priv->signalGeometryChangedID &&
			g_signal_handler_is_connected(priv->window, priv->signalGeometryChangedID))
		{
			g_signal_handler_disconnect(priv->window, priv->signalGeometryChangedID);
			priv->signalGeometryChangedID=0L;
		}

		if(priv->signalIconChangedID &&
			g_signal_handler_is_connected(priv->window, priv->signalIconChangedID))
		{
			g_signal_handler_disconnect(priv->window, priv->signalIconChangedID);
			priv->signalIconChangedID=0L;
		}

		if(priv->signalNameChangedID &&
			g_signal_handler_is_connected(priv->window, priv->signalNameChangedID))
		{
			g_signal_handler_disconnect(priv->window, priv->signalNameChangedID);
			priv->signalNameChangedID=0L;
		}

		if(priv->signalStateChangedID &&
			g_signal_handler_is_connected(priv->window, priv->signalStateChangedID))
		{
			g_signal_handler_disconnect(priv->window, priv->signalStateChangedID);
			priv->signalStateChangedID=0L;
		}

		if(priv->signalWorkspaceChangedID &&
			g_signal_handler_is_connected(priv->window, priv->signalWorkspaceChangedID))
		{
			g_signal_handler_disconnect(priv->window, priv->signalWorkspaceChangedID);
			priv->signalWorkspaceChangedID=0L;
		}
	}
	
	if(priv->labelFont) g_free(priv->labelFont);
	priv->labelFont=NULL;

	if(priv->labelTextColor) clutter_color_free(priv->labelTextColor);
	priv->labelTextColor=NULL;
	
	if(priv->labelBackgroundColor) clutter_color_free(priv->labelBackgroundColor);
	priv->labelBackgroundColor=NULL;

	/* Call parent's class dispose method */
	G_OBJECT_CLASS(xfdashboard_live_window_parent_class)->dispose(inObject);
}

/* Set/get properties */
static void xfdashboard_live_window_set_property(GObject *inObject,
												guint inPropID,
												const GValue *inValue,
												GParamSpec *inSpec)
{
	XfdashboardLiveWindow			*self=XFDASHBOARD_LIVE_WINDOW(inObject);

	switch(inPropID)
	{
		case PROP_WINDOW:
			_xfdashboard_live_window_set_window(self, g_value_get_object(inValue));
			break;
			
		case PROP_LABEL_FONT:
			xfdashboard_live_window_set_label_font(self, g_value_get_string(inValue));
			break;

		case PROP_LABEL_COLOR:
			xfdashboard_live_window_set_label_color(self, clutter_value_get_color(inValue));
			break;

		case PROP_LABEL_BACKGROUND_COLOR:
			xfdashboard_live_window_set_label_background_color(self, clutter_value_get_color(inValue));
			break;

		case PROP_LABEL_MARGIN:
			xfdashboard_live_window_set_label_margin(self, g_value_get_float(inValue));
			break;

		case PROP_LABEL_ELLIPSIZE_MODE:
			xfdashboard_live_window_set_label_ellipsize_mode(self, g_value_get_enum(inValue));
			break;

		default:
			G_OBJECT_WARN_INVALID_PROPERTY_ID(inObject, inPropID, inSpec);
			break;
	}
}

static void xfdashboard_live_window_get_property(GObject *inObject,
												guint inPropID,
												GValue *outValue,
												GParamSpec *inSpec)
{
	XfdashboardLiveWindow	*self=XFDASHBOARD_LIVE_WINDOW(inObject);

	switch(inPropID)
	{
		case PROP_WINDOW:
			g_value_set_object(outValue, self->priv->window);
			break;

		case PROP_LABEL_FONT:
			g_value_set_string(outValue, self->priv->labelFont);
			break;

		case PROP_LABEL_COLOR:
			clutter_value_set_color(outValue, self->priv->labelTextColor);
			break;

		case PROP_LABEL_BACKGROUND_COLOR:
			clutter_value_set_color(outValue, self->priv->labelBackgroundColor);
			break;

		case PROP_LABEL_MARGIN:
			g_value_set_float(outValue, self->priv->labelMargin);
			break;

		case PROP_LABEL_ELLIPSIZE_MODE:
			g_value_set_enum(outValue, self->priv->labelEllipsize);
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
static void xfdashboard_live_window_class_init(XfdashboardLiveWindowClass *klass)
{
	ClutterActorClass	*actorClass=CLUTTER_ACTOR_CLASS(klass);
	GObjectClass		*gobjectClass=G_OBJECT_CLASS(klass);

	/* Override functions */
	gobjectClass->dispose=xfdashboard_live_window_dispose;
	gobjectClass->set_property=xfdashboard_live_window_set_property;
	gobjectClass->get_property=xfdashboard_live_window_get_property;

	actorClass->show_all=xfdashboard_live_window_show_all;
	actorClass->hide_all=xfdashboard_live_window_hide_all;
	actorClass->paint=xfdashboard_live_window_paint;
	actorClass->pick=xfdashboard_live_window_pick;
	actorClass->get_preferred_width=xfdashboard_live_window_get_preferred_width;
	actorClass->get_preferred_height=xfdashboard_live_window_get_preferred_height;
	actorClass->allocate=xfdashboard_live_window_allocate;
	actorClass->destroy=xfdashboard_live_window_destroy;

	/* Set up private structure */
	g_type_class_add_private(klass, sizeof(XfdashboardLiveWindowPrivate));

	/* Define properties */
	XfdashboardLiveWindowProperties[PROP_WINDOW]=
		g_param_spec_object("window",
							"Window",
							"Window to display live",
							WNCK_TYPE_WINDOW,
							G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY);

	XfdashboardLiveWindowProperties[PROP_LABEL_FONT]=
		g_param_spec_string("label-font",
							"Label font",
							"Font description to use in label",
							NULL,
							G_PARAM_READWRITE);

	XfdashboardLiveWindowProperties[PROP_LABEL_COLOR]=
		clutter_param_spec_color("label-color",
									"Label text color",
									"Text color of label",
									&defaultTextColor,
									G_PARAM_READWRITE | G_PARAM_CONSTRUCT);

	XfdashboardLiveWindowProperties[PROP_LABEL_BACKGROUND_COLOR]=
		clutter_param_spec_color("label-background-color",
									"Label background color",
									"Background color of label",
									&defaultBackgroundColor,
									G_PARAM_READWRITE | G_PARAM_CONSTRUCT);

	XfdashboardLiveWindowProperties[PROP_LABEL_MARGIN]=
		g_param_spec_float("label-margin",
							"Label background margin",
							"Margin of label's background in pixels",
							0.0f, G_MAXFLOAT,
							4.0f,
							G_PARAM_READWRITE | G_PARAM_CONSTRUCT);

	XfdashboardLiveWindowProperties[PROP_LABEL_ELLIPSIZE_MODE]=
		g_param_spec_enum("label-ellipsize-mode",
							"Label ellipsize mode",
							"Mode of ellipsize if text in label is too long",
							PANGO_TYPE_ELLIPSIZE_MODE,
							PANGO_ELLIPSIZE_MIDDLE,
							G_PARAM_READWRITE | G_PARAM_CONSTRUCT);

	g_object_class_install_properties(gobjectClass, PROP_LAST, XfdashboardLiveWindowProperties);

	/* Define signals */
	XfdashboardLiveWindowSignals[CLICKED]=
		g_signal_new("clicked",
						G_TYPE_FROM_CLASS(klass),
						G_SIGNAL_RUN_LAST,
						G_STRUCT_OFFSET(XfdashboardLiveWindowClass, clicked),
						NULL,
						NULL,
						g_cclosure_marshal_VOID__VOID,
						G_TYPE_NONE,
						0);

	XfdashboardLiveWindowSignals[CLOSE]=
		g_signal_new("close",
						G_TYPE_FROM_CLASS(klass),
						G_SIGNAL_RUN_LAST,
						G_STRUCT_OFFSET(XfdashboardLiveWindowClass, close),
						NULL,
						NULL,
						g_cclosure_marshal_VOID__VOID,
						G_TYPE_NONE,
						0);

	XfdashboardLiveWindowSignals[GEOMETRY_CHANGED]=
		g_signal_new("geometry-changed",
						G_TYPE_FROM_CLASS(klass),
						G_SIGNAL_RUN_LAST,
						G_STRUCT_OFFSET(XfdashboardLiveWindowClass, geometry_changed),
						NULL,
						NULL,
						g_cclosure_marshal_VOID__VOID,
						G_TYPE_NONE,
						0);

	XfdashboardLiveWindowSignals[VISIBILITY_CHANGED]=
		g_signal_new("visibility-changed",
						G_TYPE_FROM_CLASS(klass),
						G_SIGNAL_RUN_LAST,
						G_STRUCT_OFFSET(XfdashboardLiveWindowClass, visibility_changed),
						NULL,
						NULL,
						g_cclosure_marshal_VOID__BOOLEAN,
						G_TYPE_NONE,
						1,
						G_TYPE_BOOLEAN);

	XfdashboardLiveWindowSignals[WORKSPACE_CHANGED]=
		g_signal_new("workspace-changed",
						G_TYPE_FROM_CLASS(klass),
						G_SIGNAL_RUN_LAST,
						G_STRUCT_OFFSET(XfdashboardLiveWindowClass, workspace_changed),
						NULL,
						NULL,
						g_cclosure_marshal_VOID__VOID,
						G_TYPE_NONE,
						0);
}

/* Object initialization
 * Create private structure and set up default values
 */
static void xfdashboard_live_window_init(XfdashboardLiveWindow *self)
{
	XfdashboardLiveWindowPrivate	*priv;

	priv=self->priv=XFDASHBOARD_LIVE_WINDOW_GET_PRIVATE(self);

	/* This actor is react on events */
	clutter_actor_set_reactive(CLUTTER_ACTOR(self), TRUE);

	/* Set up default values */
	priv->actorWindow=NULL;
	priv->actorLabel=NULL;
	priv->actorClose=NULL;

	priv->window=NULL;
	priv->canClose=FALSE;
	priv->isVisible=FALSE;
	priv->signalActionsChangedID=0L;
	priv->signalGeometryChangedID=0L;
	priv->signalIconChangedID=0L;
	priv->signalNameChangedID=0L;
	priv->signalStateChangedID=0L;
	priv->signalWorkspaceChangedID=0L;

	priv->wasClosedClicked=FALSE;

	priv->labelFont=NULL;
	priv->labelTextColor=NULL;
	priv->labelBackgroundColor=NULL;

	/* Connect signals */
	priv->clickAction=clutter_click_action_new();
	clutter_actor_add_action(CLUTTER_ACTOR(self), priv->clickAction);
	g_signal_connect(priv->clickAction, "clicked", G_CALLBACK(xfdashboard_live_window_clicked), NULL);
}

/* Implementation: Public API */

/* Create new actor */
ClutterActor* xfdashboard_live_window_new(WnckWindow* inWindow)
{
	return(g_object_new(XFDASHBOARD_TYPE_LIVE_WINDOW,
						"window", inWindow,
						NULL));
}

/* Get/set window to display */
const WnckWindow* xfdashboard_live_window_get_window(XfdashboardLiveWindow *self)
{
	g_return_val_if_fail(XFDASHBOARD_IS_LIVE_WINDOW(self), NULL);

	return(self->priv->window);
}

/* Get/set font to use in label */
const gchar* xfdashboard_live_window_get_label_font(XfdashboardLiveWindow *self)
{
	g_return_val_if_fail(XFDASHBOARD_IS_LIVE_WINDOW(self), NULL);

	return(self->priv->labelFont);
}

void xfdashboard_live_window_set_label_font(XfdashboardLiveWindow *self, const gchar *inFont)
{
	g_return_if_fail(XFDASHBOARD_IS_LIVE_WINDOW(self));
	g_return_if_fail(inFont);

	/* Set font of label */
	XfdashboardLiveWindowPrivate	*priv=XFDASHBOARD_LIVE_WINDOW(self)->priv;

	if(!priv->labelFont || g_strcmp0(priv->labelFont, inFont)!=0)
	{
		if(priv->labelFont) g_free(priv->labelFont);
		priv->labelFont=g_strdup(inFont);

		/* Set property of actor and queue a redraw if actors are created */
		if(priv->actorLabel)
		{
			xfdashboard_button_set_font(XFDASHBOARD_BUTTON(priv->actorLabel), priv->labelFont);
			clutter_actor_queue_redraw(CLUTTER_ACTOR(self));
		}
	}
}

/* Get/set color of text in label */
const ClutterColor* xfdashboard_live_window_get_label_color(XfdashboardLiveWindow *self)
{
	g_return_val_if_fail(XFDASHBOARD_IS_LIVE_WINDOW(self), NULL);

	return(self->priv->labelTextColor);
}

void xfdashboard_live_window_set_label_color(XfdashboardLiveWindow *self, const ClutterColor *inColor)
{
	g_return_if_fail(XFDASHBOARD_IS_LIVE_WINDOW(self));

	/* Set text color of label */
	XfdashboardLiveWindowPrivate	*priv=XFDASHBOARD_LIVE_WINDOW(self)->priv;

	if(!priv->labelTextColor || !clutter_color_equal(inColor, priv->labelTextColor))
	{
		if(priv->labelTextColor) clutter_color_free(priv->labelTextColor);
		priv->labelTextColor=clutter_color_copy(inColor);

		/* Set property of actor and queue a redraw if actors are created */
		if(priv->actorLabel)
		{
			xfdashboard_button_set_color(XFDASHBOARD_BUTTON(priv->actorLabel), priv->labelTextColor);
			clutter_actor_queue_redraw(CLUTTER_ACTOR(self));
		}
	}
}

/* Get/set color of label's background */
const ClutterColor* xfdashboard_live_window_get_label_background_color(XfdashboardLiveWindow *self)
{
	g_return_val_if_fail(XFDASHBOARD_IS_LIVE_WINDOW(self), NULL);

	return(self->priv->labelBackgroundColor);
}

void xfdashboard_live_window_set_label_background_color(XfdashboardLiveWindow *self, const ClutterColor *inColor)
{
	g_return_if_fail(XFDASHBOARD_IS_LIVE_WINDOW(self));
	g_return_if_fail(inColor);

	/* Set background color of label */
	XfdashboardLiveWindowPrivate	*priv=XFDASHBOARD_LIVE_WINDOW(self)->priv;

	if(!priv->labelBackgroundColor || !clutter_color_equal(inColor, priv->labelBackgroundColor))
	{
		if(priv->labelBackgroundColor) clutter_color_free(priv->labelBackgroundColor);
		priv->labelBackgroundColor=clutter_color_copy(inColor);

		/* Set property of actor and queue a redraw if actors are created */
		if(priv->actorLabel)
		{
			xfdashboard_button_set_background_color(XFDASHBOARD_BUTTON(priv->actorLabel), priv->labelBackgroundColor);
			clutter_actor_queue_redraw(CLUTTER_ACTOR(self));
		}
	}
}

/* Get/set margin of background to label */
const gfloat xfdashboard_live_window_get_label_margin(XfdashboardLiveWindow *self)
{
	g_return_val_if_fail(XFDASHBOARD_IS_LIVE_WINDOW(self), 0);

	return(self->priv->labelMargin);
}

void xfdashboard_live_window_set_label_margin(XfdashboardLiveWindow *self, const gfloat inMargin)
{
	g_return_if_fail(XFDASHBOARD_IS_LIVE_WINDOW(self));
	g_return_if_fail(inMargin>=0.0f);

	/* Set margin */
	XfdashboardLiveWindowPrivate	*priv=XFDASHBOARD_LIVE_WINDOW(self)->priv;

	if(priv->labelMargin!=inMargin)
	{
		priv->labelMargin=inMargin;

		/* Set property of actor and queue a redraw if actors are created */
		if(priv->actorLabel)
		{
			xfdashboard_button_set_margin(XFDASHBOARD_BUTTON(priv->actorLabel), priv->labelMargin);
			clutter_actor_queue_redraw(CLUTTER_ACTOR(self));
		}
	}
}

/* Get/set ellipsize mode if label's text is getting too long */
const PangoEllipsizeMode xfdashboard_live_window_get_label_ellipsize_mode(XfdashboardLiveWindow *self)
{
	g_return_val_if_fail(XFDASHBOARD_IS_LIVE_WINDOW(self), 0);

	return(self->priv->labelEllipsize);
}

void xfdashboard_live_window_set_label_ellipsize_mode(XfdashboardLiveWindow *self, const PangoEllipsizeMode inMode)
{
	g_return_if_fail(XFDASHBOARD_IS_LIVE_WINDOW(self));

	/* Set ellipsize mode */
	XfdashboardLiveWindowPrivate	*priv=XFDASHBOARD_LIVE_WINDOW(self)->priv;

	if(priv->labelEllipsize!=inMode)
	{
		priv->labelEllipsize=inMode;

		/* Set property of actor and queue a redraw if actors are created */
		if(priv->actorLabel)
		{
			xfdashboard_button_set_ellipsize_mode(XFDASHBOARD_BUTTON(priv->actorLabel), priv->labelEllipsize);
			clutter_actor_queue_redraw(CLUTTER_ACTOR(self));
		}
	}
}
