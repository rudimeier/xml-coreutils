/* 
 * Copyright (C) 2010 Laird Breyer
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

#include "htfilter.h"
#include "stdout.h"
#include "myerror.h"

#include <string.h>

extern char *progname;
extern char *inputfile;

/* see HTML4 spec ("Index of Elements").
 * These tags are treated specially if the HTML flag is set.
 */

/* these tags must be closed immediately (ie we must emit <tag/>) */
static const char_t *html_empty_tag[] = {
  "area",  "base",  "basefont",  "br",  "col",  "frame",  "hr",
  "img",  "input",  "isindex",  "link",  "meta",  "param",
  NULL
};

/* these tags imply we're in the <head> content */ 
static const char_t *html_head_tag[] = {
  "title",  "isindex",  "meta",  "link",  "base",  "script",  "style",
  NULL
};

/* these tags close a previously open lazy tag automatically */
static const char_t *html_selfish_tag[] = {
  "body",  "colgroup",  "dd",  "dt",  "head",  "html",  "li",
  "option",  "p",  "tbody",  "td",  "tfoot",  "th",  "thead",
  "tr",
  NULL
};

/* these tags stay open until a selfish tag is encountered */
static const char_t *html_lazy_tag[] = {
  "colgroup",  "dd",  "dt",  "li",
  "option",  "p",  "tbody",  "td",  "tfoot",  "th",  "thead",
  "tr",
  NULL
};

bool_t reset_htfilter(htfilter_t *f);

bool_t is_special_tag(const char_t *name, const char_t *list[]) {
  const char_t **p;
  if( name ) {
    for(p = list; *p; p++) {
      if( strcasecmp(name, *p) == 0 ) { return TRUE; }
    }
  }
  return FALSE;
}

/* DUMB FILTER CALLBACKS 
 * The dumb filter simply echoes its input (but will cause an error
 * if the input isn't well formed). It should be used if the input
 * file isn't html.
 */

result_t dumb_dfault(void *user, const char_t *data, size_t buflen) {
  write_stdout((byte_t *)data, buflen);
  return PARSER_OK;
}

/* HTML FILTER CALLBACKS 
 * This filter contains heuristics that are needed when the input
 * is an HTML file that was converted into XML by filter_buffer(). 
 *
 * Although filter_buffer() produces (besides bugs) well formed XML,
 * the result is too far from what an HTML document should look like
 * to be usable. The main reason is that in HTML, tags (such as <p>)
 * are often not closed, and filter_buffer() will close them only at
 * the end of the document [this is correct behaviour for XML type files].
 * This causes all sorts of problems for XML processors (eg libxml has
 * maximal depth limits, document sections no longer have a consitent depth etc.)
 *
 * This filter recognizes that certain tags are empty and must be closed
 * immediately, and also adds the html/head/body wrapper tags if they are 
 * missing. This last part is necessary to support empty tags properly 
 * [because if they are missing, then an empty tag at depth 1 will close 
 * the document immediately].
 *
 * This filter does not attempt to produce "real" HTML or XHTML, but
 * similarities with the HTML5 draft are purely coincidental :)
*/

result_t html_start_tag(void *user, const char_t *name, const char_t **att) {
  filter_data_t *filter = (filter_data_t *)user;
  const char_t *tag;
  if( filter ) {
    filter->depth++;
    if( filter->depth == 1 ) {
      if( strcasecmp(name, "html") != 0 ) {
    	write_start_tag_stdout("html", NULL, FALSE);
    	push_tag_xpath(&filter->xpath, "html");
	return html_start_tag(user, name, att);
      }
    }
    if( filter->depth == 2 ) {
      if( strcasecmp(name, "head") == 0 ) {
	filter->section = "head";
      } else if( strcasecmp(name, "body") == 0 ) {
	filter->section = "body";
      } else {
    	filter->section = (filter->section == NULL) ? "head" : "body";
    	write_start_tag_stdout(filter->section, NULL, FALSE);
    	push_tag_xpath(&filter->xpath, filter->section);
	return html_start_tag(user, name, att);
      }
    }
    if( filter->depth == 3 ) {
      if( (strcmp(filter->section, "head") == 0) && 
	  !is_special_tag(name, html_head_tag) ) {
	write_end_tag_stdout(filter->section);
	pop_xpath(&filter->xpath);
	filter->depth--;
	filter->section = "body";
    	write_start_tag_stdout(filter->section, NULL, FALSE);
    	push_tag_xpath(&filter->xpath, filter->section);
	return html_start_tag(user, name, att);
      }
    }


    if( filter->depth > 2 ) {
      if( is_special_tag(name, html_empty_tag) ) {
    	write_start_tag_stdout(name, att, TRUE);
	filter->depth--;
	/* printf("SPECIALTAG[%s][%s]\n", name, string_xpath(&filter->xpath)); */
    	return PARSER_OK;
      }
      tag = get_last_xpath(&filter->xpath);
      if( tag && is_special_tag(tag, html_lazy_tag) &&
	  is_special_tag(name, html_selfish_tag) ) {
	write_end_tag_stdout(tag);
	pop_xpath(&filter->xpath);
      }
    }
    push_tag_xpath(&filter->xpath, name);
    /* printf("TAG[%s][%s]\n", name, string_xpath(&filter->xpath)); */
  }
  return PARSER_DEFAULT;
}

