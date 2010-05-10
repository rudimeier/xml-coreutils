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
#include "rcm.h"
#include "rollback.h"
#include "filelist.h"
#include "tempfile.h"
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

  cstringlst_t files;
  cstringlst_t *xpaths;
  int n;

  rcm_t rcm;
  flag_t flags;
} parserinfo_rm_t;

#define RM_VERSION    0x01
#define RM_HELP       0x02
#define RM_FILES      0x03
#define RM_USAGE \
"Usage: xml-rm [OPTION]... [[FILE]... [:XPATH]...]...\n" \
"Remove nodes and print to standard output.\n" \
"\n" \
"      --help     display this help and exit\n" \
"      --version  display version information and exit\n"

#define RM_FLAG_SEEN_STDOUT  0x01
#define RM_FLAG_WARN_STDOUT  0x01

void set_option_rm(int op, char *optarg, parserinfo_rm_t *pinfo) {
  if( pinfo ) {
  switch(op) {
  case RM_VERSION:
    puts("xml-rm" COPYBLURB);
    exit(EXIT_SUCCESS);
    break;
  case RM_HELP:
    puts(RM_USAGE);
    exit(EXIT_SUCCESS);
    break;
  case RM_FILES:
    setflag(&pinfo->rcm.flags, RCM_WRITE_FILES);
    break;
  }
  }
}

result_t start_tag(void *user, const char_t *name, const char_t **att) {
  parserinfo_rm_t *pinfo = (parserinfo_rm_t *)user;
  if( pinfo ) { 
    rm_start_tag_rcm(&pinfo->rcm, &pinfo->std, name, att);
    return PARSER_OK;
  }
  return (PARSER_OK|PARSER_DEFAULT);
}

result_t end_tag(void *user, const char_t *name) {
  parserinfo_rm_t *pinfo = (parserinfo_rm_t *)user;
  if( pinfo ) { 
    rm_end_tag_rcm(&pinfo->rcm, &pinfo->std, name);
    return PARSER_OK;
  }
  return (PARSER_OK|PARSER_DEFAULT);
}

result_t chardata(void *user, const char_t *buf, size_t buflen) {
  parserinfo_rm_t *pinfo = (parserinfo_rm_t *)user;
  if( pinfo ) { 
    rm_chardata_rcm(&pinfo->rcm, &pinfo->std, buf, buflen);
  }
  return PARSER_OK;
}

result_t dfault(void *user, const char_t *data, size_t buflen) {
  parserinfo_rm_t *pinfo = (parserinfo_rm_t *)user;
  if( pinfo ) { 
    rm_dfault_rcm(&pinfo->rcm, &pinfo->std, data, buflen);
  }
  return PARSER_OK;
}

bool_t start_file_fun(void *user, const char_t *file, const char_t **xpaths) {
  parserinfo_rm_t *pinfo = (parserinfo_rm_t *)user;
  if( pinfo ) {

    if( (strcmp(file, "stdin") == 0) && 
	checkflag(pinfo->rcm.flags, RCM_WRITE_FILES) ) {
      errormsg(E_WARNING, "cannot write to stdin, ignoring this file.\n");
      return FALSE;
    }

    if( strcmp(file, "stdout") == 0 ) {
      if( checkflag(pinfo->flags, RM_FLAG_SEEN_STDOUT) ) {
	if( false_and_setflag(&pinfo->flags, RM_FLAG_WARN_STDOUT) ) {
	  errormsg(E_WARNING, 
		   "only one stdout target allowed, ignoring remaining.\n");
	}
      }
      setflag(&pinfo->flags, RM_FLAG_SEEN_STDOUT);
    }

    rm_start_file_rcm(&pinfo->rcm, file);
    return TRUE;
  }
  return TRUE;
}

result_t end_file_fun(void *user, const char_t *file, const char_t **xpaths) {
  parserinfo_rm_t *pinfo = (parserinfo_rm_t *)user;
  if( pinfo ) {
    rm_end_file_rcm(&pinfo->rcm, file);
    return TRUE;
  }
  return TRUE;
}

bool_t create_parserinfo_rm(parserinfo_rm_t *pinfo) {
  bool_t ok = TRUE;
  if( pinfo ) {
    memset(pinfo, 0, sizeof(parserinfo_rm_t));
    ok &= create_stdparserinfo(&pinfo->std);
    ok &= create_rcm(&pinfo->rcm);

    pinfo->std.setup.flags = STDPARSE_ALLNODES;
    pinfo->std.setup.cb.start_tag = start_tag;
    pinfo->std.setup.cb.end_tag = end_tag;
    pinfo->std.setup.cb.chardata = chardata;
    pinfo->std.setup.cb.dfault = dfault;

    pinfo->std.setup.start_file_fun = start_file_fun;
    pinfo->std.setup.end_file_fun = end_file_fun;

    pinfo->flags = 0;

    return ok;
  }
  return FALSE;
}


bool_t free_parserinfo_rm(parserinfo_rm_t *pinfo) {
  if( pinfo ) {
    free_stdparserinfo(&pinfo->std);
    free_rcm(&pinfo->rcm);
    return TRUE;
  }
  return FALSE;
}


int main(int argc, char **argv) {
  signed char op;
  parserinfo_rm_t pinfo;
  filelist_t fl;

  struct option longopts[] = {
    { "version", 0, NULL, RM_VERSION },
    { "help", 0, NULL, RM_HELP },
    { "write-files", 0, NULL, RM_FILES },
    { 0 }
  };

  progname = "xml-rm";
  inputfile = "";
  inputline = 0;

  if( create_parserinfo_rm(&pinfo) ) {

    while( (op = getopt_long(argc, argv, "",
			     longopts, NULL)) > -1 ) {
      set_option_rm(op, optarg, &pinfo);
    }

    if( create_filelist(&fl, -1, argv + optind, FILELIST_MIN1) ) {

      pinfo.files = getfiles_filelist(&fl);
      pinfo.xpaths = getxpaths_filelist(&fl);
      pinfo.n = getsize_filelist(&fl);


      if( pinfo.n == 1 ) {

	if( (strcmp(pinfo.files[0], "stdin") == 0) &&
	    checkflag(pinfo.rcm.flags, RCM_WRITE_FILES) ) {
	  
	}

	/* output always to stdout */
	setflag(&pinfo.rcm.flags, RCM_RM_OUTPUT);

      } else {
	if( !checkflag(pinfo.rcm.flags, RCM_WRITE_FILES) ) {
	  errormsg(E_FATAL, "too many input files (try --help).\n");
	}
      }

      init_signal_handling();
      init_file_handling();
      init_tempfile_handling();
      init_rollback_handling();

      stdparse2(pinfo.n, pinfo.files, pinfo.xpaths,
		(stdparserinfo_t *)&pinfo);

      exit_rollback_handling();
      exit_tempfile_handling();
      exit_file_handling();
      exit_signal_handling();

      free_filelist(&fl);
    }

    free_parserinfo_rm(&pinfo);
  }



  return EXIT_SUCCESS;
}
