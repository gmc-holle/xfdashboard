/*
 * window: A managed window of window manager
 * 
 * Copyright 2012-2014 Stephan Haller <nomad@froevel.de>
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

#define COGL_ENABLE_EXPERIMENTAL_API
#define CLUTTER_ENABLE_EXPERIMENTAL_API

#include "window-content.h"

#include <glib/gi18n-lib.h>
#include <clutter/x11/clutter-x11.h>
#include <cogl/cogl-texture-pixmap-x11.h>
#ifdef HAVE_XCOMPOSITE
#include <X11/extensions/Xcomposite.h>
#endif
#ifdef HAVE_XDAMAGE
#include <X11/extensions/Xdamage.h>
#endif

#include "application.h"
#include "utils.h"
#include "marshal.h"

/* Define this class in GObject system */
static void _xdashboard_window_content_clutter_content_iface_init(ClutterContentIface *inInterface);

G_DEFINE_TYPE_WITH_CODE(XfdashboardWindowContent,
						xfdashboard_window_content,
						G_TYPE_OBJECT,
						G_IMPLEMENT_INTERFACE(CLUTTER_TYPE_CONTENT, _xdashboard_window_content_clutter_content_iface_init))

/* Private structure - access only by public API if needed */
#define XFDASHBOARD_WINDOW_CONTENT_GET_PRIVATE(obj) \
	(G_TYPE_INSTANCE_GET_PRIVATE((obj), XFDASHBOARD_TYPE_WINDOW_CONTENT, XfdashboardWindowContentPrivate))

struct _XfdashboardWindowContentPrivate
{
	/* Properties related */
	XfdashboardWindowTrackerWindow		*window;
	ClutterColor						*outlineColor;
	gfloat								outlineWidth;
	gboolean							isSuspended;

	/* Instance related */
	gboolean							isFallback;
	CoglTexture							*texture;
	Window								xWindowID;
	Pixmap								pixmap;
#ifdef HAVE_XDAMAGE
	Damage								damage;
#endif

	guint								suspendSignalID;
	gboolean							isMapped;
	gboolean							isAppSuspended;
};

/* Properties */
enum
{
	PROP_0,

	PROP_WINDOW_CONTENT,
	PROP_SUSPENDED,
	PROP_OUTLINE_COLOR,
	PROP_OUTLINE_WIDTH,

	PROP_LAST
};

static GParamSpec* XfdashboardWindowContentProperties[PROP_LAST]={ 0, };

/* IMPLEMENTATION: Private variables and methods */
#define COMPOSITE_VERSION_MIN_MAJOR		0
#define COMPOSITE_VERSION_MIN_MINOR		2

static gboolean		_xfdashboard_window_content_have_checked_extensions=FALSE;
static gboolean		_xfdashboard_window_content_have_composite_extension=FALSE;
static gboolean		_xfdashboard_window_content_have_damage_extension=FALSE;
static int			_xfdashboard_window_content_damage_event_base=0;

static GHashTable*	_xfdashboard_window_content_cache=NULL;
static guint		_xfdashboard_window_content_cache_shutdownSignalID=0;

/* Forward declarations */
static void _xfdashboard_window_content_suspend(XfdashboardWindowContent *self);
static void _xfdashboard_window_content_resume(XfdashboardWindowContent *self);

/* Check extension and set up basics */
static void _xfdashboard_window_content_check_extension(void)
{
	Display		*display G_GNUC_UNUSED;
#ifdef HAVE_XDAMAGE
	int			damageError=0;
#endif
#ifdef HAVE_XCOMPOSITE
	int			compositeMajor, compositeMinor;
#endif

	/* Check if we have already checked extensions */
	if(_xfdashboard_window_content_have_checked_extensions!=FALSE) return;

	/* Mark that we have check for extensions regardless of any error*/
	_xfdashboard_window_content_have_checked_extensions=TRUE;

	/* Get display */
	display=clutter_x11_get_default_display();

	/* Check for composite extenstion */
	_xfdashboard_window_content_have_composite_extension=FALSE;
#ifdef HAVE_XCOMPOSITE
	if(clutter_x11_has_composite_extension())
	{
		compositeMajor=compositeMinor=0;
		if(XCompositeQueryVersion(display, &compositeMajor, &compositeMinor))
		{
			if(compositeMajor>=COMPOSITE_VERSION_MIN_MAJOR && compositeMinor>=COMPOSITE_VERSION_MIN_MINOR)
			{
				_xfdashboard_window_content_have_composite_extension=TRUE;
			}
				else
				{
					g_warning(_("Need at least version %d.%d of composite extension but found %d.%d - using only fallback images"),
								COMPOSITE_VERSION_MIN_MAJOR, COMPOSITE_VERSION_MIN_MINOR, compositeMajor, compositeMinor);
				}
		}
			else g_warning(_("Query for X composite extension failed - using only fallback imagess"));
	}
		else g_warning(_("X does not support composite extension - using only fallback images"));
#endif

	/* Get base of damage event in X */
	_xfdashboard_window_content_have_damage_extension=FALSE;
	_xfdashboard_window_content_damage_event_base=0;

#ifdef HAVE_XDAMAGE
	if(!XDamageQueryExtension(display, &_xfdashboard_window_content_damage_event_base, &damageError))
	{
		g_warning(_("Query for X damage extension resulted in error code %d - using only still images of windows"), damageError);
	}
		else _xfdashboard_window_content_have_damage_extension=TRUE;
#endif
}

