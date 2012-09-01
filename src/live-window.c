/*
 * livewindow.c: An actor showing and updating a window live
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
	ClutterActor		*actorLabelBackground;
	ClutterActor		*actorAppIcon;

	/* Window the actors belong to */
	WnckWindow			*window;

	/* Actor actions */
	ClutterAction		*clickAction;

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
	PROP_FONT,
	PROP_COLOR,
	PROP_BACKGROUND_COLOR,
	PROP_LABEL_MARGIN,
	PROP_ELLIPSIZE_MODE,
	
	PROP_LAST
};

static GParamSpec* XfdashboardLiveWindowProperties[PROP_LAST]={ 0, };

/* Signals */
enum
{
	CLICKED,

	SIGNAL_LAST
};

static guint XfdashboardLiveWindowSignals[SIGNAL_LAST]={ 0, };

/* Private constants */
#define DEFAULT_FONT	"Cantarell 12px"

static ClutterColor		defaultTextColor={ 0xff, 0xff , 0xff, 0xff };
static ClutterColor		defaultBackgroundColor={ 0x00, 0x00, 0x00, 0xd0 };

/* IMPLEMENTATION: Private variables and methods */

/* Set window and setup actors for display live image of window with accessiors */
void _xfdashboard_live_window_set_window(XfdashboardLiveWindow *self, const WnckWindow *inWindow)
{
	g_return_if_fail(XFDASHBOARD_IS_LIVE_WINDOW(self));
	g_return_if_fail(WNCK_IS_WINDOW(inWindow));

	/* Set window and create actors */
	XfdashboardLiveWindowPrivate	*priv=XFDASHBOARD_LIVE_WINDOW(self)->priv;

	g_return_if_fail(priv->window==NULL);

	priv->window=(WnckWindow*)inWindow;

	/* Create live-window */
	GdkPixbuf		*windowIcon;
	GError			*error;

	priv->actorWindow=clutter_x11_texture_pixmap_new_with_window(wnck_window_get_xid(priv->window));
	clutter_actor_set_parent(priv->actorWindow, CLUTTER_ACTOR(self));

	clutter_x11_texture_pixmap_set_automatic(CLUTTER_X11_TEXTURE_PIXMAP(priv->actorWindow), TRUE);

	/* Create label and its background */
	priv->actorLabel=clutter_text_new();
	clutter_actor_set_parent(priv->actorLabel, CLUTTER_ACTOR(self));

	clutter_text_set_text(CLUTTER_TEXT(priv->actorLabel), wnck_window_get_name(priv->window));
	clutter_text_set_single_line_mode(CLUTTER_TEXT(priv->actorLabel), TRUE);
	clutter_text_set_ellipsize(CLUTTER_TEXT(priv->actorLabel), priv->labelEllipsize);
	if(priv->labelFont) clutter_text_set_font_name(CLUTTER_TEXT(priv->actorLabel), priv->labelFont);
	if(priv->labelTextColor) clutter_text_set_color(CLUTTER_TEXT(priv->actorLabel), priv->labelTextColor);

	priv->actorLabelBackground=clutter_rectangle_new();
	clutter_actor_set_parent(priv->actorLabelBackground, CLUTTER_ACTOR(self));

	if(priv->labelBackgroundColor) clutter_rectangle_set_color(CLUTTER_RECTANGLE(priv->actorLabelBackground), priv->labelBackgroundColor);

	/* Create icon for application running in window */
	windowIcon=wnck_window_get_icon(priv->window);

	priv->actorAppIcon=clutter_texture_new();
	clutter_actor_set_parent(priv->actorAppIcon, CLUTTER_ACTOR(self));

	error=NULL;
	if(!clutter_texture_set_from_rgb_data(CLUTTER_TEXTURE(priv->actorAppIcon),
												gdk_pixbuf_get_pixels(windowIcon),
												gdk_pixbuf_get_has_alpha(windowIcon),
												gdk_pixbuf_get_width(windowIcon),
												gdk_pixbuf_get_height(windowIcon),
												gdk_pixbuf_get_rowstride(windowIcon),
												gdk_pixbuf_get_has_alpha(windowIcon) ? 4 : 3,
												CLUTTER_TEXTURE_NONE,
												&error))
	{
		g_warning("Could not create application icon actor: %s", (error && error->message) ?  error->message : "unknown error");
		if(error!=NULL) g_error_free(error);
	}

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
	clutter_actor_show(priv->actorLabelBackground);
	clutter_actor_show(priv->actorAppIcon);
	clutter_actor_show(self);
}

