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
#include "lessdisp.h"
#include "lessrend.h"
#include "myerror.h"
#include "mem.h"
#include "mysignal.h"

#include <slang.h>
#include <string.h>
#include <math.h>

extern volatile flag_t cmd;

typedef struct {
  char *fg;
  char *bg;
  SLtt_Char_Type att;
} colourtype_t;

/* tags are randomly coloured based on their hashed name */
colourtype_t auto_colour[DISPLAY_COLOUR_SCHEMES][(1<<DISPLAY_AUTO_BITS)] = {
{
  {"gray", "default", SLTT_BOLD_MASK},
  {"red", "default", SLTT_BOLD_MASK},
  {"green", "default", SLTT_BOLD_MASK},
  {"blue", "default", SLTT_BOLD_MASK},
  {"brown", "default", SLTT_BOLD_MASK},
  {"cyan", "default", SLTT_BOLD_MASK},
  {"magenta", "default", SLTT_BOLD_MASK},
  {"gray", "default", SLTT_BOLD_MASK},
},
{
  {"blue", "default", SLTT_BOLD_MASK},
  {"gray", "default", SLTT_BOLD_MASK},
  {"magenta", "default", SLTT_BOLD_MASK},
  {"green", "default", SLTT_BOLD_MASK},
  {"brown", "default", SLTT_BOLD_MASK},
  {"gray", "default", SLTT_BOLD_MASK},
  {"red", "default", SLTT_BOLD_MASK},
  {"cyan", "default", SLTT_BOLD_MASK},
}
};

/* various ui colour schemes */
colourtype_t ui_colour[DISPLAY_COLOUR_SCHEMES][c_last] = {
{
  {"lightgray", "default", 0}, /* c_default */
  {"green", "default", 0}, /* c_tag */
  {"lightgray", "default", SLTT_ULINE_MASK}, /* c_att_name */
  {"brown", "default", 0}, /* c_att_value */
  {"lightgray", "default", 0}, /* c_chardata */
  {"magenta", "default", 0}, /* c_comment */
  {"blue", "default", 0}, /* c_pi */
  {"cyan", "default", 0}, /* c_decl */
  {"cyan", "default", 0}, /* c_cdata */
  {"black", "white", 0}, /* c_status */
  {"lightgray", "default", 0}, /* c_indent */
  {"lightgray", "default", 0}, /* c_margin */
  {"lightgray", "default", 0}, /* c_other */
},
{
  {"lightgray", "default", 0}, /* c_default */
  {"blue", "default", 0}, /* c_tag */
  {"lightgray", "default", SLTT_ULINE_MASK}, /* c_att_name */
  {"white", "default", 0}, /* c_att_value */
  {"lightgray", "default", 0}, /* c_chardata */
  {"green", "default", 0}, /* c_comment */
  {"blue", "default", 0}, /* c_pi */
  {"magenta", "default", 0}, /* c_decl */
  {"cyan", "default", 0}, /* c_cdata */
  {"black", "white", 0}, /* c_status */
  {"lightgray", "default", 0}, /* c_indent */
  {"lightgray", "default", 0}, /* c_margin */
  {"lightgray", "default", 0}, /* c_other */
}
};

const char_t *ui_help[] = {
  "                   SUMMARY OF XML-LESS COMMANDS",
  "",
  " H C-h                  Display this help.",
  " q                      Exit.",
  "-----------------------------------------------------------------",
  "",
  "                          MOVING",
  "",
  " j C-n DOWN      Forward one element.",
  " k C-p UP        Backward one element.",
  " l RIGHT         Pan right.",
  " h LEFT          Pan left.",
  " SPACE           Forward many elements.",
  "",
  "                          RENDERING",
  "",
  " TAB             Cycle indenting pivot.",
  " a               Toggle attributes.",
  " w               Toggle wordwrap.",
  " c               Cycle color scheme.",
};
size_t ui_help_count = sizeof(ui_help)/sizeof(const char_t *);

static volatile bool_t sigwinch;
static void sigwinch_handler(int sig) {
  SLsig_block_signals();

  SLtt_get_screen_size();
  SLsmg_reinit_smg();

  SLKeyBoard_Quit = 1;

  SLsig_unblock_signals();
  SLsignal(SIGWINCH, sigwinch_handler);
}

