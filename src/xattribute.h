/* 
 * Copyright (C) 2010 Laird Breyer
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

#ifndef XATTRIBUTE_H
#define XATTRIBUTE_H

#include "common.h"

typedef struct {
  const char_t *path;
  const char_t *begin;
  const char_t *end;
  bool_t precheck;
} xattribute_t;

bool_t create_xattribute(xattribute_t *xa);
bool_t free_xattribute(xattribute_t *xa);
bool_t add_xattribute(xattribute_t *xa, int offset, const char_t *name);

bool_t compile_xattribute(xattribute_t *xa, const char_t *xpath);
bool_t match_xattribute(const xattribute_t *xa, const char_t *name);
bool_t precheck_xattribute(const xattribute_t *xa);

typedef struct {
  xattribute_t *list;
  int num;
  size_t max;
} xattributelist_t;

bool_t create_xattributelist(xattributelist_t *xal);
bool_t free_xattributelist(xattributelist_t *xal);
bool_t reset_xattributelist(xattributelist_t *xal);
bool_t add_xattributelist(xattributelist_t *xal, const char_t *xpath);
xattribute_t *get_xattributelist(const xattributelist_t *xal, int n);

bool_t compile_xattributelist(xattributelist_t *xal, cstringlst_t lst);
bool_t check_xattributelist(const xattributelist_t *xal, const char_t *xpath, const char_t *name);
bool_t update_xattributelist(xattributelist_t *xal, const char_t **att);

#endif
