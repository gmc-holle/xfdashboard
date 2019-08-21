/*
 * theme-css: A theme used for rendering xfdashboard actors with CSS.
 *            The parser and the handling of CSS files is heavily based
 *            on mx-css, mx-style and mx-stylable of library mx
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

#include <libxfdashboard/theme-css.h>

#include <glib/gi18n-lib.h>
#include <glib.h>
#include <gio/gio.h>
#include <gio/gfiledescriptorbased.h>
#include <clutter/clutter.h>
#include <gtk/gtk.h>

#include <libxfdashboard/stylable.h>
#include <libxfdashboard/css-selector.h>
#include <libxfdashboard/compat.h>
#include <libxfdashboard/debug.h>


/* Define this class in GObject system */
struct _XfdashboardThemeCSSPrivate
{
	/* Instance related */
	gchar		*themePath;

	GList		*selectors;
	GList		*styles;
	GSList		*names;

	GHashTable	*registeredFunctions;

	gint		offsetLine;
};

G_DEFINE_TYPE_WITH_PRIVATE(XfdashboardThemeCSS,
							xfdashboard_theme_css,
							G_TYPE_OBJECT)

/* Properties */
enum
{
	PROP_0,

	PROP_THEME_PATH,

	PROP_LAST
};

static GParamSpec* XfdashboardThemeCSSProperties[PROP_LAST]={ 0, };

/* IMPLEMENTATION: Private variables and methods */
typedef enum /*< skip,prefix=XFDASHBOARD_THEME_CSS_SELECTOR_TYPE >*/
{
	XFDASHBOARD_THEME_CSS_SELECTOR_TYPE_NONE=0,
	XFDASHBOARD_THEME_CSS_SELECTOR_TYPE_SELECTOR,
	XFDASHBOARD_THEME_CSS_SELECTOR_TYPE_CONSTANT
} XfdashboardThemeCSSSelectorType;

typedef struct _XfdashboardThemeCSSSelector			XfdashboardThemeCSSSelector;
struct _XfdashboardThemeCSSSelector
{
	XfdashboardThemeCSSSelectorType	type;
	XfdashboardCssSelector			*selector;
	GHashTable						*style;
};

typedef struct _XfdashboardThemeCSSSelectorMatch	XfdashboardThemeCSSSelectorMatch;
struct _XfdashboardThemeCSSSelectorMatch
{
	XfdashboardThemeCSSSelector		*selector;
	gint							score;
};

typedef struct _XfdashboardThemeCSSTableCopyData	XfdashboardThemeCSSTableCopyData;
struct _XfdashboardThemeCSSTableCopyData
{
	GHashTable						*table;
	const gchar						*name;
};

#define XFDASHBOARD_THEME_CSS_FUNCTION_CALLBACK(f)	((XfdashboardThemeCSSFunctionCallback)(f))
typedef gboolean (*XfdashboardThemeCSSFunctionCallback)(XfdashboardThemeCSS *self,
														const gchar *inName,
														GList *inArguments,
														GValue *outResult,
														GError **outError);

/* Forward declarations */
static void _xfdashboard_theme_css_set_error(XfdashboardThemeCSS *self,
												GError **outError,
												XfdashboardThemeCSSErrorEnum inCode,
												const gchar *inFormat,
												...) G_GNUC_PRINTF (4, 5);

static gchar* _xfdashboard_theme_css_resolve_at_identifier_internal(XfdashboardThemeCSS *self,
																	GScanner *inScanner,
																	GScanner *inScopeScanner,
																	GList *inScopeSelectors,
																	const gchar *inRecursionIdentifier);

static gchar* _xfdashboard_theme_css_resolve_at_identifier_by_string(XfdashboardThemeCSS *self,
																		const gchar *inValue,
																		GScanner *inScopeScanner,
																		GList *inScopeSelectors,
																		const gchar *inRecursionIdentifier);

/* Helper function to set up GError object in this parser */
static void _xfdashboard_theme_css_set_error(XfdashboardThemeCSS *self,
												GError **outError,
												XfdashboardThemeCSSErrorEnum inCode,
												const gchar *inFormat,
												...)
{
	GError		*tempError;
	gchar		*message;
	va_list		args;

	/* Get error message */
	va_start(args, inFormat);
	message=g_strdup_vprintf(inFormat, args);
	va_end(args);

	/* Create error object */
	tempError=g_error_new_literal(XFDASHBOARD_THEME_CSS_ERROR, inCode, message);

	/* Set error */
	g_propagate_error(outError, tempError);

	/* Release allocated resources */
	g_free(message);
}

/* Create css value instance */
static XfdashboardThemeCSSValue* _xfdashboard_theme_css_value_new(void)
{
	return(g_slice_new0(XfdashboardThemeCSSValue));
}

/* Destroy css value instance */
static void _xfdashboard_theme_css_value_free(XfdashboardThemeCSSValue *self)
{
	g_slice_free(XfdashboardThemeCSSValue, self);
}


/* Copy a property */
static void _xfdashboard_theme_css_copy_table(gpointer *inKey, gpointer *inValue, XfdashboardThemeCSSTableCopyData *inData)
{
	XfdashboardThemeCSSValue		*value;

	value=_xfdashboard_theme_css_value_new();
	value->source=inData->name;
	value->string=(gchar*)inValue;

	g_hash_table_insert(inData->table, inKey, value);
}

/* Free selector match */
static void _xfdashboard_themes_css_selector_match_free(XfdashboardThemeCSSSelectorMatch *inData)
{
	g_slice_free(XfdashboardThemeCSSSelectorMatch, inData);
}

/* Append strings or characters to string in-place */
static gchar* _xfdashboard_theme_css_append_string(gchar *ioString1, const gchar *inString2)
{
	gchar	*tmp;

	if(!ioString1) return(g_strdup(inString2));

	if(!inString2) return(ioString1);

	tmp=g_strconcat(ioString1, inString2, NULL);
	g_free(ioString1);
	return(tmp);
}

static gchar* _xfdashboard_theme_css_append_char(gchar *ioString, gchar inChar)
{
	gchar	*tmp;
	gint	length;

	if(!ioString)
	{
		tmp=g_malloc(2);
		length=0;
	}
		else
		{
			length=strlen(ioString);
			tmp=g_realloc(ioString, length+2);
		}

	tmp[length]=inChar;
	tmp[length+1]='\0';

	return(tmp);
}

/* Destroy selector */
static void _xfdashboard_theme_css_selector_free(XfdashboardThemeCSSSelector *inSelector)
{
	g_return_if_fail(inSelector);

	/* Free allocated resources */
	if(inSelector->selector) g_object_unref(inSelector->selector);
	if(inSelector->style) g_hash_table_unref(inSelector->style);

	/* Free selector itself */
	g_slice_free(XfdashboardThemeCSSSelector, inSelector);
}

/* Create selector */
static XfdashboardThemeCSSSelector* _xfdashboard_theme_css_selector_new(const gchar *inName)
{
	XfdashboardThemeCSSSelector		*selector;

	selector=g_slice_new0(XfdashboardThemeCSSSelector);
	selector->type=XFDASHBOARD_THEME_CSS_SELECTOR_TYPE_NONE;
  
	return(selector);
}

/* Get function argument and transform it to requested type.
 * Returned value must first be cleared with g_value_unset and
 * then freed with g_free.
 */
static GValue* _xfdashboard_theme_css_get_argument(XfdashboardThemeCSS *self,
													GList *inArguments,
													guint inArgumentIndex,
													GType inRequestedType,
													GError **outError)
{
	GValue			argValue=G_VALUE_INIT;
	GValue			*resultValue;
	guint			numberArguments;

	g_return_val_if_fail(XFDASHBOARD_IS_THEME_CSS(self), NULL);
	g_return_val_if_fail(inArguments, NULL);
	g_return_val_if_fail(!outError || *outError==NULL, NULL);

	/* Check if argument index is in range */
	numberArguments=g_list_length(inArguments);
	if(inArgumentIndex>=numberArguments)
	{
		_xfdashboard_theme_css_set_error(self,
											outError,
											XFDASHBOARD_THEME_CSS_ERROR_FUNCTION_ERROR,
											_("Cannot get argument %d because only %d arguments are available"),
											inArgumentIndex,
											numberArguments);
		return(NULL);
	}

	/* Create empty result value of requested type */
	resultValue=g_new0(GValue, 1);
	if(!resultValue)
	{
		_xfdashboard_theme_css_set_error(self,
											outError,
											XFDASHBOARD_THEME_CSS_ERROR_FUNCTION_ERROR,
											_("Cannot allocate memory for argument %d"),
											inArgumentIndex);
		return(NULL);
	}

	resultValue=g_value_init(resultValue, inRequestedType);

	/* Get argument value which is a string */
	g_value_init(&argValue, G_TYPE_STRING);
	g_value_set_string(&argValue, (gchar*)g_list_nth_data(inArguments, inArgumentIndex));

	/* Transform argument value to result value of requested type */
	if(!g_value_transform(&argValue, resultValue))
	{
		/* Set error */
		_xfdashboard_theme_css_set_error(self,
											outError,
											XFDASHBOARD_THEME_CSS_ERROR_FUNCTION_ERROR,
											_("Cannot transform argument %d from type '%s' to type '%s'"),
											inArgumentIndex,
											g_type_name(G_VALUE_TYPE(&argValue)),
											g_type_name(G_VALUE_TYPE(resultValue)));

		/* Unset result value so that NULL will returned at the end */
		g_value_unset(resultValue);
		g_free(resultValue);
		resultValue=NULL;
	}

	/* Release allocated resources */
	g_value_unset(&argValue);

	/* Return argument value of requested type */
	return(resultValue);
}

/* Helper function to convert a string containing either a number or a percentage
 * to color component value which must be between 0 and 255.
 */
static gboolean _xfdashboard_theme_css_parse_string_to_color_component(XfdashboardThemeCSS *self,
																		const gchar *inString,
																		guint8 *outValue,
																		GError **outError)
{
	gdouble			componentValue;
	gchar			*end;

	g_return_val_if_fail(XFDASHBOARD_IS_THEME_CSS(self), FALSE);

	/* Check if string is given */
	if(!inString || *inString==0)
	{
		_xfdashboard_theme_css_set_error(self,
											outError,
											XFDASHBOARD_THEME_CSS_ERROR_FUNCTION_ERROR,
											_("Missing string to convert to color component value."));
		return(FALSE);
	}

	/* Parse string to value between 0.0 and 255.0 or percentage between 0.0 and 100.0
	 * which will be transformed to a value between 0.0 and 255.0
	 */
	componentValue=g_ascii_strtod(inString, &end);
	if(*end)
	{
		if(*end=='%')
		{
			componentValue=(componentValue/100.0f)*256.0f;
			end++;
		}

		while(*end)
		{
			if(!g_ascii_isspace(*end))
			{
				gchar		*message;

				/* Set error message */
				message=g_strdup_printf(_("Cannot convert string '%s' to color component value."), inString);
				_xfdashboard_theme_css_set_error(self,
													outError,
													XFDASHBOARD_THEME_CSS_ERROR_FUNCTION_ERROR,
													"%s",
													message);
				g_free(message);

				/* Return with error status */
				return(FALSE);
			}
		}
	}

	/* Check if component value is in range (between 0.0 and 255.0) */
	if(componentValue<0.0 || componentValue>=256.0)
	{
		gchar		*message;

		/* Set error message */
		message=g_strdup_printf(_("Color component value %.2f out of range"), componentValue);
		_xfdashboard_theme_css_set_error(self,
											outError,
											XFDASHBOARD_THEME_CSS_ERROR_FUNCTION_ERROR,
											"%s",
											message);
		g_free(message);

		/* Return with error status */
		return(FALSE);
	}

	/* If we get here we could compute color component value so set up result */
	if(outValue) *outValue=(guint8)componentValue;
	return(TRUE);
}