bool_t setup_colours_display(display_t *disp) {
  colour_t c;
  int i, s;
  if( disp ) {
    s = disp->colour_scheme;
    s = (s >= DISPLAY_COLOUR_SCHEMES) ? s - DISPLAY_COLOUR_SCHEMES : s;
    for(c = c_default; c < c_last; c++) {
      SLtt_set_color(c, NULL, ui_colour[s][c].fg, ui_colour[s][c].bg);
      SLtt_add_color_attribute(c, ui_colour[s][c].att);
    }
    for(i = 0; i < (1<<DISPLAY_AUTO_BITS); i++) {
      SLtt_set_color(c_last + i, NULL, auto_colour[s][i].fg, 
		     auto_colour[s][i].bg);
      SLtt_add_color_attribute(c_last + i, auto_colour[s][i].att);
    }
    return TRUE;
  }
  return FALSE;
}

bool_t next_colour_scheme_display(display_t *disp) {
  if( disp ) {
    /* cycle through 2 * DISPLAY_COLOUR_SCHEMES
       the first half have uniform tag colours, the second
       half have random tag colours */
    disp->colour_scheme = 
      (disp->colour_scheme + 1) % (2 * DISPLAY_COLOUR_SCHEMES);
    return setup_colours_display(disp);
  }
  return FALSE;
}

bool_t set_display(display_t *disp, long mask) {
  if( disp ) {
    disp->flags |= mask;
    return TRUE;
  }
  return FALSE;
}

bool_t unset_display(display_t *disp, long mask) {
  if( disp ) {
    disp->flags &= ~mask;
    return TRUE;
  }
  return FALSE;
}

bool_t toggle_display(display_t *disp, long mask) {
  if( disp ) {
    disp->flags ^= mask;
    return TRUE;
  }
  return FALSE;
}

bool_t pivot_display(display_t *disp, int delta) {
  if( disp ) {
    disp->pivot += delta;
    disp->pivot = (disp->pivot > disp->max_visible_depth) ? 0 : disp->pivot;
    disp->pivot = (disp->pivot < 0) ? disp->max_visible_depth : disp->pivot;
  } 
  return FALSE;
}

bool_t shift_display(display_t *disp, int shift) {
  if( disp ) {
    disp->first_col += shift;
    disp->first_col = (disp->first_col < 0) ? 0 : disp->first_col;
  } 
  return FALSE;
}


bool_t open_display(display_t *disp) {
  if( disp ) {
    SLsig_block_signals();
    disp->num_rows = SLtt_Screen_Rows - 1;
    disp->num_cols = SLtt_Screen_Cols;
    return TRUE;
  }
  return FALSE;
}

bool_t close_display(display_t *disp) {
  if( disp ) {

    SLsmg_refresh();
    SLsig_unblock_signals();

    return TRUE;
  }
  return FALSE;
}

bool_t prompt_display(display_t *disp) {
  if( disp ) {
    /* num_rows is initialized as SLtt_Screen_Rows - 1 */
    SLsmg_gotorc(disp->num_rows, 0);
    SLsmg_set_color(0);
    SLsmg_write_char(':');
    SLsmg_erase_eol();
    return TRUE;
  }
  return FALSE;
}


lessui_command_t getcommand_display(display_t *disp) {
  int c, c1, c2;
  /* global var set this to 1 if you want to interrupt getkey */
  SLKeyBoard_Quit = 0;

  while( !SLang_input_pending(2) ) {
    process_pending_signal();
    if( checkflag(cmd,CMD_QUIT) ) {
      return quit;
    } else if( true_and_clearflag((flag_t *)&cmd,CMD_ALRM) ) {
      return refresh;
    } else if( true_and_clearflag((flag_t *)&cmd,CMD_CHLD) ) {
      return refresh;
    }
  }
  /* this is pretty ugly :( */
  c = SLang_getkey();
  if( c == '' ) {
    c1 = SLang_getkey();
    switch(c1) {
    case '[': /* shift-TAB = "ESC[Z" */
      c2 = SLang_getkey();
      if( c2 == 'Z' ) {
	c = c2;
	break;
      }
      SLang_ungetkey(c2);
      /* fall through */
    default:
      /* assume it's a cursor key */
      SLang_ungetkey(c1);
      SLang_ungetkey(c);
      c = SLkp_getkey();
    }
  }
  switch(c) {
  case 'q':
    return quit;
  case 'H':
    return help;
  case SL_KEY_HOME:
    return home;
  case SL_KEY_END:
    return end;
  case SL_KEY_BACKSPACE:
  case SL_KEY_PPAGE:
    return pgup;
  case ' ':
  case '\r':
  case SL_KEY_ENTER:
  case SL_KEY_NPAGE:
    return pgdown;
  case 'a':
    return attributes;
  case 'c':
    return colours;
  case 'w':
    return wordwrap;
  case 'k':
  case 0x10:
  case SL_KEY_UP:
    return backward;
  case 'j':
  case 0x0e:
  case SL_KEY_DOWN:
    return forward;
  case '\t':
    return indent;
  case 'Z':
    return backindent;
  case 'h':
  case SL_KEY_LEFT:
    return left;
  case 'l':
  case SL_KEY_RIGHT:
    return right;
  case 'N':
    return next_sibling;
  case 'r':
    return refresh;
  default:
    return nothing;
  }
}

