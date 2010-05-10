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
#include "parser.h"
#include "io.h"
#include "myerror.h"
#include "wrap.h"
#include "stdout.h"
#include "filelist.h"
#include "mysignal.h"
#include "mem.h"

#include <string.h>
#include <getopt.h>

/* for option processing */
extern char *optarg;
extern int optind, opterr, optopt;

extern const char *progname;
extern const char *inputfile;
extern long inputline;

extern volatile flag_t cmd;

#include <stdio.h>

typedef enum {
  na = 0, plaintext, xml
} chunktype_t;

typedef struct {
  chunktype_t ct;
  long startpos;
  long totalbytes;
} skipper_t;

/* this is the state for the <?xml filter */
typedef enum { xna = 0, langle, qmark, ex, em, ell } xdecl_filter_t;

typedef struct {
  /* we don't use stdparse() since the input may not
     start or finish with pure XML markup */
  parser_t parser;
  callback_t callbacks;
  long totalbytes, parsebytes;
  /* we don't use getbuf_parser() so have to manage our own buffer */
  byte_t *xbuf;
  size_t xbuflen;

  unsigned int depth; /* 0 means prolog, 1 means first tag, etc. */
  unsigned int maxdepth;
  const char_t *headwrap;
  const char_t *footwrap;

  /* filtering characters */
  chunktype_t ctype;
  xdecl_filter_t xdf;
} parserinfo_cat_t;

#define CAT_VERSION    0x01
#define CAT_HELP       0x02
#define CAT_USAGE \
"Usage: xml-cat [OPTION]... [FILE]...\n" \
"Concatenate the XML contents of FILE(s), or standard input,\n" \
"into a single well formed XML document on standard output.\n" \
"\n" \
"      --help     display this help and exit\n" \
"      --version  display version information and exit\n"

void set_option_cat(int op, char *optarg) {
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
  if( pinfo ) {
    pinfo->headwrap = get_headwrap();

  }
}

void output_wrapper_end(parserinfo_cat_t *pinfo) {
  if( pinfo && pinfo->footwrap ) {
    puts_stdout(get_close_root());
    write_stdout((byte_t *)pinfo->footwrap, strlen(pinfo->footwrap));
  }
}

/* This filter looks for instances of the string "<?xml" and
 * replaces them with "<?xm_".
 * The filter state transitions one character at a time, because
 * the strm->buf might not contain the full search string.
 * The replacement string is actually a valid XML processing instruction
 * prefix, and has the same number of characters as the prohibited
 * string to minimize the impact of the filter.
 *
 * The reason this filter is necessary is that in well formed XML,
 * the prohibited string "<?xml" is not allowed in the middle of
 * an XML document, but if we xml-cat several strings, this contingency
 * can indeed happen. 
 * We could completely remove the offending declaration from the input,
 * but it is more transparent to leave it in an "inoffensive" form.
 */
void filter_xdecl(stream_t *strm, xdecl_filter_t *filter) {
  byte_t *p, *q;
  xdecl_filter_t fs;
  if( strm && filter ) {
    p = strm->pos;
    q = strm->buf + strm->buflen;
    fs = *filter;
    while( p < q ) {
      switch(fs) {
      case xna:
	fs = (*p == '<') ? langle : xna;
	break;
      case langle:
	fs = (*p == '?') ? qmark : xna;
	break;
      case qmark:
	fs = (*p == 'x') ? ex : xna;
	break;
      case ex:
	fs = (*p == 'm') ? em : xna;
	break;
      case em:
	if( *p == 'l' ) {
	  *p = '_';
	}
	fs = xna;
	break;
      default:
	break;
      }
      p++;
    }
  }
}


