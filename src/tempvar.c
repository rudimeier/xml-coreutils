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

#include "common.h"
#include "tempvar.h"
#include "io.h"
#include "entities.h"
#include <string.h>

#include <unistd.h>

bool_t create_tempvar(tempvar_t *tv, const char_t *name,
		      size_t buflen, size_t maxbuflen) {
  return create_tempcollect(&tv->tc, name, buflen, maxbuflen);
}

bool_t free_tempvar(tempvar_t *tv) {
  return free_tempcollect(&tv->tc);
}

bool_t reset_tempvar(tempvar_t *tv) {
  return reset_tempcollect(&tv->tc);
}

bool_t checksp_tempvar(tempvar_t *tv, size_t len) {
  return (tv->tc.max_buflen > (tv->tc.bufpos + len));
}

bool_t write_tempvar(tempvar_t *tv, const byte_t *buf, size_t buflen) {
  return ( checksp_tempvar(tv, buflen) ?
	   write_tempcollect(&tv->tc, buf, buflen) :
	   FALSE );
}

bool_t puts_tempvar(tempvar_t *tv, const char_t *s) {
  return write_tempvar(tv, (byte_t *)s, s ? strlen(s) * sizeof(char_t) : 0);
}

bool_t putc_tempvar(tempvar_t *tv, char_t c) {
  return write_tempvar(tv, (byte_t *)&c, 1);
}

bool_t squeeze_tempvar(tempvar_t *tv, 
		       const char_t *buf, size_t buflen) {
  return ( checksp_tempvar(tv, buflen) ?
	   squeeze_tempcollect(&tv->tc, buf, buflen) :
	   FALSE );
}

/* remove trailing whitespace */
bool_t chomp_tempvar(tempvar_t *tv) {
  const char_t *b, *e, *n;
  if( tv ) {
    b = (char_t *)tv->tc.buf;
    e = b + tv->tc.bufpos;
    n = rskip_xml_whitespace(b, e);
    if( !n ) {
      tv->tc.bufpos = 0;
    } else if( n < e ) {
      tv->tc.bufpos = n - b;
    }
    return TRUE;
  }
  return FALSE;
}

/* appends tc with the contents of file. If a read problem occurs,
 * the data is truncated back to the pre-read state.
 */
bool_t read_from_file_tempvar(tempvar_t *tv, const char_t *file) {
  stream_t strm;
  byte_t *buf;
  size_t ipos, bufsize;
  bool_t done = FALSE;
  if( tv && file ) {
    if( open_file_stream(&strm, file) ) {
      ipos = tell_tempcollect(&tv->tc);
      bufsize = strm.blksize;
      while( checksp_tempvar(tv, bufsize) &&
	     reserve_tempcollect(&tv->tc, bufsize) ) {
	buf = tv->tc.buf + tv->tc.bufpos;
	if( read_stream(&strm, buf, bufsize) ) {
	  tv->tc.bufpos += strm.buflen;
	} else {
	  done = TRUE;
	  break;
	}
      }
      close_stream(&strm);
      if( !done ) { truncate_tempcollect(&tv->tc, ipos); }
      return done;
    }
  }
  return FALSE;
}

bool_t write_to_file_tempvar(tempvar_t *tv, const char_t *file, int openflags) {
  return (tv && file) ? write_to_file_tempcollect(&tv->tc, file, openflags) : FALSE;
}


/* peek at the tail end of tempvar. Returns a null terminated string,
 * by inserting a '\0' at bufpos. Fails to return a string if buffer has
 * no room for '\0' or tempfile exists.
 */
bool_t peeks_tempvar(tempvar_t *tv, size_t pos, const char_t **s) {
  byte_t *end;
  if( peek_tempcollect(&tv->tc, pos, tv->tc.buflen, 
		       (const byte_t **)s, (const byte_t **)&end) ) {
    if( end < tv->tc.buf + tv->tc.buflen ) {
      *end = '\0';
      return TRUE;
    }
  }
  return FALSE;
}

/* delete the first pos bytes of tempvar */
bool_t shift_tempvar(tempvar_t *tv, size_t pos) {
  if( tv && (pos > 0) ) {
    if( pos < tv->tc.bufpos ) {
      memmove(tv->tc.buf, tv->tc.buf + pos, tv->tc.bufpos - pos);
      tv->tc.bufpos -= pos;
    } else {
      tv->tc.bufpos = 0;
    }
    return TRUE;
  }
  return FALSE;
}

const char_t *string_tempvar(tempvar_t *tv) {
  const char_t *s;
  return peeks_tempvar(tv, 0, &s) ? s : "";
}

long int long_tempvar(tempvar_t *tv) {
  return atol(string_tempvar(tv));
}

unsigned long int ulong_tempvar(tempvar_t *tv) {
  return strtoul(string_tempvar(tv), NULL, 10);
}

double double_tempvar(tempvar_t *tv) {
  return atof(string_tempvar(tv));
}