void start_tag_display(fbparserinfo_t *pinfo, const char_t *name, const char_t **att, void *user) {
  renderer_t *r = (renderer_t *)user;
  if( pinfo && r ) { 
    if( !r->pivot || (pinfo->depth < r->pivot) ) {
      set_offsets_renderer(r, pinfo->offset, pinfo->nodecount);
      set_indent_renderer(r, pinfo->depth);
      start_line_renderer(r);
    }
    set_colour_renderer(r, c_tag);
    puts_renderer(r, "<");
    set_autocolour_renderer(r, name);
    puts_renderer(r, name);
    if( checkflag(r->flags, RENDERER_ATTRIBUTES) ) {
      if( reset_attlist(&r->alist) &&
	  append_attlist(&r->alist, att) &&
	  sort_attlist(&r->alist) ) {
	write_attributes_renderer(r, &r->alist);
      }
    }
    set_colour_renderer(r, c_tag);
    puts_renderer(r, ">");
    set_colour_renderer(r, c_default);
  }
}

void end_tag_display(fbparserinfo_t *pinfo, const char_t *name, void *user) {
  renderer_t *r = (renderer_t *)user;
  if( pinfo && r ) { 
    if( !r->pivot || (pinfo->depth < r->pivot) ) {
      set_offsets_renderer(r, pinfo->offset, pinfo->nodecount);
      /* fbp convention depth(end-tag) > depth(start-tag) */
      set_indent_renderer(r, pinfo->depth - 1);
      start_line_renderer(r);
    }
    set_colour_renderer(r, c_tag);
    puts_renderer(r, "</");
    set_autocolour_renderer(r, name);
    puts_renderer(r, name);
    set_colour_renderer(r, c_tag);
    puts_renderer(r, ">");
    set_colour_renderer(r, c_default);
  }
}

void chardata_display(fbparserinfo_t *pinfo, const char_t *buf, size_t buflen, void *user) {
  const char_t *sep = " ";
  renderer_t *r = (renderer_t *)user;
  if( pinfo && r ) { 
    if( pinfo->nodetype > n_space ) {
      set_offsets_renderer(r, pinfo->offset, pinfo->nodecount);
      if( checkflag(r->flags, RENDERER_WORDWRAP) ) {
	set_indent_renderer(r, pinfo->depth);
      } else if( !r->pivot || (pinfo->depth < r->pivot) ) {
	set_indent_renderer(r, pinfo->depth);
	start_line_renderer(r);
      }
      set_colour_renderer(r, c_chardata);
      write_squeeze_renderer(r, buf, buf + buflen);
      if( checkflag(r->flags, RENDERER_WORDWRAP) &&
	  (pinfo->noderep > midfrag) ) {
	puts_renderer(r, sep);
      }
      set_colour_renderer(r, c_default);
    }
  }
}

void dfault_display(fbparserinfo_t *pinfo, const char_t *buf, size_t buflen, void *user) {
  /* this isn't giving good results and has to be redone */
  renderer_t *r = (renderer_t *)user;
  if( pinfo && r ) {
    if( pinfo->nodetype > n_space ) {
      set_offsets_renderer(r, pinfo->offset, pinfo->nodecount);
      set_indent_renderer(r, pinfo->depth);
      start_line_renderer(r);
      set_colour_renderer(r, c_other);
      write_renderer(r, buf, buf + buflen);
      set_colour_renderer(r, c_other);
    }
  }
}

