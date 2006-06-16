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
#include "io.h"
#include "error.h"
#include "entities.h"

#include <sys/types.h>
#include <sys/stat.h>
#include <sys/fcntl.h>
#include <string.h>
#include <unistd.h>

extern char *inputfile;
extern int cmd;

void init_file_handling() {
}

void exit_file_handling() {
}

bool_t open_stream(stream_t *strm) {
  struct stat statbuf;
  if( strm ) {
    if( fstat(strm->fd, &statbuf) == 0 ) {
      switch(statbuf.st_mode & S_IFMT) {
      case S_IFDIR:
	errormsg(E_ERROR, "cannot open directory %s\n", inputfile);
	return FALSE;
      default:
	strm->blksize = statbuf.st_blksize;
	break;
      }
      strm->bytesread = 0;
      strm->buf = NULL;
      strm->pos = NULL;
      strm->buflen = 0;
      return TRUE;
    }
  }
  return FALSE;
}

bool_t open_file_stream(stream_t *strm, const char *path) {
  if( strm ) {
    strm->fd = (strcmp(path, "stdin") == 0) ? 
      STDIN_FILENO : open(path, O_RDONLY|O_BINARY);
    if( strm->fd == -1 ) {
      errormsg(E_ERROR, "cannot open %s\n", path);
      return FALSE;
    }
    return open_stream(strm);
  }
  return FALSE;
}


bool_t close_stream(stream_t *strm) {
  if( strm ) {
    close(strm->fd);
    strm->fd = -1;
    strm->bytesread = 0;
    strm->buf = NULL;
    strm->pos = NULL;
    strm->buflen = 0;
    return TRUE;
  }
  return FALSE;
}


bool_t shift_stream(stream_t *strm, size_t n) {
  if( (n > 0) && (n < strm->buflen) ) {
    strm->buflen -= n;
    memmove(strm->buf, strm->buf + n, strm->buflen);
    strm->pos = strm->buf;
    return TRUE;
  }
  return FALSE;
}

bool_t read_stream(stream_t *strm, byte_t *buf, size_t buflen) {
  /* we never free an existing buffer, as we don't own it */
  strm->buflen = 0;
  strm->buf = buf;
  if( strm->buf ) {
    strm->buflen = read(strm->fd, strm->buf, buflen);
    if( strm->buflen == -1 ) {
      errormsg(E_WARNING, "error reading %s\n", inputfile);
      strm->buflen = 0;
      strm->buf = NULL;
      return FALSE;
    }
    strm->pos = strm->buf;
    strm->bytesread += strm->buflen;
    return (strm->buflen > 0);
  }
  return FALSE;
}

bool_t write_file(int fd, const byte_t *buf, size_t buflen) {
  ssize_t n, written;
  
  written = 0;
  while( (written < buflen) && !checkflag(cmd,CMD_QUIT) ) {
    n = write(fd, buf + written, buflen - written);
    if( n == -1 ) {
      errormsg(E_ERROR, 
	       "couldn't write data to file descriptor %d (%d bytes)\n", 
	       fd, buflen - written);
      return FALSE;
    } 
    written += n;
  }
  return TRUE;
}

