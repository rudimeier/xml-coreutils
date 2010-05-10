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
#include "stdout.h"
#include "myerror.h"
#include "entities.h"
#include "parser.h"
#include "mem.h"

#include <sys/types.h>
#include <sys/stat.h>
#include <sys/fcntl.h>
#include <unistd.h>
#include <string.h>

#include <stdio.h>
#include <stdarg.h>

typedef struct {
  flag_t flags;
  byte_t *buf;
  size_t buflen;
  size_t pos;
  off_t byteswritten;
  parser_t parser;
} stdout_t;

static stdout_t xstdout = { 0 };
static int stdout_fileno = -1;

extern const char_t escc;
extern char *progname;

/* stdout does not open/close files, but can write to them */
bool_t open_redirect_stdout(int fno) {
  stdout_fileno = fno;
  return open_stdout();
}

bool_t open_stdout() {
  struct stat statbuf;
  if( !xstdout.buf ) {
    if( stdout_fileno == -1 ) {
      stdout_fileno = fileno(stdout);
    }
#if HAVE_SETMODE_DOS
    setmode(stdout_fileno, O_BINARY);
#endif
    if( fstat(stdout_fileno, &statbuf) == 0 ) {
      xstdout.buflen = statbuf.st_blksize;
      xstdout.buf = malloc(xstdout.buflen * sizeof(byte_t));
      xstdout.pos = 0;
      xstdout.byteswritten = 0;
    }
    return (xstdout.buf != NULL);
  }
  return FALSE;
}


bool_t setup_stdout(flag_t flags) {
  callback_t cb = {0};
  if( checkflag(flags, STDOUT_DEBUG) ) {
    setflag(&xstdout.flags, STDOUT_DEBUG);
    setflag(&flags, STDOUT_CHECKPARSER);
  }

  if( checkflag(flags, STDOUT_CHECKPARSER) ) {
    if( create_parser(&xstdout.parser, NULL) ) {
      setflag(&xstdout.flags, STDOUT_CHECKPARSER);
      setup_parser(&xstdout.parser, &cb);
    }
  }

  return TRUE;
}

bool_t flush_stdout() {
  if( xstdout.buf && (stdout_fileno != -1) && (xstdout.pos > 0) ) {
    if( checkflag(xstdout.flags, STDOUT_CHECKPARSER) ) {
      if( !do_parser2(&xstdout.parser, xstdout.buf, xstdout.pos) ) {
	/* even though the output is bad, we still write the good part,
	   not because we approve, but to help the user understand the error
	   of his ways. */
	write_file(stdout_fileno, xstdout.buf, 
		   xstdout.parser.cur.byteno - xstdout.byteswritten);
	if( checkflag(xstdout.flags, STDOUT_DEBUG) ) {
	  errormsg(E_WARNING, "\n");
	  errormsg(E_WARNING, "\n");
	  errormsg(E_WARNING, 
		   "Congratulations! You have found a bug in %s.\n",
		   progname);
	  errormsg(E_WARNING, 
		   "Please keep the input file and notify the author listed in the manpage.\n");
	  errormsg(E_WARNING, "\n");
	  errormsg(E_WARNING, "\n");
	}
	errormsg(E_FATAL, 
		 "invalid XML at line %d, column %d (byte %ld): %s.\n",
		 xstdout.parser.cur.lineno, xstdout.parser.cur.colno, 
		 xstdout.parser.cur.byteno, 
		 error_message_parser(&xstdout.parser));

      }
    }
    write_file(stdout_fileno, xstdout.buf, xstdout.pos);
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
  stdout_fileno = -1;
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
	p = xstdout.buf + xstdout.pos;
      }
    } while( buflen > 0);
    return TRUE;
  }
  return FALSE;
}


bool_t write_coded_entities_stdout(const char_t *buf, size_t buflen) {
  const char_t *p;
  const char_t *end = buf + buflen;

  if( buf && end && (buf < end) ) {
    do {
      p = find_next_special(buf, end);
      if( p && (p < end) ) {
	write_stdout((byte_t *)buf, p - buf);
	puts_stdout(get_entity(*p));
	buf = p;
	buf++;
      } else {
	write_stdout((byte_t *)buf, end - buf);
	buf = end;
      }
    } while( buf < end ); 
    return TRUE;
  }
  return FALSE;
}

