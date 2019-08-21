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

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <libxfdashboard/model.h>

#include <glib/gi18n-lib.h>


/* Define theses classes in GObject system */
struct _XfdashboardModelPrivate
{
	/* Instance related */
	GSequence					*data;
	GDestroyNotify				freeDataCallback;

	XfdashboardModelSortFunc	sortCallback;
	gpointer					sortUserData;
	GDestroyNotify				sortUserDataDestroyCallback;

	XfdashboardModelFilterFunc	filterCallback;
	gpointer					filterUserData;
	GDestroyNotify				filterUserDataDestroyCallback;
};

G_DEFINE_TYPE_WITH_PRIVATE(XfdashboardModel,
							xfdashboard_model,
							G_TYPE_OBJECT);

struct _XfdashboardModelIterPrivate
{
	/* Instance related */
	XfdashboardModel			*model;
	GSequenceIter				*iter;
};

G_DEFINE_TYPE_WITH_PRIVATE(XfdashboardModelIter,
							xfdashboard_model_iter,
							G_TYPE_OBJECT);

/* Properties */
enum
{
	PROP_0,

	PROP_ROWS,
	PROP_SORT_SET,
	PROP_FILTER_SET,
	PROP_FREE_DATA_CALLBACK,

	PROP_LAST
};

static GParamSpec* XfdashboardModelProperties[PROP_LAST]={ 0, };

/* Signals */
enum
{
	SIGNAL_ROW_ADDED,
	SIGNAL_ROW_REMOVED,
	SIGNAL_ROW_CHANGED,
	SIGNAL_SORT_CHANGED,
	SIGNAL_FILTER_CHANGED,

	SIGNAL_LAST
};

static guint XfdashboardModelSignals[SIGNAL_LAST]={ 0, };


/* IMPLEMENTATION: Private variables and methods */

typedef struct _XfdashboardModelSortData			XfdashboardModelSortData;
struct _XfdashboardModelSortData
{
	XfdashboardModel			*model;
	XfdashboardModelIter		*leftIter;
	XfdashboardModelIter		*rightIter;
};

/* Checks for valid iterator for model */
static gboolean _xfdashboard_model_iter_is_valid(XfdashboardModelIter *self, gboolean inNeedsIter)
{
	XfdashboardModelIterPrivate		*priv;

	g_return_val_if_fail(XFDASHBOARD_IS_MODEL_ITER(self), FALSE);

	priv=self->priv;

	/* Check if model is set */
	if(!priv->model) return(FALSE);

	/* Check if an iterator is set when an iterator is needed */
	if(inNeedsIter && !priv->iter) return(FALSE);

	/* Check if an iterator is set and if it is then check if associated
	 * GSequence at iterator matches the one associated with the model.
	 * If an iterator is needed the check before ensures that in this check
	 * an iterator exists and the check will be performed.
	 */
	if(priv->iter)
	{
		if(g_sequence_iter_get_sequence(priv->iter)!=priv->model->priv->data) return(FALSE);
	}

	/* If we get here all tests are passed successfully and iterator is valid */
	return(TRUE);
}

/* Checks if requested row is valid in this model */
static gboolean _xfdashboard_model_is_valid_row(XfdashboardModel *self, gint inRow)
{
	XfdashboardModelPrivate			*priv;

	g_return_val_if_fail(XFDASHBOARD_IS_MODEL(self), FALSE);

	priv=self->priv;

	/* Check if row is a positive number and smaller than the total numbers
	 * of rows in model's data.
	 */
	if(inRow<0 || inRow>=g_sequence_get_length(priv->data)) return(FALSE);

	/* If we get here the requested row is within model's data and valid */
	return(TRUE);
}

/* Internal callback function for sorting which creates iterators used for
 * user supplied callback function.
 */
static gint _xfdashboard_model_sort_internal(GSequenceIter *inLeft,
												GSequenceIter *inRight,
												gpointer inUserData)
{
	XfdashboardModelSortData		*sortData;
	XfdashboardModelPrivate			*priv;
	gint							result;

	g_return_val_if_fail(inLeft, 0);
	g_return_val_if_fail(inRight, 0);
	g_return_val_if_fail(inUserData, 0);

	sortData=(XfdashboardModelSortData*)inUserData;
	priv=sortData->model->priv;

	/* Update iterators to pass to user supplied sort callback function */
	sortData->leftIter->priv->iter=inLeft;
	sortData->rightIter->priv->iter=inRight;

	/* Call user supplied sort callback function and return its result */
	result=(priv->sortCallback)(sortData->leftIter,
								sortData->rightIter,
								priv->sortUserData);

	/* Return result of user supplied sort callback function */
	return(result);
}

/* IMPLEMENTATION: GObject */

