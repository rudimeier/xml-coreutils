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

#ifndef NHISTORY_H
#define NHISTORY_H

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "objstack.h"

typedef enum { undefined = 0, active, inactive } activity_t;

typedef struct {
  activity_t node, tag, stringval;
} nhistory_node_t;

typedef struct {
  objstack_t node_memo;
} nhistory_t;

bool_t create_nhistory(nhistory_t *nh);
bool_t free_nhistory(nhistory_t *nh);
bool_t reset_nhistory(nhistory_t *nh);

bool_t push_level_nhistory(nhistory_t *nh, int level);
bool_t pop_level_nhistory(nhistory_t *nh, int level);
nhistory_node_t *get_node_nhistory(nhistory_t *nh, int level);

/* memoize: never run the same expensive calculation twice */
#define MEMOIZE_BOOL_NHIST(cheap,expensive) \
  cheap = (((cheap) == undefined) ? ((expensive) ? active : inactive) : (cheap))

#endif
