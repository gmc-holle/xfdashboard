/*
 * winlist.c: Overview of all open windows of a workspace
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

/* TODO: #include "config.h"*/

#include "winlist.h"
#include "main.h"

#include <math.h>


/*----------------------------------------------------------------------------*/
/* DEFINITIONS */

/* Maps window to clutter actor */
typedef struct
{
	/* Actors for live window */
	ClutterActor	*actorWindow;
	ClutterActor	*actorLabel;
#if !CLUTTER_CHECK_VERSION(1,10,0)
	ClutterActor	*actorLabelBackground;
#endif
	ClutterActor	*actorAppIcon;

	/* Window the actors belong to */
	WnckWindow		*window;
} sPreviewWindow;

/* Configuration */
const char					*windowLabelFont="Cantarell 12px";
const ClutterColor			windowLabelTextColor={ 0xff, 0xff, 0xff, 0xff };
const ClutterColor			windowLabelBackgroundColor={ 0x00, 0x00, 0x00, 0xd0 };
const gint					windowLabelMargin=4;
const PangoEllipsizeMode	windowLabelEllipsize=PANGO_ELLIPSIZE_MIDDLE;
const gint					windowIconSize=32;


/*----------------------------------------------------------------------------*/
/* FUNCTIONS */

/* Forward declarations */
void previewWindow_destroy(sPreviewWindow *inMapping);


/* An actor representing an open window was clicked */
static gboolean previewWindow_onActorClicked(ClutterActor *inActor, ClutterEvent *inEvent, gpointer inUserData)
{
	sPreviewWindow	*previewWindow=(sPreviewWindow*)inUserData;

#if WNCK_CHECK_VERSION(2,10,0)
	wnck_window_activate_transient(previewWindow->window, CLUTTER_CURRENT_TIME);
#else
	wnck_window_activate_transient(previewWindow->window);
#endif

	clutter_main_quit();

	return(TRUE);
}

/* Create actor, set up actor and return mapping between window and actor */
sPreviewWindow* previewWindow_new(WnckWindow *inWindow, ClutterContainer *inParent)
{
	sPreviewWindow	*mapping;
	GdkPixbuf		*windowIcon;
	GError			*error;
#if CLUTTER_CHECK_VERSION(1,10,0)
	ClutterMargin	labelMargin={ windowLabelMargin, windowLabelMargin, windowLabelMargin, windowLabelMargin };
#endif
	
	g_return_val_if_fail(inWindow!=NULL, NULL);

	/* Create mapping between window and clutter actor */
	mapping=g_new(sPreviewWindow, 1);
	if(!mapping)
	{
		g_error("Cannot allocate memory for window-actor mapping!");
		return(NULL);
	}

	/* Set window in mapping the actors will belong to */
	mapping->window=inWindow;

	/* Create actor for window */
	mapping->actorWindow=clutter_x11_texture_pixmap_new_with_window(wnck_window_get_xid(inWindow));
	if(!mapping->actorWindow)
	{
		previewWindow_destroy(mapping);
		g_error("Error loading window texture for actor!");
		return(NULL);
	}
	clutter_x11_texture_pixmap_set_automatic(CLUTTER_X11_TEXTURE_PIXMAP(mapping->actorWindow), TRUE);

	clutter_actor_set_reactive(mapping->actorWindow, TRUE);
	g_signal_connect(mapping->actorWindow, "button-press-event", G_CALLBACK(previewWindow_onActorClicked), mapping);

	/* Create actor for label containg window name */
	mapping->actorLabel=clutter_text_new_full(windowLabelFont, wnck_window_get_name(inWindow), &windowLabelTextColor);
	if(!mapping->actorLabel)
	{
		previewWindow_destroy(mapping);
		g_error("Error creating label actor for window!");
		return(NULL);
	}
#if CLUTTER_CHECK_VERSION(1,10,0)
	clutter_actor_set_margin(mapping->actorLabel, &labelMargin);

	clutter_actor_set_background_color(mapping->actorLabel, &windowLabelBackgroundColor);
#else
	mapping->actorLabelBackground=clutter_rectangle_new_with_color(&windowLabelBackgroundColor);
	if(!mapping->actorLabelBackground)
	{
		previewWindow_destroy(mapping);
		g_error("Error creating label background actor for window!");
		return(NULL);
	}
#endif
	clutter_text_set_single_line_mode(CLUTTER_TEXT(mapping->actorLabel), TRUE);
	clutter_text_set_ellipsize(CLUTTER_TEXT(mapping->actorLabel), windowLabelEllipsize);

	/* Create actor for application icon of window */
	windowIcon=wnck_window_get_icon(inWindow);
	if(windowIcon)
	{
		/* Create texture actor */
		mapping->actorAppIcon=clutter_texture_new();
		if(!mapping->actorAppIcon)
		{
			previewWindow_destroy(mapping);
			g_error("Error creating application icon actor for window!");
			return(NULL);
		}
		if(!clutter_texture_set_from_rgb_data(CLUTTER_TEXTURE(mapping->actorAppIcon),
												gdk_pixbuf_get_pixels(windowIcon),
												gdk_pixbuf_get_has_alpha(windowIcon),
												gdk_pixbuf_get_width(windowIcon),
												gdk_pixbuf_get_height(windowIcon),
												gdk_pixbuf_get_rowstride(windowIcon),
												gdk_pixbuf_get_has_alpha(windowIcon) ? 4 : 3,
												CLUTTER_TEXTURE_NONE,
												&error))
		{
			previewWindow_destroy(mapping);
			g_error("Error creating application icon actor for window: %s", error->message ?  error->message : "unknown error");
			if(error!=NULL) g_error_free(error);
		}
	}
	
	/* Add actors to parent container (most likely the stage).
	 * Order is important! Otherwise we should set z-depth of each actor */
	clutter_container_add_actor(inParent, mapping->actorWindow);
#if !CLUTTER_CHECK_VERSION(1,10,0)
	clutter_container_add_actor(inParent, mapping->actorLabelBackground);
#endif
	clutter_container_add_actor(inParent, mapping->actorLabel);
	clutter_container_add_actor(inParent, mapping->actorAppIcon);

	return(mapping);
}

