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
#include "leafparse.h"
#include "entities.h"
#include "unecho.h"
#include "filelist.h"
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

flag_t u_options = 0;

#include <stdio.h>

typedef struct {
  leafparserinfo_t lfp;
  flag_t flags;

  cstringlst_t files;
  cstringlst_t *xpaths;
  int n;

} parserinfo_unecho_t;

#define UNECHO_VERSION    0x01
#define UNECHO_HELP       0x02
#define UNECHO_XPATHSEP   0x03
#define UNECHO_XMLSED     0x04
#define UNECHO_ATTRIBUTES 0x05
#define UNECHO_USAGE \
"Usage: xml-unecho [OPTION]... [[FILE] [:XPATH]...]\n" \
"For each leaf node in FILE or STDIN, print a corresponding xml-echo line.\n" \
"\n" \
"      --help     display this help and exit\n" \
"      --version  display version information and exit\n"

#define UNECHO_FLAG_SED   0x01

void set_option_unecho(int op, char *optarg) {
  switch(op) {
  case UNECHO_VERSION:
    puts("xml-unecho" COPYBLURB);
    exit(EXIT_SUCCESS);
    break;
  case UNECHO_HELP:
    puts(UNECHO_USAGE);
    exit(EXIT_SUCCESS);
    break;
  case UNECHO_XPATHSEP:
    redefine_xpath_specials(optarg);
    break;
  case UNECHO_XMLSED:
    setflag(&u_options,UNECHO_FLAG_SED);
    break;
  }
}

/* bool_t do_write_stdout_unecho(void *user, byte_t *buf, size_t buflen) { */
/*   unecho_t *ue = (unecho_t *)user; */
/*   if( ue ) { */
/*     write_stdout_tempcollect((tempcollect_t *)&ue->sv); */
/*     return TRUE; */
/*   } */
/*   return FALSE; */
/* } */

result_t do_leaf_node(void *user, unecho_t *ue) {

  parserinfo_unecho_t *pinfo = (parserinfo_unecho_t *)user;

  if( pinfo && ue ) {

      write_stdout_tempcollect((tempcollect_t*)&ue->sv);

      if( checkflag(pinfo->flags,UNECHO_FLAG_SED) ) {
	putc_stdout('\n'); /* pretty print */
      }

  }
  return PARSER_OK;
}

bool_t create_parserinfo_unecho(parserinfo_unecho_t *pinfo) {
  bool_t ok = TRUE;
  if( pinfo ) {

    memset(pinfo, 0, sizeof(parserinfo_unecho_t));
    ok &= create_leafparserinfo(&pinfo->lfp);

    pinfo->flags = u_options;
    pinfo->lfp.setup.flags = 0;
    pinfo->lfp.setup.cb.leaf_node = do_leaf_node;

    if( checkflag(pinfo->flags,UNECHO_FLAG_SED) ) {
      setflag(&pinfo->lfp.setup.flags,LFP_ABSOLUTE_PATH);
    }

    setflag(&pinfo->lfp.setup.flags,LFP_PRE_OPEN);
    setflag(&pinfo->lfp.setup.flags,LFP_PRE_CLOSE);
    clearflag(&pinfo->lfp.setup.flags,LFP_POST_OPEN);
    clearflag(&pinfo->lfp.setup.flags,LFP_POST_CLOSE);

    return TRUE;
  }
  return FALSE;
}

bool_t free_parserinfo_unecho(parserinfo_unecho_t *pinfo) {
  free_leafparserinfo(&pinfo->lfp);
  return TRUE;
}

int main(int argc, char **argv) {
  signed char op;
  parserinfo_unecho_t pinfo;
  filelist_t fl;

  struct option longopts[] = {
    { "version", 0, NULL, UNECHO_VERSION },
    { "help", 0, NULL, UNECHO_HELP },
    { "xpath-separator", 1, NULL, UNECHO_XPATHSEP },
    { "xml-sed", 0, NULL, UNECHO_XMLSED },
    { 0 }
  };

  progname = "xml-unecho";
  inputfile = "";
  inputline = 0;

  while( (op = getopt_long(argc, argv, "",
			   longopts, NULL)) > -1 ) {
    set_option_unecho(op, optarg);
  }

  if( create_parserinfo_unecho(&pinfo) ) {

    init_signal_handling();
    init_file_handling();

    if( create_filelist(&fl, MAXFILES, argv + optind, FILELIST_EQ1) ) {

      pinfo.files = getfiles_filelist(&fl);
      pinfo.xpaths = getxpaths_filelist(&fl);
      pinfo.n = getsize_filelist(&fl);
      
      inputfile = (char *)pinfo.files[0];

      open_stdout();
      leafparse(pinfo.files[0], pinfo.xpaths[0], (leafparserinfo_t *)&pinfo);
      close_stdout();

      free_filelist(&fl);
    }    

    exit_file_handling();
    exit_signal_handling();

    free_parserinfo_unecho(&pinfo);
  }

  return EXIT_SUCCESS;
}
