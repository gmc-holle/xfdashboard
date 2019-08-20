/*
 * model: A simple and generic data model holding one value per row
 * 
 * Copyright 2012-2019 Stephan Haller <nomad@froevel.de>
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

#ifndef __LIBXFDASHBOARD_MODEL__
#define __LIBXFDASHBOARD_MODEL__

#if !defined(__LIBXFDASHBOARD_H_INSIDE__) && !defined(LIBXFDASHBOARD_COMPILATION)
#error "Only <libxfdashboard/libxfdashboard.h> can be included directly."
#endif

#include <glib-object.h>

G_BEGIN_DECLS

/* Object declaration: XfdashboardModelIter */
#define XFDASHBOARD_TYPE_MODEL_ITER				(xfdashboard_model_iter_get_type())
#define XFDASHBOARD_MODEL_ITER(obj)				(G_TYPE_CHECK_INSTANCE_CAST((obj), XFDASHBOARD_TYPE_MODEL_ITER, XfdashboardModelIter))
#define XFDASHBOARD_IS_MODEL_ITER(obj)			(G_TYPE_CHECK_INSTANCE_TYPE((obj), XFDASHBOARD_TYPE_MODEL_ITER))
#define XFDASHBOARD_MODEL_ITER_CLASS(klass)		(G_TYPE_CHECK_CLASS_CAST((klass), XFDASHBOARD_TYPE_MODEL_ITER, XfdashboardModelIterClass))
#define XFDASHBOARD_IS_MODEL_ITER_CLASS(klass)	(G_TYPE_CHECK_CLASS_TYPE((klass), XFDASHBOARD_TYPE_MODEL_ITER))
#define XFDASHBOARD_MODEL_ITER_GET_CLASS(obj)	(G_TYPE_INSTANCE_GET_CLASS((obj), XFDASHBOARD_TYPE_MODEL_ITER, XfdashboardModelIterClass))

typedef struct _XfdashboardModelIter			XfdashboardModelIter;
typedef struct _XfdashboardModelIterClass		XfdashboardModelIterClass;
typedef struct _XfdashboardModelIterPrivate		XfdashboardModelIterPrivate;

struct _XfdashboardModelIter
{
	/*< private >*/
	/* Parent instance */
	GObjectClass					parent_instance;

	/* Private structure */
	XfdashboardModelIterPrivate		*priv;
};

struct _XfdashboardModelIterClass
{
	/*< private >*/
	/* Parent class */
	GObjectClass					parent_class;

	/*< public >*/
	/* Virtual functions */
};


/* Object declaration: XfdashboardModel */
#define XFDASHBOARD_TYPE_MODEL				(xfdashboard_model_get_type())
#define XFDASHBOARD_MODEL(obj)				(G_TYPE_CHECK_INSTANCE_CAST((obj), XFDASHBOARD_TYPE_MODEL, XfdashboardModel))
#define XFDASHBOARD_IS_MODEL(obj)			(G_TYPE_CHECK_INSTANCE_TYPE((obj), XFDASHBOARD_TYPE_MODEL))
#define XFDASHBOARD_MODEL_CLASS(klass)		(G_TYPE_CHECK_CLASS_CAST((klass), XFDASHBOARD_TYPE_MODEL, XfdashboardModelClass))
#define XFDASHBOARD_IS_MODEL_CLASS(klass)	(G_TYPE_CHECK_CLASS_TYPE((klass), XFDASHBOARD_TYPE_MODEL))
#define XFDASHBOARD_MODEL_GET_CLASS(obj)	(G_TYPE_INSTANCE_GET_CLASS((obj), XFDASHBOARD_TYPE_MODEL, XfdashboardModelClass))

typedef struct _XfdashboardModel			XfdashboardModel;
typedef struct _XfdashboardModelClass		XfdashboardModelClass;
typedef struct _XfdashboardModelPrivate		XfdashboardModelPrivate;

struct _XfdashboardModel
{
	/* Parent instance */
	GObjectClass					parent_instance;

	/* Private structure */
	XfdashboardModelPrivate			*priv;
};

struct _XfdashboardModelClass
{
	/*< private >*/
	/* Parent class */
	GObjectClass					parent_class;

	/*< public >*/
	/* Virtual functions */
	void (*row_added)(XfdashboardModel *self,
						XfdashboardModelIter *inIter);
	void (*row_removed)(XfdashboardModel *self,
						XfdashboardModelIter *inIter);
	void (*row_changed)(XfdashboardModel *self,
						XfdashboardModelIter *inIter);

