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
#include "io.h"
#include "stdout.h"
#include "filelist.h"
#include "mysignal.h"
#include "myerror.h"
#include "entities.h"
#include "cstring.h"
#include "xpath.h"
#include "wrap.h"
#include "objstack.h"
#include "stringlist.h"
#include "mem.h"
#include "htfilter.h"

#include <string.h>
#include <getopt.h>
#include <stdarg.h>

/* FIRST READ THE COMMENT ABOVE filter_buffer() */

/* for option processing */
extern char *optarg;
extern int optind, opterr, optopt;

extern char *progname;
extern char *inputfile;
extern long inputline;

extern int cmd;

extern const char_t escc;
extern const char_t xpath_delims[];

flag_t u_options = 0;

#include <stdio.h>

/* see xml 1.0 specification (5th ed) */
typedef enum { 
  f_xmlstart=0, f_xmlliteral, f_xmlskip, f_xmlmultiplex,
  f_xmlstring, f_xmlquotedstring, f_xmlremexcl,
  f_document, f_chardata, f_mainloop,
  f_PIOrXMLDecl, f_PI, f_PITarget,
  f_S, f_So, f_Eq, f_Tag, f_STag, f_ETag,
  f_Name, f_NameStartChar, f_NameChar, f_Attribute, 
  f_AttValue, f_SystemLiteral, f_PubidLiteral,
  f_Reference, f_CharRef, f_xmldigit, f_xmlxdigit, f_PEReference,
  f_EntityValue, f_Meta, f_Meta1, f_Meta2,
  f_Comment,
  f_CDSect,
  f_XMLDecl, f_VersionInfo, f_EncodingDecl, f_SDDecl, 
  f_doctypedecl, f_ExternalID, f_SystemID, f_PublicID1, f_PublicID2, 
  f_intSubset, 
  f_ElementDeclOrEntityDecl, f_ElementDecl, 
  f_EntityDecl, f_PEDecl, f_GEDecl, f_PEDef, f_EntityDef, 
  f_AttlistDecl, f_AttDef, f_AttType, f_NotationType,
  f_Enumeration, f_Nmtoken, f_DefaultDecl, f_NotationDecl, f_contentspec,
  f_Mixed, f_children, f_choiceOrseq, f_choice, f_seq, f_cp,
  f_xmlend,
  f_xmlerror
} pstate_t;

typedef enum {
  e_unrecoverable= 0,
  e_missing_root,
  e_missing_eq,
  e_malformed_tag,
  e_unexpected_literal,
  e_unexpected_name,
  e_unhandled_multiplex,
  e_ref_required,
  e_nmtoken,
  e_choice
} errorcode_t;

typedef struct {
  pstate_t pst;
  int pos;
} cstate_t;

typedef struct {
  const char_t *s;
  int pos;
} literal_t;

typedef struct {
  const char_t *accept;
  const char_t *reject;
} skip_t;

typedef struct {
  const char_t *delim;
  const char_t *expand;
  const char_t *bailout;
} string_t;

typedef struct {
  const char_t *accept;
  pstate_t pst;
  int pos;
} multiplex_t;

typedef struct {
#define MAXMULTI 10
    multiplex_t dest[MAXMULTI];
    int n;
} multi_t;

typedef struct {
  cstate_t state;
  const byte_t *begin, *end, *peg;

  literal_t lit;
  skip_t skip;
  string_t str;
  multi_t multi;
  
  cstring_t sbuf;
  const char_t *sbuf_ptr;

  stringlist_t entities;
  stringlist_t attributes;

  xpath_t xpath;
  objstack_t stack;
  htfilter_t out;
  
} fixtagsinfo_t;

#define FIXTAGS_VERSION    0x01
#define FIXTAGS_HELP       0x02
#define FIXTAGS_WRAP       0x03
#define FIXTAGS_HTML       0x04
#define FIXTAGS_XML        0x05
#define FIXTAGS_USAGE0 \
"Usage: xml-fixtags [OPTION]... [FILE]\n" \
"Aggressively fix tags and entities in FILE or standard input,\n" \
"printing a well formed XML document on standard output.\n" \
"\n" \
"      --help    display this help and exit\n" \
"      --version display version information and exit\n"

#define FIXTAGS_FLAG_WRAP   0x01
#define FIXTAGS_FLAG_HTML   0x02
#define FIXTAGS_FLAG_XML    0x04

int set_option(int op, char *optarg) {
  int c = 0;
  switch(op) {
  case FIXTAGS_VERSION:
    puts("xml-fixtags" COPYBLURB);
    exit(EXIT_SUCCESS);
    break;
  case FIXTAGS_HELP:
    puts(FIXTAGS_USAGE0);
    exit(EXIT_SUCCESS);
    break;
  case FIXTAGS_WRAP:
    setflag(&u_options,FIXTAGS_FLAG_WRAP);
    break;
  case FIXTAGS_HTML:
    setflag(&u_options,FIXTAGS_FLAG_HTML);
    break;
  case FIXTAGS_XML:
    setflag(&u_options,FIXTAGS_FLAG_XML);
    break;
  }
  return c;
}

typedef struct {
  byte_t *buf;
  size_t size;
} ffbuf_t;

bool_t read_ffbuf(ffbuf_t *ff, stream_t *strm) {
  bool_t ok;
  ok = read_stream(strm, ff->buf, strm->blksize);
  ff->size = strm->buflen;
  return ok;
}


/* debug */
const char *sstate(pstate_t s) {
  switch(s) {
  case f_xmlstart: return "f_xmlstart";
  case f_xmlliteral: return "f_xmlliteral";
  case f_xmlskip: return "f_xmlskip";
  case f_xmlstring: return "f_xmlstring";
  case f_xmlquotedstring: return "f_xmlquotedstring";
  case f_xmlremexcl: return "f_xmlremexcl";
  case f_xmlmultiplex: return "f_xmlmultiplex";
  case f_document: return "f_document";
  case f_chardata: return "f_chardata";
  case f_mainloop: return "f_mainloop";
  case f_PIOrXMLDecl: return "f_PIOrXMLDecl";
  case f_PI: return "f_PI";
  case f_PITarget: return "f_PITarget";
  case f_S: return "f_S";
  case f_So: return "f_So";
  case f_Eq: return "f_Eq";
  case f_Tag: return "f_Tag";
  case f_STag: return "f_STag";
  case f_Name: return "f_Name";
  case f_NameStartChar: return "f_NameStartChar";
  case f_NameChar: return "f_NameChar";
  case f_Attribute: return "f_Attribute";
  case f_AttValue: return "f_AttValue";
  case f_SystemLiteral: return "f_SystemLiteral";
  case f_PubidLiteral: return "f_PubidLiteral";
  case f_ETag: return "f_ETag";
  case f_Reference: return "f_Reference";
  case f_CharRef: return "f_CharRef";
  case f_xmldigit: return "f_xmldigit";
  case f_xmlxdigit: return "f_xmlxdigit";
  case f_PEReference: return "f_PEReference";
  case f_EntityValue: return "f_EntityValue";
  case f_Meta: return "f_Meta";
  case f_Meta1: return "f_Meta1";
  case f_Meta2: return "f_Meta2";
  case f_Comment: return "f_Comment";
  case f_CDSect: return "f_CDSect";
  case f_XMLDecl: return "f_XMLDecl";
  case f_VersionInfo: return "f_VersionInfo";
  case f_EncodingDecl: return "f_EncodingDecl";
  case f_SDDecl: return "f_SDDecl";
  case f_doctypedecl: return "f_doctypedecl";
  case f_ExternalID: return "f_ExternalID";
  case f_SystemID: return "f_SystemID";
  case f_PublicID1: return "f_PublicID1";
  case f_PublicID2: return "f_PublicID2";
  case f_intSubset: return "f_intSubset";
  case f_ElementDeclOrEntityDecl: return "f_ElementDeclOrEntityDecl";
  case f_ElementDecl: return "f_ElementDecl";
  case f_PEDecl: return "f_PEDecl";
  case f_GEDecl: return "f_GEDecl";
  case f_EntityDecl: return "f_EntityDecl";
  case f_PEDef: return "f_PEDef";
  case f_EntityDef: return "f_EntityDef";
  case f_AttlistDecl: return "f_AttlistDecl";
  case f_AttDef: return "f_AttDef";
  case f_AttType: return "f_AttType";
  case f_NotationType: return "f_NotationType";
  case f_Enumeration: return "f_Enumeration";
  case f_Nmtoken: return "f_Nmtoken";
  case f_DefaultDecl: return "f_DefaultDecl";
  case f_NotationDecl: return "f_NotationDecl";
  case f_contentspec: return "f_contentspec";
  case f_Mixed: return "f_Mixed";
  case f_children: return "f_children";
  case f_choiceOrseq: return "f_choiceOrseq";
  case f_choice: return "f_choice";
  case f_seq: return "f_seq";
  case f_cp: return "f_cp";
  case f_xmlend: return "f_xmlend";
  case f_xmlerror: return "f_xmlerror";
  }
  return NULL;
}

