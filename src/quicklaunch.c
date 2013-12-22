/*
 * quicklaunch: Quicklaunch box
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

#include "quicklaunch.h"
#include "enums.h"
#include "application.h"
#include "application-button.h"
#include "toggle-button.h"
#include "drag-action.h"
#include "drop-action.h"
#include "applications-view.h"

#include <glib/gi18n-lib.h>
#include <math.h>
#include <gtk/gtk.h>

/* Define this class in GObject system */
G_DEFINE_TYPE(XfdashboardQuicklaunch,
				xfdashboard_quicklaunch,
				XFDASHBOARD_TYPE_BACKGROUND)
                                                
/* Private structure - access only by public API if needed */
#define XFDASHBOARD_QUICKLAUNCH_GET_PRIVATE(obj) \
	(G_TYPE_INSTANCE_GET_PRIVATE((obj), XFDASHBOARD_TYPE_QUICKLAUNCH, XfdashboardQuicklaunchPrivate))

struct _XfdashboardQuicklaunchPrivate
{
	/* Properties related */
	GPtrArray				*favourites;

	gfloat					normalIconSize;
	gfloat					scaleMin;
	gfloat					scaleMax;
	gfloat					scaleStep;

	gfloat					spacing;

	ClutterOrientation		orientation;

	/* Instance related */
	XfconfChannel			*xfconfChannel;

	gfloat					scaleCurrent;

	ClutterActor			*appsButton;
	ClutterActor			*trashButton;

	guint					dragMode;
	ClutterActor			*dragPreviewIcon;
};

/* Properties */
enum
{
	PROP_0,

	PROP_FAVOURITES,
	PROP_NORMAL_ICON_SIZE,
	PROP_SPACING,
	PROP_ORIENTATION,

	PROP_LAST
};

static GParamSpec* XfdashboardQuicklaunchProperties[PROP_LAST]={ 0, };

/* Signals */
enum
{
	SIGNAL_FAVOURITE_ADDED,
	SIGNAL_FAVOURITE_REMOVED,

	SIGNAL_LAST
};

static guint XfdashboardQuicklaunchSignals[SIGNAL_LAST]={ 0, };

/* IMPLEMENTATION: Private variables and methods */
#define DEFAULT_NORMAL_ICON_SIZE	64					// TODO: Replace by settings/theming object
#define DEFAULT_SCALE_MIN			0.1
#define DEFAULT_SCALE_MAX			1.0
#define DEFAULT_SCALE_STEP			0.1

#define DEFAULT_APPS_BUTTON_ICON	GTK_STOCK_HOME		// TODO: Replace by settings/theming object
#define DEFAULT_TRASH_BUTTON_ICON	GTK_STOCK_DELETE	// TODO: Replace by settings/theming object

#define DEFAULT_ORIENTATION			CLUTTER_ORIENTATION_VERTICAL

enum
{
	DRAG_MODE_NONE,
	DRAG_MODE_CREATE,
	DRAG_MODE_MOVE_EXISTING
};

/* Forward declarations */
static void _xfdashboard_quicklaunch_update_property_from_icons(XfdashboardQuicklaunch *self);

/* An application icon (favourite) in quicklaunch was clicked */
static void _xfdashboard_quicklaunch_on_favourite_clicked(XfdashboardQuicklaunch *self, gpointer inUserData)
{
	XfdashboardApplicationButton		*button;

	g_return_if_fail(XFDASHBOARD_IS_QUICKLAUNCH(self));
	g_return_if_fail(XFDASHBOARD_IS_APPLICATION_BUTTON(inUserData));

	button=XFDASHBOARD_APPLICATION_BUTTON(inUserData);

	/* Launch application */
	if(xfdashboard_application_button_execute(button))
	{
		/* Launching application seems to be successfuly so quit application */
		xfdashboard_application_quit();
		return;
	}
}

/* Drag of a quicklaunch icon begins */
static void _xfdashboard_quicklaunch_on_favourite_drag_begin(ClutterDragAction *inAction,
																ClutterActor *inActor,
																gfloat inStageX,
																gfloat inStageY,
																ClutterModifierType inModifiers,
																gpointer inUserData)
{
	XfdashboardQuicklaunch			*self;
	XfdashboardQuicklaunchPrivate	*priv;
	ClutterActor					*dragHandle;
	ClutterStage					*stage;
	const gchar						*desktopName;

	g_return_if_fail(CLUTTER_IS_DRAG_ACTION(inAction));
	g_return_if_fail(XFDASHBOARD_IS_APPLICATION_BUTTON(inActor));
	g_return_if_fail(XFDASHBOARD_IS_QUICKLAUNCH(inUserData));

	self=XFDASHBOARD_QUICKLAUNCH(inUserData);
	priv=self->priv;

	/* Prevent signal "clicked" from being emitted on dragged icon */
	g_signal_handlers_block_by_func(inActor, _xfdashboard_quicklaunch_on_favourite_clicked, inUserData);

	/* Get stage */
	stage=CLUTTER_STAGE(clutter_actor_get_stage(inActor));

	/* Create a clone of application icon for drag handle and hide it
	 * initially. It is only shown if pointer is outside of quicklaunch.
	 */
	desktopName=xfdashboard_application_button_get_desktop_filename(XFDASHBOARD_APPLICATION_BUTTON(inActor));

	dragHandle=xfdashboard_application_button_new_from_desktop_file(desktopName);
	clutter_actor_set_position(dragHandle, inStageX, inStageY);
	xfdashboard_button_set_icon_size(XFDASHBOARD_BUTTON(dragHandle), priv->normalIconSize);
	xfdashboard_button_set_sync_icon_size(XFDASHBOARD_BUTTON(dragHandle), FALSE);
	xfdashboard_button_set_style(XFDASHBOARD_BUTTON(dragHandle), XFDASHBOARD_STYLE_ICON);
	clutter_actor_add_child(CLUTTER_ACTOR(stage), dragHandle);

	clutter_drag_action_set_drag_handle(inAction, dragHandle);
}

/* Drag of a quicklaunch icon ends */
static void _xfdashboard_quicklaunch_on_favourite_drag_end(ClutterDragAction *inAction,
															ClutterActor *inActor,
															gfloat inStageX,
															gfloat inStageY,
															ClutterModifierType inModifiers,
															gpointer inUserData)
{
	ClutterActor					*dragHandle;

	g_return_if_fail(CLUTTER_IS_DRAG_ACTION(inAction));
	g_return_if_fail(XFDASHBOARD_IS_APPLICATION_BUTTON(inActor));
	g_return_if_fail(XFDASHBOARD_IS_QUICKLAUNCH(inUserData));

	/* Destroy clone of application icon used as drag handle */
	dragHandle=clutter_drag_action_get_drag_handle(inAction);
	if(dragHandle)
	{
#if CLUTTER_CHECK_VERSION(1, 14, 0)
		/* Only unset drag handle if not running Clutter in version
		 * 1.12. This prevents a critical warning message in 1.12.
		 * Later versions of Clutter are fixed already.
		 */
		clutter_drag_action_set_drag_handle(inAction, NULL);
#endif
		clutter_actor_destroy(dragHandle);
	}

	/* Allow signal "clicked" from being emitted again */
	g_signal_handlers_unblock_by_func(inActor, _xfdashboard_quicklaunch_on_favourite_clicked, inUserData);
}