	void (*sort_changed)(XfdashboardModel *self);

	void (*filter_changed)(XfdashboardModel *self);
};


/* Public API */
typedef void (*XfdashboardModelForeachFunc)(XfdashboardModelIter *inIter,
											gpointer inData,
											gpointer inUserData);
typedef gint (*XfdashboardModelSortFunc)(XfdashboardModelIter *inLeftIter,
											XfdashboardModelIter *inRightIter,
											gpointer inUserData);
typedef gboolean (*XfdashboardModelFilterFunc)(XfdashboardModelIter *inIter,
												gpointer inUserData);

GType xfdashboard_model_get_type(void) G_GNUC_CONST;
GType xfdashboard_model_iter_get_type(void) G_GNUC_CONST;

/* Model creation functions */
XfdashboardModel* xfdashboard_model_new(void);
XfdashboardModel* xfdashboard_model_new_with_free_data(GDestroyNotify inFreeDataFunc);

/* Model information functions */
gint xfdashboard_model_get_rows_count(XfdashboardModel *self);

/* Model access functions */
gpointer xfdashboard_model_get(XfdashboardModel *self, gint inRow);

/* Model manipulation functions */
gboolean xfdashboard_model_append(XfdashboardModel *self,
									gpointer inData,
									XfdashboardModelIter **outIter);
gboolean xfdashboard_model_prepend(XfdashboardModel *self,
									gpointer inData,
									XfdashboardModelIter **outIter);
gboolean xfdashboard_model_insert(XfdashboardModel *self,
									gint inRow,
									gpointer inData,
									XfdashboardModelIter **outIter);
gboolean xfdashboard_model_set(XfdashboardModel *self,
								gint inRow,
								gpointer inData,
								XfdashboardModelIter **outIter);
gboolean xfdashboard_model_remove(XfdashboardModel *self, gint inRow);
void xfdashboard_model_remove_all(XfdashboardModel *self);

/* Model foreach functions */
void xfdashboard_model_foreach(XfdashboardModel *self,
								XfdashboardModelForeachFunc inForeachCallback,
								gpointer inUserData);

/* Model sort functions */
gboolean xfdashboard_model_is_sorted(XfdashboardModel *self);
void xfdashboard_model_set_sort(XfdashboardModel *self,
								XfdashboardModelSortFunc inSortCallback,
								gpointer inUserData,
								GDestroyNotify inUserDataDestroyCallback);
void xfdashboard_model_resort(XfdashboardModel *self);

/* Model filter functions */
gboolean xfdashboard_model_is_filtered(XfdashboardModel *self);
void xfdashboard_model_set_filter(XfdashboardModel *self,
									XfdashboardModelFilterFunc inFilterCallback,
									gpointer inUserData,
									GDestroyNotify inUserDataDestroyCallback);
gboolean xfdashboard_model_filter_row(XfdashboardModel *self, gint inRow);


/* Model iterator functions */
XfdashboardModelIter* xfdashboard_model_iter_new(XfdashboardModel *inModel);
XfdashboardModelIter* xfdashboard_model_iter_new_for_row(XfdashboardModel *inModel, gint inRow);
XfdashboardModelIter* xfdashboard_model_iter_copy(XfdashboardModelIter *self);
gboolean xfdashboard_model_iter_next(XfdashboardModelIter *self);
gboolean xfdashboard_model_iter_prev(XfdashboardModelIter *self);
gboolean xfdashboard_model_iter_move_to_row(XfdashboardModelIter *self, gint inRow);

/* Model iterator information functions */
XfdashboardModel* xfdashboard_model_iter_get_model(XfdashboardModelIter *self);
guint xfdashboard_model_iter_get_row(XfdashboardModelIter *self);

/* Model iterator access functions */
gpointer xfdashboard_model_iter_get(XfdashboardModelIter *self);

/* Model iterator manipulation functions */
gboolean xfdashboard_model_iter_set(XfdashboardModelIter *self, gpointer inData);
gboolean xfdashboard_model_iter_remove(XfdashboardModelIter *self);

/* Model filter functions */
gboolean xfdashboard_model_iter_filter(XfdashboardModelIter *self);

G_END_DECLS

#endif	/* __LIBXFDASHBOARD_MODEL__ */