/* Destroy mapping and free resources */
void previewWindow_destroy(sPreviewWindow *inMapping)
{
	g_return_if_fail(inMapping!=NULL);

	if(inMapping->actorWindow) clutter_actor_destroy(inMapping->actorWindow);
	if(inMapping->actorLabel) clutter_actor_destroy(inMapping->actorLabel);
#if !CLUTTER_CHECK_VERSION(1,10,0)
	if(inMapping->actorLabelBackground) clutter_actor_destroy(inMapping->actorLabelBackground);
#endif
	if(inMapping->actorAppIcon) clutter_actor_destroy(inMapping->actorAppIcon);
	g_free(inMapping);
}

/* Set position and size of all actors for mapping */
void previewWindow_setPositionAndSize(sPreviewWindow *inMapping, gint inX, gint inY, gint inWidth, gint inHeight)
{
	g_return_if_fail(inMapping!=NULL);

	/* Set window actor */
	if(inMapping->actorWindow)
	{
		clutter_actor_set_position(inMapping->actorWindow, inX, inY);
		clutter_actor_set_size(inMapping->actorWindow, inWidth, inHeight);
	}

	/* Set label actor */
	if(inMapping->actorLabel)
	{
		gdouble		textWidth=clutter_actor_get_width(inMapping->actorLabel);
		gdouble		textHeight=clutter_actor_get_height(inMapping->actorLabel);

		if(textWidth>inWidth) textWidth=inWidth;

#if CLUTTER_CHECK_VERSION(1,10,0)
		clutter_actor_set_position(inMapping->actorLabel, inX+(inWidth/2.0f)-(textWidth/2.0), inY+inHeight-textHeight);
		clutter_actor_set_size(inMapping->actorLabel, textWidth, textHeight);
#else
		clutter_actor_set_position(inMapping->actorLabel, inX+(inWidth/2.0f)-(textWidth/2.0), inY+inHeight-textHeight-windowLabelMargin);
		clutter_actor_set_size(inMapping->actorLabel, textWidth, textHeight);

		clutter_actor_set_position(inMapping->actorLabelBackground,
									clutter_actor_get_x(inMapping->actorLabel)-windowLabelMargin,
									clutter_actor_get_y(inMapping->actorLabel)-windowLabelMargin);
		clutter_actor_set_size(inMapping->actorLabelBackground,
									clutter_actor_get_width(inMapping->actorLabel)+(2*windowLabelMargin),
									clutter_actor_get_height(inMapping->actorLabel)+(2*windowLabelMargin));
#endif
	}

	/* Set application icon */
	if(inMapping->actorAppIcon)
	{
		gdouble		iconWidth=clutter_actor_get_width(inMapping->actorAppIcon);
		gdouble		iconHeight=clutter_actor_get_height(inMapping->actorAppIcon);
		gdouble		iconSizeMax;
		gdouble		scale;
		
		/* Find max icon size */
		iconSizeMax=(inWidth<inHeight ? inWidth : inHeight);
		if(iconSizeMax>windowIconSize) iconSizeMax=windowIconSize;

		/* Resize icon */
		if(iconWidth>iconHeight) scale=iconSizeMax/iconWidth;
			else scale=iconSizeMax/iconHeight;

		iconWidth*=scale;
		iconHeight*=scale;

		clutter_actor_set_position(inMapping->actorAppIcon, inX+inWidth-iconWidth, inY+inHeight-iconHeight);
		clutter_actor_set_size(inMapping->actorAppIcon, iconWidth, iconHeight);
	}
}

