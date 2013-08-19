/*
 * live-window: An actor showing the content of a window which will
 *              be updated if changed and visible on active workspace.
 *              It also provides controls to manipulate it.
 * 
 * Copyright 2012-2013 Stephan Haller <nomad@froevel.de>
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

#include <glib/gi18n-lib.h>
#include <clutter/clutter.h>
#include <clutter/x11/clutter-x11.h>
#include <math.h>

#include "button.h"
#include "stage.h"
#include "utils.h"

/* Define this class in GObject system */
G_DEFINE_TYPE(XfdashboardLiveWindow,
				xfdashboard_live_window,
				CLUTTER_TYPE_ACTOR)

/* Private structure - access only by public API if needed */
#define XFDASHBOARD_LIVE_WINDOW_GET_PRIVATE(obj) \
	(G_TYPE_INSTANCE_GET_PRIVATE((obj), XFDASHBOARD_TYPE_LIVE_WINDOW, XfdashboardLiveWindowPrivate))

struct _XfdashboardLiveWindowPrivate
{
	/* Properties related */
	WnckWindow		*window;

	/* Instance related */
	gboolean		isVisible;

	ClutterActor	*actorWindow;
	ClutterActor	*actorClose;
	ClutterActor	*actorTitle;

	/* Settings */
	gfloat			marginCloseButton;
	gfloat			marginTitleActor;
};

/* Properties */
enum
{
	PROP_0,

	PROP_WINDOW,

	PROP_CLOSE_BUTTON,
	PROP_CLOSE_BUTTON_MARGIN,

	PROP_TITLE_ACTOR,
	PROP_TITLE_ACTOR_MARGIN,

	PROP_LAST
};

GParamSpec* XfdashboardLiveWindowProperties[PROP_LAST]={ 0, };

/* Signals */
enum
{
	SIGNAL_CLICKED,
	SIGNAL_CLOSE,

	SIGNAL_GEOMETRY_CHANGED,
	SIGNAL_VISIBILITY_CHANGED,
	SIGNAL_WORKSPACE_CHANGED,

	SIGNAL_LAST
};

guint XfdashboardLiveWindowSignals[SIGNAL_LAST]={ 0, };

/* IMPLEMENTATION: Private variables and methods */
#define DEFAULT_MARGIN_TITLE_ACTOR		4.0f			// TODO: Replace by settings/theming object
#define DEFAULT_MARGIN_CLOSE_BUTTON		4.0f			// TODO: Replace by settings/theming object
#define WINDOW_CLOSE_BUTTON_ICON		GTK_STOCK_CLOSE	// TODO: Replace by settings/theming object

/* Determine if window state flags specify window's visibility */
gboolean _xfdashboard_live_window_is_window_visiblity_flag(XfdashboardLiveWindow *self, WnckWindowState inState)
{
	return((inState & WNCK_WINDOW_STATE_SKIP_PAGER) ||
			(inState & WNCK_WINDOW_STATE_SKIP_TASKLIST));
}

/* Check if window should be shown */
gboolean _xfdashboard_live_window_is_visible_window(XfdashboardLiveWindow *self, WnckWindow *inWindow)
{
	g_return_val_if_fail(WNCK_IS_WINDOW(inWindow), FALSE);

	WnckWindowState		state;

	/* Determine if windows should be shown depending on its state */
	state=wnck_window_get_state(inWindow);
	if((state & WNCK_WINDOW_STATE_SKIP_PAGER) ||
		(state & WNCK_WINDOW_STATE_SKIP_TASKLIST))
	{
		return(FALSE);
	}

	/* If we get here the window should be shown */
	return(TRUE);
}

/* The close button of this actor was clicked */
void _xfdashboard_live_window_on_close_clicked(XfdashboardLiveWindow *self, gpointer inUserData)
{
	g_return_if_fail(XFDASHBOARD_IS_LIVE_WINDOW(self));

	/* Emit "close" signal */
	g_signal_emit(self, XfdashboardLiveWindowSignals[SIGNAL_CLOSE], 0);
}

