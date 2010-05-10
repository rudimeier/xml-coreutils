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
#include "stdparse.h"

#include <sys/types.h>
#include <sys/stat.h>
#include <sys/fcntl.h>
#include <string.h>
#include <unistd.h>

extern const char *inputfile;
extern volatile flag_t cmd;

result_t std_chardata(void *user, const char_t *buf, size_t buflen) {
  stdparserinfo_t *pinfo = (stdparserinfo_t *)user;
  result_t r;
  if( pinfo && pinfo->setup.cb.chardata ) { 
    setflag(&pinfo->reserved,STDPARSE_RESERVED_CHARDATA);

    push_node_xpath(&pinfo->cp, n_chardata);
    activate_stringval_stdselect(&pinfo->sel, pinfo->depth, &pinfo->cp);

    r = 
      (pinfo->sel.active || checkflag(pinfo->setup.flags,STDPARSE_ALLNODES)) ?
      pinfo->setup.cb.chardata(user, buf, buflen) : PARSER_OK;

    pop_xpath(&pinfo->cp);
    activate_tag_stdselect(&pinfo->sel, pinfo->depth, &pinfo->cp, NULL);

    return r;
  }
  return PARSER_OK;
}

result_t std_start_tag(void *user, const char_t *name, const char_t **att) {
  stdparserinfo_t *pinfo = (stdparserinfo_t *)user;
  result_t retval;
  bool_t ok;
  if( pinfo ) { 

    if( pinfo->setup.cb.chardata && 
	checkflag(pinfo->setup.flags,STDPARSE_ALWAYS_CHARDATA) &&
	!checkflag(pinfo->reserved,STDPARSE_RESERVED_CHARDATA) ) {
      std_chardata(user, "", 0);
    }
    clearflag(&pinfo->reserved,STDPARSE_RESERVED_CHARDATA);

    pinfo->depth++;
    pinfo->maxdepth = MAX(pinfo->depth, pinfo->maxdepth);
    push_tag_xpath(&pinfo->cp, name);
    activate_tag_stdselect(&pinfo->sel, pinfo->depth, &pinfo->cp, att);

    ok = checkflag(pinfo->setup.flags,STDPARSE_ALLNODES);

    retval = pinfo->setup.cb.start_tag && (ok || pinfo->sel.active)  ?
      pinfo->setup.cb.start_tag(user, name, att) : PARSER_OK;

    if( pinfo->setup.cb.attribute && 
	(att && *att) && (ok || pinfo->sel.attrib) ) {
      do {
	push_attribute_xpath(&pinfo->cp, att[0]);
	activate_attribute_stdselect(&pinfo->sel, pinfo->depth, &pinfo->cp, att[0]);
	if( ok || pinfo->sel.active ) {
	  retval |= pinfo->setup.cb.attribute(user, att[0], att[1]);
	}
	pop_attribute_xpath(&pinfo->cp);
	att += 2;
      } while( att && *att );
      activate_tag_stdselect(&pinfo->sel, pinfo->depth, &pinfo->cp, NULL);
    }

    return retval;
  }
  return PARSER_OK;
}

result_t std_end_tag(void *user, const char_t *name) {
  stdparserinfo_t *pinfo = (stdparserinfo_t *)user;
  result_t r;
  if( pinfo ) { 

    if( pinfo->setup.cb.chardata && 
	checkflag(pinfo->setup.flags,STDPARSE_ALWAYS_CHARDATA) &&
	!checkflag(pinfo->reserved,STDPARSE_RESERVED_CHARDATA) ) {
      std_chardata(user, "", 0);
    }
    clearflag(&pinfo->reserved,STDPARSE_RESERVED_CHARDATA);

    r = (pinfo->setup.cb.end_tag &&
	 (pinfo->sel.active || 
	  checkflag(pinfo->setup.flags,STDPARSE_ALLNODES))) ? 
      pinfo->setup.cb.end_tag(user, name) : PARSER_OK;

    pinfo->depth--;
    pop_xpath(&pinfo->cp);
    activate_tag_stdselect(&pinfo->sel, pinfo->depth, &pinfo->cp, NULL);

    return r;
  }
  return PARSER_OK;
}


