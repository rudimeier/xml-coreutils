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
#include "io.h"
#include "myerror.h"
#include "wrap.h"
#include "stdout.h"
#include "stdparse.h"
#include "smatch.h"
#include "tempvar.h"
#include "interval.h"
#include "stringlist.h"
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

/*
 * ALGORITHM OUTLINE
 *
 * The basic grepping algorithm is as follows (flags and command line
 * options can modify the behaviour slightly).
 *
 * At each epoch, the parser supplies us with a token which is either
 * 1) a start tag, 2) a closing tag, or 3) a chardata string. In each case,
 * the token is provisionally written for us on the tape (pinfo->tape).
 *
 * In case 1), we can accept (the tape contents) provisionally, or accept
 * it permanently (commit).
 * In case 2), we can accept provisionally, or roll back.
 * In case 3), we can commit or roll back.
 *
 * When the tape has been committed,it is written to the output,
 * and cleared. Thus everything that was on it is final.
 *
 * The decision to commit depends on whether a token matches the 
 * supplied grep pattern, and also on whether the current depth 
 * of the token is covered  by the "grep neighbourhood" (gnb).
 *
 * The gnb is a set of depths which is selected by command line options
 * and the nature of a string pattern match.
 */


#include <stdio.h>

/* EXIT_SUCCESS indicates match success,
   EXIT_FAILURE indicates match failure,
   EXIT_ERROR indicates error */
#define EXIT_ERROR 2

typedef enum { t_undefined, t_start_tag, t_end_tag, t_chardata } token_t;

typedef struct {
  token_t id;
  size_t pos;
  unsigned int ldepth;
} rollback_t;

typedef struct {
  stdparserinfo_t std; /* must be first so we can cast correctly */
  struct {
    token_t id;
    unsigned int ldepth; /* logical depth != std.depth */
    tempvar_t stringval; 
  } token;
  objstack_t savepoints; 
  smatcher_t sm;
  intervalmgr_t gnb; 
  tempcollect_t tape; 

  flag_t flags;
  const char_t *headwrap;
  const char_t *footwrap;
  const char_t *root;
} parserinfo_grep_t;

#define GREP_VERSION         0x01
#define GREP_HELP            0x02
#define GREP_INVERT          0x03
#define GREP_ICASE           0x04
#define GREP_EXTEND          0x05
#define GREP_SUBTREE         0x06
#define GREP_ATTRIBUTES      0x07
#define GREP_USAGE \
"Usage: xml-grep [OPTION] PATTERN [[FILE]... [:XPATH]...]...\n" \
"Print those XML nodes matching PATTERN in given FILE(s), or standard input.\n" \
"\n" \
"      --help     display this help and exit\n" \
"      --version  display version information and exit\n"

#define GREP_FLAG_CHARDATA   0x01
#define GREP_FLAG_NEWLINE    0x02
#define GREP_FLAG_ICASE      0x04
#define GREP_FLAG_INVERT     0x08
#define GREP_FLAG_EXTEND     0x10
#define GREP_FLAG_SUBTREE    0x20
#define GREP_FLAG_ATTRIBUTES 0x40
#define GREP_FLAG_MATCHFOUND 0x80

#define ACTION_NONE       0x00
#define ACTION_COMMIT     0x01
#define ACTION_ROLLBACK   0x02
#define ACTION_SAVEPOINT  0x04

void set_option_grep(int op, char *optarg, stringlist_t *patterns) {
  switch(op) {
  case GREP_VERSION:
    puts("xml-grep" COPYBLURB);
    exit(EXIT_SUCCESS);
    break;
  case GREP_HELP:
    puts(GREP_USAGE);
    exit(EXIT_SUCCESS);
    break;
  case 'v':
  case GREP_INVERT:
    setflag(&u_options, GREP_FLAG_INVERT);
    break;
  case 'e':
    add_unique_stringlist(patterns, optarg, STRINGLIST_DONTFREE);
    break;
  case 'i':
  case GREP_ICASE:
    setflag(&u_options, GREP_FLAG_ICASE);
    break;
  case 'E':
  case GREP_EXTEND:
    setflag(&u_options, GREP_FLAG_EXTEND);
    break;
  case GREP_SUBTREE:
    setflag(&u_options, GREP_FLAG_SUBTREE);
    break;
  case GREP_ATTRIBUTES:
    setflag(&u_options, GREP_FLAG_ATTRIBUTES);
    break;
  }
}

