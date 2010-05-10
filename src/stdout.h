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

#ifndef STDOUT_H
#define STDOUT_H

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#define STDOUT_CHECKPARSER   0x01
#define STDOUT_DEBUG         0x02

bool_t open_stdout();
bool_t open_redirect_stdout(int fno); /* use instead of open_stdout only */
bool_t setup_stdout(flag_t flags);
bool_t write_stdout(const byte_t *buf, size_t buflen);
bool_t puts_stdout(const char_t *s);
bool_t putc_stdout(char_t c);
bool_t nputc_stdout(char_t c, int n);
bool_t nprintf_stdout(int size, const char_t *fmt, ...)
#ifdef __GNUC__
  __attribute__((format (printf, 2, 3)))
#endif
;
bool_t write_entity_stdout(char_t c);
bool_t write_start_tag_stdout(const char_t *name, const char_t **att, bool_t slash);
bool_t write_end_tag_stdout(const char_t *name);

bool_t write_coded_entities_stdout(const char_t *buf, size_t buflen);
bool_t write_unescaped_stdout(const char_t *buf, size_t buflen);
bool_t squeeze_stdout(const byte_t *buf, size_t buflen);
bool_t squeeze_coded_entities_stdout(const char_t *buf, size_t buflen);
bool_t backspace_stdout();
bool_t flush_stdout();
bool_t close_stdout();
bool_t redirect_stdout(int fno);

#endif
