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
#include "wrap.h"
#include "stdout.h"
#include "leafparse.h"
#include "entities.h"
#include "unecho.h"
#include "mem.h"
#include "sed.h"
#include "objstack.h"
#include "echo.h"
#include "filelist.h"
#include "stringlist.h"
#include "mysignal.h"

#include <string.h>
#include <getopt.h>
#include <limits.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

/* for option processing */
extern char *optarg;
extern int optind, opterr, optopt;

extern char *progname;
extern char *inputfile;
extern long inputline;

extern volatile flag_t cmd;
extern const char_t escc;
extern const wchar_t Lescc;
extern const char_t xpath_starts[];
extern const char_t xpath_ends[];

#include <stdio.h>

typedef struct {
  int ic; /* instruction counter */
  flag_t flags;
  sclist_t commands;
  tempvar_t patternsp;
  tempvar_t holdsp;
  stringlist_t prepend;
  stringlist_t append;

  xpath_t tmpath;
  /* tempvar_t tmpvar; */
} sedvm_t;

#define SEDVM_FLAG_AUTOPRINT   0x02
#define SEDVM_FLAG_QUIT        0x04
#define SEDVM_FLAG_JOIN        0x08
#define SEDVM_FLAG_OKSUB       0x10
#define SEDVM_FLAG_NOAUTOEXEC  0x20

bool_t create_sedvm(sedvm_t *vm) {
  bool_t ok = TRUE;
  if( vm ) {

    memset(vm, 0, sizeof(sedvm_t));

    vm->flags = SEDVM_FLAG_AUTOPRINT;

    ok &= create_sclist(&vm->commands);
    ok &= create_tempvar(&vm->patternsp, "pat", MINVARSIZE, MAXVARSIZE);
    ok &= create_tempvar(&vm->holdsp, "holly", MINVARSIZE, MAXVARSIZE);
    ok &= create_stringlist(&vm->prepend);
    ok &= create_stringlist(&vm->append);
    ok &= create_xpath(&vm->tmpath);
    /* ok &= create_tempvar(&vm->tmpvar, "tempy", */
    /* 			 MINVARSISE, MAXVARSIZE); */

    return ok;
  }
  return FALSE;
}


bool_t free_sedvm(sedvm_t *vm) {
  if( vm ) {
    free_sclist(&vm->commands);
    free_tempvar(&vm->patternsp);
    free_tempvar(&vm->holdsp);
    free_stringlist(&vm->prepend);
    free_stringlist(&vm->append);
    free_xpath(&vm->tmpath);
    /* free_tempvar(&vm->tmpvar); */
  }
  return FALSE;
}

typedef struct {
  leafparserinfo_t lfp; /* must be first */
  echo_t echo;
  flag_t flags;

  sedvm_t vm;

  objstack_t default_addresses; 

  cstringlst_t files;
  cstringlst_t *xpaths;
  int n;

} parserinfo_sed_t;

#define SED_VERSION    0x01
#define SED_HELP       0x02
#define SED_DEBUG      0x03
#define SED_NONEMPTY   0x04
#define SED_SCRIPT     0x08

#define SED_USAGE \
"Usage: xml-sed [OPTION]... SCRIPT [[FILE] [:XPATH]...]\n" \
"For each leaf node in FILE or STDIN, perform basic text and XML transformations in SCRIPT.\n" \
"\n" \
"      --help     display this help and exit\n" \
"      --version  display version information and exit\n"

#define SED_FLAG_DEBUG       0x01