/* check if token.ldepth matches grep neighbourhood */
bool_t gnbmatch(parserinfo_grep_t *pinfo) {
  bool_t match = FALSE;
  int ga, gb;
  if( pinfo ) {
    match = memberof_intervalmgr(&pinfo->gnb, pinfo->token.ldepth);
    if( match ) {
      if( peek_intervalmgr(&pinfo->gnb, &ga, &gb) ) {
	if( pinfo->token.ldepth <= ga ) {
	  pop_intervalmgr(&pinfo->gnb);
	}
      }
    }

  }
  return match;
}

bool_t makegnb(parserinfo_grep_t *pinfo) {
  unsigned int a, b;
  if( pinfo ) {
    if( is_empty_intervalmgr(&pinfo->gnb) ) {
      switch(pinfo->token.id) {
      case t_start_tag:
	a = pinfo->std.depth - 1;
	if( checkflag(pinfo->flags,GREP_FLAG_SUBTREE) ) {
	  b = INFDEPTH;
	} else {
	  b = pinfo->token.ldepth;
	}
	break;
      case t_chardata:
	/* nothing */
      default:
	/* these can't happen */
	return TRUE;
      }

      push_intervalmgr(&pinfo->gnb, a, b);
      return TRUE;
    }
  }
  return FALSE;
}

/* check if token.stringval matches grep pattern */
bool_t grepmatch(parserinfo_grep_t *pinfo) {
  const char_t *s;
  bool_t match = FALSE;
  size_t n;
  if( pinfo ) {

    if( !is_empty_tempvar(&pinfo->token.stringval) &&
	peeks_tempvar(&pinfo->token.stringval, 0, &s) ) {

      /* do actual string grepping */
      n = -1;
      if( find_next_smatcher(&pinfo->sm, s, &n) ) {
	match = TRUE;
      }

      if( match ) makegnb(pinfo);

    }

  }
  return match;
}

bool_t spacematch(parserinfo_grep_t *pinfo) {
  const char_t *s;
  if( pinfo ) {
    if( !is_empty_tempvar(&pinfo->token.stringval) &&
	peeks_tempvar(&pinfo->token.stringval, 0, &s) ) {
      
      return is_xml_space(s, s + strlen(s));
    }
  }
  return FALSE;
}


/* void printsav(parserinfo_grep_t *pinfo) { */
/*     puts_stdout("TAPE["); */
/*     write_stdout_tempcollect(&pinfo->tape); */
/*     puts_stdout("]\n"); */
/* } */

token_t peek_id_savepoint(parserinfo_grep_t *pinfo) {
  rollback_t rb;
  return ( (pinfo && 
	    peek_objstack(&pinfo->savepoints, 
			  (byte_t *)&rb, sizeof(rollback_t))) ?
	   rb.id : t_undefined );
}

bool_t savepoint(parserinfo_grep_t *pinfo) {
  rollback_t rb;
  if( pinfo ) {
    rb.pos = tell_tempcollect(&pinfo->tape);
    rb.ldepth = pinfo->token.ldepth;
    rb.id = pinfo->token.id;
    if( !push_objstack(&pinfo->savepoints, 
		       (byte_t *)&rb, sizeof(rollback_t)) ) {
      errormsg(E_FATAL, "stack failure.\n");
    }
    return TRUE;
  }
  return FALSE;
}

bool_t rollback(parserinfo_grep_t *pinfo) {
  rollback_t rb;
  if( pinfo ) {
    if( peek_objstack(&pinfo->savepoints, 
		      (byte_t *)&rb, sizeof(rollback_t)) ) {
      if( pop_objstack(&pinfo->savepoints, sizeof(rollback_t)) ) {
	truncate_tempcollect(&pinfo->tape, rb.pos);
	return TRUE;
      }
    }
  }
  return FALSE;
}

bool_t commit(parserinfo_grep_t *pinfo) {
  if( pinfo ) {
    write_stdout_tempcollect(&pinfo->tape);
    reset_tempcollect(&pinfo->tape);
    clear_objstack(&pinfo->savepoints);
    setflag(&pinfo->flags, GREP_FLAG_MATCHFOUND);
    return TRUE;
  }
  return FALSE;
}