/* Suspension state of application changed */
static void _xfdashboard_window_content_on_application_suspended_changed(XfdashboardWindowContent *self,
																			GParamSpec *inSpec,
																			gpointer inUserData)
{
	XfdashboardWindowContentPrivate		*priv;
	XfdashboardApplication				*app;

	g_return_if_fail(XFDASHBOARD_IS_WINDOW_CONTENT(self));
	g_return_if_fail(XFDASHBOARD_IS_APPLICATION(inUserData));

	priv=self->priv;
	app=XFDASHBOARD_APPLICATION(inUserData);

	/* Get application suspend state */
	priv->isAppSuspended=xfdashboard_application_is_suspended(app);

	/* If application is suspended then suspend this window too ... */
	if(priv->isAppSuspended)
	{
		_xfdashboard_window_content_suspend(self);
	}
		/* ... otherwise resume window if it is mapped */
		else
		{
			if(priv->isMapped) _xfdashboard_window_content_resume(self);
		}
}

/* Filter X events for damages */
static ClutterX11FilterReturn _xfdashboard_window_content_on_x_event(XEvent *inXEvent, ClutterEvent *inEvent, gpointer inUserData)
{
	XfdashboardWindowContent			*self;
	XfdashboardWindowContentPrivate		*priv;

	g_return_val_if_fail(XFDASHBOARD_IS_WINDOW_CONTENT(inUserData), CLUTTER_X11_FILTER_CONTINUE);

	self=XFDASHBOARD_WINDOW_CONTENT(inUserData);
	priv=self->priv;

	/* Check for mapped, unmapped related X events as pixmap, damage, texture etc.
	 * needs to get resumed (acquired) or suspended (released)
	 */
	if(inXEvent->xany.window==priv->xWindowID)
	{
		switch(inXEvent->type)
		{
			case MapNotify:
			case ConfigureNotify:
				priv->isMapped=TRUE;
				if(!priv->isAppSuspended) _xfdashboard_window_content_resume(self);
				break;

			case UnmapNotify:
			case DestroyNotify:
				priv->isMapped=FALSE;
				_xfdashboard_window_content_suspend(self);
				break;

			default:
				/* We do not handle this type of X event, drop through ... */
				break;
		}
	}

	/* Check for damage event */
#ifdef HAVE_XDAMAGE
	if(_xfdashboard_window_content_have_damage_extension &&
		_xfdashboard_window_content_damage_event_base &&
		inXEvent->type==(_xfdashboard_window_content_damage_event_base + XDamageNotify) &&
		((XDamageNotifyEvent*)inXEvent)->damage==priv->damage)
	{
		/* Update texture for live window content */
		clutter_content_invalidate(CLUTTER_CONTENT(self));
	}
#endif

	return(CLUTTER_X11_FILTER_CONTINUE);
}

/* Release all resources used by this instance */
static void _xfdashboard_window_content_release_resources(XfdashboardWindowContent *self)
{
	XfdashboardWindowContentPrivate		*priv;
	Display								*display;
	gint								trapError;

	g_return_if_fail(XFDASHBOARD_IS_WINDOW_CONTENT(self));

	priv=self->priv;

	/* Get display as it used more than once ;) */
	display=clutter_x11_get_default_display();

	/* Release resources. It might be important to release them
	 * in reverse order as they were created.
	 */
	clutter_x11_remove_filter(_xfdashboard_window_content_on_x_event, (gpointer)self);

	clutter_x11_trap_x_errors();
	{
		if(priv->texture)
		{
			cogl_object_unref(priv->texture);
			priv->texture=NULL;
		}

#ifdef HAVE_XDAMAGE
		if(priv->damage!=None)
		{
			XDamageDestroy(display, priv->damage);
			XSync(display, False);
			priv->damage=None;
		}
#endif
		if(priv->pixmap!=None)
		{
			XFreePixmap(display, priv->pixmap);
			priv->pixmap=None;
		}

		if(priv->xWindowID!=None)
		{
#ifdef HAVE_XCOMPOSITE
			if(_xfdashboard_window_content_have_composite_extension)
			{
				XCompositeUnredirectWindow(display, priv->xWindowID, CompositeRedirectAutomatic);
				XSync(display, False);
			}
#endif
			priv->xWindowID=None;
		}

		/* Window is suspended now */
		if(priv->isSuspended!=TRUE)
		{
			priv->isSuspended=TRUE;

			/* Notify about property change */
			g_object_notify_by_pspec(G_OBJECT(self), XfdashboardWindowContentProperties[PROP_SUSPENDED]);
		}
	}

	/* Check if everything went well */
	trapError=clutter_x11_untrap_x_errors();
	if(trapError!=0)
	{
		g_debug(_("X error %d occured while releasing resources for window '%s"), trapError, xfdashboard_window_tracker_window_get_title(priv->window));
		return;
	}

	g_debug("Released resources for window '%s' to handle live texture updates", xfdashboard_window_tracker_window_get_title(priv->window));
}

