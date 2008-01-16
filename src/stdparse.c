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
#include "error.h"
#include "stdparse.h"

#include <sys/types.h>
#include <sys/stat.h>
#include <sys/fcntl.h>
#include <string.h>
#include <unistd.h>

extern char *inputfile;
extern int cmd;

result_t std_start_tag(void *user, const char_t *name, const char_t **att) {
  stdparserinfo_t *pinfo = (stdparserinfo_t *)user;
  if( pinfo ) { 
    pinfo->depth++;
    pinfo->maxdepth = MAX(pinfo->depth, pinfo->maxdepth);
    push_tag_xpath(&pinfo->cp, name);
    pinfo->sel.active = cmp_prefix_xpath(&pinfo->cp, pinfo->sel.path);
    if( pinfo->sel.active ) {
      pinfo->sel.mindepth = MIN(pinfo->depth, pinfo->sel.mindepth);
      pinfo->sel.maxdepth = MAX(pinfo->depth, pinfo->sel.maxdepth);
    }
    return pinfo->setup.cb.start_tag && 
      (pinfo->sel.active || checkflag(pinfo->setup.flags,STDPARSE_ALLNODES)) ? 
      pinfo->setup.cb.start_tag(user, name, att) : PARSER_OK;
  }
  return PARSER_OK;
}

result_t std_end_tag(void *user, const char_t *name) {
  stdparserinfo_t *pinfo = (stdparserinfo_t *)user;
  result_t r;
  if( pinfo ) { 
    r = (pinfo->setup.cb.end_tag &&
	 (pinfo->sel.active || 
	  checkflag(pinfo->setup.flags,STDPARSE_ALLNODES))) ? 
      pinfo->setup.cb.end_tag(user, name) : PARSER_OK;
    pinfo->depth--;
    pop_xpath(&pinfo->cp);
    pinfo->sel.active = cmp_prefix_xpath(&pinfo->cp, pinfo->sel.path);
    return r;
  }
  return PARSER_OK;
}

result_t std_chardata(void *user, const char_t *buf, size_t buflen) {
  stdparserinfo_t *pinfo = (stdparserinfo_t *)user;
  if( pinfo ) { 
    return pinfo->setup.cb.chardata &&
      (pinfo->sel.active || checkflag(pinfo->setup.flags,STDPARSE_ALLNODES)) ? 
      pinfo->setup.cb.chardata(user, buf, buflen) : PARSER_OK;
  }
  return PARSER_OK;
}


result_t std_pidata(void *user, const char_t *target, const char_t *data) {
  stdparserinfo_t *pinfo = (stdparserinfo_t *)user;
  if( pinfo ) { 
    return pinfo->setup.cb.pidata &&
      (pinfo->sel.active || checkflag(pinfo->setup.flags,STDPARSE_ALLNODES)) ? 
      pinfo->setup.cb.pidata(user, target, data) : PARSER_OK;
  }
  return PARSER_OK;
}

result_t std_comment(void *user, const char_t *data) {
  stdparserinfo_t *pinfo = (stdparserinfo_t *)user;
  if( pinfo ) { 
    return pinfo->setup.cb.comment &&
      (pinfo->sel.active || checkflag(pinfo->setup.flags,STDPARSE_ALLNODES)) ? 
      pinfo->setup.cb.comment(user, data) : PARSER_OK;
  }
  return PARSER_OK;
}

result_t std_start_cdata(void *user) {
  stdparserinfo_t *pinfo = (stdparserinfo_t *)user;
  if( pinfo ) { 
    return pinfo->setup.cb.start_cdata &&
      (pinfo->sel.active || checkflag(pinfo->setup.flags,STDPARSE_ALLNODES)) ? 
      pinfo->setup.cb.start_cdata(user) : PARSER_OK;
  }
  return PARSER_OK;
}

result_t std_end_cdata(void *user) {
  stdparserinfo_t *pinfo = (stdparserinfo_t *)user;
  if( pinfo ) { 
    return pinfo->setup.cb.end_cdata &&
      (pinfo->sel.active || checkflag(pinfo->setup.flags,STDPARSE_ALLNODES)) ? 
      pinfo->setup.cb.end_cdata(user) : PARSER_OK;
  }
  return PARSER_OK;
}

