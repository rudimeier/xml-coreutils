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

#ifndef XMATCH_H
#define XMATCH_H

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

typedef const char_t * ccp_t;
typedef struct {
  ccp_t *xpath;
  size_t max_xpath;
  size_t xpath_count;
} xmatcher_t;

/* stack of xpaths for matching */
bool_t create_xmatcher(xmatcher_t *xm);
bool_t free_xmatcher(xmatcher_t *xm);
bool_t reset_xmatcher(xmatcher_t *xm);

bool_t push_xmatcher(xmatcher_t *xm, const char_t *xpath);
bool_t push_unique_xmatcher(xmatcher_t *xm, const char_t *path);
bool_t pop_xmatcher(xmatcher_t *xm);

const char_t *get_xmatcher(xmatcher_t *xm, size_t n);
bool_t find_first_xmatcher(xmatcher_t *xm, const char_t *xpath, size_t *n);
bool_t find_next_xmatcher(xmatcher_t *xm, const char_t *xpath, size_t *n);

#endif