/* This actor was clicked */
void _xfdashboard_live_window_on_clicked(XfdashboardLiveWindow *self, ClutterActor *inActor, gpointer inUserData)
{
	g_return_if_fail(XFDASHBOARD_IS_LIVE_WINDOW(self));
	g_return_if_fail(CLUTTER_IS_ACTOR(inActor));
	g_return_if_fail(CLUTTER_IS_CLICK_ACTION(inUserData));

	XfdashboardLiveWindowPrivate	*priv=self->priv;
	ClutterClickAction				*action=CLUTTER_CLICK_ACTION(inUserData);
	gfloat							eventX, eventY;
	ClutterActor					*eventActor;

	/* TODO: Do I use ClutterClickAction wrong here or problem in proxying "click" signal
	 *       in XfdashboardButton or Clutter bug?
	 *
	 * Every second click on the close button of this actor emits the "clicked" signal
	 * for this actor instead emitting for the close button. So we need to check if
	 * this "clicked" action was really emitted by this actor.
	 */
	clutter_click_action_get_coords(action, &eventX, &eventY);
	eventActor=clutter_stage_get_actor_at_pos(CLUTTER_STAGE(clutter_actor_get_stage(inActor)),
												CLUTTER_PICK_REACTIVE,
												eventX, eventY);
	if(eventActor==priv->actorClose)
	{
		_xfdashboard_live_window_on_close_clicked(self, NULL);
		return;
	}

	/* Emit "clicked" signal */
	g_signal_emit(self, XfdashboardLiveWindowSignals[SIGNAL_CLICKED], 0);
}

/* Position and/or size of window has changed */
void _xfdashboard_live_window_on_geometry_changed(XfdashboardLiveWindow *self, gpointer inUserData)
{
	g_return_if_fail(XFDASHBOARD_IS_LIVE_WINDOW(self));
	g_return_if_fail(WNCK_IS_WINDOW(inUserData));

	/* Actor's allocation may change because of new geometry so relayout */
	clutter_actor_queue_relayout(CLUTTER_ACTOR(self));

	/* Emit "geometry-changed" signal */
	g_signal_emit(self, XfdashboardLiveWindowSignals[SIGNAL_GEOMETRY_CHANGED], 0);
}

/* Action items of window has changed */
void _xfdashboard_live_window_on_actions_changed(XfdashboardLiveWindow *self,
													WnckWindowActions inChangedMask,
													WnckWindowActions inNewValue,
													gpointer inUserData)
{
	g_return_if_fail(XFDASHBOARD_IS_LIVE_WINDOW(self));
	g_return_if_fail(WNCK_IS_WINDOW(inUserData));

	XfdashboardLiveWindowPrivate	*priv=self->priv;

	/* Show or hide close button actor */
	if(inChangedMask & WNCK_WINDOW_ACTION_CLOSE)
	{
		if(inNewValue & WNCK_WINDOW_ACTION_CLOSE) clutter_actor_show(priv->actorClose);
			else clutter_actor_hide(priv->actorClose);
	}
}

/* Icon of window has changed */
void _xfdashboard_live_window_on_icon_changed(XfdashboardLiveWindow *self, gpointer inUserData)
{
	g_return_if_fail(XFDASHBOARD_IS_LIVE_WINDOW(self));
	g_return_if_fail(WNCK_IS_WINDOW(inUserData));

	XfdashboardLiveWindowPrivate	*priv=self->priv;
	WnckWindow						*window=WNCK_WINDOW(inUserData);
	ClutterImage					*icon;

	/* Set new icon in title actor */
	icon=xfdashboard_get_image_for_pixbuf(wnck_window_get_icon(window));
	xfdashboard_button_set_icon_image(XFDASHBOARD_BUTTON(priv->actorTitle), icon);
	g_object_unref(icon);
}

/* Title of window has changed */
void _xfdashboard_live_window_on_name_changed(XfdashboardLiveWindow *self, gpointer inUserData)
{
	g_return_if_fail(XFDASHBOARD_IS_LIVE_WINDOW(self));
	g_return_if_fail(WNCK_IS_WINDOW(inUserData));

	XfdashboardLiveWindowPrivate	*priv=self->priv;
	WnckWindow						*window=WNCK_WINDOW(inUserData);

	/* Set new icon in title actor */
	xfdashboard_button_set_text(XFDASHBOARD_BUTTON(priv->actorTitle), wnck_window_get_name(window));
}