/* Dispose this object of type XfdashboardModel */
static void _xfdashboard_model_dispose(GObject *inObject)
{
	XfdashboardModel				*self=XFDASHBOARD_MODEL(inObject);
	XfdashboardModelPrivate			*priv=self->priv;

	/* Release our allocated variables */
	if(priv->sortUserData &&
		priv->sortUserDataDestroyCallback)
	{
		(priv->sortUserDataDestroyCallback)(priv->sortUserData);
	}
	priv->sortUserDataDestroyCallback=NULL;
	priv->sortUserData=NULL;
	priv->sortCallback=NULL;

	if(priv->filterUserData &&
		priv->filterUserDataDestroyCallback)
	{
		(priv->filterUserDataDestroyCallback)(priv->filterUserData);
	}
	priv->filterUserDataDestroyCallback=NULL;
	priv->filterUserData=NULL;
	priv->filterCallback=NULL;

	if(priv->data)
	{
		g_sequence_free(priv->data);
		priv->data=NULL;
	}
	priv->freeDataCallback=NULL;

	/* Call parent's class dispose method */
	G_OBJECT_CLASS(xfdashboard_model_parent_class)->dispose(inObject);
}

/* Set/get properties of type XfdashboardModel */
static void _xfdashboard_model_set_property(GObject *inObject,
											guint inPropID,
											const GValue *inValue,
											GParamSpec *inSpec)
{
	XfdashboardModel				*self=XFDASHBOARD_MODEL(inObject);

	switch(inPropID)
	{
		case PROP_FREE_DATA_CALLBACK:
			self->priv->freeDataCallback=g_value_get_pointer(inValue);
			break;

		default:
			G_OBJECT_WARN_INVALID_PROPERTY_ID(inObject, inPropID, inSpec);
			break;
	}
}

static void _xfdashboard_model_get_property(GObject *inObject,
											guint inPropID,
											GValue *outValue,
											GParamSpec *inSpec)
{
	XfdashboardModel				*self=XFDASHBOARD_MODEL(inObject);

	switch(inPropID)
	{
		case PROP_ROWS:
			g_value_set_int(outValue, xfdashboard_model_get_rows_count(self));
			break;

		case PROP_SORT_SET:
			g_value_set_boolean(outValue, xfdashboard_model_is_sorted(self));
			break;

		case PROP_FILTER_SET:
			g_value_set_boolean(outValue, xfdashboard_model_is_filtered(self));
			break;

		default:
			G_OBJECT_WARN_INVALID_PROPERTY_ID(inObject, inPropID, inSpec);
			break;
	}
}

/* Class initialization of type XfdashboardModel
 * Override functions in parent classes and define properties
 * and signals
 */
static void xfdashboard_model_class_init(XfdashboardModelClass *klass)
{
	GObjectClass			*gobjectClass=G_OBJECT_CLASS(klass);

	/* Override functions */
	gobjectClass->dispose=_xfdashboard_model_dispose;
	gobjectClass->set_property=_xfdashboard_model_set_property;
	gobjectClass->get_property=_xfdashboard_model_get_property;

	/* Define properties */
	XfdashboardModelProperties[PROP_ROWS]=
		g_param_spec_int("rows",
							_("Rows"),
							_("The number of rows this model contains"),
							0, G_MAXINT,
							0,
							G_PARAM_READABLE | G_PARAM_STATIC_STRINGS);

	XfdashboardModelProperties[PROP_SORT_SET]=
		g_param_spec_boolean("sort-set",
								_("Sort set"),
								_("Whether a sorting function is set or not"),
								FALSE,
								G_PARAM_READABLE | G_PARAM_STATIC_STRINGS);

	XfdashboardModelProperties[PROP_FILTER_SET]=
		g_param_spec_boolean("filter-set",
								_("Filter set"),
								_("Whether a filter is set or not"),
								FALSE,
								G_PARAM_READABLE | G_PARAM_STATIC_STRINGS);

	XfdashboardModelProperties[PROP_FREE_DATA_CALLBACK]=
		g_param_spec_pointer("free-data-callback",
								_("Free data callback"),
								_("Callback function to free data when removing or overwriting data in model"),
								G_PARAM_WRITABLE | G_PARAM_CONSTRUCT_ONLY | G_PARAM_STATIC_STRINGS);

	g_object_class_install_properties(gobjectClass, PROP_LAST, XfdashboardModelProperties);

	/* Define signals */
	XfdashboardModelSignals[SIGNAL_ROW_ADDED]=
		g_signal_new("row-added",
						G_TYPE_FROM_CLASS(klass),
						G_SIGNAL_RUN_LAST,
						G_STRUCT_OFFSET(XfdashboardModelClass, row_added),
						NULL,
						NULL,
						g_cclosure_marshal_VOID__OBJECT,
						G_TYPE_NONE,
						1,
						XFDASHBOARD_TYPE_MODEL_ITER);

	XfdashboardModelSignals[SIGNAL_ROW_REMOVED]=
		g_signal_new("row-removed",
						G_TYPE_FROM_CLASS(klass),
						G_SIGNAL_RUN_LAST,
						G_STRUCT_OFFSET(XfdashboardModelClass, row_removed),
						NULL,
						NULL,
						g_cclosure_marshal_VOID__OBJECT,
						G_TYPE_NONE,
						1,
						XFDASHBOARD_TYPE_MODEL_ITER);

	XfdashboardModelSignals[SIGNAL_ROW_CHANGED]=
		g_signal_new("row-changed",
						G_TYPE_FROM_CLASS(klass),
						G_SIGNAL_RUN_LAST,
						G_STRUCT_OFFSET(XfdashboardModelClass, row_changed),
						NULL,
						NULL,
						g_cclosure_marshal_VOID__OBJECT,
						G_TYPE_NONE,
						1,
						XFDASHBOARD_TYPE_MODEL_ITER);

	XfdashboardModelSignals[SIGNAL_SORT_CHANGED]=
		g_signal_new("sort-changed",
						G_TYPE_FROM_CLASS(klass),
						G_SIGNAL_RUN_LAST,
						G_STRUCT_OFFSET(XfdashboardModelClass, sort_changed),
						NULL,
						NULL,
						g_cclosure_marshal_VOID__VOID,
						G_TYPE_NONE,
						0);

	XfdashboardModelSignals[SIGNAL_FILTER_CHANGED]=
		g_signal_new("filter-changed",
						G_TYPE_FROM_CLASS(klass),
						G_SIGNAL_RUN_LAST,
						G_STRUCT_OFFSET(XfdashboardModelClass, filter_changed),
						NULL,
						NULL,
						g_cclosure_marshal_VOID__VOID,
						G_TYPE_NONE,
						0);
}

