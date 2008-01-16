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

#ifndef CURSORMGR_H
#define CURSORMGR_H

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "cursor.h"
#include "fbparser.h"
#include "skip.h"

typedef struct {
  /* temp cursors */
  cursor_t c0;
  cursor_t c1;
} cursormanager_t;

bool_t create_cursormanager(cursormanager_t *cmg);
bool_t reset_cursormanager(cursormanager_t *cmg);
bool_t free_cursormanager(cursormanager_t *cmg);

bool_t next_sibling_cursormanager(cursormanager_t *cmg, cursor_t *cursor, fbparser_t *fbp);
bool_t prev_sibling_cursormanager(cursormanager_t *cmg, cursor_t *cursor, fbparser_t *fbp);
bool_t next_cursormanager(cursormanager_t *cmg, cursor_t *cursor, fbparser_t *fbp);
bool_t prev_cursormanager(cursormanager_t *cmg, cursor_t *cursor, fbparser_t *fbp);
bool_t parent_cursormanager(cursormanager_t *cmg, cursor_t *cursor, fbparser_t *fbp);
bool_t next_pivot_cursormanager(cursormanager_t *cmg, cursor_t *cursor, fbparser_t *fbp, int pivot);
bool_t prev_pivot_cursormanager(cursormanager_t *cmg, cursor_t *cursor, fbparser_t *fbp, int pivot);

#endif
 