const char_t *compile_command(parserinfo_sed_t *pinfo, const char_t *begin, const char_t *end) {
  sedcmd_t *c;
  sedad_t *pad;
  const char_t *p;
  if( pinfo && begin && (begin < end) ) {

    if( add_sclist(&pinfo->vm.commands) ) {

      c = get_last_sclist(&pinfo->vm.commands);

      begin = skip_xml_whitespace(begin, end);

      /* extract optional address */
      p = compile_sedad(&c->address, begin, end);
      if( !p ) {
	pop_sclist(&pinfo->vm.commands);
	return NULL;
      } 

      if( p == begin ) { /* no address */
	if( peek_objstack(&pinfo->default_addresses, 
			  (byte_t *)&c->address.args.psedad, sizeof(sedad_t *)) ) {
	  c->address.id = PTR;
	} else {
	  c->address.id = ANY;
	}
      } else if( p > begin ) { /* found an address */
	begin = p;
      }

      begin = skip_xml_whitespace(begin, end);

      p = compile_sedcmd(c, begin, end);
      if( !p ) {
	pop_sclist(&pinfo->vm.commands);
	return NULL;
      }

      if( p == begin ) { /* no command */
	return NULL;
      } else { /* found command */

	if( c->id == OPENBLOCK ) {
	  pad = &c->address;
	  if( !push_objstack(&pinfo->default_addresses, 
			     (byte_t *)&pad, sizeof(sedad_t *)) ) {
	    errormsg(E_FATAL, "ran out of stack space.\n");
	  }
	} else if( c->id == CLOSEBLOCK ) {
	  if( !pop_objstack(&pinfo->default_addresses, sizeof(sedad_t *)) ) {
	     errormsg(E_ERROR, "block count mismatch.\n");
	     return FALSE;
	  }
	}

	begin = p;
      }

      begin = skip_xml_whitespace(begin, end);
      return begin;
    }
  }
  return NULL;
}

bool_t compile_script(parserinfo_sed_t *pinfo,
		      const char_t *begin, const char_t *end) {
  const char_t *stop, *p;

  if( pinfo && begin && end ) {
    setflag(&pinfo->flags,SED_SCRIPT);
    while( begin < end ) {

      stop = skip_unescaped_delimiters(begin, end, ";{}\n", escc);
      if( (*stop == '{') || (*stop == '}') ) {
	stop++;
      }
      begin = skip_xml_whitespace(begin, stop); 
      if( begin < stop ) {
	p = compile_command(pinfo, begin, stop);
	if( !p ) {
	  *((char_t *)stop) = '\0';
	  errormsg(E_FATAL, "syntax: %s\n", begin);
	} 
	begin = p;
      }
      if( (*begin == ';') || (*begin == '\n') ) {
	begin++;
      }

    }
    /* print_sclist(&pinfo->commands); */
    if( !is_empty_objstack(&pinfo->default_addresses) ) {
      errormsg(E_FATAL, "unclosed block.\n");
    }

    return TRUE;
  }
  return FALSE;
}

bool_t check_regex_match(const char_t *s, size_t pos, const regex_t *re, 
			 int nmatch, regmatch_t pmatch[], flag_t flags) {
  const char_t *d;
  bool_t r = FALSE;

  if( s && re ) {

    if( checkflag(flags,SED_FLAG_SUBSTITUTE_PATH_STRINGVAL) ) {

      s += pos;
      r = (0 == regexec(re, (char *)s, nmatch, pmatch, 0)) ? TRUE : FALSE;
      
    } else if( checkflag(flags,SED_FLAG_SUBSTITUTE_PATH) ) {

      d = skip_unescaped_delimiters(s, NULL, xpath_ends, escc);

      if( pos == 0 ) { pos++; } /* skip initial '[' */
      if( s + pos >= d ) { return FALSE; }
      s += pos;

      *(char_t *)d = '\0';

      r = (0 == regexec(re, (char *)s, nmatch, pmatch, 0)) ? TRUE : FALSE;
      
      *(char_t *)d = *xpath_ends;

    } else { /* match only stringval */

      d = skip_unescaped_delimiters(s, NULL, xpath_ends, escc);
      d++;

      pos = ( s + pos < d ) ? (d - s) : pos;
      s += pos;
      r = (0 == regexec(re, (char *)s, nmatch, pmatch, 0)) ? TRUE : FALSE;

    }

    while(--nmatch >= 0) {
      if( pmatch[nmatch].rm_so != -1 ) {
	pmatch[nmatch].rm_so += pos * sizeof(char_t);
	pmatch[nmatch].rm_eo += pos * sizeof(char_t);
      }
    }

  }

  return r;
}

/* substitute the regex match (0) with c->replace, interpolating
 * unescaped & with match (0), and \i with match (i) 
 */
