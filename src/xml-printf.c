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
#include "parser.h"
#include "io.h"
#include "mem.h"
#include "myerror.h"
#include "xpath.h"
#include "xmatch.h"
#include "xpredicate.h"
#include "xattribute.h"
#include "tempcollect.h"
#include "stringlist.h"
#include "format.h"
#include "stdout.h"
#include "filelist.h"
#include "stdparse.h"
#include "tempvar.h"
#include "cstring.h"
#include "mysignal.h"

extern const char_t xpath_magic[];

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

typedef struct {
  tempcollect_t *tc;
  bool_t exists;
  bool_t active;
  bool_t newline;
  const char_t *fmt;
  char_t fmt_type;
  char_t val_type;
  unsigned int mindepth;
} argstatus_t;

typedef struct {
  argstatus_t *status;
  size_t count;
  size_t max;
  size_t mark;
} printf_args_t;

bool_t create_printf_args(printf_args_t *pargs) {
  pargs->count = 0;
  return create_mem(&pargs->status, &pargs->max, sizeof(argstatus_t), 16);      
}

void add_printf_args(printf_args_t *pargs, tempcollect_t *tc, const char_t *fmt) {
  argstatus_t *as;
  if( pargs && tc && fmt ) {
    if( pargs->count >= pargs->max ) {
      grow_mem(&pargs->status, &pargs->max, sizeof(argstatus_t), 16); 
    }
    if( pargs->count < pargs->max ) {
      as = &pargs->status[pargs->count];
      as->tc = tc;
      as->exists = FALSE;
      as->active = FALSE;
      as->newline = FALSE;
      as->fmt = fmt;

      for(; fmt[0] && fmt[1]; fmt++);
      as->fmt_type = *fmt;
      as->val_type = '\0';

      pargs->count++;
    }  
  }
}

bool_t find_printf_args(printf_args_t *pargs, const char_t *path, int *n) {
  size_t i;
  const char_t *p;
  char_t *q;

  if( pargs && path && n) {
    for(i = pargs->mark; i < pargs->count; i++) {
      p = pargs->status[i].tc->name;
      q = strchr(p, *xpath_magic);
      if( q ) {
	q++;
	if( strcmp(path, q) == 0) {
	  *n = i;
	  return TRUE;
	}
      }
    }
  }
  return FALSE;
}


bool_t free_printf_args(printf_args_t *pargs) {
  pargs->count = 0;
  return free_mem(&pargs->status, &pargs->max);
}

bool_t check_printf_args(printf_args_t *args) {
  size_t i;
  bool_t ok = TRUE;
  if( args ) {
    for(i = 0; i < args->count; i++) {
      if( !args->status[i].exists ) {
	errormsg(E_ERROR, "tag not found: %s\n", 
		 args->status[i].tc->name);
	ok = FALSE;
      }
    }
    return ok;
  }
  return FALSE;
}


typedef struct {
  stdparserinfo_t std;
  flag_t flags;

  cstringlst_t files;
  cstringlst_t *xpaths;
  int n;
  stringlist_t ufiles;
  stringlist_t cids;
  tclist_t collectors;
  xmatcher_t xm;
  xpredicatelist_t xp;
  xattributelist_t xa;
  tempvar_t tv; /* for text values */
  tempvar_t av; /* for attribute values */

  const char_t *format;
  stringlist_t fmt;
  charbuf_t pcs;
  printf_args_t args;
} parserinfo_printf_t;

#define PRINTF_FLAG_ACTIVE   0x01
#define PRINTF_FLAG_CHARDATA 0x02

#define PRINTF_VERSION       0x01
#define PRINTF_HELP          0x02
#define PRINTF_USAGE0 \
"Usage: xml-printf [OPTION]... FORMAT [[FILE]... [:XPATH]...]...\n" \
"Print the text value(s) of XPATH(s) according to FORMAT.\n" \
"\n" \
"      --help     display this help and exit\n" \
"      --version  display version information and exit\n" \
"This command prints the string FORMAT only, but FORMAT can contain\n" \
"C printf style formatting instructions which specify how subsequent\n" \
"arguments are converted for output.\n"

#define PRINTF_USAGE1 \
"Each subsequent argument represents the text value of an XML subtree,\n" \
"located in the XML document FILE and given by XPATH. If FILE is omitted,\n" \
"it is assumed that standard input contains a well formed XML document and\n" \
"XPATH is a path within that document.\n" \
"SOURCE is an optional XML file path, and XPATH represents a node relative\n" \
"to SOURCE, or if omitted, relative to the XML document in standard input.\n" \
"Only the text value of each XPATH is substituted.\n"

