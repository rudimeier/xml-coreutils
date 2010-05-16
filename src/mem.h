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

#ifndef MEM_H
#define MEM_H

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

bool_t grow_mem(void *pptr, size_t *nmemb, size_t size, size_t minmemb);
bool_t create_mem(void *pptr, size_t *nmemb, size_t size, size_t minmemb);
bool_t free_mem(void *pptr, size_t *nmemb);
bool_t ensure_bytes_mem(int numbytes, void *pptr, size_t *nmemb, size_t size);

char_t *dup_string(const char_t *begin, const char_t *end);

/* Below are a few search functions for simple parsing of strings which are
 * not (necessarily) null terminated. 
 *
 * IMPORTANT: A char_t is either one or two bytes wide, so these functions
 * cannot be used to search for arbitrary UTF-8 characters, only for the 
 * subset of ASCII chars, which are always encoded as a single char. 
 *
 * But in normal cases, nearly all characters of interest are US-ASCII anyway,
 * which fits in a single byte.
 *
 * The functions are called skip_* because they are normally used to 
 * skip until the next occurrence of some character, or until the end of
 * the input string. The usual C search functions don't work on portions
 * of strings (no terminating null) and don't work with escaped characters.
 */
const char_t *skip_unescaped_delimiters(const char_t *begin, const char_t *end, const char_t *delims, char_t esc);
const char_t *skip_unescaped_substring(const char_t *begin, const char_t *end, const char_t *substr, char_t esc);
size_t parallel_skip_prefix(const char_t **pp, const char_t **qq, char_t delim, char_t esc);
const char_t *rskip_unescaped_delimiter(const char_t *begin, const char_t *end, const char_t delim, char_t esc);
const char_t *skip_delimiter(const char_t *begin, const char_t *end, char_t delim);
const byte_t *skip_byte(const byte_t *begin, const byte_t *end, byte_t delim);

size_t total_unescaped_delimiters(const char_t *begin, const char_t *end, const char_t *delims, char_t esc);

bool_t globmatch(const char_t *begin, const char_t *end, const char_t *glob);

#endif
