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

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <libxfdashboard/css-selector.h>

#include <glib/gi18n-lib.h>

#include <libxfdashboard/compat.h>
#include <libxfdashboard/debug.h>


/* Define this class in GObject system */
struct _XfdashboardCssSelectorPrivate
{
	/* Properties related */
	gint							priority;

	/* Instance related */
	XfdashboardCssSelectorRule		*rule;
};

G_DEFINE_TYPE_WITH_PRIVATE(XfdashboardCssSelector,
				xfdashboard_css_selector,
				G_TYPE_OBJECT)

/* Properties */
enum
{
	PROP_0,

	PROP_PRIORITY,

	PROP_LAST
};

static GParamSpec* XfdashboardCssSelectorProperties[PROP_LAST]={ 0, };

/* IMPLEMENTATION: Private variables and methods */
typedef enum /*< skip,prefix=XFDASHBOARD_CSS_SELECTOR_RULE_MODE >*/
{
	XFDASHBOARD_CSS_SELECTOR_RULE_MODE_NONE=0,
	XFDASHBOARD_CSS_SELECTOR_RULE_MODE_PARENT,
	XFDASHBOARD_CSS_SELECTOR_RULE_MODE_ANCESTOR
} XfdashboardCssSelectorRuleMode;

struct _XfdashboardCssSelectorRule
{
	gchar							*type;
	gchar							*id;
	gchar							*classes;
	gchar							*pseudoClasses;
	XfdashboardCssSelectorRule		*parentRule;
	XfdashboardCssSelectorRuleMode	parentRuleMode;

	gchar							*source;
	gint							priority;
	guint							line;
	guint							position;

	guint							origLine;
	guint							origPosition;
};

/* Create rule */
static XfdashboardCssSelectorRule* _xfdashboard_css_selector_rule_new(const gchar *inSource,
																		gint inPriority,
																		guint inLine,
																		guint inPosition)
{
	XfdashboardCssSelectorRule		*rule;

	rule=g_slice_new0(XfdashboardCssSelectorRule);
	rule->source=g_strdup(inSource);
	rule->priority=inPriority;
	rule->origLine=rule->line=inLine;
	rule->origPosition=rule->position=inPosition;

	return(rule);
}

/* Destroy selector */
static void _xfdashboard_css_selector_rule_free(XfdashboardCssSelectorRule *inRule)
{
	g_return_if_fail(inRule);

	/* Free allocated resources */
	if(inRule->type) g_free(inRule->type);
	if(inRule->id) g_free(inRule->id);
	if(inRule->classes) g_free(inRule->classes);
	if(inRule->pseudoClasses) g_free(inRule->pseudoClasses);
	if(inRule->source) g_free(inRule->source);

	/* Destroy parent selector */
	if(inRule->parentRule) _xfdashboard_css_selector_rule_free(inRule->parentRule);

	/* Free selector itself */
	g_slice_free(XfdashboardCssSelectorRule, inRule);
}

/* Get string for selector */
static gchar* _xfdashboard_css_selector_rule_to_string(XfdashboardCssSelectorRule *inRule)
{
	gchar							*parentSelector;
	gchar							*selector;

	g_return_val_if_fail(inRule, NULL);

	selector=NULL;
	parentSelector=NULL;

	/* If a parent selector is available get a string representation of it first */
	if(inRule->parentRule)
	{
		gchar						*temp;

		switch(inRule->parentRuleMode)
		{
			case XFDASHBOARD_CSS_SELECTOR_RULE_MODE_ANCESTOR:
			case XFDASHBOARD_CSS_SELECTOR_RULE_MODE_PARENT:
				temp=_xfdashboard_css_selector_rule_to_string(inRule->parentRule);
				if(!temp)
				{
					g_critical(_("Could not create string for parent css selector"));
					return(NULL);
				}
				break;

			default:
				g_critical(_("Invalid mode for parent rule in CSS selector"));
				return(NULL);
		}

		parentSelector=g_strdup_printf("%s%s ",
										temp,
										(inRule->parentRuleMode==XFDASHBOARD_CSS_SELECTOR_RULE_MODE_PARENT) ? " >" : "");

		g_free(temp);
	}

	/* Build string for selector */
	selector=g_strdup_printf("%s%s%s%s%s%s%s%s",
								(parentSelector) ? parentSelector : "",
								(inRule->type) ? inRule->type : "",
								(inRule->id) ? "#" : "",
								(inRule->id) ? inRule->id : "",
								(inRule->classes) ? "." : "",
								(inRule->classes) ? inRule->classes : "",
								(inRule->pseudoClasses) ? ":" : "",
								(inRule->pseudoClasses) ? inRule->pseudoClasses : "");

	/* Release allocated resources not needed anymore */
	if(parentSelector) g_free(parentSelector);

	/* Return newly created string for selector */
	return(selector);
}