/* CSS function: try_icons(icon_name[, ...])
 * Takes a variable number of arguments. Each argument is a string containing
 * the icon name to lookup. The first one which could be found successfully
 * will be returned. If no one could be found an empty string will be returned.
 */
static gboolean _xfdashboard_theme_css_function_try_icons(XfdashboardThemeCSS *self,
															const gchar *inName,
															GList *inArguments,
															GValue *outResult,
															GError **outError)
{
	XfdashboardThemeCSSPrivate	*priv;
	GtkIconTheme				*iconTheme;

	g_return_val_if_fail(XFDASHBOARD_IS_THEME_CSS(self), FALSE);
	g_return_val_if_fail(inName && *inName, FALSE);
	g_return_val_if_fail(inArguments, FALSE);
	g_return_val_if_fail(outResult, FALSE);
	g_return_val_if_fail(!outError || *outError==NULL, FALSE);

	priv=self->priv;
	iconTheme=gtk_icon_theme_get_default();

	/* Initialize result with empty string. It will be overriden with found
	 * icon name if successful.
	 */
	g_value_init(outResult, G_TYPE_STRING);
	g_value_set_string(outResult, "\'\'");

	/* Iterate through arguments, try to find the icon and return first one found */
	while(inArguments)
	{
		const gchar				*iconName;
		gchar					*iconFilename;

		/* Get icon name */
		iconName=(const gchar*)inArguments->data;

		/* If icon name is an absolute path to a file then check if it exists */
		if(g_path_is_absolute(iconName) &&
			g_file_test(iconName, G_FILE_TEST_EXISTS))
		{
			/* Set up result */
			g_value_set_string(outResult, iconName);

			return(TRUE);
		}

		/* Check if it is a relative path to a file and check if it exists */
		iconFilename=g_build_filename(priv->themePath, iconName, NULL);
		if(g_file_test(iconFilename, G_FILE_TEST_EXISTS))
		{
			/* Release allocated resources */
			g_free(iconFilename);

			/* Set up result */
			g_value_set_string(outResult, iconName);

			return(TRUE);
		}
		g_free(iconFilename);

		/* Icon name is neither an absolute nor relative path to a file
		 * so check if it is a stock icon.
		 */
		if(gtk_icon_theme_has_icon(iconTheme, iconName))
		{
			/* Set up result */
			g_value_set_string(outResult, iconName);

			return(TRUE);
		}

		/* The icon name could not be found try next one */
		inArguments=g_list_next(inArguments);
	}

	return(TRUE);
}

/* CSS function: lighter(color) and darken(color)
 * Takes one argument which is a color and lightens or darkens it.
 */
static gboolean _xfdashboard_theme_css_function_lighter_darker(XfdashboardThemeCSS *self,
																const gchar *inName,
																GList *inArguments,
																GValue *outResult,
																GError **outError)
{
	GError			*error;
	GValue			*value;
	ClutterColor	*color;
	ClutterColor	result;

	g_return_val_if_fail(XFDASHBOARD_IS_THEME_CSS(self), FALSE);
	g_return_val_if_fail(inName && *inName, FALSE);
	g_return_val_if_fail(inArguments, FALSE);
	g_return_val_if_fail(outResult, FALSE);
	g_return_val_if_fail(!outError || *outError==NULL, FALSE);

	error=NULL;

	/* Get color to lighten or to darken */
	value=_xfdashboard_theme_css_get_argument(self, inArguments, 0, CLUTTER_TYPE_COLOR, &error);
	if(!value)
	{
		g_propagate_error(outError, error);
		return(FALSE);
	}

	color=clutter_color_copy(clutter_value_get_color(value));
	g_value_unset(value);
	g_free(value);

	/* Lighten or darken color */
	if(g_strcmp0(inName, "lighter")==0) clutter_color_lighten(color, &result);
		else clutter_color_darken(color, &result);

	/* Set up result */
	g_value_init(outResult, CLUTTER_TYPE_COLOR);
	clutter_value_set_color(outResult, &result);

	return(TRUE);
}

/* CSS function: alpha(color, factor)
 * Takes two arguments. First one is a color and the second one is the factor to increase
 * or descrease alpha value of color with.
 */
static gboolean _xfdashboard_theme_css_function_alpha(XfdashboardThemeCSS *self,
														const gchar *inName,
														GList *inArguments,
														GValue *outResult,
														GError **outError)
{
	GError			*error;
	GValue			*value;
	ClutterColor	*color;
	gdouble			factor;
	gdouble			alpha;
	ClutterColor	result;

	g_return_val_if_fail(XFDASHBOARD_IS_THEME_CSS(self), FALSE);
	g_return_val_if_fail(inName && *inName, FALSE);
	g_return_val_if_fail(inArguments, FALSE);
	g_return_val_if_fail(outResult, FALSE);
	g_return_val_if_fail(!outError || *outError==NULL, FALSE);

	error=NULL;

	/* Get color to adjust alpha value */
	value=_xfdashboard_theme_css_get_argument(self, inArguments, 0, CLUTTER_TYPE_COLOR, &error);
	if(!value)
	{
		g_propagate_error(outError, error);
		return(FALSE);
	}

	color=clutter_color_copy(clutter_value_get_color(value));
	g_value_unset(value);
	g_free(value);

	/* Get alpha change factor */
	value=_xfdashboard_theme_css_get_argument(self, inArguments, 1, G_TYPE_DOUBLE, &error);
	if(!value)
	{
		/* Release allocated resources */
		clutter_color_free(color);

		/* Propagate error message and return failure result */
		g_propagate_error(outError, error);
		return(FALSE);
	}

	factor=g_value_get_double(value);
	g_value_unset(value);
	g_free(value);

	/* Change alpha value of color by factor */
	alpha=(gdouble)color->alpha * factor;
	if(alpha<0.0) alpha=0.0;
		else if(alpha>255.0) alpha=255.0;

	clutter_color_init(&result,
						color->red,
						color->green,
						color->blue,
						(guint8)alpha);

	/* Set up result */
	g_value_init(outResult, CLUTTER_TYPE_COLOR);
	clutter_value_set_color(outResult, &result);

	/* Release allocated resources */
	clutter_color_free(color);

	return(TRUE);
}

/* CSS function: shade(color, factor)
 * Takes two arguments. First one is a color and the second one is the shade factor.
 */
static gboolean _xfdashboard_theme_css_function_shade(XfdashboardThemeCSS *self,
														const gchar *inName,
														GList *inArguments,
														GValue *outResult,
														GError **outError)
{
	GError			*error;
	GValue			*value;
	ClutterColor	*color;
	gdouble			factor;
	ClutterColor	result;

	g_return_val_if_fail(XFDASHBOARD_IS_THEME_CSS(self), FALSE);
	g_return_val_if_fail(inName && *inName, FALSE);
	g_return_val_if_fail(inArguments, FALSE);
	g_return_val_if_fail(outResult, FALSE);
	g_return_val_if_fail(!outError || *outError==NULL, FALSE);

	error=NULL;

	/* Get color to shade */
	value=_xfdashboard_theme_css_get_argument(self, inArguments, 0, CLUTTER_TYPE_COLOR, &error);
	if(!value)
	{
		g_propagate_error(outError, error);
		return(FALSE);
	}

	color=clutter_color_copy(clutter_value_get_color(value));
	g_value_unset(value);
	g_free(value);

	/* Get shade factor */
	value=_xfdashboard_theme_css_get_argument(self, inArguments, 1, G_TYPE_DOUBLE, &error);
	if(!value)
	{
		/* Release allocated resources */
		clutter_color_free(color);

		/* Propagate error message and return failure result */
		g_propagate_error(outError, error);
		return(FALSE);
	}

	factor=g_value_get_double(value);
	g_value_unset(value);
	g_free(value);

	/* Shade color by factor */
	clutter_color_shade(color, factor, &result);

	/* Set up result */
	g_value_init(outResult, CLUTTER_TYPE_COLOR);
	clutter_value_set_color(outResult, &result);

	return(TRUE);
}

/* CSS function: mix(color1, color2, factor)
 * Takes three arguments. First two are colors and the third one is a factor
 * between 0.0 and 1.0 defining how much the resulting color moved from color1
 * to color2. The factor is more or less a progress fraction of the transformation
 * of color1 to color2.
 */
static gboolean _xfdashboard_theme_css_function_mix(XfdashboardThemeCSS *self,
													const gchar *inName,
													GList *inArguments,
													GValue *outResult,
													GError **outError)
{
	GError			*error;
	GValue			*value;
	ClutterColor	*color1;
	ClutterColor	*color2;
	gdouble			factor;
	ClutterColor	result;

	g_return_val_if_fail(XFDASHBOARD_IS_THEME_CSS(self), FALSE);
	g_return_val_if_fail(inName && *inName, FALSE);
	g_return_val_if_fail(inArguments, FALSE);
	g_return_val_if_fail(outResult, FALSE);
	g_return_val_if_fail(!outError || *outError==NULL, FALSE);

	error=NULL;

	/* Get first color (initial) */
	value=_xfdashboard_theme_css_get_argument(self, inArguments, 0, CLUTTER_TYPE_COLOR, &error);
	if(!value)
	{
		g_propagate_error(outError, error);
		return(FALSE);
	}

	color1=clutter_color_copy(clutter_value_get_color(value));
	g_value_unset(value);
	g_free(value);

	/* Get second color (final) */
	value=_xfdashboard_theme_css_get_argument(self, inArguments, 1, CLUTTER_TYPE_COLOR, &error);
	if(!value)
	{
		/* Release allocated resources */
		clutter_color_free(color1);

		/* Propagate error message and return failure result */
		g_propagate_error(outError, error);
		return(FALSE);
	}

	color2=clutter_color_copy(clutter_value_get_color(value));
	g_value_unset(value);
	g_free(value);

	/* Get factor (progress fraction) */
	value=_xfdashboard_theme_css_get_argument(self, inArguments, 2, G_TYPE_DOUBLE, &error);
	if(!value)
	{
		/* Release allocated resources */
		clutter_color_free(color2);
		clutter_color_free(color1);

		/* Propagate error message and return failure result */
		g_propagate_error(outError, error);
		return(FALSE);
	}

	factor=g_value_get_double(value);
	g_value_unset(value);
	g_free(value);

	if(factor<0.0 || factor>1.0)
	{
		gchar		*message;

		/* Release allocated resources */
		clutter_color_free(color2);
		clutter_color_free(color1);

		/* Set error message */
		message=g_strdup_printf(_("Factor %.2f is out of range"), factor);
		_xfdashboard_theme_css_set_error(self,
											outError,
											XFDASHBOARD_THEME_CSS_ERROR_FUNCTION_ERROR,
											"%s",
											message);
		g_free(message);

		/* Return with error status */
		return(FALSE);
	}

	/* Compute mixed color by factor */
	clutter_color_interpolate(color1, color2, factor, &result);

	/* Set up result */
	g_value_init(outResult, CLUTTER_TYPE_COLOR);
	clutter_value_set_color(outResult, &result);

	return(TRUE);
}

