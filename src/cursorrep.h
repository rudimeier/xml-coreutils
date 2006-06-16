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

#ifndef CURSORREP_H
#define CURSORREP_H

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "cursor.h"
#include "xpath.h"
#include "fbparser.h"

typedef struct {
  xpath_t xpath;
} cursorrep_t;

bool_t create_cursorrep(cursorrep_t *cr);
bool_t reset_cursorrep(cursorrep_t *cr);
bool_t free_cursorrep(cursorrep_t *cr);
bool_t build_cursorrep(cursorrep_t *cr, cursor_t *cursor, fbparser_t *fbp);
const char_t *get_locator_cursorrep(cursorrep_t *cr);


#endif
 
