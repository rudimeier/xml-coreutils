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

#ifndef STDPRINT_H
#define STDPRINT_H

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "stdparse.h"

bool_t stdprint_start_tag(stdparserinfo_t *sp, 
			  const char_t *name, const char_t **att);
bool_t stdprint_end_tag(stdparserinfo_t *sp, const char_t *name);
bool_t stdprint_chardata(stdparserinfo_t *sp, const char_t *buf, size_t buflen);
bool_t stdprint_start_cdata(stdparserinfo_t *sp);
bool_t stdprint_end_cdata(stdparserinfo_t *sp);
bool_t stdprint_comment(stdparserinfo_t *sp, const char_t *data);
bool_t stdprint_dfault(stdparserinfo_t *sp, const char_t *data, size_t buflen);
bool_t stdprint_pidata(stdparserinfo_t *sp, const char_t *target, const char_t *data);
bool_t stdprint_pop_newline(stdparserinfo_t *sp);
bool_t stdprint_pop_verbatim(stdparserinfo_t *sp);

bool_t stdprint_unwind(xpath_t *xp);

#endif
