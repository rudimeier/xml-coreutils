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
#include <string.h>

#include "cstring.h"

/* static char_t *dtd = ""; */

#define STD_ROOT_TAG   "root"
#define STD_OPEN_ROOT  "<root>"
#define STD_CLOSE_ROOT "</root>"
#define STD_HEADWRAP   "<?xml version=\"1.0\"?>\n"
#define STD_FOOTWRAP   "\n"

static char_t *std_root_tag = STD_ROOT_TAG;
static char_t *std_open_root = STD_OPEN_ROOT;
static char_t *std_close_root = STD_CLOSE_ROOT;

static char_t *std_headwrap = STD_HEADWRAP;
static char_t *std_footwrap = STD_FOOTWRAP;

static CSTRING(root_tag,STD_ROOT_TAG);
static CSTRING(open_root,STD_OPEN_ROOT);
static CSTRING(close_root,STD_CLOSE_ROOT);

static CSTRING(headwrap,STD_HEADWRAP);
static CSTRING(footwrap,STD_FOOTWRAP);

const char_t *get_headwrap() {
  return p_cstring(&headwrap);
}

const char_t *get_footwrap() {
  return p_cstring(&footwrap);
}

const char_t *get_open_root() {
  return p_cstring(&open_root);
}

const char_t *get_close_root() {
  return p_cstring(&close_root);
}

const char_t *get_root_tag() {
  return p_cstring(&root_tag);
}

void set_root_tag(const char_t *root) {
  if( root ) {
    if( p_cstring(&root_tag) != std_root_tag ) { 
      free_cstring(&root_tag); 
    }
    strdup_cstring(&root_tag, root);

    if( p_cstring(&open_root) != std_open_root ) { 
      free_cstring(&open_root); 
    }
    create_cstring(&open_root, "", 3 + strlen(root));
    vstrcat_cstring(&open_root, "<", root, ">", NULLPTR);

    if( p_cstring(&close_root) != std_close_root ) {
      free_cstring(&close_root); 
    }
    create_cstring(&close_root, "", 4 + strlen(root));
    vstrcat_cstring(&close_root, "</", root, ">", NULLPTR);
  }
}

void set_headwrap(char_t *wrap) {
  if( wrap ) {
    if( p_cstring(&headwrap) != std_headwrap ) { 
      free_cstring(&headwrap); 
    }
    strdup_cstring(&headwrap, wrap);
  }
}

void set_footwrap(char_t *wrap) {
  if( wrap ) {
    if( p_cstring(&footwrap) != std_footwrap ) { 
      free_cstring(&footwrap); 
    }
    strdup_cstring(&footwrap, wrap);
  }
}
