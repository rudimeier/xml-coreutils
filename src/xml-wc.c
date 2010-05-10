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
  int tags;
  int depth;
  int height;
} statistics_t;

typedef struct {
  stdparserinfo_t std; /* must be first so we can cast correctly */
  flag_t flags;
  statistics_t file;
  statistics_t summary;
  int numfiles;
} parserinfo_wc_t;

#define WC_VERSION    0x01
#define WC_HELP       0x02
#define WC_USAGE \
"Usage: xml-wc [OPTION]... [[FILE]... [:XPATH]...]...\n" \
"Prints statistics (height,depth,tags) for each FILE(s), and\n" \
"a total line if more than one FILE is specified.\n" \
"\n" \
"      --help     display this help and exit\n" \
"      --version  display version information and exit\n"

void set_option_wc(int op, char *optarg) {
  switch(op) {
  case WC_VERSION:
    puts("xml-wc" COPYBLURB);
    exit(EXIT_SUCCESS);
    break;
  case WC_HELP:
    puts(WC_USAGE);
    exit(EXIT_SUCCESS);
    break;
  }
}

result_t start_tag(void *user, const char_t *name, const char_t **att) {
  parserinfo_wc_t *pinfo = (parserinfo_wc_t *)user;
  if( pinfo ) { 
    pinfo->file.tags++;
    pinfo->file.height += (pinfo->std.depth == 2) ? 1 : 0;

  }
  return PARSER_OK;
}


bool_t reset_statistics(parserinfo_wc_t *pinfo) {
  if( pinfo ) {
    pinfo->file.height = 0;
    pinfo->file.depth = 0;
    pinfo->file.tags = 0;
    pinfo->numfiles++;
    return TRUE;
  }
  return FALSE;
}

bool_t file_statistics(parserinfo_wc_t *pinfo, 
		       const char_t *filename,
		       const char_t **xpaths) {
  if( pinfo ) {
    pinfo->file.depth = pinfo->std.maxdepth;
    
    pinfo->summary.tags += pinfo->file.tags;
    pinfo->summary.depth = MAX(pinfo->summary.depth,pinfo->file.depth);
    pinfo->summary.height += pinfo->file.height;

    nprintf_stdout(80, "%7d %7d %7d %s",
		   pinfo->file.height,
		   pinfo->file.depth,
		   pinfo->file.tags,
		   filename);
    while( *xpaths ) {
      puts_stdout(" :");
      puts_stdout(*xpaths);
      xpaths++;
    }
    putc_stdout('\n');

    return TRUE;
  }
  return FALSE;
}

bool_t summary_statistics(parserinfo_wc_t *pinfo) {
  if( pinfo ) {
    if( pinfo->numfiles > 1) {
      nprintf_stdout(80, "%7d %7d %7d total\n",
		     pinfo->summary.height,
		     pinfo->summary.depth,
		     pinfo->summary.tags);
    }
    return TRUE;
  }
  return FALSE;
}

bool_t start_file_fun(void *user, const char_t *file, const char_t **xpaths) {
  parserinfo_wc_t *pinfo = (parserinfo_wc_t *)user;
  if( pinfo ) {
    return reset_statistics(pinfo);
  }
  return TRUE;
}

bool_t end_file_fun(void *user, const char_t *file, const char_t **xpaths) {
  parserinfo_wc_t *pinfo = (parserinfo_wc_t *)user;
  if( pinfo ) {
    return file_statistics(pinfo, file, xpaths);
  }
  return TRUE;
}

bool_t create_parserinfo_wc(parserinfo_wc_t *pinfo) {
  bool_t ok = TRUE;
  if( pinfo ) {

    memset(pinfo, 0, sizeof(parserinfo_wc_t));
    ok &= create_stdparserinfo(&pinfo->std);

    pinfo->std.setup.flags = STDPARSE_MIN1FILE;
    pinfo->std.setup.cb.start_tag = start_tag;

    pinfo->std.setup.start_file_fun = start_file_fun;
    pinfo->std.setup.end_file_fun = end_file_fun;

    return ok;
  }
  return FALSE;
}

bool_t free_parserinfo_wc(parserinfo_wc_t *pinfo) {
  free_stdparserinfo(&pinfo->std);
  return TRUE;
}

int main(int argc, char **argv) {
  signed char op;
  parserinfo_wc_t pinfo;

  struct option longopts[] = {
    { "version", 0, NULL, WC_VERSION },
    { "help", 0, NULL, WC_HELP },
    { 0 }
  };

  progname = "xml-wc";
  inputfile = "";
  inputline = 0;

  if( create_parserinfo_wc(&pinfo) ) {

    while( (op = getopt_long(argc, argv, "",
			     longopts, NULL)) > -1 ) {
      set_option_wc(op, optarg);
    }

    init_signal_handling();
    init_file_handling();

    open_stdout();

    stdparse(MAXFILES, argv + optind, (stdparserinfo_t *)&pinfo);

    summary_statistics(&pinfo);

    close_stdout();

    exit_file_handling();
    exit_signal_handling();

    free_parserinfo_wc(&pinfo);
  }
  return EXIT_SUCCESS;
}
