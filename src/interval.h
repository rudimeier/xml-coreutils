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

#ifndef INTERVAL_H
#define INTERVAL_H

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif
#include "objstack.h"

typedef struct {
  int a, b;
} interval_t;

typedef struct {
  objstack_t iset;
} intervalmgr_t;

bool_t create_intervalmgr(intervalmgr_t *im);
bool_t free_intervalmgr(intervalmgr_t *im);
bool_t push_intervalmgr(intervalmgr_t *im, int a, int b);
bool_t pop_intervalmgr(intervalmgr_t *im);
bool_t peek_intervalmgr(intervalmgr_t *im, int *a, int *b);
bool_t memberof_intervalmgr(intervalmgr_t *im, int x);
bool_t is_empty_intervalmgr(intervalmgr_t *im);

#endif
