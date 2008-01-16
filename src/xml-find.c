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

typedef enum { print } command_t;

typedef struct {
  stdparserinfo_t std; /* must be first so we can cast correctly */
  command_t cmd;
} parserinfo_find_t;

#define FIND_VERSION    0x01
#define FIND_HELP       0x02
#define FIND_USAGE \
"Usage: xml-find [OPTION] [[FILE]:XPATH]...\n" \
"Search for XML nodes in an FILE, or standard input.\n" \
"\n" \
"      --help     display this help and exit\n" \
"      --version  display version information and exit\n"

void set_option(int op, char *optarg) {
  switch(op) {
  case FIND_VERSION:
    puts("xml-find" COPYBLURB);
    exit(EXIT_SUCCESS);
    break;
  case FIND_HELP:
    puts(FIND_USAGE);
    exit(EXIT_SUCCESS);
    break;
  }
}

result_t start_tag(void *user, const char_t *name, const char_t **att) {
  parserinfo_find_t *pinfo = (parserinfo_find_t *)user;
  if( pinfo ) { 
    puts_stdout(pinfo->std.cp.path);
    putc_stdout('\n');
  }
  return PARSER_OK;
}

result_t end_tag(void *user, const char_t *name) {
  parserinfo_find_t *pinfo = (parserinfo_find_t *)user;
  if( pinfo ) { 
  }
  return PARSER_OK;
}

result_t chardata(void *user, const char_t *buf, size_t buflen) {
  parserinfo_find_t *pinfo = (parserinfo_find_t *)user;
  if( pinfo ) { 
  }
  return PARSER_OK;
}

result_t pidata(void *user, const char_t *target, const char_t *data) {
  /* just ignore */
  return PARSER_OK;
}

result_t dfault(void *user, const char_t *data, size_t buflen) {
  parserinfo_find_t *pinfo = (parserinfo_find_t *)user;
  if( pinfo ) { 
  }
  return PARSER_OK;
}

int main(int argc, char **argv) {
  signed char op;
  parserinfo_find_t pinfo;

  struct option longopts[] = {
    { "version", 0, NULL, FIND_VERSION },
    { "help", 0, NULL, FIND_HELP },
  };

  progname = "xml-find";
  inputfile = "stdin";
  inputline = 0;

  while( (op = getopt_long(argc, argv, "",
			   longopts, NULL)) > -1 ) {
    set_option(op, optarg);
  }

  init_file_handling();

  memset(&pinfo, 0, sizeof(parserinfo_find_t));
  pinfo.std.setup.flags = 0; /* STDPARSE_ALLNODES; */
  pinfo.std.setup.cb.start_tag = start_tag;
  pinfo.std.setup.cb.end_tag = end_tag;
  pinfo.std.setup.cb.chardata = chardata;
  pinfo.std.setup.cb.pidata = NULL; /* pidata; */
  pinfo.std.setup.cb.comment = NULL;
  pinfo.std.setup.cb.dfault = NULL; /* dfault; */

  pinfo.cmd = print;

  open_stdout();

  stdparse(optind, argv, (stdparserinfo_t *)&pinfo);

  close_stdout();

  exit_file_handling();

  return EXIT_SUCCESS;
}
