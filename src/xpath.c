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
#include "xpath.h"
#include "entities.h"
#include <string.h>

/* these are externs (see common.c) so that we can override 
   the XPATH syntax globally if we want to */
extern const char_t escc; /* backslash */
extern const char_t xpath_delims[]; /* slash */
extern const char_t xpath_att_names[]; /* at-sign */
extern const char_t xpath_att_values[]; /* equal-sign */
extern const char_t xpath_pis[]; /* question-mark */
extern const char_t xpath_pred_starts[]; /* open square bracket */
extern const char_t xpath_pred_ends[]; /* close square bracket */
extern const char_t xpath_starts[]; /* open square bracket */
extern const char_t xpath_ends[]; /* close square bracket */
extern const char_t xpath_specials[]; /* delimiters with special meanings */
extern const char_t xpath_ptags[]; /* slash dot dot */
extern const char_t xpath_ctags[]; /* slash dot */


bool_t create_xpath(xpath_t *xp) {
  if( xp ) {
    return (NULL != create_cstring(&xp->path, "", 64));
  }
  return FALSE;
}

bool_t free_xpath(xpath_t *xp) {
  if( xp ) {
    free_cstring(&xp->path);
    return TRUE;
  }
  return FALSE;
}


bool_t reset_xpath(xpath_t *xp) {
  if( xp ) {
    truncate_cstring(&xp->path, 0);
    return TRUE;
  }
  return FALSE;
}

bool_t pop_delim_xpath(xpath_t *xp, char_t delim) {
  char_t *p;
  if( xp ) {
    p = (char_t *)
      rskip_unescaped_delimiter(begin_cstring(&xp->path), 
				end_cstring(&xp->path), 
				delim, escc);
    if( p ) { 
      *p = '\0'; 
    }
    return (p != NULL);
  }
  return FALSE;
}

bool_t push_attribute_xpath(xpath_t *xp, const char_t *name) {
  if( xp && name ) {
    return (NULL != vstrcat_cstring(&xp->path, xpath_att_names, name, NULLPTR));
  }
  return FALSE;
}

bool_t push_attributes_values_xpath(xpath_t *xp, const char_t **att) {
  const char_t *p;
  if( xp ) {
    while( att && *att ) {
      p = vstrcat_cstring(&xp->path, 
			  xpath_att_names, att[0], 
			  xpath_att_values, NULLPTR);
      if( p ) {
	p = write_escape_cstring(&xp->path, p, xpath_specials, escc, 
				 att[1], strlen(att[1]));
      }
      if( !p ) { return FALSE; }
      att += 2;
    }
    return TRUE;
  }
  return FALSE;
}

/* pop off @name or /@name patterns but leave / delimiter */
bool_t pop_attribute_xpath(xpath_t *xp) {
  return pop_delim_xpath(xp, *xpath_att_names);
}

bool_t push_tag_xpath(xpath_t *xp, const char_t *tagname) {
  if( xp && tagname ) {
    return (NULL != vstrcat_cstring(&xp->path, xpath_delims, tagname, NULLPTR));
  }
  return FALSE;
}

bool_t push_node_xpath(xpath_t *xp, nodetype_t t) {
  const char_t *label = NULL;
  if( xp ) {
    /* note: we use the fact that digits are not valid tag names in XML */
    switch(t) {
    case n_comment:
      label = "2";
      break;
    case n_dfault:
      label = "0";
      break;
    case n_pidata:
      label = "3";
      break;
    case n_chardata:
      label = "1";
      break;
    case n_none:
    case n_space:
    case n_start_tag:
    case n_end_tag:
    case n_start_cdata:
    case n_end_cdata:
    case n_start_doctype:
    case n_end_doctype:
    case n_entitydecl:
      /* nothing */
      break;
    }
    /* note: a well formed xml tag name cannot start with a digit */
    return label ? push_tag_xpath(xp, label) : FALSE;
  }
  return FALSE;
}

bool_t push_predicate_xpath(xpath_t *xp, const char_t *pred) {
  if( xp && pred ) {
    return (NULL != vstrcat_cstring(&xp->path, 
				    xpath_pred_starts, pred, 
				    xpath_pred_ends, NULLPTR));
  }
  return FALSE;
}

bool_t pop_xpath(xpath_t *xp) {
  return pop_delim_xpath(xp, *xpath_delims);
}

bool_t write_xpath(xpath_t *xp, const char_t *path, size_t pathlen) {
  if( xp && xp->path.cb.buf ) {
    return (NULL != write_cstring(&xp->path, end_cstring(&xp->path), 
				  path, pathlen));
  }
  return FALSE;
}