/* CSS function: rgb(r, g, b) and rgba(r, g, b, a)
 * Take variable number of arguments which must all be string. Iterates through
 * string argument and returns first one resolving in a valid icon (file or stock icon).
 */
static gboolean _xfdashboard_theme_css_function_rgb_rgba(XfdashboardThemeCSS *self,
															const gchar *inName,
															GList *inArguments,
															GValue *outResult,
															GError **outError)
{
	GError			*error;
	GValue			*value;
	gboolean		isRGBA;
	gint			i;
	guint8			color[4];
	ClutterColor	result;

	g_return_val_if_fail(XFDASHBOARD_IS_THEME_CSS(self), FALSE);
	g_return_val_if_fail(inName && *inName, FALSE);
	g_return_val_if_fail(inArguments, FALSE);
	g_return_val_if_fail(outResult, FALSE);
	g_return_val_if_fail(!outError || *outError==NULL, FALSE);

	error=NULL;

	/* Check if RGB or RGBA should be computed */
	isRGBA=(g_strcmp0(inName, "rgba")==0 ? TRUE : FALSE);

	/* Get arguments and compute color component values but stop on
	 * first argument failing.
	 */
	for(i=0; i<3; i++)
	{
		/* Get argument */
		value=_xfdashboard_theme_css_get_argument(self, inArguments, i, G_TYPE_STRING, &error);
		if(!value)
		{
			g_propagate_error(outError, error);
			return(FALSE);
		}

		/* Compute color component value */
		if(!_xfdashboard_theme_css_parse_string_to_color_component(self,
																	g_value_get_string(value),
																	&color[i],
																	&error))
		{
			g_propagate_error(outError, error);

			/* Release allocated resources */
			g_value_unset(value);
			g_free(value);

			return(FALSE);
		}

		/* Release allocated resources */
		g_value_unset(value);
		g_free(value);
	}

	if(isRGBA)
	{
		gfloat		alpha;

		/* Get alpha factor */
		value=_xfdashboard_theme_css_get_argument(self, inArguments, 3, G_TYPE_DOUBLE, &error);
		if(!value)
		{
			/* Propagate error message and return failure result */
			g_propagate_error(outError, error);
			return(FALSE);
		}

		alpha=g_value_get_double(value);
		g_value_unset(value);
		g_free(value);

		if(alpha<0.0 || alpha>1.0)
		{
			gchar		*message;

			/* Set error message */
			message=g_strdup_printf(_("Alpha factor %.2f is out of range"), alpha);
			_xfdashboard_theme_css_set_error(self,
												outError,
												XFDASHBOARD_THEME_CSS_ERROR_FUNCTION_ERROR,
												"%s",
												message);
			g_free(message);

			/* Return with error status */
			return(FALSE);
		}

		/* Set alpha factor */
		color[3]=(guint8)(alpha*255.0f);
	}
		else color[3]=0xff;

	/* Set up result */
	clutter_color_init(&result, color[0], color[1], color[2], color[3]);

	g_value_init(outResult, CLUTTER_TYPE_COLOR);
	clutter_value_set_color(outResult, &result);

	return(TRUE);
}

/* Register CSS function */
static void _xfdashboard_theme_css_register_function(XfdashboardThemeCSS *self,
														const gchar *inName,
														XfdashboardThemeCSSFunctionCallback inCallback)
{
	XfdashboardThemeCSSPrivate		*priv;

	g_return_if_fail(XFDASHBOARD_IS_THEME_CSS(self));
	g_return_if_fail(inName);
	g_return_if_fail(inCallback);

	priv=self->priv;

	/* Create hash-table for mapping function names to function callback
	 * if this hash-table does not exist yet.
	 */
	if(!priv->registeredFunctions)
	{
		priv->registeredFunctions=g_hash_table_new_full(g_str_hash,
														g_str_equal,
														g_free,
														NULL);
	}

	/* Check if a function with this name is already registered */
	if(g_hash_table_lookup_extended(priv->registeredFunctions, inName, NULL, NULL))
	{
		g_warning(_("CSS function '%s' is already registered."), inName);
		return;
	}

	/* Register function by adding name (as key) and callback (as value) to
	 * hash-table of registered functions.
	 */
	g_hash_table_insert(priv->registeredFunctions, g_strdup(inName), inCallback);
}

/* Resolve '@' identifier.
 * Prints error message and returns NULL if unresolvable.
 */
