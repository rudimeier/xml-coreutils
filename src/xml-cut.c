/* 
 * Copyright (C) 2009 Laird Breyer
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
#include "parser.h"
#include "io.h"
#include "myerror.h"
#include "mem.h"
#include "wrap.h"
#include "stdout.h"
#include "stdparse.h"
#include "stdprint.h"
#include "entities.h"
#include "tempcollect.h"
#include "interval.h"
#include "mysignal.h"

#include <string.h>
#include <getopt.h>

/* for option processing */
extern char *optarg;
extern int optind, opterr, optopt;

extern char *progname;
extern char *inputfile;
extern long inputline;

extern volatile flag_t cmd;

#include <stdio.h>
#include <ctype.h>

typedef struct {
  stdparserinfo_t std; /* must be first so we can cast correctly */
  flag_t flags;
  intervalmgr_t im;
  tempcollect_t stringval;
} parserinfo_cut_t;

#define CUT_VERSION    0x01
#define CUT_HELP       0x02
#define CUT_USAGE \
"Usage: xml-cut OPTION... [[FILE] [:XPATH]...]\n" \
"Print selected parts of nodes from FILE, or standard input.\n" \
"\n" \
"      --help     display this help and exit\n" \
"      --version  display version information and exit\n"

#define CUT_FLAG_CHAR  0x01
#define CUT_FLAG_FIELD 0x02
#define CUT_FLAG_TAG   0x04

bool_t numeric(char *s) {
  /* returns true if s is "" */
  while(s && *s) {
    if( !isdigit(*s) ) {
      return FALSE;
    }
    s++;
  }
  return TRUE;
}

void read_interval_spec(char *spec, intervalmgr_t *im) {
  long a, b;
  bool_t badrange = FALSE;
  char *p, *q;

  p = strtok(spec, ",");
  while( p && !badrange ) {

    q = strchr(p, '-');
    
    if( !q ) {
      a = b = numeric(p) ? atoi(p) : ((badrange = TRUE), 0);
    } else {
      *q++ = '\0';
      a = numeric(p) ? atoi(p) : ((badrange = TRUE), 0);
      b = numeric(q) ? atoi(q) : ((badrange = TRUE), 0);
      if( a == 0 ) { a = 1; }
      if( b == 0 ) { b = 65535; }
    }

    if( badrange ) {
      errormsg(E_FATAL, "bad range. Try 1, 1-10, -10, 1-, etc.\n");
      exit(EXIT_FAILURE);
    } else {
      push_intervalmgr(im, a, b);
    }

    p = strtok(NULL, ",");
  }

}


void set_option_cut(int op, char *optarg, parserinfo_cut_t *pinfo) {
  switch(op) {
  case CUT_VERSION:
    puts("xml-cut" COPYBLURB);
    exit(EXIT_SUCCESS);
    break;
  case CUT_HELP:
    puts(CUT_USAGE);
    exit(EXIT_SUCCESS);
    break;
  case 'c':
    setflag(&pinfo->flags,CUT_FLAG_CHAR);
    read_interval_spec(optarg, &pinfo->im);
    break;
  case 'f':
    setflag(&pinfo->flags,CUT_FLAG_FIELD);
    read_interval_spec(optarg, &pinfo->im);
    break;
  case 't':
    setflag(&pinfo->flags,CUT_FLAG_TAG);
    read_interval_spec(optarg, &pinfo->im);
    break;
  }
}

typedef struct {
  int fno;
  bool_t field_lock;
  char_t delim;
  intervalmgr_t *im;
} fields_fun_t;


/* print all spaces, and only selected field numbers, on each line in turn */
bool_t fields_fun(void *user, byte_t *buf, size_t buflen) {
  const char_t *p, *q, *r;
  fields_fun_t *fft = (fields_fun_t *)user;

  if( fft ) {
    p = (char_t *)buf;
    q = (char_t *)(buf + buflen);
      while( p < q ) {
	if( xml_whitespace(*p) ) {
	  fft->field_lock = FALSE;
	  /* all whitespace is printed */
	  r = skip_xml_whitespace(p, q);
	  write_stdout((byte_t *)p, (r - p));
	  if( memchr(p, '\n', r - p) ) {
	    fft->fno = 0; /* reset field number if newline */
	  }
	  p = r;
	}
	if( p < q ) {
	  if( !fft->field_lock ) {
	    fft->fno++;
	    fft->field_lock = TRUE;
	  }
	  /* find end of token */
	  r = find_xml_whitespace(p, q);
	  if( memberof_intervalmgr(fft->im, fft->fno) ) {
	    write_stdout((byte_t *)p, (r - p));
	  }
	  p = r;
	}
      }
    return TRUE;
  }
  return FALSE;
}

