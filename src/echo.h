/* 
 * Copyright (C) 2009 Laird Breyer
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

#ifndef ECHO_H
#define ECHO_H

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "xpath.h"
#include "cstring.h"

typedef struct {
  xpath_t xpath; /* current path */
  xpath_t tmpath; /* temp path */
  int depth;
  int indentdepth; /* pivot */
  flag_t flags;
  cstring_t root; /* saved root tag (optional) */
} echo_t;

/* echo indents automatically all strings whose depth is
 * smaller than the pivot, and doesn't indent the strings
 * whose depth is greater. So if you want to disable all
 * indenting, set the pivot to zero, and if you want universal
 * indenting, set the pivot to infinity.
 */
#define ECHO_INDENTALL   32767
#define ECHO_INDENTNONE  0

#define ECHO_FLAG_REINDENT   0x01 /* obsolete */
#define ECHO_FLAG_CDATA      0x02
#define ECHO_FLAG_SUPPRESSNL 0x04
#define ECHO_FLAG_HASROOT    0x08
#define ECHO_FLAG_COMMENT    0x10

bool_t create_echo(echo_t *e, size_t indentdepth, flag_t flags);
bool_t free_echo(echo_t *e);

bool_t open_stdout_echo(echo_t *e, const char_t *root);
bool_t close_stdout_echo(echo_t *e);

void puts_stdout_echo(echo_t *e, const char_t *s);
bool_t open_abspath_stdout_echo(echo_t *e, xpath_t *fullpath);
bool_t open_relpath_stdout_echo(echo_t *e, xpath_t *relpath);
bool_t close_path_stdout_echo(echo_t *e);

#endif
