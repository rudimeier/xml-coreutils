/* 
 * Copyright (C) 2009 Laird Breyer
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
#include "echo.h"
#include "mem.h"
#include "stdout.h"
#include "entities.h"
#include "format.h"
#include "myerror.h"
#include "wrap.h"

#include <string.h>

extern const char_t escc;
extern const char_t xpath_starts[];
extern const char_t xpath_ends[];
extern const char_t xpath_pis[];
extern const char_t xpath_att_names[];
extern const char_t xpath_att_values[];
extern const char_t xpath_tag_delims[];

bool_t create_echo(echo_t *e, size_t indentdepth, flag_t flags) {
  bool_t ok = TRUE;
  if( e ) {
    memset(e, 0, sizeof(echo_t));

    e->flags = flags;
    e->depth = 0;
    e->indentdepth = indentdepth; /* ECHO_INDENTALL; */

    ok &= create_xpath(&e->xpath);
    ok &= create_xpath(&e->tmpath);

    return ok; 
  }
  return FALSE;
}

bool_t free_echo(echo_t *e) {
  if( e ) {
    free_xpath(&e->xpath);
    free_xpath(&e->tmpath);
    free_cstring(&e->root);
    return TRUE;
  }
  return FALSE;
}

bool_t write_indent_stdout(echo_t *e) {
  if( e && (e->indentdepth > e->depth) ) {
    if( !true_and_clearflag(&e->flags,ECHO_FLAG_SUPPRESSNL) ) {
      putc_stdout('\n');
    }
    nputc_stdout('\t', e->depth);
    return TRUE;
  }
  return FALSE;
} 

bool_t write_start_cdata_stdout(echo_t *e) {
  if( e ) {
    if( false_and_setflag(&e->flags,ECHO_FLAG_CDATA) ) {
      puts_stdout("<![CDATA[");
      return TRUE;
    }
  }
  return FALSE;
}

bool_t write_end_cdata_stdout(echo_t *e) {
  if( e ) {
    if( true_and_clearflag(&e->flags,ECHO_FLAG_CDATA) ) {
      puts_stdout("]]>");
      return TRUE;
    }
  }
  return FALSE;
}

bool_t write_start_comment_stdout(echo_t *e) {
  if( e ) {
    if( false_and_setflag(&e->flags,ECHO_FLAG_COMMENT) ) {
      puts_stdout("<!-- ");
      return TRUE;
    }
  }
  return FALSE;
}

bool_t write_end_comment_stdout(echo_t *e) {
  if( e ) {
    if( true_and_clearflag(&e->flags,ECHO_FLAG_COMMENT) ) {
      puts_stdout(" -->");
      return TRUE;
    }
  }
  return FALSE;
}

bool_t write_current_chardata(echo_t *e, const char_t *begin, const char_t *end) {
  const char_t *p;
  if( begin && end && (begin < end) ) {

    if( *begin == escc ) {

      switch(begin[1]) {
      case 'b':
	backspace_stdout();
	break;
      case 'c':
	write_start_comment_stdout(e);
	break;
      case 'C':
	write_end_comment_stdout(e);
	break;
      case 'I':
	e->indentdepth = e->depth;
	break;
      case 'i':
	e->indentdepth = ECHO_INDENTALL;
	break;
      case 'n':
	putc_stdout('\n');
	write_indent_stdout(e);
	break;
      case 'Q':
	write_start_cdata_stdout(e);
	break;
      case 'q':
	write_end_cdata_stdout(e);
	break;
      default:
	putc_stdout(convert_backslash(begin));
	break;
      }
      begin += 2;

    } else {

      if( !CSTRINGP(e->root) ) {
	open_stdout_echo(e, get_root_tag());
      }

      write_indent_stdout(e);
      p = skip_delimiter(begin, end, escc);
      p = p ? p : end;

      if( checkflag(e->flags,ECHO_FLAG_CDATA|ECHO_FLAG_COMMENT) ) {
	write_stdout((byte_t *)begin, p - begin);
      } else {
	write_coded_entities_stdout(begin, p - begin);
      }
      begin = p;
      
    }
    return write_current_chardata(e, begin, end);

  }
  return FALSE;
}

const char_t *write_single_attribute(const char_t *begin, const char_t *end) {
  const char_t *p, *e;
  if( begin && (begin < end) ) {
    p = skip_unescaped_delimiters(begin, end, xpath_tag_delims, escc);
    e = skip_unescaped_delimiters(begin, p, xpath_att_values, escc);
    if( e >= p ) {
      *((char_t *)end) = '\0';
      errormsg(E_FATAL, "cannot parse attribute %s\n", begin);
    }
    e++;
    putc_stdout(' ');
    write_stdout((byte_t *)begin, e - begin);

    putc_stdout('"');
    write_unescaped_stdout(e, p - e);
    putc_stdout('"');

    return p;
  }
  return NULL;
}

