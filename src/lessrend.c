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

#include "common.h"
#include "lessrend.h"
#include "entities.h"
#include "io.h"
#include <slang.h>
#include <string.h>

bool_t create_renderer(renderer_t *r, display_t *disp) {
  if( r && disp ) {
    reset_renderer(r);
    create_attlist(&r->alist);
    r->num_rows = disp->num_rows;
    r->num_cols = disp->num_cols;
    r->pivot = disp->pivot;
    return TRUE;
  }
  return FALSE;
}

bool_t reset_renderer(renderer_t *r) {
  if( r ) {
    r->flags = 0;
    r->row = 0;
    r->col = 0;
    r->colour = 0;
    r->indent = 0;
    r->pivot = 0;
    r->nodecount = 0;
    r->byteoffset = 0;
    r->bytesize = 0;
    r->margin = 0;
    reset_attlist(&r->alist);
    return TRUE;
  }
  return FALSE;
}

bool_t free_renderer(renderer_t *r) {
  if( r ) {
    free_attlist(&r->alist);
    end_screen_renderer(r);
    return TRUE;
  }
  return FALSE;
}

bool_t update_renderer(renderer_t *r) {
  if( r && !checkflag(r->flags, RENDERER_DONE) ) {
    if( r->row >= r->num_rows ) {
      setflag(&r->flags, RENDERER_DONE);
    }
  }
  return TRUE;
}

bool_t is_done_renderer(renderer_t *r) {
  return (r ? checkflag(r->flags, RENDERER_DONE) : TRUE);
}

bool_t erase_eos_renderer(renderer_t *r) {
  if( r && !checkflag(r->flags, RENDERER_DONE) ) {
    SLsmg_set_color(0);
    SLsmg_erase_eos();
    SLsmg_set_color(r->colour);
    return TRUE;
  }
  return FALSE;
}

bool_t erase_eol_renderer(renderer_t *r) {
  if( r && !checkflag(r->flags, RENDERER_DONE) ) {
    SLsmg_set_color(0);
    SLsmg_erase_eol();
    SLsmg_set_color(r->colour);
    return TRUE;
  }
  return FALSE;
}

bool_t end_screen_renderer(renderer_t *r) {
  if( r && !checkflag(r->flags, RENDERER_DONE) ) {
    erase_eos_renderer(r);
    setflag(&r->flags, RENDERER_DONE);
    return TRUE;
  }
  return FALSE;
}

bool_t start_line_renderer(renderer_t *r) {
  if( r && !checkflag(r->flags, RENDERER_DONE) ) {
    if( r->col > 0 ) {
      /* assume line has already been filled, so open next line */
      if( r->col < r->num_cols ) {
	SLsmg_gotorc(r->row, r->col);
	erase_eol_renderer(r);
      }
      r->row++;
    }
    fill_margin_renderer(r);
    write_repeat_renderer(r, r->indent - r->margin, ' ', c_indent);
    return update_renderer(r);
  }
  return FALSE;
}

bool_t set_filesize_renderer(renderer_t *r, off_t s) {
  if( r && !checkflag(r->flags, RENDERER_DONE) ) {
    r->bytesize = s;
    return update_renderer(r);
  }
  return FALSE;
}

bool_t set_offset_renderer(renderer_t *r, off_t o) {
  if( r && !checkflag(r->flags, RENDERER_DONE) ) {
    r->byteoffset = o;
    return update_renderer(r);
  }
  return FALSE;
}

bool_t set_margin_renderer(renderer_t *r, int margin) {
  if( r && !checkflag(r->flags, RENDERER_DONE) ) {
    r->margin = MIN(margin, r->num_cols);
    return update_renderer(r);
  }
  return FALSE;
}

