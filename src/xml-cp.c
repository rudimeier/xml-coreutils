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
#include "tempcollect.h"
#include "filelist.h"
#include "rcm.h"
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
  stdparserinfo_t std; /* must be first */

  cstringlst_t files;
  cstringlst_t *xpaths;
  int n;

  rcm_t rcm;
  tempcollect_t sav;
  flag_t flags;
} parserinfo_cp_t;

#define CP_VERSION    0x01
#define CP_HELP       0x02
#define CP_FILES      0x03
#define CP_PREPEND    0x04
#define CP_REPLACE    0x05
#define CP_APPEND     0x06
#define CP_MULTI      0x07
#define CP_USAGE \
"Usage: xml-cp [OPTION]... [[FILE]... [:XPATH]...]... TARGET [:XPATH]...\n" \
"Reformat each node in FILE(s), or standard input.\n" \
"\n" \
"      --help     display this help and exit\n" \
"      --version  display version information and exit\n"

#define CP_FLAG_TARGET   0x01


void set_option_cp(int op, char *optarg, parserinfo_cp_t *pinfo) {
  switch(op) {
  case CP_VERSION:
    puts("xml-cp" COPYBLURB);
    exit(EXIT_SUCCESS);
    break;
  case CP_HELP:
    puts(CP_USAGE);
    exit(EXIT_SUCCESS);
    break;
  case CP_FILES:
    setflag(&pinfo->rcm.flags, RCM_WRITE_FILES);
    break;
  case CP_PREPEND:
    setflag(&pinfo->rcm.flags, RCM_CP_PREPEND);
    break;
  case CP_REPLACE:
    setflag(&pinfo->rcm.flags, RCM_CP_REPLACE);
    break;
  case CP_APPEND:
    setflag(&pinfo->rcm.flags, RCM_CP_APPEND);
    break;
  case CP_MULTI:
    setflag(&pinfo->rcm.flags, RCM_CP_MULTI);
    break;
  }
}


result_t start_tag(void *user, const char_t *name, const char_t **att) {
  parserinfo_cp_t *pinfo = (parserinfo_cp_t *)user;
  const char_t *path;
  if( pinfo ) { 

    if( checkflag(pinfo->flags,CP_FLAG_TARGET) ) {

      cp_start_tag_rcm(&pinfo->rcm, &pinfo->std, name, att);

    } else {

      if( pinfo->std.sel.active ) {
	write_start_tag_tempcollect(&pinfo->sav, name, att);
      } else if( pinfo->std.sel.attrib ) {
	path = string_xpath(&pinfo->std.cp);
	do {
	  if( check_xattributelist(&pinfo->std.sel.atts, path, att[0]) ) {
	    puts_tempcollect(&pinfo->sav, att[1]);
	  }
	  att += 2;
	} while( att && *att );
      }

    }

  }
  return PARSER_OK;
}

result_t end_tag(void *user, const char_t *name) {
  parserinfo_cp_t *pinfo = (parserinfo_cp_t *)user;
  if( pinfo ) { 

    if( checkflag(pinfo->flags,CP_FLAG_TARGET) ) {

      cp_end_tag_rcm(&pinfo->rcm, &pinfo->std, name);

      if( (pinfo->std.depth == 1) &&
	  !checkflag(pinfo->rcm.flags,RCM_CP_OKINSERT) ) {
	if( !checkflag(pinfo->rcm.flags,RCM_CP_MULTI) ) {
	  errormsg(E_WARNING, 
		   "missing target insertion point, nothing copied.\n");
	}
      }

    } else {

      if( pinfo->std.sel.active ) {
	write_end_tag_tempcollect(&pinfo->sav, name);
      }

    }

  }
  return PARSER_OK;
}

result_t chardata(void *user, const char_t *buf, size_t buflen) {
  parserinfo_cp_t *pinfo = (parserinfo_cp_t *)user;
  if( pinfo ) { 

    if( checkflag(pinfo->flags,CP_FLAG_TARGET) ) {

      cp_chardata_rcm(&pinfo->rcm, &pinfo->std, buf, buflen);

    } else {
      if( pinfo->std.sel.active ) {
	write_coded_entities_tempcollect(&pinfo->sav, buf, buflen);
      }

    }
  }
  return PARSER_OK;
}

result_t dfault(void *user, const char_t *data, size_t buflen) {
  parserinfo_cp_t *pinfo = (parserinfo_cp_t *)user;
  if( pinfo ) { 

    if( checkflag(pinfo->flags,CP_FLAG_TARGET) ) {

      cp_dfault_rcm(&pinfo->rcm, &pinfo->std, data, buflen);

    } else {
      if( pinfo->std.sel.active ) {
	write_tempcollect(&pinfo->sav, (byte_t *)data, buflen);
      }

    }
  }
  return PARSER_OK;
}


