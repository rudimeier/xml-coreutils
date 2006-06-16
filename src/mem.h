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

#ifndef MEM_H
#define MEM_H

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

bool_t grow_mem(byte_t **ptr, size_t *nmemb, size_t size, size_t minmemb);
bool_t create_mem(byte_t **ptr, size_t *nmemb, size_t size, size_t minmemb);
bool_t free_mem(byte_t **ptr, size_t *nmemb);
const char_t *find_unescaped_delimiter(const char_t *begin, const char_t *end, char_t delim, char_t esc);
const char_t *find_delimiter(const char_t *begin, const char_t *end, char_t delim);
const byte_t *find_byte(const byte_t *begin, const byte_t *end, byte_t delim);

#endif
