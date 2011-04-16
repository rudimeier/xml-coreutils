/* 
 * Copyright (C) 2011 Laird Breyer
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
#include "wrap.h"
#include "stdout.h"
#include "stdparse.h"
#include "entities.h"
#include "stdprint.h"
#include "tempcollect.h"
#include "filelist.h"
#include "rcm.h"
#include "tempfile.h"
#include "mysignal.h"
#include "objstack.h"

#include <string.h>
#include <getopt.h>

/* for option processing */
extern char *optarg;
extern int optind, opterr, optopt;

extern char *progname;
extern char *inputfile;
extern long inputline;

extern volatile flag_t cmd;

const char *tabname = "tab";
const char *tabpath = "/*/tab";

#include <stdio.h>

typedef struct {
  const char *filename;
  const char *xpath;
} fx_t;

#define NODEREADER_TEMPORARY 0x01

typedef struct {
  stdparserinfo_t std; /* must be first */
  enum {nr_nodata = 0, nr_ready, nr_stopped, nr_dead, nr_endofile} state;
  stream_t strm;
  parser_t parser;
  tempcollect_t sav;
  const char *xp[2];
  flag_t flags;
} nodereader_t;

result_t start_tag(void *user, const char_t *name, const char_t **att) {
  nodereader_t *nr = (nodereader_t *)user;
  if( nr ) { 
    if( nr->std.sel.active ) {
      if( checkflag(nr->flags, NODEREADER_TEMPORARY) &&
	  (nr->std.depth == 2) &&
	  (strcmp(name, tabname) == 0) ) {
	/* don't include <tab> from temporary files */
      } else {
	write_start_tag_tempcollect(&nr->sav, name, att);
      }
    }
  }
  return PARSER_OK;
}

result_t end_tag(void *user, const char_t *name) {
  nodereader_t *nr = (nodereader_t *)user;
  if( nr ) { 
    if( nr->std.sel.active ) {
      
      if( checkflag(nr->flags, NODEREADER_TEMPORARY) &&
	  (nr->std.depth == 2) &&
	  (strcmp(name, tabname) == 0) ) {
	/* don't include <tab> from temporary files */
      } else {
	write_end_tag_tempcollect(&nr->sav, name);
      }
      /* we detect the end of selection by comparing the 
	 end-tag depth with the minimum selection depth
	 (which corresponds to the first active start-tag)
	 Note: we must reset the mindepth before the next
	 selection starts or we will get nonsense.
      */
      if( nr->std.depth <= nr->std.sel.mindepth ) {
	nr->std.sel.mindepth = INFDEPTH; 
	return PARSER_STOP;
      }
    }
  }
  return PARSER_OK;
}

result_t chardata(void *user, const char_t *buf, size_t buflen) {
  nodereader_t *nr = (nodereader_t *)user;
  if( nr ) { 
    if( nr->std.sel.active ) {
      write_tempcollect(&nr->sav, (byte_t *)buf, buflen);
    }
  }
  return PARSER_OK;
}

result_t dfault(void *user, const char_t *data, size_t buflen) {
  nodereader_t *nr = (nodereader_t *)user;
  if( nr ) { 
    if( nr->std.sel.active ) {
      write_tempcollect(&nr->sav, (byte_t *)data, buflen);
    }
  }
  return PARSER_OK;
}


bool_t free_nodereader(nodereader_t *nr) {
  if( nr ) {
    stdparse3_free(&nr->parser, &nr->std);
    close_stream(&nr->strm);
    if( checkflag(nr->flags, NODEREADER_TEMPORARY) ) {
      remove_tempfile(nr->sav.name);
    }
    free_tempcollect(&nr->sav);
    free_stdparserinfo(&nr->std);
  }
  return FALSE;
}

bool_t create_nodereader(nodereader_t *nr, const char *filename, const char *xpath, flag_t flags) {
  bool_t ok = TRUE;
  if( nr ) {
    memset(nr, 0, sizeof(nodereader_t)); /* important */
    ok &= create_stdparserinfo(&nr->std);
    ok &= create_tempcollect(&nr->sav, filename, MINVARSIZE, MAXVARSIZE);
    ok &= open_file_stream(&nr->strm, filename);
    if( ok ) {

      nr->std.setup.flags = STDPARSE_ALLNODES|STDPARSE_ALWAYS_CHARDATA;
      nr->std.setup.cb.start_tag = start_tag;
      nr->std.setup.cb.end_tag = end_tag;
      nr->std.setup.cb.chardata = chardata;
      nr->std.setup.cb.dfault = dfault;

      ok &= stdparse3_create(&nr->parser, &nr->std);
      
      nr->xp[0] = xpath;
      ok &= setup_xpaths_stdselect(&nr->std.sel, nr->xp);
      
      nr->flags = flags;

    }
    if( !ok ) {
      free_nodereader(nr);
    }
    return ok;
  }
  return FALSE;
}


