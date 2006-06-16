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

#ifndef STDOUT_H
#define STDOUT_H

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#define STDOUT_CHECKPARSER   0x01

bool_t open_stdout();
bool_t setup_stdout(flag_t flags);
bool_t write_stdout(const byte_t *buf, size_t buflen);
bool_t puts_stdout(const char_t *s);
bool_t putc_stdout(char_t c);
bool_t nputc_stdout(char_t c, size_t n);
bool_t write_entity_stdout(char_t c);

bool_t squeeze_stdout(const byte_t *buf, size_t buflen);
bool_t backspace_stdout();
bool_t flush_stdout();
bool_t close_stdout();

#endif