bool_t create_parserinfo_cp(parserinfo_cp_t *pinfo) {

  bool_t ok = TRUE;
  if( pinfo ) {
    memset(pinfo, 0, sizeof(parserinfo_cp_t));
    ok &= create_stdparserinfo(&pinfo->std);
    ok &= create_rcm(&pinfo->rcm);
    ok &= create_tempcollect(&pinfo->sav, "sav", MINVARSIZE, MAXVARSIZE);

    if( ok ) {

      pinfo->std.setup.flags = STDPARSE_ALLNODES|STDPARSE_ALWAYS_CHARDATA;
      pinfo->std.setup.cb.start_tag = start_tag;
      pinfo->std.setup.cb.end_tag = end_tag;
      pinfo->std.setup.cb.chardata = chardata;
      pinfo->std.setup.cb.dfault = dfault;

      pinfo->flags = 0;

    }
    return ok;
  }
  return FALSE;
}

bool_t start_file_fun(void *user, const char_t *file, const char_t **xpaths) {
  parserinfo_cp_t *pinfo = (parserinfo_cp_t *)user;
  if( pinfo ) {

    if( (strcmp(file, "stdin") == 0) && 
	checkflag(pinfo->rcm.flags, RCM_WRITE_FILES) ) {
      errormsg(E_WARNING, "cannot write to stdin, ignoring this file.\n");
      return FALSE;
    }
    cp_start_target_rcm(&pinfo->rcm, file);

    return TRUE;
  }
  return TRUE;
}

bool_t end_file_fun(void *user, const char_t *file, const char_t **xpaths) {
  parserinfo_cp_t *pinfo = (parserinfo_cp_t *)user;
  if( pinfo ) {
    cp_end_target_rcm(&pinfo->rcm, file);
    return TRUE;
  }
  return TRUE;
}

bool_t reinit_parserinfo_cp(parserinfo_cp_t *pinfo) {
  if( pinfo ) {

    insert_rcm(&pinfo->rcm, &pinfo->sav);
    setflag(&pinfo->flags, CP_FLAG_TARGET);	
    
    pinfo->std.setup.start_file_fun = start_file_fun;
    pinfo->std.setup.end_file_fun = end_file_fun;

    return TRUE;
  }
  return FALSE;
}


bool_t free_parserinfo_cp(parserinfo_cp_t *pinfo) {
  if( pinfo ) {
    free_stdparserinfo(&pinfo->std);
    free_rcm(&pinfo->rcm);
    free_tempcollect(&pinfo->sav);
    return TRUE;
  }
  return FALSE;

}

int main(int argc, char **argv) {
  signed char op;
  parserinfo_cp_t pinfo;
  filelist_t fl;

  struct option longopts[] = {
    { "version", 0, NULL, CP_VERSION },
    { "help", 0, NULL, CP_HELP },
    { "write-files", 0, NULL, CP_FILES },
    { "prepend", 0, NULL, CP_PREPEND },
    { "replace", 0, NULL, CP_REPLACE },
    { "append", 0, NULL, CP_APPEND },
    { "multi", 0, NULL, CP_MULTI },
    { 0 }
  };

  progname = "xml-cp";
  inputfile = "";
  inputline = 0;

  if( create_parserinfo_cp(&pinfo) ) {

    while( (op = getopt_long(argc, argv, "",
			     longopts, NULL)) > -1 ) {
      set_option_cp(op, optarg, &pinfo);
    }

    if( !checkflag(pinfo.rcm.flags, RCM_CP_PREPEND|RCM_CP_APPEND) ) {
      setflag(&pinfo.rcm.flags,RCM_CP_APPEND);
      setflag(&pinfo.rcm.flags,RCM_CP_REPLACE);
    } 

    if( create_filelist(&fl, -1, argv + optind, FILELIST_MIN2) ) {

      pinfo.files = getfiles_filelist(&fl);
      pinfo.xpaths = getxpaths_filelist(&fl);
      pinfo.n = getsize_filelist(&fl);
    
      if( pinfo.n < 2 ) { 
	errormsg(E_FATAL, "no target specified (try --help).\n");
      }

      init_signal_handling(SIGNALS_DEFAULT);
      init_file_handling();
      init_tempfile_handling();

      if( stdparse2(pinfo.n - 1, pinfo.files, pinfo.xpaths, &pinfo.std) ) {

	if( reinit_parserinfo_cp(&pinfo) ) {
	  /* output always to stdout */
	  setflag(&pinfo.rcm.flags, RCM_CP_OUTPUT);

	  stdparse2(1, &pinfo.files[pinfo.n - 1], 
		    &pinfo.xpaths[pinfo.n - 1], &pinfo.std);

	}

      }

      exit_tempfile_handling();
      exit_file_handling();      
      exit_signal_handling();

      free_filelist(&fl);
    }

    free_parserinfo_cp(&pinfo);
  }

  return EXIT_SUCCESS;
}