void set_option_printf(int op, char *optarg) {
  switch(op) {
  case PRINTF_VERSION:
    puts("xml-printf" COPYBLURB);
    exit(EXIT_SUCCESS);
    break;
  case PRINTF_HELP:
    puts(PRINTF_USAGE0);
    puts(PRINTF_USAGE1);
    exit(EXIT_SUCCESS);
    break;
  }
}

bool_t read_format(parserinfo_printf_t *pinfo, const char_t *format) {
  const char_t *p, *q;
  char_t *r;
  if( pinfo && format ) {
    pinfo->format = format;
    r = create_charbuf(&pinfo->pcs, 2 * strlen(format) + 1);
    while( *format ) {
      p = skip_delimiter(format, NULL, '%');
      if( *p ) {
	if( p[1] == '%' ) {
	  p += 2;
	} else {
	  q = skip_unescaped_delimiters(p + 1, NULL, "sdfgu", '\0') + 1;
	  add_stringlist(&pinfo->fmt, r, STRINGLIST_DONTFREE);
	  while( p < q ) {
	    *r++ = *p++;
	  }
	  *r++ = '\0';
	}
      }
      format = p;
    }

    return TRUE;
  }
  return FALSE;
}


/* for each file%fmt:xpath, create a collector id */
bool_t make_cids(parserinfo_printf_t *pinfo) {
  int i, f, x, s;
  cstring_t name;
  const char_t *format;
  if( pinfo && pinfo->files && pinfo->xpaths) {
    i = 0;
    for(f = 0; f < pinfo->n; f++) {
      for(x = 0; pinfo->xpaths[f][x]; x++) {

	format = get_stringlist(&pinfo->fmt, i++ % pinfo->fmt.num);

	s = strlen(pinfo->files[f]) + strlen(pinfo->xpaths[f][x]) + 
	  strlen(format) + 2;

	create_cstring(&name, "", s);
	vstrcat_cstring(&name, pinfo->files[f], format, xpath_magic, 
			pinfo->xpaths[f][x], NULLPTR);

	add_stringlist(&pinfo->cids, p_cstring(&name), STRINGLIST_FREE); 

      }
    }
    return TRUE;
  }
  return FALSE;
}

void unique_files(parserinfo_printf_t *pinfo) {
  int i;
  if( pinfo ) {
    for(i = 0; i < pinfo->n; i++) {
      add_unique_stringlist(&pinfo->ufiles, pinfo->files[i], 
			    STRINGLIST_DONTFREE);
    }
  }
}

void assign_file_collectors(parserinfo_printf_t *pinfo, const char *inputfile) {
  int i, f, x, w;
  const char *name, *fmt;
  tempcollect_t *tc;

  if( pinfo && inputfile ) {
    reset_xmatcher(&pinfo->xm);
    reset_xpredicatelist(&pinfo->xp);
    reset_xattributelist(&pinfo->xa);
    /* anything below pinfo->collectors.num is for another file */
    pinfo->args.mark = pinfo->collectors.num;
    i = 0;
    for(f = 0; f < pinfo->n; f++) {
      if( strcmp(inputfile, pinfo->files[f]) == 0 ) {
	for(x = 0; pinfo->xpaths[f][x]; x++) {
	  if( push_unique_xmatcher(&pinfo->xm, pinfo->xpaths[f][x],
				   XMATCHER_DONTFREE) &&
	      add_xpredicatelist(&pinfo->xp, pinfo->xpaths[f][x]) &&
	      add_xattributelist(&pinfo->xa, pinfo->xpaths[f][x]) ) {
	    name = get_stringlist(&pinfo->cids, i);
	    fmt = get_stringlist(&pinfo->fmt, i % pinfo->fmt.num);
	    i++;

	    if( add_tclist(&pinfo->collectors, name,
			   MINVARSIZE, MAXVARSIZE) ) { 

	      tc = get_last_tclist(&pinfo->collectors);
	      if( tc ) {
		add_printf_args(&pinfo->args, tc, fmt);
		continue;
	      }
	    }
	    errormsg(E_FATAL, "couldn't create tempcollector for %s\n", name);
	  }
	}
      }
    }
    /* now prepare */
    for(w = pinfo->args.mark; w < pinfo->args.count; w++) {
      pinfo->args.status[w].active = FALSE;
    }
  }
}

