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

#ifndef TEMPCOLLECT_H
#define TEMPCOLLECT_H

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <stdio.h>

/* A tempcollect_t is a general purpose byte container. It is ideal
   for temporarily saving a possibly very long string before printing
   it later. The contents are file backed if necessary, so the memory
   is essentially unlimited. However, files cause complications, so 
   the facilities for editing the string in-place are not great, and
   it's best to think of this as a string _stream_ .
 */
typedef struct {
  const char_t *name;
  byte_t *buf;
  size_t bufpos;
  size_t buflen;
  size_t max_buflen;
  FILE *tempfile;
} tempcollect_t;

typedef bool_t (tempcollect_adapter_fun)(void *user, byte_t *buf, size_t buflen);
typedef struct {
  tempcollect_adapter_fun *fun;
  void *user;
} tempcollect_adapter_t;

typedef struct {
  tempcollect_t *list;
  size_t num;
  size_t max;
} tclist_t;

bool_t create_tempcollect(tempcollect_t *tc, const char_t *name, 
			  size_t buflen, size_t maxlen);
bool_t free_tempcollect(tempcollect_t *tc);
bool_t reset_tempcollect(tempcollect_t *tc);
size_t tell_tempcollect(tempcollect_t *tc);
bool_t reserve_tempcollect(tempcollect_t *tc, int buflen);
bool_t truncate_tempcollect(tempcollect_t *tc, size_t newlen);

bool_t write_tempcollect(tempcollect_t *tc, const byte_t *buf, size_t buflen);
bool_t puts_tempcollect(tempcollect_t *tc, const char_t *s);
bool_t putc_tempcollect(tempcollect_t *tc, char_t c);
bool_t nputc_tempcollect(tempcollect_t *tc, char_t c, size_t n);
bool_t nprintf_tempcollect(tempcollect_t *tc, int size, const char_t *fmt, ...)
#ifdef __GNUC__
  __attribute__((format (printf, 3, 4)))
#endif
;

bool_t squeeze_tempcollect(tempcollect_t *tc, const char_t *buf, size_t buflen);
bool_t write_start_tag_tempcollect(tempcollect_t *tc, const char_t *name, const char_t **att);
bool_t write_end_tag_tempcollect(tempcollect_t *tc, const char_t *name);
bool_t write_coded_entities_tempcollect(tempcollect_t *tc, const char_t *buf, size_t buflen);

bool_t open_file_tempcollect(tempcollect_t *tc);
bool_t close_file_tempcollect(tempcollect_t *tc);
bool_t flush_file_tempcollect(tempcollect_t *tc);
bool_t load_file_tempcollect(tempcollect_t *tc);
bool_t rewind_file_tempcollect(tempcollect_t *tc);

bool_t is_empty_tempcollect(tempcollect_t *tc);
/* peeking only works if no tempfile is present */
bool_t peek_tempcollect(tempcollect_t *tc, size_t start, size_t end,
			const byte_t **bufstart, const byte_t **bufend);
bool_t peeks_tempcollect(tempcollect_t *tc, size_t pos, const char_t **s);

bool_t write_adapter_tempcollect(tempcollect_t *tc, tempcollect_adapter_t *a);
bool_t write_stdout_tempcollect(tempcollect_t *tc);
bool_t squeeze_stdout_tempcollect(tempcollect_t *tc);

bool_t read_from_file_tempcollect(tempcollect_t *tc, const char_t *file);
bool_t write_to_file_tempcollect(tempcollect_t *tc, 
				 const char_t *file, int openflags);

bool_t create_tclist(tclist_t *tcl);
bool_t free_tclist(tclist_t *tcl);
bool_t reset_tclist(tclist_t *tcl);
bool_t add_tclist(tclist_t *tcl, const char_t *name, size_t buflen, size_t maxlen);
tempcollect_t *get_tclist(tclist_t *tcl, size_t n);
tempcollect_t *get_last_tclist(tclist_t *tcl);
tempcollect_t *find_byname_tclist(tclist_t *tcl, const char_t *name);

/* this doesn't swap the memory areas, only their pointers. 
   this may or may not be what you want */
__inline__ static void swap_tempcollect(tempcollect_t *ps1, tempcollect_t *ps2) {
  tempcollect_t tmp;
  const char_t *p;

  /* swap names twice, good for debugging */
  p = ps1->name;
  ps1->name = ps2->name;
  ps2->name = p;

  tmp = *ps1;
  *ps1 = *ps2;
  *ps2 = tmp;
}

#endif