/* Window's state has changed */
void _xfdashboard_live_window_on_state_changed(XfdashboardLiveWindow *self,
												WnckWindowState inChangedMask,
												WnckWindowState inNewState,
												gpointer inUserData)
{
	g_return_if_fail(XFDASHBOARD_IS_LIVE_WINDOW(self));
	g_return_if_fail(WNCK_IS_WINDOW(inUserData));

	XfdashboardLiveWindowPrivate	*priv=self->priv;
	WnckWindow						*window=WNCK_WINDOW(inUserData);
	gboolean						isVisible;

	/* If certain state flags has changed check window's visibility */
	if(_xfdashboard_live_window_is_window_visiblity_flag(self, inChangedMask))
	{
		isVisible=_xfdashboard_live_window_is_visible_window(self, window);
		if(priv->isVisible!=isVisible)
		{
			priv->isVisible=isVisible;
			g_signal_emit(self, XfdashboardLiveWindowSignals[SIGNAL_VISIBILITY_CHANGED], 0);
		}
	}
}

/* Window's workspace has changed */
void _xfdashboard_live_window_on_workspace_changed(XfdashboardLiveWindow *self, gpointer inUserData)
{
	g_return_if_fail(XFDASHBOARD_IS_LIVE_WINDOW(self));
	g_return_if_fail(WNCK_IS_WINDOW(inUserData));

	/* Emit "workspace-changed" signal */
	g_signal_emit(self, XfdashboardLiveWindowSignals[SIGNAL_WORKSPACE_CHANGED], 0);
}

/* IMPLEMENTATION: ClutterActor */

/* Get preferred width/height */
void _xfdashboard_live_window_get_preferred_height(ClutterActor *self,
													gfloat inForWidth,
													gfloat *outMinHeight,
													gfloat *outNaturalHeight)
{
	XfdashboardLiveWindowPrivate	*priv=XFDASHBOARD_LIVE_WINDOW(self)->priv;
	gfloat							minHeight, naturalHeight;
	gfloat							childMinHeight, childNaturalHeight;

	minHeight=naturalHeight=0.0f;

	/* Determine size of window if available and visible (should usually be the largest actor) */
	if(priv->actorWindow && CLUTTER_ACTOR_IS_VISIBLE(priv->actorWindow))
	{
		clutter_actor_get_preferred_height(priv->actorWindow,
											inForWidth,
											&childMinHeight,
											&childNaturalHeight);
		if(childMinHeight>minHeight) minHeight=childMinHeight;
		if(childNaturalHeight>naturalHeight) naturalHeight=childNaturalHeight;
	}

	/* Determine size of title actor if visible */
	if(CLUTTER_ACTOR_IS_VISIBLE(priv->actorTitle))
	{
		clutter_actor_get_preferred_height(priv->actorTitle,
											inForWidth,
											&childMinHeight,
											&childNaturalHeight);
		childMinHeight+=(2*priv->marginTitleActor);
		childNaturalHeight+=(2*priv->marginTitleActor);
		if(childMinHeight>minHeight) minHeight=childMinHeight;
		if(childNaturalHeight>naturalHeight) naturalHeight=childNaturalHeight;
	}

	/* Determine size of close button actor if visible */
	if(CLUTTER_ACTOR_IS_VISIBLE(priv->actorClose))
	{
		clutter_actor_get_preferred_height(priv->actorClose,
											inForWidth,
											&childMinHeight,
											&childNaturalHeight);
		childMinHeight+=(2*priv->marginCloseButton);
		childNaturalHeight+=(2*priv->marginCloseButton);
		if(childMinHeight>minHeight) minHeight=childMinHeight;
		if(childNaturalHeight>naturalHeight) naturalHeight=childNaturalHeight;
	}

	/* Store sizes computed */
	if(outMinHeight) *outMinHeight=minHeight;
	if(outNaturalHeight) *outNaturalHeight=naturalHeight;
}

