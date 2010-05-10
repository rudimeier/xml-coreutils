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
} parserinfo_fmt_t;

#define FMT_VERSION    0x01
#define FMT_HELP       0x02
#define FMT_USAGE \
"Usage: xml-fmt [OPTION]... [FILE]\n" \
"Reformat each node in FILE(s), or standard input.\n" \
"\n" \
"      --help     display this help and exit\n" \
"      --version  display version information and exit\n"

void set_option_fmt(int op, char *optarg) {
  switch(op) {
  case FMT_VERSION:
    puts("xml-fmt" COPYBLURB);
    exit(EXIT_SUCCESS);
    break;
  case FMT_HELP:
    puts(FMT_USAGE);
    exit(EXIT_SUCCESS);
    break;
  }
}

result_t start_tag(void *user, const char_t *name, const char_t **att) {
  parserinfo_fmt_t *pinfo = (parserinfo_fmt_t *)user;
  if( pinfo ) { 
    stdprint_start_tag(&pinfo->std, name, att);
  }
  return PARSER_OK;
}

result_t end_tag(void *user, const char_t *name) {
  parserinfo_fmt_t *pinfo = (parserinfo_fmt_t *)user;
  if( pinfo ) { 
    stdprint_end_tag(&pinfo->std, name);
  }
  return PARSER_OK;
}

result_t chardata(void *user, const char_t *buf, size_t buflen) {
  parserinfo_fmt_t *pinfo = (parserinfo_fmt_t *)user;
  if( pinfo ) { 
    if( pinfo->std.depth == 0 ) {
	write_coded_entities_stdout(buf, buflen);
    } else {
      stdprint_chardata(&pinfo->std, buf, buflen);
    }
  }
  return PARSER_OK;
}

result_t dfault(void *user, const char_t *buf, size_t buflen) {
  parserinfo_fmt_t *pinfo = (parserinfo_fmt_t *)user;
  if( pinfo ) { 
    if( pinfo->std.depth == 0 ) {
      /* write_stdout((byte_t *)buf, buflen); */
    } else {
      stdprint_chardata(&pinfo->std, buf, buflen);
    }
  }
  return PARSER_OK;
}


result_t pidata(void *user, const char_t *target, const char_t *data) {
  parserinfo_fmt_t *pinfo = (parserinfo_fmt_t *)user;
  if( pinfo ) { 
    stdprint_pidata(&pinfo->std, target, data);
  }
  return PARSER_OK;
}

result_t start_cdata(void *user) {
  parserinfo_fmt_t *pinfo = (parserinfo_fmt_t *)user;
  if( pinfo ) { 
    stdprint_start_cdata(&pinfo->std);
  }
  return PARSER_OK;
}

result_t end_cdata(void *user) {
  parserinfo_fmt_t *pinfo = (parserinfo_fmt_t *)user;
  if( pinfo ) { 
    stdprint_end_cdata(&pinfo->std);
  }
  return PARSER_OK;
}

result_t comment(void *user, const char_t *data) {
  parserinfo_fmt_t *pinfo = (parserinfo_fmt_t *)user;
  if( pinfo ) { 
    stdprint_comment(&pinfo->std, data);
  }
  return PARSER_OK;
}

bool_t create_parserinfo_fmt(parserinfo_fmt_t *pinfo) {
  bool_t ok = TRUE;
  if( pinfo ) {

    memset(pinfo, 0, sizeof(parserinfo_fmt_t));
    ok &= create_stdparserinfo(&pinfo->std);

    pinfo->std.setup.flags = 
      STDPARSE_ALLNODES|STDPARSE_EQ1FILE|STDPARSE_NOXPATHS;
    pinfo->std.setup.cb.start_tag = start_tag;
    pinfo->std.setup.cb.end_tag = end_tag;
    pinfo->std.setup.cb.chardata = chardata;
    pinfo->std.setup.cb.start_cdata = start_cdata;
    pinfo->std.setup.cb.end_cdata = end_cdata;
    pinfo->std.setup.cb.pidata = pidata;
    pinfo->std.setup.cb.comment = comment;
    pinfo->std.setup.cb.dfault = dfault;
    pinfo->flags = 0;

    return ok;
  }
  return FALSE;
} 

bool_t free_parserinfo_fmt(parserinfo_fmt_t *pinfo) {
  free_stdparserinfo(&pinfo->std);
  return TRUE;
}


int main(int argc, char **argv) {
  signed char op;
  parserinfo_fmt_t pinfo;

  struct option longopts[] = {
    { "version", 0, NULL, FMT_VERSION },
    { "help", 0, NULL, FMT_HELP },
    { 0 }
  };

  progname = "xml-fmt";
  inputfile = "";
  inputline = 0;

  if( create_parserinfo_fmt(&pinfo) ) {
    while( (op = getopt_long(argc, argv, "",
			     longopts, NULL)) > -1 ) {
      set_option_fmt(op, optarg);
    }

    init_signal_handling();
    init_file_handling();

    open_stdout();

    stdparse(MAXFILES, argv + optind, (stdparserinfo_t *)&pinfo);
    close_stdout();

    exit_file_handling();
    exit_signal_handling();

    free_parserinfo_fmt(&pinfo);
  }

  return EXIT_SUCCESS;
}
