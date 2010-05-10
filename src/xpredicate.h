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

#ifndef XPREDICATE_H
#define XPREDICATE_H

#include "common.h"

typedef struct {
  int offset;
  enum { xpred_num, xpred_att } expr;
  bool_t value;
  union {
    struct {
      int target;
      int count;
    } num;
    struct {
      const char_t *name;
      const char_t *value;
    } att;
  } args;
} xpred_t;

typedef struct {
  const char_t *path;
  xpred_t *list;
  int num;
  size_t max;
} xpredicate_t;

bool_t create_xpredicate(xpredicate_t *xp);
bool_t free_xpredicate(xpredicate_t *xp);
bool_t add_xpredicate(xpredicate_t *xp, int offset, const char_t *predicate );
xpred_t *get_xpredicate(const xpredicate_t *xp, int n);

bool_t compile_xpredicate(xpredicate_t *xp, const char_t *xpath);
bool_t valid_xpredicate(const xpredicate_t *xp);

typedef struct {
  xpredicate_t *list;
  int num;
  size_t max;
} xpredicatelist_t;

bool_t create_xpredicatelist(xpredicatelist_t *xpl);
bool_t free_xpredicatelist(xpredicatelist_t *xpl);
bool_t reset_xpredicatelist(xpredicatelist_t *xpl);
bool_t add_xpredicatelist(xpredicatelist_t *xpl, const char_t *xpath);
xpredicate_t *get_xpredicatelist(const xpredicatelist_t *xpl, int n);

bool_t compile_xpredicatelist(xpredicatelist_t *xpl, cstringlst_t lst);
bool_t update_xpredicatelist(xpredicatelist_t *xpl, const char_t *xpath, const char_t **att);

#endif
