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

#ifndef LESSREND_H
#define LESSREND_H

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "lessdisp.h"
#include "attributes.h"

#define RENDERER_DONE              0x0001
#define RENDERER_ATTRIBUTES        0x0002
#define RENDERER_LINEWRAP          0x0004
#define RENDERER_WORDWRAP          0x0008
#define RENDERER_SHOW_OFFSETS      0x0010
#define RENDERER_SHOW_NODECOUNTS   0x0020
#define RENDERER_NEW_OFFSETS       0x0040

typedef struct {
  unsigned long flags;
  int row;
  int col;
  int colour;
  int indent;
  int pivot;
  int nodecount;
  off_t byteoffset;
  off_t bytesize;
  attlist_t alist;
  int num_rows;
  int num_cols;
  int margin;
} renderer_t;

bool_t create_renderer(renderer_t *r, display_t *disp);
bool_t reset_renderer(renderer_t *r);
bool_t free_renderer(renderer_t *r);

bool_t is_done_renderer(renderer_t *r);
bool_t start_line_renderer(renderer_t *r);
bool_t end_screen_renderer(renderer_t *r);
bool_t set_indent_renderer(renderer_t *r, int n);
bool_t set_wordwrap_renderer(renderer_t *r, bool_t yes);
bool_t set_linewrap_renderer(renderer_t *r, bool_t yes);
bool_t set_colour_renderer(renderer_t *r, colour_t c);
bool_t set_autocolour_renderer(renderer_t *r, const char_t *s);
bool_t set_offsets_renderer(renderer_t *r, off_t off, int nord);
bool_t set_filesize_renderer(renderer_t *r, off_t s);
bool_t set_margin_renderer(renderer_t *r, int margin);

bool_t erase_eol_renderer(renderer_t *r);
bool_t erase_eos_renderer(renderer_t *r);

bool_t write_renderer(renderer_t *r, const char_t *begin, const char_t *end);
bool_t write_repeat_renderer(renderer_t *r, int count, char_t c, colour_t l);
bool_t write_squeeze_renderer(renderer_t *r, const char_t *begin, const char_t *end);
bool_t puts_renderer(renderer_t *r, const char_t *s);
bool_t printf_renderer(renderer_t *r, char *fmt, ...);

bool_t write_attributes_renderer(renderer_t *r, attlist_t *alist);
bool_t fill_margin_renderer(renderer_t *r);

#endif