bool_t write_pi_content(const char_t *begin, const char_t *end) {
  const char_t *p;
  if( begin && (begin < end) ) {

    p = skip_unescaped_delimiters(begin, end, xpath_tag_delims, escc);
    write_stdout((byte_t *)begin, p - begin);      

    while( p < end ) {
      if( *p == *xpath_att_names ) {
	p = write_single_attribute(p + 1, end);
      } else {
	write_stdout((byte_t *)begin, p - begin);      
      }
      begin = p + 1;
    }

    return TRUE;
  }
  return FALSE;
}

bool_t write_single_tag(echo_t *cur, const char_t *begin, const char_t *end, bool_t start) {
  const char_t *q;
  if( begin && (end >= begin) ) {

    if( (end - begin == 1) && (*begin == '.') ) {
      /* path is normalized to nothing */
      return TRUE; 
    }

    /* printf("\nwrite_single_tag(%d)(%s)(%s)\n", end - begin, begin, end); */
    q = skip_unescaped_delimiters(begin, end, xpath_tag_delims, escc);
    if( q > begin ) {

      write_indent_stdout(cur);
      putc_stdout('<');
      if( !start ) { 
	putc_stdout('/'); 
      }

      write_stdout((byte_t *)begin, q - begin);

      if( start ) {
	while( q && (q < end) ) {

	  if( *q == *xpath_att_names ) {
	    write_single_attribute(q + 1, end);
	  }

	  q++;
	  q = skip_unescaped_delimiters(q, end, xpath_tag_delims, escc);
	}
      }
      putc_stdout('>');

    }

    return TRUE;
  }
  return FALSE;
}

bool_t write_tags_absolute(xpath_t *xp, tagtype_t t, const char_t *begin, const char_t *end, void *user) {
  echo_t *e = (echo_t *)user;
  switch(t) {
  case empty:
  case current:
  case parent:
    /* bad should not happen */
    return FALSE;
  case root:
    if( !CSTRINGP(e->root) ) { 
      create_cstring(&e->root, begin, end - begin);
      write_single_tag(e, begin, end, TRUE);
    }
    e->depth++;
    /* don't display this one, but check it's what we expect */
    return (strncmp(p_cstring(&e->root), begin, end - begin) == 0);
  case simpletag:
  case complextag:
    write_single_tag(e, begin, end, TRUE);
    e->depth++;
    break;
  }
  return TRUE;
}

bool_t write_tags_relative(xpath_t *xp, tagtype_t t, const char_t *begin, const char_t *end, void *user) {
  const char_t *last;
  echo_t *e = (echo_t *)user;
  if( e ) {
    switch(t) {
    case empty:
      /* bad should not happen */
      return FALSE;
    case current:
      if( length_xpath(&e->xpath) > 1 ) {
	last = get_last_xpath(&e->xpath);
	if( !last ) {
	  return FALSE;
	}
	e->depth--;
	write_single_tag(e, last, last + strlen(last), FALSE);
	write_single_tag(e, last, last + strlen(last), TRUE);
	e->depth++;
      }
      break;
    case parent:
      last = get_last_xpath(&e->xpath);
      if( !last || !pop_xpath(&e->xpath) ) {
	/* bad, very bad */
	return FALSE;
      }
      if( length_xpath(&e->xpath) == 0 ) {
	/* very bad, user didn't count correct depth */
	return FALSE;
      }
      e->depth--;
      write_single_tag(e, last, last + strlen(last), FALSE);
      break;
    case root:
    case simpletag:
    case complextag:
      if( !CSTRINGP(e->root) ) { 
	create_cstring(&e->root, begin, end - begin);
      }
      write_single_tag(e, begin, end, TRUE);
      append_xpath(&e->xpath, begin, end - begin);
      e->depth++;
      break;
    }
    return TRUE;
  }
  return FALSE;
}

bool_t open_abspath_stdout_echo(echo_t *e, xpath_t *fullpath) {
  return ( copy_xpath(&e->xpath, fullpath) && 
	   normalize_xpath(&e->xpath) && 
	   ((e->depth = 0),
	    iterate_xpath(&e->xpath, write_tags_absolute, e)) );
}

bool_t open_relpath_stdout_echo(echo_t *e, xpath_t *relpath) {
  return ( iterate_xpath(relpath, write_tags_relative, e) &&
	   normalize_xpath(&e->xpath) );
}

