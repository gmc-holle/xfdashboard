/*
 * recently-used-search-provider: A search provider using GTK+ recent
 *   manager as search source
 * 
 * Copyright 2012-2022 Stephan Haller <nomad@froevel.de>
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

#include "recently-used-search-provider.h"

#include <glib/gi18n-lib.h>
#include <gtk/gtk.h>


/* Define this class in GObject system */
struct _XfdashboardRecentlyUsedSearchProviderPrivate
{
	/* Instance related */
	GtkRecentManager		*recentManager;
};

G_DEFINE_DYNAMIC_TYPE_EXTENDED(XfdashboardRecentlyUsedSearchProvider,
								xfdashboard_recently_used_search_provider,
								XFDASHBOARD_TYPE_SEARCH_PROVIDER,
								0,
								G_ADD_PRIVATE_DYNAMIC(XfdashboardRecentlyUsedSearchProvider))

/* Define this class in this plugin */
XFDASHBOARD_DEFINE_PLUGIN_TYPE(xfdashboard_recently_used_search_provider);


/* IMPLEMENTATION: Private variables and methods */

/* Check if given recent info matches search terms and return score as fraction
 * between 0.0and 1.0 - so called "relevance". A negative score means that
 * the given recent info does not match at all.
 */
static gfloat _xfdashboard_recently_used_search_provider_score(XfdashboardRecentlyUsedSearchProvider *self,
																gchar **inSearchTerms,
																GtkRecentInfo *inInfo)
{
	G_GNUC_UNUSED XfdashboardRecentlyUsedSearchProviderPrivate		*priv;
	gchar															*title;
	gchar															*description;
	const gchar														*uri;
	const gchar														*value;
	gint															matchesFound, matchesExpected;
	gfloat															pointsSearch;
	gfloat															score;

	g_return_val_if_fail(XFDASHBOARD_IS_RECENTLY_USED_SEARCH_PROVIDER(self), -1.0f);
	g_return_val_if_fail(inInfo, -1.0f);

	priv=self->priv;
	score=-1.0f;

	/* Empty search term matches no menu item */
	if(!inSearchTerms) return(0.0f);

	matchesExpected=g_strv_length(inSearchTerms);
	if(matchesExpected==0) return(0.0f);

	/* Compare available data of recent info with search terms */
	value=gtk_recent_info_get_display_name(inInfo);
	if(value) title=g_utf8_strdown(value, -1);
		else title=NULL;

	value=gtk_recent_info_get_description(inInfo);
	if(value) description=g_utf8_strdown(value, -1);
		else description=NULL;

	uri=gtk_recent_info_get_uri(inInfo);

	matchesFound=0;
	pointsSearch=0.0f;
	while(*inSearchTerms)
	{
		gboolean											termMatch;
		gchar												*uriPos;
		gfloat												pointsTerm;

		/* Reset "found" indicator and score of current search term */
		termMatch=FALSE;
		pointsTerm=0.0f;

		/* Check for current search term */
		if(title &&
			g_strstr_len(title, -1, *inSearchTerms))
		{
			pointsTerm+=0.5;
			termMatch=TRUE;
		}

		if(uri)
		{
			uriPos=g_strstr_len(uri, -1, *inSearchTerms);
			if(uriPos &&
				(uriPos==uri || *(uriPos-1)==G_DIR_SEPARATOR))
			{
				pointsTerm+=0.35;
				termMatch=TRUE;
			}
		}

		if(description &&
			g_strstr_len(description, -1, *inSearchTerms))
		{
			pointsTerm+=0.15;
			termMatch=TRUE;
		}

		/* Increase match counter if we found a match */
		if(termMatch)
		{
			matchesFound++;
			pointsSearch+=pointsTerm;
		}

		/* Continue with next search term */
		inSearchTerms++;
	}

	/* If we got a match in either title, description or command for each search term
	 * then calculate score and also check if we should take the number of
	 * launches of this application into account.
	 */
	if(matchesFound>=matchesExpected)
	{
		gfloat												currentPoints;
		gfloat												maxPoints;

		currentPoints=0.0f;
		maxPoints=0.0f;

		/* Set maximum points to the number of expected number of matches */
		currentPoints+=pointsSearch;
		maxPoints+=matchesExpected*1.0f;

		/* Calculate score but if maximum points is still zero we should do a simple
		 * match by setting score to 1.
		 */
		if(maxPoints>0.0f) score=currentPoints/maxPoints;
			else score=1.0f;
	}

	/* Release allocated resources */
	if(description) g_free(description);
	if(title) g_free(title);

	/* Return score of this application for requested search terms */
	return(score);
}

