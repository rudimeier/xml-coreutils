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
#include "stringlist.h"
#include "mem.h"
#include "myerror.h"
#include <string.h>

bool_t create_stringlist(stringlist_t *sl) {
  if( sl ) {
    sl->num = 0;
    sl->flags = 0;
    return create_mem(&sl->list, &sl->max, sizeof(char_t *), 16);
  }
  return FALSE;
}

bool_t free_stringlist(stringlist_t *sl) {
  if( sl ) {
    if( sl->list ) {
      reset_stringlist(sl);
      free_mem(&sl->list, &sl->max);
    }
  }
  return FALSE;
}

bool_t reset_stringlist(stringlist_t *sl) {
  int i;
  if( sl ) {
    if( !checkflag(sl->flags,STRINGLIST_DONTFREE) ) {
      for(i = 0; i < sl->num; i++) {
	if( sl->list[i] ) { free((void *)sl->list[i]); }
      }
    }
    sl->num = 0;
    return TRUE;
  }
  return FALSE;
}

/* this command interferes with argv_stringlist() */
bool_t strikeout_stringlist(stringlist_t *sl, const char_t *name) {
  bool_t ok = FALSE;
  int i;
  if( sl ) {
    for(i = 0; i < sl->num; i++) {
      if( sl->list[i] && (strcmp(name, sl->list[i]) == 0) ) {
	/* keep this as is */
	name = sl->list[i];
	sl->list[i] = NULL;
	if( !checkflag(sl->flags,STRINGLIST_DONTFREE) ) {
	  free((void *)name);
	}
	ok = TRUE;
      }
    }
    return ok;
  }
  return FALSE;
}

const char_t *get_stringlist(const stringlist_t *sl, int n) {
  return (sl && (0 <= n) && (n < sl->num)) ? sl->list[n] : NULL;
}

int find_stringlist(const stringlist_t *sl, const char_t *s) {
  int i;
  if( sl && s ) {
    for(i = 0; i < sl->num; i++) {
      if( sl->list[i] && (strcmp(sl->list[i], s) == 0) ) {
	return i;
      }
    }
  }
  return -1;
}

/* can add name = NULL, but causes problems with argv_stringlist */
bool_t add_stringlist(stringlist_t *sl, const char_t *name, flag_t flags) {
  if( sl ) {
    if( sl->num >= sl->max ) {
      grow_mem(&sl->list, &sl->max, sizeof(char_t *), 16);
    }
    if( sl->num < sl->max ) {
      if( sl->flags == 0 ) { setflag(&sl->flags,flags); }
      if( sl->flags == flags ) {
	sl->list[sl->num] = (checkflag(sl->flags,STRINGLIST_STRDUP) ? 
			     (name ? strdup(name) : NULL) : (char_t *)name);
	sl->num++;
	return TRUE;
      }
    }
  }
  return FALSE;
}

/* must add name != NULL */
bool_t add_unique_stringlist(stringlist_t *sl, const char_t *name, flag_t flags) {
  int i; 
  if( sl && name ) {
    for(i = 0; i < sl->num; i++) {
      if( sl->list[i] && (strcmp(name, sl->list[i]) == 0) ) {
	return FALSE;
      }
    }
    return add_stringlist(sl, name, flags);
  }
  return FALSE;
}

/* return the list of strings, with a NULL pointer after the last string */
const char **argv_stringlist(stringlist_t *sl) {
  if( sl ) {
    if( (sl->num < sl->max) || 
	grow_mem(&sl->list, &sl->max, sizeof(char_t *), 16) ) {
      sl->list[sl->num] = NULL;
      return sl->list;
    }
  }
  return NULL;
}

