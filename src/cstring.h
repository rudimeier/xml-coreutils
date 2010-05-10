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

#ifndef CSTRINGBUF_H
#define CSTRINGBUF_H

#include "common.h"
#include <string.h>

typedef struct {
  char_t *buf; /* must be first, then we can cast to (char *) */
  size_t size;
} charbuf_t;

char_t *create_charbuf(charbuf_t *cb, size_t size);
char_t *memwrap_charbuf(charbuf_t *cb, char_t *buf, size_t buflen);
bool_t free_charbuf(charbuf_t *cb);
bool_t ensure_size_charbuf(charbuf_t *cb, int size);
const char_t *begin_charbuf(const charbuf_t *cb);
const char_t *end_charbuf(const charbuf_t *cb);
char_t *write_charbuf(charbuf_t *cb, char_t *p, const char_t *buf, size_t buflen); 

typedef struct {
  charbuf_t cb; 
} cstring_t;

#define CSTRING(cs,s) cstring_t cs = { { (s), sizeof(s) } }

#define CSTRINGP(cs) ((cs).cb.buf != NULL)

const char_t *create_cstring(cstring_t *cs, const char_t *buf, size_t n);
const char_t *wrap_string_cstring(cstring_t *cs, char_t *s);
const char_t *strdup_cstring(cstring_t *cs, const char_t *s);
bool_t free_cstring(cstring_t *cs);

size_t strlen_cstring(const cstring_t *cs);
size_t buflen_cstring(const cstring_t *cs);

const char_t *truncate_cstring(cstring_t *cs, size_t n);

const char_t *write_cstring(cstring_t *cs, const char_t *p, const char_t *buf, size_t buflen);
const char_t *insert_cstring(cstring_t *cs, const char_t *p, const char_t *buf, size_t buflen);
const char_t *delete_cstring(cstring_t *cs, const char_t *p, size_t len);

const char_t *puts_cstring(cstring_t *cs, const char_t *p, const char_t *s);
const char_t *vputs_cstring(cstring_t *cs, const char_t *p, ...);

const char_t *strcpy_cstring(cstring_t *cs, const char_t *s);
const char_t *strcat_cstring(cstring_t *cs, const char_t *s);
const char_t *vstrcat_cstring(cstring_t *cs, ...);

char_t *filter_escaped_chars(char_t *dest, const char_t *src, const char_t *which, char_t esc);
const char_t *write_escape_cstring(cstring_t *cs, const char_t *p, const char_t *which, char_t esc, const char_t *buf, size_t buflen);
const char_t *write_coded_entities_cstring(cstring_t *cs, const char_t *p, const char_t *buf, size_t buflen);

/* inline const inspectors */

__inline__ static char_t *p_cstring(const cstring_t *cs) {
  return cs ? cs->cb.buf : NULL;
}

__inline__ static const char_t *begin_cstring(const cstring_t *cs) {
  return (cs && cs->cb.buf) ? cs->cb.buf : NULL;
}

__inline__ static const char_t *end_cstring(const cstring_t *cs) {
  char_t *p;
  return (cs && cs->cb.buf) ? ((p = memchr(cs->cb.buf, '\0', cs->cb.size)) ? p : cs->cb.buf) : NULL;
}

__inline__ static bool_t check_range_cstring(const cstring_t *cs, const char_t *p) {
  return cs && (cs->cb.buf <= p) && (p <= cs->cb.buf + cs->cb.size);
}

__inline__ static bool_t ensure_cstring(cstring_t *cs, size_t totalsize) {
  return cs && ensure_size_charbuf(&cs->cb, totalsize);
}

__inline__ static const char_t *putc_cstring(cstring_t *cs, const char_t *p, char_t c) {
  int offset = p - cs->cb.buf;
  if( cs && (0 <= offset) && 
      ((offset < cs->cb.size) ||
       ensure_size_charbuf(&cs->cb, offset + 1)) ) {
    cs->cb.buf[offset++] = c;
    cs->cb.buf[offset] = '\0';
  }
  return (cs->cb.buf + offset); /* return p on failure */
}

#endif