result_t start_tag(void *user, const char_t *name, const char_t **att) {
  parserinfo_cat_t *pinfo = (parserinfo_cat_t *)user;
  if( pinfo ) { 
    if( 1 == ++pinfo->depth ) {
      if( pinfo->headwrap ) {
	write_stdout((byte_t *)pinfo->headwrap, strlen(pinfo->headwrap));
	puts_stdout(get_open_root());
      }
      pinfo->maxdepth = MAX(pinfo->depth, pinfo->maxdepth);
      return PARSER_OK; /* don't call dfault() */
    }
  }
  return (PARSER_OK|PARSER_DEFAULT);/* call dfault() */
}

result_t end_tag(void *user, const char_t *name) {
  parserinfo_cat_t *pinfo = (parserinfo_cat_t *)user;
  if( pinfo ) { 
    if( 1 == pinfo->depth-- ) {
      if( pinfo->headwrap ) {
	pinfo->headwrap = NULL;
	pinfo->footwrap = get_footwrap();
      }
    }
  }
  return (PARSER_OK|PARSER_DEFAULT); /* call dfault() */
}

/* this function does most of the work */
result_t dfault(void *user, const char_t *data, size_t buflen) {
  parserinfo_cat_t *pinfo = (parserinfo_cat_t *)user;
  if( pinfo ) { 
    if( 0 < pinfo->depth ) {
      write_stdout((byte_t *)data, buflen);
    }
  }
  return PARSER_OK;
}

bool_t create_parserinfo_cat(parserinfo_cat_t *pinfo) {
  bool_t ok = TRUE;
  if( pinfo ) {
    memset(pinfo, 0, sizeof(parserinfo_cat_t));
    ok &= create_parser(&pinfo->parser, pinfo);
    ok &= create_mem(&pinfo->xbuf, &pinfo->xbuflen, sizeof(byte_t), 4096);

    if( ok ) {
      memset(&pinfo->callbacks, 0, sizeof(callback_t));
      pinfo->callbacks.start_tag = start_tag;
      pinfo->callbacks.end_tag = end_tag;
      pinfo->callbacks.dfault = dfault;
      setup_parser(&pinfo->parser, &pinfo->callbacks);
    }

    return ok;
  }
  return FALSE;
} 

bool_t free_parserinfo_cat(parserinfo_cat_t *pinfo) {
  free_parser(&pinfo->parser);
  free_mem(&pinfo->xbuf, &pinfo->xbuflen);
  return TRUE;
}

/* ensures strm.buf starts with (potential) XML */
void skip_plaintext(parserinfo_cat_t *pinfo, stream_t *strm) {
  const byte_t *p;
  long skipcount = 0;
  if( pinfo && (pinfo->ctype == plaintext) && 
      strm && (strm->buflen > 0) ) {
    p = memchr(strm->buf, '<', strm->buflen);
    if( p && (p > strm->buf) ) {
      skipcount = (p - strm->buf);
      strm->buflen -= skipcount;
      memmove(strm->buf, p, strm->buflen);
      pinfo->ctype = xml;
    } else if( !p ) {
      skipcount = strm->buflen;
      strm->buflen = 0;
    }
    pinfo->parsebytes += skipcount;
  }
}

void reset_parser_pinfo(parserinfo_cat_t *pinfo, stream_t *strm) {
  const byte_t *p;
  long bytesleft;

  if( pinfo && strm ) {
    bytesleft = 
      pinfo->totalbytes - (pinfo->parsebytes + pinfo->parser.cur.byteno);
    pinfo->parsebytes += pinfo->parser.cur.byteno;
    p = strm->buf + (strm->buflen - bytesleft);
    strm->buflen = bytesleft;
    memmove(strm->buf, p, strm->buflen);
    if( !reset_parser(&pinfo->parser) ) {
      errormsg(E_FATAL, "cannot reset parser.\n");
    }
  }
}