void comment_display(fbparserinfo_t *pinfo, const char_t *data, void *user) {
  renderer_t *r = (renderer_t *)user;
  const char_t *p, *q;
  if( pinfo && r ) { 
    set_offsets_renderer(r, pinfo->offset, pinfo->nodecount);
    set_indent_renderer(r, pinfo->depth);
    start_line_renderer(r);
    set_colour_renderer(r, c_comment);
    puts_renderer(r, "<!-- ");
    p = data;
    while( p && *p ) {
      q = skip_unescaped_delimiters(p, NULL, "\r\n", '\0');
      write_renderer(r, p, q);
      while( (*q == '\r') || (*q == '\n') ) { q++; }
      p = q;
      if( *q ) {
	start_line_renderer(r);
      }
    }
    puts_renderer(r, " -->");
    set_colour_renderer(r, c_default);
  }
}

void pidata_display(fbparserinfo_t *pinfo, const char_t *target, const char_t *data, void *user) {
  renderer_t *r = (renderer_t *)user;
  if( pinfo && r ) { 
    set_offsets_renderer(r, pinfo->offset, pinfo->nodecount);
    set_indent_renderer(r, pinfo->depth);
    start_line_renderer(r);
    puts_renderer(r, "<?");
    puts_renderer(r, target);
    if( checkflag(r->flags, RENDERER_ATTRIBUTES) ) {
      puts_renderer(r, " ");
      puts_renderer(r, data);
    }
    puts_renderer(r, "?>");
  }
}

void start_cdata_display(fbparserinfo_t *pinfo, void *user) {
  renderer_t *r = (renderer_t *)user;
  if( pinfo && r ) { 
    /* set_offsets_renderer(r, pinfo->offset, pinfo->nodecount); */
    /* set_indent_renderer(r, pinfo->depth); */
    /* start_line_renderer(r); */
    set_colour_renderer(r, c_cdata);
    puts_renderer(r, "<![CDATA[");
  }
}

void end_cdata_display(fbparserinfo_t *pinfo, void *user) {
  renderer_t *r = (renderer_t *)user;
  if( pinfo && r ) { 
    /* set_offsets_renderer(r, pinfo->offset, pinfo->nodecount); */
    /* set_indent_renderer(r, pinfo->depth); */
    /* start_line_renderer(r); */
    set_colour_renderer(r, c_cdata);
    puts_renderer(r, "]]>");
    set_colour_renderer(r, c_default);
  }
}

void start_doctypedecl_display(fbparserinfo_t *pinfo, const char_t *name, const char_t *sysid, const char_t *pubid, bool_t intsub, void *user) {
  renderer_t *r = (renderer_t *)user;
  if( pinfo && r ) { 
    if( !r->pivot || (pinfo->depth < r->pivot) ) {
      set_offsets_renderer(r, pinfo->offset, pinfo->nodecount);
      set_indent_renderer(r, pinfo->depth);
      start_line_renderer(r);
    }
    set_colour_renderer(r, c_decl);
    puts_renderer(r, "<!DOCTYPE");
    if( name ) {
      puts_renderer(r, " ");
      puts_renderer(r, name);
    }
    if( checkflag(r->flags, RENDERER_ATTRIBUTES) ) {
      set_colour_renderer(r, c_att_value);
      if( sysid ) {
	puts_renderer(r, " ");
	puts_renderer(r, sysid);
      }
      if( pubid ) {
	puts_renderer(r, " ");
	puts_renderer(r, pubid);
      }
    }
    set_colour_renderer(r, c_decl);
    puts_renderer(r, " [");
    set_colour_renderer(r, c_default);
  }
}

void end_doctypedecl_display(fbparserinfo_t *pinfo, void *user) {
  renderer_t *r = (renderer_t *)user;
  if( pinfo && r ) { 
    if( !r->pivot || (pinfo->depth + 1 < r->pivot) ) {
      set_offsets_renderer(r, pinfo->offset, pinfo->nodecount);
      set_indent_renderer(r, pinfo->depth + 1);
      start_line_renderer(r);
    }
    set_colour_renderer(r, c_decl);
    puts_renderer(r, "]>");
    set_colour_renderer(r, c_default);
  }
}

