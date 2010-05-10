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
#include "fbparser.h"
#include "myerror.h"
#include "mem.h"
#include "entities.h"
#include <string.h>
#include <limits.h>
#include <stdio.h>

result_t start_tag_fbp(void *user, const char_t *name, const char_t **att) {
  fbparser_t *fbp = (fbparser_t *)user;
  if( fbp ) { 
    fbp->info.nodetype = n_start_tag;
    /* fprintf(stderr,"<%s>(%d)\n", name, fbp->info.depth); */
    if( fbp->callbacks.start_tag ) {
      fbp->callbacks.start_tag(&fbp->info, name, att, fbp->callbacks.user);
    }
    if( fbp->callbacks.node ) {
      fbp->callbacks.node(&fbp->info, fbp->callbacks.user);
    }
    fbp->info.depth++;
    fbp->info.maxdepth = MAX(fbp->info.depth,fbp->info.maxdepth);
  }
  return PARSER_OK;
}

result_t end_tag_fbp(void *user, const char_t *name) {
  fbparser_t *fbp = (fbparser_t *)user;
  if( fbp ) { 
    fbp->info.nodetype = n_end_tag;
/*     audit_parser(&fbp->parser); */
/*     if( fbp->parser.cur.length == 0 ) { */
/*       printf("</>(%d)\n", fbp->info.depth); */
/*     } else { */
    /* fprintf(stderr,"</%s>(%d)\n", name, fbp->info.depth); */
/*     } */
    if( fbp->callbacks.end_tag ) {
      fbp->callbacks.end_tag(&fbp->info, name, fbp->callbacks.user);
    }
    if( fbp->callbacks.node ) {
      fbp->callbacks.node(&fbp->info, fbp->callbacks.user);
    }
    /* depth decremented after node. Do not tamper with this,
       as the skip.c code depends on it */
    fbp->info.depth--;
  }
  return PARSER_OK;
}

result_t chardata_fbp(void *user, const char_t *buf, size_t buflen) {
  fbparser_t *fbp = (fbparser_t *)user;
  if( fbp ) { 
    fbp->info.nodetype = is_xml_space(buf, buf + buflen) ? n_space : n_chardata;
    if( fbp->callbacks.chardata ) {
      fbp->callbacks.chardata(&fbp->info, buf, buflen, fbp->callbacks.user);
    }
    if( (fbp->info.noderep >= endfrag) && fbp->callbacks.node ) {
      fbp->callbacks.node(&fbp->info, fbp->callbacks.user);
    }
  }
  return PARSER_OK;
}

result_t comment_fbp(void *user, const char_t *data) {
  fbparser_t *fbp = (fbparser_t *)user;
  if( fbp ) { 
    fbp->info.nodetype = n_comment;
    if( fbp->callbacks.comment ) {
      fbp->callbacks.comment(&fbp->info, data, fbp->callbacks.user);
    }
    if( (fbp->info.noderep >= endfrag) && fbp->callbacks.node ) {
      fbp->callbacks.node(&fbp->info, fbp->callbacks.user);
    }
  }
  return PARSER_OK;
}

result_t pidata_fbp(void *user, const char_t *target, const char_t *data) {
  fbparser_t *fbp = (fbparser_t *)user;
  if( fbp ) { 
    fbp->info.nodetype = n_pidata;
    if( fbp->callbacks.pidata ) {
      fbp->callbacks.pidata(&fbp->info, target, data, fbp->callbacks.user);
    }
    if( (fbp->info.noderep >= endfrag) && fbp->callbacks.node ) {
      fbp->callbacks.node(&fbp->info, fbp->callbacks.user);
    }
  }
  return PARSER_OK;
}

result_t start_cdata_fbp(void *user) {
  fbparser_t *fbp = (fbparser_t *)user;
  if( fbp ) { 
    fbp->info.nodetype = n_start_cdata;
    fbp->info.ignore = TRUE;
    if( fbp->callbacks.start_cdata ) {
      fbp->callbacks.start_cdata(&fbp->info, fbp->callbacks.user);
    }
    if( (fbp->info.noderep >= endfrag) && fbp->callbacks.node ) {
      fbp->callbacks.node(&fbp->info, fbp->callbacks.user);
    }
  }
  return PARSER_OK;
}

result_t end_cdata_fbp(void *user) {
  fbparser_t *fbp = (fbparser_t *)user;
  if( fbp ) { 
    fbp->info.nodetype = n_end_cdata;
    if( fbp->callbacks.end_cdata ) {
      fbp->callbacks.end_cdata(&fbp->info, fbp->callbacks.user);
    }
    if( (fbp->info.noderep >= endfrag) && fbp->callbacks.node ) {
      fbp->callbacks.node(&fbp->info, fbp->callbacks.user);
    }
    fbp->info.ignore = FALSE;
  }
  return PARSER_OK;
}

