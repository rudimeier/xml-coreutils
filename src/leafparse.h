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

#ifndef LEAFPARSE_H
#define LEAFPARSE_H

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

/*
 * The leafparser traverses an XML document one "leafnode" at a time.
 * In normal XML, a "node" is a subtree which can contain tags and
 * chardata etc. Here we call a "leafnode" a structure which can only contain
 * a chardata element. The leafnode has an address which is the XML node
 * which immediately contains the chardata. A "leafnode" need not contain
 * an actual chardata element, only the possibility is required.
 *
 * In other words, the leafnodes occur in between the normal tags (opening
 * or closing) of the document, where chardata is possible. 
 */

#include "parser.h"
#include "leafnode.h"
#include "xpath.h"
#include "unecho.h"
#include "tempcollect.h"
#include "stdselect.h"

#define LFP_R_NEWLINE   0x01
#define LFP_R_VERBATIM  0x02
#define LFP_R_CHARDATA  0x04
#define LFP_R_CDATA     0x08
#define LFP_R_EMPTY     0x10

#define LFP_SQUEEZE           0x01 /* squeeze whitespace */
#define LFP_ABSOLUTE_PATH     0x02 /* force all paths absolute */
#define LFP_SKIP_EMPTY        0x04 /* skip whitespace chardata */
#define LFP_ATTRIBUTES        0x08 /* incl. attributes in paths */
#define LFP_ALWAYS_CHARDATA   0x10 /* always call chardata fun */

/* these flags tell the parser when to call leaf_node_fun
   make sure to set at least one, otherwise the parser won't
   call the leaf_node_fun at all */
#define LFP_PRE_OPEN      0x08 
#define LFP_POST_OPEN     0x10
#define LFP_PRE_CLOSE     0x20
#define LFP_POST_CLOSE    0x40

typedef enum {lt_first = 0, lt_middle, lt_last} linetype_t;
typedef struct {
  long int no; /* total opened tags + closed tags = chardata slots */
  linetype_t typ;
  bool_t selected;
} lineinfo_t;

typedef result_t (leaf_node_fun)(void *user, unecho_t *ue);
typedef result_t (headwrap_fun)(void *user, const char_t *root, tempcollect_t *wrap);
typedef result_t (footwrap_fun)(void *user, const char_t *root);

typedef struct {
  leaf_node_fun *leaf_node; 
  headwrap_fun *headwrap;
  footwrap_fun *footwrap;
} leaf_callback_t;

/* this structure is used for standard bookkeeping by leafparse.
   When calling leafparse, you should create your own pinfo structure
   whose first element is a leafparserinfo_t, like so:

   typedef struct {
     leafparserinfo_t std;
     int mydata;
   } myparserinfo_t

   Then you can cast this structure when passing it to stdparse:

   myparserinfo_t pinfo;
   leafparse(index, argv, (leafparserinfo_t *)&pinfo);

   We do this so that the callbacks can have access to the leafparserinfo_t
   data if they want to, without too much fuss. 

   The leafparserinfo_t struct must be partially filled (all members of 
   the setup section). 
*/

typedef struct {
  unsigned int depth; /* 0 means prolog, 1 means first tag, etc. */
  unsigned int maxdepth; /* max depth seen so far */
  lineinfo_t line;
  xpath_t cp; /* current path */
  unecho_t ue; /* current unecho'd leafnode */
  tempcollect_t tc; /* original head or footwrap */
  flag_t reserved; /* not for users */
  stdselect_t sel; /* user selection (XPath) */
  struct {
    flag_t flags; /* set some flags, or 0 if no flags wanted */
    leaf_callback_t cb;
  } setup;
} leafparserinfo_t;

bool_t create_leafparserinfo(leafparserinfo_t *pinfo);
bool_t free_leafparserinfo(leafparserinfo_t *pinfo);
bool_t leafparse(const char *file, cstringlst_t xpaths, leafparserinfo_t *pinfo);

#endif