/* close all but the root tag in reverse order */
bool_t close_path_stdout_echo(echo_t *e) {
  char_t *t;
  if( e ) {
    write_end_cdata_stdout(e);
    e->depth = length_xpath(&e->xpath); /* d = -1 if bad path */
    while( e->depth > 1 ) { 
      t = (char_t *)get_last_xpath(&e->xpath);
      e->depth--;
      if( !( t && 
	     write_single_tag(e, t, t + strlen(t), FALSE) && 
	     pop_xpath(&e->xpath) ) ) {
	return FALSE;
      }
    }
    return ( (e->depth > 0) || !CSTRINGP(e->root) );
  }
  return FALSE;
}

bool_t write_current_pathdata(echo_t *e, const char_t *begin, const char_t *end) {
  xpath_t *path = &e->tmpath;
  if( begin && end && (begin < end) ) {
    reset_xpath(path);
    write_xpath(path, begin, end - begin);
    write_end_cdata_stdout(e); /* only prints if ECHO_FLAG_CDATA is set */
    if( is_absolute_xpath(path) ) {
      return ( close_path_stdout_echo(e) &&
	       open_abspath_stdout_echo(e, path) );
    } else if( is_relative_xpath(path) ) {
      return ( open_relpath_stdout_echo(e, path) );
    }
  }
  return FALSE;
}

bool_t write_current_pidata(echo_t *e, const char_t *begin, const char_t *end) {
  const char_t *p, *q;
  if( begin && (begin < end) ) {
    p = skip_unescaped_delimiters(begin, end, " \t\r\n@", escc);

    write_indent_stdout(e);
    puts_stdout("<?");

    write_stdout((byte_t *)begin, p - begin);
    putc_stdout(' ');

    if( p < end ) {
      p++;
      q = skip_unescaped_delimiters(p, end, xpath_ends, escc);
      write_pi_content(p, q);
      p = q + 1;
    }

    puts_stdout("?>");
    return TRUE;
  }
  return FALSE;
}

/* Print a single echo string. The structural elements
 * must be complete, ie you cannot print a fragment only.
 *
 * If the root tag e->root is NULL, it will be set as follows:
 * If s starts with chardata, then e->root = get_root_tag(),
 * otherwise if s starts with an absolute or a relative path,
 * then this path defines e->root implicitly.
 *
 * The root tag cannot be changed once set, as an XML document
 * cannot have more than one root tag.
 */
void puts_stdout_echo(echo_t *e, const char_t *s) {
  const char_t *p;
  const char_t *q;
  if( s ) {
    p = s;
    while( *p ) {
      q = skip_unescaped_delimiters(p, NULL, xpath_starts, escc);
      write_current_chardata(e, p, q);
      if( *q == '\0' ) {
	break;
      } 
      p = q;
      q = skip_unescaped_delimiters(p, NULL, xpath_ends, escc);
      if( checkflag(e->flags,ECHO_FLAG_COMMENT) ) {
	write_current_chardata(e, p, q + 1);
      } else if( p[1] == *xpath_pis ) {
	if( !write_current_pidata(e, p+2, q) ) {
	  flush_stdout();
	  *((char_t *)q) = '\0';
	  errormsg(E_FATAL, "bad processing instruction %s\n", p+1);
	}
      } else if( !write_current_pathdata(e, p+1, q) ) {
	flush_stdout();
	*((char_t *)q) = '\0';
	errormsg(E_FATAL, "bad path %s\n", p+1);
      }
      if( *q == '\0' ) {
	break;
      }
      p = q;
      p++;
    }
  }
}


/* set the root tag and print it */ 
bool_t open_stdout_echo(echo_t *e, const char_t *root) {
  if( e ) {
    setflag(&e->flags, ECHO_FLAG_SUPPRESSNL);
    if( !CSTRINGP(e->root) ) {
      strdup_cstring(&e->root, root);
    }
    if( CSTRINGP(e->root) ) {
      push_tag_xpath(&e->xpath, p_cstring(&e->root));
      write_single_tag(e, begin_cstring(&e->root), end_cstring(&e->root), TRUE);
      e->depth++;
    }
    return TRUE;
  }
  return FALSE;
}

bool_t close_stdout_echo(echo_t *e) {
  if( e ) {
    write_end_cdata_stdout(e);
    write_end_comment_stdout(e);

    close_path_stdout_echo(e);
    e->depth--;

    if( CSTRINGP(e->root) ) {
      write_single_tag(e, begin_cstring(&e->root), end_cstring(&e->root), FALSE);
      pop_xpath(&e->xpath);
    }
    flush_stdout();
    return TRUE;
  }
  return FALSE;
}

