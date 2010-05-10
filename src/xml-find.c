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
#include "entities.h"
#include "myerror.h"
#include "wrap.h"
#include "stdout.h"
#include "stdparse.h"
#include "stringlist.h"
#include "tempcollect.h"
#include "cstring.h"
#include "tempfile.h"
#include "mysignal.h"

#include <stdio.h>
#include <string.h>
#include <getopt.h>
#include <unistd.h>
#include <sys/types.h>

/* for option processing */
extern char *optarg;
extern int optind, opterr, optopt;

extern char *progname;
extern char *inputfile;
extern long inputline;

extern volatile flag_t cmd;

extern const char_t escc;
extern const char_t xpath_delims[];
extern const char_t xpath_specials[];

#include <stdio.h>

typedef enum { 
  NOP=0, TRU, FALS, AND, OR, NOT, COMMA, OPEN, CLOSE, 
  NAME, PATH,
  PRINT, EXEC, EXECNODE,
  /* argument-like IDs must follow ARG value, needed by parse_expressions() */
  ARG=100, SUBP, SUBA, SUBF, SEMIC
} expid_t;

typedef struct { 
  expid_t id;
  union {
    const char_t *string;
    int integer;
  } arg;
} exp_t;

typedef struct {
  exp_t *list;
  int num;
  size_t max;
} explist_t;

bool_t create_explist(explist_t *el) {
  if( el ) {
    el->num = 0;
    return create_mem(&el->list, &el->max, sizeof(exp_t), 16);
  }
  return FALSE;
}

bool_t free_explist(explist_t *el) {
  if( el ) {
    if( el->list ) {
      free_mem(&el->list, &el->max);
    }
  }
  return FALSE;
}

exp_t *add_explist(explist_t *el) {
  exp_t *e = NULL;
  if( el ) {
    if( el->num >= el->max ) {
      grow_mem(&el->list, &el->max, sizeof(exp_t), 16);
    }
    if( el->num < el->max ) {
      e = &el->list[el->num];
      memset(e, 0, sizeof(exp_t));
      el->num++;
    }
  }
  return e;
}

exp_t *get_explist(explist_t *el, int n) {
  return (el && (0 <= n) && (n < el->num)) ? &el->list[n] : NULL;
}

/* contains delayed information about a node,
 * everything that is needed for evaluating expressions
 * but can't be known until we've traversed the end tag 
 */
typedef struct {
  int depth;
  cstring_t path;
  charbuf_t atts;
  cstring_t basename;
  struct {
    cstring_t file;
    int start, stop;
  } xml;
} findnode_t;

bool_t create_findnode(findnode_t *f) {
  if( f ) {
    memset(f, 0, sizeof(findnode_t));
    return TRUE;
  }
  return FALSE;
}

bool_t free_findnode(findnode_t *f) {
  if( f ) {
    free_cstring(&f->path);
    free_cstring(&f->basename);
    free_charbuf(&f->atts);
    if( CSTRINGP(f->xml.file) ) { 
      remove_tempfile(p_cstring(&f->xml.file));
      free_cstring(&f->xml.file);
    }
    return TRUE;
  }
  return FALSE;
}

typedef struct {
  findnode_t *list;
  int num;
  size_t max;
} findnodelist_t;

bool_t create_findnodelist(findnodelist_t *fnl) {
  if( fnl ) {
    fnl->num = 0;
    return create_mem(&fnl->list, &fnl->max, sizeof(findnode_t), 16);
  }
  return FALSE;
}

bool_t reset_findnodelist(findnodelist_t *fnl) {
  int i;
  if( fnl ) {
    for(i = 0; i < fnl->num; i++) {
      free_findnode(&fnl->list[i]);
    }
    fnl->num = 0;
  }
  return FALSE;
}

bool_t free_findnodelist(findnodelist_t *fnl) {
  if( fnl ) {
    if( fnl->list ) {
      reset_findnodelist(fnl);
      free_mem(&fnl->list, &fnl->max);
    }
  }
  return FALSE;
}