bool_t activate_collector(argstatus_t *as, parserinfo_printf_t *pinfo, bool_t activate, char_t val_type) {
  if( as ) {
    if( activate ) {
      as->newline |= as->exists && !as->active; /* separate disjoint nodes */
      as->active = TRUE;
      as->mindepth = pinfo->std.depth;

      if( !as->exists ) {
	as->exists = TRUE;
	as->val_type = val_type;
      }

      /* printf("<path=%s, %s active=%d type=%c>\n", */
      /* 	     string_xpath(&pinfo->std.cp), */
      /* 	     as->tc->name, */
      /* 	     as->active, */
      /* 	     as->val_type); */
    } else {
      as->active = FALSE;
    }
    return TRUE;
  }
  return FALSE;
}

/* activate/deactivate all collectors of interest.  
 * 
 * If attrib != NULL, then we take care to not activate/deactivate
 * chardata collectors, only attribute value collectors.
 * If attrib == NULL, then activation/deactivation applies to all 
 * types of collectors.
 */
bool_t activate_collectors(parserinfo_printf_t *pinfo, const char_t *attrib) {
  int w, z, m;
  const xattribute_t *xa;
  const char_t *path;
  bool_t v, tmatch, amatch, active = FALSE;

  if( pinfo ) {
    for(z = pinfo->args.mark; z < pinfo->args.count; z++) {
      path = string_xpath(&pinfo->std.cp);
      m = match_no_att_no_pred_xpath(pinfo->xm.xpath[z], NULL, path);
      v = valid_xpredicate(&pinfo->xp.list[z]);
      xa = get_xattributelist(&pinfo->xa, z);
      tmatch = (!attrib && !xa->begin && 
		((m == 0) || (m == -1)) && v);
      amatch = (attrib && xa->begin && xa->precheck && 
		(m == 0) && v && match_xattribute(xa, attrib));

      if( attrib && xa->begin ) { 
	if( amatch ) {
	  /* this collector must be activated */
	  if( find_printf_args(&pinfo->args, 
			       get_xmatcher(&pinfo->xm, z), &w) ) {
	    activate_collector(&pinfo->args.status[w], pinfo, TRUE, 'a');
	    active = TRUE;
	  }
	} else {
	  /* this collector must be deactivated */
	  activate_collector(&pinfo->args.status[z], pinfo, FALSE, 'a');
	}
      } else if( !attrib ) { 
	if( tmatch ) {
	  /* this collector must be activated */
	  if( find_printf_args(&pinfo->args, 
			       get_xmatcher(&pinfo->xm, z), &w) ) {
	    activate_collector(&pinfo->args.status[w], pinfo, TRUE, 't');
	    active = TRUE;
	  }
	} else {
	  /* this collector must be deactivated */
	  activate_collector(&pinfo->args.status[z], pinfo, FALSE, 't');
	}
      }
    }
  }
  return active;
}

bool_t write_active_collectors(printf_args_t *pargs, tempvar_t *tv) {
  size_t i;
  const char *s;
  long int l;
  unsigned long int u;
  double g;
  bool_t ok = TRUE;

  if( pargs && tv && !is_empty_tempvar(tv) ) {

    for(i = pargs->mark; i < pargs->count; i++) {
      argstatus_t *as = &pargs->status[i];
      if( as->active && (as->val_type == *tv->tc.name) ) {
	if( as->newline ) {
	  /* printf("NEWLINE\n"); */
	  putc_tempcollect(as->tc, '\n');
	  as->newline = FALSE;
	}
	switch(as->fmt_type) {
	case 's':
	  s = string_tempvar(tv);
	  ok &= nprintf_tempcollect(as->tc, strlen(s), as->fmt, s);
	  break;
	case 'd':
	  l = long_tempvar(tv);
	  ok &= nprintf_tempcollect(as->tc, 16, as->fmt, l);
	  break;
	case 'u':
	  u = ulong_tempvar(tv);
	  ok &= nprintf_tempcollect(as->tc, 16, as->fmt, u);
	  break;
	case 'f':
	case 'g':
	  g = double_tempvar(tv);
	  ok &= nprintf_tempcollect(as->tc, 16, as->fmt, g);
	  break;
	default: /* do nothing */
	  ok = FALSE;
	  break;
	}
      }
    }
    reset_tempvar(tv);
    return ok;
  }
  return FALSE;
}

