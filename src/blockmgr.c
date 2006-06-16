/* 
 * Copyright (C) 2006 Laird Breyer
 *  
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
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
#include "blockmgr.h"
#include <string.h>


bool_t create_blockmanager(blockmanager_t *bm, size_t blocksize, size_t maxblocks) {
  if( bm ) {
    bm->blocksize = blocksize;
    bm->maxblocks = maxblocks;
    if( create_mem((byte_t **)&bm->blocks, &bm->numblocks, sizeof(block_t), 8) &&
	create_mem(&bm->data, &bm->numdata, bm->blocksize, 8) ) {

      bm->root = NULL;
      bm->count = 0;
      return TRUE;
    }
  }
  return FALSE;
}

bool_t reset_blockmanager(blockmanager_t *bm) {
  if( bm ) {
    memset(bm->blocks, 0, sizeof(block_t) * bm->numblocks);
    bm->count = 0;
    return TRUE;
  }
  return FALSE;
}

bool_t free_blockmanager(blockmanager_t *bm) {
  if( bm ) {
    free_mem(&bm->data, &bm->numdata);
    free_mem((byte_t **)&bm->blocks, &bm->numblocks);
    return TRUE;
  }
  return FALSE;
}

bool_t create_block_blockmanager(blockmanager_t *bm, block_t **result) {
  size_t i;
  block_t *p;
  if( bm && result) {
    /* first try to grow */
    if( (bm->count >= bm->numblocks) && (bm->numblocks < bm->maxblocks) ) {
      grow_mem(&bm->data, &bm->numdata, bm->blocksize, 8);
      grow_mem((byte_t **)&bm->blocks, &bm->numblocks, sizeof(block_t), 8);
    }
    /* now replace least useful block */
    if( bm->count >= bm->numblocks ) {
      p = &bm->blocks[0];
      for(i = 1; i < bm->count; i++) {
	if( p->touch >= bm->blocks[i].touch ) {
	  p = &bm->blocks[i];
	}
      }
      memset(p, 0, sizeof(block_t));
      *result = p;
      return TRUE;
    } 
    /* create a new block */
    p = &bm->blocks[bm->count];
    memset(p, 0, sizeof(block_t));
    bm->count++;
    *result = p;
    return TRUE;
  }
  return FALSE;
}

bool_t insert_block_blockmanager(blockmanager_t *bm, block_t *i) {
  block_t *p;
  if( bm && i ) {
    p = bm->root;
    if( !p ) {
      bm->root = i;
    } else {
      while( p ) {
	if( p->blockid == i->blockid ) {
	  /* this should never happen */
	  p->touch++;
	  return FALSE;
	}
	if( p->blockid > i->blockid ) {
	  if( p->left ) {
	    p = p->left;
	  } else {
	    p->left = i;
	    break;
	  }
	} else {
	  if( p->right ) {
	    p = p->right; 
	  } else {
	    p->right = i;
	    break;
	  }
	}
      }
    }
    return TRUE;
  }
  return FALSE;
}

bool_t find_block_blockmanager(blockmanager_t *bm, int blockid, block_t **result) {
  block_t *p;
  if( bm && result ) {
    p = bm->root;
    while( p ) {
      if( p->blockid == blockid ) {
	p->touch++;
	*result = p;
	return TRUE;
      }
      p = (p->blockid > blockid) ? p->left : p->right;
    }
  }
  return FALSE;
}

bool_t get_buffer_blockmanager(blockmanager_t *bm, block_t *b, byte_t **buf, size_t *len) {
  if( bm && b && buf && len) {
    *buf = bm->data + bm->blocksize * (b - bm->blocks);
    *len = bm->blocksize;
    return TRUE;
  }
  return FALSE;
}
