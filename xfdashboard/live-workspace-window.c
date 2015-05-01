/*
 * live-workspace-window: An actor used at workspace actor showing
 *                        a window and its content or its window icon
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

#include "live-workspace-window.h"

#include <glib/gi18n-lib.h>
#include <gdk/gdk.h>

#include "enums.h"
#include "window-content.h"
#include "image-content.h"

/* Define this class in GObject system */
G_DEFINE_TYPE(XfdashboardLiveWorkspaceWindow,
				xfdashboard_live_workspace_window,
				XFDASHBOARD_TYPE_BACKGROUND)

/* Private structure - access only by public API if needed */
#define XFDASHBOARD_LIVE_WORKSPACE_WINDOW_GET_PRIVATE(obj) \
	(G_TYPE_INSTANCE_GET_PRIVATE((obj), XFDASHBOARD_TYPE_LIVE_WORKSPACE_WINDOW, XfdashboardLiveWorkspaceWindowPrivate))

struct _XfdashboardLiveWorkspaceWindowPrivate
{
	/* Properties related */
	XfdashboardWindowTrackerWindow		*window;

	gboolean							showWindowContent;

	gboolean							windowIconFillKeepAspect;
	gboolean							windowIconXFill;
	gboolean							windowIconYFill;
	gfloat								windowIconXAlign;
	gfloat								windowIconYAlign;
	gfloat								windowIconXScale;
	gfloat								windowIconYScale;
	XfdashboardAnchorPoint				windowIconAnchorPoint;

	/* Instance related */
	ClutterActor						*windowIconActor;
};

/* Properties */
enum
{
	PROP_0,

	PROP_WINDOW,

	PROP_SHOW_WINDOW_CONTENT,

	PROP_WINDOW_ICON_FILL_KEEP_ASPECT,
	PROP_WINDOW_ICON_X_FILL,
	PROP_WINDOW_ICON_Y_FILL,
	PROP_WINDOW_ICON_X_ALIGN,
	PROP_WINDOW_ICON_Y_ALIGN,
	PROP_WINDOW_ICON_X_SCALE,
	PROP_WINDOW_ICON_Y_SCALE,
	PROP_WINDOW_ICON_ANCHOR_POINT,

	PROP_LAST
};

static GParamSpec* XfdashboardLiveWorkspaceWindowProperties[PROP_LAST]={ 0, };

/* IMPLEMENTATION: Private variables and methods */

/* IMPLEMENTATION: ClutterActor */

