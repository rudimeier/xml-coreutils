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
#include "sed.h"
#include "mem.h"
#include "myerror.h"
#include "stdout.h"
#include "entities.h"
#include <string.h>

#include <unistd.h>
#include <limits.h>

extern const char_t escc;

bool_t create_sedad(sedad_t *sa, sedadid_t id) {
  if( sa ) {
    memset(sa, 0, sizeof(sedad_t));
    sa->id = id;
    return TRUE;
  }
  return FALSE;
}

bool_t free_sedad(sedad_t *sa) {
  if( sa ) {
    return TRUE;
  }
  return FALSE;
}

const char_t *parse_single_address(const char_t *begin, const char_t *end, long *val) {
  if( *begin == '$' ) {
    *val = LONG_MAX;
    return (++begin);
  } 
  return skip_xml_integer(begin, end, val);
}

const char_t *compile_range_sedad(sedad_t *sa, const char_t *begin, const char_t *end) {
  const char_t *r = NULL;
  if( sa && begin && (begin < end) ) {
    sa->id = INTERVAL;
    r = parse_single_address(begin, end, &sa->args.range.line1);
    if( *r == ',' ) {
      r++;
      r = parse_single_address(r, end, &sa->args.range.line2);
      sa->args.range.line2 = 
	MAX(sa->args.range.line2,(sa->args.range.line1 + 1));
    } else {
      sa->args.range.line2 = sa->args.range.line1 + 1;
    }
  }
  return r;
}


const char_t *compile_sedad(sedad_t *sa, const char_t *begin, const char_t *end) {
  const char_t *r = NULL;
  if( sa && begin && (begin < end) ) {
    if( xml_isdigit(*begin) ) {
      r = compile_range_sedad(sa, begin, end);
      if( !r ) {
	errormsg(E_FATAL, "bad address %.*s\n", end - begin, begin);
      }
    } else {
      r = begin;
    }
    if( *r == '!' ) {
      setflag(&sa->flags,SEDAD_FLAG_INVERT);
      r++;
    }
  }
  return r;
}


bool_t create_sedcmd(sedcmd_t *sc, sedcmdid_t id, sedadid_t ad) {
  if( sc ) {
    memset(sc, 0, sizeof(sedcmd_t));
    sc->id = id;
    return create_sedad(&sc->address, ad);
  }
  return FALSE;
}

bool_t free_sedcmd(sedcmd_t *sc) {
  if( sc ) {
    free_sedad(&sc->address);
    if( sc->id == SUBSTITUTE ) {
      regfree(&sc->args.s.find);
      free_cstring(&sc->args.s.replace);
    } else if( sc->id == TRANSLITERATE ) {
      if( sc->args.y.from ) { free(sc->args.y.from); }
      if( sc->args.y.to ) { free(sc->args.y.to); }
    } else if( (sc->id == APPEND) || 
	       (sc->id == REPLACE) || 
	       (sc->id == INSERT) ||
	       (sc->id == GOTO) ) {
      free_cstring(&sc->args.lit.string);
    } else if( (sc->id == READF) ||
	       (sc->id == WRITEF) ) {
      free_cstring(&sc->args.file.name);
    }
    return TRUE;
  }
  return FALSE;
}

const char_t *parse_double_delim(const char_t *begin, const char_t *end,
				 char_t delim, char_t esc,
				 char_t **b1, char_t **e1, 
				 char_t **b2, char_t **e2) {
  const char_t *r;
  char_t delims[2] = {0};
  delims[0] = delim;

  if( begin && (begin < end) && b1 && e1 && b2 && e2) {
    if( *begin == delim ) {
      begin++;
      r = skip_unescaped_delimiters(begin, end, delims, esc);
      if( (r < end) && (*r == delim) ) {
	*b1 = (char_t *)begin;
	*e1 = (char_t *)r;
	begin = r + 1;
	r = skip_unescaped_delimiters(begin, end, delims, esc);
	if( (r < end) && (*r == delim) ) {
	  *b2 = (char_t *)begin;
	  *e2 = (char_t *)r;
	  return (r + 1);
	}
      }
    }
  }
  return NULL;
}

