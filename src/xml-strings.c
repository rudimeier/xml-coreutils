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
  flag_t flags, reserved;
} parserinfo_strings_t;

#define STRINGS_VERSION    0x01
#define STRINGS_HELP       0x02
#define STRINGS_VERBATIM   0x03
#define STRINGS_USAGE \
"Usage: xml-strings [OPTION]... [[FILE]... [:XPATH]...]...\n" \
"Display textual strings in FILE(s), or standard input.\n" \
"\n" \
"      --help     display this help and exit\n" \
"      --version  display version information and exit\n"

#define STRINGS_FLAG_SPLIT          0x01
#define STRINGS_FLAG_SQUEEZE        0x02
#define STRINGS_RESERVED_CHARDATA   0x01

void set_option_strings(int op, char *optarg, parserinfo_strings_t *pinfo) {
  switch(op) {
  case STRINGS_VERSION:
    puts("xml-strings" COPYBLURB);
    exit(EXIT_SUCCESS);
    break;
  case STRINGS_HELP:
    puts(STRINGS_USAGE);
    exit(EXIT_SUCCESS);
    break;
  case STRINGS_VERBATIM:
    clearflag(&pinfo->flags,STRINGS_FLAG_SQUEEZE);
    break;
  default:
    break;
  }
}

result_t start_tag(void *user, const char_t *name, const char_t **att) {
  parserinfo_strings_t *pinfo = (parserinfo_strings_t *)user;
  if( pinfo ) { 
    if( true_and_clearflag(&pinfo->reserved,STRINGS_RESERVED_CHARDATA) ) {
      if( checkflag(pinfo->flags,STRINGS_FLAG_SPLIT) ) {
	putc_stdout('\n');
      }
    }
  }
  return PARSER_OK;
}

result_t end_tag(void *user, const char_t *name) {
  parserinfo_strings_t *pinfo = (parserinfo_strings_t *)user;
  if( pinfo ) { 
    if( true_and_clearflag(&pinfo->reserved,STRINGS_RESERVED_CHARDATA) ) {
      if( checkflag(pinfo->flags,STRINGS_FLAG_SPLIT) ) {
	putc_stdout('\n');
      }
    }
  }
  return PARSER_OK;
}

result_t chardata(void *user, const char_t *buf, size_t buflen) {
  parserinfo_strings_t *pinfo = (parserinfo_strings_t *)user;
  if( pinfo ) { 
    if( 0 < pinfo->std.depth ) {
      if( checkflag(pinfo->flags,STRINGS_FLAG_SQUEEZE) ) {
	squeeze_stdout((byte_t *)buf, buflen);
      } else {
	write_stdout((byte_t *)buf, buflen);
      }
      setflag(&pinfo->reserved,STRINGS_RESERVED_CHARDATA);
    }
  }
  return PARSER_OK;
}

result_t attribute(void *user, const char_t *name, const char_t *value) {
  parserinfo_strings_t *pinfo = (parserinfo_strings_t *)user;
  if( pinfo ) { 
    if( 0 < pinfo->std.depth ) {
      puts_stdout(value);
      if( checkflag(pinfo->flags,STRINGS_FLAG_SPLIT) ) {
	putc_stdout('\n');
      }
    }
  }
  return PARSER_OK;
}

bool_t create_parserinfo_strings(parserinfo_strings_t *pinfo) {
  bool_t ok = TRUE;
  if( pinfo ) {

    memset(pinfo, 0, sizeof(parserinfo_strings_t));
    ok &= create_stdparserinfo(&pinfo->std);

    pinfo->std.setup.flags = STDPARSE_MIN1FILE;
    pinfo->std.setup.cb.start_tag = start_tag;
    pinfo->std.setup.cb.end_tag = end_tag;
    pinfo->std.setup.cb.chardata = chardata;
    pinfo->std.setup.cb.attribute = attribute;

    setflag(&pinfo->flags,(STRINGS_FLAG_SPLIT|STRINGS_FLAG_SQUEEZE));

    return ok;
  }
  return FALSE;
} 

bool_t free_parserinfo_strings(parserinfo_strings_t *pinfo) {
  free_stdparserinfo(&pinfo->std);
  return TRUE;
}

int main(int argc, char **argv) {
  signed char op;
  parserinfo_strings_t pinfo;

  struct option longopts[] = {
    { "version", 0, NULL, STRINGS_VERSION },
    { "help", 0, NULL, STRINGS_HELP },
    { "no-squeeze", 0, NULL, STRINGS_VERBATIM },
    { 0 }
  };

  progname = "xml-strings";
  inputfile = "";
  inputline = 0;

  if( create_parserinfo_strings(&pinfo) ) {
    while( (op = getopt_long(argc, argv, "",
			     longopts, NULL)) > -1 ) {
      set_option_strings(op, optarg, &pinfo);
    }

    init_signal_handling(SIGNALS_DEFAULT);
    init_file_handling();

    open_stdout();
    stdparse(MAXFILES, argv + optind, (stdparserinfo_t *)&pinfo);
    close_stdout();

    exit_file_handling();
    exit_signal_handling();

    free_parserinfo_strings(&pinfo);
  }
  return EXIT_SUCCESS;
}
