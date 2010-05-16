/* 
 * Copyright (C) 2010 Laird Breyer
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
#include "stdparse.h"
#include "tempvar.h"
#include "io.h"
#include "awkvm.h"

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
  stdparserinfo_t std; /* must be first */
  flag_t flags;

  awkvm_t vm;

  cstringlst_t files;
  cstringlst_t *xpaths;
  int n;

} parserinfo_awk_t;

#define AWK_VERSION    0x01
#define AWK_HELP       0x02
#define AWK_DEBUG      0x03
#define AWK_SCRIPT     0x04

#define AWK_USAGE \
"Usage: xml-awk [OPTION]... SCRIPT [[FILE]... [:XPATH]...]...\n" \
"\n" \
"\n" \
"      --help     display this help and exit\n" \
"      --version  display version information and exit\n"



bool_t create_parserinfo_awk(parserinfo_awk_t *pinfo) {
  bool_t ok = TRUE;
  if( pinfo ) {

    memset(pinfo, 0, sizeof(parserinfo_awk_t));
    ok &= create_stdparserinfo(&pinfo->std);

    pinfo->flags = 0;

    pinfo->std.setup.flags = 0;

    ok &= create_awkvm(&pinfo->vm);

    return ok;
  }
  return FALSE;
}

bool_t free_parserinfo_awk(parserinfo_awk_t *pinfo) {
  if( pinfo ) {
    free_stdparserinfo(&pinfo->std);
    free_awkvm(&pinfo->vm);
  }
  return FALSE;
}


bool_t compile_string_script(parserinfo_awk_t *pinfo, const char_t *filename,
			  const char_t *begin, const char_t *end) {
  if( pinfo && begin && end ) {
    inputfile = (char *)filename;
    inputline = 0;
    return parse_string_awkvm(&pinfo->vm, begin, end);
  }
  return FALSE;
}

bool_t compile_file_script(parserinfo_awk_t *pinfo, const char *file) {
  bool_t retval = FALSE;
  if( pinfo && file ) {
    inputfile = (char *)file;
    inputline = 0;
    retval = parse_file_awkvm(&pinfo->vm, file);
  }
  return retval;
}

void set_option_awk(int op, char *optarg, parserinfo_awk_t *pinfo) {
  switch(op) {
  case AWK_VERSION:
    puts("xml-awk" COPYBLURB);
    exit(EXIT_SUCCESS);
    break;
  case AWK_HELP:
    puts(AWK_USAGE);
    exit(EXIT_SUCCESS);
    break;
  case 'f':
    compile_file_script(pinfo, optarg);
    break;
  }
}

int main(int argc, char **argv) {
  signed char op;
  parserinfo_awk_t pinfo;
  char_t *script = NULL;

  struct option longopts[] = {
    { "version", 0, NULL, AWK_VERSION },
    { "help", 0, NULL, AWK_HELP },
    { 0 }
  };

  progname = "xml-awk";
  inputfile = "";
  inputline = 0;

  if( create_parserinfo_awk(&pinfo) ) {

    while( (op = getopt_long(argc, argv, "f:",
			     longopts, NULL)) > -1 ) {
      set_option_awk(op, optarg, &pinfo);
    }

    if( !checkflag(pinfo.flags,AWK_SCRIPT) ) {
      if( !argv[optind] ) {
	puts(AWK_USAGE);
	exit(EXIT_FAILURE);
      }
      script = argv[optind];
      compile_string_script(&pinfo, "ARGV", script, script + strlen(script));
      optind++;
    }

    init_file_handling();

    stdparse(MAXFILES, argv + optind, (stdparserinfo_t *)&pinfo);

    exit_file_handling();

    free_parserinfo_awk(&pinfo);
  }


  return EXIT_SUCCESS;
}
