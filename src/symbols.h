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

#ifndef SYMBOL_H
#define SYMBOL_H

#include "common.h"

typedef int symbol_t;

typedef struct {
  unsigned int id : 31;
  unsigned int mark : 1;
  size_t pos;
} jump_t;

typedef struct {
  jump_t *hash;
  int num;
  size_t maxnum;
} jumptable_t;

/* This class builds a list of (immutable) strings which can be
 * searched quickly.  When a string is found, the position in the list
 * identifies it uniquely.  The strings are stored in a trie for fast
 * retrieval. In case the input strings have common prefixes, this is
 * also more memory efficient.
 */
typedef struct {
  byte_t *bigs;
  size_t bigmax;
  int bigs_len;
  jumptable_t jt;
  symbol_t last;
} symbols_t;


bool_t create_symbols(symbols_t *sb);
bool_t free_symbols(symbols_t *sb);
symbol_t getvalue_symbols(symbols_t *sb, bool_t generate, const char_t *begin, const char_t *end);

#endif
