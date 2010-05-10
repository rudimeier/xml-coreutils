/* 
 * Copyright (C) 2010 Laird Breyer
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

#ifndef HTFILTER_H
#define HTFILTER_H

#include "common.h"
#include "parser.h"
#include "xpath.h"

#define HTFILTER_DUMB   0x01
#define HTFILTER_HTML   0x02
#define HTFILTER_DEBUG  0x04

typedef struct {
  xpath_t xpath;
  int depth;
  const char_t *section;
} filter_data_t;

typedef struct {
  parser_t parser;
  byte_t *buf, *p;
  size_t buflen;
  flag_t flags;

  filter_data_t filter;
} htfilter_t;

bool_t create_htfilter(htfilter_t *f, flag_t flags);
bool_t free_htfilter(htfilter_t *f);

bool_t initialize_htfilter(htfilter_t *f);
bool_t finalize_htfilter(htfilter_t *f);

bool_t write_htfilter(htfilter_t *f, const char_t *buf, size_t buflen);
bool_t flush_htfilter(htfilter_t *f);

#endif
