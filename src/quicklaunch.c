/*
 * quicklaunch.c: Quicklaunch box
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

#include "quicklaunch.h"
#include "quicklaunch-icon.h"

#include <math.h>

/* Define this class in GObject system */
G_DEFINE_TYPE(XfdashboardQuicklaunch,
				xfdashboard_quicklaunch,
				CLUTTER_TYPE_ACTOR)

/* Private structure - access only by public API if needed */
#define XFDASHBOARD_QUICKLAUNCH_GET_PRIVATE(obj) \
	(G_TYPE_INSTANCE_GET_PRIVATE((obj), XFDASHBOARD_TYPE_QUICKLAUNCH, XfdashboardQuicklaunchPrivate))

struct _XfdashboardQuicklaunchPrivate
{
	/* Background cairo texture */
	ClutterActor			*background;
	
	/* Icon group (container of XfdashboardQuicklaunchIcon) */
	ClutterActor			*icons;
	guint					maxIconsCount;
	gfloat					iconsScale;
	guint					normalIconSize;

	/* Spacing between icons (is also margin to background */
	gfloat					spacing;
};

/* Properties */
enum
{
	PROP_0,

	PROP_ICONS_COUNT,
	PROP_ICONS_MAX_COUNT,
	PROP_ICONS_SCALE,
	PROP_ICONS_NORMAL_SIZE,

	PROP_SPACING,
	
	PROP_LAST
};

static GParamSpec* XfdashboardQuicklaunchProperties[PROP_LAST]={ 0, };

/* IMPLEMENTATION: Private variables and methods */
#define DEG2RAD(deg)	(deg*(M_PI/180.0f))

#define SCALE_MIN		0.25
#define SCALE_MAX		1.0
#define SCALE_STEP		0.25

static gchar	*quicklaunch_apps[]=	{
											"firefox.desktop",
											"evolution.desktop",
											"Terminal.desktop",
											"Thunar.desktop"
										};

/* Icon was clicked */
static gboolean _xfdashboard_quicklaunch_on_clicked_icon(ClutterActor *inActor, gpointer inUserData)
{
	g_return_val_if_fail(XFDASHBOARD_IS_QUICKLAUNCH_ICON(inActor), FALSE);
	g_return_val_if_fail(XFDASHBOARD_IS_QUICKLAUNCH(inUserData), FALSE);

	XfdashboardQuicklaunchIcon	*icon=XFDASHBOARD_QUICKLAUNCH_ICON(inActor);
	const GDesktopAppInfo		*appInfo;
	GError						*error=NULL;

	/* Get application information object from icon */
	appInfo=xfdashboard_quicklaunch_icon_get_desktop_application_info(icon);
	if(!appInfo)
	{
		g_warning("Could not launch application: NULL-application-info-object");
		return(FALSE);
	}

	/* Launch application */
	if(!g_app_info_launch(G_APP_INFO(appInfo), NULL, NULL, &error))
	{
		g_warning("Could not launch application %s",
					(error && error->message) ?  error->message : "unknown error");
					
		return(FALSE);
	}

	/* After successfully spawn new process of application (which does not
	 * indicate successful startup of application) quit application
	 */
	clutter_main_quit();

	return(TRUE);
}

/* Relayout icons */
static void _xfdashboard_quicklaunch_layout_icons(XfdashboardQuicklaunch *self)
{
	XfdashboardQuicklaunchPrivate	*priv=XFDASHBOARD_QUICKLAUNCH(self)->priv;
	gint							i, numberIcons;
	ClutterGroup					*iconGroup;

	iconGroup=CLUTTER_GROUP(priv->icons);
	numberIcons=clutter_group_get_n_children(iconGroup);
	for(i=0; i<numberIcons; i++)
	{
		ClutterActor				*actor=clutter_group_get_nth_child(iconGroup, i);
		
		clutter_actor_set_position(actor, priv->spacing, priv->spacing+i*(priv->normalIconSize+priv->spacing));
		clutter_actor_set_size(actor, priv->normalIconSize, priv->normalIconSize);
	}

	clutter_actor_queue_relayout(CLUTTER_ACTOR(self));
}

/* Check width/height of icons, scale icons to fit allocation and
 * enforce size of background etc. */
