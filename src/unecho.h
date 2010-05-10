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

#ifndef UNECHO_H
#define UNECHO_H

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "xpath.h"
#include "tempvar.h"

typedef struct {
  xpath_t cp; /* current path */
  tempvar_t sv; /* string value */
  flag_t flags;
} unecho_t;

#define UNECHO_FLAG_ABSOLUTE   0x01

bool_t create_unecho(unecho_t *ue, flag_t flags);
bool_t reset_unecho(unecho_t *ue, flag_t flags);
bool_t free_unecho(unecho_t *ue);
bool_t write_unecho(unecho_t *ue, 
		    const xpath_t *xpath, 
		    const byte_t *buf, size_t buflen);
bool_t reconstruct_unecho(unecho_t *ue, 
			  const xpath_t *xpath, 
			  const byte_t *buf, size_t buflen);
bool_t squeeze_unecho(unecho_t *ue, 
		      const xpath_t *xpath, 
		      const byte_t *buf, size_t buflen);

#endif