bool_t exec_substitution(sedcmd_t *c, size_t *pos, tempvar_t *from, tempvar_t *to) {

  regmatch_t pmatch[10] = { {0} };
  bool_t ok = TRUE;
  const char_t *p, *q, *r, *s;
  int i;
  
  if( c && pos && from && to ) {

    if( peeks_tempvar(from, 0, &s) ) {
      
      if( check_regex_match(s, *pos, &c->args.s.find, 10, pmatch, c->flags) &&
      	  (pmatch[0].rm_so != -1) ) {

	/* printf("exec_subst(%s){%s}\n", p_cstring(&c->args.s.replace), s); */
	reset_tempvar(to);

	if( pmatch[0].rm_so > 0 ) {
	  ok &= write_tempvar(to, (byte_t *)s, pmatch[0].rm_so);
	}

	p = q = r = begin_cstring(&c->args.s.replace);
	i = -1;
	while( *p ) {

	  if( *p == '&' ) {
	    i = 0;
	  } else if( *p == escc ) {
	    p++;
	    if( xml_isdigit(*p) ) {
	      i = (toascii(*p) - 0x30);
	    } else {
	      r++;
	    }
	  } 

	  if( i >= 0 ) {
	    if( r > q ) {
	      ok &= write_tempvar(to, (byte_t *)q, r - q);
	    }
	    p++;
	    q = r = p;
	    if( pmatch[i].rm_so != -1 ) {
	      ok &= write_tempvar(to, (byte_t *)(s + pmatch[i].rm_so), 
				  pmatch[i].rm_eo - pmatch[i].rm_so); 
	    }
	    i = -1;
	  } else {
	    p++;
	    r++;
	  }

	}
	
	if( r > q ) {
	  ok &= write_tempvar(to, (byte_t *)q, r - q);
	}

	*pos = tell_tempcollect((tempcollect_t *)to);

	s += pmatch[0].rm_eo;
	if( *s ) {
	  ok &= puts_tempvar(to, s);
	}

	/* puts_stdout("gotmatch:"); */
	/* write_stdout_tempcollect((tempcollect_t*)to); */
	/* putc_stdout('\n'); */
	/* nprintf_stdout(10, "ok = %d\n", ok); */
	return ok;
      }

    }
  }
  return FALSE;
}

bool_t exec_transliteration(sedcmd_t *c, tempvar_t *from, tempvar_t *to) {
  const char_t *s, *b;
  char_t r[MB_LEN_MAX+1];
  wchar_t w;
  wchar_t *z, *y;
  mbstate_t mbs;
  int n;
  bool_t inpath = FALSE;

  if( c && from && to ) {
    if( peeks_tempvar(from, 0, &s) ) {

      memset(&mbs, 0, sizeof(mbstate_t));
      reset_tempvar(to);

      b = skip_unescaped_delimiters(s, NULL, "[]", escc);

      while( *s ) {
	
	if( s == b ) {
	  inpath = (*s == *xpath_starts);
	  b = skip_unescaped_delimiters(s + 1, NULL, "[]", escc);
	} 

	n = mbrtowc(&w, s, MB_LEN_MAX, &mbs);
	if( n < -1 ) {
	  s++; /* just ignore */
	  continue;
	} else {
	  s += n;
	}
	if( (inpath && checkflag(c->flags, SED_FLAG_TRANSLITERATE_PATH|SED_FLAG_TRANSLITERATE_PATH_STRINGVAL)) ||
	    (!inpath && !checkflag(c->flags, SED_FLAG_TRANSLITERATE_PATH)) ) {
	  /* transliterate */
	  for(z = c->args.y.from, y = c->args.y.to; *z; z++, y++) {
	    if( *z == Lescc ) { z++; }
	    if( *y == Lescc ) { y++; }
	    if( *z == w ) {
	      w = *y;
	      break;
	    }
	  }
	}
	n = wctomb(r, w);
	write_tempvar(to, (byte_t *)r, n);
      }

      return TRUE;
    }
  }
  return FALSE;
}

bool_t check_address(sedad_t *ad, long lineno) {
  bool_t retval = FALSE;
  if( ad ) {
    if( ad->id == ANY ) {
      retval = TRUE;
    } else if( ad->id == PTR ) {
      retval = check_address(ad->args.psedad, lineno); 
    } else if( ad->id == INTERVAL ) {
      retval = ( (ad->args.range.line1 <= lineno) &&
		 (lineno < ad->args.range.line2) );
    }
    retval = ( checkflag(ad->flags,SEDAD_FLAG_INVERT) ? !retval : retval);
  }
  return retval;
}

/* Since output starts with an absolute path, the default behaviour
   of echo_t is to close all open tags and then open the absolute
   path. What we want is to close and open only the minimal number of
   tags so that the echo_t path is synchronized with output. */
