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
#include "myerror.h"
#include "filelist.h"
#include "leafparse.h"
#include "entities.h"

#include <sys/types.h>
#include <sys/stat.h>
#include <sys/fcntl.h>
#include <string.h>
#include <unistd.h>

extern char *inputfile;
extern volatile flag_t cmd;

result_t lf_chardata(void *user, const char_t *buf, size_t buflen) {
  leafparserinfo_t *pinfo = (leafparserinfo_t *)user;
  bool_t ok = TRUE;
  if( pinfo ) { 

    setflag(&pinfo->reserved,LFP_R_CHARDATA);
    flipflag(&pinfo->reserved,LFP_R_EMPTY,is_xml_space(buf, buf + buflen));

    if( !( checkflag(pinfo->reserved,LFP_R_EMPTY) &&
	   checkflag(pinfo->setup.flags,LFP_SKIP_EMPTY) ) ) {
      
      if( checkflag(pinfo->reserved,LFP_R_CDATA) ) {
	ok &= write_unecho(&pinfo->ue, &pinfo->cp,(byte_t *)"\\Q", 2);
      }

      ok &= checkflag(pinfo->setup.flags,LFP_SQUEEZE) ?
	squeeze_unecho(&pinfo->ue, &pinfo->cp, (byte_t *)buf, buflen) :
	reconstruct_unecho(&pinfo->ue, &pinfo->cp, (byte_t *)buf, buflen) ;

      if( checkflag(pinfo->reserved,LFP_R_CDATA) ) {
	ok &= write_unecho(&pinfo->ue, &pinfo->cp,(byte_t *)"\\q", 2);
      }

    }

    if( !ok ) {
      errormsg(E_ERROR, "string value exceeds available pattern space\n");
      return PARSER_STOP;
    }

    return PARSER_OK;
  }
  return PARSER_OK;
}

result_t lf_start_tag(void *user, const char_t *name, const char_t **att) {
  leafparserinfo_t *pinfo = (leafparserinfo_t *)user;
  result_t r1, r2, r3;
  bool_t skip;
  if( pinfo ) { 

    /* pre tag below */

    if( pinfo->depth == 0 ) {
      r3 = ( pinfo->setup.cb.headwrap ?
	     pinfo->setup.cb.headwrap(user, name, &pinfo->tc) : 0 );
    } else {
      r3 = ( checkflag(pinfo->setup.flags,LFP_ALWAYS_CHARDATA) &&
	     false_and_setflag(&pinfo->reserved,LFP_R_CHARDATA) ?
	     lf_chardata(user, "", 0) : 0 );
    }

    pinfo->line.typ = ((pinfo->depth == 0) ? lt_first : lt_middle);
    activate_node_stdselect(&pinfo->sel, pinfo->depth, &pinfo->cp);
    pinfo->line.selected = pinfo->sel.active;
    skip = ( checkflag(pinfo->setup.flags,LFP_SKIP_EMPTY) &&
	     checkflag(pinfo->reserved,LFP_R_EMPTY) );

    r1 = ( pinfo->setup.cb.leaf_node && (pinfo->depth > 0) && !skip &&
	   checkflag(pinfo->setup.flags,LFP_PRE_OPEN) ) ?
      pinfo->setup.cb.leaf_node(user, &pinfo->ue) : PARSER_OK;

    clearflag(&pinfo->reserved,LFP_R_CHARDATA);

    /* post tag below */

    pinfo->depth++;
    pinfo->maxdepth = MAX(pinfo->depth, pinfo->maxdepth);
    pinfo->line.no++;
    push_tag_xpath(&pinfo->cp, name);

    activate_node_stdselect(&pinfo->sel, pinfo->depth, &pinfo->cp);
    pinfo->line.selected = pinfo->sel.active;
    if( checkflag(pinfo->setup.flags,LFP_ATTRIBUTES) ) {
      push_attributes_values_xpath(&pinfo->cp, att);
    }

    r2 = (pinfo->setup.cb.leaf_node &&
	 checkflag(pinfo->setup.flags,LFP_POST_OPEN) ) ?
      pinfo->setup.cb.leaf_node(user, &pinfo->ue) : PARSER_OK;

    return (r1|r2|r3);
  }
  return PARSER_OK;
}

result_t lf_end_tag(void *user, const char_t *name) {
  leafparserinfo_t *pinfo = (leafparserinfo_t *)user;
  result_t r0, r1, r2;
  bool_t skip;
  if( pinfo ) { 

    /* pre tag below */

    r0 = ( checkflag(pinfo->setup.flags,LFP_ALWAYS_CHARDATA) &&
	   false_and_setflag(&pinfo->reserved,LFP_R_CHARDATA) ?
	   lf_chardata(user, "", 0) : 0 );

    pinfo->line.typ = ((pinfo->depth == 1) ? lt_last : lt_middle);
    activate_node_stdselect(&pinfo->sel, pinfo->depth, &pinfo->cp);
    pinfo->line.selected = pinfo->sel.active;
    skip = ( checkflag(pinfo->setup.flags,LFP_SKIP_EMPTY) &&
	     checkflag(pinfo->reserved,LFP_R_EMPTY) );

    r1 = ( pinfo->setup.cb.leaf_node && !skip &&
	   checkflag(pinfo->setup.flags,LFP_PRE_CLOSE) ) ?
      pinfo->setup.cb.leaf_node(user, &pinfo->ue) : PARSER_OK;

    clearflag(&pinfo->reserved,LFP_R_CHARDATA);

    /* post tag below */

    pinfo->depth--;
    pinfo->line.no++;
    pop_xpath(&pinfo->cp);
    activate_node_stdselect(&pinfo->sel, pinfo->depth, &pinfo->cp);
    pinfo->line.selected = pinfo->sel.active;

    if( pinfo->depth == 0 ) {

      r2 = ( pinfo->setup.cb.footwrap ? 
	     pinfo->setup.cb.footwrap(user, name) : 0 );

    } else {

      r2 = ( pinfo->setup.cb.leaf_node && 
	     checkflag(pinfo->setup.flags,LFP_POST_CLOSE) ) ?
	pinfo->setup.cb.leaf_node(user, &pinfo->ue) : PARSER_OK;

    }

    return (r0|r1|r2);
  }
  return PARSER_OK;
}



