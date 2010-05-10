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

#ifndef STDSELECT_H
#define STDSELECT_H

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "xpath.h"
#include "xmatch.h"
#include "nhistory.h"
#include "xpredicate.h"
#include "xattribute.h"

/* XPATH selection framework. Sets some variables for the user to query */
typedef struct {
  bool_t active; /* is the current node selected? */
  bool_t attrib; /* is an (existing) attribute of the current tag selected? */
  unsigned int mindepth; /* min depth seen so far */
  unsigned int maxdepth; /* max depth seen so far */

  /* reserved not for user */
  xmatcher_t paths;
  xpredicatelist_t preds;
  xattributelist_t atts;
  nhistory_t history;
} stdselect_t;

bool_t create_stdselect(stdselect_t *sel);
bool_t reset_stdselect(stdselect_t *sel);
bool_t free_stdselect(stdselect_t *sel);

bool_t setup_xpaths_stdselect(stdselect_t *sel, cstringlst_t xpaths);

bool_t activate_tag_stdselect(stdselect_t *sel, int depth, const xpath_t *xpath, const char_t **att);
bool_t activate_stringval_stdselect(stdselect_t *sel, int depth, const xpath_t *xpath);
bool_t activate_node_stdselect(stdselect_t *sel, int depth, const xpath_t *xpath);
bool_t activate_attribute_stdselect(stdselect_t *sel, int depth, const xpath_t *xpath, const char_t *name);

#endif