result_t start_doctypedecl_fbp(void *user, const char_t *name, const char_t *sysid, const char_t *pubid, bool_t intsub) {
  fbparser_t *fbp = (fbparser_t *)user;
  if( fbp ) { 
    fbp->info.nodetype = n_start_doctype;
    fbp->info.ignore = TRUE;
    if( fbp->callbacks.start_doctypedecl ) {
      fbp->callbacks.start_doctypedecl(&fbp->info, name, sysid, pubid, intsub, fbp->callbacks.user);
    }
    if( (fbp->info.noderep >= endfrag) && fbp->callbacks.node ) {
      fbp->callbacks.node(&fbp->info, fbp->callbacks.user);
    }
  }
  return PARSER_OK;
}

result_t end_doctypedecl_fbp(void *user) {
  fbparser_t *fbp = (fbparser_t *)user;
  if( fbp ) { 
    fbp->info.nodetype = n_end_doctype;
    if( fbp->callbacks.end_doctypedecl ) {
      fbp->callbacks.end_doctypedecl(&fbp->info, fbp->callbacks.user);
    }
    if( (fbp->info.noderep >= endfrag) && fbp->callbacks.node ) {
      fbp->callbacks.node(&fbp->info, fbp->callbacks.user);
    }
    fbp->info.ignore = FALSE;
  }
  return PARSER_OK;
}

result_t entitydecl_fbp(void *user, const char_t *name, bool_t isparam, const char_t *value, int len, const char_t *base, const char_t *sysid, const char_t *pubid, const char_t *notation) {
  fbparser_t *fbp = (fbparser_t *)user;
  if( fbp ) { 
    fbp->info.nodetype = n_entitydecl;
    if( fbp->callbacks.entitydecl ) {
      fbp->callbacks.entitydecl(&fbp->info, name, isparam, value, len, base, sysid, pubid, notation, fbp->callbacks.user);
    }
    if( (fbp->info.noderep >= endfrag) && fbp->callbacks.node ) {
      fbp->callbacks.node(&fbp->info, fbp->callbacks.user);
    }
  }
  return PARSER_OK;
}

result_t dfault_fbp(void *user, const char_t *buf, size_t buflen) {
  fbparser_t *fbp = (fbparser_t *)user;
  if( fbp ) { 
    fbp->info.nodetype = is_xml_space(buf, buf + buflen) ? n_space : n_dfault;
    if( fbp->callbacks.dfault ) {
      fbp->callbacks.dfault(&fbp->info, buf, buflen, fbp->callbacks.user);
    }
    if( (fbp->info.noderep >= endfrag) && fbp->callbacks.node ) {
      fbp->callbacks.node(&fbp->info, fbp->callbacks.user);
    }
  }
  return PARSER_OK;
}

bool_t open_fileblockparser(fbparser_t *fbp, const char *path, size_t maxblocks) {
  callback_t callbacks;
  bool_t ok = TRUE;
  if( fbp ) {
    ok &= open_fileblockreader(&fbp->reader, path, maxblocks);
    ok &= create_parser(&fbp->parser, fbp);
    if( ok ) {

      memset(&callbacks, 0, sizeof(callback_t));
      callbacks.start_tag = start_tag_fbp;
      callbacks.end_tag = end_tag_fbp;
      callbacks.chardata = chardata_fbp;
      callbacks.pidata = pidata_fbp;
      callbacks.comment = comment_fbp;
      callbacks.start_cdata = start_cdata_fbp;
      callbacks.end_cdata = end_cdata_fbp;
      callbacks.start_doctypedecl = start_doctypedecl_fbp;
      callbacks.end_doctypedecl = end_doctypedecl_fbp;
      callbacks.entitydecl = entitydecl_fbp;
      callbacks.dfault = dfault_fbp;

      setup_parser(&fbp->parser, &callbacks);

      reset_parserinfo_fileblockparser(&fbp->info);

      memset(&fbp->callbacks, 0, sizeof(fbcallback_t));

      return TRUE;
    }
  }
  return FALSE;
}

bool_t close_fileblockparser(fbparser_t *fbp) {
  if( fbp ) {
    free_parser(&fbp->parser);
    close_fileblockreader(&fbp->reader);
    return TRUE;
  }
  return FALSE;
}

bool_t heartbeat_fileblockparser(fbparser_t *fbp) {
  /* if( fbp ) { */
  /*   mtime_fileblockreader(&fbp->reader); */
  /*   return TRUE; */
  /* } */
  return FALSE;
}

