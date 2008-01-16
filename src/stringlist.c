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

#include "common.h"
#include "stringlist.h"
#include "mem.h"
#include "error.h"
#include <string.h>

bool_t create_stringlist(stringlist_t *sl) {
  if( sl ) {
    sl->num = 0;
    return grow_mem(&sl->list, &sl->max, sizeof(char_t *), 16);
  }
  return FALSE;
}

bool_t free_stringlist(stringlist_t *sl) {
  if( sl ) {
    if( sl->list ) {
      reset_stringlist(sl);
      free_mem(&sl->list, &sl->max);
    }
  }
  return FALSE;
}

bool_t reset_stringlist(stringlist_t *sl) {
  size_t i;
  if( sl ) {
    for(i = 0; i < sl->num; i++) {
      free(sl->list[i]);
    }
    sl->num = 0;
    return TRUE;
  }
  return FALSE;
}

const char_t *get_stringlist(stringlist_t *sl, size_t n) {
  return (sl && (n < sl->num)) ? sl->list[n] : NULL;
}

bool_t add_stringlist(stringlist_t *sl, const char_t *name) {
  if( sl ) {
    if( sl->num >= sl->max ) {
      grow_mem(&sl->list, &sl->max, sizeof(char_t *), 16);
    }
    if( sl->num < sl->max ) {
      sl->list[sl->num] = strdup(name);
      sl->num++;
      return TRUE;
    }
  }
  return FALSE;
}

bool_t add_unique_stringlist(stringlist_t *sl, const char_t *name) {
  size_t i; 
  if( sl ) {
    for(i = 0; i < sl->num; i++) {
      if( strcmp(name, sl->list[i]) == 0 ) {
	return FALSE;
      }
    }
    return add_stringlist(sl, name);
  }
  return FALSE;
}
