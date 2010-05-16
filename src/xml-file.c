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

#include "common.h"
#include "parser.h"
#include "io.h"
#include "myerror.h"
#include "wrap.h"
#include "stdout.h"
#include "stdparse.h"
#include "entities.h"
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
flag_t u_options = 0;

#include <stdio.h>

typedef struct {
  char_t *file;
  char_t *doctype;
  char_t *sysid;
  char_t *pubid;
  char_t *tag;
} features_t;

typedef struct {
  stdparserinfo_t std; /* must be first so we can cast correctly */
  features_t feats;
  flag_t flags;
  int numfiles;
  int llf;
} parserinfo_file_t;

#define FILE_VERSION    0x01
#define FILE_HELP       0x02
#define FILE_SHOW       0x03
#define FILE_USAGE \
"Usage: xml-file [OPTION]... FILE [FILE]...\n" \
"Determine type of FILE(s).\n" \
"\n" \
"      --help     display this help and exit\n" \
"      --version  display version information and exit\n"

#define FILE_FLAG_SHOW 0x01

typedef enum { m_nothing = 0, m_doctype, m_sysid, m_pubid, m_tag } mtype_t;
typedef enum { m_done, m_and, m_or } mnext_t;
typedef struct {
  mtype_t mtype;
  mnext_t next;
  const char_t *mvalue;
  const char_t *ident;
} magic_t;

/* list of possible document types, scanned from top to bottom until
   first match. Eventually, this will have to be made efficient, and
   read/written from/to an external magic file, eg magic(5) */ 
static const magic_t magic[] = {
  { m_doctype, m_done, "html", "HTML text document" },
  { m_tag, m_done, "html", "HTML text fragment" },
  { m_tag, m_done, "rdf:RDF", "RDF text fragment" },
  { m_tag, m_done, "rss", "RSS 2.0 text document" },
  { m_doctype, m_done, "svg", "SVG text document" },
  { m_tag, m_done, "svg", "SVG text fragment" },
  { m_tag, m_done, "math", "MathML text fragment" },
  { m_tag, m_done, "mrow", "MathML text fragment" },
  { 0 }
};

bool_t cmp_magic(features_t *f, const magic_t *m) {
  if( f && m ) {
    switch(m->mtype) {
    case m_nothing:
      return FALSE;
    case m_doctype: 
      return (f->doctype && (strcasecmp(f->doctype, m->mvalue) == 0));
    case m_sysid:
      return (f->sysid && (strcasecmp(f->sysid, m->mvalue) == 0));
    case m_pubid:
      return (f->pubid && (strcasecmp(f->pubid, m->mvalue) == 0));
    case m_tag:
      return (f->tag && (strcasecmp(f->tag, m->mvalue) == 0));
    }
  }
  return FALSE;
}

void set_option_file(int op, char *optarg) {
  switch(op) {
  case FILE_VERSION:
    puts("xml-file" COPYBLURB);
    exit(EXIT_SUCCESS);
    break;
  case FILE_HELP:
    puts(FILE_USAGE);
    exit(EXIT_SUCCESS);
    break;
  case FILE_SHOW:
    u_options |= FILE_FLAG_SHOW;
    break;
  }
}

result_t start_doctypedecl(void *user, const char_t *name, const char_t *sysid, const char_t *pubid, bool_t intsub) {
 parserinfo_file_t *pinfo = (parserinfo_file_t *)user;
  if( pinfo ) { 
    pinfo->feats.doctype = name ? strdup(name) : NULL;
    pinfo->feats.sysid = sysid ? strdup(sysid) : NULL;
    pinfo->feats.pubid = pubid ? strdup(pubid) : NULL;
  }
  return PARSER_OK;
}

result_t end_doctypedecl(void *user) {
parserinfo_file_t *pinfo = (parserinfo_file_t *)user;
  if( pinfo ) { 
  }
  return PARSER_OK;
}

result_t start_tag(void *user, const char_t *name, const char_t **att) {
  parserinfo_file_t *pinfo = (parserinfo_file_t *)user;
  if( pinfo ) { 
    pinfo->feats.tag = name ? strdup(name) : NULL;
  }
  return PARSER_ABORT; /* we only look at a single tag */
}

bool_t create_features(features_t *feats) {
  if( feats ) {
    memset(feats, 0, sizeof(features_t));
    return TRUE;
  }
  return FALSE;
}

bool_t free_features(features_t *feats) {
  if( feats ) {
    if( feats->file ) free(feats->file);
    if( feats->tag ) free(feats->tag);
    if( feats->doctype ) free(feats->doctype);
    if( feats->sysid ) free(feats->sysid);
    if( feats->pubid ) free(feats->pubid);
    return TRUE;
  }
  return FALSE;
}