/* Hide all children of this one */
static void xfdashboard_live_window_hide_all(ClutterActor *self)
{
	XfdashboardLiveWindowPrivate	*priv=XFDASHBOARD_LIVE_WINDOW(self)->priv;

	clutter_actor_hide(self);
	clutter_actor_hide(priv->actorWindow);
	clutter_actor_hide(priv->actorLabel);
	clutter_actor_hide(priv->actorLabelBackground);
	clutter_actor_hide(priv->actorAppIcon);
}

/* Get preferred width/height */
static void xfdashboard_live_window_get_preferred_height(ClutterActor *self,
														gfloat inForWidth,
														gfloat *outMinHeight,
														gfloat *outNaturalHeight)
{
	XfdashboardLiveWindowPrivate	*priv=XFDASHBOARD_LIVE_WINDOW(self)->priv;

	clutter_actor_get_preferred_height(priv->actorWindow,
										inForWidth,
										outMinHeight,
										outNaturalHeight);
}

static void xfdashboard_live_window_get_preferred_width(ClutterActor *self,
														gfloat inForHeight,
														gfloat *outMinWidth,
														gfloat *outNaturalWidth)
{
	XfdashboardLiveWindowPrivate	*priv=XFDASHBOARD_LIVE_WINDOW(self)->priv;

	clutter_actor_get_preferred_width(priv->actorWindow,
										inForHeight,
										outMinWidth,
										outNaturalWidth);
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

	/* Set application icon */
	ClutterActorBox				*boxActorAppIcon;
	gdouble						iconWidth=clutter_actor_get_width(priv->actorAppIcon);
	gdouble						iconHeight=clutter_actor_get_height(priv->actorAppIcon);

	right=clutter_actor_box_get_x(boxActorWindow)+clutter_actor_box_get_width(boxActorWindow)-priv->labelMargin;
	left=right-iconWidth;
	bottom=clutter_actor_box_get_y(boxActorWindow)+clutter_actor_box_get_height(boxActorWindow)-priv->labelMargin;
	top=bottom-iconHeight;
	boxActorAppIcon=clutter_actor_box_new(left, top, right, bottom);
	clutter_actor_allocate(priv->actorAppIcon, boxActorAppIcon, inFlags);

	/* Set label actors */
	ClutterActorBox				*boxActorLabel, *boxActorLabelBackground;
	gfloat						textWidth, textHeight, maxRight;

	clutter_actor_get_preferred_width(priv->actorLabel, -1, NULL, &textWidth);
	textHeight=clutter_actor_get_height(priv->actorLabel);
	maxRight=clutter_actor_box_get_x(boxActorAppIcon)-(2*priv->labelMargin);

	if(textWidth>clutter_actor_box_get_width(inBox)) textWidth=clutter_actor_box_get_width(inBox);

	left=clutter_actor_box_get_x(boxActorWindow)+((clutter_actor_box_get_width(boxActorWindow)-textWidth)/2.0f);
	right=left+textWidth;
	bottom=clutter_actor_box_get_y(boxActorWindow)+clutter_actor_box_get_height(boxActorWindow)-(2*priv->labelMargin);
	top=bottom-textHeight;
	if(right>maxRight)
	{
		left+=(right-maxRight);
		right=maxRight;
	}
	if(left>right) left=right-1.0f;
	boxActorLabel=clutter_actor_box_new(left, top, right, bottom);
	clutter_actor_allocate(priv->actorLabel, boxActorLabel, inFlags);

	left-=priv->labelMargin;
	top-=priv->labelMargin;
	right+=priv->labelMargin;
	bottom+=priv->labelMargin;
	boxActorLabelBackground=clutter_actor_box_new(left, top, right, bottom);
	clutter_actor_allocate(priv->actorLabelBackground, boxActorLabelBackground, inFlags);

	/* Release allocated memory */
	clutter_actor_box_free(boxActorWindow);
	clutter_actor_box_free(boxActorLabel);
	clutter_actor_box_free(boxActorLabelBackground);
	clutter_actor_box_free(boxActorAppIcon);
}

/* Paint actor */
static void xfdashboard_live_window_paint(ClutterActor *self)
{
	XfdashboardLiveWindowPrivate	*priv=XFDASHBOARD_LIVE_WINDOW(self)->priv;

	/* Order of actors being painted is important! */
	if(priv->actorWindow &&
		CLUTTER_ACTOR_IS_MAPPED(priv->actorWindow)) clutter_actor_paint(priv->actorWindow);

	if(priv->actorLabelBackground &&
		CLUTTER_ACTOR_IS_MAPPED(priv->actorLabelBackground)) clutter_actor_paint(priv->actorLabelBackground);

	if(priv->actorLabel &&
		CLUTTER_ACTOR_IS_MAPPED(priv->actorLabel)) clutter_actor_paint(priv->actorLabel);

	if(priv->actorAppIcon &&
		CLUTTER_ACTOR_IS_MAPPED(priv->actorAppIcon)) clutter_actor_paint(priv->actorAppIcon);
}

