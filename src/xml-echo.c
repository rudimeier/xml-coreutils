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
#include "io.h"
#include "wrap.h"
#include "error.h"
#include "xpath.h"
#include "mem.h"
#include "entities.h"
#include "format.h"
#include "stdout.h"

#include <string.h>
#include <getopt.h>

/* for option processing */
extern char *optarg;
extern int optind, opterr, optopt;

extern char *progname;
extern char *inputfile;
extern long inputline;

extern int cmd;

#define U_OPT_ECHO_INTERPRET 0x01
flag_t u_options = 0;

#include <stdio.h>

#define INDENTALL   32767
typedef struct {
  xpath_t xpath;
  size_t depth;
  size_t indentdepth;
  bool_t reindent;
} echo_info_t;

#define ECHO_VERSION    0x01
#define ECHO_HELP       0x02
#define ECHO_USAGE0 \
"Usage: xml-echo [OPTION]... [STRING]...\n" \
"Echo an XML document to standard output.\n" \
"\n" \
"  -e            enable interpretation of control information\n" \
"      --help    display this help and exit\n" \
"      --version display version information and exit\n"

#define ECHO_USAGE1 \
"If -e is in effect, the following escape sequences are recognized:\n" \
"  \\\\   backslash\n" \
"  \\n   new line\n" \
"  \\t   tab\n" \
"  \\i   hierarchically indent following (default)\n" \
"  \\c   linearly output following\n" \
"  In addition, STRING can contain \"[PATH]\", where PATH is an absolute\n" \
"  or relative node path. This has the effect of outputting the corresponding\n" \
"  XML tags.\n"

int set_option(int op, char *optarg) {
  int c = 0;
  switch(op) {
  case 'e':
    setflag(&u_options,U_OPT_ECHO_INTERPRET);
    break;
  case ECHO_VERSION:
    puts("xml-echo" COPYBLURB);
    exit(EXIT_SUCCESS);
    break;
  case ECHO_HELP:
    puts(ECHO_USAGE0);
    puts(ECHO_USAGE1);
    exit(EXIT_SUCCESS);
    break;
  }
  return c;
}

bool_t write_encoded_stdout(const char_t *begin, const char_t *end) {
  const char_t *p;
  if( begin && end && (begin < end) ) {
    do {
      p = find_next_special(begin, end);
      if( p && (p < end) ) {
	write_stdout((byte_t *)begin, p - begin);
	puts_stdout(get_entity(*p));
	begin = p + 1;
      } else {
	write_stdout((byte_t *)begin, end - begin);
	begin = end;
      }
    } while( begin < end ); 
    return TRUE;
  }
  return FALSE;
}


void reindent_if_pending(echo_info_t *cur) {
  if( cur && cur->reindent ) {
    cur->indentdepth = INDENTALL;
    cur->reindent = FALSE;
  }
}

bool_t write_indent_stdout(echo_info_t *cur) {
  if( cur->indentdepth > cur->depth ) {
    putc_stdout('\n');
    nputc_stdout(' ', cur->depth - 1);
    return TRUE;
  }
  return FALSE;
} 


bool_t write_current_chardata(echo_info_t *cur, const char_t *begin, const char_t *end) {
  const char_t *p;
  if( begin && end && (begin < end) ) {
    write_indent_stdout(cur);
    do {
      p = find_delimiter(begin, end, '\\');
      if( p && (p < end) ) {

	reindent_if_pending(cur);

	write_encoded_stdout(begin, p);
	switch(p[1]) {
	case 'c':
	  cur->indentdepth = cur->depth;
	  break;
	case 'i':
	  cur->reindent = TRUE;
	  break;
	case 'n':
	  write_indent_stdout(cur) || putc_stdout('\n');
	  break;
	default:
	  putc_stdout(convert_backslash(p));
	  break;
	}
	begin = p + 2;
      } else {
	write_encoded_stdout(begin, end);
	begin = end;
      }
    } while( begin < end ); 
    return TRUE;
  }
  return FALSE;
}