/* Suspend from handling live updates */
static void _xfdashboard_window_content_suspend(XfdashboardWindowContent *self)
{
	XfdashboardWindowContentPrivate		*priv;
	Display								*display;
	gint								trapError;

	g_return_if_fail(XFDASHBOARD_IS_WINDOW_CONTENT(self));

	priv=self->priv;

	/* Get display as it used more than once ;) */
	display=clutter_x11_get_default_display();

	/* Release resources */
	clutter_x11_trap_x_errors();
	{
		/* Suspend live updates from texture */
		if(priv->texture && !priv->isFallback)
		{
#ifdef HAVE_XDAMAGE
			cogl_texture_pixmap_x11_set_damage_object(COGL_TEXTURE_PIXMAP_X11(priv->texture), 0, 0);
#endif
		}

		/* Release damage */
#ifdef HAVE_XDAMAGE
		if(priv->damage!=None)
		{
			XDamageDestroy(display, priv->damage);
			XSync(display, False);
			priv->damage=None;
		}
#endif

		/* Release pixmap */
		if(priv->pixmap!=None)
		{
			XFreePixmap(display, priv->pixmap);
			priv->pixmap=None;
		}

		/* Window is suspended now */
		if(priv->isSuspended!=TRUE)
		{
			priv->isSuspended=TRUE;

			/* Notify about property change */
			g_object_notify_by_pspec(G_OBJECT(self), XfdashboardWindowContentProperties[PROP_SUSPENDED]);
		}
	}

	/* Check if everything went well */
	trapError=clutter_x11_untrap_x_errors();
	if(trapError!=0)
	{
		g_debug(_("X error %d occured while suspending '%s"), trapError, xfdashboard_window_tracker_window_get_title(priv->window));
		return;
	}

	g_debug("Successfully suspended live texture updates for window '%s'", xfdashboard_window_tracker_window_get_title(priv->window));
}

/* Resume to handle live window updates */
static void _xfdashboard_window_content_resume(XfdashboardWindowContent *self)
{
	XfdashboardWindowContentPrivate		*priv;
	Display								*display G_GNUC_UNUSED;
	CoglContext							*context;
	GError								*error;
	gint								trapError;
	CoglTexture							*windowTexture;

	g_return_if_fail(XFDASHBOARD_IS_WINDOW_CONTENT(self));
	g_return_if_fail(self->priv->window);

	priv=self->priv;
	error=NULL;
	windowTexture=NULL;

	/* We need at least the X composite extension to display images of windows
	 * if still images or live updated ones
	 */
	if(!_xfdashboard_window_content_have_composite_extension) return;

	/* Get display as it used more than once ;) */
	display=clutter_x11_get_default_display();

	/* Set up resources */
	clutter_x11_trap_x_errors();
	while(1)
	{
#ifdef HAVE_XCOMPOSITE
		/* Get pixmap to render texture for */
		priv->pixmap=XCompositeNameWindowPixmap(display, priv->xWindowID);
		XSync(display, False);
		if(priv->pixmap==None)
		{
			g_warning(_("Could not get pixmap for window '%s"), xfdashboard_window_tracker_window_get_title(priv->window));
			_xfdashboard_window_content_suspend(self);
			break;
		}
#else
		/* We should never get here as existance of composite extension was checked before */
		g_critical("Cannot resume window '%s' as composite extension is not available",
					xfdashboard_window_tracker_window_get_title(priv->window));
		break;
#endif

		/* Create cogl X11 texture for live updates */
		context=clutter_backend_get_cogl_context(clutter_get_default_backend());
		windowTexture=COGL_TEXTURE(cogl_texture_pixmap_x11_new(context, priv->pixmap, TRUE, &error));
		if(!windowTexture || error)
		{
			/* Creating texture may fail if window is _NOT_ on active workspace
			 * so display error message just as debug message (this time)
			 */
			g_debug(_("Could not create texture for window '%s': %s"),
						xfdashboard_window_tracker_window_get_title(priv->window),
						error ? error->message : _("Unknown error"));
			if(error)
			{
				g_error_free(error);
				error=NULL;
			}

			if(windowTexture)
			{
				cogl_object_unref(windowTexture);
				windowTexture=NULL;
			}

			_xfdashboard_window_content_suspend(self);

			break;
		}

		/* Set up damage to get notified about changed in pixmap */
#ifdef HAVE_XDAMAGE
		if(_xfdashboard_window_content_have_damage_extension)
		{
			priv->damage=XDamageCreate(display, priv->pixmap, XDamageReportBoundingBox);
			XSync(display, False);
			if(priv->damage==None)
			{
				g_warning(_("Could not create damage for window '%s' - using still image of window"), xfdashboard_window_tracker_window_get_title(priv->window));
			}
		}
#endif

		/* Release old texture (should be the fallback texture) and set new texture */
		if(priv->texture)
		{
			cogl_object_unref(priv->texture);
			priv->texture=windowTexture;
		}

		/* Set damage to new window texture */
#ifdef HAVE_XDAMAGE
		if(_xfdashboard_window_content_have_damage_extension &&
			priv->damage!=None)
		{
			cogl_texture_pixmap_x11_set_damage_object(COGL_TEXTURE_PIXMAP_X11(priv->texture), priv->damage, COGL_TEXTURE_PIXMAP_X11_DAMAGE_BOUNDING_BOX);
		}
#endif

		/* Now we use the window as texture and not the fallback texture anymore */
		priv->isFallback=FALSE;

		/* Window is not suspended anymore */
		if(priv->isSuspended!=FALSE)
		{
			priv->isSuspended=FALSE;

			/* Notify about property change */
			g_object_notify_by_pspec(G_OBJECT(self), XfdashboardWindowContentProperties[PROP_SUSPENDED]);
		}

		/* End reached so break to get out of while loop */
		break;
	}

	/* Check if everything went well */
	trapError=clutter_x11_untrap_x_errors();
	if(trapError!=0)
	{
		g_debug(_("X error %d occured while resuming window '%s"), trapError, xfdashboard_window_tracker_window_get_title(priv->window));
		return;
	}

	g_debug("Resuming live texture updates for window '%s'", xfdashboard_window_tracker_window_get_title(priv->window));
}

