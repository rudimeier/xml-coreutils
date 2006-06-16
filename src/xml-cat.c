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
#include "error.h"
#include "wrap.h"
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

typedef enum {
  na = 0, plaintext, xml, partmatch
} chunktype_t;

typedef struct {
  byte_t *buf;
  size_t buflen;
  unsigned long skipcount;
  chunktype_t buftype;
} state_t;

typedef struct {
  unsigned int depth; /* 0 means prolog, 1 means first tag, etc. */
  unsigned int maxdepth;
  char_t *headwrap;
  char_t *footwrap;
} parserinfo_cat_t;

#define CAT_VERSION    0x01
#define CAT_HELP       0x02
#define CAT_USAGE \
"Usage: xml-cat [OPTION] [FILE]...\n" \
"Concatenate the XML contents of FILE(s), or standard input,\n" \
"into a single well formed XML document on standard output.\n" \
"\n" \
"      --help     display this help and exit\n" \
"      --version  display version information and exit\n"

void set_option(int op, char *optarg) {
  switch(op) {
  case CAT_VERSION:
    puts("xml-cat" COPYBLURB);
    exit(EXIT_SUCCESS);
    break;
  case CAT_HELP:
    puts(CAT_USAGE);
    exit(EXIT_SUCCESS);
    break;
  }
}

void output_wrapper_start(parserinfo_cat_t *pinfo) {
  open_stdout();
  if( pinfo ) {
    pinfo->headwrap = get_headwrap();
  }
}

void output_wrapper_end(parserinfo_cat_t *pinfo) {
  if( pinfo && pinfo->footwrap ) {
    write_stdout((byte_t *)pinfo->footwrap, strlen(pinfo->footwrap));
  }
  close_stdout();
}

bool_t get_next_chunk(stream_t *strm, parser_t *parser, state_t *state) {
  byte_t delim = '<';
  byte_t *p;

  if( state && strm && strm->buf && 
      (strm->pos < strm->buf + strm->buflen) ) {
    switch(state->buftype) {
    case na:
      state->buf = NULL;
      state->buflen = 0;
      state->skipcount = 0;
      /* fall through */
    case plaintext:
      p = memchr(strm->pos, delim, strm->buf + strm->buflen - strm->pos);
      if( p ) {
	state->buf = p;
	state->buflen = strm->buf + strm->buflen - p;
	state->buftype = xml;
	state->skipcount += (p - strm->buf);
	strm->pos = strm->buf + strm->buflen;
	return TRUE;
      }
      strm->pos = strm->buf + strm->buflen;
      state->buf = strm->pos;
      state->buflen = 0;
      return FALSE;
    case xml:
      state->buf = strm->buf;
      state->buflen = strm->buflen;
      strm->pos = strm->buf + state->buflen;
      return TRUE;
    case partmatch:
      /* nothing */
      break;
    }
  }
  state->buf = NULL;
  state->buflen = -1;
  state->skipcount = 0;
  return FALSE;
}

bool_t shift_reset_chunk(stream_t *strm, parser_t *parser, 
			 parserinfo_cat_t *pinfo, state_t *state, size_t bytesleft) {
  if( strm && parser && state && (bytesleft > 0) && 
      (bytesleft <= (strm->pos - strm->buf)) ) {
    reset_parser(parser);
    strm->pos -= bytesleft;
    state->buftype = plaintext;
    state->skipcount = (strm->bytesread - bytesleft);
    pinfo->depth = 0;
    return TRUE;
  }
  return FALSE;
}

result_t start_tag(void *user, const char_t *name, const char_t **att) {
  parserinfo_cat_t *pinfo = (parserinfo_cat_t *)user;
  if( pinfo ) { 
    if( 1 == ++pinfo->depth ) {
      if( pinfo->headwrap ) {
	write_stdout((byte_t *)pinfo->headwrap, strlen(pinfo->headwrap));
      }
      pinfo->maxdepth = MAX(pinfo->depth, pinfo->maxdepth);
      return PARSER_OK;
    }
  }
  return (PARSER_OK|PARSER_DEFAULT);
}