/* Drag of an actor to quicklaunch as drop target begins */
static gboolean _xfdashboard_quicklaunch_on_drop_begin(XfdashboardQuicklaunch *self,
														XfdashboardDragAction *inDragAction,
														gpointer inUserData)
{
	XfdashboardQuicklaunchPrivate	*priv;
	ClutterActor					*dragSource;
	ClutterActor					*draggedActor;
	const gchar						*desktopName;

	g_return_val_if_fail(XFDASHBOARD_IS_QUICKLAUNCH(self), FALSE);
	g_return_val_if_fail(XFDASHBOARD_IS_DRAG_ACTION(inDragAction), FALSE);
	g_return_val_if_fail(XFDASHBOARD_IS_DROP_ACTION(inUserData), FALSE);

	priv=self->priv;

	/* Get source where dragging started and actor being dragged */
	dragSource=xfdashboard_drag_action_get_source(inDragAction);
	draggedActor=xfdashboard_drag_action_get_actor(inDragAction);

	/* Check if we can handle dragged actor from given source */
	priv->dragMode=DRAG_MODE_NONE;

	if(XFDASHBOARD_IS_QUICKLAUNCH(dragSource) &&
		XFDASHBOARD_IS_APPLICATION_BUTTON(draggedActor) &&
		xfdashboard_application_button_get_desktop_filename(XFDASHBOARD_APPLICATION_BUTTON(draggedActor)))
	{
		priv->dragMode=DRAG_MODE_MOVE_EXISTING;
	}

	if(!XFDASHBOARD_IS_QUICKLAUNCH(dragSource) &&
		XFDASHBOARD_IS_APPLICATION_BUTTON(draggedActor) &&
		xfdashboard_application_button_get_desktop_filename(XFDASHBOARD_APPLICATION_BUTTON(draggedActor)))
	{
		priv->dragMode=DRAG_MODE_CREATE;
	}

	/* Create a visible copy of dragged application button and insert it
	 * after dragged icon in quicklaunch. This one is the one which is
	 * moved within quicklaunch. It is used as preview how quicklaunch
	 * will look like if drop will be successful. It is also needed to
	 * restore original order of all favourite icons if drag was
	 * cancelled by just destroying this preview icon.
	 * If dragging an favourite icon we will hide the it and leave it
	 * untouched when drag is cancelled.
	 */
	if(priv->dragMode!=DRAG_MODE_NONE)
	{
		desktopName=xfdashboard_application_button_get_desktop_filename(XFDASHBOARD_APPLICATION_BUTTON(draggedActor));

		priv->dragPreviewIcon=xfdashboard_application_button_new_from_desktop_file(desktopName);
		xfdashboard_button_set_icon_size(XFDASHBOARD_BUTTON(priv->dragPreviewIcon), priv->normalIconSize);
		xfdashboard_button_set_sync_icon_size(XFDASHBOARD_BUTTON(priv->dragPreviewIcon), FALSE);
		xfdashboard_button_set_style(XFDASHBOARD_BUTTON(priv->dragPreviewIcon), XFDASHBOARD_STYLE_ICON);
		if(priv->dragMode==DRAG_MODE_CREATE) clutter_actor_hide(priv->dragPreviewIcon);
		clutter_actor_add_child(CLUTTER_ACTOR(self), priv->dragPreviewIcon);

		if(priv->dragMode==DRAG_MODE_MOVE_EXISTING)
		{
			clutter_actor_set_child_below_sibling(CLUTTER_ACTOR(self),
													priv->dragPreviewIcon,
													draggedActor);
			clutter_actor_hide(draggedActor);
		}
	}

	/* If drag mode is set return TRUE because we can handle dragged actor
	 * otherwise return FALSE
	 */
	return(priv->dragMode!=DRAG_MODE_NONE ? TRUE : FALSE);
}

/* Dragged actor was dropped on this drop target */
static void _xfdashboard_quicklaunch_on_drop_drop(XfdashboardQuicklaunch *self,
													XfdashboardDragAction *inDragAction,
													gfloat inX,
													gfloat inY,
													gpointer inUserData)
{
	XfdashboardQuicklaunchPrivate		*priv;
	ClutterActor						*draggedActor;
	GAppInfo							*appInfo;

	g_return_if_fail(XFDASHBOARD_IS_QUICKLAUNCH(self));
	g_return_if_fail(XFDASHBOARD_IS_DRAG_ACTION(inDragAction));
	g_return_if_fail(XFDASHBOARD_IS_DROP_ACTION(inUserData));

	priv=self->priv;

	/* Get dragged actor */
	draggedActor=xfdashboard_drag_action_get_actor(inDragAction);

	/* Emit signal when a favourite icon was added */
	if(priv->dragMode==DRAG_MODE_CREATE)
	{
		appInfo=xfdashboard_application_button_get_app_info(XFDASHBOARD_APPLICATION_BUTTON(draggedActor));
		if(appInfo)
		{
			xfdashboard_notify(CLUTTER_ACTOR(self),
								xfdashboard_application_button_get_icon_name(XFDASHBOARD_APPLICATION_BUTTON(draggedActor)),
								_("Favourite '%s' added"),
								xfdashboard_application_button_get_display_name(XFDASHBOARD_APPLICATION_BUTTON(draggedActor)));
			g_signal_emit(self, XfdashboardQuicklaunchSignals[SIGNAL_FAVOURITE_ADDED], 0, appInfo);
			g_object_unref(appInfo);
		}
	}

	/* If drag mode is reorder move originally dragged application icon
	 * to its final position and destroy preview application icon.
	 */
	if(priv->dragMode==DRAG_MODE_MOVE_EXISTING)
	{
		clutter_actor_set_child_below_sibling(CLUTTER_ACTOR(self),
												draggedActor,
												priv->dragPreviewIcon);
		clutter_actor_show(draggedActor);

		if(priv->dragPreviewIcon)
		{
			clutter_actor_destroy(priv->dragPreviewIcon);
			priv->dragPreviewIcon=NULL;
		}
	}

	/* Update favourites from icon order */
	_xfdashboard_quicklaunch_update_property_from_icons(self);

	/* Reset drag mode */
	priv->dragMode=DRAG_MODE_NONE;
}

/* Drag of an actor to this drop target ended without actor being dropped here */
static void _xfdashboard_quicklaunch_on_drop_end(XfdashboardQuicklaunch *self,
													XfdashboardDragAction *inDragAction,
													gpointer inUserData)
{
	XfdashboardQuicklaunchPrivate		*priv;
	ClutterActor						*draggedActor;

	g_return_if_fail(XFDASHBOARD_IS_QUICKLAUNCH(self));
	g_return_if_fail(XFDASHBOARD_IS_DRAG_ACTION(inDragAction));
	g_return_if_fail(XFDASHBOARD_IS_DROP_ACTION(inUserData));

	priv=self->priv;

	/* Get dragged actor */
	draggedActor=xfdashboard_drag_action_get_actor(inDragAction);

	/* Destroy preview icon and show originally dragged favourite icon.
	 * Doing it this way will restore the order of favourite icons.
	 */
	if(priv->dragPreviewIcon)
	{
		clutter_actor_destroy(priv->dragPreviewIcon);
		priv->dragPreviewIcon=NULL;
	}

	if(priv->dragMode==DRAG_MODE_MOVE_EXISTING) clutter_actor_show(draggedActor);
	priv->dragMode=DRAG_MODE_NONE;
}

/* Drag of an actor entered this drop target */
static void _xfdashboard_quicklaunch_on_drop_enter(XfdashboardQuicklaunch *self,
													XfdashboardDragAction *inDragAction,
													gpointer inUserData)
{
	ClutterActor					*dragHandle;

	g_return_if_fail(XFDASHBOARD_IS_QUICKLAUNCH(self));
	g_return_if_fail(XFDASHBOARD_IS_DRAG_ACTION(inDragAction));
	g_return_if_fail(XFDASHBOARD_IS_DROP_ACTION(inUserData));

	/* Get source where dragging started and actor being dragged */
	dragHandle=clutter_drag_action_get_drag_handle(CLUTTER_DRAG_ACTION(inDragAction));

	/* Hide drag handle in this quicklaunch */
	clutter_actor_hide(dragHandle);
}