tempcollect_t *find_collector(parserinfo_printf_t *pinfo, int i) {
  const char *name;
  tempcollect_t *tc = NULL;
  if( pinfo ) {
    name = get_stringlist(&pinfo->cids, i);
    if( name ) {
      tc = find_byname_tclist(&pinfo->collectors, name);
      if( !tc ) {
	errormsg(E_FATAL, "cannot find data for %s\n", name);
      }
    }
  }
  return tc;
}

result_t start_tag(void *user, const char_t *name, const char_t **att) {
  parserinfo_printf_t *pinfo = (parserinfo_printf_t *)user;
  if( pinfo ) {
    update_xpredicatelist(&pinfo->xp, string_xpath(&pinfo->std.cp), att);
    update_xattributelist(&pinfo->xa, att);

    clearflag(&pinfo->flags,PRINTF_FLAG_CHARDATA);
    write_active_collectors(&pinfo->args, &pinfo->tv);
    flipflag(&pinfo->flags, PRINTF_FLAG_ACTIVE, 
	     activate_collectors(pinfo, NULL));
  }
  return PARSER_OK;
}

result_t end_tag(void *user, const char_t *name) {
  parserinfo_printf_t *pinfo = (parserinfo_printf_t *)user;
  if( pinfo ) {
    clearflag(&pinfo->flags,PRINTF_FLAG_CHARDATA);
    write_active_collectors(&pinfo->args, &pinfo->tv);
    flipflag(&pinfo->flags, PRINTF_FLAG_ACTIVE, 
	     activate_collectors(pinfo, NULL));
  }
  return PARSER_OK;
}

result_t attribute(void *user, const char_t *name, const char_t *value) {
  parserinfo_printf_t *pinfo = (parserinfo_printf_t *)user;
  if( pinfo ) { 
    flipflag(&pinfo->flags, PRINTF_FLAG_ACTIVE, 
	     activate_collectors(pinfo, name));
    if( true_and_clearflag(&pinfo->flags,PRINTF_FLAG_ACTIVE) ) {
      /* printf("squeezing-attribute[%.*s]\n", strlen(value), value); */
      squeeze_tempvar(&pinfo->av, value, strlen(value));
      write_active_collectors(&pinfo->args, &pinfo->av);
    }
  }
  return PARSER_OK;
}

result_t chardata(void *user, const char_t *buf, size_t buflen) {
  parserinfo_printf_t *pinfo = (parserinfo_printf_t *)user;
  if( pinfo ) {
    flipflag(&pinfo->flags, PRINTF_FLAG_ACTIVE, 
	     activate_collectors(pinfo, NULL));
    if( checkflag(pinfo->flags,PRINTF_FLAG_ACTIVE) ) {
      /* note: squeezing removes whitespace aggressively. 
       * If the selection contains several tags, this will remove
       * the whitespace that is contained *in between* the tags.
       * this is correct if xml-printf believes space between
       * tags is non-significant, but wrong otherwise. What to do?
       */
      /* printf("squeezing-chardata[%.*s]\n", buflen, buf); */
      squeeze_tempvar(&pinfo->tv, buf, buflen);
    }
  }
  return PARSER_OK;
}

bool_t start_file_fun(void *user, const char_t *file, const char_t **xpaths) {
  parserinfo_printf_t *pinfo = (parserinfo_printf_t *)user;
  if( pinfo ) {
    /* prepare all the collectors associated with file */
    assign_file_collectors(pinfo, file);
  }
  return TRUE;
}


/* count % args to see if all are used */
bool_t check_printf_format(parserinfo_printf_t *pinfo) {
  const char_t *s;
  int n, f, x;
  if( pinfo ) {

    for(n = 0, f = 0; f < pinfo->n; f++) { 
      for(x = 0; pinfo->xpaths[f][x]; x++) {
	n++;
      }
    }
    for(s = pinfo->format; *s; s++) {
      if( *s == '%' ) {
	s++;
	n--;
	if( *s == '%' ) {
	  n++;
	}
	if( !*s ) {
	  break;
	}
      }
    }
    return (n == 0);
  }
  return FALSE;
}

bool_t write_printf_format(parserinfo_printf_t *pinfo, bool_t reuse) {
  size_t i;
  const char_t *p, *q;
  tempcollect_t *tc;
  if( pinfo ) {
    if( check_printf_args(&pinfo->args) ) {
      i = 0; 
      do {
	p = pinfo->format;
	q = strpbrk(p, "%\\");
	while( q ) {
	  write_stdout((byte_t *)p, q - p);
	  p = q + 1;
	  if( *q == '%' ) {
	    q++;
	    if( *q == '%' ) {
	      putc_stdout('%');
	    } else {
	      tc = find_collector(pinfo, i);
	      if( tc ) {
		write_stdout_tempcollect(tc);
	      }
	      p = skip_unescaped_delimiters(p, NULL, "sdfgu", '\0');
	      i++;
	    }
	  } else if( *q == '\\' ) {
	    putc_stdout(convert_backslash(q));
	  }
	  p++;
	  q = strpbrk(p, "%\\");
	}
	puts_stdout(p);
      } while( reuse && (i < pinfo->cids.num) );

      return TRUE;
    }
  }
  return FALSE;
}

