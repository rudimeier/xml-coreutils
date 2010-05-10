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

/* This class doesn't use cstring_t, to make implementation of
 * argv_stringlist() easy. Besides, the class isn't supposed to modify
 * the strings anyway.
 */
typedef struct {
  cstringlst_t list;
  int num;
  size_t max;
  flag_t flags;
} stringlist_t;

#define STRINGLIST_STRDUP   0x01 /* make a strdup */
#define STRINGLIST_DONTFREE 0x02 /* don't call free on exit */
#define STRINGLIST_FREE     0x04 /* call free on exit */

bool_t create_stringlist(stringlist_t *sl);
bool_t free_stringlist(stringlist_t *sl);
bool_t reset_stringlist(stringlist_t *sl);
bool_t add_stringlist(stringlist_t *sl, const char_t *name, flag_t flags);
bool_t add_unique_stringlist(stringlist_t *sl, const char_t *name, flag_t flags);
const char_t *get_stringlist(const stringlist_t *sl, int n);
int find_stringlist(const stringlist_t *sl, const char_t *s);
const char **argv_stringlist(stringlist_t *sl);
bool_t strikeout_stringlist(stringlist_t *sl, const char_t *name);

#endif