/* Drag of an actor moves over this drop target */
static void _xfdashboard_quicklaunch_on_drop_motion(XfdashboardQuicklaunch *self,
													XfdashboardDragAction *inDragAction,
													gfloat inX,
													gfloat inY,
													gpointer inUserData)
{
	XfdashboardQuicklaunchPrivate	*priv;
	ClutterActor					*draggedActor;
	ClutterActor					*dragHandle;

	g_return_if_fail(XFDASHBOARD_IS_QUICKLAUNCH(self));
	g_return_if_fail(XFDASHBOARD_IS_DRAG_ACTION(inDragAction));
	g_return_if_fail(XFDASHBOARD_IS_DROP_ACTION(inUserData));

	priv=self->priv;

	/* Get actor being dragged and the drag handle */
	draggedActor=xfdashboard_drag_action_get_actor(inDragAction);
	dragHandle=clutter_drag_action_get_drag_handle(CLUTTER_DRAG_ACTION(inDragAction));

	/* Get new position of preview application icon in quicklaunch
	 * if dragged actor is an application icon
	 */
	if(XFDASHBOARD_IS_APPLICATION_BUTTON(draggedActor))
	{
		ClutterStage				*stage;
		gboolean					oldPreviewReactive;
		gboolean					oldHandleReactive;
		gfloat						stageX, stageY;
		gfloat						deltaX, deltaY;
		ClutterActor				*actorUnderMouse;

		/* Preview icon and drag handle should not be reactive to prevent
		 * clutter_stage_get_actor_at_pos() choosing one of both as the
		 * actor under mouse. But remember their state to reset it later.
		 */
		oldPreviewReactive=clutter_actor_get_reactive(priv->dragPreviewIcon);
		clutter_actor_set_reactive(priv->dragPreviewIcon, FALSE);

		oldHandleReactive=clutter_actor_get_reactive(dragHandle);
		clutter_actor_set_reactive(dragHandle, FALSE);

		/* Get new position and move preview icon */
		clutter_drag_action_get_motion_coords(CLUTTER_DRAG_ACTION(inDragAction), &stageX, &stageY);
		xfdashboard_drag_action_get_motion_delta(inDragAction, &deltaX, &deltaY);

		stage=CLUTTER_STAGE(clutter_actor_get_stage(dragHandle));
		actorUnderMouse=clutter_stage_get_actor_at_pos(stage, CLUTTER_PICK_REACTIVE, stageX, stageY);
		if(XFDASHBOARD_IS_APPLICATION_BUTTON(actorUnderMouse) &&
			actorUnderMouse!=priv->dragPreviewIcon)
		{
			if((priv->orientation==CLUTTER_ORIENTATION_HORIZONTAL && deltaX<0) ||
				(priv->orientation==CLUTTER_ORIENTATION_VERTICAL && deltaY<0))
			{
				clutter_actor_set_child_below_sibling(CLUTTER_ACTOR(self),
														priv->dragPreviewIcon,
														actorUnderMouse);
			}
				else
				{
					clutter_actor_set_child_above_sibling(CLUTTER_ACTOR(self),
															priv->dragPreviewIcon,
															actorUnderMouse);
				}

			/* Show preview icon now if drag mode is "new". Doing it earlier will
			 * show preview icon at wrong position when entering quicklaunch
			 */
			if(priv->dragMode==DRAG_MODE_CREATE) clutter_actor_show(priv->dragPreviewIcon);
		}

		/* Reset reactive state of preview icon and drag handle */
		clutter_actor_set_reactive(priv->dragPreviewIcon, oldPreviewReactive);
		clutter_actor_set_reactive(dragHandle, oldHandleReactive);
	}
}

/* Drag of an actor entered this drop target */
static void _xfdashboard_quicklaunch_on_drop_leave(XfdashboardQuicklaunch *self,
													XfdashboardDragAction *inDragAction,
													gpointer inUserData)
{
	XfdashboardQuicklaunchPrivate	*priv;
	ClutterActor					*dragHandle;

	g_return_if_fail(XFDASHBOARD_IS_QUICKLAUNCH(self));
	g_return_if_fail(XFDASHBOARD_IS_DRAG_ACTION(inDragAction));
	g_return_if_fail(XFDASHBOARD_IS_DROP_ACTION(inUserData));

	priv=self->priv;

	/* Get source where dragging started and drag handle */
	dragHandle=clutter_drag_action_get_drag_handle(CLUTTER_DRAG_ACTION(inDragAction));

	/* Show drag handle outside this quicklaunch */
	clutter_actor_show(dragHandle);

	/* Hide preview icon if drag mode is "create new" favourite */
	if(priv->dragMode==DRAG_MODE_CREATE) clutter_actor_hide(priv->dragPreviewIcon);
}

/* Drag of an actor to trash drop target begins */
static gboolean _xfdashboard_quicklaunch_on_trash_drop_begin(XfdashboardQuicklaunch *self,
																XfdashboardDragAction *inDragAction,
																gpointer inUserData)
{
	XfdashboardQuicklaunchPrivate	*priv;
	ClutterActor					*dragSource;
	ClutterActor					*draggedActor;

	g_return_val_if_fail(XFDASHBOARD_IS_QUICKLAUNCH(self), FALSE);
	g_return_val_if_fail(XFDASHBOARD_IS_DRAG_ACTION(inDragAction), FALSE);
	g_return_val_if_fail(XFDASHBOARD_IS_DROP_ACTION(inUserData), FALSE);

	priv=self->priv;

	/* Get source where dragging started and actor being dragged */
	dragSource=xfdashboard_drag_action_get_source(inDragAction);
	draggedActor=xfdashboard_drag_action_get_actor(inDragAction);

	/* Check if we can handle dragged actor from given source */
	if(XFDASHBOARD_IS_QUICKLAUNCH(dragSource) &&
		XFDASHBOARD_IS_APPLICATION_BUTTON(draggedActor))
	{
		/* Dragged actor is a favourite icon from quicklaunch. So hide
		 * "applications" button and show an unhighlited trash button instead.
		 * Trash button will be hidden and "applications" button shown again
		 * when drag ends.
		 */
		clutter_actor_hide(priv->appsButton);
		clutter_actor_show(priv->trashButton);

		return(TRUE);
	}

	/* If we get here we cannot handle dragged actor at trash drop target */
	return(FALSE);
}

/* Dragged actor was dropped on trash drop target */
static void _xfdashboard_quicklaunch_on_trash_drop_drop(XfdashboardQuicklaunch *self,
															XfdashboardDragAction *inDragAction,
															gfloat inX,
															gfloat inY,
															gpointer inUserData)
{
	XfdashboardQuicklaunchPrivate	*priv;
	ClutterActor					*draggedActor;
	GAppInfo						*appInfo;

	g_return_if_fail(XFDASHBOARD_IS_QUICKLAUNCH(self));
	g_return_if_fail(XFDASHBOARD_IS_DRAG_ACTION(inDragAction));
	g_return_if_fail(XFDASHBOARD_IS_DROP_ACTION(inUserData));

	priv=self->priv;

	/* Get dragged favourite icon */
	draggedActor=xfdashboard_drag_action_get_actor(inDragAction);

	/* Emit signal */
	appInfo=xfdashboard_application_button_get_app_info(XFDASHBOARD_APPLICATION_BUTTON(draggedActor));
	if(appInfo)
	{
		xfdashboard_notify(CLUTTER_ACTOR(self),
							xfdashboard_application_button_get_icon_name(XFDASHBOARD_APPLICATION_BUTTON(draggedActor)),
							_("Favourite '%s' removed"),
							xfdashboard_application_button_get_display_name(XFDASHBOARD_APPLICATION_BUTTON(draggedActor)));
		g_signal_emit(self, XfdashboardQuicklaunchSignals[SIGNAL_FAVOURITE_REMOVED], 0, appInfo);
		g_object_unref(appInfo);
	}

	/* Destroy dragged favourite icon before updating property */
	clutter_actor_destroy(draggedActor);

	/* Destroy preview icon before updating property */
	if(priv->dragPreviewIcon)
	{
		clutter_actor_destroy(priv->dragPreviewIcon);
		priv->dragPreviewIcon=NULL;
	}

	/* Show "applications" button again and hide trash button instead */
	clutter_actor_hide(priv->trashButton);
	clutter_actor_show(priv->appsButton);

	/* Update favourites from icon order */
	_xfdashboard_quicklaunch_update_property_from_icons(self);

	/* Reset drag mode */
	priv->dragMode=DRAG_MODE_NONE;

}