static gchar* _xfdashboard_theme_css_parse_at_identifier(XfdashboardThemeCSS *self,
															GScanner *ioScanner,
															GScanner *inScopeScanner,
															GList *inScopeSelectors,
															const gchar *inRecursionIdentifier)
{
	XfdashboardThemeCSSPrivate				*priv;
	GTokenType								token;
	gchar									*identifier;
	GList									*iter;
	XfdashboardThemeCSSSelector				*selector;
	gpointer								value;
	gchar									*errorMessage;
	XfdashboardThemeCSSFunctionCallback		functionCallback;

	g_return_val_if_fail(XFDASHBOARD_IS_THEME_CSS(self), NULL);
	g_return_val_if_fail(ioScanner, NULL);
	g_return_val_if_fail(!inRecursionIdentifier || *inRecursionIdentifier, NULL);

	priv=self->priv;

	/* Get identifier */
	token=g_scanner_get_next_token(ioScanner);
	if(token!=G_TOKEN_IDENTIFIER)
	{
		g_scanner_unexp_token(inScopeScanner,
								G_TOKEN_IDENTIFIER,
								NULL,
								NULL,
								NULL,
								_("An identifier must follow '@'"),
								FALSE);
		return(NULL);
	}

	identifier=g_strdup(ioScanner->value.v_identifier);

	/* Check if identifier is a function then parse function argument and
	 * call function.
	 */
	if(g_hash_table_lookup_extended(priv->registeredFunctions, identifier, NULL, (gpointer*)&functionCallback))
	{
		gboolean				error;
		GList					*arguments;
		gchar					*arg;
		GScannerConfig			*scannerConfig;
		GScannerConfig			*oldScannerConfig;
		gchar					*result;

		error=FALSE;
		arguments=NULL;
		result=NULL;
		arg=NULL;

		/* Set up scanner config for parsing function arguments */
		oldScannerConfig=ioScanner->config;

		scannerConfig=(GScannerConfig*)g_memdup(ioScanner->config, sizeof(GScannerConfig));
		scannerConfig->cset_skip_characters=" \t\n";
		scannerConfig->char_2_token=TRUE;
		scannerConfig->scan_string_sq=TRUE;
		scannerConfig->scan_string_dq=TRUE;
		ioScanner->config=scannerConfig;

		/* Function arguments must begin with '('. Check for it and parse function arguments ... */
		XFDASHBOARD_DEBUG(self, THEME,
							"Fetching arguments to for calling function '%s'",
							identifier);

		token=g_scanner_get_next_token(ioScanner);
		if(token==G_TOKEN_LEFT_PAREN)
		{
			token=g_scanner_get_next_token(ioScanner);
			while(!error && token!=G_TOKEN_RIGHT_PAREN && token!=G_TOKEN_EOF)
			{
				switch((gint)token)
				{
					case '@':
						token=g_scanner_peek_next_token(ioScanner);
						if(token==G_TOKEN_IDENTIFIER)
						{
							gchar		*orginalValue;
							gchar		*resolvedValue;

							/* Remember identifier to resolve */
							orginalValue=g_strdup(ioScanner->value.v_identifier);

							/* Restore old scanner config */
							ioScanner->config=oldScannerConfig;

							/* Resolve '@'-identifier */
							resolvedValue=_xfdashboard_theme_css_parse_at_identifier(self,
																						ioScanner,
																						inScopeScanner,
																						inScopeSelectors,
																						inRecursionIdentifier);

							/* Resolve resolved value to get final value */
							if(resolvedValue)
							{
								gchar	*finalResolvedValue;

								/* Resolve resolved value */
								finalResolvedValue=_xfdashboard_theme_css_resolve_at_identifier_by_string(self,
																											resolvedValue,
																											inScopeScanner,
																											inScopeSelectors,
																											inRecursionIdentifier);

								/* Release old value and set new one */
								g_free(resolvedValue);
								resolvedValue=finalResolvedValue;
							}

							if(resolvedValue)
							{
								arg=_xfdashboard_theme_css_append_string(arg, resolvedValue);
								g_free(resolvedValue);
							}
								else
								{
									error=TRUE;
									XFDASHBOARD_DEBUG(self, THEME,
														"Could not resolve '%s' for argument #%d of function '%s'",
														orginalValue,
														g_list_length(arguments),
														identifier);
								}

							/* Set new scanner config again to parse remaining arguments */
							ioScanner->config=scannerConfig;

							/* Forget identifier */
							g_free(orginalValue);
						}
							else
							{
								error=TRUE;
								g_scanner_unexp_token(inScopeScanner,
														G_TOKEN_IDENTIFIER,
														NULL,
														NULL,
														NULL,
														_("An identifier must follow '@'"),
														FALSE);
							}
						break;

					case G_TOKEN_COMMA:
						if(arg)
						{
							/* Add function argument to list of arguments */
							arguments=g_list_append(arguments, arg);
							XFDASHBOARD_DEBUG(self, THEME,
												"Added argument #%d: '%s'",
												g_list_length(arguments),
												arg);

							/* Prepare for next argument */
							arg=NULL;
						}
							else
							{
								error=TRUE;
								g_scanner_unexp_token(inScopeScanner,
														G_TOKEN_ERROR,
														NULL,
														NULL,
														NULL,
														_("Missing function argument"),
														FALSE);
							}
						break;

					case G_TOKEN_IDENTIFIER:
						arg=_xfdashboard_theme_css_append_string(arg, ioScanner->value.v_identifier);
						break;

					case G_TOKEN_STRING:
						arg=_xfdashboard_theme_css_append_string(arg, ioScanner->value.v_string);
						break;

					case G_TOKEN_LEFT_PAREN:
					case G_TOKEN_LEFT_CURLY:
					case G_TOKEN_RIGHT_CURLY:
					case G_TOKEN_LEFT_BRACE:
					case G_TOKEN_RIGHT_BRACE:
						{
							gchar		*message;

							error=TRUE;

							/* Set error message */
							message=g_strdup_printf(_("Invalid character '%c' in function argument"), (gchar)token);
							g_scanner_unexp_token(inScopeScanner,
													G_TOKEN_ERROR,
													NULL,
													NULL,
													NULL,
													message,
													FALSE);
							g_free(message);
						}
						break;

					default:
						if(token<128 && g_ascii_isprint((gchar)token))
						{
							arg=_xfdashboard_theme_css_append_char(arg, (gchar)token);
						}
							else
							{
								error=TRUE;
								g_scanner_unexp_token(inScopeScanner,
														G_TOKEN_ERROR,
														NULL,
														NULL,
														NULL,
														_("Invalid character in function argument"),
														FALSE);
							}
						break;
				}

				/* Continue with next token */
				token=g_scanner_get_next_token(ioScanner);
			}

			/* Add final function argument to list of arguments if available
			 * because we will not see a comma as function argument seperator
			 * which will cause the argument to be added. So check now for a
			 * remaining and final argument and add it.
			 */
			if(arg)
			{
				/* Add function argument to list of arguments */
				arguments=g_list_append(arguments, arg);
				XFDASHBOARD_DEBUG(self, THEME,
									"Added final argument #%d: '%s'",
									g_list_length(arguments),
									arg);

				/* Does not make sense but just in case - prepare for next argument ;) */
				arg=NULL;
			}

			/* A function must end at ')'. If it does not it is an error. */
			if(!error && token!=G_TOKEN_RIGHT_PAREN)
			{
				error=TRUE;
				g_scanner_unexp_token(inScopeScanner,
										G_TOKEN_RIGHT_PAREN,
										NULL,
										NULL,
										NULL,
										_("Missing ')' after function"),
										FALSE);
			}
		}
			/* ... otherwise it an error because the function arguments did not begin
			 * with '(' and could not be parsed.
			 */
			else
			{
				error=TRUE;
				g_scanner_unexp_token(inScopeScanner,
										G_TOKEN_LEFT_PAREN,
										NULL,
										NULL,
										NULL,
										_("Missing '(' after function"),
										FALSE);
			}

		/* Restore old scanner config */
		ioScanner->config=oldScannerConfig;
		g_free(scannerConfig);

		/* Do function call if no error occured so far */
		if(!error)
		{
			GValue				functionValue=G_VALUE_INIT;
			GError				*functionError;
			gboolean			functionSuccess;

			/* Initialize for function call */
			functionError=NULL;

			XFDASHBOARD_DEBUG(self, THEME,
								"Calling registered function %s with %d arguments",
								identifier,
								g_list_length(arguments));
			functionSuccess=(functionCallback)(self, identifier, arguments, &functionValue, &functionError);
			if(functionSuccess)
			{
				GValue			stringValue=G_VALUE_INIT;

				/* Transform function value into a string */
				g_value_init(&stringValue, G_TYPE_STRING);

				if(g_value_transform(&functionValue, &stringValue))
				{
					result=g_value_dup_string(&stringValue);
				}
					else
					{
						error=TRUE;

						/* Set error message */
						errorMessage=g_strdup_printf(_("Could not transform result of function '%s' to a string"),
														identifier);
						g_scanner_unexp_token(inScopeScanner,
												G_TOKEN_ERROR,
												NULL,
												NULL,
												NULL,
												errorMessage,
												FALSE);
						g_free(errorMessage);
					}

				/* Release allocated resources */
				g_value_unset(&stringValue);
				g_value_unset(&functionValue);

				XFDASHBOARD_DEBUG(self, THEME,
									"Calling function %s with %d arguments succeeded with result: %s",
									identifier,
									g_list_length(arguments),
									result);
			}
				else
				{
					gchar		*message;

					XFDASHBOARD_DEBUG(self, THEME,
										"Calling function %s with %d arguments failed: %s",
										identifier,
										g_list_length(arguments),
										(functionError && functionError->message) ? functionError->message : _("Unknown error"));

					/* Set error message */
					if(functionError && functionError->message)
					{
						message=g_strdup_printf(_("Function '%s' failed with error: %s"),
													identifier,
													functionError->message);
					}
						else
						{
							message=g_strdup_printf(_("Function '%s' failed!"),
														identifier);
						}

					g_scanner_unexp_token(inScopeScanner,
											G_TOKEN_ERROR,
											NULL,
											NULL,
											NULL,
											message,
											FALSE);

					/* Release allocated resources */
					if(functionError) g_error_free(functionError);
					if(message) g_free(message);
				}
		}

		/* Release allocated resources */
		if(arguments) g_list_free_full(arguments, (GDestroyNotify)g_free);
		g_free(identifier);

		/* Return value found */
		return(result);
	}

	/* Identifier is a constant so lookup constant by iterating through all
	 * constants selectors backwards and use first value found. We iterate
	 * through these selectors backwards (first in selectors of current file/scope
	 * then all previous ones) to let last definition win.
	 */
	if(inRecursionIdentifier &&
		g_strcmp0(identifier, inRecursionIdentifier)==0)
	{
		/* Identifier was unresolvable because of indefinite recursion,
		 * so print error message and also a critical message.
		 */
		g_critical("Indefinte recursion of @-constant '%s' detected - aborting parsing", inRecursionIdentifier);

		errorMessage=g_strdup_printf(_("Cannot resolve identifier '@%s' because of indefinite recursion"), identifier);
		g_scanner_unexp_token(inScopeScanner,
								G_TOKEN_ERROR,
								NULL,
								NULL,
								NULL,
								errorMessage,
								FALSE);
		g_free(errorMessage);

		/* Release allocated resources */
		g_free(identifier);

		/* Identifier was unresolvable so return NULL */
		return(NULL);
	}

	for(iter=g_list_last(inScopeSelectors); iter; iter=g_list_previous(iter))
	{
		selector=(XfdashboardThemeCSSSelector*)iter->data;

		/* Handle only constants selectors */
		if(selector->type==XFDASHBOARD_THEME_CSS_SELECTOR_TYPE_CONSTANT)
		{
			if(g_hash_table_lookup_extended(selector->style, identifier, NULL, &value))
			{
				/* Release allocated resources */
				g_free(identifier);

				/* Return value found */
				return(g_strdup(value));
			}
		}
	}

	for(iter=g_list_last(priv->selectors); iter; iter=g_list_previous(iter))
	{
		selector=(XfdashboardThemeCSSSelector*)iter->data;

		/* Handle only constants selectors */
		if(selector->type==XFDASHBOARD_THEME_CSS_SELECTOR_TYPE_CONSTANT)
		{
			if(g_hash_table_lookup_extended(selector->style, identifier, NULL, &value))
			{
				/* Release allocated resources */
				g_free(identifier);

				/* Return value found */
				return(g_strdup(value));
			}
		}
	}

	/* Identifier was unresolvable so print error message */
	errorMessage=g_strdup_printf(_("Unresolvable identifier '@%s'"), identifier);
	g_scanner_unexp_token(inScopeScanner,
							G_TOKEN_ERROR,
							NULL,
							NULL,
							NULL,
							errorMessage,
							FALSE);
	g_free(errorMessage);

	/* Release allocated resources */
	g_free(identifier);

	/* Identifier was unresolvable so return NULL */
	return(NULL);
}

static gchar* _xfdashboard_theme_css_resolve_at_identifier_internal(XfdashboardThemeCSS *self,
																	GScanner *inScanner,
																	GScanner *inScopeScanner,
																	GList *inScopeSelectors,
																	const gchar *inRecursionIdentifier)
{
	GTokenType		token;
	gchar			*value;
	gchar			*haveResolvedAtIdentifier;

	g_return_val_if_fail(XFDASHBOARD_IS_THEME_CSS(self), NULL);
	g_return_val_if_fail(inScanner, NULL);
	g_return_val_if_fail(inScopeScanner, NULL);
	g_return_val_if_fail(!inRecursionIdentifier || *inRecursionIdentifier, NULL);

	/* Parse value and resolve '@' identifier */
	haveResolvedAtIdentifier=NULL;
	value=NULL;

	token=g_scanner_get_next_token(inScanner);
	while(token!=G_TOKEN_EOF)
	{
		switch(token)
		{
			case G_TOKEN_IDENTIFIER:
				value=_xfdashboard_theme_css_append_string(value, inScanner->value.v_identifier);
				break;

			case G_TOKEN_STRING:
				value=_xfdashboard_theme_css_append_string(value, inScanner->value.v_string);
				break;

			case G_TOKEN_CHAR:
				/* Check for character '@' identifing an identifier which needs to get resolved ... */
				if(inScanner->value.v_char=='@')
				{
					gchar		*constantValue;

					/* Resolve value and append resolved value but stop parsing and return NULL
					 * if unresolvable. Also remember '@' identifier we have resolved to get the
					 * new value resolved.
					 */
					token=g_scanner_peek_next_token(inScanner);
					if(token!=G_TOKEN_IDENTIFIER) return(NULL);

					haveResolvedAtIdentifier=g_strdup(inScanner->next_value.v_identifier);

					constantValue=_xfdashboard_theme_css_parse_at_identifier(self, inScanner, inScopeScanner, inScopeSelectors, inRecursionIdentifier);
					if(!constantValue)
					{
						g_free(value);
						return(NULL);
					}

					value=_xfdashboard_theme_css_append_string(value, constantValue);

					/* Release allocated resources */
					g_free(constantValue);
				}
					/* ... otherwise just add character to value */
					else value=_xfdashboard_theme_css_append_char(value, inScanner->value.v_char);
				break;

			default:
				/* This code should never be reached! */
				g_assert_not_reached();
				break;
		}

		/* Continue with next token */
		token=g_scanner_get_next_token(inScanner);
	}

	/* If any '@' identifier was found and resolved, we need to re-parse resolved value
	 * for further '@' identifiers.
	 */
	if(haveResolvedAtIdentifier)
	{
		gchar		*resolvedValue;

		/* Call ourselve recursive (via _xfdashboard_theme_css_resolve_at_identifier_by_string)
		 * to resolve any '@' identifier which might be in value resolved this time.
		 */
		XFDASHBOARD_DEBUG(self, THEME,
							"Resolving css value '%s' because of constant identifier '%s'",
							value,
							haveResolvedAtIdentifier);

		resolvedValue=_xfdashboard_theme_css_resolve_at_identifier_by_string(self,
																				value,
																				inScopeScanner,
																				inScopeSelectors,
																				haveResolvedAtIdentifier);

		XFDASHBOARD_DEBUG(self, THEME,
							"Resolved css value '%s' to '%s' recursively because of constant identifier '%s'",
							value,
							resolvedValue,
							haveResolvedAtIdentifier);

		/* Release old value and new one */
		g_free(value);
		value=resolvedValue;

		/* Release allocated resources */
		g_free(haveResolvedAtIdentifier);
	}

	/* Return resolved value */
	return(value);
}