findnode_t *add_findnodelist(findnodelist_t *fnl) {
  findnode_t *f = NULL;
  if( fnl ) {
    if( fnl->num >= fnl->max ) {
      grow_mem(&fnl->list, &fnl->max, sizeof(findnode_t), 16);
    }
    if( fnl->num < fnl->max ) {
      f = &fnl->list[fnl->num];
      if( create_findnode(f) ) {
	fnl->num++;
      } else {
	f = NULL;
      }
    }
  }
  return f;
}

findnode_t *get_findnodelist(findnodelist_t *fnl, int n) {
  return (fnl && (0 <= n) && (n < fnl->num)) ? &fnl->list[n] : NULL;
}

findnode_t *find_findnodelist(findnodelist_t *fnl, int depth) {
  int n;
  if( fnl ) {
    for(n = 0; n < fnl->num; n++) {
      if( fnl->list[n].depth == depth ) {
	return &fnl->list[n];
      }
    }
  }
  return NULL;
}

findnode_t *find_xmlstop_findnodelist(findnodelist_t *fnl, int depth) {
  int n;
  findnode_t *f;
  if( fnl ) {
    for(n = 0; n < fnl->num; n++) {
      f = &fnl->list[n];
      if( (f->depth == depth) && (f->xml.stop == 0) ) {
	return f;
      }
    }
  }
  return NULL;
}

typedef struct {
  stdparserinfo_t std; /* must be first so we can cast correctly */
  flag_t flags;

  findnodelist_t nodes;
  explist_t expressions;
  tempcollect_t sav;
  int savd;

  stringlist_t tmp;
} parserinfo_find_t;

#define FIND_VERSION    0x01
#define FIND_HELP       0x02
#define FIND_USAGE \
"Usage: xml-find [[FILE]... [:XPATH]...]... [EXPRESSION]\n" \
"Search for XML nodes in an FILE, or standard input, and evaluate\n" \
"EXPRESSION on each.\n" \
"      --help     display this help and exit\n" \
"      --version  display version information and exit\n"

#define FIND_FLAG_DELAYNODES   0x01
#define FIND_FLAG_SAVEPATH     0x02
#define FIND_FLAG_SAVEBASE     0x04
#define FIND_FLAG_SAVEATTS     0x08
#define FIND_FLAG_SAVEXML      0x10
#define FIND_FLAG_HAS_ACTION   0x20

void set_option_find(int op, char *optarg) {
  switch(op) {
  case FIND_VERSION:
    puts("xml-find" COPYBLURB);
    exit(EXIT_SUCCESS);
    break;
  case FIND_HELP:
    puts(FIND_USAGE);
    exit(EXIT_SUCCESS);
    break;
  }
}

/* assume all options and expressions are after the list of filenames */ 
int first_expression(int argc, char **argv) {
  int index = -1;
  if( argv ) {
    for(index = 0; index < argc; index++) {
      if( (argv[index][0] == '(') ||
	  (argv[index][0] == '!') ||
	  ((argv[index][0] == '-') && (argv[index][1] != '-')) ) {
	return index;
      }
    }
  }
  return index;
}

/* Note: the expression list contains (unnecessary) NOP codes, but
 * these are needed to keep the expression number in sync with the 
 * argv position number.
 */