/* Object initialization of type XfdashboardModel
 * Create private structure and set up default values
 */
static void xfdashboard_model_init(XfdashboardModel *self)
{
	XfdashboardModelPrivate			*priv;

	priv=self->priv=xfdashboard_model_get_instance_private(self);

	/* Set up default values */
	priv->data=g_sequence_new(NULL);
	priv->freeDataCallback=NULL;

	priv->sortCallback=NULL;
	priv->sortUserData=NULL;
	priv->sortUserDataDestroyCallback=NULL;

	priv->filterCallback=NULL;
	priv->filterUserData=NULL;
	priv->filterUserDataDestroyCallback=NULL;
}

/* Dispose this object of type XfdashboardModelIter */
static void _xfdashboard_model_iter_dispose(GObject *inObject)
{
	XfdashboardModelIter			*self=XFDASHBOARD_MODEL_ITER(inObject);
	XfdashboardModelIterPrivate		*priv=self->priv;

	/* Release our allocated variables */
	if(priv->model)
	{
		g_object_unref(priv->model);
		priv->model=NULL;
	}

	priv->iter=NULL;

	/* Call parent's class dispose method */
	G_OBJECT_CLASS(xfdashboard_model_iter_parent_class)->dispose(inObject);
}

/* Class initialization of type XfdashboardModelIter
 * Override functions in parent classes and define properties
 * and signals
 */
static void xfdashboard_model_iter_class_init(XfdashboardModelIterClass *klass)
{
	GObjectClass			*gobjectClass=G_OBJECT_CLASS(klass);

	/* Override functions */
	gobjectClass->dispose=_xfdashboard_model_iter_dispose;
}

/* Object initialization of type XfdashboardModelIter
 * Create private structure and set up default values
 */
static void xfdashboard_model_iter_init(XfdashboardModelIter *self)
{
	XfdashboardModelIterPrivate		*priv;

	priv=self->priv=xfdashboard_model_iter_get_instance_private(self);

	/* Set up default values */
	priv->model=NULL;
	priv->iter=NULL;
}


/* IMPLEMENTATION: Public API of XfdashboardModel */

/* Model creation functions */
XfdashboardModel* xfdashboard_model_new(void)
{
	GObject		*model;

	model=g_object_new(XFDASHBOARD_TYPE_MODEL, NULL);
	if(!model) return(NULL);

	return(XFDASHBOARD_MODEL(model));
}

/* Return number of rows in this model */
gint xfdashboard_model_get_rows_count(XfdashboardModel *self)
{
	g_return_val_if_fail(XFDASHBOARD_IS_MODEL(self), 0);

	return(g_sequence_get_length(self->priv->data));
}

/* Get item at requested row of this model */
gpointer xfdashboard_model_get(XfdashboardModel *self, gint inRow)
{
	XfdashboardModelPrivate			*priv;
	GSequenceIter					*iter;
	gpointer						item;

	g_return_val_if_fail(XFDASHBOARD_IS_MODEL(self), NULL);
	g_return_val_if_fail(_xfdashboard_model_is_valid_row(self, inRow), NULL);

	priv=self->priv;
	item=NULL;

	/* Get iterator at requested row */
	iter=g_sequence_get_iter_at_pos(priv->data, inRow);
	if(iter)
	{
		/* Get item from iterator */
		item=g_sequence_get(iter);
	}

	/* Return item found */
	return(item);
}

/* Add a new item to end of model's data */
gboolean xfdashboard_model_append(XfdashboardModel *self,
									gpointer inData,
									XfdashboardModelIter **outIter)
{
	XfdashboardModelPrivate			*priv;
	XfdashboardModelIter			*iter;
	GSequenceIter					*seqIter;

	g_return_val_if_fail(XFDASHBOARD_IS_MODEL(self), FALSE);
	g_return_val_if_fail(outIter==NULL || *outIter==NULL, FALSE);

	priv=self->priv;

	/* Append data to model's data */
	seqIter=g_sequence_append(priv->data, inData);

	/* Create iterator for returned sequence iterator */
	iter=xfdashboard_model_iter_new(self);
	iter->priv->iter=seqIter;

	/* Emit signal */
	g_signal_emit(self, XfdashboardModelSignals[SIGNAL_ROW_ADDED], 0, iter);

	/* Store iterator if callee requested it */
	if(outIter) *outIter=XFDASHBOARD_MODEL_ITER(g_object_ref(iter));

	/* Release allocated resources */
	if(iter) g_object_unref(iter);

	/* Return TRUE for success */
	return(TRUE);
}

