/* 
 * Copyright (C) 2010 Laird Breyer
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

#include "tempfile.h"
#include "cstring.h"
#include "io.h"
#include "stringlist.h"
#include "mysignal.h" 

#include <unistd.h>
#include <stdlib.h>
#include <limits.h>
#include <time.h>

extern volatile flag_t cmd;

static stringlist_t tempfiles;
static volatile sig_atomic_t busy = 0; /* defined in mysignal.h */

/* this can be called during a signal so we do not tamper with the
 * tempfiles list, and don't do anything if the list is not in a
 * consistent state. 
 */
void cleanup_tempfile() {
  int i;
  if( !busy ) {
    for(i = 0; i < tempfiles.num; i++) {
      if( tempfiles.list[i] ) {
	unlink(tempfiles.list[i]);
      }
    }
  }
}

void init_tempfile_handling() {
  create_stringlist(&tempfiles);
  atexit(cleanup_tempfile);
}

void exit_tempfile_handling() {
  int i;

  busy = 1; /* watch for possible compiler reordering! */
  if( busy ) { 
    for(i = 0; i < tempfiles.num; i++) {
      if( tempfiles.list[i] ) {
	unlink(tempfiles.list[i]);
      }
    }
  }
  if( busy ) {
    /* not freed since cleanup_tempfile() can be called at any time */
    reset_stringlist(&tempfiles); 
  }
  busy = 0;
}

/* make a temporary file name (template for mkstemp) that user must free */
char *make_template_tempfile(const char *basename) {
  const char *t;
  cstring_t cs;
  
  t = getenv("TMPDIR");
  t = t ? t : getenv("TMP");
  t = t ? t : "/tmp";
  create_cstring(&cs, t, PATH_MAX);
  t = vstrcat_cstring(&cs, "/", basename, ".XXXXXX", NULLPTR);
  
  /* if we return a string, we must not free cs */
  return ( t < begin_cstring(&cs) + PATH_MAX ) ?
    p_cstring(&cs) : (free_cstring(&cs), NULL);
}

int open_tempfile(char *template) {
  int fd;
  fd = mkstemp(template);
  if( fd != -1 ) {
    busy = 1; /* possible compiler reordering? */
    if( busy ) {
      add_unique_stringlist(&tempfiles, template, STRINGLIST_STRDUP);
    }
    busy = 0;
  }
  return fd;
}

bool_t remove_tempfile(char *template) {
  int r = FALSE;
  if( template ) {
    busy = 1; /* possible compiler reordering? */
    if( busy ) {
      r = strikeout_stringlist(&tempfiles, template) ? 
	(0 == unlink(template)) : FALSE;
    }
    busy = 0;
  }
  return r;
}

int save_stdin_tempfile(char **tempfile, pid_t *child_pid, const char *progname) {
  int fd;
  pid_t pid;
  stream_t strm;
  byte_t *buf;
  clock_t c;

  if( tempfile && child_pid ) {
    *tempfile = make_template_tempfile(progname);
    fd = open_tempfile(*tempfile);
    pid = fork();
    if( pid == -1 ) {
      /* failed */
      close(fd);
      fd = -1;
    } else if( pid == 0) {
      /* child: writes stdin to fd */
      c = clock();
      if( open_file_stream(&strm, "stdin") ) {
	buf = malloc(strm.blksize);
	/* since read_stream is blocking by default,
	 * we raise an alarm periodically to force
	 * an interruption.
	 * Note: we do not call process_pending_signal()
	 */
#define MINFORCE 1
#define MAXFORCE 1
	alarm(MAXFORCE);
	while( buf && !checkflag(cmd,CMD_QUIT) &&
	       read_stream(&strm, buf, strm.blksize) &&
	       write_file(fd, buf, strm.buflen) ) {
	  if( c - clock() > MINFORCE * CLOCKS_PER_SEC ) { 
	    /* tell the parent we have more data */
	    kill(getppid(), SIGALRM);
	    c = clock();
	  }
	  alarm(MAXFORCE);
	};
	free(buf);
	close(fd);
	close_stream(&strm);
	kill(getppid(), SIGALRM);
	_exit(EXIT_SUCCESS); /* do not call atexit() registrations */
      }
    }
    /* parent: reads fd and should unlink it */
    *child_pid = pid;
    return dup(fd);
  }
  return -1;
}
