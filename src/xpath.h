/* 
 * Copyright (C) 2006 Laird Breyer
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
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA.
 * 
 * Author:   Laird Breyer <laird@lbreyer.com>
 */

#ifndef XPATH_H
#define XPATH_H

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

typedef struct {
  char_t *path;
  size_t pathlen;
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
  n_dfault = 0x100
} nodetype_t;

#define NODEMASK_ALL (flag_t)(n_start_tag|n_end_tag|n_chardata|n_comment|n_pidata|n_dfault)
#define NODEMASK_TAGS (flag_t)(n_start_tag|n_end_tag)
#define NODEMASK_PARSER (flag_t)(-1)

typedef bool_t (iterator_xpath_fun)(xpath_t *xp, tagtype_t t, const char_t *begin, const char_t *end, void *user);


/* allows easy updating of xpath one node at a time */
bool_t create_xpath(xpath_t *xp);
bool_t free_xpath(xpath_t *xp);
bool_t reset_xpath(xpath_t *xp);
bool_t copy_xpath(xpath_t *dest, xpath_t *src);
bool_t append_xpath(xpath_t *xp, const char_t *begin, size_t len);

bool_t push_tag_xpath(xpath_t *xp, const char_t *tagname);
bool_t push_node_xpath(xpath_t *xp, nodetype_t t);
bool_t push_predicate_xpath(xpath_t *xp, const char_t *pred);
bool_t pop_xpath(xpath_t *xp);

size_t length_xpath(xpath_t *xp);
bool_t is_absolute_xpath(xpath_t *xp);
bool_t is_relative_xpath(xpath_t *xp);

bool_t normalize_xpath(xpath_t *xp);
bool_t write_xpath(xpath_t *xp, const char_t *path, size_t pathlen);
bool_t read_first_element_xpath(xpath_t *xp, char_t **begin, char_t **end, char_t **from);
bool_t read_next_element_xpath(xpath_t *xp, char_t **begin, char_t **end, char_t **from);
tagtype_t identify_element_xpath(const char_t *begin, const char_t *end);

const char_t *get_last_xpath(xpath_t *xp);
const char_t *get_full_xpath(xpath_t *xp);
bool_t iterate_xpath(xpath_t *xp, iterator_xpath_fun *f, void *user);

#endif
