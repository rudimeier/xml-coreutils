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
#include "mem.h"
#include <string.h>

bool_t grow_mem(byte_t **ptr, size_t *nmemb, size_t size, size_t minmemb) {
  byte_t *p;
  if( ptr && nmemb ) {
    if( !*ptr ) {
      *nmemb = minmemb;
      *ptr = calloc( *nmemb, size );
      return ( *ptr != NULL);
    } else {
      p = realloc(*ptr, size * 2 * (*nmemb));
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

bool_t create_mem(byte_t **ptr, size_t *nmemb, size_t size, size_t min) {
  if( ptr && nmemb ) {
    *ptr = NULL;
    *nmemb = 0;
    return grow_mem((byte_t **)ptr, nmemb, size, min);
  }
  return FALSE;
}

bool_t free_mem(byte_t **ptr, size_t *nmemb) {
  if( ptr && *ptr && nmemb ) {
    free(*ptr);
    *ptr = NULL;
    *nmemb = 0;
    return TRUE;
  }
  return FALSE;
}

/* get first delimiter or end pointer. If end is NULL, searches unboundedly */
const char_t *find_unescaped_delimiter(const char_t *begin, const char_t *end, char_t delim, char_t esc) {
  if( begin ) {
    while( !end || (begin < end) ) {
      if( *begin == esc ) { begin++; }
      if( (*begin == '\0') || (*begin == delim) || (begin == end) ) { break; }
      begin++;
    }
    return begin;
  }
  return NULL;
}

/* get first delimiter or end pointer. If end is NULL, searches unboundedly */
const char_t *find_delimiter(const char_t *begin, const char_t *end, char_t delim) {
  while( !end || (begin < end) ) {
    if( (*begin == delim) || (*begin == '\0') || (begin == end) ) {
      return begin;
    }
    begin++;
  }
  return end;
}

const byte_t *find_byte(const byte_t *begin, const byte_t *end, byte_t delim) {
  while( !end || (begin < end) ) {
    if( (*begin == delim) || (begin == end) ) {
      return begin;
    }
    begin++;
  }
  return end;
}