void show_state(fixtagsinfo_t *fti) {
  switch(fti->state.pst) {
  case f_xmlstring:
    printf("[%.10s|%s delim=%s expand=%s bailout=%s\n",
	   fti->begin, sstate(fti->state.pst), 
	   fti->str.delim, fti->str.expand, fti->str.bailout); 
    break;
  case f_xmlliteral:
    printf("[%.10s|%s lit=%s\n",
	   fti->begin, sstate(fti->state.pst), 
	   fti->lit.s); 
    break;
  case f_Name:
  case f_NameStartChar:
  case f_NameChar:
    printf("[%.10s|%s sbuf=[%s]\n",
	   fti->begin, sstate(fti->state.pst), 
	   begin_cstring(&fti->sbuf)); 
    break;
  default:
    printf("[%.10s|%s:%d|%s]\n", 
	   fti->begin, sstate(fti->state.pst), fti->state.pos, 
	   string_xpath(&fti->xpath));
    break;
  }
}

void f_replace(fixtagsinfo_t *fti, char_t c) {
  *((char_t *)fti->begin) = c;
}
/* call a function starting at a specified position */
void f_call_at(fixtagsinfo_t *fti, pstate_t s, int pos) {
  push_objstack(&fti->stack, (byte_t *)&fti->state, sizeof(cstate_t));
  fti->state.pst = s;
  fti->state.pos = pos;
}

/* call a function starting at the beginning */
void f_call(fixtagsinfo_t *fti, pstate_t s) {
  f_call_at(fti, s, 0);
}

/* goto */
void f_next_at(fixtagsinfo_t *fti, int pos) {
  fti->state.pos = pos;
}

/* increment position pointer */
void f_next(fixtagsinfo_t *fti) {
  fti->state.pos++;
}

/* return from function call */
void f_done(fixtagsinfo_t *fti) {
  peek_objstack(&fti->stack, (byte_t *)&fti->state, sizeof(cstate_t));
  pop_objstack(&fti->stack, sizeof(cstate_t));
}

/* parse a string literal, stops after the last character of s */
void f_literal(fixtagsinfo_t *fti, const char_t *s) {
  if( s ) {
    fti->lit.s = s;
    fti->lit.pos = 0;
    f_call(fti, f_xmlliteral);
  }
}

/* ignore some characters until the current char matches accept or reject */
void f_skip(fixtagsinfo_t *fti, const char_t *accept, const char_t *reject) {
  fti->skip.accept = accept;
  fti->skip.reject = reject;
  f_call(fti, f_xmlskip);
}

void f_quotedstring(fixtagsinfo_t *fti, const char_t *expand) {
  if( !fti->peg ) {
    /* this causes problems with references in current implementation */
    errormsg(E_FATAL, "sbuf corruption.\n"); 
  }
  fti->str.expand = expand;
  f_call(fti, f_xmlquotedstring);
}

/* if current char is in accept, goto apos, otherwise goto rpos */
void f_test(fixtagsinfo_t *fti, const char_t *accept, int apos, int rpos) {
  fti->state.pos =
    ( accept && strchr(accept, *fti->begin) ) ? apos : rpos;
}

/* if char is first char of lit (parse lit, then goto apos) else goto rpos */
void f_test_literal(fixtagsinfo_t *fti, const char_t *lit, int apos, int rpos) {
  if( *lit == *fti->begin ) {
    fti->state.pos = apos;
    f_literal(fti, lit);
  } else {
    fti->state.pos = rpos;
  }
}

/* if char is in accept, call function s starting at pos */
void f_test_call_at(fixtagsinfo_t *fti, const char_t *accept, pstate_t s, int pos) {
  if( accept && strchr(accept, *fti->begin) ) {
    f_call_at(fti, s, pos);
  }
}

/* if char is in accept, call function s starting at beginning */
void f_test_call(fixtagsinfo_t *fti, const char_t *accept, pstate_t s) {
  f_test_call_at(fti, accept, s, 0);
}

/* skip common string literal, then if char belongs to accept, 
 * call the corresponding function at position pos. 
 * If no match, f_error. If accept = NULL, always call function.
 */
void f_multiplex(fixtagsinfo_t *fti, int n, ...) {
  int i;
  va_list vap;
  if( n < MAXMULTI ) {
    fti->multi.n = n;
    va_start(vap, n);
    for(i = n - 1; i >= 0; --i) {
      fti->multi.dest[i].accept = va_arg(vap, const char_t *);
      fti->multi.dest[i].pst = va_arg(vap, pstate_t);
      fti->multi.dest[i].pos = va_arg(vap, int);
    }
    va_end(vap);
    f_next(fti);
    f_call(fti, f_xmlmultiplex);
  }
}

/* write pegged data, s, and move the peg. If no peg, write to sbuf */
void f_emit(fixtagsinfo_t *fti, const char_t *s) {
  if( fti->peg ) {
    if( fti->peg < fti->begin ) {
      /* printf("F_EMIT_PEG[%.*s]\n", fti->begin - fti->peg, fti->peg); */
      write_htfilter(&fti->out, (char_t *)fti->peg, fti->begin - fti->peg);
      fti->peg = fti->begin;
    }
    if( s ) {
      /* printf("F_EMIT_S[%s]\n", s); */
      write_htfilter(&fti->out, s, strlen(s));
    }
  } else {
    fti->sbuf_ptr = puts_cstring(&fti->sbuf, fti->sbuf_ptr, s);
    /* printf("F_EMIT_SBUF[%s]\n", begin_cstring(&fti->sbuf)); */
  }
}


/* if enable, write pegged data, if disable write sbuf */
void f_enable_buffering(fixtagsinfo_t *fti, bool_t enable) {
  if( fti->peg && enable ) {
    f_emit(fti, NULL);
    fti->peg = NULL;
    fti->sbuf_ptr = begin_cstring(&fti->sbuf);
  } else if( !fti->peg && !enable ) {
    fti->peg = fti->begin;
    f_emit(fti, begin_cstring(&fti->sbuf));
    truncate_cstring(&fti->sbuf, 0);
  }
}

