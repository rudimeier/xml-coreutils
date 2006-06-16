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
#include "parser.h"
#include "io.h"
#include "mem.h"
#include "error.h"
#include "xpath.h"
#include "xmatch.h"
#include "tempcollect.h"
#include "stringlist.h"
#include "format.h"
#include "stdout.h"

#include <string.h>
#include <getopt.h>

/* for option processing */
extern char *optarg;
extern int optind, opterr, optopt;

extern char *progname;
extern char *inputfile;
extern long inputline;

extern int cmd;

#include <stdio.h>

typedef struct {
  tempcollect_t *tc;
  bool_t exists;
  bool_t active;
  unsigned int mindepth;
} argstatus_t;

typedef struct {
  argstatus_t *status;
  size_t count;
  size_t max;
  size_t mark;
} printf_args_t;

typedef struct {
  unsigned int depth;
  unsigned int maxdepth;
  xpath_t cp;
  xmatcher_t xm;
  const char_t *format;
  stringlist_t files;
  tclist_t collectors;
  printf_args_t args;
} parserinfo_printf_t;

#define PRINTF_VERSION    0x01
#define PRINTF_HELP       0x02
#define PRINTF_USAGE0 \
"Usage: xml-printf FORMAT [[FILE]:XPATH]...\n" \
"Print the text value of XPATH(s) according to FORMAT.\n" \
"\n" \
"      --help     display this help and exit\n" \
"      --version  display version information and exit\n" \
"This command prints the string FORMAT only, but FORMAT can contain\n" \
"C printf style formatting instructions which specify how subsequent\n" \
"arguments are converted for output.\n"

#define PRINTF_USAGE1 \
"Each subsequent argument represents the text value of an XML subtree,\n" \
"located in the XML document FILE and given by XPATH. If FILE is omitted,\n" \
"it is assumed that standard input contains a well formed XML document and\n" \
"XPATH is a path within that document.\n" \
"SOURCE is an optional XML file path, and XPATH represents a node relative\n" \
"to SOURCE, or if omitted, relative to the XML document in standard input.\n" \
"Only the text value of each XPATH is substituted.\n"

void set_option(int op, char *optarg) {
  switch(op) {
  case PRINTF_VERSION:
    puts("xml-printf" COPYBLURB);
    exit(EXIT_SUCCESS);
    break;
  case PRINTF_HELP:
    puts(PRINTF_USAGE0);
    puts(PRINTF_USAGE1);
    exit(EXIT_SUCCESS);
    break;
  }
}


bool_t create_printf_args(printf_args_t *pargs) {
  pargs->count = 0;
  return create_mem((byte_t **)&pargs->status, &pargs->max, sizeof(argstatus_t), 16);      
}

void add_printf_args(printf_args_t *pargs, tempcollect_t *tc) {
  if( pargs->count >= pargs->max ) {
    grow_mem((byte_t **)&pargs->status, &pargs->max, sizeof(argstatus_t), 16); 
  }
  if( pargs->count < pargs->max ) {
    pargs->status[pargs->count].tc = tc;
    pargs->status[pargs->count].exists = FALSE;
    pargs->status[pargs->count].active = FALSE;
    pargs->count++;
  }  
}

bool_t find_printf_args(printf_args_t *pargs, const char_t *path, size_t *n) {
  size_t i;
  const char_t *p;
  char_t *q;
  char_t delim = ':';
  if( pargs && path && n) {
    for(i = pargs->mark; i < pargs->count; i++) {
      p = pargs->status[i].tc->name;
      q = strchr(p, delim);
      if( q ) {
	q++;
	if( strcmp(path, q) == 0) {
	  *n = i;
	  return TRUE;
	}
      }
    }
  }
  return FALSE;
}


bool_t free_printf_args(printf_args_t *pargs) {
  pargs->count = 0;
  return free_mem((byte_t **)&pargs->status, &pargs->max);
}

bool_t init_parserinfo_printf(parserinfo_printf_t *pinfo) {
  if( pinfo ) {
    memset(pinfo, 0, sizeof(parserinfo_printf_t));
    if( create_xpath(&pinfo->cp) && 
	create_xmatcher(&pinfo->xm) &&
	create_stringlist(&pinfo->files) && 
	create_tclist(&pinfo->collectors) &&
	create_printf_args(&pinfo->args) ) {
      return TRUE;
    }
  }
  return FALSE;
}

bool_t free_parserinfo_printf(parserinfo_printf_t *pinfo) {
  if( pinfo ) {
    free_xpath(&pinfo->cp);
    free_xmatcher(&pinfo->xm);
    free_stringlist(&pinfo->files);
    free_printf_args(&pinfo->args);
    free_tclist(&pinfo->collectors);
    return TRUE;
  }
  return FALSE;
}