int main(int argc, char **argv) {
  stream_t strm;
  bool_t ok = TRUE;
  
  parserinfo_cat_t pinfo;

  signed char op;
  struct option longopts[] = {
    { "version", 0, NULL, CAT_VERSION },
    { "help", 0, NULL, CAT_HELP },
    { 0 }
  };

  filelist_t fl;
  const char **files;
  int f, n;

  progname = "xml-cat";
  inputfile = "";
  inputline = 0;

  while( (op = getopt_long(argc, argv, "",
			   longopts, NULL)) > -1 ) {
    set_option_cat(op, optarg);
  }

  if( create_parserinfo_cat(&pinfo) ) {

    init_signal_handling();
    init_file_handling();

    if( create_filelist(&fl, -1, argv + optind, FILELIST_MIN1) ) {

      if( hasxpaths_filelist(&fl) ) {
	errormsg(E_WARNING, "ignoring XPATH(s) after filename(s).\n");
      }

      open_stdout();
      output_wrapper_start(&pinfo);

      files = getfiles_filelist(&fl);
      n = getsize_filelist(&fl);

      pinfo.totalbytes = 0; /* updated on buffer reads */
      pinfo.parsebytes = 0; /* updated on parser restarts and skips */

      for(f = 0; (f < n) && !checkflag(cmd,CMD_QUIT); f++) {

	inputfile = files[f];

	if( inputfile && open_file_stream(&strm, inputfile) ) {

	  pinfo.xdf = xna;
	  pinfo.ctype = plaintext;
	  while( !checkflag(cmd,CMD_QUIT) && 
		 ensure_bytes_mem(strm.blksize, 
				  &pinfo.xbuf, &pinfo.xbuflen, 
				  sizeof(byte_t)) &&
		 read_stream(&strm, pinfo.xbuf, strm.blksize) ) {

	    filter_xdecl(&strm, &pinfo.xdf);
	    pinfo.totalbytes += strm.buflen;

	    do {

	      skip_plaintext(&pinfo, &strm);
	      if( strm.buflen <= 0 ) break;

	      /* note: below, the parser is reset on various errors.
	       * This doesn't work reliably with do_parser(), so we 
	       * must use do_parser2(), which is slower, but doesn't
	       * segfault at inconvenient times.
	       */
	      ok = do_parser2(&pinfo.parser, strm.buf, strm.buflen);
	      pinfo.ctype = ok ? xml : plaintext;

	      if( !ok ) {
		/* Possible recoverable errors:
		 * 1) in plaintext state when we see '<'
		 * followed by an invalid XML character, e.g "4 <
		 * 5", we don't want to parse this. This only occurs
		 * when byteno = 1.
		 * 2) when an XML document stops (root closing tag
		 * is encountered, and then a new document starts.
		 */
		if( (pinfo.parser.cur.byteno == 1) ||
		    ((pinfo.depth == 0) && (pinfo.maxdepth > 0)) ) {
		  flush_stdout();
		  reset_parser_pinfo(&pinfo, &strm);
		} else {
		  flush_stdout(); /* show user where we are */
		  errormsg(E_FATAL, 
			   "invalid XML at %s (byte %ld, depth %d): %s.\n",
			   inputfile, 
			   pinfo.parsebytes + pinfo.parser.cur.byteno, 
			   pinfo.depth, error_message_parser(&pinfo.parser));
		}

	      }
	    } while( !ok );

	    strm.pos = strm.buf + strm.buflen; /* finished with this buffer */
	  }

	  if( pinfo.depth > 0 ) {
	    errormsg(E_FATAL, 
		     "incomplete XML at byte %ld of %s.\n",
		     pinfo.parsebytes, inputfile);

	  }

	  close_stream(&strm);
	}

	inputfile = NULL;
	reset_parser(&pinfo.parser);
      }

      output_wrapper_end(&pinfo);
      close_stdout();

      free_filelist(&fl);
    }

    exit_file_handling();
    exit_signal_handling();

    free_parserinfo_cat(&pinfo);
  }
  return EXIT_SUCCESS;
}
