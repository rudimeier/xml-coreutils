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

#ifndef OBJSTACK_H
#define OBJSTACK_H

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

/* a simple object stack. */

typedef struct {
  byte_t *stack;
  size_t size, nmemb;
  int top;
} objstack_t;

bool_t create_objstack(objstack_t *os, size_t size);
bool_t free_objstack(objstack_t *os);
bool_t clear_objstack(objstack_t *os);

bool_t is_empty_objstack(objstack_t *os);
bool_t push_objstack(objstack_t *os, const byte_t *obj, size_t objsize);
bool_t peek_objstack(objstack_t *os, byte_t *obj, size_t objsize);
bool_t peekn_objstack(objstack_t *os, int n, byte_t *obj, size_t objsize);
bool_t pop_objstack(objstack_t *os, size_t objsize);

/* access like an array */
void *get_objstack(objstack_t *os, int n, size_t objsize);

#endif