bool_t parse_expressions(parserinfo_find_t *pinfo, char **argv) {
  exp_t *e, *f;
  int i;
  if( pinfo && argv ) {
    if( *argv ) {
      e = add_explist(&pinfo->expressions);
      if( e ) {
	if( strcmp(*argv,"(") == 0 ) {
	  e->id = OPEN;
	} else if( strcmp(*argv,")") == 0 ) {
	  e->id = CLOSE;
	  for(i = pinfo->expressions.num - 1; i >= 0; i--) {
	    f = get_explist(&pinfo->expressions, i);
	    if( f->id == OPEN ) {
	      f->arg.integer = pinfo->expressions.num - 1;
	      break;
	    } else if( i == 0 ) {
	      errormsg(E_FATAL, "missing open parenthesis.\n");
	    }
	  }
	} else if( strcmp(*argv,"!") == 0 ) {
	  e->id = NOT;
	  e->arg.integer = 0; /* computed later */
	} else if( strcmp(*argv,",") == 0 ) {
	  e->id = COMMA;
	} else if( strcmp(*argv,"-true") == 0 ) {
	  e->id = TRU;
	} else if( strcmp(*argv,"-false") == 0 ) {
	  e->id = FALS;
	} else if( strcmp(*argv,"-a") == 0 ) {
	  e->id = AND;
	} else if( strcmp(*argv,"-o") == 0 ) {
	  e->id = OR;
	} else if( strcmp(*argv,"-print") == 0 ) {
	  e->id = PRINT;
	  setflag(&pinfo->flags,FIND_FLAG_SAVEPATH);
	  setflag(&pinfo->flags,FIND_FLAG_HAS_ACTION);
	} else if( strcmp(*argv,"-execnode") == 0 ) {
	  e->id = EXECNODE;
	  setflag(&pinfo->flags,FIND_FLAG_HAS_ACTION);
	} else if( strcmp(*argv,"-exec") == 0 ) {
	  e->id = EXEC;
	  setflag(&pinfo->flags,FIND_FLAG_HAS_ACTION);
	} else if( strcmp(*argv,"-name") == 0 ) {
	  e->id = NAME;
	  argv++;
	  if( argv && *argv ) {
	    e->arg.string = *argv;
	    e = add_explist(&pinfo->expressions);
	    e->id = NOP; /* keep argument counts correct */
	  } else {
	    errormsg(E_FATAL, "missing argument for -name switch.\n");
	  }
	  setflag(&pinfo->flags,FIND_FLAG_SAVEBASE);
	} else if( strcmp(*argv,"-path") == 0 ) {
	  e->id = PATH;
	  argv++;
	  if( argv && *argv ) {
	    e->arg.string = *argv;
	    e = add_explist(&pinfo->expressions);
	    e->id = NOP; /* keep argument counts correct */
	  } else {
	    errormsg(E_FATAL, "missing argument for -path switch.\n");
	  }
	  setflag(&pinfo->flags,FIND_FLAG_SAVEPATH);
	} else {
	  errormsg(E_FATAL, "bad expression %s\n", *argv);
	} 
	if( (e->id == EXEC) || (e->id == EXECNODE) ) {
	  i = pinfo->expressions.num - 1;
	  argv++;
	  while( argv && *argv ) {
	    e = add_explist(&pinfo->expressions);
	    if( e ) {
	      if( strcmp(*argv, "{}") == 0 ) {
		e->id = SUBP; /* substitute node path */
		setflag(&pinfo->flags,FIND_FLAG_SAVEPATH);
		setflag(&pinfo->flags,FIND_FLAG_SAVEBASE);
	      } else if( strcmp(*argv, "{@}") == 0 ) {
		e->id = SUBA; /* substitute attributes */
		setflag(&pinfo->flags,FIND_FLAG_SAVEATTS);
	      } else if( strcmp(*argv, "{-}") == 0 ) {
		e->id = SUBF; /* substitute temporary file */
		e->arg.string = make_template_tempfile(progname);
		setflag(&pinfo->flags,FIND_FLAG_SAVEXML);
		setflag(&pinfo->flags,FIND_FLAG_SAVEPATH);
	      } else if( strcmp(*argv,";") == 0 ) {
		e->id = SEMIC;
		break;
	      } else {
		e->id = ARG;
		e->arg.string = *argv;
	      }
	    }	    
	    argv++;
	  }
	  if( !e || (e->id != SEMIC) ) {
	    errormsg(E_FATAL, "exec command must end with semicolon ';'.\n");
	  }
	  f = get_explist(&pinfo->expressions, i);
	  f->arg.integer = pinfo->expressions.num - 1;
	}
      }
      return parse_expressions(pinfo, ++argv);
    }

    /* static checking phase */

    if( !checkflag(pinfo->flags,FIND_FLAG_HAS_ACTION) ) {
      e = add_explist(&pinfo->expressions);
      if( e ) {
	e->id = PRINT;
	setflag(&pinfo->flags,FIND_FLAG_SAVEPATH);
      }
    } 
    
    for(i = 0; i < pinfo->expressions.num; i++) {
      f = get_explist(&pinfo->expressions, i);
      if( (f->id == OPEN) && (f->arg.integer == 0) ) {
	errormsg(E_FATAL, "missing close parenthesis.\n");
      } else if( f->id == NOT ) {
	f->arg.integer = i + 1;
	e = get_explist(&pinfo->expressions, f->arg.integer);
	if( e->id == OPEN ) {
	  /* NOT applies to the next block only */
	  f->arg.integer = e->arg.integer;
	} else {
	  /* NOT applies only to the next command-with-args 
	   * any id > ARG is treated as an ARG 
	   */
	  while(f->arg.integer < pinfo->expressions.num) {
	    f->arg.integer++;
	    e = get_explist(&pinfo->expressions, f->arg.integer);
	    if( e->id < ARG ) {
	      break;
	    }
	  }
	}
      }
    }

    if( checkflag(pinfo->flags,FIND_FLAG_SAVEXML) ) {
      setflag(&pinfo->flags,FIND_FLAG_DELAYNODES);
    }
  }
  return FALSE;
}

