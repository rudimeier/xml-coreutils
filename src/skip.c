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
#include "skip.h"
#include <string.h>


bool_t check_skip(const skip_t *skip, const fbparserinfo_t *pinfo) {
  if( skip && pinfo && !pinfo->ignore ) {
    if( pinfo->nodetype & skip->nodemask ) {
/*       debug_cursor(cursor); */
      switch(skip->what) {
      case any:
	return TRUE; /* this is probably NOT what you want */
      case eq_depth:
	return (pinfo->depth == skip->depth);
      case gt_depth:
	return (pinfo->depth > skip->depth);
      case gte_depth:
	return (pinfo->depth >= skip->depth);
      case lt_depth:
	return (pinfo->depth < skip->depth);
      case lte_depth:
	return (pinfo->depth <= skip->depth);
      case not_endtag:
	return (pinfo->nodetype != n_end_tag);
      }
    }
  }
  return FALSE;
} 

typedef struct {
  bool_t done;
  cursor_t *cursor;
  skip_t *skip;
} forward_t;

/* warning: node_forward_skip can be called several times for a single
 * tag, e.g. if we encounter <a/> then we get called for <a> and
 * immediately for </a>. This can cause naive counting to go wrong. 
 *
 * Also: remember that an closing tag </a> is one depth lower than
 * the corresponding opening tag <a>. So when we remove a closing tag,
 * we must also pop the opening tag.
 */
void node_forward_skip(fbparserinfo_t *pinfo, void *user) {
  forward_t *fw = (forward_t *)user;
  if( pinfo && fw ) {
    if( check_skip(fw->skip, pinfo) ) {
      bump_cursor(fw->cursor, pinfo->depth, pinfo->offset, pinfo->nodecount);
      fw->skip->count--;
      fw->done = (fw->skip->count <= 0);
    }
  }
}

/* this is an all purpose cursor iterator. The input cursor is modified
 * to point to the next cursor as defined by skip semantics.
 * skip->count is the number of nodes which satisfy a condition to skip.
 * skip->depth is a condition on the depth. All nodes related to depth 
 * are skipped.
 * skip->nodemask is a condition on the node type that must be true if
 * skipping should count for something.
 *
 * NOTE: if the cursor ends up on a end-tag, then you will probably get
 * errors with parsing later. This should not be a problem if the condition
 * involves the depth, because end-tags have greater depth than the corresponding
 * start-tag (see also end_tag_fbp()).
 */
bool_t forward_skip(skip_t *skip, cursor_t *cursor, fbparser_t *fbp) {
  position_t pos;
  fbcallback_t callbacks;
  forward_t fw;
  if( skip && cursor && fbp ) {
    fw.done = FALSE;
    fw.cursor = cursor;
    fw.skip = skip;

    memset(&callbacks, 0, sizeof(fbcallback_t));
    callbacks.node = node_forward_skip;
    callbacks.user = (void *)&fw;
    if( parse_first_fileblockparser(fbp, cursor, NULL, &pos) ) {

      setup_fileblockparser(fbp, &callbacks);
      while( !fw.done && parse_next_fileblockparser(fbp, &pos) );
      return fw.done;
    }
  }
  return FALSE;
}


typedef struct {
  bool_t done;
  cursor_t *cursor;
  off_t target;
  skip_t *skip;
} backward_t;

void node_backward_skip(fbparserinfo_t *pinfo, void *user) {
  backward_t *bw = (backward_t *)user;
  if( pinfo && bw ) {
    bw->done = (pinfo->offset >= bw->target);
    if( !bw->done && check_skip(bw->skip, pinfo) ) {
      bump_cursor(bw->cursor, pinfo->depth, pinfo->offset, pinfo->nodecount);
    }
  }
}

/* finds the last node satisfying skip criteria before the cursor.
 * Algoritm is not efficient. Returns origin cursor if no other
 * cursor satisfies the criteria. Not all criteria work properly.
 */
bool_t backward_skip(skip_t *skip, cursor_t *cursor, fbparser_t *fbp) {
  position_t pos;
  fbcallback_t callbacks;
  backward_t bw;

  if( skip && cursor && fbp ) {
    while( skip->count-- > 0 ) {

      bw.done = FALSE;
      bw.target = get_top_offset_cursor(cursor);
      bw.skip = skip;


      if( !parent_cursor(cursor) ) {
	return TRUE;
      }
      bw.cursor = cursor;

      memset(&callbacks, 0, sizeof(fbcallback_t));
      callbacks.node = node_backward_skip;
      callbacks.user = (void *)&bw;
      if( parse_first_fileblockparser(fbp, cursor, NULL, &pos) ) {
	setup_fileblockparser(fbp, &callbacks);
	while( !bw.done && parse_next_fileblockparser(fbp, &pos) );
	if( !bw.done ) {
	  return FALSE; /* this is probably serious */
	}
      }

    }
    return TRUE;
  }
  return FALSE;
}
