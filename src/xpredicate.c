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
#include "xpredicate.h"
#include "xpath.h"
#include <string.h>

extern const char_t escc;
extern const char_t xpath_delims[];
extern const char_t xpath_pred_starts[];
extern const char_t xpath_pred_ends[];

bool_t create_xpredicate(xpredicate_t *xp) {
  if( xp ) {
    xp->path = NULL;
    xp->num = 0;
    xp->list = NULL; /* lazy creation during add */
    xp->max = 0;
    return TRUE;
  }
  return FALSE;
}

bool_t free_xpredicate(xpredicate_t *xp) {
  if( xp ) {
    xp->num = 0;
    free_mem(&xp->list, &xp->max);
    return TRUE;
  }
  return FALSE;
}

bool_t add_xpredicate(xpredicate_t *xp, int offset, const char_t *predicate) {
  xpred_t *x;
  if( xp ) {
    if( xp->num >= xp->max ) {
      grow_mem(&xp->list, &xp->max, sizeof(xpred_t), 1);
    }
    if( xp->num < xp->max ) {
      x = &xp->list[xp->num];
      xp->num++;
      
      x->offset = offset;
      x->expr = xpred_num;
      x->args.num.target = atoi(predicate);
      x->args.num.count = 0;

      return (x->args.num.target > 0);
    }
  }
  return FALSE;
}

xpred_t *get_xpredicate(const xpredicate_t *xp, int n) {
  return (xp && (0 <= n) && (n < xp->num)) ? &xp->list[n] : NULL;
}

/* a predicate is valid if all the (precalculated) expression values are true.
 * Special case: if num = 0, then always valid. 
 */
bool_t valid_xpredicate(const xpredicate_t *xp) {
  int i;
  if( xp ) {
    for(i = 0; i < xp->num; i++) {
      /* printf("valid_xpredicate(%.*s, %d)\n", xp->list[i].offset, xp->path, xp->list[i].value); */
      if( xp->list[i].value == FALSE ) {
  	return FALSE;
      }
    }
    return TRUE;
  }
  return FALSE;
}

bool_t compile_xpredicate(xpredicate_t *xp, const char_t *path) {
  const char_t *begin, *end, *p, *q;
  if( xp && path ) {
    /* printf("compile_xpredicate(%s)\n", path); */
    xp->path = path;
    begin = path;
    end = path + strlen(path);
    while( begin < end ) {
      p = skip_unescaped_delimiters(begin, end, xpath_pred_starts, escc);
      if( p < end ) {
	q = skip_unescaped_delimiters(p, end, xpath_delims, escc);
	if( !add_xpredicate(xp, q - begin, p + 1) ) {
	  errormsg(E_ERROR, "invalid predicate [%s\n", p + 1);
	}
      }
      begin = p + 1;
    }
    return TRUE;
  }
  return FALSE;
}


bool_t create_xpredicatelist(xpredicatelist_t *xpl) {
  if( xpl ) {
    xpl->num = 0;
    return create_mem(&xpl->list, &xpl->max, sizeof(xpredicate_t), 4);
  }
  return FALSE;
}

bool_t free_xpredicatelist(xpredicatelist_t *xpl) {
  if( xpl ) {
    if( xpl->list ) {
      reset_xpredicatelist(xpl);
      free_mem(&xpl->list, &xpl->max);
    }
  }
  return FALSE;
}

bool_t reset_xpredicatelist(xpredicatelist_t *xpl) {
  int i;
  if( xpl ) {
    for(i = 0; i < xpl->num; i++) {
      free_xpredicate(&xpl->list[i]);
    }
    xpl->num = 0;
    return TRUE;
  }
  return FALSE;
}

bool_t add_xpredicatelist(xpredicatelist_t *xpl, const char_t *xpath) {
  if( xpl ) {
    if( xpl->num >= xpl->max ) {
      grow_mem(&xpl->list, &xpl->max, sizeof(xpredicate_t), 4);
    }
    if( xpl->num < xpl->max ) {
      if( create_xpredicate(&xpl->list[xpl->num]) &&
	  compile_xpredicate(&xpl->list[xpl->num], xpath) ) {
	xpl->num++;
	return TRUE;
      }
    }
  }
  return FALSE;
}

bool_t compile_xpredicatelist(xpredicatelist_t *xpl, cstringlst_t lst) {
  int i;
  if( xpl ) {
    reset_xpredicatelist(xpl);
    for(i = 0; lst && lst[i]; i++) {
      add_xpredicatelist(xpl, lst[i]);
    }
    return TRUE;
  }
  return FALSE;
}

xpredicate_t *get_xpredicatelist(const xpredicatelist_t *xpl, int n) {
  return (xpl && (0 <= n) && (n < xpl->num)) ? &xpl->list[n] : NULL;
}

/* Increment the counts for ALL registered predicates, etc.
 * Each predicate expression is recalculated.
 */
bool_t update_xpredicatelist(xpredicatelist_t *xpl, const char_t *xpath, const char_t **att) {
  int i, j, k;
  xpredicate_t *xp;
  if( xpl && xpath ) {
    for(i = 0; i < xpl->num; i++) {
      xp = &xpl->list[i];
      for(j = 0; j < xp->num; j++) {
	k = match_no_att_no_pred_xpath(xp->path, 
				       xp->path + xp->list[j].offset, 
				       xpath);
	if( k == 0 ) {
	  xp->list[j].args.num.count++;
	  xp->list[j].value = 
	    (xp->list[j].args.num.count == xp->list[j].args.num.target);
	  while( ++j < xp->num ) {
	    xp->list[j].args.num.count = 0;
	  }
	} else if( k > 0 ) {
	  break;
	}
      }
    }
    return TRUE;
  }
  return FALSE;
}

/* return true if xpredicatelist_t contains at least one currently
   valid predicate */
bool_t find_valid_predicatelist(xpredicatelist_t *xpl) {
  int n;
  if( xpl ) {
    for(n = 0; n < xpl->num; n++) {
      if( valid_xpredicate(&xpl->list[n]) ) {
	return TRUE;
      }
    }
  }
  return FALSE;
}
