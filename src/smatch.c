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
#include "smatch.h"
#include <string.h>

bool_t create_smatcher(smatcher_t *sm) {
  if( sm ) {
    sm->smatch_count = 0;
    sm->max_smatch = 0;
    sm->flags = 0;
    if( !create_mem(&sm->smatch, &sm->max_smatch, sizeof(char_t *), 16) ) {
      errormsg(E_ERROR, "cannot create smatch list\n");
      return FALSE;
    }
    if( !create_mem(&sm->sregex, &sm->max_smatch, sizeof(regex_t), 16) ) {
      errormsg(E_ERROR, "cannot create sregex list\n");
      return FALSE;
    }
    return TRUE;
  }
  return FALSE;
}

bool_t free_smatcher(smatcher_t *sm) {
  int i;
  if( sm ) {
    if( sm->smatch ) {
      /* free the regexes */
      if( sm->sregex ) {
	for(i = 0; i < sm->smatch_count; i++) {
	  regfree(&sm->sregex[i]);
	}
	free(sm->sregex);
      }
      /* don't free individual xpaths */
      free(sm->smatch);
      sm->sregex = NULL;
      sm->smatch = NULL;
      sm->smatch_count = 0;
    }
    return TRUE;
  }
  return FALSE;
}

bool_t reset_smatcher(smatcher_t *sm) {
  if( sm ) {
    /* don't free individual xpaths */
    sm->smatch_count = 0;
    return TRUE;
  }
  return FALSE;
}

bool_t push_smatcher(smatcher_t *sm, const char_t *match, flag_t flags) {
  int e;
#define EBUF 127
  char ebuf[EBUF + 1];
  flag_t sf;

  if( sm && sm->smatch && sm->sregex ) {
    if( sm->smatch_count + 1 >= sm->max_smatch ) {
      if( !grow_mem(&sm->smatch, &sm->max_smatch, sizeof(char_t *), 16) ) {
	errormsg(E_ERROR, "cannot grow smatch list\n");
	return FALSE;
      } 
      if( !grow_mem(&sm->sregex, &sm->max_smatch, sizeof(regex_t), 16) ) {
	errormsg(E_ERROR, "cannot grow sregex list\n");
	return FALSE;
      } 
    }

    sf = REG_NOSUB;
    if( checkflag(flags, SMATCH_FLAG_ICASE) ) {
      sf |= REG_ICASE;
    }
    if( checkflag(flags, SMATCH_FLAG_EXTEND) ) {
      sf |= REG_EXTENDED;
    }

    e = regcomp(&sm->sregex[sm->smatch_count], match, sf);
    if( e != 0 ) {
      regerror(e, &sm->sregex[sm->smatch_count], ebuf, EBUF);
      errormsg(E_ERROR, "%s %s\n", match, ebuf);
      return FALSE;
    }
    sm->smatch[sm->smatch_count++] = match;
    return TRUE;
  }
  return FALSE;
}

bool_t pop_smatcher(smatcher_t *sm) {
  if( sm && sm->smatch ) {
    if( sm->smatch_count > 0 ) {
      regfree(&sm->sregex[sm->smatch_count]);
      sm->smatch[sm->smatch_count--] = NULL;
      return TRUE;
    }
  }
  return FALSE;
}

const char_t *get_smatcher(smatcher_t *sm, size_t n) {
  return (sm && (n < sm->smatch_count)) ? sm->smatch[n] : NULL;
}

bool_t push_unique_smatcher(smatcher_t *sm, const char_t *match, flag_t flags) {
  int i;
  for(i = 0; i < sm->smatch_count; i++) {
    if( strcmp(sm->smatch[i], match) == 0 ) {
      return FALSE;
    }
  }
  return push_smatcher(sm, match, flags);
}

bool_t do_match_smatcher(smatcher_t *sm, size_t n, const char_t *s) {
  if( sm && s && (n < sm->smatch_count) ) {
    return (regexec(&sm->sregex[n], s, 0, NULL, 0) == 0);
  }
  return FALSE;
}

bool_t find_first_smatcher(smatcher_t *sm, const char_t *s, size_t *n) {
  *n = -1;
  return find_next_smatcher(sm, s, n);
}

bool_t find_next_smatcher(smatcher_t *sm, const char_t *s, size_t *n) {
  size_t i;
  if( sm ) {
    for( i = *n + 1; i < sm->smatch_count; i++ ) {
      if( do_match_smatcher(sm, i, s) ) {
	*n = i;
	return TRUE;
      }
    }    
    return FALSE;
  }
  return FALSE;
}

