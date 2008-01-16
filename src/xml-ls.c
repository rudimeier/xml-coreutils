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
#include "error.h"
#include "wrap.h"
#include "stdout.h"
#include "stdparse.h"

#include <string.h>
#include <getopt.h>

/* for option processing */
extern char *optarg;
extern int optind, opterr, optopt;

extern char *progname;
extern char *inputfile;
extern long inputline;

extern int cmd;

#include <stdio.h>

typedef struct {
  stdparserinfo_t std; /* must be first so we can cast correctly */
  unsigned int indent;
  bool_t contin;
  unsigned int npos;
  unsigned int pd;
} parserinfo_ls_t;

#define LS_VERSION    0x01
#define LS_HELP       0x02
#define LS_USAGE \
"Usage: xml-ls [OPTION] [FILE]...\n" \
"List information about the FILE, or standard input.\n" \
"\n" \
"      --help     display this help and exit\n" \
"      --version  display version information and exit\n"

void set_option(int op, char *optarg) {
  switch(op) {
  case LS_VERSION:
    puts("xml-ls" COPYBLURB);
    exit(EXIT_SUCCESS);
    break;
  case LS_HELP:
    puts(LS_USAGE);
    exit(EXIT_SUCCESS);
    break;
  }
}

/* replace \n[ t]* sequences with \n[ ]{indent} */
/* bool_t indent_stdout(const char_t *buf, size_t buflen, unsigned int indent) { */
/*   const char_t *p, *q, *e; */
/*   if( buf && (buflen > 0) ) { */
/*     p = q = buf; */
/*     e = buf + buflen; */
/*     while( p < e ) { */
/*       p = find_delimiter(q, e, '\n'); */
/*       write_stdout((byte_t *)q, p - q); */
/*       p = skip_xml_whitespace(p, e); */
/*       if( p < e ) { */
/* 	q = p; */
/* 	nputc_stdout(' ', indent); */
/*       } */
/*     }  */
/*     return TRUE; */
/*   } */
/*   return FALSE; */
/* } */

result_t start_tag(void *user, const char_t *name, const char_t **att) {
  parserinfo_ls_t *pinfo = (parserinfo_ls_t *)user;
  unsigned int d;
  if( pinfo ) { 
    pinfo->npos++;
    pinfo->contin = FALSE;
    d = pinfo->std.depth - pinfo->std.sel.mindepth;
    if( (d <= pinfo->pd) && (d >= 0) ) {
      putc_stdout('\n');
      nputc_stdout(' ', pinfo->indent + d);
      putc_stdout('<');
      puts_stdout(name);
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
      nputc_stdout(' ', pinfo->indent + d);
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
	  nputc_stdout(' ', pinfo->indent + d + 1);
	  if( buflen < TRUNCATE ) {
	    squeeze_stdout((byte_t *)buf, buflen);
	  } else {
	    squeeze_stdout((byte_t *)buf, TRUNCATE);
	    puts_stdout("...");
	  }
	  pinfo->contin = TRUE;
	}
      }
    }
  }
  return PARSER_OK;
}

result_t pidata(void *user, const char_t *target, const char_t *data) {
  /* just ignore */
  return PARSER_OK;
}

result_t dfault(void *user, const char_t *data, size_t buflen) {
  parserinfo_ls_t *pinfo = (parserinfo_ls_t *)user;
  if( pinfo ) { 
    if( 0 < pinfo->std.depth ) {
      write_stdout((byte_t *)data, buflen);
    }
  }
  return PARSER_OK;
}

int main(int argc, char **argv) {
  signed char op;
  parserinfo_ls_t pinfo;

  struct option longopts[] = {
    { "version", 0, NULL, LS_VERSION },
    { "help", 0, NULL, LS_HELP },
  };

  progname = "xml-ls";
  inputfile = "stdin";
  inputline = 0;

  while( (op = getopt_long(argc, argv, "",
			   longopts, NULL)) > -1 ) {
    set_option(op, optarg);
  }

  init_file_handling();

  memset(&pinfo, 0, sizeof(parserinfo_ls_t));
  pinfo.std.setup.flags = 0; /* STDPARSE_ALLNODES; */
  pinfo.std.setup.cb.start_tag = start_tag;
  pinfo.std.setup.cb.end_tag = end_tag;
  pinfo.std.setup.cb.chardata = chardata;
  pinfo.std.setup.cb.pidata = NULL; /* pidata; */
  pinfo.std.setup.cb.comment = NULL;
  pinfo.std.setup.cb.dfault = NULL; /* dfault; */

  pinfo.indent = 1;
  pinfo.contin = FALSE;
  pinfo.pd = 1;


  open_stdout();
  puts_stdout(get_headwrap());

  stdparse(optind, argv, (stdparserinfo_t *)&pinfo);

  putc_stdout('\n');
  puts_stdout(get_footwrap());
  close_stdout();

  exit_file_handling();

  return EXIT_SUCCESS;
}
