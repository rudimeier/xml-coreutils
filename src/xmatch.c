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
#include "error.h"
#include "mem.h"
#include "xmatch.h"
#include <string.h>

bool_t create_xmatcher(xmatcher_t *xm) {
  if( xm ) {
    xm->xpath_count = 0;
    xm->max_xpath = 0;
    if( !grow_mem((byte_t **)&xm->xpath, &xm->max_xpath, sizeof(char_t *), 16) ) {
      errormsg(E_ERROR, "cannot create xpath list\n");
      return FALSE;
    }
    return TRUE;
  }
  return FALSE;
}

bool_t free_xmatcher(xmatcher_t *xm) {
  if( xm ) {
    if( xm->xpath ) {
      /* don't free individual xpaths */
      free(xm->xpath);
      xm->xpath = NULL;
      xm->xpath_count = 0;
    }
    return TRUE;
  }
  return FALSE;
}

bool_t reset_xmatcher(xmatcher_t *xm) {
  if( xm ) {
    /* don't free individual xpaths */
    xm->xpath_count = 0;
    return TRUE;
  }
  return FALSE;
}

bool_t push_xmatcher(xmatcher_t *xm, const char_t *path) {
  if( xm && xm->xpath ) {
    if( xm->xpath_count + 1 >= xm->max_xpath ) {
      if( !grow_mem((byte_t **)&xm->xpath, &xm->max_xpath, sizeof(char_t *), 16) ) {
	errormsg(E_ERROR, "cannot grow xpath list\n");
	return FALSE;
      } 
    }
    xm->xpath[xm->xpath_count++] = path;
    return TRUE;
  }
  return FALSE;
}

bool_t pop_xmatcher(xmatcher_t *xm) {
  if( xm && xm->xpath ) {
    if( xm->xpath_count > 0 ) {
      xm->xpath[xm->xpath_count--] = NULL;
      return TRUE;
    }
  }
  return FALSE;
}

const char_t *get_xmatcher(xmatcher_t *xm, size_t n) {
  return (xm && (n < xm->xpath_count)) ? xm->xpath[n] : NULL;
}

bool_t push_unique_xmatcher(xmatcher_t *xm, const char_t *path) {
  int i;
  for(i = 0; i < xm->xpath_count; i++) {
    if( strcmp(xm->xpath[i], path) == 0 ) {
      return FALSE;
    }
  }
  return push_xmatcher(xm, path);
}

bool_t do_match_xmatcher(xmatcher_t *xm, size_t n, const char_t *xpath) {
  const char_t *p;
  if( xm && xpath && (n < xm->xpath_count) ) {
    p = xm->xpath[n];
    while( *p && (*p == *xpath) ) { p++; xpath++; }
    return (*p == '\0');
/*     return (strncmp(xpath, xm->xpath[n], strlen(xm->xpath[n])) == 0); */
  }
  return FALSE;
}

bool_t find_first_xmatcher(xmatcher_t *xm, const char_t *xpath, size_t *n) {
  *n = -1;
  return find_next_xmatcher(xm, xpath, n);
}

bool_t find_next_xmatcher(xmatcher_t *xm, const char_t *xpath, size_t *n) {
  size_t i;
  if( xm ) {
    for( i = *n + 1; i < xm->xpath_count; i++ ) {
      if( do_match_xmatcher(xm, i, xpath) ) {
	*n = i;
	return TRUE;
      }
    }    
  }
  return FALSE;
}
