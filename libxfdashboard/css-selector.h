/*
 * css-selector: A CSS simple selector class
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

#ifndef __LIBXFDASHBOARD_CSS_SELECTOR__
#define __LIBXFDASHBOARD_CSS_SELECTOR__

#if !defined(__LIBXFDASHBOARD_H_INSIDE__) && !defined(LIBXFDASHBOARD_COMPILATION)
#error "Only <libxfdashboard/libxfdashboard.h> can be included directly."
#endif

#include <glib-object.h>
#include <glib.h>

#include <libxfdashboard/stylable.h>

G_BEGIN_DECLS

#define XFDASHBOARD_TYPE_CSS_SELECTOR				(xfdashboard_css_selector_get_type())
#define XFDASHBOARD_CSS_SELECTOR(obj)				(G_TYPE_CHECK_INSTANCE_CAST((obj), XFDASHBOARD_TYPE_CSS_SELECTOR, XfdashboardCssSelector))
#define XFDASHBOARD_IS_CSS_SELECTOR(obj)			(G_TYPE_CHECK_INSTANCE_TYPE((obj), XFDASHBOARD_TYPE_CSS_SELECTOR))
#define XFDASHBOARD_CSS_SELECTOR_CLASS(klass)		(G_TYPE_CHECK_CLASS_CAST((klass), XFDASHBOARD_TYPE_CSS_SELECTOR, XfdashboardCssSelectorClass))
#define XFDASHBOARD_IS_CSS_SELECTOR_CLASS(klass)	(G_TYPE_CHECK_CLASS_TYPE((klass), XFDASHBOARD_TYPE_CSS_SELECTOR))
#define XFDASHBOARD_CSS_SELECTOR_GET_CLASS(obj)		(G_TYPE_INSTANCE_GET_CLASS((obj), XFDASHBOARD_TYPE_CSS_SELECTOR, XfdashboardCssSelectorClass))

typedef struct _XfdashboardCssSelector				XfdashboardCssSelector; 
typedef struct _XfdashboardCssSelectorPrivate		XfdashboardCssSelectorPrivate;
typedef struct _XfdashboardCssSelectorClass			XfdashboardCssSelectorClass;

struct _XfdashboardCssSelector
{
	/*< private >*/
	/* Parent instance */
	GObject							parent_instance;

	/* Private structure */
	XfdashboardCssSelectorPrivate	*priv;
};

struct _XfdashboardCssSelectorClass
{
	/*< private >*/
	/* Parent class */
	GObjectClass					parent_class;

	/*< public >*/
	/* Virtual functions */
};

typedef struct _XfdashboardCssSelectorRule			XfdashboardCssSelectorRule;

/* Public declarations */
#define XFDASHBOARD_CSS_SELECTOR_PARSE_FINISH_OK			TRUE
#define XFDASHBOARD_CSS_SELECTOR_PARSE_FINISH_BAD_STATE		FALSE

typedef gboolean (*XfdashboardCssSelectorParseFinishCallback)(XfdashboardCssSelector *inSelector,
																GScanner *inScanner,
																GTokenType inPeekNextToken,
																gpointer inUserData);

/* Public API */
GType xfdashboard_css_selector_get_type(void) G_GNUC_CONST;

XfdashboardCssSelector* xfdashboard_css_selector_new_from_string(const gchar *inSelector);
XfdashboardCssSelector* xfdashboard_css_selector_new_from_string_with_priority(const gchar *inSelector, gint inPriority);
XfdashboardCssSelector* xfdashboard_css_selector_new_from_scanner(GScanner *ioScanner,
																	XfdashboardCssSelectorParseFinishCallback inFinishCallback,
																	gpointer inUserData);
XfdashboardCssSelector* xfdashboard_css_selector_new_from_scanner_with_priority(GScanner *ioScanner,
																				gint inPriority,
																				XfdashboardCssSelectorParseFinishCallback inFinishCallback,
																				gpointer inUserData);

gchar* xfdashboard_css_selector_to_string(XfdashboardCssSelector *self);

gint xfdashboard_css_selector_score(XfdashboardCssSelector *self, XfdashboardStylable *inStylable);

void xfdashboard_css_selector_adjust_to_offset(XfdashboardCssSelector *self, gint inLine, gint inPosition);

XfdashboardCssSelectorRule* xfdashboard_css_selector_get_rule(XfdashboardCssSelector *self);

const gchar* xfdashboard_css_selector_rule_get_type(XfdashboardCssSelectorRule *inRule);
const gchar* xfdashboard_css_selector_rule_get_id(XfdashboardCssSelectorRule *inRule);
const gchar* xfdashboard_css_selector_rule_get_classes(XfdashboardCssSelectorRule *inRule);
const gchar* xfdashboard_css_selector_rule_get_pseudo_classes(XfdashboardCssSelectorRule *inRule);
XfdashboardCssSelectorRule* xfdashboard_css_selector_rule_get_parent(XfdashboardCssSelectorRule *inRule);
XfdashboardCssSelectorRule* xfdashboard_css_selector_rule_get_ancestor(XfdashboardCssSelectorRule *inRule);
const gchar* xfdashboard_css_selector_rule_get_source(XfdashboardCssSelectorRule *inRule);
gint xfdashboard_css_selector_rule_get_priority(XfdashboardCssSelectorRule *inRule);
guint xfdashboard_css_selector_rule_get_line(XfdashboardCssSelectorRule *inRule);
guint xfdashboard_css_selector_rule_get_position(XfdashboardCssSelectorRule *inRule);

G_END_DECLS

#endif	/* __LIBXFDASHBOARD_CSS_SELECTOR__ */