bool_t write_single_tag(const char_t *begin, const char_t *end, bool_t start, echo_info_t *cur) {
  const char_t *p;
  const char_t *q;
  const char_t *e;
  if( begin && (end >= begin) ) {

    if( start ) {
      reindent_if_pending(cur);
    }
    write_indent_stdout(cur);

    q = find_unescaped_delimiter(begin, end, '@', '\\');

    putc_stdout('<');
    if( !start ) { 
      putc_stdout('/'); 
    }
    write_stdout((byte_t *)begin, q - begin);
    if( start ) {
      while( q < end ) {
	q++;
	p = find_unescaped_delimiter(q, end, '@', '\\');
	if( p ) {
	  e = find_unescaped_delimiter(q, p, '=', '\\');
	  if( e >= p ) {
	    *((char *)end) = '\0';
	    errormsg(E_FATAL, "cannot parse tag %s\n", begin);
	  }
	  e++;
	  putc_stdout(' ');
	  write_stdout((byte_t *)q, e - q);
	  putc_stdout('"');
	  write_stdout((byte_t *)e, p - e);
	  putc_stdout('"');
	}
	q = find_unescaped_delimiter(q, end, '@', '\\');
      }
    }
    putc_stdout('>');

    if( !start ) {
      reindent_if_pending(cur);
    }

    return TRUE;
  }
  return FALSE;
}

bool_t write_tags_absolute(xpath_t *xp, tagtype_t t, const char_t *begin, const char_t *end, void *user) {
  echo_info_t *cur = (echo_info_t *)user;
  switch(t) {
  case empty:
  case current:
  case parent:
    /* bad should not happen */
    return FALSE;
  case root:
    /* don't display this one, but check it's what we expect */
    return (strncmp(get_root_tag(), begin, end - begin) == 0);
  case simpletag:
  case complextag:
    write_single_tag(begin, end, TRUE, cur);
    cur->depth++;
    break;
  }
  return TRUE;
}

bool_t write_tags_relative(xpath_t *xp, tagtype_t t, const char_t *begin, const char_t *end, void *user) {
  const char_t *last;
  echo_info_t *cur = (echo_info_t *)user;
  if( cur ) {
    switch(t) {
    case empty:
      /* bad should not happen */
      return FALSE;
    case current:
      if( length_xpath(&cur->xpath) > 1 ) {
	last = get_last_xpath(&cur->xpath);
	if( !last ) {
	  return FALSE;
	}
	cur->depth--;
	write_single_tag(last, last + strlen(last), FALSE, cur);
	write_single_tag(last, last + strlen(last), TRUE, cur);
	cur->depth++;
      }
      break;
    case parent:
      last = get_last_xpath(&cur->xpath);
      if( !last || !pop_xpath(&cur->xpath) ) {
	/* bad, very bad */
	return FALSE;
      }
      if( length_xpath(&cur->xpath) == 0 ) {
	/* very bad, user didn't count correct depth */
	return FALSE;
      }
      cur->depth--;
      write_single_tag(last, last + strlen(last), FALSE, cur);
      break;
    case root:
    case simpletag:
    case complextag:
      write_single_tag(begin, end, TRUE, cur);
      append_xpath(&cur->xpath, begin, end - begin);
      cur->depth++;
      break;
    }
    return TRUE;
  }
  return FALSE;
}

bool_t open_full_path_stdout(echo_info_t *cur, xpath_t *fullpath) {
  return ( copy_xpath(&cur->xpath, fullpath) && 
	   (cur->depth = 1, normalize_xpath(&cur->xpath)) &&
	   iterate_xpath(&cur->xpath, write_tags_absolute, cur) );
}

bool_t open_relative_path_stdout(echo_info_t *cur, xpath_t *relpath) {
  return ( iterate_xpath(relpath, write_tags_relative, cur) &&
	   normalize_xpath(&cur->xpath) );
}