bool_t echo_relative_string(echo_t *echo, const char_t *s, 
			    xpath_t *tmp1, xpath_t *tmp2) {
  const char_t *d;
  if( echo && s && tmp1 && tmp2 ) {

    d = s;

    if( *s == *xpath_starts ) {
      s++;
      d = skip_unescaped_delimiters(s, NULL, xpath_ends, escc);
      reset_xpath(tmp1);

      if( write_xpath(tmp1, s, d - s) ) {
	copy_xpath(tmp2, &echo->xpath);
	retarget_xpath(tmp2, tmp1);
	open_relpath_stdout_echo(echo, tmp2);
      }
      d++;
    }

    puts_stdout_echo(echo, d);

    return TRUE;
  }
  return FALSE;
}

bool_t echo_relative(echo_t *echo, tempvar_t *output, 
		     xpath_t *tmp1, xpath_t *tmp2) {
  const char_t *s;
  if( peeks_tempvar(output, 0, &s) ) {
    return echo_relative_string(echo, s, tmp1, tmp2);
  }
  return FALSE;
}

int goto_label(sedvm_t *vm, const char_t *label) {
  int i = 0;
  sedcmd_t *c;
  if( *label ) {
    for(; i < vm->commands.num; i++) {
      c = &vm->commands.list[vm->ic];
      if( (c->id == LABEL) && 
	  (strcmp(p_cstring(&c->args.lit.string),label) == 0) ) {
	return i;
      } 
    }
  }
  return i;
}

bool_t list_pattern_stdout_sedvm(sedvm_t *vm, lineinfo_t *line) {
  bool_t ok = TRUE;
  if( vm && line ) {
    ok &= puts_stdout("\n<!-- ");
    ok &= nprintf_stdout(20, "%ld%c ", line->no, line->selected ? '*' : ' ');
    ok &= ( is_empty_tempcollect(&vm->patternsp.tc) ||
	    write_stdout_tempcollect(&vm->patternsp.tc) );
    ok &= puts_stdout(" -->\n");
  }
  return ok;
}

/* an echo chunk looks like this "[tags]chardata" */
bool_t del_echo_chunk(tempvar_t *tv) {
  const char_t *s, *r;
  if( tv ) {
    if( peeks_tempvar(tv, 0, &s) && *s ) {
      r = skip_unescaped_delimiters(s + 1, NULL, xpath_starts, escc);
      shift_tempvar(tv, r - s);
      return TRUE;
    }
  }
  return FALSE;
}

bool_t print_echo_chunk(sedvm_t *vm, echo_t *echo, tempvar_t *tv) {
  const char_t *s;
  char_t *r;
  char_t c;
  bool_t ok;
  if( vm && echo && tv ) {
    if( peeks_tempvar(tv, 0, &s) && *s ) {
      r = (char_t *)skip_unescaped_delimiters(s + 1, NULL, xpath_starts, escc);
      c = *r;
      *r = '\0';
      ok = echo_relative_string(echo, s, &echo->tmpath, &vm->tmpath);
      *r = c;
      return ok;
    }
  }
  return FALSE;
}

/* this function performs a single sed execution cycle,
 * starting at the instruction number ic. On entry, the pattern space
 * has been filled with new input line lineno.
 */
