/* 
 * Copyright (C) 2009 Laird Breyer
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

#ifndef ROLLBACK_H
#define ROLLBACK_H

#include "common.h"
#include "cstring.h"

typedef struct {
  int fd;
  cstring_t path;
  flag_t flags;
} rollback_t;

#define RBM_FLAG_COMMIT   0x01

typedef struct {
  rollback_t *files;
  size_t maxfiles;
} rollbackmgr_t;

/* _with_rollback() functions:
 * open a temporary file to write into, and upon closing decide if it
 * should be renamed as path or deleted. The file is automatically
 * deleted in case of a signal. NOT THREAD SAFE.
 */

void init_rollback_handling();
void exit_rollback_handling();

int open_file_with_rollback(const char *path);
void commit_file_with_rollback(int fd);
void close_file_with_rollback(int fd);

/* not for users */
bool_t create_mgr_with_rollback(rollbackmgr_t *rbm);
bool_t free_mgr_with_rollback(rollbackmgr_t *rbm);
bool_t clear_mgr_with_rollback(rollbackmgr_t *rbm);
rollback_t *find_mgr_with_rollback(rollbackmgr_t *rbm, int fd);


#endif
