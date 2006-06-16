/* 
 * Copyright (C) 2006 Laird Breyer
 *  
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
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

#ifndef COMMON_H
#define COMMON_H

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <expat.h>

typedef unsigned char byte_t;
typedef XML_Char char_t;
typedef int bool_t;
typedef unsigned long flag_t;
#define FALSE ((bool_t)0)
#define TRUE  ((bool_t)1)

#define CMD_QUIT     0x01

#define MIN(x,y) (((x)<(y))?(x):(y))
#define MAX(x,y) (((x)>(y))?(x):(y))

/* flag manip */
__inline__ static bool_t checkflag(flag_t x, flag_t y) {
  return x & y;
}

__inline__ static void setflag(flag_t *x, flag_t y) {
  *x |= y;
}

__inline__ static void clearflag(flag_t *x, flag_t y) {
  *x &= ~y;
}

__inline__ static void flipflag(flag_t *x, flag_t y, bool_t yes) {
  if(yes) { setflag(x,y); } else { clearflag(x, y); }
}

/* in jenkins.c */
unsigned long int hash(unsigned char *k,
		       unsigned long int length,
		       unsigned long int initval);

#ifndef VERSION
#define VERSION PACKAGE_VERSION
#endif

#define COPYBLURB " (xml-coreutils version " VERSION  ")\n" \
                  "Copyright (c) 2006 L.A. Breyer. All rights reserved.\n" \
                  "This is free software, It comes with NO WARRANTY, and is licensed\n" \
                  "to you under the terms of the GNU General Public License Version 2 or later.\n"

#endif
