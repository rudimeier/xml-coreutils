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
#include "rollback.h"
#include "tempfile.h"
#include "mysignal.h"

#include <string.h>
#include <getopt.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

/* for option processing */
extern char *optarg;
extern int optind, opterr, optopt;

extern char *progname;
extern char *inputfile;
extern long inputline;

extern volatile flag_t cmd;

#include <stdio.h>


typedef struct {
  stdparserinfo_t std;  /* must be first */

  cstringlst_t files;
  cstringlst_t *xpaths;
  int n;

  const char_t *target;
  cstringlst_t tpaths;
  char_t *tempfile;
  int tempfd;

  rcm_t rcm;
  tempcollect_t sav;
  flag_t flags;
} parserinfo_mv_t;

#define MV_VERSION    0x01
#define MV_HELP       0x02
#define MV_FILES      0x03
#define MV_PREPEND    0x04
#define MV_REPLACE    0x05
#define MV_APPEND     0x06
#define MV_USAGE \
"Usage: xml-mv [OPTION]... [[FILE]... [:XPATH]...]...\n" \
"Reformat each node in FILE(s), or standard input.\n" \
"\n" \
"      --help     display this help and exit\n" \
"      --version  display version information and exit\n"

#define MV_FLAG_TARGET       0x01
#define MV_FLAG_SEEN_STDOUT  0x02
#define MV_FLAG_WARN_STDOUT  0x04


void set_option_mv(int op, char *optarg, parserinfo_mv_t *pinfo) {
  switch(op) {
  case MV_VERSION:
    puts("xml-mv" COPYBLURB);
    exit(EXIT_SUCCESS);
    break;
  case MV_HELP:
    puts(MV_USAGE);
    exit(EXIT_SUCCESS);
    break;
  case MV_FILES:
    setflag(&pinfo->rcm.flags, RCM_WRITE_FILES);
    break;
  case MV_PREPEND:
    setflag(&pinfo->rcm.flags, RCM_CP_PREPEND);
    break;
  case MV_REPLACE:
    setflag(&pinfo->rcm.flags, RCM_CP_REPLACE);
    break;
  case MV_APPEND:
    setflag(&pinfo->rcm.flags, RCM_CP_APPEND);
    break;
  }
}

/* In the pinfo->file list, we replace all the instances of the target
 * file (except the first) with a temporary file that we can read and
 * write at will.  If some replacements were performed, then tempfile is
 * not null.
 * The tempfile is opened exclusively, and should be kept open throughout
 * the lifetime of xml-mv as a way of guaranteeing privacy.
 */
bool_t substitute_target(parserinfo_mv_t *pinfo) {
  bool_t first = TRUE;
  int i;
  if( pinfo ) {

    for(i = 0; i < pinfo->n; i++) {
      if( strcmp(pinfo->files[i],pinfo->target) == 0 ) {
	if( !first ) {
	  if( !pinfo->tempfile ) {
	    pinfo->tempfile = make_template_tempfile(progname);
	    pinfo->tempfd = open_tempfile(pinfo->tempfile);
	    if( pinfo->tempfd == -1 ) {
	      errormsg(E_FATAL, 
		       "could not create temporary file %s, aborting.\n",
		       pinfo->tempfile);
	    }
	  }
	  pinfo->files[i] = pinfo->tempfile;
	} else {
	  first = FALSE;
	}
      }
    }
    return (pinfo->tempfile != NULL);
  }
  return FALSE;
}

result_t start_tag(void *user, const char_t *name, const char_t **att) {
  parserinfo_mv_t *pinfo = (parserinfo_mv_t *)user;
  const char_t *path;
  const char_t **fatt;
  if( pinfo ) { 

    if( checkflag(pinfo->flags,MV_FLAG_TARGET) ) {
      cp_start_tag_rcm(&pinfo->rcm, &pinfo->std, name, att);
    } else {
      if( pinfo->std.sel.active ) {
	write_start_tag_tempcollect(&pinfo->sav, name, att);
      } else if( pinfo->std.sel.attrib ) {
	path = string_xpath(&pinfo->std.cp);
	fatt = att;
	do {
	  if( check_xattributelist(&pinfo->std.sel.atts, path, fatt[0]) ) {
	    puts_tempcollect(&pinfo->sav, fatt[1]);
	  }
	  fatt += 2;
	} while( fatt && *fatt );
      }
      rm_start_tag_rcm(&pinfo->rcm, &pinfo->std, name, att);
    }

  }
  return PARSER_OK;
}