void _xfdashboard_live_window_get_preferred_width(ClutterActor *self,
													gfloat inForHeight,
													gfloat *outMinWidth,
													gfloat *outNaturalWidth)
{
	XfdashboardLiveWindowPrivate	*priv=XFDASHBOARD_LIVE_WINDOW(self)->priv;
	gfloat							minWidth, naturalWidth;
	gfloat							childMinWidth, childNaturalWidth;

	minWidth=naturalWidth=0.0f;

	/* Determine size of window if available and visible (should usually be the largest actor) */
	if(priv->actorWindow && CLUTTER_ACTOR_IS_VISIBLE(priv->actorWindow))
	{
		clutter_actor_get_preferred_width(priv->actorWindow,
											inForHeight,
											&childMinWidth,
											&childNaturalWidth);
		if(childMinWidth>minWidth) minWidth=childMinWidth;
		if(childNaturalWidth>naturalWidth) naturalWidth=childNaturalWidth;
	}

	/* Determine size of title actor if visible */
	if(CLUTTER_ACTOR_IS_VISIBLE(priv->actorTitle))
	{
		clutter_actor_get_preferred_width(priv->actorTitle,
											inForHeight,
											&childMinWidth,
											 &childNaturalWidth);
		childMinWidth+=(2*priv->marginTitleActor);
		childNaturalWidth+=(2*priv->marginTitleActor);
		if(childMinWidth>minWidth) minWidth=childMinWidth;
		if(childNaturalWidth>naturalWidth) naturalWidth=childNaturalWidth;
	}

	/* Determine size of close button actor if visible */
	if(CLUTTER_ACTOR_IS_VISIBLE(priv->actorClose))
	{
		clutter_actor_get_preferred_width(priv->actorClose,
											inForHeight,
											&childMinWidth,
											&childNaturalWidth);
		childMinWidth+=(2*priv->marginCloseButton);
		childNaturalWidth+=(2*priv->marginCloseButton);
		if(childMinWidth>minWidth) minWidth=childMinWidth;
		if(childNaturalWidth>naturalWidth) naturalWidth=childNaturalWidth;
	}

	/* Store sizes computed */
	if(outMinWidth) *outMinWidth=minWidth;
	if(outNaturalWidth) *outNaturalWidth=naturalWidth;
}

/* Allocate position and size of actor and its children*/
void _xfdashboard_live_window_allocate(ClutterActor *self,
										const ClutterActorBox *inBox,
										ClutterAllocationFlags inFlags)
{
	XfdashboardLiveWindowPrivate	*priv=XFDASHBOARD_LIVE_WINDOW(self)->priv;
	ClutterActorBox					*boxActorWindow=NULL;
	ClutterActorBox					*boxActorTitle=NULL;
	ClutterActorBox					*boxActorClose=NULL;
	gfloat							maxWidth;
	gfloat							titleWidth, titleHeight;
	gfloat							closeWidth, closeHeight;
	gfloat							left, top, right, bottom;

	/* Chain up to store the allocation of the actor */
	CLUTTER_ACTOR_CLASS(xfdashboard_live_window_parent_class)->allocate(self, inBox, inFlags);

	/* Set allocation on window texture */
	boxActorWindow=clutter_actor_box_copy(inBox);
	clutter_actor_box_set_origin(boxActorWindow, 0.0f, 0.0f);
	clutter_actor_allocate(priv->actorWindow, boxActorWindow, inFlags);

	/* Set allocation on close actor */
	clutter_actor_get_preferred_size(priv->actorClose,
										NULL, NULL,
										&closeWidth, &closeHeight);

	right=clutter_actor_box_get_x(boxActorWindow)+clutter_actor_box_get_width(boxActorWindow)-priv->marginCloseButton;
	left=MAX(right-closeWidth, priv->marginCloseButton);
	top=clutter_actor_box_get_y(boxActorWindow)+priv->marginCloseButton;
	bottom=top+closeHeight;

	right=MAX(left, right);
	bottom=MAX(top, bottom);

	boxActorClose=clutter_actor_box_new(floor(left), floor(top), floor(right), floor(bottom));
	clutter_actor_allocate(priv->actorClose, boxActorClose, inFlags);

	/* Set allocation on title actor
	 * But prevent that title overlaps close button
	 */
	clutter_actor_get_preferred_size(priv->actorTitle,
										NULL, NULL,
										&titleWidth, &titleHeight);

	maxWidth=clutter_actor_box_get_width(boxActorWindow)-(2*priv->marginTitleActor);
	if(titleWidth>maxWidth) titleWidth=maxWidth;

	left=clutter_actor_box_get_x(boxActorWindow)+((clutter_actor_box_get_width(boxActorWindow)-titleWidth)/2.0f);
	right=left+titleWidth;
	bottom=clutter_actor_box_get_y(boxActorWindow)+clutter_actor_box_get_height(boxActorWindow)-(2*priv->marginTitleActor);
	top=bottom-titleHeight;
	if(left>right) left=right-1.0f;
	if(top<(clutter_actor_box_get_y(boxActorClose)+clutter_actor_box_get_height(boxActorClose)))
	{
		if(right>=clutter_actor_box_get_x(boxActorClose))
		{
			right=clutter_actor_box_get_x(boxActorClose)-MIN(priv->marginTitleActor, priv->marginCloseButton);
		}

		if(top<clutter_actor_box_get_y(boxActorClose))
		{
			top=clutter_actor_box_get_y(boxActorClose);
			bottom=top+titleHeight;
		}
	}

	right=MAX(left, right);
	bottom=MAX(top, bottom);

	boxActorTitle=clutter_actor_box_new(floor(left), floor(top), floor(right), floor(bottom));
	clutter_actor_allocate(priv->actorTitle, boxActorTitle, inFlags);

	/* Release allocated resources */
	if(boxActorWindow) clutter_actor_box_free(boxActorWindow);
	if(boxActorTitle) clutter_actor_box_free(boxActorTitle);
	if(boxActorClose) clutter_actor_box_free(boxActorClose);
}

