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

#include "cstring.h"
#include "mem.h"
#include "myerror.h"
#include "entities.h"

#include <string.h>
#include <stdarg.h>

/* charbuf */

char_t *create_charbuf(charbuf_t *cb, size_t size) {
  if( cb ) {
    cb->buf = NULL;
    if( create_mem(&cb->buf, &cb->size, sizeof(char_t), size) ) {
      return cb->buf;
    }
  }
  return NULL;
}

char_t *memwrap_charbuf(charbuf_t *cb, char_t *buf, size_t buflen) {
  return cb ? (cb->buf = buf, cb->size = buflen, cb->buf) : NULL;
}

bool_t free_charbuf(charbuf_t *cb) {
  if( cb ) {
    if( cb->buf ) { free(cb->buf); }
    cb->buf = NULL;
    cb->size = 0;
    return TRUE;
  }
  return FALSE;
}

bool_t ensure_size_charbuf(charbuf_t *cb, int totalsize) {
  return (cb && (0 <= totalsize) && (totalsize < MAXSTRINGSIZE)) ?
    ensure_bytes_mem(totalsize * sizeof(char_t), &cb->buf, 
		     &cb->size, sizeof(char_t)) : 
    (errormsg(E_WARNING, "maximum cstring_t size reached! (req. %d)\n", totalsize), FALSE);
}

const char_t *begin_charbuf(const charbuf_t *cb) {
  return cb ? cb->buf : NULL;
}

const char_t *end_charbuf(const charbuf_t *cb) {
  return cb ? (cb->buf + cb->size) : NULL;
}

/* write to the buffer at pointer p, but only as much as will fit. Return
 * position after write, or NULL if p is not in buffer range.
 */
char_t *write_charbuf(charbuf_t *cb, char_t *p, const char_t *buf, size_t buflen) {
  if( cb && (cb->buf <= p) && (p <= cb->buf + cb->size) ) {
    buflen = MIN(buflen, cb->size - (p - cb->buf));
    memcpy(p, buf, buflen);
    return p + buflen;
  }
  return NULL;
}


/* cstring */

/* reserve n bytes and prefill with string s up to possibly the limit. */
const char_t *create_cstring(cstring_t *cs, const char_t *s, size_t n) {
  if( cs && create_charbuf(&cs->cb, n + 1) ) {
    memccpy(cs->cb.buf, s, '\0', n);
    cs->cb.buf[n] = '\0';
    return cs->cb.buf;
  }
  return NULL;
}

const char_t *wrap_cstring(cstring_t *cs, char_t *s) {
  return (cs && s) ? (cs->cb.buf = s, cs->cb.size = strlen(s) + 1, cs->cb.buf) : NULL;
}

const char_t *strdup_cstring(cstring_t *cs, const char_t *s) {
  if( cs ) {
    cs->cb.buf = strdup(s);
    cs->cb.size = cs->cb.buf ? (strlen(s) + 1) : 0;
    return cs->cb.buf;
  }
  return NULL;
}

bool_t free_cstring(cstring_t *cs) {
  return free_charbuf(&cs->cb);
}

/* bool_t ensure_cstring(cstring_t *cs, size_t totalsize) { */
/*   return cs && ensure_size_charbuf(&cs->cb, totalsize); */
/* } */

/* bool_t require_cstring(cstring_t *cs, size_t extra) { */
/*   return ensure_cstring(cs, extra + cs->cb.size); */
/* } */

/* direct pointer access */

/* char_t *p_cstring(const cstring_t *cs) { */
/* } */

/* bool_t checkrange_cstring(cstring_t *cs, const char_t *p) { */
/*   return cs && (cs->cb.buf <= p) && (p <= cs->cb.buf + cs->cb.size); */
/* } */

/* const char_t *begin_cstring(const cstring_t *cs) { */
/*   return (cs && cs->cb.buf) ? cs->cb.buf : NULL; */
/* } */

/* const char_t *end_cstring(const cstring_t *cs) { */
/*   return (cs && cs->cb.buf) ? memchr(cs->cb.buf, '\0', cs->cb.size) : NULL; */
/* } */