#define MAX_NR 16 /* max number of simultaneously open files */
typedef struct {
  nodereader_t list[MAX_NR];
  int num;
} nodereader_list_t;

bool_t add_nodereader_list(nodereader_list_t *nrl, const char *filename, const char *xpath, flag_t flags) {
  if( nrl && (nrl->num < MAX_NR) ) {
    if( create_nodereader(&nrl->list[nrl->num], filename, xpath, flags) ) {
      nrl->num++;
      return TRUE;
    }
  }
  return FALSE;
}

bool_t create_nodereader_list(nodereader_list_t *nrl) {
  if( nrl ) {
    memset(nrl, 0, sizeof(nodereader_list_t));
    return TRUE; 
  }
  return FALSE;
}

bool_t free_nodereader_list(nodereader_list_t *nrl) {
  int i;
  for(i = 0; nrl && (i < nrl->num); i++) {
    free_nodereader(&nrl->list[i]);
  }
  return TRUE;
}

bool_t reset_nodereader_list(nodereader_list_t *nrl) {
  if( nrl ) {
    free_nodereader_list(nrl);
    memset(nrl, 0, sizeof(nodereader_list_t));
    return TRUE;
  }
  return FALSE;
}

typedef struct {
  cstringlst_t files;
  cstringlst_t *xpaths;
  int n;

  objstack_t fxstack;
  nodereader_list_t nrl;
  enum {tab_start = 0, tab_end} tab;

} parserinfo_paste_t;

#define PASTE_VERSION    0x01
#define PASTE_HELP       0x02
#define PASTE_USAGE \
"Usage: xml-paste [OPTION]... [[FILE]... [:XPATH]...]...\n" \
"Merge selected nodes of FILE(s) sequentially.\n" \
"\n" \
"      --help     display this help and exit\n" \
"      --version  display version information and exit\n"

#define PASTE_FLAG_TARGET   0x01


void set_option_paste(int op, char *optarg, parserinfo_paste_t *pinfo) {
  switch(op) {
  case PASTE_VERSION:
    puts("xml-paste" COPYBLURB);
    exit(EXIT_SUCCESS);
    break;
  case PASTE_HELP:
    puts(PASTE_USAGE);
    exit(EXIT_SUCCESS);
    break;
  }
}



bool_t create_parserinfo_paste(parserinfo_paste_t *pinfo) {

  bool_t ok = TRUE;
  if( pinfo ) {
    memset(pinfo, 0, sizeof(parserinfo_paste_t));
    ok &= create_nodereader_list(&pinfo->nrl);
    ok &= create_objstack(&pinfo->fxstack, sizeof(fx_t));
    return ok;
  }
  return FALSE;
}

bool_t free_parserinfo_paste(parserinfo_paste_t *pinfo) {
  if( pinfo ) {
    free_nodereader_list(&pinfo->nrl);
    free_objstack(&pinfo->fxstack);
    return TRUE;
  }
  return FALSE;

}

bool_t prepare_files(parserinfo_paste_t *pinfo) {
  int nsi = 0;
  int f, x;
  fx_t fxn;
  if( pinfo ) {
    for(f = 0; f < pinfo->n; f++) {
      if( strcmp(pinfo->files[f], "stdin") == 0 ) {
	for(x = 0; pinfo->xpaths[f][x]; x++) {
	  nsi++;
	}
      } 
    }
    if( nsi > 1 ) {
      errormsg(E_FATAL, "only one (stdin,xpath) pair is supported.\n");
    }

    for(f = 0; f < pinfo->n; f++) {
      for(x = 0; pinfo->xpaths[f][x]; x++) {
	fxn.filename = pinfo->files[f];
	fxn.xpath = pinfo->xpaths[f][x];
	if( !push_objstack(&pinfo->fxstack, (byte_t *)&fxn, sizeof(fx_t)) ) {
	  errormsg(E_FATAL, "too many command line parameters\n");
	}
      }
    }
    return TRUE;
  }
  return FALSE;
}

/* returns TRUE when stopped and there is more data to process */
bool_t read_nodes(nodereader_t *nr) {
  if( nr ) {
    parser_t *parser = &nr->parser;
    stream_t *strm = &nr->strm;

    while( !checkflag(cmd,CMD_QUIT) ) {
      switch(nr->state) {
      case nr_nodata:
	nr->state = read_stream(strm, getbuf_parser(parser, strm->blksize), 
				strm->blksize) ? nr_ready : nr_endofile;
	break;
      case nr_ready:
	if( do_parser(parser, strm->buflen) ) {
	  nr->state = nr_nodata;
	} else {
	  stdparserinfo_t *std = &nr->std;
	  if( (std->depth == 0) && (std->maxdepth > 0) ) {
	    nr->state = nr_endofile;
	    return FALSE;
	  } 
	  if( suspended_parser(parser) ) {
	    nr->state = nr_stopped;
	    return TRUE;
	  }
	  nr->state = nr_dead;
	  errormsg(E_FATAL, 
		   "%s: %s at line %d, column %d, "
		   "byte %ld, depth %d\n",
		   nr->sav.name, error_message_parser(parser),
		   parser->cur.lineno, parser->cur.colno, 
		   parser->cur.byteno, std->depth);
	}
	break;
      case nr_stopped:
	restart_parser(parser);
	nr->state = nr_ready;
	break;
      case nr_dead:
      case nr_endofile:
	return FALSE;
      }
    }    
    nr->state = nr_endofile;
  }
  return FALSE;
}