bool_t set_offsets_renderer(renderer_t *r, off_t off, int nord) {
  if( r && !checkflag(r->flags, RENDERER_DONE) ) {
    r->byteoffset = off;
    r->nodecount = nord;
    setflag(&r->flags, RENDERER_NEW_OFFSETS);
    return update_renderer(r);
  }
  return FALSE;
}

bool_t set_indent_renderer(renderer_t *r, int n) {
  if( r && !checkflag(r->flags, RENDERER_DONE) ) {
    if( n >= 0 ) {
      r->indent = r->margin + MIN(n, r->num_cols - r->margin);
    }
    return update_renderer(r);
  }
  return FALSE;
}

bool_t set_wordwrap_renderer(renderer_t *r, bool_t yes) {
  if( r && !checkflag(r->flags, RENDERER_DONE) ) {
    flipflag(&r->flags, RENDERER_WORDWRAP, yes);
    if( yes ) { flipflag(&r->flags, RENDERER_LINEWRAP, yes); }
    return update_renderer(r);
  }
  return FALSE;
}

bool_t set_linewrap_renderer(renderer_t *r, bool_t yes) {
  if( r && !checkflag(r->flags, RENDERER_DONE) ) {
    flipflag(&r->flags, RENDERER_LINEWRAP, yes);
    return update_renderer(r);
  }
  return FALSE;
}

bool_t set_raw_colour_renderer(renderer_t *r, int c) {
  if( r ) {
    SLsmg_set_color(c);
    r->colour = c;
    return TRUE;
  }
  return FALSE;
}

bool_t set_colour_renderer(renderer_t *r, colour_t c) {
  if( c < c_last ) {
    return set_raw_colour_renderer(r, c);
  }
  return FALSE;
}


bool_t fill_margin_renderer(renderer_t *r) {
  int c;
  if( r && !checkflag(r->flags, RENDERER_DONE) && (r->margin > 0) ) {
    c = r->colour;
    r->col = 0;
    SLsmg_gotorc(r->row, r->col);
    set_colour_renderer(r, c_margin);
    if( checkflag(r->flags, RENDERER_NEW_OFFSETS) ) {
      if( checkflag(r->flags, RENDERER_SHOW_NODECOUNTS) ) {
	printf_renderer(r, "%*d", r->margin - 1, r->nodecount);
      } else if( checkflag(r->flags, RENDERER_SHOW_OFFSETS) ) {
	printf_renderer(r, "%0*d", r->margin - 1, r->byteoffset);
      }
    }
    set_raw_colour_renderer(r, c);
    if( r->col <= r->margin ) {
      write_repeat_renderer(r, r->margin - r->col, ' ', c_margin);
    } else {
      r->col = r->margin - 1;
      SLsmg_gotorc(r->row, r->col);
      puts_renderer(r, "+");
    }
    return update_renderer(r);
  }
  return FALSE;
}

bool_t write_repeat_renderer(renderer_t *r, int count, char_t c, colour_t l) {
  int i;
  int rl;
  if( r && !checkflag(r->flags, RENDERER_DONE) ) {
    rl = r->colour;
    set_colour_renderer(r, l);
    for(i = 0; i < count; i++) {
      SLsmg_write_char(c);
      r->col++;
    }
    set_raw_colour_renderer(r, rl);
    return update_renderer(r);
  }
  return FALSE;
}

bool_t write_linecut_renderer(renderer_t *r, const char_t *begin, const char_t *end) {
  int n;
  if( r && (r->col < r->num_cols) ) {
    n = MIN(r->num_cols - r->col, end - begin);
    SLsmg_write_nchars((char *)begin, n);
    r->col += n;
    return update_renderer(r);
  }
  return FALSE;
}

bool_t write_linewrap_renderer(renderer_t *r, const char_t *begin, const char_t *end) {
  int n;
  if( r ) {
    while( (begin < end) && !checkflag(r->flags, RENDERER_DONE) ) {
      if( r->col >= r->num_cols ) {
	r->row++;
	fill_margin_renderer(r);
      }
      n = MIN(r->num_cols - r->col, end - begin);
      SLsmg_write_nchars((char *)begin, n);
      begin += n;
      r->col += n;
    }
    return update_renderer(r);
  }
  return FALSE;
}