/* returns 0 if string is not null terminated */
size_t strlen_cstring(const cstring_t *cs) {
  const char_t *p;
  p = end_cstring(cs);
  return p ? (p - cs->cb.buf) : 0;
}

size_t buflen_cstring(const cstring_t *cs) {
  return cs ? cs->cb.size : 0;
}

const char_t *truncate_cstring(cstring_t *cs, size_t n) {
  if( cs && (0 <= n) && (n < cs->cb.size) ) {
    cs->cb.buf[n] = '\0';
    return cs->cb.buf + n;
  }
  return NULL;
}

/* write to the string, appending a '\0' and returning the last location
 * written. If the string could not be made big enough for the write,
 * nothing is written and NULL is returned.
 */
const char_t *write_cstring(cstring_t *cs, const char_t *p, const char_t *buf, size_t buflen) {
  size_t offset;
  if( buf && check_range_cstring(cs,p) ) {
    offset = p - begin_cstring(cs);
    if( ensure_cstring(cs, offset + buflen + 1) ) {
      p = begin_cstring(cs) + offset; /* begin could have changed */
      memcpy((void *)p, buf, buflen);
      p += buflen;
      (*(char *)p) = '\0';
      return p;
    }
  }
  return NULL;
}

/* like write, but inserts (pushing the remainder of the string forward) */
const char_t *insert_cstring(cstring_t *cs, const char_t *p, const char_t *buf, size_t buflen) {
  size_t offset, bump;
  if( buf && check_range_cstring(cs,p) ) {
    offset = p - begin_cstring(cs);
    bump = end_cstring(cs) - p + 1; /* +1 in case of trailing '\0' */
    if( ensure_cstring(cs, offset + buflen + bump) ) {
      p = begin_cstring(cs) + offset; /* begin could have changed */
      memmove((void *)(p + buflen), p, bump); 
      memcpy((void *)p, buf, buflen);
      p += buflen;
      return p;
    }
  }
  return NULL;
}

/* remove a portion starting at p */
const char_t *delete_cstring(cstring_t *cs, const char_t *p, size_t len) {
  size_t max;
  if( (len > 0) && check_range_cstring(cs,p) ) {
    max = end_cstring(cs) - p;
    len = MIN(len, max);
    memmove((void *)p, p + len, len);
    *(char_t *)(p + len) = '\0';
  }
  return p;
}

/* write a string (incl. '\0') at a specific pointer p, overwriting
 * what's there and extending the size of the string if necessary.
 * If the size cannot be extended, the overflow is thrown away but
 * the '\0' is always written.
 * On success, returns a pointer to the last character written, which
 * is always the terminating '\0'. Note that because the string may
 * be resized, the return pointer need not be near the original value
 * of p.
 * Failure occurs 
 * 1) when the input pointer p does not point inside the
 * current string buffer. In that case, NULL is returned.
 * 2) when the string could not be extended for the full length required.
 * In that case NULL is returned also, and the buffer is truncated to
 * the original offset of p.
 *
 * This routine is designed to be used safely even when p is NULL, so 
 * you don't need to check p in a pipeline. In particular, partial data
 * is not left in the string, but if you write in the middle of the string,
 * then a truncation could occur.
 */
const char_t *puts_cstring(cstring_t *cs, const char_t *p, const char_t *s) {

  size_t n, rollback;
  if( s && check_range_cstring(cs,p) ) {
    rollback = p - cs->cb.buf;
    n = cs->cb.size - rollback;
    if( n > 0 ) {
      p = memccpy((void *)p, s, '\0', n);
      if( p ) {
	return --p;
      }
      cs->cb.buf[cs->cb.size - 1] = '\0';
      s += (n - 1);
    }
    n = strlen(s) + 1;
    if( ensure_cstring(cs, cs->cb.size + n) ) {
      p = end_cstring(cs);
      p = memccpy((void *)p, s, '\0', n);
      return --p;
    }
    truncate_cstring(cs, rollback);
  }
  return NULL;
}