/* IMPLEMENTATION: GObject */

/* Dispose this object */
void _xfdashboard_live_window_dispose(GObject *inObject)
{
	XfdashboardLiveWindow			*self=XFDASHBOARD_LIVE_WINDOW(inObject);
	XfdashboardLiveWindowPrivate	*priv=self->priv;

	/* Dispose allocated resources */
	if(priv->window)
	{
		g_signal_handlers_disconnect_by_data(priv->window, self);
		priv->window=NULL;
	}

	if(priv->actorWindow)
	{
		clutter_actor_destroy(priv->actorWindow);
		priv->actorWindow=NULL;
	}

	if(priv->actorTitle)
	{
		clutter_actor_destroy(priv->actorTitle);
		priv->actorTitle=NULL;
	}

	if(priv->actorClose)
	{
		clutter_actor_destroy(priv->actorClose);
		priv->actorClose=NULL;
	}

	/* Call parent's class dispose method */
	G_OBJECT_CLASS(xfdashboard_live_window_parent_class)->dispose(inObject);
}

/* Set/get properties */
void _xfdashboard_live_window_set_property(GObject *inObject,
														guint inPropID,
														const GValue *inValue,
														GParamSpec *inSpec)
{
	XfdashboardLiveWindow			*self=XFDASHBOARD_LIVE_WINDOW(inObject);
	
	switch(inPropID)
	{
		case PROP_WINDOW:
			xfdashboard_live_window_set_window(self, g_value_get_object(inValue));
			break;

		case PROP_CLOSE_BUTTON_MARGIN:
			xfdashboard_live_window_set_close_button_margin(self, g_value_get_float(inValue));
			break;

		case PROP_TITLE_ACTOR_MARGIN:
			xfdashboard_live_window_set_title_actor_margin(self, g_value_get_float(inValue));
			break;

		default:
			G_OBJECT_WARN_INVALID_PROPERTY_ID(inObject, inPropID, inSpec);
			break;
	}
}

