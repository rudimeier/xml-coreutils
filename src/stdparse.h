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

#define STDPARSE_ALLNODES  0x01

typedef struct {
  unsigned int depth; /* 0 means prolog, 1 means first tag, etc. */
  unsigned int maxdepth; /* max depth seen so far */
  xpath_t cp; /* current path */
  struct {
    char_t *path;
    bool_t active;
    unsigned int mindepth;
    unsigned int maxdepth;
  } sel; /* user selection (XPath) */
  struct {
    unsigned int flags; /* set some flags, or 0 if no flags wanted */
    callback_t cb; /* fill this with your callbacks */
  } setup;
} stdparserinfo_t;

bool_t stdparse(int index, char **argv, stdparserinfo_t *pinfo);

#endif
