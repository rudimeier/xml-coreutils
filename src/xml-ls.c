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
#include "parser.h"
#include "io.h"
#include "mem.h"
#include "entities.h"
#include "myerror.h"
#include "wrap.h"
#include "stdout.h"
#include "stdparse.h"
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
  unsigned int indent;
  bool_t contin;
  unsigned int npos;
  unsigned int pd;
  flag_t flags;
} parserinfo_ls_t;

#define LS_VERSION     0x01
#define LS_HELP        0x02
#define LS_ATTRIBUTES  0x03
#define LS_USAGE \
"Usage: xml-ls [OPTION]... [[FILE]... [:XPATH]...]...\n" \
"List structural information about the FILE(s), or standard input.\n" \
"\n" \
"      --help     display this help and exit\n" \
"      --version  display version information and exit\n"

#define LS_FLAG_ATTRIBUTES 0x01

void set_option_ls(int op, char *optarg, parserinfo_ls_t *pinfo) {
  switch(op) {
  case LS_VERSION:
    puts("xml-ls" COPYBLURB);
    exit(EXIT_SUCCESS);
    break;
  case LS_HELP:
    puts(LS_USAGE);
    exit(EXIT_SUCCESS);
    break;
  case 'a':
  case LS_ATTRIBUTES:
    setflag(&pinfo->flags,LS_FLAG_ATTRIBUTES);
    break;
  }
}

void write_truncate_stdout(int max, const char_t *buf, size_t buflen) {
  if( buf ) {
    if( buflen < max ) {
      squeeze_coded_entities_stdout(buf, buflen);
    } else {
      squeeze_coded_entities_stdout(buf, max);
      puts_stdout("...");
    }
  }
}

result_t start_tag(void *user, const char_t *name, const char_t **att) {
  parserinfo_ls_t *pinfo = (parserinfo_ls_t *)user;
  unsigned int d;
  if( pinfo ) { 
    pinfo->npos++;
    pinfo->contin = FALSE;
    d = pinfo->std.depth - pinfo->std.sel.mindepth;
    if( (d <= pinfo->pd) && (d >= 0) ) {
      putc_stdout('\n');
      nputc_stdout('\t', pinfo->indent + d);
      putc_stdout('<');
      puts_stdout(name);

      if( checkflag(pinfo->flags,LS_FLAG_ATTRIBUTES) ) {

#define TRUNCATE_ATTS 10
	while( att && *att ) {
	  putc_stdout(' ');
	  puts_stdout(att[0]);
	  puts_stdout("=\"");
	  write_truncate_stdout(TRUNCATE_ATTS, att[1], strlen(att[1]));
	  putc_stdout('\"');
	  att += 2;
	}

      }

      /* also include the node position in original document */
/*       nprintf_stdout(32, " pos=\"%d\"/>", pinfo->npos); */
      puts_stdout( (d < pinfo->pd) ? ">" : "/>");
    }
  }
  return PARSER_OK;
}

result_t end_tag(void *user, const char_t *name) {
  parserinfo_ls_t *pinfo = (parserinfo_ls_t *)user;
  unsigned int d;
  if( pinfo ) { 
    pinfo->contin = FALSE;
    d = pinfo->std.depth - pinfo->std.sel.mindepth;
    if( (d < pinfo->pd) && (d >= 0) ) {
      putc_stdout('\n');
      nputc_stdout('\t', pinfo->indent + d);
      puts_stdout("</");
      puts_stdout(name);
      putc_stdout('>');
    }
  }
  return PARSER_OK;
}

result_t chardata(void *user, const char_t *buf, size_t buflen) {
#define TRUNCATE 50
  parserinfo_ls_t *pinfo = (parserinfo_ls_t *)user;
  unsigned int d;
  const char_t *p;
  if( pinfo ) { 
    d = pinfo->std.depth - pinfo->std.sel.mindepth;
    if( (d < pinfo->pd) && (d >= 0) ) {
      if( !pinfo->contin ) {
	p = skip_xml_whitespace(buf, buf + buflen);
	if( p < buf + buflen ) {
	  putc_stdout('\n');
	  nputc_stdout('\t', pinfo->indent + d + 1);
	  write_truncate_stdout(TRUNCATE, buf, buflen);
	  pinfo->contin = TRUE;
	}
      }
    }
  }
  return PARSER_OK;
}

bool_t create_parserinfo_ls(parserinfo_ls_t *pinfo) {
  bool_t ok = TRUE;
  if( pinfo ) {

    memset(pinfo, 0, sizeof(parserinfo_ls_t));
    ok &= create_stdparserinfo(&pinfo->std);

    pinfo->std.setup.flags = STDPARSE_MIN1FILE; 
    pinfo->std.setup.cb.start_tag = start_tag;
    pinfo->std.setup.cb.end_tag = end_tag;
    pinfo->std.setup.cb.chardata = chardata;

    pinfo->indent = 1;
    pinfo->contin = FALSE;
    pinfo->pd = 1;


    return ok;
  }
  return FALSE;
} 

bool_t free_parserinfo_ls(parserinfo_ls_t *pinfo) {
  free_stdparserinfo(&pinfo->std);
  return TRUE;
}


int main(int argc, char **argv) {
  signed char op;
  parserinfo_ls_t pinfo;

  struct option longopts[] = {
    { "version", 0, NULL, LS_VERSION },
    { "help", 0, NULL, LS_HELP },
    { "attributes", 0, NULL, LS_ATTRIBUTES },
    { 0 }
  };

  progname = "xml-ls";
  inputfile = "";
  inputline = 0;

  if( create_parserinfo_ls(&pinfo) ) {

    while( (op = getopt_long(argc, argv, "a",
			     longopts, NULL)) > -1 ) {
      set_option_ls(op, optarg, &pinfo);
    }

    init_signal_handling(SIGNALS_DEFAULT);
    init_file_handling();

    open_stdout();
    puts_stdout(get_headwrap());
    puts_stdout(get_open_root());

    stdparse(MAXFILES, argv + optind, (stdparserinfo_t *)&pinfo);

    putc_stdout('\n');
    puts_stdout(get_close_root());
    puts_stdout(get_footwrap());
    close_stdout();

    exit_file_handling();
    exit_signal_handling();
    
    free_parserinfo_ls(&pinfo);
  }

  return EXIT_SUCCESS;
}