bool_t action_print(findnode_t *node) {
  if( node ) {
    puts_stdout(p_cstring(&node->path));
    putc_stdout('\n');
    return TRUE;
  }
  return FALSE;
}

/* this is a hack. Ideally, close_xpath() should handle this, 
 * but it was written before it existed and it works :)
 */
bool_t write_path_file(int fd, const char_t *path, bool_t fwd) {
  const char_t *stag, *etag;
  if( (fd != -1) && path && *path ) {
    if( fwd ) {
      while( *path ) {
	if( *path == *xpath_delims ) path++;
	etag = skip_unescaped_delimiters(path, NULL, xpath_specials, escc);
	if( *etag ) { /* dont print last tag */
	  write_file(fd, (byte_t *)"<", 1);
	  write_file(fd, (byte_t *)path, etag - path);
	  write_file(fd, (byte_t *)">\n", 2);
	}
	path = skip_unescaped_delimiters(etag, NULL, xpath_delims, escc);
      }
    } else {
      /* don't print last tag */
      etag = path + strlen(path);
      stag = rskip_unescaped_delimiter(path, etag, *xpath_delims, escc);
      do {
	etag = stag - 1;
	stag = rskip_unescaped_delimiter(path, etag, *xpath_delims, escc);
	if( stag >= path ) {
	  write_file(fd, (byte_t *)"<", 1);
	  write_file(fd, (byte_t *)stag, etag - stag + 1);
	  write_file(fd, (byte_t *)">\n", 2);
	}
      } while( stag > path );
    }
    return TRUE;
  }
  return FALSE;
}

typedef struct {
  int fd;
  tempcollect_t *tc;
  int pos, start, stop;
} write_xml_tempfile_fun_t;

bool_t write_xml_tempfile_fun(void *user, byte_t *buf, size_t buflen) {
  write_xml_tempfile_fun_t *wxtf = (write_xml_tempfile_fun_t *)user;
  bool_t retval = TRUE;
  int bufstart, bufend;
  if( wxtf ) {
    bufstart = wxtf->pos;
    bufend = wxtf->pos + buflen;
    if( (bufstart <= wxtf->start) && (wxtf->start < bufend) ) {
      if( bufend < wxtf->stop ) {
	retval = write_file(wxtf->fd, buf + (wxtf->start - bufstart),
			    bufend - wxtf->start);
	wxtf->start = bufend;
      } else {
	retval = write_file(wxtf->fd, buf + (wxtf->start - bufstart), 
			    wxtf->stop - wxtf->start);
      }
    }
    wxtf->pos += buflen;
    return retval && (wxtf->pos < wxtf->stop);
  }
  return FALSE;
}