/* Check if haystack contains needle.
 * The haystack is a string representing a list which entries is seperated
 * by a seperator character. This function looks up the haystack if it
 * contains an entry matching the needle and returns TRUE in this case.
 * Otherwise FALSE is returned. A needle length of -1 signals that needle
 * is a NULL-terminated string and length should be determine automatically.
 */
static gboolean _xfdashboard_css_selector_list_contains(const gchar *inNeedle,
														gint inNeedleLength,
														const gchar *inHaystack,
														gchar inSeperator)
{
	const gchar		*start;

	g_return_val_if_fail(inNeedle && *inNeedle!=0, FALSE);
	g_return_val_if_fail(inNeedleLength>0 || inNeedleLength==-1, FALSE);
	g_return_val_if_fail(inHaystack && *inHaystack!=0, FALSE);
	g_return_val_if_fail(inSeperator, FALSE);

	/* If given length of needle is negative it is a NULL-terminated string */
	if(inNeedleLength<0) inNeedleLength=strlen(inNeedle);

	/* Lookup needle in haystack */
	for(start=inHaystack; start; start=strchr(start, inSeperator))
	{
		gint		length;
		gchar		*nextEntry;

		/* Move to character after separator */
		if(start[0]==inSeperator) start++;

		/* Find end of this haystack entry */
		nextEntry=strchr(start, inSeperator);
		if(!nextEntry) length=strlen(start);
			else length=nextEntry-start;

		/* If enrty in haystack is not of same length as needle,
		 * then it is not a match
		 */
		if(length!=inNeedleLength) continue;

		if(!strncmp(inNeedle, start, inNeedleLength)) return(TRUE);
	}

	/* Needle was not found */
	return(FALSE);
}

/* Check and score this selector against stylable node.
 * A score below 0 means that they did not match.
 */