bool_t paste_nodes(parserinfo_paste_t *pinfo, char *filename) {
  flag_t active = 0;
  int i, fd = -1;
  if( pinfo ) {

    if( strcmp(filename, "stdout") != 0 ) {
      fd = open_tempfile(filename); /* overwrites XXX in filename */
      if( fd < 0 ) {
	errormsg(E_FATAL, "couldn't open temporary file %s\n", filename);
      }
      open_redirect_stdout(fd);
    } else {
      open_stdout();
    }
    puts_stdout(get_headwrap());
    puts_stdout(get_open_root());
    putc_stdout('\n');

    active = (1 << pinfo->nrl.num) - 1;
    while( active ) {

      pinfo->tab = tab_start; /* lazy print <tab> */

      for(i = 0; !checkflag(cmd,CMD_QUIT) && (i < pinfo->nrl.num); i++) {
	if( active & (1<<i) ) {
	  nodereader_t *nr = &pinfo->nrl.list[i];
	  if( !read_nodes(nr) ) {
	    /* we're done with this file */
	    active &= ~(1<<i);
	  }
	  if( !is_empty_tempcollect(&nr->sav) ) {
	    if( pinfo->tab == tab_start ) {
	      write_start_tag_stdout(tabname, NULL, FALSE);
	      pinfo->tab = tab_end;
	    }
	    write_stdout_tempcollect(&nr->sav);
	    reset_tempcollect(&nr->sav);
	  }
	}
      }

      if( pinfo->tab == tab_end ) {
	write_end_tag_stdout(tabname);
	putc_stdout('\n');
      }
    }

    puts_stdout(get_close_root());
    puts_stdout(get_footwrap());
    close_stdout();
    if( fd != -1 ) {
      close(fd);
    }

    return TRUE;
  }
  return FALSE;
}

int main(int argc, char **argv) {
  signed char op;
  parserinfo_paste_t pinfo;
  filelist_t fl;
  int f;
  fx_t fxn;
  flag_t flags;

  struct option longopts[] = {
    { "version", 0, NULL, PASTE_VERSION },
    { "help", 0, NULL, PASTE_HELP },
    { 0 }
  };

  progname = "xml-paste";
  inputfile = "";
  inputline = 0;

  if( create_parserinfo_paste(&pinfo) ) {

    while( (op = getopt_long(argc, argv, "",
			     longopts, NULL)) > -1 ) {
      set_option_paste(op, optarg, &pinfo);
    }

    if( create_filelist(&fl, -1, argv + optind, FILELIST_MIN1) ) {

      pinfo.files = getfiles_filelist(&fl);
      pinfo.xpaths = getxpaths_filelist(&fl);
      pinfo.n = getsize_filelist(&fl);
    
      init_signal_handling(SIGNALS_DEFAULT);
      init_file_handling();
      init_tempfile_handling();

      if( prepare_files(&pinfo) ) {

	for(f = 0; f < pinfo.fxstack.top; f++) {
	  if( !peekn_objstack(&pinfo.fxstack, f, 
			      (byte_t *)&fxn, sizeof(fx_t)) ) {
	    errormsg(E_FATAL, "unexpected stack corruption\n");
	    break;
	  }
	  flags = (f >= pinfo.n) ? NODEREADER_TEMPORARY : 0;
	  if( !add_nodereader_list(&pinfo.nrl, 
				   fxn.filename, fxn.xpath, flags) ) {
	    fxn.filename = make_template_tempfile(progname);
	    fxn.xpath = tabpath;
	    if( !push_objstack(&pinfo.fxstack, (byte_t *)&fxn, sizeof(fx_t)) ) {
	      errormsg(E_FATAL, "stack push failure");
	    }
	    paste_nodes(&pinfo, (char *)fxn.filename); /* allow overwrite */
	    reset_nodereader_list(&pinfo.nrl);
	    f--; /* retry this path */
	  }
	}
	paste_nodes(&pinfo, "stdout");

      }

      exit_tempfile_handling();
      exit_file_handling();      
      exit_signal_handling();

      free_filelist(&fl);
    }

    free_parserinfo_paste(&pinfo);
  }

  return EXIT_SUCCESS;
}