/* Callback to sort each item in result set */
static gint _xfdashboard_recently_used_search_provider_sort_result_set(GVariant *inLeft,
																		GVariant *inRight,
																		gpointer inUserData)
{
	XfdashboardRecentlyUsedSearchProvider			*self;
	XfdashboardRecentlyUsedSearchProviderPrivate	*priv;
	const gchar										*leftURI;
	const gchar										*rightURI;
	GtkRecentInfo									*leftInfo;
	GtkRecentInfo									*rightInfo;
	const gchar										*leftName;
	const gchar										*rightName;
	gchar											*lowerLeftName;
	gchar											*lowerRightName;
	gint											result;

	g_return_val_if_fail(inLeft, 0);
	g_return_val_if_fail(inRight, 0);
	g_return_val_if_fail(XFDASHBOARD_IS_RECENTLY_USED_SEARCH_PROVIDER(inUserData), 0);

	self=XFDASHBOARD_RECENTLY_USED_SEARCH_PROVIDER(inUserData);
	priv=self->priv;

	/* Get URI of both items */
	leftURI=g_variant_get_string(inLeft, NULL);
	rightURI=g_variant_get_string(inRight, NULL);

	/* Get data of both recent items */
	leftInfo=gtk_recent_manager_lookup_item(priv->recentManager, leftURI, NULL);
	if(leftInfo) leftName=gtk_recent_info_get_display_name(leftInfo);
		else leftName=NULL;

	rightInfo=gtk_recent_manager_lookup_item(priv->recentManager, rightURI, NULL);
	if(rightInfo) rightName=gtk_recent_info_get_display_name(rightInfo);
		else rightName=NULL;

	/* Get result of comparing both desktop application information objects */
	if(leftName) lowerLeftName=g_utf8_strdown(leftName, -1);
		else lowerLeftName=NULL;

	if(rightName) lowerRightName=g_utf8_strdown(rightName, -1);
		else lowerRightName=NULL;

	result=g_strcmp0(lowerLeftName, lowerRightName);

	/* Release allocated resources */
	if(rightInfo) gtk_recent_info_unref(rightInfo);
	if(leftInfo) gtk_recent_info_unref(leftInfo);
	if(lowerLeftName) g_free(lowerLeftName);
	if(lowerRightName) g_free(lowerRightName);

	/* Return result */
	return(result);
}

/* IMPLEMENTATION: XfdashboardSearchProvider */

/* One-time initialization of search provider */
static void _xfdashboard_recently_used_search_provider_initialize(XfdashboardSearchProvider *inProvider)
{
	g_return_if_fail(XFDASHBOARD_IS_RECENTLY_USED_SEARCH_PROVIDER(inProvider));

	/* Initialize search provider */
	/* TODO: Here the search provider is initialized. This function is only called once after
	 *       the search provider was enabled.
	 */
}

/* Get display name for this search provider */
static const gchar* _xfdashboard_recently_used_search_provider_get_name(XfdashboardSearchProvider *inProvider)
{
	return(_("Recently used"));
}

/* Get icon-name for this search provider */
static const gchar* _xfdashboard_recently_used_search_provider_get_icon(XfdashboardSearchProvider *inProvider)
{
	return("document-open-recent");
}