bool_t exec_sedvm(sedvm_t *vm, echo_t *output, tempvar_t *tmpvar,
		  lineinfo_t *line) {
  sedcmd_t *c;
  size_t pos;
  bool_t ok = TRUE;

  if( vm && output ) {

    clearflag(&vm->flags, SEDVM_FLAG_OKSUB);

    for( ; vm->ic < vm->commands.num; vm->ic++ ) {

      c = &vm->commands.list[vm->ic];
      if( !check_address(&c->address, line->no) ) {
	continue;
      }

      switch(c->id) {
      case SUBSTITUTE:
	pos = 0; 
	while( exec_substitution(c, &pos, &vm->patternsp, tmpvar) ) { 
	  swap_tempvar(tmpvar, &vm->patternsp);
	  setflag(&vm->flags, SEDVM_FLAG_OKSUB);
	  if( !checkflag(c->flags, SED_FLAG_SUBSTITUTE_ALL) ) {
	    break;
	  }
	}
	break;
      case TRANSLITERATE:
	if( exec_transliteration(c, &vm->patternsp, tmpvar) ) {
	  swap_tempvar(tmpvar, &vm->patternsp);
	  setflag(&vm->flags, SEDVM_FLAG_OKSUB);
	}
	break;
      case TEST:
	if( checkflag(vm->flags, SEDVM_FLAG_OKSUB) ) {
	  vm->ic = goto_label(vm, p_cstring(&c->args.lit.string));
	}
	break;
      case QUIT:
	setflag(&vm->flags,SEDVM_FLAG_QUIT);
	return TRUE;
      case DELP:
	reset_tempvar(&vm->patternsp);
	vm->ic = vm->commands.num; /* finish this cycle */
	break;
      case PRINTP:
	ok &= echo_relative(output, &vm->patternsp, &output->tmpath, &vm->tmpath);
	break;
      case APPEND:
	add_stringlist(&vm->append, p_cstring(&c->args.lit.string), STRINGLIST_DONTFREE);
	break;
      case REPLACE:
	reset_tempvar(&vm->patternsp);
	if( (line->typ != lt_last) && (c->address.id == INTERVAL) && 
	    ( (line->no + 1) < c->address.args.range.line2 ) ) {
	  /* don't change the ic, so on next cycle we start here */
	  setflag(&vm->flags, SEDVM_FLAG_NOAUTOEXEC);
	  return TRUE;
	}
	puts_tempvar(&vm->patternsp, p_cstring(&c->args.lit.string));
	vm->ic = vm->commands.num; /* finish this cycle */
	break;
      case INSERT:
	add_stringlist(&vm->prepend, p_cstring(&c->args.lit.string), STRINGLIST_DONTFREE);
	/* reset_tempvar(tmpvar); */
	/* puts_tempvar(tmpvar, p_cstring(&c->args.lit.string)); */
	/* ok &= echo_relative(output, tmpvar, &output->tmpath, &vm->tmpath); */
	break;
      case GOTO:
	vm->ic = goto_label(vm, p_cstring(&c->args.lit.string));
	break;
      case COPYH:
	reset_tempvar(&vm->holdsp);
	/* fall through */
      case COPYHA:
	puts_tempvar(&vm->holdsp, string_tempvar(&vm->patternsp));
	break;
      case PASTEH:
	reset_tempvar(&vm->patternsp);
	/* fall through */
      case PASTEHA:
	puts_tempvar(&vm->patternsp, string_tempvar(&vm->holdsp));
	break;
      case SWAPH:
	swap_tempvar(&vm->patternsp, &vm->holdsp);
	break;
      case NEXT:
	ok &= echo_relative(output, &vm->patternsp, &output->tmpath, &vm->tmpath);
	reset_tempvar(&vm->patternsp);
	vm->ic++;
	setflag(&vm->flags, SEDVM_FLAG_NOAUTOEXEC);
	return TRUE;
      case LISTP:
	ok &= list_pattern_stdout_sedvm(vm, line);
	break;
      case READF:
	ok &= read_from_file_tempvar(&vm->patternsp, p_cstring(&c->args.file.name));
	break;
      case WRITEF:
	ok &= write_to_file_tempvar(&vm->patternsp, p_cstring(&c->args.file.name),
				    O_CREAT|O_WRONLY|O_APPEND);
	break;
      case JOIN:
	setflag(&vm->flags,SEDVM_FLAG_JOIN);
	vm->ic++;
	setflag(&vm->flags,SEDVM_FLAG_NOAUTOEXEC);
	return TRUE;
      case DELJ:
	del_echo_chunk(&vm->patternsp);
	break;
      case PRINTJ:
	print_echo_chunk(vm, output, &vm->patternsp);
	break;
      default:
	break;
      }

      if( !ok ) {
	errormsg(E_FATAL, "runtime failure at command %d\n", vm->ic);
      }

    }

    vm->ic = 0; /* for next time */
    return TRUE;
  }
  return FALSE;
}

