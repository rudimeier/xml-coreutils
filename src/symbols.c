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

#include "symbols.h"
#include "mem.h"
#include "myerror.h"

#include <string.h>

/* used by jumptable */

#define FILLEDP(a) ((a)->id)
#define EQUALP(a,b) ((a)==(b))
#define SET(a,b) (a = (b))

#define SETMARK(a) ((a)->mark = (unsigned int)1)
#define UNSETMARK(a) ((a)->mark = (unsigned int)0)
#define MARKEDP(a) ((a)->mark == (unsigned int)1)


bool_t create_jumptable(jumptable_t *jt) {
  if( jt ) {
    jt->num = 0;
    create_mem(&jt->hash, &jt->maxnum, sizeof(jump_t), 16);
    return TRUE;
  }
  return FALSE;
}

bool_t free_jumptable(jumptable_t *jt) {
  if( jt ) {
    jt->num = 0;
    free_mem(&jt->hash, &jt->maxnum);
    return TRUE;
  }
  return FALSE;
}

jump_t *find_in_jumptable(jumptable_t *jt, size_t id) {
  register jump_t *i, *loop;
  /* start at id */
  i = loop = &jt->hash[id % jt->maxnum];

  while( FILLEDP(i) ) {
    if( EQUALP(i->id,id) ) {
      return i; /* found id */
    } else {
      i++; /* not found */
      /* wrap around */
      i = (i >= &jt->hash[jt->maxnum]) ? jt->hash : i; 
      if( i == loop ) {
	return NULL; /* when hash table is full */
      }
    }
  }

  /* empty slot, so not found */

  return i; 
}


bool_t grow_jumptable(jumptable_t *jt) {
  jump_t *i, *temp_item;
  size_t oldsize;
  int c;
  if( jt ) {
    oldsize = jt->maxnum;
    if( grow_mem(&jt->hash, &jt->maxnum, sizeof(jump_t), 16) ) {

      memset(&jt->hash[oldsize], 0, (jt->maxnum - oldsize) * sizeof(jump_t));
      
      /* now mark every used slot */
      for(c = 0; c < oldsize; c++) {
	if( FILLEDP(&jt->hash[c]) ) {
	  SETMARK(&jt->hash[c]);
	}
      }

      /* now relocate each marked slot and clear it */
      for(c = 0; c < oldsize; c++) {
	while( MARKEDP(&jt->hash[c]) ) {
	  /* find where it belongs */
	  i = &jt->hash[jt->hash[c].id % jt->maxnum];
	  while( FILLEDP(i) && !MARKEDP(i) ) {
	    i++;
	    i = (i > &jt->hash[jt->maxnum]) ? jt->hash : i;
	  } /* guaranteed to exit since hash is larger than original */

	  /* clear the mark - this must happen after we look for i,
	   since it should be possible to find i == jt->hash[c] */
	  UNSETMARK(&jt->hash[c]); 

	  /* now refile */
	  if( i != &jt->hash[c] ) {
	    if( MARKEDP(i) ) {
	      /* swap */
	      memcpy(&temp_item, i, sizeof(jump_t));
	      memcpy(i, &jt->hash[c], sizeof(jump_t));
	      memcpy(&jt->hash[c], &temp_item, sizeof(jump_t));
	    } else {
	      /* copy and clear */
	      memcpy(i, &jt->hash[c], sizeof(jump_t));
	      memset(&jt->hash[c], 0, sizeof(jump_t));
	    }
	  } 
	  /* now &jt->hash[c] is marked iff there was a swap */
	}
      }
    }
  }
  return FALSE;
}

bool_t insert_jumptable(jumptable_t *jt, unsigned int id, size_t pos) {
  jump_t *i;
  if( jt ) {
    if( (100 * jt->num < 95 * jt->maxnum) || grow_jumptable(jt) ) {
      i = find_in_jumptable(jt, id);
      if( !FILLEDP(i) ) {
	i->id = id;
	i->pos = pos;
	jt->num++;
	return TRUE;
      }
    }
   }
  return FALSE;
}


bool_t create_symbols(symbols_t *sb) {
  if( sb ) {
    create_mem(&sb->bigs, &sb->bigmax, sizeof(byte_t), 64);
    sb->bigs_len = 0;
    sb->last = -1;
    create_jumptable(&sb->jt);
    return TRUE;
  }
  return FALSE;
}

bool_t free_symbols(symbols_t *sb) {
  if( sb ) {
    free_mem(&sb->bigs, &sb->bigmax);
    free_jumptable(&sb->jt);
    return TRUE;
  }
  return FALSE;
}

symbol_t next_symbols(symbols_t *sb, bool_t generate,
			    const char_t *begin, const char_t *end) {
  byte_t *p;
  if( sb && generate && (begin <= end) ) {
    p = memccpy(sb->bigs + sb->bigs_len, begin, 0, end - begin);
    if( p ) { *p++ = 0; }

    sb->bigs_len = p - sb->bigs;
    sb->last++;
    memcpy(p, &sb->last, sizeof(symbol_t));
    sb->bigs_len += sizeof(symbol_t);
    return sb->last;
  }
  return -1;
}

/* returns -1 on failure, >= 0 is an actual symbol value. String is
 * searched until terminating null or end. The empty string cannot
 * receive a symbol value.
 */
symbol_t getvalue_symbols(symbols_t *sb, bool_t generate,
			  const char_t *begin, const char_t *end) {
  const byte_t *p;
  bool_t bdone, pdone;
  jump_t *j;

  if( sb && begin && *begin ) {
    if( !end ) {
      end = begin + strlen(begin);
    }
    
    if( (sb->bigs_len + (end - begin) + 1 + sizeof(symbol_t)) > sb->bigmax ) {
      if( !grow_mem(&sb->bigs, &sb->bigmax, sizeof(byte_t), 16) ) {
	errormsg(E_WARNING, "out of symbol memory\n");
	return -1;
      }
    }
    
    if( sb->bigs_len == 0 ) {
      return next_symbols(sb, generate, begin, end);
    }

    p = sb->bigs;
    while( *begin && (begin < end) ) {

      /* skip common prefix */
      while( (*begin == *p) && *begin && (begin < end) ) {
	p++;
	begin++;
      }

      bdone = ((begin == end) || (*begin == 0));
      pdone = (*p == 0);

      /* either begin string is empty, or p string is empty, or both */
      if( bdone && pdone ) {

	/* we found the string */
	return *((symbol_t *)p);

      } else if( bdone ) { /* a longer p string was inserted first */

	/* follow as many branches as possible */
	j = find_in_jumptable(&sb->jt, p - sb->bigs);
	while( j && sb->bigs[j->pos] ) {
	  p = sb->bigs + j->pos;
	  j = find_in_jumptable(&sb->jt, p - sb->bigs);
	}

	if( j && (sb->bigs[j->pos] == 0) ) { /* found the string */
	  p = sb->bigs + j->pos;
	  return *((symbol_t *)p);
	} else { /* there are no further branches at p */
	  insert_jumptable(&sb->jt, p - sb->bigs, sb->bigs_len);
	  return next_symbols(sb, generate, begin, end);
	}	  

      } else { /* 0 != *begin != *p */

	j = find_in_jumptable(&sb->jt, p - sb->bigs);
	if( !j ) { /* no branch at p, so we append begin */
	  insert_jumptable(&sb->jt, p - sb->bigs, sb->bigs_len);
	  return next_symbols(sb, generate, begin, end);
	}

	/* in here, branch exists */
	p = sb->bigs + j->pos;
      }
      /* and back to while */
    }
  }
  return -1;
}
