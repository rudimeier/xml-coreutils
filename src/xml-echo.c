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
#include "io.h"
#include "wrap.h"
#include "myerror.h"
#include "xpath.h"
#include "mem.h"
#include "entities.h"
#include "format.h"
#include "stdout.h"
#include "echo.h"
#include "tempvar.h"

#include <string.h>
#include <getopt.h>

/* for option processing */
extern char *optarg;
extern int optind, opterr, optopt;

extern char *progname;
extern char *inputfile;
extern long inputline;

extern volatile flag_t cmd;

#define U_OPT_ECHO_INTERPRET 0x01
#define U_OPT_ECHO_NOINDENT  0x02

flag_t u_options = 0;

#include <stdio.h>

#define INDENTALL   INFDEPTH
typedef struct {
  echo_t e;
  tempvar_t res;
} echoinfo_t;

#define ECHO_VERSION    0x01
#define ECHO_HELP       0x02
#define ECHO_XPATHSEP   0x03
#define ECHO_USAGE0 \
"Usage: xml-echo [OPTION]... [STRING]...\n" \
"Echo an XML document to standard output.\n" \
"\n" \
"  -e            enable interpretation of control information\n" \
"      --help    display this help and exit\n" \
"      --version display version information and exit\n"

#define ECHO_USAGE1 \
"If -e is in effect, the following escape sequences are recognized:\n" \
"  \\\\   backslash\n" \
"  \\n   new line\n" \
"  \\t   tab\n" \
"  \\i   hierarchically indent following (default)\n" \
"  \\c   linearly output following\n" \
"  In addition, STRING can contain \"[PATH]\", where PATH is an absolute\n" \
"  or relative node path. This has the effect of outputting the corresponding\n" \
"  XML tags.\n"

int set_option(int op, char *optarg) {
  int c = 0;
  switch(op) {
  case 'E':
    clearflag(&u_options,U_OPT_ECHO_INTERPRET);
    break;
  case 'e':
    setflag(&u_options,U_OPT_ECHO_INTERPRET);
    break;
  case 'n':
    setflag(&u_options,U_OPT_ECHO_NOINDENT);
    break;
  case ECHO_VERSION:
    puts("xml-echo" COPYBLURB);
    exit(EXIT_SUCCESS);
    break;
  case ECHO_HELP:
    puts(ECHO_USAGE0);
    puts(ECHO_USAGE1);
    exit(EXIT_SUCCESS);
    break;
  case ECHO_XPATHSEP:
    redefine_xpath_specials(optarg);
    break;
  }
  return c;
}

void echo_interpreted(echoinfo_t *ei, const char_t *s) {
  if( ei ) {
    puts_stdout_echo(&ei->e, s);
  }
}

void echo_simple(const char_t *s) {
  if( s ) {
    puts_stdout(s);
  }
}

bool_t create_echoinfo(echoinfo_t *ei) {
  bool_t ok = TRUE;
  int indent;
  if( ei ) {
    indent = (checkflag(u_options,U_OPT_ECHO_NOINDENT) ? 0 : ECHO_INDENTALL);
    ok &= create_echo(&ei->e, indent, ECHO_FLAG_SUPPRESSNL);
    ok &= create_tempvar(&ei->res, "robust-echo-string",
			 MINVARSIZE, MAXVARSIZE);
    return ok;
  }
  return FALSE;
}

bool_t free_echoinfo(echoinfo_t *ei) {
  if( ei ) {
    free_echo(&ei->e);
    free_tempvar(&ei->res);
  }
  return TRUE;
}

int main(int argc, char **argv) {
  signed char op;
  echoinfo_t einfo;

  struct option longopts[] = {
    { "version", 0, NULL, ECHO_VERSION },
    { "help", 0, NULL, ECHO_HELP },
    { "path-separator", 1, NULL, ECHO_XPATHSEP },
    { 0 }
  };

  progname = "xml-echo";
  inputfile = "";
  inputline = 0;
  

  while( (op = getopt_long(argc, argv, "eEn",
			   longopts, NULL)) > -1 ) {
    set_option(op, optarg);
  }

  if( create_echoinfo(&einfo) ) {

    open_stdout();
    /* this runs an XML parser on anything we output, to 
       make sure it is well formed XML. This is simpler
       than the alternative, which consists of checking
       all input before we write it to STDOUT, e.g. checking
       that tag names follow the XML grammar, that chardata 
       doesn't contain prohibited chars etc. */
    setup_stdout(STDOUT_CHECKPARSER);

    puts_stdout(get_headwrap());

    if( (optind < 0) || !argv[optind] ) {
      open_stdout_echo(&einfo.e, get_root_tag());
      echo_simple(""); /* dummy */
      close_stdout_echo(&einfo.e);
    } else {
      if( checkflag(u_options,U_OPT_ECHO_INTERPRET) ) {

	/* In principle we could run echo_interpreted() 
	   on each argument in turn, but this causes 
	   a subtle lack of robustness: 

	   Suppose the command line is intended to be 
	   "[a@b=c d/e]x". This should normally be 
	   interpreted as <a b="c d"><e>x</e></a>. 
	   However, if the shell splits it on the space,
	   then xml-echo will see the two strings
	   "[a@b=c" and "d/e]x". If we call 
	   echo_interpreted() twice, then
	   this will not give the correct result.

	   Although it should be the responsibility
	   of the user to quote protect all strings correctly,
	   it is more robust if we concatenate all the 
	   strings together here ourselves.
	*/

	while(!checkflag(cmd,CMD_QUIT) && argv[optind] ) {
	  if( !is_empty_tempvar(&einfo.res) ) {
	    /* insert and immediately remove a space */
	    puts_tempvar(&einfo.res, " \\b");
	  }
	  if( *argv[optind] && !puts_tempvar(&einfo.res, argv[optind]) ) {
	    errormsg(E_FATAL, "string too long.\n");
	  }
	  optind++;
	}
	/* root tag may be implicit in echo-string */
	echo_interpreted(&einfo, string_tempvar(&einfo.res));
	close_stdout_echo(&einfo.e);

      } else {

	open_stdout_echo(&einfo.e, get_root_tag());
	while(!checkflag(cmd,CMD_QUIT) && argv[optind] ) {
	  echo_simple(checkflag(u_options,U_OPT_ECHO_NOINDENT) ? "" : "\n");
	  echo_simple(argv[optind]);
	  optind++;
	}
	close_stdout_echo(&einfo.e);

      }
    }

    puts_stdout(get_footwrap());

    close_stdout();

    free_echoinfo(&einfo);
  }
  return EXIT_SUCCESS;
}