result_t end_tag(void *user, const char_t *name) {
  parserinfo_cat_t *pinfo = (parserinfo_cat_t *)user;
  if( pinfo ) { 
    if( 1 == pinfo->depth-- ) {
      if( pinfo->headwrap ) {
	pinfo->headwrap = NULL;
	pinfo->footwrap = get_footwrap();
      }
      return PARSER_OK;
    }
  }
  return (PARSER_OK|PARSER_DEFAULT);
}

result_t chardata(void *user, const char_t *buf, size_t buflen) {
  return PARSER_OK;
}



result_t procdata(void *user, const char_t *target, const char_t *data) {
  /* just ignore */
  return PARSER_OK;
}

result_t dfault(void *user, const char_t *data, size_t buflen) {
  parserinfo_cat_t *pinfo = (parserinfo_cat_t *)user;
  if( pinfo ) { 
    if( 0 < pinfo->depth ) {
      write_stdout((byte_t *)data, buflen);
    }
  }
  return PARSER_OK;
}

int main(int argc, char **argv) {
  parser_t parser;
  stream_t strm;
  signed char op;
  state_t state;
  callback_t callbacks;
  parserinfo_cat_t pinfo;
  size_t bytesleft; 
  struct option longopts[] = {
    { "version", 0, NULL, CAT_VERSION },
    { "help", 0, NULL, CAT_HELP },
  };

  progname = "xml-cat";
  inputfile = "stdin";
  inputline = 0;

  while( (op = getopt_long(argc, argv, "",
			   longopts, NULL)) > -1 ) {
    set_option(op, optarg);
  }

  init_file_handling();

  memset(&pinfo, 0, sizeof(parserinfo_cat_t));
  if( create_parser(&parser, &pinfo) ) {

    memset(&callbacks, 0, sizeof(callback_t));
    callbacks.start_tag = start_tag;
    callbacks.end_tag = end_tag;
    callbacks.chardata = NULL; /* chardata; */
    callbacks.pidata = NULL; /* procdata; */
    callbacks.comment = NULL;
    callbacks.dfault = dfault;
    setup_parser(&parser, &callbacks);

    output_wrapper_start(&pinfo);

    memset(&state, 0, sizeof(state_t));

    while( !checkflag(cmd,CMD_QUIT) ) {
      if( (optind > -1) && argv[optind] ) {
	inputfile = argv[optind];
      }
      if( inputfile && open_file_stream(&strm, inputfile) ) {

	while( !checkflag(cmd,CMD_QUIT) && 
	       read_stream(&strm, getbuf_parser(&parser, strm.blksize), strm.blksize) ) {

	  while( get_next_chunk(&strm, &parser, &state) ) {

	    if( state.buf > strm.buf ) {
	      shift_stream(&strm, state.buf - strm.buf);
	    }
	    if( !do_parser(&parser, state.buflen) ) {
	      if( parser.cur.byteno == 1 ) {
		/* this occurs in plaintext state when we see '<'
		   followed by an invalid XML character, e.g "4 <
		   5", we don't want to parse this */
		bytesleft = state.buflen - 1;
		shift_reset_chunk(&strm, &parser, &pinfo, &state, bytesleft);
		continue;
	      }
	    }
	    if( !ok_parser(&parser) ) {
	      if( (pinfo.depth == 0) && (pinfo.maxdepth > 0) ) {
		/* XML chunk is finished, go back to plain text mode */
		bytesleft = strm.bytesread - parser.cur.byteno;
		shift_reset_chunk(&strm, &parser, &pinfo, &state, bytesleft);
		continue;
	      } 
	      errormsg(E_FATAL, 
		       "invalid XML at line %d, column %d (byte %ld, depth %d) of %s\n",
		       parser.cur.lineno, parser.cur.colno, 
		       state.skipcount + parser.cur.byteno, 
		       pinfo.depth, inputfile);
	    }

	  }
	}

	close_stream(&strm);
      }

      if( !argv[optind] ) {
	break;
      }
      optind++;
      reset_parser(&parser);
    }

    output_wrapper_end(&pinfo);

    free_parser(&parser);
  }

  exit_file_handling();

  return EXIT_SUCCESS;
}