/* Get result set for requested search terms */
static XfdashboardSearchResultSet* _xfdashboard_recently_used_search_provider_get_result_set(XfdashboardSearchProvider *inProvider,
																								const gchar **inSearchTerms,
																								XfdashboardSearchResultSet *inPreviousResultSet)
{
	XfdashboardRecentlyUsedSearchProvider			*self;
	XfdashboardRecentlyUsedSearchProviderPrivate	*priv;
	XfdashboardSearchResultSet						*resultSet;
	guint											numberTerms;
	gchar											**terms, **termsIter;
	GList											*recent, *iter;
	GtkRecentInfo									*info;
	gfloat											score;

	g_return_val_if_fail(XFDASHBOARD_IS_RECENTLY_USED_SEARCH_PROVIDER(inProvider), NULL);

	self=XFDASHBOARD_RECENTLY_USED_SEARCH_PROVIDER(inProvider);
	priv=self->priv;

	/* To perform case-insensitive searches, convert all search terms to lower-case
	 * before starting search. Remember that string list must be NULL terminated.
	 */
	numberTerms=g_strv_length((gchar**)inSearchTerms);
	if(numberTerms==0)
	{
		/* If we get here no search term is given, return no result set */
		return(NULL);
	}

	terms=g_new(gchar*, numberTerms+1);
	if(!terms)
	{
		g_critical("Could not allocate memory to copy search criteria for case-insensitive search");
		return(NULL);
	}

	termsIter=terms;
	while(*inSearchTerms)
	{
		*termsIter=g_utf8_strdown(*inSearchTerms, -1);

		/* Move to next entry where to store lower-case and
		 * initialize with NULL for NULL termination of list.
		 */
		termsIter++;
		*termsIter=NULL;

		/* Move to next search term to convert to lower-case */
		inSearchTerms++;
	}

	/* Create empty result set to store matching result items */
	resultSet=xfdashboard_search_result_set_new();

	/* Perform search by iteratint through recently used files from GTK+ recent
	 * manager and lookup for matches against search terms.
	 */
	recent=gtk_recent_manager_get_items(priv->recentManager);
	for(iter=recent; iter; iter=g_list_next(iter))
	{
		/* Get iterated recent entry to check for match*/
		info=(GtkRecentInfo*)(iter->data);
		if(!info) continue;

		/* Check for a match against search terms */
		score=_xfdashboard_recently_used_search_provider_score(self, terms, info);
		if(score>=0.0f)
		{
			GVariant									*resultItem;

			/* Create result item */
			resultItem=g_variant_new_string(gtk_recent_info_get_uri(info));

			/* Add result item to result set */
			xfdashboard_search_result_set_add_item(resultSet, resultItem);
			xfdashboard_search_result_set_set_item_score(resultSet, resultItem, score);
		}
	}

	/* Sort result set */
	xfdashboard_search_result_set_set_sort_func_full(resultSet,
														_xfdashboard_recently_used_search_provider_sort_result_set,
														g_object_ref(self),
														g_object_unref);


	/* Release allocated resources */
	if(recent) g_list_free_full(recent, (GDestroyNotify)gtk_recent_info_unref);

	if(terms)
	{
		termsIter=terms;
		while(*termsIter)
		{
			g_free(*termsIter);
			termsIter++;
		}
		g_free(terms);
	}

	/* Return result set */
	return(resultSet);
}

/* Create actor for a result item of the result set returned from a search request */
static ClutterActor* _xfdashboard_recently_used_search_provider_create_result_actor(XfdashboardSearchProvider *inProvider,
																				GVariant *inResultItem)
{
	XfdashboardRecentlyUsedSearchProvider			*self;
	XfdashboardRecentlyUsedSearchProviderPrivate	*priv;
	GtkRecentInfo									*info;
	const gchar										*uri;
	const gchar										*name;
	const gchar										*description;
	gchar											*titleDescription;
	GIcon											*icon;
	ClutterActor									*actor;
	gchar											*title;
	GError											*error;

	g_return_val_if_fail(XFDASHBOARD_IS_RECENTLY_USED_SEARCH_PROVIDER(inProvider), NULL);
	g_return_val_if_fail(inResultItem, NULL);

	self=XFDASHBOARD_RECENTLY_USED_SEARCH_PROVIDER(inProvider);
	priv=self->priv;
	error=NULL;

	/* Get URI as it is the ID for lookups etc. */
	uri=g_variant_get_string(inResultItem, NULL);

	/* Get recent data for result item */
	info=gtk_recent_manager_lookup_item(priv->recentManager, uri, &error);
	if(!info)
	{
		g_warning("Cannot create actor for recent item '%s' in result set of %s: %s",
					uri,
					G_OBJECT_TYPE_NAME(inProvider),
					error ? error->message : "<unknown>");

		/* Release allocated resources */
		if(error) g_error_free(error);

		return(NULL);
	}

	/* Collect data to create actor */
	name=gtk_recent_info_get_display_name(info);
	icon=gtk_recent_info_get_gicon(info);

	description=gtk_recent_info_get_description(info);
	if(!description)
	{
		const gchar									*mimeType;
		gchar										*contentType;

		mimeType=gtk_recent_info_get_mime_type(info);
		contentType=g_content_type_from_mime_type(mimeType);
		if(contentType)
		{
			titleDescription=g_content_type_get_description(contentType);
			g_free(contentType);
		}
			else titleDescription=g_strdup(mimeType);
	}
		else titleDescription=g_strdup(description);

	/* Create actor for result item */
	title=g_markup_printf_escaped("<b>%s</b>\n<small><i>%s</i></small>\n\n%s",
									name,
									uri,
									titleDescription);
	actor=xfdashboard_button_new_full_with_gicon(icon, title);
	g_free(title);

	/* Release allocated resources */
	if(titleDescription) g_free(titleDescription);
	if(icon) g_object_unref(icon);
	if(info) gtk_recent_info_unref(info);

	/* Return created actor */
	return(actor);
}

