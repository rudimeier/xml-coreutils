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
#include "leafnode.h"
#include <string.h>

bool_t create_leafnode(leafnode_t *lf) {
  if( lf ) {
    return ( create_xpath(&lf->path) && 
	     create_tempvar(&lf->stringval, "leaf-node",
			    MINVARSIZE, MAXVARSIZE) );
  }
  return FALSE;
}

bool_t free_leafnode(leafnode_t *lf) {
  if( lf ) {
    free_xpath(&lf->path);
    free_tempvar(&lf->stringval);
    return TRUE;
  }
  return FALSE;
}