void f_declare_entity(fixtagsinfo_t *fti) {
  add_unique_stringlist(&fti->entities, 
			begin_cstring(&fti->sbuf), STRINGLIST_STRDUP);
}

/* convert an unknown &name; to &amp;name; (WFC: Entity Declared)
 * NOTE: sbuf doesn't start with '&'
 */
void f_verify_entity(fixtagsinfo_t *fti) {
  const char_t *p = begin_cstring(&fti->sbuf);
  if( (p[0] == '&') && (p[1] != '#') ) {
    /* input always ends in ; */
    truncate_cstring(&fti->sbuf, strlen_cstring(&fti->sbuf) - 1);
    if( -1 == find_stringlist(&fti->entities, p + 1) ) {
      insert_cstring(&fti->sbuf, begin_cstring(&fti->sbuf) + 1, "amp;", 4);  
    } else {
      strcat_cstring(&fti->sbuf, ";");
    }
  }
}

void f_declare_per(fixtagsinfo_t *fti) {
  /* todo */
}

void f_verify_per(fixtagsinfo_t *fti) {
  const char_t *p = begin_cstring(&fti->sbuf);
  if( (p[0] == '%') ) {
    /* ignored: to do */
  }
}

void f_verify_hex(fixtagsinfo_t *fti) {
  if( *fti->begin == 'X' ) {
    f_replace(fti, 'x');
  }
}

void f_enter_stag(fixtagsinfo_t *fti) {
  push_tag_xpath(&fti->xpath, begin_cstring(&fti->sbuf));
  reset_stringlist(&fti->attributes);
}

/* within a tag, attribute names must be unique (WFC: Unique Att Spec) */
void f_verify_attribute(fixtagsinfo_t *fti) {
  const char_t *e;
  int i = 1;
  e = end_cstring(&fti->sbuf);
  if( -1 != find_stringlist(&fti->attributes, begin_cstring(&fti->sbuf)) ) {
    putc_cstring(&fti->sbuf, e, '0' + i++);
    while(-1 != find_stringlist(&fti->attributes, begin_cstring(&fti->sbuf)) ) {
      *(char_t *)e = '0' + i++;
      if( i > 9 ) {
	errormsg(E_FATAL, "too many identical attributes.\n");
      }
    }
  }
  add_unique_stringlist(&fti->attributes, 
			begin_cstring(&fti->sbuf), STRINGLIST_STRDUP);
}



bool_t cmp_path_section(const char_t *path, const char_t *name, int len) {
  return ( (strncasecmp(path + 1, name, len) == 0) && 
	   ((path[len + 1] == '\0') || (path[len + 1] == *xpath_delims)) );
}

int find_tag_name(const char_t *path, const char_t *name, int len) {
  int i = 0;
  const char_t *p;
  p = rskip_unescaped_delimiter(path, NULL, *xpath_delims, escc);
  while( p ) {
    i++;
    if( cmp_path_section(p, name, len) ) {
      break;
    }
    p = rskip_unescaped_delimiter(path, --p, *xpath_delims, escc);
  }
  return p ? i : -1;
}

/* when /name occurs in sbuf, check if name is in xpath. If yes, close
 * all tags up to (incl) name, if no, emit <name/>.
 *
 * Note: XML is case sensitive, but most HTML documents in the wild are
 * not careful with case in tag names. Thus we proceed as follows.
 * When searching xpath, the comparisons are case insensitive.
 * Also, the actual tag that is written is always copied from xpath, not taken
 * from sbuf, since this guarantees that the spelling of emitted opening
 * and closing tags is the same (WFC: Element Type Match).
 */
void f_close_etag(fixtagsinfo_t *fti) {
  const char_t *name, *tag;
  int len, n;

  name = begin_cstring(&fti->sbuf);
  len = strlen(name);
  tag = get_last_xpath(&fti->xpath);

  if( name && *name && tag && *tag ) {
    if( strcasecmp(tag, name + 1) == 0 ) {
      puts_cstring(&fti->sbuf, name + 1, tag);
      pop_xpath(&fti->xpath);
    } else {
      n = find_tag_name(string_xpath(&fti->xpath), name + 1, len - 1);
      if( n < 0 ) {
	/* write <name/> */
	delete_cstring(&fti->sbuf, begin_cstring(&fti->sbuf), 1);
	strcat_cstring(&fti->sbuf, xpath_delims);
      } else { 
	/* just close minimal number */
	tag = get_last_xpath(&fti->xpath);
	while( n-- > 0 ) {
	  name = insert_cstring(&fti->sbuf, name, "/", 1);
	  name = insert_cstring(&fti->sbuf, name, tag, strlen(tag));
	  name = insert_cstring(&fti->sbuf, name, "><", 2);
	  pop_xpath(&fti->xpath);
	  tag = get_last_xpath(&fti->xpath);
	} 
	truncate_cstring(&fti->sbuf, name - begin_cstring(&fti->sbuf) - 2);
      }
    }
  }
}

/*
 * This is called whenever the parser fails. On return from f_error(),
 * the parser will continue, so this is the place where the input
 * can be fixed and the current state can be changed.
 * Most of the errors will occur in low level code, so the call stack
 * must be inspected to know what the error means.
 */
void f_error(fixtagsinfo_t *fti, errorcode_t e) {
  /* cstate_t cs; */
  switch(e) {

  case e_unrecoverable:
    errormsg(E_FATAL, "unrepairable document.\n");
    break;

  case e_missing_root:
    if( strncmp(string_xpath(&fti->xpath) + 1, 
		get_root_tag(), strlen(get_root_tag())) != 0 ) {
      f_emit(fti, get_headwrap());
      f_emit(fti, get_open_root());
      push_tag_xpath(&fti->xpath, get_root_tag());
    }
    break;

  case e_malformed_tag:
    puts_cstring(&fti->sbuf, begin_cstring(&fti->sbuf), "&lt;");
    break;

  case e_missing_eq:
    f_emit(fti, "=");
    f_done(fti); 
    break;

  case e_unexpected_literal:
    break;

  case e_unexpected_name:
    if( strchr("'\"<>", *fti->begin) ) {
      f_done(fti); f_done(fti); /* bail out of f_Name */
    } else {
      f_replace(fti, '_'); /* there should be a better way */
    }
    break;

  case e_unhandled_multiplex:
    errormsg(E_FATAL, "e_unhandled_multiplex [%.20s]\n", fti->begin); break;
  case e_ref_required:
    errormsg(E_FATAL, "e_ref_required\n"); break;
  case e_nmtoken:
    errormsg(E_FATAL, "e_nmtoken\n"); break;
  case e_choice:
    errormsg(E_FATAL, "e_choice\n"); break;
  default:
    f_call(fti, f_xmlend); 
    break; 
  }
}


void init_filter(fixtagsinfo_t *fti, ffbuf_t *ffb) {
  fti->begin = ffb->buf;
  fti->end = ffb->buf + ffb->size;
  if( fti->peg ) {
    fti->peg = fti->begin;
  }
}


/* macros for readability */