bool_t write_xml_tempfile(tempcollect_t *sav, char *tmplate,
			  int start, int stop, const char_t *path) {
  write_xml_tempfile_fun_t wxtf;
  tempcollect_adapter_t ad;

  if( sav && (start <= stop) ) {
    wxtf.fd = open_tempfile(tmplate);
    if( wxtf.fd != -1 ) {

      if( write_file(wxtf.fd, (byte_t *)get_headwrap(), 
		     strlen(get_headwrap())) ) {
	write_path_file(wxtf.fd, path, TRUE);
	wxtf.tc = sav;
	wxtf.pos = 0;
	wxtf.start = start;
	wxtf.stop = stop;
	ad.fun = write_xml_tempfile_fun;
	ad.user = &wxtf;
	write_adapter_tempcollect(sav, &ad);
	write_path_file(wxtf.fd, path, FALSE);
      }

      close(wxtf.fd);
      return TRUE;
    }
  }
  
  return FALSE;
}

bool_t action_exec(explist_t *el, findnode_t *node, tempcollect_t *sav,
		   int nstart, int nstop, stringlist_t *tmp, 
		   bool_t relative) {
  const char *filename, *a;
  char *p;
  exp_t *f;
  int n;
  if( el && node && tmp ) {
    reset_stringlist(tmp);
    f = get_explist(el, nstart);    
    filename = f->arg.string;  
    /* zeroth arg is filename */
    add_stringlist(tmp, f->arg.string, STRINGLIST_DONTFREE);
    for(n = nstart + 1; n < nstop; n++) {
      f = get_explist(el, n);    
      switch(f->id) {
      case ARG:
	add_stringlist(tmp, f->arg.string, STRINGLIST_DONTFREE);
	break;
      case SUBP:
	a = relative ? p_cstring(&node->basename) : p_cstring(&node->path);
	add_stringlist(tmp, a, STRINGLIST_DONTFREE);
	break;
      case SUBA:
	a = begin_charbuf(&node->atts);
	while(a && *a ) {
	  add_stringlist(tmp, a, STRINGLIST_DONTFREE);
	  a += strlen(a) + 1;
	  add_stringlist(tmp, a, STRINGLIST_DONTFREE);
	  a += strlen(a) + 1;
	}
	break;
      case SUBF:
	strdup_cstring(&node->xml.file, f->arg.string);
	p = p_cstring(&node->xml.file);
	a = relative ? "" : p_cstring(&node->path);
	write_xml_tempfile(sav, p, node->xml.start, node->xml.stop, a);
	add_stringlist(tmp, p, STRINGLIST_DONTFREE);
	break;
      default:
	errormsg(E_FATAL, 
		 "unexpected exec error (arg=%d,id=%d).\n", n, f->id);
      }

    }
    return exec_cmdline(filename, argv_stringlist(tmp));
  }
  return FALSE;
}

