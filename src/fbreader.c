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
#include "io.h"
#include "mem.h"
#include "fbreader.h"
#include "error.h"

#include <sys/types.h>
#include <sys/stat.h>
#include <sys/fcntl.h>
#include <string.h>
#include <unistd.h>

bool_t open_fileblockreader(fbreader_t *fbr, const char *path, size_t maxblocks) {
  struct stat statbuf;
  if( fbr ) {
    fbr->fd = open(path, O_RDONLY|O_BINARY);
    if( fbr->fd == -1 ) {
      errormsg(E_ERROR, "cannot open %s\n", path);
      return FALSE;
    }
    if( fstat(fbr->fd, &statbuf) == 0 ) {
      switch(statbuf.st_mode & S_IFMT) {
      case S_IFDIR:
	errormsg(E_ERROR, "cannot open directory %s\n", path);
	return FALSE;
      default:
	fbr->size = statbuf.st_size;
	fbr->blksize = statbuf.st_blksize;
	break;
      }
      return create_blockmanager(&fbr->bm, fbr->blksize, maxblocks);
    }
  }
  return FALSE;
}

bool_t close_fileblockreader(fbreader_t *fbr) {
  if( fbr ) {
    close(fbr->fd);
    fbr->fd = -1;
    free_blockmanager(&fbr->bm);
    return TRUE;
  }
  return FALSE;
}

bool_t read_fileblockreader(fbreader_t *fbr, off_t offset, byte_t **begin, byte_t **end) {
  int blockid;
  off_t where;
  block_t *block;
  byte_t *buf;
  size_t buflen;
  if( fbr && begin && end && (offset >= 0) && (offset < fbr->size) ) {
    blockid = offset / fbr->blksize;
    if( !find_block_blockmanager(&fbr->bm, blockid, &block) ) {
      where = blockid * fbr->blksize;
      if( where != lseek(fbr->fd, where, SEEK_SET) ) {
	return FALSE;
      }
      /* no need to free the block if we fail to use it */
      if( !create_block_blockmanager(&fbr->bm, &block) ) {
	return FALSE;
      }
      if( !get_buffer_blockmanager(&fbr->bm, block, &buf, &buflen) ) {
	return FALSE;
      }
      block->blockid = blockid;
      block->touch = 1;
      block->bytecount = read(fbr->fd, buf, buflen);
      if( block->bytecount == -1 ) {
	return FALSE;
      }
      if( !insert_block_blockmanager(&fbr->bm, block) ) {
	return FALSE;
      }
/*       debug("READING BLOCK %d\n", blockid); */
    } 
    if( get_buffer_blockmanager(&fbr->bm, block, &buf, &buflen) ) {
      *begin = buf + MIN(offset % fbr->blksize, block->bytecount);
      *end = buf + block->bytecount;
      return TRUE;
    }
  } 
  return FALSE;
}

bool_t touch_fileblockreader(fbreader_t *fbr, off_t offset) {
  int blockid;
  block_t *block;
  if( fbr && (offset >= 0) ) {
    blockid = offset / fbr->blksize;
    if( find_block_blockmanager(&fbr->bm, blockid, &block) ) {
      block->touch++;
      return TRUE;
    }
  }
  return FALSE;
}
