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

#include "common.h"

char_t *root = "root";
char_t *dtd = "";

char_t *standard_headwrap = "<?xml version=\"1.0\"?>\n<root>";
char_t *standard_footwrap = "</root>\n";

char_t *get_headwrap() {
  return standard_headwrap;
}

char_t *get_footwrap() {
  return standard_footwrap;
}

char_t *get_root_tag() {
  return root;
}