/* Allocate position and size of actor and its children */
static void _xfdashboard_live_workspace_window_allocate(ClutterActor *self,
														const ClutterActorBox *inBox,
														ClutterAllocationFlags inFlags)
{
	XfdashboardLiveWorkspaceWindowPrivate	*priv=XFDASHBOARD_LIVE_WORKSPACE_WINDOW(self)->priv;
	gfloat									availableWidth, availableHeight;
	ClutterContent							*content;
	ClutterActorBox							childAllocation={ 0, };

	/* Chain up to store the allocation of the actor */
	CLUTTER_ACTOR_CLASS(xfdashboard_live_workspace_window_parent_class)->allocate(self, inBox, inFlags);

	/* If window icon actor is not available stop here */
	if(!priv->windowIconActor) return;

	/* Get allocation size */
	availableWidth=inBox->x2-inBox->x1;
	availableHeight=inBox->y2-inBox->y1;

	/* By defalt cover whole allocation area with window icon */
	childAllocation.x1=0;
	childAllocation.y1=0;
	childAllocation.x2=availableWidth;
	childAllocation.y2=availableHeight;

	/* Get window icon actor's content */
	content=clutter_actor_get_content(priv->windowIconActor);

	/* Determine actor box allocation of window icon actor. We can skip calculation
	 * if window icon actor should be expanded in both (x and y) direction.
	 */
	if(content &&
		(!priv->windowIconXFill || !priv->windowIconYFill))
	{
		gfloat								iconWidth, iconHeight;

		/* Get size of window icon */
		clutter_content_get_preferred_size(content, &iconWidth, &iconHeight);
		iconWidth*=priv->windowIconXScale;
		iconHeight*=priv->windowIconYScale;

		/* Determine left and right boundary of window icon
		 * if window icon should not expand in X axis.
		 */
		if(!priv->windowIconXFill)
		{
			gfloat						offset;

			/* Get boundary in X axis depending on gravity and scaled width */
			offset=(priv->windowIconXAlign*availableWidth);
			switch(priv->windowIconAnchorPoint)
			{
				/* Align to left boundary.
				 * This is also the default if gravity is none or undefined.
				 */
				default:
				case XFDASHBOARD_ANCHOR_POINT_NONE:
				case XFDASHBOARD_ANCHOR_POINT_WEST:
				case XFDASHBOARD_ANCHOR_POINT_NORTH_WEST:
				case XFDASHBOARD_ANCHOR_POINT_SOUTH_WEST:
					break;

				/* Align to center of X axis */
				case XFDASHBOARD_ANCHOR_POINT_CENTER:
				case XFDASHBOARD_ANCHOR_POINT_NORTH:
				case XFDASHBOARD_ANCHOR_POINT_SOUTH:
					offset-=(iconWidth/2.0f);
					break;

				/* Align to right boundary */
				case XFDASHBOARD_ANCHOR_POINT_EAST:
				case XFDASHBOARD_ANCHOR_POINT_NORTH_EAST:
				case XFDASHBOARD_ANCHOR_POINT_SOUTH_EAST:
					offset-=iconWidth;
					break;
			}

			/* Set boundary in X axis */
			childAllocation.x1=offset;
			childAllocation.x2=childAllocation.x1+iconWidth;
		}

		/* Determine left and right boundary of window icon
		 * if window icon should not expand in X axis.
		 */
		if(!priv->windowIconYFill)
		{
			gfloat						offset;

			/* Get boundary in Y axis depending on gravity and scaled width */
			offset=(priv->windowIconYAlign*availableHeight);
			switch(priv->windowIconAnchorPoint)
			{
				/* Align to upper boundary.
				 * This is also the default if gravity is none or undefined.
				 */
				default:
				case XFDASHBOARD_ANCHOR_POINT_NONE:
				case XFDASHBOARD_ANCHOR_POINT_NORTH:
				case XFDASHBOARD_ANCHOR_POINT_NORTH_WEST:
				case XFDASHBOARD_ANCHOR_POINT_NORTH_EAST:
					break;

				/* Align to center of Y axis */
				case XFDASHBOARD_ANCHOR_POINT_CENTER:
				case XFDASHBOARD_ANCHOR_POINT_WEST:
				case XFDASHBOARD_ANCHOR_POINT_EAST:
					offset-=(iconHeight/2.0f);
					break;

				/* Align to lower boundary */
				case XFDASHBOARD_ANCHOR_POINT_SOUTH:
				case XFDASHBOARD_ANCHOR_POINT_SOUTH_WEST:
				case XFDASHBOARD_ANCHOR_POINT_SOUTH_EAST:
					offset-=iconHeight;
					break;
			}

			/* Set boundary in Y axis */
			childAllocation.y1=offset;
			childAllocation.y2=childAllocation.y1+iconHeight;
		}
	}

	/* If window icon actor should fill in both (x and y) direction and it should
	 * also keep the aspect then re-calculate allocation.
	 */
	if(content &&
		priv->windowIconXFill &&
		priv->windowIconYFill &&
		priv->windowIconFillKeepAspect)
	{
		gfloat								iconWidth, iconHeight;
		gfloat								iconAspectRatio;
		gfloat								aspectWidth, aspectHeight;

		/* Get size of window icon */
		clutter_content_get_preferred_size(content, &iconWidth, &iconHeight);

		/* Calculate aspect ratio and new position and size of window icon actor */
		iconAspectRatio=iconWidth/iconHeight;
		aspectWidth=availableWidth;
		aspectHeight=aspectWidth*iconAspectRatio;

		if(aspectHeight>availableHeight)
		{
			iconAspectRatio=iconHeight/iconWidth;
			aspectHeight=availableHeight;
			aspectWidth=aspectHeight*iconAspectRatio;
		}

		childAllocation.x1=(availableWidth-aspectWidth)/2.0f;
		childAllocation.x2=childAllocation.x1+aspectWidth;
		childAllocation.y1=(availableHeight-aspectHeight)/2.0f;
		childAllocation.y2=childAllocation.y1+aspectHeight;
	}

	/* Clip allocation of window icon actor to this actor's allocation */
	if(childAllocation.x1<0 ||
		childAllocation.x2>availableWidth ||
		childAllocation.y1<0 ||
		childAllocation.y2>availableHeight)
	{
		gfloat								clipX, clipY;

		clipX=0;
		if(childAllocation.x1<0) clipX=-childAllocation.x1;

		clipY=0;
		if(childAllocation.y1<0) clipY=-childAllocation.y1;

		clutter_actor_set_clip(priv->windowIconActor,
								clipX,
								clipY,
								availableWidth,
								availableHeight);
	}
		else
		{
			if(clutter_actor_has_clip(priv->windowIconActor))
			{
				clutter_actor_remove_clip(priv->windowIconActor);
			}
		}

	/* Set allocation of window icon actor */
	clutter_actor_allocate(priv->windowIconActor, &childAllocation, inFlags);
}