result_t std_dfault(void *user, const char_t *data, size_t buflen) {
  stdparserinfo_t *pinfo = (stdparserinfo_t *)user;
  if( pinfo ) { 
    return pinfo->setup.cb.dfault &&
      (pinfo->sel.active || checkflag(pinfo->setup.flags,STDPARSE_ALLNODES)) ? 
      pinfo->setup.cb.dfault(user, data, buflen) : PARSER_OK;
  }
  return PARSER_OK;
}

bool_t init_stdparserinfo(stdparserinfo_t *pinfo) {
#define INFDEPTH 32678
  if( pinfo ) {
    pinfo->depth = 0;
    pinfo->maxdepth = 0;
    pinfo->sel.active = FALSE;
    pinfo->sel.mindepth = INFDEPTH;
    pinfo->sel.maxdepth = 0;
    if( create_xpath(&pinfo->cp) ) {
      return TRUE;
    }
  }
  return FALSE;
}

bool_t free_stdparserinfo(stdparserinfo_t *pinfo) {
  if( pinfo ) {
    free_xpath(&pinfo->cp);
    /* zero memory so people don't try to use its (now invalid) contents */
    memset(pinfo, 0, sizeof(stdparserinfo_t));
    return TRUE;
  }
  return FALSE;
}

bool_t stdparse(int optind, char **argv, stdparserinfo_t *pinfo) {
  parser_t parser;
  stream_t strm;
  callback_t cb;
  char_t delim = ':';

  if( (optind > -1) && argv && pinfo ) {
    if( init_stdparserinfo(pinfo) ) {
      if( create_parser(&parser, pinfo) ) {

	cb.start_tag = std_start_tag; /* always needed */
	cb.end_tag = std_end_tag; /* always needed */
	cb.chardata = pinfo->setup.cb.chardata ? std_chardata : NULL;
	cb.pidata = pinfo->setup.cb.pidata ? std_pidata : NULL;
	cb.comment = pinfo->setup.cb.comment ? std_comment : NULL;
	cb.start_cdata = pinfo->setup.cb.start_cdata ? std_start_cdata : NULL;
	cb.end_cdata = pinfo->setup.cb.end_cdata ? std_end_cdata : NULL;
	cb.dfault = pinfo->setup.cb.dfault ? std_dfault : NULL;

	setup_parser(&parser, &cb);
	while( !checkflag(cmd,CMD_QUIT) ) {

	  pinfo->sel.path = NULL;
	  if( (optind > -1) && argv[optind] ) {
	    inputfile = argv[optind];
	    pinfo->sel.path = strchr(inputfile, delim);
	    if( pinfo->sel.path ) { *pinfo->sel.path++ = '\0'; }
	    if( !*inputfile ) { inputfile = "stdin"; }
	  }
	  if( !pinfo->sel.path ) { pinfo->sel.path = "/"; }

	  if( inputfile && open_file_stream(&strm, inputfile) ) {

	    while( !checkflag(cmd,CMD_QUIT) && 
		   read_stream(&strm, getbuf_parser(&parser, strm.blksize), 
			       strm.blksize) ) {
	    
	      if( !do_parser(&parser, strm.buflen) ) {
		if( (pinfo->depth == 0) && (pinfo->maxdepth > 0) ) {
		  /* we're done */
		  break;
		} 
		errormsg(E_FATAL, 
			 "invalid XML at line %d, column %d"
			 "(byte %ld, depth %d) of %s\n",
			 parser.cur.lineno, parser.cur.colno, 
			 parser.cur.byteno, pinfo->depth, inputfile);
	      }
	    }

	    close_stream(&strm);
	  }

	  if( !argv[optind] ) {
	    break;
	  }
	  optind++;
	  inputfile = NULL;
	  reset_parser(&parser);
	}
	free_parser(&parser);
      }
      free_stdparserinfo(pinfo);
      return TRUE;
    }
  }
  return FALSE;
}


