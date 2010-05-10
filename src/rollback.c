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
#include "myerror.h"
#include "mem.h"
#include "io.h"
#include "rollback.h"
#include "cstring.h"
#include "tempfile.h"

#include <sys/types.h>
#include <sys/stat.h>
#include <sys/fcntl.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <limits.h>
#include <stdio.h>

extern char *progname;

static rollbackmgr_t RBM = { 0 };
static const char_t *TEMPLATE = ".XXXXXX";

void init_rollback_handling() {
  if( !create_mgr_with_rollback(&RBM) ) {
    errormsg(E_FATAL, "failed to initialize file rollbacks.\n");
  }
}

void exit_rollback_handling() {
  free_mgr_with_rollback(&RBM);
}

bool_t create_mgr_with_rollback(rollbackmgr_t *rbm) {
  bool_t ok;
  unsigned int f;
  if( rbm ) {
    rbm->files = NULL;
    rbm->maxfiles = 0;
    ok = create_mem(&rbm->files, &rbm->maxfiles, sizeof(rollback_t), 8);
    if( ok ) {
      memset(rbm->files, 0, sizeof(rollback_t) * rbm->maxfiles);
      for(f = 0; f < rbm->maxfiles; f++) {
	rbm->files[f].fd = -1;
      }
    }
    return ok;
  }
  return FALSE;
}

bool_t free_mgr_with_rollback(rollbackmgr_t *rbm) {
  if( rbm ) {
    clear_mgr_with_rollback(rbm);
    free_mem(&rbm->files, &rbm->maxfiles);
    rbm->files = NULL;
    rbm->maxfiles = 0;
    return TRUE;
  }
  return FALSE;
}

bool_t clear_mgr_with_rollback(rollbackmgr_t *rbm) {
  unsigned int f;
  if( rbm && rbm->files ) {
    for(f = 0; f < rbm->maxfiles; f++) {
      close_file_with_rollback(rbm->files[f].fd);
    }
    return TRUE;
  }
  return FALSE;
}

/* fd = -1 searches for a free rollback_t, fd >= 0 searches for
   an existing rollback_t */
rollback_t *find_mgr_with_rollback(rollbackmgr_t *rbm, int fd) {
  unsigned int f, g;
  if( rbm ) {
    for(f = 0; f < rbm->maxfiles; f++) {
      if( rbm->files[f].fd == fd ) {
	return &(rbm->files[f]);
      }
    }
    /* nothing available */
    if( fd == -1 ) {
      f = rbm->maxfiles;
      if( grow_mem(&rbm->files, &rbm->maxfiles, sizeof(rollback_t), 8) ) {
	/* initialize */
	memset(rbm->files + f, 0, sizeof(rollback_t) * (rbm->maxfiles - f));
	for(g = f; g < rbm->maxfiles; g++) {
	  rbm->files[g].fd = -1;
	}
	return &(rbm->files[f]);
      }
    }
  }
  return FALSE;
}

int open_file_with_rollback(const char_t *path) {
  rollback_t *rb;
  int n;
  rb = find_mgr_with_rollback(&RBM, -1);
  if( rb ) {
    n = strlen(path) + strlen(progname) + sizeof(TEMPLATE) + 2;
    if( n >  (PATH_MAX + 1) ) {
      return -1;
    }
    create_cstring(&rb->path, path, n);
    vstrcat_cstring(&rb->path, ".", progname, TEMPLATE, NULLPTR);
    rb->fd = open_tempfile(p_cstring(&rb->path));
    if( rb->fd != -1 ) {
      return rb->fd;
    }
    /* failed */
    free_cstring(&rb->path);
  }
  return -1;
}

void commit_file_with_rollback(int fd) {
  rollback_t *rb;
  rb = find_mgr_with_rollback(&RBM, fd);
  if( rb ) {
    setflag(&rb->flags, RBM_FLAG_COMMIT);
  }
}

void close_file_with_rollback(int fd) {
  rollback_t *rb;
  cstring_t oldpath;
  const char_t *p, *q;
  int n = 0;
  if( fd > -1 ) {
    rb = find_mgr_with_rollback(&RBM, fd);
    if( rb ) {
      close(rb->fd);
      q = p_cstring(&rb->path);
      if( checkflag(rb->flags, RBM_FLAG_COMMIT) ) {
	p = strdup_cstring(&oldpath, q);
	if( p ) {
	  n = strlen_cstring(&oldpath) - strlen(TEMPLATE) - strlen(progname) - 1;
	  truncate_cstring(&oldpath, n);
	  n = rename(q, p);
	  if( n != 0 ) {
	    errormsg(E_WARNING, "failed to update %s\n", p);
	  } 
	  free_cstring(&oldpath);
	}
      }
      if( n != 0 ) { 
	unlink(q);
      }
      free_cstring(&rb->path);
      rb->fd = -1;
      rb->flags = 0;
    }
  }
}