static gint _xfdashboard_css_selector_score_node(XfdashboardCssSelectorRule *inRule,
															XfdashboardStylable *inStylable)
{
	gint					score;
	gint					a, b, c;
	const gchar				*classes;
	const gchar				*pseudoClasses;
	const gchar				*id;

	g_return_val_if_fail(inRule, -1);
	g_return_val_if_fail(XFDASHBOARD_IS_STYLABLE(inStylable), -1);

	/* For information about how the scoring is done, see documentation
	 * "Cascading Style Sheets, level 1" of W3C, section "3.2 Cascading order"
	 * URL: http://www.w3.org/TR/2008/REC-CSS1-20080411/#cascading-order
	 *
	 * 1. Find all declarations that apply to the element/property in question.
	 *    Declarations apply if the selector matches the element in question.
	 *    If no declarations apply, the inherited value is used. If there is
	 *    no inherited value (this is the case for the 'HTML' element and
	 *    for properties that do not inherit), the initial value is used.
	 * 2. Sort the declarations by explicit weight: declarations marked
	 *    '!important' carry more weight than unmarked (normal) declarations.
	 * 3. Sort by origin: the author's style sheets override the reader's
	 *    style sheet which override the UA's default values. An imported
	 *    style sheet has the same origin as the style sheet from which it
	 *    is imported.
	 * 4. Sort by specificity of selector: more specific selectors will
	 *    override more general ones. To find the specificity, count the
	 *    number of ID attributes in the selector (a), the number of CLASS
	 *    attributes in the selector (b), and the number of tag names in
	 *    the selector (c). Concatenating the three numbers (in a number
	 *    system with a large base) gives the specificity.
	 *    Pseudo-elements and pseudo-classes are counted as normal elements
	 *    and classes, respectively.
	 * 5. Sort by order specified: if two rules have the same weight, the
	 *    latter specified wins. Rules in imported style sheets are considered
	 *    to be before any rules in the style sheet itself.
	 *
	 * NOTE: Keyword '!important' is not supported.
	 */
	a=b=c=0;

	/* Get properties for given stylable */
	id=xfdashboard_stylable_get_name(XFDASHBOARD_STYLABLE(inStylable));
	classes=xfdashboard_stylable_get_classes(XFDASHBOARD_STYLABLE(inStylable));
	pseudoClasses=xfdashboard_stylable_get_pseudo_classes(XFDASHBOARD_STYLABLE(inStylable));

	/* Check and score type of selectors but ignore NULL or universal selectors */
	if(inRule->type && inRule->type[0]!='*')
	{
		GType						ruleTypeID;
		GType						nodeTypeID;

		/* Get type of this rule */
		ruleTypeID=g_type_from_name(inRule->type);
		if(!ruleTypeID) return(-1);

		/* Get type of other rule to check against and score it */
		nodeTypeID=G_OBJECT_TYPE(inStylable);
		if(!nodeTypeID) return(-1);

		/* Check if type of this rule matches type of other rule */
		if(!g_type_is_a(nodeTypeID, ruleTypeID)) return(-1);

		/* Determine depth difference between both types
		 * which is the score of this test with a maximum of 99
		 */
		c=g_type_depth(ruleTypeID)-g_type_depth(nodeTypeID);
		c=MAX(ABS(c), 99);
	}

	/* Check and score ID */
	if(inRule->id)
	{
		/* If node has no ID return immediately */
		if(!id || strcmp(inRule->id, id)) return(-1);

		/* Score ID */
		a+=10;
	}

	/* Check and score classes */
	if(inRule->classes)
	{
		gchar				*needle;
		gint				numberMatches;

		/* If node has no pseudo class return immediately */
		if(!classes) return(-1);

		/* Check that each class from the selector's rule appears in the
		 * list of classes from the node, i.e. the selector's rule class list
		 * is a subset of the node's class list
		 */
		numberMatches=0;
		for(needle=inRule->classes; needle; needle=strchr(needle, '.'))
		{
			gint			needleLength;
			gchar			*nextNeedle;

			/* Move pointer of needle beyond class seperator '.' */
			if(needle[0]=='.') needle++;

			/* Get length of needle */
			nextNeedle=strchr(needle, '.');
			if(nextNeedle) needleLength=nextNeedle-needle;
				else needleLength=strlen(needle);

			/* If pseudo-class from the selector does not appear in the
			 * list of pseudo-classes from the node, then this is not a
			 * match
			 */
			if(!_xfdashboard_css_selector_list_contains(needle, needleLength, classes, '.')) return(-1);
			numberMatches++;
		}

		/* Score matching class */
		b=b+(10*numberMatches);
	}

	/* Check and score pseudo classes */
	if(inRule->pseudoClasses)
	{
		gchar				*needle;
		gint				numberMatches;

		/* If node has no pseudo class return immediately */
		if(!pseudoClasses) return(-1);

		/* Check that each pseudo-class from the selector appears in the
		 * pseudo-classes from the node, i.e. the selector pseudo-class list
		 * is a subset of the node's pseudo-class list
		 */
		numberMatches=0;
		for(needle=inRule->pseudoClasses; needle; needle=strchr(needle, ':'))
		{
			gint			needleLength;
			gchar			*nextNeedle;

			/* Move pointer of needle beyond pseudo-class seperator ':' */
			if(needle[0]==':') needle++;

			/* Get length of needle */
			nextNeedle=strchr(needle, ':');
			if(nextNeedle) needleLength=nextNeedle-needle;
				else needleLength=strlen(needle);

			/* If pseudo-class from the selector does not appear in the
			 * list of pseudo-classes from the node, then this is not a
			 * match
			 */
			if(!_xfdashboard_css_selector_list_contains(needle, needleLength, pseudoClasses, ':')) return(-1);
			numberMatches++;
		}

		/* Score matching pseudo-class */
		b=b+(10*numberMatches);
	}

	/* Check and score parent */
	if(inRule->parentRule &&
		inRule->parentRuleMode==XFDASHBOARD_CSS_SELECTOR_RULE_MODE_PARENT)
	{
		gint					parentScore;
		XfdashboardStylable		*parent;

		/* If node has no parent, no parent can match ;) so return immediately */
		parent=xfdashboard_stylable_get_parent(inStylable);
		if(!parent || !XFDASHBOARD_IS_STYLABLE(parent)) return(-1);

		/* Check if there are matching parents. If not return immediately. */
		parentScore=_xfdashboard_css_selector_score_node(inRule->parentRule, parent);
		if(parentScore<0) return(-1);

		/* Score matching parents */
		c+=parentScore;
	}

	/* Check and score ancestor */
	if(inRule->parentRule &&
		inRule->parentRuleMode==XFDASHBOARD_CSS_SELECTOR_RULE_MODE_ANCESTOR)
	{
		gint					ancestorScore;
		XfdashboardStylable		*ancestor;

		ancestor=inStylable;

		/* If node has no parents, no ancestor can match so return immediately */
		do
		{
			ancestor=xfdashboard_stylable_get_parent(ancestor);
		}
		while(ancestor && !XFDASHBOARD_IS_STYLABLE(ancestor));

		if(!ancestor || !XFDASHBOARD_IS_STYLABLE(ancestor)) return(-1);

		/* Iterate through ancestors and check and score them */
		while(ancestor)
		{
			/* Get number of matches for ancestor and if at least one matches,
			 * stop search and score
			 */
			ancestorScore=_xfdashboard_css_selector_score_node(inRule->parentRule, ancestor);
			if(ancestorScore>=0)
			{
				c+=ancestorScore;
				break;
			}

			/* Get next ancestor to check but skip actors not implementing
			 * the XfdashboardStylable interface
			 */
			do
			{
				ancestor=xfdashboard_stylable_get_parent(ancestor);
			}
			while(ancestor && !XFDASHBOARD_IS_STYLABLE(ancestor));

			if(!ancestor || !XFDASHBOARD_IS_STYLABLE(ancestor)) return(-1);
		}
	}

	/* Calculate final score */
	score=(a*10000)+(b*100)+c;
	return(score);
}

