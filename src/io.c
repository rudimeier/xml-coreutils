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
#include "mysignal.h"
#include "io.h"
#include "myerror.h"
#include "entities.h"
#include "wrap.h"
#include "mem.h"
#include "cstring.h"

#include <sys/types.h>
#include <sys/stat.h>
#include <sys/fcntl.h>
#include <sys/wait.h>
#include <string.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <limits.h>
#include <errno.h>


extern volatile flag_t cmd;
extern char *progname;
extern char *inputfile;
extern long inputline;

/* dummy XML file to allow us to "read" from STDOUT */
char_t *stdout_dummy = "<?xml version=\"1.0\"?>\n<root>\n</root>\n";

void init_file_handling() {
  /* strict file creation mask used by tempfiles etc */
  /* umask(0644); */
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

/* opens a memory buffer as stream. DOES NOT OWN MEMORY */
bool_t open_mem_stream(stream_t *strm, byte_t *buf, size_t buflen) {
  if( strm && buf ) {
    strm->fd = -2;
    strm->bytesread = 0;
    strm->buf = buf;
    strm->pos = buf;
    strm->buflen = buflen;
    strm->blksize = 1024; /* irrelevant */
    return TRUE;
  }
  return FALSE;
}


bool_t open_file_stream(stream_t *strm, const char *path) {
  if( strm ) {
    strm->fd = (strcmp(path, "stdin") == 0) ? 
      STDIN_FILENO : open(path, O_RDONLY|O_BINARY);
    if( strcmp(path, "stdout") == 0 ) {
      return open_mem_stream(strm, 
			     (byte_t *)stdout_dummy, strlen(stdout_dummy));
    }
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
    if( strm->fd >= 0 ) { close(strm->fd); }
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

/* very limited stream seek within the currently available buffer */
bool_t seekbuf_stream(stream_t *strm, long numbytes) {
  if( strm && (numbytes <= strm->bytesread) ) {
    if( numbytes + strm->buflen >= strm->bytesread ) {
      strm->pos -= (strm->bytesread - numbytes);
      return TRUE;
    }
  }
  return FALSE;
}

bool_t read_file_stream(stream_t *strm, byte_t *buf, size_t buflen) {
  /* we never free an existing buffer, as we don't own it */
  strm->buflen = 0;
  strm->buf = buf;
  if( strm->buf ) {
    strm->buflen = read(strm->fd, strm->buf, buflen);
    if( strm->buflen == -1 ) {
      switch(errno) {
      case EAGAIN:
      case EINTR:
	/* non-fatal errors */
	strm->buflen = 0;
	strm->pos = strm->buf;
	return TRUE;
      default:
	/* fatal errors */
	errormsg(E_WARNING, "error reading %s\n", inputfile);
	strm->buflen = 0;
	strm->buf = NULL;
	return FALSE;
      }
    }
    strm->pos = strm->buf;
    strm->bytesread += strm->buflen;
    return (strm->buflen > 0);
  }
  return FALSE;
}

bool_t read_mem_stream(stream_t *strm, byte_t *buf, size_t buflen) {
  long numbytes;
  if( buf && strm->buf ) {
    numbytes = MIN(buflen, (strm->buflen - strm->bytesread));
    if( numbytes > 0 ) {
      memcpy(buf, strm->pos, numbytes);
      strm->pos += numbytes;
      strm->bytesread += numbytes;
      return TRUE;
    }
  }
  return FALSE;
}


bool_t read_stream(stream_t *strm, byte_t *buf, size_t buflen) {
  if( strm->fd >= 0 ) {
    return read_file_stream(strm, buf, buflen);
  } else if( strm->fd == -2 ) {
    return read_mem_stream(strm, buf, buflen);
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
	       "couldn't write data to file descriptor %d (%lu bytes)\n", 
	       fd, (unsigned long)(buflen - written));
      return FALSE;
    } 
    written += n;
  }
  return TRUE;
}

/* return TRUE on zero status */
bool_t exec_cmdline(const char *filename, const char **argv) {
  pid_t pid, wpid;
  int status;

  pid = fork();
  if( pid == -1 ) {
    errormsg(E_WARNING, "failed to fork\n"); 
    return FALSE;
  } else if( (pid == 0) && (-1 == execvp(filename, (char **)argv)) ) {
    errormsg(E_WARNING, "failed to exec %s: %s\n", 
	     filename, strerror(errno));
    _exit(EXIT_FAILURE);
  }

  /* IMPORTANT: do not handle SIGCHLD during the wait */
  wpid = waitpid(pid, &status, 0);
  if( wpid == 0 ) {
    return TRUE; 
  }
  if( (wpid == 0) || 
      ((wpid == pid) && (WIFEXITED(status) && (WEXITSTATUS(status) == 0))) ) {
    return TRUE;
  }
  errormsg(E_WARNING, 
	     "during exec %s: %s\n", filename, strerror(errno));
  return FALSE;
}



bool_t reaper(pid_t pid) {
  pid_t p;
  kill(pid, SIGTERM); /* this is intercepted in mysignal.c */
  p = waitpid(pid, NULL, 0);
  return (p == 0); 
}
