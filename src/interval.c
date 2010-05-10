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
#include "interval.h"

bool_t create_intervalmgr(intervalmgr_t *im) {
  if( im ) {
    return create_objstack(&im->iset, sizeof(interval_t));
  }
  return FALSE;
}

bool_t free_intervalmgr(intervalmgr_t *im) {
  if( im ) {
    return free_objstack(&im->iset);
  }
  return FALSE;
}

bool_t push_intervalmgr(intervalmgr_t *im, int a, int b) {
  interval_t i;
  if( im ) {
    i.a = a;
    i.b = b;
    return push_objstack(&im->iset, (byte_t *)&i, sizeof(interval_t));
  }
  return FALSE;
}

bool_t pop_intervalmgr(intervalmgr_t *im) {
  if( im ) {
    return pop_objstack(&im->iset, sizeof(interval_t)); 
  } 
  return FALSE;
}

bool_t peek_intervalmgr(intervalmgr_t *im, int *a, int *b) {
  interval_t i;
  if( im && a && b ) {
    if( peek_objstack(&im->iset, (byte_t *)&i, sizeof(interval_t)) ) {
      *a = i.a;
      *b = i.b;
      return TRUE;
    }
  }
  return FALSE;
}

bool_t memberof_intervalmgr(intervalmgr_t *im, int x) {
  int i;
  interval_t *ilist;
  if( im ) {
    ilist = (interval_t *)im->iset.stack;
    for(i = 0; i < im->iset.top; i++) {
      if( (ilist[i].a <= x) && (x <= ilist[i].b) ) {
	return TRUE;
      }
    }
  }
  return FALSE;
}

bool_t is_empty_intervalmgr(intervalmgr_t *im) {
  return ( !im || (im->iset.top == 0) );
}