/* scan a literal */ 
#define LIT(S) {f_next(fti); f_literal(fti, S);}
/* scan a quoted string with optional expansion of specials */
#define QUOTEX(EX) {f_next(fti); f_quotedstring(fti, EX);}
/* goto another instruction */
#define GOTO(N) {f_next_at(fti, N);}
/* if any chars in S match, goto T else goto F */
#define IF(S,T,F) {f_test(fti, S, T, F);}
/* try to scan a literal, on success goto T else goto F */
#define IF_LIT(S,T,F) {f_test_literal(fti, S, T, F);}
/* call a function */
#define CALL(F) {f_next(fti); f_call(fti, F);}
/* if any chars in S match, call a function */
#define IF_CALL(S,F) {f_next(fti); f_test_call(fti, S, F);}
/* if none of the chars in S match, call a function */
#define UNLESS_CALL(S,F) {f_next(fti); f_test_call(fti, S, F);}
/* skip common prefix, then switch to a function */
#define SWITCH2(A1,F1,P1, A2,F2,P2) {f_multiplex(fti,2, A1,F1,P1, A2,F2,P2);}
#define SWITCH3(A1,F1,P1, A2,F2,P2, A3,F3,P3) {f_multiplex(fti,3, A1,F1,P1, A2,F2,P2, A3,F3,P3);}
#define SWITCH4(A1,F1,P1, A2,F2,P2, A3,F3,P3, A4,F4,P4) {f_multiplex(fti,4, A1,F1,P1, A2,F2,P2, A3,F3,P3, A4,F4,P4);}
/* return to preceding block */
#define RETURN {f_done(fti);}
/* scan characters, stopping when one of the chars in S is seen */
#define WAIT_FOR(S) {f_next(fti); f_skip(fti, NULL, S);}
/* switch from peg to sbuf */
#define SAVE {f_next(fti); f_enable_buffering(fti, TRUE);}
/* switch from sbuf to peg */
#define RESTORE {f_next(fti); f_enable_buffering(fti, FALSE);}

/* we roll our own XML parser because expat's aborts on errors, and
 * it's not a good idea to hack its internals to repair input errors.
 * It's a state machine (fti->state), made to look like a primitive
 * programming language so that XML productions (from XML 1.0 5th edition)
 * can be easily recognized.
 *
 * THE TASK OF filter_buffer() IS TO OUTPUT WELL FORMED XML. IT DOES NOT
 * INTERPRET THE INPUT (E.G. AS HTML). IT IS THE RESPONSIBILITY OF f->out
 * TO TRANSFORM THE XML IT RECEIVES SEMANTICALLY (E.G. AS HTML).
 *
 * The basic strategy is that input is read one character at a time.
 * If it scans, the character will be printed eventually. If it doesn't
 * scan, then we print whatever we think will keep the output well formed.
 * The well formed constraints (WFC:) are shown as comments where 
 * appropriate (see XML 1.0 5th ed for definitions).
 *
 * After processing, each incoming character is either left after the
 * peg or put into the sbuf for later processing. The sbuf typically
 * contains a single token such as a tag name or an entity name.
 * The peg is for large amounts of text which can be printed unchanged.
 *
 * All "repairs" occur by calling f_emit() as required. This is the 
 * only function which actually outputs text, and handles the peg/sbuf
 * distinctions transparently.
 * 
 * If a literal doesn't scan, then it is emitted automatically. This
 * ensures that the syntactic sugar is always correct. For semantic
 * errors and errors in names or content etc, the repairs are more complex. 
 * 
 */   
