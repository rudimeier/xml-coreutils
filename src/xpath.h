/* 
 * Copyright (C) 2006 Laird Breyer
 *  
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA.
 * 
 * Author:   Laird Breyer <laird@lbreyer.com>
 */

#ifndef XPATH_H
#define XPATH_H

#include "common.h"
#include "cstring.h"

/* What is an xpath?
 *
 * The xpath_t is one of the most pervasive structures in xml-coreutils.
 * Technically, it is simply a string, that looks roughly like a W3C XPath,
 * but can be used for matching nodes (xmatch.c) as well as building them 
 * (echo.c).
 * The tags are delimited by slashes, and there can be embedded attributes
 * delimited by at-sign and equal-sign, and embedded predicates delimited
 * by square brackets. 
 *
 * The xpath_t is structured like a stack when adding or removing elements,
 * and various navigation and search functions are available. These functions
 * typically skip over attributes and predicates as if they didn't exist.
 * This is by design, as these pieces of information are considered 
 * "secondary". If you need to search or match them directly, see the
 * xpredicate_t and xattribute_t objects which do so efficiently.
 * 
 */
typedef struct {
  cstring_t path;
} xpath_t;

typedef enum { empty, root, current, parent, simpletag, complextag } tagtype_t;

/* nodetype is sometimes used as a bit flag. CAREFUL: order is important,
 * as we want to be able to say (nodetype > space) etc. 
 */
typedef enum { 
  n_none = 0x00, 
  n_space = 0x01, 
  n_start_tag = 0x02, 
  n_end_tag = 0x04, 
  n_chardata = 0x08, 
  n_comment = 0x10, 
  n_pidata = 0x20, 
  n_start_cdata = 0x40,
  n_end_cdata = 0x80,
  n_start_doctype = 0x100,
  n_end_doctype = 0x200,
  n_entitydecl = 0x400,
  n_dfault = 0x800
} nodetype_t;

#define NODEMASK_ALL (flag_t)(n_start_tag|n_end_tag|n_chardata|n_comment|n_pidata|n_dfault)
#define NODEMASK_TAGS (flag_t)(n_start_tag|n_end_tag)
#define NODEMASK_PARSER (flag_t)(-1)

typedef bool_t (iterator_xpath_fun)(xpath_t *xp, tagtype_t t, const char_t *begin, const char_t *end, void *user);


/* allows easy updating of xpath one node at a time */
bool_t create_xpath(xpath_t *xp);
bool_t free_xpath(xpath_t *xp);
bool_t reset_xpath(xpath_t *xp);
bool_t copy_xpath(xpath_t *dest, const xpath_t *src);
bool_t append_xpath(xpath_t *xp, const char_t *begin, size_t len);

bool_t push_tag_xpath(xpath_t *xp, const char_t *tagname);
bool_t pop_xpath(xpath_t *xp);
bool_t push_attribute_xpath(xpath_t *xp, const char_t *name);
bool_t pop_attribute_xpath(xpath_t *xp);
bool_t push_attributes_values_xpath(xpath_t *xp, const char_t **att);
bool_t push_node_xpath(xpath_t *xp, nodetype_t t);
bool_t push_predicate_xpath(xpath_t *xp, const char_t *pred);

size_t length_xpath(xpath_t *xp);
bool_t is_absolute_xpath(const xpath_t *xp);
bool_t is_relative_xpath(const xpath_t *xp);

bool_t normalize_xpath(xpath_t *xp);
bool_t relativize_xpath(xpath_t *xpath, const xpath_t *from);
bool_t retarget_xpath(xpath_t *xpath, const xpath_t *to);

bool_t write_xpath(xpath_t *xp, const char_t *path, size_t pathlen);
bool_t read_first_element_xpath(xpath_t *xp, char_t **begin, char_t **end, char_t **from);
bool_t read_next_element_xpath(xpath_t *xp, char_t **begin, char_t **end, char_t **from);
tagtype_t identify_element_xpath(const char_t *begin, const char_t *end);

const char_t *get_last_xpath(xpath_t *xp);
const char_t *get_full_xpath(xpath_t *xp);
bool_t iterate_xpath(xpath_t *xp, iterator_xpath_fun *f, void *user);
bool_t close_xpath(xpath_t *xp, iterator_xpath_fun *f, void *user);
int cmp_no_attributes_xpath(const char_t *p1, const char_t *p2);
int match_no_att_no_pred_xpath(const char_t *patbegin, const char_t *patend, 
			       const char_t *xpath);
bool_t equal_no_attributes_xpath(const xpath_t *xp1, const xpath_t *xp2);
bool_t equal_xpath(const xpath_t *xp1, const xpath_t *xp2);

__inline__ static const char_t *string_xpath(const xpath_t *xp) {
  return xp ? p_cstring(&xp->path) : NULL;
}

/* returns TRUE if prefix (pattern) matches a string prefix of path. */
__inline__ static bool_t match_prefix_xpath(const char_t *prefix, const char_t *path) {
  return (match_no_att_no_pred_xpath(prefix, NULL, path) <= 0);
}

#endif