/* Drag of an actor to trash drop target ended without actor being dropped here */
static void _xfdashboard_quicklaunch_on_trash_drop_end(XfdashboardQuicklaunch *self,
														XfdashboardDragAction *inDragAction,
														gpointer inUserData)
{
	XfdashboardQuicklaunchPrivate	*priv;

	g_return_if_fail(XFDASHBOARD_IS_QUICKLAUNCH(self));
	g_return_if_fail(XFDASHBOARD_IS_DRAG_ACTION(inDragAction));
	g_return_if_fail(XFDASHBOARD_IS_DROP_ACTION(inUserData));

	priv=self->priv;

	/* Show "applications" button again and hide trash button instead */
	clutter_actor_hide(priv->trashButton);
	clutter_actor_show(priv->appsButton);
}

/* Drag of an actor entered trash drop target */
static void _xfdashboard_quicklaunch_on_trash_drop_enter(XfdashboardQuicklaunch *self,
															XfdashboardDragAction *inDragAction,
															gpointer inUserData)
{
	XfdashboardQuicklaunchPrivate	*priv;

	g_return_if_fail(XFDASHBOARD_IS_QUICKLAUNCH(self));
	g_return_if_fail(XFDASHBOARD_IS_DRAG_ACTION(inDragAction));
	g_return_if_fail(XFDASHBOARD_IS_DROP_ACTION(inUserData));

	priv=self->priv;

	/* Set toggle state on trash drop target as dragged actor is over it */
	xfdashboard_toggle_button_set_toggle_state(XFDASHBOARD_TOGGLE_BUTTON(priv->trashButton), TRUE);
}

/* Drag of an actor leaves trash drop target */
static void _xfdashboard_quicklaunch_on_trash_drop_leave(XfdashboardQuicklaunch *self,
															XfdashboardDragAction *inDragAction,
															gpointer inUserData)
{
	XfdashboardQuicklaunchPrivate	*priv;

	g_return_if_fail(XFDASHBOARD_IS_QUICKLAUNCH(self));
	g_return_if_fail(XFDASHBOARD_IS_DRAG_ACTION(inDragAction));
	g_return_if_fail(XFDASHBOARD_IS_DROP_ACTION(inUserData));

	priv=self->priv;

	/* Set toggle state on trash drop target as dragged actor is over it */
	xfdashboard_toggle_button_set_toggle_state(XFDASHBOARD_TOGGLE_BUTTON(priv->trashButton), FALSE);
}

/* Update property from icons in quicklaunch */
static void _xfdashboard_quicklaunch_update_property_from_icons(XfdashboardQuicklaunch *self)
{
	XfdashboardQuicklaunchPrivate	*priv;
	ClutterActor					*child;
	ClutterActorIter				iter;
	XfdashboardApplicationButton	*button;
	const gchar						*desktopFile;
	GValue							*desktopValue;

	g_return_if_fail(XFDASHBOARD_IS_QUICKLAUNCH(self));

	priv=self->priv;

	/* Free current list of desktop files */
	if(priv->favourites)
	{
		xfconf_array_free(priv->favourites);
		priv->favourites=NULL;
	}

	/* Create array of strings pointing to desktop files for new order,
	 * update quicklaunch and store settings
	 */
	priv->favourites=g_ptr_array_new();

	clutter_actor_iter_init(&iter, CLUTTER_ACTOR(self));
	while(clutter_actor_iter_next(&iter, &child))
	{
		/* Only add desktop file if it is an application button and
		 * provides a desktop file name
		 */
 		if(!XFDASHBOARD_IS_APPLICATION_BUTTON(child)) continue;

		button=XFDASHBOARD_APPLICATION_BUTTON(child);
		desktopFile=xfdashboard_application_button_get_desktop_filename(button);
		if(!desktopFile) continue;

		/* Add desktop file name to array */
		desktopValue=g_value_init(g_new0(GValue, 1), G_TYPE_STRING);
		g_value_set_string(desktopValue, desktopFile);
		g_ptr_array_add(priv->favourites, desktopValue);
	}

	/* Notify about property change */
	g_object_notify_by_pspec(G_OBJECT(self), XfdashboardQuicklaunchProperties[PROP_FAVOURITES]);
}

/* Update icons in quicklaunch from property */
static void _xfdashboard_quicklaunch_update_icons_from_property(XfdashboardQuicklaunch *self)
{
	XfdashboardQuicklaunchPrivate	*priv;
	ClutterActor					*child;
	ClutterActorIter				iter;
	guint							i;
	ClutterActor					*actor;
	GValue							*desktopFile;
	ClutterAction					*dragAction;

	g_return_if_fail(XFDASHBOARD_IS_QUICKLAUNCH(self));

	priv=self->priv;

	/* Remove all application buttons */
	clutter_actor_iter_init(&iter, CLUTTER_ACTOR(self));
	while(clutter_actor_iter_next(&iter, &child))
	{
		if(XFDASHBOARD_IS_APPLICATION_BUTTON(child)) clutter_actor_iter_destroy(&iter);
	}

	/* Now re-add all application icons for current favourites */
	for(i=0; i<priv->favourites->len; i++)
	{
		/* Create application button from desktop file and hide label in quicklaunch */
		desktopFile=(GValue*)g_ptr_array_index(priv->favourites, i);

		actor=xfdashboard_application_button_new_from_desktop_file(g_value_get_string(desktopFile));
		xfdashboard_button_set_icon_size(XFDASHBOARD_BUTTON(actor), priv->normalIconSize);
		xfdashboard_button_set_sync_icon_size(XFDASHBOARD_BUTTON(actor), FALSE);
		xfdashboard_button_set_style(XFDASHBOARD_BUTTON(actor), XFDASHBOARD_STYLE_ICON);
		clutter_actor_show(actor);
		clutter_actor_add_child(CLUTTER_ACTOR(self), actor);
		g_signal_connect_swapped(actor, "clicked", G_CALLBACK(_xfdashboard_quicklaunch_on_favourite_clicked), self);

		/* Set up drag'n'drop */
		dragAction=xfdashboard_drag_action_new_with_source(CLUTTER_ACTOR(self));
		clutter_drag_action_set_drag_threshold(CLUTTER_DRAG_ACTION(dragAction), -1, -1);
		clutter_actor_add_action(actor, dragAction);
		g_signal_connect(dragAction, "drag-begin", G_CALLBACK(_xfdashboard_quicklaunch_on_favourite_drag_begin), self);
		g_signal_connect(dragAction, "drag-end", G_CALLBACK(_xfdashboard_quicklaunch_on_favourite_drag_end), self);
	}
}

/* Set up favourites array from string array value */
static void _xfdashboard_quicklaunch_set_favourites(XfdashboardQuicklaunch *self, const GValue *inValue)
{
	XfdashboardQuicklaunchPrivate	*priv;
	GPtrArray						*desktopFiles;
	guint							i;
	GValue							*element;
	GValue							*desktopFile;

	g_return_if_fail(XFDASHBOARD_IS_QUICKLAUNCH(self));
	g_return_if_fail(G_IS_VALUE(inValue));

	priv=self->priv;

	/* Free current list of favourites */
	if(priv->favourites)
	{
		xfconf_array_free(priv->favourites);
		priv->favourites=NULL;
	}

	/* Copy array of string pointing to desktop files */
	desktopFiles=g_value_get_boxed(inValue);
	if(desktopFiles)
	{
		priv->favourites=g_ptr_array_sized_new(desktopFiles->len);
		for(i=0; i<desktopFiles->len; ++i)
		{
			element=(GValue*)g_ptr_array_index(desktopFiles, i);

			/* Filter string value types */
			if(G_VALUE_HOLDS_STRING(element))
			{
				desktopFile=g_value_init(g_new0(GValue, 1), G_TYPE_STRING);
				g_value_copy(element, desktopFile);
				g_ptr_array_add(priv->favourites, desktopFile);
			}
		}
	}
		else priv->favourites=g_ptr_array_new();

	/* Update list of icons for desktop files */
	_xfdashboard_quicklaunch_update_icons_from_property(self);
}

