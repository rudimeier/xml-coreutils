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

#include "common.h"
#include "myerror.h"
#include "mem.h"
#include "xattribute.h"
#include "xpath.h"
#include <string.h>

extern const char_t escc;
extern const char_t xpath_att_names[];
extern const char_t xpath_specials[];

bool_t create_xattribute(xattribute_t *xa) {
  if( xa ) {
    memset(xa, 0, sizeof(xattribute_t));
    return TRUE;
  }
  return FALSE;
}

bool_t free_xattribute(xattribute_t *xa) {
  if( xa ) {
    /* nothing */
    return TRUE;
  }
  return FALSE;
}

bool_t match_xattribute(const xattribute_t *xa, const char_t *name) {
  return (xa && xa->begin && name) ? 
    ( (strncmp(xa->begin + 1, name, xa->end - xa->begin - 1) == 0) ||
      (strncmp(xa->begin, "@*", xa->end - xa->begin) == 0) ) : FALSE;
}

bool_t precheck_xattribute(const xattribute_t *xa) {
  if( xa ) {
    return xa->begin ? xa->precheck : TRUE;
  }
  return FALSE;
}

bool_t compile_xattribute(xattribute_t *xa, const char_t *path) {
  const char_t *end;
  if( xa && path ) {
    xa->path = path;
    end = path + strlen(path);
    /* (begin,end) string includes initial '@' on purpose */
    xa->begin = rskip_unescaped_delimiter(path, end, *xpath_att_names, escc);
    xa->end = xa->begin ?
      skip_unescaped_delimiters(xa->begin + 1, end, xpath_specials, escc) : NULL;
    xa->precheck = FALSE;
    return TRUE;
  }
  return FALSE;
}

bool_t create_xattributelist(xattributelist_t *xal) {
  if( xal ) {
    xal->num = 0;
    return create_mem(&xal->list, &xal->max, sizeof(xattribute_t), 4);
  }
  return FALSE;
}

bool_t free_xattributelist(xattributelist_t *xal) {
  if( xal ) {
    if( xal->list ) {
      reset_xattributelist(xal);
      free_mem(&xal->list, &xal->max);
    }
  }
  return FALSE;
}

bool_t reset_xattributelist(xattributelist_t *xal) {
  int i;
  if( xal ) {
    for(i = 0; i < xal->num; i++) {
      free_xattribute(&xal->list[i]);
    }
    xal->num = 0;
    return TRUE;
  }
  return FALSE;
}

bool_t add_xattributelist(xattributelist_t *xpl, const char_t *xpath) {
  if( xpl ) {
    if( xpl->num >= xpl->max ) {
      grow_mem(&xpl->list, &xpl->max, sizeof(xattribute_t), 4);
    }
    if( xpl->num < xpl->max ) {
      if( create_xattribute(&xpl->list[xpl->num]) &&
	  compile_xattribute(&xpl->list[xpl->num], xpath) ) {
	xpl->num++;
	return TRUE;
      }
    }
  }
  return FALSE;
}

bool_t compile_xattributelist(xattributelist_t *xal, cstringlst_t lst) {
  int i;
  if( xal ) {
    reset_xattributelist(xal);
    for(i = 0; lst && lst[i]; i++) {
      add_xattributelist(xal, lst[i]);
    }
    return TRUE;
  }
  return FALSE;
}

xattribute_t *get_xattributelist(const xattributelist_t *xal, int n) {
  return (xal && (0 <= n) && (n < xal->num)) ? &xal->list[n] : NULL;
}

/* this matches when xa->path = xpath = "somepath/@name" */
bool_t check_xattributelist(const xattributelist_t *xal, const char_t *xpath, const char_t *name) {
  xattribute_t *xa;
  int i;
  if( xal && xpath && name ) {
    for(i = 0; i < xal->num; i++) {
      xa = &xal->list[i];
      if( match_xattribute(xa, name) &&
	  (0 == match_no_att_no_pred_xpath(xa->path, xa->begin, xpath)) ) {
	return TRUE;
      }
    }
  }
  return FALSE;
}

bool_t update_xattributelist(xattributelist_t *xal, const char_t **att) {
  int i, j;
  xattribute_t *xa;
  if( xal && att ) {
    for(i = 0; i < xal->num; i++) {
      xa = &xal->list[i];
      if( xa->begin ) {
	xa->precheck = FALSE;
	for(j = 0; att[j] ; j += 2) {
	  if( match_xattribute(xa, att[j]) ) {
	    xa->precheck = TRUE;
	  }
	}
      }
    }
    return TRUE;
  }
  return FALSE;
}