/* Set window to handle and to display */
static void _xfdashboard_window_content_set_window(XfdashboardWindowContent *self, XfdashboardWindowTrackerWindow *inWindow)
{
	XfdashboardWindowContentPrivate		*priv;
	Display								*display;
	GdkPixbuf							*windowIcon;
	XWindowAttributes					windowAttrs;

	g_return_if_fail(XFDASHBOARD_IS_WINDOW_CONTENT(self));
	g_return_if_fail(inWindow!=NULL && XFDASHBOARD_IS_WINDOW_TRACKER_WINDOW(inWindow));
	g_return_if_fail(self->priv->window==NULL);

	priv=self->priv;

	/* Freeze notifications and collect them */
	g_object_freeze_notify(G_OBJECT(self));

	/* Get display as it used more than once ;) */
	display=clutter_x11_get_default_display();

	/* Set new value */
	priv->window=inWindow;

	/* Create fallback texture first in case we cannot create
	 * a live updated texture for window in the next steps
	 */
	windowIcon=xfdashboard_window_tracker_window_get_icon(priv->window);
	priv->texture=cogl_texture_new_from_data(gdk_pixbuf_get_width(windowIcon),
												gdk_pixbuf_get_height(windowIcon),
												COGL_TEXTURE_NONE,
												gdk_pixbuf_get_has_alpha(windowIcon) ? COGL_PIXEL_FORMAT_RGBA_8888 : COGL_PIXEL_FORMAT_RGB_888,
												COGL_PIXEL_FORMAT_ANY,
												gdk_pixbuf_get_rowstride(windowIcon),
												gdk_pixbuf_get_pixels(windowIcon));
	priv->isFallback=TRUE;

	/* Get X window and its attributes */
	priv->xWindowID=xfdashboard_window_tracker_window_get_xid(priv->window);
	if(!XGetWindowAttributes(display, priv->xWindowID, &windowAttrs))
	{
		g_warning(_("Could not get attributes of window '%s'"), xfdashboard_window_tracker_window_get_title(priv->window));
		XSync(display, False);
	}

	/* We need at least the X composite extension to display images of windows
	 * if still images or live updated ones by redirecting window
	 */
#ifdef HAVE_XCOMPOSITE
	if(_xfdashboard_window_content_have_composite_extension)
	{
		/* Redirect window */
		XCompositeRedirectWindow(display, priv->xWindowID, CompositeRedirectAutomatic);
		XSync(display, False);
	}
#endif

	/* We are interested in receiving mapping events of windows */
	XSelectInput(display, priv->xWindowID, windowAttrs.your_event_mask | StructureNotifyMask);

	/* Acquire new window and handle live updates */
	_xfdashboard_window_content_resume(self);
	priv->isMapped=!priv->isSuspended;

	/* Notify about property change */
	g_object_notify_by_pspec(G_OBJECT(self), XfdashboardWindowContentProperties[PROP_WINDOW_CONTENT]);

	/* Thaw notifications and send them now */
	g_object_thaw_notify(G_OBJECT(self));
}

