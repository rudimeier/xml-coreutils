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

#ifndef TEMPVAR_H
#define TEMPVAR_H

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "tempcollect.h"
#include <stdio.h>

/* A tempvar_t is a general purpose growable byte container which
   is not file backed, ie has a maximum memory footprint.
   It is simply a tempcollect_t where the writing functions are
   filtered to prevent a tempfile forming. However, it also
   has new other member functions which allow direct random access to 
   the memory, not just by streaming.

   It is useful as a temporary workspace. It is also possible
   to cast a tempvar_t into a tempcollect_t if you want to 
   use the tempcollect_t member functions yourself.
 */
typedef struct {
  tempcollect_t tc; /* poor man's inheritance */
} tempvar_t;


bool_t create_tempvar(tempvar_t *tv, const char_t *name, 
		      size_t buflen, size_t maxlen);
bool_t free_tempvar(tempvar_t *tv);
bool_t reset_tempvar(tempvar_t *tv);

bool_t write_tempvar(tempvar_t *tv, const byte_t *buf, size_t buflen);
bool_t puts_tempvar(tempvar_t *tv, const char_t *s);
bool_t chomp_tempvar(tempvar_t *tv);

bool_t putc_tempvar(tempvar_t *tv, char_t c);
bool_t squeeze_tempvar(tempvar_t *tv, const char_t *buf, size_t buflen);

bool_t read_from_file_tempvar(tempvar_t *tv, const char_t *file);
bool_t write_to_file_tempvar(tempvar_t *tv, const char_t *file, int openflags);

bool_t peeks_tempvar(tempvar_t *tv, size_t pos, const char_t **s);
bool_t shift_tempvar(tempvar_t *tv, size_t pos);

const char_t *string_tempvar(tempvar_t *tv);
long int long_tempvar(tempvar_t *tv);
unsigned long int ulong_tempvar(tempvar_t *tv);
double double_tempvar(tempvar_t *tv);

__inline__ static bool_t is_empty_tempvar(tempvar_t *tv) {
  return !tv || (tv->tc.bufpos == 0);
}

__inline__ static void swap_tempvar(tempvar_t *ps1, tempvar_t *ps2) {
  swap_tempcollect((tempcollect_t *)ps1, (tempcollect_t *)ps2);
}


#endif