bool_t squeeze_coded_entities_stdout(const char_t *buf, size_t buflen) {
  const char_t *p;
  const char_t *end = buf + buflen;

  if( buf && end && (buf < end) ) {
    do {
      p = find_next_special(buf, end);
      if( p && (p < end) ) {
	squeeze_stdout((byte_t *)buf, p - buf);
	puts_stdout(get_entity(*p));
	buf = p;
	buf++;
      } else {
	squeeze_stdout((byte_t *)buf, end - buf);
	buf = end;
      }
    } while( buf < end ); 
    return TRUE;
  }
  return FALSE;
}

bool_t write_unescaped_stdout(const char_t *buf, size_t buflen) {
  const char_t *p;
  const char_t *end = buf + buflen;

  if( buf && end && (buf < end) ) {
    do {
      p = skip_delimiter(buf, end, escc);
      if( p && (p < end) ) {
	write_stdout((byte_t *)buf, p - buf);
	buf = p;
	buf++;
      } else {
	write_stdout((byte_t *)buf, end - buf);
	buf = end;
      }
    } while( buf < end ); 
    return TRUE;
  }
  return FALSE;
}

bool_t puts_stdout(const char_t *s) {
  return write_stdout((byte_t *)s, s ? strlen(s) : 0);
}

bool_t putc_stdout(char_t c) {
  return write_stdout((byte_t *)&c, 1);
}

/* repeat a char, this is used a lot in formatting */
bool_t nputc_stdout(char_t c, int buflen) {
  size_t n;
  if( xstdout.buf && (buflen > 0) ) {
    do {
      n = xstdout.buflen - xstdout.pos;      
      if( buflen < n ) {
	memset(xstdout.buf + xstdout.pos, c, buflen);
	xstdout.pos += buflen;
	buflen = 0;
	break;
      } else {
	memset(xstdout.buf + xstdout.pos, c, n);
	xstdout.pos = xstdout.buflen;
	flush_stdout();
	buflen -= n;
      }
    } while( buflen > 0);
    return TRUE;
  }
  return FALSE;
}

bool_t backspace_stdout() {
  if( xstdout.pos > 0 ) {
    xstdout.pos--;
    return TRUE;
  }
  return FALSE;
}

/* this is suitable for size < buflen only, and causes a flush
   if there isn't enough room in the buffer */
bool_t nprintf_stdout(int size, const char_t *fmt, ...) {
  va_list vap;
  int n = 0;

  /* we write directly into the buffer, so make room for size bytes */
  if( xstdout.buf && fmt ) {
    if( xstdout.pos + size > xstdout.buflen ) {
      flush_stdout();
    }
    if( xstdout.pos + size > xstdout.buflen ) {
      return FALSE;
    }
  }

#if HAVE_VPRINTF

  size = xstdout.buflen - xstdout.pos;
  va_start(vap, fmt);
  n = vsnprintf((char *)(xstdout.buf + xstdout.pos), size, fmt, vap);
  va_end(vap);
  if( n <= size ) {
    xstdout.pos += n;
    return TRUE;
  }

#endif

  return FALSE;
}

bool_t write_start_tag_stdout(const char_t *name, const char_t **att, 
			      bool_t slash) {
  putc_stdout('<');
  puts_stdout(name);

  while( att && *att ) {
    putc_stdout(' ');
    puts_stdout(att[0]);
    puts_stdout("=\"");
    write_coded_entities_stdout(att[1], strlen(att[1]));
    putc_stdout('\"');
    att += 2;
  }

  if( slash ) {
    putc_stdout('/');
  }
  putc_stdout('>');
  return TRUE;
}

bool_t write_end_tag_stdout(const char_t *name) {
  puts_stdout("</");
  puts_stdout(name);
  putc_stdout('>');
  return TRUE;
}

