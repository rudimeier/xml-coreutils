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
#include "error.h"
#include "mem.h"
#include "xpath.h"
#include <string.h>

bool_t create_xpath(xpath_t *xp) {
  if( xp ) {
    return create_mem(&xp->path, &xp->pathlen, sizeof(char_t), 64);
  }
  return FALSE;
}

bool_t free_xpath(xpath_t *xp) {
  if( xp ) {
    if( xp->path ) {
      free(xp->path);
      xp->path = NULL;
      xp->pathlen = 0;
    }
    return TRUE;
  }
  return FALSE;
}

bool_t reset_xpath(xpath_t *xp) {
  if( xp ) {
    xp->path[0] = '\0'; 
    return TRUE;
  }
  return FALSE;
}

bool_t push_tag_xpath(xpath_t *xp, const char_t *tagname) {
  size_t l;
  char_t *delim = "/";
  if( xp && tagname ) {
    l = strlen(tagname) + strlen(delim) + strlen(xp->path) + 1;
    while( l > xp->pathlen ) {
      if( !grow_mem(&xp->path, &xp->pathlen, sizeof(char_t), 64) ) {
	return FALSE;
      }
    }
    strcat(strcat(xp->path, delim), tagname);
    return TRUE;
  }
  return FALSE;
}

bool_t push_node_xpath(xpath_t *xp, nodetype_t t) {
  const char_t *label = NULL;
  if( xp ) {
    switch(t) {
    case n_comment:
      label = "COMMENT";
      break;
    case n_dfault:
      label = "DFAULT";
      break;
    case n_pidata:
      label = "PIDATA";
      break;
    case n_chardata:
      label = "CHARDATA";
      break;
    case n_none:
    case n_space:
    case n_start_tag:
    case n_end_tag:
    case n_start_cdata:
    case n_end_cdata:
      /* nothing */
      break;
    }
    return label ? push_tag_xpath(xp, label) : FALSE;
  }
  return FALSE;
}

bool_t push_predicate_xpath(xpath_t *xp, const char_t *pred) {
  size_t l;
  char_t *delim1 = "[";
  char_t *delim2 = "]";
  if( xp && pred ) {
    l = strlen(pred) + strlen(delim1) + strlen(delim2) + strlen(xp->path) + 1;
    while( l > xp->pathlen ) {
      if( !grow_mem(&xp->path, &xp->pathlen, sizeof(char_t), 64) ) {
	return FALSE;
      }
    }
    strcat(strcat(strcat(xp->path, delim1), pred), delim2);
    return TRUE;
  }
  return FALSE;
}

bool_t pop_xpath(xpath_t *xp) {
  char_t *p;
  if( xp ) {
    p = strrchr(xp->path, '/');
    if( p ) { 
      *p = '\0'; 
    }
    return (p != NULL);
  }
  return FALSE;
}

bool_t write_xpath(xpath_t *xp, const char_t *path, size_t pathlen) {
  size_t l, m;
  if( xp && xp->path ) {
    l = strlen(xp->path);
    m = l + pathlen + 1;
    while( l > xp->pathlen ) {
      if( !grow_mem(&xp->path, &xp->pathlen, sizeof(char_t), 64) ) {
	return FALSE;
      }
    }
    memcpy(xp->path + l, path, pathlen);
    xp->path[m-1] = '\0';
    return TRUE;
  }
  return FALSE;
}

bool_t append_xpath(xpath_t *xp, const char_t *tag, size_t len) {
  if( xp && tag ) {
    return ( write_xpath(xp, "/", 1) && write_xpath(xp, tag, len) );
  }
  return FALSE;
}

bool_t copy_xpath(xpath_t *dest, xpath_t *src) {
  if( dest && src ) {
    return reset_xpath(dest) && write_xpath(dest, src->path, strlen(src->path));
  }
  return FALSE;
}

bool_t is_absolute_xpath(xpath_t *xp) {
  char_t delim = '/';
  return xp && xp->path && (xp->path[0] == delim);
}

bool_t is_relative_xpath(xpath_t *xp) {
  return !is_absolute_xpath(xp) && (xp->path[0] != '\0');
}

const char_t *get_last_xpath(xpath_t *xp) {
  const char_t *p;
  if( xp ) {
    p = strrchr(xp->path, '/');
    return p ? (p + 1) : NULL;
  }
  return NULL;
}

const char_t *get_full_xpath(xpath_t *xp) {
  return xp ? xp->path : NULL;
}

