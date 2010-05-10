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

#ifndef SMATCH_H
#define SMATCH_H

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <sys/types.h>
#include <regex.h>

typedef const char_t * csm_t;
typedef struct {
  csm_t *smatch;
  regex_t *sregex;
  size_t max_smatch;
  size_t smatch_count;
  flag_t flags;
} smatcher_t;

#define SMATCH_FLAG_ICASE     0x01
#define SMATCH_FLAG_EXTEND    0x02

/* stack of strings for matching */
bool_t create_smatcher(smatcher_t *sm);
bool_t free_smatcher(smatcher_t *sm);
bool_t reset_smatcher(smatcher_t *sm);

bool_t push_smatcher(smatcher_t *sm, const char_t *s, flag_t flags);
bool_t push_unique_smatcher(smatcher_t *sm, const char_t *s, flag_t flags);
bool_t pop_smatcher(smatcher_t *sm);

const char_t *get_smatcher(smatcher_t *sm, size_t n);
bool_t find_first_smatcher(smatcher_t *sm, const char_t *s, size_t *n);
bool_t find_next_smatcher(smatcher_t *sm, const char_t *s, size_t *n);
bool_t do_match_smatcher(smatcher_t *sm, size_t n, const char_t *s);

#endif
