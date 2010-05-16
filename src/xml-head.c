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
#include "parser.h"
#include "io.h"
#include "myerror.h"
#include "wrap.h"
#include "stdout.h"
#include "stdparse.h"
#include "entities.h"
#include "stdprint.h"
#include "mem.h"
#include "mysignal.h"

#include <string.h>
#include <getopt.h>

/* for option processing */
extern char *optarg;
extern int optind, opterr, optopt;

extern char *progname;
extern char *inputfile;
extern long inputline;

extern volatile flag_t cmd;

#include <stdio.h>

typedef struct {
  stdparserinfo_t std; /* must be first so we can cast correctly */
  flag_t flags;
  long int maxc, maxl, maxt;
  long int c, l, t;
} parserinfo_head_t;

#define HEAD_VERSION    0x01
#define HEAD_HELP       0x02
#define HEAD_CHARS      0x03
#define HEAD_LINES      0x04
#define HEAD_TAGS       0x05
#define HEAD_USAGE \
"Usage: xml-head [OPTION]... [FILE]\n" \
"Reformat each node in FILE(s), or standard input.\n" \
"\n" \
"      --help     display this help and exit\n" \
"      --version  display version information and exit\n"

#define HEAD_FLAG_CHARS 0x01
#define HEAD_FLAG_LINES 0x02
#define HEAD_FLAG_TAGS  0x04

void set_option_head(int op, char *optarg, parserinfo_head_t *pinfo) {
  switch(op) {
  case HEAD_VERSION:
    puts("xml-head" COPYBLURB);
    exit(EXIT_SUCCESS);
    break;
  case HEAD_HELP:
    puts(HEAD_USAGE);
    exit(EXIT_SUCCESS);
    break;
  case 'c':
  case HEAD_CHARS:
    setflag(&pinfo->flags, HEAD_FLAG_CHARS);
    /* note: people count from 1, computers from 0 */
    pinfo->maxc = atol(optarg); 
    break;
  case 'n':
  case HEAD_LINES:
    setflag(&pinfo->flags, HEAD_FLAG_CHARS);
    pinfo->maxl = atol(optarg);
    break;
  case 't':
  case HEAD_TAGS:
    setflag(&pinfo->flags, HEAD_FLAG_TAGS);
    pinfo->maxt = atol(optarg) + 1;
    break;
  default:
    break;
  }
}

long int set_obsolete_option(char *firstarg) {
  long int n = -1;
  if( firstarg && (*firstarg == '-') ) {
    firstarg++;
    if( *firstarg ) {
      n = atol(firstarg) + 1;
      while( *firstarg && xml_isdigit(*firstarg) ) {
	firstarg++;
      }
      if( *firstarg != '\0' ) {
	n = -1;
      }
    }
  }
  return n;
}

result_t start_tag(void *user, const char_t *name, const char_t **att) {
  parserinfo_head_t *pinfo = (parserinfo_head_t *)user;
  if( pinfo ) { 
    if( pinfo->std.depth > 0 ) {
      pinfo->c = 0;
      pinfo->l = 0;
      pinfo->t++;
    }

    if( checkflag(pinfo->flags,HEAD_FLAG_TAGS) ) {
      if( (pinfo->maxt > -1) && (pinfo->t > pinfo->maxt) ) {
	return PARSER_OK;
      }
    }

    write_start_tag_stdout(name, att, FALSE);
  }
  return PARSER_OK;
}

bool_t unwind_path(stdparserinfo_t *sp) {
  xpath_t path;
  const char_t *name;
  if( sp ) {
    if( create_xpath(&path) ) {
      copy_xpath(&path, &sp->cp);
      do {
	name = get_last_xpath(&path);
	if( name && *name ) {
	  write_end_tag_stdout(name);
	  putc_stdout('\n');
	}
      } while( pop_xpath(&path) );
    }
    return TRUE;
  }
  return FALSE;
}

result_t end_tag(void *user, const char_t *name) {
  parserinfo_head_t *pinfo = (parserinfo_head_t *)user;
  if( pinfo ) { 
    pinfo->c = 0;
    pinfo->l = 0;
    pinfo->t++;

    if( checkflag(pinfo->flags,HEAD_FLAG_TAGS) ) {
      if( (pinfo->maxt > -1) && (pinfo->t > pinfo->maxt) ) {
	unwind_path(&pinfo->std);
	return PARSER_ABORT;
      }
    }

    write_end_tag_stdout(name);
  }
  return PARSER_OK;
}

/* print the data in the buffer as follows:
 * each line is at most maxc characters long
 * maxl is the maximum number of lines printed.
 */