/* Add a new item to begin of model's data */
gboolean xfdashboard_model_prepend(XfdashboardModel *self,
									gpointer inData,
									XfdashboardModelIter **outIter)
{
	XfdashboardModelPrivate			*priv;
	XfdashboardModelIter			*iter;
	GSequenceIter					*seqIter;

	g_return_val_if_fail(XFDASHBOARD_IS_MODEL(self), FALSE);
	g_return_val_if_fail(outIter==NULL || *outIter==NULL, FALSE);

	priv=self->priv;

	/* Append data to model's data */
	seqIter=g_sequence_prepend(priv->data, inData);

	/* Create iterator for returned sequence iterator */
	iter=xfdashboard_model_iter_new(self);
	iter->priv->iter=seqIter;

	/* Emit signal */
	g_signal_emit(self, XfdashboardModelSignals[SIGNAL_ROW_ADDED], 0, iter);

	/* Store iterator if callee requested it */
	if(outIter) *outIter=XFDASHBOARD_MODEL_ITER(g_object_ref(iter));

	/* Release allocated resources */
	if(iter) g_object_unref(iter);

	/* Return TRUE for success */
	return(TRUE);
}

/* Add a new item at requested row (i.e. before the item at requested row)
 * at model's data.
 */
gboolean xfdashboard_model_insert(XfdashboardModel *self,
									gint inRow,
									gpointer inData,
									XfdashboardModelIter **outIter)
{
	XfdashboardModelPrivate			*priv;
	XfdashboardModelIter			*iter;
	GSequenceIter					*seqIter;
	GSequenceIter					*insertIter;

	g_return_val_if_fail(XFDASHBOARD_IS_MODEL(self), FALSE);
	g_return_val_if_fail(_xfdashboard_model_is_valid_row(self, inRow), FALSE);
	g_return_val_if_fail(outIter==NULL || *outIter==NULL, FALSE);

	priv=self->priv;

	/* Create sequence iterator where to insert new data at */
	insertIter=g_sequence_get_iter_at_pos(priv->data, inRow);

	/* Insert data at "insert iterator" at model's data */
	seqIter=g_sequence_insert_before(insertIter, inData);

	/* Create iterator for returned sequence iterator */
	iter=xfdashboard_model_iter_new(self);
	iter->priv->iter=seqIter;

	/* Emit signal */
	g_signal_emit(self, XfdashboardModelSignals[SIGNAL_ROW_ADDED], 0, iter);

	/* Store iterator if callee requested it */
	if(outIter) *outIter=XFDASHBOARD_MODEL_ITER(g_object_ref(iter));

	/* Release allocated resources */
	if(iter) g_object_unref(iter);

	/* Return TRUE for success */
	return(TRUE);
}

/* Set or replace data at iterator */
gboolean xfdashboard_model_set(XfdashboardModel *self,
								gint inRow,
								gpointer inData,
								XfdashboardModelIter **outIter)
{
	XfdashboardModelPrivate			*priv;
	XfdashboardModelIter			*iter;
	GSequenceIter					*seqIter;

	g_return_val_if_fail(XFDASHBOARD_IS_MODEL(self), FALSE);
	g_return_val_if_fail(_xfdashboard_model_is_valid_row(self, inRow), FALSE);

	priv=self->priv;

	/* Create sequence iterator to row which is set */
	seqIter=g_sequence_get_iter_at_pos(priv->data, inRow);

	/* If a function is provided to free data on removal then call it now */
	if(priv->freeDataCallback)
	{
		gpointer					oldData;

		/* Get data to remove */
		oldData=g_sequence_get(seqIter);

		/* Call function to free data */
		(priv->freeDataCallback)(oldData);
	}

	/* Set new data at iterator */
	g_sequence_set(seqIter, inData);

	/* Create iterator for returned sequence iterator */
	iter=xfdashboard_model_iter_new(self);
	iter->priv->iter=seqIter;

	/* Emit signal */
	g_signal_emit(self, XfdashboardModelSignals[SIGNAL_ROW_CHANGED], 0, iter);

	/* Store iterator if callee requested it */
	if(outIter) *outIter=XFDASHBOARD_MODEL_ITER(g_object_ref(iter));

	/* Release allocated resources */
	if(iter) g_object_unref(iter);

	/* Return TRUE for success */
	return(TRUE);
}