result_t std_pidata(void *user, const char_t *target, const char_t *data) {
  stdparserinfo_t *pinfo = (stdparserinfo_t *)user;
  result_t r;
  if( pinfo ) { 
    r = pinfo->setup.cb.pidata &&
      (pinfo->sel.active || checkflag(pinfo->setup.flags,STDPARSE_ALLNODES)) ? 
      pinfo->setup.cb.pidata(user, target, data) : PARSER_OK;
    return r;
  }
  return PARSER_OK;
}

result_t std_comment(void *user, const char_t *data) {
  stdparserinfo_t *pinfo = (stdparserinfo_t *)user;
  result_t r;
  if( pinfo ) { 
    r = pinfo->setup.cb.comment &&
      (pinfo->sel.active || checkflag(pinfo->setup.flags,STDPARSE_ALLNODES)) ? 
      pinfo->setup.cb.comment(user, data) : PARSER_OK;
    return r;
  }
  return PARSER_OK;
}

result_t std_start_cdata(void *user) {
  stdparserinfo_t *pinfo = (stdparserinfo_t *)user;
  result_t r;
  if( pinfo ) { 
    r = pinfo->setup.cb.start_cdata &&
      (pinfo->sel.active || checkflag(pinfo->setup.flags,STDPARSE_ALLNODES)) ? 
      pinfo->setup.cb.start_cdata(user) : PARSER_OK;
    return r;
  }
  return PARSER_OK;
}

result_t std_end_cdata(void *user) {
  stdparserinfo_t *pinfo = (stdparserinfo_t *)user;
  result_t r;
  if( pinfo ) { 
    r = pinfo->setup.cb.end_cdata &&
      (pinfo->sel.active || checkflag(pinfo->setup.flags,STDPARSE_ALLNODES)) ? 
      pinfo->setup.cb.end_cdata(user) : PARSER_OK;
    return r;
  }
  return PARSER_OK;
}

result_t std_dfault(void *user, const char_t *data, size_t buflen) {
  stdparserinfo_t *pinfo = (stdparserinfo_t *)user;
  result_t r;
  if( pinfo ) { 
    r = pinfo->setup.cb.dfault &&
      (pinfo->sel.active || checkflag(pinfo->setup.flags,STDPARSE_ALLNODES)) ? 
      pinfo->setup.cb.dfault(user, data, buflen) : PARSER_OK;
    return r;
  }
  return PARSER_OK;
}

result_t std_start_doctypedecl(void *user, const char_t *name, const char_t *sysid, const char_t *pubid, bool_t intsubset) {
  stdparserinfo_t *pinfo = (stdparserinfo_t *)user;
  result_t r;
  if( pinfo ) {
    setflag(&pinfo->reserved,STDPARSE_RESERVED_INTSUBSET);
    r = pinfo->setup.cb.start_doctypedecl ?
      pinfo->setup.cb.start_doctypedecl(user, name, sysid, pubid, intsubset) : PARSER_OK;
    return r;
  }
  return PARSER_OK;
}

result_t std_end_doctypedecl(void *user) {
  stdparserinfo_t *pinfo = (stdparserinfo_t *)user;
  result_t r;
  if( pinfo ) {
    clearflag(&pinfo->reserved,STDPARSE_RESERVED_INTSUBSET);
    r = pinfo->setup.cb.end_doctypedecl ?
      pinfo->setup.cb.end_doctypedecl(user) : PARSER_OK;
    return r;
  }
  return PARSER_OK;
}

bool_t create_stdparserinfo(stdparserinfo_t *pinfo) {
  bool_t ok = TRUE;
  if( pinfo ) {
    memset(pinfo, 0, sizeof(stdparserinfo_t));
    ok &= create_xpath(&pinfo->cp);
    ok &= create_stdselect(&pinfo->sel);
    return ok;
  }
  return FALSE;
}

bool_t reset_stdparserinfo(stdparserinfo_t *pinfo) {
  if( pinfo ) {
    pinfo->depth = 0;
    pinfo->maxdepth = 0;
    reset_xpath(&pinfo->cp);
    pinfo->reserved = 0;
    reset_stdselect(&pinfo->sel);
    /* don't touch setup structure */
    return TRUE;
  }
  return FALSE;
}

bool_t free_stdparserinfo(stdparserinfo_t *pinfo) {
  if( pinfo ) {
    free_xpath(&pinfo->cp);
    free_stdselect(&pinfo->sel);
    /* zero memory so people don't try to use its (now invalid) contents */
    memset(pinfo, 0, sizeof(stdparserinfo_t));
    return TRUE;
  }
  return FALSE;
}