bool_t next_epoch_normal(parserinfo_grep_t *pinfo) {
  bool_t match;
  token_t tok;
  if( pinfo ) {
    match = gnbmatch(pinfo) || grepmatch(pinfo);
    switch(pinfo->token.id) {
    case t_chardata:
      if( spacematch(pinfo) ) {
	/* combine this space with the previous token */
	/* we could instead roll it back, but this looks better (YMMV) */
	pop_objstack(&pinfo->savepoints, sizeof(rollback_t));
	break;
      }
      if( match ) {
	commit(pinfo);
      } else {
	rollback(pinfo);
      }
      break;
    case t_start_tag:
      if( match ) {
	commit(pinfo);
      }
      break;
    case t_end_tag:
      pop_objstack(&pinfo->savepoints, sizeof(rollback_t)); /* our end tag */
      tok = peek_id_savepoint(pinfo);
      if( tok == t_start_tag ) {
	rollback(pinfo);
      }
      break;
    default:
      break;
    }

    /* don't need this anymore */
    reset_tempvar(&pinfo->token.stringval);

    return TRUE;
  }
  return FALSE;
}

/* this is not a simple negation of next_epoch_normal() */
bool_t next_epoch_inverted(parserinfo_grep_t *pinfo) {
  bool_t match;
  token_t tok;
  if( pinfo ) {
    switch(pinfo->token.id) {
    case t_chardata:
      if( spacematch(pinfo) ) {
	/* combine this space with the previous token */
	/* we could instead roll it back, but this looks better (YMMV) */
	pop_objstack(&pinfo->savepoints, sizeof(rollback_t));
	break;
      }
      match = grepmatch(pinfo);
      if( match ) {
      	rollback(pinfo);
      } else {
	if( checkflag(pinfo->flags,GREP_FLAG_SUBTREE) &&
	       gnbmatch(pinfo) ) {
	  rollback(pinfo);
	} else {
	  commit(pinfo);
	}
      }
      break;
    case t_start_tag:
      grepmatch(pinfo); /* builds gnb */
      break;
    case t_end_tag:
      gnbmatch(pinfo); /* clears gnb if necessary */
      pop_objstack(&pinfo->savepoints, sizeof(rollback_t)); /* our end tag */
      tok = peek_id_savepoint(pinfo);
      if( tok == t_start_tag ) {
	rollback(pinfo);
      }
      break;
    default:
      break;
    }

    /* don't need this anymore */
    reset_tempvar(&pinfo->token.stringval);
    return TRUE;
  }
  return FALSE;
}

bool_t next_epoch(parserinfo_grep_t *pinfo) {
  return ( checkflag(pinfo->flags,GREP_FLAG_INVERT) ?
	   next_epoch_inverted(pinfo) : next_epoch_normal(pinfo) );
}


/* remove attributes which do not match the pattern */
bool_t filter_attributes(parserinfo_grep_t *pinfo, const char_t **att) {
  bool_t match;
  const char_t **next;
  if( pinfo ) {
    next = att;
    while( att && *att ) {
      puts_tempvar(&pinfo->token.stringval, att[1]);
      match = grepmatch(pinfo);
      match = checkflag(pinfo->flags,GREP_FLAG_INVERT) ? !match : match;
      if( match ) {
	if( next < att ) {
	  next[0] = att[0];
	  next[1] = att[1];
	}
	next += 2;
      }
      att += 2;
    }
    if( next < att ) {
      next[0] = NULL;
      next[1] = NULL;
    }
    return TRUE;
  }
  return FALSE;
}


bool_t stringval_from_attributes(parserinfo_grep_t *pinfo, const char_t **att) {
  if( pinfo ) {
    reset_tempvar(&pinfo->token.stringval);
    while( att && *att ) {
      puts_tempvar(&pinfo->token.stringval, att[1]);
      putc_tempvar(&pinfo->token.stringval, ' ');
      att += 2;
    }
    return TRUE;
  }
  return FALSE;
}