static gfloat _xfdashboard_quicklaunch_check_size(XfdashboardQuicklaunch *self,
													ClutterActorBox *ioBox)
{
	XfdashboardQuicklaunchPrivate	*priv=XFDASHBOARD_QUICKLAUNCH(self)->priv;
	gfloat							maxW, maxH;
	gfloat							iconW, iconH;
	gfloat							iconScale;
	gint							numberIcons;

	/* If check fail return negative scaling to indicate failure */
	g_return_val_if_fail(ioBox, -1.0f);

	/* Get width and heigt of allocation box */
	clutter_actor_box_get_size(ioBox, &maxW, &maxH);

	/* Check if all children of icon group fits in width and height.
	 * If thex do not scale them down from maximum scale to minimum
	 * scale in defined steps until they fit.
	 */
	iconScale=SCALE_MAX+SCALE_STEP;
	numberIcons=clutter_group_get_n_children(CLUTTER_GROUP(priv->icons));
	do
	{
		/* Try next scaling */
		iconScale-=SCALE_STEP;

		/* Determine scaled width of icons */
		iconW=priv->normalIconSize;
		iconW*=iconScale;

		/* Determine scaled height of icons */
		iconH=numberIcons*(priv->normalIconSize+priv->spacing);
		iconH*=iconScale;
	}
	while((iconW>maxW || iconH>maxH) && iconScale>SCALE_MIN);

	/* Update properties */
	priv->iconsScale=iconScale;

	/* Update size in allocation box to scale size of icons */
	clutter_actor_box_set_origin(ioBox, 0, 0);
	clutter_actor_box_set_size(ioBox, iconW, iconH);

	/* Return scaling */
	return(iconScale);
}

/* Draw background into Cairo texture */
static gboolean _xfdashboard_quicklaunch_draw_background(ClutterCairoTexture *inActor,
															cairo_t *inCairo,
															gpointer inUserData)
{
	g_return_val_if_fail(XFDASHBOARD_IS_QUICKLAUNCH(inUserData), FALSE);
	
	XfdashboardQuicklaunchPrivate	*priv=XFDASHBOARD_QUICKLAUNCH(inUserData)->priv;
	guint							width, height;

	/* Clear the contents of actor to avoid painting
	 * over the previous contents
	 */
	clutter_cairo_texture_clear(inActor);

	/* Scale to the size of actor */
	clutter_cairo_texture_get_surface_size(inActor, &width, &height);

	cairo_set_line_cap(inCairo, CAIRO_LINE_CAP_ROUND);
	cairo_set_line_width(inCairo, 1.5);

	/* Draw surrounding rectangle and fill it */
	clutter_cairo_set_source_color(inCairo, CLUTTER_COLOR_White);

	cairo_move_to(inCairo, 0, 0);
	cairo_rel_line_to(inCairo, width-priv->spacing, 0);
	cairo_arc(inCairo,
				width-priv->spacing, priv->spacing,
				priv->spacing,
				DEG2RAD(-90), DEG2RAD(0));

	cairo_move_to(inCairo, width, priv->spacing);
	cairo_rel_line_to(inCairo, 0, height-(2*priv->spacing));
	cairo_arc(inCairo,
				width-priv->spacing, height-priv->spacing,
				priv->spacing,
				DEG2RAD(0), DEG2RAD(90));

	cairo_move_to(inCairo, width-priv->spacing, height);
	cairo_rel_line_to(inCairo, -(width-priv->spacing), 0);

	cairo_close_path(inCairo);
	cairo_stroke(inCairo);

	return(TRUE);
}

/* IMPLEMENTATION: ClutterActor */

/* Show this actor and all children */
static void xfdashboard_quicklaunch_show_all(ClutterActor *self)
{
	XfdashboardQuicklaunchPrivate	*priv=XFDASHBOARD_QUICKLAUNCH(self)->priv;

	clutter_actor_show(priv->background);
	clutter_actor_show(priv->icons);
	clutter_actor_show(self);
}

/* Hide this actor and all children */
static void xfdashboard_quicklaunch_hide_all(ClutterActor *self)
{
	XfdashboardQuicklaunchPrivate	*priv=XFDASHBOARD_QUICKLAUNCH(self)->priv;

	clutter_actor_hide(self);
	clutter_actor_hide(priv->background);
	clutter_actor_hide(priv->icons);
}

