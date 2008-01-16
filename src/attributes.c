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
#include "attributes.h"
#include "mem.h"

#include <string.h>

bool_t create_attlist(attlist_t *alist) {
  if( alist ) {
    alist->count = 0;
    return ( create_mem(&alist->list, &alist->max_count, 
			sizeof(attribute_t), 16) &&
	     reset_attlist(alist) );
  }
  return FALSE;
}

bool_t free_attlist(attlist_t *alist) {
  if( alist ) {
    if( alist->list ) {
      free(alist->list);
      alist->list = NULL;
      alist->max_count = 0;
    }
    return TRUE;
  }
  return FALSE;
}

bool_t reset_attlist(attlist_t *alist) {
  if( alist ) {
    alist->count = 0;
    return TRUE;
  }
  return FALSE;
}

bool_t append_attlist(attlist_t *alist, const char_t **att) {
  if( alist && att ) {
    while( att[0] ) {
      if( !push_attlist(alist, att[0], att[1]) ) {
	break;
      }
      att += 2;
    }
    return TRUE;
  }
  return FALSE;
}

bool_t push_attlist(attlist_t *alist, const char_t *name, const char_t *value) {
  if( alist ) {
    if( alist->count >= alist->max_count ) {
      grow_mem(&alist->list, &alist->max_count, sizeof(attribute_t), 16);
    }
    if( alist->count < alist->max_count ) {
      alist->list[alist->count].name = name;
      alist->list[alist->count].value = value;
      alist->count++;
      return TRUE;
    }
  }
  return FALSE;
}

bool_t pop_attlist(attlist_t *alist) {
  if( alist ) {
    if( alist->count > 0 ) {
      alist->count--;
      return TRUE;
    } 
  }
  return FALSE;
}

const attribute_t *get_attlist(attlist_t *alist, size_t i) {
  return (alist && (i < alist->count) && (i >= 0)) ? &alist->list[i] : NULL;
}

int cmp_attribute(const void *a, const void *b) {
  attribute_t *aa = (attribute_t *)a;
  attribute_t *ab = (attribute_t *)b;
  return ( aa->name && ab->name ) ? strcmp(aa->name, ab->name) : 0;
}

bool_t sort_attlist(attlist_t *alist) {
  if( alist ) {
    qsort(alist->list, alist->count, sizeof(attribute_t), cmp_attribute);
    return TRUE;
  }
  return FALSE;
}