static gchar* _xfdashboard_theme_css_resolve_at_identifier_by_string(XfdashboardThemeCSS *self,
																		const gchar *inText,
																		GScanner *inScopeScanner,
																		GList *inScopeSelectors,
																		const gchar *inRecursionIdentifier)
{
	GScanner		*scanner;
	gchar			*value;
	static gint		callDepth=0;

	g_return_val_if_fail(XFDASHBOARD_IS_THEME_CSS(self), NULL);
	g_return_val_if_fail(inScopeScanner, NULL);
	g_return_val_if_fail(inText, NULL);
	g_return_val_if_fail(!inRecursionIdentifier || *inRecursionIdentifier, NULL);

	/* Increment call depth and check if it is too deep now to avoid recursive
	 * resolves of '@' identifier definitions.
	 */
	callDepth++;
	if(callDepth>128)
	{
		gchar		*error;

		error=g_strdup_printf(_("Possible recursion detected because call depth is too deep while resolving '%s'"),
								inText);
		g_scanner_unexp_token(inScopeScanner,
								G_TOKEN_ERROR,
								NULL,
								NULL,
								NULL,
								error,
								FALSE);
		g_free(error);

		return(NULL);
	}

	/* Create scanner to resolve value of '@' identifier */
	scanner=g_scanner_new(NULL);

	/* Set up scanner config
	 * - Identifiers are allowed to contain '-' (minus sign) as non-first characters
	 * - Disallow scanning float values as we need '.' for identifiers
	 * - Set up single comment line not to include '#' as this character is need for identifiers
	 * - Disable parsing HEX values
	 * - Identifiers cannot be single quoted
	 * - Identifiers cannot be double quoted
	 */
	scanner->config->cset_identifier_first=G_CSET_a_2_z "#_-0123456789" G_CSET_A_2_Z G_CSET_LATINS G_CSET_LATINC;
	scanner->config->cset_identifier_nth=scanner->config->cset_identifier_first;
	scanner->config->scan_identifier_1char=1;
	scanner->config->char_2_token=FALSE;
	scanner->config->cset_skip_characters="";
	scanner->config->scan_string_sq=TRUE;
	scanner->config->scan_string_dq=TRUE;
	scanner->config->scan_float=FALSE;

	/* Set string to parse and resolve */
	g_scanner_input_text(scanner, inText, strlen(inText));

	/* Resolve '@' identifier */
	value=_xfdashboard_theme_css_resolve_at_identifier_internal(self,
																scanner,
																inScopeScanner,
																inScopeSelectors,
																inRecursionIdentifier);

	/* Destroy scanner */
	g_scanner_destroy(scanner);

	/* Decrement call depth counter again */
	callDepth--;

	/* Return resolved '@' identifier which may be NULL in case of error */
	return(value);
}

/* Parse CSS from stream */
static GTokenType _xfdashboard_theme_css_parse_css_key_value(XfdashboardThemeCSS *self,
																GScanner *inScanner,
																GList *inScopeSelectors,
																gboolean inDoResolveAt,
																gchar **outKey,
																gchar **outValue)
{
	GTokenType		token;
	gboolean		propertyStartsWithDash;
	GScannerConfig	*scannerConfig;
	GScannerConfig	*oldScannerConfig;

	g_return_val_if_fail(XFDASHBOARD_IS_THEME_CSS(self), G_TOKEN_ERROR);
	g_return_val_if_fail(inScanner, G_TOKEN_ERROR);
	g_return_val_if_fail(outKey && *outKey==NULL, G_TOKEN_ERROR);
	g_return_val_if_fail(outValue && *outValue==NULL, G_TOKEN_ERROR);

	/* Parse property name. Property names can start with '-' but it needs
	 * an identifier at least
	 */
	propertyStartsWithDash=FALSE;

	token=g_scanner_get_next_token(inScanner);
	if(token=='-')
	{
		token=g_scanner_get_next_token(inScanner);
		propertyStartsWithDash=TRUE;
	}

	if(token!=G_TOKEN_IDENTIFIER)
	{
		g_scanner_unexp_token(inScanner,
								G_TOKEN_IDENTIFIER,
								NULL,
								NULL,
								NULL,
								_("Invalid property name"),
								TRUE);

		return(G_TOKEN_IDENTIFIER);
	}

	/* Build key */
	if(propertyStartsWithDash) *outKey=g_strconcat("-", inScanner->value.v_identifier, NULL);
		else *outKey=g_strdup(inScanner->value.v_identifier);

	/* Key and value must be seperated by a colon */
	token=g_scanner_get_next_token(inScanner);
	if(token!=':')
	{
		/* Show parser error message */
		g_scanner_unexp_token(inScanner,
								':',
								NULL,
								NULL,
								NULL,
								_("Property names and values must be separated by colon"),
								TRUE);

		/* Return error result */
		return(':');
	}

	/* Set parser option to parse property value and parse them */
	scannerConfig=(GScannerConfig*)g_memdup(inScanner->config, sizeof(GScannerConfig));
	scannerConfig->cset_identifier_first=G_CSET_a_2_z "#_-0123456789" G_CSET_A_2_Z G_CSET_LATINS G_CSET_LATINC;
	scannerConfig->cset_identifier_nth=scannerConfig->cset_identifier_first;
	scannerConfig->scan_identifier_1char=1;
	scannerConfig->char_2_token=FALSE;
	scannerConfig->cset_skip_characters="\n";
	scannerConfig->scan_string_sq=TRUE;
	scannerConfig->scan_string_dq=TRUE;

	oldScannerConfig=inScanner->config;
	inScanner->config=scannerConfig;

	while(inScanner->next_token!=G_TOKEN_CHAR ||
			inScanner->next_value.v_char!=';')
	{
		token=g_scanner_get_next_token(inScanner);
		switch(token)
		{
			case G_TOKEN_IDENTIFIER:
				*outValue=_xfdashboard_theme_css_append_string(*outValue, inScanner->value.v_identifier);
				break;

			case G_TOKEN_CHAR:
				*outValue=_xfdashboard_theme_css_append_char(*outValue, inScanner->value.v_char);
				break;

			case G_TOKEN_STRING:
				/* If we should not re-scan the value to resolve it then we need to enclose the string
				 * we get from scanner with single quotation marks. It must be single quotation marks
				 * because the value is already escaped and they are needed to get G_TOKEN_STRING tokens
				 * from subsequent scanner in further resolving attempts.
				 */
				if(!inDoResolveAt) *outValue=_xfdashboard_theme_css_append_char(*outValue, '\'');
				*outValue=_xfdashboard_theme_css_append_string(*outValue, inScanner->value.v_string);
				if(!inDoResolveAt) *outValue=_xfdashboard_theme_css_append_char(*outValue, '\'');
				break;

			default:
				/* Restore old parser options */
				inScanner->config=oldScannerConfig;
				g_free(scannerConfig);

				/* Show parser error message */
				g_scanner_unexp_token(inScanner,
										';',
										NULL,
										NULL,
										NULL,
										_("Invalid property value"),
										TRUE);

				/* Return error result */
				return(';');
		}

		g_scanner_peek_next_token(inScanner);
	}

	/* Property values must end at a semi-colon */
	token=g_scanner_get_next_token(inScanner);
	if(token!=G_TOKEN_CHAR ||
		inScanner->value.v_char!=';')
	{
		/* Restore old parser options */
		inScanner->config=oldScannerConfig;
		g_free(scannerConfig);

		/* Show parser error message */
		g_scanner_unexp_token(inScanner,
								';',
								NULL,
								NULL,
								NULL,
								_("Property values must end with semi-colon"),
								TRUE);

		/* Return error result */
		return(';');
	}

	/* Resolve '@' identifiers if requested */
	if(inDoResolveAt && *outValue)
	{
		gchar		*resolvedValue;

		/* Resolve value */
		XFDASHBOARD_DEBUG(self, THEME,
							"Resolving css value '%s'",
							*outValue);
		resolvedValue=_xfdashboard_theme_css_resolve_at_identifier_by_string(self,
																				*outValue,
																				inScanner,
																				inScopeSelectors,
																				NULL);
		XFDASHBOARD_DEBUG(self, THEME,
							"Resolved css value '%s' to '%s'",
							*outValue,
							resolvedValue);

		/* Release old value and set new one */
		g_free(*outValue);
		*outValue=resolvedValue;
	}

	/* Strip leading and trailing whitespace from value */
	if(*outValue) g_strstrip(*outValue);

	/* Restore old parser options */
	inScanner->config=oldScannerConfig;
	g_free(scannerConfig);

	/* If no value (means NULL value) is set when '@' identifiers were resolved
	 * then an error is occurred.
	 */
	if(inDoResolveAt && !*outValue) return(G_TOKEN_ERROR);

	/* Successfully parsed */
	return(G_TOKEN_NONE);
}

static GTokenType _xfdashboard_theme_css_parse_css_styles(XfdashboardThemeCSS *self,
															GScanner *inScanner,
															GList *inScopeSelectors,
															gboolean inDoResolveAt,
															GHashTable *ioHashtable)
{
	GTokenType		token;
	gchar			*key;
	gchar			*value;

	g_return_val_if_fail(XFDASHBOARD_IS_THEME_CSS(self), G_TOKEN_ERROR);
	g_return_val_if_fail(inScanner, G_TOKEN_ERROR);
	g_return_val_if_fail(ioHashtable, G_TOKEN_ERROR);

	/* Check that style begin with left curly bracket */
	token=g_scanner_get_next_token(inScanner);
	if(token!=G_TOKEN_LEFT_CURLY) return(G_TOKEN_LEFT_CURLY);

	/* Parse styles until closing right curly bracket is reached */
	token=g_scanner_peek_next_token(inScanner);
	while(token!=G_TOKEN_RIGHT_CURLY)
	{
		/* Reset key and value variables */
		key=value=NULL;

		/* Parse key and value */
		token=_xfdashboard_theme_css_parse_css_key_value(self,
															inScanner,
															inScopeSelectors,
															inDoResolveAt,
															&key,
															&value);
		if(token!=G_TOKEN_NONE) return(token);

		/* Insert key and value into hashtable */
		g_hash_table_insert(ioHashtable, key, value);

		/* Get next token */
		token=g_scanner_peek_next_token(inScanner);
	}

	/* If we get here we expect the right curly bracket */
	token=g_scanner_get_next_token(inScanner);
	if(token!=G_TOKEN_RIGHT_CURLY) return(G_TOKEN_RIGHT_CURLY);

	/* Successfully parsed */
	return(G_TOKEN_NONE);
}

