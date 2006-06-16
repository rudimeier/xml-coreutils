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

#ifndef ENTITIES_H
#define ENTITIES_H

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

const char_t *get_entity(char_t c);
const char_t *find_next_special(const char_t *begin, const char_t *end);
__inline__ static bool_t xml_whitespace(char_t c) {
  return ((c == 0x20) || (c == 0x09) || (c == 0x0D) || (c == 0x0A));
}

const char_t *skip_xml_whitespace(const char_t *begin, const char_t *end);
bool_t is_xml_space(const char_t *begin, const char_t *end);
const char_t *find_xml_whitespace(const char_t *begin, const char_t *end);
const char_t *rfind_xml_whitespace(const char_t *begin, const char_t *end);

#endif