bool_t read_first_element_xpath(xpath_t *xp, char_t **begin, char_t **end, char_t **from) {
  if( xp && begin && end && from) {
    *from = xp->path;
    return read_next_element_xpath(xp, begin, end, from);
  }
  return FALSE;
}

bool_t read_next_element_xpath(xpath_t *xp, char_t **begin, char_t **end, char_t **from) {
  char_t delim = '/';
  char_t esc = '\\';
  char_t *start = *from;
  char_t *finish = *from;
  if( xp && begin && end && 
      (start >= xp->path) && (start <= xp->path + xp->pathlen) ) {
    if( *start == '\0' ) { 
      return FALSE; 
    }
    if( (start == xp->path) && (*start == delim) ) { 
      /* absolute path skip initial delim */
      start++;
    }
    finish = (char_t *)find_unescaped_delimiter(start + 1, NULL, delim, esc);
    *begin = start;
    *end = finish;
    *from = (*finish == delim) ? finish + 1 : finish;
/*     puts_stdout("\nread_next_element["); */
/*     write_stdout(start, finish - start); */
/*     puts_stdout("]["); */
/*     puts_stdout(*from); */
/*     puts_stdout("]\n"); */
    return (*end > *begin);
  }
  return FALSE;
}

tagtype_t identify_element_xpath(const char_t *begin, const char_t *end) {
  if( begin >= end ) {
    return empty;
  } 
  if( begin[0] == '.' ) {
    if( end == begin + 1 ) {
      return current;
    } else if( begin[1] == '.' && (end == begin + 2) ) {
      return parent;
    }
  }
  return simpletag;
}

bool_t iterate_xpath(xpath_t *xp, iterator_xpath_fun *f, void *user) {
  char_t *p;
  char_t *q;
  char_t *t;
  if( xp && f ) {
    if( read_first_element_xpath(xp, &p, &q, &t) ) {
      if( !(*f)(xp, 
		( (is_absolute_xpath(xp) && (p - xp->path == 1)) ?
		  root :
		  identify_element_xpath(p, q) ),
		p, q, user) ) {
	return FALSE;
      }
      while( read_next_element_xpath(xp, &p, &q, &t) ) {
	if( !(*f)(xp, identify_element_xpath(p, q), p, q, user) ) {
	  return FALSE;
	}
      } 
      return TRUE;
    }
  }
  return FALSE;
}

bool_t length_fun(xpath_t *xp, tagtype_t t, const char_t *begin, const char_t *end, void *user) {
  size_t *n = (size_t *)user;
  if( n ) {
    *n = *n + 1;
    return TRUE;
  }
  return FALSE;
}

/* there are quicker ways to compute the length, but this also has
   the side effect of checking for iterability */
size_t length_xpath(xpath_t *xp) {
  size_t n = 0;
  if( !iterate_xpath(xp, length_fun, &n) ) {
    return -1;
  }
  return n;
}

bool_t normalize_xpath(xpath_t *xp) {
  char_t *p;
  char_t *q;
  char_t delim = '/';
  char_t *delnode = "/..";
  char_t *simpnode = "/.";
  int op = 0;
#define DELNODE  0x01
#define SIMPNODE 0x02

  if( xp && xp->path ) {
    do {
      op = 0;
      /* delete a node */
      p = strstr(xp->path, delnode);
      if( p ) {
	if( p == xp->path ) {
	  return FALSE;
	}
	q = p + strlen(delnode);
	if( (*q == delim) || (*q == '\0') ) {
	  do {
	    p--;
	  } while( (p > xp->path) && (*p != delim) );
	  if( (p >= xp->path) && (*p == delim) ) {
	    memmove(p, q, strlen(q) + 1);
	  }
	}
	op |= DELNODE;
      }
      /* simplify a node */
      p = strstr(xp->path, simpnode);
      if( p && (p > xp->path) ) {
	q = p + strlen(simpnode);
	if( (*q == delim) || (*q == '\0') ) {
	  memmove(p, q, strlen(q) + 1);
	}
	op |= SIMPNODE;
      }
    } while( op );
    return TRUE;
  }
  return FALSE;
}

/* true if path is a prefix of xp->path 
 * also works if path ends in slash .
 */ 
bool_t cmp_prefix_xpath(xpath_t *xp, const char_t *path) {
  const char_t *p;
  if( xp ) {
    p = xp->path;
    while( *p && (*p == *path) ) { p++; path++; }
    return ((*path == '\0') || ((path[0] == '/') && (path[1] == '\0'))) ;
  }
  return FALSE;
}