/* Destroy cache hashtable */
static void _xfdashboard_window_content_destroy_cache(void)
{
	XfdashboardApplication		*application;
	gint						cacheSize;

	/* Only an existing cache can be destroyed */
	if(!_xfdashboard_window_content_cache) return;

	/* Disconnect application "shutdown" signal handler */
	application=xfdashboard_application_get_default();
	g_signal_handler_disconnect(application, _xfdashboard_window_content_cache_shutdownSignalID);
	_xfdashboard_window_content_cache_shutdownSignalID=0;

	/* Destroy cache hashtable */
	cacheSize=g_hash_table_size(_xfdashboard_window_content_cache);
	if(cacheSize>0) g_warning(_("Destroying window content cache still containing %d windows."), cacheSize);

	g_debug("Destroying window content cache hashtable");
	g_hash_table_destroy(_xfdashboard_window_content_cache);
	_xfdashboard_window_content_cache=NULL;
}

/* Create cache hashtable if not already set up */
static void _xfdashboard_window_content_create_cache(void)
{
	XfdashboardApplication		*application;

	/* Cache was already set up */
	if(_xfdashboard_window_content_cache) return;

	/* Create create hashtable */
	_xfdashboard_window_content_cache=g_hash_table_new(g_direct_hash, g_direct_equal);
	g_debug("Created window content cache hashtable");

	/* Connect to "shutdown" signal of application to
	 * clean up hashtable
	 */
	application=xfdashboard_application_get_default();
	_xfdashboard_window_content_cache_shutdownSignalID=g_signal_connect(application, "shutdown-final", G_CALLBACK(_xfdashboard_window_content_destroy_cache), NULL);
}

/* IMPLEMENTATION: ClutterContent */

/* Paint texture */
static void _xdashboard_window_content_clutter_content_iface_paint_content(ClutterContent *inContent,
																	ClutterActor *inActor,
																	ClutterPaintNode *inRootNode)
{
	XfdashboardWindowContent			*self=XFDASHBOARD_WINDOW_CONTENT(inContent);
	XfdashboardWindowContentPrivate		*priv=self->priv;
	ClutterScalingFilter				minFilter, magFilter;
	ClutterContentRepeat				repeatContent;
	ClutterPaintNode					*node;
	ClutterActorBox						actorBox;
	ClutterColor						color;
	guint8								opacity;
	ClutterColor						outlineColor;
	ClutterActorBox						outlinePath;

	/* Check if we have a texture to paint */
	if(priv->texture==NULL) return;

	/* Get needed data for painting */
	clutter_actor_get_content_box(inActor, &actorBox);
	clutter_actor_get_content_scaling_filters(inActor, &minFilter, &magFilter);
	opacity=clutter_actor_get_paint_opacity(inActor);
	repeatContent=clutter_actor_get_content_repeat(inActor);

	color.red=opacity;
	color.green=opacity;
	color.blue=opacity;
	color.alpha=opacity;

	/* Draw background if texture is a fallback */
	if(priv->isFallback)
	{
		ClutterColor					backgroundColor;

		/* Set up background color */
		backgroundColor.red=0;
		backgroundColor.green=0;
		backgroundColor.blue=0;
		backgroundColor.alpha=opacity;

		/* Draw background */
		node=clutter_color_node_new(&backgroundColor);
		clutter_paint_node_set_name(node, "fallback-background");
		clutter_paint_node_add_rectangle(node, &actorBox);
		clutter_paint_node_add_child(inRootNode, node);
		clutter_paint_node_unref(node);
	}

	/* Set up paint nodes for texture */
	node=clutter_texture_node_new(priv->texture, &color, minFilter, magFilter);
	clutter_paint_node_set_name(node, G_OBJECT_TYPE_NAME(self));

	if(repeatContent==CLUTTER_REPEAT_NONE)
	{
		clutter_paint_node_add_rectangle(node, &actorBox);
	}
		else
		{
			gfloat				textureWidth=1.0f;
			gfloat				textureHeight=1.0f;

			if((repeatContent & CLUTTER_REPEAT_X_AXIS)!=FALSE)
			{
				textureWidth=(actorBox.x2-actorBox.x1)/cogl_texture_get_width(priv->texture);
			}

			if((repeatContent & CLUTTER_REPEAT_Y_AXIS)!=FALSE)
			{
				textureHeight=(actorBox.y2-actorBox.y1)/cogl_texture_get_height(priv->texture);
			}

			clutter_paint_node_add_texture_rectangle(node,
														&actorBox,
														0.0f, 0.0f,
														textureWidth, textureHeight);
		}

	clutter_paint_node_add_child(inRootNode, node);
	clutter_paint_node_unref(node);

	/* Draw outline (color is depending is texture is fallback or not.
	 * That should be done last to get outline always visible
	 */
	if(priv->isFallback || priv->outlineColor==NULL)
	{
		outlineColor.red=0xff;
		outlineColor.green=0xff;
		outlineColor.blue=0xff;
		outlineColor.alpha=opacity;
	}
		else
		{
			outlineColor.red=priv->outlineColor->red;
			outlineColor.green=priv->outlineColor->green;
			outlineColor.blue=priv->outlineColor->blue;
			outlineColor.alpha=opacity;
		}

	node=clutter_color_node_new(&outlineColor);
	clutter_paint_node_set_name(node, "outline-top");
	clutter_actor_box_init_rect(&outlinePath, actorBox.x1, 0.0f, actorBox.x2-actorBox.x1, priv->outlineWidth);
	clutter_paint_node_add_rectangle(node, &outlinePath);
	clutter_paint_node_add_child(inRootNode, node);
	clutter_paint_node_unref(node);

	node=clutter_color_node_new(&outlineColor);
	clutter_paint_node_set_name(node, "outline-bottom");
	clutter_actor_box_init_rect(&outlinePath, actorBox.x1, actorBox.y2-priv->outlineWidth, actorBox.x2-actorBox.x1, priv->outlineWidth);
	clutter_paint_node_add_rectangle(node, &outlinePath);
	clutter_paint_node_add_child(inRootNode, node);
	clutter_paint_node_unref(node);

	node=clutter_color_node_new(&outlineColor);
	clutter_paint_node_set_name(node, "outline-left");
	clutter_actor_box_init_rect(&outlinePath, actorBox.x1, actorBox.y1, priv->outlineWidth, actorBox.y2-actorBox.y1);
	clutter_paint_node_add_rectangle(node, &outlinePath);
	clutter_paint_node_add_child(inRootNode, node);
	clutter_paint_node_unref(node);

	node=clutter_color_node_new(&outlineColor);
	clutter_paint_node_set_name(node, "outline-right");
	clutter_actor_box_init_rect(&outlinePath, actorBox.x2-priv->outlineWidth, actorBox.y1, priv->outlineWidth, actorBox.y2-actorBox.y1);
	clutter_paint_node_add_rectangle(node, &outlinePath);
	clutter_paint_node_add_child(inRootNode, node);
	clutter_paint_node_unref(node);
}