result_t end_tag(void *user, const char_t *name) {
  parserinfo_mv_t *pinfo = (parserinfo_mv_t *)user;
  if( pinfo ) { 

    if( checkflag(pinfo->flags,MV_FLAG_TARGET) ) {

      cp_end_tag_rcm(&pinfo->rcm, &pinfo->std, name);
      if( (pinfo->std.depth == 1) &&
	  !checkflag(pinfo->rcm.flags,RCM_CP_OKINSERT) ) {
	errormsg(E_WARNING, 
		 "missing target insertion point, nothing copied.\n");
      }

    } else {

      if( pinfo->std.sel.active ) {
	write_end_tag_tempcollect(&pinfo->sav, name);
      }
      rm_end_tag_rcm(&pinfo->rcm, &pinfo->std, name);

    }

  }
  return PARSER_OK;
}

result_t chardata(void *user, const char_t *buf, size_t buflen) {
  parserinfo_mv_t *pinfo = (parserinfo_mv_t *)user;
  if( pinfo ) { 

    if( checkflag(pinfo->flags,MV_FLAG_TARGET) ) {
      cp_chardata_rcm(&pinfo->rcm, &pinfo->std, buf, buflen);
    } else {
      if( pinfo->std.sel.active ) {
	write_coded_entities_tempcollect(&pinfo->sav, buf, buflen);
      }
      rm_chardata_rcm(&pinfo->rcm, &pinfo->std, buf, buflen);
    }

  }
  return PARSER_OK;
}

result_t dfault(void *user, const char_t *data, size_t buflen) {
  parserinfo_mv_t *pinfo = (parserinfo_mv_t *)user;
  if( pinfo ) { 

    if( checkflag(pinfo->flags,MV_FLAG_TARGET) ) {
      cp_dfault_rcm(&pinfo->rcm, &pinfo->std, data, buflen);
    } else {
      if( pinfo->std.sel.active ) {
	write_tempcollect(&pinfo->sav, (byte_t *)data, buflen);
      }
      rm_dfault_rcm(&pinfo->rcm, &pinfo->std, data, buflen);
    }

  }
  return PARSER_OK;
}

bool_t start_file_fun(void *user, const char_t *file, const char_t **xpaths) {
  parserinfo_mv_t *pinfo = (parserinfo_mv_t *)user;
  if( pinfo ) {

    if( checkflag(pinfo->flags,MV_FLAG_TARGET) ) {

      if( (strcmp(file, "stdin") == 0) && 
	  checkflag(pinfo->rcm.flags, RCM_WRITE_FILES) ) {
	errormsg(E_WARNING, "cannot write to stdin, ignoring this file.\n");
	return FALSE;
      }
      cp_start_target_rcm(&pinfo->rcm, file);

    } else {

      if( strcmp(file, "stdout") == 0 ) {
	if( checkflag(pinfo->flags, MV_FLAG_SEEN_STDOUT) ) {
	  if( false_and_setflag(&pinfo->flags, MV_FLAG_WARN_STDOUT) ) {
	    errormsg(E_WARNING, 
		     "only one stdout target allowed, ignoring remaining.\n");
	  }
	}
	setflag(&pinfo->flags, MV_FLAG_SEEN_STDOUT);
      }
    
      if( checkflag(pinfo->rcm.flags, RCM_WRITE_FILES) ) {

	if( strcmp(file, "stdin") == 0 ) {
	  errormsg(E_WARNING, "cannot write to stdin, ignoring this file.\n");
	  return FALSE;
	}

      } else {

	/* this only occurs the first time the target is seen */
	if( (strcmp(file, pinfo->target) == 0) && 
	    pinfo->tempfile && (pinfo->tempfd != -1) ) {
	  open_redirect_stdout(pinfo->tempfd);
	  setflag(&pinfo->rcm.flags, RCM_RM_OUTPUT);
	}

      }
      rm_start_file_rcm(&pinfo->rcm, file);

    }
    return TRUE;
  }
  return FALSE;
}

bool_t end_file_fun(void *user, const char_t *file, const char_t **xpaths) {
  parserinfo_mv_t *pinfo = (parserinfo_mv_t *)user;
  if( pinfo ) {

    if( checkflag(pinfo->flags,MV_FLAG_TARGET) ) {

      cp_end_target_rcm(&pinfo->rcm, file);

    } else {

      rm_end_file_rcm(&pinfo->rcm, file);

      if( !checkflag(pinfo->rcm.flags, RCM_WRITE_FILES) ) {
	/* this only occurs the first time the target is seen */
	if( (strcmp(file, pinfo->target) == 0) && pinfo->tempfile ) {
	  close(pinfo->tempfd);
	  pinfo->tempfd = -1;
	  clearflag(&pinfo->rcm.flags, RCM_RM_OUTPUT);
	}
      }
    }

    return TRUE;
  }
  return FALSE;
}

