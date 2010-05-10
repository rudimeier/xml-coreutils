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

#ifndef CURSOR_H
#define CURSOR_H

#include "common.h"
#include <unistd.h>

/* a cursor at final offset o is the smallest sequence of nodes such
 * that the document after o is well formed. A cursor is represented
 * by the offset for each node, up to o. 
 * There is one offset (off) for each depth. 
 *
 * There is also an ordinal number (ord) for each depth, which counts the number
 * of nodes at the same depth which precede it, ie it's the number of
 * siblings earlier in the file.
 *
 * Finally, there is an ordinal number (nord) for each depth, which counts
 * the total number of nodes up to the parent cursor at the given depth.
 *
 * example:
 * <a><b><c/><d/>hello</b></a>
 * The offset of the string "hello" is 14. The smallest well formed document
 * which contains "hello</b></a>" is "<a><b>hello</b></a>". The cursor for
 * the offset "hello" is ["<a>" "<b>" "hello"], and it is represented as
 * [0/0/0 3/0/1 14/2/4] (off/ord/nord).
 * 
 * Note: there is an implicit requirement that the sequence of offsets is
 * strictly increasing for a valid cursor. Because the parser traverses
 * the cursor by reading the node located at each offset, it must not read the
 * same offset twice, as it might cause a parsing error either immediately
 * or later. Similarly, the nord counts are strictly increasing as well.
 */

typedef struct {
  off_t off;
  int ord;
  int nord;
} coff_t;

typedef struct {
  coff_t *stack;
  size_t stacklen;
  size_t top;
} cursor_t;

#define CURSOR_MINSTACK 4

bool_t create_cursor(cursor_t *cursor);
bool_t reset_cursor(cursor_t *cursor);
bool_t free_cursor(cursor_t *cursor);
bool_t push_cursor(cursor_t *cursor, off_t offset, int ord, int nord);
bool_t pop_cursor(cursor_t *cursor);

size_t get_length_cursor(const cursor_t *cursor);
size_t get_depth_cursor(const cursor_t *cursor);
int get_top_ord_cursor(const cursor_t *cursor);
int get_top_nord_cursor(const cursor_t *cursor);
off_t get_top_offset_cursor(const cursor_t *cursor);
int get_depth_ord_cursor(const cursor_t *cursor, size_t depth);
off_t get_depth_offset_cursor(const cursor_t *cursor, size_t depth);
int get_depth_nord_cursor(const cursor_t *cursor, size_t depth);

bool_t bump_cursor(cursor_t *cursor, size_t depth, off_t offset, int nord);
bool_t parent_cursor(cursor_t *cursor);
int cmp_cursor(const cursor_t *cursor1, const cursor_t *cursor2);
bool_t copy_cursor(cursor_t *dest, const cursor_t *src);


void debug_cursor(const cursor_t *cursor);

#endif
 