/* Remove data at requested row from model's data */
gboolean xfdashboard_model_remove(XfdashboardModel *self, gint inRow)
{
	XfdashboardModelPrivate			*priv;
	XfdashboardModelIter			*iter;
	GSequenceIter					*seqIter;

	g_return_val_if_fail(XFDASHBOARD_IS_MODEL(self), FALSE);
	g_return_val_if_fail(_xfdashboard_model_is_valid_row(self, inRow), FALSE);

	priv=self->priv;

	/* Create sequence iterator to row which is to remove */
	seqIter=g_sequence_get_iter_at_pos(priv->data, inRow);

	/* Create iterator for returned sequence iterator */
	iter=xfdashboard_model_iter_new(self);
	iter->priv->iter=seqIter;

	/* Emit signal before removal to give signal handlers a changed
	 * to access the data at iterator a last time.
	 */
	g_signal_emit(self, XfdashboardModelSignals[SIGNAL_ROW_REMOVED], 0, iter);

	/* If a function is provided to free data on removal then call it now */
	if(priv->freeDataCallback)
	{
		gpointer					oldData;

		/* Get data to remove */
		oldData=g_sequence_get(seqIter);

		/* Call function to free data */
		(priv->freeDataCallback)(oldData);
	}

	/* Remove data from model's data */
	g_sequence_remove(seqIter);

	/* Release allocated resources */
	if(iter) g_object_unref(iter);

	/* Return TRUE for success */
	return(TRUE);
}

/* Remove all data from model's data */
void xfdashboard_model_remove_all(XfdashboardModel *self)
{
	XfdashboardModelPrivate			*priv;
	XfdashboardModelIter			*iter;

	g_return_if_fail(XFDASHBOARD_IS_MODEL(self));

	priv=self->priv;

	/* Create iterator used to iterate through all items in model's data
	 * and it is used when emitting signal.
	 */
	iter=xfdashboard_model_iter_new(self);
	iter->priv->iter=g_sequence_get_begin_iter(priv->data);

	/* Iterate through all items in model's data, emit signal for each item
	 * being remove and remove them finally. If model provides a function to
	 * free data call it with the item to remove.
	 */
	while(!g_sequence_iter_is_end(iter->priv->iter))
	{
		/* Emit signal before removal to give signal handlers a changed
		 * to access the data at iterator a last time.
		 */
		g_signal_emit(self, XfdashboardModelSignals[SIGNAL_ROW_REMOVED], 0, iter);

		/* If a function is provided to free data on removal then call it now */
		if(priv->freeDataCallback)
		{
			gpointer					oldData;

			/* Get data to remove */
			oldData=g_sequence_get(iter->priv->iter);

			/* Call function to free data */
			(priv->freeDataCallback)(oldData);
		}

		/* Remove data from model's data */
		g_sequence_remove(iter->priv->iter);

		/* Move iterator to next item in model's data */
		iter->priv->iter=g_sequence_iter_next(iter->priv->iter);
	}

	/* Release allocated resources */
	if(iter) g_object_unref(iter);
}

/* Iterate through all items in model's data and call user supplied callback
 * function for each item.
 */
void xfdashboard_model_foreach(XfdashboardModel *self,
								XfdashboardModelForeachFunc inForeachCallback,
								gpointer inUserData)
{
	XfdashboardModelIter			*iter;
	gpointer						item;

	g_return_if_fail(XFDASHBOARD_IS_MODEL(self));
	g_return_if_fail(inForeachCallback);

	/* Iterate through all items in model's data */
	/* Call user supplied callback function */
	iter=xfdashboard_model_iter_new(self);
	while(xfdashboard_model_iter_next(iter))
	{
		/* Get item at position the iterator points to */
		item=xfdashboard_model_iter_get(iter);

		/* Call user supplied callback function */
		(inForeachCallback)(iter, item, inUserData);
	}

	/* Release allocated resources */
	if(iter) g_object_unref(iter);
}

/* Model sort functions */
gboolean xfdashboard_model_is_sorted(XfdashboardModel *self)
{
	XfdashboardModelPrivate			*priv;

	g_return_val_if_fail(XFDASHBOARD_IS_MODEL(self), FALSE);

	priv=self->priv;

	/* If a sort function is set then return TRUE ... */
	if(priv->sortCallback) return(TRUE);

	/* ... otherwise FALSE */
	return(FALSE);
}

/* Set sorting function */
void xfdashboard_model_set_sort(XfdashboardModel *self,
								XfdashboardModelSortFunc inSortCallback,
								gpointer inUserData,
								GDestroyNotify inUserDataDestroyCallback)
{
	XfdashboardModelPrivate			*priv;

	g_return_if_fail(XFDASHBOARD_IS_MODEL(self));

	priv=self->priv;

	/* Set value if changed */
	if(priv->sortCallback!=inSortCallback ||
		priv->sortUserData!=inUserData ||
		priv->sortUserDataDestroyCallback!=inUserDataDestroyCallback)
	{
		gboolean				oldSortIsSet;
		gboolean				newSortIsSet;

		/* Get old "sort-set" value. It is used later to determine if this
		 * property has changed also.
		 */
		oldSortIsSet=xfdashboard_model_is_sorted(self);

		/* Release old values */
		if(priv->sortUserData &&
			priv->sortUserDataDestroyCallback)
		{
			(priv->sortUserDataDestroyCallback)(priv->sortUserData);
		}
		priv->sortUserDataDestroyCallback=NULL;
		priv->sortUserData=NULL;
		priv->sortCallback=NULL;

		/* Set value */
		priv->sortCallback=inSortCallback;
		priv->sortUserData=inUserData;
		priv->sortUserDataDestroyCallback=inUserDataDestroyCallback;

		/* Get new "sort-set" value to determine if this property has
		 * changed also.
		 */
		newSortIsSet=xfdashboard_model_is_sorted(self);

		/* Sort model if sort function is set */
		if(newSortIsSet) xfdashboard_model_resort(self);

		/* Notify about change of 'sort-set' if changed and after model
		 * was sorted.
		 */
		if(oldSortIsSet!=newSortIsSet)
		{
			g_object_notify_by_pspec(G_OBJECT(self), XfdashboardModelProperties[PROP_SORT_SET]);
		}

		/* Emit signal that sorting has changed */
		g_signal_emit(self, XfdashboardModelSignals[SIGNAL_SORT_CHANGED], 0);
	}
}

