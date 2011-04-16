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

#ifndef STDPARSE_H
#define STDPARSE_H

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "parser.h"
#include "xpath.h"
#include "stdselect.h"

/*
 * The stdparser is the workhorse of the xml-coreutils. On input, it accepts
 * several files and XPATH expressions according to the common unified
 * command line convention (see design document), and it traverses them.
 */

#define STDPARSE_ALLNODES          0x01 /* call user even if not selected */
#define STDPARSE_EQ1FILE           0x02 /* exactly 1 input file allowed */
#define STDPARSE_NOXPATHS          0x04 /* no selection allowed */
#define STDPARSE_MIN1FILE          0x08 /* at least 1 input file needed */
#define STDPARSE_ALWAYS_CHARDATA   0x10 /* chardata fun even when empty */
#define STDPARSE_QUIET             0x20 /* if parsing error, no errormsg */


#define STDPARSE_RESERVED_NEWLINE   0x01
#define STDPARSE_RESERVED_VERBATIM  0x02
#define STDPARSE_RESERVED_CHARDATA  0x04
#define STDPARSE_RESERVED_INTSUBSET 0x08
#define STDPARSE_RESERVED_PARSEFAIL 0x10

typedef bool_t (xml_start_file_fun)(void *user, const char_t *file, cstringlst_t xpaths);
typedef bool_t (xml_end_file_fun)(void *user, const char_t *file, cstringlst_t xpaths);

/* this structure is used for standard bookkeeping by stdparse.
   When calling stdparse, you should create your own pinfo structure
   whose first element is a stdparserinfo_t, like so:

   typedef struct {
     stdparserinfo_t std;
     int mydata;
   } myparserinfo_t

   Then you can cast this structure when passing it to stdparse:

   myparserinfo_t pinfo;
   stdparse(index, argv, (stdparserinfo_t *)&pinfo);

   We do this so that the callbacks can have access to the stdparserinfo_t
   data if they want to, without too much fuss. 

   The stdparserinfo_t struct must be partially filled (all members of 
   the setup section). 
*/

typedef struct {
  unsigned int depth; /* 0 means prolog, 1 means first tag, etc. */
  unsigned int maxdepth; /* max depth seen so far */
  xpath_t cp; /* current path */
  flag_t reserved; /* not for users */
  stdselect_t sel; /* user selection (XPath) */
  struct {
    flag_t flags; /* set some flags, or 0 if no flags wanted */
    callback_t cb; /* fill this with your callbacks */
    xml_start_file_fun *start_file_fun; /* ret false skips file */
    xml_end_file_fun *end_file_fun; /* ret false ends parsing */
  } setup;
} stdparserinfo_t;

bool_t create_stdparserinfo(stdparserinfo_t *pinfo);
bool_t free_stdparserinfo(stdparserinfo_t *pinfo);

/* stdparse: this builds a filelist for you and calls stdparse2 */
bool_t stdparse(int index, char **argv, stdparserinfo_t *pinfo);
/* stdparse2: run a parser on each of input files in turn and select
 * any xpaths using stdselect_t. If a file is NULL, then it is merely
 * skipped.  
 * The start_file_fun is called before each file is opened, and the
 * file is skipped if it returns FALSE. 
 * The end_file_fun is called after each file, and a FALSE return value
 * stops parsing any further input.
 */
bool_t stdparse2(int n, cstringlst_t files, cstringlst_t *xpaths,
		 stdparserinfo_t *pinfo);
/* stdparse3: create/destroy parser objects used internally by stdparse2 */
bool_t stdparse3_create(parser_t *parser, stdparserinfo_t *pinfo);
bool_t stdparse3_free(parser_t *parser, stdparserinfo_t *pinfo);


bool_t stdparse_failed(stdparserinfo_t *pinfo);

#endif
