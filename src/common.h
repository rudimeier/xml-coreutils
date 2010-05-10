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

#ifndef COMMON_H
#define COMMON_H

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <expat.h>

typedef unsigned char byte_t;

/* expat returns XML_Char strings which are either UTF-8 encoded or
   UTF-16 encoded. In UTF-8, only US-ASCII chars have the high order
   bit unset, whereas all other chars are longer than a single
   byte, and all the bytes have the high order bit set. 
   This has  little impact for us, most string manipulations
   can be done as if the characters are one bit
*/
typedef XML_Char char_t;

#ifdef XML_UNICODE
#error "Wrong sized XML_Char. This program requires expat library in UTF-8 mode.\n"
#endif

/* we want flags to be atomic wrt signals */
typedef unsigned int flag_t;
typedef int bool_t;

#define NULLPTR ((void *)NULL)

typedef const char **cstringlst_t;

#define FALSE ((bool_t)0)
#define TRUE  ((bool_t)1)

#define CMD_QUIT     0x01
#define CMD_ALRM     0x02
#define CMD_CHLD     0x04

/* not safe: can eval x, y twice */
#define MIN(x,y) ( (x) < (y) ? (x) : (y) )
#define MAX(x,y) ( (x) > (y) ? (x) : (y) )

#define MAXSTRINGSIZE  67108864 /* 64MB for a cstring_t */

#define MAXFILES    32768
#define INFDEPTH    32768
/* maximum variable/collector size. Beyond this, a collector goes
   to a file, and a tempvar won't accept any more data. Change
   this if memory is tight */
#define MINVARSIZE  64       /* 64 bytes */
#define MAXVARSIZE  MAXSTRINGSIZE

#define MKTMP_NUMTRIES 10

/* flag manip */
/* true if y bits are set in x */
__inline__ static bool_t checkflag(flag_t x, flag_t y) {
  return x & y;
}

/* set y bits in x */
__inline__ static void setflag(flag_t *x, flag_t y) {
  *x |= y;
}

/* clear y bits in x */
__inline__ static void clearflag(flag_t *x, flag_t y) {
  *x &= ~y;
}

/* true if y bits not set in x, immediately set y bits in x */
/* use if you want to execute some code once when flag is clear */
__inline__ static bool_t false_and_setflag(flag_t *x, flag_t y) {
  bool_t z = !checkflag(*x, y);
  return z ? (setflag(x, y), z) : z;
}

/* true if y bits set in x, immediately clear y bits in x */
__inline__ static bool_t true_and_clearflag(flag_t *x, flag_t y) {
  bool_t z = checkflag(*x, y);
  return z ? (clearflag(x, y), z) : z;
}

/* set or clear y bits in x, depending on boolean var */
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
                  "Copyright (c) 2007,2009,2010 L.A. Breyer. All rights reserved.\n" \
                  "This is free software, It comes with NO WARRANTY, and is licensed\n" \
                  "to you under the terms of the GNU General Public License Version 3 or later.\n"

void redefine_xpath_specials(const char_t *newspecials);

#endif