bool_t autoexec_sedvm(sedvm_t *vm, echo_t *output, tempvar_t *tmpvar,
		      lineinfo_t *line) {
  bool_t ok = TRUE;
  int i;
  if( vm ) {

    if( true_and_clearflag(&vm->flags, SEDVM_FLAG_NOAUTOEXEC) ) {
      return TRUE;
    }

    if( !checkflag(vm->flags, SEDVM_FLAG_AUTOPRINT) ) {
      reset_tempvar(&vm->patternsp);
    }

    if( vm->prepend.num > 0 ) {
      reset_tempvar(tmpvar);
      for(i = 0; i < vm->prepend.num; i++) {
	puts_tempvar(tmpvar, get_stringlist(&vm->prepend, i));
      }
      reset_stringlist(&vm->prepend);

      ok &= echo_relative(output, tmpvar, &output->tmpath, &vm->tmpath);
    }

    if( !is_empty_tempvar(&vm->patternsp) ) {
      ok &= echo_relative(output, &vm->patternsp, &output->tmpath, &vm->tmpath);
    }

    if( vm->append.num > 0 ) {
      reset_tempvar(&vm->patternsp);
      for(i = 0; i < vm->append.num; i++) {
	puts_tempvar(&vm->patternsp, get_stringlist(&vm->append, i));
      }
      reset_stringlist(&vm->append);

      ok &= echo_relative(output, &vm->patternsp, &output->tmpath, &vm->tmpath);
    }

    if( checkflag(vm->flags, SEDVM_FLAG_QUIT) ) {
      close_stdout_echo(output);
      return TRUE;
    }

    if( !ok ) {
      errormsg(E_FATAL, "autoexec echo failure.\n");
    }

  }
  return TRUE;
}


result_t do_leaf_node(void *user, unecho_t *ue) {
  tempvar_t *tmp;
  parserinfo_sed_t *pinfo = (parserinfo_sed_t *)user;
  if( pinfo && ue ) {

    /* puts_stdout("do_leaf_node"); */
    /* puts_stdout(p_cstring(&pinfo->lfp.cp.path)); */
    /* putc_stdout('\n'); */
    /* write_stdout_tempcollect((tempcollect_t*)&ue->sv); */
    /* putc_stdout('\n'); */


    if( true_and_clearflag(&pinfo->vm.flags, SEDVM_FLAG_JOIN) ) {
      if( !puts_tempvar(&pinfo->vm.patternsp, string_tempvar(&ue->sv)) ) {
	errormsg(E_FATAL, "out of pattern space.\n");
      }
    } else {
      /* fast copy ue into patternsp */
      swap_tempvar(&pinfo->vm.patternsp, &ue->sv);
    }
    /* use now obsolete ue as a temporary */
    tmp = &ue->sv;

    /* puts_stdout("pre_exec"); */
    /* write_stdout_tempcollect((tempcollect_t*)&pinfo->vm.patternsp); */
    /* putc_stdout('\n'); */

    if( !pinfo->lfp.sel.active ||
	(pinfo->lfp.sel.active && /* only exec script on active leaf nodes */
	 exec_sedvm(&pinfo->vm, &pinfo->echo, tmp, &pinfo->lfp.line)) ) {
      
      if( checkflag(pinfo->flags, SED_FLAG_DEBUG) ) {
	list_pattern_stdout_sedvm(&pinfo->vm, &pinfo->lfp.line);
      }

      autoexec_sedvm(&pinfo->vm, &pinfo->echo, tmp, &pinfo->lfp.line);

    }

  }
  return checkflag(pinfo->vm.flags,SEDVM_FLAG_QUIT) ? 
    PARSER_ABORT : PARSER_OK;
}

result_t do_headwrap(void *user, const char_t *root, tempcollect_t *wrap) {
  parserinfo_sed_t *pinfo = (parserinfo_sed_t *)user;
  if( pinfo && root && wrap ) {
    /* modify wrap if you want to change the headwrapper */
    write_stdout_tempcollect(wrap);
  }
  return PARSER_OK;
}

result_t do_footwrap(void *user, const char_t *root) {
  parserinfo_sed_t *pinfo = (parserinfo_sed_t *)user;
  if( pinfo ) {
    close_stdout_echo(&pinfo->echo); 
    /* the actual footwrapper from the input file is not available,
     * because the XML parser stops at the closing tag. 
     */
    puts_stdout(get_footwrap()); 
  }
  return PARSER_OK;
}