/* IMPLEMENTATION: GObject */

/* Dispose this object */
static void _xfdashboard_live_workspace_window_dispose(GObject *inObject)
{
	/* Release allocated variables */
	XfdashboardLiveWorkspaceWindowPrivate	*priv=XFDASHBOARD_LIVE_WORKSPACE_WINDOW(inObject)->priv;

	if(priv->windowIconActor)
	{
		clutter_actor_destroy(priv->windowIconActor);
		priv->windowIconActor=NULL;
	}

	/* Call parent's class dispose method */
	G_OBJECT_CLASS(xfdashboard_live_workspace_window_parent_class)->dispose(inObject);
}

/* Set/get properties */
static void _xfdashboard_live_workspace_window_set_property(GObject *inObject,
													guint inPropID,
													const GValue *inValue,
													GParamSpec *inSpec)
{
	XfdashboardLiveWorkspaceWindow			*self=XFDASHBOARD_LIVE_WORKSPACE_WINDOW(inObject);

	switch(inPropID)
	{
		case PROP_WINDOW:
			xfdashboard_live_workspace_window_set_window(self, XFDASHBOARD_WINDOW_TRACKER_WINDOW(g_value_get_object(inValue)));
			break;

		case PROP_SHOW_WINDOW_CONTENT:
			xfdashboard_live_workspace_window_set_show_window_content(self, g_value_get_boolean(inValue));
			break;

		case PROP_WINDOW_ICON_FILL_KEEP_ASPECT:
			xfdashboard_live_workspace_window_set_window_icon_fill_keep_aspect(self, g_value_get_boolean(inValue));
			break;

		case PROP_WINDOW_ICON_X_FILL:
			xfdashboard_live_workspace_window_set_window_icon_x_fill(self, g_value_get_boolean(inValue));
			break;

		case PROP_WINDOW_ICON_Y_FILL:
			xfdashboard_live_workspace_window_set_window_icon_y_fill(self, g_value_get_boolean(inValue));
			break;

		case PROP_WINDOW_ICON_X_ALIGN:
			xfdashboard_live_workspace_window_set_window_icon_x_align(self, g_value_get_float(inValue));
			break;

		case PROP_WINDOW_ICON_Y_ALIGN:
			xfdashboard_live_workspace_window_set_window_icon_y_align(self, g_value_get_float(inValue));
			break;

		case PROP_WINDOW_ICON_X_SCALE:
			xfdashboard_live_workspace_window_set_window_icon_x_scale(self, g_value_get_float(inValue));
			break;

		case PROP_WINDOW_ICON_Y_SCALE:
			xfdashboard_live_workspace_window_set_window_icon_y_scale(self, g_value_get_float(inValue));
			break;

		case PROP_WINDOW_ICON_ANCHOR_POINT:
			xfdashboard_live_workspace_window_set_window_icon_anchor_point(self, g_value_get_enum(inValue));
			break;

		default:
			G_OBJECT_WARN_INVALID_PROPERTY_ID(inObject, inPropID, inSpec);
			break;
	}
}

