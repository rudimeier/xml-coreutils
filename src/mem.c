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
#include "mem.h"
#include <string.h>
#include <stdio.h>

bool_t grow_mem(void *pptr, size_t *nmemb, size_t size, size_t minmemb) {
  byte_t **ptr = (byte_t **)pptr;
  byte_t *p;
  size_t nbytes;
  if( ptr && nmemb ) {
    if( !*ptr ) {
      *nmemb = minmemb;
      *ptr = calloc( *nmemb, size );
      return ( *ptr != NULL);
    } else {
      nbytes = size * 2 * (*nmemb);
      p = realloc(*ptr, nbytes);
      if( !p ) {
	return FALSE;
      }
      (*nmemb) *= 2;
      (*ptr) = p;
      return TRUE;
    }
  }
  return FALSE;
}

bool_t create_mem(void *pptr, size_t *nmemb, size_t size, size_t min) {
  byte_t **ptr = (byte_t **)pptr;
  if( ptr && nmemb ) {
    *ptr = NULL;
    *nmemb = 0;
    return grow_mem(ptr, nmemb, size, min);
  }
  return FALSE;
}

bool_t free_mem(void *pptr, size_t *nmemb) {
  byte_t **ptr = (byte_t **)pptr;
  if( ptr && *ptr && nmemb ) {
    free(*ptr);
    *ptr = NULL;
    *nmemb = 0;
    return TRUE;
  }
  return FALSE;
}


bool_t ensure_bytes_mem(int numbytes, 
			void *pptr, size_t *nmemb, size_t size) {
  while( numbytes > (long int)((*nmemb) * size) ) {
    if( !grow_mem(pptr, nmemb, size, 1) ) {
      return FALSE;
     }
  }
  return TRUE;
}

char_t *dup_string(const char_t *begin, const char_t *end) {
  char_t *q = NULL;
  if( begin && (end - begin >= 0) ) {
    q = malloc(end - begin + 1);
    if( q ) {
      memcpy(q, begin, end - begin);
      q[end - begin] = '\0';
    }
  }
  return q;
}

/* get first delimiter(s) or end pointer. If end is NULL, searches unboundedly
 * can also be used to find delimiters regardless of esc, if you put esc = '\0' 
 *
 * DOES NOT WORK IF esc IS ALSO A DELIMITER 
 */
const char_t *skip_unescaped_delimiters(const char_t *begin, const char_t *end, const char_t *delims, char_t esc) {
  const char_t *q;
  bool_t shift = FALSE;
  if( begin ) {
    for( ; (!end && *begin) || (begin < end) ; begin++ ) {
      /* printf("FUD{%d,%s,%s,%s}\n", shift, delims, begin, end); */
      if( shift ) {
	shift = FALSE;
      } else {
	shift = (*begin == esc);
	if( !shift ) {
	  for(q = delims; *q && (*q != *begin); q++);
	  if( *q ) {
	    break;
	  }
	}
      }
    }
    return (!end || (begin < end)) ? begin : end;
  }
  return NULL;
}

/* skip a string of the form str  but not esc-str */
const char_t *skip_unescaped_substring(const char_t *begin, const char_t *end, const char_t *str, char_t esc) {
  const char_t *q, *r;
  bool_t shift = FALSE;
  if( begin ) {
    for( ; (!end && *begin) || (begin < end) ; begin++ ) {
      if( shift ) {
  	shift = FALSE;
      } else {
  	shift = (*begin == esc);
  	if( !shift ) {
  	  for(q = str, r = begin; *q && (!end || (r < end)) && (*q == *r);
  	      q++, r++);
  	  if( !*q ) {
  	    break;
  	  }
  	}
      }
    }
    return (!end || (begin < end)) ? begin : end;
  }
  return NULL;
}

const char_t *rskip_unescaped_delimiter(const char_t *begin, const char_t *end, char_t delim, char_t esc) {
  if( begin ) {
    if( !end ) {
      end = begin + strlen(begin);
    }
    while( begin < end ) {
      if( *end == delim ) {
	if( end[-1] != esc ) {
	  return end;
	}
	--end;
      }
      --end;
    }
    return (*end == delim) ? end : NULL;
  }
  return NULL;
}


/* get first delimiter or end pointer. If end is NULL, searches unboundedly */
const char_t *skip_delimiter(const char_t *begin, const char_t *end, char_t delim) {
  while( !end || (begin < end) ) {
    if( (*begin == delim) || (*begin == '\0') || (begin == end) ) {
      return begin;
    }
    begin++;
  }
  return end;
}

const byte_t *skip_byte(const byte_t *begin, const byte_t *end, byte_t delim) {
  while( !end || (begin < end) ) {
    if( (*begin == delim) || (begin == end) ) {
      return begin;
    }
    begin++;
  }
  return end;
}

/* Skip a delimiter on two strings at once, but only if they
 * are the same. Returns > 0 if pointers were moved.
 */
size_t parallel_skip_prefix(const char_t **pp, const char_t **qq, 
			    char_t delim, char_t esc) {
  const char_t *p, *p1, *q, *q1;
  size_t n, m;
  bool_t same, more;
  size_t i = 0;
  char_t delims[2] = { 0 };
  
  p = *pp;
  q = *qq;
  delims[0] = delim;

  do {
    p1 = (char_t *)skip_unescaped_delimiters(p, NULL, delims, esc);
    q1 = (char_t *)skip_unescaped_delimiters(q, NULL, delims, esc);
    n = p1 - p;
    m = q1 - q;
    same = ((m == n) && (memcmp(p,q,n) == 0));
    more = ((*p1 == delim) && (*q1 == delim));

    p = (more ? (p1 + 1) : (same ? p1 : p));
    q = (more ? (q1 + 1) : (same ? q1 : q));
    i++;
  } while( same && more );

  *pp = p;
  *qq = q;

  return i;
}

size_t total_unescaped_delimiters(const char_t *begin, const char_t *end, const char_t *delims, char_t esc) {
  size_t c;
  for(c = 0; (!end && *begin) || (begin < end); c++) {
    begin = skip_unescaped_delimiters(++begin, end, delims, esc);
  }
  return c;
}

/* returns true if the string matches glob, including * and ? wildcards in
 * glob. Matching stops at end or if null char is encountered.
 */ 
bool_t globmatch(const char_t *begin, const char_t *end, const char_t *glob) {
  if( !glob || !begin ) { return TRUE; }
  switch(*glob) {
  case '\0': return ((*begin == '\0') || (begin == end));
  case '?':
    /* skip a full UTF-8 character - first byte is 0x00-0x7F,
     * remaining bytes are 0x80-0xbf 
     */
    begin++;
    while( (begin < end) && (*begin & 0x80) ) { begin++; }
    return globmatch(begin, end, ++glob);
  case '*':
    glob++;
    do {
      begin = (*glob == '?') ? begin : skip_delimiter(begin, end, *glob);
      if( *begin == '\0' || (begin == end) ) { 
	return (*glob == '\0'); 
      }
    } while( !globmatch(begin++, end, glob) );
    return TRUE;
  default:
    return (*begin == *glob) ? globmatch(++begin, end, ++glob) : FALSE;
  }
}

