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

#include "common.h"
#include "format.h"


char_t convert_backslash(const char_t *p) {
  if( !p ) {
    return (char_t)0;
  }
  if( *p != '\\' ) {
    return *p;
  }
  p++;
  switch(*p) {
  case 'a':
    return (char_t)'\a';
  case 'b':
    return (char_t)'\b';
  case 'f':
    return (char_t)'\f';
  case 'n':
    return (char_t)'\n';
  case 't':
    return (char_t)'\t';
  case 'v':
    return (char_t)'\v';
  default:
    return *p;
  }
}