bool_t eval_expressions(explist_t *el, findnode_t *node, tempcollect_t *sav,
			int n, int nstop, stringlist_t *tmp) {
  exp_t *e;
  bool_t ok = TRUE;
  /* printf("eval_expressions %d %d\n", n, nstop); */
  if( el && node && tmp && (nstop <= el->num) ) {
    while( n < nstop ) {
      e = get_explist(el, n);    
      /* printf("n=%d,id=%d\n", n, e->id); */
      if( !ok && (e->id != OR ) ) {
	break;
      }
      if( e->id == OPEN ) {
	ok &= eval_expressions(el, node, sav, n + 1, e->arg.integer, tmp);
	n = e->arg.integer;
      } else if( e->id == CLOSE ) {
	n++;
      } else if( e->id == COMMA ) {
	ok = TRUE;
	n++;
      } else if( e->id == TRU ) {
	ok &= TRUE;
	n++;
      } else if( e->id == FALS ) {
	ok &= FALSE;
	n++;
      } else if( e->id == AND ) {
	/* implicit */
	n++;
      } else if( e->id == OR ) {
	ok = (ok || eval_expressions(el, node, sav, n + 1, nstop, tmp));
	n = nstop;
      } else if( e->id == NOT ) {
	ok &= !eval_expressions(el, node, sav, n + 1, e->arg.integer, tmp);
	n = e->arg.integer;
      } else if( e->id == PRINT ) {
	ok &= action_print(node);
	n++;
      } else if( e->id == EXEC ) {
	ok &= action_exec(el, node, sav, n + 1, e->arg.integer, tmp, FALSE);
	n = e->arg.integer;
      } else if( e->id == EXECNODE ) {
	ok &= action_exec(el, node, sav, n + 1, e->arg.integer, tmp, TRUE);
	n = e->arg.integer;
      } else if( e->id == SEMIC ) {
	n++;
      } else if( e->id == NAME ) {
	ok &= globmatch(begin_cstring(&node->basename), 
			end_cstring(&node->basename), e->arg.string);
	n += 2;
      } else if( e->id == PATH ) {
	ok &= globmatch(begin_cstring(&node->path), 
			end_cstring(&node->path), e->arg.string);
	n += 2;
      } else if( e->id == NOP ) {
	n++;
      } else {
	/* should never happen, unless I'm debugging ;-) */
	errormsg(E_FATAL, 
		 "unexpected eval error (arg=%d,id=%d).\n", n, e->id);
      }

    }
    /* printf("/eval_expressions n=%d, %d\n", n, ok); */
    return ok;
  }
  return FALSE;
}

bool_t process_available_nodes(findnodelist_t *fnl, explist_t *el, 
			       tempcollect_t *sav, stringlist_t *tmp) {
  int i;
  findnode_t *f;
  if( fnl && el && sav ) {
    for(i = 0; i < fnl->num; i++) {
      f = get_findnodelist(fnl, i);
      eval_expressions(el, f, sav, 0, el->num, tmp);
    }
    reset_findnodelist(fnl);
    reset_tempcollect(sav);
    return TRUE;
  }
  return FALSE;
}
    
bool_t strdup_attributes(charbuf_t *cb, const char_t **att) {
  
  char_t *a;
  const char_t **btt;
  size_t length;
  if( att ) {
    for(length = 0, btt = att; btt && *btt; btt += 2 ) {
      length += strlen(btt[0]) + 1 + strlen(btt[1]) + 1;
    }
    a = create_charbuf(cb, length + 2);
    for( ; att && *att; att += 2) {
      a = write_charbuf(cb, a, att[0], strlen(att[0]) + 1);
      a = write_charbuf(cb, a, att[1], strlen(att[1]) + 1);
    }
    a = write_charbuf(cb, a, "\0\0", 2);

    return (a != NULL);
  }
  return FALSE;
}

result_t start_tag(void *user, const char_t *name, const char_t **att) {
  parserinfo_find_t *pinfo = (parserinfo_find_t *)user;
  findnode_t *f;
  if( pinfo ) { 
    f = add_findnodelist(&pinfo->nodes);
    if( f ) {
      f->depth = pinfo->savd;
      if( checkflag(pinfo->flags,FIND_FLAG_SAVEPATH) ) {
	strdup_cstring(&f->path, get_full_xpath(&pinfo->std.cp));
      }
      if( checkflag(pinfo->flags,FIND_FLAG_SAVEBASE) ) {
	strdup_cstring(&f->basename, get_last_xpath(&pinfo->std.cp));
      }
      if( checkflag(pinfo->flags,FIND_FLAG_SAVEATTS) ) {
	strdup_attributes(&f->atts, att);
      }
      if( checkflag(pinfo->flags,FIND_FLAG_SAVEXML) ) {
	f->xml.start = tell_tempcollect(&pinfo->sav);
	write_start_tag_tempcollect(&pinfo->sav, name, att);
      }
    }

    if( !checkflag(pinfo->flags,FIND_FLAG_DELAYNODES) ) {
      process_available_nodes(&pinfo->nodes, &pinfo->expressions, 
			      &pinfo->sav, &pinfo->tmp);
    }

    pinfo->savd++;
  }
  return PARSER_OK|PARSER_DEFAULT;
}