bool_t print_fields_stdout(parserinfo_cut_t *pinfo) {
  fields_fun_t ff;
  tempcollect_adapter_t ad;
  ff.fno = 0;
  ff.field_lock = FALSE;
  ff.delim = ' ';
  ff.im = &pinfo->im;
  ad.fun = fields_fun;
  ad.user = &ff;

  write_adapter_tempcollect(&pinfo->stringval, &ad);
  return TRUE;
}

/* THIS IS AN ALTERNATIVE FIELDS_FUN WHICH PRINTS DELIM BETWEEN FIELDS */

/* typedef struct { */
/*   int fno; */
/*   bool_t has_output; */
/*   char_t delim; */
/*   intervalmgr_t *im; */
/* } fields_fun_t; */

/* bool_t fields_fun(void *user, byte_t *buf, size_t buflen) { */
/*   const char_t *p, *q, *r; */
/*   bool_t show, at_end; */
/*   fields_fun_t *fft = (fields_fun_t *)user; */

/*   if( fft ) { */
/*     p = (char_t *)buf; */
/*     q = (char_t *)(buf + buflen); */
/*     while( p < q ) { */
/*       /\* all whitespace is printed *\/ */
/*       r = skip_xml_whitespace(p, q); */
/*       write_stdout((byte_t *)p, (r - p)); */
/*       p = r; */
      
/*       /\* find end of token *\/ */
/*       r = find_xml_whitespace(p, q); */
/*       show = memberof_intervalmgr(fft->im, fft->fno); */
/*       at_end = (r >= q); */

/*       if( show ) { */
/* 	write_stdout((byte_t *)p, (r - p)); */
/* 	fft->has_output = TRUE; */
/* 	if( !at_end ) { */
/* 	  putc_stdout(fft->delim); */
/* 	} */
/*       }  */

/*       p = (at_end) ? r : (r + 1); */
/*       fft->fno += (at_end) ? 0 : 1; */

/*     } */
/*     return TRUE; */
/*   } */
/*   return FALSE; */
/* } */

/* bool_t print_fields_stdout(parserinfo_cut_t *pinfo) { */
/*   fields_fun_t ff; */
/*   tempcollect_adapter_t ad; */
/*   ff.fno = 1; */
/*   ff.has_output = FALSE; */
/*   ff.delim = ' '; */
/*   ff.im = &pinfo->im; */
/*   ad.fun = fields_fun; */
/*   ad.user = &ff; */

/*   write_adapter_tempcollect(&pinfo->stringval, &ad); */
/*   return ff.has_output; */
/* } */


typedef struct {
  int cno;
  bool_t has_output;
  intervalmgr_t *im;
} chars_fun_t;

bool_t chars_fun(void *user, byte_t *buf, size_t buflen) {
  chars_fun_t *cft = (chars_fun_t *)user;
  if( cft ) {
    while( buflen-- > 0 ) {
      if( *buf == '\n' ) {
      	putc_stdout(*buf);
      	cft->has_output = TRUE;
      	cft->cno = 1;
      } else if( memberof_intervalmgr(cft->im, cft->cno++) ) {
      	putc_stdout(*buf);
      	cft->has_output = TRUE;
      }
      buf++;
    }
    return TRUE;
  }
  return FALSE;
}

/* print only selected chars on each line of pinfo->im */
bool_t print_chars_stdout(parserinfo_cut_t *pinfo) {
  chars_fun_t cf;
  tempcollect_adapter_t ad;
  cf.cno = 1;
  cf.has_output = FALSE;
  cf.im = &pinfo->im;
  ad.fun = chars_fun;
  ad.user = &cf;

  write_adapter_tempcollect(&pinfo->stringval, &ad);
  return cf.has_output;
}


result_t flush_stringval_stdout(parserinfo_cut_t *pinfo) {
  if( pinfo ) {
    if( checkflag(pinfo->flags,CUT_FLAG_FIELD) ) {

      if( !is_empty_tempcollect(&pinfo->stringval) ) {

	if( !print_fields_stdout(pinfo) ) {
	  /* write_stdout_tempcollect(&pinfo->stringval); /\* safety *\/ */
	}
	reset_tempcollect(&pinfo->stringval);
      }

    } else if( checkflag(pinfo->flags,CUT_FLAG_CHAR) ) {

      if( !is_empty_tempcollect(&pinfo->stringval) ) {
	if( !print_chars_stdout(pinfo) ) {
	  /* write_stdout_tempcollect(&pinfo->stringval); */
	}
	reset_tempcollect(&pinfo->stringval);
      }

    }
    return TRUE;
  }
  return FALSE;
}

result_t start_tag(void *user, const char_t *name, const char_t **att) {
  parserinfo_cut_t *pinfo = (parserinfo_cut_t *)user;
  if( pinfo ) { 

    if( checkflag(pinfo->flags,CUT_FLAG_TAG) ) {
      if( memberof_intervalmgr(&pinfo->im, pinfo->std.depth) ) {
	write_start_tag_stdout(name, att, FALSE);
      }
    } else {
      flush_stringval_stdout(pinfo);
      write_start_tag_stdout(name, att, FALSE);
    }

  }
  return PARSER_OK;
}