static GTokenType _xfdashboard_theme_css_command_import(XfdashboardThemeCSS *self,
														GScanner *inScanner,
														GList **ioSelectors)
{
	XfdashboardThemeCSSPrivate	*priv;
	GTokenType					token;
	GScannerConfig				*scannerConfig;
	GScannerConfig				*oldScannerConfig;
	gchar						*filename;
	GError						*error;
	gint						oldLineOffset;

	g_return_val_if_fail(XFDASHBOARD_IS_THEME_CSS(self), G_TOKEN_ERROR);
	g_return_val_if_fail(inScanner, G_TOKEN_ERROR);
	g_return_val_if_fail(ioSelectors, G_TOKEN_ERROR);

	priv=self->priv;
	filename=NULL;
	error=NULL;

	/* Set parser option to parse property value and parse them */
	scannerConfig=(GScannerConfig*)g_memdup(inScanner->config, sizeof(GScannerConfig));
	scannerConfig->scan_identifier_1char=1;
	scannerConfig->char_2_token=FALSE;
	scannerConfig->scan_string_sq=TRUE;
	scannerConfig->scan_string_dq=TRUE;

	oldScannerConfig=inScanner->config;
	inScanner->config=scannerConfig;

	/* Syntax is: @import(filename)
	 * Expect '(' as next character in stream.
	 */
	token=g_scanner_get_next_token(inScanner);
	if(token!=G_TOKEN_CHAR ||
		inScanner->value.v_char!='(')
	{
		/* Restore old parser options */
		inScanner->config=oldScannerConfig;
		g_free(scannerConfig);

		/* Show parser error message */
		g_scanner_unexp_token(inScanner,
								G_TOKEN_LEFT_PAREN,
								NULL,
								NULL,
								NULL,
								_("Missing '(' after @import keyword."),
								TRUE);
		return(G_TOKEN_LEFT_PAREN);
	}

	/* Expect only identifiers, characters and strings and concetenate
	 * them to a filename. But stop at closing ')' character.
	 */
	token=g_scanner_get_next_token(inScanner);
	while((token==G_TOKEN_CHAR && inScanner->value.v_char!=')') ||
			token==G_TOKEN_IDENTIFIER ||
			token==G_TOKEN_STRING)
	{
		switch(token)
		{
			case G_TOKEN_CHAR:
				filename=_xfdashboard_theme_css_append_char(filename, inScanner->value.v_char);
				break;

			case G_TOKEN_STRING:
				filename=_xfdashboard_theme_css_append_string(filename, inScanner->value.v_string);
				break;

			case G_TOKEN_IDENTIFIER:
				filename=_xfdashboard_theme_css_append_string(filename, inScanner->value.v_identifier);
				break;

			default:
				/* Restore old parser options */
				inScanner->config=oldScannerConfig;
				g_free(scannerConfig);

				/* Release allocated resources */
				if(filename) g_free(filename);

				/* Show parser error message */
				g_scanner_unexp_token(inScanner,
										G_TOKEN_ERROR,
										NULL,
										NULL,
										NULL,
										_("Unexpected token in filename to import"),
										TRUE);
				return(token);
		}

		token=g_scanner_get_next_token(inScanner);
	}

	if(!filename)
	{
		/* Restore old parser options */
		inScanner->config=oldScannerConfig;
		g_free(scannerConfig);

		/* Show parser error message */
		g_scanner_unexp_token(inScanner,
								G_TOKEN_ERROR,
								NULL,
								NULL,
								NULL,
								_("Missing filename to import"),
								TRUE);
		return(G_TOKEN_ERROR);
	}

	/* Expect closing ')' as next character in stream */
	if(token!=G_TOKEN_CHAR ||
		inScanner->value.v_char!=')')
	{
		/* Restore old parser options */
		inScanner->config=oldScannerConfig;
		g_free(scannerConfig);

		/* Release allocated resources */
		if(filename) g_free(filename);

		/* Show parser error message */
		g_scanner_unexp_token(inScanner,
								G_TOKEN_RIGHT_PAREN,
								NULL,
								NULL,
								NULL,
								_("Missing closing ')' at @import keyword."),
								TRUE);
		return(G_TOKEN_RIGHT_PAREN);
	}

	/* Import and parse CSS file */
	if(!g_path_is_absolute(filename))
	{
		gchar					*tempFilename;
		gchar					*cssPath;
		gboolean				foundFile;

		foundFile=FALSE;

		/* Path to file is relative so check if a file, relative to
		 * path of CSS file currently parsed, exists.
		 */
		if(inScanner->input_fd>0 &&
			inScanner->input_name)
		{
			cssPath=g_path_get_dirname(inScanner->input_name);

			tempFilename=g_build_filename(cssPath, filename, NULL);
			if(g_file_test(tempFilename, G_FILE_TEST_EXISTS))
			{
				XFDASHBOARD_DEBUG(self, THEME,
									"Resolved relative path '%s' to import to '%s' which is relative to current css file '%s'.",
									filename,
									tempFilename,
									inScanner->input_name);

				g_free(filename);
				filename=g_strdup(tempFilename);
				foundFile=TRUE;
			}

			g_free(cssPath);
			g_free(tempFilename);
		}

		/* If current CSS being parsed is not a file or the relative path
		 * to current CSS file to import does not exist, assume it is a
		 * relative path to theme.
		 */
		if(!foundFile)
		{
			tempFilename=g_build_filename(priv->themePath, filename, NULL);

			XFDASHBOARD_DEBUG(self, THEME,
								"Resolved relative path '%s' to import to '%s' which is relative to theme path '%s'.",
								filename,
								tempFilename,
								priv->themePath);

			g_free(filename);
			filename=g_strdup(tempFilename);
			g_free(tempFilename);
		}
	}

	oldLineOffset=priv->offsetLine;
	priv->offsetLine+=g_scanner_cur_line(inScanner);
	if(!xfdashboard_theme_css_add_file(self, filename, GPOINTER_TO_INT(inScanner->user_data), &error))
	{
		gchar					*errorMessage;

		/* Build error message */
		errorMessage=g_strdup_printf(_("Failed to import CSS file '%s': %s"),
										filename,
										error && error->message ? error->message : _("Unknown error"));

		/* Show parser error message */
		g_scanner_unexp_token(inScanner,
								G_TOKEN_ERROR,
								NULL,
								NULL,
								NULL,
								errorMessage,
								TRUE);

		/* Restore old parser options */
		inScanner->config=oldScannerConfig;
		g_free(scannerConfig);

		/* Release allocated resources */
		if(error) g_error_free(error);
		if(errorMessage) g_free(errorMessage);
		if(filename) g_free(filename);

		/* Correct line offset */
		priv->offsetLine-=oldLineOffset;

		/* Return error result */
		return(G_TOKEN_ERROR);
	}

	XFDASHBOARD_DEBUG(self, THEME,
						"Imported CSS file '%s'",
						filename);

	/* Correct line offset */
	priv->offsetLine-=oldLineOffset;

	/* Restore old parser options */
	inScanner->config=oldScannerConfig;
	g_free(scannerConfig);

	/* Release allocated resources */
	if(error) g_error_free(error);
	if(filename) g_free(filename);

	/* Import was successful so return success value */
	return(G_TOKEN_NONE);
}

static gboolean _xfdashboard_theme_css_parse_css_ruleset_finish(XfdashboardCssSelector *inSelector,
																GScanner *inScanner,
																GTokenType inPeekNextToken,
																gpointer inUserData)
{
	g_return_val_if_fail(XFDASHBOARD_IS_CSS_SELECTOR(inSelector), XFDASHBOARD_CSS_SELECTOR_PARSE_FINISH_BAD_STATE);
	g_return_val_if_fail(inScanner, XFDASHBOARD_CSS_SELECTOR_PARSE_FINISH_BAD_STATE);
	g_return_val_if_fail(XFDASHBOARD_IS_THEME_CSS(inUserData), XFDASHBOARD_CSS_SELECTOR_PARSE_FINISH_BAD_STATE);

	/* Scanner is in good state if current token in stream is a comma
	 * or a left curly bracket.
	 */
	if(inPeekNextToken=='{') return(XFDASHBOARD_CSS_SELECTOR_PARSE_FINISH_OK);

	if(inPeekNextToken==',')
	{
		/* Eat comma from stream to adjust cursor to next token */
		g_scanner_get_next_token(inScanner);

		return(XFDASHBOARD_CSS_SELECTOR_PARSE_FINISH_OK);
	}

	/* If we get here scanner is in bad state now */
	return(XFDASHBOARD_CSS_SELECTOR_PARSE_FINISH_BAD_STATE);
}

static GTokenType _xfdashboard_theme_css_parse_css_ruleset(XfdashboardThemeCSS *self,
															GScanner *inScanner,
															GList **ioSelectors)
{
	XfdashboardThemeCSSPrivate		*priv;
	GTokenType						token;
	XfdashboardThemeCSSSelector		*selector;
	gboolean						hasAtSelector;

	g_return_val_if_fail(XFDASHBOARD_IS_THEME_CSS(self), G_TOKEN_ERROR);
	g_return_val_if_fail(inScanner, G_TOKEN_ERROR);
	g_return_val_if_fail(ioSelectors, G_TOKEN_ERROR);

	priv=self->priv;

	/* Parse comma-seperated selectors until a left curly bracket is found */
	selector=NULL;
	hasAtSelector=FALSE;

	token=g_scanner_peek_next_token(inScanner);
	while(token!=G_TOKEN_LEFT_CURLY)
	{
		/* Check if a @-identifier was parsed because it can only be one
		 * in a ruleset and @-identifers and selectors cannot be mixed.
		 */
		if(hasAtSelector)
		{
			g_scanner_get_next_token(inScanner);
			g_scanner_unexp_token(inScanner,
									G_TOKEN_LEFT_CURLY,
									NULL,
									NULL,
									NULL,
									_("Mixing selectors and '@' identifiers or defining more than one '@' identifier at once is not allowed"),
									TRUE);
			return(G_TOKEN_LEFT_CURLY);
		}

		switch((guint)token)
		{
			case G_TOKEN_IDENTIFIER:
			case '*':
			case '#':
			case '.':
			case ':':
			case ',':
				/* A selector must have been defined before other one can follow comma-separated */
				if(token==',' && g_list_length(*ioSelectors)==0)
				{
					g_scanner_unexp_token(inScanner,
											G_TOKEN_IDENTIFIER,
											NULL,
											NULL,
											NULL,
											_("A selector must have been defined before other one can follow comma-separated."),
											TRUE);
					return(token);
				}

				/* Create new selector and add it to list of read-in selectors */
				selector=_xfdashboard_theme_css_selector_new(inScanner->input_name);
				*ioSelectors=g_list_prepend(*ioSelectors, selector);

				/* Parse selector */
				selector->selector=xfdashboard_css_selector_new_from_scanner_with_priority(inScanner,
																							GPOINTER_TO_INT(inScanner->user_data),
																							(XfdashboardCssSelectorParseFinishCallback)_xfdashboard_theme_css_parse_css_ruleset_finish,
																							self);
				if(!selector->selector) return(G_TOKEN_ERROR);

				xfdashboard_css_selector_adjust_to_offset(selector->selector, priv->offsetLine, 0);

				/* If we get here selector could be parse so set type */
				selector->type=XFDASHBOARD_THEME_CSS_SELECTOR_TYPE_SELECTOR;
				break;

			case '@':
				g_scanner_get_next_token(inScanner);

				/* An identifier must follow the character '@' */
				token=g_scanner_get_next_token(inScanner);
				if(token!=G_TOKEN_IDENTIFIER)
				{
					g_scanner_unexp_token(inScanner,
											G_TOKEN_IDENTIFIER,
											NULL,
											NULL,
											NULL,
											_("An identifier must follow '@'"),
											TRUE);
					return(token);
				}

				/* Handle '@constants'-identifier */
				if(g_strcmp0(inScanner->value.v_identifier, "constants")==0)
				{
					/* Create selector for constant @-identifier */
					selector=_xfdashboard_theme_css_selector_new(inScanner->input_name);
					selector->type=XFDASHBOARD_THEME_CSS_SELECTOR_TYPE_CONSTANT;

					/* Add it to list of read-in selectors */
					*ioSelectors=g_list_prepend(*ioSelectors, selector);

					/* Set flag that an '@'-selector was parsed */
					hasAtSelector=TRUE;
				}
					/* Handle '@import'-identifier */
					else if(g_strcmp0(inScanner->value.v_identifier, "import")==0)
					{
						token=_xfdashboard_theme_css_command_import(self,
																	inScanner,
																	ioSelectors);
						if(token!=G_TOKEN_NONE) return(token);
					}
					/* If we get here the '@'-identifier is unknown and could not be handled, skip it */
					else
					{
						gchar		*message;

						/* Build warning message */
						message=g_strdup_printf(_("Skipping block of unknown identifier '@%s'"),
												inScanner->value.v_identifier);

						/* Set warning */
						g_scanner_unexp_token(inScanner,
												G_TOKEN_IDENTIFIER,
												_("'@' identifier"),
												NULL,
												NULL,
												message,
												FALSE);

						/* Release allocated resources */
						g_free(message);

						/* As this is just a warning return G_TOKEN_NONE to continue
						 * parsing CSS. Otherwise parses would stop with error.
						 */
						return(G_TOKEN_NONE);
					}

				break;

			case G_TOKEN_EOF:
				/* If any selector was parsed then a css block must follow.
				 * Reaching end of file now is an error ...
				 */
				if(g_list_length(*ioSelectors)!=0 || selector)
				{
					g_scanner_get_next_token(inScanner);
					g_scanner_unexp_token(inScanner,
											G_TOKEN_ERROR,
											NULL,
											NULL,
											NULL,
											_("Unhandled selector"),
											TRUE);
					return(G_TOKEN_LEFT_CURLY);
				}

				/* ... otherwise it is ok so return G_TOKEN_EOF for
				 * success result and to stop further parses.
				 */
				return(G_TOKEN_EOF);

			default:
				g_scanner_get_next_token(inScanner);
				g_scanner_unexp_token(inScanner,
										G_TOKEN_ERROR,
										NULL,
										NULL,
										NULL,
										_("Unhandled selector"),
										TRUE);
				return(G_TOKEN_LEFT_CURLY);
		}

		/* Continue parsing with next token */
		token=g_scanner_peek_next_token(inScanner);
	}

	/* Successfully parsed */
	return(G_TOKEN_NONE);
}