/* Get scale factor to fit all children into given width */
static gfloat _xfdashboard_quicklaunch_get_scale_for_width(XfdashboardQuicklaunch *self,
															gfloat inForWidth,
															gboolean inDoMinimumSize)
{
	XfdashboardQuicklaunchPrivate	*priv;
	ClutterActor					*child;
	ClutterActorIter				iter;
	gint							numberChildren;
	gfloat							totalWidth, scalableWidth;
	gfloat							childWidth;
	gfloat							childMinWidth, childNaturalWidth;
	gfloat							scale;
	gboolean						recheckWidth;

	g_return_val_if_fail(XFDASHBOARD_IS_QUICKLAUNCH(self), 0.0f);
	g_return_val_if_fail(inForWidth>=0.0f, 0.0f);

	priv=self->priv;

	/* Count visible children and determine their total width */
	numberChildren=0;
	totalWidth=0.0f;
	clutter_actor_iter_init(&iter, CLUTTER_ACTOR(self));
	while(clutter_actor_iter_next(&iter, &child))
	{
		/* Only check visible children */
		if(!CLUTTER_ACTOR_IS_VISIBLE(child)) continue;

		/* Get width of child */
		clutter_actor_get_preferred_width(child, -1, &childMinWidth, &childNaturalWidth);
		if(inDoMinimumSize==TRUE) childWidth=childMinWidth;
			else childWidth=childNaturalWidth;

		/* Determine total size so far */
		totalWidth+=ceil(childWidth);

		/* Count visible children */
		numberChildren++;
	}
	if(numberChildren==0) return(priv->scaleMax);

	/* Determine scalable width. That is the width without spacing
	 * between children and the spacing used as padding.
	 */
	scalableWidth=inForWidth-((numberChildren+1)*priv->spacing);

	/* Get scale factor */
	scale=priv->scaleMax;
	if(totalWidth>0.0f)
	{
		scale=floor((scalableWidth/totalWidth)/priv->scaleStep)*priv->scaleStep;
		scale=MIN(scale, priv->scaleMax);
		scale=MAX(scale, priv->scaleMin);
	}

	/* Check if all visible children would really fit into width
	 * otherwise we need to decrease scale factor one step down
	 */
	if(scale>priv->scaleMin)
	{
		do
		{
			recheckWidth=FALSE;
			totalWidth=priv->spacing;

			/* Iterate through visible children and sum their scaled
			 * widths. The total width will be initialized with unscaled
			 * spacing and all visible children's scaled width will also
			 * be added with unscaled spacing to have the padding added.
			 */
			clutter_actor_iter_init(&iter, CLUTTER_ACTOR(self));
			while(clutter_actor_iter_next(&iter, &child))
			{
				/* Only check visible children */
				if(!CLUTTER_ACTOR_IS_VISIBLE(child)) continue;

				/* Get scaled size of child and add to total width */
				clutter_actor_get_preferred_width(child, -1, &childMinWidth, &childNaturalWidth);
				if(inDoMinimumSize==TRUE) childWidth=childMinWidth;
					else childWidth=childNaturalWidth;

				childWidth*=scale;
				totalWidth+=ceil(childWidth)+priv->spacing;
			}

			/* If total width is greater than given width
			 * decrease scale factor by one step and recheck
			 */
			if(totalWidth>inForWidth && scale>priv->scaleMin)
			{
				scale-=priv->scaleStep;
				recheckWidth=TRUE;
			}
		}
		while(recheckWidth==TRUE);
	}

	/* Return found scale factor */
	return(scale);
}

/* Get scale factor to fit all children into given height */
static gfloat _xfdashboard_quicklaunch_get_scale_for_height(XfdashboardQuicklaunch *self,
															gfloat inForHeight,
															gboolean inDoMinimumSize)
{
	XfdashboardQuicklaunchPrivate	*priv;
	ClutterActor					*child;
	ClutterActorIter				iter;
	gint							numberChildren;
	gfloat							totalHeight, scalableHeight;
	gfloat							childHeight;
	gfloat							childMinHeight, childNaturalHeight;
	gfloat							scale;
	gboolean						recheckHeight;

	g_return_val_if_fail(XFDASHBOARD_IS_QUICKLAUNCH(self), 0.0f);
	g_return_val_if_fail(inForHeight>=0.0f, 0.0f);

	priv=self->priv;

	/* Count visible children and determine their total height */
	numberChildren=0;
	totalHeight=0.0f;
	clutter_actor_iter_init(&iter, CLUTTER_ACTOR(self));
	while(clutter_actor_iter_next(&iter, &child))
	{
		/* Only check visible children */
		if(!CLUTTER_ACTOR_IS_VISIBLE(child)) continue;

		/* Get height of child */
		clutter_actor_get_preferred_height(child, -1, &childMinHeight, &childNaturalHeight);
		if(inDoMinimumSize==TRUE) childHeight=childMinHeight;
			else childHeight=childNaturalHeight;

		/* Determine total size so far */
		totalHeight+=ceil(childHeight);

		/* Count visible children */
		numberChildren++;
	}
	if(numberChildren==0) return(priv->scaleMax);

	/* Determine scalable height. That is the height without spacing
	 * between children and the spacing used as padding.
	 */
	scalableHeight=inForHeight-((numberChildren+1)*priv->spacing);

	/* Get scale factor */
	scale=priv->scaleMax;
	if(totalHeight>0.0f)
	{
		scale=floor((scalableHeight/totalHeight)/priv->scaleStep)*priv->scaleStep;
		scale=MIN(scale, priv->scaleMax);
		scale=MAX(scale, priv->scaleMin);
	}

	/* Check if all visible children would really fit into height
	 * otherwise we need to decrease scale factor one step down
	 */
	if(scale>priv->scaleMin)
	{
		do
		{
			recheckHeight=FALSE;
			totalHeight=priv->spacing;

			/* Iterate through visible children and sum their scaled
			 * heights. The total height will be initialized with unscaled
			 * spacing and all visible children's scaled height will also
			 * be added with unscaled spacing to have the padding added.
			 */
			clutter_actor_iter_init(&iter, CLUTTER_ACTOR(self));
			while(clutter_actor_iter_next(&iter, &child))
			{
				/* Only check visible children */
				if(!CLUTTER_ACTOR_IS_VISIBLE(child)) continue;

				/* Get scaled size of child and add to total height */
				clutter_actor_get_preferred_height(child, -1, &childMinHeight, &childNaturalHeight);
				if(inDoMinimumSize==TRUE) childHeight=childMinHeight;
					else childHeight=childNaturalHeight;

				childHeight*=scale;
				totalHeight+=ceil(childHeight)+priv->spacing;
			}

			/* If total height is greater than given height
			 * decrease scale factor by one step and recheck
			 */
			if(totalHeight>inForHeight && scale>priv->scaleMin)
			{
				scale-=priv->scaleStep;
				recheckHeight=TRUE;
			}
		}
		while(recheckHeight==TRUE);
	}

	/* Return found scale factor */
	return(scale);
}

/* IMPLEMENTATION: ClutterActor */