/* Parse selector */
static GTokenType _xfdashboard_css_selector_parse_css_simple_selector(XfdashboardCssSelector *self,
																		GScanner *inScanner,
																		XfdashboardCssSelectorRule *ioRule)
{
	GTokenType		token;

	g_return_val_if_fail(XFDASHBOARD_IS_CSS_SELECTOR(self), G_TOKEN_ERROR);
	g_return_val_if_fail(inScanner, G_TOKEN_ERROR);
	g_return_val_if_fail(ioRule, G_TOKEN_ERROR);

	/* Parse type of selector. It is optional as '*' can be used as wildcard */
	token=g_scanner_peek_next_token(inScanner);
	switch((guint)token)
	{
		case '*':
			g_scanner_get_next_token(inScanner);
			ioRule->type=g_strdup("*");

			/* Check if next token follows directly after this identifier.
			 * It is determined by checking if scanner needs to move more than
			 * one (the next) character. If there is a gap then either a new
			 * selector follows or it is a new typeless selector.
			 */
			token=g_scanner_peek_next_token(inScanner);
			if(inScanner->next_line==g_scanner_cur_line(inScanner) &&
				(inScanner->next_position-g_scanner_cur_position(inScanner))>1)
			{
				return(G_TOKEN_NONE);
			}
			break;

		case G_TOKEN_IDENTIFIER:
			g_scanner_get_next_token(inScanner);
			ioRule->type=g_strdup(inScanner->value.v_identifier);

			/* Check if next token follows directly after this identifier.
			 * It is determined by checking if scanner needs to move more than
			 * one (the next) character. If there is a gap then either a new
			 * selector follows or it is a new typeless selector.
			 */
			token=g_scanner_peek_next_token(inScanner);
			if(inScanner->next_line==g_scanner_cur_line(inScanner) &&
				(inScanner->next_position-g_scanner_cur_position(inScanner))>1)
			{
				return(G_TOKEN_NONE);
			}
			break;

		default:
			break;
	}

	/* Here we look for '#', '.' or ':' and return if we find anything else */
	token=g_scanner_peek_next_token(inScanner);
	while(token!=G_TOKEN_NONE)
	{
		switch((guint)token)
		{
			/* Parse ID */
			case '#':
				g_scanner_get_next_token(inScanner);
				token=g_scanner_get_next_token(inScanner);
				if(token!=G_TOKEN_IDENTIFIER)
				{
					g_scanner_unexp_token(inScanner,
											G_TOKEN_IDENTIFIER,
											NULL,
											NULL,
											NULL,
											_("Invalid name identifier"),
											TRUE);
					return(G_TOKEN_ERROR);
				}

				/* Return immediately if ID was already set because it should be a new child but print a debug message */
				if(ioRule->id)
				{
					XFDASHBOARD_DEBUG(self, STYLE,
										"Unexpected new ID '%s' at rule %p for previous ID '%s' at line %d and position %d",
										inScanner->value.v_identifier,
										ioRule,
										ioRule->id,
										g_scanner_cur_line(inScanner),
										g_scanner_cur_position(inScanner));
					return(G_TOKEN_NONE);
				}

				ioRule->id=g_strdup(inScanner->value.v_identifier);
				break;

			/* Parse class */
			case '.':
				g_scanner_get_next_token(inScanner);
				token=g_scanner_get_next_token(inScanner);
				if(token!=G_TOKEN_IDENTIFIER)
				{
					g_scanner_unexp_token(inScanner,
											G_TOKEN_IDENTIFIER,
											NULL,
											NULL,
											NULL,
											_("Invalid class identifier"),
											TRUE);
					return(G_TOKEN_ERROR);
				}

				if(ioRule->classes)
				{
					/* Remember old classes as it can only be freed afterwards */
					gchar		*oldClasses=ioRule->classes;

					/* Create new list of classes */
					ioRule->classes=g_strconcat(ioRule->classes,
												".",
												inScanner->value.v_identifier,
												NULL);

					/* Now free old classes */
					g_free(oldClasses);
				}
					else
					{
						ioRule->classes=g_strdup(inScanner->value.v_identifier);
					}
				break;

			/* Parse pseudo-class */
			case ':':
				g_scanner_get_next_token(inScanner);
				token=g_scanner_get_next_token(inScanner);
				if(token!=G_TOKEN_IDENTIFIER)
				{
					g_scanner_unexp_token(inScanner,
											G_TOKEN_IDENTIFIER,
											NULL,
											NULL,
											NULL,
											_("Invalid pseudo-class identifier"),
											TRUE);
					return(G_TOKEN_ERROR);
				}

				if(ioRule->pseudoClasses)
				{
					/* Remember old pseudo-classes as it can only be freed afterwards */
					gchar		*oldPseudoClasses=ioRule->pseudoClasses;

					/* Create new list of pseudo-classes */
					ioRule->pseudoClasses=g_strconcat(ioRule->pseudoClasses,
														":",
														inScanner->value.v_identifier,
														NULL);

					/* Now free old pseudo-classes */
					g_free(oldPseudoClasses);
				}
					else
					{
						ioRule->pseudoClasses=g_strdup(inScanner->value.v_identifier);
					}
				break;

			default:
				return(G_TOKEN_NONE);
		}

		/* Get next token */
		token=g_scanner_peek_next_token(inScanner);
	}

	/* Successfully parsed */
	return(G_TOKEN_NONE);
}