/* Resort this model's data with sorting function set */
void xfdashboard_model_resort(XfdashboardModel *self)
{
	XfdashboardModelPrivate			*priv;
	XfdashboardModelSortData		sortData;

	g_return_if_fail(XFDASHBOARD_IS_MODEL(self));

	priv=self->priv;

	/* If no sort function is set return immediately because this model
	 * cannot be sorted without a sort function.
	 */
	if(!priv->sortCallback) return;

	/* Set up sort data which wraps all needed information into a structure.
	 * The interators are pre-initialized and updated in internal sort function
	 * which is passed to GSequence's sort function. This internal callback
	 * only updates the existing iterators to reduce creation and destructions
	 * of these iterator. This can be done because the model does not change
	 * while sorting.
	 */
	sortData.model=XFDASHBOARD_MODEL(g_object_ref(self));
	sortData.leftIter=xfdashboard_model_iter_new(self);
	sortData.rightIter=xfdashboard_model_iter_new(self);

	/* Sort this model again by using internal sorting function which
	 * calls user's sort function with adjusted parameters.
	 */
	g_sequence_sort_iter(priv->data, _xfdashboard_model_sort_internal, &sortData);

	/* Release allocated resources */
	if(sortData.model) g_object_unref(sortData.model);
	if(sortData.leftIter) g_object_unref(sortData.leftIter);
	if(sortData.rightIter) g_object_unref(sortData.rightIter);
}

/* Model filter functions */
gboolean xfdashboard_model_is_filtered(XfdashboardModel *self)
{
	XfdashboardModelPrivate			*priv;

	g_return_val_if_fail(XFDASHBOARD_IS_MODEL(self), FALSE);

	priv=self->priv;

	/* If a filter function is set then return TRUE ... */
	if(priv->filterCallback) return(TRUE);

	/* ... otherwise FALSE */
	return(FALSE);
}

/* Set filter function */
void xfdashboard_model_set_filter(XfdashboardModel *self,
									XfdashboardModelFilterFunc inFilterCallback,
									gpointer inUserData,
									GDestroyNotify inUserDataDestroyCallback)
{
	XfdashboardModelPrivate			*priv;

	g_return_if_fail(XFDASHBOARD_IS_MODEL(self));

	priv=self->priv;

	/* Set value if changed */
	if(priv->filterCallback!=inFilterCallback ||
		priv->filterUserData!=inUserData ||
		priv->filterUserDataDestroyCallback!=inUserDataDestroyCallback)
	{
		gboolean				oldFilterIsSet;
		gboolean				newFilterIsSet;

		/* Get old "filter-set" value. It is used later to determine if this
		 * property has changed also.
		 */
		oldFilterIsSet=xfdashboard_model_is_filtered(self);

		/* Release old values */
		if(priv->filterUserData &&
			priv->filterUserDataDestroyCallback)
		{
			(priv->filterUserDataDestroyCallback)(priv->filterUserData);
		}
		priv->filterUserDataDestroyCallback=NULL;
		priv->filterUserData=NULL;
		priv->filterCallback=NULL;

		/* Set value */
		priv->filterCallback=inFilterCallback;
		priv->filterUserData=inUserData;
		priv->filterUserDataDestroyCallback=inUserDataDestroyCallback;

		/* Get new "sort-set" value to determine if this property has
		 * changed also.
		 */
		newFilterIsSet=xfdashboard_model_is_filtered(self);

		/* Notify about change of 'sort-set' if changed and after model
		 * was sorted.
		 */
		if(oldFilterIsSet!=newFilterIsSet)
		{
			g_object_notify_by_pspec(G_OBJECT(self), XfdashboardModelProperties[PROP_FILTER_SET]);
		}

		/* Emit signal that filter has changed */
		g_signal_emit(self, XfdashboardModelSignals[SIGNAL_FILTER_CHANGED], 0);
	}
}