result_t html_end_tag(void *user, const char_t *name) {
  const char_t *tag;
  filter_data_t *filter = (filter_data_t *)user;
  if( filter ) {

    /* only filter->xpath is actually authoritative for closing tags */

    tag = get_last_xpath(&filter->xpath);
    if( !tag || (tag && (strcmp(tag, name) != 0)) ) {
      /* printf("IGNORING[%s][%s]\n", name, string_xpath(&filter->xpath)); */
      return PARSER_OK;
    }

    pop_xpath(&filter->xpath);
    filter->depth--;
  }
  /* printf("CLOSETAG[%s][%s]\n", name, string_xpath(&filter->xpath)); */
  return PARSER_DEFAULT;
}

result_t html_dfault(void *user, const char_t *data, size_t buflen) {
  write_stdout((byte_t *)data, buflen);
  return PARSER_OK;
}

/* HTFILTER */

bool_t create_htfilter(htfilter_t *f, flag_t flags) {
  callback_t cb;
  if( f ) {
    memset(f, 0, sizeof(htfilter_t));
    memset(&cb, 0, sizeof(callback_t));
    f->flags = flags;
    f->buflen = 4096;

    create_xpath(&f->filter.xpath);
    if( create_parser(&f->parser, &f->filter) ) {

      if( checkflag(f->flags, HTFILTER_DUMB) ) {
	cb.dfault = dumb_dfault;
      } else if( checkflag(f->flags, HTFILTER_HTML) ) {
	cb.start_tag = html_start_tag;
	cb.end_tag = html_end_tag;
	cb.dfault = html_dfault;
      } else if( checkflag(f->flags, HTFILTER_DEBUG) ) {
	/* nothing */
      }

      setup_parser(&f->parser, &cb);
      return TRUE;
    }
  }
  return FALSE;
}

bool_t free_htfilter(htfilter_t *f) {
  if( f ) {
    flush_htfilter(f);
    free_parser(&f->parser);
    free_xpath(&f->filter.xpath);
    return TRUE;
  }
  return FALSE;
}

bool_t reset_htfilter(htfilter_t *f) {
  if( f ) {
    if( !checkflag(f->flags, HTFILTER_DEBUG) ) {
      f->buf = getbuf_parser(&f->parser, f->buflen);
      f->p = f->buf;
    }
    return TRUE;
  }
  return FALSE;
}

bool_t initialize_htfilter(htfilter_t *f) {
  if( f ) {
    reset_htfilter(f);
    return TRUE;
  }
  return FALSE;
}


bool_t close_htfilter_fun(xpath_t *xp, tagtype_t t, const char_t *begin, const char_t *end, void *user) {
  if( begin && (end > begin) ) {
    puts_stdout("</");
    write_stdout((byte_t *)begin, end - begin);
    puts_stdout(">\n");
    return TRUE;
  }
  return FALSE;
}

void close_htfilter_path(filter_data_t *filter) {
  close_xpath(&filter->xpath, close_htfilter_fun, NULL);
}


bool_t finalize_htfilter(htfilter_t *f) {
  if( f ) {
    flush_htfilter(f);
    /* printf("FINALIZING:[%s]\n", string_xpath(&f->filter.xpath)); */
    close_htfilter_path(&f->filter);
    flush_stdout();
    return TRUE;
  }
  return FALSE;
}

bool_t write_htfilter(htfilter_t *f, const char_t *buf, size_t buflen) {
  int nbytes; 
  if( f && buf && (buflen > 0) ) {
    /* printf("WRITING[%.*s]\n", buflen, buf); */
    if( checkflag(f->flags, HTFILTER_DEBUG) ) {
      return write_stdout((byte_t *)buf, buflen);
    } else if( f->buf ) {
      if( f->p >= (f->buf + f->buflen) ) {
	flush_htfilter(f);
      }
      while( buflen > 0 ) {
	nbytes = MIN(buflen, f->buf + f->buflen - f->p);
	memcpy(f->p, buf, nbytes);

	buf += nbytes;
	buflen -= nbytes;

	f->p += nbytes;
	if( f->p == (f->buf + f->buflen) ) {
	  flush_htfilter(f);
	}
      }
      return TRUE;
    } 
  }
  return FALSE;
}


bool_t flush_htfilter(htfilter_t *f) {
  if( f ) {
    if( !checkflag(f->flags,HTFILTER_DEBUG) && f->buf && (f->p > f->buf) ) {
      if( !do_parser(&f->parser, f->p - f->buf) ) {
	flush_stdout(); /* print everything we have to help debugging */
	errormsg(E_FATAL, 
		 "%s: %s at line %d, column %d, byte %ld\n"
		 "\n"
		 "This is a bug in %s, please keep the input file and notify the author listed in the manpage.\n",
		 
		 inputfile, 
		 error_message_parser(&f->parser),
		 f->parser.cur.lineno, 
		 f->parser.cur.colno,
		 f->parser.cur.byteno,
		 progname);
      }
      return reset_htfilter(f);
    }
    return TRUE;
  }
  return FALSE;
}