static GTokenType _xfdashboard_css_selector_parse_css_rule(XfdashboardCssSelector *self,
															GScanner *inScanner)
{
	XfdashboardCssSelectorPrivate	*priv;
	GTokenType						token;
	XfdashboardCssSelectorRule		*rule, *parentRule;

	g_return_val_if_fail(XFDASHBOARD_IS_CSS_SELECTOR(self), G_TOKEN_ERROR);
	g_return_val_if_fail(inScanner, G_TOKEN_ERROR);

	priv=self->priv;

	/* Parse comma-seperated selectors until a left curly bracket is found */
	parentRule=NULL;
	rule=NULL;

	token=g_scanner_peek_next_token(inScanner);
	while(token!=G_TOKEN_EOF)
	{
		switch((guint)token)
		{
			case G_TOKEN_IDENTIFIER:
			case '*':
			case '#':
			case '.':
			case ':':
				/* Set last selector as parent if available */
				if(rule) parentRule=rule;
					else parentRule=NULL;

				/* Create new selector */
				rule=_xfdashboard_css_selector_rule_new(inScanner->input_name,
														priv->priority,
														g_scanner_cur_line(inScanner),
														g_scanner_cur_position(inScanner));
				priv->rule=rule;

				/* Check if there was a previous selector and if so, the new one
				 * should use the previous selector to match an ancestor
				 */
				if(parentRule)
				{
					rule->parentRule=parentRule;
					rule->parentRuleMode=XFDASHBOARD_CSS_SELECTOR_RULE_MODE_ANCESTOR;
				}

				/* Parse selector */
				token=_xfdashboard_css_selector_parse_css_simple_selector(self, inScanner, rule);
				if(token!=G_TOKEN_NONE) return(token);
				break;

			case '>':
				g_scanner_get_next_token(inScanner);

				/* Set last selector as parent selector */
				if(!rule)
				{
					g_scanner_unexp_token(inScanner,
											G_TOKEN_IDENTIFIER,
											NULL,
											NULL,
											NULL,
											_("No parent when parsing '>'"),
											TRUE);
					return(G_TOKEN_ERROR);
				}
				parentRule=rule;

				/* Create new selector */
				rule=_xfdashboard_css_selector_rule_new(inScanner->input_name,
														priv->priority,
														g_scanner_cur_line(inScanner),
														g_scanner_cur_position(inScanner));
				priv->rule=rule;

				/* Link parent to the new selector as parent selector */
				rule->parentRule=parentRule;
				rule->parentRuleMode=XFDASHBOARD_CSS_SELECTOR_RULE_MODE_PARENT;


				/* Parse selector */
				token=_xfdashboard_css_selector_parse_css_simple_selector(self, inScanner, rule);
				if(token!=G_TOKEN_NONE) return(token);
				break;

			default:
				/* Stop at first invalid character in stream and
				 * return with success result.
				 */
				return(G_TOKEN_NONE);
		}

		/* Continue parsing with next token */
		token=g_scanner_peek_next_token(inScanner);
	}

	/* Eat "eof" token */
	if(token==G_TOKEN_EOF) token=g_scanner_get_next_token(inScanner);

	/* Successfully parsed */
	return(G_TOKEN_EOF);
}