void filter_buffer(fixtagsinfo_t *fti, ffbuf_t *ffb) {

  while( fti->begin < fti->end ) {
    /* show_state(fti); */
    switch(fti->state.pst) {

    case f_xmlstart:

      f_enable_buffering(fti, FALSE);
      if( checkflag(u_options, FIXTAGS_FLAG_WRAP) ) {
	f_error(fti,e_missing_root);
	CALL(f_mainloop); continue;
      } else {
	CALL(f_document); continue;
      }
      break;

    case f_xmlliteral: 
      if( fti->lit.s[fti->lit.pos] ) {
	if( fti->lit.s[fti->lit.pos] == *fti->begin ) {
	  fti->lit.pos++;
	} else { /* unexpected or incomplete - flag error and emit lit */
	  if( fti->lit.pos == 0 ) {
	    f_error(fti, e_unexpected_literal); 
	  }
	  f_emit(fti, fti->lit.s + fti->lit.pos);
	  fti->lit.s = "";
	  f_done(fti); continue;
	}
      } else {
	fti->lit.s = "";
	f_done(fti); continue;
      }
      break;

    case f_xmlskip:

      if( (fti->skip.reject && strchr(fti->skip.reject, *fti->begin)) ||
	  (fti->skip.accept && !strchr(fti->skip.accept, *fti->begin)) ) {
	f_done(fti); continue;
      }
      break;

    case f_xmlstring:

      if( fti->str.delim && (*fti->begin == *fti->str.delim) ) {
	f_done(fti); continue;
      } else if( (*fti->begin == '&') && 
		 fti->str.expand && strchr(fti->str.expand, '&') ) {
	f_call(fti, f_Reference); continue;
      } else if( (*fti->begin == '%') && 
		 fti->str.expand && strchr(fti->str.expand, '%') ) {
	f_call(fti, f_PEReference); continue;
      } else if( fti->str.bailout && strchr(fti->str.bailout, *fti->begin) ) {
	f_done(fti); continue;
      } else if( fti->str.expand && strchr(fti->str.expand, *fti->begin) ) {
	/* amp will not be expanded here */
	f_emit(fti, ( (*fti->begin == '&') ? "&amp" :
		      (*fti->begin == '>') ? "&gt" :
		      (*fti->begin == '\'') ? "&apos" :
		      (*fti->begin == '"') ? "&quot" :
		      (*fti->begin == '<') ? "&lt" : NULL ));
	f_replace(fti, ';');
	continue;
      }
      break;

    case f_xmlquotedstring:
      switch(fti->state.pos) {
      case 0: 
	switch(*fti->begin) {
	case '"': fti->str.delim = "\""; fti->str.bailout = NULL; break;
	case '\'': fti->str.delim = "'"; fti->str.bailout = NULL; break;
	default: 
	  fti->str.delim = "\""; fti->str.bailout = " \t\r\n>'\""; break;
	}
	f_next(fti); f_literal(fti, fti->str.delim); continue;
      case 1: f_next(fti); f_call(fti, f_xmlstring); continue;
      case 2: f_next(fti); f_literal(fti, fti->str.delim); continue;
      default: f_done(fti); continue;
      }
      break;

    case f_xmlremexcl: 
      /* wraps an unknown <!xyz> tag in a comment: <!-- xyz --> */
      switch(fti->state.pos) {
      case 0: LIT("-- "); continue;
      case 1: WAIT_FOR(">"); continue;
      case 2: LIT(" --"); continue;
      case 3: LIT(">"); continue;
      default: f_done(fti); continue;
      }
      break;

    case f_xmlmultiplex:
      switch(fti->state.pos) {
      case 0: 
	while( --fti->multi.n >= 0 ) {
	  if( !fti->multi.dest[fti->multi.n].accept ||
	      strchr(fti->multi.dest[fti->multi.n].accept,*fti->begin) ) {
	    f_next(fti);
	    f_call_at(fti, 
		      fti->multi.dest[fti->multi.n].pst, 
		      fti->multi.dest[fti->multi.n].pos);
	    break;
	  }
	}
	if( fti->multi.n >= 0 ) {
	  continue;
	}
	f_error(fti, e_unhandled_multiplex);
	/* fall through */
      default: f_done(fti); continue;
      }
      break;

    case f_xmlerror:
      f_error(fti, fti->state.pos); f_done(fti); continue;

    case f_document:

      switch(fti->state.pos) {
      case 0: IF_LIT("<", 1, 4); continue;
      case 1: SWITCH4("?", f_PIOrXMLDecl, 1,
		      "!", f_Meta, 1,
		      "/", f_ETag, 1,
		      NULL, f_STag, 1); continue;
      case 2: GOTO(4); continue;
      case 3: f_error(fti, e_missing_root); CALL(f_chardata); continue;
      case 4: CALL(f_mainloop); continue;
      default: RETURN; continue;
      }
      break;

    case f_chardata:

      switch(fti->state.pos) {
      case 0: 
	fti->str.delim = NULL;
	fti->str.expand = "&";
	fti->str.bailout = "<";
	f_next(fti); f_call(fti, f_xmlstring); continue;
      default: RETURN; continue;
      }	
      break;

    case f_mainloop:

      switch(fti->state.pos) {
      case 0: IF("<", 1, 8); continue;
      case 1: SAVE; continue;
      case 2: LIT("<"); continue;
      case 3: IF("<>&'\" \t\r\n0123456789", 4, 6); continue;
      case 4: f_error(fti, e_malformed_tag); RESTORE; continue;
      case 5: GOTO(9); continue;
      case 6: RESTORE; continue;
      case 7: SWITCH4("?", f_PI, 1,
		      "!", f_Meta1, 1,
		      "/", f_ETag, 1,
		      NULL, f_STag, 1); continue;
      case 8: GOTO(0);
      case 9: CALL(f_chardata); continue;
      case 10: GOTO(0); continue;
      default: RETURN; continue;
      }
      break;

    case f_PIOrXMLDecl:

      switch(fti->state.pos) {
      case 0: LIT("<"); continue;
      case 1: LIT("?"); continue;
      case 2: SWITCH2("x", f_XMLDecl, 1,
		      NULL, f_PI, 1); continue;
      default: RETURN; continue;
      }
      break;

    case f_PI:

      switch(fti->state.pos) {
      case 0: LIT("<"); continue; /* multiplex */
      case 1: LIT("?"); continue;
      case 2: CALL(f_PITarget); continue;
      case 3: CALL(f_So); continue;
      case 4: WAIT_FOR(">?"); continue;
      case 5: LIT("?"); continue; /* adds ? if we see > */
      case 6: IF_LIT(">", 7, 4); continue;
      default: RETURN; continue;
      }
      break;

    case f_PITarget:

      switch(fti->state.pos) {
      case 0: IF("xX", 1, 8); continue;
      case 1: f_next(fti); break; /* skip char */
      case 2: IF("mM", 3, 6); continue;
      case 3: f_next(fti); break; /* skip char */
      case 4: IF("lL", 5, 6); continue;
      case 5: f_next(fti); f_replace(fti, '_'); continue; /* replace char */
      case 6: f_next(fti); f_call_at(fti, f_Name, 1); continue;
      case 7: RETURN; continue;
      case 8: CALL(f_Name); continue;
      default: RETURN; continue;
      }
      break;
      
    case f_S:
      /* consume a mandatory space, or create one if necessary */
      if( !xml_whitespace(*fti->begin) ) {
	f_emit(fti, " "); 
	RETURN; continue;
      }
      fti->state.pst = f_So; /* but always allow optional space */
      break;

    case f_So:
      /* consume optional white space */
      if( !xml_whitespace(*fti->begin) ) {
	RETURN; continue;
      }
      break;

    case f_Eq:

      switch(fti->state.pos) {
      case 0: CALL(f_So); continue;
      case 1: IF_LIT("=", 3, 2); continue;
      case 2: f_next(fti); f_error(fti, e_missing_eq); continue;
      case 3: CALL(f_So); continue;
      default: RETURN; continue;
      }
      break;

    case f_Tag:

      switch(fti->state.pos) {
      case 0: LIT("<"); continue;
      case 1: SWITCH2("/", f_ETag, 1,
		      NULL, f_STag, 1); continue;
      default: RETURN; continue;
      }
      break;

    case f_STag:

      switch(fti->state.pos) { 
      case 0: LIT("<"); continue;
      case 1: SAVE; continue;
      case 2: CALL(f_Name); continue;
      case 3: f_enter_stag(fti); RESTORE; continue;
      case 4: CALL(f_So); continue;
      case 5: IF(">/", 8, 6); continue;
      case 6: CALL(f_Attribute); continue;
      case 7: GOTO(4); continue;
      case 8: IF_LIT("/", 9, 10); continue;
      case 9: pop_xpath(&fti->xpath); f_next(fti); continue;
      case 10: LIT(">"); continue;
      default: RETURN; continue;
      }
      break;

    case f_Name:

      switch(fti->state.pos) {
      case 0: CALL(f_NameStartChar); continue;
      case 1: CALL(f_NameChar); continue;
      default: RETURN; continue;
      }
      break;

    case f_NameStartChar:

      if( !xml_startnamechar(*fti->begin) ) {
	f_error(fti, e_unexpected_name);
	continue;
      }
      RETURN; /* not continue */
      break;

    case f_NameChar:

      if( !xml_namechar(*fti->begin) ) {
	RETURN; continue;
      }
      break;

    case f_Attribute:
      /* WFC: Unique Att Spec */
      switch(fti->state.pos) { 
      case 0: SAVE;
      case 1: CALL(f_Name); continue;
      case 2: f_verify_attribute(fti); RESTORE; continue;
      case 3: CALL(f_Eq); continue;
      case 4: CALL(f_AttValue); continue;
      default: RETURN; continue;
      }
      break;
      
    case f_AttValue:
      /* WFC: No External Entity References */
      /* WFC: No < in Attribute Values */
      switch(fti->state.pos) {
      case 0: QUOTEX("&<>'\""); continue; 
      default: RETURN; continue;
      }
      break;

    case f_SystemLiteral:

      switch(fti->state.pos) {
      case 0: QUOTEX(NULL); continue;
      default: RETURN; continue;
      }
      break;

    case f_PubidLiteral:

      switch(fti->state.pos) {
      case 0: QUOTEX(NULL); continue;
      default: RETURN; continue;
      }
      break;

    case f_ETag:
      /* WFC: Element Type Match */
      switch(fti->state.pos) { 
      case 0: LIT("<"); continue; /* multiplex */
      case 1: SAVE; continue;
      case 2: LIT("/"); continue;
      case 3: CALL(f_Name); continue;
      case 4: f_close_etag(fti); RESTORE; continue; 
      case 5: CALL(f_So); continue;
      case 6: LIT(">"); continue;
      default: 
	if( *string_xpath(&fti->xpath) == '\0' ) {
	  f_call(fti, f_xmlend);
	} else {
	  RETURN; continue;
	}
      }
      break;

    case f_Reference:
      /* WFC: Entity Declared */
      /* WFC: Parsed Entity (superfluous) */
      /* WFC: No Recursion */
      switch(fti->state.pos) { 
      case 0: SAVE; continue;
      case 1: LIT("&"); continue;
      case 2: IF("#", 3, 5); continue;
      case 3: CALL(f_CharRef); continue;
      case 4: GOTO(6); continue;
      case 5: CALL(f_Name); continue;
      case 6: LIT(";"); continue;
      case 7: f_verify_entity(fti); RESTORE; continue;
      default: RETURN; continue;
      }
      break;

    case f_CharRef:

      switch(fti->state.pos) { 
      case 0: LIT("#"); continue;
      case 1: f_verify_hex(fti); IF_LIT("x", 2, 4); continue;
      case 2: CALL(f_xmlxdigit); continue;
      case 3: GOTO(5); continue;
      case 4: CALL(f_xmldigit); continue;
      case 5: LIT(";");
      default: RETURN; continue;
      }
      break;

    case f_xmldigit:
      
      if( !xml_digit(*fti->begin) ) {
	RETURN; continue;
      }
      break;

    case f_xmlxdigit:
      
      if( !xml_xdigit(*fti->begin) ) {
	RETURN; continue;
      }
      break;

    case f_PEReference:
      /* WFC: No Recursion */
      switch(fti->state.pos) { 
      case 0: SAVE; continue;
      case 1: LIT("%"); continue;
      case 2: CALL(f_Name); continue;
      case 3: LIT(";"); continue;
      case 4: f_verify_per(fti); RESTORE; continue;
      default: RETURN; continue;
      }
      break;

    case f_EntityValue:

      switch(fti->state.pos) {
      case 0: QUOTEX("&%"); continue;
      default: RETURN; continue;
      }
      break;

    case f_Meta:
      /* used in prolog */
      switch(fti->state.pos) { 
      case 0: LIT("<"); continue;
      case 1: LIT("!"); continue;
      case 2: SWITCH3("-", f_Comment, 1,
		      "[", f_CDSect, 1,
		      "D", f_doctypedecl, 1); continue;
      default: RETURN; continue;
      }
      break;

    case f_Meta1:
      /* used in element body */
      switch(fti->state.pos) { 
      case 0: LIT("<"); continue;
      case 1: LIT("!"); continue;
      case 2: SWITCH3("-", f_Comment, 1,
		      "[", f_CDSect, 1,
		      NULL, f_xmlremexcl, 0); continue;
      default: RETURN; continue;
      }
      break;

    case f_Meta2:
      /* used in internal subset */
      switch(fti->state.pos) { 
      case 0: LIT("<"); continue;
      case 1: LIT("!"); continue;
      case 2: SWITCH4("-", f_Comment, 1,
		      "E", f_ElementDeclOrEntityDecl, 1,
		      "A", f_AttlistDecl, 1,
		      "N", f_NotationDecl, 1); continue;
      default: RETURN; continue;
      }
      break;

    case f_Comment:

      switch(fti->state.pos) {
      case 0: LIT("<!"); continue; /* multiplex */
      case 1: LIT("--"); continue;
      case 2: WAIT_FOR("-"); continue;
      case 3: LIT("-"); continue; 
      case 4: IF_LIT("->", 5, 2); continue;
      default: RETURN; continue;
      }
      break;

    case f_CDSect:

      switch(fti->state.pos) {
      case 0: LIT("<!"); continue; /* multiplex */
      case 1: LIT("[DATA["); continue;
      case 2: WAIT_FOR("]"); continue;
      case 3: LIT("]"); continue; 
      case 4: IF_LIT("]", 5, 2); continue;
      case 5: IF_LIT(">", 6, 2); continue;
      case 6: LIT(">"); continue;
      default: RETURN; continue;
      }
      break;

    case f_XMLDecl:

      switch(fti->state.pos) {
      case 0: LIT("<?"); continue;
      case 1: LIT("xml"); continue;
      case 2: CALL(f_VersionInfo); continue;
      case 3: IF("?", 8, 4); continue;
      case 4: CALL(f_So); continue;
      case 5: IF_CALL("e", f_EncodingDecl); continue;
      case 6: IF_CALL("s", f_SDDecl); continue;
      case 7: CALL(f_So); continue;
      case 8: LIT("?>"); continue;
      default: RETURN; continue;
      }
      break;
      
    case f_VersionInfo:

      switch(fti->state.pos) {
      case 0: CALL(f_S); continue;
      case 1: LIT("version"); continue;
      case 2: CALL(f_Eq); continue;
      case 3: LIT("\"1.0\""); continue;
      default: RETURN; continue;
      }
      break;

    case f_EncodingDecl:

      switch(fti->state.pos) {
      case 0: LIT("encoding"); continue;
      case 1: CALL(f_Eq); continue;
      case 2: CALL(f_AttValue); continue;
      default: RETURN; continue;
      }
      break;

    case f_SDDecl:

      switch(fti->state.pos) {
      case 0: LIT("standalone"); continue;
      case 1: CALL(f_Eq); continue;
      case 2: CALL(f_AttValue); continue;
      default: RETURN; continue;
      }
      break;

    case f_doctypedecl:

      switch(fti->state.pos) {
      case 0: LIT("<!"); continue; /* multiplex */
      case 1: LIT("DOCTYPE"); continue;
      case 2: CALL(f_S); continue;
      case 3: CALL(f_Name); continue;
      case 4: CALL(f_So); continue;
      case 5: IF_CALL("SP", f_ExternalID); continue;
      case 6: CALL(f_So); continue;
      case 7: IF_CALL("[", f_intSubset); continue;
      case 8: LIT(">"); continue;
      default: RETURN; continue;
      }
      break;

    case f_ExternalID:

      switch(fti->state.pos) {
      case 0: SWITCH2("S", f_SystemID, 0,
		      "P", f_PublicID1, 0); continue;
      default: RETURN; continue;
      }
      break;

    case f_SystemID:

      switch(fti->state.pos) {
      case 0: LIT("SYSTEM"); continue;
      case 1: CALL(f_S); continue;
      case 2: CALL(f_SystemLiteral); continue;
      default: RETURN; continue;
      }
      break;

    case f_PublicID1:

      switch(fti->state.pos) {
      case 0: LIT("PUBLIC"); continue;
      case 1: CALL(f_S); continue;
      case 2: CALL(f_PubidLiteral); continue;
      case 3: CALL(f_S); continue;
      case 4: CALL(f_SystemLiteral); continue;
      default: RETURN; continue;
      }
      break;

    case f_PublicID2:

      switch(fti->state.pos) {
      case 0: LIT("PUBLIC"); continue;
      case 1: CALL(f_S); continue;
      case 2: CALL(f_PubidLiteral); continue;
      case 3: CALL(f_So); continue;
      case 4: IF_CALL("\"'", f_SystemLiteral); continue;
      default: RETURN; continue;
      }
      break;

    case f_intSubset:
      /* WFC: PE Between Declarations */
      /* WFC: PEs in Internal Subset */
      switch(fti->state.pos) {
      case 0: LIT("["); continue;
      case 1: CALL(f_So); continue;
      case 2: IF_LIT("<", 3, 5); continue;
      case 3: SWITCH3("!", f_Meta2, 1,
		      "?", f_PI, 1,
		      "%", f_PEReference, 1); continue; 
      case 4: GOTO(1); continue;
      case 5: LIT("]"); continue;
      case 6: CALL(f_So); continue;
      default: RETURN; continue;
      }
      break;

    case f_ElementDeclOrEntityDecl:
      
      switch(fti->state.pos) {
      case 0: LIT("<!"); continue; /* multiplex */
      case 1: LIT("E"); continue;
      case 2: SWITCH2("L", f_ElementDecl, 1,
		      "N", f_EntityDecl, 1); continue;
      default: RETURN; continue;
      }
      break;

    case f_ElementDecl:

      switch(fti->state.pos) {
      case 0: LIT("<!E"); continue; /* multiplex */
      case 1: LIT("LEMENT"); continue;
      case 2: CALL(f_S); continue;
      case 3: CALL(f_Name); continue;
      case 4: CALL(f_S); continue;
      case 5: CALL(f_contentspec); continue;
      case 6: CALL(f_So); continue;
      case 7: LIT(">"); continue;
      default: RETURN; continue;
      }
      break;

    case f_EntityDecl:

      switch(fti->state.pos) {
      case 0: LIT("<!E"); continue; /* multiplex */
      case 1: LIT("NTITY"); continue;
      case 2: CALL(f_S); continue;
      case 3: SWITCH2("%", f_PEDecl, 0,
		      NULL, f_GEDecl, 0); continue;
      case 4: CALL(f_So); continue;
      case 5: LIT(">"); continue;
      default: RETURN; continue;
      }
      break;

    case f_PEDecl:

      switch(fti->state.pos) {
      case 0: LIT("%"); continue;
      case 1: CALL(f_S); continue;
      case 2: SAVE; continue;
      case 3: CALL(f_Name); continue;
      case 4: f_declare_per(fti); RESTORE; continue;
      case 5: CALL(f_S); continue;
      case 6: CALL(f_PEDef); continue;
      default: RETURN; continue;
      }
      break;

    case f_PEDef:

      switch(fti->state.pos) {
      case 0: IF("SP", 1, 3); continue;
      case 1: CALL(f_ExternalID); continue;
      case 2: RETURN; continue;
      case 3: CALL(f_EntityValue); continue;
      default: RETURN; continue;
      }
      break;

    case f_GEDecl:
      switch(fti->state.pos) {
      case 0: SAVE; continue;
      case 1: CALL(f_Name); continue;
      case 2: f_declare_entity(fti); RESTORE; continue;
      case 3: CALL(f_S); continue;
      case 4: CALL(f_EntityDef); continue;
      default: RETURN; continue;
      }
      break;

    case f_EntityDef:

      switch(fti->state.pos) {
      case 0: IF("SP", 1, 7); continue;
      case 1: CALL(f_ExternalID); continue;
      case 2: CALL(f_So); continue;
      case 3: IF_LIT("NDATA", 4, 6); continue;
      case 4: CALL(f_S); continue;
      case 5: CALL(f_Name); continue;
      case 6: RETURN; continue;
      case 7: CALL(f_EntityValue); continue;
      default: RETURN; continue;
      }
      break;

    case f_AttlistDecl:

      switch(fti->state.pos) {
      case 0: LIT("<!"); continue; /* multiplex */
      case 1: LIT("ATTLIST"); continue;
      case 2: CALL(f_S); continue;
      case 3: CALL(f_Name); continue;
      case 4: IF(">", 9, 5); continue;
      case 5: CALL(f_S); continue;
      case 6: IF(">", 9, 7); continue;
      case 7: CALL(f_AttDef); continue;
      case 8: GOTO(4); continue;
      case 9: LIT(">"); continue;
      default: RETURN; continue;
      }
      break;

    case f_AttDef:
      switch(fti->state.pos) {
      case 0: CALL(f_Name); continue;
      case 1: CALL(f_S); continue;
      case 2: CALL(f_AttType); continue;
      case 3: CALL(f_S); continue;
      case 4: CALL(f_DefaultDecl); continue;
      default: RETURN; continue;
      }
      break;

    case f_AttType:

      switch(fti->state.pos) {
      case 0: IF_LIT("CDATA", 1, 2); continue;
      case 1: RETURN; continue;
      case 2: IF_LIT("ID", 3, 6); continue;
      case 3: IF_LIT("REF", 4, 5); continue;
      case 4: IF_LIT("S", 5, 5); continue;
      case 5: RETURN; continue;
      case 6: IF_LIT("ENTIT", 7, 10); continue;
      case 7: IF_LIT("IES", 9, 8); continue;
      case 8: LIT("Y"); continue;
      case 9: RETURN; continue;
      case 10: IF_LIT("NMTOKEN", 11, 13); continue;
      case 11: IF_LIT("S", 12, 12); continue;
      case 12: RETURN; continue;
      case 13: SWITCH2("N", f_NotationType, 0,
		       NULL, f_Enumeration, 0); continue;
      default: RETURN; continue;
      }
      break;

    case f_NotationType:

      switch(fti->state.pos) {
      case 0: LIT("NOTATION"); continue; 
      case 1: CALL(f_S); continue;
      case 2: LIT("("); continue;
      case 3: CALL(f_So); continue;
      case 4: CALL(f_Name); continue;
      case 5: CALL(f_So); continue;
      case 6: IF_LIT("|", 7, 10); continue;
      case 7: CALL(f_So); continue;
      case 8: CALL(f_Name); continue;
      case 9: GOTO(5); continue;
      case 10: LIT(")"); continue;
      default: RETURN; continue;
      }
      break;
      
    case f_Enumeration:

      switch(fti->state.pos) {
      case 0: LIT("("); continue; 
      case 1: CALL(f_So); continue;
      case 2: CALL(f_Nmtoken); continue;
      case 3: CALL(f_So); continue;
      case 4: IF_LIT("|", 5, 8); continue;
      case 5: CALL(f_So); continue;
      case 6: CALL(f_Nmtoken); continue;
      case 7: GOTO(3); continue;
      case 8: LIT(")"); continue;
      default: RETURN; continue;
      }
      break;

    case f_Nmtoken:

      switch(fti->state.pos) {
      case 0: 
	if( xml_namechar(*fti->begin) ) {
	  fti->state.pos++;
	} else {
	  f_error(fti, e_nmtoken);
	}
      case 1:
	if( !xml_namechar(*fti->begin) ) {
	  RETURN; continue;
	}
      }
      break;

    case f_DefaultDecl:

      switch(fti->state.pos) {
      case 0: IF_LIT("#", 1, 5); continue;
      case 1: IF_LIT("REQUIRED", 6, 2); continue;
      case 2: IF_LIT("IMPLIED", 6, 3); continue;
      case 3: LIT("FIXED"); continue;
      case 4: CALL(f_S); continue;
      case 5: CALL(f_AttValue); continue;
      default: RETURN; continue;
      }
      break;

    case f_NotationDecl:

      switch(fti->state.pos) {
      case 0: LIT("NOTATION"); continue; 
      case 1: CALL(f_S); continue;
      case 2: CALL(f_Name); continue;
      case 3: CALL(f_S); continue;
      case 4: SWITCH2("S", f_SystemID, 0,
		      "P", f_PublicID2, 0); continue;
      case 5: CALL(f_So); continue;
      case 6: LIT(">"); continue;
      default: RETURN; continue;
      }
      break;

    case f_contentspec:

      switch(fti->state.pos) {
      case 0: IF_LIT("EMPTY", 5, 1); continue;
      case 1: IF_LIT("ANY", 5, 2); continue;
      case 2: LIT("("); continue;
      case 3: CALL(f_So); continue;
      case 4: SWITCH2("#", f_Mixed, 1,
		      NULL, f_children, 0); continue;
      default: RETURN; continue;
      }
      break;

    case f_Mixed:

      switch(fti->state.pos) {
      case 0: LIT("("); continue; /* multiplex */
      case 1: CALL(f_So); continue;
      case 2: LIT("#PCDATA"); continue;
      case 3: CALL(f_So); continue;
      case 4: IF_LIT(")", 11, 5); continue;
      case 5: IF_LIT("|", 6, 10); continue;
      case 6: CALL(f_So); continue;
      case 7: CALL(f_Name); continue;
      case 8: CALL(f_So); continue;
      case 9: GOTO(5); continue;
      case 10: LIT(")*"); continue;
      default: RETURN; continue;
      }
      break;

    case f_children:

      switch(fti->state.pos) {
      case 0: f_next(fti); f_call_at(fti, f_choiceOrseq, 2); continue;
      case 1: IF_LIT("?", 4, 2); continue;
      case 2: IF_LIT("*", 4, 3); continue;
      case 3: IF_LIT("+", 4, 4); continue;
      default: RETURN; continue;
      }
      break;

    case f_choiceOrseq:

      switch(fti->state.pos) {
      case 0: LIT("("); continue; /* multiplex */
      case 1: CALL(f_So); continue;
      case 2: CALL(f_cp); continue;
      case 3: CALL(f_So); continue;
      case 4: IF_LIT(")", 6, 5); continue;
      case 5: SWITCH2(",", f_seq, 4,
		      NULL, f_choice, 4); continue;
      default: RETURN; continue;
      }
      break;

    case f_cp:

      switch(fti->state.pos) {
      case 0: SWITCH2("(", f_choiceOrseq, 0,
		      NULL, f_Name, 0); continue;
      case 1: IF_LIT("?", 4, 2); continue;
      case 2: IF_LIT("*", 4, 3); continue;
      case 3: IF_LIT("+", 4, 4); continue;
      default: RETURN; continue;
      }
      break;

    case f_seq:

      switch(fti->state.pos) {
      case 0: LIT("("); continue;
      case 1: CALL(f_So); continue;
      case 2: CALL(f_cp); continue;
      case 3: CALL(f_So); continue;
      case 4: IF_LIT(",", 5, 8); continue;
      case 5: CALL(f_So); continue;
      case 6: CALL(f_cp); continue;
      case 7: GOTO(3); continue;
      case 8: LIT(")"); continue;
      default: RETURN; continue;
      }
      break;

    case f_choice:

      switch(fti->state.pos) {
      case 0: LIT("("); continue;
      case 1: CALL(f_So); continue;
      case 2: CALL(f_cp); continue;
      case 3: CALL(f_So); continue;
      case 4: IF_LIT("|", 5, 9); continue;
      case 5: CALL(f_So); continue;
      case 6: CALL(f_cp); continue;
      case 7: CALL(f_So); continue;
      case 8: GOTO(4); continue;
      case 9: LIT(")"); continue;
      default: RETURN; continue;
      }
      break;

    case f_xmlend:
      /* do nothing, the stdout debugger will print a message */
      f_emit(fti, NULL); 
      return;

    }
    if( !fti->peg ) {
      fti->sbuf_ptr = putc_cstring(&fti->sbuf, fti->sbuf_ptr, *fti->begin);
    }
    fti->begin++;
  }

}

