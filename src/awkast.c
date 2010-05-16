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

#include "awkast.h"

bool_t create_awkast(awkast_t *ast) {
  if( ast ) {
    return create_awkmem(&ast->rom);
  }
  return FALSE;
}

bool_t free_awkast(awkast_t *ast) {
  if( ast ) {
    return free_awkmem(&ast->rom);
  }
  return FALSE;
}

bool_t clear_awkast(awkast_t *ast) {
  if( ast ) {
    sbrk_awkmem(&ast->rom, 0);
    return TRUE;
  }
  return FALSE;
}

awkmem_ptr_t mknode_awkast(awkast_t *ast, astnode_type_t id) {
  awkmem_ptr_t p = (awkmem_ptr_t)-1;
  astnode_t *n;
  if( ast ) {
    p = sbrk_awkmem(&ast->rom, sizeof(astnode_t));
    if( p != (awkmem_ptr_t)-1 ) {
      n = (astnode_t *)(ast->rom.start + p);
      n->id = id;
    }
  }
  return p;
}