static gboolean _xfdashboard_css_selector_parse(XfdashboardCssSelector *self, GScanner *ioScanner)
{
	GScannerConfig			*oldScannerConfig;
	GScannerConfig			*scannerConfig;
	gboolean				success;
	GTokenType				token;

	g_return_val_if_fail(XFDASHBOARD_IS_CSS_SELECTOR(self), FALSE);
	g_return_val_if_fail(ioScanner, FALSE);

	success=TRUE;

	/* Set up scanner configuration for parsing css selectors:
	 * - Identifiers are allowed to contain '-' (minus sign) as non-first characters
	 * - Disallow scanning float values as we need '.' for identifiers
	 * - Set up single comment line not to include '#' as this character is need for identifiers
	 * - Disable parsing HEX values
	 * - Identifiers cannot be single quoted
	 * - Identifiers cannot be double quoted
	 */
	scannerConfig=(GScannerConfig*)g_memdup(ioScanner->config, sizeof(GScannerConfig));
	scannerConfig->cset_skip_characters=" \n\r\t";
	scannerConfig->cset_identifier_nth=G_CSET_a_2_z "-_0123456789" G_CSET_A_2_Z G_CSET_LATINS G_CSET_LATINC;
	scannerConfig->scan_float=FALSE;
	scannerConfig->cpair_comment_single="\1\n";
	scannerConfig->scan_hex=FALSE;
	scannerConfig->scan_string_sq=FALSE;
	scannerConfig->scan_string_dq=FALSE;

	/* Set new scanner configuration but remember old one to restore it later */
	oldScannerConfig=ioScanner->config;
	ioScanner->config=scannerConfig;

	/* Parse input stream */
	token=g_scanner_peek_next_token(ioScanner);
	if(token!=G_TOKEN_EOF)
	{
		token=_xfdashboard_css_selector_parse_css_rule(self, ioScanner);
		if(token==G_TOKEN_ERROR)
		{
			g_warning(_("Failed to parse css selector."));
			success=FALSE;
		}
	}
		else
		{
			g_warning(_("Failed to parse css selector because stream is empty."));
			success=FALSE;
		}

	/* Restore old scanner configuration */
	ioScanner->config=oldScannerConfig;

	/* Release allocated resources */
	g_free(scannerConfig);

	/* Return success result */
	return(success);
}

/* IMPLEMENTATION: GObject */

/* Dispose this object */
static void _xfdashboard_css_selector_dispose(GObject *inObject)
{
	XfdashboardCssSelector			*self=XFDASHBOARD_CSS_SELECTOR(inObject);
	XfdashboardCssSelectorPrivate	*priv=self->priv;

	/* Release allocated resources */
	if(priv->rule)
	{
		_xfdashboard_css_selector_rule_free(priv->rule);
		priv->rule=NULL;
	}

	/* Call parent's class dispose method */
	G_OBJECT_CLASS(xfdashboard_css_selector_parent_class)->dispose(inObject);
}

/* Set/get properties */
static void _xfdashboard_css_selector_set_property(GObject *inObject,
													guint inPropID,
													const GValue *inValue,
													GParamSpec *inSpec)
{
	XfdashboardCssSelector		*self=XFDASHBOARD_CSS_SELECTOR(inObject);

	switch(inPropID)
	{
		case PROP_PRIORITY:
			self->priv->priority=g_value_get_int(inValue);
			break;

		default:
			G_OBJECT_WARN_INVALID_PROPERTY_ID(inObject, inPropID, inSpec);
			break;
	}
}