/* Paint actor */
static void xfdashboard_quicklaunch_paint(ClutterActor *self)
{
	XfdashboardQuicklaunchPrivate	*priv=XFDASHBOARD_QUICKLAUNCH(self)->priv;

	/* Paint background and icon group */
	clutter_actor_paint(priv->background);
	clutter_actor_paint(priv->icons);
}

/* Retrieve paint volume of this actor and its children */
static gboolean xfdashboard_quicklaunch_get_paint_volume(ClutterActor *self,
															ClutterPaintVolume *inVolume)
{
	XfdashboardQuicklaunchPrivate	*priv=XFDASHBOARD_QUICKLAUNCH(self)->priv;
	const ClutterPaintVolume		*childVolume;

	/* Get paint volume of background */
	childVolume=clutter_actor_get_transformed_paint_volume(priv->background, self);
	if(!childVolume) return(FALSE);

	clutter_paint_volume_union(inVolume, childVolume);

	/* Union the paint volumes of icon group */
	childVolume=clutter_actor_get_transformed_paint_volume(priv->icons, self);
	if(!childVolume) return(FALSE);

	clutter_paint_volume_union(inVolume, childVolume);

	return(TRUE);
}

/* Pick all the child actors */
static void xfdashboard_quicklaunch_pick(ClutterActor *self, const ClutterColor *inPick)
{
	XfdashboardQuicklaunchPrivate	*priv=XFDASHBOARD_QUICKLAUNCH(self)->priv;

	CLUTTER_ACTOR_CLASS(xfdashboard_quicklaunch_parent_class)->pick(self, inPick);

	clutter_actor_paint(priv->background);
	clutter_actor_paint(priv->icons);
}

/* Get preferred width/height */
static void xfdashboard_quicklaunch_get_preferred_width(ClutterActor *self,
														gfloat inForHeight,
														gfloat *outMinWidth,
														gfloat *outNaturalWidth)
{
	XfdashboardQuicklaunchPrivate	*priv=XFDASHBOARD_QUICKLAUNCH(self)->priv;
	gfloat							maxMinWidth, maxNaturalWidth;
	gfloat							minWidth, naturalWidth;

	/* Minimum size is at least size of box plus spacing twice */
	maxMinWidth=maxNaturalWidth=priv->normalIconSize+(2*priv->spacing);

	/* Get preferred size of background first */
	clutter_actor_get_preferred_width(priv->background,
										inForHeight,
										&minWidth,
										&naturalWidth);

	if(minWidth>maxMinWidth) maxMinWidth=minWidth;
	if(naturalWidth>maxNaturalWidth) maxNaturalWidth=naturalWidth;

	/* Now get preferred size of icon group */
	clutter_actor_get_preferred_width(priv->icons,
										inForHeight,
										&minWidth,
										&naturalWidth);

	if(minWidth>maxMinWidth) maxMinWidth=minWidth;
	if(naturalWidth>maxNaturalWidth) maxNaturalWidth=naturalWidth;

	*outMinWidth=maxMinWidth;
	*outNaturalWidth=maxNaturalWidth;
}

static void xfdashboard_quicklaunch_get_preferred_height(ClutterActor *self,
															gfloat inForWidth,
															gfloat *outMinHeight,
															gfloat *outNaturalHeight)
{
	XfdashboardQuicklaunchPrivate	*priv=XFDASHBOARD_QUICKLAUNCH(self)->priv;
	gfloat							maxMinHeight, maxNaturalHeight;
	gfloat							minHeight, naturalHeight;
	gint							numberIcons;

	/* Minimum size is at least number of icons multiplied with size of
	 * a single icon plus spacing and additionally increased by spacing
	 * at minimum scale
	 */
	numberIcons=clutter_group_get_n_children(CLUTTER_GROUP(priv->icons));

	maxMinHeight=(numberIcons*(priv->normalIconSize+priv->spacing))+priv->spacing;
	maxMinHeight*=SCALE_MIN;
	
	maxNaturalHeight=maxMinHeight;
			
	/* Get preferred size of background first */
	clutter_actor_get_preferred_height(priv->background,
										inForWidth,
										&minHeight,
										&naturalHeight);

	if(minHeight>maxMinHeight) maxMinHeight=minHeight;
	if(naturalHeight>maxNaturalHeight) maxNaturalHeight=naturalHeight;

	/* Now get preferred size of icon group */
	clutter_actor_get_preferred_height(priv->icons,
										inForWidth,
										&minHeight,
										&naturalHeight);

	if(minHeight>maxMinHeight) maxMinHeight=minHeight;
	if(naturalHeight>maxNaturalHeight) maxNaturalHeight=naturalHeight;

	*outMinHeight=maxMinHeight;
	*outNaturalHeight=maxNaturalHeight;
}

