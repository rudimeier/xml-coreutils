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

#ifndef WRAP_H
#define WRAP_H

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

const char_t *get_open_root();
const char_t *get_close_root();
const char_t *get_root_tag();

const char_t *get_headwrap();
const char_t *get_footwrap();

void set_root_tag(const char_t *root);
void set_headwrap(const char_t *wrap);
void set_footwrap(const char_t *wrap);

#endif
