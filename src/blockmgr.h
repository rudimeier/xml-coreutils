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

#ifndef BLOCKMGR_H
#define BLOCKMGR_H

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

typedef struct block {
  int blockid;
  size_t bytecount;
  size_t touch;
  struct block *left;
  struct block *right;
} block_t;

typedef struct {
  block_t *root;
  size_t count;
  size_t blocksize;
  size_t numblocks;
  size_t maxblocks;
  block_t *blocks;
  byte_t *data;
  size_t numdata;
} blockmanager_t;

bool_t create_blockmanager(blockmanager_t *bm, size_t blocksize, size_t maxblocks);
bool_t reset_blockmanager(blockmanager_t *bm);
bool_t free_blockmanager(blockmanager_t *bm);
bool_t create_block_blockmanager(blockmanager_t *bm, block_t **result);
bool_t insert_block_blockmanager(blockmanager_t *bm, const block_t *i);
bool_t remove_block_blockmanager(blockmanager_t *bm, const block_t *p);
bool_t unlink_subtree_blockmanager(blockmanager_t *bm, const block_t *p);
bool_t find_block_blockmanager(blockmanager_t *bm, int blockid, block_t **result);
bool_t get_buffer_blockmanager(blockmanager_t *bm, block_t *b, byte_t **buf, size_t *len);

#endif
