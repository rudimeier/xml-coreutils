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

#ifndef ERROR_H
#define ERROR_H

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#define E_ERROR 0
#define E_WARNING 1
#define E_FATAL 2
void errormsg(int type, const char *fmt, ...)
#ifdef __GNUC__
__attribute__((format (printf, 2, 3)))
#endif
;
void debug(const char *fmt, ...)
#ifdef __GNUC__
__attribute__((format (printf, 1, 2)))
#endif
;

#endif