result_t end_tag(void *user, const char_t *name) {
  parserinfo_find_t *pinfo = (parserinfo_find_t *)user;
  findnode_t *f;
  if( pinfo ) { 
    pinfo->savd--;
    if( checkflag(pinfo->flags,FIND_FLAG_SAVEXML) ) {
      write_end_tag_tempcollect(&pinfo->sav, name);
      f =find_xmlstop_findnodelist(&pinfo->nodes, pinfo->savd);
      if( f ) {
	f->xml.stop = tell_tempcollect(&pinfo->sav);
      }
    }

    if( pinfo->savd <= 0 ) {
      process_available_nodes(&pinfo->nodes, &pinfo->expressions, 
			      &pinfo->sav, &pinfo->tmp);
    }
  }
  return PARSER_OK|PARSER_DEFAULT;
}

result_t attribute(void *user, const char_t *name, const char_t *value) {
  parserinfo_find_t *pinfo = (parserinfo_find_t *)user;
  if( pinfo ) {
    /* not implemented */
  }
  return PARSER_OK;
}

result_t chardata(void *user, const char_t *buf, size_t buflen) {
  parserinfo_find_t *pinfo = (parserinfo_find_t *)user;
  if( pinfo && (pinfo->savd >= 0) ) { 
    if( checkflag(pinfo->flags,FIND_FLAG_SAVEXML) ) {
      write_coded_entities_tempcollect(&pinfo->sav, buf, buflen);
    }
  }
  return PARSER_OK;
}

bool_t create_parserinfo_find(parserinfo_find_t *pinfo) {
  bool_t ok = TRUE;
  if( pinfo ) {

    memset(pinfo, 0, sizeof(parserinfo_find_t));
    ok &= create_stdparserinfo(&pinfo->std);

    pinfo->std.setup.flags = STDPARSE_MIN1FILE; 
    pinfo->std.setup.cb.start_tag = start_tag;
    pinfo->std.setup.cb.end_tag = end_tag;
    pinfo->std.setup.cb.chardata = chardata;
    /* pinfo->std.setup.cb.attribute = attribute; */

    ok &= create_findnodelist(&pinfo->nodes);
    ok &= create_tempcollect(&pinfo->sav, "sav", MINVARSIZE, MAXVARSIZE);
    ok &= create_explist(&pinfo->expressions);
    ok &= create_stringlist(&pinfo->tmp);
    return ok;
  }
  return FALSE;
} 

bool_t free_parserinfo_find(parserinfo_find_t *pinfo) {
  free_stdparserinfo(&pinfo->std);
  free_findnodelist(&pinfo->nodes);
  free_tempcollect(&pinfo->sav);
  free_explist(&pinfo->expressions);
  free_stringlist(&pinfo->tmp);
  return TRUE;
}

int main(int argc, char **argv) {
  signed char op;
  parserinfo_find_t pinfo;
  int fe;

  struct option longopts[] = {
    { "version", 0, NULL, FIND_VERSION },
    { "help", 0, NULL, FIND_HELP },
    { 0 }
  };

  progname = "xml-find";
  inputfile = "";
  inputline = 0;

  if( create_parserinfo_find(&pinfo) ) {

    fe = first_expression(argc, argv);

    while( (op = getopt_long(fe, argv, "",
			     longopts, NULL)) > -1 ) {
      set_option_find(op, optarg);
    }

    init_signal_handling();
    init_file_handling();
    init_tempfile_handling();

    parse_expressions(&pinfo, argv + fe);

    open_stdout();

    stdparse(fe - 1, argv + optind, (stdparserinfo_t *)&pinfo);

    close_stdout();

    exit_tempfile_handling();
    exit_file_handling();
    exit_signal_handling();

    free_parserinfo_find(&pinfo);
  }
  return EXIT_SUCCESS;
}