bool_t append_xpath(xpath_t *xp, const char_t *tag, size_t len) {
  const char_t *p;
  if( xp && tag ) {
    p = end_cstring(&xp->path);
    p = write_cstring(&xp->path, p, xpath_delims, 1);
    p = write_cstring(&xp->path, p, tag, len);
    return (p != NULL);
  }
  return FALSE;
}

bool_t copy_xpath(xpath_t *dest, const xpath_t *src) {
  const char_t *p, *q;
  if( dest && src ) {
    p = begin_cstring(&src->path);
    q = end_cstring(&src->path);
    return reset_xpath(dest) && write_xpath(dest, p, q - p);
  }
  return FALSE;
}

bool_t is_absolute_xpath(const xpath_t *xp) {
  const char_t *p = xp ? begin_cstring(&xp->path) : NULL;
  return p && (*p == *xpath_delims); 
}

bool_t is_relative_xpath(const xpath_t *xp) {
  const char_t *p = xp ? begin_cstring(&xp->path) : NULL;
  return p && *p && (*p != *xpath_delims); 
}

const char_t *get_last_xpath(xpath_t *xp) {
  const char_t *p, *q;
  if( xp ) {
    p = begin_cstring(&xp->path);
    q = end_cstring(&xp->path);
    p = rskip_unescaped_delimiter(p, q, *xpath_delims, escc);
    return p ? (p + 1) : NULL;
  }
  return NULL;
}

const char_t *get_full_xpath(xpath_t *xp) {
  return xp ? p_cstring(&xp->path) : NULL;
}

bool_t read_first_element_xpath(xpath_t *xp, char_t **begin, char_t **end, char_t **from) {
  if( xp && begin && end && from) {
    *from = p_cstring(&xp->path);
    return read_next_element_xpath(xp, begin, end, from);
  }
  return FALSE;
}

/* Compute the begin and end pointers of the next path element, using from
 * as the parser state.
 */ 