bool_t close_fun(xpath_t *xp, tagtype_t t, const char_t *begin, const char_t *end, void *user) {
  fixtagsinfo_t *fti = (fixtagsinfo_t *)user;
  if( fti ) {
    write_htfilter(&fti->out, "</", 2);
    write_htfilter(&fti->out, begin, end - begin);
    write_htfilter(&fti->out, ">\n", 2);
    return TRUE;
  }
  return FALSE;
}

void close_path(fixtagsinfo_t *fti) {
  f_emit(fti, NULL);
  close_xpath(&fti->xpath, close_fun, fti);
}

void exit_filter(fixtagsinfo_t *fti, ffbuf_t *ffb) {
  f_emit(fti, NULL);
}


int filter_file(fixtagsinfo_t *fti, const char_t *file) {
  stream_t strm;
  ffbuf_t ffbuf;
  int exit_value = EXIT_SUCCESS;

  if( open_file_stream(&strm, file) ) {

    ffbuf.buf = malloc(strm.blksize);
    if( !ffbuf.buf ) {
      errormsg(E_FATAL, "out of memory.\n");
    }
    read_ffbuf(&ffbuf, &strm);

    initialize_htfilter(&fti->out);

    do {
      init_filter(fti, &ffbuf);
      filter_buffer(fti, &ffbuf);
      exit_filter(fti, &ffbuf);
    } while( read_ffbuf(&ffbuf, &strm) );
    
    close_path(fti);
    finalize_htfilter(&fti->out);

    free((void*)ffbuf.buf);

    close_stream(&strm);
  }
  return exit_value;
}