/* Get preferred size of texture */
static gboolean _xdashboard_window_content_clutter_content_iface_get_preferred_size(ClutterContent *inContent,
																					gfloat *outWidth,
																					gfloat *outHeight)
{
	XfdashboardWindowContentPrivate		*priv=XFDASHBOARD_WINDOW_CONTENT(inContent)->priv;
	gfloat								w, h;

	/* No texture - no size to retrieve */
	if(priv->texture==NULL) return(FALSE);

	/* Get sizes */
	if(priv->isFallback)
	{
		/* Is a fallback texture so get real window size */
		gint							windowW, windowH;

		xfdashboard_window_tracker_window_get_size(priv->window, &windowW, &windowH);
		w=windowW;
		h=windowH;
	}
		else
		{
			/* Get size of window texture */
			w=cogl_texture_get_width(priv->texture);
			h=cogl_texture_get_height(priv->texture);
		}

	/* Set result values */
	if(outWidth) *outWidth=w;
	if(outHeight) *outHeight=h;

	return(TRUE);
}

/* Initialize interface of type ClutterContent */
static void _xdashboard_window_content_clutter_content_iface_init(ClutterContentIface *inInterface)
{
	inInterface->get_preferred_size=_xdashboard_window_content_clutter_content_iface_get_preferred_size;
	inInterface->paint_content=_xdashboard_window_content_clutter_content_iface_paint_content;
}

/* IMPLEMENTATION: GObject */

/* Dispose this object */
static void _xfdashboard_window_content_dispose(GObject *inObject)
{
	XfdashboardWindowContent			*self=XFDASHBOARD_WINDOW_CONTENT(inObject);
	XfdashboardWindowContentPrivate		*priv=self->priv;

	/* Dispose allocated resources */
	_xfdashboard_window_content_release_resources(self);

	if(priv->window)
	{
		/* Remove from cache */
		g_debug("Removing window content for window '%s' with ref-count %d" ,
					xfdashboard_window_tracker_window_get_title(priv->window),
					G_OBJECT(self)->ref_count);
		g_hash_table_remove(_xfdashboard_window_content_cache, priv->window);

		/* Disconnect signals */
		g_signal_handlers_disconnect_by_data(priv->window, self);

		/* libwnck resources should never be freed. Just set to NULL */
		priv->window=NULL;
	}

	if(priv->suspendSignalID)
	{
		g_signal_handler_disconnect(xfdashboard_application_get_default(), priv->suspendSignalID);
		priv->suspendSignalID=0;
	}

	/* Call parent's class dispose method */
	G_OBJECT_CLASS(xfdashboard_window_content_parent_class)->dispose(inObject);
}