bool_t write_wordwrap_renderer(renderer_t *r, const char_t *begin, const char_t *end) {
  int n;
  const char_t *s;
  if( r ) {
    while( (begin < end) && !checkflag(r->flags, RENDERER_DONE) ) {
      n = MIN(r->num_cols - r->col, end - begin);
      s = rfind_xml_whitespace(begin, begin + n);
      if( s > begin ) {
	n = s - begin;
      }
      if( r->col + n >= r->num_cols ) {
	erase_eol_renderer(r);
	r->row++;
	fill_margin_renderer(r);
      }
      SLsmg_write_nchars((char *)begin, n);
      begin += n;
      r->col += n;
    }
    return update_renderer(r);
  }
  return FALSE;
}

bool_t write_renderer(renderer_t *r, const char_t *begin, const char_t *end) {
  if( r && !checkflag(r->flags, RENDERER_DONE) ) {
    if( !checkflag(r->flags, RENDERER_LINEWRAP) ) {
	return write_linecut_renderer(r, begin, end);
    } else {
      if( checkflag(r->flags, RENDERER_WORDWRAP) ) {
	return write_wordwrap_renderer(r, begin, end);
      } else {
	return write_linewrap_renderer(r, begin, end);
      }
    }
  }
  return FALSE;
}

bool_t puts_renderer(renderer_t *r, const char_t *s) {
  return write_renderer(r, s, s + strlen(s));
}


bool_t write_attributes_renderer(renderer_t *r, attlist_t *alist) {
  size_t i;
  const attribute_t *a;
  if( r && alist && !checkflag(r->flags, RENDERER_DONE) ) {
    for(i = 0; i < alist->count; i++) {
      a = get_attlist(alist, i);
      if( a && a->name && a->value ) {
	puts_renderer(r, " ");
	set_colour_renderer(r, c_att_name);
	puts_renderer(r, a->name);
	set_colour_renderer(r, c_tag);
	puts_renderer(r, "=");
	set_colour_renderer(r, c_att_value);
	puts_renderer(r, "\"");
	puts_renderer(r, a->value);
	puts_renderer(r, "\"");
	set_colour_renderer(r, c_tag);
      }
    }
    return update_renderer(r);
  }
  return FALSE;
}

bool_t write_squeeze_renderer(renderer_t *r, const char_t *begin, const char_t *end) {
  const char_t *e = begin;
  const char_t *sep = " "; 
  while( begin && (begin < end) ) {
    begin = skip_xml_whitespace(e, end);
    e = find_xml_whitespace(begin, end);
    write_renderer(r, begin, e);
    if( e < end) { 
      puts_renderer(r, sep); 
    }
  }
  return update_renderer(r);
}

bool_t set_autocolour_renderer(renderer_t *r, const char_t *s) {
  int c;
  if( r && s ) {
    c = hash((unsigned char *)s, strlen(s), 0) & ((1<<DISPLAY_AUTO_BITS)-1);
    return set_raw_colour_renderer(r, c_last + c);
  }
  return FALSE;
}

/* warning: never use this for big strings, as it cannot wrap lines properly 
 * (useful for debugging).
 */
bool_t printf_renderer(renderer_t *r, char *fmt, ...) {
  va_list vap;
  if( r && !checkflag(r->flags, RENDERER_DONE) ) {
#if HAVE_VPRINTF
    va_start(vap, fmt);
    SLsmg_vprintf(fmt, vap);
    va_end(vap);
#else
    SLsmg_printf("%s", fmt);
#endif
    r->row = SLsmg_get_row();
    r->col = SLsmg_get_column();
    return update_renderer(r);
  }
  return FALSE;
}

