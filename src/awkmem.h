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

#ifndef AWKMEM_H
#define AWKMEM_H

#include "common.h"

/* can't use real pointers if the memory is resizable */
typedef size_t awkmem_ptr_t;

typedef struct {
  byte_t *start;
  int brk; /* bytes used */
  size_t size; /* bytes allocated */
} awkmem_t;

bool_t create_awkmem(awkmem_t *am);
bool_t free_awkmem(awkmem_t *am);
bool_t reset_awkmem(awkmem_t *am, awkmem_ptr_t p);
awkmem_ptr_t sbrk_awkmem(awkmem_t *am, size_t numbytes);

typedef char_t *awkstring_t;
typedef double awknum_t;

typedef struct {
  awkmem_t rom;
} awkstrings_t;

bool_t create_awkstrings(awkstrings_t *as);
bool_t free_awkstrings(awkstrings_t *as);

awkstring_t string_awkstrings(awkstrings_t *as, awkmem_ptr_t p);
awknum_t number_awkstrings(awkstrings_t *as, awkmem_ptr_t p);


#endif
