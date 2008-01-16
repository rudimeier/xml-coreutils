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

#ifndef STRINGLIST_H
#define STRINGLIST_H

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

typedef struct {
  char_t **list;
  size_t num;
  size_t max;
} stringlist_t;

bool_t create_stringlist(stringlist_t *sl);
bool_t free_stringlist(stringlist_t *sl);
bool_t reset_stringlist(stringlist_t *sl);
bool_t add_stringlist(stringlist_t *sl, const char_t *name);
bool_t add_unique_stringlist(stringlist_t *sl, const char_t *name);
const char_t *get_stringlist(stringlist_t *sl, size_t n);

#endif