/* Set/get properties */
static void _xfdashboard_window_content_set_property(GObject *inObject,
												guint inPropID,
												const GValue *inValue,
												GParamSpec *inSpec)
{
	XfdashboardWindowContent	*self=XFDASHBOARD_WINDOW_CONTENT(inObject);

	switch(inPropID)
	{
		case PROP_WINDOW_CONTENT:
			_xfdashboard_window_content_set_window(self, XFDASHBOARD_WINDOW_TRACKER_WINDOW(g_value_get_object(inValue)));
			break;

		case PROP_OUTLINE_COLOR:
			xfdashboard_window_content_set_outline_color(self, clutter_value_get_color(inValue));
			break;

		case PROP_OUTLINE_WIDTH:
			xfdashboard_window_content_set_outline_width(self, g_value_get_float(inValue));
			break;

		default:
			G_OBJECT_WARN_INVALID_PROPERTY_ID(inObject, inPropID, inSpec);
			break;
	}
}

static void _xfdashboard_window_content_get_property(GObject *inObject,
												guint inPropID,
												GValue *outValue,
												GParamSpec *inSpec)
{
	XfdashboardWindowContent		*self=XFDASHBOARD_WINDOW_CONTENT(inObject);
	XfdashboardWindowContentPrivate	*priv=self->priv;

	switch(inPropID)
	{
		case PROP_WINDOW_CONTENT:
			g_value_set_object(outValue, priv->window);
			break;

		case PROP_SUSPENDED:
			g_value_set_boolean(outValue, priv->isSuspended);
			break;

		case PROP_OUTLINE_COLOR:
			clutter_value_set_color(outValue, priv->outlineColor);
			break;

		case PROP_OUTLINE_WIDTH:
			g_value_set_float(outValue, priv->outlineWidth);
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
void xfdashboard_window_content_class_init(XfdashboardWindowContentClass *klass)
{
	GObjectClass		*gobjectClass=G_OBJECT_CLASS(klass);

	/* Override functions */
	gobjectClass->dispose=_xfdashboard_window_content_dispose;
	gobjectClass->set_property=_xfdashboard_window_content_set_property;
	gobjectClass->get_property=_xfdashboard_window_content_get_property;

	/* Set up private structure */
	g_type_class_add_private(klass, sizeof(XfdashboardWindowContentPrivate));

	/* Define properties */
	XfdashboardWindowContentProperties[PROP_WINDOW_CONTENT]=
		g_param_spec_object("window",
							_("Window"),
							_("The window to handle and display"),
							XFDASHBOARD_TYPE_WINDOW_TRACKER_WINDOW,
							G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY);

	XfdashboardWindowContentProperties[PROP_SUSPENDED]=
		g_param_spec_boolean("suspended",
							_("Suspended"),
							_("Is this window suspended"),
							TRUE,
							G_PARAM_READABLE);

	XfdashboardWindowContentProperties[PROP_OUTLINE_COLOR]=
		clutter_param_spec_color("outline-color",
									_("Outline color"),
									_("Color to draw outline of mapped windows with"),
									CLUTTER_COLOR_Black,
									G_PARAM_READWRITE);

	XfdashboardWindowContentProperties[PROP_OUTLINE_WIDTH]=
		g_param_spec_float("outline-width",
							_("Outline width"),
							_("Width of line used to draw outline of mapped windows"),
							0.0f, G_MAXFLOAT,
							1.0f,
							G_PARAM_READWRITE);

	g_object_class_install_properties(gobjectClass, PROP_LAST, XfdashboardWindowContentProperties);
}

/* Object initialization
 * Create private structure and set up default values
 */
void xfdashboard_window_content_init(XfdashboardWindowContent *self)
{
	XfdashboardWindowContentPrivate		*priv;
	XfdashboardApplication				*app;

	priv=self->priv=XFDASHBOARD_WINDOW_CONTENT_GET_PRIVATE(self);

	/* Set default values */
	priv->window=NULL;
	priv->texture=NULL;
	priv->xWindowID=None;
	priv->pixmap=None;
#ifdef HAVE_XDAMAGE
	priv->damage=None;
#endif
	priv->isFallback=FALSE;
	priv->outlineColor=clutter_color_copy(CLUTTER_COLOR_Black);
	priv->outlineWidth=1.0f;
	priv->isSuspended=TRUE;
	priv->suspendSignalID=0;
	priv->isMapped=FALSE;

	/* Check extensions (will only be done once) */
	_xfdashboard_window_content_check_extension();

	/* Add event filter for this instance */
	clutter_x11_add_filter(_xfdashboard_window_content_on_x_event, self);

	/* Handle suspension signals from application */
	app=xfdashboard_application_get_default();
	priv->suspendSignalID=g_signal_connect_swapped(app,
													"notify::is-suspended",
													G_CALLBACK(_xfdashboard_window_content_on_application_suspended_changed),
													self);
	priv->isAppSuspended=xfdashboard_application_is_suspended(app);
}

/* Implementation: Public API */

/* Create new instance */
ClutterContent* xfdashboard_window_content_new_for_window(XfdashboardWindowTrackerWindow *inWindow)
{
	ClutterContent		*content;

	g_return_val_if_fail(XFDASHBOARD_IS_WINDOW_TRACKER_WINDOW(inWindow), NULL);

	/* If we a hash table (cache) set up lookup if window content is already cached
	 * and return a new reference to it
	 */
	if(_xfdashboard_window_content_cache &&
		g_hash_table_contains(_xfdashboard_window_content_cache, inWindow))
	{
		content=CLUTTER_CONTENT(g_hash_table_lookup(_xfdashboard_window_content_cache, inWindow));
		g_object_ref(content);
		g_debug("Using cached window content for '%s' - ref-count is now %d" ,
					xfdashboard_window_tracker_window_get_title(XFDASHBOARD_WINDOW_CONTENT(content)->priv->window),
					G_OBJECT(content)->ref_count);

		return(content);
	}

	/* Create window content */
	content=CLUTTER_CONTENT(g_object_new(XFDASHBOARD_TYPE_WINDOW_CONTENT,
											"window", inWindow,
											NULL));
	g_return_val_if_fail(content, NULL);

	/* Create cache if not available */
	if(!_xfdashboard_window_content_cache) _xfdashboard_window_content_create_cache();

	/* Store new window content into cache */
	g_hash_table_insert(_xfdashboard_window_content_cache, inWindow, content);
	g_debug("Added window content for '%s' with ref-count %d" ,
				xfdashboard_window_tracker_window_get_title(inWindow),
				G_OBJECT(content)->ref_count);

	return(content);
}

/* Get window to handle and to display */
XfdashboardWindowTrackerWindow* xfdashboard_window_content_get_window(XfdashboardWindowContent *self)
{
	g_return_val_if_fail(XFDASHBOARD_IS_WINDOW_CONTENT(self), NULL);

	return(self->priv->window);
}

/* Get state of suspension */
gboolean xfdashboard_window_content_is_suspended(XfdashboardWindowContent *self)
{
	g_return_val_if_fail(XFDASHBOARD_IS_WINDOW_CONTENT(self), TRUE);

	return(self->priv->isSuspended);
}

/* Get/set color to draw outline with */
const ClutterColor* xfdashboard_window_content_get_outline_color(XfdashboardWindowContent *self)
{
	g_return_val_if_fail(XFDASHBOARD_IS_WINDOW_CONTENT(self), NULL);

	return(self->priv->outlineColor);
}

void xfdashboard_window_content_set_outline_color(XfdashboardWindowContent *self, const ClutterColor *inColor)
{
	XfdashboardWindowContentPrivate		*priv;

	g_return_if_fail(XFDASHBOARD_IS_WINDOW_CONTENT(self));
	g_return_if_fail(inColor);

	priv=self->priv;

	/* Set value if changed */
	if(priv->outlineColor==NULL || clutter_color_equal(inColor, priv->outlineColor)==FALSE)
	{
		/* Set value */
		if(priv->outlineColor) clutter_color_free(priv->outlineColor);
		priv->outlineColor=clutter_color_copy(inColor);

		/* Invalidate ourselve to get us redrawn */
		clutter_content_invalidate(CLUTTER_CONTENT(self));

		/* Notify about property change */
		g_object_notify_by_pspec(G_OBJECT(self), XfdashboardWindowContentProperties[PROP_OUTLINE_COLOR]);
	}
}

/* Get/set line width for outline */
gfloat xfdashboard_window_content_get_outline_width(XfdashboardWindowContent *self)
{
	g_return_val_if_fail(XFDASHBOARD_IS_WINDOW_CONTENT(self), 0.0f);

	return(self->priv->outlineWidth);
}

void xfdashboard_window_content_set_outline_width(XfdashboardWindowContent *self, const gfloat inWidth)
{
	XfdashboardWindowContentPrivate		*priv;

	g_return_if_fail(XFDASHBOARD_IS_WINDOW_CONTENT(self));
	g_return_if_fail(inWidth>=0.0f);

	priv=self->priv;

	/* Set value if changed */
	if(priv->outlineWidth!=inWidth)
	{
		/* Set value */
		priv->outlineWidth=inWidth;

		/* Invalidate ourselve to get us redrawn */
		clutter_content_invalidate(CLUTTER_CONTENT(self));

		/* Notify about property change */
		g_object_notify_by_pspec(G_OBJECT(self), XfdashboardWindowContentProperties[PROP_OUTLINE_WIDTH]);
	}
}
