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
#include "myerror.h"
#include "mem.h"
#include "xmatch.h"
#include "entities.h"
#include "cstring.h"

extern const char_t escc;
extern const char_t xpath_delims[];
extern const char_t xpath_att_names[];
extern const char_t xpath_pred_starts[];
extern const char_t xpath_pred_ends[];

bool_t create_xmatcher(xmatcher_t *xm) {
  if( xm ) {
    xm->xpath_count = 0;
    xm->max_xpath = 0;
    xm->flags = 0;
    if( !grow_mem(&xm->xpath, &xm->max_xpath, sizeof(char_t *), 16) ) {
      errormsg(E_ERROR, "cannot create xpath list\n");
      return FALSE;
    }
    return TRUE;
  }
  return FALSE;
}

/* lst == NULL is ok */
bool_t create_wrap_xmatcher(xmatcher_t *xm, cstringlst_t lst) {
  int i;
  if( xm ) {
    xm->xpath = lst;
    xm->max_xpath = 0;
    xm->flags = 0;
    for(i = 0; lst && lst[i]; i++);
    xm->xpath_count = i;
    setflag(&xm->flags,XMATCHER_DONTFREE);
    return TRUE;
  }
  return FALSE;
}

bool_t free_xmatcher(xmatcher_t *xm) {
  if( xm ) {
    if( xm->xpath ) {
      if( !checkflag(xm->flags,XMATCHER_DONTFREE) ) {
	/* don't free individual xpaths */
	free_mem(xm->xpath, &xm->max_xpath);
      }
      xm->xpath = NULL;
      xm->xpath_count = 0;
      xm->flags = 0;
    }
    return TRUE;
  }
  return FALSE;
}

bool_t reset_xmatcher(xmatcher_t *xm) {
  if( xm ) {
    free_xmatcher(xm);
    if( !grow_mem(&xm->xpath, &xm->max_xpath, sizeof(char_t *), 16) ) {
      errormsg(E_ERROR, "cannot create xpath list\n");
      return FALSE;
    }
    return TRUE;
  }
  return FALSE;
}

bool_t push_xmatcher(xmatcher_t *xm, const char_t *path, flag_t flags) {
  if( xm && xm->xpath ) {
    if( xm->xpath_count + 1 >= xm->max_xpath ) {
      if( !grow_mem(&xm->xpath, &xm->max_xpath, sizeof(char_t *), 16) ) {
	errormsg(E_ERROR, "cannot grow xpath list\n");
	return FALSE;
      } 
    }
    if( xm->flags == 0 ) { setflag(&xm->flags,flags); }
    if( xm->flags == flags ) {
      xm->xpath[xm->xpath_count++] = (checkflag(xm->flags,XMATCHER_STRDUP) ?
				      strdup(path) : (char_t *)path);
      return TRUE;
    }
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

const char_t *get_xmatcher(xmatcher_t *xm, int n) {
  return (xm && (n >= 0) && (n < xm->xpath_count)) ? xm->xpath[n] : NULL;
}

bool_t push_unique_xmatcher(xmatcher_t *xm, const char_t *path, flag_t flags) {
  int i;
  for(i = 0; i < xm->xpath_count; i++) {
    if( strcmp(xm->xpath[i], path) == 0 ) {
      return FALSE;
    }
  }
  return push_xmatcher(xm, path, flags);
}

/* return true if xmatcher contains a prefix of xpath */
bool_t find_prefix_xmatcher(xmatcher_t *xm, const xpath_t *xpath) {
  int n;
  if( xm && xpath ) {
    for(n = 0; n < xm->xpath_count; n++) {
      if( match_prefix_xpath(xm->xpath[n], string_xpath(xpath)) ) {
	return TRUE; 
      }
    }
  }
  return FALSE;
}

bool_t do_match_xmatcher(xmatcher_t *xm, size_t n, const char_t *path) {
  if( xm && path && (n < xm->xpath_count) ) {
    /* printf("do_match[%s,%s]\n", xm->xpath[n],xpath); */
    return match_prefix_xpath(xm->xpath[n], path);
  }
  return FALSE;
}

bool_t find_first_xmatcher(xmatcher_t *xm, const char_t *xpath, int *n) {
  *n = -1;
  return find_next_xmatcher(xm, xpath, n);
}

bool_t find_next_xmatcher(xmatcher_t *xm, const char_t *xpath, int *n) {
  int i;
  if( xm ) {
    for( i = *n + 1; i < xm->xpath_count; i++ ) {
      if( match_prefix_xpath(xm->xpath[i], xpath) ) {
	*n = i;
	return TRUE;
      }
    }    
  }
  return FALSE;
}