/* Get preferred width/height */
static void _xfdashboard_quicklaunch_get_preferred_height(ClutterActor *inActor,
															gfloat inForWidth,
															gfloat *outMinHeight,
															gfloat *outNaturalHeight)
{
	XfdashboardQuicklaunch			*self=XFDASHBOARD_QUICKLAUNCH(inActor);
	XfdashboardQuicklaunchPrivate	*priv=self->priv;
	gfloat							minHeight, naturalHeight;
	ClutterActor					*child;
	ClutterActorIter				iter;
	gfloat							childMinHeight, childNaturalHeight;
	gint							numberChildren;
	gfloat							scale;

	/* Set up default values */
	minHeight=naturalHeight=0.0f;

	/* Determine height for horizontal orientation ... */
	if(priv->orientation==CLUTTER_ORIENTATION_HORIZONTAL)
	{
		/* Iterate through visible children and determine heights */
		numberChildren=0;
		clutter_actor_iter_init(&iter, CLUTTER_ACTOR(inActor));
		while(clutter_actor_iter_next(&iter, &child))
		{
			/* Only check visible children */
			if(!CLUTTER_ACTOR_IS_VISIBLE(child)) continue;

			/* Get sizes of child */
			clutter_actor_get_preferred_height(child,
												-1,
												&childMinHeight,
												&childNaturalHeight);

			/* Determine heights */
			minHeight=MAX(minHeight, childMinHeight);
			naturalHeight=MAX(naturalHeight, childNaturalHeight);

			/* Count visible children */
			numberChildren++;
		}

		/* Check if we need to scale width because of the need to fit
		 * all visible children into given limiting width
		 */
		if(inForWidth>=0.0f)
		{
			scale=_xfdashboard_quicklaunch_get_scale_for_width(self, inForWidth, TRUE);
			minHeight*=scale;

			scale=_xfdashboard_quicklaunch_get_scale_for_width(self, inForWidth, FALSE);
			naturalHeight*=scale;
		}

		/* Add spacing as padding */
		if(numberChildren>0)
		{
			minHeight+=2*priv->spacing;
			naturalHeight+=2*priv->spacing;
		}
	}
		/* ... otherwise determine height for vertical orientation */
		else
		{
			/* Iterate through visible children and determine height */
			numberChildren=0;
			clutter_actor_iter_init(&iter, CLUTTER_ACTOR(inActor));
			while(clutter_actor_iter_next(&iter, &child))
			{
				/* Only check visible children */
				if(!CLUTTER_ACTOR_IS_VISIBLE(child)) continue;

				/* Get child's size */
				clutter_actor_get_preferred_height(child,
													inForWidth,
													&childMinHeight,
													&childNaturalHeight);

				/* Determine heights */
				minHeight+=childMinHeight;
				naturalHeight+=childNaturalHeight;

				/* Count visible children */
				numberChildren++;
			}

			/* Add spacing between children and spacing as padding */
			if(numberChildren>0)
			{
				minHeight+=(numberChildren+1)*priv->spacing;
				naturalHeight+=(numberChildren+1)*priv->spacing;
			}
		}

	/* Store sizes computed */
	if(outMinHeight) *outMinHeight=minHeight;
	if(outNaturalHeight) *outNaturalHeight=naturalHeight;
}

static void _xfdashboard_quicklaunch_get_preferred_width(ClutterActor *inActor,
															gfloat inForHeight,
															gfloat *outMinWidth,
															gfloat *outNaturalWidth)
{
	XfdashboardQuicklaunch			*self=XFDASHBOARD_QUICKLAUNCH(inActor);
	XfdashboardQuicklaunchPrivate	*priv=self->priv;
	gfloat							minWidth, naturalWidth;
	ClutterActor					*child;
	ClutterActorIter				iter;
	gfloat							childMinWidth, childNaturalWidth;
	gint							numberChildren;
	gfloat							scale;

	/* Set up default values */
	minWidth=naturalWidth=0.0f;

	/* Determine width for horizontal orientation ... */
	if(priv->orientation==CLUTTER_ORIENTATION_HORIZONTAL)
	{
		/* Iterate through visible children and determine width */
		numberChildren=0;
		clutter_actor_iter_init(&iter, CLUTTER_ACTOR(inActor));
		while(clutter_actor_iter_next(&iter, &child))
		{
			/* Only check visible children */
			if(!CLUTTER_ACTOR_IS_VISIBLE(child)) continue;

			/* Get child's size */
			clutter_actor_get_preferred_width(child,
												inForHeight,
												&childMinWidth,
												&childNaturalWidth);

			/* Determine widths */
			minWidth+=childMinWidth;
			naturalWidth+=childNaturalWidth;

			/* Count visible children */
			numberChildren++;
		}

		/* Add spacing between children and spacing as padding */
		if(numberChildren>0)
		{
			minWidth+=(numberChildren+1)*priv->spacing;
			naturalWidth+=(numberChildren+1)*priv->spacing;
		}
	}
		/* ... otherwise determine width for vertical orientation */
		else
		{
			/* Iterate through visible children and determine widths */
			numberChildren=0;
			clutter_actor_iter_init(&iter, CLUTTER_ACTOR(inActor));
			while(clutter_actor_iter_next(&iter, &child))
			{
				/* Only check visible children */
				if(!CLUTTER_ACTOR_IS_VISIBLE(child)) continue;

				/* Get sizes of child */
				clutter_actor_get_preferred_width(child,
													-1,
													&childMinWidth,
													&childNaturalWidth);

				/* Determine widths */
				minWidth=MAX(minWidth, childMinWidth);
				naturalWidth=MAX(naturalWidth, childNaturalWidth);

				/* Count visible children */
				numberChildren++;
			}

			/* Check if we need to scale width because of the need to fit
			 * all visible children into given limiting height
			 */
			if(inForHeight>=0.0f)
			{
				scale=_xfdashboard_quicklaunch_get_scale_for_height(self, inForHeight, TRUE);
				minWidth*=scale;

				scale=_xfdashboard_quicklaunch_get_scale_for_height(self, inForHeight, FALSE);
				naturalWidth*=scale;
			}

			/* Add spacing as padding */
			if(numberChildren>0)
			{
				minWidth+=2*priv->spacing;
				naturalWidth+=2*priv->spacing;
			}
		}

	/* Store sizes computed */
	if(outMinWidth) *outMinWidth=minWidth;
	if(outNaturalWidth) *outNaturalWidth=naturalWidth;
}

/* Allocate position and size of actor and its children */
static void _xfdashboard_quicklaunch_allocate(ClutterActor *inActor,
												const ClutterActorBox *inBox,
												ClutterAllocationFlags inFlags)
{
	XfdashboardQuicklaunch			*self=XFDASHBOARD_QUICKLAUNCH(inActor);
	XfdashboardQuicklaunchPrivate	*priv=self->priv;
	gfloat							availableWidth, availableHeight;
	gfloat							childWidth, childHeight;
	ClutterActor					*child;
	ClutterActorIter				iter;
	ClutterActorBox					childAllocation={ 0, };

	/* Chain up to store the allocation of the actor */
	CLUTTER_ACTOR_CLASS(xfdashboard_quicklaunch_parent_class)->allocate(inActor, inBox, inFlags);

	/* Get available size */
	clutter_actor_box_get_size(inBox, &availableWidth, &availableHeight);

	/* Find scaling to get all children fit the allocation */
	priv->scaleCurrent=_xfdashboard_quicklaunch_get_scale_for_height(self, availableHeight, FALSE);

	/* Calculate new position and size of visible children */
	childAllocation.x1=childAllocation.y1=priv->spacing;
	clutter_actor_iter_init(&iter, CLUTTER_ACTOR(inActor));
	while(clutter_actor_iter_next(&iter, &child))
	{
		/* Is child visible? */
		if(!CLUTTER_ACTOR_IS_VISIBLE(child)) continue;

		/* Calculate new position and size of child. Because we will set
		 * a scale factor to the actor we have to set the real unscaled
		 * width and height but the position should be "translated" to
		 * scaled sizes.
		 */
		clutter_actor_get_preferred_size(child, NULL, NULL, &childWidth, &childHeight);

		if(priv->orientation==CLUTTER_ORIENTATION_HORIZONTAL)
		{
			childAllocation.y1=ceil(MAX(((availableHeight-(childHeight*priv->scaleCurrent))/2.0f), priv->spacing));
			childAllocation.y2=ceil(childAllocation.y1+childHeight);
			childAllocation.x2=ceil(childAllocation.x1+childWidth);
		}
			else
			{
				childAllocation.x1=ceil(MAX(((availableWidth-(childWidth*priv->scaleCurrent))/2.0f), priv->spacing));
				childAllocation.x2=ceil(childAllocation.x1+childWidth);
				childAllocation.y2=ceil(childAllocation.y1+childHeight);
			}

		clutter_actor_set_scale(child, priv->scaleCurrent, priv->scaleCurrent);
		clutter_actor_allocate(child, &childAllocation, inFlags);

		/* Set up for next child */
		childWidth*=priv->scaleCurrent;
		childHeight*=priv->scaleCurrent;
		if(priv->orientation==CLUTTER_ORIENTATION_HORIZONTAL) childAllocation.x1=ceil(childAllocation.x1+childWidth+priv->spacing);
			else childAllocation.y1=ceil(childAllocation.y1+childHeight+priv->spacing);
	}
}