static GTokenType _xfdashboard_theme_css_parse_css_block(XfdashboardThemeCSS *self,
															GScanner *inScanner,
															GList **ioSelectors,
															GList **ioStyles)
{
	GTokenType						token;
	GList							*selectors;
	GList							*iter;
	GHashTable						*styles;
	XfdashboardThemeCSSSelector		*selector;
	gboolean						doResolveAt;

	g_return_val_if_fail(XFDASHBOARD_IS_THEME_CSS(self), G_TOKEN_ERROR);
	g_return_val_if_fail(inScanner, G_TOKEN_ERROR);
	g_return_val_if_fail(ioSelectors, G_TOKEN_ERROR);
	g_return_val_if_fail(ioStyles, G_TOKEN_ERROR);

	selectors=NULL;
	styles=NULL;

	/* CSS blocks begin with rulesets (list of selectors) - parse them */
	token=_xfdashboard_theme_css_parse_css_ruleset(self, inScanner, &selectors);
	if(token!=G_TOKEN_NONE)
	{
		g_list_free_full(selectors, (GDestroyNotify)_xfdashboard_theme_css_selector_free);

		return(token);
	}

	/* Determine if we should resolve '@' identifiers or to copy property value only
	 * because a '@' selector is parsed.
	 */
	doResolveAt=TRUE;
	for(iter=selectors; iter; iter=g_list_next(iter))
	{
		selector=(XfdashboardThemeCSSSelector*)iter->data;
		if(selector->type==XFDASHBOARD_THEME_CSS_SELECTOR_TYPE_CONSTANT)
		{
			g_assert(g_list_length(selectors)==1);
			doResolveAt=FALSE;
		}
	}

	/* Create a hash table for the properties */
	styles=g_hash_table_new_full(g_str_hash,
									g_str_equal,
									g_free,
									(GDestroyNotify)g_free);

	token=_xfdashboard_theme_css_parse_css_styles(self,
													inScanner,
													*ioSelectors,
													doResolveAt,
													styles);
	if(token!=G_TOKEN_NONE)
	{
		g_list_free_full(selectors, (GDestroyNotify)_xfdashboard_theme_css_selector_free);

		g_hash_table_unref(styles);

		return(token);
	}

	/* Assign all the selectors to this style */
	for(iter=selectors; iter; iter=g_list_next(iter))
	{
		selector=(XfdashboardThemeCSSSelector*)iter->data;
		selector->style=g_hash_table_ref(styles);
	}

	/* Store selectors and styles */
	*ioSelectors=g_list_concat(*ioSelectors, selectors);
	*ioStyles=g_list_append(*ioStyles, styles);

	return(token);
}

static gboolean _xfdashboard_theme_css_parse_css(XfdashboardThemeCSS *self,
													GInputStream *inStream,
													const gchar *inName,
													gint inPriority,
													GList **outSelectors,
													GList **outStyles,
													GError **outError)
{
	XfdashboardThemeCSSPrivate		*priv;
	GScanner						*scanner;
	GTokenType						token;
	gboolean						success;
	GList							*selectors;
	GList							*styles;

	g_return_val_if_fail(XFDASHBOARD_IS_THEME_CSS(self), FALSE);
	g_return_val_if_fail(G_IS_INPUT_STREAM(inStream), FALSE);
	g_return_val_if_fail(outError==NULL || *outError==NULL, FALSE);
	g_return_val_if_fail(outSelectors && *outSelectors==NULL, FALSE);
	g_return_val_if_fail(outStyles && *outStyles==NULL, FALSE);

	priv=self->priv;
	success=TRUE;
	selectors=NULL;
	styles=NULL;

	/* Create scanner object with default settings */
	scanner=g_scanner_new(NULL);
	scanner->input_name=inName;
	scanner->user_data=GINT_TO_POINTER(inPriority);

	/* Set up scanner config
	 * - Identifiers are allowed to contain '-' (minus sign) as non-first characters
	 * - Disallow scanning float values as we need '.' for identifiers
	 * - Set up single comment line not to include '#' as this character is need for identifiers
	 * - Disable parsing HEX values
	 * - Identifiers cannot be single quoted
	 * - Identifiers cannot be double quoted
	 */
	scanner->config->cset_identifier_nth=G_CSET_a_2_z "-_0123456789" G_CSET_A_2_Z G_CSET_LATINS G_CSET_LATINC;
	scanner->config->scan_float=FALSE;
	scanner->config->cpair_comment_single="\1\n";
	scanner->config->scan_hex=FALSE;
	scanner->config->scan_string_sq=FALSE;
	scanner->config->scan_string_dq=FALSE;

	/* Set input stream */
	if(G_IS_FILE_DESCRIPTOR_BASED(inStream))
	{
		g_scanner_input_file(scanner,
								g_file_descriptor_based_get_fd(G_FILE_DESCRIPTOR_BASED(inStream)));
	}
		else
		{
			/* Set error */
			_xfdashboard_theme_css_set_error(self,
												outError,
												XFDASHBOARD_THEME_CSS_ERROR_UNSUPPORTED_STREAM,
												_("The input stream of type %s is not supported"),
												G_OBJECT_TYPE_NAME(inStream));

			/* Destroy scanner */
			g_scanner_destroy(scanner);

			/* Return failure result */
			return(FALSE);
		}

	/* Parse input stream */
	token=g_scanner_peek_next_token(scanner);
	while(token!=G_TOKEN_EOF)
	{
		token=_xfdashboard_theme_css_parse_css_block(self, scanner, &selectors, &styles);
		if(token!=G_TOKEN_NONE) break;

		/* Get next token of input stream */
		token=g_scanner_peek_next_token(scanner);
	}

	/* Check that we reached end of stream when we stopped parsing */
	if(token!=G_TOKEN_EOF)
	{
		/* It is not the end of stream so print parser error message
		 * and set error
		 */
		g_scanner_unexp_token(scanner,
								G_TOKEN_EOF,
								NULL,
								NULL,
								NULL,
								_("Parser did not reach end of stream"),
								TRUE);

		_xfdashboard_theme_css_set_error(self,
											outError,
											XFDASHBOARD_THEME_CSS_ERROR_PARSER_ERROR,
											_("Parser did not reach end of stream"));

		success=FALSE;

		/* Free selectors and styles */
		g_list_free_full(selectors, (GDestroyNotify)_xfdashboard_theme_css_selector_free);
		selectors=NULL;

		g_list_free_full(styles, (GDestroyNotify)g_hash_table_unref);
		styles=NULL;
	}

	/* Add lines parsed in scanner to line offset */
	priv->offsetLine+=g_scanner_cur_line(scanner)+1;

	/* Return selectors and styles */
	if(outSelectors) *outSelectors=selectors;
	if(outStyles) *outStyles=styles;

	/* Destroy scanner */
	g_scanner_destroy(scanner);

	/* Return success result */
	return(success);
}

/* Callback for sorting selector matches by score */
static gint _xfdashboard_theme_css_sort_by_score(XfdashboardThemeCSSSelectorMatch *inLeft,
													XfdashboardThemeCSSSelectorMatch *inRight)
{
	gint							priority;
	guint							line;
	guint							position;
	gint							score;
	XfdashboardCssSelectorRule		*leftRule;
	XfdashboardCssSelectorRule		*rightRule;

	g_assert(inLeft);
	g_assert(inRight);

	score=inLeft->score-inRight->score;
	if(score!=0) return(score);

	/* Get CSS selector rule to compare them */
	g_assert(inLeft->selector && inLeft->selector->selector);
	g_assert(inRight->selector && inRight->selector->selector);

	leftRule=xfdashboard_css_selector_get_rule(inLeft->selector->selector);
	rightRule=xfdashboard_css_selector_get_rule(inRight->selector->selector);

	/* Compare CSS selector rules */
	priority=xfdashboard_css_selector_rule_get_priority(leftRule);
	priority-=xfdashboard_css_selector_rule_get_priority(rightRule);
	if(priority!=0) return(priority);

	line=xfdashboard_css_selector_rule_get_line(leftRule);
	line-=xfdashboard_css_selector_rule_get_line(rightRule);
	if(line!=0) return(line);

	position=xfdashboard_css_selector_rule_get_position(leftRule);
	position-=xfdashboard_css_selector_rule_get_position(rightRule);
	if(position!=0) return(position);

	return(0);
}

/* IMPLEMENTATION: GObject */