const char_t *vputs2_cstring(cstring_t *cs, const char_t *p, va_list ap) {
  const char_t *s;
  s = va_arg(ap, const char_t *);
  while( s ) {
    p = *s ? puts_cstring(cs, p, s) : p;
    s = va_arg(ap, const char_t *);
  }
  return p;
}

/* puts a vector of const strings, last argument should be NULLPTR */
const char_t *vputs_cstring(cstring_t *cs, const char_t *p, ...) {
  va_list ap;
  va_start(ap, p);
  p = vputs2_cstring(cs, p, ap);
  va_end(ap);
  return p;
}

const char_t *strcpy_cstring(cstring_t *cs, const char_t *s) {
  return puts_cstring(cs, begin_cstring(cs), s);
}

const char_t *strcat_cstring(cstring_t *cs, const char_t *s) {
  return puts_cstring(cs, end_cstring(cs), s);
}

/* append a list of strings in order. Last must be NULLPTR */ 
const char_t *vstrcat_cstring(cstring_t *cs, ...) {
  const char_t *p;
  va_list ap;
  va_start(ap, cs);
  p = vputs2_cstring(cs, end_cstring(cs), ap);
  va_end(ap);
  return p;
}

/* write to the string, appending a '\0' and returning the last location
 * written. Every instance of a char in WHICH prepended by ESC.
 * If the string could not be made big enough for the write,
 * nothing is written and NULL is returned.
 */
const char_t *write_escape_cstring(cstring_t *cs, const char_t *p, const char_t *which, char_t esc, const char_t *buf, size_t buflen) {
  size_t offset;
  const char_t *q;
  char_t *r;
  if( buf && check_range_cstring(cs,p) ) {
    offset = p - begin_cstring(cs);
    if( ensure_cstring(cs, offset + buflen + 1) ) {
      r = p_cstring(cs) + offset; /* begin could have changed */

      while( buflen-- > 0 ) {
	for(q = which; *q && (*buf != *q); q++);
	if( *q ) { *r++ = esc; }
	*r++ = *buf++;
      }

      *r = '\0';
      return r;
    }
  }
  return NULL;
}

/* write to string cs, replacing xml specials with entities */
const char_t *write_coded_entities_cstring(cstring_t *cs, const char_t *p, const char_t *buf, size_t buflen) {
  const char_t *end = buf + buflen;
  const char_t *q;
  if( buf && (buf < end) && check_range_cstring(cs,p) ) {
    do {
      q = find_next_special(buf, end);
      if( q && (q < end) ) {
	p = write_cstring(cs, p, buf, q - buf);
	p = puts_cstring(cs, p, get_entity(*q));
	buf = q;
	buf++;
      } else {
	p = write_cstring(cs, p, buf, end - buf);
	buf = end;
      }
    } while( buf < end ); 
    return p;
  }
  return NULL;
  /* const char_t *p; */
  /* const char_t *end = buf + buflen; */

  /* if( buf && end && (buf < end) ) { */
  /*   do { */
  /*     p = find_next_special(buf, end); */
  /*     if( p && (p < end) ) { */
  /* 	write_stdout((byte_t *)buf, p - buf); */
  /* 	puts_stdout(get_entity(*p)); */
  /* 	buf = p; */
  /* 	buf++; */
  /*     } else { */
  /* 	write_stdout((byte_t *)buf, end - buf); */
  /* 	buf = end; */
  /*     } */
  /*   } while( buf < end );  */
  /*   return TRUE; */
  /* } */
  /* return FALSE; */
}

/* The null terminated string src is shrunk/copied, by replacing
   every sequence EC with C, where E=esc and C is a non-null character
   found in the string which. This works ok even if dest == src.
 */
char_t *filter_escaped_chars(char_t *dest, const char_t *src, const char_t *which, char_t esc) {
  char_t *q;
  const char_t *r;
  if( dest && src && which ) {
    q = dest;
    while( *src ) {
      if( *src == esc ) {
	src++;
	for(r = which; *r && (*r != *src); r++);
	if( *r == '\0' ) { *q++ = esc; }
      }
      *q++ = *src++;
    }
    *q = '\0';
  }
  return dest;
}

