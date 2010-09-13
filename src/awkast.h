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

#ifndef AWKAST_H
#define AWKAST_H

#include "common.h"
#include "awkmem.h"
#include "symbols.h"

typedef enum {
  aNULL = 0, aDUMMY
} awkast_node_type_t;

typedef union {
  awkmem_ptr_t ptr;
  symbol_t sym;
} awkast_variant_t;

typedef struct awkast {
  awkast_node_type_t id;
  awkast_variant_t chi; /* child ptr */
  awkast_variant_t sib; /* sibling ptr */
} awkast_node_t;

typedef struct {
  awkmem_t rom;
} awkast_t;

bool_t create_awkast(awkast_t *ast);
bool_t free_awkast(awkast_t *ast);
bool_t clear_awkast(awkast_t *ast);
awkmem_ptr_t mknode_awkast(awkast_t *ast, awkast_node_type_t id,
			   awkast_variant_t chi, awkast_variant_t sib);


#endif