bool_t create_fixtagsinfo(fixtagsinfo_t *fti) {
  bool_t ok = TRUE;
  flag_t hflags = 0;
  if( fti ) {
    ok &= (NULL != create_cstring(&fti->sbuf, "", 64));
    ok &= create_xpath(&fti->xpath);
    ok &= create_objstack(&fti->stack, sizeof(cstate_t));
    ok &= create_stringlist(&fti->entities);
    ok &= create_stringlist(&fti->attributes);
    hflags = 
      checkflag(u_options, FIXTAGS_FLAG_HTML) ? HTFILTER_HTML : HTFILTER_DUMB;
    ok &= create_htfilter(&fti->out, hflags);
    /* ok &= create_htfilter(&fti->out, HTFILTER_DEBUG); */

    if( ok ) {
      add_stringlist(&fti->entities, "amp", STRINGLIST_STRDUP);
      add_stringlist(&fti->entities, "lt", STRINGLIST_STRDUP);
      add_stringlist(&fti->entities, "gt", STRINGLIST_STRDUP);
      add_stringlist(&fti->entities, "apos", STRINGLIST_STRDUP);
      add_stringlist(&fti->entities, "quot", STRINGLIST_STRDUP);

      fti->state.pst = f_xmlend;
      fti->state.pos = 0;
      fti->peg = NULL;
      fti->sbuf_ptr = begin_cstring(&fti->sbuf);
      fti->lit.s = "";
      f_call(fti, f_xmlstart);
    }
    return ok;
  }
  return FALSE;
}