/* Activate result item */
static gboolean _xfdashboard_recently_used_search_provider_activate_result(XfdashboardSearchProvider* inProvider,
																			GVariant *inResultItem,
																			ClutterActor *inActor,
																			const gchar **inSearchTerms)
{
	XfdashboardRecentlyUsedSearchProvider			*self;
	XfdashboardRecentlyUsedSearchProviderPrivate	*priv;
	GtkRecentInfo									*info;
	const gchar										*uri;
	GList											*files;
	const gchar										*mimeType;
	gchar											*contentType;
	gboolean										started;
	GAppInfo										*appInfo;
	GAppLaunchContext								*context;
	GIcon											*appGIcon;
	gchar											*appIconName;
	GError											*error;

	g_return_val_if_fail(XFDASHBOARD_IS_RECENTLY_USED_SEARCH_PROVIDER(inProvider), FALSE);
	g_return_val_if_fail(inResultItem, FALSE);

	self=XFDASHBOARD_RECENTLY_USED_SEARCH_PROVIDER(inProvider);
	priv=self->priv;
	error=NULL;

	/* Get URI as it is the ID for lookups etc. */
	uri=g_variant_get_string(inResultItem, NULL);

	/* Get recent data for result item */
	info=gtk_recent_manager_lookup_item(priv->recentManager, uri, &error);
	if(!info)
	{
		/* Notify user about failure */
		xfdashboard_notify(NULL,
							"dialog-error",
							_("Launching application for '%s' failed: %s"),
							uri,
							error ? error->message : "unknown" );
		g_warning("Could not get recent info for file '%s' failed: %s",
					uri,
					(error && error->message) ? error->message : _("unknown error"));

		/* Release allocated resources */
		if(error) g_error_free(error);

		/* Return FALSE as activation failed */
		return(FALSE);
	}

	/* Get mime and content type of result item */
	mimeType=gtk_recent_info_get_mime_type(info);
	contentType=g_content_type_from_mime_type(mimeType);
	if(!contentType)
	{
		/* Notify user about failure */
		xfdashboard_notify(NULL,
							"dialog-error",
							_("Launching application for file '%s' failed: %s"),
							gtk_recent_info_get_display_name(info),
							_("No information available for application"));
		g_warning("Could not get content-type for mime-type '%s' of file '%s'",
					mimeType ? mimeType : "<unknown>",
					uri);

		/* Release allocated resources */
		if(info) gtk_recent_info_unref(info);

		/* Return FALSE as activation failed */
		return(FALSE);
	}

	/* Get default application for content type of result item */
	appInfo=g_app_info_get_default_for_type(contentType, TRUE);
	if(!appInfo)
	{
		/* Notify user about failure */
		xfdashboard_notify(NULL,
							"dialog-error",
							_("Launching application for file '%s' failed: %s"),
							gtk_recent_info_get_display_name(info),
							_("No information available for application"));
		g_warning("Could not get default application for file '%s' of mime-type '%s' and content-type '%s'",
					uri,
					mimeType,
					contentType);

		/* Release allocated resources */
		if(contentType) g_free(contentType);
		if(info) gtk_recent_info_unref(info);

		/* Return FALSE as activation failed */
		return(FALSE);
	}

	/* Get icon name of application if not ambiguous */
	appGIcon=g_app_info_get_icon(appInfo);

	appIconName=NULL;
	if(!appIconName &&
		appGIcon &&
		G_IS_FILE_ICON(appGIcon))
	{
		GFile				*iconFile;

		/* Get file object of icon*/
		iconFile=g_file_icon_get_file(G_FILE_ICON(appGIcon));
		if(iconFile)
		{
			/* Get file name of icon */
			appIconName=g_file_get_path(iconFile);
		}
	}

	if(!appIconName &&
		appGIcon &&
		G_IS_THEMED_ICON(appGIcon))
	{
		const gchar* const	*iconNames;

		iconNames=g_themed_icon_get_names(G_THEMED_ICON(appGIcon));
		if(iconNames &&
			g_strv_length((gchar**)iconNames)>0)
		{
			/* Get icon-name */
			appIconName=g_strdup(*iconNames);
		}
	}

	if(appGIcon) g_object_unref(appGIcon);

	/* Build file list to pass to application when launched */
	files=g_list_prepend(NULL, g_file_new_for_uri(uri));

	/* Activate result item */
	started=FALSE;

	context=xfdashboard_create_app_context(NULL);
	if(!g_app_info_launch(appInfo, files, context, &error))
	{
		/* Show notification about failed application launch */
		xfdashboard_notify(NULL,
							appIconName,
							_("Launching application '%s' failed: %s"),
							g_app_info_get_display_name(appInfo),
							(error && error->message) ? error->message : _("unknown error"));
		g_warning("Launching application '%s' for file '%s' failed: %s",
					g_app_info_get_display_name(appInfo),
					gtk_recent_info_get_display_name(info),
					(error && error->message) ? error->message : "unknown error");
		if(error) g_error_free(error);
	}
		else
		{
			/* Show notification about successful application launch */
			xfdashboard_notify(NULL,
								appIconName,
								_("Application '%s' launched"),
								g_app_info_get_display_name(appInfo));
			XFDASHBOARD_DEBUG(self, PLUGINS,
								"Application '%s' launched for file URI '%s'",
								g_app_info_get_display_name(appInfo),
								uri);

			/* Emit signal for successful application launch */
			g_signal_emit_by_name(xfdashboard_core_get_default(), "application-launched", appInfo);

			/* Set status that application has been started successfully */
			started=TRUE;
		}

	/* Release allocated resources */
	if(files) g_list_free_full(files, g_object_unref);
	if(appIconName) g_free(appIconName);
	if(context) g_object_unref(context);
	if(appInfo) g_object_unref(appInfo);
	if(contentType) g_free(contentType);
	if(info) gtk_recent_info_unref(info);

	/* Return status */
	return(started);
}


