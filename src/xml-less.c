/* 
 * Copyright (C) 2006 Laird Breyer
 *  
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
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
#include "fbparser.h"
#include "cursor.h"
#include "error.h"
#include "lessui.h"
#include "stdout.h"

#include <stdio.h>
#include <getopt.h>

/* for option processing */
extern char *optarg;
extern int optind, opterr, optopt;

extern char *progname;
extern char *inputfile;
extern long inputline;

extern int cmd;
int u_options = 0;

#define LESS_VERSION    0x01
#define LESS_HELP       0x02
#define LESS_USAGE \
"Usage: xml-less FILE\n" \
"Interactively display the XML document contained in FILE on the terminal.\n"

void set_option(int op, char *optarg) {
  switch(op) {
  case LESS_VERSION:
    puts("xml-less" COPYBLURB);
    exit(EXIT_SUCCESS);
    break;
  case LESS_HELP:
    puts(LESS_USAGE);
    break;
  }
}

int main(int argc, char **argv) {
  signed char op;
  lessui_t ui;
  fbparser_t fbp;
  struct option longopts[] = {
    { "version", 0, NULL, LESS_VERSION },
    { "help", 0, NULL, LESS_HELP },
  };

  progname = "xml-less";
  inputfile = "";
  inputline = 0;
  

  while( (op = getopt_long(argc, argv, "",
			   longopts, NULL)) > -1 ) {
    set_option(op, optarg);
  }

  open_stdout();

  if( create_lessui(&ui) ) {

    inputfile = (optind > -1) ? argv[optind] : NULL;
    if( inputfile && open_fileblockparser(&fbp, inputfile, 32) ) {

      mainloop_lessui(&ui, &fbp);

      close_fileblockparser(&fbp);
    }

    free_lessui(&ui);
  }

  close_stdout();

  return EXIT_SUCCESS;
}