/* Dispose this object */
static void _xfdashboard_theme_css_dispose(GObject *inObject)
{
	XfdashboardThemeCSS				*self=XFDASHBOARD_THEME_CSS(inObject);
	XfdashboardThemeCSSPrivate		*priv=self->priv;

	/* Release allocated resources */
	if(priv->themePath)
	{
		g_free(priv->themePath);
		priv->themePath=NULL;
	}

	if(priv->selectors)
	{
		g_list_free_full(priv->selectors, (GDestroyNotify)_xfdashboard_theme_css_selector_free);
		priv->selectors=NULL;
	}

	if(priv->styles)
	{
		g_list_free_full(priv->styles, (GDestroyNotify)g_hash_table_unref);
		priv->styles=NULL;
	}

	if(priv->names)
	{
		g_slist_free_full(priv->names, (GDestroyNotify)g_free);
		priv->names=NULL;
	}

	if(priv->registeredFunctions)
	{
		g_hash_table_unref(priv->registeredFunctions);
		priv->registeredFunctions=NULL;
	}

	/* Call parent's class dispose method */
	G_OBJECT_CLASS(xfdashboard_theme_css_parent_class)->dispose(inObject);
}

/* Set properties */
static void _xfdashboard_theme_css_set_property(GObject *inObject,
												guint inPropID,
												const GValue *inValue,
												GParamSpec *inSpec)
{
	XfdashboardThemeCSS			*self=XFDASHBOARD_THEME_CSS(inObject);
	XfdashboardThemeCSSPrivate	*priv=self->priv;

	switch(inPropID)
	{
		case PROP_THEME_PATH:
			if(priv->themePath)
			{
				g_free(priv->themePath);
				priv->themePath=NULL;
			}

			priv->themePath=g_value_dup_string(inValue);
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
void xfdashboard_theme_css_class_init(XfdashboardThemeCSSClass *klass)
{
	GObjectClass		*gobjectClass=G_OBJECT_CLASS(klass);

	/* Override functions */
	gobjectClass->dispose=_xfdashboard_theme_css_dispose;
	gobjectClass->set_property=_xfdashboard_theme_css_set_property;

	/* Define properties */
	XfdashboardThemeCSSProperties[PROP_THEME_PATH]=
		g_param_spec_string("theme-path",
							_("Theme path"),
							_("Path of theme loading from"),
							NULL,
							G_PARAM_WRITABLE | G_PARAM_STATIC_STRINGS | G_PARAM_CONSTRUCT_ONLY);

	g_object_class_install_properties(gobjectClass, PROP_LAST, XfdashboardThemeCSSProperties);
}

/* Object initialization
 * Create private structure and set up default values
 */
void xfdashboard_theme_css_init(XfdashboardThemeCSS *self)
{
	XfdashboardThemeCSSPrivate		*priv;

	priv=self->priv=xfdashboard_theme_css_get_instance_private(self);

	/* Set default values */
	priv->themePath=NULL;
	priv->selectors=NULL;
	priv->styles=NULL;
	priv->names=NULL;
	priv->registeredFunctions=NULL;
	priv->offsetLine=0;

	/* Register CSS functions */
#define REGISTER_CSS_FUNC(name, callback) \
	_xfdashboard_theme_css_register_function(self, \
												name, \
												XFDASHBOARD_THEME_CSS_FUNCTION_CALLBACK(callback));

	REGISTER_CSS_FUNC("rgb", _xfdashboard_theme_css_function_rgb_rgba);
	REGISTER_CSS_FUNC("rgba", _xfdashboard_theme_css_function_rgb_rgba);
	REGISTER_CSS_FUNC("mix", _xfdashboard_theme_css_function_mix);
	REGISTER_CSS_FUNC("lighter", _xfdashboard_theme_css_function_lighter_darker);
	REGISTER_CSS_FUNC("darker", _xfdashboard_theme_css_function_lighter_darker);
	REGISTER_CSS_FUNC("shade", _xfdashboard_theme_css_function_shade);
	REGISTER_CSS_FUNC("alpha", _xfdashboard_theme_css_function_alpha);
	REGISTER_CSS_FUNC("try_icons", _xfdashboard_theme_css_function_try_icons)

#undef REGISTER_CSS_FUNC
}

/* IMPLEMENTATION: Errors */

GQuark xfdashboard_theme_css_error_quark(void)
{
	return(g_quark_from_static_string("xfdashboard-theme-css-error-quark"));
}

/* IMPLEMENTATION: Public API */

/* Create new instance */
XfdashboardThemeCSS* xfdashboard_theme_css_new(const gchar *inThemePath)
{
	return(XFDASHBOARD_THEME_CSS(g_object_new(XFDASHBOARD_TYPE_THEME_CSS,
												"theme-path", inThemePath,
												NULL)));
}

/* Load a CSS file into theme */
gboolean xfdashboard_theme_css_add_file(XfdashboardThemeCSS *self,
											const gchar *inPath,
											gint inPriority,
											GError **outError)
{
	XfdashboardThemeCSSPrivate		*priv;
	GFile							*file;
	GFileInputStream				*stream;
	GError							*error;
	GList							*selectors;
	GList							*styles;

	g_return_val_if_fail(XFDASHBOARD_IS_THEME_CSS(self), FALSE);
	g_return_val_if_fail(inPath!=NULL && *inPath!=0, FALSE);
	g_return_val_if_fail(outError==NULL || *outError==NULL, FALSE);

	priv=self->priv;
	selectors=NULL;
	styles=NULL;

	/* Load and parse CSS file */
	file=g_file_new_for_path(inPath);
	if(!file)
	{
		_xfdashboard_theme_css_set_error(self,
											outError,
											XFDASHBOARD_THEME_CSS_ERROR_UNSUPPORTED_STREAM,
											_("Could not get file for path '%s'"),
											inPath);

		return(FALSE);
	}

	error=NULL;
	stream=g_file_read(file, NULL, &error);
	if(error)
	{
		g_propagate_error(outError, error);

		/* Release allocated resources */
		g_object_unref(file);

		return(FALSE);
	}

	_xfdashboard_theme_css_parse_css(self,
										G_INPUT_STREAM(stream),
										inPath,
										inPriority,
										&selectors,
										&styles,
										&error);
	if(error)
	{
		g_propagate_error(outError, error);

		/* Release allocated resources */
		g_object_unref(file);

		return(FALSE);
	}

	/* If we get here adding, loading and parsing CSS file was successful
	 * so add filename to list of loaded sources, add selectors and styles
	 * to list of selectors and styles in this theme and return TRUE
	 */
	priv->names=g_slist_prepend(priv->names, strdup(inPath));

	if(selectors)
	{
		priv->selectors=g_list_concat(priv->selectors, selectors);
		XFDASHBOARD_DEBUG(self, THEME,
							"Successfully parsed '%s' and added %d selectors - total %d selectors",
							inPath,
							g_list_length(selectors),
							g_list_length(priv->selectors));
	}

	if(styles)
	{
		priv->styles=g_list_concat(priv->styles, styles);
		XFDASHBOARD_DEBUG(self, THEME,
							"Successfully parsed '%s' and added %d styles - total %d styles",
							inPath,
							g_list_length(styles),
							g_list_length(priv->styles));
	}

	/* Release allocated resources */
	g_object_unref(file);

	return(TRUE);
}

/* Return properties for a stylable actor */
GHashTable* xfdashboard_theme_css_get_properties(XfdashboardThemeCSS *self,
													XfdashboardStylable *inStylable)
{
	XfdashboardThemeCSSPrivate			*priv;
	GList								*entry, *matches;
	XfdashboardThemeCSSSelectorMatch	*match;
	GHashTable							*result;
#ifdef DEBUG
	GTimer								*timer=NULL;
	const gchar							*styleID;
	const gchar							*styleClasses;
	const gchar							*stylePseudoClasses;
	const gchar							*styleTypeName;
	gchar								*styleSelector;
#endif

	g_return_val_if_fail(XFDASHBOARD_IS_THEME_CSS(self), NULL);
	g_return_val_if_fail(XFDASHBOARD_IS_STYLABLE(inStylable), NULL);

	priv=self->priv;
	matches=NULL;
	match=NULL;

#ifdef DEBUG
	styleID=xfdashboard_stylable_get_name(inStylable);
	styleClasses=xfdashboard_stylable_get_classes(inStylable);
	stylePseudoClasses=xfdashboard_stylable_get_pseudo_classes(inStylable);
	styleTypeName=G_OBJECT_TYPE_NAME(inStylable);
	styleSelector=g_strdup_printf("%s%s%s%s%s%s%s",
									(styleTypeName) ? styleTypeName : "",
									(styleClasses) ? "." : "",
									(styleClasses) ? styleClasses : "",
									(styleID) ? "#" : "",
									(styleID) ? styleID : "",
									(stylePseudoClasses) ? ":" : "",
									(stylePseudoClasses) ? stylePseudoClasses : "");
	XFDASHBOARD_DEBUG(self, STYLE,
						"Looking up matches for %s ",
						styleSelector);

	timer=g_timer_new();
#endif

	/* Find and collect matching selectors */
	for(entry=priv->selectors; entry; entry=g_list_next(entry))
	{
		gint							score;
		XfdashboardThemeCSSSelector		*selector;

		selector=(XfdashboardThemeCSSSelector*)entry->data;
		if(selector->type==XFDASHBOARD_THEME_CSS_SELECTOR_TYPE_SELECTOR)
		{
			score=xfdashboard_css_selector_score(selector->selector, inStylable);
			if(score>=0)
			{
				match=g_slice_new(XfdashboardThemeCSSSelectorMatch);
				match->selector=entry->data;
				match->score=score;
				matches=g_list_prepend(matches, match);
			}
		}
	}

	/* Sort matching selectors by their score */
	matches=g_list_sort(matches,
						(GCompareFunc)_xfdashboard_theme_css_sort_by_score);

	/* Get properties from matching selectors' styles */
	result=g_hash_table_new_full(g_str_hash,
									g_str_equal,
									NULL,
									(GDestroyNotify)_xfdashboard_theme_css_value_free);
	for(entry=matches; entry; entry=g_list_next(entry))
	{
		XfdashboardThemeCSSTableCopyData	copyData;

		/* Get selector */
		match=(XfdashboardThemeCSSSelectorMatch*)entry->data;

		/* Copy selector properties to result set */
		copyData.name=xfdashboard_css_selector_rule_get_source(xfdashboard_css_selector_get_rule(match->selector->selector));
		copyData.table=result;

		g_hash_table_foreach(match->selector->style,
								(GHFunc)_xfdashboard_theme_css_copy_table,
								&copyData);
	}

	g_list_free_full(matches, (GDestroyNotify)_xfdashboard_themes_css_selector_match_free);

#ifdef DEBUG
	XFDASHBOARD_DEBUG(self, STYLE,
						"Found %u properties for %s in %f seconds" ,
						g_hash_table_size(result),
						styleSelector,
						g_timer_elapsed(timer, NULL));
	g_timer_destroy(timer);
	g_free(styleSelector);
#endif

	/* Return found properties */
	return(result);
}
