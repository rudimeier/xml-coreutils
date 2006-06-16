/* 
 * Copyright (C) 2006 Laird Breyer
 *  
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
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

#ifndef SKIP_H
#define SKIP_H

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "cursor.h"
#include "fbparser.h"

typedef enum { 
  any, 
  eq_depth, gt_depth, gte_depth, lt_depth, lte_depth
} skiptype_t;
typedef struct {
  int count;
  int depth;
  skiptype_t what;
  flag_t nodemask;
} skip_t;

bool_t forward_skip(skip_t *skip, cursor_t *cursor, fbparser_t *fbp);

#endif
 
