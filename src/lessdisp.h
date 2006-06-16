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

#ifndef LESSDISP_H
#define LESSDISP_H

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "fbparser.h"

#define DISPLAY_ATTRIBUTES       0x01
#define DISPLAY_WRAP             0x02
#define DISPLAY_OFFSETS          0x04
#define DISPLAY_NODECOUNTS       0x08

#define DISPLAY_AUTO_BITS        3
typedef enum {
  c_default = 0, 
  c_tag, c_att_name, c_att_value, 
  c_chardata, 
  c_comment, 
  c_pi,
  c_cdata,
  c_status,
  c_indent,
  c_margin,
  c_other, 
  c_last
} colour_t;

typedef struct {
  int num_rows;
  int num_cols;
  int pivot;
  int max_visible_depth;
  flag_t flags;
} display_t;

typedef enum {
  nothing = 0, quit, help,
  forward, backward, 
  left, right,
  select1, select2,
  next_sibling, prev_sibling
} lessui_command_t;

bool_t create_display(display_t *disp);
bool_t reset_display(display_t *disp);
bool_t free_display(display_t *disp);

bool_t set_display(display_t *disp, long mask);
bool_t unset_display(display_t *disp, long mask);
bool_t toggle_display(display_t *disp, long mask);
bool_t pivot_display(display_t *ui, int delta);

bool_t open_display(display_t *disp);
bool_t close_display(display_t *disp);
bool_t redraw_cursor_display(display_t *disp, cursor_t *cursor, fbparser_t *fbp);
bool_t redraw_locator_display(display_t *disp, const cursor_t *cursor);
/* bool_t redraw_locator_display(display_t *disp, const char_t *rep); */

bool_t do_help_display(display_t *disp);

lessui_command_t getcommand_display(display_t *disp);
void errormsg_display(display_t *disp, int etype, const char *fmt, ...);


#endif