bool_t print_chardata(const char_t *buf, size_t buflen,
		      long int *c, long int maxc,
		      long int *l, long int maxl) {
  const char_t *start, *end, *nl;
  long int n;
  if( buf && (buflen > 0) ) {
    start = buf;
    end = buf + buflen;
    while( (start < end) && ((maxl < 0) || (*l < maxl)) ) {
      nl = skip_delimiter(start, end, '\n');
      if( (maxc < 0) || (*c < maxc) ) {
	n = (maxc < 0) ? (nl - start) : MIN(nl - start, maxc - *c);
	if( n > 0 ) {
	  *c = *c + n;
	  write_coded_entities_stdout(start, n);
	}
      }
      if( nl < end ) {
	*l = *l + 1;
	*c = 0;
	putc_stdout('\n');
      }
      start = nl + 1;
    }
    return TRUE;
  }
  return FALSE;
}

result_t chardata(void *user, const char_t *buf, size_t buflen) {
  parserinfo_head_t *pinfo = (parserinfo_head_t *)user;
  if( pinfo ) { 
    if( pinfo->std.depth == 0 ) {
	write_coded_entities_stdout(buf, buflen);
    } else {
      if( checkflag(pinfo->flags,HEAD_FLAG_CHARS|HEAD_FLAG_LINES) ) {
	print_chardata(buf, buflen, 
		       &pinfo->c, pinfo->maxc, &pinfo->l, pinfo->maxl);
      } else {
	write_coded_entities_stdout(buf, buflen);
      }
    }
  }
  return PARSER_OK;
}

result_t dfault(void *user, const char_t *data, size_t buflen) {
  parserinfo_head_t *pinfo = (parserinfo_head_t *)user;
  if( pinfo ) { 
    stdprint_dfault(&pinfo->std, data, buflen);
  }
  return PARSER_OK;
}

bool_t create_parserinfo_head(parserinfo_head_t *pinfo) {
  bool_t ok = TRUE;
  if( pinfo ) {

    memset(pinfo, 0, sizeof(parserinfo_head_t));
    ok &= create_stdparserinfo(&pinfo->std);

    /* must fix onefile limitation at some point ?? */
    pinfo->std.setup.flags = STDPARSE_ALLNODES|STDPARSE_EQ1FILE;
    pinfo->std.setup.cb.start_tag = start_tag;
    pinfo->std.setup.cb.end_tag = end_tag;
    pinfo->std.setup.cb.chardata = chardata;
    pinfo->std.setup.cb.dfault = dfault;
    pinfo->flags = 0;

    pinfo->c = 0;
    pinfo->l = 0;
    pinfo->t = 0;

    pinfo->maxc = -1;
    pinfo->maxl = -1;
    pinfo->maxt = -1;

    return ok;
  }
  return FALSE;
} 

bool_t free_parserinfo_head(parserinfo_head_t *pinfo) {
  free_stdparserinfo(&pinfo->std);
  return TRUE;
}

int main(int argc, char **argv) {
  signed char op;
  parserinfo_head_t pinfo;

  struct option longopts[] = {
    { "version", 0, NULL, HEAD_VERSION },
    { "help", 0, NULL, HEAD_HELP },
    { "chars", 0, NULL, HEAD_CHARS },
    { "tags", 0, NULL, HEAD_TAGS },
    { 0 }
  };

  progname = "xml-head";
  inputfile = "";
  inputline = 0;

  if( create_parserinfo_head(&pinfo) ) {

    pinfo.maxt = set_obsolete_option(argv[1]);
    if( pinfo.maxt >= 0 ) {
      setflag(&pinfo.flags, HEAD_FLAG_TAGS);
      argv[1] = "-d"; /* dummy */
    }

    while( (op = getopt_long(argc, argv, "c:n:t:d",
			     longopts, NULL)) > -1 ) {
      set_option_head(op, optarg, &pinfo);
    }

    if( !checkflag(pinfo.flags, 
		   HEAD_FLAG_TAGS|HEAD_FLAG_LINES|HEAD_FLAG_CHARS) ) {
      setflag(&pinfo.flags, HEAD_FLAG_TAGS);
      pinfo.maxt = 10;
    }

    init_signal_handling(SIGNALS_DEFAULT);
    init_file_handling();

    open_stdout();
    stdparse(MAXFILES, argv + optind, (stdparserinfo_t *)&pinfo);
    close_stdout();

    exit_file_handling();
    exit_signal_handling();

    free_parserinfo_head(&pinfo);
  }

  return EXIT_SUCCESS;
}