const char_t *compile_substitution(sedcmd_t *sc, const char_t *begin, const char_t *end) {
  const char_t *r = NULL;
  char_t *b1, *e1, *b2, *e2;
  char_t delim;
  int flags;
  if( sc && begin && (begin < end) ) {
    if( *begin == 's' ) {
      begin++;
      delim = *begin;
      r = parse_double_delim(begin, end, delim, escc, &b1, &e1, &b2, &e2);
      if( r ) {
	sc->id = SUBSTITUTE;

	*e2 = '\0';
	b2 = (char *)strdup_cstring(&sc->args.s.replace, b2);
	/* sc->args.s.replace =  */
	/*   filter_escaped_chars(sc->args.s.replace,  */
	/* 		       sc->args.s.replace, "/", '\\'); */
	b2 = filter_escaped_chars(b2, b2, "/", escc);

	flags = 0;
	for(r++,e2++; e2 && (e2 < end); r++,e2++) {
	  if( *e2 == 'i' ) {
	    setflag(&sc->flags,SED_FLAG_SUBSTITUTE_ICASE);
	    flags |= REG_ICASE;
	  } else if( *e2 == 'g' ) {
	    setflag(&sc->flags,SED_FLAG_SUBSTITUTE_ALL);
	  } else if( *e2 == 'z' ) {
	    setflag(&sc->flags,SED_FLAG_SUBSTITUTE_PATH_STRINGVAL);
	  } else if( *e2 == 'x' ) {
	    setflag(&sc->flags,SED_FLAG_SUBSTITUTE_PATH);
	  } else {
	    r = e2;
	    break;
	  }
	}
	
	if( !checkflag(sc->flags,
		       SED_FLAG_SUBSTITUTE_PATH|
		       SED_FLAG_SUBSTITUTE_PATH_STRINGVAL) ) {
	  /* \n are significant if we're only matching stringval,
	   * otherwise the search string is mathed as a whole. 
	   */
	  flags |= REG_NEWLINE;
	}

	*e1 = '\0';
	if( 0 != regcomp(&sc->args.s.find, b1, flags) ) {
	  errormsg(E_ERROR, "bad regular expression.\n");
	  sc->id = NOP;
	  return NULL;
	}

      }
    }
  }
  return r;
}

const char_t *compile_transliteration(sedcmd_t *sc, const char_t *begin, const char_t *end) {
  const char_t *r = NULL;
  char_t *b1, *e1, *b2, *e2;
  char_t delim;
  int flags, l;
  if( sc && begin && (begin < end) ) {
    if( *begin == 'y' ) {
      begin++;
      delim = *begin;
      r = parse_double_delim(begin, end, delim, escc, &b1, &e1, &b2, &e2);
      if( r ) {
	sc->id = TRANSLITERATE;

	*e1 = '\0';
	*e2 = '\0';

	flags = 0;
	for(r++,e2++; e2 && (e2 < end); r++,e2++) {
	  if( *e2 == 'z' ) {
	    setflag(&sc->flags,SED_FLAG_TRANSLITERATE_PATH_STRINGVAL);
	  } else if( *e2 == 'x' ) {
	    setflag(&sc->flags,SED_FLAG_TRANSLITERATE_PATH);
	  } else {
	    break;
	  }
	}
	
	l = mbstowcs(NULL, b1, 0);
	sc->args.y.from = malloc((l+1) * sizeof(wchar_t));
	if( l != mbstowcs(sc->args.y.from, b1, l+1) ) {
	  errormsg(E_ERROR, "transliteration cannot be performed.\n");
	  sc->id = NOP;
	  return NULL;
	}

	l = mbstowcs(NULL, b2, 0);
	sc->args.y.to = malloc((l+1) * sizeof(wchar_t));
	if( l != mbstowcs(sc->args.y.to, b2, l+1) ) {
	  errormsg(E_ERROR, "transliteration cannot be performed.\n");
	  sc->id = NOP;
	  return NULL;
	}

      }
    }
  }
  return r;
}

const char_t *compile_file(sedcmd_t *sc, const char_t *begin, const char_t *end) {
  const char_t *r = NULL;
  if( sc && begin && (begin < end) ) {
    if( begin[1] == ' ' ) {
      switch(*begin) {
      case 'r': sc->id = READF; break;
      case 'w': sc->id = WRITEF; break;
      default: 
	errormsg(E_ERROR, "unrecognized command %c\\", *begin);
	return NULL;
      }
      r = skip_unescaped_delimiters(begin + 2, end, " \t\n;", '\0');
      create_cstring(&sc->args.file.name, begin + 2, r - (begin + 2) );
    }
  }
  return r;
}

const char_t *compile_literal(sedcmd_t *sc, const char_t *begin, const char_t *end) {
  const char_t *q, *r = NULL;
  if( sc && begin && (begin < end) ) {
    switch(*begin) {
    case 'a': sc->id = APPEND; break;
    case 'c': sc->id = REPLACE; break;
    case 'i': sc->id = INSERT; break;
    default: 
      errormsg(E_ERROR, "unrecognized command %c\\", *begin);
      return NULL;
    }
    r = skip_unescaped_delimiters(begin + 1, end, "\n", '\\');
    q = create_cstring(&sc->args.lit.string, begin + 1, r - (begin + 1) );
    filter_escaped_chars( (char *)q, q, "\n", '\\');
  }
  return r;
}

const char_t *compile_label(sedcmd_t *sc, const char_t *begin, const char_t *end) {
  const char_t *r = NULL;
  if( sc && begin && (begin < end) ) {
    if( (*begin == 'b') || (*begin == ':') || (*begin == 't') ) {
      r = skip_unescaped_delimiters(begin, end, " \t\n;", '\0');
      if( r > begin ) {

	sc->id = ( (*begin == 'b') ? GOTO : 
		   (*begin == 't') ? TEST : LABEL );
	create_cstring(&sc->args.lit.string, begin + 1, r - (begin + 1));
      }
    }
  }
  return r;
}