/* Check if requested row is filtered */
gboolean xfdashboard_model_filter_row(XfdashboardModel *self, gint inRow)
{
	XfdashboardModelPrivate			*priv;
	XfdashboardModelIter			*iter;
	gboolean						isVisible;

	g_return_val_if_fail(XFDASHBOARD_IS_MODEL(self), FALSE);
	g_return_val_if_fail(_xfdashboard_model_is_valid_row(self, inRow), FALSE);

	priv=self->priv;
	isVisible=TRUE;

	/* Call user supplied filter callback function to determine if this
	 * row should be filtered or not but only if filter function is set.
	 */
	if(priv->filterCallback)
	{
		/* Create iterator */
		iter=xfdashboard_model_iter_new_for_row(self, inRow);

		/* Determine if row is filtered */
		isVisible=(priv->filterCallback)(iter, priv->filterUserData);

		/* Release allocated resources */
		if(iter) g_object_unref(iter);
	}

	/* Return filter status */
	return(isVisible);
}

/* Create iterator for model */
XfdashboardModelIter* xfdashboard_model_iter_new(XfdashboardModel *inModel)
{
	XfdashboardModelIter			*iter;

	g_return_val_if_fail(XFDASHBOARD_IS_MODEL(inModel), NULL);

	/* Create iterator */
	iter=XFDASHBOARD_MODEL_ITER(g_object_new(XFDASHBOARD_TYPE_MODEL_ITER, NULL));

	/* Set up iterator */
	iter->priv->model=XFDASHBOARD_MODEL(g_object_ref(inModel));
	iter->priv->iter=NULL;

	/* Return newly created iterator */
	return(iter);
}

/* Create iterator for model at requested row */
XfdashboardModelIter* xfdashboard_model_iter_new_for_row(XfdashboardModel *inModel, gint inRow)
{
	XfdashboardModelIter			*iter;
	XfdashboardModelPrivate			*modelPriv;

	g_return_val_if_fail(XFDASHBOARD_IS_MODEL(inModel), NULL);
	g_return_val_if_fail(_xfdashboard_model_is_valid_row(inModel, inRow), NULL);

	modelPriv=inModel->priv;

	/* Create iterator */
	iter=XFDASHBOARD_MODEL_ITER(g_object_new(XFDASHBOARD_TYPE_MODEL_ITER, NULL));

	/* Set up iterator */
	iter->priv->model=XFDASHBOARD_MODEL(g_object_ref(inModel));
	iter->priv->iter=g_sequence_get_iter_at_pos(modelPriv->data, inRow);

	/* Return newly created iterator */
	return(iter);
}

/* Create copy of an iterator */
XfdashboardModelIter* xfdashboard_model_iter_copy(XfdashboardModelIter *self)
{
	XfdashboardModelIterPrivate		*priv;
	XfdashboardModelIter			*copyIter;

	g_return_val_if_fail(XFDASHBOARD_IS_MODEL_ITER(self), NULL);

	priv=self->priv;

	/* Create iterator */
	copyIter=XFDASHBOARD_MODEL_ITER(g_object_new(XFDASHBOARD_TYPE_MODEL_ITER, NULL));

	/* Set up iterator to be a copy of given iterator */
	copyIter->priv->model=XFDASHBOARD_MODEL(g_object_ref(priv->model));
	copyIter->priv->iter=priv->iter;

	/* Return copy of iterator */
	return(copyIter);
}

/* Move iterator to next item in model's data */
gboolean xfdashboard_model_iter_next(XfdashboardModelIter *self)
{
	XfdashboardModelIterPrivate		*priv;
	XfdashboardModelPrivate			*modelPriv;
	GSequenceIter					*newIter;

	g_return_val_if_fail(_xfdashboard_model_iter_is_valid(self, FALSE), FALSE);

	priv=self->priv;
	modelPriv=priv->model->priv;

	/* If no iterator was initialized then create an iterator pointing
	 * to begin of model's data ...
	 */
	if(!priv->iter)
	{
		/* Get and set iterator pointing to begin of model's data */
		newIter=g_sequence_get_begin_iter(modelPriv->data);
	}
		/* ... otherwise move iterator to next item in model's data */
		else
		{
			/* Move iterator to next item in model's data */
			newIter=g_sequence_iter_next(priv->iter);
		}

	/* If iterator is invalid or end of model's data is reached
	 * return FALSE here.
	 */
	if(!newIter ||
		g_sequence_iter_is_end(newIter))
	{
		return(FALSE);
	}

	/* New iterator is valid so set it */
	priv->iter=newIter;

	/* If we get here then all went well and we can return TRUE */
	return(TRUE);
}

/* Move iterator to previous item in model's data */
gboolean xfdashboard_model_iter_prev(XfdashboardModelIter *self)
{
	XfdashboardModelIterPrivate		*priv;
	XfdashboardModelPrivate			*modelPriv;
	GSequenceIter					*newIter;

	g_return_val_if_fail(_xfdashboard_model_iter_is_valid(self, FALSE), FALSE);

	priv=self->priv;
	modelPriv=priv->model->priv;

	/* If no iterator was initialized then create an iterator pointing
	 * to end of model's data ...
	 */
	if(!priv->iter)
	{
		/* Get and set iterator pointing to end of model's data */
		newIter=g_sequence_get_end_iter(modelPriv->data);
	}
		/* ... otherwise move iterator to previous item in model's data */
		else
		{
			/* Move iterator to next item in model's data */
			newIter=g_sequence_iter_prev(priv->iter);
		}

	/* If iterator is invalid or begin of model's data is reached
	 * return FALSE here.
	 */
	if(!newIter ||
		g_sequence_iter_is_begin(newIter))
	{
		return(FALSE);
	}

	/* New iterator is valid so set it */
	priv->iter=newIter;

	/* If we get here then all went well and we can return TRUE */
	return(TRUE);
}