result_t lf_pidata(void *user, const char_t *target, const char_t *data) {
  leafparserinfo_t *pinfo = (leafparserinfo_t *)user;
  if( pinfo ) { 
  }
  return PARSER_OK;
}

result_t lf_comment(void *user, const char_t *data) {
  leafparserinfo_t *pinfo = (leafparserinfo_t *)user;
  if( pinfo ) { 
  }
  return PARSER_OK;
}

result_t lf_start_cdata(void *user) {
  leafparserinfo_t *pinfo = (leafparserinfo_t *)user;
  if( pinfo ) { 
    setflag(&pinfo->reserved,LFP_R_CDATA);
    return PARSER_OK;
  }
  return PARSER_OK;
}

result_t lf_end_cdata(void *user) {
  leafparserinfo_t *pinfo = (leafparserinfo_t *)user;
  if( pinfo ) { 
    clearflag(&pinfo->reserved,LFP_R_CDATA);
    return PARSER_OK;
  }
  return PARSER_OK;
}

result_t lf_dfault(void *user, const char_t *data, size_t buflen) {
  leafparserinfo_t *pinfo = (leafparserinfo_t *)user;
  if( pinfo ) { 
    if( (pinfo->depth == 0) && pinfo->setup.cb.headwrap ) {
      write_tempcollect(&pinfo->tc, (byte_t *)data, buflen);
    }
    return PARSER_OK;
  }
  return PARSER_OK;
}

bool_t create_leafparserinfo(leafparserinfo_t *pinfo) {
  bool_t ok = TRUE;
  if( pinfo ) {
    memset(pinfo, 0, sizeof(leafparserinfo_t));
    ok &= create_xpath(&pinfo->cp);
    ok &= create_unecho(&pinfo->ue, 0);
    ok &= create_tempcollect(&pinfo->tc, "wrap", MINVARSIZE, MAXVARSIZE);
    ok &= create_stdselect(&pinfo->sel);
    return ok;
  }
  return FALSE;
}

bool_t reset_leafparserinfo(leafparserinfo_t *pinfo) {
  flag_t ue_flags = 0;

  if( pinfo ) {
    pinfo->depth = 0;
    pinfo->maxdepth = 0;
    pinfo->reserved = 0;
    reset_xpath(&pinfo->cp);

    if( checkflag(pinfo->setup.flags,LFP_ABSOLUTE_PATH) ) {
      setflag(&ue_flags,UNECHO_FLAG_ABSOLUTE);
    }
    reset_unecho(&pinfo->ue, ue_flags);

    reset_tempcollect(&pinfo->tc);

    reset_stdselect(&pinfo->sel);

    return TRUE;
  }
  return FALSE;
}

bool_t free_leafparserinfo(leafparserinfo_t *pinfo) {
  if( pinfo ) {
    free_xpath(&pinfo->cp);
    free_unecho(&pinfo->ue);
    free_tempcollect(&pinfo->tc);
    free_stdselect(&pinfo->sel);
    /* zero memory so people don't try to use its (now invalid) contents */
    memset(pinfo, 0, sizeof(leafparserinfo_t));
    return TRUE;
  }
  return FALSE;
}

bool_t leafparse(const char *file, cstringlst_t xpaths, leafparserinfo_t *pinfo) {
  parser_t parser;
  stream_t strm;
  callback_t cb;

  /* we don't require xpaths != NULL */
  if( file && pinfo ) {
    if( reset_leafparserinfo(pinfo) ) {
      if( create_parser(&parser, pinfo) ) {

	cb.start_tag = lf_start_tag; /* always needed */
	cb.end_tag = lf_end_tag; /* always needed */
	cb.chardata = lf_chardata;
	cb.pidata = lf_pidata;
	cb.comment = lf_comment;
	cb.start_cdata = lf_start_cdata;
	cb.end_cdata = lf_end_cdata;
	cb.dfault = lf_dfault;

	setup_parser(&parser, &cb);
	setup_xpaths_stdselect(&pinfo->sel, xpaths);

	if( !checkflag(cmd,CMD_QUIT) ) {

	  inputfile = (char *)file;

	  if( inputfile && open_file_stream(&strm, inputfile) ) {

	    while( !checkflag(cmd,CMD_QUIT) && 
		   read_stream(&strm, getbuf_parser(&parser, strm.blksize), 
			       strm.blksize) ) {
	      
	      if( !do_parser(&parser, strm.buflen) ) {
		if( (pinfo->depth == 0) && (pinfo->maxdepth > 0) ) {
		  /* we're done */
		  break;
		} 
		if( aborted_parser(&parser) ) {
		  /* user abort, this is not an error */
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

	  inputfile = NULL;
	  reset_parser(&parser);
	}

	free_parser(&parser);
      }
      free_leafparserinfo(pinfo);
      return TRUE;
    }
  }
  return FALSE;
}