/* IMPLEMENTATION: GObject */

/* Dispose this object */
static void _xfdashboard_recently_used_search_provider_dispose(GObject *inObject)
{
	XfdashboardRecentlyUsedSearchProvider			*self=XFDASHBOARD_RECENTLY_USED_SEARCH_PROVIDER(inObject);
	XfdashboardRecentlyUsedSearchProviderPrivate	*priv=self->priv;

	/* Release allocated resources */
	if(priv->recentManager)
	{
		g_object_unref(priv->recentManager);
		priv->recentManager=NULL;
	}

	/* Call parent's class dispose method */
	G_OBJECT_CLASS(xfdashboard_recently_used_search_provider_parent_class)->dispose(inObject);
}

/* Class initialization
 * Override functions in parent classes and define properties
 * and signals
 */
void xfdashboard_recently_used_search_provider_class_init(XfdashboardRecentlyUsedSearchProviderClass *klass)
{
	XfdashboardSearchProviderClass	*providerClass=XFDASHBOARD_SEARCH_PROVIDER_CLASS(klass);
	GObjectClass					*gobjectClass=G_OBJECT_CLASS(klass);

	/* Override functions */
	gobjectClass->dispose=_xfdashboard_recently_used_search_provider_dispose;

	providerClass->initialize=_xfdashboard_recently_used_search_provider_initialize;
	providerClass->get_icon=_xfdashboard_recently_used_search_provider_get_icon;
	providerClass->get_name=_xfdashboard_recently_used_search_provider_get_name;
	providerClass->get_result_set=_xfdashboard_recently_used_search_provider_get_result_set;
	providerClass->create_result_actor=_xfdashboard_recently_used_search_provider_create_result_actor;
	providerClass->activate_result=_xfdashboard_recently_used_search_provider_activate_result;
}

/* Class finalization */
void xfdashboard_recently_used_search_provider_class_finalize(XfdashboardRecentlyUsedSearchProviderClass *klass)
{
}

/* Object initialization
 * Create private structure and set up default values
 */
void xfdashboard_recently_used_search_provider_init(XfdashboardRecentlyUsedSearchProvider *self)
{
	XfdashboardRecentlyUsedSearchProviderPrivate	*priv;

	self->priv=priv=xfdashboard_recently_used_search_provider_get_instance_private(self);

	/* Set up default values */
	priv->recentManager=gtk_recent_manager_get_default();
}