/* close all but the root tag in reverse order */
bool_t close_path_stdout(echo_info_t *cur) {
  char_t *t;
  if( cur ) {
    cur->depth = length_xpath(&cur->xpath); /* d = -1 if bad path */
    while( cur->depth > 1 ) {
      t = (char_t *)get_last_xpath(&cur->xpath);
      cur->depth--;
      if( !( t && 
	     write_single_tag(t, t + strlen(t), FALSE, cur) && 
	     pop_xpath(&cur->xpath) ) ) {
	return FALSE;
      }
    }
    return (cur->depth > 0);
  }
  return FALSE;
}


bool_t write_current_pathdata(echo_info_t *cur, const char_t *begin, const char_t *end) {
  xpath_t path;
  if( begin && end && (begin < end) ) {
    create_xpath(&path);
    write_xpath(&path, begin, end - begin);
    if( is_absolute_xpath(&path) ) {
      return ( close_path_stdout(cur) &&
	       open_full_path_stdout(cur, &path) );
    } else if( is_relative_xpath(&path) ) {
      return ( open_relative_path_stdout(cur, &path) );
    }
  }
  return FALSE;
}


void echo_interpreted(echo_info_t *cur, const char_t *s) {
  const char_t *p;
  const char_t *q;
  char_t start = '[';
  char_t stop = ']';
  char_t esc = '\\';

  if( s ) {
    p = s;
    while( p ) {
      q = find_unescaped_delimiter(p, NULL, start, esc);
      write_current_chardata(cur, p, q);
      if( *q == '\0' ) {
	break;
      } 
      p = q;
      q = find_unescaped_delimiter(p, NULL, stop, esc);
      if( !write_current_pathdata(cur, p+1, q) ) {
	flush_stdout();
	*((char_t *)q) = '\0';
	errormsg(E_FATAL, "bad path %s\n", p+1);
      }
      if( *q == '\0' ) {
	break;
      }
      p = q + 1;
    }
  }
}

void echo_simple(const char_t *s) {
  if( s && *s ) {
    puts_stdout(s);
    putc_stdout('\n');
  }
}

int main(int argc, char **argv) {
  signed char op;
  echo_info_t cur;
  int exit_value = EXIT_FAILURE;
  struct option longopts[] = {
    { "version", 0, NULL, ECHO_VERSION },
    { "help", 0, NULL, ECHO_HELP },
  };

  progname = "xml-echo";
  inputfile = "";
  inputline = 0;
  

  while( (op = getopt_long(argc, argv, "e",
			   longopts, NULL)) > -1 ) {
    set_option(op, optarg);
  }

  if( create_xpath(&cur.xpath) ) {
    push_tag_xpath(&cur.xpath, get_root_tag());
    cur.depth = 1;
    cur.indentdepth = INDENTALL;
    cur.reindent = FALSE;

    open_stdout();
    /* this runs an XML parser on anything we output, to 
       make sure it is well formed XML. This is simpler
       than the alternative, which consists of checking
       all input before we write it to STDOUT, e.g. checking
       that tag names follow the XML grammar, that chardata 
       doesn't contain prohitibed chars etc. */
    setup_stdout(STDOUT_CHECKPARSER);

    puts_stdout(get_headwrap());
    if( !checkflag(u_options,U_OPT_ECHO_INTERPRET) ) {
      putc_stdout('\n');
    }

    if( (optind < 0) || !argv[optind] ) {
      echo_simple("");
    } else {
      while(!checkflag(cmd,CMD_QUIT) && argv[optind] ) {
	if( checkflag(u_options,U_OPT_ECHO_INTERPRET) ) {
	  echo_interpreted(&cur, argv[optind]);
	} else {
	  echo_simple(argv[optind]);
	}
	optind++;
      }

      if( checkflag(u_options,U_OPT_ECHO_INTERPRET) ) {
	close_path_stdout(&cur);
      }
    }

    if( checkflag(u_options,U_OPT_ECHO_INTERPRET) ) {
      putc_stdout('\n');
    }
    puts_stdout(get_footwrap());

    close_stdout();

  }
  return exit_value;
}