bool_t create_parserinfo_mv(parserinfo_mv_t *pinfo) {

  bool_t ok = TRUE;
  if( pinfo ) {
    memset(pinfo, 0, sizeof(parserinfo_mv_t));
    ok &= create_stdparserinfo(&pinfo->std);
    ok &= create_rcm(&pinfo->rcm);
    ok &= create_tempcollect(&pinfo->sav, "sav", MINVARSIZE, MAXVARSIZE);

    if( ok ) {

      pinfo->std.setup.flags = STDPARSE_ALLNODES|STDPARSE_ALWAYS_CHARDATA;
      pinfo->std.setup.cb.start_tag = start_tag;
      pinfo->std.setup.cb.end_tag = end_tag;
      pinfo->std.setup.cb.chardata = chardata;
      pinfo->std.setup.cb.dfault = dfault;

      pinfo->std.setup.start_file_fun = start_file_fun;
      pinfo->std.setup.end_file_fun = end_file_fun;

      pinfo->flags = 0;
      
      pinfo->tempfile = NULL;
      pinfo->tempfd = -1;
    }
    return ok;
  }
  return FALSE;
}

bool_t reinit_parserinfo_mv(parserinfo_mv_t *pinfo) {
  if( pinfo ) {

    insert_rcm(&pinfo->rcm, &pinfo->sav);
    setflag(&pinfo->flags, MV_FLAG_TARGET);	
    
    return TRUE;
  }
  return FALSE;
}


bool_t free_parserinfo_mv(parserinfo_mv_t *pinfo) {
  if( pinfo ) {
    free_stdparserinfo(&pinfo->std);
    free_rcm(&pinfo->rcm);
    free_tempcollect(&pinfo->sav);

    if( pinfo->tempfile ) {
      free(pinfo->tempfile);
      pinfo->tempfile = NULL;
      if( pinfo->tempfd != -1 ) {
	close(pinfo->tempfd);
	pinfo->tempfd = -1;
      }
    }
    return TRUE;
  }
  return FALSE;

}

int main(int argc, char **argv) {
  signed char op;
  parserinfo_mv_t pinfo;
  filelist_t fl;

  struct option longopts[] = {
    { "version", 0, NULL, MV_VERSION },
    { "help", 0, NULL, MV_HELP },
    { "write-files", 0, NULL, MV_FILES },
    { "prepend", 0, NULL, MV_PREPEND },
    { "replace", 0, NULL, MV_REPLACE },
    { "append", 0, NULL, MV_APPEND },
    { 0 }
  };

  progname = "xml-mv";
  inputfile = "";
  inputline = 0;

  if( create_parserinfo_mv(&pinfo) ) {

    while( (op = getopt_long(argc, argv, "",
			     longopts, NULL)) > -1 ) {
      set_option_mv(op, optarg, &pinfo);
    }

    init_signal_handling(SIGNALS_DEFAULT);
    init_file_handling();
    init_tempfile_handling();
    init_rollback_handling();

    if( !checkflag(pinfo.rcm.flags,RCM_CP_PREPEND|RCM_CP_APPEND) ) {
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

      pinfo.target = pinfo.files[pinfo.n - 1];
      pinfo.tpaths = pinfo.xpaths[pinfo.n - 1];

      if( !checkflag(pinfo.rcm.flags,RCM_WRITE_FILES) ) {
	/* When --write-files is not selected, we cannot write
	 * to the files on the command line. So we use the following
	 * trick to implement moves within the target file (last on command
	 * line): we replace all instances of the target with a common
	 * tempfile, which we can write to as required. The first instance
	 * of the target is not replaced, since it must be read (to copy
	 * its contents into the tempfile).
	 */
	substitute_target(&pinfo);
      }

      if( stdparse2(pinfo.n - 1, pinfo.files, pinfo.xpaths, &pinfo.std) ) {

	if( reinit_parserinfo_mv(&pinfo) ) {
	  /* output always to stdout */
	  setflag(&pinfo.rcm.flags, RCM_CP_OUTPUT);
	  clearflag(&pinfo.rcm.flags, RCM_RM_OUTPUT);

	  stdparse2(1, &pinfo.files[pinfo.n - 1], 
		    &pinfo.xpaths[pinfo.n - 1], &pinfo.std);

	}

      }

      free_filelist(&fl);
    }

    exit_rollback_handling();
    exit_tempfile_handling();
    exit_file_handling();      
    exit_signal_handling();

    free_parserinfo_mv(&pinfo);
  }

  return EXIT_SUCCESS;
}
