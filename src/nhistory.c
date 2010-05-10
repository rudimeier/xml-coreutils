/* 
 * Copyright (C) 2009 Laird Breyer
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

#include "common.h"
#include "nhistory.h"
#include "mem.h"
#include <string.h>

bool_t create_nhistory(nhistory_t *nh) {
  if( nh ) {
    return create_objstack(&nh->node_memo, sizeof(nhistory_node_t));
  }
  return FALSE;
}

bool_t free_nhistory(nhistory_t *nh) {
  if( nh ) {
    return free_objstack(&nh->node_memo);
  }
  return FALSE;
}

bool_t reset_nhistory(nhistory_t *nh) {
  if( nh ) {
    clear_objstack(&nh->node_memo);
    return TRUE;
  }
  return FALSE;
}

bool_t push_level_nhistory(nhistory_t *nh, int level) {
  nhistory_node_t dummy = { 0 }; /* everything inactive */
  if( nh ) {
    if( level == nh->node_memo.top ) {
      return push_objstack(&nh->node_memo, 
			   (byte_t *)&dummy, sizeof(nhistory_node_t));
    }
  }
  return FALSE;
}

bool_t pop_level_nhistory(nhistory_t *nh, int level) {
  if( nh ) {
    if( (level + 1) == nh->node_memo.top ) {
      return pop_objstack(&nh->node_memo, sizeof(nhistory_node_t));
    }
  }
  return FALSE;
}

nhistory_node_t *get_node_nhistory(nhistory_t *nh, int level) {
  nhistory_node_t *n = NULL;
  if( nh && (nh->node_memo.top == (level + 1)) ) {
    n = (nhistory_node_t *)nh->node_memo.stack + level;
  }
  return n;
}