/* Move iterator to requested row in model's data */
gboolean xfdashboard_model_iter_move_to_row(XfdashboardModelIter *self, gint inRow)
{
	XfdashboardModelIterPrivate		*priv;
	XfdashboardModelPrivate			*modelPriv;

	g_return_val_if_fail(_xfdashboard_model_iter_is_valid(self, FALSE), FALSE);

	priv=self->priv;
	modelPriv=priv->model->priv;

	/* Check that requested row is within range */
	g_return_val_if_fail(_xfdashboard_model_is_valid_row(priv->model, inRow), FALSE);

	/* Move iterator to requested row */
	priv->iter=g_sequence_get_iter_at_pos(modelPriv->data, inRow);

	/* If we get here then all went well and we can return TRUE */
	return(TRUE);
}

/* Get model to which this iterator belongs to */
XfdashboardModel* xfdashboard_model_iter_get_model(XfdashboardModelIter *self)
{
	g_return_val_if_fail(_xfdashboard_model_iter_is_valid(self, FALSE), FALSE);

	return(self->priv->model);
}

/* Get row at model's data this iterator points to currently */
guint xfdashboard_model_iter_get_row(XfdashboardModelIter *self)
{
	XfdashboardModelIterPrivate		*priv;
	gint							position;

	g_return_val_if_fail(_xfdashboard_model_iter_is_valid(self, TRUE), 0);

	priv=self->priv;

	/* Get position from iterator */
	position=g_sequence_iter_get_position(priv->iter);
	if(position<0) position=0;

	/* Return position (maybe corrected for unsigned integer) */
	return((guint)position);
}

/* Get item at position and model of this iterator */
gpointer xfdashboard_model_iter_get(XfdashboardModelIter *self)
{
	XfdashboardModelIterPrivate		*priv;

	g_return_val_if_fail(_xfdashboard_model_iter_is_valid(self, TRUE), NULL);

	priv=self->priv;

	/* Get item of model this iterator belongs to */
	return(g_sequence_get(priv->iter));
}

/* Set or replace data at iterator */
gboolean xfdashboard_model_iter_set(XfdashboardModelIter *self, gpointer inData)
{
	XfdashboardModelIterPrivate		*priv;
	XfdashboardModelPrivate			*modelPriv;

	g_return_val_if_fail(_xfdashboard_model_iter_is_valid(self, TRUE), FALSE);

	priv=self->priv;
	modelPriv=priv->model->priv;

	/* If a function at model is provided to free data on removal
	 * then call it now.
	 */
	if(modelPriv->freeDataCallback)
	{
		gpointer					oldData;

		/* Get data to remove */
		oldData=g_sequence_get(priv->iter);

		/* Call function to free data */
		(modelPriv->freeDataCallback)(oldData);
	}

	/* Set new data at iterator */
	g_sequence_set(priv->iter, inData);

	/* Emit signal */
	g_signal_emit(self, XfdashboardModelSignals[SIGNAL_ROW_CHANGED], 0, self);

	/* Return TRUE for success */
	return(TRUE);
}

/* Remove data at iterator */
gboolean xfdashboard_model_iter_remove(XfdashboardModelIter *self)
{
	XfdashboardModelIterPrivate		*priv;
	XfdashboardModelPrivate			*modelPriv;

	g_return_val_if_fail(_xfdashboard_model_iter_is_valid(self, TRUE), FALSE);

	priv=self->priv;
	modelPriv=priv->model->priv;

	/* Emit signal before removal to give signal handlers a changed
	 * to access the data at iterator a last time.
	 */
	g_signal_emit(self, XfdashboardModelSignals[SIGNAL_ROW_REMOVED], 0, self);

	/* If a function at model is provided to free data on removal
	 * then call it now.
	 */
	if(modelPriv->freeDataCallback)
	{
		gpointer					oldData;

		/* Get data to remove */
		oldData=g_sequence_get(priv->iter);

		/* Call function to free data */
		(modelPriv->freeDataCallback)(oldData);
	}

	/* Remove data from model's data */
	g_sequence_remove(priv->iter);

	/* Return TRUE for success */
	return(TRUE);
}

/* Check if row is filtered to which this iterator is pointing to */
gboolean xfdashboard_model_iter_filter(XfdashboardModelIter *self)
{
	XfdashboardModelIterPrivate		*priv;
	XfdashboardModelPrivate			*modelPriv;
	gboolean						isVisible;

	g_return_val_if_fail(_xfdashboard_model_iter_is_valid(self, TRUE), FALSE);

	priv=self->priv;
	modelPriv=priv->model->priv;
	isVisible=TRUE;

	/* Call user supplied filter callback function to determine if this
	 * row should be filtered or not but only if filter function is set.
	 */
	if(modelPriv->filterCallback)
	{
		/* Determine if row is filtered */
		isVisible=(modelPriv->filterCallback)(self, modelPriv->filterUserData);
	}

	/* Return filter status */
	return(isVisible);
}
