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

#ifndef ATTLIST_H
#define ATTLIST_H

#include "common.h"

typedef struct {
  const char_t *name;
  const char_t *value;
} attribute_t;

typedef struct {
  attribute_t *list;
  size_t count;
  size_t max_count;
} attlist_t;

bool_t create_attlist(attlist_t *alist);
bool_t reset_attlist(attlist_t *alist);
bool_t free_attlist(attlist_t *alist);
const attribute_t *get_attlist(attlist_t *alist, size_t i);
bool_t append_attlist(attlist_t *alist, const char_t **att);
bool_t push_attlist(attlist_t *alist, const char_t *name, const char_t *value);
bool_t sort_attlist();

#endif