bool_t optimize_args(parserinfo_printf_t *pinfo, int n, int argc, char **argv) {
  char_t *file;
  char_t *path;
  char_t delim = ':';
  if( pinfo && argv[n] ) {
    /* argc - n has enough room for all files or paths + 1 */
    pinfo->format = argv[n++];
    while( argv[n] ) {
      file = argv[n];
      path = strchr(file, delim);
      if( path ) { *path = '\0'; }
      if( !*file ) { file = "stdin"; }
      add_unique_stringlist(&pinfo->files, file);
      if( path ) { *path = delim; }
      n++;
    }
    return TRUE;
  }
  return FALSE;
}

void prepare_matcher(parserinfo_printf_t *pinfo, const char *inputfile, 
		   int n, int argc, char **argv) {
  char_t *file;
  char_t *path;
  char_t delim = ':';
  tempcollect_t *tc;
  if( pinfo && inputfile ) {
    reset_xmatcher(&pinfo->xm);
    /* anything below pinfo->collectors.num is for another file */
    pinfo->args.mark = pinfo->collectors.num;
    while( argv[n] ) {
      file = argv[n];
      path = strchr(file, delim);
      if( ((path == file) && (strcmp(inputfile, "stdin") == 0)) ||
	  (strncmp(file, inputfile, strlen(inputfile)) == 0) ) {
	if( path ) { 
	  path++; /* skip delim */
	  if( push_unique_xmatcher(&pinfo->xm, path) ) {
	    if( add_tclist(&pinfo->collectors, argv[n], 64, 1024*1024) ) { 
	      tc = get_last_tclist(&pinfo->collectors);
	      if( tc ) {
		add_printf_args(&pinfo->args, tc);
		continue;
	      }
	    }
	    errormsg(E_FATAL, "couldn't create tempcollector for %s\n", argv[n]);
	    exit(EXIT_FAILURE);
	  }
	}
      }
      n++;
    }
  }
}

void activate_collectors(parserinfo_printf_t *pinfo) {
  size_t n;
  size_t w;
  const char_t *p;

  if( pinfo ) {
    for(w = pinfo->args.mark; w < pinfo->args.count; w++) {
      pinfo->args.status[w].active = FALSE;
    }
    if( find_first_xmatcher(&pinfo->xm, pinfo->cp.path, &n) ) {
      do {
	p = get_xmatcher(&pinfo->xm, n);
	if( p && find_printf_args(&pinfo->args, p, &w) ) {
	  argstatus_t *as = &pinfo->args.status[w];
	  as->exists = TRUE;
	  as->active = TRUE;
	  as->mindepth = pinfo->depth;
/* 	  printf("<path=%s, %s active=%d>\n",  */
/* 		 pinfo->cp.path, */
/* 		 pinfo->args.status[w].tc->name, */
/* 		 pinfo->args.status[w].active); */
	}
      } while( find_next_xmatcher(&pinfo->xm, pinfo->cp.path, &n) );
    }
  }
}


void write_active_collectors(printf_args_t *pargs, 
			     unsigned int depth, byte_t *buf, size_t buflen) {
  size_t i;
  for(i = pargs->mark; i < pargs->count; i++) {
    argstatus_t *as = &pargs->status[i];
    if( as->active && depth == as->mindepth) {
      write_tempcollect(as->tc, buf, buflen);
    }
  }
}

result_t start_tag(void *user, const char_t *name, const char_t **att) {
  parserinfo_printf_t *pinfo = (parserinfo_printf_t *)user;
  if( pinfo ) {
    pinfo->depth++;
    pinfo->maxdepth = MAX(pinfo->depth, pinfo->maxdepth);
    push_tag_xpath(&pinfo->cp, name);
    activate_collectors(pinfo);
  }
  return PARSER_OK;
}

result_t end_tag(void *user, const char_t *name) {
  parserinfo_printf_t *pinfo = (parserinfo_printf_t *)user;
  if( pinfo ) {
    pinfo->depth--;
    pop_xpath(&pinfo->cp);
    activate_collectors(pinfo);
  }
  return PARSER_OK;
}

result_t chardata(void *user, const char_t *buf, size_t buflen) {
  parserinfo_printf_t *pinfo = (parserinfo_printf_t *)user;
  if( pinfo ) {
    write_active_collectors(&pinfo->args, pinfo->depth, (byte_t *)buf, buflen);
  }
  return PARSER_OK;
}

result_t procdata(void *user, const char_t *target, const char_t *data) {
  /* just ignore */
  return PARSER_OK;
}

