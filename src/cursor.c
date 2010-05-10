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
#include "mem.h"
#include "cursor.h"
#include "myerror.h"
#include <string.h>

void debug_cursor(const cursor_t *cursor) {
  size_t i;
  debug("[");
  for(i = 0; i < cursor->top; i++) {
    debug("%lx/%d/%d ", 
	  cursor->stack[i].off, cursor->stack[i].ord, cursor->stack[i].nord);
  }
  debug("]\n"); 
}

bool_t create_cursor(cursor_t *cursor) {
  bool_t ok = TRUE;
  if( cursor ) {
    cursor->top = 0;
    ok &= create_mem(&cursor->stack, &cursor->stacklen, 
		     sizeof(coff_t), CURSOR_MINSTACK);
    if( ok ) { reset_cursor(cursor); }
    return ok;
  }
  return FALSE;
}

bool_t free_cursor(cursor_t *cursor) {
  if( cursor ) {
    if( cursor->stack ) {
      free(cursor->stack);
      cursor->stack = NULL;
      cursor->stacklen = 0;
    }
    return TRUE;
  }
  return FALSE;
}

bool_t reset_cursor(cursor_t *cursor) {
  if( cursor ) {
    cursor->top = 0;
    push_cursor(cursor, 0, 0, 0);
    return TRUE;
  }
  return FALSE;
}

bool_t push_cursor(cursor_t *cursor, off_t offset, int ord, int nord) {
  if( cursor ) {
    if( cursor->top >= cursor->stacklen ) {
      grow_mem(&cursor->stack, &cursor->stacklen, sizeof(coff_t), CURSOR_MINSTACK);
    }
    if( cursor->top < cursor->stacklen ) {
      /* we must not allow duplicates in the stack, or the parser
	 might get confused. This is not a problem with our algorithm,
	 but is a problem with libexpat. When expat encounters a 
	 <a/> tag, it converts it to two tags <a> and </a> which
	 *both* start at the same offset. In reality, only <a> starts
	 at that offset and </a> is fake, but our code doesn't know.
	 So it will try to push the same offset twice. This is not
	 a fatal problem, but it causes four tags (<a></a><a></a>) to
	 be seen when the cursor is traversed, whereas only a single
	 <a/> tag exists! The fix is to prevent pushing duplicates. */
      if( (cursor->top == 0) || 
	  (cursor->stack[cursor->top - 1].off < offset) ) {
	cursor->stack[cursor->top].off = offset;
	cursor->stack[cursor->top].ord = ord;
	cursor->stack[cursor->top].nord = nord;
	cursor->top++;
	return TRUE;
      }
    }
  }
  return FALSE;
}

bool_t pop_cursor(cursor_t *cursor) {
  if( cursor ) {
    if( cursor->top > 0 ) {
      cursor->top--;
      if( cursor->top == 0 ) {
	reset_cursor(cursor);
      }
      return TRUE;
    } 
  }
  return FALSE;
}

size_t get_length_cursor(const cursor_t *cursor) {
  return cursor ? cursor->top : 0;
}

size_t get_depth_cursor(const cursor_t *cursor) {
  return cursor ? cursor->top - 1 : 0;
}

off_t get_depth_offset_cursor(const cursor_t *cursor, size_t depth) {
  return (cursor && (depth < cursor->top)) ? cursor->stack[depth].off : -1;
}

off_t get_top_offset_cursor(const cursor_t *cursor) {
  return (cursor && (cursor->top > 0)) ? cursor->stack[cursor->top - 1].off : -1;
}

int get_depth_ord_cursor(const cursor_t *cursor, size_t depth) {
  return (cursor && (depth < cursor->top)) ? cursor->stack[depth].ord : 0;
}

int get_top_ord_cursor(const cursor_t *cursor) {
  return (cursor && (cursor->top > 0)) ? cursor->stack[cursor->top - 1].ord : 0;
}

int get_depth_nord_cursor(const cursor_t *cursor, size_t depth) {
  return (cursor && (depth < cursor->top)) ? cursor->stack[depth].nord : 0;
}

int get_top_nord_cursor(const cursor_t *cursor) {
  return (cursor && (cursor->top > 0)) ? cursor->stack[cursor->top - 1].nord : 0;
}

bool_t parent_cursor(cursor_t *cursor) {
  return pop_cursor(cursor);
}

bool_t bump_cursor(cursor_t *cursor, size_t depth, off_t offset, int nord) {
/*   debug("bump_cursor %d %d (top=%d)\n", depth, offset, cursor->top); */
  if( cursor && (depth >= 0) ) {
    if( cursor->top == depth ) {
      push_cursor(cursor, offset, 0, nord);
    } else {
      cursor->stack[depth].off = offset;
      cursor->stack[depth].ord++;
      cursor->stack[depth].nord = nord;
      cursor->top = depth + 1;
    }
    return TRUE;
  }
  return FALSE;
}

/* returns positive number if cursor1 > cursor2, zero if equal (or null).
 * Note: the numbers don't mean anything. 
 */
int cmp_cursor(const cursor_t *cursor1, const cursor_t *cursor2) {
  size_t i;
  if( cursor1 && cursor2 ) {
    if( cursor1->top != cursor2->top ) {
      return cursor1->top - cursor2->top;
    } else {
      for(i = cursor1->top - 1; i >= 0; i--) {
	if( cursor1->stack[i].off != cursor2->stack[i].off ) {
	  return cursor1->stack[i].off - cursor2->stack[i].off;
	}
      }
    }
  }
  return 0;
}

bool_t copy_cursor(cursor_t *dest, const cursor_t *src) {
  size_t n;
  if( dest && src && (dest != src) ) {
    if( src->top >= dest->stacklen ) {
      n = CURSOR_MINSTACK;
      while( n < src->top ) { n *= 2; }
      grow_mem(&dest->stack, &dest->stacklen, sizeof(coff_t), n);
    }
    memcpy(dest->stack, src->stack, sizeof(coff_t) * src->top);
    dest->top = src->top;
    return TRUE;
  }
  return FALSE;
}