bool_t create_parserinfo_sed(parserinfo_sed_t *pinfo) {
  bool_t ok = TRUE;
  if( pinfo ) {

    memset(pinfo, 0, sizeof(parserinfo_sed_t));
    ok &= create_leafparserinfo(&pinfo->lfp);

    pinfo->flags = 0;

    pinfo->lfp.setup.flags = 0;
    pinfo->lfp.setup.cb.leaf_node = do_leaf_node;
    pinfo->lfp.setup.cb.headwrap = do_headwrap;
    pinfo->lfp.setup.cb.footwrap = do_footwrap;

    setflag(&pinfo->lfp.setup.flags,LFP_ABSOLUTE_PATH);
    setflag(&pinfo->lfp.setup.flags,LFP_ALWAYS_CHARDATA);
    clearflag(&pinfo->lfp.setup.flags,LFP_SKIP_EMPTY);
    
    setflag(&pinfo->lfp.setup.flags,LFP_PRE_OPEN);
    setflag(&pinfo->lfp.setup.flags,LFP_PRE_CLOSE);
    clearflag(&pinfo->lfp.setup.flags,LFP_POST_OPEN);
    clearflag(&pinfo->lfp.setup.flags,LFP_POST_CLOSE);

    ok &= create_echo(&pinfo->echo, ECHO_INDENTNONE, 0);

    ok &= create_sedvm(&pinfo->vm);

    ok &= create_objstack(&pinfo->default_addresses, sizeof(sedad_t *));

    return ok;
  }
  return FALSE;
}

bool_t free_parserinfo_sed(parserinfo_sed_t *pinfo) {
  if( pinfo ) {
    free_leafparserinfo(&pinfo->lfp);
    free_echo(&pinfo->echo);
    free_sedvm(&pinfo->vm);
    free_objstack(&pinfo->default_addresses);
  }
  return FALSE;
}


bool_t read_file_script(parserinfo_sed_t *pinfo, const char *file) {
  tempvar_t tv;
  bool_t retval = FALSE;
  const char_t *begin, *end;
  if( pinfo && file && create_tempvar(&tv, "", MINVARSIZE, MAXVARSIZE) ) {
    if( read_from_file_tempvar(&tv, file) ) {
      begin = string_tempvar(&tv);
      end = begin + strlen(begin);
      retval = compile_script(pinfo, begin, end);
    }
    free_tempvar(&tv);
  }
  return retval;
}

void set_option_sed(int op, char *optarg, parserinfo_sed_t *pinfo) {
  switch(op) {
  case SED_VERSION:
    puts("xml-sed" COPYBLURB);
    exit(EXIT_SUCCESS);
    break;
  case SED_HELP:
    puts(SED_USAGE);
    exit(EXIT_SUCCESS);
    break;
  case SED_DEBUG:
    setflag(&pinfo->flags, SED_FLAG_DEBUG);
    break;
  case 'e':
    compile_script(pinfo, optarg, optarg + strlen(optarg));
    break;
  case 'f':
    read_file_script(pinfo, optarg);
    break;
  case 'n':
    clearflag(&pinfo->vm.flags, SEDVM_FLAG_AUTOPRINT);
    break;
  case SED_NONEMPTY:
    setflag(&pinfo->lfp.setup.flags, LFP_SKIP_EMPTY);
    break;
  }
}

int main(int argc, char **argv) {
  signed char op;
  parserinfo_sed_t pinfo;
  filelist_t fl;
  char_t *script = NULL;

  struct option longopts[] = {
    { "version", 0, NULL, SED_VERSION },
    { "help", 0, NULL, SED_HELP },
    { "unecho", 0, NULL, SED_DEBUG },
    { "non-empty", 0, NULL, SED_NONEMPTY },
    { 0 }
  };

  progname = "xml-sed";
  inputfile = "";
  inputline = 0;

  if( create_parserinfo_sed(&pinfo) ) {

    while( (op = getopt_long(argc, argv, "ne:f:",
			     longopts, NULL)) > -1 ) {
      set_option_sed(op, optarg, &pinfo);
    }

    init_signal_handling();
    init_file_handling();

    if( !checkflag(pinfo.flags,SED_SCRIPT) ) {
      if( !argv[optind] ) {
	puts(SED_USAGE);
	exit(EXIT_FAILURE);
      }
      script = argv[optind];
      compile_script(&pinfo, script, script + strlen(script));
      optind++;
    }

    if( create_filelist(&fl, MAXFILES, argv + optind, FILELIST_EQ1) ) {

      pinfo.files = getfiles_filelist(&fl);
      pinfo.xpaths = getxpaths_filelist(&fl);
      pinfo.n = getsize_filelist(&fl);
      
      inputfile = (char *)pinfo.files[0];

      open_stdout();
      setup_stdout(STDOUT_CHECKPARSER);

      leafparse(pinfo.files[0], pinfo.xpaths[0], (leafparserinfo_t *)&pinfo);

      close_stdout();

      free_filelist(&fl);
    }    

    exit_file_handling();
    exit_signal_handling();

    free_parserinfo_sed(&pinfo);
  }


  return EXIT_SUCCESS;
}
