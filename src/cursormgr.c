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
#include "cursormgr.h"
#include "myerror.h"
#include <string.h>

bool_t create_cursormanager(cursormanager_t *cmg) {
  bool_t ok = TRUE;
  if( cmg ) {
    ok &= create_cursor(&cmg->c0);
    ok &= create_cursor(&cmg->c1);
    return ok;
  }
  return FALSE;
}

bool_t free_cursormanager(cursormanager_t *cmg) {
  if( cmg ) {
    free_cursor(&cmg->c0);
    free_cursor(&cmg->c1);
    return TRUE;
  }
  return FALSE;
}

bool_t reset_cursormanager(cursormanager_t *cmg) {
  if( cmg ) {
    return ( reset_cursor(&cmg->c0) &&
	     reset_cursor(&cmg->c1) );
    return TRUE;
  }
  return FALSE;
}

bool_t parent_cursormanager(cursormanager_t *cmg, cursor_t *cursor, fbparser_t *fbp) {
  if( cmg && cursor && fbp ) {
    return parent_cursor(cursor);
  }
  return FALSE;
}

bool_t next_sibling_cursormanager(cursormanager_t *cmg, cursor_t *cursor, fbparser_t *fbp) {
  skip_t skip;
  skip.count = 1;
  skip.depth = get_depth_cursor(cursor);
  skip.what = eq_depth;
  skip.nodemask = NODEMASK_ALL;
  if( cmg && cursor && fbp ) {
    return forward_skip(&skip, cursor, fbp);
  }
  return FALSE;
}

bool_t prev_sibling_cursormanager(cursormanager_t *cmg, cursor_t *cursor, fbparser_t *fbp) {
  skip_t skip;
  int n;
  if( cmg && cursor && fbp ) {
    n = get_top_ord_cursor(cursor);
    if( n < 2 ) {
/*       return parent_cursor(cursor); */
      return FALSE;
    } else {
      skip.count = n;
      skip.depth = get_depth_cursor(cursor);
      skip.what = eq_depth;
      skip.nodemask = NODEMASK_ALL;
      return parent_cursor(cursor) && forward_skip(&skip, cursor, fbp);
    }
  }
  return FALSE;
}

bool_t next_cursormanager(cursormanager_t *cmg, cursor_t *cursor, fbparser_t *fbp, int count) {
  skip_t skip;
  skip.count = count;
  skip.depth = 0;
  skip.what = not_endtag;
  skip.nodemask = NODEMASK_ALL;
  if( cmg && cursor && fbp ) {
    return forward_skip(&skip, cursor, fbp);
  }
  return FALSE;
}

bool_t prev_cursormanager(cursormanager_t *cmg, cursor_t *cursor, fbparser_t *fbp, int count) {
  skip_t skip;
  skip.count = count;
  skip.depth = 0;
  skip.what = not_endtag;
  skip.nodemask = NODEMASK_ALL;
  if( cmg && cursor && fbp ) {
    return backward_skip(&skip, cursor, fbp);
  }
  return FALSE;
}

bool_t next_pivot_cursormanager(cursormanager_t *cmg, cursor_t *cursor, fbparser_t *fbp, int pivot, int count) {
  skip_t skip;
  skip.count = count;
  skip.depth = pivot;
  skip.what = lt_depth;
  skip.nodemask = NODEMASK_ALL;
  if( cmg && cursor && fbp ) {
    return forward_skip(&skip, cursor, fbp);
  }
  return FALSE;
}

bool_t prev_pivot_cursormanager(cursormanager_t *cmg, cursor_t *cursor, fbparser_t *fbp, int pivot, int count) {
  skip_t skip;
  skip.count = count;
  skip.depth = pivot;
  skip.what = lt_depth;
  skip.nodemask = NODEMASK_ALL;
  if( cmg && cursor && fbp ) {
    return backward_skip(&skip, cursor, fbp);
  }
  return FALSE;


  /* int n; */
  /* bool_t changed = FALSE; */
  /* if( cmg && cursor && fbp ) { */
  /*   n = get_depth_cursor(cursor); */
  /*   if( n >= pivot ) { */
  /*     while( (n > pivot) && parent_cursor(cursor) ) {  */
  /* 	n--;  */
  /* 	changed = TRUE; */
  /*     } */
  /*     return ( changed ||  */
  /* 	       prev_sibling_cursormanager(cmg, cursor, fbp) || */
  /* 	       parent_cursormanager(cmg, cursor, fbp) ); */
  /*   } else { */
  /*     /\* incomplete: this should go to the last cursor with pivot depth *\/ */
  /*     return ( prev_sibling_cursormanager(cmg, cursor, fbp) || */
  /* 	       parent_cursormanager(cmg, cursor, fbp) ); */
  /*   } */
  /* } */
  /* return FALSE; */
}