result_t end_tag(void *user, const char_t *name) {
  parserinfo_cut_t *pinfo = (parserinfo_cut_t *)user;
  if( pinfo ) { 

    if( checkflag(pinfo->flags,CUT_FLAG_TAG) ) {
      if( memberof_intervalmgr(&pinfo->im, pinfo->std.depth) ) {
	write_end_tag_stdout(name);
      }
    } else {
      flush_stringval_stdout(pinfo);
      write_end_tag_stdout(name);
    }      

  }
  return PARSER_OK;
}

result_t chardata(void *user, const char_t *buf, size_t buflen) {
  parserinfo_cut_t *pinfo = (parserinfo_cut_t *)user;
  if( pinfo ) { 

  /* stringval is needed to save the string value of the node,
     as we cannot decide if we want to print it until we've
     seen it in its entirety */

    if( checkflag(pinfo->flags,CUT_FLAG_TAG) ) {
      if( memberof_intervalmgr(&pinfo->im, pinfo->std.depth + 1) ) {
	write_coded_entities_stdout(buf, buflen);
      }
    } else { 
      write_coded_entities_tempcollect(&pinfo->stringval, buf, buflen);
    }

  }
  return PARSER_OK;
}

result_t dfault(void *user, const char_t *data, size_t buflen) {
  parserinfo_cut_t *pinfo = (parserinfo_cut_t *)user;
  if( pinfo ) { 
    write_stdout((byte_t *)data, buflen);
  }
  return PARSER_OK;
}

bool_t create_parserinfo_cut(parserinfo_cut_t *pinfo) {
  bool_t ok = TRUE;
  if( pinfo ) {

    memset(pinfo, 0, sizeof(parserinfo_cut_t));
    ok &= create_stdparserinfo(&pinfo->std);

    pinfo->std.setup.flags = STDPARSE_EQ1FILE;
    pinfo->std.setup.cb.start_tag = start_tag;
    pinfo->std.setup.cb.end_tag = end_tag;
    pinfo->std.setup.cb.chardata = chardata;
    pinfo->std.setup.cb.dfault = dfault;
    pinfo->flags = 0;

    ok &= create_intervalmgr(&pinfo->im);
    ok &= create_tempcollect(&pinfo->stringval, "stringval", 
			     MINVARSIZE, MAXVARSIZE);
    return ok;
  }
  return FALSE;
} 

bool_t free_parserinfo_cut(parserinfo_cut_t *pinfo) {
  free_stdparserinfo(&pinfo->std);
  free_intervalmgr(&pinfo->im);
  free_tempcollect(&pinfo->stringval);
  return TRUE;
}

void sanity_check(parserinfo_cut_t *pinfo) {
  int f;
  
  f = (checkflag(pinfo->flags,CUT_FLAG_CHAR) > 0) + 
    (checkflag(pinfo->flags,CUT_FLAG_FIELD) > 0) + 
    (checkflag(pinfo->flags,CUT_FLAG_TAG) > 0);

  if( f != 1 ) {
    errormsg(E_FATAL, "exactly one of -c, -f, or -t options must be given.\n");
  }

}

void output_wrapper_start(parserinfo_cut_t *pinfo) {
  open_stdout();
  if( pinfo ) {
    puts_stdout(get_headwrap());
    puts_stdout(get_open_root());
    putc_stdout('\n');
  }
}

void output_wrapper_end(parserinfo_cut_t *pinfo) {
  if( pinfo ) {
    putc_stdout('\n');
    puts_stdout(get_close_root());
    puts_stdout(get_footwrap());
  }
  close_stdout();
}

int main(int argc, char **argv) {
  signed char op;
  parserinfo_cut_t pinfo;

  struct option longopts[] = {
    { "version", 0, NULL, CUT_VERSION },
    { "help", 0, NULL, CUT_HELP },
    { 0 }
  };

  progname = "xml-cut";
  inputfile = "";
  inputline = 0;

  if( create_parserinfo_cut(&pinfo) ) {

    while( (op = getopt_long(argc, argv, "c:f:t:",
			     longopts, NULL)) > -1 ) {
      set_option_cut(op, optarg, &pinfo);
    }
    sanity_check(&pinfo);

    init_signal_handling(SIGNALS_DEFAULT);
    init_file_handling();

    output_wrapper_start(&pinfo);

    stdparse(MAXFILES, argv + optind, (stdparserinfo_t *)&pinfo);

    output_wrapper_end(&pinfo);

    exit_file_handling();
    exit_signal_handling();

    free_parserinfo_cut(&pinfo);
  }
  return EXIT_SUCCESS;
}
