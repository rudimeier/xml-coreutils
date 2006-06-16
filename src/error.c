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

#include "error.h"
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>

char *progname = "";
char *inputfile = "";
long inputline = 0;

void debug(const char *fmt, ...) {
  va_list vap;
#if HAVE_VPRINTF
  va_start(vap, fmt);
  vfprintf(stderr, fmt, vap);
  va_end(vap);
#else
  fprintf(stderr, "%s", fmt);
#endif
}

void errormsg(int etype, const char *fmt, ...) {
  va_list vap;

  switch(etype) {
  case E_WARNING:
    fprintf(stderr, "%s: warning: ", progname);
    break;
  default:
  case E_ERROR:
  case E_FATAL:
    fprintf(stderr, "%s: error: ", progname);
    break;
  }

#if HAVE_VPRINTF
  va_start(vap, fmt);
  vfprintf(stderr, fmt, vap);
  va_end(vap);
#else
  fprintf(stderr, "%s", fmt);
#endif

  if( etype == E_FATAL ) { 
    exit(EXIT_FAILURE);
  }
}