/* Pick all the child actors */
static void xfdashboard_live_window_pick(ClutterActor *self, const ClutterColor *inColor)
{
	XfdashboardLiveWindowPrivate	*priv=XFDASHBOARD_LIVE_WINDOW(self)->priv;

	/* Chain up so we get a bounding box painted (if we are reactive) */
	CLUTTER_ACTOR_CLASS(xfdashboard_live_window_parent_class)->pick(self, inColor);

	/* clutter_actor_pick() might be deprecated by clutter_actor_paint().
	 * Do not know what to with ClutterColor here.
	 * Order of actors being painted is important!
	 */
	if(priv->actorWindow &&
		CLUTTER_ACTOR_IS_MAPPED(priv->actorWindow)) clutter_actor_paint(priv->actorWindow);

	if(priv->actorLabelBackground &&
		CLUTTER_ACTOR_IS_MAPPED(priv->actorLabelBackground)) clutter_actor_paint(priv->actorLabelBackground);

	if(priv->actorLabel &&
		CLUTTER_ACTOR_IS_MAPPED(priv->actorLabel)) clutter_actor_paint(priv->actorLabel);

	if(priv->actorAppIcon &&
		CLUTTER_ACTOR_IS_MAPPED(priv->actorAppIcon)) clutter_actor_paint(priv->actorAppIcon);
}

