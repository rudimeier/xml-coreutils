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
#include "cursorrep.h"
#include "error.h"
#include "xpath.h"

#include <stdio.h>
#include <string.h>

bool_t create_cursorrep(cursorrep_t *cr) {
  if( cr ) {
    return ( create_xpath(&cr->xpath) );
  }
  return FALSE;
}

bool_t free_cursorrep(cursorrep_t *cr) {
  if( cr ) {
    free_xpath(&cr->xpath);
    return TRUE;
  }
  return FALSE;
}

bool_t reset_cursorrep(cursorrep_t *cr) {
  if( cr ) {
    return reset_xpath(&cr->xpath);
  }
  return FALSE;
}

typedef struct {
  xpath_t *xpath;
  cursor_t *cursor;
} build_t;

void append_node_number(build_t *b, int depth) {
  static char_t buf[32];
  /* this needs more thought to find something useful */
  snprintf(buf, 32, "(+%d)", get_depth_ord_cursor(b->cursor, depth));
  write_xpath(b->xpath, buf, strlen(buf));
}


void start_tag_cursorrep(fbparserinfo_t *pinfo, const char_t *name, const char_t **att, void *user) {
  build_t *b = (build_t *)user;
  if( pinfo && b ) { 
    push_tag_xpath(b->xpath, name);
    append_node_number(b, pinfo->depth);
  }
}

void end_tag_cursorrep(fbparserinfo_t *pinfo, const char_t *name, void *user) {
  build_t *b = (build_t *)user;
  if( pinfo && b ) { 
    /* if an end tag is seen, then it must cancel with the previous start tag */
    pop_xpath(b->xpath);
  }
}

void chardata_cursorrep(fbparserinfo_t *pinfo, const char_t *buf, size_t buflen, void *user) {
  build_t *b = (build_t *)user;
  if( pinfo && b ) { 
    push_node_xpath(b->xpath, n_chardata);
    append_node_number(b, pinfo->depth);
  }
}

void comment_cursorrep(fbparserinfo_t *pinfo, const char_t *data, void *user) {
  build_t *b = (build_t *)user;
  if( pinfo && b ) { 
    push_node_xpath(b->xpath, n_comment);
    append_node_number(b, pinfo->depth);
  }
}

void pidata_cursorrep(fbparserinfo_t *pinfo, const char_t *target, const char_t *data, void *user) {
  build_t *b = (build_t *)user;
  if( pinfo && b ) { 
    push_node_xpath(b->xpath, n_pidata);
    append_node_number(b, pinfo->depth);
  }
}

void dfault_cursorrep(fbparserinfo_t *pinfo, const char_t *buf, size_t buflen, void *user) {
  build_t *b = (build_t *)user;
  if( pinfo && b ) { 
    push_node_xpath(b->xpath, n_dfault);
    append_node_number(b, pinfo->depth);
  }
}

bool_t fill_callbacks_cursorrep(cursorrep_t *cr, build_t *b, fbcallback_t *callbacks) {
  if( cr && callbacks ) {

    memset(callbacks, 0, sizeof(fbcallback_t));

    callbacks->start_tag = start_tag_cursorrep;
    callbacks->end_tag = end_tag_cursorrep;
    callbacks->chardata = chardata_cursorrep;
    callbacks->pidata = pidata_cursorrep;
    callbacks->comment = comment_cursorrep;
    callbacks->dfault = dfault_cursorrep;
    callbacks->user = (void *)b;
    return TRUE;
  }
  return FALSE;
}


bool_t build_cursorrep(cursorrep_t *cr, cursor_t *cursor, fbparser_t *fbp) {
  position_t pos;
  fbcallback_t callbacks;
  build_t b;
  if( cr && cursor && fbp ) {
    reset_cursorrep(cr);
    b.xpath = &cr->xpath;
    b.cursor = cursor;
    fill_callbacks_cursorrep(cr, &b, &callbacks);
    return ( parse_cursor_fileblockparser(fbp, cursor, &callbacks, &pos) &&
	     (pos.status != ps_error) &&
	     normalize_xpath(&cr->xpath) );
  }
  return FALSE;
}

const char_t *get_locator_cursorrep(cursorrep_t *cr) {
  if( cr ) {
    return get_full_xpath(&cr->xpath);
  }
  return NULL;
}