static void _xfdashboard_css_selector_get_property(GObject *inObject,
													guint inPropID,
													GValue *outValue,
													GParamSpec *inSpec)
{
	XfdashboardCssSelector		*self=XFDASHBOARD_CSS_SELECTOR(inObject);

	switch(inPropID)
	{
		case PROP_PRIORITY:
			g_value_set_int(outValue, self->priv->priority);
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
static void xfdashboard_css_selector_class_init(XfdashboardCssSelectorClass *klass)
{
	GObjectClass				*gobjectClass=G_OBJECT_CLASS(klass);

	/* Override functions */
	gobjectClass->set_property=_xfdashboard_css_selector_set_property;
	gobjectClass->get_property=_xfdashboard_css_selector_get_property;
	gobjectClass->dispose=_xfdashboard_css_selector_dispose;

	/* Define properties */
	XfdashboardCssSelectorProperties[PROP_PRIORITY]=
		g_param_spec_int("priority",
							_("Priority"),
							_("The priority of this CSS selector"),
							G_MININT, G_MAXINT,
							0,
							G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS | G_PARAM_CONSTRUCT_ONLY);

	g_object_class_install_properties(gobjectClass, PROP_LAST, XfdashboardCssSelectorProperties);
}

/* Object initialization
 * Create private structure and set up default values
 */
static void xfdashboard_css_selector_init(XfdashboardCssSelector *self)
{
	XfdashboardCssSelectorPrivate	*priv;

	priv=self->priv=xfdashboard_css_selector_get_instance_private(self);

	/* Set up default values */
	priv->priority=0;
	priv->rule=NULL;
}

/* IMPLEMENTATION: Public API */

/* Create new instance of CSS selector by string */
XfdashboardCssSelector* xfdashboard_css_selector_new_from_string(const gchar *inSelector)
{
	return(xfdashboard_css_selector_new_from_string_with_priority(inSelector, G_MININT));
}

XfdashboardCssSelector* xfdashboard_css_selector_new_from_string_with_priority(const gchar *inSelector, gint inPriority)
{
	GObject				*selector;
	GScanner			*scanner;

	g_return_val_if_fail(inSelector || *inSelector, NULL);

	/* Create selector instance */
	selector=g_object_new(XFDASHBOARD_TYPE_CSS_SELECTOR,
							"priority", inPriority,
							NULL);
	if(!selector)
	{
		g_warning(_("Could not create selector."));
		return(NULL);
	}

	/* Create scanner for requested string */
	scanner=g_scanner_new(NULL);
	g_scanner_input_text(scanner, inSelector, strlen(inSelector));

	/* Parse string with created scanner */
	if(!_xfdashboard_css_selector_parse(XFDASHBOARD_CSS_SELECTOR(selector), scanner))
	{
		g_object_unref(selector);
		selector=NULL;
	}

	/* If scanner does not point to EOF after parsing, it is an error and selector
	 * needs to be destroyed.
	 */
	if(selector &&
		!g_scanner_eof(scanner))
	{
		/* It is not the end of string so print parser error message */
		g_scanner_unexp_token(scanner,
								G_TOKEN_EOF,
								NULL,
								NULL,
								NULL,
								_("Parser did not reach end of stream"),
								TRUE);

		g_object_unref(selector);
		selector=NULL;
	}

	/* Destroy allocated resources */
	g_scanner_destroy(scanner);

	/* Return created selector which may be NULL in case of error */
	return(XFDASHBOARD_CSS_SELECTOR(selector));
}

/* Create new instance of CSS selector with initialized scanner.
 * Parsing stops at first invalid token for a selector. The callee is responsible
 * to check if scanner points to a valid token in his context (e.g. CSS ruleset,
 * a comma-separated list of CSS selectos) and to unref the returned selector
 * if scanner points to an invalid token.
 */
XfdashboardCssSelector* xfdashboard_css_selector_new_from_scanner(GScanner *ioScanner,
																	XfdashboardCssSelectorParseFinishCallback inFinishCallback,
																	gpointer inUserData)
{
	return(xfdashboard_css_selector_new_from_scanner_with_priority(ioScanner, G_MININT, inFinishCallback, inUserData));
}

XfdashboardCssSelector* xfdashboard_css_selector_new_from_scanner_with_priority(GScanner *ioScanner,
																				gint inPriority,
																				XfdashboardCssSelectorParseFinishCallback inFinishCallback,
																				gpointer inUserData)
{
	GObject				*selector;

	g_return_val_if_fail(ioScanner, NULL);
	g_return_val_if_fail(!g_scanner_eof(ioScanner), NULL);

	/* Create selector instance */
	selector=g_object_new(XFDASHBOARD_TYPE_CSS_SELECTOR,
							"priority", inPriority,
							NULL);
	if(!selector)
	{
		g_warning(_("Could not create selector."));
		return(NULL);
	}

	/* Parse selector from scanner provided  */
	if(!_xfdashboard_css_selector_parse(XFDASHBOARD_CSS_SELECTOR(selector), ioScanner))
	{
		g_object_unref(selector);
		return(NULL);
	}

	/* If a callback is given to call after parsing finished so call it now
	 * to determine if scanner is still in good state. If it is in bad state
	 * then return NULL.
	 */
	if(inFinishCallback)
	{
		gboolean		goodState;

		goodState=(inFinishCallback)(XFDASHBOARD_CSS_SELECTOR(selector), ioScanner, g_scanner_peek_next_token(ioScanner), inUserData);
		if(!goodState)
		{
			g_scanner_unexp_token(ioScanner,
									G_TOKEN_ERROR,
									NULL,
									NULL,
									NULL,
									_("Unexpected state of CSS scanner."),
									TRUE);
			g_object_unref(selector);
			return(NULL);
		}
	}

	/* Return created selector which may be NULL in case of error */
	return(XFDASHBOARD_CSS_SELECTOR(selector));
}

/* Get string for selector.
 * Free string returned with g_free().
 */
gchar* xfdashboard_css_selector_to_string(XfdashboardCssSelector *self)
{
	XfdashboardCssSelectorPrivate	*priv;
	gchar							*selector;

	g_return_val_if_fail(XFDASHBOARD_IS_CSS_SELECTOR(self), NULL);

	priv=self->priv;
	selector=NULL;

	/* Get string for selector */
	if(priv->rule)
	{
		selector=_xfdashboard_css_selector_rule_to_string(priv->rule);
	}

	/* Return newly created string for selector */
	return(selector);
}

/* Check and score this selector against a stylable node.
 * A score below 0 means that they did not match.
 */
gint xfdashboard_css_selector_score(XfdashboardCssSelector *self, XfdashboardStylable *inStylable)
{
	g_return_val_if_fail(XFDASHBOARD_IS_CSS_SELECTOR(self), -1);
	g_return_val_if_fail(XFDASHBOARD_IS_STYLABLE(inStylable), -1);

	/* Check and score rules */
	return(_xfdashboard_css_selector_score_node(self->priv->rule, inStylable));
}

/* Adjust source line and position of this selector to an offset */
void xfdashboard_css_selector_adjust_to_offset(XfdashboardCssSelector *self, gint inLine, gint inPosition)
{
	XfdashboardCssSelectorPrivate	*priv;
	gint							newLine;
	gint							newPosition;

	g_return_if_fail(XFDASHBOARD_IS_CSS_SELECTOR(self));

	priv=self->priv;

	/* Adjust to offset */
	if(priv->rule)
	{
		newLine=inLine+priv->rule->origLine;
		priv->rule->line=MAX(0, newLine);

		newPosition=inPosition+priv->rule->origPosition;
		priv->rule->position=MAX(0, newPosition);
	}
}

/* Get rule parsed */
XfdashboardCssSelectorRule* xfdashboard_css_selector_get_rule(XfdashboardCssSelector *self)
{
	g_return_val_if_fail(XFDASHBOARD_IS_CSS_SELECTOR(self), NULL);

	return(self->priv->rule);
}

/* Get values from rule */
const gchar* xfdashboard_css_selector_rule_get_type(XfdashboardCssSelectorRule *inRule)
{
	g_return_val_if_fail(inRule, NULL);

	return(inRule->type);
}

const gchar* xfdashboard_css_selector_rule_get_id(XfdashboardCssSelectorRule *inRule)
{
	g_return_val_if_fail(inRule, NULL);

	return(inRule->id);
}

const gchar* xfdashboard_css_selector_rule_get_classes(XfdashboardCssSelectorRule *inRule)
{
	g_return_val_if_fail(inRule, NULL);

	return(inRule->classes);
}

const gchar* xfdashboard_css_selector_rule_get_pseudo_classes(XfdashboardCssSelectorRule *inRule)
{
	g_return_val_if_fail(inRule, NULL);

	return(inRule->pseudoClasses);
}

XfdashboardCssSelectorRule* xfdashboard_css_selector_rule_get_parent(XfdashboardCssSelectorRule *inRule)
{
	g_return_val_if_fail(inRule, NULL);

	if(inRule->parentRuleMode!=XFDASHBOARD_CSS_SELECTOR_RULE_MODE_PARENT) return(NULL);
	return(inRule->parentRule);
}

XfdashboardCssSelectorRule* xfdashboard_css_selector_rule_get_ancestor(XfdashboardCssSelectorRule *inRule)
{
	g_return_val_if_fail(inRule, NULL);

	if(inRule->parentRuleMode!=XFDASHBOARD_CSS_SELECTOR_RULE_MODE_ANCESTOR) return(NULL);
	return(inRule->parentRule);
}

const gchar* xfdashboard_css_selector_rule_get_source(XfdashboardCssSelectorRule *inRule)
{
	g_return_val_if_fail(inRule, NULL);

	return(inRule->source);
}

gint xfdashboard_css_selector_rule_get_priority(XfdashboardCssSelectorRule *inRule)
{
	g_return_val_if_fail(inRule, -1);

	return(inRule->priority);
}

guint xfdashboard_css_selector_rule_get_line(XfdashboardCssSelectorRule *inRule)
{
	g_return_val_if_fail(inRule, -1);

	return(inRule->line);
}

guint xfdashboard_css_selector_rule_get_position(XfdashboardCssSelectorRule *inRule)
{
	g_return_val_if_fail(inRule, -1);

	return(inRule->position);
}
