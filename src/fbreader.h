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

#ifndef FBREADER_H
#define FBREADER_H

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "blockmgr.h"

typedef struct {
  int fd;
  off_t size;
  size_t blksize;
  blockmanager_t bm;
} fbreader_t;

bool_t open_fileblockreader(fbreader_t *fbr, const char *path, size_t maxblocks);
bool_t close_fileblockreader(fbreader_t *fbr);
bool_t read_fileblockreader(fbreader_t *fbr, off_t offset, byte_t **begin, byte_t **end);
bool_t touch_fileblockreader(fbreader_t *fbr, off_t offset);
#endif
 
