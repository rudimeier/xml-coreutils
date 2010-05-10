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
#include "blockmgr.h"
#include <string.h>


bool_t create_blockmanager(blockmanager_t *bm, size_t blocksize, size_t maxblocks) {
  bool_t ok = TRUE;
  if( bm ) {
    bm->blocksize = blocksize;
    bm->maxblocks = maxblocks;
    ok &= create_mem(&bm->blocks, &bm->numblocks, sizeof(block_t), 8);
    ok &= create_mem(&bm->data, &bm->numdata, bm->blocksize, 8);
    bm->root = NULL;
    bm->count = 0;
    return ok;
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
    free_mem(&bm->blocks, &bm->numblocks);
    return TRUE;
  }
  return FALSE;
}

bool_t create_block_blockmanager(blockmanager_t *bm, block_t **result) {
  size_t i, d;
  block_t *p;
  if( bm && result) {
    /* first try to grow */
    if( (bm->count >= bm->numblocks) && (bm->numblocks < bm->maxblocks) ) {
      if( grow_mem(&bm->data, &bm->numdata, bm->blocksize, 8) ) {
	if( !grow_mem(&bm->blocks, &bm->numblocks, sizeof(block_t), 8) ) {
	  /* serious trouble */
	  free_blockmanager(bm);
	  create_blockmanager(bm, bm->blocksize, bm->maxblocks);
	  return FALSE;
	}
	/* we've grown, now pointers are wrong. Recreate tree. */
	bm->root = NULL;
	for(i = 0 ; i < bm->count; i++) {
	  bm->blocks[i].left = NULL;
	  bm->blocks[i].right = NULL;
	  insert_block_blockmanager(bm, &bm->blocks[i]);
	}
      }
    }
    /* if we couldn't grow, we replace least useful block.
       least useful = smallest touch count. 
       never replace blocks[0] which contains blockid=0. */
    if( bm->count >= bm->numblocks ) {
      p = &bm->blocks[1]; 
      for(i = 2; i < bm->count; i++) {
	d = bm->blocks[i].touch - p->touch;
	if( d < 0 ) {
/* 	  p = ((d < 0) || (rand() > RAND_MAX/2)) ? &bm->blocks[i] : p; */
	  p = &bm->blocks[i];
	}
      }
      if( remove_block_blockmanager(bm, p) ) {
/* 	debug("(-> %d) ", p->blockid); */
	memset(p, 0, sizeof(block_t));
	*result = p;
	return TRUE;
      }
      return FALSE;
    } 
    /* if we're here, there's room. create a new block at the end. */
    p = &bm->blocks[bm->count];
    memset(p, 0, sizeof(block_t));
    bm->count++;
    *result = p;
    return TRUE;
  }
  return FALSE;
}

bool_t insert_block_blockmanager(blockmanager_t *bm, const block_t *i) {
  block_t *p;
  if( bm && i ) {
    p = bm->root;
    if( !p ) {
      bm->root = (block_t *)i;
    } else {
      while( p ) {
	if( p->blockid == i->blockid ) {
	  /* this should never happen (user error) */
	  p->touch++;
	  return FALSE;
	}
	if( p->blockid > i->blockid ) {
	  if( p->left ) {
	    p = p->left;
	  } else {
	    p->left = (block_t *)i;
	    break;
	  }
	} else {
	  if( p->right ) {
	    p = p->right; 
	  } else {
	    p->right = (block_t *)i;
	    break;
	  }
	}
      }
    }
    return TRUE;
  } 
  return FALSE;
}

bool_t unlink_subtree_blockmanager(blockmanager_t *bm, const block_t *p) {
  block_t *q;
  /* first remove link from parent, then reinsert left and right children */
  q = bm->root;
  if( q == p ) {
    bm->root = NULL;
    return TRUE;
  } else {
    while( q ) {
      /* q is never equal to p */
      if( q->blockid > p->blockid ) {
	if( q->left == p ) {
	  q->left = NULL;
	  return TRUE;
	} else {
	  q = q->left;
	}
      } else {
	if( q->right == p ) {
	  q->right = NULL;
	  return TRUE;
	} else {
	  q = q->right;
	}
      }
    }
  }
  return FALSE;
}

bool_t remove_block_blockmanager(blockmanager_t *bm, const block_t *p) {
  if( bm && p ) {
    if( unlink_subtree_blockmanager(bm, p) ) {
      return (p->left ? insert_block_blockmanager(bm, p->left) : TRUE) &&
	(p->right ? insert_block_blockmanager(bm, p->right) : TRUE);
    }
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