/* Allocate position and size of actor and its children*/
static void xfdashboard_quicklaunch_allocate(ClutterActor *self,
												const ClutterActorBox *inBox,
												ClutterAllocationFlags inFlags)
{
	XfdashboardQuicklaunchPrivate	*priv=XFDASHBOARD_QUICKLAUNCH(self)->priv;
	ClutterActorBox					*box=NULL;
	gfloat							allocW, allocH;
	gfloat							iconsW, iconsH;
	gfloat							iconScale;

	/* Get size of allocation */
	clutter_actor_box_get_size(inBox, &allocW, &allocH);

	/* Determine minimum size for icons */
	box=clutter_actor_box_copy(inBox);
	iconScale=_xfdashboard_quicklaunch_check_size(XFDASHBOARD_QUICKLAUNCH(self), box);
	if(G_UNLIKELY(iconScale<SCALE_MIN))
	{
		g_warning("Icons of quicklaunch will not fit - using allocation unmodified!");
		iconScale=SCALE_MIN;
		clutter_actor_box_free(box);
		box=clutter_actor_box_copy(inBox);
	}
	clutter_actor_box_get_size(box, &iconsW, &iconsH);

	/* Adjust allocation to maximum height of given allocation or
	 * determined minimum height and increase for spacing.
	 * 
	 * We MUST decrease width and height because allocation will
	 * not be changed and constraints will NOT work! Decreasing
	 * by 0.1px should not harm but might scale icons */
	allocW=iconsW;
	allocW-=0.1f;
	allocH=(iconsH>allocH ? iconsH : allocH);
	allocH-=0.1f;

	/* Set allocation and scale for icons */
	clutter_actor_box_set_origin(box, 0.0f, 0.0f);
	clutter_actor_box_set_size(box, allocW, allocH);

	clutter_actor_allocate(priv->icons, box, inFlags);
	clutter_actor_set_scale(priv->icons, iconScale, iconScale);

	/* Allocation for background */
	clutter_actor_allocate(priv->background, box, inFlags);
	
	/* Call parent's class allocation method */
	CLUTTER_ACTOR_CLASS(xfdashboard_quicklaunch_parent_class)->allocate(self, box, inFlags);

	/* Free copied allocation box */
	clutter_actor_box_free(box);

	/* Update properties */
	priv->maxIconsCount=(allocH/SCALE_MIN)/priv->normalIconSize;
}

/* Destroy this actor */
static void xfdashboard_quicklaunch_destroy(ClutterActor *self)
{
	XfdashboardQuicklaunchPrivate		*priv=XFDASHBOARD_QUICKLAUNCH(self)->priv;

	/* Destroy background and all our children */
	if(priv->background)
	{
		clutter_actor_destroy(priv->background);
		priv->background=NULL;
	}

	if(priv->icons)
	{
		clutter_actor_destroy(priv->icons);
		priv->icons=NULL;
	}

	/* Call parent's class destroy method */
	if(CLUTTER_ACTOR_CLASS(xfdashboard_quicklaunch_parent_class)->destroy)
	{
		CLUTTER_ACTOR_CLASS(xfdashboard_quicklaunch_parent_class)->destroy(self);
	}
}

/* IMPLEMENTATION: GObject */

/* Dispose this object */
static void xfdashboard_quicklaunch_dispose(GObject *inObject)
{
	G_GNUC_UNUSED XfdashboardQuicklaunch		*self=XFDASHBOARD_QUICKLAUNCH(inObject);

	/* Call parent's class dispose method */
	G_OBJECT_CLASS(xfdashboard_quicklaunch_parent_class)->dispose(inObject);
}