bool_t stdparse_failed(stdparserinfo_t *pinfo) {
  return (!pinfo || checkflag(pinfo->reserved,STDPARSE_RESERVED_PARSEFAIL));
}

bool_t stdparse2(int n, cstringlst_t files, cstringlst_t *xpaths,
		 stdparserinfo_t *pinfo) {
  parser_t parser;
  stream_t strm;
  callback_t cb;
  cstringlst_t xp;
  int f;

  if( pinfo && files ) {

    if( reset_stdparserinfo(pinfo) ) {

      if( create_parser(&parser, pinfo) ) {

	cb.start_tag = std_start_tag; /* always needed */
	cb.end_tag = std_end_tag; /* always needed */

	cb.chardata = pinfo->setup.cb.chardata ? std_chardata : NULL;
	cb.pidata = pinfo->setup.cb.pidata ? std_pidata : NULL;
	cb.comment = pinfo->setup.cb.comment ? std_comment : NULL;
	cb.start_cdata = pinfo->setup.cb.start_cdata ? std_start_cdata : NULL;
	cb.end_cdata = pinfo->setup.cb.end_cdata ? std_end_cdata : NULL;
	cb.start_doctypedecl = pinfo->setup.cb.start_doctypedecl ? std_start_doctypedecl : NULL;
	cb.end_doctypedecl = pinfo->setup.cb.end_doctypedecl ? std_end_doctypedecl : NULL;
	cb.entitydecl = pinfo->setup.cb.entitydecl;
	cb.dfault = pinfo->setup.cb.dfault ? std_dfault : NULL;

	setup_parser(&parser, &cb);

	for(f = 0; (f < n) && !checkflag(cmd,CMD_QUIT); f++) {

	  inputfile = files[f];
	  xp = xpaths ? xpaths[f] : NULL;
	  setup_xpaths_stdselect(&pinfo->sel, xp);

	  if( pinfo->setup.start_file_fun ) {
	    if( !pinfo->setup.start_file_fun(pinfo, files[f], xp) ) {
	      continue;
	    }
	  }

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
		if( !checkflag(pinfo->setup.flags,STDPARSE_QUIET) ) {
		  errormsg(E_FATAL, 
			   "%s: %s at line %d, column %d, "
			   "byte %ld, depth %d\n",
			   inputfile, error_message_parser(&parser),
			   parser.cur.lineno, parser.cur.colno, 
			   parser.cur.byteno, pinfo->depth);
		}

		setflag(&pinfo->reserved,STDPARSE_RESERVED_PARSEFAIL);
		break;
	      }

	    }

	    close_stream(&strm);
	  }

	  if( pinfo->setup.end_file_fun ) {
	    if( !pinfo->setup.end_file_fun(pinfo, files[f], xp) ) {
	      break;
	    }
	  }

	  inputfile = NULL;
	  reset_parser(&parser);
	  reset_stdparserinfo(pinfo);
	}

	free_parser(&parser);
      }
      return TRUE;
    }
  }
  return FALSE;
}

bool_t stdparse(int argc, char **argv, stdparserinfo_t *pinfo) {

  filelist_t fl;
  flag_t ff;
  cstringlst_t files;
  cstringlst_t *xpaths;
  int n;
  bool_t retval = FALSE;

  if( argv && pinfo ) {

    ff = 0;
    if( checkflag(pinfo->setup.flags, STDPARSE_EQ1FILE) ) {
      setflag(&ff,FILELIST_EQ1);
    }
    if( checkflag(pinfo->setup.flags, STDPARSE_MIN1FILE) ) {
      setflag(&ff,FILELIST_MIN1);
    }

    if( create_filelist(&fl, argc, argv, ff) ) {

      files = getfiles_filelist(&fl);
      xpaths = getxpaths_filelist(&fl);
      n = getsize_filelist(&fl);

      if( checkflag(pinfo->setup.flags, STDPARSE_EQ1FILE) && (n > 1) ) {
	errormsg(E_FATAL, "too many input files\n");
      }
      if( hasxpaths_filelist(&fl) &&
	  checkflag(pinfo->setup.flags, STDPARSE_NOXPATHS) ) {
	errormsg(E_FATAL, "command does not accept XPATH after filename(s)\n");
      }

      retval = stdparse2(n, files, xpaths, pinfo);

      free_filelist(&fl);
    }
  }
  return retval;
}