bool_t read_next_element_xpath(xpath_t *xp, char_t **begin, char_t **end, char_t **from) {
  char_t *start = *from;
  char_t *finish = *from;
  if( xp && begin && end && check_range_cstring(&xp->path, start) ) {

    if( *start == '\0' ) { 
      return FALSE; 
    }
    if( (start == begin_cstring(&xp->path)) && (*start == *xpath_delims) ) { 
      /* absolute path skip initial delim */
      start++;
    }
    finish = 
      (char_t *)skip_unescaped_delimiters(start + 1, NULL, xpath_delims, escc);

    *begin = start;
    *end = finish;
    *from = (*finish == *xpath_delims) ? finish + 1 : finish;
    /* puts_stdout("\nread_next_element["); */
    /* write_stdout(start, finish - start); */
    /* puts_stdout("]["); */
    /* puts_stdout(*from); */
    /* puts_stdout("]\n"); */
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
		( (is_absolute_xpath(xp) && (p - xp->path.cb.buf == 1)) ?
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

/* pop off each tag and execute a function each time. FALSE stops. */
bool_t close_xpath(xpath_t *xp, iterator_xpath_fun *f, void *user) {
  const char_t *p, *q;
  while( *xp->path.cb.buf ) {
    p = (char_t *)get_last_xpath(xp);
    if( p && f ) {
      q = p + strlen(p);
      if( !(*f)(xp, 
		( (is_absolute_xpath(xp) && (p - xp->path.cb.buf == 1)) ?
		  root :
		  identify_element_xpath(p, q) ),
		p, q, user) ) {
	return FALSE;
      }
    };
    pop_xpath(xp);
  }
  return( *xp->path.cb.buf != '\0' );
}

/* remove embedded . and .. symbols */
bool_t normalize_xpath(xpath_t *xp) {
  char_t *p, *q, *r;
  int op = 0;
#define DELNODE  0x01
#define SIMPNODE 0x02

  if( xp && CSTRINGP(xp->path) ) {
    do {
      op = 0;
      r = p_cstring(&xp->path);
      /* delete a node */
      p = (char_t *)skip_unescaped_substring(r, NULL, xpath_ptags, escc);
      /* p = find_unescaped_string(r, xpath_ctags, escc); */
      if( p && *p ) { 
	if( p == r ) {
	  return FALSE;
	}
	q = p + strlen(xpath_ptags);
	if( (*q == *xpath_delims) || (*q == '\0') ) {
	  do {
	    p--;
	  } while( (p > r) && (*p != *xpath_delims) );
	  if( (p >= r) && (*p == *xpath_delims) ) {
	    memmove(p, q, strlen(q) + 1);
	  }
	}
	op |= DELNODE;
      }
      /* simplify a node */
      p = (char_t *)skip_unescaped_substring(r, NULL, xpath_ctags, escc);
      /* p = find_unescaped_string(r, xpath_ctags, escc); */
      if( p && *p && (p > r) ) {
	q = p + strlen(xpath_ctags);
	if( (*q == *xpath_delims) || (*q == '\0') ) {
	  memmove(p, q, strlen(q) + 1);
	}
	op |= SIMPNODE;
      }
    } while( op );

    return TRUE;
  }
  return FALSE;
}

/* BELOW CODE DOES NOT LEVERAGE CSTRING (MUST FIX) */

/* Replace xp minimally with a path xp' that leads to target
 * according to the equation (xp + xp') = target.
 *
 * Example: (xp = a/b/c, target = a/e/f) -> xp' = ../../e/f
 */
bool_t retarget_xpath(xpath_t *xp, const xpath_t *target) {
  size_t numbytes;
  long l, levels;
  const char_t *p, *q;

  if( xp && target ) {

    /* puts_stdout("relativize_xpath{"); */
    /* puts(xp->path.cb.buf); */
    /* puts_stdout("}{"); */
    /* puts_stdout(target->path.cb.buf); */
    /* puts_stdout("}\n"); */

    if( is_absolute_xpath(target) ) {

      p = begin_cstring(&xp->path);
      q = begin_cstring(&target->path);
      parallel_skip_prefix(&p, &q, *xpath_delims, escc);
      if( *p == *xpath_delims ) { p++; }
      if( *q == *xpath_delims ) { q++; }

      /* printf("{%s}{%s}\n", p, q); */
      if( !*p && !*q ) {

	/* both paths are the same */
	
	reset_xpath(xp);
	push_tag_xpath(xp, ".");

	return TRUE;

      } 

      numbytes = 1;

      if( *p ) { /* must remove some levels */

	levels = total_unescaped_delimiters(p, NULL, xpath_delims, escc);
	numbytes += 3 * levels * sizeof(char_t);
	
	if( !ensure_cstring(&xp->path, numbytes) ) {
	  return FALSE;
	}

	truncate_cstring(&xp->path, 0);

	p = begin_cstring(&xp->path);
	for(l = 0, numbytes = 1; l < levels; l++, numbytes += 3) {
	  p = vputs_cstring(&xp->path, p, "..", xpath_delims, NULLPTR);
	}

      } else {
	truncate_cstring(&xp->path, 0);
      }

      if( *q ) { /* must add some levels */

 	if( *q == *xpath_delims ) { q++; }

	numbytes += strlen(q);

	if( !ensure_cstring(&xp->path, numbytes) ) {
	  return FALSE;
	}

	strcat_cstring(&xp->path, q);

      }
      
      /* final cleanup - remove final slash if present but not escaped */
      l = numbytes - 1;
      if( ((l == 1) && (xp->path.cb.buf[l-1] == *xpath_delims)) ||
	  ((l > 1) && (xp->path.cb.buf[l-1] == *xpath_delims) && (xp->path.cb.buf[l-2] != escc)) ) {
	xp->path.cb.buf[l-1] = '\0';
      }

      return TRUE;
    } 
  }
  return FALSE;
}


/* compares the two paths as strings EXACTLY */
bool_t equal_xpath(const xpath_t *xp1, const xpath_t *xp2) {
  return (xp1 && xp2 && 
	  (strcmp(p_cstring(&xp1->path),
		  p_cstring(&xp2->path)) == 0));
}

/* compares the two strings as paths, but ignoring embedded attributes and
 * ignoring embedded predicates and ignoring embedded processing instructions.
 * returns 0 if the two paths are the same.
 * returns -1 if xp1 is a prefix of xp2, +1 if xp2 is a prefix of xp1.
 * returns 2 otherwise. 
 * COMPARISON DOES NOT WORK WITH WILDCARDS.
 */
int cmp_no_attributes_xpath(const char_t *p1, const char_t *p2) {
  const char_t *p[2], *q[2];
  size_t n[2];
  int i = -2;
  bool_t same, done;

  p[0] = p1;
  p[1] = p2;

  done = !(p1 && p2);
  same = (!done && ( *p[0] == *p[1] ));
  i = -2; /* return value */

  while( same && !done ) {
    for(i = 0; i < 2; i++) {
      p[i]++;
      q[i] = skip_unescaped_delimiters(p[i], NULL, xpath_specials, escc);
      n[i] = q[i] - p[i];
    }
    same = ((n[0] == n[1]) && (memcmp(p[0],p[1],n[0]) == 0));
    i = +2; /* return value */
    if( same ) {
      for(i = 0; i < 2; i++) {
	p[i] = ( (*q[i] == *xpath_delims) ? q[i] : 
		 skip_unescaped_delimiters(q[i], NULL, xpath_delims, escc) );
      }
      same = ((*p[0]) == (*p[1]));
      done = ((*p[0] == '\0') || (*p[1] == '\0'));
      i = (*p[0] == '\0') ? -1 : +1; /* return value */
    }
  } 

  return ( (same && done) ? 0 : i );
}

bool_t equal_no_attributes_xpath(const xpath_t *xp1, const xpath_t *xp2) {
  if( xp1 && xp2 ) {
    return (0 == cmp_no_attributes_xpath(p_cstring(&xp1->path), 
					 p_cstring(&xp2->path)));
  }
  return FALSE;
}

/* skips forward over attributes predicates, processing instructions
 * and optional parentheses. Stops if a slash is encountered, or some
 * other character or the end is reached.
 */
const char_t *skip_attributes_predicates(const char_t *begin, const char_t *end) {
  while( (!end || (begin < end)) && *begin ) {
    if( *begin == *xpath_att_names ) {
      begin = skip_unescaped_delimiters(begin, end, xpath_delims, escc);
    } else if( *begin == *xpath_pred_starts ) {
      begin = skip_unescaped_delimiters(begin, end, xpath_pred_ends, escc);
      if( (end && (begin < end)) || (*begin == *xpath_pred_ends) ) {
	begin++;
      }
    } else {
      break;
    }
  }
  return begin;
}

/* checks if pattern matches (according to the rules below) a
 * string prefix of xpath or vice versa. The rules are:
 * 1) embedded attributes in xpath are ignored
 * 2) * in pattern skips forward a tag name
 * 3) // in pattern skips zero or more tag names
 * 4) If pattern contains a predicate "[pred]" it is skipped.
 *
 * NOTES:
 *    In W3C XPath, a '*' wildcard can only match a tag name,
 *    and a tag cannot contain arbitrary characters, but here we
 *    still allow anything that doesn't start with a digit
 *    for simplicity (this is more flexible, so we can
 *    encode extra information in a path if we want to, eg attributes).
 *    EXCEPTION: We don't allow tags to start with a digit.
 *
 *    Also, with our algorithm, we get for free the ability to
 *    end a partial tag name with a '*' wildcard. This is not in XPath,
 *    but is the natural Unix shell philosophy.
 *    HOWEVER: the algorithm doesn't handle full unix '*' semantics.
 *
 * RETURNS:
 *  -2 if pattern matches a prefix of xpath including a partial tag prefix
 *  -1 if pattern matches a prefix of xpath with each tag name exactly matched
 *   0 if pattern exactly matches xpath
 *  +1 if xpath matches a prefix of pattern 
 *   2 otherwise 
 * Since a pattern can conceivably return more than one value, the priority
 * ordering is 0, -1, +1, -2, 2. For the vast majority of purposes, we are 
 * only interested in the cases 0, -1 anyway. 
 */
int match_no_att_no_pred_xpath(const char_t *patbegin, const char_t *patend,
			       const char_t *xpath) {
  int i, j;
  char_t last = -2; 
  /* printf("match[%.*s, %s ", patend - patbegin, patbegin, xpath); */
  if( patbegin && xpath ) {
    if( !patend ) {
      patend = patbegin + strlen(patbegin);
    }
    while( (patbegin < patend) && *xpath ) {
      xpath = skip_attributes_predicates(xpath, NULL);
      patbegin = skip_attributes_predicates(patbegin, patend);
      /* now match */
      if( !xpath || (patbegin == patend) ) {
	break;
      } else if( (*patbegin == '*') && !xml_isdigit(*xpath) ) {
	last = *patbegin;
	patbegin++;
	xpath = skip_unescaped_delimiters(xpath, NULL, xpath_delims, escc);
      } else if( (patbegin[0] == *xpath_delims) &&
		 (patbegin[1] == *xpath_delims) ) {
	patbegin++;
	j = 2;
	while( *xpath ) {
	  i = match_no_att_no_pred_xpath(patbegin, patend, xpath);
	  j = ((abs(i) < abs(j)) ? i : (i < j) ? i : j);
	  if( i == 0 ) { break; } /* best possible result */
	  xpath = skip_unescaped_delimiters(++xpath, NULL, xpath_delims, escc);
	}
	/* printf(" = %d]\n", j); */
	return j;
      } else if( *patbegin == *xpath ) {
	last = *patbegin;
	patbegin++;
	xpath++;
      } else {
	break;
      }
    }
    xpath = skip_attributes_predicates(xpath, NULL);
    patbegin = skip_attributes_predicates(patbegin, patend);

    j = ((last == '*') || (*xpath == *xpath_delims) || (last == *xpath_delims)) ? -1 : -2;
    i = (patbegin == patend) ? (*xpath ? j : 0) : (*xpath ? 2 : +1) ;
    /* printf(" = %d %d {%s,%s}]\n", i, j, patbegin,xpath); */
    return i;
  }
  return 2;
}