/* IMPLEMENTATION: GObject */

/* Dispose this object */
static void _xfdashboard_quicklaunch_dispose(GObject *inObject)
{
	XfdashboardQuicklaunchPrivate	*priv=XFDASHBOARD_QUICKLAUNCH(inObject)->priv;

	/* Release our allocated variables */
	priv->xfconfChannel=NULL;

	/* Call parent's class dispose method */
	G_OBJECT_CLASS(xfdashboard_quicklaunch_parent_class)->dispose(inObject);
}

/* Set/get properties */
static void _xfdashboard_quicklaunch_set_property(GObject *inObject,
													guint inPropID,
													const GValue *inValue,
													GParamSpec *inSpec)
{
	XfdashboardQuicklaunch			*self=XFDASHBOARD_QUICKLAUNCH(inObject);

	switch(inPropID)
	{
		case PROP_FAVOURITES:
			_xfdashboard_quicklaunch_set_favourites(self, inValue);
			break;

		case PROP_NORMAL_ICON_SIZE:
			xfdashboard_quicklaunch_set_normal_icon_size(self, g_value_get_float(inValue));
			break;

		case PROP_SPACING:
			xfdashboard_quicklaunch_set_spacing(self, g_value_get_float(inValue));
			break;

		case PROP_ORIENTATION:
			xfdashboard_quicklaunch_set_orientation(self, g_value_get_enum(inValue));
			break;

		default:
			G_OBJECT_WARN_INVALID_PROPERTY_ID(inObject, inPropID, inSpec);
			break;
	}
}