void _xfdashboard_live_window_get_property(GObject *inObject,
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

		case PROP_CLOSE_BUTTON:
			g_value_set_object(outValue, self->priv->actorClose);
			break;

		case PROP_CLOSE_BUTTON_MARGIN:
			g_value_set_float(outValue, self->priv->marginCloseButton);
			break;

		case PROP_TITLE_ACTOR:
			g_value_set_object(outValue, self->priv->actorTitle);
			break;

		case PROP_TITLE_ACTOR_MARGIN:
			g_value_set_float(outValue, self->priv->marginTitleActor);
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
void xfdashboard_live_window_class_init(XfdashboardLiveWindowClass *klass)
{
	ClutterActorClass	*actorClass=CLUTTER_ACTOR_CLASS(klass);
	GObjectClass		*gobjectClass=G_OBJECT_CLASS(klass);

	/* Override functions */
	actorClass->get_preferred_width=_xfdashboard_live_window_get_preferred_width;
	actorClass->get_preferred_height=_xfdashboard_live_window_get_preferred_height;
	actorClass->allocate=_xfdashboard_live_window_allocate;

	gobjectClass->dispose=_xfdashboard_live_window_dispose;
	gobjectClass->set_property=_xfdashboard_live_window_set_property;
	gobjectClass->get_property=_xfdashboard_live_window_get_property;

	/* Set up private structure */
	g_type_class_add_private(klass, sizeof(XfdashboardLiveWindowPrivate));

	/* Define properties */
	XfdashboardLiveWindowProperties[PROP_WINDOW]=
		g_param_spec_object("window",
								_("Window"),
								_("The window to show"),
								WNCK_TYPE_WINDOW,
								G_PARAM_READWRITE);

	XfdashboardLiveWindowProperties[PROP_CLOSE_BUTTON]=
		g_param_spec_object("close-button",
								_("Close button"),
								_("The actor used to show the close button"),
								XFDASHBOARD_TYPE_BUTTON,
								G_PARAM_READABLE);

	XfdashboardLiveWindowProperties[PROP_CLOSE_BUTTON_MARGIN]=
		g_param_spec_float("close-button-margin",
							_("Close button margin"),
							_("Margin of close button to window actor in pixels"),
							0.0f, G_MAXFLOAT,
							DEFAULT_MARGIN_CLOSE_BUTTON,
							G_PARAM_READWRITE);

	XfdashboardLiveWindowProperties[PROP_TITLE_ACTOR]=
		g_param_spec_object("title-actor",
								_("Title actor"),
								_("The actor used to show the window title and icon"),
								XFDASHBOARD_TYPE_BUTTON,
								G_PARAM_READABLE);

	XfdashboardLiveWindowProperties[PROP_TITLE_ACTOR_MARGIN]=
		g_param_spec_float("title-actor-margin",
							_("Title actor margin"),
							_("Margin of title actor to window actor in pixels"),
							0.0f, G_MAXFLOAT,
							DEFAULT_MARGIN_TITLE_ACTOR,
							G_PARAM_READWRITE);

	g_object_class_install_properties(gobjectClass, PROP_LAST, XfdashboardLiveWindowProperties);

	/* Define signals */
	XfdashboardLiveWindowSignals[SIGNAL_CLICKED]=
		g_signal_new("clicked",
						G_TYPE_FROM_CLASS(klass),
						G_SIGNAL_RUN_LAST,
						G_STRUCT_OFFSET(XfdashboardLiveWindowClass, clicked),
						NULL,
						NULL,
						g_cclosure_marshal_VOID__VOID,
						G_TYPE_NONE,
						0);

	XfdashboardLiveWindowSignals[SIGNAL_CLOSE]=
		g_signal_new("close",
						G_TYPE_FROM_CLASS(klass),
						G_SIGNAL_RUN_LAST,
						G_STRUCT_OFFSET(XfdashboardLiveWindowClass, close),
						NULL,
						NULL,
						g_cclosure_marshal_VOID__VOID,
						G_TYPE_NONE,
						0);

	XfdashboardLiveWindowSignals[SIGNAL_GEOMETRY_CHANGED]=
		g_signal_new("geometry-changed",
						G_TYPE_FROM_CLASS(klass),
						G_SIGNAL_RUN_LAST,
						G_STRUCT_OFFSET(XfdashboardLiveWindowClass, geometry_changed),
						NULL,
						NULL,
						g_cclosure_marshal_VOID__VOID,
						G_TYPE_NONE,
						0);

	XfdashboardLiveWindowSignals[SIGNAL_VISIBILITY_CHANGED]=
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

	XfdashboardLiveWindowSignals[SIGNAL_WORKSPACE_CHANGED]=
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
void xfdashboard_live_window_init(XfdashboardLiveWindow *self)
{
	XfdashboardLiveWindowPrivate	*priv;
	ClutterAction					*action;

	priv=self->priv=XFDASHBOARD_LIVE_WINDOW_GET_PRIVATE(self);

	/* This actor reacts on events */
	clutter_actor_set_reactive(CLUTTER_ACTOR(self), TRUE);

	/* Set default values */
	priv->window=NULL;
	priv->marginTitleActor=DEFAULT_MARGIN_TITLE_ACTOR;
	priv->marginCloseButton=DEFAULT_MARGIN_CLOSE_BUTTON;

	/* Connect signals to this actor */
	action=clutter_click_action_new();
	clutter_actor_add_action(CLUTTER_ACTOR(self), action);
	g_signal_connect_swapped(action, "clicked", G_CALLBACK(_xfdashboard_live_window_on_clicked), self);

	/* Set up child actors (order is important) */
	priv->actorWindow=clutter_x11_texture_pixmap_new();
	clutter_actor_show(priv->actorWindow);
	clutter_actor_add_child(CLUTTER_ACTOR(self), priv->actorWindow);

	priv->actorTitle=xfdashboard_button_new();
	xfdashboard_button_set_style(XFDASHBOARD_BUTTON(priv->actorTitle), XFDASHBOARD_STYLE_BOTH);
	// TODO: xfdashboard_button_set_font(XFDASHBOARD_BUTTON(priv->actorTitle), priv->labelFont);
	// TODO: if(priv->labelTextColor) xfdashboard_button_set_color(XFDASHBOARD_BUTTON(priv->actorTitle), priv->labelTextColor);
	// TODO: xfdashboard_button_set_ellipsize_mode(XFDASHBOARD_BUTTON(priv->actorTitle), priv->labelEllipsize);
	xfdashboard_background_set_background_type(XFDASHBOARD_BACKGROUND(priv->actorTitle), XFDASHBOARD_BACKGROUND_TYPE_RECTANGLE);
	xfdashboard_background_set_corner_radius(XFDASHBOARD_BACKGROUND(priv->actorTitle), priv->marginTitleActor);
	// TODO: if(priv->labelBackgroundColor) xfdashboard_background_set_color(XFDASHBOARD_BACKGROUND(self), priv->labelBackgroundColor);

	clutter_actor_set_reactive(priv->actorTitle, FALSE);
	clutter_actor_show(priv->actorTitle);
	clutter_actor_add_child(CLUTTER_ACTOR(self), priv->actorTitle);

	priv->actorClose=xfdashboard_button_new_with_icon(WINDOW_CLOSE_BUTTON_ICON);
	xfdashboard_background_set_background_type(XFDASHBOARD_BACKGROUND(priv->actorClose), XFDASHBOARD_BACKGROUND_TYPE_RECTANGLE);
	xfdashboard_background_set_corner_radius(XFDASHBOARD_BACKGROUND(priv->actorClose), priv->marginCloseButton);
	clutter_actor_show(priv->actorClose);
	clutter_actor_add_child(CLUTTER_ACTOR(self), priv->actorClose);
	g_signal_connect_swapped(priv->actorClose, "clicked", G_CALLBACK(_xfdashboard_live_window_on_close_clicked), self);
}

/* Implementation: Public API */

/* Create new instance */
ClutterActor* xfdashboard_live_window_new(void)
{
	return(CLUTTER_ACTOR(g_object_new(XFDASHBOARD_TYPE_LIVE_WINDOW, NULL)));
}

ClutterActor* xfdashboard_live_window_new_for_window(WnckWindow *inWindow)
{
	g_return_val_if_fail(WNCK_IS_WINDOW(inWindow), NULL);

	return(CLUTTER_ACTOR(g_object_new(XFDASHBOARD_TYPE_LIVE_WINDOW,
										"window", inWindow,
										NULL)));
}

/* Get/set window to show */
WnckWindow* xfdashboard_live_window_get_window(XfdashboardLiveWindow *self)
{
	g_return_val_if_fail(XFDASHBOARD_IS_LIVE_WINDOW(self), NULL);

	return(self->priv->window);
}

void xfdashboard_live_window_set_window(XfdashboardLiveWindow *self, WnckWindow *inWindow)
{
	g_return_if_fail(XFDASHBOARD_IS_LIVE_WINDOW(self));
	g_return_if_fail(WNCK_IS_WINDOW(inWindow));

	XfdashboardLiveWindowPrivate	*priv=self->priv;

	/* Only set value if it changes */
	if(inWindow==priv->window) return;

	/* Release old value */
	if(priv->window)
	{
		g_signal_handlers_disconnect_by_data(priv->window, self);
		priv->window=NULL;
	}

	/* Set new value
	 * libwnck objects should never be refed or unrefed, so just set new value
	 */
	priv->window=inWindow;
	priv->isVisible=_xfdashboard_live_window_is_visible_window(self, priv->window);

	/* Setup window actor */
	clutter_x11_texture_pixmap_set_window(CLUTTER_X11_TEXTURE_PIXMAP(priv->actorWindow), wnck_window_get_xid(priv->window), TRUE);
	clutter_x11_texture_pixmap_sync_window(CLUTTER_X11_TEXTURE_PIXMAP(priv->actorWindow));
	clutter_x11_texture_pixmap_set_automatic(CLUTTER_X11_TEXTURE_PIXMAP(priv->actorWindow), TRUE);

	/* Connect signals */
	g_signal_connect_swapped(inWindow, "geometry-changed", G_CALLBACK(_xfdashboard_live_window_on_geometry_changed), self);
	g_signal_connect_swapped(inWindow, "actions-changed", G_CALLBACK(_xfdashboard_live_window_on_actions_changed), self);
	g_signal_connect_swapped(inWindow, "icon-changed", G_CALLBACK(_xfdashboard_live_window_on_icon_changed), self);
	g_signal_connect_swapped(inWindow, "name-changed", G_CALLBACK(_xfdashboard_live_window_on_name_changed), self);
	g_signal_connect_swapped(inWindow, "workspace-changed", G_CALLBACK(_xfdashboard_live_window_on_workspace_changed), self);

	/* Set up this actor and child actor by calling each signal handler now */
	_xfdashboard_live_window_on_geometry_changed(self, priv->window);
	_xfdashboard_live_window_on_actions_changed(self, -1, wnck_window_get_actions(priv->window), priv->window);
	_xfdashboard_live_window_on_icon_changed(self, priv->window);
	_xfdashboard_live_window_on_name_changed(self, priv->window);
	_xfdashboard_live_window_on_state_changed(self, -1, wnck_window_get_state(priv->window), priv->window);
	_xfdashboard_live_window_on_workspace_changed(self, priv->window);

	/* Notify about property change */
	g_object_notify_by_pspec(G_OBJECT(self), XfdashboardLiveWindowProperties[PROP_WINDOW]);
}

/* Get title actor */
XfdashboardButton* xfdashboard_live_window_get_title_actor(XfdashboardLiveWindow *self)
{
	g_return_val_if_fail(XFDASHBOARD_IS_LIVE_WINDOW(self), NULL);

	return(XFDASHBOARD_BUTTON(self->priv->actorTitle));
}

/* Get/set margin of title actor */
gfloat xfdashboard_live_window_get_title_actor_margin(XfdashboardLiveWindow *self)
{
	g_return_val_if_fail(XFDASHBOARD_IS_LIVE_WINDOW(self), 0.0f);

	return(self->priv->marginTitleActor);
}

void xfdashboard_live_window_set_title_actor_margin(XfdashboardLiveWindow *self, gfloat inMargin)
{
	g_return_if_fail(XFDASHBOARD_IS_LIVE_WINDOW(self));
	g_return_if_fail(inMargin>=0.0f);

	XfdashboardLiveWindowPrivate	*priv=self->priv;

	/* Only set value if it changes */
	if(inMargin==priv->marginTitleActor) return;

	/* Set new value */
	priv->marginTitleActor=inMargin;
	xfdashboard_background_set_corner_radius(XFDASHBOARD_BACKGROUND(priv->actorTitle), priv->marginTitleActor);
	clutter_actor_queue_relayout(CLUTTER_ACTOR(self));

	/* Notify about property change */
	g_object_notify_by_pspec(G_OBJECT(self), XfdashboardLiveWindowProperties[PROP_TITLE_ACTOR_MARGIN]);
}

/* Get close button actor */
XfdashboardButton* xfdashboard_live_window_get_close_button(XfdashboardLiveWindow *self)
{
	g_return_val_if_fail(XFDASHBOARD_IS_LIVE_WINDOW(self), NULL);

	return(XFDASHBOARD_BUTTON(self->priv->actorClose));
}

/* Get/set margin of close button actor */
gfloat xfdashboard_live_window_get_close_button_margin(XfdashboardLiveWindow *self)
{
	g_return_val_if_fail(XFDASHBOARD_IS_LIVE_WINDOW(self), 0.0f);

	return(self->priv->marginCloseButton);
}

void xfdashboard_live_window_set_close_button_margin(XfdashboardLiveWindow *self, gfloat inMargin)
{
	g_return_if_fail(XFDASHBOARD_IS_LIVE_WINDOW(self));
	g_return_if_fail(inMargin>=0.0f);

	XfdashboardLiveWindowPrivate	*priv=self->priv;

	/* Only set value if it changes */
	if(inMargin==priv->marginCloseButton) return;

	/* Set new value */
	priv->marginCloseButton=inMargin;
	xfdashboard_background_set_corner_radius(XFDASHBOARD_BACKGROUND(priv->actorClose), priv->marginCloseButton);
	clutter_actor_queue_relayout(CLUTTER_ACTOR(self));

	/* Notify about property change */
	g_object_notify_by_pspec(G_OBJECT(self), XfdashboardLiveWindowProperties[PROP_CLOSE_BUTTON_MARGIN]);
}
