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

#ifndef FBPARSER_H
#define FBPARSER_H

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "parser.h"
#include "cursor.h"
#include "fbreader.h"
#include "xpath.h"

typedef enum { ps_ok, ps_error } position_status_t;
typedef struct {
  off_t offset; /* byte offset from start of document */
  int nodecount; /* number of complete nodes from start of document */
  int skip; /* number of elements to skip */
  position_status_t status;
} position_t;

/* noderep is sometimes used as a bit flag. CAREFUL: order is important,
 * as we want to be able to say (noderep > midfrag) etc. 
 */
typedef enum {
  na = 0x00,
  midfrag = 0x01,
  endfrag = 0x02,
  full = 0x04
} noderep_t;

typedef struct {
  off_t offset;
  size_t depth;
  size_t maxdepth;
  nodetype_t nodetype;
  noderep_t noderep;
  int nodecount;
  bool_t ignore;
} fbparserinfo_t;

typedef void (fb_start_tag_fun)(fbparserinfo_t *pinfo, const char_t *name, const char_t **att, void *user);
typedef void (fb_end_tag_fun)(fbparserinfo_t *pinfo, const char_t *name, void *user);
typedef void (fb_chardata_fun)(fbparserinfo_t *pinfo, const char_t *buf, size_t buflen, void *user);
typedef void (fb_pidata_fun)(fbparserinfo_t *pinfo, const char_t *target, const char_t *data, void *user);
typedef void (fb_start_cdata_fun)(fbparserinfo_t *pinfo, void *user);
typedef void (fb_end_cdata_fun)(fbparserinfo_t *pinfo, void *user);
typedef void (fb_comment_fun)(fbparserinfo_t *pinfo, const char_t *data, void *user);
typedef void (fb_default_fun)(fbparserinfo_t *pinfo, const char_t *data, size_t buflen, void *user);
typedef void (fb_node_fun)(fbparserinfo_t *pinfo, void *user);

typedef struct {
  fb_start_tag_fun *start_tag; /* after start tag */
  fb_end_tag_fun *end_tag; /* after end tag */
  fb_chardata_fun *chardata; /* after any char data fragment */
  fb_pidata_fun *pidata; /* after processing instruction */
  fb_start_cdata_fun *start_cdata; /* after start of CDATA */
  fb_end_cdata_fun *end_cdata; /* after end of CDATA */
  fb_comment_fun *comment; /* after comment */
  fb_default_fun *dfault; /* any fragment not covered above */
  fb_node_fun *node; /* after any node (never a fragment) */
  void *user;
} fbcallback_t;

typedef struct {
  fbreader_t reader;
  parser_t parser;
  fbparserinfo_t info;
  fbcallback_t callbacks;
} fbparser_t;

bool_t open_fileblockparser(fbparser_t *fbp, const char *path, size_t maxblocks);
bool_t close_fileblockparser(fbparser_t *fbp);

bool_t setup_fileblockparser(fbparser_t *fbp, fbcallback_t *fbcallbacks);
bool_t parse_cursor_fileblockparser(fbparser_t *fbp, const cursor_t *cursor, fbcallback_t *fbcallbacks, position_t *pos);
bool_t parse_first_fileblockparser(fbparser_t *fbp, const cursor_t *cursor, fbcallback_t *fbcallbacks, position_t *pos);
bool_t parse_next_fileblockparser(fbparser_t *fbp, position_t *pos);
bool_t reset_parserinfo_fileblockparser(fbparserinfo_t *pinfo);

#endif
 
