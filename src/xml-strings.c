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
} parserinfo_strings_t;

#define STRINGS_VERSION    0x01
#define STRINGS_HELP       0x02
#define STRINGS_USAGE \
"Usage: xml-strings [OPTION] [[FILE]:XPATH]...\n" \
"Display textual strings in FILE(s), or standard input.\n" \
"\n" \
"      --help     display this help and exit\n" \
"      --version  display version information and exit\n"

void set_option(int op, char *optarg) {
  switch(op) {
  case STRINGS_VERSION:
    puts("xml-strings" COPYBLURB);
    exit(EXIT_SUCCESS);
    break;
  case STRINGS_HELP:
    puts(STRINGS_USAGE);
    exit(EXIT_SUCCESS);
    break;
  }
}

result_t start_tag(void *user, const char_t *name, const char_t **att) {
  parserinfo_strings_t *pinfo = (parserinfo_strings_t *)user;
  if( pinfo ) { 
    return PARSER_OK;
  }
  return (PARSER_OK|PARSER_DEFAULT);
}

result_t end_tag(void *user, const char_t *name) {
  parserinfo_strings_t *pinfo = (parserinfo_strings_t *)user;
  if( pinfo ) { 
    return PARSER_OK;
  }
  return (PARSER_OK|PARSER_DEFAULT);
}

result_t chardata(void *user, const char_t *buf, size_t buflen) {
  parserinfo_strings_t *pinfo = (parserinfo_strings_t *)user;
  if( pinfo ) { 
    if( 0 < pinfo->std.depth ) {
      write_stdout((byte_t *)buf, buflen);
    }
  }
  return PARSER_OK;
}

result_t pidata(void *user, const char_t *target, const char_t *data) {
  /* just ignore */
  return PARSER_OK;
}

result_t dfault(void *user, const char_t *data, size_t buflen) {
  parserinfo_strings_t *pinfo = (parserinfo_strings_t *)user;
  if( pinfo ) { 
    if( 0 < pinfo->std.depth ) {
      write_stdout((byte_t *)data, buflen);
    }
  }
  return PARSER_OK;
}

int main(int argc, char **argv) {
  signed char op;
  parserinfo_strings_t pinfo;

  struct option longopts[] = {
    { "version", 0, NULL, STRINGS_VERSION },
    { "help", 0, NULL, STRINGS_HELP },
  };

  progname = "xml-strings";
  inputfile = "stdin";
  inputline = 0;

  while( (op = getopt_long(argc, argv, "",
			   longopts, NULL)) > -1 ) {
    set_option(op, optarg);
  }

  init_file_handling();

  memset(&pinfo, 0, sizeof(parserinfo_strings_t));
  pinfo.std.setup.flags = 0;
  pinfo.std.setup.cb.start_tag = start_tag;
  pinfo.std.setup.cb.end_tag = end_tag;
  pinfo.std.setup.cb.chardata = chardata;
  pinfo.std.setup.cb.pidata = NULL; /* pidata; */
  pinfo.std.setup.cb.comment = NULL;
  pinfo.std.setup.cb.dfault = NULL; /* dfault; */

  open_stdout();
  stdparse(optind, argv, (stdparserinfo_t *)&pinfo);
  close_stdout();

  exit_file_handling();

  return EXIT_SUCCESS;
}
