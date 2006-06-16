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
#include "stdout.h"
#include "error.h"
#include "entities.h"
#include "parser.h"

#include <sys/types.h>
#include <sys/stat.h>
#include <sys/fcntl.h>
#include <string.h>

typedef struct {
  flag_t flags;
  byte_t *buf;
  size_t buflen;
  size_t pos;
  off_t byteswritten;
  parser_t parser;
} stdout_t;

static stdout_t xstdout = { 0 };

bool_t open_stdout() {
  struct stat statbuf;
  setmode(STDOUT_FILENO, O_BINARY);
  if( fstat(STDOUT_FILENO, &statbuf) == 0 ) {
    xstdout.buflen = statbuf.st_blksize;
    xstdout.buf = malloc(xstdout.buflen * sizeof(byte_t));
    xstdout.pos = 0;
    xstdout.byteswritten = 0;
  }
  return (xstdout.buf != NULL);
}

bool_t setup_stdout(flag_t flags) {
  if( checkflag(flags, STDOUT_CHECKPARSER) ) {
    if( create_parser(&xstdout.parser, NULL) ) {
      setflag(&xstdout.flags, STDOUT_CHECKPARSER);
    }
  }
  return TRUE;
}

bool_t flush_stdout() {
  if( xstdout.buf && (xstdout.pos > 0) ) {
    if( checkflag(xstdout.flags, STDOUT_CHECKPARSER) ) {
      if( !do_parser2(&xstdout.parser, xstdout.buf, xstdout.pos) ) {
	/* even though the output is bad, we still write the good part,
	   not because we approve, but to help the user understand the error
	   of his ways. */
	write_file(STDOUT_FILENO, xstdout.buf, 
		   xstdout.parser.cur.byteno - xstdout.byteswritten);
	errormsg(E_FATAL, 
		 "invalid XML at line %d, column %d (byte %ld), aborting.\n",
		 xstdout.parser.cur.lineno, xstdout.parser.cur.colno, 
		 xstdout.parser.cur.byteno);
      }
    }
    write_file(STDOUT_FILENO, xstdout.buf, xstdout.pos);
    xstdout.byteswritten += xstdout.pos;
    xstdout.pos = 0;
  }
  return (xstdout.pos == 0);
}

bool_t close_stdout() {
  flush_stdout();
  if( checkflag(xstdout.flags, STDOUT_CHECKPARSER) ) {
    free_parser(&xstdout.parser);
    clearflag(&xstdout.flags, STDOUT_CHECKPARSER);
  }
  if( xstdout.buf ) {
    free(xstdout.buf);
    xstdout.buf = NULL;
  }
  return (xstdout.buf == NULL);
}

/* this is a buffered write to stdout, since we can expect many small writes
   if the XML is fragmented. */
bool_t write_stdout(const byte_t *buf, size_t buflen) {
  size_t n;
  if( xstdout.buf && buf && (buflen > 0) ) {
    do {
      n = xstdout.buflen - xstdout.pos;      
      if( buflen < n ) {
	memcpy(xstdout.buf + xstdout.pos, buf, buflen);
	xstdout.pos += buflen;
	buflen = 0;
	break;
      } else {
	memcpy(xstdout.buf + xstdout.pos, buf, n);
	xstdout.pos = xstdout.buflen;
	flush_stdout();
	buf += n;
	buflen -= n;
      }
    } while( buflen > 0);
    return TRUE;
  }
  return FALSE;
}

/* squeezes white space, replacing it with single space char.
   Note: All newlines are lost. */
bool_t squeeze_stdout(const byte_t *buf, size_t buflen) {
  byte_t *p;
  bool_t in_space, sq;
  if( xstdout.buf && (buflen > 0) ) {
    p = xstdout.buf + xstdout.pos;
    in_space = (xstdout.pos > 0) ? xml_whitespace(*buf) : FALSE;
    sq = TRUE;
    do {
      while( (xstdout.pos < xstdout.buflen) && (buflen > 0) ) {
	if( in_space ) {
	  if( !sq ) {
	    *p++ = ' ';
	    xstdout.pos++;
	    sq = TRUE;
	  }
	  buf++;
	} else {
	  *p++ = *buf++;
	  xstdout.pos++;
	}
	buflen--;
	in_space = xml_whitespace(*buf);
	sq = sq && in_space;
      }
      if( xstdout.pos >= xstdout.buflen ) {
	xstdout.pos = xstdout.buflen;
	flush_stdout();
      }
    } while( buflen > 0);
    return TRUE;
  }
  return FALSE;
}

bool_t puts_stdout(const char_t *s) {
  return write_stdout((byte_t *)s, strlen(s));
}

bool_t putc_stdout(char_t c) {
  return write_stdout((byte_t *)&c, 1);
}

/* this should be speeded up since it's used a lot in formatting */
bool_t nputc_stdout(char_t c, size_t n) {
  bool_t ok = TRUE;
  size_t i;
  for(i = 0; i < n; i++) {
    ok = ok && write_stdout((byte_t *)&c, 1);
  }
  return ok;
}

bool_t backspace_stdout() {
  if( xstdout.pos > 0 ) {
    xstdout.pos--;
    return TRUE;
  }
  return FALSE;
}

bool_t write_entity_stdout(char_t c) {
  char_t *p = NULL;
  switch(c) {
  case '<':
    p = "&lt;";
    break;
  case '>':
    p = "&gt;";
    break;
  case '&':
    p = "&amp;";
    break;
  case '\'':
    p = "&apos;";
    break;
  case '"':
    p = "&quot;";
    break;
  default:
    break;
  }
  if( p ) {
    puts_stdout(p);
    return TRUE;
  } 
  putc_stdout(c);
  return FALSE;
}

