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

#ifndef TEMPCOLLECT_H
#define TEMPCOLLECT_H

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

typedef struct {
  const char_t *name;
  byte_t *buf;
  size_t bufpos;
  size_t buflen;
  size_t max_buflen;
} tempcollect_t;

typedef struct {
  tempcollect_t *list;
  size_t num;
  size_t max;
} tclist_t;

bool_t create_tempcollect(tempcollect_t *tc, const char_t *name, 
			  size_t buflen, size_t maxlen);
bool_t free_tempcollect(tempcollect_t *tc);
bool_t reset_tempcollect(tempcollect_t *tc);
bool_t write_tempcollect(tempcollect_t *tc, const byte_t *buf, size_t buflen);
bool_t write_stdout_tempcollect(tempcollect_t *tc);
bool_t squeeze_stdout_tempcollect(tempcollect_t *tc);
bool_t read_tempcollect(tempcollect_t *tc, byte_t *buf, size_t buflen);

bool_t create_tclist(tclist_t *tcl);
bool_t free_tclist(tclist_t *tcl);
bool_t reset_tclist(tclist_t *tcl);
bool_t add_tclist(tclist_t *tcl, const char_t *name, size_t buflen, size_t maxlen);
tempcollect_t *get_tclist(tclist_t *tcl, size_t n);
tempcollect_t *get_last_tclist(tclist_t *tcl);
tempcollect_t *find_byname_tclist(tclist_t *tcl, const char_t *name);

#endif
