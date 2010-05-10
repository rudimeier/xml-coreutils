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

#ifndef IO_H
#define IO_H

#include "common.h"

typedef struct {
  int fd;
  long bytesread;
  byte_t *buf;
  byte_t *pos;
  size_t buflen;
  size_t blksize;
} stream_t;

#define STDIN_FILENO  0
#define STDOUT_FILENO 1
#define STDERR_FILENO 2

#ifndef _O_BINARY
#define _O_BINARY 0
#endif
#ifndef O_BINARY
#define O_BINARY _O_BINARY
#endif

void init_file_handling();
void exit_file_handling();

bool_t open_file_stream(stream_t *strm, const char *path);
bool_t close_stream(stream_t *strm);
bool_t shift_stream(stream_t *strm, size_t n);
bool_t seekbuf_stream(stream_t *strm, long numbytes);
bool_t read_stream(stream_t *strm, byte_t *buf, size_t buflen);

bool_t write_file(int fd, const byte_t *buf, size_t buflen);

bool_t exec_cmdline(const char *filename, const char **argv);
bool_t reaper(pid_t pid);

#endif