void entitydecl_display(fbparserinfo_t *pinfo, const char_t *name, bool_t isparam, const char_t *value, int len, const char_t *base, const char_t *sysid, const char_t *pubid, const char_t *notation, void *user) {
  renderer_t *r = (renderer_t *)user;
  if( pinfo && r ) { 
    if( !r->pivot || (pinfo->depth + 1 < r->pivot) ) {
      set_offsets_renderer(r, pinfo->offset, pinfo->nodecount);
      set_indent_renderer(r, pinfo->depth + 1);
      start_line_renderer(r);
    }
    set_colour_renderer(r, c_decl);
    puts_renderer(r, "<!ENTITY");
    puts_renderer(r, " ");
    puts_renderer(r, name);
    if( checkflag(r->flags, RENDERER_ATTRIBUTES) ) {
      set_colour_renderer(r, c_att_value);
      if( value ) {
      	puts_renderer(r, " \"");
      	write_renderer(r, value, value + len);
      	puts_renderer(r, "\"");
      }
      if( base ) {
      	puts_renderer(r, " ");
      	puts_renderer(r, base);
      }
      if( sysid ) {
      	puts_renderer(r, " ");
      	puts_renderer(r, sysid);
      }
      if( pubid ) {
      	puts_renderer(r, " ");
      	puts_renderer(r, pubid);
      }
      if( notation ) {
      	puts_renderer(r, " ");
      	puts_renderer(r, notation);
      }
    }
    set_colour_renderer(r, c_decl);
    puts_renderer(r, ">");
    set_colour_renderer(r, c_default);
  }
}

bool_t create_display(display_t *disp) {
  if( disp ) {
    memset(disp, 0, sizeof(display_t));
    SLang_init_tty(-1,0,0);
    SLtt_get_terminfo();
    if( -1 == SLsmg_init_smg() ) {
      errormsg(E_FATAL, "unable to initialize terminal.\n");
    }
    SLkp_init();
    SLsignal(SIGWINCH, sigwinch_handler);
    setup_colours_display(disp);
    return TRUE;
  }
  return FALSE;
}

bool_t free_display(display_t *disp) {
  if( disp ) {
    SLsignal(SIGWINCH, NULL);

    SLsmg_reset_smg();
    SLang_reset_tty();

    return TRUE;
  }
  return FALSE;
}

bool_t fill_callbacks_display(display_t *disp, renderer_t *r, fbcallback_t *callbacks) {
  if( callbacks ) {

    memset(callbacks, 0, sizeof(fbcallback_t));

    callbacks->start_tag = start_tag_display;
    callbacks->end_tag = end_tag_display;
    callbacks->chardata = chardata_display;
    callbacks->pidata = pidata_display;
    callbacks->start_cdata = start_cdata_display;
    callbacks->end_cdata = end_cdata_display;
    callbacks->comment = comment_display;
    callbacks->start_doctypedecl = start_doctypedecl_display;
    callbacks->end_doctypedecl = end_doctypedecl_display;
    callbacks->entitydecl = entitydecl_display;
    callbacks->dfault = dfault_display;
    callbacks->user = (void *)r;
    return TRUE;
  }
  return FALSE;
}

bool_t setup_renderer_display(display_t *disp, renderer_t *r, off_t filesize) {
  double n;
  if( disp && r  ) {
    r->pivot = disp->pivot;
    if( checkflag(disp->flags, DISPLAY_WRAP) ) {
      setflag(&r->flags, RENDERER_WORDWRAP);
      setflag(&r->flags, RENDERER_LINEWRAP);
    }
    if( checkflag(disp->flags, DISPLAY_ATTRIBUTES) ) {
      setflag(&r->flags, RENDERER_ATTRIBUTES);
    }

    setflag(&r->flags, RENDERER_SHOW_NODECOUNTS);
    
    /* this margin size is correct for OFFSETS, but too big for NODECOUNTS */
    n = (filesize > 0) ? ceil(log(1.0*filesize)/log(10.0)) : 0.0;
    set_margin_renderer(r, 1 + (int)n);

/*     if( checkflag(disp->flags, DISPLAY_NODECOUNTS) ) { */
/*       setflag(&r->flags, RENDERER_SHOW_NODECOUNTS); */
/*       set_margin_renderer(r, 8); */
/*     } else if( checkflag(disp->flags, DISPLAY_OFFSETS) ) { */
/*       setflag(&r->flags, RENDERER_SHOW_OFFSETS); */
/*       set_margin_renderer(r, 8); */
/*     } */

    r->last_col = disp->first_col;
    SLsmg_set_screen_start(NULL, &r->last_col);
    r->last_col = disp->first_col + disp->num_cols + 5;

    return TRUE;
  }
  return FALSE;
}