/* Set/get properties */
static void xfdashboard_quicklaunch_set_property(GObject *inObject,
													guint inPropID,
													const GValue *inValue,
													GParamSpec *inSpec)
{
	XfdashboardQuicklaunch		*self=XFDASHBOARD_QUICKLAUNCH(inObject);
	
	switch(inPropID)
	{
		case PROP_ICONS_NORMAL_SIZE:
			self->priv->normalIconSize=g_value_get_uint(inValue);
			_xfdashboard_quicklaunch_layout_icons(self);
			break;

		case PROP_SPACING:
			self->priv->spacing=g_value_get_float(inValue);
			_xfdashboard_quicklaunch_layout_icons(self);
			break;

		default:
			G_OBJECT_WARN_INVALID_PROPERTY_ID(inObject, inPropID, inSpec);
			break;
	}
}

static void xfdashboard_quicklaunch_get_property(GObject *inObject,
													guint inPropID,
													GValue *outValue,
													GParamSpec *inSpec)
{
	XfdashboardQuicklaunch		*self=XFDASHBOARD_QUICKLAUNCH(inObject);

	switch(inPropID)
	{
		case PROP_ICONS_COUNT:
			g_value_set_uint(outValue, clutter_group_get_n_children(CLUTTER_GROUP(self->priv->icons)));
			break;

		case PROP_ICONS_MAX_COUNT:
			g_value_set_uint(outValue, self->priv->maxIconsCount);
			break;

		case PROP_ICONS_SCALE:
			g_value_set_float(outValue, self->priv->iconsScale);
			break;

		case PROP_ICONS_NORMAL_SIZE:
			g_value_set_uint(outValue, self->priv->normalIconSize);
			break;

		case PROP_SPACING:
			g_value_set_float(outValue, self->priv->spacing);
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
	GObjectClass			*gobjectClass=G_OBJECT_CLASS(klass);
	ClutterActorClass		*actorClass=CLUTTER_ACTOR_CLASS(klass);

	/* Override functions */
	actorClass->show_all=xfdashboard_quicklaunch_show_all;
	actorClass->hide_all=xfdashboard_quicklaunch_hide_all;
	actorClass->paint=xfdashboard_quicklaunch_paint;
	actorClass->get_paint_volume=xfdashboard_quicklaunch_get_paint_volume;
	actorClass->pick=xfdashboard_quicklaunch_pick;
	actorClass->get_preferred_width=xfdashboard_quicklaunch_get_preferred_width;
	actorClass->get_preferred_height=xfdashboard_quicklaunch_get_preferred_height;
	actorClass->allocate=xfdashboard_quicklaunch_allocate;
	actorClass->destroy=xfdashboard_quicklaunch_destroy;

	gobjectClass->set_property=xfdashboard_quicklaunch_set_property;
	gobjectClass->get_property=xfdashboard_quicklaunch_get_property;
	gobjectClass->dispose=xfdashboard_quicklaunch_dispose;

	/* Set up private structure */
	g_type_class_add_private(klass, sizeof(XfdashboardQuicklaunchPrivate));

	/* Define properties */
	XfdashboardQuicklaunchProperties[PROP_ICONS_COUNT]=
		g_param_spec_uint("icons-count",
							"Number of icons",
							"Number of icons in quicklaunch",
							0,
							G_MAXUINT,
							0,
							G_PARAM_READABLE);

	XfdashboardQuicklaunchProperties[PROP_ICONS_MAX_COUNT]=
		g_param_spec_uint("icons-max-count",
							"Maximum number of icons",
							"Maximum number of icons the quicklaunch box can hold at current scale",
							0,
							G_MAXUINT,
							0,
							G_PARAM_READABLE);

	XfdashboardQuicklaunchProperties[PROP_ICONS_SCALE]=
		g_param_spec_float("icons-scale",
							"Scale factor for icons",
							"Current scale factor used for icons in quicklaunch to make them fit",
							0.0,
							SCALE_MAX,
							SCALE_MAX,
							G_PARAM_READABLE);

	XfdashboardQuicklaunchProperties[PROP_ICONS_NORMAL_SIZE]=
		g_param_spec_uint("icons-normal-size",
							"Normal size of an icon",
							"The normal size in pixels of a single icon in quicklaunch at maximum scale factor",
							0,
							G_MAXUINT,
							64,
							G_PARAM_READWRITE | G_PARAM_CONSTRUCT);

	XfdashboardQuicklaunchProperties[PROP_SPACING]=
		g_param_spec_float("spacing",
							"Spacing",
							"Spacing in pixels between all elements in quicklaunch",
							0.0,
							G_MAXFLOAT,
							8.0,
							G_PARAM_READWRITE | G_PARAM_CONSTRUCT);

	g_object_class_install_properties(gobjectClass, PROP_LAST, XfdashboardQuicklaunchProperties);
}

/* Object initialization
 * Create private structure and set up default values
 */
static void xfdashboard_quicklaunch_init(XfdashboardQuicklaunch *self)
{
	XfdashboardQuicklaunchPrivate	*priv;
	ClutterConstraint				*constraint;
	gint							i;

	priv=self->priv=XFDASHBOARD_QUICKLAUNCH_GET_PRIVATE(self);

	/* Set up default values */
	priv->maxIconsCount=0;
	priv->iconsScale=SCALE_MAX;
	priv->normalIconSize=0;
	priv->spacing=0.0f;

	/* Set up background actor and bind its size to this box */
	priv->background=clutter_cairo_texture_new(1, 1);
	clutter_cairo_texture_set_auto_resize(CLUTTER_CAIRO_TEXTURE(priv->background), TRUE);
	g_signal_connect(priv->background, "draw", G_CALLBACK(_xfdashboard_quicklaunch_draw_background), self);
	clutter_actor_set_parent(priv->background, CLUTTER_ACTOR(self));

	constraint=clutter_bind_constraint_new(CLUTTER_ACTOR(self), CLUTTER_BIND_POSITION, 0);
	clutter_actor_add_constraint(priv->background, constraint);

	constraint=clutter_bind_constraint_new(CLUTTER_ACTOR(self), CLUTTER_BIND_SIZE, 0);
	clutter_actor_add_constraint(priv->background, constraint);

	/* Set up icon group */
	priv->icons=clutter_group_new();
	clutter_actor_set_parent(priv->icons, CLUTTER_ACTOR(self));

	constraint=clutter_bind_constraint_new(CLUTTER_ACTOR(self), CLUTTER_BIND_POSITION, priv->spacing);
	clutter_actor_add_constraint(priv->icons, constraint);

	constraint=clutter_bind_constraint_new(CLUTTER_ACTOR(self), CLUTTER_BIND_SIZE, -(2*priv->spacing));
	clutter_actor_add_constraint(priv->icons, constraint);

	/* TODO: Remove the following actor(s) for application icons
	 *       in quicklaunch box as soon as xfconf is implemented
	 */
	for(i=0; i<(sizeof(quicklaunch_apps)/sizeof(quicklaunch_apps[0])); i++)
	{
		ClutterActor				*actor;
		
		actor=xfdashboard_quicklaunch_icon_new_full(quicklaunch_apps[i]);
		clutter_container_add_actor(CLUTTER_CONTAINER(priv->icons), actor);
		g_signal_connect(actor, "clicked", G_CALLBACK(_xfdashboard_quicklaunch_on_clicked_icon), self);
	}

	/* Re-layout icons. This also enforces a queued re-layout of
	 * all children added
	 */
	_xfdashboard_quicklaunch_layout_icons(self);
}

/* Implementation: Public API */

/* Create new actor */
ClutterActor* xfdashboard_quicklaunch_new(void)
{
	return(g_object_new(XFDASHBOARD_TYPE_QUICKLAUNCH, NULL));
}

/* Get/set icons' normal size */
guint xfdashboard_quicklaunch_get_normal_icon_size(XfdashboardQuicklaunch *self)
{
	g_return_val_if_fail(XFDASHBOARD_IS_QUICKLAUNCH(self), 0);

	return(self->priv->normalIconSize);
}

void xfdashboard_quicklaunch_set_normal_icon_size(XfdashboardQuicklaunch *self, guint inSize)
{
	g_return_if_fail(XFDASHBOARD_IS_QUICKLAUNCH(self));

	self->priv->normalIconSize=inSize;
	_xfdashboard_quicklaunch_layout_icons(self);
}

/* Get/set spacing */
gfloat xfdashboard_quicklaunch_get_spacing(XfdashboardQuicklaunch *self)
{
	g_return_val_if_fail(XFDASHBOARD_IS_QUICKLAUNCH(self), 0);

	return(self->priv->normalIconSize);
}

void xfdashboard_quicklaunch_set_spacing(XfdashboardQuicklaunch *self, gfloat inSpacing)
{
	g_return_if_fail(XFDASHBOARD_IS_QUICKLAUNCH(self));

	self->priv->spacing=inSpacing;
	_xfdashboard_quicklaunch_layout_icons(self);
}