bool_t refresh_fileblockparser(fbparser_t *fbp) {
  if( fbp ) {
    if( refresh_fileblockreader(&fbp->reader) ) {
      return TRUE;
    }
  }
  return FALSE;
}

bool_t reset_parserinfo_fileblockparser(fbparserinfo_t *pinfo) {
  if( pinfo ) {
    pinfo->depth = 0;
    pinfo->maxdepth = 0;
    pinfo->nodetype = n_none;
    pinfo->noderep = na;
    pinfo->nodecount = 0;
    pinfo->offset = 0;
    pinfo->ignore = FALSE;
    return TRUE;
  }
  return FALSE;
}

bool_t setup_fileblockparser(fbparser_t *parser, fbcallback_t *callbacks) {
  if( parser ) {
    if( callbacks ) {
      memcpy(&parser->callbacks, callbacks, sizeof(fbcallback_t));
    } else {
      memset(&parser->callbacks, 0, sizeof(fbcallback_t));
    }
    return TRUE;
  }
  return FALSE;
}

bool_t parse_just_cursor_fileblockparser(fbparser_t *fbp, const cursor_t *cursor, fbcallback_t *callbacks, position_t *pos, bool_t fc) {
  size_t i;
  if( fbp && cursor && pos ) {
    reset_parser(&fbp->parser);
    reset_parserinfo_fileblockparser(&fbp->info);
    setup_fileblockparser(fbp, (fc || (cursor->top == 0)) ? callbacks : NULL);
    /* fprintf(stderr,"====\n"); */
    pos->offset = 0;
    pos->nodecount = 0;

    if( cursor->top > 1 ) {
      for(i = 0; i < cursor->top - 1; i++) {
	pos->offset = get_depth_offset_cursor(cursor, i);      
	pos->nodecount = get_depth_nord_cursor(cursor, i);
	if( (pos->offset < 0) ||
	    !parse_next_fileblockparser(fbp, pos) ) {
	  return FALSE;
	}
	touch_fileblockreader(&fbp->reader, fbp->info.offset);
      }
    }

    setup_fileblockparser(fbp, callbacks);
    pos->offset = get_top_offset_cursor(cursor);
    pos->nodecount = get_top_nord_cursor(cursor);
    touch_fileblockreader(&fbp->reader, fbp->info.offset);
    /* debug_cursor(cursor); */
    /* fprintf(stderr,"----\n"); */
    return parse_next_fileblockparser(fbp, pos);

  } 
  return FALSE;
}

/* parse up to the cursor with callbacks */
bool_t parse_cursor_fileblockparser(fbparser_t *fbp, const cursor_t *cursor, fbcallback_t *callbacks, position_t *pos) {
  return parse_just_cursor_fileblockparser(fbp, cursor, callbacks, pos, TRUE);
}

/* parse up to the cursor with no callbacks */
bool_t parse_first_fileblockparser(fbparser_t *fbp, const cursor_t *cursor, fbcallback_t *callbacks, position_t *pos) {
  return parse_just_cursor_fileblockparser(fbp, cursor, callbacks, pos, FALSE);
}

bool_t parse_next_fileblockparser(fbparser_t *fbp, position_t *pos) {
  size_t len;
  byte_t lookfor;
  bool_t retval = FALSE;
  byte_t *begin, *end;
  const byte_t *e;
  if( fbp && pos ) {
    fbp->info.offset = pos->offset;
    fbp->info.nodecount = pos->nodecount;
    pos->status = ps_ok;
    if( read_fileblockreader(&fbp->reader, fbp->info.offset, &begin, &end) ) {
      lookfor = (*begin == '<') ? '>' : '<';

      do {
	e = skip_byte(begin, end, lookfor);
	if( e < end ) {
	  fbp->info.noderep = 
	    (*e == '>') ? ((*begin == '<') ? full : endfrag) :
	    (*e == '<') ? endfrag : 
	    midfrag;
	  e = ( *e == '>' ) ? e + 1 : e;
	  len = (e - begin);
	} else {
	  fbp->info.noderep = midfrag;
	  len = end - begin;
	}

	if( !do_parser2(&fbp->parser, begin, len) ) {
	  /* parsing error? */
	  pos->status = ps_error;
	  return FALSE;
	}


	fbp->info.offset += len;

	if( fbp->info.noderep >= endfrag ) {
	  /* stop after every full node */
	  if( (fbp->info.nodetype > n_space) && !fbp->info.ignore ) { 
	    fbp->info.nodecount++; 
	  }
	  retval = TRUE;
	  break;
	}

      } while( read_fileblockreader(&fbp->reader, 
				    fbp->info.offset, &begin, &end) );

      pos->offset = fbp->info.offset;
      pos->nodecount = fbp->info.nodecount;
    }

  }
  return retval;
}