bool_t free_fixtagsinfo(fixtagsinfo_t *fti) {
  if( fti ) {
    free_cstring(&fti->sbuf);
    free_xpath(&fti->xpath);
    free_objstack(&fti->stack);
    free_stringlist(&fti->entities);
    free_stringlist(&fti->attributes);
    free_htfilter(&fti->out);
  }
  return TRUE;
}

int main(int argc, char **argv) {
  signed char op;
  fixtagsinfo_t ftinfo;
  filelist_t fl;
  cstringlst_t files;
  int exit_value = EXIT_FAILURE;

  struct option longopts[] = {
    { "version", 0, NULL, FIXTAGS_VERSION },
    { "help", 0, NULL, FIXTAGS_HELP },
    { "root-wrap", 0, NULL, FIXTAGS_WRAP },
    { "html", 0, NULL, FIXTAGS_HTML },
    { "xml", 0, NULL, FIXTAGS_XML },
    { 0 }
  };

  progname = "xml-fixtags";
  inputfile = "";
  inputline = 0;
  
  while( (op = getopt_long(argc, argv, "",
			   longopts, NULL)) > -1 ) {
    set_option(op, optarg);
  }

  if( create_fixtagsinfo(&ftinfo) ) {

    init_signal_handling();
    init_file_handling();

    if( create_filelist(&fl, -1, argv + optind, FILELIST_EQ1) ) {

      files = getfiles_filelist(&fl);

      open_stdout();

      inputfile = (char *)files[0];
      exit_value = filter_file(&ftinfo, inputfile);

      close_stdout();

    }

    exit_file_handling();
    exit_signal_handling();

    free_fixtagsinfo(&ftinfo);
  }
  return exit_value;
}