result_t dfault(void *user, const char_t *data, size_t buflen) {
/*   parserinfo_printf_t *pinfo = (parserinfo_printf_t *)user; */
/*   if( pinfo ) {  */
/*     if( 0 < pinfo->depth ) { */
/*       write_stdout((byte_t *)data, buflen); */
/*     } */
/*   } */
  return PARSER_OK;
}

bool_t check_printf_format(const char_t *format, int n) {
  if( format ) {
    while(*format) {
      if( *format == '%' ) {
	format++;
	n--;
	if( *format == '%' ) {
	  n++;
	}
	if( !*format ) {
	  break;
	}
      }
      format++;
    }
    return (n == 0);
  }
  return FALSE;
}

bool_t write_printf_format(parserinfo_printf_t *pinfo, int n, int argc, char **argv) {
  size_t i;
  tempcollect_t *tc;
  bool_t ok = TRUE;
  const char_t *p;
  char_t *q;
  for(i = 0; i < pinfo->args.count; i++) {
    if( !pinfo->args.status[i].exists ) {
      errormsg(E_ERROR, "node missing in input %s\n", 
	       pinfo->args.status[i].tc->name);
      ok = FALSE;
    }
  }
  if( ok ) {
    p = pinfo->format;
    q = strpbrk(p, "%\\");
    i = n + 1; 
    while( q ) {
      write_stdout((byte_t *)p, q - p);
      p = q + 1;
      switch(*q) {
      case '%':
	switch(*p) {
	case '%':
	  puts_stdout("%");
	  break;
	case 's':
	  tc = find_byname_tclist(&pinfo->collectors, argv[i]);
	  if( !tc ) {
	    errormsg(E_FATAL, "cannot find data for %s\n", argv[i]);
	  }
	  squeeze_stdout_tempcollect(tc);
	  i++;
	  break;
	default:
	  errormsg(E_FATAL, "format specifier %%%c not supported\n", *p);
	  break;
	}
	break;
      case '\\':
	putc_stdout(convert_backslash(q));
	break;
      }
      p++;
      q = strpbrk(p, "%\\");
    }
    puts_stdout(p);
    return TRUE;
  }
  return FALSE;
}

int main(int argc, char **argv) {
  parser_t parser;
  stream_t strm;
  signed char op;
  callback_t callbacks;
  parserinfo_printf_t pinfo;
  int n;
  int exit_value = EXIT_FAILURE;
  struct option longopts[] = {
    { "version", 0, NULL, PRINTF_VERSION }
  };

  progname = "xml-printf";
  inputfile = "stdin";
  inputline = 0;
  
  while( (op = getopt_long(argc, argv, "",
			   longopts, NULL)) > -1 ) {
    set_option(op, optarg);
  }

  init_file_handling();

  if( create_parser(&parser, &pinfo) ) {

    memset(&callbacks, 0, sizeof(callback_t));
    callbacks.start_tag = start_tag;
    callbacks.end_tag = end_tag;
    callbacks.chardata = chardata;
    callbacks.pidata = NULL; /* procdata; */
    callbacks.comment = NULL;
    callbacks.dfault = NULL; /* dfault; */
    setup_parser(&parser, &callbacks);

    init_parserinfo_printf(&pinfo);
    optimize_args(&pinfo, optind, argc, argv);
    if( !check_printf_format(pinfo.format, argc - optind - 1) ) {
      errormsg(E_FATAL, "wrong format string\n");
    }

    n = 0;
    while( !checkflag(cmd,CMD_QUIT) ) {
      inputfile = (char_t *)get_stringlist(&pinfo.files, n);

      if( inputfile && open_file_stream(&strm, inputfile) ) {

	prepare_matcher(&pinfo, inputfile, optind, argc, argv);

	while( !checkflag(cmd,CMD_QUIT) && 
	       read_stream(&strm, getbuf_parser(&parser, strm.blksize), strm.blksize) ) {

	  if( !do_parser(&parser, strm.buflen) ) {
	    if( (pinfo.depth == 0) && (pinfo.maxdepth > 0) ) {
	      /* we're done */
	      break;
	    } 
	    errormsg(E_FATAL, 
		     "invalid XML at line %d, column %d (byte %ld, depth %d) of %s\n",
		     parser.cur.lineno, parser.cur.colno, 
		     parser.cur.byteno, pinfo.depth, inputfile);
	  }

	}

	close_stream(&strm);
      } else {
	break;
      }
      n++;
      reset_parser(&parser);
    }

    open_stdout();
    if( write_printf_format(&pinfo, optind, argc, argv) ) {
      exit_value = EXIT_SUCCESS;
    }
    close_stdout();


    free_xmatcher(&pinfo.xm);
    free_parser(&parser);
    free_parserinfo_printf(&pinfo);
  }

  exit_file_handling();

  return exit_value;
}