const char_t *compile_sedcmd(sedcmd_t *sc, const char_t *begin, const char_t *end) {
  const char_t *r = NULL;
  if( sc && begin && (begin < end) ) {
    switch(*begin) {
    case '#': 
      r = end; 
      break;
    case '{': 
      sc->id = OPENBLOCK;
      r = begin + 1;
      break;
    case '}':
      sc->id = CLOSEBLOCK;
      r = begin + 1;
      break;
    case 'a':
    case 'c':
    case 'i':
      r = compile_literal(sc, begin, end);
      break;
    case 'b':
    case ':':
    case 't':
      r = compile_label(sc, begin, end);
      break;
    case 'd':
      sc->id = DELP;
      r = begin + 1;
      break;
    case 'D':
      sc->id = DELJ;
      r = begin + 1;
      break;
    case 'g':
      sc->id = PASTEH;
      r = begin + 1;
      break;
    case 'G':
      sc->id = PASTEHA;
      r = begin + 1;
      break;
    case 'h':
      sc->id = COPYH;
      r = begin + 1;
      break;
    case 'H':
      sc->id = COPYHA;
      r = begin + 1;
      break;
    case 'l':
      sc->id = LISTP;
      r = begin + 1;
      break;
    case 'n':
      sc->id = NEXT;
      r = begin + 1;
      break;
    case 'N':
      sc->id = JOIN;
      r = begin + 1;
      break;
    case 'p':
      sc->id = PRINTP;
      r = begin + 1;
      break;
    case 'P':
      sc->id = PRINTJ;
      r = begin + 1;
      break;
    case 'q':
      sc->id = QUIT;
      r = begin + 1;
      break;
    case 's':
      r = compile_substitution(sc, begin, end);
      break;
    case 'x':
      sc->id = SWAPH;
      r =begin + 1;
      break;
    case 'y':
      r = compile_transliteration(sc, begin, end);
      break;
    }
  }
  return r;
}

bool_t create_sclist(sclist_t *scl) {
  if( scl ) {
    scl->num = 0;
    return grow_mem(&scl->list, &scl->max, sizeof(sedcmd_t), 16);
  }
  return FALSE;
}

bool_t free_sclist(sclist_t *scl) {
  if( scl ) {
    if( scl->list ) {
      reset_sclist(scl);
      free_mem(&scl->list, &scl->max);
    }
  }
  return FALSE;
}

bool_t reset_sclist(sclist_t *scl) {
  size_t i;
  if( scl ) {
    for(i = 0; i < scl->num; i++) {
      free_sedcmd(&scl->list[i]);
    }
    scl->num = 0;
    return TRUE;
  }
  return FALSE;
}

sedcmd_t *get_sclist(sclist_t *scl, size_t n) {
  return scl && (n < scl->num) ? &scl->list[n] : NULL;
}

sedcmd_t *get_last_sclist(sclist_t *scl) {
  return scl ?  get_sclist(scl, scl->num - 1) : NULL;
}

bool_t add_sclist(sclist_t *scl) {
  if( scl ) {
    if( scl->num >= scl->max ) {
      grow_mem(&scl->list, &scl->max, sizeof(sedcmd_t), 16);
    }
    if( scl->num < scl->max ) {
      if( create_sedcmd(&scl->list[scl->num], NOP, ANY) ) {
	scl->num++;
	return TRUE;
      }
    }
  }
  return FALSE;
}

bool_t pop_sclist(sclist_t *scl) {
  if( scl ) {
    if( scl->num > 0 ) {
      scl->num--;
      free_sedcmd(&scl->list[scl->num]);
      return TRUE;
    }
  }
  return FALSE;
}

#include <stdio.h>

/* this is for debugging only */
void print_sclist(sclist_t *scl) {
  int i;
  if( scl ) {
    for(i = 0; i < scl->num; i++) {
      switch(scl->list[i].address.id) {
      case ANY: printf("ANY"); break;
      case PTR: printf("PTR"); break;
      case INTERVAL: printf("INTERVAL[%ld,%ld)", 
			    scl->list[i].address.args.range.line1,
			    scl->list[i].address.args.range.line2); break;
      default: printf("???"); break;
      }
      printf("\t");
      switch(scl->list[i].id) {
      case NOP: printf("NOP"); break;
      case OPENBLOCK: printf("OPENBLOCK"); break;
      case CLOSEBLOCK: printf("CLOSEBLOCK"); break;
      case SUBSTITUTE: printf("FINDREP(%p,%s)",
			      (void*)&scl->list[i].args.s.find,
			      p_cstring(&scl->list[i].args.s.replace)); 
	break;
      case TRANSLITERATE: printf("TRANSLIT"); break;
      default: printf("???"); break;
      }
      printf("\t%d\n", scl->list[i].flags);
    }
  }
}