/* Creates clutter actors for each window of workspace and releases any actor
 * created before
 */
void winlist_createActors(WnckScreen *inScreen, WnckWorkspace *inWorkspace)
{
	GList			*windowList;
	
	/* Release any actors representing windows created before */
	if(windows)
	{
		for(windowList=windows; windowList!=NULL; windowList=windowList->next)
		{
			previewWindow_destroy((sPreviewWindow*)windowList->data);
		}
		g_list_free(windows);
		windows=NULL;
	}
	 
	/* Create actors for workspace */
	for(windowList=wnck_screen_get_windows_stacked(inScreen); windowList!=NULL; windowList=windowList->next)
	{
		WnckWindow		*window=WNCK_WINDOW(windowList->data);
		sPreviewWindow	*mapping=NULL;

		/* Window must be on workspace and must not be flagged to skip tasklist */
		if(wnck_window_is_on_workspace(window, inWorkspace) &&
			!wnck_window_is_skip_tasklist(window))
		{
			/* Create actor and mapping between window and clutter actor */
			mapping=previewWindow_new(window, CLUTTER_CONTAINER(stage));
			if(!mapping) return;

			/* Add mapping to list */
			windows=g_list_append(windows, mapping);
		}
	}
	
	/* Layout actors */
	winlist_layoutActors();
}

/* Layouts all actors in overview */
void winlist_layoutActors()
{
	int		numberRows, numberCols;
	int		previewSizeW, previewSizeH, previewSize;
	int		row=0, col=0;
	int		x, y, w, h;
	gfloat	stageWidth, stageHeight;

	/* Reposition and resize actors depending on number of windows */
	numberCols=ceil(sqrt((double)g_list_length(windows)));
	numberRows=ceil((double)g_list_length(windows) / (double)numberCols);
	
	clutter_actor_get_size(stage, &stageWidth, &stageHeight);

	previewSizeW=floor(stageWidth / numberCols);
	previewSizeH=floor(stageHeight / numberRows);
	previewSize=(previewSizeW<previewSizeH ? previewSizeW : previewSizeH);
	
	for(GList *windowIter=windows; windowIter!=NULL; windowIter=windowIter->next)
	{
		sPreviewWindow	*mapping;
		WnckWindow		*window;
		int				winX, winY, winW, winH;

		/* Get mapping of window and actor */
		mapping=(sPreviewWindow*)(windowIter->data);
		
		/* Get geometry of window */
		window=WNCK_WINDOW(mapping->window);
		wnck_window_get_client_window_geometry(window, &winX, &winY, &winW, &winH);

		/* Calculate position and size of actor */
		if(winW>winH)
		{
			w=previewSize;
			h=((((gfloat)winH)/((gfloat)winW)) * ((gfloat)previewSize));
		}
			else
			{
				w=((((gfloat)winW)/((gfloat)winH)) * ((gfloat)previewSize));
				h=previewSize;
			}

		x=(col*previewSizeW)+((previewSizeW-w)/2.0);
		y=(row*previewSizeH)+((previewSizeH-h)/2.0);

		previewWindow_setPositionAndSize(mapping, x, y, w, h);

		/* Set up for next actor */
		col++;
		if(col>=numberCols)
		{
			col=0;
			row++;
		}
	}
}
