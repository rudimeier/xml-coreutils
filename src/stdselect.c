/* 
 * Copyright (C) 2009 Laird Breyer
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
#include "stdselect.h"
#include "myerror.h"

bool_t create_stdselect(stdselect_t *sel) {
  bool_t ok = TRUE;
  if( sel ) {
    ok &= create_wrap_xmatcher(&sel->paths, NULL);
    ok &= create_xpredicatelist(&sel->preds);
    ok &= create_xattributelist(&sel->atts);
    ok &= create_nhistory(&sel->history);

    sel->active = FALSE;
    sel->mindepth = INFDEPTH;
    sel->maxdepth = 0;
    return ok;
  }
  return FALSE;
}

bool_t reset_stdselect(stdselect_t *sel) {
  if( sel ) {
    reset_xmatcher(&sel->paths);
    reset_xpredicatelist(&sel->preds);
    reset_xattributelist(&sel->atts);
    reset_nhistory(&sel->history);
    push_level_nhistory(&sel->history, 0);
    sel->active = FALSE;
    sel->mindepth = INFDEPTH;
    sel->maxdepth = 0;
    return TRUE;
  }
  return FALSE;
}

bool_t free_stdselect(stdselect_t *sel) {
  if( sel ) {
    free_xmatcher(&sel->paths);
    free_xpredicatelist(&sel->preds);
    free_xattributelist(&sel->atts);
    free_nhistory(&sel->history);
    return TRUE;
  }
  return FALSE;
}

bool_t setup_xpaths_stdselect(stdselect_t *sel, cstringlst_t xpaths) {
  if( sel ) {
    if( !create_wrap_xmatcher(&sel->paths, xpaths) ||
	!compile_xpredicatelist(&sel->preds, xpaths) ||
	!compile_xattributelist(&sel->atts, xpaths) ) {
      errormsg(E_FATAL, "bad xpath list.\n");
    }
    return TRUE;
  }
  return FALSE;
}

bool_t find_matching_xpath_stdselect(stdselect_t *sel, const xpath_t *xpath) {
  const char_t *path;
  const xmatcher_t *xm;
  const xpredicate_t *xp;
  const xattribute_t *xa;
  bool_t v, tmatch, amatch;
  int m, n;
  if( sel && xpath ) {
    path = string_xpath(xpath);
    xm = &sel->paths;
    tmatch = amatch = FALSE;
    for(n = 0; n < xm->xpath_count; n++) {
      xp = get_xpredicatelist(&sel->preds, n);
      xa = get_xattributelist(&sel->atts, n);
      m = match_no_att_no_pred_xpath(xm->xpath[n], NULL, path);
      v = valid_xpredicate(xp);
      tmatch |= (!xa->begin && ((m == 0) || (m == -1)) && v);
      amatch |= (xa->begin && xa->precheck && (m == 0) && v);
    }
    sel->attrib = amatch;
    return tmatch;
  }
  return FALSE;
}

/* may create or delete a node */
nhistory_node_t *getnode_stdselect(stdselect_t *sel, int depth) {
  nhistory_node_t *node = NULL;
  int topd;
  if( sel ) {
    topd = sel->history.node_memo.top;
    if( topd == depth ) {
      push_level_nhistory(&sel->history, depth);
      node = get_node_nhistory(&sel->history, depth);
    } else if( --topd == depth ) {
      node = get_node_nhistory(&sel->history, depth);
    } else if( --topd == depth ) {
      pop_level_nhistory(&sel->history, depth + 1);
      node = get_node_nhistory(&sel->history, depth);
    }
    if( !node ) {
      errormsg(E_FATAL, "fatal corruption in nhistory (depth=%d, top=%d)\n", 
	       depth, sel->history.node_memo.top);
    }
  }
  return node;
}


bool_t activate_tag_stdselect(stdselect_t *sel, int depth, const xpath_t *xpath, const char_t **att) {
  bool_t update;
  nhistory_node_t *node;
  if( sel && xpath && (sel->paths.xpath_count > 0) ) {
    node = getnode_stdselect(sel, depth);
    update = (node->tag == undefined);

    if( update && att ) { /* start-tag event */
      update_xpredicatelist(&sel->preds, string_xpath(xpath), att);
      update_xattributelist(&sel->atts, att);
    }

    MEMOIZE_BOOL_NHIST( node->tag, 
			find_matching_xpath_stdselect(sel, xpath) );

    sel->active = (node->tag == active);

    if( update && sel->active ) {
      sel->mindepth = MIN(depth, sel->mindepth);
      sel->maxdepth = MAX(depth, sel->maxdepth);
    }
    return TRUE;
  }
  return FALSE;
}

bool_t activate_stringval_stdselect(stdselect_t *sel, int depth, const xpath_t *xpath) {
  nhistory_node_t *node;
  if( sel && xpath && (sel->paths.xpath_count > 0) ) {
    node = getnode_stdselect(sel, depth);

    MEMOIZE_BOOL_NHIST( node->stringval, 
			find_matching_xpath_stdselect(sel, xpath) );
    sel->active = (node->stringval == active);

    return TRUE;
  }
  return FALSE;
}

bool_t activate_node_stdselect(stdselect_t *sel, int depth, const xpath_t *xpath) {
  bool_t update;
  nhistory_node_t *node;
  if( sel && xpath && (sel->paths.xpath_count > 0) ) {
    node = getnode_stdselect(sel, depth);
    update = (node->node == undefined);

    if( update ) {
      update_xpredicatelist(&sel->preds, string_xpath(xpath), NULL);
      update_xattributelist(&sel->atts, NULL);
    }

    MEMOIZE_BOOL_NHIST( node->node, 
			find_matching_xpath_stdselect(sel, xpath) );
    sel->active = (node->node == active);

    return TRUE;
  }
  return FALSE;
}

bool_t activate_attribute_stdselect(stdselect_t *sel, int depth, const xpath_t *xpath, const char_t *name) {
  if( sel && xpath ) {
    sel->active = check_xattributelist(&sel->atts, string_xpath(xpath), name);
    return TRUE;
  }
  return FALSE;
}