void reset_features(features_t *feats, const char_t *file) {
  if( feats ) {
    free_features(feats);
    memset(feats, 0, sizeof(features_t));
    feats->file = file ? strdup(file) : NULL;
  }
}

const char_t *ident_string(features_t *feats) {
  const magic_t *m;
  for(m = magic; m->mtype != m_nothing; m++) {
    if( cmp_magic(feats, m) ) {
      return m->ident;
    }
  }
  return "XML text";
}

/* mimic file(1) with keywords "text", "data" */
void identify_from_features(parserinfo_file_t *pinfo) {
  const char_t *id = "unrecognized data";
  if( !stdparse_failed(&pinfo->std) ) {
    id = ident_string(&pinfo->feats);
  }
  puts_stdout(pinfo->feats.file);
  putc_stdout(':');
  nputc_stdout(' ', pinfo->llf + 1 - strlen(pinfo->feats.file));
  puts_stdout(id);
  putc_stdout('\n');
  flush_stdout();
}

void show_features(parserinfo_file_t *pinfo) {
  if( pinfo ) {
    puts_stdout("[file] "); puts_stdout(pinfo->feats.file);
    puts_stdout("[doctype] "); puts_stdout(pinfo->feats.doctype);
    puts_stdout("[sysid] "); puts_stdout(pinfo->feats.sysid);
    puts_stdout("[pubid] "); puts_stdout(pinfo->feats.pubid);
    puts_stdout("[tag] "); puts_stdout(pinfo->feats.tag);
  }
}

bool_t start_file_fun(void *user, const char_t *file, const char_t **xpaths) {
  parserinfo_file_t *pinfo = (parserinfo_file_t *)user;
  if( pinfo ) {
    reset_features(&pinfo->feats, file);
  }
  return TRUE;
}

bool_t end_file_fun(void *user, const char_t *file, const char_t **xpaths) {
  parserinfo_file_t *pinfo = (parserinfo_file_t *)user;
  if( pinfo ) {
    if( checkflag(u_options, FILE_FLAG_SHOW) ) {
      show_features(pinfo);
    }
    identify_from_features(pinfo);
  }
  return TRUE;
}

bool_t create_parserinfo_file(parserinfo_file_t *pinfo) {
  bool_t ok = TRUE;
  if( pinfo ) {

    memset(pinfo, 0, sizeof(parserinfo_file_t));
    ok &= create_stdparserinfo(&pinfo->std);
    ok &= create_features(&pinfo->feats);

    pinfo->std.setup.flags = 
      STDPARSE_MIN1FILE|STDPARSE_NOXPATHS|STDPARSE_QUIET;
    pinfo->std.setup.cb.start_tag = start_tag;
    pinfo->std.setup.cb.start_doctypedecl = start_doctypedecl;
    pinfo->std.setup.cb.end_doctypedecl = end_doctypedecl;

    pinfo->std.setup.start_file_fun = start_file_fun;
    pinfo->std.setup.end_file_fun = end_file_fun;

    return ok;
  }
  return FALSE;
}

bool_t free_parserinfo_file(parserinfo_file_t *pinfo) {
  free_features(&pinfo->feats);
  free_stdparserinfo(&pinfo->std);
  return TRUE;
}

int length_longest_filename(char **argv) {
  int n;
  for(n = 0; argv && *argv; argv++ ) {
    n = MAX(n, strlen(*argv));
  }
  return n;
}

int main(int argc, char **argv) {
  signed char op;
  parserinfo_file_t pinfo;

  struct option longopts[] = {
    { "version", 0, NULL, FILE_VERSION },
    { "help", 0, NULL, FILE_HELP },
    { "show-everything", 0, NULL, FILE_SHOW },
    { 0 }
  };

  progname = "xml-file";
  inputfile = "";
  inputline = 0;

  if( create_parserinfo_file(&pinfo) ) {

    while( (op = getopt_long(argc, argv, "",
			     longopts, NULL)) > -1 ) {
      set_option_file(op, optarg);
    }

    if( !argv[optind] ) {
      puts(FILE_USAGE);
      exit(EXIT_SUCCESS);
    }

    pinfo.llf = length_longest_filename(argv + optind);

    init_signal_handling(SIGNALS_DEFAULT);
    init_file_handling();

    open_stdout();

    stdparse(MAXFILES, argv + optind, (stdparserinfo_t *)&pinfo);

    close_stdout();

    exit_file_handling();
    exit_signal_handling();

    free_parserinfo_file(&pinfo);
  }
  return EXIT_SUCCESS;
}