static void _xfdashboard_live_workspace_window_get_property(GObject *inObject,
													guint inPropID,
													GValue *outValue,
													GParamSpec *inSpec)
{
	XfdashboardLiveWorkspaceWindow			*self=XFDASHBOARD_LIVE_WORKSPACE_WINDOW(inObject);
	XfdashboardLiveWorkspaceWindowPrivate	*priv=self->priv;

	switch(inPropID)
	{
		case PROP_WINDOW:
			g_value_set_object(outValue, priv->window);
			break;

		case PROP_SHOW_WINDOW_CONTENT:
			g_value_set_boolean(outValue, priv->showWindowContent);
			break;

		case PROP_WINDOW_ICON_FILL_KEEP_ASPECT:
			g_value_set_boolean(outValue, priv->windowIconFillKeepAspect);
			break;

		case PROP_WINDOW_ICON_X_FILL:
			g_value_set_boolean(outValue, priv->windowIconXFill);
			break;

		case PROP_WINDOW_ICON_Y_FILL:
			g_value_set_boolean(outValue, priv->windowIconYFill);
			break;

		case PROP_WINDOW_ICON_X_ALIGN:
			g_value_set_float(outValue, priv->windowIconXAlign);
			break;

		case PROP_WINDOW_ICON_Y_ALIGN:
			g_value_set_float(outValue, priv->windowIconYAlign);
			break;

		case PROP_WINDOW_ICON_X_SCALE:
			g_value_set_float(outValue, priv->windowIconXScale);
			break;

		case PROP_WINDOW_ICON_Y_SCALE:
			g_value_set_float(outValue, priv->windowIconYScale);
			break;

		case PROP_WINDOW_ICON_ANCHOR_POINT:
			g_value_set_enum(outValue, priv->windowIconAnchorPoint);
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
static void xfdashboard_live_workspace_window_class_init(XfdashboardLiveWorkspaceWindowClass *klass)
{
	XfdashboardActorClass	*actorClass=XFDASHBOARD_ACTOR_CLASS(klass);
	ClutterActorClass		*clutterActorClass=CLUTTER_ACTOR_CLASS(klass);
	GObjectClass			*gobjectClass=G_OBJECT_CLASS(klass);

	/* Override functions */
	gobjectClass->dispose=_xfdashboard_live_workspace_window_dispose;
	gobjectClass->set_property=_xfdashboard_live_workspace_window_set_property;
	gobjectClass->get_property=_xfdashboard_live_workspace_window_get_property;

	clutterActorClass->allocate=_xfdashboard_live_workspace_window_allocate;

	/* Set up private structure */
	g_type_class_add_private(klass, sizeof(XfdashboardLiveWorkspaceWindowPrivate));

	/* Define properties */
	XfdashboardLiveWorkspaceWindowProperties[PROP_WINDOW]=
		g_param_spec_object("window",
							_("Window"),
							_("The window to handle and display"),
							XFDASHBOARD_TYPE_WINDOW_TRACKER_WINDOW,
							G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS);

	XfdashboardLiveWorkspaceWindowProperties[PROP_SHOW_WINDOW_CONTENT]=
		g_param_spec_boolean("show-window-content",
								_("show-window-content"),
								_("If TRUE the window content should be shown otherwise the window's icon will be shown"),
								TRUE,
								G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS);

	XfdashboardLiveWorkspaceWindowProperties[PROP_WINDOW_ICON_FILL_KEEP_ASPECT]=
		g_param_spec_boolean("window-icon-fill-keep-aspect",
							_("Window icon fill keep aspect"),
							_("Whether the window icon should keep their aspect when filling space. Only applies when filling in X and Y direction."),
							FALSE,
							G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS);

	XfdashboardLiveWorkspaceWindowProperties[PROP_WINDOW_ICON_X_FILL]=
		g_param_spec_boolean("window-icon-x-fill",
							_("Window icon X fill"),
							_("Whether the window icon should fill up horizontal space"),
							TRUE,
							G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS);

	XfdashboardLiveWorkspaceWindowProperties[PROP_WINDOW_ICON_Y_FILL]=
		g_param_spec_boolean("window-icon-y-fill",
							_("Window icon y fill"),
							_("Whether the window icon should fill up vertical space"),
							TRUE,
							G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS);

	XfdashboardLiveWorkspaceWindowProperties[PROP_WINDOW_ICON_X_ALIGN]=
		g_param_spec_float("window-icon-x-align",
							_("Window icon X align"),
							_("The alignment of the window icon on the X axis within the allocation in normalized coordinate between 0 and 1"),
							0.0f, 1.0f,
							0.0f,
							G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS);

	XfdashboardLiveWorkspaceWindowProperties[PROP_WINDOW_ICON_Y_ALIGN]=
		g_param_spec_float("window-icon-y-align",
							_("Window icon Y align"),
							_("The alignment of the window icon on the Y axis within the allocation in normalized coordinate between 0 and 1"),
							0.0f, 1.0f,
							0.0f,
							G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS);

	XfdashboardLiveWorkspaceWindowProperties[PROP_WINDOW_ICON_X_SCALE]=
		g_param_spec_float("window-icon-x-scale",
							_("Window icon X scale"),
							_("Scale factor of window icon on the X axis"),
							0.0f, G_MAXFLOAT,
							1.0f,
							G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS);

	XfdashboardLiveWorkspaceWindowProperties[PROP_WINDOW_ICON_Y_SCALE]=
		g_param_spec_float("window-icon-y-scale",
							_("Window icon Y scale"),
							_("Scale factor of window icon on the Y axis"),
							0.0f, G_MAXFLOAT,
							1.0f,
							G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS);

	XfdashboardLiveWorkspaceWindowProperties[PROP_WINDOW_ICON_ANCHOR_POINT]=
		g_param_spec_enum("window-icon-anchor-point",
							_("Window icon anchor point"),
							_("The anchor point of window icon"),
							XFDASHBOARD_TYPE_ANCHOR_POINT,
							XFDASHBOARD_ANCHOR_POINT_NONE,
							G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS);

	g_object_class_install_properties(gobjectClass, PROP_LAST, XfdashboardLiveWorkspaceWindowProperties);

	/* Define stylable properties */
	xfdashboard_actor_install_stylable_property(actorClass, XfdashboardLiveWorkspaceWindowProperties[PROP_SHOW_WINDOW_CONTENT]);
	xfdashboard_actor_install_stylable_property(actorClass, XfdashboardLiveWorkspaceWindowProperties[PROP_WINDOW_ICON_FILL_KEEP_ASPECT]);
	xfdashboard_actor_install_stylable_property(actorClass, XfdashboardLiveWorkspaceWindowProperties[PROP_WINDOW_ICON_X_FILL]);
	xfdashboard_actor_install_stylable_property(actorClass, XfdashboardLiveWorkspaceWindowProperties[PROP_WINDOW_ICON_Y_FILL]);
	xfdashboard_actor_install_stylable_property(actorClass, XfdashboardLiveWorkspaceWindowProperties[PROP_WINDOW_ICON_X_ALIGN]);
	xfdashboard_actor_install_stylable_property(actorClass, XfdashboardLiveWorkspaceWindowProperties[PROP_WINDOW_ICON_Y_ALIGN]);
	xfdashboard_actor_install_stylable_property(actorClass, XfdashboardLiveWorkspaceWindowProperties[PROP_WINDOW_ICON_X_SCALE]);
	xfdashboard_actor_install_stylable_property(actorClass, XfdashboardLiveWorkspaceWindowProperties[PROP_WINDOW_ICON_Y_SCALE]);
	xfdashboard_actor_install_stylable_property(actorClass, XfdashboardLiveWorkspaceWindowProperties[PROP_WINDOW_ICON_ANCHOR_POINT]);
}

/* Object initialization
 * Create private structure and set up default values
 */
static void xfdashboard_live_workspace_window_init(XfdashboardLiveWorkspaceWindow *self)
{
	XfdashboardLiveWorkspaceWindowPrivate	*priv;

	priv=self->priv=XFDASHBOARD_LIVE_WORKSPACE_WINDOW_GET_PRIVATE(self);

	/* Set up default values */
	priv->window=NULL;
	priv->showWindowContent=TRUE;
	priv->windowIconFillKeepAspect=FALSE;
	priv->windowIconXFill=TRUE;
	priv->windowIconYFill=TRUE;
	priv->windowIconXAlign=0.0f;
	priv->windowIconYAlign=0.0f;
	priv->windowIconXScale=1.0f;
	priv->windowIconYScale=1.0f;
	priv->windowIconAnchorPoint=XFDASHBOARD_ANCHOR_POINT_NONE;

	/* This actor reacts on events */
	clutter_actor_set_reactive(CLUTTER_ACTOR(self), TRUE);

	/* Set up window icon actor */
	priv->windowIconActor=xfdashboard_actor_new();
	clutter_actor_add_child(CLUTTER_ACTOR(self), CLUTTER_ACTOR(priv->windowIconActor));
}

/* IMPLEMENTATION: Public API */

/* Create new actor */
ClutterActor* xfdashboard_live_workspace_window_new(void)
{
	return(CLUTTER_ACTOR(g_object_new(XFDASHBOARD_TYPE_LIVE_WORKSPACE_WINDOW, NULL)));
}

ClutterActor* xfdashboard_live_workspace_window_new_for_window(XfdashboardWindowTrackerWindow *inWindow)
{
	g_return_val_if_fail(XFDASHBOARD_IS_WINDOW_TRACKER_WINDOW(inWindow), NULL);

	return(CLUTTER_ACTOR(g_object_new(XFDASHBOARD_TYPE_LIVE_WORKSPACE_WINDOW,
										"window", inWindow,
										NULL)));
}

/* Get/set window to show */
XfdashboardWindowTrackerWindow* xfdashboard_live_workspace_window_get_window(XfdashboardLiveWorkspaceWindow *self)
{
	g_return_val_if_fail(XFDASHBOARD_IS_LIVE_WORKSPACE_WINDOW(self), NULL);

	return(self->priv->window);
}

void xfdashboard_live_workspace_window_set_window(XfdashboardLiveWorkspaceWindow *self, XfdashboardWindowTrackerWindow *inWindow)
{
	XfdashboardLiveWorkspaceWindowPrivate	*priv;
	ClutterContent							*content;

	g_return_if_fail(XFDASHBOARD_IS_LIVE_WORKSPACE_WINDOW(self));
	g_return_if_fail(XFDASHBOARD_IS_WINDOW_TRACKER_WINDOW(inWindow));

	priv=self->priv;

	/* Set value if changed */
	if(priv->window!=inWindow)
	{
		/* Set value */
		priv->window=inWindow;

		/* Set up window content if window is set and should be shown ... */
		if(priv->window && priv->showWindowContent)
		{
			/* Set window content to show */
			content=xfdashboard_window_content_new_for_window(priv->window);
			clutter_actor_set_content(CLUTTER_ACTOR(self), content);
			g_object_unref(content);

			/* Remove image content from window icon actor and hide it*/
			if(priv->windowIconActor)
			{
				clutter_actor_set_content(CLUTTER_ACTOR(priv->windowIconActor), NULL);
				clutter_actor_hide(priv->windowIconActor);
			}
		}
			/* ... otherwise remove window content */
			else
			{
				/* Remove window content */
				clutter_actor_set_content(CLUTTER_ACTOR(self), NULL);

				/* Set window icon as image at window icon actor and show it */
				if(priv->windowIconActor)
				{
					content=xfdashboard_image_content_new_for_pixbuf(xfdashboard_window_tracker_window_get_icon(priv->window));
					clutter_actor_set_content(CLUTTER_ACTOR(priv->windowIconActor), content);
					g_object_unref(content);

					clutter_actor_show(priv->windowIconActor);
				}
			}

		/* Notify about property change */
		g_object_notify_by_pspec(G_OBJECT(self), XfdashboardLiveWorkspaceWindowProperties[PROP_WINDOW]);
	}
}

/* Get/set if the window content should be shown or the window's icon */
gboolean xfdashboard_live_workspace_window_get_show_window_content(XfdashboardLiveWorkspaceWindow *self)
{
	g_return_val_if_fail(XFDASHBOARD_IS_LIVE_WORKSPACE_WINDOW(self), TRUE);

	return(self->priv->showWindowContent);
}

void xfdashboard_live_workspace_window_set_show_window_content(XfdashboardLiveWorkspaceWindow *self, gboolean inShowWindowContent)
{
	XfdashboardLiveWorkspaceWindowPrivate	*priv;
	ClutterContent							*content;

	g_return_if_fail(XFDASHBOARD_IS_LIVE_WORKSPACE_WINDOW(self));

	priv=self->priv;

	/* Set value if changed */
	if(priv->showWindowContent!=inShowWindowContent)
	{
		/* Set value */
		priv->showWindowContent=inShowWindowContent;

		/* Set up window content if window is set and should be shown ... */
		if(priv->window && priv->showWindowContent)
		{
			/* Set window content to show */
			content=xfdashboard_window_content_new_for_window(priv->window);
			clutter_actor_set_content(CLUTTER_ACTOR(self), content);
			g_object_unref(content);

			/* Remove image content from window icon actor and hide it*/
			if(priv->windowIconActor)
			{
				clutter_actor_set_content(CLUTTER_ACTOR(priv->windowIconActor), NULL);
				clutter_actor_hide(priv->windowIconActor);
			}
		}
			/* ... otherwise remove window content */
			else
			{
				/* Remove window content */
				clutter_actor_set_content(CLUTTER_ACTOR(self), NULL);

				/* Set window icon as image at window icon actor and show it */
				if(priv->windowIconActor)
				{
					content=xfdashboard_image_content_new_for_pixbuf(xfdashboard_window_tracker_window_get_icon(priv->window));
					clutter_actor_set_content(CLUTTER_ACTOR(priv->windowIconActor), content);
					g_object_unref(content);

					clutter_actor_show(priv->windowIconActor);
				}
			}

		/* Notify about property change */
		g_object_notify_by_pspec(G_OBJECT(self), XfdashboardLiveWorkspaceWindowProperties[PROP_SHOW_WINDOW_CONTENT]);
	}
}

/* Get/set keep aspect when filling allocation area of window icon */
gboolean xfdashboard_live_workspace_window_get_window_icon_fill_keep_aspect(XfdashboardLiveWorkspaceWindow *self)
{
	g_return_val_if_fail(XFDASHBOARD_IS_LIVE_WORKSPACE_WINDOW(self), FALSE);

	return(self->priv->windowIconFillKeepAspect);
}

void xfdashboard_live_workspace_window_set_window_icon_fill_keep_aspect(XfdashboardLiveWorkspaceWindow *self, const gboolean inKeepAspect)
{
	XfdashboardLiveWorkspaceWindowPrivate		*priv;

	g_return_if_fail(XFDASHBOARD_IS_LIVE_WORKSPACE_WINDOW(self));

	priv=self->priv;

	/* Set value if changed */
	if(priv->windowIconFillKeepAspect!=inKeepAspect)
	{
		/* Set value */
		priv->windowIconFillKeepAspect=inKeepAspect;

		/* Force a redraw of this actor */
		clutter_actor_queue_relayout(CLUTTER_ACTOR(self));

		/* Notify about property change */
		g_object_notify_by_pspec(G_OBJECT(self), XfdashboardLiveWorkspaceWindowProperties[PROP_WINDOW_ICON_FILL_KEEP_ASPECT]);
	}
}

/* Get/set x fill of window icon */
gboolean xfdashboard_live_workspace_window_get_window_icon_x_fill(XfdashboardLiveWorkspaceWindow *self)
{
	g_return_val_if_fail(XFDASHBOARD_IS_LIVE_WORKSPACE_WINDOW(self), FALSE);

	return(self->priv->windowIconXFill);
}

void xfdashboard_live_workspace_window_set_window_icon_x_fill(XfdashboardLiveWorkspaceWindow *self, const gboolean inFill)
{
	XfdashboardLiveWorkspaceWindowPrivate		*priv;

	g_return_if_fail(XFDASHBOARD_IS_LIVE_WORKSPACE_WINDOW(self));

	priv=self->priv;

	/* Set value if changed */
	if(priv->windowIconXFill!=inFill)
	{
		/* Set value */
		priv->windowIconXFill=inFill;

		/* Force a redraw of this actor */
		clutter_actor_queue_relayout(CLUTTER_ACTOR(self));

		/* Notify about property change */
		g_object_notify_by_pspec(G_OBJECT(self), XfdashboardLiveWorkspaceWindowProperties[PROP_WINDOW_ICON_X_FILL]);
	}
}

/* Get/set y fill of window icon */
gboolean xfdashboard_live_workspace_window_get_window_icon_y_fill(XfdashboardLiveWorkspaceWindow *self)
{
	g_return_val_if_fail(XFDASHBOARD_IS_LIVE_WORKSPACE_WINDOW(self), FALSE);

	return(self->priv->windowIconYFill);
}

void xfdashboard_live_workspace_window_set_window_icon_y_fill(XfdashboardLiveWorkspaceWindow *self, const gboolean inFill)
{
	XfdashboardLiveWorkspaceWindowPrivate		*priv;

	g_return_if_fail(XFDASHBOARD_IS_LIVE_WORKSPACE_WINDOW(self));

	priv=self->priv;

	/* Set value if changed */
	if(priv->windowIconYFill!=inFill)
	{
		/* Set value */
		priv->windowIconYFill=inFill;

		/* Force a redraw of this actor */
		clutter_actor_queue_relayout(CLUTTER_ACTOR(self));

		/* Notify about property change */
		g_object_notify_by_pspec(G_OBJECT(self), XfdashboardLiveWorkspaceWindowProperties[PROP_WINDOW_ICON_Y_FILL]);
	}
}

/* Get/set x align of window icon */
gfloat xfdashboard_live_workspace_window_get_window_icon_x_align(XfdashboardLiveWorkspaceWindow *self)
{
	g_return_val_if_fail(XFDASHBOARD_IS_LIVE_WORKSPACE_WINDOW(self), 0.0f);

	return(self->priv->windowIconXAlign);
}

void xfdashboard_live_workspace_window_set_window_icon_x_align(XfdashboardLiveWorkspaceWindow *self, const gfloat inAlign)
{
	XfdashboardLiveWorkspaceWindowPrivate		*priv;

	g_return_if_fail(XFDASHBOARD_IS_LIVE_WORKSPACE_WINDOW(self));
	g_return_if_fail(inAlign>=0.0f && inAlign<=1.0f);

	priv=self->priv;

	/* Set value if changed */
	if(priv->windowIconXAlign!=inAlign)
	{
		/* Set value */
		priv->windowIconXAlign=inAlign;

		/* Force a redraw of this actor */
		clutter_actor_queue_relayout(CLUTTER_ACTOR(self));

		/* Notify about property change */
		g_object_notify_by_pspec(G_OBJECT(self), XfdashboardLiveWorkspaceWindowProperties[PROP_WINDOW_ICON_X_ALIGN]);
	}
}

/* Get/set y align of window icon */
gfloat xfdashboard_live_workspace_window_get_window_icon_y_align(XfdashboardLiveWorkspaceWindow *self)
{
	g_return_val_if_fail(XFDASHBOARD_IS_LIVE_WORKSPACE_WINDOW(self), 0.0f);

	return(self->priv->windowIconYAlign);
}

void xfdashboard_live_workspace_window_set_window_icon_y_align(XfdashboardLiveWorkspaceWindow *self, const gfloat inAlign)
{
	XfdashboardLiveWorkspaceWindowPrivate		*priv;

	g_return_if_fail(XFDASHBOARD_IS_LIVE_WORKSPACE_WINDOW(self));
	g_return_if_fail(inAlign>=0.0f && inAlign<=1.0f);

	priv=self->priv;

	/* Set value if changed */
	if(priv->windowIconYAlign!=inAlign)
	{
		/* Set value */
		priv->windowIconYAlign=inAlign;

		/* Force a redraw of this actor */
		clutter_actor_queue_relayout(CLUTTER_ACTOR(self));

		/* Notify about property change */
		g_object_notify_by_pspec(G_OBJECT(self), XfdashboardLiveWorkspaceWindowProperties[PROP_WINDOW_ICON_Y_ALIGN]);
	}
}

/* Get/set x scale of window icon */
gfloat xfdashboard_live_workspace_window_get_window_icon_x_scale(XfdashboardLiveWorkspaceWindow *self)
{
	g_return_val_if_fail(XFDASHBOARD_IS_LIVE_WORKSPACE_WINDOW(self), 0.0f);

	return(self->priv->windowIconXScale);
}

void xfdashboard_live_workspace_window_set_window_icon_x_scale(XfdashboardLiveWorkspaceWindow *self, const gfloat inScale)
{
	XfdashboardLiveWorkspaceWindowPrivate		*priv;

	g_return_if_fail(XFDASHBOARD_IS_LIVE_WORKSPACE_WINDOW(self));
	g_return_if_fail(inScale>=0.0f);

	priv=self->priv;

	/* Set value if changed */
	if(priv->windowIconXScale!=inScale)
	{
		/* Set value */
		priv->windowIconXScale=inScale;

		/* Force a redraw of this actor */
		clutter_actor_queue_relayout(CLUTTER_ACTOR(self));

		/* Notify about property change */
		g_object_notify_by_pspec(G_OBJECT(self), XfdashboardLiveWorkspaceWindowProperties[PROP_WINDOW_ICON_X_SCALE]);
	}
}

/* Get/set y scale of window icon */
gfloat xfdashboard_live_workspace_window_get_window_icon_y_scale(XfdashboardLiveWorkspaceWindow *self)
{
	g_return_val_if_fail(XFDASHBOARD_IS_LIVE_WORKSPACE_WINDOW(self), 0.0f);

	return(self->priv->windowIconYScale);
}

void xfdashboard_live_workspace_window_set_window_icon_y_scale(XfdashboardLiveWorkspaceWindow *self, const gfloat inScale)
{
	XfdashboardLiveWorkspaceWindowPrivate		*priv;

	g_return_if_fail(XFDASHBOARD_IS_LIVE_WORKSPACE_WINDOW(self));
	g_return_if_fail(inScale>=0.0f);

	priv=self->priv;

	/* Set value if changed */
	if(priv->windowIconYScale!=inScale)
	{
		/* Set value */
		priv->windowIconYScale=inScale;

		/* Force a redraw of this actor */
		clutter_actor_queue_relayout(CLUTTER_ACTOR(self));

		/* Notify about property change */
		g_object_notify_by_pspec(G_OBJECT(self), XfdashboardLiveWorkspaceWindowProperties[PROP_WINDOW_ICON_Y_SCALE]);
	}
}

/* Get/set gravity (anchor point) of window icon */
XfdashboardAnchorPoint xfdashboard_live_workspace_window_get_window_icon_anchor_point(XfdashboardLiveWorkspaceWindow *self)
{
	g_return_val_if_fail(XFDASHBOARD_IS_LIVE_WORKSPACE_WINDOW(self), XFDASHBOARD_ANCHOR_POINT_NONE);

	return(self->priv->windowIconAnchorPoint);
}

void xfdashboard_live_workspace_window_set_window_icon_anchor_point(XfdashboardLiveWorkspaceWindow *self, const XfdashboardAnchorPoint inAnchorPoint)
{
	XfdashboardLiveWorkspaceWindowPrivate		*priv;

	g_return_if_fail(XFDASHBOARD_IS_LIVE_WORKSPACE_WINDOW(self));
	g_return_if_fail(inAnchorPoint>=XFDASHBOARD_ANCHOR_POINT_NONE);
	g_return_if_fail(inAnchorPoint<=XFDASHBOARD_ANCHOR_POINT_CENTER);

	priv=self->priv;

	/* Set value if changed */
	if(priv->windowIconAnchorPoint!=inAnchorPoint)
	{
		/* Set value */
		priv->windowIconAnchorPoint=inAnchorPoint;

		/* Force a redraw of this actor */
		clutter_actor_queue_relayout(CLUTTER_ACTOR(self));

		/* Notify about property change */
		g_object_notify_by_pspec(G_OBJECT(self), XfdashboardLiveWorkspaceWindowProperties[PROP_WINDOW_ICON_ANCHOR_POINT]);
	}
}
