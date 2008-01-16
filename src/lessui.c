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
#include "lessui.h"
#include "error.h"

#include <slang.h>

/* #define create_display(a) 1 */
/* #define free_display(a) 1 */

bool_t create_lessui(lessui_t *ui) {
  if( ui ) {
    if( create_cursor(&ui->cursor) &&
	create_cursormanager(&ui->cmg) &&
	create_cursorrep(&ui->cr) &&
	create_display(&ui->display) ) {
      return TRUE;
    }
  }
  return FALSE;
}

bool_t free_lessui(lessui_t *ui) {
  if( ui ) {
    free_display(&ui->display);
    free_cursorrep(&ui->cr);
    free_cursormanager(&ui->cmg);
    free_cursor(&ui->cursor);
    return TRUE;
  }
  return FALSE;
}

bool_t reset_lessui(lessui_t *ui) {
  if( ui ) {
    reset_cursorrep(&ui->cr);
    reset_cursormanager(&ui->cmg);
    reset_cursor(&ui->cursor);
    ui->dirty = TRUE;
    ui->display.flags = 0;
    return TRUE;
  }
  return FALSE;
}

bool_t forward_lessui(lessui_t *ui, fbparser_t *fbp) {
  if( ui && fbp ) {
    if( ui->display.pivot > 0 ) {
      return next_pivot_cursormanager(&ui->cmg, &ui->cursor, fbp, ui->display.pivot);
    } else {
      return next_cursormanager(&ui->cmg, &ui->cursor, fbp);
    }
  }
  return FALSE;
}

bool_t back_lessui(lessui_t *ui, fbparser_t *fbp) {
  if( ui && fbp ) {
    if( ui->display.pivot > 0 ) {
      return prev_pivot_cursormanager(&ui->cmg, &ui->cursor, fbp, ui->display.pivot);
    } else {
      return prev_cursormanager(&ui->cmg, &ui->cursor, fbp);
    }
  }
  return FALSE;
}

bool_t next_sibling_lessui(lessui_t *ui, fbparser_t *fbp) {
  if( ui && fbp ) {
    return next_sibling_cursormanager(&ui->cmg, &ui->cursor, fbp);
  }
  return FALSE;
}

bool_t prev_sibling_lessui(lessui_t *ui, fbparser_t *fbp) {
  if( ui && fbp ) {
    return prev_sibling_cursormanager(&ui->cmg, &ui->cursor, fbp);
  }
  return FALSE;
}

bool_t help_lessui(lessui_t *ui) {
  if( ui ) {
    do_help_display(&ui->display);
  }
  return FALSE;
}

bool_t show_cursor_lessui(lessui_t *ui, cursor_t *cursor, fbparser_t *fbp) {
/*   const char_t *where; */
  if( ui && cursor && fbp ) {
/*     if( build_cursorrep(&ui->cr, cursor, fbp) ) { */
/*       where = get_locator_cursorrep(&ui->cr); */
/*       redraw_locator_display(&ui->display, where); */
/*     } */
    redraw_locator_display(&ui->display, cursor);
  }
  return FALSE;
}


bool_t mainloop_lessui(lessui_t *ui, fbparser_t *fbp) {
  bool_t done = FALSE;

  ui->dirty = TRUE;
  while(!done) {

    if( ui->dirty ) {
/*       debug_cursor(&ui->cursor); */
      if( redraw_cursor_display(&ui->display, &ui->cursor, fbp) ) {
	if( ui->display.pivot > 0 ) {
	  /* when pivot is being cycled, it's easy to get confused,
	   so we need something to represent the cursor. For now
	  I'm drawing the cursor itself, but it's not user friendly */
	  show_cursor_lessui(ui, &ui->cursor, fbp);
	}
      }
      ui->dirty = FALSE;
    }

    switch(getcommand_display(&ui->display)) {
    case quit:
      done = 1;
      break;
    case forward:
      forward_lessui(ui, fbp);
      break;
    case backward:
      back_lessui(ui, fbp);
      break;
    case left:
      pivot_display(&ui->display, -1);
      break;
    case right:
      pivot_display(&ui->display, +1);
      break;
    case select1:
      toggle_display(&ui->display, DISPLAY_ATTRIBUTES);
      break;
    case select2:
      toggle_display(&ui->display, DISPLAY_WRAP);
      break;
    case next_sibling:
      next_sibling_lessui(ui, fbp);
      break;
    case prev_sibling:
      break;
    case help:
      help_lessui(ui);
      break;
    default:
      break;
    }
    ui->dirty = TRUE;
  }
  return done;
}
