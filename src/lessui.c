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
#include "myerror.h"

#include <slang.h>

/* #define create_display(a) 1 */
/* #define free_display(a) 1 */

bool_t create_lessui(lessui_t *ui) {
  bool_t ok = TRUE;
  if( ui ) {
    ok &= create_cursor(&ui->cursor);
    ok &= create_cursormanager(&ui->cmg);
    ok &= create_cursorrep(&ui->cr);
    ok &= create_display(&ui->display);
    if( ok ) {

      /* toggle_display(&ui->display, DISPLAY_WRAP); */
      /* toggle_display(&ui->display, DISPLAY_ATTRIBUTES); */

    }
    return ok;
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

bool_t home_lessui(lessui_t *ui, fbparser_t *fbp) {
  if( ui && fbp ) {
    while( get_length_cursor(&ui->cursor) > 1 ) {
      parent_cursormanager(&ui->cmg, &ui->cursor, fbp);
    }
    return TRUE;
  }
  return FALSE;
}

bool_t end_lessui(lessui_t *ui, fbparser_t *fbp) {
  if( ui && fbp ) {
    return next_cursormanager(&ui->cmg, &ui->cursor, fbp, INT_MAX);
  }
  return FALSE;
}

bool_t forward_lessui(lessui_t *ui, fbparser_t *fbp, int count) {
  bool_t retval;
  if( ui && fbp ) {
    if( ui->display.pivot > 0 ) {
      return next_pivot_cursormanager(&ui->cmg, &ui->cursor, fbp, ui->display.pivot, count);
    } else {
      retval = next_cursormanager(&ui->cmg, &ui->cursor, fbp, count);
      /* debug_cursor(&ui->cursor); */
      return retval;
    }
  }
  return FALSE;
}

bool_t back_lessui(lessui_t *ui, fbparser_t *fbp, int count) {
  if( ui && fbp ) {
    if( ui->display.pivot > 0 ) {
      return prev_pivot_cursormanager(&ui->cmg, &ui->cursor, fbp, ui->display.pivot, count);
    } else {
      return prev_cursormanager(&ui->cmg, &ui->cursor, fbp, count);
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
    return do_help_display(&ui->display);
  }
  return FALSE;
}

bool_t colour_cycle_lessui(lessui_t *ui) {
  if( ui ) {
    return next_colour_scheme_display(&ui->display);
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
      forward_lessui(ui, fbp, 1);
      break;
    case backward:
      back_lessui(ui, fbp, 1);
      break;
    case indent:
      pivot_display(&ui->display, -1);
      break;
    case backindent:
      pivot_display(&ui->display, +1);
      break;
    case right:
      shift_display(&ui->display, +5);
      break;
    case left:
      shift_display(&ui->display, -5);
      break;
    case pgdown:
      forward_lessui(ui, fbp, 10);
      break;
    case pgup:
      back_lessui(ui, fbp, 10);
      break;
    case home:
      home_lessui(ui, fbp);
      break;
    case end:
      end_lessui(ui, fbp);
      break;
    case attributes:
      toggle_display(&ui->display, DISPLAY_ATTRIBUTES);
      break;
    case wordwrap:
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
    case colours:
      colour_cycle_lessui(ui);
      break;
    case refresh:
      refresh_fileblockparser(fbp);
      break;
    default:
      break;
    }
    ui->dirty = TRUE;
  }
  return done;
}