bool_t create_parserinfo_printf(parserinfo_printf_t *pinfo) {
  bool_t ok = TRUE;
  if( pinfo ) {
    memset(pinfo, 0, sizeof(parserinfo_printf_t));
    ok &= create_stdparserinfo(&pinfo->std);
    
    pinfo->std.setup.flags = STDPARSE_ALLNODES;
    pinfo->std.setup.cb.start_tag = start_tag;
    pinfo->std.setup.cb.end_tag = end_tag;
    pinfo->std.setup.cb.chardata = chardata;
    pinfo->std.setup.cb.attribute = attribute;

    pinfo->std.setup.start_file_fun = start_file_fun;

    ok &= create_stringlist(&pinfo->cids);
    ok &= create_stringlist(&pinfo->ufiles);
    ok &= create_xmatcher(&pinfo->xm);
    ok &= create_xpredicatelist(&pinfo->xp);
    ok &= create_xattributelist(&pinfo->xa);
    ok &= create_tempvar(&pinfo->tv, "t", MINVARSIZE, MAXVARSIZE);
    ok &= create_tempvar(&pinfo->av, "a", MINVARSIZE, MAXVARSIZE);
    ok &= create_tclist(&pinfo->collectors);
    ok &= create_printf_args(&pinfo->args);
    ok &= create_stringlist(&pinfo->fmt);

    return ok;
  }
  return FALSE;
}

bool_t free_parserinfo_printf(parserinfo_printf_t *pinfo) {
  if( pinfo ) {
    free_stdparserinfo(&pinfo->std);
    free_stringlist(&pinfo->cids);
    free_stringlist(&pinfo->ufiles);
    free_xmatcher(&pinfo->xm);
    free_xpredicatelist(&pinfo->xp);
    free_xattributelist(&pinfo->xa);
    free_tempvar(&pinfo->tv);
    free_tempvar(&pinfo->av);
    free_tclist(&pinfo->collectors);
    free_printf_args(&pinfo->args);
    free_stringlist(&pinfo->fmt);
    free_charbuf(&pinfo->pcs);
    return TRUE;
  }
  return FALSE;
}

int main(int argc, char **argv) {
  signed char op;
  parserinfo_printf_t pinfo;
  int exit_value = EXIT_FAILURE;
  struct option longopts[] = {
    { "version", 0, NULL, PRINTF_VERSION },
    { "help", 0, NULL, PRINTF_HELP },
    { 0 }
  };

  filelist_t fl;

  progname = "xml-printf";
  inputfile = "";
  inputline = 0;
  
  while( (op = getopt_long(argc, argv, "",
			   longopts, NULL)) > -1 ) {
    set_option_printf(op, optarg);
  }

  init_signal_handling(SIGNALS_DEFAULT); 
  init_file_handling();

  if( create_parserinfo_printf(&pinfo) ) {

    if( !read_format(&pinfo, argv[optind++]) ) {
      errormsg(E_FATAL, "bad format string.\n");
    }

    if( create_filelist(&fl, -1, argv + optind, 0) ) {

      pinfo.files = getfiles_filelist(&fl);
      pinfo.xpaths = getxpaths_filelist(&fl);
      pinfo.n = getsize_filelist(&fl);

      /* if( !check_printf_format(&pinfo) ) { */
      /* 	errormsg(E_FATAL, "argument count mismatch in format string\n"); */
      /* } */

      if( make_cids(&pinfo) ) {

	unique_files(&pinfo);
	open_stdout();
	if( stdparse2(pinfo.ufiles.num, pinfo.ufiles.list, 
		      NULL /* no stdselect */, &pinfo.std) ) {

	  if( write_printf_format(&pinfo, TRUE) ) {
	    exit_value = EXIT_SUCCESS;
	  }

	}
	close_stdout();
      }
      free_filelist(&fl);
    }
    free_parserinfo_printf(&pinfo);
  }

  exit_file_handling();
  exit_signal_handling();

  return exit_value;
}