static void _xfdashboard_quicklaunch_get_property(GObject *inObject,
													guint inPropID,
													GValue *outValue,
													GParamSpec *inSpec)
{
	XfdashboardQuicklaunch			*self=XFDASHBOARD_QUICKLAUNCH(inObject);
	XfdashboardQuicklaunchPrivate	*priv=self->priv;

	switch(inPropID)
	{
		case PROP_FAVOURITES:
			g_value_set_boxed(outValue, priv->favourites);
			break;

		case PROP_NORMAL_ICON_SIZE:
			g_value_set_float(outValue, priv->normalIconSize);
			break;

		case PROP_SPACING:
			g_value_set_float(outValue, priv->spacing);
			break;

		case PROP_ORIENTATION:
			g_value_set_enum(outValue, priv->orientation);
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
static void xfdashboard_quicklaunch_class_init(XfdashboardQuicklaunchClass *klass)
{
	ClutterActorClass	*actorClass=CLUTTER_ACTOR_CLASS(klass);
	GObjectClass		*gobjectClass=G_OBJECT_CLASS(klass);

	/* Override functions */
	gobjectClass->dispose=_xfdashboard_quicklaunch_dispose;
	gobjectClass->set_property=_xfdashboard_quicklaunch_set_property;
	gobjectClass->get_property=_xfdashboard_quicklaunch_get_property;

	actorClass->get_preferred_width=_xfdashboard_quicklaunch_get_preferred_width;
	actorClass->get_preferred_height=_xfdashboard_quicklaunch_get_preferred_height;
	actorClass->allocate=_xfdashboard_quicklaunch_allocate;

	/* Set up private structure */
	g_type_class_add_private(klass, sizeof(XfdashboardQuicklaunchPrivate));

	/* Define properties */
	XfdashboardQuicklaunchProperties[PROP_FAVOURITES]=
		g_param_spec_boxed("favourites",
							_("Favourites"),
							_("An array of strings pointing to desktop files shown as icons"),
							XFDASHBOARD_TYPE_POINTER_ARRAY,
							G_PARAM_READWRITE);

	XfdashboardQuicklaunchProperties[PROP_NORMAL_ICON_SIZE]=
		g_param_spec_float("normal-icon-size",
								_("Normal icon size"),
								_("Unscale size of icon"),
								0.0, G_MAXFLOAT,
								DEFAULT_NORMAL_ICON_SIZE,
								G_PARAM_READWRITE);

	XfdashboardQuicklaunchProperties[PROP_SPACING]=
		g_param_spec_float("spacing",
								_("Spacing"),
								_("The spacing between children"),
								0.0, G_MAXFLOAT,
								0.0,
								G_PARAM_READWRITE);

	XfdashboardQuicklaunchProperties[PROP_ORIENTATION]=
		g_param_spec_enum("orientation",
							_("Orientation"),
							_("The orientation to layout children"),
							CLUTTER_TYPE_ORIENTATION,
							DEFAULT_ORIENTATION,
							G_PARAM_READWRITE);

	g_object_class_install_properties(gobjectClass, PROP_LAST, XfdashboardQuicklaunchProperties);

	/* Define signals */
	XfdashboardQuicklaunchSignals[SIGNAL_FAVOURITE_ADDED]=
		g_signal_new("favourite-added",
						G_TYPE_FROM_CLASS(klass),
						G_SIGNAL_RUN_LAST,
						G_STRUCT_OFFSET(XfdashboardQuicklaunchClass, favourite_added),
						NULL,
						NULL,
						g_cclosure_marshal_VOID__OBJECT,
						G_TYPE_NONE,
						1,
						G_TYPE_APP_INFO);

	XfdashboardQuicklaunchSignals[SIGNAL_FAVOURITE_REMOVED]=
		g_signal_new("favourite-removed",
						G_TYPE_FROM_CLASS(klass),
						G_SIGNAL_RUN_LAST,
						G_STRUCT_OFFSET(XfdashboardQuicklaunchClass, favourite_removed),
						NULL,
						NULL,
						g_cclosure_marshal_VOID__OBJECT,
						G_TYPE_NONE,
						1,
						G_TYPE_APP_INFO);
}

/* Object initialization
 * Create private structure and set up default values
 */
static void xfdashboard_quicklaunch_init(XfdashboardQuicklaunch *self)
{
	XfdashboardQuicklaunchPrivate	*priv;
	ClutterRequestMode				requestMode;
	ClutterAction					*dropAction;

	priv=self->priv=XFDASHBOARD_QUICKLAUNCH_GET_PRIVATE(self);

	/* Set up default values */
	priv->favourites=NULL;
	priv->spacing=0.0f;
	priv->orientation=DEFAULT_ORIENTATION;
	priv->normalIconSize=DEFAULT_NORMAL_ICON_SIZE;
	priv->scaleCurrent=DEFAULT_SCALE_MAX;
	priv->scaleMin=DEFAULT_SCALE_MIN;
	priv->scaleMax=DEFAULT_SCALE_MAX;
	priv->scaleStep=DEFAULT_SCALE_STEP;
	priv->xfconfChannel=XFCONF_CHANNEL(g_object_ref(xfdashboard_application_get_xfconf_channel()));
	priv->dragMode=DRAG_MODE_NONE;
	priv->dragPreviewIcon=NULL;

	/* Set up this actor */
	clutter_actor_set_reactive(CLUTTER_ACTOR(self), TRUE);
	requestMode=(priv->orientation==CLUTTER_ORIENTATION_HORIZONTAL ? CLUTTER_REQUEST_HEIGHT_FOR_WIDTH : CLUTTER_REQUEST_WIDTH_FOR_HEIGHT);
	clutter_actor_set_request_mode(CLUTTER_ACTOR(self), requestMode);

	dropAction=xfdashboard_drop_action_new();
	clutter_actor_add_action(CLUTTER_ACTOR(self), dropAction);
	g_signal_connect_swapped(dropAction, "begin", G_CALLBACK(_xfdashboard_quicklaunch_on_drop_begin), self);
	g_signal_connect_swapped(dropAction, "drop", G_CALLBACK(_xfdashboard_quicklaunch_on_drop_drop), self);
	g_signal_connect_swapped(dropAction, "end", G_CALLBACK(_xfdashboard_quicklaunch_on_drop_end), self);
	g_signal_connect_swapped(dropAction, "drag-enter", G_CALLBACK(_xfdashboard_quicklaunch_on_drop_enter), self);
	g_signal_connect_swapped(dropAction, "drag-motion", G_CALLBACK(_xfdashboard_quicklaunch_on_drop_motion), self);
	g_signal_connect_swapped(dropAction, "drag-leave", G_CALLBACK(_xfdashboard_quicklaunch_on_drop_leave), self);

	/* Add "applications" button */
	priv->appsButton=xfdashboard_toggle_button_new_full(DEFAULT_APPS_BUTTON_ICON, _("Applications"));
	xfdashboard_button_set_icon_size(XFDASHBOARD_BUTTON(priv->appsButton), priv->normalIconSize);
	xfdashboard_button_set_sync_icon_size(XFDASHBOARD_BUTTON(priv->appsButton), FALSE);
	xfdashboard_button_set_style(XFDASHBOARD_BUTTON(priv->appsButton), XFDASHBOARD_STYLE_ICON);
	clutter_actor_add_child(CLUTTER_ACTOR(self), priv->appsButton);

	/* Next add trash button to box but initially hidden and register as drop target */
	priv->trashButton=xfdashboard_toggle_button_new_full(DEFAULT_TRASH_BUTTON_ICON, _("Remove"));
	clutter_actor_hide(priv->trashButton);
	xfdashboard_button_set_icon_size(XFDASHBOARD_BUTTON(priv->trashButton), priv->normalIconSize);
	xfdashboard_button_set_sync_icon_size(XFDASHBOARD_BUTTON(priv->trashButton), FALSE);
	xfdashboard_button_set_style(XFDASHBOARD_BUTTON(priv->trashButton), XFDASHBOARD_STYLE_ICON);
	clutter_actor_add_child(CLUTTER_ACTOR(self), priv->trashButton);

	dropAction=xfdashboard_drop_action_new();
	clutter_actor_add_action(priv->trashButton, CLUTTER_ACTION(dropAction));
	g_signal_connect_swapped(dropAction, "begin", G_CALLBACK(_xfdashboard_quicklaunch_on_trash_drop_begin), self);
	g_signal_connect_swapped(dropAction, "drop", G_CALLBACK(_xfdashboard_quicklaunch_on_trash_drop_drop), self);
	g_signal_connect_swapped(dropAction, "end", G_CALLBACK(_xfdashboard_quicklaunch_on_trash_drop_end), self);
	g_signal_connect_swapped(dropAction, "drag-enter", G_CALLBACK(_xfdashboard_quicklaunch_on_trash_drop_enter), self);
	g_signal_connect_swapped(dropAction, "drag-leave", G_CALLBACK(_xfdashboard_quicklaunch_on_trash_drop_leave), self);

	/* Bind to xfconf to react on changes */
	xfconf_g_property_bind(priv->xfconfChannel, "/favourites", XFDASHBOARD_TYPE_POINTER_ARRAY, self, "favourites");
}

/* IMPLEMENTATION: Public API */

/* Create new actor */
ClutterActor* xfdashboard_quicklaunch_new(void)
{
	return(g_object_new(XFDASHBOARD_TYPE_QUICKLAUNCH, NULL));
}

ClutterActor* xfdashboard_quicklaunch_new_with_orientation(ClutterOrientation inOrientation)
{
	g_return_val_if_fail(inOrientation==CLUTTER_ORIENTATION_HORIZONTAL || inOrientation==CLUTTER_ORIENTATION_VERTICAL, NULL);

	return(g_object_new(XFDASHBOARD_TYPE_QUICKLAUNCH,
						"orientation", inOrientation,
						NULL));
}

/* Get/set spacing between children */
gfloat xfdashboard_quicklaunch_get_normal_icon_size(XfdashboardQuicklaunch *self)
{
	g_return_val_if_fail(XFDASHBOARD_IS_QUICKLAUNCH(self), 0.0f);

	return(self->priv->normalIconSize);
}

void xfdashboard_quicklaunch_set_normal_icon_size(XfdashboardQuicklaunch *self, const gfloat inIconSize)
{
	XfdashboardQuicklaunchPrivate	*priv;

	g_return_if_fail(XFDASHBOARD_IS_QUICKLAUNCH(self));
	g_return_if_fail(inIconSize>=0.0f);

	priv=self->priv;

	/* Set value if changed */
	if(priv->normalIconSize!=inIconSize)
	{
		/* Set value */
		priv->normalIconSize=inIconSize;
		clutter_actor_queue_relayout(CLUTTER_ACTOR(self));

		/* Notify about property change */
		g_object_notify_by_pspec(G_OBJECT(self), XfdashboardQuicklaunchProperties[PROP_NORMAL_ICON_SIZE]);
	}
}

/* Get/set spacing between children */
gfloat xfdashboard_quicklaunch_get_spacing(XfdashboardQuicklaunch *self)
{
	g_return_val_if_fail(XFDASHBOARD_IS_QUICKLAUNCH(self), 0.0f);

	return(self->priv->spacing);
}

void xfdashboard_quicklaunch_set_spacing(XfdashboardQuicklaunch *self, const gfloat inSpacing)
{
	XfdashboardQuicklaunchPrivate	*priv;

	g_return_if_fail(XFDASHBOARD_IS_QUICKLAUNCH(self));
	g_return_if_fail(inSpacing>=0.0f);

	priv=self->priv;

	/* Set value if changed */
	if(priv->spacing!=inSpacing)
	{
		/* Set value */
		priv->spacing=inSpacing;
		clutter_actor_queue_relayout(CLUTTER_ACTOR(self));

		xfdashboard_background_set_corner_radius(XFDASHBOARD_BACKGROUND(self), priv->spacing);

		/* Notify about property change */
		g_object_notify_by_pspec(G_OBJECT(self), XfdashboardQuicklaunchProperties[PROP_SPACING]);
	}
}

/* Get/set orientation */
ClutterOrientation xfdashboard_quicklaunch_get_orientation(XfdashboardQuicklaunch *self)
{
	g_return_val_if_fail(XFDASHBOARD_IS_QUICKLAUNCH(self), DEFAULT_ORIENTATION);

	return(self->priv->orientation);
}

void xfdashboard_quicklaunch_set_orientation(XfdashboardQuicklaunch *self, ClutterOrientation inOrientation)
{
	XfdashboardQuicklaunchPrivate	*priv;
	ClutterRequestMode				requestMode;

	g_return_if_fail(XFDASHBOARD_IS_QUICKLAUNCH(self));
	g_return_if_fail(inOrientation==CLUTTER_ORIENTATION_HORIZONTAL ||
						inOrientation==CLUTTER_ORIENTATION_VERTICAL);

	priv=self->priv;

	/* Set value if changed */
	if(priv->orientation!=inOrientation)
	{
		/* Set value */
		priv->orientation=inOrientation;

		requestMode=(priv->orientation==CLUTTER_ORIENTATION_HORIZONTAL ? CLUTTER_REQUEST_HEIGHT_FOR_WIDTH : CLUTTER_REQUEST_WIDTH_FOR_HEIGHT);
		clutter_actor_set_request_mode(CLUTTER_ACTOR(self), requestMode);

		clutter_actor_queue_relayout(CLUTTER_ACTOR(self));

		/* Notify about property change */
		g_object_notify_by_pspec(G_OBJECT(self), XfdashboardQuicklaunchProperties[PROP_ORIENTATION]);
	}
}

/* Get apps button */
XfdashboardToggleButton* xfdashboard_quicklaunch_get_apps_button(XfdashboardQuicklaunch *self)
{
	g_return_val_if_fail(XFDASHBOARD_IS_QUICKLAUNCH(self), NULL);

	return(XFDASHBOARD_TOGGLE_BUTTON(self->priv->appsButton));
}