/* proxy ClickAction signals */
static void xfdashboard_live_window_clicked(ClutterClickAction *inAction,
											ClutterActor *inActor,
											gpointer inUserData)
{
	g_signal_emit(inActor, XfdashboardLiveWindowSignals[CLICKED], 0);
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

	if(priv->actorLabelBackground) clutter_actor_destroy(priv->actorLabelBackground);
	priv->actorLabelBackground=NULL;

	if(priv->actorAppIcon) clutter_actor_destroy(priv->actorAppIcon);
	priv->actorAppIcon=NULL;

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
	XfdashboardLiveWindow	*self=XFDASHBOARD_LIVE_WINDOW(inObject);

	g_free(self->priv->labelFont);
	self->priv->labelFont=NULL;

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
			
		case PROP_FONT:
			xfdashboard_live_window_set_font(self, g_value_get_string(inValue));
			break;

		case PROP_COLOR:
			xfdashboard_live_window_set_color(self, clutter_value_get_color(inValue));
			break;

		case PROP_BACKGROUND_COLOR:
			xfdashboard_live_window_set_background_color(self, clutter_value_get_color(inValue));
			break;

		case PROP_LABEL_MARGIN:
			xfdashboard_live_window_set_margin(self, g_value_get_float(inValue));
			break;

		case PROP_ELLIPSIZE_MODE:
			xfdashboard_live_window_set_ellipsize_mode(self, g_value_get_enum(inValue));
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

		case PROP_FONT:
			g_value_set_string(outValue, self->priv->labelFont);
			break;

		case PROP_COLOR:
			clutter_value_set_color(outValue, self->priv->labelTextColor);
			break;

		case PROP_BACKGROUND_COLOR:
			clutter_value_set_color(outValue, self->priv->labelBackgroundColor);
			break;

		case PROP_LABEL_MARGIN:
			g_value_set_float(outValue, self->priv->labelMargin);
			break;

		case PROP_ELLIPSIZE_MODE:
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

	XfdashboardLiveWindowProperties[PROP_FONT]=
		g_param_spec_string("font",
							"Label font",
							"Font description to use in label",
							DEFAULT_FONT,
							G_PARAM_READWRITE | G_PARAM_CONSTRUCT);

	XfdashboardLiveWindowProperties[PROP_COLOR]=
		clutter_param_spec_color("color",
									"Label text color",
									"Text color of label",
									&defaultTextColor,
									G_PARAM_READWRITE | G_PARAM_CONSTRUCT);

	XfdashboardLiveWindowProperties[PROP_BACKGROUND_COLOR]=
		clutter_param_spec_color("background-color",
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

	XfdashboardLiveWindowProperties[PROP_ELLIPSIZE_MODE]=
		g_param_spec_enum("ellipsize-mode",
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
	priv->actorLabelBackground=NULL;
	priv->actorAppIcon=NULL;

	priv->window=NULL;

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
const gchar* xfdashboard_live_window_get_font(XfdashboardLiveWindow *self)
{
	g_return_val_if_fail(XFDASHBOARD_IS_LIVE_WINDOW(self), NULL);

	return(self->priv->labelFont);
}

void xfdashboard_live_window_set_font(XfdashboardLiveWindow *self, const gchar *inFont)
{
	g_return_if_fail(XFDASHBOARD_IS_LIVE_WINDOW(self));
	g_return_if_fail(inFont!=NULL);

	/* Set font of label */
	XfdashboardLiveWindowPrivate	*priv=XFDASHBOARD_LIVE_WINDOW(self)->priv;

	if(priv->labelFont) g_free(priv->labelFont);
	priv->labelFont=g_strdup(inFont);

	clutter_text_set_font_name(CLUTTER_TEXT(priv->actorLabel), priv->labelFont);

	clutter_actor_queue_redraw(CLUTTER_ACTOR(self));
}

/* Get/set color of text in label */
const ClutterColor* xfdashboard_live_window_get_color(XfdashboardLiveWindow *self)
{
	g_return_val_if_fail(XFDASHBOARD_IS_LIVE_WINDOW(self), NULL);

	return(self->priv->labelTextColor);
}

void xfdashboard_live_window_set_color(XfdashboardLiveWindow *self, const ClutterColor *inColor)
{
	g_return_if_fail(XFDASHBOARD_IS_LIVE_WINDOW(self));

	/* Set text color of label */
	XfdashboardLiveWindowPrivate	*priv=XFDASHBOARD_LIVE_WINDOW(self)->priv;

	if(priv->labelTextColor) clutter_color_free(priv->labelTextColor);
	priv->labelTextColor=clutter_color_copy(inColor);

	clutter_text_set_color(CLUTTER_TEXT(priv->actorLabel), priv->labelTextColor);

	clutter_actor_queue_redraw(CLUTTER_ACTOR(self));
}

/* Get/set color of label's background */
const ClutterColor* xfdashboard_live_window_get_background_color(XfdashboardLiveWindow *self)
{
	g_return_val_if_fail(XFDASHBOARD_IS_LIVE_WINDOW(self), NULL);

	return(self->priv->labelBackgroundColor);
}

void xfdashboard_live_window_set_background_color(XfdashboardLiveWindow *self, const ClutterColor *inColor)
{
	g_return_if_fail(XFDASHBOARD_IS_LIVE_WINDOW(self));

	/* Set background color of label */
	XfdashboardLiveWindowPrivate	*priv=XFDASHBOARD_LIVE_WINDOW(self)->priv;

	if(priv->labelBackgroundColor) clutter_color_free(priv->labelBackgroundColor);
	priv->labelBackgroundColor=clutter_color_copy(inColor);

	clutter_rectangle_set_color(CLUTTER_RECTANGLE(priv->actorLabelBackground), priv->labelBackgroundColor);

	clutter_actor_queue_redraw(CLUTTER_ACTOR(self));
}

/* Get/set margin of background to label */
const gfloat xfdashboard_live_window_get_margin(XfdashboardLiveWindow *self)
{
	g_return_val_if_fail(XFDASHBOARD_IS_LIVE_WINDOW(self), 0);

	return(self->priv->labelMargin);
}

void xfdashboard_live_window_set_margin(XfdashboardLiveWindow *self, const gfloat inMargin)
{
	g_return_if_fail(XFDASHBOARD_IS_LIVE_WINDOW(self));

	/* Set window to display */
	XfdashboardLiveWindowPrivate	*priv=XFDASHBOARD_LIVE_WINDOW(self)->priv;

	priv->labelMargin=inMargin;

	clutter_actor_queue_redraw(CLUTTER_ACTOR(self));
}

/* Get/set ellipsize mode if label's text is getting too long */
const PangoEllipsizeMode xfdashboard_live_window_get_ellipsize_mode(XfdashboardLiveWindow *self)
{
	g_return_val_if_fail(XFDASHBOARD_IS_LIVE_WINDOW(self), 0);

	return(self->priv->labelEllipsize);
}

void xfdashboard_live_window_set_ellipsize_mode(XfdashboardLiveWindow *self, const PangoEllipsizeMode inMode)
{
	g_return_if_fail(XFDASHBOARD_IS_LIVE_WINDOW(self));

	/* Set window to display */
	XfdashboardLiveWindowPrivate	*priv=XFDASHBOARD_LIVE_WINDOW(self)->priv;

	priv->labelEllipsize=inMode;

	clutter_text_set_ellipsize(CLUTTER_TEXT(priv->actorLabel), priv->labelEllipsize);

	clutter_actor_queue_redraw(CLUTTER_ACTOR(self));
}