result_t start_tag(void *user, const char_t *name, const char_t **att) {
  parserinfo_grep_t *pinfo = (parserinfo_grep_t *)user;
  if( pinfo && (pinfo->std.depth > 1) ) { 

    if( true_and_clearflag(&pinfo->flags,GREP_FLAG_CHARDATA) ) {
      next_epoch(pinfo);
    }

    if( checkflag(pinfo->flags,GREP_FLAG_ATTRIBUTES) ) {
      filter_attributes(pinfo, att);
    }

    pinfo->token.id = t_start_tag;
    pinfo->token.ldepth = pinfo->std.depth; 
    stringval_from_attributes(pinfo, att); /* stringval */
    savepoint(pinfo);
    write_start_tag_tempcollect(&pinfo->tape, name, att);

    next_epoch(pinfo);

  }
  return PARSER_OK;
}

result_t attribute(void *user, const char_t *name, const char_t *value) {
  parserinfo_grep_t *pinfo = (parserinfo_grep_t *)user;
  if( pinfo ) {
    /* to be implemented. */
  }
  return PARSER_OK;
}


result_t end_tag(void *user, const char_t *name) {
  parserinfo_grep_t *pinfo = (parserinfo_grep_t *)user;
  if( pinfo && (pinfo->std.depth > 1) ) { 

    if( true_and_clearflag(&pinfo->flags,GREP_FLAG_CHARDATA) ) {
      next_epoch(pinfo);
    }

    pinfo->token.id = t_end_tag;
    pinfo->token.ldepth = pinfo->std.depth - 1;
    /* no stringval */
    savepoint(pinfo);

    write_end_tag_tempcollect(&pinfo->tape, name);

    next_epoch(pinfo);
  }
  return PARSER_OK;
}

result_t chardata(void *user, const char_t *buf, size_t buflen) {
  parserinfo_grep_t *pinfo = (parserinfo_grep_t *)user;
  if( pinfo ) {

    if( (pinfo->std.depth > 0) ) { 
      pinfo->token.id = t_chardata;
      pinfo->token.ldepth = pinfo->std.depth; 
      write_tempvar(&pinfo->token.stringval, (byte_t *)buf, buflen);

      if( false_and_setflag(&pinfo->flags,GREP_FLAG_CHARDATA) ) {
	savepoint(pinfo);
      }
      write_coded_entities_tempcollect(&pinfo->tape, buf, buflen);
    }
  }
  return PARSER_OK;
}

result_t start_cdata(void *user) {
  parserinfo_grep_t *pinfo = (parserinfo_grep_t *)user;
  if( pinfo ) { 
    puts_tempcollect(&pinfo->tape, "<![CDATA[");
  }
  return PARSER_OK;
}

result_t end_cdata(void *user) {
  parserinfo_grep_t *pinfo = (parserinfo_grep_t *)user;
  if( pinfo ) { 
    puts_tempcollect(&pinfo->tape, "]]>");
  }
  return PARSER_OK;
}

result_t dfault(void *user, const char_t *data, size_t buflen) {
  parserinfo_grep_t *pinfo = (parserinfo_grep_t *)user;
  if( pinfo && (pinfo->std.depth > 0) ) { 
    write_tempcollect(&pinfo->tape, (byte_t *)data, buflen);
  }
  return PARSER_OK;
}


bool_t create_parserinfo_grep(parserinfo_grep_t *pinfo) {
  bool_t ok = TRUE;
  if( pinfo ) {
    memset(pinfo, 0, sizeof(parserinfo_grep_t));

    ok &= create_stdparserinfo(&pinfo->std);

    ok &= create_smatcher(&pinfo->sm);
    ok &= create_intervalmgr(&pinfo->gnb);

    /* sav contains printable text that hasn't been printed yet */
    ok &= create_tempcollect(&pinfo->tape, "sav", MINVARSIZE, MAXVARSIZE);
    /* grep contains the extracted text that will be searched */
    /* must define grep as a tempvar_t because the 
       regexec() calls need the whole string in memory */
    ok &= create_tempvar(&pinfo->token.stringval, "grep", 
			 MINVARSIZE, MAXVARSIZE);
    pinfo->token.ldepth = 0;
    pinfo->token.id = t_undefined;

    ok &= create_objstack(&pinfo->savepoints, sizeof(rollback_t));


    if( ok ) {

      pinfo->std.setup.flags = STDPARSE_MIN1FILE;
      pinfo->std.setup.cb.start_tag = start_tag;
      pinfo->std.setup.cb.end_tag = end_tag;
      pinfo->std.setup.cb.chardata = chardata;
      /* pinfo->std.setup.cb.attribute = attribute; */
      pinfo->std.setup.cb.start_cdata = start_cdata;
      pinfo->std.setup.cb.end_cdata = end_cdata;
      pinfo->std.setup.cb.dfault = dfault;
      pinfo->flags = u_options;

    }
    return ok;
  }
  return FALSE;
}

