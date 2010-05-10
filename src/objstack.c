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
#include "objstack.h"
#include "mem.h"
#include <string.h>

bool_t create_objstack(objstack_t *os, size_t size) {
  if( os ) {
    os->size = size;
    os->nmemb = 0;
    os->top = 0;
    return create_mem(&os->stack, &os->nmemb, os->size, 16);
  }
  return FALSE;
}

bool_t free_objstack(objstack_t *os) {
  if( os ) {
    os->top = 0;
    return free_mem(&os->stack, &os->nmemb);
  }
  return FALSE;
}

bool_t clear_objstack(objstack_t *os) {
  if( os ) {
    os->top = 0;
    return TRUE;
  }
  return FALSE;
}

bool_t is_empty_objstack(objstack_t *os) {
  return os && (os->top == 0);
}

bool_t push_objstack(objstack_t *os, const byte_t *obj, size_t objsize) {
  byte_t *q;
  if( os && (objsize == os->size) ) {
    if( os->top >= os->nmemb ) {
      grow_mem(&os->stack, &os->nmemb, objsize, 16);
    }
    if( os->top < os->nmemb ) {
      q = os->stack + objsize * os->top;
      memcpy(q, obj, objsize);
      os->top++;
      return TRUE;
    }
  }
  return FALSE;
}

bool_t peek_objstack(objstack_t *os, byte_t *obj, size_t objsize) {
  byte_t *q;
  if( os && obj && (objsize == os->size) && (os->top > 0) ) {
    q = os->stack + objsize * (os->top - 1);
    memcpy(obj, q, objsize);
    return TRUE;
  }
  return FALSE;
}

bool_t peekn_objstack(objstack_t *os, int n, byte_t *obj, size_t objsize) {
  byte_t *q;
  int pos;
  if( os && obj && (objsize == os->size) && (os->top > 0) ) {
    pos = (n > 0) ? MIN(n, os->top - 1) : MAX(os->top - 1 + n, 0);
    q = os->stack + objsize * n;
    memcpy(obj, q, objsize);
    return TRUE;
  }
  return FALSE;
}

bool_t pop_objstack(objstack_t *os, size_t objsize) {
  if( os && (objsize == os->size) && (os->top > 0) ) {
    os->top--;
    return TRUE;
  }
  return FALSE;
}

/* void print_objstack(objstack_t *os, size_t objsize) { */
/*   int i,j,k; */
/*   byte_t *q; */
/*   printf("objstack[\n"); */
/*   k= 0; */
/*   for(i = 0; i < os->top; i++) { */
/*     for(j = 0; j < objsize; j++) { */
/*       printf("%d ", os->stack[k++]); */
/*     } */
/*     printf("\n"); */
/*   } */
/*   printf("]\n"); */
/* } */
