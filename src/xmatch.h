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

#include "common.h"
#include "xpath.h"

typedef struct {
  cstringlst_t xpath;
  size_t max_xpath;
  int xpath_count;
  flag_t flags;
} xmatcher_t;

#define XMATCHER_STRDUP   0x01 /* make a strdup */
#define XMATCHER_DONTFREE 0x02 /* don't call free on exit */
#define XMATCHER_FREE     0x04 /* call free on exit */

/* stack of xpaths for matching */
bool_t create_xmatcher(xmatcher_t *xm);
bool_t create_wrap_xmatcher(xmatcher_t *xm, cstringlst_t lst);
bool_t free_xmatcher(xmatcher_t *xm);
bool_t reset_xmatcher(xmatcher_t *xm);

bool_t push_xmatcher(xmatcher_t *xm, const char_t *xpath, flag_t flags);
bool_t push_unique_xmatcher(xmatcher_t *xm, const char_t *path, flag_t flags);
bool_t pop_xmatcher(xmatcher_t *xm);

const char_t *get_xmatcher(xmatcher_t *xm, int n);
bool_t find_first_xmatcher(xmatcher_t *xm, const char_t *xpath, int *n);
bool_t find_next_xmatcher(xmatcher_t *xm, const char_t *xpath, int *n);

bool_t find_prefix_xmatcher(xmatcher_t *xm, const xpath_t *xpath);
#endif