bool_t free_parserinfo_grep(parserinfo_grep_t *pinfo) {
  if( pinfo ) {
    free_stdparserinfo(&pinfo->std);
    free_smatcher(&pinfo->sm);
    free_intervalmgr(&pinfo->gnb);
    free_tempcollect(&pinfo->tape);
    free_tempvar(&pinfo->token.stringval);
    free_objstack(&pinfo->savepoints);
    return TRUE;
  }
  return FALSE;

}


void output_wrapper_start(parserinfo_grep_t *pinfo) {
  open_stdout();
  if( pinfo ) {
    puts_tempcollect(&pinfo->tape, get_headwrap());
    write_start_tag_tempcollect(&pinfo->tape, get_root_tag(), NULL);
    savepoint(pinfo);
  }
}

void output_wrapper_end(parserinfo_grep_t *pinfo) {
  if( pinfo ) {
    write_end_tag_tempcollect(&pinfo->tape, get_root_tag());
    puts_tempcollect(&pinfo->tape, get_footwrap());

    write_stdout_tempcollect(&pinfo->tape);
  }
  close_stdout();
}

int main(int argc, char **argv) {
  signed char op;
  parserinfo_grep_t pinfo;
  int retval = EXIT_FAILURE;
  flag_t sflags;
  stringlist_t patterns;
  int i;

  struct option longopts[] = {
    { "version", 0, NULL, GREP_VERSION },
    { "help", 0, NULL, GREP_HELP },
    { "invert-match", 0, NULL, GREP_INVERT },
    { "ignore-case", 0, NULL, GREP_ICASE },
    { "extended-regexp", 0, NULL, GREP_EXTEND },
    { "subtree", 0, NULL, GREP_SUBTREE },
    { "attributes", 0, NULL, GREP_ATTRIBUTES },
    { 0 }
  };


  progname = "xml-grep";
  inputfile = "";
  inputline = 0;

  create_stringlist(&patterns);

  while( (op = getopt_long(argc, argv, "viEe:",
			   longopts, NULL)) > -1 ) {
    set_option_grep(op, optarg, &patterns);
  }

  init_signal_handling(SIGNALS_DEFAULT);
  init_file_handling();

  if( create_parserinfo_grep(&pinfo) ) {
    if( (argc < optind) || 
	((patterns.num == 0) && !argv[optind]) ) {

      puts(GREP_USAGE);
      retval = EXIT_ERROR;

    } else {

      if( patterns.num == 0 ) {
	add_stringlist(&patterns, argv[optind], STRINGLIST_DONTFREE);
	optind++;
      }

      sflags = 
	(checkflag(pinfo.flags,GREP_FLAG_ICASE) ? SMATCH_FLAG_ICASE : 0)|
	(checkflag(pinfo.flags,GREP_FLAG_EXTEND) ? SMATCH_FLAG_EXTEND : 0);

      for(i = 0; i < patterns.num; i++) {
	if( !push_smatcher(&pinfo.sm, 
			   get_stringlist(&patterns, i), sflags) ) {
	  retval = EXIT_ERROR;
	}
      }

      output_wrapper_start(&pinfo);

      stdparse(MAXFILES, argv + optind, (stdparserinfo_t *)&pinfo);
      if( checkflag(pinfo.flags, GREP_FLAG_MATCHFOUND) ) {
	retval = EXIT_SUCCESS;
      }

      output_wrapper_end(&pinfo);

    }

    free_parserinfo_grep(&pinfo);
  }

  exit_file_handling();
  exit_signal_handling();

  free_stringlist(&patterns);

  return retval;
}