bool_t redraw_cursor_display(display_t *disp, cursor_t *cursor, fbparser_t *fbp) {
  position_t pos;
  fbcallback_t callbacks;
  renderer_t r;
  if( disp && cursor && fbp ) {

    if( open_display(disp) ) {
      if( create_renderer(&r, disp) &&
	  setup_renderer_display(disp, &r, fbp->reader.size) ) {

	fill_callbacks_display(disp, &r, &callbacks);

	if( parse_first_fileblockparser(fbp, cursor, &callbacks, &pos) ) {

	  /* this still needs logic to skip over nodes which don't get displayed. */
	  while( !checkflag(r.flags,RENDERER_DONE) &&
		 parse_next_fileblockparser(fbp, &pos) );
	  /* 	debug("REDRAW done\n"); */
	}

	disp->max_visible_depth = fbp->info.maxdepth;

	free_renderer(&r);
      }

      prompt_display(disp);

      close_display(disp);
    }

    if( pos.status == ps_error ) {
      errormsg_display(disp, E_WARNING, "parsing error at file offset %x: %s",
		       pos.offset, error_message_parser(&fbp->parser));
      return FALSE;
    }

    return TRUE;
  }
  return FALSE;
}

bool_t redraw_locator_display(display_t *disp, const cursor_t *cursor) {
  size_t n, i;
  if( disp && cursor ) {
    /* num_rows is initialized as SLtt_Screen_Rows - 1 */
    SLsmg_gotorc(disp->num_rows, 0);
    SLsmg_set_color(c_status);

    SLsmg_write_nchars("[", 1);
    n = get_length_cursor(cursor);
    for(i = 0; i < n; i++ ) {
      SLsmg_printf("%ld/%d/%d%s", 
		   get_depth_offset_cursor(cursor, i),
		   get_depth_ord_cursor(cursor, i),
		   get_depth_nord_cursor(cursor, i),
		   ((i+1) < n) ? " " : "");
    }
    SLsmg_write_nchars("]", 1);

    SLsmg_set_color(0);
    SLsmg_erase_eol();
    SLsmg_refresh();
    return TRUE;
  }
  return FALSE;
}

/* bool_t redraw_locator_display(display_t *disp, const char_t *rep) { */
/*   if( disp && rep ) { */
/*     /\* num_rows is initialized as SLtt_Screen_Rows - 1 *\/ */
/*     SLsmg_gotorc(disp->num_rows, 0); */
/*     SLsmg_set_color(c_status); */
/*     SLsmg_write_nchars((char *)rep, strlen(rep)); */
/*     SLsmg_set_color(0); */
/*     SLsmg_erase_eol(); */
/*     SLsmg_refresh(); */
/*     return TRUE; */
/*   } */
/*   return FALSE; */
/* } */

void errormsg_display(display_t *disp, int etype, const char *fmt, ...) {
  va_list vap;

  if( disp ) {
    SLsmg_gotorc(disp->num_rows, 0);
    SLsmg_set_color(c_status);
    
#if HAVE_VPRINTF
    va_start(vap, fmt);
    SLsmg_vprintf((char*)fmt, vap);
    va_end(vap);
#else
    SLsmg_printf(stderr, "%s", fmt);
#endif

    SLsmg_set_color(0);
    SLsmg_erase_eol();
    SLsmg_refresh();
  }
}

bool_t do_help_display(display_t *disp) {
  size_t i;
  renderer_t r;
  if( disp ) {
    disp->first_col = 0;
    if( open_display(disp) ) {
      if( create_renderer(&r, disp) &&
	  setup_renderer_display(disp, &r, 0) ) {

	set_indent_renderer(&r, 0);

	for(i = 0; i < ui_help_count; i++) {
	  start_line_renderer(&r);
	  puts_renderer(&r, ui_help[i]);
	}
	erase_eos_renderer(&r);
	prompt_display(disp);

	free_renderer(&r);
      }
      close_display(disp);
    }

    while(quit != getcommand_display(disp));

    return TRUE;
  }
  return FALSE;
}
